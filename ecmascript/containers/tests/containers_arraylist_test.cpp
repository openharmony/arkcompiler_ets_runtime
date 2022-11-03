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


#include "ecmascript/containers/containers_arraylist.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/js_api/js_api_arraylist_iterator.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;

namespace panda::test {
class ContainersArrayListTest : public testing::Test {
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

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};

    class TestClass : public base::BuiltinsBase {
    public:
        static JSTaggedValue TestForEachFunc(EcmaRuntimeCallInfo *argv)
        {
            JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> arrayList = GetCallArg(argv, 1);
            if (!arrayList->IsUndefined()) {
                if (value->IsNumber()) {
                    TaggedArray *elements = TaggedArray::Cast(JSAPIArrayList::Cast(arrayList.GetTaggedValue().
                                            GetTaggedObject())->GetElements().GetTaggedObject());
                    JSTaggedValue result = elements->Get(value->GetInt());
                    EXPECT_EQ(result, value.GetTaggedValue());
                }
            }
            return JSTaggedValue::True();
        }
    };
protected:
    JSTaggedValue InitializeArrayListConstructor()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();
        auto objCallInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6); // 6 means the value
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(
            0, JSTaggedValue(static_cast<int>(ContainerTag::ArrayList))); // 0 means the argument
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        return result;
    }

    JSHandle<JSAPIArrayList> CreateJSAPIArrayList()
    {
        JSHandle<JSFunction> newTarget(thread, InitializeArrayListConstructor());
        auto objCallInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4); // 4 means the value
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersArrayList::ArrayListConstructor(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPIArrayList> ArrayList(thread, result);
        return ArrayList;
    }

    JSTaggedValue ArrayListAdd(JSHandle<JSAPIArrayList> arrayList, JSTaggedValue value)
    {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6); // 4 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(arrayList.GetTaggedValue());
        callInfo->SetCallArg(0, value);

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersArrayList::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        return result;
    }

    JSTaggedValue ArrayListRemoveByRange(JSHandle<JSAPIArrayList> arrayList, JSTaggedValue startIndex,
                                         JSTaggedValue endIndex)
    {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8); // 6 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(arrayList.GetTaggedValue());
        callInfo->SetCallArg(0, startIndex);
        callInfo->SetCallArg(1, endIndex);

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersArrayList::RemoveByRange(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        return result;
    }

    JSTaggedValue ArrayListSubArrayList(JSHandle<JSAPIArrayList> arrayList, JSTaggedValue startIndex,
                                         JSTaggedValue endIndex)
    {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8); // 8 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(arrayList.GetTaggedValue());
        callInfo->SetCallArg(0, startIndex);
        callInfo->SetCallArg(1, endIndex);
        
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersArrayList::SubArrayList(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        return result;
    }
};

HWTEST_F_L0(ContainersArrayListTest, ArrayListConstructor)
{
    InitializeArrayListConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeArrayListConstructor());
    auto objCallInfo =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);   // 4 means the value
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = ContainersArrayList::ArrayListConstructor(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.IsJSAPIArrayList());
    JSHandle<JSAPIArrayList> arrayList(thread, result);
    JSTaggedValue resultProto = JSObject::GetPrototype(JSHandle<JSObject>::Cast(arrayList));
    JSTaggedValue funcProto = newTarget->GetFunctionPrototype();
    ASSERT_EQ(resultProto, funcProto);
    int length = arrayList->GetLength().GetInt();
    ASSERT_EQ(length, 0);   // 0 means the value
}

HWTEST_F_L0(ContainersArrayListTest, RemoveByRange)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        JSTaggedValue result = ArrayListAdd(arrayList, JSTaggedValue(i));
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(arrayList->GetSize(), static_cast<int>(i + 1));
    }

    // remove success
    {
        JSTaggedValue result = ArrayListRemoveByRange(arrayList, JSTaggedValue(1), JSTaggedValue(3));
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(arrayList->GetSize(), static_cast<int>(NODE_NUMBERS - 2));
        for (uint32_t i = 0; i < NODE_NUMBERS - 2; i++) {
            if (i < 1) {
                EXPECT_EQ(arrayList->Get(thread, i), JSTaggedValue(i));
            } else {
                EXPECT_EQ(arrayList->Get(thread, i), JSTaggedValue(i + 2));
            }
        }
    }

    // input startIndex type error
    {
        JSTaggedValue result = ArrayListRemoveByRange(arrayList, JSTaggedValue::Undefined(), JSTaggedValue(3));
        EXPECT_TRUE(thread->HasPendingException());
        EXPECT_EQ(result, JSTaggedValue::Exception());
        thread->ClearException();
    }

    // input endIndex type error
    {
        JSTaggedValue result = ArrayListRemoveByRange(arrayList, JSTaggedValue(1), JSTaggedValue::Undefined());
        EXPECT_TRUE(thread->HasPendingException());
        EXPECT_EQ(result, JSTaggedValue::Exception());
        thread->ClearException();
    }
}

HWTEST_F_L0(ContainersArrayListTest, SubArrayList)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        JSTaggedValue result = ArrayListAdd(arrayList, JSTaggedValue(i));
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(arrayList->GetSize(), static_cast<int>(i + 1));
    }
    
    // input startIndex type error
    {
        JSTaggedValue result = ArrayListSubArrayList(arrayList, JSTaggedValue::Undefined(), JSTaggedValue(2));
        EXPECT_TRUE(thread->HasPendingException());
        EXPECT_EQ(result, JSTaggedValue::Exception());
        thread->ClearException();
    }

    // input endIndex type error
    {
        JSTaggedValue result = ArrayListSubArrayList(arrayList, JSTaggedValue(2), JSTaggedValue::Undefined());
        EXPECT_TRUE(thread->HasPendingException());
        EXPECT_EQ(result, JSTaggedValue::Exception());
        thread->ClearException();
    }
    
    // success
    {
        JSTaggedValue newArrayList = ArrayListSubArrayList(arrayList, JSTaggedValue(1), JSTaggedValue(3));
        JSHandle<TaggedArray> elements(thread, JSAPIArrayList::Cast(newArrayList.GetTaggedObject())->GetElements());
        EXPECT_EQ(elements->GetLength(), static_cast<uint32_t>(2)); // length = 3 - 1
        EXPECT_EQ(elements->Get(thread, 0), JSTaggedValue(1));
        EXPECT_EQ(elements->Get(thread, 1), JSTaggedValue(2));
    }
}
}  // namespace panda::test