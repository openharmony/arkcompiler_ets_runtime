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

#ifndef ECMASCRIPT_TEMPLATE_MAP_H
#define ECMASCRIPT_TEMPLATE_MAP_H

#include "ecmascript/js_array.h"
#include "ecmascript/tagged_hash_table.h"

namespace panda::ecmascript {
class TemplateMap : public TaggedHashTable<TemplateMap> {
public:
    using HashTable = TaggedHashTable<TemplateMap>;
    static inline bool IsMatch([[maybe_unused]] const JSThread *thread, const JSTaggedValue &key,
                               const JSTaggedValue &other)
    {
        return key == other;
    }
    static inline int Hash([[maybe_unused]] const JSThread *thread, const JSTaggedValue &obj)
    {
        ASSERT(obj.IsJSArray());
        JSArray *array = JSArray::Cast(obj.GetTaggedObject());
        uint32_t len = array->GetArrayLength();
        return len;
    }
    inline static int GetKeyIndex(int entry)
    {
        return HashTable::TABLE_HEADER_SIZE + entry * GetEntrySize() + ENTRY_KEY_INDEX;
    }
    inline static int GetValueIndex(int entry)
    {
        return HashTable::TABLE_HEADER_SIZE + entry * GetEntrySize() + ENTRY_VALUE_INDEX;
    }
    inline static int GetEntryIndex(int entry)
    {
        return HashTable::TABLE_HEADER_SIZE + entry * GetEntrySize();
    }
    inline static int GetEntrySize()
    {
        return ENTRY_SIZE;
    }
    static TemplateMap *Cast(TaggedObject *object)
    {
        return reinterpret_cast<TemplateMap *>(object);
    }
    static const int DEFAULT_ELEMENTS_NUMBER = 16;
    static JSHandle<TemplateMap> Create(JSThread *thread, int numberOfElements = DEFAULT_ELEMENTS_NUMBER)
    {
        return HashTable::Create(thread, numberOfElements);
    }
    static int ComputeCompactSize([[maybe_unused]] const JSThread *thread,
                                  [[maybe_unused]] const JSHandle<TemplateMap> &table, int computeHashTableSize,
                                  [[maybe_unused]] int tableSize, [[maybe_unused]] int addedElements)
    {
        return computeHashTableSize;
    }
    static const int ENTRY_SIZE = 2;
    static const int ENTRY_KEY_INDEX = 0;
    static const int ENTRY_VALUE_INDEX = 1;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TEMPLATE_MAP_H
