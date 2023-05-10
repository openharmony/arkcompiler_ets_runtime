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

#ifndef ECMASCRIPT_HPROF_HEAP_SAMPLING_H
#define ECMASCRIPT_HPROF_HEAP_SAMPLING_H

#include "ecmascript/dfx/stackinfo/js_stackgetter.h"
#include "ecmascript/mem/heap.h"

namespace panda::ecmascript {
struct CallFrameInfo {
    std::string codeType_ = "";
    std::string functionName_ = "";
    int columnNumber_ = -1;
    int lineNumber_ = -1;
    int scriptId_ = 0;
    std::string url_ = "";
};

struct Sample {
    Sample(size_t size, size_t nodeId, size_t ordinal)
        : size_(size),
        nodeId_(nodeId),
        ordinal_(ordinal) {}
    const size_t size_;
    const size_t nodeId_;
    const size_t ordinal_;
};

struct SamplingNode {
    std::map<size_t, unsigned int> allocations_;
    std::map<MethodKey, std::unique_ptr<struct SamplingNode>> children_;
    CallFrameInfo callFrameInfo_;
    size_t id_ = 0;
};

struct AllocationNode {
    size_t selfSize_ = 0;
    CVector<struct AllocationNode> children_;
    CallFrameInfo callFrameInfo_;
    size_t id_ = 0;
};

struct SamplingInfo {
    struct AllocationNode head_;
    CVector<struct Sample> samples_;
};

class HeapSampling {
public:
    HeapSampling(const EcmaVM *vm, Heap *const heap, uint64_t interval, int stackDepth);
    std::unique_ptr<SamplingInfo> GetAllocationProfile();
    void ImplementSampling(Address addr, size_t size);
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
    void TransferSamplingNode(AllocationNode* node,
                              const SamplingNode *samplingNode);
private:
    const EcmaVM *vm_;
    [[maybe_unused]] Heap *const heap_;
    [[maybe_unused]] uint64_t rate_;
    SamplingNode rootNode_;
    CVector<std::unique_ptr<Sample>> samples_;
    int stackDepth_;
    uint32_t nodeId_ = 0;
    size_t sampleId_ = 0;
    CMap<struct MethodKey, struct CallFrameInfo> stackInfoMap_;
    CVector<struct MethodKey> frameStack_;
    CMap<std::string, int> scriptIdMap_;
    CVector<FrameInfoTemp> frameInfoTemps_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_HPROF_HEAP_SAMPLING_H