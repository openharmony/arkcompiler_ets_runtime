/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "common_interfaces/objects/base_type.h"
#include "ecmascript/dynamic_type_converter.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_bigint-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/tests/test_helper.h"
#include "common_interfaces/thread/thread_holder.h"
#include "common_interfaces/thread/thread_holder_manager.h"

using namespace panda::ecmascript;

namespace panda::test {
class DynamicTypeConverterTest : public BaseTestWithScope<false> {};

static JSHandle<JSObject> JSObjectCreate(JSThread *thread)
{
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = globalEnv->GetObjectFunction();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    return obj;
}

/**
 * @tc.name: WrapTagged_Test0
 * @tc.desc: Convert BaseType to DynamicType by converter.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(DynamicTypeConverterTest, WrapTagged_Test0)
{
    DynamicTypeConverter dynTypeConverter_;
    ThreadHolder *threadHolder = thread->GetThreadHolder();
    // 1. Test monostate (empty variant)
    {
        BaseType value = std::monostate{};
        JSTaggedValue result = dynTypeConverter_.WrapTagged(threadHolder, value);
        EXPECT_TRUE(result.IsHole());
    }
    // 2. Test boolean types
    {
        BaseType value = true;
        JSTaggedValue result = dynTypeConverter_.WrapTagged(threadHolder, value);
        EXPECT_TRUE(result.IsTrue());
    }
    {
        BaseType value = false;
        JSTaggedValue result = dynTypeConverter_.WrapTagged(threadHolder, value);
        EXPECT_TRUE(result.IsFalse());
    }
    // 3. Test all integer types
    // int8_t
    {
        BaseType value = static_cast<int8_t>(42);
        JSTaggedValue result = dynTypeConverter_.WrapTagged(threadHolder, value);
        EXPECT_TRUE(result.IsInt());
        EXPECT_EQ(result.GetInt(), 42);
    }
    // uint8_t
    {
        BaseType value = static_cast<uint8_t>(255);
        JSTaggedValue result = dynTypeConverter_.WrapTagged(threadHolder, value);
        EXPECT_TRUE(result.IsInt());
        EXPECT_EQ(result.GetInt(), 255);
    }
    // int16_t
    {
        BaseType value = static_cast<int16_t>(32767);
        JSTaggedValue result = dynTypeConverter_.WrapTagged(threadHolder, value);
        EXPECT_TRUE(result.IsInt());
        EXPECT_EQ(result.GetInt(), 32767);
    }
    // uint16_t
    {
        BaseType value = static_cast<uint16_t>(65535);
        JSTaggedValue result = dynTypeConverter_.WrapTagged(threadHolder, value);
        EXPECT_TRUE(result.IsInt());
        EXPECT_EQ(result.GetInt(), 65535);
    }
}

/**
 * @tc.name: WrapTagged_Test1
 * @tc.desc: Convert BaseType to DynamicType by converter.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(DynamicTypeConverterTest, WrapTagged_Test1)
{
    DynamicTypeConverter dynTypeConverter_;
    ThreadHolder *threadHolder = thread->GetThreadHolder();
    // int32_t
    {
        BaseType value = static_cast<int32_t>(2147483647);
        JSTaggedValue result = dynTypeConverter_.WrapTagged(threadHolder, value);
        EXPECT_TRUE(result.IsInt());
        EXPECT_EQ(result.GetInt(), 2147483647);
    }
    // uint32_t (special case)
    {
        BaseType value = static_cast<uint32_t>(4294967295);
        JSTaggedValue result = dynTypeConverter_.WrapTagged(threadHolder, value);
        EXPECT_TRUE(result.IsDouble());
    }
    // 4. Test floating point types
    {
        BaseType value = 3.14f;
        JSTaggedValue result = dynTypeConverter_.WrapTagged(threadHolder, value);
        EXPECT_TRUE(result.IsDouble());
        EXPECT_FLOAT_EQ(result.GetDouble(), 3.14f);
    }
    {
        BaseType value = 2.71828;
        JSTaggedValue result = dynTypeConverter_.WrapTagged(threadHolder, value);
        EXPECT_TRUE(result.IsDouble());
        EXPECT_DOUBLE_EQ(result.GetDouble(), 2.71828);
    }
    // 5. Test 64-bit integers (converted to double)
    {
        BaseType value = static_cast<int64_t>(9223372036854775807LL);
        JSTaggedValue result = dynTypeConverter_.WrapTagged(threadHolder, value);
        EXPECT_TRUE(result.IsDouble());
        EXPECT_DOUBLE_EQ(result.GetDouble(), 9223372036854775807.0);
    }
    {
        BaseType value = static_cast<uint64_t>(18446744073709551615ULL);
        JSTaggedValue result = dynTypeConverter_.WrapTagged(threadHolder, value);
        EXPECT_TRUE(result.IsDouble());
        EXPECT_DOUBLE_EQ(result.GetDouble(), 18446744073709551615.0);
    }
}

/**
 * @tc.name: WrapTagged_Test2
 * @tc.desc: Convert BaseType to DynamicType by converter.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(DynamicTypeConverterTest, WrapTagged_Test2)
{
    DynamicTypeConverter dynTypeConverter_;
    ThreadHolder *threadHolder = thread->GetThreadHolder();
    // 6. Test undefined and null
    {
        BaseType value = common::BaseUndefined{};
        JSTaggedValue result = dynTypeConverter_.WrapTagged(threadHolder, value);
        EXPECT_TRUE(result.IsUndefined());
    }
    {
        BaseType value = common::BaseNull{};
        JSTaggedValue result = dynTypeConverter_.WrapTagged(threadHolder, value);
        EXPECT_TRUE(result.IsNull());
    }
    // 7. Test BigInt with new struct definition
    {
        common::BaseBigInt bigIntValue;
        bigIntValue.length = 3;
        bigIntValue.sign = true; // Negative value
        bigIntValue.data = {0x12345678, 0x9ABCDEF0, 0x13579BDF};
        BaseType value = bigIntValue;
        JSTaggedValue result = dynTypeConverter_.WrapTagged(threadHolder, value);

        EXPECT_TRUE(result.IsBigInt());
        BigInt* bigInt = BigInt::Cast(result.GetTaggedObject());
        // Verify BigInt properties
        EXPECT_EQ(bigInt->GetLength(), bigIntValue.length);
        EXPECT_EQ(bigInt->GetSign(), bigIntValue.sign);

        // Verify all digits were copied correctly
        for (uint32_t i = 0; i < bigIntValue.length; i++) {
            EXPECT_EQ(bigInt->GetDigit(i), bigIntValue.data[i]);
        }
    }
    // 8. Test object type
    {
        JSHandle<JSObject> obj = JSObjectCreate(thread);
        BaseType value = static_cast<BaseObject*>(obj.GetTaggedValue().GetTaggedObject());
        JSTaggedValue result = dynTypeConverter_.WrapTagged(threadHolder, value);
        EXPECT_EQ(result, obj.GetTaggedValue());
    }
    // 9. Test String with BaseString
    {
        uint8_t arrayU8[] = {"xyz123!@#"};
        size_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
        EcmaString *str = EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8[0], lengthEcmaStrU8, true);
        BaseType value = str->ToBaseString();
        JSTaggedValue result = dynTypeConverter_.WrapTagged(threadHolder, value);

        EXPECT_TRUE(result.IsString());
        EcmaString* str1 = EcmaString::Cast(result.GetTaggedObject());
        // Verify BigInt properties
        EXPECT_EQ(EcmaStringAccessor(str1).GetLength(), str->ToBaseString()->GetLength());
        EXPECT_EQ(EcmaStringAccessor(str1).GetDataUtf8(), str->ToBaseString()->GetDataUtf8());
    }
}

/**
 * @tc.name: UnWrapTagged0
 * @tc.desc: Convert DynamicType to BaseType by converter.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(DynamicTypeConverterTest, UnWrapTagged_Test0)
{
    DynamicTypeConverter dynTypeConverter_;
    /* Boolean type tests */
    {
        // Test boolean true
        JSTaggedValue trueValue = JSTaggedValue::True();
        auto result = dynTypeConverter_.UnWrapTagged(trueValue);
        EXPECT_TRUE(std::holds_alternative<bool>(result));
        EXPECT_TRUE(std::get<bool>(result));

        // Test boolean false
        JSTaggedValue falseValue = JSTaggedValue::False();
        result = dynTypeConverter_.UnWrapTagged(falseValue);
        EXPECT_TRUE(std::holds_alternative<bool>(result));
        EXPECT_FALSE(std::get<bool>(result));
    }
    /* Integer type tests */
    {
        // Test positive integer
        JSTaggedValue intValue(42);
        auto result = dynTypeConverter_.UnWrapTagged(intValue);
        EXPECT_TRUE(std::holds_alternative<int32_t>(result));
        EXPECT_EQ(std::get<int32_t>(result), 42);

        // Test negative integer
        JSTaggedValue negativeIntValue(-123);
        result = dynTypeConverter_.UnWrapTagged(negativeIntValue);
        EXPECT_TRUE(std::holds_alternative<int32_t>(result));
        EXPECT_EQ(std::get<int32_t>(result), -123);
    }
    /* Double type tests */
    {
        JSTaggedValue doubleValue(3.14159);
        auto result = dynTypeConverter_.UnWrapTagged(doubleValue);
        EXPECT_TRUE(std::holds_alternative<double>(result));
        EXPECT_DOUBLE_EQ(std::get<double>(result), 3.14159);
    }
}

/**
 * @tc.name: UnWrapTagged1
 * @tc.desc: Convert DynamicType to BaseType by converter.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(DynamicTypeConverterTest, UnWrapTagged_Test1)
{
    DynamicTypeConverter dynTypeConverter_;
    /* Undefined type tests */
    {
        JSTaggedValue undefinedValue = JSTaggedValue::Undefined();
        auto result = dynTypeConverter_.UnWrapTagged(undefinedValue);
        EXPECT_TRUE(std::holds_alternative<common::BaseUndefined>(result));
    }
    /* Null type tests */
    {
        JSTaggedValue nullValue = JSTaggedValue::Null();
        auto result = dynTypeConverter_.UnWrapTagged(nullValue);
        EXPECT_TRUE(std::holds_alternative<common::BaseNull>(result));
    }
    /* BigInt type tests */
    {
        const uint32_t length = 2;
        bool sign = false;
        uint64_t digits[length] = {123456789, 987654321};
        BigInt* bigInt = *BigInt::CreateBigint(thread, length);
        bigInt->SetSign(sign);
        for (uint32_t i = 0; i < length; i++) {
            bigInt->SetDigit(i, digits[i]);
        }
        JSTaggedValue bigIntValue(bigInt);
        auto result = dynTypeConverter_.UnWrapTagged(bigIntValue);

        EXPECT_TRUE(std::holds_alternative<common::BaseBigInt>(result));
        const common::BaseBigInt& baseBigInt = std::get<common::BaseBigInt>(result);

        EXPECT_EQ(baseBigInt.length, length);
        EXPECT_EQ(baseBigInt.sign, sign);
        EXPECT_EQ(baseBigInt.data.size(), length);
        for (uint32_t i = 0; i < length; i++) {
            EXPECT_EQ(baseBigInt.data[i], digits[i]);
        }
    }
    /* HeapObject type tests */
    {
        JSHandle<JSObject> obj = JSObjectCreate(thread);
        auto result = dynTypeConverter_.UnWrapTagged(obj.GetTaggedValue());
        EXPECT_TRUE(std::holds_alternative<BaseObject*>(result));
        EXPECT_EQ(std::get<BaseObject*>(result), static_cast<BaseObject*>(obj.GetTaggedValue().GetTaggedObject()));
    }
    /* String type tests */
    {
        uint8_t arrayU8[] = {"xyz123!@#"};
        size_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
        EcmaString *str = EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8[0], lengthEcmaStrU8, true);
        auto result = dynTypeConverter_.UnWrapTagged(JSTaggedValue(str));

        EXPECT_TRUE(std::holds_alternative<BaseString*>(result));
        EXPECT_EQ(std::get<BaseString*>(result), str->ToBaseString());
    }
}
}
