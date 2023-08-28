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

#ifndef ECMASCRIPT_COMPILER_NUMBER_SPECULATIVE_RUNNER_H
#define ECMASCRIPT_COMPILER_NUMBER_SPECULATIVE_RUNNER_H

#include "ecmascript/compiler/number_speculative_lowering.h"
#include "ecmascript/compiler/pass_manager.h"
#include "ecmascript/compiler/number_speculative_retype.h"
#include "ecmascript/compiler/range_guard.h"
#include "ecmascript/ts_types/ts_manager.h"

namespace panda::ecmascript::kungfu {
class NumberSpeculativeRunner {
public:
    NumberSpeculativeRunner(Circuit *circuit, bool enableLog, const std::string& name, Chunk* chunk, bool onHeapCheck)
        : circuit_(circuit), acc_(circuit), enableLog_(enableLog), methodName_(name),
          chunk_(chunk), typeInfos_(chunk), rangeInfos_(chunk), onHeapCheck_(onHeapCheck) {}

    ~NumberSpeculativeRunner() = default;
    void Run();
private:
    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    bool IsOnHeap() const
    {
        return onHeapCheck_;
    }

    const std::string& GetMethodName() const
    {
        return methodName_;
    }

    Circuit *circuit_ {nullptr};
    GateAccessor acc_;
    bool enableLog_ {false};
    std::string methodName_;
    Chunk *chunk_ {nullptr};
    ChunkVector<TypeInfo> typeInfos_;
    ChunkVector<RangeInfo> rangeInfos_;
    bool onHeapCheck_ {false};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_NUMBER_SPECULATIVE_RUNNER_H
