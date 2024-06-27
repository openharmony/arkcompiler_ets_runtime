/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "ecmascript/mem/free_object_set.h"
#include "ecmascript/mem/free_object_list.h"
#include "ecmascript/mem/mem.h"

namespace panda::ecmascript {
#ifdef ENABLE_JITFORT
template <typename T>
FreeObjectList<T>::FreeObjectList() : sets_(new FreeObjectSet<T> *[NUMBER_OF_SETS](), NUMBER_OF_SETS),
    lastSets_(new FreeObjectSet<T> *[NUMBER_OF_SETS](), NUMBER_OF_SETS)
#else
FreeObjectList::FreeObjectList() : sets_(new FreeObjectSet *[NUMBER_OF_SETS](), NUMBER_OF_SETS),
    lastSets_(new FreeObjectSet *[NUMBER_OF_SETS](), NUMBER_OF_SETS)
#endif
{
    for (int i = 0; i < NUMBER_OF_SETS; i++) {
        sets_[i] = nullptr;
        lastSets_[i] = nullptr;
    }
    noneEmptySetBitMap_ = 0;
}
#ifdef ENABLE_JITFORT
template FreeObjectList<FreeObject>::FreeObjectList();
template FreeObjectList<MemDesc>::FreeObjectList();

template <>
FreeObjectList<MemDesc>::FreeObjectList(JitFort *fort)
    : sets_(new FreeObjectSet<MemDesc> *[NUMBER_OF_SETS](), NUMBER_OF_SETS),
    lastSets_(new FreeObjectSet<MemDesc> *[NUMBER_OF_SETS](), NUMBER_OF_SETS),
    jitFort_(fort)
{
    for (int i = 0; i < NUMBER_OF_SETS; i++) {
        sets_[i] = nullptr;
        lastSets_[i] = nullptr;
    }
    noneEmptySetBitMap_ = 0;
}
#endif

#ifdef ENABLE_JITFORT
template <typename T>
FreeObjectList<T>::~FreeObjectList()
#else
FreeObjectList::~FreeObjectList()
#endif
{
    delete[] sets_.data();
    delete[] lastSets_.data();
    noneEmptySetBitMap_ = 0;
}
#ifdef ENABLE_JITFORT
template FreeObjectList<FreeObject>::~FreeObjectList();
template FreeObjectList<MemDesc>::~FreeObjectList();
#endif

#ifdef ENABLE_JITFORT
template <typename T>
T *FreeObjectList<T>::Allocate(size_t size)
#else
FreeObject *FreeObjectList::Allocate(size_t size)
#endif
{
    if (noneEmptySetBitMap_ == 0) {
        return nullptr;
    }
    // find from suitable
    SetType type = SelectSetType(size);
#ifdef ENABLE_JITFORT
    if (type == FreeObjectSet<T>::INVALID_SET_TYPE)
#else
    if (type == FreeObjectSet::INVALID_SET_TYPE)
#endif
    {
        return nullptr;
    }

    SetType lastType = type - 1;
    for (type = static_cast<int32_t>(CalcNextNoneEmptyIndex(type)); type > lastType && type < NUMBER_OF_SETS;
        type = static_cast<int32_t>(CalcNextNoneEmptyIndex(type + 1))) {
        lastType = type;
#ifdef ENABLE_JITFORT
        FreeObjectSet<T> *current = sets_[type];
#else
        FreeObjectSet *current = sets_[type];
#endif
        while (current != nullptr) {
            if (current->Available() < size) {
                current = current->next_;
                continue;
            }
#ifdef ENABLE_JITFORT
            FreeObjectSet<T> *next = nullptr;
            T *object = T::Cast(INVALID_OBJPTR);
#else
            FreeObjectSet *next = nullptr;
            FreeObject *object = INVALID_OBJECT;
#endif
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
#ifdef ENABLE_JITFORT
            if (object != T::Cast(INVALID_OBJPTR))
#else
            if (object != INVALID_OBJECT)
#endif
            {
                available_ -= object->Available();
                return object;
            }
            current = next;
        }
    }
    return nullptr;
}
#ifdef ENABLE_JITFORT
template FreeObject *FreeObjectList<FreeObject>::Allocate(size_t size);
template MemDesc *FreeObjectList<MemDesc>::Allocate(size_t size);
#endif

#ifdef ENABLE_JITFORT
template <typename T>
T *FreeObjectList<T>::LookupSuitableFreeObject(size_t size)
#else
FreeObject *FreeObjectList::LookupSuitableFreeObject(size_t size)
#endif
{
    if (noneEmptySetBitMap_ == 0) {
        return nullptr;
    }
    // find a suitable type
    SetType type = SelectSetType(size);
#ifdef ENABLE_JITFORT
    if (type == FreeObjectSet<T>::INVALID_SET_TYPE)
#else
    if (type == FreeObjectSet::INVALID_SET_TYPE)
#endif
    {
        return nullptr;
    }

    SetType lastType = type - 1;
    for (type = static_cast<int32_t>(CalcNextNoneEmptyIndex(type)); type > lastType && type < NUMBER_OF_SETS;
        type = static_cast<int32_t>(CalcNextNoneEmptyIndex(type + 1))) {
        lastType = type;
#ifdef ENABLE_JITFORT
        FreeObjectSet<T> *current = sets_[type];
#else
        FreeObjectSet *current = sets_[type];
#endif
        while (current != nullptr) {
#ifdef ENABLE_JITFORT
            FreeObjectSet<T> *next = nullptr;
            T *object = INVALID_OBJECT;
#else
            FreeObjectSet *next = nullptr;
            FreeObject *object = INVALID_OBJECT;
#endif
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
#ifdef ENABLE_JITFORT
template FreeObject *FreeObjectList<FreeObject>::LookupSuitableFreeObject(size_t);
#endif

#ifdef ENABLE_JITFORT
template <typename T>
void FreeObjectList<T>::Free(uintptr_t start, size_t size, bool isAdd)
#else
void FreeObjectList::Free(uintptr_t start, size_t size, bool isAdd)
#endif
{
    if (UNLIKELY(start == 0)) {
        return;
    }
    if (UNLIKELY(size < MIN_SIZE)) {
        Region *region = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(start));
        region->IncreaseWasted(size);
        if (isAdd) {
            wasted_ += size;
        }
        return;
    }
    SetType type = SelectSetType(size);
#ifdef ENABLE_JITFORT
    if (type == FreeObjectSet<T>::INVALID_SET_TYPE)
#else
    if (type == FreeObjectSet::INVALID_SET_TYPE)
#endif
    {
        return;
    }

    Region *region = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(start));
    auto set = region->GetFreeObjectSet(type);
    if (set == nullptr) {
        LOG_FULL(FATAL) << "The set of region is nullptr";
        return;
    }
    set->Free(start, size);

    if (isAdd) {
        if (!set->isAdded_) {
            AddSet(set);
        } else {
            available_ += size;
        }
    }
}

#ifdef ENABLE_JITFORT
// template class instance for non JitFort space uses FreeObject and Region.
template void FreeObjectList<FreeObject>::Free(uintptr_t, size_t, bool);
// template class instance for JitFort space uses MemDesc and JitFortRegion
template <>
void FreeObjectList<MemDesc>::Free(uintptr_t start, size_t size, bool isAdd)
{
    if (UNLIKELY(start == 0)) {
        return;
    }
    if (UNLIKELY(size < MIN_SIZE)) {
        JitFortRegion *region = jitFort_->ObjectAddressToRange(start);
        region->IncreaseWasted(size);
        if (isAdd) {
            wasted_ += size;
        }
        return;
    }
    SetType type = SelectSetType(size);
    if (type == FreeObjectSet<MemDesc>::INVALID_SET_TYPE) {
        return;
    }

    JitFortRegion *region = jitFort_->ObjectAddressToRange(start);
    auto set = region->GetFreeObjectSet(type);
    if (set == nullptr) {
        LOG_FULL(FATAL) << "The set of region is nullptr";
        return;
    }
    set->Free(start, size);

    if (isAdd) {
        if (!set->isAdded_) {
            AddSet(set);
        } else {
            available_ += size;
        }
    }
}
#endif

#ifdef ENABLE_JITFORT
template <typename T>
void FreeObjectList<T>::Rebuild()
#else
void FreeObjectList::Rebuild()
#endif
{
#ifdef ENABLE_JITFORT
    EnumerateSets([](FreeObjectSet<T> *set) { set->Rebuild(); });
#else
    EnumerateSets([](FreeObjectSet *set) { set->Rebuild(); });
#endif
    for (int i = 0; i < NUMBER_OF_SETS; i++) {
        sets_[i] = nullptr;
        lastSets_[i] = nullptr;
    }
    available_ = 0;
    wasted_ = 0;
    noneEmptySetBitMap_ = 0;
}
#ifdef ENABLE_JITFORT
template void FreeObjectList<FreeObject>::Rebuild();
template void FreeObjectList<MemDesc>::Rebuild();
#endif

#ifdef ENABLE_JITFORT
template <typename T>
bool FreeObjectList<T>::MatchFreeObjectInSet(FreeObjectSet<T> *set, size_t size)
#else
bool FreeObjectList::MatchFreeObjectInSet(FreeObjectSet *set, size_t size)
#endif
{
    if (set == nullptr || set->Empty()) {
        return false;
    }
    // find a suitable type
    SetType type = SelectSetType(size);
#ifdef ENABLE_JITFORT
    if (type == FreeObjectSet<T>::INVALID_SET_TYPE)
#else
    if (type == FreeObjectSet::INVALID_SET_TYPE)
#endif
    {
        return false;
    }

#ifdef ENABLE_JITFORT
    T *object = nullptr;
#else
    FreeObject *object = nullptr;
#endif
    if (type <= SMALL_SET_MAX_INDEX) {
        object = set->LookupSmallFreeObject(size);
    } else {
        object = set->LookupLargeFreeObject(size);
    }
    return object != nullptr;
}
#ifdef ENABLE_JITFORT
template bool FreeObjectList<FreeObject>::MatchFreeObjectInSet(FreeObjectSet<FreeObject> *, size_t);
#endif

#ifdef ENABLE_JITFORT
template <typename T>
bool FreeObjectList<T>::AddSet(FreeObjectSet<T> *set)
#else
bool FreeObjectList::AddSet(FreeObjectSet *set)
#endif
{
    if (set == nullptr || set->Empty() || set->isAdded_) {
        return false;
    }
    SetType type = set->setType_;
#ifdef ENABLE_JITFORT
    FreeObjectSet<T> *top = sets_[type];
#else
    FreeObjectSet *top = sets_[type];
#endif
    if (set == top) {
        return false;
    }
    if (top != nullptr) {
        top->prev_ = set;
    }
    set->isAdded_ = true;
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
#ifdef ENABLE_JITFORT
template bool FreeObjectList<FreeObject>::AddSet(FreeObjectSet<FreeObject> *);
template bool FreeObjectList<MemDesc>::AddSet(FreeObjectSet<MemDesc> *);
#endif

#ifdef ENABLE_JITFORT
template <typename T>
void FreeObjectList<T>::RemoveSet(FreeObjectSet<T> *set)
#else
void FreeObjectList::RemoveSet(FreeObjectSet *set)
#endif
{
    if (set == nullptr) {
        return;
    }
    SetType type = set->setType_;
#ifdef ENABLE_JITFORT
    FreeObjectSet<T> *top = sets_[type];
    FreeObjectSet<T> *end = lastSets_[type];
#else
    FreeObjectSet *top = sets_[type];
    FreeObjectSet *end = lastSets_[type];
#endif
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
    set->isAdded_ = false;
    set->prev_ = nullptr;
    set->next_ = nullptr;
    if (sets_[type] == nullptr) {
        ClearNoneEmptyBit(type);
    }
    available_ -= set->Available();
}
#ifdef ENABLE_JITFORT
template void FreeObjectList<FreeObject>::RemoveSet(FreeObjectSet<FreeObject> *);
#endif

#ifdef ENABLE_JITFORT
template <typename T>
void FreeObjectList<T>::Merge(FreeObjectList<T> *list)
#else
void FreeObjectList::Merge(FreeObjectList *list)
#endif
{
#ifdef ENABLE_JITFORT
    list->EnumerateTopAndLastSets([this](FreeObjectSet<T> *set, FreeObjectSet<T> *end)
#else
    list->EnumerateTopAndLastSets([this](FreeObjectSet *set, FreeObjectSet *end)
#endif
    {
        if (set == nullptr || set->Empty()) {
            return;
        }
        SetType type = set->setType_;
#ifdef ENABLE_JITFORT
        FreeObjectSet<T> *top = sets_[type];
#else
        FreeObjectSet *top = sets_[type];
#endif
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

#ifdef ENABLE_JITFORT
template<typename T>
template<class Callback>
void FreeObjectList<T>::EnumerateSets(const Callback &cb) const
#else
template<class Callback>
void FreeObjectList::EnumerateSets(const Callback &cb) const
#endif
{
    for (SetType i = 0; i < NUMBER_OF_SETS; i++) {
        EnumerateSets(i, cb);
    }
}

#ifdef ENABLE_JITFORT
template<typename T>
template<class Callback>
void FreeObjectList<T>::EnumerateSets(SetType type, const Callback &cb) const
#else
template<class Callback>
void FreeObjectList::EnumerateSets(SetType type, const Callback &cb) const
#endif
{
#ifdef ENABLE_JITFORT
    FreeObjectSet<T> *current = sets_[type];
#else
    FreeObjectSet *current = sets_[type];
#endif
    while (current != nullptr) {
        // maybe reset
#ifdef ENABLE_JITFORT
        FreeObjectSet<T> *next = current->next_;
#else
        FreeObjectSet *next = current->next_;
#endif
        cb(current);
        current = next;
    }
}

#ifdef ENABLE_JITFORT
template<typename T>
template<class Callback>
void FreeObjectList<T>::EnumerateTopAndLastSets(const Callback &cb) const
#else
template<class Callback>
void FreeObjectList::EnumerateTopAndLastSets(const Callback &cb) const
#endif
{
    for (SetType i = 0; i < NUMBER_OF_SETS; i++) {
        cb(sets_[i], lastSets_[i]);
    }
}
}  // namespace panda::ecmascript
