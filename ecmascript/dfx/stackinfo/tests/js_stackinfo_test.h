/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_DFX_STACKINFO_JS_STACKINFO_TEST_H
#define ECMASCRIPT_DFX_STACKINFO_JS_STACKINFO_TEST_H

#include <cstdio>
#include <cstdint>

#define E_OK 0
#define E_INVAL (-1)

#define LAZY_DEOPT_FLAG_BIT1 30

#define mask_of_frame_type(frame_type) (1UL << (frame_type))
#define build_frame_type_mask0 0
#define build_frame_type_mask1(frame_type)  mask_of_frame_type(frame_type)
#define build_frame_type_mask2(frame_type, ...) mask_of_frame_type(frame_type) | build_frame_type_mask1(__VA_ARGS__)
#define build_frame_type_mask3(frame_type, ...) mask_of_frame_type(frame_type) | build_frame_type_mask2(__VA_ARGS__)
#define build_frame_type_mask4(frame_type, ...) mask_of_frame_type(frame_type) | build_frame_type_mask3(__VA_ARGS__)
#define build_frame_type_mask5(frame_type, ...) mask_of_frame_type(frame_type) | build_frame_type_mask4(__VA_ARGS__)
#define build_frame_type_mask6(frame_type, ...) mask_of_frame_type(frame_type) | build_frame_type_mask5(__VA_ARGS__)
#define build_frame_type_mask7(frame_type, ...) mask_of_frame_type(frame_type) | build_frame_type_mask6(__VA_ARGS__)
#define build_frame_type_mask8(frame_type, ...) mask_of_frame_type(frame_type) | build_frame_type_mask7(__VA_ARGS__)
#define build_frame_type_mask9(frame_type, ...) mask_of_frame_type(frame_type) | build_frame_type_mask8(__VA_ARGS__)
#define build_frame_type_mask10(frame_type, ...) mask_of_frame_type(frame_type) | build_frame_type_mask9(__VA_ARGS__)
#define build_frame_type_mask11(frame_type, ...) mask_of_frame_type(frame_type) | build_frame_type_mask10(__VA_ARGS__)
#define build_frame_type_mask12(frame_type, ...) mask_of_frame_type(frame_type) | build_frame_type_mask11(__VA_ARGS__)
#define build_frame_type_mask13(frame_type, ...) mask_of_frame_type(frame_type) | build_frame_type_mask12(__VA_ARGS__)
#define build_frame_type_mask14(frame_type, ...) mask_of_frame_type(frame_type) | build_frame_type_mask13(__VA_ARGS__)
#define build_frame_type_mask15(frame_type, ...) mask_of_frame_type(frame_type) | build_frame_type_mask14(__VA_ARGS__)
#define build_frame_type_mask16(frame_type, ...) mask_of_frame_type(frame_type) | build_frame_type_mask15(__VA_ARGS__)
#define build_frame_type_mask17(frame_type, ...) mask_of_frame_type(frame_type) | build_frame_type_mask16(__VA_ARGS__)
#define build_frame_type_mask18(frame_type, ...) mask_of_frame_type(frame_type) | build_frame_type_mask17(__VA_ARGS__)

#define VA_ARGS_NUM(...) VA_ARGS_NUM_IMPL(__VA_ARGS__, 18, 17, 16, 15, 14, 13, 12, \
                                          11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define VA_ARGS_NUM_IMPL(x, y, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, count, ...) count

#define __build_frame_type_mask(n, ...) (build_frame_type_mask##n(__VA_ARGS__))
#define build_frame_type_mask(n, ...) __build_frame_type_mask(n, __VA_ARGS__)
#define is_frame_type(frame_type, max_frame_type, ...) (((frame_type) <= (max_frame_type)) \
        && ((mask_of_frame_type(frame_type) & \
            build_frame_type_mask(VA_ARGS_NUM(__VA_ARGS__), __VA_ARGS__)) != 0UL))

struct unwind_user_context_s {
    int count;
    int read_mem(unwind_user_context_s *ctx, uintptr_t addr, uintptr_t *value, [[maybe_unused]] size_t size)
    {
        if (count == -1) { // -1: means no check
            *value = *reinterpret_cast<uintptr_t *>(addr);
            return E_OK;
        } else if (count == 0) {
            return E_INVAL;
        } else {
            *value = *reinterpret_cast<uintptr_t *>(addr);
            count--;
            return E_OK;
        }
    }
};

void unwind_user_finish(unwind_user_context_s *ctx);

struct unwind_stack_frame_s {
    uintptr_t fp;
    uintptr_t pc;
};

void mem_zero_s(unwind_stack_frame_s &frame);

#define UNWIND_FRAME_EMPTY { 0, 0 }

enum arkts_frame_type {
    OPTIMIZED_FRAME = 0,
    OPTIMIZED_ENTRY_FRAME,
    OPTIMIZED_JS_FUNCTION_FRAME,
    OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME,
    FASTJIT_FUNCTION_FRAME,
    FASTJIT_FAST_CALL_FUNCTION_FRAME,
    ASM_BRIDGE_FRAME,
    LEAVE_FRAME,
    LEAVE_FRAME_WITH_ARGV,
    BUILTIN_CALL_LEAVE_FRAME,
    INTERPRETER_FRAME,
    ASM_INTERPRETER_FRAME,
    INTERPRETER_CONSTRUCTOR_FRAME,
    BUILTIN_FRAME,
    BUILTIN_FRAME_WITH_ARGV,
    BUILTIN_ENTRY_FRAME,
    INTERPRETER_BUILTIN_FRAME,
    INTERPRETER_FAST_NEW_FRAME,
    INTERPRETER_ENTRY_FRAME,
    ASM_INTERPRETER_ENTRY_FRAME,
    ASM_INTERPRETER_BRIDGE_FRAME,
    OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME,
    OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME,
    BUILTIN_FRAME_WITH_ARGV_STACK_OVER_FLOW_FRAME,
    BASELINE_BUILTIN_FRAME,
    FRAME_TYPE_MAX,

    FRAME_TYPE_FIRST = OPTIMIZED_FRAME,
    FRAME_TYPE_LAST = OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME,
    INTERPRETER_FIRST = INTERPRETER_FRAME,
    INTERPRETER_LAST = INTERPRETER_FAST_NEW_FRAME,
    BUILTIN_FIRST = BUILTIN_FRAME,
    BUILTIN_LAST = BUILTIN_ENTRY_FRAME,
};

struct frame_offset_s {
    int type_offset;
    int fp_offset;
    int pc_offset;
};

#define define_offset(__type, __type_offset, __fp_offset, __pc_offset) \
[__type] = \
{ \
    .type_offset = (__type_offset), \
    .fp_offset = (__fp_offset), \
    .pc_offset = (__pc_offset), \
}

static frame_offset_s frame_offset_table64[] = {
    define_offset(OPTIMIZED_FRAME, 0, 8, 16),
    define_offset(OPTIMIZED_ENTRY_FRAME, 8, 0, -1),
    define_offset(OPTIMIZED_JS_FUNCTION_FRAME, 8, 16, 24),
    define_offset(OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME, 8, 16, 24),
    define_offset(FASTJIT_FUNCTION_FRAME, 16, 24, 0),
    define_offset(FASTJIT_FAST_CALL_FUNCTION_FRAME, 16, 24, 0),
    define_offset(ASM_BRIDGE_FRAME, 0, 8, 16),
    define_offset(LEAVE_FRAME, 0, 8, 16),
    define_offset(LEAVE_FRAME_WITH_ARGV, 0, 8, 16),
    define_offset(BUILTIN_CALL_LEAVE_FRAME, 0, 8, 16),
    define_offset(INTERPRETER_FRAME, 64, 56, 48),
    define_offset(ASM_INTERPRETER_FRAME, 64, 56, 48),
    define_offset(INTERPRETER_CONSTRUCTOR_FRAME, 64, 56, 48),
    define_offset(BUILTIN_FRAME, 0, 8, 16),
    define_offset(BUILTIN_FRAME_WITH_ARGV, 0, 8, 16),
    define_offset(BUILTIN_ENTRY_FRAME, 0, 8, 16),
    define_offset(INTERPRETER_BUILTIN_FRAME, 24, 16, -1),
    define_offset(INTERPRETER_FAST_NEW_FRAME, 64, 56, 48),
    define_offset(INTERPRETER_ENTRY_FRAME, 16, 8, -1),
    define_offset(ASM_INTERPRETER_ENTRY_FRAME, 16, 8, -1),
    define_offset(ASM_INTERPRETER_BRIDGE_FRAME, 16, 8, 24),
    define_offset(OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME, 8, 16, 24),
    define_offset(OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME, 8, 16, 24),
    define_offset(BUILTIN_FRAME_WITH_ARGV_STACK_OVER_FLOW_FRAME, 0, 8, 16),
    define_offset(BASELINE_BUILTIN_FRAME, 0, 8, 16),
};

bool is_entry_frame(unsigned int frame_type);
bool is_js_function_frame(unsigned int frame_type);
bool is_native_function_frame(unsigned int frame_type);
uintptr_t calc_pc_addr_from_fp_addr(uintptr_t fp_addr, unsigned int frame_type);
uintptr_t calc_fp_addr_from_fp_addr(uintptr_t fp_addr, unsigned int frame_type);
uintptr_t clear_lazy_deopt_flag(uintptr_t frame_type_raw);
int next_ark_frame(unwind_user_context_s *ctx, uintptr_t fp_addr, unsigned int curr_frame_type,
    unwind_stack_frame_s *frame, bool *ret_frame_avail);
int next_ark_frame(unwind_user_context_s *ctx, uintptr_t fp_addr,
    unwind_stack_frame_s *frame, unsigned int *ret_frame_type);
unwind_stack_frame_s step_ark_frame(unwind_user_context_s *ctx,
    const unwind_stack_frame_s *cur_frame);
unwind_stack_frame_s unwind_arkts(unwind_user_context_s *context,
    const unwind_stack_frame_s *cur_frame);

#endif  // ECMASCRIPT_DFX_STACKINFO_JS_STACKINFO_TEST_H