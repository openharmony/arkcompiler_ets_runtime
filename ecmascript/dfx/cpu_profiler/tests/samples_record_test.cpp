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

#include "ecmascript/dfx/cpu_profiler/samples_record.h"
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::ecmascript {
class SamplesRecordFriendTest {
public:
    SamplesRecordFriendTest() : samples_record() {}
    
    std::string AddRunningStateTest(char *functionName, RunningState state, kungfu::DeoptType type)
    {
        return samples_record.AddRunningState(functionName, state, type);
    }

    void SetEnableVMTag(bool flag)
    {
        samples_record.SetEnableVMTag(flag);
    }

    void StatisticStateTimeTest(int timeDelta, RunningState state)
    {
        samples_record.StatisticStateTime(timeDelta, state);
    }

    bool PushNapiStackInfoTest(const FrameInfoTemp &frameInfoTemp)
    {
        return samples_record.PushNapiStackInfo(frameInfoTemp);
    }

    void NapiFrameInfoTempToMapTest()
    {
        samples_record.NapiFrameInfoTempToMap();
    }

    void TranslateUrlPositionBySourceMapTest(struct FrameInfo &codeEntry)
    {
        samples_record.TranslateUrlPositionBySourceMap(codeEntry);
    }

    void sourceMapTranslateCallbackTest(SourceMapTranslateCallback sourceMapTranslateCallback_)
    {
        samples_record.sourceMapTranslateCallback_ = sourceMapTranslateCallback_;
    }

    std::unique_ptr<ProfileInfo> GetProfileInfoTest()
    {
        return std::move(samples_record.profileInfo_);
    }

private:
    SamplesRecord samples_record;
};
}

namespace panda::test {
class SamplesRecordTest : public testing::Test {
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

HWTEST_F_L0(SamplesRecordTest, AddRunningStateTest)
{
    SamplesRecordFriendTest samplesRecord;
    char funcName[] = "testFunction";
    std::string result = samplesRecord.AddRunningStateTest(funcName, RunningState::AINT_D,
        kungfu::DeoptType::NONE);
    EXPECT_EQ(result, "testFunction(AINT-D)");

    result = samplesRecord.AddRunningStateTest(funcName, RunningState::GC,
        kungfu::DeoptType::NOTDOUBLE1);
    EXPECT_EQ(result, "testFunction(GC)");

    result = samplesRecord.AddRunningStateTest(funcName, RunningState::CINT,
        kungfu::DeoptType::NOTDOUBLE1);
    EXPECT_EQ(result, "testFunction");

    result = samplesRecord.AddRunningStateTest(funcName, RunningState::AINT,
        kungfu::DeoptType::NOTDOUBLE1);
    EXPECT_EQ(result, "testFunction");

    result = samplesRecord.AddRunningStateTest(funcName, RunningState::AOT,
        kungfu::DeoptType::NOTDOUBLE1);
    EXPECT_EQ(result, "testFunction");

    result = samplesRecord.AddRunningStateTest(funcName, RunningState::BUILTIN,
        kungfu::DeoptType::NOTDOUBLE1);
    EXPECT_EQ(result, "testFunction(BUILTIN)");

    result = samplesRecord.AddRunningStateTest(funcName, RunningState::NAPI,
        kungfu::DeoptType::NOTDOUBLE1);
    EXPECT_EQ(result, "testFunction(NAPI)");

    result = samplesRecord.AddRunningStateTest(funcName, RunningState::ARKUI_ENGINE,
        kungfu::DeoptType::NOTDOUBLE1);
    EXPECT_EQ(result, "testFunction(ARKUI_ENGINE)");

    result = samplesRecord.AddRunningStateTest(funcName, RunningState::RUNTIME,
        kungfu::DeoptType::NOTDOUBLE1);
    EXPECT_EQ(result, "testFunction");

    result = samplesRecord.AddRunningStateTest(funcName, RunningState::JIT,
        kungfu::DeoptType::NOTDOUBLE1);
    EXPECT_EQ(result, "testFunction(JIT)");

    samplesRecord.SetEnableVMTag(true);
    result = samplesRecord.AddRunningStateTest(funcName, RunningState::CINT,
        kungfu::DeoptType::NOTDOUBLE1);
    EXPECT_EQ(result, "testFunction(CINT)");

    result = samplesRecord.AddRunningStateTest(funcName, RunningState::AINT,
        kungfu::DeoptType::NOTDOUBLE1);
    EXPECT_EQ(result, "testFunction(AINT)");

    result = samplesRecord.AddRunningStateTest(funcName, RunningState::AOT,
        kungfu::DeoptType::NOTDOUBLE1);
    EXPECT_EQ(result, "testFunction(AOT)");

    result = samplesRecord.AddRunningStateTest(funcName, RunningState::RUNTIME,
        kungfu::DeoptType::NOTDOUBLE1);
    EXPECT_EQ(result, "testFunction(RUNTIME)");

    result = samplesRecord.AddRunningStateTest(funcName, RunningState::AINT_D,
        kungfu::DeoptType::NOTINT1);
    EXPECT_EQ(result, "testFunction(AINT-D)(DEOPT:NotInt1)");
}

HWTEST_F_L0(SamplesRecordTest, StatisticStateTimeTest)
{
    SamplesRecordFriendTest samplesRecord;
    samplesRecord.StatisticStateTimeTest(100, RunningState::AINT_D);
    samplesRecord.StatisticStateTimeTest(101, RunningState::GC);
    samplesRecord.StatisticStateTimeTest(102, RunningState::CINT);
    samplesRecord.StatisticStateTimeTest(103, RunningState::AINT);
    samplesRecord.StatisticStateTimeTest(104, RunningState::AOT);
    samplesRecord.StatisticStateTimeTest(105, RunningState::BUILTIN);
    samplesRecord.StatisticStateTimeTest(106, RunningState::NAPI);
    samplesRecord.StatisticStateTimeTest(107, RunningState::ARKUI_ENGINE);
    samplesRecord.StatisticStateTimeTest(108, RunningState::RUNTIME);
    samplesRecord.StatisticStateTimeTest(109, RunningState::JIT);

    std::unique_ptr<ProfileInfo> profileInfo = samplesRecord.GetProfileInfoTest();
    EXPECT_EQ(profileInfo->asmInterpreterDeoptTime, 100);
    EXPECT_EQ(profileInfo->gcTime, 101);
    EXPECT_EQ(profileInfo->cInterpreterTime, 102);
    EXPECT_EQ(profileInfo->asmInterpreterTime, 103);
    EXPECT_EQ(profileInfo->aotTime, 104);
    EXPECT_EQ(profileInfo->builtinTime, 105);
    EXPECT_EQ(profileInfo->napiTime, 106);
    EXPECT_EQ(profileInfo->arkuiEngineTime, 107);
    EXPECT_EQ(profileInfo->runtimeTime, 108);
    EXPECT_EQ(profileInfo->jitTime, 109);
}

HWTEST_F_L0(SamplesRecordTest, PushStackTest)
{
    SamplesRecord record;
    MethodKey key;

    for (size_t i = 0; i < MAX_STACK_SIZE; ++i) {
        key.lineNumber = static_cast<int>(i);
        EXPECT_TRUE(record.PushFrameStack(key));
    }
    EXPECT_FALSE(record.PushFrameStack(key));

    for (size_t i = 0; i < MAX_STACK_SIZE; ++i) {
        key.lineNumber = static_cast<int>(i);
        EXPECT_TRUE(record.PushNapiFrameStack(key));
    }
    EXPECT_FALSE(record.PushNapiFrameStack(key));

    FrameInfoTemp frameInfoTemp;
    for (size_t i = 0; i < MAX_STACK_SIZE; ++i) {
        frameInfoTemp.lineNumber = static_cast<int>(i);
        EXPECT_TRUE(record.PushStackInfo(frameInfoTemp));
    }
    EXPECT_FALSE(record.PushStackInfo(frameInfoTemp));

    for (size_t i = 0; i < MAX_STACK_SIZE; ++i) {
        frameInfoTemp.lineNumber = static_cast<int>(i);
        EXPECT_TRUE(record.PushNapiStackInfo(frameInfoTemp));
    }
    EXPECT_FALSE(record.PushNapiStackInfo(frameInfoTemp));
}

HWTEST_F_L0(SamplesRecordTest, GetModuleNameTest)
{
    SamplesRecord record;

    char input1[] = "/path/to/module@version";
    EXPECT_EQ(record.GetModuleName(input1), "module");

    char input2[] = "/path/to/module";
    EXPECT_EQ(record.GetModuleName(input2), "");

    char input3[] = "module@version";
    EXPECT_EQ(record.GetModuleName(input3), "");
}

HWTEST_F_L0(SamplesRecordTest, NapiFrameInfoTempToMapTest)
{
    SamplesRecordFriendTest samplesRecord;
    samplesRecord.NapiFrameInfoTempToMapTest();
    FrameInfoTemp frameInfoTemp;
    frameInfoTemp.lineNumber = static_cast<int>(5);
    samplesRecord.PushNapiStackInfoTest(frameInfoTemp);
    samplesRecord.NapiFrameInfoTempToMapTest();
}

HWTEST_F_L0(SamplesRecordTest, TranslateUrlPositionBySourceMapTest)
{
    SamplesRecordFriendTest samplesRecord;
    FrameInfo entry1;
    SourceMapTranslateCallback sourceMapTranslateCallback_ = [](const std::string&, int, int) { return true; };
    samplesRecord.sourceMapTranslateCallbackTest(sourceMapTranslateCallback_);
    samplesRecord.TranslateUrlPositionBySourceMapTest(entry1);
    EXPECT_EQ(entry1.url, "");

    FrameInfo entry2;
    entry2.url = "some_url.js";
    sourceMapTranslateCallback_ = [](const std::string&, int, int) { return true; };
    samplesRecord.sourceMapTranslateCallbackTest(sourceMapTranslateCallback_);
    samplesRecord.TranslateUrlPositionBySourceMapTest(entry2);
    EXPECT_EQ(entry2.url, "some_url.js");

    FrameInfo entry3;
    entry3.url = "path/to/_.js";
    sourceMapTranslateCallback_ = [](const std::string&, int, int) { return false; };
    samplesRecord.sourceMapTranslateCallbackTest(sourceMapTranslateCallback_);
    samplesRecord.TranslateUrlPositionBySourceMapTest(entry3);
    EXPECT_EQ(entry3.url, "path/to/_.js");

    FrameInfo entry4;
    entry4.url = "entry/some_key.ets";
    sourceMapTranslateCallback_ = [](const std::string&, int, int) { return false; };
    samplesRecord.sourceMapTranslateCallbackTest(sourceMapTranslateCallback_);
    samplesRecord.TranslateUrlPositionBySourceMapTest(entry4);
    EXPECT_EQ(entry4.url, "entry/build/default/cache/default/default@CompileArkTS/esmodule/debug/some_key.js");

    FrameInfo entry5;
    entry5.url = "entry/some_key.other";
    sourceMapTranslateCallback_ = [](const std::string&, int, int) { return false; };
    samplesRecord.sourceMapTranslateCallbackTest(sourceMapTranslateCallback_);
    samplesRecord.TranslateUrlPositionBySourceMapTest(entry5);
    EXPECT_EQ(entry5.url, "entry/some_key.other");

    FrameInfo entry6;
    entry6.url = "other/path.js";
    sourceMapTranslateCallback_ = [](const std::string&, int, int) { return false; };
    samplesRecord.sourceMapTranslateCallbackTest(sourceMapTranslateCallback_);
    samplesRecord.TranslateUrlPositionBySourceMapTest(entry6);
    EXPECT_EQ(entry6.url, "other/path.js");
}
}  // namespace panda::test