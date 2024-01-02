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

#ifndef ECMASCRIPT_DFX_TRACING_TRACING_H
#define ECMASCRIPT_DFX_TRACING_TRACING_H

#include "ecmascript/dfx/cpu_profiler/samples_record.h"
#include "ecmascript/method.h"
#include "ecmascript/debugger/notification_manager.h"

namespace panda::ecmascript {

struct TraceEventCpuProfileArg {
    std::vector<int> nodes;
    std::vector<int> samples;
    std::vector<int> timeDeltas;
};

struct TraceEvent {
    TraceEvent(std::string cat, std::string name, std::string ph, int64_t pid, int64_t tid)
        : cat_(cat), name_(name), ph_(ph), pid_(pid), tid_(tid)
    {
    }
    ~TraceEvent() = default;

    void SetTs(int64_t ts)
    {
        ts_ = ts;
    }

    void SetDur(int64_t dur)
    {
        dur_ = dur;
    }

    void SetTdur(int64_t tdur)
    {
        tdur_ = tdur;
    }

    void SetTts(int64_t tts)
    {
        tts_ = tts;
    }

    void SetS(const std::string& s)
    {
        s_ = s;
    }

    void SetId(const std::string& Id)
    {
        id_ = Id;
    }

    void SetArgs(const std::string& args)
    {
        args_ = args;
    }

    void SetCpuProfileArgs(TraceEventCpuProfileArg& args)
    {
        cpuProfileArgs_ = args;
    }

    uint64_t GetEventBufferSize();

    std::string cat_;
    std::string name_;
    std::string ph_;
    [[maybe_unused]] int64_t pid_ {0};
    [[maybe_unused]] int64_t tid_ {0};
    int64_t ts_ {0};
    std::string s_;
    std::string id_;
    int64_t dur_ {0};
    int64_t tdur_ {0};
    int64_t tts_ {0};
    std::string args_;
    std::optional<struct TraceEventCpuProfileArg> cpuProfileArgs_;
};

class Tracing : public tooling::RuntimeListener {
public:
    explicit Tracing(const EcmaVM *vm);
    virtual ~Tracing();

    void StartTracing(std::string &categories);
    std::unique_ptr<std::vector<TraceEvent>> StopTracing();
    void TraceEventRecordMemory();
    void TraceEventRecordCpuProfilerStart(struct ProfileInfo* profileInfo);
    void TraceEventRecordCpuProfiler(struct ProfileInfo* profileInfo, int &nodePos, uint32_t &samplePos);
    void TraceEventRecordCpuProfilerEnd(struct ProfileInfo* profileInfo);
    void GetBufferUseage(double &percentFull, uint32_t &eventCount, double &value);
    void TraceEventRecordTracingStart();
    void TraceEventUpdateCpuProfiler(struct ProfileInfo *profileInfo);

    void LoadModule(std::string_view name, std::string_view) override;
    void BytecodePcChanged(JSThread *thread, JSHandle<Method> method, uint32_t bcOffset) override;
    bool HandleDebuggerStmt(JSHandle<Method> method, uint32_t bcOffset) override;
    void VmStart() override;
    void VmDeath() override;
    void NativeCalling(const void *nativeAddress) override;
    void NativeReturn(const void *nativeAddress) override;
    void MethodEntry(JSHandle<Method> method, JSHandle<JSTaggedValue> envHandle) override;
    void MethodExit(JSHandle<Method> method) override;

    bool IsTracing()
    {
        return isTracing_;
    }

private:
    void RecordTraceEvent(TraceEvent &event);
    uint64_t GetTimeStamp();

    const EcmaVM *vm_;
    [[maybe_unused]] std::string categories_;
    bool isTracing_ {false};
    static constexpr uint64_t MAX_BUFFER_SIZE_DEFAULT = 200 * 1024 * 1024;
    uint64_t maxBufferSize_ {MAX_BUFFER_SIZE_DEFAULT};
    uint64_t bufferSize_ {0};
    pthread_t tid_ {0};

    std::unique_ptr<std::vector<TraceEvent>> traceEvents_;
    std::unique_ptr<std::vector<TraceEvent>> traceEventsCpuProfiler_;
    std::mutex lock_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_DFX_TRACING_TRACING_H