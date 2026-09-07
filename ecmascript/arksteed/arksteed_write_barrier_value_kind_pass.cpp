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

#include "ecmascript/arksteed/arksteed_write_barrier_value_kind_pass.h"

#include <initializer_list>
#include <limits>

#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript::arksteed {
namespace {
bool IsKnownNonHeapTaggedValue(JSTaggedValue value)
{
    if (value.IsInt() || value.IsDouble() || value.IsSpecial()) {
        return true;
    }
    return (value.GetRawData() & JSTaggedValue::TAG_HEAPOBJECT_MASK) == JSTaggedValue::TAG_BOOLEAN_MASK;
}
}  // namespace

ArkSteedWriteBarrierValueKind ClassifyDirectWriteBarrierValueKind(ValueVertex *value)
{
    if (value == nullptr) {
        return ArkSteedWriteBarrierValueKind::Unknown;
    }
    if (auto *tagged = value->TryCast<TaggedConstantVertex>()) {
        JSTaggedValue taggedValue(static_cast<JSTaggedType>(tagged->GetValue()));
        if (IsKnownNonHeapTaggedValue(taggedValue)) {
            return ArkSteedWriteBarrierValueKind::NonHeap;
        }
        if (taggedValue.IsHeapObject()) {
            return ArkSteedWriteBarrierValueKind::HeapObject;
        }
        return ArkSteedWriteBarrierValueKind::Unknown;
    }
    if (value->Is<HeapConstantVertex>()) {
        return ArkSteedWriteBarrierValueKind::HeapObject;
    }
    if (value->Is<I32ToTaggedIntVertex>() ||
        value->Is<CheckedNonNegativeI32ToTaggedIntVertex>() ||
        value->Is<F64ToTaggedDoubleVertex>()) {
        return ArkSteedWriteBarrierValueKind::NonHeap;
    }
    return ArkSteedWriteBarrierValueKind::Unknown;
}

void WriteBarrierValueKindPass::Run()
{
    CollectPhis();
    ComputePhiKinds();
    RewriteStores();
}

ArkSteedWriteBarrierValueKind WriteBarrierValueKindPass::ClassifyDirectValue(ValueVertex *value) const
{
    return ClassifyDirectWriteBarrierValueKind(value);
}

ArkSteedWriteBarrierValueKind WriteBarrierValueKindPass::ClassifyValue(ValueVertex *value) const
{
    ArkSteedWriteBarrierValueKind direct = ClassifyDirectValue(value);
    if (direct != ArkSteedWriteBarrierValueKind::Unknown || value == nullptr || !value->Is<PhiVertex>()) {
        return direct;
    }
    auto it = valueKinds_.find(value);
    return it != valueKinds_.end() ? it->second : ArkSteedWriteBarrierValueKind::Unknown;
}

bool WriteBarrierValueKindPass::InputCanBeTarget(
    ValueVertex *input, TargetKind target, const ChunkMap<PhiVertex *, PhiTargetState> &states) const
{
    if (input == nullptr) {
        return false;
    }
    if (auto *phi = input->TryCast<PhiVertex>()) {
        auto it = states.find(phi);
        return it != states.end() && it->second.allInputsTarget;
    }
    return ClassifyDirectValue(input) == ToValueKind(target);
}

bool WriteBarrierValueKindPass::InputHasTargetSource(
    ValueVertex *input, TargetKind target, const ChunkMap<PhiVertex *, PhiTargetState> &states) const
{
    if (input == nullptr) {
        return false;
    }
    if (auto *phi = input->TryCast<PhiVertex>()) {
        auto it = states.find(phi);
        return it != states.end() && it->second.hasTargetSource;
    }
    return ClassifyDirectValue(input) == ToValueKind(target);
}

void WriteBarrierValueKindPass::CollectPhis()
{
    for (BB *block : *graph_) {
        if (!block->HasPhi()) {
            continue;
        }
        for (PhiVertex *phi : block->GetPhis()) {
            phis_.push_back(phi);
        }
    }
}

void WriteBarrierValueKindPass::ComputePhiKindsForTarget(
    TargetKind target, ChunkMap<PhiVertex *, PhiTargetState> *states)
{
    ASSERT(states != nullptr);
    for (PhiVertex *phi : phis_) {
        states->emplace(phi, PhiTargetState {});
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (PhiVertex *phi : phis_) {
            bool hasTargetSource = false;
            for (uint32_t i = 0; i < phi->GetInputCount(); ++i) {
                hasTargetSource |= InputHasTargetSource(phi->GetInput(i), target, *states);
            }

            auto it = states->find(phi);
            ASSERT(it != states->end());
            if (it->second.hasTargetSource != hasTargetSource) {
                it->second.hasTargetSource = hasTargetSource;
                changed = true;
            }
        }
    }

    for (PhiVertex *phi : phis_) {
        auto it = states->find(phi);
        ASSERT(it != states->end());
        it->second.allInputsTarget = it->second.hasTargetSource;
    }

    changed = true;
    while (changed) {
        changed = false;
        for (PhiVertex *phi : phis_) {
            auto it = states->find(phi);
            ASSERT(it != states->end());
            bool allInputsTarget = it->second.hasTargetSource && phi->GetInputCount() > 0;
            for (uint32_t i = 0; i < phi->GetInputCount(); ++i) {
                if (!InputCanBeTarget(phi->GetInput(i), target, *states)) {
                    allInputsTarget = false;
                    break;
                }
            }
            if (it->second.allInputsTarget != allInputsTarget) {
                it->second.allInputsTarget = allInputsTarget;
                changed = true;
            }
        }
    }
}

void WriteBarrierValueKindPass::ComputePhiKinds()
{
    ChunkMap<PhiVertex *, PhiTargetState> nonHeapStates(chunk_);
    ChunkMap<PhiVertex *, PhiTargetState> heapObjectStates(chunk_);
    ComputePhiKindsForTarget(TargetKind::NonHeap, &nonHeapStates);
    ComputePhiKindsForTarget(TargetKind::HeapObject, &heapObjectStates);

    for (PhiVertex *phi : phis_) {
        PhiTargetState nonHeap = nonHeapStates.find(phi)->second;
        PhiTargetState heapObject = heapObjectStates.find(phi)->second;
        bool provenNonHeap = nonHeap.hasTargetSource && nonHeap.allInputsTarget;
        bool provenHeapObject = heapObject.hasTargetSource && heapObject.allInputsTarget;

        ArkSteedWriteBarrierValueKind kind = ArkSteedWriteBarrierValueKind::Unknown;
        if (provenNonHeap && !provenHeapObject) {
            kind = ArkSteedWriteBarrierValueKind::NonHeap;
        } else if (provenHeapObject && !provenNonHeap) {
            kind = ArkSteedWriteBarrierValueKind::HeapObject;
        }
        valueKinds_.emplace(phi, kind);
    }
}

void WriteBarrierValueKindPass::RewriteStores()
{
    for (BB *block : *graph_) {
        ChunkVector<NonControlVertex *> &vertices = block->GetVertices();
        for (auto it = vertices.begin(); it != vertices.end();) {
            auto *curVertex = (*it)->TryCast<SetValueWithBarrierVertex>();
            if (curVertex != nullptr && ShouldRemoveSetValueWithBarrier(curVertex)) {
                it = vertices.erase(it);
                continue;
            }
            *it = TryRewriteStore(*it);
            ++it;
        }
    }
}

bool WriteBarrierValueKindPass::ShouldRemoveSetValueWithBarrier(SetValueWithBarrierVertex *vertex) const
{
    ValueVertex *value = vertex->GetInput(SetValueWithBarrierVertex::VALUE_INDEX);
    return ClassifyValue(value) == ArkSteedWriteBarrierValueKind::NonHeap;
}

NonControlVertex *WriteBarrierValueKindPass::TryRewriteStore(NonControlVertex *vertex)
{
    if (auto *store = vertex->TryCast<StoreTaggedFieldWithBarrierVertex>()) {
        ArkSteedWriteBarrierValueKind valueKind =
            ClassifyValue(store->GetInput(StoreTaggedFieldWithBarrierVertex::VALUE_INDEX));
        if (valueKind != ArkSteedWriteBarrierValueKind::Unknown) {
            store->SetValueKind(valueKind);
        }
        if (valueKind == ArkSteedWriteBarrierValueKind::NonHeap) {
            return NewStoreWithoutBarrier(store->GetOwner(),
                store->GetInput(StoreTaggedFieldWithBarrierVertex::OBJECT_INDEX),
                store->GetInput(StoreTaggedFieldWithBarrierVertex::VALUE_INDEX), store->GetOffset());
        }
        return store;
    }

    if (auto *store = vertex->TryCast<StoreTaggedElementWithBarrierVertex>()) {
        ArkSteedWriteBarrierValueKind valueKind =
            ClassifyValue(store->GetInput(StoreTaggedElementWithBarrierVertex::VALUE_INDEX));
        if (valueKind != ArkSteedWriteBarrierValueKind::Unknown) {
            store->SetValueKind(valueKind);
        }
        if (valueKind == ArkSteedWriteBarrierValueKind::NonHeap) {
            return NewElementStoreWithoutBarrier(store->GetOwner(),
                                                 store->GetInput(StoreTaggedElementWithBarrierVertex::OBJECT_INDEX),
                                                 store->GetInput(StoreTaggedElementWithBarrierVertex::INDEX_INDEX),
                                                 store->GetInput(StoreTaggedElementWithBarrierVertex::VALUE_INDEX));
        }
        return store;
    }

    if (auto *store = vertex->TryCast<StoreSharedFieldWithBarrierVertex>()) {
        ArkSteedWriteBarrierValueKind valueKind =
            ClassifyValue(store->GetInput(StoreSharedFieldWithBarrierVertex::VALUE_INDEX));
        if (valueKind != ArkSteedWriteBarrierValueKind::Unknown) {
            store->SetValueKind(valueKind);
        }
        if (valueKind == ArkSteedWriteBarrierValueKind::NonHeap) {
            return NewStoreWithoutBarrier(store->GetOwner(),
                store->GetInput(StoreSharedFieldWithBarrierVertex::OBJECT_INDEX),
                store->GetInput(StoreSharedFieldWithBarrierVertex::VALUE_INDEX), store->GetOffset());
        }
        return store;
    }

    if (auto *store = vertex->TryCast<StoreTaggedFieldByHClassVertex>()) {
        ArkSteedWriteBarrierValueKind valueKind =
            ClassifyValue(store->GetInput(StoreTaggedFieldByHClassVertex::VALUE_INDEX));
        if (valueKind != ArkSteedWriteBarrierValueKind::Unknown) {
            store->SetValueKind(valueKind);
        }
        return store;
    }

    return vertex;
}

StoreTaggedFieldVertex *WriteBarrierValueKindPass::NewStoreWithoutBarrier(
    BB *owner, ValueVertex *object, ValueVertex *value, int32_t offset)
{
    std::initializer_list<ValueVertex *> inputs {object, value};
    auto *store = Vertex::New<StoreTaggedFieldVertex>(chunk_, inputs, offset);
    store->SetOwner(owner);
    return store;
}

StoreTaggedElementVertex *WriteBarrierValueKindPass::NewElementStoreWithoutBarrier(BB *owner, ValueVertex *object,
                                                                                   ValueVertex *index,
                                                                                   ValueVertex *value)
{
    std::initializer_list<ValueVertex *> inputs {object, index, value};
    auto *store = Vertex::New<StoreTaggedElementVertex>(chunk_, inputs);
    store->SetOwner(owner);
    return store;
}

bool WriteBarrierValueKindPass::TryGetIntPtrConstant(ValueVertex *value, int32_t *result) const
{
    if (value == nullptr || result == nullptr) {
        return false;
    }
    auto *constant = value->TryCast<IntPtrConstantVertex>();
    if (constant == nullptr) {
        return false;
    }
    intptr_t raw = constant->GetValue();
    if (raw < std::numeric_limits<int32_t>::min() || raw > std::numeric_limits<int32_t>::max()) {
        return false;
    }
    *result = static_cast<int32_t>(raw);
    return true;
}

ArkSteedWriteBarrierValueKind WriteBarrierValueKindPass::ToValueKind(TargetKind target)
{
    switch (target) {
        case TargetKind::NonHeap:
            return ArkSteedWriteBarrierValueKind::NonHeap;
        case TargetKind::HeapObject:
            return ArkSteedWriteBarrierValueKind::HeapObject;
    }
    UNREACHABLE();
}
}  // namespace panda::ecmascript::arksteed
