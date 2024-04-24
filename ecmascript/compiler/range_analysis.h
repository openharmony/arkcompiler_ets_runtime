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

#ifndef ECMASCRIPT_COMPILER_RANGE_ANALYSIS_H
#define ECMASCRIPT_COMPILER_RANGE_ANALYSIS_H

#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/combined_pass_visitor.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/mcr_gate_meta_data.h"
#include "ecmascript/compiler/number_gate_info.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {
class RangeAnalysis : public PassVisitor {
public:
    RangeAnalysis(Circuit* circuit, RPOVisitor* visitor, Chunk* chunk, ChunkVector<TypeInfo>& typeInfos,
                  ChunkVector<RangeInfo>& rangeInfos)
        : PassVisitor(circuit, chunk, visitor), acc_(circuit), builder_(circuit),
          typeInfos_(typeInfos), rangeInfos_(rangeInfos) {}
    GateRef VisitGate(GateRef gate);
    void PrintRangeInfo() const;

private:
    GateRef VisitPhi(GateRef gate);
    GateRef VisitTypedBinaryOp(GateRef gate);
    GateRef VisitTypedUnaryOp(GateRef gate);
    GateRef VisitConstant(GateRef gate);
    GateRef VisitOthers(GateRef gate);
    GateRef VisitIndexCheck(GateRef gate);
    GateRef VisitLoadArrayLength(GateRef gate);
    GateRef VisitLoadStringLength(GateRef gate);
    GateRef VisitLoadMapSize(GateRef gate);
    GateRef VisitLoadTypedArrayLength(GateRef gate);
    GateRef VisitRangeGuard(GateRef gate);
    template<TypedBinOp Op>
    RangeInfo GetRangeOfCalculate(GateRef gate);
    template<TypedBinOp Op>
    RangeInfo GetRangeOfShift(GateRef gate);
    RangeInfo TryGetRangeOfBranch(GateRef state, GateRef value);
    RangeInfo GetRangeOfCompare(GateRef gate, GateRef value, bool flag);
    GateRef UpdateRange(GateRef gate, const RangeInfo& info);
    RangeInfo GetRange(GateRef gate) const;
    bool IsInt32Type(GateRef gate) const;
    GateAccessor acc_;
    CircuitBuilder builder_;
    ChunkVector<TypeInfo>& typeInfos_;
    ChunkVector<RangeInfo>& rangeInfos_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_RANGE_ANALYSIS_H
