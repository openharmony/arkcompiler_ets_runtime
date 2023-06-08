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
#ifndef ECMASCRIPT_DFX_VMSTAT_OPT_CODE_PROFILER_H
#define ECMASCRIPT_DFX_VMSTAT_OPT_CODE_PROFILER_H

#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/ecma_opcode_des.h"

namespace panda::ecmascript {
class OptCodeProfiler {
public:
    using EcmaOpcode = kungfu::EcmaOpcode;

    enum Mode {
        TYPED_PATH,
        SLOW_PATH,
    };

    struct Value {
        Value() : typedPathValue(0), slowPathValue(0) {}

        uint64_t TypedPathCount() const
        {
            return typedPathValue;
        }

        uint64_t SlowPathCount() const
        {
            return slowPathValue;
        }

        uint64_t Count() const
        {
            return typedPathValue + slowPathValue;
        }

        void ResetStat()
        {
            typedPathValue = 0;
            slowPathValue = 0;
        }

        uint64_t typedPathValue;
        uint64_t slowPathValue;
    };

    OptCodeProfiler()
    {
#if ECMASCRIPT_ENABLE_OPT_CODE_PROFILER
        profMap_ = {
#define BYTECODE_PROF_MAP(name) { kungfu::EcmaOpcode::name, OptCodeProfiler::Value() },
    ECMA_OPCODE_LIST(BYTECODE_PROF_MAP)
#undef BYTECODE_PROF_MAP
        };
#endif
    }

    ~OptCodeProfiler();

    void Update(EcmaOpcode opcode, Mode mode)
    {
        auto it = profMap_.find(opcode);
        if (it != profMap_.end()) {
            (mode == Mode::TYPED_PATH) ? (it->second.typedPathValue++) : (it->second.slowPathValue++);
        }
    }

    void PrintAndReset();

private:
    std::map<EcmaOpcode, Value> profMap_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_DFX_VMSTAT_OPT_CODE_PROFILER_H
