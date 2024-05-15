/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_BASELINE_BASELINE_COMPILER_H
#define ECMASCRIPT_COMPILER_BASELINE_BASELINE_COMPILER_H

#include <unordered_map>
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/machine_code.h"
#include "ecmascript/compiler/baseline/baseline_assembler.h"
#include "ecmascript/compiler/ecma_opcode_des.h"

namespace panda::ecmascript::kungfu {
static constexpr uint32_t AccRegisterID = 0xFFFFFFFF;
class BaselineCompiler {
public:
    explicit BaselineCompiler(EcmaVM *inputVM)
        : vm(inputVM), baselineAssembler(vm->GetJSOptions().GetTargetTriple()) {}

    ~BaselineCompiler()
    {
        for (auto elem : jumpMap) {
            if (elem.second != nullptr) {
                delete elem.second;
                elem.second = nullptr;
            }
        }
        jumpMap.clear();
    }

    void Compile(JSHandle<JSFunction> &jsFunction);

    BaselineAssembler &GetBaselineAssembler()
    {
        return baselineAssembler;
    }

    void CollectMemoryCodeInfos(MachineCodeDesc &codeDesc);

    void SetPfHeaderAddr(const Method *method);

    void GetJumpToOffsets(const uint8_t *start, const uint8_t *end, std::unordered_set<size_t> &jumpToOffsets) const;
private:
    EcmaVM *vm = nullptr;
    BaselineAssembler baselineAssembler;
    size_t bytecodeOffset = 0;
    std::unordered_map<size_t, JumpLabel*> jumpMap;

#define BYTECODE_BASELINE_HANDLER(name) void Handle##name(const uint8_t *bytecodeArray);
    ECMA_OPCODE_LIST(BYTECODE_BASELINE_HANDLER)
#undef BYTECODE_BASELINE_HANDLER
    const uint8_t *pfHeaderAddr = nullptr;
    const uint8_t *firstPC = nullptr;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BASELINE_BASELINE_COMPILER_H
