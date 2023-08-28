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

#ifndef ECMASCRIPT_COMPILER_STATE_SPLIT_LINEARIZER_H
#define ECMASCRIPT_COMPILER_STATE_SPLIT_LINEARIZER_H

#include <numeric>

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/lcr_lowering.h"
#include "ecmascript/compiler/graph_linearizer.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {
class StateSplitLinearizer {
public:
    StateSplitLinearizer(Circuit *circuit, RPOVisitor *visitor, CompilationConfig *cmpCfg,
                         bool enableLog, const std::string& name, Chunk* chunk)
        : enableLog_(enableLog), methodName_(name), circuit_(circuit),
        graphLinearizer_(circuit, enableLog, name, chunk),
        lcrLowering_(circuit, visitor, cmpCfg, chunk) {}

    void Run();
private:
    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    const std::string& GetMethodName() const
    {
        return methodName_;
    }

    void LinearizeStateSplit();

    bool enableLog_ {false};
    std::string methodName_;
    Circuit* circuit_ {nullptr};
    GraphLinearizer graphLinearizer_;
    LCRLowering lcrLowering_;
    friend class StateDependBuilder;
};
};  // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_STATE_SPLIT_LINEARIZER_H
