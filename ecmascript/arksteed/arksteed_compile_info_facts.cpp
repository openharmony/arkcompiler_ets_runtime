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

NodeInfo::PossibleHClasses NodeInfo::GetPossibleHClasses() const
{
    PossibleHClasses result;
    result.reserve(possibleHClasses_.size());
    for (const auto &info : possibleHClasses_) {
        result.push_back(info.hclass);
    }
    return result;
}

void NodeInfo::RecordHClass(JSHClass *hclass, bool isStable)
{
    possibleHClasses_.clear();
    AddPossibleHClass(hclass, isStable);
    IntersectType(NodeInfo::NodeType::JS_RECEIVER);
}

void NodeInfo::RecordPossibleHClasses(const PossibleHClasses &hclasses, bool isStable)
{
    possibleHClasses_.clear();
    for (JSHClass *hclass : hclasses) {
        AddPossibleHClass(hclass, isStable);
    }
    if (!possibleHClasses_.empty()) {
        IntersectType(NodeInfo::NodeType::JS_RECEIVER);
    }
}

void NodeInfo::ClearPossibleHClasses()
{
    possibleHClasses_.clear();
}

void NodeInfo::ClearUnstable()
{
    possibleHClasses_.erase(std::remove_if(possibleHClasses_.begin(), possibleHClasses_.end(),
                                           [](const PossibleHClassInfo &info) { return !info.isStable; }),
                            possibleHClasses_.end());
}

bool NodeInfo::MergeWith(const NodeInfo &other)
{
    UnionType(other.type_);
    alternatives_.MergeWith(other.alternatives_);
    UnionPossibleHClasses(other);
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
    return copy;
}

CompileInfoFacts *CompileInfoFacts::CloneForLoopHeader() const
{
    auto *copy = chunk_->New<CompileInfoFacts>(chunk_);
    copy->nodeInfos_.insert(nodeInfos_.begin(), nodeInfos_.end());
    for (auto infoIt = copy->nodeInfos_.begin(); infoIt != copy->nodeInfos_.end();) {
        infoIt->second.ClearUnstable();
        if (infoIt->second.NoInfoAvailable()) {
            infoIt = copy->nodeInfos_.erase(infoIt);
        } else {
            ++infoIt;
        }
    }
    copy->CopyLoadedProperties(copy->loadedConstantProperties_, loadedConstantProperties_);
    copy->effectEpoch_ = effectEpoch_;
    copy->IncrementEffectEpoch();
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
    MergeAvailableExpressions(other);
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

void CompileInfoFacts::RecordHClass(ValueVertex *node, JSHClass *hclass, bool isStable)
{
    GetOrCreateInfoFor(node)->RecordHClass(hclass, isStable);
}

JSHClass *CompileInfoFacts::TryGetHClass(ValueVertex *node) const
{
    const NodeInfo *info = TryGetInfoFor(node);
    if (info == nullptr || !info->HasKnownHClass()) {
        return nullptr;
    }
    return info->GetKnownHClass();
}

void CompileInfoFacts::RecordPossibleHClasses(ValueVertex *node, const NodeInfo::PossibleHClasses &hclasses,
                                              bool isStable)
{
    GetOrCreateInfoFor(node)->RecordPossibleHClasses(hclasses, isStable);
}

void CompileInfoFacts::SetPossibleHClasses(ValueVertex *node, const ChunkVector<JSHClass *> &hclasses,
                                           NodeInfo::NodeType possibleType)
{
    NodeInfo::PossibleHClasses copy;
    copy.reserve(hclasses.size());
    copy.insert(copy.end(), hclasses.begin(), hclasses.end());
    NodeInfo *info = GetOrCreateInfoFor(node);
    info->RecordPossibleHClasses(copy, false);
    info->IntersectType(possibleType);
}

std::optional<NodeInfo::PossibleHClasses> CompileInfoFacts::TryGetPossibleHClasses(ValueVertex *node) const
{
    const NodeInfo *info = TryGetInfoFor(node);
    if (info == nullptr || !info->HasPossibleHClasses()) {
        return std::nullopt;
    }
    return info->GetPossibleHClasses();
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
        case SideEffectKind::MAP_TRANSITION:
            ClearUnstable();
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

void CompileInfoFacts::ClearUnstable()
{
    for (auto infoIt = nodeInfos_.begin(); infoIt != nodeInfos_.end();) {
        infoIt->second.ClearUnstable();
        if (infoIt->second.NoInfoAvailable()) {
            infoIt = nodeInfos_.erase(infoIt);
        } else {
            ++infoIt;
        }
    }
    ClearLoadedProperties();
}

void CompileInfoFacts::ClearAll()
{
    nodeInfos_.clear();
    loadedProperties_.clear();
    loadedConstantProperties_.clear();
    availableExpressions_.clear();
}

void CompileInfoFacts::OnSideEffect()
{
    ClearUnstable();
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
    if (auto *constant = node->TryCast<ConstantVertex>()) {
        return NodeTypeFromJSTaggedValue(constant->GetValue());
    }
    if (auto *constant = node->TryCast<TaggedConstantVertex>()) {
        return NodeTypeFromJSTaggedValue(JSTaggedValue(constant->GetValue()));
    }
    if (auto *root = node->TryCast<RootConstantVertex>()) {
        switch (root->GetIndex()) {
            case RootConstantVertex::RootIndex::UNDEFINED:
                return NodeInfo::NodeType::UNDEFINED;
            case RootConstantVertex::RootIndex::NULL_VALUE:
                return NodeInfo::NodeType::NULL_TYPE;
            case RootConstantVertex::RootIndex::TRUE_VALUE:
            case RootConstantVertex::RootIndex::FALSE_VALUE:
                return NodeInfo::NodeType::BOOLEAN;
        }
        UNREACHABLE();
    }
    if (node->Is<BooleanConstantVertex>()) {
        return NodeInfo::NodeType::BOOLEAN;
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

}  // namespace panda::ecmascript::arksteed
