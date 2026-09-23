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

#ifndef ECMASCRIPT_ARKSTEED_PGO_ACCESS_INFO_H
#define ECMASCRIPT_ARKSTEED_PGO_ACCESS_INFO_H

#include <array>
#include <vector>

#include "ecmascript/arksteed/arksteed_heap_ref.h"
#include "ecmascript/elements.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/on_heap.h"

namespace panda::ecmascript::arksteed {

static constexpr uint32_t MAX_IC_POLY_CASES = 4;
static constexpr uint32_t MAX_NAMED_IC_POLY_CASES = MAX_IC_POLY_CASES;
static constexpr uint32_t MAX_ELEMENT_IC_POLY_CASES = MAX_IC_POLY_CASES;

enum class AccessMode : uint8_t {
    NAMED_LOAD,
    NAMED_STORE,
};

enum class AccessKind : uint8_t {
    UNSUPPORTED,
    NOT_FOUND,
    DATA_FIELD,
    FAST_DATA_CONSTANT,
    DICTIONARY_DATA_FIELD,
    DICTIONARY_PROTO_DATA_CONSTANT,
    FAST_ACCESSOR_CONSTANT,
    DICTIONARY_PROTO_ACCESSOR_CONSTANT,
    MODULE_EXPORT,
    STRING_LENGTH,
    STRING_WRAPPER_LENGTH,
    TYPED_ARRAY_LENGTH,
    TRANSITION,

    FIELD = DATA_FIELD,
    PROTOTYPE_FIELD = DATA_FIELD,
    NON_EXIST = NOT_FOUND,
    ACCESSOR = FAST_ACCESSOR_CONSTANT,
};

enum class AccessDependencyKind : uint8_t {
    NONE,
    HCLASS,
    PROTOTYPE_CHAIN,
    NOT_PROTOTYPE,
};

enum class AccessFeedbackSlotKind : uint8_t {
    UNKNOWN,
    NAMED_LOAD,
    NAMED_STORE,
    VALUE_LOAD,
    ELEMENT_STORE,
};

enum class ValueLoadAccessKind : uint8_t {
    UNSUPPORTED,
    NAMED,
    ELEMENT,
};

enum class ElementLoadKind : uint8_t {
    UNSUPPORTED,
    NORMAL,
    STRING,
    TYPED_ARRAY,
};

enum class ElementStoreKind : uint8_t {
    UNSUPPORTED,
    JS_ARRAY,
    TYPED_ARRAY,
};

enum class AccessFieldRepresentation : uint8_t {
    UNKNOWN,
    TAGGED,
    INT32,
    DOUBLE,
};

enum class AccessFieldType : uint8_t {
    UNKNOWN,
    ANY,
    TAGGED,
    NUMBER,
    STRING,
    BOOLEAN,
    OBJECT,
};

enum class AccessFieldStorage : uint8_t {
    UNKNOWN,
    IN_OBJECT,
    PROPERTIES_ARRAY,
};

struct AccessFeedbackSource {
    uint32_t slotId {0};
    bool isPoly {false};
    AccessFeedbackSlotKind slotKind {AccessFeedbackSlotKind::UNKNOWN};
};

struct AccessDependencyInfo {
    AccessDependencyKind hclassDependency {AccessDependencyKind::NONE};
    AccessDependencyKind protoChainDependency {AccessDependencyKind::NONE};
    AccessDependencyKind notPrototypeDependency {AccessDependencyKind::NONE};
    bool canAssumeStableHClass {false};
    bool canAssumeStableProtoChain {false};
    bool canAssumeNotPrototype {false};
};

struct AccessGuardInfo {
    ArkSteedHClassRef expectedHClass {};
    ArkSteedObjectRef holder {};
    ArkSteedProtoCellRef protoCell {};
    bool hasHClassGuard {false};
    bool hasHolder {false};
    bool holderIsReceiver {true};
    bool hasProtoCellGuard {false};
    bool hasNotFoundProtoCellGuard {false};
};

struct PropertyAccessInfo {
    AccessMode mode {AccessMode::NAMED_LOAD};
    AccessKind kind {AccessKind::UNSUPPORTED};
    AccessGuardInfo guards {};
    AccessDependencyInfo dependencies {};
    ArkSteedHClassRef expectedHClass {};
    ArkSteedObjectRef holder {};
    ArkSteedProtoCellRef protoCell {};
    uint64_t handlerInfo {0};
    uint32_t fieldIndex {0};
    int32_t fieldOffset {0};
    AccessFieldStorage fieldStorage {AccessFieldStorage::UNKNOWN};
    int32_t dictionaryIndex {0};
    AccessFieldRepresentation fieldRepresentation {AccessFieldRepresentation::UNKNOWN};
    AccessFieldType fieldType {AccessFieldType::UNKNOWN};
    ArkSteedNameRef name {};
    ArkSteedObjectRef constant {};
    ArkSteedHClassRef transitionHClass {};
    ArkSteedHClassRef holderHClass {};
    ArkSteedHClassRef fieldOwnerHClass {};
    ArkSteedHClassRef fieldHClass {};
    uint32_t holderDepth {0};
    bool holderIsReceiver {true};
    bool hasProtoCell {false};
    bool hasNotFoundProtoCellGuard {false};
    bool hasTransitionHClass {false};
    bool hasFieldHClass {false};
    bool isSharedStore {false};

    bool IsInvalid() const
    {
        return kind == AccessKind::UNSUPPORTED;
    }

    bool IsNotFound() const
    {
        return kind == AccessKind::NOT_FOUND;
    }

    bool IsDataField() const
    {
        return kind == AccessKind::DATA_FIELD;
    }

    bool IsFastDataConstant() const
    {
        return kind == AccessKind::FAST_DATA_CONSTANT;
    }

    bool IsFastAccessorConstant() const
    {
        return kind == AccessKind::FAST_ACCESSOR_CONSTANT;
    }

    bool IsDictionaryDataField() const
    {
        return kind == AccessKind::DICTIONARY_DATA_FIELD;
    }

    bool IsDictionaryProtoDataConstant() const
    {
        return kind == AccessKind::DICTIONARY_PROTO_DATA_CONSTANT;
    }

    bool IsDictionaryProtoAccessorConstant() const
    {
        return kind == AccessKind::DICTIONARY_PROTO_ACCESSOR_CONSTANT;
    }

    bool IsModuleExport() const
    {
        return kind == AccessKind::MODULE_EXPORT;
    }

    bool IsStringLength() const
    {
        return kind == AccessKind::STRING_LENGTH;
    }

    bool IsStringWrapperLength() const
    {
        return kind == AccessKind::STRING_WRAPPER_LENGTH;
    }

    bool IsTypedArrayLength() const
    {
        return kind == AccessKind::TYPED_ARRAY_LENGTH;
    }

    bool HasHolder() const
    {
        return HasHolderHClass();
    }

    bool HasHolderHClass() const
    {
        return !holderIsReceiver && holderHClass.IsSafeForCompile();
    }

    bool HasTransitionHClass() const
    {
        return hasTransitionHClass;
    }
};

using NamedStoreAccessInfo = PropertyAccessInfo;
using NamedLoadAccessInfo = PropertyAccessInfo;

struct PropertyAccessSet {
    AccessFeedbackSource feedback {};
    std::array<PropertyAccessInfo, MAX_NAMED_IC_POLY_CASES> cases {};
    uint32_t caseCount {0};
    uint32_t slotId {0};
    bool isPoly {false};
};

using NamedStoreAccessSet = PropertyAccessSet;
using NamedLoadAccessSet = PropertyAccessSet;

struct ElementLoadAccessInfo {
    ArkSteedHClassRef expectedHClass {};
    uint64_t handlerInfo {0};
    ElementLoadKind kind {ElementLoadKind::UNSUPPORTED};
};

struct ValueLoadAccessSet {
    ValueLoadAccessKind kind {ValueLoadAccessKind::UNSUPPORTED};
    AccessFeedbackSource feedback {};
    ArkSteedNameRef key {};
    NamedLoadAccessSet named {};
    std::array<ElementLoadAccessInfo, MAX_NAMED_IC_POLY_CASES> elements {};
    uint32_t elementCount {0};
};

struct ElementStoreAccessCase {
    ArkSteedHClassRef expectedHClass {};
    ElementsKind elementsKind {ElementsKind::NONE};
};

struct ElementStoreTransitionGroup {
    ArkSteedHClassRef targetHClass {};
    std::array<ArkSteedHClassRef, MAX_ELEMENT_IC_POLY_CASES> transitionSources {};
    uint32_t sourceCount {0};
    ElementsKind targetElementsKind {ElementsKind::NONE};
    bool useExactHClassTransition {false};
};

struct ElementStoreAccessInfo {
    AccessFeedbackSource feedback {};
    std::array<ElementStoreAccessCase, MAX_ELEMENT_IC_POLY_CASES> cases {};
    uint32_t caseCount {0};
    std::array<ElementStoreTransitionGroup, MAX_ELEMENT_IC_POLY_CASES> transitionGroups {};
    uint32_t transitionGroupCount {0};
    ElementStoreKind kind {ElementStoreKind::UNSUPPORTED};
    JSType typedArrayType {JSType::INVALID};
    OnHeapMode onHeapMode {OnHeapMode::NONE};

    bool IsJSArray() const
    {
        return kind == ElementStoreKind::JS_ARRAY;
    }

    bool IsTypedArray() const
    {
        return kind == ElementStoreKind::TYPED_ARRAY;
    }
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_PGO_ACCESS_INFO_H
