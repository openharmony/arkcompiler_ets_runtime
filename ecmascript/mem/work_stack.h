/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_WORK_STACK_H
#define ECMASCRIPT_MEM_WORK_STACK_H

#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/platform/mutex.h"

namespace panda::ecmascript {
template <typename T, size_t capacity>
class StackBase {
public:
    using ElementType = T;

    static constexpr size_t GetAllocateSize()
    {
        return sizeof(StackBase) + sizeof(T) * capacity;
    }

    StackBase() = default;
    ~StackBase()
    {
        ASSERT(IsEmpty());
    }

    NO_COPY_SEMANTIC(StackBase);
    NO_MOVE_SEMANTIC(StackBase);

    bool IsEmpty() const;

    bool IsFull() const;

    void Push(T e);

    void Pop(T *e);

    StackBase *GetNext() const;

    void SetNext(StackBase *next);

private:
    T *Data();

    size_t top_ {0};
    StackBase *next_ {nullptr};
};

template <typename T, size_t capacity>
class StackList {
public:
    using InternalStack = StackBase<T, capacity>;
    StackList() : top_(nullptr) {}
    ~StackList() = default;

    NO_COPY_SEMANTIC(StackList);
    NO_MOVE_SEMANTIC(StackList);

    void Push(InternalStack *stack);

    bool Pop(InternalStack **stack);

    void Clear();

    bool IsEmpty();

private:
    InternalStack *top_ {nullptr};
    Mutex mtx_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_WORK_STACK_H