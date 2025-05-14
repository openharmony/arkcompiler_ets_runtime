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
#ifndef ARK_COMMON_ALLOC_DATA_H
#define ARK_COMMON_ALLOC_DATA_H
#include "common_components/common_runtime/src/unwind_stack/gc_stack_info.h"
#include "common_components/common_runtime/src/inspector/heap_data.h"
#include "common_components/common_runtime/src/inspector/heap_snapshot_json_serializer.h"
namespace panda {
struct TraceFunctionInfo {
    CString functionName;
    CString scriptName;
    CString url = "";
    int32_t line;
    int32_t column = -1;
};

struct TraceNodeField {
    int32_t id = 0; // Unique ID
    int32_t functionInfoIndex;
    int32_t selfSize;
    std::vector<TraceNodeField*> children;
};

struct Sample {
    int32_t size;
    int32_t nodeId;
    int32_t orinal;
};

class ArkAllocData {
public:
    static ArkAllocData* GetArkAllocData();
    static void SetArkAllocData();
    TraceNodeField* FindNode(const FrameAddress*, const char*);
    int32_t FindKey(const FrameAddress*, const char*);
    void DeleteArkAllocData();
    bool IsRecording() { return recording_.load(); };
    void SetRecording(bool isRecording) { recording_.store(isRecording, std::memory_order_release); };
    void DeleteAllNode(TraceNodeField* node);
    void SerializeArkAllocData();
    void SerializeSamples();
    void SerializeCallFrames();
    void SerializeStats();
    void SerializeFunctionInfo(int32_t idx);
    void SerializeEachFrame(TraceNodeField* node);
    void InitAllocParam();
    void InitRoot();
    void RecordAllocNodes(const TypeInfo* klass, uint32_t size);
    int32_t SetNodeID() { return ++traceNodeID_; };
    friend class AllocStackInfo;
private:
    std::unordered_map<int32_t, TraceNodeField*> traceNodeMap_;
    TraceNodeField* traceNodeHead_; // ROOT node
    std::vector<Sample*> samples_;
    std::vector<TraceFunctionInfo*> traceFunctionInfo_;
    int32_t sampSize_;
    int32_t allocSize_;
    int32_t traceNodeID_ = 0;
    StreamWriter* writer_ = nullptr;
    std::atomic<bool> recording_{ false };
    std::mutex sharedMtx_;
};

class AllocStackInfo : public GCStackInfo {
public:
    int32_t ProcessTraceInfo(FrameInfo &frame);
    void ProcessTraceNode(TraceNodeField* head, uint32_t allocSize);
    void ProcessStackTrace(uint32_t size);
private:
    std::stack<FrameInfo* > frames_;
};

} // namespace panda
#endif
