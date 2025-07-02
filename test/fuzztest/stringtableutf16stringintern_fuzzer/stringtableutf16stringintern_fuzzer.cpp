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

#include "stringtableutf16stringintern_fuzzer.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_string_table_optimization-inl.h"
#include "common_components/base/utf_helper.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include <thread>
#include <vector>
#include <cstdint>
#include <optional>

using namespace panda;
using namespace panda::ecmascript;
using namespace common::utf_helper;

namespace OHOS {
    constexpr size_t ONE_BYTE = 1;
    constexpr size_t DOUBLE_BYTE = 2;
    constexpr size_t TRIPLE_BYTE = 3;
    constexpr size_t ONE_BYTE_BITS = 8;
    constexpr uint16_t REPLACEMENT_CHAR = 0xFFFD;

    struct CodeUnitResult {
        std::vector<uint16_t> units;
        size_t step;
    };

    CodeUnitResult ProcessCodeUnit(const uint8_t* data, size_t i, size_t size, bool bigEndian, uint16_t codeUnit)
    {
        CodeUnitResult result;
        if (codeUnit < 0xD800 || codeUnit > 0xDFFF) {
            result.units.push_back(codeUnit);
            result.step = DOUBLE_BYTE;
            return result;
        }

        if (codeUnit >= 0xD800 && codeUnit <= 0xDBFF) {
            if (i + TRIPLE_BYTE >= size) {
                result.units.push_back(REPLACEMENT_CHAR);
                result.step = DOUBLE_BYTE;
                return result;
            }

            uint16_t lowSurrogate;
            if (bigEndian) {
                lowSurrogate = (static_cast<uint16_t>(data[i + DOUBLE_BYTE]) << ONE_BYTE_BITS) | data[i + TRIPLE_BYTE];
            } else {
                lowSurrogate = (static_cast<uint16_t>(data[i + TRIPLE_BYTE]) << ONE_BYTE_BITS) | data[i + DOUBLE_BYTE];
            }

            if (lowSurrogate >= 0xDC00 && lowSurrogate <= 0xDFFF) {
                result.units.push_back(codeUnit);
                result.units.push_back(lowSurrogate);
                result.step = DOUBLE_BYTE * DOUBLE_BYTE;
                return result;
            }
        }

        result.units.push_back(REPLACEMENT_CHAR);
        result.step = DOUBLE_BYTE;
        return result;
    }

    std::vector<uint16_t> CreateValidUtf16(const uint8_t *data, size_t size)
    {
        std::vector<uint16_t> result;
        if (size == 0) {
            return result;
        }

        bool bigEndian = false;
        size_t startIndex = 0;
        if (size >= DOUBLE_BYTE) {
            if (data[0] == 0xFE && data[1] == 0xFF) {
                bigEndian = true;
                startIndex = DOUBLE_BYTE;
            } else if (data[0] == 0xFF && data[1] == 0xFE) {
                startIndex = DOUBLE_BYTE;
            }
        }

        size_t byteCount = size - startIndex;
        if (byteCount % DOUBLE_BYTE != 0) {
            byteCount--;
        }
        result.reserve(byteCount / DOUBLE_BYTE);

        size_t i = startIndex;
        while (i < startIndex + byteCount) {
            uint16_t codeUnit;
            if (bigEndian) {
                codeUnit = (static_cast<uint16_t>(data[i]) << ONE_BYTE_BITS) | data[i + ONE_BYTE];
            } else {
                codeUnit = (static_cast<uint16_t>(data[i + ONE_BYTE]) << ONE_BYTE_BITS) | data[i];
            }
            auto unitResult = ProcessCodeUnit(data, i, size, bigEndian, codeUnit);
            for (auto unit : unitResult.units) {
                result.push_back(unit);
            }
            i += unitResult.step;
        }
        return result;
    }

    void StringTableUtf16StringInternFuzzTest(const uint8_t *data, size_t size)
    {
        if (data == nullptr || size < DOUBLE_BYTE) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        ObjectFactory *factory = vm->GetFactory();
        EcmaStringTable *table = vm->GetEcmaStringTable();

        std::vector<uint16_t> utf16Data = CreateValidUtf16(data, size);
        JSHandle<EcmaString> str = factory->NewFromUtf16(utf16Data.data(), utf16Data.size());
        table->GetOrInternString(vm, utf16Data.data(), utf16Data.size(), false);
        table->GetOrInternString(vm, *str);

        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::StringTableUtf16StringInternFuzzTest(data, size);
    return 0;
}