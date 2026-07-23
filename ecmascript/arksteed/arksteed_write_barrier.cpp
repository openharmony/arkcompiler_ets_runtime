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
namespace {

ArkSteedRegList GetBarrierCallArgumentRegisters()
{
    return ArkSteedRegList{ArkSteedAssembler::GetParameterRegister(0),
                           ArkSteedAssembler::GetParameterRegister(1),
                           ArkSteedAssembler::GetParameterRegister(2),
                           ArkSteedAssembler::GetParameterRegister(3)};
}

class DeferredBarrierCall final : public ArkSteedDeferredCode {
public:
    DeferredBarrierCall(kungfu::RuntimeStubCSigns::ID runtimeId, ArkSteedRegister glue,
                        ArkSteedRegister object, int32_t offset, ArkSteedRegister value,
                        const DeferredRegisterSnapshot &registerSnapshot, bool preserveInputs)
        : runtimeId_(runtimeId),
          glue_(glue),
          object_(object),
          offset_(offset),
          value_(value),
          registerSnapshot_(registerSnapshot),
          preserveInputs_(preserveInputs)
    {}

    Label *GetReturnLabel()
    {
        return &returnLabel_;
    }

    void Generate(ArkSteedAssembler *assembler) override
    {
        ArkSteedRegList clobberedRegisters;
        ArkDoubleRegList clobberedDoubleRegisters;
        switch (runtimeId_) {
            case RTSTUB_ID(ASMFastWriteBarrier):
                clobberedRegisters = GetASMFastWriteBarrierClobberedGeneralRegisters();
                // The stub preserves its argument registers, but the deferred adapter overwrites them
                // while materializing arbitrary ArkSteed input locations into the barrier ABI.
                clobberedRegisters |= GetBarrierCallArgumentRegisters();
                clobberedDoubleRegisters = GetASMFastWriteBarrierClobberedDoubleRegisters();
                break;
            case RTSTUB_ID(InsertOldToNewRSet):
            case RTSTUB_ID(InsertLocalToShareRSet):
            case RTSTUB_ID(MarkingBarrier):
            case RTSTUB_ID(SharedGCMarkingBarrier):
                clobberedRegisters = GetNGCRuntimeCallerSavedGeneralRegisters();
                clobberedDoubleRegisters = GetNGCRuntimeCallerSavedDoubleRegisters();
                break;
            default:
                UNREACHABLE();
        }

        ArkSteedRegList liveRegisters = registerSnapshot_.liveRegisters;
        if (preserveInputs_) {
            liveRegisters.Set(glue_);
            liveRegisters.Set(object_);
            liveRegisters.Set(value_);
        }
        liveRegisters &= clobberedRegisters;
        ArkDoubleRegList liveDoubleRegisters = registerSnapshot_.liveDoubleRegisters;
        liveDoubleRegisters &= clobberedDoubleRegisters;

        for (ArkSteedRegister reg : liveRegisters) {
            assembler->Push(reg);
        }
        for (ArkSteedDoubleRegister reg : liveDoubleRegisters) {
            assembler->Push(reg);
        }

#if defined(PANDA_TARGET_AMD64)
        bool needsAlignmentSlot = ((liveRegisters.Count() + liveDoubleRegisters.Count()) & 1U) != 0;
        if (needsAlignmentSlot) {
            assembler->ReserveCallArgSlots(1);
        }
#endif

        constexpr int32_t GLUE_SLOT = 0;
        constexpr int32_t OBJECT_SLOT = 1;
        constexpr int32_t OFFSET_SLOT = 2;
        constexpr int32_t VALUE_SLOT = 3;
        constexpr int32_t ABI_ARG_SLOT_COUNT = 4;
        assembler->ReserveCallArgSlots(ABI_ARG_SLOT_COUNT);
        assembler->MoveRepr(MachineRepresentation::Word64, assembler->GetCallArgSlot(GLUE_SLOT), glue_);
        assembler->MoveRepr(MachineRepresentation::Tagged, assembler->GetCallArgSlot(OBJECT_SLOT), object_);
        assembler->MoveRepr(MachineRepresentation::Tagged, assembler->GetCallArgSlot(VALUE_SLOT), value_);

        ArkSteedRegister glueArg = ArkSteedAssembler::GetParameterRegister(0);
        ArkSteedRegister objectArg = ArkSteedAssembler::GetParameterRegister(1);
        ArkSteedRegister offsetArg = ArkSteedAssembler::GetParameterRegister(2);
        ArkSteedRegister valueArg = ArkSteedAssembler::GetParameterRegister(3);
        assembler->Move(offsetArg, static_cast<int64_t>(offset_));
        assembler->MoveRepr(MachineRepresentation::Word64, assembler->GetCallArgSlot(OFFSET_SLOT), offsetArg);

        assembler->MoveRepr(MachineRepresentation::Word64, glueArg, assembler->GetCallArgSlot(GLUE_SLOT));
        assembler->MoveRepr(MachineRepresentation::Tagged, objectArg, assembler->GetCallArgSlot(OBJECT_SLOT));
        assembler->MoveRepr(MachineRepresentation::Word64, offsetArg, assembler->GetCallArgSlot(OFFSET_SLOT));
        assembler->MoveRepr(MachineRepresentation::Tagged, valueArg, assembler->GetCallArgSlot(VALUE_SLOT));
        assembler->CallNGCRuntime(runtimeId_);
        assembler->FreeCallArgSlots(ABI_ARG_SLOT_COUNT);

#if defined(PANDA_TARGET_AMD64)
        if (needsAlignmentSlot) {
            assembler->FreeCallArgSlots(1);
        }
#endif

        for (int code = ArkSteedDoubleRegister::NUM_REGISTERS - 1; code >= 0; --code) {
            ArkSteedDoubleRegister reg = ArkSteedDoubleRegister::FromCode(code);
            if (liveDoubleRegisters.Has(reg)) {
                assembler->Pop(reg);
            }
        }
        for (int code = ArkSteedRegister::NUM_REGISTERS - 1; code >= 0; --code) {
            ArkSteedRegister reg = ArkSteedRegister::FromCode(code);
            if (liveRegisters.Has(reg)) {
                assembler->Pop(reg);
            }
        }
        assembler->Jump(&returnLabel_);
    }

private:
    kungfu::RuntimeStubCSigns::ID runtimeId_;
    ArkSteedRegister glue_;
    ArkSteedRegister object_;
    int32_t offset_;
    ArkSteedRegister value_;
    DeferredRegisterSnapshot registerSnapshot_;
    bool preserveInputs_;
    Label returnLabel_;
};

}  // namespace

#define __ assembler_->

void ArkSteedWriteBarrierEmitter::CallBarrierRuntime(kungfu::RuntimeStubCSigns::ID runtimeId, ArkSteedRegister glue,
                                                     ArkSteedRegister object, int32_t offset,
                                                     ArkSteedRegister value, bool preserveInputs)
{
    ASSERT(chunk_ != nullptr);
    ASSERT(deferredCode_ != nullptr);
    auto *deferredCall = chunk_->New<DeferredBarrierCall>(runtimeId, glue, object, offset, value,
                                                          registerSnapshot_, preserveInputs);
    deferredCode_->push_back(deferredCall);
    __ Jump(deferredCall->GetEntryLabel());
    __ Bind(deferredCall->GetReturnLabel());
}

void ArkSteedWriteBarrierEmitter::EmitFastWriteBarrier(ArkSteedRegister glue, ArkSteedRegister object,
                                                       ArkSteedRegister value, int32_t offset)
{
    CallBarrierRuntime(RTSTUB_ID(ASMFastWriteBarrier), glue, object, offset, value, false);
}

void ArkSteedWriteBarrierEmitter::EmitLocalToShareRSet(ArkSteedRegister glue, ArkSteedRegister object,
                                                       ArkSteedRegister value, int32_t offset,
                                                       ArkSteedRegister objectRegionScratch,
                                                       ArkSteedRegister bitsetWordAddrScratch,
                                                       Label *next)
{
    ASSERT(objectRegionScratch != bitsetWordAddrScratch);
    ASSERT(glue != objectRegionScratch);
    ASSERT(glue != bitsetWordAddrScratch);

    Label callRuntime;
    Label restoreGlue;
    constexpr int64_t REGION_BASE_MASK = static_cast<int64_t>(~(JSTaggedValue::TAG_MARK | DEFAULT_REGION_MASK));
    constexpr uint32_t BITSET_WORD_BYTE_INDEX_SHIFT =
        TAGGED_TYPE_SIZE_LOG + GCBitset::BIT_PER_WORD_LOG2 - GCBitset::BYTE_PER_WORD_LOG2;
    constexpr uint32_t BITSET_WORD_BYTE_INDEX_MASK = static_cast<uint32_t>(~uint64_t{0} >> TAGGED_TYPE_SIZE_LOG) >>
        GCBitset::BIT_PER_WORD_LOG2 << GCBitset::BYTE_PER_WORD_LOG2;
    static_assert(BITSET_WORD_BYTE_INDEX_MASK == 0x1ffffffc, "LocalToShareSet byte index layout changed");

    ArkSteedRegister regionBase = objectRegionScratch;
    ArkSteedRegister bitsetWordAddr = bitsetWordAddrScratch;
    ArkSteedRegister slotOffset = glue;

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
    __ JumpIf(Condition::COND_NOT_EQUAL, &restoreGlue);

    __ LoadInt32Field(slotOffset, bitsetWordAddr, 0);
    __ Or(slotOffset, bitMask);
    __ StoreInt32Field(slotOffset, bitsetWordAddr, 0);
    __ Jump(&restoreGlue);

    __ Bind(&callRuntime);
    CallBarrierRuntime(RTSTUB_ID(InsertLocalToShareRSet), glue, object, offset, value, true);
    __ Jump(next);

    __ Bind(&restoreGlue);
    __ LoadGlue(glue);
    __ Jump(next);
}

void ArkSteedWriteBarrierEmitter::EmitWriteBarrier(ArkSteedRegister glue, ArkSteedRegister object,
                                                   ArkSteedRegister value, int32_t offset,
                                                   ArkSteedWriteBarrierKind barrierKind,
                                                   ArkSteedRegister objectRegionScratch,
                                                   ArkSteedRegister valueRegionScratch)
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
    ArkSteedRegister objectRegion = objectRegionScratch;
    ArkSteedRegister valueRegion = valueRegionScratch;
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
        __ Compare(objectRegion, static_cast<int32_t>(RegionSpaceFlag::IN_YOUNG_SPACE));
        __ JumpIf(Condition::COND_EQUAL, &notOldToYoung);
        __ Compare(valueRegion, static_cast<int32_t>(RegionSpaceFlag::IN_YOUNG_SPACE));
        __ JumpIf(Condition::COND_NOT_EQUAL, &notOldToYoung);
        CallBarrierRuntime(RTSTUB_ID(InsertOldToNewRSet), glue, object, offset, value, true);
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
    EmitLocalToShareRSet(glue, object, value, offset, objectRegionScratch, valueRegionScratch, &notLocalToShare);
    __ Bind(&notLocalToShare);

    __ Move(objectRegion, glue);
    __ LoadField(objectRegion, objectRegion,
                          static_cast<int32_t>(JSThread::GlueData::GetSharedGCStateBitFieldOffset(false)));
    __ And(objectRegion, static_cast<int64_t>(JSThread::SHARED_CONCURRENT_MARKING_BITFIELD_MASK));
    __ Compare(objectRegion, static_cast<int32_t>(SharedMarkStatus::READY_TO_CONCURRENT_MARK));
    __ JumpIf(Condition::COND_EQUAL, &done);
    CallBarrierRuntime(RTSTUB_ID(SharedGCMarkingBarrier), glue, object, offset, value, false);
    __ Jump(&done);

    if (barrierKind == ArkSteedWriteBarrierKind::GENERIC_BARRIER) {
        __ Bind(&checkNormalMarking);
        __ Move(objectRegion, glue);
        __ LoadField(objectRegion, objectRegion,
                              static_cast<int32_t>(JSThread::GlueData::GetGCStateBitFieldOffset(false)));
        __ And(objectRegion, static_cast<int64_t>(JSThread::CONCURRENT_MARKING_BITFIELD_MASK));
        __ Compare(objectRegion, static_cast<int32_t>(MarkStatus::READY_TO_MARK));
        __ JumpIf(Condition::COND_EQUAL, &done);
        CallBarrierRuntime(RTSTUB_ID(MarkingBarrier), glue, object, offset, value, false);
    }

    __ Bind(&done);
}

void ArkSteedWriteBarrierEmitter::EmitPostStoreWriteBarrier(ArkSteedRegister glue, ArkSteedRegister object,
                                                            ArkSteedRegister value, int32_t offset,
                                                            ArkSteedWriteBarrierKind barrierKind,
                                                            ArkSteedRegister objectRegionScratch,
                                                            ArkSteedRegister valueRegionScratch)
{
#if USE_STICKY_CMS_GC
    if (barrierKind == ArkSteedWriteBarrierKind::GENERIC_BARRIER) {
        EmitFastWriteBarrier(glue, object, value, offset);
        return;
    }
#endif
    EmitWriteBarrier(glue, object, value, offset, barrierKind, objectRegionScratch, valueRegionScratch);
}

void ArkSteedWriteBarrierEmitter::StoreTaggedField(ArkSteedRegister glue, ArkSteedRegister object,
                                                   ArkSteedRegister value, int32_t offset,
                                                   ArkSteedWriteBarrierKind barrierKind,
                                                   ArkSteedRegister objectRegionScratch,
                                                   ArkSteedRegister valueRegionScratch,
                                                   ArkSteedWriteBarrierValueKind valueKind)
{
    if (barrierKind == ArkSteedWriteBarrierKind::NO_BARRIER ||
        valueKind == ArkSteedWriteBarrierValueKind::NonHeap) {
        __ StoreField(value, object, offset);
        return;
    }

    if (valueKind == ArkSteedWriteBarrierValueKind::HeapObject) {
        __ StoreField(value, object, offset);
        EmitPostStoreWriteBarrier(glue, object, value, offset, barrierKind, objectRegionScratch,
                                  valueRegionScratch);
        return;
    }

    Label needsBarrier;
    Label done;
    __ Move(objectRegionScratch, value);
    __ And(objectRegionScratch, static_cast<int64_t>(JSTaggedValue::TAG_HEAPOBJECT_MASK));
    __ Compare(objectRegionScratch, 0);
    __ JumpIf(Condition::COND_EQUAL, &needsBarrier);
    __ StoreField(value, object, offset);
    __ Jump(&done);

    __ Bind(&needsBarrier);
    __ StoreField(value, object, offset);
    EmitPostStoreWriteBarrier(glue, object, value, offset, barrierKind, objectRegionScratch, valueRegionScratch);
    __ Bind(&done);
}

void ArkSteedWriteBarrierEmitter::TransitionHClass(ArkSteedRegister glue, ArkSteedRegister object,
                                                   ArkSteedRegister hclass,
                                                   ArkSteedRegister objectRegionScratch,
                                                   ArkSteedRegister hclassRegionScratch)
{
    static_assert(TaggedObject::HCLASS_OFFSET == 0);
    constexpr int32_t offset = static_cast<int32_t>(TaggedObject::HCLASS_OFFSET);
#ifndef ARK_USE_SATB_BARRIER
    __ StoreInt32FieldRelease(hclass, object, offset);
    EmitPostStoreWriteBarrier(glue, object, hclass, offset, ArkSteedWriteBarrierKind::GENERIC_BARRIER,
                              objectRegionScratch, hclassRegionScratch);
#else
    CallBarrierRuntime(RTSTUB_ID(ASMFastWriteBarrier), glue, object, offset, hclass, true);
    __ StoreInt32FieldRelease(hclass, object, offset);
#endif
}

#undef __
}  // namespace panda::ecmascript::arksteed
