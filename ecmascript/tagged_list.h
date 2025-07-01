/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_TAGGED_LIST_H
#define ECMASCRIPT_TAGGED_LIST_H

#include "ecmascript/js_handle.h"
#include "ecmascript/js_symbol.h"
#include "ecmascript/js_tagged_number.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript {
template<typename Derived>
class TaggedList : public TaggedArray {
public:
    static const int NUMBER_OF_NODE_INDEX = 0;
    static const int NUMBER_OF_DELETED_NODES_INDEX = 1;
    static const int HEAD_TABLE_INDEX = 2;
    static const int TAIL_TABLE_INDEX = 3;
    static const int ELEMENTS_START_INDEX = 4;
    static const int DEFAULT_ARRAY_LENGHT = 10;
    static const int NEXT_PTR_OFFSET = 1;
    static const int PREV_PTR_OFFSET = 2;

    static JSHandle<Derived> Create(const JSThread *thread, int numberOfNodes = DEFAULT_ARRAY_LENGHT);
    static JSHandle<Derived> GrowCapacity(const JSThread *thread, const JSHandle<Derived> &taggedList);
    static JSTaggedValue TaggedListToArray(const JSThread *thread, const JSHandle<Derived> &taggedList);
    static JSHandle<TaggedArray> OwnKeys(JSThread *thread, const JSHandle<Derived> &taggedList);
    void CopyArray(const JSThread *thread, JSHandle<Derived> &taggedList);
    void Clear(const JSThread *thread);
    JSTaggedValue FindElementByIndex(const JSThread *thread, int index) const;
    std::pair<int, JSTaggedValue> FindElementByDataIndex(const JSThread *thread, int dataindex) const;
    int FindIndexByElement(const JSThread *thread, const JSTaggedValue &element);
    int FindLastIndexByElement(const JSThread *thread, const JSTaggedValue &element);
    int FindDataIndexByNodeIndex(int index) const;
    void MapNodeIndexToDataIndex(std::vector<int> &nodeIndexMapToDataIndex, int length);
    void RemoveNode(JSThread *thread, int prevDataIndex);
    int FindPrevNodeByIndex(int index) const;
    int FindPrevNodeByValue(const JSThread *thread, const JSTaggedValue &element);
    JSTaggedValue RemoveByIndex(JSThread *thread, const int &index);
    inline int Length()
    {
        return NumberOfNodes();
    }

    inline JSTaggedValue GetFirst(const JSThread *thread)
    {
        int firstDataIndex = GetPrimitiveElement(ELEMENTS_START_INDEX + NEXT_PTR_OFFSET).GetInt();
        return GetElement(thread, firstDataIndex);
    }

    inline JSTaggedValue GetLast(const JSThread *thread)
    {
        int lastDataIndex = GetPrimitiveElement(TAIL_TABLE_INDEX).GetInt();
        return GetElement(thread, lastDataIndex);
    }

    inline int GetCapacityFromTaggedArray()
    {
        return static_cast<int>(GetLength());
    }

    inline void SetElement(const JSThread *thread, int index, const JSTaggedValue &element)
    {
        if (UNLIKELY((index < 0 || index >= static_cast<int>(GetLength())))) {
            return;
        }
        Set(thread, index, element);
    }

    inline JSTaggedValue GetElement(const JSThread *thread, int index) const
    {
        if (UNLIKELY((index < 0 || index >= static_cast<int>(GetLength())))) {
            return JSTaggedValue::Undefined();
        }
        return Get(thread, index);
    }

    inline JSTaggedValue GetPrimitiveElement(int index) const
    {
        if (UNLIKELY((index < 0 || index >= static_cast<int>(GetLength())))) {
            return JSTaggedValue::Undefined();
        }
        return GetPrimitive(index);
    }

    inline int NumberOfNodes() const
    {
        return GetPrimitive(NUMBER_OF_NODE_INDEX).GetInt();
    }

    inline int NumberOfDeletedNodes() const
    {
        return GetPrimitive(NUMBER_OF_DELETED_NODES_INDEX).GetInt();
    }

    inline void SetNumberOfDeletedNodes(const JSThread *thread, int nod)
    {
        Set(thread, NUMBER_OF_DELETED_NODES_INDEX, JSTaggedValue(nod));
    }

    inline void SetNumberOfNodes(const JSThread *thread, int nof)
    {
        Set(thread, NUMBER_OF_NODE_INDEX, JSTaggedValue(nof));
    }

    int GetNextDataIndex(int dataIndex) const
    {
        dataIndex = GetPrimitiveElement(dataIndex + NEXT_PTR_OFFSET).GetInt();
        if (dataIndex != ELEMENTS_START_INDEX) {
            return dataIndex;
        }
        return -1;
    }
};

class TaggedSingleList : public TaggedList<TaggedSingleList> {
public:
    static const int ENTRY_SIZE = 2;
    static TaggedSingleList *Cast(TaggedObject *obj)
    {
        return static_cast<TaggedSingleList *>(obj);
    }

    static JSTaggedValue Create(const JSThread *thread, int numberOfElements = TaggedSingleList::DEFAULT_ARRAY_LENGHT);
    static JSTaggedValue Add(const JSThread *thread, const JSHandle<TaggedSingleList> &taggedList,
                             const JSHandle<JSTaggedValue> &value);
    static JSTaggedValue Insert(JSThread *thread, const JSHandle<TaggedSingleList> &taggedList,
                                const JSHandle<JSTaggedValue> &value, const int index);
    static JSTaggedValue AddNode(const JSThread *thread, const JSHandle<TaggedSingleList> &taggedList,
                                 const JSHandle<JSTaggedValue> &value, const int index, int prevDataIndex);
    static JSTaggedValue Set(JSThread *thread, const JSHandle<TaggedSingleList> &taggedList,
                             const int index, const JSHandle<JSTaggedValue> &value);
    static JSTaggedValue ReplaceAllElements(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle,
                                            const JSHandle<JSTaggedValue> &callbackFn,
                                            const JSHandle<JSTaggedValue> &thisArg,
                                            const JSHandle<TaggedSingleList> &taggedList);
    static JSTaggedValue Sort(JSThread *thread, const JSHandle<JSTaggedValue> &callbackFn,
                              const JSHandle<TaggedSingleList> &taggedList);
    static JSTaggedValue ConvertToArray(const JSThread *thread, const JSHandle<TaggedSingleList> &taggedList);
    static void GetSubList(JSThread *thread, const JSHandle<TaggedSingleList> &taggedList,
                           const int fromIndex, const int toIndex, const JSHandle<TaggedSingleList> &subList);
    static JSHandle<TaggedArray> OwnKeys(JSThread *thread, const JSHandle<TaggedSingleList> &taggedList);
    static JSTaggedValue SortByNodeOrder(const JSThread *thread, const JSHandle<TaggedSingleList> &taggedList);

    void Clear(const JSThread *thread);
    bool IsEmpty() const;
    bool Has(const JSThread *thread, const JSTaggedValue &value);
    JSTaggedValue Get(const JSThread *thread, const int index);
    std::pair<int, JSTaggedValue> GetByDataIndex(const JSThread *thread, const int dataIndex);
    int GetIndexOf(const JSThread *thread, const JSTaggedValue &value);
    int GetLastIndexOf(const JSThread *thread, const JSTaggedValue &value);
    void InsertNode(const JSThread *thread, const JSHandle<JSTaggedValue> &value, const int prevDataIndex,
                    const int finalDataIndex);
    JSTaggedValue RemoveByIndex(JSThread *thread, const int &index);
    JSTaggedValue Remove(JSThread *thread, const JSTaggedValue &element);
    JSTaggedValue Equal(const JSThread *thread, const JSHandle<TaggedSingleList> &compareList);
    DECL_DUMP()
};

class TaggedDoubleList : public TaggedList<TaggedDoubleList> {
public:
    static const int ENTRY_SIZE = 3;
    static TaggedDoubleList *Cast(TaggedObject *obj)
    {
        return static_cast<TaggedDoubleList *>(obj);
    }

    static JSTaggedValue Create(const JSThread *thread, int numberOfElements = TaggedDoubleList::DEFAULT_ARRAY_LENGHT);
    static JSTaggedValue Add(const JSThread *thread, const JSHandle<TaggedDoubleList> &taggedList,
                             const JSHandle<JSTaggedValue> &value);
    static JSTaggedValue AddFirst(const JSThread *thread, const JSHandle<TaggedDoubleList> &taggedList,
                                  const JSHandle<JSTaggedValue> &value);
    static JSTaggedValue Insert(JSThread *thread, const JSHandle<TaggedDoubleList> &taggedList,
                                const JSHandle<JSTaggedValue> &value, const int index);
    static JSTaggedValue AddNode(const JSThread *thread, const JSHandle<TaggedDoubleList> &taggedList,
                                 const JSHandle<JSTaggedValue> &value, const int index, int prevDataIndex);
    static JSTaggedValue Set(JSThread *thread, const JSHandle<TaggedDoubleList> &taggedList, const int index,
                             const JSHandle<JSTaggedValue> &value);
    static JSTaggedValue ConvertToArray(const JSThread *thread, const JSHandle<TaggedDoubleList> &taggedList);
    static JSHandle<TaggedArray> OwnKeys(JSThread *thread, const JSHandle<TaggedDoubleList> &taggedList);
    static JSTaggedValue RemoveFirstFound(JSThread *thread, const JSHandle<TaggedDoubleList> &taggedList,
                                          const JSTaggedValue &element);
    static JSTaggedValue RemoveLastFound(JSThread *thread, const JSHandle<TaggedDoubleList> &taggedList,
                                         const JSTaggedValue &element);
    void Clear(const JSThread *thread);
    JSTaggedValue Get(const JSThread *thread, const int index);
    std::pair<int, JSTaggedValue> GetByDataIndex(const JSThread *thread, const int dataIndex);
    int GetPrevNode(const int index);
    bool Has(const JSThread *thread, const JSTaggedValue &value);
    void InsertNode(const JSThread *thread, const JSHandle<JSTaggedValue> &value, const int prevDataIndex,
                    const int finalDataIndex);
    JSTaggedValue RemoveFirst(JSThread *thread);
    JSTaggedValue RemoveLast(JSThread *thread);
    JSTaggedValue RemoveByIndex(JSThread *thread, const int &index);
    JSTaggedValue Remove(JSThread *thread, const JSTaggedValue &element);
    int GetIndexOf(const JSThread *thread, const JSTaggedValue &value);
    int GetLastIndexOf(const JSThread *thread, const JSTaggedValue &value);
    int FindPrevNodeByIndexAtLast(const int index) const;
    int FindPrevNodeByValueAtLast(const JSThread *thread, const JSTaggedValue &element);
    DECL_DUMP()

protected:
    inline JSTaggedValue FindElementByIndexAtLast(const JSThread *thread, int index) const;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TAGGED_LIST_H
