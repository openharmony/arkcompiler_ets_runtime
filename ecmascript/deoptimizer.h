/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_DEOPTIMIZER_H
#define ECMASCRIPT_DEOPTIMIZER_H
#include "ecmascript/base/aligned_struct.h"
#include "ecmascript/calleeReg.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/llvm_stackmap_type.h"

// todo:place to Deoptimizier class
#define LLVM_DEOPT_RELOCATE_SYMBOL "__llvm_deoptimize"
namespace panda::ecmascript {
class JSThread;
enum class SpecVregIndex: int {
    PC_INDEX = -1,
    ACC_INDEX = -2,
    BC_OFFSET_INDEX = -3,
};

struct Context
{
    uintptr_t callsiteSp;
    uintptr_t callsiteFp;
    uintptr_t* preFrameSp;
    uintptr_t returnAddr;
    kungfu::CalleeRegAndOffsetVec calleeRegAndOffset;
};

enum class CommonArgIdx : uint8_t {
    LEXENV = 0,
    ACTUAL_ARGC,
    FUNC,
    NEW_TARGET,
    THIS,
    NUM_OF_ARGS,
};

struct AsmStackContext : public base::AlignedStruct<base::AlignedPointer::Size(),
                                                    base::AlignedPointer,
                                                    base::AlignedPointer,
                                                    base::AlignedPointer> {
    enum class Index : size_t {
        OutputCountIndex = 0,
        CallFrameTopIndex,
        CallerFramePointerIndex,
        NumOfMembers
    };

    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    static size_t GetOutputCountOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::OutputCountIndex)>(isArch32);
    }

    static size_t GetCallFrameTopOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::CallFrameTopIndex)>(isArch32);
    }

    static size_t GetCallerFpOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::CallerFramePointerIndex)>(isArch32);
    }

    static constexpr size_t GetSize(bool isArch32)
    {
        return isArch32 ? AsmStackContext::SizeArch32 : AsmStackContext::SizeArch64;
    }

    alignas(EAS) size_t outputCount_ {0};
    alignas(EAS) uintptr_t callFrameTop_{0};
    alignas(EAS) uintptr_t callerFp_{0};
    // out put data
};

class Deoptimizier
{
public:
    static constexpr uint64_t LLVM_DEOPT_RELOCATE_ADDR = 0xabcdef0f;
    Deoptimizier(JSThread * thread) : thread_(thread) {
        kungfu::CalleeReg callreg;
        numCalleeRegs_ = callreg.GetCallRegNum();
    }
    void CollectVregs(const std::vector<kungfu::ARKDeopt>& deoptBundle);
    void CollectDeoptBundleVec(std::vector<kungfu::ARKDeopt>& deoptBundle);
    JSTaggedType ConstructAsmInterpretFrame();
    JSTaggedType GetArgv(CommonArgIdx idx)
    {
        ASSERT(AotArgvs_ != nullptr);
        return AotArgvs_[static_cast<int>(idx)];
    }

    JSTaggedType GetArgv(int idx)
    {
        ASSERT(AotArgvs_ != nullptr);
        return AotArgvs_[static_cast<int>(idx)];
    }

    JSThread *GetThread() const {
        return thread_;
    }

private:
    Method* GetMethod(JSTaggedValue &target);
    void RelocateCalleeSave();
    JSThread *thread_ {nullptr};
    uintptr_t *calleeRegAddr_ {nullptr};
    size_t numCalleeRegs_ {0};
    AsmStackContext stackContext_;

    std::unordered_map<kungfu::OffsetType, JSTaggedValue> vregs_;
    struct Context context_ {0, 0, nullptr, 0, {}};
    uint32_t pc_;
    JSTaggedValue callTarget_ {JSTaggedValue::Undefined()};
    JSTaggedType *AotArgvs_ {nullptr};
};

}  // namespace panda::ecmascript
#endif // ECMASCRIPT_DEOPTIMIZER_H