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

#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda::ecmascript;

namespace panda::test {
class EcmaExternalStringTest : public BaseTestWithScope<false> {
  public:
    static constexpr uint32_t TYPE_INFO_SIZE = 4;
    static constexpr uint32_t UTF8_CACHED_DATA_SIZE = 4;
    static constexpr uint32_t UTF16_CACHED_DATA_SIZE = 8;
    static constexpr uint32_t UTF16_STRING_LENGTH = UTF16_CACHED_DATA_SIZE / sizeof(uint16_t);

    static inline void CallBackFn([[maybe_unused]] void *data, void *hint)
    {
        free(hint);
    }
};

/*
* @tc.name: CachedExternalEcmaStringCastTest
* @tc.desc: Test base functions in class CachedExternalEcmaString
* @tc.type: FUNC
*/
HWTEST_F_L0(EcmaExternalStringTest, CachedExternalEcmaStringBaseTestUtf8)
{
    uint8_t *hint = (uint8_t *)std::malloc(TYPE_INFO_SIZE + UTF8_CACHED_DATA_SIZE);
    uint8_t *cachedData = hint + TYPE_INFO_SIZE;
    memset_s(cachedData, UTF8_CACHED_DATA_SIZE, 'a', UTF8_CACHED_DATA_SIZE);
    EcmaString *ecmaString = EcmaStringAccessor::CreateFromExternalResource(
        instance, cachedData, UTF8_CACHED_DATA_SIZE, true, EcmaExternalStringTest::CallBackFn, hint);

    // Test cast functions in calss CachedExternalEcmaString.
    JSHandle<EcmaString> ecmaStrHandle(thread, ecmaString);
    CachedExternalEcmaString *cachedEcmaString = CachedExternalEcmaString::Cast(ecmaString);
    EXPECT_NE(cachedEcmaString, nullptr);
    EXPECT_NE(CachedExternalEcmaString::Cast(*ecmaStrHandle), nullptr);
    EXPECT_NE(CachedExternalEcmaString::ConstCast(*ecmaStrHandle), nullptr);
    EXPECT_NE(CachedExternalEcmaString::Cast(ecmaStrHandle.GetTaggedValue()), nullptr);
    EXPECT_NE(cachedEcmaString, nullptr);
    EXPECT_NE(CachedExternalEcmaString::Cast(const_cast<EcmaString *>(ecmaString)), nullptr);
    CachedExternalString *cachedExternalString = cachedEcmaString->ToCachedExternalString();
    EXPECT_NE(cachedExternalString, nullptr);
    const CachedExternalString *constCachedExternalString = cachedEcmaString->ToCachedExternalString();
    EXPECT_NE(constCachedExternalString, nullptr);
    EXPECT_NE(CachedExternalEcmaString::FromBaseString(cachedExternalString), nullptr);
}

/*
* @tc.name: CachedExternalEcmaStringBaseTestUtf16
* @tc.desc: Test the basic interfaces in class CachedExternalString while input utf16 data
* @tc.type: FUNC
*/
HWTEST_F_L0(EcmaExternalStringTest, CachedExternalEcmaStringBaseTestUtf16)
{
    uint8_t *hint = (uint8_t *)std::malloc(TYPE_INFO_SIZE + UTF16_CACHED_DATA_SIZE);
    uint8_t *cachedData = hint + TYPE_INFO_SIZE;
    memset_s(cachedData, UTF16_CACHED_DATA_SIZE, 0xff, UTF16_CACHED_DATA_SIZE);
    EcmaString *ecmaString = EcmaStringAccessor::CreateFromExternalResource(
        instance, cachedData, UTF16_STRING_LENGTH, false, EcmaExternalStringTest::CallBackFn, hint);

    CachedExternalString *cachedExternalString = CachedExternalEcmaString::Cast(ecmaString)->ToCachedExternalString();
    EXPECT_EQ(cachedExternalString->Get(0), 0xffff);
}

/*
* @tc.name: CreateFromExternalResourceTest
* @tc.desc: Test a function that create ECMA string from external resource
* @tc.type: FUNC
*/
HWTEST_F_L0(EcmaExternalStringTest, CreateFromExternalResourceTest)
{
    uint8_t *hint = (uint8_t *)std::malloc(TYPE_INFO_SIZE + UTF8_CACHED_DATA_SIZE);
    uint8_t *cachedData = hint + TYPE_INFO_SIZE;
    memset_s(cachedData, UTF8_CACHED_DATA_SIZE, 'a', UTF8_CACHED_DATA_SIZE);
    EcmaString *ecmaString = EcmaStringAccessor::CreateFromExternalResource(
        instance, cachedData, UTF8_CACHED_DATA_SIZE, true, EcmaExternalStringTest::CallBackFn, hint);

    EcmaStringAccessor strAcc(ecmaString);
    EXPECT_TRUE(strAcc.IsCachedExternalString());
    EXPECT_TRUE(strAcc.IsLineOrCachedExternalString());
    EXPECT_EQ(strAcc.GetLength(), UTF8_CACHED_DATA_SIZE);
    EXPECT_STREQ(EcmaStringAccessor(ecmaString).ToCString(thread).c_str(), "aaaa");
}

/*
* @tc.name: GetNonTreeUtf8DataTest
* @tc.desc: Test function EcmaStringAccessor::GetNonTreeUtf8Data
* @tc.type: FUNC
*/
HWTEST_F_L0(EcmaExternalStringTest, GetNonTreeUtf8DataTest)
{
    uint8_t *hint = (uint8_t *)std::malloc(TYPE_INFO_SIZE + UTF8_CACHED_DATA_SIZE);
    uint8_t *cachedData = hint + TYPE_INFO_SIZE;
    memset_s(cachedData, UTF8_CACHED_DATA_SIZE, 'a', UTF8_CACHED_DATA_SIZE);

    EcmaString *ecmaStr = EcmaStringAccessor::CreateFromUtf8(instance, cachedData, UTF8_CACHED_DATA_SIZE, true);
    const uint8_t *lineStrData = EcmaStringAccessor::GetNonTreeUtf8Data(thread, ecmaStr);
    EXPECT_EQ(lineStrData[0], 'a');
    ecmaStr = EcmaStringAccessor::CreateFromExternalResource(instance, cachedData, UTF8_CACHED_DATA_SIZE, true,
                                                             EcmaExternalStringTest::CallBackFn, hint);
    const uint8_t *externalStrData = EcmaStringAccessor::GetNonTreeUtf8Data(thread, ecmaStr);
    EXPECT_EQ(externalStrData[0], 'a');
    EXPECT_EQ(externalStrData, cachedData);
}

/*
* @tc.name: GetNonTreeUtf16DataTest
* @tc.desc: Test function EcmaStringAccessor::GetNonTreeUtf16DataTest
* @tc.type: FUNC
*/
HWTEST_F_L0(EcmaExternalStringTest, GetNonTreeUtf16DataTest)
{
    uint8_t *hint = (uint8_t *)std::malloc(TYPE_INFO_SIZE + UTF16_CACHED_DATA_SIZE);
    uint16_t *cachedData = reinterpret_cast<uint16_t *>(hint + TYPE_INFO_SIZE);
    memset_s(cachedData, UTF16_CACHED_DATA_SIZE, 0xff, UTF16_CACHED_DATA_SIZE);

    EcmaString *ecmaStr = EcmaStringAccessor::CreateFromUtf16(instance, cachedData, UTF16_STRING_LENGTH, false);
    const uint16_t *lineStrData = EcmaStringAccessor::GetNonTreeUtf16Data(thread, ecmaStr);
    EXPECT_EQ(lineStrData[0], 0xffff);
    ecmaStr = EcmaStringAccessor::CreateFromExternalResource(instance, cachedData, UTF16_STRING_LENGTH, false,
                                                             EcmaExternalStringTest::CallBackFn, hint);
    const uint16_t *externalStrData = EcmaStringAccessor::GetNonTreeUtf16Data(thread, ecmaStr);
    EXPECT_EQ(externalStrData, cachedData);
    EXPECT_EQ(externalStrData[0], 0xffff);
}

/*
* @tc.name: GetUtf16DataFlatTest
* @tc.desc: Test function EcmaStringAccessor::GetUtf16DataFlat
* @tc.type: FUNC
*/
HWTEST_F_L0(EcmaExternalStringTest, GetUtf16DataFlatTest)
{
    uint8_t *hint = (uint8_t *)std::malloc(TYPE_INFO_SIZE + UTF16_CACHED_DATA_SIZE);
    uint16_t *cachedData = reinterpret_cast<uint16_t *>(hint + TYPE_INFO_SIZE);
    memset_s(cachedData, UTF16_CACHED_DATA_SIZE, 0xff, UTF16_CACHED_DATA_SIZE);

    EcmaString *ecmaStr = EcmaStringAccessor::CreateFromUtf16(instance, cachedData, UTF16_STRING_LENGTH, false);
    CVector<uint16_t> utf16Buf1;
    const uint16_t *lineStrData = EcmaStringAccessor::GetUtf16DataFlat(thread, ecmaStr, utf16Buf1);
    EXPECT_EQ(lineStrData[0], 0xffff);
    ecmaStr = EcmaStringAccessor::CreateFromExternalResource(instance, cachedData, UTF16_STRING_LENGTH, false,
                                                             EcmaExternalStringTest::CallBackFn, hint);
    CVector<uint16_t> utf16Buf2;
    const uint16_t *externalStrData = EcmaStringAccessor::GetUtf16DataFlat(thread, ecmaStr, utf16Buf2);
    EXPECT_EQ(externalStrData, cachedData);
    EXPECT_EQ(externalStrData[0], 0xffff);
}

/*
* @tc.name: GetUtf8DataFlatTest
* @tc.desc: Test function EcmaStringAccessor::GetUtf8DataFlat
* @tc.type: FUNC
*/
HWTEST_F_L0(EcmaExternalStringTest, GetUtf8DataFlatTest)
{
    uint8_t *hint = (uint8_t *)std::malloc(TYPE_INFO_SIZE + UTF8_CACHED_DATA_SIZE);
    uint8_t *cachedData = hint + TYPE_INFO_SIZE;
    memset_s(cachedData, UTF8_CACHED_DATA_SIZE, 'a', UTF8_CACHED_DATA_SIZE);

    EcmaString *ecmaStr = EcmaStringAccessor::CreateFromUtf8(instance, cachedData, UTF8_CACHED_DATA_SIZE, true);
    CVector<uint8_t> utf8Buf1;
    const uint8_t *lineStrData = EcmaStringAccessor::GetUtf8DataFlat(thread, ecmaStr, utf8Buf1);
    EXPECT_EQ(lineStrData[0], 'a');
    ecmaStr = EcmaStringAccessor::CreateFromExternalResource(instance, cachedData, UTF8_CACHED_DATA_SIZE, true,
                                                             EcmaExternalStringTest::CallBackFn, hint);
    CVector<uint8_t> utf8Buf2;
    const uint8_t *externalStrData = EcmaStringAccessor::GetUtf8DataFlat(thread, ecmaStr, utf8Buf2);
    EXPECT_EQ(externalStrData[0], 'a');
    EXPECT_EQ(externalStrData, cachedData);
}

/*
* @tc.name: AtTest
* @tc.desc: Test function EcmaString::At
* @tc.type: FUNC
*/
HWTEST_F_L0(EcmaExternalStringTest, AtTest)
{
    uint8_t *hint = (uint8_t *)std::malloc(TYPE_INFO_SIZE + UTF8_CACHED_DATA_SIZE);
    uint8_t *cachedData = hint + TYPE_INFO_SIZE;
    memset_s(cachedData, UTF8_CACHED_DATA_SIZE, 'a', UTF8_CACHED_DATA_SIZE);
    EcmaString *ecmaString = EcmaStringAccessor::CreateFromExternalResource(
        instance, cachedData, UTF8_CACHED_DATA_SIZE, true, EcmaExternalStringTest::CallBackFn, hint);

    EcmaStringAccessor strAcc(ecmaString);
    EXPECT_EQ(strAcc.Get(thread, 0), 'a');
}

/*
* @tc.name: WriteToFlatTest
* @tc.desc: Test function EcmaString::WriteToFlat
* @tc.type: FUNC
*/
HWTEST_F_L0(EcmaExternalStringTest, WriteToFlatTest)
{
    // Test for utf8 data
    uint8_t *hint = (uint8_t *)std::malloc(TYPE_INFO_SIZE + UTF8_CACHED_DATA_SIZE);
    uint8_t *cachedData = hint + TYPE_INFO_SIZE;
    memset_s(cachedData, UTF8_CACHED_DATA_SIZE, 'a', UTF8_CACHED_DATA_SIZE);
    EcmaString *ecmaString = EcmaStringAccessor::CreateFromExternalResource(
        instance, cachedData, UTF8_CACHED_DATA_SIZE, true, EcmaExternalStringTest::CallBackFn, hint);

    char *buf = (char *)malloc(UTF8_CACHED_DATA_SIZE);
    EcmaStringAccessor::WriteToFlat(thread, ecmaString, buf, UTF8_CACHED_DATA_SIZE);
    EXPECT_EQ(buf[0], 'a');
    free(buf);

    // Test for utf16 data
    uint8_t *hintForUtf16 = (uint8_t *)std::malloc(TYPE_INFO_SIZE + UTF16_CACHED_DATA_SIZE);
    uint8_t *cachedDataForUtf16 = hintForUtf16 + TYPE_INFO_SIZE;
    memset_s(cachedDataForUtf16, UTF16_CACHED_DATA_SIZE, 'a', UTF16_CACHED_DATA_SIZE);
    EcmaString *ecmaStrUft16 = EcmaStringAccessor::CreateFromExternalResource(
        instance, cachedDataForUtf16, UTF16_STRING_LENGTH, true, EcmaExternalStringTest::CallBackFn, hintForUtf16);

    char *bufForUtf16 = (char *)malloc(UTF16_CACHED_DATA_SIZE);
    EcmaStringAccessor::WriteToFlat(thread, ecmaStrUft16, bufForUtf16, UTF16_STRING_LENGTH);
    EXPECT_EQ(bufForUtf16[0], 'a');
    free(bufForUtf16);
}

/*
* @tc.name: WriteToFlatWithPosTest
* @tc.desc: Test function EcmaString::WriteToFlatWithPos
* @tc.type: FUNC
*/
HWTEST_F_L0(EcmaExternalStringTest, WriteToFlatWithPosTest)
{
    // Test for utf8 data
    uint8_t *hint = (uint8_t *)std::malloc(TYPE_INFO_SIZE + UTF8_CACHED_DATA_SIZE);
    uint8_t *cachedData = hint + TYPE_INFO_SIZE;
    memset_s(cachedData, UTF8_CACHED_DATA_SIZE, 'a', UTF8_CACHED_DATA_SIZE);
    EcmaString *ecmaString = EcmaStringAccessor::CreateFromExternalResource(
        instance, cachedData, UTF8_CACHED_DATA_SIZE, true, EcmaExternalStringTest::CallBackFn, hint);

    char *buf = (char *)malloc(UTF8_CACHED_DATA_SIZE);
    EcmaStringAccessor::WriteToFlatWithPos(thread, ecmaString, buf, UTF8_CACHED_DATA_SIZE, 0);
    EXPECT_EQ(buf[0], 'a');
    free(buf);

    // Test for utf16 data
    uint8_t *hintForUtf16 = (uint8_t *)std::malloc(TYPE_INFO_SIZE + UTF16_CACHED_DATA_SIZE);
    uint8_t *cachedDataForUtf16 = hintForUtf16 + TYPE_INFO_SIZE;
    memset_s(cachedDataForUtf16, UTF16_CACHED_DATA_SIZE, 'a', UTF16_CACHED_DATA_SIZE);
    EcmaString *ecmaStrUft16 = EcmaStringAccessor::CreateFromExternalResource(
        instance, cachedDataForUtf16, UTF16_STRING_LENGTH, true, EcmaExternalStringTest::CallBackFn, hintForUtf16);

    char *bufForUtf16 = (char *)malloc(UTF16_CACHED_DATA_SIZE);
    EcmaStringAccessor::WriteToFlatWithPos(thread, ecmaStrUft16, bufForUtf16, UTF16_STRING_LENGTH, 0);
    EXPECT_EQ(bufForUtf16[0], 'a');
    free(bufForUtf16);
}
} // namespace panda::test
