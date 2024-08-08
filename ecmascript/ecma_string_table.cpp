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

#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/runtime_lock.h"
namespace panda::ecmascript {
uint32_t EcmaStringTableCleaner::PostSweepWeakRefTask(const WeakRootVisitor &visitor)
{
    StartSweepWeakRefTask();
    uint32_t curSweepCount = sweepWeakRefCount_.load(std::memory_order_relaxed);
    GCWorkerPool::GetCurrentTaskpool()->PostTask(std::make_unique<SweepWeakRefTask>(this, visitor, curSweepCount));
    return curSweepCount;
}

void EcmaStringTableCleaner::TakeOrWaitSweepWeakRefTask(const WeakRootVisitor &visitor, uint32_t curSweepCount)
{
    if (CheckAndSwitchRunningState(curSweepCount)) {
        stringTable_->SweepWeakRef(visitor);
        SwitchFinishState(curSweepCount);
    } else {
        WaitSweepWeakRefTask();
    }
}

void EcmaStringTableCleaner::StartSweepWeakRefTask()
{
    // No need lock here, only the daemon thread will reset the state.
    sweepWeakRefFinished_ = false;
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
    if (cleaner_->CheckAndSwitchRunningState(curSweepCount_)) {
        cleaner_->stringTable_->SweepWeakRef(visitor_);
        cleaner_->SwitchFinishState(curSweepCount_);
        cleaner_->SignalSweepWeakRefTask();
    }
    return true;
}

std::pair<EcmaString *, uint32_t> EcmaStringTable::GetStringThreadUnsafe(const JSHandle<EcmaString> &firstString,
                                                                         const JSHandle<EcmaString> &secondString) const
{
    ASSERT(EcmaStringAccessor(firstString).NotTreeString());
    ASSERT(EcmaStringAccessor(secondString).NotTreeString());
    auto [hashCode, isInteger] = EcmaStringAccessor(firstString).ComputeRawHashcode();
    hashCode = EcmaStringAccessor(secondString).ComputeHashcode(hashCode, isInteger);

    auto range = table_.equal_range(hashCode);
    for (auto item = range.first; item != range.second; ++item) {
        auto foundString = item->second;
        if (EcmaStringAccessor(foundString).EqualToSplicedString(*firstString, *secondString)) {
            return std::make_pair(foundString, hashCode);
        }
    }
    return std::make_pair(nullptr, hashCode);
}

std::pair<EcmaString *, uint32_t> EcmaStringTable::GetStringThreadUnsafe(const uint8_t *utf8Data, uint32_t utf8Len,
                                                                         bool canBeCompress) const
{
    uint32_t hashCode = EcmaStringAccessor::ComputeHashcodeUtf8(utf8Data, utf8Len, canBeCompress);
    auto range = table_.equal_range(hashCode);
    for (auto item = range.first; item != range.second; ++item) {
        auto foundString = item->second;
        if (EcmaStringAccessor::StringIsEqualUint8Data(foundString, utf8Data, utf8Len, canBeCompress)) {
            return std::make_pair(foundString, hashCode);
        }
    }
    return std::make_pair(nullptr, hashCode);
}

std::pair<EcmaString *, uint32_t> EcmaStringTable::GetStringThreadUnsafe(const uint16_t *utf16Data,
                                                                         uint32_t utf16Len) const
{
    uint32_t hashCode = EcmaStringAccessor::ComputeHashcodeUtf16(const_cast<uint16_t *>(utf16Data), utf16Len);
    auto range = table_.equal_range(hashCode);
    for (auto item = range.first; item != range.second; ++item) {
        auto foundString = item->second;
        if (EcmaStringAccessor::StringsAreEqualUtf16(foundString, utf16Data, utf16Len)) {
            return std::make_pair(foundString, hashCode);
        }
    }
    return std::make_pair(nullptr, hashCode);
}

EcmaString *EcmaStringTable::GetStringWithHashThreadUnsafe(EcmaString *string, uint32_t hashcode) const
{
    auto range = table_.equal_range(hashcode);
    for (auto item = range.first; item != range.second; ++item) {
        auto foundString = item->second;
        if (EcmaStringAccessor::StringsAreEqual(foundString, string)) {
            return foundString;
        }
    }
    return nullptr;
}

EcmaString *EcmaStringTable::GetStringThreadUnsafe(EcmaString *string) const
{
    auto hashcode = EcmaStringAccessor(string).GetHashcode();
    auto range = table_.equal_range(hashcode);
    for (auto item = range.first; item != range.second; ++item) {
        auto foundString = item->second;
        if (EcmaStringAccessor::StringsAreEqual(foundString, string)) {
            return foundString;
        }
    }
    return nullptr;
}

void EcmaStringTable::InternStringThreadUnsafe(EcmaString *string)
{
    if (EcmaStringAccessor(string).IsInternString()) {
        return;
    }
    // Strings in string table should not be in the young space.
    ASSERT(Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(string))->InSharedHeap());
    ASSERT(EcmaStringAccessor(string).NotTreeString());
    auto hashcode = EcmaStringAccessor(string).GetHashcode();
    table_.emplace(hashcode, string);
    EcmaStringAccessor(string).SetInternString();
}

void EcmaStringTable::InternEmptyString(JSThread *thread, EcmaString *emptyStr)
{
    RuntimeLockHolder locker(thread, mutex_);
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    auto vm = thread->GetEcmaVM();
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    InternStringThreadUnsafe(emptyStr);
}

EcmaString *EcmaStringTable::GetOrInternString(EcmaVM *vm, const JSHandle<EcmaString> &firstString,
                                               const JSHandle<EcmaString> &secondString)
{
    if (vm->GetJsDebuggerManager()->GetSignalState()) {
        JSThread *thread = vm->GetJSThreadNoCheck();
        return GetOrInternStringWithoutLock(thread, firstString, secondString);
    } else {
        RuntimeLockHolder locker(vm->GetJSThread(), mutex_);
        JSThread *thread = vm->GetJSThread();
        return GetOrInternStringWithoutLock(thread, firstString, secondString);
    }
}

EcmaString *EcmaStringTable::GetOrInternStringWithoutLock(JSThread *thread, const JSHandle<EcmaString> &firstString,
                                                          const JSHandle<EcmaString> &secondString)
{
    EcmaVM *vm = thread->GetEcmaVM();
    auto firstFlat = JSHandle<EcmaString>(thread, EcmaStringAccessor::Flatten(vm, firstString));
    auto secondFlat = JSHandle<EcmaString>(thread, EcmaStringAccessor::Flatten(vm, secondString));
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    std::pair<EcmaString *, uint32_t> result = GetStringThreadUnsafe(firstFlat, secondFlat);
    if (result.first != nullptr) {
        return result.first;
    }
    JSHandle<EcmaString> concatHandle(thread,
        EcmaStringAccessor::Concat(vm, firstFlat, secondFlat, MemSpaceType::SHARED_OLD_SPACE));
    EcmaString *concatString = EcmaStringAccessor::Flatten(vm, concatHandle, MemSpaceType::SHARED_OLD_SPACE);
    concatString->SetMixHashcode(result.second);
    InternStringThreadUnsafe(concatString);
    return concatString;
}

EcmaString *EcmaStringTable::GetOrInternString(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len,
                                               bool canBeCompress)
{
    if (vm->GetJsDebuggerManager()->GetSignalState()) {
        return GetOrInternStringWithoutLock(vm, utf8Data, utf8Len, canBeCompress);
    } else {
        RuntimeLockHolder locker(vm->GetJSThread(), mutex_);
        return GetOrInternStringWithoutLock(vm, utf8Data, utf8Len, canBeCompress);
    }
}

EcmaString *EcmaStringTable::GetOrInternStringWithoutLock(EcmaVM *vm, const uint8_t *utf8Data,
                                                          uint32_t utf8Len, bool canBeCompress)
{
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    std::pair<EcmaString *, uint32_t> result = GetStringThreadUnsafe(utf8Data, utf8Len, canBeCompress);
    if (result.first != nullptr) {
        return result.first;
    }

    EcmaString *str =
        EcmaStringAccessor::CreateFromUtf8(vm, utf8Data, utf8Len, canBeCompress, MemSpaceType::SHARED_OLD_SPACE);
    str->SetMixHashcode(result.second);
    InternStringThreadUnsafe(str);
    return str;
}

EcmaString *EcmaStringTable::GetOrInternCompressedSubString(EcmaVM *vm, const JSHandle<EcmaString> &string,
    uint32_t offset, uint32_t utf8Len)
{
    RuntimeLockHolder locker(vm->GetJSThread(), mutex_);
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    auto *utf8Data = EcmaStringAccessor(string).GetDataUtf8() + offset;
    std::pair<EcmaString *, uint32_t> result = GetStringThreadUnsafe(utf8Data, utf8Len, true);
    if (result.first != nullptr) {
        return result.first;
    }

    EcmaString *str = EcmaStringAccessor::CreateFromUtf8CompressedSubString(
        vm, string, offset, utf8Len);
    str->SetMixHashcode(result.second);
    InternStringThreadUnsafe(str);
    return str;
}

/*
    This function is used to create global constant strings from non-movable sapce only.
    It only inserts string into string-table and provides no string-table validity check.
*/
EcmaString *EcmaStringTable::CreateAndInternStringNonMovable(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len)
{
    RuntimeLockHolder locker(vm->GetJSThread(), mutex_);
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    std::pair<EcmaString *, uint32_t> result = GetStringThreadUnsafe(utf8Data, utf8Len, true);
    if (result.first != nullptr) {
        return result.first;
    }
    EcmaString *str = EcmaStringAccessor::CreateFromUtf8(vm, utf8Data, utf8Len, true, MemSpaceType::SHARED_NON_MOVABLE);
    str->SetMixHashcode(result.second);
    InternStringThreadUnsafe(str);
    return str;
}

/*
    This function is used to create global constant strings from read-only sapce only.
    It only inserts string into string-table and provides no string-table validity check.
*/
EcmaString *EcmaStringTable::CreateAndInternStringReadOnly(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len,
                                                           bool canBeCompress)
{
    RuntimeLockHolder locker(vm->GetJSThread(), mutex_);
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    std::pair<EcmaString *, uint32_t> result = GetStringThreadUnsafe(utf8Data, utf8Len, canBeCompress);
    if (result.first != nullptr) {
        return result.first;
    }
    EcmaString *str = EcmaStringAccessor::CreateFromUtf8(vm, utf8Data, utf8Len, canBeCompress,
                                                         MemSpaceType::SHARED_READ_ONLY_SPACE);
    str->SetMixHashcode(result.second);
    InternStringThreadUnsafe(str);
    return str;
}

EcmaString *EcmaStringTable::GetOrInternString(EcmaVM *vm, const uint16_t *utf16Data, uint32_t utf16Len,
                                               bool canBeCompress)
{
    RuntimeLockHolder locker(vm->GetJSThread(), mutex_);
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    std::pair<EcmaString *, uint32_t> result = GetStringThreadUnsafe(utf16Data, utf16Len);
    if (result.first != nullptr) {
        return result.first;
    }

    EcmaString *str =
        EcmaStringAccessor::CreateFromUtf16(vm, utf16Data, utf16Len, canBeCompress, MemSpaceType::SHARED_OLD_SPACE);
    str->SetMixHashcode(result.second);
    InternStringThreadUnsafe(str);
    return str;
}

EcmaString *EcmaStringTable::GetOrInternString(EcmaVM *vm, EcmaString *string)
{
    if (EcmaStringAccessor(string).IsInternString()) {
        return string;
    }
    auto thread = vm->GetJSThread();
    JSHandle<EcmaString> strHandle(thread, string);
    // may gc
    auto strFlat = EcmaStringAccessor::Flatten(vm, strHandle, MemSpaceType::SHARED_OLD_SPACE);
    if (EcmaStringAccessor(strFlat).IsInternString()) {
        return strFlat;
    }
    JSHandle<EcmaString> strFlatHandle(thread, strFlat);
    // may gc
    RuntimeLockHolder locker(thread, mutex_);
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    strFlat = *strFlatHandle;
    EcmaString *result = GetStringThreadUnsafe(strFlat);
    if (result != nullptr) {
        return result;
    }

    InternStringThreadUnsafe(strFlat);
    return strFlat;
}

EcmaString *EcmaStringTable::GetOrInternStringThreadUnsafe(EcmaVM *vm, EcmaString *string)
{
    if (EcmaStringAccessor(string).IsInternString()) {
        return string;
    }
    JSHandle<EcmaString> strHandle(vm->GetJSThread(), string);
    EcmaString *strFlat = EcmaStringAccessor::Flatten(vm, strHandle, MemSpaceType::SHARED_OLD_SPACE);
    if (EcmaStringAccessor(strFlat).IsInternString()) {
        return strFlat;
    }
    EcmaString *result = GetStringThreadUnsafe(strFlat);
    if (result != nullptr) {
        return result;
    }

    InternStringThreadUnsafe(strFlat);
    return strFlat;
}

void EcmaStringTable::InsertStringToTableWithHashThreadUnsafe(EcmaString* string, uint32_t hashcode)
{
    // Strings in string table should not be in the young space.
    ASSERT(Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(string))->InSharedHeap());
    ASSERT(EcmaStringAccessor(string).NotTreeString());
    ASSERT(EcmaStringAccessor(string).GetHashcode() == hashcode);
    table_.emplace(hashcode, string);
    EcmaStringAccessor(string).SetInternString();
}

EcmaString *EcmaStringTable::InsertStringToTable(EcmaVM *vm, const JSHandle<EcmaString> &strHandle)
{
    RuntimeLockHolder locker(vm->GetJSThread(), mutex_);
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    auto strFlat = EcmaStringAccessor::Flatten(vm, strHandle, MemSpaceType::SHARED_OLD_SPACE);
    InternStringThreadUnsafe(strFlat);
    return strFlat;
}

EcmaString *EcmaStringTable::TryGetInternString(JSThread *thread, const JSHandle<EcmaString> &string)
{
    RuntimeLockHolder locker(thread, mutex_);
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    auto vm = thread->GetEcmaVM();
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    return GetStringThreadUnsafe(*string);
}

EcmaString *EcmaStringTable::GetOrInternStringWithSpaceType(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len,
                                                            bool canBeCompress, MemSpaceType type,
                                                            bool isConstantString, uint32_t idOffset)
{
    ASSERT(IsSMemSpace(type));
    RuntimeLockHolder locker(vm->GetJSThread(), mutex_);
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    std::pair<EcmaString *, uint32_t> result = GetStringThreadUnsafe(utf8Data, utf8Len, canBeCompress);
    if (result.first != nullptr) {
        return result.first;
    }
    type = (type == MemSpaceType::SHARED_NON_MOVABLE) ? type : MemSpaceType::SHARED_OLD_SPACE;
    EcmaString *str = nullptr;
    if (canBeCompress) {
        // Constant string will be created in this branch.
        str = EcmaStringAccessor::CreateFromUtf8(vm, utf8Data, utf8Len, canBeCompress, type, isConstantString,
            idOffset);
    } else {
        str = EcmaStringAccessor::CreateFromUtf8(vm, utf8Data, utf8Len, canBeCompress, type);
    }
    str->SetMixHashcode(result.second);
    InternStringThreadUnsafe(str);
    return str;
}

EcmaString *EcmaStringTable::GetOrInternStringWithSpaceType(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf16Len,
                                                            MemSpaceType type)
{
    ASSERT(IsSMemSpace(type));
    type = (type == MemSpaceType::SHARED_NON_MOVABLE) ? type : MemSpaceType::SHARED_OLD_SPACE;
    RuntimeLockHolder locker(vm->GetJSThread(), mutex_);
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    EcmaString *str = EcmaStringAccessor::CreateUtf16StringFromUtf8(vm, utf8Data, utf16Len, type);
    EcmaString *result = GetStringThreadUnsafe(str);
    if (result != nullptr) {
        return result;
    }
    InternStringThreadUnsafe(str);
    return str;
}

void EcmaStringTable::SweepWeakRef(const WeakRootVisitor &visitor)
{
    // No need lock here, only shared gc will sweep string table, meanwhile other threads are suspended.
    for (auto it = table_.begin(); it != table_.end();) {
        auto *object = it->second;
        auto fwd = visitor(object);
        ASSERT(Region::ObjectAddressToRange(object)->InSharedHeap());
        if (fwd == nullptr) {
            LOG_ECMA(VERBOSE) << "StringTable: delete string " << std::hex << object;
            it = table_.erase(it);
        } else if (fwd != object) {
            it->second = static_cast<EcmaString *>(fwd);
            ++it;
            LOG_ECMA(VERBOSE) << "StringTable: forward " << std::hex << object << " -> " << fwd;
        } else {
            ++it;
        }
    }
}

void EcmaStringTable::RelocateConstantData(EcmaVM *vm, const JSPandaFile *jsPandaFile)
{
    RuntimeLockHolder locker(vm->GetJSThread(), mutex_);
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    auto thread = vm->GetJSThread();
    for (auto it = table_.begin(); it != table_.end();) {
        auto *object = it->second;
        if (!EcmaStringAccessor(object).IsConstantString()) {
            ++it;
            continue;
        }
        auto constantStr = ConstantString::Cast(object);
        if (constantStr->GetEntityId() < 0 || !jsPandaFile->Contain(constantStr->GetConstantData())) {
            // EntityId is -1, which means this str has been relocated. Or the data is not in pandafile.
            ++it;
            continue;
        }
        uint32_t id = constantStr->GetEntityIdU32();
        panda_file::File::StringData sd = jsPandaFile->GetStringData(EntityId(id));
        if (constantStr->GetConstantData() == sd.data) {
            uint32_t strLen = sd.utf16_length;
            if (UNLIKELY(strLen == 0)) {
                it->second = *(vm->GetFactory()->GetEmptyString());
            }
            size_t byteLength = sd.is_ascii ? 1 : sizeof(uint16_t);
            JSMutableHandle<ByteArray> newData(vm->GetJSThread(), JSTaggedValue::Undefined());
            newData.Update(vm->GetFactory()->NewByteArray(
                strLen, byteLength, reinterpret_cast<void *>(const_cast<uint8_t *>(sd.data)),
                MemSpaceType::SHARED_NON_MOVABLE));
            constantStr->SetRelocatedData(thread, newData.GetTaggedValue());
            constantStr->SetConstantData(static_cast<uint8_t *>(newData->GetData()));
            constantStr->SetEntityId(-1);
        } else {
            LOG_ECMA(ERROR) << "ConstantString data pointer is inconsistent with sd.data";
        }
        ++it;
    }
}

bool EcmaStringTable::CheckStringTableValidity(JSThread *thread)
{
    RuntimeLockHolder locker(thread, mutex_);
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    auto vm = thread->GetEcmaVM();
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    for (auto itemOuter = table_.begin(); itemOuter != table_.end(); ++itemOuter) {
        auto outerString = itemOuter->second;
        if (!EcmaStringAccessor(outerString).NotTreeString()) {
            return false;
        }
        int counter = 0;
        auto hashcode = EcmaStringAccessor(outerString).GetHashcode();
        auto range = table_.equal_range(hashcode);
        auto it = range.first;
        for (; it != range.second; ++it) {
            auto foundString = it->second;
            if (EcmaStringAccessor::StringsAreEqual(foundString, outerString)) {
                ++counter;
            }
        }
        if (counter > 1) {
            return false;
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
}  // namespace panda::ecmascript
