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

#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda::ecmascript;

namespace panda::test {
class LineEcmaStringTest : public BaseTestWithScope<false> {
};

/*
 * @tc.name: ComputeSizeUtf8
 * @tc.desc: Check whether the value returned through calling ComputeSizeUtf8 function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(LineEcmaStringTest, ComputeSizeUtf8)
{
    uint32_t scale = 3333;
    for (uint32_t i = BaseString::MAX_STRING_LENGTH - 1; i > scale; i = i - scale) {
        uint32_t length = i;
        EXPECT_EQ(LineEcmaString::ComputeSizeUtf8(length), length + LineString::SIZE);
    }
}

/*
 * @tc.name: ComputeSizeUtf16
 * @tc.desc: Check whether the value returned through calling ComputeSizeUtf16 function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(LineEcmaStringTest, ComputeSizeUtf16)
{
    uint32_t scale = 3333;
    for (uint32_t i = BaseString::MAX_STRING_LENGTH - 1; i > scale; i = i - scale) {
        uint32_t length = i;
        EXPECT_EQ(LineEcmaString::ComputeSizeUtf16(length), 2 * length + LineString::SIZE);
    }
}

class EcmaStringAccessorTest : public BaseTestWithScope<false> {
};

/*
 * @tc.name: CreateLineString
 * @tc.desc: Check whether the EcmaString created through calling CreateLineString function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, CreateLineString)
{
    // CreateLineString( , true, ).
    size_t sizeAllocComp = 5;
    JSHandle<EcmaString> handleEcmaStrAllocComp(thread,
        EcmaStringAccessor::CreateLineString(instance, sizeAllocComp, true));
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrAllocComp).GetLength(), sizeAllocComp);
    EXPECT_TRUE(EcmaStringAccessor(handleEcmaStrAllocComp).IsUtf8());
    EXPECT_FALSE(EcmaStringAccessor(handleEcmaStrAllocComp).IsUtf16());

    // CreateLineString( , false, ).
    size_t sizeAllocNotComp = 5;
    JSHandle<EcmaString> handleEcmaStrAllocNotComp(thread,
        EcmaStringAccessor::CreateLineString(instance, sizeAllocNotComp, false));
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrAllocNotComp).GetLength(), sizeAllocNotComp);
    EXPECT_FALSE(EcmaStringAccessor(handleEcmaStrAllocNotComp).IsUtf8());
    EXPECT_TRUE(EcmaStringAccessor(handleEcmaStrAllocNotComp).IsUtf16());
}

/*
 * @tc.name: CreateEmptyString
 * @tc.desc: Check whether the EcmaString created through calling CreateEmptyString function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, CreateEmptyString)
{
    JSHandle<EcmaString> handleEcmaStrEmpty(thread, EcmaStringAccessor::CreateEmptyString(instance));
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrEmpty).GetLength(), 0U);
    EXPECT_TRUE(EcmaStringAccessor(handleEcmaStrEmpty).IsUtf8());
    EXPECT_FALSE(EcmaStringAccessor(handleEcmaStrEmpty).IsUtf16());
}

/*
 * @tc.name: CreateFromUtf8
 * @tc.desc: Check whether the EcmaString created through calling CreateFromUtf8 function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, CreateFromUtf8)
{
    uint8_t arrayU8[] = {"xyz123!@#"};
    size_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
    JSHandle<EcmaString> handleEcmaStrU8(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8[0], lengthEcmaStrU8, true));
    for (uint32_t i = 0; i < lengthEcmaStrU8; i++) {
        EXPECT_EQ(arrayU8[i], EcmaStringAccessor(handleEcmaStrU8).Get(i));
    }
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrU8).GetLength(), lengthEcmaStrU8);
    EXPECT_TRUE(EcmaStringAccessor(handleEcmaStrU8).IsUtf8());
    EXPECT_FALSE(EcmaStringAccessor(handleEcmaStrU8).IsUtf16());
}

/*
 * @tc.name: CreateFromUtf16
 * @tc.desc: Check whether the EcmaString created through calling CreateFromUtf16 function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, CreateFromUtf16)
{
    // CreateFromUtf16( , , , true).
    uint16_t arrayU16Comp[] = {1, 23, 45, 67, 127};
    size_t lengthEcmaStrU16Comp = sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0]);
    JSHandle<EcmaString> handleEcmaStrU16Comp(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16Comp[0], lengthEcmaStrU16Comp, true));
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrU16Comp).GetLength(), lengthEcmaStrU16Comp);
    EXPECT_TRUE(EcmaStringAccessor(handleEcmaStrU16Comp).IsUtf8());
    EXPECT_FALSE(EcmaStringAccessor(handleEcmaStrU16Comp).IsUtf16());

    // CreateFromUtf16( , , , false).
    uint16_t arrayU16NotComp[] = {127, 33, 128, 12, 256, 11100, 65535};
    size_t lengthEcmaStrU16NotComp = sizeof(arrayU16NotComp) / sizeof(arrayU16NotComp[0]);
    JSHandle<EcmaString> handleEcmaStrU16NotComp(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotComp[0], lengthEcmaStrU16NotComp, false));
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrU16NotComp).GetLength(), lengthEcmaStrU16NotComp);
    EXPECT_FALSE(EcmaStringAccessor(handleEcmaStrU16NotComp).IsUtf8());
    EXPECT_TRUE(EcmaStringAccessor(handleEcmaStrU16NotComp).IsUtf16());
}

/*
 * @tc.name: Concat_001
 * @tc.desc: Check whether the EcmaString returned through calling Concat function between EcmaString made by
 * CreateFromUtf8() and EcmaString made by CreateFromUtf8() is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, Concat_001)
{
    // Concat(). EcmaString made by CreateFromUtf8() and EcmaString made by CreateFromUtf8().
    uint8_t arrayFrontU8[] = {"abcdef"};
    uint8_t arrayBackU8[] = {"ABCDEF"};
    uint32_t lengthEcmaStrFrontU8 = sizeof(arrayFrontU8) - 1;
    uint32_t lengthEcmaStrBackU8 = sizeof(arrayBackU8) - 1;
    JSHandle<EcmaString> handleEcmaStrFrontU8(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayFrontU8[0], lengthEcmaStrFrontU8, true));
    JSHandle<EcmaString> handleEcmaStrBackU8(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayBackU8[0], lengthEcmaStrBackU8, true));
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
 * @tc.name: Concat_002
 * @tc.desc: Check whether the EcmaString returned through calling Concat function between EcmaString made by
 * CreateFromUtf16( , , , false) and EcmaString made by CreateFromUtf16( , , , false) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, Concat_002)
{
    // Concat(). EcmaString made by CreateFromUtf16( , , , false) and EcmaString made by CreateFromUtf16( , , , false).
    uint16_t arrayFrontU16NotComp[] = {128, 129, 256, 11100, 65535, 100};
    uint16_t arrayBackU16NotComp[] = {88, 768, 1, 270, 345, 333};
    uint32_t lengthEcmaStrFrontU16NotComp = sizeof(arrayFrontU16NotComp) / sizeof(arrayFrontU16NotComp[0]);
    uint32_t lengthEcmaStrBackU16NotComp = sizeof(arrayBackU16NotComp) / sizeof(arrayBackU16NotComp[0]);
    JSHandle<EcmaString> handleEcmaStrFrontU16NotComp(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayFrontU16NotComp[0], lengthEcmaStrFrontU16NotComp, false));
    JSHandle<EcmaString> handleEcmaStrBackU16NotComp(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayBackU16NotComp[0], lengthEcmaStrBackU16NotComp, false));
    JSHandle<EcmaString> handleEcmaStrConcatU16NotComp(thread,
        EcmaStringAccessor::Concat(instance, handleEcmaStrFrontU16NotComp, handleEcmaStrBackU16NotComp));
    EXPECT_TRUE(EcmaStringAccessor(handleEcmaStrConcatU16NotComp).IsUtf16());
    for (uint32_t i = 0; i < lengthEcmaStrFrontU16NotComp; i++) {
        EXPECT_EQ(EcmaStringAccessor(handleEcmaStrConcatU16NotComp).Get(i), arrayFrontU16NotComp[i]);
    }
    for (uint32_t i = 0; i < lengthEcmaStrBackU16NotComp; i++) {
        EXPECT_EQ(EcmaStringAccessor(handleEcmaStrConcatU16NotComp).Get(i + lengthEcmaStrFrontU16NotComp),
            arrayBackU16NotComp[i]);
    }
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrConcatU16NotComp).GetLength(),
        lengthEcmaStrFrontU16NotComp + lengthEcmaStrBackU16NotComp);
}

/*
 * @tc.name: Concat_003
 * @tc.desc: Check whether the EcmaString returned through calling Concat function between EcmaString made by
 * CreateFromUtf8() and EcmaString made by CreateFromUtf16( , , , false) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, Concat_003)
{
    // Concat(). EcmaString made by CreateFromUtf8() and EcmaString made by CreateFromUtf16( , , , false).
    EcmaTestCommon::ConcatCommonCase2(thread, instance);
}

/*
 * @tc.name: FastSubString_001
 * @tc.desc: Check whether the EcmaString returned through calling FastSubString function from EcmaString made by
 * CreateFromUtf8() is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, FastSubString_001)
{
    // FastSubString(). From EcmaString made by CreateFromUtf8().
    std::vector<uint8_t> arrayU8{3, 7, 19, 54, 99};
    auto func = std::bind(EcmaStringAccessor::CreateFromUtf8, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4, MemSpaceType::SHARED_OLD_SPACE);
    EcmaTestCommon::FastSubStringCommonCase(thread, instance, arrayU8, func);
}

/*
 * @tc.name: FastSubString_002
 * @tc.desc: Check whether the EcmaString returned through calling FastSubString function from EcmaString made by
 * CreateFromUtf16( , , , true) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, FastSubString_002)
{
    // FastSubString(). From EcmaString made by CreateFromUtf16( , , , true).
    uint16_t arrayU16Comp[] = {1, 12, 34, 56, 127};
    uint32_t lengthEcmaStrU16Comp = sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0]);
    JSHandle<EcmaString> handleEcmaStrU16Comp(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16Comp[0], lengthEcmaStrU16Comp, true));
    uint32_t indexStartSubU16Comp = 0;
    uint32_t lengthSubU16Comp = 2;
    JSHandle<EcmaString> handleEcmaStrSubU16Comp(thread,
        EcmaStringAccessor::FastSubString(instance, handleEcmaStrU16Comp, indexStartSubU16Comp, lengthSubU16Comp));
    for (uint32_t i = 0; i < lengthSubU16Comp; i++) {
        EXPECT_EQ(EcmaStringAccessor(handleEcmaStrSubU16Comp).Get(i),
            EcmaStringAccessor(handleEcmaStrU16Comp).Get(i + indexStartSubU16Comp));
    }
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrSubU16Comp).GetLength(), lengthSubU16Comp);
}

/*
 * @tc.name: FastSubString_003
 * @tc.desc: Check whether the EcmaString returned through calling FastSubString function from EcmaString made by
 * CreateFromUtf16( , , , false) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, FastSubString_003)
{
    // FastSubString(). From EcmaString made by CreateFromUtf16( , , , false).
    uint16_t arrayU16NotComp[] = {19, 54, 256, 11100, 65535};
    uint32_t lengthEcmaStrU16NotComp = sizeof(arrayU16NotComp) / sizeof(arrayU16NotComp[0]);
    JSHandle<EcmaString> handleEcmaStrU16NotComp(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotComp[0], lengthEcmaStrU16NotComp, false));
    uint32_t indexStartSubU16NotComp = 0;
    uint32_t lengthSubU16NotComp = 2;
    JSHandle<EcmaString> handleEcmaStrSubU16NotComp(thread, EcmaStringAccessor::FastSubString(
        instance, handleEcmaStrU16NotComp, indexStartSubU16NotComp, lengthSubU16NotComp));
    for (uint32_t i = 0; i < lengthSubU16NotComp; i++) {
        EXPECT_EQ(EcmaStringAccessor(handleEcmaStrSubU16NotComp).Get(i),
            EcmaStringAccessor(handleEcmaStrU16NotComp).Get(i + indexStartSubU16NotComp));
    }
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrSubU16NotComp).GetLength(), lengthSubU16NotComp);
}

/*
 * @tc.name: FastSubString_004
 * @tc.desc: Check whether the EcmaString returned through calling FastSubString function from EcmaString
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, FastSubString_004)
{
    ObjectFactory* factory = instance->GetFactory();
    {
        JSHandle<EcmaString> sourceString = factory->NewFromUtf8("整数integer");
        JSHandle<EcmaString> tmpString = factory->NewFromASCII("integer");
        EXPECT_TRUE(EcmaStringAccessor(sourceString).IsUtf16());
        EcmaString *res = EcmaStringAccessor::FastSubString(instance, sourceString, 2, 7);
        EXPECT_TRUE(EcmaStringAccessor(res).IsUtf8());
        EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(res, *tmpString));
    }
    {
        JSHandle<EcmaString> sourceString = factory->NewFromUtf8("整数integer");
        JSHandle<EcmaString> tmpString = factory->NewFromUtf8("整数");
        EXPECT_TRUE(EcmaStringAccessor(sourceString).IsUtf16());
        EcmaString *res = EcmaStringAccessor::FastSubString(instance, sourceString, 0, 2);
        EXPECT_TRUE(EcmaStringAccessor(res).IsUtf16());
        EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(res, *tmpString));
    }
    {
        JSHandle<EcmaString> sourceString = factory->NewFromUtf8("整数integer");
        JSHandle<EcmaString> tmpString = factory->NewFromUtf8("数intege");
        EXPECT_TRUE(EcmaStringAccessor(sourceString).IsUtf16());
        EcmaString *res = EcmaStringAccessor::FastSubString(instance, sourceString, 1, 7);
        EXPECT_TRUE(EcmaStringAccessor(res).IsUtf16());
        EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(res, *tmpString));
    }
    {
        JSHandle<EcmaString> sourceString = factory->NewFromASCII("integer123");
        JSHandle<EcmaString> tmpString = factory->NewFromASCII("integer");
        EXPECT_TRUE(EcmaStringAccessor(sourceString).IsUtf8());
        EcmaString *res = EcmaStringAccessor::FastSubString(instance, sourceString, 0, 7);
        EXPECT_TRUE(EcmaStringAccessor(res).IsUtf8());
        EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(res, *tmpString));
    }
}

/*
 * @tc.name: GetUtf8Length
 * @tc.desc: Check whether the value returned through calling GetUtf8Length function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, GetUtf8Length)
{
    uint8_t arrayU8[6] = {3, 7, 19, 54, 99};
    uint16_t arrayU16Comp[] = {1, 12, 34, 56, 127};
    uint16_t arrayU16NotComp[] = {19, 54, 256, 11100, 65535};
    uint32_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
    uint32_t lengthEcmaStrU16Comp = sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0]);
    uint32_t lengthEcmaStrU16NotComp = sizeof(arrayU16NotComp) / sizeof(arrayU16NotComp[0]);
    JSHandle<EcmaString> handleEcmaStrU8(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8[0], lengthEcmaStrU8, true));
    JSHandle<EcmaString> handleEcmaStrU16Comp(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16Comp[0], lengthEcmaStrU16Comp, true));
    JSHandle<EcmaString> handleEcmaStrU16NotComp(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotComp[0], lengthEcmaStrU16NotComp, false));
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrU8).GetUtf8Length(), lengthEcmaStrU8 + 1);
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrU16Comp).GetUtf8Length(), lengthEcmaStrU16Comp + 1);
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrU16NotComp).GetUtf8Length(), 2 * lengthEcmaStrU16NotComp + 1);
}

/*
 * @tc.name: ObjectSize
 * @tc.desc: Check whether the value returned through calling ObjectSize function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, ObjectSize)
{
    JSHandle<EcmaString> handleEcmaStrEmpty(thread, EcmaStringAccessor::CreateEmptyString(instance));
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrEmpty).ObjectSize(), BaseString::SIZE + 0);

    size_t lengthEcmaStrAllocComp = 5;
    JSHandle<EcmaString> handleEcmaStrAllocComp(thread,
        EcmaStringAccessor::CreateLineString(instance, lengthEcmaStrAllocComp, true));
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrAllocComp).ObjectSize(),
        BaseString::SIZE + sizeof(uint8_t) * lengthEcmaStrAllocComp);

    size_t lengthEcmaStrAllocNotComp = 5;
    JSHandle<EcmaString> handleEcmaStrAllocNotComp(thread,
        EcmaStringAccessor::CreateLineString(instance, lengthEcmaStrAllocNotComp, false));
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrAllocNotComp).ObjectSize(),
        BaseString::SIZE + sizeof(uint16_t) * lengthEcmaStrAllocNotComp);

    uint8_t arrayU8[] = {"abcde"};
    size_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
    JSHandle<EcmaString> handleEcmaStrU8(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8[0], lengthEcmaStrU8, true));
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrU8).ObjectSize(), BaseString::SIZE + sizeof(uint8_t) * lengthEcmaStrU8);

    // ObjectSize(). EcmaString made by CreateFromUtf16( , , , true).
    uint16_t arrayU16Comp[] = {1, 23, 45, 67, 127};
    size_t lengthEcmaStrU16Comp = sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0]);
    JSHandle<EcmaString> handleEcmaStrU16Comp(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16Comp[0], lengthEcmaStrU16Comp, true));
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrU16Comp).ObjectSize(),
        BaseString::SIZE + sizeof(uint8_t) * lengthEcmaStrU16Comp);

    // ObjectSize(). EcmaString made by CreateFromUtf16( , , , false).
    uint16_t arrayU16NotComp[] = {127, 128, 256, 11100, 65535};
    size_t lengthEcmaStrU16NotComp = sizeof(arrayU16NotComp) / sizeof(arrayU16NotComp[0]);
    JSHandle<EcmaString> handleEcmaStrU16NotComp(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotComp[0], lengthEcmaStrU16NotComp, false));
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrU16NotComp).ObjectSize(),
        BaseString::SIZE + sizeof(uint16_t) * lengthEcmaStrU16NotComp);
}

/*
 * @tc.name: SetInternString
 * @tc.desc: Call SetInternString function, check whether the bool returned through calling IsInternString function
 * is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, SetInternString)
{
    uint8_t arrayU8[] = {"abc"};
    uint32_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
    JSHandle<EcmaString> handleEcmaStrU8(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8[0], lengthEcmaStrU8, true));
    EXPECT_FALSE(EcmaStringAccessor(handleEcmaStrU8).IsInternString());
    EcmaStringAccessor(handleEcmaStrU8).SetInternString();
    EXPECT_TRUE(EcmaStringAccessor(handleEcmaStrU8).IsInternString());

    uint16_t arrayU16Comp[] = {97, 98, 99};
    uint32_t lengthEcmaStrU16Comp = sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0]);
    JSHandle<EcmaString> handleEcmaStrU16Comp(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16Comp[0], lengthEcmaStrU16Comp, true));
    EXPECT_FALSE(EcmaStringAccessor(handleEcmaStrU16Comp).IsInternString());
    EcmaStringAccessor(handleEcmaStrU16Comp).SetInternString();
    EXPECT_TRUE(EcmaStringAccessor(handleEcmaStrU16Comp).IsInternString());

    uint16_t arrayU16NotComp[] = {97, 98, 99};
    uint32_t lengthEcmaStrU16NotComp = sizeof(arrayU16NotComp) / sizeof(arrayU16NotComp[0]);
    JSHandle<EcmaString> handleEcmaStrU16NotComp(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotComp[0], lengthEcmaStrU16NotComp, true));
    EXPECT_FALSE(EcmaStringAccessor(handleEcmaStrU16NotComp).IsInternString());
    EcmaStringAccessor(handleEcmaStrU16NotComp).SetInternString();
    EXPECT_TRUE(EcmaStringAccessor(handleEcmaStrU16NotComp).IsInternString());
}

/*
 * @tc.name: GetDataUtf8
 * @tc.desc: Check whether the pointer returned through calling GetDataUtf8 function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, GetDataUtf8)
{
    // From EcmaString made by CreateFromUtf8().
    uint8_t arrayU8[] = {"abcde"};
    uint32_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
    JSHandle<EcmaString> handleEcmaStrU8(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8[0], lengthEcmaStrU8, true));
    for (uint32_t i = 0; i < lengthEcmaStrU8; i++) {
        EXPECT_EQ(*(EcmaStringAccessor(handleEcmaStrU8).GetDataUtf8() + i), arrayU8[i]);
    }

    // From EcmaString made by CreateFromUtf16( , , , true).
    uint16_t arrayU16Comp[] = {3, 1, 34, 123, 127, 111, 42, 3, 20, 10};
    uint32_t lengthEcmaStrU16Comp = sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0]);
    JSHandle<EcmaString> handleEcmaStrU16Comp(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16Comp[0], lengthEcmaStrU16Comp, true));
    for (uint32_t i = 0; i < sizeof(arrayU16Comp) / arrayU16Comp[0]; i++) {
        EXPECT_EQ(*(EcmaStringAccessor(handleEcmaStrU16Comp).GetDataUtf8() + i), arrayU16Comp[i]);
    }
}

/*
 * @tc.name: GetDataUtf16
 * @tc.desc: Check whether the pointer returned through calling GetDataUtf16 function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, GetDataUtf16)
{
    // From EcmaString made by CreateFromUtf16( , , , false).
    uint16_t arrayU16NotComp[] = {67, 777, 1999, 1, 45, 66, 23456, 65535, 127, 333};
    uint32_t lengthEcmaStrU16NotComp = sizeof(arrayU16NotComp) / sizeof(arrayU16NotComp[0]);
    JSHandle<EcmaString> handleEcmaStrU16NotComp(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotComp[0], lengthEcmaStrU16NotComp, false));
    for (uint32_t i = 0; i < lengthEcmaStrU16NotComp; i++) {
        EXPECT_EQ(*(EcmaStringAccessor(handleEcmaStrU16NotComp).GetDataUtf16() + i), arrayU16NotComp[i]);
    }
}

/*
 * @tc.name: WriteToFlatUtf8
 * @tc.desc: Check whether the returned value and the changed array through a source EcmaString's calling
 * WriteToFlatUtf8 function are within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, WriteToFlatUtf8)
{
    // WriteToFlatUtf8(). From EcmaString made by CreateFromUtf8().
    uint8_t arrayU8CopyFrom[6] = {1, 12, 34, 56, 127};
    uint32_t lengthEcmaStrU8CopyFrom = sizeof(arrayU8CopyFrom) - 1;
    JSHandle<EcmaString> handleEcmaStrU8CopyFrom(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8CopyFrom[0], lengthEcmaStrU8CopyFrom, true));
    const size_t lengthArrayU8Target = 6;
    uint8_t arrayU8CopyTo[lengthArrayU8Target];

    size_t lengthReturnU8 = EcmaStringAccessor(handleEcmaStrU8CopyFrom)
        .WriteToFlatUtf8(&arrayU8CopyTo[0], lengthArrayU8Target);

    EXPECT_EQ(lengthReturnU8, lengthArrayU8Target);
    for (uint32_t i = 0; i < lengthReturnU8 - 1; i++) {
        EXPECT_EQ(arrayU8CopyTo[i], arrayU8CopyFrom[i]);
    }
    EXPECT_EQ(arrayU8CopyTo[lengthReturnU8 - 1], 0);

    // CopyDataUtf8(). From EcmaString made by CreateFromUtf16( , , , true).
    uint16_t arrayU16CompCopyFrom[] = {1, 12, 34, 56, 127};
    uint32_t lengthEcmaStrU16CompCopyFrom = sizeof(arrayU16CompCopyFrom) / sizeof(arrayU16CompCopyFrom[0]);
    JSHandle<EcmaString> handleEcmaStrU16CompCopyFrom(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16CompCopyFrom[0], lengthEcmaStrU16CompCopyFrom, true));
    const size_t lengthArrayU16Target = 6;
    uint8_t arrayU8CompCopyTo[lengthArrayU16Target];

    size_t lengthReturnU16Comp = EcmaStringAccessor(handleEcmaStrU16CompCopyFrom)
        .WriteToFlatUtf8(&arrayU8CompCopyTo[0], lengthArrayU16Target);

    EXPECT_EQ(lengthReturnU16Comp, lengthArrayU16Target);
    for (uint32_t i = 0; i < lengthReturnU16Comp - 1; i++) {
        EXPECT_EQ(arrayU8CompCopyTo[i], arrayU16CompCopyFrom[i]);
    }
    EXPECT_EQ(arrayU8CompCopyTo[lengthReturnU16Comp - 1], 0U);

    uint8_t buffer[200] = {0};
    std::string str(
        "您订单尾号为：1442的车辆提车点已由重庆市渝北区龙兴镇新能源办公大楼变更为重庆市江北区鱼嘴镇永和路39号2屋208室，请携带好相关材料按照预约时间前往提车点提车2,如有296ss");
    JSHandle<EcmaString> ecmaStr = thread->GetEcmaVM()->GetFactory()->NewFromStdString(str);
    uint32_t len = EcmaStringAccessor(ecmaStr).WriteToFlatUtf8(buffer, 198, true);
    EXPECT_EQ(len, 197);
}

/*
 * @tc.name: WriteToFlatUtf16
 * @tc.desc: Check whether the returned value and the changed array through a source EcmaString's calling
 * WriteToFlatUtf16 function are within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, WriteToFlatUtf16)
{
    // WriteToFlatUtf16(). From EcmaString made by CreateFromUtf16( , , , false).
    uint16_t arrayU16NotCompCopyFrom[10] = {67, 777, 1999, 1, 45, 66, 23456, 65535, 127, 333};
    uint32_t lengthEcmaStrU16NotCompCopyFrom = sizeof(arrayU16NotCompCopyFrom) / sizeof(arrayU16NotCompCopyFrom[0]);
    JSHandle<EcmaString> handleEcmaStrU16NotCompCopyFrom(thread, EcmaStringAccessor::CreateFromUtf16(
        instance, &arrayU16NotCompCopyFrom[0], lengthEcmaStrU16NotCompCopyFrom, false));
    const size_t lengthArrayU16Target = 13;
    uint16_t arrayU16NotCompCopyTo[lengthArrayU16Target];
    uint8_t defaultOneByteValueOfArrayU16NotCompCopyTo = 244;
    int checkResUtf16 = memset_s(&arrayU16NotCompCopyTo[0], sizeof(uint16_t) * lengthArrayU16Target,
        defaultOneByteValueOfArrayU16NotCompCopyTo, sizeof(uint16_t) * lengthArrayU16Target);
    EXPECT_TRUE(checkResUtf16 == 0);

    size_t lengthReturnU16NotComp = EcmaStringAccessor(handleEcmaStrU16NotCompCopyFrom)
        .WriteToFlatUtf16(&arrayU16NotCompCopyTo[0], lengthArrayU16Target);

    EXPECT_EQ(lengthReturnU16NotComp, lengthEcmaStrU16NotCompCopyFrom);
    for (uint32_t i = 0; i < lengthReturnU16NotComp; i++) {
        EXPECT_EQ(arrayU16NotCompCopyTo[i], EcmaStringAccessor(handleEcmaStrU16NotCompCopyFrom).Get(i));
    }
    for (uint32_t i = lengthReturnU16NotComp; i < lengthArrayU16Target; i++) {
        EXPECT_EQ(arrayU16NotCompCopyTo[i], ((uint16_t)defaultOneByteValueOfArrayU16NotCompCopyTo) * (1 + (1 << 8)));
    }
}

/*
 * @tc.name: ReadData_001
 * @tc.desc: Check whether the target EcmaString made by CreateLineString( , true, ) changed through calling ReadData
 * function with a source EcmaString made by CreateFromUtf8() is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, ReadData_001)
{
    // ReadData(). From EcmaString made by CreateFromUtf8() to EcmaString made by CreateLineString( , true, ).
    uint8_t arrayU8WriteFrom[6] = {1, 12, 34, 56, 127};
    uint32_t lengthEcmaStrU8WriteFrom = sizeof(arrayU8WriteFrom) - 1;
    JSHandle<EcmaString> handleEcmaStrU8WriteFrom(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8WriteFrom[0], lengthEcmaStrU8WriteFrom, true));
    size_t sizeEcmaStrU8WriteTo = 5;
    JSHandle<EcmaString> handleEcmaStrAllocTrueWriteTo(thread,
        EcmaStringAccessor::CreateLineString(instance, sizeEcmaStrU8WriteTo, true));
    uint32_t indexStartWriteFromArrayU8 = 2;
    uint32_t lengthWriteFromArrayU8 = 2;
    EcmaStringAccessor::ReadData(*handleEcmaStrAllocTrueWriteTo, *handleEcmaStrU8WriteFrom, indexStartWriteFromArrayU8,
        sizeEcmaStrU8WriteTo, lengthWriteFromArrayU8);
    for (uint32_t i = 0; i < lengthWriteFromArrayU8; i++) {
        EXPECT_EQ(EcmaStringAccessor(handleEcmaStrAllocTrueWriteTo).Get(i + indexStartWriteFromArrayU8),
            EcmaStringAccessor(handleEcmaStrU8WriteFrom).Get(i));
    }
}

/*
 * @tc.name: ReadData_002
 * @tc.desc: Check whether the target EcmaString made by CreateLineString( , false, ) changed through calling
 * ReadData function with a source EcmaString made by CreateFromUtf16( , , , false) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, ReadData_002)
{
    /* ReadData(). From EcmaString made by CreateFromUtf16( , , , false) to EcmaStringU16 made by
     * CreateLineString( , false, ).
     */
    uint16_t arrayU16WriteFrom[10] = {67, 777, 1999, 1, 45, 66, 23456, 65535, 127, 333};
    uint32_t lengthEcmaStrU16WriteFrom = sizeof(arrayU16WriteFrom) / sizeof(arrayU16WriteFrom[0]);
    JSHandle<EcmaString> handleEcmaStrU16WriteFrom(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16WriteFrom[0], lengthEcmaStrU16WriteFrom, false));
    size_t sizeEcmaStrU16WriteTo = 10;
    JSHandle<EcmaString> handleEcmaStrU16WriteTo(thread,
        EcmaStringAccessor::CreateLineString(instance, sizeEcmaStrU16WriteTo, false));
    uint32_t indexStartWriteFromArrayU16 = 3;
    uint32_t numBytesWriteFromArrayU16 = 2 * 3;
    EcmaStringAccessor::ReadData(*handleEcmaStrU16WriteTo, *handleEcmaStrU16WriteFrom, indexStartWriteFromArrayU16,
        sizeEcmaStrU16WriteTo, numBytesWriteFromArrayU16);
    for (uint32_t i = 0; i < (numBytesWriteFromArrayU16 / 2); i++) {
        EXPECT_EQ(EcmaStringAccessor(handleEcmaStrU16WriteTo).Get(i + indexStartWriteFromArrayU16),
            EcmaStringAccessor(handleEcmaStrU16WriteFrom).Get(i));
    }
}

/*
 * @tc.name: ReadData_003
 * @tc.desc: Check whether the target EcmaString made by CreateLineString( , false, ) changed through calling
 * ReadData function with a source EcmaString made by CreateFromUtf8() is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, ReadData_003)
{
    // ReadData(). From EcmaString made by CreateFromUtf8() to EcmaString made by CreateLineString( , false, ).
    uint8_t arrayU8WriteFrom[6] = {1, 12, 34, 56, 127};
    uint32_t lengthEcmaStrU8WriteFrom = sizeof(arrayU8WriteFrom) - 1;
    JSHandle<EcmaString> handleEcmaStrU8WriteFrom(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8WriteFrom[0], lengthEcmaStrU8WriteFrom, true));
    size_t sizeEcmaStrU16WriteTo = 10;
    JSHandle<EcmaString> handleEcmaStrU16WriteTo(thread,
        EcmaStringAccessor::CreateLineString(instance, sizeEcmaStrU16WriteTo, false));
    uint32_t indexStartWriteFromU8ToU16 = 1;
    uint32_t numBytesWriteFromU8ToU16 = 4;
    EcmaStringAccessor::ReadData(*handleEcmaStrU16WriteTo, *handleEcmaStrU8WriteFrom,
        indexStartWriteFromU8ToU16, sizeEcmaStrU16WriteTo,
        numBytesWriteFromU8ToU16);
    for (uint32_t i = 0; i < numBytesWriteFromU8ToU16; i++) {
        EXPECT_EQ(EcmaStringAccessor(handleEcmaStrU16WriteTo).Get(i + indexStartWriteFromU8ToU16),
            EcmaStringAccessor(handleEcmaStrU8WriteFrom).Get(i));
    }
}

/*
 * @tc.name: Set_001
 * @tc.desc: Check whether the target EcmaString made by CreateLineString( , true, ) changed through calling Set
 * function from a source char is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, Set_001)
{
    // Set(). From char to EcmaString made by CreateLineString( , true, ).
    char u8Write = 'a';
    size_t sizeEcmaStrU8WriteTo = 5;
    JSHandle<EcmaString> handleEcmaStrAllocTrueWriteTo(thread,
        EcmaStringAccessor::CreateLineString(instance, sizeEcmaStrU8WriteTo, true));
    uint32_t indexAtWriteFromU8 = 4;
    EcmaStringAccessor(handleEcmaStrAllocTrueWriteTo).Set(indexAtWriteFromU8, u8Write);
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrAllocTrueWriteTo).Get(indexAtWriteFromU8), u8Write);
}

/*
 * @tc.name: Set_002
 * @tc.desc: Check whether the target EcmaString made by CreateLineString( , false, ) changed through calling
 * Set function with a source char is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, Set_002)
{
    // Set(). From char to EcmaString made by CreateLineString( , false, ).
    size_t sizeEcmaStrU16WriteTo = 10;
    JSHandle<EcmaString> handleEcmaStrU16WriteTo(thread,
        EcmaStringAccessor::CreateLineString(instance, sizeEcmaStrU16WriteTo, false));
    char u8Write = 'a';
    uint32_t indexAt = 4;
    EcmaStringAccessor(handleEcmaStrU16WriteTo).Set(indexAt, u8Write);
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrU16WriteTo).Get(indexAt), u8Write);
}

/*
 * @tc.name: GetHashcode_001
 * @tc.desc: Check whether the value returned through an EcmaString made by CreateFromUtf8() calling GetHashcode
 * function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, GetHashcode_001)
{
    // GetHashcode(). EcmaString made by CreateFromUtf8().
    uint8_t arrayU8[] = {"abc"};
    uint32_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
    JSHandle<EcmaString> handleEcmaStrU8(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8[0], lengthEcmaStrU8, true));
    uint32_t hashExpect = 0;
    for (uint32_t i = 0; i < lengthEcmaStrU8; i++) {
        hashExpect = hashExpect * 31 + arrayU8[i];
    }
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrU8).GetHashcode(), hashExpect);
}

/*
 * @tc.name: GetHashcode_002
 * @tc.desc: Check whether the value returned through an EcmaString made by CreateFromUtf16( , , , true) calling
 * GetHashcode function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, GetHashcode_002)
{
    // GetHashcode(). EcmaString made by CreateFromUtf16( , , , true).
    uint16_t arrayU16Comp[] = {45, 92, 78, 24};
    uint32_t lengthEcmaStrU16Comp = sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0]);
    JSHandle<EcmaString> handleEcmaStrU16Comp(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16Comp[0], lengthEcmaStrU16Comp, true));
    uint32_t hashExpect = 0;
    for (uint32_t i = 0; i < lengthEcmaStrU16Comp; i++) {
        hashExpect = hashExpect * 31 + arrayU16Comp[i];
    }
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrU16Comp).GetHashcode(), hashExpect);
}

/*
 * @tc.name: GetHashcode_003
 * @tc.desc: Check whether the value returned through an EcmaString made by CreateFromUtf16( , , , false) calling
 * GetHashcode function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, GetHashcode_003)
{
    // GetHashcode(). EcmaString made by CreateFromUtf16( , , , false).
    uint16_t arrayU16NotComp[] = {199, 1, 256, 65535, 777};
    uint32_t lengthEcmaStrU16NotComp = sizeof(arrayU16NotComp) / sizeof(arrayU16NotComp[0]);
    JSHandle<EcmaString> handleEcmaStrU16NotComp(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotComp[0], lengthEcmaStrU16NotComp, false));
    uint32_t hashExpect = 0;
    for (uint32_t i = 0; i < lengthEcmaStrU16NotComp; i++) {
        hashExpect = hashExpect * 31 + arrayU16NotComp[i];
    }
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrU16NotComp).GetHashcode(), hashExpect);
}

/*
 * @tc.name: GetHashcode_004
 * @tc.desc: Check whether the value returned through an EcmaString made by CreateEmptyString() calling GetHashcode
 * function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, GetHashcode_004)
{
    // GetHashcode(). EcmaString made by CreateEmptyString().
    JSHandle<EcmaString> handleEcmaStrEmpty(thread, EcmaStringAccessor::CreateEmptyString(instance));
    EXPECT_EQ(EcmaStringAccessor(handleEcmaStrEmpty).GetHashcode(), 0U);
}

/*
 * @tc.name: ComputeHashcodeUtf8
 * @tc.desc: Check whether the value returned through calling ComputeHashcodeUtf8 function with an Array(uint8_t) is
 * within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, ComputeHashcodeUtf8)
{
    uint8_t arrayU8[] = {"abc"};
    uint32_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
    uint32_t hashExpect = 0;
    for (uint32_t i = 0; i < lengthEcmaStrU8; i++) {
        hashExpect = hashExpect * 31 + arrayU8[i];
    }
    EXPECT_EQ(EcmaStringAccessor::ComputeHashcodeUtf8(&arrayU8[0], lengthEcmaStrU8, true), hashExpect);
}

/*
 * @tc.name: ComputeHashcodeUtf16
 * @tc.desc: Check whether the value returned through calling ComputeHashcodeUtf16 function with an Array(uint16_t) is
 * within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, ComputeHashcodeUtf16)
{
    uint16_t arrayU16[] = {199, 1, 256, 65535, 777};
    uint32_t lengthEcmaStrU16 = sizeof(arrayU16) / sizeof(arrayU16[0]);
    uint32_t hashExpect = 0;
    for (uint32_t i = 0; i < lengthEcmaStrU16; i++) {
        hashExpect = hashExpect * 31 + arrayU16[i];
    }
    EXPECT_EQ(EcmaStringAccessor::ComputeHashcodeUtf16(&arrayU16[0], lengthEcmaStrU16), hashExpect);
}

/*
 * @tc.name: IndexOf_001
 * @tc.desc: Check whether the value returned through a source EcmaString made by CreateFromUtf8() calling IndexOf
 * function with a target EcmaString made by CreateFromUtf8() is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, IndexOf_001)
{
    // IndexOf(). Find EcmaString made by CreateFromUtf8() From EcmaString made by CreateFromUtf8().
    uint8_t arrayU8From[7] = {23, 25, 1, 3, 39, 80};
    uint8_t arrayU8Target[4] = {1, 3, 39};
    uint32_t lengthEcmaStrU8From = sizeof(arrayU8From) - 1;
    uint32_t lengthEcmaStrU8Target = sizeof(arrayU8Target) - 1;
    JSHandle<EcmaString> handleEcmaStr(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8From[0], lengthEcmaStrU8From, true));
    JSHandle<EcmaString> handleEcmaStr1(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8Target[0], lengthEcmaStrU8Target, true));
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
 * @tc.name: IndexOf_002
 * @tc.desc: Check whether the value returned through a source EcmaString made by CreateFromUtf16( , , , false) calling
 * IndexOf function with a target EcmaString made by CreateFromUtf8() is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, IndexOf_002)
{
    // IndexOf(). Find EcmaString made by CreateFromUtf8() From EcmaString made by CreateFromUtf16( , , , false).
    uint8_t arrayU8Target[4] = {1, 3, 39};
    uint16_t arrayU16NotCompFromNo1[] = {67, 65535, 127, 777, 1453, 44, 1, 3, 39, 80, 333};
    uint32_t lengthEcmaStrU8Target = sizeof(arrayU8Target) - 1;
    uint32_t lengthEcmaStrU16NotCompFromNo1 = sizeof(arrayU16NotCompFromNo1) / sizeof(arrayU16NotCompFromNo1[0]);
    JSHandle<EcmaString> handleEcmaStr(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8Target[0], lengthEcmaStrU8Target, true));
    JSHandle<EcmaString> handleEcmaStr1(thread, EcmaStringAccessor::CreateFromUtf16(
        instance, &arrayU16NotCompFromNo1[0], lengthEcmaStrU16NotCompFromNo1, false));
    int32_t posStart = 0;
    EXPECT_EQ(EcmaStringAccessor::IndexOf(instance, handleEcmaStr1, handleEcmaStr, posStart), 6);
    EXPECT_EQ(EcmaStringAccessor::IndexOf(instance, handleEcmaStr, handleEcmaStr1, posStart), -1);
    posStart = -1;
    EXPECT_EQ(EcmaStringAccessor::IndexOf(instance, handleEcmaStr1, handleEcmaStr, posStart), 6);
    posStart = 1;
    EXPECT_EQ(EcmaStringAccessor::IndexOf(instance, handleEcmaStr1, handleEcmaStr, posStart), 6);
    posStart = 6;
    EXPECT_EQ(EcmaStringAccessor::IndexOf(instance, handleEcmaStr1, handleEcmaStr, posStart), 6);
    posStart = 7;
    EXPECT_EQ(EcmaStringAccessor::IndexOf(instance, handleEcmaStr1, handleEcmaStr, posStart), -1);
}

/*
 * @tc.name: IndexOf_003
 * @tc.desc: Check whether the value returned through a source EcmaString made by CreateFromUtf16( , , , false) calling
 * IndexOf function with a target EcmaString made by CreateFromUtf16( , , , false) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, IndexOf_003)
{
    /* IndexOf(). Find EcmaString made by CreateFromUtf16( , , , false) From EcmaString made by
     * CreateFromUtf16( , , , false).
     */
    uint16_t arrayU16NotCompTarget[] = {1453, 44};
    uint16_t arrayU16NotCompFrom[] = {67, 65535, 127, 777, 1453, 44, 1, 3, 39, 80, 333};
    uint32_t lengthEcmaStrU16NotCompTarget = sizeof(arrayU16NotCompTarget) / sizeof(arrayU16NotCompTarget[0]);
    uint32_t lengthEcmaStrU16NotCompFrom = sizeof(arrayU16NotCompFrom) / sizeof(arrayU16NotCompFrom[0]);
    JSHandle<EcmaString> handleEcmaStrU16NotCompTarget(thread, EcmaStringAccessor::CreateFromUtf16(
        instance, &arrayU16NotCompTarget[0], lengthEcmaStrU16NotCompTarget, false));
    JSHandle<EcmaString> handleEcmaStrU16NotCompFrom(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotCompFrom[0], lengthEcmaStrU16NotCompFrom, false));
    int32_t posStart = 0;
    EXPECT_EQ(EcmaStringAccessor::IndexOf(instance,
        handleEcmaStrU16NotCompFrom, handleEcmaStrU16NotCompTarget, posStart), 4);
    EXPECT_EQ(EcmaStringAccessor::IndexOf(instance,
        handleEcmaStrU16NotCompTarget, handleEcmaStrU16NotCompFrom, posStart), -1);
    posStart = -1;
    EXPECT_EQ(EcmaStringAccessor::IndexOf(instance,
        handleEcmaStrU16NotCompFrom, handleEcmaStrU16NotCompTarget, posStart), 4);
    posStart = 1;
    EXPECT_EQ(EcmaStringAccessor::IndexOf(instance,
        handleEcmaStrU16NotCompFrom, handleEcmaStrU16NotCompTarget, posStart), 4);
    posStart = 4;
    EXPECT_EQ(EcmaStringAccessor::IndexOf(instance,
        handleEcmaStrU16NotCompFrom, handleEcmaStrU16NotCompTarget, posStart), 4);
    posStart = 5;
    EXPECT_EQ(EcmaStringAccessor::IndexOf(instance,
        handleEcmaStrU16NotCompFrom, handleEcmaStrU16NotCompTarget, posStart), -1);
}

/*
 * @tc.name: IndexOf_004
 * @tc.desc: Check whether the value returned through a source EcmaString made by CreateFromUtf8() calling IndexOf
 * function with a target EcmaString made by CreateFromUtf16() is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, IndexOf_004)
{
    // IndexOf(). Find EcmaString made by CreateFromUtf16() From EcmaString made by CreateFromUtf8().
    std::vector<uint16_t> ecmaStrU16NotCompTarget = {3, 39, 80};
    std::vector<uint8_t> arrayU8From{23, 25, 1, 3, 39, 80};
    auto func = std::bind(EcmaStringAccessor::CreateFromUtf8, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4, MemSpaceType::SHARED_OLD_SPACE);
    EcmaTestCommon::IndexOfCommonCase(thread, instance, ecmaStrU16NotCompTarget, arrayU8From, func);
}

/*
 * @tc.name: Compare_001
 * @tc.desc: Check whether the value returned through calling Compare function between EcmaStrings made by
 * CreateFromUtf8() is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, Compare_001)
{
    // Compare(). Between EcmaStrings made by CreateFromUtf8().
    uint8_t arrayU8No1[3] = {1, 23};
    uint8_t arrayU8No2[4] = {1, 23, 49};
    uint8_t arrayU8No3[6] = {1, 23, 45, 97, 127};
    uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
    uint32_t lengthEcmaStrU8No2 = sizeof(arrayU8No2) - 1;
    uint32_t lengthEcmaStrU8No3 = sizeof(arrayU8No3) - 1;
    JSHandle<EcmaString> handleEcmaStrU8No1(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8No1[0], lengthEcmaStrU8No1, true));
    JSHandle<EcmaString> handleEcmaStrU8No2(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8No2[0], lengthEcmaStrU8No2, true));
    JSHandle<EcmaString> handleEcmaStrU8No3(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8No3[0], lengthEcmaStrU8No3, true));
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU8No1, handleEcmaStrU8No2), -1);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU8No2, handleEcmaStrU8No1), 1);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU8No2, handleEcmaStrU8No3), 49 - 45);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU8No3, handleEcmaStrU8No2), 45 - 49);
}

/*
 * @tc.name: Compare_002
 * @tc.desc: Check whether the value returned through calling Compare function between EcmaStrings made by
 * CreateFromUtf16( , , , true) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, Compare_002)
{
    // Compare(). Between EcmaStrings made by CreateFromUtf16( , , , true).
    uint16_t arrayU16CompNo1[] = {1, 23};
    uint16_t arrayU16CompNo2[] = {1, 23, 49};
    uint16_t arrayU16CompNo3[] = {1, 23, 45, 97, 127};
    uint32_t lengthEcmaStrU16CompNo1 = sizeof(arrayU16CompNo1) / sizeof(arrayU16CompNo1[0]);
    uint32_t lengthEcmaStrU16CompNo2 = sizeof(arrayU16CompNo2) / sizeof(arrayU16CompNo2[0]);
    uint32_t lengthEcmaStrU16CompNo3 = sizeof(arrayU16CompNo3) / sizeof(arrayU16CompNo3[0]);
    JSHandle<EcmaString> handleEcmaStrU16CompNo1(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16CompNo1[0], lengthEcmaStrU16CompNo1, true));
    JSHandle<EcmaString> handleEcmaStrU16CompNo2(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16CompNo2[0], lengthEcmaStrU16CompNo2, true));
    JSHandle<EcmaString> handleEcmaStrU16CompNo3(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16CompNo3[0], lengthEcmaStrU16CompNo3, true));
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16CompNo1, handleEcmaStrU16CompNo2), -1);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16CompNo2, handleEcmaStrU16CompNo1), 1);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16CompNo2, handleEcmaStrU16CompNo3), 49 - 45);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16CompNo3, handleEcmaStrU16CompNo2), 45 - 49);
}

/*
 * @tc.name: Compare_003
 * @tc.desc: Check whether the value returned through calling Compare function between EcmaString made by
 * CreateFromUtf8() and EcmaString made by CreateFromUtf16( , , , true) made by CreateFromUtf16( , , , true) is within
 * expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, Compare_003)
{
    auto createUtf8 = std::bind(EcmaStringAccessor::CreateFromUtf8, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4, MemSpaceType::SHARED_OLD_SPACE);
    auto createUtf16 = std::bind(EcmaStringAccessor::CreateFromUtf16, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4, MemSpaceType::SHARED_OLD_SPACE);
    EcmaTestCommon::CompareCommonCase(thread, instance, createUtf8, createUtf16, EcmaStringAccessor::Compare);
}

/*
 * @tc.name: Compare_004
 * @tc.desc: Check whether the value returned through calling Compare function between EcmaStrings made by
 * CreateFromUtf16( , , , false) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, Compare_004)
{
    // Compare(). Between EcmaStrings made by CreateFromUtf16( , , , false).
    uint16_t arrayU16NotCompNo1[] = {1, 23};
    uint16_t arrayU16NotCompNo2[] = {1, 23, 49};
    uint16_t arrayU16NotCompNo3[] = {1, 23, 456, 6789, 65535, 127};
    uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
    uint32_t lengthEcmaStrU16NotCompNo2 = sizeof(arrayU16NotCompNo2) / sizeof(arrayU16NotCompNo2[0]);
    uint32_t lengthEcmaStrU16NotCompNo3 = sizeof(arrayU16NotCompNo3) / sizeof(arrayU16NotCompNo3[0]);
    JSHandle<EcmaString> handleEcmaStrU16NotCompNo1(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1, true));
    JSHandle<EcmaString> handleEcmaStrU16NotCompNo2(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotCompNo2[0], lengthEcmaStrU16NotCompNo2, true));
    JSHandle<EcmaString> handleEcmaStrU16NotCompNo3(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotCompNo3[0], lengthEcmaStrU16NotCompNo3, false));
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16NotCompNo1, handleEcmaStrU16NotCompNo2), -1);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16NotCompNo2, handleEcmaStrU16NotCompNo1), 1);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16NotCompNo2, handleEcmaStrU16NotCompNo3), 49 - 456);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16NotCompNo3, handleEcmaStrU16NotCompNo2), 456 - 49);
}

/*
 * @tc.name: Compare_005
 * @tc.desc: Check whether the value returned through calling Compare function between EcmaString made by
 * CreateFromUtf8() and EcmaString made by CreateFromUtf16( , , , false) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, Compare_005)
{
    // Compare(). EcmaString made by CreateFromUtf8() and EcmaString made by CreateFromUtf16( , , , false).
    uint8_t arrayU8No1[3] = {1, 23};
    uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
    JSHandle<EcmaString> handleEcmaStrU8No1(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8No1[0], lengthEcmaStrU8No1, true));

    uint8_t arrayU8No2[4] = {1, 23, 49};
    uint32_t lengthEcmaStrU8No2 = sizeof(arrayU8No2) - 1;
    JSHandle<EcmaString> handleEcmaStrU8No2(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8No2[0], lengthEcmaStrU8No2, true));

    uint16_t arrayU16NotCompNo1[] = {1, 23};
    uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
    JSHandle<EcmaString> handleEcmaStrU16NotCompNo1(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1, true));
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU8No1, handleEcmaStrU16NotCompNo1), 0);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16NotCompNo1, handleEcmaStrU8No1), 0);

    uint16_t arrayU16NotCompNo2[] = {1, 23, 49};
    uint32_t lengthEcmaStrU16NotCompNo2 = sizeof(arrayU16NotCompNo2) / sizeof(arrayU16NotCompNo2[0]);
    JSHandle<EcmaString> handleEcmaStrU16NotCompNo2(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotCompNo2[0], lengthEcmaStrU16NotCompNo2, true));
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU8No1, handleEcmaStrU16NotCompNo2), -1);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16NotCompNo2, handleEcmaStrU8No1), 1);
    
    uint16_t arrayU16NotCompNo3[] = {1, 23, 456, 6789, 65535, 127};
    uint32_t lengthEcmaStrU16NotCompNo3 = sizeof(arrayU16NotCompNo3) / sizeof(arrayU16NotCompNo3[0]);
    JSHandle<EcmaString> handleEcmaStrU16NotCompNo3(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotCompNo3[0], lengthEcmaStrU16NotCompNo3, false));
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU8No2, handleEcmaStrU16NotCompNo3), 49 - 456);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16NotCompNo3, handleEcmaStrU8No2), 456 - 49);
}

/*
 * @tc.name: Compare_006
 * @tc.desc: Check whether the value returned through calling Compare function between EcmaString made by
 * CreateFromUtf16( , , , true) and EcmaString made by CreateFromUtf16( , , , false) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, Compare_006)
{
    // Compare(). EcmaString made by CreateFromUtf16( , , , true) and EcmaString made by CreateFromUtf16( , , , false).
    uint16_t arrayU16CompNo1[] = {1, 23};
    uint32_t lengthEcmaStrU16CompNo1 = sizeof(arrayU16CompNo1) / sizeof(arrayU16CompNo1[0]);
    JSHandle<EcmaString> handleEcmaStrU16CompNo1(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16CompNo1[0], lengthEcmaStrU16CompNo1, true));

    uint16_t arrayU16CompNo2[] = {1, 23, 49};
    uint32_t lengthEcmaStrU16CompNo2 = sizeof(arrayU16CompNo2) / sizeof(arrayU16CompNo2[0]);
    JSHandle<EcmaString> handleEcmaStrU16CompNo2(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16CompNo2[0], lengthEcmaStrU16CompNo2, true));

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

    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16CompNo1, handleEcmaStrU16NotCompNo1), 0);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16NotCompNo1, handleEcmaStrU16CompNo1), 0);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16CompNo1, handleEcmaStrU16NotCompNo2), -1);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16NotCompNo2, handleEcmaStrU16CompNo1), 1);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16CompNo2, handleEcmaStrU16NotCompNo3), 49 - 456);
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16NotCompNo3, handleEcmaStrU16CompNo2), 456 - 49);
}

/*
 * @tc.name: StringsAreEqual_001
 * @tc.desc: Check whether the bool returned through calling StringsAreEqual function with two EcmaStrings made by
 * CreateFromUtf8() is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, StringsAreEqual_001)
{
    // StringsAreEqual().
    uint8_t arrayU8No1[4] = {45, 92, 78};
    uint8_t arrayU8No2[4] = {45, 92, 78};
    uint8_t arrayU8No3[5] = {45, 92, 78, 1};
    uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
    uint32_t lengthEcmaStrU8No2 = sizeof(arrayU8No2) - 1;
    uint32_t lengthEcmaStrU8No3 = sizeof(arrayU8No3) - 1;
    JSHandle<EcmaString> handleEcmaStrU8No1(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8No1[0], lengthEcmaStrU8No1, true));
    JSHandle<EcmaString> handleEcmaStrU8No2(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8No2[0], lengthEcmaStrU8No2, true));
    JSHandle<EcmaString> handleEcmaStrU8No3(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8No3[0], lengthEcmaStrU8No3, true));
    EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(*handleEcmaStrU8No1, *handleEcmaStrU8No2));
    EXPECT_FALSE(EcmaStringAccessor::StringsAreEqual(*handleEcmaStrU8No1, *handleEcmaStrU8No3));
    EXPECT_FALSE(EcmaStringAccessor::StringsAreEqual(*handleEcmaStrU8No3, *handleEcmaStrU8No1));
}

/*
 * @tc.name: StringsAreEqual_002
 * @tc.desc: Check whether the bool returned through calling StringsAreEqual function with a EcmaString made by
 * CreateFromUtf8() and a EcmaString made by CreateFromUtf16(, , , true) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, StringsAreEqual_002)
{
    // StringsAreEqual().
    uint8_t arrayU8No1[4] = {45, 92, 78};
    uint16_t arrayU16CompNo2[] = {45, 92, 78};
    uint16_t arrayU16CompNo3[] = {45, 92, 78, 1};
    uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
    uint32_t lengthEcmaStrU16CompNo2 = sizeof(arrayU16CompNo2) / sizeof(arrayU16CompNo2[0]);
    uint32_t lengthEcmaStrU16CompNo3 = sizeof(arrayU16CompNo3) / sizeof(arrayU16CompNo3[0]);
    JSHandle<EcmaString> handleEcmaStrU8No1(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8No1[0], lengthEcmaStrU8No1, true));
    JSHandle<EcmaString> handleEcmaStrU16CompNo2(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16CompNo2[0], lengthEcmaStrU16CompNo2, true));
    JSHandle<EcmaString> handleEcmaStrU16CompNo3(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16CompNo3[0], lengthEcmaStrU16CompNo3, true));
    EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(*handleEcmaStrU8No1, *handleEcmaStrU16CompNo2));
    EXPECT_FALSE(EcmaStringAccessor::StringsAreEqual(*handleEcmaStrU8No1, *handleEcmaStrU16CompNo3));
}

/*
 * @tc.name: StringsAreEqual_003
 * @tc.desc: Check whether the bool returned through calling StringsAreEqual function with two EcmaStrings made by
 * CreateFromUtf16(, , , true) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, StringsAreEqual_003)
{
    // StringsAreEqual().
    uint16_t arrayU16CompNo1[] = {45, 92, 78};
    uint16_t arrayU16CompNo2[] = {45, 92, 78};
    uint16_t arrayU16CompNo3[] = {45, 92, 78, 1};
    uint32_t lengthEcmaStrU16CompNo1 = sizeof(arrayU16CompNo1) / sizeof(arrayU16CompNo1[0]);
    uint32_t lengthEcmaStrU16CompNo2 = sizeof(arrayU16CompNo2) / sizeof(arrayU16CompNo2[0]);
    uint32_t lengthEcmaStrU16CompNo3 = sizeof(arrayU16CompNo3) / sizeof(arrayU16CompNo3[0]);
    JSHandle<EcmaString> handleEcmaStrU16CompNo1(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16CompNo1[0], lengthEcmaStrU16CompNo1, true));
    JSHandle<EcmaString> handleEcmaStrU16CompNo2(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16CompNo2[0], lengthEcmaStrU16CompNo2, true));
    JSHandle<EcmaString> handleEcmaStrU16CompNo3(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16CompNo3[0], lengthEcmaStrU16CompNo3, true));
    EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(*handleEcmaStrU16CompNo1, *handleEcmaStrU16CompNo2));
    EXPECT_FALSE(EcmaStringAccessor::StringsAreEqual(*handleEcmaStrU16CompNo1, *handleEcmaStrU16CompNo3));
    EXPECT_FALSE(EcmaStringAccessor::StringsAreEqual(*handleEcmaStrU16CompNo3, *handleEcmaStrU16CompNo1));
}

/*
 * @tc.name: StringsAreEqual_004
 * @tc.desc: Check whether the bool returned through calling StringsAreEqual function with a EcmaString made by
 * CreateFromUtf8() and a EcmaString made by CreateFromUtf16(, , , false) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, StringsAreEqual_004)
{
    // StringsAreEqual().
    uint8_t arrayU8No1[4] = {45, 92, 78};
    uint16_t arrayU16NotCompNo1[] = {45, 92, 78};
    uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
    uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
    JSHandle<EcmaString> handleEcmaStrU8No1(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8No1[0], lengthEcmaStrU8No1, true));
    JSHandle<EcmaString> handleEcmaStrU16NotCompNo1(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1, true));
    EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(*handleEcmaStrU8No1, *handleEcmaStrU16NotCompNo1));
    EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(*handleEcmaStrU16NotCompNo1, *handleEcmaStrU8No1));
}

/*
 * @tc.name: StringsAreEqual_005
 * @tc.desc: Check whether the bool returned through calling StringsAreEqual function with a EcmaString made by
 * CreateFromUtf16(, , , true) and a EcmaString made by CreateFromUtf16(, , , false) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, StringsAreEqual_005)
{
    // StringsAreEqual().
    uint16_t arrayU16CompNo1[] = {45, 92, 78};
    uint16_t arrayU16NotCompNo1[] = {45, 92, 78};
    uint32_t lengthEcmaStrU16CompNo1 = sizeof(arrayU16CompNo1) / sizeof(arrayU16CompNo1[0]);
    uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
    JSHandle<EcmaString> handleEcmaStrU16CompNo1(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16CompNo1[0], lengthEcmaStrU16CompNo1, true));
    JSHandle<EcmaString> handleEcmaStrU16NotCompNo1(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1, true));
    EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(*handleEcmaStrU16CompNo1, *handleEcmaStrU16NotCompNo1));
    EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(*handleEcmaStrU16NotCompNo1, *handleEcmaStrU16CompNo1));
}

/*
 * @tc.name: StringsAreEqual_006
 * @tc.desc: Check whether the bool returned through calling StringsAreEqual function with two EcmaStrings made by
 * CreateFromUtf16(, , , false) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, StringsAreEqual_006)
{
    // StringsAreEqual().
    uint16_t arrayU16NotCompNo1[] = {234, 345, 127, 2345, 65535, 5};
    uint16_t arrayU16NotCompNo2[] = {234, 345, 127, 2345, 65535, 5};
    uint16_t arrayU16NotCompNo3[] = {1, 234, 345, 127, 2345, 65535, 5};
    uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
    uint32_t lengthEcmaStrU16NotCompNo2 = sizeof(arrayU16NotCompNo2) / sizeof(arrayU16NotCompNo2[0]);
    uint32_t lengthEcmaStrU16NotCompNo3 = sizeof(arrayU16NotCompNo3) / sizeof(arrayU16NotCompNo3[0]);
    JSHandle<EcmaString> handleEcmaStrU16NotCompNo1(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1, false));
    JSHandle<EcmaString> handleEcmaStrU16NotCompNo2(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotCompNo2[0], lengthEcmaStrU16NotCompNo2, false));
    JSHandle<EcmaString> handleEcmaStrU16NotCompNo3(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotCompNo3[0], lengthEcmaStrU16NotCompNo3, false));
    EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(*handleEcmaStrU16NotCompNo1, *handleEcmaStrU16NotCompNo2));
    EXPECT_FALSE(EcmaStringAccessor::StringsAreEqual(*handleEcmaStrU16NotCompNo1, *handleEcmaStrU16NotCompNo3));
    EXPECT_FALSE(EcmaStringAccessor::StringsAreEqual(*handleEcmaStrU16NotCompNo3, *handleEcmaStrU16NotCompNo1));
}

/*
 * @tc.name: StringsAreEqualUtf8_001
 * @tc.desc: Check whether the bool returned through calling StringIsEqualUint8Data function with an EcmaString made by
 * CreateFromUtf8() and an Array(uint8_t) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, StringsAreEqualUtf8_001)
{
    // StringIsEqualUint8Data(). EcmaString made by CreateFromUtf8(), Array:U8.
    uint8_t arrayU8No1[4] = {45, 92, 78};
    uint8_t arrayU8No2[5] = {45, 92, 78, 24};
    uint8_t arrayU8No3[3] = {45, 92};
    uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
    uint32_t lengthEcmaStrU8No2 = sizeof(arrayU8No2) - 1;
    uint32_t lengthEcmaStrU8No3 = sizeof(arrayU8No3) - 1;
    JSHandle<EcmaString> handleEcmaStrU8No1(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8No1[0], lengthEcmaStrU8No1, true));
    JSHandle<EcmaString> handleEcmaStrU8No2(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8No2[0], lengthEcmaStrU8No2, true));
    JSHandle<EcmaString> handleEcmaStrU8No3(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8No3[0], lengthEcmaStrU8No3, true));
    EXPECT_TRUE(EcmaStringAccessor::StringIsEqualUint8Data(
        *handleEcmaStrU8No1, &arrayU8No1[0], lengthEcmaStrU8No1, true));
    EXPECT_FALSE(EcmaStringAccessor::StringIsEqualUint8Data(
        *handleEcmaStrU8No1, &arrayU8No1[0], lengthEcmaStrU8No1, false));
    EXPECT_FALSE(EcmaStringAccessor::StringIsEqualUint8Data(
        *handleEcmaStrU8No2, &arrayU8No1[0], lengthEcmaStrU8No1, true));
    EXPECT_FALSE(EcmaStringAccessor::StringIsEqualUint8Data(
        *handleEcmaStrU8No3, &arrayU8No1[0], lengthEcmaStrU8No1, true));
}

/*
 * @tc.name: StringsAreEqualUtf8_002
 * @tc.desc: Check whether the bool returned through calling StringIsEqualUint8Data function with an EcmaString made by
 * CreateFromUtf16( , , , true) and an Array(uint8_t) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, StringsAreEqualUtf8_002)
{
    // StringIsEqualUint8Data(). EcmaString made by CreateFromUtf16( , , , true), Array:U8.
    uint8_t arrayU8No1[4] = {45, 92, 78};
    uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;

    uint16_t arrayU16CompNo1[] = {45, 92, 78};
    uint32_t lengthEcmaStrU16CompNo1 = sizeof(arrayU16CompNo1) / sizeof(arrayU16CompNo1[0]);

    uint16_t arrayU16CompNo2[] = {45, 92, 78, 24};
    uint32_t lengthEcmaStrU16CompNo2 = sizeof(arrayU16CompNo2) / sizeof(arrayU16CompNo2[0]);

    uint16_t arrayU16CompNo3[] = {45, 92};
    uint32_t lengthEcmaStrU16CompNo3 = sizeof(arrayU16CompNo3) / sizeof(arrayU16CompNo3[0]);
    JSHandle<EcmaString> handleEcmaStrU16CompNo1(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16CompNo1[0], lengthEcmaStrU16CompNo1, true));
    JSHandle<EcmaString> handleEcmaStrU16CompNo2(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16CompNo2[0], lengthEcmaStrU16CompNo2, true));
    JSHandle<EcmaString> handleEcmaStrU16CompNo3(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16CompNo3[0], lengthEcmaStrU16CompNo3, true));
    EXPECT_TRUE(EcmaStringAccessor::StringIsEqualUint8Data(
        *handleEcmaStrU16CompNo1, &arrayU8No1[0], lengthEcmaStrU8No1, true));
    EXPECT_FALSE(EcmaStringAccessor::StringIsEqualUint8Data(
        *handleEcmaStrU16CompNo1, &arrayU8No1[0], lengthEcmaStrU8No1, false));
    EXPECT_FALSE(EcmaStringAccessor::StringIsEqualUint8Data(
        *handleEcmaStrU16CompNo2, &arrayU8No1[0], lengthEcmaStrU8No1, true));
    EXPECT_FALSE(EcmaStringAccessor::StringIsEqualUint8Data(
        *handleEcmaStrU16CompNo3, &arrayU8No1[0], lengthEcmaStrU8No1, true));
}

/*
 * @tc.name: StringsAreEqualUtf8_003
 * @tc.desc: Check whether the bool returned through calling StringIsEqualUint8Data function with an EcmaString made by
 * CreateFromUtf16( , , , false) and an Array(uint8_t) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, StringsAreEqualUtf8_003)
{
    // StringIsEqualUint8Data(). EcmaString made by CreateFromUtf16( , , , false), Array:U8.
    EcmaTestCommon::StringIsEqualCommonCase(thread, instance, EcmaStringAccessor::StringIsEqualUint8Data);
}

/*
 * @tc.name: StringsAreEqualUtf16_001
 * @tc.desc: Check whether the bool returned through calling StringsAreEqualUtf16 function with an EcmaString made by
 * CreateFromUtf8() and an Array(uint16_t) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, StringsAreEqualUtf16_001)
{
    // StringsAreEqualUtf16(). EcmaString made by CreateFromUtf8, Array:U16(1-127).
    uint8_t arrayU8No1[4] = {45, 92, 78};
    uint8_t arrayU8No2[5] = {45, 92, 78, 24};
    uint8_t arrayU8No3[3] = {45, 92};
    uint16_t arrayU16NotCompNo1[] = {45, 92, 78};
    uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
    uint32_t lengthEcmaStrU8No2 = sizeof(arrayU8No2) - 1;
    uint32_t lengthEcmaStrU8No3 = sizeof(arrayU8No3) - 1;
    uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
    JSHandle<EcmaString> handleEcmaStrU8No1(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8No1[0], lengthEcmaStrU8No1, true));
    JSHandle<EcmaString> handleEcmaStrU8No2(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8No2[0], lengthEcmaStrU8No2, true));
    JSHandle<EcmaString> handleEcmaStrU8No3(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, &arrayU8No3[0], lengthEcmaStrU8No3, true));
    EXPECT_TRUE(EcmaStringAccessor::StringsAreEqualUtf16(
        *handleEcmaStrU8No1, &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1));
    EXPECT_FALSE(EcmaStringAccessor::StringsAreEqualUtf16(
        *handleEcmaStrU8No2, &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1));
    EXPECT_FALSE(EcmaStringAccessor::StringsAreEqualUtf16(
        *handleEcmaStrU8No3, &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1));
}

/*
 * @tc.name: StringsAreEqualUtf16_002
 * @tc.desc: Check whether the bool returned through calling StringsAreEqualUtf16 function with an EcmaString made by
 * CreateFromUtf16( , , , true) and an Array(uint16_t) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, StringsAreEqualUtf16_002)
{
    // StringsAreEqualUtf16(). EcmaString made by CreateFromUtf16( , , , true), Array:U16(1-127).
    uint16_t arrayU16CompNo1[] = {45, 92, 78};
    uint32_t lengthEcmaStrU16CompNo1 = sizeof(arrayU16CompNo1) / sizeof(arrayU16CompNo1[0]);
    JSHandle<EcmaString> handleEcmaStrU16CompNo1(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16CompNo1[0], lengthEcmaStrU16CompNo1, true));

    uint16_t arrayU16CompNo2[] = {45, 92, 78, 24};
    uint32_t lengthEcmaStrU16CompNo2 = sizeof(arrayU16CompNo2) / sizeof(arrayU16CompNo2[0]);
    JSHandle<EcmaString> handleEcmaStrU16CompNo2(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16CompNo2[0], lengthEcmaStrU16CompNo2, true));

    uint16_t arrayU16CompNo3[] = {45, 92};
    uint32_t lengthEcmaStrU16CompNo3 = sizeof(arrayU16CompNo3) / sizeof(arrayU16CompNo3[0]);
    JSHandle<EcmaString> handleEcmaStrU16CompNo3(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16CompNo3[0], lengthEcmaStrU16CompNo3, true));

    uint16_t arrayU16CompNo4[] = {25645, 25692, 25678}; // 25645 % 256 == 45...
    uint32_t lengthEcmaStrU16CompNo4 = sizeof(arrayU16CompNo4) / sizeof(arrayU16CompNo4[0]);
    JSHandle<EcmaString> handleEcmaStrU16CompNo4(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16CompNo4[0], lengthEcmaStrU16CompNo4, true));

    EXPECT_TRUE(EcmaStringAccessor::StringsAreEqualUtf16(
        *handleEcmaStrU16CompNo1, &arrayU16CompNo1[0], lengthEcmaStrU16CompNo1));
    EXPECT_FALSE(EcmaStringAccessor::StringsAreEqualUtf16(
        *handleEcmaStrU16CompNo2, &arrayU16CompNo1[0], lengthEcmaStrU16CompNo1));
    EXPECT_FALSE(EcmaStringAccessor::StringsAreEqualUtf16(
        *handleEcmaStrU16CompNo3, &arrayU16CompNo1[0], lengthEcmaStrU16CompNo1));
    EXPECT_TRUE(EcmaStringAccessor::StringsAreEqualUtf16(
        *handleEcmaStrU16CompNo4, &arrayU16CompNo1[0], lengthEcmaStrU16CompNo1));
}

/*
 * @tc.name: StringsAreEqualUtf16_003
 * @tc.desc: Check whether the bool returned through calling StringsAreEqualUtf16 function with an EcmaString made by
 * CreateFromUtf16( , , , false) and an Array(uint16_t) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, StringsAreEqualUtf16_003)
{
    // StringsAreEqualUtf16(). EcmaString made by CreateFromUtf16( , , , false), Array:U16(0-65535).
    uint16_t arrayU16NotCompNo1[] = {25645, 25692, 25678};
    uint16_t arrayU16NotCompNo2[] = {25645, 25692, 78}; // 25645 % 256 == 45...
    uint16_t arrayU16NotCompNo3[] = {25645, 25692, 25678, 65535};
    uint16_t arrayU16NotCompNo4[] = {25645, 25692};
    uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
    uint32_t lengthEcmaStrU16NotCompNo2 = sizeof(arrayU16NotCompNo2) / sizeof(arrayU16NotCompNo2[0]);
    uint32_t lengthEcmaStrU16NotCompNo3 = sizeof(arrayU16NotCompNo3) / sizeof(arrayU16NotCompNo3[0]);
    uint32_t lengthEcmaStrU16NotCompNo4 = sizeof(arrayU16NotCompNo4) / sizeof(arrayU16NotCompNo4[0]);
    JSHandle<EcmaString> handleEcmaStrU16NotCompNo1(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1, false));
    JSHandle<EcmaString> handleEcmaStrU16NotCompNo2(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotCompNo2[0], lengthEcmaStrU16NotCompNo2, false));
    JSHandle<EcmaString> handleEcmaStrU16NotCompNo3(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotCompNo3[0], lengthEcmaStrU16NotCompNo3, false));
    JSHandle<EcmaString> handleEcmaStrU16NotCompNo4(thread,
        EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotCompNo4[0], lengthEcmaStrU16NotCompNo4, false));
    EXPECT_TRUE(EcmaStringAccessor::StringsAreEqualUtf16(*handleEcmaStrU16NotCompNo1, &arrayU16NotCompNo1[0],
        lengthEcmaStrU16NotCompNo1));
    EXPECT_FALSE(EcmaStringAccessor::StringsAreEqualUtf16(*handleEcmaStrU16NotCompNo1, &arrayU16NotCompNo2[0],
        lengthEcmaStrU16NotCompNo2));
    EXPECT_FALSE(EcmaStringAccessor::StringsAreEqualUtf16(*handleEcmaStrU16NotCompNo2, &arrayU16NotCompNo1[0],
        lengthEcmaStrU16NotCompNo1));
    EXPECT_FALSE(EcmaStringAccessor::StringsAreEqualUtf16(*handleEcmaStrU16NotCompNo3, &arrayU16NotCompNo1[0],
        lengthEcmaStrU16NotCompNo1));
    EXPECT_FALSE(EcmaStringAccessor::StringsAreEqualUtf16(*handleEcmaStrU16NotCompNo4, &arrayU16NotCompNo1[0],
        lengthEcmaStrU16NotCompNo1));
}

/*
 * @tc.name: EqualToSplicedString_001
 * @tc.desc: Tests whether the source string is equal to the concatenated string.
 * is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, EqualToSplicedString_001)
{
    ObjectFactory* factory = instance->GetFactory();
    {
        JSHandle<EcmaString> sourceString = factory->NewFromUtf8("Start开始");
        JSHandle<EcmaString> firstString = factory->NewFromASCII("Start");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("开始");
        EXPECT_TRUE(EcmaStringAccessor(sourceString).IsUtf16());
        EXPECT_TRUE(EcmaStringAccessor(firstString).IsUtf8());
        EXPECT_TRUE(EcmaStringAccessor(secondString).IsUtf16());
        bool result = EcmaStringAccessor(sourceString).EqualToSplicedString(*firstString, *secondString);
        EXPECT_TRUE(result);
    }

    {
        JSHandle<EcmaString> sourceString = factory->NewFromUtf8("Start开始");
        JSHandle<EcmaString> firstString = factory->NewFromASCII("Start");
        JSHandle<EcmaString> secondString = factory->NewFromASCII("start");
        EXPECT_TRUE(EcmaStringAccessor(sourceString).IsUtf16());
        EXPECT_TRUE(EcmaStringAccessor(firstString).IsUtf8());
        EXPECT_TRUE(EcmaStringAccessor(secondString).IsUtf8());
        bool result = EcmaStringAccessor(sourceString).EqualToSplicedString(*firstString, *secondString);
        EXPECT_TRUE(!result);
    }

    {
        JSHandle<EcmaString> sourceString = factory->NewFromUtf8("Start开始");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("Start开");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("始");
        EXPECT_TRUE(EcmaStringAccessor(sourceString).IsUtf16());
        EXPECT_TRUE(EcmaStringAccessor(firstString).IsUtf16());
        EXPECT_TRUE(EcmaStringAccessor(secondString).IsUtf16());
        bool result = EcmaStringAccessor(sourceString).EqualToSplicedString(*firstString, *secondString);
        EXPECT_TRUE(result);
    }

    {
        JSHandle<EcmaString> sourceString = factory->NewFromUtf8("Startstart");
        JSHandle<EcmaString> firstString = factory->NewFromASCII("Start");
        JSHandle<EcmaString> secondString = factory->NewFromASCII("start");
        EXPECT_TRUE(EcmaStringAccessor(sourceString).IsUtf8());
        EXPECT_TRUE(EcmaStringAccessor(firstString).IsUtf8());
        EXPECT_TRUE(EcmaStringAccessor(secondString).IsUtf8());
        bool result = EcmaStringAccessor(sourceString).EqualToSplicedString(*firstString, *secondString);
        EXPECT_TRUE(result);
    }

    {
        JSHandle<EcmaString> sourceString = factory->NewFromUtf8("Startstart");
        JSHandle<EcmaString> firstString = factory->NewFromASCII("Start");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("开始");
        EXPECT_TRUE(EcmaStringAccessor(sourceString).IsUtf8());
        EXPECT_TRUE(EcmaStringAccessor(firstString).IsUtf8());
        EXPECT_TRUE(EcmaStringAccessor(secondString).IsUtf16());
        bool result = EcmaStringAccessor(sourceString).EqualToSplicedString(*firstString, *secondString);
        EXPECT_TRUE(!result);
    }

    {
        JSHandle<EcmaString> sourceString = factory->NewFromUtf8("Startstat");
        JSHandle<EcmaString> firstString = factory->NewFromASCII("Start");
        JSHandle<EcmaString> secondString = factory->NewFromASCII("start");
        EXPECT_TRUE(EcmaStringAccessor(sourceString).IsUtf8());
        EXPECT_TRUE(EcmaStringAccessor(firstString).IsUtf8());
        EXPECT_TRUE(EcmaStringAccessor(secondString).IsUtf8());
        bool result = EcmaStringAccessor(sourceString).EqualToSplicedString(*firstString, *secondString);
        EXPECT_TRUE(!result);
    }

    {
        JSHandle<EcmaString> sourceString = factory->NewFromUtf8("Start开始");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("Stat开");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("始");
        EXPECT_TRUE(EcmaStringAccessor(sourceString).IsUtf16());
        EXPECT_TRUE(EcmaStringAccessor(firstString).IsUtf16());
        EXPECT_TRUE(EcmaStringAccessor(secondString).IsUtf16());
        bool result = EcmaStringAccessor(sourceString).EqualToSplicedString(*firstString, *secondString);
        EXPECT_TRUE(!result);
    }

    {
        JSHandle<EcmaString> sourceString = factory->NewFromUtf8("Start开始");
        JSHandle<EcmaString> firstString = factory->NewFromASCII("Stat");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("开始");
        EXPECT_TRUE(EcmaStringAccessor(sourceString).IsUtf16());
        EXPECT_TRUE(EcmaStringAccessor(firstString).IsUtf8());
        EXPECT_TRUE(EcmaStringAccessor(secondString).IsUtf16());
        bool result = EcmaStringAccessor(sourceString).EqualToSplicedString(*firstString, *secondString);
        EXPECT_TRUE(!result);
    }
}

/*
 * @tc.name: EqualToSplicedString_002
 * @tc.desc: Tests whether the source string is equal to the concatenated string.
 * is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, EqualToSplicedString_002)
{
    ObjectFactory* factory = instance->GetFactory();
    {
        JSHandle<EcmaString> allString = factory->NewFromUtf8("123A");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("123");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("A");
        JSHandle<EcmaString> ans = factory->ConcatFromString(firstString, secondString);
        bool result = EcmaStringAccessor::StringsAreEqual(*allString, *ans);
        EXPECT_TRUE(result);
    }

    {
        JSHandle<EcmaString> allString = factory->NewFromUtf8("A123");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("A");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("123");
        JSHandle<EcmaString> ans = factory->ConcatFromString(firstString, secondString);
        bool result = EcmaStringAccessor::StringsAreEqual(*allString, *ans);
        EXPECT_TRUE(result);
    }

    {
        JSHandle<EcmaString> allString = factory->NewFromUtf8("123456789A");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("123456789");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("A");
        JSHandle<EcmaString> ans = factory->ConcatFromString(firstString, secondString);
        bool result = EcmaStringAccessor::StringsAreEqual(*allString, *ans);
        EXPECT_TRUE(result);
    }

    {
        JSHandle<EcmaString> allString = factory->NewFromUtf8("123456789011A");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("123456789011");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("A");
        JSHandle<EcmaString> ans = factory->ConcatFromString(firstString, secondString);
        bool result = EcmaStringAccessor::StringsAreEqual(*allString, *ans);
        EXPECT_TRUE(result);
    }
}

/*
 * @tc.name: EqualToSplicedString_003
 * @tc.desc: Tests whether the source string is equal to the concatenated string.
 * is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, EqualToSplicedString_003)
{
    ObjectFactory* factory = instance->GetFactory();
    {
        JSHandle<EcmaString> allString = factory->NewFromUtf8("12345678901");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("123456789");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("01");
        JSHandle<EcmaString> ans = factory->ConcatFromString(firstString, secondString);
        bool result = EcmaStringAccessor::StringsAreEqual(*allString, *ans);
        EXPECT_TRUE(result);
    }

    {
        JSHandle<EcmaString> allString = factory->NewFromUtf8("12345678901");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("1234567890");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("1");
        JSHandle<EcmaString> ans = factory->ConcatFromString(firstString, secondString);
        bool result = EcmaStringAccessor::StringsAreEqual(*allString, *ans);
        EXPECT_TRUE(result);
    }

    {
        JSHandle<EcmaString> allString = factory->NewFromUtf8("12345678901");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("123456789");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("01");
        JSHandle<EcmaString> ans = factory->ConcatFromString(firstString, secondString);
        bool result = EcmaStringAccessor::StringsAreEqual(*allString, *ans);
        EXPECT_TRUE(result);
    }

    {
        JSHandle<EcmaString> allString = factory->NewFromUtf8("12345678901");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("12345678");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("901");
        JSHandle<EcmaString> ans = factory->ConcatFromString(firstString, secondString);
        bool result = EcmaStringAccessor::StringsAreEqual(*allString, *ans);
        EXPECT_TRUE(result);
    }

    {
        JSHandle<EcmaString> allString = factory->NewFromUtf8("12345678901");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("1234567");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("8901");
        JSHandle<EcmaString> ans = factory->ConcatFromString(firstString, secondString);
        bool result = EcmaStringAccessor::StringsAreEqual(*allString, *ans);
        EXPECT_TRUE(result);
    }

    {
        JSHandle<EcmaString> allString = factory->NewFromUtf8("123456789");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("12345");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("6789");
        JSHandle<EcmaString> ans = factory->ConcatFromString(firstString, secondString);
        bool result = EcmaStringAccessor::StringsAreEqual(*allString, *ans);
        EXPECT_TRUE(result);
    }

    {
        JSHandle<EcmaString> allString = factory->NewFromUtf8("123456789");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("1234");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("56789");
        JSHandle<EcmaString> ans = factory->ConcatFromString(firstString, secondString);
        bool result = EcmaStringAccessor::StringsAreEqual(*allString, *ans);
        EXPECT_TRUE(result);
    }
}

/*
 * @tc.name: EqualToSplicedString_004
 * @tc.desc: Tests whether the source string is equal to the concatenated string.
 * is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */

HWTEST_F_L0(EcmaStringAccessorTest, EqualToSplicedString_004)
{
    ObjectFactory* factory = instance->GetFactory();
    {
        JSHandle<EcmaString> allString = factory->NewFromUtf8("你好在干嘛");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("你好");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("在干嘛");
        JSHandle<EcmaString> ans = factory->ConcatFromString(firstString, secondString);
        bool result = EcmaStringAccessor::StringsAreEqual(*allString, *ans);
        EXPECT_TRUE(result);
    }

    {
        JSHandle<EcmaString> allString = factory->NewFromUtf8("你吃饭了么");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("你吃");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("饭了么");
        JSHandle<EcmaString> ans = factory->ConcatFromString(firstString, secondString);
        bool result = EcmaStringAccessor::StringsAreEqual(*allString, *ans);
        EXPECT_TRUE(result);
    }

    {
        JSHandle<EcmaString> allString = factory->NewFromUtf8("你你你你你你你你你我我我我我我我我我");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("你你你你你你你你你");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("我我我我我我我我我");
        JSHandle<EcmaString> ans = factory->ConcatFromString(firstString, secondString);
        bool result = EcmaStringAccessor::StringsAreEqual(*allString, *ans);
        EXPECT_TRUE(result);
    }
}

/*
 * @tc.name: EqualToSplicedString_005
 * @tc.desc: Tests whether the source string is equal to the concatenated string.
 * is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, EqualToSplicedString_005)
{
    ObjectFactory* factory = instance->GetFactory();
    {
        JSHandle<EcmaString> allString = factory->NewFromUtf8("你好233");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("你");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("好233");
        JSHandle<EcmaString> ans = factory->ConcatFromString(firstString, secondString);
        bool result = EcmaStringAccessor::StringsAreEqual(*allString, *ans);
        EXPECT_TRUE(result);
    }

    {
        JSHandle<EcmaString> allString = factory->NewFromUtf8("你好abc");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("你好");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("abc");
        JSHandle<EcmaString> ans = factory->ConcatFromString(firstString, secondString);
        bool result = EcmaStringAccessor::StringsAreEqual(*allString, *ans);
        EXPECT_TRUE(result);
    }

    {
        JSHandle<EcmaString> allString = factory->NewFromUtf8("233你好");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("233");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("你好");
        JSHandle<EcmaString> ans = factory->ConcatFromString(firstString, secondString);
        bool result = EcmaStringAccessor::StringsAreEqual(*allString, *ans);
        EXPECT_TRUE(result);
    }

    {
        JSHandle<EcmaString> allString = factory->NewFromUtf8("abc你好");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("abc");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("你好");
        JSHandle<EcmaString> ans = factory->ConcatFromString(firstString, secondString);
        bool result = EcmaStringAccessor::StringsAreEqual(*allString, *ans);
        EXPECT_TRUE(result);
    }

    {
        JSHandle<EcmaString> allString = factory->NewFromUtf8("你吃饭了么abc");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("你吃");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("饭了么abc");
        JSHandle<EcmaString> ans = factory->ConcatFromString(firstString, secondString);
        bool result = EcmaStringAccessor::StringsAreEqual(*allString, *ans);
        EXPECT_TRUE(result);
    }

    {
        JSHandle<EcmaString> allString = factory->NewFromUtf8("你吃饭了么abc");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("你吃饭了么");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("abc");
        JSHandle<EcmaString> ans = factory->ConcatFromString(firstString, secondString);
        bool result = EcmaStringAccessor::StringsAreEqual(*allString, *ans);
        EXPECT_TRUE(result);
    }
}

/*
 * @tc.name: EqualToSplicedString_006
 * @tc.desc: Tests whether the source string is equal to the concatenated string.
 * is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, EqualToSplicedString_006)
{
    ObjectFactory* factory = instance->GetFactory();
    {
        JSHandle<EcmaString> allString = factory->NewFromUtf8("你好");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("你好");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("");
        JSHandle<EcmaString> ans = factory->ConcatFromString(firstString, secondString);
        bool result = EcmaStringAccessor::StringsAreEqual(*allString, *ans);
        EXPECT_TRUE(result);
    }

    {
        JSHandle<EcmaString> allString = factory->NewFromUtf8("你好");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("你好");
        JSHandle<EcmaString> ans = factory->ConcatFromString(firstString, secondString);
        bool result = EcmaStringAccessor::StringsAreEqual(*allString, *ans);
        EXPECT_TRUE(result);
    }

    {
        JSHandle<EcmaString> allString = factory->NewFromUtf8("233");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("233");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("");
        JSHandle<EcmaString> ans = factory->ConcatFromString(firstString, secondString);
        bool result = EcmaStringAccessor::StringsAreEqual(*allString, *ans);
        EXPECT_TRUE(result);
    }

    {
        JSHandle<EcmaString> allString = factory->NewFromUtf8("233");
        JSHandle<EcmaString> firstString = factory->NewFromUtf8("");
        JSHandle<EcmaString> secondString = factory->NewFromUtf8("233");
        JSHandle<EcmaString> ans = factory->ConcatFromString(firstString, secondString);
        bool result = EcmaStringAccessor::StringsAreEqual(*allString, *ans);
        EXPECT_TRUE(result);
    }
}

/*
 * @tc.name: CanBeCompressed
 * @tc.desc: Check whether the bool returned through calling CanBeCompressed function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, CanBeCompressed)
{
    uint8_t arrayU8[] = {12, 34, 77, 127, 99, 1};
    uint16_t arrayU16Comp[] = {1, 4, 37, 91, 127, 1};
    uint16_t arrayU16NotComp[] = {72, 43, 337, 961, 1317, 65535};
    EXPECT_TRUE(EcmaStringAccessor::CanBeCompressed(arrayU8, sizeof(arrayU8) / sizeof(arrayU8[0])));
    EXPECT_TRUE(EcmaStringAccessor::CanBeCompressed(arrayU16Comp, sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0])));
    EXPECT_FALSE(EcmaStringAccessor::CanBeCompressed(arrayU16NotComp, sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0])));
}

/*
 * @tc.name: TryToLower
 * @tc.desc: Check whether the EcmaString created through calling TryToLower function is within expectations.
 * is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringAccessorTest, TryToLower)
{
    ObjectFactory* factory = instance->GetFactory();
    JSHandle<EcmaString> lowerStr = factory->NewFromASCII("aaabbbcccddd");
    JSHandle<EcmaString> upperStr = factory->NewFromASCII("AAABBBCCCDDD");
    JSHandle<EcmaString> testStr1 = factory->NewFromASCII("aaaBBBcccDDD");
    JSHandle<EcmaString> testStr2 = factory->NewFromASCII("AAAbbbcccDDD");
    std::vector<JSHandle<EcmaString>> caseStrings{upperStr, testStr1, testStr2};
    EcmaTestCommon::TryLowerCommonCase(thread, instance, lowerStr, caseStrings);
}
}  // namespace panda::test
