/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/tagged_tree.h"

#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_function.h"

namespace panda::ecmascript {
template<typename Derived>
JSHandle<Derived> TaggedTree<Derived>::Create(const JSThread *thread, int numberOfElements)
{
    ASSERT_PRINT(numberOfElements > 0, "size must be a non-negative integer");
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    int capacity = numberOfElements > MIN_CAPACITY ? numberOfElements : MIN_CAPACITY;
    int length = ELEMENTS_START_INDEX + capacity * (Derived::ENTRY_SIZE);

    auto tree = JSHandle<Derived>::Cast(factory->NewTaggedArray(length));
    tree->SetNumberOfElements(thread, 0);
    tree->SetNumberOfDeletedElements(thread, 0);
    tree->SetRootEntries(thread, -1);
    tree->SetCompare(thread, JSTaggedValue::Hole());
    tree->SetCapacity(thread, capacity);
    return tree;
}

template<typename Derived>
void TaggedTree<Derived>::InsertRebalance(const JSThread *thread, int index)
{
    while (IsValidIndex(thread, index) && GetColor(GetParent(index)) == TreeColor::RED) {
        if (IsLeft(GetParent(index))) {
            int bro = GetLeftBrother(GetParent(index));
            if (GetColor(bro)) {
                SetColor(thread, GetParent(index), TreeColor::BLACK);
                SetColor(thread, bro, TreeColor::BLACK);
                SetColor(thread, GetParent(GetParent(index)), TreeColor::RED);
                index = GetParent(GetParent(index));
            } else {
                if (IsRight(index)) {
                    index = GetParent(index);
                    LeftRotate(thread, index);
                }
                SetColor(thread, GetParent(index), TreeColor::BLACK);
                SetColor(thread, GetParent(GetParent(index)), TreeColor::RED);
                RightRotate(thread, GetParent(GetParent(index)));
            }
        } else {
            int bro = GetRightBrother(GetParent(index));
            if (GetColor(bro)) {
                SetColor(thread, GetParent(index), TreeColor::BLACK);
                SetColor(thread, bro, TreeColor::BLACK);
                SetColor(thread, GetParent(GetParent(index)), TreeColor::RED);
                index = GetParent(GetParent(index));
            } else {
                if (IsLeft(index)) {
                    index = GetParent(index);
                    RightRotate(thread, index);
                }
                SetColor(thread, GetParent(index), TreeColor::BLACK);
                SetColor(thread, GetParent(GetParent(index)), TreeColor::RED);
                LeftRotate(thread, GetParent(GetParent(index)));
            }
        }
    }
    SetColor(thread, GetRootEntries(), TreeColor::BLACK);
}

template<typename Derived>
void TaggedTree<Derived>::LeftRotate(const JSThread *thread, int index)
{
    if (index >= 0) {
        int right = GetRightChild(index).GetInt();
        JSTaggedValue leftOfRight = GetLeftChild(right);
        SetRightChild(thread, index, leftOfRight);
        if (!leftOfRight.IsHole()) {
            SetParent(thread, leftOfRight.GetInt(), JSTaggedValue(index));
        }
        int parentOfIndex = GetParent(index);
        SetParent(thread, right, JSTaggedValue(parentOfIndex));
        if (parentOfIndex < 0) {
            SetRootEntries(thread, right);
        } else {
            JSTaggedValue left = GetLeftChild(parentOfIndex);
            if (!left.IsHole() && left.GetInt() == index) { // change to isleft
                SetLeftChild(thread, parentOfIndex, JSTaggedValue(right));
            } else {
                SetRightChild(thread, parentOfIndex, JSTaggedValue(right));
            }
        }
        SetLeftChild(thread, right, JSTaggedValue(index));
        SetParent(thread, index, JSTaggedValue(right));
    }
}

template<typename Derived>
void TaggedTree<Derived>::RightRotate(const JSThread *thread, int index)
{
    if (index >= 0) {
        int left = GetLeftChild(index).GetInt();
        JSTaggedValue rightOfLeft = GetRightChild(left);
        SetLeftChild(thread, index, rightOfLeft);
        if (!rightOfLeft.IsHole()) {
            SetParent(thread, rightOfLeft.GetInt(), JSTaggedValue(index));
        }
        int parentOfIndex = GetParent(index);
        SetParent(thread, left, JSTaggedValue(parentOfIndex));
        if (parentOfIndex < 0) {
            SetRootEntries(thread, left);
        } else {
            JSTaggedValue right = GetRightChild(parentOfIndex);
            if (!right.IsHole() && right.GetInt() == index) { // change to isright
                SetRightChild(thread, parentOfIndex, JSTaggedValue(left));
            } else {
                SetLeftChild(thread, parentOfIndex, JSTaggedValue(left));
            }
        }
        SetRightChild(thread, left, JSTaggedValue(index));
        SetParent(thread, index, JSTaggedValue(left));
    }
}

template<typename Derived>
JSHandle<Derived> TaggedTree<Derived>::AdjustTaggedTree(const JSThread *thread, const JSHandle<Derived> &tree, int len)
{
    JSMutableHandle<Derived> newTree(thread, JSTaggedValue::Undefined());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (tree->NumberOfDeletedElements() == 0) {
        newTree.Update(factory->ExtendArray(JSHandle<TaggedArray>::Cast(tree), len).GetTaggedValue());
        return newTree;
    }

    uint32_t elements = tree->NumberOfElements();
    newTree.Update(factory->NewTaggedArray(len).GetTaggedValue());
    newTree->SetNumberOfElements(thread, elements);
    newTree->SetNumberOfDeletedElements(thread, 0);

    newTree->SetRootEntries(thread, 0);
    CQueue<int> entries;
    entries.push(tree->GetRootEntries());
    int index = 0;
    newTree->SetParent(thread, index, JSTaggedValue(-1));
    int child = 1;
    while (!entries.empty()) {
        int parent = entries.front();
        JSTaggedValue left = tree->GetLeftChild(parent);
        if (!left.IsHole()) {
            entries.push(left.GetInt());
            newTree->SetLeftChild(thread, index, JSTaggedValue(child));
            newTree->SetParent(thread, child++, JSTaggedValue(index));
        }
        JSTaggedValue right = tree->GetRightChild(parent);
        if (!right.IsHole()) {
            entries.push(right.GetInt());
            newTree->SetRightChild(thread, index, JSTaggedValue(child));
            newTree->SetParent(thread, child++, JSTaggedValue(index));
        }
        tree->CopyEntry(thread, parent, newTree, index);
        entries.pop();
        index++;
    }
    return newTree;
}

template<typename Derived>
void TaggedTree<Derived>::Transplant(const JSThread *thread, int dst, int src)
{
    int parent = GetParent(dst);
    if (parent < 0) {
        SetRootEntries(thread, src);
    } else if (IsLeft(dst)) {
        JSTaggedValue child = src < 0 ? JSTaggedValue::Hole() : JSTaggedValue(src);
        SetLeftChild(thread, parent, child);
    } else {
        JSTaggedValue child = src < 0 ? JSTaggedValue::Hole() : JSTaggedValue(src);
        SetRightChild(thread, parent, child);
    }
    SetParent(thread, src, JSTaggedValue(parent));
}

template<typename Derived>
void TaggedTree<Derived>::Remove(const JSThread *thread, const JSHandle<Derived> &tree, int entry)
{
    int successor = entry;
    if (!tree->GetLeftChild(entry).IsHole() && !tree->GetRightChild(entry).IsHole()) {
        successor = tree->GetSuccessor(entry);
        tree->CopyData(thread, entry, successor);
    }
    JSTaggedValue left = tree->GetLeftChild(successor);
    JSTaggedValue right = tree->GetRightChild(successor);
    int child = left.IsHole() ? (right.IsHole() ? -1 : right.GetInt()) : left.GetInt();
    if (child < 0) {
        if (tree->GetColor(successor) == TreeColor::BLACK) {
            tree->DeleteRebalance(thread, successor);
        }
    }
    tree->Transplant(thread, successor, child);

    if (child >= 0) {
        if (tree->GetColor(successor) == TreeColor::BLACK) {
            tree->DeleteRebalance(thread, child);
        }
    }
    tree->RemoveEntry(thread, successor);
    ASSERT(tree->NumberOfElements() > 0);
    tree->SetNumberOfElements(thread, tree->NumberOfElements() - 1);
    tree->SetNumberOfDeletedElements(thread, tree->NumberOfDeletedElements() + 1);
}

template<typename Derived>
void TaggedTree<Derived>::DeleteRebalance(const JSThread *thread, int index)
{
    while (index != GetRootEntries() && GetColor(index) == TreeColor::BLACK) {
        if (IsLeft(index)) {
            int bro = GetLeftBrother(index);
            if (GetColor(bro)) {
                SetColor(thread, bro, TreeColor::BLACK);
                SetColor(thread, GetParent(index), TreeColor::RED);
                LeftRotate(thread, GetParent(index));
                bro = GetLeftBrother(index);
            }
            if (GetColor(GetLeftChildIndex(bro)) == TreeColor::BLACK &&
                GetColor(GetRightChildIndex(bro)) == TreeColor::BLACK) {
                SetColor(thread, bro, TreeColor::RED);
                index = GetParent(index);
            } else {
                if (GetColor(GetRightChildIndex(bro)) == TreeColor::BLACK) {
                    SetColor(thread, GetLeftChildIndex(bro), TreeColor::BLACK);
                    SetColor(thread, bro, TreeColor::RED);
                    RightRotate(thread, bro);
                    bro = GetLeftBrother(index);
                }
                SetColor(thread, bro, GetColor(GetParent(index)));
                SetColor(thread, GetParent(index), TreeColor::BLACK);
                SetColor(thread, GetRightChildIndex(bro), TreeColor::BLACK);
                LeftRotate(thread, GetParent(index));
                index = GetRootEntries();
            }
        } else {
            int bro = GetRightBrother(index);
            if (GetColor(bro)) {
                SetColor(thread, bro, TreeColor::BLACK);
                SetColor(thread, GetParent(index), TreeColor::RED);
                RightRotate(thread, GetParent(index));
                bro = GetRightBrother(index);
            }
            if (GetColor(GetRightChildIndex(bro)) == TreeColor::BLACK &&
                GetColor(GetLeftChildIndex(bro)) == TreeColor::BLACK) {
                SetColor(thread, bro, TreeColor::RED);
                index = GetParent(index);
            } else {
                if (GetColor(GetLeftChildIndex(bro)) == TreeColor::BLACK) {
                    SetColor(thread, GetRightChildIndex(bro), TreeColor::BLACK);
                    SetColor(thread, bro, TreeColor::RED);
                    LeftRotate(thread, bro);
                    bro = GetRightBrother(index);
                }
                SetColor(thread, bro, GetColor(GetParent(index)));
                SetColor(thread, GetParent(index), TreeColor::BLACK);
                SetColor(thread, GetLeftChildIndex(bro), TreeColor::BLACK);
                RightRotate(thread, GetParent(index));
                index = GetRootEntries();
            }
        }
    }
    SetColor(thread, index, TreeColor::BLACK);
}

template<typename Derived>
int TaggedTree<Derived>::GetPreDecessor(int entry) const
{
    int child = GetLeftChildIndex(entry);
    if (child >= 0) {
        return GetMaximum(child);
    }
    int parent = GetParent(entry);
    while (parent >= 0 && (GetLeftChildIndex(parent) == entry)) {
        entry = parent;
        parent = GetParent(entry);
    }
    return parent;
}

template<typename Derived>
int TaggedTree<Derived>::GetSuccessor(int entry) const
{
    int child = GetRightChildIndex(entry);
    if (child >= 0) {
        return GetMinimum(child);
    }
    int parent = GetParent(entry);
    while (parent >= 0 && (GetRightChildIndex(parent) == entry)) {
        entry = parent;
        parent = GetParent(entry);
    }
    return parent;
}

template<typename Derived>
int TaggedTree<Derived>::FindEntry(JSThread *thread, const JSHandle<Derived> &tree, const JSHandle<JSTaggedValue> &key)
{
    int parentIndex = tree->GetRootEntries();
    JSMutableHandle<JSTaggedValue> parentKey(thread, tree->GetKey(thread, parentIndex));
    ComparisonResult res;
    while (!parentKey->IsHole()) {
        res = EntryCompare(thread, key, parentKey, tree);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
        if (res == ComparisonResult::EQUAL) {
            return parentIndex;
        } else if (res == ComparisonResult::LESS) {
            JSTaggedValue child = tree->GetLeftChild(parentIndex);
            if (child.IsHole()) {
                break;
            }
            parentIndex = child.GetInt();
        } else {
            JSTaggedValue child = tree->GetRightChild(parentIndex);
            if (child.IsHole()) {
                break;
            }
            parentIndex = child.GetInt();
        }
        parentKey.Update(tree->GetKey(thread, parentIndex));
    }
    return -1;
}

template<typename Derived>
ComparisonResult TaggedTree<Derived>::EntryCompare(JSThread *thread, const JSHandle<JSTaggedValue> valueX,
    const JSHandle<JSTaggedValue> valueY, JSHandle<Derived> tree)
{
    JSTaggedValue fn = tree->GetCompare(thread);
    if (fn.IsHole()) {
        return OrdinayEntryCompare(thread, valueX, valueY);
    }
    if (valueX->IsUndefined()) {
        return valueY->IsUndefined() ? ComparisonResult::EQUAL : ComparisonResult::GREAT;
    }
    if (valueY->IsUndefined()) {
        return ComparisonResult::LESS;
    }
    if (valueX->IsNull()) {
        return valueY->IsNull() ? ComparisonResult::EQUAL : ComparisonResult::GREAT;
    }
    if (valueY->IsNull()) {
        return ComparisonResult::LESS;
    }
    JSHandle<JSTaggedValue> compareFn(thread, fn);
    JSHandle<JSTaggedValue> thisArgHandle = thread->GlobalConstants()->GetHandledUndefined();
    const uint32_t argsLength = 2;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, compareFn, thisArgHandle, undefined, argsLength);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
    info->SetCallArg(valueX.GetTaggedValue(), valueY.GetTaggedValue());
    JSTaggedValue callResult = JSFunction::Call(info);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
    int compareResult = -1;
    if (callResult.IsBoolean()) {
        // if callResult is true, compareResult = -1.
        if (callResult.IsFalse()) {
            info = EcmaInterpreter::NewRuntimeCallInfo(thread, compareFn, thisArgHandle, undefined, argsLength);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
            info->SetCallArg(valueY.GetTaggedValue(), valueX.GetTaggedValue());
            callResult = JSFunction::Call(info);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
            compareResult = callResult.IsTrue() ? 1 : 0;
        }
    } else if (callResult.IsInt()) {
        compareResult = callResult.GetInt();
    } else {
        JSTaggedNumber v = JSTaggedValue::ToNumber(thread, JSHandle<JSTaggedValue>(thread, callResult));
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
        double value = v.GetNumber();
        if (std::isnan(value)) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "CompareFn has illegal return value", ComparisonResult::UNDEFINED);
        }
        compareResult = static_cast<int>(value);
    }
    return compareResult > 0 ? ComparisonResult::GREAT :
                            (compareResult < 0 ? ComparisonResult::LESS : ComparisonResult::EQUAL);
}

template<typename Derived>
JSHandle<TaggedArray> TaggedTree<Derived>::GetSortArray(const JSThread *thread, const JSHandle<Derived> &tree)
{
    JSHandle<TaggedArray> sortArray = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(tree->NumberOfElements());
    CStack<int> entries;
    int index = tree->GetRootEntries();
    int aid = 0;
    while (index >= 0 || !entries.empty()) {
        while (index >= 0) {
            entries.emplace(index);
            index = tree->GetLeftChildIndex(index);
        }
        if (!entries.empty()) {
            sortArray->Set(thread, aid++, JSTaggedValue(entries.top()));
            index = tree->GetRightChildIndex(entries.top());
            entries.pop();
        }
    }
    return sortArray;
}

template<typename Derived>
JSHandle<Derived> TaggedTree<Derived>::Insert(JSThread *thread, JSHandle<Derived> &tree,
                                              const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value)
{
    ASSERT(IsKey(key.GetTaggedValue()));
    JSMutableHandle<JSTaggedValue> parentKey(thread, tree->GetRootKey(thread));
    if (parentKey->IsHole()) {
        tree->SetRoot(thread, 0, key.GetTaggedValue(), value.GetTaggedValue());
        tree->SetNumberOfElements(thread, tree->NumberOfElements() + 1);
        return tree;
    }

    JSHandle<Derived> newTree = GrowCapacity(thread, tree);
    int parentIndex = newTree->GetRootEntries();
    ComparisonResult res;
    while (!parentKey->IsHole()) {
        res = EntryCompare(thread, key, parentKey, tree);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSHandle<Derived>(thread, JSTaggedValue::Exception()));
        if (res == ComparisonResult::EQUAL) {
            newTree->SetValue(thread, parentIndex, value.GetTaggedValue());
            return newTree;
        } else if (res == ComparisonResult::LESS) {
            JSTaggedValue child = newTree->GetLeftChild(parentIndex);
            if (child.IsHole()) {
                break;
            }
            parentIndex = child.GetInt();
        } else {
            JSTaggedValue child = newTree->GetRightChild(parentIndex);
            if (child.IsHole()) {
                break;
            }
            parentIndex = child.GetInt();
        }
        parentKey.Update(newTree->GetKey(thread, parentIndex));
    }

    uint32_t entry = newTree->NumberOfElements() + newTree->NumberOfDeletedElements();
    if (res != ComparisonResult::LESS) {
        newTree->InsertRightEntry(thread, parentIndex, entry, key.GetTaggedValue(), value.GetTaggedValue());
    } else {
        newTree->InsertLeftEntry(thread, parentIndex, entry, key.GetTaggedValue(), value.GetTaggedValue());
    }
    newTree->SetNumberOfElements(thread, newTree->NumberOfElements() + 1);
    newTree->InsertRebalance(thread, entry);
    return newTree;
}

template<typename Derived>
JSHandle<Derived> TaggedTree<Derived>::GrowCapacity(const JSThread *thread, JSHandle<Derived> &tree)
{
    uint32_t nof = tree->NumberOfElements() + tree->NumberOfDeletedElements();
    int oldCapacity = tree->Capacity();
    if (static_cast<int>(nof + 1) <= oldCapacity) {
        return tree;
    }

    int newCapacity = ComputeCapacity(oldCapacity);
    int length = ELEMENTS_START_INDEX + newCapacity * (Derived::ENTRY_SIZE);
    JSHandle<Derived> newTree = AdjustTaggedTree(thread, tree, length);
    // 20 : version isolation at api20
    if (thread->GetEcmaVM()->GetVMAPIVersion() >= 20) {
        JSTaggedValue fn = tree->GetCompare(thread);
        if (!fn.IsUndefined() && !fn.IsNull()) {
            newTree->SetCompare(thread, fn);
        }
    }
    newTree->SetCapacity(thread, newCapacity);
    return newTree;
}

template<typename Derived>
JSTaggedValue TaggedTree<Derived>::GetLowerKey(JSThread *thread, const JSHandle<Derived> &tree,
                                               const JSHandle<JSTaggedValue> &key)
{
    int parentIndex = tree->GetRootEntries();
    JSMutableHandle<JSTaggedValue> parentKey(thread, tree->GetKey(thread, parentIndex));
    int resultIndex = -1;
    ComparisonResult res;
    while (parentIndex >= 0) {
        res = EntryCompare(thread, key, parentKey, tree);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (res == ComparisonResult::GREAT) {
            resultIndex = parentIndex;
            parentIndex = tree->GetRightChildIndex(parentIndex);
        } else {
            parentIndex = tree->GetLeftChildIndex(parentIndex);
        }
        parentKey.Update(tree->GetKey(thread, parentIndex));
    }
    JSTaggedValue lowerKey = tree->GetKey(thread, resultIndex);
    return tree->Transform(lowerKey);
}

template<typename Derived>
JSTaggedValue TaggedTree<Derived>::GetHigherKey(JSThread *thread, const JSHandle<Derived> &tree,
                                                const JSHandle<JSTaggedValue> &key)
{
    int parentIndex = tree->GetRootEntries();
    JSMutableHandle<JSTaggedValue> parentKey(thread, tree->GetKey(thread, parentIndex));
    int resultIndex = -1;
    ComparisonResult res;
    while (parentIndex >= 0) {
        res = EntryCompare(thread, key, parentKey, tree);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (res == ComparisonResult::LESS) {
            resultIndex = parentIndex;
            parentIndex = tree->GetLeftChildIndex(parentIndex);
        } else {
            parentIndex = tree->GetRightChildIndex(parentIndex);
        }
        parentKey.Update(tree->GetKey(thread, parentIndex));
    }
    JSTaggedValue lowerKey = tree->GetKey(thread, resultIndex);
    return tree->Transform(lowerKey);
}

template<typename Derived>
JSHandle<Derived> TaggedTree<Derived>::Shrink(const JSThread *thread, const JSHandle<Derived> &tree)
{
    int oldCapacity = static_cast<int>(tree->Capacity());
    if (static_cast<int>(tree->NumberOfElements()) >= (oldCapacity + 1) / 4) { // 4: quarter
        return tree;
    }
    uint32_t newCapacity = static_cast<uint32_t>(oldCapacity - 1) >> 1;
    if (newCapacity < static_cast<uint32_t>(Derived::MIN_SHRINK_CAPACITY)) {
        return tree;
    }

    int length = ELEMENTS_START_INDEX + static_cast<int>(newCapacity) * (Derived::ENTRY_SIZE);
    JSHandle<Derived> newTree = AdjustTaggedTree(thread, tree, length);
    JSTaggedValue fn = tree->GetCompare(thread);
    JSHandle<JSTaggedValue> compareFn = JSHandle<JSTaggedValue>(thread, fn);
    if (!compareFn->IsUndefined() && !compareFn->IsNull()) {
        newTree->SetCompare(thread, compareFn.GetTaggedValue());
    }
    newTree->SetCapacity(thread, newCapacity);
    return newTree;
}

// TaggedTreeMap
JSTaggedValue TaggedTreeMap::Create(const JSThread *thread, int numberOfElements)
{
    return RBTree::Create(thread, numberOfElements).GetTaggedValue();
}

JSHandle<TaggedArray> TaggedTreeMap::GetArrayFromMap(const JSThread *thread, const JSHandle<TaggedTreeMap> &map)
{
    return RBTree::GetSortArray(thread, map);
}

JSTaggedValue TaggedTreeMap::Set(JSThread *thread, JSHandle<TaggedTreeMap> &obj,
                                 const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value)
{
    return RBTree::Insert(thread, obj, key, value).GetTaggedValue();
}

JSTaggedValue TaggedTreeMap::Delete(JSThread *thread, const JSHandle<TaggedTreeMap> &map, int entry)
{
    RBTree::Remove(thread, map, entry);
    return RBTree::Shrink(thread, map).GetTaggedValue();
}

bool TaggedTreeMap::HasValue([[maybe_unused]] const JSThread *thread, JSTaggedValue value) const
{
    int root = GetRootEntries();
    if (root < 0) {
        return false;
    }

    CQueue<int> entries;
    entries.push(root);
    while (!entries.empty()) {
        int parent = entries.front();
        if (JSTaggedValue::SameValue(thread, GetValue(thread, parent), value)) {
            return true;
        }
        int left = GetLeftChildIndex(parent);
        if (left >= 0) {
            entries.push(left);
        }
        int right = GetRightChildIndex(parent);
        if (right >= 0) {
            entries.push(right);
        }
        entries.pop();
    }
    return false;
}

JSTaggedValue TaggedTreeMap::SetAll(JSThread *thread, JSHandle<TaggedTreeMap> &dst, const JSHandle<TaggedTreeMap> &src)
{
    CQueue<int> entries;
    entries.push(src->GetRootEntries());
    JSHandle<TaggedTreeMap> map = dst;
    while (!entries.empty()) {
        int parent = entries.front();
        map = Insert(thread, map, JSHandle<JSTaggedValue>(thread, src->GetKey(thread, parent)),
                     JSHandle<JSTaggedValue>(thread, src->GetValue(thread, parent)));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        int left = src->GetLeftChildIndex(parent);
        if (left >= 0) {
            entries.push(left);
        }
        int right = src->GetRightChildIndex(parent);
        if (right >= 0) {
            entries.push(right);
        }
        entries.pop();
    }
    return map.GetTaggedValue();
}

JSTaggedValue TaggedTreeMap::GetLowerKey(JSThread *thread, const JSHandle<TaggedTreeMap> &map,
                                         const JSHandle<JSTaggedValue> &key)
{
    return RBTree::GetLowerKey(thread, map, key);
}

JSTaggedValue TaggedTreeMap::GetHigherKey(JSThread *thread, const JSHandle<TaggedTreeMap> &map,
                                          const JSHandle<JSTaggedValue> &key)
{
    return RBTree::GetHigherKey(thread, map, key);
}

int TaggedTreeMap::FindEntry(JSThread *thread, const JSHandle<TaggedTreeMap> &map, const JSHandle<JSTaggedValue> &key)
{
    return RBTree::FindEntry(thread, map, key);
}

// TaggedTreeSet
JSTaggedValue TaggedTreeSet::Create(const JSThread *thread, int numberOfElements)
{
    return RBTree::Create(thread, numberOfElements).GetTaggedValue();
}

JSHandle<TaggedArray> TaggedTreeSet::GetArrayFromSet(const JSThread *thread, const JSHandle<TaggedTreeSet> &set)
{
    return RBTree::GetSortArray(thread, set);
}

JSTaggedValue TaggedTreeSet::Add(JSThread *thread, JSHandle<TaggedTreeSet> &obj, const JSHandle<JSTaggedValue> &value)
{
    return RBTree::Insert(thread, obj, value, value).GetTaggedValue();
}

JSTaggedValue TaggedTreeSet::Delete(JSThread *thread, const JSHandle<TaggedTreeSet> &set, int entry)
{
    RBTree::Remove(thread, set, entry);
    return RBTree::Shrink(thread, set).GetTaggedValue();
}

JSTaggedValue TaggedTreeSet::GetLowerKey(JSThread *thread, const JSHandle<TaggedTreeSet> &set,
                                         const JSHandle<JSTaggedValue> &key)
{
    return RBTree::GetLowerKey(thread, set, key);
}

JSTaggedValue TaggedTreeSet::GetHigherKey(JSThread *thread, const JSHandle<TaggedTreeSet> &set,
                                          const JSHandle<JSTaggedValue> &key)
{
    return RBTree::GetHigherKey(thread, set, key);
}

int TaggedTreeSet::FindEntry(JSThread *thread, const JSHandle<TaggedTreeSet> &set, const JSHandle<JSTaggedValue> &key)
{
    return RBTree::FindEntry(thread, set, key);
}
}  // namespace panda::ecmascript
