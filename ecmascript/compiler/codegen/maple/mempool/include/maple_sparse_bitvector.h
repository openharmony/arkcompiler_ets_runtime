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

#ifndef MEMPOOL_INCLUDE_MAPLE_SPARSE_BITVECTOR_H
#define MEMPOOL_INCLUDE_MAPLE_SPARSE_BITVECTOR_H
#include <climits>
#include <iterator>
#include <list>
#include <iostream>
#include <bitset>
#include <set>
#include "mempool_allocator.h"

namespace maple {
template <unsigned bitVectorSize = 64>
struct MapleSparseBitVectorElement {
    using BitWord = unsigned long long;
    enum { kBitWordSize = sizeof(BitWord) * CHAR_BIT, kBitWordNum = (bitVectorSize + kBitWordSize - 1) / kBitWordSize };

    explicit MapleSparseBitVectorElement(unsigned idx) : index(idx), bitVector {0} {}

    BitWord GetWord(unsigned idx) const
    {
        return bitVector[idx];
    }

    unsigned GetIndex() const
    {
        return index;
    }

    void Set(unsigned idx)
    {
        bitVector[idx / kBitWordSize] |= 1ULL << (idx % kBitWordSize);
    }

    void Reset(unsigned idx)
    {
        bitVector[idx / kBitWordSize] &= ~(1ULL << (idx % kBitWordSize));
    }

    bool Test(unsigned idx) const
    {
        return bitVector[idx / kBitWordSize] & (1ULL << (idx % kBitWordSize));
    }

    bool Empty() const
    {
        for (unsigned i = 0; i < kBitWordNum; i++) {
            if (bitVector[i]) {
                return false;
            }
        }
        return true;
    }

    void ConvertToSet(MapleSet<uint32> &res, unsigned base) const
    {
        for (unsigned i = 0; i < kBitWordNum; i++) {
            BitWord tmp = bitVector[i];
            unsigned bitPos = 0;
            while (tmp != 0) {
                unsigned trailingZeroNum = static_cast<unsigned>(__builtin_ctzll(tmp));
                bitPos += trailingZeroNum;
                tmp >>= trailingZeroNum;
                DEBUG_ASSERT(tmp > 0, "tmp should not be zero; tmp should end with 1 in binary after shifting");
                tmp -= 1;
                res.insert(base + bitPos + i * kBitWordSize);
            }
        }
    }

    void ConvertToSet(std::set<uint32> &res, unsigned base) const
    {
        for (unsigned i = 0; i < kBitWordNum; i++) {
            BitWord tmp = bitVector[i];
            unsigned bitPos = 0;
            while (tmp != 0) {
                unsigned trailingZeroNum = static_cast<unsigned>(__builtin_ctzll(tmp));
                bitPos += trailingZeroNum;
                tmp >>= trailingZeroNum;
                tmp -= 1;
                res.insert(base + bitPos + i * kBitWordSize);
            }
        }
    }

    bool And(const MapleSparseBitVectorElement &rhs, bool &becameZero)
    {
        bool changed = false;
        bool allzero = true;
        becameZero = false;
        for (unsigned i = 0; i < kBitWordNum; i++) {
            BitWord oldBitWord = bitVector[i];
            bitVector[i] &= rhs.bitVector[i];
            changed = changed || (oldBitWord != bitVector[i]);
            if (bitVector[i] != 0) {
                allzero = false;
            }
        }
        becameZero = allzero;
        return changed;
    }

    bool Or(const MapleSparseBitVectorElement &rhs)
    {
        bool changed = false;
        for (unsigned i = 0; i < kBitWordNum; i++) {
            BitWord oldBitWord = bitVector[i];
            bitVector[i] |= rhs.bitVector[i];
            changed = changed || (oldBitWord != bitVector[i]);
        }
        return changed;
    }

    bool Diff(const MapleSparseBitVectorElement &rhs, bool &becameZero)
    {
        bool changed = false;
        bool allzero = true;
        becameZero = false;
        for (unsigned i = 0; i < kBitWordNum; i++) {
            BitWord oldBitWord = bitVector[i];
            bitVector[i] &= ~rhs.bitVector[i];
            changed = changed || (oldBitWord != bitVector[i]);
            if (bitVector[i] != 0) {
                allzero = false;
            }
        }
        becameZero = allzero;
        return changed;
    }

    bool operator==(const MapleSparseBitVectorElement &rhs) const
    {
        if (index != rhs.GetIndex()) {
            return false;
        }
        for (unsigned i = 0; i < kBitWordNum; i++) {
            if (bitVector[i] != rhs.bitVector[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const MapleSparseBitVectorElement &rhs) const
    {
        return !(*this == rhs);
    }

private:
    unsigned index;
    BitWord bitVector[kBitWordNum];
};

template <unsigned bitVectorSize = 64>
class MapleSparseBitVector {
    using ElementList = MapleList<MapleSparseBitVectorElement<bitVectorSize>>;
    using ElementListIterator = typename ElementList::iterator;
    using ElementListConstIterator = typename ElementList::const_iterator;
    using BitWord = unsigned long long;

public:
    explicit MapleSparseBitVector(const MapleAllocator &alloc)
        : allocator(alloc), elementList(allocator.Adapter()), currIter(elementList.begin())
    {
    }

    explicit MapleSparseBitVector(const MapleSparseBitVector &rhs, const MapleAllocator &alloc)
        : allocator(alloc), elementList(rhs.elementList, allocator.Adapter()), currIter(elementList.begin())
    {
    }

    MapleSparseBitVector(const MapleSparseBitVector &rhs)
        : allocator(rhs.allocator), elementList(rhs.elementList, allocator.Adapter()), currIter(elementList.begin())
    {
    }

    void Set(unsigned bitNO)
    {
        unsigned idx = bitNO / bitVectorSize;
        ElementListIterator iter;
        if (elementList.empty()) {
            iter = elementList.emplace(elementList.end(), idx);
        } else {
            iter = LowerBoundFor(idx);
            if (iter == elementList.end() || iter->GetIndex() != idx) {
                if (iter != elementList.end() && iter->GetIndex() < idx) {
                    ++iter;
                }
                iter = elementList.emplace(iter, idx);
            }
        }
        currIter = iter;
        iter->Set(bitNO % bitVectorSize);
    }

    void Reset(unsigned bitNO)
    {
        if (elementList.empty()) {
            return;
        }
        unsigned idx = bitNO / bitVectorSize;
        ElementListIterator iter = LowerBoundFor(idx);
        if (iter == elementList.end() || iter->GetIndex() != idx) {
            return;
        }
        iter->Reset(bitNO % bitVectorSize);
        if (iter->Empty()) {
            ++currIter;
            elementList.erase(iter);
        }
    }

    bool Test(unsigned bitNO) const
    {
        if (elementList.empty()) {
            return false;
        }
        unsigned idx = bitNO / bitVectorSize;
        ElementListConstIterator iter = LowerBoundForConst(idx);
        if (iter == elementList.end() || iter->GetIndex() != idx) {
            return false;
        }
        return (iter->Test(bitNO % bitVectorSize));
    }

    MapleSparseBitVector &operator=(const MapleSparseBitVector &rhs)
    {
        if (this == &rhs) {
            return *this;
        }
        allocator = rhs.allocator;
        elementList = rhs.elementList;
        currIter = elementList.begin();
        return *this;
    }

    bool operator&=(const MapleSparseBitVector &rhs)
    {
        if (this == &rhs) {
            return false;
        }

        bool changed = false;
        ElementListIterator iter1 = elementList.begin();
        ElementListConstIterator iter2 = rhs.elementList.begin();

        if (elementList.empty() || rhs.elementList.empty()) {
            return false;
        }

        while (iter2 != rhs.elementList.end()) {
            if (iter1 == elementList.end()) {
                currIter = elementList.begin();
                return changed;
            }

            if (iter1->GetIndex() > iter2->GetIndex()) {
                ++iter2;
            } else if (iter1->GetIndex() == iter2->GetIndex()) {
                bool becameZero;
                changed = iter1->And(*iter2, becameZero) || changed;
                if (becameZero) {
                    ElementListIterator iterTmp = iter1;
                    ++iter1;
                    elementList.erase(iterTmp);
                } else {
                    ++iter1;
                }
                ++iter2;
            } else {
                ElementListIterator iterTmp = iter1;
                ++iter1;
                elementList.erase(iterTmp);
                changed = true;
            }
        }
        if (iter1 != elementList.end()) {
            elementList.erase(iter1, elementList.end());
            changed = true;
        }
        currIter = elementList.begin();
        return changed;
    }

    bool operator|=(const MapleSparseBitVector &rhs)
    {
        if (this == &rhs) {
            return false;
        }

        bool changed = false;
        ElementListIterator iter1 = elementList.begin();
        ElementListConstIterator iter2 = rhs.elementList.begin();

        if (rhs.elementList.empty()) {
            return false;
        }

        while (iter2 != rhs.elementList.end()) {
            if (iter1 == elementList.end() || iter1->GetIndex() > iter2->GetIndex()) {
                elementList.insert(iter1, *iter2);
                ++iter2;
                changed = true;
            } else if (iter1->GetIndex() == iter2->GetIndex()) {
                changed = iter1->Or(*iter2) || changed;
                ++iter1;
                ++iter2;
            } else {
                ++iter1;
            }
        }
        currIter = elementList.begin();
        return changed;
    }

    void Clear()
    {
        elementList.clear();
        currIter = elementList.begin();
    }

    bool operator==(const MapleSparseBitVector &rhs) const
    {
        ElementListConstIterator iter1 = elementList.begin();
        ElementListConstIterator iter2 = rhs.elementList.begin();
        for (; iter1 != elementList.end() && iter2 != rhs.elementList.end(); ++iter1, ++iter2) {
            if (*iter1 != *iter2) {
                return false;
            }
        }
        return iter1 == elementList.end() && iter2 == rhs.elementList.end();
    }

    bool operator!=(const MapleSparseBitVector &rhs) const
    {
        return !(*this == rhs);
    }

    bool Empty() const
    {
        if (elementList.empty()) {
            return true;
        }
        ElementListConstIterator iter1 = elementList.begin();
        for (; iter1 != elementList.end(); ++iter1) {
            if (!iter1->Empty()) {
                return false;
            }
        }
        return true;
    }

    bool Diff(const MapleSparseBitVector &rhs)
    {
        if (this == &rhs) {
            if (!Empty()) {
                Clear();
                return true;
            }
            return false;
        }

        bool changed = false;
        ElementListIterator iter1 = elementList.begin();
        ElementListConstIterator iter2 = rhs.elementList.begin();

        if (elementList.empty() || rhs.elementList.empty()) {
            return false;
        }

        while (iter2 != rhs.elementList.end()) {
            if (iter1 == elementList.end()) {
                currIter = elementList.begin();
                return changed;
            }

            if (iter1->GetIndex() > iter2->GetIndex()) {
                ++iter2;
            } else if (iter1->GetIndex() == iter2->GetIndex()) {
                bool becameZero;
                changed = iter1->Diff(*iter2, becameZero) || changed;
                if (becameZero) {
                    ElementListIterator iterTmp = iter1;
                    ++iter1;
                    elementList.erase(iterTmp);
                } else {
                    ++iter1;
                }
                ++iter2;
            } else {
                ++iter1;
            }
        }
        currIter = elementList.begin();
        return changed;
    }

    void ConvertToSet(MapleSet<uint32> &res) const
    {
        for (auto &element : elementList) {
            unsigned pos = bitVectorSize * element.GetIndex();
            element.ConvertToSet(res, pos);
        }
    }

    void ConvertToSet(std::set<uint32> &res) const
    {
        for (auto &element : elementList) {
            unsigned pos = bitVectorSize * element.GetIndex();
            element.ConvertToSet(res, pos);
        }
    }

private:
    ElementListIterator LowerBoundForImpl(unsigned idx) const
    {
        ElementListIterator begin = const_cast<MapleSparseBitVector *>(this)->elementList.begin();
        ElementListIterator end = const_cast<MapleSparseBitVector *>(this)->elementList.end();

        if (elementList.empty()) {
            currIter = begin;
            return currIter;
        }

        if (currIter == end) {
            --currIter;
        }

        ElementListIterator iter = currIter;
        if (iter->GetIndex() == idx) {
            return iter;
        } else if (iter->GetIndex() > idx) {
            while (iter != begin && iter->GetIndex() > idx) {
                --iter;
            }
        } else {
            while (iter != end && iter->GetIndex() < idx) {
                ++iter;
            }
        }
        currIter = iter;
        return iter;
    }

    ElementListConstIterator LowerBoundForConst(unsigned idx) const
    {
        return LowerBoundForImpl(idx);
    }

    ElementListIterator LowerBoundFor(unsigned idx)
    {
        return LowerBoundForImpl(idx);
    }

    MapleAllocator allocator;
    ElementList elementList;
    mutable ElementListIterator currIter;
};
}  // namespace maple
#endif /* MAPLEBE_INCLUDE_CG_SPARSE_BITVECTOR_H */
