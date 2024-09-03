/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_DFX_CPU_PROFILER_SAMPLES_RECORD_H
#define ECMASCRIPT_DFX_CPU_PROFILER_SAMPLES_RECORD_H

#include <atomic>
#include <ctime>
#include <cstring>
#include <fstream>
#include <semaphore.h>

#include "ecmascript/dfx/stackinfo/js_stackgetter.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/platform/mutex.h"

namespace panda::ecmascript {
const int MAX_STACK_SIZE = 128; // 128:the maximum size of the js stack
const int MAX_NODE_COUNT = 20000; // 20000:the maximum size of the array
const int MIN_TIME_DELTA = 10; // 10: the minimum value of the time delta
const int PROGRAM_NODE_ID = 2; // 2: the (program) node id
const int QUEUE_CAPACITY = 51; // the capacity of the circular queue is QUEUE_CAPACITY - 1
const size_t NAPI_CALL_SETP = 2; // 2: step size of the variable napiCallIdx in while loop
const size_t PRE_IDX_RANGE = 5; // 5: length of variable preIdx looping backward
const size_t SUB_LEN = 6; // 6: Truncate the path length
const std::string JS_PATH = "entry/build/default/cache/default/default@CompileArkTS/esmodule/debug/";

struct FrameInfo {
    int scriptId = 0;
    int lineNumber = -1;
    int columnNumber = -1;
    std::string functionName = "";
    std::string moduleName = "";
    std::string url = "";
};

struct CpuProfileNode {
    int id = 0;
    int parentId = 0;
    int hitCount = 0;
    struct FrameInfo codeEntry;
    CVector<int> children;
};

struct ProfileInfo {
    uint64_t tid = 0;
    uint64_t startTime = 0;
    uint64_t stopTime = 0;
    struct CpuProfileNode nodes[MAX_NODE_COUNT];
    int nodeCount = 0;
    CVector<int> samples;
    CVector<int> timeDeltas;
    // state time statistic
    uint64_t gcTime = 0;
    uint64_t cInterpreterTime = 0;
    uint64_t asmInterpreterTime = 0;
    uint64_t aotTime = 0;
    uint64_t asmInterpreterDeoptTime = 0;
    uint64_t builtinTime = 0;
    uint64_t napiTime = 0;
    uint64_t arkuiEngineTime = 0;
    uint64_t runtimeTime = 0;
    uint64_t jitTime = 0;
    uint64_t otherTime = 0;
};

struct FrameStackAndInfo {
    struct FrameInfoTemp frameInfoTemps[MAX_STACK_SIZE] = {};
    struct MethodKey frameStack[MAX_STACK_SIZE] = {};
    int frameInfoTempsLength {};
    int frameStackLength {};
    uint64_t timeStamp {};
};

class SamplesQueue {
public:
    SamplesQueue() = default;
    ~SamplesQueue() = default;

    NO_COPY_SEMANTIC(SamplesQueue);
    NO_MOVE_SEMANTIC(SamplesQueue);

    void PostFrame(FrameInfoTemp *frameInfoTemps, MethodKey *frameStack,
                   int frameInfoTempsLength, int frameStackLength);
    void PostNapiFrame(CVector<FrameInfoTemp> &napiFrameInfoTemps,
                       CVector<MethodKey> &napiFrameStack);
    FrameStackAndInfo *PopFrame();
    bool IsEmpty();
    bool IsFull();
    int GetSize();
    int GetFrontIndex();
    int GetRearIndex();
    bool CheckAndCopy(char *dest, size_t length, const char *src) const;

private:
    FrameStackAndInfo frames_[QUEUE_CAPACITY] = {};
    int front_ = 0;
    int rear_ = 0;
    Mutex mtx_;
};

class SamplesRecord {
public:
    SamplesRecord();
    virtual ~SamplesRecord();

    void NodeInit();
    void AddSample(FrameStackAndInfo *frame);
    void AddEmptyStackSample(uint64_t sampleTimeStamp);
    void StringifySampleData();
    int GetMethodNodeCount() const;
    int GetframeStackLength() const;
    std::string GetSampleData() const;
    std::string GetModuleName(char *recordName);
    void SetThreadStartTime(uint64_t threadStartTime);
    uint64_t GetThreadStartTime();
    void SetThreadStopTime();
    void SetFileName(std::string &fileName);
    const std::string GetFileName() const;
    std::unique_ptr<struct ProfileInfo> GetProfileInfo();
    bool GetIsStart() const;
    void SetIsStart(bool isStart);
    bool GetGcState() const;
    void SetGcState(bool gcState);
    bool GetRuntimeState() const;
    void SetRuntimeState(bool runtimeState);
    int SemInit(int index, int pshared, int value);
    int SemPost(int index);
    int SemWait(int index);
    int SemDestroy(int index);
    const CMap<struct MethodKey, struct FrameInfo> &GetStackInfo() const;
    void InsertStackInfo(struct MethodKey &methodKey, struct FrameInfo &codeEntry);
    bool PushFrameStack(struct MethodKey &methodKey);
    bool PushStackInfo(const FrameInfoTemp &frameInfoTemp);
    bool GetCallNapiFlag();
    void SetCallNapiFlag(bool flag);
    bool PushNapiFrameStack(struct MethodKey &methodKey);
    bool PushNapiStackInfo(const FrameInfoTemp &frameInfoTemp);
    int GetNapiFrameStackLength();
    void ClearNapiStack();
    void PostFrame();
    void PostNapiFrame();
    void ResetFrameLength();
    uint64_t GetCallTimeStamp();
    void SetCallTimeStamp(uint64_t timeStamp);
    void AddTraceEvent(bool isFinish);
    void AddStartTraceEvent();

    void SetEnableVMTag(bool flag)
    {
        enableVMTag_ = flag;
    }

    void SetSourceMapTranslateCallback(SourceMapTranslateCallback cb)
    {
        sourceMapTranslateCallback_ = cb;
    }

    void SetTimeDeltaThreshold(uint32_t timeDeltaThreshold)
    {
        timeDeltaThreshold_ = timeDeltaThreshold;
    }

    std::ofstream fileHandle_;
    SamplesQueue *samplesQueue_ {nullptr};

private:
    void StringifyStateTimeStatistic();
    void StringifyNodes();
    void StringifySamples();
    struct FrameInfo GetMethodInfo(struct MethodKey &methodKey);
    std::string AddRunningState(char *functionName, RunningState state, kungfu::DeoptType type);
    void FrameInfoTempToMap(FrameInfoTemp *frameInfoTemps, int frameInfoTempLength);
    void NapiFrameInfoTempToMap();
    void StatisticStateTime(int timeDelta, RunningState state);
    void TranslateUrlPositionBySourceMap(struct FrameInfo &codeEntry);

    int previousId_ = 0;
    RunningState previousState_ = RunningState::OTHER;
    uint64_t previousTimeStamp_ = 0;
    std::atomic_bool gcState_ = false;
    std::atomic_bool runtimeState_ = false;
    std::atomic_bool isStart_ = false;
    std::atomic_bool callNapi_ = false;
    std::unique_ptr<struct ProfileInfo> profileInfo_;
    CMap<struct NodeKey, int> nodeMap_;
    std::string sampleData_ = "";
    std::string fileName_ = "";
    sem_t sem_[3]; // 3 : sem_ size is three.
    CMap<struct MethodKey, struct FrameInfo> stackInfoMap_;
    struct MethodKey frameStack_[MAX_STACK_SIZE] = {};
    int frameStackLength_ = 0;
    CMap<std::string, int> scriptIdMap_;
    FrameInfoTemp frameInfoTemps_[MAX_STACK_SIZE] = {};
    int frameInfoTempLength_ = 0;
    // napi stack
    CVector<struct MethodKey> napiFrameStack_;
    CVector<FrameInfoTemp> napiFrameInfoTemps_;
    bool enableVMTag_ {false};
    uint64_t callTimeStamp_ = 0;
    uint32_t timeDeltaThreshold_ = 0;
    SourceMapTranslateCallback sourceMapTranslateCallback_ {nullptr};
    int traceEventNodePos_ = 0;
    uint32_t traceEventSamplePos_ = 0;

    friend class SamplesRecordFriendTest;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_DFX_CPU_PROFILER_SAMPLES_RECORD_H
