/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/builtins/builtins_boolean.h"
#include "ecmascript/builtins/builtins_function.h"
#include "ecmascript/builtins/builtins_shared_async_function.h"
#include "ecmascript/builtins/builtins_shared_function.h"

#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_object-inl.h"

#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;
using BuiltinsBase = panda::ecmascript::base::BuiltinsBase;
using JSArray = panda::ecmascript::JSArray;

namespace panda::test {
class BuiltinsSharedFunctionTest : public BaseTestWithScope<false> {
};

// native function for test apply and call
JSTaggedValue TestFunctionApplyAndCall(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    int result = 0;
    for (uint32_t index = 0; index < argv->GetArgsNumber(); ++index) {
        result += BuiltinsBase::GetCallArg(argv, index)->GetInt();
    }
    JSHandle<JSTaggedValue> thisValue(BuiltinsBase::GetThis(argv));

    JSTaggedValue testA = JSObject::GetProperty(thread, thisValue,
        JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_a"))).GetValue().GetTaggedValue();
    JSTaggedValue testB = JSObject::GetProperty(thread, thisValue,
        JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_b"))).GetValue().GetTaggedValue();

    result = result + testA.GetInt() + testB.GetInt();
    return BuiltinsBase::GetTaggedInt(result);
}

enum class AlgorithmType {
    PROTOTYPE_APPLY,
    PROTOTYPE_BIND,
    PROTOTYPE_CALL,
};

static JSTaggedValue FunctionAlgorithm(JSThread *thread, JSHandle<JSFunction> &thisArg,
                                       std::vector<JSTaggedValue> &args, uint32_t argLen,
                                       AlgorithmType type = AlgorithmType::PROTOTYPE_APPLY)
{
    auto ecmaRuntimeCallInfos = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), argLen);
    ecmaRuntimeCallInfos->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfos->SetThis(thisArg.GetTaggedValue());
    for (size_t i = 0; i < args.size(); i++) {
        ecmaRuntimeCallInfos->SetCallArg(i, args[i]);
    }
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfos);
    JSTaggedValue result;
    switch (type) {
        case AlgorithmType::PROTOTYPE_BIND:
            result = BuiltinsFunction::FunctionPrototypeBind(ecmaRuntimeCallInfos);
            break;
        case AlgorithmType::PROTOTYPE_APPLY:
            result = BuiltinsFunction::FunctionPrototypeApply(ecmaRuntimeCallInfos);
            break;
        case AlgorithmType::PROTOTYPE_CALL:
            result = BuiltinsFunction::FunctionPrototypeCall(ecmaRuntimeCallInfos);
            break;
        default:
            break;
    }
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

// func Constructor
HWTEST_F_L0(BuiltinsSharedFunctionTest, FunctionConstructor)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    JSTaggedValue result = BuiltinsSharedFunction::SharedFunctionConstructor(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::Exception().GetRawData());
}

// async func Constructor
HWTEST_F_L0(BuiltinsSharedFunctionTest, AsyncFunctionConstructor)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    JSTaggedValue result = BuiltinsSharedAsyncFunction::SharedAsyncFunctionConstructor(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::Exception().GetRawData());
}

// func.apply(thisArg)
HWTEST_F_L0(BuiltinsSharedFunctionTest, FunctionPrototypeApply)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // ecma 19.2.3.1: func
    JSHandle<JSFunction> func = factory->NewSFunction(env, reinterpret_cast<void *>(TestFunctionApplyAndCall));

    // ecma 19.2.3.1: thisArg
    JSHandle<JSObject> thisArg(thread, env->GetGlobalObject());
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(thisArg),
                          JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_a")),
                          JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(thisArg),
                          JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_b")),
                          JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)));

    std::vector<JSTaggedValue> args{thisArg.GetTaggedValue()};
    auto result = FunctionAlgorithm(thread, func, args, 6, AlgorithmType::PROTOTYPE_APPLY);

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(3).GetRawData());

    JSObject::DeleteProperty(thread, (thisArg),
                             JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_a")));
    JSObject::DeleteProperty(thread, (thisArg),
                             JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_b")));
}

// func.apply(thisArg, argArray)
HWTEST_F_L0(BuiltinsSharedFunctionTest, FunctionPrototypeApply1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // ecma 19.2.3.1: func
    JSHandle<JSFunction> func = factory->NewSFunction(env, reinterpret_cast<void *>(TestFunctionApplyAndCall));

    // ecma 19.2.3.1: thisArg
    JSHandle<JSObject> thisArg(thread, env->GetGlobalObject());
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(thisArg),
                          JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_a")),
                          JSHandle<JSTaggedValue>(thread, JSTaggedValue(10)));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(thisArg),
                          JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_b")),
                          JSHandle<JSTaggedValue>(thread, JSTaggedValue(20)));

    // ecma 19.2.3.1: argArray
    JSHandle<JSObject> array(JSArray::ArrayCreate(thread, JSTaggedNumber(2)));
    PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(30)));
    JSArray::DefineOwnProperty(thread, array, JSHandle<JSTaggedValue>(thread, JSTaggedValue(0)), desc);

    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(40)));
    JSArray::DefineOwnProperty(thread, array, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), desc1);

    std::vector<JSTaggedValue> args{thisArg.GetTaggedValue(), array.GetTaggedValue()};
    auto result = FunctionAlgorithm(thread, func, args, 8, AlgorithmType::PROTOTYPE_APPLY);

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(100).GetRawData());

    JSObject::DeleteProperty(thread, (thisArg),
                             JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_a")));
    JSObject::DeleteProperty(thread, (thisArg),
                             JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_b")));
}

// target.bind(thisArg)
HWTEST_F_L0(BuiltinsSharedFunctionTest, FunctionPrototypeBind)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSFunction> target = factory->NewSFunction(env);
    JSHandle<JSObject> thisArg(thread, env->GetGlobalObject());
    std::vector<JSTaggedValue> args{thisArg.GetTaggedValue()};
    auto result = FunctionAlgorithm(thread, target, args, 6, AlgorithmType::PROTOTYPE_BIND);
    ASSERT_TRUE(result.IsECMAObject());

    JSHandle<JSBoundFunction> resultFunc(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    // test BoundTarget
    ASSERT_EQ(resultFunc->GetBoundTarget(), target.GetTaggedValue());
    // test BoundThis
    ASSERT_EQ(resultFunc->GetBoundThis(), thisArg.GetTaggedValue());
    // test BoundArguments
    JSHandle<TaggedArray> array(thread, resultFunc->GetBoundArguments());
    ASSERT_EQ(array->GetLength(), 0U);
}

// target.bind(thisArg, 123, "helloworld")
HWTEST_F_L0(BuiltinsSharedFunctionTest, FunctionPrototypeBind1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSFunction> target = factory->NewSFunction(env);
    JSHandle<JSObject> thisArg(thread, env->GetGlobalObject());
    JSHandle<EcmaString> str = factory->NewFromASCII("helloworld");

    std::vector<JSTaggedValue> args{thisArg.GetTaggedValue(), JSTaggedValue(static_cast<int32_t>(123)),
        str.GetTaggedValue()};
    auto result = FunctionAlgorithm(thread, target, args, 10, AlgorithmType::PROTOTYPE_BIND);
    ASSERT_TRUE(result.IsECMAObject());

    JSHandle<JSBoundFunction> resultFunc(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    // test BoundTarget
    ASSERT_EQ(resultFunc->GetBoundTarget(), target.GetTaggedValue());
    // test BoundThis
    ASSERT_EQ(resultFunc->GetBoundThis(), thisArg.GetTaggedValue());
    // test BoundArguments
    JSHandle<TaggedArray> array(thread, resultFunc->GetBoundArguments());
    ASSERT_EQ(array->GetLength(), 2U);
    JSTaggedValue elem = array->Get(0);
    JSTaggedValue elem1 = array->Get(1);
    ASSERT_EQ(elem.GetRawData(), JSTaggedValue(123).GetRawData());

    ASSERT_EQ(elem1.GetRawData(), str.GetTaggedType());
    ASSERT_TRUE(elem1.IsString());
}

// target.bind(thisArg, 123, "helloworld") set target_name = EmptyString()
HWTEST_F_L0(BuiltinsSharedFunctionTest, FunctionPrototypeBind2)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSFunction> target = factory->NewJSFunction(env);
    PropertyDescriptor nameDesc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(123)), false, false, true);
    JSTaggedValue::DefinePropertyOrThrow(thread, JSHandle<JSTaggedValue>(target),
                                         thread->GlobalConstants()->GetHandledNameString(), nameDesc);
    JSFunction::SetFunctionLength(thread, target, JSTaggedValue(5));

    JSHandle<JSObject> thisArg(thread, env->GetGlobalObject());
    JSHandle<EcmaString> str = factory->NewFromASCII("helloworld");
    std::vector<JSTaggedValue> args{thisArg.GetTaggedValue(), JSTaggedValue(static_cast<int32_t>(123)),
        str.GetTaggedValue()};
    auto result = FunctionAlgorithm(thread, target, args, 10, AlgorithmType::PROTOTYPE_BIND);
    ASSERT_TRUE(result.IsECMAObject());

    JSHandle<JSBoundFunction> resultFunc(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    // test BoundThis
    ASSERT_EQ(resultFunc->GetBoundThis(), thisArg.GetTaggedValue());
    // test BoundTarget
    ASSERT_EQ(resultFunc->GetBoundTarget(), target.GetTaggedValue());
    // test BoundArguments
    JSHandle<TaggedArray> array(thread, resultFunc->GetBoundArguments());
    ASSERT_EQ(array->GetLength(), 2U);
    JSTaggedValue elem = array->Get(0);
    JSTaggedValue elem1 = array->Get(1);
    ASSERT_EQ(elem.GetRawData(), JSTaggedValue(123).GetRawData());

    ASSERT_TRUE(elem1.IsString());
    ASSERT_EQ(elem1.GetRawData(), str.GetTaggedType());
}

// func.call(thisArg)
HWTEST_F_L0(BuiltinsSharedFunctionTest, FunctionPrototypeCall)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // ecma 19.2.3.3: func
    JSHandle<JSFunction> func = factory->NewSFunction(env, reinterpret_cast<void *>(TestFunctionApplyAndCall));
    // ecma 19.2.3.3: thisArg
    JSHandle<JSObject> thisArg(thread, env->GetGlobalObject());
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(thisArg),
                          JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_a")),
                          JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(thisArg),
                          JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_b")),
                          JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)));
    std::vector<JSTaggedValue> args{thisArg.GetTaggedValue()};
    auto result = FunctionAlgorithm(thread, func, args, 6, AlgorithmType::PROTOTYPE_CALL);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(3).GetRawData());

    JSObject::DeleteProperty(thread, (thisArg),
                             JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_a")));
    JSObject::DeleteProperty(thread, (thisArg),
                             JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_b")));
}

// func.call(thisArg, 123, 456, 789)
HWTEST_F_L0(BuiltinsSharedFunctionTest, FunctionPrototypeCall1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // ecma 19.2.3.3: func
    JSHandle<JSFunction> func = factory->NewSFunction(env, reinterpret_cast<void *>(TestFunctionApplyAndCall));
    // ecma 19.2.3.3: thisArg
    JSHandle<JSObject> thisArg(thread, env->GetGlobalObject());
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(thisArg),
                          JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_a")),
                          JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(thisArg),
                          JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_b")),
                          JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)));

    // func thisArg ...args
    std::vector<JSTaggedValue> args{thisArg.GetTaggedValue(), JSTaggedValue(static_cast<int32_t>(123)),
                                JSTaggedValue(static_cast<int32_t>(456)), JSTaggedValue(static_cast<int32_t>(789))};
    auto result = FunctionAlgorithm(thread, func, args, 12, AlgorithmType::PROTOTYPE_CALL);

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(1371).GetRawData());

    JSObject::DeleteProperty(thread, (thisArg),
                             JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_a")));
    JSObject::DeleteProperty(thread, (thisArg),
                             JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_b")));
}

HWTEST_F_L0(BuiltinsSharedFunctionTest, FunctionPrototypeHasInstance)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> booleanCtor(env->GetBooleanFunction());

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*booleanCtor), 6);
    ecmaRuntimeCallInfo1->SetFunction(booleanCtor.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(123)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = BuiltinsBoolean::BooleanConstructor(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSObject> booleanInstance(thread, result);

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(booleanCtor.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, booleanInstance.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    EXPECT_TRUE(BuiltinsFunction::FunctionPrototypeHasInstance(ecmaRuntimeCallInfo2).GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

/**
 * @tc.name: FunctionPrototypeToString
 * @tc.desc: Create msgs through "CreateEcmaRuntimeCallInfo" function, Set ArgsNumber and CallArg, then call
 *           the "FunctionPrototypeToString" function to get the result of Function.prototype.call.toString().
 * @tc.type: FUNC
 * @tc.require: issueI5INW1
 */
HWTEST_F_L0(BuiltinsSharedFunctionTest, FunctionPrototypeToString)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> func = factory->NewSFunction(
                                env, reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeCall));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(func.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsFunction::FunctionPrototypeToString(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSHandle<EcmaString> test = factory->NewFromASCII("function undefined() { [native code] }");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, resultHandle, test), 0);
}
}  // namespace panda::test
