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
        default:
            break;
    }
}

ChunkSet<GateType> PGOTypeInfer::UpdateType(GateType tsType, PGORWOpType pgoType, JSTaggedValue prop)
{
    ChunkSet<GateType> inferTypes(chunk_);
    CollectGateType(inferTypes, tsType, pgoType);

    if (NoNeedUpdate(inferTypes)) {
        return inferTypes;
    }

    EliminateSubclassTypes(inferTypes);
    ComputeCommonSuperClassTypes(inferTypes, prop);

    return inferTypes;
}

void PGOTypeInfer::CheckAndInsert(ChunkSet<GateType> &types, GateType type)
{
    if (tsManager_->IsUserDefinedClassTypeKind(type)) {
        int hclassIndex = tsManager_->GetHClassIndexByClassGateType(type);
        if (hclassIndex == -1) {
            return;
        }
        JSHClass *hclass = JSHClass::Cast(tsManager_->GetHClassFromCache(hclassIndex).GetTaggedObject());
        if (hclass->HasTSSubtyping()) {
            types.insert(type);
        }
    }
}

void PGOTypeInfer::CollectGateType(ChunkSet<GateType> &types, GateType tsType, PGORWOpType pgoTypes)
{
    CheckAndInsert(types, tsType);

    for (int i = 0; i < pgoTypes.GetCount(); i++) {
        ClassType classType = pgoTypes.GetObjectInfo(i).GetClassType();
        const GateType pgoType = tsManager_->GetGateTypeByPt(classType);
        CheckAndInsert(types, pgoType);
    }
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

        if (tsManager_->IsClassTypeKind(value.tsType)) {
            log += "TSType: " + TSTypeAccessor(tsManager_, value.tsType).GetClassTypeName();
        } else {
            log += "TSType: Any";
        }
        log += ", PGOTypes: [ ";
        for (auto pgoType : value.pgoTypes) {
            log += TSTypeAccessor(tsManager_, pgoType).GetClassTypeName() + ", ";
        }
        log += "], InferTypes: [ ";
        for (auto type : value.inferTypes) {
            log += TSTypeAccessor(tsManager_, type).GetClassTypeName() + ", ";
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
        for (int i = 0; i < pgoType.GetCount(); i++) {
            value.pgoTypes.emplace_back(tsManager_->GetGateTypeByPt(pgoType.GetObjectInfo(i).GetClassType()));
        }
        for (GateType type : inferTypes) {
            value.inferTypes.emplace_back(type);
        }
        profiler_.datas.emplace_back(value);
    }
}

bool PGOTypeInfer::CheckPGOType(PGORWOpType pgoTypes, RWOpLoc &rwOpLoc) const
{
    uint32_t count = pgoTypes.GetCount();
    for (uint32_t i = 0; i < count; ++i) {
        if (rwOpLoc == RWOpLoc::UNKONWN) {
            if (pgoTypes.GetObjectInfo(i).InConstructor()) {
                rwOpLoc = RWOpLoc::CONSTRUCTOR;
            } else {
                rwOpLoc = RWOpLoc::INSTANCE;
            }
            continue;
        }
        if ((rwOpLoc == RWOpLoc::CONSTRUCTOR && pgoTypes.GetObjectInfo(i).InConstructor()) ||
            (rwOpLoc == RWOpLoc::INSTANCE && !pgoTypes.GetObjectInfo(i).InConstructor())){
            continue;
        } else {
            return false;
        }
    }
    return true;
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

void PGOTypeInfer::UpdateTypeForRWOp(GateRef gate, GateRef receiver, JSTaggedValue prop)
{
    GateType tsType = acc_.GetGateType(receiver);
    RWOpLoc rwOpLoc = RWOpLoc::UNKONWN;
    if (tsManager_->IsClassTypeKind(tsType)) {
        rwOpLoc = RWOpLoc::CONSTRUCTOR;
    } else if (tsManager_->IsClassInstanceTypeKind(tsType)){
        rwOpLoc = RWOpLoc::INSTANCE;
        GlobalTSTypeRef instanceGT = tsType.GetGTRef();
        tsType = GateType(tsManager_->GetClassType(instanceGT));
    }
    PGORWOpType pgoType = builder_->GetPGOType(gate);
    if (!CheckPGOType(pgoType, rwOpLoc) || rwOpLoc == RWOpLoc::UNKONWN) {
        return;
    }
    ChunkSet<GateType> inferTypes = UpdateType(tsType, pgoType, prop);
    if (!IsMonoTypes(inferTypes)) {
        return;
    }

    AddProfiler(gate, tsType, pgoType, inferTypes);
    GateType inferType;
    if (rwOpLoc == RWOpLoc::CONSTRUCTOR) {
        inferType = *inferTypes.begin();
    } else {
        GlobalTSTypeRef gt = inferTypes.begin()->GetGTRef();
        GlobalTSTypeRef instanceGT = tsManager_->CreateClassInstanceType(gt);
        inferType = GateType(instanceGT);
    }
    acc_.SetGateType(receiver, inferType);
}
}  // namespace panda::ecmascript
