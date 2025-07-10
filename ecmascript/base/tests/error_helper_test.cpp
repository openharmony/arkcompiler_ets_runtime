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

#include "ecmascript/base/error_helper.h"
#include "ecmascript/global_env.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class ErrorHelperTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(ErrorHelperTest, ErrorCommonToString_001)
{
    auto factory = instance->GetFactory();
    auto env = instance->GetGlobalEnv();
    JSHandle<JSTaggedValue> errorFunc = env->GetErrorFunction();
    JSHandle<JSTaggedValue> evalErrorFunc = env->GetEvalErrorFunction();
    JSHandle<JSTaggedValue> typeErrorFunc = env->GetTypeErrorFunction();
    JSHandle<JSTaggedValue> rangeErrorFunc = env->GetRangeErrorFunction();
    JSHandle<JSObject> errorObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorFunc), errorFunc);
    JSHandle<JSObject> evalErrorObj =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(evalErrorFunc), evalErrorFunc);
    JSHandle<JSObject> typeErrorObj =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(typeErrorFunc), typeErrorFunc);
    JSHandle<JSObject> rangeErrorObj =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(rangeErrorFunc), rangeErrorFunc);

    EcmaRuntimeCallInfo* argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    argv->SetFunction(JSTaggedValue::Undefined());
    argv->SetThis(JSTaggedValue(*errorObj));
    auto prev = TestHelper::SetupFrame(thread, argv);
    JSHandle<JSTaggedValue> error(thread, ErrorHelper::ErrorCommonToString(argv, ErrorType::ERROR));
    TestHelper::TearDownFrame(thread, prev);

    argv->SetThis(JSTaggedValue(*evalErrorObj));
    prev = TestHelper::SetupFrame(thread, argv);
    JSHandle<JSTaggedValue> evalError(thread, ErrorHelper::ErrorCommonToString(argv, ErrorType::EVAL_ERROR));
    TestHelper::TearDownFrame(thread, prev);

    argv->SetThis(JSTaggedValue(*typeErrorObj));
    prev = TestHelper::SetupFrame(thread, argv);
    JSHandle<JSTaggedValue> typeError(thread, ErrorHelper::ErrorCommonToString(argv, ErrorType::TYPE_ERROR));
    TestHelper::TearDownFrame(thread, prev);

    argv->SetThis(JSTaggedValue(*rangeErrorObj));
    prev = TestHelper::SetupFrame(thread, argv);
    JSHandle<JSTaggedValue> rangeError(thread, ErrorHelper::ErrorCommonToString(argv, ErrorType::RANGE_ERROR));
    TestHelper::TearDownFrame(thread, prev);

    EcmaStringAccessor errorStrAcc(JSHandle<EcmaString>::Cast(error));
    EcmaStringAccessor evalErrorStrAcc(JSHandle<EcmaString>::Cast(evalError));
    EcmaStringAccessor typeErrorStrAcc(JSHandle<EcmaString>::Cast(typeError));
    EcmaStringAccessor rangeErrorStrAcc(JSHandle<EcmaString>::Cast(rangeError));
    EXPECT_STREQ(errorStrAcc.ToCString(thread).c_str(), "Error");
    EXPECT_STREQ(evalErrorStrAcc.ToCString(thread).c_str(), "EvalError");
    EXPECT_STREQ(typeErrorStrAcc.ToCString(thread).c_str(), "TypeError");
    EXPECT_STREQ(rangeErrorStrAcc.ToCString(thread).c_str(), "RangeError");
}

HWTEST_F_L0(ErrorHelperTest, ErrorCommonToString_002)
{
    auto factory = instance->GetFactory();
    auto env = instance->GetGlobalEnv();
    JSHandle<JSTaggedValue> uriErrorFunc = env->GetURIErrorFunction();
    JSHandle<JSTaggedValue> oomErrorFunc = env->GetOOMErrorFunction();
    JSHandle<JSTaggedValue> syntaxErrorFunc = env->GetSyntaxErrorFunction();
    JSHandle<JSTaggedValue> referenceErrorFunc = env->GetReferenceErrorFunction();
    JSHandle<JSTaggedValue> aggregateErrorFunc = env->GetAggregateErrorFunction();
    JSHandle<JSTaggedValue> terminationErrorFunc = env->GetTerminationErrorFunction();
    JSHandle<JSObject> uriErrorObj =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(uriErrorFunc), uriErrorFunc);
    JSHandle<JSObject> oomErrorObj =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(oomErrorFunc), oomErrorFunc);
    JSHandle<JSObject> syntaxErrorObj =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(syntaxErrorFunc), syntaxErrorFunc);
    JSHandle<JSObject> referenceErrorObj =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(referenceErrorFunc), referenceErrorFunc);
    JSHandle<JSObject> aggregateErrorObj =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(aggregateErrorFunc), aggregateErrorFunc);
    JSHandle<JSObject> terminationErrorObj =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(terminationErrorFunc), terminationErrorFunc);

    EcmaRuntimeCallInfo* argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    argv->SetFunction(JSTaggedValue::Undefined());
    argv->SetThis(JSTaggedValue(*uriErrorObj));
    auto prev = TestHelper::SetupFrame(thread, argv);
    JSHandle<JSTaggedValue> uriError(thread, ErrorHelper::ErrorCommonToString(argv, ErrorType::URI_ERROR));
    TestHelper::TearDownFrame(thread, prev);

    argv->SetThis(JSTaggedValue(*oomErrorObj));
    prev = TestHelper::SetupFrame(thread, argv);
    JSHandle<JSTaggedValue> oomError(thread, ErrorHelper::ErrorCommonToString(argv, ErrorType::OOM_ERROR));
    TestHelper::TearDownFrame(thread, prev);

    argv->SetThis(JSTaggedValue(*terminationErrorObj));
    prev = TestHelper::SetupFrame(thread, argv);
    JSHandle<JSTaggedValue> terminationError(thread,
        ErrorHelper::ErrorCommonToString(argv, ErrorType::TERMINATION_ERROR));
    TestHelper::TearDownFrame(thread, prev);

    argv->SetThis(JSTaggedValue(*syntaxErrorObj));
    prev = TestHelper::SetupFrame(thread, argv);
    JSHandle<JSTaggedValue> syntaxError(thread, ErrorHelper::ErrorCommonToString(argv, ErrorType::SYNTAX_ERROR));
    TestHelper::TearDownFrame(thread, prev);

    argv->SetThis(JSTaggedValue(*referenceErrorObj));
    prev = TestHelper::SetupFrame(thread, argv);
    JSHandle<JSTaggedValue> referenceError(thread, ErrorHelper::ErrorCommonToString(argv, ErrorType::REFERENCE_ERROR));
    TestHelper::TearDownFrame(thread, prev);

    argv->SetThis(JSTaggedValue(*aggregateErrorObj));
    prev = TestHelper::SetupFrame(thread, argv);
    JSHandle<JSTaggedValue> aggregateError(thread, ErrorHelper::ErrorCommonToString(argv, ErrorType::AGGREGATE_ERROR));
    TestHelper::TearDownFrame(thread, prev);

    EcmaStringAccessor uriErrorStrAcc(JSHandle<EcmaString>::Cast(uriError));
    EcmaStringAccessor oomErrorStrAcc(JSHandle<EcmaString>::Cast(oomError));
    EcmaStringAccessor syntaxErrorStrAcc(JSHandle<EcmaString>::Cast(syntaxError));
    EcmaStringAccessor referenceErrorStrAcc(JSHandle<EcmaString>::Cast(referenceError));
    EcmaStringAccessor aggregateErrorStrAcc(JSHandle<EcmaString>::Cast(aggregateError));
    EXPECT_STREQ(uriErrorStrAcc.ToCString(thread).c_str(), "URIError");
    EXPECT_STREQ(oomErrorStrAcc.ToCString(thread).c_str(), "OutOfMemoryError");
    EXPECT_STREQ(syntaxErrorStrAcc.ToCString(thread).c_str(), "SyntaxError");
    EXPECT_STREQ(referenceErrorStrAcc.ToCString(thread).c_str(), "ReferenceError");
    EXPECT_STREQ(aggregateErrorStrAcc.ToCString(thread).c_str(), "AggregateError");
}

HWTEST_F_L0(ErrorHelperTest, ErrorCommonConstructor_001)
{
    auto factory = instance->GetFactory();
    auto env = instance->GetGlobalEnv();
    JSHandle<JSTaggedValue> msgKey = thread->GlobalConstants()->GetHandledMessageString();
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSFunction> error(env->GetErrorFunction());
    JSHandle<JSFunction> evalError(env->GetEvalErrorFunction());
    JSHandle<JSFunction> typeError(env->GetTypeErrorFunction());

    JSHandle<JSTaggedValue> errorMsg(factory->NewFromASCII("You have an Error!"));
    EcmaRuntimeCallInfo *argv1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 6);
    argv1->SetFunction(error.GetTaggedValue());
    argv1->SetThis(JSTaggedValue(*error));
    argv1->SetCallArg(0, errorMsg.GetTaggedValue());
    auto prev1 = TestHelper::SetupFrame(thread, argv1);
    JSHandle<JSTaggedValue> errorResult(thread, ErrorHelper::ErrorCommonConstructor(argv1, ErrorType::ERROR));
    TestHelper::TearDownFrame(thread, prev1);
    JSHandle<JSTaggedValue> errorMsgValue(JSObject::GetProperty(thread, errorResult, msgKey).GetValue());
    JSHandle<JSTaggedValue> errorNameValue(JSObject::GetProperty(thread, errorResult, nameKey).GetValue());
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(errorMsgValue)).ToCString(thread).c_str(),
                 "You have an Error!");
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(errorNameValue)).ToCString(thread).c_str(), "Error");

    JSHandle<JSTaggedValue> evalErrorMsg(factory->NewFromASCII("You have an eval error!"));
    EcmaRuntimeCallInfo *argv2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*evalError), 6);
    argv2->SetFunction(evalError.GetTaggedValue());
    argv2->SetThis(JSTaggedValue(*evalError));
    argv2->SetCallArg(0, evalErrorMsg.GetTaggedValue());
    auto prev2 = TestHelper::SetupFrame(thread, argv2);
    JSHandle<JSTaggedValue> evalErrorResult(thread, ErrorHelper::ErrorCommonConstructor(argv2, ErrorType::EVAL_ERROR));
    TestHelper::TearDownFrame(thread, prev2);
    JSHandle<JSTaggedValue> evalMsgValue(JSObject::GetProperty(thread, evalErrorResult, msgKey).GetValue());
    JSHandle<JSTaggedValue> evalNameValue(JSObject::GetProperty(thread, evalErrorResult, nameKey).GetValue());
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(evalMsgValue)).ToCString(thread).c_str(),
                 "You have an eval error!");
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(evalNameValue)).ToCString(thread).c_str(), "EvalError");

    JSHandle<JSTaggedValue> typeErrorMsg(factory->NewFromASCII("You have a type error!"));
    EcmaRuntimeCallInfo *argv3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typeError), 6);
    argv3->SetFunction(typeError.GetTaggedValue());
    argv3->SetThis(JSTaggedValue(*typeError));
    argv3->SetCallArg(0, typeErrorMsg.GetTaggedValue());
    auto prev3 = TestHelper::SetupFrame(thread, argv3);
    JSHandle<JSTaggedValue> typeErrorResult(thread, ErrorHelper::ErrorCommonConstructor(argv3, ErrorType::TYPE_ERROR));
    TestHelper::TearDownFrame(thread, prev3);
    JSHandle<JSTaggedValue> typeMsgValue(JSObject::GetProperty(thread, typeErrorResult, msgKey).GetValue());
    JSHandle<JSTaggedValue> typeNameValue(JSObject::GetProperty(thread, typeErrorResult, nameKey).GetValue());
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(typeMsgValue)).ToCString(thread).c_str(),
                 "You have a type error!");
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(typeNameValue)).ToCString(thread).c_str(), "TypeError");
}

HWTEST_F_L0(ErrorHelperTest, ErrorCommonConstructor_002)
{
    auto factory = instance->GetFactory();
    auto env = instance->GetGlobalEnv();
    JSHandle<JSTaggedValue> msgKey = thread->GlobalConstants()->GetHandledMessageString();
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSFunction> rangeError(env->GetRangeErrorFunction());
    JSHandle<JSFunction> uriError(env->GetURIErrorFunction());
    JSHandle<JSFunction> oomError(env->GetOOMErrorFunction());

    JSHandle<JSTaggedValue> rangeErrorMsg(factory->NewFromASCII("You have an range error!"));
    EcmaRuntimeCallInfo *argv1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*rangeError), 6);
    argv1->SetFunction(rangeError.GetTaggedValue());
    argv1->SetThis(JSTaggedValue(*rangeError));
    argv1->SetCallArg(0, rangeErrorMsg.GetTaggedValue());
    auto prev1 = TestHelper::SetupFrame(thread, argv1);
    JSHandle<JSTaggedValue> rangeErrorResult(thread,
                                             ErrorHelper::ErrorCommonConstructor(argv1, ErrorType::RANGE_ERROR));
    TestHelper::TearDownFrame(thread, prev1);
    JSHandle<JSTaggedValue> rangeMsgValue(JSObject::GetProperty(thread, rangeErrorResult, msgKey).GetValue());
    JSHandle<JSTaggedValue> rangeNameValue(JSObject::GetProperty(thread, rangeErrorResult, nameKey).GetValue());
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(rangeMsgValue)).ToCString(thread).c_str(),
                 "You have an range error!");
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(rangeNameValue)).ToCString(thread).c_str(),
                 "RangeError");

    JSHandle<JSTaggedValue> uriErrorMsg(factory->NewFromASCII("You have an uri error!"));
    EcmaRuntimeCallInfo *argv2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*uriError), 6);
    argv2->SetFunction(uriError.GetTaggedValue());
    argv2->SetThis(JSTaggedValue(*uriError));
    argv2->SetCallArg(0, uriErrorMsg.GetTaggedValue());
    auto prev2 = TestHelper::SetupFrame(thread, argv2);
    JSHandle<JSTaggedValue> uriErrorResult(thread, ErrorHelper::ErrorCommonConstructor(argv2, ErrorType::URI_ERROR));
    TestHelper::TearDownFrame(thread, prev2);
    JSHandle<JSTaggedValue> uriMsgValue(JSObject::GetProperty(thread, uriErrorResult, msgKey).GetValue());
    JSHandle<JSTaggedValue> uriNameValue(JSObject::GetProperty(thread, uriErrorResult, nameKey).GetValue());
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(uriMsgValue)).ToCString(thread).c_str(),
                 "You have an uri error!");
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(uriNameValue)).ToCString(thread).c_str(), "URIError");

    JSHandle<JSTaggedValue> oomErrorMsg(factory->NewFromASCII("You have an out of memory error!"));
    EcmaRuntimeCallInfo *argv3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*oomError), 6);
    argv3->SetFunction(oomError.GetTaggedValue());
    argv3->SetThis(JSTaggedValue(*oomError));
    argv3->SetCallArg(0, oomErrorMsg.GetTaggedValue());
    auto prev3 = TestHelper::SetupFrame(thread, argv3);
    JSHandle<JSTaggedValue> oomErrorResult(thread, ErrorHelper::ErrorCommonConstructor(argv3, ErrorType::OOM_ERROR));
    TestHelper::TearDownFrame(thread, prev3);
    JSHandle<JSTaggedValue> oomMsgValue(JSObject::GetProperty(thread, oomErrorResult, msgKey).GetValue());
    JSHandle<JSTaggedValue> oomNameValue(JSObject::GetProperty(thread, oomErrorResult, nameKey).GetValue());
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(oomMsgValue)).ToCString(thread).c_str(),
                 "You have an out of memory error!");
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(oomNameValue)).ToCString(thread).c_str(),
                 "OutOfMemoryError");
}

HWTEST_F_L0(ErrorHelperTest, ErrorCommonConstructor_003)
{
    auto factory = instance->GetFactory();
    auto env = instance->GetGlobalEnv();
    JSHandle<JSTaggedValue> msgKey = thread->GlobalConstants()->GetHandledMessageString();
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSFunction> syntaxError(env->GetSyntaxErrorFunction());
    JSHandle<JSFunction> referenceError(env->GetReferenceErrorFunction());
    JSHandle<JSFunction> aggregateError(env->GetAggregateErrorFunction());

    JSHandle<JSTaggedValue> syntaxErrorMsg(factory->NewFromASCII("You have an syntax error!"));
    EcmaRuntimeCallInfo *argv1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*syntaxError), 6);
    argv1->SetFunction(syntaxError.GetTaggedValue());
    argv1->SetThis(JSTaggedValue(*syntaxError));
    argv1->SetCallArg(0, syntaxErrorMsg.GetTaggedValue());
    auto prev1 = TestHelper::SetupFrame(thread, argv1);
    JSHandle<JSTaggedValue> syntaxErrorResult(thread,
                                              ErrorHelper::ErrorCommonConstructor(argv1, ErrorType::SYNTAX_ERROR));
    TestHelper::TearDownFrame(thread, prev1);
    JSHandle<JSTaggedValue> syntaxMsgValue(JSObject::GetProperty(thread, syntaxErrorResult, msgKey).GetValue());
    JSHandle<JSTaggedValue> syntaxNameValue(JSObject::GetProperty(thread, syntaxErrorResult, nameKey).GetValue());
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(syntaxMsgValue)).ToCString(thread).c_str(),
                 "You have an syntax error!");
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(syntaxNameValue)).ToCString(thread).c_str(),
                                    "SyntaxError");

    JSHandle<JSTaggedValue> referenceErrorMsg(factory->NewFromASCII("You have an reference error!"));
    EcmaRuntimeCallInfo *argv2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*referenceError), 6);
    argv2->SetFunction(referenceError.GetTaggedValue());
    argv2->SetThis(JSTaggedValue(*referenceError));
    argv2->SetCallArg(0, referenceErrorMsg.GetTaggedValue());
    auto prev2 = TestHelper::SetupFrame(thread, argv2);
    JSHandle<JSTaggedValue> referenceErrorResult(thread,
        ErrorHelper::ErrorCommonConstructor(argv2, ErrorType::REFERENCE_ERROR));
    TestHelper::TearDownFrame(thread, prev2);
    JSHandle<JSTaggedValue> referenceMsgValue(JSObject::GetProperty(thread, referenceErrorResult, msgKey).GetValue());
    JSHandle<JSTaggedValue> referenceNameValue(
        JSObject::GetProperty(thread, referenceErrorResult, nameKey).GetValue());
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(referenceMsgValue)).ToCString(thread).c_str(),
                 "You have an reference error!");
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(referenceNameValue)).ToCString(thread).c_str(),
                 "ReferenceError");

    JSHandle<JSTaggedValue> aggregateErrorMsg(factory->NewFromASCII("You have an aggregate error!"));
    EcmaRuntimeCallInfo *argv3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*aggregateError), 6);
    argv3->SetFunction(aggregateError.GetTaggedValue());
    argv3->SetThis(JSTaggedValue(*aggregateError));
    argv3->SetCallArg(0, aggregateErrorMsg.GetTaggedValue());
    auto prev3 = TestHelper::SetupFrame(thread, argv3);
    JSHandle<JSTaggedValue> aggregateErrorResult(thread,
        ErrorHelper::ErrorCommonConstructor(argv3, ErrorType::AGGREGATE_ERROR));
    TestHelper::TearDownFrame(thread, prev3);
    JSHandle<JSTaggedValue> aggregateMsgValue(JSObject::GetProperty(thread, aggregateErrorResult, msgKey).GetValue());
    JSHandle<JSTaggedValue> aggregateNameValue(
        JSObject::GetProperty(thread, aggregateErrorResult, nameKey).GetValue());
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(aggregateMsgValue)).ToCString(thread).c_str(),
                 "You have an aggregate error!");
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(aggregateNameValue)).ToCString(thread).c_str(),
                 "AggregateError");
}

HWTEST_F_L0(ErrorHelperTest, ErrorCommonConstructor_004)
{
    auto factory = instance->GetFactory();
    auto env = instance->GetGlobalEnv();
    JSHandle<JSTaggedValue> msgKey = thread->GlobalConstants()->GetHandledMessageString();
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();
    JSHandle<JSTaggedValue> causeKey = thread->GlobalConstants()->GetHandledCauseString();

    JSHandle<JSFunction> error(env->GetErrorFunction());
    JSHandle<JSFunction> typeError(env->GetTypeErrorFunction());
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> causeValue(factory->NewFromASCII("error cause")); // test error cause
    JSObject::SetProperty(thread, optionsObj, causeKey, causeValue);

    JSHandle<JSTaggedValue> errorMsg(factory->NewFromASCII("You have an Error!"));
    EcmaRuntimeCallInfo *argv1 =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 8); // 8 means 2 call args
    argv1->SetFunction(error.GetTaggedValue());
    argv1->SetThis(JSTaggedValue(*error));
    argv1->SetCallArg(0, errorMsg.GetTaggedValue());
    argv1->SetCallArg(1, optionsObj.GetTaggedValue());
    auto prev1 = TestHelper::SetupFrame(thread, argv1);
    JSHandle<JSTaggedValue> errorResult(thread, ErrorHelper::ErrorCommonConstructor(argv1, ErrorType::ERROR));
    TestHelper::TearDownFrame(thread, prev1);
    JSHandle<JSTaggedValue> errorMsgValue(JSObject::GetProperty(thread, errorResult, msgKey).GetValue());
    JSHandle<JSTaggedValue> errorNameValue(JSObject::GetProperty(thread, errorResult, nameKey).GetValue());
    JSHandle<JSTaggedValue> errorCauseValue(JSObject::GetProperty(thread, errorResult, causeKey).GetValue());
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(errorMsgValue)).ToCString(thread).c_str(),
                 "You have an Error!");
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(errorNameValue)).ToCString(thread).c_str(), "Error");
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(errorCauseValue)).ToCString(thread).c_str(),
                 "error cause");

    JSHandle<JSTaggedValue> typeErrorMsg(factory->NewFromASCII("You have a type error!"));
    EcmaRuntimeCallInfo *argv2 =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typeError), 8); // 8 means 2 call args
    argv2->SetFunction(typeError.GetTaggedValue());
    argv2->SetThis(JSTaggedValue(*typeError));
    argv2->SetCallArg(0, typeErrorMsg.GetTaggedValue());
    argv2->SetCallArg(1, optionsObj.GetTaggedValue());
    auto prev2 = TestHelper::SetupFrame(thread, argv2);
    JSHandle<JSTaggedValue> typeErrorResult(thread, ErrorHelper::ErrorCommonConstructor(argv2, ErrorType::TYPE_ERROR));
    TestHelper::TearDownFrame(thread, prev2);
    JSHandle<JSTaggedValue> typeMsgValue(JSObject::GetProperty(thread, typeErrorResult, msgKey).GetValue());
    JSHandle<JSTaggedValue> typeNameValue(JSObject::GetProperty(thread, typeErrorResult, nameKey).GetValue());
    JSHandle<JSTaggedValue> typeCauseValue(JSObject::GetProperty(thread, typeErrorResult, causeKey).GetValue());
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(typeMsgValue)).ToCString(thread).c_str(),
                 "You have a type error!");
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(typeNameValue)).ToCString(thread).c_str(), "TypeError");
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(typeCauseValue)).ToCString(thread).c_str(),
                 "error cause");
}

HWTEST_F_L0(ErrorHelperTest, ErrorCommonConstructor_005)
{
    auto factory = instance->GetFactory();
    auto env = instance->GetGlobalEnv();
    JSHandle<JSTaggedValue> stackKey = thread->GlobalConstants()->GetHandledStackString();

    JSHandle<JSFunction> error(env->GetErrorFunction());

    JSHandle<JSTaggedValue> errorMsg(factory->NewFromASCII("You have an Error!"));
    EcmaRuntimeCallInfo *argv1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 6);
    argv1->SetFunction(error.GetTaggedValue());
    argv1->SetThis(JSTaggedValue(*error));
    argv1->SetCallArg(0, errorMsg.GetTaggedValue());
    auto prev1 = TestHelper::SetupFrame(thread, argv1);
    JSHandle<JSTaggedValue> errorResult(thread, ErrorHelper::ErrorCommonConstructor(argv1, ErrorType::ERROR));
    TestHelper::TearDownFrame(thread, prev1);
    JSHandle<JSTaggedValue> errorStackValue(JSObject::GetProperty(thread, errorResult, stackKey).GetValue());

    EXPECT_TRUE(EcmaStringAccessor(JSHandle<EcmaString>::Cast(errorStackValue)).ToCString(thread).empty());
}
}  // namespace panda::test
