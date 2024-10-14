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

#include "ecmascript/ic/profile_type_info.h"

namespace panda::ecmascript {
void ProfileTypeAccessor::AddElementHandler(JSHandle<JSTaggedValue> hclass, JSHandle<JSTaggedValue> handler) const
{
    ALLOW_LOCAL_TO_SHARE_WEAK_REF_HANDLE;
    auto profileData = profileTypeInfo_->GetIcSlot(slotId_);
    ASSERT(!profileData.IsHole());
    auto index = slotId_;
    if (profileData.IsUndefined()) {
        profileTypeInfo_->SetMultiIcSlotLocked(thread_, index, GetWeakRef(hclass.GetTaggedValue()),
            index + 1, handler.GetTaggedValue());
        return;
    }
    // clear key ic
    if (!profileData.IsWeak() && (profileData.IsString() || profileData.IsSymbol())) {
        profileTypeInfo_->SetMultiIcSlotLocked(thread_, index, GetWeakRef(hclass.GetTaggedValue()),
            index + 1, handler.GetTaggedValue());
        return;
    }
    AddHandlerWithoutKey(hclass, handler);
}

void ProfileTypeAccessor::AddWithoutKeyPoly(JSHandle<JSTaggedValue> hclass, JSHandle<JSTaggedValue> handler,
                                            uint32_t index, JSTaggedValue profileData) const
{
    ASSERT(profileTypeInfo_->GetIcSlot(index + 1).IsHole());
    JSHandle<TaggedArray> arr(thread_, profileData);
    const uint32_t step = 2;
    uint32_t newLen = arr->GetLength() + step;
    if (newLen > CACHE_MAX_LEN) {
        profileTypeInfo_->SetMultiIcSlotLocked(thread_, index, JSTaggedValue::Hole(), index + 1, JSTaggedValue::Hole());
        return;
    }
    auto factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> newArr = factory->NewTaggedArray(newLen);
    uint32_t i = 0;
    for (; i < arr->GetLength(); i += step) {
        newArr->Set(thread_, i, arr->Get(i));
        newArr->Set(thread_, i + 1, arr->Get(i + 1));
    }
    newArr->Set(thread_, i, GetWeakRef(hclass.GetTaggedValue()));
    newArr->Set(thread_, i + 1, handler.GetTaggedValue());
    profileTypeInfo_->SetMultiIcSlotLocked(thread_, index, newArr.GetTaggedValue(), index + 1, JSTaggedValue::Hole());
}

void ProfileTypeAccessor::AddHandlerWithoutKey(JSHandle<JSTaggedValue> hclass, JSHandle<JSTaggedValue> handler) const
{
    ALLOW_LOCAL_TO_SHARE_WEAK_REF_HANDLE;
    auto index = slotId_;
    if (IsNamedGlobalIC(GetKind())) {
        profileTypeInfo_->SetIcSlot(thread_, index, handler.GetTaggedValue());
        return;
    }
    auto profileData = profileTypeInfo_->GetIcSlot(slotId_);
    ASSERT(!profileData.IsHole());
    if (profileData.IsUndefined()) {
        profileTypeInfo_->SetMultiIcSlotLocked(thread_, index, GetWeakRef(hclass.GetTaggedValue()),
            index + 1, handler.GetTaggedValue());
        return;
    }
    if (!profileData.IsWeak() && profileData.IsTaggedArray()) {  // POLY
        AddWithoutKeyPoly(hclass, handler, index, profileData);
        return;
    }
    // MONO to POLY
    auto factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> newArr = factory->NewTaggedArray(POLY_CASE_NUM);
    uint32_t arrIndex = 0;
    newArr->Set(thread_, arrIndex++, profileTypeInfo_->GetIcSlot(index));
    newArr->Set(thread_, arrIndex++, profileTypeInfo_->GetIcSlot(index + 1));
    newArr->Set(thread_, arrIndex++, GetWeakRef(hclass.GetTaggedValue()));
    newArr->Set(thread_, arrIndex, handler.GetTaggedValue());

    profileTypeInfo_->SetMultiIcSlotLocked(thread_, index, newArr.GetTaggedValue(), index + 1, JSTaggedValue::Hole());
}

void ProfileTypeAccessor::AddHandlerWithKey(JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> hclass,
                                            JSHandle<JSTaggedValue> handler) const
{
    ALLOW_LOCAL_TO_SHARE_WEAK_REF_HANDLE;
    if (IsValueGlobalIC(GetKind())) {
        AddGlobalHandlerKey(key, handler);
        return;
    }
    auto profileData = profileTypeInfo_->GetIcSlot(slotId_);
    ASSERT(!profileData.IsHole());
    auto index = slotId_;
    if (profileData.IsUndefined()) {
        const int arrayLength = 2;
        JSHandle<TaggedArray> newArr = thread_->GetEcmaVM()->GetFactory()->NewTaggedArray(arrayLength);
        newArr->Set(thread_, 0, GetWeakRef(hclass.GetTaggedValue()));
        newArr->Set(thread_, 1, handler.GetTaggedValue());
        profileTypeInfo_->SetMultiIcSlotLocked(thread_, index,
            key.GetTaggedValue(), index + 1, newArr.GetTaggedValue());
        return;
    }
    // for element ic, profileData may hclass or tagged array
    if (key.GetTaggedValue() != profileData) {
        profileTypeInfo_->SetMultiIcSlotLocked(thread_, index, JSTaggedValue::Hole(), index + 1, JSTaggedValue::Hole());
        return;
    }
    JSTaggedValue patchValue = profileTypeInfo_->GetIcSlot(index + 1);
    ASSERT(patchValue.IsTaggedArray());
    JSHandle<TaggedArray> arr(thread_, patchValue);
    const uint32_t step = 2;
    if (arr->GetLength() > step) {  // POLY
        uint32_t newLen = arr->GetLength() + step;
        if (newLen > CACHE_MAX_LEN) {
            profileTypeInfo_->SetMultiIcSlotLocked(thread_, index,
                JSTaggedValue::Hole(), index + 1, JSTaggedValue::Hole());
            return;
        }
        auto factory = thread_->GetEcmaVM()->GetFactory();
        JSHandle<TaggedArray> newArr = factory->NewTaggedArray(newLen);
        newArr->Set(thread_, 0, GetWeakRef(hclass.GetTaggedValue()));
        newArr->Set(thread_, 1, handler.GetTaggedValue());
        for (uint32_t i = 0; i < arr->GetLength(); i += step) {
            newArr->Set(thread_, i + step, arr->Get(i));
            newArr->Set(thread_, i + step + 1, arr->Get(i + 1));
        }
        profileTypeInfo_->SetIcSlot(thread_, index + 1, newArr.GetTaggedValue());
        return;
    }
    // MONO
    auto factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> newArr = factory->NewTaggedArray(POLY_CASE_NUM);
    uint32_t arrIndex = 0;
    newArr->Set(thread_, arrIndex++, arr->Get(0));
    newArr->Set(thread_, arrIndex++, arr->Get(1));
    newArr->Set(thread_, arrIndex++, GetWeakRef(hclass.GetTaggedValue()));
    newArr->Set(thread_, arrIndex++, handler.GetTaggedValue());

    profileTypeInfo_->SetIcSlot(thread_, index + 1, newArr.GetTaggedValue());
}

void ProfileTypeAccessor::AddGlobalHandlerKey(JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> handler) const
{
    ALLOW_LOCAL_TO_SHARE_WEAK_REF_HANDLE;
    auto index = slotId_;
    const uint8_t step = 2;  // key and value pair
    JSTaggedValue indexVal = profileTypeInfo_->GetIcSlot(index);
    if (indexVal.IsUndefined()) {
        auto factory = thread_->GetEcmaVM()->GetFactory();
        JSHandle<TaggedArray> newArr = factory->NewTaggedArray(step);
        newArr->Set(thread_, 0, GetWeakRef(key.GetTaggedValue()));
        newArr->Set(thread_, 1, handler.GetTaggedValue());
        profileTypeInfo_->SetIcSlot(thread_, index, newArr.GetTaggedValue());
        return;
    }
    ASSERT(indexVal.IsTaggedArray());
    JSHandle<TaggedArray> arr(thread_, indexVal);
    uint32_t newLen = arr->GetLength() + step;
    if (newLen > CACHE_MAX_LEN) {
        profileTypeInfo_->SetIcSlot(thread_, index, JSTaggedValue::Hole());
        return;
    }
    auto factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> newArr = factory->NewTaggedArray(newLen);
    newArr->Set(thread_, 0, GetWeakRef(key.GetTaggedValue()));
    newArr->Set(thread_, 1, handler.GetTaggedValue());

    for (uint32_t i = 0; i < arr->GetLength(); i += step) {
        newArr->Set(thread_, i + step, arr->Get(i));
        newArr->Set(thread_, i + step + 1, arr->Get(i + 1));
    }
    profileTypeInfo_->SetIcSlot(thread_, index, newArr.GetTaggedValue());
}

void ProfileTypeAccessor::AddGlobalRecordHandler(JSHandle<JSTaggedValue> handler) const
{
    uint32_t index = slotId_;
    profileTypeInfo_->SetIcSlot(thread_, index, handler.GetTaggedValue());
}

void ProfileTypeAccessor::SetAsMega() const
{
    if (IsGlobalIC(kind_)) {
        profileTypeInfo_->SetIcSlot(thread_, slotId_, JSTaggedValue::Hole());
    } else {
        profileTypeInfo_->SetMultiIcSlotLocked(thread_, slotId_,
            JSTaggedValue::Hole(), slotId_ + 1, JSTaggedValue::Hole());
    }
}

std::string ICKindToString(ICKind kind)
{
    switch (kind) {
        case ICKind::NamedLoadIC:
            return "NamedLoadIC";
        case ICKind::NamedStoreIC:
            return "NamedStoreIC";
        case ICKind::LoadIC:
            return "LoadIC";
        case ICKind::StoreIC:
            return "StoreIC";
        case ICKind::NamedGlobalLoadIC:
            return "NamedGlobalLoadIC";
        case ICKind::NamedGlobalStoreIC:
            return "NamedGlobalStoreIC";
        case ICKind::NamedGlobalTryLoadIC:
            return "NamedGlobalTryLoadIC";
        case ICKind::NamedGlobalTryStoreIC:
            return "NamedGlobalTryStoreIC";
        case ICKind::GlobalLoadIC:
            return "GlobalLoadIC";
        case ICKind::GlobalStoreIC:
            return "GlobalStoreIC";
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
    }
}

std::string ProfileTypeAccessor::ICStateToString(ProfileTypeAccessor::ICState state)
{
    switch (state) {
        case ICState::UNINIT:
            return "uninit";
        case ICState::MONO:
            return "mono";
        case ICState::POLY:
            return "poly";
        case ICState::MEGA:
            return "mega";
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
    }
}

ProfileTypeAccessor::ICState ProfileTypeAccessor::GetICState() const
{
    auto profileData = profileTypeInfo_->GetIcSlot(slotId_);
    if (profileData.IsUndefined()) {
        return ICState::UNINIT;
    }

    if (profileData.IsHole()) {
        return ICState::MEGA;
    }

    switch (kind_) {
        case ICKind::NamedLoadIC:
        case ICKind::NamedStoreIC:
            if (profileData.IsWeak()) {
                return ICState::MONO;
            }
            ASSERT(profileData.IsTaggedArray());
            return ICState::POLY;
        case ICKind::LoadIC:
        case ICKind::StoreIC: {
            if (profileData.IsWeak()) {
                return ICState::MONO;
            }
            if (profileData.IsTaggedArray()) {
                TaggedArray *array = TaggedArray::Cast(profileData.GetTaggedObject());
                return array->GetLength() == MONO_CASE_NUM ? ICState::MONO : ICState::POLY; // 2 : test case
            }
            profileData = profileTypeInfo_->GetIcSlot(slotId_ + 1);
            TaggedArray *array = TaggedArray::Cast(profileData.GetTaggedObject());
            return array->GetLength() == MONO_CASE_NUM ? ICState::MONO : ICState::POLY; // 2 : test case
        }
        case ICKind::NamedGlobalLoadIC:
        case ICKind::NamedGlobalStoreIC:
        case ICKind::NamedGlobalTryLoadIC:
        case ICKind::NamedGlobalTryStoreIC:
            ASSERT(profileData.IsPropertyBox());
            return ICState::MONO;
        case ICKind::GlobalLoadIC:
        case ICKind::GlobalStoreIC: {
            ASSERT(profileData.IsTaggedArray());
            TaggedArray *array = TaggedArray::Cast(profileData.GetTaggedObject());
            return array->GetLength() == MONO_CASE_NUM ? ICState::MONO : ICState::POLY; // 2 : test case
        }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
    }
    return ICState::UNINIT;
}
}  // namespace panda::ecmascript
