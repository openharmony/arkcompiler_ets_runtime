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

#ifndef MEMPOOL_INCLUDE_MEMPOOL_ALLOCATOR_H
#define MEMPOOL_INCLUDE_MEMPOOL_ALLOCATOR_H
#include <cstddef>
#include <limits>
#include <vector>
#include <deque>
#include <queue>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <forward_list>
#include "mempool.h"

namespace maple {
template <typename T>
class MapleAllocatorAdapter;  // circular dependency exists, no other choice
class MapleAllocator {
public:
    explicit MapleAllocator(MemPool *m) : memPool(m) {}
    virtual ~MapleAllocator() = default;

    // Get adapter for use in STL containers. See arena_containers.h .
    MapleAllocatorAdapter<void> Adapter();
    virtual void *Alloc(size_t bytes)
    {
        return (memPool != nullptr) ? memPool->Malloc(bytes) : nullptr;
    }

    template <class T, typename... Arguments>
    T *New(Arguments &&... args)
    {
        void *p = memPool->Malloc(sizeof(T));
        DEBUG_ASSERT(p != nullptr, "ERROR: New error");
        p = new (p) T(std::forward<Arguments>(args)...);
        return static_cast<T *>(p);
    }

    MemPool *GetMemPool() const
    {
        return memPool;
    }

    void SetMemPool(MemPool *m)
    {
        memPool = m;
    }

protected:
    MemPool *memPool;
    template <typename U>
    friend class MapleAllocatorAdapter;
};  // MapleAllocator

// must be destroyed, otherwise there will be memory leak
// only one allocater can be used at the same time
class LocalMapleAllocator : public MapleAllocator {
public:
    explicit LocalMapleAllocator(StackMemPool &m)
        : MapleAllocator(&m),
          fixedStackTopMark(m.fixedMemStackTop),
          bigStackTopMark(m.bigMemStackTop),
          fixedCurPtrMark(m.curPtr),
          bigCurPtrMark(m.bigCurPtr)
    {
        m.PushAllocator(this);
    }
    ~LocalMapleAllocator() override
    {
        static_cast<StackMemPool *>(memPool)->ResetStackTop(this, fixedCurPtrMark, fixedStackTopMark, bigCurPtrMark,
                                                            bigStackTopMark);
    };

    StackMemPool &GetStackMemPool() const
    {
        return static_cast<StackMemPool &>(*memPool);
    }

    MemPool *GetMemPool() const = delete;

    void *Alloc(size_t bytes) override
    {
        static_cast<StackMemPool *>(memPool)->CheckTopAllocator(this);
        return memPool->Malloc(bytes);
    }

private:
    using MapleAllocator::MapleAllocator;
    MemBlock *const fixedStackTopMark;
    MemBlock *const bigStackTopMark;
    uint8_t *const fixedCurPtrMark;
    uint8_t *const bigCurPtrMark;
};

template <typename T>
class MapleAllocatorAdapter;  // circular dependency exists, no other choice

template <typename T>
using MapleQueue = std::deque<T, MapleAllocatorAdapter<T>>;

template <typename T>
using MapleVector = std::vector<T, MapleAllocatorAdapter<T>>;

template <typename T>
class MapleStack {
public:
    using size_type = typename MapleVector<T>::size_type;
    using iterator = typename MapleVector<T>::iterator;

    MapleStack(MapleAllocatorAdapter<T> adapter) : vect(adapter) {}

    ~MapleStack() = default;

    MapleVector<T> &GetVector()
    {
        return vect;
    }

    bool empty() const
    {
        return vect.empty();
    }

    size_type size() const
    {
        return vect.size();
    }

    iterator begin()
    {
        return vect.begin();
    }

    iterator end()
    {
        return vect.end();
    }

    void push(T x)
    {
        vect.push_back(x);
    }

    void pop()
    {
        vect.pop_back();
    }

    T top()
    {
        return vect.back();
    }

    void clear()
    {
        vect.resize(0);
    }

private:
    MapleVector<T> vect;
};

template <typename T>
using MapleList = std::list<T, MapleAllocatorAdapter<T>>;

template <typename T>
using MapleForwardList = std::forward_list<T, MapleAllocatorAdapter<T>>;

template <typename T, typename Comparator = std::less<T>>
using MapleSet = std::set<T, Comparator, MapleAllocatorAdapter<T>>;

template <typename T, typename Hash = std::hash<T>, typename Equal = std::equal_to<T>>
using MapleUnorderedSet = std::unordered_set<T, Hash, Equal, MapleAllocatorAdapter<T>>;

template <typename K, typename V, typename Comparator = std::less<K>>
using MapleMap = std::map<K, V, Comparator, MapleAllocatorAdapter<std::pair<const K, V>>>;

template <typename K, typename V, typename Comparator = std::less<K>>
using MapleMultiMap = std::multimap<K, V, Comparator, MapleAllocatorAdapter<std::pair<const K, V>>>;

template <typename T, typename Comparator = std::less<T>>
using MapleMultiSet = std::multiset<T, Comparator, MapleAllocatorAdapter<T>>;

template <typename K, typename V, typename Hash = std::hash<K>, typename Equal = std::equal_to<K>>
using MapleUnorderedMap = std::unordered_map<K, V, Hash, Equal, MapleAllocatorAdapter<std::pair<const K, V>>>;

template <typename K, typename V, typename Hash = std::hash<K>, typename Equal = std::equal_to<K>>
using MapleUnorderedMultiMap = std::unordered_multimap<K, V, Hash, Equal, MapleAllocatorAdapter<std::pair<const K, V>>>;

// Implementation details below.
template <>
class MapleAllocatorAdapter<void> {
public:
    using value_type = void;
    using pointer = void *;
    using const_pointer = const void *;
    template <typename U>
    struct rebind {
        using other = MapleAllocatorAdapter<U>;
    };

    explicit MapleAllocatorAdapter(MapleAllocator &currMapleAllocator) : mapleAllocator(&currMapleAllocator) {}

    template <typename U>
    MapleAllocatorAdapter(const MapleAllocatorAdapter<U> &other) : mapleAllocator(other.mapleAllocator)
    {
    }

    MapleAllocatorAdapter(const MapleAllocatorAdapter &other) = default;
    MapleAllocatorAdapter &operator=(const MapleAllocatorAdapter &other) = default;
    ~MapleAllocatorAdapter() = default;

private:
    template <typename U>
    friend class MapleAllocatorAdapter;
    MapleAllocator *mapleAllocator;
};

template <typename T>
class MapleAllocatorAdapter {
public:
    using value_type = T;
    using pointer = T *;
    using reference = T &;
    using const_pointer = const T *;
    using const_reference = const T &;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    template <typename U>
    struct rebind {
        using other = MapleAllocatorAdapter<U>;
    };

    explicit MapleAllocatorAdapter(MapleAllocator &currMapleAllocator) : mapleAllocator(&currMapleAllocator) {}

    template <typename U>
    MapleAllocatorAdapter(const MapleAllocatorAdapter<U> &other) : mapleAllocator(other.mapleAllocator)
    {
    }

    MapleAllocatorAdapter(const MapleAllocatorAdapter &other) = default;
    MapleAllocatorAdapter &operator=(const MapleAllocatorAdapter &other) = default;
    ~MapleAllocatorAdapter() = default;
    size_type max_size() const
    {
        return static_cast<size_type>(-1) / sizeof(T);
    }

    pointer address(reference x) const
    {
        return &x;
    }

    const_pointer address(const_reference x) const
    {
        return &x;
    }

    pointer allocate(size_type n)
    {
        return reinterpret_cast<T *>(mapleAllocator->Alloc(n * sizeof(T)));
    }

    void deallocate(pointer, size_type) {}

    void construct(const pointer p, const_reference val)
    {
        new (static_cast<void *>(p)) value_type(val);
    }

    void destroy(const pointer p)
    {
        CHECK_NULL_FATAL(p);
        p->~value_type();
    }

private:
    template <typename U>
    friend class MapleAllocatorAdapter;
    template <typename U>
    friend bool operator==(const MapleAllocatorAdapter<U> &lhs, const MapleAllocatorAdapter<U> &rhs);
    MapleAllocator *mapleAllocator;
};

template <typename T>
inline bool operator==(const MapleAllocatorAdapter<T> &lhs, const MapleAllocatorAdapter<T> &rhs)
{
    return lhs.mapleAllocator == rhs.mapleAllocator;
}

template <typename T>
inline bool operator!=(const MapleAllocatorAdapter<T> &lhs, const MapleAllocatorAdapter<T> &rhs)
{
    return !(lhs == rhs);
}

inline MapleAllocatorAdapter<void> MapleAllocator::Adapter()
{
    return MapleAllocatorAdapter<void>(*this);
}
}  // namespace maple
#endif  // MEMPOOL_INCLUDE_MEMPOOL_ALLOCATOR_H
