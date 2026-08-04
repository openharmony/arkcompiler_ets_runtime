/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_ARKSTEED_ARKSTEED_WRITE_BARRIER_VALUE_KIND_PASS_H
#define ECMASCRIPT_ARKSTEED_ARKSTEED_WRITE_BARRIER_VALUE_KIND_PASS_H

#include "ecmascript/arksteed/arksteed_graph.h"
#include "ecmascript/arksteed/arksteed_opcode.h"

namespace panda::ecmascript::arksteed {

class WriteBarrierValueKindPass {
public:
    explicit WriteBarrierValueKindPass(Graph *graph)
        : graph_(graph), chunk_(graph->GetChunk()), phis_(chunk_), valueKinds_(chunk_)
    {
    }

    void Run();

private:
    enum class TargetKind : uint8_t {
        NonHeap,
        HeapObject,
    };

    struct PhiTargetState {
        bool hasTargetSource {false};
        bool allInputsTarget {true};
    };

    ArkSteedWriteBarrierValueKind ClassifyDirectValue(ValueVertex *value) const;
    ArkSteedWriteBarrierValueKind ClassifyValue(ValueVertex *value) const;
    bool InputCanBeTarget(ValueVertex *input, TargetKind target,
                          const ChunkMap<PhiVertex *, PhiTargetState> &states) const;
    bool InputHasTargetSource(ValueVertex *input, TargetKind target,
                              const ChunkMap<PhiVertex *, PhiTargetState> &states) const;
    void CollectPhis();
    void ComputePhiKindsForTarget(TargetKind target, ChunkMap<PhiVertex *, PhiTargetState> *states);
    void ComputePhiKinds();
    void RewriteStores();
    NonControlVertex *TryRewriteStore(NonControlVertex *vertex);
    StoreTaggedFieldVertex *NewStoreWithoutBarrier(
        BB *owner, ValueVertex *object, ValueVertex *value, int32_t offset);
    StoreTaggedElementVertex *NewElementStoreWithoutBarrier(BB *owner, ValueVertex *object, ValueVertex *index,
                                                            ValueVertex *value);
    bool TryGetIntPtrConstant(ValueVertex *value, int32_t *result) const;

    static ArkSteedWriteBarrierValueKind ToValueKind(TargetKind target);

    Graph *graph_ {nullptr};
    Chunk *chunk_ {nullptr};
    ChunkVector<PhiVertex *> phis_;
    ChunkMap<ValueVertex *, ArkSteedWriteBarrierValueKind> valueKinds_;
};

ArkSteedWriteBarrierValueKind ClassifyDirectWriteBarrierValueKind(ValueVertex *value);

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_ARKSTEED_WRITE_BARRIER_VALUE_KIND_PASS_H
