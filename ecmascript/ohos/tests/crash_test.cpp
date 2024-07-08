/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include <memory>
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/ohos/aot_runtime_info.h"
#include "ecmascript/ohos/tests/mock/mock_aot_runtime_info.h"
#include "ecmascript/platform/aot_crash_info.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::kungfu;

namespace panda::test {
class CrashTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override {}
    void TearDown() override {}

    MockAotRuntimeInfo mockAotRuntimeInfo;
protected:
    EcmaVM *vm_ {nullptr};
};

HWTEST_F_L0(CrashTest, CrashGetBuildId)
{
    char soBuildId[NAME_MAX];
    ohos::AotRuntimeInfo *runtimeInfo = new MockAotRuntimeInfo();
    runtimeInfo->GetRuntimeBuildId(soBuildId);
    ASSERT_TRUE(std::string(soBuildId).size() > 0);
    ASSERT_EQ(std::string(soBuildId), "abcd1234567890");
}

HWTEST_F_L0(CrashTest, GetMicrosecondsTimeStamp)
{
    char timestamp[ohos::AotRuntimeInfo::TIME_STAMP_SIZE];
    ohos::AotRuntimeInfo *runtimeInfo = new MockAotRuntimeInfo();
    runtimeInfo->GetMicrosecondsTimeStamp(timestamp);
    ASSERT_TRUE(std::string(timestamp).size() > 0);
    ASSERT_EQ(std::string(timestamp), "1970-01-01 00:00:00");
}

HWTEST_F_L0(CrashTest, BuildCrashRuntimeInfo)
{
    char timestamp[ohos::AotRuntimeInfo::TIME_STAMP_SIZE];
    char soBuildId[NAME_MAX];
    ohos::AotRuntimeInfo *runtimeInfo = new MockAotRuntimeInfo();
    runtimeInfo->GetMicrosecondsTimeStamp(timestamp);
    runtimeInfo->GetRuntimeBuildId(soBuildId);
    char sanboxRealPath[PATH_MAX];
    mkdir(MockAotRuntimeInfo::SANBOX_DIR, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::ofstream file(sanboxRealPath);
    file.close();
    runtimeInfo->GetCrashSandBoxRealPath(sanboxRealPath);

    runtimeInfo->BuildCrashRuntimeInfo(ecmascript::ohos::RuntimeInfoType::AOT_CRASH);
    std::map<ecmascript::ohos::RuntimeInfoType, int> escapeMap = runtimeInfo->CollectCrashSum();
    ASSERT_EQ(escapeMap[ecmascript::ohos::RuntimeInfoType::AOT_CRASH], 1);
    ASSERT_EQ(runtimeInfo->GetCompileCountByType(ecmascript::ohos::RuntimeInfoType::AOT_CRASH), 1);

    unlink(sanboxRealPath);
    rmdir(MockAotRuntimeInfo::SANBOX_DIR);
}

HWTEST_F_L0(CrashTest, BuildCompileRuntimeInfo)
{
    char timestamp[ohos::AotRuntimeInfo::TIME_STAMP_SIZE];
    char soBuildId[NAME_MAX];
    ohos::AotRuntimeInfo *runtimeInfo = new MockAotRuntimeInfo();
    runtimeInfo->GetMicrosecondsTimeStamp(timestamp);
    runtimeInfo->GetRuntimeBuildId(soBuildId);
    char sanboxRealPath[PATH_MAX];
    mkdir(MockAotRuntimeInfo::SANBOX_DIR, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::ofstream file(sanboxRealPath);
    file.close();
    runtimeInfo->GetCrashSandBoxRealPath(sanboxRealPath);

    runtimeInfo->BuildCompileRuntimeInfo(ecmascript::ohos::RuntimeInfoType::AOT_CRASH, sanboxRealPath);
    std::map<ecmascript::ohos::RuntimeInfoType, int> escapeMapRealPath = runtimeInfo->CollectCrashSum(sanboxRealPath);
    ASSERT_EQ(escapeMapRealPath[ecmascript::ohos::RuntimeInfoType::AOT_CRASH], 1);
    ASSERT_EQ(runtimeInfo->GetCompileCountByType(ecmascript::ohos::RuntimeInfoType::AOT_CRASH, sanboxRealPath), 1);

    unlink(sanboxRealPath);
    rmdir(MockAotRuntimeInfo::SANBOX_DIR);
}
} // namespace panda::test