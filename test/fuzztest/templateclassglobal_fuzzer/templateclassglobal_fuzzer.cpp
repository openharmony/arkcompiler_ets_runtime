/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "templateclassglobal_fuzzer.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
    void TemplateGlobalFuzzerTest([[maybe_unused]]const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Local<BooleanRef> current = BooleanRef::New(vm, true);
        [[maybe_unused]]Global<JSValueRef> global(vm, current);
        JSNApi::DestroyJSVM(vm);
    }

    void TemplateToLocalFuzzerTest([[maybe_unused]]const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Global<BooleanRef> global(vm, BooleanRef::New(vm, true));
        [[maybe_unused]]Local<JSValueRef> local = global.ToLocal();
        JSNApi::DestroyJSVM(vm);
    }

    void TemplateToLocalFromVMFuzzerTest([[maybe_unused]]const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Global<BooleanRef> global(vm, BooleanRef::New(vm, true));
        [[maybe_unused]]Local<JSValueRef> local = global.ToLocal(vm);
        JSNApi::DestroyJSVM(vm);
    }

    void TemplateEmptyFuzzerTest([[maybe_unused]]const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Global<BooleanRef> global(vm, BooleanRef::New(vm, true));
        global.Empty();
        JSNApi::DestroyJSVM(vm);
    }

    void TemplateFreeGlobalHandleAddrFuzzerTest([[maybe_unused]]const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Global<BooleanRef> global(vm, BooleanRef::New(vm, true));
        global.FreeGlobalHandleAddr();
        JSNApi::DestroyJSVM(vm);
    }

    void GlobalOperatorStarFuzzerTest([[maybe_unused]]const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Global<BooleanRef> global(vm, BooleanRef::New(vm, true));
        [[maybe_unused]]bool b = (*global)->BooleaValue(vm);
        JSNApi::DestroyJSVM(vm);
    }

    void GlobalOperatorPointToFuzzerTest([[maybe_unused]]const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Global<BooleanRef> global(vm, BooleanRef::New(vm, true));
        [[maybe_unused]]bool b = global->BooleaValue(vm);
        JSNApi::DestroyJSVM(vm);
    }

    void GlobalIsEmptyFuzzerTest([[maybe_unused]]const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Global<JSValueRef> global;
        [[maybe_unused]]bool b = global.IsEmpty();
        JSNApi::DestroyJSVM(vm);
    }

    void GlobalSetWeakFuzzerTest([[maybe_unused]]const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Global<JSValueRef> global(vm, BooleanRef::New(vm, true));
        global.SetWeak();
        bool res = global.IsWeak();
        if (!res) {
            LOG_ECMA(ERROR) << "SetWeak failed";
            return;
        }
        
        JSNApi::DestroyJSVM(vm);
    }

    void GlobalClearWeakFuzzerTest([[maybe_unused]]const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Global<JSValueRef> global(vm, BooleanRef::New(vm, true));
        global.ClearWeak();
        JSNApi::DestroyJSVM(vm);
    }

    void GlobalIsWeakFuzzerTest([[maybe_unused]]const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Global<JSValueRef> global(vm, BooleanRef::New(vm, true));
        global.SetWeak();
        [[maybe_unused]]bool b = global.IsWeak();
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::TemplateGlobalFuzzerTest(data, size);
    OHOS::TemplateToLocalFuzzerTest(data, size);
    OHOS::TemplateToLocalFromVMFuzzerTest(data, size);
    OHOS::TemplateEmptyFuzzerTest(data, size);
    OHOS::TemplateFreeGlobalHandleAddrFuzzerTest(data, size);
    OHOS::GlobalOperatorStarFuzzerTest(data, size);
    OHOS::GlobalOperatorPointToFuzzerTest(data, size);
    OHOS::GlobalIsEmptyFuzzerTest(data, size);
    OHOS::GlobalSetWeakFuzzerTest(data, size);
    OHOS::GlobalClearWeakFuzzerTest(data, size);
    OHOS::GlobalIsWeakFuzzerTest(data, size);
    return 0;
}