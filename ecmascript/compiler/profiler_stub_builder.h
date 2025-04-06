/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_PROFILER_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_PROFILER_STUB_BUILDER_H

#include "ecmascript/compiler/profiler_operation.h"
#include "ecmascript/compiler/stub_builder.h"

namespace panda::ecmascript::kungfu {
class SlotIDInfo {
public:
    enum class SlotIDInfoType : uint8_t {
        PC,
        SLOT_ID,
        PC_FORMAT,
    };

    SlotIDInfo(GateRef info, SlotIDInfoType slotType) : slotType_(slotType)
    {
        ASSERT(slotType != SlotIDInfoType::PC_FORMAT);
        if (slotType == SlotIDInfoType::PC) {
            pc_ = info;
        } else {
            ASSERT(slotType == SlotIDInfoType::SLOT_ID);
            slotID_ = info;
        }
    }

    SlotIDInfo(GateRef pc, SlotIDFormat format)
        : slotType_(SlotIDInfoType::PC_FORMAT), pc_(pc), format_(format) {}

    ~SlotIDInfo() = default;

    GateRef GetSlotID() const
    {
        return slotID_;
    }

    SlotIDInfoType GetSlotType() const
    {
        return slotType_;
    }

    GateRef GetPC() const
    {
        return pc_;
    }

    SlotIDFormat GetFormat() const
    {
        return format_;
    }

private:
    SlotIDInfoType slotType_;
    GateRef slotID_ = 0;
    GateRef pc_ = 0;
    SlotIDFormat format_ = SlotIDFormat::IMM16;
};

struct OffsetInfo {
    GateRef offset = 0;
    GateRef pc = 0;
    bool isPc = true;
};

class ProfilerStubBuilder : public StubBuilder {
public:
    explicit ProfilerStubBuilder(StubBuilder *parent) : StubBuilder(parent) {}
    explicit ProfilerStubBuilder(Environment *env) : StubBuilder(env) {}
    ~ProfilerStubBuilder() override = default;
    NO_MOVE_SEMANTIC(ProfilerStubBuilder);
    NO_COPY_SEMANTIC(ProfilerStubBuilder);
    void GenerateCircuit() override {}

    // used for interpreter builtin
    void PGOProfiler(GateRef glue, GateRef pc, GateRef func, GateRef profileTypeInfo,
        const std::vector<GateRef> &values, SlotIDFormat format, OperationType type);
    // used for baseline builtin
    void PGOProfiler(GateRef glue, GateRef func, GateRef profileTypeInfo, GateRef slotIdOrOffset,
        const std::vector<GateRef> &values, OperationType type);

    void TryDump(GateRef glue, GateRef func, GateRef profileTypeInfo);
    void TryPreDump(GateRef glue, GateRef func, GateRef profileTypeInfo);

    void ProfileCall(
        GateRef glue, SlotIDInfo slotInfo, GateRef func, GateRef target, GateRef profileTypeInfo);
    void ProfileNativeCall(
        GateRef glue, SlotIDInfo slotInfo, GateRef func, GateRef target, GateRef profileTypeInfo);
    void ProfileGetterSetterCall(GateRef glue, GateRef target);
    void ProfileOpType(
        GateRef glue, SlotIDInfo slotInfo, GateRef func, GateRef profileTypeInfo, GateRef type);
    void ProfileDefineClass(
        GateRef glue, SlotIDInfo slotInfo, GateRef func, GateRef constructor, GateRef profileTypeInfo);
    void ProfileCreateObject(
        GateRef glue, SlotIDInfo slotInfo, GateRef func, GateRef newObj, GateRef profileTypeInfo);
    void ProfileBranch(GateRef glue, SlotIDInfo slotInfo, GateRef func, GateRef profileTypeInfo, bool isTrue);
    void ProfileGetIterator(
        GateRef glue, SlotIDInfo slotInfo, GateRef func, GateRef iterator, GateRef profileTypeInfo);

    GateRef UpdateTrackTypeInPropAttr(GateRef attr, GateRef value, ProfileOperation callback);
    void UpdatePropAttrIC(GateRef glue, GateRef receiver, GateRef value, GateRef handler, ProfileOperation callback);
    void UpdatePropAttrWithValue(
        GateRef glue, GateRef receiver, GateRef attr, GateRef value, ProfileOperation callback);

    GateRef IsProfileTypeInfoDumped(GateRef profileTypeInfo, ProfileOperation callback);

    void TryJitCompile(GateRef glue, OffsetInfo opCodeInfo, GateRef func, GateRef profileTypeInfo);
    GateRef IsCompiledOrTryCompile(GateRef glue, GateRef func, GateRef profileTypeInfo, ProfileOperation callback,
        GateRef pc);
    GateRef IsCompiledOrTryCompile(GateRef glue, GateRef func, GateRef profileTypeInfo, GateRef pc);
    GateRef GetJitHotnessThreshold(GateRef profileTypeInfo);
    GateRef GetBaselineJitHotnessThreshold(GateRef profileTypeInfo);
    void SetJitHotnessCnt(GateRef glue, GateRef profileTypeInfo, GateRef hotnessCnt);

private:
    static constexpr size_t MAX_PROFILE_CALL_COUNT = 10000;
    static constexpr size_t MIN_PROFILE_CALL_INTERVAL = 5;
    static constexpr size_t BITS_OF_WORD = 8;
    static constexpr size_t HIGH_WORD_OFFSET = 2;
    static constexpr size_t LOW_WORD_OFFSET = 1;

    void TryPreDumpInner(GateRef glue, GateRef func, GateRef profileTypeInfo);

    GateRef GetSlotID(const SlotIDInfo &slotInfo);
    GateRef GetBitFieldOffsetFromProfileTypeInfo(GateRef profileTypeInfo);
    GateRef IsProfileTypeInfoDumped(GateRef profileTypeInfo);
    GateRef IsProfileTypeInfoPreDumped(GateRef profileTypeInfo);
    GateRef IsProfileTypeInfoWithBigMethod(GateRef profileTypeInfo);
    GateRef IsProfileTypeInfoHotAndValid(GateRef profileTypeInfo);
    void SetDumpPeriodIndex(GateRef glue, GateRef profileTypeInfo);
    void SetPreDumpPeriodIndex(GateRef glue, GateRef profileTypeInfo);
    GateRef TaggedToTrackType(GateRef value);
    GateRef GetIterationFunctionId(GateRef glue, GateRef iterator);
    GateRef TryGetBuiltinFunctionId(GateRef glue, GateRef target);
    GateRef GetJitHotnessCnt(GateRef profileTypeInfo);
    GateRef GetJitHotnessThresholdOffset(GateRef profileTypeInfo);
    GateRef GetJitHotnessCntOffset(GateRef profileTypeInfo);
    GateRef GetJitCallCntOffset(GateRef profileTypeInfo);
    GateRef GetJitCallCnt(GateRef profileTypeInfo);
    GateRef GetOsrHotnessThresholdOffset(GateRef profileTypeInfo);
    GateRef GetOsrHotnessThreshold(GateRef profileTypeInfo);
    GateRef GetBaselineJitHotnessThresholdOffset(GateRef profileTypeInfo);
    GateRef GetOsrHotnessCntOffset(GateRef profileTypeInfo);
    GateRef GetOsrHotnessCnt(GateRef profileTypeInfo);
    void PGOProfiler(GateRef glue, GateRef func, GateRef profileTypeInfo, SlotIDInfo slotIdInfo,
        const std::vector<GateRef> &values, OperationType type);
};
} // namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_PROFILER_STUB_BUILDER_H
