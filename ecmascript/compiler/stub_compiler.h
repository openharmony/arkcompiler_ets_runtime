/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_STUB_COMPILER_H
#define ECMASCRIPT_COMPILER_STUB_COMPILER_H

#include <cstring>

#include "ecmascript/compiler/codegen/llvm/llvm_ir_builder.h"
#include "ecmascript/compiler/compiler_log.h"

namespace panda::ecmascript::kungfu {
class StubCompiler {
public:
    StubCompiler(std::string &triple, std::string &filePath, size_t optLevel, size_t relocMode,
                 CompilerLog *log, MethodLogList *logList, bool concurrentCompile)
        : triple_(triple),
          filePath_(filePath),
          optLevel_(optLevel),
          relocMode_(relocMode),
          log_(log),
          logList_(logList),
          concurrentCompile_(concurrentCompile)
    {
    }

    ~StubCompiler() = default;

    bool BuildStubModuleAndSave() const;

    CompilerLog *GetLog() const
    {
        return log_;
    }

    MethodLogList *GetLogList() const
    {
        return logList_;
    }
private:
    void RunPipeline(LLVMModule *module, NativeAreaAllocator *allocator) const;
    void InitializeCS() const;
    std::string triple_ {};
    std::string filePath_ {};
    size_t optLevel_ {3}; // 3 : default backend optimization level
    size_t relocMode_ {2}; // 2 : default relocation mode-- PIC
    CompilerLog *log_ {nullptr};
    MethodLogList *logList_ {nullptr};
    bool concurrentCompile_ {false};
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_STUB_COMPILER_H
