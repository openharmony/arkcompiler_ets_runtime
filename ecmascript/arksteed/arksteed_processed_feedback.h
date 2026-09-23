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

#ifndef ECMASCRIPT_ARKSTEED_PROCESSED_FEEDBACK_H
#define ECMASCRIPT_ARKSTEED_PROCESSED_FEEDBACK_H

#include <array>

#include "ecmascript/arksteed/arksteed_pgo_access_info.h"
#include "ecmascript/js_tagged_value_wrapper.h"

namespace panda::ecmascript::arksteed {

enum class ProcessedFeedbackKind : uint8_t {
    INSUFFICIENT,
    NAMED_ACCESS,
    VALUE_ACCESS,
    ELEMENT_ACCESS,
    GLOBAL_ACCESS,
};

enum class ValueAccessFeedbackKind : uint8_t {
    INSUFFICIENT,
    NAMED,
    ELEMENT,
};

enum class ArkSteedOperationHint : uint8_t {
    NONE,
    INT,
    NUMBER,
    STRING,
    NUMBER_OR_STRING,
    ANY,
};

struct ProcessedFeedbackBase {
    ProcessedFeedbackKind kind {ProcessedFeedbackKind::INSUFFICIENT};
    AccessFeedbackSource source {};

    bool IsInsufficient() const
    {
        return kind == ProcessedFeedbackKind::INSUFFICIENT;
    }
};

struct OperationFeedback {
    uint32_t slotId {0};
    ArkSteedOperationHint hint {ArkSteedOperationHint::NONE};
    uint32_t rawTypeBits {0};
    uint32_t trueWeight {0};
    uint32_t falseWeight {0};

    bool IsInsufficient() const
    {
        return hint == ArkSteedOperationHint::NONE;
    }
};

struct NamedAccessCaseFeedback {
    ArkSteedHClassRef expectedHClass {};
    ArkSteedHandlerRef handler {};
};

struct NamedAccessFeedback {
    ProcessedFeedbackBase base {};
    ArkSteedNameRef name {};
    std::array<NamedAccessCaseFeedback, MAX_NAMED_IC_POLY_CASES> cases {};
    uint32_t caseCount {0};
};

struct ValueAccessFeedback {
    ProcessedFeedbackBase base {};
    ValueAccessFeedbackKind kind {ValueAccessFeedbackKind::INSUFFICIENT};
    ArkSteedNameRef key {};
    std::array<NamedAccessCaseFeedback, MAX_NAMED_IC_POLY_CASES> cases {};
    uint32_t caseCount {0};
};

struct ElementAccessCaseFeedback {
    ArkSteedHClassRef expectedHClass {};
    ArkSteedHandlerRef handler {};
};

struct ElementAccessFeedback {
    ProcessedFeedbackBase base {};
    std::array<ElementAccessCaseFeedback, MAX_ELEMENT_IC_POLY_CASES> cases {};
    uint32_t caseCount {0};
};

// Value cell of a global-record or global-object binding cached by a global IC slot.
struct GlobalAccessFeedback {
    ProcessedFeedbackBase base {};
    ArkSteedObjectRef box {};
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_PROCESSED_FEEDBACK_H
