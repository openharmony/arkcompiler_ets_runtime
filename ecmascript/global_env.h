/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_GLOBAL_ENV_H
#define ECMASCRIPT_GLOBAL_ENV_H

#include "ecmascript/accessor_data.h"
#include "ecmascript/base_env.h"
#include "ecmascript/js_global_object.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/global_env_fields.h"
#include "ecmascript/snapshot/mem/snapshot_env.h"

namespace panda::ecmascript {
class JSThread;
class GlobalEnv : public BaseEnv {
public:
    using Field = GlobalEnvField;

#define GLOBAL_ENV_SLOT(type, name, index) \
    static constexpr uint16_t index = static_cast<uint16_t>(GlobalEnvField::index);

    GLOBAL_ENV_FIELDS(GLOBAL_ENV_SLOT)
    static constexpr uint16_t FIRST_DETECTOR_SYMBOL_INDEX = static_cast<uint16_t>(Field::REPLACE_SYMBOL_INDEX);
    static constexpr uint16_t LAST_DETECTOR_SYMBOL_INDEX = static_cast<uint16_t>(Field::SPECIES_SYMBOL_INDEX);
    static constexpr uint16_t FINAL_INDEX = static_cast<uint16_t>(GlobalEnvField::FINAL_INDEX);
    static constexpr uint8_t RESERVED_LENGTH = 1; // divide the gc area
    static constexpr uint16_t JSTHREAD_INDEX = FINAL_INDEX; // not need gc
#undef GLOBAL_ENV_SLOT

    JSTaggedValue GetGlobalObject() const
    {
        return GetJSGlobalObject().GetTaggedValue();
    }

    uintptr_t ComputeObjectAddress(size_t index) const
    {
        return reinterpret_cast<uintptr_t>(this) + HEADER_SIZE + index * JSTaggedValue::TaggedTypeSize();
    }

    JSHandle<JSTaggedValue> GetGlobalEnvObjectByIndex(size_t index) const
    {
        ASSERT(index < FINAL_INDEX);
        uintptr_t address = ComputeObjectAddress(index);
        JSHandle<JSTaggedValue> result(address);
        return result;
    }

    JSHandle<JSTaggedValue> GetNoLazyEnvObjectByIndex(size_t index) const
    {
        JSHandle<JSTaggedValue> result = GetGlobalEnvObjectByIndex(index);
        if (result->IsInternalAccessor()) {
            JSThread *thread = GetJSThread();
            AccessorData *accessor = AccessorData::Cast(result->GetTaggedObject());
            accessor->CallInternalGet(thread, JSHandle<JSObject>::Cast(GetJSGlobalObject()));
        }
        return result;
    }

    size_t GetGlobalEnvFieldSize() const
    {
        return FINAL_INDEX;
    }

    void Init(JSThread *thread);

    static GlobalEnv *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsJSGlobalEnv());
        return reinterpret_cast<GlobalEnv *>(object);
    }

    JSThread* GetJSThread() const
    {
        uintptr_t address = ComputeObjectAddress(JSTHREAD_INDEX);
        return *reinterpret_cast<JSThread**>(address);
    }

    void SetJSThread(JSThread *thread)
    {
        uintptr_t address = ComputeObjectAddress(JSTHREAD_INDEX);
        *reinterpret_cast<JSThread**>(address) = thread;
    }

    // For work serialize, add initialized global env object to snapshot env map
    void AddValueToSnapshotEnv(const JSThread *thread, JSTaggedValue value, uint16_t index, uint32_t offset)
    {
        if (!value.IsInternalAccessor()) {
            SnapshotEnv *snapshotEnv = thread->GetEcmaVM()->GetSnapshotEnv();
            if (!RemoveValueFromSnapshotEnv(snapshotEnv, value, offset)) {
                return;
            }
            size_t globalConstCount = thread->GlobalConstants()->GetConstantCount();
            snapshotEnv->Push(value.GetRawData(), index + globalConstCount);
        }
    }

    // For work serialize, remove old value from snapshot env map
    bool RemoveValueFromSnapshotEnv(SnapshotEnv *snapshotEnv, JSTaggedValue value, uint32_t offset)
    {
        JSTaggedValue oldValue(Barriers::GetTaggedValue(this, offset));
        if (oldValue == value) {
            return false;
        }
        if (oldValue.IsHeapObject() && !oldValue.IsInternalAccessor()) {
            // Remove old value
            snapshotEnv->Remove(oldValue.GetRawData());
        }
        return true;
    }

    void InitElementKindHClass(const JSThread *thread, JSHandle<JSHClass> originHClass)
    {
        {
            JSHandle<JSHClass> hclass;
#define INIT_ARRAY_HCLASS_INDEX_ARRAYS(name)                                                            \
            hclass = JSHClass::CloneWithElementsKind(thread, originHClass, ElementsKind::name, false);  \
            this->SetElement##name##Class(thread, hclass);

            ELEMENTS_KIND_INIT_HCLASS_LIST(INIT_ARRAY_HCLASS_INDEX_ARRAYS)
#undef INIT_ARRAY_HCLASS_INDEX_ARRAYS
        }
        this->SetElementHOLE_TAGGEDClass(thread, originHClass);
        {
            JSHandle<JSHClass> hclass;
#define INIT_ARRAY_HCLASS_INDEX_ARRAYS(name)                                                            \
            hclass = JSHClass::CloneWithElementsKind(thread, originHClass, ElementsKind::name, true);   \
            this->SetElement##name##ProtoClass(thread, hclass);

            ELEMENTS_KIND_INIT_HCLASS_LIST(INIT_ARRAY_HCLASS_INDEX_ARRAYS)
#undef INIT_ARRAY_HCLASS_INDEX_ARRAYS
        }
    }

    JSHandle<JSTaggedValue> GetSymbol(JSThread *thread, const JSHandle<JSTaggedValue> &string);
    JSHandle<JSTaggedValue> GetStringFunctionByName(JSThread *thread, const char *name);
    JSHandle<JSTaggedValue> GetStringPrototypeFunctionByName(JSThread *thread, const char *name);

    static inline uintptr_t GetFirstDetectorSymbolAddr(const GlobalEnv *env)
    {
        constexpr size_t offset = HEADER_SIZE + FIRST_DETECTOR_SYMBOL_INDEX * JSTaggedValue::TaggedTypeSize();
        uintptr_t addr = reinterpret_cast<uintptr_t>(env) + offset;
        return *reinterpret_cast<uintptr_t *>(addr);
    }

    static uintptr_t GetLastDetectorSymbolAddr(const GlobalEnv *env)
    {
        constexpr size_t offset = HEADER_SIZE + LAST_DETECTOR_SYMBOL_INDEX * JSTaggedValue::TaggedTypeSize();
        uintptr_t addr = reinterpret_cast<uintptr_t>(env) + offset;
        return *reinterpret_cast<uintptr_t *>(addr);
    }

    bool IsArrayPrototypeChangedGuardiansInvalid() const
    {
        return !GetArrayPrototypeChangedGuardians();
    }

    void ResetGuardians()
    {
        SetArrayPrototypeChangedGuardians(true);
    }

    void NotifyArrayPrototypeChangedGuardians(JSHandle<JSObject> receiver);

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GLOBAL_ENV_FIELD_ACCESSORS(type, name, index)                                                   \
    inline JSHandle<type> Get##name() const                                                             \
    {                                                                                                   \
        /* every GLOBAL_ENV_FIELD is JSTaggedValue */                                                   \
        size_t offset = HEADER_SIZE + (index) * JSTaggedValue::TaggedTypeSize();                        \
        const uintptr_t address = reinterpret_cast<uintptr_t>(this) + offset;                           \
        JSTaggedType value = Barriers::GetTaggedValue(address);                                         \
        *reinterpret_cast<JSTaggedType *>(address) = value;                                             \
        JSHandle<type> result(address);                                                                 \
        if (result.GetTaggedValue().IsInternalAccessor()) {                                             \
            JSThread *thread = GetJSThread();                                                           \
            AccessorData *accessor = AccessorData::Cast(result.GetTaggedValue().GetTaggedObject());     \
            accessor->CallInternalGet(thread, JSHandle<JSObject>::Cast(GetJSGlobalObject()));           \
        }                                                                                               \
        return result;                                                                                  \
    }                                                                                                   \
    inline JSTaggedValue GetTagged##name() const                                                        \
    {                                                                                                   \
        uint32_t offset = HEADER_SIZE + index * JSTaggedValue::TaggedTypeSize();                        \
        JSTaggedValue result(Barriers::GetTaggedValue(this, offset));                                   \
        if (result.IsInternalAccessor()) {                                                              \
            JSThread *thread = GetJSThread();                                                           \
            AccessorData *accessor = AccessorData::Cast(result.GetTaggedObject());                      \
            accessor->CallInternalGet(thread, JSHandle<JSObject>::Cast(GetJSGlobalObject()));           \
        }                                                                                               \
        return result;                                                                                  \
    }                                                                                                   \
    inline JSHandle<type> GetRaw##name() const                                                          \
    {                                                                                                   \
        const uintptr_t address =                                                                       \
            reinterpret_cast<uintptr_t>(this) + HEADER_SIZE + index * JSTaggedValue::TaggedTypeSize();  \
        JSHandle<type> result(address);                                                                 \
        return result;                                                                                  \
    }                                                                                                   \
    template<typename T>                                                                                \
    inline void Set##name(const JSThread *thread, JSHandle<T> value, BarrierMode mode = WRITE_BARRIER)  \
    {                                                                                                   \
        uint32_t offset = HEADER_SIZE + index * JSTaggedValue::TaggedTypeSize();                        \
        if (mode == WRITE_BARRIER && value.GetTaggedValue().IsHeapObject()) {                           \
            AddValueToSnapshotEnv(thread, value.GetTaggedValue(), index, offset);                       \
            Barriers::SetObject<true>(thread, this, offset, value.GetTaggedValue().GetRawData());       \
        } else {                                                                                        \
            SnapshotEnv *snapshotEnv = thread->GetEcmaVM()->GetSnapshotEnv();                           \
            RemoveValueFromSnapshotEnv(snapshotEnv, value.GetTaggedValue(), offset);                    \
            Barriers::SetPrimitive<JSTaggedType>(this, offset, value.GetTaggedValue().GetRawData());    \
        }                                                                                               \
    }                                                                                                   \
    inline void Set##name(const JSThread *thread, type value, BarrierMode mode = WRITE_BARRIER)         \
    {                                                                                                   \
        uint32_t offset = HEADER_SIZE + index * JSTaggedValue::TaggedTypeSize();                        \
        if (mode == WRITE_BARRIER && value.IsHeapObject()) {                                            \
            AddValueToSnapshotEnv(thread, value, index, offset);                                        \
            Barriers::SetObject<true>(thread, this, offset, value.GetRawData());                        \
        } else {                                                                                        \
            SnapshotEnv *snapshotEnv = thread->GetEcmaVM()->GetSnapshotEnv();                           \
            RemoveValueFromSnapshotEnv(snapshotEnv, value, offset);                                     \
            Barriers::SetPrimitive<JSTaggedType>(this, offset, value.GetRawData());                     \
        }                                                                                               \
    }
    GLOBAL_ENV_FIELDS(GLOBAL_ENV_FIELD_ACCESSORS)
#undef GLOBAL_ENV_FIELD_ACCESSORS

    static constexpr size_t HEADER_SIZE = BaseEnv::DATA_OFFSET;
    static constexpr size_t DATA_SIZE = HEADER_SIZE + FINAL_INDEX * JSTaggedValue::TaggedTypeSize();
    static constexpr size_t BIT_FIELD_OFFSET = DATA_SIZE + RESERVED_LENGTH * JSTaggedValue::TaggedTypeSize();

    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);
    DECL_VISIT_OBJECT(HEADER_SIZE, DATA_SIZE);

    // define BitField
    static constexpr uint32_t DEFAULT_LAZY_BITFIELD_VALUE = 0x7fe; // 0000'...'0000'0111'1111'1110
    static constexpr size_t ARRAYPROTOTYPE_CHANGED_GUARDIANS = 1;
    static constexpr size_t DETECTOR_BITS = 1;
    FIRST_BIT_FIELD(BitField, ArrayPrototypeChangedGuardians, bool, ARRAYPROTOTYPE_CHANGED_GUARDIANS)
    NEXT_BIT_FIELD(BitField, RegExpReplaceDetector, bool, DETECTOR_BITS, ArrayPrototypeChangedGuardians)
    NEXT_BIT_FIELD(BitField, MapIteratorDetector, bool, DETECTOR_BITS, RegExpReplaceDetector)
    NEXT_BIT_FIELD(BitField, SetIteratorDetector, bool, DETECTOR_BITS, MapIteratorDetector)
    NEXT_BIT_FIELD(BitField, StringIteratorDetector, bool, DETECTOR_BITS, SetIteratorDetector)
    NEXT_BIT_FIELD(BitField, ArrayIteratorDetector, bool, DETECTOR_BITS, StringIteratorDetector)
    NEXT_BIT_FIELD(BitField, TypedArrayIteratorDetector, bool, DETECTOR_BITS, ArrayIteratorDetector)
    NEXT_BIT_FIELD(BitField, TypedArraySpeciesProtectDetector, bool, DETECTOR_BITS, TypedArrayIteratorDetector)
    NEXT_BIT_FIELD(BitField, NumberStringNotRegexpLikeDetector, bool, DETECTOR_BITS, TypedArraySpeciesProtectDetector)
    NEXT_BIT_FIELD(BitField, RegExpFlagsDetector, bool, DETECTOR_BITS, NumberStringNotRegexpLikeDetector)
    NEXT_BIT_FIELD(BitField, RegExpSpeciesDetector, bool, DETECTOR_BITS, RegExpFlagsDetector)
    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_GLOBAL_ENV_H
