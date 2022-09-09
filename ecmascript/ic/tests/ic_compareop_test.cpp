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

#include <thread>

#include "ecmascript/builtins/builtins_boolean.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/ic/ic_compare_op.cpp"
#include "ecmascript/ic/ic_compare_op.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
namespace panda::test {
class IcCompareOPTest : public testing::Test {
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
        TestHelper::CreateEcmaVMWithScope(ecmaVm, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(ecmaVm, scope);
    }

    EcmaVM *ecmaVm {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(IcCompareOPTest, EqualWithIC)
{
    ObjectFactory *factory = ecmaVm->GetFactory();

    JSHandle<JSTaggedValue> Str1 = JSHandle<JSTaggedValue>(factory->NewFromASCII("1"));
    JSTaggedValue arg1(static_cast<uint32_t>(1));
    JSTaggedValue arg2(static_cast<double>(1.0));
    JSTaggedValue arg3(false);
    JSTaggedValue arg4(true);
    JSHandle<JSTaggedValue> arg1Handle(thread, arg1);
    JSHandle<JSTaggedValue> arg2Handle(thread, arg2);
    JSHandle<JSTaggedValue> arg3Handle(thread, arg3);
    JSHandle<JSTaggedValue> arg4Handle(thread, arg4);

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    JSHandle<JSFunction> boolean(env->GetBooleanFunction());
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*boolean), 6);
    ecmaRuntimeCallInfo->SetFunction(boolean.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));

    JSTaggedValue booleanObj = builtins::BuiltinsBoolean::BooleanConstructor(ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> booleanObjHandle(thread, booleanObj);

    JSTaggedValue resInSlowPath1 = SlowRuntimeStub::Eq(thread, arg1Handle.GetTaggedValue(),
                                                          arg2Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath2 = SlowRuntimeStub::Eq(thread, Str1.GetTaggedValue(), arg1Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath3 = SlowRuntimeStub::Eq(thread, Str1.GetTaggedValue(), arg3Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath4 = SlowRuntimeStub::Eq(thread, Str1.GetTaggedValue(), arg4Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath5 = SlowRuntimeStub::Eq(thread, booleanObjHandle.GetTaggedValue(),
                                                          arg4Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath9 = SlowRuntimeStub::Eq(thread, JSTaggedValue::Undefined(), JSTaggedValue::Null());
    JSTaggedValue resInSlowPath10 = SlowRuntimeStub::Eq(thread, JSTaggedValue::Undefined(), JSTaggedValue::True());

    JSTaggedValue resInICPath1 = CompareOp::EqualWithIC(thread, arg1Handle.GetTaggedValue(),
                                                        arg2Handle.GetTaggedValue(), CompareOpType::NUMBER_NUMBER);
    JSTaggedValue resInICPath2 = CompareOp::EqualWithIC(thread, Str1.GetTaggedValue(),
                                                        arg1Handle.GetTaggedValue(), CompareOpType::STRING_NUMBER);
    JSTaggedValue resInICPath3 = CompareOp::EqualWithIC(thread, Str1.GetTaggedValue(),
                                                        arg3Handle.GetTaggedValue(), CompareOpType::STRING_BOOLEAN);
    JSTaggedValue resInICPath4 = CompareOp::EqualWithIC(thread, Str1.GetTaggedValue(),
                                                        arg4Handle.GetTaggedValue(), CompareOpType::STRING_BOOLEAN);
    JSTaggedValue resInICPath5 = CompareOp::EqualWithIC(thread, booleanObjHandle.GetTaggedValue(),
                                                        arg4Handle.GetTaggedValue(), CompareOpType::OBJ_BOOLEAN);
    JSTaggedValue resInICPath9 = CompareOp::EqualWithIC(thread, JSTaggedValue::Undefined(),
                                                        JSTaggedValue::Null(), CompareOpType::UNDEFINED_NULL);
    JSTaggedValue resInICPath10 = CompareOp::EqualWithIC(thread, JSTaggedValue::Undefined(),
                                                        JSTaggedValue::True(), CompareOpType::OTHER);

    EXPECT_EQ(resInSlowPath1, resInICPath1);
    EXPECT_EQ(resInSlowPath2, resInICPath2);
    EXPECT_EQ(resInSlowPath3, resInICPath3);
    EXPECT_EQ(resInSlowPath4, resInICPath4);
    EXPECT_EQ(resInSlowPath5, resInICPath5);
    EXPECT_EQ(resInSlowPath9, resInICPath9);
    EXPECT_EQ(resInSlowPath10, resInICPath10);
};

HWTEST_F_L0(IcCompareOPTest, NotEqualWithIC)
{
    ObjectFactory *factory = ecmaVm->GetFactory();

    JSHandle<JSTaggedValue> Str1 = JSHandle<JSTaggedValue>(factory->NewFromASCII("1"));
    JSTaggedValue arg1(static_cast<uint32_t>(1));
    JSTaggedValue arg2(static_cast<double>(2.0));
    JSTaggedValue arg3(false);
    JSTaggedValue arg4(true);
    JSHandle<JSTaggedValue> arg1Handle(thread, arg1);
    JSHandle<JSTaggedValue> arg2Handle(thread, arg2);
    JSHandle<JSTaggedValue> arg3Handle(thread, arg3);
    JSHandle<JSTaggedValue> arg4Handle(thread, arg4);

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    JSHandle<JSFunction> boolean(env->GetBooleanFunction());
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*boolean), 6);
    ecmaRuntimeCallInfo->SetFunction(boolean.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(123)));

    JSTaggedValue booleanObj = builtins::BuiltinsBoolean::BooleanConstructor(ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> booleanObjHandle(thread, booleanObj);
    JSTaggedValue resInSlowPath1 = SlowRuntimeStub::NotEq(thread, arg1Handle.GetTaggedValue(),
                                                             arg2Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath2 = SlowRuntimeStub::NotEq(thread, Str1.GetTaggedValue(),
                                                             arg1Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath3 = SlowRuntimeStub::NotEq(thread, Str1.GetTaggedValue(),
                                                             arg3Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath4 = SlowRuntimeStub::NotEq(thread, Str1.GetTaggedValue(),
                                                             arg4Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath5 = SlowRuntimeStub::NotEq(thread, arg1Handle.GetTaggedValue(),
                                                             booleanObjHandle.GetTaggedValue());
    JSTaggedValue resInSlowPath9 = SlowRuntimeStub::NotEq(thread, JSTaggedValue::Undefined(),
                                                             JSTaggedValue::Null());
    JSTaggedValue resInSlowPath10 = SlowRuntimeStub::NotEq(thread, JSTaggedValue::Undefined(),
                                                              JSTaggedValue::True());

    JSTaggedValue resInICPath1 = CompareOp::NotEqualWithIC(thread, arg1Handle.GetTaggedValue(),
                                                           arg2Handle.GetTaggedValue(),
                                                           CompareOpType::NUMBER_NUMBER);
    JSTaggedValue resInICPath2 = CompareOp::NotEqualWithIC(thread, Str1.GetTaggedValue(),
                                                           arg1Handle.GetTaggedValue(), CompareOpType::STRING_NUMBER);
    JSTaggedValue resInICPath3 = CompareOp::NotEqualWithIC(thread, Str1.GetTaggedValue(),
                                                           arg3Handle.GetTaggedValue(), CompareOpType::STRING_BOOLEAN);
    JSTaggedValue resInICPath4 = CompareOp::NotEqualWithIC(thread, Str1.GetTaggedValue(),
                                                           arg4Handle.GetTaggedValue(), CompareOpType::STRING_BOOLEAN);
    JSTaggedValue resInICPath5 = CompareOp::NotEqualWithIC(thread, arg1Handle.GetTaggedValue(),
                                                           booleanObjHandle.GetTaggedValue(),
                                                           CompareOpType::NUMBER_OBJ);
    JSTaggedValue resInICPath9 = CompareOp::NotEqualWithIC(thread, JSTaggedValue::Undefined(),
                                                           JSTaggedValue::Null(), CompareOpType::UNDEFINED_NULL);
    JSTaggedValue resInICPath10 = CompareOp::NotEqualWithIC(thread, JSTaggedValue::Undefined(),
                                                            JSTaggedValue::True(), CompareOpType::OTHER);

    EXPECT_EQ(resInSlowPath1, resInICPath1);
    EXPECT_EQ(resInSlowPath2, resInICPath2);
    EXPECT_EQ(resInSlowPath3, resInICPath3);
    EXPECT_EQ(resInSlowPath4, resInICPath4);
    EXPECT_EQ(resInSlowPath5, resInICPath5);
    EXPECT_EQ(resInSlowPath9, resInICPath9);
    EXPECT_EQ(resInSlowPath10, resInICPath10);
};


HWTEST_F_L0(IcCompareOPTest, LessWithIC)
{
    ObjectFactory *factory = ecmaVm->GetFactory();

    JSHandle<JSTaggedValue> Str1 = JSHandle<JSTaggedValue>(factory->NewFromASCII("0"));
    JSTaggedValue arg1(static_cast<uint32_t>(1));
    JSTaggedValue arg2(static_cast<double>(0.5));
    JSTaggedValue arg3(false);
    JSTaggedValue arg4(true);
    JSHandle<JSTaggedValue> arg1Handle(thread, arg1);
    JSHandle<JSTaggedValue> arg2Handle(thread, arg2);
    JSHandle<JSTaggedValue> arg3Handle(thread, arg3);
    JSHandle<JSTaggedValue> arg4Handle(thread, arg4);

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    JSHandle<JSFunction> boolean(env->GetBooleanFunction());
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*boolean), 6);
    ecmaRuntimeCallInfo->SetFunction(boolean.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(123)));

    JSTaggedValue booleanObj = builtins::BuiltinsBoolean::BooleanConstructor(ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> booleanObjHandle(thread, booleanObj);

    JSTaggedValue resInSlowPath1 = SlowRuntimeStub::Less(thread, arg1Handle.GetTaggedValue(),
                                                            arg2Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath2 = SlowRuntimeStub::Less(thread, Str1.GetTaggedValue(),
                                                            arg1Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath3 = SlowRuntimeStub::Less(thread, Str1.GetTaggedValue(),
                                                            arg3Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath4 = SlowRuntimeStub::Less(thread, Str1.GetTaggedValue(),
                                                            arg4Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath5 = SlowRuntimeStub::Less(thread, arg1Handle.GetTaggedValue(),
                                                            booleanObjHandle.GetTaggedValue());
    JSTaggedValue resInSlowPath9 = SlowRuntimeStub::Less(thread,
        JSTaggedValue::Undefined(), JSTaggedValue::Null());
    JSTaggedValue resInSlowPath10 = SlowRuntimeStub::Less(thread,
        JSTaggedValue::Undefined(), JSTaggedValue::True());

    JSTaggedValue resInICPath1 = CompareOp::LessWithIC(thread,  arg1Handle.GetTaggedValue(),
                                                          arg2Handle.GetTaggedValue(), CompareOpType::NUMBER_NUMBER);
    JSTaggedValue resInICPath2 = CompareOp::LessWithIC(thread, Str1.GetTaggedValue(),
                                                           arg1Handle.GetTaggedValue(), CompareOpType::STRING_NUMBER);
    JSTaggedValue resInICPath3 = CompareOp::LessWithIC(thread, Str1.GetTaggedValue(),
                                                          arg3Handle.GetTaggedValue(), CompareOpType::STRING_BOOLEAN);
    JSTaggedValue resInICPath4 = CompareOp::LessWithIC(thread, Str1.GetTaggedValue(),
                                                          arg4Handle.GetTaggedValue(), CompareOpType::STRING_BOOLEAN);
    JSTaggedValue resInICPath5 = CompareOp::LessWithIC(thread,  arg1Handle.GetTaggedValue(),
                                                          booleanObjHandle.GetTaggedValue(),
                                                          CompareOpType::NUMBER_OBJ);
    JSTaggedValue resInICPath9 = CompareOp::LessWithIC(thread, JSTaggedValue::Undefined(),
                                                          JSTaggedValue::Null(), CompareOpType::UNDEFINED_NULL);
    JSTaggedValue resInICPath10 = CompareOp::LessWithIC(thread, JSTaggedValue::Undefined(),
                                                           JSTaggedValue::True(), CompareOpType::OTHER);

    EXPECT_EQ(resInSlowPath1, resInICPath1);
    EXPECT_EQ(resInSlowPath2, resInICPath2);
    EXPECT_EQ(resInSlowPath3, resInICPath3);
    EXPECT_EQ(resInSlowPath4, resInICPath4);
    EXPECT_EQ(resInSlowPath5, resInICPath5);
    EXPECT_EQ(resInSlowPath9, resInICPath9);
    EXPECT_EQ(resInSlowPath10, resInICPath10);
};


HWTEST_F_L0(IcCompareOPTest, LessEqWithIC)
{
    ObjectFactory *factory = ecmaVm->GetFactory();

    JSHandle<JSTaggedValue> Str1 = JSHandle<JSTaggedValue>(factory->NewFromASCII("1"));
    JSTaggedValue arg1(static_cast<uint32_t>(1));
    JSTaggedValue arg2(static_cast<double>(0.5));
    JSTaggedValue arg3(false);
    JSTaggedValue arg4(true);
    JSHandle<JSTaggedValue> arg1Handle(thread, arg1);
    JSHandle<JSTaggedValue> arg2Handle(thread, arg2);
    JSHandle<JSTaggedValue> arg3Handle(thread, arg3);
    JSHandle<JSTaggedValue> arg4Handle(thread, arg4);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    JSHandle<JSFunction> boolean(env->GetBooleanFunction());
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*boolean), 6);
    ecmaRuntimeCallInfo->SetFunction(boolean.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(123)));

    JSTaggedValue booleanObj = builtins::BuiltinsBoolean::BooleanConstructor(ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> booleanObjHandle(thread, booleanObj);
    JSTaggedValue resInSlowPath1 = SlowRuntimeStub::LessEq(thread, arg1Handle.GetTaggedValue(),
                                                              arg2Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath2 = SlowRuntimeStub::LessEq(thread, Str1.GetTaggedValue(),
                                                              arg1Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath3 = SlowRuntimeStub::LessEq(thread, Str1.GetTaggedValue(),
                                                              arg3Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath4 = SlowRuntimeStub::LessEq(thread, Str1.GetTaggedValue(),
                                                              arg4Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath5 = SlowRuntimeStub::LessEq(thread, arg1Handle.GetTaggedValue(),
                                                              booleanObjHandle.GetTaggedValue());
    JSTaggedValue resInSlowPath9 = SlowRuntimeStub::LessEq(thread, JSTaggedValue::Undefined(),
                                                              JSTaggedValue::Null());
    JSTaggedValue resInSlowPath10 = SlowRuntimeStub::LessEq(thread, JSTaggedValue::Undefined(),
                                                               JSTaggedValue::True());
    JSTaggedValue resInICPath1 = CompareOp::LessEqWithIC(thread, arg1Handle.GetTaggedValue(),
                                                            arg2Handle.GetTaggedValue(),
                                                            CompareOpType::NUMBER_NUMBER);
    JSTaggedValue resInICPath2 = CompareOp::LessEqWithIC(thread, Str1.GetTaggedValue(),
                                                            arg1Handle.GetTaggedValue(),
                                                            CompareOpType::STRING_NUMBER);
    JSTaggedValue resInICPath3 = CompareOp::LessEqWithIC(thread, Str1.GetTaggedValue(),
                                                            arg3Handle.GetTaggedValue(),
                                                            CompareOpType::STRING_BOOLEAN);
    JSTaggedValue resInICPath4 = CompareOp::LessEqWithIC(thread, Str1.GetTaggedValue(),
                                                            arg4Handle.GetTaggedValue(),
                                                            CompareOpType::STRING_BOOLEAN);
    JSTaggedValue resInICPath5 = CompareOp::LessEqWithIC(thread,
                                                            arg1Handle.GetTaggedValue(),
                                                            booleanObjHandle.GetTaggedValue(),
                                                            CompareOpType::NUMBER_OBJ);
    JSTaggedValue resInICPath9 = CompareOp::LessEqWithIC(thread, JSTaggedValue::Undefined(),
                                                            JSTaggedValue::Null(), CompareOpType::UNDEFINED_NULL);
    JSTaggedValue resInICPath10 = CompareOp::LessEqWithIC(thread, JSTaggedValue::Undefined(),
                                                            JSTaggedValue::True(), CompareOpType::OTHER);

    EXPECT_EQ(resInSlowPath1, resInICPath1);
    EXPECT_EQ(resInSlowPath2, resInICPath2);
    EXPECT_EQ(resInSlowPath3, resInICPath3);
    EXPECT_EQ(resInSlowPath4, resInICPath4);
    EXPECT_EQ(resInSlowPath5, resInICPath5);
    EXPECT_EQ(resInSlowPath9, resInICPath9);
    EXPECT_EQ(resInSlowPath10, resInICPath10);
};


HWTEST_F_L0(IcCompareOPTest, GreaterWithIC)
{
    ObjectFactory *factory = ecmaVm->GetFactory();

    JSHandle<JSTaggedValue> Str1 = JSHandle<JSTaggedValue>(factory->NewFromASCII("1"));
    JSTaggedValue arg1(static_cast<uint32_t>(1));
    JSTaggedValue arg2(static_cast<double>(1.0));
    JSTaggedValue arg3(false);
    JSTaggedValue arg4(true);
    JSHandle<JSTaggedValue> arg1Handle(thread, arg1);
    JSHandle<JSTaggedValue> arg2Handle(thread, arg2);
    JSHandle<JSTaggedValue> arg3Handle(thread, arg3);
    JSHandle<JSTaggedValue> arg4Handle(thread, arg4);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    JSHandle<JSFunction> boolean(env->GetBooleanFunction());
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*boolean), 6);
    ecmaRuntimeCallInfo->SetFunction(boolean.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));

    JSTaggedValue booleanObj = builtins::BuiltinsBoolean::BooleanConstructor(ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> booleanObjHandle(thread, booleanObj);
    JSTaggedValue resInSlowPath1 = SlowRuntimeStub::Greater(thread, arg1Handle.GetTaggedValue(),
                                                               arg2Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath2 = SlowRuntimeStub::Greater(thread, Str1.GetTaggedValue(),
                                                               arg1Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath3 = SlowRuntimeStub::Greater(thread, Str1.GetTaggedValue(),
                                                               arg3Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath4 = SlowRuntimeStub::Greater(thread, Str1.GetTaggedValue(),
                                                               arg4Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath5 = SlowRuntimeStub::Greater(thread, arg1Handle.GetTaggedValue(),
                                                               booleanObjHandle.GetTaggedValue());
    JSTaggedValue resInSlowPath9 = SlowRuntimeStub::Greater(thread, JSTaggedValue::Undefined(),
                                                               JSTaggedValue::Null());
    JSTaggedValue resInSlowPath10 = SlowRuntimeStub::Greater(thread, JSTaggedValue::Undefined(),
                                                                JSTaggedValue::True());

    JSTaggedValue resInICPath1 = CompareOp::GreaterWithIC(thread, arg1Handle.GetTaggedValue(),
                                                             arg2Handle.GetTaggedValue(),
                                                             CompareOpType::NUMBER_NUMBER);
    JSTaggedValue resInICPath2 = CompareOp::GreaterWithIC(thread, Str1.GetTaggedValue(),
                                                             arg1Handle.GetTaggedValue(),
                                                             CompareOpType::STRING_NUMBER);
    JSTaggedValue resInICPath3 = CompareOp::GreaterWithIC(thread, Str1.GetTaggedValue(),
                                                             arg3Handle.GetTaggedValue(),
                                                             CompareOpType::STRING_BOOLEAN);
    JSTaggedValue resInICPath4 = CompareOp::GreaterWithIC(thread, Str1.GetTaggedValue(),
                                                             arg4Handle.GetTaggedValue(),
                                                             CompareOpType::STRING_BOOLEAN);
    JSTaggedValue resInICPath5 = CompareOp::GreaterWithIC(thread, arg1Handle.GetTaggedValue(),
                                                             booleanObjHandle.GetTaggedValue(),
                                                             CompareOpType::NUMBER_OBJ);
    JSTaggedValue resInICPath9 = CompareOp::GreaterWithIC(thread, JSTaggedValue::Undefined(),
                                                             JSTaggedValue::Null(), CompareOpType::UNDEFINED_NULL);
    JSTaggedValue resInICPath10 = CompareOp::GreaterWithIC(thread, JSTaggedValue::Undefined(),
                                                              JSTaggedValue::True(), CompareOpType::OTHER);

    EXPECT_EQ(resInSlowPath1, resInICPath1);
    EXPECT_EQ(resInSlowPath2, resInICPath2);
    EXPECT_EQ(resInSlowPath3, resInICPath3);
    EXPECT_EQ(resInSlowPath4, resInICPath4);
    EXPECT_EQ(resInSlowPath5, resInICPath5);
    EXPECT_EQ(resInSlowPath9, resInICPath9);
    EXPECT_EQ(resInSlowPath10, resInICPath10);
};


HWTEST_F_L0(IcCompareOPTest, GreaterEqWithIC)
{
    ObjectFactory *factory = ecmaVm->GetFactory();

    JSHandle<JSTaggedValue> Str1 = JSHandle<JSTaggedValue>(factory->NewFromASCII("1"));
    JSTaggedValue arg1(static_cast<uint32_t>(1));
    JSTaggedValue arg2(static_cast<double>(1.0));
    JSTaggedValue arg3(false);
    JSTaggedValue arg4(true);
    JSHandle<JSTaggedValue> arg1Handle(thread, arg1);
    JSHandle<JSTaggedValue> arg2Handle(thread, arg2);
    JSHandle<JSTaggedValue> arg3Handle(thread, arg3);
    JSHandle<JSTaggedValue> arg4Handle(thread, arg4);

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    JSHandle<JSFunction> boolean(env->GetBooleanFunction());
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*boolean), 6);
    ecmaRuntimeCallInfo->SetFunction(boolean.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(0)));

    JSTaggedValue booleanObj = builtins::BuiltinsBoolean::BooleanConstructor(ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> booleanObjHandle(thread, booleanObj);
    JSTaggedValue resInSlowPath1 = SlowRuntimeStub::GreaterEq(thread, arg1Handle.GetTaggedValue(),
                                                                 arg2Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath2 = SlowRuntimeStub::GreaterEq(thread, Str1.GetTaggedValue(),
                                                                 arg1Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath3 = SlowRuntimeStub::GreaterEq(thread, Str1.GetTaggedValue(),
                                                                 arg3Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath4 = SlowRuntimeStub::GreaterEq(thread, Str1.GetTaggedValue(),
                                                                 arg4Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath5 = SlowRuntimeStub::GreaterEq(thread, arg1Handle.GetTaggedValue(),
                                                                 booleanObjHandle.GetTaggedValue());
    JSTaggedValue resInSlowPath9 = SlowRuntimeStub::GreaterEq(thread, JSTaggedValue::Undefined(),
                                                                 JSTaggedValue::Null());
    JSTaggedValue resInSlowPath10 = SlowRuntimeStub::GreaterEq(thread, JSTaggedValue::Undefined(),
                                                                  JSTaggedValue::True());

    JSTaggedValue resInICPath1 = CompareOp::GreaterEqWithIC(thread, arg1Handle.GetTaggedValue(),
                                                               arg2Handle.GetTaggedValue(),
                                                               CompareOpType::NUMBER_NUMBER);
    JSTaggedValue resInICPath2 = CompareOp::GreaterEqWithIC(thread, Str1.GetTaggedValue(),
                                                               arg1Handle.GetTaggedValue(),
                                                               CompareOpType::STRING_NUMBER);
    JSTaggedValue resInICPath3 = CompareOp::GreaterEqWithIC(thread, Str1.GetTaggedValue(),
                                                               arg3Handle.GetTaggedValue(),
                                                               CompareOpType::STRING_BOOLEAN);
    JSTaggedValue resInICPath4 = CompareOp::GreaterEqWithIC(thread, Str1.GetTaggedValue(),
                                                               arg4Handle.GetTaggedValue(),
                                                               CompareOpType::STRING_BOOLEAN);
    JSTaggedValue resInICPath5 = CompareOp::GreaterEqWithIC(thread, arg1Handle.GetTaggedValue(),
                                                               booleanObjHandle.GetTaggedValue(),
                                                               CompareOpType::NUMBER_OBJ);
    JSTaggedValue resInICPath9 = CompareOp::GreaterEqWithIC(thread, JSTaggedValue::Undefined(),
                                                               JSTaggedValue::Null(), CompareOpType::UNDEFINED_NULL);
    JSTaggedValue resInICPath10 = CompareOp::GreaterEqWithIC(thread, JSTaggedValue::Undefined(),
                                                                JSTaggedValue::True(), CompareOpType::OTHER);

    EXPECT_EQ(resInSlowPath1, resInICPath1);
    EXPECT_EQ(resInSlowPath2, resInICPath2);
    EXPECT_EQ(resInSlowPath3, resInICPath3);
    EXPECT_EQ(resInSlowPath4, resInICPath4);
    EXPECT_EQ(resInSlowPath5, resInICPath5);
    EXPECT_EQ(resInSlowPath9, resInICPath9);
    EXPECT_EQ(resInSlowPath10, resInICPath10);
};
}  // namespace panda::test
