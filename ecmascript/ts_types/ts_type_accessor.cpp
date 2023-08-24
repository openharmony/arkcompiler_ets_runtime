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

#include "ecmascript/ts_types/ts_type_accessor.h"

namespace panda::ecmascript {
void TSTypeAccessor::MarkPropertyInitialized(JSTaggedValue key)
{
    ASSERT(tsManager_->IsClassTypeKind(gt_));
    JSHandle<TSObjLayoutInfo> layout = GetInstanceTypeLayout();
    int elemenIndex = layout->GetElementIndexByKey(key);
    if (TSObjLayoutInfo::IsValidIndex(elemenIndex)) {
        JSTaggedValue attribute = layout->GetAttribute(elemenIndex);
        ASSERT(attribute.IsInt());
        layout->SetAttribute(thread_, elemenIndex, MarkInitialized(attribute));
    }
}

void TSTypeAccessor::UpdateNonStaticProp(JSTaggedValue key, GlobalTSTypeRef newGT)
{
    ASSERT(tsManager_->IsClassTypeKind(gt_));
    JSHandle<TSClassType> classType = GetClassType();
    JSHandle<JSTaggedValue> propName(thread_, key);
    TSClassType::UpdateNonStaticPropTypeGT(thread_, classType, propName, newGT);
}

void TSTypeAccessor::UpdateStaticProp(JSTaggedValue key, GlobalTSTypeRef newGT)
{
    ASSERT(tsManager_->IsClassTypeKind(gt_));
    JSHandle<TSClassType> classType = GetClassType();
    JSHandle<JSTaggedValue> propName(thread_, key);
    TSClassType::UpdateStaticPropTypeGT(thread_, classType, propName, newGT);
}

void TSTypeAccessor::UpdateForEachCBPara(kungfu::GateType targetType)
{
    if (!tsManager_->IsFunctionTypeKind(gt_)) {
        return;
    }
    JSHandle<TSFunctionType> callbackType = GetFunctionType();
    uint32_t paraSz = callbackType->GetLength();
    JSHandle<TaggedArray> parameterTypes(thread_, callbackType->GetParameterTypes());
    const uint32_t maxParaSz = 3; // elementValue, index and array
    if (paraSz > maxParaSz) {
        return;
    }
    GlobalTSTypeRef indeGT = kungfu::GateType::IntType().GetGTRef();
    GlobalTSTypeRef elementGT = GlobalTSTypeRef::Default();
    GlobalTSTypeRef targetGT = targetType.GetGTRef();
    if (tsManager_->IsArrayTypeKind(targetType)) {
        elementGT = tsManager_->GetArrayParameterTypeGT(targetType);
    }
    if (tsManager_->IsTypedArrayType(targetType)) {
        elementGT = kungfu::GateType::NumberType().GetGTRef();
    }
    if (elementGT.IsDefault()) {
        return;
    }
    std::vector<GlobalTSTypeRef> tempGTs{elementGT, indeGT, targetGT};
    for (uint32_t i = 0; i < paraSz; i++) {
        if (!parameterTypes->Get(i).GetInt()) {
            parameterTypes->Set(thread_, i, JSTaggedValue(tempGTs[i].GetType()));
        }
    }
}

JSTaggedValue TSTypeAccessor::MarkInitialized(JSTaggedValue attribute)
{
    ASSERT(attribute.IsInt());
    TSFieldAttributes fieldAttr(static_cast<uint32_t>(attribute.GetInt()));
    fieldAttr.SetInitializedFlag(true);
    return JSTaggedValue(fieldAttr.GetValue());
}

bool TSTypeAccessor::IsPropertyInitialized(JSTaggedValue key) const
{
    ASSERT(tsManager_->IsClassTypeKind(gt_));
    JSHandle<TSObjLayoutInfo> layout = GetInstanceTypeLayout();
    int elemenIndex = layout->GetElementIndexByKey(key);
    if (!TSObjLayoutInfo::IsValidIndex(elemenIndex)) {
        return false;
    }
    JSTaggedValue attribute = layout->GetAttribute(elemenIndex);
    ASSERT(attribute.IsInt());
    TSFieldAttributes fieldAttr(static_cast<uint32_t>(attribute.GetInt()));
    return fieldAttr.GetInitializedFlag();
}

std::string TSTypeAccessor::GetInitializedProperties() const
{
    ASSERT(tsManager_->IsClassTypeKind(gt_));
    JSHandle<TSObjLayoutInfo> layout = GetInstanceTypeLayout();
    std::string names("");
    uint32_t length = layout->GetNumOfProperties();
    for (uint32_t i = 0; i < length; i++) {
        JSTaggedValue attribute = layout->GetAttribute(i);
        ASSERT(attribute.IsInt());
        TSFieldAttributes fieldAttr(static_cast<uint32_t>(attribute.GetInt()));
        if (fieldAttr.GetInitializedFlag()) {
            JSTaggedValue name = layout->GetKey(i);
            ASSERT(name.IsString());
            names += (EcmaStringAccessor(name).ToStdString() + " ");
        }
    }
    return names;
}

std::string TSTypeAccessor::GetClassTypeName() const
{
    ASSERT(tsManager_->IsClassTypeKind(gt_));
    return tsManager_->GetClassTypeStr(gt_);
}

std::string TSTypeAccessor::GetFunctionName() const
{
    JSHandle<TSFunctionType> funcType = GetFunctionType();
    EcmaStringAccessor acc(funcType->GetName());
    std::string nameStr = acc.ToStdString();
    return nameStr;
}

JSHandle<TSObjLayoutInfo> TSTypeAccessor::GetInstanceTypeLayout() const
{
    ASSERT(tsManager_->IsClassTypeKind(gt_));
    JSHandle<TSClassType> classType = GetClassType();
    JSHandle<TSObjectType> instanceType(thread_, classType->GetInstanceType());
    JSHandle<TSObjLayoutInfo> layout(thread_, instanceType->GetObjLayoutInfo());
    return layout;
}

JSHandle<TSObjLayoutInfo> TSTypeAccessor::GetPrototypeTypeLayout() const
{
    ASSERT(tsManager_->IsClassTypeKind(gt_));
    JSHandle<TSClassType> classType = GetClassType();
    JSHandle<TSObjectType> prototypeType(thread_, classType->GetPrototypeType());
    JSHandle<TSObjLayoutInfo> layout(thread_, prototypeType->GetObjLayoutInfo());
    return layout;
}

GlobalTSTypeRef TSTypeAccessor::GetAccessorGT(JSTaggedValue key, bool isSetter) const
{
    ASSERT(tsManager_->IsClassTypeKind(gt_));
    JSHandle<TSObjLayoutInfo> layout = GetPrototypeTypeLayout();
    std::vector<int> vec;
    layout->GetAccessorIndexByKey(key, vec);
    if (vec.empty()) {
        return GlobalTSTypeRef();
    }

    for (size_t i = 0; i < vec.size(); i++) {
        int index = vec[i];
        ASSERT(TSObjLayoutInfo::IsValidIndex(index));
        auto gt = GlobalTSTypeRef(layout->GetTypeId(index).GetInt());
        uint32_t parameterLength = tsManager_->GetFunctionTypeLength(gt);
        if (IsSetterGT(parameterLength, isSetter) || IsGetterGT(parameterLength, isSetter)) {
            return gt;
        }
    }
    return GlobalTSTypeRef();
}
}  // namespace panda::ecmascript
