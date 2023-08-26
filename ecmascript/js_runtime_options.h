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
    DEFAULT = -1,  // default value 1000001011100 -> 0x105c
    OPTIONAL_LOG = 1,
    GC_STATS_PRINT = 1 << 1,
    PARALLEL_GC = 1 << 2,  // default enable
    CONCURRENT_MARK = 1 << 3,  // default enable
    CONCURRENT_SWEEP = 1 << 4,  // default enable
    THREAD_CHECK = 1 << 5,
    ENABLE_ARKTOOLS = 1 << 6,  // default enable
    ENABLE_SNAPSHOT_SERIALIZE = 1 << 7,
    ENABLE_SNAPSHOT_DESERIALIZE = 1 << 8,
    EXCEPTION_BACKTRACE = 1 << 9,
    GLOBAL_OBJECT_LEAK_CHECK = 1 << 10,
    GLOBAL_PRIMITIVE_LEAK_CHECK = 1 << 11,
    ENABLE_IDLE_GC = 1 << 12,  // default enable
    CPU_PROFILER_COLD_START_MAIN_THREAD = 1 << 13,
    ENABLE_CPU_PROFILER_VM_TAG = 1 << 14,
    ENABLE_GC_TRACER = 1 << 15,
    CPU_PROFILER_COLD_START_WORKER_THREAD = 1 << 16,
    CPU_PROFILER_ANY_TIME_MAIN_THREAD = 1 << 17,
    CPU_PROFILER_ANY_TIME_WORKER_THREAD = 1 << 18
};

// asm interpreter control parsed option
struct AsmInterParsedOption {
    int handleStart {-1};
    int handleEnd {-1};
    bool enableAsm {false};
};

extern const std::string PUBLIC_API COMMON_HELP_HEAD_MSG;
extern const std::string PUBLIC_API STUB_HELP_HEAD_MSG;
extern const std::string PUBLIC_API COMPILER_HELP_HEAD_MSG;
extern const std::string PUBLIC_API HELP_OPTION_MSG;

enum CommandValues {
    OPTION_DEFAULT,
    OPTION_ENABLE_ARK_TOOLS,
    OPTION_STUB_FILE,
    OPTION_ENABLE_FORCE_GC,
    OPTION_FORCE_FULL_GC,
    OPTION_ARK_PROPERTIES,
    OPTION_ARK_BUNDLENAME,
    OPTION_GC_THREADNUM,
    OPTION_GC_LONG_PAUSED_TIME,
    OPTION_AOT_FILE,
    OPTION_COMPILER_TARGET_TRIPLE,
    OPTION_ASM_OPT_LEVEL,
    OPTION_RELOCATION_MODE,
    OPTION_MAX_UNMOVABLE_SPACE,
    OPTION_ENABLE_ASM_INTERPRETER,
    OPTION_ENABLE_BUILTINS_LAZY,
    OPTION_ASM_OPCODE_DISABLE_RANGE,
    OPTION_SERIALIZER_BUFFER_SIZE_LIMIT,
    OPTION_HEAP_SIZE_LIMIT,
    OPTION_ENABLE_IC,
    OPTION_ICU_DATA_PATH,
    OPTION_STARTUP_TIME,
    OPTION_COMPILER_LOG_OPT,
    OPTION_COMPILER_LOG_METHODS,
    OPTION_COMPILER_TYPE_THRESHOLD,
    OPTION_ENABLE_RUNTIME_STAT,
    OPTION_COMPILER_ASSERT_TYPES,
    OPTION_COMPILER_PRINT_TYPE_INFO,
    OPTION_COMPILER_LOG_SNAPSHOT,
    OPTION_COMPILER_LOG_TIME,
    OPTION_ENABLE_WORKER,
    OPTION_BUILTINS_DTS,
    OPTION_COMPILER_TRACE_BC,
    OPTION_COMPILER_TRACE_DEOPT,
    OPTION_COMPILER_TRACE_INLINE,
    OPTION_COMPILER_MAX_INLINE_BYTECODES,
    OPTION_COMPILER_DEOPT_THRESHOLD,
    OPTION_COMPILER_STRESS_DEOPT,
    OPTION_COMPILER_OPT_CODE_PROFILER,
    OPTION_LOG_LEVEL,
    OPTION_LOG_DEBUG,
    OPTION_LOG_INFO,
    OPTION_LOG_WARNING,
    OPTION_LOG_ERROR,
    OPTION_LOG_FATAL,
    OPTION_LOG_COMPONENTS,
    OPTION_COMPILER_OPT_MAX_METHOD,
    OPTION_COMPILER_MODULE_METHODS,
    OPTION_ENTRY_POINT,
    OPTION_MERGE_ABC,
    OPTION_COMPILER_OPT_TYPE_LOWERING,
    OPTION_COMPILER_OPT_EARLY_ELIMINATION,
    OPTION_COMPILER_OPT_LATER_ELIMINATION,
    OPTION_COMPILER_OPT_VALUE_NUMBERING,
    OPTION_COMPILER_OPT_INLINING,
    OPTION_COMPILER_OPT_PGOTYPE,
    OPTION_COMPILER_OPT_TRACK_FIELD,
    OPTION_COMPILER_OPT_GLOBAL_TYPEINFER,
    OPTION_HELP,
    OPTION_COMPILER_PGO_PROFILER_PATH,
    OPTION_COMPILER_PGO_HOTNESS_THRESHOLD,
    OPTION_ENABLE_PGO_PROFILER,
    OPTION_OPTIONS,
    OPTION_PRINT_EXECUTE_TIME,
    OPTION_COMPILER_VERIFY_VTABLE,
    OPTION_COMPILER_SELECT_METHODS,
    OPTION_COMPILER_SKIP_METHODS,
    OPTION_TARGET_COMPILER_MODE,
    OPTION_HAP_PATH,
    OPTION_HAP_ABC_OFFSET,
    OPTION_HAP_ABC_SIZE,
    OPTION_COMPILER_NOCHECK,
    OPTION_FAST_AOT_COMPILE_MODE,
    OPTION_COMPILER_OPT_LOOP_PEELING,
    OPTION_COMPILER_OPT_ON_HEAP_CHECK,
    OPTION_COMPILER_OPT_ARRAY_BOUNDS_CHECK_ELIMINATION,
};

class PUBLIC_API JSRuntimeOptions {
public:
    JSRuntimeOptions() {}
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

    void SetAOTOutputFile(const std::string& value)
    {
        aotOutputFile_ = panda::os::file::File::GetExtendedFilePath(value);
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

    void SetArkBundleName(std::string bundleName)
    {
        if (bundleName != "") {
            arkBundleName_ = bundleName;
        }
    }

    int GetDefaultProperties()
    {
        return ArkProperties::PARALLEL_GC | ArkProperties::CONCURRENT_MARK | ArkProperties::CONCURRENT_SWEEP |
            ArkProperties::ENABLE_ARKTOOLS | ArkProperties::ENABLE_IDLE_GC;
    }

    int GetArkProperties()
    {
        return arkProperties_;
    }

    std::string GetArkBundleName() const
    {
        return arkBundleName_;
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

    bool EnableIdleGC() const
    {
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::ENABLE_IDLE_GC) != 0;
    }

    bool EnableGCTracer() const
    {
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::ENABLE_GC_TRACER) != 0;
    }

    bool EnableGlobalObjectLeakCheck() const
    {
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::GLOBAL_OBJECT_LEAK_CHECK) != 0;
    }

    bool EnableGlobalPrimitiveLeakCheck() const
    {
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::GLOBAL_PRIMITIVE_LEAK_CHECK) != 0;
    }

    bool EnableGlobalLeakCheck() const
    {
        return EnableGlobalObjectLeakCheck() || EnableGlobalPrimitiveLeakCheck();
    }

    bool EnableCpuProfilerColdStartMainThread() const
    {
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::CPU_PROFILER_COLD_START_MAIN_THREAD) != 0;
    }

    bool EnableCpuProfilerColdStartWorkerThread() const
    {
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::CPU_PROFILER_COLD_START_WORKER_THREAD) != 0;
    }

    bool EnableCpuProfilerAnyTimeMainThread() const
    {
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::CPU_PROFILER_ANY_TIME_MAIN_THREAD) != 0;
    }

    bool EnableCpuProfilerAnyTimeWorkerThread() const
    {
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::CPU_PROFILER_ANY_TIME_WORKER_THREAD) != 0;
    }

    bool EnableCpuProfilerVMTag() const
    {
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::ENABLE_CPU_PROFILER_VM_TAG) != 0;
    }

    bool IsStartGlobalLeakCheck() const
    {
        return startGlobalLeakCheck_;
    }

    void SwitchStartGlobalLeakCheck()
    {
        startGlobalLeakCheck_ = !startGlobalLeakCheck_;
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

    void DisableReportModuleResolvingFailure()
    {
        reportModuleResolvingFailure_ = false;
    }

    bool EnableReportModuleResolvingFailure() const
    {
        return reportModuleResolvingFailure_;
    }

    bool WasSetMaxNonmovableSpaceCapacity() const
    {
        return WasOptionSet(OPTION_MAX_UNMOVABLE_SPACE);
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

    void SetEnableBuiltinsLazy(bool value)
    {
        enableBuiltinsLazy_ = value;
    }

    bool GetEnableBuiltinsLazy() const
    {
        return enableBuiltinsLazy_;
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
            int start = strStart.empty() ? 0 : std::stoi(strStart);
            int end = strEnd.empty() ? kungfu::BYTECODE_STUB_END_ID : std::stoi(strEnd);
            if (start >= 0 && start < kungfu::BytecodeStubCSigns::NUM_OF_ALL_NORMAL_STUBS &&
                end >= 0 && end < kungfu::BytecodeStubCSigns::NUM_OF_ALL_NORMAL_STUBS &&
                start <= end) {
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

    void SetCompilerLogSnapshot(bool value)
    {
        compilerLogSnapshot_ = value;
    }

    bool IsEnableCompilerLogSnapshot() const
    {
        return compilerLogSnapshot_;
    }

    bool WasSetCompilerLogSnapshot() const
    {
        return WasOptionSet(OPTION_COMPILER_LOG_SNAPSHOT);
    }

    void SetCompilerLogTime(bool value)
    {
        compilerLogTime_ = value;
    }

    bool IsEnableCompilerLogTime() const
    {
        return compilerLogTime_;
    }

    bool WasSetCompilerLogTime() const
    {
        return WasOptionSet(OPTION_COMPILER_LOG_TIME);
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

    bool PrintTypeInfo() const
    {
        return printTypeInfo_;
    }

    void SetPrintTypeInfo(bool value)
    {
        printTypeInfo_ = value;
    }

    void SetBuiltinsDTS(const std::string& value)
    {
        builtinsDTS_ = panda::os::file::File::GetExtendedFilePath(value);
    }

    bool WasSetBuiltinsDTS() const
    {
        return WasOptionSet(OPTION_BUILTINS_DTS);
    }

    std::string GetBuiltinsDTS() const
    {
        return builtinsDTS_;
    }

    void SetTraceBc(bool value)
    {
        traceBc_ = value;
    }

    bool IsTraceBC() const
    {
        return traceBc_;
    }

    bool WasSetTraceBc() const
    {
        return WasOptionSet(OPTION_COMPILER_TRACE_BC);
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

    double GetTypeThreshold() const
    {
        return typeThreshold_;
    }

    void SetTypeThreshold(double threshold)
    {
        typeThreshold_ = threshold;
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

    void SetPGOProfilerPath(const std::string& value)
    {
        pgoProfilerPath_ = panda::os::file::File::GetExtendedFilePath(value);
    }

    bool IsPGOProfilerPathEmpty() const
    {
        return pgoProfilerPath_.empty();
    }

    void SetEnableTypeLowering(bool value)
    {
        enableTypeLowering_ = value;
    }

    bool IsEnableArrayBoundsCheckElimination() const
    {
        return enableArrayBoundsCheckElimination_;
    }

    void SetEnableArrayBoundsCheckElimination(bool value)
    {
        enableArrayBoundsCheckElimination_ = value;
    }

    bool IsEnableTypeLowering() const
    {
        return enableTypeLowering_;
    }

    void SetEnableEarlyElimination(bool value)
    {
        enableEarlyElimination_ = value;
    }

    bool IsEnableEarlyElimination() const
    {
        return enableEarlyElimination_;
    }

    void SetEnableLaterElimination(bool value)
    {
        enableLaterElimination_ = value;
    }

    bool IsEnableLaterElimination() const
    {
        return enableLaterElimination_;
    }

    void SetEnableValueNumbering(bool value)
    {
        enableValueNumbering_ = value;
    }

    bool IsEnableValueNumbering() const
    {
        return enableValueNumbering_;
    }

    void SetEnableOptInlining(bool value)
    {
        enableOptInlining_ = value;
    }

    bool IsEnableOptInlining() const
    {
        return enableOptInlining_;
    }

    void SetEnableOptPGOType(bool value)
    {
        enableOptPGOType_ = value;
    }

    bool IsEnableOptPGOType() const
    {
        return enableOptPGOType_;
    }

    void SetEnableOptTrackField(bool value)
    {
        enableOptTrackField_ = value;
    }

    bool IsEnableOptTrackField() const
    {
        return enableOptTrackField_;
    }

    void SetEnableGlobalTypeInfer(bool value)
    {
        enableGlobalTypeInfer_ = value;
    }

    bool IsEnableGlobalTypeInfer() const
    {
        return enableGlobalTypeInfer_;
    }

    uint32_t GetCompilerModuleMethods() const
    {
        return compilerModuleMethods_;
    }

    void SetCompilerModuleMethods(uint32_t compilerModuleMethods)
    {
        compilerModuleMethods_ = compilerModuleMethods;
    }

    void WasSet(int opt)
    {
        wasSet_ |= 1ULL << static_cast<uint64_t>(opt);
    }

    void SetTraceDeopt(bool value)
    {
        traceDeopt_ = value;
    }

    bool GetTraceDeopt() const
    {
        return traceDeopt_;
    }

    void SetDeoptThreshold(uint8_t value)
    {
        deoptThreshold_ = value;
    }

    uint32_t GetDeoptThreshold() const
    {
        return deoptThreshold_;
    }

    void SetStressDeopt(bool value)
    {
        stressDeopt_ = value;
    }

    bool GetStressDeopt() const
    {
        return stressDeopt_;
    }

    void SetOptCodeProfiler(bool value)
    {
        optCodeProfiler_ = value;
    }

    bool GetOptCodeProfiler() const
    {
        return optCodeProfiler_;
    }

    void SetVerifyVTable(bool value)
    {
        verifyVTable_ = value;
    }

    bool GetVerifyVTable() const
    {
        return verifyVTable_;
    }

    std::string GetCompilerSelectMethods() const
    {
        return compilerSelectMethods_;
    }

    void SetCompilerSelectMethods(std::string value)
    {
        compilerSelectMethods_ = std::move(value);
    }

    std::string GetCompilerSkipMethods() const
    {
        return compilerSkipMethods_;
    }

    void SetCompilerSkipMethods(std::string value)
    {
        compilerSkipMethods_ = std::move(value);
    }

    void SetTraceInline(bool value)
    {
        traceInline_ = value;
    }

    bool GetTraceInline() const
    {
        return traceInline_;
    }

    void SetMaxInlineBytecodes(size_t value)
    {
        maxInlineBytecodes_ = value;
    }

    size_t GetMaxInlineBytecodes()
    {
        return maxInlineBytecodes_;
    }

    void SetTargetCompilerMode(std::string mode)
    {
        targetCompilerMode_ = std::move(mode);
    }

    std::string GetTargetCompilerMode() const
    {
        return targetCompilerMode_;
    }

    bool IsTargetCompilerMode() const
    {
        return IsPartialCompilerMode() || IsFullCompilerMode();
    }

    bool IsPartialCompilerMode() const
    {
        return targetCompilerMode_ == "partial";
    }

    bool IsFullCompilerMode() const
    {
        return targetCompilerMode_ == "full";
    }

    void SetHapPath(std::string path)
    {
        hapPath_ = std::move(path);
    }

    std::string GetHapPath() const
    {
        return hapPath_;
    }

    void SetHapAbcOffset(uint32_t offset)
    {
        hapAbcOffset_ = offset;
    }

    uint32_t GetHapAbcOffset() const
    {
        return hapAbcOffset_;
    }

    void SetHapAbcSize(uint32_t size)
    {
        hapAbcSize_ = size;
    }

    uint32_t GetHapAbcSize() const
    {
        return hapAbcSize_;
    }

    void SetCompilerNoCheck(bool value)
    {
        compilerNoCheck_ = value;
    }

    bool IsCompilerNoCheck() const
    {
        return compilerNoCheck_;
    }

    void SetTargetBuiltinsDtsPath();

    void SetOptionsForTargetCompilation();

    void SetFastAOTCompileMode(bool value)
    {
        fastAOTCompileMode_ = value;
    }

    bool GetFastAOTCompileMode() const
    {
        return fastAOTCompileMode_;
    }

    void SetEnableOptLoopPeeling(bool value)
    {
        enableOptLoopPeeling_ = value;
    }

    bool IsEnableOptLoopPeeling() const
    {
        return enableOptLoopPeeling_;
    }

    void SetEnableOptOnHeapCheck(bool value)
    {
        enableOptOnHeapCheck_ = value;
    }

    bool IsEnableOptOnHeapCheck() const
    {
        return enableOptOnHeapCheck_;
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
    bool ParseDoubleParam(const std::string &option, double* argDouble);
    bool ParseIntParam(const std::string &option, int* argInt);
    bool ParseUint32Param(const std::string &option, uint32_t *argUInt32);
    bool ParseUint64Param(const std::string &option, uint64_t *argUInt64);
    void ParseListArgParam(const std::string &option, arg_list_t *argListStr, std::string delimiter);

    bool enableArkTools_ {true};
    std::string stubFile_ {"stub.an"};
    bool enableForceGc_ {true};
    bool forceFullGc_ {true};
    int arkProperties_ = GetDefaultProperties();
    std::string arkBundleName_ = {""};
    uint32_t gcThreadNum_ {7}; // 7: default thread num
    uint32_t longPauseTime_ {40}; // 40: default pause time
    std::string aotOutputFile_ {""};
    std::string targetTriple_ {TARGET_X64};
    uint32_t asmOptLevel_ {2};
    uint32_t relocationMode_ {2}; // 2: default relocation mode
    uint32_t maxNonmovableSpaceCapacity_ {4_MB};
    bool enableAsmInterpreter_ {true};
    bool enableBuiltinsLazy_ {true};
    std::string asmOpcodeDisableRange_ {""};
    AsmInterParsedOption asmInterParsedOption_;
    uint64_t serializerBufferSizeLimit_ {2_GB};
    uint32_t heapSizeLimit_ {512_MB};
    bool enableIC_ {true};
    std::string icuDataPath_ {"default"};
    bool startupTime_ {false};
    std::string compilerLogOpt_ {"none"};
    std::string compilerLogMethods_ {"none"};
    bool compilerLogSnapshot_ {false};
    bool compilerLogTime_ {false};
    bool enableRuntimeStat_ {false};
    bool assertTypes_ {false};
    bool printTypeInfo_ {false};
    bool isWorker_ {false};
    std::string builtinsDTS_ {""};
    bool traceBc_ {false};
    std::string logLevel_ {"error"};
    arg_list_t logDebug_ {{"all"}};
    arg_list_t logInfo_ {{"all"}};
    arg_list_t logWarning_ {{"all"}};
    arg_list_t logError_ {{"all"}};
    arg_list_t logFatal_ {{"all"}};
    arg_list_t logComponents_ {{"all"}};
    bool enableAOT_ {false};
    uint32_t maxAotMethodSize_ {32_KB};
    double typeThreshold_ {-1};
    std::string entryPoint_ {"_GLOBAL::func_main_0"};
    bool mergeAbc_ {false};
    bool enableArrayBoundsCheckElimination_ {false};
    bool enableTypeLowering_ {true};
    bool enableEarlyElimination_ {true};
    bool enableLaterElimination_ {true};
    bool enableValueNumbering_ {true};
    bool enableOptInlining_ {true};
    bool enableOptPGOType_ {true};
    bool enableGlobalTypeInfer_ {false};
    bool enableOptTrackField_ {true};
    uint32_t compilerModuleMethods_ {100};
    uint64_t wasSet_ {0};
    bool enablePrintExecuteTime_ {false};
    bool enablePGOProfiler_ {false};
    bool reportModuleResolvingFailure_ {true};
    uint32_t pgoHotnessThreshold_ {1};
    std::string pgoProfilerPath_ {""};
    bool traceDeopt_ {false};
    uint8_t deoptThreshold_ {10};
    bool stressDeopt_ {false};
    bool optCodeProfiler_ {false};
    bool startGlobalLeakCheck_ {false};
    bool verifyVTable_ {false};
    std::string compilerSelectMethods_ {""};
    std::string compilerSkipMethods_ {""};
    bool traceInline_ {false};
    size_t maxInlineBytecodes_ {45};
    std::string targetCompilerMode_ {""};
    std::string hapPath_ {""};
    uint32_t hapAbcOffset_ {0};
    uint32_t hapAbcSize_ {0};
    bool compilerNoCheck_ {false};
    bool fastAOTCompileMode_ {false};
    bool enableOptLoopPeeling_ {true};
    bool enableOptOnHeapCheck_ {true};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_RUNTIME_OPTIONS_H_
