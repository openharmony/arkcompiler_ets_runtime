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

#include "ecmascript/platform/directory.h"

#include <directory_ex.h>

namespace panda::ecmascript {
bool ForceCreateDirectory(const std::string &path)
{
    if (!OHOS::ForceCreateDirectory(path)) {
        return false;
    }
    return true;
}

bool SetDirModeAsDefault(const std::string &path)
{
    mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    return OHOS::ChangeModeDirectory(path, mode);
}

bool SetFileModeAsDefault(const std::string &filename)
{
    mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    return OHOS::ChangeModeFile(filename, mode);
}

uint64_t GetFolderSize(const std::string &path)
{
    return OHOS::GetFolderSize(path);
}
}  // namespace panda::ecmascript