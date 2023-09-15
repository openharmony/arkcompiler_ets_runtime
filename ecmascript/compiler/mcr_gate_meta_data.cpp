/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/mcr_gate_meta_data.h"
#include "ecmascript/compiler/gate_meta_data_builder.h"

namespace panda::ecmascript::kungfu {

TypedBinOp TypedBinaryMetaData::GetRevCompareOp(TypedBinOp op)
{
    switch (op) {
        case TypedBinOp::TYPED_LESS:
            return TypedBinOp::TYPED_GREATEREQ;
        case TypedBinOp::TYPED_LESSEQ:
            return TypedBinOp::TYPED_GREATER;
        case TypedBinOp::TYPED_GREATER:
            return TypedBinOp::TYPED_LESSEQ;
        case TypedBinOp::TYPED_GREATEREQ:
            return TypedBinOp::TYPED_LESS;
        case TypedBinOp::TYPED_EQ:
            return TypedBinOp::TYPED_NOTEQ;
        case TypedBinOp::TYPED_NOTEQ:
            return TypedBinOp::TYPED_EQ;
        default:
            UNREACHABLE();
            return op;
    }
}

TypedBinOp TypedBinaryMetaData::GetSwapCompareOp(TypedBinOp op)
{
    switch (op) {
        case TypedBinOp::TYPED_LESS:
            return TypedBinOp::TYPED_GREATER;
        case TypedBinOp::TYPED_LESSEQ:
            return TypedBinOp::TYPED_GREATEREQ;
        case TypedBinOp::TYPED_GREATER:
            return TypedBinOp::TYPED_LESS;
        case TypedBinOp::TYPED_GREATEREQ:
            return TypedBinOp::TYPED_LESSEQ;
        case TypedBinOp::TYPED_EQ:
            return TypedBinOp::TYPED_EQ;
        case TypedBinOp::TYPED_NOTEQ:
            return TypedBinOp::TYPED_NOTEQ;
        default:
            UNREACHABLE();
            return op;
    }
}

bool GateMetaData::IsTypedOperator() const
{
    return (opcode_ == OpCode::TYPED_BINARY_OP) || (opcode_ == OpCode::TYPE_CONVERT) ||
        (opcode_ == OpCode::TYPED_UNARY_OP);
}

bool GateMetaData::IsCheckWithTwoIns() const
{
    return (opcode_ == OpCode::OBJECT_TYPE_CHECK) ||
           (opcode_ == OpCode::INDEX_CHECK) ||
           (opcode_ == OpCode::TYPED_CALL_CHECK);
}

bool GateMetaData::IsCheckWithOneIn() const
{
    return (opcode_ == OpCode::PRIMITIVE_TYPE_CHECK) ||
           (opcode_ == OpCode::HEAP_OBJECT_CHECK) ||
           (opcode_ == OpCode::STABLE_ARRAY_CHECK) ||
           (opcode_ == OpCode::TYPED_ARRAY_CHECK);
}

}