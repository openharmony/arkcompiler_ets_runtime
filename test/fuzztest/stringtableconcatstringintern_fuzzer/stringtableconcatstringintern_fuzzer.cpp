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

#include "stringtableconcatstringintern_fuzzer.h"
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
    constexpr size_t MIN_BYTE_SIZE = 4;
    constexpr size_t MAX_RANDOM = 50;

    void StringTableConcatStringInternFuzzTest(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size < MIN_BYTE_SIZE) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        ObjectFactory* factory = vm->GetFactory();
        EcmaStringTable* table = vm->GetEcmaStringTable();

        size_t str1Size = std::min<size_t>(MAX_RANDOM, size / MIN_BYTE_SIZE);
        std::string prefix(reinterpret_cast<const char*>(data), str1Size);
        JSHandle<EcmaString> str1 = factory->NewFromStdString(prefix);

        size_t str2Size = std::min<size_t>(MAX_RANDOM, size / MIN_BYTE_SIZE);
        std::string suffix(reinterpret_cast<const char*>(data + str1Size), str2Size);
        JSHandle<EcmaString> str2 = factory->NewFromStdString(suffix);
        table->GetOrInternString(vm, str1, str2);

        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::StringTableConcatStringInternFuzzTest(data, size);
    return 0;
}