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

#ifndef ECMASCRIPT_ARKSTEED_CONDITION_CODE_H
#define ECMASCRIPT_ARKSTEED_CONDITION_CODE_H

#include <cstdint>
#include "libpandabase/macros.h"

namespace panda::ecmascript::arksteed {

enum class Condition : uint8_t {
    EQUAL,
    NOT_EQUAL,
    LESS_THAN,
    LESS_THAN_OR_EQUAL,
    GREATER_THAN,
    GREATER_THAN_OR_EQUAL,
    ABOVE,
    BELOW,
    ABOVE_OR_EQUAL,
    BELOW_OR_EQUAL,
    ZERO,
    NOT_ZERO,
    OVERFLOWED,
    NOT_OVERFLOW,
    PARITY,
    NOT_PARITY
};

// JS-level comparison operators. These are bytecode/front-end semantics, not
// machine condition codes: strict equality is type-aware and cannot be
// represented as a single Condition.
enum class JSCondition : uint8_t {
    EQUAL,
    NOT_EQUAL,
    LESS_THAN,
    LESS_THAN_OR_EQUAL,
    GREATER_THAN,
    GREATER_THAN_OR_EQUAL,
    STRICT_EQUAL,
    STRICT_NOT_EQUAL,
};

constexpr Condition NegateCondition(Condition cond)
{
    switch (cond) {
        case Condition::EQUAL:
            return Condition::NOT_EQUAL;
        case Condition::NOT_EQUAL:
            return Condition::EQUAL;
        case Condition::LESS_THAN:
            return Condition::GREATER_THAN_OR_EQUAL;
        case Condition::LESS_THAN_OR_EQUAL:
            return Condition::GREATER_THAN;
        case Condition::GREATER_THAN:
            return Condition::LESS_THAN_OR_EQUAL;
        case Condition::GREATER_THAN_OR_EQUAL:
            return Condition::LESS_THAN;
        case Condition::ABOVE:
            return Condition::BELOW_OR_EQUAL;
        case Condition::BELOW:
            return Condition::ABOVE_OR_EQUAL;
        case Condition::ABOVE_OR_EQUAL:
            return Condition::BELOW;
        case Condition::BELOW_OR_EQUAL:
            return Condition::ABOVE;
        case Condition::ZERO:
            return Condition::NOT_ZERO;
        case Condition::NOT_ZERO:
            return Condition::ZERO;
        case Condition::OVERFLOWED:
            return Condition::NOT_OVERFLOW;
        case Condition::NOT_OVERFLOW:
            return Condition::OVERFLOWED;
        case Condition::PARITY:
            return Condition::NOT_PARITY;
        case Condition::NOT_PARITY:
            return Condition::PARITY;
        default:
            UNREACHABLE();
    }
}

constexpr const char *ConditionName(Condition cond)
{
    switch (cond) {
        case Condition::EQUAL:
            return "equal";
        case Condition::NOT_EQUAL:
            return "not_equal";
        case Condition::LESS_THAN:
            return "less_than";
        case Condition::LESS_THAN_OR_EQUAL:
            return "less_than_or_equal";
        case Condition::GREATER_THAN:
            return "greater_than";
        case Condition::GREATER_THAN_OR_EQUAL:
            return "greater_than_or_equal";
        case Condition::ABOVE:
            return "above";
        case Condition::BELOW:
            return "below";
        case Condition::ABOVE_OR_EQUAL:
            return "above_or_equal";
        case Condition::BELOW_OR_EQUAL:
            return "below_or_equal";
        case Condition::ZERO:
            return "zero";
        case Condition::NOT_ZERO:
            return "not_zero";
        case Condition::OVERFLOWED:
            return "overflow";
        case Condition::NOT_OVERFLOW:
            return "not_overflow";
        case Condition::PARITY:
            return "parity";
        case Condition::NOT_PARITY:
            return "not_parity";
        default:
            return "<unknown>";
    }
}

constexpr const char *JSConditionName(JSCondition kind)
{
    switch (kind) {
        case JSCondition::EQUAL:
            return "EQUAL";
        case JSCondition::NOT_EQUAL:
            return "NOT_EQUAL";
        case JSCondition::LESS_THAN:
            return "LESS_THAN";
        case JSCondition::LESS_THAN_OR_EQUAL:
            return "LESS_THAN_OR_EQUAL";
        case JSCondition::GREATER_THAN:
            return "GREATER_THAN";
        case JSCondition::GREATER_THAN_OR_EQUAL:
            return "GREATER_THAN_OR_EQUAL";
        case JSCondition::STRICT_EQUAL:
            return "STRICT_EQUAL";
        case JSCondition::STRICT_NOT_EQUAL:
            return "STRICT_NOT_EQUAL";
    }
    return "UNKNOWN";
}

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_CONDITION_CODE_H
