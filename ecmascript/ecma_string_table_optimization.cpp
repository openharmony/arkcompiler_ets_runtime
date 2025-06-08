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

#include "common_components/objects/string_table/hashtriemap-inl.h"
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
    while ((index = GetNextIndexId(iter)) < HashTrieMap<StringTableMutex, JSThread>::INDIRECT_SIZE) {
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
    PendingTaskCount_.store(HashTrieMap<StringTableMutex, JSThread>::INDIRECT_SIZE, std::memory_order_relaxed);
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
    auto readBarrier = [](const void* obj, size_t offset)-> TaggedObject* {
        return Barriers::GetTaggedObject(obj, offset);
    };
    return EcmaString::FromBaseString(stringTable_.Load<false>(std::move(readBarrier), hashcode, str->ToBaseString()));
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
    auto readBarrier = [](const void* obj, size_t offset)-> TaggedObject* {
        return Barriers::GetTaggedObject(obj, offset);
    };
    auto loadResult = stringTable_.Load(std::move(readBarrier), hashcode, string->ToBaseString());
    if (loadResult.value != nullptr) {
        return EcmaString::FromBaseString(loadResult.value);
    }
    JSThread *thread = vm->GetJSThread();
    JSHandle<EcmaString> stringHandle(thread, string);
    BaseString* result = stringTable_.StoreOrLoad<true, decltype(readBarrier), ReadOnlyHandle<BaseString>>(
        vm->GetJSThread(), std::move(readBarrier), hashcode, loadResult, stringHandle);
    ASSERT(result != nullptr);
    return EcmaString::FromBaseString(result);
}

EcmaString *EcmaStringTable::GetOrInternStringFromCompressedSubString(EcmaVM *vm, const JSHandle<EcmaString> &string,
                                                                      uint32_t offset, uint32_t utf8Len)
{
    const uint8_t *utf8Data = EcmaStringAccessor(string).GetDataUtf8() + offset;
    uint32_t hashcode = EcmaStringAccessor::ComputeHashcodeUtf8(utf8Data, utf8Len, true);
    auto readBarrier = [](const void* obj, size_t offset)-> TaggedObject* {
        return Barriers::GetTaggedObject(obj, offset);
    };
    auto loadResult = stringTable_.Load(std::move(readBarrier), hashcode, string, offset, utf8Len);
    if (loadResult.value != nullptr) {
        return EcmaString::FromBaseString(loadResult.value);
    }
    BaseString *result = stringTable_.StoreOrLoad(
        vm->GetJSThread(), hashcode, loadResult,
        [vm, string, offset, utf8Len, hashcode]()->ReadOnlyHandle<BaseString> {
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
        [utf8Len, string, offset](BaseString *foundString) {
            const uint8_t *utf8Data = EcmaStringAccessor(string).GetDataUtf8() + offset;
            return EcmaStringAccessor::StringIsEqualUint8Data(EcmaString::FromBaseString(foundString), utf8Data,
                                                              utf8Len, true);
        });
    ASSERT(result != nullptr);
    return EcmaString::FromBaseString(result);
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
    auto readBarrier = [](const void* obj, size_t offset)-> TaggedObject* {
        return Barriers::GetTaggedObject(obj, offset);
    };
    auto loadResult = stringTable_.Load(std::move(readBarrier), hashcode, strFlat->ToBaseString());
    if (loadResult.value != nullptr) {
        return EcmaString::FromBaseString(loadResult.value);
    }
    JSHandle<EcmaString> strFlatHandle(thread, strFlat);
    BaseString* result = stringTable_.StoreOrLoad<true, decltype(readBarrier), ReadOnlyHandle<BaseString>>(
        vm->GetJSThread(), std::move(readBarrier), hashcode, loadResult, strFlatHandle);
    ASSERT(result != nullptr);
    return EcmaString::FromBaseString(result);
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
    BaseString *result = stringTable_.LoadOrStore<true>(
        vm->GetJSThread(), hashcode,
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
        [firstFlat, secondFlat](BaseString *foundString) {
            EcmaString *firstStr = *firstFlat;
            EcmaString *secondStr = *secondFlat;
            return EcmaStringAccessor(EcmaString::FromBaseString(foundString)).
                EqualToSplicedString(firstStr, secondStr);
        });
    ASSERT(result != nullptr);
    return EcmaString::FromBaseString(result);
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
    BaseString *result = stringTable_.LoadOrStore<true>(
        vm->GetJSThread(), hashcode,
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
        [utf8Data, utf8Len, canBeCompress](BaseString *foundString) {
            return EcmaStringAccessor::StringIsEqualUint8Data(EcmaString::FromBaseString(foundString), utf8Data,
                                                              utf8Len, canBeCompress);
        });
    ASSERT(result != nullptr);
    return EcmaString::FromBaseString(result);
}

EcmaString* EcmaStringTable::GetOrInternString(EcmaVM* vm, const uint8_t* utf8Data, uint32_t utf16Len,
                                               MemSpaceType type)
{
    ASSERT(IsSMemSpace(type));
    ASSERT(type == MemSpaceType::SHARED_NON_MOVABLE || type == MemSpaceType::SHARED_OLD_SPACE);
    JSThread* thread = vm->GetJSThread();
    EcmaString* str = EcmaStringAccessor::CreateUtf16StringFromUtf8(vm, utf8Data, utf16Len, type);
    uint32_t hashcode = EcmaStringAccessor(str).GetHashcode();
    auto readBarrier = [](const void* obj, size_t offset)-> TaggedObject* {
        return Barriers::GetTaggedObject(obj, offset);
    };
    auto loadResult = stringTable_.Load(std::move(readBarrier), hashcode, str->ToBaseString());
    if (loadResult.value != nullptr) {
        return EcmaString::FromBaseString(loadResult.value);
    }
    JSHandle<EcmaString> strHandle(thread, str);
    BaseString* result = stringTable_.StoreOrLoad<true, decltype(readBarrier), ReadOnlyHandle<BaseString>>(
        vm->GetJSThread(), std::move(readBarrier), hashcode, loadResult, strHandle);
    ASSERT(result != nullptr);
    return EcmaString::FromBaseString(result);
}

EcmaString *EcmaStringTable::GetOrInternString(EcmaVM *vm, const uint16_t *utf16Data, uint32_t utf16Len,
                                               bool canBeCompress)
{
    uint32_t hashcode = EcmaStringAccessor::ComputeHashcodeUtf16(const_cast<uint16_t *>(utf16Data), utf16Len);

    BaseString *result = stringTable_.LoadOrStore<true>(
        vm->GetJSThread(), hashcode,
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
        [utf16Data, utf16Len](BaseString *foundString) {
            return EcmaStringAccessor::StringsAreEqualUtf16(EcmaString::FromBaseString(foundString), utf16Data,
                                                            utf16Len);
        });
    ASSERT(result != nullptr);
    return EcmaString::FromBaseString(result);
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
    BaseString *result = stringTable_.LoadOrStoreForJit(
        vm->GetJSThread(), hashcode,
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
            return value->ToBaseString();
        },
        [utf8Data, utf8Len, canBeCompress](BaseString* foundString) {
            return EcmaStringAccessor::StringIsEqualUint8Data(EcmaString::FromBaseString(foundString), utf8Data,
                                                              utf8Len, canBeCompress);
        });
    ASSERT(result != nullptr);
    return EcmaString::FromBaseString(result);
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

    BaseString *result = stringTable_.LoadOrStoreForJit(
        vm->GetJSThread(), hashcode,
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
            return value->ToBaseString();
        },
        [utf16Data, utf16Len](BaseString* foundString) {
            return EcmaStringAccessor::StringsAreEqualUtf16(EcmaString::FromBaseString(foundString), utf16Data,
                                                            utf16Len);
        });
    ASSERT(result != nullptr);
    return EcmaString::FromBaseString(result);
}

void EcmaStringTable::SweepWeakRef(const WeakRootVisitor &visitor)
{
    // No need lock here, only shared gc will sweep string table, meanwhile other
    // threads are suspended.
    HashTrieMap<StringTableMutex, JSThread>::Indirect *root_node = stringTable_.root_.load(std::memory_order_relaxed);
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
    HashTrieMap<StringTableMutex, JSThread>::Indirect *root_node = stringTable_.root_.load(std::memory_order_relaxed);
    stringTable_.ClearNodeFromGC(root_node, index, visitor);
}

bool EcmaStringTable::CheckStringTableValidity()
{
    bool isValid = true;
    auto readBarrier = [](const void* obj, size_t offset)-> TaggedObject* {
        return Barriers::GetTaggedObject(obj, offset);
    };
    stringTable_.Range(std::move(readBarrier), isValid);
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
    BaseString *result = stringTable_.LoadOrStore<false>(
        vm->GetJSThread(), hashcode,
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
        [firstFlat, secondFlat](BaseString *foundString) {
            EcmaString *firstStr = *firstFlat;
            EcmaString *secondStr = *secondFlat;
            return EcmaStringAccessor(EcmaString::FromBaseString(foundString)).
                EqualToSplicedString(firstStr, secondStr);
        });
    ASSERT(result != nullptr);
    return EcmaString::FromBaseString(result);
}

// This should only call in Debugger Signal, and need to fix and remove
EcmaString *EcmaStringTable::GetOrInternStringThreadUnsafe(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len,
                                                           bool canBeCompress)
{
    ASSERT(vm->GetJsDebuggerManager()->GetSignalState());
    uint32_t hashcode = EcmaStringAccessor::ComputeHashcodeUtf8(utf8Data, utf8Len, canBeCompress);
    BaseString *result = stringTable_.LoadOrStore<false>(
        vm->GetJSThread(), hashcode,
        [vm, utf8Data, utf8Len, canBeCompress, hashcode]() {
            EcmaString *value = EcmaStringAccessor::CreateFromUtf8(vm, utf8Data, utf8Len, canBeCompress,
                                                                   MemSpaceType::SHARED_OLD_SPACE);
            value->SetRawHashcode(hashcode);
            JSThread *thread = vm->GetJSThread();
            JSHandle<EcmaString> stringHandle(thread, value);
            return stringHandle;
        },
        [utf8Data, utf8Len, canBeCompress](BaseString *foundString) {
            return EcmaStringAccessor::StringIsEqualUint8Data(EcmaString::FromBaseString(foundString), utf8Data,
                                                              utf8Len, canBeCompress);
        });
    ASSERT(result != nullptr);
    return EcmaString::FromBaseString(result);
}
#ifdef USE_CMC_GC
void EcmaStringTable::IterWeakRoot(const WeakRefFieldVisitor& visitor)
{
    // No need lock here, only shared gc will sweep string table, meanwhile other threads are suspended.
    for (uint32_t index = 0; index < stringTable_.INDIRECT_SIZE; ++index) {
        IterWeakRoot(visitor, index);
    }
}

void EcmaStringTable::IterWeakRoot(const WeakRefFieldVisitor &visitor, uint32_t index)
{
    ASSERT(index >= 0 && index < stringTable_.INDIRECT_SIZE);
    HashTrieMap<StringTableMutex, JSThread>::Indirect *root_node = stringTable_.root_.load(std::memory_order_relaxed);
    stringTable_.ClearNodeFromGC(root_node, index, visitor);
}
#endif
#endif
}  // namespace panda::ecmascript
