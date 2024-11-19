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

#include "compilerinterfacestub_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include "aot_compiler_service.h"
#include "securec.h"
#include "system_ability_definition.h"

using namespace OHOS::ArkCompiler;

namespace OHOS {
constexpr int DATA_ZERO = 0;
constexpr int DATA_ONE = 1;
constexpr int DATA_TWO = 2;
constexpr int DATA_THREE = 3;
constexpr size_t OFFSET_ONE = 8;
constexpr size_t OFFSET_TWO = 16;
constexpr size_t OFFSET_THREE = 24;
constexpr size_t MESSAGE_SIZE = MIN_TRANSACTION_ID + 4; // 4 code methods OnRemoteRequest
constexpr size_t U32_AT_SIZE = 4;
static AotCompilerService compilerService(AOT_COMPILER_SERVICE_ID, false);

uint32_t GetU32Data(const char* ptr)
{
    return (ptr[DATA_ZERO] << OFFSET_THREE) | (ptr[DATA_ONE] << OFFSET_TWO) |
           (ptr[DATA_TWO] << OFFSET_ONE) | (ptr[DATA_THREE]);
}

bool DoSomethingInterestingWithMyAPI(const char* data, size_t size)
{
    uint32_t code = (GetU32Data(data) % MESSAGE_SIZE);
    MessageParcel datas;
    std::u16string descriptor = AotCompilerService::GetDescriptor();
    datas.WriteInterfaceToken(descriptor);
    datas.WriteBuffer(data, size);
    datas.RewindRead(0);
    MessageParcel reply;
    MessageOption option;
    compilerService.OnRemoteRequest(code, datas, reply, option);
    return true;
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    if (data == nullptr) {
        return 0;
    }

    if (size < OHOS::U32_AT_SIZE) {
        return 0;
    }

    char* dataPtr = static_cast<char*>(malloc(size + 1));
    if (dataPtr == nullptr) {
        return 0;
    }

    (void)memset_s(dataPtr, size + 1, 0x00, size + 1);
    if (memcpy_s(dataPtr, size + 1, data, size) != EOK) {
        free(dataPtr);
        dataPtr = nullptr;
        return 0;
    }

    OHOS::DoSomethingInterestingWithMyAPI(dataPtr, size);
    free(dataPtr);
    dataPtr = nullptr;
    return 0;
}