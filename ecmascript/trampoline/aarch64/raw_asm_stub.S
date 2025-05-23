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

#include "ecmascript/trampoline/asm_defines.h"

.extern GetGlueFromThreadLocalName
.extern GetFixedReturnAddrName
.extern GetDeoptHandlerAsmOffsetName

.global LazyDeoptEntryName

// ARM64 callee-saved registers: X19-X28, D8-D15
.macro PUSH_CALLEE_SAVE
    stp x27, x28, [sp, #-REG_PAIR_SIZE_ARM64]!
    stp x25, x26, [sp, #-REG_PAIR_SIZE_ARM64]!
    stp x23, x24, [sp, #-REG_PAIR_SIZE_ARM64]!
    stp x21, x22, [sp, #-REG_PAIR_SIZE_ARM64]!
    stp x19, x20, [sp, #-REG_PAIR_SIZE_ARM64]!
    stp d14, d15, [sp, #-REG_PAIR_SIZE_ARM64]!
    stp d12, d13, [sp, #-REG_PAIR_SIZE_ARM64]!
    stp d10, d11, [sp, #-REG_PAIR_SIZE_ARM64]!
    stp d8, d9, [sp, #-REG_PAIR_SIZE_ARM64]!
.endm

.macro RESTORE_CALLEE_SAVE
    ldp d8, d9, [sp], #REG_PAIR_SIZE_ARM64
    ldp d10, d11, [sp], #REG_PAIR_SIZE_ARM64
    ldp d12, d13, [sp], #REG_PAIR_SIZE_ARM64
    ldp d14, d15, [sp], #REG_PAIR_SIZE_ARM64
    ldp x19, x20, [sp], #REG_PAIR_SIZE_ARM64
    ldp x21, x22, [sp], #REG_PAIR_SIZE_ARM64
    ldp x23, x24, [sp], #REG_PAIR_SIZE_ARM64
    ldp x25, x26, [sp], #REG_PAIR_SIZE_ARM64
    ldp x27, x28, [sp], #REG_PAIR_SIZE_ARM64
.endm

/*
+---------------+
|               | 
|  Caller Stack | 
|               | 
+---------------+ <---------       // SP + 176 (SP before LazyDeoptEntry)
| x30           | (8 bytes)        // SP + 168
| x29           | (8 bytes)        // SP + 160
+---------------+ 
| x1            | (8 bytes)        // SP + 152
| x0            | (8 bytes)        // SP + 144
+---------------+ 
| x27/x28       | (16 bytes)       // SP + 128
| x25/x26       | (16 bytes)       // SP + 112
| x23/x24       | (16 bytes)       // SP + 96
| x21/x22       | (16 bytes)       // SP + 80
| x19/x20       | (16 bytes)       // SP + 64
| d14/d15       | (16 bytes)       // SP + 48
| d12/d13       | (16 bytes)       // SP + 32
| d10/d11       | (16 bytes)       // SP + 16
| d8/d9         | (16 bytes)       // SP
+---------------+ 
*/

LazyDeoptEntryName :
    stp     x29, x30, [sp, #-REG_PAIR_SIZE_ARM64]!  // Frame pointer and return address
    stp     x0, x3, [sp, #-REG_PAIR_SIZE_ARM64]!    // Save maybeAcc, x3 use to align up
    PUSH_CALLEE_SAVE                                // Callee-saved registers

    // Get glue pointer from thread local storage
    bl GetGlueFromThreadLocalName                   // x0 = glue pointer

    // Prepare arguments for GetFixedReturnAddr
    mov x19, x0                                     // Save glue to x19, arg0: glue
    add x1, sp, #PRE_SP_OFFSET_ARM64                // arg1: prevCallSiteSp
    bl GetFixedReturnAddrName                       // x0 = return address offset
    str x0, [sp, #RETURN_ADDRESS_OFFSET_ARM64]      // Store origin return address

    // Prepare deoptimization handler call
    mov x0, #0                                      // arg0: False
    bl GetDeoptHandlerAsmOffsetName                 // x0 = DeoptHandlerAsm offset
    ldr x4, [x19, x0]                               // x4 = *(glue + offset) (DeoptHandleAsm Address)

    // Set up arguments and jump to DeoptHandlerASM
    mov x0, x19                                     // arg0: glue
    mov x1, #LAZY_DEOPT_TYPE_OFFSET                 // arg1: LAZY_DEOPT
    RESTORE_CALLEE_SAVE                             // Restore callee-saved registers
    ldp x2, x3, [sp], #REG_PAIR_SIZE_ARM64          // arg2: maybeAcc
    ldp x29, x30, [sp], #REG_PAIR_SIZE_ARM64        // Restore frame pointer and return address
    br x4                                           // Tail call to DeoptHandler