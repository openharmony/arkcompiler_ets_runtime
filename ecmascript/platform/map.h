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

#ifndef ECMASCRIPT_PLATFORM_MAP_H
#define ECMASCRIPT_PLATFORM_MAP_H

#include <cstddef>

#include "ecmascript/common.h"
#include "ecmascript/mem/c_string.h"

namespace panda::ecmascript {
class MemMap {
public:
    MemMap() : originAddr_(nullptr), mem_(nullptr), size_(0) {}
    MemMap(void *mem, size_t size) : originAddr_(mem), mem_(mem), size_(size) {};
    MemMap(void *originAddr, void *mem, size_t size) : originAddr_(originAddr), mem_(mem), size_(size) {};
    ~MemMap() = default;

    void Reset()
    {
        originAddr_ = nullptr;
        mem_ = nullptr;
        size_ = 0;
    }

    inline void *GetMem() const
    {
        return mem_;
    }

    inline size_t GetSize() const
    {
        return size_;
    }

    inline void *GetOriginAddr() const
    {
        return originAddr_;
    }

    bool operator<(const MemMap &o) const
    {
        return GetMem() < o.GetMem();
    }
private:
    void *originAddr_ {nullptr};
    void *mem_ {nullptr};
    size_t size_ {0};
};

enum class PageTagType : uint8_t {
    HEAP,
    MACHINE_CODE,
    MEMPOOL_CACHE,
    METHOD_LITERAL,
};

#ifdef PANDA_TARGET_WINDOWS
#define PAGE_PROT_NONE 0x01
#define PAGE_PROT_READ 0x02
#define PAGE_PROT_READWRITE 0x04
#define PAGE_PROT_EXEC_READ 0x20
// For safety reason, Disallow prot have both write & exec capability except in JIT.
#define PAGE_PROT_EXEC_READWRITE 0x40
#define PAGE_FLAG_MAP_FIXED 0x10
#else
#define PAGE_PROT_NONE 0
#define PAGE_PROT_READ 1
#define PAGE_PROT_READWRITE 3
#define PAGE_PROT_EXEC_READ 5
// For safety reason, Disallow prot have both write & exec capability except in JIT.
#define PAGE_PROT_EXEC_READWRITE 7
#define PAGE_FLAG_MAP_FIXED 0x10
#endif

// Jit Fort space protection control
inline int PageProtectProt([[maybe_unused]] bool disable_codesign)
{
    return PAGE_PROT_EXEC_READWRITE;
}

static constexpr char HEAP_TAG[] = "ArkTS Heap";
static constexpr char CODE_TAG[] = "ArkTS Code";
MemMap PUBLIC_API PageMap(size_t size, int prot = PAGE_PROT_NONE, size_t alignment = 0, void *addr = nullptr,
    int flags = 0, bool jitfort = false);
void PUBLIC_API PageUnmap(MemMap it, bool jitfort = false);
MemMap PUBLIC_API MachineCodePageMap(size_t size, int prot = PAGE_PROT_NONE, size_t alignment = 0);
void PUBLIC_API MachineCodePageUnmap(MemMap it);
void PageRelease(void *mem, size_t size);
void PUBLIC_API PagePreRead(void *mem, size_t size);
void PageTag(void *mem, size_t size, PageTagType type, const std::string &spaceName = "",
             const uint32_t threadId = 0);
void PageClearTag(void *mem, size_t size);
const std::string GetPageTagString(PageTagType type, const std::string &spaceName, const uint32_t threadId = 0);
const char *GetPageTagString(PageTagType type);
int PageProtect(void *mem, size_t size, int prot);
size_t PUBLIC_API PageSize();

// ---------------------------------------------------------------------------
// Shared "fill template" backing store.
//
// A small anonymous memfd pre-filled with one repeating 64-bit word (for us,
// JSTaggedValue::VALUE_HOLE). Mapping it MAP_PRIVATE gives memory that *reads*
// as that word without owning any physical page: every mapping shares the same
// page-cache pages until it is written to, at which point only the written page
// is copied.
//
// The fd is created once, before appspawn forks, so every application process
// inherits it and shares the same physical pages.
//
// CreateFillTemplate() is idempotent and returns false if the platform cannot
// provide it (Windows, or memfd_create unavailable); callers must then fall
// back to writing the value themselves.
// wordSize is the width of one repetition of fillWord, in bytes: 8 for plain
// JSTaggedType slots, 4 for compressed ones. It matters - a template filled
// with 8-byte 0x5 reads as [0x5, 0x0] when interpreted as two 4-byte slots, and
// a zeroed tagged value is IsHeapObject(), i.e. a null pointer, not a hole.
//
// One template can exist per width; call this once per width you need. They are
// independent memfds, each costing its own `size` bytes of shared pages.
bool PUBLIC_API CreateFillTemplate(size_t size, uint64_t fillWord, size_t wordSize);
void PUBLIC_API DestroyFillTemplates();
bool PUBLIC_API IsFillTemplateReady(size_t wordSize);

// Replace [addr, addr + size) with a private mapping of the wordSize template.
// addr and size must be page aligned. Ranges larger than the template are
// covered by repeating it, which costs one VMA per template-sized chunk.
bool PUBLIC_API MapFillTemplate(void *addr, size_t size, size_t wordSize);
// Put plain anonymous (zeroed) memory back, so the range can be recycled by the
// mem-map pool without carrying the file mapping with it.
bool PUBLIC_API UnmapFillTemplate(void *addr, size_t size);
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PLATFORM_MAP_H
