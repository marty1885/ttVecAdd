
#include "common/core_coord.h"
#include "tt_metal/host_api.hpp"
#include "common/bfloat16.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <vector>

using namespace tt::tt_metal;

std::shared_ptr<Buffer> MakeBuffer(Device *device, uint32_t size, uint32_t page_size, bool sram)
{
    InterleavedBufferConfig config{
        .device= device,
        .size = size,
        .page_size = page_size,
        .buffer_type = (sram ? BufferType::L1 : BufferType::DRAM)
    };
    return CreateBuffer(config);
}

// Allocate a buffer on DRAM or SRAM. Assuming the buffer holds BFP16 data.
// A tile on TensorTile is 32x32 elements, given us using BFP16, we need 2 bytes per element.
// Making the tile size 32x32x2 = 2048 bytes.
// @param device: The device to allocate the buffer on.
// @param n_tiles: The number of tiles to allocate.
// @param sram: If true, allocate the buffer on SRAM, otherwise allocate it on DRAM.
std::shared_ptr<Buffer> MakeBufferBFP16(Device *device, uint32_t n_tiles, bool sram)
{
    constexpr uint32_t tile_size = 2 * (32 * 32);
    const uint32_t page_tiles = sram ? n_tiles : 1;
    return MakeBuffer(device, tile_size * n_tiles, page_tiles * tile_size, sram);
}

CBHandle MakeCircularBuffer(Program& program, const CoreCoord& core, tt::CB cb, uint32_t size, uint32_t page_size, tt::DataFormat format)
{
    CircularBufferConfig cb_src0_config = CircularBufferConfig(
        size,
        {{
            cb,
            tt::DataFormat::Float16_b
    }})
    .set_page_size(cb, page_size);
    return CreateCircularBuffer(program, core, cb_src0_config);
}

// Circular buffers are Tenstorrent's way of communicating between the data movement and the compute kernels.
// kernels queue tiles into the circular buffer and takes them when they are ready. The circular buffer is
// backed by SRAM. There can be multiple circular buffers on a single Tensix core. 
// @param program: The program to create the circular buffer on.
// @param core: The core to create the circular buffer on.
// @param cb: Which circular buffer to create (c_in0, c_in1, c_out0, c_out1, etc..). This is just an ID
// @param n_tiles: The number of tiles the circular buffer can hold.
CBHandle MakeCircularBufferBFP16(Program& program, const CoreCoord& core, tt::CB cb, uint32_t n_tiles)
{
    constexpr uint32_t tile_size = 2 * (32 * 32);
    return MakeCircularBuffer(program, core, cb, n_tiles * tile_size, tile_size, tt::DataFormat::Float16_b);
}


int main(int argc, char **argv)
{
    int device_id = 0;
    Device *device = CreateDevice(device_id);

    Program program = CreateProgram();
    // This example program will only use 1 Tensix core. So we set the core to {0, 0}.
    constexpr CoreCoord core = {0, 0};

    CommandQueue& cq = device->command_queue();
    const uint32_t n_tiles = 64;
    const uint32_t tile_size = 32 * 32;
    // Create 3 buffers on DRAM. These will hold the input and output data. A and B are the input buffers, C is the output buffer.
    auto a = MakeBufferBFP16(device, n_tiles, false);
    auto b = MakeBufferBFP16(device, n_tiles, false);
    auto c = MakeBufferBFP16(device, n_tiles, false);

    std::random_device rd;
    std::vector<uint32_t> a_data = create_random_vector_of_bfloat16(tile_size * n_tiles * 2, 10, rd());
    std::vector<uint32_t> b_data = create_random_vector_of_bfloat16(tile_size * n_tiles * 2, 10, rd());

    const uint32_t tiles_per_cb = 4;
    // Create 3 circular buffers. These will be used by the data movement kernels to stream data into the compute cores and for the compute cores to stream data out.
    CBHandle cb_a = MakeCircularBufferBFP16(program, core, tt::CB::c_in0, tiles_per_cb);
    CBHandle cb_b = MakeCircularBufferBFP16(program, core, tt::CB::c_in1, tiles_per_cb);
    CBHandle cb_c = MakeCircularBufferBFP16(program, core, tt::CB::c_out0, tiles_per_cb);

    EnqueueWriteBuffer(cq, a, a_data, false);
    EnqueueWriteBuffer(cq, b, b_data, false);

    // A Tensix core is made up with 5 processors. 2 data movement processors, and 3 compute processors. The 2  data movement
    // processors acts independently other cores. And the 3 compute processors acts together (hence 1 kerenl for compute).
    // There is no need to explicitly parallelize the compute kernels. Unlike traditional CPU/GPU style SPMD programming,
    // the 3 compute processors moves data from SRAM into the FPU(tensor engine)/SFPU(SIMD engine), operates on the data, and
    // move it back to SRAM. The data movement processors moves data from the NoC, or in our case, the DRAM, into the SRAM.
    // 
    // The vector add example consists of 3 kernels. `interleaved_tile_read` reads tiles from the input buffers A and B
    // into 2 circular buffers. `add` reads tiles from the circular buffers, adds them together, and dumps the result into
    // a third circular buffer. `tile_write` reads tiles from the third circular buffer and writes them to the output buffer C.
    auto reader = CreateKernel(
        program,
        "vecadd_kernels/interleaved_tile_read.cpp",
        core,
        DataMovementConfig {.processor = DataMovementProcessor::RISCV_0, .noc = NOC::RISCV_0_default}
    );
    auto writer = CreateKernel(
        program,
        "vecadd_kernels/tile_write.cpp",
        core,
        DataMovementConfig {.processor = DataMovementProcessor::RISCV_1, .noc = NOC::RISCV_1_default}
    );
    auto compute = CreateKernel(
        program,
        "vecadd_kernels/add.cpp",
        core,
        ComputeConfig{
            .math_approx_mode = false,
            .compile_args = {},
            .defines = {}
        }
    );

    // Set the runtime arguments for the kernels. This also registers
    // the kernels with the program.
    SetRuntimeArgs(program, reader, core, {
        a->address(),
        b->address(),
        n_tiles
    });
    SetRuntimeArgs(program, writer, core, {
        c->address(),
        n_tiles
    });
    SetRuntimeArgs(program, compute, core, {
        n_tiles
    });

    // We have setup the program. Now we can queue the kernel for execution.
    // The last argument to EnqueueProgram is a boolean that specifies whether
    // we wait for the program to finish execution before returning. I've set
    // it to true. But alternatively, you can set it to false and call
    // `Finish(cq)` to wait for all programs to finish.
    // But it shouldn't matter in this case since we block on reading the output
    // buffer.
    EnqueueProgram(cq, program, true);
    // Finish(cq);
    std::cout << "Kernel execution finished" << std::endl;

    // Read the output buffer.
    std::vector<uint32_t> c_data;
    EnqueueReadBuffer(cq, c, c_data, true);

    // Print partial results so we can see the output is correct (plus or minus some error due to BFP16 precision)
    std::cout << "Partial results: (note we are running under BFP16. It's going to be less accurate)\n";
    size_t n = std::min((size_t)10, (size_t)tile_size * n_tiles);
    bfloat16* a_bf16 = reinterpret_cast<bfloat16*>(a_data.data());
    bfloat16* b_bf16 = reinterpret_cast<bfloat16*>(b_data.data());
    bfloat16* c_bf16 = reinterpret_cast<bfloat16*>(c_data.data());
    for(int i = 0; i < n; i++)
        std::cout << "  " << a_bf16[i].to_float() << " + " << b_bf16[i].to_float() << " = " << c_bf16[i].to_float() << "\n";
    std::cout << std::flush;

    // Finally, we close the device.
    CloseDevice(device);
    return 0;
}