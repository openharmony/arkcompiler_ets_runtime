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

#include "parsefromtext_fuzzer.h"
#include <fuzzer/FuzzedDataProvider.h>
#include "ecmascript/pgo_profiler/pgo_profiler_info.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::pgo;

namespace OHOS {
    void ParseFromTextFuzzTest(const uint8_t* data, size_t size)
    {
        // Use FuzzedDataProvider to parse input data
        FuzzedDataProvider dataProvider(data, size);
 
        std::string inputText = dataProvider.ConsumeRemainingBytesAsString();
      
        std::string tempFileName = "temp_input_file.txt";
        {
            std::ofstream tempFile(tempFileName);
            tempFile << inputText;
        }

        std::ifstream inputFile(tempFileName);
        if (!inputFile.is_open()) {
            return;
        }

        PGOPandaFileInfos pandaFileInfos;
        PGORecordDetailInfos recordDetailInfos(0);

        pandaFileInfos.ParseFromText(inputFile);

        // reset
        inputFile.clear();
        inputFile.seekg(0, std::ios::beg);
        recordDetailInfos.ParseFromText(inputFile);

        // close and delete tmp
        inputFile.close();
        std::remove(tempFileName.c_str());
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::ParseFromTextFuzzTest(data, size);
    return 0;
}