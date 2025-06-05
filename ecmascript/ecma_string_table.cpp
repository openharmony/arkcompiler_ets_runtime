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

#include "ecmascript/ecma_string_table.h"

#include "common_components/taskpool/taskpool.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/runtime_lock.h"
namespace panda::ecmascript {
#if !ENABLE_NEXT_OPTIMIZATION
void EcmaStringTableCleaner::PostSweepWeakRefTask(const WeakRootVisitor &visitor)
{
    StartSweepWeakRefTask();
    iter_ = std::make_shared<std::atomic<uint32_t>>(0U);
    const uint32_t postTaskCount = common::Taskpool::GetCurrentTaskpool()->GetTotalThreadNum();
    for (uint32_t i = 0U; i < postTaskCount; ++i) {
        common::Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<SweepWeakRefTask>(iter_, this, visitor));
    }
}

void EcmaStringTableCleaner::JoinAndWaitSweepWeakRefTask(const WeakRootVisitor &visitor)
{
    ProcessSweepWeakRef(iter_, this, visitor);
    WaitSweepWeakRefTask();
    iter_.reset();
}

void EcmaStringTableCleaner::ProcessSweepWeakRef(IteratorPtr& iter, EcmaStringTableCleaner *cleaner,
                                                 const WeakRootVisitor &visitor)
{
    uint32_t tableId = 0U;
    while ((tableId = GetNextTableId(iter)) < EcmaStringTable::SEGMENT_COUNT) {
        cleaner->stringTable_->SweepWeakRef(visitor, tableId);
        if (ReduceCountAndCheckFinish(cleaner)) {
            cleaner->SignalSweepWeakRefTask();
        }
    }
}

void EcmaStringTableCleaner::StartSweepWeakRefTask()
{
    // No need lock here, only the daemon thread will reset the state.
    sweepWeakRefFinished_ = false;
    PendingTaskCount_.store(EcmaStringTable::SEGMENT_COUNT, std::memory_order_relaxed);
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

EcmaString *EcmaStringTable::GetStringFromCompressedSubString(JSThread *thread, const JSHandle<EcmaString> string,
                                                              uint32_t offset, uint32_t utf8Len, uint32_t hashcode)
{
    RuntimeLockHolder locker(thread, stringTable_[GetTableId(hashcode)].mutex_);
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    EcmaVM *vm = thread->GetEcmaVM();
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    const uint8_t *utf8Data = EcmaStringAccessor(string).GetDataUtf8() + offset;
    auto range = stringTable_[GetTableId(hashcode)].table_.equal_range(hashcode);
    for (auto item = range.first; item != range.second;) {
        auto foundString = (item++)->second;
        if (EcmaStringAccessor::StringIsEqualUint8Data(foundString, utf8Data, utf8Len, true)) {
            return foundString;
        }
    }
    return nullptr;
}

EcmaString *EcmaStringTable::GetString(JSThread *thread, JSHandle<EcmaString> string, uint32_t hashcode)
{
    RuntimeLockHolder locker(thread, stringTable_[GetTableId(hashcode)].mutex_);
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    EcmaVM *vm = thread->GetEcmaVM();
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    EcmaString *str = *string;
    auto range = stringTable_[GetTableId(hashcode)].table_.equal_range(hashcode);
    for (auto item = range.first; item != range.second;) {
        auto foundString = (item++)->second;
        if (EcmaStringAccessor::StringsAreEqual(foundString, str)) {
            return foundString;
        }
    }
    return nullptr;
}

EcmaString *EcmaStringTable::GetString(JSThread *thread, const JSHandle<EcmaString> firstString,
                                       const JSHandle<EcmaString> secondString, uint32_t hashcode)
{
    ASSERT(EcmaStringAccessor(firstString).NotTreeString());
    ASSERT(EcmaStringAccessor(secondString).NotTreeString());
    RuntimeLockHolder locker(thread, stringTable_[GetTableId(hashcode)].mutex_);
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    EcmaVM *vm = thread->GetEcmaVM();
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    EcmaString *firstStr = *firstString;
    EcmaString *secondStr = *secondString;
    auto range = stringTable_[GetTableId(hashcode)].table_.equal_range(hashcode);
    for (auto item = range.first; item != range.second;) {
        auto foundString = (item++)->second;
        if (EcmaStringAccessor(foundString).EqualToSplicedString(firstStr, secondStr)) {
            return foundString;
        }
    }
    return nullptr;
}

EcmaString *EcmaStringTable::GetString(JSThread *thread, const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress,
                                       uint32_t hashcode)
{
    RuntimeLockHolder locker(thread, stringTable_[GetTableId(hashcode)].mutex_);
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    EcmaVM *vm = thread->GetEcmaVM();
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    auto range = stringTable_[GetTableId(hashcode)].table_.equal_range(hashcode);
    for (auto item = range.first; item != range.second;) {
        auto foundString = (item++)->second;
        if (EcmaStringAccessor::StringIsEqualUint8Data(foundString, utf8Data, utf8Len, canBeCompress)) {
            return foundString;
        }
    }
    return nullptr;
}

EcmaString *EcmaStringTable::GetString(JSThread *thread, const uint16_t *utf16Data, uint32_t utf16Len,
                                       uint32_t hashcode)
{
    RuntimeLockHolder locker(thread, stringTable_[GetTableId(hashcode)].mutex_);
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    EcmaVM *vm = thread->GetEcmaVM();
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    auto range = stringTable_[GetTableId(hashcode)].table_.equal_range(hashcode);
    for (auto item = range.first; item != range.second;) {
        auto foundString = (item++)->second;
        if (EcmaStringAccessor::StringsAreEqualUtf16(foundString, utf16Data, utf16Len)) {
            return foundString;
        }
    }
    return nullptr;
}

EcmaString *EcmaStringTable::GetStringThreadUnsafe(EcmaString *string, uint32_t hashcode) const
{
    ASSERT(EcmaStringAccessor(string).NotTreeString());
    auto range = stringTable_[GetTableId(hashcode)].table_.equal_range(hashcode);
    for (auto item = range.first; item != range.second;) {
        auto foundString = (item++)->second;
        if (EcmaStringAccessor::StringsAreEqual(foundString, string)) {
            return foundString;
        }
    }
    return nullptr;
}

void EcmaStringTable::InternStringThreadUnsafe(EcmaString *string, uint32_t hashcode)
{
    ASSERT(!EcmaStringAccessor(string).IsInternString());
    ASSERT(EcmaStringAccessor(string).NotTreeString());
    // Strings in string table should not be in the young space.
    ASSERT(JSTaggedValue(string).IsInSharedHeap());
    if (EcmaStringAccessor(string).IsInternString()) {
        return;
    }
    ASSERT(!g_isEnableCMCGC || EcmaStringAccessor(string).NotTreeString());
    stringTable_[GetTableId(hashcode)].table_.emplace(hashcode, string);
    EcmaStringAccessor(string).SetInternString();
}

EcmaString *EcmaStringTable::AtomicGetOrInternStringImpl(JSThread *thread, const JSHandle<EcmaString> string,
                                                         uint32_t hashcode)
{
    ASSERT(EcmaStringAccessor(string).NotTreeString());
    RuntimeLockHolder locker(thread, stringTable_[GetTableId(hashcode)].mutex_);
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    EcmaVM *vm = thread->GetEcmaVM();
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    EcmaString *str = *string;
    EcmaString *result = GetStringThreadUnsafe(str, hashcode);
    if (result == nullptr) {
        InternStringThreadUnsafe(str, hashcode);
        return str;
    }
    return result;
}

EcmaString* EcmaStringTable::AtomicGetOrInternStringImplNoGC(JSThread *thread, EcmaString* string, uint32_t hashcode)
{
    ASSERT(EcmaStringAccessor(string).NotTreeString());
    Mutex& mutex = stringTable_[GetTableId(hashcode)].mutex_;
    mutex.Lock();
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    EcmaVM *vm = thread->GetEcmaVM();
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    EcmaString *result = GetStringThreadUnsafe(string, hashcode);
    if (result == nullptr) {
        InternStringThreadUnsafe(string, hashcode);
        mutex.Unlock();
        return string;
    }
    mutex.Unlock();
    return result;
}

EcmaString *EcmaStringTable::GetOrInternFlattenString(EcmaVM *vm, EcmaString *string)
{
    ASSERT(EcmaStringAccessor(string).NotTreeString());
    if (EcmaStringAccessor(string).IsInternString()) {
        return string;
    }
    JSThread *thread = vm->GetJSThread();
    JSHandle<EcmaString> stringHandle(thread, string);
    uint32_t hashcode = EcmaStringAccessor(string).GetHashcode();
    return AtomicGetOrInternStringImpl(thread, stringHandle, hashcode);
}

EcmaString* EcmaStringTable::GetOrInternFlattenStringNoGC(EcmaVM* vm, EcmaString* string)
{
    ASSERT(EcmaStringAccessor(string).NotTreeString());
    if (EcmaStringAccessor(string).IsInternString()) {
        return string;
    }
    uint32_t hashcode = EcmaStringAccessor(string).GetHashcode();
    return AtomicGetOrInternStringImplNoGC(vm->GetJSThread(), string, hashcode);
}

EcmaString *EcmaStringTable::GetOrInternStringFromCompressedSubString(EcmaVM *vm, const JSHandle<EcmaString> &string,
    uint32_t offset, uint32_t utf8Len)
{
    const uint8_t *utf8Data = EcmaStringAccessor(string).GetDataUtf8() + offset;
    uint32_t hashcode = EcmaStringAccessor::ComputeHashcodeUtf8(utf8Data, utf8Len, true);
    JSThread *thread = vm->GetJSThread();
    EcmaString *result = GetStringFromCompressedSubString(thread, string, offset, utf8Len, hashcode);
    if (result != nullptr) {
        return result;
    }
    utf8Data = EcmaStringAccessor(string).GetDataUtf8() + offset;
    EcmaString *str = EcmaStringAccessor::CreateFromUtf8CompressedSubString(vm, string, offset, utf8Len,
        MemSpaceType::SHARED_OLD_SPACE);
    JSHandle<EcmaString> strHandle(thread, str);
    strHandle->SetRawHashcode(hashcode);
    return AtomicGetOrInternStringImpl(thread, strHandle, hashcode);
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
    JSHandle<EcmaString> strFlatHandle(thread, strFlat);
    EcmaString *result = GetString(thread, strFlatHandle, hashcode);
    if (result != nullptr) {
        return result;
    }
    return AtomicGetOrInternStringImpl(thread, strFlatHandle, hashcode);
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
    EcmaString *result = GetString(thread, firstFlat, secondFlat, hashcode);
    if (result != nullptr) {
        return result;
    }
    JSHandle<EcmaString> concatHandle(thread,
        EcmaStringAccessor::Concat(vm, firstFlat, secondFlat, MemSpaceType::SHARED_OLD_SPACE));
    JSHandle<EcmaString> flattenConcatHandle(thread,
        EcmaStringAccessor::Flatten(vm, concatHandle, MemSpaceType::SHARED_OLD_SPACE));
    flattenConcatHandle->SetRawHashcode(hashcode);
    return AtomicGetOrInternStringImpl(thread, flattenConcatHandle, hashcode);
}

EcmaString *EcmaStringTable::GetOrInternString(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len,
                                               bool canBeCompress, MemSpaceType type)
{
    ASSERT(IsSMemSpace(type));
    bool signalState = vm->GetJsDebuggerManager()->GetSignalState();
    if (UNLIKELY(signalState)) {
        return GetOrInternStringThreadUnsafe(vm, utf8Data, utf8Len, canBeCompress);
    }
    JSThread *thread = vm->GetJSThread();
    uint32_t hashcode = EcmaStringAccessor::ComputeHashcodeUtf8(utf8Data, utf8Len, canBeCompress);
    EcmaString *result = GetString(thread, utf8Data, utf8Len, canBeCompress, hashcode);
    if (result != nullptr) {
        return result;
    }
    JSHandle<EcmaString> strHandle(thread,
        EcmaStringAccessor::CreateFromUtf8(vm, utf8Data, utf8Len, canBeCompress, type));
    strHandle->SetRawHashcode(hashcode);
    return AtomicGetOrInternStringImpl(thread, strHandle, hashcode);
}

EcmaString *EcmaStringTable::GetOrInternString(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf16Len,
                                               MemSpaceType type)
{
    ASSERT(IsSMemSpace(type));
    ASSERT(type == MemSpaceType::SHARED_NON_MOVABLE || type == MemSpaceType::SHARED_OLD_SPACE);
    JSThread *thread = vm->GetJSThread();
    EcmaString *str = EcmaStringAccessor::CreateUtf16StringFromUtf8(vm, utf8Data, utf16Len, type);
    JSHandle<EcmaString> strHandle(thread, str);
    uint32_t hashcode = EcmaStringAccessor(str).GetHashcode();
    return AtomicGetOrInternStringImpl(thread, strHandle, hashcode);
}

EcmaString *EcmaStringTable::GetOrInternString(EcmaVM *vm, const uint16_t *utf16Data, uint32_t utf16Len,
                                               bool canBeCompress)
{
    uint32_t hashcode = EcmaStringAccessor::ComputeHashcodeUtf16(const_cast<uint16_t *>(utf16Data), utf16Len);
    JSThread *thread = vm->GetJSThread();
    EcmaString *result = GetString(thread, utf16Data, utf16Len, hashcode);
    if (result != nullptr) {
        return result;
    }
    JSHandle<EcmaString> strHandle(thread,
        EcmaStringAccessor::CreateFromUtf16(vm, utf16Data, utf16Len, canBeCompress, MemSpaceType::SHARED_OLD_SPACE));
    strHandle->SetRawHashcode(hashcode);
    return AtomicGetOrInternStringImpl(thread, strHandle, hashcode);
}

void EcmaStringTable::InsertStringToTableWithHashThreadUnsafe(EcmaString* string, uint32_t hashcode)
{
    // Strings in string table should not be in the young space.
    ASSERT(JSTaggedValue(string).IsInSharedHeap());
    ASSERT(EcmaStringAccessor(string).NotTreeString());
    ASSERT(EcmaStringAccessor(string).GetHashcode() == hashcode);
    stringTable_[GetTableId(hashcode)].table_.emplace(hashcode, string);
    EcmaStringAccessor(string).SetInternString();
}

EcmaString *EcmaStringTable::InsertStringToTable(EcmaVM *vm, const JSHandle<EcmaString> &strHandle)
{
    JSThread *thread = vm->GetJSThread();
    EcmaString *strFlat = EcmaStringAccessor::Flatten(vm, strHandle, MemSpaceType::SHARED_OLD_SPACE);
    JSHandle<EcmaString> strFlatHandle(thread, strFlat);
    uint32_t hashcode = EcmaStringAccessor(strFlat).GetHashcode();
    RuntimeLockHolder locker(thread, stringTable_[GetTableId(hashcode)].mutex_);
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    strFlat = *strFlatHandle;
    InternStringThreadUnsafe(strFlat, hashcode);
    return strFlat;
}

EcmaString *EcmaStringTable::TryGetInternString(JSThread *thread, const JSHandle<EcmaString> &string)
{
    uint32_t hashcode = EcmaStringAccessor(*string).GetHashcode();
    return GetString(thread, string, hashcode);
}

// used in jit thread, which unsupport create jshandle
EcmaString *EcmaStringTable::GetOrInternStringWithoutJSHandleForJit(EcmaVM *vm, const uint8_t *utf8Data,
    uint32_t utf8Len, bool canBeCompress, MemSpaceType type)
{
    ASSERT(IsSMemSpace(type));
    ASSERT(type == MemSpaceType::SHARED_NON_MOVABLE || type == MemSpaceType::SHARED_OLD_SPACE);
    uint32_t hashcode = EcmaStringAccessor::ComputeHashcodeUtf8(utf8Data, utf8Len, canBeCompress);
    JSThread *thread = vm->GetJSThread();
    RuntimeLockHolder locker(thread, stringTable_[GetTableId(hashcode)].mutex_);
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    EcmaString *result = GetStringThreadUnsafe(utf8Data, utf8Len, canBeCompress, hashcode);
    if (result != nullptr) {
        return result;
    }
    EcmaString *str = nullptr;
    str = EcmaStringAccessor::CreateFromUtf8(vm, utf8Data, utf8Len, canBeCompress, type);
    str->SetRawHashcode(hashcode);
    InternStringThreadUnsafe(str, hashcode);
    return str;
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
    RuntimeLockHolder locker(vm->GetJSThread(), stringTable_[GetTableId(hashcode)].mutex_);
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    EcmaString *result = GetStringThreadUnsafe(u16Buffer.data(), utf16Len, hashcode);
    if (result != nullptr) {
        return result;
    }
    EcmaString *str = EcmaStringAccessor::CreateFromUtf16(vm, u16Buffer.data(), utf16Len, false, type);
    str->SetRawHashcode(hashcode);
    InternStringThreadUnsafe(str, hashcode);
    return str;
}

void EcmaStringTable::SweepWeakRef(const WeakRootVisitor &visitor)
{
    // No need lock here, only shared gc will sweep string table, meanwhile other threads are suspended.
    for (uint32_t tableId = 0; tableId < stringTable_.size(); ++tableId) {
        SweepWeakRef(visitor, tableId);
    }
}

void EcmaStringTable::SweepWeakRef(const WeakRootVisitor &visitor, uint32_t tableId)
{
    ASSERT(tableId >= 0 && tableId < stringTable_.size());
    auto &table = stringTable_[tableId].table_;
    for (auto it = table.begin(); it != table.end();) {
        auto *object = it->second;
        auto fwd = visitor(object);
        ASSERT(JSTaggedValue(object).IsInSharedHeap());
        if (fwd == nullptr) {
            LOG_ECMA(VERBOSE) << "StringTable: delete string " << std::hex << object;
            it = table.erase(it);
        } else if (fwd != object) {
            it->second = static_cast<EcmaString *>(fwd);
            ++it;
            LOG_ECMA(VERBOSE) << "StringTable: forward " << std::hex << object << " -> " << fwd;
        } else {
            ++it;
        }
    }
}

void EcmaStringTable::IterWeakRoot(WeakVisitor &visitor)
{
    // iter by cmc
    for (uint32_t tableId = 0; tableId < stringTable_.size(); ++tableId) {
        ASSERT(tableId >= 0 && tableId < stringTable_.size());
        auto &table = stringTable_[tableId].table_;
        for (auto it = table.begin(); it != table.end();) {
            ObjectSlot slot(reinterpret_cast<uintptr_t>(&(it->second)));
            bool isAlive = visitor.VisitRoot(Root::ROOT_VM, slot);
            if (!isAlive) {
                it = table.erase(it);
            } else {
                it++;
            }
        }
    }
}

bool EcmaStringTable::CheckStringTableValidity(JSThread *thread)
{
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    auto vm = thread->GetEcmaVM();
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    for (auto &[table, mutex] : stringTable_) {
        RuntimeLockHolder locker(thread, mutex);
        for (auto itemOuter = table.begin(); itemOuter != table.end(); ++itemOuter) {
            auto outerString = itemOuter->second;
            if (!EcmaStringAccessor(outerString).NotTreeString()) {
                return false;
            }
            int counter = 0;
            auto hashcode = EcmaStringAccessor(outerString).GetHashcode();
            auto range = table.equal_range(hashcode);
            for (auto it = range.first; it != range.second; ++it) {
                auto foundString = it->second;
                counter += EcmaStringAccessor::StringsAreEqual(foundString, outerString) ? 1 : 0;
            }
            if (counter > 1) {
                return false;
            }
        }
    }
    return true;
}

JSTaggedValue SingleCharTable::CreateSingleCharTable(JSThread *thread)
{
    auto table = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(MAX_ONEBYTE_CHARCODE,
        JSTaggedValue::Undefined(), MemSpaceType::SHARED_NON_MOVABLE);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    for (uint32_t i = 1; i < MAX_ONEBYTE_CHARCODE; ++i) {
        std::string tmp(1, i + 0X00); // 1: size
        table->Set(thread, i, factory->NewFromASCIIReadOnly(tmp).GetTaggedValue());
    }
    return table.GetTaggedValue();
}

// This should only call in Debugger Signal, and need to fix and remove
EcmaString *EcmaStringTable::GetOrInternStringThreadUnsafe(EcmaVM *vm,
                                                           const JSHandle<EcmaString> firstString,
                                                           const JSHandle<EcmaString> secondString)
{
    ASSERT(vm->GetJsDebuggerManager()->GetSignalState());
    JSThread *thread = vm->GetJSThreadNoCheck();
    JSHandle<EcmaString> firstFlat(thread, EcmaStringAccessor::Flatten(vm, firstString));
    JSHandle<EcmaString> secondFlat(thread, EcmaStringAccessor::Flatten(vm, secondString));
    uint32_t hashcode = EcmaStringAccessor::CalculateAllConcatHashCode(firstFlat, secondFlat);
    EcmaString *result = GetStringThreadUnsafe(firstFlat, secondFlat, hashcode);
    if (result != nullptr) {
        return result;
    }
    JSHandle<EcmaString> concatHandle(thread,
        EcmaStringAccessor::Concat(vm, firstFlat, secondFlat, MemSpaceType::SHARED_OLD_SPACE));
    EcmaString *concatString = EcmaStringAccessor::Flatten(vm, concatHandle, MemSpaceType::SHARED_OLD_SPACE);
    concatString->SetRawHashcode(hashcode);
    InternStringThreadUnsafe(concatString, hashcode);
    return concatString;
}

// This should only call in Debugger Signal, and need to fix and remove
EcmaString *EcmaStringTable::GetOrInternStringThreadUnsafe(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len,
                                                           bool canBeCompress)
{
    ASSERT(vm->GetJsDebuggerManager()->GetSignalState());
    uint32_t hashcode = EcmaStringAccessor::ComputeHashcodeUtf8(utf8Data, utf8Len, canBeCompress);
    EcmaString *result = GetStringThreadUnsafe(utf8Data, utf8Len, canBeCompress, hashcode);
    if (result != nullptr) {
        return result;
    }
    EcmaString *str =
        EcmaStringAccessor::CreateFromUtf8(vm, utf8Data, utf8Len, canBeCompress, MemSpaceType::SHARED_OLD_SPACE);
    str->SetRawHashcode(hashcode);
    InternStringThreadUnsafe(str, hashcode);
    return str;
}

// This should only call in Debugger Signal, and need to fix and remove
EcmaString *EcmaStringTable::GetStringThreadUnsafe(const JSHandle<EcmaString> firstString,
                                                   const JSHandle<EcmaString> secondString,
                                                   uint32_t hashcode) const
{
    ASSERT(EcmaStringAccessor(firstString).NotTreeString());
    ASSERT(EcmaStringAccessor(secondString).NotTreeString());
    EcmaString *firstStr = *firstString;
    EcmaString *secondStr = *secondString;
    auto range = stringTable_[GetTableId(hashcode)].table_.equal_range(hashcode);
    for (auto item = range.first; item != range.second;) {
        auto foundString = (item++)->second;
        if (EcmaStringAccessor(foundString).EqualToSplicedString(firstStr, secondStr)) {
            return foundString;
        }
    }
    return nullptr;
}

// This should only call in Debugger Signal, and need to fix and remove
EcmaString *EcmaStringTable::GetStringThreadUnsafe(const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress,
                                                   uint32_t hashcode) const
{
    auto range = stringTable_[GetTableId(hashcode)].table_.equal_range(hashcode);
    for (auto item = range.first; item != range.second;) {
        auto foundString = (item++)->second;
        if (EcmaStringAccessor::StringIsEqualUint8Data(foundString, utf8Data, utf8Len, canBeCompress)) {
            return foundString;
        }
    }
    return nullptr;
}

// This should only call in JIT Thread, and need to fix and remove
EcmaString *EcmaStringTable::GetStringThreadUnsafe(const uint16_t *utf16Data, uint32_t utf16Len,
                                                   uint32_t hashcode) const
{
    auto range = stringTable_[GetTableId(hashcode)].table_.equal_range(hashcode);
    for (auto item = range.first; item != range.second;) {
        auto foundString = (item++)->second;
        if (EcmaStringAccessor::StringsAreEqualUtf16(foundString, utf16Data, utf16Len)) {
            return foundString;
        }
    }
    return nullptr;
}
#endif
}  // namespace panda::ecmascript
