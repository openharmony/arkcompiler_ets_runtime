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

#include "ecmascript/js_file_path.h"

namespace panda::ecmascript {
std::string JSFilePath::GetBaseName(const std::string &filePath)
{
    auto beginPos = filePath.find_last_of("/");
    if (beginPos != std::string::npos) {
        beginPos++;
    } else {
        beginPos = 0;
    }

    return filePath.substr(beginPos);
}

std::string JSFilePath::GetFileName(const std::string &filePath)
{
    auto beginPos = filePath.find_last_of('/');
    if (beginPos != std::string::npos) {
        beginPos++;
    } else {
        beginPos = 0;
    }
    auto endPos = filePath.find_last_of('.');
    if (endPos != std::string::npos) {
        return filePath.substr(beginPos, endPos - beginPos);
    }

    return filePath.substr(beginPos);
}

std::string JSFilePath::GetFileExtension(const std::string &filePath)
{
    auto beginPos = filePath.find_last_of('.');
    if (beginPos != std::string::npos) {
        return filePath.substr(beginPos);
    }

    return "";
}

std::string JSFilePath::GetHapName(const JSPandaFile *jsPandaFile)
{
    std::string jsPandaFileDesc = jsPandaFile->GetJSPandaFileDesc().c_str();
    if (jsPandaFileDesc.find(JSPandaFile::MERGE_ABC_NAME) == std::string::npos) {
        return "";
    }

    auto found = jsPandaFileDesc.find(JSPandaFile::BUNDLE_INSTALL_PATH);
    if (found == std::string::npos) {
        return "";
    }
    std::string subPath = jsPandaFileDesc.substr(sizeof(JSPandaFile::BUNDLE_INSTALL_PATH) - 1);
    auto endPos = subPath.find("/");
    if (endPos == std::string::npos) {
        return "";
    }

    return subPath.substr(0, endPos);
}

}  // namespace panda::ecmascript
