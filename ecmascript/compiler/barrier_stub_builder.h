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
          dstObj_(obj),
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
    GateRef IsLocalToShareSwappedFast(GateRef region);
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
    // Target object header (TaggedObject start) of the write barrier. Any barrier path that needs
    // to resolve "the object's owning Region" must use dstObj_, never dstAddr_ (see dstAddr_).
    const GateRef dstObj_;
    // Start address of the contiguous slot range to barrier. Usually equals dstObj_ (writing from
    // the header), but Array.prototype.concat and the like copy a second source past an offset inside
    // the result object, so dstAddr_ can be an interior address far beyond dstObj_ — potentially more
    // than a Region away.
    //
    // Hazard: a huge object spans multiple 256KB Regions, but ObjectAddressToRange() is a plain
    // address mask, so only the header's Region is valid:
    //
    //       Region 0 (256KB)           Region 1 (256KB)
    //     +------------------+       +------------------+
    //     | dstObj_ (header) |  ...  |  ^ dstAddr_      |   <- concat copy target (interior addr)
    //     +------------------+       +------------------+
    //
    //   ObjectAddressToRange(dstObj_)  -> Region 0  (valid owning Region)
    //   ObjectAddressToRange(dstAddr_) -> Region 1  (BOGUS: inside object data;
    //                                                Barriers::Update deref -> SIGSEGV)
    //
    // Hence MarkingBarrier/SharedGCMarkingBarrier must be passed dstObj_ + a header-relative offset
    // (see HandleMark); bitset barriers (DoBatchBarrierInternal etc.) instead use objectRegion_,
    // resolved from dstObj_ at construction.
    const GateRef dstAddr_;
    // Length of the slot range (in JSTaggedValue units).
    const GateRef slotCount_;
    // Owning Region resolved from dstObj_, for use by bitset barriers.
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
