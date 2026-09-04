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

#include "ecmascript/arksteed/arksteed_compile_info_facts.h"

#include <algorithm>
#include <tuple>

#include "ecmascript/arksteed/arksteed_opcode.h"
#include "ecmascript/mem/tagged_object.h"

namespace panda::ecmascript::arksteed {
namespace {
bool InJSTypeRange(JSType type, JSType first, JSType last)
{
    return static_cast<uint8_t>(type) >= static_cast<uint8_t>(first) &&
           static_cast<uint8_t>(type) <= static_cast<uint8_t>(last);
}

bool IsJSArrayLikeJSType(JSType type)
{
    return type == JSType::JS_ARRAY || InJSTypeRange(type, JSType::JS_API_ARRAY_LIST, JSType::JS_API_QUEUE);
}

bool IsJSTypedArrayJSType(JSType type)
{
    return InJSTypeRange(type, JSType::JS_TYPED_ARRAY_FIRST, JSType::JS_TYPED_ARRAY_LAST) ||
           InJSTypeRange(type, JSType::JS_SHARED_TYPED_ARRAY_FIRST, JSType::JS_SHARED_TYPED_ARRAY_LAST);
}

bool IsEcmaObjectJSType(JSType type)
{
    return static_cast<uint8_t>(type) > static_cast<uint8_t>(JSType::STRING_LAST);
}
}  // namespace

NodeInfo::NodeType NodeTypeFromJSTaggedValue(JSTaggedValue value)
{
    if (value.IsInt()) {
        return NodeInfo::NodeType::INT;
    }
    if (value.IsNull()) {
        return NodeInfo::NodeType::NULL_TYPE;
    }
    if (value.IsUndefined()) {
        return NodeInfo::NodeType::UNDEFINED;
    }
    if (value.IsTrue() || value.IsFalse()) {
        return NodeInfo::NodeType::BOOLEAN;
    }
    if (value.IsDouble()) {
        return NodeInfo::NodeType::DOUBLE;
    }
    if (value.IsHeapObject()) {
        return NodeTypeFromHClass(value.GetTaggedObject()->GetClass());
    }
    return NodeInfo::NodeType::UNKNOWN;
}

NodeInfo::NodeType NodeTypeFromJSType(JSType type)
{
    if (InJSTypeRange(type, JSType::STRING_FIRST, JSType::STRING_LAST)) {
        return NodeInfo::NodeType::STRING;
    }
    if (type == JSType::SYMBOL) {
        return NodeInfo::NodeType::SYMBOL;
    }
    if (type == JSType::BIGINT) {
        return NodeInfo::NodeType::BIGINT;
    }
    if (IsJSArrayLikeJSType(type)) {
        return NodeInfo::NodeType::JS_ARRAY;
    }
    if (InJSTypeRange(type, JSType::JS_FUNCTION_FIRST, JSType::JS_FUNCTION_LAST)) {
        return NodeInfo::NodeType::JS_FUNCTION;
    }
    if (IsJSTypedArrayJSType(type)) {
        return NodeInfo::NodeType::JS_TYPED_ARRAY;
    }
    if (IsEcmaObjectJSType(type)) {
        return NodeInfo::NodeType::OTHER_JS_RECEIVER;
    }
    return NodeInfo::NodeType::UNKNOWN;
}

NodeInfo::NodeType NodeTypeFromHClass(const JSHClass *hclass)
{
    if (hclass == nullptr) {
        return NodeInfo::NodeType::UNKNOWN;
    }
    return NodeTypeFromJSType(hclass->GetObjectType());
}

AlternativeNodes::AlternativeNodes()
{
    store_.fill(nullptr);
}

ValueVertex *AlternativeNodes::Get(Kind kind) const
{
    return store_[static_cast<size_t>(kind)];
}

void AlternativeNodes::Set(Kind kind, ValueVertex *node)
{
    ASSERT(node != nullptr);
    store_[static_cast<size_t>(kind)] = node;
}

bool AlternativeNodes::Empty() const
{
    for (ValueVertex *node : store_) {
        if (node != nullptr) {
            return false;
        }
    }
    return true;
}

void AlternativeNodes::MergeWith(const AlternativeNodes &other)
{
    for (size_t index = 0; index < STORE_SIZE; ++index) {
        if (store_[index] != nullptr && store_[index] != other.store_[index]) {
            store_[index] = nullptr;
        }
    }
}

NodeInfo::PossibleHClasses NodeInfo::GetFreshPossibleHClasses() const
{
    if (!HasFreshPossibleHClasses()) {
        return {};
    }
    return GetPossibleHClassesForRevalidation();
}

NodeInfo::PossibleHClasses NodeInfo::GetPossibleHClassesForRevalidation() const
{
    PossibleHClasses result;
    result.reserve(possibleHClasses_.size());
    for (const auto &info : possibleHClasses_) {
        result.push_back(info.hclass);
    }
    return result;
}

bool NodeInfo::HasUnstablePossibleHClass() const
{
    return std::any_of(possibleHClasses_.begin(), possibleHClasses_.end(),
                       [](const PossibleHClassInfo &info) { return !info.isStable; });
}

bool NodeInfo::ContainsPossibleHClass(JSHClass *hclass) const
{
    return std::any_of(possibleHClasses_.begin(), possibleHClasses_.end(),
                       [hclass](const PossibleHClassInfo &info) { return info.hclass == hclass; });
}

void NodeInfo::RecordHClass(JSHClass *hclass, bool isStable)
{
    possibleHClasses_.clear();
    possibleHClassesAreStale_ = false;
    AddPossibleHClass(hclass, isStable);
    IntersectTypeWithPossibleHClasses();
    CheckPossibleHClassInvariants();
}

void NodeInfo::RecordPossibleHClasses(const PossibleHClasses &hclasses, bool isStable)
{
    possibleHClasses_.clear();
    possibleHClassesAreStale_ = false;
    for (JSHClass *hclass : hclasses) {
        AddPossibleHClass(hclass, isStable);
    }
    IntersectTypeWithPossibleHClasses();
    CheckPossibleHClassInvariants();
}

void NodeInfo::RecordPossibleHClasses(const PossibleHClassInfos &hclasses)
{
    possibleHClasses_.clear();
    possibleHClassesAreStale_ = false;
    for (const PossibleHClassInfo &info : hclasses) {
        AddPossibleHClass(info.hclass, info.isStable);
    }
    IntersectTypeWithPossibleHClasses();
    CheckPossibleHClassInvariants();
}

bool NodeInfo::NarrowPossibleHClasses(const PossibleHClasses &hclasses)
{
    ASSERT(HasFreshPossibleHClasses());
    possibleHClasses_.erase(
        std::remove_if(possibleHClasses_.begin(), possibleHClasses_.end(), [&hclasses](const auto &info) {
            return std::find(hclasses.begin(), hclasses.end(), info.hclass) == hclasses.end();
        }),
        possibleHClasses_.end());
    if (possibleHClasses_.empty()) {
        possibleHClassesAreStale_ = false;
        CheckPossibleHClassInvariants();
        return false;
    }
    IntersectTypeWithPossibleHClasses();
    CheckPossibleHClassInvariants();
    return true;
}

void NodeInfo::ClearPossibleHClasses()
{
    possibleHClasses_.clear();
    possibleHClassesAreStale_ = false;
    CheckPossibleHClassInvariants();
}

void NodeInfo::MarkUnstableHClassesStale()
{
    if (HasUnstablePossibleHClass()) {
        possibleHClassesAreStale_ = true;
    }
    CheckPossibleHClassInvariants();
}

void NodeInfo::MarkHClassesStaleForElementsKindTransition(const PossibleHClasses &sourceHClasses)
{
    if (!HasFreshPossibleHClasses()) {
        return;
    }
    bool containsMatchingHClass = false;
    for (PossibleHClassInfo &info : possibleHClasses_) {
        if (std::find(sourceHClasses.begin(), sourceHClasses.end(), info.hclass) == sourceHClasses.end()) {
            continue;
        }
        info.isStable = false;
        containsMatchingHClass = true;
    }
    if (containsMatchingHClass) {
        possibleHClassesAreStale_ = true;
    }
    CheckPossibleHClassInvariants();
}

void NodeInfo::MarkPossibleHClassesFresh()
{
    ASSERT(!possibleHClasses_.empty());
    possibleHClassesAreStale_ = false;
    CheckPossibleHClassInvariants();
}

bool NodeInfo::MarkPossibleHClassStable(JSHClass *hclass)
{
    ASSERT(!possibleHClassesAreStale_);
    auto it = std::find_if(possibleHClasses_.begin(), possibleHClasses_.end(),
                           [hclass](const PossibleHClassInfo &info) { return info.hclass == hclass; });
    if (it == possibleHClasses_.end()) {
        return false;
    }
    it->isStable = true;
    CheckPossibleHClassInvariants();
    return true;
}

bool NodeInfo::MergeWith(const NodeInfo &other)
{
    UnionType(other.type_);
    nonHole_ = nonHole_ && other.nonHole_;
    alternatives_.MergeWith(other.alternatives_);
    bool mergedHClassesAreStale = possibleHClassesAreStale_ || other.possibleHClassesAreStale_;
    UnionPossibleHClasses(other);
    possibleHClassesAreStale_ = possibleHClasses_.empty() ? false : mergedHClassesAreStale;
    CheckPossibleHClassInvariants();
    return !NoInfoAvailable();
}

void NodeInfo::AddPossibleHClass(JSHClass *hclass, bool isStable)
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

void NodeInfo::IntersectTypeWithPossibleHClasses()
{
    if (possibleHClasses_.empty()) {
        return;
    }
    IntersectType(NodeType::JS_RECEIVER);
    NodeType possibleType = NodeType::NONE;
    for (const PossibleHClassInfo &info : possibleHClasses_) {
        possibleType = UnionNodeType(possibleType, NodeTypeFromHClass(info.hclass));
    }
    possibleType = IntersectNodeType(possibleType, NodeType::JS_RECEIVER);
    if (!IsEmptyNodeType(possibleType)) {
        IntersectType(possibleType);
    }
}

void NodeInfo::UnionPossibleHClasses(const NodeInfo &other)
{
    if (possibleHClasses_.empty() || other.possibleHClasses_.empty()) {
        possibleHClasses_.clear();
        return;
    }
    for (const auto &otherInfo : other.possibleHClasses_) {
        AddPossibleHClass(otherInfo.hclass, otherInfo.isStable);
    }
}

void NodeInfo::CheckPossibleHClassInvariants() const
{
    ASSERT(!possibleHClasses_.empty() || !possibleHClassesAreStale_);
    ASSERT(!possibleHClassesAreStale_ || HasUnstablePossibleHClass());
}

bool LoadedPropertyKeyCompare::operator()(const LoadedPropertyKey &lhs, const LoadedPropertyKey &rhs) const
{
    return std::tie(lhs.receiver_, lhs.propertyKey_, lhs.propertyLookupResult_) <
           std::tie(rhs.receiver_, rhs.propertyKey_, rhs.propertyLookupResult_);
}

CompileInfoFacts::CompileInfoFacts(Chunk *chunk)
    : chunk_(chunk),
      nodeInfos_(chunk),
      loadedProperties_(chunk),
      loadedConstantProperties_(chunk),
      loadedEnvSlots_(chunk),
      loadedEnvConstants_(chunk),
      availableExpressions_(chunk)
{
    ASSERT(chunk_ != nullptr);
}

CompileInfoFacts *CompileInfoFacts::Clone() const
{
    auto *copy = chunk_->New<CompileInfoFacts>(chunk_);
    copy->nodeInfos_.insert(nodeInfos_.begin(), nodeInfos_.end());
    copy->CopyLoadedProperties(copy->loadedProperties_, loadedProperties_);
    copy->CopyLoadedProperties(copy->loadedConstantProperties_, loadedConstantProperties_);
    copy->CopyEnvSlots(copy->loadedEnvSlots_, loadedEnvSlots_);
    copy->CopyEnvSlots(copy->loadedEnvConstants_, loadedEnvConstants_);
    for (const auto &entry : availableExpressions_) {
        const AvailableExpression &expression = entry.second;
        copy->availableExpressions_.emplace(entry.first, AvailableExpression {
                                                             expression.node,
                                                             expression.opcode,
                                                             copy->CopyExpressionInputs(*expression.inputs),
                                                             copy->CopyExpressionOptions(*expression.options),
                                                             expression.effectEpoch,
                                                         });
    }
    copy->effectEpoch_ = effectEpoch_;
    copy->envSlotAliasMode_ = envSlotAliasMode_;
    copy->freshUnstableHClassesRequireInvalidation_ = freshUnstableHClassesRequireInvalidation_;
    return copy;
}

CompileInfoFacts *CompileInfoFacts::CloneForLoopHeader() const
{
    auto *copy = chunk_->New<CompileInfoFacts>(chunk_);
    copy->nodeInfos_.insert(nodeInfos_.begin(), nodeInfos_.end());
    for (auto infoIt = copy->nodeInfos_.begin(); infoIt != copy->nodeInfos_.end();) {
        infoIt->second.MarkUnstableHClassesStale();
        if (infoIt->second.NoInfoAvailable()) {
            infoIt = copy->nodeInfos_.erase(infoIt);
        } else {
            ++infoIt;
        }
    }
    copy->CopyLoadedProperties(copy->loadedConstantProperties_, loadedConstantProperties_);
    copy->CopyEnvSlots(copy->loadedEnvConstants_, loadedEnvConstants_);
    copy->effectEpoch_ = effectEpoch_;
    copy->IncrementEffectEpoch();
    copy->envSlotAliasMode_ = EnvSlotAliasMode::NONE;
    copy->freshUnstableHClassesRequireInvalidation_ = false;
    return copy;
}

void CompileInfoFacts::Merge(const CompileInfoFacts &other)
{
    for (auto infoIt = nodeInfos_.begin(); infoIt != nodeInfos_.end();) {
        auto otherIt = other.nodeInfos_.find(infoIt->first);
        if (otherIt == other.nodeInfos_.end() || !infoIt->second.MergeWith(otherIt->second)) {
            infoIt = nodeInfos_.erase(infoIt);
        } else {
            ++infoIt;
        }
    }

    MergeLoadedProperties(loadedProperties_, other.loadedProperties_);
    MergeLoadedProperties(loadedConstantProperties_, other.loadedConstantProperties_);
    if (effectEpoch_ != other.effectEpoch_) {
        AdvanceEpochAfterMerge(other.effectEpoch_);
    }
    MergeEnvSlots(loadedEnvSlots_, other.loadedEnvSlots_);
    MergeEnvSlots(loadedEnvConstants_, other.loadedEnvConstants_);
    MergeAvailableExpressions(other);
    envSlotAliasMode_ = MergeEnvSlotAliasMode(envSlotAliasMode_, other.envSlotAliasMode_);
    RecomputeFreshUnstableHClassesRequireInvalidation();
}

NodeInfo *CompileInfoFacts::GetOrCreateInfoFor(ValueVertex *node)
{
    ASSERT(node != nullptr);
    auto [it, inserted] = nodeInfos_.emplace(node, NodeInfo());
    static_cast<void>(inserted);
    return &it->second;
}

NodeInfo *CompileInfoFacts::TryGetInfoFor(ValueVertex *node)
{
    auto it = nodeInfos_.find(node);
    return it == nodeInfos_.end() ? nullptr : &it->second;
}

const NodeInfo *CompileInfoFacts::TryGetInfoFor(ValueVertex *node) const
{
    auto it = nodeInfos_.find(node);
    return it == nodeInfos_.end() ? nullptr : &it->second;
}

NodeInfo::NodeType CompileInfoFacts::GetKnownType(ValueVertex *node) const
{
    NodeInfo::NodeType staticType = GetStaticNodeType(node);
    const NodeInfo *info = TryGetInfoFor(node);
    if (info == nullptr) {
        return staticType;
    }
    return NodeInfo::IntersectNodeType(staticType, info->GetType());
}

bool CompileInfoFacts::MayBeNullOrUndefined(ValueVertex *node) const
{
    return NodeInfo::NodeTypeCanBe(GetKnownType(node), NodeInfo::NodeType::NULL_OR_UNDEFINED);
}

bool CompileInfoFacts::CheckType(ValueVertex *node, NodeInfo::NodeType type) const
{
    NodeInfo::NodeType knownType = GetKnownType(node);
    return !NodeInfo::IsEmptyNodeType(knownType) && NodeInfo::NodeTypeIs(knownType, type);
}

NodeInfo::NodeType CompileInfoFacts::EnsureType(ValueVertex *node, NodeInfo::NodeType type)
{
    return GetOrCreateInfoFor(node)->IntersectType(type);
}

void CompileInfoFacts::RecordNonHole(ValueVertex *node)
{
    ASSERT(node != nullptr);
    GetOrCreateInfoFor(node)->MarkNonHole();
}

bool CompileInfoFacts::IsKnownNonHole(ValueVertex *node) const
{
    ASSERT(node != nullptr);
    const NodeInfo *info = TryGetInfoFor(node);
    return info != nullptr && info->IsNonHole();
}

void CompileInfoFacts::RecordHClass(ValueVertex *node, JSHClass *hclass, bool isStable)
{
    NodeInfo *info = GetOrCreateInfoFor(node);
    info->RecordHClass(hclass, isStable);
    if (info->HasFreshPossibleHClasses() && info->HasUnstablePossibleHClass()) {
        freshUnstableHClassesRequireInvalidation_ = true;
    }
}

JSHClass *CompileInfoFacts::TryGetHClass(ValueVertex *node) const
{
    const NodeInfo *info = TryGetInfoFor(node);
    if (info == nullptr || !info->HasFreshKnownHClass()) {
        return nullptr;
    }
    return info->GetFreshKnownHClass();
}

void CompileInfoFacts::RecordPossibleHClasses(ValueVertex *node, const NodeInfo::PossibleHClasses &hclasses,
                                              bool isStable)
{
    NodeInfo *info = GetOrCreateInfoFor(node);
    info->RecordPossibleHClasses(hclasses, isStable);
    if (info->HasFreshPossibleHClasses() && info->HasUnstablePossibleHClass()) {
        freshUnstableHClassesRequireInvalidation_ = true;
    }
}

void CompileInfoFacts::RecordPossibleHClasses(ValueVertex *node, const NodeInfo::PossibleHClassInfos &hclasses)
{
    NodeInfo *info = GetOrCreateInfoFor(node);
    info->RecordPossibleHClasses(hclasses);
    if (info->HasFreshPossibleHClasses() && info->HasUnstablePossibleHClass()) {
        freshUnstableHClassesRequireInvalidation_ = true;
    }
}

bool CompileInfoFacts::NarrowPossibleHClasses(ValueVertex *node, const NodeInfo::PossibleHClasses &hclasses)
{
    NodeInfo *info = TryGetInfoFor(node);
    if (info == nullptr || !info->HasFreshPossibleHClasses() || !info->NarrowPossibleHClasses(hclasses)) {
        return false;
    }
    if (info->HasUnstablePossibleHClass()) {
        freshUnstableHClassesRequireInvalidation_ = true;
    }
    return true;
}

void CompileInfoFacts::SetPossibleHClasses(ValueVertex *node, const ChunkVector<JSHClass *> &hclasses,
                                           NodeInfo::NodeType possibleType)
{
    NodeInfo::PossibleHClasses copy;
    copy.reserve(hclasses.size());
    copy.insert(copy.end(), hclasses.begin(), hclasses.end());
    RecordPossibleHClasses(node, copy, false);
    NodeInfo *info = GetOrCreateInfoFor(node);
    info->IntersectType(possibleType);
}

std::optional<NodeInfo::PossibleHClasses> CompileInfoFacts::TryGetPossibleHClasses(ValueVertex *node) const
{
    const NodeInfo *info = TryGetInfoFor(node);
    if (info == nullptr || !info->HasFreshPossibleHClasses()) {
        return std::nullopt;
    }
    return info->GetFreshPossibleHClasses();
}

std::optional<NodeInfo::PossibleHClasses> CompileInfoFacts::TryGetPossibleHClassesForRevalidation(
    ValueVertex *node) const
{
    const NodeInfo *info = TryGetInfoFor(node);
    if (info == nullptr || info->PossibleHClassCount() == 0) {
        return std::nullopt;
    }
    return info->GetPossibleHClassesForRevalidation();
}

bool CompileInfoFacts::PossibleHClassesAreStale(ValueVertex *node) const
{
    const NodeInfo *info = TryGetInfoFor(node);
    return info != nullptr && info->PossibleHClassesAreStale();
}

void CompileInfoFacts::MarkPossibleHClassesFresh(ValueVertex *node)
{
    NodeInfo *info = TryGetInfoFor(node);
    ASSERT(info != nullptr && info->PossibleHClassesAreStale());
    info->MarkPossibleHClassesFresh();
    if (info->HasUnstablePossibleHClass()) {
        freshUnstableHClassesRequireInvalidation_ = true;
    }
}

bool CompileInfoFacts::MarkPossibleHClassStable(ValueVertex *node, JSHClass *hclass)
{
    NodeInfo *info = TryGetInfoFor(node);
    return info != nullptr && info->MarkPossibleHClassStable(hclass);
}

void CompileInfoFacts::SetAlternative(ValueVertex *node, AlternativeNodes::Kind kind, ValueVertex *alternative)
{
    GetOrCreateInfoFor(node)->GetAlternatives().Set(kind, alternative);
}

ValueVertex *CompileInfoFacts::TryGetAlternative(ValueVertex *node, AlternativeNodes::Kind kind) const
{
    const NodeInfo *info = TryGetInfoFor(node);
    return info == nullptr ? nullptr : info->GetAlternatives().Get(kind);
}

void CompileInfoFacts::RecordLoadedProperty(const LoadedPropertyKey &key, ValueVertex *value)
{
    RecordLoadedProperty(loadedProperties_, key, value);
}

ValueVertex *CompileInfoFacts::LookupLoadedProperty(const LoadedPropertyKey &key) const
{
    return LookupLoadedProperty(loadedProperties_, key);
}

void CompileInfoFacts::RecordLoadedProperty(ValueVertex *object, PropertyKey key, ValueVertex *value)
{
    RecordLoadedProperty(LoadedPropertyKey::FromPropertyKey(object, key), value);
}

ValueVertex *CompileInfoFacts::LookupLoadedProperty(ValueVertex *object, PropertyKey key) const
{
    return LookupLoadedProperty(LoadedPropertyKey::FromPropertyKey(object, key));
}

void CompileInfoFacts::RecordLoadedConstantProperty(const LoadedPropertyKey &key, ValueVertex *value)
{
    RecordLoadedProperty(loadedConstantProperties_, key, value);
}

ValueVertex *CompileInfoFacts::LookupLoadedConstantProperty(const LoadedPropertyKey &key) const
{
    return LookupLoadedProperty(loadedConstantProperties_, key);
}

void CompileInfoFacts::RecordLoadedConstantProperty(ValueVertex *object, PropertyKey key, ValueVertex *value)
{
    RecordLoadedConstantProperty(LoadedPropertyKey::FromPropertyKey(object, key), value);
}

ValueVertex *CompileInfoFacts::LookupLoadedConstantProperty(ValueVertex *object, PropertyKey key) const
{
    return LookupLoadedConstantProperty(LoadedPropertyKey::FromPropertyKey(object, key));
}

void CompileInfoFacts::ClearLoadedProperties()
{
    loadedProperties_.clear();
}

void CompileInfoFacts::ClearLoadedPropertiesForReceiver(ValueVertex *receiver)
{
    for (auto it = loadedProperties_.begin(); it != loadedProperties_.end();) {
        if (it->first.MatchesReceiver(receiver)) {
            it = loadedProperties_.erase(it);
        } else {
            ++it;
        }
    }
}

void CompileInfoFacts::ClearLoadedPropertiesForKey(PropertyKey key)
{
    if (key.GetKind() == PropertyKey::Kind::UNKNOWN) {
        ClearLoadedProperties();
        return;
    }
    for (auto it = loadedProperties_.begin(); it != loadedProperties_.end();) {
        if (it->first.MatchesPropertyKey(key)) {
            it = loadedProperties_.erase(it);
        } else {
            ++it;
        }
    }
}

void CompileInfoFacts::RecordEnvSlot(ValueVertex *env, int32_t slot, ValueVertex *value)
{
    ASSERT(env != nullptr);
    ASSERT(value != nullptr);
    loadedEnvSlots_[EnvSlotKey(env, slot)] = value;
    UpdateEnvSlotAliasMode(env);
}

ValueVertex *CompileInfoFacts::LookupEnvSlot(ValueVertex *env, int32_t slot) const
{
    auto it = loadedEnvSlots_.find(EnvSlotKey(env, slot));
    return it == loadedEnvSlots_.end() ? nullptr : it->second;
}

void CompileInfoFacts::RecordEnvConstant(ValueVertex *env, int32_t slot, ValueVertex *value)
{
    ASSERT(env != nullptr);
    ASSERT(value != nullptr);
    loadedEnvConstants_[EnvSlotKey(env, slot)] = value;
}

ValueVertex *CompileInfoFacts::LookupEnvConstant(ValueVertex *env, int32_t slot) const
{
    auto it = loadedEnvConstants_.find(EnvSlotKey(env, slot));
    return it == loadedEnvConstants_.end() ? nullptr : it->second;
}

CompileInfoFacts::ClearedEnvSlotKeys CompileInfoFacts::ClearAliasedEnvSlotsFor(ValueVertex *env, int32_t slot,
                                                                               ValueVertex *newValue)
{
    ClearedEnvSlotKeys cleared(chunk_);
    if (env == nullptr || slot < 0) {
        OnSideEffect();
        return cleared;
    }

    // Lexical-env bytecodes write user variable slots. The parent/global-env
    // fields are tracked as constants and do not invalidate cached user slots.
    if (IsEnvConstantFieldOffset(slot)) {
        return cleared;
    }

    UpdateEnvSlotAliasMode(env);
    if (envSlotAliasMode_ != EnvSlotAliasMode::MAY_ALIAS) {
        return cleared;
    }

    for (auto it = loadedEnvSlots_.begin(); it != loadedEnvSlots_.end();) {
        const EnvSlotKey &key = it->first;
        if (key.GetSlot() == slot && key.GetEnv() != env && EnvMayAlias(key.GetEnv(), env) && it->second != newValue) {
            cleared.emplace_back(key);
            it = loadedEnvSlots_.erase(it);
            continue;
        }
        ++it;
    }
    return cleared;
}

void CompileInfoFacts::ClearEnvSlotsFor(ValueVertex *env)
{
    if (env == nullptr) {
        return;
    }
    bool erased = false;
    for (auto it = loadedEnvSlots_.begin(); it != loadedEnvSlots_.end();) {
        if (it->first.GetEnv() == env) {
            it = loadedEnvSlots_.erase(it);
            erased = true;
            continue;
        }
        ++it;
    }
    if (erased) {
        RecomputeEnvSlotAliasMode();
    }
}

void CompileInfoFacts::AddExpression(uint32_t hash, ValueVertex *node, const ExpressionInputs &inputs,
                                     const ExpressionOptions &options, bool needsEpochCheck)
{
    ASSERT(node != nullptr);
    uint32_t epoch = needsEpochCheck ? effectEpoch_ : EFFECT_EPOCH_FOR_PURE_INSTRUCTIONS;
    if (epoch == EFFECT_EPOCH_OVERFLOW) {
        return;
    }
    availableExpressions_.emplace(hash, AvailableExpression {
                                            node,
                                            node->GetOpcode(),
                                            CopyExpressionInputs(inputs),
                                            CopyExpressionOptions(options),
                                            epoch,
                                        });
}

ValueVertex *CompileInfoFacts::FindExpression(uint32_t hash, VertexOpcode opcode, const ExpressionInputs &inputs,
                                              const ExpressionOptions &options, bool needsEpochCheck)
{
    auto it = availableExpressions_.find(hash);
    if (it == availableExpressions_.end()) {
        return nullptr;
    }
    AvailableExpression &expression = it->second;
    if (needsEpochCheck && effectEpoch_ > expression.effectEpoch) {
        availableExpressions_.erase(it);
        return nullptr;
    }
    return ExpressionMatches(expression, opcode, inputs, options, needsEpochCheck) ? expression.node : nullptr;
}

void CompileInfoFacts::ClearAvailableExpressions()
{
    availableExpressions_.clear();
}

void CompileInfoFacts::MarkPossibleSideEffect(const SideEffectDescriptor &effect)
{
    switch (effect.kind) {
        case SideEffectKind::NO_SIDE_EFFECT:
            return;
        case SideEffectKind::FIELD_WRITE:
            ClearLoadedPropertiesForKey(effect.propertyKey);
            IncrementEffectEpoch();
            return;
        case SideEffectKind::ELEMENTS_WRITE:
            ClearLoadedPropertiesForKey(PropertyKey::Elements());
            ClearLoadedPropertiesForKey(PropertyKey::Length());
            if (effect.receiver != nullptr) {
                ClearLoadedPropertiesForReceiver(effect.receiver);
            }
            IncrementEffectEpoch();
            return;
        case SideEffectKind::ENV_SLOT_WRITE:
            ClearAliasedEnvSlotsFor(effect.env, effect.envSlot, effect.envSlotValue);
            IncrementEffectEpoch();
            return;
        case SideEffectKind::MAP_TRANSITION:
            MarkHClassesStaleForTransition(effect.receiver);
            ClearLoadedProperties();
            IncrementEffectEpoch();
            return;
        case SideEffectKind::UNKNOWN_CALL:
            OnSideEffect();
            return;
        case SideEffectKind::SAFE_CALL:
            IncrementEffectEpoch();
            return;
    }
}

void CompileInfoFacts::MarkAllFreshUnstableHClassesStale()
{
    if (!freshUnstableHClassesRequireInvalidation_) {
        return;
    }
    for (auto &entry : nodeInfos_) {
        entry.second.MarkUnstableHClassesStale();
    }
    freshUnstableHClassesRequireInvalidation_ = false;
}

void CompileInfoFacts::MarkHClassesStaleForTransition(ValueVertex *receiver)
{
    if (!freshUnstableHClassesRequireInvalidation_) {
        return;
    }
    const NodeInfo *receiverInfo = TryGetInfoFor(receiver);
    JSHClass *oldHClass = receiverInfo == nullptr ? nullptr : receiverInfo->GetFreshKnownHClass();
    if (oldHClass == nullptr) {
        MarkAllFreshUnstableHClassesStale();
        return;
    }
    bool hasRemainingFreshUnstableHClasses = false;
    for (auto &entry : nodeInfos_) {
        NodeInfo &info = entry.second;
        if (!info.HasFreshPossibleHClasses() || !info.HasUnstablePossibleHClass()) {
            continue;
        }
        if (info.ContainsPossibleHClass(oldHClass)) {
            info.MarkUnstableHClassesStale();
            continue;
        }
        hasRemainingFreshUnstableHClasses = true;
    }
    freshUnstableHClassesRequireInvalidation_ = hasRemainingFreshUnstableHClasses;
}

void CompileInfoFacts::MarkHClassesStaleForElementsKindTransition(
    const NodeInfo::PossibleHClasses &sourceHClasses)
{
    if (sourceHClasses.empty()) {
        return;
    }
    for (auto &entry : nodeInfos_) {
        entry.second.MarkHClassesStaleForElementsKindTransition(sourceHClasses);
    }
    RecomputeFreshUnstableHClassesRequireInvalidation();
}

void CompileInfoFacts::RecomputeFreshUnstableHClassesRequireInvalidation()
{
    freshUnstableHClassesRequireInvalidation_ = std::any_of(
        nodeInfos_.begin(), nodeInfos_.end(), [](const auto &entry) {
            const NodeInfo &info = entry.second;
            return info.HasFreshPossibleHClasses() && info.HasUnstablePossibleHClass();
        });
}

void CompileInfoFacts::ClearAll()
{
    nodeInfos_.clear();
    loadedProperties_.clear();
    loadedConstantProperties_.clear();
    loadedEnvSlots_.clear();
    loadedEnvConstants_.clear();
    availableExpressions_.clear();
    envSlotAliasMode_ = EnvSlotAliasMode::NONE;
    freshUnstableHClassesRequireInvalidation_ = false;
}

void CompileInfoFacts::OnSideEffect()
{
    MarkAllFreshUnstableHClassesStale();
    ClearLoadedProperties();
    loadedEnvSlots_.clear();
    envSlotAliasMode_ = EnvSlotAliasMode::NONE;
    IncrementEffectEpoch();
}

void CompileInfoFacts::IncrementEffectEpoch()
{
    if (effectEpoch_ < EFFECT_EPOCH_OVERFLOW) {
        ++effectEpoch_;
    }
}

NodeInfo::NodeType CompileInfoFacts::GetStaticNodeType(ValueVertex *node) const
{
    if (node == nullptr) {
        return NodeInfo::NodeType::UNKNOWN;
    }
    if (auto *constant = node->TryCast<TaggedConstantVertex>()) {
        return NodeTypeFromJSTaggedValue(JSTaggedValue(constant->GetValue()));
    }
    if (auto *constant = node->TryCast<HeapConstantVertex>()) {
        return static_cast<NodeInfo::NodeType>(constant->GetStaticNodeType());
    }
    if (node->Is<Int32ConstantVertex>() || node->IsAnyInt32()) {
        return NodeInfo::NodeType::INT;
    }
    if (node->Is<Float64ConstantVertex>() || node->IsAnyFloat64()) {
        return NodeInfo::NodeType::DOUBLE;
    }
    return NodeInfo::NodeType::UNKNOWN;
}

void CompileInfoFacts::AdvanceEpochAfterMerge(uint32_t otherEpoch)
{
    uint32_t mergedEpoch = std::max(effectEpoch_, otherEpoch);
    if (mergedEpoch < EFFECT_EPOCH_OVERFLOW) {
        effectEpoch_ = mergedEpoch + 1;
        return;
    }
    effectEpoch_ = EFFECT_EPOCH_OVERFLOW;
}

CompileInfoFacts::ExpressionInputs *CompileInfoFacts::CopyExpressionInputs(const ExpressionInputs &inputs)
{
    auto *copy = chunk_->New<ExpressionInputs>(chunk_);
    copy->insert(copy->end(), inputs.begin(), inputs.end());
    return copy;
}

CompileInfoFacts::ExpressionOptions *CompileInfoFacts::CopyExpressionOptions(const ExpressionOptions &options)
{
    auto *copy = chunk_->New<ExpressionOptions>(chunk_);
    copy->insert(copy->end(), options.begin(), options.end());
    return copy;
}

bool CompileInfoFacts::ExpressionMatches(const AvailableExpression &expression, VertexOpcode opcode,
                                         const ExpressionInputs &inputs, const ExpressionOptions &options,
                                         bool needsEpochCheck) const
{
    if (expression.opcode != opcode || (needsEpochCheck && effectEpoch_ > expression.effectEpoch)) {
        return false;
    }
    if (expression.inputs->size() != inputs.size() || expression.options->size() != options.size()) {
        return false;
    }
    return std::equal(expression.inputs->begin(), expression.inputs->end(), inputs.begin()) &&
           std::equal(expression.options->begin(), expression.options->end(), options.begin());
}

ValueVertex *CompileInfoFacts::LookupLoadedProperty(const LoadedPropertyMap &map, const LoadedPropertyKey &key) const
{
    auto it = map.find(key);
    return it == map.end() ? nullptr : it->second;
}

void CompileInfoFacts::RecordLoadedProperty(LoadedPropertyMap &map, const LoadedPropertyKey &key, ValueVertex *value)
{
    ASSERT(key.GetReceiver() != nullptr);
    ASSERT(value != nullptr);
    map[key] = value;
}

void CompileInfoFacts::MergeLoadedProperties(LoadedPropertyMap &target, const LoadedPropertyMap &other)
{
    for (auto it = target.begin(); it != target.end();) {
        auto otherIt = other.find(it->first);
        if (otherIt == other.end() || otherIt->second != it->second) {
            it = target.erase(it);
        } else {
            ++it;
        }
    }
}

void CompileInfoFacts::MergeEnvSlots(LoadedEnvSlots &target, const LoadedEnvSlots &other)
{
    for (auto it = target.begin(); it != target.end();) {
        auto otherIt = other.find(it->first);
        if (otherIt == other.end() || otherIt->second != it->second) {
            it = target.erase(it);
        } else {
            ++it;
        }
    }
}

void CompileInfoFacts::MergeAvailableExpressions(const CompileInfoFacts &other)
{
    for (auto it = availableExpressions_.begin(); it != availableExpressions_.end();) {
        auto otherIt = other.availableExpressions_.find(it->first);
        if (otherIt == other.availableExpressions_.end()) {
            it = availableExpressions_.erase(it);
            continue;
        }

        const AvailableExpression &expression = it->second;
        const AvailableExpression &otherExpression = otherIt->second;
        if (expression.node == otherExpression.node && expression.effectEpoch == otherExpression.effectEpoch &&
            ExpressionMatches(expression, otherExpression.opcode, *otherExpression.inputs, *otherExpression.options,
                              false) &&
            expression.effectEpoch >= effectEpoch_) {
            ++it;
            continue;
        }

        it = availableExpressions_.erase(it);
    }
}

void CompileInfoFacts::CopyLoadedProperties(LoadedPropertyMap &target, const LoadedPropertyMap &source) const
{
    ASSERT(target.empty());
    target.insert(source.begin(), source.end());
}

void CompileInfoFacts::CopyEnvSlots(LoadedEnvSlots &target, const LoadedEnvSlots &source) const
{
    ASSERT(target.empty());
    target.insert(source.begin(), source.end());
}

void CompileInfoFacts::UpdateEnvSlotAliasMode(ValueVertex *env)
{
    if (envSlotAliasMode_ == EnvSlotAliasMode::MAY_ALIAS) {
        return;
    }
    EnvSlotAliasMode mode = EnvSlotAliasMode::MAY_ALIAS;
    if (env != nullptr && env->Is<InitialValueVertex>()) {
        mode = EnvSlotAliasMode::CURRENT_ENV_ONLY;
    } else if (env != nullptr && (env->Is<TaggedConstantVertex>() || env->Is<HeapConstantVertex>())) {
        mode = EnvSlotAliasMode::CONSTANT_ENV_ONLY;
    }
    envSlotAliasMode_ = MergeEnvSlotAliasMode(envSlotAliasMode_, mode);
}

void CompileInfoFacts::RecomputeEnvSlotAliasMode()
{
    envSlotAliasMode_ = EnvSlotAliasMode::NONE;
    for (const auto &entry : loadedEnvSlots_) {
        UpdateEnvSlotAliasMode(entry.first.GetEnv());
    }
}

bool CompileInfoFacts::EnvMayAlias(ValueVertex *lhs, ValueVertex *rhs) const
{
    return lhs != rhs;
}

EnvSlotAliasMode CompileInfoFacts::MergeEnvSlotAliasMode(EnvSlotAliasMode lhs, EnvSlotAliasMode rhs)
{
    if (lhs == rhs) {
        return lhs;
    }
    if (lhs == EnvSlotAliasMode::NONE) {
        return rhs;
    }
    if (rhs == EnvSlotAliasMode::NONE) {
        return lhs;
    }
    return EnvSlotAliasMode::MAY_ALIAS;
}

}  // namespace panda::ecmascript::arksteed
