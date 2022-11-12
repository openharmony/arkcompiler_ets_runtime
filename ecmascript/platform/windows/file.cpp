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

#include "ecmascript/platform/file.h"

#include <climits>
#include <shlwapi.h>

#ifdef ERROR
#undef ERROR
#endif

#include "ecmascript/log_wrapper.h"

namespace panda::ecmascript {

bool RealPath(const std::string &path, std::string &realPath, [[maybe_unused]] bool readOnly)
{
    realPath = "";
    if (path.empty() || path.size() > PATH_MAX) {
        LOG_ECMA(WARN) << "File path is illeage";
        return false;
    }
    char buffer[PATH_MAX] = { '\0' };
    if (!_fullpath(buffer, path.c_str(), sizeof(buffer) - 1)) {
        LOG_ECMA(WARN) << "File path:" << path << " full path failure";
        return false;
    }
    realPath = std::string(buffer);
    return true;
}
}  // namespace panda::ecmascript
