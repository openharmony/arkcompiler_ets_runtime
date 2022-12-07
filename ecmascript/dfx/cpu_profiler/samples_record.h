/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_SAMPLES_RECORD_H
#define ECMASCRIPT_SAMPLES_RECORD_H

#include <atomic>
#include <ctime>
#include <fstream>
#include <cstring>
#include <semaphore.h>

#include "ecmascript/js_thread.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/mem/c_containers.h"

namespace panda::ecmascript {
const int MAX_ARRAY_COUNT = 100; // 100:the maximum size of the array
const int MAX_NODE_COUNT = 10000; // 10000:the maximum size of the array
enum class RunningState : size_t {
    OTHER = 0,
    GC,
    CINT,
    AINT,
    AOT,
    BUILTIN,
    NAPI,
    ARKUI_ENGINE,
    RUNTIME
};

struct MethodKey {
    void *methodIdentifier = nullptr;
    RunningState state = RunningState::OTHER;
    bool operator < (const MethodKey &methodKey) const
    {
        return state < methodKey.state ||
               (state == methodKey.state && methodIdentifier < methodKey.methodIdentifier);
    }
};

struct NodeKey {
    struct MethodKey methodKey = {0};
    int parentId = 0;
    bool operator < (const NodeKey &nodeKey) const
    {
        return parentId < nodeKey.parentId ||
               (parentId == nodeKey.parentId && methodKey < nodeKey.methodKey);
    }
};

struct FrameInfo {
    std::string codeType = "";
    std::string functionName = "";
    int columnNumber = 0;
    int lineNumber = 0;
    int scriptId = 0;
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
    uint64_t builtinTime = 0;
    uint64_t napiTime = 0;
    uint64_t arkuiEngineTime = 0;
    uint64_t runtimeTime = 0;
    uint64_t otherTime = 0;
};

struct FrameInfoTemp {
    char codeType[20] = {0}; // 20:the maximum size of the codeType
    char functionName[50] = {0}; // 50:the maximum size of the functionName
    int columnNumber = 0;
    int lineNumber = 0;
    int scriptId = 0;
    char url[500] = {0}; // 500:the maximum size of the url
    struct MethodKey methodKey = {0};
};

class SamplesRecord {
public:
    explicit SamplesRecord();
    virtual ~SamplesRecord();

    void AddSample(uint64_t sampleTimeStamp);
    void AddSampleCallNapi(uint64_t *sampleTimeStamp);
    void StringifySampleData();
    int GetMethodNodeCount() const;
    int GetframeStackLength() const;
    std::string GetSampleData() const;
    void SetThreadStartTime(uint64_t threadStartTime);
    void SetThreadStopTime(uint64_t threadStopTime);
    void SetStartsampleData(std::string sampleData);
    void SetFileName(std::string &fileName);
    const std::string GetFileName() const;
    void ClearSampleData();
    std::unique_ptr<struct ProfileInfo> GetProfileInfo();
    bool GetIsStart() const;
    void SetIsStart(bool isStart);
    bool GetGcState() const;
    void SetGcState(bool gcState);
    bool GetRuntimeState() const;
    void SetRuntimeState(bool runtimeState);
    void SetIsBreakSampleFlag(bool sampleFlag);
    int SemInit(int index, int pshared, int value);
    int SemPost(int index);
    int SemWait(int index);
    int SemDestroy(int index);
    const CMap<struct MethodKey, struct FrameInfo> &GetStackInfo() const;
    void InsertStackInfo(struct MethodKey &methodKey, struct FrameInfo &codeEntry);
    bool PushFrameStack(struct MethodKey &methodKey);
    bool PushStackInfo(const FrameInfoTemp &frameInfoTemp);
    bool GetBeforeGetCallNapiStackFlag();
    void SetBeforeGetCallNapiStackFlag(bool flag);
    bool GetAfterGetCallNapiStackFlag();
    void SetAfterGetCallNapiStackFlag(bool flag);
    bool GetCallNapiFlag();
    void SetCallNapiFlag(bool flag);
    bool PushNapiFrameStack(struct MethodKey &methodKey);
    bool PushNapiStackInfo(const FrameInfoTemp &frameInfoTemp);
    int GetNapiFrameStackLength();
    void ClearNapiStack();
    std::ofstream fileHandle_;

private:
    void StringifyStateTimeStatistic();
    void StringifyNodes();
    void StringifySamples();
    struct FrameInfo GetMethodInfo(struct MethodKey &methodKey);
    std::string AddRunningStateToName(char *functionName, RunningState state);
    void FrameInfoTempToMap();
    void NapiFrameInfoTempToMap();
    void StatisticStateTime(int timeDelta, RunningState state);

    int previousId_ = 0;
    RunningState previousState_ = RunningState::OTHER;
    uint64_t threadStartTime_ = 0;
    std::atomic_bool isBreakSample_ = false;
    std::atomic_bool gcState_ = false;
    std::atomic_bool runtimeState_ = false;
    std::atomic_bool isStart_ = false;
    std::atomic_bool beforeCallNapi_ = false;
    std::atomic_bool afterCallNapi_ = false;
    std::atomic_bool callNapi_ = false;
    std::unique_ptr<struct ProfileInfo> profileInfo_;
    CVector<int> stackTopLines_;
    CMap<struct NodeKey, int> nodeMap_;
    std::string sampleData_ = "";
    std::string fileName_ = "";
    sem_t sem_[3]; // 3 : sem_ size is three.
    CMap<struct MethodKey, struct FrameInfo> stackInfoMap_;
    struct MethodKey frameStack_[MAX_ARRAY_COUNT] = {};
    int frameStackLength_ = 0;
    CMap<std::string, int> scriptIdMap_;
    FrameInfoTemp frameInfoTemps_[MAX_ARRAY_COUNT] = {};
    int frameInfoTempLength_ = 0;
    // napi stack
    CVector<struct MethodKey> napiFrameStack_;
    CVector<FrameInfoTemp> napiFrameInfoTemps_;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_SAMPLES_RECORD_H