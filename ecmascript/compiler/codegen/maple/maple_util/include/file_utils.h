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

#ifndef MAPLE_DRIVER_INCLUDE_FILE_UTILS_H
#define MAPLE_DRIVER_INCLUDE_FILE_UTILS_H
#include <string>

namespace maple {
extern const std::string kFileSeperatorStr;
extern const char kFileSeperatorChar;
// Use char[] since getenv receives char* as parameter
constexpr char kMapleRoot[] = "MAPLE_ROOT";

class FileUtils {
public:
    static std::string SafeGetenv(const char *envVar);
    static std::string GetRealPath(const std::string &filePath);
    static std::string GetFileName(const std::string &filePath, bool isWithExtension);
    static std::string GetFileExtension(const std::string &filePath);
    static std::string GetFileFolder(const std::string &filePath);
    static int Remove(const std::string &filePath);
    static bool IsFileExists(const std::string &filePath);
    static std::string AppendMapleRootIfNeeded(bool needRootPath, const std::string &path,
                                               const std::string &defaultRoot = "." + kFileSeperatorStr);
};
}  // namespace maple
#endif  // MAPLE_DRIVER_INCLUDE_FILE_UTILS_H
