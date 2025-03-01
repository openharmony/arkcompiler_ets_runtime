/*
* Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_BARRIER_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BARRIER_STUB_BUILDER_H

#include "ecmascript/compiler/stub_builder.h"

namespace panda::ecmascript::kungfu {
class BarrierStubBuilder : public StubBuilder {
public:
    BarrierStubBuilder(StubBuilder *parent, GateRef glue, GateRef obj, GateRef startAddr, GateRef slotCount)
        : StubBuilder(parent),
          glue_(glue),
          dstAddr_(startAddr),
          slotCount_(slotCount),
          objectRegion_(ObjectAddressToRange(obj)) {}

    ~BarrierStubBuilder() override = default;
    NO_MOVE_SEMANTIC(BarrierStubBuilder);
    NO_COPY_SEMANTIC(BarrierStubBuilder);

    void GenerateCircuit() override {}

    void DoMoveBarrierCrossRegion(GateRef srcAddr, GateRef srcRegion);

    void DoBatchBarrier();

    void DoMoveBarrierInRegion(GateRef srcAddr);

    void DoReverseBarrier();
private:
    enum BitSetSelect {
        LocalToShared = 0b1,
        OldToNew = 0b10,
    };

    enum RegionKind {
        InSameRegion,
        CrossRegion,
    };

    GateRef GetBitSetDataAddr(GateRef objectRegion, GateRef loadOffset, int32_t createFunID);
    void HandleMark();
    void DoBatchBarrierInternal();
    void BarrierBatchBitSet(uint8_t select);
    void FlushBatchBitSet(uint8_t bitSetSelect, GateRef quadIdx,
                          Variable &localToShareBitSetVar, Variable &oldToNewBitSetVar, Label *next);
    GateRef IsLocalToShareSwapped(GateRef region);
    GateRef IsOldToNewSwapped(GateRef region);
    void BitSetRangeMove(GateRef srcBitSet, GateRef dstBitSet, GateRef srcStart, GateRef dstStart, GateRef length);
    void BitSetRangeMoveForward(GateRef srcBitSet, GateRef dstBitSet, GateRef srcStart, GateRef dstStart,
                                GateRef length);
    void BitSetRangeMoveBackward(GateRef srcBitSet, GateRef dstBitSet, GateRef srcStart, GateRef dstStart,
                                 GateRef length);
    void DoReverseBarrierInternal();
    void BitSetRangeReverse(GateRef bitSet, GateRef startIdx, GateRef length);
    void DoMoveBarrierSameRegionKind(GateRef srcAddr, GateRef srcRegion, RegionKind regionKind);
    const GateRef glue_;
    const GateRef dstAddr_;
    const GateRef slotCount_;
    const GateRef objectRegion_;

    static constexpr int64_t BIT_PER_QUAD_MASK = 63;
    static constexpr int64_t BIT_PER_QUAD_LOG2 = 6;
    static constexpr int64_t BYTE_PER_QUAD_LOG2 = 3;
    static constexpr int64_t BYTE_PER_QUAD = 8;
    static constexpr int64_t BIT_PER_BYTE_LOG2 = 3;
    static constexpr int64_t BIT_PER_QUAD = 64;
    static constexpr int64_t ALL_ONE_MASK = -1;
    static constexpr size_t FLUSH_RANGE = GCBitset::BIT_PER_WORD * GCBitset::BIT_PER_BYTE;

    static constexpr int8_t LOCAL_TO_SHARE_SWAPPED_MASK =
        static_cast<int8_t>(RSetSwapFlag::LOCAL_TO_SHARE_SWAPPED_MASK) |
        static_cast<int8_t>(RSetSwapFlag::LOCAL_TO_SHARE_COLLECTED_MASK);
    static constexpr int8_t OLD_TO_NEW_SWAPPED_MASK = static_cast<int8_t>(RSetSwapFlag::OLD_TO_NEW_SWAPPED_MASK);
};
}

#endif //ECMASCRIPT_COMPILER_BARRIER_STUB_BUILDER_H
