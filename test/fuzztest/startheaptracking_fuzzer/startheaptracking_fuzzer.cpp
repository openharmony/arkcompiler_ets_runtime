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

#include "startheaptracking_fuzzer.h"

#include "ecmascript/dfx/hprof/file_stream.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/napi/include/jsnapi.h"


using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
    void StartHeapTrackingFuzzTest(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size < 0) {
            std::cout << "illegal input!";
            return;
        }
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        vm->SetEnableForceGC(false);

        auto factory = vm->GetFactory();
        bool isVmMode = true;
        bool traceAllocation = false;
        double timeInterval = 10; // 10 : time interval 10 ms
        ecmascript::FileStream *stream = nullptr;
        DFXJSNApi::StartHeapTracking(vm, timeInterval, isVmMode, stream, traceAllocation);

        sleep(1);
        int count = 100;
        while (count-- > 0) {
            JSHandle<EcmaString> string = factory->NewFromASCII("testString");
            factory->NewJSString(JSHandle<JSTaggedValue>(string));
        }
        const std::string filePath(data, data + size);
        std::fstream outputString(filePath, std::ios::out);
        outputString.close();
        outputString.clear();
        DFXJSNApi::StopHeapTracking(vm, filePath);
        std::remove(filePath.c_str());
        vm->SetEnableForceGC(true);
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::StartHeapTrackingFuzzTest(data, size);
    return 0;
}