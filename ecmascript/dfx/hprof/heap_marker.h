/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_DFX_HPROF_HEAP_MARKER_H
#define ECMASCRIPT_DFX_HPROF_HEAP_MARKER_H

#include <functional>

#include "common_components/heap/allocator/region_desc.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/region.h"

namespace panda::ecmascript {
class HeapMarker {
public:
    HeapMarker() = default;
    ~HeapMarker() = default;
    NO_MOVE_SEMANTIC(HeapMarker);
    NO_COPY_SEMANTIC(HeapMarker);

    bool Mark(JSTaggedType addr);
    bool CMCMark(JSTaggedType addr);
    bool IsMarked(JSTaggedType addr);
    bool IsCMCMarked(JSTaggedType addr);
    void Clear();
    void IterateMarked(const std::function<void(JSTaggedType)> &cb);
    void IterateCMCMarked(const std::function<void(JSTaggedType)> &cb);

    uint32_t Count() const
    {
        return count_;
    }

private:
    static constexpr size_t BITSET_SIZE = DEFAULT_REGION_SIZE >> TAGGED_TYPE_SIZE_LOG;
    static constexpr size_t CMC_BITSET_SIZE = common::RegionDesc::UNIT_SIZE >> TAGGED_TYPE_SIZE_LOG;
    CUnorderedMap<Region *, std::bitset<BITSET_SIZE>> regionBitsetMap_ {};
    CUnorderedMap<JSTaggedType, std::bitset<CMC_BITSET_SIZE>> cmcRegionBitsetMap_ {};
    uint32_t count_ {0};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_DFX_HPROF_HEAP_MARKER_H
