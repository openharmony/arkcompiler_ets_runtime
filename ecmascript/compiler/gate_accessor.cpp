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

#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/graph_editor.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript::kungfu {
using UseIterator = GateAccessor::UseIterator;

size_t GateAccessor::GetNumIns(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetNumIns();
}

MarkCode GateAccessor::GetMark(GateRef gate) const
{
    return circuit_->GetMark(gate);
}

void GateAccessor::SetMark(GateRef gate, MarkCode mark)
{
    circuit_->SetMark(gate, mark);
}

bool GateAccessor::IsFinished(GateRef gate) const
{
    return GetMark(gate) == MarkCode::FINISHED;
}

bool GateAccessor::IsVisited(GateRef gate) const
{
    return GetMark(gate) == MarkCode::VISITED;
}

bool GateAccessor::IsNotMarked(GateRef gate) const
{
    return GetMark(gate) == MarkCode::NO_MARK;
}

void GateAccessor::SetFinished(GateRef gate)
{
    SetMark(gate, MarkCode::FINISHED);
}

void GateAccessor::SetVisited(GateRef gate)
{
    SetMark(gate, MarkCode::VISITED);
}

OpCode GateAccessor::GetOpCode(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetOpCode();
}

BitField GateAccessor::TryGetValue(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->TryGetValue();
}

ICmpCondition GateAccessor::GetICmpCondition(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::ICMP);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return static_cast<ICmpCondition>(gatePtr->GetOneParameterMetaData()->GetValue());
}

FCmpCondition GateAccessor::GetFCmpCondition(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::FCMP);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return static_cast<FCmpCondition>(gatePtr->GetOneParameterMetaData()->GetValue());
}

size_t GateAccessor::GetOffset(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::LOAD_CONST_OFFSET ||
           GetOpCode(gate) == OpCode::STORE_CONST_OFFSET);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetOneParameterMetaData()->GetValue();
}

uint32_t GateAccessor::GetTrueWeight(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::IF_BRANCH);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    auto accessor = BranchAccessor(gatePtr->GetOneParameterMetaData()->GetValue());
    return accessor.GetTrueWeight();
}

uint32_t GateAccessor::GetFalseWeight(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::IF_BRANCH);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    auto accessor = BranchAccessor(gatePtr->GetOneParameterMetaData()->GetValue());
    return accessor.GetFalseWeight();
}

bool GateAccessor::HasBranchWeight(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::IF_BRANCH);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    auto accessor = BranchAccessor(gatePtr->GetOneParameterMetaData()->GetValue());
    return (accessor.GetTrueWeight() != 0) || (accessor.GetFalseWeight() != 0);
}

size_t GateAccessor::GetIndex(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::GET_GLOBAL_ENV_OBJ_HCLASS ||
           GetOpCode(gate) == OpCode::GET_GLOBAL_CONSTANT_VALUE ||
           GetOpCode(gate) == OpCode::GET_GLOBAL_ENV_OBJ);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetOneParameterMetaData()->GetValue();
}

uint32_t GateAccessor::GetArraySize(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::CREATE_ARRAY ||
           GetOpCode(gate) == OpCode::CREATE_ARRAY_WITH_BUFFER);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    auto array = gatePtr->GetOneParameterMetaData()->GetValue();
    return ArrayMetaDataAccessor(array).GetArrayLength();
}

void GateAccessor::SetArraySize(GateRef gate, uint32_t size)
{
    ASSERT(GetOpCode(gate) == OpCode::CREATE_ARRAY ||
           GetOpCode(gate) == OpCode::CREATE_ARRAY_WITH_BUFFER);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    ArrayMetaDataAccessor accessor(gatePtr->GetOneParameterMetaData()->GetValue());
    accessor.SetArrayLength(size);
    const_cast<OneParameterMetaData *>(gatePtr->GetOneParameterMetaData())->SetValue(accessor.ToValue());
}

TypedUnaryAccessor GateAccessor::GetTypedUnAccessor(GateRef gate) const
{
    ASSERT((GetOpCode(gate) == OpCode::TYPED_UNARY_OP));
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return TypedUnaryAccessor(gatePtr->GetOneParameterMetaData()->GetValue());
}

TypedJumpAccessor GateAccessor::GetTypedJumpAccessor(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::TYPED_CONDITION_JUMP);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return TypedJumpAccessor(gatePtr->GetOneParameterMetaData()->GetValue());
}

ArrayMetaDataAccessor GateAccessor::GetArrayMetaDataAccessor(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::STABLE_ARRAY_CHECK ||
           GetOpCode(gate) == OpCode::HCLASS_STABLE_ARRAY_CHECK ||
           GetOpCode(gate) == OpCode::CREATE_ARRAY ||
           GetOpCode(gate) == OpCode::CREATE_ARRAY_WITH_BUFFER);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return ArrayMetaDataAccessor(gatePtr->GetOneParameterMetaData()->GetValue());
}

ObjectTypeAccessor GateAccessor::GetObjectTypeAccessor(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::OBJECT_TYPE_CHECK ||
           GetOpCode(gate) == OpCode::OBJECT_TYPE_COMPARE);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return ObjectTypeAccessor(gatePtr->GetOneParameterMetaData()->GetValue());
}

bool GateAccessor::TypedOpIsTypedArray(GateRef gate, TypedOpKind kind) const
{
    switch (kind) {
        case TypedOpKind::TYPED_LOAD_OP: {
            TypedLoadOp op = GetTypedLoadOp(gate);
            return TypedLoadOp::TYPED_ARRAY_FIRST <= op && op <=TypedLoadOp::TYPED_ARRAY_LAST;
        }
        case TypedOpKind::TYPED_STORE_OP: {
            TypedStoreOp op = GetTypedStoreOp(gate);
            return TypedStoreOp::TYPED_ARRAY_FIRST <= op && op <= TypedStoreOp::TYPED_ARRAY_LAST;
        }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

TypedLoadOp GateAccessor::GetTypedLoadOp(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::LOAD_ELEMENT);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return static_cast<TypedLoadOp>(gatePtr->GetOneParameterMetaData()->GetValue());
}

TypedStoreOp GateAccessor::GetTypedStoreOp(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::STORE_ELEMENT);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return static_cast<TypedStoreOp>(gatePtr->GetOneParameterMetaData()->GetValue());
}

TypedCallTargetCheckOp GateAccessor::GetTypedCallTargetCheckOp(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::TYPED_CALLTARGETCHECK_OP);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetTypedCallTargetCheckMetaData()->GetTypedCallTargetCheckOp();
}

MemoryType GateAccessor::GetMemoryType(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::STORE_MEMORY);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return static_cast<MemoryType>(gatePtr->GetOneParameterMetaData()->GetValue());
}

TypedBinOp GateAccessor::GetTypedBinaryOp(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::TYPED_BINARY_OP);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetTypedBinaryMetaData()->GetTypedBinaryOp();
}

PGOSampleType GateAccessor::GetTypedBinaryType(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::TYPED_BINARY_OP);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetTypedBinaryMetaData()->GetType();
}

bool GateAccessor::HasNumberType(GateRef gate) const
{
    auto sampleType = GetTypedBinaryType(gate);
    if (sampleType.IsNumber()) {
        return true;
    }
    if (sampleType.IsNone()) {
        GateType leftType = GetLeftType(gate);
        GateType rightType = GetRightType(gate);
        if (leftType.IsNumberType() && rightType.IsNumberType()) {
            return true;
        }
    }
    return false;
}

GlobalTSTypeRef GateAccessor::GetFuncGT(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::JSINLINETARGET_TYPE_CHECK);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    auto value = static_cast<uint32_t>((gatePtr->GetOneParameterMetaData()->GetValue()));
    return GlobalTSTypeRef(value);
}

GateType GateAccessor::GetParamGateType(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::PRIMITIVE_TYPE_CHECK ||
           GetOpCode(gate) == OpCode::TYPED_ARRAY_CHECK ||
           GetOpCode(gate) == OpCode::INDEX_CHECK ||
           GetOpCode(gate) == OpCode::TYPED_CALLTARGETCHECK_OP ||
           GetOpCode(gate) == OpCode::CREATE_ARRAY_WITH_BUFFER);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    GateTypeAccessor accessor(gatePtr->GetOneParameterMetaData()->GetValue());
    return accessor.GetGateType();
}

bool GateAccessor::IsConvertSupport(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::CONVERT ||
           GetOpCode(gate) == OpCode::CHECK_AND_CONVERT);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    ValuePairTypeAccessor accessor(gatePtr->GetOneParameterMetaData()->GetValue());
    return accessor.IsConvertSupport();
}

ValueType GateAccessor::GetSrcType(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::CONVERT ||
           GetOpCode(gate) == OpCode::CHECK_AND_CONVERT);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    ValuePairTypeAccessor accessor(gatePtr->GetOneParameterMetaData()->GetValue());
    return accessor.GetSrcType();
}

ValueType GateAccessor::GetDstType(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::CONVERT ||
           GetOpCode(gate) == OpCode::CHECK_AND_CONVERT);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    ValuePairTypeAccessor accessor(gatePtr->GetOneParameterMetaData()->GetValue());
    return accessor.GetDstType();
}

GateType GateAccessor::GetLeftType(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::TYPED_UNARY_OP ||
           GetOpCode(gate) == OpCode::TYPED_BINARY_OP ||
           GetOpCode(gate) == OpCode::TYPE_CONVERT);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    GatePairTypeAccessor accessor(gatePtr->GetOneParameterMetaData()->GetValue());
    return accessor.GetLeftType();
}

GateType GateAccessor::GetRightType(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::TYPED_BINARY_OP ||
           GetOpCode(gate) == OpCode::TYPE_CONVERT);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    GatePairTypeAccessor accessor(gatePtr->GetOneParameterMetaData()->GetValue());
    return accessor.GetRightType();
}

uint32_t GateAccessor::GetFirstValue(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::RANGE_GUARD);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    UInt32PairAccessor accessor(gatePtr->GetOneParameterMetaData()->GetValue());
    return accessor.GetFirstValue();
}

uint32_t GateAccessor::GetSecondValue(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::RANGE_GUARD);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    UInt32PairAccessor accessor(gatePtr->GetOneParameterMetaData()->GetValue());
    return accessor.GetSecondValue();
}

size_t GateAccessor::GetVirtualRegisterIndex(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::SAVE_REGISTER ||
           GetOpCode(gate) == OpCode::RESTORE_REGISTER);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return static_cast<size_t>(gatePtr->GetOneParameterMetaData()->GetValue());
}

uint64_t GateAccessor::GetConstantValue(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::CONSTANT);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetOneParameterMetaData()->GetValue();
}

const ChunkVector<char>& GateAccessor::GetConstantString(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::CONSTSTRING);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetStringMetaData()->GetString();
}

bool GateAccessor::IsVtable(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::LOAD_PROPERTY);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetBoolMetaData()->GetBool();
}

bool GateAccessor::GetNoGCFlag(GateRef gate) const
{
    if (gate == Circuit::NullGate()) {
        return false;
    }
    OpCode op = GetOpCode(gate);
    if (op != OpCode::TYPEDCALL && op != OpCode::TYPEDFASTCALL) {
        return false;
    }
    return TypedCallIsNoGC(gate);
}

bool GateAccessor::TypedCallIsNoGC(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::TYPEDCALL || GetOpCode(gate) == OpCode::TYPEDFASTCALL);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetTypedCallMetaData()->IsNoGC();
}

bool GateAccessor::IsNoGC(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::CALL_OPTIMIZED || GetOpCode(gate) == OpCode::FAST_CALL_OPTIMIZED);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetBoolMetaData()->GetBool();
}

uint32_t GateAccessor::TryGetPcOffset(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    OpCode op = GetOpCode(gate);
    switch (op) {
        case OpCode::JS_BYTECODE:
            return gatePtr->GetJSBytecodeMetaData()->GetPcOffset();
        case OpCode::TYPED_CALL_BUILTIN:
        case OpCode::CONSTRUCT:
        case OpCode::CALL_GETTER:
        case OpCode::CALL_SETTER:
            return static_cast<uint32_t>(gatePtr->GetOneParameterMetaData()->GetValue());
        case OpCode::TYPEDCALL:
        case OpCode::TYPEDFASTCALL:
            return static_cast<uint32_t>(gatePtr->GetTypedCallMetaData()->GetValue());
        case OpCode::FRAME_STATE: {
            UInt32PairAccessor accessor(gatePtr->GetOneParameterMetaData()->GetValue());
            return accessor.GetFirstValue();
        }
        default:
            break;
    }
    return 0;
}

PGOSampleType GateAccessor::TryGetPGOType(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    OpCode op = GetOpCode(gate);
    if (op == OpCode::JS_BYTECODE) {
        return gatePtr->GetJSBytecodeMetaData()->GetType();
    }
    if (op == OpCode::TYPED_BINARY_OP) {
        return GetTypedBinaryType(gate);
    }
    return PGOSampleType::NoneType();
}

void GateAccessor::TrySetPGOType(GateRef gate, PGOSampleType type)
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    OpCode op = GetOpCode(gate);
    if (op == OpCode::JS_BYTECODE) {
        const_cast<JSBytecodeMetaData *>(gatePtr->GetJSBytecodeMetaData())->SetType(type);
    }
}

ElementsKind GateAccessor::TryGetElementsKind(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    OpCode op = GetOpCode(gate);
    if (op == OpCode::JS_BYTECODE) {
        return gatePtr->GetJSBytecodeMetaData()->GetElementsKind();
    }
    return ElementsKind::GENERIC;
}

ElementsKind GateAccessor::TryGetArrayElementsKind(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    OpCode op = GetOpCode(gate);
    if (op == OpCode::JS_BYTECODE) {
        ElementsKind kind = gatePtr->GetJSBytecodeMetaData()->GetElementsKind();
        if (Elements::IsGeneric(kind)) {
            return kind;
        }
        std::vector<ElementsKind> kinds = gatePtr->GetJSBytecodeMetaData()->GetElementsKinds();
        for (auto &x : kinds) {
            kind = Elements::MergeElementsKind(kind, x);
        }
        return kind;
    }
    return ElementsKind::GENERIC;
}

void GateAccessor::TrySetElementsKind(GateRef gate, ElementsKind kind)
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    OpCode op = GetOpCode(gate);
    if (op == OpCode::JS_BYTECODE) {
        const_cast<JSBytecodeMetaData *>(gatePtr->GetJSBytecodeMetaData())->SetElementsKind(kind);
    }
}

EcmaOpcode GateAccessor::GetByteCodeOpcode(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::JS_BYTECODE);
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetJSBytecodeMetaData()->GetByteCodeOpcode();
}

void GateAccessor::Print(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    gatePtr->Print();
}

void GateAccessor::ShortPrint(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    gatePtr->ShortPrint();
}

GateId GateAccessor::GetId(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetId();
}

size_t GateAccessor::GetInValueStarts(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetInValueStarts();
}

GateRef GateAccessor::GetValueIn(GateRef gate, size_t idx) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    ASSERT(idx < gatePtr->GetInValueCount());
    size_t valueIndex = gatePtr->GetInValueStarts();
    return circuit_->GetIn(gate, valueIndex + idx);
}

size_t GateAccessor::GetNumValueIn(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetInValueCount();
}

bool GateAccessor::IsGCRelated(GateRef gate) const
{
    return GetGateType(gate).IsGCRelated();
}

GateRef GateAccessor::GetIn(GateRef gate, size_t idx) const
{
    return circuit_->GetIn(gate, idx);
}

GateRef GateAccessor::GetState(GateRef gate, size_t idx) const
{
    ASSERT(idx < circuit_->LoadGatePtr(gate)->GetStateCount());
    return circuit_->GetIn(gate, idx);
}

void GateAccessor::GetInStates(GateRef gate, std::vector<GateRef>& ins) const
{
    const Gate *curGate = circuit_->LoadGatePtrConst(gate);
    for (size_t idx = 0; idx < curGate->GetStateCount(); idx++) {
        ins.push_back(circuit_->GetGateRef(curGate->GetInGateConst(idx)));
    }
}

void GateAccessor::GetIns(GateRef gate, std::vector<GateRef>& ins) const
{
    const Gate *curGate = circuit_->LoadGatePtrConst(gate);
    for (size_t idx = 0; idx < curGate->GetNumIns(); idx++) {
        ins.push_back(circuit_->GetGateRef(curGate->GetInGateConst(idx)));
    }
}

void GateAccessor::GetOuts(GateRef gate, std::vector<GateRef>& outs) const
{
    const Gate *curGate = circuit_->LoadGatePtrConst(gate);
    if (!curGate->IsFirstOutNull()) {
        const Out *curOut = curGate->GetFirstOutConst();
        GateRef ref = circuit_->GetGateRef(curOut->GetGateConst());
        outs.push_back(ref);
        while (!curOut->IsNextOutNull()) {
            curOut = curOut->GetNextOutConst();
            ref = circuit_->GetGateRef(curOut->GetGateConst());
            outs.push_back(ref);
        }
    }
}

bool GateAccessor::HasOuts(GateRef gate) const
{
    const Gate *curGate = circuit_->LoadGatePtrConst(gate);
    return !curGate->IsFirstOutNull();
}

void GateAccessor::DeleteGateIfNoUse(GateRef gate)
{
    if (!HasOuts(gate)) {
        DeleteGate(gate);
    }
}

void GateAccessor::GetOutStates(GateRef gate, std::vector<GateRef>& outStates) const
{
    const Gate *curGate = circuit_->LoadGatePtrConst(gate);
    if (!curGate->IsFirstOutNull()) {
        const Out *curOut = curGate->GetFirstOutConst();
        GateRef ref = circuit_->GetGateRef(curOut->GetGateConst());
        if (GetMetaData(ref)->IsState()) {
            outStates.push_back(ref);
        }
        while (!curOut->IsNextOutNull()) {
            curOut = curOut->GetNextOutConst();
            ref = circuit_->GetGateRef(curOut->GetGateConst());
            if (GetMetaData(ref)->IsState()) {
                outStates.push_back(ref);
            }
        }
    }
}

void GateAccessor::GetStateUses(GateRef gate, std::vector<GateRef> &stateUses)
{
    stateUses.clear();
    auto uses = Uses(gate);
    for (auto it = uses.begin(); it != uses.end(); it++) {
        if (IsStateIn(it)) {
            stateUses.emplace_back(*it);
        }
    }
}

void GateAccessor::GetDependUses(GateRef gate, std::vector<GateRef> &dependUses)
{
    dependUses.clear();
    auto uses = Uses(gate);
    for (auto it = uses.begin(); it != uses.end(); it++) {
        if (IsDependIn(it)) {
            dependUses.emplace_back(*it);
        }
    }
}

void GateAccessor::GetValueUses(GateRef gate, std::vector<GateRef> &valueUses)
{
    valueUses.clear();
    auto uses = Uses(gate);
    for (auto it = uses.begin(); it != uses.end(); it++) {
        if (IsValueIn(it)) {
            valueUses.emplace_back(*it);
        }
    }
}

void GateAccessor::GetAllGates(std::vector<GateRef>& gates) const
{
    circuit_->GetAllGates(gates);
}

bool GateAccessor::IsInGateNull(GateRef gate, size_t idx) const
{
    return circuit_->IsInGateNull(gate, idx);
}

bool GateAccessor::IsValueSelector(GateRef g) const
{
    return GetOpCode(g) == OpCode::VALUE_SELECTOR;
}

bool GateAccessor::IsSelector(GateRef g) const
{
    auto op = GetOpCode(g);
    return (op == OpCode::VALUE_SELECTOR) || (op == OpCode::DEPEND_SELECTOR);
}

bool GateAccessor::IsIn(GateRef g, GateRef in) const
{
    size_t n = GetNumIns(g);
    for (size_t id = 0; id < n; id++) {
        GateRef i = GetIn(g, id);
        if (i == in) {
            return true;
        }
    }
    return false;
}

bool GateAccessor::IsSimpleState(GateRef g) const
{
    auto op = GetOpCode(g);
    return (op == OpCode::IF_TRUE ||
            op == OpCode::IF_FALSE ||
            op == OpCode::SWITCH_CASE ||
            op == OpCode::DEFAULT_CASE ||
            op == OpCode::LOOP_BACK ||
            op == OpCode::MERGE ||
            op == OpCode::VALUE_SELECTOR ||
            op == OpCode::DEPEND_SELECTOR ||
            op == OpCode::DEPEND_RELAY ||
            op == OpCode::ORDINARY_BLOCK);
}

bool GateAccessor::IsControlCase(GateRef gate) const
{
    return circuit_->IsControlCase(gate);
}

bool GateAccessor::IsLoopExit(GateRef gate) const
{
    return (GetOpCode(gate) == OpCode::LOOP_EXIT);
}

bool GateAccessor::IsLoopExitRelated(GateRef gate) const
{
    return (GetOpCode(gate) == OpCode::LOOP_EXIT) ||
           (GetOpCode(gate) == OpCode::LOOP_EXIT_DEPEND) ||
           (GetOpCode(gate) == OpCode::LOOP_EXIT_VALUE);
}

bool GateAccessor::IsLoopHead(GateRef gate) const
{
    return circuit_->IsLoopHead(gate);
}

bool GateAccessor::IsLoopBack(GateRef gate) const
{
    return GetOpCode(gate) == OpCode::LOOP_BACK;
}

bool GateAccessor::IsState(GateRef gate) const
{
    return GetMetaData(gate)->IsState();
}

bool GateAccessor::IsConstant(GateRef gate) const
{
    return GetMetaData(gate)->IsConstant();
}

bool GateAccessor::IsDependSelector(GateRef gate) const
{
    return GetMetaData(gate)->IsDependSelector();
}

bool GateAccessor::IsConstantValue(GateRef gate, uint64_t value) const
{
    if (GetOpCode(gate) == OpCode::CONSTANT) {
        uint64_t bitField = GetConstantValue(gate);
        return bitField == value;
    }
    return false;
}

bool GateAccessor::IsConstantUndefined(GateRef gate) const
{
    return IsConstantValue(gate, JSTaggedValue::VALUE_UNDEFINED);
}

bool GateAccessor::IsTypedOperator(GateRef gate) const
{
    return GetMetaData(gate)->IsTypedOperator();
}

bool GateAccessor::IsNotWrite(GateRef gate) const
{
    return GetMetaData(gate)->IsNotWrite();
}

bool GateAccessor::IsCheckWithTwoIns(GateRef gate) const
{
    return GetMetaData(gate)->IsCheckWithTwoIns();
}

bool GateAccessor::IsCheckWithOneIn(GateRef gate) const
{
    return GetMetaData(gate)->IsCheckWithOneIn();
}

bool GateAccessor::IsSchedulable(GateRef gate) const
{
    return GetMetaData(gate)->IsSchedulable();
}

bool GateAccessor::IsVirtualState(GateRef gate) const
{
    return GetMetaData(gate)->IsVirtualState();
}

bool GateAccessor::IsGeneralState(GateRef gate) const
{
    return GetMetaData(gate)->IsGeneralState();
}

GateRef GateAccessor::GetDep(GateRef gate, size_t idx) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    ASSERT(idx < gatePtr->GetDependCount());
    size_t dependIndex = gatePtr->GetStateCount();
    return circuit_->GetIn(gate, dependIndex + idx);
}

size_t GateAccessor::GetImmediateId(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    ASSERT(gatePtr->GetGateType() == GateType::NJSValue());
    ASSERT(gatePtr->GetOpCode() == OpCode::CONSTANT);
    ASSERT(gatePtr->GetMachineType() == MachineType::I64);
    size_t imm = gatePtr->GetOneParameterMetaData()->GetValue();
    return imm;
}

void GateAccessor::SetDep(GateRef gate, GateRef depGate, size_t idx)
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    ASSERT(idx < gatePtr->GetDependCount());
    size_t dependIndex = gatePtr->GetStateCount();
    gatePtr->ModifyIn(dependIndex + idx, circuit_->LoadGatePtr(depGate));
}

UseIterator GateAccessor::ReplaceIn(const UseIterator &useIt, GateRef replaceGate)
{
    UseIterator next = useIt;
    next++;
    Gate *curGatePtr = circuit_->LoadGatePtr(*useIt);
    Gate *replaceGatePtr = circuit_->LoadGatePtr(replaceGate);
    curGatePtr->ModifyIn(useIt.GetIndex(), replaceGatePtr);
    return next;
}

GateType GateAccessor::GetGateType(GateRef gate) const
{
    return circuit_->LoadGatePtr(gate)->GetGateType();
}

void GateAccessor::SetGateType(GateRef gate, GateType gt)
{
    circuit_->LoadGatePtr(gate)->SetGateType(gt);
}

UseIterator GateAccessor::ReplaceHirIfSuccess(const UseIterator &useIt, GateRef state)
{
    ASSERT(GetOpCode(*useIt) == OpCode::IF_SUCCESS);
    auto uses = Uses(*useIt);
    for (auto it = uses.begin(); it != uses.end();) {
        if (IsStateIn(it)) {
            it = ReplaceIn(it, state);
        }
    }
    auto next = DeleteGate(useIt);
    return next;
}

UseIterator GateAccessor::ReplaceHirIfException(const UseIterator &useIt, StateDepend replacement)
{
    ASSERT(GetOpCode(*useIt) == OpCode::IF_EXCEPTION);
    auto uses = Uses(*useIt);
    for (auto it = uses.begin(); it != uses.end();) {
        if (IsStateIn(it)) {
            it = ReplaceIn(it, replacement.State());
        } else if (IsDependIn(it)) {
            it = ReplaceIn(it, replacement.Depend());
        } else {
            ASSERT(!IsValueIn(it));
        }
    }
    UseIterator next = useIt;
    next++;
    return next;
}

void GateAccessor::ExceptionReturn(GateRef state, GateRef depend)
{
    CircuitBuilder builder(circuit_);
    auto constant = builder.ExceptionConstant();
    builder.Return(state, depend, constant);
}

void GateAccessor::ReplaceHirWithIfBranch(GateRef hirGate, StateDepend success,
                                          StateDepend exception, GateRef value)
{
    auto uses = Uses(hirGate);
    GateRef ifException = Circuit::NullGate();
    for (auto it = uses.begin(); it != uses.end();) {
        if (IsStateIn(it)) {
            const OpCode op = GetOpCode(*it);
            if (op == OpCode::IF_SUCCESS) {
                it = ReplaceHirIfSuccess(it, success.State());
            } else if (op == OpCode::IF_EXCEPTION) {
                ifException = *it;
                it = ReplaceHirIfException(it, exception);
            } else if (GetMetaData(*it)->IsVirtualState()) {
                it = ReplaceIn(it, success.State());
            } else {
                ExceptionReturn(exception.State(), exception.Depend());
                it = ReplaceIn(it, success.State());
            }
        } else if (IsDependIn(it)) {
            const OpCode op = GetOpCode(*it);
            if (op == OpCode::IF_EXCEPTION) {
                // ignore it now.
                it++;
            } else {
                it = ReplaceIn(it, success.Depend());
            }
        } else {
            ASSERT(IsValueIn(it));
            it = ReplaceIn(it, value);
        }
    }

    if (ifException != Circuit::NullGate()) {
        DeleteGate(ifException);
    }

    // delete old gate
    DeleteGate(hirGate);
}

void GateAccessor::ReplaceHirDirectly(GateRef hirGate,
    StateDepend replacement, GateRef value)
{
    auto uses = Uses(hirGate);
    for (auto it = uses.begin(); it != uses.end();) {
        if (IsStateIn(it)) {
            ASSERT(GetOpCode(*it) != OpCode::IF_SUCCESS &&
                GetOpCode(*it) != OpCode::IF_EXCEPTION);
            it = ReplaceIn(it, replacement.State());
        } else if (IsDependIn(it)) {
            it = ReplaceIn(it, replacement.Depend());
        } else {
            ASSERT(IsValueIn(it));
            it = ReplaceIn(it, value);
        }
    }

    // delete old gate
    DeleteGate(hirGate);
}

void GateAccessor::ReplaceHirAndDeleteIfException(GateRef hirGate,
    StateDepend replacement, GateRef value)
{
    if (value != Circuit::NullGate()) {
        auto type = GetGateType(hirGate);
        if (!type.IsAnyType()) {
            SetGateType(value, type);
        }
    }
    GateRef ifException = Circuit::NullGate();
    auto uses = Uses(hirGate);
    for (auto it = uses.begin(); it != uses.end();) {
        if (IsStateIn(it)) {
            const OpCode op = GetOpCode(*it);
            if (op == OpCode::IF_SUCCESS) {
                it = ReplaceHirIfSuccess(it, replacement.State());
            } else if (op == OpCode::IF_EXCEPTION) {
                ifException = *it;
                it = ReplaceIn(it, circuit_->DeadGate());
            } else {
                it = ReplaceIn(it, replacement.State());
            }
        } else if (IsDependIn(it)) {
            const OpCode op = GetOpCode(*it);
            if (op == OpCode::IF_EXCEPTION) {
                it = ReplaceIn(it, circuit_->DeadGate());
            } else {
                it = ReplaceIn(it, replacement.Depend());
            }
        } else {
            ASSERT(IsValueIn(it));
            it = ReplaceIn(it, value);
        }
    }

    // delete old gate
    DeleteGate(hirGate);
    if (ifException != Circuit::NullGate()) {
        GraphEditor::RemoveDeadState(circuit_, ifException);
    }
}

void GateAccessor::EliminateRedundantPhi()
{
    GraphEditor::EliminateRedundantPhi(circuit_);
}

UseIterator GateAccessor::DeleteGate(const UseIterator &useIt)
{
    auto next = useIt;
    next++;
    circuit_->DeleteGate(*useIt);
    return next;
}

void GateAccessor::DecreaseIn(const UseIterator &useIt)
{
    size_t idx = useIt.GetIndex();
    circuit_->DecreaseIn(*useIt, idx);
}


void GateAccessor::DecreaseIn(GateRef gate, size_t index)
{
    circuit_->DecreaseIn(gate, index);
}

void GateAccessor::NewIn(GateRef gate, size_t idx, GateRef in)
{
    circuit_->NewIn(gate, idx, in);
}

size_t GateAccessor::GetStateCount(GateRef gate) const
{
    return circuit_->LoadGatePtr(gate)->GetStateCount();
}

size_t GateAccessor::GetDependCount(GateRef gate) const
{
    return circuit_->LoadGatePtr(gate)->GetDependCount();
}

size_t GateAccessor::GetInValueCount(GateRef gate) const
{
    return circuit_->LoadGatePtr(gate)->GetInValueCount();
}

void GateAccessor::UpdateAllUses(GateRef oldIn, GateRef newIn)
{
    if (oldIn == newIn) {
        return;
    }
    auto uses = Uses(oldIn);
    for (auto useIt = uses.begin(); useIt != uses.end();) {
        useIt = ReplaceIn(useIt, newIn);
    }
}

void GateAccessor::ReplaceIn(GateRef gate, size_t index, GateRef in)
{
    circuit_->ModifyIn(gate, index, in);
}

void GateAccessor::DeleteIn(GateRef gate, size_t idx)
{
    ASSERT(idx < circuit_->LoadGatePtrConst(gate)->GetNumIns());
    ASSERT(!circuit_->IsInGateNull(gate, idx));
    circuit_->LoadGatePtr(gate)->DeleteIn(idx);
}

void GateAccessor::ReplaceStateIn(GateRef gate, GateRef in, size_t index)
{
    ASSERT(index < GetStateCount(gate));
    circuit_->ModifyIn(gate, index, in);
}

void GateAccessor::ReplaceDependIn(GateRef gate, GateRef in, size_t index)
{
    ASSERT(index < GetDependCount(gate));
    size_t stateCount = GetStateCount(gate);
    circuit_->ModifyIn(gate, stateCount + index, in);
}

void GateAccessor::ReplaceValueIn(GateRef gate, GateRef in, size_t index)
{
    ASSERT(index < GetInValueCount(gate));
    size_t valueStartIndex = GetInValueStarts(gate);
    circuit_->ModifyIn(gate, valueStartIndex + index, in);
}

void GateAccessor::DeleteGate(GateRef gate)
{
    circuit_->DeleteGate(gate);
}

MachineType GateAccessor::GetMachineType(GateRef gate) const
{
    return circuit_->GetMachineType(gate);
}

void GateAccessor::SetMachineType(GateRef gate, MachineType type)
{
    circuit_->SetMachineType(gate, type);
}

GateRef GateAccessor::GetConstantGate(MachineType bitValue, BitField bitfield, GateType type) const
{
    return circuit_->GetConstantGate(bitValue, bitfield, type);
}

GateRef GateAccessor::GetInitialEnvGate(GateRef jsFunc) const
{
    return circuit_->GetInitialEnvGate(jsFunc);
}

bool GateAccessor::IsConstantNumber(GateRef gate) const
{
    DISALLOW_GARBAGE_COLLECTION;
    if (GetGateType(gate).IsNJSValueType() ||
        (GetOpCode(gate) != OpCode::CONSTANT)) {
        return false;
    }
    JSTaggedValue value(GetConstantValue(gate));
    return value.IsNumber();
}

double GateAccessor::GetFloat64FromConstant(GateRef gate) const
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT(GetOpCode(gate) == OpCode::CONSTANT);
    uint64_t rawValue = GetConstantValue(gate);
    if (GetGateType(gate).IsNJSValueType()) {
        ASSERT(GetMachineType(gate) == MachineType::F64);
        return base::bit_cast<double>(rawValue);
    }
    JSTaggedValue value(rawValue);
    return value.GetDouble();
}

int GateAccessor::GetInt32FromConstant(GateRef gate) const
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT(GetOpCode(gate) == OpCode::CONSTANT);
    uint64_t rawValue = GetConstantValue(gate);
    if (GetGateType(gate).IsNJSValueType()) {
        ASSERT(GetMachineType(gate) == MachineType::I32);
        return static_cast<int>(rawValue);
    }
    JSTaggedValue value(rawValue);
    return value.GetInt();
}

bool GateAccessor::IsStateIn(const UseIterator &useIt) const
{
    size_t stateStartIndex = 0;
    size_t stateEndIndex = stateStartIndex + GetStateCount(*useIt);
    size_t index = useIt.GetIndex();
    return (index >= stateStartIndex && index < stateEndIndex);
}

bool GateAccessor::IsDependIn(const UseIterator &useIt) const
{
    size_t dependStartIndex = GetStateCount(*useIt);
    size_t dependEndIndex = dependStartIndex + GetDependCount(*useIt);
    size_t index = useIt.GetIndex();
    return (index >= dependStartIndex && index < dependEndIndex);
}

bool GateAccessor::IsValueIn(const UseIterator &useIt) const
{
    size_t valueStartIndex = GetInValueStarts(*useIt);
    size_t valueEndIndex = valueStartIndex + GetInValueCount(*useIt);
    size_t index = useIt.GetIndex();
    return (index >= valueStartIndex && index < valueEndIndex);
}

bool GateAccessor::IsFrameStateIn(const UseIterator &useIt) const
{
    size_t index = useIt.GetIndex();
    return IsFrameStateIn(*useIt, index);
}

bool GateAccessor::IsStateIn(GateRef gate, size_t index) const
{
    size_t stateStartIndex = 0;
    size_t stateEndIndex = stateStartIndex + GetStateCount(gate);
    return (index >= stateStartIndex && index < stateEndIndex);
}

bool GateAccessor::IsDependIn(GateRef gate, size_t index) const
{
    size_t dependStartIndex = GetStateCount(gate);
    size_t dependEndIndex = dependStartIndex + GetDependCount(gate);
    return (index >= dependStartIndex && index < dependEndIndex);
}

bool GateAccessor::IsValueIn(GateRef gate, size_t index) const
{
    size_t valueStartIndex = GetInValueStarts(gate);
    size_t valueEndIndex = valueStartIndex + GetInValueCount(gate);
    return (index >= valueStartIndex && index < valueEndIndex);
}

bool GateAccessor::IsFrameStateIn(GateRef gate, size_t index) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    size_t frameStateStartIndex = gatePtr->GetInFrameStateStarts();
    size_t FrameStateEndIndex = frameStateStartIndex + gatePtr->GetInFrameStateCount();
    return (index >= frameStateStartIndex && index < FrameStateEndIndex);
}

void GateAccessor::ReplaceGate(GateRef gate, GateRef state, GateRef depend, GateRef value)
{
    if (value != Circuit::NullGate()) {
        GateType type = GetGateType(gate);
        GateType valueType = GetGateType(value);
        if (!type.IsAnyType() && !valueType.IsNJSValueType()) {
            SetGateType(value, type);
        }
    }

    auto uses = Uses(gate);
    for (auto useIt = uses.begin(); useIt != uses.end();) {
        if (IsStateIn(useIt)) {
            ASSERT(state != Circuit::NullGate());
            useIt = ReplaceIn(useIt, state);
        } else if (IsDependIn(useIt)) {
            ASSERT(depend != Circuit::NullGate());
            useIt = ReplaceIn(useIt, depend);
        } else if (IsValueIn(useIt)) {
            ASSERT(value != Circuit::NullGate());
            useIt = ReplaceIn(useIt, value);
        } else {
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
    }
    DeleteGate(gate);
}

GateRef GateAccessor::GetFrameState(GateRef gate) const
{
    ASSERT(HasFrameState(gate));
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    size_t index = gatePtr->GetInFrameStateStarts();
    return circuit_->GetIn(gate, index);
}

GateRef GateAccessor::FindNearestFrameState(GateRef gate) const
{
    auto statesplit = FindNearestStateSplit(gate);
    return GetFrameState(statesplit);
}

GateRef GateAccessor::FindNearestStateSplit(GateRef gate) const
{
    auto statesplit = gate;
    while (GetOpCode(statesplit) != OpCode::STATE_SPLIT) {
        statesplit = GetDep(statesplit);
    }
    return statesplit;
}

bool GateAccessor::HasFrameState(GateRef gate) const
{
    return GetMetaData(gate)->HasFrameState();
}

void GateAccessor::ReplaceFrameStateIn(GateRef gate, GateRef in)
{
    ASSERT(HasFrameState(gate));
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    size_t index = gatePtr->GetInFrameStateStarts();
    circuit_->ModifyIn(gate, index, in);
}

GateRef GateAccessor::GetRoot(OpCode opcode) const
{
    GateRef root = circuit_->GetRoot();
    if (opcode == OpCode::CIRCUIT_ROOT) {
        return root;
    }

    auto uses = ConstUses(root);
    for (auto useIt = uses.begin(); useIt != uses.end(); ++useIt) {
        if (GetOpCode(*useIt) == opcode) {
            return *useIt;
        }
    }
    return Circuit::NullGate();
}

GateRef GateAccessor::GetGlueFromArgList() const
{
    auto argRoot = GetArgRoot();
    ASSERT(static_cast<size_t>(CommonArgIdx::GLUE) == 0);
    const Gate *curGate = circuit_->LoadGatePtrConst(argRoot);

    const Out *curOut = curGate->GetFirstOutConst();
    ASSERT(!curGate->IsFirstOutNull());
    while (!curOut->IsNextOutNull()) {
        curOut = curOut->GetNextOutConst();
    }
    return circuit_->GetGateRef(curOut->GetGateConst());
}

void GateAccessor::GetArgsOuts(std::vector<GateRef>& outs) const
{
    auto argRoot = GetArgRoot();
    GetOuts(argRoot, outs);
}

void GateAccessor::GetReturnOuts(std::vector<GateRef>& outs) const
{
    auto returnRoot = GetReturnRoot();
    GetOuts(returnRoot, outs);
}

const GateMetaData *GateAccessor::GetMetaData(GateRef gate) const
{
    return circuit_->LoadGatePtrConst(gate)->GetMetaData();
}

void GateAccessor::SetMetaData(GateRef gate, const GateMetaData* meta)
{
    return circuit_->LoadGatePtr(gate)->SetMetaData(meta);
}

bool GateAccessor::IsFixed(GateRef g) const
{
    return GetMetaData(g)->IsFixed();
}

bool GateAccessor::IsProlog(GateRef g) const
{
    return GetMetaData(g)->IsProlog();
}

bool GateAccessor::IsCFGMerge(GateRef g) const
{
    return GetMetaData(g)->IsCFGMerge();
}

bool GateAccessor::MetaDataEqu(GateRef g1, GateRef g2) const
{
    return GetMetaData(g1) == GetMetaData(g2);
}

bool GateAccessor::IsNop(GateRef g) const
{
    return GetMetaData(g)->IsNop();
}

bool GateAccessor::IsRoot(GateRef g) const
{
    return GetMetaData(g)->IsRoot();
}

const GateMetaData *ConstGateAccessor::GetMetaData(GateRef g) const
{
    return circuit_->LoadGatePtrConst(g)->GetMetaData();
}

bool ConstGateAccessor::IsFixed(GateRef g) const
{
    return GetMetaData(g)->IsFixed();
}

bool ConstGateAccessor::IsProlog(GateRef g) const
{
    return GetMetaData(g)->IsProlog();
}

bool ConstGateAccessor::IsSchedulable(GateRef g) const
{
    return GetMetaData(g)->IsSchedulable();
}

GateRef GateAccessor::GetDependSelectorFromMerge(GateRef gate)
{
    GateRef depend = Circuit::NullGate();
    auto uses = Uses(gate);
    for (auto useIt = uses.begin(); useIt != uses.end(); useIt++) {
        if (GetOpCode(*useIt) == OpCode::DEPEND_SELECTOR) {
            depend = *useIt;
            break;
        }
    }
    ASSERT(depend != Circuit::NullGate());
    return depend;
}

bool GateAccessor::HasIfExceptionUse(GateRef gate) const
{
    ASSERT(GetStateCount(gate) > 0);
    auto uses = ConstUses(gate);
    for (auto it = uses.begin(); it != uses.end(); it++) {
        if (GetOpCode(*it) == OpCode::IF_EXCEPTION) {
            return true;
        }
    }
    return false;
}

bool GateAccessor::IsHeapObjectFromElementsKind(GateRef gate)
{
    OpCode opcode = GetOpCode(gate);
    if (opcode == OpCode::JS_BYTECODE) {
        auto bc = GetByteCodeOpcode(gate);
        if (bc == EcmaOpcode::LDOBJBYVALUE_IMM8_V8 || bc == EcmaOpcode::LDOBJBYVALUE_IMM16_V8 ||
            bc == EcmaOpcode::LDTHISBYVALUE_IMM8 || bc == EcmaOpcode::LDTHISBYVALUE_IMM16) {
            ElementsKind kind = TryGetElementsKind(gate);
            return Elements::IsObject(kind);
        }
        return false;
    }

    if (opcode == OpCode::LOAD_ELEMENT) {
        TypedLoadOp typedOp = GetTypedLoadOp(gate);
        return typedOp == TypedLoadOp::ARRAY_LOAD_OBJECT_ELEMENT;
    }

    return false;
}

bool GateAccessor::IsLoopBackUse(const UseIterator &useIt) const
{
    if (IsStateIn(useIt)) {
        return (useIt.GetIndex() == 1) && IsLoopHead(*useIt);
    }
    if ((IsValueSelector(*useIt) && IsValueIn(useIt)) ||
        (IsDependSelector(*useIt) && IsDependIn(useIt))) {
        return (useIt.GetIndex() == 2) && IsLoopHead(GetState(*useIt));
    }
    return false;
}
}  // namespace panda::ecmascript::kungfu
