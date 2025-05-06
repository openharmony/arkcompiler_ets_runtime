/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/compiler/bc_call_signature.h"
#include "ecmascript/mem/mem_common.h"
#include "libpandabase/os/file.h"
#include "ecmascript/base/number_helper.h"

namespace {
constexpr size_t DEFAULT_OPT_LEVEL = 3;  // 3: default opt level
constexpr size_t DEFAULT_REL_MODE = 2;
}  // namespace

// namespace panda {
namespace panda::ecmascript {
using arg_list_t = std::vector<std::string>;
enum ArkProperties {
    DEFAULT = -1, // default value 000'0000'0001'0000'0101'1100 -> 0x105c
    OPTIONAL_LOG = 1,
    GC_STATS_PRINT = 1 << 1,
    PARALLEL_GC = 1 << 2, // default enable
    CONCURRENT_MARK = 1 << 3, // default enable
    CONCURRENT_SWEEP = 1 << 4, // default enable
    THREAD_CHECK = 1 << 5,
    ENABLE_ARKTOOLS = 1 << 6, // default enable
    ENABLE_SNAPSHOT_SERIALIZE = 1 << 7,
    ENABLE_SNAPSHOT_DESERIALIZE = 1 << 8,
    EXCEPTION_BACKTRACE = 1 << 9,
    GLOBAL_OBJECT_LEAK_CHECK = 1 << 10,
    GLOBAL_PRIMITIVE_LEAK_CHECK = 1 << 11,
    ENABLE_IDLE_GC = 1 << 12, // default enable
    CPU_PROFILER_COLD_START_MAIN_THREAD = 1 << 13,
    ENABLE_CPU_PROFILER_VM_TAG = 1 << 14,
    ENABLE_GC_TRACER = 1 << 15,
    CPU_PROFILER_COLD_START_WORKER_THREAD = 1 << 16,
    CPU_PROFILER_ANY_TIME_MAIN_THREAD = 1 << 17,
    CPU_PROFILER_ANY_TIME_WORKER_THREAD = 1 << 18,
    ENABLE_HEAP_VERIFY = 1 << 19,
    ENABLE_MICROJOB_TRACE = 1 << 20,
    // Use DISABLE to adapt to the exsiting ArkProperties in testing scripts.
    DISABLE_SHARED_CONCURRENT_MARK = 1 << 22,
    ENABLE_ESM_TRACE = 1 << 24,
    ENABLE_MODULE_LOG = 1 << 25,
    ENABLE_SERIALIZATION_TIMEOUT_CHECK = 1 << 26,
    ENABLE_PAGETAG_THREAD_ID = 1 << 27,
    ENABLE_MODULE_EXCEPTION = 1 << 29,
    ENABLE_PENDING_CHEAK = 1 << 30,
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
    OPTION_HELP,
    OPTION_ENABLE_ARK_TOOLS,
    OPTION_STUB_FILE,
    OPTION_ENABLE_FORCE_GC,
    OPTION_FORCE_FULL_GC,
    OPTION_ENABLE_FORCE_SHARED_GC_FREQUENCY,
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
    OPTION_COMPILER_LOG_SNAPSHOT,
    OPTION_COMPILER_LOG_TIME,
    OPTION_ENABLE_WORKER,
    OPTION_COMPILER_TRACE_BC,
    OPTION_COMPILER_TRACE_DEOPT,
    OPTION_COMPILER_TRACE_INLINE,
    OPTION_COMPILER_MAX_INLINE_BYTECODES,
    OPTION_COMPILER_DEOPT_THRESHOLD,
    OPTION_COMPILER_STRESS_DEOPT,
    OPTION_COMPILER_OPT_CODE_PROFILER,
    OPTION_COMPILER_OPT_BC_RANGE,
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
    OPTION_ENABLE_CONTEXT,
    OPTION_COMPILER_OPT_TYPE_LOWERING,
    OPTION_COMPILER_OPT_EARLY_ELIMINATION,
    OPTION_COMPILER_OPT_LATER_ELIMINATION,
    OPTION_COMPILER_THERMAL_LEVEL,
    OPTION_COMPILER_OPT_VALUE_NUMBERING,
    OPTION_COMPILER_OPT_INLINING,
    OPTION_COMPILER_OPT_PGOTYPE,
    OPTION_COMPILER_OPT_TRACK_FIELD,
    OPTION_COMPILER_PGO_PROFILER_PATH,
    OPTION_COMPILER_PGO_HOTNESS_THRESHOLD,
    OPTION_COMPILER_PGO_SAVE_MIN_INTERVAL,
    OPTION_ENABLE_PGO_PROFILER,
    OPTION_INVALID,
    OPTION_SPLIT_ONE,
    OPTION_PRINT_EXECUTE_TIME,
    OPTION_COMPILER_DEVICE_STATE,
    OPTION_COMPILER_VERIFY_VTABLE,
    OPTION_COMPILER_SELECT_METHODS,
    OPTION_COMPILER_SKIP_METHODS,
    OPTION_TARGET_COMPILER_MODE,
    OPTION_HAP_PATH,
    OPTION_HAP_ABC_OFFSET,
    OPTION_HAP_ABC_SIZE,
    OPTION_COMPILER_NOCHECK,
    OPTION_COMPILER_PIPELINE_HOST_AOT,
    OPTION_FAST_AOT_COMPILE_MODE,
    OPTION_COMPILER_OPT_LOOP_PEELING,
    OPTION_COMPILER_OPT_ON_HEAP_CHECK,
    OPTION_COMPILER_PKG_INFO,
    OPTION_COMPILER_EXTERNAL_PKG_INFO,
    OPTION_COMPILER_ENABLE_EXTERNAL_PKG,
    OPTION_COMPILER_FRAMEWORK_ABC_PATH,
    OPTION_COMPILER_OPT_ARRAY_BOUNDS_CHECK_ELIMINATION,
    OPTION_COMPILER_OPT_LOOP_INVARIANT_CODE_MOTION,
    OPTION_COMPILER_OPT_CONSTANT_FOLDING,
    OPTION_COMPILER_ENABLE_LEXENV_SPECIALIZATION,
    OPTION_COMPILER_TRACE_INSTRUCTION_COMBINE,
    OPTION_COMPILER_TRACE_VALUE_NUMBERING,
    OPTION_COMPILER_OPT_INSTRUCTIONE_COMBINE,
    OPTION_COMPILER_OPT_NEW_VALUE_NUMBERING,
    OPTION_COMPILER_ENABLE_NATIVE_INLINE,
    OPTION_COMPILER_ENABLE_LOWERING_BUILTIN,
    OPTION_COMPILER_ENABLE_LITECG,
    OPTION_COMPILER_ENABLE_JIT,
    OPTION_COMPILER_ENABLE_OSR,
    OPTION_COMPILER_JIT_HOTNESS_THRESHOLD,
    OPTION_COMPILER_JIT_CALL_THRESHOLD,
    OPTION_COMPILER_OSR_HOTNESS_THRESHOLD,
    OPTION_COMPILER_FORCE_JIT_COMPILE_MAIN,
    OPTION_COMPILER_TRACE_JIT,
    OPTION_COMPILER_ENABLE_JIT_PGO,
    OPTION_COMPILER_ENABLE_AOT_PGO,
    OPTION_COMPILER_ENABLE_FRAMEWORK_AOT,
    OPTION_COMPILER_ENABLE_PROPFILE_DUMP,
    OPTION_ENABLE_MUTANT_ARRAY,
    OPTION_ENABLE_ELEMENTS_KIND,
    OPTION_COMPILER_TYPED_OP_PROFILER,
    OPTION_COMPILER_OPT_BRANCH_PROFILING,
    OPTION_TEST_ASSERT,
    OPTION_COMPILER_METHODS_RANGE,
    OPTION_COMPILER_CODEGEN_OPT,
    OPTION_COMPILER_OPT_BC_RANGE_HELP,
    OPTION_COMPILER_MEMORY_ANALYSIS,
    OPTION_COMPILER_CHECK_PGO_VERSION,
    OPTION_COMPILER_OPT_ESCAPE_ANALYSIS,
    OPTION_COMPILER_TRACE_ESCAPE_ANALYSIS,
    OPTION_ENABLE_JITFORT,
    OPTION_CODESIGN_DISABLE,
    OPTION_ENABLE_ASYNC_COPYTOFORT,
    OPTION_LAST,
    OPTION_COMPILER_OPT_INDUCTION_VARIABLE,
    OPTION_COMPILER_TRACE_INDUCTION_VARIABLE,
    OPTION_COMPILER_ENABLE_BASELINEJIT,
    OPTION_COMPILER_BASELINEJIT_HOTNESS_THRESHOLD,
    OPTION_COMPILER_FORCE_BASELINEJIT_COMPILE_MAIN,
    OPTION_ENABLE_AOT_CRASH_ESCAPE,
    OPTION_COMPILER_ENABLE_JIT_FAST_COMPILE,
    OPTION_SPLIT_TWO,
    OPTION_COMPILER_ENABLE_MEGA_IC,
    OPTION_COMPILER_BASELINE_PGO,
    OPTION_ASYNC_LOAD_ABC,
    OPTION_ASYNC_LOAD_ABC_TEST,
    OPTION_PGO_TRACE,
    OPTION_COMPILER_PGO_FORCE_DUMP,
    OPTION_COMPILER_ENABLE_CONCURRENT,
    OPTION_COMPILER_ENABLE_STORE_BARRIER_OPT,
    OPTION_COMPILER_ENABLE_LDOBJVALUE_OPT,
    OPTION_COMPILER_OPT_STRING,
    OPTION_OPEN_ARK_TOOLS,
    OPTION_COMPILER_OPT_FRAME_STATE_ELIMINATION,
    OPTION_COMPILER_ENABLE_PGO_SPACE,
    OPTION_ENABLE_INLINE_PROPERTY_OPTIMIZATION,
    OPTION_COMPILER_ENABLE_AOT_CODE_COMMENT,
    OPTION_ENABLE_JIT_VERIFY_PASS,
    OPTION_COMPILER_AN_FILE_MAX_SIZE,
    OPTION_COMPILER_TRACE_BUILTINS,
    OPTION_ENABLE_HEAP_VERIFY,
    OPTION_COMPILER_ENABLE_DFX_HISYS_EVENT,
    OPTION_ENABLE_LOADING_STUBS_LOG,
    OPTION_COMPILER_ENABLE_MERGE_POLY,
};
static_assert(OPTION_INVALID == 63); // Placeholder for invalid options
static_assert(OPTION_SPLIT_ONE == 64); // add new option at the bottom, DO NOT modify this value
static_assert(OPTION_SPLIT_TWO == 128); // add new option at the bottom, DO NOT modify this value

class PUBLIC_API JSRuntimeOptions {
public:
    JSRuntimeOptions() {}
    ~JSRuntimeOptions() = default;
    DEFAULT_COPY_SEMANTIC(JSRuntimeOptions);
    DEFAULT_MOVE_SEMANTIC(JSRuntimeOptions);

    bool ParseCommand(const int argc, const char** argv);
    bool SetDefaultValue(char* argv);

    bool EnableArkTools() const
    {
        return (enableArkTools_) || ((static_cast<uint32_t>(arkProperties_) & ArkProperties::ENABLE_ARKTOOLS) != 0);
    }

    void SetEnableArkTools(bool value)
    {
        enableArkTools_ = value;
    }

    bool WasSetEnableArkTools() const
    {
        return WasOptionSet(OPTION_ENABLE_ARK_TOOLS);
    }

    bool IsOpenArkTools() const
    {
        return openArkTools_;
    }

    void SetOpenArkTools(bool value)
    {
        openArkTools_ = value;
    }

    bool WasSetOpenArkTools() const
    {
        return WasOptionSet(OPTION_OPEN_ARK_TOOLS);
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

    void SetCompilerPkgJsonInfo(std::string pkgJsonInfo)
    {
        compilerPkgInfo_ = std::move(pkgJsonInfo);
    }

    const std::string& GetCompilerPkgJsonInfo() const
    {
        return compilerPkgInfo_;
    }

    void SetCompilerExternalPkgJsonInfo(std::string pkgJsonInfo)
    {
        compilerExternalPkgInfo_ = std::move(pkgJsonInfo);
    }

    const std::string& GetCompilerExternalPkgJsonInfo() const
    {
        return compilerExternalPkgInfo_;
    }

    void SetCompilerEnableExternalPkg(bool compilerEnableExternalPkg)
    {
        compilerEnableExternalPkg_ = compilerEnableExternalPkg;
    }

    bool GetCompilerEnableExternalPkg() const
    {
        return compilerEnableExternalPkg_;
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

    bool EnableLoadingStubsLog() const
    {
        return enableLoadingStubsLog_;
    }

    void SetEnableLoadingStubsLog(bool value)
    {
        enableLoadingStubsLog_ = value;
    }

    bool ForceFullGC() const
    {
        return forceFullGc_;
    }

    void SetForceFullGC(bool value)
    {
        forceFullGc_ = value;
    }

    void SetForceSharedGCFrequency(size_t frequency)
    {
        forceSharedGc_ = frequency;
    }

    uint32_t GetForceSharedGCFrequency() const
    {
        return forceSharedGc_;
    }

    void SetEnableHeapVerify(bool value)
    {
        enableHeapVerify_ = value;
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

    void SetTraceLoadStoreBundleName(std::string bundleName)
    {
        if (bundleName != "") {
            traceLoadStoreBundleName_ = bundleName;
        }
    }

    void SetMemConfigProperty(std::string configProperty)
    {
        if (configProperty != "") {
            std::string key;
            std::string value;
            for (char c: configProperty) {
                if (isdigit(c)) {
                    value += c;
                } else {
                    key += c;
                }
            }
            if (key == "jsHeap") {
                heapSize_ = static_cast<size_t>(stoi(value)) * 1_MB;
            }
            if (key == "openArkTools") {
                openArkTools_ = true;
            }
        }
    }

    size_t GetHeapSize() const
    {
        return heapSize_;
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

    std::string GetTraceLoadStoreBundleName() const
    {
        return traceLoadStoreBundleName_;
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

    bool EnableSharedConcurrentMark() const
    {
        // Use DISABLE to adapt to the exsiting ArkProperties in testing scripts.
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::DISABLE_SHARED_CONCURRENT_MARK) == 0;
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
        return false;
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

    bool EnableHeapVerify() const
    {
        return enableHeapVerify_ && (static_cast<uint32_t>(arkProperties_) & ArkProperties::ENABLE_HEAP_VERIFY) != 0;
    }

    bool EnableMicroJobTrace() const
    {
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::ENABLE_MICROJOB_TRACE) != 0;
    }

    bool EnableESMTrace() const
    {
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::ENABLE_ESM_TRACE) != 0;
    }

    bool EnableModuleLog() const
    {
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::ENABLE_MODULE_LOG) != 0;
    }

    bool EnableSerializationTimeoutCheck() const
    {
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::ENABLE_SERIALIZATION_TIMEOUT_CHECK) != 0;
    }

    bool EnablePageTagThreadId() const
    {
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::ENABLE_PAGETAG_THREAD_ID) != 0;
    }

    bool EnableModuleException() const
    {
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::ENABLE_MODULE_EXCEPTION) != 0;
    }

    bool EnablePendingCheak() const
    {
        return (static_cast<uint32_t>(arkProperties_) & ArkProperties::ENABLE_PENDING_CHEAK) != 0;
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

    void SetDisableCodeSign(bool value)
    {
        disableCodeSign_ = value;
    }

    bool GetDisableCodeSign() const
    {
        return disableCodeSign_;
    }

    void SetEnableJitFort(bool value)
    {
        enableJitFort_ = value;
    }

    bool GetEnableJitFort() const
    {
        return enableJitFort_;
    }

    void SetEnableAsyncCopyToFort(bool value)
    {
        enableAsyncCopyToFort_ = value;
    }

    bool GetEnableAsyncCopyToFort() const
    {
        return enableAsyncCopyToFort_;
    }

    void SetLargeHeap(bool largeHeap)
    {
        enableLargeHeap_ = largeHeap;
    }

    bool GetLargeHeap() const
    {
        return enableLargeHeap_;
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
            int64_t inputStart;
            int64_t inputEnd;
            if (!base::NumberHelper::StringToInt64(strStart, inputStart)) {
                inputStart = 0;
                LOG_ECMA_IF(!strStart.empty(), INFO) << "when get start, strStart is " << strStart;
            }
            if (!base::NumberHelper::StringToInt64(strEnd, inputEnd)) {
                inputEnd = kungfu::BYTECODE_STUB_END_ID;
                LOG_ECMA_IF(!strEnd.empty(), INFO) << "when get end, strEnd is " << strEnd;
            }
            int start = static_cast<int>(inputStart);
            int end = static_cast<int>(inputEnd);
            if (start >= 0 && start < kungfu::BytecodeStubCSigns::NUM_OF_ALL_NORMAL_STUBS && end >= 0 &&
                end < kungfu::BytecodeStubCSigns::NUM_OF_ALL_NORMAL_STUBS && start <= end) {
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
        return WasOptionSet(OPTION_COMPILER_LOG_OPT) && GetCompilerLogOption().find("none") == std::string::npos;
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
        return WasOptionSet(OPTION_COMPILER_LOG_METHODS) && GetCompilerLogOption().find("none") == std::string::npos &&
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

    void SetIsRestrictedWorker(bool isRestrictedWorker)
    {
        isRestrictedWorker_ = isRestrictedWorker;
    }

    bool IsWorker() const
    {
        return isWorker_;
    }

    bool IsRestrictedWorker() const
    {
        return isRestrictedWorker_;
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

    void SetEnableContext(bool value)
    {
        enableContext_ = value;
    }

    bool IsEnableContext() const
    {
        return enableContext_;
    }

    void SetEnablePrintExecuteTime(bool value)
    {
        enablePrintExecuteTime_ = value;
    }

    bool IsEnablePrintExecuteTime() const
    {
        return enablePrintExecuteTime_;
    }

    void SetEnableMutantArray(bool value)
    {
        enableMutantArray_ = value;
    }

    bool IsEnableMutantArray() const
    {
        return enableMutantArray_;
    }

    void SetEnableElementsKind(bool value)
    {
        enableElementsKind_ = value;
    }

    bool IsEnableElementsKind() const
    {
        return enableElementsKind_;
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

    uint32_t GetPGOSaveMinInterval() const
    {
        return pgoSaveMinInterval_;
    }

    void SetPGOSaveMinInterval(uint32_t value)
    {
        pgoSaveMinInterval_ = value;
    }

    std::string GetPGOProfilerPath() const
    {
        return pgoProfilerPath_;
    }

    void SetEnableBaselinePgo(bool value)
    {
        enableBaselinePgo_ = value;
    }

    bool IsEnableBaselinePgo() const
    {
        return enableBaselinePgo_;
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

    bool IsEnableFrameStateElimination() const
    {
        return enableFrameStateElimination_;
    }

    void SetEnableFrameStateElimination(bool value)
    {
        enableFrameStateElimination_ = value;
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

    void SetEnableInstrcutionCombine(bool value)
    {
        enableInstrcutionCombine = value;
    }

    bool IsEnableInstrcutionCombine() const
    {
        return enableInstrcutionCombine;
    }

    void SetEnableValueNumbering(bool value)
    {
        enableValueNumbering_ = value;
    }

    bool IsEnableValueNumbering() const
    {
        return enableValueNumbering_;
    }

    void SetEnableJIT(bool value)
    {
        enableFastJIT_ = value;
    }

    bool IsEnableJIT() const
    {
        return enableFastJIT_;
    }

    void SetEnableDFXHiSysEvent(bool value)
    {
        enableDFXHiSysEvent_ = value;
    }

    bool IsEnableDFXHiSysEvent() const
    {
        return enableDFXHiSysEvent_;
    }

    void SetEnableAPPJIT(bool value)
    {
        enableAPPJIT_ = value;
    }

    bool IsEnableAPPJIT() const
    {
        return enableAPPJIT_;
    }

    void SetEnableJitFrame(bool value)
    {
        enableJitFrame_ = value;
    }

    bool IsEnableJitFrame() const
    {
        return enableJitFrame_;
    }

    bool IsEnableJitDfxDump() const
    {
        return isEnableJitDfxDump_;
    }

    void SetEnableJitDfxDump(bool value)
    {
        isEnableJitDfxDump_ = value;
    }

    void SetEnableOSR(bool value)
    {
        enableOSR_ = value;
    }

    bool IsEnableOSR() const
    {
        return enableOSR_;
    }

    void SetJitHotnessThreshold(uint16_t value)
    {
        jitHotnessThreshold_ = value;
    }

    uint16_t GetJitHotnessThreshold() const
    {
        return jitHotnessThreshold_;
    }

    void SetJitCallThreshold(uint16_t value)
    {
        jitCallThreshold_ = value;
    }

    uint16_t GetJitCallThreshold() const
    {
        return jitCallThreshold_;
    }

    void SetOsrHotnessThreshold(uint16_t value)
    {
        osrHotnessThreshold_ = value;
    }

    uint16_t GetOsrHotnessThreshold() const
    {
        return osrHotnessThreshold_;
    }

    void SetForceJitCompileMain(bool value)
    {
        forceJitCompileMain_ = value;
    }

    bool IsEnableForceJitCompileMain() const
    {
        return forceJitCompileMain_;
    }

    void SetEnableBaselineJIT(bool value)
    {
        enableBaselineJIT_ = value;
    }

    bool IsEnableBaselineJIT() const
    {
        return enableBaselineJIT_;
    }

    void SetBaselineJitHotnessThreshold(uint16_t value)
    {
        baselineJitHotnessThreshold_ = value;
    }

    uint16_t GetBaselineJitHotnessThreshold() const
    {
        return baselineJitHotnessThreshold_;
    }

    void SetForceBaselineCompileMain(bool value)
    {
        forceBaselineCompileMain_ = value;
    }

    bool IsEnableForceBaselineCompileMain() const
    {
        return forceBaselineCompileMain_;
    }

    void SetEnableNewValueNumbering(bool value)
    {
        enableNewValueNumbering_ = value;
    }

    bool IsEnableNewValueNumbering() const
    {
        return enableNewValueNumbering_;
    }

    void SetEnableOptString(bool value)
    {
        enableOptString_ = value;
    }

    bool IsEnableOptString() const
    {
        return enableOptString_;
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

    uint32_t GetCompilerModuleMethods() const
    {
        return compilerModuleMethods_;
    }

    void SetCompilerModuleMethods(uint32_t compilerModuleMethods)
    {
        compilerModuleMethods_ = compilerModuleMethods;
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

    void SetDeviceState(bool value)
    {
        deviceIsScreenOff_ = value;
    }

    bool GetDeviceState() const
    {
        return deviceIsScreenOff_;
    }

    bool WasSetDeviceState() const
    {
        return WasOptionSet(OPTION_COMPILER_DEVICE_STATE);
    }

    void SetThermalLevel(int32_t level)
    {
        deviceThermalLevel_ = level;
    }

    int32_t GetThermalLevel() const
    {
        return deviceThermalLevel_;
    }

    bool WasSetThermalLevel() const
    {
        return WasOptionSet(OPTION_COMPILER_THERMAL_LEVEL);
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

    void SetPGOTrace(bool value)
    {
        pgoTrace_ = value;
    }

    bool GetPGOTrace() const
    {
        return pgoTrace_;
    }

    void SetTraceInline(bool value)
    {
        traceInline_ = value;
    }

    bool GetTraceInline() const
    {
        return traceInline_;
    }

    bool GetTraceBuiltins() const
    {
        return traceBuiltins_;
    }

    void SetTraceBuiltins(bool value)
    {
        traceBuiltins_ = value;
    }

    void SetTraceValueNumbering(bool value)
    {
        traceValueNumbering_ = value;
    }

    bool GetTraceValueNumbering() const
    {
        return traceValueNumbering_;
    }

    void SetTraceJIT(bool value)
    {
        traceJIT_ = value;
    }

    bool GetTraceJIT() const
    {
        return traceJIT_;
    }

    void SetTraceInstructionCombine(bool value)
    {
        traceInstructionCombine_ = value;
    }

    bool GetTraceInstructionCombine() const
    {
        return traceInstructionCombine_;
    }

    void SetMaxInlineBytecodes(size_t value)
    {
        maxInlineBytecodes_ = value;
    }

    size_t GetMaxInlineBytecodes()
    {
        return maxInlineBytecodes_;
    }

    void SetCompilerFrameworkAbcPath(std::string frameworkAbcPath)
    {
        frameworkAbcPath_ = std::move(frameworkAbcPath);
    }

    std::string GetCompilerFrameworkAbcPath() const
    {
        return frameworkAbcPath_;
    }

    bool WasSetCompilerFrameworkAbcPath() const
    {
        return WasOptionSet(OPTION_COMPILER_FRAMEWORK_ABC_PATH);
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

    bool IsApplicationCompilation() const
    {
        return IsTargetCompilerMode() || IsCompilerPipelineHostAOT();
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

    void SetOptionsForTargetCompilation();

    void BindCPUCoreForTargetCompilation();

    void SetCompilerPipelineHostAOT(bool value)
    {
        compilerPipelineHostAOT_ = value;
    }

    bool IsCompilerPipelineHostAOT() const
    {
        return compilerPipelineHostAOT_;
    }

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

    void SetEnableOptLoopInvariantCodeMotion(bool value)
    {
        enableOptLoopInvariantCodeMotion_ = value;
    }

    bool IsEnableOptLoopInvariantCodeMotion() const
    {
        return enableOptLoopInvariantCodeMotion_;
    }

    bool IsEnableOptConstantFolding() const
    {
        return enableOptConstantFolding_;
    }

    void SetEnableOptConstantFolding(bool value)
    {
        enableOptConstantFolding_ = value;
    }

    void SetEnableOptOnHeapCheck(bool value)
    {
        enableOptOnHeapCheck_ = value;
    }

    bool IsEnableOptOnHeapCheck() const
    {
        return enableOptOnHeapCheck_;
    }

    bool IsEnableLexenvSpecialization() const
    {
        return enableLexenvSpecialization_;
    }

    void SetEnableLexenvSpecialization(bool value)
    {
        enableLexenvSpecialization_ = value;
    }

    bool IsEnableNativeInline() const
    {
        return enableNativeInline_;
    }

    void SetEnableNativeInline(bool value)
    {
        enableNativeInline_ = value;
    }

    bool IsEnableLoweringBuiltin() const
    {
        return enableLoweringBuiltin_;
    }

    void SetEnableLoweringBuiltin(bool value)
    {
        enableLoweringBuiltin_ = value;
    }

    void SetCompilerEnableLiteCG(bool value)
    {
        enableLiteCG_ = value;
    }

    bool IsCompilerEnableLiteCG() const
    {
        return enableLiteCG_;
    }

    void SetTypedOpProfiler(bool value)
    {
        enableTypedOpProfiler_ = value;
    }

    bool GetTypedOpProfiler() const
    {
        return enableTypedOpProfiler_;
    }

    bool IsEnableBranchProfiling() const
    {
        return enableBranchProfiling_;
    }

    void SetEnableBranchProfiling(bool value)
    {
        enableBranchProfiling_ = value;
    }

    void SetTestAssert(bool value)
    {
        testAssert_ = value;
    }

    bool GetTestAssert() const
    {
        return testAssert_;
    }

    void SetCompilerMethodsRange(arg_list_t* argListStr)
    {
        compileMethodsRange_.first = std::stoull((*argListStr)[0]);
        compileMethodsRange_.second = std::stoull((*argListStr)[1]);
    }

    const std::pair<uint32_t, uint32_t>& GetCompilerMethodsRange() const
    {
        return compileMethodsRange_;
    }

    void SetCompilerCodegenOptions(arg_list_t argListStr)
    {
        compileCodegenOption_ = std::move(argListStr);
    }

    const arg_list_t& GetCompilerCodegenOptions() const
    {
        return compileCodegenOption_;
    }

    void SetOptCodeRange(std::string value)
    {
        optBCRange_ = std::move(value);
    }

    std::string GetOptCodeRange() const
    {
        return optBCRange_;
    }

    void SetEnableEscapeAnalysis(bool value)
    {
        enableEscapeAnalysis_ = value;
    }

    bool IsEnableEscapeAnalysis() const
    {
        return enableEscapeAnalysis_;
    }

    void SetEnableTraceEscapeAnalysis(bool value)
    {
        traceEscapeAnalysis_ = value;
    }

    bool GetTraceEscapeAnalysis() const
    {
        return traceEscapeAnalysis_;
    }

    void SetEnableInductionVariableAnalysis(bool value)
    {
        enableInductionVariableAnalysis_ = value;
    }

    bool IsEnableInductionVariableAnalysis() const
    {
        return enableInductionVariableAnalysis_;
    }

    void SetEnableTraceInductionVariableAnalysis(bool value)
    {
        traceInductionVariableAnalysis_ = value;
    }

    bool GetTraceInductionVariableAnalysis() const
    {
        return traceInductionVariableAnalysis_;
    }

    void SetEnableMemoryAnalysis(bool value)
    {
        enableMemoryAnalysis_ = value;
    }

    bool IsEnableMemoryAnalysis() const
    {
        return enableMemoryAnalysis_;
    }

    void SetCheckPgoVersion(bool value)
    {
        checkPgoVersion_ = value;
    }

    bool IsCheckPgoVersion() const
    {
        return checkPgoVersion_;
    }

    void SetEnableJITPGO(bool value)
    {
        enableJITPGO_ = value;
    }

    bool IsEnableJITPGO() const
    {
        return enableJITPGO_;
    }

    void SetEnableProfileDump(bool value)
    {
        enableProfileDump_ = value;
    }

    bool IsEnableProfileDump() const
    {
        return enableProfileDump_;
    }

    void SetEnableAOTPGO(bool value)
    {
        enableAOTPGO_ = value;
    }

    bool IsEnableAOTPGO() const
    {
        return enableAOTPGO_;
    }

    void SetEnableJitFastCompile(bool value)
    {
        enableJitFastCompile_ = value;
    }

    bool IsEnableJitFastCompile() const
    {
        return enableJitFastCompile_;
    }
    
    void SetEnableMegaIC(bool value)
    {
        enableMegaIC_ = value;
        isMegaICInitialized = true;
    }

    bool IsEnableMegaIC() const
    {
        return enableMegaIC_ && IsEnableJIT();
    }

    bool IsMegaICInitialized() const
    {
        return isMegaICInitialized;
    }

    void SetEnableFrameworkAOT(bool value)
    {
        enableFrameworkAOT_ = value;
    }

    bool IsEnableFrameworkAOT() const
    {
        return enableFrameworkAOT_;
    }

    void SetAsyncLoadAbc(bool value)
    {
        asyncLoadAbc_ = value;
    }

    bool IsAsyncLoadAbc() const
    {
        return asyncLoadAbc_;
    }

    void SetAsyncLoadAbcTest(bool value)
    {
        asyncLoadAbcTest_ = value;
    }

    bool IsAsyncLoadAbcTest() const
    {
        return asyncLoadAbcTest_;
    }

    bool IsPgoForceDump() const
    {
        return forceDump_;
    }

    void SetPgoForceDump(bool value)
    {
        forceDump_ = value;
    }

    void SetConcurrentCompile(bool value)
    {
        concurrentCompile = value;
    }

    bool IsConcurrentCompile() const
    {
        return concurrentCompile;
    }

    void SetStoreBarrierOpt(bool value)
    {
        storeBarrierOpt_ = value;
    }

    bool IsStoreBarrierOpt() const
    {
        return storeBarrierOpt_;
    }

    void SetLdObjValueOpt(bool value)
    {
        enableLdObjValueOpt_ = value;
    }

    bool IsLdObjValueOpt() const
    {
        return enableLdObjValueOpt_;
    }

    void SetAOTHasException(bool value)
    {
        aotHasException_ = value;
    }

    bool GetAOTHasException() const
    {
        return aotHasException_;
    }

    void SetCompilerEnablePgoSpace(bool value)
    {
        enablePgoSpace_ = value;
    }

    bool IsCompilerEnablePgoSpace() const
    {
        return enablePgoSpace_;
    }

    void SetEnableAotCodeComment(bool value)
    {
        enableAotCodeComment_ = value;
    }

    bool IsEnableAotCodeComment() const
    {
        return enableAotCodeComment_;
    }

    void SetEnableInlinePropertyOptimization(bool value)
    {
        enableInlinePropertyOptimization_ = value;
    }

    bool IsEnableInlinePropertyOptimization() const
    {
        return enableInlinePropertyOptimization_;
    }

    void SetEnableJitVerifyPass(bool value)
    {
        enableJitVerifyPass_ = value;
    }

    bool IsEnableJitVerifyPass() const
    {
        return enableJitVerifyPass_;
    }

    void SetCompilerAnFileMaxByteSize(uint64_t value)
    {
        CompilerAnFileMaxByteSize_ = value;
    }

    uint64_t GetCompilerAnFileMaxByteSize() const
    {
        return CompilerAnFileMaxByteSize_;
    }

    bool IsCompilerAnFileMaxByteSizeDefault() const
    {
        return CompilerAnFileMaxByteSize_ == 0;
    }

    void SetEnableMergePoly(bool value)
    {
        enableMergePoly_ = value;
    }

    bool IsEnableMergePoly() const
    {
        return enableMergePoly_;
    }

public:
    static constexpr int32_t MAX_APP_COMPILE_METHOD_SIZE = 4_KB;

private:

    static bool StartsWith(const std::string& haystack, const std::string& needle)
    {
        return std::equal(needle.begin(), needle.end(), haystack.begin());
    }

    void WasSet(int option)
    {
        if (option < OPTION_SPLIT_ONE) {
            wasSetPartOne_ |= (1ULL << static_cast<uint64_t>(option));
        } else if (option < OPTION_SPLIT_TWO) {
            wasSetPartTwo_ |= (1ULL << static_cast<uint64_t>(option - OPTION_SPLIT_ONE));
        } else {
            wasSetPartThree_ |= (1ULL << static_cast<uint64_t>(option - OPTION_SPLIT_TWO));
        }
    }

    bool WasOptionSet(int option) const
    {
        if (option < OPTION_SPLIT_ONE) {
            return ((1ULL << static_cast<uint64_t>(option)) & wasSetPartOne_) != 0;
        } else if (option < OPTION_SPLIT_TWO) {
            return ((1ULL << static_cast<uint64_t>(option - OPTION_SPLIT_ONE)) & wasSetPartTwo_) != 0;
        }
        return ((1ULL << static_cast<uint64_t>(option - OPTION_SPLIT_TWO)) & wasSetPartThree_) != 0;
    }

    bool ParseBoolParam(bool* argBool);
    bool ParseDoubleParam(const std::string& option, double* argDouble);
    bool ParseIntParam(const std::string& option, int* argInt);
    bool ParseUint32Param(const std::string& option, uint32_t* argUInt32);
    bool ParseUint64Param(const std::string& option, uint64_t* argUInt64);
    void ParseListArgParam(const std::string& option, arg_list_t* argListStr, std::string delimiter);

    bool enableArkTools_ {true};
    bool openArkTools_ {false};
    std::string stubFile_ {"stub.an"};
    std::string compilerPkgInfo_ {};
    std::string compilerExternalPkgInfo_ {};
    bool compilerEnableExternalPkg_ {true};
    bool enableForceGc_ {true};
    bool enableLoadingStubsLog_ {false};
    bool forceFullGc_ {true};
    bool enableHeapVerify_ {true};
    uint32_t forceSharedGc_ {1};
    int32_t deviceThermalLevel_ {0};
    int arkProperties_ = GetDefaultProperties();
    std::string arkBundleName_ = {""};
    std::string traceLoadStoreBundleName_ = {""};
    size_t heapSize_ = {0};
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
    bool isWorker_ {false};
    bool isRestrictedWorker_ {false};
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
    bool enableFrameStateElimination_ {true};
    bool enableTypeLowering_ {true};
    bool enableEarlyElimination_ {true};
    bool enableLaterElimination_ {true};
    bool enableValueNumbering_ {true};
    bool enableOptString_ {true};
    bool enableMutantArray_ {false};
#if ENABLE_NEXT_OPTIMIZATION
    bool enableElementsKind_ {false};
#else
    bool enableElementsKind_ {false};
#endif
    bool enableInstrcutionCombine {true};
    bool enableNewValueNumbering_ {true};
    bool enableOptInlining_ {true};
    bool enableOptPGOType_ {true};
    bool enableFastJIT_ {false};
    bool enableDFXHiSysEvent_ {true};
    bool enableAPPJIT_ {false};
    bool isEnableJitDfxDump_ {false};
    bool enableOSR_ {false};
    uint16_t jitHotnessThreshold_ {2};
    uint16_t jitCallThreshold_ {1};
    uint16_t osrHotnessThreshold_ {2};
    bool forceJitCompileMain_ {false};
    bool enableBaselineJIT_ {false};
    uint16_t baselineJitHotnessThreshold_ {1};
    bool forceBaselineCompileMain_ {false};
    bool enableOptTrackField_ {true};
    uint32_t compilerModuleMethods_ {100};
    uint64_t wasSetPartOne_ {0};
    uint64_t wasSetPartTwo_ {0};
    uint64_t wasSetPartThree_ {0};
    bool enableContext_ {false};
    bool enablePrintExecuteTime_ {false};
    bool enablePGOProfiler_ {false};
    bool enableJITPGO_ {true};
    bool enableAOTPGO_ {true};
    bool enableProfileDump_ {true};
    bool enableFrameworkAOT_ {false};
    uint32_t pgoHotnessThreshold_ {1};
    std::string pgoProfilerPath_ {""};
    uint32_t pgoSaveMinInterval_ {30};
    bool traceDeopt_ {false};
    uint8_t deoptThreshold_ {10};
    bool stressDeopt_ {false};
    bool deviceIsScreenOff_ {true};
    bool optCodeProfiler_ {false};
    bool startGlobalLeakCheck_ {false};
    bool verifyVTable_ {false};
    std::string compilerSelectMethods_ {""};
    std::string compilerSkipMethods_ {""};
    bool pgoTrace_ {false};
    bool traceInline_ {false};
    bool traceBuiltins_ {false};
    bool traceJIT_ {false};
    bool traceValueNumbering_ {false};
    bool traceInstructionCombine_ {false};
    bool compilerPipelineHostAOT_ {false};
    size_t maxInlineBytecodes_ {45};
    std::string targetCompilerMode_ {""};
    std::string frameworkAbcPath_ {""};
    std::string hapPath_ {""};
    uint32_t hapAbcOffset_ {0};
    uint32_t hapAbcSize_ {0};
    std::string optBCRange_ {""};
    bool compilerNoCheck_ {false};
    bool fastAOTCompileMode_ {false};
    bool enableOptLoopPeeling_ {true};
    bool enableOptConstantFolding_ {true};
    bool enableOptOnHeapCheck_ {true};
    bool enableOptLoopInvariantCodeMotion_ {false};
    bool enableLexenvSpecialization_ {false};
    bool enableNativeInline_ {true};
    bool enableLoweringBuiltin_ {true};
    bool enableLiteCG_ {false};
    bool enableTypedOpProfiler_ {false};
    bool enableBranchProfiling_ {true};
    bool enablePgoSpace_ {false};
    bool enableAotCodeComment_ {false};
    bool testAssert_ {false};
    std::pair<uint32_t, uint32_t> compileMethodsRange_ {0, UINT32_MAX};
    arg_list_t compileCodegenOption_ {{""}};
    bool enableEscapeAnalysis_ {false};
    bool traceEscapeAnalysis_ {false};
    bool enableInductionVariableAnalysis_ {false};
    bool traceInductionVariableAnalysis_ {false};
    bool enableMemoryAnalysis_ {true};
    bool checkPgoVersion_ {false};
    bool enableJitFastCompile_ {false};
    bool enableJitFrame_ {false};
    bool enableMegaIC_ {false};
    bool isMegaICInitialized {false};
    bool disableCodeSign_ {true};
    bool enableJitFort_ {true};
    bool enableAsyncCopyToFort_ {true};
    bool enableBaselinePgo_ {false};
    bool enableLargeHeap_ {false};
    bool asyncLoadAbc_ {true};
    bool asyncLoadAbcTest_ {false};
    bool forceDump_ {true};
    bool concurrentCompile {true};
    bool aotHasException_ {false};
    bool enableInlinePropertyOptimization_ {NEXT_OPTIMIZATION_BOOL};
    bool storeBarrierOpt_ {true};
    bool enableLdObjValueOpt_ {true};
    uint64_t CompilerAnFileMaxByteSize_ {0_MB};
    bool enableJitVerifyPass_ {true};
    bool enableMergePoly_ {true};
};
} // namespace panda::ecmascript

#endif // ECMASCRIPT_JS_RUNTIME_OPTIONS_H_
