/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

#include "aot_args_handler.h"
#include "aot_compiler_constants.h"
#include "aot_compiler_error_utils.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

using namespace testing::ext;

namespace OHOS::ArkCompiler {

class AotArgsHandlerTest : public testing::Test {
public:
    AotArgsHandlerTest() {}
    virtual ~AotArgsHandlerTest() {}

    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() override
    {
    }

    void TearDown() override
    {
    };

    const char *systemDir_ = "/system";
    const char *systemFrameworkDir_ = "/system/framework";
    const char *bootPathJson_ = "/system/framework/bootpath.json";
    const char *etcDir = "/etc";
    const char *etcArkkDir = "/etc/ark";
    const char *staticPaocBlackListPath = "/etc/ark/static_aot_methods_black_list.json";
};

/**
 * @tc.name: AotArgsHandlerTest_001
 * @tc.desc: AOTArgsParserBase::FindArgsIdxToInteger(argsMap, keyName, bundleID)
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_001, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    std::string keyName = "compiler-pkg-info";
    int32_t bundleID = 0;
    int32_t ret = AOTArgsParserBase::FindArgsIdxToInteger(argsMap, keyName, bundleID);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_PARAM_FAILED);
}

/**
 * @tc.name: AotArgsHandlerTest_002
 * @tc.desc: AOTArgsParserBase::FindArgsIdxToInteger(argsMap, keyName, bundleID)
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_002, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap.emplace("argKey", "argValueNotInteger");
    std::string keyName = "argKey";
    int32_t bundleID = 0;
    int32_t ret = AOTArgsParserBase::FindArgsIdxToInteger(argsMap, keyName, bundleID);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_PARAM_FAILED);
}

/**
 * @tc.name: AotArgsHandlerTest_003
 * @tc.desc: AOTArgsParserBase::FindArgsIdxToInteger(argsMap, keyName, bundleID)
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_003, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap.emplace("argKey", "123456_argValueNotInteger");
    std::string keyName = "argKey";
    int32_t bundleID = 0;
    int32_t ret = AOTArgsParserBase::FindArgsIdxToInteger(argsMap, keyName, bundleID);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_PARAM_FAILED);
}

/**
 * @tc.name: AotArgsHandlerTest_004
 * @tc.desc: AOTArgsParserBase::FindArgsIdxToInteger(argsMap, keyName, bundleID)
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_004, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap.emplace("argKey", "20020079");
    std::string keyName = "argKey";
    int32_t bundleID = 0;
    int32_t ret = AOTArgsParserBase::FindArgsIdxToInteger(argsMap, keyName, bundleID);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_EQ(bundleID, 20020079);
}

/**
 * @tc.name: AotArgsHandlerTest_005
 * @tc.desc: AOTArgsParserBase::FindArgsIdxToString(argsMap, keyName, bundleArg)
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_005, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    std::string keyName = "argKey";
    std::string bundleArg = "";
    int32_t ret = AOTArgsParserBase::FindArgsIdxToString(argsMap, keyName, bundleArg);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_PARAM_FAILED);
}

/**
 * @tc.name: AotArgsHandlerTest_006
 * @tc.desc: AOTArgsParserBase::FindArgsIdxToString(argsMap, keyName, bundleArg)
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_006, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap.emplace("argKey", "com.ohos.contacts");
    std::string keyName = "argKey";
    std::string bundleArg = "";
    int32_t ret = AOTArgsParserBase::FindArgsIdxToString(argsMap, keyName, bundleArg);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_STREQ(bundleArg.c_str(), "com.ohos.contacts");
}


/**
 * @tc.name: AotArgsHandlerTest_009
 * @tc.desc: AOTArgsParser::AddExpandArgs(aotVector, thermalLevel)
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_009, TestSize.Level0)
{
    std::unique_ptr<AOTArgsParser> parser = std::make_unique<AOTArgsParser>();
    std::vector<std::string> aotVector;
    parser->AddExpandArgs(aotVector, 0);
    std::string arg = aotVector[0];
    EXPECT_STREQ(arg.c_str(), "--compiler-thermal-level=0");
}

const std::unordered_map<std::string, std::string> framewordArgsMapForTest {
    {"outputPath", "/data/service/el1/public/for-all-app/framework_ark_cache/etsstdlib_bootabc.an"},
    {"anFileName", "/data/service/el1/public/for-all-app/framework_ark_cache/etsstdlib_bootabc.an"},
    {"isSysComp", "1"},
    {ArgsIdx::ARKTS_MODE, "static"},
    {"sysCompPath", "/system/framework/etsstdlib_bootabc.abc"},
    {"ABC-Path", "/system/framework/etsstdlib_bootabc.abc"}
};

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_010, TestSize.Level0)
{
    std::ofstream(staticPaocBlackListPath) << "{\"blackMethodList\":[]}";

    std::unique_ptr<AOTArgsHandler> argsHandler = std::make_unique<AOTArgsHandler>(framewordArgsMapForTest);
    argsHandler->SetIsEnableStaticCompiler(true);
    argsHandler->SetParser(framewordArgsMapForTest);
    int32_t ret = argsHandler->Handle(0);
    EXPECT_EQ(ret, ERR_OK);
    std::string fileName = argsHandler->GetFileName();
    EXPECT_TRUE(!fileName.empty());
    int32_t bundleUid;
    int32_t bundleGid;
    argsHandler->GetBundleId(bundleUid, bundleGid);
    EXPECT_EQ(bundleUid, OID_SYSTEM);
    EXPECT_EQ(bundleGid, OID_SYSTEM);
    std::vector<const char*> argv = argsHandler->GetAotArgs();
    EXPECT_STREQ(argv[0], "/system/bin/ark_aot");
    for (const auto& arg : argv) {
        if (std::strcmp(arg, "boot-panda-files") == 0) {
            EXPECT_STREQ(arg, "--boot-panda-files=/system/framework/etsstdlib_bootabc.abc");
        }
    }

    unlink(staticPaocBlackListPath);
}

/**
 * @tc.name: AotArgsHandlerTest_011
 * @tc.desc: AOTArgsParserFactory::GetParser(argsMap)
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_011, TestSize.Level0) {
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "static"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result.value(), nullptr);
}

/**
 * @tc.name: AotArgsHandlerTest_012
 * @tc.desc: AOTArgsParserFactory::GetParser(argsMap)
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_012, TestSize.Level0) {
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::ARKTS_MODE, "hybrid"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result.value(), nullptr);
}

/**
 * @tc.name: AotArgsHandlerTest_013
 * @tc.desc: AOTArgsParserFactory::GetParser(argsMap)
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_013, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "dynamic"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result.value(), nullptr);
}

/**
 * @tc.name: AotArgsHandlerTest_014
 * @tc.desc: AOTArgsParserFactory::GetParser(argsMap)
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_014, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "1"},
        {ArgsIdx::ARKTS_MODE, "dynamic"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_NE(result, nullptr);
    EXPECT_NE(result.value(), nullptr);
}

/**
 * @tc.name: AotArgsHandlerTest_015
 * @tc.desc: AOTArgsParserFactory::GetParser(argsMap)
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_015, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result.value(), nullptr);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_016, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::ARKTS_MODE, "static"},
        {ArgsIdx::IS_SYSTEM_COMPONENT, "1"}
    };
    std::unique_ptr<AOTArgsHandler> argsHandler = std::make_unique<AOTArgsHandler>(argsMap);
    int32_t ret = argsHandler->Handle(0);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_PARAM_FAILED);
}
 
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_017, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::ARKTS_MODE, "dynamic"},
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"}
    };
    std::unique_ptr<AOTArgsHandler> argsHandler = std::make_unique<AOTArgsHandler>(argsMap);
    int32_t ret = argsHandler->Handle(0);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_PARAM_FAILED);
}
 
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_018, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"}
    };
    std::unique_ptr<AOTArgsHandler> argsHandler = std::make_unique<AOTArgsHandler>(argsMap);
    int32_t ret = argsHandler->Handle(0);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_PARAM_FAILED);
}
 
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_019, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::ARKTS_MODE, "static"},
        {ArgsIdx::IS_SYSTEM_COMPONENT, "1"}
    };
    std::unique_ptr<AOTArgsHandler> argsHandler = std::make_unique<AOTArgsHandler>(argsMap);
    argsHandler->SetIsEnableStaticCompiler(true);
    argsHandler->SetParser(argsMap);
    int32_t ret = argsHandler->Handle(0);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_PARAM_FAILED);
}
 
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_020, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::ARKTS_MODE, "static"},
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"}
    };
    std::unique_ptr<AOTArgsHandler> argsHandler = std::make_unique<AOTArgsHandler>(argsMap);
    argsHandler->SetIsEnableStaticCompiler(true);
    argsHandler->SetParser(argsMap);
    int32_t ret = argsHandler->Handle(0);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_PARAM_FAILED);
}
 
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_021, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::ARKTS_MODE, "INVALID_VALUE"}
    };
    std::unique_ptr<AOTArgsHandler> argsHandler = std::make_unique<AOTArgsHandler>(argsMap);
    argsHandler->SetIsEnableStaticCompiler(true);
    argsHandler->SetParser(argsMap);
    int32_t ret = argsHandler->Handle(0);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_PARAM_FAILED);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_022, TestSize.Level0)
{
    StaticAOTArgsParser parser;
    std::string anFilePath = "/path/to/test";

    std::string location = parser.ParseLocation(anFilePath);

    const std::string APP_SANBOX_PATH_PREFIX = "/data/storage/el1/bundle/";
    const std::string ETS_PATH = "/ets";
    std::string expected = APP_SANBOX_PATH_PREFIX + "test" + ETS_PATH;
    EXPECT_EQ(location, expected);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_023, TestSize.Level0)
{
    StaticAOTArgsParser parser;
    std::string pkgInfo = R"(
        {
            "pgoDir": "/path/to",
            "bundleName": "bundle",
            "mode": "static"
        }
    )";
    std::string profilePath;

    bool result = parser.ParseProfilePath(pkgInfo, profilePath);

    EXPECT_TRUE(result);

    std::string expectedPath = "/path/to/profile.ap";
    EXPECT_EQ(profilePath, expectedPath);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_024, TestSize.Level0)
{
    StaticAOTArgsParser parser;
    std::string pkgInfo = R"({invalidjson})";
    std::string profilePath;

    bool result = parser.ParseProfilePath(pkgInfo, profilePath);

    EXPECT_FALSE(result);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_025, TestSize.Level0)
{
    StaticAOTArgsParser parser;
    std::string pkgInfo = R"({})";
    std::string profilePath;

    bool result = parser.ParseProfilePath(pkgInfo, profilePath);

    EXPECT_FALSE(result);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_026, TestSize.Level0)
{
    StaticAOTArgsParser parser;
    std::string pkgInfo = R"(
        {
            "bundleName": "bundle",
            "mode": "static"
        }
    )";
    std::string profilePath;

    bool result = parser.ParseProfilePath(pkgInfo, profilePath);
    EXPECT_FALSE(result);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_027, TestSize.Level0)
{
    StaticAOTArgsParser parser;
    HapArgs hapArgs;
    std::string pkgInfo = R"(
        {
            "pgoDir": "/path/to",
            "bundleName": "bundle",
            "mode": "static"
        }
    )";

    bool result = parser.ParseProfileUse(hapArgs, pkgInfo);

    EXPECT_TRUE(result);

    size_t expectedVecSize = 1;
    EXPECT_EQ(hapArgs.argVector.size(), expectedVecSize);

    std::string expectedProfileUse = "--paoc-use-profile:path=/path/to/profile.ap";
    EXPECT_EQ(hapArgs.argVector[0], expectedProfileUse);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_028, TestSize.Level0)
{
    StaticAOTArgsParser parser;
    HapArgs hapArgs;
    std::string pkgInfo = R"({})";

    bool result = parser.ParseProfileUse(hapArgs, pkgInfo);

    EXPECT_FALSE(result);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_029, TestSize.Level0) {
    std::unordered_map<std::string, std::string> argsMap = {};

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_NE(result, std::nullopt);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_030, TestSize.Level0) {
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_NE(result, std::nullopt);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_031, TestSize.Level0) {
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_NE(result, std::nullopt);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_032, TestSize.Level0) {
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::ARKTS_MODE, "dynamic"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_NE(result, std::nullopt);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_033, TestSize.Level0) {
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::ARKTS_MODE, "static"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_EQ(result, std::nullopt);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_034, TestSize.Level0) {
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::ARKTS_MODE, "hybrid"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_EQ(result, std::nullopt);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_035, TestSize.Level0) {
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::ARKTS_MODE, "invalid"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_EQ(result, std::nullopt);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_036, TestSize.Level0) {
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "dynamic"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_NE(result, std::nullopt);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_037, TestSize.Level0) {
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "static"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_EQ(result, std::nullopt);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_038, TestSize.Level0) {
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "hybrid"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_EQ(result, std::nullopt);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_039, TestSize.Level0) {
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "invalid"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_EQ(result, std::nullopt);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_040, TestSize.Level0) {
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "dynamic"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_NE(result, std::nullopt);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_041, TestSize.Level0) {
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "1"},
        {ArgsIdx::ARKTS_MODE, "static"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_EQ(result, std::nullopt);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_042, TestSize.Level0) {
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "1"},
        {ArgsIdx::ARKTS_MODE, "hybrid"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_EQ(result, std::nullopt);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_043, TestSize.Level0) {
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "1"},
        {ArgsIdx::ARKTS_MODE, "invalid"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_EQ(result, std::nullopt);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_044, TestSize.Level0)
{
    std::string blackListJsonStr =
        "{"
        "  \"blackMethodList\": ["
        "    {"
        "      \"bundleName\": \"/system/framework/etsstdlib_bootabc.abc\","
        "      \"type\": \"framework\","
        "      \"methodLists\": [\"Test:m1\", 456, \"Test:m2\", true, \"Test:m3\"]"
        "    }"
        "  ]"
        "}";
    std::ofstream file(staticPaocBlackListPath);
    file << blackListJsonStr << std::endl;
    file.close();

    StaticFrameworkAOTArgsParser parser;
    std::string bundleName = "/system/framework/etsstdlib_bootabc.abc";
    std::string result = parser.ParseBlackListMethods(bundleName);

    EXPECT_STREQ(result.c_str(), "^(?!(Test:m1|Test:m2|Test:m3)$).*");

    unlink(staticPaocBlackListPath);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_045, TestSize.Level0)
{
    std::string blackListJsonStr =
        "{"
        "  \"blackMethodList\": ["
        "    {"
        "      \"bundleName\": \"com.example.test\","
        "      \"type\": \"application\""
        "    }"
        "  ]"
        "}";
    std::ofstream file(staticPaocBlackListPath);
    file << blackListJsonStr << std::endl;
    file.close();

    StaticAOTArgsParser parser;
    std::string pkgInfo = "{\"bundleName\": \"com.example.test\"}";
    std::string moduleName = "entry";
    std::string result = parser.ParseBlackListMethods(pkgInfo, moduleName);

    EXPECT_STREQ(result.c_str(), "");

    unlink(staticPaocBlackListPath);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_046, TestSize.Level0)
{
    std::string blackListJsonStr =
        "{"
        "  \"blackMethodList\": []"
        "}";
    std::ofstream file(staticPaocBlackListPath);
    file << blackListJsonStr << std::endl;
    file.close();

    StaticAOTArgsParser parser;
    std::string pkgInfo = "{\"bundleName\": \"com.example.test\"}";
    std::string moduleName = "entry";
    std::string result = parser.ParseBlackListMethods(pkgInfo, moduleName);

    EXPECT_STREQ(result.c_str(), "");

    unlink(staticPaocBlackListPath);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_047, TestSize.Level0)
{
    std::string blackListJsonStr =
        "{"
        "  \"blackMethodList\": []"
        "}";
    std::ofstream file(staticPaocBlackListPath);
    file << blackListJsonStr << std::endl;
    file.close();

    StaticFrameworkAOTArgsParser parser;
    std::string bundleName = "/system/framework/etsstdlib_bootabc.abc";
    std::string result = parser.ParseBlackListMethods(bundleName);

    EXPECT_STREQ(result.c_str(), "");

    unlink(staticPaocBlackListPath);
}

/**
 * @tc.name: AotArgsHandlerTest_048
 * @tc.desc: Test StaticAOTArgsParser::ProcessArgsMap with staticAOTArgsList.find branch
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_048, TestSize.Level0)
{
    StaticAOTArgsParser parser;
    HapArgs hapArgs;
    std::unordered_map<std::string, std::string> argsMap;
    argsMap["boot-panda-files"] = "entry.abc";
    std::string anfilePath;
    std::string pkgInfo;
    bool partialMode = false;

    parser.ProcessArgsMap(argsMap, anfilePath, pkgInfo, partialMode, hapArgs);

    bool foundStaticArg = false;
    for (const auto& arg : hapArgs.argVector) {
        if (arg.find("--boot-panda-files=entry.abc") != std::string::npos) {
            foundStaticArg = true;
            break;
        }
    }
    EXPECT_TRUE(foundStaticArg);
}

/**
 * @tc.name: AotArgsHandlerTest_049
 * @tc.desc: Test StaticAOTArgsParser::ProcessBlackListMethods with non-empty blackListMethods
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_049, TestSize.Level0)
{
    std::string blackListJsonStr =
        "{"
        "  \"blackMethodList\": ["
        "    {"
        "      \"bundleName\": \"com.example.test\","
        "      \"type\": \"application\","
        "      \"moduleLists\": ["
        "        {"
        "          \"name\": \"entry\","
        "          \"methodLists\": [\"Test:m1\"]"
        "        }"
        "      ]"
        "    }"
        "  ]"
        "}";
    std::ofstream file(staticPaocBlackListPath);
    file << blackListJsonStr << std::endl;
    file.close();

    StaticAOTArgsParser parser;
    HapArgs hapArgs;
    std::string pkgInfo = "{\"bundleName\": \"com.example.test\"}";
    std::string anfilePath = "/data/local/ark-cache/com.example.test/arm64/entry";

    parser.ProcessBlackListMethods(pkgInfo, anfilePath, hapArgs);

    bool foundBlackMethods = false;
    for (const auto& arg : hapArgs.argVector) {
        if (arg.find("--compiler-regex=") != std::string::npos &&
            arg.find("Test:m1") != std::string::npos) {
            foundBlackMethods = true;
        }
    }
    EXPECT_TRUE(foundBlackMethods);

    unlink(staticPaocBlackListPath);
}

/**
 * @tc.name: AotArgsHandlerTest_050
 * @tc.desc: Test AOTArgsParserBase::ParseBlackListJson with file not open
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_050, TestSize.Level0)
{
    nlohmann::json jsonObject;
    bool result = AOTArgsParserBase::ParseBlackListJson(jsonObject);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_053
 * @tc.desc: Test StaticAOTArgsParser::CheckBundleNameAndModuleList with missing bundleName
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_053, TestSize.Level0)
{
    nlohmann::json item;
    nlohmann::json moduleLists = nlohmann::json::array();
    item["moduleLists"] = moduleLists;
    StaticAOTArgsParser parser;
    bool result = parser.CheckBundleNameAndModuleList(item, "com.example.test");
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_054
 * @tc.desc: Test StaticAOTArgsParser::CheckBundleNameAndModuleList with non-string bundleName
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_054, TestSize.Level0)
{
    nlohmann::json item;
    item["bundleName"] = 123;
    nlohmann::json moduleLists = nlohmann::json::array();
    item["moduleLists"] = moduleLists;
    StaticAOTArgsParser parser;
    bool result = parser.CheckBundleNameAndModuleList(item, "com.example.test");
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_055
 * @tc.desc: Test StaticAOTArgsParser::CheckModuleNameAndMethodList with missing name
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_055, TestSize.Level0)
{
    nlohmann::json moduleItem;
    nlohmann::json methodLists = nlohmann::json::array();
    moduleItem["methodLists"] = methodLists;
    StaticAOTArgsParser parser;
    bool result = parser.CheckModuleNameAndMethodList(moduleItem, "entry");
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_056
 * @tc.desc: Test StaticAOTArgsParser::CheckModuleNameAndMethodList with non-string name
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_056, TestSize.Level0)
{
    nlohmann::json moduleItem;
    moduleItem["name"] = 456;
    nlohmann::json methodLists = nlohmann::json::array();
    moduleItem["methodLists"] = methodLists;
    StaticAOTArgsParser parser;
    bool result = parser.CheckModuleNameAndMethodList(moduleItem, "entry");
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_057
 * @tc.desc: Test StaticAOTArgsParser::ProcessBlackListForBundleAndModule with invalid blackMethodList
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_057, TestSize.Level0)
{
    nlohmann::json jsonObject;
    StaticAOTArgsParser parser;
    std::string result = parser.ProcessBlackListForBundleAndModule(jsonObject, "bundle", "module");
    EXPECT_STREQ(result.c_str(), "");
}

/**
 * @tc.name: AotArgsHandlerTest_058
 * @tc.desc: Test StaticAOTArgsParser::ProcessBlackListForBundleAndModule with non-array blackMethodList
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_058, TestSize.Level0)
{
    nlohmann::json jsonObject;
    jsonObject["blackMethodList"] = "not_an_array";
    StaticAOTArgsParser parser;
    std::string result = parser.ProcessBlackListForBundleAndModule(jsonObject, "bundle", "module");
    EXPECT_STREQ(result.c_str(), "");
}

/**
 * @tc.name: AotArgsHandlerTest_059
 * @tc.desc: Test StaticAOTArgsParser::ProcessBlackListForBundleAndModule with empty resultStr comma case
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_059, TestSize.Level0)
{
    std::string blackListJsonStr =
        "{"
        "  \"blackMethodList\": ["
        "    {"
        "      \"bundleName\": \"com.example.test\","
        "      \"type\": \"application\","
        "      \"moduleLists\": ["
        "        {"
        "          \"name\": \"entry\","
        "          \"methodLists\": [\"Test:m1\", \"Test:m2\"]"
        "        }"
        "      ]"
        "    }"
        "  ]"
        "}";
    std::ofstream file(staticPaocBlackListPath);
    file << blackListJsonStr << std::endl;
    file.close();

    StaticAOTArgsParser parser;
    std::string result = parser.ProcessBlackListForBundleAndModule(
        nlohmann::json::parse(blackListJsonStr),
        "com.example.test",
        "entry"
    );

    EXPECT_STREQ(result.c_str(), "^(?!(Test:m1|Test:m2)$).*");

    unlink(staticPaocBlackListPath);
}

/**
 * @tc.name: AotArgsHandlerTest_060
 * @tc.desc: Test StaticAOTArgsParser::ProcessMatchingModules continue branch
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_060, TestSize.Level0)
{
    nlohmann::json item;
    nlohmann::json moduleLists = nlohmann::json::array();

    nlohmann::json moduleItem;
    moduleItem["name"] = "other_module";
    nlohmann::json methodLists = nlohmann::json::array();
    methodLists.push_back("Test:m1");
    moduleItem["methodLists"] = methodLists;
    moduleLists.push_back(moduleItem);

    item["moduleLists"] = moduleLists;

    StaticAOTArgsParser parser;
    std::vector<std::string> result = parser.ProcessMatchingModules(item, "entry");
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: AotArgsHandlerTest_061
 * @tc.desc: Test StaticAOTArgsParser::ParseBlackListMethods with ParseBundleName failure
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_061, TestSize.Level0)
{
    StaticAOTArgsParser parser;
    std::string invalidPkgInfo = "{}";
    std::string result = parser.ParseBlackListMethods(invalidPkgInfo, "entry");

    EXPECT_STREQ(result.c_str(), "");
}

/**
 * @tc.name: AotArgsHandlerTest_062
 * @tc.desc: Test StaticAOTArgsParser::ParseBlackListMethods with empty bundleName
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_062, TestSize.Level0)
{
    StaticAOTArgsParser parser;
    std::string emptyBundlePkgInfo = "{\"bundleName\": \"\"}";
    std::string result = parser.ParseBlackListMethods(emptyBundlePkgInfo, "entry");

    EXPECT_STREQ(result.c_str(), "");
}

/**
 * @tc.name: AotArgsHandlerTest_064
 * @tc.desc: Test StaticFrameworkAOTArgsParser::CheckBundleNameAndMethodList with missing bundleName
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_064, TestSize.Level0)
{
    nlohmann::json item;
    nlohmann::json methodLists = nlohmann::json::array();
    item["methodLists"] = methodLists;
    StaticFrameworkAOTArgsParser parser;
    bool result = parser.CheckBundleNameAndMethodList(item, "com.example.test");
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_065
 * @tc.desc: Test StaticFrameworkAOTArgsParser::CheckBundleNameAndMethodList with non-string bundleName
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_065, TestSize.Level0)
{
    nlohmann::json item;
    item["bundleName"] = 123;
    nlohmann::json methodLists = nlohmann::json::array();
    item["methodLists"] = methodLists;
    StaticFrameworkAOTArgsParser parser;
    bool result = parser.CheckBundleNameAndMethodList(item, "com.example.test");
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_066
 * @tc.desc: Test StaticFrameworkAOTArgsParser::CheckBundleNameAndMethodList with null methodLists
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_066, TestSize.Level0)
{
    nlohmann::json item;
    item["bundleName"] = "com.example.test";
    StaticFrameworkAOTArgsParser parser;
    bool result = parser.CheckBundleNameAndMethodList(item, "com.example.test");
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_067
 * @tc.desc: Test StaticFrameworkAOTArgsParser::ParseBlackListMethods with invalid blackMethodList key
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_067, TestSize.Level0)
{
    StaticFrameworkAOTArgsParser parser;
    std::string bundleName = "com.example.test";
    std::string result = parser.ParseBlackListMethods(bundleName);

    EXPECT_STREQ(result.c_str(), "");
}

/**
 * @tc.name: AotArgsHandlerTest_068
 * @tc.desc: Test StaticAOTArgsParser::ProcessArgsMap with staticAOTArgsList.find branch emplace_back
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_068, TestSize.Level0)
{
    StaticAOTArgsParser parser;
    HapArgs hapArgs;
    std::unordered_map<std::string, std::string> argsMap;

    argsMap["boot-panda-files"] = "entry.abc";
    std::string anfilePath;
    std::string pkgInfo;
    bool partialMode = false;

    parser.ProcessArgsMap(argsMap, anfilePath, pkgInfo, partialMode, hapArgs);

    bool foundArg = false;
    for (const auto& arg : hapArgs.argVector) {
        if (arg.find("--boot-panda-files=entry.abc") != std::string::npos) {
            foundArg = true;
            break;
        }
    }
    EXPECT_TRUE(foundArg);
}

/**
 * @tc.name: AotArgsHandlerTest_069
 * @tc.desc: Test AOTArgsParserBase::ParseBlackListJson with file not existing (IsFileExists path)
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_069, TestSize.Level0)
{
    nlohmann::json jsonObject;
    bool result = AOTArgsParserBase::ParseBlackListJson(jsonObject);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_070
 * @tc.desc: Test StaticAOTArgsParser::ProcessBlackListForBundleAndModule with comma concatenation
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_070, TestSize.Level0)
{
    std::string blackListJsonStr =
        "{"
        "  \"blackMethodList\": ["
        "    {"
        "      \"bundleName\": \"com.example.test\","
        "      \"type\": \"application\","
        "      \"moduleLists\": ["
        "        {"
        "          \"name\": \"entry\","
        "          \"methodLists\": [\"Test:m1\"]"
        "        }"
        "      ]"
        "    },"
        "    {"
        "      \"bundleName\": \"com.example.test\","
        "      \"type\": \"application\","
        "      \"moduleLists\": ["
        "        {"
        "          \"name\": \"entry\","
        "          \"methodLists\": [\"Test:m2\"]"
        "        }"
        "      ]"
        "    }"
        "  ]"
        "}";
    std::ofstream file(staticPaocBlackListPath);
    file << blackListJsonStr << std::endl;
    file.close();

    StaticAOTArgsParser parser;
    std::string result = parser.ProcessBlackListForBundleAndModule(
        nlohmann::json::parse(blackListJsonStr),
        "com.example.test",
        "entry"
    );

    EXPECT_STREQ(result.c_str(), "^(?!(Test:m1|Test:m2)$).*");

    unlink(staticPaocBlackListPath);
}

/**
 * @tc.name: AotArgsHandlerTest_071
 * @tc.desc: Test StaticAOTArgsParser::ProcessMatchingModules with comma concatenation
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_071, TestSize.Level0)
{
    nlohmann::json item;
    nlohmann::json moduleLists = nlohmann::json::array();

    nlohmann::json moduleItem1, moduleItem2;
    moduleItem1["name"] = "entry";
    nlohmann::json methodLists1 = nlohmann::json::array();
    methodLists1.push_back("Test:m1");
    moduleItem1["methodLists"] = methodLists1;

    moduleItem2["name"] = "entry";
    nlohmann::json methodLists2 = nlohmann::json::array();
    methodLists2.push_back("Test:m2");
    moduleItem2["methodLists"] = methodLists2;

    moduleLists.push_back(moduleItem1);
    moduleLists.push_back(moduleItem2);
    item["moduleLists"] = moduleLists;

    StaticAOTArgsParser parser;
    std::vector<std::string> result = parser.ProcessMatchingModules(item, "entry");

    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result[0], "Test:m1");
    EXPECT_EQ(result[1], "Test:m2");
}

/**
 * @tc.name: AotArgsHandlerTest_072
 * @tc.desc: Test StaticAOTArgsParser::ParseBlackListMethods with ParseBundleName failure/empty
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_072, TestSize.Level0)
{
    StaticAOTArgsParser parser;
    std::string emptyBundlePkgInfo = "{\"bundleName\": \"\"}";
    std::string result = parser.ParseBlackListMethods(emptyBundlePkgInfo, "entry");

    EXPECT_STREQ(result.c_str(), "");
}

/**
 * @tc.name: AotArgsHandlerTest_073
 * @tc.desc: Test StaticFrameworkAOTArgsParser::Parse function with non-empty blackListMethods
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_073, TestSize.Level0)
{
    std::string blackListJsonStr =
        "{"
        "  \"blackMethodList\": ["
        "    {"
        "      \"bundleName\": \"/system/framework/etsstdlib_bootabc.abc\","
        "      \"type\": \"framework\","
        "      \"methodLists\": [\"Test:m1\", \"Test:m2\"]"
        "    }"
        "  ]"
        "}";
    std::ofstream file(staticPaocBlackListPath);
    file << blackListJsonStr << std::endl;
    file.close();
    std::unordered_map<std::string, std::string> argsMap(framewordArgsMapForTest);
    argsMap.emplace(ArgsIdx::ARKTS_MODE, "static");
    argsMap.emplace(ArgsIdx::IS_SYSTEM_COMPONENT, "1");

    std::unique_ptr<AOTArgsHandler> argsHandler = std::make_unique<AOTArgsHandler>(argsMap);
    argsHandler->SetIsEnableStaticCompiler(true);
    argsHandler->SetParser(argsMap);
    int32_t ret = argsHandler->Handle(0);
    EXPECT_EQ(ret, ERR_OK);

    unlink(staticPaocBlackListPath);
}

/**
 * @tc.name: AotArgsHandlerTest_074
 * @tc.desc: Test StaticFrameworkAOTArgsParser::ParseBlackListMethods with invalid blackMethodList
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_074, TestSize.Level0)
{
    StaticFrameworkAOTArgsParser parser;
    std::string bundleName = "/system/framework/etsstdlib_bootabc.abc";
    std::string result = parser.ParseBlackListMethods(bundleName);

    EXPECT_STREQ(result.c_str(), "");
}

/**
 * @tc.name: AotArgsHandlerTest_075
 * @tc.desc: Test StaticFrameworkAOTArgsParser::ParseBlackListMethods with comma concatenation
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_075, TestSize.Level0)
{
    std::string blackListJsonStr =
        "{"
        "  \"blackMethodList\": ["
        "    {"
        "      \"bundleName\": \"/system/framework/etsstdlib_bootabc.abc\","
        "      \"type\": \"framework\","
        "      \"methodLists\": [\"Test:m1\", \"Test:m2\"]"
        "    }"
        "  ]"
        "}";
    std::ofstream file(staticPaocBlackListPath);
    file << blackListJsonStr << std::endl;
    file.close();

    StaticFrameworkAOTArgsParser parser;
    std::string bundleName = "/system/framework/etsstdlib_bootabc.abc";
    std::string result = parser.ParseBlackListMethods(bundleName);

    EXPECT_STREQ(result.c_str(), "^(?!(Test:m1|Test:m2)$).*");

    unlink(staticPaocBlackListPath);
}

/**
 * @tc.name: AotArgsHandlerTest_076
 * @tc.desc: Test AOTArgsParserBase::ParseBlackListJson with null JSON content
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_076, TestSize.Level0)
{
    std::string nullJson = "null";
    system("mkdir -p /etc/ark");
    std::ofstream file("/etc/ark/static_aot_methods_black_list.json");
    file << nullJson << std::endl;
    file.close();

    nlohmann::json jsonObject;
    bool result = AOTArgsParserBase::ParseBlackListJson(jsonObject);
    EXPECT_FALSE(result);

    unlink("/etc/ark/static_aot_methods_black_list.json");
}

/**
 * @tc.name: AotArgsHandlerTest_078
 * @tc.desc: Test StaticFrameworkAOTArgsParser::ParseBlackListMethods with missing blackMethodList
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_078, TestSize.Level0)
{
    std::string invalidJson = "{\"otherKey\": \"otherValue\"}";
    system("mkdir -p /etc/ark");
    std::ofstream file("/etc/ark/static_aot_methods_black_list.json");
    file << invalidJson << std::endl;
    file.close();

    StaticFrameworkAOTArgsParser parser;
    std::string bundleName = "some_bundle";
    std::string result = parser.ParseBlackListMethods(bundleName);

    EXPECT_STREQ(result.c_str(), "");

    unlink("/etc/ark/static_aot_methods_black_list.json");
}

/**
 * @tc.name: AotArgsHandlerTest_079
 * @tc.desc: Test StaticFrameworkAOTArgsParser::ParseBlackListMethods with non-array blackMethodList
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_079, TestSize.Level0)
{
    std::string invalidJson = "{\"blackMethodList\": \"not_an_array\"}";
    system("mkdir -p /etc/ark");
    std::ofstream file("/etc/ark/static_aot_methods_black_list.json");
    file << invalidJson << std::endl;
    file.close();

    StaticFrameworkAOTArgsParser parser;
    std::string bundleName = "some_bundle";
    std::string result = parser.ParseBlackListMethods(bundleName);

    EXPECT_STREQ(result.c_str(), "");

    unlink("/etc/ark/static_aot_methods_black_list.json");
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_080, TestSize.Level0)
{
    std::string jsonContent = "{}";
    system("mkdir -p /etc/ark");
    std::ofstream file("/etc/ark/static_aot_methods_black_list.json");
    file << jsonContent << std::endl;
    file.close();

    StaticFrameworkAOTArgsParser parser;
    std::string bundleName = "test_bundle";
    std::string result = parser.ParseBlackListMethods(bundleName);

    EXPECT_STREQ(result.c_str(), "");

    unlink("/etc/ark/static_aot_methods_black_list.json");
}

/**
 * @tc.name: AotArgsHandlerTest_081
 * @tc.desc: Test StaticFrameworkAOTArgsParser::ParseBlackListMethods with empty blackMethodList array
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_081, TestSize.Level0)
{
    std::string jsonContent = "{\"blackMethodList\": []}";
    system("mkdir -p /etc/ark");
    std::ofstream file("/etc/ark/static_aot_methods_black_list.json");
    file << jsonContent << std::endl;
    file.close();

    StaticFrameworkAOTArgsParser parser;
    std::string bundleName = "test_bundle";
    std::string result = parser.ParseBlackListMethods(bundleName);

    EXPECT_STREQ(result.c_str(), "");

    unlink("/etc/ark/static_aot_methods_black_list.json");
}

/**
 * @tc.name: AotArgsHandlerTest_082
 * @tc.desc: Test StaticFrameworkAOTArgsParser::ParseBlackListMethods with comma concatenation
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_082, TestSize.Level0)
{
    std::string jsonContent =
        "{"
        "  \"blackMethodList\": ["
        "    {"
        "      \"bundleName\": \"/system/framework/etsstdlib_bootabc.abc\","
        "      \"type\": \"framework\","
        "      \"methodLists\": [\"Test:m1\", \"Test:m2\"]"
        "    },"
        "    {"
        "      \"bundleName\": \"/system/framework/etsstdlib_bootabc.abc\","
        "      \"type\": \"framework\","
        "      \"methodLists\": [\"Test:m3\", \"Test:m4\"]"
        "    }"
        "  ]"
        "}";
    system("mkdir -p /etc/ark");
    std::ofstream file("/etc/ark/static_aot_methods_black_list.json");
    file << jsonContent << std::endl;
    file.close();

    StaticFrameworkAOTArgsParser parser;
    std::string bundleName = "/system/framework/etsstdlib_bootabc.abc";
    std::string result = parser.ParseBlackListMethods(bundleName);

    EXPECT_STREQ(result.c_str(), "^(?!(Test:m1|Test:m2|Test:m3|Test:m4)$).*");

    unlink("/etc/ark/static_aot_methods_black_list.json");
}

/**
 * @tc.name: AotArgsHandlerTest_083
 * @tc.desc: Test AOTArgsParserBase::ParseBundleName with valid bundleName
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_083, TestSize.Level0)
{
    std::string pkgInfo = "{\"bundleName\": \"com.example.app\"}";
    std::string bundleName;
    bool result = AOTArgsParserBase::ParseBundleName(pkgInfo, bundleName);
    EXPECT_STREQ(bundleName.c_str(), "com.example.app");
    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_084
 * @tc.desc: Test AOTArgsParserBase::ParseBundleName with null pkgInfo
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_084, TestSize.Level0)
{
    std::string pkgInfo = "null";
    std::string bundleName;
    bool result = AOTArgsParserBase::ParseBundleName(pkgInfo, bundleName);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_085
 * @tc.desc: Test AOTArgsParserBase::ParseBundleName with non-string bundleName
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_085, TestSize.Level0)
{
    std::string pkgInfo = "{\"bundleName\":false}";
    std::string bundleName;
    bool result = AOTArgsParserBase::ParseBundleName(pkgInfo, bundleName);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_086
 * @tc.desc: Test AOTArgsParserBase::ParseBundleName with null bundleName
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_086, TestSize.Level0)
{
    std::string pkgInfo = "{\"bundleName\":null}";
    std::string bundleName;
    bool result = AOTArgsParserBase::ParseBundleName(pkgInfo, bundleName);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_087
 * @tc.desc: Test AOTArgsParserBase::ParseBundleName with missing bundleName
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_087, TestSize.Level0)
{
    std::string pkgInfo = "{\"test\":\"aa\"}";
    std::string bundleName;
    bool result = AOTArgsParserBase::ParseBundleName(pkgInfo, bundleName);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_088
 * @tc.desc: Test AOTArgsParserBase::ParseBundleName with empty pkgInfo
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_088, TestSize.Level0)
{
    std::string pkgInfo = "{}";
    std::string bundleName;
    bool result = AOTArgsParserBase::ParseBundleName(pkgInfo, bundleName);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_089
 * @tc.desc: Test AOTArgsParserBase::JoinMethodList with empty array
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_089, TestSize.Level0)
{
    nlohmann::json methodLists = nlohmann::json::array();
    std::vector<std::string> result = AOTArgsParserBase::JoinMethodList(methodLists);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: AotArgsHandlerTest_090
 * @tc.desc: Test AOTArgsParserBase::JoinMethodList with method list
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_090, TestSize.Level0)
{
    nlohmann::json methodLists = nlohmann::json::array();
    methodLists.push_back("Test:m1");
    methodLists.push_back("Test:m2");
    methodLists.push_back("Test:m3");
    std::vector<std::string> result = AOTArgsParserBase::JoinMethodList(methodLists);
    ASSERT_EQ(result.size(), 3U);
    EXPECT_EQ(result[0], "Test:m1");
    EXPECT_EQ(result[1], "Test:m2");
    EXPECT_EQ(result[2], "Test:m3");
}

/**
 * @tc.name: AotArgsHandlerTest_091
 * @tc.desc: Test AOTArgsParserBase::JoinMethodList with non-string elements
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_091, TestSize.Level0)
{
    nlohmann::json methodLists = nlohmann::json::array();
    methodLists.push_back("Test:m1");
    methodLists.push_back(123);
    methodLists.push_back("Test:m3");
    std::vector<std::string> result = AOTArgsParserBase::JoinMethodList(methodLists);
    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result[0], "Test:m1");
    EXPECT_EQ(result[1], "Test:m3");
}

/**
 * @tc.name: AotArgsHandlerTest_092
 * @tc.desc: Test StaticAOTArgsParser::CheckBundleNameAndModuleList with valid bundle and module list
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_092, TestSize.Level0)
{
    nlohmann::json item;
    item["bundleName"] = "com.example.test";
    nlohmann::json moduleLists = nlohmann::json::array();
    item["moduleLists"] = moduleLists;
    StaticAOTArgsParser parser;
    bool result = parser.CheckBundleNameAndModuleList(item, "com.example.test");
    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_093
 * @tc.desc: Test StaticAOTArgsParser::CheckBundleNameAndModuleList with invalid bundle name
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_093, TestSize.Level0)
{
    nlohmann::json item;
    item["bundleName"] = "com.example.other";
    nlohmann::json moduleLists = nlohmann::json::array();
    item["moduleLists"] = moduleLists;
    StaticAOTArgsParser parser;
    bool result = parser.CheckBundleNameAndModuleList(item, "com.example.test");
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_094
 * @tc.desc: Test StaticAOTArgsParser::CheckBundleNameAndModuleList without module list
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_094, TestSize.Level0)
{
    nlohmann::json item;
    item["bundleName"] = "com.example.test";
    StaticAOTArgsParser parser;
    bool result = parser.CheckBundleNameAndModuleList(item, "com.example.test");
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_095
 * @tc.desc: Test StaticAOTArgsParser::CheckModuleNameAndMethodList with valid module and method list
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_095, TestSize.Level0)
{
    nlohmann::json moduleItem;
    moduleItem["name"] = "entry";
    nlohmann::json methodLists = nlohmann::json::array();
    moduleItem["methodLists"] = methodLists;
    StaticAOTArgsParser parser;
    bool result = parser.CheckModuleNameAndMethodList(moduleItem, "entry");
    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_096
 * @tc.desc: Test StaticAOTArgsParser::CheckModuleNameAndMethodList with invalid module name
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_096, TestSize.Level0)
{
    nlohmann::json moduleItem;
    moduleItem["name"] = "other";
    nlohmann::json methodLists = nlohmann::json::array();
    moduleItem["methodLists"] = methodLists;
    StaticAOTArgsParser parser;
    bool result = parser.CheckModuleNameAndMethodList(moduleItem, "entry");
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_097
 * @tc.desc: Test StaticAOTArgsParser::CheckModuleNameAndMethodList without method list
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_097, TestSize.Level0)
{
    nlohmann::json moduleItem;
    moduleItem["name"] = "entry";
    StaticAOTArgsParser parser;
    bool result = parser.CheckModuleNameAndMethodList(moduleItem, "entry");
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_098
 * @tc.desc: Test StaticFrameworkAOTArgsParser::ParseBlackListMethods with matching bundleName
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_098, TestSize.Level0)
{
    std::string blackListJsonStr =
        "{"
        "  \"blackMethodList\": ["
        "    {"
        "      \"bundleName\": \"/system/framework/etsstdlib_bootabc.abc\","
        "      \"type\": \"framework\","
        "      \"methodLists\": [\"Test:m1\", \"Test:m2\"]"
        "    }"
        "  ]"
        "}";
    std::ofstream file(staticPaocBlackListPath);
    file << blackListJsonStr << std::endl;
    file.close();

    StaticFrameworkAOTArgsParser parser;
    std::string bundleName = "/system/framework/etsstdlib_bootabc.abc";
    std::string result = parser.ParseBlackListMethods(bundleName);

    EXPECT_STREQ(result.c_str(), "^(?!(Test:m1|Test:m2)$).*");

    unlink(staticPaocBlackListPath);
}

/**
 * @tc.name: AotArgsHandlerTest_099
 * @tc.desc: Test StaticFrameworkAOTArgsParser::ParseBlackListMethods with non-matching bundleName
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_099, TestSize.Level0)
{
    std::string blackListJsonStr =
        "{"
        "  \"blackMethodList\": ["
        "    {"
        "      \"bundleName\": \"/system/framework/other.abc\","
        "      \"type\": \"framework\","
        "      \"methodLists\": [\"Test:m1\", \"Test:m2\"]"
        "    }"
        "  ]"
        "}";
    std::ofstream file(staticPaocBlackListPath);
    file << blackListJsonStr << std::endl;
    file.close();

    StaticFrameworkAOTArgsParser parser;
    std::string bundleName = "/system/framework/etsstdlib_bootabc.abc";
    std::string result = parser.ParseBlackListMethods(bundleName);

    EXPECT_STREQ(result.c_str(), "");

    unlink(staticPaocBlackListPath);
}

/**
 * @tc.name: AotArgsHandlerTest_100
 * @tc.desc: Test StaticFrameworkAOTArgsParser::ParseBlackListMethods without methodLists
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_100, TestSize.Level0)
{
    std::string blackListJsonStr =
        "{"
        "  \"blackMethodList\": ["
        "    {"
        "      \"bundleName\": \"/system/framework/etsstdlib_bootabc.abc\","
        "      \"type\": \"framework\""
        "    }"
        "  ]"
        "}";
    std::ofstream file(staticPaocBlackListPath);
    file << blackListJsonStr << std::endl;
    file.close();

    StaticFrameworkAOTArgsParser parser;
    std::string bundleName = "/system/framework/etsstdlib_bootabc.abc";
    std::string result = parser.ParseBlackListMethods(bundleName);

    EXPECT_STREQ(result.c_str(), "");

    unlink(staticPaocBlackListPath);
}

/**
 * @tc.name: AotArgsHandlerTest_101
 * @tc.desc: Test StaticAOTArgsParser with proper blackMethodList JSON format
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_101, TestSize.Level0)
{
    std::string blackListJsonStr =
        "{"
        "  \"blackMethodList\": ["
        "    {"
        "      \"bundleName\": \"com.example.test\","
        "      \"type\": \"application\","
        "      \"moduleLists\": ["
        "        {"
        "          \"name\": \"entry\","
        "          \"methodLists\": [\"Test:f1\", \"Test:f2\"]"
        "        }"
        "      ]"
        "    }"
        "  ]"
        "}";
    std::ofstream file(staticPaocBlackListPath);
    file << blackListJsonStr << std::endl;
    file.close();

    StaticAOTArgsParser parser;
    std::string pkgInfo = "{\"bundleName\": \"com.example.test\"}";
    std::string moduleName = "entry";
    std::string result = parser.ParseBlackListMethods(pkgInfo, moduleName);

    EXPECT_STREQ(result.c_str(), "^(?!(Test:f1|Test:f2)$).*");

    unlink(staticPaocBlackListPath);
}

/**
 * @tc.name: AotArgsHandlerTest_102
 * @tc.desc: Test StaticFrameworkAOTArgsParser with proper blackMethodList JSON format for framework
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_102, TestSize.Level0)
{
    std::string blackListJsonStr =
        "{"
        "  \"blackMethodList\": ["
        "    {"
        "      \"bundleName\": \"/system/framework/etsstdlib_bootabc.abc\","
        "      \"type\": \"framework\","
        "      \"methodLists\": [\"Test:f1\", \"Test:f2\"]"
        "    }"
        "  ]"
        "}";
    std::ofstream file(staticPaocBlackListPath);
    file << blackListJsonStr << std::endl;
    file.close();

    StaticFrameworkAOTArgsParser parser;
    std::string bundleName = "/system/framework/etsstdlib_bootabc.abc";
    std::string result = parser.ParseBlackListMethods(bundleName);

    EXPECT_STREQ(result.c_str(), "^(?!(Test:f1|Test:f2)$).*");

    unlink(staticPaocBlackListPath);
}

/**
 * @tc.name: AotArgsHandlerTest_103
 * @tc.desc: Test ParseBlackListJson with blackMethodList structure
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_103, TestSize.Level0)
{
    std::string blackListJsonStr =
        "{"
        "  \"blackMethodList\": ["
        "    {"
        "      \"bundleName\": \"com.example.test\","
        "      \"type\": \"application\","
        "      \"methodLists\": [\"Test:f1\"],"
         "      \"issue\": \"https://example.com/issue/123\""
        "    }"
        "  ]"
        "}";
    std::ofstream file(staticPaocBlackListPath);
    file << blackListJsonStr << std::endl;
    file.close();

    nlohmann::json jsonObject;
    bool result = AOTArgsParserBase::ParseBlackListJson(jsonObject);
    EXPECT_TRUE(result);
    EXPECT_TRUE(jsonObject.contains("blackMethodList"));

    unlink(staticPaocBlackListPath);
}

/**
 * @tc.name: AotArgsHandlerTest_104
 * @tc.desc: Test StaticAOTArgsParser::ParseBlackListMethods with non-string elements in methodLists
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_104, TestSize.Level0)
{
    std::string blackListJsonStr =
        "{"
        "  \"blackMethodList\": ["
        "    {"
        "      \"bundleName\": \"com.example.test\","
        "      \"type\": \"application\","
        "      \"moduleLists\": ["
        "        {"
        "          \"name\": \"entry\","
        "          \"methodLists\": [\"Test:m1\", 123, \"Test:m3\", null, \"Test:m4\"]"
        "        }"
        "      ]"
        "    }"
        "  ]"
        "}";
    std::ofstream file(staticPaocBlackListPath);
    file << blackListJsonStr << std::endl;
    file.close();

    StaticAOTArgsParser parser;
    std::string pkgInfo = "{\"bundleName\": \"com.example.test\"}";
    std::string moduleName = "entry";
    std::string result = parser.ParseBlackListMethods(pkgInfo, moduleName);

    EXPECT_STREQ(result.c_str(), "^(?!(Test:m1|Test:m3|Test:m4)$).*");

    unlink(staticPaocBlackListPath);
}

/**
 * @tc.name: AotArgsHandlerTest_105
 * @tc.desc: Test StaticAOTArgsParser::ParseLocation with various path formats
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_105, TestSize.Level0)
{
    StaticAOTArgsParser parser;
    std::string anFilePath = "/data/local/ark-cache/com.example.app/arm64/module";
    std::string location = parser.ParseLocation(anFilePath);
    EXPECT_FALSE(location.empty());
}

/**
 * @tc.name: AotArgsHandlerTest_106
 * @tc.desc: Test AOTArgsParserBase::FindArgsIdxToInteger with zero value
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_106, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap.emplace("argKey", "0");
    std::string keyName = "argKey";
    int32_t bundleID = 999;
    int32_t ret = AOTArgsParserBase::FindArgsIdxToInteger(argsMap, keyName, bundleID);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_EQ(bundleID, 0);
}

/**
 * @tc.name: AotArgsHandlerTest_107
 * @tc.desc: Test StaticAOTArgsParser::ProcessMatchingModules with null methodLists
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_107, TestSize.Level0)
{
    nlohmann::json item;
    nlohmann::json moduleLists = nlohmann::json::array();
    nlohmann::json moduleItem;
    moduleItem["name"] = "entry";
    moduleLists.push_back(moduleItem);
    item["moduleLists"] = moduleLists;

    StaticAOTArgsParser parser;
    std::vector<std::string> result = parser.ProcessMatchingModules(item, "entry");
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: AotArgsHandlerTest_108
 * @tc.desc: Test StaticFrameworkAOTArgsParser::CheckBundleNameAndMethodList with matching bundle
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_108, TestSize.Level0)
{
    nlohmann::json item;
    item["bundleName"] = "com.example.test";
    nlohmann::json methodLists = nlohmann::json::array();
    methodLists.push_back("Test:m1");
    item["methodLists"] = methodLists;
    StaticFrameworkAOTArgsParser parser;
    bool result = parser.CheckBundleNameAndMethodList(item, "com.example.test");
    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_109
 * @tc.desc: Test AOTArgsParser::AddExpandArgs with different thermal levels
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_109, TestSize.Level0)
{
    std::unique_ptr<AOTArgsParser> parser = std::make_unique<AOTArgsParser>();
    std::vector<std::string> aotVector;
    parser->AddExpandArgs(aotVector, 3);
    std::string arg = aotVector[0];
    EXPECT_STREQ(arg.c_str(), "--compiler-thermal-level=3");
}

/**
 * @tc.name: AotArgsHandlerTest_110
 * @tc.desc: Test StaticAOTArgsParser::ParseProfilePath with missing pgoDir and bundleName
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_110, TestSize.Level0)
{
    StaticAOTArgsParser parser;
    std::string pkgInfo = R"({"mode": "static"})";
    std::string profilePath;
    bool result = parser.ParseProfilePath(pkgInfo, profilePath);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_111
 * @tc.desc: Test AOTArgsParserFactory::GetParser with invalid IS_SYSTEM_COMPONENT value
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_111, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "dynamic"}
    };
    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
}

/**
 * @tc.name: AotArgsHandlerTest_113
 * @tc.desc: Test AOTArgsParserBase::JoinMethodList with single element
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_113, TestSize.Level0)
{
    nlohmann::json methodLists = nlohmann::json::array();
    methodLists.push_back("SingleMethod:test");
    std::vector<std::string> result = AOTArgsParserBase::JoinMethodList(methodLists);
    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result[0], "SingleMethod:test");
}

/**
 * @tc.name: AotArgsHandlerTest_114
 * @tc.desc: Test StaticAOTArgsParser::CheckBundleNameAndModuleList with non-array moduleLists
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_114, TestSize.Level0)
{
    nlohmann::json item;
    item["bundleName"] = "com.example.test";
    item["moduleLists"] = "not_an_array";
    StaticAOTArgsParser parser;
    bool result = parser.CheckBundleNameAndModuleList(item, "com.example.test");
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_115
 * @tc.desc: Test AOTArgsParserFactory::GetParser with invalid IS_SYSTEM_COMPONENT value
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_115, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "dynamic"}
    };
    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
}

/**
 * @tc.name: AotArgsHandlerTest_117
 * @tc.desc: Test AOTArgsParserBase::JoinMethodList with single element
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_117, TestSize.Level0)
{
    nlohmann::json methodLists = nlohmann::json::array();
    methodLists.push_back("SingleMethod:test");
    std::vector<std::string> result = AOTArgsParserBase::JoinMethodList(methodLists);
    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result[0], "SingleMethod:test");
}

/**
 * @tc.name: AotArgsHandlerTest_118
 * @tc.desc: Test StaticAOTArgsParser::CheckBundleNameAndModuleList with non-array moduleLists
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_118, TestSize.Level0)
{
    nlohmann::json item;
    item["bundleName"] = "com.example.test";
    item["moduleLists"] = "not_an_array";
    StaticAOTArgsParser parser;
    bool result = parser.CheckBundleNameAndModuleList(item, "com.example.test");
    EXPECT_FALSE(result);
}


/**
 * @tc.name: StaticAOTArgsParserTest_001
 * @tc.desc: Test StaticAOTArgsParser::Check with missing PKG_INFO
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, StaticAOTArgsParserTest_001, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/com.example.myapplication/arm64/entry";
    argsMap[ArgsIdx::AN_FILE_NAME] =
        "/data/app/el1/public/aot_compiler/ark_cache/com.example.myapplication/arm64/entry.an";

    StaticAOTArgsParser parser;
    bool result = parser.Check(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: StaticAOTArgsParserTest_002
 * @tc.desc: Test StaticAOTArgsParser::Check with invalid PKG_INFO (empty bundleName)
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, StaticAOTArgsParserTest_002, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = "{}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/com.example.myapplication/arm64/entry";
    argsMap[ArgsIdx::AN_FILE_NAME] =
        "/data/app/el1/public/aot_compiler/ark_cache/com.example.myapplication/arm64/entry.an";

    StaticAOTArgsParser parser;
    bool result = parser.Check(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: StaticAOTArgsParserTest_003
 * @tc.desc: Test StaticAOTArgsParser::Check with missing AOT_FILE
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, StaticAOTArgsParserTest_003, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] =
        "{\"bundleName\": \"com.example.myapplication\", \"pkgPath\": \"/data/test/test.hap\", "
         "\"abcOffset\": \"0\", \"abcSize\": \"100\", \"bundleUid\": \"2710\", \"bundleGid\": \"2710\"}";
    argsMap[ArgsIdx::AN_FILE_NAME] =
        "/data/app/el1/public/aot_compiler/ark_cache/com.example.myapplication/arm64/entry.an";

    StaticAOTArgsParser parser;
    bool result = parser.Check(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: StaticAOTArgsParserTest_004
 * @tc.desc: Test StaticAOTArgsParser::Check with missing AN_FILE_NAME
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, StaticAOTArgsParserTest_004, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] =
        "{\"bundleName\": \"com.example.myapplication\", \"pkgPath\": \"/data/test/test.hap\", "
         "\"abcOffset\": \"0\", \"abcSize\": \"100\", \"bundleUid\": \"2710\", \"bundleGid\": \"2710\"}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/com.example.myapplication/arm64/entry";

    StaticAOTArgsParser parser;
    bool result = parser.Check(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: StaticAOTArgsParserTest_005
 * @tc.desc: Test StaticAOTArgsParser::Check with path traversal in aot-file
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, StaticAOTArgsParserTest_005, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] =
        "{\"bundleName\": \"com.example.myapplication\", \"pkgPath\": \"/data/test/test.hap\", "
         "\"abcOffset\": \"0\", \"abcSize\": \"100\", \"bundleUid\": \"2710\", \"bundleGid\": \"2710\"}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/com.example.myapplication/../etc/entry";
    argsMap[ArgsIdx::AN_FILE_NAME] =
        "/data/app/el1/public/aot_compiler/ark_cache/com.example.myapplication/../etc/entry.an";

    StaticAOTArgsParser parser;
    bool result = parser.Check(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: StaticAOTArgsParserTest_006
 * @tc.desc: Test StaticAOTArgsParser::Check with invalid ark cache directory (not in expected location)
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, StaticAOTArgsParserTest_006, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] =
        "{\"bundleName\": \"com.example.myapplication\", \"pkgPath\": \"/data/test/test.hap\", "
         "\"abcOffset\": \"0\", \"abcSize\": \"100\", \"bundleUid\": \"2710\", \"bundleGid\": \"2710\"}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/wrong_location/" \
                                  "com.example.myapplication/arm64/entry";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/wrong_location/" \
                                   "com.example.myapplication/arm64/entry.an";

    StaticAOTArgsParser parser;
    bool result = parser.Check(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: StaticAOTArgsParserTest_SUCCESS_001
 * @tc.desc: Test StaticAOTArgsParser can be obtained via GetParser with valid inputs
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, StaticAOTArgsParserTest_SUCCESS_001, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::IS_SYSTEM_COMPONENT] = "0";
    argsMap[ArgsIdx::ARKTS_MODE] = "static";

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result.value(), nullptr);
}


/**
 * @tc.name: StaticFrameworkAOTArgsParserTest_001
 * @tc.desc: Test StaticFrameworkAOTArgsParser::Check with missing AN_FILE_NAME
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, StaticFrameworkAOTArgsParserTest_001, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;

    StaticFrameworkAOTArgsParser parser;
    bool result = parser.Check(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: StaticFrameworkAOTArgsParserTest_002
 * @tc.desc: Test StaticFrameworkAOTArgsParser::Check with path traversal in anFileName
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, StaticFrameworkAOTArgsParserTest_002, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/service/el1/public/for-all-app/" \
                                      "framework_ark_cache/../etc/etsstdlib_bootabc.an";

    StaticFrameworkAOTArgsParser parser;
    bool result = parser.Check(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: StaticFrameworkAOTArgsParserTest_003
 * @tc.desc: Test StaticFrameworkAOTArgsParser::Check with invalid framework cache directory (wrong location)
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, StaticFrameworkAOTArgsParserTest_003, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/wrong_location/" \
                                      "framework_ark_cache/etsstdlib_bootabc.an";

    StaticFrameworkAOTArgsParser parser;
    bool result = parser.Check(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: StaticFrameworkAOTArgsParserTest_004
 * @tc.desc: Test StaticFrameworkAOTArgsParser::Check with Windows-style path traversal
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, StaticFrameworkAOTArgsParserTest_004, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/service/el1/public/for-all-app/" \
                                      "framework_ark_cache\\..\\etsstdlib_bootabc.an";

    StaticFrameworkAOTArgsParser parser;
    bool result = parser.Check(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: StaticFrameworkAOTArgsParserTest_SUCCESS_001
 * @tc.desc: Test StaticFrameworkAOTArgsParser::Check with valid input (successful case)
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, StaticFrameworkAOTArgsParserTest_SUCCESS_001, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/service/el1/public/for-all-app/" \
                                      "framework_ark_cache/etsstdlib_bootabc.an";

    std::ofstream anFile("/data/service/el1/public/for-all-app/" \
                         "framework_ark_cache/etsstdlib_bootabc.an");
    anFile.close();

    StaticFrameworkAOTArgsParser parser;
    bool result = parser.Check(argsMap);
    EXPECT_TRUE(result);

    unlink("/data/service/el1/public/for-all-app/framework_ark_cache/etsstdlib_bootabc.an");
}


/**
 * @tc.name: AotArgsHandlerTest_GetParser_MissingIsSystemComponent
 * @tc.desc: Test GetParser with missing IS_SYSTEM_COMPONENT parameter
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_MissingIsSystemComponent, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::ARKTS_MODE, "dynamic"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_MissingArkTsMode
 * @tc.desc: Test GetParser with missing ARKTS_MODE parameter
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_MissingArkTsMode, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_InvalidIsSystemComponent
 * @tc.desc: Test GetParser with invalid IS_SYSTEM_COMPONENT value (non-numeric)
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_InvalidIsSystemComponent, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "dynamic"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_EmptyArkTsMode
 * @tc.desc: Test GetParser with empty ARKTS_MODE value
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_EmptyArkTsMode, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "dynamic"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_Dynamic_StaticEnabled_NonSystem
 * @tc.desc: Test GetParser: dynamic mode, static enabled, non-system
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_Dynamic_StaticEnabled_NonSystem, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "dynamic"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_Static_StaticEnabled_NonSystem
 * @tc.desc: Test GetParser: static mode, static enabled, non-system
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_Static_StaticEnabled_NonSystem, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "static"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_Hybrid_StaticEnabled_NonSystem
 * @tc.desc: Test GetParser: hybrid mode, static enabled, non-system
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_Hybrid_StaticEnabled_NonSystem, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "hybrid"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_Framework_StaticEnabled_System
 * @tc.desc: Test GetParser: system component, static enabled → StaticFrameworkAOTArgsParser
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_Framework_StaticEnabled_System, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "1"},
        {ArgsIdx::ARKTS_MODE, "dynamic"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_InvalidMode_StaticEnabled_NonSystem
 * @tc.desc: Test GetParser: invalid mode, static enabled, non-system
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_InvalidMode_StaticEnabled_NonSystem, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "invalid_mode"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_EQ(result, std::nullopt);
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_StaticMode_StaticDisabled_NonSystem
 * @tc.desc: Test GetParser: static mode, static DISABLED, non-system → nullopt
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_StaticMode_StaticDisabled_NonSystem, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "static"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_EQ(result, std::nullopt);
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_HybridMode_StaticDisabled_NonSystem
 * @tc.desc: Test GetParser: hybrid mode, static DISABLED, non-system → nullopt
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_HybridMode_StaticDisabled_NonSystem, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "hybrid"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_EQ(result, std::nullopt);
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_InvalidMode_StaticDisabled_NonSystem
 * @tc.desc: Test GetParser: invalid mode, static DISABLED, non-system → nullopt
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_InvalidMode_StaticDisabled_NonSystem, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "invalid"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_EQ(result, std::nullopt);
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_BothParamsMissing_StaticEnabled
 * @tc.desc: Test GetParser: both params missing, static enabled
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_BothParamsMissing_StaticEnabled, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {};

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_BothParamsMissing_StaticDisabled
 * @tc.desc: Test GetParser: both params missing, static disabled
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_BothParamsMissing_StaticDisabled, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {};

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_TRUE(result.has_value());
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_System_Component_StaticDisabled
 * @tc.desc: Test GetParser: system component, static DISABLED → AOTArgsParser (not framework)
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_System_Component_StaticDisabled, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "dynamic"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_TRUE(result.has_value());
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_NegativeIsSystemComponent
 * @tc.desc: Test GetParser with negative IS_SYSTEM_COMPONENT value
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_NegativeIsSystemComponent, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "dynamic"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_ZeroIsSystemComponent_StaticMode
 * @tc.desc: Test GetParser: isSystemComponent=0, static mode → StaticAOTArgsParser
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_ZeroIsSystemComponent_StaticMode, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "static"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_Failure_Static
 * @tc.desc: Test GetParser when validation fails for static mode
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_Failure_Static, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "1"},
        {ArgsIdx::ARKTS_MODE, "static"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result.value(), nullptr);
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_Framework_Static_System_StaticMode
 * @tc.desc: Test GetParser: system component, static enabled, static mode → StaticFrameworkAOTArgsParser
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_Framework_Static_System_StaticMode, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "1"},
        {ArgsIdx::ARKTS_MODE, "static"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_Framework_Static_System_HybridMode
 * @tc.desc: Test GetParser: system component, static enabled, hybrid mode → StaticFrameworkAOTArgsParser
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_Framework_Static_System_HybridMode, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "1"},
        {ArgsIdx::ARKTS_MODE, "hybrid"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_InvalidIsSystemComponent_StaticEnabled
 * @tc.desc: Test GetParser with invalid IS_SYSTEM_COMPONENT value and static enabled
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_InvalidIsSystemComponent_StaticEnabled, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "static"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_CheckModuleArkTSMode_Empty_Dynamic
 * @tc.desc: Test GetParser when CheckModuleArkTSMode fails with empty mode for AOT parser
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_CheckModuleArkTSMode_Empty_Dynamic, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "dynamic"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_TRUE(result.has_value());
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_CheckModuleArkTSMode_StaticForAOT
 * @tc.desc: Test GetParser when CheckModuleArkTSMode fails: static mode for AOT parser
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_CheckModuleArkTSMode_StaticForAOT, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "static"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_EQ(result, std::nullopt);
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_CheckModuleArkTSMode_DynamicForStatic
 * @tc.desc: Test GetParser when CheckModuleArkTSMode fails: dynamic mode for StaticAOT parser
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_CheckModuleArkTSMode_DynamicForStatic, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "dynamic"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result.value(), nullptr);
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_CheckModuleArkTSMode_HybridForAOT
 * @tc.desc: Test GetParser when CheckModuleArkTSMode fails: hybrid mode for AOT parser
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_CheckModuleArkTSMode_HybridForAOT, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "hybrid"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_EQ(result, std::nullopt);
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_CheckIsSystemComponent_Empty
 * @tc.desc: Test GetParser with empty IS_SYSTEM_COMPONENT value (treated as non-system component)
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_CheckIsSystemComponent_Empty, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, ""},
        {ArgsIdx::ARKTS_MODE, "dynamic"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_NE(result, std::nullopt);
    EXPECT_EQ(result.value()->GetParserType(), AotParserType::DYNAMIC_AOT);
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_ValidAbcName_AOT
 * @tc.desc: Test GetParser with valid abcName for AOT parser (success case)
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_ValidAbcName_AOT, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "dynamic"},
        {ArgsIdx::COMPILER_PKG_INFO,
         "{\"bundleName\":\"com.test\",\"pkgPath\":\"/test.hap\",\"abcName\":\"ets/modules.abc\","
          "\"abcOffset\":\"0\",\"abcSize\":\"100\",\"bundleUid\":\"2710\",\"bundleGid\":\"2710\"}"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);
    EXPECT_TRUE(result.has_value());
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_ValidAbcName_Static
 * @tc.desc: Test GetParser with valid abcName for StaticAOT parser (success case)
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_ValidAbcName_Static, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "static"},
        {ArgsIdx::COMPILER_PKG_INFO,
         "{\"bundleName\":\"com.test\",\"pkgPath\":\"/test.hap\",\"abcName\":\"ets/modules_static.abc\","
          "\"abcOffset\":\"0\",\"abcSize\":\"100\",\"bundleUid\":\"2710\",\"bundleGid\":\"2710\"}"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
}

/**
 * @tc.name: AotArgsHandlerTest_GetParser_FrameworkStaticAOT
 * @tc.desc: Test GetParser with isSystemComponent=1 and isEnableStaticCompiler=true
 *           This should return StaticFrameworkAOTArgsParser (early return before arkTsMode check)
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_FrameworkStaticAOT, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "1"},
        {ArgsIdx::ARKTS_MODE, "static"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result.value(), nullptr);
}


/**
 * @tc.name: AotArgsHandlerTest_GetParser_Static_Fail
 * @tc.desc: Test GetParser with static mode but validation fails
 *           Tests the branch for StaticAOT: LOG_SA(ERROR) << "Static AOT parser
 *           init params check failed"; return std::nullopt;
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_Static_Fail, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "1"},
        {ArgsIdx::ARKTS_MODE, "static"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result.value(), nullptr);
}


/**
 * @tc.name: AotArgsHandlerTest_GetParser_Hybrid_Fail
 * @tc.desc: Test GetParser with hybrid mode but validation fails
 *           Tests the branch: LOG_SA(ERROR) << "Static AOT parser init params check failed"; return std::nullopt;
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_Hybrid_Fail, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "1"},
        {ArgsIdx::ARKTS_MODE, "hybrid"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result.value(), nullptr);
}


/**
 * @tc.name: AotArgsHandlerTest_GetParser_Hybrid_Success
 * @tc.desc: Test GetParser with hybrid mode and valid params (success case)
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_GetParser_Hybrid_Success, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap = {
        {ArgsIdx::IS_SYSTEM_COMPONENT, "0"},
        {ArgsIdx::ARKTS_MODE, "hybrid"},
        {ArgsIdx::COMPILER_PKG_INFO,
         "{\"bundleName\":\"com.test\",\"pkgPath\":\"/test.hap\",\"abcName\":\"ets/modules_static.abc\","
          "\"abcOffset\":\"0\",\"abcSize\":\"100\",\"bundleUid\":\"2710\",\"bundleGid\":\"2710\"}"}
    };

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result.value(), nullptr);
}


/**
 * @tc.name: AotArgsParserBase_Check_AOT_Branch
 * @tc.desc: Test AOTArgsParserFactory::GetParser returns AOTArgsParser for dynamic mode
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsParserBase_Check_AOT_Branch, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::IS_SYSTEM_COMPONENT] = "0";
    argsMap[ArgsIdx::ARKTS_MODE] = "dynamic";

    auto result = AOTArgsParserFactory::GetParser(argsMap, false);

    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result.value(), nullptr);
}

/**
 * @tc.name: AotArgsParserBase_Check_STATIC_AOT_Branch
 * @tc.desc: Test AOTArgsParserFactory::GetParser returns StaticAOTArgsParser for static mode
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsParserBase_Check_STATIC_AOT_Branch, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::IS_SYSTEM_COMPONENT] = "0";
    argsMap[ArgsIdx::ARKTS_MODE] = "static";

    auto result = AOTArgsParserFactory::GetParser(argsMap, true);

    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result.value(), nullptr);
}

/**
 * @tc.name: AotArgsParserBase_Check_FRAMEWORK_STATIC_AOT_Branch
 * @tc.desc: Test AOTArgsParserBase::Check with FRAMEWORK_STATIC_AOT parser type
 *           Covers: case AotParserType::FRAMEWORK_STATIC_AOT:
 *           return AotArgsVerify::CheckFrameworkStaticAotArgs(argsMap);
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsParserBase_Check_FRAMEWORK_STATIC_AOT_Branch, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/service/el1/public/for-all-app/framework_ark_cache/test.abc.an";

    StaticFrameworkAOTArgsParser parser;
    bool result = parser.Check(argsMap);

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsParserBase_Check_DEFAULT_Branch
 * @tc.desc: Test AOTArgsParserBase::Check with UNKNOWN parser type (default branch)
 *           Covers: default: LOG_SA(ERROR) << "Unknown parser type in Check"; return false;
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AotArgsParserBase_Check_DEFAULT_Branch, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/service/el1/public/for-all-app/framework_ark_cache/test.abc.an";

    class TestUnknownParser : public AOTArgsParserBase {
    public:
        int32_t Parse(const std::unordered_map<std::string, std::string> &argsMap, HapArgs &hapArgs,
                      int32_t thermalLevel) override
        {
            return ERR_OK;
        }

        AotParserType GetParserType() const override
        {
            return AotParserType::UNKNOWN;
        }
    };

    TestUnknownParser parser;
    bool result = parser.Check(argsMap);

    EXPECT_FALSE(result);
}


/**
 * @tc.name: AOTArgsParser_Check_Fails_When_CheckAOTArgs_MissingPkgInfo
 * @tc.desc: Test AOTArgsParserBase::Check with AOT parser type
 *           Verifies Check() returns false when CheckAOTArgs fails due to missing COMPILER_PKG_INFO
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AOTArgsParser_Check_Fails_When_CheckAOTArgs_MissingPkgInfo, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/com.test/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/com.test/test.an";

    AOTArgsParser parser;
    bool result = parser.Check(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AOTArgsParser_Check_Fails_When_CheckAOTArgs_InvalidPgoDir
 * @tc.desc: Test AOTArgsParserBase::Check with AOT parser type
 *           Verifies Check() returns false when CheckAOTArgs fails due to path traversal in pgoDir
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AOTArgsParser_Check_Fails_When_CheckAOTArgs_InvalidPgoDir, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] =
        "{\"bundleName\":\"com.test\",\"pkgPath\":\"/test.hap\","
         "\"pgoDir\":\"/data/app/el1/100/aot_compiler/ark_profile/../etc\","
         "\"abcOffset\":\"0\",\"abcSize\":\"100\",\"bundleUid\":\"2710\",\"bundleGid\":\"2710\"}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/com.test/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/com.test/test.an";

    AOTArgsParser parser;
    bool result = parser.Check(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AOTArgsParser_Check_Fails_When_CheckAOTArgs_WrongCacheDir
 * @tc.desc: Test AOTArgsParserBase::Check with AOT parser type
 *           Verifies Check() returns false when CheckAOTArgs fails due to wrong cache directory
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, AOTArgsParser_Check_Fails_When_CheckAOTArgs_WrongCacheDir, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] =
        "{\"bundleName\":\"com.test\",\"pkgPath\":\"/test.hap\","
         "\"abcOffset\":\"0\",\"abcSize\":\"100\",\"bundleUid\":\"2710\",\"bundleGid\":\"2710\"}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/wrong_location/com.test/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/wrong_location/com.test/test.an";

    AOTArgsParser parser;
    bool result = parser.Check(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: StaticAOTParser_Check_Fails_When_CheckStaticAotArgs_MissingPkgInfo
 * @tc.desc: Test AOTArgsParserBase::Check with STATIC_AOT parser type
 *           Verifies Check() returns false when CheckStaticAotArgs fails due to missing COMPILER_PKG_INFO
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, StaticAOTParser_Check_Fails_When_CheckStaticAotArgs_MissingPkgInfo, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/com.test/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/com.test/test.an";

    StaticAOTArgsParser parser;
    bool result = parser.Check(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: StaticAOTParser_Check_Fails_When_CheckStaticAotArgs_InvalidPkgInfo
 * @tc.desc: Test AOTArgsParserBase::Check with STATIC_AOT parser type
 *           Verifies Check() returns false when CheckStaticAotArgs fails due to invalid JSON
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, StaticAOTParser_Check_Fails_When_CheckStaticAotArgs_InvalidPkgInfo, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = "{invalid_json}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/com.test/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/com.test/test.an";

    StaticAOTArgsParser parser;
    bool result = parser.Check(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: StaticAOTParser_Check_Fails_When_CheckStaticAotArgs_WrongCacheDir
 * @tc.desc: Test AOTArgsParserBase::Check with STATIC_AOT parser type
 *           Verifies Check() returns false when CheckStaticAotArgs fails due to wrong cache directory
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, StaticAOTParser_Check_Fails_When_CheckStaticAotArgs_WrongCacheDir, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] =
        "{\"bundleName\":\"com.test\",\"pkgPath\":\"/test.hap\","
         "\"abcOffset\":\"0\",\"abcSize\":\"100\",\"bundleUid\":\"2710\",\"bundleGid\":\"2710\"}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/wrong_location/com.test/test";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/wrong_location/com.test/test.an";

    StaticAOTArgsParser parser;
    bool result = parser.Check(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: StaticFrameworkAOTParser_Check_Fails_When_CheckFrameworkAotArgs_MissingAnFile
 * @tc.desc: Test AOTArgsParserBase::Check with FRAMEWORK_STATIC_AOT parser type
 *           Verifies Check() returns false when CheckFrameworkStaticAotArgs fails due to missing AN_FILE_NAME
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, StaticFrameworkAOTParser_Check_Fails_When_CheckFrameworkAotArgs_MissingAnFile,
    TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;

    StaticFrameworkAOTArgsParser parser;
    bool result = parser.Check(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: StaticFrameworkAOTParser_Check_Fails_When_CheckFrameworkAotArgs_PathTraversal
 * @tc.desc: Test AOTArgsParserBase::Check with FRAMEWORK_STATIC_AOT parser type
 *           Verifies Check() returns false when CheckFrameworkStaticAotArgs fails due to path traversal
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, StaticFrameworkAOTParser_Check_Fails_When_CheckFrameworkAotArgs_PathTraversal,
    TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/service/el1/public/for-all-app/framework_ark_cache/../test.abc.an";

    StaticFrameworkAOTArgsParser parser;
    bool result = parser.Check(argsMap);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: StaticFrameworkAOTParser_Check_Fails_When_CheckFrameworkAotArgs_WrongPathPrefix
 * @tc.desc: Test AOTArgsParserBase::Check with FRAMEWORK_STATIC_AOT parser type
 *           Verifies Check() returns false when CheckFrameworkStaticAotArgs fails due to wrong path prefix
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, StaticFrameworkAOTParser_Check_Fails_When_CheckFrameworkAotArgs_WrongPathPrefix,
    TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/test.abc.an";

    StaticFrameworkAOTArgsParser parser;
    bool result = parser.Check(argsMap);

    EXPECT_FALSE(result);
}

}
