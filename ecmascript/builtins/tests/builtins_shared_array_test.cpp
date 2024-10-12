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

#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"

#include "ecmascript/object_factory.h"
#include "ecmascript/object_operator.h"
#include "ecmascript/tests/test_helper.h"
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
                                                       SCheckMode::SKIP);
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
    };
};

std::vector<JSHandle<JSTaggedValue>> SharedArrayDefineOwnPropertyTest(JSThread* thread, JSHandle<JSObject>& obj,
    std::vector<int>& vals)
{
    std::vector<JSHandle<JSTaggedValue>> keys;
    for (size_t i = 0; i < vals.size(); i++) {
        keys.push_back(JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<int>(i))));
        PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(vals[i])), true, true, true);
        JSArray::DefineOwnProperty(thread, obj, keys[i], desc0);
    }
    return keys;
}

HWTEST_F_L0(BuiltinsSharedArrayTest, Create1)
{
    static int32_t len = 3;
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0))->GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP) \
        .GetValue()->GetInt(), 0);

    JSHandle<JSFunction> array(thread->GetEcmaVM()->GetGlobalEnv()->GetSharedArrayFunction());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, array.GetTaggedValue(), 8);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(len));
    ecmaRuntimeCallInfo1->SetCallArg(1, obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Create(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result.IsECMAObject());
    EXPECT_EQ(JSSharedArray::Cast(result.GetTaggedObject())->GetArrayLength(), len);
}

HWTEST_F_L0(BuiltinsSharedArrayTest, Create2)
{
    static double len = 100;
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0))->GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP) \
        .GetValue()->GetInt(), 0);

    JSHandle<JSFunction> array(thread->GetEcmaVM()->GetGlobalEnv()->GetSharedArrayFunction());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, array.GetTaggedValue(), 8);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(len));
    ecmaRuntimeCallInfo1->SetCallArg(1, obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Create(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result.IsECMAObject());
    EXPECT_EQ(JSSharedArray::Cast(result.GetTaggedObject())->GetArrayLength(), len);
}

HWTEST_F_L0(BuiltinsSharedArrayTest, ShrinkTo1)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(10))->GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);

    std::vector<int> descVals{1, 2, 3, 4, 5};
    auto keys = SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    JSHandle<JSFunction> array(thread->GetEcmaVM()->GetGlobalEnv()->GetSharedArrayFunction());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, array.GetTaggedValue(), 6);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(3)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::ShrinkTo(ecmaRuntimeCallInfo1);
    EXPECT_EQ(result, JSTaggedValue::Undefined());
    EXPECT_EQ(arr->GetArrayLength(), 3U);
    TestHelper::TearDownFrame(thread, prev);
}

HWTEST_F_L0(BuiltinsSharedArrayTest, ShrinkTo2)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(10))->GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);

    std::vector<int> descVals{1, 2, 3, 4, 5};
    auto keys = SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    JSHandle<JSFunction> array(thread->GetEcmaVM()->GetGlobalEnv()->GetSharedArrayFunction());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, array.GetTaggedValue(), 6);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<double>(3)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::ShrinkTo(ecmaRuntimeCallInfo1);
    EXPECT_EQ(result, JSTaggedValue::Undefined());
    EXPECT_EQ(arr->GetArrayLength(), 3U);
    TestHelper::TearDownFrame(thread, prev);
}

HWTEST_F_L0(BuiltinsSharedArrayTest, ExtendTo1)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0))->GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);

    JSHandle<JSFunction> array(thread->GetEcmaVM()->GetGlobalEnv()->GetSharedArrayFunction());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, array.GetTaggedValue(), 8);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(3)));
    ecmaRuntimeCallInfo1->SetCallArg(1, obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::ExtendTo(ecmaRuntimeCallInfo1);
    ASSERT_EQ(result, JSTaggedValue::Undefined());
    EXPECT_EQ(arr->GetArrayLength(), 3U);
    TestHelper::TearDownFrame(thread, prev);
}

HWTEST_F_L0(BuiltinsSharedArrayTest, ExtendTo2)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0))->GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);

    JSHandle<JSFunction> array(thread->GetEcmaVM()->GetGlobalEnv()->GetSharedArrayFunction());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, array.GetTaggedValue(), 8);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<double>(3)));
    ecmaRuntimeCallInfo1->SetCallArg(1, obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::ExtendTo(ecmaRuntimeCallInfo1);
    ASSERT_EQ(result, JSTaggedValue::Undefined());
    EXPECT_EQ(arr->GetArrayLength(), 3U);
    TestHelper::TearDownFrame(thread, prev);
}
}  // namespace panda::test