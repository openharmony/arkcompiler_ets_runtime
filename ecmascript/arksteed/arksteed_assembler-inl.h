/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_ARKSTEED_ASSEMBLER_INL_H
#define ECMASCRIPT_ARKSTEED_ASSEMBLER_INL_H

#include "ecmascript/arksteed/arksteed_assembler.h"

// Include platform-specific implementations
#if defined(PANDA_TARGET_AMD64)
#include "ecmascript/arksteed/arch/x64/arksteed_assembler_x64-inl.h"
#elif defined(PANDA_TARGET_ARM64)
#include "ecmascript/arksteed/arch/arm64/arksteed_assembler_arm64-inl.h"
#else
#error "ArkSteed does not support this architecture."
#endif

namespace panda::ecmascript::arksteed {

// =============================================================================
// Common Inline Implementations (platform-agnostic)
// =============================================================================

// Add any common inline implementations here that work across all platforms.
// Platform-specific inline implementations are in arch/xxx/arksteed_assembler_xxx_inl.h

inline void ArkSteedAssembler::Branch(Condition condition, Label *ifTrue, bool fallthroughWhenTrue, Label *ifFalse,
                                      bool fallthroughWhenFalse)
{
    if (fallthroughWhenFalse) {
        if (fallthroughWhenTrue) {
            ASSERT(ifTrue == ifFalse);
            return;
        }
        // Jump over the false block if true, otherwise fall through into it.
        JumpIf(condition, ifTrue);
    } else {
        // Jump to the false block if true.
        JumpIf(NegateCondition(condition), ifFalse);
        // Jump to the true block if it's not the next block.
        if (!fallthroughWhenTrue) {
            Jump(ifTrue);
        }
    }
}
void ArkSteedAssembler::CallRuntime(kungfu::RuntimeStubCSigns::ID runtimeId)
{
    ScratchRegisterScope scope;
    ASSERT(entryThread_ != nullptr);
    Address address = entryThread_->GetRTInterface(static_cast<size_t>(kungfu::RuntimeStubCSigns::ID_CallRuntime));
    auto scratch = scope.AcquireScratch();
#if defined(PANDA_TARGET_AMD64)
    Move(x64::rax, static_cast<uint64_t>(entryThread_->GetGlueAddr()));
#elif defined(PANDA_TARGET_ARM64)
    Move(aarch64::x0, static_cast<uint64_t>(entryThread_->GetGlueAddr()));
#endif
    Move(scratch, static_cast<uint64_t>(address));
    Call(scratch);
}
inline void ArkSteedAssembler::CallCommonStub(uint32_t stubId)
{
    ScratchRegisterScope scope;
    ASSERT(entryThread_ != nullptr);
    Address address = entryThread_->GetFastStubEntry(stubId);
    auto scratch = scope.AcquireScratch();
    Move(scratch, static_cast<uint64_t>(address));
    Call(scratch);
}
}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_ASSEMBLER_INL_H
