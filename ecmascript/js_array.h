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

#ifndef ECMASCRIPT_JSARRAY_H
#define ECMASCRIPT_JSARRAY_H

#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript {
enum class ArrayMode : uint8_t { UNDEFINED = 0, DICTIONARY, LITERAL };
// ecma6 9.4.2 Array Exotic Object
class JSArray : public JSObject {
public:
    // array instance property:
    static constexpr int LENGTH_INLINE_PROPERTY_INDEX = 0;
    // array prototype property:
    static constexpr int CONSTRUCTOR_INLINE_PROPERTY_INDEX = 1;
    // array constructor property:
    static constexpr int ARRAY_FUNCTION_INLINE_PROPERTY_NUM = 7;
    static constexpr int ARRAY_FUNCTION_SPECIES_INDEX = 0;
    CAST_CHECK(JSArray, IsJSArray);

    static PUBLIC_API JSHandle<JSTaggedValue> ArrayCreate(JSThread *thread, JSTaggedNumber length,
                                                          ArrayMode mode = ArrayMode::UNDEFINED);
    static JSHandle<JSTaggedValue> ArrayCreate(JSThread *thread, JSTaggedNumber length,
                                               const JSHandle<JSTaggedValue> &newTarget,
                                               ArrayMode mode = ArrayMode::UNDEFINED);
    static JSTaggedValue GetConstructorOrSpeciesInlinedProp(JSTaggedValue object, uint32_t inlinePropIndex);
    static JSTaggedValue ArraySpeciesCreate(JSThread *thread, const JSHandle<JSObject> &originalArray,
                                            JSTaggedNumber length);
    static bool ArraySetLength(JSThread *thread, const JSHandle<JSObject> &array, const PropertyDescriptor &desc);
    static bool DefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &array, const JSHandle<JSTaggedValue> &key,
                                  const PropertyDescriptor &desc);
    static bool DefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &array, uint32_t index,
                                  const PropertyDescriptor &desc);

    static bool IsLengthString(JSThread *thread, const JSHandle<JSTaggedValue> &key);
    static bool IsProtoNotChangeJSArray(JSThread *thread, const JSHandle<JSObject> &obj);
    static bool IsProtoNotModifiedDictionaryJSArray(JSThread *thread, const JSHandle<JSObject> &obj);
    // ecma6 7.3 Operations on Objects
    static JSHandle<JSArray> CreateArrayFromList(JSThread *thread, const JSHandle<TaggedArray> &elements);
    static JSHandle<JSArray> CreateArrayFromList(JSThread *thread, const JSHandle<JSTaggedValue> &newtarget,
                                                 const JSHandle<TaggedArray> &elements);
    static JSTaggedValue FastConcatDictionaryArray(JSThread *thread, JSHandle<JSObject> obj,
        JSHandle<JSObject> &newArrayHandle, JSMutableHandle<JSTaggedValue> &fromValHandle,
        JSMutableHandle<JSTaggedValue> &toKey, int64_t &n);
    
    // use first inlined property slot for array length
    inline uint32_t GetArrayLength() const
    {
        return GetLength();
    }

    inline void SetArrayLength([[maybe_unused]]const JSThread *thread, uint32_t length)
    {
        SetLength(length);
    }

    inline uint32_t GetHintLength() const
    {
        auto trackInfo = GetTrackInfo();
        if (trackInfo.IsInt()) {
            int hint = trackInfo.GetInt();
            return hint > 0 ? hint : 0;
        }
        return 0;
    }

    static constexpr size_t LENGTH_OFFSET = JSObject::SIZE;
    ACCESSORS_PRIMITIVE_FIELD(Length, uint32_t, LENGTH_OFFSET, TRACE_INDEX_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(TraceIndex, uint32_t, TRACE_INDEX_OFFSET, TRACK_INFO_OFFSET)
    ACCESSORS(TrackInfo, TRACK_INFO_OFFSET, SIZE)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, TRACK_INFO_OFFSET, SIZE)

    static const uint32_t MAX_ARRAY_INDEX = MAX_ELEMENT_INDEX;
    DECL_DUMP()

    static uint32_t GetInlinedPropertyOffset(uint32_t index)
    {
        return JSArray::SIZE + index * JSTaggedValue::TaggedTypeSize();
    }

    static int32_t GetArrayLengthOffset()
    {
        return LENGTH_OFFSET;
    }

    static bool PropertyKeyToArrayIndex(JSThread *thread, const JSHandle<JSTaggedValue> &key, uint32_t *output);

    static JSTaggedValue LengthGetter(JSThread *thread, const JSHandle<JSObject> &self);

    static bool LengthSetter(JSThread *thread, const JSHandle<JSObject> &self, const JSHandle<JSTaggedValue> &value,
                             bool mayThrow = false);

    static JSHandle<JSTaggedValue> FastGetPropertyByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                          uint32_t index);

    static JSHandle<JSTaggedValue> FastGetPropertyByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                          const JSHandle<JSTaggedValue> &key);

    static bool FastSetPropertyByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t index,
                                       const JSHandle<JSTaggedValue> &value);

    static bool FastSetPropertyByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                       const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value);

    static bool TryFastCreateDataProperty(JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index,
                                          const JSHandle<JSTaggedValue> &value,
                                          SCheckMode sCheckMode = SCheckMode::CHECK);

    static JSTaggedValue Sort(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &fn);
    static JSTaggedValue CopySortedListToReceiver(JSThread *thread, const JSHandle<JSTaggedValue> &thisObjVal,
                                                  JSHandle<TaggedArray> sortedList, uint32_t len);
    static bool IncludeInSortedValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                     const JSHandle<JSTaggedValue> &value);
    static JSHandle<TaggedArray> ToTaggedArray(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    static void PUBLIC_API CheckAndCopyArray(const JSThread *thread, JSHandle<JSArray> obj);
    static void SetCapacity(JSThread *thread, const JSHandle<JSObject> &array, uint32_t oldLen, uint32_t newLen,
                            bool isNew = false);
    static void TransformElementsKindAfterSetCapacity(JSThread *thread, const JSHandle<JSObject> &array,
                                                      [[maybe_unused]] uint32_t oldLen, uint32_t newLen,
                                                      [[maybe_unused]] bool isNew);
    static void SortElements(JSThread *thread, const JSHandle<TaggedArray> &elements,
                             const JSHandle<JSTaggedValue> &fn);
    static void SortElementsByObject(JSThread *thread, const JSHandle<JSObject> &thisObjHandle,
                                     const JSHandle<JSTaggedValue> &fn);
    static void SortElementsByInsertionSort(JSThread *thread, const JSHandle<TaggedArray> &elements, uint32_t len,
        const JSHandle<JSTaggedValue> &fn);
    static void SortElementsByMergeSort(JSThread *thread, const JSHandle<TaggedArray> &elements,
        const JSHandle<JSTaggedValue> &fn, int64_t startIdx, int64_t endIdx);
    static void MergeSortedElements(JSThread *thread, const JSHandle<TaggedArray> &elements,
        const JSHandle<JSTaggedValue> &fn, int64_t startIdx, int64_t middleIdx, int64_t endIdx);

    static JSHandle<JSHClass> CreateJSArrayPrototypeClass(const JSThread *thread, ObjectFactory *factory,
                                                          JSHandle<JSTaggedValue> proto, uint32_t inlinedProps);

    static JSHandle<JSHClass> CreateJSArrayFunctionClass(const JSThread *thread, ObjectFactory *factory,
                                                         const JSHandle<GlobalEnv> &env);

    template <class Callback>
    static JSTaggedValue ArrayCreateWithInit(JSThread *thread, uint32_t length, const Callback &cb)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<TaggedArray> newElements(factory->NewTaggedArray(length));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<JSTaggedValue> array(JSArray::CreateArrayFromList(thread, newElements));
        cb(newElements, length);
        return array.GetTaggedValue();
    }
};

class TrackInfo : public TaggedObject {
public:
    static TrackInfo *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsTrackInfoObject());
        return static_cast<TrackInfo *>(object);
    }

    static constexpr size_t CACHED_HCLASS_OFFSET = TaggedObjectSize();
    ACCESSORS(CachedHClass, CACHED_HCLASS_OFFSET, CACHED_FUNC_OFFSET);
    ACCESSORS(CachedFunc, CACHED_FUNC_OFFSET, BIT_FIELD_OFFSET);
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, ARRAY_LENGTH_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(ArrayLength, uint32_t, ARRAY_LENGTH_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define BitField
    static constexpr size_t ELEMENTS_KIND_BITS = 8;
    static constexpr size_t SPACE_FALG_BITS = 8;
    FIRST_BIT_FIELD(BitField, ElementsKind, ElementsKind, ELEMENTS_KIND_BITS);
    NEXT_BIT_FIELD(BitField, SpaceFlag, RegionSpaceFlag, SPACE_FALG_BITS, ElementsKind);

    DECL_DUMP()

    DECL_VISIT_OBJECT(CACHED_HCLASS_OFFSET, BIT_FIELD_OFFSET);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JSARRAY_H
