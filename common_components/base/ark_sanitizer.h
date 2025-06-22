/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef COMMON_COMPONENTS_BASE_ARK_SANITIZER_H
#define COMMON_COMPONENTS_BASE_ARK_SANITIZER_H

#if (defined(__has_feature) && __has_feature(address_sanitizer)) || defined(__SANITIZE_ADDRESS__)
#define HAS_SANITIZER
#endif

#if defined(HAS_SANITIZER) && defined(RUN_WITH_ASAN)
#define ARK_ASAN_ON
#endif

#ifdef USE_ASAN
extern "C" {
// NOLINTNEXTLINE(readability-identifier-naming)
void __asan_poison_memory_region(void const volatile *addr, size_t size) __attribute__((visibility("default")));
// NOLINTNEXTLINE(readability-identifier-naming)
void __asan_unpoison_memory_region(void const volatile *addr, size_t size) __attribute__((visibility("default")));
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#ifndef ASAN_POISON_MEMORY_REGION
#define ASAN_POISON_MEMORY_REGION(addr, size) __asan_poison_memory_region((addr), (size))
#endif
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#ifndef ASAN_UNPOISON_MEMORY_REGION
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) __asan_unpoison_memory_region((addr), (size))
#endif
#elif defined(USE_HWASAN)
extern "C" {
// NOLINTNEXTLINE(readability-identifier-naming)
void __hwasan_tag_memory(void const volatile *p, unsigned char tag, size_t size) __attribute__((visibility("default")));
// NOLINTNEXTLINE(readability-identifier-naming)
void* __hwasan_tag_pointer(void const volatile *addr, unsigned char tag) __attribute__((visibility("default")));
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#ifndef ASAN_POISON_MEMORY_REGION
#define ASAN_POISON_MEMORY_REGION(addr, size) \
    do { \
        auto __addr = (addr); \
        std::srand(static_cast<unsigned>(std::time(nullptr))); \
        unsigned char __tag = static_cast<unsigned char>(std::rand() % 255 + 1); \
        __hwasan_tag_memory(static_cast<const volatile void*>(__addr), __tag, (size)); \
        (void)__hwasan_tag_pointer(static_cast<const volatile void*>(__addr), __tag); \
    } while (0)
#endif
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#ifndef ASAN_UNPOISON_MEMORY_REGION
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) \
    do { \
        auto __addr = (addr); \
        if ((0xff00000000000000ULL & reinterpret_cast<uintptr_t>(__addr))) { \
            void* __untagged = const_cast<void*>(__hwasan_tag_pointer(static_cast<const volatile void*>(__addr), 0)); \
            __hwasan_tag_memory(static_cast<const volatile void*>(__untagged), 0, (size)); \
        } \
    } while (0)
#endif

#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#ifdef ASAN_POISON_MEMORY_REGION
#undef ASAN_POISON_MEMORY_REGION
#endif
#define ASAN_POISON_MEMORY_REGION(addr, size) ((void)(addr), (void)(size))
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#ifdef ASAN_UNPOISON_MEMORY_REGION
#undef ASAN_UNPOISON_MEMORY_REGION
#endif
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) ((void)(addr), (void)(size))
#endif

#endif  // COMMON_COMPONENTS_BASE_ARK_SANITIZER_H
