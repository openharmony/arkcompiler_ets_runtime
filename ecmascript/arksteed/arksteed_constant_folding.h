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

#ifndef ECMASCRIPT_ARKSTEED_CONSTANT_FOLDING_H
#define ECMASCRIPT_ARKSTEED_CONSTANT_FOLDING_H

#include <cstdint>

#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript::arksteed {

class ArkSteedGraphBuilder;
class ValueVertex;

enum class BinaryFoldOp : uint8_t {
    ADD,
    SUB,
    MUL,
    DIV,
    SHL,
    SHR,
    ASHR,
    AND,
    OR,
    XOR,
    EQ,
    NOT_EQ,
    STRICT_EQ,
    STRICT_NOT_EQ,
    LESS,
    LESS_EQ,
    GREATER,
    GREATER_EQ,
};

enum class UnaryFoldOp : uint8_t { NEG, INC, DEC, NOT, TO_NUMBER, TO_NUMERIC };

bool TryConstFoldBinary(ArkSteedGraphBuilder *builder, ValueVertex *lhs, ValueVertex *rhs, BinaryFoldOp op);
bool TryConstFoldUnary(ArkSteedGraphBuilder *builder, ValueVertex *input, UnaryFoldOp op);
bool TryFoldToBooleanConstant(ValueVertex *vertex, bool *result);

bool TryFoldBinaryConstant(ValueVertex *lhs, ValueVertex *rhs, BinaryFoldOp op, JSTaggedValue *result);
bool TryFoldUnaryConstant(ValueVertex *input, UnaryFoldOp op, JSTaggedValue *result);

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_CONSTANT_FOLDING_H
