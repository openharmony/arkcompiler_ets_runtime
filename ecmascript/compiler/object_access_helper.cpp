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

#include "ecmascript/compiler/object_access_helper.h"

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/ts_types/ts_type.h"

namespace panda::ecmascript::kungfu {
bool ObjectAccessHelper::Compute(ChunkVector<ObjectAccessInfo> &infos)
{
    ASSERT(compilationEnv_->IsAotCompiler());
    ASSERT(infos.empty());
    bool result = false;
    ObjectAccessInfo info(type_);
    TSTypeKind kind = tsManager_->GetTypeKind(type_.GetGTRef());
    switch (kind) {
        case TSTypeKind::CLASS_INSTANCE:
            result = ComputeForClassInstance(info);
            break;
        case TSTypeKind::CLASS:
        case TSTypeKind::OBJECT:
            result = ComputeForClassOrObject(info);
            break;
        case TSTypeKind::UNION:
            return ComputePolymorphism(infos);
        default:
            return false;
    }

    infos.emplace_back(info);
    return result;
}

bool ObjectAccessHelper::ComputeForClassInstance(ObjectAccessInfo &info)
{
    int hclassIndex = tsManager_->GetHClassIndexByInstanceGateType(info.Type());
    if (hclassIndex == -1) {
        return false;
    }

    JSHClass *hclass = JSHClass::Cast(tsManager_->GetAOTHClassInfoByIndex(hclassIndex).GetTaggedObject());
    if (!hclass->HasTSSubtyping()) {
        return false;
    }

    PropertyLookupResult plr = JSHClass::LookupPropertyInAotHClass(compilationEnv_->GetJSThread(), hclass, key_);
    info.Set(hclassIndex, plr);

    if (IsLoading()) {
        return plr.IsFound();
    }

    return (plr.IsFound() && !plr.IsFunction());
}

bool ObjectAccessHelper::ComputeForClassOrObject(ObjectAccessInfo &info)
{
    GateType type = info.Type();
    int hclassIndex = -1;
    if (tsManager_->IsClassTypeKind(type)) {
        hclassIndex = tsManager_->GetConstructorHClassIndexByClassGateType(type);
    } else if (tsManager_->IsObjectTypeKind(type)) {
        hclassIndex = tsManager_->GetHClassIndexByObjectType(type);
    }

    if (hclassIndex == -1) {
        return false;
    }

    JSHClass *hclass = JSHClass::Cast(tsManager_->GetAOTHClassInfoByIndex(hclassIndex).GetTaggedObject());
    PropertyLookupResult plr = JSHClass::LookupPropertyInAotHClass(compilationEnv_->GetJSThread(), hclass, key_);
    info.Set(hclassIndex, plr);

    if (IsLoading()) {
        return (plr.IsFound() && plr.IsLocal() && !plr.IsAccessor());
    }

    return (plr.IsFound() && plr.IsLocal() && !plr.IsAccessor() && plr.IsWritable());
}

bool ObjectAccessHelper::ComputePolymorphism(ChunkVector<ObjectAccessInfo> &infos)
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT(tsManager_->IsUnionTypeKind(type_));
    JSHandle<TSUnionType> unionType(tsManager_->GetTSType(type_.GetGTRef()));
    TaggedArray *components = TaggedArray::Cast(unionType->GetComponents().GetTaggedObject());
    uint32_t length = components->GetLength();
    for (uint32_t i = 0; i < length; ++i) {
        GlobalTSTypeRef gt(components->Get(i).GetInt());
        GateType type = GateType(gt);
        ObjectAccessInfo info(type);
        TSTypeKind kind = tsManager_->GetTypeKind(gt);
        switch (kind) {
            case TSTypeKind::CLASS_INSTANCE: {
                if (!ComputeForClassInstance(info)) {
                    return false;
                }
                break;
            }
            case TSTypeKind::CLASS:
            case TSTypeKind::OBJECT: {
                if (!ComputeForClassOrObject(info)) {
                    return false;
                }
                break;
            }
            default:
                return false;
        }
        infos.emplace_back(info);
    }

    return infos.size() <= POLYMORPHIC_MAX_SIZE;
}

bool PGOObjectAccessHelper::ComputeForClassInstance(PGOObjectAccessInfo &info)
{
    auto type = info.Type();
    PGOTypeManager *ptManager = compilationEnv_->GetPTManager();
    int hclassIndex = static_cast<int>(ptManager->GetHClassIndexByProfileType(type));
    if (hclassIndex == -1) {
        return false;
    }

    JSHClass *hclass = JSHClass::Cast(ptManager->QueryHClass(type.first, type.second).GetTaggedObject());

    PropertyLookupResult plr = JSHClass::LookupPropertyInPGOHClass(compilationEnv_->GetJSThread(), hclass, key_);
    info.Set(hclassIndex, plr);

    if (IsLoading()) {
        return plr.IsFound();
    }

    return (plr.IsFound() && !plr.IsFunction());
}

bool PGOObjectAccessHelper::ClassInstanceIsCallable(PGOObjectAccessInfo &info)
{
    auto type = info.Type();
    PGOTypeManager *ptManager = compilationEnv_->GetPTManager();
    int hclassIndex = static_cast<int>(ptManager->GetHClassIndexByProfileType(type));
    if (hclassIndex == -1) {
        return false;
    }

    JSHClass *hclass = JSHClass::Cast(ptManager->QueryHClass(type.first, type.second).GetTaggedObject());

    return hclass->IsCallable();
}
}  // namespace panda::ecmascript::kungfu
