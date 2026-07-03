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

#include "ecmascript/arksteed/arksteed_write_barrier.h"

#include "ecmascript/arksteed/arksteed_assembler-inl.h"  // IWYU pragma: keep
#include "ecmascript/js_thread.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/remembered_set.h"
#include "ecmascript/mem/region.h"

namespace panda::ecmascript::arksteed {
#define __ assembler_->

void ArkSteedWriteBarrierEmitter::CallBarrierRuntime(kungfu::RuntimeStubCSigns::ID runtimeId, ArkSteedRegister glue,
                                                     ArkSteedRegister object, int32_t offset,
                                                     ArkSteedRegister value, ArkSteedRegister offsetScratch,
                                                     bool preserveInputs)
{
    ASSERT(glue == ArkSteedAssembler::GetParameterRegister(0));
    ASSERT(object == ArkSteedAssembler::GetParameterRegister(1));
    ASSERT(offsetScratch == ArkSteedAssembler::GetParameterRegister(2));
    ASSERT(value == ArkSteedAssembler::GetParameterRegister(3));
    constexpr int reservedSlotCount = 2;
    if (preserveInputs) {
        __ ReserveCallArgSlots(reservedSlotCount);
        __ MoveRepr(MachineRepresentation::Word64, __ GetCallArgSlot(0), object);
        __ MoveRepr(MachineRepresentation::Word64, __ GetCallArgSlot(1), value);
    }
    __ Move(offsetScratch, static_cast<int64_t>(offset));
    __ CallNGCRuntime(runtimeId);
    if (preserveInputs) {
        __ LoadGlue(glue);
        __ MoveRepr(MachineRepresentation::Word64, object, __ GetCallArgSlot(0));
        __ MoveRepr(MachineRepresentation::Word64, value, __ GetCallArgSlot(1));
        __ FreeCallArgSlots(reservedSlotCount);
    }
}

void ArkSteedWriteBarrierEmitter::EmitFastWriteBarrier(ArkSteedRegister glue, ArkSteedRegister object,
                                                       ArkSteedRegister value, int32_t offset,
                                                       ArkSteedRegister offsetScratch)
{
    ASSERT(glue == ArkSteedAssembler::GetParameterRegister(0));
    ASSERT(object == ArkSteedAssembler::GetParameterRegister(1));
    ASSERT(value == ArkSteedAssembler::GetParameterRegister(3));

    __ Move(offsetScratch, static_cast<int64_t>(offset));
    __ CallNGCRuntime(RTSTUB_ID(ASMFastWriteBarrier));
}

void ArkSteedWriteBarrierEmitter::EmitLocalToShareRSet(ArkSteedRegister glue, ArkSteedRegister object,
                                                       ArkSteedRegister value, int32_t offset,
                                                       ArkSteedRegister objectRegionScratch,
                                                       ArkSteedRegister bitsetWordAddrScratch,
                                                       ArkSteedRegister bitScratch,
                                                       Label *next)
{
    ASSERT(objectRegionScratch != bitsetWordAddrScratch);
    ASSERT(objectRegionScratch != bitScratch);
    ASSERT(bitsetWordAddrScratch != bitScratch);

    Label callRuntime;
    constexpr int64_t REGION_BASE_MASK = static_cast<int64_t>(~(JSTaggedValue::TAG_MARK | DEFAULT_REGION_MASK));
    constexpr uint32_t BITSET_WORD_BYTE_INDEX_SHIFT =
        TAGGED_TYPE_SIZE_LOG + GCBitset::BIT_PER_WORD_LOG2 - GCBitset::BYTE_PER_WORD_LOG2;
    constexpr uint32_t BITSET_WORD_BYTE_INDEX_MASK = static_cast<uint32_t>(~uint64_t{0} >> TAGGED_TYPE_SIZE_LOG) >>
        GCBitset::BIT_PER_WORD_LOG2 << GCBitset::BYTE_PER_WORD_LOG2;
    static_assert(BITSET_WORD_BYTE_INDEX_MASK == 0x1ffffffc, "LocalToShareSet byte index layout changed");

    ArkSteedRegister regionBase = objectRegionScratch;
    ArkSteedRegister bitsetWordAddr = bitsetWordAddrScratch;
    ArkSteedRegister slotOffset = bitScratch;

    __ Move(regionBase, object);
    __ And(regionBase, REGION_BASE_MASK);
    __ LoadField(bitsetWordAddr, regionBase,
                 static_cast<int32_t>(Region::PackedData::GetLocalToShareSetOffset(false)));
    __ Compare(bitsetWordAddr, 0);
    __ JumpIf(Condition::COND_EQUAL, &callRuntime);

    __ Move(slotOffset, object);
    __ And(slotOffset, static_cast<int64_t>(DEFAULT_REGION_MASK));
    __ Add(slotOffset, offset);
    __ ShiftRightLogical(slotOffset, BITSET_WORD_BYTE_INDEX_SHIFT);
    __ And(slotOffset, static_cast<int64_t>(BITSET_WORD_BYTE_INDEX_MASK));
    __ Add(bitsetWordAddr, static_cast<int32_t>(RememberedSet::GCBITSET_DATA_OFFSET));
    __ Add(bitsetWordAddr, slotOffset);

    __ Move(slotOffset, object);
    __ And(slotOffset, static_cast<int64_t>(DEFAULT_REGION_MASK));
    __ Add(slotOffset, offset);
    __ ShiftRightLogical32(slotOffset, TAGGED_TYPE_SIZE_LOG);
    __ And(slotOffset, static_cast<int64_t>(GCBitset::BIT_PER_WORD_MASK));
    ArkSteedRegister bitMask = objectRegionScratch;
    __ MoveBitMask32(bitMask, slotOffset);

    __ LoadInt32Field(slotOffset, bitsetWordAddr, 0);
    __ And(slotOffset, bitMask);
    __ Compare(slotOffset, 0);
    __ JumpIf(Condition::COND_NOT_EQUAL, next);

    __ LoadInt32Field(slotOffset, bitsetWordAddr, 0);
    __ Or(slotOffset, bitMask);
    __ StoreInt32Field(slotOffset, bitsetWordAddr, 0);
    __ Jump(next);

    __ Bind(&callRuntime);
    CallBarrierRuntime(RTSTUB_ID(InsertLocalToShareRSet), glue, object, offset, value, bitsetWordAddrScratch, true);
}

void ArkSteedWriteBarrierEmitter::EmitWriteBarrier(ArkSteedRegister glue, ArkSteedRegister object,
                                                   ArkSteedRegister value, int32_t offset,
                                                   ArkSteedWriteBarrierKind barrierKind,
                                                   ArkSteedRegister primaryScratch,
                                                   ArkSteedRegister offsetScratch,
                                                   ArkSteedRegister secondaryScratch)
{
    switch (barrierKind) {
        case ArkSteedWriteBarrierKind::NO_BARRIER:
            return;
        case ArkSteedWriteBarrierKind::GENERIC_BARRIER:
        case ArkSteedWriteBarrierKind::SHARED_BARRIER:
            break;
    }

    Label done;
    Label valueNotShared;
    Label checkSharedMarking;
    Label checkNormalMarking;

    constexpr int64_t REGION_BASE_MASK = static_cast<int64_t>(~(JSTaggedValue::TAG_MARK | DEFAULT_REGION_MASK));
    ArkSteedRegister objectRegion = primaryScratch;
    ArkSteedRegister valueRegion = offsetScratch;
    __ Move(objectRegion, object);
    __ And(objectRegion, REGION_BASE_MASK);
    __ Move(valueRegion, value);
    __ And(valueRegion, REGION_BASE_MASK);

    __ LoadField(valueRegion, valueRegion, static_cast<int32_t>(Region::PackedData::GetFlagsOffset(false)));
    __ And(valueRegion, static_cast<int64_t>(RegionSpaceFlag::VALID_SPACE_MASK));

    if (barrierKind == ArkSteedWriteBarrierKind::GENERIC_BARRIER) {
        __ Compare(valueRegion, static_cast<int32_t>(RegionSpaceFlag::SHARED_SPACE_BEGIN));
        __ JumpIf(Condition::COND_LESS_THAN, &valueNotShared);
        __ Compare(valueRegion, static_cast<int32_t>(RegionSpaceFlag::SHARED_SPACE_END));
        __ JumpIf(Condition::COND_GREATER_THAN, &valueNotShared);
        __ Jump(&checkSharedMarking);

        __ Bind(&valueNotShared);
        Label notOldToYoung;
        __ LoadField(objectRegion, objectRegion,
                              static_cast<int32_t>(Region::PackedData::GetFlagsOffset(false)));
        __ And(objectRegion, static_cast<int64_t>(RegionSpaceFlag::VALID_SPACE_MASK));
        __ Compare(objectRegion, static_cast<int32_t>(RegionSpaceFlag::GENERAL_OLD_BEGIN));
        __ JumpIf(Condition::COND_LESS_THAN, &notOldToYoung);
        __ Compare(objectRegion, static_cast<int32_t>(RegionSpaceFlag::GENERAL_OLD_END));
        __ JumpIf(Condition::COND_GREATER_THAN, &notOldToYoung);
        __ Compare(valueRegion, static_cast<int32_t>(RegionSpaceFlag::IN_YOUNG_SPACE));
        __ JumpIf(Condition::COND_NOT_EQUAL, &notOldToYoung);
        CallBarrierRuntime(RTSTUB_ID(InsertOldToNewRSet), glue, object, offset, value, offsetScratch, true);
        __ Bind(&notOldToYoung);
        __ Jump(&checkNormalMarking);
    }

    __ Bind(&checkSharedMarking);
    Label notLocalToShare;
    __ Compare(valueRegion, static_cast<int32_t>(RegionSpaceFlag::SHARED_SWEEPABLE_SPACE_BEGIN));
    __ JumpIf(Condition::COND_LESS_THAN, &done);
    __ Compare(valueRegion, static_cast<int32_t>(RegionSpaceFlag::SHARED_SWEEPABLE_SPACE_END));
    __ JumpIf(Condition::COND_GREATER_THAN, &done);
    __ Move(objectRegion, object);
    __ And(objectRegion, REGION_BASE_MASK);
    __ LoadField(objectRegion, objectRegion, static_cast<int32_t>(Region::PackedData::GetFlagsOffset(false)));
    __ And(objectRegion, static_cast<int64_t>(RegionSpaceFlag::VALID_SPACE_MASK));
    __ Compare(objectRegion, static_cast<int32_t>(RegionSpaceFlag::SHARED_SPACE_BEGIN));
    __ JumpIf(Condition::COND_GREATER_THAN_OR_EQUAL, &notLocalToShare);
    EmitLocalToShareRSet(glue, object, value, offset, primaryScratch, offsetScratch, secondaryScratch,
                         &notLocalToShare);
    __ Bind(&notLocalToShare);

    __ Move(objectRegion, glue);
    __ LoadField(objectRegion, objectRegion,
                          static_cast<int32_t>(JSThread::GlueData::GetSharedGCStateBitFieldOffset(false)));
    __ And(objectRegion, static_cast<int64_t>(JSThread::SHARED_CONCURRENT_MARKING_BITFIELD_MASK));
    __ Compare(objectRegion, static_cast<int32_t>(SharedMarkStatus::READY_TO_CONCURRENT_MARK));
    __ JumpIf(Condition::COND_EQUAL, &done);
    CallBarrierRuntime(RTSTUB_ID(SharedGCMarkingBarrier), glue, object, offset, value, offsetScratch, false);
    __ Jump(&done);

    if (barrierKind == ArkSteedWriteBarrierKind::GENERIC_BARRIER) {
        __ Bind(&checkNormalMarking);
        __ Move(objectRegion, glue);
        __ LoadField(objectRegion, objectRegion,
                              static_cast<int32_t>(JSThread::GlueData::GetGCStateBitFieldOffset(false)));
        __ And(objectRegion, static_cast<int64_t>(JSThread::CONCURRENT_MARKING_BITFIELD_MASK));
        __ Compare(objectRegion, static_cast<int32_t>(MarkStatus::READY_TO_MARK));
        __ JumpIf(Condition::COND_EQUAL, &done);
        CallBarrierRuntime(RTSTUB_ID(MarkingBarrier), glue, object, offset, value, offsetScratch, false);
    }

    __ Bind(&done);
}

void ArkSteedWriteBarrierEmitter::EmitPostStoreWriteBarrier(ArkSteedRegister glue, ArkSteedRegister object,
                                                            ArkSteedRegister value, int32_t offset,
                                                            ArkSteedWriteBarrierKind barrierKind,
                                                            ArkSteedRegister primaryScratch,
                                                            ArkSteedRegister offsetScratch,
                                                            ArkSteedRegister secondaryScratch)
{
    if (barrierKind == ArkSteedWriteBarrierKind::GENERIC_BARRIER) {
        EmitFastWriteBarrier(glue, object, value, offset, offsetScratch);
        return;
    }
    EmitWriteBarrier(glue, object, value, offset, barrierKind, primaryScratch, offsetScratch, secondaryScratch);
}

void ArkSteedWriteBarrierEmitter::StoreTaggedField(ArkSteedRegister glue, ArkSteedRegister object,
                                                   ArkSteedRegister value, int32_t offset,
                                                   ArkSteedWriteBarrierKind barrierKind,
                                                   ArkSteedRegister primaryScratch,
                                                   ArkSteedRegister offsetScratch,
                                                   ArkSteedRegister secondaryScratch,
                                                   ArkSteedWriteBarrierValueKind valueKind)
{
    if (barrierKind == ArkSteedWriteBarrierKind::NO_BARRIER ||
        valueKind == ArkSteedWriteBarrierValueKind::NonHeap) {
        __ StoreField(value, object, offset);
        return;
    }

    if (valueKind == ArkSteedWriteBarrierValueKind::HeapObject) {
        __ StoreField(value, object, offset);
        EmitPostStoreWriteBarrier(glue, object, value, offset, barrierKind, primaryScratch, offsetScratch,
                                  secondaryScratch);
        return;
    }

    Label needsBarrier;
    Label done;
    __ Move(primaryScratch, value);
    __ And(primaryScratch, static_cast<int64_t>(JSTaggedValue::TAG_HEAPOBJECT_MASK));
    __ Compare(primaryScratch, 0);
    __ JumpIf(Condition::COND_EQUAL, &needsBarrier);
    __ StoreField(value, object, offset);
    __ Jump(&done);

    __ Bind(&needsBarrier);
    __ StoreField(value, object, offset);
    EmitPostStoreWriteBarrier(glue, object, value, offset, barrierKind, primaryScratch, offsetScratch,
                              secondaryScratch);
    __ Bind(&done);
}

#undef __
}  // namespace panda::ecmascript::arksteed
