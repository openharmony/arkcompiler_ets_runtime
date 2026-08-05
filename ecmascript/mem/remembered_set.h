/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_REMEMBERED_SET_H
#define ECMASCRIPT_MEM_REMEMBERED_SET_H

#include "ecmascript/mem/gc_bitset.h"

namespace panda::ecmascript {

template <ReferenceType refType>
class RememberedSetBase {
    using BitSet = GCBitsetBase<refType>;
    static constexpr size_t TYPE_SIZE = BitSet::TYPE_SIZE;
    static constexpr size_t TYPE_SIZE_LOG = BitSet::TYPE_SIZE_LOG;
public:
    static constexpr size_t GCBITSET_DATA_OFFSET = sizeof(size_t);
    explicit RememberedSetBase(size_t size) : size_(size) {}

    NO_COPY_SEMANTIC(RememberedSetBase);
    NO_MOVE_SEMANTIC(RememberedSetBase);

    BitSet *GCBitsetData()
    {
        return reinterpret_cast<BitSet *>(reinterpret_cast<uintptr_t>(this) + GCBITSET_DATA_OFFSET);
    }

    const BitSet *GCBitsetData() const
    {
        return reinterpret_cast<BitSet *>(reinterpret_cast<uintptr_t>(this) + GCBITSET_DATA_OFFSET);
    }

    void ClearAll()
    {
        GCBitsetData()->Clear(size_);
    }

    bool Insert(uintptr_t begin, uintptr_t addr)
    {
        return GCBitsetData()->template SetBit<AccessType::NON_ATOMIC>((addr - begin) >> TYPE_SIZE_LOG);
    }

    template <typename Dummy = void,
              typename = std::enable_if_t<!ReferenceIsCompressed<refType>, Dummy>>
    bool InsertRange(uintptr_t begin, uintptr_t addr, uint32_t mask)
    {
        return GCBitsetData()->SetBitRange((addr - begin) >> TYPE_SIZE_LOG, mask);
    }

    bool AtomicInsert(uintptr_t begin, uintptr_t addr)
    {
        return GCBitsetData()->template SetBit<AccessType::ATOMIC>((addr - begin) >> TYPE_SIZE_LOG);
    }

    void ClearBit(uintptr_t begin, uintptr_t addr)
    {
        GCBitsetData()->ClearBit((addr - begin) >> TYPE_SIZE_LOG);
    }

    void ClearRange(uintptr_t begin, uintptr_t start, uintptr_t end)
    {
        GCBitsetData()->template ClearBitRange<AccessType::NON_ATOMIC>(
            (start - begin) >> TYPE_SIZE_LOG, (end - begin) >> TYPE_SIZE_LOG);
    }

    void AtomicClearRange(uintptr_t begin, uintptr_t start, uintptr_t end)
    {
        GCBitsetData()->template ClearBitRange<AccessType::ATOMIC>(
            (start - begin) >> TYPE_SIZE_LOG, (end - begin) >> TYPE_SIZE_LOG);
    }

    bool TestBit(uintptr_t begin, uintptr_t addr) const
    {
        return GCBitsetData()->TestBit((addr - begin) >> TYPE_SIZE_LOG);
    }

    template <typename Visitor>
    void IterateAllMarkedBits(uintptr_t begin, Visitor &&visitor)
    {
        GCBitsetData()->template IterateMarkedBits<Visitor, AccessType::NON_ATOMIC>(begin, size_, visitor);
    }

    template <typename Visitor>
    void AtomicIterateAllMarkedBits(uintptr_t begin, Visitor &&visitor)
    {
        GCBitsetData()->template IterateMarkedBits<Visitor, AccessType::ATOMIC>(begin, size_, visitor);
    }

    template <typename Visitor>
    void IterateAllMarkedBitsConst(uintptr_t begin, Visitor &&visitor) const
    {
        GCBitsetData()->IterateMarkedBitsConst(begin, size_, visitor);
    }

    void Merge(RememberedSetBase *rset)
    {
        BitSet *bitset = rset->GCBitsetData();
        GCBitsetData()->Merge(bitset, size_);
    }

    size_t Size() const
    {
        return size_ + GCBITSET_DATA_OFFSET;
    }

private:
    size_t size_;
};

using RememberedSet = RememberedSetBase<ReferenceType::NORMAL>;
using CompressedRememberedSet = RememberedSetBase<ReferenceType::COMPRESSED>;
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_REMEMBERED_SET_H
