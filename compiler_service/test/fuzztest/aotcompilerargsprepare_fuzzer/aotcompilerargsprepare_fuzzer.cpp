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

#include "aotcompilerargsprepare_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include "aot_compiler_impl.h"
#include "securec.h"
#include "system_ability_definition.h"

using namespace OHOS::ArkCompiler;

namespace OHOS {
constexpr size_t BYTE_LEN = 8;
constexpr size_t MAP_MAX_LEN = 1024;
constexpr size_t VEC_MAX_LEN = 2048;

int16_t GetI16Data(const char* ptr, size_t index)
{
    return (ptr[index] << BYTE_LEN) | (ptr[index - 1]);
}

bool DoSomethingInterestingWithMyAPI(const char* data, size_t size)
{
    std::string argsKey(data, size);
    std::string argsValue = "fuzz test";

    std::unordered_map<std::string, std::string> argsMap;
    for (size_t i = size % MAP_MAX_LEN; i > 0; --i) {
        argsMap.emplace(argsKey, argsValue);
    }
    std::vector<int16_t> sigData;
    for (size_t j = size % VEC_MAX_LEN; j > 0; --j) {
        sigData.emplace_back(GetI16Data(data, j));
    }
    AotCompilerImpl::GetInstance().EcmascriptAotCompiler(argsMap, sigData);
    return true;
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    const char* dataPtr = reinterpret_cast<const char*>(data);

    OHOS::DoSomethingInterestingWithMyAPI(dataPtr, size);
    dataPtr = nullptr;
    return 0;
}