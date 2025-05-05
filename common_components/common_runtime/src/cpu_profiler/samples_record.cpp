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

#include "common_components/common_runtime/src/cpu_profiler/samples_record.h"
#include <algorithm>
#include "common_components/common_runtime/src/base/time_utils.h"

namespace panda {
const int USEC_PER_SEC = 1000 * 1000;
const int NSEC_PER_USEC = 1000;
uint64_t SamplesRecord::GetMicrosecondsTimeStamp()
{
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return time.tv_sec * USEC_PER_SEC + time.tv_nsec / NSEC_PER_USEC;
}

SamplesRecord::~SamplesRecord()
{
    ReleaseProfileInfo();
}

void SamplesRecord::InitProfileInfo()
{
    profileInfo_ = new ProfileInfo();
    LOGF_CHECK(profileInfo_ != nullptr) << "new profileInfo fail";
    NodeInit();
}

void SamplesRecord::ReleaseProfileInfo()
{
    if (profileInfo_ != nullptr) {
        delete profileInfo_;
        profileInfo_ = nullptr;
    }
}

void SamplesRecord::NodeInit()
{
    CpuProfileNode methodNode;

    methodNode.id = ROOT_NODE_ID;
    methodNode.parentId = UNKNOWN_NODE_ID;
    methodNode.codeEntry.functionName = "(root)";
    profileInfo_->nodes[profileInfo_->nodeCount++] = methodNode;
    profileInfo_->nodeSet.insert(methodNode);

    methodNode.id = PROGRAM_NODE_ID;
    methodNode.parentId = ROOT_NODE_ID;
    methodNode.codeEntry.functionName = "(program)";
    profileInfo_->nodes[profileInfo_->nodeCount++] = methodNode;
    profileInfo_->nodes[ROOT_NODE_ID - 1].children.emplace_back(methodNode.id);
    profileInfo_->nodeSet.insert(methodNode);

    methodNode.id = IDLE_NODE_ID;
    methodNode.parentId = ROOT_NODE_ID;
    methodNode.codeEntry.functionName = "(idle)";
    profileInfo_->nodes[profileInfo_->nodeCount++] = methodNode;
    profileInfo_->nodes[ROOT_NODE_ID - 1].children.emplace_back(methodNode.id);
    profileInfo_->nodeSet.insert(methodNode);
}

uint64_t SamplesRecord::UpdateScriptIdMap(CString& url)
{
    auto iter = scriptIdMap_.find(url);
    if (iter == scriptIdMap_.end()) {
        scriptIdMap_.emplace(url, scriptIdMap_.size() + 1);
        return static_cast<uint64_t>(scriptIdMap_.size());
    } else {
        return iter->second;
    }
}

bool SamplesRecord::UpdateNodeMap(ProfileInfo* info, CpuProfileNode& methodNode)
{
    auto& nodSet = info->nodeSet;
    auto result = nodSet.find(methodNode);
    if (result == nodSet.end()) {
        info->previousId = methodNode.id = nodSet.size() + 1;
        nodSet.insert(methodNode);
        return true;
    } else {
        info->previousId = methodNode.id = result->id;
        return false;
    }
}

void SamplesRecord::AddNodes(ProfileInfo* info, CpuProfileNode& methodNode)
{
    info->nodes[info->nodeCount++] = methodNode;
    if (methodNode.parentId > 0 && methodNode.parentId <= info->nodeCount) {
        info->nodes[methodNode.parentId - 1].children.emplace_back(methodNode.id);
    }
}

int SamplesRecord::GetSampleNodeId(uint64_t previousId, uint64_t topFrameNodeId)
{
    return previousId == 0 ? ROOT_NODE_ID : topFrameNodeId;
}

uint64_t SamplesRecord::GetPreviousTimeStamp(uint64_t previousTimeStamp, uint64_t startTime)
{
    return previousTimeStamp == 0 ? startTime : previousTimeStamp;
}

void SamplesRecord::DeleteAbnormalSample(ProfileInfo* info, int timeDelta)
{
    if (timeDelta > static_cast<int>(timeDeltaThreshold_)) {
        uint32_t size = info->samples.size();
        if (size > 0) {
            info->samples[size - 1] = PROGRAM_NODE_ID;
        }
        info->previousState = FrameType::UNKNOWN;
    }
}

void SamplesRecord::SetPreviousState(ProfileInfo* info, FrameType frameType)
{
    info->previousState = frameType;
}

void SamplesRecord::IncreaseNodeHitCount(ProfileInfo* info, int sampleNodeId)
{
    info->nodes[sampleNodeId - 1].hitCount++;
}

void SamplesRecord::AddSampleNodeId(ProfileInfo* info, int sampleNodeId)
{
    info->samples.emplace_back(sampleNodeId);
}

void SamplesRecord::AddTimeDelta(ProfileInfo* info, int timeDelta)
{
    info->timeDeltas.emplace_back(timeDelta);
}

void SamplesRecord::SetPreviousTimeStamp(ProfileInfo* info, uint64_t timeStamp)
{
    info->previousTimeStamp = timeStamp;
}

ProfileInfo* SamplesRecord::GetProfileInfo(uint64_t mutatorId)
{
    if (profileInfo_->mutatorId != mutatorId) {
        profileInfo_->mutatorId = mutatorId;
    }
    return profileInfo_;
}

void SamplesRecord::AddSample(SampleTask& task)
{
    ProfileInfo* info = GetProfileInfo(task.mutatorId);
    if (info->nodeCount >= MAX_NODE_COUNT) {
        LOG_COMMON(WARN) << "CpuProfileNode counts over 20000.";
        return;
    }
    std::vector<CodeInfo> codeInfos = GetCodeInfos(task);
    int frameSize = codeInfos.size();
    CpuProfileNode methodNode;
    methodNode.id = 1;
    for (; frameSize >= 1; frameSize--) {
        methodNode.codeEntry = codeInfos[frameSize - 1];
        methodNode.parentId =  methodNode.id;
        if (UpdateNodeMap(info, methodNode)) {
            AddNodes(info, methodNode);
        }
    }
    int sampleNodeId = GetSampleNodeId(info->previousId, methodNode.id);
    uint64_t previousTimeStamp = GetPreviousTimeStamp(info->previousTimeStamp, info->startTime);
    int timeDelta = static_cast<int>(task.timeStamp - previousTimeStamp);

    DeleteAbnormalSample(info, timeDelta);

    StatisticStateTime(info, timeDelta);

    SetPreviousState(info, codeInfos.front().frameType);

    IncreaseNodeHitCount(info, sampleNodeId);

    AddSampleNodeId(info, sampleNodeId);

    AddTimeDelta(info, timeDelta);

    SetPreviousTimeStamp(info, task.timeStamp);
}

void SamplesRecord::AddEmptySample(SampleTask& task)
{
    ProfileInfo* info = GetProfileInfo(task.mutatorId);
    uint64_t previousTimeStamp = GetPreviousTimeStamp(info->previousTimeStamp, info->startTime);
    int timeDelta = static_cast<int>(task.timeStamp - previousTimeStamp);

    DeleteAbnormalSample(info, timeDelta);

    StatisticStateTime(info, timeDelta);

    SetPreviousState(info, FrameType::UNKNOWN);

    IncreaseNodeHitCount(info, ROOT_NODE_ID);

    AddSampleNodeId(info, IDLE_NODE_ID);

    AddTimeDelta(info, timeDelta);

    SetPreviousTimeStamp(info, task.timeStamp);
}

void SamplesRecord::StatisticStateTime(ProfileInfo* info, int timeDelta)
{
    FrameType state = info->previousState;
    switch (state) {
        case FrameType::MANAGED: {
            info->managedTime += static_cast<uint64_t>(timeDelta);
            break;
        }
        default: {
            info->unknownTime += static_cast<uint64_t>(timeDelta);
        }
    }
}

void SamplesRecord::StringifySampleData(ProfileInfo* info)
{
    sampleData_ = "";
    sampleData_.Append(CString::FormatString("{\"mutatorId\":%d,", info->mutatorId));
    sampleData_.Append(CString::FormatString("\"startTime\":%llu,", info->startTime));
    sampleData_.Append(CString::FormatString("\"endTime\":%llu,", info->stopTime));

    StringifyStateTimeStatistic(info);
    StringifyNodes(info);
    StringifySamples(info);
}

void SamplesRecord::StringifyStateTimeStatistic(ProfileInfo* info)
{
    sampleData_.Append(CString::FormatString("\"managedTime\":%llu,", info->managedTime));
    sampleData_.Append(CString::FormatString("\"unknownTime\":%llu,", info->unknownTime));
}

void SamplesRecord::StringifyNodes(ProfileInfo* info)
{
    sampleData_.Append("\"nodes\":[");
    size_t nodeCount = static_cast<size_t>(info->nodeCount);

    for (size_t i = 0; i < nodeCount; i++) {
        struct CpuProfileNode& node = info->nodes[i];
        struct CodeInfo& codeEntry = node.codeEntry;
        sampleData_.Append(CString::FormatString("{\"id\":%d,", node.id));
        sampleData_.Append("\"callFrame\":{\"functionName\":\"");
        sampleData_.Append(codeEntry.functionName);
        sampleData_.Append(CString::FormatString("\",\"scriptId\":\"%d\",", codeEntry.scriptId));
        sampleData_.Append("\"url\":\"");
        sampleData_.Append(codeEntry.url);
        sampleData_.Append(CString::FormatString("\",\"lineNumber\":%d},", codeEntry.lineNumber));
        sampleData_.Append(CString::FormatString("\"hitCount\":%d,\"children\":[", node.hitCount));

        std::vector<uint64_t> children = node.children;
        size_t childrenCount = children.size();
        for (size_t j = 0; j < childrenCount; j++) {
            sampleData_.Append(CString::FormatString("%d,", children[j]));
        }
        if (childrenCount > 0) {
            sampleData_ = sampleData_.SubStr(0, sampleData_.Length() - 1);
        }
        sampleData_ += "]},";
    }
    sampleData_ = sampleData_.SubStr(0, sampleData_.Length() - 1);
    sampleData_ += "],";
}

void SamplesRecord::StringifySamples(ProfileInfo* info)
{
    std::vector<int>& samples = info->samples;
    std::vector<int>& timeDeltas = info->timeDeltas;

    size_t samplesCount = samples.size();
    if (samplesCount == 0) {
        sampleData_.Append("\"samples\":[],\"timeDeltas\":[]}");
        return;
    }
    CString samplesIdStr = "";
    CString timeDeltasStr = "";
    for (size_t i = 0; i < samplesCount; i++) {
        samplesIdStr.Append(CString::FormatString("%d,", samples[i]));
        timeDeltasStr.Append(CString::FormatString("%d,", timeDeltas[i]));
    }

    samplesIdStr = samplesIdStr.SubStr(0, samplesIdStr.Length() - 1);
    timeDeltasStr = timeDeltasStr.SubStr(0, timeDeltasStr.Length() - 1);

    sampleData_.Append("\"samples\":[");
    sampleData_.Append(samplesIdStr);
    sampleData_.Append("],\"timeDeltas\":[");
    sampleData_.Append(timeDeltasStr);
    sampleData_.Append("]}");
}

void SamplesRecord::DumpProfileInfo()
{
    StringifySampleData(profileInfo_);
    WriteFile();
}

bool SamplesRecord::OpenFile(int fd)
{
    if (fd == -1) {
        return false;
    }

    if (ftruncate(fd, 0) == -1) {
        return false;
    }
    fileDesc_ = fd;
    return true;
}

void SamplesRecord::WriteFile()
{
    int err = write(fileDesc_, sampleData_.Str(), sampleData_.Length());
    if (err == -1) {
        LOG_COMMON(ERROR) << "Write file failed. msg: " << strerror(errno);
    }
}

void SamplesRecord::RunTaskLoop()
{
    while (!taskQueue_.empty()) {
        auto task = taskQueue_.front();
        taskQueue_.pop_front();
        if (task.frameCnt == 0) {
            AddEmptySample(task);
        } else {
            AddSample(task);
        }
    }
}

void SamplesRecord::DoSingleTask(uint64_t previousTimeStemp)
{
    if (IsTimeout(previousTimeStemp)) {
        return;
    }
    if (taskQueue_.empty()) {
        return;
    }
    auto task = taskQueue_.front();
    if (!task.finishParsed) {
        return;
    }
    taskQueue_.pop_front();
    if (task.frameCnt == 0) {
        AddEmptySample(task);
    } else {
        AddSample(task);
    }
}

void SamplesRecord::ParseSampleData(uint64_t previousTimeStemp)
{
    if (taskQueue_.empty()) {
        return;
    }
    for (auto& task : taskQueue_) {
        if (task.finishParsed) {
            continue;
        }
        for (int i = task.checkPoint; i < task.frameCnt; ++i) {
            GetDemangleName(task.funcDescRefs[i]);
            GetUrl(task.funcDescRefs[i]);
            if (IsTimeout(previousTimeStemp)) {
                task.checkPoint = i + 1;
                return;
            }
        }
        task.finishParsed = true;
    }
}

bool SamplesRecord::IsTimeout(uint64_t previousTimeStemp)
{
    uint64_t currentTimeStamp = SamplesRecord::GetMicrosecondsTimeStamp();
    int64_t ts = static_cast<int64_t>(interval_) -
        static_cast<int64_t>(currentTimeStamp - previousTimeStemp);
    if (ts < 0) {
        return true;
    }
    return false;
}

void SamplesRecord::Post(uint64_t mutatorId, std::vector<uint64_t>& FuncDescRefs,
                         std::vector<FrameType>& FrameTypes, std::vector<uint32_t>& LineNumbers)
{
    uint64_t timeStamp = SamplesRecord::GetMicrosecondsTimeStamp();
    SampleTask task(timeStamp, mutatorId, FuncDescRefs, FrameTypes, LineNumbers);
    taskQueue_.push_back(task);
}

std::vector<CodeInfo> SamplesRecord::GetCodeInfos(SampleTask& task)
{
    std::vector<CodeInfo> codeInfos;
    for (int i = 0; i < task.frameCnt; ++i) {
        CodeInfo codeInfo;
        codeInfo.lineNumber = task.lineNumbers[i];
        codeInfo.frameType = task.frameTypes[i];
        codeInfo.funcIdentifier = task.funcDescRefs[i];
        codeInfo.functionName = GetDemangleName(codeInfo.funcIdentifier);
        codeInfo.url = GetUrl(codeInfo.funcIdentifier);
        codeInfo.scriptId = UpdateScriptIdMap(codeInfo.url);
        codeInfos.emplace_back(codeInfo);
    }
    return codeInfos;
}

CString SamplesRecord::GetUrl(uint64_t funcIdentifier)
{
    if (identifierUrlMap_.find(funcIdentifier) != identifierUrlMap_.end()) {
        return identifierUrlMap_[funcIdentifier];
    }
    return ParseUrl(funcIdentifier);
}

CString SamplesRecord::ParseUrl(uint64_t funcIdentifier) { return CString(); }

CString SamplesRecord::GetDemangleName(uint64_t funcIdentifier)
{
    if (identifierFuncnameMap_.find(funcIdentifier) != identifierFuncnameMap_.end()) {
        return identifierFuncnameMap_[funcIdentifier];
    }
    return ParseDemangleName(funcIdentifier);
}

CString SamplesRecord::ParseDemangleName(uint64_t funcIdentifier)
{
    LOG_COMMON(FATAL) << "Unresolved fatal";
    UNREACHABLE_CC();
}
}
