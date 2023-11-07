/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/base/depend_chain_helper.h"

namespace panda::ecmascript::kungfu {

void DependChains::Merge(DependChains* that)
{
    // find common sub list
    while (size_ > that->size_) {
        head_ = head_->next;
        size_--;
    }

    auto lhs = this->head_;
    auto rhs = that->head_;
    size_t rhsSize = that->size_;
    while (rhsSize > size_) {
        rhs = rhs->next;
        rhsSize--;
    }
    while (lhs != rhs) {
        ASSERT(lhs != nullptr);
        lhs = lhs->next;
        rhs = rhs->next;
        size_--;
    }
    head_ = lhs;
}

bool DependChains::Equals(DependChains* that)
{
    if (that == nullptr) {
        return false;
    }
    if (size_ != that->size_) {
        return false;
    }
    auto lhs = this->head_;
    auto rhs = that->head_;
    while (lhs != rhs) {
        if (lhs->gate != rhs->gate) {
            return false;
        }
        lhs = lhs->next;
        rhs = rhs->next;
    }
    return true;
}

DependChains* DependChains::UpdateNode(GateRef gate)
{
    // assign node->next to head
    Node* node = chunk_->New<Node>(gate, head_);
    DependChains* that = new (chunk_) DependChains(chunk_);
    // assign head to node
    that->head_ = node;
    that->size_ = size_ + 1;
    return that;
}
} // namespace panda::ecmascript::kungfu