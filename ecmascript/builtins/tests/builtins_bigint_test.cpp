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

#include "ecmascript/builtins/builtins_bigint.h"

#include "ecmascript/global_env.h"
#include "ecmascript/js_bigint.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
using BigInt = ecmascript::BigInt;
class BuiltinsBigIntTest : public BaseTestWithScope<true> {
};

enum class AlgorithmType {
    BIGINT_CONSTRUCTOR,
    BIGINT_ASINTN,
    BIGINT_ASUINTN,
    BIGINT_TOLOCALSTR,
    BIGINT_TOSTR,
    BIGINT_VALUEOF
};

static JSTaggedValue BigIntAlgorithm(JSThread *thread, std::vector<JSTaggedValue>& args, int32_t argLen,
    AlgorithmType type, JSTaggedValue argThis = JSTaggedValue::Undefined())
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), argLen);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(argThis);
    for (size_t i = 0; i < args.size(); i++) {
        ecmaRuntimeCallInfo->SetCallArg(i, args[i]);
    }

    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result;
    switch (type) {
        case AlgorithmType::BIGINT_CONSTRUCTOR:
            result = BuiltinsBigInt::BigIntConstructor(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::BIGINT_ASINTN:
            result = BuiltinsBigInt::AsIntN(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::BIGINT_ASUINTN:
            result = BuiltinsBigInt::AsUintN(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::BIGINT_TOLOCALSTR:
            result = BuiltinsBigInt::ToLocaleString(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::BIGINT_TOSTR:
            result = BuiltinsBigInt::ToString(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::BIGINT_VALUEOF:
            result = BuiltinsBigInt::ValueOf(ecmaRuntimeCallInfo);
            break;
        default:
            break;
    }
    
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

// new BigInt(123)
HWTEST_F_L0(BuiltinsBigIntTest, BigIntConstructor_001)
{
    JSHandle<JSTaggedValue> numericValue(thread, JSTaggedValue(123));
    std::vector<JSTaggedValue> vals{numericValue.GetTaggedValue()};
    auto result = BigIntAlgorithm(thread, vals, 6, AlgorithmType::BIGINT_CONSTRUCTOR);

    EXPECT_TRUE(result.IsBigInt());
}

// new BigInt("456")
HWTEST_F_L0(BuiltinsBigIntTest, BigIntConstructor_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("456"));

    std::vector<JSTaggedValue> vals{numericValue.GetTaggedValue()};
    auto result = BigIntAlgorithm(thread, vals, 6, AlgorithmType::BIGINT_CONSTRUCTOR);
    EXPECT_TRUE(result.IsBigInt());
}

// AsIntN(64, (2 ^ 63 - 1))
HWTEST_F_L0(BuiltinsBigIntTest, AsIntN_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("9223372036854775807"));
    int bit = 64; // 64-bit
    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<int>(bit)), numericValue.GetTaggedValue()};
    auto result = BigIntAlgorithm(thread, vals, 8, AlgorithmType::BIGINT_ASINTN);
    EXPECT_TRUE(result.IsBigInt());

    JSHandle<BigInt> bigIntHandle(thread, result);
    JSHandle<EcmaString> resultStr = BigInt::ToString(thread, bigIntHandle);
    JSHandle<EcmaString> str = factory->NewFromASCII("9223372036854775807");
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, resultStr, str), 0);
}

// AsIntN(64, (2 ^ 63))
HWTEST_F_L0(BuiltinsBigIntTest, AsIntN_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("9223372036854775808"));
    int bit = 64; // 64-bit
    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<int>(bit)), numericValue.GetTaggedValue()};
    auto result = BigIntAlgorithm(thread, vals, 8, AlgorithmType::BIGINT_ASINTN);

    EXPECT_TRUE(result.IsBigInt());
    JSHandle<BigInt> bigIntHandle(thread, result);
    JSHandle<EcmaString> resultStr = BigInt::ToString(thread, bigIntHandle);
    JSHandle<EcmaString> str = factory->NewFromASCII("-9223372036854775808");
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, resultStr, str), 0);
}

// AsUintN(64, (2 ^ 64 - 1))
HWTEST_F_L0(BuiltinsBigIntTest, AsUintN_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("18446744073709551615"));
    int bit = 64; // 64-bit
    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<int>(bit)), numericValue.GetTaggedValue()};
    auto result = BigIntAlgorithm(thread, vals, 8, AlgorithmType::BIGINT_ASUINTN);

    EXPECT_TRUE(result.IsBigInt());
    JSHandle<BigInt> bigIntHandle(thread, result);
    JSHandle<EcmaString> resultStr = BigInt::ToString(thread, bigIntHandle);
    JSHandle<EcmaString> str = factory->NewFromASCII("18446744073709551615");
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, resultStr, str), 0);
}

// AsUintN(64, (2 ^ 64))
HWTEST_F_L0(BuiltinsBigIntTest, AsUintN_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("18446744073709551616"));
    int bit = 64; // 64-bit

    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<int>(bit)), numericValue.GetTaggedValue()};
    auto result = BigIntAlgorithm(thread, vals, 8, AlgorithmType::BIGINT_ASUINTN);

    EXPECT_TRUE(result.IsBigInt());
    JSHandle<BigInt> bigIntHandle(thread, result);
    JSHandle<EcmaString> resultStr = BigInt::ToString(thread, bigIntHandle);
    JSHandle<EcmaString> str = factory->NewFromASCII("0");
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, resultStr, str), 0);
}

// using locale
HWTEST_F_L0(BuiltinsBigIntTest, ToLocaleString_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("123456789123456789"));

    std::vector<JSTaggedValue> vals{numericValue.GetTaggedValue()};
    auto result1 = BigIntAlgorithm(thread, vals, 6, AlgorithmType::BIGINT_CONSTRUCTOR);

    JSHandle<BigInt> bigIntHandle(thread, result1);
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("de-DE"));

    std::vector<JSTaggedValue> vals2{locale.GetTaggedValue(), JSTaggedValue::Undefined()};
    auto result2 = BigIntAlgorithm(thread, vals2, 8, AlgorithmType::BIGINT_TOLOCALSTR, bigIntHandle.GetTaggedValue());

    EXPECT_TRUE(result2.IsString());
    JSHandle<EcmaString> ecmaStrHandle(thread, result2);
    JSHandle<EcmaString> resultStr(factory->NewFromASCII("123.456.789.123.456.789"));
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, ecmaStrHandle, resultStr), 0);
}

// using locale and options
HWTEST_F_L0(BuiltinsBigIntTest, ToLocaleString_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("123456789123456789"));
    JSHandle<JSTaggedValue> formatStyle = thread->GlobalConstants()->GetHandledStyleString();
    JSHandle<JSTaggedValue> styleKey(factory->NewFromASCII("currency"));
    JSHandle<JSTaggedValue> styleValue(factory->NewFromASCII("EUR"));

    std::vector<JSTaggedValue> vals{numericValue.GetTaggedValue()};
    auto result1 = BigIntAlgorithm(thread, vals, 6, AlgorithmType::BIGINT_CONSTRUCTOR);

    JSHandle<BigInt> bigIntHandle(thread, result1);
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("de-DE"));
    JSObject::SetProperty(thread, optionsObj, formatStyle, styleKey);
    JSObject::SetProperty(thread, optionsObj, styleKey, styleValue);

    std::vector<JSTaggedValue> vals2{locale.GetTaggedValue(), optionsObj.GetTaggedValue()};
    auto result2 = BigIntAlgorithm(thread, vals2, 8, AlgorithmType::BIGINT_TOLOCALSTR, bigIntHandle.GetTaggedValue());

    EXPECT_TRUE(result2.IsString());
    JSHandle<EcmaString> ecmaStrHandle(thread, result2);
    EXPECT_STREQ("123.456.789.123.456.789,00 €", EcmaStringAccessor(ecmaStrHandle).ToCString().c_str());
}

// 17.ToStirng()
HWTEST_F_L0(BuiltinsBigIntTest, ToString_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("17"));

    std::vector<JSTaggedValue> vals{numericValue.GetTaggedValue()};
    auto result1 = BigIntAlgorithm(thread, vals, 6, AlgorithmType::BIGINT_CONSTRUCTOR);

    JSHandle<BigInt> bigIntHandle(thread, result1);
    std::vector<JSTaggedValue> vals2{JSTaggedValue::Undefined()};
    auto result2 = BigIntAlgorithm(thread, vals2, 6, AlgorithmType::BIGINT_TOSTR, bigIntHandle.GetTaggedValue());

    EXPECT_TRUE(result2.IsString());
    JSHandle<EcmaString> ecmaStrHandle(thread, result2);
    EXPECT_STREQ("17", EcmaStringAccessor(ecmaStrHandle).ToCString().c_str());
}

// -0.ToStirng()
HWTEST_F_L0(BuiltinsBigIntTest, ToString_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("-0"));

    std::vector<JSTaggedValue> vals{numericValue.GetTaggedValue()};
    auto result1 = BigIntAlgorithm(thread, vals, 6, AlgorithmType::BIGINT_CONSTRUCTOR);

    JSHandle<BigInt> bigIntHandle(thread, result1);
    std::vector<JSTaggedValue> vals2{JSTaggedValue::Undefined()};
    auto result2 = BigIntAlgorithm(thread, vals2, 6, AlgorithmType::BIGINT_TOSTR, bigIntHandle.GetTaggedValue());

    EXPECT_TRUE(result2.IsString());
    JSHandle<EcmaString> ecmaStrHandle(thread, result2);
    EXPECT_STREQ("0", EcmaStringAccessor(ecmaStrHandle).ToCString().c_str());
}

// -10.ToStirng(2)
HWTEST_F_L0(BuiltinsBigIntTest, ToString_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("-10"));

    std::vector<JSTaggedValue> vals{numericValue.GetTaggedValue()};
    auto result1 = BigIntAlgorithm(thread, vals, 6, AlgorithmType::BIGINT_CONSTRUCTOR);

    JSHandle<BigInt> bigIntHandle(thread, result1);
    JSHandle<JSTaggedValue> radix(thread, JSTaggedValue(2));
    std::vector<JSTaggedValue> vals2{radix.GetTaggedValue()};
    auto result2 = BigIntAlgorithm(thread, vals2, 6, AlgorithmType::BIGINT_TOSTR, bigIntHandle.GetTaggedValue());

    EXPECT_TRUE(result2.IsString());
    JSHandle<EcmaString> ecmaStrHandle(thread, result2);
    EXPECT_STREQ("-1010", EcmaStringAccessor(ecmaStrHandle).ToCString().c_str());
}

// 254.ToStirng(16)
HWTEST_F_L0(BuiltinsBigIntTest, ToString_004)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("254"));

    std::vector<JSTaggedValue> vals{numericValue.GetTaggedValue()};
    auto result1 = BigIntAlgorithm(thread, vals, 6, AlgorithmType::BIGINT_CONSTRUCTOR);

    JSHandle<BigInt> bigIntHandle(thread, result1);
    JSHandle<JSTaggedValue> radix(thread, JSTaggedValue(16));
    std::vector<JSTaggedValue> vals2{radix.GetTaggedValue()};
    auto result2 = BigIntAlgorithm(thread, vals2, 6, AlgorithmType::BIGINT_TOSTR, bigIntHandle.GetTaggedValue());

    EXPECT_TRUE(result2.IsString());
    JSHandle<EcmaString> ecmaStrHandle(thread, result2);
    EXPECT_STREQ("fe", EcmaStringAccessor(ecmaStrHandle).ToCString().c_str());
}

// BigInt.ValueOf
HWTEST_F_L0(BuiltinsBigIntTest, ValueOf_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("-65536"));

    std::vector<JSTaggedValue> vals{numericValue.GetTaggedValue()};
    auto result1 = BigIntAlgorithm(thread, vals, 6, AlgorithmType::BIGINT_CONSTRUCTOR);

    JSHandle<BigInt> bigIntHandle(thread, result1);
    std::vector<JSTaggedValue> vals2{};
    auto result2 = BigIntAlgorithm(thread, vals2, 4, AlgorithmType::BIGINT_VALUEOF, bigIntHandle.GetTaggedValue());

    EXPECT_EQ(BigInt::SameValue(result1, result2), true);
}

// Object.ValueOf
HWTEST_F_L0(BuiltinsBigIntTest, ValueOf_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("65535"));

    std::vector<JSTaggedValue> vals{numericValue.GetTaggedValue()};
    auto result1 = BigIntAlgorithm(thread, vals, 6, AlgorithmType::BIGINT_CONSTRUCTOR);

    JSHandle<BigInt> bigIntHandle(thread, result1);
    JSHandle<JSTaggedValue> bigIntObj(bigIntHandle);
    std::vector<JSTaggedValue> vals2{};

    JSHandle<JSPrimitiveRef> jsPrimitiveRef = factory->NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_BIGINT, bigIntObj);
    auto result2 = BigIntAlgorithm(thread, vals2, 4, AlgorithmType::BIGINT_VALUEOF, jsPrimitiveRef.GetTaggedValue());

    EXPECT_EQ(BigInt::SameValue(bigIntHandle.GetTaggedValue(), result2), true);
}

// testcases of NumberToBigint()
HWTEST_F_L0(BuiltinsBigIntTest, NumberToBigint)
{
    JSHandle<JSTaggedValue> number(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> bigint(thread, JSTaggedValue::Undefined());

    number = JSHandle<JSTaggedValue>(thread, JSTaggedValue(base::MAX_VALUE));
    bigint = JSHandle<JSTaggedValue>(thread, BigInt::NumberToBigInt(thread, number));
    ASSERT_TRUE(bigint->IsBigInt());
    bool compareRes = JSTaggedValue::Equal(thread, number, bigint);
    ASSERT_TRUE(compareRes);

    number = JSHandle<JSTaggedValue>(thread, JSTaggedValue(-base::MAX_VALUE));
    bigint = JSHandle<JSTaggedValue>(thread, BigInt::NumberToBigInt(thread, number));
    ASSERT_TRUE(bigint->IsBigInt());
    compareRes = JSTaggedValue::Equal(thread, number, bigint);
    ASSERT_TRUE(JSHandle<BigInt>::Cast(bigint)->GetSign());
    ASSERT_TRUE(compareRes);

    number = JSHandle<JSTaggedValue>(thread, JSTaggedValue(-0xffffffff));
    bigint = JSHandle<JSTaggedValue>(thread, BigInt::NumberToBigInt(thread, number));
    ASSERT_TRUE(bigint->IsBigInt());
    compareRes = JSTaggedValue::Equal(thread, number, bigint);
    ASSERT_TRUE(compareRes);

    number = JSHandle<JSTaggedValue>(thread, JSTaggedValue(0));
    bigint = JSHandle<JSTaggedValue>(thread, BigInt::NumberToBigInt(thread, number));
    ASSERT_TRUE(bigint->IsBigInt());
    compareRes = JSTaggedValue::Equal(thread, number, bigint);
    ASSERT_TRUE(compareRes);

    number = JSHandle<JSTaggedValue>(thread, JSTaggedValue(std::numeric_limits<double>::infinity()));
    bigint = JSHandle<JSTaggedValue>(thread, BigInt::NumberToBigInt(thread, number));
    ASSERT_TRUE(bigint->IsException());
    thread->ClearException();
}

// testcases of BigintToNumber()
HWTEST_F_L0(BuiltinsBigIntTest, BigintToNumber)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> bigint(thread, JSTaggedValue::Undefined());
    JSTaggedNumber number(0);

    JSHandle<JSTaggedValue> parma(factory->NewFromASCII("0xffff"));
    bigint = JSHandle<JSTaggedValue>(thread, JSTaggedValue::ToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    number = BigInt::BigIntToNumber(JSHandle<BigInt>::Cast(bigint));
    ASSERT_EQ(number.GetNumber(), static_cast<double>(0xffff));

    parma = JSHandle<JSTaggedValue>(
        factory->NewFromASCII("0xfffffffffffff8000000000000000000000000000000000000000000000000000"
                              "0000000000000000000000000000000000000000000000000000000000000000000"
                              "0000000000000000000000000000000000000000000000000000000000000000000"
                              "000000000000000000000000000000000000000000000000000000000"));
    bigint = JSHandle<JSTaggedValue>(thread, JSTaggedValue::ToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    number = BigInt::BigIntToNumber(JSHandle<BigInt>::Cast(bigint));
    ASSERT_EQ(number.GetNumber(), base::MAX_VALUE);

    parma = JSHandle<JSTaggedValue>(thread, JSTaggedValue::False());
    bigint = JSHandle<JSTaggedValue>(thread, JSTaggedValue::ToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    ASSERT_TRUE(JSHandle<BigInt>::Cast(bigint)->IsZero());
    number = BigInt::BigIntToNumber(JSHandle<BigInt>::Cast(bigint));
    ASSERT_EQ(number.GetNumber(), 0.0);

    parma = JSHandle<JSTaggedValue>(thread, JSTaggedValue(base::MAX_VALUE));
    bigint = JSHandle<JSTaggedValue>(thread, BigInt::NumberToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    number = BigInt::BigIntToNumber(JSHandle<BigInt>::Cast(bigint));
    ASSERT_EQ(number.GetNumber(), base::MAX_VALUE);

    parma = JSHandle<JSTaggedValue>(thread, JSTaggedValue(-base::MAX_VALUE));
    bigint = JSHandle<JSTaggedValue>(thread, BigInt::NumberToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    number = BigInt::BigIntToNumber(JSHandle<BigInt>::Cast(bigint));
    ASSERT_EQ(number.GetNumber(), -base::MAX_VALUE);

    parma = JSHandle<JSTaggedValue>(thread, JSTaggedValue(-0xffffffff));
    bigint = JSHandle<JSTaggedValue>(thread, BigInt::NumberToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    number = BigInt::BigIntToNumber(JSHandle<BigInt>::Cast(bigint));
    ASSERT_EQ(number.GetNumber(), -0xffffffff);
}

// testcases of StringToBigInt(EcmaString)
HWTEST_F_L0(BuiltinsBigIntTest, StringToBigInt)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> bigint;
    JSHandle<EcmaString> str;
    JSHandle<JSTaggedValue> parma;

    // hex string
    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("0xffff"));
    bigint = JSHandle<JSTaggedValue>(thread, base::NumberHelper::StringToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    str = BigInt::ToString(thread, JSHandle<BigInt>::Cast(bigint), BigInt::HEXADECIMAL);
    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("ffff"));
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, str, JSHandle<EcmaString>(parma)), 0);

    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("0XFFFF"));
    bigint = JSHandle<JSTaggedValue>(thread, base::NumberHelper::StringToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    str = BigInt::ToString(thread, JSHandle<BigInt>::Cast(bigint), BigInt::HEXADECIMAL);
    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("ffff"));
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, str, JSHandle<EcmaString>(parma)), 0);

    // binary string
    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("0b11111111"));
    bigint = JSHandle<JSTaggedValue>(thread, base::NumberHelper::StringToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    str = BigInt::ToString(thread, JSHandle<BigInt>::Cast(bigint), BigInt::BINARY);
    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("11111111"));
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, str, JSHandle<EcmaString>(parma)), 0);

    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("0B11111111"));
    bigint = JSHandle<JSTaggedValue>(thread, base::NumberHelper::StringToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    str = BigInt::ToString(thread, JSHandle<BigInt>::Cast(bigint), BigInt::BINARY);
    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("11111111"));
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, str, JSHandle<EcmaString>(parma)), 0);

    // octal string
    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("0o123456"));
    bigint = JSHandle<JSTaggedValue>(thread, base::NumberHelper::StringToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    str = BigInt::ToString(thread, JSHandle<BigInt>::Cast(bigint), BigInt::OCTAL);
    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("123456"));
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, str, JSHandle<EcmaString>(parma)), 0);

    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("0O123456"));
    bigint = JSHandle<JSTaggedValue>(thread, base::NumberHelper::StringToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    str = BigInt::ToString(thread, JSHandle<BigInt>::Cast(bigint), BigInt::OCTAL);
    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("123456"));
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, str, JSHandle<EcmaString>(parma)), 0);

    // decimal string
    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("999999999"));
    bigint = JSHandle<JSTaggedValue>(thread, base::NumberHelper::StringToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    str = BigInt::ToString(thread, JSHandle<BigInt>::Cast(bigint));
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, str, JSHandle<EcmaString>(parma)), 0);

    // string has space
    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("  123  "));
    bigint = JSHandle<JSTaggedValue>(thread, base::NumberHelper::StringToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    JSHandle<JSTaggedValue> number(thread, JSTaggedValue(static_cast<double>(123)));
    bool compareRes = JSTaggedValue::Equal(thread, bigint, number);
    ASSERT_TRUE(compareRes);

    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("123   "));
    bigint = JSHandle<JSTaggedValue>(thread, base::NumberHelper::StringToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    number = JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<double>(123)));
    compareRes = JSTaggedValue::Equal(thread, bigint, number);
    ASSERT_TRUE(compareRes);

    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("   123"));
    bigint = JSHandle<JSTaggedValue>(thread, base::NumberHelper::StringToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    number = JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<double>(123)));
    compareRes = JSTaggedValue::Equal(thread, bigint, number);
    ASSERT_TRUE(compareRes);

    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII(""));
    bigint = JSHandle<JSTaggedValue>(thread, base::NumberHelper::StringToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    ASSERT_TRUE(JSHandle<BigInt>::Cast(bigint)->IsZero());

    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("    "));
    bigint = JSHandle<JSTaggedValue>(thread, base::NumberHelper::StringToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    ASSERT_TRUE(JSHandle<BigInt>::Cast(bigint)->IsZero());
}
}  // namespace panda::test
