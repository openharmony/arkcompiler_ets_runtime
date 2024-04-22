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

#include "ecmascript/compiler/type_inference/pgo_type_infer.h"
#include "ecmascript/compiler/bytecode_circuit_builder.h"

namespace panda::ecmascript::kungfu {
void PGOTypeInfer::Run()
{
    std::vector<GateRef> gateList;
    circuit_->GetAllGates(gateList);
    for (const auto &gate : gateList) {
        auto op = acc_.GetOpCode(gate);
        if (op == OpCode::JS_BYTECODE) {
            RunTypeInfer(gate);
        }
    }
}

void PGOTypeInfer::RunTypeInfer(GateRef gate)
{
    ASSERT(acc_.GetOpCode(gate) == OpCode::JS_BYTECODE);
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    switch (ecmaOpcode) {
        case EcmaOpcode::LDOBJBYNAME_IMM8_ID16:
        case EcmaOpcode::LDOBJBYNAME_IMM16_ID16:
        case EcmaOpcode::LDTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::LDTHISBYNAME_IMM16_ID16:
            InferLdObjByName(gate);
            break;
        case EcmaOpcode::STOWNBYINDEX_IMM8_V8_IMM16:
        case EcmaOpcode::STOWNBYINDEX_IMM16_V8_IMM16:
            InferStOwnByIndex(gate);
            break;
        case EcmaOpcode::LDOBJBYVALUE_IMM8_V8:
        case EcmaOpcode::LDOBJBYVALUE_IMM16_V8:
        case EcmaOpcode::LDTHISBYVALUE_IMM8:
        case EcmaOpcode::LDTHISBYVALUE_IMM16:
        case EcmaOpcode::STOBJBYVALUE_IMM8_V8_V8:
        case EcmaOpcode::STOBJBYVALUE_IMM16_V8_V8:
        case EcmaOpcode::STTHISBYVALUE_IMM8_V8:
        case EcmaOpcode::STTHISBYVALUE_IMM16_V8:
            InferAccessObjByValue(gate);
            break;
        case EcmaOpcode::CREATEEMPTYARRAY_IMM8:
        case EcmaOpcode::CREATEEMPTYARRAY_IMM16:
        case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
        case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM16_ID16:
            InferCreateArray(gate);
            break;
        default:
            break;
    }
}

void PGOTypeInfer::InferLdObjByName(GateRef gate)
{
    if (!builder_->ShouldPGOTypeInfer(gate)) {
        return;
    }

    TrySetElementsKind(gate);
}

void PGOTypeInfer::InferStOwnByIndex(GateRef gate)
{
    if (!builder_->ShouldPGOTypeInfer(gate)) {
        return;
    }
    TrySetElementsKind(gate);
    TrySetTransitionElementsKind(gate);
    TrySetOnHeapMode(gate);
}

void PGOTypeInfer::InferCreateArray(GateRef gate)
{
    if (!builder_->ShouldPGOTypeInfer(gate)) {
        return;
    }
    auto length = builder_->GetArrayElementsLength(gate);
    acc_.TrySetArrayElementsLength(gate, length);
    ElementsKind kind = builder_->GetElementsKindForCreater(gate);
    if (Elements::IsGeneric(kind)) {
        return;
    }

    acc_.TrySetElementsKind(gate, kind);
}

void PGOTypeInfer::InferAccessObjByValue(GateRef gate)
{
    if (!builder_->ShouldPGOTypeInfer(gate)) {
        return;
    }
    GateRef propKey = Circuit::NullGate();
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    switch (ecmaOpcode) {
        case EcmaOpcode::LDOBJBYVALUE_IMM8_V8:
        case EcmaOpcode::LDOBJBYVALUE_IMM16_V8:
        case EcmaOpcode::STOBJBYVALUE_IMM8_V8_V8:
        case EcmaOpcode::STOBJBYVALUE_IMM16_V8_V8:
            propKey = acc_.GetValueIn(gate, 2);  // 2: the third parameter
            break;
        case EcmaOpcode::LDTHISBYVALUE_IMM8:
        case EcmaOpcode::LDTHISBYVALUE_IMM16:
        case EcmaOpcode::STTHISBYVALUE_IMM8_V8:
        case EcmaOpcode::STTHISBYVALUE_IMM16_V8:
            propKey = acc_.GetValueIn(gate, 1);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    TrySetPropKeyKind(gate, propKey);
    TrySetElementsKind(gate);
    TrySetTransitionElementsKind(gate);
    TrySetOnHeapMode(gate);
}

void PGOTypeInfer::TrySetElementsKind(GateRef gate)
{
    auto kinds = builder_->GetElementsKindsForUser(gate);
    if (kinds.empty()) {
        return;
    }
    for (auto kind : kinds) {
        acc_.TrySetElementsKind(gate, kind);
    }
}

void PGOTypeInfer::TrySetTransitionElementsKind(GateRef gate)
{
    // The kinds array here has [transitioned elementsKind] stored.
    auto kinds = builder_->GetTransitionElementsKindsForUser(gate);
    if (kinds.empty()) {
        return;
    }
    for (auto kind : kinds) {
        acc_.TrySetTransitionElementsKind(gate, kind);
    }
}

void PGOTypeInfer::TrySetPropKeyKind(GateRef gate, GateRef propKey)
{
    auto pgoTypes = acc_.TryGetPGOType(gate);
    const PGORWOpType *pgoRwTypes = pgoTypes.GetPGORWOpType();
    GateType oldType = acc_.GetGateType(propKey);
    if (oldType.IsAnyType() && IsMonoNumberType(*pgoRwTypes)) {
        acc_.SetGateType(propKey, GateType::NumberType());
    }
}

void PGOTypeInfer::TrySetOnHeapMode(GateRef gate)
{
    PGOTypeRef pgoTypes = acc_.TryGetPGOType(gate);
    const PGORWOpType *pgoRwTypes = pgoTypes.GetPGORWOpType();
    std::vector<OnHeapMode> modes;

    for (uint32_t i = 0; i < pgoRwTypes->GetCount(); i++) {
        ProfileType profileType = pgoRwTypes->GetObjectInfo(i).GetProfileType();
        if (profileType.IsBuiltinsType()) {
            ProfileType::BuiltinsTypedArrayId id(profileType.GetId());
            modes.emplace_back(id.GetOnHeapMode());
        }
    }

    if (modes.empty()) {
        return;
    }

    // monomorphic
    if (modes.size() == 1) {
        acc_.TrySetOnHeapMode(gate, modes[0]);
        return;
    }

    // polymorphism
    // If the modes can be merged into monomorphic, still optimize it. Otherwise degrade it to NONE.
    OnHeapMode mode = modes[0];
    for (uint32_t i = 1; i < modes.size(); i++) {
        mode = OnHeap::Merge(mode, modes[i]);
        if (OnHeap::IsNone(mode)) {
            break;
        }
    }
    acc_.TrySetOnHeapMode(gate, mode);
}
}  // namespace panda::ecmascript
