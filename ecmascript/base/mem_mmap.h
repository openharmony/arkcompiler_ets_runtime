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

#ifndef ECMASCRIPT_BASE_MEM_MMAP_H
#define ECMASCRIPT_BASE_MEM_MMAP_H

namespace panda::ecmascript::base {
class MemMmap {
public:
    MemMmap() = default;
    ~MemMmap() = default;

#ifdef PANDA_TARGET_WINDOWS
    static void *mmap(size_t size, int fd, off_t offset);
#endif
    static void *Mmap(size_t allocSize);
    static void Munmap(void *addr, size_t allocSize);
};
}  // namespace panda::ecmascript::base

#endif  // ECMASCRIPT_BASE_MEM_MMAP_H