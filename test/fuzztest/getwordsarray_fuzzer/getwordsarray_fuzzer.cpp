/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/base/string_helper.h"
#include "getwordsarray_fuzzer.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
    void GetWordsArrayFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            return;
        }
        bool sign = false;
        const size_t uint64BytesNum = 8;
        size_t wordsNum = size / uint64BytesNum;
        size_t hasRemain = size % uint64BytesNum;
        if (hasRemain) {
            wordsNum++;
        }
        uint64_t *words = new uint64_t[wordsNum]();
        if (memcpy_s(words, size, data, size) != EOK) {
            std::cout << "memcpy_s failed!";
            UNREACHABLE();
        }
        Local<JSValueRef> bigWordsValue = BigIntRef::CreateBigWords(vm, sign, wordsNum, words);
        Local<BigIntRef> bigWords(bigWordsValue);
        bigWords->GetWordsArray(vm, &sign, wordsNum, words);
        delete[] words;
        words = nullptr;
        JSNApi::DestroyJSVM(vm);
        return;
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::GetWordsArrayFuzzTest(data, size);
    return 0;
}