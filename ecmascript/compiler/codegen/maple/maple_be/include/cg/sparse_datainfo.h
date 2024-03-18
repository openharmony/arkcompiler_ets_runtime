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
#include "maple_sparse_bitvector.h"

namespace maplebe {
class SparseDataInfo {
/*
 * SparseDataInfo has the same imterface like DataInfo
 * it can be used when the data member is small while the data
 * range is big.like in live analysis, in some extreme case the
 * vreg num range is 10k while in each bb, the max num of is 30+
*/
public:
    SparseDataInfo(uint32 bitNum, const MapleAllocator &alloc) : allocator(alloc), info(allocator), maxRegNum(bitNum) {}

    SparseDataInfo(const SparseDataInfo &other, const MapleAllocator &alloc)
        : allocator(alloc),
          info(other.info, allocator),
          maxRegNum(other.maxRegNum)
    {
    }

    SparseDataInfo(const SparseDataInfo &other)
        : allocator(other.allocator),
          info(other.info, allocator),
          maxRegNum(other.maxRegNum)
    {
    }

    SparseDataInfo &Clone(MapleAllocator &alloc) const
    {
        auto *dataInfo = alloc.New<SparseDataInfo>(*this, alloc);
        return *dataInfo;
    }

    ~SparseDataInfo() = default;

    SparseDataInfo &operator=(const SparseDataInfo &other)
    {
        if (this == &other) {
            return *this;
        }
        allocator = other.GetAllocator();
        info = other.GetInfo();
        maxRegNum = other.GetMaxRegNum();
        return *this;
    }

    void SetBit(uint32 bitNO)
    {
        info.Set(bitNO);
    }

    void ResetBit(uint32 bitNO)
    {
        info.Reset(bitNO);
    }

    bool TestBit(uint32 bitNO) const
    {
        return info.Test(bitNO);
    }

    bool NoneBit() const
    {
        return info.Empty();
    }

    size_t Size() const
    {
        return maxRegNum;
    }

    const MapleAllocator &GetAllocator() const
    {
        return allocator;
    }

    const MapleSparseBitVector<> &GetInfo() const
    {
        return info;
    }

    uint32 GetMaxRegNum() const
    {
        return maxRegNum;
    }

    bool IsEqual(const SparseDataInfo &secondInfo) const
    {
        return info == secondInfo.GetInfo();
    }

    bool IsEqual(const MapleSparseBitVector<> &liveInfoBak) const
    {
        return info == liveInfoBak;
    }

    void AndBits(const SparseDataInfo &secondInfo)
    {
        info &= secondInfo.info;
    }

    bool OrBits(const SparseDataInfo &secondInfo)
    {
        return info |= secondInfo.info;
    }

    /* if bit in secondElem is 1, bit in current DataInfo is set 0 */
    bool Difference(const SparseDataInfo &secondInfo)
    {
        return info.Diff(secondInfo.info);
    }

    void ResetAllBit()
    {
        info.Clear();
    }

    void EnlargeCapacityToAdaptSize(uint32 /* bitNO */) const
    {
        /* add one more size for each enlarge action */
    }

    void ClearDataInfo()
    {
        info.Clear();
    }

    template<typename T>
    void GetBitsOfInfo(T &wordRes) const
    {
        info.ConvertToSet(wordRes);
    }

private:
    MapleAllocator allocator;
    MapleSparseBitVector<> info;
    uint32 maxRegNum;
};
}  /* namespace maplebe */
#endif  /* MAPLEBE_INCLUDE_CG_SPARSE_DATAINFO_H */
