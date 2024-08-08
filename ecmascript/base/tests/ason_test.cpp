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
class AsonParserTest : public BaseTestWithScope<false> {
public:
    using TransformType = base::JsonHelper::TransformType;

    bool CheckSendableConstraint(JSTaggedValue value) const
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
            value.D();
            return false;
        }
        if (!CheckSendableProps(value, obj)) {
            return false;
        }
        // trace proto chain
        auto proto = jsHClass->GetPrototype();
        if (!proto.IsNull() && !proto.IsJSShared()) {
            LOG_ECMA(ERROR) << "sendable check failed. proto is not sendable.";
            value.D();
            return false;
        }
        return true;
    }

    bool CheckSendableProps(JSTaggedValue value, TaggedObject *obj) const
    {
        auto *jsHClass = obj->GetClass();
        auto layoutValue = jsHClass->GetLayout();
        if (layoutValue.IsNull()) {
            return true;
        }
        auto *layoutInfo = LayoutInfo::Cast(layoutValue.GetTaggedObject());
        auto *jsObj = JSObject::Cast(obj);
        auto *propsValue = TaggedArray::Cast(jsObj->GetProperties());
        if (propsValue->IsDictionaryMode()) {
            for (int idx = 0; idx < static_cast<int>(jsHClass->NumberOfProps()); idx++) {
                auto attr = layoutInfo->GetAttr(idx);
                if (attr.IsInlinedProps()) {
                    // Do not check inline props
                    continue;
                }
                if (attr.IsWritable()) {
                    LOG_ECMA(ERROR) << "sendable check failed. supposed to be un-writable. idx: " << idx;
                    value.D();
                    return false;
                }
                auto val = propsValue->Get(thread, idx - jsHClass->GetInlinedProperties());
                if (!CheckSendableConstraint(val)) {
                    LOG_ECMA(ERROR) << "sendable check failed. supposed to be sendable. idx: " << idx;
                    value.D();
                    return false;
                }
            }
        }
        return true;
    }
};

HWTEST_F_L0(AsonParserTest, Parser_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    Utf8JsonParser parser(thread, TransformType::SENDABLE);
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
    // JSON LiteraF
    JSHandle<JSTaggedValue> handleMsg4(factory->NewFromASCII("false"));
    JSHandle<EcmaString> handleStr4(JSTaggedValue::ToString(thread, handleMsg4));
    JSHandle<JSTaggedValue> result4 = parser.Parse(handleStr4);
    EXPECT_EQ(result4.GetTaggedValue(), JSTaggedValue::False());
    // JSON Unexpected
    JSHandle<JSTaggedValue> handleMsg5(factory->NewFromASCII("trus"));
    JSHandle<EcmaString> handleStr5(JSTaggedValue::ToString(thread, handleMsg5));
    JSHandle<JSTaggedValue> result5 = parser.Parse(handleStr5);
    EXPECT_EQ(result5.GetTaggedValue(), JSTaggedValue::Exception());
}

HWTEST_F_L0(AsonParserTest, Parser_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    Utf16JsonParser parser(thread, TransformType::SENDABLE);

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
    EXPECT_STREQ("string", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

HWTEST_F_L0(AsonParserTest, Parser_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    Utf8JsonParser parser(thread, TransformType::SENDABLE);
    JSHandle<JSTaggedValue> handleMsg(factory->NewFromASCII(
        "\t\r \n{\t\r \n \"json\"\t\r\n:\t\r \n{\t\r \n}\t\r \n,\t\r \n \"prop2\"\t\r \n:\t\r \n [\t\r \nfalse\t\r"
        "\n,\t\r \nnull\t\r, \ntrue\t\r,123.456\t\r \n]\t\r \n}\t\r \n"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg)); // JSON Object
    JSHandle<JSTaggedValue> result = parser.Parse(handleStr);
    result->D();
    EXPECT_TRUE(CheckSendableConstraint(result.GetTaggedValue()));
}

HWTEST_F_L0(AsonParserTest, Parser_004)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    Utf8JsonParser parser(thread, TransformType::SENDABLE);
    JSHandle<JSTaggedValue> handleMsg(factory->NewFromASCII("[100,2.5,\"abc\"]"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg)); // JSON Array
    JSHandle<JSTaggedValue> result = parser.Parse(handleStr);
    result->D();
    EXPECT_TRUE(CheckSendableConstraint(result.GetTaggedValue()));
}

HWTEST_F_L0(AsonParserTest, Parser_005)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    Utf8JsonParser parser(thread, TransformType::SENDABLE);
    JSHandle<JSTaggedValue> handleMsg(factory->NewFromASCII("{\"epf\":100,\"key1\":400}"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg));
    JSHandle<JSTaggedValue> result = parser.Parse(handleStr);
    EXPECT_TRUE(CheckSendableConstraint(result.GetTaggedValue()));
}

HWTEST_F_L0(AsonParserTest, Parser_006)
{
    Utf8JsonParser parser(thread, TransformType::SENDABLE);
    JSHandle<EcmaString> emptyString(thread->GlobalConstants()->GetHandledEmptyString());
    JSHandle<JSTaggedValue> result = parser.Parse(emptyString);
    EXPECT_TRUE(result->IsException());
}

HWTEST_F_L0(AsonParserTest, Parser_007)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    Utf8JsonParser parser(thread, TransformType::SENDABLE);

    JSHandle<EcmaString> handleStr(factory->NewFromASCII("\"\""));
    JSHandle<JSTaggedValue> result = parser.Parse(handleStr);
    EXPECT_FALSE(result->IsException());
}

HWTEST_F_L0(AsonParserTest, Parser_008)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    Utf8JsonParser parser(thread, TransformType::SENDABLE);
    JSHandle<JSTaggedValue> handleMsg(
        factory->NewFromASCII(R"({"innerEntry": {"x":1, "y":"abc", "str": "innerStr"}, "x": 1, "str": "outerStr"})"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg));
    JSHandle<JSTaggedValue> result = parser.Parse(handleStr);
    result->D();
    EXPECT_FALSE(result->IsException());
    EXPECT_TRUE(CheckSendableConstraint(result.GetTaggedValue()));
}

HWTEST_F_L0(AsonParserTest, Parser_009)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    Utf8JsonParser parser(thread, TransformType::SENDABLE);
    JSHandle<JSTaggedValue> handleMsg(factory->NewFromASCII(R"({})"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg));
    JSHandle<JSTaggedValue> result = parser.Parse(handleStr);
    result->D();
    EXPECT_TRUE(CheckSendableConstraint(result.GetTaggedValue()));
}

HWTEST_F_L0(AsonParserTest, Parser_010)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    Utf8JsonParser parser(thread, TransformType::SENDABLE);
    JSHandle<JSTaggedValue> handleMsg(factory->NewFromASCII(R"([])"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg));
    JSHandle<JSTaggedValue> result = parser.Parse(handleStr);
    result->D();
    EXPECT_TRUE(CheckSendableConstraint(result.GetTaggedValue()));
}

HWTEST_F_L0(AsonParserTest, Parser_011)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    Utf8JsonParser parser(thread, TransformType::SENDABLE);
    JSHandle<JSTaggedValue> handleMsg(factory->NewFromASCII(R"([1, 2, 3])"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg));
    JSHandle<JSTaggedValue> result = parser.Parse(handleStr);
    result->D();
    EXPECT_TRUE(CheckSendableConstraint(result.GetTaggedValue()));
}

HWTEST_F_L0(AsonParserTest, Parser_012)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    Utf8JsonParser parser(thread, TransformType::SENDABLE);
    JSHandle<JSTaggedValue> handleMsg(
        factory->NewFromASCII(R"({"innerEntry": {"array": [1, 2, 3]}, "x": 1, "str": "outerStr"})"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg));
    JSHandle<JSTaggedValue> result = parser.Parse(handleStr);
    result->D();
    EXPECT_TRUE(CheckSendableConstraint(result.GetTaggedValue()));
}

HWTEST_F_L0(AsonParserTest, Parser_013)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSMutableHandle<JSFunction> constructor(thread, JSTaggedValue::Undefined());
    constructor.Update(env->GetSObjectFunction());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> root = factory->NewJSObjectByConstructor(constructor);
    JSHandle<JSTaggedValue> rootName(factory->GetEmptyString());
    JSHandle<JSTaggedValue> result;
    JSHandle<JSTaggedValue> undefined = JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined());
    result = Internalize::InternalizeJsonProperty(thread, root, rootName, undefined, TransformType::SENDABLE);
    ASSERT_TRUE(!result->IsUndefined());
}
} // namespace panda::test
