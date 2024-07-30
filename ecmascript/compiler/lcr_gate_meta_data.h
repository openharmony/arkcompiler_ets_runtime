/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/type.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript::kungfu {
    
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