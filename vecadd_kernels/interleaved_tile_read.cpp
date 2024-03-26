#include <cstdint>

void kernel_main()
{
    uint32_t a_addr = get_arg_val<uint32_t>(0);
    uint32_t b_addr = get_arg_val<uint32_t>(1);
    uint32_t n_tiles = get_arg_val<uint32_t>(2);

    // uint32_t tile_size = 32 * 32;

    constexpr uint32_t cb_in0 = tt::CB::c_in0;
    constexpr uint32_t cb_in1 = tt::CB::c_in1;

    uint32_t tile_size = get_tile_size(cb_in0);
    const uint32_t tile_size_bytes = 32 * 32 * 2;

    const InterleavedAddrGenFast<true> a = {
        .bank_base_address = a_addr,
        .page_size = tile_size_bytes,
        .data_format = DataFormat::Float16_b,
    };

    const InterleavedAddrGenFast<true> b = {
        .bank_base_address = b_addr,
        .page_size = tile_size_bytes,
        .data_format = DataFormat::Float16_b,
    };

    for(uint32_t i = 0; i < n_tiles; i++)
    {
        cb_reserve_back(cb_in0, 1);
        uint32_t cb_in0_addr = get_write_ptr(cb_in0);
        noc_async_read_tile(i, a, cb_in0_addr);
        noc_async_read_barrier();
        cb_push_back(cb_in0, 1);

        cb_reserve_back(cb_in1, 1);
        uint32_t cb_in1_addr = get_write_ptr(cb_in1);
        noc_async_read_tile(i, b, cb_in1_addr);
        noc_async_read_barrier();
        cb_push_back(cb_in1, 1);
    }
}