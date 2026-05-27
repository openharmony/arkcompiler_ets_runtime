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

#include "ecmascript/arksteed/arksteed_known_node_aspect.h"

#include <algorithm>
#include <tuple>

#include "libpandabase/macros.h"

namespace panda::ecmascript::arksteed {

bool KnownNodeInfo::NodeKindIncludes(NodeKind lhs, NodeKind rhs)
{
    if (lhs == rhs) {
        return true;
    }
    if (lhs == NodeKind::JS_OBJECT && rhs == NodeKind::HEAP_OBJECT) {
        return true;
    }
    return rhs == NodeKind::UNKNOWN;
}

KnownNodeInfo::NodeKind KnownNodeInfo::IntersectNodeKind(NodeKind lhs, NodeKind rhs)
{
    if (lhs == rhs) {
        return lhs;
    }
    if (lhs == NodeKind::UNKNOWN || rhs == NodeKind::UNKNOWN) {
        return NodeKind::UNKNOWN;
    }
    if (NodeKindIncludes(lhs, rhs)) {
        return lhs;
    }
    if (NodeKindIncludes(rhs, lhs)) {
        return rhs;
    }
    return NodeKind::UNKNOWN;
}

bool KnownNodeInfo::CheckType(NodeKind kind) const
{
    return NodeKindIncludes(nodeKind_, kind);
}

bool KnownNodeInfo::EnsureType(NodeKind kind)
{
    if (CheckType(kind)) {
        return true;
    }
    nodeKind_ = kind;
    return false;
}

void KnownNodeInfo::RecordHClass(JSHClass *hclass, bool isStable)
{
    possibleHClasses_.clear();
    AddPossibleHClass(hclass, isStable);
    EnsureType(NodeKind::JS_OBJECT);
}

void KnownNodeInfo::RecordPossibleHClasses(const PossibleHClasses &hclasses, bool isStable)
{
    possibleHClasses_.clear();
    for (JSHClass *hclass : hclasses) {
        AddPossibleHClass(hclass, isStable);
    }
    if (!possibleHClasses_.empty()) {
        EnsureType(NodeKind::JS_OBJECT);
    }
}

KnownNodeInfo::PossibleHClasses KnownNodeInfo::GetPossibleHClasses() const
{
    PossibleHClasses result;
    result.reserve(possibleHClasses_.size());
    for (const auto &info : possibleHClasses_) {
        result.push_back(info.hclass);
    }
    return result;
}

void KnownNodeInfo::AddPossibleHClass(JSHClass *hclass, bool isStable)
{
    if (hclass == nullptr) {
        return;
    }
    auto it = std::find_if(possibleHClasses_.begin(), possibleHClasses_.end(),
                           [hclass](const PossibleHClassInfo &info) { return info.hclass == hclass; });
    if (it != possibleHClasses_.end()) {
        it->isStable = it->isStable && isStable;
        return;
    }
    possibleHClasses_.push_back(PossibleHClassInfo {
        .hclass = hclass,
        .isStable = isStable,
    });
}

void KnownNodeInfo::UnionPossibleHClasses(const KnownNodeInfo &other)
{
    if (possibleHClasses_.empty() || other.possibleHClasses_.empty()) {
        possibleHClasses_.clear();
        return;
    }
    for (const auto &otherInfo : other.possibleHClasses_) {
        AddPossibleHClass(otherInfo.hclass, otherInfo.isStable);
    }
}

void KnownNodeInfo::ClearUnstable()
{
    possibleHClasses_.erase(
        std::remove_if(possibleHClasses_.begin(), possibleHClasses_.end(),
                       [](const PossibleHClassInfo &info) { return !info.isStable; }),
        possibleHClasses_.end());
}

bool KnownNodeInfo::MergeWith(const KnownNodeInfo &other)
{
    nodeKind_ = IntersectNodeKind(nodeKind_, other.nodeKind_);
    UnionPossibleHClasses(other);
    return !NoInfoAvailable();
}

bool KnownLoadKeyCompare::operator()(const KnownLoadKey &lhs, const KnownLoadKey &rhs) const
{
    return std::tie(lhs.receiver, lhs.keyKind, lhs.keyData, lhs.propertyLookupResult) <
           std::tie(rhs.receiver, rhs.keyKind, rhs.keyData, rhs.propertyLookupResult);
}

KnownNodeAspect::KnownNodeAspect(Chunk *chunk) : nodeInfos_(chunk), loadedProperties_(chunk)
{
    ASSERT(chunk != nullptr);
}

KnownNodeAspect::KnownNodeAspect(const KnownNodeAspect &other, Chunk *chunk)
    : nodeInfos_(chunk), loadedProperties_(chunk)
{
    ASSERT(chunk != nullptr);
    nodeInfos_.insert(other.nodeInfos_.begin(), other.nodeInfos_.end());
    loadedProperties_.insert(other.loadedProperties_.begin(), other.loadedProperties_.end());
}

KnownNodeAspect *KnownNodeAspect::Clone(Chunk *chunk) const
{
    return chunk->New<KnownNodeAspect>(*this, chunk);
}

KnownNodeInfo *KnownNodeAspect::TryGetInfoFor(ValueVertex *node)
{
    auto it = nodeInfos_.find(node);
    if (it == nodeInfos_.end()) {
        return nullptr;
    }
    return &it->second;
}

const KnownNodeInfo *KnownNodeAspect::TryGetInfoFor(ValueVertex *node) const
{
    auto it = nodeInfos_.find(node);
    if (it == nodeInfos_.end()) {
        return nullptr;
    }
    return &it->second;
}

KnownNodeInfo *KnownNodeAspect::GetOrCreateInfoFor(ValueVertex *node)
{
    auto [it, inserted] = nodeInfos_.emplace(node, KnownNodeInfo());
    return &it->second;
}

bool KnownNodeAspect::CheckType(ValueVertex *node, KnownNodeInfo::NodeKind kind) const
{
    const KnownNodeInfo *info = TryGetInfoFor(node);
    return info != nullptr && info->CheckType(kind);
}

bool KnownNodeAspect::EnsureType(ValueVertex *node, KnownNodeInfo::NodeKind kind)
{
    return GetOrCreateInfoFor(node)->EnsureType(kind);
}

void KnownNodeAspect::RecordHClass(ValueVertex *node, JSHClass *hclass, bool isStable)
{
    GetOrCreateInfoFor(node)->RecordHClass(hclass, isStable);
}

JSHClass *KnownNodeAspect::TryGetHClass(ValueVertex *node) const
{
    const KnownNodeInfo *info = TryGetInfoFor(node);
    if (info == nullptr || !info->HasKnownHClass()) {
        return nullptr;
    }
    return info->GetKnownHClass();
}

void KnownNodeAspect::RecordPossibleHClasses(ValueVertex *node, const KnownNodeInfo::PossibleHClasses &hclasses,
                                             bool isStable)
{
    GetOrCreateInfoFor(node)->RecordPossibleHClasses(hclasses, isStable);
}

std::optional<KnownNodeInfo::PossibleHClasses> KnownNodeAspect::TryGetPossibleHClasses(ValueVertex *node) const
{
    const KnownNodeInfo *info = TryGetInfoFor(node);
    if (info == nullptr || !info->HasPossibleHClasses()) {
        return std::nullopt;
    }
    return info->GetPossibleHClasses();
}

ValueVertex *KnownNodeAspect::TryFindLoadedProperty(const KnownLoadKey &key) const
{
    auto it = loadedProperties_.find(key);
    if (it == loadedProperties_.end()) {
        return nullptr;
    }
    return it->second;
}

void KnownNodeAspect::RecordLoadedProperty(const KnownLoadKey &key, ValueVertex *value)
{
    loadedProperties_[key] = value;
}

void KnownNodeAspect::ClearLoadedProperties()
{
    loadedProperties_.clear();
}

void KnownNodeAspect::ClearLoadedPropertiesForReceiver(ValueVertex *receiver)
{
    auto it = loadedProperties_.begin();
    while (it != loadedProperties_.end()) {
        if (it->first.receiver == receiver) {
            it = loadedProperties_.erase(it);
        } else {
            ++it;
        }
    }
}

void KnownNodeAspect::ClearUnstable()
{
    auto infoIt = nodeInfos_.begin();
    while (infoIt != nodeInfos_.end()) {
        infoIt->second.ClearUnstable();
        if (infoIt->second.NoInfoAvailable()) {
            infoIt = nodeInfos_.erase(infoIt);
        } else {
            ++infoIt;
        }
    }
    ClearLoadedProperties();
}

void KnownNodeAspect::ClearAll()
{
    nodeInfos_.clear();
    loadedProperties_.clear();
}

void KnownNodeAspect::Merge(const KnownNodeAspect &other)
{
    auto infoIt = nodeInfos_.begin();
    while (infoIt != nodeInfos_.end()) {
        auto otherIt = other.nodeInfos_.find(infoIt->first);
        if (otherIt == other.nodeInfos_.end() || !infoIt->second.MergeWith(otherIt->second)) {
            infoIt = nodeInfos_.erase(infoIt);
        } else {
            ++infoIt;
        }
    }

    auto loadIt = loadedProperties_.begin();
    while (loadIt != loadedProperties_.end()) {
        auto otherIt = other.loadedProperties_.find(loadIt->first);
        if (otherIt == other.loadedProperties_.end() || otherIt->second != loadIt->second) {
            loadIt = loadedProperties_.erase(loadIt);
        } else {
            ++loadIt;
        }
    }
}

}  // namespace panda::ecmascript::arksteed
