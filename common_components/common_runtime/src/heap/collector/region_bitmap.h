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

    struct BitMaskInfo {
        size_t headWordIdx;
        uint64_t headMaskBits;
        size_t tailWordCnt; // count of mask words excludes the start mask
        uint64_t lastMaskBits;
    };

    static void GetBitMaskInfo(size_t start, size_t byteCnt, BitMaskInfo& maskInfo)
    {
        (void)byteCnt;
        size_t headWordIdx = (start / kMarkedBytesPerBit) / kBitsPerWord;
        size_t headMaskBitStart = (start / kMarkedBytesPerBit) % kBitsPerWord;
        maskInfo.headWordIdx = headWordIdx;
        maskInfo.headMaskBits = static_cast<uint64_t>(1) << headMaskBitStart;
        maskInfo.tailWordCnt = 0;
        maskInfo.lastMaskBits = 0;
    }

    bool ApplyBitMaskInfo(const BitMaskInfo& maskInfo, size_t byteCnt, size_t regionSize)
    {
        uint64_t old = markWords[maskInfo.headWordIdx].load();
        bool isMarked = ((old & maskInfo.headMaskBits) != 0);
        if (!isMarked) {
            old = markWords[maskInfo.headWordIdx].fetch_or(maskInfo.headMaskBits);
            isMarked = ((old & maskInfo.headMaskBits) != 0);
            if (!isMarked) {
                size_t markWordSize = regionSize / (kMarkedBytesPerBit * kBitsPerWord);
                CHECK_CC(markWordSize >= factor);
                partLiveBytes[maskInfo.headWordIdx / (markWordSize / factor)].fetch_add(
                    __builtin_popcountll(maskInfo.headMaskBits));
                liveBytes.fetch_add(byteCnt);

                if (maskInfo.tailWordCnt > 0) {
                    size_t lastWordIdx = maskInfo.headWordIdx + maskInfo.tailWordCnt;
                    if (maskInfo.lastMaskBits != 0) {
                        for (size_t idx = maskInfo.headWordIdx + 1; idx < lastWordIdx; idx++) {
                            uint64_t zeros = markWords[idx].fetch_or(~static_cast<uint64_t>(0));
                            partLiveBytes[idx / (markWordSize / factor)].fetch_add(kBitsPerWord);
                            DCHECK_CC(zeros == 0);
                        }
                        markWords[lastWordIdx].fetch_or(maskInfo.lastMaskBits);
                        partLiveBytes[lastWordIdx / (markWordSize / factor)].fetch_add(
                            __builtin_popcountll(maskInfo.lastMaskBits));
                    } else {
                        for (size_t idx = maskInfo.headWordIdx + 1; idx <= lastWordIdx; idx++) {
                            uint64_t zeros = markWords[idx].fetch_or(~static_cast<uint64_t>(0));
                            partLiveBytes[idx / (markWordSize / factor)].fetch_add(kBitsPerWord);
                            DCHECK_CC(zeros == 0);
                        }
                    }
                }
            }
            return isMarked;
        }
        return isMarked;
    }

    explicit RegionBitmap(size_t regionSize) : liveBytes(0), wordCnt(regionSize / (kMarkedBytesPerBit * kBitsPerWord))
    {}

    bool MarkBits(size_t start, size_t byteCnt, size_t regionSize)
    {
        BitMaskInfo maskInfo;
        GetBitMaskInfo(start, byteCnt, maskInfo);
        return ApplyBitMaskInfo(maskInfo, byteCnt, regionSize);
    }

    bool IsMarked(size_t start)
    {
        size_t headWordIdx = (start / kMarkedBytesPerBit) / kBitsPerWord;
        size_t headMaskBitStart = (start / kMarkedBytesPerBit) % kBitsPerWord;
        uint64_t mask = static_cast<uint64_t>(1) << headMaskBitStart;
        bool ret = ((markWords[headWordIdx].load() & mask) != 0);
        return ret;
    }

    uint64_t GetPreLiveBytes(size_t offset, size_t regionSize)
    {
        uint64_t preLiveBits = 0;
        ssize_t  partStartIndex = 0;
        ssize_t  index = static_cast<ssize_t>(offset / (kBitsPerWord * kMarkedBytesPerBit));
        size_t markWordSize = regionSize / (kMarkedBytesPerBit * kBitsPerWord);
        CHECK_CC(markWordSize >= factor);
        int8_t partIndex = static_cast<int8_t>(index / (markWordSize / factor) - 1);
        while (partIndex >= 0) {
            preLiveBits += partLiveBytes[partIndex--];
            partStartIndex += static_cast<ssize_t>(markWordSize / factor);
        }
        size_t bitIndex = (offset / kMarkedBytesPerBit) % kBitsPerWord;
        uint64_t mask = (static_cast<uint64_t>(1) << bitIndex) - 1;
        size_t liveBits = __builtin_popcountll(markWords[index].load() & mask);
        if (index == partStartIndex) {
            return (preLiveBits + liveBits) * kMarkedBytesPerBit;
        }
        index--;
        while (index >= partStartIndex) {
            uint64_t makeBit = markWords[index].load();
            liveBits += __builtin_popcountll(makeBit);
            index--;
        }
        return (preLiveBits + liveBits) * kMarkedBytesPerBit;
    }
};
struct RegionLiveDesc {
    static constexpr HeapAddress TEMPORARY_PTR = 0x1234;
    RegionDesc* relatedRegion = nullptr;
    RegionBitmap* markBitmap = nullptr;
    RegionBitmap* resurrectBitmap = nullptr;
    RegionBitmap* enqueueBitmap = nullptr;

    uint64_t GetPreLiveBytes(size_t offset, size_t regionSize)
    {
        uint64_t liveBytes = 0;
        if (markBitmap != nullptr) {
            liveBytes += markBitmap->GetPreLiveBytes(offset, regionSize);
        }
        if (resurrectBitmap != nullptr) {
            liveBytes += resurrectBitmap->GetPreLiveBytes(offset, regionSize);
        }
        return liveBytes;
    }
};

} // namespace panda
#endif // ARK_COMMON_LIVE_INFO_H
