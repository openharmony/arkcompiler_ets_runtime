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
#include <cstring>
#include <string>
#include <vector>
#include "aot_compiler_impl.h"
#include "securec.h"
#include "system_ability_definition.h"

using namespace OHOS::ArkCompiler;

namespace OHOS {
namespace {
bool ReadFuzzString(const char *data, size_t size, size_t &offset, std::string &out)
{
    if (offset + sizeof(uint8_t) >= size) {
        return false;
    }
    uint8_t len = static_cast<uint8_t>(data[offset]);
    offset += sizeof(uint8_t);
    if (offset + len >= size) {
        return false;
    }
    out = std::string(&data[offset], len);
    offset += len;
    return true;
}

bool ReadFuzzInt32(const char *data, size_t size, size_t &offset, int32_t &out)
{
    if (offset + sizeof(int32_t) >= size) {
        return false;
    }
    if (memcpy_s(&out, sizeof(int32_t), &data[offset], sizeof(int32_t)) != 0) {
        return false;
    }
    offset += sizeof(int32_t);
    return true;
}

bool ParseSigData(const char *data, size_t size, size_t &offset, std::vector<uint8_t> &sigData)
{
    while (offset + sizeof(uint8_t) < size) {
        uint8_t signalValue;
        if (memcpy_s(&signalValue, sizeof(uint8_t), &data[offset], sizeof(uint8_t)) != 0) {
            break;
        }
        sigData.push_back(signalValue);
        offset += sizeof(uint8_t);
    }
    return true;
}
} // namespace

bool DoSomethingInterestingWithMyAPI(const char *data, size_t size)
{
    AotCompilerArgs args;
    std::vector<uint8_t> sigData;
    size_t offset = 0;

    if (offset + sizeof(uint8_t) >= size) {
        return false;
    }
    args.isSysComp = static_cast<bool>(data[offset]);
    offset += sizeof(uint8_t);

    if (!ReadFuzzString(data, size, offset, args.compileMode)) {
        return false;
    }
    if (!ReadFuzzString(data, size, offset, args.moduleArkTSMode)) {
        return false;
    }
    if (!ReadFuzzString(data, size, offset, args.bundleName)) {
        return false;
    }
    if (!ReadFuzzString(data, size, offset, args.hostBundleName)) {
        return false;
    }
    if (!ReadFuzzString(data, size, offset, args.moduleName)) {
        return false;
    }
    if (!ReadFuzzInt32(data, size, offset, args.bundleUid)) {
        return false;
    }
    if (!ReadFuzzInt32(data, size, offset, args.bundleGid)) {
        return false;
    }
    if (!ReadFuzzInt32(data, size, offset, args.bundleType)) {
        return false;
    }
    if (!ReadFuzzString(data, size, offset, args.anFileName)) {
        return false;
    }
    ParseSigData(data, size, offset, sigData);

    AotCompilerImpl::GetInstance().EcmascriptAotCompiler(args, sigData);
    return true;
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    const char *dataPtr = reinterpret_cast<const char *>(data);
    OHOS::DoSomethingInterestingWithMyAPI(dataPtr, size);
    return 0;
}
