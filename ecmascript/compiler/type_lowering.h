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
    TypeLowering(BytecodeCircuitBuilder *bcBuilder, Circuit *circuit, CompilationConfig *cmpCfg, TSManager *tsManager,
                 bool enableLog, const std::string& name)
        : bcBuilder_(bcBuilder), circuit_(circuit), acc_(circuit), builder_(circuit, cmpCfg),
          dependEntry_(Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY))), tsManager_(tsManager),
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
    void LowerTypeCheck(GateRef gate);
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
    void LowerTypedInc(GateRef gate);
    void LowerTypedDec(GateRef gate);
    void LowerTypedNeg(GateRef gate);
    void LowerTypedNot(GateRef gate);
    void LowerPrimitiveToNumber(GateRef dst, GateRef src, GateType srcType);
    void LowerIntCheck(GateRef gate);
    void LowerDoubleCheck(GateRef gate);
    void LowerNumberCheck(GateRef gate);
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
    void LowerNumberInc(GateRef gate);
    void LowerNumberDec(GateRef gate);
    void LowerNumberNeg(GateRef gate);
    void LowerNumberNot(GateRef gate);
    void LowerObjectTypeCheck(GateRef gate, GateRef glue);
    void LowerClassInstanceCheck(GateRef gate, GateRef glue);
    void LowerFloat32ArrayCheck(GateRef gate, GateRef glue);
    void LowerLoadProperty(GateRef gate, GateRef glue);
    void LowerStoreProperty(GateRef gate, GateRef glue);
    void LowerLoadElement(GateRef gate);
    void LowerStoreElement(GateRef gate, GateRef glue);

    template<OpCode::Op Op>
    GateRef FastAddOrSubOrMul(GateRef left, GateRef right);
    template<OpCode::Op Op>
    GateRef CalculateNumbers(GateRef left, GateRef right, GateType leftType, GateType rightType);
    template<OpCode::Op Op>
    GateRef ShiftNumber(GateRef left, GateRef right, GateType leftType, GateType rightType);
    template<TypedBinOp Op>
    GateRef CompareNumbers(GateRef left, GateRef right, GateType leftType, GateType rightType);
    template<TypedBinOp Op>
    GateRef CompareInt(GateRef left, GateRef right);
    template<TypedBinOp Op>
    GateRef CompareDouble(GateRef left, GateRef right);
    template<TypedUnOp Op>
    GateRef MonocularNumber(GateRef value, GateType valueType);
    template<OpCode::Op Op, MachineType Type>
    GateRef BinaryOp(GateRef x, GateRef y);
    GateRef DoubleToTaggedDoublePtr(GateRef gate);
    GateRef ChangeInt32ToFloat64(GateRef gate);
    GateRef TruncDoubleToInt(GateRef gate);
    GateRef Int32Mod(GateRef left, GateRef right);
    GateRef DoubleMod(GateRef left, GateRef right);
    GateRef IntToTaggedIntPtr(GateRef x);
    GateRef DoubleIsINF(GateRef x);
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

    BytecodeCircuitBuilder *bcBuilder_;
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
