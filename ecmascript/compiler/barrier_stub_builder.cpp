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

#include "ecmascript/compiler/barrier_stub_builder.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript::kungfu {
void BarrierStubBuilder::DoBatchBarrier()
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label handleMark(env);
    Label handleBitSet(env);
    BRANCH_NO_WEIGHT(InSharedHeap(objectRegion_), &handleMark, &handleBitSet);
    Bind(&handleBitSet);
    {
        DoBatchBarrierInternal();
        Jump(&handleMark);
    }

    Bind(&handleMark);
    HandleMark();
    Jump(&exit);
    Bind(&exit);
    env->SubCfgExit();
}

void BarrierStubBuilder::DoBatchBarrierInternal()
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label inYoung(env);
    Label notInYoung(env);
    BRANCH_NO_WEIGHT(InYoungGeneration(objectRegion_), &inYoung, &notInYoung);
    Bind(&notInYoung);
    {
        BarrierBatchBitSet(LocalToShared | OldToNew);
        Jump(&exit);
    }
    Bind(&inYoung);
    {
        BarrierBatchBitSet(LocalToShared);
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

GateRef BarrierStubBuilder::GetBitSetDataAddr(GateRef objectRegion, GateRef loadOffset, int32_t createFunID)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(oldBitSet, VariableType::NATIVE_POINTER(),
                Load(VariableType::NATIVE_POINTER(), objectRegion, loadOffset));

    Label createRset(env);
    BRANCH_UNLIKELY(IntPtrEqual(*oldBitSet, IntPtr(0)), &createRset, &exit);
    Bind(&createRset);
    {
        oldBitSet = CallNGCRuntime(glue_, createFunID, {objectRegion});
        Jump(&exit);
    }

    Bind(&exit);
    GateRef bitSetDataAddr = PtrAdd(*oldBitSet, IntPtr(RememberedSet::GCBITSET_DATA_OFFSET));
    env->SubCfgExit();
    return bitSetDataAddr;
}

void BarrierStubBuilder::HandleMark()
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    bool isArch32 = GetEnvironment()->Is32Bit();
    GateRef gcStateBitField = Load(VariableType::INT64(), glue_,
                                   Int64(JSThread::GlueData::GetGCStateBitFieldOffset(isArch32)));
    GateRef gcState = Int64And(gcStateBitField, Int64(JSThread::CONCURRENT_MARKING_BITFIELD_MASK));
    GateRef sharedGCStateBitField = Load(VariableType::INT64(), glue_,
                                         Int64(JSThread::GlueData::GetSharedGCStateBitFieldOffset(isArch32)));
    GateRef sharedGCState = Int64And(sharedGCStateBitField, Int64(JSThread::SHARED_CONCURRENT_MARKING_BITFIELD_MASK));
    Label needMarking(env);
    GateRef marking = Int64NotEqual(gcState, Int64(static_cast<int64_t>(MarkStatus::READY_TO_MARK)));
    GateRef sharedMarking = Int64NotEqual(sharedGCState,
                                          Int64(static_cast<int64_t>(SharedMarkStatus::READY_TO_CONCURRENT_MARK)));
    BRANCH_LIKELY(BitAnd(BoolNot(marking), BoolNot(sharedMarking)), &exit, &needMarking);
    Bind(&needMarking);
    Label begin(env);
    Label body(env);
    Label endLoop(env);
    DEFVARIABLE(index, VariableType::INT32(), Int32(0));
    Jump(&begin);
    LoopBegin(&begin);
    {
        BRANCH_LIKELY(Int32UnsignedLessThan(*index, slotCount_), &body, &exit);
        Bind(&body);
        {
            GateRef offset = PtrMul(ZExtInt32ToPtr(*index), IntPtr(JSTaggedValue::TaggedTypeSize()));
            GateRef value = Load(VariableType::JS_ANY(), dstAddr_, offset);
            Label doMarking(env);
            BRANCH_NO_WEIGHT(TaggedIsHeapObject(value), &doMarking, &endLoop);
            Bind(&doMarking);

            GateRef valueRegion = ObjectAddressToRange(value);
            Label callMarking(env);
            Label doSharedMarking(env);
            BRANCH_NO_WEIGHT(LogicAndBuilder(env).And(marking).And(BoolNot(InSharedHeap(valueRegion))).Done(),
                             &callMarking, &doSharedMarking);
            Bind(&callMarking);
            {
                CallNGCRuntime(glue_, RTSTUB_ID(MarkingBarrier), {glue_, dstAddr_, offset, value});
                Jump(&endLoop);
            }
            Bind(&doSharedMarking);
            {
                Label callSharedMarking(env);
                BRANCH_NO_WEIGHT(
                    LogicAndBuilder(env).And(sharedMarking).And(InSharedSweepableSpace(valueRegion)).Done(),
                    &callSharedMarking, &endLoop);
                Bind(&callSharedMarking);
                {
                    CallNGCRuntime(glue_, RTSTUB_ID(SharedGCMarkingBarrier), {glue_, value});
                    Jump(&endLoop);
                }
            }
        }
    }
    Bind(&endLoop);
    index = Int32Add(*index, Int32(1));
    LoopEnd(&begin);
    Bind(&exit);
    env->SubCfgExit();
}

void BarrierStubBuilder::BarrierBatchBitSet(uint8_t bitSetSelect)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    Label beforeExit(env);
    Label exit(env);
    Label begin(env);
    Label loopBody(env);
    Label endLoop(env);
    GateRef initByteIdx = TruncPtrToInt32(IntPtrLSR(PtrSub(TaggedCastToIntPtr(dstAddr_), objectRegion_),
                                                    IntPtr(TAGGED_TYPE_SIZE_LOG)));
    GateRef initQuadIdx = Int32LSR(initByteIdx, Int32(BIT_PER_QUAD_LOG2));
    DEFVARIABLE(quadIdx, VariableType::INT32(), initQuadIdx);
    DEFVARIABLE(localToShareBitSet, VariableType::INT64(), Int64(0));
    DEFVARIABLE(oldToNewBitSet, VariableType::INT64(), Int64(0));
    DEFVARIABLE(index, VariableType::INT32(), initByteIdx);

    Jump(&begin);
    LoopBegin(&begin);
    {
        BRANCH_NO_WEIGHT(Int32UnsignedLessThan(*index, Int32Add(initByteIdx, slotCount_)), &loopBody, &beforeExit);
        Bind(&loopBody);
        {
            Label checkLocalRegion(env);
            Label updateLocalToShared(env);
            Label updateOldToNew(env);
            GateRef offset = PtrMul(ZExtInt32ToPtr(*index), IntPtr(JSTaggedValue::TaggedTypeSize()));
            GateRef value = Load(VariableType::JS_ANY(), objectRegion_, offset);
            Label doUpdate(env);
            BRANCH_NO_WEIGHT(TaggedIsHeapObject(value), &doUpdate, &endLoop);
            Bind(&doUpdate);
            {
                GateRef valueRegion = ObjectAddressToRange(value);
                Label flushLast(env);
                Label curCheck(env);
                GateRef curQuadIdx = Int32LSR(*index, Int32(BIT_PER_QUAD_LOG2));
                GateRef bitOffset = Int32And(*index, Int32(BIT_PER_QUAD_MASK));
                GateRef mask = Int64LSL(Int64(1), ZExtInt32ToInt64(bitOffset));
                BRANCH_LIKELY(Int32Equal(curQuadIdx, *quadIdx), &curCheck, &flushLast);
                Bind(&flushLast);
                {
                    Label updateIdx(env);
                    FlushBatchBitSet(bitSetSelect, *quadIdx,  localToShareBitSet, oldToNewBitSet, &updateIdx);
                    Bind(&updateIdx);
                    quadIdx = curQuadIdx;
                    Jump(&curCheck);
                }
                Bind(&curCheck);
                if ((bitSetSelect & LocalToShared) != 0) {
                    BRANCH_NO_WEIGHT(InSharedSweepableSpace(valueRegion), &updateLocalToShared, &checkLocalRegion);
                    Bind(&updateLocalToShared);
                    {
                        localToShareBitSet = Int64Or(*localToShareBitSet, mask);
                        Jump(&endLoop);
                    }
                    Bind(&checkLocalRegion);
                }
                if ((bitSetSelect & OldToNew) != 0) {
                    BRANCH_NO_WEIGHT(InYoungGeneration(valueRegion), &updateOldToNew, &endLoop);
                    Bind(&updateOldToNew);
                    {
                        oldToNewBitSet = Int64Or(*oldToNewBitSet, mask);
                    }
                }
                Jump(&endLoop);
            }
        }
        Bind(&endLoop);
        index = Int32Add(*index, Int32(1));
        LoopEnd(&begin);
    }
    Bind(&beforeExit);
    FlushBatchBitSet(bitSetSelect, *quadIdx, localToShareBitSet, oldToNewBitSet, &exit);
    Bind(&exit);
    env->SubCfgExit();
}

void BarrierStubBuilder::FlushBatchBitSet(uint8_t bitSetSelect, GateRef quadIdx,
                                          Variable &localToShareBitSetVar, Variable &oldToNewBitSetVar, Label* next)
{
    auto env = GetEnvironment();
    Label batchUpdateLocalToShare(env);
    Label checkNext(env);
    Label batchUpdateOldToNew(env);
    if ((bitSetSelect & LocalToShared) != 0) {
        BRANCH_LIKELY(Int64NotEqual(*localToShareBitSetVar, Int64(0)), &batchUpdateLocalToShare, &checkNext);
        Bind(&batchUpdateLocalToShare);
        {
            GateRef loadOffset = IntPtr(Region::PackedData::GetLocalToShareSetOffset(env->Is32Bit()));
            GateRef bitSetAddr = GetBitSetDataAddr(objectRegion_, loadOffset, RTSTUB_ID(CreateLocalToShare));

            GateRef byteIndex = Int32LSL(quadIdx, Int32(BYTE_PER_QUAD_LOG2));
            GateRef oldValue = Load(VariableType::INT64(), bitSetAddr, ZExtInt32ToInt64(byteIndex));
            Store(VariableType::INT64(), glue_, bitSetAddr, ZExtInt32ToInt64(byteIndex),
                  Int64Or(oldValue, *localToShareBitSetVar), MemoryAttribute::NoBarrier());
            localToShareBitSetVar = Int64(0);
            Jump(&checkNext);
        }
        Bind(&checkNext);
    }
    if ((bitSetSelect & OldToNew) != 0) {
        BRANCH_LIKELY(Int64NotEqual(*oldToNewBitSetVar, Int64(0)), &batchUpdateOldToNew, next);
        Bind(&batchUpdateOldToNew);
        {
            GateRef loadOffset = IntPtr(Region::PackedData::GetOldToNewSetOffset(env->Is32Bit()));
            GateRef bitSetAddr = GetBitSetDataAddr(objectRegion_, loadOffset, RTSTUB_ID(CreateOldToNew));
            GateRef byteIndex = Int32LSL(quadIdx, Int32(BYTE_PER_QUAD_LOG2));
            GateRef oldValue = Load(VariableType::INT64(), bitSetAddr, ZExtInt32ToInt64(byteIndex));
            Store(VariableType::INT64(), glue_, bitSetAddr, ZExtInt32ToInt64(byteIndex),
                  Int64Or(oldValue, *oldToNewBitSetVar), MemoryAttribute::NoBarrier());
            oldToNewBitSetVar = Int64(0);
        }
    }
    Jump(next);
}

}
