/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/mem/free_object_list.h"

#include "ecmascript/free_object.h"
#include "ecmascript/mem/jit_fort.h"

namespace panda::ecmascript {
template <typename T>
FreeObjectList<T>::FreeObjectList(JitFort * fort) : sets_(new FreeObjectSet<T> *[NUMBER_OF_SETS](), NUMBER_OF_SETS),
    lastSets_(new FreeObjectSet<T> *[NUMBER_OF_SETS](), NUMBER_OF_SETS), jitFort_(fort)
{
    for (int i = 0; i < NUMBER_OF_SETS; i++) {
        sets_[i] = nullptr;
        lastSets_[i] = nullptr;
    }
    noneEmptySetBitMap_ = 0;
}
template FreeObjectList<FreeObject>::FreeObjectList(JitFort* fort);
template FreeObjectList<MemDesc>::FreeObjectList(JitFort* fort);

template <typename T>
FreeObjectList<T>::~FreeObjectList()
{
    delete[] sets_.data();
    delete[] lastSets_.data();
    noneEmptySetBitMap_ = 0;
}
template FreeObjectList<FreeObject>::~FreeObjectList();
template FreeObjectList<MemDesc>::~FreeObjectList();

template <typename T>
T *FreeObjectList<T>::Allocate(size_t size)
{
    if (noneEmptySetBitMap_ == 0) {
        return nullptr;
    }
    // find from suitable
    SetType type = SelectSetType(size);
    if (type == FreeObjectSet<T>::INVALID_SET_TYPE) {
        return nullptr;
    }

    SetType lastType = type - 1;
    for (type = static_cast<int32_t>(CalcNextNoneEmptyIndex(type)); type > lastType && type < NUMBER_OF_SETS;
        type = static_cast<int32_t>(CalcNextNoneEmptyIndex(type + 1))) {
        lastType = type;
        FreeObjectSet<T> *current = sets_[type];
        while (current != nullptr) {
            if (current->Available() < size || size > current->MaxAvailableFreeSize()) {
                current = current->next_;
                continue;
            }
            FreeObjectSet<T> *next = nullptr;
            T *object = T::Cast(INVALID_OBJPTR);
            if (type <= SMALL_SET_MAX_INDEX) {
                object = current->ObtainSmallFreeObject(size);
            } else {
                next = current->next_;
                object = current->ObtainLargeFreeObject(size);
            }
            if (current->Empty()) {
                RemoveSet(current);
                current->Rebuild();
            }
            if (object != T::Cast(INVALID_OBJPTR)) {
                available_ -= object->Available();
                return object;
            }
            current = next;
        }
    }
    return nullptr;
}
template FreeObject *FreeObjectList<FreeObject>::Allocate(size_t size);
template MemDesc *FreeObjectList<MemDesc>::Allocate(size_t size);

template <typename T>
T *FreeObjectList<T>::LookupSuitableFreeObject(size_t size)
{
    if (noneEmptySetBitMap_ == 0) {
        return nullptr;
    }
    // find a suitable type
    SetType type = SelectSetType(size);
    if (type == FreeObjectSet<T>::INVALID_SET_TYPE) {
        return nullptr;
    }

    SetType lastType = type - 1;
    for (type = static_cast<int32_t>(CalcNextNoneEmptyIndex(type)); type > lastType && type < NUMBER_OF_SETS;
        type = static_cast<int32_t>(CalcNextNoneEmptyIndex(type + 1))) {
        lastType = type;
        FreeObjectSet<T> *current = sets_[type];
        while (current != nullptr) {
            FreeObjectSet<T> *next = nullptr;
            T *object = INVALID_OBJECT;
            if (type <= SMALL_SET_MAX_INDEX) {
                object = current->LookupSmallFreeObject(size);
            } else {
                next = current->next_;
                object = current->LookupLargeFreeObject(size);
            }
            if (object != INVALID_OBJECT) {
                return object;
            }
            current = next;
        }
    }
    return nullptr;
}
template FreeObject *FreeObjectList<FreeObject>::LookupSuitableFreeObject(size_t);

template <typename T>
template <typename U>
void FreeObjectList<T>::FreeImpl(U* region, uintptr_t start, size_t size, bool isAdd)
{
    if (UNLIKELY(start == 0)) { // LCOV_EXCL_BR_LINE
        return;
    }
    if (UNLIKELY(size < MIN_SIZE)) { // LCOV_EXCL_BR_LINE
        region->IncreaseWasted(size);
        if (isAdd) {
            wasted_ += size;
        }
        return;
    }

    SetType type = SelectSetType(size);
    if (type == FreeObjectSet<T>::INVALID_SET_TYPE) {
        return;
    }

    auto set = region->GetFreeObjectSet(type);
    if (set == nullptr) { // LCOV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "The set of region is nullptr";
        return;
    }
    set->Free(start, size);

    if (isAdd) {
        if (set->isAdded_ == 0) {
            AddSet(set);
        } else {
            available_ += size;
        }
    }
}
template void FreeObjectList<FreeObject>::FreeImpl<Region>(Region* region, uintptr_t start, size_t size, bool isAdd);
template void FreeObjectList<MemDesc>::FreeImpl<JitFortRegion>(JitFortRegion* region,
    uintptr_t start, size_t size, bool isAdd);

template <typename T>
void FreeObjectList<T>::Free(uintptr_t start, size_t size, bool isAdd)
{
    return FreeImpl<Region>(Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(start)), start, size, isAdd);
}

// template class instance for non JitFort space uses FreeObject and Region.
template void FreeObjectList<FreeObject>::Free(uintptr_t, size_t, bool);
// template class instance for JitFort space uses MemDesc and JitFortRegion
template <>
void FreeObjectList<MemDesc>::Free(uintptr_t start, size_t size, bool isAdd)
{
    return FreeImpl<JitFortRegion>(jitFort_->ObjectAddressToRange(start), start, size, isAdd);
}

template <typename T>
void FreeObjectList<T>::Rebuild()
{
    EnumerateSets([](FreeObjectSet<T> *set) { set->Rebuild(); });
    for (int i = 0; i < NUMBER_OF_SETS; i++) {
        sets_[i] = nullptr;
        lastSets_[i] = nullptr;
    }
    available_ = 0;
    wasted_ = 0;
    noneEmptySetBitMap_ = 0;
}
template void FreeObjectList<FreeObject>::Rebuild();
template void FreeObjectList<MemDesc>::Rebuild();

template <typename T>
bool FreeObjectList<T>::MatchFreeObjectInSet(FreeObjectSet<T> *set, size_t size)
{
    if (set == nullptr || set->Empty()) {
        return false;
    }
    // find a suitable type
    SetType type = SelectSetType(size);
    if (type == FreeObjectSet<T>::INVALID_SET_TYPE) {
        return false;
    }

    T *object = nullptr;
    if (type <= SMALL_SET_MAX_INDEX) {
        object = set->LookupSmallFreeObject(size);
    } else {
        object = set->LookupLargeFreeObject(size);
    }
    return object != nullptr;
}
template bool FreeObjectList<FreeObject>::MatchFreeObjectInSet(FreeObjectSet<FreeObject> *, size_t);

template <typename T>
bool FreeObjectList<T>::AddSet(FreeObjectSet<T> *set)
{
    if (set == nullptr || set->Empty() || set->isAdded_ != 0) {
        return false;
    }
    SetType type = set->setType_;
    FreeObjectSet<T> *top = sets_[type];
    if (set == top) {
        return false;
    }
    if (top != nullptr) {
        top->prev_ = set;
    }
    set->isAdded_ = 1; // 1: added
    set->next_ = top;
    set->prev_ = nullptr;
    if (lastSets_[type] == nullptr) {
        lastSets_[type] = set;
    }
    if (sets_[type] == nullptr) {
        SetNoneEmptyBit(type);
    }
    sets_[type] = set;
    available_ += set->Available();
    return true;
}
template bool FreeObjectList<FreeObject>::AddSet(FreeObjectSet<FreeObject> *);
template bool FreeObjectList<MemDesc>::AddSet(FreeObjectSet<MemDesc> *);

template <typename T>
void FreeObjectList<T>::RemoveSet(FreeObjectSet<T> *set)
{
    if (set == nullptr) {
        return;
    }
    SetType type = set->setType_;
    FreeObjectSet<T> *top = sets_[type];
    FreeObjectSet<T> *end = lastSets_[type];
    if (top == set) {
        sets_[type] = top->next_;
    }
    if (end == set) {
        lastSets_[type] = end->prev_;
    }
    if (set->prev_ != nullptr) {
        set->prev_->next_ = set->next_;
    }
    if (set->next_ != nullptr) {
        set->next_->prev_ = set->prev_;
    }
    set->isAdded_ = 0;
    set->prev_ = nullptr;
    set->next_ = nullptr;
    if (sets_[type] == nullptr) {
        ClearNoneEmptyBit(type);
    }
    available_ -= set->Available();
}
template void FreeObjectList<FreeObject>::RemoveSet(FreeObjectSet<FreeObject> *);

template <typename T>
void FreeObjectList<T>::Merge(FreeObjectList<T> *list)
{
    list->EnumerateTopAndLastSets([this](FreeObjectSet<T> *set, FreeObjectSet<T> *end) {
        if (set == nullptr || set->Empty()) {
            return;
        }
        SetType type = set->setType_;
        FreeObjectSet<T> *top = sets_[type];
        if (top == nullptr) {
            top = set;
        } else {
            lastSets_[type]->next_ = set;
            set->prev_ = lastSets_[type];
        }
        lastSets_[type] = end;
        SetNoneEmptyBit(type);
    });
    available_ += list->available_;
    list->Rebuild();
}

template<typename T>
template<class Callback>
void FreeObjectList<T>::EnumerateSets(const Callback &cb) const
{
    for (SetType i = 0; i < NUMBER_OF_SETS; i++) {
        EnumerateSets(i, cb);
    }
}

template<typename T>
template<class Callback>
void FreeObjectList<T>::EnumerateSets(SetType type, const Callback &cb) const
{
    FreeObjectSet<T> *current = sets_[type];
    while (current != nullptr) {
        // maybe reset
        FreeObjectSet<T> *next = current->next_;
        cb(current);
        current = next;
    }
}

template<typename T>
template<class Callback>
void FreeObjectList<T>::EnumerateTopAndLastSets(const Callback &cb) const
{
    for (SetType i = 0; i < NUMBER_OF_SETS; i++) {
        cb(sets_[i], lastSets_[i]);
    }
}
}  // namespace panda::ecmascript
