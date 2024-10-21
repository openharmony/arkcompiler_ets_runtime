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

#include "ecmascript/sustaining_js_handle.h"

#include "ecmascript/ecma_context.h"

namespace panda::ecmascript {
SustainingJSHandle::SustainingJSHandle(EcmaVM *vm) : vm_(vm)
{
    context_ = vm_->GetJSThread()->GetCurrentEcmaContext();
    context_->AddSustainingJSHandle(this);
}

SustainingJSHandle::~SustainingJSHandle()
{
    context_->RemoveSustainingJSHandle(this);
    for (auto block : handleBlocks_) {
        delete block;
    }
    handleBlocks_.clear();
}

uintptr_t SustainingJSHandle::GetJsHandleSlot(JSTaggedType value)
{
    if (blockNext_ == blockLimit_) {
        Expand();
    }
    ASSERT(blockNext_ != blockLimit_);

    *blockNext_ = value;
    uintptr_t slot = reinterpret_cast<uintptr_t>(blockNext_);
    blockNext_++;
    return slot;
}

uintptr_t SustainingJSHandle::Expand()
{
    auto block = new std::array<JSTaggedType, BLOCK_SIZE>();
    handleBlocks_.push_back(block);

    blockNext_ = &block->data()[0];
    blockLimit_ = &block->data()[BLOCK_SIZE];
    return reinterpret_cast<uintptr_t>(blockNext_);
}

void SustainingJSHandle::Iterate(const RootRangeVisitor &rv)
{
    size_t size = handleBlocks_.size();
    for (size_t i = 0; i < size; ++i) {
        auto block = handleBlocks_.at(i);
        auto start = block->data();
        auto end = (i != (size - 1)) ? &(block->data()[BLOCK_SIZE]) : blockNext_;
        rv(ecmascript::Root::ROOT_HANDLE, ObjectSlot(ToUintPtr(start)), ObjectSlot(ToUintPtr(end)));
    }
}

void SustainingJSHandleList::AddSustainingJSHandle(SustainingJSHandle *sustainingJSHandle)
{
    LockHolder lock(mutex_);
    if (sustainingJSHandle == nullptr) {
        return;
    }

    if (listHead_ == nullptr) {
        listHead_ = sustainingJSHandle;
        return;
    }
    sustainingJSHandle->next_ = listHead_;
    listHead_->pre_ = sustainingJSHandle;
    listHead_ = sustainingJSHandle;
}

void SustainingJSHandleList::RemoveSustainingJSHandle(SustainingJSHandle *sustainingJSHandle)
{
    LockHolder lock(mutex_);
    if (sustainingJSHandle == nullptr) {
        return;
    }

    auto next = sustainingJSHandle->next_;
    auto pre = sustainingJSHandle->pre_;
    if (pre != nullptr) {
        pre->next_ = next;
    }
    if (next != nullptr) {
        next->pre_ = pre;
    }

    if (listHead_ == sustainingJSHandle) {
        listHead_ = sustainingJSHandle->next_;
    }
}

void SustainingJSHandleList::Iterate(const RootRangeVisitor &rv)
{
    LockHolder lock(mutex_);
    for (auto handles = listHead_; handles != nullptr; handles = handles->next_) {
        handles->Iterate(rv);
    }
}
}  // namespace panda::ecmascript
