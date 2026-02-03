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
#include "ecmascript/string/hashtriemap-inl.h"

namespace panda::ecmascript {
#if ENABLE_NEXT_OPTIMIZATION
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

void EcmaStringTableCleaner::WaitSweepWeakRefTaskFinished()
{
    WaitSweepWeakRefTask();
}

void EcmaStringTableCleaner::PostConcurrentSweepWeakRefTask(const WeakRootVisitor &visitor)
{
    StartSweepWeakRefTask();
    reinterpret_cast<typename DisableCMCGCConcurrentSweepTrait::HashTrieMapType *>(
        stringTable_->GetHashTrieMap())->StartSweeping();
    iter_ = std::make_shared<std::atomic<uint32_t>>(0U);
    const uint32_t postTaskCount = common::Taskpool::GetCurrentTaskpool()->GetTotalThreadNum();
    for (uint32_t i = 0U; i < postTaskCount; ++i) {
        common::Taskpool::GetCurrentTaskpool()->PostTask(
            std::make_unique<ConcurrentSweepWeakRefTask>(iter_, this, visitor));
    }
}

void EcmaStringTableCleaner::WaitConcurrentSweepWeakRefTaskAndSuspend(JSThread *thread)
{
    WaitConcurrentSweepWeakRefTask();
    SuspendAllAndFinishSweeping(thread);
    SignalSweepWeakRefTaskFinish();
    iter_.reset();
}

void EcmaStringTableCleaner::WaitConcurrentSweepWeakRefTaskAndSuspendByDaemonThread(DaemonThread *dThread)
{
    WaitConcurrentSweepWeakRefTask();
    SuspendAllAndFinishSweepingByDaemonThread(dThread);
    SignalSweepWeakRefTaskFinish();
    iter_.reset();
}

void EcmaStringTableCleaner::ProcessSweepWeakRef(IteratorPtr &iter, EcmaStringTableCleaner *cleaner,
                                                 const WeakRootVisitor &visitor)
{
    uint32_t index = 0U;
    while ((index = GetNextIndexId(iter)) < TrieMapConfig::ROOT_SIZE) {
        cleaner->stringTable_->SweepWeakRef(visitor, index);
        if (ReduceCountAndCheckFinish(cleaner)) {
            cleaner->SignalSweepWeakRefTaskFinish();
        }
    }
}

void EcmaStringTableCleaner::ProcessConcurrentSweepWeakRef(IteratorPtr &iter, EcmaStringTableCleaner *cleaner,
                                                           const WeakRootVisitor &visitor)
{
    uint32_t index = 0U;
    while ((index = GetNextIndexId(iter)) < TrieMapConfig::ROOT_SIZE) {
        std::vector<HashTrieMapEntry*>& waitFreeEntries = cleaner->waitFreeEntries_[index];
        for (const HashTrieMapEntry* entry : waitFreeEntries) {
            delete entry;
        }
        waitFreeEntries.clear();
        cleaner->stringTable_->ConcurrentSweepWeakRef(visitor, index, waitFreeEntries);
        if (ReduceCountAndCheckFinish(cleaner)) {
            cleaner->SignalSweepWeakRefTaskPending();
        }
    }
}

void EcmaStringTableCleaner::StartSweepWeakRefTask()
{
    // No need lock here, only the daemon thread will reset the state.
    sweepWeakRefFinished_ = SweepState::SWEEPING;
    PendingTaskCount_.store(TrieMapConfig::ROOT_SIZE, std::memory_order_relaxed);
}

void EcmaStringTableCleaner::WaitSweepWeakRefTask()
{
    LockHolder holder(sweepWeakRefMutex_);
    while (sweepWeakRefFinished_ != SweepState::FINISHED) {
        sweepWeakRefCV_.Wait(&sweepWeakRefMutex_);
    }
}

void EcmaStringTableCleaner::WaitConcurrentSweepWeakRefTask()
{
    LockHolder holder(sweepWeakRefMutex_);
    while (sweepWeakRefFinished_ == SweepState::SWEEPING) {
        sweepWeakRefCV_.Wait(&sweepWeakRefMutex_);
    }
}

void EcmaStringTableCleaner::SignalSweepWeakRefTaskFinish()
{
    LockHolder holder(sweepWeakRefMutex_);
    sweepWeakRefFinished_ = SweepState::FINISHED;
    sweepWeakRefCV_.SignalAll();
}

void EcmaStringTableCleaner::SignalSweepWeakRefTaskPending()
{
    LockHolder holder(sweepWeakRefMutex_);
    sweepWeakRefFinished_ = SweepState::PENDING;
    sweepWeakRefCV_.SignalAll();
}

void EcmaStringTableCleaner::SuspendAllAndFinishSweeping(JSThread *thread)
{
    SuspendAllScope scope(thread);
    typename DisableCMCGCConcurrentSweepTrait::HashTrieMapType *hashTrieMap =
        reinterpret_cast<typename DisableCMCGCConcurrentSweepTrait::HashTrieMapType *>(stringTable_->GetHashTrieMap());
    hashTrieMap->ClearToSpaceTagForFreshEntries();
    hashTrieMap->CleanUp();
    hashTrieMap->FinishSweeping();
}

void EcmaStringTableCleaner::SuspendAllAndFinishSweepingByDaemonThread(DaemonThread *dThread)
{
    ThreadManagedScope runningScope(dThread);
    SuspendAllScope scope(dThread);
    typename DisableCMCGCConcurrentSweepTrait::HashTrieMapType *hashTrieMap =
        reinterpret_cast<typename DisableCMCGCConcurrentSweepTrait::HashTrieMapType *>(stringTable_->GetHashTrieMap());
    hashTrieMap->ClearToSpaceTagForFreshEntries();
    hashTrieMap->CleanUp();
    hashTrieMap->FinishSweeping();
}

bool EcmaStringTableCleaner::SweepWeakRefTask::Run([[maybe_unused]] uint32_t threadIndex)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "EcmaStringTableCleaner::ProcessSweepWeakRef", "");
    ProcessSweepWeakRef(iter_, cleaner_, visitor_);
    return true;
}

bool EcmaStringTableCleaner::ConcurrentSweepWeakRefTask::Run([[maybe_unused]] uint32_t threadIndex)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK,
        "EcmaStringTableCleaner::ProcessConcurrentSweepWeakRef", "");
    ProcessConcurrentSweepWeakRef(iter_, cleaner_, visitor_);
    return true;
}

template <typename Traits>
EcmaString* EcmaStringTableImpl::GetOrInternFlattenString(EcmaVM* vm, EcmaString* string)
{
    typename Traits::HashTrieMapOperationType hashTrieMapOperation(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    typename Traits::HashTrieMapInUseScopeType mapInUse(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    ASSERT(EcmaStringAccessor(string).NotTreeString());
    if (EcmaStringAccessor(string).IsInternString()) {
        return string;
    }
    JSThread *thread = vm->GetAssociatedJSThread();
    uint32_t hashcode = EcmaStringAccessor(string).GetHashcode(thread);
    ASSERT(!EcmaStringAccessor(string).IsInternString());
    ASSERT(EcmaStringAccessor(string).NotTreeString());
    // Strings in string table should not be in the young space.
    if constexpr (Traits::EnableCMCGC) {
        ASSERT(string->IsInSharedHeap());
    } else {
        ASSERT(Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(string))->InSharedHeap());
    }
    auto readBarrierForLoad = [thread](const void *obj, size_t offset) -> TaggedObject* {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    auto loadResult = hashTrieMapOperation.template Load(std::move(readBarrierForLoad), hashcode,
                                                         string->ToBaseString());
    if (loadResult.value != nullptr) {
        return EcmaString::FromBaseString(loadResult.value);
    }
    JSHandle<EcmaString> stringHandle(thread, string);
    typename Traits::ThreadType* holder = GetThreadHolder<Traits>(thread);
    auto readBarrierForStoreOrLoad = [thread](const void *obj, size_t offset) -> TaggedObject* {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    BaseString *result = hashTrieMapOperation.template StoreOrLoad<
        true, decltype(readBarrierForStoreOrLoad), common::ReadOnlyHandle<BaseString>>(
        holder, std::move(readBarrierForStoreOrLoad), hashcode, loadResult, stringHandle);
    ASSERT(result != nullptr);
    return EcmaString::FromBaseString(result);
}

template <typename Traits>
EcmaString* EcmaStringTableImpl::GetOrInternFlattenStringNoGC(EcmaVM* vm, EcmaString* string)
{
    typename Traits::HashTrieMapOperationType hashTrieMapOperation(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    typename Traits::HashTrieMapInUseScopeType mapInUse(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    ASSERT(EcmaStringAccessor(string).NotTreeString());
    if (EcmaStringAccessor(string).IsInternString()) {
        return string;
    }
    JSThread *thread = vm->GetAssociatedJSThread();
    uint32_t hashcode = EcmaStringAccessor(string).GetHashcode(thread);
    ASSERT(!EcmaStringAccessor(string).IsInternString());
    ASSERT(EcmaStringAccessor(string).NotTreeString());
    // Strings in string table should not be in the young space.
    if constexpr (Traits::EnableCMCGC) {
        ASSERT(string->IsInSharedHeap());
    } else {
        ASSERT(Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(string))->InSharedHeap());
    }
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject* {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    auto loadResult = hashTrieMapOperation.template Load(readBarrier, hashcode, string->ToBaseString());
    if (loadResult.value != nullptr) {
        return EcmaString::FromBaseString(loadResult.value);
    }
    typename Traits::ThreadType* holder = GetThreadHolder<Traits>(vm->GetJSThread());
    JSHandle<EcmaString> stringHandle(thread, string);
    BaseString* result = hashTrieMapOperation.template StoreOrLoad<
        false, decltype(readBarrier), common::ReadOnlyHandle<BaseString>>(
        holder, std::move(readBarrier), hashcode, loadResult, stringHandle);
    ASSERT(result != nullptr);
    return EcmaString::FromBaseString(result);
}

template <typename Traits>
EcmaString* EcmaStringTableImpl::GetOrInternStringFromCompressedSubString(
    EcmaVM* vm, const JSHandle<EcmaString>& string, uint32_t offset, uint32_t utf8Len)
{
    typename Traits::HashTrieMapOperationType hashTrieMapOperation(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    typename Traits::HashTrieMapInUseScopeType mapInUse(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    const uint8_t* utf8Data = EcmaStringAccessor(string).GetDataUtf8() + offset;
    uint32_t hashcode = EcmaStringAccessor::ComputeHashcodeUtf8(utf8Data, utf8Len, true);
    JSThread *thread = vm->GetAssociatedJSThread();
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject* {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    auto loadResult = hashTrieMapOperation.template Load(std::move(readBarrier), hashcode, string, offset, utf8Len);
    if (loadResult.value != nullptr) {
        return EcmaString::FromBaseString(loadResult.value);
    }
    typename Traits::ThreadType* holder = GetThreadHolder<Traits>(vm->GetJSThread());
    BaseString *result = hashTrieMapOperation.template StoreOrLoad(
        holder, hashcode, loadResult,
        [vm, string, offset, utf8Len, hashcode]() -> common::ReadOnlyHandle<BaseString> {
            EcmaString* str = EcmaStringAccessor::CreateFromUtf8CompressedSubString(vm, string, offset, utf8Len,
                MemSpaceType::SHARED_OLD_SPACE);
            str->SetMixHashcode(hashcode);
            ASSERT(!EcmaStringAccessor(str).IsInternString());
            ASSERT(EcmaStringAccessor(str).NotTreeString());
            // Strings in string table should not be in the young space.
            if constexpr (Traits::EnableCMCGC) {
                ASSERT(str->IsInSharedHeap());
            } else {
                ASSERT(Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(str))->InSharedHeap());
            }
            JSThread *thread = vm->GetJSThread();
            JSHandle<EcmaString> strHandle(thread, str);
            return strHandle;
        },
        [vm, utf8Len, string, offset](BaseString *foundString) {
            JSThread *thread = vm->GetAssociatedJSThread();
            const uint8_t *utf8Data = EcmaStringAccessor(string).GetDataUtf8() + offset;
            return EcmaStringAccessor::StringIsEqualUint8Data(thread, EcmaString::FromBaseString(foundString), utf8Data,
                                                              utf8Len, true);
        });
    ASSERT(result != nullptr);
    return EcmaString::FromBaseString(result);
}

template <typename Traits>
EcmaString *EcmaStringTableImpl::GetOrInternString(EcmaVM *vm, EcmaString *string)
{
    typename Traits::HashTrieMapOperationType hashTrieMapOperation(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    typename Traits::HashTrieMapInUseScopeType mapInUse(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
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
    uint32_t hashcode = EcmaStringAccessor(strFlat).GetHashcode(thread);
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject* {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    auto loadResult = hashTrieMapOperation.template Load(readBarrier, hashcode, strFlat->ToBaseString());
    if (loadResult.value != nullptr) {
        return EcmaString::FromBaseString(loadResult.value);
    }
    JSHandle<EcmaString> strFlatHandle(thread, strFlat);
    typename Traits::ThreadType* holder = GetThreadHolder<Traits>(vm->GetJSThread());
    BaseString* result = hashTrieMapOperation.template StoreOrLoad<
        true, decltype(readBarrier), common::ReadOnlyHandle<BaseString>>(
        holder, std::move(readBarrier), hashcode, loadResult, strFlatHandle);
    ASSERT(result != nullptr);
    return EcmaString::FromBaseString(result);
}

template <typename Traits>
EcmaString* EcmaStringTableImpl::GetOrInternString(EcmaVM* vm, const JSHandle<EcmaString>& firstString,
                                                   const JSHandle<EcmaString>& secondString)
{
    typename Traits::HashTrieMapOperationType hashTrieMapOperation(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    typename Traits::HashTrieMapInUseScopeType mapInUse(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    bool signalState = vm->GetJsDebuggerManager()->GetSignalState();
    if (UNLIKELY(signalState)) {
        return GetOrInternStringThreadUnsafe<Traits>(vm, firstString, secondString);
    }
    JSThread *thread = vm->GetJSThread();
    JSHandle<EcmaString> firstFlat(thread, EcmaStringAccessor::Flatten(vm, firstString));
    JSHandle<EcmaString> secondFlat(thread, EcmaStringAccessor::Flatten(vm, secondString));
    uint32_t hashcode = EcmaStringAccessor::CalculateAllConcatHashCode(thread, firstFlat, secondFlat);
    ASSERT(EcmaStringAccessor(firstFlat).NotTreeString());
    ASSERT(EcmaStringAccessor(secondFlat).NotTreeString());
    typename Traits::ThreadType* holder = GetThreadHolder<Traits>(vm->GetJSThread());
    BaseString *result = hashTrieMapOperation.template LoadOrStore<true>(
        holder, hashcode,
        [vm, hashcode, thread, firstFlat, secondFlat]() {
            JSHandle<EcmaString> concatHandle(
                thread, EcmaStringAccessor::Concat(vm, firstFlat, secondFlat, MemSpaceType::SHARED_OLD_SPACE));
            EcmaString *value = EcmaStringAccessor::Flatten(vm, concatHandle, MemSpaceType::SHARED_OLD_SPACE);
            value->SetMixHashcode(hashcode);
            ASSERT(!EcmaStringAccessor(value).IsInternString());
            ASSERT(EcmaStringAccessor(value).NotTreeString());
            // Strings in string table should not be in the young space.
            if constexpr (Traits::EnableCMCGC) {
                ASSERT(value->IsInSharedHeap());
            } else {
                ASSERT(Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(value))->InSharedHeap());
            }
            JSHandle<EcmaString> stringHandle(thread, value);
            return stringHandle;
        },
        [thread, firstFlat, secondFlat](BaseString *foundString) {
            EcmaString *firstStr = *firstFlat;
            EcmaString *secondStr = *secondFlat;
            return EcmaStringAccessor(EcmaString::FromBaseString(foundString))
                .EqualToSplicedString(thread, firstStr, secondStr);
        });
    ASSERT(result != nullptr);
    return EcmaString::FromBaseString(result);
}

template <typename Traits>
EcmaString* EcmaStringTableImpl::GetOrInternString(EcmaVM* vm, const uint8_t* utf8Data, uint32_t utf8Len,
                                                   bool canBeCompress, [[maybe_unused]] MemSpaceType type)
{
    typename Traits::HashTrieMapOperationType hashTrieMapOperation(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    typename Traits::HashTrieMapInUseScopeType mapInUse(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    ASSERT(IsSMemSpace(type));
    bool signalState = vm->GetJsDebuggerManager()->GetSignalState();
    if (UNLIKELY(signalState)) {
        return GetOrInternStringThreadUnsafe<Traits>(vm, utf8Data, utf8Len, canBeCompress);
    }
    typename Traits::ThreadType* holder = GetThreadHolder<Traits>(vm->GetJSThread());
    uint32_t hashcode = EcmaStringAccessor::ComputeHashcodeUtf8(utf8Data, utf8Len, canBeCompress);
    BaseString *result = hashTrieMapOperation.template LoadOrStore<true>(
        holder, hashcode,
        [vm, hashcode, utf8Data, utf8Len, canBeCompress, type]() {
            EcmaString* value = EcmaStringAccessor::CreateFromUtf8(vm, utf8Data, utf8Len, canBeCompress, type);
            value->SetMixHashcode(hashcode);
            ASSERT(!EcmaStringAccessor(value).IsInternString());
            ASSERT(EcmaStringAccessor(value).NotTreeString());
            // Strings in string table should not be in the young space.
            if constexpr (Traits::EnableCMCGC) {
                ASSERT(value->IsInSharedHeap());
            } else {
                ASSERT(Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(value))->InSharedHeap());
            }
            JSThread *thread = vm->GetJSThread();
            JSHandle<EcmaString> stringHandle(thread, value);
            return stringHandle;
        },
        [vm, utf8Data, utf8Len, canBeCompress](BaseString *foundString) {
            JSThread *thread = vm->GetAssociatedJSThread();
            return EcmaStringAccessor::StringIsEqualUint8Data(thread, EcmaString::FromBaseString(foundString), utf8Data,
                                                              utf8Len, canBeCompress);
        });
    ASSERT(result != nullptr);
    return EcmaString::FromBaseString(result);
}

template <typename Traits>
EcmaString* EcmaStringTableImpl::GetOrInternString(EcmaVM* vm, const uint8_t* utf8Data, uint32_t utf16Len,
                                                   MemSpaceType type)
{
    typename Traits::HashTrieMapOperationType hashTrieMapOperation(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    typename Traits::HashTrieMapInUseScopeType mapInUse(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    ASSERT(IsSMemSpace(type));
    ASSERT(type == MemSpaceType::SHARED_NON_MOVABLE || type == MemSpaceType::SHARED_OLD_SPACE);
    JSThread* thread = vm->GetJSThread();
    EcmaString* str = EcmaStringAccessor::CreateUtf16StringFromUtf8(vm, utf8Data, utf16Len, type);
    uint32_t hashcode = EcmaStringAccessor(str).GetHashcode(thread);
    auto readBarrierForLoad = [thread](const void *obj, size_t offset) -> TaggedObject* {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    auto loadResult = hashTrieMapOperation.template Load(std::move(readBarrierForLoad), hashcode, str->ToBaseString());
    if (loadResult.value != nullptr) {
        return EcmaString::FromBaseString(loadResult.value);
    }
    JSHandle<EcmaString> strHandle(thread, str);
    typename Traits::ThreadType* holder = GetThreadHolder<Traits>(vm->GetJSThread());
    auto readBarrierForStoreOrLoad = [thread](const void *obj, size_t offset) -> TaggedObject* {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    BaseString* result = hashTrieMapOperation.template StoreOrLoad<
        true, decltype(readBarrierForStoreOrLoad), common::ReadOnlyHandle<BaseString>>(
        holder, std::move(readBarrierForStoreOrLoad), hashcode, loadResult, strHandle);
    ASSERT(result != nullptr);
    return EcmaString::FromBaseString(result);
}

template <typename Traits>
EcmaString* EcmaStringTableImpl::GetOrInternString(EcmaVM* vm, const uint16_t* utf16Data, uint32_t utf16Len,
                                                   bool canBeCompress)
{
    typename Traits::HashTrieMapOperationType hashTrieMapOperation(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    typename Traits::HashTrieMapInUseScopeType mapInUse(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    typename Traits::ThreadType* holder = GetThreadHolder<Traits>(vm->GetJSThread());
    uint32_t hashcode = EcmaStringAccessor::ComputeHashcodeUtf16(const_cast<uint16_t*>(utf16Data), utf16Len);
    BaseString *result = hashTrieMapOperation.template LoadOrStore<true>(
        holder, hashcode,
        [vm, utf16Data, utf16Len, canBeCompress, hashcode]() {
            EcmaString* value = EcmaStringAccessor::CreateFromUtf16(vm, utf16Data, utf16Len, canBeCompress,
                                                                    MemSpaceType::SHARED_OLD_SPACE);
            value->SetMixHashcode(hashcode);
            ASSERT(!EcmaStringAccessor(value).IsInternString());
            ASSERT(EcmaStringAccessor(value).NotTreeString());
            // Strings in string table should not be in the young space.
            if constexpr (Traits::EnableCMCGC) {
                ASSERT(value->IsInSharedHeap());
            } else {
                ASSERT(Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(value))->InSharedHeap());
            }
            JSThread *thread = vm->GetJSThread();
            JSHandle<EcmaString> stringHandle(thread, value);
            return stringHandle;
        },
        [vm, utf16Data, utf16Len](BaseString *foundString) {
            JSThread *thread = vm->GetAssociatedJSThread();
            return EcmaStringAccessor::StringsAreEqualUtf16(thread, EcmaString::FromBaseString(foundString), utf16Data,
                                                            utf16Len);
        });
    ASSERT(result != nullptr);
    return EcmaString::FromBaseString(result);
}

template <typename Traits>
EcmaString *EcmaStringTableImpl::TryGetInternString(JSThread *thread, const JSHandle<EcmaString> &string)
{
    typename Traits::HashTrieMapOperationType hashTrieMapOperation(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    typename Traits::HashTrieMapInUseScopeType mapInUse(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    uint32_t hashcode = EcmaStringAccessor(*string).GetHashcode(thread);
    EcmaString *str = *string;
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject* {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    return EcmaString::FromBaseString(
        hashTrieMapOperation.template Load<false>(std::move(readBarrier), hashcode, str->ToBaseString()));
}

// used in jit thread, which unsupport create jshandle
template <typename Traits>
EcmaString* EcmaStringTableImpl::GetOrInternStringWithoutJSHandleForJit(EcmaVM* vm, const uint8_t* utf8Data,
    uint32_t utf8Len, bool canBeCompress, MemSpaceType type)
{
    typename Traits::HashTrieMapOperationType hashTrieMapOperation(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    typename Traits::HashTrieMapInUseScopeType mapInUse(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    ASSERT(IsSMemSpace(type));
    ASSERT(type == MemSpaceType::SHARED_NON_MOVABLE || type == MemSpaceType::SHARED_OLD_SPACE);
    uint32_t hashcode = EcmaStringAccessor::ComputeHashcodeUtf8(utf8Data, utf8Len, canBeCompress);
    typename Traits::ThreadType* holder = GetThreadHolder<Traits>(vm->GetJSThread());
    BaseString *result = hashTrieMapOperation.LoadOrStoreForJit(
        holder, hashcode,
        [vm, utf8Data, utf8Len, canBeCompress, type, hashcode]() {
            EcmaString *value = EcmaStringAccessor::CreateFromUtf8(vm, utf8Data, utf8Len, canBeCompress, type);

            value->SetMixHashcode(hashcode);
            ASSERT(!EcmaStringAccessor(value).IsInternString());
            ASSERT(EcmaStringAccessor(value).NotTreeString());
            // Strings in string table should not be in the young space.
            if constexpr (Traits::EnableCMCGC) {
                ASSERT(value->IsInSharedHeap());
            } else {
                ASSERT(Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(value))->InSharedHeap());
            }
            return value->ToBaseString();
        },
        [vm, utf8Data, utf8Len, canBeCompress](BaseString *foundString) {
            JSThread *thread = vm->GetAssociatedJSThread();
            return EcmaStringAccessor::StringIsEqualUint8Data(thread, EcmaString::FromBaseString(foundString), utf8Data,
                                                              utf8Len, canBeCompress);
        });
    ASSERT(result != nullptr);
    return EcmaString::FromBaseString(result);
}

// used in jit thread, which unsupport create jshandle
template <typename Traits>
EcmaString* EcmaStringTableImpl::GetOrInternStringWithoutJSHandleForJit(EcmaVM* vm, const uint8_t* utf8Data,
    uint32_t utf16Len, MemSpaceType type)
{
    typename Traits::HashTrieMapOperationType hashTrieMapOperation(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    typename Traits::HashTrieMapInUseScopeType mapInUse(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    ASSERT(vm->GetJSThread()->IsJitThread());
    ASSERT(IsSMemSpace(type));
    type = (type == MemSpaceType::SHARED_NON_MOVABLE) ? type : MemSpaceType::SHARED_OLD_SPACE;
    CVector<uint16_t> u16Buffer(utf16Len);
    utf::ConvertRegionMUtf8ToUtf16(utf8Data, u16Buffer.data(), utf::Mutf8Size(utf8Data), utf16Len, 0);
    uint32_t hashcode = EcmaStringAccessor::ComputeHashcodeUtf16(u16Buffer.data(), utf16Len);
    const uint16_t *utf16Data = u16Buffer.data();
    typename Traits::ThreadType* holder = GetThreadHolder<Traits>(vm->GetJSThread());
    BaseString *result = hashTrieMapOperation.LoadOrStoreForJit(
        holder, hashcode,
        [vm, u16Buffer, utf16Len, hashcode, type]() {
            EcmaString *value = EcmaStringAccessor::CreateFromUtf16(vm, u16Buffer.data(), utf16Len, false, type);
            value->SetMixHashcode(hashcode);
            ASSERT(!EcmaStringAccessor(value).IsInternString());
            ASSERT(EcmaStringAccessor(value).NotTreeString());
            // Strings in string table should not be in the young space.
            if constexpr (Traits::EnableCMCGC) {
                ASSERT(value->IsInSharedHeap());
            } else {
                ASSERT(Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(value))->InSharedHeap());
            }
            return value->ToBaseString();
        },
        [vm, utf16Data, utf16Len](BaseString *foundString) {
            JSThread *thread = vm->GetAssociatedJSThread();
            return EcmaStringAccessor::StringsAreEqualUtf16(thread, EcmaString::FromBaseString(foundString), utf16Data,
                                                            utf16Len);
        });
    ASSERT(result != nullptr);
    return EcmaString::FromBaseString(result);
}

template <typename Traits, std::enable_if_t<!Traits::ConcurrentSweep, int>>
void EcmaStringTableImpl::SweepWeakRef(const WeakRootVisitor &visitor, uint32_t rootID)
{
    typename Traits::HashTrieMapOperationType hashTrieMapOperation(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    ASSERT(rootID >= 0 && rootID < TrieMapConfig::ROOT_SIZE);
    auto *root_node = reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap())->
        root_[rootID].load(std::memory_order_relaxed);
    if (root_node == nullptr) {
        return;
    }
    for (uint32_t index = 0; index < TrieMapConfig::INDIRECT_SIZE; ++index) {
        hashTrieMapOperation.ClearNodeFromGC(root_node, index, visitor);
    }
}

template <typename Traits, std::enable_if_t<Traits::ConcurrentSweep, int>>
void EcmaStringTableImpl::ConcurrentSweepWeakRef(const WeakRootVisitor &visitor, uint32_t rootID,
                                                 std::vector<HashTrieMapEntry*>& waitDeleteEntries)
{
    typename Traits::HashTrieMapOperationType hashTrieMapOperation(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    ASSERT(rootID >= 0 && rootID < TrieMapConfig::ROOT_SIZE);
    auto *root_node = reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap())->
        root_[rootID].load(std::memory_order_relaxed);
    if (root_node == nullptr) {
        return;
    }
    for (uint32_t index = 0; index < TrieMapConfig::INDIRECT_SIZE; ++index) {
        hashTrieMapOperation.ClearNodeFromGC(root_node, index, visitor, waitDeleteEntries);
    }
}

template <typename Traits>
bool EcmaStringTableImpl::CheckStringTableValidity(JSThread *thread)
{
    typename Traits::HashTrieMapOperationType hashTrieMapOperation(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    typename Traits::HashTrieMapInUseScopeType mapInUse(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    bool isValid = true;
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject* {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    hashTrieMapOperation.Range(std::move(readBarrier), isValid);
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
template <typename Traits>
EcmaString* EcmaStringTableImpl::GetOrInternStringThreadUnsafe(
    EcmaVM* vm, const JSHandle<EcmaString> firstString,
    const JSHandle<EcmaString> secondString)
{
    typename Traits::HashTrieMapOperationType hashTrieMapOperation(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    typename Traits::HashTrieMapInUseScopeType mapInUse(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    ASSERT(vm->GetJsDebuggerManager()->GetSignalState());
    JSThread *thread = vm->GetJSThreadNoCheck();
    JSHandle<EcmaString> firstFlat(thread, EcmaStringAccessor::Flatten(vm, firstString));
    JSHandle<EcmaString> secondFlat(thread, EcmaStringAccessor::Flatten(vm, secondString));
    uint32_t hashcode = EcmaStringAccessor::CalculateAllConcatHashCode(thread, firstFlat, secondFlat);
    ASSERT(EcmaStringAccessor(firstFlat).NotTreeString());
    ASSERT(EcmaStringAccessor(secondFlat).NotTreeString());
    typename Traits::ThreadType* holder = GetThreadHolder<Traits>(vm->GetJSThread());
    BaseString *result = hashTrieMapOperation.template LoadOrStore<false>(
        holder, hashcode,
        [hashcode, thread, vm, firstFlat, secondFlat]() {
            JSHandle<EcmaString> concatHandle(
                thread, EcmaStringAccessor::Concat(vm, firstFlat, secondFlat, MemSpaceType::SHARED_OLD_SPACE));
            EcmaString *concatString = EcmaStringAccessor::Flatten(vm, concatHandle, MemSpaceType::SHARED_OLD_SPACE);
            concatString->SetMixHashcode(hashcode);
            ASSERT(!EcmaStringAccessor(concatString).IsInternString());
            ASSERT(EcmaStringAccessor(concatString).NotTreeString());
            // Strings in string table should not be in the young space.
            if constexpr (Traits::EnableCMCGC) {
                ASSERT(concatString->IsInSharedHeap());
            } else {
                ASSERT(Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(concatString))->InSharedHeap());
            }
            JSHandle<EcmaString> stringHandle(thread, concatString);
            return stringHandle;
        },
        [vm, firstFlat, secondFlat](BaseString *foundString) {
            EcmaString *firstStr = *firstFlat;
            EcmaString *secondStr = *secondFlat;
            JSThread *thread = vm->GetAssociatedJSThread();
            return EcmaStringAccessor(EcmaString::FromBaseString(foundString))
                .EqualToSplicedString(thread, firstStr, secondStr);
        });
    ASSERT(result != nullptr);
    return EcmaString::FromBaseString(result);
}

// This should only call in Debugger Signal, and need to fix and remove
template <typename Traits>
EcmaString* EcmaStringTableImpl::GetOrInternStringThreadUnsafe(EcmaVM* vm, const uint8_t* utf8Data,
                                                               uint32_t utf8Len, bool canBeCompress)
{
    typename Traits::HashTrieMapOperationType hashTrieMapOperation(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    typename Traits::HashTrieMapInUseScopeType mapInUse(
        reinterpret_cast<typename Traits::HashTrieMapType *>(GetHashTrieMap()));
    ASSERT(vm->GetJsDebuggerManager()->GetSignalState());
    uint32_t hashcode = EcmaStringAccessor::ComputeHashcodeUtf8(utf8Data, utf8Len, canBeCompress);
    typename Traits::ThreadType* holder = GetThreadHolder<Traits>(vm->GetJSThread());
    BaseString *result = hashTrieMapOperation.template LoadOrStore<false>(
        holder, hashcode,
        [vm, utf8Data, utf8Len, canBeCompress, hashcode]() {
            EcmaString *value = EcmaStringAccessor::CreateFromUtf8(vm, utf8Data, utf8Len, canBeCompress,
                                                                   MemSpaceType::SHARED_OLD_SPACE);
            value->SetMixHashcode(hashcode);
            JSThread *thread = vm->GetJSThread();
            JSHandle<EcmaString> stringHandle(thread, value);
            return stringHandle;
        },
        [vm, utf8Data, utf8Len, canBeCompress](BaseString *foundString) {
            JSThread *thread = vm->GetAssociatedJSThread();
            return EcmaStringAccessor::StringIsEqualUint8Data(thread, EcmaString::FromBaseString(foundString), utf8Data,
                                                              utf8Len, canBeCompress);
        });
    ASSERT(result != nullptr);
    return EcmaString::FromBaseString(result);
}

EcmaString* EcmaStringTable::GetOrInternFlattenString(EcmaVM* vm, EcmaString* string)
{
#ifdef USE_CMC_GC
    if (enableCMCGC_) {
        return impl_.GetOrInternFlattenString<EnableCMCGCTrait>(vm, string);
    }
#endif
    if (IsSweeping()) {
        return impl_.GetOrInternFlattenString<DisableCMCGCConcurrentSweepTrait>(vm, string);
    }
    return impl_.GetOrInternFlattenString<DisableCMCGCNormalTrait>(vm, string);
}

EcmaString* EcmaStringTable::GetOrInternFlattenStringNoGC(EcmaVM* vm, EcmaString* string)
{
#ifdef USE_CMC_GC
    if (enableCMCGC_) {
        return impl_.GetOrInternFlattenStringNoGC<EnableCMCGCTrait>(vm, string);
    }
#endif
    if (IsSweeping()) {
        return impl_.GetOrInternFlattenStringNoGC<DisableCMCGCConcurrentSweepTrait>(vm, string);
    }
    return impl_.GetOrInternFlattenStringNoGC<DisableCMCGCNormalTrait>(vm, string);
}

EcmaString* EcmaStringTable::GetOrInternStringFromCompressedSubString(EcmaVM* vm, const JSHandle<EcmaString>& string,
                                                                      uint32_t offset, uint32_t utf8Len)
{
#ifdef USE_CMC_GC
    if (enableCMCGC_) {
        return impl_.GetOrInternStringFromCompressedSubString<EnableCMCGCTrait>(vm, string, offset, utf8Len);
    }
#endif
    if (IsSweeping()) {
        return impl_.GetOrInternStringFromCompressedSubString<DisableCMCGCConcurrentSweepTrait>(vm, string, offset,
                                                                                                utf8Len);
    }
    return impl_.GetOrInternStringFromCompressedSubString<DisableCMCGCNormalTrait>(vm, string, offset, utf8Len);
}

EcmaString* EcmaStringTable::GetOrInternString(EcmaVM* vm, EcmaString* string)
{
#ifdef USE_CMC_GC
    if (enableCMCGC_) {
        return impl_.GetOrInternString<EnableCMCGCTrait>(vm, string);
    }
#endif
    if (IsSweeping()) {
        return impl_.GetOrInternString<DisableCMCGCConcurrentSweepTrait>(vm, string);
    }
    return impl_.GetOrInternString<DisableCMCGCNormalTrait>(vm, string);
}

EcmaString* EcmaStringTable::GetOrInternString(EcmaVM* vm, const JSHandle<EcmaString>& firstString,
                                               const JSHandle<EcmaString>& secondString)
{
#ifdef USE_CMC_GC
    if (enableCMCGC_) {
        return impl_.GetOrInternString<EnableCMCGCTrait>(vm, firstString, secondString);
    }
#endif
    if (IsSweeping()) {
        return impl_.GetOrInternString<DisableCMCGCConcurrentSweepTrait>(vm, firstString, secondString);
    }
    return impl_.GetOrInternString<DisableCMCGCNormalTrait>(vm, firstString, secondString);
}

EcmaString* EcmaStringTable::GetOrInternString(EcmaVM* vm, const uint8_t* utf8Data, uint32_t utf8Len,
                                               bool canBeCompress,
                                               MemSpaceType type)
{
#ifdef USE_CMC_GC
    if (enableCMCGC_) {
        return impl_.GetOrInternString<EnableCMCGCTrait>(vm, utf8Data, utf8Len, canBeCompress, type);
    }
#endif
    if (IsSweeping()) {
        return impl_.GetOrInternString<DisableCMCGCConcurrentSweepTrait>(vm, utf8Data, utf8Len, canBeCompress, type);
    }
    return impl_.GetOrInternString<DisableCMCGCNormalTrait>(vm, utf8Data, utf8Len, canBeCompress, type);
}

EcmaString* EcmaStringTable::GetOrInternString(EcmaVM* vm, const uint8_t* utf8Data, uint32_t utf16Len,
                                               MemSpaceType type)
{
#ifdef USE_CMC_GC
    if (enableCMCGC_) {
        return impl_.GetOrInternString<EnableCMCGCTrait>(vm, utf8Data, utf16Len, type);
    }
#endif
    if (IsSweeping()) {
        return impl_.GetOrInternString<DisableCMCGCConcurrentSweepTrait>(vm, utf8Data, utf16Len, type);
    }
    return impl_.GetOrInternString<DisableCMCGCNormalTrait>(vm, utf8Data, utf16Len, type);
}

EcmaString* EcmaStringTable::GetOrInternString(EcmaVM* vm, const uint16_t* utf16Data, uint32_t utf16Len,
                                               bool canBeCompress)
{
#ifdef USE_CMC_GC
    if (enableCMCGC_) {
        return impl_.GetOrInternString<EnableCMCGCTrait>(vm, utf16Data, utf16Len, canBeCompress);
    }
#endif
    if (IsSweeping()) {
        return impl_.GetOrInternString<DisableCMCGCConcurrentSweepTrait>(vm, utf16Data, utf16Len, canBeCompress);
    }
    return impl_.GetOrInternString<DisableCMCGCNormalTrait>(vm, utf16Data, utf16Len, canBeCompress);
}

// This is ONLY for JIT Thread, since JIT could not create JSHandle so need to allocate String with holding
// lock_ --- need to support JSHandle
EcmaString* EcmaStringTable::GetOrInternStringWithoutJSHandleForJit(EcmaVM* vm, const uint8_t* utf8Data,
                                                                    uint32_t utf16Len,
                                                                    MemSpaceType type)
{
#ifdef USE_CMC_GC
    if (enableCMCGC_) {
        return impl_.GetOrInternStringWithoutJSHandleForJit<EnableCMCGCTrait>(vm, utf8Data, utf16Len, type);
    }
#endif
    if (IsSweeping()) {
        return impl_.GetOrInternStringWithoutJSHandleForJit<DisableCMCGCConcurrentSweepTrait>(vm, utf8Data,
                                                                                              utf16Len, type);
    }
    return impl_.GetOrInternStringWithoutJSHandleForJit<DisableCMCGCNormalTrait>(vm, utf8Data, utf16Len, type);
}

EcmaString* EcmaStringTable::GetOrInternStringWithoutJSHandleForJit(EcmaVM* vm, const uint8_t* utf8Data,
                                                                    uint32_t utf8Len,
                                                                    bool canBeCompress, MemSpaceType type)
{
#ifdef USE_CMC_GC
    if (enableCMCGC_) {
        return impl_.GetOrInternStringWithoutJSHandleForJit<EnableCMCGCTrait>(vm, utf8Data, utf8Len,
                                                                               canBeCompress, type);
    }
#endif
    if (IsSweeping()) {
        return impl_.GetOrInternStringWithoutJSHandleForJit<DisableCMCGCConcurrentSweepTrait>(vm, utf8Data, utf8Len,
                                                                                              canBeCompress, type);
    }
    return impl_.GetOrInternStringWithoutJSHandleForJit<DisableCMCGCNormalTrait>(vm, utf8Data, utf8Len,
                                                                                 canBeCompress, type);
}

EcmaString *EcmaStringTable::TryGetInternString(JSThread *thread, const JSHandle<EcmaString> &string)
{
#ifdef USE_CMC_GC
    if (enableCMCGC_) {
        return impl_.TryGetInternString<EnableCMCGCTrait>(thread, string);
    }
#endif
    if (IsSweeping()) {
        return impl_.TryGetInternString<DisableCMCGCConcurrentSweepTrait>(thread, string);
    }
    return impl_.TryGetInternString<DisableCMCGCNormalTrait>(thread, string);
}

void EcmaStringTable::SweepWeakRef(const WeakRootVisitor& visitor, uint32_t rootID)
{
#ifdef USE_CMC_GC
    if (enableCMCGC_) {
        UNREACHABLE();
    }
#endif
    impl_.SweepWeakRef<DisableCMCGCNormalTrait>(visitor, rootID);
}

void EcmaStringTable::ConcurrentSweepWeakRef(const WeakRootVisitor& visitor, uint32_t rootID,
                                             std::vector<HashTrieMapEntry*>& waitDeleteEntries)
{
#ifdef USE_CMC_GC
    if (enableCMCGC_) {
        UNREACHABLE();
    }
#endif
    impl_.ConcurrentSweepWeakRef<DisableCMCGCConcurrentSweepTrait>(visitor, rootID, waitDeleteEntries);
}

bool EcmaStringTable::CheckStringTableValidity(JSThread *thread)
{
#ifdef USE_CMC_GC
    if (enableCMCGC_) {
        return impl_.CheckStringTableValidity<EnableCMCGCTrait>(thread);
    }
#endif
    if (IsSweeping()) {
        return impl_.CheckStringTableValidity<DisableCMCGCConcurrentSweepTrait>(thread);
    }
    return impl_.CheckStringTableValidity<DisableCMCGCNormalTrait>(thread);
}


EcmaString* EcmaStringTable::GetOrInternStringThreadUnsafe(EcmaVM* vm, const JSHandle<EcmaString> firstString,
                                                           const JSHandle<EcmaString> secondString)
{
#ifdef USE_CMC_GC
    if (enableCMCGC_) {
        return impl_.GetOrInternStringThreadUnsafe<EnableCMCGCTrait>(vm, firstString, secondString);
    }
#endif
    if (IsSweeping()) {
        return impl_.GetOrInternStringThreadUnsafe<DisableCMCGCConcurrentSweepTrait>(vm, firstString, secondString);
    }
    return impl_.GetOrInternStringThreadUnsafe<DisableCMCGCNormalTrait>(vm, firstString, secondString);
}

// This should only call in Debugger Signal, and need to fix and remove
EcmaString* EcmaStringTable::GetOrInternStringThreadUnsafe(EcmaVM* vm, const uint8_t* utf8Data, uint32_t utf8Len,
                                                           bool canBeCompress)
{
#ifdef USE_CMC_GC
    if (enableCMCGC_) {
        return impl_.GetOrInternStringThreadUnsafe<EnableCMCGCTrait>(vm, utf8Data, utf8Len, canBeCompress);
    }
#endif
    if (IsSweeping()) {
        return impl_.GetOrInternStringThreadUnsafe<DisableCMCGCConcurrentSweepTrait>(vm, utf8Data, utf8Len,
                                                                                     canBeCompress);
    }
    return impl_.GetOrInternStringThreadUnsafe<DisableCMCGCNormalTrait>(vm, utf8Data, utf8Len, canBeCompress);
}

void EcmaStringTable::TransferToNativeAndWaitSweepWeakRefTaskFinished(JSThread *thread)
{
#ifdef USE_CMC_GC
    if (enableCMCGC_) {
        return;
    }
#endif
    panda::ecmascript::ThreadNativeScope scope(thread);
    cleaner_->WaitSweepWeakRefTaskFinished();
}
#endif
}  // namespace panda::ecmascript
