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

#include "ecmascript/ic/ic_info.h"

#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/ic_runtime.h"
#include "ecmascript/ic/ic_runtime_stub-inl.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/js_function.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
IcAccessorLockScope::IcAccessorLockScope(JSThread *thread)
{
    if (thread->GetEcmaVM()->IsEnableFastJit() || thread->GetEcmaVM()->IsEnableBaselineJit()) {
        lockHolder_.emplace(thread->GetIcAccessorLock());
    }
}

IcAccessor::IcAccessor(JSThread* thread, JSHandle<ProfileTypeInfo> profileTypeInfo, uint32_t slotId, ICKind kind)
    : IcAccessor(thread, JSHandle<ICInfo>(profileTypeInfo), slotId, kind)
{
}

IcAccessor::IcAccessor(JSThread* thread, JSHandle<ICInfo> icInfo, uint32_t slotId, ICKind kind)
    : thread_(thread), icInfo_(icInfo), slotId_(slotId), kind_(kind)
{
    enableICMega_ = thread_->GetEcmaVM()->GetJSOptions().IsEnableMegaIC();
}

void ICInfo::SetMultiIcSlotLocked(JSThread *thread, uint32_t firstIdx, const JSTaggedValue &firstValue,
    uint32_t secondIdx, const JSTaggedValue &secondValue)
{
    IcAccessorLockScope accessorLockScope(thread);
    ASSERT(firstIdx < GetLength());
    ASSERT(secondIdx < GetLength());
    Set(thread, firstIdx, firstValue);
    Set(thread, secondIdx, secondValue);
}

void IcAccessor::AddElementHandler(JSHandle<JSTaggedValue> hclass, JSHandle<JSTaggedValue> handler) const
{
    ALLOW_LOCAL_TO_SHARE_WEAK_REF_HANDLE;
    auto profileData = icInfo_->GetIcSlot(thread_, slotId_);
    ASSERT(!profileData.IsHole());
    auto index = slotId_;
    if (profileData.IsUndefined()) {
        icInfo_->SetMultiIcSlotLocked(thread_, index, GetWeakRef(hclass.GetTaggedValue()),
            index + 1, handler.GetTaggedValue());
        return;
    }
    // clear key ic
    if (!profileData.IsWeak() && (profileData.IsString() || profileData.IsSymbol())) {
        icInfo_->SetMultiIcSlotLocked(thread_, index, GetWeakRef(hclass.GetTaggedValue()),
            index + 1, handler.GetTaggedValue());
        return;
    }
    AddHandlerWithoutKey(hclass, handler);
}

void IcAccessor::AddWithoutKeyPoly(JSHandle<JSTaggedValue> hclass, JSHandle<JSTaggedValue> handler,
                                   uint32_t index, JSTaggedValue profileData,
                                   JSHandle<JSTaggedValue> keyForMegaIC, MegaICCache::MegaICKind kind) const
{
    ASSERT(icInfo_->GetIcSlot(thread_, index + 1).IsHole());
    JSHandle<TaggedArray> arr(thread_, profileData);
    const uint32_t step = 2;
    uint32_t newLen = arr->GetLength() + step;
    if (newLen > CACHE_MAX_LEN) {
        if (!enableICMega_ || keyForMegaIC.IsEmpty() || !keyForMegaIC->IsString()) {
            icInfo_->SetMultiIcSlotLocked(thread_, index, JSTaggedValue::Hole(), index + 1,
                                          JSTaggedValue::Hole());
            return;
        }
        // The keyForMegaIC must be a String to ensure fast subsequent reads; assembly code will access using
        // String.
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
        icInfo_->SetMultiIcSlotLocked(thread_, index, JSTaggedValue::Hole(), index + 1,
                                      keyForMegaIC.GetTaggedValue());
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
    icInfo_->SetMultiIcSlotLocked(thread_, index, newArr.GetTaggedValue(), index + 1, JSTaggedValue::Hole());
}

void IcAccessor::AddHandlerWithoutKey(JSHandle<JSTaggedValue> hclass, JSHandle<JSTaggedValue> handler,
                                      JSHandle<JSTaggedValue> keyForMegaIC,
                                      MegaICCache::MegaICKind kind) const
{
    ALLOW_LOCAL_TO_SHARE_WEAK_REF_HANDLE;
    auto index = slotId_;
    if (IsNamedGlobalIC(GetKind())) {
        icInfo_->SetIcSlot(thread_, index, handler.GetTaggedValue());
        return;
    }
    auto profileData = icInfo_->GetIcSlot(thread_, slotId_);
    ASSERT(!profileData.IsHole());
    if (profileData.IsUndefined()) {
        icInfo_->SetMultiIcSlotLocked(thread_, index, GetWeakRef(hclass.GetTaggedValue()),
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
    newArr->Set(thread_, arrIndex++, icInfo_->GetIcSlot(thread_, index));
    newArr->Set(thread_, arrIndex++, icInfo_->GetIcSlot(thread_, index + 1));
    newArr->Set(thread_, arrIndex++, GetWeakRef(hclass.GetTaggedValue()));
    newArr->Set(thread_, arrIndex, handler.GetTaggedValue());

    icInfo_->SetMultiIcSlotLocked(thread_, index, newArr.GetTaggedValue(), index + 1, JSTaggedValue::Hole());
}

void IcAccessor::AddHandlerWithKey(JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> hclass,
                                   JSHandle<JSTaggedValue> handler) const
{
    ALLOW_LOCAL_TO_SHARE_WEAK_REF_HANDLE;
    if (IsValueGlobalIC(GetKind())) {
        AddGlobalHandlerKey(key, handler);
        return;
    }
    auto profileData = icInfo_->GetIcSlot(thread_, slotId_);
    ASSERT(!profileData.IsHole());
    auto index = slotId_;
    if (profileData.IsUndefined()) {
        const int arrayLength = 2;
        JSHandle<TaggedArray> newArr = thread_->GetEcmaVM()->GetFactory()->NewTaggedArray(arrayLength);
        newArr->Set(thread_, 0, GetWeakRef(hclass.GetTaggedValue()));
        newArr->Set(thread_, 1, handler.GetTaggedValue());
        icInfo_->SetMultiIcSlotLocked(thread_, index,
            key.GetTaggedValue(), index + 1, newArr.GetTaggedValue());
        return;
    }
    // for element ic, profileData may hclass or tagged array
    if (key.GetTaggedValue() != profileData) {
        icInfo_->SetMultiIcSlotLocked(thread_, index, JSTaggedValue::Hole(), index + 1, JSTaggedValue::Hole());
        return;
    }
    JSTaggedValue patchValue = icInfo_->GetIcSlot(thread_, index + 1);
    ASSERT(patchValue.IsTaggedArray());
    JSHandle<TaggedArray> arr(thread_, patchValue);
    const uint32_t step = 2;
    if (arr->GetLength() > step) {  // POLY
        uint32_t newLen = arr->GetLength() + step;
        if (newLen > CACHE_MAX_LEN) {
            icInfo_->SetMultiIcSlotLocked(thread_, index,
                JSTaggedValue::Hole(), index + 1, JSTaggedValue::Hole());
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
        icInfo_->SetIcSlot(thread_, index + 1, newArr.GetTaggedValue());
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

    icInfo_->SetIcSlot(thread_, index + 1, newArr.GetTaggedValue());
}

void IcAccessor::AddGlobalHandlerKey(JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> handler) const
{
    ALLOW_LOCAL_TO_SHARE_WEAK_REF_HANDLE;
    auto index = slotId_;
    const uint8_t step = 2;  // key and value pair
    JSTaggedValue indexVal = icInfo_->GetIcSlot(thread_, index);
    if (indexVal.IsUndefined()) {
        auto factory = thread_->GetEcmaVM()->GetFactory();
        JSHandle<TaggedArray> newArr = factory->NewTaggedArray(step);
        newArr->Set(thread_, 0, GetWeakRef(key.GetTaggedValue()));
        newArr->Set(thread_, 1, handler.GetTaggedValue());
        icInfo_->SetIcSlot(thread_, index, newArr.GetTaggedValue());
        return;
    }
    ASSERT(indexVal.IsTaggedArray());
    JSHandle<TaggedArray> arr(thread_, indexVal);
    uint32_t newLen = arr->GetLength() + step;
    if (newLen > CACHE_MAX_LEN) {
        icInfo_->SetIcSlot(thread_, index, JSTaggedValue::Hole());
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
    icInfo_->SetIcSlot(thread_, index, newArr.GetTaggedValue());
}

void IcAccessor::AddGlobalRecordHandler(JSHandle<JSTaggedValue> handler) const
{
    uint32_t index = slotId_;
    icInfo_->SetIcSlot(thread_, index, handler.GetTaggedValue());
}

void IcAccessor::SetAsMegaForTraceSlowMode([[maybe_unused]] ObjectOperator& op) const
{
#if ECMASCRIPT_ENABLE_TRACE_LOAD
        if (op.IsFoundDict()) {
            SetAsMegaForTrace(JSTaggedValue(IcAccessor::MegaState::DICT_MEGA));
        } else if (!op.IsFound()) {
            SetAsMegaForTrace(JSTaggedValue(IcAccessor::MegaState::NOTFOUND_MEGA));
        } else {
            SetAsMega();
        }
#else
        SetAsMega();
#endif
}

void IcAccessor::SetAsMega() const
{
    if (IsGlobalIC(kind_)) {
        icInfo_->SetIcSlot(thread_, slotId_, JSTaggedValue::Hole());
    } else {
        icInfo_->SetMultiIcSlotLocked(thread_, slotId_,
            JSTaggedValue::Hole(), slotId_ + 1, JSTaggedValue::Hole());
    }
}

void IcAccessor::SetAsMegaIfUndefined() const
{
    if (icInfo_->GetIcSlot(thread_, slotId_).IsUndefined()) {
        SetAsMega();
    }
}

void IcAccessor::SetAsMegaForTrace(JSTaggedValue value) const
{
    if (IsGlobalIC(kind_)) {
        icInfo_->SetIcSlot(thread_, slotId_, JSTaggedValue::Hole());
    } else {
        icInfo_->SetMultiIcSlotLocked(thread_, slotId_,
            JSTaggedValue::Hole(), slotId_ + 1, value);
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

std::string IcAccessor::ICStateToString(IcAccessor::ICState state)
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

IcAccessor::ICState IcAccessor::GetMegaState() const
{
    if (IsGlobalIC(kind_)) {
        return ICState::MEGA;
    }
    auto profileDataSecond = icInfo_->Get(thread_, slotId_ + 1);
    if (profileDataSecond.IsString()) {
        return ICState::IC_MEGA;
    } else {
        return ICState::MEGA;
    }
}

IcAccessor::ICState IcAccessor::GetICState() const
{
    auto profileData = icInfo_->GetIcSlot(thread_, slotId_);
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
                return array->GetLength() == MONO_CASE_NUM ? ICState::MONO : ICState::POLY; // 2 : test case
            }
            profileData = icInfo_->GetIcSlot(thread_, slotId_ + 1);
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
JSTaggedValue NapiICInfo::TryLoadKeyIC(JSThread *thread, JSHandle<NapiICInfo> icInfo,
                                       JSTaggedValue obj, JSTaggedValue key)
{
    JSTaggedValue slot0 = icInfo->GetIcSlot(thread, 0);
    JSTaggedValue slot1 = icInfo->GetIcSlot(thread, 1);
    if (IsKeyIC(slot0)) {
        return ICRuntimeStub::TryLoadICByValue(thread, obj, key, slot0, slot1);
    }
    return JSTaggedValue::Hole();
}

JSTaggedValue NapiICInfo::TryLoadICOrMiss(JSThread *thread, JSHandle<NapiICInfo> icInfo,
                                          JSHandle<JSTaggedValue> obj,
                                          JSMutableHandle<JSTaggedValue> &key, bool *hit)
{
    // slot0 of Key-IC is always interned, but key might be a normal string.
    // So intern string keys here to make TryStoreICByValue's pointer equality (firstValue == key) works.
    if (key->IsString() && !EcmaStringAccessor(key.GetTaggedValue()).IsInternString()) {
        key.Update(JSTaggedValue(thread->GetEcmaVM()->GetFactory()->InternString(key)));
    }

    // Intern string might trigger GC, reload ic slots
    JSTaggedValue slot0 = icInfo->GetIcSlot(thread, 0);
    JSTaggedValue slot1 = icInfo->GetIcSlot(thread, 1);

    // Skip IC probe for UNINIT state (slot0 == Undefined) — go directly to miss handler.
    // TryLoadICByValue uses pointer equality (firstValue == key); if both are Undefined
    // it would match and then treat slot1 as a TaggedArray, which crashes.
    JSTaggedValue res = JSTaggedValue::Hole();
    if (LIKELY(!slot0.IsUndefined())) {
        res = ICRuntimeStub::TryLoadICByValue(thread, obj.GetTaggedValue(), key.GetTaggedValue(), slot0, slot1);
    }

    if (res.IsHole()) {
        // KEY_IC key mismatch: reset to give new key a fresh start.
        // No lock needed: NAPI ICInfo is not attached to any JSFunction's ProfileTypeInfo,
        // so the JIT compiler thread never reads these slots.
        if (IsKeyIC(slot0) && slot0 != key.GetTaggedValue()) {
            icInfo->SetIcSlot(thread, 0, JSTaggedValue::Undefined());
            icInfo->SetIcSlot(thread, 1, JSTaggedValue::Undefined());
            icInfo->ClearICKind(thread);
        }
        LoadICRuntime icRT(thread, JSHandle<ICInfo>(icInfo), 0, ICKind::LoadIC);
        res = icRT.LoadValueMiss(obj, key);
        if (!res.IsHole()) {
            icInfo->MarkAsGetIC(thread);
        }
    } else if (hit != nullptr) {
        *hit = true;
    }

    return res;
}

JSTaggedValue NapiICInfo::TryStoreICOrMiss(JSThread *thread, JSHandle<NapiICInfo> icInfo,
                                           JSHandle<JSTaggedValue> obj,
                                           JSMutableHandle<JSTaggedValue> &key,
                                           JSHandle<JSTaggedValue> value, bool *hit)
{
    // slot0 of Key-IC is always interned, but key might be a normal string.
    // So intern string keys here to make TryStoreICByValue's pointer equality (firstValue == key) works.
    if (key->IsString() && !EcmaStringAccessor(key.GetTaggedValue()).IsInternString()) {
        key.Update(JSTaggedValue(thread->GetEcmaVM()->GetFactory()->InternString(key)));
    }

    // Intern string might trigger GC, read after intern
    JSTaggedValue slot0 = icInfo->GetIcSlot(thread, 0);
    JSTaggedValue slot1 = icInfo->GetIcSlot(thread, 1);

    // Skip IC probe for UNINIT state — see TryLoadICOrMiss comment for rationale.
    JSTaggedValue res = JSTaggedValue::Hole();
    if (LIKELY(!slot0.IsUndefined())) {
        res = ICRuntimeStub::TryStoreICByValue(thread, obj.GetTaggedValue(),
                                               key.GetTaggedValue(), slot0, slot1,
                                               value.GetTaggedValue());
    }

    if (res.IsHole()) {
        // KEY_IC key mismatch: reset to give new key a fresh start.
        // No lock needed: NAPI ICInfo is not attached to any JSFunction's ProfileTypeInfo,
        // so the JIT compiler thread never reads these slots.
        if (IsKeyIC(slot0) && slot0 != key.GetTaggedValue()) {
            icInfo->SetIcSlot(thread, 0, JSTaggedValue::Undefined());
            icInfo->SetIcSlot(thread, 1, JSTaggedValue::Undefined());
            icInfo->ClearICKind(thread);
        }
        StoreICRuntime icRT(thread, JSHandle<ICInfo>(icInfo), 0, ICKind::StoreIC);
        res = icRT.StoreMiss(obj, key, value);
        if (!res.IsHole()) {
            icInfo->MarkAsSetIC(thread);
        }
    } else if (hit != nullptr) {
        *hit = true;
    }

    return res;
}
}  // namespace panda::ecmascript
