/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda::ecmascript;

namespace panda::test {
class ConstantStringTest : public BaseTestWithScope<false> {
};

/*
 * @tc.name: CreateConstantString
 * @tc.desc: Check whether the EcmaString created through calling CreateConstantString function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ConstantStringTest, CreateEmptyString)
{
    JSHandle<EcmaString> handleEcmaStrEmpty(thread, EcmaStringAccessor::CreateConstantString(instance, nullptr,
        0U, true));
    EXPECT_TRUE(EcmaStringAccessor(handleEcmaStrEmpty).IsConstantString());
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrEmpty).GetLength(), 0U);
    EXPECT_TRUE(EcmaStringAccessor(handleEcmaStrEmpty).IsUtf8());
    EXPECT_FALSE(EcmaStringAccessor(handleEcmaStrEmpty).IsUtf16());
}

/*
 * @tc.name: CreateConstantString
 * @tc.desc: Check whether the EcmaString created through calling CreateConstantString function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ConstantStringTest, CreateConstantString)
{
    uint8_t arrayU8[] = {"xyz123!@#"};
    size_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
    JSHandle<EcmaString> handleEcmaStrU8(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayU8[0], lengthEcmaStrU8, true));
    for (uint32_t i = 0; i < lengthEcmaStrU8; i++) {
        EXPECT_EQ(arrayU8[i], EcmaStringAccessor(handleEcmaStrU8).Get(i));
    }
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrU8).GetLength(), lengthEcmaStrU8);
    EXPECT_TRUE(EcmaStringAccessor(handleEcmaStrU8).IsUtf8());
    EXPECT_FALSE(EcmaStringAccessor(handleEcmaStrU8).IsUtf16());
}

/*
 * @tc.name: ObjectSize
 * @tc.desc: Check whether the value returned through calling ObjectSize function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ConstantStringTest, ObjectSize)
{
    JSHandle<EcmaString> handleEcmaStrEmpty(thread, EcmaStringAccessor::CreateConstantString(instance, nullptr,
        0U, true));
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrEmpty).ObjectSize(), ConstantString::SIZE);

    uint8_t arrayU8[] = {"abcde"};
    size_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
    JSHandle<EcmaString> handleEcmaStrU8(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayU8[0], lengthEcmaStrU8, true));
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrU8).ObjectSize(), ConstantString::SIZE);
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrU8).GetLength(), lengthEcmaStrU8);
}

/*
 * @tc.name: Compare_001
 * @tc.desc: Check whether the value returned through calling Compare function between EcmaStrings made by
 * CreateFromUtf8() is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ConstantStringTest, Compare_001)
{
    // Compare(). Between EcmaStrings made by CreateConstantString().
    uint8_t arrayU8No1[3] = {1, 23};
    uint8_t arrayU8No2[4] = {1, 23, 49};
    uint8_t arrayU8No3[6] = {1, 23, 45, 97, 127};
    uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
    uint32_t lengthEcmaStrU8No2 = sizeof(arrayU8No2) - 1;
    uint32_t lengthEcmaStrU8No3 = sizeof(arrayU8No3) - 1;
    JSHandle<EcmaString> handleEcmaStrU8No1(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayU8No1[0], lengthEcmaStrU8No1, true));
    JSHandle<EcmaString> handleEcmaStrU8No2(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayU8No2[0], lengthEcmaStrU8No2, true));
    JSHandle<EcmaString> handleEcmaStrU8No3(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayU8No3[0], lengthEcmaStrU8No3, true));
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU8No1, handleEcmaStrU8No2), -1);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU8No2, handleEcmaStrU8No1), 1);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU8No2, handleEcmaStrU8No3), 49 - 45);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU8No3, handleEcmaStrU8No2), 45 - 49);
}

/*
 * @tc.name: Compare_005
 * @tc.desc: Check whether the value returned through calling Compare function between EcmaString made by
 * CreateFromUtf8() and EcmaString made by CreateFromUtf16( , , , false) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ConstantStringTest, Compare_005)
{
    // Compare(). EcmaString made by CreateFromUtf8() and EcmaString made by CreateFromUtf16( , , , false).
    uint8_t arrayU8No1[3] = {1, 23};
    uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
    JSHandle<EcmaString> handleEcmaStrU8No1(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayU8No1[0], lengthEcmaStrU8No1, true));

    uint8_t arrayU8No2[4] = {1, 23, 49};
    uint32_t lengthEcmaStrU8No2 = sizeof(arrayU8No2) - 1;
    JSHandle<EcmaString> handleEcmaStrU8No2(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayU8No2[0], lengthEcmaStrU8No2, true));

    uint16_t arrayU16NotCompNo1[] = {1, 23};
    uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
    JSHandle<EcmaString> handleEcmaStrU16NotCompNo1(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1, true));

    uint16_t arrayU16NotCompNo2[] = {1, 23, 49};
    uint32_t lengthEcmaStrU16NotCompNo2 = sizeof(arrayU16NotCompNo2) / sizeof(arrayU16NotCompNo2[0]);
    JSHandle<EcmaString> handleEcmaStrU16NotCompNo2(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotCompNo2[0], lengthEcmaStrU16NotCompNo2, true));
    
    uint16_t arrayU16NotCompNo3[] = {1, 23, 456, 6789, 65535, 127};
    uint32_t lengthEcmaStrU16NotCompNo3 = sizeof(arrayU16NotCompNo3) / sizeof(arrayU16NotCompNo3[0]);
    JSHandle<EcmaString> handleEcmaStrU16NotCompNo3(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotCompNo3[0], lengthEcmaStrU16NotCompNo3, false));

    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU8No1, handleEcmaStrU16NotCompNo1), 0);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16NotCompNo1, handleEcmaStrU8No1), 0);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU8No1, handleEcmaStrU16NotCompNo2), -1);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16NotCompNo2, handleEcmaStrU8No1), 1);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU8No2, handleEcmaStrU16NotCompNo3), 49 - 456);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16NotCompNo3, handleEcmaStrU8No2), 456 - 49);
}

/*
 * @tc.name: Concat_001
 * @tc.desc: Check whether the EcmaString returned through calling Concat function between EcmaString made by
 * CreateConstantString() and EcmaString made by CreateConstantString() is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ConstantStringTest, Concat_001)
{
    // Concat(). EcmaString made by CreateConstantString() and EcmaString made by CreateConstantString().
    uint8_t arrayFrontU8[] = {"abcdef"};
    uint8_t arrayBackU8[] = {"ABCDEF"};
    uint32_t lengthEcmaStrFrontU8 = sizeof(arrayFrontU8) - 1;
    uint32_t lengthEcmaStrBackU8 = sizeof(arrayBackU8) - 1;
    JSHandle<EcmaString> handleEcmaStrFrontU8(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayFrontU8[0], lengthEcmaStrFrontU8, true));
    JSHandle<EcmaString> handleEcmaStrBackU8(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayBackU8[0], lengthEcmaStrBackU8, true));
    JSHandle<EcmaString> handleEcmaStrConcatU8(thread,
        EcmaStringAccessor::Concat(instance, handleEcmaStrFrontU8, handleEcmaStrBackU8));
    EXPECT_TRUE(EcmaStringAccessor(handleEcmaStrConcatU8).IsUtf8());
    for (uint32_t i = 0; i < lengthEcmaStrFrontU8; i++) {
        EXPECT_EQ(EcmaStringAccessor(handleEcmaStrConcatU8).Get(i), arrayFrontU8[i]);
    }
    for (uint32_t i = 0; i < lengthEcmaStrBackU8; i++) {
        EXPECT_EQ(EcmaStringAccessor(handleEcmaStrConcatU8).Get(i + lengthEcmaStrFrontU8), arrayBackU8[i]);
    }
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrConcatU8).GetLength(), lengthEcmaStrFrontU8 + lengthEcmaStrBackU8);
}

/*
 * @tc.name: Concat_003
 * @tc.desc: Check whether the EcmaString returned through calling Concat function between EcmaString made by
 * CreateFromUtf8() and EcmaString made by CreateFromUtf16( , , , false) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ConstantStringTest, Concat_003)
{
    // Concat(). EcmaString made by CreateFromUtf8() and EcmaString made by CreateFromUtf16( , , , false).
    uint8_t arrayFrontU8[] = {"abcdef"};
    uint16_t arrayBackU16NotComp[] = {88, 768, 1, 270, 345, 333};
    uint32_t lengthEcmaStrFrontU8 = sizeof(arrayFrontU8) - 1;
    uint32_t lengthEcmaStrBackU16NotComp = sizeof(arrayBackU16NotComp) / sizeof(arrayBackU16NotComp[0]);
    JSHandle<EcmaString> handleEcmaStrFrontU8(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayFrontU8[0], lengthEcmaStrFrontU8, true));
    JSHandle<EcmaString> handleEcmaStrBackU16NotComp(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayBackU16NotComp[0], lengthEcmaStrBackU16NotComp, false));
    JSHandle<EcmaString> handleEcmaStrConcatU8U16NotComp(thread,
        EcmaStringAccessor::Concat(instance, handleEcmaStrFrontU8, handleEcmaStrBackU16NotComp));
    EXPECT_TRUE(EcmaStringAccessor(handleEcmaStrConcatU8U16NotComp).IsUtf16());
    for (uint32_t i = 0; i < lengthEcmaStrFrontU8; i++) {
        EXPECT_EQ(EcmaStringAccessor(handleEcmaStrConcatU8U16NotComp).Get(i), arrayFrontU8[i]);
    }
    for (uint32_t i = 0; i < lengthEcmaStrBackU16NotComp; i++) {
        EXPECT_EQ(EcmaStringAccessor(handleEcmaStrConcatU8U16NotComp).Get(i + lengthEcmaStrFrontU8),
            arrayBackU16NotComp[i]);
    }
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrConcatU8U16NotComp).GetLength(),
        lengthEcmaStrFrontU8 + lengthEcmaStrBackU16NotComp);
}

/*
 * @tc.name: FastSubString_001
 * @tc.desc: Check whether the EcmaString returned through calling FastSubString function from EcmaString made by
 * CreateConstantString() is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ConstantStringTest, FastSubString_001)
{
    // FastSubString(). From EcmaString made by CreateConstantString().
    std::vector<uint8_t> arrayU8{3, 7, 19, 54, 99};
    auto func = std::bind(EcmaStringAccessor::CreateConstantString, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4, MemSpaceType::SHARED_OLD_SPACE, 0);
    EcmaTestCommon::FastSubStringCommonCase(thread, instance, arrayU8, func);
}

/*
 * @tc.name: GetUtf8Length
 * @tc.desc: Check whether the value returned through calling GetUtf8Length function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ConstantStringTest, GetUtf8Length)
{
    uint8_t arrayU8[6] = {3, 7, 19, 54, 99};
    uint32_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
    JSHandle<EcmaString> handleEcmaStrU8(thread,
    EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8[0], lengthEcmaStrU8, true));
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrU8).GetUtf8Length(), lengthEcmaStrU8 + 1);
}

/*
 * @tc.name: GetDataUtf8
 * @tc.desc: Check whether the pointer returned through calling GetDataUtf8 function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ConstantStringTest, GetDataUtf8)
{
    // From EcmaString made by CreateFromUtf8().
    uint8_t arrayU8[] = {"abcde"};
    uint32_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
    JSHandle<EcmaString> handleEcmaStrU8(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayU8[0], lengthEcmaStrU8, true));
    for (uint32_t i = 0; i < lengthEcmaStrU8; i++) {
        EXPECT_EQ(*(EcmaStringAccessor(handleEcmaStrU8).GetDataUtf8() + i), arrayU8[i]);
    }
}

/*
 * @tc.name: IndexOf_001
 * @tc.desc: Check whether the value returned through a source EcmaString made by CreateConstantString() calling
 * IndexOf function with a target EcmaString made by CreateConstantString() is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ConstantStringTest, IndexOf_001)
{
    // IndexOf(). Find EcmaString made by CreateConstantString() From EcmaString made by CreateConstantString().
    uint8_t arrayU8From[7] = {23, 25, 1, 3, 39, 80};
    uint8_t arrayU8Target[4] = {1, 3, 39};
    uint32_t lengthEcmaStrU8From = sizeof(arrayU8From) - 1;
    uint32_t lengthEcmaStrU8Target = sizeof(arrayU8Target) - 1;
    JSHandle<EcmaString> handleEcmaStr(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayU8From[0], lengthEcmaStrU8From, true));
    JSHandle<EcmaString> handleEcmaStr1(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayU8Target[0], lengthEcmaStrU8Target, true));
    int32_t posStart = 0;
    EXPECT_EQ(EcmaStringAccessor::IndexOf(instance, handleEcmaStr, handleEcmaStr1, posStart), 2);
    EXPECT_EQ(EcmaStringAccessor::IndexOf(instance, handleEcmaStr1, handleEcmaStr, posStart), -1);
    posStart = -1;
    EXPECT_EQ(EcmaStringAccessor::IndexOf(instance, handleEcmaStr, handleEcmaStr1, posStart), 2);
    posStart = 1;
    EXPECT_EQ(EcmaStringAccessor::IndexOf(instance, handleEcmaStr, handleEcmaStr1, posStart), 2);
    posStart = 2;
    EXPECT_EQ(EcmaStringAccessor::IndexOf(instance, handleEcmaStr, handleEcmaStr1, posStart), 2);
    posStart = 3;
    EXPECT_EQ(EcmaStringAccessor::IndexOf(instance, handleEcmaStr, handleEcmaStr1, posStart), -1);
}

/*
 * @tc.name: IndexOf_004
 * @tc.desc: Check whether the value returned through a source EcmaString made by CreateConstantString() calling
 * IndexOf function with a target EcmaString made by CreateFromUtf16() is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ConstantStringTest, IndexOf_004)
{
    // IndexOf(). Find EcmaString made by CreateFromUtf16() From EcmaString made by CreateConstantString().
    std::vector<uint16_t> ecmaStrU16NotCompTarget = {3, 39, 80};
    std::vector<uint8_t> arrayU8From{23, 25, 1, 3, 39, 80};
    auto func = std::bind(EcmaStringAccessor::CreateConstantString, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4, MemSpaceType::SHARED_OLD_SPACE, 0);
    EcmaTestCommon::IndexOfCommonCase(thread, instance, ecmaStrU16NotCompTarget, arrayU8From, func);
}

/*
 * @tc.name: StringsAreEqual_001
 * @tc.desc: Check whether the bool returned through calling StringsAreEqual function with two EcmaStrings made by
 * CreateConstantString() is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ConstantStringTest, StringsAreEqual_001)
{
    // StringsAreEqual().
    uint8_t arrayU8No1[4] = {45, 92, 78};
    uint8_t arrayU8No2[4] = {45, 92, 78};
    uint8_t arrayU8No3[5] = {45, 92, 78, 1};
    uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
    uint32_t lengthEcmaStrU8No2 = sizeof(arrayU8No2) - 1;
    uint32_t lengthEcmaStrU8No3 = sizeof(arrayU8No3) - 1;
    JSHandle<EcmaString> handleEcmaStrU8No1(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayU8No1[0], lengthEcmaStrU8No1, true));
    JSHandle<EcmaString> handleEcmaStrU8No2(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayU8No2[0], lengthEcmaStrU8No2, true));
    JSHandle<EcmaString> handleEcmaStrU8No3(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayU8No3[0], lengthEcmaStrU8No3, true));
    EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(*handleEcmaStrU8No1, *handleEcmaStrU8No2));
    EXPECT_FALSE(EcmaStringAccessor::StringsAreEqual(*handleEcmaStrU8No1, *handleEcmaStrU8No3));
    EXPECT_FALSE(EcmaStringAccessor::StringsAreEqual(*handleEcmaStrU8No3, *handleEcmaStrU8No1));
}

/*
 * @tc.name: StringsAreEqual_002
 * @tc.desc: Check whether the bool returned through calling StringsAreEqual function with a EcmaString made by
 * CreateConstantString() and a EcmaString made by CreateFromUtf16(, , , true) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ConstantStringTest, StringsAreEqual_002)
{
    // StringsAreEqual().
    uint8_t arrayU8No1[4] = {45, 92, 78};
    uint16_t arrayU16CompNo2[] = {45, 92, 78};
    uint16_t arrayU16CompNo3[] = {45, 92, 78, 1};
    uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
    uint32_t lengthEcmaStrU16CompNo2 = sizeof(arrayU16CompNo2) / sizeof(arrayU16CompNo2[0]);
    uint32_t lengthEcmaStrU16CompNo3 = sizeof(arrayU16CompNo3) / sizeof(arrayU16CompNo3[0]);
    JSHandle<EcmaString> handleEcmaStrU8No1(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayU8No1[0], lengthEcmaStrU8No1, true));
    JSHandle<EcmaString> handleEcmaStrU16CompNo2(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16CompNo2[0], lengthEcmaStrU16CompNo2, true));
    JSHandle<EcmaString> handleEcmaStrU16CompNo3(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16CompNo3[0], lengthEcmaStrU16CompNo3, true));
    EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(*handleEcmaStrU8No1, *handleEcmaStrU16CompNo2));
    EXPECT_FALSE(EcmaStringAccessor::StringsAreEqual(*handleEcmaStrU8No1, *handleEcmaStrU16CompNo3));
}

/*
 * @tc.name: StringsAreEqualUtf8_001
 * @tc.desc: Check whether the bool returned through calling StringIsEqualUint8Data function with an EcmaString made by
 * CreateConstantString() and an Array(uint8_t) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ConstantStringTest, StringsAreEqualUtf8_001)
{
    // StringIsEqualUint8Data(). EcmaString made by CreateConstantString(), Array:U8.
    uint8_t arrayU8No1[4] = {45, 92, 78};
    uint8_t arrayU8No2[5] = {45, 92, 78, 24};
    uint8_t arrayU8No3[3] = {45, 92};
    uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
    uint32_t lengthEcmaStrU8No2 = sizeof(arrayU8No2) - 1;
    uint32_t lengthEcmaStrU8No3 = sizeof(arrayU8No3) - 1;
    JSHandle<EcmaString> handleEcmaStrU8No1(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayU8No1[0], lengthEcmaStrU8No1, true));
    JSHandle<EcmaString> handleEcmaStrU8No2(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayU8No2[0], lengthEcmaStrU8No2, true));
    JSHandle<EcmaString> handleEcmaStrU8No3(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayU8No3[0], lengthEcmaStrU8No3, true));
    EXPECT_TRUE(EcmaStringAccessor::StringIsEqualUint8Data(*handleEcmaStrU8No1, &arrayU8No1[0], lengthEcmaStrU8No1,
        true));
    EXPECT_FALSE(EcmaStringAccessor::StringIsEqualUint8Data(*handleEcmaStrU8No1, &arrayU8No1[0], lengthEcmaStrU8No1,
        false));
    EXPECT_FALSE(EcmaStringAccessor::StringIsEqualUint8Data(*handleEcmaStrU8No2, &arrayU8No1[0], lengthEcmaStrU8No1,
        true));
    EXPECT_FALSE(EcmaStringAccessor::StringIsEqualUint8Data(*handleEcmaStrU8No3, &arrayU8No1[0], lengthEcmaStrU8No1,
        true));
}

/*
 * @tc.name: StringsAreEqualUtf16_001
 * @tc.desc: Check whether the bool returned through calling StringsAreEqualUtf16 function with an EcmaString made by
 * CreateConstantString() and an Array(uint16_t) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ConstantStringTest, StringsAreEqualUtf16_001)
{
    // StringsAreEqualUtf16(). EcmaString made by CreateConstantString, Array:U16(1-127).
    uint8_t arrayU8No3[3] = {45, 92};
    uint8_t arrayU8No1[4] = {45, 92, 78};
    uint8_t arrayU8No2[5] = {45, 92, 78, 24};
    uint16_t arrayU16NotCompNo1[] = {45, 92, 78};
    uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
    uint32_t lengthEcmaStrU8No2 = sizeof(arrayU8No2) - 1;
    uint32_t lengthEcmaStrU8No3 = sizeof(arrayU8No3) - 1;
    uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
    JSHandle<EcmaString> handleEcmaStrU8No1(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayU8No1[0], lengthEcmaStrU8No1, true));
    JSHandle<EcmaString> handleEcmaStrU8No2(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayU8No2[0], lengthEcmaStrU8No2, true));
    JSHandle<EcmaString> handleEcmaStrU8No3(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayU8No3[0], lengthEcmaStrU8No3, true));
    EXPECT_TRUE(
        EcmaStringAccessor::StringsAreEqualUtf16(*handleEcmaStrU8No1,
            &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1));
    EXPECT_FALSE(
        EcmaStringAccessor::StringsAreEqualUtf16(*handleEcmaStrU8No2,
            &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1));
    EXPECT_FALSE(
        EcmaStringAccessor::StringsAreEqualUtf16(*handleEcmaStrU8No3,
            &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1));
}

/*
 * @tc.name: GetHashcode_001
 * @tc.desc: Check whether the value returned through an EcmaString made by CreateConstantString() calling
 * GetHashcode function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ConstantStringTest, GetHashcode_001)
{
    // GetHashcode(). EcmaString made by CreateConstantString().
    uint8_t arrayU8[] = {"abc"};
    uint32_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
    JSHandle<EcmaString> handleEcmaStrU8(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayU8[0], lengthEcmaStrU8, true));
    uint32_t hashExpect = 0;
    for (uint32_t i = 0; i < lengthEcmaStrU8; i++) {
        hashExpect = hashExpect * 31 + arrayU8[i];
    }
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrU8).GetHashcode(), hashExpect);
}

/*
 * @tc.name: SetIsInternString
 * @tc.desc: Call SetIsInternString function, check whether the bool returned through calling IsInternString function
 * is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ConstantStringTest, SetIsInternString)
{
    uint8_t arrayU8[] = {"abc"};
    uint32_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
    JSHandle<EcmaString> handleEcmaStrU8(thread,
        EcmaStringAccessor::CreateConstantString(instance, &arrayU8[0], lengthEcmaStrU8, true));
    EXPECT_FALSE(EcmaStringAccessor(handleEcmaStrU8).IsInternString());
    EcmaStringAccessor(handleEcmaStrU8).SetInternString();
    EXPECT_TRUE(EcmaStringAccessor(handleEcmaStrU8).IsInternString());
}

/*
 * @tc.name: EqualToSplicedString
 * @tc.desc: Tests whether the source string is equal to the concatenated string.
 * is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ConstantStringTest, EqualToSplicedString)
{
    uint8_t arrayU8[] = {"Startnow"};
    uint32_t srcLengthEcmaStrU8 = sizeof(arrayU8) - 1;
    JSHandle<EcmaString> sourceString(thread,
    EcmaStringAccessor::CreateConstantString(instance, &arrayU8[0], srcLengthEcmaStrU8, true));
    uint8_t firstArrayU8[] = {"Start"};
    uint32_t firstLengthEcmaStrU8 = sizeof(firstArrayU8) - 1;
    JSHandle<EcmaString> firstString(thread,
    EcmaStringAccessor::CreateConstantString(instance, &firstArrayU8[0], firstLengthEcmaStrU8, true));
    uint8_t secondArrayU8[] = {"now"};
    uint32_t secondLengthEcmaStrU8 = sizeof(secondArrayU8) - 1;
    JSHandle<EcmaString> secondString(thread,
    EcmaStringAccessor::CreateConstantString(instance, &secondArrayU8[0], secondLengthEcmaStrU8, true));

    EXPECT_TRUE(EcmaStringAccessor(sourceString).IsUtf8());
    EXPECT_TRUE(EcmaStringAccessor(firstString).IsUtf8());
    EXPECT_TRUE(EcmaStringAccessor(secondString).IsUtf8());
    bool result =  EcmaStringAccessor(sourceString).EqualToSplicedString(*firstString, *secondString);
    EXPECT_TRUE(result);
}

/*
 * @tc.name: TryToLower
 * @tc.desc: Check whether the EcmaString created through calling TryToLower function is within expectations.
 * is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ConstantStringTest, ConvertUtf8ToLowerOrUpper)
{
    uint8_t lowerU8[] = {"aaabbbcccddd"};
    uint32_t lowerLength = sizeof(lowerU8) - 1;
    JSHandle<EcmaString> lowerStr(thread,
        EcmaStringAccessor::CreateConstantString(instance, &lowerU8[0], lowerLength, true));
    uint8_t upperU8[] = {"AAABBBCCCDDD"};
    uint32_t upperLength = sizeof(upperU8) - 1;
    JSHandle<EcmaString> upperStr(thread,
        EcmaStringAccessor::CreateConstantString(instance, &upperU8[0], upperLength, true));

    uint8_t testStr1U8[] = {"aaaBBBcccDDD"};
    uint32_t testLength1 = sizeof(testStr1U8) - 1;
    JSHandle<EcmaString> testStr1(thread,
        EcmaStringAccessor::CreateConstantString(instance, &testStr1U8[0], testLength1, true));

    uint8_t testStr2U8[] = {"AAAbbbcccDDD"};
    uint32_t testLength2 = sizeof(testStr2U8) - 1;
    JSHandle<EcmaString> testStr2(thread,
        EcmaStringAccessor::CreateConstantString(instance, &testStr2U8[0], testLength2, true));
    std::vector<JSHandle<EcmaString>> caseStrings{upperStr, testStr1, testStr2};
    EcmaTestCommon::TryLowerCommonCase(thread, instance, lowerStr, caseStrings);
}
}  // namespace panda::test