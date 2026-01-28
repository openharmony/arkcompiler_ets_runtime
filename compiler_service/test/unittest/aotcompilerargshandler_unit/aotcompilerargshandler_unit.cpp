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
        mkdir(systemDir_, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        mkdir(systemFrameworkDir_, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        std::string bootpathJsonStr =
            "{\"bootpath\":\"/system/framework/etsstdlib_bootabc.abc:/system/framework/arkoala.abc\"}";
        std::ofstream file(bootPathJson_);
        file << bootpathJsonStr << std::endl;
        file.close();

        mkdir(etcDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        mkdir(etcArkkDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }

    void TearDown() override
    {
        unlink(bootPathJson_);
        rmdir(systemFrameworkDir_);
        rmdir(bootPathJson_);

        rmdir(etcDir);
        rmdir(etcArkkDir);
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

const std::string COMPILER_PKG_INFO_VALUE =
    "{\"abcName\":\"ets/modules.abc\","
    "\"abcOffset\":\"0x1000\","
    "\"abcSize\":\"0x60b058\","
    "\"appIdentifier\":\"5765880207853624761\","
    "\"bundleName\":\"com.ohos.contacts\","
    "\"BundleUid\":\"0x1317b6f\","
    "\"isEncryptedBundle\":\"0x0\","
    "\"isScreenOff\":\"0x1\","
    "\"moduleName\":\"entry\","
    "\"pgoDir\":\"/data/local/ark-profile/100/com.ohos.contacts\","
    "\"pkgPath\":\"/system/app/Contacts/Contacts.hap\","
    "\"processUid\":\"0xbf4\"}";

const std::unordered_map<std::string, std::string> argsMapForTest {
    {"target-compiler-mode", "partial"},
    {"aot-file", "/data/local/ark-cache/com.ohos.contacts/arm64/entry"},
    {"compiler-pkg-info", COMPILER_PKG_INFO_VALUE},
    {"compiler-external-pkg-info", "[]"},
    {"compiler-opt-bc-range", ""},
    {"compiler-device-state", "1"},
    {"compiler-baseline-pgo", "0"},
    {"ABC-Path", "/system/app/Contacts/Contacts.hap/ets/modules.abc"},
    {"BundleUid", "20020079"},
    {"BundleGid", "20020079"},
    {"anFileName", "/data/local/ark-cache/com.ohos.contacts/arm64/entry.an"},
    {"appIdentifier", "5765880207853624761"},
};

/**
 * @tc.name: AotArgsHandlerTest_007
 * @tc.desc: AOTArgsHandler::Handle(argsMap)
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_007, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap["target-compiler-mode"] = "partial";
    argsMap["aot-file"] = "/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64/entry";
    argsMap["compiler-pkg-info"] =
        "{\"abcName\":\"ets/modules.abc\","
        "\"abcOffset\":\"0x1000\","
        "\"abcSize\":\"0x60b058\","
        "\"appIdentifier\":\"5765880207853624761\","
        "\"bundleName\":\"com.ohos.contacts\","
        "\"BundleUid\":\"0x1317b6f\","
        "\"isEncryptedBundle\":\"0x0\","
        "\"isScreenOff\":\"0x1\","
        "\"moduleName\":\"entry\","
        "\"pgoDir\":\"/data/app/el1/100/aot_compiler/ark_profile/com.ohos.contacts\","
        "\"pkgPath\":\"/system/app/Contacts/Contacts.hap\","
        "\"processUid\":\"0xbf4\"}";
    argsMap["compiler-external-pkg-info"] = "[]";
    argsMap["compiler-opt-bc-range"] = "";
    argsMap["compiler-device-state"] = "1";
    argsMap["compiler-baseline-pgo"] = "0";
    argsMap["ABC-Path"] = "/system/app/Contacts/Contacts.hap/ets/modules.abc";
    argsMap["BundleUid"] = "20020079";
    argsMap["BundleGid"] = "20020079";
    argsMap["anFileName"] = "/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64/entry.an";
    argsMap["appIdentifier"] = "5765880207853624761";

    argsMap.emplace(ArgsIdx::ARKTS_MODE, "dynamic");

    // Create the required subdirectories
    mkdir("/data/app/el1/100/aot_compiler/ark_profile/com.ohos.contacts", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    // Create dummy files for testing
    std::ofstream anFile("/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64/entry.an");
    anFile.close();

    std::unique_ptr<AOTArgsHandler> argsHandler = std::make_unique<AOTArgsHandler>(argsMap);
    argsHandler->SetIsEnableStaticCompiler(true);
    argsHandler->SetParser(argsMap);
    int32_t ret = argsHandler->Handle(0);
    EXPECT_EQ(ret, ERR_OK);
    std::vector<const char*> argv = argsHandler->GetAotArgs();
    EXPECT_STREQ(argv[0], "/system/bin/ark_aot_compiler");

    // Clean up created files and directories
    unlink("/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64/entry.an");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts");
    rmdir("/data/app/el1/100/aot_compiler/ark_profile/com.ohos.contacts");
}

/**
 * @tc.name: AotArgsHandlerTest_008
 * @tc.desc: AOTArgsHandler::Handle(argsMap)
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_008, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap["target-compiler-mode"] = "partial";
    argsMap["aot-file"] = "/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64/entry";
    argsMap["compiler-pkg-info"] =
        "{\"abcName\":\"ets/modules.abc\","
        "\"abcOffset\":\"0x1000\","
        "\"abcSize\":\"0x60b058\","
        "\"appIdentifier\":\"5765880207853624761\","
        "\"bundleName\":\"com.ohos.contacts\","
        "\"BundleUid\":\"0x1317b6f\","
        "\"isEncryptedBundle\":\"0x0\","
        "\"isScreenOff\":\"0x1\","
        "\"moduleName\":\"entry\","
        "\"pgoDir\":\"/data/app/el1/100/aot_compiler/ark_profile/com.ohos.contacts\","
        "\"pkgPath\":\"/system/app/Contacts/Contacts.hap\","
        "\"processUid\":\"0xbf4\"}";
    argsMap["compiler-external-pkg-info"] = "[]";
    argsMap["compiler-opt-bc-range"] = "";
    argsMap["compiler-device-state"] = "1";
    argsMap["compiler-baseline-pgo"] = "0";
    argsMap["ABC-Path"] = "/system/app/Contacts/Contacts.hap/ets/modules.abc";
    argsMap["BundleUid"] = "20020079";
    argsMap["BundleGid"] = "20020079";
    argsMap["anFileName"] = "/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64/entry.an";
    argsMap["appIdentifier"] = "5765880207853624761";

    argsMap.emplace(ArgsIdx::ARKTS_MODE, "static");

    // Create the required subdirectories
    mkdir("/data/app/el1/100/aot_compiler/ark_profile/com.ohos.contacts", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    // Create dummy files for testing
    std::ofstream anFile("/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64/entry.an");
    anFile.close();

    std::unique_ptr<AOTArgsHandler> argsHandler = std::make_unique<AOTArgsHandler>(argsMap);
    argsHandler->SetIsEnableStaticCompiler(true);
    argsHandler->SetParser(argsMap);
    int32_t ret = argsHandler->Handle(0);
    EXPECT_EQ(ret, ERR_OK);
    std::vector<const char*> argv = argsHandler->GetAotArgs();
    EXPECT_STREQ(argv[0], "/system/bin/ark_aot");

    // Clean up created files and directories
    unlink("/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64/entry.an");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts");
    rmdir("/data/app/el1/100/aot_compiler/ark_profile/com.ohos.contacts");
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
    // Create abc file needed for ParseFrameworkBootPandaFiles
    std::string abcPath = "/system/framework/etsstdlib_bootabc.abc";
    std::ofstream abcFile(abcPath);
    abcFile.close();

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

    // Clean up
    unlink(abcPath.c_str());
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
    // pgoDir field in json
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
    // invalid json
    std::string pkgInfo = R"({invalidjson})";
    std::string profilePath;

    bool result = parser.ParseProfilePath(pkgInfo, profilePath);

    EXPECT_FALSE(result);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_025, TestSize.Level0)
{
    StaticAOTArgsParser parser;
    // empty json
    std::string pkgInfo = R"({})";
    std::string profilePath;

    bool result = parser.ParseProfilePath(pkgInfo, profilePath);

    EXPECT_FALSE(result);
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_026, TestSize.Level0)
{
    StaticAOTArgsParser parser;
    // no pgoDir field in json
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
    // pgoDir field in json
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
    // empty json
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
        {ArgsIdx::IS_SYSTEM_COMPONENT, "1"},
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
        {ArgsIdx::IS_SYSTEM_COMPONENT, "1"},
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
    // Test StaticFrameworkAOTArgsParser::ParseBlackListMethods with non-string elements in methodLists
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
    // Test StaticAOTArgsParser::ParseBlackListMethods with missing moduleLists
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
    // Test StaticAOTArgsParser::ParseBlackListMethods with empty blackMethodList
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
    // Test StaticFrameworkAOTArgsParser::ParseBlackListMethods with empty blackMethodList
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

    // Call ProcessArgsMap - this should trigger the staticAOTArgsList.find branch
    parser.ProcessArgsMap(argsMap, anfilePath, pkgInfo, partialMode, hapArgs);

    // Verify that the vector contains entries from the staticAOTArgsList branch
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
    // Create black list JSON file with matching bundle and module
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

    // Call ProcessBlackListMethods - this should add the method list to hapArgs
    parser.ProcessBlackListMethods(pkgInfo, anfilePath, hapArgs);

    // Verify that both arguments were added to the argument vector
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
    // This test verifies the case where the file doesn't exist
    // Use a path that doesn't exist
    nlohmann::json jsonObject;
    bool result = AOTArgsParserBase::ParseBlackListJson(jsonObject);

    // The function should return false when the file doesn't exist
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
    // Missing bundleName
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
    item["bundleName"] = 123; // Non-string value
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
    // Missing name
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
    moduleItem["name"] = 456; // Non-string value
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
    // Missing blackMethodList key
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
    jsonObject["blackMethodList"] = "not_an_array"; // Non-array value
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
    // Create black list JSON with multiple matching methods to trigger comma logic
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

    // Should return the regex pattern
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

    // Add a module with a non-matching name
    nlohmann::json moduleItem;
    moduleItem["name"] = "other_module";
    nlohmann::json methodLists = nlohmann::json::array();
    methodLists.push_back("Test:m1");
    moduleItem["methodLists"] = methodLists;
    moduleLists.push_back(moduleItem);

    item["moduleLists"] = moduleLists;

    StaticAOTArgsParser parser;
    std::vector<std::string> result = parser.ProcessMatchingModules(item, "entry");
    // Should return empty since no module matches "entry"
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
    std::string invalidPkgInfo = "{}"; // BundleName is missing
    std::string result = parser.ParseBlackListMethods(invalidPkgInfo, "entry");

    // Should return empty string when bundleName parsing fails
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
    std::string emptyBundlePkgInfo = "{\"bundleName\": \"\"}"; // BundleName is empty
    std::string result = parser.ParseBlackListMethods(emptyBundlePkgInfo, "entry");

    // Should return empty string when bundleName is empty
    EXPECT_STREQ(result.c_str(), "");
}

/**
 * @tc.name: AotArgsHandlerTest_063
 * @tc.desc: Test StaticAOTArgsHandler::Handle with blackListMethods non-empty path
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_063, TestSize.Level0)
{
    // Create black list JSON file
    std::ofstream(staticPaocBlackListPath)
        << "{\"blackMethodList\":[{\"bundleName\":\"com.ohos.contacts\",\"type\":\"application\","
        << "\"moduleLists\":[{\"name\":\"entry\",\"methodLists\":[\"Test:m1\",\"Test:m2\"]}]}]}";

    // Setup args map with all required fields
    std::unordered_map<std::string, std::string> argsMap = {
        {"target-compiler-mode", "partial"},
        {"aot-file", "/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64/entry"},
        {"compiler-pkg-info", "{\"abcName\":\"ets/modules.abc\",\"abcOffset\":\"0x1000\","
            "\"abcSize\":\"0x60b058\",\"appIdentifier\":\"5765880207853624761\","
            "\"bundleName\":\"com.ohos.contacts\",\"BundleUid\":\"0x1317b6f\","
            "\"isEncryptedBundle\":\"0x0\",\"isScreenOff\":\"0x1\",\"moduleName\":\"entry\","
            "\"pgoDir\":\"/data/app/el1/100/aot_compiler/ark_profile/com.ohos.contacts\","
            "\"pkgPath\":\"/system/app/Contacts/Contacts.hap\",\"processUid\":\"0xbf4\"}"},
        {"compiler-external-pkg-info", "[]"}, {"compiler-opt-bc-range", ""},
        {"compiler-device-state", "1"}, {"compiler-baseline-pgo", "0"},
        {"ABC-Path", "/system/app/Contacts/Contacts.hap/ets/modules.abc"},
        {"BundleUid", "20020079"}, {"BundleGid", "20020079"},
        {"anFileName", "/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64/entry.an"},
        {"appIdentifier", "5765880207853624761"},
        {ArgsIdx::ARKTS_MODE, "static"}
    };

    // Create test directories and file
    std::string basePath = "/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts";
    mkdir("/data/app/el1/100/aot_compiler/ark_profile/com.ohos.contacts",
          S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir(basePath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir((basePath + "/arm64").c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::ofstream((basePath + "/arm64/entry.an").c_str()).close();

    // Execute test
    std::unique_ptr<AOTArgsHandler> argsHandler = std::make_unique<AOTArgsHandler>(argsMap);
    argsHandler->SetIsEnableStaticCompiler(true);
    argsHandler->SetParser(argsMap);
    EXPECT_EQ(argsHandler->Handle(0), ERR_OK);

    // Clean up
    unlink((basePath + "/arm64/entry.an").c_str());
    rmdir((basePath + "/arm64").c_str());
    rmdir(basePath.c_str());
    rmdir("/data/app/el1/100/aot_compiler/ark_profile/com.ohos.contacts");
    unlink(staticPaocBlackListPath);
}

/**
 * @tc.name: AotArgsHandlerTest_064
 * @tc.desc: Test StaticFrameworkAOTArgsParser::CheckBundleNameAndMethodList with missing bundleName
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_064, TestSize.Level0)
{
    nlohmann::json item;
    // Missing bundleName
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
    item["bundleName"] = 123; // Non-string value
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
    // methodLists is null
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

    // Should return empty when black list file doesn't exist
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

    // Use a known argument that exists in staticAOTArgsList
    argsMap["boot-panda-files"] = "entry.abc";
    std::string anfilePath;
    std::string pkgInfo;
    bool partialMode = false;

    // Call ProcessArgsMap - this should trigger the staticAOTArgsList.find branch
    parser.ProcessArgsMap(argsMap, anfilePath, pkgInfo, partialMode, hapArgs);

    // Verify that the vector contains the specific argument from staticAOTArgsList branch
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
    // The ParseBlackListJson function specifically looks for a fixed path at runtime
    // This covers the !IsFileExists(STATIC_PAOC_BLACK_LIST_PATH) path
    nlohmann::json jsonObject;
    bool result = AOTArgsParserBase::ParseBlackListJson(jsonObject);
    // This should return false since the file doesn't exist by default
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsHandlerTest_070
 * @tc.desc: Test StaticAOTArgsParser::ProcessBlackListForBundleAndModule with comma concatenation
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_070, TestSize.Level0)
{
    // Create JSON with multiple items to trigger the comma logic
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

    // Should return regex pattern
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

    // Add modules with matching names to trigger comma concatenation
    nlohmann::json moduleItem1, moduleItem2;
    moduleItem1["name"] = "entry";
    nlohmann::json methodLists1 = nlohmann::json::array();
    methodLists1.push_back("Test:m1");
    moduleItem1["methodLists"] = methodLists1;

    moduleItem2["name"] = "entry";  // Same name to match
    nlohmann::json methodLists2 = nlohmann::json::array();
    methodLists2.push_back("Test:m2");
    moduleItem2["methodLists"] = methodLists2;

    moduleLists.push_back(moduleItem1);
    moduleLists.push_back(moduleItem2);
    item["moduleLists"] = moduleLists;

    StaticAOTArgsParser parser;
    std::vector<std::string> result = parser.ProcessMatchingModules(item, "entry");

    // Should return concatenated methods with comma
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
    // Test with empty bundleName - this should trigger the bundleName.empty() check
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
    // Create black list JSON file to be found
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

    // Create abc file needed for ParseFrameworkBootPandaFiles
    std::string abcPath = "/system/framework/etsstdlib_bootabc.abc";
    std::ofstream abcFile(abcPath);
    abcFile.close();

    // Use framework args map
    std::unordered_map<std::string, std::string> argsMap(framewordArgsMapForTest);
    argsMap.emplace(ArgsIdx::ARKTS_MODE, "static");
    argsMap.emplace(ArgsIdx::IS_SYSTEM_COMPONENT, "1");

    std::unique_ptr<AOTArgsHandler> argsHandler = std::make_unique<AOTArgsHandler>(argsMap);
    argsHandler->SetIsEnableStaticCompiler(true);
    argsHandler->SetParser(argsMap);
    int32_t ret = argsHandler->Handle(0);
    EXPECT_EQ(ret, ERR_OK);

    // Clean up
    unlink(staticPaocBlackListPath);
    unlink(abcPath.c_str());
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

    // Should return empty when black list file doesn't exist or is invalid
    EXPECT_STREQ(result.c_str(), "");
}

/**
 * @tc.name: AotArgsHandlerTest_075
 * @tc.desc: Test StaticFrameworkAOTArgsParser::ParseBlackListMethods with comma concatenation
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_075, TestSize.Level0)
{
    // Create black list JSON with multiple matching methods
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

    // Should return regex pattern
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
    // Create a JSON file with 'null' content to trigger the jsonObject.is_null() condition
    std::string nullJson = "null";
    system("mkdir -p /etc/ark");
    std::ofstream file("/etc/ark/static_aot_methods_black_list.json");
    file << nullJson << std::endl;
    file.close();

    nlohmann::json jsonObject;
    bool result = AOTArgsParserBase::ParseBlackListJson(jsonObject);
    // When the parsed JSON is null, the function should return false due to is_null() check
    // Note: This may or may not actually trigger the condition depending on how nlohmann::json handles 'null'
    EXPECT_FALSE(result); // Expecting false due to null JSON

    unlink("/etc/ark/static_aot_methods_black_list.json");
}

/**
 * @tc.name: AotArgsHandlerTest_078
 * @tc.desc: Test StaticFrameworkAOTArgsParser::ParseBlackListMethods with missing blackMethodList
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_078, TestSize.Level0)
{
    // Create a JSON file without blackMethodList to trigger the condition
    std::string invalidJson = "{\"otherKey\": \"otherValue\"}";  // Missing blackMethodList
    system("mkdir -p /etc/ark");
    std::ofstream file("/etc/ark/static_aot_methods_black_list.json");
    file << invalidJson << std::endl;
    file.close();

    StaticFrameworkAOTArgsParser parser;
    std::string bundleName = "some_bundle";
    std::string result = parser.ParseBlackListMethods(bundleName);

    // Should return empty string when blackMethodList is missing
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
    // Create a JSON file with blackMethodList as non-array to trigger the condition
    std::string invalidJson = "{\"blackMethodList\": \"not_an_array\"}";  // blackMethodList is string, not array
    system("mkdir -p /etc/ark");
    std::ofstream file("/etc/ark/static_aot_methods_black_list.json");
    file << invalidJson << std::endl;
    file.close();

    StaticFrameworkAOTArgsParser parser;
    std::string bundleName = "some_bundle";
    std::string result = parser.ParseBlackListMethods(bundleName);

    // Should return empty string when blackMethodList is not an array
    EXPECT_STREQ(result.c_str(), "");

    unlink("/etc/ark/static_aot_methods_black_list.json");
}

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_080, TestSize.Level0)
{
    // Create a JSON file that exists but has no blackMethodList to test the exact condition
    std::string jsonContent = "{}";  // Empty JSON object
    system("mkdir -p /etc/ark");
    std::ofstream file("/etc/ark/static_aot_methods_black_list.json");
    file << jsonContent << std::endl;
    file.close();

    StaticFrameworkAOTArgsParser parser;
    std::string bundleName = "test_bundle";
    std::string result = parser.ParseBlackListMethods(bundleName);

    // Should return empty string when blackMethodList is missing
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
    std::string jsonContent = "{\"blackMethodList\": []}";  // Empty array
    system("mkdir -p /etc/ark");
    std::ofstream file("/etc/ark/static_aot_methods_black_list.json");
    file << jsonContent << std::endl;
    file.close();

    StaticFrameworkAOTArgsParser parser;
    std::string bundleName = "test_bundle";
    std::string result = parser.ParseBlackListMethods(bundleName);

    // Should return empty string when blackMethodList is an empty array
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
    // Create a JSON file with multiple matching items to trigger resultStr += "," condition
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

    // Should return concatenated methods with comma when multiple items match
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
    // Test AOTArgsParserBase::JoinMethodList with empty array
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
    // Test AOTArgsParserBase::JoinMethodList with method list
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
    // Test AOTArgsParserBase::JoinMethodList with non-string elements
    nlohmann::json methodLists = nlohmann::json::array();
    methodLists.push_back("Test:m1");
    methodLists.push_back(123);  // non-string element
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
    // Test StaticAOTArgsParser::CheckBundleNameAndModuleList with valid bundle and module list
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
    // Test StaticAOTArgsParser::CheckBundleNameAndModuleList with invalid bundle name
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
    // Test StaticAOTArgsParser::CheckBundleNameAndModuleList without module list
    nlohmann::json item;
    item["bundleName"] = "com.example.test";
    // Missing moduleLists
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
    // Test StaticAOTArgsParser::CheckModuleNameAndMethodList with valid module and method list
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
    // Test StaticAOTArgsParser::CheckModuleNameAndMethodList with invalid module name
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
    // Test StaticAOTArgsParser::CheckModuleNameAndMethodList without method list
    nlohmann::json moduleItem;
    moduleItem["name"] = "entry";
    // Missing methodLists
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
    // Test StaticFrameworkAOTArgsParser::ParseBlackListMethods with matching bundleName
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
    // Test StaticFrameworkAOTArgsParser::ParseBlackListMethods with non-matching bundleName
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
    // Test StaticFrameworkAOTArgsParser::ParseBlackListMethods without methodLists
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
    // Test StaticAOTArgsParser with proper blackMethodList JSON format (as per paoc.md)
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
    // Test StaticFrameworkAOTArgsParser with proper blackMethodList JSON format for framework (as per paoc.md)
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
    // Test ParseBlackListJson with blackMethodList structure as per paoc.md
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
    // Test StaticAOTArgsParser::ParseBlackListMethods with non-string elements in methodLists
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
    // methodLists is null
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
        {ArgsIdx::IS_SYSTEM_COMPONENT, "invalid_value"},
        {ArgsIdx::ARKTS_MODE, "dynamic"}
    };
    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
}

/**
 * @tc.name: AotArgsHandlerTest_112
 * @tc.desc: Test AOTArgsHandler::Handle with hybrid mode
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_112, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap(argsMapForTest);
    argsMap.emplace(ArgsIdx::ARKTS_MODE, "hybrid");
    std::unique_ptr<AOTArgsHandler> argsHandler = std::make_unique<AOTArgsHandler>(argsMap);
    argsHandler->SetIsEnableStaticCompiler(true);
    argsHandler->SetParser(argsMap);
    int32_t ret = argsHandler->Handle(0);
    EXPECT_EQ(ret, ERR_OK);
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
        {ArgsIdx::IS_SYSTEM_COMPONENT, "invalid_value"},
        {ArgsIdx::ARKTS_MODE, "dynamic"}
    };
    auto result = AOTArgsParserFactory::GetParser(argsMap, true);
    EXPECT_TRUE(result.has_value());
}

/**
 * @tc.name: AotArgsHandlerTest_116
 * @tc.desc: Test AOTArgsHandler::Handle with hybrid mode
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_116, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap(argsMapForTest);
    argsMap.emplace(ArgsIdx::ARKTS_MODE, "hybrid");
    std::unique_ptr<AOTArgsHandler> argsHandler = std::make_unique<AOTArgsHandler>(argsMap);
    argsHandler->SetIsEnableStaticCompiler(true);
    argsHandler->SetParser(argsMap);
    int32_t ret = argsHandler->Handle(0);
    EXPECT_EQ(ret, ERR_OK);
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

// ============================================================================
// StaticAOTArgsParser::Check 测试用例
// ============================================================================

/**
 * @tc.name: StaticAOTArgsParserTest_001
 * @tc.desc: Test StaticAOTArgsParser::Check with missing PKG_INFO
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, StaticAOTArgsParserTest_001, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    // Don't add PKG_INFO to trigger the failure condition
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
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = "{}"; // Empty JSON, no bundleName
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
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = "{\"bundleName\": \"com.example.myapplication\"}";
    // Don't add AOT_FILE to trigger the failure condition
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
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = "{\"bundleName\": \"com.example.myapplication\"}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/com.example.myapplication/arm64/entry";
    // Don't add AN_FILE_NAME to trigger the failure condition

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
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = "{\"bundleName\": \"com.example.myapplication\"}";
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
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = "{\"bundleName\": \"com.example.myapplication\"}";
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
 * @tc.desc: Test StaticAOTArgsParser::Check with valid inputs (successful case)
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, StaticAOTArgsParserTest_SUCCESS_001, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    argsMap[ArgsIdx::COMPILER_PKG_INFO] = "{\"bundleName\": \"com.example.myapplication\"}";
    argsMap[ArgsIdx::AOT_FILE] = "/data/app/el1/public/aot_compiler/ark_cache/" \
                                "com.example.myapplication/arm64/entry";
    argsMap[ArgsIdx::AN_FILE_NAME] = "/data/app/el1/public/aot_compiler/ark_cache/" \
                                 "com.example.myapplication/arm64/entry.an";

    // Create the required subdirectory
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/com.example.myapplication/arm64",
          S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    // Create dummy files for testing
    std::ofstream anFile("/data/app/el1/public/aot_compiler/ark_cache/" \
                         "com.example.myapplication/arm64/entry.an");
    anFile.close();

    StaticAOTArgsParser parser;
    bool result = parser.Check(argsMap);

    EXPECT_TRUE(result);

    // Clean up created files and directories
    unlink("/data/app/el1/public/aot_compiler/ark_cache/com.example.myapplication/arm64/entry.an");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache/com.example.myapplication/arm64");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache/com.example.myapplication");
}

// ============================================================================
// StaticFrameworkAOTArgsParser::Check 测试用例
// ============================================================================

/**
 * @tc.name: StaticFrameworkAOTArgsParserTest_001
 * @tc.desc: Test StaticFrameworkAOTArgsParser::Check with missing AN_FILE_NAME
 * @tc.type: Func
 */
HWTEST_F(AotArgsHandlerTest, StaticFrameworkAOTArgsParserTest_001, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    // Don't add AN_FILE_NAME to trigger the failure condition

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

    // Create the required subdirectory
    mkdir("/data/service/el1/public/for-all-app/framework_ark_cache",
          S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    // Create dummy file for testing
    std::ofstream anFile("/data/service/el1/public/for-all-app/" \
                         "framework_ark_cache/etsstdlib_bootabc.an");
    anFile.close();

    StaticFrameworkAOTArgsParser parser;
    bool result = parser.Check(argsMap);

    EXPECT_TRUE(result);

    // Clean up created files and directories
    unlink("/data/service/el1/public/for-all-app/framework_ark_cache/etsstdlib_bootabc.an");
    rmdir("/data/service/el1/public/for-all-app/framework_ark_cache");
    rmdir("/data/service/el1/public/for-all-app");
    rmdir("/data/service/el1/public");
}

} // namespace OHOS::ArkCompiler
