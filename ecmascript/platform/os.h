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

#ifndef ECMASCRIPT_PLATFORM_OS_H
#define ECMASCRIPT_PLATFORM_OS_H

#include <cstddef>
#include <cstdint>
#include <string>
#include "ecmascript/common.h"

namespace panda::ecmascript {
const char XATTR_KEY[] = {"user.security"};
constexpr std::string_view DEFAULT_DATA_LEVEL = "s1";
const int DEFAULT_DATA_LENGTH = 2;
size_t MallocUsableSize(void *p);
uint32_t NumberOfCpuCore();
size_t PhysicalSize();
int PrctlSetVMA(const void *p, const size_t size, const char *tag);
long PtracePeektext(int pid, uintptr_t addr);
PUBLIC_API void BindSmallCpuCore();
PUBLIC_API void BindMidCpuCore();
PUBLIC_API void SetSecurityLabel(const std::string& path);
void PUBLIC_API *PageMapExecFortSpace(void *addr, size_t size, int prot);
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PLATFORM_OS_H
