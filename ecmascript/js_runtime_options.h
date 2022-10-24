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

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ecmascript/compiler/bc_call_signature.h"
#include "ecmascript/mem/mem_common.h"

// namespace panda {
namespace panda::ecmascript {
using arg_list_t = std::vector<std::string>;
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
    EXCEPTION_BACKTRACE = 1 << 9,
};

// asm interpreter control parsed option
struct AsmInterParsedOption {
    int handleStart {-1};
    int handleEnd {-1};
    bool enableAsm {false};
};

const std::string COMMON_HELP_HEAD_MSG =
    "Usage: panda [OPTIONS] [file1:file2:file3] [entrypoint] -- [arguments]\n"
    "\n"
    "optional arguments:\n";

const std::string STUB_HELP_HEAD_MSG =
    "Usage: ark_stub_compiler [OPTIONS]\n"
    "\n"
    "optional arguments:\n";

const std::string HELP_OPTION_MSG =
    "--abc-list-file: abc's list file.\n"
    "--aot-file: Path (file suffix not needed) to AOT output file. Default: \"aot_file\"\n"
    "--ark-properties: set ark properties\n"
    "--asm-interpreter: Enable asm interpreter. Default: true\n"
    "--asm-opcode-disable-range: Opcode range when asm interpreter is enabled.\n"
    "--assert-types: Enable type assertion for type inference tests. Default: false\n"
    "--builtins-dts: builtins.d.abc file path for AOT.\n"
    "--compiler-log: log Option For aot compiler and stub compiler,\n"
    "       \"none\": no log,\n"
    "       \"allllircirasm or all012\": print llIR file, CIR log and asm log for all methods,\n"
    "       \"allcir or all0\": print cir info for all methods,\n"
    "       \"allllir or all1\": print llir info for all methods,\n"
    "       \"allasm or all2\": print asm log for all methods,\n"
    "       \"cerllircirasm or cer0112\": print llIR file, CIR log and asm log for certain method defined "
                                          "in 'mlist-for-log',\n"
    "       \"cercir or cer0\": print cir info for certain method illustrated in 'mlist-for-log',\n"
    "       \"cerasm or cer2\": print asm log for certain method illustrated in 'mlist-for-log',\n"
    "       Default: \"none\"\n"
    "--compiler-log-methods: specific method list for compiler log output, only used when compiler-log."
                            "Default: \"none\"\n"
    "--enable-ark-tools: Enable ark tools to debug. Default: false\n"
    "--enable-bytecode-trace: enable tracing bytecode for aot runtime. Default: false\n"
    "--enable-cpuprofiler: Enable cpuprofiler to sample call stack and output to json file. Default: false\n"
    "--enable-force-gc: enable force gc when allocating object. Default: true\n"
    "--enable-ic: switch of inline cache. Default: true\n"
    "--enable-runtime-stat: enable statistics of runtime state. Default: false\n"
    "--enable-type-lowering: enable TSTypeLowering and TypeLowering for aot runtime. Default:true\n"
    "--entry-point: full name of entrypoint function or method. Default: _GLOBAL::func_main_0\n"
    "--force-full-gc: if true trigger full gc, else trigger semi and old gc. Default: true\n"
    "--framework-abc-file: snapshot file. Default: \"strip.native.min.abc\"\n"
    "--gcThreadNum: set gcThreadNum. Default: 7\n"
    "--heap-size-limit: Max heap size. Default: 512MB\n"
    "--help: Print this message and exit\n"
    "--icu-data-path: Path to generated icu data file. Default: \"default\"\n"
    "--IsWorker: whether is worker vm. Default: false\n"
    "--log-level: Log level. Possible values: [\"debug\", \"info\", \"warning\", \"error\", \"fatal\"].\n"
    "--log-components: Enable logs from specified components. Possible values: [\"all\", \"gc\", \"ecma\","
        "\"interpreter\", \"debugger\", \"compiler\", \"all\"]. Default: [\"all\"]\n"
    "--log-debug: Enable debug or above logs from specified components. Possible values: [\"all\", \"gc\", \"ecma\","
        "\"interpreter\", \"debugger\", \"compiler\", \"all\"]. Default: [\"all\"]\n"
    "--log-error: Enable error or above logs from specified components. Possible values: [\"all\", \"gc\", \"ecma\","
        "\"interpreter\", \"debugger\", \"compiler\", \"all\"]. Default: [\"all\"]\n"
    "--log-fatal: Enable fatal logs from specified components. Possible values: [\"all\", \"gc\", \"ecma\","
        "\"interpreter\", \"debugger\", \"compiler\", \"all\"]. Default: [\"all\"]\n"
    "--log-info: Enable info or above logs from specified components. Possible values: [\"all\", \"gc\", \"ecma\","
        "\"interpreter\", \"debugger\", \"compiler\", \"all\"]. Default: [\"all\"]\n"
    "--log-warning: Enable warning or above logs from specified components. Possible values: [\"all\", \"gc\","
        "\"ecma\", \"interpreter\", \"debugger\", \"compiler\", \"all\"]. Default: [\"all\"]\n"
    "--longPauseTime: set longPauseTime. Default: 40ms\n"
    "--maxAotMethodSize: enable aot to skip too large method. Default size: 32 KB\n"
    "--maxNonmovableSpaceCapacity: set max nonmovable space capacity\n"
    "--merge-abc: abc file is merge abc. Default: false\n"
    "--opt-level: Optimization level configuration on llvm back end. Default: \"3\"\n"
    "--options: Print compiler and runtime options\n"
    "--print-any-types: Enable TypeFilter to print any types after type inference. Default: false\n"
    "--reloc-mode: Relocation configuration on llvm back end. Default: \"2\"\n"
    "--serializer-buffer-size-limit: Max serializer buffer size used by the VM. Default: 2GB\n"
    "--snapshot-file: snapshot file. Default: \"/system/etc/snapshot\"\n"
    "--startup-time: Print the start time of command execution. Default: false\n"
    "--stub-file: Path of file includes common stubs module compiled by stub compiler. Default: \"stub.an\"\n"
    "--enable-pgo-profiler: Enable pgo profiler to sample jsfunction call and output to file. Default: false\n"
    "--pgo-hotness-threshold: set hotness threshold for pgo in aot compiler. Default: 2\n"
    "--pgo-profiler-path: The pgo sampling profiler file output dir for application or ark_js_vm runtime,"
        "or the sampling profiler file input dir for AOT PGO compiler. Default: ""\n"
    "--target-triple: target triple for aot compiler or stub compiler.\n"
    "--enable-print-execute-time: enable print execute pandafile spent time\"\n"
    "       Possible values: [\"x86_64-unknown-linux-gnu\", \"arm-unknown-linux-gnu\", "
                             "\"aarch64-unknown-linux-gnu\"].\n"
    "       Default: \"x86_64-unknown-linux-gnu\"\n";

const std::string HELP_TAIL_MSG =
    "Tail arguments:\n"
    "files: path to pandafiles\n";

enum CommandValues {
    OPTION_DEFAULT,
    OPTION_ENABLE_ARK_TOOLS,
    OPTION_ENABLE_CPUPROFILER,
    OPTION_STUB_FILE,
    OPTION_ENABLE_FORCE_GC,
    OPTION_FORCE_FULL_GC,
    OPTION_ARK_PROPERTIES,
    OPTION_GC_THREADNUM,
    OPTION_LONG_PAUSE_TIME,
    OPTION_AOT_FILE,
    OPTION_TARGET_TRIPLE,
    OPTION_ASM_OPT_LEVEL,
    OPTION_RELOCATION_MODE,
    OPTION_MAX_NONMOVABLE_SPACE_CAPACITY,
    OPTION_ENABLE_ASM_INTERPRETER,
    OPTION_ASM_OPCODE_DISABLE_RANGE,
    OPTION_SERIALIZER_BUFFER_SIZE_LIMIT,
    OPTION_HEAP_SIZE_LIMIT,
    OPTION_ENABLE_IC,
    OPTION_SNAPSHOT_FILE,
    OPTION_FRAMEWORK_ABC_FILE,
    OPTION_ICU_DATA_PATH,
    OPTION_STARTUP_TIME,
    OPTION_COMPILER_LOG_OPT,
    OPTION_COMPILER_LOG_METHODS,
    OPTION_ENABLE_RUNTIME_STAT,
    OPTION_ASSERT_TYPES,
    OPTION_PRINT_ANY_TYPES,
    OPTION_IS_WORKER,
    OPTION_BUILTINS_DTS,
    OPTION_ENABLE_BC_TRACE,
    OPTION_LOG_LEVEL,
    OPTION_LOG_DEBUG,
    OPTION_LOG_INFO,
    OPTION_LOG_WARNING,
    OPTION_LOG_ERROR,
    OPTION_LOG_FATAL,
    OPTION_LOG_COMPONENTS,
    OPTION_MAX_AOTMETHODSIZE,
    OPTION_ABC_FILES_LIST,
    OPTION_ENTRY_POINT,
    OPTION_MERGE_ABC,
    OPTION_ENABLE_TYPE_LOWERING,
    OPTION_HELP,
    OPTION_PGO_PROFILER_PATH,
    OPTION_PGO_HOTNESS_THRESHOLD,
    OPTION_ENABLE_PGO_PROFILER,
    OPTION_OPTIONS,
    OPTION_PRINT_EXECUTE_TIME
};

class PUBLIC_API JSRuntimeOptions {
public:
    explicit JSRuntimeOptions() {}
    ~JSRuntimeOptions() = default;
    DEFAULT_COPY_SEMANTIC(JSRuntimeOptions);
    DEFAULT_MOVE_SEMANTIC(JSRuntimeOptions);

    bool ParseCommand(const int argc, const char **argv);
    bool SetDefaultValue(char* argv);

    bool EnableArkTools() const
    {
        return (enableArkTools_) ||
            ((static_cast<uint32_t>(arkProperties_) & ArkProperties::ENABLE_ARKTOOLS) != 0);
    }

    void SetEnableArkTools(bool value) {
        enableArkTools_ = value;
    }

    bool WasSetEnableArkTools() const
    {
        return WasOptionSet(OPTION_ENABLE_ARK_TOOLS);
    }

    bool IsEnableRuntimeStat() const
    {
        return enableRuntimeStat_;
    }

    void SetEnableRuntimeStat(bool value)
    {
        enableRuntimeStat_ = value;
    }

    bool WasSetEnableRuntimeStat() const
    {
        return WasOptionSet(OPTION_ENABLE_RUNTIME_STAT);
    }

    std::string GetStubFile() const
    {
        return stubFile_;
    }

    void SetStubFile(std::string value)
    {
        stubFile_ = std::move(value);
    }

    bool WasStubFileSet() const
    {
        return WasOptionSet(OPTION_STUB_FILE);
    }

    void SetEnableAOT(bool value)
    {
        enableAOT_ = value;
    }

    bool GetEnableAOT() const
    {
        return enableAOT_;
    }

    std::string GetAOTOutputFile() const
    {
        return aotOutputFile_;
    }

    void SetAOTOutputFile(std::string value)
    {
        aotOutputFile_ = std::move(value);
    }

    bool WasAOTOutputFileSet() const
    {
        return WasOptionSet(OPTION_AOT_FILE);
    }

    std::string GetTargetTriple() const
    {
        return targetTriple_;
    }

    void SetTargetTriple(std::string value)
    {
        targetTriple_ = std::move(value);
    }

    size_t GetOptLevel() const
    {
        return asmOptLevel_;
    }

    void SetOptLevel(size_t value)
    {
        asmOptLevel_ = value;
    }

    size_t GetRelocMode() const
    {
        return relocationMode_;
    }

    void SetRelocMode(size_t value)
    {
        relocationMode_ = value;
    }

    bool EnableForceGC() const
    {
        return enableForceGc_;
    }

    void SetEnableForceGC(bool value)
    {
        enableForceGc_ = value;
    }

    bool ForceFullGC() const
    {
        return forceFullGc_;
    }

    void SetForceFullGC(bool value)
    {
        forceFullGc_ = value;
    }

    bool EnableCpuProfiler() const
    {
        return enableCpuprofiler_;
    }

    void SetEnableCpuprofiler(bool value)
    {
        enableCpuprofiler_ = value;
    }

    void SetGcThreadNum(size_t num)
    {
        gcThreadNum_ = num;
    }

    size_t GetGcThreadNum() const
    {
        return gcThreadNum_;
    }

    void SetLongPauseTime(size_t time)
    {
        longPauseTime_ = time;
    }

    size_t GetLongPauseTime() const
    {
        return longPauseTime_;
    }

    void SetArkProperties(int prop)
    {
        if (prop != ArkProperties::DEFAULT) {
            arkProperties_ = prop;
        }
    }

    int GetDefaultProperties()
    {
        return ArkProperties::PARALLEL_GC | ArkProperties::CONCURRENT_MARK | ArkProperties::CONCURRENT_SWEEP
            | ArkProperties::ENABLE_ARKTOOLS;
    }

    int GetArkProperties()
    {
        return arkProperties_;
    }

    bool EnableOptionalLog() const
    {
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::OPTIONAL_LOG) != 0;
    }

    bool EnableGCStatsPrint() const
    {
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::GC_STATS_PRINT) != 0;
    }

    bool EnableParallelGC() const
    {
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::PARALLEL_GC) != 0;
    }

    bool EnableConcurrentMark() const
    {
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::CONCURRENT_MARK) != 0;
    }

    bool EnableExceptionBacktrace() const
    {
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::EXCEPTION_BACKTRACE) != 0;
    }

    bool EnableConcurrentSweep() const
    {
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::CONCURRENT_SWEEP) != 0;
    }

    bool EnableThreadCheck() const
    {
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::THREAD_CHECK) != 0;
    }

    bool EnableSnapshotSerialize() const
    {
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::ENABLE_SNAPSHOT_SERIALIZE) != 0;
    }

    bool EnableSnapshotDeserialize() const
    {
        if (WIN_OR_MAC_OR_IOS_PLATFORM) {
            return false;
        }

        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::ENABLE_SNAPSHOT_DESERIALIZE) != 0;
    }

    bool WasSetMaxNonmovableSpaceCapacity() const
    {
        return WasOptionSet(OPTION_MAX_NONMOVABLE_SPACE_CAPACITY);
    }

    size_t MaxNonmovableSpaceCapacity() const
    {
        return maxNonmovableSpaceCapacity_;
    }

    void SetMaxNonmovableSpaceCapacity(uint32_t value)
    {
        maxNonmovableSpaceCapacity_ = value;
    }

    void SetEnableAsmInterpreter(bool value)
    {
        enableAsmInterpreter_ = value;
    }

    bool GetEnableAsmInterpreter() const
    {
        return enableAsmInterpreter_;
    }

    void SetAsmOpcodeDisableRange(std::string value)
    {
        asmOpcodeDisableRange_ = std::move(value);
    }

    void ParseAsmInterOption()
    {
        asmInterParsedOption_.enableAsm = enableAsmInterpreter_;
        std::string strAsmOpcodeDisableRange = asmOpcodeDisableRange_;
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
        return compilerLogOpt_;
    }

    void SetCompilerLogOption(std::string value)
    {
        compilerLogOpt_ = std::move(value);
    }

    bool WasSetCompilerLogOption() const
    {
        return 1ULL << static_cast<uint64_t>(OPTION_COMPILER_LOG_OPT) & wasSet_ &&
            GetCompilerLogOption().find("none") == std::string::npos;
    }

    std::string GetMethodsListForLog() const
    {
        return compilerLogMethods_;
    }

    void SetMethodsListForLog(std::string value)
    {
        compilerLogMethods_ = std::move(value);
    }

    bool WasSetMethodsListForLog() const
    {
        return 1ULL << static_cast<uint64_t>(OPTION_COMPILER_LOG_METHODS) & wasSet_ &&
            GetCompilerLogOption().find("none") == std::string::npos &&
            GetCompilerLogOption().find("all") == std::string::npos;
    }

    uint64_t GetSerializerBufferSizeLimit() const
    {
        return serializerBufferSizeLimit_;
    }

    void SetSerializerBufferSizeLimit(uint64_t value)
    {
        serializerBufferSizeLimit_ = value;
    }

    uint32_t GetHeapSizeLimit() const
    {
        return heapSizeLimit_;
    }

    void SetHeapSizeLimit(uint32_t value)
    {
        heapSizeLimit_ = value;
    }

    bool WasSetHeapSizeLimit() const
    {
        return WasOptionSet(OPTION_HEAP_SIZE_LIMIT);
    }

    void SetIsWorker(bool isWorker)
    {
        isWorker_ = isWorker;
    }

    bool IsWorker() const
    {
        return isWorker_;
    }

    bool EnableIC() const
    {
        return enableIC_;
    }

    void SetEnableIC(bool value)
    {
        enableIC_ = value;
    }

    bool WasSetEnableIC() const
    {
        return WasOptionSet(OPTION_ENABLE_IC);
    }

    std::string GetSnapshotFile() const
    {
        return snapshotFile_;
    }

    void SetSnapshotFile(std::string value)
    {
        snapshotFile_ = std::move(value);
    }

    bool WasSetSnapshotFile() const
    {
        return WasOptionSet(OPTION_SNAPSHOT_FILE);
    }

    std::string GetFrameworkAbcFile() const
    {
        return frameworkAbcFile_;
    }

    void SetFrameworkAbcFile(std::string value)
    {
        frameworkAbcFile_ = std::move(value);
    }

    bool WasSetFrameworkAbcFile() const
    {
        return WasOptionSet(OPTION_FRAMEWORK_ABC_FILE);
    }

    std::string GetIcuDataPath() const
    {
        return icuDataPath_;
    }

    void SetIcuDataPath(std::string value)
    {
        icuDataPath_ = std::move(value);
    }

    bool WasSetIcuDataPath() const
    {
        return WasOptionSet(OPTION_ICU_DATA_PATH);
    }

    bool IsStartupTime() const
    {
        return startupTime_;
    }

    void SetStartupTime(bool value)
    {
        startupTime_ = value;
    }

    bool WasSetStartupTime() const
    {
        return WasOptionSet(OPTION_STARTUP_TIME);
    }

    bool AssertTypes() const
    {
        return assertTypes_;
    }

    void SetAssertTypes(bool value)
    {
        assertTypes_ = value;
    }

    bool PrintAnyTypes() const
    {
        return printAnyTypes_;
    }

    void SetPrintAnyTypes(bool value)
    {
        printAnyTypes_ = value;
    }

    void SetBuiltinsDTS(std::string value)
    {
        builtinsDTS_ = std::move(value);
    }

    bool WasSetBuiltinsDTS() const
    {
        return WasOptionSet(OPTION_BUILTINS_DTS);
    }

    std::string GetBuiltinsDTS() const
    {
        return builtinsDTS_;
    }

    void SetEnableByteCodeTrace(bool value)
    {
        enablebcTrace_ = value;
    }

    bool IsEnableByteCodeTrace() const
    {
        return enablebcTrace_;
    }

    bool WasSetEnableByteCodeTrace() const
    {
        return WasOptionSet(OPTION_ENABLE_BC_TRACE);
    }

    std::string GetLogLevel() const
    {
        return logLevel_;
    }

    void SetLogLevel(std::string value)
    {
        logLevel_ = std::move(value);
    }

    bool WasSetLogLevel() const
    {
        return WasOptionSet(OPTION_LOG_LEVEL);
    }

    arg_list_t GetLogComponents() const
    {
        return logComponents_;
    }

    void SetLogComponents(arg_list_t value)
    {
        logComponents_ = std::move(value);
    }

    bool WasSetLogComponents() const
    {
        return WasOptionSet(OPTION_LOG_COMPONENTS);
    }

    arg_list_t GetLogDebug() const
    {
        return logDebug_;
    }

    void SetLogDebug(arg_list_t value)
    {
        logDebug_ = std::move(value);
    }

    bool WasSetLogDebug() const
    {
        return WasOptionSet(OPTION_LOG_DEBUG);
    }

    arg_list_t GetLogInfo() const
    {
        return logInfo_;
    }

    void SetLogInfo(arg_list_t value)
    {
        logInfo_ = std::move(value);
    }

    bool WasSetLogInfo() const
    {
        return WasOptionSet(OPTION_LOG_INFO);
    }

    arg_list_t GetLogWarning() const
    {
        return logWarning_;
    }

    void SetLogWarning(arg_list_t value)
    {
        logWarning_ = std::move(value);
    }

    bool WasSetLogWarning() const
    {
        return WasOptionSet(OPTION_LOG_WARNING);
    }

    arg_list_t GetLogError() const
    {
        return logError_;
    }

    void SetLogError(arg_list_t value)
    {
        logError_ = std::move(value);
    }

    bool WasSetLogError() const
    {
        return WasOptionSet(OPTION_LOG_ERROR);
    }

    arg_list_t GetLogFatal() const
    {
        return logFatal_;
    }

    void SetLogFatal(arg_list_t value)
    {
        logFatal_ = std::move(value);
    }

    bool WasSetLogFatal() const
    {
        return WasOptionSet(OPTION_LOG_FATAL);
    }

    size_t GetMaxAotMethodSize() const
    {
        return maxAotMethodSize_;
    }

    void SetMaxAotMethodSize(uint32_t value)
    {
        maxAotMethodSize_ = value;
    }

    std::string GetAbcListFile() const
    {
        return abcFilelist_;
    }

    void SetAbcListFile(std::string value)
    {
        abcFilelist_ = std::move(value);
    }

    bool WasSetAbcListFile() const
    {
        return WasOptionSet(OPTION_ABC_FILES_LIST);
    }

    std::string GetEntryPoint() const
    {
        return entryPoint_;
    }

    void SetEntryPoint(std::string value)
    {
        entryPoint_ = std::move(value);
    }

    bool WasSetEntryPoint() const
    {
        return WasOptionSet(OPTION_ENTRY_POINT);
    }

    bool GetMergeAbc() const
    {
        return mergeAbc_;
    }

    void SetMergeAbc(bool value)
    {
        mergeAbc_ = value;
    }

    void SetEnablePrintExecuteTime(bool value)
    {
        enablePrintExecuteTime_ = value;
    }

    bool IsEnablePrintExecuteTime()
    {
        return enablePrintExecuteTime_;
    }

    void ParseAbcListFile(std::vector<std::string> &moduleList) const
    {
        std::ifstream moduleFile(abcFilelist_);
        if (moduleFile.is_open()) {
            char moduleName[FILENAME_MAX];
            while (!moduleFile.eof()) {
                moduleFile.getline(moduleName, FILENAME_MAX);
                if (moduleName[0] != '\0') {
                    moduleList.emplace_back(std::string(moduleName));
                }
            }
            moduleFile.close();
        }
    }

    void SetEnablePGOProfiler(bool value)
    {
        enablePGOProfiler_ = value;
    }

    bool IsEnablePGOProfiler() const
    {
        return enablePGOProfiler_;
    }

    uint32_t GetPGOHotnessThreshold() const
    {
        return pgoHotnessThreshold_;
    }

    void SetPGOHotnessThreshold(uint32_t threshold)
    {
        pgoHotnessThreshold_ = threshold;
    }

    std::string GetPGOProfilerPath() const
    {
        return pgoProfilerPath_;
    }

    void SetPGOProfilerPath(std::string value)
    {
        pgoProfilerPath_ = std::move(value);
    }

    void SetEnableTypeLowering(bool value)
    {
        enableTypeLowering_ = value;
    }

    bool IsEnableTypeLowering() const
    {
        return enableTypeLowering_;
    }

    void WasSet(int opt)
    {
        wasSet_ |= 1ULL << static_cast<uint64_t>(opt);
    }
private:
    static bool StartsWith(const std::string &haystack, const std::string &needle)
    {
        return std::equal(needle.begin(), needle.end(), haystack.begin());
    }

    bool WasOptionSet(int option) const
    {
        return ((1ULL << static_cast<uint64_t>(option)) & wasSet_) != 0;
    }

    bool ParseBoolParam(bool* argBool);
    bool ParseIntParam(const std::string &option, int* argInt);
    bool ParseUint32Param(const std::string &option, uint32_t *argUInt32);
    bool ParseUint64Param(const std::string &option, uint64_t *argUInt64);
    void ParseListArgParam(const std::string &option, arg_list_t *argListStr, std::string delimiter);

    bool enableArkTools_ {true};
    bool enableCpuprofiler_ {false};
    std::string stubFile_ {"stub.an"};
    bool enableForceGc_ {true};
    bool forceFullGc_ {true};
    int arkProperties_ = GetDefaultProperties();
    uint32_t gcThreadNum_ {7}; // 7: default thread num
    uint32_t longPauseTime_ {40}; // 40: default pause time
    std::string aotOutputFile_ {"aot_file"};
    std::string targetTriple_ {"x86_64-unknown-linux-gnu"};
    uint32_t asmOptLevel_ {3}; // 3: default opt level
    uint32_t relocationMode_ {2}; // 2: default relocation mode
    uint32_t maxNonmovableSpaceCapacity_ {4_MB};
    bool enableAsmInterpreter_ {true};
    std::string asmOpcodeDisableRange_ {""};
    AsmInterParsedOption asmInterParsedOption_;
    uint64_t serializerBufferSizeLimit_ {2_GB};
    uint32_t heapSizeLimit_ {512_MB};
    bool enableIC_ {true};
    std::string snapshotFile_ {"/system/etc/snapshot"};
    std::string frameworkAbcFile_ {"strip.native.min.abc"};
    std::string icuDataPath_ {"default"};
    bool startupTime_ {false};
    std::string compilerLogOpt_ {"none"};
    std::string compilerLogMethods_ {"none"};
    bool enableRuntimeStat_ {false};
    bool assertTypes_ {false};
    bool printAnyTypes_ {false};
    bool isWorker_ {false};
    std::string builtinsDTS_ {""};
    bool enablebcTrace_ {false};
    std::string logLevel_ {"error"};
    arg_list_t logDebug_ {{"all"}};
    arg_list_t logInfo_ {{"all"}};
    arg_list_t logWarning_ {{"all"}};
    arg_list_t logError_ {{"all"}};
    arg_list_t logFatal_ {{"all"}};
    arg_list_t logComponents_ {{"all"}};
    bool enableAOT_ {false};
    uint32_t maxAotMethodSize_ {32_KB};
    std::string abcFilelist_ {"none"};
    std::string entryPoint_ {"_GLOBAL::func_main_0"};
    bool mergeAbc_ {false};
    bool enableTypeLowering_ {true};
    uint64_t wasSet_ {0};
    bool enablePrintExecuteTime_ {false};
    bool enablePGOProfiler_ {false};
    uint32_t pgoHotnessThreshold_ {2};
    std::string pgoProfilerPath_ {""};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_RUNTIME_OPTIONS_H_
