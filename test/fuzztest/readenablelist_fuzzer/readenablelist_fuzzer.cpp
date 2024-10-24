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

#include "readenablelist_fuzzer.h"
#include <fuzzer/FuzzedDataProvider.h>
#include "ecmascript/ohos/enable_aot_list_helper.h"

using namespace panda::ecmascript::ohos;

namespace OHOS {
    constexpr size_t MAX_STRING_LENGTH = 100;

    void ReadEnableListFuzzTest(const uint8_t *data, size_t size)
    {
        FuzzedDataProvider dataProvider(data, size);

        // generate random listname of AOT JIT
        std::string aotJitListName = dataProvider.ConsumeRandomLengthString(MAX_STRING_LENGTH);
        EnableAotJitListHelper helper(aotJitListName);

        std::string candidate = dataProvider.ConsumeRandomLengthString(MAX_STRING_LENGTH);
        helper.IsEnableAot(candidate);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::ReadEnableListFuzzTest(data, size);
    return 0;
}