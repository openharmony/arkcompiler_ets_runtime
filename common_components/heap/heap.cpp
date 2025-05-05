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

#include "common_interfaces/heap/heap.h"

#include "common_components/common_runtime/src/heap_manager.h"
#include "common_components/common_runtime/src/heap/heap.h"

namespace panda::common {
void Heap::RequestGC(GcType type)
{
    switch (type) {
        case GcType::SYNC: {
            HeapManager::RequestGC(GC_REASON_USER, false);
            break;
        }
        case GcType::ASYNC: {
            HeapManager::RequestGC(GC_REASON_USER, true);
            break;
        }
        case GcType::FULL: {
            HeapManager::RequestGC(GC_REASON_BACKUP, false);
            break;
        }
    }
}

bool Heap::ForEachObj(HeapVisitor& visitor, bool safe)
{
    return panda::Heap::GetHeap().ForEachObject(visitor, safe);
}

size_t Heap::GetRegionSize()
{
    return 0;
}
}  // namespace panda::common
