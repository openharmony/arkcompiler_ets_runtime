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

#include "ecmascript/tests/test_helper.h"

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

static void unwind_user_finish(unwind_user_context_s *ctx) {};

struct unwind_stack_frame_s {
    uintptr_t fp;
    uintptr_t pc;
};

static void mem_zero_s(unwind_stack_frame_s &frame)
{
    frame.fp = 0;
    frame.pc = 0;
}

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

static bool is_entry_frame(unsigned int frame_type)
{
    return is_frame_type(frame_type, FRAME_TYPE_MAX,
                 OPTIMIZED_ENTRY_FRAME,
                 ASM_INTERPRETER_ENTRY_FRAME);
}

static bool is_js_function_frame(unsigned int frame_type)
{
    return is_frame_type(frame_type, FRAME_TYPE_MAX,
                 ASM_INTERPRETER_FRAME,
                 INTERPRETER_CONSTRUCTOR_FRAME,
                 INTERPRETER_FRAME,
                 INTERPRETER_FAST_NEW_FRAME);
}

static bool is_native_function_frame(unsigned int frame_type)
{
    return is_frame_type(frame_type, FRAME_TYPE_MAX,
                         OPTIMIZED_FRAME,
                         BASELINE_BUILTIN_FRAME,
                         ASM_BRIDGE_FRAME,
                         OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME,
                         OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME,
                         OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME,
                         OPTIMIZED_JS_FUNCTION_FRAME,
                         LEAVE_FRAME,
                         LEAVE_FRAME_WITH_ARGV,
                         BUILTIN_CALL_LEAVE_FRAME,
                         BUILTIN_FRAME,
                         BUILTIN_ENTRY_FRAME,
                         BUILTIN_FRAME_WITH_ARGV,
                         BUILTIN_FRAME_WITH_ARGV_STACK_OVER_FLOW_FRAME,
                         ASM_INTERPRETER_BRIDGE_FRAME);
}

static uintptr_t calc_pc_addr_from_fp_addr(uintptr_t fp_addr, unsigned int frame_type)
{
    uintptr_t type_addr = fp_addr - sizeof(uintptr_t);
    uintptr_t addr = type_addr - (uintptr_t)frame_offset_table64[frame_type].type_offset;
    addr += (uintptr_t)frame_offset_table64[frame_type].pc_offset;
    return addr;
}

static uintptr_t calc_fp_addr_from_fp_addr(uintptr_t fp_addr, unsigned int frame_type)
{
    uintptr_t type_addr = fp_addr - sizeof(uintptr_t);
    uintptr_t addr = type_addr - (uintptr_t)frame_offset_table64[frame_type].type_offset;
    addr += (uintptr_t)frame_offset_table64[frame_type].fp_offset;
    return addr;
}

static uintptr_t clear_lazy_deopt_flag(uintptr_t frame_type_raw)
{
    return (frame_type_raw & (uintptr_t)(~(1UL << LAZY_DEOPT_FLAG_BIT1)));
}

static int next_ark_frame(unwind_user_context_s *ctx, uintptr_t fp_addr, unsigned int curr_frame_type,
                          unwind_stack_frame_s *frame, bool *ret_frame_avail)
{
    int err = E_OK;
    uintptr_t next_pc_addr = 0;
    uintptr_t next_fp_addr = 0;
    uintptr_t pc = 0UL;
    uintptr_t fp = 0UL;

    if (is_entry_frame(curr_frame_type)) {
        next_fp_addr = fp_addr;
        next_pc_addr = (uintptr_t)(fp_addr + sizeof(uintptr_t));
    } else {
        if (is_js_function_frame(curr_frame_type) || is_native_function_frame(curr_frame_type)) {
            next_pc_addr = calc_pc_addr_from_fp_addr(fp_addr, curr_frame_type);
        }
        next_fp_addr = calc_fp_addr_from_fp_addr(fp_addr, curr_frame_type);
    }
    if (err == E_OK) {
        if (next_fp_addr != 0UL) {
            err = ctx->read_mem(ctx, next_fp_addr, &fp, sizeof(uintptr_t));
        }
        if (err == E_OK && next_pc_addr != 0UL) {
            err = ctx->read_mem(ctx, next_pc_addr, &pc, sizeof(uintptr_t));
            *ret_frame_avail = (err == E_OK);
        }
        frame->fp = fp;
        frame->pc = pc;
    }

    return err;
}

static int next_ark_frame(unwind_user_context_s *ctx, uintptr_t fp_addr,
                          unwind_stack_frame_s *frame, unsigned int *ret_frame_type)
{
    int err = E_OK;
    uintptr_t frame_type = 0UL;
    while (err == E_OK) {
        bool frame_avail = false;
        uintptr_t type_addr = (uintptr_t)(fp_addr - sizeof(uintptr_t));
        err = ctx->read_mem(ctx, type_addr, &frame_type, sizeof(uintptr_t));
        if (err == E_OK) {
            frame_type = clear_lazy_deopt_flag(frame_type);
            if (frame_type > BASELINE_BUILTIN_FRAME) {
                err = E_INVAL;
                break;
            }
        }
        if (err == E_OK) {
            err = next_ark_frame(ctx, fp_addr, (unsigned int)frame_type, frame, &frame_avail);
        }
        if (err == E_OK) {
            fp_addr = frame->fp;
            if (frame_avail) {
                *ret_frame_type = (unsigned int)frame_type;
                break;
            }
        }
    }

    return err;
}

static unwind_stack_frame_s step_ark_frame(unwind_user_context_s *ctx,
                                           const unwind_stack_frame_s *cur_frame)
{
    unsigned int frame_type = 0U;
    unwind_stack_frame_s frame = UNWIND_FRAME_EMPTY;
    int err = E_OK;

    err = next_ark_frame(ctx, cur_frame->fp, &frame, &frame_type);
    if (err == E_OK) {
        if (is_entry_frame(frame_type) || !is_js_function_frame(frame_type)) {
            unwind_user_finish(ctx);
        }
    }
    if (err != E_OK) {
        mem_zero_s(frame);
    }
    return frame;
}

static unwind_stack_frame_s unwind_arkts(unwind_user_context_s *context,
                                         const unwind_stack_frame_s *cur_frame)
{
    unwind_stack_frame_s frame = UNWIND_FRAME_EMPTY;
    if (context != NULL && cur_frame != NULL) {
        frame = step_ark_frame(context, cur_frame);
    }
    return frame;
}

#endif  // ECMASCRIPT_DFX_STACKINFO_JS_STACKINFO_TEST_H