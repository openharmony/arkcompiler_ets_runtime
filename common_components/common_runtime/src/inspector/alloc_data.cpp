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
#include "common_components/common_runtime/src/inspector/alloc_data.h"
#include <iostream>
#include <chrono>
#include <common/scoped_object_access.h>
#include <chrono>
#include "common_components/common_runtime/src/common/stack_type.h"
#include "common_components/common_runtime/src/unwind_stack/print_stack_info.h"
#include "common_components/common_runtime/src/unwind_stack/stack_info.h"
#include "common_components/common_runtime/src/unwind_stack/stack_metadata_helper.h"
#include "common_components/common_runtime/src/inspector/file_stream.h"

namespace panda {
static ArkAllocData* g_allocInfo;
ArkAllocData* ArkAllocData::GetArkAllocData()
{
    if (g_allocInfo == nullptr) {
        g_allocInfo = new ArkAllocData();
    }
    return g_allocInfo;
}

void ArkAllocData::SetArkAllocData()
{
    g_allocInfo -> InitAllocParam();
}

TraceNodeField* ArkAllocData::FindNode(const FrameAddress* ptr, const char* str)
{
    if (str == nullptr) {
        return nullptr;
    };
    size_t key = FindKey(ptr, str);
    auto iter = traceNodeMap_.find(key);
    if (iter != traceNodeMap_.end()) {
        return iter->second;
    } else {
        return nullptr;
    }
}

int32_t ArkAllocData::FindKey(const FrameAddress* ptr, const char* str)
{
    // The identifier of a stack frame depends on the FA of the frame and the name of the frame.
    std::size_t hash1 = std::hash<const FrameAddress* >{}(ptr);
    std::size_t hash2 = std::hash<std::string>{}(str);
    return hash1 ^ hash2;
}

void ArkAllocData::DeleteArkAllocData()
{
    std::unique_lock<std::mutex> lock(sharedMtx_);
    SetRecording(false);
    HeapProfilerStream* stream = &panda::HeapProfilerStream::GetInstance();
    stream->SetContext(ALLOCATION);
    g_allocInfo->SerializeArkAllocData();
    for (auto sample : g_allocInfo->samples_) {
        delete sample;
        sample = nullptr;
    }
    samples_.clear();
    for (auto traceInfo : g_allocInfo->traceFunctionInfo_) {
        delete traceInfo;
        traceInfo = nullptr;
    }
    traceFunctionInfo_.clear();
    g_allocInfo->DeleteAllNode(g_allocInfo->traceNodeHead_);
    delete g_allocInfo->writer_;
}

void ArkAllocData::DeleteAllNode(TraceNodeField* node)
{
    for (size_t i = 0; i < node->children.size(); i++) {
        DeleteAllNode(node->children[i]);
    }
    delete node;
}

void ArkAllocData::SerializeArkAllocData()
{
    writer_->WriteString("{\\\"head\\\":");
    // 1. dump callFrame
    SerializeCallFrames();
    writer_->WriteString(",");
    // 2. dump samples
    SerializeSamples();
    writer_->WriteChar('}');
    writer_->End();
}

void ArkAllocData::SerializeCallFrames() { SerializeEachFrame(traceNodeHead_); }

void ArkAllocData::SerializeFunctionInfo(int32_t idx)
{
    writer_->WriteString("{\\\"functionName\\\":\\\"");
    writer_->WriteString(traceFunctionInfo_[idx]->functionName);
    writer_->WriteString("\\\",\\\"scriptName\\\":\\\"");
    writer_->WriteString(traceFunctionInfo_[idx]->scriptName);
    writer_->WriteString("\\\",\\\"lineNumber\\\":");
    writer_->WriteNumber(traceFunctionInfo_[idx]->line);
    writer_->WriteString(",\\\"columnNumber\\\":");
    writer_->WriteNumber(traceFunctionInfo_[idx]->column);
    writer_->WriteChar('}');
}

void ArkAllocData::SerializeEachFrame(TraceNodeField* node)
{
    writer_->WriteString("{\\\"callFrame\\\":");
    SerializeFunctionInfo(node->functionInfoIndex);
    writer_->WriteChar(',');
    writer_->WriteString("\\\"selfSize\\\":");
    writer_->WriteNumber(node->selfSize);
    writer_->WriteString(",\\\"id\\\":");
    writer_->WriteNumber(node->id);
    writer_->WriteString(",\\\"children\\\":[");
    if (node->children.size() != 0) {
        for (size_t i = 0; i < node->children.size(); i++) {
            SerializeEachFrame(node->children[i]);
            if (i < node->children.size() - 1) {
                writer_->WriteChar(',');
            }
        }
    }
    writer_->WriteChar(']');
    writer_->WriteChar('}');
}

void ArkAllocData::SerializeSamples()
{
    writer_->WriteString("\\\"samples\\\":[");
    // for each sample
    for (size_t i = 0; i < samples_.size(); i++) {
        if (i != 0) {
            writer->WriteChar(',');
        }
        writer->WriteString("{\\\"size\\\":");
        writer->WriteNumber(samples_[i]->size);
        writer->WriteChar(',');
        writer->WriteString("\\\"nodeId\\\":");
        writer->WriteNumber(samples_[i]->nodeId);
        writer->WriteChar(',');
        writer->WriteString("\\\"ordinal\\\":");
        writer->WriteNumber(samples_[i]->orinal);
        writer->WriteChar('}');
    }
    writer_->WriteChar(']');
}
void ArkAllocData::InitRoot()
{
    TraceNodeField* traceNode = new TraceNodeField();
    if (traceNode == nullptr) {
        LOG_COMMON(ERROR) << "init traceNode failed";
        return;
    }
    traceNode->id = 0;
    traceNode->functionInfoIndex = 0;
    traceNode->selfSize = 0;
    traceNodeHead_ = traceNode;
    TraceFunctionInfo* traceFunction = new TraceFunctionInfo();
    if (traceFunction == nullptr) {
        LOG_COMMON(ERROR) << "init traceFunction failed";
        return;
    }
    traceFunction->functionName = "{root}";
    traceFunction->scriptName = "";
    traceFunction->line = -1;
    ArkAllocData::GetArkAllocData()->traceFunctionInfo_.push_back(traceFunction);
}

void ArkAllocData::InitAllocParam()
{
    sampSize_ = 1 * 1024; // default 1 * 1024 b
    InitRoot();
    HeapProfilerStream* stream = &panda::HeapProfilerStream::GetInstance();
    writer_ = new StreamWriter(stream);
}

void ArkAllocData::SerializeStats()
{
    HeapProfilerStream* stream = &panda::HeapProfilerStream::GetInstance();
    stream->SetContext(STATSUPDATE);
    writer_->WriteString("[");
    writer_->WriteNumber(traceNodeID_);
    writer_->WriteChar(',');
    writer_->WriteNumber(0);
    writer_->WriteChar(',');
    ssize_t allocatedSize = panda::Heap::GetHeap().GetAllocatedSize();
    writer_->WriteNumber(allocatedSize);
    writer_->WriteString("]");
    writer_->End();
}

void ArkAllocData::RecordAllocNodes(const TypeInfo* klass, uint32_t size)
{
    std::unique_lock<std::mutex> lock(sharedMtx_);
    if (!IsRecording()) { // avoid delete func was called at this time
        return;
    }
    allocSize_ += size;
    if (allocSize < sampSize_) {
        return;
    } else {
        AllocStackInfo* allocStackInfo  = new AllocStackInfo();
        allocStackInfo->ProcessStackTrace(size);
        delete allocStackInfo;
        allocSize_ = 0;
        SerializeStats();
    }
}

int32_t AllocStackInfo::ProcessTraceInfo(FrameInfo &frame)
{
    if (frame.GetFrameType() == FrameType::NATIVE) {
        return -1;
    }
    TraceFunctionInfo* traceFunction = new TraceFunctionInfo();
    if (frame.GetFrameType() == FrameType::MANAGED) {
        StackMetadataHelper stackMetadataHelper(frame);
        traceFunction->scriptName = frame.GetFileName();
        traceFunction->scriptName.ReplaceAll("/", "\\");
        traceFunction->functionName = frame.GetFuncName();
        traceFunction->line = stackMetadataHelper.GetLineNumber();
    } else {
        delete traceFunction;
        return -1;
    }

    ArkAllocData::GetArkAllocData()->traceFunctionInfo_.push_back(traceFunction);
    int32_t functionInfoIndex = ArkAllocData::GetArkAllocData()->traceFunctionInfo_.size() - 1;
    return functionInfoIndex;
}

void AllocStackInfo::ProcessTraceNode(TraceNodeField* head, uint32_t allocSize)
{
    // Initialize nodes and fill nodes in head in sequence.
    if (frames_.empty()) {
        // This node is the root node.
        head->selfSize += allocSize;
        return;
    }
    while (!frames_.empty()) {
        FrameInfo* f = frames_.top();
        frames_.pop();
        TraceNodeField* traceNode = new TraceNodeField();
        traceNode->id = ArkAllocData::GetArkAllocData()->SetNodeID();
        traceNode->functionInfoIndex = ProcessTraceInfo(*f);
        if (traceNode->functionInfoIndex == -1) {
            delete traceNode;
            continue;
        }
        traceNode->selfSize = 0;
        // Add the children node of the upper-level node.
        head->children.push_back(traceNode);
        // In this case, head becomes a child node. Used to insert a child node next time.
        head = traceNode;
        // Add to the map for next search.
        FrameAddress* FA = f->mFrame.GetFA();
        size_t key = ArkAllocData::GetArkAllocData()->FindKey(FA, f->GetFuncName().Str());
        ArkAllocData::GetArkAllocData()->traceNodeMap_.insert(std::pair<size_t, TraceNodeField*>(key, traceNode));
        delete f;
        f = nullptr;
    }
    head->selfSize += allocSize;
}
void AllocStackInfo::ProcessStackTrace(uint32_t size)
{
    UnwindContext uwContext;
    // Top unwind context can only be runtime or Arkcommon context.
    CheckTopUnwindContextAndInit(uwContext);
    while (!uwContext.frameInfo.mFrame.IsAnchorFrame(anchorFA)) {
        AnalyseAndSetFrameType(uwContext);
        FrameInfo* f = new FrameInfo(uwContext.frameInfo);
        // 1. If the node has been recorded, add the content in the stack to the end of the node.
        FrameAddress* FA = f->mFrame.GetFA();
        TraceNodeField* node = ArkAllocData::GetArkAllocData()->FindNode(FA, f->GetFuncName().Str());
        if (node != nullptr) {
            delete f;
            ProcessTraceNode(node, size);
            return;
        }
        frames_.push(f);
        UnwindContext caller;
        lastFrameType = uwContext.frameInfo.GetFrameType();
#ifndef _WIN64
        if (uwContext.UnwindToCallerContext(caller) == false) {
#else
        if (uwContext.UnwindToCallerContext(caller, uwCtxStatus_) == false) {
#endif
            return;
        }
        uwContext = caller;
    }
    // 2. If the stack back is not recorded, the call chain is a new call chain and the root node is the root node.
    ProcessTraceNode(ArkAllocData::GetArkAllocData()->traceNodeHead_, size);

    // 3. Record the samples at this time.
    Sample* sample = new Sample();
    sample->size = size;
    sample->nodeId = ArkAllocData::GetArkAllocData()->traceFunctionInfo_.size();
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    sample->orinal = static_cast<int32_t>(timestamp);
    ArkAllocData::GetArkAllocData()->samples_.push_back(sample);
}
}
