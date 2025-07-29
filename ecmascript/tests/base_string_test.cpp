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

#include "ecmascript/ecma_string-inl.h"
#include "common_interfaces/objects/string/base_string_impl.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda::ecmascript;

namespace panda::test {
    class BaseStringTest : public BaseTestWithScope<false> {
    public:
        static void SetUpTestCase()
        {
            GTEST_LOG_(INFO) << "SetUpTestCase";
            uint8_t arrayU8[] = {12, 34, 77, 127, 99, 1};
            uint16_t arrayU16Comp[] = {1, 4, 37, 91, 127, 1};
            uint16_t arrayU16NotComp[] = {72, 43, 337, 961, 1317, 65535};
            EXPECT_TRUE(BaseString::CanBeCompressed(arrayU8, sizeof(arrayU8) / sizeof(arrayU8[0])));
            EXPECT_TRUE(BaseString::CanBeCompressed(arrayU16Comp, sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0])));
            EXPECT_FALSE(BaseString::CanBeCompressed(arrayU16NotComp, sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0])));
        }

        auto LoadBarrier()
        {
            return [this](const void *obj, size_t offset) { return Barriers::GetTaggedObject(thread, obj, offset); };
        }
        auto LoadTBarrier()
        {
            return [this](const void *obj, size_t offset) { return Barriers::GetTaggedObject<>(thread, obj, offset); };
        }
    };

    /*
     * @tc.name: CanBeCompressed
     * @tc.desc: Check whether the bool returned through calling CanBeCompressed function is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, CanBeCompressed)
    {
        uint8_t arrayU8[] = {12, 34, 77, 127, 99, 1};
        uint16_t arrayU16Comp[] = {1, 4, 37, 91, 127, 1};
        uint16_t arrayU16NotComp[] = {72, 43, 337, 961, 1317, 65535};
        EXPECT_TRUE(BaseString::CanBeCompressed(arrayU8, sizeof(arrayU8) / sizeof(arrayU8[0])));
        EXPECT_TRUE(BaseString::CanBeCompressed(arrayU16Comp, sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0])));
        EXPECT_FALSE(BaseString::CanBeCompressed(arrayU16NotComp, sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0])));
    }

    /*
     * @tc.name: CreateEmptyString
     * @tc.desc: Check whether the EcmaString created through calling CreateEmptyString function is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, CreateEmptyString)
    {
        JSHandle<EcmaString> handleEcmaStrEmpty(thread, EcmaStringAccessor::CreateEmptyString(instance));
        EXPECT_EQ(handleEcmaStrEmpty->ToBaseString()->GetLength(), 0U);
        EXPECT_TRUE(handleEcmaStrEmpty->ToBaseString()->IsUtf8());
        EXPECT_FALSE(handleEcmaStrEmpty->ToBaseString()->IsUtf16());
    }

    /*
     * @tc.name: CreateLineString
     * @tc.desc: Check whether the EcmaString created through calling CreateLineString function is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, CreateLineString)
    {
        // CreateLineString( , true, ).
        size_t sizeAllocComp = 5;
        JSHandle<EcmaString> handleEcmaStrAllocComp(
            thread, EcmaStringAccessor::CreateLineString(instance, sizeAllocComp, true));
        EXPECT_EQ(handleEcmaStrAllocComp->ToBaseString()->GetLength(), sizeAllocComp);
        EXPECT_TRUE(handleEcmaStrAllocComp->ToBaseString()->IsUtf8());
        EXPECT_FALSE(handleEcmaStrAllocComp->ToBaseString()->IsUtf16());

        // CreateLineString( , false, ).
        size_t sizeAllocNotComp = 5;
        JSHandle<EcmaString> handleEcmaStrAllocNotComp(thread,
                                                       EcmaStringAccessor::CreateLineString(
                                                           instance, sizeAllocNotComp, false));
        EXPECT_EQ(handleEcmaStrAllocNotComp->ToBaseString()->GetLength(), sizeAllocNotComp);
        EXPECT_FALSE(handleEcmaStrAllocNotComp->ToBaseString()->IsUtf8());
        EXPECT_TRUE(handleEcmaStrAllocNotComp->ToBaseString()->IsUtf16());
    }

    /*
     * @tc.name: CreateFromUtf8
     * @tc.desc: Check whether the EcmaString created through calling CreateFromUtf8 function is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, CreateFromUtf8)
    {
        uint8_t arrayU8[] = {"xyz123!@#"};
        size_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
        JSHandle<EcmaString> handleEcmaStrU8(thread,
                                             EcmaStringAccessor::CreateFromUtf8(
                                                 instance, &arrayU8[0], lengthEcmaStrU8, true));
        for (uint32_t i = 0; i < lengthEcmaStrU8; i++) {
            EXPECT_EQ(arrayU8[i], handleEcmaStrU8->ToBaseString()->At(LoadTBarrier(), i));
        }
        EXPECT_EQ(handleEcmaStrU8->ToBaseString()->GetLength(), lengthEcmaStrU8);
        EXPECT_TRUE(handleEcmaStrU8->ToBaseString()->IsUtf8());
        EXPECT_FALSE(handleEcmaStrU8->ToBaseString()->IsUtf16());
    }

    /*
     * @tc.name: CreateFromUtf16
     * @tc.desc: Check whether the EcmaString created through calling CreateFromUtf16 function is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, CreateFromUtf16)
    {
        // CreateFromUtf16( , , , true).
        uint16_t arrayU16Comp[] = {1, 23, 45, 67, 127};
        size_t lengthEcmaStrU16Comp = sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0]);
        JSHandle<EcmaString> handleEcmaStrU16Comp(thread,
                                                  EcmaStringAccessor::CreateFromUtf16(
                                                      instance, &arrayU16Comp[0], lengthEcmaStrU16Comp, true));
        EXPECT_EQ(handleEcmaStrU16Comp->ToBaseString()->GetLength(), lengthEcmaStrU16Comp);
        EXPECT_TRUE(handleEcmaStrU16Comp->ToBaseString()->IsUtf8());
        EXPECT_FALSE(handleEcmaStrU16Comp->ToBaseString()->IsUtf16());

        // CreateFromUtf16( , , , false).
        uint16_t arrayU16NotComp[] = {127, 33, 128, 12, 256, 11100, 65535};
        size_t lengthEcmaStrU16NotComp = sizeof(arrayU16NotComp) / sizeof(arrayU16NotComp[0]);
        JSHandle<EcmaString> handleEcmaStrU16NotComp(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16NotComp[0], lengthEcmaStrU16NotComp,
                                                         false));
        EXPECT_EQ(handleEcmaStrU16NotComp->ToBaseString()->GetLength(), lengthEcmaStrU16NotComp);
        EXPECT_FALSE(handleEcmaStrU16NotComp->ToBaseString()->IsUtf8());
        EXPECT_TRUE(handleEcmaStrU16NotComp->ToBaseString()->IsUtf16());
    }

    /*
     * @tc.name: ObjectSize
     * @tc.desc: Check whether the value returned through calling ObjectSize function is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, ObjectSize)
    {
        JSHandle<EcmaString> handleEcmaStrEmpty(thread, EcmaStringAccessor::CreateEmptyString(instance));
        EXPECT_EQ(EcmaStringAccessor(handleEcmaStrEmpty).ObjectSize(), BaseString::SIZE + 0);

        size_t lengthEcmaStrAllocComp = 5;
        JSHandle<EcmaString> handleEcmaStrAllocComp(thread,
                                                    EcmaStringAccessor::CreateLineString(
                                                        instance, lengthEcmaStrAllocComp, true));
        EXPECT_EQ(EcmaStringAccessor(handleEcmaStrAllocComp).ObjectSize(),
                  BaseString::SIZE + sizeof(uint8_t) * lengthEcmaStrAllocComp);

        size_t lengthEcmaStrAllocNotComp = 5;
        JSHandle<EcmaString> handleEcmaStrAllocNotComp(thread,
                                                       EcmaStringAccessor::CreateLineString(
                                                           instance, lengthEcmaStrAllocNotComp, false));
        EXPECT_EQ(EcmaStringAccessor(handleEcmaStrAllocNotComp).ObjectSize(),
                  BaseString::SIZE + sizeof(uint16_t) * lengthEcmaStrAllocNotComp);

        uint8_t arrayU8[] = {"abcde"};
        size_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
        JSHandle<EcmaString> handleEcmaStrU8(thread,
                                             EcmaStringAccessor::CreateFromUtf8(
                                                 instance, &arrayU8[0], lengthEcmaStrU8, true));
        EXPECT_EQ(EcmaStringAccessor(handleEcmaStrU8).ObjectSize(),
                  BaseString::SIZE + sizeof(uint8_t) * lengthEcmaStrU8);

        // ObjectSize(). EcmaString made by CreateFromUtf16( , , , true).
        uint16_t arrayU16Comp[] = {1, 23, 45, 67, 127};
        size_t lengthEcmaStrU16Comp = sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0]);
        JSHandle<EcmaString> handleEcmaStrU16Comp(thread,
                                                  EcmaStringAccessor::CreateFromUtf16(
                                                      instance, &arrayU16Comp[0], lengthEcmaStrU16Comp, true));
        EXPECT_EQ(EcmaStringAccessor(handleEcmaStrU16Comp).ObjectSize(),
                  BaseString::SIZE + sizeof(uint8_t) * lengthEcmaStrU16Comp);

        // ObjectSize(). EcmaString made by CreateFromUtf16( , , , false).
        uint16_t arrayU16NotComp[] = {127, 128, 256, 11100, 65535};
        size_t lengthEcmaStrU16NotComp = sizeof(arrayU16NotComp) / sizeof(arrayU16NotComp[0]);
        JSHandle<EcmaString> handleEcmaStrU16NotComp(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16NotComp[0], lengthEcmaStrU16NotComp,
                                                         false));
        EXPECT_EQ(EcmaStringAccessor(handleEcmaStrU16NotComp).ObjectSize(),
                  BaseString::SIZE + sizeof(uint16_t) * lengthEcmaStrU16NotComp);
    }

    /*
     * @tc.name: Compare_001
     * @tc.desc: Check whether the value returned through calling Compare function between EcmaStrings made by
     * CreateFromUtf8() is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, Compare_001)
    {
        // Compare(). Between EcmaStrings made by CreateFromUtf8().
        uint8_t arrayU8No1[3] = {1, 23};
        uint8_t arrayU8No2[4] = {1, 23, 49};
        uint8_t arrayU8No3[6] = {1, 23, 45, 97, 127};
        uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
        uint32_t lengthEcmaStrU8No2 = sizeof(arrayU8No2) - 1;
        uint32_t lengthEcmaStrU8No3 = sizeof(arrayU8No3) - 1;
        JSHandle<EcmaString> handleEcmaStrU8No1(thread,
                                                EcmaStringAccessor::CreateFromUtf8(
                                                    instance, &arrayU8No1[0], lengthEcmaStrU8No1, true));
        JSHandle<EcmaString> handleEcmaStrU8No2(thread,
                                                EcmaStringAccessor::CreateFromUtf8(
                                                    instance, &arrayU8No2[0], lengthEcmaStrU8No2, true));
        JSHandle<EcmaString> handleEcmaStrU8No3(thread,
                                                EcmaStringAccessor::CreateFromUtf8(
                                                    instance, &arrayU8No3[0], lengthEcmaStrU8No3, true));
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
    HWTEST_F_L0(BaseStringTest, Compare_002)
    {
        // Compare(). Between EcmaStrings made by CreateFromUtf16( , , , true).
        uint16_t arrayU16CompNo1[] = {1, 23};
        uint16_t arrayU16CompNo2[] = {1, 23, 49};
        uint16_t arrayU16CompNo3[] = {1, 23, 45, 97, 127};
        uint32_t lengthEcmaStrU16CompNo1 = sizeof(arrayU16CompNo1) / sizeof(arrayU16CompNo1[0]);
        uint32_t lengthEcmaStrU16CompNo2 = sizeof(arrayU16CompNo2) / sizeof(arrayU16CompNo2[0]);
        uint32_t lengthEcmaStrU16CompNo3 = sizeof(arrayU16CompNo3) / sizeof(arrayU16CompNo3[0]);
        JSHandle<EcmaString> handleEcmaStrU16CompNo1(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16CompNo1[0], lengthEcmaStrU16CompNo1, true));
        JSHandle<EcmaString> handleEcmaStrU16CompNo2(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16CompNo2[0], lengthEcmaStrU16CompNo2, true));
        JSHandle<EcmaString> handleEcmaStrU16CompNo3(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16CompNo3[0], lengthEcmaStrU16CompNo3, true));
        EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16CompNo1, handleEcmaStrU16CompNo2), -1);
        EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16CompNo2, handleEcmaStrU16CompNo1), 1);
        EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16CompNo2, handleEcmaStrU16CompNo3), 49 - 45);
        EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16CompNo3, handleEcmaStrU16CompNo2), 45 - 49);
    }

    /*
     * @tc.name: Compare_003
     * @tc.desc: Check whether the value returned through calling Compare function between EcmaString made by
     * CreateFromUtf8() and EcmaString made by CreateFromUtf16( , , , true) made by CreateFromUtf16( , , , true) is
     * within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, Compare_003)
    {
        // Compare(). EcmaString made by CreateFromUtf8() and EcmaString made by CreateFromUtf16( , , , true).
        uint8_t arrayU8No1[3] = {1, 23};
        uint8_t arrayU8No2[4] = {1, 23, 49};
        uint16_t arrayU16CompNo1[] = {1, 23};
        uint16_t arrayU16CompNo2[] = {1, 23, 49};
        uint16_t arrayU16CompNo3[] = {1, 23, 45, 97, 127};
        uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
        uint32_t lengthEcmaStrU8No2 = sizeof(arrayU8No2) - 1;
        uint32_t lengthEcmaStrU16CompNo1 = sizeof(arrayU16CompNo1) / sizeof(arrayU16CompNo1[0]);
        uint32_t lengthEcmaStrU16CompNo2 = sizeof(arrayU16CompNo2) / sizeof(arrayU16CompNo2[0]);
        uint32_t lengthEcmaStrU16CompNo3 = sizeof(arrayU16CompNo3) / sizeof(arrayU16CompNo3[0]);
        JSHandle<EcmaString> handleEcmaStrU8No1(thread,
                                                EcmaStringAccessor::CreateFromUtf8(
                                                    instance, &arrayU8No1[0], lengthEcmaStrU8No1, true));
        JSHandle<EcmaString> handleEcmaStrU8No2(thread,
                                                EcmaStringAccessor::CreateFromUtf8(
                                                    instance, &arrayU8No2[0], lengthEcmaStrU8No2, true));
        JSHandle<EcmaString> handleEcmaStrU16CompNo1(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16CompNo1[0], lengthEcmaStrU16CompNo1, true));
        JSHandle<EcmaString> handleEcmaStrU16CompNo2(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16CompNo2[0], lengthEcmaStrU16CompNo2, true));
        JSHandle<EcmaString> handleEcmaStrU16CompNo3(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16CompNo3[0], lengthEcmaStrU16CompNo3, true));
        EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU8No1, handleEcmaStrU16CompNo1), 0);
        EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16CompNo1, handleEcmaStrU8No1), 0);
        EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU8No1, handleEcmaStrU16CompNo2), -1);
        EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16CompNo2, handleEcmaStrU8No1), 1);
        EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU8No2, handleEcmaStrU16CompNo3), 49 - 45);
        EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16CompNo3, handleEcmaStrU8No2), 45 - 49);
    }

    /*
     * @tc.name: Compare_004
     * @tc.desc: Check whether the value returned through calling Compare function between EcmaStrings made by
     * CreateFromUtf16( , , , false) is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, Compare_004)
    {
        // Compare(). Between EcmaStrings made by CreateFromUtf16( , , , false).
        uint16_t arrayU16NotCompNo1[] = {1, 23};
        uint16_t arrayU16NotCompNo2[] = {1, 23, 49};
        uint16_t arrayU16NotCompNo3[] = {1, 23, 456, 6789, 65535, 127};
        uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
        uint32_t lengthEcmaStrU16NotCompNo2 = sizeof(arrayU16NotCompNo2) / sizeof(arrayU16NotCompNo2[0]);
        uint32_t lengthEcmaStrU16NotCompNo3 = sizeof(arrayU16NotCompNo3) / sizeof(arrayU16NotCompNo3[0]);
        JSHandle<EcmaString> handleEcmaStrU16NotCompNo1(thread,
                                                        EcmaStringAccessor::CreateFromUtf16(
                                                            instance, &arrayU16NotCompNo1[0],
                                                            lengthEcmaStrU16NotCompNo1, true));
        JSHandle<EcmaString> handleEcmaStrU16NotCompNo2(thread,
                                                        EcmaStringAccessor::CreateFromUtf16(
                                                            instance, &arrayU16NotCompNo2[0],
                                                            lengthEcmaStrU16NotCompNo2, true));
        JSHandle<EcmaString> handleEcmaStrU16NotCompNo3(thread,
                                                        EcmaStringAccessor::CreateFromUtf16(
                                                            instance, &arrayU16NotCompNo3[0],
                                                            lengthEcmaStrU16NotCompNo3, false));
        EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16NotCompNo1, handleEcmaStrU16NotCompNo2), -1);
        EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16NotCompNo2, handleEcmaStrU16NotCompNo1), 1);
        EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16NotCompNo2, handleEcmaStrU16NotCompNo3),
                  49 - 456);
        EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16NotCompNo3, handleEcmaStrU16NotCompNo2),
                  456 - 49);
    }

    /*
     * @tc.name: Compare_005
     * @tc.desc: Check whether the value returned through calling Compare function between EcmaString made by
     * CreateFromUtf8() and EcmaString made by CreateFromUtf16( , , , false) is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, Compare_005)
    {
        // Compare(). EcmaString made by CreateFromUtf8() and EcmaString made by CreateFromUtf16( , , , false).
        uint8_t arrayU8No1[3] = {1, 23};
        uint8_t arrayU8No2[4] = {1, 23, 49};
        uint16_t arrayU16NotCompNo1[] = {1, 23};
        uint16_t arrayU16NotCompNo2[] = {1, 23, 49};
        uint16_t arrayU16NotCompNo3[] = {1, 23, 456, 6789, 65535, 127};
        uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
        uint32_t lengthEcmaStrU8No2 = sizeof(arrayU8No2) - 1;
        uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
        uint32_t lengthEcmaStrU16NotCompNo2 = sizeof(arrayU16NotCompNo2) / sizeof(arrayU16NotCompNo2[0]);
        uint32_t lengthEcmaStrU16NotCompNo3 = sizeof(arrayU16NotCompNo3) / sizeof(arrayU16NotCompNo3[0]);
        JSHandle<EcmaString> handleEcmaStrU8No1(thread,
                                                EcmaStringAccessor::CreateFromUtf8(
                                                    instance, &arrayU8No1[0], lengthEcmaStrU8No1, true));
        JSHandle<EcmaString> handleEcmaStrU8No2(thread,
                                                EcmaStringAccessor::CreateFromUtf8(
                                                    instance, &arrayU8No2[0], lengthEcmaStrU8No2, true));
        JSHandle<EcmaString> handleEcmaStrU16NotCompNo1(thread,
                                                        EcmaStringAccessor::CreateFromUtf16(
                                                            instance, &arrayU16NotCompNo1[0],
                                                            lengthEcmaStrU16NotCompNo1, true));
        JSHandle<EcmaString> handleEcmaStrU16NotCompNo2(thread,
                                                        EcmaStringAccessor::CreateFromUtf16(
                                                            instance, &arrayU16NotCompNo2[0],
                                                            lengthEcmaStrU16NotCompNo2, true));
        JSHandle<EcmaString> handleEcmaStrU16NotCompNo3(thread,
                                                        EcmaStringAccessor::CreateFromUtf16(
                                                            instance, &arrayU16NotCompNo3[0],
                                                            lengthEcmaStrU16NotCompNo3, false));
        EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU8No1, handleEcmaStrU16NotCompNo1), 0);
        EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16NotCompNo1, handleEcmaStrU8No1), 0);
        EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU8No1, handleEcmaStrU16NotCompNo2), -1);
        EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16NotCompNo2, handleEcmaStrU8No1), 1);
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
    HWTEST_F_L0(BaseStringTest, Compare_006)
    {
        // Compare(). EcmaString made by CreateFromUtf16( , , , true) and EcmaString made by
        // CreateFromUtf16( , , , false).
        uint16_t arrayU16CompNo1[] = {1, 23};
        uint16_t arrayU16CompNo2[] = {1, 23, 49};
        uint16_t arrayU16NotCompNo1[] = {1, 23};
        uint16_t arrayU16NotCompNo2[] = {1, 23, 49};
        uint16_t arrayU16NotCompNo3[] = {1, 23, 456, 6789, 65535, 127};
        uint32_t lengthEcmaStrU16CompNo1 = sizeof(arrayU16CompNo1) / sizeof(arrayU16CompNo1[0]);
        uint32_t lengthEcmaStrU16CompNo2 = sizeof(arrayU16CompNo2) / sizeof(arrayU16CompNo2[0]);
        uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
        uint32_t lengthEcmaStrU16NotCompNo2 = sizeof(arrayU16NotCompNo2) / sizeof(arrayU16NotCompNo2[0]);
        uint32_t lengthEcmaStrU16NotCompNo3 = sizeof(arrayU16NotCompNo3) / sizeof(arrayU16NotCompNo3[0]);
        JSHandle<EcmaString> handleEcmaStrU16CompNo1(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16CompNo1[0], lengthEcmaStrU16CompNo1, true));
        JSHandle<EcmaString> handleEcmaStrU16CompNo2(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16CompNo2[0], lengthEcmaStrU16CompNo2, true));
        JSHandle<EcmaString> handleEcmaStrU16NotCompNo1(thread,
                                                        EcmaStringAccessor::CreateFromUtf16(
                                                            instance, &arrayU16NotCompNo1[0],
                                                            lengthEcmaStrU16NotCompNo1, true));
        JSHandle<EcmaString> handleEcmaStrU16NotCompNo2(thread,
                                                        EcmaStringAccessor::CreateFromUtf16(
                                                            instance, &arrayU16NotCompNo2[0],
                                                            lengthEcmaStrU16NotCompNo2, true));
        JSHandle<EcmaString> handleEcmaStrU16NotCompNo3(thread,
                                                        EcmaStringAccessor::CreateFromUtf16(
                                                            instance, &arrayU16NotCompNo3[0],
                                                            lengthEcmaStrU16NotCompNo3, false));
        EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16CompNo1, handleEcmaStrU16NotCompNo1), 0);
        EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16NotCompNo1, handleEcmaStrU16CompNo1), 0);
        EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16CompNo1, handleEcmaStrU16NotCompNo2), -1);
        EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16NotCompNo2, handleEcmaStrU16CompNo1), 1);
        EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16CompNo2, handleEcmaStrU16NotCompNo3), 49 - 456);
        EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStrU16NotCompNo3, handleEcmaStrU16CompNo2), 456 - 49);
    }

    /*
     * @tc.name: Concat_001
     * @tc.desc: Check whether the EcmaString returned through calling Concat function between EcmaString made by
     * CreateFromUtf8() and EcmaString made by CreateFromUtf8() is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, Concat_001)
    {
        // Concat(). EcmaString made by CreateFromUtf8() and EcmaString made by CreateFromUtf8().
        uint8_t arrayFrontU8[] = {"abcdef"};
        uint8_t arrayBackU8[] = {"ABCDEF"};
        uint32_t lengthEcmaStrFrontU8 = sizeof(arrayFrontU8) - 1;
        uint32_t lengthEcmaStrBackU8 = sizeof(arrayBackU8) - 1;
        JSHandle<EcmaString> handleEcmaStrFrontU8(thread,
                                                  EcmaStringAccessor::CreateFromUtf8(
                                                      instance, &arrayFrontU8[0], lengthEcmaStrFrontU8, true));
        JSHandle<EcmaString> handleEcmaStrBackU8(thread,
                                                 EcmaStringAccessor::CreateFromUtf8(
                                                     instance, &arrayBackU8[0], lengthEcmaStrBackU8, true));
        JSHandle<EcmaString> handleEcmaStrConcatU8(thread,
                                                   EcmaStringAccessor::Concat(
                                                       instance, handleEcmaStrFrontU8, handleEcmaStrBackU8));
        EXPECT_TRUE(handleEcmaStrConcatU8->ToBaseString()->IsUtf8());
        for (uint32_t i = 0; i < lengthEcmaStrFrontU8; i++) {
            EXPECT_EQ(handleEcmaStrConcatU8->ToBaseString()->At(LoadTBarrier(), i), arrayFrontU8[i]);
        }
        for (uint32_t i = 0; i < lengthEcmaStrBackU8; i++) {
            EXPECT_EQ(handleEcmaStrConcatU8->ToBaseString()->At(LoadTBarrier(), i + lengthEcmaStrFrontU8),
                      arrayBackU8[i]);
        }
        EXPECT_EQ(handleEcmaStrConcatU8->ToBaseString()->GetLength(), lengthEcmaStrFrontU8 + lengthEcmaStrBackU8);
    }

    /*
     * @tc.name: Concat_002
     * @tc.desc: Check whether the EcmaString returned through calling Concat function between EcmaString made by
     * CreateFromUtf16( , , , false) and EcmaString made by CreateFromUtf16( , , , false) is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, Concat_002)
    {
        // Concat(). EcmaString made by CreateFromUtf16( , , , false) and EcmaString made by
        // CreateFromUtf16( , , , false).
        uint16_t arrayFrontU16NotComp[] = {128, 129, 256, 11100, 65535, 100};
        uint16_t arrayBackU16NotComp[] = {88, 768, 1, 270, 345, 333};
        uint32_t lengthEcmaStrFrontU16NotComp = sizeof(arrayFrontU16NotComp) / sizeof(arrayFrontU16NotComp[0]);
        uint32_t lengthEcmaStrBackU16NotComp = sizeof(arrayBackU16NotComp) / sizeof(arrayBackU16NotComp[0]);
        JSHandle<EcmaString> handleEcmaStrFrontU16NotComp(thread,
                                                          EcmaStringAccessor::CreateFromUtf16(
                                                              instance, &arrayFrontU16NotComp[0],
                                                              lengthEcmaStrFrontU16NotComp, false));
        JSHandle<EcmaString> handleEcmaStrBackU16NotComp(thread,
                                                         EcmaStringAccessor::CreateFromUtf16(
                                                             instance, &arrayBackU16NotComp[0],
                                                             lengthEcmaStrBackU16NotComp, false));
        JSHandle<EcmaString> handleEcmaStrConcatU16NotComp(thread,
                                                           EcmaStringAccessor::Concat(
                                                               instance, handleEcmaStrFrontU16NotComp,
                                                               handleEcmaStrBackU16NotComp));
        EXPECT_TRUE(handleEcmaStrConcatU16NotComp->ToBaseString()->IsUtf16());
        for (uint32_t i = 0; i < lengthEcmaStrFrontU16NotComp; i++) {
            EXPECT_EQ(handleEcmaStrConcatU16NotComp->ToBaseString()->At(LoadTBarrier(), i), arrayFrontU16NotComp[i]);
        }
        for (uint32_t i = 0; i < lengthEcmaStrBackU16NotComp; i++) {
            EXPECT_EQ(
                handleEcmaStrConcatU16NotComp->ToBaseString()->At(LoadTBarrier(), i + lengthEcmaStrFrontU16NotComp),
                arrayBackU16NotComp[i]);
        }
        EXPECT_EQ(handleEcmaStrConcatU16NotComp->ToBaseString()->GetLength(),
                  lengthEcmaStrFrontU16NotComp + lengthEcmaStrBackU16NotComp);
    }

    /*
     * @tc.name: Concat_003
     * @tc.desc: Check whether the EcmaString returned through calling Concat function between EcmaString made by
     * CreateFromUtf8() and EcmaString made by CreateFromUtf16( , , , false) is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, Concat_003)
    {
        // Concat(). EcmaString made by CreateFromUtf8() and EcmaString made by CreateFromUtf16( , , , false).
        uint8_t arrayFrontU8[] = {"abcdef"};
        uint16_t arrayBackU16NotComp[] = {88, 768, 1, 270, 345, 333};
        uint32_t lengthEcmaStrFrontU8 = sizeof(arrayFrontU8) - 1;
        uint32_t lengthEcmaStrBackU16NotComp = sizeof(arrayBackU16NotComp) / sizeof(arrayBackU16NotComp[0]);
        JSHandle<EcmaString> handleEcmaStrFrontU8(thread,
                                                  EcmaStringAccessor::CreateFromUtf8(
                                                      instance, &arrayFrontU8[0], lengthEcmaStrFrontU8, true));
        JSHandle<EcmaString> handleEcmaStrBackU16NotComp(thread,
                                                         EcmaStringAccessor::CreateFromUtf16(
                                                             instance, &arrayBackU16NotComp[0],
                                                             lengthEcmaStrBackU16NotComp, false));
        JSHandle<EcmaString> handleEcmaStrConcatU8U16NotComp(thread,
                                                             EcmaStringAccessor::Concat(
                                                                 instance, handleEcmaStrFrontU8,
                                                                 handleEcmaStrBackU16NotComp));
        EXPECT_TRUE(handleEcmaStrConcatU8U16NotComp->ToBaseString()->IsUtf16());
        for (uint32_t i = 0; i < lengthEcmaStrFrontU8; i++) {
            EXPECT_EQ(handleEcmaStrConcatU8U16NotComp->ToBaseString()->At(LoadTBarrier(), i), arrayFrontU8[i]);
        }
        for (uint32_t i = 0; i < lengthEcmaStrBackU16NotComp; i++) {
            EXPECT_EQ(handleEcmaStrConcatU8U16NotComp->ToBaseString()->At(LoadTBarrier(), i + lengthEcmaStrFrontU8),
                      arrayBackU16NotComp[i]);
        }
        EXPECT_EQ(handleEcmaStrConcatU8U16NotComp->ToBaseString()->GetLength(),
                  lengthEcmaStrFrontU8 + lengthEcmaStrBackU16NotComp);
    }

    /*
     * @tc.name: FastSubString_001
     * @tc.desc: Check whether the EcmaString returned through calling FastSubString function from EcmaString made by
     * CreateFromUtf8() is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, FastSubString_001)
    {
        // FastSubString(). From EcmaString made by CreateFromUtf8().
        uint8_t arrayU8[6] = {3, 7, 19, 54, 99};
        uint32_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
        JSHandle<EcmaString> handleEcmaStrU8(thread,
                                             EcmaStringAccessor::CreateFromUtf8(
                                                 instance, &arrayU8[0], lengthEcmaStrU8, true));
        uint32_t indexStartSubU8 = 2;
        uint32_t lengthSubU8 = 2;
        JSHandle<EcmaString> handleEcmaStrSubU8(thread,
                                                EcmaStringAccessor::FastSubString(
                                                    instance, handleEcmaStrU8, indexStartSubU8, lengthSubU8));
        for (uint32_t i = 0; i < lengthSubU8; i++) {
            EXPECT_EQ(handleEcmaStrSubU8->ToBaseString()->At(LoadTBarrier(), i),
                      handleEcmaStrU8->ToBaseString()->At(LoadTBarrier(), i + indexStartSubU8));
        }
        EXPECT_EQ(handleEcmaStrSubU8->ToBaseString()->GetLength(), lengthSubU8);
    }

    /*
     * @tc.name: FastSubString_002
     * @tc.desc: Check whether the EcmaString returned through calling FastSubString function from EcmaString made by
     * CreateFromUtf16( , , , true) is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, FastSubString_002)
    {
        // FastSubString(). From EcmaString made by CreateFromUtf16( , , , true).
        uint16_t arrayU16Comp[] = {1, 12, 34, 56, 127};
        uint32_t lengthEcmaStrU16Comp = sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0]);
        JSHandle<EcmaString> handleEcmaStrU16Comp(thread,
                                                  EcmaStringAccessor::CreateFromUtf16(
                                                      instance, &arrayU16Comp[0], lengthEcmaStrU16Comp, true));
        uint32_t indexStartSubU16Comp = 0;
        uint32_t lengthSubU16Comp = 2;
        JSHandle<EcmaString> handleEcmaStrSubU16Comp(thread,
                                                     EcmaStringAccessor::FastSubString(
                                                         instance, handleEcmaStrU16Comp, indexStartSubU16Comp,
                                                         lengthSubU16Comp));
        for (uint32_t i = 0; i < lengthSubU16Comp; i++) {
            EXPECT_EQ(handleEcmaStrSubU16Comp->ToBaseString()->At(LoadTBarrier(), i),
                      handleEcmaStrU16Comp->ToBaseString()->At(LoadTBarrier(), i + indexStartSubU16Comp));
        }
        EXPECT_EQ(handleEcmaStrSubU16Comp->ToBaseString()->GetLength(), lengthSubU16Comp);
    }

    /*
     * @tc.name: FastSubString_003
     * @tc.desc: Check whether the EcmaString returned through calling FastSubString function from EcmaString made by
     * CreateFromUtf16( , , , false) is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, FastSubString_003)
    {
        // FastSubString(). From EcmaString made by CreateFromUtf16( , , , false).
        uint16_t arrayU16NotComp[] = {19, 54, 256, 11100, 65535};
        uint32_t lengthEcmaStrU16NotComp = sizeof(arrayU16NotComp) / sizeof(arrayU16NotComp[0]);
        JSHandle<EcmaString> handleEcmaStrU16NotComp(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16NotComp[0], lengthEcmaStrU16NotComp,
                                                         false));
        uint32_t indexStartSubU16NotComp = 0;
        uint32_t lengthSubU16NotComp = 2;
        JSHandle<EcmaString> handleEcmaStrSubU16NotComp(thread,
                                                        EcmaStringAccessor::FastSubString(
                                                            instance, handleEcmaStrU16NotComp, indexStartSubU16NotComp,
                                                            lengthSubU16NotComp));
        for (uint32_t i = 0; i < lengthSubU16NotComp; i++) {
            EXPECT_EQ(handleEcmaStrSubU16NotComp->ToBaseString()->At(LoadTBarrier(), i),
                      handleEcmaStrU16NotComp->ToBaseString()->At(LoadTBarrier(), i + indexStartSubU16NotComp));
        }
        EXPECT_EQ(handleEcmaStrSubU16NotComp->ToBaseString()->GetLength(), lengthSubU16NotComp);
    }

    /*
     * @tc.name: FastSubString_004
     * @tc.desc: Check whether the EcmaString returned through calling FastSubString function from EcmaString
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, FastSubString_004)
    {
        ObjectFactory* factory = instance->GetFactory();
        {
            JSHandle<EcmaString> sourceString = factory->NewFromUtf8("整数integer");
            JSHandle<EcmaString> tmpString = factory->NewFromASCII("integer");
            EXPECT_TRUE(sourceString->ToBaseString()->IsUtf16());
            EcmaString* res = EcmaStringAccessor::FastSubString(instance, sourceString, 2, 7);
            EXPECT_TRUE(res->ToBaseString()->IsUtf8());
            EXPECT_TRUE(BaseString::StringsAreEqual(LoadTBarrier(), res->ToBaseString(), tmpString->ToBaseString()));
        }
        {
            JSHandle<EcmaString> sourceString = factory->NewFromUtf8("整数integer");
            JSHandle<EcmaString> tmpString = factory->NewFromUtf8("整数");
            EXPECT_TRUE(sourceString->ToBaseString()->IsUtf16());
            EcmaString* res = EcmaStringAccessor::FastSubString(instance, sourceString, 0, 2);
            EXPECT_TRUE(res->ToBaseString()->IsUtf16());
            EXPECT_TRUE(BaseString::StringsAreEqual(LoadTBarrier(), res->ToBaseString(), tmpString->ToBaseString()));
        }
        {
            JSHandle<EcmaString> sourceString = factory->NewFromUtf8("整数integer");
            JSHandle<EcmaString> tmpString = factory->NewFromUtf8("数intege");
            EXPECT_TRUE(sourceString->ToBaseString()->IsUtf16());
            EcmaString* res = EcmaStringAccessor::FastSubString(instance, sourceString, 1, 7);
            EXPECT_TRUE(res->ToBaseString()->IsUtf16());
            EXPECT_TRUE(BaseString::StringsAreEqual(LoadTBarrier(), res->ToBaseString(), tmpString->ToBaseString()));
        }
        {
            JSHandle<EcmaString> sourceString = factory->NewFromASCII("integer123");
            JSHandle<EcmaString> tmpString = factory->NewFromASCII("integer");
            EXPECT_TRUE(sourceString->ToBaseString()->IsUtf8());
            EcmaString* res = EcmaStringAccessor::FastSubString(instance, sourceString, 0, 7);
            EXPECT_TRUE(res->ToBaseString()->IsUtf8());
            EXPECT_TRUE(BaseString::StringsAreEqual(LoadTBarrier(), res->ToBaseString(), tmpString->ToBaseString()));
        }
    }

    /*
     * @tc.name: WriteData_001
     * @tc.desc: Check whether the target EcmaString made by CreateLineString( , true, ) changed through calling
     * WriteData
     * function with a source EcmaString made by CreateFromUtf8() is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, WriteData_001)
    {
        // WriteData(). From EcmaString made by CreateFromUtf8() to EcmaString made by CreateLineString( , true, ).
        uint8_t arrayU8WriteFrom[6] = {1, 12, 34, 56, 127};
        uint32_t lengthEcmaStrU8WriteFrom = sizeof(arrayU8WriteFrom) - 1;
        JSHandle<EcmaString> handleEcmaStrU8WriteFrom(thread,
                                                      EcmaStringAccessor::CreateFromUtf8(
                                                          instance, &arrayU8WriteFrom[0], lengthEcmaStrU8WriteFrom,
                                                          true));
        size_t sizeEcmaStrU8WriteTo = 5;
        JSHandle<EcmaString> handleEcmaStrAllocTrueWriteTo(thread,
                                                           EcmaStringAccessor::CreateLineString(
                                                               instance, sizeEcmaStrU8WriteTo, true));
        uint32_t indexStartWriteFromArrayU8 = 2;
        uint32_t lengthWriteFromArrayU8 = 2;
        handleEcmaStrAllocTrueWriteTo->ToBaseString()->WriteData(
            LoadTBarrier(), handleEcmaStrU8WriteFrom->ToBaseString(), indexStartWriteFromArrayU8, sizeEcmaStrU8WriteTo,
            lengthWriteFromArrayU8);
        for (uint32_t i = 0; i < lengthWriteFromArrayU8; i++) {
            EXPECT_EQ(handleEcmaStrAllocTrueWriteTo->ToBaseString()->At(LoadTBarrier(), i + indexStartWriteFromArrayU8),
                      handleEcmaStrU8WriteFrom->ToBaseString()->At(LoadTBarrier(), i));
        }
    }

    /*
     * @tc.name: WriteData_002
     * @tc.desc: Check whether the target EcmaString made by CreateLineString( , true, ) changed through calling
     * WriteData
     * function from a source char is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, WriteData_002)
    {
        // WriteData(). From char to EcmaString made by CreateLineString( , true, ).
        char u8Write = 'a';
        size_t sizeEcmaStrU8WriteTo = 5;
        JSHandle<EcmaString> handleEcmaStrAllocTrueWriteTo(thread,
                                                           EcmaStringAccessor::CreateLineString(
                                                               instance, sizeEcmaStrU8WriteTo, true));
        uint32_t indexAtWriteFromU8 = 4;
        handleEcmaStrAllocTrueWriteTo->ToBaseString()->WriteData(indexAtWriteFromU8, u8Write);
        EXPECT_EQ(handleEcmaStrAllocTrueWriteTo->ToBaseString()->At(LoadTBarrier(), indexAtWriteFromU8), u8Write);
    }

    /*
     * @tc.name: WriteData_003
     * @tc.desc: Check whether the target EcmaString made by CreateLineString( , false, ) changed through calling
     * WriteData function with a source EcmaString made by CreateFromUtf16( , , , false) is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, WriteData_003)
    {
        /* WriteData(). From EcmaString made by CreateFromUtf16( , , , false) to EcmaStringU16 made by
         * CreateLineString( , false, ).
         */
        uint16_t arrayU16WriteFrom[10] = {67, 777, 1999, 1, 45, 66, 23456, 65535, 127, 333};
        uint32_t lengthEcmaStrU16WriteFrom = sizeof(arrayU16WriteFrom) / sizeof(arrayU16WriteFrom[0]);
        JSHandle<EcmaString> handleEcmaStrU16WriteFrom(thread,
                                                       EcmaStringAccessor::CreateFromUtf16(
                                                           instance, &arrayU16WriteFrom[0], lengthEcmaStrU16WriteFrom,
                                                           false));
        size_t sizeEcmaStrU16WriteTo = 10;
        JSHandle<EcmaString> handleEcmaStrU16WriteTo(thread,
                                                     EcmaStringAccessor::CreateLineString(
                                                         instance, sizeEcmaStrU16WriteTo, false));
        uint32_t indexStartWriteFromArrayU16 = 3;
        uint32_t numBytesWriteFromArrayU16 = 2 * 3;
        handleEcmaStrU16WriteTo->ToBaseString()->WriteData(LoadTBarrier(), handleEcmaStrU16WriteFrom->ToBaseString(),
                                                           indexStartWriteFromArrayU16, sizeEcmaStrU16WriteTo,
                                                           numBytesWriteFromArrayU16);
        for (uint32_t i = 0; i < (numBytesWriteFromArrayU16 / 2); i++) {
            EXPECT_EQ(handleEcmaStrU16WriteTo->ToBaseString()->At(LoadTBarrier(), i + indexStartWriteFromArrayU16),
                      handleEcmaStrU16WriteFrom->ToBaseString()->At(LoadTBarrier(), i));
        }
    }

    /*
     * @tc.name: WriteData_004
     * @tc.desc: Check whether the target EcmaString made by CreateLineString( , false, ) changed through calling
     * WriteData function with a source EcmaString made by CreateFromUtf8() is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, WriteData_004)
    {
        // WriteData(). From EcmaString made by CreateFromUtf8() to EcmaString made by CreateLineString( , false, ).
        uint8_t arrayU8WriteFrom[6] = {1, 12, 34, 56, 127};
        uint32_t lengthEcmaStrU8WriteFrom = sizeof(arrayU8WriteFrom) - 1;
        JSHandle<EcmaString> handleEcmaStrU8WriteFrom(thread,
                                                      EcmaStringAccessor::CreateFromUtf8(
                                                          instance, &arrayU8WriteFrom[0], lengthEcmaStrU8WriteFrom,
                                                          true));
        size_t sizeEcmaStrU16WriteTo = 10;
        JSHandle<EcmaString> handleEcmaStrU16WriteTo(thread,
                                                     EcmaStringAccessor::CreateLineString(
                                                         instance, sizeEcmaStrU16WriteTo, false));
        uint32_t indexStartWriteFromU8ToU16 = 1;
        uint32_t numBytesWriteFromU8ToU16 = 4;
        handleEcmaStrU16WriteTo->ToBaseString()->WriteData(LoadTBarrier(), handleEcmaStrU8WriteFrom->ToBaseString(),
                                                           indexStartWriteFromU8ToU16, sizeEcmaStrU16WriteTo,
                                                           numBytesWriteFromU8ToU16);
        for (uint32_t i = 0; i < numBytesWriteFromU8ToU16; i++) {
            EXPECT_EQ(handleEcmaStrU16WriteTo->ToBaseString()->At(LoadTBarrier(), i + indexStartWriteFromU8ToU16),
                      handleEcmaStrU8WriteFrom->ToBaseString()->At(LoadTBarrier(), i));
        }
    }

    /*
     * @tc.name: WriteData_005
     * @tc.desc: Check whether the target EcmaString made by CreateLineString( , false, ) changed through calling
     * WriteData function with a source char is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, WriteData_005)
    {
        // WriteData(). From char to EcmaString made by CreateLineString( , false, ).
        size_t sizeEcmaStrU16WriteTo = 10;
        JSHandle<EcmaString> handleEcmaStrU16WriteTo(thread,
                                                     EcmaStringAccessor::CreateLineString(
                                                         instance, sizeEcmaStrU16WriteTo, false));
        char u8Write = 'a';
        uint32_t indexAt = 4;
        handleEcmaStrU16WriteTo->ToBaseString()->WriteData(indexAt, u8Write);
        EXPECT_EQ(handleEcmaStrU16WriteTo->ToBaseString()->At(LoadTBarrier(), indexAt), u8Write);
    }

    /*
     * @tc.name: GetUtf8Length
     * @tc.desc: Check whether the value returned through calling GetUtf8Length function is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, GetUtf8Length)
    {
        uint8_t arrayU8[6] = {3, 7, 19, 54, 99};
        uint16_t arrayU16Comp[] = {1, 12, 34, 56, 127};
        uint16_t arrayU16NotComp[] = {19, 54, 256, 11100, 65535};
        uint32_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
        uint32_t lengthEcmaStrU16Comp = sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0]);
        uint32_t lengthEcmaStrU16NotComp = sizeof(arrayU16NotComp) / sizeof(arrayU16NotComp[0]);
        JSHandle<EcmaString> handleEcmaStrU8(thread,
                                             EcmaStringAccessor::CreateFromUtf8(
                                                 instance, &arrayU8[0], lengthEcmaStrU8, true));
        JSHandle<EcmaString> handleEcmaStrU16Comp(thread,
                                                  EcmaStringAccessor::CreateFromUtf16(
                                                      instance, &arrayU16Comp[0], lengthEcmaStrU16Comp, true));
        JSHandle<EcmaString> handleEcmaStrU16NotComp(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16NotComp[0], lengthEcmaStrU16NotComp,
                                                         false));
        EXPECT_EQ(handleEcmaStrU8->ToBaseString()->GetUtf8Length(LoadTBarrier()), lengthEcmaStrU8 + 1);
        EXPECT_EQ(handleEcmaStrU16Comp->ToBaseString()->GetUtf8Length(LoadTBarrier()), lengthEcmaStrU16Comp + 1);
        EXPECT_EQ(handleEcmaStrU16NotComp->ToBaseString()->GetUtf8Length(LoadTBarrier()),
                  2 * lengthEcmaStrU16NotComp + 1);
    }

    /*
     * @tc.name: GetDataUtf8
     * @tc.desc: Check whether the pointer returned through calling GetDataUtf8 function is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, GetDataUtf8)
    {
        // From EcmaString made by CreateFromUtf8().
        uint8_t arrayU8[] = {"abcde"};
        uint32_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
        JSHandle<EcmaString> handleEcmaStrU8(thread,
                                             EcmaStringAccessor::CreateFromUtf8(
                                                 instance, &arrayU8[0], lengthEcmaStrU8, true));
        for (uint32_t i = 0; i < lengthEcmaStrU8; i++) {
            EXPECT_EQ(*(handleEcmaStrU8->ToBaseString()->GetDataUtf8() + i), arrayU8[i]);
        }

        // From EcmaString made by CreateFromUtf16( , , , true).
        uint16_t arrayU16Comp[] = {3, 1, 34, 123, 127, 111, 42, 3, 20, 10};
        uint32_t lengthEcmaStrU16Comp = sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0]);
        JSHandle<EcmaString> handleEcmaStrU16Comp(thread,
                                                  EcmaStringAccessor::CreateFromUtf16(
                                                      instance, &arrayU16Comp[0], lengthEcmaStrU16Comp, true));
        for (uint32_t i = 0; i < sizeof(arrayU16Comp) / arrayU16Comp[0]; i++) {
            EXPECT_EQ(*(handleEcmaStrU16Comp->ToBaseString()->GetDataUtf8() + i), arrayU16Comp[i]);
        }
    }

    /*
     * @tc.name: GetDataUtf16
     * @tc.desc: Check whether the pointer returned through calling GetDataUtf16 function is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, GetDataUtf16)
    {
        // From EcmaString made by CreateFromUtf16( , , , false).
        uint16_t arrayU16NotComp[] = {67, 777, 1999, 1, 45, 66, 23456, 65535, 127, 333};
        uint32_t lengthEcmaStrU16NotComp = sizeof(arrayU16NotComp) / sizeof(arrayU16NotComp[0]);
        JSHandle<EcmaString> handleEcmaStrU16NotComp(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16NotComp[0], lengthEcmaStrU16NotComp,
                                                         false));
        for (uint32_t i = 0; i < lengthEcmaStrU16NotComp; i++) {
            EXPECT_EQ(*(handleEcmaStrU16NotComp->ToBaseString()->GetDataUtf16() + i), arrayU16NotComp[i]);
        }
    }

    /*
     * @tc.name: CopyDataRegionUtf8
     * @tc.desc: Check whether the returned value and the changed array through a source EcmaString's calling
     * CopyDataRegionUtf8 function are within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, CopyDataRegionUtf8)
    {
        // CopyDataRegionUtf8(). From EcmaString made by CreateFromUtf8().
        uint8_t arrayU8CopyFrom[6] = {1, 12, 34, 56, 127};
        uint32_t lengthEcmaStrU8CopyFrom = sizeof(arrayU8CopyFrom) - 1;
        JSHandle<EcmaString> handleEcmaStrU8CopyFrom(thread,
                                                     EcmaStringAccessor::CreateFromUtf8(
                                                         instance, &arrayU8CopyFrom[0], lengthEcmaStrU8CopyFrom, true));
        const size_t lengthArrayU8Target = 7;
        uint8_t defaultByteForU8CopyTo = 1;
        uint8_t arrayU8CopyTo[lengthArrayU8Target];
        int checkResUtf8 = memset_s(&arrayU8CopyTo[0], lengthArrayU8Target, defaultByteForU8CopyTo,
                                    lengthArrayU8Target);
        EXPECT_TRUE(checkResUtf8 == 0);

        size_t indexStartFromArrayU8 = 2;
        size_t lengthCopyToEcmaStrU8 = 3;
        size_t lengthReturnU8 = handleEcmaStrU8CopyFrom->ToBaseString()->CopyDataRegionUtf8(
            LoadTBarrier(), arrayU8CopyTo, indexStartFromArrayU8, lengthCopyToEcmaStrU8, lengthArrayU8Target);

        EXPECT_EQ(lengthReturnU8, lengthCopyToEcmaStrU8);
        for (uint32_t i = 0; i < lengthCopyToEcmaStrU8; i++) {
            EXPECT_EQ(arrayU8CopyTo[i],
                      handleEcmaStrU8CopyFrom->ToBaseString()->At(LoadTBarrier(), i + indexStartFromArrayU8));
        }
        for (uint32_t i = lengthCopyToEcmaStrU8; i < lengthArrayU8Target; i++) {
            EXPECT_EQ(arrayU8CopyTo[i], defaultByteForU8CopyTo);
        }

        // CopyDataRegionUtf8(). From EcmaString made by CreateFromUtf16( , , , true).
        uint16_t arrayU16CompCopyFrom[] = {1, 12, 34, 56, 127};
        uint32_t lengthEcmaStrU16CompCopyFrom = sizeof(arrayU16CompCopyFrom) / sizeof(arrayU16CompCopyFrom[0]);
        JSHandle<EcmaString> handleEcmaStrU16CompCopyFrom(thread,
                                                          EcmaStringAccessor::CreateFromUtf16(
                                                              instance, &arrayU16CompCopyFrom[0],
                                                              lengthEcmaStrU16CompCopyFrom, true));
        const size_t lengthArrayU16Target = 8;
        uint8_t defaultByteForU16CompCopyTo = 1;
        uint8_t arrayU16CompCopyTo[lengthArrayU16Target];
        int checkResUtf16 = memset_s(&arrayU16CompCopyTo[0], lengthArrayU16Target, defaultByteForU16CompCopyTo,
                                     lengthArrayU16Target);
        EXPECT_TRUE(checkResUtf16 == 0);

        size_t indexStartFromArrayU16Comp = 2;
        size_t lengthCopyToEcmaStrU16Comp = 3;
        size_t lengthReturnU16Comp = handleEcmaStrU16CompCopyFrom->ToBaseString()->CopyDataRegionUtf8(
            LoadTBarrier(), &arrayU16CompCopyTo[0], indexStartFromArrayU16Comp, lengthCopyToEcmaStrU16Comp,
            lengthArrayU16Target);

        EXPECT_EQ(lengthReturnU16Comp, lengthCopyToEcmaStrU16Comp);
        for (uint32_t i = 0; i < lengthReturnU16Comp; i++) {
            EXPECT_EQ(arrayU16CompCopyTo[i],
                      handleEcmaStrU16CompCopyFrom->ToBaseString()->At(LoadTBarrier(), i + indexStartFromArrayU16Comp));
        }
        for (uint32_t i = lengthReturnU16Comp; i < lengthArrayU16Target; i++) {
            EXPECT_EQ(arrayU16CompCopyTo[i], defaultByteForU16CompCopyTo);
        }
    }

    /*
     * @tc.name: CopyDataUtf16
     * @tc.desc: Check whether the returned value and the changed array through a source EcmaString's calling
     * CopyDataUtf16 function are within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, CopyDataUtf16)
    {
        // CopyDataUtf16(). From EcmaString made by CreateFromUtf16( , , , false).
        uint16_t arrayU16NotCompCopyFrom[10] = {67, 777, 1999, 1, 45, 66, 23456, 65535, 127, 333};
        uint32_t lengthEcmaStrU16NotCompCopyFrom = sizeof(arrayU16NotCompCopyFrom) / sizeof(arrayU16NotCompCopyFrom[0]);
        JSHandle<EcmaString> handleEcmaStrU16NotCompCopyFrom(thread,
                                                             EcmaStringAccessor::CreateFromUtf16(
                                                                 instance, &arrayU16NotCompCopyFrom[0],
                                                                 lengthEcmaStrU16NotCompCopyFrom, false));
        const size_t lengthArrayU16Target = 13;
        uint16_t arrayU16NotCompCopyTo[lengthArrayU16Target];
        uint8_t defaultOneByteValueOfArrayU16NotCompCopyTo = 244;
        int checkResUtf16 = memset_s(&arrayU16NotCompCopyTo[0], sizeof(uint16_t) * lengthArrayU16Target,
                                     defaultOneByteValueOfArrayU16NotCompCopyTo,
                                     sizeof(uint16_t) * lengthArrayU16Target);
        EXPECT_TRUE(checkResUtf16 == 0);

        size_t lengthReturnU16NotComp = handleEcmaStrU16NotCompCopyFrom->ToBaseString()->CopyDataUtf16(
            LoadTBarrier(), &arrayU16NotCompCopyTo[0], lengthArrayU16Target);

        EXPECT_EQ(lengthReturnU16NotComp, lengthEcmaStrU16NotCompCopyFrom);
        for (uint32_t i = 0; i < lengthReturnU16NotComp; i++) {
            EXPECT_EQ(arrayU16NotCompCopyTo[i], handleEcmaStrU16NotCompCopyFrom->ToBaseString()->At(LoadTBarrier(), i));
        }
        for (uint32_t i = lengthReturnU16NotComp; i < lengthArrayU16Target; i++) {
            EXPECT_EQ(arrayU16NotCompCopyTo[i],
                      ((uint16_t)defaultOneByteValueOfArrayU16NotCompCopyTo) * (1 + (1 << 8)));
        }
    }

    /*
     * @tc.name: StringsAreEqual_001
     * @tc.desc: Check whether the bool returned through calling StringsAreEqual function with two EcmaStrings made by
     * CreateFromUtf8() is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, StringsAreEqual_001)
    {
        // StringsAreEqual().
        uint8_t arrayU8No1[4] = {45, 92, 78};
        uint8_t arrayU8No2[4] = {45, 92, 78};
        uint8_t arrayU8No3[5] = {45, 92, 78, 1};
        uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
        uint32_t lengthEcmaStrU8No2 = sizeof(arrayU8No2) - 1;
        uint32_t lengthEcmaStrU8No3 = sizeof(arrayU8No3) - 1;
        JSHandle<EcmaString> handleEcmaStrU8No1(thread,
                                                EcmaStringAccessor::CreateFromUtf8(
                                                    instance, &arrayU8No1[0], lengthEcmaStrU8No1, true));
        JSHandle<EcmaString> handleEcmaStrU8No2(thread,
                                                EcmaStringAccessor::CreateFromUtf8(
                                                    instance, &arrayU8No2[0], lengthEcmaStrU8No2, true));
        JSHandle<EcmaString> handleEcmaStrU8No3(thread,
                                                EcmaStringAccessor::CreateFromUtf8(
                                                    instance, &arrayU8No3[0], lengthEcmaStrU8No3, true));
        EXPECT_TRUE(BaseString::StringsAreEqual(LoadTBarrier(), handleEcmaStrU8No1->ToBaseString(),
                                                handleEcmaStrU8No2->ToBaseString()));
        EXPECT_FALSE(BaseString::StringsAreEqual(LoadTBarrier(), handleEcmaStrU8No1->ToBaseString(),
                                                 handleEcmaStrU8No3->ToBaseString()));
        EXPECT_FALSE(BaseString::StringsAreEqual(LoadTBarrier(), handleEcmaStrU8No3->ToBaseString(),
                                                 handleEcmaStrU8No1->ToBaseString()));
    }

    /*
     * @tc.name: StringsAreEqual_002
     * @tc.desc: Check whether the bool returned through calling StringsAreEqual function with a EcmaString made by
     * CreateFromUtf8() and a EcmaString made by CreateFromUtf16(, , , true) is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, StringsAreEqual_002)
    {
        // StringsAreEqual().
        uint8_t arrayU8No1[4] = {45, 92, 78};
        uint16_t arrayU16CompNo2[] = {45, 92, 78};
        uint16_t arrayU16CompNo3[] = {45, 92, 78, 1};
        uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
        uint32_t lengthEcmaStrU16CompNo2 = sizeof(arrayU16CompNo2) / sizeof(arrayU16CompNo2[0]);
        uint32_t lengthEcmaStrU16CompNo3 = sizeof(arrayU16CompNo3) / sizeof(arrayU16CompNo3[0]);
        JSHandle<EcmaString> handleEcmaStrU8No1(thread,
                                                EcmaStringAccessor::CreateFromUtf8(
                                                    instance, &arrayU8No1[0], lengthEcmaStrU8No1, true));
        JSHandle<EcmaString> handleEcmaStrU16CompNo2(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16CompNo2[0], lengthEcmaStrU16CompNo2, true));
        JSHandle<EcmaString> handleEcmaStrU16CompNo3(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16CompNo3[0], lengthEcmaStrU16CompNo3, true));
        EXPECT_TRUE(BaseString::StringsAreEqual(LoadTBarrier(), handleEcmaStrU8No1->ToBaseString(),
                                                handleEcmaStrU16CompNo2->ToBaseString()));
        EXPECT_FALSE(BaseString::StringsAreEqual(LoadTBarrier(), handleEcmaStrU8No1->ToBaseString(),
                                                 handleEcmaStrU16CompNo3->ToBaseString()));
    }

    /*
     * @tc.name: StringsAreEqual_003
     * @tc.desc: Check whether the bool returned through calling StringsAreEqual function with two EcmaStrings made by
     * CreateFromUtf16(, , , true) is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, StringsAreEqual_003)
    {
        // StringsAreEqual().
        uint16_t arrayU16CompNo1[] = {45, 92, 78};
        uint16_t arrayU16CompNo2[] = {45, 92, 78};
        uint16_t arrayU16CompNo3[] = {45, 92, 78, 1};
        uint32_t lengthEcmaStrU16CompNo1 = sizeof(arrayU16CompNo1) / sizeof(arrayU16CompNo1[0]);
        uint32_t lengthEcmaStrU16CompNo2 = sizeof(arrayU16CompNo2) / sizeof(arrayU16CompNo2[0]);
        uint32_t lengthEcmaStrU16CompNo3 = sizeof(arrayU16CompNo3) / sizeof(arrayU16CompNo3[0]);
        JSHandle<EcmaString> handleEcmaStrU16CompNo1(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16CompNo1[0], lengthEcmaStrU16CompNo1, true));
        JSHandle<EcmaString> handleEcmaStrU16CompNo2(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16CompNo2[0], lengthEcmaStrU16CompNo2, true));
        JSHandle<EcmaString> handleEcmaStrU16CompNo3(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16CompNo3[0], lengthEcmaStrU16CompNo3, true));
        EXPECT_TRUE(BaseString::StringsAreEqual(LoadTBarrier(), handleEcmaStrU16CompNo1->ToBaseString(),
                                                handleEcmaStrU16CompNo2->ToBaseString()));
        EXPECT_FALSE(BaseString::StringsAreEqual(LoadTBarrier(), handleEcmaStrU16CompNo1->ToBaseString(),
                                                 handleEcmaStrU16CompNo3->ToBaseString()));
        EXPECT_FALSE(BaseString::StringsAreEqual(LoadTBarrier(), handleEcmaStrU16CompNo3->ToBaseString(),
                                                 handleEcmaStrU16CompNo1->ToBaseString()));
    }

    /*
     * @tc.name: StringsAreEqual_004
     * @tc.desc: Check whether the bool returned through calling StringsAreEqual function with a EcmaString made by
     * CreateFromUtf8() and a EcmaString made by CreateFromUtf16(, , , false) is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, StringsAreEqual_004)
    {
        // StringsAreEqual().
        uint8_t arrayU8No1[4] = {45, 92, 78};
        uint16_t arrayU16NotCompNo1[] = {45, 92, 78};
        uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
        uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
        JSHandle<EcmaString> handleEcmaStrU8No1(thread,
                                                EcmaStringAccessor::CreateFromUtf8(
                                                    instance, &arrayU8No1[0], lengthEcmaStrU8No1, true));
        JSHandle<EcmaString> handleEcmaStrU16NotCompNo1(thread,
                                                        EcmaStringAccessor::CreateFromUtf16(
                                                            instance, &arrayU16NotCompNo1[0],
                                                            lengthEcmaStrU16NotCompNo1, true));
        EXPECT_TRUE(BaseString::StringsAreEqual(LoadTBarrier(), handleEcmaStrU8No1->ToBaseString(),
                                                handleEcmaStrU16NotCompNo1->ToBaseString()));
        EXPECT_TRUE(BaseString::StringsAreEqual(LoadTBarrier(), handleEcmaStrU16NotCompNo1->ToBaseString(),
                                                handleEcmaStrU8No1->ToBaseString()));
    }

    /*
     * @tc.name: StringsAreEqual_005
     * @tc.desc: Check whether the bool returned through calling StringsAreEqual function with a EcmaString made by
     * CreateFromUtf16(, , , true) and a EcmaString made by CreateFromUtf16(, , , false) is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, StringsAreEqual_005)
    {
        // StringsAreEqual().
        uint16_t arrayU16CompNo1[] = {45, 92, 78};
        uint16_t arrayU16NotCompNo1[] = {45, 92, 78};
        uint32_t lengthEcmaStrU16CompNo1 = sizeof(arrayU16CompNo1) / sizeof(arrayU16CompNo1[0]);
        uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
        JSHandle<EcmaString> handleEcmaStrU16CompNo1(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16CompNo1[0], lengthEcmaStrU16CompNo1, true));
        JSHandle<EcmaString> handleEcmaStrU16NotCompNo1(thread,
                                                        EcmaStringAccessor::CreateFromUtf16(
                                                            instance, &arrayU16NotCompNo1[0],
                                                            lengthEcmaStrU16NotCompNo1, true));
        EXPECT_TRUE(BaseString::StringsAreEqual(LoadTBarrier(), handleEcmaStrU16CompNo1->ToBaseString(),
                                                handleEcmaStrU16NotCompNo1->ToBaseString()));
        EXPECT_TRUE(BaseString::StringsAreEqual(LoadTBarrier(), handleEcmaStrU16NotCompNo1->ToBaseString(),
                                                handleEcmaStrU16CompNo1->ToBaseString()));
    }

    /*
     * @tc.name: StringsAreEqual_006
     * @tc.desc: Check whether the bool returned through calling StringsAreEqual function with two EcmaStrings made by
     * CreateFromUtf16(, , , false) is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, StringsAreEqual_006)
    {
        // StringsAreEqual().
        uint16_t arrayU16NotCompNo1[] = {234, 345, 127, 2345, 65535, 5};
        uint16_t arrayU16NotCompNo2[] = {234, 345, 127, 2345, 65535, 5};
        uint16_t arrayU16NotCompNo3[] = {1, 234, 345, 127, 2345, 65535, 5};
        uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
        uint32_t lengthEcmaStrU16NotCompNo2 = sizeof(arrayU16NotCompNo2) / sizeof(arrayU16NotCompNo2[0]);
        uint32_t lengthEcmaStrU16NotCompNo3 = sizeof(arrayU16NotCompNo3) / sizeof(arrayU16NotCompNo3[0]);
        JSHandle<EcmaString> handleEcmaStrU16NotCompNo1(thread,
                                                        EcmaStringAccessor::CreateFromUtf16(
                                                            instance, &arrayU16NotCompNo1[0],
                                                            lengthEcmaStrU16NotCompNo1, false));
        JSHandle<EcmaString> handleEcmaStrU16NotCompNo2(thread,
                                                        EcmaStringAccessor::CreateFromUtf16(
                                                            instance, &arrayU16NotCompNo2[0],
                                                            lengthEcmaStrU16NotCompNo2, false));
        JSHandle<EcmaString> handleEcmaStrU16NotCompNo3(thread,
                                                        EcmaStringAccessor::CreateFromUtf16(
                                                            instance, &arrayU16NotCompNo3[0],
                                                            lengthEcmaStrU16NotCompNo3, false));
        EXPECT_TRUE(BaseString::StringsAreEqual(LoadTBarrier(), handleEcmaStrU16NotCompNo1->ToBaseString(),
                                                handleEcmaStrU16NotCompNo2->ToBaseString()));
        EXPECT_FALSE(BaseString::StringsAreEqual(LoadTBarrier(), handleEcmaStrU16NotCompNo1->ToBaseString(),
                                                 handleEcmaStrU16NotCompNo3->ToBaseString()));
        EXPECT_FALSE(BaseString::StringsAreEqual(LoadTBarrier(), handleEcmaStrU16NotCompNo3->ToBaseString(),
                                                 handleEcmaStrU16NotCompNo1->ToBaseString()));
    }

    /*
     * @tc.name: StringsAreEqualUtf8_001
     * @tc.desc: Check whether the bool returned through calling StringIsEqualUint8Data function with an EcmaString made
     * by CreateFromUtf8() and an Array(uint8_t) is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, StringsAreEqualUtf8_001)
    {
        // StringIsEqualUint8Data(). EcmaString made by CreateFromUtf8(), Array:U8.
        uint8_t arrayU8No1[4] = {45, 92, 78};
        uint8_t arrayU8No2[5] = {45, 92, 78, 24};
        uint8_t arrayU8No3[3] = {45, 92};
        uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
        uint32_t lengthEcmaStrU8No2 = sizeof(arrayU8No2) - 1;
        uint32_t lengthEcmaStrU8No3 = sizeof(arrayU8No3) - 1;
        JSHandle<EcmaString> handleEcmaStrU8No1(thread,
                                                EcmaStringAccessor::CreateFromUtf8(
                                                    instance, &arrayU8No1[0], lengthEcmaStrU8No1, true));
        JSHandle<EcmaString> handleEcmaStrU8No2(thread,
                                                EcmaStringAccessor::CreateFromUtf8(
                                                    instance, &arrayU8No2[0], lengthEcmaStrU8No2, true));
        JSHandle<EcmaString> handleEcmaStrU8No3(thread,
                                                EcmaStringAccessor::CreateFromUtf8(
                                                    instance, &arrayU8No3[0], lengthEcmaStrU8No3, true));
        EXPECT_TRUE(BaseString::StringIsEqualUint8Data(LoadTBarrier(), handleEcmaStrU8No1->ToBaseString(),
                                                       &arrayU8No1[0], lengthEcmaStrU8No1, true));
        EXPECT_FALSE(BaseString::StringIsEqualUint8Data(LoadTBarrier(), handleEcmaStrU8No1->ToBaseString(),
                                                        &arrayU8No1[0], lengthEcmaStrU8No1, false));
        EXPECT_FALSE(BaseString::StringIsEqualUint8Data(LoadTBarrier(), handleEcmaStrU8No2->ToBaseString(),
                                                        &arrayU8No1[0], lengthEcmaStrU8No1, true));
        EXPECT_FALSE(BaseString::StringIsEqualUint8Data(LoadTBarrier(), handleEcmaStrU8No3->ToBaseString(),
                                                        &arrayU8No1[0], lengthEcmaStrU8No1, true));
    }

    /*
     * @tc.name: StringsAreEqualUtf8_002
     * @tc.desc: Check whether the bool returned through calling StringIsEqualUint8Data function with an EcmaString made
     * by CreateFromUtf16( , , , true) and an Array(uint8_t) is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, StringsAreEqualUtf8_002)
    {
        // StringIsEqualUint8Data(). EcmaString made by CreateFromUtf16( , , , true), Array:U8.
        uint8_t arrayU8No1[4] = {45, 92, 78};
        uint16_t arrayU16CompNo1[] = {45, 92, 78};
        uint16_t arrayU16CompNo2[] = {45, 92, 78, 24};
        uint16_t arrayU16CompNo3[] = {45, 92};
        uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
        uint32_t lengthEcmaStrU16CompNo1 = sizeof(arrayU16CompNo1) / sizeof(arrayU16CompNo1[0]);
        uint32_t lengthEcmaStrU16CompNo2 = sizeof(arrayU16CompNo2) / sizeof(arrayU16CompNo2[0]);
        uint32_t lengthEcmaStrU16CompNo3 = sizeof(arrayU16CompNo3) / sizeof(arrayU16CompNo3[0]);
        JSHandle<EcmaString> handleEcmaStrU16CompNo1(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16CompNo1[0], lengthEcmaStrU16CompNo1, true));
        JSHandle<EcmaString> handleEcmaStrU16CompNo2(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16CompNo2[0], lengthEcmaStrU16CompNo2, true));
        JSHandle<EcmaString> handleEcmaStrU16CompNo3(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16CompNo3[0], lengthEcmaStrU16CompNo3, true));
        EXPECT_TRUE(BaseString::StringIsEqualUint8Data(LoadTBarrier(), handleEcmaStrU16CompNo1->ToBaseString(),
                                                       &arrayU8No1[0], lengthEcmaStrU8No1, true));
        EXPECT_FALSE(BaseString::StringIsEqualUint8Data(LoadTBarrier(), handleEcmaStrU16CompNo1->ToBaseString(),
                                                        &arrayU8No1[0], lengthEcmaStrU8No1, false));
        EXPECT_FALSE(BaseString::StringIsEqualUint8Data(LoadTBarrier(), handleEcmaStrU16CompNo2->ToBaseString(),
                                                        &arrayU8No1[0], lengthEcmaStrU8No1, true));
        EXPECT_FALSE(BaseString::StringIsEqualUint8Data(LoadTBarrier(), handleEcmaStrU16CompNo3->ToBaseString(),
                                                        &arrayU8No1[0], lengthEcmaStrU8No1, true));
    }

    /*
     * @tc.name: StringsAreEqualUtf8_003
     * @tc.desc: Check whether the bool returned through calling StringIsEqualUint8Data function with an EcmaString made
     * by CreateFromUtf16( , , , false) and an Array(uint8_t) is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, StringsAreEqualUtf8_003)
    {
        // StringIsEqualUint8Data(). EcmaString made by CreateFromUtf16( , , , false), Array:U8.
        EcmaTestCommon::StringIsEqualCommonCase(thread, instance, EcmaStringAccessor::StringIsEqualUint8Data);
    }

    /*
     * @tc.name: StringsAreEqualUtf16_001
     * @tc.desc: Check whether the bool returned through calling StringsAreEqualUtf16 function with an EcmaString made
     * by CreateFromUtf8() and an Array(uint16_t) is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, StringsAreEqualUtf16_001)
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
                                                EcmaStringAccessor::CreateFromUtf8(
                                                    instance, &arrayU8No1[0], lengthEcmaStrU8No1, true));
        JSHandle<EcmaString> handleEcmaStrU8No2(thread,
                                                EcmaStringAccessor::CreateFromUtf8(
                                                    instance, &arrayU8No2[0], lengthEcmaStrU8No2, true));
        JSHandle<EcmaString> handleEcmaStrU8No3(thread,
                                                EcmaStringAccessor::CreateFromUtf8(
                                                    instance, &arrayU8No3[0], lengthEcmaStrU8No3, true));
        EXPECT_TRUE(BaseString::StringsAreEqualUtf16(LoadTBarrier(), handleEcmaStrU8No1->ToBaseString(),
                                                     &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1));
        EXPECT_FALSE(BaseString::StringsAreEqualUtf16(LoadTBarrier(), handleEcmaStrU8No2->ToBaseString(),
                                                      &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1));
        EXPECT_FALSE(BaseString::StringsAreEqualUtf16(LoadTBarrier(), handleEcmaStrU8No3->ToBaseString(),
                                                      &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1));
    }

    /*
     * @tc.name: StringsAreEqualUtf16_002
     * @tc.desc: Check whether the bool returned through calling StringsAreEqualUtf16 function with an EcmaString made
     * by CreateFromUtf16( , , , true) and an Array(uint16_t) is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, StringsAreEqualUtf16_002)
    {
        // StringsAreEqualUtf16(). EcmaString made by CreateFromUtf16( , , , true), Array:U16(1-127).
        uint16_t arrayU16CompNo1[] = {45, 92, 78};
        uint16_t arrayU16CompNo2[] = {45, 92, 78, 24};
        uint16_t arrayU16CompNo3[] = {45, 92};
        uint16_t arrayU16CompNo4[] = {25645, 25692, 25678}; // 25645 % 256 == 45...
        uint32_t lengthEcmaStrU16CompNo1 = sizeof(arrayU16CompNo1) / sizeof(arrayU16CompNo1[0]);
        uint32_t lengthEcmaStrU16CompNo2 = sizeof(arrayU16CompNo2) / sizeof(arrayU16CompNo2[0]);
        uint32_t lengthEcmaStrU16CompNo3 = sizeof(arrayU16CompNo3) / sizeof(arrayU16CompNo3[0]);
        uint32_t lengthEcmaStrU16CompNo4 = sizeof(arrayU16CompNo4) / sizeof(arrayU16CompNo4[0]);
        JSHandle<EcmaString> handleEcmaStrU16CompNo1(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16CompNo1[0], lengthEcmaStrU16CompNo1, true));
        JSHandle<EcmaString> handleEcmaStrU16CompNo2(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16CompNo2[0], lengthEcmaStrU16CompNo2, true));
        JSHandle<EcmaString> handleEcmaStrU16CompNo3(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16CompNo3[0], lengthEcmaStrU16CompNo3, true));
        JSHandle<EcmaString> handleEcmaStrU16CompNo4(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16CompNo4[0], lengthEcmaStrU16CompNo4, true));
        EXPECT_TRUE(BaseString::StringsAreEqualUtf16(LoadTBarrier(), handleEcmaStrU16CompNo1->ToBaseString(),
                                                     &arrayU16CompNo1[0], lengthEcmaStrU16CompNo1));
        EXPECT_FALSE(BaseString::StringsAreEqualUtf16(LoadTBarrier(), handleEcmaStrU16CompNo2->ToBaseString(),
                                                      &arrayU16CompNo1[0], lengthEcmaStrU16CompNo1));
        EXPECT_FALSE(BaseString::StringsAreEqualUtf16(LoadTBarrier(), handleEcmaStrU16CompNo3->ToBaseString(),
                                                      &arrayU16CompNo1[0], lengthEcmaStrU16CompNo1));
        EXPECT_TRUE(BaseString::StringsAreEqualUtf16(LoadTBarrier(), handleEcmaStrU16CompNo4->ToBaseString(),
                                                     &arrayU16CompNo1[0], lengthEcmaStrU16CompNo1));
    }

    /*
     * @tc.name: StringsAreEqualUtf16_003
     * @tc.desc: Check whether the bool returned through calling StringsAreEqualUtf16 function with an EcmaString made
     * by CreateFromUtf16( , , , false) and an Array(uint16_t) is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, StringsAreEqualUtf16_003)
    {
        uint16_t arrayU16NotCompNo1[] = {25645, 25692, 25678};
        uint16_t arrayU16NotCompNo2[] = {25645, 25692, 78}; // 25645 % 256 == 45...
        uint16_t arrayU16NotCompNo3[] = {25645, 25692, 25678, 65535};
        uint16_t arrayU16NotCompNo4[] = {25645, 25692};
        uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
        uint32_t lengthEcmaStrU16NotCompNo2 = sizeof(arrayU16NotCompNo2) / sizeof(arrayU16NotCompNo2[0]);
        uint32_t lengthEcmaStrU16NotCompNo3 = sizeof(arrayU16NotCompNo3) / sizeof(arrayU16NotCompNo3[0]);
        uint32_t lengthEcmaStrU16NotCompNo4 = sizeof(arrayU16NotCompNo4) / sizeof(arrayU16NotCompNo4[0]);
        JSHandle<EcmaString> handleEcmaStrU16NotCompNo1(thread,
                                                        EcmaStringAccessor::CreateFromUtf16(
                                                            instance, &arrayU16NotCompNo1[0],
                                                            lengthEcmaStrU16NotCompNo1, false));
        JSHandle<EcmaString> handleEcmaStrU16NotCompNo2(thread,
                                                        EcmaStringAccessor::CreateFromUtf16(
                                                            instance, &arrayU16NotCompNo2[0],
                                                            lengthEcmaStrU16NotCompNo2, false));
        JSHandle<EcmaString> handleEcmaStrU16NotCompNo3(thread,
                                                        EcmaStringAccessor::CreateFromUtf16(
                                                            instance, &arrayU16NotCompNo3[0],
                                                            lengthEcmaStrU16NotCompNo3, false));
        JSHandle<EcmaString> handleEcmaStrU16NotCompNo4(thread,
                                                        EcmaStringAccessor::CreateFromUtf16(
                                                            instance, &arrayU16NotCompNo4[0],
                                                            lengthEcmaStrU16NotCompNo4, false));
        EXPECT_TRUE(BaseString::StringsAreEqualUtf16(LoadTBarrier(), handleEcmaStrU16NotCompNo1->ToBaseString(),
                                                     &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1));
        EXPECT_FALSE(BaseString::StringsAreEqualUtf16(LoadTBarrier(), handleEcmaStrU16NotCompNo1->ToBaseString(),
                                                      &arrayU16NotCompNo2[0], lengthEcmaStrU16NotCompNo2));
        EXPECT_FALSE(BaseString::StringsAreEqualUtf16(LoadTBarrier(), handleEcmaStrU16NotCompNo2->ToBaseString(),
                                                      &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1));
        EXPECT_FALSE(BaseString::StringsAreEqualUtf16(LoadTBarrier(), handleEcmaStrU16NotCompNo3->ToBaseString(),
                                                      &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1));
        EXPECT_FALSE(BaseString::StringsAreEqualUtf16(LoadTBarrier(), handleEcmaStrU16NotCompNo4->ToBaseString(),
                                                      &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1));
    }

    /*
     * @tc.name: ComputeHashcodeUtf8
     * @tc.desc: Check whether the value returned through calling ComputeHashcodeUtf8 function with an Array(uint8_t) is
     * within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, ComputeHashcodeUtf8)
    {
        uint8_t arrayU8[] = {"abc"};
        uint32_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
        uint32_t hashExpect = 0;
        for (uint32_t i = 0; i < lengthEcmaStrU8; i++) {
            hashExpect = hashExpect * 31 + arrayU8[i];
        }
        EXPECT_EQ(BaseString::ComputeHashcodeUtf8(&arrayU8[0], lengthEcmaStrU8, true), hashExpect);
    }

    /*
     * @tc.name: ComputeHashcodeUtf16
     * @tc.desc: Check whether the value returned through calling ComputeHashcodeUtf16 function with an Array(uint16_t)
     * is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, ComputeHashcodeUtf16)
    {
        uint16_t arrayU16[] = {199, 1, 256, 65535, 777};
        uint32_t lengthEcmaStrU16 = sizeof(arrayU16) / sizeof(arrayU16[0]);
        uint32_t hashExpect = 0;
        for (uint32_t i = 0; i < lengthEcmaStrU16; i++) {
            hashExpect = hashExpect * 31 + arrayU16[i];
        }
        EXPECT_EQ(BaseString::ComputeHashcodeUtf16(&arrayU16[0], lengthEcmaStrU16), hashExpect);
    }

    /*
     * @tc.name: GetHashcode_001
     * @tc.desc: Check whether the value returned through an EcmaString made by CreateFromUtf8() calling GetHashcode
     * function is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, GetHashcode_001)
    {
        // GetHashcode(). EcmaString made by CreateFromUtf8().
        uint8_t arrayU8[] = {"abc"};
        uint32_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
        JSHandle<EcmaString> handleEcmaStrU8(thread,
                                             EcmaStringAccessor::CreateFromUtf8(
                                                 instance, &arrayU8[0], lengthEcmaStrU8, true));
        uint32_t hashExpect = 0;
        for (uint32_t i = 0; i < lengthEcmaStrU8; i++) {
            hashExpect = hashExpect * 31 + arrayU8[i];
        }
        EXPECT_EQ(handleEcmaStrU8->ToBaseString()->GetHashcode(LoadTBarrier()), hashExpect);
    }

    /*
     * @tc.name: GetHashcode_002
     * @tc.desc: Check whether the value returned through an EcmaString made by CreateFromUtf16( , , , true) calling
     * GetHashcode function is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, GetHashcode_002)
    {
        // GetHashcode(). EcmaString made by CreateFromUtf16( , , , true).
        uint16_t arrayU16Comp[] = {45, 92, 78, 24};
        uint32_t lengthEcmaStrU16Comp = sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0]);
        JSHandle<EcmaString> handleEcmaStrU16Comp(thread,
                                                  EcmaStringAccessor::CreateFromUtf16(
                                                      instance, &arrayU16Comp[0], lengthEcmaStrU16Comp, true));
        uint32_t hashExpect = 0;
        for (uint32_t i = 0; i < lengthEcmaStrU16Comp; i++) {
            hashExpect = hashExpect * 31 + arrayU16Comp[i];
        }
        EXPECT_EQ(handleEcmaStrU16Comp->ToBaseString()->GetHashcode(LoadTBarrier()), hashExpect);
    }

    /*
     * @tc.name: GetHashcode_003
     * @tc.desc: Check whether the value returned through an EcmaString made by CreateFromUtf16( , , , false) calling
     * GetHashcode function is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, GetHashcode_003)
    {
        // GetHashcode(). EcmaString made by CreateFromUtf16( , , , false).
        uint16_t arrayU16NotComp[] = {199, 1, 256, 65535, 777};
        uint32_t lengthEcmaStrU16NotComp = sizeof(arrayU16NotComp) / sizeof(arrayU16NotComp[0]);
        JSHandle<EcmaString> handleEcmaStrU16NotComp(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16NotComp[0], lengthEcmaStrU16NotComp,
                                                         false));
        uint32_t hashExpect = 0;
        for (uint32_t i = 0; i < lengthEcmaStrU16NotComp; i++) {
            hashExpect = hashExpect * 31 + arrayU16NotComp[i];
        }
        EXPECT_EQ(handleEcmaStrU16NotComp->ToBaseString()->GetHashcode(LoadTBarrier()), hashExpect);
    }

    /*
     * @tc.name: GetHashcode_004
     * @tc.desc: Check whether the value returned through an EcmaString made by CreateEmptyString() calling GetHashcode
     * function is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, GetHashcode_004)
    {
        // GetHashcode(). EcmaString made by CreateEmptyString().
        JSHandle<EcmaString> handleEcmaStrEmpty(thread, EcmaStringAccessor::CreateEmptyString(instance));
        EXPECT_EQ(handleEcmaStrEmpty->ToBaseString()->GetHashcode(LoadTBarrier()), 0U);
    }

    /*
     * @tc.name: GetHashcode_005
     * @tc.desc: Check whether the value returned through an EcmaString made by CreateLineString(, true/false, ) calling
     * GetHashcode function is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, GetHashcode_005)
    {
        // GetHashcode(). EcmaString made by CreateLineString().
        size_t sizeAlloc = 5;
        JSHandle<EcmaString> handleEcmaStrAllocComp(
            thread, EcmaStringAccessor::CreateLineString(instance, sizeAlloc, true));
        JSHandle<EcmaString> handleEcmaStrAllocNotComp(
            thread, EcmaStringAccessor::CreateLineString(instance, sizeAlloc, false));
        EXPECT_EQ(handleEcmaStrAllocComp->ToBaseString()->GetMixHashcode(), 0U);
        EXPECT_EQ(handleEcmaStrAllocNotComp->ToBaseString()->GetMixHashcode(), 0U);
    }

    /*
     * @tc.name: SetIsInternString
     * @tc.desc: Call SetIsInternString function, check whether the bool returned through calling IsInternString
     * function is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, SetIsInternString)
    {
        uint8_t arrayU8[] = {"abc"};
        uint32_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
        JSHandle<EcmaString> handleEcmaStrU8(thread,
                                             EcmaStringAccessor::CreateFromUtf8(
                                                 instance, &arrayU8[0], lengthEcmaStrU8, true));
        EXPECT_FALSE(handleEcmaStrU8->ToBaseString()->IsInternString());
        handleEcmaStrU8->ToBaseString()->SetIsInternString();
        EXPECT_TRUE(handleEcmaStrU8->ToBaseString()->IsInternString());

        uint16_t arrayU16Comp[] = {97, 98, 99};
        uint32_t lengthEcmaStrU16Comp = sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0]);
        JSHandle<EcmaString> handleEcmaStrU16Comp(thread,
                                                  EcmaStringAccessor::CreateFromUtf16(
                                                      instance, &arrayU16Comp[0], lengthEcmaStrU16Comp, true));
        EXPECT_FALSE(handleEcmaStrU16Comp->ToBaseString()->IsInternString());
        handleEcmaStrU16Comp->ToBaseString()->SetIsInternString();
        EXPECT_TRUE(handleEcmaStrU16Comp->ToBaseString()->IsInternString());

        uint16_t arrayU16NotComp[] = {97, 98, 99};
        uint32_t lengthEcmaStrU16NotComp = sizeof(arrayU16NotComp) / sizeof(arrayU16NotComp[0]);
        JSHandle<EcmaString> handleEcmaStrU16NotComp(thread,
                                                     EcmaStringAccessor::CreateFromUtf16(
                                                         instance, &arrayU16NotComp[0], lengthEcmaStrU16NotComp, true));
        EXPECT_FALSE(handleEcmaStrU16NotComp->ToBaseString()->IsInternString());
        handleEcmaStrU16NotComp->ToBaseString()->SetIsInternString();
        EXPECT_TRUE(handleEcmaStrU16NotComp->ToBaseString()->IsInternString());
    }

    /*
     * @tc.name: EqualToSplicedString
     * @tc.desc: Tests whether the source string is equal to the concatenated string.
     * is within expectations.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, EqualToSplicedString)
    {
        ObjectFactory* factory = instance->GetFactory();
        {
            JSHandle<EcmaString> sourceString = factory->NewFromUtf8("Start开始");
            JSHandle<EcmaString> firstString = factory->NewFromASCII("Start");
            JSHandle<EcmaString> secondString = factory->NewFromUtf8("开始");
            EXPECT_TRUE(sourceString->ToBaseString()->IsUtf16());
            EXPECT_TRUE(firstString->ToBaseString()->IsUtf8());
            EXPECT_TRUE(secondString->ToBaseString()->IsUtf16());
            bool result = sourceString->ToBaseString()->EqualToSplicedString(
                LoadTBarrier(), firstString->ToBaseString(), secondString->ToBaseString());
            EXPECT_TRUE(result);
        }

        {
            JSHandle<EcmaString> sourceString = factory->NewFromUtf8("Start开始");
            JSHandle<EcmaString> firstString = factory->NewFromASCII("Start");
            JSHandle<EcmaString> secondString = factory->NewFromASCII("start");
            EXPECT_TRUE(sourceString->ToBaseString()->IsUtf16());
            EXPECT_TRUE(firstString->ToBaseString()->IsUtf8());
            EXPECT_TRUE(secondString->ToBaseString()->IsUtf8());
            bool result = sourceString->ToBaseString()->EqualToSplicedString(
                LoadTBarrier(), firstString->ToBaseString(), secondString->ToBaseString());
            EXPECT_TRUE(!result);
        }

        {
            JSHandle<EcmaString> sourceString = factory->NewFromUtf8("Start开始");
            JSHandle<EcmaString> firstString = factory->NewFromUtf8("Start开");
            JSHandle<EcmaString> secondString = factory->NewFromUtf8("始");
            EXPECT_TRUE(sourceString->ToBaseString()->IsUtf16());
            EXPECT_TRUE(firstString->ToBaseString()->IsUtf16());
            EXPECT_TRUE(secondString->ToBaseString()->IsUtf16());
            bool result = sourceString->ToBaseString()->EqualToSplicedString(
                LoadTBarrier(), firstString->ToBaseString(), secondString->ToBaseString());
            EXPECT_TRUE(result);
        }

        {
            JSHandle<EcmaString> sourceString = factory->NewFromUtf8("Startstart");
            JSHandle<EcmaString> firstString = factory->NewFromASCII("Start");
            JSHandle<EcmaString> secondString = factory->NewFromASCII("start");
            EXPECT_TRUE(sourceString->ToBaseString()->IsUtf8());
            EXPECT_TRUE(firstString->ToBaseString()->IsUtf8());
            EXPECT_TRUE(secondString->ToBaseString()->IsUtf8());
            bool result = sourceString->ToBaseString()->EqualToSplicedString(
                LoadTBarrier(), firstString->ToBaseString(), secondString->ToBaseString());
            EXPECT_TRUE(result);
        }

        {
            JSHandle<EcmaString> sourceString = factory->NewFromUtf8("Startstart");
            JSHandle<EcmaString> firstString = factory->NewFromASCII("Start");
            JSHandle<EcmaString> secondString = factory->NewFromUtf8("开始");
            EXPECT_TRUE(sourceString->ToBaseString()->IsUtf8());
            EXPECT_TRUE(firstString->ToBaseString()->IsUtf8());
            EXPECT_TRUE(secondString->ToBaseString()->IsUtf16());
            bool result = sourceString->ToBaseString()->EqualToSplicedString(
                LoadTBarrier(), firstString->ToBaseString(), secondString->ToBaseString());
            EXPECT_TRUE(!result);
        }

        {
            JSHandle<EcmaString> sourceString = factory->NewFromUtf8("Startstat");
            JSHandle<EcmaString> firstString = factory->NewFromASCII("Start");
            JSHandle<EcmaString> secondString = factory->NewFromASCII("start");
            EXPECT_TRUE(sourceString->ToBaseString()->IsUtf8());
            EXPECT_TRUE(firstString->ToBaseString()->IsUtf8());
            EXPECT_TRUE(secondString->ToBaseString()->IsUtf8());
            bool result = sourceString->ToBaseString()->EqualToSplicedString(
                LoadTBarrier(), firstString->ToBaseString(), secondString->ToBaseString());
            EXPECT_TRUE(!result);
        }

        {
            JSHandle<EcmaString> sourceString = factory->NewFromUtf8("Start开始");
            JSHandle<EcmaString> firstString = factory->NewFromUtf8("Stat开");
            JSHandle<EcmaString> secondString = factory->NewFromUtf8("始");
            EXPECT_TRUE(sourceString->ToBaseString()->IsUtf16());
            EXPECT_TRUE(firstString->ToBaseString()->IsUtf16());
            EXPECT_TRUE(secondString->ToBaseString()->IsUtf16());
            bool result = sourceString->ToBaseString()->EqualToSplicedString(
                LoadTBarrier(), firstString->ToBaseString(), secondString->ToBaseString());
            EXPECT_TRUE(!result);
        }

        {
            JSHandle<EcmaString> sourceString = factory->NewFromUtf8("Start开始");
            JSHandle<EcmaString> firstString = factory->NewFromASCII("Stat");
            JSHandle<EcmaString> secondString = factory->NewFromUtf8("开始");
            EXPECT_TRUE(sourceString->ToBaseString()->IsUtf16());
            EXPECT_TRUE(firstString->ToBaseString()->IsUtf8());
            EXPECT_TRUE(secondString->ToBaseString()->IsUtf16());
            bool result = sourceString->ToBaseString()->EqualToSplicedString(
                LoadTBarrier(), firstString->ToBaseString(), secondString->ToBaseString());
            EXPECT_TRUE(!result);
        }
    }

    HWTEST_F_L0(BaseStringTest, ConvertToString001)
    {
        ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<EcmaString> testString = factory->NewFromASCII("bar2bazJavaScriptbaz");
        CString str = ConvertToString(thread, *testString, StringConvertedUsage::LOGICOPERATION, false);
        EXPECT_EQ(str, CString("bar2bazJavaScriptbaz"));
    }

    HWTEST_F_L0(BaseStringTest, ConvertToString002)
    {
        CString str = ConvertToString(thread, nullptr, StringConvertedUsage::LOGICOPERATION, false);
        EXPECT_EQ(str, CString(""));
    }

    HWTEST_F_L0(BaseStringTest, ConvertToString003)
    {
        ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<EcmaString> testString = factory->NewFromASCII("test");
        CString str = ConvertToString(thread, *testString);
        EXPECT_EQ(str, CString("test"));
    }

    /*
     * @tc.name: ComputeHashcodeUtf8Nullptr
     * @tc.desc: Check whether the value returned through calling ComputeHashcodeUtf8 function with an Array(uint8_t) is
     * within expectations for nullptr.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, ComputeHashcodeUtf8Nullptr)
    {
        EXPECT_EQ(BaseString::ComputeHashcodeUtf8(nullptr, 0, true), 0);
    }

    /*
     * @tc.name: ComputeHashcodeUtf16Nullptr
     * @tc.desc: Check whether the value returned through calling ComputeHashcodeUtf16 function with an Array(uint16_t)
     * is within expectations for nullptr.
     * @tc.type: FUNC
     * @tc.require:
     */
    HWTEST_F_L0(BaseStringTest, ComputeHashcodeUtf16Nullptr)
    {
        EXPECT_EQ(BaseString::ComputeHashcodeUtf16(nullptr, 0), 0);
    }
} // namespace panda::test
