/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "ecmascript/dfx/stackinfo/tests/js_stackinfo_test.h"

void unwind_user_finish(unwind_user_context_s *ctx) {}

void mem_zero_s(unwind_stack_frame_s &frame)
{
    frame.fp = 0;
    frame.pc = 0;
}

bool is_entry_frame(unsigned int frame_type)
{
    return is_frame_type(frame_type, FRAME_TYPE_MAX,
                         OPTIMIZED_ENTRY_FRAME,
                         ASM_INTERPRETER_ENTRY_FRAME);
}

bool is_js_function_frame(unsigned int frame_type)
{
    return is_frame_type(frame_type, FRAME_TYPE_MAX,
                         ASM_INTERPRETER_FRAME,
                         INTERPRETER_CONSTRUCTOR_FRAME,
                         INTERPRETER_FRAME,
                         INTERPRETER_FAST_NEW_FRAME);
}

bool is_native_function_frame(unsigned int frame_type)
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

uintptr_t calc_pc_addr_from_fp_addr(uintptr_t fp_addr, unsigned int frame_type)
{
    uintptr_t type_addr = fp_addr - sizeof(uintptr_t);
    uintptr_t addr = type_addr - (uintptr_t)frame_offset_table64[frame_type].type_offset;
    addr += (uintptr_t)frame_offset_table64[frame_type].pc_offset;
    return addr;
}

uintptr_t calc_fp_addr_from_fp_addr(uintptr_t fp_addr, unsigned int frame_type)
{
    uintptr_t type_addr = fp_addr - sizeof(uintptr_t);
    uintptr_t addr = type_addr - (uintptr_t)frame_offset_table64[frame_type].type_offset;
    addr += (uintptr_t)frame_offset_table64[frame_type].fp_offset;
    return addr;
}

uintptr_t clear_lazy_deopt_flag(uintptr_t frame_type_raw)
{
    return (frame_type_raw & (uintptr_t)(~(1UL << LAZY_DEOPT_FLAG_BIT1)));
}

int next_ark_frame(unwind_user_context_s *ctx, uintptr_t fp_addr, unsigned int curr_frame_type,
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

int next_ark_frame(unwind_user_context_s *ctx, uintptr_t fp_addr,
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

unwind_stack_frame_s step_ark_frame(unwind_user_context_s *ctx,
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

unwind_stack_frame_s unwind_arkts(unwind_user_context_s *context,
                                  const unwind_stack_frame_s *cur_frame)
{
    unwind_stack_frame_s frame = UNWIND_FRAME_EMPTY;
    if (context != NULL && cur_frame != NULL) {
        frame = step_ark_frame(context, cur_frame);
    }
    return frame;
}