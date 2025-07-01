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

#include "ecmascript/tagged_list.h"

#include "ecmascript/base/array_helper.h"
#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_object-inl.h"

namespace panda::ecmascript {
template <typename Derived>
JSHandle<Derived> TaggedList<Derived>::Create(const JSThread *thread, int numberOfNodes)
{
    ASSERT_PRINT(numberOfNodes > 0, "size must be a non-negative integer");
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    int length = ELEMENTS_START_INDEX + Derived::ENTRY_SIZE + numberOfNodes * Derived::ENTRY_SIZE;
    JSHandle<TaggedArray> taggedArray = factory->NewTaggedArray(length);
    auto taggedList = JSHandle<Derived>::Cast(taggedArray);
    JSTaggedValue data = JSTaggedValue(ELEMENTS_START_INDEX);
    taggedList->SetNumberOfNodes(thread, 0);
    taggedList->SetNumberOfDeletedNodes(thread, 0);
    taggedList->SetElement(thread, HEAD_TABLE_INDEX, data);
    taggedList->SetElement(thread, TAIL_TABLE_INDEX, data);
    taggedList->SetElement(thread, ELEMENTS_START_INDEX, JSTaggedValue::Hole());
    taggedList->SetElement(thread, ELEMENTS_START_INDEX + NEXT_PTR_OFFSET, data);
    return taggedList;
}

template <typename Derived>
void TaggedList<Derived>::CopyArray(const JSThread *thread, JSHandle<Derived> &taggedList)
{
    int deleteNodeNum = NumberOfDeletedNodes();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    if (deleteNodeNum == 0) {
        int capacity = GetCapacityFromTaggedArray();
        for (int i = 0; i < capacity; i++) {
            value.Update(GetElement(thread, i));
            taggedList->SetElement(thread, i, value.GetTaggedValue());
        }
        taggedList->SetNumberOfDeletedNodes(thread, NumberOfDeletedNodes());
        return;
    }

    int actualNodeNum = NumberOfNodes();
    int tailTableIndex = ELEMENTS_START_INDEX + actualNodeNum * Derived::ENTRY_SIZE;
    int nextTailIndex = ELEMENTS_START_INDEX + Derived::ENTRY_SIZE;
    JSTaggedValue data = JSTaggedValue(ELEMENTS_START_INDEX);
    taggedList->SetNumberOfNodes(thread, actualNodeNum);
    taggedList->SetNumberOfDeletedNodes(thread, 0);
    taggedList->SetElement(thread, HEAD_TABLE_INDEX, data);
    taggedList->SetElement(thread, TAIL_TABLE_INDEX, JSTaggedValue(tailTableIndex));
    taggedList->SetElement(thread, ELEMENTS_START_INDEX, JSTaggedValue::Hole());
    taggedList->SetElement(thread, ELEMENTS_START_INDEX + NEXT_PTR_OFFSET, JSTaggedValue(nextTailIndex));
    if constexpr (std::is_same_v<TaggedDoubleList, Derived>) {
        taggedList->SetElement(thread, ELEMENTS_START_INDEX + PREV_PTR_OFFSET, JSTaggedValue(tailTableIndex));
    }
    int srcDataIndex = GetPrimitiveElement(ELEMENTS_START_INDEX + NEXT_PTR_OFFSET).GetInt();
    for (int i = 0; i < actualNodeNum; i++) {
        int index = nextTailIndex + i * Derived::ENTRY_SIZE;
        taggedList->SetElement(thread, index, GetElement(thread, srcDataIndex));
        taggedList->SetElement(thread, index + NEXT_PTR_OFFSET, JSTaggedValue(index + Derived::ENTRY_SIZE));
        if constexpr (std::is_same_v<TaggedDoubleList, Derived>) {
            taggedList->SetElement(thread, index + PREV_PTR_OFFSET,
                                   JSTaggedValue(ELEMENTS_START_INDEX + i * Derived::ENTRY_SIZE));
        }
        srcDataIndex = GetPrimitiveElement(srcDataIndex + NEXT_PTR_OFFSET).GetInt();
    }
    taggedList->SetElement(thread, tailTableIndex + NEXT_PTR_OFFSET, JSTaggedValue(ELEMENTS_START_INDEX));
}

template <typename Derived>
JSHandle<Derived> TaggedList<Derived>::GrowCapacity(const JSThread *thread, const JSHandle<Derived> &taggedList)
{
    int actualNodeNum = taggedList->NumberOfNodes();
    int deleteNodeNum = taggedList->NumberOfDeletedNodes();
    int needCapacity = actualNodeNum + 1;
    int taggedArrayLength = taggedList->GetCapacityFromTaggedArray();
    int actualArrayCapacity = (taggedArrayLength - ELEMENTS_START_INDEX - (deleteNodeNum + 1) * Derived::ENTRY_SIZE);
    if (needCapacity * Derived::ENTRY_SIZE < actualArrayCapacity) {
        return taggedList;
    }
    uint32_t length = static_cast<uint32_t>(actualNodeNum);
    uint32_t newCapacity = length + (length >> 1UL);
    JSHandle<Derived> list = Create(thread, newCapacity < DEFAULT_ARRAY_LENGHT ? DEFAULT_ARRAY_LENGHT : newCapacity);
    taggedList->CopyArray(thread, list);
    return list;
}

template <typename Derived>
void TaggedList<Derived>::Clear(const JSThread *thread)
{
    int numberOfNodes = NumberOfNodes();
    int dataIndex = ELEMENTS_START_INDEX;
    for (int i = 0; i < numberOfNodes; i++) {
        dataIndex = GetPrimitiveElement(dataIndex + NEXT_PTR_OFFSET).GetInt();
        SetElement(thread, dataIndex, JSTaggedValue::Hole());
    }
    JSTaggedValue data = JSTaggedValue(ELEMENTS_START_INDEX);
    SetNumberOfNodes(thread, 0);
    SetNumberOfDeletedNodes(thread, 0);
    SetElement(thread, HEAD_TABLE_INDEX, data);
    SetElement(thread, TAIL_TABLE_INDEX, data);
    SetElement(thread, ELEMENTS_START_INDEX, JSTaggedValue::Hole());
    SetElement(thread, ELEMENTS_START_INDEX + NEXT_PTR_OFFSET, data);
}

template <typename Derived>
JSTaggedValue TaggedList<Derived>::TaggedListToArray(const JSThread *thread, const JSHandle<Derived> &list)
{
    uint32_t numberOfNodes = static_cast<uint32_t>(list->NumberOfNodes());

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSArray> array = factory->NewJSArray();
    array->SetArrayLength(thread, numberOfNodes);
    if (numberOfNodes == 0) {
        return array.GetTaggedValue();
    }
    JSHandle<TaggedArray> newElements = factory->ConvertListToArray(thread, list, numberOfNodes);
    array->SetElements(thread, newElements);
    return array.GetTaggedValue();
}

template <typename Derived>
JSHandle<TaggedArray> TaggedList<Derived>::OwnKeys(JSThread *thread, const JSHandle<Derived> &list)
{
    uint32_t length = static_cast<uint32_t>(list->NumberOfNodes());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> keys = factory->NewTaggedArray(length);

    for (uint32_t i = 0; i < length; i++) {
        auto key = base::NumberHelper::IntToEcmaString(thread, i);
        keys->Set(thread, i, key);
    }
    return keys;
}

template<typename Derived>
int TaggedList<Derived>::FindIndexByElement(const JSThread *thread, const JSTaggedValue &element)
{
    int dataIndex = ELEMENTS_START_INDEX;
    int nextDataIndex = GetPrimitiveElement(dataIndex + NEXT_PTR_OFFSET).GetInt();
    int nodeSum = 0;
    while (nextDataIndex != ELEMENTS_START_INDEX) {
        dataIndex = nextDataIndex;
        JSTaggedValue data = GetElement(thread, dataIndex);
        nextDataIndex = GetPrimitiveElement(nextDataIndex + NEXT_PTR_OFFSET).GetInt();
        if (JSTaggedValue::SameValue(thread, data, element)) {
            return nodeSum;
        }
        nodeSum++;
    }
    return -1;
}

template<typename Derived>
int TaggedList<Derived>::FindLastIndexByElement(const JSThread *thread, const JSTaggedValue &element)
{
    int dataIndex = ELEMENTS_START_INDEX;
    int nextIndex = GetPrimitiveElement(dataIndex + NEXT_PTR_OFFSET).GetInt();
    int nodeSum = 0;
    int lastIndex = -1;
    while (nextIndex != ELEMENTS_START_INDEX) {
        dataIndex = nextIndex;
        JSTaggedValue data = GetElement(thread, dataIndex);
        if (JSTaggedValue::SameValue(thread, data, element)) {
            lastIndex = nodeSum;
        }
        nextIndex = GetPrimitiveElement(nextIndex + NEXT_PTR_OFFSET).GetInt();
        nodeSum++;
    }
    return lastIndex;
}

template<typename Derived>
int TaggedList<Derived>::FindDataIndexByNodeIndex(int index) const
{
    int dataIndex = ELEMENTS_START_INDEX;
    int nextIndex = GetPrimitiveElement(dataIndex + NEXT_PTR_OFFSET).GetInt();
    int nodeSum = 0;
    while (nextIndex != ELEMENTS_START_INDEX) {
        dataIndex = nextIndex;
        if (nodeSum == index) {
            return dataIndex;
        }
        nextIndex = GetPrimitiveElement(nextIndex + NEXT_PTR_OFFSET).GetInt();
        nodeSum++;
    }
    return -1;
}

template<typename Derived>
void TaggedList<Derived>::MapNodeIndexToDataIndex(std::vector<int> &nodeIndexMapToDataIndex, int length)
{
    int i = 0;
    int nextIndex = ELEMENTS_START_INDEX;
    while (i < length) {
        nextIndex = GetPrimitiveElement(nextIndex + NEXT_PTR_OFFSET).GetInt();
        nodeIndexMapToDataIndex[i] = nextIndex;
        i++;
    }
}

template<typename Derived>
void TaggedList<Derived>::RemoveNode(JSThread *thread, int prevDataIndex)
{
    int tailTableIndex = GetPrimitiveElement(TAIL_TABLE_INDEX).GetInt();
    if (tailTableIndex != GetElement(thread, HEAD_TABLE_INDEX).GetInt()) {
        int dataIndex = GetPrimitiveElement(prevDataIndex + NEXT_PTR_OFFSET).GetInt();
        int nextDataIndex = GetPrimitiveElement(dataIndex + NEXT_PTR_OFFSET).GetInt();
        if (dataIndex == tailTableIndex) {
            SetElement(thread, TAIL_TABLE_INDEX, JSTaggedValue(prevDataIndex));
        }
        if constexpr (std::is_same_v<TaggedDoubleList, Derived>) {
            SetElement(thread, nextDataIndex + PREV_PTR_OFFSET, JSTaggedValue(prevDataIndex));
        }
        SetElement(thread, dataIndex, JSTaggedValue::Hole());
        SetElement(thread, prevDataIndex + NEXT_PTR_OFFSET, JSTaggedValue(nextDataIndex));
        SetNumberOfNodes(thread, NumberOfNodes() - 1);
        SetNumberOfDeletedNodes(thread, NumberOfDeletedNodes() + 1);
    }
}

template<typename Derived>
int TaggedList<Derived>::FindPrevNodeByIndex(int index) const
{
    int prevDataIndex = ELEMENTS_START_INDEX;
    int nodeSum = 0;
    int len = GetPrimitiveElement(NUMBER_OF_NODE_INDEX).GetInt();
    while (nodeSum <= len) {
        if (nodeSum == index) {
            return prevDataIndex;
        }
        prevDataIndex = GetPrimitiveElement(prevDataIndex + NEXT_PTR_OFFSET).GetInt();
        nodeSum++;
    }
    return -1;
}

template<typename Derived>
int TaggedList<Derived>::FindPrevNodeByValue(const JSThread *thread, const JSTaggedValue &element)
{
    int dataIndex = ELEMENTS_START_INDEX;
    int nodeSum = 0;
    int len = GetPrimitiveElement(NUMBER_OF_NODE_INDEX).GetInt();
    while (nodeSum <= len) {
        int nextDataIndex = GetPrimitiveElement(dataIndex + NEXT_PTR_OFFSET).GetInt();
        JSTaggedValue data = GetElement(thread, nextDataIndex);
        if (JSTaggedValue::SameValue(thread, data, element)) {
            return dataIndex;
        }
        dataIndex = nextDataIndex;
        nodeSum++;
    }
    return -1;
}

template<typename Derived>
JSTaggedValue TaggedList<Derived>::FindElementByIndex(const JSThread *thread, int index) const
{
    int dataIndex = GetPrimitiveElement(ELEMENTS_START_INDEX + NEXT_PTR_OFFSET).GetInt();
    int nodeSum = 0;
    while (dataIndex != ELEMENTS_START_INDEX) {
        if (nodeSum == index) {
            return GetElement(thread, dataIndex);
        }
        dataIndex = GetPrimitiveElement(dataIndex + NEXT_PTR_OFFSET).GetInt();
        nodeSum++;
    }
    return JSTaggedValue::Undefined();
}

template<typename Derived>
std::pair<int, JSTaggedValue> TaggedList<Derived>::FindElementByDataIndex(const JSThread *thread, int dataindex) const
{
    int targetDataIndex = GetPrimitiveElement(dataindex + NEXT_PTR_OFFSET).GetInt();
    JSTaggedValue value = GetElement(thread, targetDataIndex);
    while (value.IsHole() && targetDataIndex != ELEMENTS_START_INDEX) {
        targetDataIndex = GetPrimitiveElement(targetDataIndex + NEXT_PTR_OFFSET).GetInt();
        value = GetElement(thread, targetDataIndex);
    }
    if (targetDataIndex == ELEMENTS_START_INDEX) {
        return std::make_pair(-1, JSTaggedValue::Undefined());
    }
    return std::make_pair(targetDataIndex, value);
}

template<typename Derived>
JSTaggedValue TaggedList<Derived>::RemoveByIndex(JSThread *thread, const int &index)
{
    int prevDataIndex = FindPrevNodeByIndex(index);
    int curDataIndex = GetPrimitiveElement(prevDataIndex + NEXT_PTR_OFFSET).GetInt();
    JSTaggedValue data = GetElement(thread, curDataIndex);
    RemoveNode(thread, prevDataIndex);
    return data;
}

// TaggedSingleList
JSTaggedValue TaggedSingleList::Create(const JSThread *thread, int numberOfElements)
{
    return TaggedList<TaggedSingleList>::Create(thread, numberOfElements).GetTaggedValue();
}

JSTaggedValue TaggedSingleList::Add(const JSThread *thread, const JSHandle<TaggedSingleList> &taggedList,
                                    const JSHandle<JSTaggedValue> &value)
{
    int prevDataIndex = taggedList->GetPrimitiveElement(TAIL_TABLE_INDEX).GetInt();
    return TaggedSingleList::AddNode(thread, taggedList, value, -1, prevDataIndex);
}

JSTaggedValue TaggedSingleList::ConvertToArray(const JSThread *thread, const JSHandle<TaggedSingleList> &taggedList)
{
    return JSTaggedValue(TaggedList<TaggedSingleList>::TaggedListToArray(thread, taggedList));
}

JSTaggedValue TaggedSingleList::Insert(JSThread *thread, const JSHandle<TaggedSingleList> &taggedList,
                                       const JSHandle<JSTaggedValue> &value, const int index)
{
    int tailIndex = taggedList->GetPrimitiveElement(TAIL_TABLE_INDEX).GetInt();
    int prevDataIndex = (index == -1) ? tailIndex : taggedList->FindPrevNodeByIndex(index);
    return TaggedSingleList::AddNode(thread, taggedList, value, index, prevDataIndex);
}

JSTaggedValue TaggedSingleList::AddNode(const JSThread *thread, const JSHandle<TaggedSingleList> &taggedList,
                                        const JSHandle<JSTaggedValue> &value, const int index, int prevDataIndex)
{
    JSHandle<TaggedSingleList> list = GrowCapacity(thread, taggedList);
    int deleteNodeLength = list->NumberOfDeletedNodes();
    int nodeLength = list->NumberOfNodes();
    int finalDataIndex = ELEMENTS_START_INDEX + (nodeLength + 1 + deleteNodeLength) * TaggedSingleList::ENTRY_SIZE;

    if (taggedList != list) {
        if (index == -1) {
            prevDataIndex = list->GetPrimitiveElement(TAIL_TABLE_INDEX).GetInt();
        } else {
            prevDataIndex = list->FindPrevNodeByIndex(index);
        }
    }

    list->InsertNode(thread, value, prevDataIndex, finalDataIndex);
    if (index == -1 || nodeLength == index) {
        list->SetElement(thread, TAIL_TABLE_INDEX, JSTaggedValue(finalDataIndex));
    }
    return list.GetTaggedValue();
}

void TaggedSingleList::InsertNode(const JSThread *thread, const JSHandle<JSTaggedValue> &value, const int prevDataIndex,
                                  const int finalDataIndex)
{
    int prevNextIndex = prevDataIndex + NEXT_PTR_OFFSET;
    int nextDataIndex = GetPrimitiveElement(prevNextIndex).GetInt();
    SetElement(thread, prevNextIndex, JSTaggedValue(finalDataIndex));
    SetElement(thread, finalDataIndex, value.GetTaggedValue());
    SetElement(thread, finalDataIndex + 1, JSTaggedValue(nextDataIndex));
    SetNumberOfNodes(thread, NumberOfNodes() + 1);
}

bool TaggedSingleList::Has(const JSThread *thread, const JSTaggedValue &element)
{
    int dataIndex = FindIndexByElement(thread, element);
    return dataIndex != -1;
}

bool TaggedSingleList::IsEmpty() const
{
    return NumberOfNodes() == 0;
}

JSTaggedValue TaggedSingleList::Get(const JSThread *thread, const int index)
{
    return FindElementByIndex(thread, index);
}

std::pair<int, JSTaggedValue> TaggedSingleList::GetByDataIndex(const JSThread *thread, const int dataIndex)
{
    return FindElementByDataIndex(thread, dataIndex);
}

int TaggedSingleList::GetIndexOf(const JSThread *thread, const JSTaggedValue &element)
{
    return FindIndexByElement(thread, element);
}

int TaggedSingleList::GetLastIndexOf(const JSThread *thread, const JSTaggedValue &element)
{
    return FindLastIndexByElement(thread, element);
}

JSTaggedValue TaggedSingleList::Set(JSThread *thread, const JSHandle<TaggedSingleList> &taggedList,
                                    const int index, const JSHandle<JSTaggedValue> &value)
{
    int dataIndex = taggedList->FindDataIndexByNodeIndex(index);
    if (dataIndex == -1) {
        return taggedList.GetTaggedValue();
    }
    taggedList->SetElement(thread, dataIndex, value.GetTaggedValue());
    return taggedList.GetTaggedValue();
}

JSTaggedValue TaggedSingleList::ReplaceAllElements(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle,
                                                   const JSHandle<JSTaggedValue> &callbackFn,
                                                   const JSHandle<JSTaggedValue> &thisArg,
                                                   const JSHandle<TaggedSingleList> &taggedList)
{
    int length = taggedList->NumberOfNodes();
    int dataIndex = ELEMENTS_START_INDEX;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    for (int k = 0; k < length; k++) {
        dataIndex = taggedList->GetNextDataIndex(dataIndex);
        JSTaggedValue kValue = taggedList->GetElement(thread, dataIndex);
        JSTaggedValue key = JSTaggedValue(k);
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFn, thisArg, undefined, 3); // 3:three args
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(kValue, key, thisHandle.GetTaggedValue());
        JSTaggedValue funcResult = JSFunction::Call(info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult);
        JSHandle<JSTaggedValue> funcResultValue = JSHandle<JSTaggedValue>(thread, funcResult);
        TaggedSingleList::Set(thread, taggedList, k, funcResultValue);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue TaggedSingleList::Sort(JSThread *thread, const JSHandle<JSTaggedValue> &callbackFn,
                                     const JSHandle<TaggedSingleList> &taggedList)
{
    const int length = taggedList->NumberOfNodes();
    const int minSortLength = 2;
    if (length < minSortLength) {
        return JSTaggedValue::Undefined();
    }
    JSMutableHandle<JSTaggedValue> presentValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> middleValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> previousValue(thread, JSTaggedValue::Undefined());
    // create index map
    std::vector<int> nodeIndexMapToDataIndex(length, 0);
    taggedList->MapNodeIndexToDataIndex(nodeIndexMapToDataIndex, length);

    int beginIndex = 0;
    int endIndex = 0;
    int middleIndex = 0;
    double compareResult = 0;
    for (int i = 1; i < length; i++) {
        beginIndex = 0;
        endIndex = i;
        presentValue.Update(taggedList->GetElement(thread, nodeIndexMapToDataIndex[i]));
        while (beginIndex < endIndex) {
            middleIndex = (beginIndex + endIndex) / 2; // 2 : half
            middleValue.Update(taggedList->GetElement(thread, nodeIndexMapToDataIndex[middleIndex]));
            compareResult = base::ArrayHelper::SortCompare(thread, callbackFn, middleValue, presentValue);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (compareResult > 0) {
                endIndex = middleIndex;
            } else {
                beginIndex = middleIndex + 1;
            }
        }

        if (endIndex < i) {
            for (int j = i; j > endIndex; j--) {
                previousValue.Update(taggedList->GetElement(thread, nodeIndexMapToDataIndex[j - 1]));
                taggedList->SetElement(thread, nodeIndexMapToDataIndex[j], previousValue.GetTaggedValue());
            }
            taggedList->SetElement(thread, nodeIndexMapToDataIndex[endIndex], presentValue.GetTaggedValue());
        }
    }

    return JSTaggedValue::Undefined();
}

void TaggedSingleList::GetSubList(JSThread *thread, const JSHandle<TaggedSingleList> &taggedList,
                                  const int fromIndex, const int toIndex,
                                  const JSHandle<TaggedSingleList> &subList)
{
    int fromDataIndex = -1;
    int toDataIndex = -1;
    int dataIndex = taggedList->GetPrimitiveElement(ELEMENTS_START_INDEX + NEXT_PTR_OFFSET).GetInt();
    int nodeSum = 0;
    while (dataIndex != ELEMENTS_START_INDEX) {
        if (nodeSum == fromIndex) {
            fromDataIndex = dataIndex;
        }
        dataIndex = taggedList->GetPrimitiveElement(dataIndex + NEXT_PTR_OFFSET).GetInt();
        nodeSum++;
        if (nodeSum == toIndex) {
            toDataIndex = dataIndex;
            break;
        }
    }

    int preDataIndex = ELEMENTS_START_INDEX;
    JSMutableHandle<JSTaggedValue> dataHandle(thread, JSTaggedValue::Undefined());
    int curDataIndex = preDataIndex;
    while (fromDataIndex != toDataIndex) {
        curDataIndex += TaggedSingleList::ENTRY_SIZE;
        dataHandle.Update(taggedList->GetElement(thread, fromDataIndex));
        subList->SetElement(thread, preDataIndex + NEXT_PTR_OFFSET, JSTaggedValue(curDataIndex));
        subList->SetElement(thread, curDataIndex, dataHandle.GetTaggedValue());
        preDataIndex = curDataIndex;
        fromDataIndex = taggedList->GetPrimitiveElement(fromDataIndex + NEXT_PTR_OFFSET).GetInt();
    }
    subList->SetElement(thread, curDataIndex + NEXT_PTR_OFFSET, JSTaggedValue(ELEMENTS_START_INDEX));
    subList->SetElement(thread, HEAD_TABLE_INDEX, JSTaggedValue(ELEMENTS_START_INDEX));
    subList->SetElement(thread, TAIL_TABLE_INDEX, JSTaggedValue(curDataIndex));
    subList->SetNumberOfNodes(thread, toIndex - fromIndex);
    subList->SetNumberOfDeletedNodes(thread, 0);
}

JSTaggedValue TaggedSingleList::Equal(const JSThread *thread, const JSHandle<TaggedSingleList> &compareList)
{
    int compareListLength = compareList->NumberOfNodes();
    if (compareListLength != NumberOfNodes()) {
        return JSTaggedValue::False();
    }
    int nodeSum = 0;
    int compareNode = ELEMENTS_START_INDEX;
    int valueNode = ELEMENTS_START_INDEX;
    while (nodeSum < compareListLength) {
        compareNode = compareList->GetNextDataIndex(compareNode);
        valueNode = GetNextDataIndex(valueNode);
        JSTaggedValue compareValue = compareList->GetElement(thread, compareNode);
        JSTaggedValue value = GetElement(thread, valueNode);
        if (!JSTaggedValue::SameValue(thread, compareValue, value)) {
            return JSTaggedValue::False();
        }
        nodeSum++;
    }
    return JSTaggedValue::True();
}

void TaggedSingleList::Clear(const JSThread *thread)
{
    TaggedList<TaggedSingleList>::Clear(thread);
}

JSTaggedValue TaggedSingleList::RemoveByIndex(JSThread *thread, const int &index)
{
    return TaggedList<TaggedSingleList>::RemoveByIndex(thread, index);
}

JSTaggedValue TaggedSingleList::Remove(JSThread *thread, const JSTaggedValue &element)
{
    int prevDataIndex = FindPrevNodeByValue(thread, element);
    if (prevDataIndex == -1) {
        return JSTaggedValue::False();
    }
    RemoveNode(thread, prevDataIndex);
    return JSTaggedValue::True();
}

JSHandle<TaggedArray> TaggedSingleList::OwnKeys(JSThread *thread, const JSHandle<TaggedSingleList> &taggedList)
{
    return TaggedList<TaggedSingleList>::OwnKeys(thread, taggedList);
}

JSTaggedValue TaggedSingleList::SortByNodeOrder(const JSThread *thread, const JSHandle<TaggedSingleList> &taggedList)
{
    int actualNodeNum = taggedList->NumberOfNodes();
    int nextDataIndex = taggedList->GetPrimitiveElement(ELEMENTS_START_INDEX + NEXT_PTR_OFFSET).GetInt();
    uint32_t length = static_cast<uint32_t>(actualNodeNum);
    JSHandle<TaggedSingleList> list = TaggedList<TaggedSingleList>::Create(
        thread, length < DEFAULT_ARRAY_LENGHT ? DEFAULT_ARRAY_LENGHT : length);

    int tailTableIndex = ELEMENTS_START_INDEX + actualNodeNum * TaggedSingleList::ENTRY_SIZE;
    int nextTailIndex = ELEMENTS_START_INDEX + TaggedSingleList::ENTRY_SIZE;
    list->SetNumberOfNodes(thread, actualNodeNum);
    list->SetElement(thread, TAIL_TABLE_INDEX, JSTaggedValue(tailTableIndex));
    list->SetElement(thread, ELEMENTS_START_INDEX, JSTaggedValue::Hole());
    list->SetElement(thread, ELEMENTS_START_INDEX + NEXT_PTR_OFFSET, JSTaggedValue(nextTailIndex));

    for (int i = 0; i < actualNodeNum; ++i) {
        int curDataIndex = ELEMENTS_START_INDEX + (i + 1) * TaggedSingleList::ENTRY_SIZE;
        list->SetElement(thread, curDataIndex, taggedList->GetElement(thread, nextDataIndex));
        list->SetElement(thread, curDataIndex + NEXT_PTR_OFFSET,
            JSTaggedValue(curDataIndex + TaggedSingleList::ENTRY_SIZE));
        nextDataIndex = taggedList->GetPrimitiveElement(nextDataIndex + NEXT_PTR_OFFSET).GetInt();
    }
    list->SetElement(thread, tailTableIndex + NEXT_PTR_OFFSET, JSTaggedValue(ELEMENTS_START_INDEX));
    return list.GetTaggedValue();
}

// TaggedDoubleList
JSTaggedValue TaggedDoubleList::Create(const JSThread *thread, int numberOfElements)
{
    JSHandle<TaggedDoubleList> taggedList = TaggedList<TaggedDoubleList>::Create(thread, numberOfElements);
    taggedList->SetElement(thread, ELEMENTS_START_INDEX + PREV_PTR_OFFSET, JSTaggedValue(ELEMENTS_START_INDEX));
    return taggedList.GetTaggedValue();
}

JSTaggedValue TaggedDoubleList::Add(const JSThread *thread, const JSHandle<TaggedDoubleList> &taggedList,
                                    const JSHandle<JSTaggedValue> &value)
{
    int prevDataIndex = taggedList->GetPrimitiveElement(TAIL_TABLE_INDEX).GetInt();
    return TaggedDoubleList::AddNode(thread, taggedList, value, -1, prevDataIndex);
}

JSTaggedValue TaggedDoubleList::AddFirst(const JSThread *thread, const JSHandle<TaggedDoubleList> &taggedList,
                                         const JSHandle<JSTaggedValue> &value)
{
    int prevDataIndex = taggedList->FindPrevNodeByIndex(0);
    return TaggedDoubleList::AddNode(thread, taggedList, value, 0, prevDataIndex);
}

JSTaggedValue TaggedDoubleList::ConvertToArray(const JSThread *thread, const JSHandle<TaggedDoubleList> &taggedList)
{
    return JSTaggedValue(TaggedList<TaggedDoubleList>::TaggedListToArray(thread, taggedList));
}

JSTaggedValue TaggedDoubleList::Insert(JSThread *thread, const JSHandle<TaggedDoubleList> &taggedList,
                                       const JSHandle<JSTaggedValue> &value, const int index)
{
    int prevDataIndex = taggedList->GetPrevNode(index);
    return TaggedDoubleList::AddNode(thread, taggedList, value, index, prevDataIndex);
}

JSTaggedValue TaggedDoubleList::AddNode(const JSThread *thread, const JSHandle<TaggedDoubleList> &taggedList,
                                        const JSHandle<JSTaggedValue> &value, const int index, int prevDataIndex)
{
    JSHandle<TaggedDoubleList> list = GrowCapacity(thread, taggedList);
    int deleteNodeLength = list->NumberOfDeletedNodes();
    int nodeLength = list->NumberOfNodes();
    int finalDataIndex = ELEMENTS_START_INDEX + (nodeLength + 1 + deleteNodeLength) * TaggedDoubleList::ENTRY_SIZE;

    if (taggedList != list) {
        if (index == -1) {
            prevDataIndex = list->GetPrimitiveElement(TAIL_TABLE_INDEX).GetInt();
        } else if (index == 0) {
            prevDataIndex = list->FindPrevNodeByIndex(0);
        } else {
            prevDataIndex = list->GetPrevNode(index);
        }
    }

    list->InsertNode(thread, value, prevDataIndex, finalDataIndex);
    if (index == -1 || nodeLength == index) {
        list->SetElement(thread, TAIL_TABLE_INDEX, JSTaggedValue(finalDataIndex));
    }
    return list.GetTaggedValue();
}

void TaggedDoubleList::InsertNode(const JSThread *thread, const JSHandle<JSTaggedValue> &value, const int prevDataIndex,
                                  const int finalDataIndex)
{
    int prevNextIndex = prevDataIndex + NEXT_PTR_OFFSET;
    int nextDataIndex = GetPrimitiveElement(prevNextIndex).GetInt();
    int nextPrevIndex = nextDataIndex + PREV_PTR_OFFSET;
    SetElement(thread, prevNextIndex, JSTaggedValue(finalDataIndex));
    SetElement(thread, nextPrevIndex, JSTaggedValue(finalDataIndex));
    SetElement(thread, finalDataIndex, value.GetTaggedValue());
    SetElement(thread, finalDataIndex + NEXT_PTR_OFFSET, JSTaggedValue(nextDataIndex));
    SetElement(thread, finalDataIndex + PREV_PTR_OFFSET, JSTaggedValue(prevDataIndex));
    SetNumberOfNodes(thread, NumberOfNodes() + 1);
}

bool TaggedDoubleList::Has(const JSThread *thread, const JSTaggedValue &element)
{
    int dataIndex = FindIndexByElement(thread, element);
    return dataIndex != -1;
}

JSTaggedValue TaggedDoubleList::Get(const JSThread *thread, const int index)
{
    int len = NumberOfNodes();
    // 2 : 2 MEANS the half
    if (len / 2 > index) {
        return FindElementByIndex(thread, index);
    } else {
        return FindElementByIndexAtLast(thread, index);
    }
}

std::pair<int, JSTaggedValue> TaggedDoubleList::GetByDataIndex(const JSThread *thread, const int dataIndex)
{
    return FindElementByDataIndex(thread, dataIndex);
}

int TaggedDoubleList::GetPrevNode(const int index)
{
    int len = NumberOfNodes();
    // When index < (len / 2), search doubleList from the beginning
    if ((len / 2) > index) {
        return FindPrevNodeByIndex(index);
    } else {
        int leftNodeLen = len - 1 - index;
        // When insert at last
        if (leftNodeLen == -1) {
            return GetPrimitiveElement(TAIL_TABLE_INDEX).GetInt();
        }
        // when index >= (len / 2), search doubleList from the end
        return FindPrevNodeByIndexAtLast(leftNodeLen);
    }
}

int TaggedDoubleList::GetIndexOf(const JSThread *thread, const JSTaggedValue &element)
{
    return FindIndexByElement(thread, element);
}

int TaggedDoubleList::GetLastIndexOf(const JSThread *thread, const JSTaggedValue &element)
{
    return FindLastIndexByElement(thread, element);
}

JSTaggedValue TaggedDoubleList::Set(JSThread *thread, const JSHandle<TaggedDoubleList> &taggedList, const int index,
                                    const JSHandle<JSTaggedValue> &value)
{
    int nodeLength = taggedList->NumberOfNodes();
    if (index < 0 || index >= nodeLength) {
        JSTaggedValue error =
            containers::ContainerError::BusinessError(thread, containers::ErrorFlag::RANGE_ERROR,
                                                      "The value of index is out of range");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    int dataIndex = taggedList->FindDataIndexByNodeIndex(index);
    if (dataIndex == -1) {
        JSTaggedValue error =
            containers::ContainerError::BusinessError(thread, containers::ErrorFlag::RANGE_ERROR,
                                                      "The value of index is out of range");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    taggedList->SetElement(thread, dataIndex, value.GetTaggedValue());
    return taggedList.GetTaggedValue();
}

void TaggedDoubleList::Clear(const JSThread *thread)
{
    TaggedList<TaggedDoubleList>::Clear(thread);
    SetElement(thread, ELEMENTS_START_INDEX + PREV_PTR_OFFSET, JSTaggedValue(ELEMENTS_START_INDEX));
}

JSTaggedValue TaggedDoubleList::RemoveFirst(JSThread *thread)
{
    int firstDataIndex = GetPrimitiveElement(ELEMENTS_START_INDEX + NEXT_PTR_OFFSET).GetInt();
    JSTaggedValue firstData = GetElement(thread, firstDataIndex);
    RemoveNode(thread, ELEMENTS_START_INDEX);
    return firstData;
}

JSTaggedValue TaggedDoubleList::RemoveLast(JSThread *thread)
{
    int lastDataIndex = GetPrimitiveElement(ELEMENTS_START_INDEX + 2).GetInt();
    int prevDataIndex = GetPrimitiveElement(lastDataIndex + 2).GetInt();
    JSTaggedValue lastData = GetElement(thread, lastDataIndex);
    RemoveNode(thread, prevDataIndex);
    return lastData;
}

JSTaggedValue TaggedDoubleList::RemoveByIndex(JSThread *thread, const int &index)
{
    int prevDataIndex = GetPrevNode(index);
    int curDataIndex = GetPrimitiveElement(prevDataIndex + NEXT_PTR_OFFSET).GetInt();
    JSTaggedValue data = GetElement(thread, curDataIndex);
    RemoveNode(thread, prevDataIndex);
    return data;
}

JSTaggedValue TaggedDoubleList::Remove(JSThread *thread, const JSTaggedValue &element)
{
    int prevDataIndex = FindPrevNodeByValue(thread, element);
    if (prevDataIndex == -1) {
        return JSTaggedValue::False();
    }
    RemoveNode(thread, prevDataIndex);
    return JSTaggedValue::True();
}

JSTaggedValue TaggedDoubleList::RemoveFirstFound(JSThread *thread, const JSHandle<TaggedDoubleList> &taggedList,
                                                 const JSTaggedValue &element)
{
    int prevDataIndex = taggedList->FindPrevNodeByValue(thread, element);
    if (prevDataIndex == -1) {
        JSTaggedValue error =
            containers::ContainerError::BusinessError(thread, containers::ErrorFlag::IS_NOT_EXIST_ERROR,
                                                      "The element does not exist in this container");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    taggedList->RemoveNode(thread, prevDataIndex);
    return JSTaggedValue::True();
}

JSTaggedValue TaggedDoubleList::RemoveLastFound(JSThread *thread, const JSHandle<TaggedDoubleList> &taggedList,
                                                const JSTaggedValue &element)
{
    int prevDataIndex = taggedList->FindPrevNodeByValueAtLast(thread, element);
    if (prevDataIndex == -1) {
        JSTaggedValue error =
            containers::ContainerError::BusinessError(thread, containers::ErrorFlag::IS_NOT_EXIST_ERROR,
                                                      "The element does not exist in this container");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    taggedList->RemoveNode(thread, prevDataIndex);
    return JSTaggedValue::True();
}

JSHandle<TaggedArray> TaggedDoubleList::OwnKeys(JSThread *thread, const JSHandle<TaggedDoubleList> &taggedList)
{
    return TaggedList<TaggedDoubleList>::OwnKeys(thread, taggedList);
}

JSTaggedValue TaggedDoubleList::FindElementByIndexAtLast(const JSThread *thread, int index) const
{
    int dataIndex = ELEMENTS_START_INDEX;
    int preDataIndex = GetPrimitiveElement(dataIndex + PREV_PTR_OFFSET).GetInt();
    int nodeSum = GetElement(thread, NUMBER_OF_NODE_INDEX).GetInt() - 1;
    while (preDataIndex != ELEMENTS_START_INDEX) {
        dataIndex = preDataIndex;
        JSTaggedValue dataValue = GetElement(thread, dataIndex);
        if (nodeSum == index) {
            return dataValue;
        }
        preDataIndex = GetPrimitiveElement(preDataIndex + PREV_PTR_OFFSET).GetInt();
        nodeSum--;
    }
    return JSTaggedValue::Undefined();
}

int TaggedDoubleList::FindPrevNodeByIndexAtLast(const int index) const
{
    int prevDataIndex = GetPrimitiveElement(ELEMENTS_START_INDEX + PREV_PTR_OFFSET).GetInt();
    int nodeSum = 0;
    int len = GetPrimitiveElement(NUMBER_OF_NODE_INDEX).GetInt();
    while (nodeSum <= len) {
        int prePreDataIndex = GetPrimitiveElement(prevDataIndex + PREV_PTR_OFFSET).GetInt();
        if (nodeSum == index) {
            return prePreDataIndex;
        }
        prevDataIndex = prePreDataIndex;
        nodeSum++;
    }
    return -1;
}

int TaggedDoubleList::FindPrevNodeByValueAtLast(const JSThread *thread, const JSTaggedValue &element)
{
    int prevDataIndex = GetPrimitiveElement(ELEMENTS_START_INDEX + PREV_PTR_OFFSET).GetInt();
    int nodeSum = 0;
    int len = GetPrimitiveElement(NUMBER_OF_NODE_INDEX).GetInt();
    while (nodeSum <= len) {
        int prePreDataIndex = GetPrimitiveElement(prevDataIndex + PREV_PTR_OFFSET).GetInt();
        JSTaggedValue data = GetElement(thread, prevDataIndex);
        if (JSTaggedValue::SameValue(thread, data, element)) {
            return prePreDataIndex;
        }
        prevDataIndex = prePreDataIndex;
        nodeSum++;
    }
    return -1;
}
} // namespace panda::ecmascript
