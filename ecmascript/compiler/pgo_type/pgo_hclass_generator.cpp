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

#include "ecmascript/compiler/pgo_type/pgo_hclass_generator.h"

namespace panda::ecmascript::kungfu {
bool PGOHClassGenerator::IsCache() const
{
    return status_ == Status::ISCACHE;
}

bool PGOHClassGenerator::IsPreprocessObjectLiteralLength() const
{
    return status_ == Status::PREPROCESSOR;
}

void PGOHClassGenerator::SetStatus(Status status) const
{
    status_ = status;
}

bool PGOHClassGenerator::FindHClassLayoutDesc(PGOSampleType type) const
{
    PGOHClassTreeDesc *desc;
    typeRecorder_.GetHClassTreeDesc(type, &desc);
    if (desc == nullptr) {
        return false;
    }
    if (desc->GetHClassLayoutDesc(type.GetProfileType()) == nullptr) {
        return false;
    }
    return true;
}

bool PGOHClassGenerator::GenerateHClass(PGOSampleType type) const
{
    auto thread = ptManager_->GetJSThread();
    JSHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    PGOHClassTreeDesc *desc;
    if (!typeRecorder_.GetHClassTreeDesc(type, &desc)) {
        return false;
    }

    if (desc->CheckHasInvalidType()) {
        return false;
    }

    auto rootType = type.GetProfileType();
    auto rootHClassDesc = desc->GetHClassLayoutDesc(rootType);
    if (rootHClassDesc == nullptr) {
        return false;
    }

    uint32_t rootNumOfProps = reinterpret_cast<RootHClassLayoutDesc *>(rootHClassDesc)->NumOfProps();
    uint32_t maxNumOfProps = rootNumOfProps;
    CalculateMaxNumOfObj(desc, rootHClassDesc, rootNumOfProps, maxNumOfProps);
    CalculateMaxNumOfObjIncludeProtoTransition(type, maxNumOfProps);
    if (!CheckIsValid(type, maxNumOfProps)) {
        return false;
    }

    if (IsPreprocessObjectLiteralLength()) {
        if (!ObjectFactory::CanObjectLiteralHClassCache(maxNumOfProps)) {
            return false;
        }
        ptManager_->SetMaxPropsNum(rootNumOfProps, maxNumOfProps);
        JSHClass::CalculateMaxNumForChild(rootHClassDesc, maxNumOfProps);
    } else {
        JSHandle<JSHClass> rootHClass;
        if (rootType.IsGeneralizedPrototype()) {
            rootHClass = CreateRootPHClass(rootType, rootHClassDesc, maxNumOfProps, rootType.IsTransitionPrototype());
        } else if (rootType.IsConstructor()) {
            rootHClass = CreateRootCHClass(rootType, rootHClassDesc, maxNumOfProps);
        } else if (IsCache() && ObjectFactory::CanObjectLiteralHClassCache(maxNumOfProps)) {
            uint32_t maxChildNum = ptManager_->GetMaxPropsNum(rootNumOfProps);
            ASSERT(maxChildNum >= maxNumOfProps);
            rootHClass = CreateRootHClassWithCached(rootType, rootHClassDesc,
                                                    rootNumOfProps, maxChildNum);
        } else {
            rootHClass = CreateRootHClass(rootType, rootHClassDesc, maxNumOfProps);
        }
        CreateChildHClass(rootType, desc, rootHClass, rootHClassDesc);
    }
    return true;
}

bool PGOHClassGenerator::GenerateIHClass(PGOSampleType type, const JSHandle<JSTaggedValue> &prototype) const
{
    PGOHClassTreeDesc *desc;
    if (!typeRecorder_.GetHClassTreeDesc(type, &desc)) {
        return false;
    }

    if (desc->CheckHasInvalidType()) {
        return false;
    }

    auto rootType = type.GetProfileType();
    auto rootHClassDesc = desc->GetHClassLayoutDesc(rootType);
    if (rootHClassDesc == nullptr) {
        ptManager_->RecordHClass(rootType, rootType, prototype.GetTaggedType());
        return false;
    }

    uint32_t rootNumOfProps = reinterpret_cast<RootHClassLayoutDesc *>(rootHClassDesc)->NumOfProps();
    uint32_t maxNumOfProps = rootNumOfProps;
    CalculateMaxNumOfObj(desc, rootHClassDesc, rootNumOfProps, maxNumOfProps);
    CalculateMaxNumOfObjIncludeProtoTransition(type, maxNumOfProps);
    if (maxNumOfProps > PropertyAttributes::MAX_FAST_PROPS_CAPACITY) {
        return false;
    }

    JSHandle<JSHClass> rootHClass = CreateRootHClass(rootType, rootHClassDesc, maxNumOfProps);
    auto thread = ptManager_->GetJSThread();
    rootHClass->SetProto(thread, prototype);

    CreateChildHClass(rootType, desc, rootHClass, rootHClassDesc);
    return true;
}

void PGOHClassGenerator::CalculateMaxNumOfObjIncludeProtoTransition(PGOSampleType type, uint32_t &maxNum) const
{
    auto rootType = type.GetProfileType();
    std::vector<ProfileType> transPhcs = ptManager_->FindAllTransPhcByBaseType(rootType);
    for (auto &transPhc : transPhcs) {
        PGOSampleType transType(transPhc);
        PGOHClassTreeDesc *desc;
        if (!typeRecorder_.GetHClassTreeDesc(transType, &desc)) {
            continue;
        }
        auto rootHClassDesc = desc->GetHClassLayoutDesc(transPhc);
        if (rootHClassDesc == nullptr) {
            continue;
        }
        uint32_t rootNumOfProps = reinterpret_cast<RootHClassLayoutDesc *>(rootHClassDesc)->NumOfProps();
        uint32_t maxNumOfProps = rootNumOfProps;
        CalculateMaxNumOfObj(desc, rootHClassDesc, rootNumOfProps, maxNumOfProps);
        maxNum = std::max(maxNum, maxNumOfProps);
    }
}

void PGOHClassGenerator::CalculateMaxNumOfObj(
    const PGOHClassTreeDesc *desc, const HClassLayoutDesc *parent, uint32_t lastNum, uint32_t &maxNum) const
{
    if (lastNum > maxNum) {
        maxNum = lastNum;
    }
    parent->IterateChilds([this, desc, lastNum, &maxNum] (const ProfileType &childType) -> bool {
        auto layoutDesc = desc->GetHClassLayoutDesc(childType);
        if (layoutDesc == nullptr) {
            return true;
        }
        CalculateMaxNumOfObj(desc, layoutDesc, lastNum + 1, maxNum);
        return true;
    });
}

bool PGOHClassGenerator::CheckIsValid(PGOSampleType type, uint32_t maxNum) const
{
    if (maxNum > PropertyAttributes::MAX_FAST_PROPS_CAPACITY) {
        return false;
    }
    if ((IsCache() || IsPreprocessObjectLiteralLength()) &&
        !ObjectFactory::CanObjectLiteralHClassCache(maxNum)) {
        return typeRecorder_.IsValidPt(type.GetProfileType());
    }
    return true;
}

JSHandle<JSHClass> PGOHClassGenerator::CreateRootPHClass(
    ProfileType rootType, const HClassLayoutDesc *layoutDesc, uint32_t maxNum, bool isTransitionPhc) const
{
    JSHandle<JSHClass> rootHClass = CreateRootHClass(rootType, layoutDesc, maxNum);
    // transition phc is for common function now
    if (!isTransitionPhc) {
        rootHClass->SetClassPrototype(true);
    }
    rootHClass->SetIsPrototype(true);
    return rootHClass;
}

JSHandle<JSHClass> PGOHClassGenerator::CreateRootCHClass(
    ProfileType rootType, const HClassLayoutDesc *layoutDesc, uint32_t maxNum) const
{
    JSHandle<JSHClass> rootHClass = CreateRootHClass(rootType, layoutDesc, maxNum);
    rootHClass->SetClassConstructor(true);
    rootHClass->SetConstructor(true);
    rootHClass->SetCallable(true);
    return rootHClass;
}

JSHandle<JSHClass> PGOHClassGenerator::CreateRootHClass(
    ProfileType rootType, const HClassLayoutDesc *layoutDesc, uint32_t maxNum) const
{
    auto thread = ptManager_->GetJSThread();
    auto hclassValue = ptManager_->QueryHClass(rootType, rootType);
    JSHandle<JSHClass> rootHClass(thread, hclassValue);
    if (hclassValue.IsUndefined()) {
        rootHClass = JSHClass::CreateRootHClassFromPGO(thread, layoutDesc, maxNum);
        ptManager_->RecordHClass(rootType, rootType, rootHClass.GetTaggedType());
    }
    return rootHClass;
}

JSHandle<JSHClass> PGOHClassGenerator::CreateRootHClassWithCached(
    ProfileType rootType, const HClassLayoutDesc *layoutDesc,
    uint32_t literalLength, uint32_t maxPropsNum) const
{
    auto thread = ptManager_->GetJSThread();
    auto hclassValue = ptManager_->QueryHClass(rootType, rootType);
    JSHandle<JSHClass> rootHClass(thread, hclassValue);
    if (hclassValue.IsUndefined()) {
        rootHClass = JSHClass::CreateRootHClassWithCached(thread, layoutDesc, literalLength, maxPropsNum);
        ptManager_->RecordHClass(rootType, rootType, rootHClass.GetTaggedType());
    }
    return rootHClass;
}

void PGOHClassGenerator::CreateChildHClass(ProfileType rootType, const PGOHClassTreeDesc *desc,
    const JSHandle<JSHClass> &parent, const HClassLayoutDesc *parentLayoutDesc) const
{
    parentLayoutDesc->IterateChilds([this, rootType, desc, parent] (const ProfileType &childType) -> bool {
        auto layoutDesc = desc->GetHClassLayoutDesc(childType);
        if (layoutDesc == nullptr) {
            return true;
        }
        auto hclassValue = ptManager_->QueryHClass(rootType, childType);
        auto thread = ptManager_->GetJSThread();
        JSHandle<JSHClass> childHClass(thread, hclassValue);
        if (hclassValue.IsUndefined()) {
            childHClass = JSHClass::CreateChildHClassFromPGO(thread, parent, layoutDesc);
            ptManager_->RecordHClass(rootType, childType, childHClass.GetTaggedType());
        }
        CreateChildHClass(rootType, desc, childHClass, layoutDesc);
        return true;
    });
}
}  // namespace panda::ecmascript
