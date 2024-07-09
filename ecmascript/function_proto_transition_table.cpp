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

#include "ecmascript/function_proto_transition_table.h"

#include "ecmascript/js_hclass-inl.h"

namespace panda::ecmascript {
using ProtoArray = FunctionProtoTransitionTable::ProtoArray;
FunctionProtoTransitionTable::FunctionProtoTransitionTable(const JSThread *thread)
{
    protoTransitionTable_ = PointerToIndexDictionary::Create(thread, DEFAULT_FIRST_LEVEL_SIZE).GetTaggedValue();
}

FunctionProtoTransitionTable::~FunctionProtoTransitionTable()
{
    fakeParentMap_.clear();
}

void FunctionProtoTransitionTable::Iterate(const RootVisitor &v)
{
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&protoTransitionTable_)));
    for (auto &iter : fakeParentMap_) {
        v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&iter.first)));
        v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&iter.second)));
    }
    for (auto &iter : fakeChildMap_) {
        v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&iter.first)));
        v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&iter.second)));
    }
}

void FunctionProtoTransitionTable::UpdateProtoTransitionTable(const JSThread *thread,
                                                              JSHandle<PointerToIndexDictionary> map)
{
    // PointerToIndexDictionary's hash value is a hclass pointer,
    // and the hclass pointer could change during deserialized,
    // so rehash is needed after deserialized.
    auto newMap = PointerToIndexDictionary::Create(thread, map->Size());
    map->Rehash(thread, *newMap);
    protoTransitionTable_ = newMap.GetTaggedValue();
}

void FunctionProtoTransitionTable::InsertTransitionItem(const JSThread *thread,
                                                        JSHandle<JSTaggedValue> ihc,
                                                        JSHandle<JSTaggedValue> baseIhc,
                                                        JSHandle<JSTaggedValue> transIhc,
                                                        JSHandle<JSTaggedValue> transPhc)
{
    JSHandle<PointerToIndexDictionary> protoTransitionHandle(thread, protoTransitionTable_);
    int32_t entry1 = protoTransitionHandle->FindEntry(ihc.GetTaggedValue());
    if (entry1 != -1) {
        JSHandle<ProtoArray> protoArray(thread, protoTransitionHandle->GetValue(entry1));
        auto entry2 = protoArray->FindEntry(baseIhc.GetTaggedValue());
        if (entry2 != -1) {
            LOG_ECMA(DEBUG) << "Record prototype for current function already!";
            return;
        }
        uint32_t insertEntry = ProtoArray::GetEntry(JSHandle<JSHClass>(baseIhc));
        protoArray->SetEntry(
            thread, insertEntry, baseIhc.GetTaggedValue(), transIhc.GetTaggedValue(), transPhc.GetTaggedValue());
        return;
    }
    JSHandle<ProtoArray> protoArray = ProtoArray::Create(thread);
    uint32_t insertEntry = ProtoArray::GetEntry(JSHandle<JSHClass>(baseIhc));
    protoArray->SetEntry(
        thread, insertEntry, baseIhc.GetTaggedValue(), transIhc.GetTaggedValue(), transPhc.GetTaggedValue());
    JSHandle<PointerToIndexDictionary> newTransitionTable =
        PointerToIndexDictionary::PutIfAbsent(thread, protoTransitionHandle, ihc, JSHandle<JSTaggedValue>(protoArray));
    protoTransitionTable_ = newTransitionTable.GetTaggedValue();
}

std::pair<JSTaggedValue, JSTaggedValue> FunctionProtoTransitionTable::FindTransitionByHClass(
    const JSThread *thread, JSHandle<JSTaggedValue> ihc, JSHandle<JSTaggedValue> baseIhc) const
{
    ASSERT(ihc->IsJSHClass());
    ASSERT(baseIhc->IsJSHClass());
    JSHandle<PointerToIndexDictionary> protoTransitionHandle(thread, protoTransitionTable_);
    int32_t entry1 = protoTransitionHandle->FindEntry(ihc.GetTaggedValue());
    if (entry1 == -1) {
        LOG_ECMA(DEBUG) << "Selected ihc not found2!";
        return std::make_pair(JSTaggedValue::Undefined(), JSTaggedValue::Undefined());
    }
    JSHandle<ProtoArray> protoArray(thread, protoTransitionHandle->GetValue(entry1));
    int32_t entry2 = protoArray->FindEntry(baseIhc.GetTaggedValue());
    if (entry2 == -1) {
        LOG_ECMA(ERROR) << "Selected baseIhc not found!";
        return std::make_pair(JSTaggedValue::Undefined(), JSTaggedValue::Undefined());
    }
    return protoArray->GetTransition(entry2);
}

bool FunctionProtoTransitionTable::TryInsertFakeParentItem(JSTaggedType child, JSTaggedType parent)
{
    // Situation:
    // 1: d1.prototype = p
    // 2: d2.prototype = p
    // this check is true for step 2
    auto iter1 = fakeParentMap_.find(child);
    if (child == parent && iter1 != fakeParentMap_.end()) {
        return true;
    }
    auto iter2 = fakeChildMap_.find(parent);
    if (iter2 != fakeChildMap_.end()) {
        if (iter2->second != child) {
            LOG_ECMA(DEBUG) << "Unsupported mapping for a parent to more than 1 childs!";
            return false;
        }
    }

    if (iter1 == fakeParentMap_.end()) {
        fakeParentMap_[child] = parent;
        fakeChildMap_[parent] = child;
        return true;
    }
    if (iter1->second == parent) {
        return true;
    }
    LOG_ECMA(ERROR) << "Conflict mapping for the same child";
    return false;
}

JSTaggedType FunctionProtoTransitionTable::GetFakeParent(JSTaggedType child) const
{
    auto iter = fakeParentMap_.find(child);
    if (iter != fakeParentMap_.end()) {
        return iter->second;
    }
    return 0;
}

// static
JSHandle<ProtoArray> ProtoArray::Create(const JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> result = factory->NewTaggedArray(ENTRY_NUMBER * ENTRY_SIZE);
    return JSHandle<ProtoArray>(result);
}

// static
bool ProtoArray::GetEntry(JSHandle<JSHClass> hclass)
{
    return JSHandle<JSHClass>(hclass)->IsPrototype() ? ProtoArray::CLONED_ENTRY_INDEX
                                                     : ProtoArray::INIT_ENTRY_INDEX;
}

void ProtoArray::SetEntry(const JSThread *thread, uint32_t index, JSTaggedValue key, JSTaggedValue transIhc,
                          JSTaggedValue transPhc)
{
    uint32_t entryIndex = index * ENTRY_SIZE;
    uint32_t keyIndex = entryIndex + KEY_INDEX;
    uint32_t ihcIndex = entryIndex + TRANS_IHC_INDEX;
    uint32_t phcIndex = entryIndex + TRANS_PHC_INDEX;
    Set(thread, keyIndex, key);
    Set(thread, ihcIndex, transIhc);
    Set(thread, phcIndex, transPhc);
}

JSTaggedValue ProtoArray::GetKey(uint32_t index) const
{
    uint32_t entryIndex = index * ENTRY_SIZE;
    uint32_t keyIndex = entryIndex + KEY_INDEX;
    return Get(keyIndex);
}

std::pair<JSTaggedValue, JSTaggedValue> ProtoArray::GetTransition(uint32_t index) const
{
    uint32_t entryIndex = index * ENTRY_SIZE + KEY_INDEX;
    JSTaggedValue transIhc = Get(entryIndex + TRANS_IHC_INDEX);
    JSTaggedValue transPhc = Get(entryIndex + TRANS_PHC_INDEX);
    return std::make_pair(transIhc, transPhc);
}

std::pair<JSTaggedValue, JSTaggedValue> ProtoArray::FindTransition(JSTaggedValue key) const
{
    for (uint32_t i = 0; i < ENTRY_NUMBER; i++) {
        uint32_t index = i * ENTRY_SIZE + KEY_INDEX;
        JSTaggedValue keyValue = Get(index);
        if (keyValue == key) {
            JSTaggedValue transIhc = Get(index + TRANS_IHC_INDEX);
            JSTaggedValue transPhc = Get(index + TRANS_PHC_INDEX);
            return std::make_pair(transIhc, transPhc);
        }
    }
    return std::make_pair(JSTaggedValue::Undefined(), JSTaggedValue::Undefined());
}

int32_t ProtoArray::FindEntry(JSTaggedValue key) const
{
    for (uint32_t i = 0; i < ENTRY_NUMBER; i++) {
        uint32_t index = i * ENTRY_SIZE + KEY_INDEX;
        JSTaggedValue keyValue = Get(index);
        if (keyValue == key) {
            return i;
        }
    }
    return -1;
}
}  // namespace panda::ecmascript