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

#ifndef ECMASCRIPT_MEM_CMS_MEM_SLOT_SPACE_CONFIG_H
#define ECMASCRIPT_MEM_CMS_MEM_SLOT_SPACE_CONFIG_H

#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/mem_common.h"

namespace panda::ecmascript {

class SlotSpaceConfig {
public:
    // fixme: just a proto to test
    static constexpr size_t SLOT_STEP_SIZE = static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT);
    static constexpr size_t MAX_SLOT_SIZE = DEFAULT_REGION_SIZE;
    static constexpr size_t MAX_REGULAR_HEAP_OBJECT_SLOT_SIZE = MAX_SLOT_SIZE / 2;
    static_assert(MAX_SLOT_SIZE % SLOT_STEP_SIZE == 0);
    static_assert(MAX_REGULAR_HEAP_OBJECT_SLOT_SIZE % SLOT_STEP_SIZE == 0);
    static constexpr size_t NUM_SLOTS = MAX_REGULAR_HEAP_OBJECT_SLOT_SIZE / SLOT_STEP_SIZE + 1;
    static constexpr size_t MAX_GC_TLAB_BUFFER_SIZE = MAX_REGULAR_HEAP_OBJECT_SLOT_SIZE / 4;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_CMS_MEM_SLOT_SPACE_H
