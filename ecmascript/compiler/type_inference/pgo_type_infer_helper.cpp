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

#include "ecmascript/compiler/type_inference/pgo_type_infer_helper.h"
#include "ecmascript/ts_types/ts_type_accessor.h"

namespace panda::ecmascript::kungfu {
bool ClassTypeStrategy::CheckAndInsert(CollectedType &types, const GateType &type, TSManager *tsManager)
{
    if (tsManager->IsUserDefinedClassTypeKind(type)) {
        int hclassIndex = tsManager->GetConstructorHClassIndexByClassGateType(type);
        if (hclassIndex == -1) {
            return true;
        }
        types.classTypes.insert(type);
        return true;
    }
    return false;
}

void ClassTypeStrategy::Merge(ChunkSet<GateType> &inferTypes, CollectedType &types,
                              [[maybe_unused]] TSManager *tsManager)
{
    for (GateType type : types.classTypes) {
        inferTypes.insert(type);
    }
}

bool ClassInstanceTypeStrategy::CheckAndInsert(CollectedType &types, const GateType &type, TSManager *tsManager)
{
    if (tsManager->IsClassInstanceTypeKind(type) &&
        tsManager->IsUserDefinedClassTypeKind(tsManager->GetClassType(type))) {
        int hclassIndex = tsManager->GetHClassIndexByInstanceGateType(type);
        if (hclassIndex == -1) {
            return true;
        }
        JSHClass *hclass = JSHClass::Cast(tsManager->GetValueFromCache(hclassIndex).GetTaggedObject());
        if (hclass->HasTSSubtyping()) {
            GlobalTSTypeRef instanceGT = type.GetGTRef();
            GateType classType = GateType(tsManager->GetClassType(instanceGT));
            types.classInstanceTypes.insert(classType);
        }
        return true;
    }
    return false;
}

void ClassInstanceTypeStrategy::Merge(ChunkSet<GateType> &inferTypes, CollectedType &types, TSManager *tsManager)
{
    for (GateType type : types.classInstanceTypes) {
        GlobalTSTypeRef gt = type.GetGTRef();
        GlobalTSTypeRef instanceGT = tsManager->CreateClassInstanceType(gt);
        inferTypes.insert(GateType(instanceGT));
    }
}

bool BuiltinTypeStrategy::CheckAndInsert(CollectedType &types, const GateType &type, TSManager *tsManager)
{
    if (tsManager->IsValidTypedArrayType(type)) {
        types.builtinTypes.insert(type);
        return true;
    }
    return false;
}

void BuiltinTypeStrategy::Merge(ChunkSet<GateType> &inferTypes, CollectedType &types,
                                [[maybe_unused]] TSManager *tsManager)
{
    for (GateType type : types.builtinTypes) {
        inferTypes.insert(type);
    }
}

bool OtherTypeStrategy::CheckAndInsert(CollectedType &types, const GateType &type, [[maybe_unused]]TSManager *tsManager)
{
    if (!type.IsAnyType()) {
        types.otherTypes.insert(type);
        return true;
    }
    return false;
}

void OtherTypeStrategy::Merge(ChunkSet<GateType> &inferTypes, CollectedType &types,
                              [[maybe_unused]] TSManager *tsManager)
{
    for (GateType type : types.otherTypes) {
        inferTypes.insert(type);
    }
}

void PGOTypeInferHelper::CollectGateType(CollectedType &types, GateType tsType, PGORWOpType pgoTypes)
{
    // for static TS uinon type
    if (tsManager_->IsUnionTypeKind(tsType)) {
        JSHandle<TSUnionType> unionType(tsManager_->GetTSType(tsType.GetGTRef()));
        TaggedArray *components = TaggedArray::Cast(unionType->GetComponents().GetTaggedObject());
        uint32_t length = components->GetLength();
        for (uint32_t i = 0; i < length; ++i) {
            GlobalTSTypeRef gt(components->Get(i).GetInt());
            CheckAndInsert(types, GateType(gt));
        }
    } else {
        CheckAndInsert(types, tsType);
    }

    // for pgo type
    for (uint32_t i = 0; i < pgoTypes.GetCount(); i++) {
        ClassType classType = pgoTypes.GetObjectInfo(i).GetClassType();
        GateType pgoType = tsManager_->GetGateTypeByPt(classType);
        if (pgoType.IsAnyType()) {
            tsManager_->AddToSkipTrackFieldSet(classType);
            continue;
        }
        if (tsManager_->IsClassTypeKind(pgoType) && !pgoTypes.GetObjectInfo(i).InConstructor()) {
            pgoType = GateType(tsManager_->CreateClassInstanceType(pgoType));
        }
        CheckAndInsert(types, pgoType);
    }
}

ChunkSet<GateType> PGOTypeInferHelper::GetInferTypes(Chunk *chunk, CollectedType &types, JSTaggedValue prop)
{
    UpdateType(types, prop);
    ChunkSet<GateType> inferTypes(chunk);
    for (auto &strategy : strategies_) {
        strategy->Merge(inferTypes, types, tsManager_);
    }
    return inferTypes;
}

void PGOTypeInferHelper::CheckAndInsert(CollectedType &types, GateType type)
{
    for (auto &strategy : strategies_) {
        if (strategy->CheckAndInsert(types, type, tsManager_)) {
            return;
        }
    }
}

void PGOTypeInferHelper::UpdateType(CollectedType &types, JSTaggedValue prop)
{
    ChunkSet<GateType> &classTypes = types.classTypes;
    InferTypeForClass(classTypes, prop);

    ChunkSet<GateType> &classInstanceTypes = types.classInstanceTypes;
    InferTypeForClass(classInstanceTypes, prop);

    ChunkSet<GateType> &builtinTypes = types.builtinTypes;
    InferTypeForBuiltin(builtinTypes);
}

void PGOTypeInferHelper::InferTypeForClass(ChunkSet<GateType> &types, JSTaggedValue prop)
{
    if (NoNeedUpdate(types)) {
        return;
    }
    EliminateSubclassTypes(types);
    ComputeCommonSuperClassTypes(types, prop);
}

void PGOTypeInferHelper::InferTypeForBuiltin(ChunkSet<GateType> &types)
{
    if (NoNeedUpdate(types)) {
        return;
    }
    std::set<GateType> deletedGates;
    std::set<GlobalTSTypeRef> builtinGTSet;

    for (GateType type : types) {
        GlobalTSTypeRef classGT = tsManager_->GetClassType(type);
        auto it = builtinGTSet.find(classGT);
        if (it != builtinGTSet.end()) {
            deletedGates.insert(type);
            continue;
        }
        builtinGTSet.insert(classGT);
    }
    for (GateType gateType : deletedGates) {
        types.erase(gateType);
    }
}

void PGOTypeInferHelper::EliminateSubclassTypes(ChunkSet<GateType> &types)
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

void PGOTypeInferHelper::ComputeCommonSuperClassTypes(ChunkSet<GateType> &types, JSTaggedValue prop)
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
}  // namespace panda::ecmascript
