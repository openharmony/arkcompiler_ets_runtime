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

#ifndef ECMASCRIPT_DFX_HPROF_HEAP_SAMPLING_H
#define ECMASCRIPT_DFX_HPROF_HEAP_SAMPLING_H

#include "ecmascript/dfx/stackinfo/js_stackgetter.h"
#include "ecmascript/mem/heap.h"

namespace panda::ecmascript {
struct CallFrameInfo {
    std::string codeType_ = "";
    std::string functionName_ = "";
    std::string moduleName_ = "";
    int columnNumber_ = -1;
    int lineNumber_ = -1;
    int scriptId_ = 0;
    std::string url_ = "";
};

struct Sample {
    Sample(size_t size, uint32_t nodeId, uint64_t ordinal, unsigned int count)
        : size_(size),
        nodeId_(nodeId),
        ordinal_(ordinal),
        count_(count) {}
    const size_t size_;
    const uint32_t nodeId_;
    const uint64_t ordinal_;
    const unsigned int count_;
};

struct SamplingNode {
    std::map<size_t, unsigned int> allocations_;
    std::map<MethodKey, std::unique_ptr<struct SamplingNode>> children_;
    CallFrameInfo callFrameInfo_;
    uint32_t id_ = 0;
    size_t selfSize_ = 0;
};

struct SamplingInfo {
    struct SamplingNode head_;
    CVector<struct Sample> samples_;
};

class HeapSampling {
public:
    HeapSampling(const EcmaVM *vm, Heap *const heap, uint64_t interval, int stackDepth);
    const struct SamplingInfo* GetAllocationProfile();
    void ImplementSampling([[maybe_unused]] Address addr, size_t size);
    virtual ~HeapSampling();

private:
    void GetStack();
    struct SamplingNode *PushAndGetNode();
    CallFrameInfo GetMethodInfo(const MethodKey &methodKey);
    struct SamplingNode *FindOrAddNode(struct SamplingNode *node, const MethodKey &methodKey);
    bool PushStackInfo(const struct MethodKey &methodKey);
    bool PushFrameInfo(const FrameInfoTemp &frameInfoTemp);
    void ResetFrameLength();
    uint32_t CreateNodeId();
    uint64_t CreateSampleId();
    void FillScriptIdAndStore();
    std::string AddRunningState(char *functionName, RunningState state, kungfu::DeoptType type);
    unsigned int AdjustSampleCount(size_t size, unsigned int count) const;
    void CalNodeSelfSize(SamplingNode *node);

private:
    const EcmaVM *vm_;
    [[maybe_unused]] Heap *const heap_;
    [[maybe_unused]] uint64_t rate_;
    std::unique_ptr<struct SamplingInfo> samplingInfo_;
    int stackDepth_;
    AllocationInspector allocationInspector_;
    uint32_t nodeId_ = 0;
    size_t sampleId_ = 0;
    CMap<struct MethodKey, struct CallFrameInfo> stackInfoMap_;
    CVector<struct MethodKey> frameStack_;
    CMap<std::string, int> scriptIdMap_;
    CVector<FrameInfoTemp> frameInfoTemps_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_DFX_HPROF_HEAP_SAMPLING_H