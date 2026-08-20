/**
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

    void NodeInit()
    {
        samples_record.NodeInit();
    }
    
    std::string AddRunningStateTest(const char *functionName, RunningState state, kungfu::DeoptType type)
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

    int GetFrameStackLengthTest()
    {
        return samples_record.GetframeStackLength();
    }

    bool GetFrameInfoTempsTest(int index, FrameInfoTemp &frameInfoTemp)
    {
        if (index < 0 || index >= samples_record.frameInfoTempLength_) {
            return false;
        }
        frameInfoTemp = samples_record.frameInfoTemps_[index];
        return true;
    }

    void PostHybridStackFrameTest(const arkplatform::HybridFrameInfo &frameInfo)
    {
        samples_record.PostHybridStackFrame(frameInfo);
    }

    void AddSampleTest(const FrameStackAndInfo &frame)
    {
        samples_record.AddSample(frame);
    }

    void SetNodeCount(int count)
    {
        samples_record.profileInfo_->nodeCount = count;
    }

    int GetSamplesSize()
    {
        return static_cast<int>(samples_record.profileInfo_->samples.size());
    }

    int GetTimeDeltasSize()
    {
        return static_cast<int>(samples_record.profileInfo_->timeDeltas.size());
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
    SourceMapTranslateCallback sourceMapTranslateCallback_ = [](const std::string&, int, int,
                                                                std::string) { return true; };
    samplesRecord.sourceMapTranslateCallbackTest(sourceMapTranslateCallback_);
    samplesRecord.TranslateUrlPositionBySourceMapTest(entry1);
    EXPECT_EQ(entry1.url, "");

    FrameInfo entry2;
    entry2.url = "some_url.js";
    entry2.packageName = "name";
    sourceMapTranslateCallback_ = [](const std::string&, int, int, std::string) { return true; };
    samplesRecord.sourceMapTranslateCallbackTest(sourceMapTranslateCallback_);
    samplesRecord.TranslateUrlPositionBySourceMapTest(entry2);
    EXPECT_EQ(entry2.url, "some_url.js");

    FrameInfo entry3;
    entry3.url = "path/to/_.js";
    entry3.packageName = "name";
    sourceMapTranslateCallback_ = [](const std::string&, int, int, std::string) { return false; };
    samplesRecord.sourceMapTranslateCallbackTest(sourceMapTranslateCallback_);
    samplesRecord.TranslateUrlPositionBySourceMapTest(entry3);
    EXPECT_EQ(entry3.url, "path/to/_.js");

    FrameInfo entry4;
    entry4.url = "entry/some_key.ets";
    entry4.packageName = "name";
    sourceMapTranslateCallback_ = [](const std::string&, int, int, std::string) { return false; };
    samplesRecord.sourceMapTranslateCallbackTest(sourceMapTranslateCallback_);
    samplesRecord.TranslateUrlPositionBySourceMapTest(entry4);
    EXPECT_EQ(entry4.url, "entry/build/default/cache/default/default@CompileArkTS/esmodule/debug/some_key.js");

    FrameInfo entry5;
    entry5.url = "entry/some_key.other";
    entry5.packageName = "name";
    sourceMapTranslateCallback_ = [](const std::string&, int, int, std::string) { return false; };
    samplesRecord.sourceMapTranslateCallbackTest(sourceMapTranslateCallback_);
    samplesRecord.TranslateUrlPositionBySourceMapTest(entry5);
    EXPECT_EQ(entry5.url, "entry/some_key.other");

    FrameInfo entry6;
    entry6.url = "other/path.js";
    entry6.packageName = "name";
    sourceMapTranslateCallback_ = [](const std::string&, int, int, std::string) { return false; };
    samplesRecord.sourceMapTranslateCallbackTest(sourceMapTranslateCallback_);
    samplesRecord.TranslateUrlPositionBySourceMapTest(entry6);
    EXPECT_EQ(entry6.url, "other/path.js");
}

/**
 * @tc.name: PostHybridStackFrameTest_Normal
 * @tc.desc: Test normal PostHybridStackFrame behavior.
 *           Verifies normal frame info can be pushed into the frame stack.
 * @tc.type: FUNC
 */
HWTEST_F_L0(SamplesRecordTest, PostHybridStackFrameTest_Normal)
{
    SamplesRecord record;
    record.NodeInit();

    arkplatform::HybridFrameInfo frameInfo;
    frameInfo.SetFunctionName("testFunction");
    frameInfo.SetUrl("test.ets");
    frameInfo.scriptId = 1;
    frameInfo.lineNumber = 10;
    frameInfo.columnNumber = 5;
    frameInfo.isStaticFrame = true;

    record.PostHybridStackFrame(frameInfo);

    EXPECT_EQ(record.GetframeStackLength(), 1);
}

/**
 * @tc.name: PostHybridStackFrameTest_EmptyFunctionName
 * @tc.desc: Test PostHybridStackFrame with empty function name
 *           empty function name should use default value "anonymous"
 * @tc.type: FUNC
 */
HWTEST_F_L0(SamplesRecordTest, PostHybridStackFrameTest_EmptyFunctionName)
{
    SamplesRecordFriendTest samplesRecord;
    samplesRecord.NodeInit();

    arkplatform::HybridFrameInfo frameInfo;
    frameInfo.SetFunctionName("");
    frameInfo.SetUrl("test.ets");
    frameInfo.scriptId = 1;
    frameInfo.lineNumber = 10;
    frameInfo.columnNumber = 5;

    samplesRecord.PostHybridStackFrameTest(frameInfo);

    EXPECT_EQ(samplesRecord.GetFrameStackLengthTest(), 1);

    FrameInfoTemp resultFrame;
    EXPECT_TRUE(samplesRecord.GetFrameInfoTempsTest(0, resultFrame));
    EXPECT_STREQ(resultFrame.functionName, "anonymous");
}

/**
 * @tc.name: PostHybridStackFrameTest_EmptyUrl
 * @tc.desc: Test PostHybridStackFrame with empty URL
 *           empty URL should use default value "ets:static"
 * @tc.type: FUNC
 */
HWTEST_F_L0(SamplesRecordTest, PostHybridStackFrameTest_EmptyUrl)
{
    SamplesRecordFriendTest samplesRecord;
    samplesRecord.NodeInit();

    arkplatform::HybridFrameInfo frameInfo;
    frameInfo.SetFunctionName("testFunction");
    frameInfo.SetUrl("");
    frameInfo.scriptId = 1;
    frameInfo.lineNumber = 10;
    frameInfo.columnNumber = 5;

    samplesRecord.PostHybridStackFrameTest(frameInfo);

    EXPECT_EQ(samplesRecord.GetFrameStackLengthTest(), 1);

    FrameInfoTemp resultFrame;
    EXPECT_TRUE(samplesRecord.GetFrameInfoTempsTest(0, resultFrame));
    EXPECT_STREQ(resultFrame.url, "");
}

/**
 * @tc.name: PostHybridStackFrameTest_MultipleFrames
 * @tc.desc: Test PostHybridStackFrame with multiple frames
 *           multiple frames can be pushed into frame stack correctly
 * @tc.type: FUNC
 */
HWTEST_F_L0(SamplesRecordTest, PostHybridStackFrameTest_MultipleFrames)
{
    SamplesRecordFriendTest samplesRecord;
    samplesRecord.NodeInit();

    for (int i = 0; i < 5; ++i) {
        arkplatform::HybridFrameInfo frameInfo;
        frameInfo.SetFunctionName("function" + std::to_string(i));
        frameInfo.SetUrl("test" + std::to_string(i) + ".ets");
        frameInfo.scriptId = i;
        frameInfo.lineNumber = i * 10;
        frameInfo.columnNumber = i;

        samplesRecord.PostHybridStackFrameTest(frameInfo);
    }

    EXPECT_EQ(samplesRecord.GetFrameStackLengthTest(), 5);
}

/**
 * @tc.name: PostHybridStackFrameTest_MaxStackSize
 * @tc.desc: Test PostHybridStackFrame max stack depth
 *           pushing up to MAX_STACK_SIZE should not crash
 * @tc.type: FUNC
 */
HWTEST_F_L0(SamplesRecordTest, PostHybridStackFrameTest_MaxStackSize)
{
    SamplesRecordFriendTest samplesRecord;
    samplesRecord.NodeInit();

    for (int i = 0; i < MAX_STACK_SIZE; ++i) {
        arkplatform::HybridFrameInfo frameInfo;
        frameInfo.SetFunctionName("function" + std::to_string(i));
        frameInfo.SetUrl("test.ets");
        frameInfo.scriptId = i;
        frameInfo.lineNumber = i;
        frameInfo.columnNumber = i;

        samplesRecord.PostHybridStackFrameTest(frameInfo);
    }

    EXPECT_EQ(samplesRecord.GetFrameStackLengthTest(), MAX_STACK_SIZE);
}

/**
 * @tc.name: PostHybridStackFrameTest_LongFunctionName
 * @tc.desc: Test PostHybridStackFrame with long function name
 *           overlong function name should be truncated without crash
 * @tc.type: FUNC
 */
HWTEST_F_L0(SamplesRecordTest, PostHybridStackFrameTest_LongFunctionName)
{
    SamplesRecordFriendTest samplesRecord;
    samplesRecord.NodeInit();

    arkplatform::HybridFrameInfo frameInfo;
    frameInfo.SetFunctionName(std::string(512, 'a'));
    frameInfo.SetUrl("test.ets");
    frameInfo.scriptId = 1;
    frameInfo.lineNumber = 10;
    frameInfo.columnNumber = 5;

    samplesRecord.PostHybridStackFrameTest(frameInfo);

    EXPECT_EQ(samplesRecord.GetFrameStackLengthTest(), 1);

    FrameInfoTemp resultFrame;
    EXPECT_TRUE(samplesRecord.GetFrameInfoTempsTest(0, resultFrame));
    EXPECT_EQ(strlen(resultFrame.functionName), 99);
}

/**
 * @tc.name: PostHybridStackFrameTest_LongUrl
 * @tc.desc: Test PostHybridStackFrame with long URL
 *           overlong URL should be truncated without crash
 * @tc.type: FUNC
 */
HWTEST_F_L0(SamplesRecordTest, PostHybridStackFrameTest_LongUrl)
{
    SamplesRecordFriendTest samplesRecord;
    samplesRecord.NodeInit();

    arkplatform::HybridFrameInfo frameInfo;
    frameInfo.SetFunctionName("testFunction");
    frameInfo.SetUrl(std::string(512, 'b'));
    frameInfo.scriptId = 1;
    frameInfo.lineNumber = 10;
    frameInfo.columnNumber = 5;

    samplesRecord.PostHybridStackFrameTest(frameInfo);

    EXPECT_EQ(samplesRecord.GetFrameStackLengthTest(), 1);

    FrameInfoTemp resultFrame;
    EXPECT_FALSE(samplesRecord.GetFrameInfoTempsTest(0, resultFrame));
    EXPECT_EQ(strlen(resultFrame.url), 0);
}

/**
 * @tc.name: PostHybridStackFrameTest_EmptyBoth
 * @tc.desc: Test PostHybridStackFrame with both empty function name and URL
 *           both default values should be set correctly
 * @tc.type: FUNC
 */
HWTEST_F_L0(SamplesRecordTest, PostHybridStackFrameTest_EmptyBoth)
{
    SamplesRecordFriendTest samplesRecord;
    samplesRecord.NodeInit();

    arkplatform::HybridFrameInfo frameInfo;
    frameInfo.SetFunctionName("");
    frameInfo.SetUrl("");
    frameInfo.scriptId = 1;
    frameInfo.lineNumber = 10;
    frameInfo.columnNumber = 5;

    samplesRecord.PostHybridStackFrameTest(frameInfo);

    EXPECT_EQ(samplesRecord.GetFrameStackLengthTest(), 1);

    FrameInfoTemp resultFrame;
    EXPECT_TRUE(samplesRecord.GetFrameInfoTempsTest(0, resultFrame));
    EXPECT_STREQ(resultFrame.functionName, "anonymous");
    EXPECT_STREQ(resultFrame.url, "");
}

/**
 * @tc.name: PostHybridStackFrameTest_NullNativePtr
 * @tc.desc: Test PostHybridStackFrame with nativePtr as nullptr
 *           should not crash when nativePtr is nullptr
 * @tc.type: FUNC
 */
HWTEST_F_L0(SamplesRecordTest, PostHybridStackFrameTest_NullNativePtr)
{
    SamplesRecordFriendTest samplesRecord;
    samplesRecord.NodeInit();

    arkplatform::HybridFrameInfo frameInfo;
    frameInfo.SetFunctionName("testFunction");
    frameInfo.SetUrl("test.ets");
    frameInfo.scriptId = 1;
    frameInfo.lineNumber = 10;
    frameInfo.columnNumber = 5;
    frameInfo.nativePtr = nullptr;

    samplesRecord.PostHybridStackFrameTest(frameInfo);

    EXPECT_EQ(samplesRecord.GetFrameStackLengthTest(), 1);
}

/**
 * @tc.name: PostHybridStackFrameTest_Overflow
 * @tc.desc: Test PostHybridStackFrame push after stack is full
 *           no crash and frame count remains MAX_STACK_SIZE
 * @tc.type: FUNC
 */
HWTEST_F_L0(SamplesRecordTest, PostHybridStackFrameTest_Overflow)
{
    SamplesRecordFriendTest samplesRecord;
    samplesRecord.NodeInit();

    for (int i = 0; i < MAX_STACK_SIZE + 10; ++i) {
        arkplatform::HybridFrameInfo frameInfo;
        frameInfo.SetFunctionName("function" + std::to_string(i));
        frameInfo.SetUrl("test.ets");
        frameInfo.scriptId = i;
        frameInfo.lineNumber = i;
        frameInfo.columnNumber = i;

        samplesRecord.PostHybridStackFrameTest(frameInfo);
    }

    EXPECT_EQ(samplesRecord.GetFrameStackLengthTest(), MAX_STACK_SIZE);
}

/**
 * @tc.name: PostHybridStackFrameTest_NegativeLineColumn
 * @tc.desc: Test PostHybridStackFrame with negative line and column numbers
 *           negative line and column numbers are preserved correctly
 * @tc.type: FUNC
 */
HWTEST_F_L0(SamplesRecordTest, PostHybridStackFrameTest_NegativeLineColumn)
{
    SamplesRecordFriendTest samplesRecord;
    samplesRecord.NodeInit();

    arkplatform::HybridFrameInfo frameInfo;
    frameInfo.SetFunctionName("testFunction");
    frameInfo.SetUrl("test.ets");
    frameInfo.scriptId = -1;
    frameInfo.lineNumber = -100;
    frameInfo.columnNumber = -50;

    samplesRecord.PostHybridStackFrameTest(frameInfo);

    EXPECT_EQ(samplesRecord.GetFrameStackLengthTest(), 1);

    FrameInfoTemp resultFrame;
    EXPECT_TRUE(samplesRecord.GetFrameInfoTempsTest(0, resultFrame));
    EXPECT_EQ(resultFrame.lineNumber, -100);
    EXPECT_EQ(resultFrame.columnNumber, -50);
}

/**
* @tc.name: SamplesQueuePopFrameTest_Basic
* @tc.desc: Test PopFrame basic functionality after lock-split optimization.
*           Post one frame and pop it, verify data integrity.
* @tc.type: FUNC
*/
HWTEST_F_L0(SamplesRecordTest, SamplesQueuePopFrameTest_Basic)
{
    SamplesQueue queue;

    // Prepare test data
    FrameInfoTemp frameInfoTemps[MAX_STACK_SIZE] = {};
    MethodKey frameStack[MAX_STACK_SIZE] = {};

    frameInfoTemps[0].lineNumber = 10;
    frameInfoTemps[0].columnNumber = 5;
    frameInfoTemps[0].scriptId = 1;
    int ret = snprintf_s(frameInfoTemps[0].functionName, sizeof(frameInfoTemps[0].functionName),
                         sizeof(frameInfoTemps[0].functionName) - 1, "testFunc");
    EXPECT_GE(ret, 0);
    ret = snprintf_s(frameInfoTemps[0].url, sizeof(frameInfoTemps[0].url),
                     sizeof(frameInfoTemps[0].url) - 1, "test.js");
    EXPECT_GE(ret, 0);

    frameStack[0].methodIdentifier = reinterpret_cast<void *>(0x1234);
    frameStack[0].state = RunningState::CINT;

    queue.PostFrame(frameInfoTemps, frameStack, 1, 1);

    // Pop and verify
    FrameStackAndInfo out = {};
    EXPECT_TRUE(queue.PopFrame(out));
    EXPECT_EQ(out.frameInfoTempsLength, 1);
    EXPECT_EQ(out.frameStackLength, 1);
    EXPECT_EQ(out.frameInfoTemps[0].lineNumber, 10);
    EXPECT_EQ(out.frameInfoTemps[0].columnNumber, 5);
    EXPECT_EQ(out.frameInfoTemps[0].scriptId, 1);
    EXPECT_STREQ(out.frameInfoTemps[0].functionName, "testFunc");
    EXPECT_STREQ(out.frameInfoTemps[0].url, "test.js");
    EXPECT_EQ(out.frameStack[0].methodIdentifier, reinterpret_cast<void *>(0x1234));
    EXPECT_EQ(out.frameStack[0].state, RunningState::CINT);

    // Queue should now be empty
    EXPECT_FALSE(queue.PopFrame(out));
}

/**
* @tc.name: SamplesQueuePopFrameTest_PartialData
* @tc.desc: Test PopFrame with frameInfoTempsLength < MAX_STACK_SIZE.
*           Only the specified number of elements should be meaningful.
* @tc.type: FUNC
*/
HWTEST_F_L0(SamplesRecordTest, SamplesQueuePopFrameTest_PartialData)
{
    SamplesQueue queue;

    FrameInfoTemp frameInfoTemps[MAX_STACK_SIZE] = {};
    MethodKey frameStack[MAX_STACK_SIZE] = {};

    // Only fill 3 frameInfoTemps and 5 frameStack entries
    for (int i = 0; i < 3; ++i) {
        frameInfoTemps[i].lineNumber = i;
        frameInfoTemps[i].scriptId = i + 100;
        std::string funcName = "partialFunc" + std::to_string(i);
        int ret = snprintf_s(frameInfoTemps[i].functionName, sizeof(frameInfoTemps[i].functionName),
                             sizeof(frameInfoTemps[i].functionName) - 1, "%s", funcName.c_str());
        EXPECT_GE(ret, 0);
    }
    for (int i = 0; i < 5; ++i) {
        frameStack[i].methodIdentifier = reinterpret_cast<void *>(0x2000 + i);
        frameStack[i].lineNumber = i * 2;
    }

    queue.PostFrame(frameInfoTemps, frameStack, 3, 5);

    FrameStackAndInfo out = {};
    EXPECT_TRUE(queue.PopFrame(out));
    EXPECT_EQ(out.frameInfoTempsLength, 3);
    EXPECT_EQ(out.frameStackLength, 5);

    // Verify the 3 frameInfoTemps
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(out.frameInfoTemps[i].lineNumber, i);
        EXPECT_EQ(out.frameInfoTemps[i].scriptId, i + 100);
        EXPECT_STREQ(out.frameInfoTemps[i].functionName, ("partialFunc" + std::to_string(i)).c_str());
    }
    // Verify the 5 frameStack entries
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(out.frameStack[i].methodIdentifier, reinterpret_cast<void *>(0x2000 + i));
        EXPECT_EQ(out.frameStack[i].lineNumber, i * 2);
    }
}

/**
* @tc.name: AddSampleMaxNodeCountTest_DiscardIncompleteSample
* @tc.desc: Test AddSample discards sample when nodeCount reaches MAX_NODE_COUNT.
*           After early return, no sample should be attributed to wrong node.
* @tc.type: FUNC
*/
HWTEST_F_L0(SamplesRecordTest, AddSampleMaxNodeCountTest_DiscardIncompleteSample)
{
    SamplesRecordFriendTest samplesRecord;
    samplesRecord.NodeInit();

    // NodeInit creates 3 nodes: (root), (program), (idle)
    // Set nodeCount to MAX_NODE_COUNT - 1 so the next new node creation hits the limit
    samplesRecord.SetNodeCount(MAX_NODE_COUNT - 1);

    int samplesBefore = samplesRecord.GetSamplesSize();
    int timeDeltasBefore = samplesRecord.GetTimeDeltasSize();

    // Build a frame with 2 stack entries that would require creating new nodes
    FrameStackAndInfo frame = {};
    frame.frameStackLength = 2;
    frame.frameInfoTempsLength = 2;
    frame.timeStamp = 1000;

    // Each unique methodIdentifier creates a new node
    frame.frameStack[0].methodIdentifier = reinterpret_cast<void *>(0xA000);
    frame.frameStack[0].state = RunningState::CINT;
    frame.frameStack[1].methodIdentifier = reinterpret_cast<void *>(0xA001);
    frame.frameStack[1].state = RunningState::CINT;

    frame.frameInfoTemps[0].lineNumber = 1;
    int ret = snprintf_s(frame.frameInfoTemps[0].functionName,
                         sizeof(frame.frameInfoTemps[0].functionName),
                         sizeof(frame.frameInfoTemps[0].functionName) - 1, "funcA");
    EXPECT_GE(ret, 0);
    frame.frameInfoTemps[0].methodKey = frame.frameStack[0];

    frame.frameInfoTemps[1].lineNumber = 2;
    ret = snprintf_s(frame.frameInfoTemps[1].functionName,
                     sizeof(frame.frameInfoTemps[1].functionName),
                     sizeof(frame.frameInfoTemps[1].functionName) - 1, "funcB");
    EXPECT_GE(ret, 0);
    frame.frameInfoTemps[1].methodKey = frame.frameStack[1];

    samplesRecord.AddSampleTest(frame);

    // After early return: samples and timeDeltas should not increase
    EXPECT_EQ(samplesRecord.GetSamplesSize(), samplesBefore);
    EXPECT_EQ(samplesRecord.GetTimeDeltasSize(), timeDeltasBefore);

    // nodeCount should be MAX_NODE_COUNT (one new node created before return)
    EXPECT_EQ(samplesRecord.GetProfileInfoTest()->nodeCount, MAX_NODE_COUNT);
}
}  // namespace panda::test