#include <cstdint>

void kernel_main()
{
    uint32_t c_addr = get_arg_val<uint32_t>(0);
    uint32_t n_tiles = get_arg_val<uint32_t>(1);
    
    constexpr uint32_t cb_out0 = tt::CB::c_out0;
    uint32_t tile_size = get_tile_size(cb_out0);
    const uint32_t tile_size_bytes = 32 * 32 * 2;

    const InterleavedAddrGenFast<true> b = {
        .bank_base_address = c_addr,
        .page_size = tile_size_bytes,
        .data_format = DataFormat::Float16_b,
    };

    for(uint32_t i = 0; i < n_tiles; i++)
    {
        cb_wait_front(cb_out0, 1);
        uint32_t cb_out0_addr = get_read_ptr(cb_out0);
        noc_async_write_tile(i, b, cb_out0_addr);
        noc_async_write_barrier();
        cb_pop_front(cb_out0, 1);
    }

}