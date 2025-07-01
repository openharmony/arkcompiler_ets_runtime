/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JS_API_JS_API_LIGHTWEIGHTSET_H
#define ECMASCRIPT_JS_API_JS_API_LIGHTWEIGHTSET_H

#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript {
class JSAPILightWeightSet : public JSObject {
public:
    static constexpr int DEFAULT_CAPACITY_LENGTH = 8;
    static constexpr int32_t HASH_REBELLION = 0xFFFFFFFF;
    static JSAPILightWeightSet *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsJSAPILightWeightSet());
        return static_cast<JSAPILightWeightSet *>(object);
    }
    static bool Add(JSThread *thread, const JSHandle<JSAPILightWeightSet> &obj, const JSHandle<JSTaggedValue> &value);
    static JSHandle<TaggedArray> CreateSlot(const JSThread *thread, const uint32_t capacity);
    static void EnsureCapacity(const JSThread *thread, const JSHandle<JSAPILightWeightSet> &obj,
                               uint32_t minimumCapacity);
    static void SizeCopy(const JSThread *thread, const JSHandle<JSAPILightWeightSet> &obj, uint32_t capacity,
                         uint32_t newCapacity);
    static bool AddAll(JSThread *thread, const JSHandle<JSAPILightWeightSet> &obj,
                       const JSHandle<JSTaggedValue> &value);
    static bool Equal(JSThread *thread, const JSHandle<JSAPILightWeightSet> &obj,
                      const JSHandle<JSTaggedValue> &value);
    static void IncreaseCapacityTo(JSThread *thread, const JSHandle<JSAPILightWeightSet> &obj, int32_t minCapacity);
    static JSTaggedValue ToString(JSThread *thread, const JSHandle<JSAPILightWeightSet> &obj);
    static JSTaggedValue ForEach(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle,
                                 const JSHandle<JSTaggedValue> &callbackFn, const JSHandle<JSTaggedValue> &thisArg);
    static JSHandle<JSTaggedValue> GetIteratorObj(JSThread *thread, const JSHandle<JSAPILightWeightSet> &obj,
                                                  IterationKind kind);
    void AdjustArray(JSThread *thread, JSHandle<TaggedArray> srcArray, uint32_t fromIndex, uint32_t toIndex,
                     bool direction);
    void Clear(JSThread *thread);
    JSTaggedValue Get(const JSThread *thread, const uint32_t index);
    JSTaggedValue GetHashAt(const JSThread *thread, int32_t index);
    JSTaggedValue GetValueAt(const JSThread *thread, int32_t index);
    JSTaggedValue Remove(JSThread *thread, JSHandle<JSTaggedValue> &value);
    bool Has(const JSThread *thread, const JSHandle<JSTaggedValue> &value);
    bool HasHash(const JSThread *thread, const JSHandle<JSTaggedValue> &hashCode);
    bool HasAll(const JSThread *thread, const JSHandle<JSTaggedValue> &value);
    bool RemoveAt(JSThread *thread, int32_t index);
    void RemoveValue(const JSThread *thread, JSHandle<TaggedArray> &taggedArray,
                     uint32_t index, bool isHash = false);
    bool IsEmpty();
    int32_t GetIndexOf(const JSThread *thread, JSHandle<JSTaggedValue> &value);
    int32_t BinarySearchHashes(const JSThread *thread, uint32_t hash, int32_t size);
    int32_t GetHashIndex(const JSThread *thread, const JSHandle<JSTaggedValue> &value, int32_t size);
    uint32_t Hash(const JSThread *thread, JSTaggedValue key);
    inline uint32_t GetSize() const
    {
        return GetLength();
    }
    static void CheckAndCopyValues(const JSThread *thread, JSHandle<JSAPILightWeightSet> obj);
    static constexpr size_t HASHES_OFFSET = JSObject::SIZE;
    ACCESSORS(Hashes, HASHES_OFFSET, VALUES_OFFSET);
    ACCESSORS(Values, VALUES_OFFSET, LENGTH_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(Length, uint32_t, LENGTH_OFFSET, LAST_OFFSET);
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, HASHES_OFFSET, LENGTH_OFFSET)
    DECL_DUMP()
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_API_JS_API_LIGHTWEIGHTSET_H
