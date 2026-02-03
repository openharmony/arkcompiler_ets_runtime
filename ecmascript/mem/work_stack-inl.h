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

#ifndef ECMASCRIPT_MEM_WORK_STACK_INL_H
#define ECMASCRIPT_MEM_WORK_STACK_INL_H

#include "ecmascript/mem/work_stack.h"

namespace panda::ecmascript {

template <typename T, size_t capacity>
bool StackBase<T, capacity>::IsEmpty() const
{
    ASSERT(top_ <= capacity);
    return top_ == 0;
}

template <typename T, size_t capacity>
bool StackBase<T, capacity>::IsFull() const
{
    ASSERT(top_ <= capacity);
    return top_ == capacity;
}

template <typename T, size_t capacity>
void StackBase<T, capacity>::Push(T e)
{
    ASSERT(top_ <= capacity);
    ASSERT(!IsFull());
    Data()[top_++] = e;
}

template <typename T, size_t capacity>
void StackBase<T, capacity>::Pop(T *e)
{
    ASSERT(top_ <= capacity);
    ASSERT(!IsEmpty());
    *e = Data()[--top_];
}

template <typename T, size_t capacity>
StackBase<T, capacity> *StackBase<T, capacity>::GetNext() const
{
    return next_;
}

template <typename T, size_t capacity>
void StackBase<T, capacity>::SetNext(StackBase *next)
{
    next_ = next;
}

template <typename T, size_t capacity>
T *StackBase<T, capacity>::Data()
{
    return reinterpret_cast<T *>(this + 1);
}

template <typename T, size_t capacity>
void StackList<T, capacity>::Push(InternalStack *stack)
{
    if (stack == nullptr) {
        return;
    }
    LockHolder lock(mtx_);
    stack->SetNext(top_);
    top_ = stack;
}

template <typename T, size_t capacity>
bool StackList<T, capacity>::Pop(InternalStack **stack)
{
    LockHolder lock(mtx_);
    if (top_ == nullptr) {
        return false;
    }
    *stack = top_;
    top_ = top_->GetNext();
    return true;
}

template <typename T, size_t capacity>
void StackList<T, capacity>::Clear()
{
    LockHolder lock(mtx_);
    if (top_ != nullptr) {
        LOG_ECMA(ERROR) << "StackList is not empty";
    }
    top_ = nullptr;
}

template <typename T, size_t capacity>
bool StackList<T, capacity>::IsEmpty()
{
    LockHolder lock(mtx_);
    return top_ == nullptr;
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_WORK_STACK_INL_H