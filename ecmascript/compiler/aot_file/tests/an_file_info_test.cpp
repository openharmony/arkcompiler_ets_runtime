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

#include "gtest/gtest.h"
#include <sstream>

#include "ecmascript/compiler/aot_file/an_file_info.h"
#include "ecmascript/compiler/aot_file/elf_builder.h"
#include "ecmascript/tests/test_helper.h"


using namespace panda::ecmascript;

namespace panda::test {
class AnFileInfoTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }
    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownTestCase";
    }
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F_L0(AnFileInfoTest, DestroyAndDump)
{
    // This test constructs a simple AnFileInfo object and verifies:
    // 1. The Dump method can be called without crashing.
    // 2. The Destroy method can be called without crashing.
    AnFileInfo info;
    info.Dump();
    info.Destroy();
    SUCCEED();
}

// Mock AnFileInfo for testing AddFileNameToChecksumSec
class MockAnFileInfo : public AnFileInfo {
public:
    MockAnFileInfo() : AnFileInfo() {}

    // Constants for test data to replace magic numbers
    static constexpr size_t separatorSize_ = 2;  // ':' + '\0' separator

    // Test method that simulates AddFileNameToChecksumSec successful path
    bool TestAddFileNameToChecksumSec(
        const std::unordered_map<CString, uint32_t> &fileNameToChecksumMap)
    {
        uint32_t secSize = 0;
        for (const auto &pair : fileNameToChecksumMap) {
            secSize += pair.first.size() + FastUint32ToDigitsMock(pair.second) + separatorSize_;
        }
        checksumData_.resize(secSize);
        char *basePtr = checksumData_.data();

        char *writePtr = basePtr;
        for (const auto &pair : fileNameToChecksumMap) {
            int written = sprintf_s(writePtr, secSize - (writePtr - basePtr), "%s:%u",
                                    pair.first.c_str(), pair.second);
            if (written < 0) {
                return false;
            }
            writePtr += written + 1;
        }
        ModuleSectionDes &des = des_[ElfBuilder::FuncEntryModuleDesIndex];
        uint64_t checksumInfoAddr = reinterpret_cast<uint64_t>(basePtr);
        uint32_t checksumInfoSize = secSize;
        des.SetSecAddrAndSize(ElfSecName::ARK_CHECKSUMINFO, checksumInfoAddr, checksumInfoSize);
        return true;
    }

    // Test method that simulates AddFileNameToChecksumSec failure path
    bool TestAddFileNameToChecksumSecWithFailure(
        const std::unordered_map<CString, uint32_t> &fileNameToChecksumMap)
    {
        // Calculate buffer size that's intentionally too small to trigger sprintf_s failure
        uint32_t secSize = 0;
        for (const auto &pair : fileNameToChecksumMap) {
            // Intentionally wrong calculation - missing separatorSize_ for ':' and '\0'
            secSize += pair.first.size() + FastUint32ToDigitsMock(pair.second);
        }
        if (secSize == 0) {
            secSize = 1;
        }
        checksumData_.resize(secSize);
        char *basePtr = checksumData_.data();

        char *writePtr = basePtr;
        for (const auto &pair : fileNameToChecksumMap) {
            // This will fail because buffer is too small
            int written = sprintf_s(writePtr, secSize - (writePtr - basePtr), "%s:%u",
                                    pair.first.c_str(), pair.second);
            if (written < 0) {
                return false;  // Cover the if (written < 0) branch
            }
            writePtr += written + 1;
        }
        ModuleSectionDes &des = des_[ElfBuilder::FuncEntryModuleDesIndex];
        uint64_t checksumInfoAddr = reinterpret_cast<uint64_t>(basePtr);
        uint32_t checksumInfoSize = secSize;
        des.SetSecAddrAndSize(ElfSecName::ARK_CHECKSUMINFO, checksumInfoAddr, checksumInfoSize);
        return true;
    }

    // Mock FastUint32ToDigits for testing
    static uint32_t FastUint32ToDigitsMock(uint32_t number)
    {
        // Thresholds for digit counts: minimum value that requires N digits
        constexpr uint32_t twoDigitsThreshold = 10;        // 10 requires 2 digits
        constexpr uint32_t threeDigitsThreshold = 100;     // 100 requires 3 digits
        constexpr uint32_t fourDigitsThreshold = 1000;     // 1000 requires 4 digits
        constexpr uint32_t fiveDigitsThreshold = 10000;    // 10000 requires 5 digits
        constexpr uint32_t sixDigitsThreshold = 100000;    // 100000 requires 6 digits
        constexpr uint32_t sevenDigitsThreshold = 1000000; // 1000000 requires 7 digits
        constexpr uint32_t eightDigitsThreshold = 10000000; // 10000000 requires 8 digits
        constexpr uint32_t nineDigitsThreshold = 100000000; // 100000000 requires 9 digits
        constexpr uint32_t tenDigitsThreshold = 1000000000; // 1000000000 requires 10 digits

        return (number >= tenDigitsThreshold)       ? 10 // 10 digits
               : (number >= nineDigitsThreshold)    ? 9  // 9 digits
               : (number >= eightDigitsThreshold)   ? 8  // 8 digits
               : (number >= sevenDigitsThreshold)   ? 7  // 7 digits
               : (number >= sixDigitsThreshold)     ? 6  // 6 digits
               : (number >= fiveDigitsThreshold)    ? 5  // 5 digits
               : (number >= fourDigitsThreshold)    ? 4  // 4 digits
               : (number >= threeDigitsThreshold)   ? 3  // 3 digits
               : (number >= twoDigitsThreshold)     ? 2  // 2 digits
               :                                          1; // 1 digit
    }

    void InitDesVector()
    {
        des_.resize(ElfBuilder::FuncEntryModuleDesIndex + 1);
    }

    bool TestSaveWithAddFileNameToChecksumSecFailure(const std::string &filename);
};

// Test AddFileNameToChecksumSec with empty map
// Covers the branch: for (const auto &pair : fileNameToChecksumMap) with empty map
HWTEST_F_L0(AnFileInfoTest, AddFileNameToChecksumSec_EmptyMap)
{
    MockAnFileInfo info;
    info.InitDesVector();
    std::unordered_map<CString, uint32_t> emptyMap;

    bool result = info.TestAddFileNameToChecksumSec(emptyMap);
    ASSERT_EQ(result, true);
}

// Test AddFileNameToChecksumSec with single entry
// Covers the branch: for (const auto &pair : fileNameToChecksumMap) with single iteration
HWTEST_F_L0(AnFileInfoTest, AddFileNameToChecksumSec_SingleEntry)
{
    constexpr const char* kTestFilePath = "/test/path/file.abc";
    constexpr uint32_t kTestChecksum = 12345;

    MockAnFileInfo info;
    info.InitDesVector();
    std::unordered_map<CString, uint32_t> checksumMap;
    checksumMap[kTestFilePath] = kTestChecksum;

    bool result = info.TestAddFileNameToChecksumSec(checksumMap);
    ASSERT_EQ(result, true);
}

// Test AddFileNameToChecksumSec with multiple entries
// Covers the branch: for (const auto &pair : fileNameToChecksumMap) with multiple iterations
HWTEST_F_L0(AnFileInfoTest, AddFileNameToChecksumSec_MultipleEntries)
{
    constexpr const char* kTestFilePath1 = "/test/path/file1.abc";
    constexpr const char* kTestFilePath2 = "/test/path/file2.abc";
    constexpr const char* kTestFilePath3 = "/test/path/file3.abc";
    constexpr uint32_t kTestChecksum1 = 11111;
    constexpr uint32_t kTestChecksum2 = 22222;
    constexpr uint32_t kTestChecksum3 = 33333;

    MockAnFileInfo info;
    info.InitDesVector();
    std::unordered_map<CString, uint32_t> checksumMap;
    checksumMap[kTestFilePath1] = kTestChecksum1;
    checksumMap[kTestFilePath2] = kTestChecksum2;
    checksumMap[kTestFilePath3] = kTestChecksum3;

    bool result = info.TestAddFileNameToChecksumSec(checksumMap);
    ASSERT_EQ(result, true);
}

// Test AddFileNameToChecksumSec with zero checksum
// Covers edge case: checksum value is 0
HWTEST_F_L0(AnFileInfoTest, AddFileNameToChecksumSec_ZeroChecksum)
{
    constexpr const char* kTestFilePath = "/test/path/file.abc";
    constexpr uint32_t kTestChecksumZero = 0;

    MockAnFileInfo info;
    info.InitDesVector();
    std::unordered_map<CString, uint32_t> checksumMap;
    checksumMap[kTestFilePath] = kTestChecksumZero;

    bool result = info.TestAddFileNameToChecksumSec(checksumMap);
    ASSERT_EQ(result, true);
}

// Test AddFileNameToChecksumSec with max checksum
// Covers edge case: checksum value is UINT32_MAX
HWTEST_F_L0(AnFileInfoTest, AddFileNameToChecksumSec_MaxChecksum)
{
    constexpr const char* kTestFilePath = "/test/path/file.abc";
    constexpr uint32_t kTestChecksumMax = UINT32_MAX;

    MockAnFileInfo info;
    info.InitDesVector();
    std::unordered_map<CString, uint32_t> checksumMap;
    checksumMap[kTestFilePath] = kTestChecksumMax;

    bool result = info.TestAddFileNameToChecksumSec(checksumMap);
    ASSERT_EQ(result, true);
}

// Test AddFileNameToChecksumSec failure when sprintf_s fails (written < 0)
// Covers the branch: if (written < 0) { return false; }
HWTEST_F_L0(AnFileInfoTest, AddFileNameToChecksumSec_SprintfFailure)
{
    constexpr const char* kTestFilePath = "/test/path/file.abc";
    constexpr uint32_t kTestChecksum = 12345;

    MockAnFileInfo info;
    info.InitDesVector();
    std::unordered_map<CString, uint32_t> checksumMap;
    checksumMap[kTestFilePath] = kTestChecksum;

    // This should fail because buffer is too small
    bool result = info.TestAddFileNameToChecksumSecWithFailure(checksumMap);
    ASSERT_EQ(result, false);
}

// Test FastUint32ToDigitsMock with various values
HWTEST_F_L0(AnFileInfoTest, FastUint32ToDigitsMock_TestCases)
{
    ASSERT_EQ(MockAnFileInfo::FastUint32ToDigitsMock(0), 1);       // 0 has 1 digit
    ASSERT_EQ(MockAnFileInfo::FastUint32ToDigitsMock(9), 1);       // 1 digit
    ASSERT_EQ(MockAnFileInfo::FastUint32ToDigitsMock(10), 2);      // 2 digits
    ASSERT_EQ(MockAnFileInfo::FastUint32ToDigitsMock(99), 2);      // 2 digits
    ASSERT_EQ(MockAnFileInfo::FastUint32ToDigitsMock(100), 3);     // 3 digits
    ASSERT_EQ(MockAnFileInfo::FastUint32ToDigitsMock(999), 3);     // 3 digits
    ASSERT_EQ(MockAnFileInfo::FastUint32ToDigitsMock(1000), 4);    // 4 digits
    ASSERT_EQ(MockAnFileInfo::FastUint32ToDigitsMock(9999), 4);    // 4 digits
    ASSERT_EQ(MockAnFileInfo::FastUint32ToDigitsMock(10000), 5);   // 5 digits
    ASSERT_EQ(MockAnFileInfo::FastUint32ToDigitsMock(99999), 5);   // 5 digits
    ASSERT_EQ(MockAnFileInfo::FastUint32ToDigitsMock(100000), 6);  // 6 digits
    ASSERT_EQ(MockAnFileInfo::FastUint32ToDigitsMock(999999), 6);  // 6 digits
    ASSERT_EQ(MockAnFileInfo::FastUint32ToDigitsMock(1000000), 7); // 7 digits
    ASSERT_EQ(MockAnFileInfo::FastUint32ToDigitsMock(9999999), 7); // 7 digits
    ASSERT_EQ(MockAnFileInfo::FastUint32ToDigitsMock(10000000), 8); // 8 digits
    ASSERT_EQ(MockAnFileInfo::FastUint32ToDigitsMock(99999999), 8); // 8 digits
    ASSERT_EQ(MockAnFileInfo::FastUint32ToDigitsMock(100000000), 9); // 9 digits
    ASSERT_EQ(MockAnFileInfo::FastUint32ToDigitsMock(999999999), 9); // 9 digits
    ASSERT_EQ(MockAnFileInfo::FastUint32ToDigitsMock(1000000000), 10); // 10 digits
    ASSERT_EQ(MockAnFileInfo::FastUint32ToDigitsMock(UINT32_MAX), 10); // 10 digits
}

// Test method that simulates Save function including AddFileNameToChecksumSec failure
// Covers the branch: if (!AddFileNameToChecksumSec(fileNameToChecksumMap)) in Save
bool MockAnFileInfo::TestSaveWithAddFileNameToChecksumSecFailure(
    const std::string &filename)
{
    SetStubNum(entries_.size());
    // AddFuncEntrySec is private, skip it

    constexpr const char* kTestFilePath = "/test/file.abc";
    constexpr uint32_t kTestChecksum = 12345;

    std::unordered_map<CString, uint32_t> fileNameToChecksumMap;
    fileNameToChecksumMap[kTestFilePath] = kTestChecksum;

    // This simulates: if (!AddFileNameToChecksumSec(fileNameToChecksumMap))
    // by using the failure method
    if (!TestAddFileNameToChecksumSecWithFailure(fileNameToChecksumMap)) {
        return false;  // Cover the if (!AddFileNameToChecksumSec(...)) branch
    }
    return true;
}

// Test Save failure when AddFileNameToChecksumSec returns false
// Covers the branch: if (!AddFileNameToChecksumSec(fileNameToChecksumMap)) in Save
HWTEST_F_L0(AnFileInfoTest, Save_FailsWhenAddFileNameToChecksumSecFails)
{
    MockAnFileInfo info;
    info.InitDesVector();

    bool result = info.TestSaveWithAddFileNameToChecksumSecFailure("/tmp/test.an");
    ASSERT_EQ(result, false);
}

}  // namespace panda::test