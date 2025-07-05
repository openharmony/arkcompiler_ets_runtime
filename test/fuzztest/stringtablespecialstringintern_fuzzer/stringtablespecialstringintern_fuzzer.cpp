/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "stringtablespecialstringintern_fuzzer.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_string_table_optimization-inl.h"
#include "common_components/base/utf_helper.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include <thread>
#include <vector>

using namespace panda;
using namespace panda::ecmascript;
using namespace common::utf_helper;

namespace OHOS {
    constexpr size_t VALID_MIN_SIZE = 2;
    constexpr int ALIGNMENT = 1;

    void StringTableSpecialStringInternFuzzTest(const uint8_t *data, size_t size)
    {
        if (data == nullptr || size <= VALID_MIN_SIZE) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);

        JSThread *thread = vm->GetJSThread();
        ObjectFactory *factory = vm->GetFactory();
        EcmaStringTable *table = vm->GetEcmaStringTable();

        // empty string
        JSHandle<EcmaString> emptyStr(thread, EcmaStringAccessor::CreateEmptyString(vm));
        table->GetOrInternFlattenString(vm, *emptyStr);
        table->TryGetInternString(thread, emptyStr);

        // no gc pattern
        uint8_t tsData[] = {0x74, 0x65, 0x73, 0x74};
        table->GetOrInternFlattenStringNoGC(vm, EcmaStringAccessor::CreateFromUtf8(vm, tsData, sizeof(tsData), true));

        // substring, random offset
        JSHandle<EcmaString> baseStr = factory->NewFromASCII("base_string_for_substring_testing");
        uint32_t offset = data[0] % (EcmaStringAccessor(*baseStr).GetLength() + ALIGNMENT);
        uint32_t length = data[1] % (EcmaStringAccessor(*baseStr).GetLength() - offset + ALIGNMENT);
        table->GetOrInternStringFromCompressedSubString(vm, baseStr, offset, length);

        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::StringTableSpecialStringInternFuzzTest(data, size);
    return 0;
}