/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ecmascript/builtins/builtins_promise_job.h"
#include "ecmascript/global_env.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/module/napi_module_loader.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_array.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

using FunctionCallbackInfo = Local<JSValueRef> (*)(JsiRuntimeCallInfo *);
namespace panda::test {
class BuiltinsPromiseJobTest : public BaseTestWithScope<true> {
    public:
        static Local<JSValueRef> MockGetModule(JsiRuntimeCallInfo *runtimeCallInfo);
        static Local<JSValueRef> MockGetModuleJSError(JsiRuntimeCallInfo *runtimeCallInfo);
};

Local<JSValueRef> BuiltinsPromiseJobTest::MockGetModule(JsiRuntimeCallInfo *runtimeCallInfo)
{
    auto *thread = runtimeCallInfo->GetThread();
    auto vm = thread->GetEcmaVM();
    Local<StringRef> requestPath = StringRef::NewFromUtf8(vm, "requestPath");
    Local<ObjectRef> exportObejct = ObjectRef::New(vm);
    exportObejct->Set(vm, requestPath, runtimeCallInfo->GetCallArgRef(0));
    return exportObejct;
}

Local<JSValueRef> BuiltinsPromiseJobTest::MockGetModuleJSError(JsiRuntimeCallInfo *runtimeCallInfo)
{
    auto *thread = runtimeCallInfo->GetThread();
    auto vm = thread->GetEcmaVM();
    JsiFastNativeScope fastNativeScope(vm);
    Local<JSValueRef> error(JSValueRef::Undefined(vm));
    error = Exception::Error(vm, runtimeCallInfo->GetCallArgRef(0));
    Local<JSValueRef> codeKey = StringRef::NewFromUtf8(vm, "code");
    Local<JSValueRef> codeValue = runtimeCallInfo->GetCallArgRef(0);
    Local<ObjectRef> errorObj(error);
    errorObj->Set(vm, codeKey, codeValue);
    JSNApi::ThrowException(vm, error);
    return runtimeCallInfo->GetCallArgRef(0);
}

// dynamic import static module after load 1.0 module failed
HWTEST_F_L0(BuiltinsPromiseJobTest, DynamicImportJobCatchException)
{
    /**
     * Both the handle and the stack are allocated using maloc.
     * When newJsError is called, the C interpreter will step back one frame before executing.
     * In the UT, there is only one frame, and stepping back causes it to step on the handle address.
     * This is a special scenario caused by the UT, and it would not occur during normal execution.
     */
    if (!thread->IsAsmInterpreter()) {
        return;
    }
    auto vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();

    auto globalConstants = thread->GlobalConstants();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSTaggedValue> pandaObject(thread, arr);
    JSTaggedValue::SetProperty(thread, pandaObject,
        globalConstants->GetHandledGetModuleString(),
        JSNApiHelper::ToJSHandle(FunctionRef::New(const_cast<panda::EcmaVM*>(vm), MockGetModule)));
    Local<ObjectRef> globalObject = JSNApi::GetGlobalObject(vm);
    globalObject->Set(vm,
        JSNApiHelper::ToLocal<StringRef>(globalConstants->GetHandledPandaString()),
        JSNApiHelper::ToLocal<JSValueRef>(pandaObject));
    
    JSHandle<JSTaggedValue> promiseFunc = env->GetPromiseFunction();
    JSHandle<JSPromise> jsPromise =
        JSHandle<JSPromise>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(promiseFunc), promiseFunc));
    JSHandle<ResolvingFunctionsRecord> resolvingFunctions =
        JSPromise::CreateResolvingFunctions(thread, jsPromise);
    JSHandle<JSPromiseReactionsFunction> resolve(thread, resolvingFunctions->GetResolveFunction(thread));
    JSHandle<JSPromiseReactionsFunction> reject(thread, resolvingFunctions->GetRejectFunction(thread));
    JSHandle<JSTaggedValue> dirPath(factory->NewFromASCII("./main.abc"));
    JSHandle<JSTaggedValue> specifier(factory->NewFromASCII("exportFile"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 14);
    Local<JSValueRef> contextValue = JSNApi::GetCurrentContext(vm);
    JSHandle<LexicalEnv> lexicalEnv(JSNApiHelper::ToJSHandle(contextValue));
    JSHandle<JSFunction> funHandle = factory->NewJSFunction(env);
    funHandle->SetLexicalEnv(thread, lexicalEnv.GetTaggedValue());
    ecmaRuntimeCallInfo->SetFunction(funHandle.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, resolve.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, reject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(2, dirPath.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(3, specifier.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(4, JSTaggedValue::Undefined());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsPromiseJob::DynamicImportJob(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<JSTaggedValue> result(thread, jsPromise->GetPromiseResult(thread));
    EXPECT_EQ(result->IsJSProxy(), true);
    JSHandle<JSTaggedValue> requestPath(factory->NewFromASCII("requestPath"));
    EXPECT_EQ(JSTaggedValue::SameValue(thread, JSTaggedValue::GetProperty(thread, result, requestPath).GetValue(),
                                       specifier), true);
}

// throw 1.2 load failed
HWTEST_F_L0(BuiltinsPromiseJobTest, DynamicImportJobCatchException2)
{
    /**
     * Both the handle and the stack are allocated using maloc.
     * When newJsError is called, the C interpreter will step back one frame before executing.
     * In the UT, there is only one frame, and stepping back causes it to step on the handle address.
     * This is a special scenario caused by the UT, and it would not occur during normal execution.
     */
    if (!thread->IsAsmInterpreter()) {
        return;
    }
    auto vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();

    auto globalConstants = thread->GlobalConstants();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSTaggedValue> pandaObject(thread, arr);
    JSTaggedValue::SetProperty(thread, pandaObject,
        globalConstants->GetHandledGetModuleString(),
        JSNApiHelper::ToJSHandle(FunctionRef::New(const_cast<panda::EcmaVM*>(vm), MockGetModuleJSError)));
    Local<ObjectRef> globalObject = JSNApi::GetGlobalObject(vm);
    globalObject->Set(vm,
        JSNApiHelper::ToLocal<StringRef>(globalConstants->GetHandledPandaString()),
        JSNApiHelper::ToLocal<JSValueRef>(pandaObject));
    
    JSHandle<JSTaggedValue> promiseFunc = env->GetPromiseFunction();
    JSHandle<JSPromise> jsPromise =
        JSHandle<JSPromise>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(promiseFunc), promiseFunc));
    JSHandle<ResolvingFunctionsRecord> resolvingFunctions =
        JSPromise::CreateResolvingFunctions(thread, jsPromise);
    JSHandle<JSPromiseReactionsFunction> resolve(thread, resolvingFunctions->GetResolveFunction(thread));
    JSHandle<JSPromiseReactionsFunction> reject(thread, resolvingFunctions->GetRejectFunction(thread));
    JSHandle<JSTaggedValue> dirPath(factory->NewFromASCII("./main.abc"));
    JSHandle<JSTaggedValue> specifier(factory->NewFromASCII("exportFile"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 14);
    Local<JSValueRef> contextValue = JSNApi::GetCurrentContext(vm);
    JSHandle<LexicalEnv> lexicalEnv(JSNApiHelper::ToJSHandle(contextValue));
    JSHandle<JSFunction> funHandle = factory->NewJSFunction(env);
    funHandle->SetLexicalEnv(thread, lexicalEnv.GetTaggedValue());
    ecmaRuntimeCallInfo->SetFunction(funHandle.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, resolve.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, reject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(2, dirPath.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(3, specifier.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(4, JSTaggedValue::Undefined());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsPromiseJob::DynamicImportJob(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<JSTaggedValue> error(thread, jsPromise->GetPromiseResult(thread));
    JSHandle<JSTaggedValue> code(factory->NewFromASCII("code"));
    JSHandle<JSTaggedValue> message = JSTaggedValue::GetProperty(thread, error, code).GetValue();
    EXPECT_EQ(JSTaggedValue::SameValue(thread, message, specifier), true);
    thread->ClearException();
}
}
