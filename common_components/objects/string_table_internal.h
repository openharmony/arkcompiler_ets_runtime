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

#ifndef COMMON_COMPONENTS_OBJECTS_STRING_TABLE_INTERNAL_H
#define COMMON_COMPONENTS_OBJECTS_STRING_TABLE_INTERNAL_H

#include "common_components/base/globals.h"
#include "common_components/platform/mutex.h"
#include "common_interfaces/thread/thread_holder.h"
#include "common_interfaces/objects/base_string_table.h"
#include "common_components/objects/string_table/hashtriemap.h"

namespace common {

class BaseStringTableMutex {
public:
    explicit BaseStringTableMutex(bool isInit = true) : mtx_(isInit) {}

    void LockWithThreadState(ThreadHolder* holder)
    {
        if (mtx_.TryLock()) {
            return;
        }
#ifndef NDEBUG
        BaseRuntime::RequestGC(GcType::ASYNC);  // Trigger CMC FULL GC
#endif
        ThreadStateTransitionScope<ThreadHolder, ThreadState::WAIT> ts(holder);
        mtx_.Lock();
    }

    void Lock()
    {
        return mtx_.Lock();
    }

    void Unlock()
    {
        return mtx_.Unlock();
    }

private:
    Mutex mtx_;
};

class BaseStringTableImpl : public BaseStringTableInterface<BaseStringTableImpl> {
public:
    BaseStringTableImpl() {}

    BaseString *GetOrInternFlattenString(ThreadHolder *holder, const HandleCreator &handleCreator, BaseString *string);

    BaseString *GetOrInternStringFromCompressedSubString(ThreadHolder *holder, const HandleCreator &handleCreator,
                                                         const ReadOnlyHandle<BaseString> &string, uint32_t offset,
                                                         uint32_t utf8Len);

    BaseString *GetOrInternString(ThreadHolder *holder, const HandleCreator &handleCreator, const uint8_t *utf8Data,
                                  uint32_t utf8Len, bool canBeCompress);

    BaseString *GetOrInternString(ThreadHolder *holder, const HandleCreator &handleCreator, const uint16_t *utf16Data,
                                  uint32_t utf16Len, bool canBeCompress);

    BaseString *TryGetInternString(const ReadOnlyHandle<BaseString> &string);

    HashTrieMap<BaseStringTableMutex, ThreadHolder> &GetHashTrieMap()
    {
        return stringTable_;
    }
private:
    HashTrieMap<BaseStringTableMutex, ThreadHolder> stringTable_{};
    static BaseString* AllocateLineStringObject(size_t size);
    static constexpr size_t MAX_REGULAR_HEAP_OBJECT_SIZE = 32 * KB;
};
}
#endif //COMMON_COMPONENTS_OBJECTS_STRING_TABLE_INTERNAL_H
