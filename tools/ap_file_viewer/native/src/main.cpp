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

#include <fstream>
#include <iostream>
#include <string>
#include "ecmascript/log_wrapper.h"
#include "ecmascript/platform/file.h"
#include "prof_dump_json.h"
#include "securec.h"

static constexpr size_t OUTPUT_FILE_ARG = 2;
static constexpr size_t ARGS_NUMBER = 3;

int main(int argc, char** argv)
{
    if (argc != ARGS_NUMBER) {
        std::cout << "please use this tools like:" << std::endl;
        std::cout << "    ./apViewer input.ap out.json" << std::endl;
        return 0;
    }
    std::string input = argv[1];
    std::string output = argv[OUTPUT_FILE_ARG];
    size_t length = native::ConvertApToJson(input.c_str(), input.length());
    if (length == 0 || length >= SIZE_MAX) {
        LOG_NO_TAG(ERROR) << "ConvertApToJson returned zero or invalid length!";
        return 0;
    }
    char *buffer = (char *)malloc(length);
    if (buffer == nullptr) {
        LOG_NO_TAG(ERROR) << "malloc buffer failed";
        return 0;
    }
    if (memset_s(buffer, length, 0, length) != EOK) {
        LOG_NO_TAG(ERROR) << "clean malloc buffer failed";
        free(buffer);
        return 0;
    }

    auto result = native::GetConvertResult(buffer, length);
    if (result) {
        std::cout << "convert " << argv[1] << " to " << argv[OUTPUT_FILE_ARG] << " success!" << std::endl;
    } else {
        std::cout << "convert " << argv[1] << " to " << argv[OUTPUT_FILE_ARG] << " failed!" << std::endl;
    }

    std::string realOutPath;
    if (!panda::ecmascript::RealPath(output, realOutPath, false)) {
        LOG_NO_TAG(ERROR) << "Can not load filepath " << output;
        free(buffer);
        return 0;
    }
    std::ofstream outFile(realOutPath);
    if (!outFile.is_open()) {
        LOG_NO_TAG(ERROR) << "open " << realOutPath << " failed!";
        free(buffer);
        return 0;
    }
    std::string strRet(buffer, length);
    outFile << strRet;
    outFile.close();
    free(buffer);
    return 0;
}
