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

#include "ecmascript/builtins/builtins_shared_array.h"

#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/shared_objects/js_shared_array.h"
#include "ecmascript/shared_objects/js_shared_array_iterator.h"
#include "ecmascript/js_array_iterator.h"

#include "ecmascript/builtins/builtins_shared_map.h"
#include "ecmascript/shared_objects/js_shared_map.h"
#include "ecmascript/shared_objects/js_shared_map_iterator.h"
#include "ecmascript/builtins/builtins_shared_set.h"
#include "ecmascript/shared_objects/js_shared_set.h"
#include "ecmascript/shared_objects/js_shared_set_iterator.h"

#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"

#include "ecmascript/object_factory.h"
#include "ecmascript/object_operator.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/builtins/tests/builtin_test_util.h"
#include "ecmascript/builtins/builtins_object.h"
#include "ecmascript/js_array.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;
using namespace panda::ecmascript::base;

enum class ArrayIndex {
    ARRAY_INDEX_0,
    ARRAY_INDEX_1,
    ARRAY_INDEX_2,
    ARRAY_INDEX_3
};

namespace panda::test {
using Array = ecmascript::builtins::BuiltinsSharedArray;
class BuiltinsSharedArrayTest : public BaseTestWithScope<false> {
public:
    class TestClass : public base::BuiltinsBase {
    public:
        static JSTaggedValue TestForEachFunc(EcmaRuntimeCallInfo *argv)
        {
            JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);
            if (key->IsUndefined()) {
                return JSTaggedValue::Undefined();
            }
            JSSharedArray *JSSharedArray = JSSharedArray::Cast(GetThis(argv)->GetTaggedObject());
            uint32_t length = JSSharedArray->GetArrayLength() + 1U;
            JSSharedArray->SetArrayLength(argv->GetThread(), length);
            return JSTaggedValue::Undefined();
        }

        static JSTaggedValue TestEveryFunc(EcmaRuntimeCallInfo *argv)
        {
            uint32_t argc = argv->GetArgsNumber();
            if (argc > 0) {
                if (GetCallArg(argv, 0)->GetInt() > 10) { // 10 : test case
                    return GetTaggedBoolean(true);
                }
            }
            return GetTaggedBoolean(false);
        }

        static JSTaggedValue TestMapFunc(EcmaRuntimeCallInfo *argv)
        {
            int accumulator = GetCallArg(argv, 0)->GetInt();
            accumulator = accumulator * 2; // 2 : mapped to 2 times the original value
            return BuiltinsBase::GetTaggedInt(accumulator);
        }

        static JSTaggedValue TestFlatMapFunc(EcmaRuntimeCallInfo *argv)
        {
            int accumulator = GetCallArg(argv, 0)->GetInt();
            accumulator = accumulator * 2; // 2 : mapped to 2 times the original value

            JSThread *thread = argv->GetThread();
            JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
            JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
                .GetTaggedValue().GetTaggedObject());
            EXPECT_TRUE(arr != nullptr);
            JSHandle<JSObject> obj(thread, arr);
            auto property = JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle,
                                                       SCheckMode::SKIP, SCheckMode::SKIP);
            EXPECT_EQ(property.GetValue()->GetInt(), 0);

            JSHandle<JSTaggedValue> key(thread, JSTaggedValue(0));
            PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(accumulator)),
                                                                    true, true, true);
            JSSharedArray::DefineOwnProperty(thread, obj, key, desc, SCheckMode::SKIP);
            return obj.GetTaggedValue();
        }

        static JSTaggedValue TestFindFunc(EcmaRuntimeCallInfo *argv)
        {
            uint32_t argc = argv->GetArgsNumber();
            if (argc > 0) {
                // 10 : test case
                if (GetCallArg(argv, 0)->GetInt() > 10) {
                    return GetTaggedBoolean(true);
                }
            }
            return GetTaggedBoolean(false);
        }

        static JSTaggedValue TestFindIndexFunc(EcmaRuntimeCallInfo *argv)
        {
            uint32_t argc = argv->GetArgsNumber();
            if (argc > 0) {
                // 10 : test case
                if (GetCallArg(argv, 0)->GetInt() > 10) {
                    return GetTaggedBoolean(true);
                }
            }
            return GetTaggedBoolean(false);
        }

        static JSTaggedValue TestFindLastFunc(EcmaRuntimeCallInfo *argv)
        {
            uint32_t argc = argv->GetArgsNumber();
            if (argc > 0) {
                // 20 : test case
                if (GetCallArg(argv, 0)->GetInt() > 20) {
                    return GetTaggedBoolean(true);
                }
            }
            return GetTaggedBoolean(false);
        }

        static JSTaggedValue TestFindLastIndexFunc(EcmaRuntimeCallInfo *argv)
        {
            uint32_t argc = argv->GetArgsNumber();
            if (argc > 0) {
                // 20 : test case
                if (GetCallArg(argv, 0)->GetInt() > 20) {
                    return GetTaggedBoolean(true);
                }
            }
            return GetTaggedBoolean(false);
        }

        static JSTaggedValue TestReduceFunc(EcmaRuntimeCallInfo *argv)
        {
            int accumulator = GetCallArg(argv, 0)->GetInt();
            accumulator = accumulator + GetCallArg(argv, 1)->GetInt();
            return BuiltinsBase::GetTaggedInt(accumulator);
        }

        static JSTaggedValue TestReduceRightFunc(EcmaRuntimeCallInfo *argv)
        {
            int accumulator = GetCallArg(argv, 0)->GetInt();
            accumulator = accumulator + GetCallArg(argv, 1)->GetInt();
            return BuiltinsBase::GetTaggedInt(accumulator);
        }

        static JSTaggedValue TestSomeFunc(EcmaRuntimeCallInfo *argv)
        {
            uint32_t argc = argv->GetArgsNumber();
            if (argc > 0) {
                if (GetCallArg(argv, 0)->GetInt() > 10) { // 10 : test case
                    return GetTaggedBoolean(true);
                }
            }
            return GetTaggedBoolean(false);
        }

        static JSTaggedValue TestToSortedFunc(EcmaRuntimeCallInfo *argv)
        {
            uint32_t argc = argv->GetArgsNumber();
            if (argc > 1) {
                // x < y
                if (GetCallArg(argv, 0)->GetInt() > GetCallArg(argv, 1)->GetInt()) {
                    return GetTaggedBoolean(true);
                }
            }
            return GetTaggedBoolean(false);
        }

        static JSTaggedValue TestFromFunc(EcmaRuntimeCallInfo *argv)
        {
            int accumulator = GetCallArg(argv, 0)->GetInt();
            int coefficient = 2;
            accumulator = accumulator * coefficient;
            return BuiltinsBase::GetTaggedInt(accumulator);
        }

        static JSSharedMap *CreateSBuiltinsMap(JSThread *thread)
        {
            JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
            JSHandle<JSFunction> newTarget(env->GetSBuiltininMapFunction());
            // 4 : test case
            auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 4);
            ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
            ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
            JSTaggedValue result = BuiltinsSharedMap::Constructor(ecmaRuntimeCallInfo);
            TestHelper::TearDownFrame(thread, prev);

            EXPECT_TRUE(result.IsECMAObject());
            JSSharedMap *jsSMap = JSSharedMap::Cast(reinterpret_cast<TaggedObject *>(result.GetRawData()));
            return jsSMap;
        }

        static JSSharedSet *CreateBuiltinsSharedSet(JSThread *thread)
        {
            JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
            JSHandle<JSFunction> newTarget(env->GetSBuiltininSetFunction());
            // 4 : test case
            auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 4);
            ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
            ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
            JSTaggedValue result = BuiltinsSharedSet::Constructor(ecmaRuntimeCallInfo);
            TestHelper::TearDownFrame(thread, prev);

            EXPECT_TRUE(result.IsECMAObject());
            JSSharedSet *jsSSet = JSSharedSet::Cast(reinterpret_cast<TaggedObject *>(result.GetRawData()));
            return jsSSet;
        }
    };
};

// Array.from ( items [ , mapfn [ , thisArg ] ] )
HWTEST_F_L0(BuiltinsSharedArrayTest, From_Two)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0))->GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);

    std::vector<int> descVals{1, 2, 3, 4, 5};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestFromFunc));

    JSHandle<JSFunction> array(thread->GetEcmaVM()->GetGlobalEnv()->GetSharedArrayFunction());

    JSHandle<JSTaggedValue> map(thread, TestClass::CreateSBuiltinsMap(thread));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(map.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);

    // test Values()
    JSTaggedValue resultMap = BuiltinsSharedMap::Values(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<JSSharedMapIterator> iter(thread, resultMap);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, array.GetTaggedValue(), 8);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, iter.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, func.GetTaggedValue());

    [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::From(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev1);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
}

// Array.from ( items [ , mapfn [ , thisArg ] ] )
HWTEST_F_L0(BuiltinsSharedArrayTest, From_Four)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0))->GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);

    std::vector<int> descVals{1, 2, 3, 4, 5};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestFromFunc));

    JSHandle<JSFunction> array(thread->GetEcmaVM()->GetGlobalEnv()->GetSharedArrayFunction());

    JSHandle<JSTaggedValue> set(thread, TestClass::CreateBuiltinsSharedSet(thread));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(set.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);

    // test Values()
    JSTaggedValue resultSet = BuiltinsSharedSet::Values(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<JSSharedMapIterator> iter(thread, resultSet);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, array.GetTaggedValue(), 8);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, iter.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, func.GetTaggedValue());

    [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::From(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev1);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedArrayTest, From_Three)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0))->GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);

    std::vector<int> descVals{1, 2, 3, 4, 5};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestFromFunc));

    JSHandle<JSFunction> array(thread->GetEcmaVM()->GetGlobalEnv()->GetSharedArrayFunction());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, array.GetTaggedValue(), 8);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, func.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::From(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    std::vector<int> vals{2, 4, 6, 8, 10};
    BuiltTestUtil::SharedArrayCheckKeyValueCommon(thread, valueHandle, descRes, keys, vals);
}

HWTEST_F_L0(BuiltinsSharedArrayTest, Map)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);

    std::vector<int> descVals{50, 200, 3};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    JSHandle<JSSharedArray> JSSharedArray(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestMapFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, JSSharedArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Map(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());

    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    std::vector<int> vals{100, 400, 6};
    EXPECT_EQ(
        JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKeyHandle,
                                   SCheckMode::SKIP, SCheckMode::SKIP).GetValue()->GetInt(), 3);
    BuiltTestUtil::SharedArrayCheckKeyValueCommon(thread, valueHandle, descRes, keys, vals);
}

HWTEST_F_L0(BuiltinsSharedArrayTest, Find)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);

    std::vector<int> vals{1, 102, 3};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, vals);
    JSHandle<JSSharedArray> JSSharedArray(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestFindFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, JSSharedArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result2 = Array::Find(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(result2.GetRawData(), JSTaggedValue(102).GetRawData());
}

HWTEST_F_L0(BuiltinsSharedArrayTest, FindLast_One)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(
        JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);
    // arr [50, 40, 2]
    std::vector<int> descVals{50, 40, 2};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    JSHandle<JSSharedArray> JSSharedArray(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestFindLastFunc));

    auto ecmaRuntimeCallInfo1 =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8); // 8 means 2 call args
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, JSSharedArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::FindLast(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(result.GetRawData(), JSTaggedValue(40).GetRawData());
}

HWTEST_F_L0(BuiltinsSharedArrayTest, FindLast_Two)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(
        JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);
    // arr [5, 7, 9]
    std::vector<int> descVals{5, 7, 9};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    JSHandle<JSSharedArray> JSSharedArray(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestFindLastFunc));

    auto ecmaRuntimeCallInfo1 =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8); // 8 means 2 call args
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, JSSharedArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::FindLast(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_EQ(result.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);
}

HWTEST_F_L0(BuiltinsSharedArrayTest, FindLastIndex_One)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(
        JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);
    // arr [50, 40, 2]
    std::vector<int> descVals{50, 40, 2};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    JSHandle<JSSharedArray> JSSharedArray(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestFindLastIndexFunc));

    auto ecmaRuntimeCallInfo1 =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8); // 8 means 2 call args
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, JSSharedArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::FindLastIndex(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(result.GetRawData(), JSTaggedValue(static_cast<double>(1)).GetRawData());
}

HWTEST_F_L0(BuiltinsSharedArrayTest, FindLastIndex_Two)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(
        JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle,
                                         SCheckMode::SKIP, SCheckMode::SKIP).GetValue()->GetInt(), 0);
    // arr [5, 7, 9]
    std::vector<int> descVals{5, 7, 9};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    JSHandle<JSSharedArray> JSSharedArray(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env,
        reinterpret_cast<void *>(TestClass::TestFindLastIndexFunc));

    auto ecmaRuntimeCallInfo1 =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8); // 8 means 2 call args
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, JSSharedArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::FindLastIndex(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(result.GetRawData(), JSTaggedValue(static_cast<double>(-1)).GetRawData());
}

HWTEST_F_L0(BuiltinsSharedArrayTest, FindIndex)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);

    std::vector<int> descVals{1, 2, 30};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    JSHandle<JSSharedArray> JSSharedArray(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestFindIndexFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, JSSharedArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result2 = Array::FindIndex(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(result2.GetRawData(), JSTaggedValue(static_cast<double>(2)).GetRawData());
}

HWTEST_F_L0(BuiltinsSharedArrayTest, ForEach)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);

    std::vector<int> descVals{1, 2, 3};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    JSHandle<JSSharedArray> JSSharedArray(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForEachFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, JSSharedArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result2 = Array::ForEach(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result2.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);
    EXPECT_EQ(JSSharedArray->GetArrayLength(), 3U);
}

#define ARRAY_DEFINE_OWN_PROPERTY(dest, index, value)                                                               \
    do {                                                                                                            \
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(index));                                                  \
        PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(value)), true, true, true);   \
        JSSharedArray::DefineOwnProperty(thread, dest, key, desc, SCheckMode::SKIP);                                \
    } while (false)

#define ARRAY_BUILTIN_METHOD_TEST_CASE_ARG0(method, target, expected)                                               \
    do {                                                                                                            \
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);    \
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());                                               \
        ecmaRuntimeCallInfo->SetThis((target).GetTaggedValue());                                                    \
                                                                                                                    \
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);                           \
        JSTaggedValue result = Array::method(ecmaRuntimeCallInfo);                                                  \
        TestHelper::TearDownFrame(thread, prev);                                                                    \
        ASSERT_TRUE(JSTaggedValue::StrictEqual(thread, result, JSTaggedValue(expected)));                           \
    } while (false)

#define ARRAY_BUILTIN_METHOD_TEST_CASE_ARG1(method, target, expected, arg0)                                         \
    do {                                                                                                            \
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);    \
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());                                               \
        ecmaRuntimeCallInfo->SetThis((target).GetTaggedValue());                                                    \
        ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(arg0));                                                    \
                                                                                                                    \
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);                           \
        JSTaggedValue result = Array::method(ecmaRuntimeCallInfo);                                                  \
        TestHelper::TearDownFrame(thread, prev);                                                                    \
        ASSERT_TRUE(JSTaggedValue::StrictEqual(thread, result, JSTaggedValue(expected)));                           \
    } while (false)

#define ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(method, target, expected, arg0, arg1)                                   \
    do {                                                                                                            \
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);    \
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());                                               \
        ecmaRuntimeCallInfo->SetThis((target).GetTaggedValue());                                                    \
        ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(arg0));                                                    \
        ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(arg1));                                                    \
                                                                                                                    \
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);                           \
        JSTaggedValue result = Array::method(ecmaRuntimeCallInfo);                                                  \
        TestHelper::TearDownFrame(thread, prev);                                                                    \
        ASSERT_TRUE(JSTaggedValue::StrictEqual(thread, result, JSTaggedValue(expected)));                           \
    } while (false)

// Array.IndexOf(searchElement [ , fromIndex ])
HWTEST_F_L0(BuiltinsSharedArrayTest, IndexOf)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(20)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 20);

    // arr = [1, 2, 3, 4, 3, 0, 2.0, +0.0, 3.0, -0.0, <hole>, <hole>, undefined]
    ARRAY_DEFINE_OWN_PROPERTY(obj, 0, 1);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 1, 2);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 2, 3);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 3, 4);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 4, 3);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 5, 0);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 6, 2.0);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 7, +0.0);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 8, 3.0);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 9, -0.0);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 12, JSTaggedValue::Undefined());

    // arr.indexOf(3, 0) == 2
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(IndexOf, obj, 2, 3, 0);
    // arr.indexOf(3, 3) == 4
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(IndexOf, obj, 4, 3, 3);
    // arr.indexOf(5, 0) == -1
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(IndexOf, obj, -1, 5, 0);
    // arr.indexOf(3) == 2
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG1(IndexOf, obj, 2, 3);

    // Expects int32_t(x) and double(x) to be strictly equal
    // arr.indexOf(3.0) == 2
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG1(IndexOf, obj, 2, 3.0);
    // arr.indexOf(3, 5) == 8
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(IndexOf, obj, 8, 3, 5);

    // Expects 0, +0.0, -0.0 to be strictly equal
    // arr.indexOf(+0.0) == 5
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG1(IndexOf, obj, 5, +0.0);
    // arr.indexOf(-0.0) == 5
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG1(IndexOf, obj, 5, -0.0);
    // arr.indexOf(0, 6) == 7
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(IndexOf, obj, 7, 0, 6);
    // arr.indexOf(-0.0, 6) == 7
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(IndexOf, obj, 7, -0.0, 6);
    // arr.indexOf(0, 8) == 9
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(IndexOf, obj, 9, 0, 8);
    // arr.indexOf(+0.0, 8) == 9
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(IndexOf, obj, 9, +0.0, 8);

    // Expects undefined to be found
    // arr.indexOf() == 12, where the first argument is undefined
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG0(IndexOf, obj, 12);
}

// Array.LastIndexOf(searchElement [ , fromIndex ])
HWTEST_F_L0(BuiltinsSharedArrayTest, LastIndexOf)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0))->GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);

    // arr.lastIndexOf(0) == -1
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG1(LastIndexOf, obj, -1, 0);

    // arr = [1, 2, 3, 4, 3, 0, 2.0, +0.0, 3.0, -0.0, <hole>, <hole>, undefined, <hole>, <hole>, -1]
    ARRAY_DEFINE_OWN_PROPERTY(obj, 0, 1);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 1, 2);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 2, 3);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 3, 4);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 4, 3);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 5, 0);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 6, 2.0);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 7, +0.0);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 8, 3.0);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 9, -0.0);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 12, JSTaggedValue::Undefined());
    ARRAY_DEFINE_OWN_PROPERTY(obj, 15, -1);

    // arr.lastIndexOf(1, -17) == -1
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(LastIndexOf, obj, -1, 1, -17);

    // arr.lastIndexOf(3, 4) == 4
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(LastIndexOf, obj, 4, 3, 4);
    // arr.lastIndexOf(3, 3) == 2
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(LastIndexOf, obj, 2, 3, 3);
    // arr.lastIndexOf(5, 4) == -1
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(LastIndexOf, obj, -1, 5, 4);

    // Expects int32_t(x) and double(x) to be strictly equal
    // arr.lastIndexOf(3) == 8
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG1(LastIndexOf, obj, 8, 3);
    // arr.lastIndexOf(1.0) == 0
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG1(LastIndexOf, obj, 0, 1.0);

    // Expects 0, +0.0, -0.0 to be strictly equal
    // arr.indexOf(+0.0) == 9
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG1(LastIndexOf, obj, 9, +0.0);
    // arr.indexOf(0) == 9
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG1(LastIndexOf, obj, 9, 0);
    // arr.indexOf(0, 8) == 7
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(LastIndexOf, obj, 7, 0, 8);
    // arr.indexOf(-0.0, 8) == 7
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(LastIndexOf, obj, 7, -0.0, 8);
    // arr.indexOf(-0.0, 6) == 5
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(LastIndexOf, obj, 5, -0.0, 6);
    // arr.indexOf(+0.0, 6) == 5
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(LastIndexOf, obj, 5, +0.0, 6);

    // Expects undefined to be found
    // arr.indexOf() == 12, where the first argument is undefined
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG0(LastIndexOf, obj, 12);
}

HWTEST_F_L0(BuiltinsSharedArrayTest, ReduceRight_One)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(
        JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);

    std::vector<int> descVals{1, 2, 3};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestReduceRightFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    // set initial value to 10
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(10)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::ReduceRight(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(16).GetRawData());
}

HWTEST_F_L0(BuiltinsSharedArrayTest, ReduceRight_Two)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(
        JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle,
            SCheckMode::SKIP, SCheckMode::SKIP).GetValue()->GetInt(), 0);

    std::vector<int> descVals{1, 2, 3};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestReduceRightFunc));
    // no initial value
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::ReduceRight(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(6).GetRawData());
}

HWTEST_F_L0(BuiltinsSharedArrayTest, IncludeInSortedValue)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(7)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);

    std::vector<int> descVals{2, 3, 4, 5, 7, 500, 600};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestToSortedFunc));
    JSHandle<JSTaggedValue> funcValue(thread, func.GetTaggedValue());

    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 0);
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);

    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(7));
    auto res = JSSharedArray::IncludeInSortedValue(thread, JSHandle<JSTaggedValue>(obj), value);
    ASSERT_TRUE(res);
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(100));
    res = JSSharedArray::IncludeInSortedValue(thread, JSHandle<JSTaggedValue>(obj), value1);
    ASSERT_TRUE(!res);
    TestHelper::TearDownFrame(thread, prev);
}

HWTEST_F_L0(BuiltinsSharedArrayTest, Some)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr =
        JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle,
                                         SCheckMode::SKIP, SCheckMode::SKIP).GetValue()->GetInt(), 0);

    std::vector<int> descVals{1, 20, 3};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    JSHandle<JSSharedArray> jsSharedArray(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestSomeFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, jsSharedArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result2 = Array::Some(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result2.GetRawData(), JSTaggedValue::True().GetRawData());
}

HWTEST_F_L0(BuiltinsSharedArrayTest, Every)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread,
                                             JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj),
        lengthKeyHandle, SCheckMode::SKIP, SCheckMode::SKIP).GetValue()->GetInt(), 0);

    std::vector<int> descVals{100, 200, 300};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    JSHandle<JSSharedArray> jsSharedArray(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestEveryFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, jsSharedArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result2 = Array::Every(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_EQ(result2.GetRawData(), JSTaggedValue::True().GetRawData());
    EXPECT_EQ(JSSharedArrayIterator::Next(ecmaRuntimeCallInfo1), JSTaggedValue::Exception());
}

template<typename T=int>
std::vector<JSHandle<JSTaggedValue>> DefineSharedArrProperty(JSThread* thread, JSHandle<JSObject>& obj,
                                                             std::vector<T>& vals)
{
    std::vector<JSHandle<JSTaggedValue>> keys;
    for (size_t i = 0; i < vals.size(); i++) {
        keys.push_back(JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<int>(i))));
        ARRAY_DEFINE_OWN_PROPERTY(obj, static_cast<int>(i), vals[i]);
    }
    return keys;
}

template<typename T=int>
void CheckSharedArrKeyValue(JSThread* thread, JSHandle<JSObject>& valueHandle, PropertyDescriptor& descRes,
    std::vector<JSHandle<JSTaggedValue>>& keys, std::vector<T>& vals)
{
    for (size_t i = 0; i < vals.size(); i++) {
        JSObject::GetOwnProperty(thread, valueHandle, keys[i], descRes);
        ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(vals[i]));
    }
}

HWTEST_F_L0(BuiltinsSharedArrayTest, Of_One)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    JSHandle<JSFunction> array(env->GetSharedArrayFunction());

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(3)));
    ecmaRuntimeCallInfo1->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Of(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    std::vector<JSHandle<JSTaggedValue>> keys{key0, key1, key2};
    std::vector<int> vals{1, 3, 5};
    CheckSharedArrKeyValue(thread, valueHandle, descRes, keys, vals);
}

HWTEST_F_L0(BuiltinsSharedArrayTest, Of_Two)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    JSHandle<JSFunction> array(env->GetSharedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(2)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(4)));
    ecmaRuntimeCallInfo1->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(6)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Of(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    std::vector<JSHandle<JSTaggedValue>> keys{key0, key1, key2};
    std::vector<int> vals{2, 4, 6};
    CheckSharedArrKeyValue(thread, valueHandle, descRes, keys, vals);
}

HWTEST_F_L0(BuiltinsSharedArrayTest, Reduce)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);

    std::vector<int> descVals{1, 2, 3};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestReduceFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(10)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Reduce(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(16).GetRawData());
}

HWTEST_F_L0(BuiltinsSharedArrayTest, CopyWithin_One)
{
    std::vector<int> descVals{1, 2, 3, 4, 5};
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread,
                                             JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle,
              SCheckMode::SKIP, SCheckMode::SKIP).GetValue()->GetInt(), 0);
    auto keys = DefineSharedArrProperty(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(0)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(3)));
    ecmaRuntimeCallInfo1->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::CopyWithin(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    std::vector<int> vals{4, 5, 3, 4, 5};
    CheckSharedArrKeyValue(thread, valueHandle, descRes, keys, vals);
}

HWTEST_F_L0(BuiltinsSharedArrayTest, CopyWithin_Two)
{
    std::vector<int> descVals{1, 2, 3, 4, 5};
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread,
                                             JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle,
              SCheckMode::SKIP, SCheckMode::SKIP).GetValue()->GetInt(), 0);
    auto keys = DefineSharedArrProperty(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::CopyWithin(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsException());
}

HWTEST_F_L0(BuiltinsSharedArrayTest, CopyWithin_Three)
{
    std::vector<int> descVals{1, 2, 3, 4, 5};
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread,
                                             JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle,
              SCheckMode::SKIP, SCheckMode::SKIP).GetValue()->GetInt(), 0);
    auto keys = DefineSharedArrProperty(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(0)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(2, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::CopyWithin(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    std::vector<int> vals{1, 2, 3, 4, 5};
    CheckSharedArrKeyValue(thread, valueHandle, descRes, keys, vals);
}

HWTEST_F_L0(BuiltinsSharedArrayTest, CopyWithin_Four)
{
    std::vector<int> descVals{1, 2, 3, 4, 5, 6};
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread,
                                             JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle,
              SCheckMode::SKIP, SCheckMode::SKIP).GetValue()->GetInt(), 0);
    auto keys = DefineSharedArrProperty(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(-3)));
    ecmaRuntimeCallInfo1->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::CopyWithin(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    std::vector<int> vals{1, 4, 5, 4, 5, 6};
    CheckSharedArrKeyValue(thread, valueHandle, descRes, keys, vals);
}

HWTEST_F_L0(BuiltinsSharedArrayTest, CopyWithin_Five)
{
    std::vector<int> descVals{1, 2, 3, 4, 5, 6, 7, 8};
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread,
                                             JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle,
              SCheckMode::SKIP, SCheckMode::SKIP).GetValue()->GetInt(), 0);
    auto keys = DefineSharedArrProperty(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(4)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(2)));
    ecmaRuntimeCallInfo1->SetCallArg(2, JSTaggedValue(JSTaggedValue::Undefined()));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::CopyWithin(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    std::vector<int> vals{1, 2, 3, 4, 3, 4, 5, 6};
    CheckSharedArrKeyValue(thread, valueHandle, descRes, keys, vals);
}

HWTEST_F_L0(BuiltinsSharedArrayTest, Reverse)
{
    std::vector<int> descVals{1, 2, 3, 4, 5};
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread,
                                             JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle,
              SCheckMode::SKIP, SCheckMode::SKIP).GetValue()->GetInt(), 0);
    auto keys = DefineSharedArrProperty(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Reverse(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    std::vector<int> vals{5, 4, 3, 2, 1};
    CheckSharedArrKeyValue(thread, valueHandle, descRes, keys, vals);
}

HWTEST_F_L0(BuiltinsSharedArrayTest, Reverse_one)
{
    std::vector<double> descVals{1.5, 2.3, 3.6, 4.4, 5.9};
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread,
                                             JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle,
              SCheckMode::SKIP, SCheckMode::SKIP).GetValue()->GetInt(), 0);
    auto keys = DefineSharedArrProperty<double>(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Reverse(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    std::vector<double> vals{5.9, 4.4, 3.6, 2.3, 1.5};
    CheckSharedArrKeyValue<double>(thread, valueHandle, descRes, keys, vals);
}
}  // namespace panda::test
