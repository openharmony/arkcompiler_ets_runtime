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
#ifndef COMMON_COMPONENTS_HEAP_COLLECTOR_REGION_RSET_H
#define COMMON_COMPONENTS_HEAP_COLLECTOR_REGION_RSET_H
#include <atomic>

namespace common {
static constexpr size_t CARD_SIZE = 512;
class RegionRSet {
public:
    explicit RegionRSet(size_t regionSize) : cardCnt(regionSize / CARD_SIZE)
    {
#ifdef _WIN64
        void* startAddress = VirtualAlloc(NULL, cardCnt * sizeof(uint64_t), MEM_RESERVE, PAGE_READWRITE);
        if (startAddress == NULL) {
            LOG_COMMON(FATAL) << "failed to initialize HeapBitmapManager";
            UNREACHABLE_CC();
        }
#else
        void* startAddress = mmap(nullptr, cardCnt * sizeof(uint64_t),
            PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (startAddress == MAP_FAILED) {
            LOG_COMMON(FATAL) << "failed to initialize HeapBitmapManager";
            UNREACHABLE_CC();
        } else {
#ifndef __APPLE__
            (void)madvise(startAddress, cardCnt * sizeof(uint64_t), MADV_NOHUGEPAGE);
            COMMON_PRCTL(startAddress, cardCnt * sizeof(uint64_t), "forward_data");
#endif
        }
#endif
        cardTable = reinterpret_cast<std::atomic<uint64_t>*>(startAddress);
    }

    ~RegionRSet()
    {
#ifdef _WIN64
        if (!VirtualFree(reinterpret_cast<void*>(cardTable), 0, MEM_RELEASE)) {
            LOG_COMMON(ERROR) << "VirtualFree error for HeapBitmapManager";
        }
#else
        if (munmap(reinterpret_cast<void*>(cardTable), cardCnt * sizeof(uint64_t)) != 0) {
            LOG_COMMON(ERROR) << "munmap error for HeapBitmapManager";
        }
#endif
    }

    bool MarkCardTable(size_t offset)
    {
        size_t cardIdx = (offset / kMarkedBytesPerBit) / kBitsPerWord;
        size_t headMaskBitStart = (offset / kMarkedBytesPerBit) % kBitsPerWord;
        uint64_t headMaskBits = static_cast<uint64_t>(1) << headMaskBitStart;
        uint64_t card = cardTable[cardIdx].load();
        bool isMarked = ((card & headMaskBits) != 0);
        if (!isMarked) {
            card = cardTable[cardIdx].fetch_or(headMaskBits);
            isMarked = ((card & headMaskBits) != 0);
            return isMarked;
        }
        return isMarked;
    }

    bool IsMarkedCard(size_t offset)
    {
        size_t cardIdx = (offset / kMarkedBytesPerBit) / kBitsPerWord;
        size_t headMaskBitStart = (offset / kMarkedBytesPerBit) % kBitsPerWord;
        uint64_t headMaskBits = static_cast<uint64_t>(1) << headMaskBitStart;
        return (cardTable[cardIdx].load() & headMaskBits) != 0;
    }

    void ClearCardTable()
    {
        LOGF_CHECK(memset_s(cardTable, cardCnt * sizeof(uint64_t), 0, cardCnt * sizeof(uint64_t)) == EOK)
            << "memset_s fail";
    }

    void VisitAllMarkedCard(const std::function<void(BaseObject*)>& func, HeapAddress regionStart)
    {
        for (size_t i = 0; i < cardCnt.load(); i++) {
            uint64_t card = cardTable[i].load();
            for (size_t j = 0; j < kBitsPerWord; j++) {
                uint64_t mask = static_cast<uint64_t>(1) << j;
                if ((card & mask) == 0) {
                    continue;
                }
                BaseObject* obj = reinterpret_cast<BaseObject*>(regionStart +
                    static_cast<HeapAddress>((i * kBitsPerWord) * kBitsPerByte + j * kBitsPerByte));
                func(obj);
            }
        }
    }
private:
    std::atomic<size_t> cardCnt;
    std::atomic<uint64_t>* cardTable;
};
}
#endif // COMMON_COMPONENTS_HEAP_COLLECTOR_REGION_RSET_H
