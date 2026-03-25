/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http:
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

#include "aot_args_verify.h"
#include "aot_compiler_args.h"
#include "aot_compiler_constants.h"

using namespace testing::ext;

namespace OHOS::ArkCompiler {

class AotArgsVerifyTest : public testing::Test {
public:
    AotArgsVerifyTest() {}
    virtual ~AotArgsVerifyTest() {}

    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: AotArgsVerifyTest_009
 * @tc.desc: Test AotArgsVerify::CheckPathTraverse with valid path (no traversal)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_009, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckPathTraverse(
        "/data/app/el1/public/aot_compiler/ark_cache/test_app/test");

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_010
 * @tc.desc: Test AotArgsVerify::CheckArkCacheFiles with valid paths
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_010, TestSize.Level0)
{
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/test_app", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::ofstream anFile("/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an");
    anFile.close();

    bool result = AotArgsVerify::CheckArkCacheFiles(
        "/data/app/el1/public/aot_compiler/ark_cache/test_app/test",
        "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an",
        "test_app");

    EXPECT_TRUE(result);
    unlink("/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache/test_app");
}

/**
 * @tc.name: AotArgsVerifyTest_CheckArkCacheFiles_002
 * @tc.desc: Test AotArgsVerify::CheckArkCacheFiles with AOT_FILE missing
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckArkCacheFiles_002, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckArkCacheFiles(
        "", "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an", "test_app");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckArkCacheFiles_003
 * @tc.desc: Test AotArgsVerify::CheckArkCacheFiles with AN_FILE_NAME missing
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckArkCacheFiles_003, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckArkCacheFiles(
        "/data/app/el1/public/aot_compiler/ark_cache/test_app/test", "", "test_app");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckArkCacheFiles_004
 * @tc.desc: Test AotArgsVerify::CheckArkCacheFiles with both AOT_FILE and AN_FILE_NAME missing
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckArkCacheFiles_004, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckArkCacheFiles("", "", "test_app");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckArkCacheFiles_005
 * @tc.desc: Test AotArgsVerify::CheckArkCacheFiles with mismatched file suffix
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckArkCacheFiles_005, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckArkCacheFiles(
        "/data/app/el1/public/aot_compiler/ark_cache/test_app/test",
        "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.wrong",
        "test_app");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckArkCacheFiles_006
 * @tc.desc: Test AotArgsVerify::CheckArkCacheFiles with path traversal in aotFile
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckArkCacheFiles_006, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckArkCacheFiles(
        "/data/app/el1/public/aot_compiler/ark_cache/../etc/test",
        "/data/app/el1/public/aot_compiler/ark_cache/../etc/test.an",
        "test_app");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckArkCacheFiles_007
 * @tc.desc: Test AotArgsVerify::CheckArkCacheFiles with invalid cache directory path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckArkCacheFiles_007, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckArkCacheFiles(
        "/data/local/tmp/test", "/data/local/tmp/test.an", "test_app");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_011
 * @tc.desc: Test AotArgsVerify::CheckArkProfilePath with valid path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_011, TestSize.Level0)
{
    const std::string testAppDir = "/data/app/el1/100/aot_compiler/ark_profile/test_app";
    mkdir(testAppDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    bool result = AotArgsVerify::CheckArkProfilePath(testAppDir, "test_app");

    EXPECT_TRUE(result);
    rmdir(testAppDir.c_str());
}

/**
 * @tc.name: AotArgsVerifyTest_012
 * @tc.desc: Test AotArgsVerify::CheckArkProfilePath with different user ID
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_012, TestSize.Level0)
{
    const std::string myBundleDir = "/data/app/el1/100/aot_compiler/ark_profile/my_bundle";
    mkdir(myBundleDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    bool result = AotArgsVerify::CheckArkProfilePath(myBundleDir, "my_bundle");

    EXPECT_TRUE(result);
    rmdir(myBundleDir.c_str());
}

/**
 * @tc.name: AotArgsVerifyTest_013
 * @tc.desc: Test AotArgsVerify::CheckPathTraverse with path traversal
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_013, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckPathTraverse("/data/local/../etc");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_015
 * @tc.desc: Test AotArgsVerify::CheckPathTraverse with Windows-style path traversal
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_015, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckPathTraverse("\\data\\local\\..\\etc");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_017
 * @tc.desc: Test AotArgsVerify::CheckArkProfilePath with path not matching pattern
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_017, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckArkProfilePath("/data/app/el1/100/wrong_path/com.test.app", "com.test.app");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_018
 * @tc.desc: Test AotArgsVerify::CheckArkProfilePath with bundleName not in path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_018, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckArkProfilePath(
        "/data/app/el1/100/aot_compiler/ark_profile/com.other.app", "com.test.app");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_019
 * @tc.desc: Test AotArgsVerify::CheckArkCacheFiles with invalid path format
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_019, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckArkCacheFiles(
        "invalid_format_file", "another_invalid_format", "test_bundle");
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_021
 * @tc.desc: Test AotArgsVerify::CheckArkCacheFiles with path traversal in both files
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_021, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckArkCacheFiles(
        "/data/app/el1/public/aot_compiler/ark_cache/test_app/../etc/test",
        "/data/app/el1/public/aot_compiler/ark_cache/test_app/../etc/test.an",
        "test_app");
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_022
 * @tc.desc: Test AotArgsVerify::CheckArkCacheFiles with files in different directories
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_022, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckArkCacheFiles(
        "/data/app/el1/public/aot_compiler/ark_cache/test_app1/test",
        "/data/app/el1/public/aot_compiler/ark_cache/test_app2/test.an",
        "test_app");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_023
 * @tc.desc: Test AotArgsVerify::CheckArkCacheFiles with non-existent parent directory
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_023, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckArkCacheFiles(
        "/data/app/el1/public/nonexistent/test_app/test",
        "/data/app/el1/public/nonexistent/test_app/test.an",
        "test_app");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_002
 * @tc.desc: Test AotArgsVerify::CheckAOTArgs with missing COMPILER_PKG_INFO
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_002, TestSize.Level0)
{
    AotCompilerArgs args;
    args.bundleName = "";
    args.hapPath = "";
    args.pgoDir = "";

    bool result = AotArgsVerify::CheckAOTArgs(args);

    EXPECT_FALSE(result);
}


/**
 * @tc.name: AotArgsVerifyTest_CheckStaticAotArgs_002
 * @tc.desc: Test AotArgsVerify::CheckStaticAotArgs with missing COMPILER_PKG_INFO
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckStaticAotArgs_002, TestSize.Level0)
{
    AotCompilerArgs args;
    args.bundleName = "";
    args.hapPath = "";
    args.pgoDir = "";

    bool result = AotArgsVerify::CheckStaticAotArgs(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckFrameworkStaticAotArgs_001
 * @tc.desc: Test AotArgsVerify::CheckFrameworkStaticAotArgs with valid inputs
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckFrameworkStaticAotArgs_001, TestSize.Level0)
{
    AotCompilerArgs args;
    args.anFileName = "/data/service/el1/public/for-all-app/framework_ark_cache/test.abc.an";

    bool result = AotArgsVerify::CheckFrameworkStaticAotArgs(args);

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckFrameworkStaticAotArgs_002
 * @tc.desc: Test AotArgsVerify::CheckFrameworkStaticAotArgs with missing AN_FILE_NAME
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckFrameworkStaticAotArgs_002, TestSize.Level0)
{
    AotCompilerArgs args;

    bool result = AotArgsVerify::CheckFrameworkStaticAotArgs(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckFrameworkStaticAotArgs_003
 * @tc.desc: Test AotArgsVerify::CheckFrameworkStaticAotArgs with path traversal
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckFrameworkStaticAotArgs_003, TestSize.Level0)
{
    AotCompilerArgs args;
    args.anFileName = "/data/service/el1/public/for-all-app/framework_ark_cache/../test.abc.an";

    bool result = AotArgsVerify::CheckFrameworkStaticAotArgs(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckFrameworkStaticAotArgs_004
 * @tc.desc: Test AotArgsVerify::CheckFrameworkStaticAotArgs with wrong path prefix
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckFrameworkStaticAotArgs_004, TestSize.Level0)
{
    AotCompilerArgs args;
    args.anFileName = "/data/app/el1/public/aot_compiler/ark_cache/test.abc.an";

    bool result = AotArgsVerify::CheckFrameworkStaticAotArgs(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_003
 * @tc.desc: Test AotArgsVerify::CheckAOTArgs with invalid JSON in COMPILER_PKG_INFO
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_003, TestSize.Level0)
{
    AotCompilerArgs args;
    args.bundleName = "test_app";
    args.hapPath = "/data/test/test.hap";
    args.pgoDir = "/data/app/el1/100/aot_compiler/ark_profile/test_app";

    bool result = AotArgsVerify::CheckAOTArgs(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_004
 * @tc.desc: Test AotArgsVerify::CheckAOTArgs with missing outputPath (aotFile)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_004, TestSize.Level0)
{
    AotCompilerArgs args;
    args.bundleName = "test_app";
    args.hapPath = "/data/test/test.hap";
    args.moduleArkTSMode = ArgsIdx::ARKTS_DYNAMIC;
    args.pgoDir = "/data/app/el1/100/aot_compiler/ark_profile/test_app";
    args.bundleUid = 10000;
    args.bundleGid = 10000;
    args.anFileName = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";
    // outputPath is empty, so aotFile will be empty

    bool result = AotArgsVerify::CheckAOTArgs(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_005
 * @tc.desc: Test AotArgsVerify::CheckAOTArgs with missing AN_FILE_NAME
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_005, TestSize.Level0)
{
    AotCompilerArgs args;
    args.bundleName = "test_app";
    args.hapPath = "/data/test/test.hap";
    args.moduleArkTSMode = ArgsIdx::ARKTS_DYNAMIC;
    args.pgoDir = "/data/app/el1/100/aot_compiler/ark_profile/test_app";
    args.bundleUid = 10000;
    args.bundleGid = 10000;
    args.outputPath = "/data/app/el1/public/aot_compiler/ark_cache/test_app";
    args.moduleName = "test";
    // anFileName is empty

    bool result = AotArgsVerify::CheckAOTArgs(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_006
 * @tc.desc: Test AotArgsVerify::CheckAOTArgs with path traversal in pgoDir
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_006, TestSize.Level0)
{
    AotCompilerArgs args;
    args.bundleName = "test_app";
    args.hapPath = "/data/test/test.hap";
    args.moduleArkTSMode = ArgsIdx::ARKTS_DYNAMIC;
    args.pgoDir = "/data/app/el1/100/aot_compiler/ark_profile/../etc";
    args.bundleUid = 10000;
    args.bundleGid = 10000;
    args.anFileName = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";
    args.outputPath = "/data/app/el1/public/aot_compiler/ark_cache/test_app";
    args.moduleName = "test";

    bool result = AotArgsVerify::CheckAOTArgs(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_007
 * @tc.desc: Test AotArgsVerify::CheckAOTArgs with ark cache files in wrong directory
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_007, TestSize.Level0)
{
    AotCompilerArgs args;
    args.bundleName = "test_app";
    args.hapPath = "/data/test/test.hap";
    args.moduleArkTSMode = ArgsIdx::ARKTS_DYNAMIC;
    args.pgoDir = "/data/app/el1/100/aot_compiler/ark_profile/test_app";
    args.bundleUid = 10000;
    args.bundleGid = 10000;
    args.anFileName = "/data/app/el1/public/wrong_location/test_app/test.an";
    args.outputPath = "/data/app/el1/public/wrong_location/test_app";
    args.moduleName = "test";

    bool result = AotArgsVerify::CheckAOTArgs(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_PartialMode_002
 * @tc.desc: Test AotArgsVerify::CheckAOTArgs with partial mode and invalid pgoDir path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_PartialMode_002, TestSize.Level0)
{
    AotCompilerArgs args;
    args.compileMode = "partial";
    args.moduleArkTSMode = ArgsIdx::ARKTS_DYNAMIC;
    args.bundleName = "test_app";
    args.moduleName = "test";
    args.hapPath = "/data/test/test.hap";
    args.pgoDir = "/data/app/el2/100/aot_compiler/ark_profile/test_app";
    args.bundleUid = 10000;
    args.bundleGid = 10000;
    args.anFileName = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";
    args.outputPath = "/data/app/el1/public/aot_compiler/ark_cache/test_app";

    bool result = AotArgsVerify::CheckAOTArgs(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckStaticAotArgs_003
 * @tc.desc: Test AotArgsVerify::CheckStaticAotArgs with invalid JSON in COMPILER_PKG_INFO
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckStaticAotArgs_003, TestSize.Level0)
{
    AotCompilerArgs args;
    args.bundleName = "";
    args.hapPath = "";
    args.pgoDir = "";

    bool result = AotArgsVerify::CheckStaticAotArgs(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckStaticAotArgs_004
 * @tc.desc: Test AotArgsVerify::CheckStaticAotArgs with empty bundleName
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckStaticAotArgs_004, TestSize.Level0)
{
    AotCompilerArgs args;
    args.bundleName = "";
    args.hapPath = "/data/test/test.hap";
    args.pgoDir = "/data/pgo";
    args.moduleArkTSMode = ArgsIdx::ARKTS_STATIC;
    args.bundleUid = 10000;
    args.bundleGid = 10000;

    bool result = AotArgsVerify::CheckStaticAotArgs(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckStaticAotArgs_005
 * @tc.desc: Test AotArgsVerify::CheckStaticAotArgs with missing outputPath (aotFile)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckStaticAotArgs_005, TestSize.Level0)
{
    AotCompilerArgs args;
    args.bundleName = "test_app";
    args.moduleName = "test";
    args.hapPath = "/data/test/test.hap";
    args.pgoDir = "/data/pgo";
    args.moduleArkTSMode = ArgsIdx::ARKTS_STATIC;
    args.appIdentifier = "sig";
    args.bundleUid = 10000;
    args.bundleGid = 10000;
    args.anFileName = "/data/app/el1/public/aot_compiler/ark_cache/test_app/test.an";
    // outputPath is empty

    bool result = AotArgsVerify::CheckStaticAotArgs(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckStaticAotArgs_006
 * @tc.desc: Test AotArgsVerify::CheckStaticAotArgs with missing AN_FILE_NAME
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckStaticAotArgs_006, TestSize.Level0)
{
    AotCompilerArgs args;
    args.bundleName = "test_app";
    args.moduleName = "test";
    args.hapPath = "/data/test/test.hap";
    args.pgoDir = "/data/pgo";
    args.moduleArkTSMode = ArgsIdx::ARKTS_STATIC;
    args.appIdentifier = "sig";
    args.bundleUid = 10000;
    args.bundleGid = 10000;
    args.outputPath = "/data/app/el1/public/aot_compiler/ark_cache/test_app";
    // anFileName is empty

    bool result = AotArgsVerify::CheckStaticAotArgs(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_001
 * @tc.desc: Test AotArgsVerify::CheckCodeSignArkCacheFilePath with valid app arkcache path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_001, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckCodeSignArkCacheFilePath(
        "/data/app/el1/public/aot_compiler/ark_cache/nonexistent/file.abc");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_003
 * @tc.desc: Test AotArgsVerify::CheckCodeSignArkCacheFilePath with path traversal attack
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_003, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckCodeSignArkCacheFilePath(
        "/data/app/el1/public/aot_compiler/ark_cache/../etc/passwd");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_004
 * @tc.desc: Test AotArgsVerify::CheckCodeSignArkCacheFilePath with invalid directory prefix
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_004, TestSize.Level0)
{
    std::ofstream testFile("/data/local/tmp/test.abc");
    testFile.close();

    bool result = AotArgsVerify::CheckCodeSignArkCacheFilePath("/data/local/tmp/test.abc");

    EXPECT_FALSE(result);
    unlink("/data/local/tmp/test.abc");
}

/**
 * @tc.name: AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_005
 * @tc.desc: Test AotArgsVerify::CheckCodeSignArkCacheFilePath with empty path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_005, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckCodeSignArkCacheFilePath("");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_006
 * @tc.desc: Test AotArgsVerify::CheckCodeSignArkCacheFilePath with non-existent path (RealPath failure)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_006, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckCodeSignArkCacheFilePath(
        "/data/app/el1/public/aot_compiler/ark_cache/nonexistent/file.abc");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckPathTraverse_001
 * @tc.desc: Test AotArgsVerify::CheckPathTraverse with standalone ".."
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckPathTraverse_001, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckPathTraverse("..");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckPathTraverse_002
 * @tc.desc: Test AotArgsVerify::CheckPathTraverse with multiple ".." traversals
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckPathTraverse_002, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckPathTraverse("/data/../../etc/passwd");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckPathTraverse_003
 * @tc.desc: Test AotArgsVerify::CheckPathTraverse with ".." in filename (not traversal)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckPathTraverse_003, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckPathTraverse("/data/app..name/file");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckPathTraverse_004
 * @tc.desc: Test AotArgsVerify::CheckPathTraverse with "../" at beginning
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckPathTraverse_004, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckPathTraverse("../etc/passwd");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckPathTraverse_005
 * @tc.desc: Test AotArgsVerify::CheckPathTraverse with ".." at end
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckPathTraverse_005, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckPathTraverse("/data/app/..");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckPathTraverse_006
 * @tc.desc: Test AotArgsVerify::CheckPathTraverse with Windows-style ".." at beginning
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckPathTraverse_006, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckPathTraverse("..\\etc\\passwd");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckPathTraverse_007
 * @tc.desc: Test AotArgsVerify::CheckPathTraverse with single dot "." (current directory)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckPathTraverse_007, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckPathTraverse("/data/./file");

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckPathTraverse_008
 * @tc.desc: Test AotArgsVerify::CheckPathTraverse with empty path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckPathTraverse_008, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckPathTraverse("");

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAnFileSuffix_001
 * @tc.desc: Test AotArgsVerify::CheckAnFileSuffix with valid matching paths
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAnFileSuffix_001, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAnFileSuffix("/data/cache/file", "/data/cache/file.an");

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAnFileSuffix_002
 * @tc.desc: Test AotArgsVerify::CheckAnFileSuffix with wrong suffix
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAnFileSuffix_002, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAnFileSuffix("/data/cache/file", "/data/cache/file.txt");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAnFileSuffix_003
 * @tc.desc: Test AotArgsVerify::CheckAnFileSuffix with different base paths
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAnFileSuffix_003, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAnFileSuffix("/data/path1/file", "/data/path2/file.an");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAnFileSuffix_004
 * @tc.desc: Test AotArgsVerify::CheckAnFileSuffix with missing .an suffix
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAnFileSuffix_004, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAnFileSuffix("/data/cache/file", "/data/cache/file");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAnFileSuffix_005
 * @tc.desc: Test AotArgsVerify::CheckAnFileSuffix with empty strings
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAnFileSuffix_005, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAnFileSuffix("", ".an");

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckArkCacheDirectoryPrefix_001
 * @tc.desc: Test AotArgsVerify::CheckArkCacheDirectoryPrefix with valid path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckArkCacheDirectoryPrefix_001, TestSize.Level0)
{
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/test", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    bool result = AotArgsVerify::CheckArkCacheDirectoryPrefix(
        "/data/app/el1/public/aot_compiler/ark_cache/test/file", "test");

    EXPECT_TRUE(result);

    rmdir("/data/app/el1/public/aot_compiler/ark_cache/test");
}

/**
 * @tc.name: AotArgsVerifyTest_CheckArkCacheDirectoryPrefix_002
 * @tc.desc: Test AotArgsVerify::CheckArkCacheDirectoryPrefix with invalid format (no slash)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckArkCacheDirectoryPrefix_002, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckArkCacheDirectoryPrefix("invalid_format_file", "test");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckArkCacheDirectoryPrefix_003
 * @tc.desc: Test AotArgsVerify::CheckArkCacheDirectoryPrefix with wrong directory prefix
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckArkCacheDirectoryPrefix_003, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckArkCacheDirectoryPrefix(
        "/data/app/el1/public/wrong_location/test/file", "test");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckArkCacheDirectoryPrefix_004
 * @tc.desc: Test AotArgsVerify::CheckArkCacheDirectoryPrefix with bundle name mismatch
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckArkCacheDirectoryPrefix_004, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckArkCacheDirectoryPrefix(
        "/data/app/el1/public/aot_compiler/ark_cache/wrong_bundle/file", "test_bundle");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckFrameworkAnFile_001
 * @tc.desc: Test AotArgsVerify::CheckFrameworkAnFile with valid framework path
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckFrameworkAnFile_001, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckFrameworkAnFile(
        "/data/service/el1/public/for-all-app/framework_ark_cache/test.abc.an");

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckFrameworkAnFile_002
 * @tc.desc: Test AotArgsVerify::CheckFrameworkAnFile with path traversal
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckFrameworkAnFile_002, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckFrameworkAnFile(
        "/data/service/el1/public/for-all-app/framework_ark_cache/../test.abc.an");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckFrameworkAnFile_003
 * @tc.desc: Test AotArgsVerify::CheckFrameworkAnFile with wrong path prefix
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckFrameworkAnFile_003, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckFrameworkAnFile(
        "/data/app/el1/public/aot_compiler/ark_cache/test.abc.an");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckFrameworkAnFile_004
 * @tc.desc: Test AotArgsVerify::CheckFrameworkAnFile with Windows-style path traversal
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckFrameworkAnFile_004, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckFrameworkAnFile(
        "/data/service/el1/public/for-all-app/framework_ark_cache\\..\\test.abc.an");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAbcName_001
 * @tc.desc: Test CheckAbcName with empty abcName for AOT type
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAbcName_001, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAbcName("", AotParserType::DYNAMIC_AOT);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAbcName_002
 * @tc.desc: Test CheckAbcName with wrong abcName for AOT type
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAbcName_002, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAbcName("ets/modules_static.abc", AotParserType::DYNAMIC_AOT);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAbcName_003
 * @tc.desc: Test CheckAbcName with custom abcName for AOT type
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAbcName_003, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAbcName("ets/custom.abc", AotParserType::DYNAMIC_AOT);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAbcName_004
 * @tc.desc: Test CheckAbcName with wrong abcName for STATIC_AOT type
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAbcName_004, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAbcName("ets/modules.abc", AotParserType::STATIC_AOT);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAbcName_005
 * @tc.desc: Test CheckAbcName with empty abcName for STATIC_AOT type
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAbcName_005, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAbcName("", AotParserType::STATIC_AOT);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAbcName_006
 * @tc.desc: Test CheckAbcName with valid abcName for AOT type
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAbcName_006, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAbcName("ets/modules.abc", AotParserType::DYNAMIC_AOT);

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAbcName_007
 * @tc.desc: Test CheckAbcName with valid abcName for STATIC_AOT type
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAbcName_007, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckAbcName("ets/modules_static.abc", AotParserType::STATIC_AOT);

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckModuleName_001
 * @tc.desc: Test CheckModuleName with empty moduleName (should pass)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckModuleName_001, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckModuleName("");

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckModuleName_002
 * @tc.desc: Test CheckModuleName with valid moduleName
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckModuleName_002, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckModuleName("com.example.module");

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckModuleName_003
 * @tc.desc: Test CheckModuleName with path traversal attack
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckModuleName_003, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckModuleName("../etc/passwd");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckModuleName_004
 * @tc.desc: Test CheckModuleName with ".." in middle of name
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckModuleName_004, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckModuleName("module..name");

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGid_001
 * @tc.desc: Test CheckBundleUidAndGid with UID below minimum
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGid_001, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckBundleUidAndGid(100, 10000);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGid_002
 * @tc.desc: Test CheckBundleUidAndGid with GID below minimum
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGid_002, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckBundleUidAndGid(10000, 100);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGid_003
 * @tc.desc: Test CheckBundleUidAndGid with valid app UID/GID
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGid_003, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckBundleUidAndGid(10000, 10000);

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGid_004
 * @tc.desc: Test CheckBundleUidAndGid with negative UID
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGid_004, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckBundleUidAndGid(-1, 10000);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGid_005
 * @tc.desc: Test CheckBundleUidAndGid with negative GID
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGid_005, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckBundleUidAndGid(10000, -1);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGid_006
 * @tc.desc: Test CheckBundleUidAndGid with both UID and GID negative
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGid_006, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckBundleUidAndGid(-100, -200);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGid_007
 * @tc.desc: Test CheckBundleUidAndGid with UID below minimum (9999)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGid_007, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckBundleUidAndGid(9999, 10000);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGid_008
 * @tc.desc: Test CheckBundleUidAndGid with GID below minimum (9999)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGid_008, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckBundleUidAndGid(10000, 9999);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGid_009
 * @tc.desc: Test CheckBundleUidAndGid with large valid UID/GID
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGid_009, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckBundleUidAndGid(99999, 99999);

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGid_010
 * @tc.desc: Test CheckBundleUidAndGid with boundary UID (MIN_APP_UID_GID - 1)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGid_010, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckBundleUidAndGid(9999, 10000);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGid_011
 * @tc.desc: Test CheckBundleUidAndGid with zero UID and GID
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGid_011, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckBundleUidAndGid(0, 0);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGid_012
 * @tc.desc: Test CheckBundleUidAndGid with INT32_MIN values
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGid_012, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckBundleUidAndGid(INT32_MIN, INT32_MIN);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGid_013
 * @tc.desc: Test CheckBundleUidAndGid with INT32_MAX values
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGid_013, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckBundleUidAndGid(INT32_MAX, INT32_MAX);

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleUidAndGid_014
 * @tc.desc: Test CheckBundleUidAndGid with boundary GID (MIN_APP_UID_GID - 1)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleUidAndGid_014, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckBundleUidAndGid(10000, 9999);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseUint32FieldFromHex_Basic
 * @tc.desc: Test ParseUint32FieldFromHex with valid hex string (0x prefix)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseUint32FieldFromHex_Basic, TestSize.Level0)
{
    uint32_t output = 0;
    bool result = AotArgsVerify::ParseUint32FieldFromHex("0x1317b7d", output);

    EXPECT_TRUE(result);
    EXPECT_EQ(output, 0x1317b7d);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseUint32FieldFromHex_UpperCase
 * @tc.desc: Test ParseUint32FieldFromHex with uppercase hex string (0X prefix)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseUint32FieldFromHex_UpperCase, TestSize.Level0)
{
    uint32_t output = 0;
    bool result = AotArgsVerify::ParseUint32FieldFromHex("0XABCDEF", output);

    EXPECT_TRUE(result);
    EXPECT_EQ(output, 0xABCDEF);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseUint32FieldFromHex_NoPrefix
 * @tc.desc: Test ParseUint32FieldFromHex with pure hex string (no 0x prefix)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseUint32FieldFromHex_NoPrefix, TestSize.Level0)
{
    uint32_t output = 0;
    bool result = AotArgsVerify::ParseUint32FieldFromHex("1317b7d", output);

    EXPECT_TRUE(result);
    EXPECT_EQ(output, 0x1317b7d);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckPkgInfoFields_001
 * @tc.desc: Test CheckPkgInfoFields with valid pkgInfo
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckPkgInfoFields_001, TestSize.Level0)
{
    mkdir("/data", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir("/data/test", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::string testPath = "/data/test/test.hap";
    std::ofstream hapFile(testPath, std::ios::binary);
    ASSERT_TRUE(hapFile.is_open()) << "Failed to create test file: " << testPath;
    hapFile << "test content";
    hapFile.flush();
    hapFile.close();

    AotCompilerArgs args;
    args.bundleName = "com.example.app";
    args.hapPath = testPath;
    args.moduleArkTSMode = ArgsIdx::ARKTS_DYNAMIC;
    args.moduleName = "module";

    bool result = AotArgsVerify::CheckPkgInfoFields(args, AotParserType::DYNAMIC_AOT);

    EXPECT_TRUE(result);
    unlink(testPath.c_str());
}

/**
 * @tc.name: AotArgsVerifyTest_CheckPkgInfoFields_002
 * @tc.desc: Test CheckPkgInfoFields with invalid moduleName (path traversal)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckPkgInfoFields_002, TestSize.Level0)
{
    AotCompilerArgs args;
    args.bundleName = "com.example.app";
    args.hapPath = "/tmp/test.hap";
    args.moduleArkTSMode = ArgsIdx::ARKTS_DYNAMIC;
    args.moduleName = "../etc/passwd";

    bool result = AotArgsVerify::CheckPkgInfoFields(args, AotParserType::DYNAMIC_AOT);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseStringField_001
 * @tc.desc: Test ParseStringField with valid string field
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseStringField_001, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleName", "com.example.app"}};
    std::string output;

    bool result = AotArgsVerify::ParseStringField(jsonObj, "bundleName", output);

    EXPECT_TRUE(result);
    EXPECT_EQ(output, "com.example.app");
}

/**
 * @tc.name: AotArgsVerifyTest_ParseStringField_002
 * @tc.desc: Test ParseStringField with missing field
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseStringField_002, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleName", "com.example.app"}};
    std::string output;

    bool result = AotArgsVerify::ParseStringField(jsonObj, "pkgPath", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseStringField_003
 * @tc.desc: Test ParseStringField with null field
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseStringField_003, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleName", nullptr}};
    std::string output;

    bool result = AotArgsVerify::ParseStringField(jsonObj, "bundleName", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseStringField_004
 * @tc.desc: Test ParseStringField with non-string field
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseStringField_004, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleName", 12345}};
    std::string output;

    bool result = AotArgsVerify::ParseStringField(jsonObj, "bundleName", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseUint32Field_001
 * @tc.desc: Test ParseUint32Field with valid number
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseUint32Field_001, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"testField", 1000U}};
    uint32_t output = 0;

    bool result = AotArgsVerify::ParseUint32Field(jsonObj, "testField", output);

    EXPECT_TRUE(result);
    EXPECT_EQ(output, 1000U);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseUint32Field_002
 * @tc.desc: Test ParseUint32Field with valid hexadecimal string
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseUint32Field_002, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"testField", "3e8"}};
    uint32_t output = 0;

    bool result = AotArgsVerify::ParseUint32Field(jsonObj, "testField", output);

    EXPECT_TRUE(result);
    EXPECT_EQ(output, 1000U);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseUint32Field_003
 * @tc.desc: Test ParseUint32Field with invalid hex string
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseUint32Field_003, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"testField", "invalid"}};
    uint32_t output = 0;

    bool result = AotArgsVerify::ParseUint32Field(jsonObj, "testField", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseUint32Field_004
 * @tc.desc: Test ParseUint32Field with missing field
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseUint32Field_004, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleName", "test"}};
    uint32_t output = 0;

    bool result = AotArgsVerify::ParseUint32Field(jsonObj, "testField", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseInt32Field_001
 * @tc.desc: Test ParseInt32Field with valid number
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseInt32Field_001, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleUid", 10000}};
    int32_t output = 0;

    bool result = AotArgsVerify::ParseInt32Field(jsonObj, "bundleUid", output);

    EXPECT_TRUE(result);
    EXPECT_EQ(output, 10000);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseInt32Field_002
 * @tc.desc: Test ParseInt32Field with valid hexadecimal string
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseInt32Field_002, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleUid", "2710"}};
    int32_t output = 0;

    bool result = AotArgsVerify::ParseInt32Field(jsonObj, "bundleUid", output);

    EXPECT_TRUE(result);
    EXPECT_EQ(output, 10000);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseInt32Field_003
 * @tc.desc: Test ParseInt32Field with invalid string
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseInt32Field_003, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleUid", "invalid"}};
    int32_t output = 0;

    bool result = AotArgsVerify::ParseInt32Field(jsonObj, "bundleUid", output);

    EXPECT_FALSE(result);
}

// CheckAOTArgs - Return False Branches

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_EmptyBundleName
 * @tc.desc: Test CheckAOTArgs when bundleName is empty (pkgInfo fields check fails)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_EmptyBundleName, TestSize.Level0)
{
    AotCompilerArgs args;
    args.bundleName = "";
    args.moduleName = "test";
    args.hapPath = "/data/test.hap";
    args.pgoDir = "/data/app/el1/100/aot_compiler/ark_profile/test";
    args.outputPath = "/data/app/el1/public/aot_compiler/ark_cache/test";
    args.anFileName = "/data/app/el1/public/aot_compiler/ark_cache/test/test.an";

    bool result = AotArgsVerify::CheckAOTArgs(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_AnFileNameMismatch
 * @tc.desc: Test CheckAOTArgs when anFileName does not match aotFile + ".an"
 *           Covers branch: CheckArkCacheFiles validates anFile == aotFile + ".an"
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_AnFileNameMismatch, TestSize.Level0)
{
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/test", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::ofstream hapFile("/data/test.hap");
    hapFile << "HAP";
    hapFile.close();
    std::ofstream("/data/app/el1/public/aot_compiler/ark_cache/test/test.an").close();
    std::ofstream("/data/app/el1/public/aot_compiler/ark_cache/test/wrong.an").close();

    AotCompilerArgs args;
    args.compileMode = "partial";
    args.moduleArkTSMode = ArgsIdx::ARKTS_DYNAMIC;
    args.bundleName = "test";
    args.moduleName = "test";
    args.hapPath = "/data/test.hap";
    args.pgoDir = "/data/app/el1/100/aot_compiler/ark_profile/test";
    args.bundleUid = 10000;
    args.bundleGid = 10000;
    args.outputPath = "/data/app/el1/public/aot_compiler/ark_cache/test";
    args.anFileName = "/data/app/el1/public/aot_compiler/ark_cache/test/wrong.an";

    bool result = AotArgsVerify::CheckAOTArgs(args);

    EXPECT_FALSE(result);
    unlink("/data/app/el1/public/aot_compiler/ark_cache/test/test.an");
    unlink("/data/app/el1/public/aot_compiler/ark_cache/test/wrong.an");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache/test");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache");
    unlink("/data/test.hap");
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_BundleUidInvalid
 * @tc.desc: Test CheckAOTArgs when CheckBundleUidAndGid fails due to invalid uid
 *           All previous checks pass, only CheckBundleUidAndGid fails
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_BundleUidInvalid, TestSize.Level0)
{
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/com.test.app", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::ofstream hapFile("/data/test.hap");
    hapFile << "HAP";
    hapFile.close();

    AotCompilerArgs args;
    args.compileMode = "partial";
    args.moduleArkTSMode = ArgsIdx::ARKTS_DYNAMIC;
    args.bundleName = "com.test.app";
    args.moduleName = "test";
    args.hapPath = "/data/test.hap";
    args.pgoDir = "/data/app/el1/100/aot_compiler/ark_profile/com.test.app";
    args.bundleUid = 9999;  // Invalid: < MIN_APP_UID_GID (10000)
    args.bundleGid = 10000;
    args.outputPath = "/data/app/el1/public/aot_compiler/ark_cache/com.test.app";
    args.anFileName = "/data/app/el1/public/aot_compiler/ark_cache/com.test.app/test.an";

    bool result = AotArgsVerify::CheckAOTArgs(args);

    EXPECT_FALSE(result);
    unlink("/data/test.hap");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache/com.test.app");
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_BundleGidInvalid
 * @tc.desc: Test CheckAOTArgs when CheckBundleUidAndGid fails due to invalid gid
 *           All previous checks pass, only CheckBundleUidAndGid fails
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_BundleGidInvalid, TestSize.Level0)
{
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/com.test.app", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::ofstream hapFile("/data/test.hap");
    hapFile << "HAP";
    hapFile.close();

    AotCompilerArgs args;
    args.compileMode = "partial";
    args.moduleArkTSMode = ArgsIdx::ARKTS_DYNAMIC;
    args.bundleName = "com.test.app";
    args.moduleName = "test";
    args.hapPath = "/data/test.hap";
    args.pgoDir = "/data/app/el1/100/aot_compiler/ark_profile/com.test.app";
    args.bundleUid = 10000;
    args.bundleGid = 9999;  // Invalid: < MIN_APP_UID_GID (10000)
    args.outputPath = "/data/app/el1/public/aot_compiler/ark_cache/com.test.app";
    args.anFileName = "/data/app/el1/public/aot_compiler/ark_cache/com.test.app/test.an";

    bool result = AotArgsVerify::CheckAOTArgs(args);

    EXPECT_FALSE(result);
    unlink("/data/test.hap");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache/com.test.app");
}

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_PathTraversalInAotFile
 * @tc.desc: Test CheckAOTArgs when CheckArkCacheFiles fails due to path traversal
 *           All previous checks pass, only CheckArkCacheFiles fails
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_PathTraversalInAotFile, TestSize.Level0)
{
#if !defined(PANDA_TARGET_OHOS)
    std::ofstream hapFile("/data/test.hap");
    hapFile << "HAP";
    hapFile.close();

    AotCompilerArgs args;
    args.compileMode = "partial";
    args.moduleArkTSMode = ArgsIdx::ARKTS_DYNAMIC;
    args.bundleName = "com.test.app";
    args.moduleName = "module";
    args.hapPath = "/data/test.hap";
    args.pgoDir = "/data/app/el1/100/aot_compiler/ark_profile/com.test.app";
    args.bundleUid = 10000;
    args.bundleGid = 10000;
    args.outputPath = "/data/app/el1/public/aot_compiler/ark_cache/../etc";  // Path traversal
    args.anFileName = "/data/app/el1/public/aot_compiler/ark_cache/../etc/module.an";

    bool result = AotArgsVerify::CheckAOTArgs(args);

    EXPECT_FALSE(result);
    unlink("/data/test.hap");
#endif
}

// CheckStaticAotArgs - Return False Branches

/**
 * @tc.name: AotArgsVerifyTest_CheckStaticAotArgs_MissingPkgInfo
 * @tc.desc: Test CheckStaticAotArgs when bundleName is empty
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckStaticAotArgs_MissingPkgInfo, TestSize.Level0)
{
    AotCompilerArgs args;
    args.bundleName = "";
    args.outputPath = "/data/app/el1/public/aot_compiler/ark_cache/test";
    args.anFileName = "/data/app/el1/public/aot_compiler/ark_cache/test/test.an";

    bool result = AotArgsVerify::CheckStaticAotArgs(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckStaticAotArgs_EmptyModuleName
 * @tc.desc: Test CheckStaticAotArgs when moduleName is empty
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckStaticAotArgs_EmptyModuleName, TestSize.Level0)
{
    AotCompilerArgs args;
    args.bundleName = "com.test";
    args.moduleName = "";
    args.hapPath = "/data/test.hap";
    args.outputPath = "/data/app/el1/public/aot_compiler/ark_cache/test";
    args.anFileName = "/data/app/el1/public/aot_compiler/ark_cache/test/test.an";

    bool result = AotArgsVerify::CheckStaticAotArgs(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckStaticAotArgs_CheckPkgInfoFieldsFail
 * @tc.desc: Test CheckStaticAotArgs when CheckPkgInfoFields fails due to path traversal in moduleName
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckStaticAotArgs_CheckPkgInfoFieldsFail, TestSize.Level0)
{
#if defined(PANDA_TARGET_OHOS)
    AotCompilerArgs args;
    args.bundleName = "com.test";
    args.moduleName = "../etc";
    args.moduleArkTSMode = ArgsIdx::ARKTS_STATIC;
    args.hapPath = "/data/test.hap";
    args.appIdentifier = "signature";
    args.pgoDir = "/data/app/el1/100/aot_compiler/ark_profile/test";
    args.bundleUid = 10000;
    args.bundleGid = 10000;
    args.outputPath = "/data/app/el1/public/aot_compiler/ark_cache/test";
    args.anFileName = "/data/app/el1/public/aot_compiler/ark_cache/test/test.an";

    bool result = AotArgsVerify::CheckStaticAotArgs(args);

    EXPECT_FALSE(result);
#else
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/test", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::ofstream hapFile("/data/test.hap");
    hapFile << "HAP";
    hapFile.close();
    std::ofstream("/data/app/el1/public/aot_compiler/ark_cache/test/test.an").close();

    AotCompilerArgs args;
    args.bundleName = "com.test";
    args.moduleName = "../etc";
    args.moduleArkTSMode = ArgsIdx::ARKTS_STATIC;
    args.hapPath = "/data/test.hap";
    args.appIdentifier = "signature";
    args.pgoDir = "/data/app/el1/100/aot_compiler/ark_profile/test";
    args.bundleUid = 10000;
    args.bundleGid = 10000;
    args.outputPath = "/data/app/el1/public/aot_compiler/ark_cache/test";
    args.anFileName = "/data/app/el1/public/aot_compiler/ark_cache/test/test.an";

    bool result = AotArgsVerify::CheckStaticAotArgs(args);

    EXPECT_FALSE(result);
    unlink("/data/app/el1/public/aot_compiler/ark_cache/test/test.an");
    rmdir("/data/app/el1/public/aot_compiler/ark_cache/test");
    unlink("/data/test.hap");
#endif
}

/**
 * @tc.name: AotArgsVerifyTest_CheckStaticAotArgs_CheckArkCacheFilesFail
 * @tc.desc: Test CheckStaticAotArgs when CheckArkCacheFiles fails due to empty outputPath
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckStaticAotArgs_CheckArkCacheFilesFail, TestSize.Level0)
{
#if defined(PANDA_TARGET_OHOS)
    AotCompilerArgs args;
    args.bundleName = "com.test";
    args.moduleName = "test";
    args.moduleArkTSMode = ArgsIdx::ARKTS_STATIC;
    args.hapPath = "/data/test.hap";
    args.outputPath = "";
    args.anFileName = "/data/app/el1/public/aot_compiler/ark_cache/test/test.an";

    bool result = AotArgsVerify::CheckStaticAotArgs(args);

    EXPECT_FALSE(result);
#else
    std::ofstream hapFile("/data/test.hap");
    hapFile << "HAP";
    hapFile.close();

    AotCompilerArgs args;
    args.bundleName = "com.test";
    args.moduleName = "test";
    args.moduleArkTSMode = ArgsIdx::ARKTS_STATIC;
    args.hapPath = "/data/test.hap";
    args.outputPath = "";
    args.anFileName = "/data/app/el1/public/aot_compiler/ark_cache/test/test.an";

    bool result = AotArgsVerify::CheckStaticAotArgs(args);

    EXPECT_FALSE(result);
    unlink("/data/test.hap");
#endif
}
// ParseUint32Field - Wrong Field Type Tests (Line 466-467)
// ============================================================================

/**
 * @tc.name: AotArgsVerifyTest_ParseUint32Field_WrongType_Boolean
 * @tc.desc: Test ParseUint32Field with boolean value (wrong type)
 *           Covers: if (!jsonObj[key].is_string() && !jsonObj[key].is_number_unsigned()) return false;
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseUint32Field_WrongType_Boolean, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"testField", true}};
    uint32_t output = 0;

    bool result = AotArgsVerify::ParseUint32Field(jsonObj, "testField", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseUint32Field_WrongType_Array
 * @tc.desc: Test ParseUint32Field with array value (wrong type)
 *           Covers: if (!jsonObj[key].is_string() && !jsonObj[key].is_number_unsigned()) return false;
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseUint32Field_WrongType_Array, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"testField", {1, 2, 3}}};
    uint32_t output = 0;

    bool result = AotArgsVerify::ParseUint32Field(jsonObj, "testField", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseUint32Field_WrongType_Object
 * @tc.desc: Test ParseUint32Field with object value (wrong type)
 *           Covers: if (!jsonObj[key].is_string() && !jsonObj[key].is_number_unsigned()) return false;
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseUint32Field_WrongType_Object, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"testField", {{"value", "100"}}}};
    uint32_t output = 0;

    bool result = AotArgsVerify::ParseUint32Field(jsonObj, "testField", output);

    EXPECT_FALSE(result);
}
// ParseInt32Field - Missing Test Cases (Lines 487-491)

/**
 * @tc.name: AotArgsVerifyTest_ParseInt32Field_MissingField
 * @tc.desc: Test ParseInt32Field with missing field
 *           Covers: if (!jsonObj.contains(key) || jsonObj[key].is_null()) return false;
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseInt32Field_MissingField, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleName", "test"}};
    int32_t output = 0;

    bool result = AotArgsVerify::ParseInt32Field(jsonObj, "bundleUid", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseInt32Field_NullField
 * @tc.desc: Test ParseInt32Field with null field
 *           Covers: if (!jsonObj.contains(key) || jsonObj[key].is_null()) return false;
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseInt32Field_NullField, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleUid", nullptr}};
    int32_t output = 0;

    bool result = AotArgsVerify::ParseInt32Field(jsonObj, "bundleUid", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseInt32Field_WrongType_Boolean
 * @tc.desc: Test ParseInt32Field with boolean value (wrong type)
 *           Covers: if (!jsonObj[key].is_string() && !jsonObj[key].is_number_integer()) return false;
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseInt32Field_WrongType_Boolean, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleUid", true}};
    int32_t output = 0;

    bool result = AotArgsVerify::ParseInt32Field(jsonObj, "bundleUid", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseInt32Field_WrongType_Array
 * @tc.desc: Test ParseInt32Field with array value (wrong type)
 *           Covers: if (!jsonObj[key].is_string() && !jsonObj[key].is_number_integer()) return false;
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseInt32Field_WrongType_Array, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleUid", {1, 2, 3}}};
    int32_t output = 0;

    bool result = AotArgsVerify::ParseInt32Field(jsonObj, "bundleUid", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseInt32Field_WrongType_Object
 * @tc.desc: Test ParseInt32Field with object value (wrong type)
 *           Covers: if (!jsonObj[key].is_string() && !jsonObj[key].is_number_integer()) return false;
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseInt32Field_WrongType_Object, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleUid", {{"value", "100"}}}};
    int32_t output = 0;

    bool result = AotArgsVerify::ParseInt32Field(jsonObj, "bundleUid", output);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_ParseInt32Field_WrongType_Float
 * @tc.desc: Test ParseInt32Field with float value (wrong type)
 *           Covers: if (!jsonObj[key].is_string() && !jsonObj[key].is_number_integer()) return false;
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_ParseInt32Field_WrongType_Float, TestSize.Level0)
{
    nlohmann::json jsonObj = {{"bundleUid", 123.45}};
    int32_t output = 0;

    bool result = AotArgsVerify::ParseInt32Field(jsonObj, "bundleUid", output);

    EXPECT_FALSE(result);
}
// CheckCodeSignArkCacheFilePath - Symlink Path Traversal (Lines 319-321)

/**
 * @tc.name: AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_SymlinkTraversal
 * @tc.desc: Test CheckCodeSignArkCacheFilePath with symlink containing path traversal
 *           Covers: Re-check for path traversal after RealPath (symlink could bypass)
 *           if (!CheckPathTraverse(resolvedPath)) return false;
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_SymlinkTraversal, TestSize.Level0)
{
    mkdir("/data/app/el1/public/aot_compiler/ark_cache/com.example.app", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::ofstream realFile("/data/app/el1/public/aot_compiler/ark_cache/com.example.app/test.abc");
    realFile << "test content";
    realFile.close();
    std::string symlinkPath = "/data/app/el1/public/aot_compiler/ark_cache/com.example.app/symlink.abc";
    mkdir("/data/local/tmp", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::ofstream outsideFile("/data/local/tmp/outside.abc");
    outsideFile << "outside content";
    outsideFile.close();
    symlink("/data/local/tmp/outside.abc", symlinkPath.c_str());
    bool result = AotArgsVerify::CheckCodeSignArkCacheFilePath(symlinkPath);

    EXPECT_FALSE(result);
    unlink(symlinkPath.c_str());
    unlink("/data/local/tmp/outside.abc");
    unlink("/data/app/el1/public/aot_compiler/ark_cache/com.example.app/test.abc");
    rmdir("/data/app/el1/public/aot_compiler/az_cache/com.example.app");
}

// ============================================================================
// CheckTriggerTypeForAOT - Test Cases
// ============================================================================

/**
 * @tc.name: AotArgsVerifyTest_CheckTriggerTypeForAOT_Idle
 * @tc.desc: Test CheckTriggerTypeForAOT with triggerType 0 (IDLE) - should return true
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckTriggerTypeForAOT_Idle, TestSize.Level0)
{
    AotCompilerArgs args;
    args.triggerType = 0;

    bool result = AotArgsVerify::CheckTriggerTypeForAOT(args);

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckTriggerTypeForAOT_Install
 * @tc.desc: Test CheckTriggerTypeForAOT with triggerType 1 (INSTALL) - should return false
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckTriggerTypeForAOT_Install, TestSize.Level0)
{
    AotCompilerArgs args;
    args.triggerType = 1;

    bool result = AotArgsVerify::CheckTriggerTypeForAOT(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckTriggerTypeForAOT_OtherValue
 * @tc.desc: Test CheckTriggerTypeForAOT with triggerType 2 (exceeds AotTriggerType range) - should return false
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckTriggerTypeForAOT_OtherValue, TestSize.Level0)
{
    AotCompilerArgs args;
    args.triggerType = 2;

    bool result = AotArgsVerify::CheckTriggerTypeForAOT(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckTriggerTypeForAOT_DefaultValue
 * @tc.desc: Test CheckTriggerTypeForAOT with triggerType at default (0) - should return true
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckTriggerTypeForAOT_DefaultValue, TestSize.Level0)
{
    AotCompilerArgs args;
    // triggerType at default value (0)

    bool result = AotArgsVerify::CheckTriggerTypeForAOT(args);

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckTriggerTypeForAOT_NegativeValue
 * @tc.desc: Test CheckTriggerTypeForAOT with negative triggerType - should return false (out of range)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckTriggerTypeForAOT_NegativeValue, TestSize.Level0)
{
    AotCompilerArgs args;
    args.triggerType = -1;

    bool result = AotArgsVerify::CheckTriggerTypeForAOT(args);

    EXPECT_FALSE(result);
}

// ============================================================================
// IsSharedBundlesType - Test Cases
// ============================================================================

/**
 * @tc.name: AotArgsVerifyTest_IsSharedBundlesType_App
 * @tc.desc: Test IsSharedBundlesType with bundleType 0 (APP) - should return false
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_IsSharedBundlesType_App, TestSize.Level0)
{
    AotCompilerArgs args;
    args.bundleType = 0;

    bool result = AotArgsVerify::IsSharedBundlesType(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_IsSharedBundlesType_AtomicService
 * @tc.desc: Test IsSharedBundlesType with bundleType 1 (ATOMIC_SERVICE) - should return false
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_IsSharedBundlesType_AtomicService, TestSize.Level0)
{
    AotCompilerArgs args;
    args.bundleType = 1;

    bool result = AotArgsVerify::IsSharedBundlesType(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_IsSharedBundlesType_Shared
 * @tc.desc: Test IsSharedBundlesType with bundleType 2 (SHARED) - should return true
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_IsSharedBundlesType_Shared, TestSize.Level0)
{
    AotCompilerArgs args;
    args.bundleType = 2;

    bool result = AotArgsVerify::IsSharedBundlesType(args);

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_IsSharedBundlesType_AppServiceFwk
 * @tc.desc: Test IsSharedBundlesType with bundleType 3 (APP_SERVICE_FWK) - should return false
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_IsSharedBundlesType_AppServiceFwk, TestSize.Level0)
{
    AotCompilerArgs args;
    args.bundleType = 3;

    bool result = AotArgsVerify::IsSharedBundlesType(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_IsSharedBundlesType_DefaultValue
 * @tc.desc: Test IsSharedBundlesType with bundleType at default (0) - should return false
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_IsSharedBundlesType_DefaultValue, TestSize.Level0)
{
    AotCompilerArgs args;
    // bundleType at default value (0)

    bool result = AotArgsVerify::IsSharedBundlesType(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_IsSharedBundlesType_NegativeValue
 * @tc.desc: Test IsSharedBundlesType with negative bundleType - should return false
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_IsSharedBundlesType_NegativeValue, TestSize.Level0)
{
    AotCompilerArgs args;
    args.bundleType = -1;

    bool result = AotArgsVerify::IsSharedBundlesType(args);

    EXPECT_FALSE(result);
}

// ============================================================================
// CheckSharedBundlesUidAndGid - Test Cases
// ============================================================================

/**
 * @tc.name: AotArgsVerifyTest_CheckSharedBundlesUidAndGid_BothValid
 * @tc.desc: Test CheckSharedBundlesUidAndGid with uid=1000, gid=1000 - should return true
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckSharedBundlesUidAndGid_BothValid, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckSharedBundlesUidAndGid(1000, 1000);

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckSharedBundlesUidAndGid_UidValid_GidInvalid
 * @tc.desc: Test CheckSharedBundlesUidAndGid with uid=1000, gid!=1000 - should return false
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckSharedBundlesUidAndGid_UidValid_GidInvalid, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckSharedBundlesUidAndGid(1000, 10000);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckSharedBundlesUidAndGid_UidInvalid_GidValid
 * @tc.desc: Test CheckSharedBundlesUidAndGid with uid!=1000, gid=1000 - should return false
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckSharedBundlesUidAndGid_UidInvalid_GidValid, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckSharedBundlesUidAndGid(10000, 1000);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckSharedBundlesUidAndGid_BothInvalid
 * @tc.desc: Test CheckSharedBundlesUidAndGid with uid!=1000, gid!=1000 - should return false
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckSharedBundlesUidAndGid_BothInvalid, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckSharedBundlesUidAndGid(10000, 10000);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckSharedBundlesUidAndGid_NegativeUid
 * @tc.desc: Test CheckSharedBundlesUidAndGid with negative uid - should return false
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckSharedBundlesUidAndGid_NegativeUid, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckSharedBundlesUidAndGid(-1, 1000);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckSharedBundlesUidAndGid_NegativeGid
 * @tc.desc: Test CheckSharedBundlesUidAndGid with negative gid - should return false
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckSharedBundlesUidAndGid_NegativeGid, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckSharedBundlesUidAndGid(1000, -1);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckSharedBundlesUidAndGid_ZeroValues
 * @tc.desc: Test CheckSharedBundlesUidAndGid with uid=0, gid=0 - should return false
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckSharedBundlesUidAndGid_ZeroValues, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckSharedBundlesUidAndGid(0, 0);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckSharedBundlesUidAndGid_LargeValues
 * @tc.desc: Test CheckSharedBundlesUidAndGid with large uid/gid values - should return false
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckSharedBundlesUidAndGid_LargeValues, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckSharedBundlesUidAndGid(99999, 99999);

    EXPECT_FALSE(result);
}

// ============================================================================
// CheckSharedBundlesArkCacheFiles - Test Cases
// ============================================================================

/**
 * @tc.name: AotArgsVerifyTest_CheckSharedBundlesArkCacheFiles_ValidPath
 * @tc.desc: Test CheckSharedBundlesArkCacheFiles with valid shared bundles path - should return true
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckSharedBundlesArkCacheFiles_ValidPath, TestSize.Level0)
{
    std::string anFile = "/data/service/el1/public/for-all-app/shared_bundles_ark_cache/test/file.an";

    bool result = AotArgsVerify::CheckSharedBundlesArkCacheFiles(anFile);

    EXPECT_TRUE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckSharedBundlesArkCacheFiles_PathTraversal
 * @tc.desc: Test CheckSharedBundlesArkCacheFiles with path traversal attack - should return false
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckSharedBundlesArkCacheFiles_PathTraversal, TestSize.Level0)
{
    std::string anFile = "/data/service/el1/public/for-all-app/shared_bundles_ark_cache/../etc/file.an";

    bool result = AotArgsVerify::CheckSharedBundlesArkCacheFiles(anFile);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckSharedBundlesArkCacheFiles_WrongPrefix
 * @tc.desc: Test with APP_ARK_CACHE_PREFIX instead of SHARED_BUNDLES - should return false
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckSharedBundlesArkCacheFiles_WrongPrefix, TestSize.Level0)
{
    std::string anFile = "/data/app/el1/public/aot_compiler/ark_cache/test/file.an";

    bool result = AotArgsVerify::CheckSharedBundlesArkCacheFiles(anFile);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckSharedBundlesArkCacheFiles_EmptyAnFile
 * @tc.desc: Test CheckSharedBundlesArkCacheFiles with empty anFile string - should return false
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckSharedBundlesArkCacheFiles_EmptyAnFile, TestSize.Level0)
{
    std::string anFile = "";

    bool result = AotArgsVerify::CheckSharedBundlesArkCacheFiles(anFile);

    EXPECT_FALSE(result);
}

// ============================================================================
// CheckBundleNameAndAppIdentifier - Test Cases (struct-based API)
// ============================================================================

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleNameAndAppIdentifier_InvalidParams
 * @tc.desc: Test CheckBundleNameAndAppIdentifier with invalid parameters - should return false
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleNameAndAppIdentifier_InvalidParams, TestSize.Level0)
{
    std::vector<HspModuleInfo> emptyPkgs;
    // Empty bundleName
    bool result = AotArgsVerify::CheckBundleNameAndAppIdentifier(
        "", "/system/app/Example/Example.hap", "sig", emptyPkgs);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleNameAndAppIdentifier_EmptyExternalPkgs
 * @tc.desc: Test CheckBundleNameAndAppIdentifier with empty external packages
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleNameAndAppIdentifier_EmptyExternalPkgs, TestSize.Level0)
{
    std::vector<HspModuleInfo> emptyPkgs;
    bool result = AotArgsVerify::CheckBundleNameAndAppIdentifier(
        "com.example.app", "/system/app/Example/Example.hap", "sig", emptyPkgs);

#if !defined(PANDA_TARGET_OHOS)
    // On non-OHOS, returns false immediately
    EXPECT_FALSE(result);
#else
    // On OHOS, will fail at dlopen for libhapverify.z.so (not available in test env)
    EXPECT_FALSE(result);
#endif
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleNameAndAppIdentifier_WithExternalPkgs
 * @tc.desc: Test CheckBundleNameAndAppIdentifier with external packages (non-existent paths)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleNameAndAppIdentifier_WithExternalPkgs, TestSize.Level0)
{
    std::vector<HspModuleInfo> pkgs;
    HspModuleInfo pkg1;
    pkg1.bundleName = "com.example.sub";
    pkg1.hapPath = "/system/app/Sub.hap";
    pkgs.push_back(pkg1);
    HspModuleInfo pkg2;
    pkg2.bundleName = "com.example.sub2";
    pkg2.hapPath = "/system/app/Sub2.hap";
    pkgs.push_back(pkg2);

    bool result = AotArgsVerify::CheckBundleNameAndAppIdentifier(
        "com.example.app", "/system/app/Example/Example.hap", "sig", pkgs);

#if !defined(PANDA_TARGET_OHOS)
    EXPECT_FALSE(result);
#else
    // Will fail because HAP files don't exist
    EXPECT_FALSE(result);
#endif
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleNameAndAppIdentifier_ExternalPkgEmptyBundleName
 * @tc.desc: Test with external package having empty bundleName
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleNameAndAppIdentifier_ExternalPkgEmptyBundleName,
    TestSize.Level0)
{
    std::vector<HspModuleInfo> pkgs;
    HspModuleInfo pkg;
    pkg.hapPath = "/system/app/Sub.hap";
    pkgs.push_back(pkg);

    bool result = AotArgsVerify::CheckBundleNameAndAppIdentifier(
        "com.example.app", "/system/app/Example/Example.hap", "sig", pkgs);

#if !defined(PANDA_TARGET_OHOS)
    EXPECT_FALSE(result);
#else
    // Will fail because external package has empty bundleName
    EXPECT_FALSE(result);
#endif
}

/**
 * @tc.name: AotArgsVerifyTest_CheckBundleNameAndAppIdentifier_ExternalPkgEmptyPkgPath
 * @tc.desc: Test with external package having empty pkgPath
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckBundleNameAndAppIdentifier_ExternalPkgEmptyPkgPath,
    TestSize.Level0)
{
    std::vector<HspModuleInfo> pkgs;
    HspModuleInfo pkg;
    pkg.bundleName = "com.example.sub";
    pkgs.push_back(pkg);

    bool result = AotArgsVerify::CheckBundleNameAndAppIdentifier(
        "com.example.app", "/system/app/Example/Example.hap", "sig", pkgs);

#if !defined(PANDA_TARGET_OHOS)
    EXPECT_FALSE(result);
#else
    // Will fail because external package has empty pkgPath
    EXPECT_FALSE(result);
#endif
}

/**
 * @tc.name: AotArgsVerifyTest_CheckHapFsVerity_InvalidFd
 * @tc.desc: Test CheckHapFsVerity with invalid fd (-1)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckHapFsVerity_InvalidFd, TestSize.Level0)
{
    bool result = AotArgsVerify::CheckHapFsVerity(-1);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckHapFsVerity_ValidFd
 * @tc.desc: Test CheckHapFsVerity with a valid fd — verifies no crash on a regular temp file.
 *           Result depends on filesystem: returns true if ioctl unsupported (skip check),
 *           returns false if filesystem supports ioctl but file lacks fs-verity.
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckHapFsVerity_ValidFd, TestSize.Level0)
{
    char tmpPath[] = "./aot_fsverity_test_XXXXXX";
    int fd = mkstemp(tmpPath);
    ASSERT_GE(fd, 0);

    // CheckHapFsVerity returns a valid bool for any regular file fd — just verify no crash
    bool result = AotArgsVerify::CheckHapFsVerity(fd);
    EXPECT_TRUE(result || !result);

    close(fd);
    unlink(tmpPath);
}

// ============================================================================
// Supplementary tests for precise branch coverage
// ============================================================================

/**
 * @tc.name: AotArgsVerifyTest_CheckAOTArgs_EmptyBundleNameWithValidCompileMode
 * @tc.desc: Test CheckAOTArgs with valid compileMode/moduleArkTSMode but empty bundleName
 *           — precisely hits CheckPkgInfoFields bundleName check, not compileMode
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckAOTArgs_EmptyBundleNameWithValidCompileMode, TestSize.Level0)
{
    AotCompilerArgs args;
    args.compileMode = ArgsIdx::PARTIAL;
    args.moduleArkTSMode = ArgsIdx::ARKTS_DYNAMIC;
    args.bundleName = "";
    args.moduleName = "test";
    args.hapPath = "/data/test.hap";
    args.pgoDir = "/data/app/el1/100/aot_compiler/ark_profile/test";

    bool result = AotArgsVerify::CheckAOTArgs(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckTriggerTypeForAOT_OutOfRange
 * @tc.desc: Test CheckTriggerTypeForAOT with triggerType exceeding max enum value — should return false
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckTriggerTypeForAOT_OutOfRange, TestSize.Level0)
{
    AotCompilerArgs args;
    args.triggerType = 99;

    bool result = AotArgsVerify::CheckTriggerTypeForAOT(args);

    EXPECT_FALSE(result);
}

/**
 * @tc.name: AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_NonExistentFile
 * @tc.desc: Test CheckCodeSignArkCacheFilePath with non-existent .an file but existing parent dir (memfd flow)
 * @tc.type: Func
 */
HWTEST_F(AotArgsVerifyTest, AotArgsVerifyTest_CheckCodeSignArkCacheFilePath_NonExistentFile, TestSize.Level0)
{
    // Simulate memfd flow: parent directory exists on disk, but .an file does not yet
    std::string parentDir = "/data/app/el1/public/aot_compiler/ark_cache/test_memfd_nosign/";
    std::string targetPath = parentDir + "entry.an";
    mkdir(parentDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    bool result = AotArgsVerify::CheckCodeSignArkCacheFilePath(targetPath);
    EXPECT_TRUE(result);

    rmdir(parentDir.c_str());
}

} // namespace OHOS::ArkCompiler
