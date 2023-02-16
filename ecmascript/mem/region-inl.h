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

#ifndef ECMASCRIPT_MEM_REGION_INL_H
#define ECMASCRIPT_MEM_REGION_INL_H

#include "ecmascript/mem/region.h"

#include "ecmascript/js_thread.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/native_area_allocator.h"

namespace panda::ecmascript {
inline RememberedSet *Region::CreateRememberedSet()
{
    auto bitSize = GCBitset::SizeOfGCBitset(GetCapacity());
    auto setAddr = thread_->GetNativeAreaAllocator()->Allocate(bitSize + RememberedSet::GCBITSET_DATA_OFFSET);
    auto ret = new (setAddr) RememberedSet(bitSize);
    ret->ClearAll();
    return ret;
}

inline RememberedSet *Region::GetOrCreateCrossRegionRememberedSet()
{
    if (UNLIKELY(crossRegionSet_ == nullptr)) {
        os::memory::LockHolder lock(*lock_);
        if (crossRegionSet_ == nullptr) {
            crossRegionSet_ = CreateRememberedSet();
        }
    }
    return crossRegionSet_;
}

inline RememberedSet *Region::GetOrCreateOldToNewRememberedSet()
{
    if (UNLIKELY(packedData_.oldToNewSet_ == nullptr)) {
        os::memory::LockHolder lock(*lock_);
        if (packedData_.oldToNewSet_ == nullptr) {
            if (sweepingRSet_ != nullptr && IsGCFlagSet(RegionGCFlags::HAS_BEEN_SWEPT)) {
                packedData_.oldToNewSet_ = sweepingRSet_;
                sweepingRSet_ = nullptr;
            } else {
                packedData_.oldToNewSet_ = CreateRememberedSet();
            }
        }
    }
    return packedData_.oldToNewSet_;
}

inline void Region::MergeRSetForConcurrentSweeping()
{
    if (sweepingRSet_ == nullptr) {
        return;
    }
    if (packedData_.oldToNewSet_ == nullptr) {
        packedData_.oldToNewSet_ = sweepingRSet_;
        sweepingRSet_ = nullptr;
    } else {
        packedData_.oldToNewSet_->Merge(sweepingRSet_);
        DeleteSweepingRSet();
        sweepingRSet_ = nullptr;
    }
}

inline GCBitset *Region::GetMarkGCBitset() const
{
    return packedData_.markGCBitset_;
}

inline bool Region::AtomicMark(void *address)
{
    auto addrPtr = reinterpret_cast<uintptr_t>(address);
    ASSERT(InRange(addrPtr));
    return packedData_.markGCBitset_->SetBit<AccessType::ATOMIC>(
        (addrPtr & DEFAULT_REGION_MASK) >> TAGGED_TYPE_SIZE_LOG);
}

inline void Region::ClearMark(void *address)
{
    auto addrPtr = reinterpret_cast<uintptr_t>(address);
    ASSERT(InRange(addrPtr));
    packedData_.markGCBitset_->ClearBit((addrPtr & DEFAULT_REGION_MASK) >> TAGGED_TYPE_SIZE_LOG);
}

inline bool Region::Test(void *addr) const
{
    auto addrPtr = reinterpret_cast<uintptr_t>(addr);
    ASSERT(InRange(addrPtr));
    return packedData_.markGCBitset_->TestBit((addrPtr & DEFAULT_REGION_MASK) >> TAGGED_TYPE_SIZE_LOG);
}

template <typename Visitor>
inline void Region::IterateAllMarkedBits(Visitor visitor) const
{
    packedData_.markGCBitset_->IterateMarkedBitsConst(
        reinterpret_cast<uintptr_t>(this), packedData_.bitsetSize_, visitor);
}

inline void Region::ClearMarkGCBitset()
{
    if (packedData_.markGCBitset_ != nullptr) {
        packedData_.markGCBitset_->Clear(packedData_.bitsetSize_);
    }
}

inline void Region::InsertCrossRegionRSet(uintptr_t addr)
{
    auto set = GetOrCreateCrossRegionRememberedSet();
    set->Insert(ToUintPtr(this), addr);
}

inline void Region::AtomicInsertCrossRegionRSet(uintptr_t addr)
{
    auto set = GetOrCreateCrossRegionRememberedSet();
    set->AtomicInsert(ToUintPtr(this), addr);
}

template <typename Visitor>
inline void Region::IterateAllCrossRegionBits(Visitor visitor) const
{
    if (crossRegionSet_ != nullptr) {
        crossRegionSet_->IterateAllMarkedBitsConst(ToUintPtr(this), visitor);
    }
}

inline void Region::ClearCrossRegionRSet()
{
    if (crossRegionSet_ != nullptr) {
        crossRegionSet_->ClearAll();
    }
}

inline void Region::ClearCrossRegionRSetInRange(uintptr_t start, uintptr_t end)
{
    if (crossRegionSet_ != nullptr) {
        crossRegionSet_->ClearRange(ToUintPtr(this), start, end);
    }
}

inline void Region::AtomicClearCrossRegionRSetInRange(uintptr_t start, uintptr_t end)
{
    if (crossRegionSet_ != nullptr) {
        crossRegionSet_->AtomicClearRange(ToUintPtr(this), start, end);
    }
}

inline void Region::DeleteCrossRegionRSet()
{
    if (crossRegionSet_ != nullptr) {
        thread_->GetNativeAreaAllocator()->Free(crossRegionSet_, crossRegionSet_->Size());
        crossRegionSet_ = nullptr;
    }
}

inline void Region::InsertOldToNewRSet(uintptr_t addr)
{
    auto set = GetOrCreateOldToNewRememberedSet();
    set->Insert(ToUintPtr(this), addr);
}

inline void Region::ClearOldToNewRSet(uintptr_t addr)
{
    auto set = GetOrCreateOldToNewRememberedSet();
    set->ClearBit(ToUintPtr(this), addr);
}

template <typename Visitor>
inline void Region::IterateAllOldToNewBits(Visitor visitor)
{
    if (packedData_.oldToNewSet_ != nullptr) {
        packedData_.oldToNewSet_->IterateAllMarkedBits(ToUintPtr(this), visitor);
    }
}

template <typename Visitor>
inline void Region::AtomicIterateAllSweepingRSetBits(Visitor visitor)
{
    if (sweepingRSet_ != nullptr) {
        sweepingRSet_->AtomicIterateAllMarkedBits(ToUintPtr(this), visitor);
    }
}

template <typename Visitor>
inline void Region::IterateAllSweepingRSetBits(Visitor visitor)
{
    if (sweepingRSet_ != nullptr) {
        sweepingRSet_->IterateAllMarkedBits(ToUintPtr(this), visitor);
    }
}

inline void Region::ClearOldToNewRSet()
{
    if (packedData_.oldToNewSet_ != nullptr) {
        packedData_.oldToNewSet_->ClearAll();
    }
}

inline void Region::ClearOldToNewRSetInRange(uintptr_t start, uintptr_t end)
{
    if (packedData_.oldToNewSet_ != nullptr) {
        packedData_.oldToNewSet_->ClearRange(ToUintPtr(this), start, end);
    }
}

inline void Region::DeleteOldToNewRSet()
{
    if (packedData_.oldToNewSet_ != nullptr) {
        thread_->GetNativeAreaAllocator()->Free(packedData_.oldToNewSet_, packedData_.oldToNewSet_->Size());
        packedData_.oldToNewSet_ = nullptr;
    }
}

inline void Region::AtomicClearSweepingRSetInRange(uintptr_t start, uintptr_t end)
{
    if (sweepingRSet_ != nullptr) {
        sweepingRSet_->AtomicClearRange(ToUintPtr(this), start, end);
    }
}

inline void Region::ClearSweepingRSetInRange(uintptr_t start, uintptr_t end)
{
    if (sweepingRSet_ != nullptr) {
        sweepingRSet_->ClearRange(ToUintPtr(this), start, end);
    }
}

inline void Region::DeleteSweepingRSet()
{
    if (sweepingRSet_ != nullptr) {
        thread_->GetNativeAreaAllocator()->Free(sweepingRSet_, sweepingRSet_->Size());
        sweepingRSet_ = nullptr;
    }
}

inline bool Region::IsMarking() const
{
    return !thread_->IsReadyToMark();
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_REGION_INL_H
