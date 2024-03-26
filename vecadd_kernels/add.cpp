#include <cstdint>
#include "compute_kernel_api/common.h"
#include "compute_kernel_api/tile_move_copy.h"
#include "compute_kernel_api/eltwise_binary.h"
#include "compute_kernel_api.h"

namespace NAMESPACE {
void MAIN {
    uint32_t n_tiles = get_arg_val<uint32_t>(0);

    constexpr auto cb_in0 = tt::CB::c_in0;
    constexpr auto cb_in1 = tt::CB::c_in1;
    constexpr auto cb_out0 =  tt::CB::c_out0;
    constexpr uint32_t dst_reg = 0;

    binary_op_init_common(cb_in0, cb_in1, cb_out0);

    for(uint32_t i = 0; i < n_tiles; i++) {
        acquire_dst(tt::DstMode::Full);
        cb_wait_front(cb_in0, 1);
        cb_wait_front(cb_in1, 1);
        add_tiles_init();
        add_tiles(cb_in0, cb_in1, 0, 0, dst_reg);
        cb_reserve_back(cb_out0, 1);
        pack_tile(dst_reg, cb_out0);
        cb_push_back(cb_out0, 1);
        cb_pop_front(cb_in0, 1);
        cb_pop_front(cb_in1, 1);
        release_dst(tt::DstMode::Half);
    }
}
}