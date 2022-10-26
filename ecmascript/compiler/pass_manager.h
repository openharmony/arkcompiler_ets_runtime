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

#ifndef ECMASCRIPT_COMPILER_PASS_MANAGER_H
#define ECMASCRIPT_COMPILER_PASS_MANAGER_H

#include "ecmascript/compiler/compiler_log.h"
#include "ecmascript/compiler/file_generators.h"
#include "ecmascript/dfx/pgo_profiler/pgo_profiler_loader.h"
#include "ecmascript/ecma_vm.h"

namespace panda::ecmascript::kungfu {
class PassManager {
public:
    PassManager(EcmaVM* vm, std::string entry, std::string &triple, size_t optLevel, size_t relocMode,
                CompilerLog *log, AotMethodLogList *logList, size_t maxAotMethodSize, bool enableTypeLowering,
                uint32_t hotnessThreshold)
        : vm_(vm), entry_(entry), triple_(triple), optLevel_(optLevel), relocMode_(relocMode), log_(log),
          logList_(logList), maxAotMethodSize_(maxAotMethodSize), enableTypeLowering_(enableTypeLowering),
          hotnessThreshold_(hotnessThreshold) {};
    PassManager() = default;
    ~PassManager() = default;

    bool Compile(const std::string &fileName, AOTFileGenerator &generator, const std::string &profilerIn);

private:
    JSPandaFile *CreateJSPandaFile(const CString &fileName);
    JSHandle<JSTaggedValue> ResolveModuleAndConstPool(const JSPandaFile *jsPandaFile, const std::string &fileName);
    bool FilterMethod(const JSPandaFile *jsPandaFile, const CString &recordName, MethodLiteral *method,
        uint32_t methodOffset, MethodPcInfo &methodPCInfo);

    bool EnableTypeLowering() const
    {
        return enableTypeLowering_;
    }

    EcmaVM *vm_ {nullptr};
    std::string entry_ {};
    std::string triple_ {};
    size_t optLevel_ {3}; // 3 : default backend optimization level
    size_t relocMode_ {2}; // 2 : default relocation mode-- PIC
    const CompilerLog *log_ {nullptr};
    AotMethodLogList *logList_ {nullptr};
    size_t maxAotMethodSize_ {0};
    bool enableTypeLowering_ {true};
    uint32_t hotnessThreshold_ {0};
    PGOProfilerLoader profilerLoader_;
};
}
#endif // ECMASCRIPT_COMPILER_PASS_MANAGER_H
