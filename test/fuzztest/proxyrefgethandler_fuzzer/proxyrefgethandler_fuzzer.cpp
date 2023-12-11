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
#include "ecmascript/containers/containers_arraylist.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/object_factory.h"
#include "proxyrefgethandler_fuzzer.h"

using namespace panda;
using namespace panda::ecmascript;
#define MAXBYTELEN sizeof(int32_t)
namespace OHOS {
void ProxyRefGetHandler_fuzzer(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EcmaVM *vm_ = JSNApi::CreateJSVM(option);
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    ObjectFactory *factory = vm_->GetFactory();
    [[maybe_unused]] auto date1 = data;
    if (size <= 0) {
        return;
    }
    if (size > MAXBYTELEN) {
        size = MAXBYTELEN;
    }
    auto thread_ = vm_->GetAssociatedJSThread();
    JSHandle<JSTaggedValue> hclass(thread_, env->GetObjectFunction().GetObject<JSFunction>());
    JSHandle<JSTaggedValue> targetHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("x"));
    JSHandle<JSTaggedValue> value(thread_, JSTaggedValue(1));
    JSObject::SetProperty(thread_, targetHandle, key, value);
    JSHandle<JSTaggedValue> handlerHandle(
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
    JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread_, targetHandle, handlerHandle);
    JSHandle<JSTaggedValue> proxytagvalue = JSHandle<JSTaggedValue>::Cast(proxyHandle);
    Local<ProxyRef> object = JSNApiHelper::ToLocal<ProxyRef>(proxytagvalue);
    object->GetHandler(vm_);
    object->GetTarget(vm_);
    object->IsRevoked();
    JSNApi::DestroyJSVM(vm_);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::ProxyRefGetHandler_fuzzer(data, size);
    return 0;
}