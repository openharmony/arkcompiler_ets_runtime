/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/ic_info.h"
#include "ecmascript/ic/ic_runtime.h"
#include "ecmascript/ic/ic_runtime_stub-inl.h"
#include "ecmascript/js_function.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {

inline void ProfileTypeInfoNexus::SetMultiIcSlotLocked(uint32_t firstIdx, const JSTaggedValue &firstValue,
                                                       uint32_t secondIdx, const JSTaggedValue &secondValue) const
{
    IcAccessorLockScope accessorLockScope(thread_);
    ASSERT(firstIdx < pti_->GetICSlotLength());
    ASSERT(secondIdx < pti_->GetICSlotLength());
    pti_->SetICSlot(thread_, firstIdx, firstValue);
    pti_->SetICSlot(thread_, secondIdx, secondValue);
}

ProfileTypeInfoNexus::ProfileTypeInfoNexus(JSThread *thread, JSHandle<ProfileTypeInfo> pti,
                                           uint32_t slotId, ICKind kind)
    : thread_(thread), pti_(pti), slotId_(slotId), kind_(kind)
{
    enableICMega_ = thread_->GetEcmaVM()->GetJSOptions().IsEnableMegaIC();
}

void ProfileTypeInfoNexus::AddElementHandler(JSHandle<JSTaggedValue> hclass, JSHandle<JSTaggedValue> handler) const
{
    ALLOW_LOCAL_TO_SHARE_WEAK_REF_HANDLE;
    auto profileData = pti_->GetICSlot(thread_, slotId_);
    ASSERT(!profileData.IsHole());
    auto index = slotId_;
    if (profileData.IsUndefined()) {
        SetMultiIcSlotLocked(index, GetWeakRef(hclass.GetTaggedValue()),
            index + 1, handler.GetTaggedValue());
        return;
    }
    // clear key ic
    if (!profileData.IsWeak() && (profileData.IsString() || profileData.IsSymbol())) {
        SetMultiIcSlotLocked(index, GetWeakRef(hclass.GetTaggedValue()),
            index + 1, handler.GetTaggedValue());
        return;
    }
    AddHandlerWithoutKey(hclass, handler);
}

void ProfileTypeInfoNexus::AddWithoutKeyPoly(JSHandle<JSTaggedValue> hclass, JSHandle<JSTaggedValue> handler,
                                             uint32_t index, JSTaggedValue profileData,
                                             JSHandle<JSTaggedValue> keyForMegaIC,
                                             MegaICCache::MegaICKind kind) const
{
    ASSERT(pti_->GetICSlot(thread_, index + 1).IsHole());
    JSHandle<TaggedArray> arr(thread_, profileData);
    const uint32_t step = 2;
    uint32_t newLen = arr->GetLength() + step;
    if (newLen > CACHE_MAX_LEN) {
        if (!enableICMega_ || keyForMegaIC.IsEmpty() || !keyForMegaIC->IsString()) {
            SetMultiIcSlotLocked(index, JSTaggedValue::Hole(), index + 1, JSTaggedValue::Hole());
            return;
        }
        ASSERT(keyForMegaIC->IsString());
        ASSERT(kind != MegaICCache::None);
        MegaICCache *cache = nullptr;
        if (kind == MegaICCache::Load) {
            cache = thread_->GetLoadMegaICCache();
        } else {
            cache = thread_->GetStoreMegaICCache();
        }

        uint32_t i = 0;
        for (; i < arr->GetLength(); i += step) {
            if (arr->Get(thread_, i) == JSTaggedValue::Undefined()) {
                continue;
            }
            cache->Set(JSHClass::Cast(arr->Get(thread_, i).GetWeakReferentUnChecked()), keyForMegaIC.GetTaggedValue(),
                       arr->Get(thread_, i + 1), thread_);
        }
        SetMultiIcSlotLocked(index, JSTaggedValue::Hole(), index + 1, keyForMegaIC.GetTaggedValue());
        return;
    }

    auto factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> newArr = factory->NewTaggedArray(newLen);
    uint32_t newArrPos = 0;
    for (uint32_t pos = 0; pos < arr->GetLength(); pos += step) {
        if (arr->Get(thread_, pos).IsUndefined()) {
            continue;
        }
        newArr->Set(thread_, newArrPos, arr->Get(thread_, pos));
        newArr->Set(thread_, newArrPos + 1, arr->Get(thread_, pos + 1));
        newArrPos += step;
    }
    newArr->Set(thread_, newArrPos++, GetWeakRef(hclass.GetTaggedValue()));
    newArr->Set(thread_, newArrPos++, handler.GetTaggedValue());
    if (newLen > newArrPos) {
        newArr->Trim(thread_, newArrPos);
    }
    SetMultiIcSlotLocked(index, newArr.GetTaggedValue(), index + 1, JSTaggedValue::Hole());
}

void ProfileTypeInfoNexus::AddHandlerWithoutKey(JSHandle<JSTaggedValue> hclass, JSHandle<JSTaggedValue> handler,
                                                JSHandle<JSTaggedValue> keyForMegaIC,
                                                MegaICCache::MegaICKind kind) const
{
    ALLOW_LOCAL_TO_SHARE_WEAK_REF_HANDLE;
    auto index = slotId_;
    if (IsNamedGlobalIC(GetKind())) {
        pti_->SetICSlot(thread_, index, handler.GetTaggedValue());
        return;
    }
    auto profileData = pti_->GetICSlot(thread_, slotId_);
    ASSERT(!profileData.IsHole());
    if (profileData.IsUndefined()) {
        SetMultiIcSlotLocked(index, GetWeakRef(hclass.GetTaggedValue()),
            index + 1, handler.GetTaggedValue());
        return;
    }
    if (!profileData.IsWeak() && profileData.IsTaggedArray()) {  // POLY
        AddWithoutKeyPoly(hclass, handler, index, profileData, keyForMegaIC, kind);
        return;
    }
    // MONO to POLY
    auto factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> newArr = factory->NewTaggedArray(POLY_CASE_NUM);
    uint32_t arrIndex = 0;
    newArr->Set(thread_, arrIndex++, pti_->GetICSlot(thread_, index));
    newArr->Set(thread_, arrIndex++, pti_->GetICSlot(thread_, index + 1));
    newArr->Set(thread_, arrIndex++, GetWeakRef(hclass.GetTaggedValue()));
    newArr->Set(thread_, arrIndex, handler.GetTaggedValue());

    SetMultiIcSlotLocked(index, newArr.GetTaggedValue(), index + 1, JSTaggedValue::Hole());
}

void ProfileTypeInfoNexus::AddHandlerWithKey(JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> hclass,
                                             JSHandle<JSTaggedValue> handler) const
{
    ALLOW_LOCAL_TO_SHARE_WEAK_REF_HANDLE;
    if (IsValueGlobalIC(GetKind())) {
        AddGlobalHandlerKey(key, handler);
        return;
    }
    auto profileData = pti_->GetICSlot(thread_, slotId_);
    ASSERT(!profileData.IsHole());
    auto index = slotId_;
    if (profileData.IsUndefined()) {
        const int arrayLength = 2;
        JSHandle<TaggedArray> newArr = thread_->GetEcmaVM()->GetFactory()->NewTaggedArray(arrayLength);
        newArr->Set(thread_, 0, GetWeakRef(hclass.GetTaggedValue()));
        newArr->Set(thread_, 1, handler.GetTaggedValue());
        SetMultiIcSlotLocked(index, key.GetTaggedValue(), index + 1, newArr.GetTaggedValue());
        return;
    }
    if (key.GetTaggedValue() != profileData) {
        SetMultiIcSlotLocked(index, JSTaggedValue::Hole(), index + 1, JSTaggedValue::Hole());
        return;
    }
    JSTaggedValue patchValue = pti_->GetICSlot(thread_, index + 1);
    ASSERT(patchValue.IsTaggedArray());
    JSHandle<TaggedArray> arr(thread_, patchValue);
    const uint32_t step = 2;
    if (arr->GetLength() > step) {  // POLY
        uint32_t newLen = arr->GetLength() + step;
        if (newLen > CACHE_MAX_LEN) {
            SetMultiIcSlotLocked(index, JSTaggedValue::Hole(), index + 1, JSTaggedValue::Hole());
            return;
        }
        auto factory = thread_->GetEcmaVM()->GetFactory();
        JSHandle<TaggedArray> newArr = factory->NewTaggedArray(newLen);
        newArr->Set(thread_, 0, GetWeakRef(hclass.GetTaggedValue()));
        newArr->Set(thread_, 1, handler.GetTaggedValue());
        for (uint32_t i = 0; i < arr->GetLength(); i += step) {
            newArr->Set(thread_, i + step, arr->Get(thread_, i));
            newArr->Set(thread_, i + step + 1, arr->Get(thread_, i + 1));
        }
        pti_->SetICSlot(thread_, index + 1, newArr.GetTaggedValue());
        return;
    }
    // MONO
    auto factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> newArr = factory->NewTaggedArray(POLY_CASE_NUM);
    uint32_t arrIndex = 0;
    newArr->Set(thread_, arrIndex++, arr->Get(thread_, 0));
    newArr->Set(thread_, arrIndex++, arr->Get(thread_, 1));
    newArr->Set(thread_, arrIndex++, GetWeakRef(hclass.GetTaggedValue()));
    newArr->Set(thread_, arrIndex++, handler.GetTaggedValue());

    pti_->SetICSlot(thread_, index + 1, newArr.GetTaggedValue());
}

void ProfileTypeInfoNexus::AddGlobalHandlerKey(JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> handler) const
{
    ALLOW_LOCAL_TO_SHARE_WEAK_REF_HANDLE;
    auto index = slotId_;
    const uint8_t step = 2;
    JSTaggedValue indexVal = pti_->GetICSlot(thread_, index);
    if (indexVal.IsUndefined()) {
        auto factory = thread_->GetEcmaVM()->GetFactory();
        JSHandle<TaggedArray> newArr = factory->NewTaggedArray(step);
        newArr->Set(thread_, 0, GetWeakRef(key.GetTaggedValue()));
        newArr->Set(thread_, 1, handler.GetTaggedValue());
        pti_->SetICSlot(thread_, index, newArr.GetTaggedValue());
        return;
    }
    ASSERT(indexVal.IsTaggedArray());
    JSHandle<TaggedArray> arr(thread_, indexVal);
    uint32_t newLen = arr->GetLength() + step;
    if (newLen > CACHE_MAX_LEN) {
        pti_->SetICSlot(thread_, index, JSTaggedValue::Hole());
        return;
    }
    auto factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> newArr = factory->NewTaggedArray(newLen);
    newArr->Set(thread_, 0, GetWeakRef(key.GetTaggedValue()));
    newArr->Set(thread_, 1, handler.GetTaggedValue());

    for (uint32_t i = 0; i < arr->GetLength(); i += step) {
        newArr->Set(thread_, i + step, arr->Get(thread_, i));
        newArr->Set(thread_, i + step + 1, arr->Get(thread_, i + 1));
    }
    pti_->SetICSlot(thread_, index, newArr.GetTaggedValue());
}

void ProfileTypeInfoNexus::AddGlobalRecordHandler(JSHandle<JSTaggedValue> handler) const
{
    uint32_t index = slotId_;
    pti_->SetICSlot(thread_, index, handler.GetTaggedValue());
}

void ProfileTypeInfoNexus::SetAsMega() const
{
    if (IsGlobalIC(kind_)) {
        pti_->SetICSlot(thread_, slotId_, JSTaggedValue::Hole());
    } else {
        SetMultiIcSlotLocked(slotId_, JSTaggedValue::Hole(), slotId_ + 1, JSTaggedValue::Hole());
    }
}

void ProfileTypeInfoNexus::SetAsMegaIfUndefined() const
{
    if (pti_->GetICSlot(thread_, slotId_).IsUndefined()) {
        SetAsMega();
    }
}

void ProfileTypeInfoNexus::SetAsMegaForTraceSlowMode([[maybe_unused]] ObjectOperator &op) const
{
#if ECMASCRIPT_ENABLE_TRACE_LOAD
    if (op.IsFoundDict()) {
        SetAsMegaForTrace(JSTaggedValue(ProfileTypeInfoNexus::MegaState::DICT_MEGA));
    } else if (!op.IsFound()) {
        SetAsMegaForTrace(JSTaggedValue(ProfileTypeInfoNexus::MegaState::NOTFOUND_MEGA));
    } else {
        SetAsMega();
    }
#else
    SetAsMega();
#endif
}

void ProfileTypeInfoNexus::SetAsMegaForTrace(JSTaggedValue value) const
{
    if (IsGlobalIC(kind_)) {
        pti_->SetICSlot(thread_, slotId_, JSTaggedValue::Hole());
    } else {
        SetMultiIcSlotLocked(slotId_, JSTaggedValue::Hole(), slotId_ + 1, value);
    }
}

ProfileTypeInfoNexus::ICState ProfileTypeInfoNexus::GetMegaState() const
{
    if (IsGlobalIC(kind_)) {
        return ICState::MEGA;
    }
    auto profileDataSecond = pti_->GetICSlot(thread_, slotId_ + 1);
    if (profileDataSecond.IsString()) {
        return ICState::IC_MEGA;
    } else {
        return ICState::MEGA;
    }
}

ProfileTypeInfoNexus::ICState ProfileTypeInfoNexus::GetICState() const
{
    auto profileData = pti_->GetICSlot(thread_, slotId_);
    if (profileData.IsUndefined()) {
        return ICState::UNINIT;
    }

    if (profileData.IsHole()) {
        return GetMegaState();
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
                return array->GetLength() == MONO_CASE_NUM ? ICState::MONO : ICState::POLY;
            }
            profileData = pti_->GetICSlot(thread_, slotId_ + 1);
            TaggedArray *array = TaggedArray::Cast(profileData.GetTaggedObject());
            return array->GetLength() == MONO_CASE_NUM ? ICState::MONO : ICState::POLY;
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
            return array->GetLength() == MONO_CASE_NUM ? ICState::MONO : ICState::POLY;
        }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
    }
    return ICState::UNINIT;
}

std::string ProfileTypeInfoNexus::ICStateToString(ProfileTypeInfoNexus::ICState state)
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
        case ICState::IC_MEGA:
            return "ic_mega";
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
    }
}

}  // namespace panda::ecmascript
