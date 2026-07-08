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
    COND_EQUAL,
    COND_NOT_EQUAL,
    COND_LESS_THAN,
    COND_LESS_THAN_OR_EQUAL,
    COND_GREATER_THAN,
    COND_GREATER_THAN_OR_EQUAL,
    COND_ABOVE,
    COND_BELOW,
    COND_ABOVE_OR_EQUAL,
    COND_BELOW_OR_EQUAL,
    COND_ZERO,
    COND_NOT_ZERO,
    COND_OVERFLOW,
    COND_NOT_OVERFLOW,
    COND_PARITY,
    COND_NOT_PARITY
};

constexpr Condition NegateCondition(Condition cond)
{
    switch (cond) {
        case Condition::COND_EQUAL:
            return Condition::COND_NOT_EQUAL;
        case Condition::COND_NOT_EQUAL:
            return Condition::COND_EQUAL;
        case Condition::COND_LESS_THAN:
            return Condition::COND_GREATER_THAN_OR_EQUAL;
        case Condition::COND_LESS_THAN_OR_EQUAL:
            return Condition::COND_GREATER_THAN;
        case Condition::COND_GREATER_THAN:
            return Condition::COND_LESS_THAN_OR_EQUAL;
        case Condition::COND_GREATER_THAN_OR_EQUAL:
            return Condition::COND_LESS_THAN;
        case Condition::COND_ABOVE:
            return Condition::COND_BELOW_OR_EQUAL;
        case Condition::COND_BELOW:
            return Condition::COND_ABOVE_OR_EQUAL;
        case Condition::COND_ABOVE_OR_EQUAL:
            return Condition::COND_BELOW;
        case Condition::COND_BELOW_OR_EQUAL:
            return Condition::COND_ABOVE;
        case Condition::COND_ZERO:
            return Condition::COND_NOT_ZERO;
        case Condition::COND_NOT_ZERO:
            return Condition::COND_ZERO;
        case Condition::COND_OVERFLOW:
            return Condition::COND_NOT_OVERFLOW;
        case Condition::COND_NOT_OVERFLOW:
            return Condition::COND_OVERFLOW;
        case Condition::COND_PARITY:
            return Condition::COND_NOT_PARITY;
        case Condition::COND_NOT_PARITY:
            return Condition::COND_PARITY;
        default:
            UNREACHABLE();
    }
}

constexpr const char *ConditionCodeName(Condition cond)
{
    switch (cond) {
        case Condition::COND_EQUAL:
            return "equal";
        case Condition::COND_NOT_EQUAL:
            return "not_equal";
        case Condition::COND_LESS_THAN:
            return "less_than";
        case Condition::COND_LESS_THAN_OR_EQUAL:
            return "less_than_or_equal";
        case Condition::COND_GREATER_THAN:
            return "greater_than";
        case Condition::COND_GREATER_THAN_OR_EQUAL:
            return "greater_than_or_equal";
        case Condition::COND_ABOVE:
            return "above";
        case Condition::COND_BELOW:
            return "below";
        case Condition::COND_ABOVE_OR_EQUAL:
            return "above_or_equal";
        case Condition::COND_BELOW_OR_EQUAL:
            return "below_or_equal";
        case Condition::COND_ZERO:
            return "zero";
        case Condition::COND_NOT_ZERO:
            return "not_zero";
        case Condition::COND_OVERFLOW:
            return "overflow";
        case Condition::COND_NOT_OVERFLOW:
            return "not_overflow";
        case Condition::COND_PARITY:
            return "parity";
        case Condition::COND_NOT_PARITY:
            return "not_parity";
        default:
            return "<unknown>";
    }
}

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_CONDITION_CODE_H
