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

#include "mempool.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include "securec.h"
#include "mpl_logging.h"

namespace maple {
MemPoolCtrler memPoolCtrler;
bool MemPoolCtrler::freeMemInTime = false;

void MemPoolCtrler::FreeMemBlocks(const MemPool &pool, MemBlock *fixedMemHead, MemBlock *bigMemHead)
{
    (void)(pool);

    MemBlock *fixedTail = nullptr;

    if (fixedMemHead != nullptr) {
        fixedTail = fixedMemHead;
        while (fixedTail->nextMemBlock != nullptr) {
            fixedTail = fixedTail->nextMemBlock;
        }
    }

    while (bigMemHead != nullptr) {
        auto *cur = bigMemHead;
        bigMemHead = bigMemHead->nextMemBlock;
        free(cur->startPtr);
        delete cur;
    }

    if (fixedTail != nullptr) {
        fixedTail->nextMemBlock = fixedFreeMemBlocks;
        DEBUG_ASSERT(fixedTail->nextMemBlock != fixedTail, "error");
        fixedFreeMemBlocks = fixedMemHead;
    }
}

// Destructor, free all allocated memories
MemPoolCtrler::~MemPoolCtrler()
{
    FreeMem();
}

void MemPoolCtrler::FreeFixedSizeMemBlockMemory()
{
    FreeMem();
    sysMemoryMgr->ReleaseMemory();
}

// Allocate a new memory pool and register it in controller
MemPool *MemPoolCtrler::NewMemPool(const std::string &name, bool isLocalPool)
{
    MemPool *memPool = nullptr;

    if (isLocalPool) {
        memPool = new ThreadLocalMemPool(*this, name);
    } else {
        memPool = new ThreadShareMemPool(*this, name);
    }

    return memPool;
}

// This function will be removed soon, DO NOT call it, just use delete memPool
void MemPoolCtrler::DeleteMemPool(MemPool *memPool) const
{
    delete memPool;
}

void MemPoolCtrler::FreeMem()
{
    while (fixedFreeMemBlocks != nullptr) {
        MemBlock *arena = fixedFreeMemBlocks;
        fixedFreeMemBlocks = fixedFreeMemBlocks->nextMemBlock;
        delete arena;
    }
}

MemBlock *MemPoolCtrler::AllocMemBlock(const MemPool &pool, size_t size)
{
    if (size <= kMemBlockSizeMin) {
        return AllocFixMemBlock(pool);
    } else {
        return AllocBigMemBlock(pool, size);
    }
}

MemBlock *MemPoolCtrler::AllocFixMemBlock(const MemPool &pool)
{
    (void)(pool);
    MemBlock *ret = nullptr;

    if (fixedFreeMemBlocks != nullptr) {
        ret = fixedFreeMemBlocks;
        fixedFreeMemBlocks = fixedFreeMemBlocks->nextMemBlock;
        return ret;
    }

    uint8_t *ptr = sysMemoryMgr->RealAllocMemory(kMemBlockMalloc);
    // leave one MemBlock to return
    for (size_t i = 0; i < kMemBlockMalloc / kMemBlockSizeMin - 1; ++i) {
        auto *block = new MemBlock(ptr, kMemBlockSizeMin);
        ptr += kMemBlockSizeMin;
        block->nextMemBlock = fixedFreeMemBlocks;
        fixedFreeMemBlocks = block;
    }

    return new MemBlock(ptr, kMemBlockSizeMin);
}

MemBlock *MemPoolCtrler::AllocBigMemBlock(const MemPool &pool, size_t size) const
{
    DEBUG_ASSERT(size > kMemBlockSizeMin, "Big memory block must be bigger than fixed memory block");
    (void)(pool);

    uint8_t *block = reinterpret_cast<uint8_t *>(malloc(size));
    CHECK_FATAL(block != nullptr, "malloc failed");
    return new MemBlock(block, size);
}

MemPool::~MemPool()
{
    ctrler.FreeMemBlocks(*this, fixedMemHead, bigMemHead);
}

void *MemPool::Malloc(size_t size)
{
    size = BITS_ALIGN(size);
    DEBUG_ASSERT(endPtr >= curPtr, "endPtr should >= curPtr");
    if (size > static_cast<size_t>(endPtr - curPtr)) {
        return AllocNewMemBlock(size);
    }
    uint8_t *retPtr = curPtr;
    curPtr += size;
    return retPtr;
}

void MemPool::ReleaseContainingMem()
{
    ctrler.FreeMemBlocks(*this, fixedMemHead, bigMemHead);

    fixedMemHead = nullptr;
    bigMemHead = nullptr;
    endPtr = nullptr;
    curPtr = nullptr;
}

// Malloc size of memory from memory pool, then set 0
void *MemPool::Calloc(size_t size)
{
    void *p = Malloc(BITS_ALIGN(size));
    DEBUG_ASSERT(p != nullptr, "ERROR: Calloc error");
    errno_t eNum = memset_s(p, BITS_ALIGN(size), 0, BITS_ALIGN(size));
    CHECK_FATAL(eNum == EOK, "memset_s failed");
    return p;
}

// Realloc new size of memory
void *MemPool::Realloc(const void *ptr, size_t oldSize, size_t newSize)
{
    void *result = Malloc(newSize);
    DEBUG_ASSERT(result != nullptr, "ERROR: Realloc error");
    size_t copySize = ((newSize > oldSize) ? oldSize : newSize);
    if (copySize != 0 && ptr != nullptr) {
        errno_t eNum = memcpy_s(result, copySize, ptr, copySize);
        CHECK_FATAL(eNum == EOK, "memcpy_s failed");
    }
    return result;
}

uint8_t *MemPool::AllocNewMemBlock(size_t size)
{
    MemBlock **head = nullptr;
    MemBlock *newMemBlock = ctrler.AllocMemBlock(*this, size);
    if (newMemBlock->memSize <= kMemBlockSizeMin) {
        head = &fixedMemHead;
    } else {
        head = &bigMemHead;
    }

    newMemBlock->nextMemBlock = *head;
    *head = newMemBlock;
    CHECK_FATAL(newMemBlock->nextMemBlock != newMemBlock, "error");

    curPtr = newMemBlock->startPtr + size;
    endPtr = newMemBlock->startPtr + newMemBlock->memSize;
    DEBUG_ASSERT(curPtr <= endPtr, "must be");

    return newMemBlock->startPtr;
}

void *StackMemPool::Malloc(size_t size)
{
    size = BITS_ALIGN(size);
    uint8_t **curPtrPtr = nullptr;
    uint8_t *curEndPtr = nullptr;
    if (size <= kMemBlockSizeMin) {
        curPtrPtr = &curPtr;
        curEndPtr = endPtr;
    } else {
        curPtrPtr = &bigCurPtr;
        curEndPtr = bigEndPtr;
    }
    uint8_t *retPtr = *curPtrPtr;
    DEBUG_ASSERT(curEndPtr >= *curPtrPtr, "endPtr should >= curPtr");
    if (size > static_cast<size_t>(curEndPtr - *curPtrPtr)) {
        retPtr = AllocTailMemBlock(size);
    }
    *curPtrPtr = retPtr + size;
    return retPtr;
}

// scoped mem pool don't use big mem block for small size, different with normal mempool
MemBlock *StackMemPool::AllocMemBlockBySize(size_t size)
{
    if (size <= kMemBlockSizeMin) {
        return ctrler.AllocFixMemBlock(*this);
    } else {
        return ctrler.AllocBigMemBlock(*this, size);
    }
}

void StackMemPool::ResetStackTop(const LocalMapleAllocator *alloc, uint8_t *fixedCurPtrMark,
                                 MemBlock *fixedStackTopMark, uint8_t *bigCurPtrMark,
                                 MemBlock *bigStackTopMark) noexcept
{
    CheckTopAllocator(alloc);
    PopAllocator();

    if (fixedStackTopMark != nullptr) {
        fixedMemStackTop = fixedStackTopMark;
        curPtr = fixedCurPtrMark;
        endPtr = fixedMemStackTop->EndPtr();
    } else if (fixedMemHead != nullptr) {
        fixedMemStackTop = fixedMemHead;
        curPtr = fixedMemStackTop->startPtr;
        endPtr = fixedMemStackTop->EndPtr();
    }

    if (bigStackTopMark != nullptr) {
        bigMemStackTop = bigStackTopMark;
        bigCurPtr = bigCurPtrMark;
        bigEndPtr = bigMemStackTop->EndPtr();
    } else if (bigMemHead != nullptr) {
        bigMemStackTop = bigMemHead;
        bigCurPtr = bigMemStackTop->startPtr;
        bigEndPtr = bigMemStackTop->EndPtr();
    }
}

uint8_t *StackMemPool::AllocTailMemBlock(size_t size)
{
    MemBlock **head = nullptr;
    MemBlock **stackTop = nullptr;
    uint8_t **endPtrPtr = nullptr;

    if (size <= kMemBlockSizeMin) {
        head = &fixedMemHead;
        stackTop = &fixedMemStackTop;
        endPtrPtr = &endPtr;
    } else {
        head = &bigMemHead;
        stackTop = &bigMemStackTop;
        endPtrPtr = &bigEndPtr;
    }

    if (*stackTop == nullptr) {
        MemBlock *newMemBlock = AllocMemBlockBySize(size);
        *stackTop = newMemBlock;
        *head = newMemBlock;
        (*stackTop)->nextMemBlock = nullptr;
    } else {
        if ((*stackTop)->nextMemBlock != nullptr && (*stackTop)->nextMemBlock->memSize >= size) {
            *stackTop = (*stackTop)->nextMemBlock;
        } else {
            MemBlock *newMemBlock = AllocMemBlockBySize(size);
            auto *tmp = (*stackTop)->nextMemBlock;
            (*stackTop)->nextMemBlock = newMemBlock;
            *stackTop = newMemBlock;
            newMemBlock->nextMemBlock = tmp;
        }
    }
    *endPtrPtr = (*stackTop)->EndPtr();
    return (*stackTop)->startPtr;
}
}  // namespace maple
