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

#ifndef ECMASCRIPT_JSFILE_PATH_H
#define ECMASCRIPT_JSFILE_PATH_H

#include <string>
#include "ecmascript/jspandafile/js_pandafile.h"

namespace panda::ecmascript {

class JSFilePath {
public:
    static std::string GetBaseName(const std::string &filePath);
    static std::string GetFileName(const std::string &filePath);
    static std::string GetFileExtension(const std::string &filePath);
    static std::string GetHapName(const JSPandaFile *jsPandaFile);
    static std::string GetAotFullPathName(const JSPandaFile *jsPandaFile);
};
}
#endif  // ECMASCRIPT_JSFILE_PATH_H
