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
#include "ecmascript/ts_types/ts_type_accessor.h"
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

    if (IsLogEnabled()) {
        Print();
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
        case EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STOBJBYNAME_IMM16_ID16_V8:
            InferStObjByName(gate, false);
            break;
        case EcmaOpcode::STTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::STTHISBYNAME_IMM16_ID16:
            InferStObjByName(gate, true);
            break;
        case EcmaOpcode::STOWNBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STOWNBYNAME_IMM16_ID16_V8:
            InferStOwnByName(gate);
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

void PGOTypeInfer::Print() const
{
    LOG_COMPILER(INFO) << " ";
    LOG_COMPILER(INFO) << "\033[34m" << "================="
                       << " After pgo type infer "
                       << "[" << GetMethodName() << "] "
                       << "=================" << "\033[0m";

    for (const auto &value : profiler_.datas) {
        std::string log("\"id\"=" + std::to_string(acc_.GetId(value.gate)) +
                        ", \"bytecode\"=\"" + GetEcmaOpcodeStr(acc_.GetByteCodeOpcode(value.gate)) + "\", ");
        log += "TSType: [type:" + tsManager_->GetTypeStr(value.tsType) + ", "
                                + "moduleId: " + std::to_string(value.tsType.GetGTRef().GetModuleId()) + ", "
                                + "localId: " + std::to_string(value.tsType.GetGTRef().GetLocalId()) + "], ";
        log += "PGOTypes: [";
        for (auto pgoType : value.pgoTypes) {
            log += "[type:" + tsManager_->GetTypeStr(pgoType) + ", "
                            + "moduleId: " + std::to_string(pgoType.GetGTRef().GetModuleId()) + ", "
                            + "localId: " + std::to_string(pgoType.GetGTRef().GetLocalId()) + "], ";
        }
        log += "] InferTypes: [";
        for (auto type : value.inferTypes) {
            log += "[type:" + tsManager_->GetTypeStr(type) + ", "
                            + "moduleId: " + std::to_string(type.GetGTRef().GetModuleId()) + ", "
                            + "localId: " + std::to_string(type.GetGTRef().GetLocalId()) + "]";
        }
        log += "]";
        LOG_COMPILER(INFO) << std::dec << log;
    }
    LOG_COMPILER(INFO) << "\033[34m" << "=========================== End ===========================" << "\033[0m";
}

void PGOTypeInfer::AddProfiler(GateRef gate, GateType tsType, PGORWOpType pgoType, ChunkSet<GateType>& inferTypes)
{
    if (enableLog_) {
        Profiler::Value value;
        value.gate = gate;
        value.tsType = tsType;
        for (uint32_t i = 0; i < pgoType.GetCount(); i++) {
            value.pgoTypes.emplace_back(tsManager_->GetGateTypeByPt(pgoType.GetObjectInfo(i).GetClassType()));
        }
        for (GateType type : inferTypes) {
            value.inferTypes.emplace_back(type);
        }
        profiler_.datas.emplace_back(value);
    }
}

void PGOTypeInfer::InferLdObjByName(GateRef gate)
{
    if (!builder_->ShouldPGOTypeInfer(gate)) {
        return;
    }
    GateRef constData = acc_.GetValueIn(gate, 1); // 1: valueIn 1
    uint16_t propIndex = acc_.GetConstantValue(constData);
    JSTaggedValue prop = tsManager_->GetStringFromConstantPool(propIndex);
    GateRef receiver = acc_.GetValueIn(gate, 2); // 2: acc or this object

    UpdateTypeForRWOp(gate, receiver, prop);
}

void PGOTypeInfer::InferStObjByName(GateRef gate, bool isThis)
{
    if (!builder_->ShouldPGOTypeInfer(gate)) {
        return;
    }
    GateRef constData = acc_.GetValueIn(gate, 1); // 1: valueIn 1
    uint16_t propIndex = acc_.GetConstantValue(constData);
    JSTaggedValue prop = tsManager_->GetStringFromConstantPool(propIndex);
    GateRef receiver = Circuit::NullGate();
    if (isThis) {
        // 3: number of value inputs
        ASSERT(acc_.GetNumValueIn(gate) == 3);
        receiver = argAcc_.GetFrameArgsIn(gate, FrameArgIdx::THIS_OBJECT);
    } else {
        // 4: number of value inputs
        ASSERT(acc_.GetNumValueIn(gate) == 4);
        receiver = acc_.GetValueIn(gate, 2); // 2: receiver
    }

    UpdateTypeForRWOp(gate, receiver, prop);
}

void PGOTypeInfer::InferStOwnByName(GateRef gate)
{
    if (!builder_->ShouldPGOTypeInfer(gate)) {
        return;
    }
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef constData = acc_.GetValueIn(gate, 0);
    uint16_t propIndex = acc_.GetConstantValue(constData);
    JSTaggedValue prop = tsManager_->GetStringFromConstantPool(propIndex);
    GateRef receiver = acc_.GetValueIn(gate, 1);

    UpdateTypeForRWOp(gate, receiver, prop);
}

void PGOTypeInfer::InferCreateArray(GateRef gate)
{
    if (!builder_->ShouldPGOTypeInfer(gate)) {
        return;
    }

    ElementsKind kind = builder_->GetArrayElementsKind(gate);
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
    GateRef receiver = Circuit::NullGate();
    GateRef propKey = Circuit::NullGate();
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    switch (ecmaOpcode) {
        case EcmaOpcode::LDOBJBYVALUE_IMM8_V8:
        case EcmaOpcode::LDOBJBYVALUE_IMM16_V8:
        case EcmaOpcode::STOBJBYVALUE_IMM8_V8_V8:
        case EcmaOpcode::STOBJBYVALUE_IMM16_V8_V8:
            receiver = acc_.GetValueIn(gate, 1);  // 1: receiver
            propKey = acc_.GetValueIn(gate, 2);  // 2: the third parameter
            break;
        case EcmaOpcode::LDTHISBYVALUE_IMM8:
        case EcmaOpcode::LDTHISBYVALUE_IMM16:
        case EcmaOpcode::STTHISBYVALUE_IMM8_V8:
        case EcmaOpcode::STTHISBYVALUE_IMM16_V8:
            receiver = argAcc_.GetFrameArgsIn(gate, FrameArgIdx::THIS_OBJECT);
            propKey = acc_.GetValueIn(gate, 1);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    UpdateTypeForRWOp(gate, receiver);
    TrySetPropKeyKind(gate, propKey);
    TrySetElementsKind(gate);
}

void PGOTypeInfer::UpdateTypeForRWOp(GateRef gate, GateRef receiver, JSTaggedValue prop)
{
    GateType tsType = acc_.GetGateType(receiver);
    PGORWOpType pgoTypes = builder_->GetPGOType(gate);

    CollectedType types(chunk_);
    helper_.CollectGateType(types, tsType, pgoTypes);

    // polymorphism is not currently supported,
    // all types must in the same kind
    if (!types.AllInSameKind()) {
        return;
    }

    // polymorphism is not currently supported
    ChunkSet<GateType> inferTypes = helper_.GetInferTypes(chunk_, types, prop);
    AddProfiler(gate, tsType, pgoTypes, inferTypes);
    if (!IsMonoTypes(inferTypes)) {
        return;
    }

    acc_.SetGateType(receiver, *inferTypes.begin());
}

void PGOTypeInfer::TrySetElementsKind(GateRef gate)
{
    ElementsKind kind = builder_->GetElementsKind(gate);
    if (Elements::IsGeneric(kind)) {
        return;
    }
    acc_.TrySetElementsKind(gate, kind);
}

void PGOTypeInfer::TrySetPropKeyKind(GateRef gate, GateRef propKey)
{
    PGORWOpType pgoTypes = builder_->GetPGOType(gate);
    GateType oldType = acc_.GetGateType(propKey);
    if (oldType.IsAnyType() && IsMonoNumberType(pgoTypes)) {
        acc_.SetGateType(propKey, GateType::NumberType());
    }
}
}  // namespace panda::ecmascript
