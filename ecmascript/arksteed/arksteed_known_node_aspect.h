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

#ifndef ECMASCRIPT_ARKSTEED_KNOWN_NODE_ASPECT_H
#define ECMASCRIPT_ARKSTEED_KNOWN_NODE_ASPECT_H

#include <cstdint>
#include <optional>
#include <vector>

#include "ecmascript/js_hclass.h"
#include "ecmascript/mem/chunk.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::arksteed {

class ValueVertex;

class KnownNodeInfo {
public:
    using PossibleHClasses = std::vector<JSHClass *>;

    enum class NodeKind : uint8_t {
        UNKNOWN,
        HEAP_OBJECT,
        JS_OBJECT,
    };

    KnownNodeInfo() = default;
    ~KnownNodeInfo() = default;
    DEFAULT_COPY_SEMANTIC(KnownNodeInfo);
    DEFAULT_MOVE_SEMANTIC(KnownNodeInfo);

    NodeKind GetNodeKind() const
    {
        return nodeKind_;
    }

    bool CheckType(NodeKind kind) const;
    bool EnsureType(NodeKind kind);

    bool HasKnownHClass() const
    {
        return possibleHClasses_.size() == 1;
    }

    JSHClass *GetKnownHClass() const
    {
        return HasKnownHClass() ? possibleHClasses_.front().hclass : nullptr;
    }

    bool HasPossibleHClasses() const
    {
        return !possibleHClasses_.empty();
    }

    PossibleHClasses GetPossibleHClasses() const;

    size_t PossibleHClassCount() const
    {
        return possibleHClasses_.size();
    }

    bool HClassIsStable() const
    {
        return HasKnownHClass() && possibleHClasses_.front().isStable;
    }

    void RecordHClass(JSHClass *hclass, bool isStable);
    void RecordPossibleHClasses(const PossibleHClasses &hclasses, bool isStable);
    void ClearUnstable();
    bool MergeWith(const KnownNodeInfo &other);

    bool NoInfoAvailable() const
    {
        return nodeKind_ == NodeKind::UNKNOWN && possibleHClasses_.empty();
    }

private:
    struct PossibleHClassInfo {
        JSHClass *hclass {nullptr};
        bool isStable {false};
    };

    static bool NodeKindIncludes(NodeKind lhs, NodeKind rhs);
    static NodeKind IntersectNodeKind(NodeKind lhs, NodeKind rhs);
    void AddPossibleHClass(JSHClass *hclass, bool isStable);
    void UnionPossibleHClasses(const KnownNodeInfo &other);

    NodeKind nodeKind_ {NodeKind::UNKNOWN};
    std::vector<PossibleHClassInfo> possibleHClasses_;
};

struct KnownLoadKey {
    enum class PropertyKeyKind : uint8_t {
        CONST_DATA_ID,
        TAGGED_VALUE,
    };

    static KnownLoadKey ConstDataId(ValueVertex *receiver, uint32_t constDataId, PropertyLookupResult plr)
    {
        return {receiver, PropertyKeyKind::CONST_DATA_ID, constDataId, plr.GetData()};
    }

    static KnownLoadKey TaggedValue(ValueVertex *receiver, JSTaggedValue key, PropertyLookupResult plr)
    {
        return {receiver, PropertyKeyKind::TAGGED_VALUE, key.GetRawData(), plr.GetData()};
    }

    ValueVertex *receiver {nullptr};
    PropertyKeyKind keyKind {PropertyKeyKind::CONST_DATA_ID};
    uint64_t keyData {0};
    uint32_t propertyLookupResult {0};
};

struct KnownLoadKeyCompare {
    bool operator()(const KnownLoadKey &lhs, const KnownLoadKey &rhs) const;
};

class KnownNodeAspect {
public:
    explicit KnownNodeAspect(Chunk *chunk);
    KnownNodeAspect(const KnownNodeAspect &other, Chunk *chunk);
    ~KnownNodeAspect() = default;

    NO_COPY_SEMANTIC(KnownNodeAspect);
    NO_MOVE_SEMANTIC(KnownNodeAspect);

    KnownNodeAspect *Clone(Chunk *chunk) const;

    KnownNodeInfo *TryGetInfoFor(ValueVertex *node);
    const KnownNodeInfo *TryGetInfoFor(ValueVertex *node) const;
    KnownNodeInfo *GetOrCreateInfoFor(ValueVertex *node);

    bool CheckType(ValueVertex *node, KnownNodeInfo::NodeKind kind) const;
    bool EnsureType(ValueVertex *node, KnownNodeInfo::NodeKind kind);

    void RecordHClass(ValueVertex *node, JSHClass *hclass, bool isStable);
    JSHClass *TryGetHClass(ValueVertex *node) const;
    void RecordPossibleHClasses(ValueVertex *node, const KnownNodeInfo::PossibleHClasses &hclasses, bool isStable);
    std::optional<KnownNodeInfo::PossibleHClasses> TryGetPossibleHClasses(ValueVertex *node) const;

    ValueVertex *TryFindLoadedProperty(const KnownLoadKey &key) const;
    void RecordLoadedProperty(const KnownLoadKey &key, ValueVertex *value);
    void ClearLoadedProperties();
    void ClearLoadedPropertiesForReceiver(ValueVertex *receiver);

    void ClearUnstable();
    void ClearAll();
    void Merge(const KnownNodeAspect &other);

    bool Empty() const
    {
        return nodeInfos_.empty() && loadedProperties_.empty();
    }

    size_t NodeInfoCount() const
    {
        return nodeInfos_.size();
    }

    size_t LoadedPropertyCount() const
    {
        return loadedProperties_.size();
    }

private:
    using NodeInfos = ChunkMap<ValueVertex *, KnownNodeInfo>;
    using LoadedProperties = ChunkMap<KnownLoadKey, ValueVertex *, KnownLoadKeyCompare>;

    NodeInfos nodeInfos_;
    LoadedProperties loadedProperties_;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_KNOWN_NODE_ASPECT_H
