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

#ifndef ECMASCRIPT_COMPILER_NUMBER_SPECULATIVE_LOWERING_H
#define ECMASCRIPT_COMPILER_NUMBER_SPECULATIVE_LOWERING_H

#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/compiler/number_gate_info.h"
#include "ecmascript/compiler/type.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {
class NumberSpeculativeLowering {
public:
    NumberSpeculativeLowering(Circuit* circuit, CompilationConfig* cmpCfg, Chunk* chunk,
        ChunkVector<TypeInfo>& typeInfos, ChunkVector<RangeInfo>& rangeInfos)
        : circuit_(circuit), acc_(circuit), builder_(circuit, cmpCfg), typeInfos_(typeInfos),
          rangeInfos_(rangeInfos), rangeGuardGates_(chunk), glue_(acc_.GetGlueFromArgList()) {}
    void Run();

private:
    void VisitGate(GateRef gate);
    void VisitTypedBinaryOp(GateRef gate);
    void VisitNumberBinaryOp(GateRef gate);
    void VisitStringBinaryOp(GateRef gate);
    void VisitInternStringBinaryOp(GateRef gate);
    void VisitTypedUnaryOp(GateRef gate);
    void VisitNumberNot(GateRef gate);
    void VisitTypedConditionJump(GateRef gate);
    void VisitConstant(GateRef gate);
    void VisitPhi(GateRef gate);
    void VisitUndefinedStrictEqOrUndefinedStrictNotEq(GateRef gate);
    void VisitUndefinedEqOrUndefinedNotEq(GateRef gate);
    void VisitEqualOrNotEqual(GateRef gate);
    void VisitRangeGuard(GateRef gate);
    void VisitRangeCheckPredicate(GateRef gate);
    void VisitIndexCheck(GateRef gate);
    void VisitLoadArrayLength(GateRef gate);
    void VisitLoadStringLength(GateRef gate);
    void VisitLoadMapSize(GateRef gate);
    void VisitLoadElement(GateRef gate);
    void VisitLoadProperty(GateRef gate);
    void VisitLoadPropertyOnProto(GateRef gate);
    void VisitRound(GateRef gate);
    void VisitFRound(GateRef gate);

    template<TypedBinOp Op>
    void VisitNumberCalculate(GateRef gate);
    template<TypedBinOp Op>
    void VisitNumberCompare(GateRef gate);
    template<TypedBinOp Op>
    void VisitNumberShift(GateRef gate);
    template<TypedBinOp Op>
    void VisitNumberLogical(GateRef gate);
    template<TypedUnOp Op>
    void VisitNumberMonocular(GateRef gate);
    void VisitNumberDiv(GateRef gate);
    void VisitNumberMultiply(GateRef gate);
    template<TypedBinOp Op>
    void VisitNumberMod(GateRef gate);
    void VisitBooleanJump(GateRef gate);
    void VisitIsTrueOrFalse(GateRef gate, bool flag);
    void VisitStrictEqualOrStrictNotEqual(GateRef gate);

    template<TypedBinOp Op>
    void VisitStringCompare(GateRef gate);
    template<TypedBinOp Op>
    void VisitStringAdd(GateRef gate);

    template<TypedBinOp Op>
    GateRef CalculateInts(GateRef left, GateRef right);
    template<TypedBinOp Op>
    GateRef CalculateDoubles(GateRef left, GateRef right);
    template<TypedBinOp Op>
    GateRef CompareInts(GateRef left, GateRef right);
    template<TypedBinOp Op>
    GateRef CompareDoubles(GateRef left, GateRef right);
    template<TypedBinOp Op>
    GateRef ShiftInts(GateRef left, GateRef right);
    template<TypedBinOp Op>
    GateRef LogicalInts(GateRef left, GateRef right);
    template<TypedUnOp Op>
    GateRef MonocularInt(GateRef value);
    template<TypedUnOp Op>
    GateRef MonocularDouble(GateRef value);

    TypeInfo GetOutputType(GateRef gate) const
    {
        auto index = acc_.GetId(gate);
        ASSERT(index < typeInfos_.size());
        return typeInfos_[index];
    }

    void UpdateRange(GateRef gate, const RangeInfo& range);
    RangeInfo GetRange(GateRef gate) const;
    GateRef GetConstInt32(int32_t v);
    GateRef GetConstDouble(double v);

    Circuit* circuit_;
    GateAccessor acc_;
    CircuitBuilder builder_;
    ChunkVector<TypeInfo>& typeInfos_;
    ChunkVector<RangeInfo>& rangeInfos_;
    ChunkVector<GateRef> rangeGuardGates_;
    GateRef glue_ {Circuit::NullGate()};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_NUMBER_SPECULATIVE_LOWERING_H
