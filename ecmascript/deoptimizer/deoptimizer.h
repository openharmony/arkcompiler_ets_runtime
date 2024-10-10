/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_DEOPTIMIZER_DEOPTIMIZER_H
#define ECMASCRIPT_DEOPTIMIZER_DEOPTIMIZER_H

#include "ecmascript/base/aligned_struct.h"
#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/deoptimizer/calleeReg.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/stackmap/llvm/llvm_stackmap_type.h"

namespace panda::ecmascript {
class JSThread;
enum class SpecVregIndex: int {
    PC_OFFSET_INDEX = -1,
    ACC_INDEX = -2,
    ENV_INDEX = -3,
    FUNC_INDEX = -4,
    NEWTARGET_INDEX = -5,
    THIS_OBJECT_INDEX = -6,
    ACTUAL_ARGC_INDEX = -7,
    FIRST_METHOD_OFFSET_INDEX = -8,
    PADDING1 = -9,
    PADDING2 = -10,
    PADDING3 = -11,
    MAX_METHOD_OFFSET_INDEX = -12,
};

static constexpr uint32_t MAX_METHOD_OFFSET_NUM = static_cast<int32_t>(SpecVregIndex::FIRST_METHOD_OFFSET_INDEX) -
                                                  static_cast<int32_t>(SpecVregIndex::MAX_METHOD_OFFSET_INDEX) + 1;

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
        INLINE_DEPTH_INDEX = 0,
        CALLFRAME_TOP_INDEX,
        RETURN_ADDRESS_INDEX,
        CALLERFRAME_POINTER_INDEX,
        NUM_OF_MEMBER
    };

    static_assert(static_cast<size_t>(Index::NUM_OF_MEMBER) == NumOfTypes);

    static size_t GetInlineDepthOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::INLINE_DEPTH_INDEX)>(isArch32);
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

    alignas(EAS) size_t inlineDepth_ {0};
    alignas(EAS) uintptr_t callFrameTop_{0};
    alignas(EAS) uintptr_t returnAddr_{0};
    alignas(EAS) uintptr_t callerFp_{0};
    // out put data
};

class FrameWriter;
class Deoptimizier {
public:
    using ARKDeopt = kungfu::ARKDeopt;
    using CalleeReg = kungfu::CalleeReg;
    using CalleeRegAndOffsetVec = kungfu::CalleeRegAndOffsetVec;
    using CommonArgIdx = kungfu::CommonArgIdx;
    using DeoptType = kungfu::DeoptType;
    using DwarfRegType = kungfu::LLVMStackMapType::DwarfRegType;
    using DwarfRegAndOffsetType = kungfu::LLVMStackMapType::DwarfRegAndOffsetType;
    using IntType = kungfu::LLVMStackMapType::IntType;
    using LargeInt = kungfu::LLVMStackMapType::LargeInt;
    using LocationTy = kungfu::LocationTy;
    using OffsetType = kungfu::LLVMStackMapType::OffsetType;
    using VRegId = kungfu::LLVMStackMapType::VRegId;

    explicit Deoptimizier(JSThread *thread, size_t depth);

    void CollectVregs(const std::vector<kungfu::ARKDeopt>& deoptBundle, size_t shift);
    template<class T>
    void AssistCollectDeoptBundleVec(FrameIterator &it, T &frame);
    void CollectDeoptBundleVec(std::vector<kungfu::ARKDeopt>& deoptBundle);
    JSTaggedType ConstructAsmInterpretFrame();
    void UpdateAndDumpDeoptInfo(kungfu::DeoptType type);
    static PUBLIC_API std::string DisplayItems(kungfu::DeoptType type);
    static PUBLIC_API int32_t EncodeDeoptVregIndex(int32_t index, size_t depth, size_t shift);
    static PUBLIC_API size_t ComputeShift(size_t depth);
    static int32_t DecodeVregIndex(OffsetType id, size_t shift);
    static size_t DecodeDeoptDepth(OffsetType id, size_t shift);
    JSThread *GetThread() const
    {
        return thread_;
    }

    static const char *GetLLVMDeoptRelocateSymbol()
    {
        return "__llvm_deoptimize";
    }

private:
    size_t GetFrameIndex(CommonArgIdx index)
    {
        return static_cast<size_t>(index) - static_cast<size_t>(CommonArgIdx::FUNC);
    }
    JSTaggedValue GetFrameArgv(size_t idx)
    {
        ASSERT(frameArgvs_ != nullptr);
        ASSERT(idx < frameArgc_);
        return JSTaggedValue(frameArgvs_[idx]);
    }
    JSTaggedValue GetFrameArgv(CommonArgIdx index)
    {
        return GetFrameArgv(GetFrameIndex(index));
    }
    JSTaggedValue GetActualFrameArgs(int32_t index)
    {
        index += NUM_MANDATORY_JSFUNC_ARGS;
        return GetFrameArgv(static_cast<size_t>(index));
    }
    bool CollectVirtualRegisters(JSTaggedValue callTarget, Method *method, FrameWriter *frameWriter, size_t curDepth);
    bool HasDeoptValue(size_t curDepth, int32_t index) const
    {
        ASSERT(curDepth <= inlineDepth_);
        return deoptVregs_.find({curDepth, static_cast<OffsetType>(index)}) != deoptVregs_.end();
    }
    JSTaggedValue GetDeoptValue(size_t curDepth, int32_t index) const
    {
        ASSERT(curDepth <= inlineDepth_);
        if (!HasDeoptValue(curDepth, index)) {
            return JSTaggedValue::Undefined();
        }
        return deoptVregs_.at({curDepth, static_cast<OffsetType>(index)}).GetTaggedValue();
    }
    Method* GetMethod(JSTaggedValue &target);
    void RelocateCalleeSave();
    void Dump(JSTaggedValue callTarget, kungfu::DeoptType type, size_t depth);
    size_t GetCallSize(size_t curDepth, const uint8_t *resumePc);
    void ClearCompiledCodeStatusWhenDeopt(JSFunction *fun, Method *method);
    void ResetJitHotness(JSFunction *jsFunc) const;
    JSThread *thread_ {nullptr};
    uintptr_t *calleeRegAddr_ {nullptr};
    size_t numCalleeRegs_ {0};
    AsmStackContext stackContext_;

    std::map<std::pair<size_t, OffsetType>, JSHandle<JSTaggedValue>> deoptVregs_;
    struct Context context_ {0, 0, {}};
    std::unordered_map<size_t, size_t> pc_;
    std::unordered_map<size_t, size_t> jumpSize_;
    size_t frameArgc_ {0};
    JSTaggedType *frameArgvs_ {nullptr};
    bool traceDeopt_{false};
    size_t inlineDepth_ {0};
};

}  // namespace panda::ecmascript
#endif // ECMASCRIPT_DEOPTIMIZER_DEOPTIMIZER_H