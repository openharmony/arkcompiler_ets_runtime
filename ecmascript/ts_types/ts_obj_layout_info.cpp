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
#include "ecmascript/ts_types/ts_obj_layout_info.h"

namespace panda::ecmascript {
void TSObjLayoutInfo::AddProperty(const JSThread *thread, const JSTaggedValue key,
                                  const JSTaggedValue typeIdVal, const uint32_t attr)
{
    AddProperty(thread, key, typeIdVal, JSTaggedValue(attr));
}

void TSObjLayoutInfo::AddProperty(const JSThread *thread, const JSTaggedValue key,
                                  const JSTaggedValue typeIdVal, const JSTaggedValue attr)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t number = GetNumOfProperties();
    ASSERT(number + 1 <= GetPropertiesCapacity());
    ASSERT(attr.IsInt());
    SetNumOfProperties(thread, number + 1);
    SetKey(thread, number, key);
    SetTypeId(thread, number, typeIdVal);
    SetAttribute(thread, number, attr);
}

bool TSObjLayoutInfo::Find(JSTaggedValue key) const
{
    return GetElementIndexByKey(key) != INVALID_INDEX;
}

int TSObjLayoutInfo::GetElementIndexByKey(JSTaggedValue key) const
{
    [[maybe_unused]] EcmaString *str = EcmaString::Cast(key.GetTaggedObject());
    ASSERT_PRINT(EcmaStringAccessor(str).IsInternString(), "TS class field key is not an intern string");

    uint32_t length = GetNumOfProperties();
    for (uint32_t i = 0; i < length; ++i) {
        JSTaggedValue keyVal = GetKey(i);
        ASSERT_PRINT(keyVal.IsString(), "TS class field key is not a string");
        if (keyVal == key) {
            return i;
        }
    }
    return INVALID_INDEX;
}

void TSObjLayoutInfo::GetAccessorIndexByKey(JSTaggedValue key, std::vector<int> &vec)
{
    [[maybe_unused]] EcmaString *str = EcmaString::Cast(key.GetTaggedObject());
    ASSERT_PRINT(EcmaStringAccessor(str).IsInternString(), "TS class field key is not an intern string");

    uint32_t length = GetNumOfProperties();
    for (uint32_t i = 0; i < length; ++i) {
        JSTaggedValue keyVal = GetKey(i);
        ASSERT_PRINT(keyVal.IsString(), "TS class field key is not a string");
        if (keyVal == key) {
            vec.emplace_back(i);
        }
    }
}

JSTaggedValue TSObjLayoutInfo::TryGetTypeByIndexSign(const uint32_t keyType)
{
    uint32_t length = GetNumOfProperties();
    for (uint32_t i = 0; i < length; i++) {
        if (static_cast<uint32_t>(GetKey(i).GetInt()) == keyType) {
            JSTaggedValue value = GetTypeId(i);
            ASSERT(value.IsInt());
            return value;
        }
    }
    return JSTaggedValue::Undefined();
}

JSHandle<TSObjLayoutInfo> TSObjLayoutInfo::PushBack(const JSThread *thread,
                                                    const JSHandle<TSObjLayoutInfo> &oldLayout,
                                                    const JSHandle<JSTaggedValue> &key,
                                                    const JSHandle<JSTaggedValue> &value)
{
    if (oldLayout->GetNumOfProperties() < oldLayout->GetPropertiesCapacity()) {
        oldLayout->AddProperty(thread, key.GetTaggedValue(), value.GetTaggedValue());
        return oldLayout;
    }

    JSHandle<TSObjLayoutInfo> newLayout = ExtendTSObjLayoutInfo(thread, oldLayout);
    newLayout->AddProperty(thread, key.GetTaggedValue(), value.GetTaggedValue());
    return newLayout;
}

JSHandle<TSObjLayoutInfo> TSObjLayoutInfo::ExtendTSObjLayoutInfo(const JSThread *thread,
                                                                 const JSHandle<TSObjLayoutInfo> &oldLayout,
                                                                 JSTaggedValue initVal)
{
    uint32_t oldLength = oldLayout->GetPropertiesCapacity();
    ASSERT(oldLength == oldLayout->GetNumOfProperties());
    uint32_t arrayLength = TSObjLayoutInfo::ComputeArrayLength(TSObjLayoutInfo::ComputeGrowCapacity(oldLength));
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TSObjLayoutInfo> newLayout(factory->ExtendArray(JSHandle<TaggedArray>(oldLayout), arrayLength, initVal));
    newLayout->SetNumOfProperties(thread, oldLength);
    return newLayout;
}
}  // namespace panda::ecmascript
