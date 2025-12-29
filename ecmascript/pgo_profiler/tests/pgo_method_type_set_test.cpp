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

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "gtest/gtest.h"

#include "ecmascript/pgo_profiler/ap_file/pgo_file_info.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_method_type_set.h"
#include "ecmascript/pgo_profiler/pgo_context.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/pgo_profiler/tests/pgo_context_mock.h"
#include "ecmascript/pgo_profiler/types/pgo_profile_type.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::pgo;

namespace panda::test {
class PGOMethodTypeSetTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownTestCase";
    }
};

HWTEST_F_L0(PGOMethodTypeSetTest, PGOMethodTypeSetParseFromBinaryTest)
{
    // Test 1: size exceeds MAX_METHOD_TYPE_SIZE (if (size > MAX_METHOD_TYPE_SIZE))
    PGOContextMock context(PGOProfilerHeader::LAST_VERSION);
    PGOMethodTypeSet methodTypeSet;
    size_t bufferSize = sizeof(uint32_t) + sizeof(uint32_t);
    void *buffer = malloc(bufferSize);
    void *originalBuffer = buffer;
    ASSERT_NE(buffer, nullptr);
    // Write a size larger than MAX_METHOD_TYPE_SIZE
    *reinterpret_cast<uint32_t *>(buffer) = PGOMethodTypeSet::MAX_METHOD_TYPE_SIZE + 1;
    *reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(buffer) + sizeof(uint32_t)) = 0;  // No type infos
    bool result = methodTypeSet.ParseFromBinary(context, &buffer, bufferSize);
    ASSERT_FALSE(result);
    free(originalBuffer);
    // Test 2: successful parsing with size = 0
    bufferSize = sizeof(uint32_t);
    buffer = malloc(bufferSize);
    originalBuffer = buffer;
    ASSERT_NE(buffer, nullptr);
    // Write size = 0 (no type infos)
    *reinterpret_cast<uint32_t *>(buffer) = 0;
    result = methodTypeSet.ParseFromBinary(context, &buffer, bufferSize);
    ASSERT_TRUE(result);  // Should succeed
    free(originalBuffer);
}

HWTEST_F_L0(PGOMethodTypeSetTest, PGOMethodTypeSetProcessToTextSingleScalarTest)
{
    PGOMethodTypeSet methodTypeSet;
    methodTypeSet.AddType(100, PGOSampleType(PGOSampleType::Type::INT));
    TextFormatter fmt;
    methodTypeSet.ProcessToText(fmt);
    std::string text = fmt.Str();
    ASSERT_FALSE(text.empty());
    EXPECT_TRUE(text.find("100") != std::string::npos);
}

HWTEST_F_L0(PGOMethodTypeSetTest, PGOMethodTypeSetProcessToTextMultipleScalarTest)
{
    PGOMethodTypeSet methodTypeSet;
    methodTypeSet.AddType(100, PGOSampleType(PGOSampleType::Type::INT));
    methodTypeSet.AddType(200, PGOSampleType(PGOSampleType::Type::DOUBLE));
    methodTypeSet.AddType(300, PGOSampleType(PGOSampleType::Type::STRING));
    TextFormatter fmt;
    methodTypeSet.ProcessToText(fmt);
    std::string text = fmt.Str();
    ASSERT_FALSE(text.empty());
    EXPECT_TRUE(text.find("100") != std::string::npos);
    EXPECT_TRUE(text.find("200") != std::string::npos);
    EXPECT_TRUE(text.find("300") != std::string::npos);
}

HWTEST_F_L0(PGOMethodTypeSetTest, PGOMethodTypeSetProcessToTextNoneTypeTest)
{
    PGOMethodTypeSet methodTypeSet;
    methodTypeSet.AddType(100, PGOSampleType(PGOSampleType::Type::INT));
    methodTypeSet.AddType(200, PGOSampleType(PGOSampleType::Type::NONE));
    methodTypeSet.AddType(300, PGOSampleType(PGOSampleType::Type::DOUBLE));
    TextFormatter fmt;
    methodTypeSet.ProcessToText(fmt);
    std::string text = fmt.Str();
    ASSERT_FALSE(text.empty());
    EXPECT_TRUE(text.find("100") != std::string::npos);
    EXPECT_TRUE(text.find("200") == std::string::npos);
    EXPECT_TRUE(text.find("300") != std::string::npos);
}

HWTEST_F_L0(PGOMethodTypeSetTest, PGOMethodTypeSetProcessToTextRwScalarTest)
{
    PGOMethodTypeSet methodTypeSet;
    ProfileType profileType1 = ProfileType::CreateMegaType();
    ProfileType profileType2 = ProfileType::CreateMegaType();
    PGOObjectInfo objectInfo(profileType1, profileType2, profileType2,
                             profileType2, profileType2, profileType2,
                             PGOSampleType());
    methodTypeSet.AddObjectInfo(400, objectInfo);
    TextFormatter fmt;
    methodTypeSet.ProcessToText(fmt);
    std::string text = fmt.Str();
    ASSERT_FALSE(text.empty());
    EXPECT_TRUE(text.find("400") != std::string::npos);
}

HWTEST_F_L0(PGOMethodTypeSetTest, PGOMethodTypeSetProcessToTextObjDefTest)
{
    PGOMethodTypeSet methodTypeSet;
    ProfileType profileType = ProfileType::CreateMegaType();
    PGODefineOpType defineOpType(profileType);
    methodTypeSet.AddDefine(500, defineOpType);
    TextFormatter fmt;
    methodTypeSet.ProcessToText(fmt);
    std::string text = fmt.Str();
    ASSERT_FALSE(text.empty());
    EXPECT_TRUE(text.find("500") != std::string::npos);
}

HWTEST_F_L0(PGOMethodTypeSetTest, PGOMethodTypeSetProcessToTextMixedTest)
{
    PGOMethodTypeSet methodTypeSet;
    methodTypeSet.AddType(100, PGOSampleType(PGOSampleType::Type::INT));
    ProfileType profileType1 = ProfileType::CreateMegaType();
    ProfileType profileType2 = ProfileType::CreateMegaType();
    PGOObjectInfo objectInfo(profileType1, profileType2, profileType2,
                             profileType2, profileType2, profileType2,
                             PGOSampleType());
    methodTypeSet.AddObjectInfo(400, objectInfo);
    ProfileType profileType3 = ProfileType::CreateMegaType();
    PGODefineOpType defineOpType(profileType3);
    methodTypeSet.AddDefine(500, defineOpType);
    TextFormatter fmt;
    methodTypeSet.ProcessToText(fmt);
    std::string text = fmt.Str();
    ASSERT_FALSE(text.empty());
    EXPECT_TRUE(text.find("100") != std::string::npos);
    EXPECT_TRUE(text.find("400") != std::string::npos);
    EXPECT_TRUE(text.find("500") != std::string::npos);
}

HWTEST_F_L0(PGOMethodTypeSetTest, PGOMethodTypeSetProcessToJsonEmptyTest)
{
    PGOMethodTypeSet methodTypeSet;
    ProfileType::VariantVector typeArray;
    methodTypeSet.ProcessToJson(typeArray);
    ASSERT_TRUE(typeArray.empty());
}

HWTEST_F_L0(PGOMethodTypeSetTest,
            PGOMethodTypeSetProcessToJsonSingleScalarTest)
{
    PGOMethodTypeSet methodTypeSet;
    methodTypeSet.AddType(100, PGOSampleType(PGOSampleType::Type::INT));
    ProfileType::VariantVector typeArray;
    methodTypeSet.ProcessToJson(typeArray);
    ASSERT_FALSE(typeArray.empty());
    ASSERT_EQ(typeArray.size(), 1U);
    ASSERT_TRUE(std::holds_alternative<ProfileType::StringMap>(typeArray[0]));
    auto typeMap = std::get<ProfileType::StringMap>(typeArray[0]);
    EXPECT_TRUE(typeMap.find("typeOffset") != typeMap.end());
    EXPECT_EQ(typeMap.find("typeOffset")->second, "100");
}

HWTEST_F_L0(PGOMethodTypeSetTest,
            PGOMethodTypeSetProcessToJsonMultipleScalarTest)
{
    PGOMethodTypeSet methodTypeSet;
    methodTypeSet.AddType(100, PGOSampleType(PGOSampleType::Type::INT));
    methodTypeSet.AddType(200, PGOSampleType(PGOSampleType::Type::DOUBLE));
    methodTypeSet.AddType(300, PGOSampleType(PGOSampleType::Type::STRING));
    ProfileType::VariantVector typeArray;
    methodTypeSet.ProcessToJson(typeArray);
    ASSERT_FALSE(typeArray.empty());
    ASSERT_EQ(typeArray.size(), 3U);
    for (const auto& type : typeArray) {
        ASSERT_TRUE(std::holds_alternative<ProfileType::StringMap>(type));
    }
}

HWTEST_F_L0(PGOMethodTypeSetTest, PGOMethodTypeSetProcessToJsonNoneTypeTest)
{
    PGOMethodTypeSet methodTypeSet;
    methodTypeSet.AddType(100, PGOSampleType(PGOSampleType::Type::INT));
    methodTypeSet.AddType(200, PGOSampleType(PGOSampleType::Type::NONE));
    methodTypeSet.AddType(300, PGOSampleType(PGOSampleType::Type::DOUBLE));
    ProfileType::VariantVector typeArray;
    methodTypeSet.ProcessToJson(typeArray);
    ASSERT_FALSE(typeArray.empty());
    ASSERT_EQ(typeArray.size(), 2U);
}

HWTEST_F_L0(PGOMethodTypeSetTest, PGOMethodTypeSetProcessToJsonRwScalarTest)
{
    PGOMethodTypeSet methodTypeSet;
    ProfileType profileType1 = ProfileType::CreateMegaType();
    ProfileType profileType2 = ProfileType::CreateMegaType();
    PGOObjectInfo objectInfo(profileType1, profileType2, profileType2,
                            profileType2, profileType2, profileType2,
                            PGOSampleType());
    methodTypeSet.AddObjectInfo(400, objectInfo);
    ProfileType::VariantVector typeArray;
    methodTypeSet.ProcessToJson(typeArray);
    ASSERT_FALSE(typeArray.empty());
    ASSERT_EQ(typeArray.size(), 1U);
    ASSERT_TRUE(std::holds_alternative<ProfileType::MapVector>(typeArray[0]));
}

HWTEST_F_L0(PGOMethodTypeSetTest, PGOMethodTypeSetProcessToJsonObjDefTest)
{
    PGOMethodTypeSet methodTypeSet;
    ProfileType profileType = ProfileType::CreateMegaType();
    PGODefineOpType defineOpType(profileType);
    methodTypeSet.AddDefine(500, defineOpType);
    ProfileType::VariantVector typeArray;
    methodTypeSet.ProcessToJson(typeArray);
    ASSERT_FALSE(typeArray.empty());
    ASSERT_EQ(typeArray.size(), 1U);
    ASSERT_TRUE(
        std::holds_alternative<std::vector<ProfileType::StringMap>>(typeArray[0]));
}

HWTEST_F_L0(PGOMethodTypeSetTest, PGOMethodTypeSetProcessToJsonMixedTest)
{
    PGOMethodTypeSet methodTypeSet;
    methodTypeSet.AddType(100, PGOSampleType(PGOSampleType::Type::INT));
    ProfileType profileType1 = ProfileType::CreateMegaType();
    ProfileType profileType2 = ProfileType::CreateMegaType();
    PGOObjectInfo objectInfo(profileType1, profileType2, profileType2,
                            profileType2, profileType2, profileType2,
                            PGOSampleType());
    methodTypeSet.AddObjectInfo(400, objectInfo);
    ProfileType profileType3 = ProfileType::CreateMegaType();
    PGODefineOpType defineOpType(profileType3);
    methodTypeSet.AddDefine(500, defineOpType);
    ProfileType::VariantVector typeArray;
    methodTypeSet.ProcessToJson(typeArray);
    ASSERT_FALSE(typeArray.empty());
    ASSERT_EQ(typeArray.size(), 3U);
    ASSERT_TRUE(std::holds_alternative<ProfileType::StringMap>(typeArray[0]));
    ASSERT_TRUE(std::holds_alternative<ProfileType::MapVector>(typeArray[1]));
    ASSERT_TRUE(
        std::holds_alternative<std::vector<ProfileType::StringMap>>(typeArray[2]));
}
}  // namespace panda::test
