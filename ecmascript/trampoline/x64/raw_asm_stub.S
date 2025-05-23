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

.macro PUSH_CALLEE_SAVE
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    pushq %rbx
.endm

.macro RESTORE_CALLEE_SAVE
    popq %rbx
    popq %r15
    popq %r14
    popq %r13
    popq %r12
.endm

/*
LazyDeoptEntry layout:
+---------------+
|  Placeholder  | <- rsp + 56
+---------------+
|  rbp          | <- rsp + 48
+---------------+
|  rax          | <- rsp + 40
+---------------+
|  r12          | <- rsp + 32
+---------------+
|  r13          | <- rsp + 24
+---------------+
|  r14          | <- rsp + 16
+---------------+
|  r15          | <- rsp + 8
+---------------+
|  rbx          | <- rsp (after PUSH_CALLEE_SAVE)
+---------------+
*/

LazyDeoptEntryName :
    pushq   $0                                      // Place Holder (return address)
    pushq   %rbp                                    // Save original frame pointer
    pushq   %rax                                    // Save maybeAcc (original rax)
    PUSH_CALLEE_SAVE                                // Callee-saved registers

    // Get glue pointer from thread local storage
    callq   GetGlueFromThreadLocalName              // rax = glue pointer

    // Prepare arguments for GetFixedReturnAddr
    movq    %rax, %r12                              // Save glue to r12
    movq    %rax, %rdi                              // arg0: glue
    leaq    PRE_SP_OFFSET_X64(%rsp), %rsi           // arg1: prevCallSiteSp
    callq   GetFixedReturnAddrName                  // rax = return address offset
    movq    %rax, RETURN_ADDRESS_OFFSET_X64(%rsp)   // Store origin return address

    // Prepare deoptimization handler call
    movq    $0, %rdi                                // arg0: False
    callq   GetDeoptHandlerAsmOffsetName            // rax = DeoptHandlerAsm offset
    movq    %r12, %rcx                              // rcx = glue
    addq    %rcx, %rax                              // rax = glue + offset(DeoptHandleAsm Address)

    // Set up arguments and jump to DeoptHandlerASM
    movq    %rcx, %rdi                              // arg0: glue pointer
    movq    $LAZY_DEOPT_TYPE_OFFSET, %rsi           // arg1: deopt type (LAZY_DEOPT)
    RESTORE_CALLEE_SAVE                             // Restore callee-saved registers
    popq    %rdx                                    // arg2: maybeAcc (original rax value)
    popq    %rbp                                    // Restore original frame pointer
    jmpq    *(%rax)                                 // Tail call to DeoptHandler