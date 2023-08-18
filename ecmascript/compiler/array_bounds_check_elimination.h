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
#include "ecmascript/compiler/gate_meta_data.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/graph_linearizer.h"
#include "ecmascript/compiler/pass_manager.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {
class ArrayBoundsCheckElimination {
public:
    ArrayBoundsCheckElimination(Circuit *circuit, bool enableLog, const std::string& name, Chunk* chunk)
        : acc_(circuit), bounds_(chunk), circuit_(circuit), chunk_(chunk), enableLog_(enableLog),
        graphLinearizer_(circuit, enableLog, name, chunk, true), methodName_(name) {}

    ~ArrayBoundsCheckElimination() = default;
    void Run();

private:
    class Bound {
    public:
        Bound();
        Bound(GateRef v);
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
        bool IsSmaller(Bound *b)
        {
            if (b->LowerGate() != upperGate_) {
                return false;
            }
            return upper_ < b->Lower();
        }
        
    private:
        int upper_;
        GateRef upperGate_;
        int lower_;
        GateRef lowerGate_;

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
    typedef ChunkVector<GateRef> IndexCheckList;
    typedef ChunkVector<GateRef> GateLists;
    typedef ChunkVector<int> IntegerStack;

    void AddIfCondition(IntegerStack &pushed, GateRef x, GateRef y, TypedBinOp op);
    Bound *AndOp(Bound *bound, Bound *b);
    bool Contain(ChunkVector<GateRef>& gateLists, GateRef gate);
    void CalcBounds(GateRegion *block);
    void UpdateBound(IntegerStack &pushed, GateRef gate, Bound *bound);
    void UpdateBound(IntegerStack &pushed, GateRef x, TypedBinOp op, GateRef y, int constValue);
    void ProcessIndexCheck(GateRef gate);
    void RemoveIndexCheck(GateRef gate);
    Bound *GetBound(GateRef gate);
    Bound *DoConstant(GateRef gate);
    Bound *DoArithmeticOp(GateRef gate);
    void SetBound(GateRef gate, Bound *bound);
    void ProcessIf(IntegerStack &pushed, GateRegion *parent, OpCode cond);
    bool InArrayBound(Bound *bound, GateRef Length, GateRef Array);
    Bound *VisitGate(GateRef gate);

    GateAccessor acc_;
    BoundMap bounds_;
    Circuit *circuit_ {nullptr};
    Chunk *chunk_ {nullptr};
    bool enableLog_ {false};
    GraphLinearizer graphLinearizer_;
    std::string methodName_;
};
}
#endif