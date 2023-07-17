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

#include "ecmascript/dfx/cpu_profiler/samples_record.h"

#include <climits>
#include <sys/syscall.h>
#include <unistd.h>

#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#include "ecmascript/dfx/cpu_profiler/sampling_processor.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/method.h"

namespace panda::ecmascript {
SamplesRecord::SamplesRecord()
{
    profileInfo_ = std::make_unique<struct ProfileInfo>();
    int tid = syscall(SYS_gettid);
    if (tid != -1) {
        profileInfo_->tid = static_cast<uint64_t>(tid);
    }
    samplesQueue_ = new SamplesQueue();
}

SamplesRecord::~SamplesRecord()
{
    if (fileHandle_.is_open()) {
        fileHandle_.close();
    }
    if (samplesQueue_ != nullptr) {
        delete samplesQueue_;
        samplesQueue_ = nullptr;
    }
}

void SamplesRecord::NodeInit()
{
    struct NodeKey nodeKey;
    struct CpuProfileNode methodNode;

    nodeKey.methodKey.methodIdentifier = reinterpret_cast<void *>(INT_MAX - 1);
    nodeMap_.emplace(nodeKey, nodeMap_.size() + 1);
    methodNode.parentId = 0;
    methodNode.codeEntry.functionName = "(root)";
    methodNode.id = 1;
    profileInfo_->nodes[profileInfo_->nodeCount++] = methodNode;

    nodeKey.methodKey.methodIdentifier = reinterpret_cast<void *>(INT_MAX - 2);  // 2：program node id
    nodeMap_.emplace(nodeKey, nodeMap_.size() + 1);
    methodNode.codeEntry.functionName = "(program)";
    methodNode.id = 2;  // 2：program node id
    profileInfo_->nodes[profileInfo_->nodeCount++] = methodNode;
    profileInfo_->nodes[0].children.push_back(methodNode.id);

    nodeKey.methodKey.methodIdentifier = reinterpret_cast<void *>(INT_MAX - 3);  // 3：idle node id
    nodeMap_.emplace(nodeKey, nodeMap_.size() + 1);
    methodNode.codeEntry.functionName = "(idle)";
    methodNode.id = 3;  // 3：idle node id
    profileInfo_->nodes[profileInfo_->nodeCount++] = methodNode;
    profileInfo_->nodes[0].children.push_back(methodNode.id);
}

void SamplesRecord::AddSample(FrameStackAndInfo *frame)
{
    int frameStackLength = frame->frameStackLength;
    if (frameStackLength == 0) {
        AddEmptyStackSample(frame->timeStamp);
        return;
    }

    FrameInfoTempToMap(frame->frameInfoTemps, frame->frameInfoTempsLength);

    struct NodeKey nodeKey;
    struct CpuProfileNode methodNode;
    methodNode.id = 1;
    for (; frameStackLength >= 1; frameStackLength--) {
        nodeKey.methodKey = frame->frameStack[frameStackLength - 1];
        methodNode.parentId = nodeKey.parentId = methodNode.id;
        auto result = nodeMap_.find(nodeKey);
        if (result == nodeMap_.end()) {
            int id = static_cast<int>(nodeMap_.size() + 1);
            nodeMap_.emplace(nodeKey, id);
            previousId_ = methodNode.id = id;
            methodNode.codeEntry = GetMethodInfo(nodeKey.methodKey);
            profileInfo_->nodes[profileInfo_->nodeCount++] = methodNode;
            profileInfo_->nodes[methodNode.parentId - 1].children.push_back(id);
        } else {
            previousId_ = methodNode.id = result->second;
        }
    }

    int sampleNodeId = previousId_ == 0 ? 1 : methodNode.id;
    int timeDelta = static_cast<int>(frame->timeStamp -
        (previousTimeStamp_ == 0 ? profileInfo_->startTime : previousTimeStamp_));

    // delete abnormal sample
    if (timeDelta > static_cast<int>(timeDeltaThreshold_) && previousState_ != RunningState::NAPI) {
        uint32_t size = profileInfo_->samples.size();
        if (size > 0) {
            profileInfo_->samples[size - 1] = PROGRAM_NODE_ID;
        }
        previousState_ = RunningState::OTHER;
    }
    StatisticStateTime(timeDelta, previousState_);
    previousState_ = nodeKey.methodKey.state;
    profileInfo_->nodes[sampleNodeId - 1].hitCount++;
    profileInfo_->samples.push_back(sampleNodeId);
    profileInfo_->timeDeltas.push_back(timeDelta);
    previousTimeStamp_ = frame->timeStamp;
}

void SamplesRecord::AddEmptyStackSample(uint64_t sampleTimeStamp)
{
    int timeDelta = static_cast<int>(sampleTimeStamp -
        (previousTimeStamp_ == 0 ? profileInfo_->startTime : previousTimeStamp_));

    // delete abnormal sample
    if (timeDelta > static_cast<int>(timeDeltaThreshold_) && previousState_ != RunningState::NAPI) {
        uint32_t size = profileInfo_->samples.size();
        if (size > 0) {
            profileInfo_->samples[size - 1] = PROGRAM_NODE_ID;
        }
        previousState_ = RunningState::OTHER;
    }

    StatisticStateTime(timeDelta, previousState_);
    previousState_ = RunningState::OTHER;
    profileInfo_->nodes[1].hitCount++;
    profileInfo_->samples.push_back(PROGRAM_NODE_ID);
    profileInfo_->timeDeltas.push_back(timeDelta);
    previousTimeStamp_ = sampleTimeStamp;
}

void SamplesRecord::StringifySampleData()
{
    sampleData_ += "{\"tid\":"
        + std::to_string(profileInfo_->tid) + ",\"startTime\":"
        + std::to_string(profileInfo_->startTime) + ",\"endTime\":"
        + std::to_string(profileInfo_->stopTime) + ",";

    StringifyStateTimeStatistic();
    StringifyNodes();
    StringifySamples();
}

void SamplesRecord::StringifyStateTimeStatistic()
{
    sampleData_ += "\"gcTime\":"
        + std::to_string(profileInfo_->gcTime) + ",\"cInterpreterTime\":"
        + std::to_string(profileInfo_->cInterpreterTime) + ",\"asmInterpreterTime\":"
        + std::to_string(profileInfo_->asmInterpreterTime) + ",\"aotTime\":"
        + std::to_string(profileInfo_->aotTime) + ",\"builtinTime\":"
        + std::to_string(profileInfo_->builtinTime) + ",\"napiTime\":"
        + std::to_string(profileInfo_->napiTime) + ",\"arkuiEngineTime\":"
        + std::to_string(profileInfo_->arkuiEngineTime) + ",\"runtimeTime\":"
        + std::to_string(profileInfo_->runtimeTime) + ",\"otherTime\":"
        + std::to_string(profileInfo_->otherTime) + ",";
}

void SamplesRecord::StringifyNodes()
{
    sampleData_ += "\"nodes\":[";
    size_t nodeCount = static_cast<size_t>(profileInfo_->nodeCount);
    for (size_t i = 0; i < nodeCount; i++) {
        struct CpuProfileNode node = profileInfo_->nodes[i];
        struct FrameInfo codeEntry = node.codeEntry;
        std::string url = codeEntry.url;
        replace(url.begin(), url.end(), '\\', '/');
        sampleData_ += "{\"id\":"
        + std::to_string(node.id) + ",\"callFrame\":{\"functionName\":\""
        + codeEntry.functionName + "\",\"moduleName\":\""
        + codeEntry.moduleName + "\",\"scriptId\":\""
        + std::to_string(codeEntry.scriptId) + "\",\"url\":\""
        + url + "\",\"lineNumber\":"
        + std::to_string(codeEntry.lineNumber) + ",\"columnNumber\":"
        + std::to_string(codeEntry.columnNumber) + "},\"hitCount\":"
        + std::to_string(node.hitCount) + ",\"children\":[";
        CVector<int> children = node.children;
        size_t childrenCount = children.size();
        for (size_t j = 0; j < childrenCount; j++) {
            sampleData_ += std::to_string(children[j]) + ",";
        }
        if (childrenCount > 0) {
            sampleData_.pop_back();
        }
        sampleData_ += "]},";
    }
    sampleData_.pop_back();
    sampleData_ += "],";
}

void SamplesRecord::StringifySamples()
{
    CVector<int> samples = profileInfo_->samples;
    CVector<int> timeDeltas = profileInfo_->timeDeltas;

    size_t samplesCount = samples.size();
    std::string samplesIdStr = "";
    std::string timeDeltasStr = "";
    for (size_t i = 0; i < samplesCount; i++) {
        samplesIdStr += std::to_string(samples[i]) + ",";
        timeDeltasStr += std::to_string(timeDeltas[i]) + ",";
    }
    samplesIdStr.pop_back();
    timeDeltasStr.pop_back();

    sampleData_ += "\"samples\":[" + samplesIdStr + "],\"timeDeltas\":[" + timeDeltasStr + "]}";
}

/*
 * Description: Finetune samples timedelta when stop cpuprofiler
 * Use two-pointer algorithm to iterate samples and actively recorded napiCallTimeVec_ and napiCallAddrVec_
 *     Accumulate timeDelta and startTime to get the current timestamp
 *     When current timestamp larger than napiCall start time, then
 *         Loop backward PRE_IDX_RANGE times from previous index, find same address napi
 *         If find the same address napi, then call FindSampleAndFinetune to finetune timedelta
 * Parameters: null
 * Return: null
 */
void SamplesRecord::FinetuneSampleData()
{
    CVector<int> &samples = profileInfo_->samples;

    size_t samplesCount = samples.size();           // samples count
    size_t napiCallCount = napiCallTimeVec_.size(); // napiCall count
    size_t sampleIdx = 0;                           // samples index
    size_t napiCallIdx = 0;                         // napiCall index
    uint64_t sampleTime = profileInfo_->startTime;  // current timestamp

    while (sampleIdx < samplesCount && napiCallIdx < napiCallCount) {
        // accumulate timeDelta to get current timestamp until larger than napiCall start time
        sampleTime += static_cast<uint64_t>(profileInfo_->timeDeltas[sampleIdx]);
        if (sampleTime < napiCallTimeVec_[napiCallIdx]) {
            sampleIdx++;
            continue;
        }
        bool findFlag = false;
        size_t findIdx = sampleIdx;
        size_t preIdx = sampleIdx;
        uint64_t preSampleTime = sampleTime -
            static_cast<uint64_t>(profileInfo_->timeDeltas[sampleIdx]); // preIdx's timestamp
        std::string napiFunctionAddr = napiCallAddrVec_[napiCallIdx];
        if (sampleIdx - 1 >= 0) {
            preIdx = sampleIdx - 1;
            preSampleTime -= static_cast<uint64_t>(profileInfo_->timeDeltas[sampleIdx - 1]);
        }
        // loop backward PRE_IDX_RANGE times from previous index, find same address napi
        for (size_t k = preIdx; k - preIdx < PRE_IDX_RANGE && k < samplesCount; k++) {
            std::string samplesFunctionName = profileInfo_->nodes[samples[k] - 1].codeEntry.functionName;
            preSampleTime += static_cast<uint64_t>(profileInfo_->timeDeltas[k]);
            if (samplesFunctionName.find(napiFunctionAddr) != std::string::npos) {
                findFlag = true;
                findIdx = k;
                break;
            }
        }
        // found the same address napi
        if (findFlag) {
            FindSampleAndFinetune(findIdx, napiCallIdx, sampleIdx, preSampleTime, sampleTime);
        } else {
            sampleTime -= static_cast<uint64_t>(profileInfo_->timeDeltas[sampleIdx]);
        }
        napiCallIdx += NAPI_CALL_SETP;
    }
}

/*
 * Description: Finetune samples timedelta when find the same address napi
 *      1. get a continuous sample: loop the samples until find the first sample that is different from findIdx
 *      2. startIdx and endIdx: beginning and end of the continuous sample
 *      3. call FinetuneTimeDeltas to finetune startIdx and endIdx's timedelta
 * Parameters:
 *      1. findIdx: sample index of same address with napi
 *      2. napiCallIdx: napi call index
 *      3. sampleIdx: sample index
 *      4. startSampleTime: start sample timestamp
 *      5. sampleTime: current timestamp
 * Return: null
 */
void SamplesRecord::FindSampleAndFinetune(size_t findIdx, size_t napiCallIdx, size_t &sampleIdx,
                                          uint64_t startSampleTime, uint64_t &sampleTime)
{
    size_t startIdx = findIdx;
    size_t endIdx = findIdx + 1;
    size_t samplesCount = profileInfo_->samples.size();
    uint64_t endSampleTime = startSampleTime;                 // end sample timestamp
    uint64_t startNapiTime = napiCallTimeVec_[napiCallIdx];   // call napi start timestamp
    uint64_t endNapiTime = napiCallTimeVec_[napiCallIdx + 1]; // call napi end timestamp
    // get a continuous sample, accumulate endSampleTime but lack last timeDeltas
    for (; endIdx < samplesCount && profileInfo_->samples[endIdx - 1] == profileInfo_->samples[endIdx]; endIdx++) {
        endSampleTime += static_cast<uint64_t>(profileInfo_->timeDeltas[endIdx]);
    }
    // finetune startIdx‘s timedelta
    FinetuneTimeDeltas(startIdx, startNapiTime, startSampleTime, false);
    // if the continuous sample' size is 1, endSampleTime need to adjust
    if (startIdx + 1 == endIdx) {
        endSampleTime -= (startSampleTime - startNapiTime);
    }
    // finetune endIdx's timedelta
    FinetuneTimeDeltas(endIdx, endNapiTime, endSampleTime, true);
    sampleTime = endSampleTime;
    sampleIdx = endIdx;
}

/*
 * Description: Finetune time deltas
 * Parameters:
 *      1. idx: sample index
 *      2. napiTime: napi timestamp
 *      3. sampleTime: sample timestamp
 *      4. isEndSample: if is endIdx, isEndSample is true, else isEndSample is false
 * Return: null
 */
void SamplesRecord::FinetuneTimeDeltas(size_t idx, uint64_t napiTime, uint64_t &sampleTime, bool isEndSample)
{
    // timeDeltas[idx] minus a period of time, timeDeltas[idx+1] needs to add the same time
    if (isEndSample) {
        // if is endIdx, sampleTime add endTimeDelta is real current timestamp
        int endTimeDelta = profileInfo_->timeDeltas[idx];
        profileInfo_->timeDeltas[idx] -= static_cast<int>(sampleTime - napiTime) + endTimeDelta;
        profileInfo_->timeDeltas[idx + 1] += static_cast<int>(sampleTime - napiTime) + endTimeDelta;
    } else {
        profileInfo_->timeDeltas[idx] -= static_cast<int>(sampleTime - napiTime);
        profileInfo_->timeDeltas[idx + 1] += static_cast<int>(sampleTime - napiTime);
    }

    // if timeDeltas[idx] < 0, timeDeltas[idx] = MIN_TIME_DELTA
    if (profileInfo_->timeDeltas[idx] < 0) {
        // timeDelta is added part, needs other sample reduce to balance
        int timeDelta = MIN_TIME_DELTA - profileInfo_->timeDeltas[idx];
        // profileInfo_->timeDeltas[idx - 1] to reduce
        if (idx - 1 >= 0 && profileInfo_->timeDeltas[idx - 1] > timeDelta) {
            profileInfo_->timeDeltas[idx - 1] -= timeDelta;
            if (isEndSample) {
                sampleTime -= static_cast<uint64_t>(timeDelta);
            }
        // if timeDeltas[idx - 1] < timeDelta, timeDeltas[idx - 1] = MIN_TIME_DELTA
        } else if (idx - 1 >= 0) {
            // The remaining timeDeltas[idx + 1] to reduce
            profileInfo_->timeDeltas[idx + 1] -= timeDelta - profileInfo_->timeDeltas[idx - 1] + MIN_TIME_DELTA;
            if (isEndSample) {
                sampleTime -= static_cast<uint64_t>(profileInfo_->timeDeltas[idx - 1] - MIN_TIME_DELTA);
            }
            profileInfo_->timeDeltas[idx - 1] = MIN_TIME_DELTA;
        } else {
            profileInfo_->timeDeltas[idx + 1] -= timeDelta;
        }
        profileInfo_->timeDeltas[idx] = MIN_TIME_DELTA;
    }

    // if timeDeltas[idx + 1] < 0, timeDeltas equals MIN_TIME_DELTA, timeDeltas[idx] reduce added part
    if (profileInfo_->timeDeltas[idx + 1] < 0) {
        int timeDelta = MIN_TIME_DELTA - profileInfo_->timeDeltas[idx];
        profileInfo_->timeDeltas[idx] -= timeDelta;
        profileInfo_->timeDeltas[idx + 1] = MIN_TIME_DELTA;
    }
}

int SamplesRecord::GetMethodNodeCount() const
{
    return profileInfo_->nodeCount;
}

std::string SamplesRecord::GetSampleData() const
{
    return sampleData_;
}

struct FrameInfo SamplesRecord::GetMethodInfo(struct MethodKey &methodKey)
{
    struct FrameInfo entry;
    auto iter = stackInfoMap_.find(methodKey);
    if (iter != stackInfoMap_.end()) {
        entry = iter->second;
    }
    return entry;
}

std::string SamplesRecord::AddRunningState(char *functionName, RunningState state, kungfu::DeoptType type)
{
    std::string temp = functionName;
    if (state == RunningState::AOT && type != kungfu::DeoptType::NOTCHECK) {
        state = RunningState::AINT;
    }
    switch (state) {
        case RunningState::GC:
            temp.append("(GC)");
            break;
        case RunningState::CINT:
            if (enableVMTag_) {
                temp.append("(CINT)");
            }
            break;
        case RunningState::AINT:
            if (enableVMTag_) {
                temp.append("(AINT)");
            }
            break;
        case RunningState::AOT:
            if (enableVMTag_) {
                temp.append("(AOT)");
            }
            break;
        case RunningState::BUILTIN:
            temp.append("(BUILTIN)");
            break;
        case RunningState::NAPI:
            temp.append("(NAPI)");
            break;
        case RunningState::ARKUI_ENGINE:
            temp.append("(ARKUI_ENGINE)");
            break;
        case RunningState::RUNTIME:
            if (enableVMTag_) {
                temp.append("(RUNTIME)");
            }
            break;
        default:
            break;
    }
    if (type != kungfu::DeoptType::NOTCHECK && enableVMTag_) {
        std::string typeCheckStr = "(DEOPT:" + Deoptimizier::DisplayItems(type) + ")";
        temp.append(typeCheckStr);
    }
    return temp;
}

void SamplesRecord::StatisticStateTime(int timeDelta, RunningState state)
{
    switch (state) {
        case RunningState::GC: {
            profileInfo_->gcTime += static_cast<uint64_t>(timeDelta);
            return;
        }
        case RunningState::CINT: {
            profileInfo_->cInterpreterTime += static_cast<uint64_t>(timeDelta);
            return;
        }
        case RunningState::AINT: {
            profileInfo_->asmInterpreterTime += static_cast<uint64_t>(timeDelta);
            return;
        }
        case RunningState::AOT: {
            profileInfo_->aotTime += static_cast<uint64_t>(timeDelta);
            return;
        }
        case RunningState::BUILTIN: {
            profileInfo_->builtinTime += static_cast<uint64_t>(timeDelta);
            return;
        }
        case RunningState::NAPI: {
            profileInfo_->napiTime += static_cast<uint64_t>(timeDelta);
            return;
        }
        case RunningState::ARKUI_ENGINE: {
            profileInfo_->arkuiEngineTime += static_cast<uint64_t>(timeDelta);
            return;
        }
        case RunningState::RUNTIME: {
            profileInfo_->runtimeTime += static_cast<uint64_t>(timeDelta);
            return;
        }
        default: {
            profileInfo_->otherTime += static_cast<uint64_t>(timeDelta);
            return;
        }
    }
}

void SamplesRecord::SetThreadStartTime(uint64_t threadStartTime)
{
    profileInfo_->startTime = threadStartTime;
}

void SamplesRecord::SetThreadStopTime()
{
    profileInfo_->stopTime = previousTimeStamp_;
}

void SamplesRecord::SetStartsampleData(std::string sampleData)
{
    sampleData_ += sampleData;
}

void SamplesRecord::SetFileName(std::string &fileName)
{
    fileName_ = fileName;
}

const std::string SamplesRecord::GetFileName() const
{
    return fileName_;
}

void SamplesRecord::ClearSampleData()
{
    sampleData_.clear();
}

std::unique_ptr<struct ProfileInfo> SamplesRecord::GetProfileInfo()
{
    return std::move(profileInfo_);
}

void SamplesRecord::SetBeforeGetCallNapiStackFlag(bool flag)
{
    beforeCallNapi_.store(flag);
}

bool SamplesRecord::GetBeforeGetCallNapiStackFlag()
{
    return beforeCallNapi_.load();
}

void SamplesRecord::SetAfterGetCallNapiStackFlag(bool flag)
{
    afterCallNapi_.store(flag);
}

bool SamplesRecord::GetAfterGetCallNapiStackFlag()
{
    return afterCallNapi_.load();
}

void SamplesRecord::SetCallNapiFlag(bool flag)
{
    callNapi_.store(flag);
}

bool SamplesRecord::GetCallNapiFlag()
{
    return callNapi_.load();
}

int SamplesRecord::SemInit(int index, int pshared, int value)
{
    return sem_init(&sem_[index], pshared, value);
}

int SamplesRecord::SemPost(int index)
{
    return sem_post(&sem_[index]);
}

int SamplesRecord::SemWait(int index)
{
    return sem_wait(&sem_[index]);
}

int SamplesRecord::SemDestroy(int index)
{
    return sem_destroy(&sem_[index]);
}

const CMap<struct MethodKey, struct FrameInfo> &SamplesRecord::GetStackInfo() const
{
    return stackInfoMap_;
}

void SamplesRecord::InsertStackInfo(struct MethodKey &methodKey, struct FrameInfo &codeEntry)
{
    stackInfoMap_.emplace(methodKey, codeEntry);
}

bool SamplesRecord::PushFrameStack(struct MethodKey &methodKey)
{
    if (UNLIKELY(frameStackLength_ >= MAX_STACK_SIZE)) {
        return false;
    }
    frameStack_[frameStackLength_++] = methodKey;
    return true;
}

bool SamplesRecord::PushNapiFrameStack(struct MethodKey &methodKey)
{
    if (UNLIKELY(napiFrameStack_.size() >= MAX_STACK_SIZE)) {
        return false;
    }
    napiFrameStack_.push_back(methodKey);
    return true;
}

void SamplesRecord::ClearNapiStack()
{
    napiFrameStack_.clear();
    napiFrameInfoTemps_.clear();
}

void SamplesRecord::ClearNapiCall()
{
    napiCallTimeVec_.clear();
    napiCallAddrVec_.clear();
}

int SamplesRecord::GetNapiFrameStackLength()
{
    return napiFrameStack_.size();
}

bool SamplesRecord::GetGcState() const
{
    return gcState_.load();
}

void SamplesRecord::SetGcState(bool gcState)
{
    gcState_.store(gcState);
}

bool SamplesRecord::GetRuntimeState() const
{
    return runtimeState_.load();
}

void SamplesRecord::SetRuntimeState(bool runtimeState)
{
    runtimeState_.store(runtimeState);
}

bool SamplesRecord::GetIsStart() const
{
    return isStart_.load();
}

void SamplesRecord::SetIsStart(bool isStart)
{
    isStart_.store(isStart);
}

bool SamplesRecord::PushStackInfo(const FrameInfoTemp &frameInfoTemp)
{
    if (UNLIKELY(frameInfoTempLength_ >= MAX_STACK_SIZE)) {
        return false;
    }
    frameInfoTemps_[frameInfoTempLength_++] = frameInfoTemp;
    return true;
}

bool SamplesRecord::PushNapiStackInfo(const FrameInfoTemp &frameInfoTemp)
{
    if (UNLIKELY(napiFrameInfoTemps_.size() == MAX_STACK_SIZE)) {
        return false;
    }
    napiFrameInfoTemps_.push_back(frameInfoTemp);
    return true;
}

std::string SamplesRecord::GetModuleName(char *recordName)
{
    std::string recordNameStr = recordName;
    std::string::size_type atPos = recordNameStr.find("@");
    if (atPos == std::string::npos) {
        return "";
    }

    std::string::size_type slashPos = recordNameStr.rfind("/", atPos);
    if (slashPos == std::string::npos) {
        return "";
    }

    return recordNameStr.substr(slashPos + 1, atPos - slashPos - 1);
}

void SamplesRecord::FrameInfoTempToMap(FrameInfoTemp *frameInfoTemps, int frameInfoTempLength)
{
    if (frameInfoTempLength == 0) {
        return;
    }
    struct FrameInfo frameInfo;
    for (int i = 0; i < frameInfoTempLength; ++i) {
        frameInfo.url = frameInfoTemps[i].url;
        auto iter = scriptIdMap_.find(frameInfo.url);
        if (iter == scriptIdMap_.end()) {
            scriptIdMap_.emplace(frameInfo.url, scriptIdMap_.size() + 1);
            frameInfo.scriptId = static_cast<int>(scriptIdMap_.size());
        } else {
            frameInfo.scriptId = iter->second;
        }
        frameInfo.functionName = AddRunningState(frameInfoTemps[i].functionName,
                                                 frameInfoTemps[i].methodKey.state,
                                                 frameInfoTemps[i].methodKey.deoptType);
        if (strlen(frameInfoTemps[i].recordName) != 0) {
            frameInfo.moduleName = GetModuleName(frameInfoTemps[i].recordName);
        }
        frameInfo.columnNumber = frameInfoTemps[i].columnNumber;
        frameInfo.lineNumber = frameInfoTemps[i].lineNumber;
        stackInfoMap_.emplace(frameInfoTemps[i].methodKey, frameInfo);
    }
    frameInfoTempLength_ = 0;
}

void SamplesRecord::NapiFrameInfoTempToMap()
{
    size_t length = napiFrameInfoTemps_.size();
    if (length == 0) {
        return;
    }
    struct FrameInfo frameInfo;
    for (size_t i = 0; i < length; ++i) {
        frameInfo.url = napiFrameInfoTemps_[i].url;
        auto iter = scriptIdMap_.find(frameInfo.url);
        if (iter == scriptIdMap_.end()) {
            scriptIdMap_.emplace(frameInfo.url, scriptIdMap_.size() + 1);
            frameInfo.scriptId = static_cast<int>(scriptIdMap_.size());
        } else {
            frameInfo.scriptId = iter->second;
        }
        frameInfo.functionName = AddRunningState(napiFrameInfoTemps_[i].functionName,
                                                 napiFrameInfoTemps_[i].methodKey.state,
                                                 napiFrameInfoTemps_[i].methodKey.deoptType);
        if (strlen(napiFrameInfoTemps_[i].recordName) != 0) {
            frameInfo.moduleName = GetModuleName(napiFrameInfoTemps_[i].recordName);
        }
        frameInfo.columnNumber = napiFrameInfoTemps_[i].columnNumber;
        frameInfo.lineNumber = napiFrameInfoTemps_[i].lineNumber;
        stackInfoMap_.emplace(napiFrameInfoTemps_[i].methodKey, frameInfo);
    }
}

int SamplesRecord::GetframeStackLength() const
{
    return frameStackLength_;
}

void SamplesRecord::RecordCallNapiTime(uint64_t currentTime)
{
    napiCallTimeVec_.emplace_back(currentTime);
}

void SamplesRecord::RecordCallNapiAddr(const std::string &methodAddr)
{
    napiCallAddrVec_.emplace_back(methodAddr);
}

void SamplesRecord::PostFrame()
{
    samplesQueue_->PostFrame(frameInfoTemps_, frameStack_, frameInfoTempLength_, frameStackLength_);
}

void SamplesRecord::PostNapiFrame()
{
    samplesQueue_->PostNapiFrame(napiFrameInfoTemps_, napiFrameStack_);
}

void SamplesRecord::ResetFrameLength()
{
    frameStackLength_ = 0;
    frameInfoTempLength_ = 0;
}

uint64_t SamplesRecord::GetCallTimeStamp()
{
    return callTimeStamp_;
}

void SamplesRecord::SetCallTimeStamp(uint64_t timeStamp)
{
    callTimeStamp_ = timeStamp;
}

// SamplesQueue
void SamplesQueue::PostFrame(FrameInfoTemp *frameInfoTemps, MethodKey *frameStack,
                             int frameInfoTempsLength, int frameStackLength)
{
    os::memory::LockHolder holder(mtx_);
    if (!IsFull()) {
        // frameInfoTemps
        for (int i = 0; i < frameInfoTempsLength; i++) {
            CheckAndCopy(frames_[rear_].frameInfoTemps[i].functionName,
                sizeof(frames_[rear_].frameInfoTemps[i].functionName), frameInfoTemps[i].functionName);
            CheckAndCopy(frames_[rear_].frameInfoTemps[i].recordName,
                sizeof(frames_[rear_].frameInfoTemps[i].recordName), frameInfoTemps[i].recordName);
            frames_[rear_].frameInfoTemps[i].columnNumber = frameInfoTemps[i].columnNumber;
            frames_[rear_].frameInfoTemps[i].lineNumber = frameInfoTemps[i].lineNumber;
            frames_[rear_].frameInfoTemps[i].scriptId = frameInfoTemps[i].scriptId;
            CheckAndCopy(frames_[rear_].frameInfoTemps[i].url,
                sizeof(frames_[rear_].frameInfoTemps[i].url), frameInfoTemps[i].url);
            frames_[rear_].frameInfoTemps[i].methodKey.methodIdentifier = frameInfoTemps[i].methodKey.methodIdentifier;
            frames_[rear_].frameInfoTemps[i].methodKey.state = frameInfoTemps[i].methodKey.state;
        }
        // frameStack
        for (int i = 0; i < frameStackLength; i++) {
            frames_[rear_].frameStack[i].methodIdentifier = frameStack[i].methodIdentifier;
            frames_[rear_].frameStack[i].state = frameStack[i].state;
        }
        // frameStackLength
        frames_[rear_].frameStackLength = frameStackLength;
        // frameInfoTempsLength
        frames_[rear_].frameInfoTempsLength = frameInfoTempsLength;
        // timeStamp
        frames_[rear_].timeStamp = SamplingProcessor::GetMicrosecondsTimeStamp();

        rear_ = (rear_ + 1) % QUEUE_CAPACITY;
    }
}

void SamplesQueue::PostNapiFrame(CVector<FrameInfoTemp> &napiFrameInfoTemps,
                                 CVector<MethodKey> &napiFrameStack)
{
    os::memory::LockHolder holder(mtx_);
    if (!IsFull()) {
        size_t frameInfoTempsLength = napiFrameInfoTemps.size();
        size_t frameStackLength = napiFrameStack.size();
        // napiFrameInfoTemps
        for (size_t i = 0; i < frameInfoTempsLength; i++) {
            CheckAndCopy(frames_[rear_].frameInfoTemps[i].functionName,
                sizeof(frames_[rear_].frameInfoTemps[i].functionName), napiFrameInfoTemps[i].functionName);
            CheckAndCopy(frames_[rear_].frameInfoTemps[i].recordName,
                sizeof(frames_[rear_].frameInfoTemps[i].recordName), napiFrameInfoTemps[i].recordName);
            frames_[rear_].frameInfoTemps[i].columnNumber = napiFrameInfoTemps[i].columnNumber;
            frames_[rear_].frameInfoTemps[i].lineNumber = napiFrameInfoTemps[i].lineNumber;
            frames_[rear_].frameInfoTemps[i].scriptId = napiFrameInfoTemps[i].scriptId;
            CheckAndCopy(frames_[rear_].frameInfoTemps[i].url,
                sizeof(frames_[rear_].frameInfoTemps[i].url), napiFrameInfoTemps[i].url);
            frames_[rear_].frameInfoTemps[i].methodKey.methodIdentifier =
                napiFrameInfoTemps[i].methodKey.methodIdentifier;
            frames_[rear_].frameInfoTemps[i].methodKey.state = napiFrameInfoTemps[i].methodKey.state;
        }
        // napiFrameStack
        for (size_t i = 0; i < frameStackLength; i++) {
            frames_[rear_].frameStack[i].methodIdentifier = napiFrameStack[i].methodIdentifier;
            frames_[rear_].frameStack[i].state = napiFrameStack[i].state;
        }
        // frameStackLength
        frames_[rear_].frameStackLength = frameStackLength;
        // frameInfoTempsLength
        frames_[rear_].frameInfoTempsLength = frameInfoTempsLength;
        // timeStamp
        frames_[rear_].timeStamp = SamplingProcessor::GetMicrosecondsTimeStamp();

        rear_ = (rear_ + 1) % QUEUE_CAPACITY;
    }
}

FrameStackAndInfo *SamplesQueue::PopFrame()
{
    os::memory::LockHolder holder(mtx_);
    if (!IsEmpty()) {
        FrameStackAndInfo *frame = &frames_[front_];
        front_ = (front_ + 1) % QUEUE_CAPACITY;
        return frame;
    }
    return nullptr;
}

bool SamplesQueue::IsEmpty()
{
    return front_ == rear_;
}

bool SamplesQueue::IsFull()
{
    return (rear_ + 1) % QUEUE_CAPACITY == front_;
}

int SamplesQueue::GetSize()
{
    return (rear_ + QUEUE_CAPACITY - front_) % QUEUE_CAPACITY;
}

int SamplesQueue::GetFrontIndex()
{
    return front_;
}

int SamplesQueue::GetRearIndex()
{
    return rear_;
}

bool SamplesQueue::CheckAndCopy(char *dest, size_t length, const char *src) const
{
    int srcLength = strlen(src);
    if (length <= static_cast<size_t>(srcLength) || strcpy_s(dest, srcLength + 1, src) != EOK) {
        LOG_ECMA(ERROR) << "SamplesQueue PostFrame strcpy_s failed, maybe srcLength more than destLength";
        return false;
    }
    dest[srcLength] = '\0';
    return true;
}
} // namespace panda::ecmascript
