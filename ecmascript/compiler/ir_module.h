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

#ifndef ECMASCRIPT_COMPILER_IR_MODULE_H
#define ECMASCRIPT_COMPILER_IR_MODULE_H

#include "ecmascript/compiler/circuit_builder_helper.h"
#include "ecmascript/compiler/debug_info.h"
#include "ecmascript/jspandafile/js_pandafile.h"

namespace panda::ecmascript::kungfu {
enum ModuleKind : uint8_t {
    MODULE_LITECG,
    MODULE_LLVM,
};

class IRModule {
public:
    IRModule(NativeAreaAllocator *allocator, bool logDbg, const std::string &triple)
    {
        tripleStr_ = triple;
        CompilationConfig cfg(tripleStr_);
        is64Bit_ = cfg.Is64Bit();
        triple_ = cfg.GetTriple();
        debugInfo_ = new DebugInfo(allocator, logDbg);
    }

    virtual ~IRModule()
    {
        if (debugInfo_ == nullptr) {
            return;
        }
        delete debugInfo_;
        debugInfo_ = nullptr;
    }

    DebugInfo *GetDebugInfo() const
    {
        return debugInfo_;
    }

    Triple GetTriple() const
    {
        return triple_;
    }

    const std::string &GetTripleStr() const
    {
        return tripleStr_;
    }

    bool Is64Bit() const
    {
        return is64Bit_;
    }

    bool Is32Bit() const
    {
        return !is64Bit_;
    }

    virtual ModuleKind GetModuleKind() const = 0;

    static std::string GetFuncName(const MethodLiteral *methodLiteral, const JSPandaFile *jsPandaFile);

private:
    DebugInfo *debugInfo_ {nullptr};
    std::string tripleStr_;
    Triple triple_;
    bool is64Bit_ {false};
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_IR_MODULE_H
