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

namespace panda::ecmascript::kungfu {
constexpr size_t ONE_DEPEND = 1;
constexpr size_t MANY_DEPEND = 2;
constexpr size_t NO_DEPEND = 0;
// NOLINTNEXTLINE(readability-function-size)
const Properties& GateMetaData::GetProperties() const
{
// general schema: [STATE]s + [DEPEND]s + [VALUE]s + [ROOT]
// GENERAL_STATE for any opcode match in
// {IF_TRUE, IF_FALSE, SWITCH_CASE, DEFAULT_CASE, MERGE, LOOP_BEGIN, STATE_ENTRY}
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define STATE(...) (std::make_pair(std::vector<OpCode>{__VA_ARGS__}, false))
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define VALUE(...) (std::make_pair(std::vector<MachineType>{__VA_ARGS__}, false))
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define MANY_STATE(...) (std::make_pair(std::vector<OpCode>{__VA_ARGS__}, true))
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define MANY_VALUE(...) (std::make_pair(std::vector<MachineType>{__VA_ARGS__}, true))
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NO_STATE (std::nullopt)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NO_VALUE (std::nullopt)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NO_ROOT (std::nullopt)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GENERAL_STATE (NOP)
    switch (opcode_) {
        // SHARED
        case OpCode::NOP:
        case OpCode::CIRCUIT_ROOT: {
            static const Properties ps { NOVALUE, NO_STATE, NO_DEPEND, NO_VALUE, NO_ROOT };
            return ps;
        }
        case OpCode::STATE_ENTRY:
        case OpCode::DEPEND_ENTRY:
        case OpCode::RETURN_LIST:
        case OpCode::ARG_LIST: {
            static const Properties ps { NOVALUE, NO_STATE, NO_DEPEND, NO_VALUE, OpCode::CIRCUIT_ROOT };
            return ps;
        }
        case OpCode::RETURN: {
            static const Properties ps { NOVALUE, STATE(OpCode::NOP), ONE_DEPEND,
                                         VALUE(ANYVALUE), OpCode::RETURN_LIST };
            return ps;
        }
        case OpCode::RETURN_VOID: {
            static const Properties ps { NOVALUE, STATE(OpCode::NOP), ONE_DEPEND, NO_VALUE,
                                         OpCode::RETURN_LIST };
            return ps;
        }
        case OpCode::THROW: {
            static const Properties ps { NOVALUE, STATE(OpCode::NOP), ONE_DEPEND, VALUE(ANYVALUE),
                                         NO_ROOT };
            return ps;
        }
        case OpCode::ORDINARY_BLOCK: {
            static const Properties ps { NOVALUE, STATE(OpCode::NOP), NO_DEPEND, NO_VALUE, NO_ROOT };
            return ps;
        }
        case OpCode::IF_BRANCH: {
            static const Properties ps{ NOVALUE, STATE(OpCode::NOP), NO_DEPEND, VALUE(I1), NO_ROOT };
            return ps;
        }
        case OpCode::SWITCH_BRANCH: {
            static const Properties ps { NOVALUE, STATE(OpCode::NOP), NO_DEPEND, VALUE(ANYVALUE), NO_ROOT };
            return ps;
        }
        case OpCode::IF_TRUE:
        case OpCode::IF_FALSE: {
            static const Properties ps { NOVALUE, STATE(OpCode::IF_BRANCH), NO_DEPEND, NO_VALUE, NO_ROOT };
            return ps;
        }
        case OpCode::SWITCH_CASE:
        case OpCode::DEFAULT_CASE: {
            static const Properties ps { NOVALUE, STATE(OpCode::SWITCH_BRANCH), NO_DEPEND, NO_VALUE, NO_ROOT };
            return ps;
        }
        case OpCode::MERGE: {
            static const Properties ps { NOVALUE, MANY_STATE(OpCode::NOP), NO_DEPEND, NO_VALUE, NO_ROOT };
            return ps;
        }
        case OpCode::LOOP_BEGIN: {
            static const Properties ps { NOVALUE, STATE(OpCode::NOP, OpCode::LOOP_BACK), NO_DEPEND,
                                         NO_VALUE, NO_ROOT };
            return ps;
        }
        case OpCode::LOOP_BACK: {
            static const Properties ps { NOVALUE, STATE(OpCode::NOP), NO_DEPEND, NO_VALUE, NO_ROOT };
            return ps;
        }
        case OpCode::VALUE_SELECTOR: {
            static const Properties ps { FLEX, STATE(OpCode::NOP), NO_DEPEND, MANY_VALUE(FLEX), NO_ROOT };
            return ps;
        }
        case OpCode::DEPEND_SELECTOR: {
            static const Properties ps { NOVALUE, STATE(OpCode::NOP), MANY_DEPEND, NO_VALUE, NO_ROOT };
            return ps;
        }
        case OpCode::DEPEND_RELAY: {
            static const Properties ps { NOVALUE, STATE(OpCode::NOP), ONE_DEPEND, NO_VALUE, NO_ROOT };
            return ps;
        }
        case OpCode::DEPEND_AND: {
            static const Properties ps { NOVALUE, NO_STATE, MANY_DEPEND, NO_VALUE, NO_ROOT };
            return ps;
        }
        // High Level IR
        case OpCode::JS_BYTECODE: {
            static const Properties ps { FLEX, STATE(OpCode::NOP), ONE_DEPEND,
                                         MANY_VALUE(ANYVALUE), NO_ROOT };
            return ps;
        }
        case OpCode::IF_SUCCESS:
        case OpCode::IF_EXCEPTION: {
            static const Properties ps { NOVALUE, STATE(OpCode::NOP), NO_DEPEND, NO_VALUE, NO_ROOT };
            return ps;
        }
        case OpCode::GET_EXCEPTION: {
            static const Properties ps { I64, NO_STATE, ONE_DEPEND, NO_VALUE, NO_ROOT };
            return ps;
        }
        // Middle Level IR

        case OpCode::RUNTIME_CALL:
        case OpCode::NOGC_RUNTIME_CALL:
        case OpCode::BYTECODE_CALL:
        case OpCode::DEBUGGER_BYTECODE_CALL:
        case OpCode::BUILTINS_CALL:
        case OpCode::BUILTINS_CALL_WITH_ARGV:
        case OpCode::CALL:
        case OpCode::RUNTIME_CALL_WITH_ARGV: {
            static const Properties ps { FLEX, NO_STATE, ONE_DEPEND, MANY_VALUE(ANYVALUE, ANYVALUE), NO_ROOT };
            return ps;
        }
        case OpCode::ALLOCA: {
            static const Properties ps { ARCH, NO_STATE, NO_DEPEND, NO_VALUE, NO_ROOT };
            return ps;
        }
        case OpCode::ARG: {
            static const Properties ps { FLEX, NO_STATE, NO_DEPEND, NO_VALUE, OpCode::ARG_LIST };
            return ps;
        }
        case OpCode::CONST_DATA: {
            static const Properties ps { ARCH, NO_STATE, NO_DEPEND, NO_VALUE, NO_ROOT};
            return ps;
        }
        case OpCode::RELOCATABLE_DATA: {
            static const Properties ps { ARCH, NO_STATE, NO_DEPEND, NO_VALUE, NO_ROOT };
            return ps;
        }
        case OpCode::CONSTANT: {
            static const Properties ps { FLEX, NO_STATE, NO_DEPEND, NO_VALUE, NO_ROOT };
            return ps;
        }
        case OpCode::ZEXT: {
            static const Properties ps { FLEX, NO_STATE, NO_DEPEND, VALUE(ANYVALUE), NO_ROOT };
            return ps;
        }
        case OpCode::SEXT: {
            static const Properties ps { FLEX, NO_STATE, NO_DEPEND, VALUE(ANYVALUE), NO_ROOT };
            return ps;
        }
        case OpCode::TRUNC: {
            static const Properties ps { FLEX, NO_STATE, NO_DEPEND, VALUE(ANYVALUE), NO_ROOT };
            return ps;
        }
        case OpCode::FEXT: {
            static const Properties ps { F64, NO_STATE, NO_DEPEND, VALUE(ANYVALUE), NO_ROOT };
            return ps;
        }
        case OpCode::FTRUNC: {
            static const Properties ps { F32, NO_STATE, NO_DEPEND, VALUE(ANYVALUE), NO_ROOT };
            return ps;
        }
        case OpCode::REV: {
            static const Properties ps { FLEX, NO_STATE, NO_DEPEND, VALUE(FLEX), NO_ROOT };
            return ps;
        }
        case OpCode::TRUNC_FLOAT_TO_INT64: {
            static const Properties ps { I64, NO_STATE, NO_DEPEND, VALUE(ANYVALUE), NO_ROOT };
            return ps;
        }
        case OpCode::ADD:
        case OpCode::SUB:
        case OpCode::MUL:
        case OpCode::EXP:
        case OpCode::SDIV:
        case OpCode::SMOD:
        case OpCode::UDIV:
        case OpCode::UMOD:
        case OpCode::FDIV:
        case OpCode::FMOD:
        case OpCode::AND:
        case OpCode::XOR:
        case OpCode::OR:
        case OpCode::LSL:
        case OpCode::LSR:
        case OpCode::ASR: {
            static const Properties ps { FLEX, NO_STATE, NO_DEPEND, VALUE(FLEX, FLEX), NO_ROOT };
            return ps;
        }
        case OpCode::ICMP:
        case OpCode::FCMP: {
            static const Properties ps { I1, NO_STATE, NO_DEPEND, VALUE(ANYVALUE, ANYVALUE), NO_ROOT };
            return ps;
        }
        case OpCode::LOAD: {
            static const Properties ps { FLEX, NO_STATE, ONE_DEPEND, VALUE(ARCH), NO_ROOT };
            return ps;
        }
        case OpCode::STORE: {
            static const Properties ps { NOVALUE, NO_STATE, ONE_DEPEND, VALUE(ANYVALUE, ARCH), NO_ROOT };
            return ps;
        }
        case OpCode::TAGGED_TO_INT64: {
            static const Properties ps { I64, NO_STATE, NO_DEPEND, VALUE(I64), NO_ROOT };
            return ps;
        }
        case OpCode::INT64_TO_TAGGED: {
            static const Properties ps { I64, NO_STATE, NO_DEPEND, VALUE(I64), NO_ROOT };
            return ps;
        }
        case OpCode::SIGNED_INT_TO_FLOAT:
        case OpCode::UNSIGNED_INT_TO_FLOAT: {
            static const Properties ps { FLEX, NO_STATE, NO_DEPEND, VALUE(ANYVALUE), NO_ROOT };
            return ps;
        }
        case OpCode::FLOAT_TO_SIGNED_INT:
        case OpCode::UNSIGNED_FLOAT_TO_INT: {
            static const Properties ps { FLEX, NO_STATE, NO_DEPEND, VALUE(ANYVALUE), NO_ROOT };
            return ps;
        }
        case OpCode::BITCAST: {
            static const Properties ps { FLEX, NO_STATE, NO_DEPEND, VALUE(ANYVALUE), NO_ROOT };
            return ps;
        }
        // Deopt relate IR
        case OpCode::GUARD: {
            static const Properties ps { NOVALUE, NO_STATE, ONE_DEPEND, MANY_VALUE(ANYVALUE), NO_ROOT };
            return ps;
        }
        case OpCode::DEOPT: {
            static const Properties ps { NOVALUE, NO_STATE, ONE_DEPEND, MANY_VALUE(ANYVALUE), NO_ROOT };
            return ps;
        }
        case OpCode::FRAME_STATE: {
            static const Properties ps { NOVALUE, NO_STATE, NO_DEPEND, MANY_VALUE(ANYVALUE), NO_ROOT };
            return ps;
        }
        // suspend relate HIR
        case OpCode::RESTORE_REGISTER: {
            static const Properties ps { FLEX, NO_STATE, ONE_DEPEND, NO_VALUE, NO_ROOT };
            return ps;
        }
        case OpCode::SAVE_REGISTER: {
            static const Properties ps { NOVALUE, NO_STATE, ONE_DEPEND, VALUE(ANYVALUE), NO_ROOT };
            return ps;
        }
        // ts type lowering relate IR
        case OpCode::TYPE_CHECK: {
            static const Properties ps { I1, NO_STATE, NO_DEPEND, VALUE(ANYVALUE), NO_ROOT };
            return ps;
        }
        // ts type lowering relate IR
        case OpCode::TYPED_CALL_CHECK: {
            static const Properties ps { I1, STATE(OpCode::NOP), ONE_DEPEND, MANY_VALUE(ANYVALUE), NO_ROOT };
            return ps;
        }
        case OpCode::TYPED_BINARY_OP: {
            static const Properties ps { FLEX, STATE(OpCode::NOP), ONE_DEPEND,
                                         VALUE(ANYVALUE, ANYVALUE, I8), NO_ROOT };
            return ps;
        }
        case OpCode::TYPED_CALL: {
            static const Properties ps { FLEX, STATE(OpCode::NOP), ONE_DEPEND,
                                         MANY_VALUE(ANYVALUE), NO_ROOT };
            return ps;
        }
        case OpCode::TYPE_CONVERT: {
            static const Properties ps { FLEX, STATE(OpCode::NOP), ONE_DEPEND, VALUE(ANYVALUE), NO_ROOT };
            return ps;
        }
        case OpCode::TYPED_UNARY_OP: {
            static const Properties ps { FLEX, STATE(OpCode::NOP), ONE_DEPEND, VALUE(ANYVALUE), NO_ROOT };
            return ps;
        }
        case OpCode::OBJECT_TYPE_CHECK: {
            static const Properties ps { I1, STATE(OpCode::NOP), ONE_DEPEND,
                                         VALUE(ANYVALUE, I64), NO_ROOT };
            return ps;
        }
        case OpCode::HEAP_ALLOC: {
            static const Properties ps { ANYVALUE, STATE(OpCode::NOP), ONE_DEPEND, VALUE(I64), NO_ROOT };
            return ps;
        }
        case OpCode::LOAD_ELEMENT: {
            static const Properties ps { ANYVALUE, STATE(OpCode::NOP), ONE_DEPEND,
                                         VALUE(ANYVALUE, I64), NO_ROOT };
            return ps;
        }
        case OpCode::LOAD_PROPERTY: {
            static const Properties ps { ANYVALUE, STATE(OpCode::NOP), ONE_DEPEND, VALUE(ANYVALUE, ANYVALUE),
                                         NO_ROOT };
            return ps;
        }
        case OpCode::STORE_ELEMENT: {
            static const Properties ps { NOVALUE, STATE(OpCode::NOP), ONE_DEPEND,
                                         VALUE(ANYVALUE, I64, ANYVALUE), NO_ROOT };
            return ps;
        }
        case OpCode::STORE_PROPERTY: {
            static const Properties ps { NOVALUE, STATE(OpCode::NOP), ONE_DEPEND,
                                         VALUE(ANYVALUE, ANYVALUE, ANYVALUE), NO_ROOT };
            return ps;
        }
        case OpCode::TO_LENGTH: {
            static const Properties ps { I64, STATE(OpCode::NOP), ONE_DEPEND, VALUE(ANYVALUE), NO_ROOT };
            return ps;
        }
        case OpCode::GET_ENV: {
            static const Properties ps { FLEX, NO_STATE, ONE_DEPEND, NO_VALUE, NO_ROOT };
            return ps;
        }
        case OpCode::CONSTRUCT: {
            static const Properties ps { FLEX, STATE(OpCode::NOP), ONE_DEPEND,
                                         MANY_VALUE(ANYVALUE, ANYVALUE), NO_ROOT };
            return ps;
        }
        default:
            LOG_COMPILER(ERROR) << "Please complete OpCode properties (OpCode=" << opcode_ << ")";
            UNREACHABLE();
    }
#undef STATE
#undef VALUE
#undef MANY_STATE
#undef MANY_VALUE
#undef NO_STATE
#undef NO_VALUE
#undef NO_ROOT
#undef GENERAL_STATE
}

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

std::string GateMetaData::Str(OpCode opcode)
{
    const std::map<OpCode, const char *> strMap = {
#define GATE_NAME_MAP(NAME, OP, R, S, D, V)  { OpCode::OP, #OP },
    IMMUTABLE_META_DATA_CACHE_LIST(GATE_NAME_MAP)
    GATE_META_DATA_LIST_WITH_SIZE(GATE_NAME_MAP)
    GATE_META_DATA_LIST_WITH_ONE_VALUE(GATE_NAME_MAP)
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

MachineType GateMetaData::GetInMachineType(size_t idx) const
{
    auto valueProp = GetProperties().valuesIn;
    idx -= GetStateCount();
    idx -= GetDependCount();
    ASSERT(valueProp.has_value());
    if (valueProp->second) {
        return valueProp->first.at(std::min(idx, valueProp->first.size() - 1));
    }
    return valueProp->first.at(idx);
}

OpCode GateMetaData::GetInStateCode(size_t idx) const
{
    auto stateProp = GetProperties().statesIn;
    ASSERT(stateProp.has_value());
    if (stateProp->second) {
        return stateProp->first.at(std::min(idx, stateProp->first.size() - 1));
    }
    return stateProp->first.at(idx);
}

bool GateMetaData::IsRoot() const
{
    return (opcode_ == OpCode::CIRCUIT_ROOT) || (opcode_ == OpCode::STATE_ENTRY) ||
        (opcode_ == OpCode::DEPEND_ENTRY) || (opcode_ == OpCode::RETURN_LIST) ||
        (opcode_ == OpCode::ARG_LIST);
}

bool GateMetaData::IsProlog() const
{
    return (opcode_ == OpCode::ARG);
}

bool GateMetaData::IsFixed() const
{
    return (opcode_ == OpCode::VALUE_SELECTOR) || (opcode_ == OpCode::DEPEND_SELECTOR)
        || (opcode_ == OpCode::DEPEND_RELAY);
}

bool GateMetaData::IsSchedulable() const
{
    return (opcode_ != OpCode::NOP) && (!IsProlog()) && (!IsRoot())
        && (!IsFixed()) && (GetStateCount() == 0);
}

bool GateMetaData::IsState() const
{
    return (opcode_ != OpCode::NOP) && (!IsProlog()) && (!IsRoot())
        && (!IsFixed()) && (GetStateCount() > 0);
}

bool GateMetaData::IsGeneralState() const
{
    return ((opcode_ == OpCode::IF_TRUE) || (opcode_ == OpCode::IF_FALSE)
        || (opcode_ == OpCode::JS_BYTECODE) || (opcode_ == OpCode::IF_SUCCESS)
        || (opcode_ == OpCode::IF_EXCEPTION) || (opcode_ == OpCode::SWITCH_CASE)
        || (opcode_ == OpCode::DEFAULT_CASE) || (opcode_ == OpCode::MERGE)
        || (opcode_ == OpCode::LOOP_BEGIN) || (opcode_ == OpCode::ORDINARY_BLOCK)
        || (opcode_ == OpCode::STATE_ENTRY) || (opcode_ == OpCode::TYPED_BINARY_OP)
        || (opcode_ == OpCode::TYPE_CONVERT) || (opcode_ == OpCode::TYPED_UNARY_OP)
        || (opcode_ == OpCode::TO_LENGTH) || (opcode_ == OpCode::HEAP_ALLOC)
        || (opcode_ == OpCode::LOAD_ELEMENT) || (opcode_ == OpCode::LOAD_PROPERTY)
        || (opcode_ == OpCode::STORE_ELEMENT) || (opcode_ == OpCode::STORE_PROPERTY)
        || (opcode_ == OpCode::TYPED_CALL));
}

bool GateMetaData::IsTerminalState() const
{
    return ((opcode_ == OpCode::RETURN) || (opcode_ == OpCode::THROW)
        || (opcode_ == OpCode::RETURN_VOID));
}

bool GateMetaData::IsCFGMerge() const
{
    return (opcode_ == OpCode::MERGE) || (opcode_ == OpCode::LOOP_BEGIN);
}

bool GateMetaData::IsControlCase() const
{
    return (opcode_ == OpCode::IF_BRANCH) || (opcode_ == OpCode::SWITCH_BRANCH)
        || (opcode_ == OpCode::IF_TRUE) || (opcode_ == OpCode::IF_FALSE)
        || (opcode_ == OpCode::IF_SUCCESS) || (opcode_ == OpCode::IF_EXCEPTION) ||
           (opcode_ == OpCode::SWITCH_CASE) || (opcode_ == OpCode::DEFAULT_CASE);
}

bool GateMetaData::IsLoopHead() const
{
    return (opcode_ == OpCode::LOOP_BEGIN);
}

bool GateMetaData::IsNop() const
{
    return (opcode_ == OpCode::NOP);
}

bool GateMetaData::IsConstant() const
{
    return (opcode_ == OpCode::CONSTANT || opcode_ == OpCode::CONST_DATA);
}

bool GateMetaData::IsTypedOperator() const
{
    return (opcode_ == OpCode::TYPED_BINARY_OP) || (opcode_ == OpCode::TYPE_CONVERT)
        || (opcode_ == OpCode::TYPED_UNARY_OP);
}

#define CACHED_VALUE_LIST(V) \
    V(1)                     \
    V(2)                     \
    V(3)                     \
    V(4)                     \
    V(5)                     \

struct GateMetaDataChache {
static constexpr size_t ONE_VALUE = 1;
static constexpr size_t TWO_VALUE = 2;
static constexpr size_t THREE_VALUE = 3;
static constexpr size_t FOUR_VALUE = 4;
static constexpr size_t FIVE_VALUE = 5;

#define DECLARE_CACHED_GATE_META(NAME, OP, R, S, D, V)      \
    GateMetaData cached##NAME##_ { OpCode::OP, R, S, D, V };
    IMMUTABLE_META_DATA_CACHE_LIST(DECLARE_CACHED_GATE_META)
#undef DECLARE_CACHED_GATE_META

#define DECLARE_CACHED_VALUE_META(VALUE)                                           \
GateMetaData cachedMerge##VALUE##_ { OpCode::MERGE, false, VALUE, 0, 0 };          \
GateMetaData cachedDependSelector##VALUE##_ { OpCode::DEPEND_SELECTOR, false, 1, VALUE, 0 };
CACHED_VALUE_LIST(DECLARE_CACHED_VALUE_META)
#undef DECLARE_CACHED_VALUE_META

#define DECLARE_CACHED_GATE_META(NAME, OP, R, S, D, V)                 \
    GateMetaData cached##NAME##1_{ OpCode::OP, R, S, D, ONE_VALUE };   \
    GateMetaData cached##NAME##2_{ OpCode::OP, R, S, D, TWO_VALUE };   \
    GateMetaData cached##NAME##3_{ OpCode::OP, R, S, D, THREE_VALUE }; \
    GateMetaData cached##NAME##4_{ OpCode::OP, R, S, D, FOUR_VALUE };  \
    GateMetaData cached##NAME##5_{ OpCode::OP, R, S, D, FIVE_VALUE };
GATE_META_DATA_LIST_WITH_VALUE_IN(DECLARE_CACHED_GATE_META)
#undef DECLARE_CACHED_GATE_META

#define DECLARE_CACHED_GATE_META(NAME, OP, R, S, D, V)                        \
    OneValueMetaData cached##NAME##1_{ OpCode::OP, R, S, D, V, ONE_VALUE };   \
    OneValueMetaData cached##NAME##2_{ OpCode::OP, R, S, D, V, TWO_VALUE };   \
    OneValueMetaData cached##NAME##3_{ OpCode::OP, R, S, D, V, THREE_VALUE }; \
    OneValueMetaData cached##NAME##4_{ OpCode::OP, R, S, D, V, FOUR_VALUE };  \
    OneValueMetaData cached##NAME##5_{ OpCode::OP, R, S, D, V, FIVE_VALUE };
GATE_META_DATA_LIST_WITH_ONE_VALUE(DECLARE_CACHED_GATE_META)
#undef DECLARE_CACHED_GATE_META
};

namespace {
GateMetaDataChache globalGateMetaDataChache;
}

GateMetaBuilder::GateMetaBuilder(Chunk* chunk) :
    cache_(globalGateMetaDataChache), chunk_(chunk) {}

#define DECLARE_GATE_META(NAME, OP, R, S, D, V) \
const GateMetaData* GateMetaBuilder::NAME()     \
{                                               \
    return &cache_.cached##NAME##_;             \
}
IMMUTABLE_META_DATA_CACHE_LIST(DECLARE_GATE_META)
#undef DECLARE_GATE_META

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
    meta->SetType(GateMetaData::MUTABLE_WITH_SIZE);                \
    return meta;                                                   \
}
GATE_META_DATA_LIST_WITH_SIZE(DECLARE_GATE_META)
#undef DECLARE_GATE_META

#define DECLARE_GATE_META(NAME, OP, R, S, D, V)                               \
const GateMetaData* GateMetaBuilder::NAME(uint64_t value)                     \
{                                                                             \
    switch (value) {                                                          \
        case GateMetaDataChache::ONE_VALUE:                                   \
            return &cache_.cached##NAME##1_;                                  \
        case GateMetaDataChache::TWO_VALUE:                                   \
            return &cache_.cached##NAME##2_;                                  \
        case GateMetaDataChache::THREE_VALUE:                                 \
            return &cache_.cached##NAME##3_;                                  \
        case GateMetaDataChache::FOUR_VALUE:                                  \
            return &cache_.cached##NAME##4_;                                  \
        case GateMetaDataChache::FIVE_VALUE:                                  \
            return &cache_.cached##NAME##5_;                                  \
        default:                                                              \
            break;                                                            \
    }                                                                         \
    auto meta = new (chunk_) OneValueMetaData(OpCode::OP, R, S, D, V, value); \
    meta->SetType(GateMetaData::MUTABLE_ONE_VALUE);                           \
    return meta;                                                              \
}
GATE_META_DATA_LIST_WITH_ONE_VALUE(DECLARE_GATE_META)
#undef DECLARE_GATE_META

}  // namespace panda::ecmascript::kungfu
