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

#ifndef ECMASCRIPT_MEM_ASSERT_SCOPE_H
#define ECMASCRIPT_MEM_ASSERT_SCOPE_H

#include <optional>

#include "ecmascript/common.h"
#include "libpandabase/utils/bit_field.h"

namespace panda::ecmascript {

#ifndef NDEBUG
constexpr bool IS_ALLOW_CHECK = true;
#else
constexpr bool IS_ALLOW_CHECK = false;
#endif

enum class AssertType : uint8_t {
    GARBAGE_COLLECTION_ASSERT = 0,
    HEAP_ALLOC_ASSERT,
    HANDLE_ALLOC_ASSERT,
    DEREF_HANDLE_ASSERT,
    LOCAL_TO_SHARE_WEAK_REF_ASSERT,
    LAST_ASSERT_TYPE
};

template<AssertType type, bool isAllow, bool IsDebug = IS_ALLOW_CHECK>
class PUBLIC_API AssertScopeT {
public:
    static bool IsAllowed();
};

template<AssertType type, bool isAllow>
class PUBLIC_API AssertScopeT<type, isAllow, true> {
public:
    AssertScopeT();

    ~AssertScopeT();

    static bool IsAllowed();

private:
    std::optional<size_t> oldData_;
};

[[maybe_unused]] void ForceResetAssertData();

using DisallowGarbageCollection = AssertScopeT<AssertType::GARBAGE_COLLECTION_ASSERT, false, IS_ALLOW_CHECK>;
using AllowGarbageCollection = AssertScopeT<AssertType::GARBAGE_COLLECTION_ASSERT, true, IS_ALLOW_CHECK>;
using DisAllowHeapAlloc = AssertScopeT<AssertType::HEAP_ALLOC_ASSERT, false, IS_ALLOW_CHECK>;
using AllowHeapAlloc = AssertScopeT<AssertType::HEAP_ALLOC_ASSERT, true, IS_ALLOW_CHECK>;
using DisAllowHandleAllocation = AssertScopeT<AssertType::HANDLE_ALLOC_ASSERT, false, IS_ALLOW_CHECK>;
using AllowHandleAllocation = AssertScopeT<AssertType::HANDLE_ALLOC_ASSERT, true, IS_ALLOW_CHECK>;
using DisAllowDeRefHandle = AssertScopeT<AssertType::DEREF_HANDLE_ASSERT, false, IS_ALLOW_CHECK>;
using AllowDeRefHandle = AssertScopeT<AssertType::DEREF_HANDLE_ASSERT, true, IS_ALLOW_CHECK>;
using DisAllowLocalToShareWeakRefHandle = AssertScopeT<AssertType::LOCAL_TO_SHARE_WEAK_REF_ASSERT,
                                                       false, IS_ALLOW_CHECK>;
using AllowLocalToShareWeakRefHandle = AssertScopeT<AssertType::LOCAL_TO_SHARE_WEAK_REF_ASSERT,
                                                    true, IS_ALLOW_CHECK>;
#if (!defined NDEBUG) || (defined RUN_TEST)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DISALLOW_GARBAGE_COLLECTION [[maybe_unused]] DisallowGarbageCollection noGc
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ALLOW_GARBAGE_COLLECTION [[maybe_unused]] AllowGarbageCollection allowGc
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DISALLOW_HEAP_ALLOC [[maybe_unused]] DisAllowHeapAlloc noHeapAlloc
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ALLOW_HEAP_ALLOC [[maybe_unused]] AllowHeapAlloc allowHeapAlloc
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DISALLOW_HANDLE_ALLOC [[maybe_unused]] DisAllowHandleAllocation disAllowHandleAlloc
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ALLOW_HANDLE_ALLOC [[maybe_unused]] AllowHandleAllocation allowHandleAlloc
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DISALLOW_DEREF_HANDLE [[maybe_unused]] DisAllowDeRefHandle disAllowDeRefHandle
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ALLOW_DEREF_HANDLE [[maybe_unused]] AllowDeRefHandle allowDeRefHandle
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DISALLOW_LOCAL_TO_SHARE_WEAK_REF_HANDLE             \
    [[maybe_unused]] DisAllowLocalToShareWeakRefHandle disAllowLocalToShareWeakRefHandle
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ALLOW_LOCAL_TO_SHARE_WEAK_REF_HANDLE                \
    [[maybe_unused]] AllowLocalToShareWeakRefHandle allowLocalToShareRefHandle
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DISALLOW_HEAP_ACCESS \
    DISALLOW_HEAP_ALLOC;     \
    DISALLOW_HANDLE_ALLOC;   \
    DISALLOW_DEREF_HANDLE;
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ALLOW_HEAP_ACCESS \
    ALLOW_HEAP_ALLOC;     \
    ALLOW_HANDLE_ALLOC;   \
    ALLOW_DEREF_HANDLE;
#else
#define DISALLOW_GARBAGE_COLLECTION
#define ALLOW_GARBAGE_COLLECTION
#define DISALLOW_HEAP_ALLOC
#define ALLOW_HEAP_ALLOC
#define DISALLOW_HANDLE_ALLOC
#define ALLOW_HANDLE_ALLOC
#define DISALLOW_HEAP_ACCESS
#define ALLOW_HEAP_ACCESS
#define DISALLOW_DEREF_HANDLE
#define ALLOW_DEREF_HANDLE
#define DISALLOW_LOCAL_TO_SHARE_WEAK_REF_HANDLE
#define ALLOW_LOCAL_TO_SHARE_WEAK_REF_HANDLE
#endif

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CHECK_NO_GC ASSERT_PRINT(AllowGarbageCollection::IsAllowed(), "disallow execute garbage collection.")

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CHECK_NO_HEAP_ALLOC ASSERT_PRINT(AllowHeapAlloc::IsAllowed(), "disallow execute heap alloc.")

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CHECK_NO_HANDLE_ALLOC ASSERT_PRINT(AllowHandleAllocation::IsAllowed(), "disallow execute handle alloc.")

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CHECK_NO_DEREF_HANDLE ASSERT_PRINT(AllowDeRefHandle::IsAllowed(), "disallow execute deref handle.")

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CHECK_NO_LOCAL_TO_SHARE_WEAK_REF_HANDLE         \
    ASSERT_PRINT(AllowLocalToShareWeakRefHandle::IsAllowed(), "disallow local to share weak ref handle.")
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_ASSERT_SCOPE_H
