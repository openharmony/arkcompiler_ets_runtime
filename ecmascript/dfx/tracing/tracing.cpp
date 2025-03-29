/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/dfx/tracing/tracing.h"
#include "ecmascript/debugger/js_debugger_manager.h"

namespace panda::ecmascript {
uint64_t TraceEvent::GetEventBufferSize()
{
    uint64_t size = sizeof(*this) + cat_.size() + name_.size() + ph_.size() + args_.size() + s_.size() + id_.size();
    if (cpuProfileArgs_.has_value()) {
        size += cpuProfileArgs_.value().nodes.size() * sizeof(int);
        size += cpuProfileArgs_.value().samples.size() * sizeof(int);
        size += cpuProfileArgs_.value().timeDeltas.size() * sizeof(int);
    }
    return size;
}

Tracing::Tracing(const EcmaVM *vm) : vm_(vm)
{
    traceEvents_ = std::make_unique<std::vector<TraceEvent>>();
    traceEventsCpuProfiler_ = std::make_unique<std::vector<TraceEvent>>();
}

Tracing::~Tracing()
{
}

void Tracing::StartTracing(std::string &categories)
{
    if (isTracing_) {
        return;
    }

    categories_ = categories;
    if (categories_.find("cpu_profiler") != std::string::npos) {
        panda::JSNApi::SetProfilerState(vm_, true);
        panda::DFXJSNApi::StartCpuProfilerForInfo(vm_);
    }

    tid_ = static_cast<pthread_t>(syscall(SYS_gettid));
    isTracing_ = true;
    vm_->GetJsDebuggerManager()->GetNotificationManager()->AddListener(this);
    vm_->GetJSThread()->SetIsTracing(true);

    TraceEventRecordTracingStart();
    return;
}

std::unique_ptr<std::vector<TraceEvent>> Tracing::StopTracing()
{
    isTracing_ = false;
    vm_->GetJsDebuggerManager()->GetNotificationManager()->RemoveListener(this);
    if (categories_.find("cpu_profiler") != std::string::npos) {
        std::unique_ptr<ProfileInfo> profileInfo = panda::DFXJSNApi::StopCpuProfilerForInfo(vm_);
        panda::JSNApi::SetProfilerState(vm_, false);
        if (profileInfo) {
            TraceEventUpdateCpuProfiler(profileInfo.get());
        }
    }
    vm_->GetJSThread()->SetIsTracing(false);
    return std::move(traceEvents_);
}

uint64_t Tracing::GetTimeStamp()
{
    const int USEC_PER_SEC = 1000 * 1000;
    const int NSEC_PER_USEC = 1000;
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return time.tv_sec * USEC_PER_SEC + time.tv_nsec / NSEC_PER_USEC;
}

void Tracing::RecordTraceEvent(TraceEvent &event)
{
    bufferSize_ += event.GetEventBufferSize();

    std::lock_guard<std::mutex> lock(lock_);
    traceEvents_->emplace_back(event);
}

void Tracing::TraceEventRecordTracingStart()
{
    int64_t ts = static_cast<int64_t>(GetTimeStamp());
    std::string args = "{\"data\":{\"frameTreeNodeId\":1,\"frames\":[{";
    args += "\"frame\":\"0\",\"name\":\"\",";
    args += "\"processId\":" + std::to_string(getpid()) + ",";
    args += "\"url\":\"https://url not exist/\"}],";
    args += "\"persistentIds\":true}}";

    TraceEvent event("disabled-by-default-devtools.timeline", "TracingStartedInBrowser", "I", getpid(), tid_);
    event.SetTs(ts);
    event.SetTts(ts);
    event.SetS("t");
    event.SetArgs(args);

    RecordTraceEvent(event);
}

void Tracing::TraceEventRecordMemory()
{
    if (!isTracing_) {
        return;
    }

    int64_t ts = static_cast<int64_t>(GetTimeStamp());
    TraceEvent event("disabled-by-default-devtools.timeline", "UpdateCounters", "I", getpid(), tid_);
    event.SetTs(ts);
    event.SetTts(ts);
    event.SetS("t");
    std::string args = "{\"data\":{\"jsHeapSizeUsed\":" + std::to_string(DFXJSNApi::GetHeapUsedSize(vm_)) + "}}";
    event.SetArgs(args);

    RecordTraceEvent(event);
}

void Tracing::TraceEventRecordCpuProfilerStart(struct ProfileInfo* profileInfo)
{
    int64_t ts = static_cast<int64_t>(GetTimeStamp());
    std::string args = "{\"data\":{\"startTime\":" + std::to_string(profileInfo->startTime) + "}}";
    TraceEvent event("disabled-by-default-v8.cpu_profiler", "Profile", "P", getpid(), tid_);
    event.SetTs(ts);
    event.SetTts(ts);
    event.SetId("0x1");
    event.SetArgs(args);

    bufferSize_ += event.GetEventBufferSize();
    traceEventsCpuProfiler_->emplace_back(event);
}

void Tracing::TraceEventRecordCpuProfiler(struct ProfileInfo* profileInfo, int &nodePos, uint32_t &samplePos)
{
    if (!isTracing_) {
        return;
    }

    int64_t ts = static_cast<int64_t>(GetTimeStamp());
    TraceEvent event("disabled-by-default-v8.cpu_profiler", "ProfileChunk", "P", getpid(), tid_);
    event.SetTs(ts);
    event.SetTts(ts);
    event.SetId("0x1");

    struct TraceEventCpuProfileArg args;
    if (nodePos >= 0 && profileInfo->nodeCount > nodePos) {
        for (int i = nodePos; i < profileInfo->nodeCount; ++i) {
            args.nodes.emplace_back(profileInfo->nodes[i].id);
        }
        nodePos = profileInfo->nodeCount;
    }

    std::copy(profileInfo->samples.begin() + samplePos, profileInfo->samples.end(),
        std::back_inserter(args.samples));
    std::copy(profileInfo->timeDeltas.begin() + samplePos, profileInfo->timeDeltas.end(),
        std::back_inserter(args.timeDeltas));
    samplePos = profileInfo->samples.size();

    event.SetCpuProfileArgs(args);

    bufferSize_ += event.GetEventBufferSize();
    traceEventsCpuProfiler_->emplace_back(event);
}

void Tracing::TraceEventUpdateCpuProfiler(struct ProfileInfo *profileInfo)
{
    for (auto &event : *traceEventsCpuProfiler_) {
        if (!event.cpuProfileArgs_.has_value()) {
            traceEvents_->emplace_back(event);
            continue;
        }

        struct TraceEventCpuProfileArg &cpuProfileArg = event.cpuProfileArgs_.value();
        std::string args = "{\"data\":{\"cpuProfile\":{";
        // nodes
        if (cpuProfileArg.nodes.size() > 0) {
            args += "\"nodes\": [";
            for (auto &nodeId : cpuProfileArg.nodes) {
                struct CpuProfileNode &nodeInfo = profileInfo->nodes[nodeId - 1];
                args += "{\"callFrame\":{\"codeType\":\"JS\",";
                if (nodeInfo.codeEntry.columnNumber != -1) {
                    args += "\"columnNumber\":" + std::to_string(nodeInfo.codeEntry.columnNumber) + ",";
                }
                args += "\"functionName\":\"" + nodeInfo.codeEntry.functionName + "\",";
                if (nodeInfo.codeEntry.lineNumber != -1) {
                    args += "\"lineNumber\":" + std::to_string(nodeInfo.codeEntry.lineNumber) + ",";
                }
                args += "\"scriptId\":" + std::to_string(nodeInfo.codeEntry.scriptId) + ",";
                if (nodeInfo.codeEntry.scriptId != 0) {
                    args += "\"url\":\"" + nodeInfo.codeEntry.url + "\"";
                } else {
                    args.pop_back();
                }
                args += "},";
                args += "\"id\":" + std::to_string(nodeInfo.id) + ",";
                if (nodeInfo.parentId != 0) {
                    args += "\"parent\":" + std::to_string(nodeInfo.parentId) + ",";
                }
                args += "\"hitCount\":" + std::to_string(nodeInfo.hitCount) + ",";
                args += "\"children\":[";
                for (auto &it : nodeInfo.children) {
                    args += std::to_string(it) + ",";
                }
                if (nodeInfo.children.size() != 0) {
                    args.pop_back();
                }
                args += "]},";
            }
            args.pop_back();
            args += "],";
        }

        // samples
        args += "\"samples\": [";
        for (auto sample : cpuProfileArg.samples) {
            args += std::to_string(sample) + ",";
        }
        args.pop_back();
        args += "]},";

        // lines
        args += "\"lines\": [";
        for (auto sample : cpuProfileArg.samples) {
            args += std::to_string(profileInfo->nodes[profileInfo->samples[sample - 1]].codeEntry.lineNumber + 1) + ",";
        }
        args.pop_back();
        args += "],";

        // timeDeltas
        args += "\"timeDeltas\": [";
        for (auto timeDelta : cpuProfileArg.timeDeltas) {
            args += std::to_string(timeDelta) + ",";
        }
        args.pop_back();
        args += "]}}";

        event.SetArgs(args);
        traceEvents_->emplace_back(event);
    }
}

void Tracing::TraceEventRecordCpuProfilerEnd(struct ProfileInfo* profileInfo)
{
    int64_t ts = static_cast<int64_t>(GetTimeStamp());
    std::string args = "{\"data\":{\"endTime\":" + std::to_string(profileInfo->stopTime) + "}}";
    TraceEvent event("disabled-by-default-v8.cpu_profiler", "ProfileChunk", "P", getpid(), tid_);
    event.SetTs(ts);
    event.SetTts(ts);
    event.SetId("0x1");
    event.SetArgs(args);

    bufferSize_ += event.GetEventBufferSize();
    traceEventsCpuProfiler_->emplace_back(event);
}

void Tracing::GetBufferUseage(double &percentFull, uint32_t &eventCount, double &value)
{
    percentFull = (bufferSize_ >= maxBufferSize_) ? 1.0 : static_cast<double>(bufferSize_) / maxBufferSize_;
    eventCount = 0;
    value = percentFull;
}

void Tracing::GenerateAsyncFrames([[maybe_unused]] std::shared_ptr<AsyncStack> asyncStack,
                                  [[maybe_unused]] bool skipTopFrame)
{
    return;
}

void Tracing::LoadModule([[maybe_unused]] std::string_view name, [[maybe_unused]] std::string_view)
{
    return;
}
void Tracing::BytecodePcChanged([[maybe_unused]] JSThread *thread, [[maybe_unused]] JSHandle<Method> method,
                                [[maybe_unused]] uint32_t bcOffset)
{
    return;
}
bool Tracing::HandleDebuggerStmt([[maybe_unused]] JSHandle<Method> method, [[maybe_unused]] uint32_t bcOffset)
{
    return true;
}
void Tracing::VmStart()
{
    return;
}
void Tracing::VmDeath()
{
    return;
}
void Tracing::NativeCalling([[maybe_unused]] const void *nativeAddress)
{
    return;
}
void Tracing::NativeReturn([[maybe_unused]] const void *nativeAddress)
{
    return;
}
void Tracing::MethodEntry([[maybe_unused]] JSHandle<Method> method, [[maybe_unused]] JSHandle<JSTaggedValue> envHandle)
{
    return;
}
void Tracing::MethodExit([[maybe_unused]] JSHandle<Method> method)
{
    TraceEventRecordMemory();
    return;
}
}  // namespace panda::ecmascript
