/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "commoncomplog_fuzzer.h"
#include "common_components/log/log.h"
#include "test/fuzztest/containersdequecommon_fuzzer/containersdequecommon_fuzzer.h"

using namespace common;

namespace OHOS {

    void CommonCompFormatLog(const uint8_t* data, size_t size)
    {
        FormatLog("%d", data[size / 2]); // 2：Check size is even number
    }
    
    void CommonCompPrettyOrderMathNano(const uint8_t* data, size_t size)
    {
        const char* orderOfMagnitudeFromNano[] = { "n", "u", "m" };
        if (size == 0) {
            return;
        }
        PrettyOrderMathNano(size, orderOfMagnitudeFromNano[data[size - 1] % 3]); // 3: len of orderOfMagnitudeFromNano
    }
    
    void CommonCompPrettyOrderInfo(const uint8_t* data, size_t size)
    {
        const char* orderOfMagnitude[] = { "", "K", "M", "G", "T", "P", "E" };
        if (size == 0) {
            return;
        }
        PrettyOrderInfo(size, orderOfMagnitude[data[size - 1] % 7]); // 7：len of orderOfMagnitude
    }
    
    void CommonCompPretty(const uint8_t* data, size_t size)
    {
        if (size == 0) {
            return;
        }
        Pretty(data[size - 1]);
    }

    void CommonCompLevelToString(const uint8_t* data, size_t size)
    {
        if (size == 0) {
            return;
        }
        Log log;
        log.LevelToString(static_cast<Level>(data[size - 1] % 7)); // 7: FATAL + 1
    }

    void CommonCompConvertFromRuntime(const uint8_t* data, size_t size)
    {
        if (size == 0) {
            return;
        }
        Log log;
        log.ConvertFromRuntime(static_cast<LOG_LEVEL>(data[size - 1] % 5)); // 5: LOG_LEVEL + 1
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::CommonCompFormatLog(data, size);
    OHOS::CommonCompPrettyOrderMathNano(data, size);
    OHOS::CommonCompPrettyOrderInfo(data, size);
    OHOS::CommonCompPretty(data, size);
    OHOS::CommonCompLevelToString(data, size);
    OHOS::CommonCompConvertFromRuntime(data, size);

    return 0;
}