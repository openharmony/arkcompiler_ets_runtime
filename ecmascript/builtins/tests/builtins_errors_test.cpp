/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/builtins/builtins_errors.h"

#include "ecmascript/base/error_helper.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"

#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value-inl.h"

#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
using Error = ecmascript::builtins::BuiltinsError;
using RangeError = builtins::BuiltinsRangeError;
using ReferenceError = builtins::BuiltinsReferenceError;
using TypeError = builtins::BuiltinsTypeError;
using URIError = builtins::BuiltinsURIError;
using EvalError = builtins::BuiltinsEvalError;
using SyntaxError = builtins::BuiltinsSyntaxError;
using AggregateError = builtins::BuiltinsAggregateError;
using JSType = ecmascript::JSType;

class BuiltinsErrorsTest : public BaseTestWithScope<false> {
};

/*
 * @tc.name: GetJSErrorObject
 * @tc.desc: get JSError Object
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, GetJSErrorObject)
{
    /**
     * @tc.steps: step1. Create JSError object
     */
    ObjectFactory *factory = instance->GetFactory();

    JSHandle<JSObject> handleObj = factory->GetJSError(ErrorType::TYPE_ERROR);
    JSHandle<JSTaggedValue> msgKey(factory->NewFromASCII("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    /**
     * @tc.steps: step2. obtain JSError object prototype chain name property and message property
     */
    JSHandle<JSTaggedValue> msgValue(
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(handleObj), msgKey).GetValue());
    EXPECT_EQ(EcmaStringAccessor::Compare(instance,
        JSHandle<EcmaString>(msgValue), factory->NewFromASCII("")), 0);
    JSHandle<JSTaggedValue> nameValue(
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(handleObj), nameKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("TypeError"), JSHandle<EcmaString>(nameValue)), 0);
}

/*
 * @tc.name: GetJSErrorWithMessage
 * @tc.desc: Obtains the TypeError object.
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, GetJSErrorWithMessage)
{
    ObjectFactory *factory = instance->GetFactory();

    JSHandle<JSObject> handleObj = factory->GetJSError(ErrorType::TYPE_ERROR, "I am type error");
    JSHandle<JSTaggedValue> msgKey(factory->NewFromASCII("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();
    JSHandle<JSTaggedValue> msgValue(
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(handleObj), msgKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("I am type error"), JSHandle<EcmaString>(msgValue)), 0);
    JSHandle<JSTaggedValue> nameValue(
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(handleObj), nameKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("TypeError"), JSHandle<EcmaString>(nameValue)), 0);
}

/*
 * @tc.name: ErrorNoParameterConstructor
 * @tc.desc: new Error()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, ErrorNoParameterConstructor)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetErrorFunction());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 4);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = Error::ErrorConstructor(ecmaRuntimeCallInfo);

    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromASCII("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII(""), JSHandle<EcmaString>(msgValue)), 0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("Error"), JSHandle<EcmaString>(nameValue)), 0);
}

/*
 * @tc.name: ErrorParameterConstructor
 * @tc.desc: new Error("Hello Error!")
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, ErrorParameterConstructor)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetErrorFunction());
    JSHandle<JSTaggedValue> paramMsg(factory->NewFromASCII("Hello Error!"));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 6);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));
    ecmaRuntimeCallInfo->SetCallArg(0, paramMsg.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = Error::ErrorConstructor(ecmaRuntimeCallInfo);

    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromASCII("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();
    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());

    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("Hello Error!"), JSHandle<EcmaString>(msgValue)), 0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("Error"), JSHandle<EcmaString>(nameValue)), 0);
}

/*
 * @tc.name: ErrorNoParameterToString
 * @tc.desc: new Error().toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, ErrorNoParameterToString)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = Error::ToString(ecmaRuntimeCallInfo);

    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, factory->NewFromASCII("Error"), resultHandle), 0);
}

/*
 * @tc.name: ErrorToString
 * @tc.desc: new Error("This is Error!").toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, ErrorToString)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    JSHandle<JSTaggedValue> handleMsgKey(factory->NewFromASCII("message"));
    JSObject::SetProperty(
        thread, JSHandle<JSTaggedValue>(error), handleMsgKey,
        JSHandle<JSTaggedValue>(thread, factory->NewFromASCII("This is Error!").GetTaggedValue()));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = Error::ToString(ecmaRuntimeCallInfo);

    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, factory->NewFromASCII("Error: This is Error!"), resultHandle), 0);
}

/*
 * @tc.name: RangeErrorNoParameterConstructor
 * @tc.desc: new RangeError()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, RangeErrorNoParameterConstructor)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetRangeErrorFunction());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = RangeError::RangeErrorConstructor(ecmaRuntimeCallInfo);

    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromASCII("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII(""), JSHandle<EcmaString>(msgValue)), 0);
    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("RangeError"), JSHandle<EcmaString>(nameValue)), 0);
}

/*
 * @tc.name: RangeErrorParameterConstructor
 * @tc.desc: new RangeError("Hello RangeError!")
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, RangeErrorParameterConstructor)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetRangeErrorFunction());
    JSHandle<JSTaggedValue> paramMsg(factory->NewFromASCII("Hello RangeError!"));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 6);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));
    ecmaRuntimeCallInfo->SetCallArg(0, paramMsg.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = RangeError::RangeErrorConstructor(ecmaRuntimeCallInfo);

    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromASCII("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("Hello RangeError!"), JSHandle<EcmaString>(msgValue)), 0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("RangeError"), JSHandle<EcmaString>(nameValue)), 0);
}

/*
 * @tc.name: RangeErrorNoParameterToString
 * @tc.desc: new RangeError().toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, RangeErrorNoParameterToString)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetRangeErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = RangeError::ToString(ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> resultHandle(thread, result);

    EXPECT_TRUE(result.IsString());

    EXPECT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("RangeError"), JSHandle<EcmaString>(resultHandle)), 0);
}

/*
 * @tc.name: RangeErrorToString
 * @tc.desc: new RangeError("This is RangeError!").toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, RangeErrorToString)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetRangeErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    JSHandle<JSTaggedValue> handleMsgKey(factory->NewFromASCII("message"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(error), handleMsgKey,
                          JSHandle<JSTaggedValue>(factory->NewFromASCII("This is RangeError!")));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = RangeError::ToString(ecmaRuntimeCallInfo);

    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("RangeError: This is RangeError!"), resultHandle), 0);
}

// new ReferenceError()
/*
 * @tc.name: ReferenceErrorNoParameterConstructor
 * @tc.desc: new ReferenceError()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, ReferenceErrorNoParameterConstructor)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetReferenceErrorFunction());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 4);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = ReferenceError::ReferenceErrorConstructor(ecmaRuntimeCallInfo);
    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromASCII("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII(""), JSHandle<EcmaString>(msgValue)), 0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("ReferenceError"), JSHandle<EcmaString>(nameValue)), 0);
}

/*
 * @tc.name: ReferenceErrorParameterConstructor
 * @tc.desc: new ReferenceError("Hello RangeError!")
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, ReferenceErrorParameterConstructor)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetReferenceErrorFunction());
    JSHandle<JSTaggedValue> paramMsg(factory->NewFromASCII("Hello ReferenceError!"));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 6);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));
    ecmaRuntimeCallInfo->SetCallArg(0, paramMsg.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = ReferenceError::ReferenceErrorConstructor(ecmaRuntimeCallInfo);
    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromASCII("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();
    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("Hello ReferenceError!"), JSHandle<EcmaString>(msgValue)), 0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("ReferenceError"), JSHandle<EcmaString>(nameValue)), 0);
}

/*
 * @tc.name: ReferenceErrorNoParameterToString
 * @tc.desc: new ReferenceError().toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, ReferenceErrorNoParameterToString)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetReferenceErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = ReferenceError::ToString(ecmaRuntimeCallInfo);
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, factory->NewFromASCII("ReferenceError"), resultHandle), 0);
}

/*
 * @tc.name: ReferenceErrorToString
 * @tc.desc: new ReferenceError("This is ReferenceError!").toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, ReferenceErrorToString)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetReferenceErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    JSHandle<JSTaggedValue> handleMsgKey(factory->NewFromASCII("message"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(error), handleMsgKey,
                          JSHandle<JSTaggedValue>(factory->NewFromASCII("This is ReferenceError!")));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = ReferenceError::ToString(ecmaRuntimeCallInfo);
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("ReferenceError: This is ReferenceError!"), resultHandle), 0);
}

/*
 * @tc.name: TypeErrorNoParameterConstructor
 * @tc.desc: new TypeError()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, TypeErrorNoParameterConstructor)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetTypeErrorFunction());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 4);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = TypeError::TypeErrorConstructor(ecmaRuntimeCallInfo);
    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromASCII("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII(""), JSHandle<EcmaString>(msgValue)), 0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    EXPECT_EQ(EcmaStringAccessor::Compare(instance,
        JSHandle<EcmaString>(nameValue), JSHandle<EcmaString>(nameValue)), 0);
}

/*
 * @tc.name: TypeErrorParameterConstructor
 * @tc.desc: new TypeError("Hello RangeError!")
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, TypeErrorParameterConstructor)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetTypeErrorFunction());
    JSHandle<JSTaggedValue> paramMsg(factory->NewFromASCII("Hello TypeError!"));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 6);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));
    ecmaRuntimeCallInfo->SetCallArg(0, paramMsg.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = TypeError::TypeErrorConstructor(ecmaRuntimeCallInfo);
    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromASCII("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("Hello TypeError!"), JSHandle<EcmaString>(msgValue)), 0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("TypeError"), JSHandle<EcmaString>(nameValue)), 0);
}

/*
 * @tc.name: TypeErrorNoParameterToString
 * @tc.desc: new TypeError().toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, TypeErrorNoParameterToString)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetTypeErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = TypeError::ToString(ecmaRuntimeCallInfo);
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, factory->NewFromASCII("TypeError"), resultHandle), 0);
}

/*
 * @tc.name: TypeErrorToString
 * @tc.desc: new TypeError("This is TypeError!").toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, TypeErrorToString)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetTypeErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    JSHandle<JSTaggedValue> value(factory->NewFromASCII("This is TypeError!"));
    JSHandle<JSTaggedValue> handleMsgKey(factory->NewFromASCII("message"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(error), handleMsgKey, value);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = TypeError::ToString(ecmaRuntimeCallInfo);
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("TypeError: This is TypeError!"), resultHandle), 0);
}

/*
 * @tc.name: URIErrorNoParameterConstructor
 * @tc.desc: new URIError()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, URIErrorNoParameterConstructor)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetURIErrorFunction());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 4);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = URIError::URIErrorConstructor(ecmaRuntimeCallInfo);
    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromASCII("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII(""), JSHandle<EcmaString>(msgValue)), 0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("URIError"), JSHandle<EcmaString>(nameValue)), 0);
}

/*
 * @tc.name: URIErrorParameterConstructor
 * @tc.desc: new URIError("Hello RangeError!")
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, URIErrorParameterConstructor)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetURIErrorFunction());
    JSHandle<JSTaggedValue> paramMsg(factory->NewFromASCII("Hello URIError!"));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 6);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));
    ecmaRuntimeCallInfo->SetCallArg(0, paramMsg.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = URIError::URIErrorConstructor(ecmaRuntimeCallInfo);
    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromASCII("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("Hello URIError!"), JSHandle<EcmaString>(msgValue)), 0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("URIError"), JSHandle<EcmaString>(nameValue)), 0);
}

/*
 * @tc.name: URIErrorNoParameterToString
 * @tc.desc: new URIError().toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, URIErrorNoParameterToString)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetURIErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = URIError::ToString(ecmaRuntimeCallInfo);
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());

    EXPECT_EQ(EcmaStringAccessor::Compare(instance, factory->NewFromASCII("URIError"), resultHandle), 0);
}

/*
 * @tc.name: URIErrorToString
 * @tc.desc: new URIError("This is URIError!").toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, URIErrorToString)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetURIErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    JSHandle<JSTaggedValue> handleMsgKey(factory->NewFromASCII("message"));
    JSObject::SetProperty(
        thread, JSHandle<JSTaggedValue>(error), handleMsgKey,
        JSHandle<JSTaggedValue>(thread, factory->NewFromASCII("This is URIError!").GetTaggedValue()));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = URIError::ToString(ecmaRuntimeCallInfo);
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());

    EXPECT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("URIError: This is URIError!"), resultHandle), 0);
}

/*
 * @tc.name: SyntaxErrorNoParameterConstructor
 * @tc.desc: new SyntaxError()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, SyntaxErrorNoParameterConstructor)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetSyntaxErrorFunction());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 4);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = SyntaxError::SyntaxErrorConstructor(ecmaRuntimeCallInfo);
    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromASCII("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII(""), JSHandle<EcmaString>(msgValue)), 0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("SyntaxError"), JSHandle<EcmaString>(nameValue)), 0);
}

/*
 * @tc.name: SyntaxErrorParameterConstructor
 * @tc.desc: new SyntaxError("Hello RangeError!")
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, SyntaxErrorParameterConstructor)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetSyntaxErrorFunction());
    JSHandle<JSTaggedValue> paramMsg(factory->NewFromASCII("Hello SyntaxError!"));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 6);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));
    ecmaRuntimeCallInfo->SetCallArg(0, paramMsg.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = SyntaxError::SyntaxErrorConstructor(ecmaRuntimeCallInfo);
    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromASCII("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("Hello SyntaxError!"), JSHandle<EcmaString>(msgValue)), 0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("SyntaxError"), JSHandle<EcmaString>(nameValue)), 0);
}

/*
 * @tc.name: SyntaxErrorNoParameterToString
 * @tc.desc: new SyntaxError().toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, SyntaxErrorNoParameterToString)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetSyntaxErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = SyntaxError::ToString(ecmaRuntimeCallInfo);
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());

    EXPECT_EQ(EcmaStringAccessor::Compare(instance, factory->NewFromASCII("SyntaxError"), resultHandle), 0);
}

/*
 * @tc.name: SyntaxErrorToString
 * @tc.desc: new SyntaxError("This is SyntaxError!").toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, SyntaxErrorToString)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetSyntaxErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    JSHandle<JSTaggedValue> handleMsgKey(factory->NewFromASCII("message"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(error), handleMsgKey,
                          JSHandle<JSTaggedValue>(factory->NewFromASCII("This is SyntaxError!")));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = SyntaxError::ToString(ecmaRuntimeCallInfo);
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());

    EXPECT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("SyntaxError: This is SyntaxError!"), resultHandle), 0);
}

/*
 * @tc.name: EvalErrorNoParameterConstructor
 * @tc.desc: new EvalError()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, EvalErrorNoParameterConstructor)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetEvalErrorFunction());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 4);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = EvalError::EvalErrorConstructor(ecmaRuntimeCallInfo);
    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromASCII("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII(""), JSHandle<EcmaString>(msgValue)), 0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("EvalError"), JSHandle<EcmaString>(nameValue)), 0);
}

/*
 * @tc.name: EvalErrorParameterConstructor
 * @tc.desc: new EvalError("Hello RangeError!")
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, EvalErrorParameterConstructor)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetEvalErrorFunction());
    JSHandle<JSTaggedValue> paramMsg(factory->NewFromASCII("Hello EvalError!"));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 6);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));
    ecmaRuntimeCallInfo->SetCallArg(0, paramMsg.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = EvalError::EvalErrorConstructor(ecmaRuntimeCallInfo);
    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromASCII("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("Hello EvalError!"), JSHandle<EcmaString>(msgValue)), 0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("EvalError"), JSHandle<EcmaString>(nameValue)), 0);
}

/*
 * @tc.name: EvalErrorNoParameterToString
 * @tc.desc: new EvalError().toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, EvalErrorNoParameterToString)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();
    JSHandle<JSTaggedValue> errorObject = env->GetEvalErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = EvalError::ToString(ecmaRuntimeCallInfo);
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, factory->NewFromASCII("EvalError"), resultHandle), 0);
}

/*
 * @tc.name: EvalErrorToString
 * @tc.desc: new EvalError("This is EvalError!").toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, EvalErrorToString)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetEvalErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    JSHandle<JSTaggedValue> handleMsgKey(factory->NewFromASCII("message"));
    JSObject::SetProperty(
        thread, JSHandle<JSTaggedValue>(error), handleMsgKey,
        JSHandle<JSTaggedValue>(thread, factory->NewFromASCII("This is EvalError!").GetTaggedValue()));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = EvalError::ToString(ecmaRuntimeCallInfo);
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("EvalError: This is EvalError!"), resultHandle), 0);
}

/*
 * @tc.name: AggregateErrorParameterConstructor
 * @tc.desc: new AggregateError([], "Hello AggregateError", {cause: "error cause"})
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, AggregateErrorParameterConstructor)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetAggregateErrorFunction());
    JSHandle<JSTaggedValue> paramMsg(factory->NewFromASCII("Hello AggregateError!"));

    JSHandle<JSTaggedValue> errayFunc = env->GetArrayFunction();
    JSHandle<JSObject> newArray = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errayFunc), errayFunc);

    JSHandle<JSTaggedValue> causeKey = thread->GlobalConstants()->GetHandledCauseString();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> causeValue(factory->NewFromASCII("error cause")); // test error cause
    JSObject::SetProperty(thread, optionsObj, causeKey, causeValue);

    auto ecmaRuntimeCallInfo =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 10); // 10 means 3 call args
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));
    ecmaRuntimeCallInfo->SetCallArg(0, newArray.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, paramMsg.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(2, optionsObj.GetTaggedValue()); // 2 means the options arg

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = AggregateError::AggregateErrorConstructor(ecmaRuntimeCallInfo);
    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromASCII("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("Hello AggregateError!"), JSHandle<EcmaString>(msgValue)), 0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("AggregateError"), JSHandle<EcmaString>(nameValue)), 0);

    JSHandle<JSTaggedValue> errCauseValue(JSObject::GetProperty(thread, errorObject, causeKey).GetValue());
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        factory->NewFromASCII("error cause"), JSHandle<EcmaString>(errCauseValue)), 0);
}
}  // namespace panda::test
