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

#include "promiserejectinfo_fuzzer.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/base/utf_helper.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::base::utf_helper;

namespace OHOS {
    void PromiseRejectInfoNewFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Local<StringRef> promiseStirng = StringRef::NewFromUtf8(vm, (char*)data, (int)size);
        Local<JSValueRef> promise(promiseStirng);
        Local<StringRef> reasonString = StringRef::NewFromUtf8(vm, (char*)data, (int)size);
        Local<JSValueRef> reason(reasonString);
        void *data2 = static_cast<void*>(new std::string("test"));
        [[maybe_unused]]PromiseRejectInfo promiseReject(promise, reason,
            PromiseRejectInfo::PROMISE_REJECTION_EVENT::REJECT, data2);
        JSNApi::DestroyJSVM(vm);
    }

    void GetDataFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Local<StringRef> promiseStirng = StringRef::NewFromUtf8(vm, (char*)data, (int)size);
        Local<JSValueRef> promise(promiseStirng);
        Local<StringRef> reasonString = StringRef::NewFromUtf8(vm, (char*)data, (int)size);
        Local<JSValueRef> reason(reasonString);
        PromiseRejectInfo promiseReject(promise, reason,
            PromiseRejectInfo::PROMISE_REJECTION_EVENT::REJECT, (void*)data);
        promiseReject.GetData();
        JSNApi::DestroyJSVM(vm);
    }

    void PromiseRejectInfoGetPromiseFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Local<StringRef> promiseStirng = StringRef::NewFromUtf8(vm, (char*)data, (int)size);
        Local<JSValueRef> promise(promiseStirng);
        Local<StringRef> reasonString = StringRef::NewFromUtf8(vm, (char*)data, (int)size);
        Local<JSValueRef> reason(reasonString);
        void *newdata = static_cast<void*>(new std::string("test"));
        PromiseRejectInfo promiseReject(promise, reason, PromiseRejectInfo::PROMISE_REJECTION_EVENT::REJECT, newdata);
        promiseReject.GetPromise();
        JSNApi::DestroyJSVM(vm);
    }

    void PromiseRejectInfoGetReasonFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Local<StringRef> promiseStirng = StringRef::NewFromUtf8(vm, (char*)data, (int)size);
        Local<JSValueRef> promise(promiseStirng);
        Local<StringRef> reasonString = StringRef::NewFromUtf8(vm, (char*)data, (int)size);
        Local<JSValueRef> reason(reasonString);
        void *data2 = static_cast<void*>(new std::string("test"));
        PromiseRejectInfo promiseReject(promise, reason, PromiseRejectInfo::PROMISE_REJECTION_EVENT::REJECT, data2);
        [[maybe_unused]]Local<JSValueRef> reason_res = promiseReject.GetReason();
        JSNApi::DestroyJSVM(vm);
    }

    void PromiseRejectInfoGetOperationFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Local<StringRef> promiseStirng = StringRef::NewFromUtf8(vm, (char*)data, (int)size);
        Local<JSValueRef> promise(promiseStirng);
        Local<StringRef> reasonString = StringRef::NewFromUtf8(vm, (char*)data, (int)size);
        Local<JSValueRef> reason(reasonString);
        void *data2 = static_cast<void*>(new std::string("test"));
        PromiseRejectInfo promiseReject(promise, reason, PromiseRejectInfo::PROMISE_REJECTION_EVENT::REJECT, data2);
        promiseReject.GetOperation();
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::PromiseRejectInfoNewFuzzTest(data, size);
    OHOS::GetDataFuzzTest(data, size);
    OHOS::PromiseRejectInfoGetPromiseFuzzTest(data, size);
    OHOS::PromiseRejectInfoGetReasonFuzzTest(data, size);
    OHOS::PromiseRejectInfoGetOperationFuzzTest(data, size);
    return 0;
}