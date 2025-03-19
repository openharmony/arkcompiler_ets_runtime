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

#include "ecmascript/compiler/aot_file/aot_checksum_helper.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;
namespace panda::test {
class AOTChecksumTest : public testing::Test {
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

HWTEST_F_L0(AOTChecksumTest, ChecksumSerializeTest)
{
    std::unordered_map<CString, uint32_t> testMap;
    testMap.emplace("test1.abc", 12345);
    testMap.emplace("test2.abc", 67890);

    std::vector<char> dataVector;
    AOTChecksumHelper::SerializeChecksumMapToVector(testMap, dataVector);

    ASSERT_FALSE(dataVector.empty());
    ASSERT_EQ(dataVector.back(), '\0');

    uint32_t expectedSize = AOTChecksumHelper::CalculateChecksumVectorSize(testMap);
    ASSERT_EQ(dataVector.size(), expectedSize);

    std::unordered_map<CString, uint32_t> resultMap;
    AOTChecksumHelper::DeserializeChecksumMapFromChar(dataVector.data(), dataVector.size(), resultMap);

    ASSERT_EQ(resultMap.size(), testMap.size());
    for (const auto &pair : testMap) {
        auto it = resultMap.find(pair.first);
        ASSERT_TRUE(it != resultMap.end());
        ASSERT_EQ(it->second, pair.second);
    }
};

HWTEST_F_L0(AOTChecksumTest, EmptyMapTestTest)
{
    std::unordered_map<CString, uint32_t> emptyMap;
    std::vector<char> dataVector;
    ASSERT_FALSE(AOTChecksumHelper::SerializeChecksumMapToVector(emptyMap, dataVector));
};

HWTEST_F_L0(AOTChecksumTest, MaxChecksumTest)
{
    std::unordered_map<CString, uint32_t> testMap;
    testMap.emplace("test.abc", std::numeric_limits<uint32_t>::max());

    std::vector<char> dataVector;
    AOTChecksumHelper::SerializeChecksumMapToVector(testMap, dataVector);

    ASSERT_FALSE(dataVector.empty());

    std::unordered_map<CString, uint32_t> resultMap;
    AOTChecksumHelper::DeserializeChecksumMapFromChar(dataVector.data(), dataVector.size(), resultMap);

    auto it = resultMap.find("test.abc");
    ASSERT_TRUE(it != resultMap.end());
    ASSERT_EQ(it->second, std::numeric_limits<uint32_t>::max());
};

HWTEST_F_L0(AOTChecksumTest, MuiltiSeparator)
{
    // This case is designed to ensure that in future versions, if a filename contains colons, we should only match the
    // first colon. If some situations that abc contains colons, for security, disable abc load an for aovid different
    // abc checksum
    std::unordered_map<CString, uint32_t> testMap;
    testMap.emplace("path:to:file.abc", 12345);
    testMap.emplace("another:path:test.abc", 67890);

    std::vector<char> dataVector;
    AOTChecksumHelper::SerializeChecksumMapToVector(testMap, dataVector);

    std::unordered_map<CString, uint32_t> resultMap;
    ASSERT_FALSE(AOTChecksumHelper::DeserializeChecksumMapFromChar(dataVector.data(), dataVector.size(), resultMap));
};

HWTEST_F_L0(AOTChecksumTest, NullPtrProvidedTest)
{
    std::unordered_map<CString, uint32_t> resultMap;
    ASSERT_FALSE(AOTChecksumHelper::DeserializeChecksumMapFromChar(nullptr, 10, resultMap));
};

HWTEST_F_L0(AOTChecksumTest, ZeroSizeProvidedTest)
{
    std::unordered_map<CString, uint32_t> resultMap;
    ASSERT_FALSE(AOTChecksumHelper::DeserializeChecksumMapFromChar("test", 0, resultMap));
};

HWTEST_F_L0(AOTChecksumTest, NoSeparatorTest)
{
    std::vector<char> invalidData = {'t',  'e', 's', 't', 'g', 'o', 'o', 'd', ':', '1', '2', '3',
                                     '\0', 't', 'e', 's', 't', 'b', 'a', 'd', '4', '5', '6', '\0'};
    std::unordered_map<CString, uint32_t> resultMap;
    ASSERT_FALSE(AOTChecksumHelper::DeserializeChecksumMapFromChar(invalidData.data(), invalidData.size(), resultMap));
};

HWTEST_F_L0(AOTChecksumTest, NoBigEnoughBufferTest)
{
    std::unordered_map<CString, uint32_t> testMap;
    testMap.emplace("test1.abc", 12345);
    testMap.emplace("test2.abc", 67890);

    uint32_t realSize = AOTChecksumHelper::CalculateChecksumVectorSize(testMap);

    std::vector<char> smallVector(realSize - 1);

    ASSERT_FALSE(AOTChecksumHelper::WriteChecksumInfoToVector(testMap, smallVector));
};

HWTEST_F_L0(AOTChecksumTest, NoFileNameTest)
{
    std::vector<char> invalidData = {'t', 'e', 's',  't', 'g', 'o', 'o', 'd', ':', '1',
                                     '2', '3', '\0', ':', '1', '2', '3', '4', '5', '\0'};

    std::unordered_map<CString, uint32_t> resultMap;
    ASSERT_FALSE(AOTChecksumHelper::DeserializeChecksumMapFromChar(invalidData.data(), invalidData.size(), resultMap));
};

HWTEST_F_L0(AOTChecksumTest, NoChecksumTest)
{
    std::vector<char> invalidData = {'t', 'e', 's', 't',  'g', 'o', 'o', 'd', ':',
                                     '1', '2', '3', '\0', 'b', 'a', 'd', ':', '\0'};

    std::unordered_map<CString, uint32_t> resultMap;
    ASSERT_FALSE(AOTChecksumHelper::DeserializeChecksumMapFromChar(invalidData.data(), invalidData.size(), resultMap));
};

HWTEST_F_L0(AOTChecksumTest, InvalidChecksumTest)
{
    std::vector<char> invalidData = {'t', 'e',  's', 't', 'g', 'o', 'o', 'd', ':', '1', '2',
                                     '3', '\0', 'b', 'a', 'd', ':', 'a', 'b', 'c', '\0'};

    std::unordered_map<CString, uint32_t> resultMap;
    ASSERT_FALSE(AOTChecksumHelper::DeserializeChecksumMapFromChar(invalidData.data(), invalidData.size(), resultMap));
};

HWTEST_F_L0(AOTChecksumTest, NotFullFillChecksumVecTest)
{
    std::unordered_map<CString, uint32_t> testMap;
    testMap.emplace("test1.abc", 12345);
    testMap.emplace("test2.abc", 67890);

    uint32_t correctSize = AOTChecksumHelper::CalculateChecksumVectorSize(testMap);
    std::vector<char> tooLargeVector(correctSize + 10);

    ASSERT_FALSE(AOTChecksumHelper::WriteChecksumInfoToVector(testMap, tooLargeVector));
};

}  // namespace panda::test
