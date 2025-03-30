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

#ifndef ECMASCRIPT_COMPILER_NUMBER_SPECULATIVE_RETYPE_H
#define ECMASCRIPT_COMPILER_NUMBER_SPECULATIVE_RETYPE_H

#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/combined_pass_visitor.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/compiler/number_gate_info.h"
#include "ecmascript/compiler/type.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {
class NumberSpeculativeRetype {
public:
    enum class State {
        Retype,
        Convert,
    };
    NumberSpeculativeRetype(Circuit* circuit, Chunk* chunk, ChunkVector<TypeInfo>& typeInfos)
        : circuit_(circuit), acc_(circuit), builder_(circuit),
          typeInfos_(typeInfos), chunk_(chunk) {}
    GateRef VisitGate(GateRef gate);
    void setState(NumberSpeculativeRetype::State state);
    void ConvertMonoAccessorValueIn(GateRef gate);

private:

    enum class OpType {
        NORMAL,
        SHIFT_AND_LOGICAL,
    };

    enum class ConvertToNumber {
        DISABLE,
        BOOL_ONLY,
        ALL
    };

    ConvertSupport ToConvertSupport(ConvertToNumber convert)
    {
        return convert == ConvertToNumber::DISABLE ? ConvertSupport::DISABLE : ConvertSupport::ENABLE;
    }

    bool IsRetype() const
    {
        return state_ == State::Retype;
    }

    bool IsConvert() const
    {
        return state_ == State::Convert;
    }

    GateRef SetOutputType(GateRef gate, GateType type);
    GateRef SetOutputType(GateRef gate, ParamType type);
    GateRef SetOutputType(GateRef gate, Representation rep);
    GateRef SetOutputType(GateRef gate, TypeInfo type);
    TypeInfo GetNumberTypeInfo(GateRef gate);
    GateRef VisitPhi(GateRef gate);
    GateRef VisitArrayFindOrFindIndex(GateRef gate);
    GateRef VisitConstant(GateRef gate);
    GateRef VisitTypedBinaryOp(GateRef gate);
    GateRef VisitNumberBinaryOp(GateRef gate);
    GateRef VisitStringBinaryOp(GateRef gate);
    GateRef VisitUndefinedEqualCompareOrUndefinedNotEqualCompare(GateRef gate);
    GateRef VisitTypedUnaryOp(GateRef gate);

    GateRef VisitNumberMonocular(GateRef gate);
    GateRef VisitNumberNot(GateRef gate);
    GateRef VisitTypedConditionJump(GateRef gate);

    GateRef VisitIntMonocular(GateRef gate);
    GateRef VisitDoubleMonocular(GateRef gate);

    GateRef VisitNumberCalculate(GateRef gate);
    GateRef VisitNumberCompare(GateRef gate);
    GateRef VisitNumberShiftAndLogical(GateRef gate);
    GateRef VisitNumberToString(GateRef gate);
    GateRef VisitNumberParseFloat(GateRef gate);
    GateRef VisitNumberParseInt(GateRef gate);
    GateRef VisitMathDoubleParamsBuiltin(GateRef gate);
    const GateMetaData *GetNewMeta(OpCode op, TypeInfo type);
    void UpdateMeta(GateRef gate, TypeInfo newType, const GateMetaData *meta);
    GateRef VisitMathTaggedNumberParamsBuiltin(GateRef gate);
    GateRef VisitClz32Builtin(GateRef gate);
    GateRef VisitMathTrunc(GateRef gate);
    GateRef VisitMathImul(GateRef gate);
    template <bool IS_NAN>
    GateRef VisitNumberOrGlobalBuiltin(GateRef gate);
    GateRef VisitNumberIsInteger(GateRef gate);
    GateRef VisitBigIntAsIntN(GateRef gate);
    GateRef VisitBooleanJump(GateRef gate);
    GateRef VisitRangeCheckPredicate(GateRef gate);
    GateRef VisitIndexCheck(GateRef gate);
    GateRef VisitLoadArrayLength(GateRef gate);
    GateRef VisitLoadStringLength(GateRef gate);
    GateRef VisitLoadMapSize(GateRef gate);
    GateRef VisitLoadElement(GateRef gate);
    GateRef VisitStoreElement(GateRef gate);
    GateRef VisitStoreProperty(GateRef gate);
    GateRef VisitLoadProperty(GateRef gate);
    GateRef VisitNumberRelated(GateRef gate, ParamType paramType);
    GateRef VisitDataViewGet(GateRef gate);
    GateRef VisitDataViewSet(GateRef gate);
    GateRef VisitOthers(GateRef gate, GateType outputType = GateType::AnyType());
    GateRef VisitOthersWithoutConvert(GateRef gate, GateType outputType = GateType::AnyType());
    GateRef VisitBigIntConstructor(GateRef gate);
    GateRef VisitTypeConvert(GateRef gate);
    GateRef VisitFrameState(GateRef gate);
    GateRef VisitIsTrueOrFalse(GateRef gate);
    GateRef VisitWithConstantValue(GateRef gate, size_t ignoreIndex);
    GateRef VisitIntermediateValue(GateRef gate);
    GateRef VisitEqualCompareOrNotEqualCompare(GateRef gate);

    GateRef VisitStringCompare(GateRef gate);
    GateRef VisitStringAdd(GateRef gate);
    GateRef VisitMonoLoadPropertyOnProto(GateRef gate);
    GateRef VisitMonoCallGetterOnProto(GateRef gate);
    GateRef VisitMonoStoreProperty(GateRef gate);
    GateRef VisitDateGetTime(GateRef gate);
    GateRef VisitDateNow(GateRef gate);
    GateRef VisitArrayIncludesIndexOf(GateRef gate);
    GateRef VisitStringCharCodeAt(GateRef gate);
    GateRef VisitString(GateRef gate);

    void ConvertForNumberBinaryOp(GateRef gate);
    void ConvertForNumberCompareOp(GateRef gate);
    void ConvertForNumberShiftAndLogicalOperator(GateRef gate);

    void ConvertForIntOperator(GateRef gate, GateType leftType, GateType rightType);
    void ConvertForShiftAndLogicalOperator(GateRef gate, GateType leftType, GateType rightType);
    void ConvertForDoubleOperator(GateRef gate, GateType leftType, GateType rightType);

    template<bool BOOL_AS_INT = true>
    TypeInfo GetNumberInputTypeInfo(GateRef gate, bool skipTagged = false);
    void SetNewInputForMathImul(GateRef gate, int idx);
    double GetDoubleValueFromConst(GateRef gate);

    GateRef CheckAndConvertToInt32(GateRef gate, GateType gateType, ConvertSupport support = ConvertSupport::ENABLE,
                                   OpType type = OpType::NORMAL, bool needCheckFloat64 = true);
    GateRef CheckTaggedAndConvertToInt32(GateRef gate, GateType gateType, OpType type);
    GateRef CheckBoundAndConvertToInt32(GateRef gate,
                                        ConvertSupport support = ConvertSupport::ENABLE,
                                        OpType type = OpType::NORMAL);
    GateRef CheckAndConvertToFloat64(GateRef gate, GateType gateType,
                                    ConvertToNumber convert = ConvertToNumber::BOOL_ONLY);
    GateRef CheckAndConvertToTagged(GateRef gate, GateType gateType, ConvertToNumber convert);
    GateRef CheckAndConvertToBool(GateRef gate, GateType gateType);
    GateRef ConvertToTagged(GateRef gate);
    GateRef TryConvertConstant(GateRef gate, bool needInt32);
    GateRef TryConvertConstantToInt32(GateRef gate);
    GateRef ConvertTaggedToNJSValue(GateRef gate, TypeInfo output);
    TypeInfo GetOutputForPhi(GateRef gate, bool ignoreConstant);

    TypeInfo GetOutputTypeInfo(GateRef gate) const
    {
        auto index = acc_.GetId(gate);
        ASSERT(index < typeInfos_.size());
        return typeInfos_[index];
    }

    void SetOutputTypeInfo(GateRef gate, TypeInfo info)
    {
        auto index = acc_.GetId(gate);
        ASSERT(index < typeInfos_.size());
        typeInfos_[index] = info;
    }

    void ResizeAndSetTypeInfo(GateRef gate, TypeInfo info)
    {
        auto index = acc_.GetId(gate);
        if (index >= typeInfos_.size()) {
            // +1: for size
            typeInfos_.resize(index + 1, TypeInfo::NONE);
        }
        typeInfos_[index] = info;
    }

    static constexpr size_t PROPERTY_LOOKUP_RESULT_INDEX = 1;
    static constexpr size_t HCLASS_INDEX = 2;
    Circuit *circuit_ {nullptr};
    GateAccessor acc_;
    CircuitBuilder builder_;
    ChunkVector<TypeInfo>& typeInfos_;
    State state_ {0};
    [[maybe_unused]] Chunk *chunk_ {nullptr};
};

class NumberSpeculativeRetypeManager : public PassVisitor {
public:
    NumberSpeculativeRetypeManager(Circuit* circuit, RPOVisitor* visitor, Chunk* chunk,
                                   NumberSpeculativeRetype* retype, NumberSpeculativeRetype::State state)
        : PassVisitor(circuit, chunk, visitor), retype_(retype), state_(state) {}
    GateRef VisitGate(GateRef gate) override;
private:
    NumberSpeculativeRetype* retype_;
    NumberSpeculativeRetype::State state_;
};

}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_NUMBER_SPECULATIVE_RETYPE_H
