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
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <vector>

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
    void SetUp() override {}
    void TearDown() override {};
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
    std::unordered_map<std::string, std::string> argsMap(argsMapForTest);
    argsMap.emplace(ArgsIdx::ARKTS_MODE, "dynamic");
    std::unique_ptr<AOTArgsHandler> argsHandler = std::make_unique<AOTArgsHandler>(argsMap);
    int32_t ret = argsHandler->Handle(0);
    EXPECT_EQ(ret, ERR_OK);
    std::vector<const char*> argv = argsHandler->GetAotArgs();
    EXPECT_STREQ(argv[0], "/system/bin/ark_aot_compiler");
}

/**
 * @tc.name: AotArgsHandlerTest_008
 * @tc.desc: AOTArgsHandler::Handle(argsMap)
 * @tc.type: Func
*/
HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_008, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap(argsMapForTest);
    argsMap.emplace(ArgsIdx::ARKTS_MODE, "static");
    std::unique_ptr<AOTArgsHandler> argsHandler = std::make_unique<AOTArgsHandler>(argsMap);
    int32_t ret = argsHandler->Handle(0);
    EXPECT_EQ(ret, ERR_OK);
    std::vector<const char*> argv = argsHandler->GetAotArgs();
    EXPECT_STREQ(argv[0], "/system/bin/ark_aot");
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
    {"sysCompPath", "/system/framework/etsstdlib_bootabc.abc"},
    {"ABC-Path", "/system/framework/etsstdlib_bootabc.abc"}
};

HWTEST_F(AotArgsHandlerTest, AotArgsHandlerTest_010, TestSize.Level0)
{
    const char *systemDir = "/system";
    const char *systemFrameworkDir = "/system/framework";
    const char *bootPathJson = "/system/framework/bootpath.json";
    mkdir(systemDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir(systemFrameworkDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::string bootpathJsonStr =
        "{\"bootpath\":\"/system/framework/etsstdlib_bootabc.abc:/system/framework/arkoala.abc\"}";
    std::ofstream file(bootPathJson);
    file << bootpathJsonStr << std::endl;
    file.close();

    std::unique_ptr<AOTArgsHandler> argsHandler = std::make_unique<AOTArgsHandler>(framewordArgsMapForTest);
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
    unlink(bootPathJson);
    rmdir(systemFrameworkDir);
    rmdir(systemDir);
}
} // namespace OHOS::ArkCompiler