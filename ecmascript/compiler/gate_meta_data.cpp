/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "ecmascript/compiler/gate_meta_data.h"
#include "ecmascript/compiler/gate_meta_data_builder.h"

namespace panda::ecmascript::kungfu {
std::string MachineTypeToStr(MachineType machineType)
{
    switch (machineType) {
        case NOVALUE:
            return "NOVALUE";
        case ANYVALUE:
            return "ANYVALUE";
        case I1:
            return "I1";
        case I8:
            return "I8";
        case I16:
            return "I16";
        case I32:
            return "I32";
        case I64:
            return "I64";
        case F32:
            return "F32";
        case F64:
            return "F64";
        default:
            return "???";
    }
}

std::string GateMetaData::TypedBinOpToStr(TypedBinOp typedBinOp) const
{
    switch (typedBinOp) {
        case TypedBinOp::TYPED_ADD: {
            return "Add";
        }
        case TypedBinOp::TYPED_SUB: {
            return "Sub";
        }
        case TypedBinOp::TYPED_MUL: {
            return "Mul";
        }
        case TypedBinOp::TYPED_LESS: {
            return "Less";
        }
        case TypedBinOp::TYPED_LESSEQ: {
            return "LessOrEqual";
        }
        case TypedBinOp::TYPED_GREATER: {
            return "Greater";
        }
        case TypedBinOp::TYPED_GREATEREQ: {
            return "GreaterOrEqual";
        }
        case TypedBinOp::TYPED_EQ: {
            return "Equal";
        }
        case TypedBinOp::TYPED_NOTEQ: {
            return "NotEqual";
        }
        case TypedBinOp::TYPED_STRICTEQ: {
            return "StrictEqual";
        }
        case TypedBinOp::TYPED_SHL: {
            return "Shl";
        }
        case TypedBinOp::TYPED_SHR: {
            return "Shr";
        }
        case TypedBinOp::TYPED_ASHR: {
            return "Ashr";
        }
        case TypedBinOp::TYPED_AND: {
            return "And";
        }
        case TypedBinOp::TYPED_OR: {
            return "Or";
        }
        case TypedBinOp::TYPED_XOR: {
            return "Xor";
        }
        case TypedBinOp::TYPED_DIV: {
            return "Div";
        }
        case TypedBinOp::TYPED_EXP: {
            return "Exp";
        }
        default:
            return "Unknown";
    }
}

std::string GateMetaData::Str(OpCode opcode)
{
    const std::map<OpCode, const char *> strMap = {
#define GATE_NAME_MAP(NAME, OP, R, S, D, V) { OpCode::OP, #OP },
    IMMUTABLE_META_DATA_CACHE_LIST(GATE_NAME_MAP)
    GATE_META_DATA_LIST_WITH_BOOL(GATE_NAME_MAP)
    GATE_META_DATA_LIST_WITH_BOOL_VALUE_IN(GATE_NAME_MAP)
    GATE_META_DATA_LIST_WITH_SIZE(GATE_NAME_MAP)
    GATE_META_DATA_LIST_WITH_ONE_PARAMETER(GATE_NAME_MAP)
    GATE_META_DATA_LIST_WITH_PC_OFFSET(GATE_NAME_MAP)
    GATE_META_DATA_LIST_FOR_CALL(GATE_NAME_MAP)
    GATE_META_DATA_LIST_WITH_PC_OFFSET_FIXED_VALUE(GATE_NAME_MAP)
#undef GATE_NAME_MAP
#define GATE_NAME_MAP(OP) { OpCode::OP, #OP },
        GATE_OPCODE_LIST(GATE_NAME_MAP)
#undef GATE_NAME_MAP
    };
    if (strMap.count(opcode) > 0) {
        return strMap.at(opcode);
    }
    return "OP-" + std::to_string(static_cast<uint8_t>(opcode));
}

bool GateMetaData::IsRoot() const
{
    switch (opcode_) {
        case OpCode::CIRCUIT_ROOT:
        case OpCode::DEPEND_ENTRY:
        case OpCode::ARG_LIST:
        case OpCode::STATE_ENTRY:
        case OpCode::RETURN_LIST:
            return true;
        default:
            return false;
    }
}

bool GateMetaData::IsProlog() const
{
    return (opcode_ == OpCode::ARG);
}

bool GateMetaData::IsFixed() const
{
    switch (opcode_) {
        case OpCode::VALUE_SELECTOR:
        case OpCode::DEPEND_SELECTOR:
        case OpCode::DEPEND_RELAY:
        case OpCode::LOOP_EXIT_DEPEND:
        case OpCode::LOOP_EXIT_VALUE:
            return true;
        default:
            return false;
    }
}

bool GateMetaData::IsSchedulable() const
{
    ASSERT(!IsNop());
#ifndef NDEBUG
    if (GetStateCount() == 0) {
        ASSERT(!IsFixed());
    }
#endif
    return (!IsProlog()) && (!IsRoot()) && (GetStateCount() == 0);
}

bool GateMetaData::IsState() const
{
    ASSERT(!IsNop());
#ifndef NDEBUG
    if (GetStateCount() > 0) {
        ASSERT(!IsProlog());
        ASSERT(!IsRoot());
    }
#endif
    return (!IsVirtualState()) && (!IsFixed()) && (GetStateCount() > 0);
}

bool GateMetaData::IsGeneralState() const
{
    switch (opcode_) {
        case OpCode::IF_BRANCH:
        case OpCode::SWITCH_BRANCH:
        case OpCode::IF_TRUE:
        case OpCode::IF_FALSE:
        case OpCode::IF_SUCCESS:
        case OpCode::IF_EXCEPTION:
        case OpCode::SWITCH_CASE:
        case OpCode::DEFAULT_CASE:
        case OpCode::MERGE:
        case OpCode::LOOP_BEGIN:
        case OpCode::LOOP_BACK:
        case OpCode::LOOP_EXIT:
        case OpCode::ORDINARY_BLOCK:
        case OpCode::STATE_ENTRY:
        case OpCode::DEOPT_CHECK:
        case OpCode::RETURN:
        case OpCode::RETURN_VOID:
            return true;
        default:
            return false;
    }
}

bool GateMetaData::IsTerminalState() const
{
    switch (opcode_) {
        case OpCode::RETURN:
        case OpCode::THROW:
        case OpCode::RETURN_VOID:
        case OpCode::GET_EXCEPTION:
        case OpCode::STATE_SPLIT:
            return true;
        default:
            return false;
    }
}

bool GateMetaData::IsVirtualState() const
{
    switch (opcode_) {
        case OpCode::GET_EXCEPTION:
        case OpCode::STATE_SPLIT:
            return true;
        default:
            return false;
    }
}

bool GateMetaData::IsCFGMerge() const
{
    return (opcode_ == OpCode::MERGE) || (opcode_ == OpCode::LOOP_BEGIN);
}

bool GateMetaData::IsControlCase() const
{
    ASSERT(HasFlag(GateFlags::CONTROL));
    // should add relay
    switch (opcode_) {
        case OpCode::IF_BRANCH:
        case OpCode::SWITCH_BRANCH:
        case OpCode::IF_TRUE:
        case OpCode::IF_FALSE:
        case OpCode::SWITCH_CASE:
        case OpCode::DEFAULT_CASE:
            return true;
        default:
            return false;
    }
}

bool GateMetaData::IsLoopHead() const
{
    return (opcode_ == OpCode::LOOP_BEGIN);
}

bool GateMetaData::IsNop() const
{
    return (opcode_ == OpCode::NOP || opcode_ == OpCode::DEAD);
}

bool GateMetaData::IsConstant() const
{
    return (opcode_ == OpCode::CONSTANT);
}

bool GateMetaData::IsDependSelector() const
{
    return (opcode_ == OpCode::DEPEND_SELECTOR);
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

GateMetaBuilder::GateMetaBuilder(Chunk* chunk)
    : cache_(), chunk_(chunk) {}

#define DECLARE_GATE_META(NAME, OP, R, S, D, V) \
const GateMetaData* GateMetaBuilder::NAME()     \
{                                               \
    return &cache_.cached##NAME##_;             \
}
IMMUTABLE_META_DATA_CACHE_LIST(DECLARE_GATE_META)
#undef DECLARE_GATE_META

#define DECLARE_GATE_META(NAME, OP, R, S, D, V)                           \
const GateMetaData* GateMetaBuilder::NAME(bool value)                     \
{                                                                         \
    auto meta = new (chunk_) BoolMetaData(OpCode::OP, R, S, D, V, value); \
    return meta;                                                          \
}
GATE_META_DATA_LIST_WITH_BOOL(DECLARE_GATE_META)
#undef DECLARE_GATE_META

#define DECLARE_GATE_META_WITH_BOOL_VALUE_IN(NAME, OP, R, S, D, V)        \
const GateMetaData* GateMetaBuilder::NAME(size_t value, bool flag)        \
{                                                                         \
    auto meta = new (chunk_) BoolMetaData(OpCode::OP, R, S, D, V, flag);  \
    return meta;                                                          \
}
GATE_META_DATA_LIST_WITH_BOOL_VALUE_IN(DECLARE_GATE_META_WITH_BOOL_VALUE_IN)
#undef DECLARE_GATE_META_WITH_BOOL_VALUE_IN

#define DECLARE_GATE_META(NAME, OP, R, S, D, V)                    \
const GateMetaData* GateMetaBuilder::NAME(size_t value)            \
{                                                                  \
    switch (value) {                                               \
        case GateMetaDataChache::ONE_VALUE:                        \
            return &cache_.cached##NAME##1_;                       \
        case GateMetaDataChache::TWO_VALUE:                        \
            return &cache_.cached##NAME##2_;                       \
        case GateMetaDataChache::THREE_VALUE:                      \
            return &cache_.cached##NAME##3_;                       \
        case GateMetaDataChache::FOUR_VALUE:                       \
            return &cache_.cached##NAME##4_;                       \
        case GateMetaDataChache::FIVE_VALUE:                       \
            return &cache_.cached##NAME##5_;                       \
        default:                                                   \
            break;                                                 \
    }                                                              \
    auto meta = new (chunk_) GateMetaData(OpCode::OP, R, S, D, V); \
    meta->SetKind(GateMetaData::Kind::MUTABLE_WITH_SIZE);          \
    return meta;                                                   \
}
GATE_META_DATA_LIST_WITH_SIZE(DECLARE_GATE_META)
#undef DECLARE_GATE_META

#define DECLARE_GATE_META(NAME, OP, R, S, D, V)                                   \
const GateMetaData* GateMetaBuilder::NAME(uint64_t value)                         \
{                                                                                 \
    switch (value) {                                                              \
        case GateMetaDataChache::ONE_VALUE:                                       \
            return &cache_.cached##NAME##1_;                                      \
        case GateMetaDataChache::TWO_VALUE:                                       \
            return &cache_.cached##NAME##2_;                                      \
        case GateMetaDataChache::THREE_VALUE:                                     \
            return &cache_.cached##NAME##3_;                                      \
        case GateMetaDataChache::FOUR_VALUE:                                      \
            return &cache_.cached##NAME##4_;                                      \
        case GateMetaDataChache::FIVE_VALUE:                                      \
            return &cache_.cached##NAME##5_;                                      \
        default:                                                                  \
            break;                                                                \
    }                                                                             \
    auto meta = new (chunk_) OneParameterMetaData(OpCode::OP, R, S, D, V, value); \
    meta->SetKind(GateMetaData::Kind::MUTABLE_ONE_PARAMETER);                     \
    return meta;                                                                  \
}
GATE_META_DATA_LIST_WITH_VALUE(DECLARE_GATE_META)
#undef DECLARE_GATE_META

#define DECLARE_GATE_META(NAME, OP, R, S, D, V)                                   \
const GateMetaData* GateMetaBuilder::NAME(uint64_t value)                         \
{                                                                                 \
    auto meta = new (chunk_) OneParameterMetaData(OpCode::OP, R, S, D, V, value); \
    meta->SetKind(GateMetaData::Kind::MUTABLE_ONE_PARAMETER);                     \
    return meta;                                                                  \
}
GATE_META_DATA_LIST_WITH_GATE_TYPE(DECLARE_GATE_META)
#undef DECLARE_GATE_META

#define DECLARE_GATE_META(NAME, OP, R, S, D, V)                                          \
const GateMetaData* GateMetaBuilder::NAME(uint64_t value, uint64_t pcOffset)             \
{                                                                                        \
    auto meta = new (chunk_) OneParameterMetaData(OpCode::OP, R, S, D, value, pcOffset); \
    meta->SetKind(GateMetaData::Kind::MUTABLE_ONE_PARAMETER);                            \
    return meta;                                                                         \
}
GATE_META_DATA_LIST_WITH_PC_OFFSET(DECLARE_GATE_META)
#undef DECLARE_GATE_META

#define DECLARE_GATE_META_FOR_CALL(NAME, OP, R, S, D, V)                                                 \
const GateMetaData* GateMetaBuilder::NAME(uint64_t value, uint64_t pcOffset, bool noGC)         \
{                                                                                               \
    auto meta = new (chunk_) TypedCallMetaData(OpCode::OP, R, S, D, value, pcOffset, noGC);     \
    meta->SetKind(GateMetaData::Kind::TYPED_CALL);                                              \
    return meta;                                                                                \
}
GATE_META_DATA_LIST_FOR_CALL(DECLARE_GATE_META_FOR_CALL)
#undef DECLARE_GATE_META_FOR_CALL

#define DECLARE_GATE_META(NAME, OP, R, S, D, V)                                          \
const GateMetaData* GateMetaBuilder::NAME(uint64_t pcOffset) const                       \
{                                                                                        \
    auto meta = new (chunk_) OneParameterMetaData(OpCode::OP, R, S, D, V, pcOffset);     \
    meta->SetKind(GateMetaData::Kind::MUTABLE_ONE_PARAMETER);                            \
    return meta;                                                                         \
}
GATE_META_DATA_LIST_WITH_PC_OFFSET_FIXED_VALUE(DECLARE_GATE_META)
#undef DECLARE_GATE_META

const GateMetaData* GateMetaBuilder::Arg(uint64_t value)
{
    switch (value) {
#define DECLARE_CACHED_VALUE_CASE(VALUE)                 \
        case VALUE: {                                    \
            return &cache_.cachedArg##VALUE##_;          \
        }
CACHED_ARG_LIST(DECLARE_CACHED_VALUE_CASE)
#undef DECLARE_CACHED_VALUE_CASE
        default:
            break;
    }

    auto meta = new (chunk_) OneParameterMetaData(OpCode::ARG, GateFlags::HAS_ROOT, 0, 0, 0, value);
    meta->SetKind(GateMetaData::Kind::MUTABLE_ONE_PARAMETER);
    return meta;
}
}  // namespace panda::ecmascript::kungfu
