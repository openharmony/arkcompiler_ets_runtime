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

#include "stringtableconcurrentloadstore_fuzzer.h"
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
    constexpr int DOUBLE_SEQUENCE = 2;
    constexpr int TRIPLE_SEQUENCE = 3;
    constexpr int FOUR_SEQUENCE = 4;
    constexpr size_t TWO_BYTE_BITS = 16;

    bool IsValidMultiByteSequence(const uint8_t* data, size_t start, int seqLen, size_t size)
    {
        if (start + seqLen > size) {
            return false;
        }

        for (int j = 1; j < seqLen; j++) {
            if ((data[start + j] & 0xC0) != 0x80) {
                return false;
            }
        }
        return true;
    }

    void AddReplacementCharacter(std::vector<uint8_t>& result)
    {
        result.push_back(0xEF);
        result.push_back(0xBF);
        result.push_back(0xBD);
    }

    std::vector<uint8_t> CreateValidUtf8(const uint8_t *data, size_t size)
    {
        std::vector<uint8_t> result;
        result.reserve(size);
        for (size_t i = 0; i < size;) {
            uint8_t byte = data[i];
            if (byte <= 0x7F) {
                result.push_back(byte);
                i++;
                continue;
            }
            int seqLen = 0;
            if ((byte & 0xE0) == 0xC0) {
                seqLen = DOUBLE_SEQUENCE;
            } else if ((byte & 0xF0) == 0xE0) {
                seqLen = TRIPLE_SEQUENCE;
            } else if ((byte & 0xF8) == 0xF0) {
                seqLen = FOUR_SEQUENCE;
            }

            if (seqLen == 0) {
                AddReplacementCharacter(result);
                i++;
                continue;
            }
            if (IsValidMultiByteSequence(data, i, seqLen, size)) {
                for (int j = 0; j < seqLen; j++) {
                    result.push_back(data[i + j]);
                }
                i += seqLen;
            } else {
                AddReplacementCharacter(result);
                i++;
            }
        }
        return result;
    }

    struct ThreadWorkerArgs {
        EcmaVM* vm;
        common::HashTrieMap<EcmaStringTableMutex, JSThread, common::TrieMapConfig::NeedSlotBarrier>* map;
        const std::vector<uint8_t>* utf8Data;
        size_t inputSize;
        size_t processedUtf8Len;
        int opsPerThread;
    };

    void ThreadWorker(const ThreadWorkerArgs& args)
    {
        JSThread* thread = args.vm->GetJSThread();
        for (int j = 0; j < args.opsPerThread; ++j) {
            uint32_t key = (args.inputSize + j) % 0xFFFFFFFF;
            size_t strLen = args.processedUtf8Len % TWO_BYTE_BITS;

            JSHandle<EcmaString> value(
                thread,
                EcmaStringAccessor::CreateFromUtf8(args.vm, args.utf8Data->data(), strLen, true));

            args.map->template LoadOrStore<true>(thread, key,
                                                 [value]() { return value; },
                                                 [](BaseString*) { return false; });
        }
    }

    void StringTableConcurrentLoadStoreFuzzTest(const uint8_t *data, size_t size)
    {
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);

        constexpr int threadCount = 4;
        constexpr int opsPerThread = 10;

        auto *map = new common::HashTrieMap<EcmaStringTableMutex, JSThread, common::TrieMapConfig::NeedSlotBarrier>();
        std::vector<uint8_t> utf8Data = CreateValidUtf8(data, size);
        size_t utf8Len = utf8Data.size();

        std::vector<std::thread> threads;
        threads.reserve(threadCount);
        ThreadWorkerArgs args{
                .vm = vm,
                .map = map,
                .utf8Data = &utf8Data,
                .inputSize = size,
                .processedUtf8Len = utf8Len,
                .opsPerThread = opsPerThread
        };

        for (int i = 0; i < threadCount; ++i) {
            threads.emplace_back(ThreadWorker, args);
        }
        for (auto &t : threads) {
            t.join();
        }
        delete map;
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::StringTableConcurrentLoadStoreFuzzTest(data, size);
    return 0;
}