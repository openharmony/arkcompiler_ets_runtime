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

#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/ecma_string_table_optimization-inl.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/tests/test_helper.h"

using EcmaString = panda::ecmascript::EcmaString;
using EcmaStringAccessor = panda::ecmascript::EcmaStringAccessor;

namespace panda::test {
class BaseStringTableTest : public BaseTestWithScope<false> {
public:
    ecmascript::BaseStringTableInterface<ecmascript::BaseStringTableImpl>::HandleCreator handleCreator_ = [
        ](common::ThreadHolder* holder, ecmascript::BaseString* string)
            -> common::ReadOnlyHandle<ecmascript::BaseString> {
        return JSHandle<EcmaString>(holder->GetJSThread(), EcmaString::FromBaseString(string));
    };
};

/**
 * @tc.name: GetOrInternFlattenString_EmptyString
 * @tc.desc: Write empty string emptyStr to the Intern pool and takes the hash code as its index.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(BaseStringTableTest, GetOrInternFlattenString_EmptyString)
{
    if (!ecmascript::g_isEnableCMCGC) {
        return;
    }
    auto& table = ecmascript::Runtime::GetInstance()->GetBaseStringTable();

    JSHandle<EcmaString> emptyEcmaStrHandle(thread, EcmaStringAccessor::CreateEmptyString(thread->GetEcmaVM()));
    EXPECT_TRUE(!EcmaStringAccessor(emptyEcmaStrHandle).IsInternString());

    table.GetOrInternFlattenString(thread->GetThreadHolder(), handleCreator_, emptyEcmaStrHandle->ToBaseString());
    EXPECT_TRUE(!EcmaStringAccessor(emptyEcmaStrHandle).IsInternString());
    ecmascript::BaseString* emptyEcmaStr = table.TryGetInternString(emptyEcmaStrHandle);
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(emptyEcmaStr)).ToCString(thread).c_str(), "");
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(emptyEcmaStr)).IsInternString());
}

/**
 * @tc.name: GetOrInternString
 * @tc.desc: Obtain EcmaString string from utf8 encoded data. If the string already exists in the detention pool,
             it will be returned directly. If not, it will be added to the detention pool and then returned.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(BaseStringTableTest, GetOrInternString_utf8Data)
{
    if (!ecmascript::g_isEnableCMCGC) {
        return;
    }
    EcmaVM* vm = thread->GetEcmaVM();
    auto& table = ecmascript::Runtime::GetInstance()->GetBaseStringTable();

    uint8_t utf8Data[] = {0x68, 0x65, 0x6c, 0x6c, 0x6f}; // " hello "
    EcmaString* ecmaStrCreatePtr = EcmaStringAccessor::CreateFromUtf8(vm, utf8Data, sizeof(utf8Data), true);
    EXPECT_TRUE(!EcmaStringAccessor(ecmaStrCreatePtr).IsInternString());

    ecmascript::BaseString* ecmaStrGetPtr = table.GetOrInternString(thread->GetThreadHolder(), handleCreator_, utf8Data,
                                                                sizeof(utf8Data), true);
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(ecmaStrGetPtr)).ToCString(thread).c_str(), "hello");
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(ecmaStrGetPtr)).IsInternString());
}

/**
 * @tc.name: GetOrInternString
 * @tc.desc: Obtain EcmaString string from utf16 encoded data. If the string already exists in the detention pool,
             it will be returned directly. If not, it will be added to the detention pool and then returned.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(BaseStringTableTest, GetOrInternString_utf16Data)
{
    if (!ecmascript::g_isEnableCMCGC) {
        return;
    }
    EcmaVM* vm = thread->GetEcmaVM();
    auto& table = ecmascript::Runtime::GetInstance()->GetBaseStringTable();

    uint16_t utf16Data[] = {0x7F16, 0x7801, 0x89E3, 0x7801}; // “ 编码解码 ”
    EcmaString* ecmaStrCreatePtr =
        EcmaStringAccessor::CreateFromUtf16(vm, utf16Data, sizeof(utf16Data) / sizeof(uint16_t), false);
    EXPECT_TRUE(!EcmaStringAccessor(ecmaStrCreatePtr).IsInternString());

    ecmascript::BaseString* ecmaStrGetPtr = table.GetOrInternString(thread->GetThreadHolder(), handleCreator_,
        utf16Data, sizeof(utf16Data) / sizeof(uint16_t), false);
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(ecmaStrGetPtr)).ToCString(thread).c_str(), "编码解码");
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(ecmaStrGetPtr)).IsInternString());
}

/**
 * @tc.name: GetOrInternStringFromCompressedSubString_SubString
 * @tc.desc: Test creating interned string from a compressed substring of another string
 * @tc.type: FUNC
 * @tc.require: AR001
 */
HWTEST_F_L0(BaseStringTableTest, GetOrInternStringFromCompressedSubString_SubString)
{
    if (!ecmascript::g_isEnableCMCGC) {
        return;
    }
    EcmaVM* vm = thread->GetEcmaVM();
    ecmascript::ObjectFactory* factory = vm->GetFactory();
    auto& table = ecmascript::Runtime::GetInstance()->GetBaseStringTable();

    JSHandle<EcmaString> originalStr =
        factory->NewFromASCII("00000x680x650x6c0x6c0x6f0x200x770x6f0x720x6c0x64"); // "hello world"
    uint32_t offset = 4;
    uint32_t utf8Len = EcmaStringAccessor(*originalStr).GetLength() - offset;

    ecmascript::BaseString* internStr = table.GetOrInternStringFromCompressedSubString(
        thread->GetThreadHolder(), handleCreator_, originalStr, offset, utf8Len);
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(internStr)).ToCString(thread).c_str(),
                 "0x680x650x6c0x6c0x6f0x200x770x6f0x720x6c0x64");
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(internStr)).IsInternString());
}

/**
 * @tc.name: GetOrInternString_EmptyStringWithCompression
 * @tc.desc: Test interning empty string with compression enabled
 * @tc.type: FUNC
 * @tc.require: AR001
 */
HWTEST_F_L0(BaseStringTableTest, GetOrInternString_EmptyStringWithCompression)
{
    if (!ecmascript::g_isEnableCMCGC) {
        return;
    }
    auto& table = ecmascript::Runtime::GetInstance()->GetBaseStringTable();

    uint8_t emptyData[] = {0};
    ecmascript::BaseString* result = table.GetOrInternString(
        thread->GetThreadHolder(), handleCreator_, emptyData, 0, true);
    EXPECT_NE(result, nullptr);
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(result)).ToCString(thread).c_str(), "");
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(result)).IsInternString());
}

/**
 * @tc.name: GetOrInternString_EmptyStringWithoutCompression
 * @tc.desc: Test interning empty string with compression disabled
 * @tc.type: FUNC
 * @tc.require: AR001
 */
HWTEST_F_L0(BaseStringTableTest, GetOrInternString_EmptyStringWithoutCompression)
{
    if (!ecmascript::g_isEnableCMCGC) {
        return;
    }
    auto& table = ecmascript::Runtime::GetInstance()->GetBaseStringTable();

    uint8_t emptyData[] = {0};
    ecmascript::BaseString* result = table.GetOrInternString(
        thread->GetThreadHolder(), handleCreator_, emptyData, 0, false);
    EXPECT_NE(result, nullptr);
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(result)).ToCString(thread).c_str(), "");
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(result)).IsInternString());
}

/**
 * @tc.name: GetOrInternString_SingleCharacter
 * @tc.desc: Test interning single character strings
 * @tc.type: FUNC
 * @tc.require: AR001
 */
HWTEST_F_L0(BaseStringTableTest, GetOrInternString_SingleCharacter)
{
    if (!ecmascript::g_isEnableCMCGC) {
        return;
    }
    auto& table = ecmascript::Runtime::GetInstance()->GetBaseStringTable();

    uint8_t singleChar[] = {'A'};
    ecmascript::BaseString* result = table.GetOrInternString(
        thread->GetThreadHolder(), handleCreator_, singleChar, sizeof(singleChar), true);
    EXPECT_NE(result, nullptr);
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(result)).ToCString(thread).c_str(), "A");
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(result)).IsInternString());
}

/**
 * @tc.name: GetOrInternString_LongString
 * @tc.desc: Test interning long string (multiple segments)
 * @tc.type: FUNC
 * @tc.require: AR001
 */
HWTEST_F_L0(BaseStringTableTest, GetOrInternString_LongString)
{
    if (!ecmascript::g_isEnableCMCGC) {
        return;
    }
    auto& table = ecmascript::Runtime::GetInstance()->GetBaseStringTable();

    std::string longString(100, 'x'); // 100 character string
    ecmascript::BaseString* result = table.GetOrInternString(
        thread->GetThreadHolder(), handleCreator_,
        reinterpret_cast<const uint8_t*>(longString.c_str()),
        longString.length(), true);
    EXPECT_NE(result, nullptr);
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(result)).ToCString(thread).c_str(),
                 longString.c_str());
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(result)).IsInternString());
}

/**
 * @tc.name: GetOrInternString_RepeatedString
 * @tc.desc: Test interning same string multiple times returns same object
 * @tc.type: FUNC
 * @tc.require: AR001
 */
HWTEST_F_L0(BaseStringTableTest, GetOrInternString_RepeatedString)
{
    if (!ecmascript::g_isEnableCMCGC) {
        return;
    }
    auto& table = ecmascript::Runtime::GetInstance()->GetBaseStringTable();

    uint8_t testData[] = {'d', 'u', 'p', 'l', 'i', 'c', 'a', 't', 'e'};

    ecmascript::BaseString* result1 = table.GetOrInternString(
        thread->GetThreadHolder(), handleCreator_, testData, sizeof(testData), true);
    ecmascript::BaseString* result2 = table.GetOrInternString(
        thread->GetThreadHolder(), handleCreator_, testData, sizeof(testData), true);

    EXPECT_EQ(result1, result2);
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(result1)).ToCString(thread).c_str(), "duplicate");
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(result1)).IsInternString());
}

/**
 * @tc.name: GetOrInternString_NumericString
 * @tc.desc: Test interning numeric string
 * @tc.type: FUNC
 * @tc.require: AR001
 */
HWTEST_F_L0(BaseStringTableTest, GetOrInternString_NumericString)
{
    if (!ecmascript::g_isEnableCMCGC) {
        return;
    }
    auto& table = ecmascript::Runtime::GetInstance()->GetBaseStringTable();

    uint8_t numericData[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'};
    ecmascript::BaseString* result = table.GetOrInternString(
        thread->GetThreadHolder(), handleCreator_, numericData, sizeof(numericData), true);
    EXPECT_NE(result, nullptr);
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(result)).ToCString(thread).c_str(), "1234567890");
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(result)).IsInternString());
}

/**
 * @tc.name: GetOrInternString_SpecialCharacters
 * @tc.desc: Test interning string with special ASCII characters
 * @tc.type: FUNC
 * @tc.require: AR001
 */
HWTEST_F_L0(BaseStringTableTest, GetOrInternString_SpecialCharacters)
{
    if (!ecmascript::g_isEnableCMCGC) {
        return;
    }
    auto& table = ecmascript::Runtime::GetInstance()->GetBaseStringTable();

    uint8_t specialData[] = {'!', '@', '#', '$', '%', '^', '&', '*', '(', ')'};
    ecmascript::BaseString* result = table.GetOrInternString(
        thread->GetThreadHolder(), handleCreator_, specialData, sizeof(specialData), true);
    EXPECT_NE(result, nullptr);
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(result)).ToCString(thread).c_str(), "!@#$%^&*()");
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(result)).IsInternString());
}

/**
 * @tc.name: GetOrInternString_WhitespaceString
 * @tc.desc: Test interning string with only whitespace characters
 * @tc.type: FUNC
 * @tc.require: AR001
 */
HWTEST_F_L0(BaseStringTableTest, GetOrInternString_WhitespaceString)
{
    if (!ecmascript::g_isEnableCMCGC) {
        return;
    }
    auto& table = ecmascript::Runtime::GetInstance()->GetBaseStringTable();

    uint8_t whitespaceData[] = {' ', '\t', '\n', '\r', ' ', '\f'};
    ecmascript::BaseString* result = table.GetOrInternString(
        thread->GetThreadHolder(), handleCreator_, whitespaceData, sizeof(whitespaceData), true);
    EXPECT_NE(result, nullptr);
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(result)).IsInternString());
}

/**
 * @tc.name: GetOrInternStringFromCompressedSubString_FullString
 * @tc.desc: Test creating substring from entire string (offset 0)
 * @tc.type: FUNC
 * @tc.require: AR001
 */
HWTEST_F_L0(BaseStringTableTest, GetOrInternStringFromCompressedSubString_FullString)
{
    if (!ecmascript::g_isEnableCMCGC) {
        return;
    }
    ecmascript::ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    auto& table = ecmascript::Runtime::GetInstance()->GetBaseStringTable();

    JSHandle<EcmaString> originalStr = factory->NewFromASCII("testcase");
    uint32_t offset = 0;
    uint32_t utf8Len = EcmaStringAccessor(*originalStr).GetLength();

    ecmascript::BaseString* internStr = table.GetOrInternStringFromCompressedSubString(
        thread->GetThreadHolder(), handleCreator_, originalStr, offset, utf8Len);
    EXPECT_NE(internStr, nullptr);
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(internStr)).ToCString(thread).c_str(), "testcase");
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(internStr)).IsInternString());
}

/**
 * @tc.name: GetOrInternStringFromCompressedSubString_OneCharSubstring
 * @tc.desc: Test creating single character substring
 * @tc.type: FUNC
 * @tc.require: AR001
 */
HWTEST_F_L0(BaseStringTableTest, GetOrInternStringFromCompressedSubString_OneCharSubstring)
{
    if (!ecmascript::g_isEnableCMCGC) {
        return;
    }
    ecmascript::ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    auto& table = ecmascript::Runtime::GetInstance()->GetBaseStringTable();

    JSHandle<EcmaString> originalStr = factory->NewFromASCII("abcdef");
    uint32_t offset = 2;
    uint32_t utf8Len = 1;

    ecmascript::BaseString* internStr = table.GetOrInternStringFromCompressedSubString(
        thread->GetThreadHolder(), handleCreator_, originalStr, offset, utf8Len);
    EXPECT_NE(internStr, nullptr);
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(internStr)).ToCString(thread).c_str(), "c");
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(internStr)).IsInternString());
}

/**
 * @tc.name: GetOrInternStringFromCompressedSubString_RepeatedSubstring
 * @tc.desc: Test creating same substring multiple times
 * @tc.type: FUNC
 * @tc.require: AR001
 */
HWTEST_F_L0(BaseStringTableTest, GetOrInternStringFromCompressedSubString_RepeatedSubstring)
{
    if (!ecmascript::g_isEnableCMCGC) {
        return;
    }
    ecmascript::ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    auto& table = ecmascript::Runtime::GetInstance()->GetBaseStringTable();

    JSHandle<EcmaString> originalStr1 = factory->NewFromASCII("firsttest");
    JSHandle<EcmaString> originalStr2 = factory->NewFromASCII("secondtest");
    uint32_t offset = 5;
    uint32_t utf8Len = 4;

    ecmascript::BaseString* internStr1 = table.GetOrInternStringFromCompressedSubString(
        thread->GetThreadHolder(), handleCreator_, originalStr1, offset, utf8Len);
    ecmascript::BaseString* internStr2 = table.GetOrInternStringFromCompressedSubString(
        thread->GetThreadHolder(), handleCreator_, originalStr2, offset, utf8Len);

    EXPECT_EQ(internStr1, internStr2);
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(internStr1)).ToCString(thread).c_str(), "test");
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(internStr1)).IsInternString());
}

/**
 * @tc.name: GetOrInternString_HashCollisionTest
 * @tc.desc: Test strings that might cause hash collisions
 * @tc.type: FUNC
 * @tc.require: AR001
 */
HWTEST_F_L0(BaseStringTableTest, GetOrInternString_HashCollisionTest)
{
    if (!ecmascript::g_isEnableCMCGC) {
        return;
    }
    auto& table = ecmascript::Runtime::GetInstance()->GetBaseStringTable();

    // Strings that might have similar hash values
    uint8_t data1[] = {'A', 'a', '1'};
    uint8_t data2[] = {'B', 'B', '1'};

    ecmascript::BaseString* result1 = table.GetOrInternString(
        thread->GetThreadHolder(), handleCreator_, data1, sizeof(data1), true);
    ecmascript::BaseString* result2 = table.GetOrInternString(
        thread->GetThreadHolder(), handleCreator_, data2, sizeof(data2), true);

    EXPECT_NE(result1, result2);
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(result1)).ToCString(thread).c_str(), "Aa1");
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(result2)).ToCString(thread).c_str(), "BB1");
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(result1)).IsInternString());
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(result2)).IsInternString());
}

/**
 * @tc.name: GetOrInternString_UpperCaseLowerCase
 * @tc.desc: Test interning strings with different cases
 * @tc.type: FUNC
 * @tc.require: AR001
 */
HWTEST_F_L0(BaseStringTableTest, GetOrInternString_UpperCaseLowerCase)
{
    if (!ecmascript::g_isEnableCMCGC) {
        return;
    }
    auto& table = ecmascript::Runtime::GetInstance()->GetBaseStringTable();

    uint8_t upperCase[] = {'H', 'E', 'L', 'L', 'O'};
    uint8_t lowerCase[] = {'h', 'e', 'l', 'l', 'o'};

    ecmascript::BaseString* resultUpper = table.GetOrInternString(
        thread->GetThreadHolder(), handleCreator_, upperCase, sizeof(upperCase), true);
    ecmascript::BaseString* resultLower = table.GetOrInternString(
        thread->GetThreadHolder(), handleCreator_, lowerCase, sizeof(lowerCase), true);

    EXPECT_NE(resultUpper, resultLower);
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(resultUpper)).ToCString(thread).c_str(), "HELLO");
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(resultLower)).ToCString(thread).c_str(), "hello");
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(resultUpper)).IsInternString());
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(resultLower)).IsInternString());
}

/**
 * @tc.name: GetOrInternString_LargeDataSet
 * @tc.desc: Test interning many different strings
 * @tc.type: FUNC
 * @tc.require: AR001
 */
HWTEST_F_L0(BaseStringTableTest, GetOrInternString_LargeDataSet)
{
    if (!ecmascript::g_isEnableCMCGC) {
        return;
    }
    auto& table = ecmascript::Runtime::GetInstance()->GetBaseStringTable();

    std::vector<ecmascript::BaseString*> results;
    const int numStrings = 50;

    for (int i = 0; i < numStrings; i++) {
        std::string str = "string" + std::to_string(i);
        ecmascript::BaseString* result = table.GetOrInternString(
            thread->GetThreadHolder(), handleCreator_,
            reinterpret_cast<const uint8_t*>(str.c_str()),
            str.length(), true);
        EXPECT_NE(result, nullptr);
        EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(result)).IsInternString());
        results.push_back(result);
    }

    // Verify all strings are unique
    for (int i = 0; i < numStrings; i++) {
        std::string str = "string" + std::to_string(i);
        EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(results[i])).ToCString(thread).c_str(),
                     str.c_str());
    }
}

/**
 * @tc.name: GetOrInternString_MixedCompression
 * @tc.desc: Test interning same data with different compression settings
 * @tc.type: FUNC
 * @tc.require: AR001
 */
HWTEST_F_L0(BaseStringTableTest, GetOrInternString_MixedCompression)
{
    if (!ecmascript::g_isEnableCMCGC) {
        return;
    }
    auto& table = ecmascript::Runtime::GetInstance()->GetBaseStringTable();

    uint8_t testData[] = {'c', 'o', 'm', 'p', 'r', 'e', 's', 's'};

    ecmascript::BaseString* resultCompressed = table.GetOrInternString(
        thread->GetThreadHolder(), handleCreator_, testData, sizeof(testData), true);
    ecmascript::BaseString* resultUncompressed = table.GetOrInternString(
        thread->GetThreadHolder(), handleCreator_, testData, sizeof(testData), false);

    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(
        resultCompressed)).ToCString(thread).c_str(), "compress");
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(
        resultUncompressed)).ToCString(thread).c_str(), "compress");
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(resultCompressed)).IsInternString());
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(resultUncompressed)).IsInternString());
}

/**
 * @tc.name: GetOrInternStringFromCompressedSubString_EmptySubstring
 * @tc.desc: Test creating empty substring from existing string
 * @tc.type: FUNC
 * @tc.require: AR001
 */
HWTEST_F_L0(BaseStringTableTest, GetOrInternStringFromCompressedSubString_EmptySubstring)
{
    if (!ecmascript::g_isEnableCMCGC) {
        return;
    }
    ecmascript::ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    auto& table = ecmascript::Runtime::GetInstance()->GetBaseStringTable();

    JSHandle<EcmaString> originalStr = factory->NewFromASCII("content");
    uint32_t offset = 3;
    uint32_t utf8Len = 0;

    ecmascript::BaseString* internStr = table.GetOrInternStringFromCompressedSubString(
        thread->GetThreadHolder(), handleCreator_, originalStr, offset, utf8Len);
    EXPECT_NE(internStr, nullptr);
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(internStr)).ToCString(thread).c_str(), "");
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(internStr)).IsInternString());
}

} // namespace panda::test
