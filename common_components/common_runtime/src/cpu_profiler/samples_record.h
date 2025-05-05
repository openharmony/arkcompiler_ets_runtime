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
#ifndef ARK_COMMON_SAMPLES_RECORD_H
#define ARK_COMMON_SAMPLES_RECORD_H

#include <atomic>
#include <list>
#include <map>
#include <set>
#include <vector>

#include "common_components/common_runtime/src/base/c_string.h"

namespace panda {
constexpr int MAX_NODE_COUNT = 20000;   // 20000:the maximum size of the array
constexpr int UNKNOWN_NODE_ID = 0;      // 0: the (root) node parent id
constexpr int ROOT_NODE_ID = 1;         // 1: the (root) node id
constexpr int PROGRAM_NODE_ID = 2;      // 2: the (program) node id
constexpr int IDLE_NODE_ID = 3;         // 3: the (idel) node id

struct CodeInfo {
    uint64_t funcIdentifier = 0;
    uint64_t scriptId = 0;
    uint32_t lineNumber = 0;
    CString functionName = "";
    CString url = "";
};

struct CpuProfileNode {
    uint64_t id = 0;
    uint64_t parentId = 0;
    uint64_t hitCount = 0;
    struct CodeInfo codeEntry;
    std::vector<uint64_t> children;

    bool operator < (const CpuProfileNode& node) const
    {
        return parentId < node.parentId || (parentId == node.parentId && codeEntry < node.codeEntry);
    }
};

struct ProfileInfo {
    uint64_t mutatorId = 0;
    uint64_t startTime = 0;
    uint64_t stopTime = 0;
    uint64_t nodeCount = 0;

    CpuProfileNode nodes[MAX_NODE_COUNT];
    std::vector<int> samples;
    std::vector<int> timeDeltas;

    std::set<CpuProfileNode> nodeSet;
    uint64_t previousTimeStamp = 0;
    uint64_t previousId = 0;

    // state time statistic
    uint64_t managedTime = 0;
    uint64_t unknownTime = 0;
};

class SampleTask {
public:
    uint64_t timeStamp;
    uint64_t mutatorId;
    std::vector<uint64_t> funcDescRefs;
    std::vector<uint32_t> lineNumbers;
    uint64_t frameCnt;
    bool finishParsed {false};
    int checkPoint {0};
};

class SamplesRecord {
public:
    SamplesRecord() {}
    ~SamplesRecord();
    void InitProfileInfo();
    void ReleaseProfileInfo();
    void SetIsStart(bool started) { isStart_.store(started); }
    bool GetIsStart() const { return isStart_.load(); }
    void SetThreadStartTime(uint64_t threadStartTime) { profileInfo_->startTime = threadStartTime; }
    void SetSampleStopTime(uint64_t threadStopTime) { profileInfo_->stopTime = threadStopTime; }
    void AddSample(SampleTask& task);
    void AddEmptySample(SampleTask& task);
    void StringifySampleData(ProfileInfo* info);
    void DumpProfileInfo();
    void RunTaskLoop();
    void DoSingleTask(uint64_t previousTimeStemp);
    void ParseSampleData(uint64_t previousTimeStemp);
    std::vector<CodeInfo> BuildCodeInfos(SampleTask* task);
    int GetSamplingInterval() { return interval_; }
    bool OpenFile(int fd);
    static uint64_t GetMicrosecondsTimeStamp();

private:
    void StatisticStateTime(ProfileInfo* info, int timeDelta);
    void NodeInit();
    void StringifyStateTimeStatistic(ProfileInfo* info);
    void StringifyNodes(ProfileInfo* info);
    void StringifySamples(ProfileInfo* info);
    uint64_t UpdateScriptIdMap(CString& url);
    bool UpdateNodeMap(ProfileInfo* info, CpuProfileNode& methodNode);
    void AddNodes(ProfileInfo* info, CpuProfileNode& methodNode);
    int GetSampleNodeId(uint64_t previousId, uint64_t topFrameNodeId);
    uint64_t GetPreviousTimeStamp(uint64_t previousTimeStamp, uint64_t startTime);
    void DeleteAbnormalSample(ProfileInfo* info, int timeDelta);
    void IncreaseNodeHitCount(ProfileInfo* info, int sampleNodeId);
    void AddSampleNodeId(ProfileInfo* info, int sampleNodeId);
    void AddTimeDelta(ProfileInfo* info, int timeDelta);
    void SetPreviousTimeStamp(ProfileInfo* info, uint64_t timeStamp);
    std::vector<CodeInfo> GetCodeInfos(SampleTask& task);
    CString GetUrl(uint64_t funcIdentifier);
    CString ParseUrl(uint64_t funcIdentifier);
    CString GetDemangleName(uint64_t funcIdentifier);
    CString ParseDemangleName(uint64_t funcIdentifier);
    void WriteFile();
    bool IsTimeout(uint64_t previousTimeStemp);
    ProfileInfo* GetProfileInfo(uint64_t mutatorId);

    int fileDesc_{ -1 };
    uint32_t timeDeltaThreshold_{ 2000 }; // 2000 : default timeDeltaThreshold 2000us
    std::atomic_bool isStart_{ false };
    CString sampleData_{ "" };
    std::list<SampleTask> taskQueue_;
    ProfileInfo* profileInfo_{ nullptr };
    std::map<CString, uint64_t> scriptIdMap_{ { "", 0 } };
    int interval_{ 500 }; // 500 : default interval 500us
    std::map<uint64_t, CString> identifierFuncnameMap_;
    std::map<uint64_t, CString> identifierUrlMap_;
};
} // namespace panda
#endif
