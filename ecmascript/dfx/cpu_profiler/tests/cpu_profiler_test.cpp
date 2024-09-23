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

#include <cstdio>

#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::ecmascript {
class CpuProfilerFriendTest {
public:
    explicit CpuProfilerFriendTest(const EcmaVM *vm, const int interval) : cpu_profiler(vm, interval) {}

    void SetIsProfiling(bool isProfiling)
    {
        cpu_profiler.isProfiling_ = isProfiling;
    }

    void SetOutToFile(bool outToFile)
    {
        cpu_profiler.outToFile_ = outToFile;
    }

    void SetInterval(uint32_t intervalTest)
    {
        cpu_profiler.interval_ = intervalTest;
    }

    bool StartCpuProfilerForInfoTest()
    {
        return cpu_profiler.StartCpuProfilerForInfo();
    }

    bool StopCpuProfilerForInfoTest(std::unique_ptr<struct ProfileInfo> &profileInfo)
    {
        return cpu_profiler.StopCpuProfilerForInfo(profileInfo);
    }

    bool StartCpuProfilerForFileTest(const std::string &fileName)
    {
        return cpu_profiler.StartCpuProfilerForFile(fileName);
    }

    bool StopCpuProfilerForFileTest()
    {
        return cpu_profiler.StopCpuProfilerForFile();
    }

    bool GetStackBeforeCallNapiTest(JSThread *thread)
    {
        return cpu_profiler.GetStackBeforeCallNapi(thread);
    }

private:
    CpuProfiler cpu_profiler;
};
}

namespace panda::test {
class CpuProfilerTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
        instance->SetEnableForceGC(false);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(CpuProfilerTest, TestStopCpuProfilerForFile)
{
    int interval = 1;
    bool ret;
    CpuProfilerFriendTest cpuProfilerFriend(instance, interval);
    ret = cpuProfilerFriend.StopCpuProfilerForFileTest();
    EXPECT_TRUE(ret);

    cpuProfilerFriend.SetIsProfiling(true);
    cpuProfilerFriend.StartCpuProfilerForInfoTest();
    ret = cpuProfilerFriend.StopCpuProfilerForFileTest();
    EXPECT_FALSE(ret);
}

HWTEST_F_L0(CpuProfilerTest, TestStartCpuProfilerForFile)
{
    int interval = 1;
    bool ret;
    CpuProfilerFriendTest cpuProfilerFriend(instance, interval);
    std::string filename1("__CpuprofilerTest.abc");
    std::string filename2("");
    std::string filename3(PATH_MAX + 1, ' ');

    ret = cpuProfilerFriend.StartCpuProfilerForFileTest(filename1);
    EXPECT_FALSE(ret);
    ret = cpuProfilerFriend.StartCpuProfilerForFileTest(filename2);
    EXPECT_FALSE(ret);
    ret = cpuProfilerFriend.StartCpuProfilerForFileTest(filename3);
    EXPECT_FALSE(ret);

    cpuProfilerFriend.SetIsProfiling(true);
    ret = cpuProfilerFriend.StartCpuProfilerForFileTest(filename1);
    EXPECT_FALSE(ret);
}

HWTEST_F_L0(CpuProfilerTest, TestStopCpuProfilerForInfo)
{
    int interval = 1;
    bool ret;
    CpuProfilerFriendTest cpuProfilerFriend(instance, interval);
    std::unique_ptr<ProfileInfo> profileInfo;

    ret = cpuProfilerFriend.StopCpuProfilerForInfoTest(profileInfo);
    EXPECT_TRUE(ret);

    cpuProfilerFriend.StartCpuProfilerForInfoTest();
    ret = cpuProfilerFriend.StopCpuProfilerForInfoTest(profileInfo);
    EXPECT_TRUE(ret);
    
    cpuProfilerFriend.SetIsProfiling(true);
    cpuProfilerFriend.SetOutToFile(true);
    ret = cpuProfilerFriend.StopCpuProfilerForInfoTest(profileInfo);
    EXPECT_FALSE(ret);
}

HWTEST_F_L0(CpuProfilerTest, TestGetStackBeforeCallNapi)
{
    uint32_t interval = 100;
    CpuProfilerFriendTest cpuProfilerFriend(instance, interval);

    bool flag = cpuProfilerFriend.GetStackBeforeCallNapiTest(thread);
    EXPECT_TRUE(flag);
    
    cpuProfilerFriend.SetInterval(UINT32_MAX);
    flag = cpuProfilerFriend.GetStackBeforeCallNapiTest(thread);
    EXPECT_FALSE(flag);
}

HWTEST_F_L0(CpuProfilerTest, TestCpuProfilerHasSameName)
{
    int interval = 500;

    bool res = DFXJSNApi::StartCpuProfilerForInfo(instance, interval);
    EXPECT_TRUE(res);
    // The cpuprofiler_1.js only support test A->B->C, not support test A->C, B->C
    std::string fileName = DEBUGGER_ABC_DIR "cpuprofiler_1.abc";
    std::string entryPoint = "cpuprofiler_1";
    res = JSNApi::Execute(instance, fileName, entryPoint);
    ASSERT_TRUE(res);
    std::unique_ptr<ProfileInfo> profileInfo = DFXJSNApi::StopCpuProfilerForInfo(instance);
    size_t nodesLen = profileInfo->nodeCount;
    std::unordered_set<std::string> functionNames;
    // Check if the function has multiple nodes
    for (size_t i = 0; i < nodesLen; ++i) {
        const auto &cpuProfileNode = profileInfo->nodes[i];
        std::string functionName = cpuProfileNode.codeEntry.functionName;
        auto node = functionNames.find(functionName);
        if (node == functionNames.end()) {
            functionNames.insert(functionName);
        } else {
            res = false;
            break;
        }
    }
    ASSERT_TRUE(res);
}
}  // namespace panda::test