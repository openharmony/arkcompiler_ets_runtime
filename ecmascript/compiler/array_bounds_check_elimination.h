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

#ifndef ECMASCRIPT_COMPILER_ARRAY_BOUNDS_CHECK_ELIMINATION_H
#define ECMASCRIPT_COMPILER_ARRAY_BOUNDS_CHECK_ELIMINATION_H

#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/mcr_gate_meta_data.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/graph_linearizer.h"
#include "ecmascript/compiler/pass_manager.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {
class ArrayBoundsCheckElimination {
public:
    ArrayBoundsCheckElimination(Circuit *circuit, bool enableLog, const std::string& name, Chunk* chunk)
        : acc_(circuit), bounds_(chunk), circuit_(circuit), builder_(circuit), chunk_(chunk), enableLog_(enableLog),
        graphLinearizer_(circuit, enableLog, name, chunk, true, true), methodName_(name), indexCheckInfo_(chunk) {}

    ~ArrayBoundsCheckElimination() = default;
    void Run();

private:
    class Bound {
    public:
        Bound();
        explicit Bound(GateRef v);
        Bound(int lower, GateRef lowerGate, int upper, GateRef upperGate);
        Bound(TypedBinOp op, GateRef gate, int constant);
        ~Bound(){};

        int Upper()
        {
            return upper_;
        }

        GateRef UpperGate()
        {
            return upperGate_;
        }

        int Lower()
        {
            return lower_;
        }

        GateRef LowerGate()
        {
            return lowerGate_;
        }

        bool HasUpper()
        {
            return upperGate_ != Circuit::NullGate() || upper_ < INT_MAX;
        }

        bool HasLower()
        {
            return lowerGate_ != Circuit::NullGate() || lower_ > INT_MIN;
        }

        void RemoveUpper()
        {
            upperGate_ = Circuit::NullGate();
            upper_ = INT_MAX;
        }

        void RemoveLower()
        {
            lowerGate_ = Circuit::NullGate();
            lower_ = INT_MIN;
        }

        bool IsSmaller(Bound *b)
        {
            if (b->LowerGate() != upperGate_) {
                return false;
            }
            return upper_ < b->Lower();
        }

        Bound* Copy(Chunk *chunk)
        {
            return chunk->New<Bound>(lower_, lowerGate_, upper_, upperGate_);
        }

    private:
        int upper_ = 0;
        GateRef upperGate_ {Circuit::NullGate()};
        int lower_ = 0;
        GateRef lowerGate_ {Circuit::NullGate()};

        friend ArrayBoundsCheckElimination;
    };

    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    const std::string& GetMethodName() const
    {
        return methodName_;
    }

    typedef ChunkVector<Bound*> BoundStack;
    typedef ChunkVector<BoundStack*> BoundMap;
    typedef ChunkVector<int> IntegerStack;
    typedef ChunkVector<GateRef> GateLists;

    void AddAccessIndexedInfo(GateLists &indices, GateRef gate, int idx, GateRef indexCheck);
    void AddIfCondition(IntegerStack &pushed, GateRef x, GateRef y, TypedBinOp op);
    Bound *AndOp(Bound *bound, Bound *b);
    Bound *OrOp(Bound *bound, Bound *b);
    bool Contain(GateLists &gateLists, GateRef gate);
    void CalcBounds(GateRegion *block, GateRegion *loopHeader);
    bool CheckLoop(GateRef array, GateRef lowerGate, int lower, GateRef upperGate, int upper);
    GateRef FindBoundGate(GateRef gate);
    void InBlockMotion(GateLists &indexChecked, GateLists &arrays);
    bool InLoop(GateRef loopHeader, GateRef gate);
    bool IsArrayLength(GateRef gate);
    bool LoopInvariant(GateRegion *loopHeader, GateRef gate);
    void UpdateBound(IntegerStack &pushed, GateRef gate, Bound *bound);
    void UpdateBound(IntegerStack &pushed, GateRef x, TypedBinOp op, GateRef y, int constValue);
    void ProcessIndexCheck(GateRegion *loopHeader, GateRef gate);
    void RemoveIndexCheck(GateRef gate);
    void CopyStateInAndDependIn(GateRef &stateIn, GateRef &dependIn, GateRef insertAfter);
    void LoopInvariantMotionForIndexCheck(GateRef array, GateRef length, GateRef lengthMetaData,
        GateRef lowerGate, int lower, GateRef upperGate, int upper, bool isTypedArray);
    bool GetInstrAndConstValueFromUnaryOp(GateRef gate, GateRef &other, int& value);
    bool GetInstrAndConstValueFromBinaryOp(GateRef gate, GateRef &other, int& value);
    void GetInstrAndConstValueFromOp(GateRef gate, GateRef &instrValue, int& constValue);
    Bound *GetBound(GateRef gate);
    Bound *DoConstant(GateRef gate);
    Bound *DoBinaryArithmeticOp(GateRef gate);
    Bound *DoUnaryArithmeticOp(GateRef gate);
    Bound *DoPhi(GateRef gate);
    void SetBound(GateRef gate, Bound *bound);
    void ProcessIf(IntegerStack &pushed, GateRegion *parent, OpCode cond);
    bool InArrayBound(Bound *bound, GateRef length, GateRef array);
    Bound *VisitGate(GateRef gate);

    void ReplaceIn(GateRef stateIn, GateRef dependIn, GateRef newGate);

    GateRef Predicate(GateRef left, TypedBinOp cond, GateRef right);
    GateRef PredicateCmpWithConst(GateRef left, TypedBinOp cond, int right);
    GateRef PredicateAdd(GateRef left, int leftConst, TypedBinOp cond, GateRef right);
    GateRef PredicateAddCmpWithConst(GateRef left, int leftConst, TypedBinOp cond, int right);

    GateAccessor acc_;
    BoundMap bounds_;
    Circuit *circuit_ {nullptr};
    CircuitBuilder builder_;
    Chunk *chunk_ {nullptr};
    bool enableLog_ {false};
    GraphLinearizer graphLinearizer_;
    std::string methodName_;

    class IndexCheckInfo {
    public:
        IndexCheckInfo(Chunk* chunk): list_(chunk) {}
        GateLists list_;
        int min_ {0};
        int max_ {0};
    };
    typedef ChunkVector<IndexCheckInfo*> IndexCheckInfoList;
    IndexCheckInfoList indexCheckInfo_;
};
}
#endif