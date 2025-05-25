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
#ifndef ARK_COMMON_LIVE_INFO_H
#define ARK_COMMON_LIVE_INFO_H
#include "common_components/common_runtime/src/base/immortal_wrapper.h"
#include "common_components/common_runtime/src/base/mem_utils.h"
#include "common_components/common_runtime/src/base/sys_call.h"
#include "common_components/common_runtime/src/heap/heap.h"
#if defined(__linux__) || defined(PANDA_TARGET_OHOS) || defined(__APPLE__)
#include <sys/mman.h>
#endif

namespace panda {
constexpr size_t kBitsPerByte = 8;
constexpr size_t kMarkedBytesPerBit = 8;
constexpr size_t kBitsPerWord = sizeof(uint64_t) * kBitsPerByte;
class RegionDesc;
struct RegionBitmap {
    static constexpr uint8_t factor = 16;
    std::atomic<uint16_t> partLiveBytes[factor];
    std::atomic<size_t> liveBytes;

    // 1 bit marks 8 bytes in region, 64 bits per word.
    // word count = region size / (8 * 64) = region size / 512, should be dynamically decided at runtime.
    std::atomic<size_t> wordCnt;
    std::atomic<uint64_t> markWords[0];

    static size_t GetRegionBitmapSize(size_t regionSize)
    {
        return sizeof(RegionBitmap) + ((regionSize / (kMarkedBytesPerBit * kBitsPerWord)) * sizeof(uint64_t));
    }

    explicit RegionBitmap(size_t regionSize) : liveBytes(0), wordCnt(regionSize / (kMarkedBytesPerBit * kBitsPerWord))
    {}

    bool MarkBits(size_t start)
    {
        size_t headWordIdx = (start / kMarkedBytesPerBit) / kBitsPerWord;
        size_t headMaskBitStart = (start / kMarkedBytesPerBit) % kBitsPerWord;
        uint64_t headMaskBits = static_cast<uint64_t>(1) << headMaskBitStart;
        uint64_t old = markWords[headWordIdx].load();
        bool isMarked = ((old & headMaskBits) != 0);
        if (!isMarked) {
            old = markWords[headWordIdx].fetch_or(headMaskBits);
            isMarked = ((old & headMaskBits) != 0);
            return isMarked;
        }
        return isMarked;
    }

    bool IsMarked(size_t start)
    {
        size_t headWordIdx = (start / kMarkedBytesPerBit) / kBitsPerWord;
        size_t headMaskBitStart = (start / kMarkedBytesPerBit) % kBitsPerWord;
        uint64_t mask = static_cast<uint64_t>(1) << headMaskBitStart;
        bool ret = ((markWords[headWordIdx].load() & mask) != 0);
        return ret;
    }
};
struct RegionLiveDesc {
    static constexpr HeapAddress TEMPORARY_PTR = 0x1234;
    RegionDesc* relatedRegion = nullptr;
    RegionBitmap* markBitmap = nullptr;
    RegionBitmap* resurrectBitmap = nullptr;
    RegionBitmap* enqueueBitmap = nullptr;
};

} // namespace panda
#endif // ARK_COMMON_LIVE_INFO_H
