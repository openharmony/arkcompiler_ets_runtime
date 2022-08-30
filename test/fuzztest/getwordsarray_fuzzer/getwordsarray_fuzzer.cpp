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
    bool GetWordsArrayFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        bool sign = true;
        const size_t uint64BytesNum = 8;
        if (size < uint64BytesNum) {
            uint64_t words = 0;
            if (memcpy_s(&words, uint64BytesNum, data, size) != EOK) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            Local<JSValueRef> bigWordsValue = BigIntRef::CreateBigWords(vm, sign, 1U, &words); // 1 : single word
            uint64_t *wordsArray = new uint64_t[1]();
            Local<BigIntRef> bigWords(bigWordsValue);
            bigWords->GetWordsArray(&sign, 1U, wordsArray);
            JSNApi::DestroyJSVM(vm);
            return true;
        }

        size_t wordsNum = size / uint64BytesNum;
        size_t hasRemain = size % uint64BytesNum;
        if (hasRemain) {
            wordsNum++;
        }
        std::vector<uint64_t> wordsVec;
        size_t count = uint64BytesNum;
        for (uint32_t i = 0; i < wordsNum; i++) {
            uint64_t word = 0;
            if (hasRemain && (i == (wordsNum - 1U))) {
                count = hasRemain;
            }
            if (memcpy_s(&word, uint64BytesNum, data, count) != EOK) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            wordsVec.push_back(word);
            data += count;
        }
        uint64_t *words = new uint64_t[wordsNum]();
        std::copy(wordsVec.begin(), wordsVec.end(), words);
        Local<JSValueRef> bigWordsValue = BigIntRef::CreateBigWords(vm, sign, wordsNum, words);
        uint64_t *wordsArray = new uint64_t[wordsNum]();
        Local<BigIntRef> bigWords(bigWordsValue);
        bigWords->GetWordsArray(&sign, wordsNum, wordsArray);
        JSNApi::DestroyJSVM(vm);
        return true;
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::GetWordsArrayFuzzTest(data, size);
    return 0;
}