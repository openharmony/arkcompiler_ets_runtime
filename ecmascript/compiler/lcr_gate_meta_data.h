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

#ifndef ECMASCRIPT_COMPILER_LCR_GATE_META_DATA_H
#define ECMASCRIPT_COMPILER_LCR_GATE_META_DATA_H

#include <string>

#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/type.h"
#include "ecmascript/mem/chunk.h"
#include "ecmascript/mem/chunk_containers.h"

#include "ecmascript/elements.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"
#include "libpandabase/macros.h"

#include "ecmascript/compiler/share_gate_meta_data.h"

namespace panda::ecmascript::kungfu {

#define LCR_BINARY_GATE_META_DATA_CACHE_LIST(V)                                                      \
    V(Add, ADD, GateFlags::NONE_FLAG, 0, 0, 2)                                                       \
    V(Sub, SUB, GateFlags::NONE_FLAG, 0, 0, 2)                                                       \
    V(Mul, MUL, GateFlags::NONE_FLAG, 0, 0, 2)                                                       \
    V(Exp, EXP, GateFlags::NONE_FLAG, 0, 0, 2)                                                       \
    V(Sdiv, SDIV, GateFlags::NONE_FLAG, 0, 0, 2)                                                     \
    V(Smod, SMOD, GateFlags::NONE_FLAG, 0, 0, 2)                                                     \
    V(Udiv, UDIV, GateFlags::NONE_FLAG, 0, 0, 2)                                                     \
    V(Umod, UMOD, GateFlags::NONE_FLAG, 0, 0, 2)                                                     \
    V(Fdiv, FDIV, GateFlags::NONE_FLAG, 0, 0, 2)                                                     \
    V(Fmod, FMOD, GateFlags::NONE_FLAG, 0, 0, 2)                                                     \
    V(And, AND, GateFlags::NONE_FLAG, 0, 0, 2)                                                       \
    V(Xor, XOR, GateFlags::NONE_FLAG, 0, 0, 2)                                                       \
    V(Or, OR, GateFlags::NONE_FLAG, 0, 0, 2)                                                         \
    V(Lsl, LSL, GateFlags::NONE_FLAG, 0, 0, 2)                                                       \
    V(Lsr, LSR, GateFlags::NONE_FLAG, 0, 0, 2)                                                       \
    V(Asr, ASR, GateFlags::NONE_FLAG, 0, 0, 2)                                                       \
    V(Sqrt, SQRT, GateFlags::NO_WRITE, 0, 0, 1)                                                      \
    V(AddWithOverflow, ADD_WITH_OVERFLOW, GateFlags::NONE_FLAG, 0, 0, 2)                             \
    V(SubWithOverflow, SUB_WITH_OVERFLOW, GateFlags::NONE_FLAG, 0, 0, 2)                             \
    V(MulWithOverflow, MUL_WITH_OVERFLOW, GateFlags::NONE_FLAG, 0, 0, 2)                             \
    V(ExtractValue, EXTRACT_VALUE, GateFlags::NONE_FLAG, 0, 0, 2)
    
#define LCR_UNARY_GATE_META_DATA_CACHE_LIST(V)                                   \
    V(Zext, ZEXT, GateFlags::NONE_FLAG, 0, 0, 1)                                 \
    V(Sext, SEXT, GateFlags::NONE_FLAG, 0, 0, 1)                                 \
    V(Trunc, TRUNC, GateFlags::NONE_FLAG, 0, 0, 1)                               \
    V(Fext, FEXT, GateFlags::NONE_FLAG, 0, 0, 1)                                 \
    V(Ftrunc, FTRUNC, GateFlags::NONE_FLAG, 0, 0, 1)                             \
    V(Rev, REV, GateFlags::NONE_FLAG, 0, 0, 1)                                   \
    V(TaggedToInt64, TAGGED_TO_INT64, GateFlags::NONE_FLAG, 0, 0, 1)             \
    V(Int64ToTagged, INT64_TO_TAGGED, GateFlags::NONE_FLAG, 0, 0, 1)             \
    V(SignedIntToFloat, SIGNED_INT_TO_FLOAT, GateFlags::NONE_FLAG, 0, 0, 1)      \
    V(UnsignedIntToFloat, UNSIGNED_INT_TO_FLOAT, GateFlags::NONE_FLAG, 0, 0, 1)  \
    V(FloatToSignedInt, FLOAT_TO_SIGNED_INT, GateFlags::NONE_FLAG, 0, 0, 1)      \
    V(UnsignedFloatToInt, UNSIGNED_FLOAT_TO_INT, GateFlags::NONE_FLAG, 0, 0, 1)  \
    V(TruncFloatToInt64, TRUNC_FLOAT_TO_INT64, GateFlags::NONE_FLAG, 0, 0, 1)    \
    V(TruncFloatToInt32, TRUNC_FLOAT_TO_INT32, GateFlags::NONE_FLAG, 0, 0, 1)    \
    V(Bitcast, BITCAST, GateFlags::NONE_FLAG, 0, 0, 1)

#define LCR_IMMUTABLE_META_DATA_CACHE_LIST(V)                                                   \
    V(ReadSp, READSP, GateFlags::NONE_FLAG, 0, 0, 0)                                            \
    V(Load, LOAD, GateFlags::NO_WRITE, 0, 1, 1)                                                 \
    V(Store, STORE, GateFlags::NONE_FLAG, 0, 1, 2)                                              \
    LCR_BINARY_GATE_META_DATA_CACHE_LIST(V)                                                     \
    LCR_UNARY_GATE_META_DATA_CACHE_LIST(V)

#define LCR_GATE_META_DATA_LIST_WITH_BOOL_VALUE_IN(V)                              \
    V(CallOptimized, CALL_OPTIMIZED, GateFlags::NONE_FLAG, 0, 1, value)            \
    V(FastCallOptimized, FAST_CALL_OPTIMIZED, GateFlags::NONE_FLAG, 0, 1, value)

#define LCR_GATE_META_DATA_LIST_WITH_VALUE(V)                                           \
    V(Icmp, ICMP, GateFlags::NONE_FLAG, 0, 0, 2)                                        \
    V(Fcmp, FCMP, GateFlags::NONE_FLAG, 0, 0, 2)                                        \
    V(Alloca, ALLOCA, GateFlags::NONE_FLAG, 0, 0, 0)

enum MachineType : uint8_t { // Bit width
    NOVALUE = 0,
    ANYVALUE,
    ARCH,
    FLEX,
    I1,
    I8,
    I16,
    I32,
    I64,
    F32,
    F64,
};

enum class ICmpCondition : uint8_t {
    EQ = 1,
    UGT,
    UGE,
    ULT,
    ULE,
    NE,
    SGT,
    SGE,
    SLT,
    SLE,
};

enum class FCmpCondition : uint8_t {
    ALW_FALSE = 0,
    OEQ,
    OGT,
    OGE,
    OLT,
    OLE,
    ONE,
    ORD,
    UNO,
    UEQ,
    UGT,
    UGE,
    ULT,
    ULE,
    UNE,
    ALW_TRUE,
};

std::string MachineTypeToStr(MachineType machineType);

class BranchWeight {
public:
    static constexpr uint32_t ZERO_WEIGHT = 0;
    static constexpr uint32_t ONE_WEIGHT = 1;
    static constexpr uint32_t WEAK_WEIGHT = 10;
    static constexpr uint32_t STRONG_WEIGHT = 1000;
    static constexpr uint32_t DEOPT_WEIGHT = 2000;
};

}

#endif  // ECMASCRIPT_COMPILER_LCR_GATE_META_DATA_H