/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_BASE_FILESYSTEM_H
#define ECMASCRIPT_BASE_FILESYSTEM_H

#include <string>
#include <sys/types.h>

#include "ecmascript/common.h"

namespace panda::ecmascript::filesystem {
bool CreateDirectory(const std::string& path);
bool Exists(const std::string& path);
std::string TempDirectoryPath();
std::size_t FileSize(const std::string& path);
bool RemoveAll(const std::string& path);
PUBLIC_API void CreateEmptyFile(const std::string& fileName);
} // namespace panda::ecmascript::base
#endif // ECMASCRIPT_BASE_FILESYSTEM_H