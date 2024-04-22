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

std::string GateMetaData::Str(TypedBinOp op)
{
    const std::map<TypedBinOp, const char *> strMap = {
#define TYPED_BIN_OP_NAME_MAP(OP) { TypedBinOp::OP, #OP },
    TYPED_BIN_OP_LIST(TYPED_BIN_OP_NAME_MAP)
#undef TYPED_BIN_OP_NAME_MAP
    };
    if (strMap.count(op) > 0) {
        return strMap.at(op);
    }
    return "UNKNOW";
}

std::string GateMetaData::Str(TypedUnOp op)
{
    const std::map<TypedUnOp, const char *> strMap = {
#define TYPED_UN_OP_NAME_MAP(OP) { TypedUnOp::OP, #OP },
    TYPED_UN_OP_LIST(TYPED_UN_OP_NAME_MAP)
#undef TYPED_UN_OP_NAME_MAP
    };
    if (strMap.count(op) > 0) {
        return strMap.at(op);
    }
    return "UNKNOW";
}

std::string GateMetaData::Str(TypedJumpOp op)
{
    const std::map<TypedJumpOp, const char *> strMap = {
#define TYPED_JUMP_OP_NAME_MAP(OP) { TypedJumpOp::OP, #OP },
    TYPED_JUMP_OP_LIST(TYPED_JUMP_OP_NAME_MAP)
#undef TYPED_JUMP_OP_NAME_MAP
    };
    if (strMap.count(op) > 0) {
        return strMap.at(op);
    }
    return "UNKNOW";
}

std::string GateMetaData::Str(TypedLoadOp op)
{
    const std::map<TypedLoadOp, const char *> strMap = {
#define TYPED_LOAD_OP_NAME_MAP(OP) { TypedLoadOp::OP, #OP },
    TYPED_LOAD_OP_LIST(TYPED_LOAD_OP_NAME_MAP)
#undef TYPED_LOAD_OP_NAME_MAP
    };
    if (strMap.count(op) > 0) {
        return strMap.at(op);
    }
    return "UNKNOW";
}

std::string GateMetaData::Str(TypedStoreOp op)
{
    const std::map<TypedStoreOp, const char *> strMap = {
#define TYPED_STORE_OP_NAME_MAP(OP) { TypedStoreOp::OP, #OP },
    TYPED_STORE_OP_LIST(TYPED_STORE_OP_NAME_MAP)
#undef TYPED_STORE_OP_NAME_MAP
    };
    if (strMap.count(op) > 0) {
        return strMap.at(op);
    }
    return "UNKNOW";
}

std::string GateMetaData::Str(TypedCallTargetCheckOp op)
{
    const std::map<TypedCallTargetCheckOp, const char *> strMap = {
#define TYPED_CALL_TARGET_CHECK_OP_NAME_MAP(OP) { TypedCallTargetCheckOp::OP, #OP },
    TYPED_CALL_TARGET_CHECK_OP_LIST(TYPED_CALL_TARGET_CHECK_OP_NAME_MAP)
#undef TYPED_CALL_TARGET_CHECK_OP_NAME_MAP
    };
    if (strMap.count(op) > 0) {
        return strMap.at(op);
    }
    return "UNKNOW";
}

std::string GateMetaData::Str(ValueType type)
{
    const std::map<ValueType, const char *> strMap = {
#define VALUE_TYPE_NAME_MAP(TYPE) { ValueType::TYPE, #TYPE },
    VALUE_TYPE_LIST(VALUE_TYPE_NAME_MAP)
#undef VALUE_TYPE_NAME_MAP
    };
    if (strMap.count(type) > 0) {
        return strMap.at(type);
    }
    return "UNKNOW";
}
}
