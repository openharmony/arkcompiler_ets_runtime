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

/*  LOCAL_TO_SHARE_SWAPPED 和 OLD_TO_NEW_SWAPPED_MASK标记读取并发场景分析：
*   设置标记：CollectGarbage->RunPhases->Sweep->PrepareSweeping这条路径会设置这两个标记, CollectGarbage只会在当前线程local gc时使用。
*   清除标记：（1）由CreateXXXXRememberedSet清除标记，这个调用点有两种情况：
*                a. js主线程在需要读rset时会调用的方法，一定在js线程。
*                b. gc时移动对象时从gc线程调用，此时主线程是被suspendAll停下来的等待gc线程的状态。
*           （2）CollectGarbage -> RunPhases -> Evacuate -> UpdateReference ->
*               并行UpdateReferenceTask -> UpdateRSet -> Merge：
*               其中有并行任务，会主线程会等待并行任务结束，有同步的保证，可以保证清除能够被主线程观察到。
*
*   LOCAL_TO_SHARE_COLLECTED_MASK标记读取并发场景分析：
*   设置标记：SharedGCMarkBase::MarkRoots-> CollectLocalVMRSet（访问多个线程的heap） -> EnumerateRegions -> ExtractLocalToShareRSet，
*           这里会有跨线程，但是已经在SuspendAll的保证之下了。
*   清除标记：（1）入口在MergeBackAndResetRSetWorkListHandler，这个函数调用点有5个，可能不在当前线程，但是调用点处都有SuspendAll的保证。
*           （2）js线程在local gc结束时，会把自己线程被Collect的rset拿回来，这里只处理当前线程的，没有并发问题。
*/

namespace panda::ecmascript::kungfu {
void BarrierStubBuilder::DoBatchBarrier()
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label checkNext(env);
    BRANCH_UNLIKELY(
        LoadPrimitive(VariableType::BOOL(), glue_, IntPtr(JSThread::GlueData::GetIsEnableCMCGCOffset(env->Is32Bit()))),
        &exit, &checkNext);
    Bind(&checkNext);
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
                LoadPrimitive(VariableType::NATIVE_POINTER(), objectRegion, loadOffset));

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
    GateRef gcStateBitField = LoadPrimitive(VariableType::INT64(), glue_,
        Int64(JSThread::GlueData::GetGCStateBitFieldOffset(isArch32)));
    GateRef gcState = Int64And(gcStateBitField, Int64(JSThread::CONCURRENT_MARKING_BITFIELD_MASK));
    GateRef sharedGCStateBitField = LoadPrimitive(VariableType::INT64(), glue_,
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
            GateRef value = Load(VariableType::JS_ANY(), glue_, dstAddr_, offset);
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
                    CallNGCRuntime(glue_, RTSTUB_ID(SharedGCMarkingBarrier), {glue_, dstAddr_, offset, value});
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
            GateRef value = Load(VariableType::JS_ANY(), glue_, objectRegion_, offset);
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
            GateRef oldValue = LoadPrimitive(VariableType::INT64(), bitSetAddr, ZExtInt32ToInt64(byteIndex));
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
            GateRef oldValue = LoadPrimitive(VariableType::INT64(), bitSetAddr, ZExtInt32ToInt64(byteIndex));
            Store(VariableType::INT64(), glue_, bitSetAddr, ZExtInt32ToInt64(byteIndex),
                  Int64Or(oldValue, *oldToNewBitSetVar), MemoryAttribute::NoBarrier());
            oldToNewBitSetVar = Int64(0);
        }
    }
    Jump(next);
}

void BarrierStubBuilder::DoMoveBarrierCrossRegion(GateRef srcAddr, GateRef srcObj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label checkNext(env);
    BRANCH_UNLIKELY(
        LoadPrimitive(VariableType::BOOL(), glue_, IntPtr(JSThread::GlueData::GetIsEnableCMCGCOffset(env->Is32Bit()))),
        &exit, &checkNext);
    Bind(&checkNext);
    Label handleMark(env);
    Label handleBitSet(env);
    Label doMove(env);
    BRANCH_NO_WEIGHT(Int32Equal(slotCount_, Int32(0)), &exit, &doMove);
    Bind(&doMove);
    BRANCH_NO_WEIGHT(InSharedHeap(objectRegion_), &handleMark, &handleBitSet);
    Bind(&handleBitSet);
    {
        GateRef srcRegion = ObjectAddressToRange(srcObj);
        Label batchBarrier(env);
        Label srcNotInSharedHeap(env);
        // copy shared array to local heap, there is no bitset can be copied, do batch barrier for dst object.
        BRANCH_UNLIKELY(InSharedHeap(srcRegion), &batchBarrier, &srcNotInSharedHeap);
        Bind(&srcNotInSharedHeap);
        {
            GateRef sameKind = LogicOrBuilder(env)
                               .Or(BitAnd(InYoungGeneration(objectRegion_), InYoungGeneration(srcRegion)))
                               .Or(BitAnd(InGeneralOldGeneration(objectRegion_), InGeneralOldGeneration(srcRegion)))
                               .Done();
            Label sameRegionKind(env);
            Label crossRegion(env);
            BRANCH_NO_WEIGHT(sameKind, &sameRegionKind, &crossRegion);
            Bind(&sameRegionKind);
            {
                // dst and src are in same kind region, copy the bitset of them.
                DoMoveBarrierSameRegionKind(srcAddr, srcRegion, CrossRegion);
                Jump(&handleMark);
            }
            Bind(&crossRegion);
            {
                Label copyLocalToShare(env);
                BRANCH_UNLIKELY(IsLocalToShareSwapped(srcRegion), &batchBarrier, &copyLocalToShare);
                Bind(&copyLocalToShare);
                {
                    // dst and src are in different kind region, localToShare bitset can be copied unconditionally,
                    GateRef srcBitStartIdx = Int64LSR(Int64Sub(srcAddr, srcRegion), Int64(TAGGED_TYPE_SIZE_LOG));
                    GateRef dstBitStartIdx = Int64LSR(Int64Sub(dstAddr_, objectRegion_), Int64(TAGGED_TYPE_SIZE_LOG));

                    GateRef localToShareOffset = IntPtr(Region::PackedData::GetLocalToShareSetOffset(env->Is32Bit()));
                    GateRef localToShareBitSetAddr =
                        GetBitSetDataAddr(objectRegion_, localToShareOffset, RTSTUB_ID(CreateLocalToShare));
                    GateRef srcLocalToShareBitSetAddr =
                        GetBitSetDataAddr(srcRegion, localToShareOffset, RTSTUB_ID(CreateLocalToShare));
                    BitSetRangeMoveForward(srcLocalToShareBitSetAddr, localToShareBitSetAddr, srcBitStartIdx,
                                           dstBitStartIdx, ZExtInt32ToInt64(slotCount_));
                    Label inOld(env);
                    BRANCH_NO_WEIGHT(InYoungGeneration(objectRegion_), &handleMark, &inOld);
                    Bind(&inOld);
                    {
                        // copy young object to old region, do BarrierBatchBitSet bitset for oldToNew bitset.
                        BarrierBatchBitSet(OldToNew);
                        Jump(&handleMark);
                    }
                }
            }
        }
        Bind(&batchBarrier);
        {
            DoBatchBarrierInternal();
            Jump(&handleMark);
        }
    }
    Bind(&handleMark);
    HandleMark();
    Jump(&exit);
    Bind(&exit);
    env->SubCfgExit();
}

void BarrierStubBuilder::DoMoveBarrierInRegion(GateRef srcAddr)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label checkNext(env);
    BRANCH_UNLIKELY(
        LoadPrimitive(VariableType::BOOL(), glue_, IntPtr(JSThread::GlueData::GetIsEnableCMCGCOffset(env->Is32Bit()))),
        &exit, &checkNext);
    Bind(&checkNext);
    Label handleMark(env);
    Label handleBitSet(env);
    Label doMove(env);
    BRANCH_NO_WEIGHT(Int32Equal(slotCount_, Int32(0)), &exit, &doMove);
    Bind(&doMove);
    BRANCH_NO_WEIGHT(InSharedHeap(objectRegion_), &handleMark, &handleBitSet);
    Bind(&handleBitSet);
    {
        DoMoveBarrierSameRegionKind(srcAddr, objectRegion_, InSameRegion);
        Jump(&handleMark);
    }
    Bind(&handleMark);
    HandleMark();
    Jump(&exit);
    Bind(&exit);
    env->SubCfgExit();
}

void BarrierStubBuilder::DoMoveBarrierSameRegionKind(GateRef srcAddr, GateRef srcRegion, RegionKind regionKind)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label inYoung(env);
    Label inOld(env);
    Label copyBitSet(env);

    DEFVARIABLE(dstBitSetAddr, VariableType::NATIVE_POINTER(), IntPtr(0));
    DEFVARIABLE(dstBitSetAddr2, VariableType::NATIVE_POINTER(), IntPtr(0));
    DEFVARIABLE(srcBitSetAddr, VariableType::NATIVE_POINTER(), IntPtr(0));
    DEFVARIABLE(srcBitSetAddr2, VariableType::NATIVE_POINTER(), IntPtr(0));

    GateRef localToShareOffset = IntPtr(Region::PackedData::GetLocalToShareSetOffset(env->Is32Bit()));
    GateRef localToShareBitSetAddr =
        GetBitSetDataAddr(objectRegion_, localToShareOffset, RTSTUB_ID(CreateLocalToShare));
    GateRef srcLocalToShareBitSetAddr = Gate::InvalidGateRef;
    if (regionKind == InSameRegion) {
        srcLocalToShareBitSetAddr = localToShareBitSetAddr;
    } else {
        ASSERT(regionKind == CrossRegion);
        srcLocalToShareBitSetAddr = GetBitSetDataAddr(srcRegion, localToShareOffset, RTSTUB_ID(CreateLocalToShare));
    }
    GateRef localToShareSwapped = IsLocalToShareSwapped(srcRegion);
    BRANCH_NO_WEIGHT(InYoungGeneration(objectRegion_), &inYoung, &inOld);
    Bind(&inYoung);
    {
        Label batchBarrier(env);
        Label copyLocalToShare(env);
        BRANCH_UNLIKELY(localToShareSwapped, &batchBarrier, &copyLocalToShare);
        Bind(&batchBarrier);
        {
            // slowpath, localToShareRSet is swapped, it can't be copied, just set the bitset bit by bit.
            BarrierBatchBitSet(LocalToShared);
            Jump(&exit);
        }
        Bind(&copyLocalToShare);
        {
            dstBitSetAddr = localToShareBitSetAddr;
            srcBitSetAddr = srcLocalToShareBitSetAddr;
            Jump(&copyBitSet);
        }
    }
    Bind(&inOld);
    {
        GateRef oldToNewOffset = IntPtr(Region::PackedData::GetOldToNewSetOffset(env->Is32Bit()));
        GateRef oldToNewBitSetAddr = GetBitSetDataAddr(objectRegion_, oldToNewOffset, RTSTUB_ID(CreateOldToNew));
        GateRef srcOldToNewBitSetAddr = GetBitSetDataAddr(srcRegion, oldToNewOffset, RTSTUB_ID(CreateOldToNew));
        // CreateOldToNew may change the RSetFlag, so load it again.
        GateRef oldToNewSwapped = IsOldToNewSwapped(srcRegion);
        Label batchBarrier(env);
        Label tryBatchBarrier(env);
        Label copyBoth(env);
        // both localToShareSwapped and oldToNewSwapped are not swapped.
        BRANCH_LIKELY(BitAnd(BoolNot(localToShareSwapped), BoolNot(oldToNewSwapped)), &copyBoth, &tryBatchBarrier);
        Bind(&copyBoth);
        {
            dstBitSetAddr = localToShareBitSetAddr;
            srcBitSetAddr = srcLocalToShareBitSetAddr;
            dstBitSetAddr2 = oldToNewBitSetAddr;
            srcBitSetAddr2 = srcOldToNewBitSetAddr;
            Jump(&copyBitSet);
        }
        Bind(&tryBatchBarrier);
        {
            Label copyOne(env);
            BRANCH_UNLIKELY(BitAnd(localToShareSwapped, oldToNewSwapped), &batchBarrier, &copyOne);
            Bind(&batchBarrier);
            {
                // slowpath, localToShareRSet and oldToNewRSet are swapped,
                // it can't be copied, just set the bitset bit by bit.
                BarrierBatchBitSet(LocalToShared | OldToNew);
                Jump(&exit);
            }
            Bind(&copyOne);
            {
                Label localToShareBatchBarrier(env);
                Label oldToNewBatchBarrier(env);
                BRANCH_NO_WEIGHT(localToShareSwapped, &localToShareBatchBarrier, &oldToNewBatchBarrier);
                Bind(&localToShareBatchBarrier);
                {
                    // slowpath, localToShareRSet is swapped, it can't be copied, just set the bitset bit by bit.
                    // And copy oldToNewRSet.
                    BarrierBatchBitSet(LocalToShared);
                    dstBitSetAddr = oldToNewBitSetAddr;
                    srcBitSetAddr = srcOldToNewBitSetAddr;
                    Jump(&copyBitSet);
                }
                Bind(&oldToNewBatchBarrier);
                {
                    // slowpath, oldToNewRSet is swapped, it can't be copied, just set the bitset bit by bit.
                    // And copy localToShareRSet.
                    BarrierBatchBitSet(OldToNew);
                    dstBitSetAddr = localToShareBitSetAddr;
                    srcBitSetAddr = srcLocalToShareBitSetAddr;
                    Jump(&copyBitSet);
                }
            }
        }
    }
    Bind(&copyBitSet);
    {
        GateRef srcBitStartIdx = Int64LSR(Int64Sub(srcAddr, srcRegion), Int64(TAGGED_TYPE_SIZE_LOG));
        GateRef dstBitStartIdx = Int64LSR(Int64Sub(dstAddr_, objectRegion_), Int64(TAGGED_TYPE_SIZE_LOG));
        Label begin(env);
        Label endLoop(env);
        Label next(env);
        Jump(&begin);
        LoopBegin(&begin);
        {
            if (regionKind == InSameRegion) {
                BitSetRangeMove(*srcBitSetAddr, *dstBitSetAddr, srcBitStartIdx, dstBitStartIdx,
                                ZExtInt32ToInt64(slotCount_));
            } else {
                ASSERT(regionKind == CrossRegion);
                BitSetRangeMoveForward(*srcBitSetAddr, *dstBitSetAddr, srcBitStartIdx, dstBitStartIdx,
                                       ZExtInt32ToInt64(slotCount_));
            }
            BRANCH_UNLIKELY(IntPtrEqual(*dstBitSetAddr2, IntPtr(0)), &exit, &next);
            Bind(&next);
            {
                dstBitSetAddr = *dstBitSetAddr2;
                srcBitSetAddr = *srcBitSetAddr2;
                dstBitSetAddr2 = IntPtr(0);
                srcBitSetAddr2 = IntPtr(0);
                Jump(&endLoop);
            }
        }
        Bind(&endLoop);
        LoopEnd(&begin);
    }
    Bind(&exit);
    env->SubCfgExit();
}

void BarrierStubBuilder::BitSetRangeMove(GateRef srcBitSet, GateRef dstBitSet, GateRef srcStart, GateRef dstStart,
                                         GateRef length)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    length = ZExtInt32ToInt64(length);

    GateRef forward = LogicOrBuilder(env)
                      .Or(Int64LessThan(dstStart, srcStart))
                      .Or(Int64GreaterThanOrEqual(dstStart, Int64Add(srcStart, length))).Done();
    Label moveForward(env);
    Label moveBackward(env);
    BRANCH_NO_WEIGHT(forward, &moveForward, &moveBackward);
    Bind(&moveForward);
    {
        BitSetRangeMoveForward(srcBitSet, dstBitSet, srcStart, dstStart, length);
        Jump(&exit);
    }
    Bind(&moveBackward);
    {
        BitSetRangeMoveBackward(srcBitSet, dstBitSet, srcStart, dstStart, length);
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

void BarrierStubBuilder::BitSetRangeMoveForward(GateRef srcBitSet, GateRef dstBitSet, GateRef srcStart,
                                                GateRef dstStart, GateRef length)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(remainLength, VariableType::INT64(), length);
    // srcQuad <- srcStart / 64;  // 源起始 uint64_t 元素索引
    // dstQuad <- dstStart / 64; // 目标起始 uint64_t 元素索引
    // srcBitOffset <- srcStart % 64; // 源起始元素的位偏移
    // dstBitOffset <- dstStart % 64; // 目标起始元素的位偏移
    DEFVARIABLE(srcQuad, VariableType::INT64(), Int64LSR(srcStart, Int64(BIT_PER_QUAD_LOG2)));
    DEFVARIABLE(dstQuad, VariableType::INT64(), Int64LSR(dstStart, Int64(BIT_PER_QUAD_LOG2)));
    DEFVARIABLE(srcBitOffset, VariableType::INT64(), Int64And(srcStart, Int64(BIT_PER_QUAD_MASK)));
    DEFVARIABLE(dstBitOffset, VariableType::INT64(), Int64And(dstStart, Int64(BIT_PER_QUAD_MASK)));

    Label begin(env);
    Label body(env);
    Label endLoop(env);
    Jump(&begin);
    LoopBegin(&begin);

    // while remainLength > 0
    // do
    BRANCH_LIKELY(Int64GreaterThan(*remainLength, Int64(0)), &body, &exit);
    Bind(&body);
    {
        Label beforeEndLoop(env);
        // bitsInCurrentQuad <- min of {64 - srcBitOffset, 64 - dstBitOffset, remainLength};
        GateRef bitsInCurrentQuad = ThreeInt64Min(Int64Sub(Int64(BIT_PER_QUAD), *srcBitOffset),
                                                  Int64Sub(Int64(BIT_PER_QUAD), *dstBitOffset),
                                                  *remainLength);
        GateRef srcQuadData = LoadPrimitive(VariableType::INT64(), srcBitSet,
            Int64LSL(*srcQuad, Int64(BYTE_PER_QUAD_LOG2)));
        Label setValue(env);
        BRANCH_NO_WEIGHT(Int64Equal(srcQuadData, Int64(0)), &beforeEndLoop, &setValue);
        Bind(&setValue);
        {
            // srcMask <- 0xFFFFFFFFFFFFFFFF >> (64 - bitsInCurrentQuad);
            GateRef srcMask = Int64LSR(Int64(ALL_ONE_MASK), Int64Sub(Int64(BIT_PER_QUAD), bitsInCurrentQuad));
            // srcData <- (srcBitSet[srcQuad * 8] >> srcBitOffset) & srcMask;
            GateRef srcData = Int64And(Int64LSR(srcQuadData, *srcBitOffset), srcMask);
            // zeroMask <- ~(srcMask << dstBitOffset);
            GateRef zeroMask = Int64Not(Int64LSL(srcMask, *dstBitOffset));
            // dstData <- load from dstBitSet offset dstQuad * 8;
            GateRef dstData = LoadPrimitive(VariableType::INT64(), dstBitSet,
                Int64LSL(*dstQuad, Int64(BYTE_PER_QUAD_LOG2)));
            // newDataMask <- (srcData << dstBitOffset);
            GateRef newDataMask = Int64LSL(srcData, *dstBitOffset);
            // newData <- dstData & zeroMask | newDataMask;
            GateRef newData = Int64Or(Int64And(dstData, zeroMask), newDataMask);
            // bitSet[dstQuad * 8] <- newData;
            Store(VariableType::INT64(), glue_, dstBitSet, Int64LSL(*dstQuad, Int64(BYTE_PER_QUAD_LOG2)), newData);
            Jump(&beforeEndLoop);
        }
        Bind(&beforeEndLoop);
        {
            // remainLength <- remainLength - bitsInCurrentQuad;
            remainLength = Int64Sub(*remainLength, bitsInCurrentQuad);
            //  srcBitOffset <- srcBitOffset + bitsInCurrentQuad;
            srcBitOffset = Int64Add(*srcBitOffset, bitsInCurrentQuad);
            //  dstBitOffset <- dstBitOffset + bitsInCurrentQuad;
            dstBitOffset = Int64Add(*dstBitOffset, bitsInCurrentQuad);
            Label srcAdd(env);
            Label dstAdd(env);
            Label checkDst(env);
            BRANCH_NO_WEIGHT(Int64Equal(*srcBitOffset, Int64(BIT_PER_QUAD)), &srcAdd, &checkDst);
            Bind(&srcAdd);
            {
                //  if srcBitOffset == 64
                //  then srcQuad <- srcQuad + 1; srcBitOffset <- 0
                srcQuad = Int64Add(*srcQuad, Int64(1));
                srcBitOffset = Int64(0);
                Jump(&checkDst);
            }
            Bind(&checkDst);
            BRANCH_NO_WEIGHT(Int64Equal(*dstBitOffset, Int64(BIT_PER_QUAD)), &dstAdd, &endLoop);
            Bind(&dstAdd);
            {
                // if dstBitOffset == 64
                // then dstQuad <- dstQuad + 1; dstBitOffset <- 0}
                dstQuad = Int64Add(*dstQuad, Int64(1));
                dstBitOffset = Int64(0);
                Jump(&endLoop);
            }
        }
        Bind(&endLoop);
        LoopEnd(&begin);
    }
    Bind(&exit);
    env->SubCfgExit();
}

void BarrierStubBuilder::BitSetRangeMoveBackward(GateRef srcBitSet, GateRef dstBitSet, GateRef srcStart,
                                                 GateRef dstStart, GateRef length)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(remainLength, VariableType::INT64(), length);
    // srcEnd <- srcStart + remainLength - 1;
    // dstEnd <- dstStart + remainLength - 1;
    DEFVARIABLE(srcEnd, VariableType::INT64(), Int64Sub(Int64Add(srcStart, length), Int64(1)));
    DEFVARIABLE(dstEnd, VariableType::INT64(), Int64Sub(Int64Add(dstStart, length), Int64(1)));

    Label begin(env);
    Label body(env);
    Label endLoop(env);
    Jump(&begin);
    LoopBegin(&begin);
    //  while remainLength > 0
    BRANCH_LIKELY(Int64GreaterThan(*remainLength, Int64(0)), &body, &exit);
    Bind(&body);
    {
        Label beforeEndLoop(env);
        // srcBitOffset <- srcEnd % 64;
        GateRef srcBitOffset = Int64And(*srcEnd, Int64(BIT_PER_QUAD_MASK));
        // dstBitOffset <- dstEnd % 64;
        GateRef dstBitOffset = Int64And(*dstEnd, Int64(BIT_PER_QUAD_MASK));
        // bitsInCurrentQuad <- min of {srcBitOffset + 1, dstBitOffset + 1, remainLength};
        GateRef bitsInCurrentQuad = ThreeInt64Min(Int64Add(srcBitOffset, Int64(1)),
                                                  Int64Add(dstBitOffset, Int64(1)),
                                                  *remainLength);
        // srcQuadIndex <- srcEnd / 64;
        GateRef srcQuadIndex = Int64LSR(*srcEnd, Int64(BIT_PER_QUAD_LOG2));
        GateRef srcQuadData = LoadPrimitive(VariableType::INT64(), srcBitSet,
            Int64LSL(srcQuadIndex, Int64(BYTE_PER_QUAD_LOG2)));
        Label setValue(env);
        BRANCH_NO_WEIGHT(Int64Equal(srcQuadData, Int64(0)), &beforeEndLoop, &setValue);
        Bind(&setValue);
        {
            // mask <- 0xFFFFFFFFFFFFFFFF >> (64 - bitsInCurrentQuad);
            GateRef mask = Int64LSR(Int64(ALL_ONE_MASK), Int64Sub(Int64(BIT_PER_QUAD), bitsInCurrentQuad));
            //  srcData <- (srcBitSet[srcQuadIndex * 8] >> (srcBitOffset - bitsInCurrentQuad + 1)) & mask;
            GateRef srcData = Int64And(
                Int64LSR(srcQuadData, Int64Add(Int64Sub(srcBitOffset, bitsInCurrentQuad), Int64(1))), mask);
            // zeroMask <- ~(mask << (dstBitOffset - bitsInCurrentQuad + 1));
            GateRef zeroMask = Int64Not(Int64LSL(mask, Int64Add(Int64Sub(dstBitOffset, bitsInCurrentQuad), Int64(1))));
            // dstWordIndex <- dstEnd / 64;
            GateRef dstQuadIndex = Int64LSR(*dstEnd, Int64(BIT_PER_QUAD_LOG2));
            // dstData <- load from dstBitSet offset dstQuadIndex * 8
            GateRef dstData = LoadPrimitive(VariableType::INT64(), dstBitSet,
                Int64LSL(dstQuadIndex, Int64(BYTE_PER_QUAD_LOG2)));
            // newDataMask <- (srcData << (dstBitOffset - bitsInCurrentQuad + 1))
            GateRef newDataMask = Int64LSL(srcData, Int64Add(Int64Sub(dstBitOffset, bitsInCurrentQuad), Int64(1)));
            // newData <- dstData & zeroMask | newDataMask;
            GateRef newData = Int64Or(Int64And(dstData, zeroMask), newDataMask);
            // bitSet[dstQuadIndex * 8] <- newData;
            Store(VariableType::INT64(), glue_, dstBitSet, Int64LSL(dstQuadIndex, Int64(BYTE_PER_QUAD_LOG2)), newData);
            Jump(&beforeEndLoop);
        }
        Bind(&beforeEndLoop);
        {
            // remainLength <- remainLength - bitsInCurrentQuad;
            remainLength = Int64Sub(*remainLength, bitsInCurrentQuad);
            // srcEnd <- srcEnd - bitsInCurrentQuad;
            srcEnd = Int64Sub(*srcEnd, bitsInCurrentQuad);
            // dstEnd <- dstEnd - bitsInCurrentQuad;
            dstEnd = Int64Sub(*dstEnd, bitsInCurrentQuad);
            Jump(&endLoop);
        }
        Bind(&endLoop);
        LoopEnd(&begin);
    }
    Bind(&exit);
    env->SubCfgExit();
}

void BarrierStubBuilder::DoReverseBarrier()
{
    auto env = GetEnvironment();
    Label entry(env);
    Label exit(env);
    Label checkNext(env);
    Label needMarking(env);
    Label notMarkRSet(env);
    Label markInBuffer(env);
    Label continueProcessing(env);
    Label isTaggedObject(env);
    Label RefisTaggedObject(env);
    Label markRSet(env);
    Label continueLoopHead(env);
    Label continueLoopEnd(env);
    Label notMarkRSetLoopHead(env);
    Label notMarkRSetLoopEnd(env);
    Label iLessLength(env);
    Label indexLessLength(env);
    Label notIdlePhase(env);
    env->SubCfgEntry(&entry);

    BRANCH_UNLIKELY(LoadPrimitive(
        VariableType::BOOL(), glue_, IntPtr(JSThread::GlueData::GetIsEnableCMCGCOffset(env->Is32Bit()))),
        &needMarking, &checkNext);
    Bind(&needMarking);
    {
        GateRef gcPhase = GetGCPhase(glue_);
        GateRef objRegionType = GetCMCRegionType(dstObj_);
        GateRef shouldProcessSATB = ShouldProcessSATB(gcPhase);
        BRANCH(Int8Equal(gcPhase, Int8(common::GCPhase::GC_PHASE_IDLE)), &exit, &notIdlePhase);
        Bind(&notIdlePhase);
        {
            BRANCH(CMCIsInYoungSpace(objRegionType), &notMarkRSet, &continueProcessing);
            Bind(&continueProcessing);
            {
                DEFVARIABLE(i, VariableType::INT32(), Int32(0));
                Label checkOldToYoung(env);
                BRANCH(ShouldUpdateRememberSet(glue_, gcPhase), &checkOldToYoung, &notMarkRSet);
                Bind(&checkOldToYoung);
                Jump(&continueLoopHead);
                LoopBegin(&continueLoopHead);
                {
                    BRANCH(Int32UnsignedLessThan(*i, slotCount_), &iLessLength, &notMarkRSet);
                    Bind(&iLessLength);
                    {
                        GateRef offset = PtrMul(ZExtInt32ToPtr(*i), IntPtr(JSTaggedValue::TaggedTypeSize()));
                        GateRef ref = LoadPrimitive(VariableType::JS_ANY(), dstAddr_, offset);
                        BRANCH(TaggedIsHeapObject(ref), &isTaggedObject, &continueLoopEnd);
                    }
                    Bind(&isTaggedObject);
                    MarkRSetCardTable(dstObj_, &notMarkRSet);
                    Bind(&continueLoopEnd);
                    i = Int32Add(*i, Int32(1));
                    LoopEnd(&continueLoopHead);
                }
            }
            Bind(&notMarkRSet);
            {
                DEFVARIABLE(index, VariableType::INT32(), Int32(0));
                GateRef shouldProcessSATB = ShouldProcessSATB(gcPhase);
                Jump(&notMarkRSetLoopHead);
                LoopBegin(&notMarkRSetLoopHead);
                {
                    BRANCH_LIKELY(Int32UnsignedLessThan(*index, slotCount_), &indexLessLength, &exit);
                    Bind(&indexLessLength);
                    GateRef offset = PtrMul(ZExtInt32ToPtr(*index), IntPtr(JSTaggedValue::TaggedTypeSize()));
                    GateRef ref = LoadPrimitive(VariableType::JS_ANY(), dstAddr_, offset);
                    BRANCH(TaggedIsHeapObject(ref), &RefisTaggedObject, &notMarkRSetLoopEnd);
                    Bind(&RefisTaggedObject);
                    BRANCH_UNLIKELY(shouldProcessSATB, &markInBuffer, &exit);
                    Bind(&markInBuffer);
                    {
                        ASSERT(RuntimeStubCSigns::Get(RTSTUB_ID(MarkInBuffer))->IsNoTailCall());
                        CallNGCRuntime(glue_, RTSTUB_ID(MarkInBuffer), {glue_, ref});
                        Jump(&notMarkRSetLoopEnd);
                    }
                    
                    Bind(&notMarkRSetLoopEnd);
                    index = Int32Add(*index, Int32(1));
                    LoopEnd(&notMarkRSetLoopHead);
                }
            }
        }
    }
    Bind(&checkNext);
    {
        Label handleMark(env);
        Label handleBitSet(env);
        Label doReverse(env);
        BRANCH_NO_WEIGHT(Int32Equal(slotCount_, Int32(0)), &exit, &doReverse);
        Bind(&doReverse);
        BRANCH_NO_WEIGHT(InSharedHeap(objectRegion_), &handleMark, &handleBitSet);
        Bind(&handleBitSet);
        {
            DoReverseBarrierInternal();
            Jump(&handleMark);
        }
        Bind(&handleMark);
        HandleMark();
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}


void BarrierStubBuilder::DoReverseBarrierInternal()
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label inYoung(env);
    Label inOld(env);
    Label copyBitSet(env);

    DEFVARIABLE(bitSetAddr, VariableType::NATIVE_POINTER(), IntPtr(0));
    DEFVARIABLE(bitSetAddr2, VariableType::NATIVE_POINTER(), IntPtr(0));

    GateRef localToShareOffset = IntPtr(Region::PackedData::GetLocalToShareSetOffset(env->Is32Bit()));
    GateRef localToShareBitSetAddr =
        GetBitSetDataAddr(objectRegion_, localToShareOffset, RTSTUB_ID(CreateLocalToShare));

    GateRef localToShareSwapped = IsLocalToShareSwapped(objectRegion_);
    BRANCH_NO_WEIGHT(InYoungGeneration(objectRegion_), &inYoung, &inOld);
    Bind(&inYoung);
    {
        Label batchBarrier(env);
        Label reverseLocalToShare(env);
        BRANCH_UNLIKELY(localToShareSwapped, &batchBarrier, &reverseLocalToShare);
        Bind(&batchBarrier);
        {
            // slowpath, localToShareRSet is swapped, it can't be reversed, just set the bitset bit by bit.
            BarrierBatchBitSet(LocalToShared);
            Jump(&exit);
        }
        Bind(&reverseLocalToShare);
        {
            bitSetAddr = localToShareBitSetAddr;
            Jump(&copyBitSet);
        }
    }
    Bind(&inOld);
    {
        GateRef oldToNewOffset = IntPtr(Region::PackedData::GetOldToNewSetOffset(env->Is32Bit()));
        GateRef oldToNewBitSetAddr = GetBitSetDataAddr(objectRegion_, oldToNewOffset, RTSTUB_ID(CreateOldToNew));
        // CreateOldToNew may change the RSetFlag, so load it again.
        GateRef oldToNewSwapped = IsOldToNewSwapped(objectRegion_);
        Label batchBarrier(env);
        Label tryBatchBarrier(env);
        Label copyBoth(env);
        // both localToShareSwapped and oldToNewSwapped are not swapped.
        BRANCH_LIKELY(BitAnd(BoolNot(localToShareSwapped), BoolNot(oldToNewSwapped)), &copyBoth, &tryBatchBarrier);
        Bind(&copyBoth);
        {
            bitSetAddr = localToShareBitSetAddr;
            bitSetAddr2 = oldToNewBitSetAddr;
            Jump(&copyBitSet);
        }
        Bind(&tryBatchBarrier);
        {
            Label reverseOne(env);
            BRANCH_UNLIKELY(BitAnd(localToShareSwapped, oldToNewSwapped), &batchBarrier, &reverseOne);
            Bind(&batchBarrier);
            {
                // slowpath, localToShareRSet and oldToNewRSet are swapped,
                // it can't be reversed, just set the bitset bit by bit.
                BarrierBatchBitSet(LocalToShared | OldToNew);
                Jump(&exit);
            }
            Bind(&reverseOne);
            {
                Label localToShareBatchBarrier(env);
                Label oldToNewBatchBarrier(env);
                BRANCH_NO_WEIGHT(localToShareSwapped, &localToShareBatchBarrier, &oldToNewBatchBarrier);
                Bind(&localToShareBatchBarrier);
                {
                    // slowpath, localToShareRSet is swapped, it can't be reversed, just set the bitset bit by bit.
                    // And copy oldToNewRSet.
                    BarrierBatchBitSet(LocalToShared);
                    bitSetAddr = oldToNewBitSetAddr;
                    Jump(&copyBitSet);
                }
                Bind(&oldToNewBatchBarrier);
                {
                    // slowpath, oldToNewRSet is swapped, it can't be reversed, just set the bitset bit by bit.
                    // And copy localToShareRSet.
                    BarrierBatchBitSet(OldToNew);
                    bitSetAddr = localToShareBitSetAddr;
                    Jump(&copyBitSet);
                }
            }
        }
    }
    Bind(&copyBitSet);
    {
        GateRef dstBitStartIdx = Int64LSR(Int64Sub(dstAddr_, objectRegion_), Int64(TAGGED_TYPE_SIZE_LOG));
        Label begin(env);
        Label endLoop(env);
        Label next(env);
        Jump(&begin);
        LoopBegin(&begin);
        {
            BitSetRangeReverse(*bitSetAddr, dstBitStartIdx, ZExtInt32ToInt64(slotCount_));
            BRANCH_UNLIKELY(IntPtrEqual(*bitSetAddr2, IntPtr(0)), &exit, &next);
            Bind(&next);
            {
                bitSetAddr = *bitSetAddr2;
                bitSetAddr2 = IntPtr(0);
                Jump(&endLoop);
            }
        }
        Bind(&endLoop);
        LoopEnd(&begin);
    }
    Bind(&exit);
    env->SubCfgExit();
}

void BarrierStubBuilder::BitSetRangeReverse(GateRef bitSet, GateRef startIdx, GateRef length)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    // startOf <- startIdx % 64
    GateRef startOf = Int64And(startIdx, Int64(BIT_PER_QUAD_MASK));
    // endOf <- (startIdx + len) % 64
    GateRef endOf = Int64And(Int64Add(startIdx, length), Int64(BIT_PER_QUAD_MASK));

    // startItemIndex <- startIdx / 64
    GateRef startItemIndex = Int64LSR(startIdx, Int64(BIT_PER_QUAD_LOG2));
    // endItemIndex <- (startIdx + len - 1) / 64
    GateRef endItemIndex = Int64LSR(Int64Sub(Int64Add(startIdx, length), Int64(1)), Int64(BIT_PER_QUAD_LOG2));

    GateRef dstOffset = startIdx;
    // srcOffset <- startIdx + ((64 - endOf) % 64 - startOf)
    GateRef srcOffset = Int64Add(startIdx,
        Int64Sub(Int64And(Int64Sub(Int64(BIT_PER_QUAD), endOf), Int64(BIT_PER_QUAD_MASK)), startOf));

    // startByteIndex <- startItemIndex * 8
    GateRef startByteIndex = Int64LSL(startItemIndex, Int64(BIT_PER_BYTE_LOG2));
    // endByteIndex <- endItemIndex * 8
    GateRef endByteIndex = Int64LSL(endItemIndex, Int64(BIT_PER_BYTE_LOG2));

    GateRef startItem = LoadPrimitive(VariableType::INT64(), bitSet, startByteIndex);
    GateRef endItem = LoadPrimitive(VariableType::INT64(), bitSet, endByteIndex);

    // startMask <- (1 << startOf) - 1
    GateRef startMask = Int64Sub(Int64LSL(Int64(1), startOf), Int64(1));
    Label notZero(env);
    Label next(env);
    DEFVARIABLE(endMask, VariableType::INT64(), Int64(0));
    BRANCH_UNLIKELY(Int64Equal(endOf, Int64(0)), &next, &notZero);
    Bind(&notZero);
    {
        // endMask <- ~((1 << endOf) - 1)
        endMask = Int64Not(Int64Sub(Int64LSL(Int64(1), endOf), Int64(1)));
        Jump(&next);
    }

    Bind(&next);
    DEFVARIABLE(vstartByteIndex, VariableType::INT64(), startByteIndex);
    DEFVARIABLE(vendByteIndex, VariableType::INT64(), endByteIndex);

    Label loopBegin1(env);
    Label loopEnd1(env);
    Label loopExit1(env);
    Label startReverse(env);
    Jump(&loopBegin1);
    LoopBegin(&loopBegin1);
    {
        BRANCH_LIKELY(Int64LessThanOrEqual(*vstartByteIndex, *vendByteIndex), &startReverse, &loopExit1);
        Bind(&startReverse);
        {
            GateRef item1 = LoadPrimitive(VariableType::INT64(), bitSet, *vstartByteIndex);
            GateRef item2 = LoadPrimitive(VariableType::INT64(), bitSet, *vendByteIndex);

            // revStart <- Int64BitReverse(bitSet[vstartByteIndex])
            // revEnd <- Int64BitReverse(bitSet[vendByteIndex])
            GateRef revStart = Int64BitReverse(item1);
            GateRef revEnd = Int64BitReverse(item2);

            // bitSet[vstartByteIndex] <- revEnd
            // bitSet[vendByteIndex] <- revStart
            Store(VariableType::INT64(), glue_, bitSet, *vstartByteIndex, revEnd, MemoryAttribute::NoBarrier());
            Store(VariableType::INT64(), glue_, bitSet, *vendByteIndex, revStart, MemoryAttribute::NoBarrier());

            // vstartByteIndex <- vstartByteIndex + 8
            // vendByteIndex <- vendByteIndex - 8
            vstartByteIndex = Int64Add(*vstartByteIndex, Int64(BYTE_PER_QUAD));
            vendByteIndex = Int64Sub(*vendByteIndex, Int64(BYTE_PER_QUAD));
            Jump(&loopEnd1);
        }
    }
    Bind(&loopEnd1);
    LoopEnd(&loopBegin1);
    Bind(&loopExit1);
    BitSetRangeMove(bitSet, bitSet, srcOffset, dstOffset, length);

    GateRef neStartMask = Int64Not(startMask);
    GateRef neEndMask = Int64Not(*endMask);

    // bitSet[startByteIndex] <- (startItem & startMask) | (startItem1 & neStartMask);
    GateRef startItem1 = LoadPrimitive(VariableType::INT64(), bitSet, startByteIndex);
    GateRef resStartItem = Int64Or(Int64And(startItem, startMask), Int64And(startItem1, neStartMask));
    Store(VariableType::INT64(), glue_, bitSet, startByteIndex, resStartItem, MemoryAttribute::NoBarrier());

    // bitSet[endByteIndex] <- (resEndItem & endMask) | (endItem1 & neEndMask);
    GateRef endItem1 = LoadPrimitive(VariableType::INT64(), bitSet, endByteIndex);
    GateRef resEndItem = Int64Or(Int64And(endItem, *endMask), Int64And(endItem1, neEndMask));
    Store(VariableType::INT64(), glue_, bitSet, endByteIndex, resEndItem, MemoryAttribute::NoBarrier());
    Jump(&exit);
    Bind(&exit);
    env->SubCfgExit();
}

GateRef BarrierStubBuilder::IsLocalToShareSwapped(GateRef region)
{
    auto env = GetEnvironment();
    GateRef RSetSwapFlagOffset = IntPtr(Region::PackedData::GetRSetSwapFlagOffset(env->Is32Bit()));
    GateRef RSetFlag = LoadPrimitive(VariableType::INT8(), region, RSetSwapFlagOffset);
    return NotEqual(Int8And(RSetFlag, Int8(LOCAL_TO_SHARE_SWAPPED_MASK)), Int8(0));
}

GateRef BarrierStubBuilder::IsOldToNewSwapped(GateRef region)
{
    auto env = GetEnvironment();
    GateRef RSetSwapFlagOffset = IntPtr(Region::PackedData::GetRSetSwapFlagOffset(env->Is32Bit()));
    GateRef RSetFlag = LoadPrimitive(VariableType::INT8(), region, RSetSwapFlagOffset);
    return NotEqual(Int8And(RSetFlag, Int8(OLD_TO_NEW_SWAPPED_MASK)), Int8(0));
}
}
