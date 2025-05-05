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

#include "common_components/common_runtime/src/heap/heap.h"
#include "common_components/common_runtime/src/heap/collector/task_queue.h"
#include "common_components/common_runtime/src/heap/collector/collector_resources.h"
#include "common_components/common_runtime/src/heap/collector/gc_request.h"
#include "common_components/common_runtime/src/inspector/file_stream.h"
#include "common_components/common_runtime/src/inspector/alloc_data.h"
#include "common_components/common_runtime/src/heap/allocator/region_info.h"
#include "common_components/common_runtime/src/heap/allocator/alloc_buffer.h"
#include "common_components/common_runtime/src/inspector/profiler_agent_impl.h"
namespace panda {
int EnableAllocRecord(bool enable)
{
    panda::ArkAllocData::GetArkAllocData()->SetRecording(enable);
    if (enable) {
        panda::ArkAllocData::SetArkAllocData();
    } else {
        panda::ArkAllocData::GetArkAllocData()->DeleteArkAllocData();
    }
    return 0;
}
std::string ParseDomain(const std::string &message)
{
    std::string key = "\"id\":";
    size_t startPos = message.find(key);
    if (startPos == std::string::npos) {
        return "";
    }
    startPos += key.length();

    while (startPos < message.length() && std::isspace(message[startPos])) {
        ++startPos;
    }

    if (message[startPos] == '"') {
        size_t endPos = message.find('"', startPos + 1);
        if (endPos == std::string::npos) {
            return "";
        }
        return message.substr(startPos + 1, endPos - startPos - 1);
    } else {
        size_t endPos = message.find(',', startPos);
        if (endPos == std::string::npos) {
            endPos = message.find('}', startPos);
        }
        if (endPos == std::string::npos) {
            return "";
        }
        return message.substr(startPos, endPos - startPos);
    }
}

void SetEnd(const std::string &message, panda::MsgType type)
{
    panda::HeapProfilerStream* stream = &panda::HeapProfilerStream::GetInstance();
    stream->SetContext(type);
    panda::StreamWriter* writer = new panda::StreamWriter(stream);
    writer->WriteString("{\"id\":");
    writer->WriteString(panda::HeapProfilerStream::GetInstance().GetMessageID());
    writer->WriteString(",\"result\":{}");
    writer->End();
    delete writer;
}

void DumpHeapSnapshot(SendMsgCB sendMsg)
{
    panda::Heap::GetHeap().GetCollectorResources().RequestHeapDump(
        panda::GCTask::TaskType::GC_TASK_DUMP_HEAP_IDE);
}

void StartTrackingHeapObjects(const std::string &message, SendMsgCB sendMsg)
{
    SetEnd(message, panda::MsgType::END);
    EnableAllocRecord(true);
}

void StopTrackingHeapObjects(const std::string &message, SendMsgCB sendMsg)
{
    EnableAllocRecord(false);
    SetEnd(message, panda::MsgType::END);
}

void CollectGarbage(const std::string &message, SendMsgCB sendMsg)
{
    panda::Heap::GetHeap().GetCollectorResources().RequestGC(panda::GC_REASON_HEU, false);
    SetEnd(message, panda::MsgType::END);
}

void DisableCollect(const std::string &message, SendMsgCB sendMsg)
{
    SetEnd(message, panda::MsgType::DISABLE);
}

void GetHeapUsage(const std::string &message, SendMsgCB sendMsg)
{
    panda::HeapProfilerStream* stream = &panda::HeapProfilerStream::GetInstance();
    stream->SetContext(panda::MsgType::HEAPUSAGE);
    panda::StreamWriter* writer = new panda::StreamWriter(stream);
    writer->WriteString("{\"id\":");
    writer->WriteString(panda::HeapProfilerStream::GetInstance().GetMessageID());
    writer->WriteString(",\"result\":{\"usedSize\":");
    ssize_t allocatedSize = panda::Heap::GetHeap().GetAllocatedSize();
    writer->WriteNumber(allocatedSize);
    writer->WriteString(",\"totalSize\":");
    ssize_t totalSize = panda::Heap::GetHeap().GetMaxCapacity();
    writer->WriteNumber(totalSize);
    writer->WriteString("}");
    writer->End();
    delete writer;
}

void ProfilerAgentImpl(const std::string &message, SendMsgCB sendMsg)
{
    panda::HeapProfilerStream::GetInstance().SetHandler(sendMsg);
    panda::HeapProfilerStream::GetInstance().SetMessageID(message);
    if (message.find("takeHeapSnapshot", 0) != std::string::npos) {
        DumpHeapSnapshot(sendMsg);
    } else if (message.find("startTrackingHeapObjects", 0) != std::string::npos) {
        StartTrackingHeapObjects(message, sendMsg);
    } else if (message.find("stopTrackingHeapObjects", 0) != std::string::npos) {
        StopTrackingHeapObjects(message, sendMsg);
    } else if (message.find("disable", 0) != std::string::npos) {
        DisableCollect(message, sendMsg);
    } else if (message.find("collectGarbage", 0) != std::string::npos) {
        CollectGarbage(message, sendMsg);
    } else if (message.find("getHeapUsage", 0) != std::string::npos) {
        GetHeapUsage(message, sendMsg);
    } else {
        LOG_COMMON(ERROR) << "invaild request\n";
    }
}
}