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
    
    void CommonCompPrettyOrderMathNano(size_t size)
    {
        const char* orderOfMagnitudeFromNano[] = { "n", "u", "m" };
        PrettyOrderMathNano(size, orderOfMagnitudeFromNano[size % 3]); // 3: len of orderOfMagnitudeFromNano
    }
    
    void CommonCompPrettyOrderInfo(size_t size)
    {
        const char* orderOfMagnitude[] = { "", "K", "M", "G", "T", "P", "E" };
        PrettyOrderInfo(size, orderOfMagnitude[size % 7]); // 7：len of orderOfMagnitude
    }
    
    void CommonCompPretty(size_t size)
    {
        Pretty(size);
    }

    void CommonCompLevelToString(size_t size)
    {
        Log log;
        log.LevelToString(static_cast<Level>(size % 7)); // 7: FATAL + 1
    }

    void CommonCompConvertFromRuntime(size_t size)
    {
        Log log;
        log.ConvertFromRuntime(static_cast<LOG_LEVEL>(size % 5)); // 5: LOG_LEVEL + 1
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::CommonCompFormatLog(data, size);
    OHOS::CommonCompPrettyOrderMathNano(size);
    OHOS::CommonCompPrettyOrderInfo(size);
    OHOS::CommonCompPretty(size);
    OHOS::CommonCompLevelToString(size);
    OHOS::CommonCompConvertFromRuntime(size);

    return 0;
}