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

#include "ecmascript/js_object-inl.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/module/static/static_module_loader.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/module/napi_module_loader.h"
#include "ecmascript/js_array.h"
#include "ecmascript/global_env.h"
#include "ecmascript/module/module_path_helper.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;
using FunctionCallbackInfo = Local<JSValueRef> (*)(JsiRuntimeCallInfo *);
using namespace panda::ecmascript;
using ModulePathHelper = ecmascript::ModulePathHelper;

namespace panda::test {
class StaticModuleLoaderTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }
    static Local<JSValueRef> MockGetModule(JsiRuntimeCallInfo *runtimeCallInfo);
    static Local<JSValueRef> MockGetModuleJSError(JsiRuntimeCallInfo *runtimeCallInfo);
    EcmaVM *instance {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

Local<JSValueRef> StaticModuleLoaderTest::MockGetModule(JsiRuntimeCallInfo *runtimeCallInfo)
{
    auto *thread = runtimeCallInfo->GetThread();
    auto vm = thread->GetEcmaVM();
    Local<StringRef> requestPath = StringRef::NewFromUtf8(vm, "requestPath");
    Local<ObjectRef> exportObejct = ObjectRef::New(vm);
    exportObejct->Set(vm, requestPath, runtimeCallInfo->GetCallArgRef(0));
    return exportObejct;
}


Local<JSValueRef> StaticModuleLoaderTest::MockGetModuleJSError(JsiRuntimeCallInfo *runtimeCallInfo)
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

HWTEST_F_L0(StaticModuleLoaderTest, CanTryLoadStaticModulePath)
{
    CString inputFileName = "./test";
    bool res1 = StaticModuleLoader::CanTryLoadStaticModulePath(inputFileName);
    EXPECT_FALSE(res1);

    inputFileName = "test";
    bool res2 = StaticModuleLoader::CanTryLoadStaticModulePath(inputFileName);
    EXPECT_TRUE(res2);

    inputFileName = "test.js";
    bool res3 = StaticModuleLoader::CanTryLoadStaticModulePath(inputFileName);
    EXPECT_FALSE(res3);

    inputFileName = "test.ts";
    bool res4 = StaticModuleLoader::CanTryLoadStaticModulePath(inputFileName);
    EXPECT_FALSE(res4);

    inputFileName = "test.ets";
    bool res5 = StaticModuleLoader::CanTryLoadStaticModulePath(inputFileName);
    EXPECT_FALSE(res5);

    inputFileName = "@bundle:";
    bool res6 = StaticModuleLoader::CanTryLoadStaticModulePath(inputFileName);
    EXPECT_FALSE(res6);

    inputFileName = "@normalized:";
    bool res7 = StaticModuleLoader::CanTryLoadStaticModulePath(inputFileName);
    EXPECT_FALSE(res7);

    inputFileName = "@package:";
    bool res8 = StaticModuleLoader::CanTryLoadStaticModulePath(inputFileName);
    EXPECT_FALSE(res8);
}

HWTEST_F_L0(StaticModuleLoaderTest, GetStaticModuleLoadFunc)
{
    auto vm = thread->GetEcmaVM();
    Local<JSValueRef> undefinedFunc =  StaticModuleLoader::GetStaticModuleLoadFunc(vm);
    EXPECT_TRUE(undefinedFunc->IsUndefined());

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
    Local<JSValueRef> func =  StaticModuleLoader::GetStaticModuleLoadFunc(vm);
    EXPECT_FALSE(func->IsUndefined());
}

HWTEST_F_L0(StaticModuleLoaderTest, LoadStaticModule)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();

    Local<FunctionRef> func = FunctionRef::New(const_cast<panda::EcmaVM *>(vm), MockGetModule);
    JSHandle<JSTaggedValue> specifier(factory->NewFromASCII("exportFile"));
    JSHandle<EcmaString> specifierString = JSTaggedValue::ToString(thread, specifier);
    CString path = ModulePathHelper::Utf8ConvertToString(thread, specifierString.GetTaggedValue());
    JSHandle<JSTaggedValue>  result = StaticModuleLoader::LoadStaticModule(thread, func, path);

    EXPECT_EQ(result->IsJSProxy(), true);
    JSHandle<JSTaggedValue> requestPath(factory->NewFromASCII("requestPath"));
    EXPECT_EQ(JSTaggedValue::SameValue(thread, JSTaggedValue::GetProperty(thread, result, requestPath).GetValue(),
                                       specifier), true);
}


HWTEST_F_L0(StaticModuleLoaderTest, LoadStaticModuleError)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();

    Local<FunctionRef> func = FunctionRef::New(const_cast<panda::EcmaVM *>(vm), MockGetModuleJSError);
    JSHandle<JSTaggedValue> specifier(factory->NewFromASCII("exportFile"));
    JSHandle<EcmaString> specifierString = JSTaggedValue::ToString(thread, specifier);
    CString path = ModulePathHelper::Utf8ConvertToString(thread, specifierString.GetTaggedValue());
    StaticModuleLoader::LoadStaticModule(thread, func, path);
    EXPECT_EQ(thread->HasPendingException(), true);
    JSHandle<JSTaggedValue> error(thread, thread->GetException());
    JSHandle<JSTaggedValue> code(factory->NewFromASCII("code"));
    JSHandle<JSTaggedValue> message = JSTaggedValue::GetProperty(thread, error, code).GetValue();
    EXPECT_EQ(JSTaggedValue::SameValue(thread, message, specifier), true);
    thread->ClearException();
}

}  // namespace panda::test
