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

#include "ecmascript/js_stable_array.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSStableArrayTest : public testing::Test {
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
};

/**
 * @tc.name: Push
 * @tc.desc: Change a JSArray through calling Push function with the JSArray and a EcmaRuntimeCallInfo, check whether
 *           the TaggedArray of the JSArray is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSStableArrayTest, Push)
{
    int32_t lengthArr = 99;
    int32_t numElementsPush = 9;
    JSHandle<JSTaggedValue> handleTagValArr = JSArray::ArrayCreate(thread, JSTaggedNumber(lengthArr));
    JSHandle<JSArray> handleArr(handleTagValArr);

    auto ecmaRuntimeCallInfo =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4 + 2 * numElementsPush);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    for (int32_t i = 0; i < numElementsPush; i++) {
        ecmaRuntimeCallInfo->SetCallArg(i, JSTaggedValue(i));
    }
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    EXPECT_EQ(JSStableArray::Push(handleArr, ecmaRuntimeCallInfo),
              JSTaggedValue(lengthArr + numElementsPush));
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<TaggedArray> handleTagArr(thread, TaggedArray::Cast(handleArr->GetElements().GetTaggedObject()));
    EXPECT_EQ(handleArr->GetArrayLength(), static_cast<size_t>(lengthArr + numElementsPush));
    for (int32_t i = lengthArr; i < lengthArr + numElementsPush; i++) {
        EXPECT_EQ(handleTagArr->Get(i).GetNumber(), i - lengthArr);
    }
}

/**
 * @tc.name: Pop
 * @tc.desc: Change a JSArray through calling Pop function with the JSArray and a EcmaRuntimeCallInfo, check whether
 *           the JSArray and the TaggedArray of the JSArray are within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSStableArrayTest, Pop)
{
    ObjectFactory *objFactory = thread->GetEcmaVM()->GetFactory();

    int32_t lengthArr = 49;
    JSHandle<TaggedArray> handleTagArr(objFactory->NewTaggedArray(lengthArr));
    for (int i = 0; i < lengthArr; i++) {
        handleTagArr->Set(thread, i, JSTaggedValue(i));
    }
    JSHandle<JSArray> handleArr(JSArray::CreateArrayFromList(thread, handleTagArr));

    for (int32_t i = 1; i < 6; i++) {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        EXPECT_EQ(JSStableArray::Pop(handleArr, ecmaRuntimeCallInfo), JSTaggedValue(lengthArr - i));
        TestHelper::TearDownFrame(thread, prev);

        EXPECT_EQ(handleArr->GetArrayLength(), static_cast<uint32_t>(lengthArr - i));
        if (i != 5) {
            EXPECT_EQ(handleTagArr->GetLength(), static_cast<uint32_t>(lengthArr));
            EXPECT_EQ(handleTagArr->Get(lengthArr - i), JSTaggedValue::Hole());
        } else {
            EXPECT_EQ(handleTagArr->GetLength(), static_cast<uint32_t>(lengthArr - i));
        }
    }
}

/**
 * @tc.name: Splice
 * @tc.desc: Create a source TaggedArray, set value for the elements of the source TaggedArray, create an source Array
 *           through calling CreateArrayFromList function with the source TaggedArray, create a deleted Array through
 *           calling Splice function with the source Array, an EcmaRuntimeCallInfo that set Args from 2 as the
 *           delete-elements, the offsetStartInsert, the countInsert and the actualDeleteCount. Check whether the
 *           deleted Array and the source Array after change are within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSStableArrayTest, Splice)
{
    ObjectFactory *objFactory = thread->GetEcmaVM()->GetFactory();

    int32_t lengthArr = 49;

    JSHandle<JSTaggedValue> handleTagValInsertElement1(thread, JSTaggedValue(4000));
    JSHandle<JSTaggedValue> handleTagValInsertElement2(thread, JSTaggedValue(4100));
    JSHandle<TaggedArray> handleTagArr(objFactory->NewTaggedArray(lengthArr));
    for (int i = 0; i < lengthArr; i++) {
        handleTagArr->Set(thread, i, JSTaggedValue(i * 10));
    }
    JSHandle<JSArray> handleArr(JSArray::CreateArrayFromList(thread, handleTagArr));
    double offsetStartInsert = 40;
    double actualDeleteCount = 3;
    double countInsert = 2;

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
        4 + (2 + countInsert) * 2);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(2, handleTagValInsertElement1.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(3, handleTagValInsertElement2.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> handleTagValArrCombinedOfDeletedElements(thread,
        JSStableArray::Splice(handleArr, ecmaRuntimeCallInfo, offsetStartInsert, countInsert,
            actualDeleteCount));
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<JSArray> handleArrCombinedOfDeletedElements(handleTagValArrCombinedOfDeletedElements);
    EXPECT_EQ(handleArrCombinedOfDeletedElements->GetArrayLength(), actualDeleteCount);
    JSHandle<JSObject> handleObjArrCombinedOfDeletedElements(handleTagValArrCombinedOfDeletedElements);
    JSHandle<JSTaggedValue> handleTagValTagArrCombinedOfDeletedElements(thread,
        handleObjArrCombinedOfDeletedElements->GetElements());
    JSHandle<TaggedArray> handleTagArrCombinedOfDeletedElements(handleTagValTagArrCombinedOfDeletedElements);
    for (int32_t i = 0; i < actualDeleteCount; i++) {
        EXPECT_EQ(handleTagArrCombinedOfDeletedElements->Get(i).GetNumber(), (offsetStartInsert + i) * 10);
    }

    // Check the JSArray(in-out-parameter) changed through calling the Splice function.
    EXPECT_EQ(handleArr->GetArrayLength(), lengthArr - actualDeleteCount + countInsert);
    for (int32_t i = 0; i < offsetStartInsert; i++) {
        EXPECT_EQ(handleTagArr->Get(i).GetNumber(), i * 10);
    }
    EXPECT_EQ(handleTagArr->Get(offsetStartInsert).GetNumber(),
              handleTagValInsertElement1.GetTaggedValue().GetNumber());
    EXPECT_EQ(handleTagArr->Get(offsetStartInsert + 1).GetNumber(),
              handleTagValInsertElement2.GetTaggedValue().GetNumber());
    for (int32_t i = offsetStartInsert + countInsert; i < lengthArr - actualDeleteCount + countInsert; i++) {
        EXPECT_EQ(handleTagArr->Get(i).GetNumber(), (i + actualDeleteCount - countInsert) * 10);
    }
}

/**
 * @tc.name: Shift
 * @tc.desc: Create a source Array, set value for the elements of the source Array, call the Shift function with the
 *           source Array 5 times, check whether the returned JSTaggedValue and the changed source Array are within
 *           expectations after each call to the Shift function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSStableArrayTest, Shift)
{
    ObjectFactory *objFactory = thread->GetEcmaVM()->GetFactory();

    int32_t lengthArr = 49;
    JSHandle<TaggedArray> handleTagArr(objFactory->NewTaggedArray(lengthArr));
    for (int i = 0; i < lengthArr; i++) {
        handleTagArr->Set(thread, i, JSTaggedValue(i * 10));
    }
    JSHandle<JSArray> handleArr(JSArray::CreateArrayFromList(thread, handleTagArr));

    for (int32_t i = 0; i < 5; i++) {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        EXPECT_EQ(JSStableArray::Shift(handleArr, ecmaRuntimeCallInfo), JSTaggedValue(i * 10));
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(handleArr->GetArrayLength(), static_cast<uint32_t>(lengthArr - (i + 1)));
        EXPECT_EQ(handleTagArr->Get(0), JSTaggedValue((i + 1) * 10));
        if (i != 4) {
            EXPECT_EQ(handleTagArr->GetLength(), static_cast<uint32_t>(lengthArr));
            EXPECT_EQ(handleTagArr->Get(lengthArr - (i + 1)), JSTaggedValue::Hole());
            continue;
        }
        EXPECT_EQ(handleTagArr->GetLength(), static_cast<uint32_t>(lengthArr - (i + 1)));
    }
}

/**
 * @tc.name: Join_NumberElements_UndefinedSep
 * @tc.desc: Create a source Array whose elements are Numbers and an EcmaRuntimeCallInfo, check whether the EcmaString
 *           returned through calling Join function with the source Array and the EcmaRuntimeCallInfo is within
 *           expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSStableArrayTest, Join_NumberElements_UndefinedSep)
{
    ObjectFactory *objFactory = thread->GetEcmaVM()->GetFactory();

    int32_t lengthArr = 10;
    JSHandle<TaggedArray> handleTagArr(objFactory->NewTaggedArray(lengthArr));
    for (int i = 0; i < lengthArr; i++) {
        handleTagArr->Set(thread, i, JSTaggedValue(i));
    }
    JSHandle<JSArray> handleArr(JSArray::CreateArrayFromList(thread, handleTagArr));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> handleTagValEcmaStrRet(thread,
        JSStableArray::Join(handleArr, ecmaRuntimeCallInfo));
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> handleEcmaStrRet(handleTagValEcmaStrRet);
    EXPECT_STREQ(EcmaStringAccessor(handleEcmaStrRet).ToCString().c_str(), "0,1,2,3,4,5,6,7,8,9");
}

/**
 * @tc.name: Join_StringElements_UndefinedSep
 * @tc.desc: Create a source Array whose elements are EcmaStrings and an EcmaRuntimeCallInfo, check whether the
 *           EcmaString returned through calling Join function with the source Array and the EcmaRuntimeCallInfo is
 *           within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSStableArrayTest, Join_StringElements_UndefinedSep)
{
    ObjectFactory *objFactory = thread->GetEcmaVM()->GetFactory();

    int32_t lengthArr = 10;
    JSHandle<TaggedArray> handleTagArr(objFactory->NewTaggedArray(lengthArr));
    JSHandle<JSTaggedValue> handleTagValElementEcmaStr(objFactory->NewFromStdString("abc"));
    for (int i = 0; i < lengthArr; i++) {
        handleTagArr->Set(thread, i, handleTagValElementEcmaStr.GetTaggedValue());
    }
    JSHandle<JSArray> handleArr(JSArray::CreateArrayFromList(thread, handleTagArr));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> handleTagValEcmaStrRet(thread,
        JSStableArray::Join(handleArr, ecmaRuntimeCallInfo));
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> handleEcmaStrRet(handleTagValEcmaStrRet);
    EXPECT_STREQ(EcmaStringAccessor(handleEcmaStrRet).ToCString().c_str(), "abc,abc,abc,abc,abc,abc,abc,abc,abc,abc");
}

/**
 * @tc.name: Join_NumberElements_DefinedSep
 * @tc.desc: Create a source Array whose elements are Numbers and an EcmaRuntimeCallInfo, define the first arg of the
             EcmaRuntimeCallInfo an EcmaString as the seperator, check whether the EcmaString returned through calling
             Join function with the source Array and the EcmaRuntimeCallInfo is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSStableArrayTest, Join_NumberElements_DefinedSep)
{
    ObjectFactory *objFactory = thread->GetEcmaVM()->GetFactory();

    int32_t lengthArr = 10;
    JSHandle<TaggedArray> handleTagArr(objFactory->NewTaggedArray(lengthArr));
    for (int i = 0; i < lengthArr; i++) {
        handleTagArr->Set(thread, i, JSTaggedValue(i));
    }
    JSHandle<JSArray> handleArr(JSArray::CreateArrayFromList(thread, handleTagArr));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0,
        JSHandle<JSTaggedValue>::Cast(objFactory->NewFromStdString("^")).GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> handleTagValEcmaStrRet(thread,
        JSStableArray::Join(handleArr, ecmaRuntimeCallInfo));
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> handleEcmaStrRet(handleTagValEcmaStrRet);
    EXPECT_STREQ(EcmaStringAccessor(handleEcmaStrRet).ToCString().c_str(), "0^1^2^3^4^5^6^7^8^9");
}

/**
 * @tc.name: Join_StringElements_DefinedSep
 * @tc.desc: Create a source Array whose elements are EcmaStrings and an EcmaRuntimeCallInfo, define the first arg of
             the EcmaRuntimeCallInfo an EcmaString as the seperator, check whether the EcmaString returned through
             calling Join function with the source Array and the EcmaRuntimeCallInfo is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSStableArrayTest, Join_StringElements_DefinedSep)
{
    ObjectFactory *objFactory = thread->GetEcmaVM()->GetFactory();

    int32_t lengthArr = 10;
    JSHandle<TaggedArray> handleTagArr(objFactory->NewTaggedArray(lengthArr));
    JSHandle<JSTaggedValue> handleTagValElementEcmaStr(objFactory->NewFromStdString("a"));
    for (int i = 0; i < lengthArr; i++) {
        handleTagArr->Set(thread, i, handleTagValElementEcmaStr.GetTaggedValue());
    }
    JSHandle<JSArray> handleArr(JSArray::CreateArrayFromList(thread, handleTagArr));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0,
        JSHandle<JSTaggedValue>::Cast(objFactory->NewFromStdString(" <> ")).GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> handleTagValEcmaStrRet(thread,
        JSStableArray::Join(handleArr, ecmaRuntimeCallInfo));
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> handleEcmaStrRet(handleTagValEcmaStrRet);
    EXPECT_STREQ(EcmaStringAccessor(handleEcmaStrRet).ToCString().c_str(),
        "a <> a <> a <> a <> a <> a <> a <> a <> a <> a");
}
}  // namespace panda::test