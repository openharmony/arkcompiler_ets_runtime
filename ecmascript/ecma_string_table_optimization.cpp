/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ecmascript/ecma_string_table.h"

#include "common_components/taskpool/taskpool.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_string_table_optimization-inl.h"
#include "ecmascript/jspandafile/js_pandafile.h"

namespace panda::ecmascript {
#if ENABLE_NEXT_OPTIMIZATION
void EcmaStringTableCleaner::PostSweepWeakRefTask(const WeakRootVisitor &visitor)
{
    StartSweepWeakRefTask();
    iter_ = std::make_shared<std::atomic<uint32_t>>(0U);
    const uint32_t postTaskCount = Taskpool::GetCurrentTaskpool()->GetTotalThreadNum();
    for (uint32_t i = 0U; i < postTaskCount; ++i) {
        Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<SweepWeakRefTask>(iter_, this, visitor));
    }
}

void EcmaStringTableCleaner::JoinAndWaitSweepWeakRefTask(const WeakRootVisitor &visitor)
{
    ProcessSweepWeakRef(iter_, this, visitor);
    WaitSweepWeakRefTask();
    iter_.reset();
}

void EcmaStringTableCleaner::ProcessSweepWeakRef(IteratorPtr &iter, EcmaStringTableCleaner *cleaner,
                                                 const WeakRootVisitor &visitor)
{
    uint32_t index = 0U;
    while ((index = GetNextIndexId(iter)) < HashTrieMap::INDIRECT_SIZE) {
        cleaner->stringTable_->SweepWeakRef(visitor, index);
        if (ReduceCountAndCheckFinish(cleaner)) {
            cleaner->SignalSweepWeakRefTask();
        }
    }
}

void EcmaStringTableCleaner::StartSweepWeakRefTask()
{
    // No need lock here, only the daemon thread will reset the state.
    sweepWeakRefFinished_ = false;
    PendingTaskCount_.store(HashTrieMap::INDIRECT_SIZE, std::memory_order_relaxed);
}

void EcmaStringTableCleaner::WaitSweepWeakRefTask()
{
    LockHolder holder(sweepWeakRefMutex_);
    while (!sweepWeakRefFinished_) {
        sweepWeakRefCV_.Wait(&sweepWeakRefMutex_);
    }
}

void EcmaStringTableCleaner::SignalSweepWeakRefTask()
{
    LockHolder holder(sweepWeakRefMutex_);
    sweepWeakRefFinished_ = true;
    sweepWeakRefCV_.SignalAll();
}

bool EcmaStringTableCleaner::SweepWeakRefTask::Run([[maybe_unused]] uint32_t threadIndex)
{
    ProcessSweepWeakRef(iter_, cleaner_, visitor_);
    return true;
}

EcmaString *EcmaStringTable::GetString(JSHandle<EcmaString> string, uint32_t hashcode)
{
    EcmaString *str = *string;
    return stringTable_.Load<false>(hashcode, str);
}

EcmaString *EcmaStringTable::GetOrInternFlattenString(EcmaVM *vm, EcmaString *string)
{
    ASSERT(EcmaStringAccessor(string).NotTreeString());
    if (EcmaStringAccessor(string).IsInternString()) {
        return string;
    }
    uint32_t hashcode = EcmaStringAccessor(string).GetHashcode();
    ASSERT(!EcmaStringAccessor(string).IsInternString());
    ASSERT(EcmaStringAccessor(string).NotTreeString());
    // Strings in string table should not be in the young space.
#ifdef USE_CMC_GC
    ASSERT(string->IsInSharedHeap());
#else
    ASSERT(Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(string))->InSharedHeap());
#endif
    auto loadResult = stringTable_.Load(hashcode, string);
    if (loadResult) {
        return loadResult.value;
    }
    JSThread *thread = vm->GetJSThread();
    JSHandle<EcmaString> stringHandle(thread, string);
    EcmaString *result = stringTable_.StoreOrLoad(vm, hashcode, loadResult, stringHandle);
    ASSERT(result != nullptr);
    return result;
}

EcmaString *EcmaStringTable::GetOrInternStringFromCompressedSubString(EcmaVM *vm, const JSHandle<EcmaString> &string,
                                                                      uint32_t offset, uint32_t utf8Len)
{
    const uint8_t *utf8Data = EcmaStringAccessor(string).GetDataUtf8() + offset;
    uint32_t hashcode = EcmaStringAccessor::ComputeHashcodeUtf8(utf8Data, utf8Len, true);
    auto loadResult = stringTable_.Load(hashcode, string, offset, utf8Len);
    if (loadResult) {
        return loadResult.value;
    }
    EcmaString *result = stringTable_.StoreOrLoad(
        vm, hashcode, loadResult,
        [vm, string, offset, utf8Len, hashcode]() {
            EcmaString *str = EcmaStringAccessor::CreateFromUtf8CompressedSubString(vm, string, offset, utf8Len,
                                                                                    MemSpaceType::SHARED_OLD_SPACE);
            str->SetRawHashcode(hashcode);
            ASSERT(!EcmaStringAccessor(str).IsInternString());
            ASSERT(EcmaStringAccessor(str).NotTreeString());
        // Strings in string table should not be in the young space.
#ifdef USE_CMC_GC
            ASSERT(str->IsInSharedHeap());
#else
            ASSERT(Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(str))->InSharedHeap());
#endif
            JSThread *thread = vm->GetJSThread();
            JSHandle<EcmaString> strHandle(thread, str);
            return strHandle;
        },
        [utf8Len, string, offset](EcmaString *foundString) {
            const uint8_t *utf8Data = EcmaStringAccessor(string).GetDataUtf8() + offset;
            return EcmaStringAccessor::StringIsEqualUint8Data(foundString, utf8Data, utf8Len, true);
        });
    ASSERT(result != nullptr);
    return result;
}

EcmaString *EcmaStringTable::GetOrInternString(EcmaVM *vm, EcmaString *string)
{
    if (EcmaStringAccessor(string).IsInternString()) {
        return string;
    }
    JSThread *thread = vm->GetJSThread();
    JSHandle<EcmaString> stringHandle(thread, string);
    // may gc
    EcmaString *strFlat = EcmaStringAccessor::Flatten(vm, stringHandle, MemSpaceType::SHARED_OLD_SPACE);
    if (EcmaStringAccessor(strFlat).IsInternString()) {
        return strFlat;
    }
    uint32_t hashcode = EcmaStringAccessor(strFlat).GetHashcode();
    auto loadResult = stringTable_.Load(hashcode, strFlat);
    if (loadResult) {
        return loadResult.value;
    }
    JSHandle<EcmaString> strFlatHandle(thread, strFlat);
    EcmaString *result = stringTable_.StoreOrLoad(vm, hashcode, loadResult, strFlatHandle);
    ASSERT(result != nullptr);
    return result;
}

EcmaString *EcmaStringTable::GetOrInternString(EcmaVM *vm, const JSHandle<EcmaString> &firstString,
                                               const JSHandle<EcmaString> &secondString)
{
    bool signalState = vm->GetJsDebuggerManager()->GetSignalState();
    if (UNLIKELY(signalState)) {
        return GetOrInternStringThreadUnsafe(vm, firstString, secondString);
    }
    JSThread *thread = vm->GetJSThread();
    JSHandle<EcmaString> firstFlat(thread, EcmaStringAccessor::Flatten(vm, firstString));
    JSHandle<EcmaString> secondFlat(thread, EcmaStringAccessor::Flatten(vm, secondString));
    uint32_t hashcode = EcmaStringAccessor::CalculateAllConcatHashCode(firstFlat, secondFlat);
    ASSERT(EcmaStringAccessor(firstFlat).NotTreeString());
    ASSERT(EcmaStringAccessor(secondFlat).NotTreeString());
    EcmaString *result = stringTable_.LoadOrStore<true>(
        vm, hashcode,
        [vm, hashcode, thread, firstFlat, secondFlat]() {
            JSHandle<EcmaString> concatHandle(
                thread, EcmaStringAccessor::Concat(vm, firstFlat, secondFlat, MemSpaceType::SHARED_OLD_SPACE));
            EcmaString *value = EcmaStringAccessor::Flatten(vm, concatHandle, MemSpaceType::SHARED_OLD_SPACE);
            value->SetRawHashcode(hashcode);
            ASSERT(!EcmaStringAccessor(value).IsInternString());
            ASSERT(EcmaStringAccessor(value).NotTreeString());
        // Strings in string table should not be in the young space.
#ifdef USE_CMC_GC
            ASSERT(value->IsInSharedHeap());
#else
            ASSERT(Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(value))->InSharedHeap());
#endif
            JSHandle<EcmaString> stringHandle(thread, value);
            return stringHandle;
        },
        [firstFlat, secondFlat](EcmaString *foundString) {
            EcmaString *firstStr = *firstFlat;
            EcmaString *secondStr = *secondFlat;
            return EcmaStringAccessor(foundString).EqualToSplicedString(firstStr, secondStr);
        });
    ASSERT(result != nullptr);
    return result;
}

EcmaString *EcmaStringTable::GetOrInternString(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len,
                                               bool canBeCompress, MemSpaceType type)
{
    ASSERT(IsSMemSpace(type));
    bool signalState = vm->GetJsDebuggerManager()->GetSignalState();
    if (UNLIKELY(signalState)) {
        return GetOrInternStringThreadUnsafe(vm, utf8Data, utf8Len, canBeCompress);
    }
    uint32_t hashcode = EcmaStringAccessor::ComputeHashcodeUtf8(utf8Data, utf8Len, canBeCompress);
    EcmaString *result = stringTable_.LoadOrStore<true>(
        vm, hashcode,
        [vm, hashcode, utf8Data, utf8Len, canBeCompress, type]() {
            EcmaString *value = EcmaStringAccessor::CreateFromUtf8(vm, utf8Data, utf8Len, canBeCompress, type);
            value->SetRawHashcode(hashcode);
            ASSERT(!EcmaStringAccessor(value).IsInternString());
            ASSERT(EcmaStringAccessor(value).NotTreeString());
        // Strings in string table should not be in the young space.
#ifdef USE_CMC_GC
            ASSERT(value->IsInSharedHeap());
#else
            ASSERT(Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(value))->InSharedHeap());
#endif
            JSThread *thread = vm->GetJSThread();
            JSHandle<EcmaString> stringHandle(thread, value);
            return stringHandle;
        },
        [utf8Data, utf8Len, canBeCompress](EcmaString *foundString) {
            return EcmaStringAccessor::StringIsEqualUint8Data(foundString, utf8Data, utf8Len, canBeCompress);
        });
    ASSERT(result != nullptr);
    return result;
}

EcmaString *EcmaStringTable::GetOrInternString(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf16Len,
                                               MemSpaceType type)
{
    ASSERT(IsSMemSpace(type));
    ASSERT(type == MemSpaceType::SHARED_NON_MOVABLE || type == MemSpaceType::SHARED_OLD_SPACE);
    JSThread *thread = vm->GetJSThread();
    EcmaString *str = EcmaStringAccessor::CreateUtf16StringFromUtf8(vm, utf8Data, utf16Len, type);
    uint32_t hashcode = EcmaStringAccessor(str).GetHashcode();
    auto loadResult = stringTable_.Load(hashcode, str);
    if (loadResult) {
        return loadResult.value;
    }
    JSHandle<EcmaString> strHandle(thread, str);
    EcmaString *result = stringTable_.StoreOrLoad(vm, hashcode, loadResult, strHandle);
    ASSERT(result != nullptr);
    return result;
}

EcmaString *EcmaStringTable::GetOrInternString(EcmaVM *vm, const uint16_t *utf16Data, uint32_t utf16Len,
                                               bool canBeCompress)
{
    uint32_t hashcode = EcmaStringAccessor::ComputeHashcodeUtf16(const_cast<uint16_t *>(utf16Data), utf16Len);

    EcmaString *result = stringTable_.LoadOrStore<true>(
        vm, hashcode,
        [vm, utf16Data, utf16Len, canBeCompress, hashcode]() {
            EcmaString *value = EcmaStringAccessor::CreateFromUtf16(vm, utf16Data, utf16Len, canBeCompress,
                                                                    MemSpaceType::SHARED_OLD_SPACE);
            value->SetRawHashcode(hashcode);
            ASSERT(!EcmaStringAccessor(value).IsInternString());
            ASSERT(EcmaStringAccessor(value).NotTreeString());
        // Strings in string table should not be in the young space.
#ifdef USE_CMC_GC
            ASSERT(value->IsInSharedHeap());
#else
            ASSERT(Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(value))->InSharedHeap());
#endif
            JSThread *thread = vm->GetJSThread();
            JSHandle<EcmaString> stringHandle(thread, value);
            return stringHandle;
        },
        [utf16Data, utf16Len](EcmaString *foundString) {
            return EcmaStringAccessor::StringsAreEqualUtf16(foundString, utf16Data, utf16Len);
        });
    ASSERT(result != nullptr);
    return result;
}

EcmaString *EcmaStringTable::TryGetInternString(const JSHandle<EcmaString> &string)
{
    uint32_t hashcode = EcmaStringAccessor(*string).GetHashcode();
    return GetString(string, hashcode);
}

// used in jit thread, which unsupport create jshandle
EcmaString *EcmaStringTable::GetOrInternStringWithoutJSHandleForJit(EcmaVM *vm, const uint8_t *utf8Data,
                                                                    uint32_t utf8Len, bool canBeCompress,
                                                                    MemSpaceType type)
{
    ASSERT(IsSMemSpace(type));
    ASSERT(type == MemSpaceType::SHARED_NON_MOVABLE || type == MemSpaceType::SHARED_OLD_SPACE);
    uint32_t hashcode = EcmaStringAccessor::ComputeHashcodeUtf8(utf8Data, utf8Len, canBeCompress);
    EcmaString *result = stringTable_.LoadOrStoreForJit(
        vm, hashcode,
        [vm, utf8Data, utf8Len, canBeCompress, type, hashcode]() {
            EcmaString *value = EcmaStringAccessor::CreateFromUtf8(vm, utf8Data, utf8Len, canBeCompress, type);

            value->SetRawHashcode(hashcode);
            ASSERT(!EcmaStringAccessor(value).IsInternString());
            ASSERT(EcmaStringAccessor(value).NotTreeString());
        // Strings in string table should not be in the young space.
#ifdef USE_CMC_GC
            ASSERT(value->IsInSharedHeap());
#else
            ASSERT(Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(value))->InSharedHeap());
#endif
            return value;
        },
        [utf8Data, utf8Len, canBeCompress](EcmaString *foundString) {
            return EcmaStringAccessor::StringIsEqualUint8Data(foundString, utf8Data, utf8Len, canBeCompress);
        });
    ASSERT(result != nullptr);
    return result;
}

// used in jit thread, which unsupport create jshandle
EcmaString *EcmaStringTable::GetOrInternStringWithoutJSHandleForJit(EcmaVM *vm, const uint8_t *utf8Data,
                                                                    uint32_t utf16Len, MemSpaceType type)
{
    ASSERT(vm->GetJSThread()->IsJitThread());
    ASSERT(IsSMemSpace(type));
    type = (type == MemSpaceType::SHARED_NON_MOVABLE) ? type : MemSpaceType::SHARED_OLD_SPACE;
    CVector<uint16_t> u16Buffer(utf16Len);
    utf::ConvertRegionMUtf8ToUtf16(utf8Data, u16Buffer.data(), utf::Mutf8Size(utf8Data), utf16Len, 0);
    uint32_t hashcode = EcmaStringAccessor::ComputeHashcodeUtf16(u16Buffer.data(), utf16Len);
    const uint16_t *utf16Data = u16Buffer.data();

    EcmaString *result = stringTable_.LoadOrStoreForJit(
        vm, hashcode,
        [vm, u16Buffer, utf16Len, hashcode, type]() {
            EcmaString *value = EcmaStringAccessor::CreateFromUtf16(vm, u16Buffer.data(), utf16Len, false, type);
            value->SetRawHashcode(hashcode);
            ASSERT(!EcmaStringAccessor(value).IsInternString());
            ASSERT(EcmaStringAccessor(value).NotTreeString());
        // Strings in string table should not be in the young space.
#ifdef USE_CMC_GC
            ASSERT(value->IsInSharedHeap());
#else
            ASSERT(Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(value))->InSharedHeap());
#endif
            return value;
        },
        [utf16Data, utf16Len](EcmaString *foundString) {
            return EcmaStringAccessor::StringsAreEqualUtf16(foundString, utf16Data, utf16Len);
        });
    ASSERT(result != nullptr);
    return result;
}

void EcmaStringTable::SweepWeakRef(const WeakRootVisitor &visitor)
{
    // No need lock here, only shared gc will sweep string table, meanwhile other
    // threads are suspended.
    HashTrieMap::Indirect *root_node = stringTable_.root_.load(std::memory_order_relaxed);
    if (root_node == nullptr) {
        return;
    }
    for (uint32_t index = 0; index < stringTable_.INDIRECT_SIZE; ++index) {
        SweepWeakRef(visitor, index);
    }
}

void EcmaStringTable::SweepWeakRef(const WeakRootVisitor &visitor, uint32_t index)
{
    ASSERT(index >= 0 && index < stringTable_.INDIRECT_SIZE);
    HashTrieMap::Indirect *root_node = stringTable_.root_.load(std::memory_order_relaxed);
    stringTable_.ClearNodeFromGC(root_node, index, visitor);
}

bool EcmaStringTable::CheckStringTableValidity()
{
    bool isValid = true;
    stringTable_.Range(isValid);
    return isValid;
}

JSTaggedValue SingleCharTable::CreateSingleCharTable(JSThread *thread)
{
    auto table = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(MAX_ONEBYTE_CHARCODE, JSTaggedValue::Undefined(),
                                                                   MemSpaceType::SHARED_NON_MOVABLE);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    for (uint32_t i = 1; i < MAX_ONEBYTE_CHARCODE; ++i) {
        std::string tmp(1, i + 0X00);  // 1: size
        table->Set(thread, i, factory->NewFromASCIIReadOnly(tmp).GetTaggedValue());
    }
    return table.GetTaggedValue();
}

// This should only call in Debugger Signal, and need to fix and remove
EcmaString *EcmaStringTable::GetOrInternStringThreadUnsafe(EcmaVM *vm, const JSHandle<EcmaString> firstString,
                                                           const JSHandle<EcmaString> secondString)
{
    ASSERT(vm->GetJsDebuggerManager()->GetSignalState());
    JSThread *thread = vm->GetJSThreadNoCheck();
    JSHandle<EcmaString> firstFlat(thread, EcmaStringAccessor::Flatten(vm, firstString));
    JSHandle<EcmaString> secondFlat(thread, EcmaStringAccessor::Flatten(vm, secondString));
    uint32_t hashcode = EcmaStringAccessor::CalculateAllConcatHashCode(firstFlat, secondFlat);
    ASSERT(EcmaStringAccessor(firstFlat).NotTreeString());
    ASSERT(EcmaStringAccessor(secondFlat).NotTreeString());
    EcmaString *result = stringTable_.LoadOrStore<false>(
        vm, hashcode,
        [hashcode, thread, vm, firstFlat, secondFlat]() {
            JSHandle<EcmaString> concatHandle(
                thread, EcmaStringAccessor::Concat(vm, firstFlat, secondFlat, MemSpaceType::SHARED_OLD_SPACE));
            EcmaString *concatString = EcmaStringAccessor::Flatten(vm, concatHandle, MemSpaceType::SHARED_OLD_SPACE);
            concatString->SetRawHashcode(hashcode);
            ASSERT(!EcmaStringAccessor(concatString).IsInternString());
            ASSERT(EcmaStringAccessor(concatString).NotTreeString());
        // Strings in string table should not be in the young space.
#ifdef USE_CMC_GC
            ASSERT(concatString->IsInSharedHeap());
#else
            ASSERT(Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(concatString))->InSharedHeap());
#endif
            JSHandle<EcmaString> stringHandle(thread, concatString);
            return stringHandle;
        },
        [firstFlat, secondFlat](EcmaString *foundString) {
            EcmaString *firstStr = *firstFlat;
            EcmaString *secondStr = *secondFlat;
            return EcmaStringAccessor(foundString).EqualToSplicedString(firstStr, secondStr);
        });
    ASSERT(result != nullptr);
    return result;
}

// This should only call in Debugger Signal, and need to fix and remove
EcmaString *EcmaStringTable::GetOrInternStringThreadUnsafe(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len,
                                                           bool canBeCompress)
{
    ASSERT(vm->GetJsDebuggerManager()->GetSignalState());
    uint32_t hashcode = EcmaStringAccessor::ComputeHashcodeUtf8(utf8Data, utf8Len, canBeCompress);
    EcmaString *result = stringTable_.LoadOrStore<false>(
        vm, hashcode,
        [vm, utf8Data, utf8Len, canBeCompress, hashcode]() {
            EcmaString *value = EcmaStringAccessor::CreateFromUtf8(vm, utf8Data, utf8Len, canBeCompress,
                                                                   MemSpaceType::SHARED_OLD_SPACE);
            value->SetRawHashcode(hashcode);
            JSThread *thread = vm->GetJSThread();
            JSHandle<EcmaString> stringHandle(thread, value);
            return stringHandle;
        },
        [utf8Data, utf8Len, canBeCompress](EcmaString *foundString) {
            return EcmaStringAccessor::StringIsEqualUint8Data(foundString, utf8Data, utf8Len, canBeCompress);
        });
    ASSERT(result != nullptr);
    return result;
}

// Load returns the value of the key stored in the mapping, or HashTrieMapLoadResult for StoreOrLoad
HashTrieMap::HashTrieMapLoadResult HashTrieMap::Load(const uint32_t key, EcmaString *value)
{
    uint32_t hash = key;
    Indirect *current = root_.load(std::memory_order_relaxed);
    for (uint32_t hashShift = 0; hashShift < TOTAL_HASH_BITS; hashShift += N_CHILDREN_LOG2) {
        size_t index = (hash >> hashShift) & N_CHILDREN_MASK;

        std::atomic<Node *> *slot = &current->children_[index];
        Node *node = slot->load(std::memory_order_acquire);
        if (node == nullptr) {
            return {nullptr, current, hashShift, slot};
        }
        if (node->IsEntry()) {
            for (Entry *currentEntry = node->AsEntry(); currentEntry != nullptr;
                 currentEntry = currentEntry->Overflow().load(std::memory_order_acquire)) {
                if (currentEntry->Key() == key && EcmaStringAccessor::StringsAreEqual(currentEntry->Value(), value)) {
                    return {currentEntry->Value(), nullptr, hashShift, nullptr};
                }
            }
            return {nullptr, current, hashShift, slot};
        }
        current = node->AsIndirect();
    }

    LOG_ECMA(FATAL) << "StringTable: ran out of hash bits while iterating";
    UNREACHABLE();
}

// Load returns the value of the key stored in the mapping, or HashTrieMapLoadResult for StoreOrLoad
HashTrieMap::HashTrieMapLoadResult HashTrieMap::Load(const uint32_t key, const JSHandle<EcmaString> &string,
                                                     uint32_t offset, uint32_t utf8Len)
{
    uint32_t hash = key;
    Indirect *current = root_.load(std::memory_order_relaxed);
    const uint8_t *utf8Data = EcmaStringAccessor(string).GetDataUtf8() + offset;
    for (uint32_t hashShift = 0; hashShift < TOTAL_HASH_BITS; hashShift += N_CHILDREN_LOG2) {
        size_t index = (hash >> hashShift) & N_CHILDREN_MASK;

        std::atomic<Node *> *slot = &current->children_[index];
        Node *node = slot->load(std::memory_order_acquire);
        if (node == nullptr) {
            return {nullptr, current, hashShift, slot};
        }
        if (node->IsEntry()) {
            for (Entry *currentEntry = node->AsEntry(); currentEntry != nullptr;
                 currentEntry = currentEntry->Overflow().load(std::memory_order_acquire)) {
                if (currentEntry->Key() == key &&
                    EcmaStringAccessor::StringIsEqualUint8Data(currentEntry->Value(), utf8Data, utf8Len, true)) {
                    return {currentEntry->Value(), nullptr, hashShift, nullptr};
                }
            }
            return {nullptr, current, hashShift, slot};
        }
        current = node->AsIndirect();
    }

    LOG_ECMA(FATAL) << "StringTable: ran out of hash bits while iterating";
    UNREACHABLE();
}

// Based on the loadResult, try the store first
// StoreOrLoad returns the existing value of the key, store the value, and return the value
EcmaString *HashTrieMap::StoreOrLoad(EcmaVM *vm, const uint32_t key, HashTrieMapLoadResult loadResult,
                                     JSHandle<EcmaString> str)
{
    HashTrieMapInUseScope mapInUse(this);
    uint32_t hash = key;
    uint32_t hashShift = loadResult.hashShift;
    std::atomic<Node *> *slot = loadResult.slot;
    Node *node = nullptr;
    bool haveInsertPoint = true;
    Indirect *current = loadResult.current;
    RuntimeLock(vm->GetJSThread(), current->GetMutex());
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    node = slot->load(std::memory_order_acquire);
    if (node != nullptr && !node->IsEntry()) {
        RuntimeUnLock(current->GetMutex());
        current = node->AsIndirect();
        hashShift += N_CHILDREN_LOG2;
        while (true) {
            haveInsertPoint = false;
            for (; hashShift < TOTAL_HASH_BITS; hashShift += N_CHILDREN_LOG2) {
                size_t index = (hash >> hashShift) & N_CHILDREN_MASK;
                slot = &current->children_[index];
                node = slot->load(std::memory_order_acquire);
                if (node == nullptr) {
                    haveInsertPoint = true;
                    break;
                }
                // Entry, Search in overflow
                if (node->IsEntry()) {
                    for (Entry *currentEntry = node->AsEntry(); currentEntry != nullptr;
                         currentEntry = currentEntry->Overflow().load(std::memory_order_acquire)) {
                        if (currentEntry->Key() == key &&
                            EcmaStringAccessor::StringsAreEqual(currentEntry->Value(), *str)) {
                            return currentEntry->Value();
                        }
                    }
                    haveInsertPoint = true;
                    break;
                }
                // Indirect, Next level Continue to search
                current = node->AsIndirect();
            }
#ifndef NDEBUG
            if (!haveInsertPoint) {
                LOG_ECMA(FATAL) << "StringTable: ran out of hash bits while iterating";
                UNREACHABLE();
            }
#endif
            // lock and double-check
            RuntimeLock(vm->GetJSThread(), current->GetMutex());
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
            if (vm->IsCollectingScopeLockStats()) {
                vm->IncreaseStringTableLockCount();
            }
#endif
            node = slot->load(std::memory_order_acquire);
            if (node == nullptr || node->IsEntry()) {
                // see is still real, so can continue to insert.
                break;
            }

            RuntimeUnLock(current->GetMutex());
            current = node->AsIndirect();
            hashShift += N_CHILDREN_LOG2;
        }
    }

    Entry *oldEntry = nullptr;
    if (node != nullptr) {
        oldEntry = node->AsEntry();
        for (Entry *currentEntry = oldEntry; currentEntry;
             currentEntry = currentEntry->Overflow().load(std::memory_order_acquire)) {
            if (currentEntry->Key() == key && EcmaStringAccessor::StringsAreEqual(currentEntry->Value(), *str)) {
                RuntimeUnLock(current->GetMutex());
                return currentEntry->Value();
            }
        }
    }

    EcmaString *value = *str;
    EcmaStringAccessor(value).SetInternString();
    Entry *newEntry = new Entry(key, value);
    if (oldEntry == nullptr) {
        // The simple case: Create a new entry and store it.
        slot->store(newEntry, std::memory_order_release);
    } else {
        // Expand an existing entry to one or more new nodes.
        // Release the node, which will make both oldEntry and newEntry visible
        auto expandedNode = Expand<true>(oldEntry, newEntry, hash, hashShift, current);
        slot->store(expandedNode, std::memory_order_release);
    }

    RuntimeUnLock(current->GetMutex());
    return value;
}

bool HashTrieMap::CheckWeakRef(const WeakRootVisitor &visitor, HashTrieMap::Entry *entry)
{
    EcmaString *object = entry->Value();
    auto fwd = visitor(object);
#ifdef USE_CMC_GC
    ASSERT(object->IsInSharedHeap());
#else
    ASSERT(Region::ObjectAddressToRange((object))->InSharedHeap());
#endif
    if (fwd == nullptr) {
        LOG_ECMA(VERBOSE) << "StringTable: delete string " << std::hex << object;
        return true;
    } else if (fwd != object) {
        entry->SetValue(static_cast<EcmaString *>(fwd));
        LOG_ECMA(VERBOSE) << "StringTable: forward " << std::hex << object << " -> " << fwd;
    }
    return false;
}

bool HashTrieMap::CheckValidity(EcmaString *value, bool &isValid)
{
    if (!EcmaStringAccessor(value).NotTreeString()) {
        isValid = false;
        return false;
    }
    uint32_t hashcode = EcmaStringAccessor(value).GetHashcode();
    if (this->Load<true>(hashcode, value) != nullptr) {
        isValid = false;
        return false;
    }
    return true;
}

void HashTrieMap::Iter(Indirect *node, bool &isValid)
{
    if (node == nullptr)
        return;

    for (std::atomic<Node *> &child : node->children_) {
        Node *childNode = child.load(std::memory_order_relaxed);
        if (childNode == nullptr)
            continue;

        if (!(childNode->IsEntry())) {
            // Recursive traversal of indirect nodes
            Iter(childNode->AsIndirect(), isValid);
            continue;
        }

        for (Entry *e = childNode->AsEntry(); e != nullptr; e = e->Overflow().load(std::memory_order_relaxed)) {
            if (!CheckValidity(e->Value(), isValid)) {
                return;  // 用户终止迭代
            }
        }
    }
}

// Clear
void HashTrieMap::Clear()
{
    // The atom replaces the root node with nullptr and obtains the old root node
    Indirect *oldRoot = root_.exchange(nullptr, std::memory_order_relaxed);
    if (oldRoot != nullptr) {
        // Clear the entire HashTreeMap based on the Indirect destructor
        delete oldRoot;
    }
}

bool HashTrieMap::ClearNodeFromGC(Indirect *parent, int index, const WeakRootVisitor &visitor)
{
    // load sub-nodes
    Node *child = parent->children_[index].load(std::memory_order_relaxed);
    if (child == nullptr)
        return true;

    if (child->IsEntry()) {
        Entry *entry = child->AsEntry();
        // Processing the overflow linked list
        for (Entry *prev = nullptr, *current = entry->Overflow().load(std::memory_order_relaxed); current != nullptr;) {
            Entry *next = current->Overflow().load(std::memory_order_relaxed);
            if (CheckWeakRef(visitor, current)) {
                // Remove and remove a node from a linked list
                if (prev) {
                    prev->Overflow().store(next, std::memory_order_relaxed);
                } else {
                    entry->Overflow().store(next, std::memory_order_relaxed);
                }
                delete current;
            } else {
                prev = current;
            }
            current = next;
        }
        // processing entry node
        if (CheckWeakRef(visitor, entry)) {
            Entry *e = entry->Overflow().load(std::memory_order_relaxed);
            if (e == nullptr) {
                // Delete the empty Entry node and update the parent reference
                delete entry;
                parent->children_[index].store(nullptr, std::memory_order_relaxed);
                return true;
            }
            // Delete the Entry node and update the parent reference
            delete entry;
            parent->children_[index].store(e, std::memory_order_relaxed);
        }
        return false;
    } else {
        // Recursive processing of the Indirect node
        Indirect *indirect = child->AsIndirect();
        uint32_t cleanCount = 0;
        for (uint32_t i = 0; i < INDIRECT_SIZE; ++i) {
            if (ClearNodeFromGC(indirect, i, visitor)) {
                cleanCount += 1;
            }
        }
        // Check whether the indirect node is empty
        if (cleanCount == INDIRECT_SIZE && inuseCount_ == 0) {
            // Remove the empty Indirect and update the parent reference
            delete indirect;
            parent->children_[index].store(nullptr, std::memory_order_relaxed);
            return true;
        }
        return false;
    }
}

#ifdef USE_CMC_GC

bool HashTrieMap::CheckWeakRef(WeakVisitor &visitor, HashTrieMap::Entry *entry)
{
    ObjectSlot slot(reinterpret_cast<uintptr_t>(entry->ValueAddress()));
    bool isAlive = visitor.VisitRoot(Root::ROOT_VM, slot);
    if (!isAlive) {
        return true;
    }
    return false;
}

void EcmaStringTable::IterWeakRoot(WeakVisitor &visitor)
{
    // No need lock here, only shared gc will sweep string table, meanwhile other threads are suspended.
    for (uint32_t index = 0; index < stringTable_.INDIRECT_SIZE; ++index) {
        IterWeakRoot(visitor, index);
    }
}

void EcmaStringTable::IterWeakRoot(WeakVisitor &visitor, uint32_t index)
{
    ASSERT(index >= 0 && index < stringTable_.INDIRECT_SIZE);
    HashTrieMap::Indirect *root_node = stringTable_.root_.load(std::memory_order_relaxed);
    stringTable_.ClearNodeFromGC(root_node, index, visitor);
}

bool HashTrieMap::ClearNodeFromGC(Indirect *parent, int index, WeakVisitor &visitor)
{
    // load sub-nodes
    Node *child = parent->children_[index].load(std::memory_order_relaxed);
    if (child == nullptr)
        return true;

    if (child->IsEntry()) {
        Entry *entry = child->AsEntry();
        // Processing the overflow linked list
        for (Entry *prev = nullptr, *current = entry->Overflow().load(std::memory_order_relaxed); current != nullptr;) {
            Entry *next = current->Overflow().load(std::memory_order_relaxed);
            if (CheckWeakRef(visitor, current)) {
                // Remove and remove a node from a linked list
                if (prev) {
                    prev->Overflow().store(next, std::memory_order_relaxed);
                } else {
                    entry->Overflow().store(next, std::memory_order_relaxed);
                }
                delete current;
            } else {
                prev = current;
            }
            current = next;
        }
        // processing entry node
        if (CheckWeakRef(visitor, entry)) {
            Entry *e = entry->Overflow().load(std::memory_order_relaxed);
            if (e == nullptr) {
                // Delete the empty Entry node and update the parent reference
                delete entry;
                parent->children_[index].store(nullptr, std::memory_order_relaxed);
                return true;
            }
            // Delete the Entry node and update the parent reference
            delete entry;
            parent->children_[index].store(e, std::memory_order_relaxed);
        }
        return false;
    } else {
        // Recursive processing of the Indirect node
        Indirect *indirect = child->AsIndirect();
        uint32_t cleanCount = 0;
        for (uint32_t i = 0; i < INDIRECT_SIZE; ++i) {
            if (ClearNodeFromGC(indirect, i, visitor)) {
                cleanCount += 1;
            }
        }
        // Check whether the indirect node is empty
        if (cleanCount == INDIRECT_SIZE) {
            // Remove the empty Indirect and update the parent reference
            delete indirect;
            parent->children_[index].store(nullptr, std::memory_order_relaxed);
            return true;
        }
        return false;
    }
}
#endif

#endif
}  // namespace panda::ecmascript
