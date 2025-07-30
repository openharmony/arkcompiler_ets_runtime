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

#include <climits>

#include "ecmascript/js_bigint-inl.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSBigintTest : public BaseTestWithScope<false> {
};

/**
 * @tc.name: Compare
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, Compare)
{
    CString str1 = "9007199254740991012345";
    CString str2 = "9007199254740991012345 ";
    CString str3 = "-9007199254740991012345";
    CString str4 = "-9007199254740991012";
    JSHandle<BigInt> bigint1 = BigIntHelper::SetBigInt(thread, str1);
    JSHandle<BigInt> bigint2 = BigIntHelper::SetBigInt(thread, str2);
    JSHandle<BigInt> bigint3 = BigIntHelper::SetBigInt(thread, str3);
    JSHandle<BigInt> bigint4 = BigIntHelper::SetBigInt(thread, str4);
    EXPECT_EQ(BigInt::Compare(bigint1.GetTaggedValue(), bigint1.GetTaggedValue()), ComparisonResult::EQUAL);
    EXPECT_EQ(BigInt::Compare(bigint3.GetTaggedValue(), bigint2.GetTaggedValue()), ComparisonResult::LESS);
    EXPECT_EQ(BigInt::Compare(bigint1.GetTaggedValue(), bigint2.GetTaggedValue()), ComparisonResult::LESS);
    EXPECT_EQ(BigInt::Compare(bigint2.GetTaggedValue(), bigint1.GetTaggedValue()), ComparisonResult::GREAT);
    EXPECT_EQ(BigInt::Compare(bigint2.GetTaggedValue(), bigint3.GetTaggedValue()), ComparisonResult::GREAT);
    EXPECT_EQ(BigInt::Compare(bigint3.GetTaggedValue(), bigint4.GetTaggedValue()), ComparisonResult::LESS);
    EXPECT_EQ(BigInt::Compare(bigint4.GetTaggedValue(), bigint3.GetTaggedValue()), ComparisonResult::GREAT);

    JSHandle<BigInt> zero = BigInt::Uint32ToBigInt(thread, 0);
    EXPECT_EQ(BigInt::Compare(zero.GetTaggedValue(), bigint1.GetTaggedValue()), ComparisonResult::LESS);
    EXPECT_EQ(BigInt::Compare(bigint1.GetTaggedValue(), zero.GetTaggedValue()), ComparisonResult::GREAT);
    EXPECT_EQ(BigInt::Compare(zero.GetTaggedValue(), zero.GetTaggedValue()), ComparisonResult::EQUAL);
    EXPECT_EQ(BigInt::Compare(zero.GetTaggedValue(), bigint3.GetTaggedValue()), ComparisonResult::GREAT);
    EXPECT_EQ(BigInt::Compare(bigint3.GetTaggedValue(), zero.GetTaggedValue()), ComparisonResult::LESS);
}

/**
 * @tc.name: CreateBigint
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, CreateBigint)
{
    uint32_t size = 100;
    JSHandle<BigInt> bigint = BigInt::CreateBigint(thread, size);
    EXPECT_EQ(bigint->GetLength(), size);
}

/**
 * @tc.name: Equal & SameValue & SameValueZero
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, Equal_SameValue_SameValueZero)
{
    // The largest safe integer in JavaScript is in [-(2 ^ 53 - 1), 2 ^ 53 - 1].
    CString maxSafeIntStr = "9007199254740991";
    CString minSafeIntStr = "-9007199254740991";
    JSHandle<BigInt> maxSafeInt = BigIntHelper::SetBigInt(thread, maxSafeIntStr);
    JSHandle<BigInt> minSafeInt = BigIntHelper::SetBigInt(thread, minSafeIntStr);

    // Compare two integers in the safe range.
    JSHandle<BigInt> minusMinSafeInt = BigInt::UnaryMinus(thread, minSafeInt);
    JSHandle<BigInt> minusMaxSafeInt = BigInt::UnaryMinus(thread, maxSafeInt);
    bool result1 = BigInt::Equal(maxSafeInt.GetTaggedValue(), minSafeInt.GetTaggedValue());
    bool result2 = BigInt::SameValue(maxSafeInt.GetTaggedValue(), minSafeInt.GetTaggedValue());
    bool result3 = BigInt::SameValueZero(maxSafeInt.GetTaggedValue(), minSafeInt.GetTaggedValue());
    EXPECT_TRUE(!result1);
    EXPECT_TRUE(!result2);
    EXPECT_TRUE(!result3);
    result1 = BigInt::Equal(maxSafeInt.GetTaggedValue(), minusMinSafeInt.GetTaggedValue());
    result2 = BigInt::SameValue(maxSafeInt.GetTaggedValue(), minusMinSafeInt.GetTaggedValue());
    result3 = BigInt::SameValueZero(maxSafeInt.GetTaggedValue(), minusMinSafeInt.GetTaggedValue());
    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);
    EXPECT_TRUE(result3);
    result1 = BigInt::Equal(minSafeInt.GetTaggedValue(), minusMaxSafeInt.GetTaggedValue());
    result2 = BigInt::SameValue(minSafeInt.GetTaggedValue(), minusMaxSafeInt.GetTaggedValue());
    result3 = BigInt::SameValueZero(minSafeInt.GetTaggedValue(), minusMaxSafeInt.GetTaggedValue());
    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);
    EXPECT_TRUE(result3);

    // Compare two integers outside the safe range.
    CString unsafeIntStr1 = maxSafeIntStr + "0123456789";
    CString unsafeIntStr2 = minSafeIntStr + "0123456789";
    JSHandle<BigInt> unsafeInt1 = BigIntHelper::SetBigInt(thread, unsafeIntStr1);
    JSHandle<BigInt> unsafeInt2 = BigIntHelper::SetBigInt(thread, unsafeIntStr2);
    JSHandle<BigInt> minusUnsafeInt1 = BigInt::UnaryMinus(thread, unsafeInt1);
    result1 = BigInt::Equal(unsafeInt2.GetTaggedValue(), minusUnsafeInt1.GetTaggedValue());
    result2 = BigInt::SameValue(unsafeInt2.GetTaggedValue(), minusUnsafeInt1.GetTaggedValue());
    result3 = BigInt::SameValueZero(unsafeInt2.GetTaggedValue(), minusUnsafeInt1.GetTaggedValue());
    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);
    EXPECT_TRUE(result3);
}

/**
 * @tc.name: InitializationZero
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, InitializationZero)
{
    CString maxSafeIntPlusOneStr = "9007199254740992";
    JSHandle<BigInt> maxSafeIntPlusOne = BigIntHelper::SetBigInt(thread, maxSafeIntPlusOneStr);
    uint32_t size = maxSafeIntPlusOne->GetLength();
    uint32_t countZero = 0;
    for (uint32_t i = 0; i < size; i++) {
        uint32_t digit = maxSafeIntPlusOne->GetDigit(i);
        if (digit == 0) {
            countZero++;
        }
    }
    EXPECT_NE(countZero, size);

    maxSafeIntPlusOne->InitializationZero();
    for (uint32_t i = 0; i < size; i++) {
        uint32_t digit = maxSafeIntPlusOne->GetDigit(i);
        EXPECT_EQ(digit, 0U);
    }
}

/**
 * @tc.name: BitwiseOp & BitwiseAND & BitwiseXOR & BitwiseOR & BitwiseSubOne & BitwiseAddOne & BitwiseNOT
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, Bitwise_AND_XOR_OR_NOT_SubOne_AddOne)
{
    CString maxSafeIntStr = "11111111111111111111111111111111111111111111111111111"; // Binary: 2 ^ 53 - 1
    CString maxSafeIntPlusOneStr = "100000000000000000000000000000000000000000000000000000"; // Binary: 2 ^ 53
    CString bigintStr1 = "111111111111111111111111111111111111111111111111111111"; // Binary: 2 ^ 54 - 1
    CString bigintStr2 = "11011100";
    JSHandle<BigInt> maxSafeInt = BigIntHelper::SetBigInt(thread, maxSafeIntStr, BigInt::BINARY);
    JSHandle<BigInt> maxSafeIntPlusOne = BigIntHelper::SetBigInt(thread, maxSafeIntPlusOneStr, BigInt::BINARY);
    JSHandle<BigInt> bigint1 = BigIntHelper::SetBigInt(thread, bigintStr1, BigInt::BINARY);
    JSHandle<BigInt> bigint2 = BigIntHelper::SetBigInt(thread, bigintStr2, BigInt::BINARY);
    JSHandle<BigInt> bigint3 = BigInt::UnaryMinus(thread, bigint2);
    JSHandle<BigInt> bigint4 = BigInt::UnaryMinus(thread, bigint1);
    // Bitwise AND operation
    JSHandle<BigInt> addOpRes = BigInt::BitwiseOp(thread, Operate::AND, maxSafeIntPlusOne, bigint1);
    JSHandle<BigInt> andRes = BigInt::BitwiseAND(thread, maxSafeIntPlusOne, bigint1);
    EXPECT_TRUE(BigInt::Equal(addOpRes.GetTaggedValue(), maxSafeIntPlusOne.GetTaggedValue()));
    EXPECT_TRUE(BigInt::Equal(andRes.GetTaggedValue(), maxSafeIntPlusOne.GetTaggedValue()));

    JSHandle<BigInt> addOpRes1 = BigInt::BitwiseOp(thread, Operate::AND, bigint1, bigint2);
    JSHandle<BigInt> andRes1 = BigInt::BitwiseAND(thread, bigint1, bigint2);
    EXPECT_TRUE(BigInt::Equal(addOpRes1.GetTaggedValue(), bigint2.GetTaggedValue()));
    EXPECT_TRUE(BigInt::Equal(andRes1.GetTaggedValue(), bigint2.GetTaggedValue()));

    JSHandle<BigInt> addOpRes2 = BigInt::BitwiseOp(thread, Operate::AND, bigint2, bigint1);
    JSHandle<BigInt> andRes2 = BigInt::BitwiseAND(thread, bigint2, bigint1);
    EXPECT_TRUE(BigInt::Equal(addOpRes2.GetTaggedValue(), bigint2.GetTaggedValue()));
    EXPECT_TRUE(BigInt::Equal(andRes2.GetTaggedValue(), bigint2.GetTaggedValue()));

    CString bigintStr4 = "111111111111111111111111111111111111111111111100100100";
    JSHandle<BigInt> bigint = BigIntHelper::SetBigInt(thread, bigintStr4, BigInt::BINARY);
    JSHandle<BigInt> andRes3 = BigInt::BitwiseAND(thread, bigint3, bigint1);
    EXPECT_TRUE(BigInt::Equal(andRes3.GetTaggedValue(), bigint.GetTaggedValue()));

    JSHandle<BigInt> andRes4 = BigInt::BitwiseAND(thread, bigint1, bigint3);
    EXPECT_TRUE(BigInt::Equal(andRes4.GetTaggedValue(), bigint.GetTaggedValue()));

    CString bigintStr5 = "-1000000000000000000000000000000000000000000000000000000";
    JSHandle<BigInt> bigint5 = BigIntHelper::SetBigInt(thread, bigintStr5, BigInt::BINARY);
    JSHandle<BigInt> andRes5 = BigInt::BitwiseAND(thread, bigint3, bigint4);
    EXPECT_TRUE(BigInt::Equal(andRes5.GetTaggedValue(), bigint5.GetTaggedValue()));

    CString bigintStr6 = "-1000000000000000000000000000000000000000000000000000000";
    JSHandle<BigInt> bigint6 = BigIntHelper::SetBigInt(thread, bigintStr6, BigInt::BINARY);
    JSHandle<BigInt> andRes6 = BigInt::BitwiseAND(thread, bigint4, bigint3);
    EXPECT_TRUE(BigInt::Equal(andRes6.GetTaggedValue(), bigint6.GetTaggedValue()));

    // Bitwise OR operation
    JSHandle<BigInt> orOpRes = BigInt::BitwiseOp(thread, Operate::OR, maxSafeInt, maxSafeIntPlusOne);
    JSHandle<BigInt> orRes = BigInt::BitwiseOR(thread, maxSafeInt, maxSafeIntPlusOne);
    EXPECT_TRUE(BigInt::Equal(orOpRes.GetTaggedValue(), bigint1.GetTaggedValue()));
    EXPECT_TRUE(BigInt::Equal(orRes.GetTaggedValue(), bigint1.GetTaggedValue()));

    JSHandle<BigInt> orRes1 = BigInt::BitwiseOR(thread, bigint3, maxSafeIntPlusOne);
    EXPECT_TRUE(BigInt::Equal(orRes1.GetTaggedValue(), bigint3.GetTaggedValue()));

    JSHandle<BigInt> orRes2 = BigInt::BitwiseOR(thread, maxSafeIntPlusOne, bigint3);
    EXPECT_TRUE(BigInt::Equal(orRes2.GetTaggedValue(), bigint3.GetTaggedValue()));

    CString bigintStr7 = "-11011011";
    JSHandle<BigInt> bigint7 = BigIntHelper::SetBigInt(thread, bigintStr7, BigInt::BINARY);
    JSHandle<BigInt> orRes3 = BigInt::BitwiseOR(thread, bigint3, bigint4);
    EXPECT_TRUE(BigInt::Equal(orRes3.GetTaggedValue(), bigint7.GetTaggedValue()));

    // Bitwise XOR operation
    JSHandle<BigInt> xorOpRes = BigInt::BitwiseOp(thread, Operate::XOR, maxSafeIntPlusOne, bigint1);
    JSHandle<BigInt> xorRes = BigInt::BitwiseXOR(thread, maxSafeIntPlusOne, bigint1);
    EXPECT_TRUE(BigInt::Equal(xorOpRes.GetTaggedValue(), maxSafeInt.GetTaggedValue()));
    EXPECT_TRUE(BigInt::Equal(xorRes.GetTaggedValue(), maxSafeInt.GetTaggedValue()));

    CString bigintStr8 = "-100000000000000000000000000000000000000000000011011100";
    JSHandle<BigInt> bigint8 = BigIntHelper::SetBigInt(thread, bigintStr8, BigInt::BINARY);
    JSHandle<BigInt> xorRes1 = BigInt::BitwiseXOR(thread, bigint3, maxSafeIntPlusOne);
    EXPECT_TRUE(BigInt::Equal(xorRes1.GetTaggedValue(), bigint8.GetTaggedValue()));

    JSHandle<BigInt> xorRes2 = BigInt::BitwiseXOR(thread, maxSafeIntPlusOne, bigint3);
    EXPECT_TRUE(BigInt::Equal(xorRes2.GetTaggedValue(), bigint8.GetTaggedValue()));

    CString bigintStr9 = "111111111111111111111111111111111111111111111100100101";
    JSHandle<BigInt> bigint9 = BigIntHelper::SetBigInt(thread, bigintStr9, BigInt::BINARY);
    JSHandle<BigInt> xorRes3 = BigInt::BitwiseXOR(thread, bigint3, bigint4);
    EXPECT_TRUE(BigInt::Equal(xorRes3.GetTaggedValue(), bigint9.GetTaggedValue()));

    // Bitwise NOT operation, include sign bits.
    JSHandle<BigInt> notRes1 = BigInt::BitwiseNOT(thread, maxSafeInt);
    JSHandle<BigInt> minusMaxSafeInt = BigInt::UnaryMinus(thread, maxSafeIntPlusOne);
    // ~x == -x-1 == -(x+1)
    EXPECT_TRUE(BigInt::Equal(notRes1.GetTaggedValue(), minusMaxSafeInt.GetTaggedValue()));
    JSHandle<BigInt> notRes2 = BigInt::BitwiseNOT(thread, minusMaxSafeInt);
    // ~(-x) == ~(~(x-1)) == x-1
    EXPECT_TRUE(BigInt::Equal(notRes2.GetTaggedValue(), maxSafeInt.GetTaggedValue()));

    // Bitwise sub one operation, include sign bits.
    uint32_t maxSize = maxSafeIntPlusOne->GetLength();
    JSHandle<BigInt> subOneRes = BigInt::BitwiseSubOne(thread, maxSafeIntPlusOne, maxSize);
    EXPECT_TRUE(BigInt::Equal(subOneRes.GetTaggedValue(), maxSafeInt.GetTaggedValue()));

    // Bitwise add one operation, include sign bits.
    JSHandle<BigInt> addOneRes = BigInt::BitwiseAddOne(thread, maxSafeInt);
    JSHandle<BigInt> minusMaxSafePlusOneInt = BigInt::UnaryMinus(thread, maxSafeIntPlusOne);
    EXPECT_TRUE(BigInt::Equal(addOneRes.GetTaggedValue(), minusMaxSafePlusOneInt.GetTaggedValue()));

    JSHandle<BigInt> newBigint = BigInt::CreateBigint(thread, 2);
    newBigint->SetDigit(0, std::numeric_limits<uint32_t>::max());
    newBigint->SetDigit(1, std::numeric_limits<uint32_t>::max());
    JSHandle<BigInt> addOneRes1 = BigInt::BitwiseAddOne(thread, newBigint);
    addOneRes1->SetSign(false);
    JSHandle<BigInt> newBigint1 = BigInt::CreateBigint(thread, 3);
    newBigint1->SetDigit(0, 0);
    newBigint1->SetDigit(1, 0);
    newBigint1->SetDigit(2, 1);
    EXPECT_TRUE(BigInt::Equal(addOneRes1.GetTaggedValue(), newBigint1.GetTaggedValue()));
}

/**
 * @tc.name: ToString & ToStdString
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, ToString_ToStdString)
{
    CString bigintStdStr1 = "111111111111111111111111111111111111111111111111111111"; // Binary: 2 ^ 54 - 1
    CString bigintStdStr2 = "1234567890987654321"; // Decimal
    JSHandle<BigInt> bigint1 = BigIntHelper::SetBigInt(thread, bigintStdStr1, BigInt::BINARY);
    JSHandle<BigInt> bigint2 = BigIntHelper::SetBigInt(thread, bigintStdStr2, BigInt::DECIMAL);

    JSHandle<EcmaString> bigintEcmaStrBin1 = BigInt::ToString(thread, bigint1, BigInt::BINARY);
    EXPECT_STREQ(EcmaStringAccessor(bigintEcmaStrBin1).ToCString(thread).c_str(),
        "111111111111111111111111111111111111111111111111111111");
    JSHandle<EcmaString> bigintEcmaStrOct1 = BigInt::ToString(thread, bigint1, BigInt::OCTAL);
    EXPECT_STREQ(EcmaStringAccessor(bigintEcmaStrOct1).ToCString(thread).c_str(), "777777777777777777");
    JSHandle<EcmaString> bigintEcmaStrDec1 = BigInt::ToString(thread, bigint1, BigInt::DECIMAL);
    EXPECT_STREQ(EcmaStringAccessor(bigintEcmaStrDec1).ToCString(thread).c_str(), "18014398509481983");
    JSHandle<EcmaString> bigintEcmaStrHex1 = BigInt::ToString(thread, bigint1, BigInt::HEXADECIMAL);
    EXPECT_STREQ(EcmaStringAccessor(bigintEcmaStrHex1).ToCString(thread).c_str(), "3fffffffffffff");

    JSHandle<EcmaString> bigintEcmaStrBin2 = BigInt::ToString(thread, bigint2, BigInt::BINARY);
    EXPECT_STREQ(EcmaStringAccessor(bigintEcmaStrBin2).ToCString(thread).c_str(),
        "1000100100010000100001111010010110001011011000001110010110001");
    EXPECT_STREQ(EcmaStringAccessor(bigintEcmaStrBin2).ToCString(thread).c_str(),
        (bigint2->ToStdString(BigInt::BINARY)).c_str());

    JSHandle<EcmaString> bigintEcmaStrOct2 = BigInt::ToString(thread, bigint2, BigInt::OCTAL);
    EXPECT_STREQ(EcmaStringAccessor(bigintEcmaStrOct2).ToCString(thread).c_str(), "104420417226133016261");
    EXPECT_STREQ(EcmaStringAccessor(bigintEcmaStrOct2).ToCString(thread).c_str(),
        (bigint2->ToStdString(BigInt::OCTAL)).c_str());

    JSHandle<EcmaString> bigintEcmaStrDec2 = BigInt::ToString(thread, bigint2, BigInt::DECIMAL);
    EXPECT_STREQ(EcmaStringAccessor(bigintEcmaStrDec2).ToCString(thread).c_str(), "1234567890987654321");
    EXPECT_STREQ(EcmaStringAccessor(bigintEcmaStrDec2).ToCString(thread).c_str(),
        (bigint2->ToStdString(BigInt::DECIMAL)).c_str());

    JSHandle<EcmaString> bigintEcmaStrHex2 = BigInt::ToString(thread, bigint2, BigInt::HEXADECIMAL);
    EXPECT_STREQ(EcmaStringAccessor(bigintEcmaStrHex2).ToCString(thread).c_str(), "112210f4b16c1cb1");
    EXPECT_STREQ(EcmaStringAccessor(bigintEcmaStrHex2).ToCString(thread).c_str(),
        (bigint2->ToStdString(BigInt::HEXADECIMAL)).c_str());
}

/**
 * @tc.name: UnaryMinus
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, UnaryMinus)
{
    CString maxSafeIntStr = "9007199254740991";
    CString minSafeIntStr = "-9007199254740991";
    CString maxSafeIntPlusOneStr = "9007199254740992";
    CString minSafeIntSubOneStr = "-9007199254740992";
    JSHandle<BigInt> maxSafeInt = BigIntHelper::SetBigInt(thread, maxSafeIntStr);
    JSHandle<BigInt> minSafeInt = BigIntHelper::SetBigInt(thread, minSafeIntStr);
    JSHandle<BigInt> maxSafeIntPlusOne = BigIntHelper::SetBigInt(thread, maxSafeIntPlusOneStr);
    JSHandle<BigInt> minSafeIntSubOne = BigIntHelper::SetBigInt(thread, minSafeIntSubOneStr);

    JSHandle<BigInt> minusRes1 = BigInt::UnaryMinus(thread, maxSafeInt);
    EXPECT_TRUE(BigInt::Equal(minusRes1.GetTaggedValue(), minSafeInt.GetTaggedValue()));
    JSHandle<BigInt> minusRes2 = BigInt::UnaryMinus(thread, minSafeInt);
    EXPECT_TRUE(BigInt::Equal(minusRes2.GetTaggedValue(), maxSafeInt.GetTaggedValue()));
    JSHandle<BigInt> minusRes3 = BigInt::UnaryMinus(thread, maxSafeIntPlusOne);
    EXPECT_TRUE(BigInt::Equal(minusRes3.GetTaggedValue(), minSafeIntSubOne.GetTaggedValue()));
    JSHandle<BigInt> minusRes4 = BigInt::UnaryMinus(thread, minSafeIntSubOne);
    EXPECT_TRUE(BigInt::Equal(minusRes4.GetTaggedValue(), maxSafeIntPlusOne.GetTaggedValue()));

    JSHandle<BigInt> zero = BigInt::Int32ToBigInt(thread, 0);
    JSHandle<BigInt> minusRes5 = BigInt::UnaryMinus(thread, zero);
    EXPECT_TRUE(!minusRes5->GetSign());
    EXPECT_TRUE(BigInt::Equal(zero.GetTaggedValue(), minusRes5.GetTaggedValue()));
}

/**
 * @tc.name: Exponentiate & Multiply & Divide & Remainder
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, Exponentiate_Multiply_Divide_Remainder)
{
    CString baseBigintStr = "2";
    CString expBigintStr1 = "53";
    CString expBigintStr2 = "54";
    CString resBigintStr1 = "9007199254740992"; // 2 ^ 53
    CString resBigintStr2 = "18014398509481984"; // 2 ^ 54
    CString resBigintStr3 = "162259276829213363391578010288128"; // 2 ^ 107
    CString resBigintStr4 = "162259276829213363391578010288182"; // 2 ^ 107 + 54
    JSHandle<BigInt> baseBigint = BigIntHelper::SetBigInt(thread, baseBigintStr);
    JSHandle<BigInt> expBigint1 = BigIntHelper::SetBigInt(thread, expBigintStr1);
    JSHandle<BigInt> expBigint2 = BigIntHelper::SetBigInt(thread, expBigintStr2);
    JSHandle<BigInt> resBigint1 = BigIntHelper::SetBigInt(thread, resBigintStr1);
    JSHandle<BigInt> resBigint2 = BigIntHelper::SetBigInt(thread, resBigintStr2);
    JSHandle<BigInt> resBigint3 = BigIntHelper::SetBigInt(thread, resBigintStr3);
    JSHandle<BigInt> resBigint4 = BigIntHelper::SetBigInt(thread, resBigintStr4);
    JSHandle<BigInt> resBigint5 = BigInt::Int32ToBigInt(thread, -1);
    JSHandle<BigInt> zero = BigInt::Int32ToBigInt(thread, 0);
    // Exponentiate
    JSHandle<BigInt> expRes1 = BigInt::Exponentiate(thread, baseBigint, expBigint1);
    EXPECT_TRUE(BigInt::Equal(expRes1.GetTaggedValue(), resBigint1.GetTaggedValue()));
    JSHandle<BigInt> expRes2 = BigInt::Exponentiate(thread, baseBigint, expBigint2);
    EXPECT_TRUE(BigInt::Equal(expRes2.GetTaggedValue(), resBigint2.GetTaggedValue()));
    JSHandle<BigInt> expRes3 = BigInt::Exponentiate(thread, baseBigint, resBigint5);
    EXPECT_TRUE(expRes3.GetTaggedValue().IsException());
    thread->ClearException();
    // Multiply
    JSHandle<BigInt> mulRes1 = BigInt::Multiply(thread, baseBigint, baseBigint);
    for (int32_t i = 0; i < atoi(expBigintStr1.c_str()) - 2; i++) {
        mulRes1 = BigInt::Multiply(thread, mulRes1, baseBigint);
    }
    EXPECT_TRUE(BigInt::Equal(mulRes1.GetTaggedValue(), resBigint1.GetTaggedValue()));
    JSHandle<BigInt> mulRes2 = BigInt::Multiply(thread, baseBigint, baseBigint);
    for (int32_t i = 0; i < atoi(expBigintStr2.c_str()) - 2; i++) {
        mulRes2 = BigInt::Multiply(thread, mulRes2, baseBigint);
    }
    EXPECT_TRUE(BigInt::Equal(mulRes2.GetTaggedValue(), resBigint2.GetTaggedValue()));
    JSHandle<BigInt> mulRes3 = BigInt::Multiply(thread, resBigint1, resBigint2);
    EXPECT_TRUE(BigInt::Equal(mulRes3.GetTaggedValue(), resBigint3.GetTaggedValue()));

    // Divide
    // The result has no remainder.
    JSHandle<BigInt> divRes1 = BigInt::Divide(thread, resBigint3, resBigint2);
    EXPECT_TRUE(BigInt::Equal(divRes1.GetTaggedValue(), resBigint1.GetTaggedValue()));
    JSHandle<BigInt> divRes2 = BigInt::Divide(thread, resBigint3, resBigint1);
    EXPECT_TRUE(BigInt::Equal(divRes2.GetTaggedValue(), resBigint2.GetTaggedValue()));
    // The result has a remainder.
    JSHandle<BigInt> divRes3 = BigInt::Divide(thread, resBigint4, resBigint1);
    EXPECT_TRUE(BigInt::Equal(divRes3.GetTaggedValue(), resBigint2.GetTaggedValue()));
    JSHandle<BigInt> divRes4 = BigInt::Divide(thread, resBigint4, resBigint2);
    EXPECT_TRUE(BigInt::Equal(divRes4.GetTaggedValue(), resBigint1.GetTaggedValue()));
    JSHandle<BigInt> divRes5 = BigInt::Divide(thread, baseBigint, zero);
    EXPECT_TRUE(divRes5.GetTaggedValue().IsException());
    thread->ClearException();
    JSHandle<BigInt> divRes6 = BigInt::Divide(thread, expBigint2, baseBigint);
    JSHandle<BigInt> expectRes6 = BigInt::Int32ToBigInt(thread, 27); // 27 : Expected calculation results
    EXPECT_TRUE(BigInt::Equal(divRes6.GetTaggedValue(), expectRes6.GetTaggedValue()));
    JSHandle<BigInt> divRes7 = BigInt::Divide(thread, expBigint1, baseBigint);
    JSHandle<BigInt> expectRes7 = BigInt::Int32ToBigInt(thread, 26); // 26 : Expected calculation results
    EXPECT_TRUE(BigInt::Equal(divRes7.GetTaggedValue(), expectRes7.GetTaggedValue()));

    // Remainder
    JSHandle<BigInt> remRes1 = BigInt::Remainder(thread, resBigint4, resBigint1);
    EXPECT_TRUE(BigInt::Equal(remRes1.GetTaggedValue(), expBigint2.GetTaggedValue()));
    JSHandle<BigInt> remRes2 = BigInt::Remainder(thread, resBigint4, resBigint2);
    EXPECT_TRUE(BigInt::Equal(remRes2.GetTaggedValue(), expBigint2.GetTaggedValue()));
    JSHandle<BigInt> remRes3 = BigInt::Remainder(thread, resBigint4, resBigint3);
    EXPECT_TRUE(BigInt::Equal(remRes3.GetTaggedValue(), expBigint2.GetTaggedValue()));
    JSHandle<BigInt> remRes4 = BigInt::Remainder(thread, baseBigint, zero);
    EXPECT_TRUE(remRes4.GetTaggedValue().IsException());
    thread->ClearException();
    JSHandle<BigInt> remRes5 = BigInt::Remainder(thread, expBigint2, baseBigint);
    EXPECT_TRUE(BigInt::Equal(remRes5.GetTaggedValue(), zero.GetTaggedValue()));
    JSHandle<BigInt> remRes6 = BigInt::Remainder(thread, expBigint1, baseBigint);
    JSHandle<BigInt> expect = BigInt::Int32ToBigInt(thread, 1);
    EXPECT_TRUE(BigInt::Equal(remRes6.GetTaggedValue(), expect.GetTaggedValue()));
}

/**
 * @tc.name: ToInt64
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, ToInt64)
{
    CString resBigintStr1 = std::to_string(LLONG_MAX).c_str();
    CString resBigintStr2 = std::to_string(LLONG_MIN).c_str();
    CString resBigintStr3 = std::to_string(INT_MAX).c_str();
    CString resBigintStr4 = std::to_string(INT_MIN).c_str();
    CString resBigintStr5 = "0";

    JSHandle<BigInt> resBigint1 = BigIntHelper::SetBigInt(thread, resBigintStr1);
    JSHandle<BigInt> resBigint2 = BigIntHelper::SetBigInt(thread, resBigintStr2);
    JSHandle<BigInt> resBigint3 = BigIntHelper::SetBigInt(thread, resBigintStr3);
    JSHandle<BigInt> resBigint4 = BigIntHelper::SetBigInt(thread, resBigintStr4);
    JSHandle<BigInt> resBigint5 = BigIntHelper::SetBigInt(thread, resBigintStr5);

    EXPECT_TRUE(resBigint1->ToInt64() == LLONG_MAX);
    EXPECT_TRUE(resBigint2->ToInt64() == LLONG_MIN);
    EXPECT_TRUE(resBigint3->ToInt64() == INT_MAX);
    EXPECT_TRUE(resBigint4->ToInt64() == INT_MIN);
    EXPECT_TRUE(resBigint5->ToInt64() == 0);
}

/**
 * @tc.name: ToUint64
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, ToUint64)
{
    CString resBigintStr1 = std::to_string(ULLONG_MAX).c_str();
    CString resBigintStr2 = std::to_string(UINT_MAX).c_str();
    CString resBigintStr3 = "0";

    JSHandle<BigInt> resBigint1 = BigIntHelper::SetBigInt(thread, resBigintStr1);
    JSHandle<BigInt> resBigint2 = BigIntHelper::SetBigInt(thread, resBigintStr2);
    JSHandle<BigInt> resBigint3 = BigIntHelper::SetBigInt(thread, resBigintStr3);

    EXPECT_TRUE(resBigint1->ToUint64() == ULLONG_MAX);
    EXPECT_TRUE(resBigint2->ToUint64() == UINT_MAX);
    EXPECT_TRUE(resBigint3->ToUint64() == 0);
}

/**
 * @tc.name: Int64ToBigInt
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, Int64ToBigInt)
{
    // JSHandle<BigInt> BigInt::Int64ToBigInt(JSThread *thread, const int64_t &number)
    JSHandle<BigInt> resBigint1 = BigInt::Int64ToBigInt(thread, LLONG_MAX);
    JSHandle<BigInt> resBigint2 = BigInt::Int64ToBigInt(thread, LLONG_MIN);
    JSHandle<BigInt> resBigint3 = BigInt::Int64ToBigInt(thread, INT_MAX);
    JSHandle<BigInt> resBigint4 = BigInt::Int64ToBigInt(thread, INT_MIN);
    JSHandle<BigInt> resBigint5 = BigInt::Int64ToBigInt(thread, 0);
    JSHandle<BigInt> resBigint6 = BigInt::Int64ToBigInt(thread, 1234);
    JSHandle<BigInt> resBigint7 = BigInt::Int64ToBigInt(thread, LLONG_MAX - INT_MAX);

    EXPECT_TRUE(resBigint1->ToInt64() == LLONG_MAX);
    EXPECT_TRUE(resBigint2->ToInt64() == LLONG_MIN);
    EXPECT_TRUE(resBigint3->ToInt64() == INT_MAX);
    EXPECT_TRUE(resBigint4->ToInt64() == INT_MIN);
    EXPECT_TRUE(resBigint5->ToInt64() == 0);
    EXPECT_TRUE(resBigint6->ToInt64() == 1234);
    EXPECT_TRUE(resBigint7->ToInt64() == LLONG_MAX - INT_MAX);
}

/**
 * @tc.name: Uint64ToBigInt
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, Uint64ToBigInt)
{
    JSHandle<BigInt> resBigint1 = BigInt::Uint64ToBigInt(thread, ULLONG_MAX);
    JSHandle<BigInt> resBigint2 = BigInt::Uint64ToBigInt(thread, UINT_MAX);
    JSHandle<BigInt> resBigint3 = BigInt::Uint64ToBigInt(thread, 0);
    JSHandle<BigInt> resBigint4 = BigInt::Uint64ToBigInt(thread, 1234U);
    JSHandle<BigInt> resBigint5 = BigInt::Uint64ToBigInt(thread, ULLONG_MAX - UINT_MAX);

    EXPECT_TRUE(resBigint1->ToUint64() == ULLONG_MAX);
    EXPECT_TRUE(resBigint2->ToUint64() == UINT_MAX);
    EXPECT_TRUE(resBigint3->ToUint64() == 0);
    EXPECT_TRUE(resBigint4->ToUint64() == 1234U);
    EXPECT_TRUE(resBigint5->ToUint64() == ULLONG_MAX - UINT_MAX);
}

void GetWordsArray(bool *signBit, size_t wordCount, uint64_t *words, JSHandle<BigInt> bigintVal)
{
    uint32_t len = bigintVal->GetLength();
    uint32_t count = 0;
    uint32_t index = 0;
    for (; index < wordCount - 1; ++index) {
        words[index] = static_cast<uint64_t>(bigintVal->GetDigit(count++));
        words[index] |= static_cast<uint64_t>(bigintVal->GetDigit(count++)) << 32; // 32 : int32_t bits
    }
    if (len % 2 == 0) { // 2 : len is odd or even
        words[index] = static_cast<uint64_t>(bigintVal->GetDigit(count++));
        words[index] |= static_cast<uint64_t>(bigintVal->GetDigit(count++)) << 32; // 32 : int32_t bits
    } else {
        words[index] = static_cast<uint64_t>(bigintVal->GetDigit(count++));
    }
    *signBit = bigintVal->GetSign();
}

/**
 * @tc.name: CreateBigWords
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, CreateBigWords)
{
    size_t wordCount = 4;
    uint64_t words[] = { 0xFFFFFFFFFFFFFFFF, 34ULL, 56ULL, 0xFFFFFFFFFFFFFFFF };
    JSHandle<BigInt> bigintFalse = BigInt::CreateBigWords(thread, false, wordCount, words);
    bool sign = true;
    uint64_t wordsOut[] = { 0ULL, 0ULL, 0ULL, 0ULL };
    GetWordsArray(&sign, wordCount, wordsOut, bigintFalse);
    EXPECT_TRUE(sign == false);
    for (size_t i = 0; i < wordCount; i++) {
        EXPECT_TRUE(words[i] == wordsOut[i]);
    }

    JSHandle<BigInt> bigintTrue = BigInt::CreateBigWords(thread, true, wordCount, words);
    GetWordsArray(&sign, wordCount, wordsOut, bigintTrue);
    EXPECT_TRUE(sign == true);
    for (size_t i = 0; i < wordCount; i++) {
        EXPECT_TRUE(words[i] == wordsOut[i]);
    }

    size_t wordCount1 = 5;
    uint64_t words1[] = { 12ULL, 34ULL, 56ULL, 78ULL, 90ULL };
    JSHandle<BigInt> bigintFalse1 = BigInt::CreateBigWords(thread, false, wordCount1, words1);

    bool sign1 = true;
    uint64_t wordsOut1[] = { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL };
    GetWordsArray(&sign1, wordCount1, wordsOut1, bigintFalse1);
    EXPECT_TRUE(sign1 == false);
    for (size_t i = 0; i < wordCount1; i++) {
        EXPECT_TRUE(words1[i] == wordsOut1[i]);
    }

    size_t wordCount2 = 0;
    uint64_t words2[10] = { 0 };
    JSHandle<BigInt> bigint1 = BigInt::CreateBigWords(thread, true, wordCount2, words2);
    EXPECT_TRUE(bigint1->IsZero());

    BigInt::CreateBigWords(thread, true, INT_MAX, words2);
    EXPECT_TRUE(thread->HasPendingException());
    thread->ClearException();
}

/**
 * @tc.name: GetUint64MaxBigInt GetInt64MaxBigInt
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, GetUint64MaxBigint_GetInt64MaxBigint)
{
    JSHandle<BigInt> exponent = BigInt::Int32ToBigInt(thread, 64); // 64 : bits
    JSHandle<BigInt> exponentone = BigInt::Int32ToBigInt(thread, 63); // 63 : bits
    JSHandle<BigInt> base = BigInt::Int32ToBigInt(thread, 2); // 2 : base value
    JSHandle<BigInt> uint64MaxBigint1 = BigInt::Exponentiate(thread, base, exponent);
    JSHandle<BigInt> uint64MaxBigint2 = BigInt::GetUint64MaxBigInt(thread);
    EXPECT_TRUE(BigInt::Equal(uint64MaxBigint1.GetTaggedValue(), uint64MaxBigint2.GetTaggedValue()));
    JSHandle<BigInt> int64MaxBigint1 = BigInt::Exponentiate(thread, base, exponentone);
    JSHandle<BigInt> int64MaxBigint2 = BigInt::GetInt64MaxBigInt(thread);
    EXPECT_TRUE(BigInt::Equal(int64MaxBigint1.GetTaggedValue(), int64MaxBigint2.GetTaggedValue()));
}

/**
 * @tc.name: Int32ToBigInt
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, Int32ToBigInt)
{
    JSHandle<BigInt> resBigint1 = BigInt::Int32ToBigInt(thread, std::numeric_limits<int32_t>::max());
    JSHandle<BigInt> resBigint2 = BigInt::Int32ToBigInt(thread, std::numeric_limits<int32_t>::min());
    JSHandle<BigInt> resBigint3 = BigInt::Int32ToBigInt(thread, 0);

    EXPECT_TRUE(static_cast<int32_t>(resBigint1->GetDigit(0)) == std::numeric_limits<int32_t>::max());
    EXPECT_FALSE(resBigint1->GetSign());
    EXPECT_TRUE(static_cast<int32_t>(resBigint2->GetDigit(0)) == std::numeric_limits<int32_t>::min());
    EXPECT_TRUE(resBigint2->GetSign());
    EXPECT_TRUE(static_cast<int32_t>(resBigint3->GetDigit(0)) == 0);
    EXPECT_FALSE(resBigint3->GetSign());
}

/**
 * @tc.name: Uint32ToBigInt
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, Uint32ToBigInt)
{
    JSHandle<BigInt> resBigint1 = BigInt::Uint32ToBigInt(thread, std::numeric_limits<uint32_t>::max());
    JSHandle<BigInt> resBigint2 = BigInt::Uint32ToBigInt(thread, std::numeric_limits<uint32_t>::min());
    JSHandle<BigInt> resBigint3 = BigInt::Uint32ToBigInt(thread, 0);

    EXPECT_TRUE(resBigint1->GetDigit(0) == std::numeric_limits<uint32_t>::max());
    EXPECT_FALSE(resBigint1->GetSign());
    EXPECT_TRUE(resBigint2->GetDigit(0) == std::numeric_limits<uint32_t>::min());
    EXPECT_FALSE(resBigint2->GetSign());
    EXPECT_TRUE(resBigint3->GetDigit(0) == 0);
    EXPECT_FALSE(resBigint3->GetSign());
}

/**
 * @tc.name: BigIntToInt64
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, BigIntToInt64)
{
    JSHandle<BigInt> resBigint1 = BigInt::Int64ToBigInt(thread, LLONG_MAX);
    JSHandle<BigInt> resBigint2 = BigInt::Int64ToBigInt(thread, LLONG_MIN);
    JSHandle<BigInt> resBigint3 = BigInt::Int64ToBigInt(thread, INT_MAX);
    JSHandle<BigInt> resBigint4 = BigInt::Int64ToBigInt(thread, INT_MIN);
    JSHandle<BigInt> resBigint5 = BigInt::Int64ToBigInt(thread, 0);
    int64_t cValue = 0;
    bool lossless = false;
    BigInt::BigIntToInt64(thread, JSHandle<JSTaggedValue>(resBigint1), &cValue, &lossless);
    EXPECT_TRUE(cValue == LLONG_MAX);
    EXPECT_TRUE(lossless);
    BigInt::BigIntToInt64(thread, JSHandle<JSTaggedValue>(resBigint2), &cValue, &lossless);
    EXPECT_TRUE(cValue == LLONG_MIN);
    EXPECT_TRUE(lossless);
    BigInt::BigIntToInt64(thread, JSHandle<JSTaggedValue>(resBigint3), &cValue, &lossless);
    EXPECT_TRUE(cValue == INT_MAX);
    EXPECT_TRUE(lossless);
    BigInt::BigIntToInt64(thread, JSHandle<JSTaggedValue>(resBigint4), &cValue, &lossless);
    EXPECT_TRUE(cValue == INT_MIN);
    EXPECT_TRUE(lossless);
    BigInt::BigIntToInt64(thread, JSHandle<JSTaggedValue>(resBigint5), &cValue, &lossless);
    EXPECT_TRUE(cValue == 0);
    EXPECT_TRUE(lossless);

    JSHandle<BigInt> resBigint6 = BigInt::CreateBigint(thread, 3); // 3 : bigint length
    resBigint6->SetDigit(0, 0);
    resBigint6->SetDigit(1, 0);
    resBigint6->SetDigit(2, 1); // 2 : index
    lossless = false;
    BigInt::BigIntToInt64(thread, JSHandle<JSTaggedValue>(resBigint6), &cValue, &lossless);
    EXPECT_TRUE(cValue == 0);
    EXPECT_TRUE(!lossless);

    resBigint6->SetSign(true);
    resBigint6->SetDigit(2, 2); // 2 : index
    BigInt::BigIntToInt64(thread, JSHandle<JSTaggedValue>(resBigint6), &cValue, &lossless);
    EXPECT_TRUE(cValue == 0);
    EXPECT_TRUE(!lossless);
}

/**
 * @tc.name: BigIntToUint64
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, BigIntToUint64)
{
    JSHandle<BigInt> resBigint1 = BigInt::Uint64ToBigInt(thread, ULLONG_MAX);
    JSHandle<BigInt> resBigint2 = BigInt::Uint64ToBigInt(thread, UINT_MAX);
    JSHandle<BigInt> resBigint3 = BigInt::Uint64ToBigInt(thread, 0);

    uint64_t cValue = 0;
    bool lossless = false;
    BigInt::BigIntToUint64(thread, JSHandle<JSTaggedValue>(resBigint1), &cValue, &lossless);
    EXPECT_TRUE(cValue == ULLONG_MAX);
    EXPECT_TRUE(lossless);
    BigInt::BigIntToUint64(thread, JSHandle<JSTaggedValue>(resBigint2), &cValue, &lossless);
    EXPECT_TRUE(cValue == UINT_MAX);
    EXPECT_TRUE(lossless);
    BigInt::BigIntToUint64(thread, JSHandle<JSTaggedValue>(resBigint3), &cValue, &lossless);
    EXPECT_TRUE(cValue == 0);
    EXPECT_TRUE(lossless);

    JSHandle<BigInt> resBigint4 = BigInt::CreateBigint(thread, 3); // 3 : bigint length
    resBigint4->SetDigit(0, 0);
    resBigint4->SetDigit(1, 0);
    resBigint4->SetDigit(2, 1); // 2 : index
    lossless = false;
    BigInt::BigIntToUint64(thread, JSHandle<JSTaggedValue>(resBigint4), &cValue, &lossless);
    EXPECT_TRUE(cValue == 0);
    EXPECT_TRUE(!lossless);
}

/**
 * @tc.name: Add
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, Add)
{
    JSHandle<BigInt> resBigint = BigInt::CreateBigint(thread, 2); // 2 : bigint length
    resBigint->SetDigit(0, 0);
    resBigint->SetDigit(1, 1);
    resBigint->SetSign(true);
    JSHandle<BigInt> resBigint1 = BigInt::CreateBigint(thread, 2); // 2 : bigint length
    resBigint1->SetDigit(0, 1);
    resBigint1->SetDigit(1, 1);

    JSHandle<BigInt> addres = BigInt::Add(thread, resBigint, resBigint1);
    EXPECT_TRUE(addres->GetLength() == 1);
    EXPECT_TRUE(addres->GetDigit(0) == 1);
    JSHandle<BigInt> addres1 = BigInt::Add(thread, resBigint1, resBigint);
    EXPECT_TRUE(addres1->GetLength() == 1);
    EXPECT_TRUE(addres1->GetDigit(0) == 1);

    JSHandle<BigInt> resBigint2 = BigInt::Int32ToBigInt(thread, 1);
    JSHandle<BigInt> addres2 = BigInt::Add(thread, resBigint2, resBigint);
    EXPECT_TRUE(addres2->GetLength() == 1);
    EXPECT_TRUE(addres2->GetSign());
    EXPECT_TRUE(addres2->GetDigit(0) == std::numeric_limits<uint32_t>::max());

    JSHandle<BigInt> addres3 = BigInt::Add(thread, resBigint2, resBigint1);
    EXPECT_TRUE(addres3->GetLength() == 2); // 2 : bigint length
    EXPECT_TRUE(!addres3->GetSign());
    EXPECT_TRUE(addres3->GetDigit(0) == 2); // 2 : digit value
}

/**
 * @tc.name: Subtract
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, Subtract)
{
    JSHandle<BigInt> resBigint = BigInt::CreateBigint(thread, 2); // 2 : bigint length
    resBigint->SetDigit(0, 0);
    resBigint->SetDigit(1, 1);
    JSHandle<BigInt> resBigint1 = BigInt::CreateBigint(thread, 2); // 2 : bigint length
    resBigint1->SetDigit(0, 1);
    resBigint1->SetDigit(1, 1);

    JSHandle<BigInt> addres = BigInt::Subtract(thread, resBigint1, resBigint);
    EXPECT_TRUE(addres->GetLength() == 1);
    EXPECT_TRUE(addres->GetDigit(0) == 1);
    JSHandle<BigInt> addres1 = BigInt::Subtract(thread, resBigint1, resBigint);
    EXPECT_TRUE(addres1->GetLength() == 1);
    EXPECT_TRUE(addres1->GetDigit(0) == 1);

    JSHandle<BigInt> resBigint2 = BigInt::Int32ToBigInt(thread, -1);
    EXPECT_TRUE(resBigint2->GetSign());
    JSHandle<BigInt> addres2 = BigInt::Subtract(thread, resBigint, resBigint2);
    EXPECT_TRUE(BigInt::Equal(addres2.GetTaggedValue(), resBigint1.GetTaggedValue()));
}

/**
 * @tc.name: BigintSubOne
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, BigintSubOne)
{
    JSHandle<BigInt> resBigint = BigInt::Int32ToBigInt(thread, -1);
    EXPECT_TRUE(resBigint->GetSign());
    JSHandle<BigInt> addres = BigInt::BigintSubOne(thread, resBigint);
    EXPECT_TRUE(addres->GetSign());
    EXPECT_TRUE(addres->GetDigit(0) == 2);

    JSHandle<BigInt> resBigint1 = BigInt::Int32ToBigInt(thread, 1);
    JSHandle<BigInt> addres1 = BigInt::BigintSubOne(thread, resBigint1);
    EXPECT_TRUE(addres1->GetDigit(0) == 0);
}

/**
 * @tc.name: SignedRightShift
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, SignedRightShift)
{
    // the left operand is a positive number
    CString bigintStr1 = "111111011011110111111101111111101111111111110111111111110111111011";
    JSHandle<BigInt> bigint1 = BigIntHelper::SetBigInt(thread, bigintStr1, BigInt::BINARY);
    JSHandle<BigInt> shift1 = BigInt::Int32ToBigInt(thread, 20); // 20 : shiftBits
    JSHandle<BigInt> res1 = BigInt::SignedRightShift(thread, bigint1, shift1);

    CString expectResStr1 = "1111110110111101111111011111111011111111111101";
    JSHandle<BigInt> expectRes1 = BigIntHelper::SetBigInt(thread, expectResStr1, BigInt::BINARY);
    EXPECT_TRUE(BigInt::Equal(res1.GetTaggedValue(), expectRes1.GetTaggedValue()));

    JSHandle<BigInt> shift2 = BigInt::Int32ToBigInt(thread, 0);
    JSHandle<BigInt> res2 = BigInt::SignedRightShift(thread, bigint1, shift2);
    EXPECT_TRUE(BigInt::Equal(res2.GetTaggedValue(), bigint1.GetTaggedValue()));

    JSHandle<BigInt> res3 = BigInt::SignedRightShift(thread, shift2, bigint1);
    EXPECT_TRUE(BigInt::Equal(res3.GetTaggedValue(), shift2.GetTaggedValue()));

    JSHandle<BigInt> shift3 = BigInt::Int32ToBigInt(thread, -33); // -33 : shiftBits
    JSHandle<BigInt> res4 = BigInt::SignedRightShift(thread, bigint1, shift3);
    CString expectResStr4 =
        "111111011011110111111101111111101111111111110111111111110111111011000000000000000000000000000000000";
    JSHandle<BigInt> expectRes4 = BigIntHelper::SetBigInt(thread, expectResStr4, BigInt::BINARY);
    EXPECT_TRUE(BigInt::Equal(res4.GetTaggedValue(), expectRes4.GetTaggedValue()));

    // left operand is negative number
    JSHandle<BigInt> bigint2 = BigInt::UnaryMinus(thread, bigint1);

    CString expectResStr5 =
        "-1111110110111101111111011111111011111111111110";
    JSHandle<BigInt> expectRes5 = BigIntHelper::SetBigInt(thread, expectResStr5, BigInt::BINARY);
    JSHandle<BigInt> res5 = BigInt::SignedRightShift(thread, bigint2, shift1);
    EXPECT_TRUE(BigInt::Equal(res5.GetTaggedValue(), expectRes5.GetTaggedValue()));

    CString expectResStr6 =
        "-111111011011110111111101111111101111111111110111111111110111111011000000000000000000000000000000000";
    JSHandle<BigInt> expectRes6 = BigIntHelper::SetBigInt(thread, expectResStr6, BigInt::BINARY);
    JSHandle<BigInt> res6 = BigInt::SignedRightShift(thread, bigint2, shift3);
    EXPECT_TRUE(BigInt::Equal(res6.GetTaggedValue(), expectRes6.GetTaggedValue()));

    JSHandle<BigInt> res7 = BigInt::SignedRightShift(thread, shift3, bigint1);
    JSHandle<BigInt> expectRes7 = BigInt::Int32ToBigInt(thread, -1);
    EXPECT_TRUE(BigInt::Equal(res7.GetTaggedValue(), expectRes7.GetTaggedValue()));

    JSHandle<BigInt> shift4 = BigInt::Int32ToBigInt(thread, 65); // 65 : shiftBits
    JSHandle<BigInt> res8 = BigInt::SignedRightShift(thread, shift3, shift4);
    EXPECT_TRUE(BigInt::Equal(res8.GetTaggedValue(), expectRes7.GetTaggedValue()));
}

/**
 * @tc.name: LeftShift
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, LeftShift)
{
    // the left operand is a positive number
    CString bigintStr1 = "111111011011110111111101111111101111111111110111111111110111111011";
    JSHandle<BigInt> bigint1 = BigIntHelper::SetBigInt(thread, bigintStr1, BigInt::BINARY);
    JSHandle<BigInt> shift1 = BigInt::Int32ToBigInt(thread, 20); // 20 : shiftBits
    JSHandle<BigInt> res1 = BigInt::LeftShift(thread, bigint1, shift1);

    CString expectResStr1 =
        "11111101101111011111110111111110111111111111011111111111011111101100000000000000000000";
    JSHandle<BigInt> expectRes1 = BigIntHelper::SetBigInt(thread, expectResStr1, BigInt::BINARY);
    EXPECT_TRUE(BigInt::Equal(res1.GetTaggedValue(), expectRes1.GetTaggedValue()));

    JSHandle<BigInt> shift2 = BigInt::Int32ToBigInt(thread, 0);
    JSHandle<BigInt> res2 = BigInt::LeftShift(thread, bigint1, shift2);
    EXPECT_TRUE(BigInt::Equal(res2.GetTaggedValue(), bigint1.GetTaggedValue()));

    JSHandle<BigInt> shift3 = BigInt::Int32ToBigInt(thread, -33); // -33 : shiftBits
    JSHandle<BigInt> res4 = BigInt::LeftShift(thread, bigint1, shift3);
    CString expectResStr4 = "111111011011110111111101111111101";
    JSHandle<BigInt> expectRes4 = BigIntHelper::SetBigInt(thread, expectResStr4, BigInt::BINARY);
    EXPECT_TRUE(BigInt::Equal(res4.GetTaggedValue(), expectRes4.GetTaggedValue()));

    // left operand is negative number
    JSHandle<BigInt> bigint2 = BigInt::UnaryMinus(thread, bigint1);

    CString expectResStr5 =
        "-11111101101111011111110111111110111111111111011111111111011111101100000000000000000000";
    JSHandle<BigInt> expectRes5 = BigIntHelper::SetBigInt(thread, expectResStr5, BigInt::BINARY);
    JSHandle<BigInt> res5 = BigInt::LeftShift(thread, bigint2, shift1);
    EXPECT_TRUE(BigInt::Equal(res5.GetTaggedValue(), expectRes5.GetTaggedValue()));

    CString expectResStr6 = "-111111011011110111111101111111110";
    JSHandle<BigInt> expectRes6 = BigIntHelper::SetBigInt(thread, expectResStr6, BigInt::BINARY);
    JSHandle<BigInt> res6 = BigInt::LeftShift(thread, bigint2, shift3);
    EXPECT_TRUE(BigInt::Equal(res6.GetTaggedValue(), expectRes6.GetTaggedValue()));
}

} // namespace panda::test
