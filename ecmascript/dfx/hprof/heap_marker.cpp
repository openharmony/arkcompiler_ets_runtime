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

#include "ecmascript/dfx/hprof/heap_marker.h"

namespace panda::ecmascript {
bool HeapMarker::Mark(JSTaggedType addr)
{
    auto index = (addr & DEFAULT_REGION_MASK) >> TAGGED_TYPE_SIZE_LOG;
    Region *region = Region::ObjectAddressToRange(addr);
    auto it = regionBitsetMap_.find(region);
    if (it != regionBitsetMap_.end()) {
        if (it->second.test(index)) {
            return false;
        }
        it->second.set(index);
        ++count_;
        return true;
    }

    std::bitset<BITSET_SIZE> bitset;
    bitset.set(index);
    ++count_;
    regionBitsetMap_.emplace(region, bitset);
    return true;
}

bool HeapMarker::IsMarked(JSTaggedType addr)
{
    auto index = (addr & DEFAULT_REGION_MASK) >> TAGGED_TYPE_SIZE_LOG;
    Region *region = Region::ObjectAddressToRange(addr);
    auto bitsetIt = regionBitsetMap_.find(region);
    if (bitsetIt != regionBitsetMap_.end() && bitsetIt->second.test(index)) {
        return true;
    }
    return false;
}

void HeapMarker::Clear()
{
    regionBitsetMap_.clear();
}

void HeapMarker::IterateMarked(const std::function<void(JSTaggedType)> &cb)
{
    for (const auto &[region, bitset] : regionBitsetMap_) {
        for (size_t index = 0; index < bitset.size(); ++index) {
            if (bitset.test(index)) {
                JSTaggedType addr = reinterpret_cast<JSTaggedType>(region) + (index << TAGGED_TYPE_SIZE_LOG);
                cb(addr);
            }
        }
    }
}
}  // namespace panda::ecmascript