/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_CIRCUIT_ARG_INDICES_H
#define ECMASCRIPT_COMPILER_CIRCUIT_ARG_INDICES_H

#include <cstddef>

namespace panda::ecmascript::kungfu {
struct CircuitArgIndices {
    // CircuitBuilder::ArrayIncludesIndexOf(
    //     GateRef elements, GateRef target, GateRef fromIndex, GateRef len, GateRef callID, GateRef arrayKind)
    struct ArrayIncludesIndexOf {
        enum: size_t {
            ELEMENTS,
            TARGET,
            FROM_INDEX,
            LENGTH,
            CALL_ID,
            ARRAY_KIND,
            NUM_ARGS,
        };
    };
};
}

#endif // ECMASCRIPT_COMPILER_CIRCUIT_ARG_INDICES_H
