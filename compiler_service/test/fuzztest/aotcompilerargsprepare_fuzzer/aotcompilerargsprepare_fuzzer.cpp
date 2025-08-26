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
bool DoSomethingInterestingWithMyAPI(const char* data, size_t size)
{
    std::unordered_map<std::string, std::string> argsMap;
    std::vector<int16_t> sigData;
    size_t offset = 0;
    // check if there is enough data
    if (offset + sizeof(int16_t) >= size) {
        return false;
    }
    uint8_t numberOfArgs = static_cast<uint8_t>(data[offset]);

    // parse argsMap
    offset += sizeof(uint8_t);
    for (uint8_t i = 0; i < numberOfArgs && offset < size; ++i) {
        // read the key length
        if (offset + sizeof(int16_t) >= size) {
            break;
        }
        uint8_t keyLength = static_cast<uint8_t>(data[offset]);
        offset += sizeof(uint8_t);
        // read the key
        if (offset + keyLength >= size) {
            break;
        }
        std::string key(&data[offset], keyLength);
        offset += keyLength;
        // read the value length
        if (offset + sizeof(uint8_t) >= size) {
            break;
        }
        uint8_t valueLength = static_cast<uint8_t>(data[offset]);
        offset += sizeof(uint8_t);
        // read the value
        if (offset + valueLength >= size) {
            break;
        }
        std::string value(&data[offset], valueLength);
        offset += valueLength;
        argsMap.emplace(std::move(key), std::move(value));
    }

    // parse sigData
    while (offset + sizeof(int16_t) < size) {
        int16_t signalValue;
        if (memcpy_s(&signalValue, sizeof(int16_t), &data[offset], sizeof(int16_t)) != 0) {
            break;
        }
        sigData.push_back(signalValue);
        offset += sizeof(int16_t);
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
    return 0;
}