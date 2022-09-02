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
#include "ecmascript/compiler/emca_bytecode.h"
#include "ecmascript/compiler/type_lowering.h"

namespace panda::ecmascript::kungfu {
void TypeLowering::RunTypeLowering()
{
    std::vector<GateRef> gateList;
    circuit_->GetAllGates(gateList);
    for (const auto &gate : gateList) {
        auto op = acc_.GetOpCode(gate);
        if (op == OpCode::JS_BYTECODE) {
            Lower(gate);
        }
    }

    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "================== type lowering print all gates Start==================";
        circuit_->PrintAllGates(*bcBuilder_);
    }
}

void TypeLowering::Lower(GateRef gate)
{
    ArgumentAccessor argAcc(circuit_);
    auto glue = argAcc.GetCommonArgGate(CommonArgIdx::GLUE);

    auto pc = bcBuilder_->GetJSBytecode(gate);
    EcmaBytecode op = static_cast<EcmaBytecode>(*pc);
    // initialize label manager
    Environment env(gate, circuit_, &builder_);
    switch (op) {
        case EcmaBytecode::NEWOBJRANGE:
            LowerTypeNewObjRange(gate, glue);
            break;
        case EcmaBytecode::ADD2:
            LowerTypeAdd2Dyn(gate, glue);
            break;
        default:
            break;
    }
}

void TypeLowering::RebuildSlowpathCfg(GateRef hir, std::map<GateRef, size_t> &stateGateMap)
{
    acc_.ReplaceStateIn(hir, builder_.GetState());
    acc_.ReplaceDependIn(hir, builder_.GetDepend());
    auto uses = acc_.Uses(hir);
    GateRef stateGate = Circuit::NullGate();
    for (auto useIt = uses.begin(); useIt != uses.end(); ++useIt) {
        const OpCode op = acc_.GetOpCode(*useIt);
        if (op == OpCode::IF_SUCCESS) {
            stateGate = *useIt;
            builder_.SetState(*useIt);
            break;
        }
    }
    auto nextUses = acc_.Uses(stateGate);
    for (auto it = nextUses.begin(); it != nextUses.end(); ++it) {
        if (it.GetOpCode().IsState()) {
            stateGateMap[*it] = it.GetIndex();
        }
    }
    builder_.SetDepend(hir);
}

void TypeLowering::GenerateSuccessMerge(std::vector<GateRef> &successControl)
{
    GateRef stateMerge = builder_.GetState();
    GateRef dependSelect = builder_.GetDepend();
    successControl.emplace_back(stateMerge);
    successControl.emplace_back(dependSelect);
}

void TypeLowering::ReplaceHirToFastPathCfg(GateRef hir, GateRef outir, const std::vector<GateRef> &successControl)
{
    auto uses = acc_.Uses(hir);
    for (auto useIt = uses.begin(); useIt != uses.end();) {
        const OpCode op = acc_.GetOpCode(*useIt);
        if (op == OpCode::JS_BYTECODE && useIt.GetIndex() == 1) {
            acc_.ReplaceStateIn(*useIt, successControl[0]);
            useIt = acc_.ReplaceIn(useIt, successControl[1]);
        } else if (op == OpCode::RETURN) {
            if (acc_.GetOpCode(acc_.GetIn(*useIt, 0)) != OpCode::IF_EXCEPTION) {
                acc_.ReplaceStateIn(*useIt, successControl[0]);
                acc_.ReplaceDependIn(*useIt, successControl[1]);
                acc_.ReplaceValueIn(*useIt, outir);
            }
            ++useIt;
        } else if (op == OpCode::IF_SUCCESS || op == OpCode::IF_EXCEPTION) {
            ++useIt;
        } else if (op == OpCode::VALUE_SELECTOR) {
            if (*useIt != outir) {
                useIt = acc_.ReplaceIn(useIt, outir);
            } else {
                ++useIt;
            }
        } else if (op == OpCode::DEPEND_SELECTOR) {
            if (*useIt != successControl[1]) {
                useIt = acc_.ReplaceIn(useIt, successControl[1]);
            } else {
                ++useIt;
            }
        } else {
            useIt = acc_.ReplaceIn(useIt, outir);
        }
    }
}

void TypeLowering::ReplaceHirToCall(GateRef hirGate, GateRef callGate, bool noThrow)
{
    GateRef stateInGate = acc_.GetState(hirGate);
    // copy depend-wire of hirGate to callGate
    GateRef dependInGate = acc_.GetDep(hirGate);
    acc_.SetDep(callGate, dependInGate);

    GateRef ifBranch;
    if (!noThrow) {
        // exception value
        GateRef exceptionVal = builder_.ExceptionConstant(GateType::TaggedNPointer());
        // compare with trampolines result
        GateRef equal = builder_.BinaryLogic(OpCode(OpCode::EQ), callGate, exceptionVal);
        ifBranch = builder_.Branch(stateInGate, equal);
    } else {
        ifBranch = builder_.Branch(stateInGate, builder_.Boolean(false));
    }

    auto uses = acc_.Uses(hirGate);
    for (auto it = uses.begin(); it != uses.end();) {
        if (acc_.GetOpCode(*it) == OpCode::IF_SUCCESS) {
            acc_.SetOpCode(*it, OpCode::IF_FALSE);
            it = acc_.ReplaceIn(it, ifBranch);
        } else {
            if (acc_.GetOpCode(*it) == OpCode::IF_EXCEPTION) {
                acc_.SetOpCode(*it, OpCode::IF_TRUE);
                it = acc_.ReplaceIn(it, ifBranch);
            } else {
                it = acc_.ReplaceIn(it, callGate);
            }
        }
    }

    // delete old gate
    acc_.DeleteGate(hirGate);
}

GateRef TypeLowering::LowerCallRuntime(GateRef glue, int index, const std::vector<GateRef> &args, bool useLabel)
{
    if (useLabel) {
        GateRef result = builder_.CallRuntime(glue, index, Gate::InvalidGateRef, args);
        return result;
    } else {
        const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(CallRuntime));
        GateRef target = builder_.IntPtr(index);
        GateRef result = builder_.Call(cs, glue, target, dependEntry_, args);
        return result;
    }
}

template<OpCode::Op Op>
GateRef TypeLowering::FastAddOrSubOrMul(GateRef left, GateRef right)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    DEFVAlUE(doubleLeft, (&builder_), VariableType::FLOAT64(), builder_.Double(0));
    DEFVAlUE(doubleRight, (&builder_), VariableType::FLOAT64(), builder_.Double(0));

    Label exit(&builder_);
    Label doFloatOp(&builder_);
    Label doIntOp(&builder_);
    Label leftIsNumber(&builder_);
    Label rightIsNumber(&builder_);
    Label leftIsIntRightIsDouble(&builder_);
    Label rightIsInt(&builder_);
    Label rightIsDouble(&builder_);
    builder_.Branch(builder_.TaggedIsNumber(left), &leftIsNumber, &exit);
    builder_.Bind(&leftIsNumber);
    {
        builder_.Branch(builder_.TaggedIsNumber(right), &rightIsNumber, &exit);
        builder_.Bind(&rightIsNumber);
        {
            Label leftIsInt(&builder_);
            Label leftIsDouble(&builder_);
            builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            builder_.Bind(&leftIsInt);
            {
                builder_.Branch(builder_.TaggedIsInt(right), &doIntOp, &leftIsIntRightIsDouble);
                builder_.Bind(&leftIsIntRightIsDouble);
                {
                    doubleLeft = ChangeInt32ToFloat64(builder_.TaggedCastToInt32(left));
                    doubleRight = builder_.TaggedCastToDouble(right);
                    builder_.Jump(&doFloatOp);
                }
            }
            builder_.Bind(&leftIsDouble);
            {
                builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                builder_.Bind(&rightIsInt);
                {
                    doubleLeft = builder_.TaggedCastToDouble(left);
                    doubleRight = ChangeInt32ToFloat64(builder_.TaggedCastToInt32(right));
                    builder_.Jump(&doFloatOp);
                }
                builder_.Bind(&rightIsDouble);
                {
                    doubleLeft = builder_.TaggedCastToDouble(left);
                    doubleRight = builder_.TaggedCastToDouble(right);
                    builder_.Jump(&doFloatOp);
                }
            }
        }
    }
    builder_.Bind(&doIntOp);
    {
        Label overflow(&builder_);
        Label notOverflow(&builder_);
        // handle left is int and right is int
        GateRef res = BinaryOp<Op, MachineType::I64>(builder_.TaggedCastToInt64(left),
                                                     builder_.TaggedCastToInt64(right));
        GateRef max = builder_.Int64(INT32_MAX);
        GateRef min = builder_.Int64(INT32_MIN);
        Label greaterZero(&builder_);
        Label notGreaterZero(&builder_);
        builder_.Branch(builder_.Int32GreaterThan(builder_.TaggedCastToInt32(left), builder_.Int32(0)),
                        &greaterZero, &notGreaterZero);
        builder_.Bind(&greaterZero);
        {
            builder_.Branch(builder_.Int64GreaterThan(res, max), &overflow, &notOverflow);
        }
        builder_.Bind(&notGreaterZero);
        {
            Label lessZero(&builder_);
            builder_.Branch(builder_.Int32LessThan(builder_.TaggedCastToInt32(left), builder_.Int32(0)),
                            &lessZero, &notOverflow);
            builder_.Bind(&lessZero);
            builder_.Branch(builder_.Int64LessThan(res, min), &overflow, &notOverflow);
        }
        builder_.Bind(&overflow);
        {
            GateRef newDoubleLeft = ChangeInt32ToFloat64(builder_.TaggedCastToInt32(left));
            GateRef newDoubleRight = ChangeInt32ToFloat64(builder_.TaggedCastToInt32(right));
            GateRef middleRet = BinaryOp<Op, MachineType::F64>(newDoubleLeft, newDoubleRight);
            result = DoubleToTaggedDoublePtr(middleRet);
            builder_.Jump(&exit);
        }
        builder_.Bind(&notOverflow);
        {
            result = builder_.ToTaggedIntPtr(res);
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&doFloatOp);
    {
        // Other situations
        auto res = BinaryOp<Op, MachineType::F64>(*doubleLeft, *doubleRight);
        result = DoubleToTaggedDoublePtr(res);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

template<OpCode::Op Op, MachineType Type>
GateRef TypeLowering::BinaryOp(GateRef x, GateRef y)
{
    return builder_.BinaryArithmetic(OpCode(Op), Type, x, y);
}

GateRef TypeLowering::DoubleToTaggedDoublePtr(GateRef gate)
{
    return builder_.DoubleToTaggedDoublePtr(gate);
}

GateRef TypeLowering::ChangeInt32ToFloat64(GateRef gate)
{
    return builder_.ChangeInt32ToFloat64(gate);
}

void TypeLowering::LowerTypeNewObjRange(GateRef gate, GateRef glue)
{
    GateRef ctor = acc_.GetValueIn(gate, 0);
    GateType ctorType = acc_.GetGateType(ctor);
    if (!ctorType.IsTSType()) {
        return;
    }

    if (!tsManager_->IsClassTypeKind(ctorType)) {
        return;
    }

    GlobalTSTypeRef gt = GlobalTSTypeRef(ctorType.GetType());
    std::map<GlobalTSTypeRef, uint32_t> gtHClassIndexMap = tsManager_->GetGtHClassIndexMap();
    int64_t index = gtHClassIndexMap[gt];
    GateRef ihcIndex = builder_.ToTaggedInt(builder_.Int64(index));

    size_t range = acc_.GetNumValueIn(gate);
    std::vector<GateRef> args(range + 1);

    for (size_t i = 0; i < range; ++i) {
        args[i] = acc_.GetValueIn(gate, i);
    }
    args[range] = ihcIndex;

    const int id = RTSTUB_ID(OptNewObjWithIHClass);
    GateRef newGate = LowerCallRuntime(glue, id, args);
    ReplaceHirToCall(gate, newGate);
}

void TypeLowering::LowerTypeAdd2Dyn(GateRef gate, [[maybe_unused]]GateRef glue)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateType leftType = acc_.GetGateType(left);

    GateRef right = acc_.GetValueIn(gate, 1);
    GateType rightType = acc_.GetGateType(right);
    if (!leftType.IsTSType() || !rightType.IsTSType()) {
        return;
    }
    if (!leftType.IsNumberType() || !rightType.IsNumberType()) {
        return;
    }
    std::map<GateRef, size_t> stateGateMap;
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = FastAddOrSubOrMul<OpCode::ADD>(left, right);
    Label successExit(&builder_);
    Label slowPath(&builder_);
    builder_.Branch(builder_.IsSpecial(*result, JSTaggedValue::VALUE_HOLE),
                    &slowPath, &successExit);
    builder_.Bind(&slowPath);
    {
        // slow path
        result = gate;
        RebuildSlowpathCfg(gate, stateGateMap);
        builder_.Jump(&successExit);
    }
    builder_.Bind(&successExit);
    for (auto [state, index] : stateGateMap) {
        acc_.ReplaceIn(state, index, builder_.GetState());
    }
    std::vector<GateRef> successControl;
    GenerateSuccessMerge(successControl);
    ReplaceHirToFastPathCfg(gate, *result, successControl);
}
}  // namespace panda::ecmascript