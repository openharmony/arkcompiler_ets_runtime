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

#ifndef MAPLEBE_INCLUDE_CG_SPARSE_DATAINFO_H
#define MAPLEBE_INCLUDE_CG_SPARSE_DATAINFO_H
#include "maple_string.h"
#include "common_utils.h"
#include "mempool.h"
#include "mempool_allocator.h"

namespace maplebe {
class SparseDataInfo {
    /*
     * SparseDataInfo has the same imterface like DataInfo
     * it can be used when the data member is small while the data
     * range is big.like in live analysis, in some extreme case the
     * vreg num range is 10k while in each bb, the max num of is 30+.
     */
public:
    SparseDataInfo(uint32 bitNum, MapleAllocator &alloc) : info(alloc.Adapter()), maxRegNum(bitNum) {}

    SparseDataInfo(const SparseDataInfo &other, MapleAllocator &alloc)
        : info(other.info, alloc.Adapter()), maxRegNum(other.maxRegNum)
    {
    }

    SparseDataInfo &Clone(MapleAllocator &alloc)
    {
        auto *dataInfo = alloc.New<SparseDataInfo>(*this, alloc);
        return *dataInfo;
    }

    ~SparseDataInfo() = default;

    void SetBit(uint32 bitNO)
    {
        info.insert(bitNO);
    }

    void ResetBit(uint32 bitNO)
    {
        info.erase(bitNO);
    }

    bool TestBit(uint32 bitNO) const
    {
        return info.find(bitNO) != info.end();
    }

    bool NoneBit() const
    {
        return info.empty();
    }

    size_t Size() const
    {
        return maxRegNum;
    }

    const MapleSet<uint32> &GetInfo() const
    {
        return info;
    }

    bool IsEqual(const SparseDataInfo &secondInfo) const
    {
        return info == secondInfo.GetInfo();
    }

    bool IsEqual(const MapleSet<uint32> &LiveInfoBak) const
    {
        return info == LiveInfoBak;
    }

    void AndBits(const SparseDataInfo &secondInfo)
    {
        for (auto it = info.begin(); it != info.end();) {
            if (!secondInfo.TestBit(*it)) {
                it = info.erase(it);
            } else {
                ++it;
            }
        }
    }

    void OrBits(const SparseDataInfo &secondInfo)
    {
        for (auto data : secondInfo.GetInfo()) {
            info.insert(data);
        }
    }

    /* if bit in secondElem is 1, bit in current DataInfo is set 0 */
    void Difference(const SparseDataInfo &secondInfo)
    {
        for (auto it = info.begin(); it != info.end();) {
            if (secondInfo.TestBit(*it)) {
                it = info.erase(it);
            } else {
                ++it;
            }
        }
    }

    void ResetAllBit()
    {
        info.clear();
    }

    void EnlargeCapacityToAdaptSize(uint32 bitNO)
    {
        /* add one more size for each enlarge action */
    }

    void ClearDataInfo()
    {
        info.clear();
    }

    template <typename T>
    void GetBitsOfInfo(T &wordRes) const
    {
        wordRes = info;
    }

private:
    /* long type has 8 bytes, 64 bits */
    static constexpr int32 kWordSize = 64;
    MapleSet<uint32> info;
    uint32 maxRegNum;
};
} /* namespace maplebe */
#endif /* MAPLEBE_INCLUDE_CG_INSN_H */
