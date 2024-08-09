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

#ifndef MEMPOOL_INCLUDE_MEMPOOL_H
#define MEMPOOL_INCLUDE_MEMPOOL_H
#include <list>
#include <set>
#include <forward_list>
#include <unordered_set>
#include <stack>
#include <map>
#include <string>
#include <mutex>
#include <memory>
#include "mir_config.h"
#include "mpl_logging.h"

namespace maple {
#define BITS_ALIGN(size) (((size) + 7) & (0xFFFFFFF8))

constexpr size_t kMemBlockSizeMin = 2 * 1024;
constexpr size_t kMemBlockMalloc = 1024 * 1024;
static_assert((kMemBlockMalloc > kMemBlockSizeMin) && ((kMemBlockMalloc % kMemBlockSizeMin) == 0), "mempool error");

struct MemBlock {
    MemBlock(uint8_t *startPtr, size_t size) : startPtr(startPtr), memSize(size) {}
    ~MemBlock() = default;

    uint8_t *EndPtr() const
    {
        return startPtr + memSize;
    }

    uint8_t *startPtr = nullptr;
    size_t memSize = 0;
    MemBlock *nextMemBlock = nullptr;
};

// Class declaration
class MemPool;
class StackMemPool;
class MemPoolCtrler;
extern MemPoolCtrler memPoolCtrler;

// memory backend
class SysMemoryManager {
public:
    virtual ~SysMemoryManager() = default;
    virtual uint8_t *RealAllocMemory(size_t size) = 0;
    virtual void ReleaseMemory() = 0;
};

class MallocSysMemoryManager : public SysMemoryManager {
public:
    uint8_t *RealAllocMemory(size_t size) override
    {
        if (size == 0) {
            return nullptr;
        }
        void *block = malloc(size);
        CHECK_FATAL(block != nullptr, "malloc failed");

        mallocMemories.push_front(block);
        return reinterpret_cast<uint8_t *>(block);
    }
    ~MallocSysMemoryManager() override
    {
        for (void *ptr : mallocMemories) {
            free(ptr);
        }
        mallocMemories.clear();
    }
    void ReleaseMemory() override
    {
        for (void *ptr : mallocMemories) {
            free(ptr);
        }
        mallocMemories.clear();
    }
    std::forward_list<void *> mallocMemories;
};

// memory middle end
class MemPoolCtrler {
    friend MemPool;

public:
    static bool freeMemInTime;
    MemPoolCtrler() : sysMemoryMgr(new MallocSysMemoryManager()) {}

    ~MemPoolCtrler();

    MemPool *NewMemPool(const std::string &, bool isLocalPool);
    void DeleteMemPool(MemPool *memPool) const;

    MemBlock *AllocMemBlock(const MemPool &pool, size_t size);
    MemBlock *AllocFixMemBlock(const MemPool &pool);
    MemBlock *AllocBigMemBlock(const MemPool &pool, size_t size) const;
    void FreeFixedSizeMemBlockMemory();

private:
    struct MemBlockCmp {
        bool operator()(const MemBlock *l, const MemBlock *r) const
        {
            return l->memSize > r->memSize;
        }
    };

    void FreeMem();
    void FreeMemBlocks(const MemPool &pool, MemBlock *fixedMemHead, MemBlock *bigMemHead);

    std::mutex ctrlerMutex;  // this mutex is used to protect memPools
    MemBlock *fixedFreeMemBlocks = nullptr;
    std::unique_ptr<SysMemoryManager> sysMemoryMgr;
};

#ifdef MP_DEUG
class MemPoolStat {
public:
    ~MemPoolStat() = default;

protected:
    void SetName(const std::string &name)
    {
        this->name = name;
    }
    void SetName(const char *name)
    {
        this->name = name;
    }
    std::string name;
};
#else
class MemPoolStat {
public:
    virtual ~MemPoolStat() = default;

protected:
    void SetName(const std::string & /* name */) const {}
    void SetName(const char /* name */) const {}
};
#endif

// memory front end
class MemPool : private MemPoolStat {
    friend MemPoolCtrler;

public:
    MemPool(MemPoolCtrler &ctl, const std::string &name) : ctrler(ctl)
    {
        SetName(name);
    }
    MemPool(MemPoolCtrler &ctl, const char *name) : ctrler(ctl)
    {
        SetName(name);
    }

    ~MemPool();

    virtual void *Malloc(size_t size);
    void *Calloc(size_t size);
    void *Realloc(const void *ptr, size_t oldSize, size_t newSize);
    virtual void ReleaseContainingMem();

    MemPoolCtrler &GetCtrler()
    {
        return ctrler;
    }

    const MemPoolCtrler &GetCtrler() const
    {
        return ctrler;
    }

    template <class T>
    T *Clone(const T &t)
    {
        void *p = Malloc(sizeof(T));
        DEBUG_ASSERT(p != nullptr, "ERROR: New error");
        p = new (p) T(t);
        return static_cast<T *>(p);
    }

    template <class T, typename... Arguments>
    T *New(Arguments &&... args)
    {
        void *p = Malloc(sizeof(T));
        DEBUG_ASSERT(p != nullptr, "ERROR: New error");
        p = new (p) T(std::forward<Arguments>(args)...);
        return static_cast<T *>(p);
    }

    template <class T>
    T *NewArray(size_t num)
    {
        void *p = Malloc(sizeof(T) * num);
        DEBUG_ASSERT(p != nullptr, "ERROR: NewArray error");
        p = new (p) T[num];
        return static_cast<T *>(p);
    }

protected:
    MemPoolCtrler &ctrler;  // Hookup controller object
    uint8_t *endPtr = nullptr;
    uint8_t *curPtr = nullptr;
    MemBlock *fixedMemHead = nullptr;
    MemBlock *bigMemHead = nullptr;

    uint8_t *AllocNewMemBlock(size_t bytes);
};

using ThreadLocalMemPool = MemPool;

class ThreadShareMemPool : public MemPool {
public:
    using MemPool::MemPool;
    virtual ~ThreadShareMemPool() = default;
    void *Malloc(size_t size) override
    {
        return MemPool::Malloc(size);
    }
    void ReleaseContainingMem() override
    {
        MemPool::ReleaseContainingMem();
    }
};

class LocalMapleAllocator;
#ifdef MP_DEBUG
class StackMemPoolDebug {
protected:
    void PushAllocator(const LocalMapleAllocator *alloc)
    {
        allocators.push(alloc);
    }
    void CheckTopAllocator(const LocalMapleAllocator *alloc) const
    {
        CHECK_FATAL(alloc == allocators.top(), "only top allocator allowed");
    }
    void PopAllocator()
    {
        allocators.pop();
    }
    std::stack<const LocalMapleAllocator *> allocators;
};
#else
class StackMemPoolDebug {
protected:
    void PushAllocator(const LocalMapleAllocator * /* alloc */) const {}
    void PopAllocator() const {}
    void CheckTopAllocator(const LocalMapleAllocator * /* alloc */) const {}
};
#endif

class StackMemPool : public MemPool, private StackMemPoolDebug {
public:
    using MemPool::MemPool;
    friend LocalMapleAllocator;

private:
    // all malloc requested from LocalMapleAllocator
    void *Malloc(size_t size) override;
    uint8_t *AllocTailMemBlock(size_t size);

    // these methods should be called from LocalMapleAllocator
    template <class T>
    T *Clone(const T &t) = delete;

    template <class T, typename... Arguments>
    T *New(Arguments &&... args) = delete;

    template <class T>
    T *NewArray(size_t num) = delete;

    // reuse mempool fixedMemHead, bigMemHead, (curPtr, endPtr for fixed memory)
    MemBlock *fixedMemStackTop = nullptr;
    MemBlock *bigMemStackTop = nullptr;
    uint8_t *bigCurPtr = nullptr;
    uint8_t *bigEndPtr = nullptr;
    MemBlock *AllocMemBlockBySize(size_t size);
    void ResetStackTop(const LocalMapleAllocator *alloc, uint8_t *fixedCurPtrMark, MemBlock *fixedStackTopMark,
                       uint8_t *bigCurPtrMark, MemBlock *bigStackTopMark) noexcept;
};
}  // namespace maple
#endif  // MEMPOOL_INCLUDE_MEMPOOL_H
