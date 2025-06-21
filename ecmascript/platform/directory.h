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

#ifndef ECMASCRIPT_PLATFORM_DIRECTORY_H
#define ECMASCRIPT_PLATFORM_DIRECTORY_H

#include <string>
#include "ecmascript/common.h"

namespace panda::ecmascript {
bool PUBLIC_API ForceCreateDirectory(const std::string &path);
bool PUBLIC_API SetDirModeAsDefault(const std::string &path);
bool PUBLIC_API SetFileModeAsDefault(const std::string &filename);
uint64_t PUBLIC_API GetFolderSize(const std::string &path);
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PLATFORM_DIRECTORY_H
