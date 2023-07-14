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

struct CollectedType {
    explicit CollectedType(Chunk *chunk) : classTypes(chunk), classInstanceTypes(chunk), otherTypes(chunk) {}

    bool AllInSameKind() const
    {
        uint8_t kind = classTypes.empty() ? 0 : 1;
        kind += classInstanceTypes.empty() ? 0 : 1;
        kind += otherTypes.empty() ? 0 : 1;
        return kind == 1;
    }

    ChunkSet<GateType> Merge(Chunk *chunk, TSManager *tsManager)
    {
        ChunkSet<GateType> inferTypes(chunk);
        for (GateType type : classTypes) {
            inferTypes.insert(type);
        }
        for (GateType type : classInstanceTypes) {
            GlobalTSTypeRef gt = type.GetGTRef();
            GlobalTSTypeRef instanceGT = tsManager->CreateClassInstanceType(gt);
            inferTypes.insert(GateType(instanceGT));
        }
        for (GateType type : otherTypes) {
            inferTypes.insert(type);
        }
        return inferTypes;
    }

    ChunkSet<GateType> classTypes;
    ChunkSet<GateType> classInstanceTypes;
    ChunkSet<GateType> otherTypes;
};

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
        default:
            break;
    }
}

void PGOTypeInfer::CheckAndInsert(CollectedType &types, GateType type)
{
    if (tsManager_->IsClassTypeKind(type)) {
        int hclassIndex = tsManager_->GetConstructorHClassIndexByClassGateType(type);
        if (hclassIndex == -1) {
            return;
        }
        types.classTypes.insert(type);
    } else if (tsManager_->IsClassInstanceTypeKind(type)) {
        int hclassIndex = tsManager_->GetHClassIndexByInstanceGateType(type);
        if (hclassIndex == -1) {
            return;
        }
        JSHClass *hclass = JSHClass::Cast(tsManager_->GetHClassFromCache(hclassIndex).GetTaggedObject());
        if (hclass->HasTSSubtyping()) {
            GlobalTSTypeRef instanceGT = type.GetGTRef();
            type = GateType(tsManager_->GetClassType(instanceGT));
            types.classInstanceTypes.insert(type);
        }
    } else if (!type.IsAnyType()) {
        types.otherTypes.insert(type);
    }
}

void PGOTypeInfer::CollectGateType(CollectedType &types, GateType tsType, PGORWOpType pgoTypes)
{
    CheckAndInsert(types, tsType);

    for (uint32_t i = 0; i < pgoTypes.GetCount(); i++) {
        ClassType classType = pgoTypes.GetObjectInfo(i).GetClassType();
        GateType pgoType = tsManager_->GetGateTypeByPt(classType);
        if (tsManager_->IsClassTypeKind(pgoType) && !pgoTypes.GetObjectInfo(i).InConstructor()) {
            pgoType = GateType(tsManager_->CreateClassInstanceType(pgoType));
        }
        CheckAndInsert(types, pgoType);
    }

    // for static TS uinon type
    if (tsManager_->IsUnionTypeKind(tsType)) {
        JSHandle<TSUnionType> unionType(tsManager_->GetTSType(tsType.GetGTRef()));
        TaggedArray *components = TaggedArray::Cast(unionType->GetComponents().GetTaggedObject());
        uint32_t length = components->GetLength();
        for (uint32_t i = 0; i < length; ++i) {
            GlobalTSTypeRef gt(components->Get(i).GetInt());
            CheckAndInsert(types, GateType(gt));
        }
    }
}

void PGOTypeInfer::UpdateType(CollectedType &types, JSTaggedValue prop)
{
    ChunkSet<GateType> &classTypes = types.classTypes;
    InferTypeForClass(classTypes, prop);

    ChunkSet<GateType> &classInstanceTypes = types.classInstanceTypes;
    InferTypeForClass(classInstanceTypes, prop);
}

void PGOTypeInfer::InferTypeForClass(ChunkSet<GateType> &types, JSTaggedValue prop)
{
    if (NoNeedUpdate(types)) {
        return;
    }
    EliminateSubclassTypes(types);
    ComputeCommonSuperClassTypes(types, prop);
}

void PGOTypeInfer::EliminateSubclassTypes(ChunkSet<GateType> &types)
{
    std::set<GateType> deletedGates;
    for (GateType type : types) {
        if (deletedGates.find(type) != deletedGates.end()) {
            continue;
        }

        std::stack<GateType> ancestors;
        GateType curType = type;
        do {
            if (types.find(curType) != types.end()) {
                ancestors.push(curType);
            }
        } while (tsManager_->GetSuperGateType(curType));

        ancestors.pop(); // top type is alive
        while (!ancestors.empty()) {
            curType = ancestors.top();
            ancestors.pop();
            auto it = deletedGates.find(curType);
            if (it != deletedGates.end()) {
                deletedGates.insert(curType);
            }
        }
    }
    for (GateType gateType : deletedGates) {
        types.erase(gateType);
    }
}

void PGOTypeInfer::ComputeCommonSuperClassTypes(ChunkSet<GateType> &types, JSTaggedValue prop)
{
    JSThread *thread = tsManager_->GetEcmaVM()->GetJSThread();
    std::map<GateType, GateType> removeTypes;
    for (GateType type : types) {
        GateType curType = type;
        GateType preType = type;
        while (tsManager_->GetSuperGateType(curType)) {
            JSHClass *ihc = JSHClass::Cast(tsManager_->GetTSHClass(curType).GetTaggedObject());
            PropertyLookupResult plr = JSHClass::LookupPropertyInAotHClass(thread, ihc, prop);
            if (!plr.IsFound()) {
                break;
            }
            preType = curType;
        }
        if (type != preType) {
            removeTypes[type] = preType;
        }
    }

    for (auto item : removeTypes) {
        types.erase(item.first);
        types.insert(item.second);
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

void PGOTypeInfer::UpdateTypeForRWOp(GateRef gate, GateRef receiver, JSTaggedValue prop)
{
    GateType tsType = acc_.GetGateType(receiver);
    PGORWOpType pgoTypes = builder_->GetPGOType(gate);
    CollectedType types(chunk_);
    CollectGateType(types, tsType, pgoTypes);

    // polymorphism is not currently supported,
    // all types must in the same kind
    if (!types.AllInSameKind()) {
        return;
    }

    // polymorphism is not currently supported
    UpdateType(types, prop);
    ChunkSet<GateType> inferTypes = types.Merge(chunk_, tsManager_);
    if (!IsMonoTypes(inferTypes)) {
        return;
    }

    AddProfiler(gate, tsType, pgoTypes, inferTypes);
    acc_.SetGateType(receiver, *inferTypes.begin());
}
}  // namespace panda::ecmascript
