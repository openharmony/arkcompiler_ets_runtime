/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_FUNC_ENTRY_DES_H
#define ECMASCRIPT_COMPILER_FUNC_ENTRY_DES_H

#include "ecmascript/common.h"
#include "ecmascript/compiler/aot_file/executed_memory_allocator.h"
#include "ecmascript/compiler/bc_call_signature.h"
#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/deoptimizer/calleeReg.h"

namespace panda::ecmascript {
struct FuncEntryDes {
    using CallSignature = kungfu::CallSignature;

    uint64_t codeAddr_ {};
    CallSignature::TargetKind kind_;
    bool isMainFunc_ {};
    bool isFastCall_ {};
    uint32_t indexInKindOrMethodId_ {};
    uint32_t moduleIndex_ {};
    uint32_t abcIndexInAi_ {INVALID_INDEX};
    int fpDeltaPrevFrameSp_ {};
    uint32_t funcSize_ {};
    uint32_t calleeRegisterNum_ {};
    int32_t CalleeReg2Offset_[2 * kungfu::MAX_CALLEE_SAVE_REIGISTER_NUM];
    bool IsStub() const
    {
        return CallSignature::TargetKind::STUB_BEGIN <= kind_ && kind_ < CallSignature::TargetKind::STUB_END;
    }
    bool IsBCStub() const
    {
        return CallSignature::TargetKind::BCHANDLER_BEGIN <= kind_ &&
               kind_ < CallSignature::TargetKind::BCHANDLER_END;
    }
    bool IsBCHandlerStub() const
    {
        return (kind_ == CallSignature::TargetKind::BYTECODE_HANDLER);
    }
    bool IsBuiltinsStub() const
    {
        return (kind_ == CallSignature::TargetKind::BUILTINS_STUB ||
                kind_ == CallSignature::TargetKind::BUILTINS_WITH_ARGV_STUB ||
                kind_ == CallSignature::TargetKind::BUILTINS_STW_COPY_STUB);
    }

    bool IsCommonStub() const
    {
        return (kind_ == CallSignature::TargetKind::COMMON_STUB ||
                kind_ == CallSignature::TargetKind::COMMON_STW_COPY_STUB);
    }

    bool IsBaselineStub() const
    {
        return (kind_ == CallSignature::TargetKind::BASELINE_STUB);
    }
    bool IsGeneralRTStub() const
    {
        return (kind_ >= CallSignature::TargetKind::RUNTIME_STUB && kind_ <= CallSignature::TargetKind::DEOPT_STUB);
    }
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_COMPILER_FUNC_ENTRY_DES_H

