/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "ecmascript/compiler/argument_accessor.h"
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

struct Context {
    uintptr_t callsiteSp;
    uintptr_t callsiteFp;
    kungfu::CalleeRegAndOffsetVec calleeRegAndOffset;
};

struct AsmStackContext : public base::AlignedStruct<base::AlignedPointer::Size(),
                                                    base::AlignedPointer,
                                                    base::AlignedPointer,
                                                    base::AlignedPointer,
                                                    base::AlignedPointer> {
    enum class Index : size_t {
        OUTPUT_COUNT_INDEX = 0,
        CALLFRAME_TOP_INDEX,
        RETURN_ADDRESS_INDEX,
        CALLERFRAME_POINTER_INDEX,
        NUM_OF_MEMBER
    };

    static_assert(static_cast<size_t>(Index::NUM_OF_MEMBER) == NumOfTypes);

    static size_t GetOutputCountOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::OUTPUT_COUNT_INDEX)>(isArch32);
    }

    static size_t GetCallFrameTopOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::CALLFRAME_TOP_INDEX)>(isArch32);
    }

    static size_t GetReturnAddressOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::RETURN_ADDRESS_INDEX)>(isArch32);
    }

    static size_t GetCallerFpOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::CALLERFRAME_POINTER_INDEX)>(isArch32);
    }

    static constexpr size_t GetSize(bool isArch32)
    {
        return isArch32 ? AsmStackContext::SizeArch32 : AsmStackContext::SizeArch64;
    }

    alignas(EAS) size_t outputCount_ {0};
    alignas(EAS) uintptr_t callFrameTop_{0};
    alignas(EAS) uintptr_t returnAddr_{0};
    alignas(EAS) uintptr_t callerFp_{0};
    // out put data
};

class FrameWriter;
class Deoptimizier {
public:
    explicit Deoptimizier(JSThread * thread) : thread_(thread)
    {
        kungfu::CalleeReg callreg;
        numCalleeRegs_ = static_cast<size_t>(callreg.GetCallRegNum());
    }
    void CollectVregs(const std::vector<kungfu::ARKDeopt>& deoptBundle);
    void CollectDeoptBundleVec(std::vector<kungfu::ARKDeopt>& deoptBundle);
    JSTaggedType ConstructAsmInterpretFrame();

    JSThread *GetThread() const
    {
        return thread_;
    }

private:
    size_t GetFrameIndex(kungfu::CommonArgIdx index)
    {
        return static_cast<size_t>(index) - static_cast<size_t>(kungfu::CommonArgIdx::FUNC);
    }
    JSTaggedValue GetFrameArgv(size_t idx)
    {
        ASSERT(frameArgvs_ != nullptr);
        ASSERT(idx < frameArgc_);
        return JSTaggedValue(frameArgvs_[idx]);
    }
    JSTaggedValue GetFrameArgv(kungfu::CommonArgIdx index)
    {
        return GetFrameArgv(GetFrameIndex(index));
    }
    JSTaggedValue GetActualFrameArgs(int32_t index)
    {
        index += NUM_MANDATORY_JSFUNC_ARGS;
        return GetFrameArgv(static_cast<size_t>(index));
    }
    bool CollectVirtualRegisters(Method* method, FrameWriter *frameWriter);
    bool hasDeoptValue(int32_t index)
    {
        return deoptVregs_.find(static_cast<kungfu::OffsetType>(index)) != deoptVregs_.end();
    }
    Method* GetMethod(JSTaggedValue &target);
    void RelocateCalleeSave();
    JSThread *thread_ {nullptr};
    uintptr_t *calleeRegAddr_ {nullptr};
    size_t numCalleeRegs_ {0};
    AsmStackContext stackContext_;

    std::unordered_map<kungfu::OffsetType, JSTaggedValue> deoptVregs_;
    struct Context context_ {0, 0, {}};
    uint32_t pc_ {0};
    JSTaggedValue env_ {JSTaggedValue::Undefined()};
    size_t frameArgc_ {0};
    JSTaggedType *frameArgvs_ {nullptr};
};

}  // namespace panda::ecmascript
#endif // ECMASCRIPT_DEOPTIMIZER_H