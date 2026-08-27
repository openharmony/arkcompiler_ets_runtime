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

#ifndef ECMASCRIPT_ARKSTEED_COMPILE_INFO_FACTS_H
#define ECMASCRIPT_ARKSTEED_COMPILE_INFO_FACTS_H

#include <array>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <tuple>
#include <vector>

#include "ecmascript/arksteed/arksteed_vertex.h"
#include "ecmascript/base_env.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/lexical_env.h"
#include "ecmascript/mem/chunk_containers.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript::arksteed {

class ValueVertex;
enum class VertexOpcode : uint16_t;

class AlternativeNodes {
public:
    enum class Kind : uint8_t {
        TAGGED,
        INT32,
        UINT32,
        INT64,
        FLOAT64,
        HOLEY_FLOAT64,
        CHECKED_VALUE,
        COUNT,
    };

    AlternativeNodes();

    ValueVertex *Get(Kind kind) const;
    void Set(Kind kind, ValueVertex *node);
    bool Empty() const;
    void MergeWith(const AlternativeNodes &other);

private:
    static constexpr size_t STORE_SIZE = static_cast<size_t>(Kind::COUNT);
    std::array<ValueVertex *, STORE_SIZE> store_;
};

class NodeInfo {
public:
    using PossibleHClasses = std::vector<JSHClass *>;

    enum class NodeType : uint16_t {
        NONE = 0,
        INT = 1U << 0U,
        DOUBLE = 1U << 1U,
        NULL_TYPE = 1U << 2U,
        UNDEFINED = 1U << 3U,
        BOOLEAN = 1U << 4U,
        STRING = 1U << 5U,
        SYMBOL = 1U << 6U,
        BIGINT = 1U << 7U,
        JS_ARRAY = 1U << 8U,
        JS_FUNCTION = 1U << 9U,
        JS_TYPED_ARRAY = 1U << 10U,
        OTHER_JS_RECEIVER = 1U << 11U,

        NUMBER = static_cast<uint16_t>(INT) | static_cast<uint16_t>(DOUBLE),
        NULL_OR_UNDEFINED = static_cast<uint16_t>(NULL_TYPE) | static_cast<uint16_t>(UNDEFINED),
        ODDBALL = static_cast<uint16_t>(NULL_TYPE) | static_cast<uint16_t>(UNDEFINED) | static_cast<uint16_t>(BOOLEAN),
        JS_RECEIVER = static_cast<uint16_t>(JS_ARRAY) | static_cast<uint16_t>(JS_FUNCTION) |
                      static_cast<uint16_t>(JS_TYPED_ARRAY) | static_cast<uint16_t>(OTHER_JS_RECEIVER),
        ANY_HEAP_OBJECT = static_cast<uint16_t>(STRING) | static_cast<uint16_t>(SYMBOL) |
                          static_cast<uint16_t>(BIGINT) | static_cast<uint16_t>(JS_RECEIVER),
        UNKNOWN = static_cast<uint16_t>(INT) | static_cast<uint16_t>(DOUBLE) | static_cast<uint16_t>(NULL_TYPE) |
                  static_cast<uint16_t>(UNDEFINED) | static_cast<uint16_t>(BOOLEAN) | static_cast<uint16_t>(STRING) |
                  static_cast<uint16_t>(SYMBOL) | static_cast<uint16_t>(BIGINT) | static_cast<uint16_t>(JS_ARRAY) |
                  static_cast<uint16_t>(JS_FUNCTION) | static_cast<uint16_t>(JS_TYPED_ARRAY) |
                  static_cast<uint16_t>(OTHER_JS_RECEIVER),
    };

    NodeInfo() = default;
    ~NodeInfo() = default;
    DEFAULT_COPY_SEMANTIC(NodeInfo);
    DEFAULT_MOVE_SEMANTIC(NodeInfo);

    static constexpr uint16_t ToNodeTypeBits(NodeType type)
    {
        return static_cast<uint16_t>(type);
    }

    static constexpr NodeType ToNodeType(uint16_t bits)
    {
        return static_cast<NodeType>(bits);
    }

    static constexpr NodeType UnionNodeType(NodeType lhs, NodeType rhs)
    {
        return ToNodeType(ToNodeTypeBits(lhs) | ToNodeTypeBits(rhs));
    }

    static constexpr NodeType IntersectNodeType(NodeType lhs, NodeType rhs)
    {
        return ToNodeType(ToNodeTypeBits(lhs) & ToNodeTypeBits(rhs));
    }

    static constexpr NodeType RemoveNodeType(NodeType type, NodeType remove)
    {
        return ToNodeType(ToNodeTypeBits(type) & ~ToNodeTypeBits(remove));
    }

    static constexpr bool NodeTypeIs(NodeType type, NodeType expected)
    {
        return (ToNodeTypeBits(type) & ~ToNodeTypeBits(expected)) == 0;
    }

    static constexpr bool NodeTypeCanBe(NodeType type, NodeType expected)
    {
        return ToNodeTypeBits(IntersectNodeType(type, expected)) != ToNodeTypeBits(NodeType::NONE);
    }

    static constexpr bool IsEmptyNodeType(NodeType type)
    {
        return ToNodeTypeBits(type) == ToNodeTypeBits(NodeType::NONE);
    }

    NodeType GetType() const
    {
        return type_;
    }

    NodeType SetType(NodeType type)
    {
        type_ = type;
        return type_;
    }

    NodeType IntersectType(NodeType type)
    {
        type_ = IntersectNodeType(type_, type);
        return type_;
    }

    NodeType UnionType(NodeType type)
    {
        type_ = UnionNodeType(type_, type);
        return type_;
    }

    bool CheckType(NodeType type) const
    {
        return NodeTypeIs(type_, type);
    }

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
    void ClearPossibleHClasses();
    void ClearUnstable();
    bool MergeWith(const NodeInfo &other);

    AlternativeNodes &GetAlternatives()
    {
        return alternatives_;
    }

    const AlternativeNodes &GetAlternatives() const
    {
        return alternatives_;
    }

    bool NoInfoAvailable() const
    {
        return type_ == NodeType::UNKNOWN && alternatives_.Empty() && possibleHClasses_.empty();
    }

private:
    struct PossibleHClassInfo {
        JSHClass *hclass {nullptr};
        bool isStable {false};
    };

    void AddPossibleHClass(JSHClass *hclass, bool isStable);
    void UnionPossibleHClasses(const NodeInfo &other);

    NodeType type_ {NodeType::UNKNOWN};
    AlternativeNodes alternatives_;
    std::vector<PossibleHClassInfo> possibleHClasses_;
};

NodeInfo::NodeType NodeTypeFromJSTaggedValue(JSTaggedValue value);
NodeInfo::NodeType NodeTypeFromJSType(JSType type);
NodeInfo::NodeType NodeTypeFromHClass(const JSHClass *hclass);

class PropertyKey {
public:
    enum class Kind : uint8_t {
        NAMED,
        INDEX,
        ELEMENTS,
        LENGTH,
        CONST_DATA_ID,
        HEAP_CONSTANT,
        UNKNOWN,
    };

    static PropertyKey Named(uint32_t id)
    {
        return PropertyKey(Kind::NAMED, id);
    }

    static PropertyKey Index(uint32_t index)
    {
        return PropertyKey(Kind::INDEX, index);
    }

    static PropertyKey Elements()
    {
        return PropertyKey(Kind::ELEMENTS, 0);
    }

    static PropertyKey Length()
    {
        return PropertyKey(Kind::LENGTH, 0);
    }

    static PropertyKey ConstDataId(uint32_t constDataId)
    {
        return PropertyKey(Kind::CONST_DATA_ID, constDataId);
    }

    static PropertyKey HeapConstant(uint32_t handleIndex)
    {
        return PropertyKey(Kind::HEAP_CONSTANT, handleIndex);
    }

    static PropertyKey Unknown()
    {
        return PropertyKey(Kind::UNKNOWN, 0);
    }

    Kind GetKind() const
    {
        return kind_;
    }

    uint64_t GetPayload() const
    {
        return payload_;
    }

    bool operator<(const PropertyKey &other) const
    {
        return std::tie(kind_, payload_) < std::tie(other.kind_, other.payload_);
    }

    bool operator==(const PropertyKey &other) const
    {
        return kind_ == other.kind_ && payload_ == other.payload_;
    }

private:
    PropertyKey(Kind kind, uint64_t payload) : kind_(kind), payload_(payload) {}

    Kind kind_;
    uint64_t payload_;
};

class LoadedPropertyKey {
public:
    static LoadedPropertyKey FromPropertyKey(ValueVertex *receiver, PropertyKey propertyKey)
    {
        return LoadedPropertyKey(receiver, propertyKey, 0);
    }

    static LoadedPropertyKey ConstDataId(ValueVertex *receiver, uint32_t constDataId, PropertyLookupResult plr)
    {
        return LoadedPropertyKey(receiver, PropertyKey::ConstDataId(constDataId), plr.GetData());
    }

    static LoadedPropertyKey HeapConstant(ValueVertex *receiver, uint32_t handleIndex, PropertyLookupResult plr)
    {
        return LoadedPropertyKey(receiver, PropertyKey::HeapConstant(handleIndex), plr.GetData());
    }

    ValueVertex *GetReceiver() const
    {
        return receiver_;
    }

    PropertyKey GetPropertyKey() const
    {
        return propertyKey_;
    }

    uint32_t GetPropertyLookupResult() const
    {
        return propertyLookupResult_;
    }

    bool MatchesReceiver(ValueVertex *receiver) const
    {
        return receiver_ == receiver;
    }

    bool MatchesPropertyKey(PropertyKey key) const
    {
        return propertyKey_ == key;
    }

private:
    LoadedPropertyKey(ValueVertex *receiver, PropertyKey propertyKey, uint32_t propertyLookupResult)
        : receiver_(receiver), propertyKey_(propertyKey), propertyLookupResult_(propertyLookupResult)
    {
    }

    ValueVertex *receiver_ {nullptr};
    PropertyKey propertyKey_ {PropertyKey::Unknown()};
    uint32_t propertyLookupResult_ {0};

    friend struct LoadedPropertyKeyCompare;
};

struct LoadedPropertyKeyCompare {
    bool operator()(const LoadedPropertyKey &lhs, const LoadedPropertyKey &rhs) const;
};

class EnvSlotKey {
public:
    EnvSlotKey(ValueVertex *env, int32_t slot) : env_(env), slot_(slot) {}

    ValueVertex *GetEnv() const
    {
        return env_;
    }

    int32_t GetSlot() const
    {
        return slot_;
    }

    bool operator<(const EnvSlotKey &other) const
    {
        if (std::less<ValueVertex *>()(env_, other.env_)) {
            return true;
        }
        if (std::less<ValueVertex *>()(other.env_, env_)) {
            return false;
        }
        return slot_ < other.slot_;
    }

private:
    ValueVertex *env_;
    int32_t slot_;
};

inline bool IsEnvConstantFieldOffset(int32_t offset)
{
    constexpr int32_t taggedSize = static_cast<int32_t>(JSTaggedValue::TaggedTypeSize());
    constexpr int32_t dataOffset = static_cast<int32_t>(TaggedArray::DATA_OFFSET);
    return offset == dataOffset + static_cast<int32_t>(BaseEnv::GLOBAL_ENV_INDEX) * taggedSize ||
           offset == dataOffset + static_cast<int32_t>(LexicalEnv::PARENT_ENV_INDEX) * taggedSize;
}

enum class EnvSlotAliasMode : uint8_t {
    NONE,
    CURRENT_ENV_ONLY,
    CONSTANT_ENV_ONLY,
    MAY_ALIAS,
};

struct SideEffectDescriptor {
    SideEffectKind kind {SideEffectKind::NO_SIDE_EFFECT};
    ValueVertex *receiver {nullptr};
    PropertyKey propertyKey {PropertyKey::Unknown()};
    ValueVertex *env {nullptr};
    int32_t envSlot {-1};
    ValueVertex *envSlotValue {nullptr};
};

class CompileInfoFacts {
public:
    using ExpressionInputs = ChunkVector<ValueVertex *>;
    using ExpressionOptions = ChunkVector<uint64_t>;
    using ClearedEnvSlotKeys = ChunkVector<EnvSlotKey>;

    explicit CompileInfoFacts(Chunk *chunk);

    NO_COPY_SEMANTIC(CompileInfoFacts);
    NO_MOVE_SEMANTIC(CompileInfoFacts);

    CompileInfoFacts *Clone() const;
    CompileInfoFacts *CloneForLoopHeader() const;
    void Merge(const CompileInfoFacts &other);

    NodeInfo *GetOrCreateInfoFor(ValueVertex *node);
    NodeInfo *TryGetInfoFor(ValueVertex *node);
    const NodeInfo *TryGetInfoFor(ValueVertex *node) const;

    NodeInfo::NodeType GetKnownType(ValueVertex *node) const;
    bool MayBeNullOrUndefined(ValueVertex *node) const;
    bool CheckType(ValueVertex *node, NodeInfo::NodeType type) const;
    template <size_t N>
    std::optional<NodeInfo::NodeType> CheckTypes(ValueVertex *node,
                                                 const std::array<NodeInfo::NodeType, N> &types) const
    {
        NodeInfo::NodeType knownType = GetKnownType(node);
        if (NodeInfo::IsEmptyNodeType(knownType)) {
            return std::nullopt;
        }

        for (NodeInfo::NodeType type : types) {
            if (!NodeInfo::NodeTypeIs(knownType, type)) {
                continue;
            }
            return type;
        }
        return std::nullopt;
    }

    NodeInfo::NodeType EnsureType(ValueVertex *node, NodeInfo::NodeType type);
    void RecordHClass(ValueVertex *node, JSHClass *hclass, bool isStable);
    JSHClass *TryGetHClass(ValueVertex *node) const;
    void RecordPossibleHClasses(ValueVertex *node, const NodeInfo::PossibleHClasses &hclasses, bool isStable);
    void SetPossibleHClasses(ValueVertex *node, const ChunkVector<JSHClass *> &hclasses,
                             NodeInfo::NodeType possibleType);
    std::optional<NodeInfo::PossibleHClasses> TryGetPossibleHClasses(ValueVertex *node) const;
    void SetAlternative(ValueVertex *node, AlternativeNodes::Kind kind, ValueVertex *alternative);
    ValueVertex *TryGetAlternative(ValueVertex *node, AlternativeNodes::Kind kind) const;

    void RecordLoadedProperty(const LoadedPropertyKey &key, ValueVertex *value);
    ValueVertex *LookupLoadedProperty(const LoadedPropertyKey &key) const;
    void RecordLoadedProperty(ValueVertex *object, PropertyKey key, ValueVertex *value);
    ValueVertex *LookupLoadedProperty(ValueVertex *object, PropertyKey key) const;
    void RecordLoadedConstantProperty(const LoadedPropertyKey &key, ValueVertex *value);
    ValueVertex *LookupLoadedConstantProperty(const LoadedPropertyKey &key) const;
    void RecordLoadedConstantProperty(ValueVertex *object, PropertyKey key, ValueVertex *value);
    ValueVertex *LookupLoadedConstantProperty(ValueVertex *object, PropertyKey key) const;
    void ClearLoadedProperties();
    void ClearLoadedPropertiesForReceiver(ValueVertex *receiver);
    void ClearLoadedPropertiesForKey(PropertyKey key);

    void RecordEnvSlot(ValueVertex *env, int32_t slot, ValueVertex *value);
    ValueVertex *LookupEnvSlot(ValueVertex *env, int32_t slot) const;
    void RecordEnvConstant(ValueVertex *env, int32_t slot, ValueVertex *value);
    ValueVertex *LookupEnvConstant(ValueVertex *env, int32_t slot) const;
    ClearedEnvSlotKeys ClearAliasedEnvSlotsFor(ValueVertex *env, int32_t slot, ValueVertex *newValue);
    void ClearEnvSlotsFor(ValueVertex *env);

    void AddExpression(uint32_t hash, ValueVertex *node, const ExpressionInputs &inputs,
                       const ExpressionOptions &options, bool needsEpochCheck);
    ValueVertex *FindExpression(uint32_t hash, VertexOpcode opcode, const ExpressionInputs &inputs,
                                const ExpressionOptions &options, bool needsEpochCheck);
    void ClearAvailableExpressions();

    void MarkPossibleSideEffect(const SideEffectDescriptor &effect);
    void ClearUnstable();
    void ClearAll();
    void OnSideEffect();
    void IncrementEffectEpoch();

    EnvSlotAliasMode GetEnvSlotAliasMode() const
    {
        return envSlotAliasMode_;
    }

    void SetEnvSlotAliasMode(EnvSlotAliasMode mode)
    {
        envSlotAliasMode_ = mode;
    }

    uint32_t GetEffectEpoch() const
    {
        return effectEpoch_;
    }

    bool Empty() const
    {
        return nodeInfos_.empty() && loadedProperties_.empty() && loadedConstantProperties_.empty() &&
               loadedEnvSlots_.empty() && loadedEnvConstants_.empty() && availableExpressions_.empty();
    }

private:
    static constexpr uint32_t EFFECT_EPOCH_FOR_PURE_INSTRUCTIONS = std::numeric_limits<uint32_t>::max();
    static constexpr uint32_t EFFECT_EPOCH_OVERFLOW = EFFECT_EPOCH_FOR_PURE_INSTRUCTIONS - 1;

    using NodeInfos = ChunkMap<ValueVertex *, NodeInfo>;
    using LoadedPropertyMap = ChunkMap<LoadedPropertyKey, ValueVertex *, LoadedPropertyKeyCompare>;
    using LoadedEnvSlots = ChunkMap<EnvSlotKey, ValueVertex *>;

    struct AvailableExpression {
        ValueVertex *node {nullptr};
        VertexOpcode opcode {};
        ExpressionInputs *inputs {nullptr};
        ExpressionOptions *options {nullptr};
        uint32_t effectEpoch {0};
    };

    NodeInfo::NodeType GetStaticNodeType(ValueVertex *node) const;
    void AdvanceEpochAfterMerge(uint32_t otherEpoch);
    ExpressionInputs *CopyExpressionInputs(const ExpressionInputs &inputs);
    ExpressionOptions *CopyExpressionOptions(const ExpressionOptions &options);
    bool ExpressionMatches(const AvailableExpression &expression, VertexOpcode opcode, const ExpressionInputs &inputs,
                           const ExpressionOptions &options, bool needsEpochCheck) const;
    ValueVertex *LookupLoadedProperty(const LoadedPropertyMap &map, const LoadedPropertyKey &key) const;
    void RecordLoadedProperty(LoadedPropertyMap &map, const LoadedPropertyKey &key, ValueVertex *value);
    void MergeLoadedProperties(LoadedPropertyMap &target, const LoadedPropertyMap &other);
    void MergeEnvSlots(LoadedEnvSlots &target, const LoadedEnvSlots &other);
    void MergeAvailableExpressions(const CompileInfoFacts &other);
    void CopyLoadedProperties(LoadedPropertyMap &target, const LoadedPropertyMap &source) const;
    void CopyEnvSlots(LoadedEnvSlots &target, const LoadedEnvSlots &source) const;
    void UpdateEnvSlotAliasMode(ValueVertex *env);
    void RecomputeEnvSlotAliasMode();
    bool EnvMayAlias(ValueVertex *lhs, ValueVertex *rhs) const;

    static EnvSlotAliasMode MergeEnvSlotAliasMode(EnvSlotAliasMode lhs, EnvSlotAliasMode rhs);

    Chunk *chunk_;
    NodeInfos nodeInfos_;
    LoadedPropertyMap loadedProperties_;
    LoadedPropertyMap loadedConstantProperties_;
    LoadedEnvSlots loadedEnvSlots_;
    LoadedEnvSlots loadedEnvConstants_;
    ChunkMap<uint32_t, AvailableExpression> availableExpressions_;
    uint32_t effectEpoch_ {0};
    EnvSlotAliasMode envSlotAliasMode_ {EnvSlotAliasMode::NONE};
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_COMPILE_INFO_FACTS_H
