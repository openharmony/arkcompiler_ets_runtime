/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/dfx/hprof/heap_sampling.h"


namespace panda::ecmascript {
HeapSampling::HeapSampling(const EcmaVM *vm, Heap *const heap, uint64_t interval, int stackDepth)
    : vm_(vm),
      heap_(heap),
      rate_(interval),
      stackDepth_(stackDepth),
      allocationInspector_(heap_, rate_, this)
{
    samplingInfo_ = std::make_unique<struct SamplingInfo>();
    samplingInfo_->head_.callFrameInfo_.functionName_ = "(root)";
    samplingInfo_->head_.id_ = CreateNodeId();
    heap_->AddAllocationInspectorToAllSpaces(&allocationInspector_);
    vm_->GetJSThread()->SetIsStartHeapSampling(true);
}

HeapSampling::~HeapSampling()
{
    heap_->ClearAllocationInspectorFromAllSpaces();
    vm_->GetJSThread()->SetIsStartHeapSampling(false);
}

const struct SamplingInfo *HeapSampling::GetAllocationProfile()
{
    CalNodeSelfSize(&samplingInfo_->head_);
    return samplingInfo_.get();
}

void HeapSampling::ImplementSampling([[maybe_unused]] Address addr, size_t size)
{
    GetStack();
    SamplingNode *node = PushAndGetNode();
    node->allocations_[size]++;
    samplingInfo_->samples_.emplace_back(Sample(size, node->id_, CreateSampleId(), AdjustSampleCount(size, 1)));
}

bool HeapSampling::PushStackInfo(const struct MethodKey &methodKey)
{
    if (UNLIKELY(frameStack_.size() >= static_cast<size_t>(stackDepth_))) {
        return false;
    }
    frameStack_.emplace_back(methodKey);
    return true;
}

bool HeapSampling::PushFrameInfo(const FrameInfoTemp &frameInfoTemp)
{
    if (UNLIKELY(frameInfoTemps_.size() >= static_cast<size_t>(stackDepth_))) {
        return false;
    }
    frameInfoTemps_.emplace_back(frameInfoTemp);
    return true;
}

void HeapSampling::ResetFrameLength()
{
    frameInfoTemps_.clear();
    frameStack_.clear();
}

void HeapSampling::GetStack()
{
    ResetFrameLength();
    JSThread *thread = vm_->GetAssociatedJSThread();
    JSTaggedType *frame = const_cast<JSTaggedType *>(thread->GetCurrentFrame());
    if (frame == nullptr) {
        return;
    }
    if (JsStackGetter::CheckFrameType(thread, frame)) {
        FrameHandler frameHandler(thread);
        FrameIterator it(frameHandler.GetSp(), thread);
        bool topFrame = true;
        int stackCounter = 0;
        for (; !it.Done() && stackCounter < stackDepth_; it.Advance<>()) {
            auto method = it.CheckAndGetMethod();
            if (method == nullptr) {
                continue;
            }
            bool isNative = method->IsNativeWithCallField();
            struct MethodKey methodKey;
            if (topFrame) {
                methodKey.state = JsStackGetter::GetRunningState(it, vm_, isNative, true);
                topFrame = false;
            } else {
                methodKey.state = JsStackGetter::GetRunningState(it, vm_, isNative, false);
            }
            void *methodIdentifier = JsStackGetter::GetMethodIdentifier(method, it);
            if (methodIdentifier == nullptr) {
                continue;
            }
            methodKey.methodIdentifier = methodIdentifier;
            if (stackInfoMap_.count(methodKey) == 0) {
                struct FrameInfoTemp codeEntry;
                if (UNLIKELY(!JsStackGetter::ParseMethodInfo(methodKey, it, vm_, codeEntry))) {
                    continue;
                }
                if (UNLIKELY(!PushFrameInfo(codeEntry))) {
                    return;
                }
            }
            if (UNLIKELY(!PushStackInfo(methodKey))) {
                return;
            }
            ++stackCounter;
        }
        if (!it.Done()) {
            LOG_ECMA(INFO) << "Heap sampling actual stack depth is greater than the setted depth: " << stackDepth_;
        }
    }
}

void HeapSampling::FillScriptIdAndStore()
{
    size_t len = frameInfoTemps_.size();
    if (len == 0) {
        return;
    }
    struct CallFrameInfo callframeInfo;
    for (size_t i = 0; i < len; ++i) {
        callframeInfo.url_ = frameInfoTemps_[i].url;
        auto iter = scriptIdMap_.find(callframeInfo.url_);
        if (iter == scriptIdMap_.end()) {
            scriptIdMap_.emplace(callframeInfo.url_, scriptIdMap_.size() + 1); // scriptId start from 1
            callframeInfo.scriptId_ = static_cast<int>(scriptIdMap_.size());
        } else {
            callframeInfo.scriptId_ = iter->second;
        }
        callframeInfo.functionName_ = AddRunningState(frameInfoTemps_[i].functionName,
                                                      frameInfoTemps_[i].methodKey.state,
                                                      frameInfoTemps_[i].methodKey.deoptType);
        callframeInfo.codeType_ = frameInfoTemps_[i].codeType;
        callframeInfo.columnNumber_ = frameInfoTemps_[i].columnNumber;
        callframeInfo.lineNumber_ = frameInfoTemps_[i].lineNumber;
        stackInfoMap_.emplace(frameInfoTemps_[i].methodKey, callframeInfo);
    }
    frameInfoTemps_.clear();
}

std::string HeapSampling::AddRunningState(char *functionName, RunningState state, kungfu::DeoptType type)
{
    std::string result = functionName;
    if (state == RunningState::AOT && type != kungfu::DeoptType::NONE) {
        state = RunningState::AINT;
    }
    if (state == RunningState::BUILTIN) {
        result.append("(BUILTIN)");
    }
    return result;
}

SamplingNode *HeapSampling::PushAndGetNode()
{
    FillScriptIdAndStore();
    SamplingNode *node = &(samplingInfo_->head_);
    int frameLen = static_cast<int>(frameStack_.size()) - 1;
    for (; frameLen >= 0; frameLen--) {
        node = FindOrAddNode(node, frameStack_[frameLen]);
    }
    return node;
}

struct CallFrameInfo HeapSampling::GetMethodInfo(const MethodKey &methodKey)
{
    struct CallFrameInfo frameInfo;
    auto iter = stackInfoMap_.find(methodKey);
    if (iter != stackInfoMap_.end()) {
        frameInfo = iter->second;
    }
    return frameInfo;
}

struct SamplingNode *HeapSampling::FindOrAddNode(struct SamplingNode *node, const MethodKey &methodKey)
{
    struct SamplingNode *childNode = nullptr;
    if (node->children_.count(methodKey) != 0) {
        childNode = node->children_[methodKey].get();
    }
    if (childNode == nullptr) {
        std::unique_ptr<struct SamplingNode> tempNode = std::make_unique<struct SamplingNode>();
        tempNode->callFrameInfo_ = GetMethodInfo(methodKey);
        tempNode->id_ = CreateNodeId();
        node->children_.emplace(methodKey, std::move(tempNode));
        return node->children_[methodKey].get();
    }
    return childNode;
}

uint32_t HeapSampling::CreateNodeId()
{
    return ++nodeId_;
}

uint64_t HeapSampling::CreateSampleId()
{
    return ++sampleId_;
}

// We collect samples according to a Poisson Process. Because sampling can not record
// all allocations, we need estimate real allocations of all spaces based on the collected
// samples. Given that sampling rate is R, the probability sampling an allocation of size S
// is 1-exp(-S/R). So when collect *count* samples with size *size*, we can use the above
// probability to approximate the real count of allocations with size *size*.
unsigned int HeapSampling::AdjustSampleCount(size_t size, unsigned int count) const
{
    double scale = 1.0 / (1.0 - std::exp(-static_cast<double>(size) / rate_));
    return static_cast<unsigned int>(count * scale + base::HALF);
}

void HeapSampling::CalNodeSelfSize(SamplingNode *node)
{
    node->selfSize_ = 0;
    for (const auto &alloc : node->allocations_) {
        unsigned int realCount = AdjustSampleCount(alloc.first, alloc.second);
        node->selfSize_ += alloc.first * realCount;
    }
    for (auto &child : node->children_) {
        CalNodeSelfSize(child.second.get());
    }
}
}  // namespace panda::ecmascript
