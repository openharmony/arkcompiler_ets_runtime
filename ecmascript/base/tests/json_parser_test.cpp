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

#include "ecmascript/base/json_parser.h"
#include "ecmascript/base/json_helper.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class JsonParserTest : public BaseTestWithScope<false> {
public:
    using TransformType = base::JsonHelper::TransformType;

    void CheckUnsupportedSendableJson(JSThread *thread, JSHandle<JSTaggedValue> &result) const
    {
        EXPECT_TRUE(result->IsException());
        JSHandle<JSTaggedValue> exceptionObj(thread, thread->GetException());
        auto messageValue =
            JSTaggedValue::GetProperty(thread, exceptionObj, thread->GlobalConstants()->GetHandledMessageString())
                .GetValue();
        EXPECT_EQ(ConvertToString(thread, EcmaString::Cast(messageValue->GetTaggedObject())),
                  MessageString::GetMessageString(GET_MESSAGE_STRING_ID(SendableArrayForJson)).c_str());
    }

    bool CheckSendableConstraint(const JSThread *thread, JSTaggedValue value) const
    {
        if (!value.IsHeapObject()) {
            // tagged value always follow sendable constraint.
            return true;
        }
        TaggedObject *obj = value.IsWeak() ? value.GetTaggedWeakRef() : value.GetTaggedObject();
        auto *jsHClass = obj->GetClass();
        if (!jsHClass->IsJSShared()) {
            return false;
        }
        if (jsHClass->IsExtensible()) {
            LOG_ECMA(ERROR) << "sendable check failed. obj is extensible";
            value.D(thread);
            return false;
        }
        if (!CheckSendableProps(thread, value, obj)) {
            return false;
        }
        // trace proto chain
        auto proto = jsHClass->GetPrototype(thread);
        if (!proto.IsNull() && !proto.IsJSShared()) {
            LOG_ECMA(ERROR) << "sendable check failed. proto is not sendable.";
            value.D(thread);
            return false;
        }
        return true;
    }

    bool CheckSendableProps(const JSThread *thread, JSTaggedValue value, TaggedObject *obj) const
    {
        auto *jsHClass = obj->GetClass();
        auto layoutValue = jsHClass->GetLayout(thread);
        if (layoutValue.IsNull()) {
            return true;
        }
        auto *layoutInfo = LayoutInfo::Cast(layoutValue.GetTaggedObject());
        auto *jsObj = JSObject::Cast(obj);
        auto *propsValue = TaggedArray::Cast(jsObj->GetProperties(thread));
        if (propsValue->IsDictionaryMode()) {
            for (int idx = 0; idx < static_cast<int>(jsHClass->NumberOfProps()); idx++) {
                auto attr = layoutInfo->GetAttr(thread, idx);
                if (attr.IsInlinedProps()) {
                    // Do not check inline props
                    continue;
                }
                if (attr.IsWritable()) {
                    LOG_ECMA(ERROR) << "sendable check failed. supposed to be un-writable. idx: " << idx;
                    value.D(thread);
                    return false;
                }
                auto val = propsValue->Get(thread, idx - jsHClass->GetInlinedProperties());
                if (!CheckSendableConstraint(thread, val)) {
                    LOG_ECMA(ERROR) << "sendable check failed. supposed to be sendable. idx: " << idx;
                    value.D(thread);
                    return false;
                }
            }
        }
        return true;
    }
};

/**
 * @tc.name: Parser_001
 * @tc.desc: Passing in a character of type "uint8_t" check whether the result returned through "ParserUtf8" function
 *           Test without for no Nesting.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonParserTest, Parser_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    Utf8JsonParser parser(thread, TransformType::NORMAL);
    // JSON Number
    JSHandle<JSTaggedValue> handleMsg2(factory->NewFromASCII("1234"));
    JSHandle<EcmaString> handleStr2(JSTaggedValue::ToString(thread, handleMsg2));
    JSHandle<JSTaggedValue> result2 = parser.Parse(handleStr2);
    EXPECT_EQ(result2->GetNumber(), 1234);
    // JSON Literal
    JSHandle<JSTaggedValue> handleMsg3(factory->NewFromASCII("true"));
    JSHandle<EcmaString> handleStr3(JSTaggedValue::ToString(thread, handleMsg3));
    JSHandle<JSTaggedValue> result3 = parser.Parse(handleStr3);
    EXPECT_EQ(result3.GetTaggedValue(), JSTaggedValue::True());
    // JSON Unexpected
    JSHandle<JSTaggedValue> handleMsg4(factory->NewFromASCII("trus"));
    JSHandle<EcmaString> handleStr4(JSTaggedValue::ToString(thread, handleMsg4));
    JSHandle<JSTaggedValue> result4 = parser.Parse(handleStr4);
    EXPECT_EQ(result4.GetTaggedValue(), JSTaggedValue::Exception());
}

/**
 * @tc.name: Parser_002
 * @tc.desc: Passing in a character of type "uint16_t" check whether the result returned through "ParseUtf16" function
 *           Test without for no Nesting.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonParserTest, Parser_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    Utf16JsonParser parser(thread, TransformType::NORMAL);

    // JSON Number
    uint16_t array1Utf16[] = {0x31, 0x32, 0x33, 0x34}; // "1234"
    uint32_t array1Utf16Len = sizeof(array1Utf16) / sizeof(array1Utf16[0]);
    JSHandle<JSTaggedValue> handleMsg2(factory->NewFromUtf16(&array1Utf16[0], array1Utf16Len));
    JSHandle<EcmaString> handleStr2(JSTaggedValue::ToString(thread, handleMsg2));
    JSHandle<JSTaggedValue> result2 = parser.Parse(*handleStr2);
    EXPECT_EQ(result2->GetNumber(), 1234);
    // JSON Literal
    uint16_t array2Utf16[] = {0x74, 0x72, 0x75, 0x65}; // "true"
    uint32_t array2Utf16Len = sizeof(array2Utf16) / sizeof(array2Utf16[0]);
    JSHandle<JSTaggedValue> handleMsg3(factory->NewFromUtf16(&array2Utf16[0], array2Utf16Len));
    JSHandle<EcmaString> handleStr3(JSTaggedValue::ToString(thread, handleMsg3));
    JSHandle<JSTaggedValue> result3 = parser.Parse(*handleStr3);
    EXPECT_EQ(result3.GetTaggedValue(), JSTaggedValue::True());
    // JSON String
    uint16_t array3Utf16[] = {0x22, 0x73, 0x74, 0x72, 0x69, 0x6E, 0X67, 0x22}; // "string"
    uint32_t array3Utf16Len = sizeof(array3Utf16) / sizeof(array3Utf16[0]);
    JSHandle<JSTaggedValue> handleMsg4(factory->NewFromUtf16(&array3Utf16[0], array3Utf16Len));
    JSHandle<EcmaString> handleStr4(JSTaggedValue::ToString(thread, handleMsg4));
    JSHandle<JSTaggedValue> result4 = parser.Parse(*handleStr4);
    JSHandle<EcmaString> handleEcmaStr(result4);
    EXPECT_STREQ("string", EcmaStringAccessor(handleEcmaStr).ToCString(thread).c_str());
}

/**
 * @tc.name: Parser_003
 * @tc.desc: Passing in a character of type "uint8_t" check whether the result returned through "ParserUtf8" function
 *           Test with for Nesting of numbers, strings, objects, arrays, Booleans.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonParserTest, Parser_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    Utf8JsonParser parser(thread, TransformType::NORMAL);

    JSHandle<JSTaggedValue> handleMsg(factory->NewFromASCII(
        "\t\r \n{\t\r \n \"json\"\t\r\n:\t\r \n{\t\r \n}\t\r \n,\t\r \n \"prop2\"\t\r \n:\t\r \n [\t\r \nfalse\t\r"
        "\n,\t\r \nnull\t\r, \ntrue\t\r,123.456\t\r \n]\t\r \n}\t\r \n"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg)); // JSON Object
    JSHandle<JSTaggedValue> result = parser.Parse(handleStr);
    EXPECT_TRUE(result->IsECMAObject());
}

/**
 * @tc.name: Parser_004
 * @tc.desc: Passing in a character of type "uint8_t" check whether the result returned through "ParserUtf8" function
 *           Test with for Nesting of numbers, strings, arrays.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonParserTest, Parser_004)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    Utf8JsonParser parser(thread, TransformType::NORMAL);

    JSHandle<JSTaggedValue> handleMsg(factory->NewFromASCII("[100,2.5,\"abc\"]"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg)); // JSON Array
    JSHandle<JSTaggedValue> result = parser.Parse(handleStr);

    JSTaggedValue resultValue(static_cast<JSTaggedType>(result->GetRawData()));
    EXPECT_TRUE(resultValue.IsECMAObject());
    JSHandle<JSObject> valueHandle(thread, resultValue);

    JSHandle<JSTaggedValue> lenResult =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKeyHandle).GetValue();
    uint32_t length = JSTaggedValue::ToLength(thread, lenResult).ToUint32();
    EXPECT_EQ(length, 3U);
}

/**
 * @tc.name: Parser_005
 * @tc.desc: Passing in a character of type "uint8_t" check whether the result returned through "ParserUtf8" function
 *           Test without for Nesting of numbers, strings, objects.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonParserTest, Parser_005)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    Utf8JsonParser parser(thread, TransformType::NORMAL);

    JSHandle<JSTaggedValue> handleMsg(factory->NewFromASCII("{\"epf\":100,\"key1\":400}"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg)); // JSON Object

    JSHandle<JSTaggedValue> result = parser.Parse(handleStr);
    JSTaggedValue resultValue(static_cast<JSTaggedType>(result->GetRawData()));
    EXPECT_TRUE(resultValue.IsECMAObject());

    JSHandle<JSObject> valueHandle(thread, resultValue);
    JSHandle<TaggedArray> nameList(JSObject::EnumerableOwnNames(thread, valueHandle));
    JSHandle<JSArray> nameResult = JSArray::CreateArrayFromList(thread, nameList);

    JSHandle<JSTaggedValue> handleKey(nameResult);
    JSHandle<JSTaggedValue> lengthKey(factory->NewFromASCII("length"));
    JSHandle<JSTaggedValue> lenResult = JSObject::GetProperty(thread, handleKey, lengthKey).GetValue();
    uint32_t length = JSTaggedValue::ToLength(thread, lenResult).ToUint32();
    EXPECT_EQ(length, 2U);
}

/**
 * @tc.name: Parser_006
 * @tc.desc: Try to parse a empty string.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonParserTest, Parser_006)
{
    Utf8JsonParser parser(thread, TransformType::NORMAL);
    JSHandle<EcmaString> emptyString(thread->GlobalConstants()->GetHandledEmptyString());
    JSHandle<JSTaggedValue> result = parser.Parse(emptyString);
    EXPECT_TRUE(result->IsException());
}

/**
 * @tc.name: Parser_007
 * @tc.desc: Try to parse a string containing an empty string.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonParserTest, Parser_007)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    Utf8JsonParser parser(thread, TransformType::NORMAL);

    JSHandle<EcmaString> handleStr(factory->NewFromASCII("\"\""));
    JSHandle<JSTaggedValue> result = parser.Parse(handleStr);
    EXPECT_FALSE(result->IsException());
}

/**
 * @tc.name: Parser_008
 * @tc.desc: Try to parse a string to sendable object.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonParserTest, Parser_008)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    Utf8JsonParser parser(thread, TransformType::SENDABLE);
    JSHandle<JSTaggedValue> handleMsg(
        factory->NewFromASCII(R"({"innerEntry": {"x":1, "y":"abc", "str": "innerStr"}, "x": 1, "str": "outerStr"})"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg));
    JSHandle<JSTaggedValue> result = parser.Parse(handleStr);
    result->D(thread);
    EXPECT_FALSE(result->IsException());
    EXPECT_TRUE(CheckSendableConstraint(thread, result.GetTaggedValue()));
}

/**
 * @tc.name: Parser_009
 * @tc.desc: Try to parse a empty obj json string to sendable object.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonParserTest, Parser_009)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    Utf8JsonParser parser(thread, TransformType::SENDABLE);
    JSHandle<JSTaggedValue> handleMsg(factory->NewFromASCII(R"({})"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg));
    JSHandle<JSTaggedValue> result = parser.Parse(handleStr);
    result->D(thread);
    EXPECT_TRUE(CheckSendableConstraint(thread, result.GetTaggedValue()));
}

/**
 * @tc.name: Parser_010
 * @tc.desc: Try to parse a empty array json string to sendable object.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonParserTest, Parser_010)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    Utf8JsonParser parser(thread, TransformType::SENDABLE);
    JSHandle<JSTaggedValue> handleMsg(factory->NewFromASCII(R"([])"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg));
    JSHandle<JSTaggedValue> result = parser.Parse(handleStr);
    result->D(thread);
    EXPECT_TRUE(CheckSendableConstraint(thread, result.GetTaggedValue()));
}

/**
 * @tc.name: Parser_011
 * @tc.desc: Try to parse a simple array json string to sendable object.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonParserTest, Parser_011)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    Utf8JsonParser parser(thread, TransformType::SENDABLE);
    JSHandle<JSTaggedValue> handleMsg(factory->NewFromASCII(R"([1, 2, 3])"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg));
    JSHandle<JSTaggedValue> result = parser.Parse(handleStr);
    result->D(thread);
    EXPECT_TRUE(CheckSendableConstraint(thread, result.GetTaggedValue()));
}

/**
 * @tc.name: Parser_012
 * @tc.desc: Try to parse a json string with array to sendable object.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonParserTest, Parser_012)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    Utf8JsonParser parser(thread, TransformType::SENDABLE);
    JSHandle<JSTaggedValue> handleMsg(
        factory->NewFromASCII(R"({"innerEntry": {"array": [1, 2, 3]}, "x": 1, "str": "outerStr"})"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg));
    JSHandle<JSTaggedValue> result = parser.Parse(handleStr);
    result->D(thread);
    EXPECT_TRUE(CheckSendableConstraint(thread, result.GetTaggedValue()));
}
} // namespace panda::test
