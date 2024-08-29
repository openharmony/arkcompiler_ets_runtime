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

#ifndef ECMASCRIPT_COMPILER_FRAME_STATE_H
#define ECMASCRIPT_COMPILER_FRAME_STATE_H

#include "ecmascript/compiler/base/bit_set.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/pgo_type/pgo_type_recorder.h"
#include "ecmascript/jspandafile/method_literal.h"

namespace panda::ecmascript::kungfu {
class BytecodeCircuitBuilder;
struct BytecodeRegion;

class FrameLiveOut {
public:
    explicit FrameLiveOut(Chunk* chunk, size_t numVregs)
        : liveout_(chunk, numVregs), defRegisters_(chunk, numVregs) {}

    void SetBit(size_t index)
    {
        liveout_.SetBit(index);
    }

    void ClearBit(size_t index)
    {
        liveout_.ClearBit(index);
    }

    bool TestBit(size_t index) const
    {
        return liveout_.TestBit(index);
    }

    void CopyFrom(FrameLiveOut *other)
    {
        liveout_.CopyFrom(other->liveout_);
    }

    bool MergeLiveout(FrameLiveOut *other)
    {
        return liveout_.UnionWithChanged(other->liveout_);
    }

    void Reset()
    {
        return liveout_.Reset();
    }

private:
    // [numVRegs_] [extra args] [numArgs_] [accumulator]
    BitSet liveout_;
    BitSet defRegisters_;
    friend class BlockLoopAnalysis;
    friend class FrameStateBuilder;
};

class FrameContext {
public:
    explicit FrameContext(Chunk* chunk, size_t numVregs)
        : values_(numVregs, chunk) {}

    GateRef ValuesAt(size_t index) const
    {
        ASSERT(index < values_.size());
        return values_[index];
    }

    void SetValuesAt(size_t index, GateRef gate)
    {
        ASSERT(index < values_.size());
        values_[index] = gate;
    }

    void CopyCurrentStatus(FrameContext *other)
    {
        currentState_ = other->currentState_;
        currentDepend_ = other->currentDepend_;
        needStateSplit_ = other->needStateSplit_;
    }
private:
    // [numVRegs_] [extra args] [numArgs_] [accumulator]
    ChunkVector<GateRef> values_;
    GateRef currentState_ {Circuit::NullGate()};
    GateRef currentDepend_ {Circuit::NullGate()};
    GateRef loopBackState_ {Circuit::NullGate()};
    GateRef loopBackDepend_ {Circuit::NullGate()};
    GateRef mergeState_ {Circuit::NullGate()};
    GateRef mergeDepend_ {Circuit::NullGate()};
    size_t currentIndex_ {0};
    size_t loopBackIndex_ {0};
    size_t mergeIndex_ {0};
    bool needStateSplit_ {false};
    friend class FrameStateBuilder;
    friend class BlockLoopAnalysis;
};

struct MergeStateDependInfo {
    GateRef state;
    GateRef depend;
    size_t index;
};

class FrameStateBuilder {
public:
    FrameStateBuilder(BytecodeCircuitBuilder *builder,
        Circuit *circuit, const MethodLiteral *literal);
    ~FrameStateBuilder();

    void DoBytecodeAnalysis();
    void AdvanceToNextBc(const BytecodeInfo &bytecodeInfo, FrameLiveOut* liveout, uint32_t bcId);
    void UpdateFrameValues(const BytecodeInfo &bytecodeInfo, uint32_t bcId,
        GateRef gate);
    void UpdateMoveValues(const BytecodeInfo &bytecodeInfo);
    void UpdateStateDepend(GateRef state, GateRef depend);
    FrameLiveOut *GetOrOCreateBCEndLiveOut(uint32_t bcIndex);
    FrameLiveOut *GetOrOCreateBBLiveOut(size_t bbIndex);
    GateRef GetCurrentState() const
    {
        return liveContext_->currentState_;
    }

    GateRef GetCurrentDepend() const
    {
        return liveContext_->currentDepend_;
    }
    void MergeIntoSuccessor(const BytecodeRegion &bb, const BytecodeRegion &bbNext);
    void AdvanceToNextBB(const BytecodeRegion &bb, bool isOsrLoopExit = false);
    void InitEntryBB(const BytecodeRegion &bb);

    ChunkDeque<size_t>& GetRpoList()
    {
        return rpoList_;
    }

    bool HasLoop() const
    {
        return numLoops_ > 0;
    }

    void UpdateAccumulator(GateRef gate)
    {
        UpdateVirtualRegister(accumulatorIndex_, gate);
    }

    void SetOsrLoopHeadBB(const BytecodeRegion &loopHeadOfOSR);

    bool IsOsrLoopExit(const BytecodeRegion &curBB);

    bool OutOfOsrLoop(const BytecodeRegion &curBB);

    size_t GetOsrLoopHeadBBId() const;

    bool IsContextExists(uint32_t bbIndex) const
    {
        ASSERT(bbIndex < bbFrameContext_.size());
        return bbFrameContext_[bbIndex] != nullptr;
    }

    FrameContext *GetOrOCreateMergedContext(uint32_t bbIndex);

private:
    static constexpr size_t FIXED_ARGS = 2; // ac & env
    struct LoopInfo {
        size_t loopIndex {0};
        size_t loopHeadId {0};
        size_t numLoopBacks {0};
        size_t sortIndx {0};
        LoopInfo* parentInfo {nullptr};
        BitSet* loopBodys {nullptr};
        ChunkVector<BytecodeRegion*>* loopExits {nullptr};
        BitSet* loopAssignment {nullptr};
        FrameContext* mergedContext {nullptr};
    };

    void BuildPostOrderList(size_t size);
    bool ComputeLiveOut(size_t bbId);
    void ComputeLiveState();
    void ComputeLiveOutBC(const BytecodeInfo &bytecodeInfo);
    bool MergeIntoPredBC(uint32_t predPc);
    bool MergeFromSuccBB(size_t bbId);
    void MergeFromCatchBB(size_t bbId);

    FrameLiveOut *GetFrameLiveoutAfter(uint32_t bcId)
    {
        return bcEndStateLiveouts_[bcId];
    }

    FrameLiveOut *GetFrameLiveoutBefore(size_t bbId)
    {
        return bbBeginStateLiveouts_[bbId];
    }

    GateRef ValuesAt(size_t index) const
    {
        return liveContext_->ValuesAt(index);
    }

    GateRef ValuesAtAccumulator() const
    {
        return ValuesAt(accumulatorIndex_);
    }

    void UpdateVirtualRegister(size_t index, GateRef gate)
    {
        liveContext_->SetValuesAt(index, gate);
    }

    GateRef GetBcFrameStateCache()
    {
        ASSERT(frameStateCache_ != Circuit::NullGate());
        auto cache = frameStateCache_;
        frameStateCache_ = Circuit::NullGate();
        return cache;
    }

    FrameContext *GetMergedBbContext(uint32_t bbIndex) const
    {
        ASSERT(bbIndex < bbFrameContext_.size());
        return bbFrameContext_[bbIndex];
    }

    void FillBcInputs(const BytecodeInfo &bytecodeInfo, GateRef gate);
    void DumpLiveState();
    size_t GetNumOfStatePreds(const BytecodeRegion &bb);
    GateRef MergeValue(const BytecodeRegion &bb,
        GateRef stateMerge, GateRef currentValue, GateRef nextValue, bool isLoopBack);
    void NewMerge(const BytecodeRegion &bbNext);
    void MergeStateDepend(const BytecodeRegion &bb, const BytecodeRegion &bbNext);
    void CopyLiveoutValues(const BytecodeRegion &bbNext, FrameContext* dest, FrameContext* src);
    void SaveCurrentContext(const BytecodeRegion &bb);
    MergeStateDependInfo GetCorrespondingState(const BytecodeRegion &bb, const BytecodeRegion &bbNext);

    void NewLoopExit(const BytecodeRegion &bbNext, BitSet *loopAssignment);
    size_t ComputeLoopDepth(size_t loopHead);
    void TryInsertLoopExit(const BytecodeRegion &bb, const BytecodeRegion &bbNext);
    void ComputeLoopInfo();
    void ResizeLoopBody();
    void MergeAssignment(const BytecodeRegion &bb, const BytecodeRegion &bbNext);
    BitSet *GetLoopAssignment(const BytecodeRegion &bb);
    LoopInfo& GetLoopInfo(const BytecodeRegion &bb);
    LoopInfo& GetLoopInfo(BytecodeRegion &bb);
    LoopInfo* GetLoopInfoByLoopBody(const BytecodeRegion &bb);
    bool IsLoopBackEdge(const BytecodeRegion &bb, const BytecodeRegion &bbNext);
    bool IsLoopHead(const BytecodeRegion &bb);
    bool IfLoopNeedMerge(const BytecodeRegion &bb) const;
    GateRef InitMerge(size_t numOfIns, bool isLoop);
    bool IsGateNotEmpty(GateRef gate) const;

    GateRef BuildFrameContext(FrameContext* frameContext);
    void BindStateSplitBefore(const BytecodeInfo &bytecodeInfo, FrameLiveOut* liveout, uint32_t bcId);
    void BindStateSplitAfter(const BytecodeInfo &bytecodeInfo, uint32_t bcId, GateRef gate);
    GateRef BuildFrameValues(FrameContext* frameContext, FrameLiveOut* liveout);
    GateRef BuildStateSplit(FrameContext* frameContext, FrameLiveOut* liveout, size_t bcIndex);
    GateRef BuildFrameState(FrameContext* frameContext, FrameLiveOut* liveout, size_t bcIndex);
    void AddEmptyBlock(BytecodeRegion* bb);
    FrameContext *GetCachedContext();

    BytecodeCircuitBuilder *bcBuilder_ {nullptr};
    const PGOTypeRecorder *pgoTypeRecorder_ {nullptr};
    FrameLiveOut *liveOutResult_ {nullptr};
    FrameLiveOut *currentBBliveOut_ {nullptr};
    FrameContext *liveContext_ {nullptr};
    FrameContext *cachedContext_ {nullptr};
    FrameContext *cachedContextBackup_ {nullptr};
    size_t numVregs_ {0};
    size_t accumulatorIndex_ {0};
    size_t envIndex_ {0};
    size_t numLoops_ {0};
    size_t sortIndx_ {0};
    GateRef frameStateCache_ {Circuit::NullGate()};
    Circuit *circuit_ {nullptr};
    GateAccessor acc_;
    ChunkVector<FrameLiveOut *> bcEndStateLiveouts_;
    ChunkVector<FrameLiveOut *> bbBeginStateLiveouts_;
    ChunkVector<FrameContext *> bbFrameContext_;
    ChunkVector<LoopInfo> loops_;
    ChunkDeque<size_t> rpoList_;
    ChunkVector<size_t> postOrderList_;
    const BytecodeRegion *loopHeadOfOSR_ {nullptr};

    friend class BlockLoopAnalysis;
    friend class SubContextScope;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_FRAME_STATE_H
