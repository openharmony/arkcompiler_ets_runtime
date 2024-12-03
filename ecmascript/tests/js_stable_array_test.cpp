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

#include "ecmascript/element_accessor.h"
#include "ecmascript/element_accessor-inl.h"
#include "ecmascript/js_stable_array.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;
constexpr uint32_t ARRAY_LENGTH_4 = 4;
constexpr int32_t INT_VALUE_0 = 0;
constexpr int32_t INT_VALUE_1 = 1;
constexpr int32_t INT_VALUE_2 = 2;
constexpr int32_t INT_VALUE_3 = 3;
constexpr int32_t INT_VALUE_666 = 666;

enum class StableArrayIndex {
    STABLE_ARRAY_INDEX_0,
    STABLE_ARRAY_INDEX_1,
    STABLE_ARRAY_INDEX_2,
    STABLE_ARRAY_INDEX_3
};

namespace panda::test {
class JSStableArrayTest : public BaseTestWithScope<false> {
public:
    JSHandle<JSTaggedValue> CallJoin(JSHandle<TaggedArray> handleTagArr, JSTaggedValue sepValue) const
    {
        JSHandle<JSArray> handleArr(JSArray::CreateArrayFromList(thread, handleTagArr));
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetCallArg(0, sepValue);
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSHandle<JSTaggedValue> handleTagValEcmaStrRet(thread, JSStableArray::Join(handleArr, ecmaRuntimeCallInfo));
        TestHelper::TearDownFrame(thread, prev);
        return handleTagValEcmaStrRet;
    }

    // tests for sep is Undefined
    JSHandle<JSTaggedValue> CallJoin(JSHandle<TaggedArray> handleTagArr, int64_t lengthArr) const
    {
        JSTaggedValue sepValue = JSTaggedValue::Undefined();
        return CallJoin(handleTagArr, sepValue);
    }

    JSHandle<JSTaggedValue> CallJoin(JSHandle<TaggedArray> handleTagArr, std::string& sep, int64_t lengthArr) const
    {
        ObjectFactory* objFactory = thread->GetEcmaVM()->GetFactory();
        JSTaggedValue sepValue = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromStdString(sep)).GetTaggedValue();
        return CallJoin(handleTagArr, sepValue);
    }
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

    JSHandle<JSObject> arrObjHandle(handleArr);
    EXPECT_EQ(handleArr->GetArrayLength(), static_cast<size_t>(lengthArr + numElementsPush));
    for (int32_t i = lengthArr; i < lengthArr + numElementsPush; i++) {
        EXPECT_EQ(ElementAccessor::Get(arrObjHandle, i).GetNumber(), i - lengthArr);
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
    JSHandle<JSObject> thisObjHandle(handleArr);
    JSTaggedValue newArray = JSArray::ArraySpeciesCreate(thread, thisObjHandle,
                                                         JSTaggedNumber(static_cast<double>(actualDeleteCount)));
    JSHandle<JSObject> newArrayHandle(thread, newArray);
    uint32_t len = JSHandle<JSArray>::Cast(thisObjHandle)->GetArrayLength();
    JSHandle<JSTaggedValue> handleTagValArrCombinedOfDeletedElements(thread,
        JSStableArray::Splice(JSHandle<JSArray>::Cast(thisObjHandle), ecmaRuntimeCallInfo, offsetStartInsert,
            countInsert, actualDeleteCount, newArrayHandle, len));
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<JSArray> handleArrCombinedOfDeletedElements(handleTagValArrCombinedOfDeletedElements);
    EXPECT_EQ(handleArrCombinedOfDeletedElements->GetArrayLength(), actualDeleteCount);
    JSHandle<JSObject> handleObjArrCombinedOfDeletedElements(handleTagValArrCombinedOfDeletedElements);
    for (int32_t i = 0; i < actualDeleteCount; i++) {
        EXPECT_EQ(ElementAccessor::Get(handleObjArrCombinedOfDeletedElements, i).GetNumber(),
                  (offsetStartInsert + i) * 10);
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
    std::vector<JSTaggedValue> args{};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, args, 4);
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> handleTagValEcmaStrRet(thread,
        JSStableArray::Join(handleArr, ecmaRuntimeCallInfo));
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> handleEcmaStrRet(handleTagValEcmaStrRet);
    EXPECT_STREQ(EcmaStringAccessor(handleEcmaStrRet).ToCString().c_str(), "0,1,2,3,4,5,6,7,8,9");
    EXPECT_FALSE(EcmaStringAccessor(handleEcmaStrRet).IsTreeString());
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
    std::vector<JSTaggedValue> args{};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, args, 4);
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> handleTagValEcmaStrRet(thread,
        JSStableArray::Join(handleArr, ecmaRuntimeCallInfo));
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> handleEcmaStrRet(handleTagValEcmaStrRet);
    EXPECT_STREQ(EcmaStringAccessor(handleEcmaStrRet).ToCString().c_str(), "abc,abc,abc,abc,abc,abc,abc,abc,abc,abc");
    EXPECT_FALSE(EcmaStringAccessor(handleEcmaStrRet).IsTreeString());
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
    std::vector<JSTaggedValue> args{JSHandle<JSTaggedValue>::Cast(objFactory->NewFromStdString("^")).GetTaggedValue()};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, args, 6);
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> handleTagValEcmaStrRet(thread,
        JSStableArray::Join(handleArr, ecmaRuntimeCallInfo));
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> handleEcmaStrRet(handleTagValEcmaStrRet);
    EXPECT_STREQ(EcmaStringAccessor(handleEcmaStrRet).ToCString().c_str(), "0^1^2^3^4^5^6^7^8^9");
    EXPECT_FALSE(EcmaStringAccessor(handleEcmaStrRet).IsTreeString());
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
    EXPECT_FALSE(EcmaStringAccessor(handleEcmaStrRet).IsTreeString());
}

/**
 * @tc.name: Join_StringElements_ManyTiny
 * @tc.desc: Create a source Array whose elements are EcmaStrings and an EcmaRuntimeCallInfo, define the first arg of
             the EcmaRuntimeCallInfo an EcmaString as the seperator, check whether the EcmaString returned through
             calling Join function with the source Array and the EcmaRuntimeCallInfo is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSStableArrayTest, Join_StringElements_ManyTiny)
{
    int32_t lengthArr = 256;
    std::string sep = "";
    // tiny string join should not use tree string.
    ObjectFactory* objFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> handleTagArr(objFactory->NewTaggedArray(lengthArr));
    JSHandle<JSTaggedValue> handleTagValElementEcmaStr(objFactory->NewFromStdString("a"));
    for (int i = 0; i < lengthArr; i++) {
        handleTagArr->Set(thread, i, handleTagValElementEcmaStr.GetTaggedValue());
    }
    JSHandle<JSTaggedValue> handleTagValEcmaStrRet = CallJoin(handleTagArr, sep, lengthArr);
    JSHandle<EcmaString> handleEcmaStrRet(handleTagValEcmaStrRet);
    // 256 x a
    EXPECT_STREQ(EcmaStringAccessor(handleEcmaStrRet).ToCString().c_str(),
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    EXPECT_FALSE(EcmaStringAccessor(handleEcmaStrRet).IsTreeString());
    sep = ",";
    handleTagValEcmaStrRet = CallJoin(handleTagArr, sep, lengthArr);
    JSHandle<EcmaString> handleEcmaStrRet2(handleTagValEcmaStrRet);
    EXPECT_STREQ(EcmaStringAccessor(handleEcmaStrRet2).ToCString().c_str(),
                 "a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,"
                 "a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,"
                 "a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,"
                 "a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,"
                 "a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,"
                 "a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,"
                 "a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,"
                 "a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a");
    EXPECT_FALSE(EcmaStringAccessor(handleEcmaStrRet2).IsTreeString());
}

/**
 * @tc.name: Join_StringElements_ManyTiny
 * @tc.desc: Create a source Array whose elements are EcmaStrings and an EcmaRuntimeCallInfo, define the first arg of
             the EcmaRuntimeCallInfo an EcmaString as the seperator, check whether the EcmaString returned through
             calling Join function with the source Array and the EcmaRuntimeCallInfo is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSStableArrayTest, Join_StringElements_LargeString)
{
    int32_t lengthArr = 8;
    std::string sep = "";
    // large string should use tree string.
    ObjectFactory* objFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> handleTagArr(objFactory->NewTaggedArray(lengthArr));
    // 32 x a
    JSHandle<JSTaggedValue>
        handleTagValElementEcmaStr(objFactory->NewFromStdString("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    for (int i = 0; i < lengthArr; i++) {
        handleTagArr->Set(thread, i, handleTagValElementEcmaStr.GetTaggedValue());
    }
    JSHandle<JSTaggedValue> handleTagValEcmaStrRet = CallJoin(handleTagArr, sep, lengthArr);
    JSHandle<EcmaString> handleEcmaStrRet(handleTagValEcmaStrRet);
    EXPECT_STREQ(EcmaStringAccessor(handleEcmaStrRet).ToCString().c_str(),
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    EXPECT_TRUE(EcmaStringAccessor(handleEcmaStrRet).IsTreeString());
}

/**
* @tc.name: Join_StringElements_ManyTiny
* @tc.desc: Create a source Array whose elements are EcmaStrings and an EcmaRuntimeCallInfo, define the first arg of
         the EcmaRuntimeCallInfo an EcmaString as the seperator, check whether the EcmaString returned through
         calling Join function with the source Array and the EcmaRuntimeCallInfo is within expectations.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F_L0(JSStableArrayTest, Join_StringElements_LargeString2)
{
    int32_t lengthArr = 4;
    std::string sep = ",";
    // large string should use tree string.
    ObjectFactory* objFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> handleTagArr(objFactory->NewTaggedArray(lengthArr));
    // 64 x a
    JSHandle<JSTaggedValue> handleTagValElementEcmaStr(
        objFactory->NewFromStdString("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    for (int i = 0; i < lengthArr; i++) {
        handleTagArr->Set(thread, i, handleTagValElementEcmaStr.GetTaggedValue());
    }
    JSHandle<JSTaggedValue> handleTagValEcmaStrRet = CallJoin(handleTagArr, sep, lengthArr);

    JSHandle<EcmaString> handleEcmaStrRet(handleTagValEcmaStrRet);
    EXPECT_STREQ(EcmaStringAccessor(handleEcmaStrRet).ToCString().c_str(),
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    EXPECT_TRUE(EcmaStringAccessor(handleEcmaStrRet).IsTreeString());
}

/**
* @tc.name: Join_StringElements_ManyTiny
* @tc.desc: Create a source Array whose elements are EcmaStrings and an EcmaRuntimeCallInfo, define the first arg of
         the EcmaRuntimeCallInfo an EcmaString as the seperator, check whether the EcmaString returned through
         calling Join function with the source Array and the EcmaRuntimeCallInfo is within expectations.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F_L0(JSStableArrayTest, Join_StringElements_LargeString3)
{
    int32_t lengthArr = 5;
    std::string sep = ",";
    // large string should use tree string.
    ObjectFactory* objFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> handleTagArr(objFactory->NewTaggedArray(lengthArr));
    // 64 x a
    JSHandle<JSTaggedValue> handleTagValElementEcmaStr0(
        objFactory->NewFromStdString("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    handleTagArr->Set(thread, 0, handleTagValElementEcmaStr0.GetTaggedValue());
    JSHandle<JSTaggedValue> handleTagValElementEcmaStr1(
        objFactory->NewFromStdString("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"));
    handleTagArr->Set(thread, 1, handleTagValElementEcmaStr1.GetTaggedValue());
    JSHandle<JSTaggedValue> handleTagValElementEcmaStr2(
        objFactory->NewFromStdString("cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"));
    handleTagArr->Set(thread, 2, handleTagValElementEcmaStr2.GetTaggedValue());
    JSHandle<JSTaggedValue> handleTagValElementEcmaStr3(
        objFactory->NewFromStdString("dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"));
    handleTagArr->Set(thread, 3, handleTagValElementEcmaStr3.GetTaggedValue());
    JSHandle<JSTaggedValue> handleTagValElementEcmaStr4(
        objFactory->NewFromStdString("eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"));
    handleTagArr->Set(thread, 4, handleTagValElementEcmaStr4.GetTaggedValue());

    JSHandle<JSTaggedValue> handleTagValEcmaStrRet = CallJoin(handleTagArr, sep, lengthArr);

    JSHandle<EcmaString> handleEcmaStrRet(handleTagValEcmaStrRet);
    EXPECT_STREQ(EcmaStringAccessor(handleEcmaStrRet).ToCString().c_str(),
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,"
                 "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb,"
                 "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc,"
                 "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd,"
                 "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
    EXPECT_TRUE(EcmaStringAccessor(handleEcmaStrRet).IsTreeString());
}

/**
 * @tc.name: Join_StringElements_Stack
 * @tc.desc: Use stack to store the preprocessed elements of the source Array.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSStableArrayTest, Join_StringElements_Stack)
{
    int32_t lengthArr = 32;
    // tiny string join should not use tree string.
    ObjectFactory *objFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> handleTagArr(objFactory->NewTaggedArray(lengthArr));
    JSHandle<JSTaggedValue> handleTagValElementEcmaStr(objFactory->NewFromStdString("a"));
    for (int i = 0; i < lengthArr; i++) {
        handleTagArr->Set(thread, i, handleTagValElementEcmaStr.GetTaggedValue());
    }
    // sep is Undefined
    JSHandle<JSTaggedValue> handleTagValEcmaStrRet = CallJoin(handleTagArr, lengthArr);
    JSHandle<EcmaString> handleEcmaStrRet(handleTagValEcmaStrRet);
    // 32 x a
    EXPECT_STREQ(EcmaStringAccessor(handleEcmaStrRet).ToCString().c_str(),
                "a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a");
    EXPECT_FALSE(EcmaStringAccessor(handleEcmaStrRet).IsTreeString());
}

/**
 * @tc.name: Join_StringElements_Stack1
 * @tc.desc: Use stack to store the preprocessed elements of the source Array.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSStableArrayTest, Join_StringElements_Stack1)
{
    int32_t lengthArr = 4;
    // large string should use tree string.
    ObjectFactory *objFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> handleTagArr(objFactory->NewTaggedArray(lengthArr));
    // 64 x a
    JSHandle<JSTaggedValue> handleTagValElementEcmaStr(
        objFactory->NewFromStdString("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    for (int i = 0; i < lengthArr; i++) {
        handleTagArr->Set(thread, i, handleTagValElementEcmaStr.GetTaggedValue());
    }
    // sep is Undefined
    JSHandle<JSTaggedValue> handleTagValEcmaStrRet = CallJoin(handleTagArr, lengthArr);
    JSHandle<EcmaString> handleEcmaStrRet(handleTagValEcmaStrRet);
    EXPECT_STREQ(EcmaStringAccessor(handleEcmaStrRet).ToCString().c_str(),
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    EXPECT_TRUE(EcmaStringAccessor(handleEcmaStrRet).IsTreeString());
}

/**
 * @tc.name: Join_StringElements_Vector
 * @tc.desc: Use vector to store the preprocessed elements of the source Array.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSStableArrayTest, Join_StringElements_Vector)
{
    int32_t lengthArr = 128;
    // tiny string join should not use tree string.
    ObjectFactory *objFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> handleTagArr(objFactory->NewTaggedArray(lengthArr));
    JSHandle<JSTaggedValue> handleTagValElementEcmaStr(objFactory->NewFromStdString("a"));
    for (int i = 0; i < lengthArr; i++) {
        handleTagArr->Set(thread, i, handleTagValElementEcmaStr.GetTaggedValue());
    }
    // sep is Undefined
    JSHandle<JSTaggedValue> handleTagValEcmaStrRet = CallJoin(handleTagArr, lengthArr);
    JSHandle<EcmaString> handleEcmaStrRet(handleTagValEcmaStrRet);
    // 128 x a
    EXPECT_STREQ(EcmaStringAccessor(handleEcmaStrRet).ToCString().c_str(),
                 "a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,"
                 "a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,"
                 "a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,"
                 "a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a");
    EXPECT_FALSE(EcmaStringAccessor(handleEcmaStrRet).IsTreeString());
}

/**
 * @tc.name: Join_StringElements_Vector1
 * @tc.desc: Use vector to store the preprocessed elements of the source Array.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSStableArrayTest, Join_StringElements_Vector1)
{
    int32_t lengthArr = 65;
    std::string sep = "";
    // large string should use tree string.
    ObjectFactory *objFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> handleTagArr(objFactory->NewTaggedArray(lengthArr));
    // 40 x a
    JSHandle<JSTaggedValue> handleTagValElementEcmaStr(
        objFactory->NewFromStdString("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    for (int i = 0; i < lengthArr; i++) {
        handleTagArr->Set(thread, i, handleTagValElementEcmaStr.GetTaggedValue());
    }
    JSHandle<JSTaggedValue> handleTagValEcmaStrRet = CallJoin(handleTagArr, sep, lengthArr);
    JSHandle<EcmaString> handleEcmaStrRet(handleTagValEcmaStrRet);
    EXPECT_STREQ(EcmaStringAccessor(handleEcmaStrRet).ToCString().c_str(),
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    EXPECT_TRUE(EcmaStringAccessor(handleEcmaStrRet).IsTreeString());
}

/**
 * @tc.name: At
 * @tc.desc: Create a source Array whose elements are Numbers and an EcmaRuntimeCallInfo, define the first arg of the
             EcmaRuntimeCallInfo an number as the index, check whether the element returned through calling
             At function with the source Array and the EcmaRuntimeCallInfo is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSStableArrayTest, At_NUMBER_INDEX)
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
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(0));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);

    JSTaggedValue thisTagValue = JSStableArray::At(handleArr, ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(thisTagValue.GetNumber(), 0);

    ecmaRuntimeCallInfo  = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(9));
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);

    thisTagValue = JSStableArray::At(handleArr, ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(thisTagValue.GetNumber(), 9);

    ecmaRuntimeCallInfo  = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-1));
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);

    thisTagValue = JSStableArray::At(handleArr, ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(thisTagValue.GetNumber(), 9);

    ecmaRuntimeCallInfo  = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(10));
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);

    thisTagValue = JSStableArray::At(handleArr, ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(thisTagValue, JSTaggedValue::Undefined());
}

/**
 * @tc.name: With
 * @tc.desc: Create a source Array whose elements are Numbers, define the first arg a thread,
 *           define the second arg as the source Array, define the third arg an number as the length of source Array
 *           define the forth arg an number as the index, define the fifth args an number as the value
 *           check whether the value returned through calling With function with the source Array
 *           and the args is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSStableArrayTest, With)
{
    ObjectFactory *objFactory = thread->GetEcmaVM()->GetFactory();
    int32_t lengthArr = ARRAY_LENGTH_4;
    JSHandle<TaggedArray> handleTagArr(objFactory->NewTaggedArray(lengthArr));
    for (int i = 0; i < lengthArr; i++) {
        handleTagArr->Set(thread, i, JSTaggedValue(i));
    }
    JSHandle<JSArray> handleArr(JSArray::CreateArrayFromList(thread, handleTagArr));

    int64_t arrayLength = ARRAY_LENGTH_4;
    int64_t index = static_cast<int64_t>(StableArrayIndex::STABLE_ARRAY_INDEX_2);
    JSTaggedValue resultArr = JSStableArray::With(thread, handleArr,
                                                  arrayLength, index,
                                                  JSHandle<JSTaggedValue>(thread, JSTaggedValue(INT_VALUE_666)));
    JSHandle<JSTaggedValue> destTaggedValue(thread, resultArr);
    JSHandle<JSArray> destArr(destTaggedValue);
    JSHandle<TaggedArray> destTaggedArr(thread, TaggedArray::Cast(destArr->GetElements().GetTaggedObject()));
    for (uint32_t i = 0; i < ARRAY_LENGTH_4; ++i) {
        JSHandle<JSObject> arrObjHandle(handleArr);
        EXPECT_EQ(ElementAccessor::Get(arrObjHandle, i).GetNumber(), i);
    }
    for (uint32_t i = 0; i < ARRAY_LENGTH_4; ++i) {
        JSHandle<JSObject> arrObjHandle(destArr);
        if (i == 2) {
            EXPECT_EQ(ElementAccessor::Get(arrObjHandle, i).GetNumber(), INT_VALUE_666);
        } else {
            EXPECT_EQ(ElementAccessor::Get(arrObjHandle, i).GetNumber(), i);
        }
    }
}

/**
 * @tc.name: ToReversed
 * @tc.desc: Create a source Array whose elements are Numbers and an EcmaRuntimeCallInfo, check whether the
             value returned through calling ToReversed function with the source Array is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSStableArrayTest, ToReversed)
{
    ObjectFactory *objFactory = thread->GetEcmaVM()->GetFactory();
    int32_t lengthArr = ARRAY_LENGTH_4;
    JSHandle<TaggedArray> handleTagArr(objFactory->NewTaggedArray(lengthArr));
    for (int i = 0; i < lengthArr; i++) {
        handleTagArr->Set(thread, i, JSTaggedValue(i));
    }
    JSHandle<JSArray> handleArr(JSArray::CreateArrayFromList(thread, handleTagArr));
    JSHandle<JSObject> handleArrObj(handleArr);
    JSTaggedValue resultArr =
        JSStableArray::ToReversed(thread, handleArr, ARRAY_LENGTH_4);
    JSHandle<JSObject> dstArrObj(thread, resultArr);

    EXPECT_EQ(ElementAccessor::Get(handleArrObj,
        static_cast<uint32_t>(StableArrayIndex::STABLE_ARRAY_INDEX_0)).GetNumber(), INT_VALUE_0);
    EXPECT_EQ(ElementAccessor::Get(handleArrObj,
        static_cast<uint32_t>(StableArrayIndex::STABLE_ARRAY_INDEX_1)).GetNumber(), INT_VALUE_1);
    EXPECT_EQ(ElementAccessor::Get(handleArrObj,
        static_cast<uint32_t>(StableArrayIndex::STABLE_ARRAY_INDEX_2)).GetNumber(), INT_VALUE_2);
    EXPECT_EQ(ElementAccessor::Get(handleArrObj,
        static_cast<uint32_t>(StableArrayIndex::STABLE_ARRAY_INDEX_3)).GetNumber(), INT_VALUE_3);

    EXPECT_EQ(ElementAccessor::Get(dstArrObj,
        static_cast<uint32_t>(StableArrayIndex::STABLE_ARRAY_INDEX_0)).GetNumber(), INT_VALUE_3);
    EXPECT_EQ(ElementAccessor::Get(dstArrObj,
        static_cast<uint32_t>(StableArrayIndex::STABLE_ARRAY_INDEX_1)).GetNumber(), INT_VALUE_2);
    EXPECT_EQ(ElementAccessor::Get(dstArrObj,
        static_cast<uint32_t>(StableArrayIndex::STABLE_ARRAY_INDEX_2)).GetNumber(), INT_VALUE_1);
    EXPECT_EQ(ElementAccessor::Get(dstArrObj,
        static_cast<uint32_t>(StableArrayIndex::STABLE_ARRAY_INDEX_3)).GetNumber(), INT_VALUE_0);
}
}  // namespace panda::test
