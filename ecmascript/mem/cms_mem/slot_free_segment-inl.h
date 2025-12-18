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

#ifndef ECMASCRIPT_MEM_CMS_MEM_SLOT_FREE_SEGMENT_INL_H
#define ECMASCRIPT_MEM_CMS_MEM_SLOT_FREE_SEGMENT_INL_H

#include "ecmascript/mem/cms_mem/slot_free_segment.h"

namespace panda::ecmascript {

SlotFreeSegment *SlotFreeSegment::FillFreeSegment(uintptr_t start, uintptr_t size)
{
    ASSERT(size >= sizeof(SlotFreeSegment));
    ASSERT(size % sizeof(JSTaggedType) == 0);

    SlotFreeSegment *freeSegment = reinterpret_cast<SlotFreeSegment *>(start);

    ASAN_UNPOISON_MEMORY_REGION(freeSegment, size);
    freeSegment->SetSize(size);
    freeSegment->SetNextFreeSegment(nullptr);
    ASAN_POISON_MEMORY_REGION(freeSegment, size);

    return freeSegment;
}
}  // namespace panda::ecmascript

#endif  //  ECMASCRIPT_MEM_WORK_MANAGER_INL_H