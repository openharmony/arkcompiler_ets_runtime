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

#include "ecmascript/global_env.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "proxyrefisrevoked_fuzzer.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
void ProxyRefIsRevokedFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    auto thread_ = vm->GetAssociatedJSThread();
    uint8_t *ptr = nullptr;
    ptr = const_cast<uint8_t *>(data);
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> hclass(thread_, env->GetObjectFunction().GetObject<JSFunction>());
    JSHandle<JSTaggedValue> targetHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("x"));
    JSHandle<JSTaggedValue> value(thread_, JSTaggedValue(size));
    JSObject::SetProperty(thread_, targetHandle, key, value);
    JSHandle<JSTaggedValue> handlerHandle(
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
    JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread_, targetHandle, handlerHandle);
    JSHandle<JSTaggedValue> proxyTagValue = JSHandle<JSTaggedValue>::Cast(proxyHandle);
    Local<ProxyRef> object = JSNApiHelper::ToLocal<ProxyRef>(proxyTagValue);
    object->IsRevoked();
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::ProxyRefIsRevokedFuzzTest(data, size);
    return 0;
}