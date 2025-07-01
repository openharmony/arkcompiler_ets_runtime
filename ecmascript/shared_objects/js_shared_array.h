/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_SHARED_OBJECTS_JS_SHARED_ARRAY_H
#define ECMASCRIPT_SHARED_OBJECTS_JS_SHARED_ARRAY_H

#include "ecmascript/js_array.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript {
// enum class ArrayMode : uint8_t { UNDEFINED = 0, DICTIONARY, LITERAL };
// ecma6 9.4.2 Array Exotic Object
class JSSharedArray : public JSObject {
public:
    static constexpr int LENGTH_INLINE_PROPERTY_INDEX = 0;

    CAST_CHECK(JSSharedArray, IsJSSharedArray);

    static PUBLIC_API JSHandle<JSTaggedValue> ArrayCreate(JSThread *thread, JSTaggedNumber length,
                                                          ArrayMode mode = ArrayMode::UNDEFINED);
    static JSHandle<JSTaggedValue> ArrayCreate(JSThread *thread, JSTaggedNumber length,
                                               const JSHandle<JSTaggedValue> &newTarget,
                                               ArrayMode mode = ArrayMode::UNDEFINED);
    static JSTaggedValue ArraySpeciesCreate(JSThread *thread, const JSHandle<JSObject> &originalArray,
                                            JSTaggedNumber length);
    static bool ArraySetLength(JSThread *thread, const JSHandle<JSObject> &array, const PropertyDescriptor &desc);
    static bool DefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &array, const JSHandle<JSTaggedValue> &key,
                                  const PropertyDescriptor &desc, SCheckMode sCheckMode = SCheckMode::CHECK);
    static bool DefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &array, uint32_t index,
                                  const PropertyDescriptor &desc, SCheckMode sCheckMode = SCheckMode::CHECK);

    static bool IsLengthString(JSThread *thread, const JSHandle<JSTaggedValue> &key);
    static JSHandle<JSSharedArray> CreateArrayFromList(JSThread *thread, const JSHandle<TaggedArray> &elements);
    // use first inlined property slot for array length
    inline uint32_t GetArrayLength() const
    {
        return GetLength();
    }

    inline void SetArrayLength([[maybe_unused]]const JSThread *thread, uint32_t length)
    {
        SetLength(length);
    }

    inline uint32_t GetHintLength(const JSThread *thread) const
    {
        auto trackInfo = GetTrackInfo(thread);
        if (trackInfo.IsInt()) {
            int hint = trackInfo.GetInt();
            return hint > 0 ? hint : 0;
        }
        return 0;
    }

    inline bool IsKeyInRange(uint32_t index)
    {
        return index < GetArrayLength();
    }

    inline bool IsKeyInRange(const JSThread *thread, const JSHandle<JSTaggedValue> &key)
    {
        uint32_t keyValue = 0;
        if (key->IsInt()) {
            keyValue = static_cast<uint32_t>(key->GetInt());
        }

        if (key->IsString()) {
            JSTaggedValue::ToElementIndex(const_cast<JSThread*>(thread), key.GetTaggedValue(), &keyValue);
        }

        if (key->IsDouble()) {
            double number = key->GetDouble();
            if (number >= 0 && number < JSObject::MAX_ELEMENT_INDEX) {
                auto integer = static_cast<uint32_t>(number);
                if (integer == number) {
                    keyValue = static_cast<uint32_t>(number);
                }
            }
        }

        return IsKeyInRange(keyValue);
    }

    static constexpr size_t LENGTH_OFFSET = JSObject::SIZE;
    ACCESSORS_SYNCHRONIZED_PRIMITIVE_FIELD(Length, uint32_t, LENGTH_OFFSET, TRACE_INDEX_OFFSET)
    ACCESSORS_SYNCHRONIZED_PRIMITIVE_FIELD(TraceIndex, uint32_t, TRACE_INDEX_OFFSET, TRACK_INFO_OFFSET)
    ACCESSORS_SYNCHRONIZED(TrackInfo, TRACK_INFO_OFFSET, MOD_RECORD_OFFSET)
    ACCESSORS_SYNCHRONIZED_PRIMITIVE_FIELD(ModRecord, uint32_t, MOD_RECORD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    static constexpr uint32_t MAX_INLINE = PropertyAttributes::MAX_FAST_PROPS_CAPACITY -
        SIZE / JSTaggedValue::TaggedTypeSize() + 1;
    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, TRACK_INFO_OFFSET, MOD_RECORD_OFFSET)

    static const uint32_t MAX_ARRAY_INDEX = MAX_ELEMENT_INDEX;
    DECL_DUMP()

    static int32_t GetArrayLengthOffset()
    {
        return LENGTH_OFFSET;
    }

    static bool PropertyKeyToArrayIndex(JSThread *thread, const JSHandle<JSTaggedValue> &key, uint32_t *output);

    static JSTaggedValue LengthGetter(JSThread *thread, const JSHandle<JSObject> &self);

    static bool LengthSetter(JSThread *thread, const JSHandle<JSObject> &self, const JSHandle<JSTaggedValue> &value,
                             bool mayThrow = false);
    static bool DummyLengthSetter(JSThread *thread, const JSHandle<JSObject> &self,
                                  const JSHandle<JSTaggedValue> &value, bool mayThrow = false);

    static JSHandle<JSTaggedValue> FastGetPropertyByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                          uint32_t index);

    static JSHandle<JSTaggedValue> FastGetPropertyByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                          const JSHandle<JSTaggedValue> &key,
                                                          SCheckMode sCheckMode = SCheckMode::SKIP);

    static bool FastSetPropertyByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t index,
                                       const JSHandle<JSTaggedValue> &value);

    static bool FastSetPropertyByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                       const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value);

    static OperationResult GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                      const JSHandle<JSTaggedValue> &key, SCheckMode sCheckMode);
    static bool SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                           const JSHandle<JSTaggedValue> &value, bool mayThrow, SCheckMode sCheckMode);
    static bool SetProperty(JSThread *thread,
                            const JSHandle<JSTaggedValue> &obj,
                            uint32_t index,
                            const JSHandle<JSTaggedValue> &value,
                            bool mayThrow,
                            SCheckMode sCheckMode);

    static JSTaggedValue Sort(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &fn);
    static bool IncludeInSortedValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                     const JSHandle<JSTaggedValue> &value);
    static JSHandle<TaggedArray> ToTaggedArray(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    static void PUBLIC_API CheckAndCopyArray(const JSThread *thread, JSHandle<JSSharedArray> obj);
    static JSHandle<TaggedArray> SetCapacity(const JSThread *thread, const JSHandle<TaggedArray> &array,
                                             uint32_t capa);
    static void SetCapacity(JSThread *thread, const JSHandle<JSObject> &array, uint32_t oldLen, uint32_t newLen,
                            bool isNew = false);
    static void SortElements(JSThread *thread, const JSHandle<TaggedArray> &elements,
                             const JSHandle<JSTaggedValue> &fn);
    static void SortElementsByObject(JSThread *thread, const JSHandle<JSObject> &thisObjHandle,
                                     const JSHandle<JSTaggedValue> &fn);

    static void DeleteInElementMode(const JSThread *thread, JSHandle<JSSharedArray> &obj);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_SHARED_OBJECTS_JS_SHARED_ARRAY_H
