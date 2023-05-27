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

JSHandle<TSObjLayoutInfo> TSTypeAccessor::GetInstanceTypeLayout() const
{
    ASSERT(tsManager_->IsClassTypeKind(gt_));
    JSHandle<TSClassType> classType = GetClassType();
    JSHandle<TSObjectType> instanceType(thread_, classType->GetInstanceType());
    JSHandle<TSObjLayoutInfo> layout(thread_, instanceType->GetObjLayoutInfo());
    return layout;
}
}  // namespace panda::ecmascript
