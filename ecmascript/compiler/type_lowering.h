/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_TYPE_LOWERING_H
#define ECMASCRIPT_COMPILER_TYPE_LOWERING_H

#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/circuit_builder-inl.h"

namespace panda::ecmascript::kungfu {
// TypeLowering Process
// SW: state wire, DW: depend wire, VW: value wire
// Before Type Lowering:
//                                    SW   DW   VW
//                                    |    |    |
//                                    |    |    |
//                                    v    v    v
//                                +-------------------+
//                                |       (HIR)       |    SW     +--------------+
//                            --DW|    JS_BYTECODE    |---------->| IF_EXCEPTION |
//                            |   +-------------------+           +--------------+
//                            |            SW       VW
//                            |            |        |
//                            |            v        |
//                            |    +--------------+ |
//                            |    |  IF_SUCCESS  | |
//                            |    +--------------+ |
//                            |            SW       |
//                            |            |        |
//                            |            v        v
//                            |   +-------------------+
//                            |   |       (HIR)       |
//                            --->|    JS_BYTECODE    |
//                                +-------------------+
//
// After Type Lowering:
//                                           SW
//                                           |
//                                           v
//                                 +-------------------+
//                                 |     IF_BRANCH     |
//                                 |    (Type Check)   |
//                                 +-------------------+
//                                    SW            SW
//                                    |             |
//                                    V             V
//                            +--------------+  +--------------+
//                            |    IF_TRUE   |  |   IF_FALSE   |
//                            +--------------+  +--------------+
//                 VW   DW          SW               SW                   DW   VW
//                 |    |           |                |                    |    |
//                 |    |           V                V                    |    |
//                 |    |  +---------------+     +---------------------+  |    |
//                 ------->|   FAST PATH   |     |        (HIR)        |<-------
//                         +---------------+     |     JS_BYTECODE     |
//                            VW  DW   SW        +---------------------+
//                            |   |    |               SW         VW  DW
//                            |   |    |               |          |   |
//                            |   |    |               v          |   |
//                            |   |    |         +--------------+ |   |
//                            |   |    |         |  IF_SUCCESS  | |   |
//                            |   |    |         +--------------+ |   |
//                            |   |    |                SW        |   |
//                            |   |    |                |         |   |
//                            |   |    v                v         |   |
//                            |   |  +---------------------+      |   |
//                            |   |  |        MERGE        |      |   |
//                            |   |  +---------------------+      |   |
//                            |   |    SW         SW    SW        |   |
//                            ----|----|----------|-----|--       |   |
//                             ---|----|----------|-----|-|-------|----
//                             |  |    |          |     | |       |
//                             v  v    v          |     v v       v
//                            +-----------------+ | +----------------+
//                            | DEPEND_SELECTOR | | | VALUE_SELECTOR |
//                            +-----------------+ | +----------------+
//                                    DW          |        VW
//                                    |           |        |
//                                    v           v        v
//                                  +------------------------+
//                                  |         (HIR)          |
//                                  |      JS_BYTECODE       |
//                                  +------------------------+

class TypeLowering {
public:
    TypeLowering(Circuit *circuit, CompilationConfig *cmpCfg, TSManager *tsManager,
                 bool enableLog, const std::string& name)
        : circuit_(circuit), acc_(circuit), builder_(circuit, cmpCfg),
          dependEntry_(circuit->GetDependRoot()), tsManager_(tsManager),
          enableLog_(enableLog), methodName_(name) {}

    ~TypeLowering() = default;

    void RunTypeLowering();

private:
    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    const std::string& GetMethodName() const
    {
        return methodName_;
    }

    void Lower(GateRef gate);
    void LowerType(GateRef gate);
    void LowerPrimitiveTypeCheck(GateRef gate);
    void LowerTypedBinaryOp(GateRef gate);
    void LowerTypeConvert(GateRef gate);
    void LowerTypedUnaryOp(GateRef gate);
    void LowerTypedAdd(GateRef gate);
    void LowerTypedSub(GateRef gate);
    void LowerTypedMul(GateRef gate);
    void LowerTypedMod(GateRef gate);
    void LowerTypedLess(GateRef gate);
    void LowerTypedLessEq(GateRef gate);
    void LowerTypedGreater(GateRef gate);
    void LowerTypedGreaterEq(GateRef gate);
    void LowerTypedDiv(GateRef gate);
    void LowerTypedEq(GateRef gate);
    void LowerTypedNotEq(GateRef gate);
    void LowerTypedShl(GateRef gate);
    void LowerTypedShr(GateRef gate);
    void LowerTypedAshr(GateRef gate);
    void LowerTypedAnd(GateRef gate);
    void LowerTypedOr(GateRef gate);
    void LowerTypedXor(GateRef gate);
    void LowerTypedInc(GateRef gate, GateType valueType);
    void LowerTypedDec(GateRef gate, GateType valueType);
    void LowerTypedNeg(GateRef gate, GateType valueType);
    void LowerTypedNot(GateRef gate, GateType valueType);
    void LowerTypedToBool(GateRef gate, GateType valueType);
    void LowerPrimitiveToNumber(GateRef dst, GateRef src, GateType srcType);
    void LowerIntCheck(GateRef gate);
    void LowerDoubleCheck(GateRef gate);
    void LowerNumberCheck(GateRef gate);
    void LowerBooleanCheck(GateRef gate);
    void LowerNumberAdd(GateRef gate);
    void LowerNumberSub(GateRef gate);
    void LowerNumberMul(GateRef gate);
    void LowerNumberMod(GateRef gate);
    void LowerNumberLess(GateRef gate);
    void LowerNumberLessEq(GateRef gate);
    void LowerNumberGreater(GateRef gate);
    void LowerNumberGreaterEq(GateRef gate);
    void LowerNumberDiv(GateRef gate);
    void LowerNumberEq(GateRef gate);
    void LowerNumberNotEq(GateRef gate);
    void LowerNumberShl(GateRef gate);
    void LowerNumberShr(GateRef gate);
    void LowerNumberAshr(GateRef gate);
    void LowerNumberAnd(GateRef gate);
    void LowerNumberOr(GateRef gate);
    void LowerNumberXor(GateRef gate);
    void LowerNumberInc(GateRef gate, GateType valueType);
    void LowerNumberDec(GateRef gate, GateType valueType);
    void LowerNumberNeg(GateRef gate, GateType valueType);
    void LowerNumberNot(GateRef gate, GateType valueType);
    void LowerNumberToBool(GateRef gate, GateType valueType);
    void LowerBooleanToBool(GateRef gate);
    void LowerIndexCheck(GateRef gate);
    void LowerOverflowCheck(GateRef gate);
    void LowerTypedIncOverflowCheck(GateRef gate);
    void LowerTypedDecOverflowCheck(GateRef gate);
    void LowerTypedNegOverflowCheck(GateRef gate);
    void LowerObjectTypeCheck(GateRef gate);
    void LowerClassInstanceCheck(GateRef gate);
    void LowerFloat32ArrayCheck(GateRef gate, GateRef glue);
    void LowerArrayCheck(GateRef gate, GateRef glue);
    void LowerStableArrayCheck(GateRef gate, GateRef glue);
    void LowerTypedArrayCheck(GateRef gate, GateRef glue);
    void LowerFloat32ArrayIndexCheck(GateRef gate);
    void LowerArrayIndexCheck(GateRef gate);
    void LowerLoadProperty(GateRef gate, GateRef glue);
    void LowerStoreProperty(GateRef gate, GateRef glue);
    void LowerLoadArrayLength(GateRef gate);
    void LowerStoreElement(GateRef gate, GateRef glue);
    void LowerLoadElement(GateRef gate);
    void LowerArrayLoadElement(GateRef gate);
    void LowerFloat32ArrayLoadElement(GateRef gate);
    void LowerFloat32ArrayStoreElement(GateRef gate, GateRef glue);
    void LowerHeapAllocate(GateRef gate, GateRef glue);
    void LowerHeapAllocateInYoung(GateRef gate, GateRef glue);
    void InitializeWithSpeicalValue(Label *exit, GateRef object, GateRef glue, GateRef value,
                                    GateRef start, GateRef end);
    void LowerTypedCallBuitin(GateRef gate);
    void LowerCallTargetCheck(GateRef gate);
    void LowerTypedNewAllocateThis(GateRef gate, GateRef glue);
    void LowerTypedSuperAllocateThis(GateRef gate, GateRef glue);
    void LowerGetSuperConstructor(GateRef gate);

    GateRef LowerCallRuntime(GateRef glue, int index, const std::vector<GateRef> &args, bool useLabel = false);

    template<OpCode Op>
    GateRef CalculateNumbers(GateRef left, GateRef right, GateType leftType, GateType rightType);
    template<OpCode Op>
    GateRef ShiftNumber(GateRef left, GateRef right, GateType leftType, GateType rightType);
    template<OpCode Op>
    GateRef LogicalNumbers(GateRef left, GateRef right, GateType leftType, GateType rightType);
    template<TypedBinOp Op>
    GateRef CompareNumbers(GateRef left, GateRef right, GateType leftType, GateType rightType);
    template<TypedBinOp Op>
    GateRef CompareInt(GateRef left, GateRef right);
    template<TypedBinOp Op>
    GateRef CompareDouble(GateRef left, GateRef right);
    template<TypedUnOp Op>
    GateRef MonocularNumber(GateRef value, GateType valueType);
    template<OpCode Op, MachineType Type>
    GateRef BinaryOp(GateRef x, GateRef y);
    GateRef DoubleToTaggedDoublePtr(GateRef gate);
    GateRef ChangeInt32ToFloat64(GateRef gate);
    GateRef TruncDoubleToInt(GateRef gate);
    GateRef Int32Mod(GateRef left, GateRef right);
    GateRef DoubleMod(GateRef left, GateRef right);
    GateRef IntToTaggedIntPtr(GateRef x);
    GateRef Less(GateRef left, GateRef right);
    GateRef LessEq(GateRef left, GateRef right);
    GateRef FastDiv(GateRef left, GateRef right);
    GateRef ModNumbers(GateRef left, GateRef right, GateType leftType, GateType rightType);
    GateRef DivNumbers(GateRef left, GateRef right, GateType leftType, GateType rightType);
    GateRef FastEqual(GateRef left, GateRef right);
    GateType GetLeftType(GateRef gate);
    GateType GetRightType(GateRef gate);
    GateRef GetConstPool(GateRef jsFunc);
    GateRef GetObjectFromConstPool(GateRef jsFunc, GateRef index);
    GateRef GetFrameState(GateRef gate) const
    {
        return acc_.GetFrameState(gate);
    }

    Circuit *circuit_;
    GateAccessor acc_;
    CircuitBuilder builder_;
    GateRef dependEntry_;
    [[maybe_unused]] TSManager *tsManager_ {nullptr};
    bool enableLog_ {false};
    std::string methodName_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TYPE_LOWERING_H
