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

#include "ecmascript/base/file_path_helper.h"

#include <climits>

#include "ecmascript/log_wrapper.h"

#ifdef PANDA_TARGET_WINDOWS
#include "shlwapi.h"
#endif

namespace panda::ecmascript::base {

bool FilePathHelper::RealPath(const std::string &path, std::string &realPath, [[maybe_unused]] bool readOnly)
{
    realPath = "";
    if (path.empty() || path.size() > PATH_MAX) {
        LOG_ECMA(WARN) << "File path is illeage";
    }
    char buffer[PATH_MAX] = { '\0' };
#ifndef PANDA_TARGET_WINDOWS
    if (!realpath(path.c_str(), buffer)) {
        // Maybe file does not exist.
        if (readOnly || errno != ENOENT) {
            LOG_ECMA(ERROR) << "File path realpath failure";
            return false;
        }
    }
#else
    if (!_fullpath(buffer, path.c_str(), sizeof(buffer) - 1)) {
        return false;
    }
#endif
    realPath = std::string(buffer);
    return true;
}
}  // namespace panda::ecmascript::base
