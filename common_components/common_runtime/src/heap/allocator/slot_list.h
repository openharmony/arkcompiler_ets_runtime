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
#ifndef ARK_COMMON_SLOT_LIST_H
#define ARK_COMMON_SLOT_LIST_H

#include "common_components/common_runtime/src/common/base_object.h"

namespace panda {
struct ObjectSlot {
    panda::BaseStateWord stateWord; // same with BaseObject::stateWord
    ObjectSlot* next;
};

class SlotList {
public:
    void PushFront(BaseObject* slot)
    {
        ObjectSlot* headSlot = reinterpret_cast<ObjectSlot*>(slot);
        ClearExtraContent(slot);
        headSlot->next = head_;
        head_ = headSlot;
    }

    uintptr_t PopFront(size_t size)
    {
        if (head_ == nullptr || size != reinterpret_cast<BaseObject*>(head_)->GetSize()) {
            return 0;
        }
        ObjectSlot* allocSlot = head_;
        head_ = head_->next;
        allocSlot->next = nullptr;
        return reinterpret_cast<uintptr_t>(allocSlot);
    }

    void Clear() { head_ = nullptr; }

    // Clear the rest memory of slot object if the slot object size is greater than ObjectSlot(16 Bytes).
    void ClearExtraContent(BaseObject* slot)
    {
        size_t size = slot->GetSize() - sizeof(ObjectSlot);
        if (size > 0) {
            HeapAddress start = reinterpret_cast<uintptr_t>(slot) + sizeof(ObjectSlot);
            LOGE_IF((memset_s(reinterpret_cast<void*>(start), size, 0, size) != EOK)) << "memset_s fail";
        }
    }

private:
    ObjectSlot* head_ = nullptr;
};
} // namespace panda
#endif // ARK_COMMON_SLOT_LIST_H
