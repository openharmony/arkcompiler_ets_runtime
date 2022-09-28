/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_JS_RUNTIME_OPTIONS_H_
#define ECMASCRIPT_JS_RUNTIME_OPTIONS_H_

#include "libpandabase/utils/pandargs.h"

// namespace panda {
namespace panda::ecmascript {
enum ArkProperties {
    DEFAULT = -1,
    OPTIONAL_LOG = 1,
    GC_STATS_PRINT = 1 << 1,
    PARALLEL_GC = 1 << 2,
    CONCURRENT_MARK = 1 << 3,
    CONCURRENT_SWEEP = 1 << 4,
    THREAD_CHECK = 1 << 5,
    ENABLE_ARKTOOLS = 1 << 6,
    ENABLE_SNAPSHOT_SERIALIZE = 1 << 7,
    ENABLE_SNAPSHOT_DESERIALIZE = 1 << 8,
};

// asm interpreter control parsed option
struct AsmInterParsedOption {
    int handleStart {-1};
    int handleEnd {-1};
    bool enableAsm {false};
};

class JSRuntimeOptions {
public:
    explicit JSRuntimeOptions() {}
    ~JSRuntimeOptions() = default;
    DEFAULT_COPY_SEMANTIC(JSRuntimeOptions);
    DEFAULT_MOVE_SEMANTIC(JSRuntimeOptions);

    void AddOptions(PandArgParser *parser)
    {
        parser->Add(&enableArkTools_);
        parser->Add(&enableCpuprofiler_);
        parser->Add(&arkProperties_);
        parser->Add(&maxNonmovableSpaceCapacity_);
        parser->Add(&enableAsmInterpreter_);
        parser->Add(&asmOpcodeDisableRange_);
        parser->Add(&stubFile_);
        parser->Add(&aotOutputFile_);
        parser->Add(&targetTriple_);
        parser->Add(&asmOptLevel_);
        parser->Add(&relocationMode_);
        parser->Add(&methodsListForLog_);
        parser->Add(&compilerLogOpt_);
        parser->Add(&serializerBufferSizeLimit_);
        parser->Add(&heapSizeLimit_);
        parser->Add(&enableIC_);
        parser->Add(&snapshotFile_);
        parser->Add(&frameworkAbcFile_);
        parser->Add(&icuDataPath_);
        parser->Add(&startupTime_);
        parser->Add(&enableRuntimeStat_);
        parser->Add(&typeInferVerify_);
        parser->Add(&builtinsDTS_);
        parser->Add(&enablebcTrace_);
    }

    bool EnableArkTools() const
    {
        return (enableArkTools_.GetValue()) ||
            ((static_cast<uint32_t>(arkProperties_.GetValue()) & ArkProperties::ENABLE_ARKTOOLS) != 0);
    }

    void SetEnableArkTools(bool value)
    {
        enableArkTools_.SetValue(value);
    }

    bool WasSetEnableArkTools() const
    {
        return enableArkTools_.WasSet();
    }

    bool IsEnableRuntimeStat() const
    {
        return enableRuntimeStat_.GetValue();
    }

    void SetEnableRuntimeStat(bool value)
    {
        enableRuntimeStat_.SetValue(value);
    }

    bool WasSetEnableRuntimeStat() const
    {
        return enableRuntimeStat_.WasSet();
    }

    std::string GetStubFile() const
    {
        return stubFile_.GetValue();
    }

    void SetStubFile(std::string value)
    {
        stubFile_.SetValue(std::move(value));
    }

    bool WasStubFileSet() const
    {
        return stubFile_.WasSet();
    }

    std::string GetAOTOutputFile() const
    {
        return aotOutputFile_.GetValue();
    }

    void SetAOTOutputFile(std::string value)
    {
        aotOutputFile_.SetValue(std::move(value));
    }

    bool WasAOTOutputFileSet() const
    {
        return aotOutputFile_.WasSet();
    }

    std::string GetTargetTriple() const
    {
        return targetTriple_.GetValue();
    }

    void SetTargetTriple(std::string value)
    {
        targetTriple_.SetValue(std::move(value));
    }

    size_t GetOptLevel() const
    {
        return asmOptLevel_.GetValue();
    }

    void SetOptLevel(size_t value)
    {
        asmOptLevel_.SetValue(value);
    }

    size_t GetRelocMode() const
    {
        return relocationMode_.GetValue();
    }

    void SetRelocMode(size_t value)
    {
        relocationMode_.SetValue(value);
    }

    bool EnableForceGC() const
    {
        return enableForceGc_.GetValue();
    }

    void SetEnableForceGC(bool value)
    {
        enableForceGc_.SetValue(value);
    }

    bool ForceFullGC() const
    {
        return forceFullGc_.GetValue();
    }

    void SetForceFullGC(bool value)
    {
        forceFullGc_.SetValue(value);
    }

    bool EnableCpuProfiler() const
    {
        return enableCpuprofiler_.GetValue();
    }

    void SetEnableCpuprofiler(bool value)
    {
        enableCpuprofiler_.SetValue(value);
    }

    void SetGcThreadNum(size_t num)
    {
        gcThreadNum_.SetValue(num);
    }

    size_t GetGcThreadNum() const
    {
        return gcThreadNum_.GetValue();
    }

    void SetLongPauseTime(size_t time)
    {
        longPauseTime_.SetValue(time);
    }

    size_t GetLongPauseTime() const
    {
        return longPauseTime_.GetValue();
    }

    void SetArkProperties(int prop)
    {
        if (prop != ArkProperties::DEFAULT) {
            arkProperties_.SetValue(prop);
        }
    }

    int GetDefaultProperties()
    {
        return ArkProperties::PARALLEL_GC | ArkProperties::CONCURRENT_MARK | ArkProperties::CONCURRENT_SWEEP;
    }

    int GetArkProperties()
    {
        return arkProperties_.GetValue();
    }

    bool EnableOptionalLog() const
    {
        return (static_cast<uint32_t>(arkProperties_.GetValue()) & ArkProperties::OPTIONAL_LOG) != 0;
    }

    bool EnableGCStatsPrint() const
    {
        return (static_cast<uint32_t>(arkProperties_.GetValue()) & ArkProperties::GC_STATS_PRINT) != 0;
    }

    bool EnableParallelGC() const
    {
        return (static_cast<uint32_t>(arkProperties_.GetValue()) & ArkProperties::PARALLEL_GC) != 0;
    }

    bool EnableConcurrentMark() const
    {
        return (static_cast<uint32_t>(arkProperties_.GetValue()) & ArkProperties::CONCURRENT_MARK) != 0;
    }

    bool EnableConcurrentSweep() const
    {
        return (static_cast<uint32_t>(arkProperties_.GetValue()) & ArkProperties::CONCURRENT_SWEEP) != 0;
    }

    bool EnableThreadCheck() const
    {
        return (static_cast<uint32_t>(arkProperties_.GetValue()) & ArkProperties::THREAD_CHECK) != 0;
    }

    bool EnableSnapshotSerialize() const
    {
        return (static_cast<uint32_t>(arkProperties_.GetValue()) & ArkProperties::ENABLE_SNAPSHOT_SERIALIZE) != 0;
    }

    bool EnableSnapshotDeserialize() const
    {
        if (WIN_OR_MAC_PLATFORM) {
            return false;
        }

        return (static_cast<uint32_t>(arkProperties_.GetValue()) & ArkProperties::ENABLE_SNAPSHOT_DESERIALIZE) != 0;
    }

    bool WasSetMaxNonmovableSpaceCapacity() const
    {
        return maxNonmovableSpaceCapacity_.WasSet();
    }

    size_t MaxNonmovableSpaceCapacity() const
    {
        return maxNonmovableSpaceCapacity_.GetValue();
    }

    void SetEnableAsmInterpreter(bool value)
    {
        enableAsmInterpreter_.SetValue(value);
    }

    bool GetEnableAsmInterpreter() const
    {
        return enableAsmInterpreter_.GetValue();
    }

    void SetAsmOpcodeDisableRange(std::string value)
    {
        asmOpcodeDisableRange_.SetValue(std::move(value));
    }

    void ParseAsmInterOption()
    {
        asmInterParsedOption_.enableAsm = enableAsmInterpreter_.GetValue();
        std::string strAsmOpcodeDisableRange = asmOpcodeDisableRange_.GetValue();
        if (strAsmOpcodeDisableRange.empty()) {
            return;
        }

        // asm interpreter handle disable range
        size_t pos = strAsmOpcodeDisableRange.find(",");
        if (pos != std::string::npos) {
            std::string strStart = strAsmOpcodeDisableRange.substr(0, pos);
            std::string strEnd = strAsmOpcodeDisableRange.substr(pos + 1);
            int start =  strStart.empty() ? 0 : std::stoi(strStart);
            int end = strEnd.empty() ? kungfu::BYTECODE_STUB_END_ID : std::stoi(strEnd);
            if (start >= 0 && start < kungfu::BytecodeStubCSigns::NUM_OF_ALL_NORMAL_STUBS
                && end >= 0 && end < kungfu::BytecodeStubCSigns::NUM_OF_ALL_NORMAL_STUBS
                && start <= end) {
                asmInterParsedOption_.handleStart = start;
                asmInterParsedOption_.handleEnd = end;
            }
        }
    }

    AsmInterParsedOption GetAsmInterParsedOption() const
    {
        return asmInterParsedOption_;
    }

    std::string GetCompilerLogOption() const
    {
        return compilerLogOpt_.GetValue();
    }

    void SetCompilerLogOption(std::string value)
    {
        compilerLogOpt_.SetValue(std::move(value));
    }

    bool WasSetCompilerLogOption() const
    {
        return compilerLogOpt_.WasSet() && GetCompilerLogOption().find("none") == std::string::npos;
    }

    std::string GetMethodsListForLog() const
    {
        return methodsListForLog_.GetValue();
    }

    void SetMethodsListForLog(std::string value)
    {
        methodsListForLog_.SetValue(std::move(value));
    }

    bool WasSetMethodsListForLog() const
    {
        return methodsListForLog_.WasSet() &&
            GetCompilerLogOption().find("none") == std::string::npos &&
            GetCompilerLogOption().find("all") == std::string::npos;
    }

    uint64_t GetSerializerBufferSizeLimit() const
    {
        return serializerBufferSizeLimit_.GetValue();
    }

    uint32_t GetHeapSizeLimit() const
    {
        return heapSizeLimit_.GetValue();
    }

    void SetHeapSizeLimit(uint32_t value)
    {
        heapSizeLimit_.SetValue(value);
    }

    bool WasSetHeapSizeLimit() const
    {
        return heapSizeLimit_.WasSet();
    }

    void SetIsWorker(bool isWorker)
    {
        isWorker_.SetValue(isWorker);
    }

    bool IsWorker() const
    {
        return isWorker_.GetValue();
    }

    bool EnableIC() const
    {
        return enableIC_.GetValue();
    }

    void SetEnableIC(bool value)
    {
        enableIC_.SetValue(value);
    }

    bool WasSetEnableIC() const
    {
        return enableIC_.WasSet();
    }

    std::string GetSnapshotFile() const
    {
        return snapshotFile_.GetValue();
    }

    void SetSnapshotFile(std::string value)
    {
        snapshotFile_.SetValue(std::move(value));
    }

    bool WasSetSnapshotFile() const
    {
        return snapshotFile_.WasSet();
    }

    std::string GetFrameworkAbcFile() const
    {
        return frameworkAbcFile_.GetValue();
    }

    void SetFrameworkAbcFile(std::string value)
    {
        frameworkAbcFile_.SetValue(std::move(value));
    }

    bool WasSetFrameworkAbcFile() const
    {
        return frameworkAbcFile_.WasSet();
    }

    std::string GetIcuDataPath() const
    {
        return icuDataPath_.GetValue();
    }

    void SetIcuDataPath(std::string value)
    {
        icuDataPath_.SetValue(std::move(value));
    }

    bool WasSetIcuDataPath() const
    {
        return icuDataPath_.WasSet();
    }

    bool IsStartupTime() const
    {
        return startupTime_.GetValue();
    }

    void SetStartupTime(bool value)
    {
        startupTime_.SetValue(value);
    }

    bool WasSetStartupTime() const
    {
        return startupTime_.WasSet();
    }

    bool EnableTypeInferVerify() const
    {
        return typeInferVerify_.GetValue();
    }

    void SetEnableTypeInferVerify(bool value)
    {
        typeInferVerify_.SetValue(value);
    }

    bool WasSetBuiltinsDTS() const
    {
        return builtinsDTS_.WasSet();
    }

    std::string GetBuiltinsDTS() const
    {
        return builtinsDTS_.GetValue();
    }

    void SetEnableByteCodeTrace(bool value)
    {
        enablebcTrace_.SetValue(value);
    }

    bool IsEnableByteCodeTrace() const
    {
        return enablebcTrace_.GetValue();
    }

    bool WasSetEnableByteCodeTrace() const
    {
        return enablebcTrace_.WasSet();
    }

private:
    PandArg<bool> enableArkTools_ {"enable-ark-tools", false, R"(Enable ark tools to debug. Default: false)"};
    PandArg<bool> enableCpuprofiler_ {"enable-cpuprofiler", false,
        R"(Enable cpuprofiler to sample call stack and output to json file. Default: false)"};
    PandArg<std::string> stubFile_ {"stub-file",
        R"(stub.aot)",
        R"(Path of file includes common stubs module compiled by stub compiler. Default: "stub.aot")"};
    PandArg<bool> enableForceGc_ {"enable-force-gc", true, R"(enable force gc when allocating object)"};
    PandArg<bool> forceFullGc_ {"force-full-gc",
        true,
        R"(if true trigger full gc, else trigger semi and old gc)"};
    PandArg<int> arkProperties_ {"ark-properties", GetDefaultProperties(), R"(set ark properties)"};
    PandArg<uint32_t> gcThreadNum_ {"gcThreadNum", 7, R"(set gcThreadNum. Default: 7)"};
    PandArg<uint32_t> longPauseTime_ {"longPauseTime", 40, R"(set longPauseTime. Default: 40ms)"};
    PandArg<std::string> aotOutputFile_ {"aot-file",
        R"(aot_file)",
        R"(Path (file suffix not needed) to AOT output file. Default: "aot_file")"};
    PandArg<std::string> targetTriple_ {"target-triple", R"(x86_64-unknown-linux-gnu)",
        R"(target triple for aot compiler or stub compiler.
        Possible values: ["x86_64-unknown-linux-gnu", "arm-unknown-linux-gnu", "aarch64-unknown-linux-gnu"].
        Default: "x86_64-unknown-linux-gnu")"};
    PandArg<uint32_t> asmOptLevel_ {"opt-level", 3,
        R"(Optimization level configuration on llvm back end. Default: "3")"};
    PandArg<uint32_t> relocationMode_ {"reloc-mode", 2,
        R"(Relocation configuration on llvm back end. Default: "2")"};
    PandArg<uint32_t> maxNonmovableSpaceCapacity_ {"maxNonmovableSpaceCapacity",
        4_MB,
        R"(set max nonmovable space capacity)"};
    PandArg<bool> enableAsmInterpreter_ {"asm-interpreter", false,
        R"(Enable asm interpreter. Default: false)"};
    PandArg<std::string> asmOpcodeDisableRange_ {"asm-opcode-disable-range",
        "",
        R"(Opcode range when asm interpreter is enabled.)"};
    AsmInterParsedOption asmInterParsedOption_;
    PandArg<uint64_t> serializerBufferSizeLimit_ {"serializer-buffer-size-limit", 2_GB,
        R"(Max serializer buffer size used by the VM. Default: 2GB)"};
    PandArg<uint32_t> heapSizeLimit_ {"heap-size-limit", 512_MB,
        R"(Max heap size. Default: 512MB)"};
    PandArg<bool> enableIC_ {"enable-ic", true, R"(switch of inline cache. Default: true)"};
    PandArg<std::string> snapshotFile_ {"snapshot-file", R"(/system/etc/snapshot)",
        R"(snapshot file. Default: "/system/etc/snapshot")"};
    PandArg<std::string> frameworkAbcFile_ {"framework-abc-file", R"(strip.native.min.abc)",
        R"(snapshot file. Default: "strip.native.min.abc")"};
    PandArg<std::string> icuDataPath_ {"icu-data-path", R"(default)",
        R"(Path to generated icu data file. Default: "default")"};
    PandArg<bool> startupTime_ {"startup-time", false, R"(Print the start time of command execution. Default: false)"};
    PandArg<std::string> compilerLogOpt_ {"compiler-log",
        R"(none)",
        R"(log Option For aot compiler and stub compiler,
        "none": no log,
        "allllircirasm or all012": print llIR file, CIR log and asm log for all methods,
        "allcir or all0" : print cir info for all methods,
        "allllir or all1" : print llir info for all methods,
        "allasm or all2" : print asm log for all methods,
        "cerllircirasm or cer0112": print llIR file, CIR log and asm log for certain method defined in 'mlist-for-log',
        "cercir or cer0": print cir info for certain method illustrated in 'mlist-for-log',
        "cerasm or cer2": print asm log for certain method illustrated in 'mlist-for-log',
        Default: "none")"};
    PandArg<std::string> methodsListForLog_ {"mlist-for-log",
        R"(none)",
        R"(specific method list for compiler log output, only used when compiler-log)"};
    PandArg<bool> enableRuntimeStat_ {"enable-runtime-stat", false,
        R"(enable statistics of runtime state. Default: false)"};
    PandArg<bool> typeInferVerify_ {"typeinfer-verify", false,
        R"(Enable type verify for type inference tests. Default: false)"};
    PandArg<bool> isWorker_ {"IsWorker", false,
        R"(whether is worker vm)"};
    PandArg<std::string> builtinsDTS_ {"builtins-dts",
        "",
        R"(builtins.d.abc file path for AOT.)"};
    PandArg<bool> enablebcTrace_ {"enable-bytecode-trace", false,
        R"(enable tracing bytecode for aot runtime. Default: false)"};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_RUNTIME_OPTIONS_H_
