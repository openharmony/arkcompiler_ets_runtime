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

#ifndef ECMASCRIPT_PERSISTENT_HANDLES_H
#define ECMASCRIPT_PERSISTENT_HANDLES_H

#include "ecmascript/common.h"
#include "ecmascript/platform/mutex.h"
#include "ecmascript/ecma_vm.h"

namespace panda::ecmascript {

class PersistentHandles {
public:
    PersistentHandles(EcmaVM *vm) : vm_(vm)
    {
        Init();
    }
    ~PersistentHandles();
    template <typename T>
    JSHandle<T> NewHandle(JSHandle<T> value)
    {
        return JSHandle<T>(GetJsHandleSlot(value.GetTaggedType()));
    }

    template <typename T>
    JSHandle<T> NewHandle(JSTaggedType value)
    {
        return JSHandle<T>(GetJsHandleSlot(value));
    }

    void Iterate(const RootRangeVisitor &rv);

    void SetTerminated()
    {
        isTerminate_ = true;
    }
private:
    void Init();
    uintptr_t GetJsHandleSlot(JSTaggedType value);
    uintptr_t Expand();

    EcmaVM *vm_ { nullptr };

    JSTaggedType *blockNext_ { nullptr };
    JSTaggedType *blockLimit_ { nullptr };
    PersistentHandles *pre_ { nullptr };
    PersistentHandles *next_ { nullptr };
    std::atomic_bool isTerminate_ {false};

    static constexpr uint32_t BLOCK_SIZE = 256L;
    std::vector<std::array<JSTaggedType, BLOCK_SIZE>*> handleBlocks_;
    friend class PersistentHandlesList;
};

class PersistentHandlesList {
public:
    void AddPersistentHandles(PersistentHandles *persistentHandles);
    void RemovePersistentHandles(PersistentHandles *persistentHandles);
    void Iterate(const RootRangeVisitor &rv);

private:
    PersistentHandles *listHead_ { nullptr };
    Mutex mutex_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PERSISTENT_HANDLES_H
