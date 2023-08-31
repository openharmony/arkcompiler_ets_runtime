/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include "jsnapi_helper.h"

#include <array>
#include <cstdint>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/base/json_parser.h"
#include "ecmascript/base/json_stringifier.h"
#include "ecmascript/base/path_helper.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/builtins/builtins_object.h"
#include "ecmascript/builtins/builtins_string.h"
#include "ecmascript/builtins/builtins_typedarray.h"
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#endif
#include "ecmascript/byte_array.h"
#include "ecmascript/compiler/aot_file/an_file_data_manager.h"
#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/debugger/js_debugger_manager.h"
#include "ecmascript/ecma_context.h"
#include "ecmascript/ecma_global_storage.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_dataview.h"
#include "ecmascript/js_file_path.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/js_serializer.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_tagged_number.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/jspandafile/debug_info_extractor.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/log.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/mem_map_allocator.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/patch/quick_fix_manager.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/regexp/regexp_parser.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/js_weak_container.h"
#ifdef ARK_SUPPORT_INTL
#include "ecmascript/js_bigint.h"
#include "ecmascript/js_collator.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_number_format.h"
#endif

#include "ohos/init_data.h"

#include "libpandabase/os/mutex.h"

#if defined(ECMASCRIPT_SUPPORT_DEBUGGER) && defined(PANDA_TARGET_IOS)
namespace OHOS::ArkCompiler::Toolchain {
using DebuggerPostTask = std::function<void(std::function<void()> &&)>;
extern "C" {
    bool StartDebug(const std::string& componentName, void* vm, bool isDebugMode, int32_t instanceId,
        const DebuggerPostTask& debuggerPostTask, int port);
    void StopDebug(const std::string& componentName);
}
} // namespace OHOS::ArkCompiler::Toolchain
const std::string DEBUGGER_NAME = "PandaDebugger";
#endif

namespace panda {
using ecmascript::ECMAObject;
using ecmascript::EcmaString;
using ecmascript::EcmaStringAccessor;
using ecmascript::ErrorType;
using ecmascript::FastRuntimeStub;
using ecmascript::GlobalEnv;
using ecmascript::GlobalEnvConstants;
using ecmascript::EcmaRuntimeCallInfo;
using ecmascript::JSArray;
using ecmascript::JSArrayBuffer;
using ecmascript::JSDataView;
using ecmascript::ByteArray;
using ecmascript::JSDate;
using ecmascript::JSFunction;
using ecmascript::JSFunctionBase;
using ecmascript::JSHClass;
using ecmascript::JSMap;
using ecmascript::Method;
using ecmascript::JSNativePointer;
using ecmascript::JSObject;
using ecmascript::JSPandaFile;
using ecmascript::JSPandaFileManager;
using ecmascript::JSPrimitiveRef;
using ecmascript::JSPromise;
using ecmascript::JSRegExp;
using ecmascript::JSSerializer;
using ecmascript::JSSet;
using ecmascript::JSSymbol;
using ecmascript::JSTaggedNumber;
using ecmascript::JSTaggedType;
using ecmascript::JSTaggedValue;
using ecmascript::JSThread;
using ecmascript::LinkedHashMap;
using ecmascript::LinkedHashSet;
using ecmascript::ObjectFactory;
using ecmascript::PromiseCapability;
using ecmascript::PropertyDescriptor;
using ecmascript::OperationResult;
using ecmascript::Region;
using ecmascript::TaggedArray;
using ecmascript::JSTypedArray;
using ecmascript::base::BuiltinsBase;
using ecmascript::builtins::BuiltinsObject;
using ecmascript::base::JsonParser;
using ecmascript::base::JsonStringifier;
using ecmascript::base::StringHelper;
using ecmascript::base::TypedArrayHelper;
using ecmascript::job::MicroJobQueue;
using ecmascript::job::QueueType;
using ecmascript::JSRuntimeOptions;
using ecmascript::BigInt;
using ecmascript::MemMapAllocator;
using ecmascript::JSMapIterator;
using ecmascript::JSSetIterator;
using ecmascript::IterationKind;
using ecmascript::JSGeneratorState;
using ecmascript::JSIterator;
using ecmascript::JSGeneratorFunction;
using ecmascript::JSGeneratorObject;
using ecmascript::GeneratorContext;
using ecmascript::JSProxy;
#ifdef ARK_SUPPORT_INTL
using ecmascript::JSCollator;
using ecmascript::JSDateTimeFormat;
using ecmascript::JSNumberFormat;
#endif
using ecmascript::RegExpParser;
using ecmascript::DebugInfoExtractor;
using ecmascript::PatchErrorCode;
using ecmascript::base::NumberHelper;
using ecmascript::Log;
using ecmascript::EcmaContext;
using ecmascript::JSWeakMap;
using ecmascript::JSWeakSet;
template<typename T>
using JSHandle = ecmascript::JSHandle<T>;

template<typename T>
using JSMutableHandle = ecmascript::JSMutableHandle<T>;

using PathHelper = ecmascript::base::PathHelper;
using ModulePathHelper = ecmascript::ModulePathHelper;

namespace {
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
constexpr std::string_view ENTRY_POINTER = "_GLOBAL::func_main_0";
}
int JSNApi::vmCount_ = 0;
bool JSNApi::initialize_ = false;
static os::memory::Mutex *mutex = new panda::os::memory::Mutex();
#define XPM_PROC_PREFIX "/proc/"
#define XPM_PROC_SUFFIX "/xpm_region"
#define XPM_PROC_LENGTH 50

bool JSNApi::CheckSecureMem(uintptr_t mem)
{
    static bool hasOpen = false;
    static uintptr_t secureMemStart = 0;
    static uintptr_t secureMemEnd = 0;
    if (!hasOpen) {
        char procPath[XPM_PROC_LENGTH] = {0};
        pid_t pid = getpid();
        LOG_ECMA(DEBUG) << "Check secure memory in : " << pid << " with mem: " << std::hex << mem;
        if (sprintf_s(procPath, XPM_PROC_LENGTH, "%s%d%s", XPM_PROC_PREFIX, pid, XPM_PROC_SUFFIX) <= 0) {
            LOG_ECMA(ERROR) << "sprintf proc path failed";
            return false;
        }
        int fd = open(procPath, O_RDONLY);
        if (fd < 0) {
            LOG_ECMA(ERROR) << "Can not open xpm proc file, do not check secure memory anymore.";
            // No verification is performed when a file fails to be opened.
            hasOpen = true;
            return true;
        }

        char xpmValidateRegion[XPM_PROC_LENGTH] = {0};
        int ret = read(fd, xpmValidateRegion, sizeof(xpmValidateRegion));
        if (ret <= 0) {
            LOG_ECMA(ERROR) << "Read xpm proc file failed";
            close(fd);
            return false;
        }
        close(fd);

        if (sscanf_s(xpmValidateRegion, "%lx-%lx", &secureMemStart, &secureMemEnd) <= 0) {
            LOG_ECMA(ERROR) << "sscanf_s xpm validate region failed";
            return false;
        }
        // The check is not performed when the file is already opened.
        hasOpen = true;
    }

    // xpm proc does not exist, the read value is 0, and the check is not performed.
    if (secureMemStart == 0 && secureMemEnd == 0) {
        LOG_ECMA(ERROR) << "Secure memory check: xpm proc does not exist, do not check secure memory anymore.";
        return true;
    }

    LOG_ECMA(DEBUG) << "Secure memory check in memory start: " << std::hex << secureMemStart
                   << " memory end: " << secureMemEnd;
    if (mem < secureMemStart || mem >= secureMemEnd) {
        LOG_ECMA(ERROR) << "Secure memory check failed, mem out of secure memory, mem: " << std::hex << mem;
        return false;
    }
    return true;
}

#define CHECK_HAS_PENDING_EXCEPTION(vm, returnVal)                                    \
    do {                                                                              \
        if (vm->GetJSThread()->HasPendingException()) {                               \
            LOG_ECMA(ERROR) << "pending exception before jsnapi interface called" <<  \
                ", which is " << __FUNCTION__ << " in line: " << __LINE__;            \
            LOG_ECMA(ERROR) << "print exception info:";                               \
            JSNApi::PrintExceptionInfo(vm);                                           \
            return returnVal;                                                         \
        }                                                                             \
    } while (false)

#define CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm)                              \
    CHECK_HAS_PENDING_EXCEPTION(vm, JSValueRef::Undefined(vm))

#define CHECK_HAS_PENDING_EXCEPTION_WITHOUT_RETURN(vm)                                \
    do {                                                                              \
        if (vm->GetJSThread()->HasPendingException()) {                               \
            LOG_ECMA(ERROR) << "pending exception before jsnapi interface called" <<  \
                ", which is " << __FUNCTION__ << " in line: " << __LINE__;            \
            LOG_ECMA(ERROR) << "print exception info:";                               \
            JSNApi::PrintExceptionInfo(vm);                                           \
            return;                                                                   \
        }                                                                             \
    } while (false)

// ------------------------------------ Panda -----------------------------------------------
EcmaVM *JSNApi::CreateJSVM(const RuntimeOption &option)
{
    JSRuntimeOptions runtimeOptions;
    runtimeOptions.SetArkProperties(option.GetArkProperties());
    runtimeOptions.SetArkBundleName(option.GetArkBundleName());
    runtimeOptions.SetLongPauseTime(option.GetLongPauseTime());
    runtimeOptions.SetGcThreadNum(option.GetGcThreadNum());
    runtimeOptions.SetIsWorker(option.GetIsWorker());
    // Mem
    runtimeOptions.SetHeapSizeLimit(option.GetGcPoolSize());
// Disable the asm-interpreter of ark-engine for ios-platform temporarily.
#if !defined(PANDA_TARGET_IOS)
    // asmInterpreter
    runtimeOptions.SetEnableAsmInterpreter(option.GetEnableAsmInterpreter());
#else
    runtimeOptions.SetEnableAsmInterpreter(false);
#endif
    runtimeOptions.SetEnableBuiltinsLazy(option.GetEnableBuiltinsLazy());
    runtimeOptions.SetAsmOpcodeDisableRange(option.GetAsmOpcodeDisableRange());
    // aot
    runtimeOptions.SetEnableAOT(option.GetEnableAOT());
    runtimeOptions.SetEnablePGOProfiler(option.GetEnableProfile());
    runtimeOptions.SetPGOProfilerPath(option.GetProfileDir());

    // Dfx
    runtimeOptions.SetLogLevel(Log::LevelToString(Log::ConvertFromRuntime(option.GetLogLevel())));
    runtimeOptions.SetEnableArkTools(option.GetEnableArkTools());
    return CreateEcmaVM(runtimeOptions);
}

EcmaContext *JSNApi::CreateJSContext(EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    return EcmaContext::CreateAndInitialize(thread);
}

void JSNApi::SwitchCurrentContext(EcmaVM *vm, EcmaContext *context)
{
    JSThread *thread = vm->GetJSThread();
    thread->SwitchCurrentContext(context);
}

void JSNApi::DestroyJSContext(EcmaVM *vm, EcmaContext *context)
{
    JSThread *thread = vm->GetJSThread();
    EcmaContext::CheckAndDestroy(thread, context);
}

EcmaVM *JSNApi::CreateEcmaVM(const JSRuntimeOptions &options)
{
    {
        os::memory::LockHolder lock(*mutex);
        vmCount_++;
        if (!initialize_) {
            ecmascript::Log::Initialize(options);
            InitializeIcuData(options);
            InitializeMemMapAllocator();
            InitializePGOProfiler(options);
            initialize_ = true;
        }
    }
    auto config = ecmascript::EcmaParamConfiguration(options.IsWorker(),
        MemMapAllocator::GetInstance()->GetCapacity());
    LOG_ECMA(DEBUG) << " [NAPI]: CreateEcmaVM, isWorker = " << options.IsWorker() << ", vmCount = " << vmCount_;
    MemMapAllocator::GetInstance()->IncreaseAndCheckReserved(config.GetMaxHeapSize());
    return EcmaVM::Create(options, config);
}

void JSNApi::DestroyJSVM(EcmaVM *ecmaVm)
{
    os::memory::LockHolder lock(*mutex);
    if (!initialize_) {
        return;
    }
    auto &config = ecmaVm->GetEcmaParamConfiguration();
    MemMapAllocator::GetInstance()->DecreaseReserved(config.GetMaxHeapSize());
    EcmaVM::Destroy(ecmaVm);
    vmCount_--;
    if (vmCount_ <= 0) {
        DestroyAnDataManager();
        DestroyMemMapAllocator();
        DestroyPGOProfiler();
        initialize_ = false;
    }
}

void JSNApi::TriggerGC(const EcmaVM *vm, TRIGGER_GC_TYPE gcType)
{
    if (vm->GetJSThread() != nullptr && vm->IsInitialized()) {
        CHECK_HAS_PENDING_EXCEPTION_WITHOUT_RETURN(vm);
        switch (gcType) {
            case TRIGGER_GC_TYPE::SEMI_GC:
                vm->CollectGarbage(vm->GetHeap()->SelectGCType(), ecmascript::GCReason::EXTERNAL_TRIGGER);
                break;
            case TRIGGER_GC_TYPE::OLD_GC:
                vm->CollectGarbage(ecmascript::TriggerGCType::OLD_GC, ecmascript::GCReason::EXTERNAL_TRIGGER);
                break;
            case TRIGGER_GC_TYPE::FULL_GC:
                vm->CollectGarbage(ecmascript::TriggerGCType::FULL_GC, ecmascript::GCReason::EXTERNAL_TRIGGER);
                break;
            default:
                break;
        }
    }
}

void JSNApi::ThrowException(const EcmaVM *vm, Local<JSValueRef> error)
{
    auto thread = vm->GetJSThread();
    if (thread->HasPendingException()) {
        LOG_ECMA(ERROR) << "An exception has already occurred before, keep old exception here.";
        return;
    }
    thread->SetException(JSNApiHelper::ToJSTaggedValue(*error));
}

void JSNApi::PrintExceptionInfo(const EcmaVM *vm)
{
    JSThread* thread = vm->GetJSThread();
    [[maybe_unused]] ecmascript::EcmaHandleScope handleScope(thread);

    if (!HasPendingException(vm)) {
        return;
    }
    Local<ObjectRef> exception = GetAndClearUncaughtException(vm);
    JSHandle<JSTaggedValue> exceptionHandle = JSNApiHelper::ToJSHandle(exception);
    if (exceptionHandle->IsJSError()) {
        vm->PrintJSErrorInfo(exceptionHandle);
        ThrowException(vm, exception);
        return;
    }
    JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, exceptionHandle);
    ecmascript::CString string = ConvertToString(*result);
    LOG_ECMA(ERROR) << string;
    ThrowException(vm, exception);
}

bool JSNApi::StartDebugger([[maybe_unused]] EcmaVM *vm, [[maybe_unused]] const DebugOption &option,
                           [[maybe_unused]] int32_t instanceId,
                           [[maybe_unused]] const DebuggerPostTask &debuggerPostTask)
{
#if defined(ECMASCRIPT_SUPPORT_DEBUGGER)
#if !defined(PANDA_TARGET_IOS)
    if (vm == nullptr) {
        return false;
    }
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    const auto &handler = vm->GetJsDebuggerManager()->GetDebugLibraryHandle();
    if (handler.IsValid()) {
        return false;
    }

    if (option.libraryPath == nullptr) {
        return false;
    }
    auto handle = panda::os::library_loader::Load(std::string(option.libraryPath));
    if (!handle) {
        return false;
    }

    using StartDebugger = bool (*)(
        const std::string &, EcmaVM *, bool, int32_t, const DebuggerPostTask &, int);

    auto sym = panda::os::library_loader::ResolveSymbol(handle.Value(), "StartDebug");
    if (!sym) {
        LOG_ECMA(ERROR) << sym.Error().ToString();
        return false;
    }

    bool ret = reinterpret_cast<StartDebugger>(sym.Value())(
        "PandaDebugger", vm, option.isDebugMode, instanceId, debuggerPostTask, option.port);
    if (ret) {
        vm->GetJsDebuggerManager()->SetDebugMode(option.isDebugMode);
        vm->GetJsDebuggerManager()->SetDebugLibraryHandle(std::move(handle.Value()));
    }
    return ret;
#else
    if (vm == nullptr) {
        return false;
    }
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    bool ret = OHOS::ArkCompiler::Toolchain::StartDebug(
        DEBUGGER_NAME, vm, option.isDebugMode, instanceId, debuggerPostTask, option.port);
    if (ret) {
        vm->GetJsDebuggerManager()->SetDebugMode(option.isDebugMode);
    }
    return ret;
#endif // PANDA_TARGET_IOS
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler debugger";
    return false;
#endif // ECMASCRIPT_SUPPORT_DEBUGGER
}

bool JSNApi::StopDebugger([[maybe_unused]] EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_DEBUGGER)
#if !defined(PANDA_TARGET_IOS)
    if (vm == nullptr) {
        return false;
    }
    CHECK_HAS_PENDING_EXCEPTION(vm, false);

    const auto &handle = vm->GetJsDebuggerManager()->GetDebugLibraryHandle();

    using StopDebug = void (*)(const std::string &);

    auto sym = panda::os::library_loader::ResolveSymbol(handle, "StopDebug");
    if (!sym) {
        LOG_ECMA(ERROR) << sym.Error().ToString();
        return false;
    }

    reinterpret_cast<StopDebug>(sym.Value())("PandaDebugger");

    vm->GetJsDebuggerManager()->SetDebugMode(false);
    return true;
#else
    if (vm == nullptr) {
        return false;
    }
    CHECK_HAS_PENDING_EXCEPTION(vm, false);

    OHOS::ArkCompiler::Toolchain::StopDebug(DEBUGGER_NAME);
    vm->GetJsDebuggerManager()->SetDebugMode(false);
    return true;
#endif // PANDA_TARGET_IOS
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler debugger";
    return false;
#endif // ECMASCRIPT_SUPPORT_DEBUGGER
}

bool JSNApi::IsMixedDebugEnabled([[maybe_unused]] const EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_DEBUGGER)
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    return vm->GetJsDebuggerManager()->IsMixedDebugEnabled();
#else
    return false;
#endif
}

void JSNApi::NotifyNativeCalling([[maybe_unused]] const EcmaVM *vm, [[maybe_unused]] const void *nativeAddress)
{
#if defined(ECMASCRIPT_SUPPORT_DEBUGGER)
    CHECK_HAS_PENDING_EXCEPTION_WITHOUT_RETURN(vm);
    vm->GetJsDebuggerManager()->GetNotificationManager()->NativeCallingEvent(nativeAddress);
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler debugger";
#endif
}

void JSNApi::LoadAotFile(EcmaVM *vm, const std::string &moduleName)
{
    CHECK_HAS_PENDING_EXCEPTION_WITHOUT_RETURN(vm);
    if (!ecmascript::AnFileDataManager::GetInstance()->IsEnable()) {
        return;
    }
    std::string aotFileName = ecmascript::AnFileDataManager::GetInstance()->GetDir();
    aotFileName += moduleName;
    LOG_ECMA(INFO) << "start to load aot file: " << aotFileName;
    vm->GetJSThread()->GetCurrentEcmaContext()->LoadAOTFiles(aotFileName);
}

bool JSNApi::ExecuteInContext(EcmaVM *vm, const std::string &fileName, const std::string &entry, bool needUpdate)
{
    LOG_ECMA(DEBUG) << "start to execute ark file in context: " << fileName;
    JSThread *thread = vm->GetAssociatedJSThread();
    EcmaContext::MountContext(thread);
    if (!ecmascript::JSPandaFileExecutor::ExecuteFromFile(thread, fileName.c_str(), entry, needUpdate)) {
        LOG_ECMA(ERROR) << "Cannot execute ark file '" << fileName
                        << "' with entry '" << entry << "'" << std::endl;
        return false;
    }
    EcmaContext::UnmountContext(thread);
    return true;
}

bool JSNApi::Execute(EcmaVM *vm, const std::string &fileName, const std::string &entry, bool needUpdate)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    LOG_ECMA(DEBUG) << "start to execute ark file: " << fileName;
    JSThread *thread = vm->GetAssociatedJSThread();
    if (!ecmascript::JSPandaFileExecutor::ExecuteFromFile(thread, fileName.c_str(), entry, needUpdate)) {
        LOG_ECMA(ERROR) << "Cannot execute ark file '" << fileName
                        << "' with entry '" << entry << "'" << std::endl;
        return false;
    }
    return true;
}

// The security interface needs to be modified accordingly.
bool JSNApi::Execute(EcmaVM *vm, const uint8_t *data, int32_t size, const std::string &entry,
                     const std::string &filename, bool needUpdate)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    LOG_ECMA(DEBUG) << "start to execute ark buffer: " << filename;
    JSThread *thread = vm->GetAssociatedJSThread();
    if (!ecmascript::JSPandaFileExecutor::ExecuteFromBuffer(thread, data, size, entry, filename.c_str(), needUpdate)) {
        LOG_ECMA(ERROR) << "Cannot execute ark buffer file '" << filename
                        << "' with entry '" << entry << "'" << std::endl;
        return false;
    }
    return true;
}

// The security interface needs to be modified accordingly.
bool JSNApi::ExecuteModuleBuffer(EcmaVM *vm, const uint8_t *data, int32_t size, const std::string &filename,
                                 bool needUpdate)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    LOG_ECMA(DEBUG) << "start to execute module buffer: " << filename;
    JSThread *thread = vm->GetAssociatedJSThread();
    if (!ecmascript::JSPandaFileExecutor::ExecuteModuleBuffer(thread, data, size, filename.c_str(), needUpdate)) {
        LOG_ECMA(ERROR) << "Cannot execute module buffer file '" << filename;
        return false;
    }
    return true;
}

bool JSNApi::ExecuteSecure(EcmaVM *vm, uint8_t *data, int32_t size, const std::string &entry,
                           const std::string &filename, bool needUpdate)
{
    LOG_ECMA(INFO) << "start to execute ark buffer with secure memory: " << filename;
    if (!CheckSecureMem(reinterpret_cast<uintptr_t>(data))) {
        LOG_ECMA(ERROR) << "Secure memory check failed, please execute in srcure memory.";
        return false;
    }
    JSThread *thread = vm->GetAssociatedJSThread();
    if (!ecmascript::JSPandaFileExecutor::ExecuteFromBufferSecure(thread, data, size, entry, filename.c_str(),
                                                                  needUpdate)) {
        LOG_ECMA(ERROR) << "Cannot execute ark buffer file '" << filename
                        << "' with entry '" << entry << "'" << std::endl;
        return false;
    }
    return true;
}

bool JSNApi::ExecuteModuleBufferSecure(EcmaVM *vm, uint8_t* data, int32_t size, const std::string &filename,
                                       bool needUpdate)
{
    LOG_ECMA(INFO) << "start to execute module buffer with secure memory: " << filename;
    if (!CheckSecureMem(reinterpret_cast<uintptr_t>(data))) {
        LOG_ECMA(ERROR) << "Secure memory check failed, please execute in srcure memory.";
        return false;
    }
    JSThread *thread = vm->GetAssociatedJSThread();
    if (!ecmascript::JSPandaFileExecutor::ExecuteModuleBufferSecure(thread, data, size, filename.c_str(),
                                                                    needUpdate)) {
        LOG_ECMA(ERROR) << "Cannot execute module buffer file '" << filename;
        return false;
    }
    return true;
}

void JSNApi::PreFork(EcmaVM *vm)
{
    vm->PreFork();
}

void JSNApi::PostFork(EcmaVM *vm, const RuntimeOption &option)
{
    JSRuntimeOptions &jsOption = vm->GetJSOptions();
    LOG_ECMA(INFO) << "asmint: " << jsOption.GetEnableAsmInterpreter()
                    << ", aot: " << jsOption.GetEnableAOT()
                    << ", bundle name: " <<  option.GetBundleName();
    jsOption.SetEnablePGOProfiler(option.GetEnableProfile());
    vm->ResetPGOProfiler();
    JSRuntimeOptions runtimeOptions;
    runtimeOptions.SetLogLevel(Log::LevelToString(Log::ConvertFromRuntime(option.GetLogLevel())));
    Log::Initialize(runtimeOptions);

    if (jsOption.GetEnableAOT() && option.GetAnDir().size()) {
        ecmascript::AnFileDataManager::GetInstance()->SetDir(option.GetAnDir());
        ecmascript::AnFileDataManager::GetInstance()->SetEnable(true);
    }

    vm->PostFork();
}

void JSNApi::addWorker(EcmaVM *hostVm, EcmaVM *workerVm)
{
    if (hostVm != nullptr && workerVm != nullptr) {
        hostVm->WorkersetInfo(hostVm, workerVm);
        workerVm->SetBundleName(hostVm->GetBundleName());
    }
}

bool JSNApi::DeleteWorker(EcmaVM *hostVm, EcmaVM *workerVm)
{
    if (hostVm != nullptr && workerVm != nullptr) {
        return hostVm->DeleteWorker(hostVm, workerVm);
    }
    return false;
}

Local<ObjectRef> JSNApi::GetUncaughtException(const EcmaVM *vm)
{
    return JSNApiHelper::ToLocal<ObjectRef>(vm->GetEcmaUncaughtException());
}

Local<ObjectRef> JSNApi::GetAndClearUncaughtException(const EcmaVM *vm)
{
    return JSNApiHelper::ToLocal<ObjectRef>(vm->GetAndClearEcmaUncaughtException());
}

bool JSNApi::HasPendingException(const EcmaVM *vm)
{
    return vm->GetJSThread()->HasPendingException();
}

bool JSNApi::HasPendingJob(const EcmaVM *vm)
{
    return vm->GetJSThread()->GetCurrentEcmaContext()->HasPendingJob();
}

void JSNApi::EnableUserUncaughtErrorHandler(EcmaVM *vm)
{
    return vm->GetJSThread()->GetCurrentEcmaContext()->EnableUserUncaughtErrorHandler();
}

Local<ObjectRef> JSNApi::GetGlobalObject(const EcmaVM *vm)
{
    JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> global(vm->GetJSThread(), globalEnv->GetGlobalObject());
    return JSNApiHelper::ToLocal<ObjectRef>(global);
}

void JSNApi::ExecutePendingJob(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_WITHOUT_RETURN(vm);
    EcmaVM::ConstCast(vm)->GetJSThread()->GetCurrentEcmaContext()->ExecutePromisePendingJob();
}

uintptr_t JSNApi::GetHandleAddr(const EcmaVM *vm, uintptr_t localAddress)
{
    if (localAddress == 0) {
        return 0;
    }
    JSTaggedType value = *(reinterpret_cast<JSTaggedType *>(localAddress));
    return ecmascript::EcmaHandleScope::NewHandle(vm->GetJSThread(), value);
}

uintptr_t JSNApi::GetGlobalHandleAddr(const EcmaVM *vm, uintptr_t localAddress)
{
    if (localAddress == 0) {
        return 0;
    }
    JSTaggedType value = *(reinterpret_cast<JSTaggedType *>(localAddress));
    return vm->GetJSThread()->NewGlobalHandle(value);
}

uintptr_t JSNApi::SetWeak(const EcmaVM *vm, uintptr_t localAddress)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, 0);
    if (localAddress == 0) {
        return 0;
    }
    return vm->GetJSThread()->SetWeak(localAddress);
}

uintptr_t JSNApi::SetWeakCallback(const EcmaVM *vm, uintptr_t localAddress, void *ref,
                                  WeakRefClearCallBack freeGlobalCallBack, WeakRefClearCallBack nativeFinalizeCallback)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, 0);
    if (localAddress == 0) {
        return 0;
    }
    return vm->GetJSThread()->SetWeak(localAddress, ref, freeGlobalCallBack, nativeFinalizeCallback);
}

uintptr_t JSNApi::ClearWeak(const EcmaVM *vm, uintptr_t localAddress)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, 0);
    if (localAddress == 0) {
        return 0;
    }
    if (JSTaggedValue(reinterpret_cast<ecmascript::Node *>(localAddress)->GetObject())
        .IsUndefined()) {
        LOG_ECMA(ERROR) << "The object of weak reference has been recycled!";
        return 0;
    }
    return vm->GetJSThread()->ClearWeak(localAddress);
}

bool JSNApi::IsWeak(const EcmaVM *vm, uintptr_t localAddress)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    if (localAddress == 0) {
        return false;
    }
    return vm->GetJSThread()->IsWeak(localAddress);
}

void JSNApi::DisposeGlobalHandleAddr(const EcmaVM *vm, uintptr_t addr)
{
    if (addr == 0 || !reinterpret_cast<ecmascript::Node *>(addr)->IsUsing()) {
        return;
    }
    vm->GetJSThread()->DisposeGlobalHandle(addr);
}

void *JSNApi::SerializeValue(const EcmaVM *vm, Local<JSValueRef> value, Local<JSValueRef> transfer)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, nullptr);
    ecmascript::JSThread *thread = vm->GetJSThread();
    ecmascript::Serializer serializer(thread);
    JSHandle<JSTaggedValue> arkValue = JSNApiHelper::ToJSHandle(value);
    JSHandle<JSTaggedValue> arkTransfer = JSNApiHelper::ToJSHandle(transfer);
    std::unique_ptr<ecmascript::SerializationData> data;
    if (serializer.WriteValue(thread, arkValue, arkTransfer)) {
        data = serializer.Release();
    }
    if (data == nullptr) {
        return nullptr;
    } else {
        return reinterpret_cast<void *>(data.release());
    }
}

Local<JSValueRef> JSNApi::DeserializeValue(const EcmaVM *vm, void *recoder, void *hint)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    ecmascript::JSThread *thread = vm->GetJSThread();
    std::unique_ptr<ecmascript::SerializationData> data(reinterpret_cast<ecmascript::SerializationData *>(recoder));
    ecmascript::Deserializer deserializer(thread, data.release(), hint);
    JSHandle<JSTaggedValue> result = deserializer.ReadValue();
    return JSNApiHelper::ToLocal<ObjectRef>(result);
}

void JSNApi::DeleteSerializationData(void *data)
{
    ecmascript::SerializationData *value = reinterpret_cast<ecmascript::SerializationData *>(data);
    delete value;
    value = nullptr;
}

void HostPromiseRejectionTracker(const EcmaVM *vm,
                                 const JSHandle<JSPromise> promise,
                                 const JSHandle<JSTaggedValue> reason,
                                 const ecmascript::PromiseRejectionEvent operation,
                                 void* data)
{
    CHECK_HAS_PENDING_EXCEPTION_WITHOUT_RETURN(vm);
    ecmascript::PromiseRejectCallback promiseRejectCallback =
        vm->GetJSThread()->GetCurrentEcmaContext()->GetPromiseRejectCallback();
    if (promiseRejectCallback != nullptr) {
        Local<JSValueRef> promiseVal = JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>::Cast(promise));
        PromiseRejectInfo promiseRejectInfo(promiseVal, JSNApiHelper::ToLocal<JSValueRef>(reason),
                              static_cast<PromiseRejectInfo::PROMISE_REJECTION_EVENT>(operation), data);
        promiseRejectCallback(reinterpret_cast<void*>(&promiseRejectInfo));
    }
}

void JSNApi::SetHostPromiseRejectionTracker(EcmaVM *vm, void *cb, void* data)
{
    vm->GetJSThread()->GetCurrentEcmaContext()->SetHostPromiseRejectionTracker(HostPromiseRejectionTracker);
    vm->GetJSThread()->GetCurrentEcmaContext()->SetPromiseRejectCallback(
        reinterpret_cast<ecmascript::PromiseRejectCallback>(cb));
    vm->GetJSThread()->GetCurrentEcmaContext()->SetData(data);
}

void JSNApi::SetHostResolveBufferTracker(EcmaVM *vm,
    std::function<bool(std::string dirPath, uint8_t **buff, size_t *buffSize)> cb)
{
    vm->SetResolveBufferCallback(cb);
}

void JSNApi::SetUnloadNativeModuleCallback(EcmaVM *vm, const std::function<bool(const std::string &moduleKey)> &cb)
{
    vm->SetUnloadNativeModuleCallback(cb);
}

void JSNApi::SetNativePtrGetter(EcmaVM *vm, void* cb)
{
    vm->SetNativePtrGetter(reinterpret_cast<ecmascript::NativePtrGetter>(cb));
}

void JSNApi::SetHostEnqueueJob(const EcmaVM *vm, Local<JSValueRef> cb)
{
    CHECK_HAS_PENDING_EXCEPTION_WITHOUT_RETURN(vm);
    JSHandle<JSFunction> fun = JSHandle<JSFunction>::Cast(JSNApiHelper::ToJSHandle(cb));
    JSHandle<TaggedArray> array = vm->GetFactory()->EmptyArray();
    JSHandle<MicroJobQueue> job = vm->GetJSThread()->GetCurrentEcmaContext()->GetMicroJobQueue();
    MicroJobQueue::EnqueueJob(vm->GetJSThread(), job, QueueType::QUEUE_PROMISE, fun, array);
}

PromiseRejectInfo::PromiseRejectInfo(Local<JSValueRef> promise, Local<JSValueRef> reason,
                                     PromiseRejectInfo::PROMISE_REJECTION_EVENT operation, void* data)
    : promise_(promise), reason_(reason), operation_(operation), data_(data) {}

Local<JSValueRef> PromiseRejectInfo::GetPromise() const
{
    return promise_;
}

Local<JSValueRef> PromiseRejectInfo::GetReason() const
{
    return reason_;
}

PromiseRejectInfo::PROMISE_REJECTION_EVENT PromiseRejectInfo::GetOperation() const
{
    return operation_;
}

void* PromiseRejectInfo::GetData() const
{
    return data_;
}

bool JSNApi::ExecuteModuleFromBuffer(EcmaVM *vm, const void *data, int32_t size, const std::string &file)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    JSThread *thread = vm->GetAssociatedJSThread();
    if (!ecmascript::JSPandaFileExecutor::ExecuteFromBuffer(thread, data, size, ENTRY_POINTER, file.c_str())) {
        std::cerr << "Cannot execute panda file from memory" << std::endl;
        return false;
    }
    return true;
}

Local<ObjectRef> JSNApi::GetExportObject(EcmaVM *vm, const std::string &file, const std::string &key)
{
    ecmascript::ModuleManager *moduleManager = vm->GetAssociatedJSThread()->GetCurrentEcmaContext()->GetModuleManager();
    JSTaggedValue result = moduleManager->GetExportObject(file.c_str(), key.c_str());
    JSHandle<JSTaggedValue> exportObj(vm->GetJSThread(), result);
    return JSNApiHelper::ToLocal<ObjectRef>(exportObj);
}

Local<ObjectRef> JSNApi::GetExportObjectFromBuffer(EcmaVM *vm, const std::string &file,
                                                   const std::string &key)
{
    ecmascript::ModuleManager *moduleManager = vm->GetAssociatedJSThread()->GetCurrentEcmaContext()->GetModuleManager();
    JSTaggedValue result = moduleManager->GetExportObjectFromBuffer(file.c_str(), key.c_str());
    JSHandle<JSTaggedValue> exportObj(vm->GetJSThread(), result);
    return JSNApiHelper::ToLocal<ObjectRef>(exportObj);
}

 // Initialize IcuData Path
void JSNApi::InitializeIcuData(const JSRuntimeOptions &options)
{
    std::string icuPath = options.GetIcuDataPath();
    if (icuPath == "default") {
#if !WIN_OR_MAC_OR_IOS_PLATFORM && !defined(PANDA_TARGET_LINUX)
        SetHwIcuDirectory();
#endif
    } else {
        std::string absPath;
        if (ecmascript::RealPath(icuPath, absPath)) {
            u_setDataDirectory(absPath.c_str());
        }
    }
}

void JSNApi::InitializeMemMapAllocator()
{
    MemMapAllocator::GetInstance()->Initialize(ecmascript::DEFAULT_REGION_SIZE);
}

void JSNApi::DestroyMemMapAllocator()
{
    MemMapAllocator::GetInstance()->Finalize();
}

void JSNApi::InitializePGOProfiler(const ecmascript::JSRuntimeOptions &options)
{
    ecmascript::PGOProfilerManager::GetInstance()->Initialize(
        options.GetPGOProfilerPath(), options.GetPGOHotnessThreshold());
}

void JSNApi::DestroyPGOProfiler()
{
    ecmascript::PGOProfilerManager::GetInstance()->Destroy();
}

void JSNApi::DestroyAnDataManager()
{
    ecmascript::AnFileDataManager::GetInstance()->SafeDestroyAllData();
}

FunctionCallScope::FunctionCallScope(EcmaVM *vm) : vm_(vm)
{
    vm_->IncreaseCallDepth();
}

FunctionCallScope::~FunctionCallScope()
{
    vm_->DecreaseCallDepth();
    if (vm_->IsTopLevelCallDepth()) {
        JSThread *thread = vm_->GetJSThread();
        thread->GetCurrentEcmaContext()->ExecutePromisePendingJob();
    }
}

// ----------------------------------- HandleScope -------------------------------------
LocalScope::LocalScope(const EcmaVM *vm) : thread_(vm->GetJSThread())
{
    auto context = reinterpret_cast<JSThread *>(thread_)->GetCurrentEcmaContext();
    prevNext_ = context->GetHandleScopeStorageNext();
    prevEnd_ = context->GetHandleScopeStorageEnd();
    prevHandleStorageIndex_ = context->GetCurrentHandleStorageIndex();
    context->HandleScopeCountAdd();
}

LocalScope::LocalScope(const EcmaVM *vm, JSTaggedType value) : thread_(vm->GetJSThread())
{
    auto context = reinterpret_cast<JSThread *>(thread_)->GetCurrentEcmaContext();
    ecmascript::EcmaHandleScope::NewHandle(reinterpret_cast<JSThread *>(thread_), value);
    prevNext_ = context->GetHandleScopeStorageNext();
    prevEnd_ = context->GetHandleScopeStorageEnd();
    prevHandleStorageIndex_ = context->GetCurrentHandleStorageIndex();
    context->HandleScopeCountAdd();
}

LocalScope::~LocalScope()
{
    auto context = reinterpret_cast<JSThread *>(thread_)->GetCurrentEcmaContext();
    context->HandleScopeCountDec();
    context->SetHandleScopeStorageNext(static_cast<JSTaggedType *>(prevNext_));
    if (context->GetHandleScopeStorageEnd() != prevEnd_) {
        context->SetHandleScopeStorageEnd(static_cast<JSTaggedType *>(prevEnd_));
        context->ShrinkHandleStorage(prevHandleStorageIndex_);
    }
}

// ----------------------------------- EscapeLocalScope ------------------------------
EscapeLocalScope::EscapeLocalScope(const EcmaVM *vm) : LocalScope(vm, JSTaggedValue::Undefined().GetRawData())
{
    auto thread = vm->GetJSThread();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    escapeHandle_ = ToUintPtr(thread->GetCurrentEcmaContext()->GetHandleScopeStorageNext() - 1);
}

// ----------------------------------- PritimitiveRef ---------------------------------------
Local<JSValueRef> PrimitiveRef::GetValue(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    if (obj->IsJSPrimitiveRef()) {
        JSTaggedValue primitiveValue = JSPrimitiveRef::Cast(obj->GetTaggedObject())->GetValue();
        JSHandle<JSTaggedValue> value(vm->GetJSThread(), primitiveValue);
        return JSNApiHelper::ToLocal<JSValueRef>(value);
    }
    return Local<JSValueRef>();
}

// ----------------------------------- NumberRef ---------------------------------------
Local<NumberRef> NumberRef::New(const EcmaVM *vm, double input)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    if (std::isnan(input)) {
        input = ecmascript::base::NAN_VALUE;
    }
    JSHandle<JSTaggedValue> number(thread, JSTaggedValue(input));
    return JSNApiHelper::ToLocal<NumberRef>(number);
}

Local<NumberRef> NumberRef::New(const EcmaVM *vm, int32_t input)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> number(thread, JSTaggedValue(input));
    return JSNApiHelper::ToLocal<NumberRef>(number);
}

Local<NumberRef> NumberRef::New(const EcmaVM *vm, uint32_t input)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> number(thread, JSTaggedValue(input));
    return JSNApiHelper::ToLocal<NumberRef>(number);
}

Local<NumberRef> NumberRef::New(const EcmaVM *vm, int64_t input)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> number(thread, JSTaggedValue(input));
    return JSNApiHelper::ToLocal<NumberRef>(number);
}

double NumberRef::Value()
{
    return JSTaggedNumber(JSNApiHelper::ToJSTaggedValue(this)).GetNumber();
}

// ----------------------------------- BigIntRef ---------------------------------------
Local<BigIntRef> BigIntRef::New(const EcmaVM *vm, uint64_t input)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<BigInt> big = BigInt::Uint64ToBigInt(thread, input);
    JSHandle<JSTaggedValue> bigint = JSHandle<JSTaggedValue>::Cast(big);
    return JSNApiHelper::ToLocal<BigIntRef>(bigint);
}

Local<BigIntRef> BigIntRef::New(const EcmaVM *vm, int64_t input)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<BigInt> big = BigInt::Int64ToBigInt(thread, input);
    JSHandle<JSTaggedValue> bigint = JSHandle<JSTaggedValue>::Cast(big);
    return JSNApiHelper::ToLocal<BigIntRef>(bigint);
}

Local<JSValueRef> BigIntRef::CreateBigWords(const EcmaVM *vm, bool sign, uint32_t size, const uint64_t* words)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<BigInt> big = BigInt::CreateBigWords(thread, sign, size, words);
    JSHandle<JSTaggedValue> bigint = JSHandle<JSTaggedValue>::Cast(big);
    return JSNApiHelper::ToLocal<JSValueRef>(bigint);
}

void BigIntRef::BigIntToInt64(const EcmaVM *vm, int64_t *cValue, bool *lossless)
{
    CHECK_HAS_PENDING_EXCEPTION_WITHOUT_RETURN(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> bigintVal(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(bigintVal, ERROR);
    BigInt::BigIntToInt64(thread, bigintVal, cValue, lossless);
}

void BigIntRef::BigIntToUint64(const EcmaVM *vm, uint64_t *cValue, bool *lossless)
{
    CHECK_HAS_PENDING_EXCEPTION_WITHOUT_RETURN(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> bigintVal(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(bigintVal, ERROR);
    BigInt::BigIntToUint64(thread, bigintVal, cValue, lossless);
}

void BigIntRef::GetWordsArray(bool* signBit, size_t wordCount, uint64_t* words)
{
    JSHandle<BigInt> bigintVal(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(bigintVal, FATAL);
    uint32_t len = bigintVal->GetLength();
    uint32_t count = 0;
    uint32_t index = 0;
    for (; index < wordCount - 1; ++index) {
        words[index] = static_cast<uint64_t>(bigintVal->GetDigit(count++));
        words[index] |= static_cast<uint64_t>(bigintVal->GetDigit(count++)) << 32; // 32 : int32_t bits
    }
    if (len % 2 == 0) { // 2 : len is odd or even
        words[index] = static_cast<uint64_t>(bigintVal->GetDigit(count++));
        words[index] |= static_cast<uint64_t>(bigintVal->GetDigit(count++)) << 32; // 32 : int32_t bits
    } else {
        words[index] = static_cast<uint64_t>(bigintVal->GetDigit(count++));
    }
    *signBit = bigintVal->GetSign();
}

uint32_t BigIntRef::GetWordsArraySize()
{
    JSHandle<BigInt> bigintVal(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(bigintVal, FATAL);
    uint32_t len = bigintVal->GetLength();
    return len % 2 != 0 ? len / 2 + 1 : len / 2; // 2 : len is odd or even
}

// ----------------------------------- BooleanRef ---------------------------------------
Local<BooleanRef> BooleanRef::New(const EcmaVM *vm, bool input)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> boolean(thread, JSTaggedValue(input));
    return JSNApiHelper::ToLocal<BooleanRef>(boolean);
}

bool BooleanRef::Value()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsTrue();
}

// ----------------------------------- IntegerRef ---------------------------------------
Local<IntegerRef> IntegerRef::New(const EcmaVM *vm, int input)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> integer(thread, JSTaggedValue(input));
    return JSNApiHelper::ToLocal<IntegerRef>(integer);
}

Local<IntegerRef> IntegerRef::NewFromUnsigned(const EcmaVM *vm, unsigned int input)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> integer(thread, JSTaggedValue(input));
    return JSNApiHelper::ToLocal<IntegerRef>(integer);
}

int IntegerRef::Value()
{
    return JSNApiHelper::ToJSTaggedValue(this).GetInt();
}

// ----------------------------------- StringRef ----------------------------------------
Local<StringRef> StringRef::NewFromUtf8(const EcmaVM *vm, const char *utf8, int length)
{
    ObjectFactory *factory = vm->GetFactory();
    if (length < 0) {
        JSHandle<JSTaggedValue> current(factory->NewFromUtf8(utf8));
        return JSNApiHelper::ToLocal<StringRef>(current);
    }
    JSHandle<JSTaggedValue> current(factory->NewFromUtf8(reinterpret_cast<const uint8_t *>(utf8), length));
    return JSNApiHelper::ToLocal<StringRef>(current);
}

Local<StringRef> StringRef::NewFromUtf16(const EcmaVM *vm, const char16_t *utf16, int length)
{
    ObjectFactory *factory = vm->GetFactory();
    if (length < 0) {
        JSHandle<JSTaggedValue> current(factory->NewFromUtf16(utf16));
        return JSNApiHelper::ToLocal<StringRef>(current);
    }
    JSHandle<JSTaggedValue> current(factory->NewFromUtf16(reinterpret_cast<const uint16_t *>(utf16), length));
    return JSNApiHelper::ToLocal<StringRef>(current);
}

std::string StringRef::ToString()
{
    return EcmaStringAccessor(JSNApiHelper::ToJSTaggedValue(this)).ToStdString();
}

uint32_t StringRef::Length()
{
    return EcmaStringAccessor(JSNApiHelper::ToJSTaggedValue(this)).GetLength();
}

int32_t StringRef::Utf8Length(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, 0);
    JSHandle<EcmaString> strHandle(vm->GetJSThread(), EcmaString::Cast(JSNApiHelper::ToJSTaggedValue(this)));
    return EcmaStringAccessor(EcmaStringAccessor::Flatten(vm, strHandle)).GetUtf8Length();
}

int StringRef::WriteUtf8(char *buffer, int length, bool isWriteBuffer)
{
    return EcmaStringAccessor(JSNApiHelper::ToJSTaggedValue(this))
        .WriteToFlatUtf8(reinterpret_cast<uint8_t *>(buffer), length, isWriteBuffer);
}

int StringRef::WriteUtf16(char16_t *buffer, int length)
{
    return EcmaStringAccessor(JSNApiHelper::ToJSTaggedValue(this))
        .WriteToUtf16(reinterpret_cast<uint16_t *>(buffer), length);
}

int StringRef::WriteLatin1(char *buffer, int length)
{
    return EcmaStringAccessor(JSNApiHelper::ToJSTaggedValue(this))
        .WriteToOneByte(reinterpret_cast<uint8_t *>(buffer), length);
}

Local<StringRef> StringRef::GetNapiWrapperString(const EcmaVM *vm)
{
    JSHandle<JSTaggedValue> napiWapperString = vm->GetJSThread()->GlobalConstants()->GetHandledNapiWrapperString();
    return JSNApiHelper::ToLocal<StringRef>(napiWapperString);
}

// ----------------------------------- SymbolRef -----------------------------------------
Local<SymbolRef> SymbolRef::New(const EcmaVM *vm, Local<StringRef> description)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSSymbol> symbol = factory->NewJSSymbol();
    JSTaggedValue desc = JSNApiHelper::ToJSTaggedValue(*description);
    symbol->SetDescription(vm->GetJSThread(), desc);
    return JSNApiHelper::ToLocal<SymbolRef>(JSHandle<JSTaggedValue>(symbol));
}

Local<StringRef> SymbolRef::GetDescription(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSTaggedValue description = JSSymbol::Cast(JSNApiHelper::ToJSTaggedValue(this).GetTaggedObject())->GetDescription();
    if (!description.IsString()) {
        auto constants = vm->GetJSThread()->GlobalConstants();
        return JSNApiHelper::ToLocal<StringRef>(constants->GetHandledEmptyString());
    }
    JSHandle<JSTaggedValue> descriptionHandle(vm->GetJSThread(), description);
    return JSNApiHelper::ToLocal<StringRef>(descriptionHandle);
}

// -------------------------------- NativePointerRef ------------------------------------
Local<NativePointerRef> NativePointerRef::New(const EcmaVM *vm, void *nativePointer, size_t nativeBindingsize)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSNativePointer> obj = factory->NewJSNativePointer(nativePointer, nullptr, nullptr,
        false, nativeBindingsize);
    return JSNApiHelper::ToLocal<NativePointerRef>(JSHandle<JSTaggedValue>(obj));
}

Local<NativePointerRef> NativePointerRef::New(
    const EcmaVM *vm, void *nativePointer, NativePointerCallback callBack, void *data, size_t nativeBindingsize)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSNativePointer> obj = factory->NewJSNativePointer(nativePointer, callBack, data,
        false, nativeBindingsize);
    return JSNApiHelper::ToLocal<NativePointerRef>(JSHandle<JSTaggedValue>(obj));
}

void *NativePointerRef::Value()
{
    JSHandle<JSTaggedValue> nativePointer = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(nativePointer, FATAL);
    return JSHandle<JSNativePointer>(nativePointer)->GetExternalPointer();
}

// ----------------------------------- ObjectRef ----------------------------------------
Local<ObjectRef> ObjectRef::New(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
    JSHandle<JSFunction> constructor(globalEnv->GetObjectFunction());
    JSHandle<JSTaggedValue> object(factory->NewJSObjectByConstructor(constructor));
    return JSNApiHelper::ToLocal<ObjectRef>(object);
}

Local<ObjectRef> ObjectRef::New(const EcmaVM *vm, void *detach, void *attach)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSFunction> constructor(env->GetObjectFunction());
    JSHandle<JSTaggedValue> object(factory->NewJSObjectByConstructor(constructor));
    JSHandle<JSTaggedValue> detachKey = env->GetDetachSymbol();
    JSHandle<JSTaggedValue> attachKey = env->GetAttachSymbol();
    JSHandle<JSTaggedValue> detachValue = JSNApiHelper::ToJSHandle(NativePointerRef::New(vm, detach));
    JSHandle<JSTaggedValue> attachValue = JSNApiHelper::ToJSHandle(NativePointerRef::New(vm, attach));
    JSTaggedValue::SetProperty(vm->GetJSThread(), object, detachKey, detachValue);
    JSTaggedValue::SetProperty(vm->GetJSThread(), object, attachKey, attachValue);
    RETURN_VALUE_IF_ABRUPT(vm->GetJSThread(), JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<ObjectRef>(object);
}

bool ObjectRef::Set(const EcmaVM *vm, void *detach, void *attach)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    [[maybe_unused]] LocalScope scope(vm);
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> object = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(object, ERROR);
    JSHandle<JSTaggedValue> detachKey = env->GetDetachSymbol();
    JSHandle<JSTaggedValue> attachKey = env->GetAttachSymbol();
    JSHandle<JSTaggedValue> detachValue = JSNApiHelper::ToJSHandle(NativePointerRef::New(vm, detach));
    JSHandle<JSTaggedValue> attachValue = JSNApiHelper::ToJSHandle(NativePointerRef::New(vm, attach));
    bool detachResult = JSTaggedValue::SetProperty(vm->GetJSThread(), object, detachKey, detachValue);
    bool attachResult = JSTaggedValue::SetProperty(vm->GetJSThread(), object, attachKey, attachValue);
    RETURN_VALUE_IF_ABRUPT(vm->GetJSThread(), false);
    return detachResult && attachResult;
}

bool ObjectRef::Set(const EcmaVM *vm, Local<JSValueRef> key, Local<JSValueRef> value)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    [[maybe_unused]] LocalScope scope(vm);
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    JSHandle<JSTaggedValue> keyValue = JSNApiHelper::ToJSHandle(key);
    JSHandle<JSTaggedValue> valueValue = JSNApiHelper::ToJSHandle(value);
    bool result = JSTaggedValue::SetProperty(vm->GetJSThread(), obj, keyValue, valueValue);
    RETURN_VALUE_IF_ABRUPT(vm->GetJSThread(), false);
    return result;
}

bool ObjectRef::Set(const EcmaVM *vm, uint32_t key, Local<JSValueRef> value)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    [[maybe_unused]] LocalScope scope(vm);
    Local<JSValueRef> keyValue = NumberRef::New(vm, key);
    return Set(vm, keyValue, value);
}

bool ObjectRef::SetAccessorProperty(const EcmaVM *vm, Local<JSValueRef> key, Local<FunctionRef> getter,
    Local<FunctionRef> setter, PropertyAttribute attribute)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    [[maybe_unused]] LocalScope scope(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> getterValue = JSNApiHelper::ToJSHandle(getter);
    JSHandle<JSTaggedValue> setterValue = JSNApiHelper::ToJSHandle(setter);
    PropertyDescriptor desc(thread, attribute.IsWritable(), attribute.IsEnumerable(), attribute.IsConfigurable());
    desc.SetValue(JSNApiHelper::ToJSHandle(attribute.GetValue(vm)));
    desc.SetSetter(setterValue);
    desc.SetGetter(getterValue);
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    JSHandle<JSTaggedValue> keyValue = JSNApiHelper::ToJSHandle(key);
    bool result = JSTaggedValue::DefineOwnProperty(thread, obj, keyValue, desc);
    RETURN_VALUE_IF_ABRUPT(thread, false);
    return result;
}

Local<JSValueRef> ObjectRef::Get(const EcmaVM *vm, Local<JSValueRef> key)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    EscapeLocalScope scope(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    JSHandle<JSTaggedValue> keyValue = JSNApiHelper::ToJSHandle(key);
    OperationResult ret = JSTaggedValue::GetProperty(thread, obj, keyValue);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    if (!ret.GetPropertyMetaData().IsFound()) {
        return JSValueRef::Undefined(vm);
    }
    return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(ret.GetValue()));
}

Local<JSValueRef> ObjectRef::Get(const EcmaVM *vm, int32_t key)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    Local<JSValueRef> keyValue = IntegerRef::New(vm, key);
    return Get(vm, keyValue);
}

bool ObjectRef::GetOwnProperty(const EcmaVM *vm, Local<JSValueRef> key, PropertyAttribute &property)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    JSHandle<JSTaggedValue> keyValue = JSNApiHelper::ToJSHandle(key);
    PropertyDescriptor desc(thread);
    bool ret = JSObject::GetOwnProperty(thread, JSHandle<JSObject>(obj), keyValue, desc);
    if (!ret) {
        return false;
    }
    property.SetValue(JSNApiHelper::ToLocal<JSValueRef>(desc.GetValue()));
    if (desc.HasGetter()) {
        property.SetGetter(JSNApiHelper::ToLocal<JSValueRef>(desc.GetGetter()));
    }
    if (desc.HasSetter()) {
        property.SetSetter(JSNApiHelper::ToLocal<JSValueRef>(desc.GetSetter()));
    }
    if (desc.HasWritable()) {
        property.SetWritable(desc.IsWritable());
    }
    if (desc.HasEnumerable()) {
        property.SetEnumerable(desc.IsEnumerable());
    }
    if (desc.HasConfigurable()) {
        property.SetConfigurable(desc.IsConfigurable());
    }

    return true;
}

Local<ArrayRef> ObjectRef::GetOwnPropertyNames(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> obj(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(obj, ERROR);
    JSHandle<TaggedArray> array(JSTaggedValue::GetOwnPropertyKeys(thread, obj));
    JSHandle<JSTaggedValue> jsArray(JSArray::CreateArrayFromList(thread, array));
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<ArrayRef>(jsArray);
}

Local<ArrayRef> ObjectRef::GetAllPropertyNames(const EcmaVM *vm, uint32_t filter)
{
    // This interface is only used by napi.
    // This interface currently only supports normal objects.
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> obj(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(obj, ERROR);
    JSHandle<TaggedArray> array(JSTaggedValue::GetAllPropertyKeys(thread, obj, filter));
    JSHandle<JSTaggedValue> jsArray(JSArray::CreateArrayFromList(thread, array));
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<ArrayRef>(jsArray);
}

Local<ArrayRef> ObjectRef::GetOwnEnumerablePropertyNames(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSObject> obj(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(obj, ERROR);
    JSHandle<TaggedArray> array(JSObject::EnumerableOwnNames(thread, obj));
    JSHandle<JSTaggedValue> jsArray(JSArray::CreateArrayFromList(thread, array));
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<ArrayRef>(jsArray);
}

Local<JSValueRef> ObjectRef::GetPrototype(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSObject> object(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(object, ERROR);
    JSHandle<JSTaggedValue> prototype(thread, JSTaggedValue::GetPrototype(thread, JSHandle<JSTaggedValue>(object)));
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<JSValueRef>(prototype);
}

bool ObjectRef::SetPrototype(const EcmaVM *vm, Local<ObjectRef> prototype)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSObject> object(JSNApiHelper::ToJSHandle(this));
    JSHandle<JSObject> proto(JSNApiHelper::ToJSHandle(prototype));
    bool result = JSTaggedValue::SetPrototype(thread, JSHandle<JSTaggedValue>(object), JSHandle<JSTaggedValue>(proto));
    RETURN_VALUE_IF_ABRUPT(thread, false);
    return result;
}

bool ObjectRef::DefineProperty(const EcmaVM *vm, Local<JSValueRef> key, PropertyAttribute attribute)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> object(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(object, ERROR);
    JSHandle<JSTaggedValue> keyValue(JSNApiHelper::ToJSHandle(key));
    PropertyDescriptor desc(thread, attribute.IsWritable(), attribute.IsEnumerable(), attribute.IsConfigurable());
    desc.SetValue(JSNApiHelper::ToJSHandle(attribute.GetValue(vm)));
    bool result = object->DefinePropertyOrThrow(thread, object, keyValue, desc);
    RETURN_VALUE_IF_ABRUPT(thread, false);
    return result;
}

bool ObjectRef::Has(const EcmaVM *vm, Local<JSValueRef> key)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> object(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(object, ERROR);
    JSHandle<JSTaggedValue> keyValue(JSNApiHelper::ToJSHandle(key));
    bool result = object->HasProperty(thread, object, keyValue);
    RETURN_VALUE_IF_ABRUPT(thread, false);
    return result;
}

bool ObjectRef::Has(const EcmaVM *vm, uint32_t key)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> object(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(object, ERROR);
    bool result = object->HasProperty(thread, object, key);
    RETURN_VALUE_IF_ABRUPT(thread, false);
    return result;
}

bool ObjectRef::Delete(const EcmaVM *vm, Local<JSValueRef> key)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> object(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(object, ERROR);
    JSHandle<JSTaggedValue> keyValue(JSNApiHelper::ToJSHandle(key));
    bool result = object->DeleteProperty(thread, object, keyValue);
    RETURN_VALUE_IF_ABRUPT(thread, false);
    return result;
}

bool ObjectRef::Delete(const EcmaVM *vm, uint32_t key)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> object(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(object, ERROR);
    JSHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(key));
    bool result = object->DeleteProperty(thread, object, keyHandle);
    RETURN_VALUE_IF_ABRUPT(thread, false);
    return result;
}

Local<JSValueRef> ObjectRef::Freeze(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    EscapeLocalScope scope(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> object = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(object, ERROR);
    JSHandle<JSObject> obj(object);
    bool status = JSObject::SetIntegrityLevel(thread, obj, ecmascript::IntegrityLevel::FROZEN);
    if (JSNApi::HasPendingException(vm)) {
        JSHandle<JSTaggedValue> exception(thread, JSTaggedValue::Exception());
        return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(exception));
    }
    if (!status) {
        LOG_ECMA(ERROR) << "Freeze: freeze failed";
        Local<StringRef> message = StringRef::NewFromUtf8(vm, "Freeze: freeze failed");
        Local<JSValueRef> error = Exception::Error(vm, message);
        JSNApi::ThrowException(vm, error);
        JSHandle<JSTaggedValue> exception(thread, JSTaggedValue::Exception());
        return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(exception));
    }
    JSHandle<JSTaggedValue> resultValue(obj);
    return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(resultValue));
}

Local<JSValueRef> ObjectRef::Seal(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    EscapeLocalScope scope(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> object = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(object, ERROR);
    JSHandle<JSObject> obj(object);
    bool status = JSObject::SetIntegrityLevel(thread, obj, ecmascript::IntegrityLevel::SEALED);
    if (JSNApi::HasPendingException(vm)) {
        JSHandle<JSTaggedValue> exception(thread, JSTaggedValue::Exception());
        return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(exception));
    }
    if (!status) {
        LOG_ECMA(ERROR) << "Seal: seal failed";
        Local<StringRef> message = StringRef::NewFromUtf8(vm, "Freeze: freeze failed");
        Local<JSValueRef> error = Exception::Error(vm, message);
        JSNApi::ThrowException(vm, error);
        JSHandle<JSTaggedValue> exception(thread, JSTaggedValue::Exception());
        return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(exception));
    }
    JSHandle<JSTaggedValue> resultValue(obj);
    return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(resultValue));
}

void ObjectRef::SetNativePointerFieldCount(int32_t count)
{
    JSHandle<JSObject> object(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(object, FATAL);
    object->SetNativePointerFieldCount(count);
}

int32_t ObjectRef::GetNativePointerFieldCount()
{
    JSHandle<JSObject> object(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(object, FATAL);
    return object->GetNativePointerFieldCount();
}

void *ObjectRef::GetNativePointerField(int32_t index)
{
    JSHandle<JSObject> object(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(object, FATAL);
    return object->GetNativePointerField(index);
}

void ObjectRef::SetNativePointerField(int32_t index, void *nativePointer,
    NativePointerCallback callBack, void *data, size_t nativeBindingsize)
{
    JSHandle<JSObject> object(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(object, FATAL);
    object->SetNativePointerField(index, nativePointer, callBack, data, nativeBindingsize);
}

// ----------------------------------- FunctionRef --------------------------------------
Local<FunctionRef> FunctionRef::New(EcmaVM *vm, FunctionCallback nativeFunc,
    Deleter deleter, void *data, bool callNapi, size_t nativeBindingsize)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSFunction> current(factory->NewJSFunction(env, reinterpret_cast<void *>(Callback::RegisterCallback)));
    current->SetFunctionExtraInfo(thread, reinterpret_cast<void *>(nativeFunc), deleter, data, nativeBindingsize);
    current->SetCallNapi(callNapi);
    return JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(current));
}

Local<FunctionRef> FunctionRef::NewClassFunction(EcmaVM *vm, FunctionCallback nativeFunc,
    Deleter deleter, void *data, bool callNapi, size_t nativeBindingsize)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    EscapeLocalScope scope(vm);
    JSThread *thread = vm->GetJSThread();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutName());
    JSHandle<JSFunction> current =
        factory->NewJSFunctionByHClass(reinterpret_cast<void *>(Callback::RegisterCallback),
        hclass, ecmascript::FunctionKind::CLASS_CONSTRUCTOR);

    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> accessor = globalConst->GetHandledFunctionPrototypeAccessor();
    current->SetPropertyInlinedProps(thread, JSFunction::CLASS_PROTOTYPE_INLINE_PROPERTY_INDEX,
                                     accessor.GetTaggedValue());

    current->SetFunctionExtraInfo(thread, reinterpret_cast<void *>(nativeFunc), deleter, data, nativeBindingsize);

    JSHandle<JSObject> clsPrototype = JSFunction::NewJSFunctionPrototype(thread, current);
    clsPrototype.GetTaggedValue().GetTaggedObject()->GetClass()->SetClassPrototype(true);
    JSHandle<JSTaggedValue>::Cast(current)->GetTaggedObject()->GetClass()->SetClassConstructor(true);
    current->SetClassConstructor(true);
    JSHandle<JSTaggedValue> parent = env->GetFunctionPrototype();
    JSObject::SetPrototype(thread, JSHandle<JSObject>::Cast(current), parent);
    current->SetHomeObject(thread, clsPrototype);
    current->SetCallNapi(callNapi);
    Local<FunctionRef> result = JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(current));
    return scope.Escape(result);
}

Local<JSValueRef> FunctionRef::Call(const EcmaVM *vm, Local<JSValueRef> thisObj,
    const Local<JSValueRef> argv[],  // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    int32_t length)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    EscapeLocalScope scope(vm);
    FunctionCallScope callScope(EcmaVM::ConstCast(vm));
    JSThread *thread = vm->GetJSThread();
    if (!IsFunction()) {
        return JSValueRef::Undefined(vm);
    }
    vm->GetJsDebuggerManager()->ClearSingleStepper();
    JSHandle<JSTaggedValue> func = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(func, ERROR);
    JSHandle<JSTaggedValue> thisValue = JSNApiHelper::ToJSHandle(thisObj);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, func, thisValue, undefined, length);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    for (int32_t i = 0; i < length; i++) {
        JSHandle<JSTaggedValue> arg = JSNApiHelper::ToJSHandle(argv[i]);
        info->SetCallArg(i, arg.GetTaggedValue());
    }
    JSTaggedValue result = JSFunction::Call(info);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    JSHandle<JSTaggedValue> resultValue(thread, result);

    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    vm->GetHeap()->ClearKeptObjects();

    return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(resultValue));
}

Local<JSValueRef> FunctionRef::Constructor(const EcmaVM *vm,
    const Local<JSValueRef> argv[],  // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    int32_t length)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    FunctionCallScope callScope(EcmaVM::ConstCast(vm));
    JSThread *thread = vm->GetJSThread();
    if (!IsFunction()) {
        return JSValueRef::Undefined(vm);
    }
    JSHandle<JSTaggedValue> func = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(func, ERROR);
    JSHandle<JSTaggedValue> newTarget = func;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, func, undefined, newTarget, length);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    for (int32_t i = 0; i < length; i++) {
        JSHandle<JSTaggedValue> arg = JSNApiHelper::ToJSHandle(argv[i]);
        info->SetCallArg(i, arg.GetTaggedValue());
    }
    JSTaggedValue result = JSFunction::Construct(info);

    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    JSHandle<JSTaggedValue> resultValue(thread, result);
    return JSNApiHelper::ToLocal<JSValueRef>(resultValue);
}

Local<JSValueRef> FunctionRef::GetFunctionPrototype(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> func = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(func, FATAL);
    JSHandle<JSTaggedValue> prototype(thread, JSHandle<JSFunction>(func)->GetFunctionPrototype());
    return JSNApiHelper::ToLocal<JSValueRef>(prototype);
}

bool FunctionRef::Inherit(const EcmaVM *vm, Local<FunctionRef> parent)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    [[maybe_unused]] LocalScope scope(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> parentValue = JSNApiHelper::ToJSHandle(parent);
    JSHandle<JSObject> parentHandle = JSHandle<JSObject>::Cast(parentValue);
    JSHandle<JSObject> thisHandle = JSHandle<JSObject>::Cast(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(thisHandle, ERROR);
    // Set this.__proto__ to parent
    bool res = JSObject::SetPrototype(thread, thisHandle, parentValue);
    if (!res) {
        return false;
    }
    // Set this.Prototype.__proto__ to parent.Prototype
    JSHandle<JSTaggedValue> parentProtoType(thread, JSFunction::PrototypeGetter(thread, parentHandle));
    JSHandle<JSTaggedValue> thisProtoType(thread, JSFunction::PrototypeGetter(thread, thisHandle));
    return JSObject::SetPrototype(thread, JSHandle<JSObject>::Cast(thisProtoType), parentProtoType);
}

void FunctionRef::SetName(const EcmaVM *vm, Local<StringRef> name)
{
    CHECK_HAS_PENDING_EXCEPTION_WITHOUT_RETURN(vm);
    [[maybe_unused]] LocalScope scope(vm);
    JSThread *thread = vm->GetJSThread();
    JSFunction *func = JSFunction::Cast(JSNApiHelper::ToJSTaggedValue(this).GetTaggedObject());
    JSTaggedValue key = JSNApiHelper::ToJSTaggedValue(*name);
    JSFunction::SetFunctionNameNoPrefix(thread, func, key);
}

Local<StringRef> FunctionRef::GetName(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    EscapeLocalScope scope(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSFunctionBase> func(thread, JSNApiHelper::ToJSTaggedValue(this));
    JSHandle<JSTaggedValue> name = JSFunctionBase::GetFunctionName(thread, func);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return scope.Escape(JSNApiHelper::ToLocal<StringRef>(name));
}

Local<StringRef> FunctionRef::GetSourceCode(const EcmaVM *vm, int lineNumber)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    EscapeLocalScope scope(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSFunctionBase> func(thread, JSNApiHelper::ToJSTaggedValue(this));
    JSHandle<Method> method(thread, func->GetMethod());
    const JSPandaFile *jsPandaFile = method->GetJSPandaFile();
    DebugInfoExtractor *debugExtractor = JSPandaFileManager::GetInstance()->GetJSPtExtractor(jsPandaFile);
    ecmascript::CString entry = JSPandaFile::ENTRY_FUNCTION_NAME;
    if (jsPandaFile->IsMergedPF()) {
        JSFunction *function = JSFunction::Cast(func.GetTaggedValue().GetTaggedObject());
        JSTaggedValue recordName = function->GetRecordName();
        ASSERT(!recordName.IsHole());
        entry = ConvertToString(recordName);
    }

    uint32_t mainMethodIndex = jsPandaFile->GetMainMethodIndex(entry);
    JSMutableHandle<JSTaggedValue> sourceCodeHandle(thread, BuiltinsBase::GetTaggedString(thread, ""));
    if (mainMethodIndex == 0) {
        return scope.Escape(JSNApiHelper::ToLocal<StringRef>(sourceCodeHandle));
    }

    const std::string &allSourceCode = debugExtractor->GetSourceCode(panda_file::File::EntityId(mainMethodIndex));
    std::string sourceCode = StringHelper::GetSpecifiedLine(allSourceCode, lineNumber);
    uint32_t codeLen = sourceCode.length();
    if (codeLen == 0) {
        return scope.Escape(JSNApiHelper::ToLocal<StringRef>(sourceCodeHandle));
    }

    if (sourceCode[codeLen - 1] == '\r') {
        sourceCode = sourceCode.substr(0, codeLen - 1);
    }
    sourceCodeHandle.Update(BuiltinsBase::GetTaggedString(thread, sourceCode.c_str()));
    return scope.Escape(JSNApiHelper::ToLocal<StringRef>(sourceCodeHandle));
}

bool FunctionRef::IsNative(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSFunctionBase> func(thread, JSNApiHelper::ToJSTaggedValue(this));
    JSHandle<Method> method(thread, func->GetMethod());
    return method->IsNativeWithCallField();
}

// ----------------------------------- ArrayRef ----------------------------------------
Local<ArrayRef> ArrayRef::New(const EcmaVM *vm, uint32_t length)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSTaggedNumber arrayLen(length);
    JSHandle<JSTaggedValue> array = JSArray::ArrayCreate(thread, arrayLen);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<ArrayRef>(array);
}

uint32_t ArrayRef::Length([[maybe_unused]] const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, 0);
    return JSArray::Cast(JSNApiHelper::ToJSTaggedValue(this).GetTaggedObject())->GetArrayLength();
}

Local<JSValueRef> ArrayRef::GetValueAt(const EcmaVM *vm, Local<JSValueRef> obj, uint32_t index)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> object = JSNApiHelper::ToJSHandle(obj);
    JSHandle<JSTaggedValue> result = JSArray::FastGetPropertyByValue(thread, object, index);
    return JSNApiHelper::ToLocal<JSValueRef>(result);
}

bool ArrayRef::SetValueAt(const EcmaVM *vm, Local<JSValueRef> obj, uint32_t index, Local<JSValueRef> value)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> objectHandle = JSNApiHelper::ToJSHandle(obj);
    JSHandle<JSTaggedValue> valueHandle = JSNApiHelper::ToJSHandle(value);
    return JSArray::FastSetPropertyByValue(thread, objectHandle, index, valueHandle);
}

// ---------------------------------- Promise --------------------------------------
Local<PromiseCapabilityRef> PromiseCapabilityRef::New(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor(globalEnv->GetPromiseFunction());
    JSHandle<JSTaggedValue> capability(JSPromise::NewPromiseCapability(thread, constructor));
    return JSNApiHelper::ToLocal<PromiseCapabilityRef>(capability);
}

Local<PromiseRef> PromiseCapabilityRef::GetPromise(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<PromiseCapability> capacity(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(capacity, FATAL);
    return JSNApiHelper::ToLocal<PromiseRef>(JSHandle<JSTaggedValue>(thread, capacity->GetPromise()));
}

bool PromiseCapabilityRef::Resolve(const EcmaVM *vm, Local<JSValueRef> value)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    JSThread *thread = vm->GetJSThread();
    const GlobalEnvConstants *constants = thread->GlobalConstants();

    JSHandle<JSTaggedValue> arg = JSNApiHelper::ToJSHandle(value);
    JSHandle<PromiseCapability> capacity(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(capacity, FATAL);
    JSHandle<JSTaggedValue> resolve(thread, capacity->GetResolve());
    JSHandle<JSTaggedValue> undefined(constants->GetHandledUndefined());
    EcmaRuntimeCallInfo *info =
        ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, resolve, undefined, undefined, 1);
    RETURN_VALUE_IF_ABRUPT(thread, false);
    info->SetCallArg(arg.GetTaggedValue());
    JSFunction::Call(info);
    RETURN_VALUE_IF_ABRUPT(thread, false);

    thread->GetCurrentEcmaContext()->ExecutePromisePendingJob();
    RETURN_VALUE_IF_ABRUPT(thread, false);
    vm->GetHeap()->ClearKeptObjects();
    return true;
}

bool PromiseCapabilityRef::Reject(const EcmaVM *vm, Local<JSValueRef> reason)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    JSThread *thread = vm->GetJSThread();
    const GlobalEnvConstants *constants = thread->GlobalConstants();

    JSHandle<JSTaggedValue> arg = JSNApiHelper::ToJSHandle(reason);
    JSHandle<PromiseCapability> capacity(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(capacity, FATAL);
    JSHandle<JSTaggedValue> reject(thread, capacity->GetReject());
    JSHandle<JSTaggedValue> undefined(constants->GetHandledUndefined());

    EcmaRuntimeCallInfo *info =
        ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, reject, undefined, undefined, 1);
    RETURN_VALUE_IF_ABRUPT(thread, false);
    info->SetCallArg(arg.GetTaggedValue());
    JSFunction::Call(info);
    RETURN_VALUE_IF_ABRUPT(thread, false);

    thread->GetCurrentEcmaContext()->ExecutePromisePendingJob();
    RETURN_VALUE_IF_ABRUPT(thread, false);
    vm->GetHeap()->ClearKeptObjects();
    return true;
}

Local<PromiseRef> PromiseRef::Catch(const EcmaVM *vm, Local<FunctionRef> handler)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    const GlobalEnvConstants *constants = thread->GlobalConstants();

    JSHandle<JSTaggedValue> promise = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(promise, ERROR);
    JSHandle<JSTaggedValue> catchKey(thread, constants->GetPromiseCatchString());
    JSHandle<JSTaggedValue> reject = JSNApiHelper::ToJSHandle(handler);
    JSHandle<JSTaggedValue> undefined = constants->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, promise, undefined, 1);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    info->SetCallArg(reject.GetTaggedValue());
    JSTaggedValue result = JSFunction::Invoke(info, catchKey);

    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<PromiseRef>(JSHandle<JSTaggedValue>(thread, result));
}

Local<PromiseRef> PromiseRef::Finally(const EcmaVM *vm, Local<FunctionRef> handler)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    const GlobalEnvConstants *constants = thread->GlobalConstants();

    JSHandle<JSTaggedValue> promise = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(promise, ERROR);
    JSHandle<JSTaggedValue> finallyKey = constants->GetHandledPromiseFinallyString();
    JSHandle<JSTaggedValue> resolver = JSNApiHelper::ToJSHandle(handler);
    JSHandle<JSTaggedValue> undefined(constants->GetHandledUndefined());
    EcmaRuntimeCallInfo *info =
        ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, promise, undefined, 2); // 2: two args
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    info->SetCallArg(resolver.GetTaggedValue(), undefined.GetTaggedValue());
    JSTaggedValue result = JSFunction::Invoke(info, finallyKey);

    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<PromiseRef>(JSHandle<JSTaggedValue>(thread, result));
}

Local<PromiseRef> PromiseRef::Then(const EcmaVM *vm, Local<FunctionRef> handler)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    const GlobalEnvConstants *constants = thread->GlobalConstants();

    JSHandle<JSTaggedValue> promise = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(promise, ERROR);
    JSHandle<JSTaggedValue> thenKey(thread, constants->GetPromiseThenString());
    JSHandle<JSTaggedValue> resolver = JSNApiHelper::ToJSHandle(handler);
    JSHandle<JSTaggedValue> undefined(constants->GetHandledUndefined());
    EcmaRuntimeCallInfo *info =
        ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, promise, undefined, 2); // 2: two args
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    info->SetCallArg(resolver.GetTaggedValue(), undefined.GetTaggedValue());
    JSTaggedValue result = JSFunction::Invoke(info, thenKey);

    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<PromiseRef>(JSHandle<JSTaggedValue>(thread, result));
}

Local<PromiseRef> PromiseRef::Then(const EcmaVM *vm, Local<FunctionRef> onFulfilled, Local<FunctionRef> onRejected)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    const GlobalEnvConstants *constants = thread->GlobalConstants();

    JSHandle<JSTaggedValue> promise = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(promise, ERROR);
    JSHandle<JSTaggedValue> thenKey(thread, constants->GetPromiseThenString());
    JSHandle<JSTaggedValue> resolver = JSNApiHelper::ToJSHandle(onFulfilled);
    JSHandle<JSTaggedValue> reject = JSNApiHelper::ToJSHandle(onRejected);
    JSHandle<JSTaggedValue> undefined(constants->GetHandledUndefined());
    EcmaRuntimeCallInfo *info =
        ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, promise, undefined, 2); // 2: two args
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    info->SetCallArg(resolver.GetTaggedValue(), reject.GetTaggedValue());
    JSTaggedValue result = JSFunction::Invoke(info, thenKey);

    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<PromiseRef>(JSHandle<JSTaggedValue>(thread, result));
}
// ---------------------------------- Promise -------------------------------------

// ---------------------------------- Buffer -----------------------------------
Local<ArrayBufferRef> ArrayBufferRef::New(const EcmaVM *vm, int32_t length)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSArrayBuffer> arrayBuffer = factory->NewJSArrayBuffer(length);
    return JSNApiHelper::ToLocal<ArrayBufferRef>(JSHandle<JSTaggedValue>(arrayBuffer));
}

Local<ArrayBufferRef> ArrayBufferRef::New(
    const EcmaVM *vm, void *buffer, int32_t length, const Deleter &deleter, void *data)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSArrayBuffer> arrayBuffer =
        factory->NewJSArrayBuffer(buffer, length, reinterpret_cast<ecmascript::DeleteEntryPoint>(deleter), data);
    return JSNApiHelper::ToLocal<ArrayBufferRef>(JSHandle<JSTaggedValue>(arrayBuffer));
}

int32_t ArrayBufferRef::ByteLength([[maybe_unused]] const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, 0);
    JSHandle<JSArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(arrayBuffer, FATAL);
    return arrayBuffer->GetArrayBufferByteLength();
}

void *ArrayBufferRef::GetBuffer()
{
    JSHandle<JSArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(arrayBuffer, FATAL);
    JSTaggedValue bufferData = arrayBuffer->GetArrayBufferData();
    if (!bufferData.IsJSNativePointer()) {
        return nullptr;
    }
    return JSNativePointer::Cast(bufferData.GetTaggedObject())->GetExternalPointer();
}

void ArrayBufferRef::Detach(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
    arrayBuffer->Detach(thread);
}

bool ArrayBufferRef::IsDetach()
{
    JSHandle<JSArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
    return arrayBuffer->IsDetach();
}

JSTaggedValue BufferRef::BufferToStringCallback(ecmascript::EcmaRuntimeCallInfo *ecmaRuntimeCallInfo)
{
    JSThread *thread = ecmaRuntimeCallInfo->GetThread();
    [[maybe_unused]] LocalScope scope(thread->GetEcmaVM());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> arrayBuff = ecmaRuntimeCallInfo->GetThis();
    JSHandle<JSArrayBuffer> arrayBuffer(arrayBuff);

    uint32_t length = arrayBuffer->GetArrayBufferByteLength();
    JSTaggedValue data = arrayBuffer->GetArrayBufferData();

    ecmascript::CVector<uint16_t> valueTable;
    valueTable.reserve(length);
    for (uint32_t i = 0; i < length; i++) {
        void* rawData = reinterpret_cast<void *>(
            ToUintPtr(JSNativePointer::Cast(data.GetTaggedObject())->GetExternalPointer()) + i);
        uint8_t *block = reinterpret_cast<uint8_t *>(rawData);
        uint16_t nextCv = static_cast<uint16_t>(*block);
        valueTable.emplace_back(nextCv);
    }

    auto *char16tData0 = reinterpret_cast<const char16_t *>(valueTable.data());
    std::u16string u16str(char16tData0, length);

    const char16_t *constChar16tData = u16str.data();
    auto *char16tData = const_cast<char16_t *>(constChar16tData);
    auto *uint16tData = reinterpret_cast<uint16_t *>(char16tData);
    uint32_t u16strSize = u16str.size();
    JSTaggedValue rString = factory->NewFromUtf16Literal(uint16tData, u16strSize).GetTaggedValue();
    JSHandle<EcmaString> StringHandle = JSTaggedValue::ToString(thread, rString);
    RETURN_VALUE_IF_ABRUPT(thread, JSTaggedValue::Undefined());
    return StringHandle.GetTaggedValue();
}

Local<BufferRef> BufferRef::New(const EcmaVM *vm, int32_t length)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    ObjectFactory *factory = vm->GetFactory();
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSArrayBuffer> arrayBuffer = JSHandle<JSArrayBuffer>::Cast(factory->NewJSArrayBuffer(length));
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSFunction> current =
        factory->NewJSFunction(env, reinterpret_cast<void *>(BufferRef::BufferToStringCallback));
    Local<StringRef> key = StringRef::NewFromUtf8(vm, "toString");
    JSHandle<JSTaggedValue> keyValue = JSNApiHelper::ToJSHandle(key);
    JSHandle<JSTaggedValue> currentTaggedValue(current);
    JSHandle<JSTaggedValue> obj(arrayBuffer);
    bool result = JSTaggedValue::SetProperty(vm->GetJSThread(), obj, keyValue, currentTaggedValue);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    if (!result) {
        LOG_ECMA(ERROR) << "SetProperty failed ! ! !";
    }
    return JSNApiHelper::ToLocal<BufferRef>(obj);
}

Local<BufferRef> BufferRef::New(
    const EcmaVM *vm, void *buffer, int32_t length, const Deleter &deleter, void *data)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    ObjectFactory *factory = vm->GetFactory();
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSArrayBuffer> arrayBuffer =
        factory->NewJSArrayBuffer(buffer, length, reinterpret_cast<ecmascript::DeleteEntryPoint>(deleter), data);

    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSFunction> current =
        factory->NewJSFunction(env, reinterpret_cast<void *>(BufferRef::BufferToStringCallback));
    Local<StringRef> key = StringRef::NewFromUtf8(vm, "toString");
    JSHandle<JSTaggedValue> keyValue = JSNApiHelper::ToJSHandle(key);
    JSHandle<JSTaggedValue> currentTaggedValue(current);
    JSHandle<JSTaggedValue> obj(arrayBuffer);
    bool result = JSTaggedValue::SetProperty(vm->GetJSThread(), obj, keyValue, currentTaggedValue);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    if (!result) {
        LOG_ECMA(ERROR) << "SetProperty failed ! ! !";
    }
    return JSNApiHelper::ToLocal<ArrayBufferRef>(JSHandle<JSTaggedValue>(arrayBuffer));
}

int32_t BufferRef::ByteLength([[maybe_unused]] const EcmaVM *vm)
{
    JSHandle<JSArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(arrayBuffer, FATAL);
    return arrayBuffer->GetArrayBufferByteLength();
}

void *BufferRef::GetBuffer()
{
    JSHandle<JSArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(arrayBuffer, FATAL);
    JSTaggedValue bufferData = arrayBuffer->GetArrayBufferData();
    if (!bufferData.IsJSNativePointer()) {
        return nullptr;
    }
    return JSNativePointer::Cast(bufferData.GetTaggedObject())->GetExternalPointer();
}
// ---------------------------------- Buffer -----------------------------------

// ---------------------------------- DataView -----------------------------------
Local<DataViewRef> DataViewRef::New(
    const EcmaVM *vm, Local<ArrayBufferRef> arrayBuffer, uint32_t byteOffset, uint32_t byteLength)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    ObjectFactory *factory = vm->GetFactory();

    JSHandle<JSArrayBuffer> buffer(JSNApiHelper::ToJSHandle(arrayBuffer));
    JSHandle<JSDataView> dataView = factory->NewJSDataView(buffer, byteOffset, byteLength);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<DataViewRef>(JSHandle<JSTaggedValue>(dataView));
}

uint32_t DataViewRef::ByteLength()
{
    JSHandle<JSDataView> dataView(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(dataView, FATAL);
    return dataView->GetByteLength();
}

uint32_t DataViewRef::ByteOffset()
{
    JSHandle<JSDataView> dataView(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(dataView, FATAL);
    return dataView->GetByteOffset();
}

Local<ArrayBufferRef> DataViewRef::GetArrayBuffer(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSDataView> dataView(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(dataView, FATAL);
    JSHandle<JSTaggedValue> arrayBuffer(thread, dataView->GetViewedArrayBuffer());
    return JSNApiHelper::ToLocal<ArrayBufferRef>(arrayBuffer);
}
// ---------------------------------- DataView -----------------------------------

// ---------------------------------- TypedArray -----------------------------------
uint32_t TypedArrayRef::ByteLength([[maybe_unused]] const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, 0);
    JSHandle<JSTypedArray> typedArray(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(typedArray, FATAL);
    return typedArray->GetByteLength();
}

uint32_t TypedArrayRef::ByteOffset([[maybe_unused]] const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, 0);
    JSHandle<JSTypedArray> typedArray(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(typedArray, FATAL);
    return typedArray->GetByteOffset();
}

uint32_t TypedArrayRef::ArrayLength([[maybe_unused]] const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, 0);
    JSHandle<JSTypedArray> typedArray(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(typedArray, FATAL);
    return typedArray->GetArrayLength();
}

Local<ArrayBufferRef> TypedArrayRef::GetArrayBuffer(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTypedArray> typeArray(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(typeArray, ERROR);
    JSHandle<JSTaggedValue> arrayBuffer(thread, JSTypedArray::GetOffHeapBuffer(thread, typeArray));
    return JSNApiHelper::ToLocal<ArrayBufferRef>(arrayBuffer);
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TYPED_ARRAY_NEW(Type)                                                                             \
    Local<Type##Ref> Type##Ref::New(                                                                      \
        const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset, int32_t length)               \
    {                                                                                                     \
        CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);                                                 \
        JSThread *thread = vm->GetJSThread();                                                             \
        JSHandle<GlobalEnv> env = vm->GetGlobalEnv();                                                     \
                                                                                                          \
        JSHandle<JSTaggedValue> func = env->Get##Type##Function();                                        \
        JSHandle<JSArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(buffer));                            \
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();             \
        const uint32_t argsLength = 3;                                                                     \
        EcmaRuntimeCallInfo *info =                                                                       \
            ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, func, undefined, func, argsLength);   \
        RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));                                        \
        info->SetCallArg(arrayBuffer.GetTaggedValue(), JSTaggedValue(byteOffset), JSTaggedValue(length)); \
        JSTaggedValue result = JSFunction::Construct(info);                                               \
        RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));                                        \
        JSHandle<JSTaggedValue> resultHandle(thread, result);                                             \
        return JSNApiHelper::ToLocal<Type##Ref>(resultHandle);                                            \
    }

TYPED_ARRAY_ALL(TYPED_ARRAY_NEW)

#undef TYPED_ARRAY_NEW
// ---------------------------------- TypedArray -----------------------------------

// ---------------------------------- Error ---------------------------------------
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EXCEPTION_ERROR_NEW(name, type)                                                     \
    Local<JSValueRef> Exception::name(const EcmaVM *vm, Local<StringRef> message)           \
    {                                                                                       \
        CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);                                   \
        JSThread *thread = vm->GetJSThread();                                               \
        if (thread->HasPendingException()) {                                                \
            thread->ClearException();                                                       \
        }                                                                                   \
        ObjectFactory *factory = vm->GetFactory();                                          \
                                                                                            \
        JSHandle<EcmaString> messageValue(JSNApiHelper::ToJSHandle(message));               \
        JSHandle<JSTaggedValue> result(factory->NewJSError(ErrorType::type, messageValue)); \
        return JSNApiHelper::ToLocal<JSValueRef>(result);                                   \
    }

EXCEPTION_ERROR_ALL(EXCEPTION_ERROR_NEW)

#undef EXCEPTION_ERROR_NEW
// ---------------------------------- Error ---------------------------------------

// ---------------------------------- JSON ------------------------------------------
Local<JSValueRef> JSON::Parse(const EcmaVM *vm, Local<StringRef> string)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    auto ecmaStr = EcmaString::Cast(JSNApiHelper::ToJSTaggedValue(*string).GetTaggedObject());
    JSHandle<JSTaggedValue> result;
    if (EcmaStringAccessor(ecmaStr).IsUtf8()) {
        JsonParser<uint8_t> parser(thread);
        result = parser.ParseUtf8(EcmaString::Cast(JSNApiHelper::ToJSTaggedValue(*string).GetTaggedObject()));
    } else {
        JsonParser<uint16_t> parser(thread);
        result = parser.ParseUtf16(EcmaString::Cast(JSNApiHelper::ToJSTaggedValue(*string).GetTaggedObject()));
    }
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<JSValueRef>(result);
}

Local<JSValueRef> JSON::Stringify(const EcmaVM *vm, Local<JSValueRef> json)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    auto constants = thread->GlobalConstants();
    JsonStringifier stringifier(thread);
    JSHandle<JSTaggedValue> str = stringifier.Stringify(
        JSNApiHelper::ToJSHandle(json), constants->GetHandledUndefined(), constants->GetHandledUndefined());
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<JSValueRef>(str);
}

Local<StringRef> RegExpRef::GetOriginalSource(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSRegExp> regExp(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(regExp, FATAL);
    JSTaggedValue source = regExp->GetOriginalSource();
    if (!source.IsString()) {
        auto constants = thread->GlobalConstants();
        return JSNApiHelper::ToLocal<StringRef>(constants->GetHandledEmptyString());
    }
    JSHandle<JSTaggedValue> sourceHandle(thread, source);
    return JSNApiHelper::ToLocal<StringRef>(sourceHandle);
}

std::string RegExpRef::GetOriginalFlags()
{
    JSHandle<JSRegExp> regExp(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(regExp, FATAL);
    JSTaggedValue regExpFlags = regExp->GetOriginalFlags();
    uint32_t regExpFlagsInt = static_cast<uint32_t>(regExpFlags.GetInt());
    std::string strFlags = "";
    if (regExpFlagsInt & RegExpParser::FLAG_GLOBAL) {
        strFlags += "g";
    }
    if (regExpFlagsInt & RegExpParser::FLAG_IGNORECASE) {
        strFlags += "i";
    }
    if (regExpFlagsInt & RegExpParser::FLAG_MULTILINE) {
        strFlags += "m";
    }
    if (regExpFlagsInt & RegExpParser::FLAG_DOTALL) {
        strFlags += "s";
    }
    if (regExpFlagsInt & RegExpParser::FLAG_UTF16) {
        strFlags += "u";
    }
    if (regExpFlagsInt & RegExpParser::FLAG_STICKY) {
        strFlags += "y";
    }
    std::sort(strFlags.begin(), strFlags.end());
    return strFlags;
}

Local<JSValueRef> RegExpRef::IsGlobal(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSHandle<JSRegExp> regExp(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(regExp, FATAL);
    JSTaggedValue flags = regExp->GetOriginalFlags();
    bool result = flags.GetInt() & RegExpParser::FLAG_GLOBAL;
    Local<JSValueRef> jsValue = BooleanRef::New(vm, result);
    return jsValue;
}

Local<JSValueRef> RegExpRef::IsIgnoreCase(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSHandle<JSRegExp> regExp(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(regExp, FATAL);
    JSTaggedValue flags = regExp->GetOriginalFlags();
    bool result = flags.GetInt() & RegExpParser::FLAG_IGNORECASE;
    Local<JSValueRef> jsValue = BooleanRef::New(vm, result);
    return jsValue;
}

Local<JSValueRef> RegExpRef::IsMultiline(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSHandle<JSRegExp> regExp(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(regExp, FATAL);
    JSTaggedValue flags = regExp->GetOriginalFlags();
    bool result = flags.GetInt() & RegExpParser::FLAG_MULTILINE;
    Local<JSValueRef> jsValue = BooleanRef::New(vm, result);
    return jsValue;
}

Local<JSValueRef> RegExpRef::IsDotAll(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSHandle<JSRegExp> regExp(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(regExp, FATAL);
    JSTaggedValue flags = regExp->GetOriginalFlags();
    bool result = flags.GetInt() & RegExpParser::FLAG_DOTALL;
    Local<JSValueRef> jsValue = BooleanRef::New(vm, result);
    return jsValue;
}

Local<JSValueRef> RegExpRef::IsUtf16(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSHandle<JSRegExp> regExp(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(regExp, FATAL);
    JSTaggedValue flags = regExp->GetOriginalFlags();
    bool result = flags.GetInt() & RegExpParser::FLAG_UTF16;
    Local<JSValueRef> jsValue = BooleanRef::New(vm, result);
    return jsValue;
}

Local<JSValueRef> RegExpRef::IsStick(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSHandle<JSRegExp> regExp(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(regExp, FATAL);
    JSTaggedValue flags = regExp->GetOriginalFlags();
    bool result = flags.GetInt() & RegExpParser::FLAG_STICKY;
    Local<JSValueRef> jsValue = BooleanRef::New(vm, result);
    return jsValue;
}

Local<DateRef> DateRef::New(const EcmaVM *vm, double time)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
    JSHandle<JSFunction> dateFunction(globalEnv->GetDateFunction());
    JSHandle<JSDate> dateObject(factory->NewJSObjectByConstructor(dateFunction));
    dateObject->SetTimeValue(thread, JSTaggedValue(time));
    return JSNApiHelper::ToLocal<DateRef>(JSHandle<JSTaggedValue>(dateObject));
}

Local<StringRef> DateRef::ToString(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSDate> date(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(date, ERROR);
    JSTaggedValue dateStr = date->ToString(thread);
    if (!dateStr.IsString()) {
        auto constants = thread->GlobalConstants();
        return JSNApiHelper::ToLocal<StringRef>(constants->GetHandledEmptyString());
    }
    JSHandle<JSTaggedValue> dateStrHandle(thread, dateStr);
    return JSNApiHelper::ToLocal<StringRef>(dateStrHandle);
}

double DateRef::GetTime()
{
    JSHandle<JSDate> date(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(date, ERROR);
    if (!date->IsDate()) {
        LOG_ECMA(ERROR) << "Not a Date Object";
    }
    return date->GetTime().GetDouble();
}

Local<JSValueRef> ProxyRef::GetHandler(const EcmaVM *vm)
{
    JSHandle<JSProxy> jsProxy(JSNApiHelper::ToJSHandle(this));
    JSThread *thread = vm->GetJSThread();
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, jsProxy->GetHandler()));
}

Local<JSValueRef> ProxyRef::GetTarget(const EcmaVM *vm)
{
    JSHandle<JSProxy> jsProxy(JSNApiHelper::ToJSHandle(this));
    JSThread *thread = vm->GetJSThread();
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, jsProxy->GetTarget()));
}

bool ProxyRef::IsRevoked()
{
    JSHandle<JSProxy> jsProxy(JSNApiHelper::ToJSHandle(this));
    return jsProxy->GetIsRevoked();
}

Local<JSValueRef> MapRef::Get(const EcmaVM *vm, Local<JSValueRef> key)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSHandle<JSMap> map(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(map, FATAL);
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(vm->GetJSThread(),
                map->Get(JSNApiHelper::ToJSTaggedValue(*key))));
}

void MapRef::Set(const EcmaVM *vm, Local<JSValueRef> key, Local<JSValueRef> value)
{
    CHECK_HAS_PENDING_EXCEPTION_WITHOUT_RETURN(vm);
    JSHandle<JSMap> map(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(map, ERROR);
    JSMap::Set(vm->GetJSThread(), map, JSNApiHelper::ToJSHandle(key), JSNApiHelper::ToJSHandle(value));
}

Local<MapRef> MapRef::New(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsMapFunction();
    JSHandle<JSMap> map =
        JSHandle<JSMap>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashMap> hashMap = LinkedHashMap::Create(thread);
    map->SetLinkedMap(thread, hashMap);
    JSHandle<JSTaggedValue> mapTag = JSHandle<JSTaggedValue>::Cast(map);
    return JSNApiHelper::ToLocal<MapRef>(mapTag);
}

int32_t MapRef::GetSize()
{
    JSHandle<JSMap> map(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(map, FATAL);
    return map->GetSize();
}

int32_t MapRef::GetTotalElements()
{
    JSHandle<JSMap> map(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(map, FATAL);
    return static_cast<int>((map->GetSize())) +
        LinkedHashMap::Cast(map->GetLinkedMap().GetTaggedObject())->NumberOfDeletedElements();
}

Local<JSValueRef> MapRef::GetKey(const EcmaVM *vm, int entry)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSHandle<JSMap> map(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(map, FATAL);
    JSThread *thread = vm->GetJSThread();
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, map->GetKey(entry)));
}

Local<JSValueRef> MapRef::GetValue(const EcmaVM *vm, int entry)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSHandle<JSMap> map(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(map, FATAL);
    JSThread *thread = vm->GetJSThread();
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, map->GetValue(entry)));
}

int32_t WeakMapRef::GetSize()
{
    JSHandle<JSWeakMap> weakMap(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(weakMap, FATAL);
    return weakMap->GetSize();
}

int32_t WeakMapRef::GetTotalElements()
{
    JSHandle<JSWeakMap> weakMap(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(weakMap, FATAL);
    return weakMap->GetSize() +
                LinkedHashMap::Cast(weakMap->GetLinkedMap().GetTaggedObject())->NumberOfDeletedElements();
}

Local<JSValueRef> WeakMapRef::GetKey(const EcmaVM *vm, int entry)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSHandle<JSWeakMap> weakMap(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(weakMap, FATAL);
    JSTaggedValue key = weakMap->GetKey(entry);
    JSThread *thread = vm->GetJSThread();
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, key.GetWeakRawValue()));
}

Local<JSValueRef> WeakMapRef::GetValue(const EcmaVM *vm, int entry)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSHandle<JSWeakMap> weakMap(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(weakMap, FATAL);
    JSThread *thread = vm->GetJSThread();
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, weakMap->GetValue(entry)));
}

int32_t SetRef::GetSize()
{
    JSHandle<JSSet> set(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(set, FATAL);
    return set->GetSize();
}

int32_t SetRef::GetTotalElements()
{
    JSHandle<JSSet> set(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(set, FATAL);
    return static_cast<int>(set->GetSize()) +
        LinkedHashSet::Cast(set->GetLinkedSet().GetTaggedObject())->NumberOfDeletedElements();
}

Local<JSValueRef> SetRef::GetValue(const EcmaVM *vm, int entry)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSHandle<JSSet> set(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(set, FATAL);
    JSThread *thread = vm->GetJSThread();
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, set->GetValue(entry)));
}

int32_t WeakSetRef::GetSize()
{
    JSHandle<JSWeakSet> weakSet(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(weakSet, FATAL);
    return weakSet->GetSize();
}

int32_t WeakSetRef::GetTotalElements()
{
    JSHandle<JSWeakSet> weakSet(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(weakSet, FATAL);
    return weakSet->GetSize() +
                LinkedHashSet::Cast(weakSet->GetLinkedSet().GetTaggedObject())->NumberOfDeletedElements();
}

Local<JSValueRef> WeakSetRef::GetValue(const EcmaVM *vm, int entry)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSHandle<JSWeakSet> weakSet(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(weakSet, FATAL);
    JSTaggedValue value = weakSet->GetValue(entry);
    JSThread *thread = vm->GetJSThread();
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, value.GetWeakRawValue()));
}

int32_t MapIteratorRef::GetIndex()
{
    JSHandle<JSMapIterator> jsMapIter(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(jsMapIter, FATAL);
    return jsMapIter->GetNextIndex();
}

Local<JSValueRef> MapIteratorRef::GetKind(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSHandle<JSMapIterator> jsMapIter(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(jsMapIter, FATAL);
    IterationKind iterKind = jsMapIter->GetIterationKind();
    Local<JSValueRef> result;
    switch (iterKind) {
        case IterationKind::KEY:
            result = StringRef::NewFromUtf8(vm, "keys");
            break;
        case IterationKind::VALUE:
            result = StringRef::NewFromUtf8(vm, "values");
            break;
        case IterationKind::KEY_AND_VALUE:
            result = StringRef::NewFromUtf8(vm, "entries");
            break;
        default:
            break;
    }
    return result;
}

int32_t SetIteratorRef::GetIndex()
{
    JSHandle<JSSetIterator> jsSetIter(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(jsSetIter, FATAL);
    return jsSetIter->GetNextIndex();
}

Local<JSValueRef> SetIteratorRef::GetKind(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSHandle<JSSetIterator> jsSetIter(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(jsSetIter, FATAL);
    IterationKind iterKind = jsSetIter->GetIterationKind();
    Local<JSValueRef> result;
    switch (iterKind) {
        case IterationKind::KEY:
            result = StringRef::NewFromUtf8(vm, "keys");
            break;
        case IterationKind::VALUE:
            result = StringRef::NewFromUtf8(vm, "values");
            break;
        case IterationKind::KEY_AND_VALUE:
            result = StringRef::NewFromUtf8(vm, "entries");
            break;
        default:
            break;
    }
    return result;
}

bool GeneratorFunctionRef::IsGenerator()
{
    return IsGeneratorFunction();
}

Local<JSValueRef> GeneratorObjectRef::GetGeneratorState(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSHandle<JSGeneratorObject> jsGenerator(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(jsGenerator, FATAL);
    if (jsGenerator->GetGeneratorState() == JSGeneratorState::COMPLETED) {
        return StringRef::NewFromUtf8(vm, "closed");
    }
    return StringRef::NewFromUtf8(vm, "suspended");
}

Local<JSValueRef> GeneratorObjectRef::GetGeneratorFunction(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSGeneratorObject> jsGenerator(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(jsGenerator, FATAL);
    JSHandle<GeneratorContext> generatorContext(thread, jsGenerator->GetGeneratorContext());
    JSTaggedValue jsTagValue = generatorContext->GetMethod();
    return JSNApiHelper::ToLocal<GeneratorFunctionRef>(JSHandle<JSTaggedValue>(thread, jsTagValue));
}

Local<JSValueRef> GeneratorObjectRef::GetGeneratorReceiver(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSGeneratorObject> jsGenerator(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(jsGenerator, FATAL);
    JSHandle<GeneratorContext> generatorContext(thread, jsGenerator->GetGeneratorContext());
    JSTaggedValue jsTagValue = generatorContext->GetAcc();
    return JSNApiHelper::ToLocal<GeneratorObjectRef>(JSHandle<JSTaggedValue>(thread, jsTagValue));
}

Local<JSValueRef> CollatorRef::GetCompareFunction(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
#ifdef ARK_SUPPORT_INTL
    JSHandle<JSCollator> jsCollator(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(jsCollator, FATAL);
    JSTaggedValue jsTagValue = jsCollator->GetBoundCompare();
    return JSNApiHelper::ToLocal<CollatorRef>(JSHandle<JSTaggedValue>(thread, jsTagValue));
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler intl";
    return JSNApiHelper::ToLocal<CollatorRef>(JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
#endif
}

Local<JSValueRef> DataTimeFormatRef::GetFormatFunction(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
#ifdef ARK_SUPPORT_INTL
    JSHandle<JSDateTimeFormat> jsDateTimeFormat(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(jsDateTimeFormat, FATAL);
    JSTaggedValue jsTagValue = jsDateTimeFormat->GetBoundFormat();
    return JSNApiHelper::ToLocal<DataTimeFormatRef>(JSHandle<JSTaggedValue>(thread, jsTagValue));
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler intl";
    return JSNApiHelper::ToLocal<DataTimeFormatRef>(JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
#endif
}

Local<JSValueRef> NumberFormatRef::GetFormatFunction(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
#ifdef ARK_SUPPORT_INTL
    JSHandle<JSNumberFormat> jsNumberFormat(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(jsNumberFormat, FATAL);
    JSTaggedValue jsTagValue = jsNumberFormat->GetBoundFormat();
    return JSNApiHelper::ToLocal<NumberFormatRef>(JSHandle<JSTaggedValue>(thread, jsTagValue));
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler intl";
    return JSNApiHelper::ToLocal<NumberFormatRef>(JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
#endif
}

// ----------------------------------- FunctionCallback ---------------------------------
JSTaggedValue Callback::RegisterCallback(ecmascript::EcmaRuntimeCallInfo *ecmaRuntimeCallInfo)
{
    // Constructor
    JSThread *thread = ecmaRuntimeCallInfo->GetThread();
    JSHandle<JSTaggedValue> constructor = BuiltinsBase::GetConstructor(ecmaRuntimeCallInfo);
    if (!constructor->IsJSFunction()) {
        return JSTaggedValue::False();
    }
    [[maybe_unused]] LocalScope scope(thread->GetEcmaVM());
    JSHandle<JSFunction> function(constructor);
    JSTaggedValue extraInfoValue = function->GetFunctionExtraInfo();
    if (!extraInfoValue.IsJSNativePointer()) {
        return JSTaggedValue::False();
    }
    JSHandle<JSNativePointer> extraInfo(thread, extraInfoValue);
    // callBack
    FunctionCallback nativeFunc = reinterpret_cast<FunctionCallback>(extraInfo->GetExternalPointer());

    JsiRuntimeCallInfo jsiRuntimeCallInfo(ecmaRuntimeCallInfo, extraInfo->GetData());
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    bool getStackBeforeCallNapiSuccess = false;
    if (thread->GetIsProfiling() && function->IsCallNapi()) {
        getStackBeforeCallNapiSuccess = thread->GetEcmaVM()->GetProfiler()->GetStackBeforeCallNapi(thread);
    }
#endif
    Local<JSValueRef> result = nativeFunc(&jsiRuntimeCallInfo);
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    if (thread->GetIsProfiling() && function->IsCallNapi() && getStackBeforeCallNapiSuccess) {
        thread->GetEcmaVM()->GetProfiler()->GetStackAfterCallNapi(thread);
    }
#endif
    return JSNApiHelper::ToJSHandle(result).GetTaggedValue();
}

// -------------------------------------  JSExecutionScope ------------------------------
JSExecutionScope::JSExecutionScope([[maybe_unused]] const EcmaVM *vm)
{
}

JSExecutionScope::~JSExecutionScope()
{
    lastCurrentThread_ = nullptr;
    isRevert_ = false;
}

// ----------------------------------- JSValueRef --------------------------------------
Local<PrimitiveRef> JSValueRef::Undefined(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    const GlobalEnvConstants *constants = thread->GlobalConstants();
    return JSNApiHelper::ToLocal<PrimitiveRef>(constants->GetHandledUndefined());
}

Local<PrimitiveRef> JSValueRef::Null(const EcmaVM *vm)
{
    return JSNApiHelper::ToLocal<PrimitiveRef>(JSHandle<JSTaggedValue>(vm->GetJSThread(), JSTaggedValue::Null()));
}

Local<PrimitiveRef> JSValueRef::True(const EcmaVM *vm)
{
    return JSNApiHelper::ToLocal<PrimitiveRef>(JSHandle<JSTaggedValue>(vm->GetJSThread(), JSTaggedValue::True()));
}

Local<PrimitiveRef> JSValueRef::False(const EcmaVM *vm)
{
    return JSNApiHelper::ToLocal<PrimitiveRef>(JSHandle<JSTaggedValue>(vm->GetJSThread(), JSTaggedValue::False()));
}

Local<ObjectRef> JSValueRef::ToObject(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    if (IsUndefined() || IsNull()) {
        return Undefined(vm);
    }
    JSHandle<JSTaggedValue> obj(JSTaggedValue::ToObject(thread, JSNApiHelper::ToJSHandle(this)));
    LOG_IF_SPECIAL(obj, ERROR);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<ObjectRef>(obj);
}

Local<StringRef> JSValueRef::ToString(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    if (!obj->IsString()) {
        obj = JSHandle<JSTaggedValue>(JSTaggedValue::ToString(thread, obj));
        RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    }
    return JSNApiHelper::ToLocal<StringRef>(obj);
}

Local<NativePointerRef> JSValueRef::ToNativePointer(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<NativePointerRef>(obj);
}

bool JSValueRef::BooleaValue()
{
    return JSNApiHelper::ToJSTaggedValue(this).ToBoolean();
}

int64_t JSValueRef::IntegerValue(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, 0);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> tagged = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(tagged, ERROR);
    if (tagged->IsNumber()) {
        if (!NumberHelper::IsFinite(tagged.GetTaggedValue()) || NumberHelper::IsNaN(tagged.GetTaggedValue())) {
            return 0;
        } else {
            return NumberHelper::DoubleToInt64(tagged->GetNumber());
        }
    }
    JSTaggedNumber number = JSTaggedValue::ToInteger(thread, tagged);
    RETURN_VALUE_IF_ABRUPT(thread, 0);
    return NumberHelper::DoubleToInt64(number.GetNumber());
}

uint32_t JSValueRef::Uint32Value(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, 0);
    JSThread *thread = vm->GetJSThread();
    uint32_t number = JSTaggedValue::ToUint32(thread, JSNApiHelper::ToJSHandle(this));
    RETURN_VALUE_IF_ABRUPT(thread, 0);
    return number;
}

int32_t JSValueRef::Int32Value(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, 0);
    JSThread *thread = vm->GetJSThread();
    int32_t number = JSTaggedValue::ToInt32(thread, JSNApiHelper::ToJSHandle(this));
    RETURN_VALUE_IF_ABRUPT(thread, 0);
    return number;
}

Local<BooleanRef> JSValueRef::ToBoolean(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    JSHandle<JSTaggedValue> booleanObj(thread, JSTaggedValue(obj->ToBoolean()));
    return JSNApiHelper::ToLocal<BooleanRef>(booleanObj);
}

Local<NumberRef> JSValueRef::ToNumber(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    JSHandle<JSTaggedValue> number(thread, JSTaggedValue::ToNumber(thread, obj));
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<NumberRef>(number);
}

bool JSValueRef::IsStrictEquals(const EcmaVM *vm, Local<JSValueRef> value)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> xValue = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(xValue, ERROR);
    JSHandle<JSTaggedValue> yValue = JSNApiHelper::ToJSHandle(value);
    return JSTaggedValue::StrictEqual(thread, xValue, yValue);
}

Local<StringRef> JSValueRef::Typeof(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED(vm);
    JSThread *thread = vm->GetJSThread();
    JSTaggedValue value = FastRuntimeStub::FastTypeOf(thread, JSNApiHelper::ToJSTaggedValue(this));
    return JSNApiHelper::ToLocal<StringRef>(JSHandle<JSTaggedValue>(thread, value));
}

bool JSValueRef::InstanceOf(const EcmaVM *vm, Local<JSValueRef> value)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> origin = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(origin, ERROR);
    JSHandle<JSTaggedValue> target = JSNApiHelper::ToJSHandle(value);
    bool result = JSObject::InstanceOf(thread, origin, target);
    RETURN_VALUE_IF_ABRUPT(thread, false);
    return result;
}

bool JSValueRef::IsUndefined()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsUndefined();
}

bool JSValueRef::IsNull()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsNull();
}

bool JSValueRef::IsHole()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsHole();
}

bool JSValueRef::IsTrue()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsTrue();
}

bool JSValueRef::IsFalse()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsFalse();
}

bool JSValueRef::IsNumber()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsNumber();
}

bool JSValueRef::IsBigInt()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsBigInt();
}

bool JSValueRef::IsInt()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsInt();
}

bool JSValueRef::WithinInt32()
{
    return JSNApiHelper::ToJSTaggedValue(this).WithinInt32();
}

bool JSValueRef::IsBoolean()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsBoolean();
}

bool JSValueRef::IsString()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsString();
}

bool JSValueRef::IsSymbol()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsSymbol();
}

bool JSValueRef::IsObject()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsECMAObject();
}

bool JSValueRef::IsArray(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    JSThread *thread = vm->GetJSThread();
    return JSNApiHelper::ToJSTaggedValue(this).IsArray(thread);
}

bool JSValueRef::IsJSArray(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    return JSNApiHelper::ToJSTaggedValue(this).IsJSArray();
}

bool JSValueRef::IsConstructor()
{
    JSTaggedValue value = JSNApiHelper::ToJSTaggedValue(this);
    return value.IsHeapObject() && value.IsConstructor();
}

bool JSValueRef::IsFunction()
{
    JSTaggedValue value = JSNApiHelper::ToJSTaggedValue(this);
    return value.IsHeapObject() && value.IsCallable();
}

bool JSValueRef::IsProxy()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSProxy();
}

bool JSValueRef::IsPromise()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSPromise();
}

bool JSValueRef::IsDataView()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsDataView();
}

bool JSValueRef::IsTypedArray()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsTypedArray();
}

bool JSValueRef::IsNativePointer()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSNativePointer();
}

bool JSValueRef::IsDate()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsDate();
}

bool JSValueRef::IsError()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSError();
}

bool JSValueRef::IsMap()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSMap();
}

bool JSValueRef::IsSet()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSSet();
}

bool JSValueRef::IsWeakRef()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSWeakRef();
}

bool JSValueRef::IsWeakMap()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSWeakMap();
}

bool JSValueRef::IsWeakSet()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSWeakSet();
}

bool JSValueRef::IsRegExp()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSRegExp();
}

bool JSValueRef::IsArrayIterator()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSArrayIterator();
}

bool JSValueRef::IsStringIterator()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsStringIterator();
}

bool JSValueRef::IsSetIterator()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSSetIterator();
}

bool JSValueRef::IsMapIterator()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSMapIterator();
}

bool JSValueRef::IsArrayBuffer()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsArrayBuffer();
}

bool JSValueRef::IsBuffer()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsArrayBuffer();
}

bool JSValueRef::IsUint8Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSUint8Array();
}

bool JSValueRef::IsInt8Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSInt8Array();
}

bool JSValueRef::IsUint8ClampedArray()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSUint8ClampedArray();
}

bool JSValueRef::IsInt16Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSInt16Array();
}

bool JSValueRef::IsUint16Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSUint16Array();
}

bool JSValueRef::IsInt32Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSInt32Array();
}

bool JSValueRef::IsUint32Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSUint32Array();
}

bool JSValueRef::IsFloat32Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSFloat32Array();
}

bool JSValueRef::IsFloat64Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSFloat64Array();
}

bool JSValueRef::IsBigInt64Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSBigInt64Array();
}

bool JSValueRef::IsBigUint64Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSBigUint64Array();
}

bool JSValueRef::IsJSPrimitiveRef()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSPrimitiveRef();
}

bool JSValueRef::IsJSPrimitiveNumber()
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, FATAL);
    return JSPrimitiveRef::Cast(obj->GetTaggedObject())->IsNumber();
}

bool JSValueRef::IsJSPrimitiveInt()
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, FATAL);
    return JSPrimitiveRef::Cast(obj->GetTaggedObject())->IsInt();
}

bool JSValueRef::IsJSPrimitiveBoolean()
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, FATAL);
    return JSPrimitiveRef::Cast(obj->GetTaggedObject())->IsBoolean();
}

bool JSValueRef::IsJSPrimitiveString()
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, FATAL);
    return JSPrimitiveRef::Cast(obj->GetTaggedObject())->IsString();
}

bool JSValueRef::IsJSPrimitiveSymbol()
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, FATAL);
    return JSPrimitiveRef::Cast(obj->GetTaggedObject())->IsSymbol();
}

bool JSValueRef::IsGeneratorObject()
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    bool rst = obj->IsGeneratorObject();
    return rst;
}

bool JSValueRef::IsModuleNamespaceObject()
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    bool rst = obj->IsModuleNamespace();
    return rst;
}

bool JSValueRef::IsSharedArrayBuffer()
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    bool rst = obj->IsSharedArrayBuffer();
    return rst;
}

bool JSValueRef::IsJSLocale()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSLocale();
}

bool JSValueRef::IsJSDateTimeFormat()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSDateTimeFormat();
}

bool JSValueRef::IsJSRelativeTimeFormat()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSRelativeTimeFormat();
}

bool JSValueRef::IsJSIntl()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSIntl();
}

bool JSValueRef::IsJSNumberFormat()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSNumberFormat();
}

bool JSValueRef::IsJSCollator()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSCollator();
}

bool JSValueRef::IsJSPluralRules()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSPluralRules();
}

bool JSValueRef::IsJSListFormat()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSListFormat();
}

bool JSValueRef::IsAsyncGeneratorObject()
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    return obj->IsAsyncGeneratorObject();
}

bool JSValueRef::IsAsyncFunction()
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    bool rst = obj->IsJSAsyncFunction();
    return rst;
}

bool JSValueRef::IsArgumentsObject()
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    bool rst = obj->IsArguments();
    return rst;
}

bool JSValueRef::IsGeneratorFunction()
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    bool rst = obj->IsGeneratorFunction();
    return rst;
}

bool JSValueRef::IsAsyncGeneratorFunction()
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    return obj->IsAsyncGeneratorFunction();
}

bool JSValueRef::IsArrayList()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIArrayList();
}

bool JSValueRef::IsDeque()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIDeque();
}

bool JSValueRef::IsHashMap()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIHashMap();
}

bool JSValueRef::IsHashSet()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIHashSet();
}

bool JSValueRef::IsLightWeightMap()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPILightWeightMap();
}

bool JSValueRef::IsLightWeightSet()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPILightWeightSet();
}

bool JSValueRef::IsLinkedList()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPILinkedList();
}

bool JSValueRef::IsLinkedListIterator()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPILinkedListIterator();
}

bool JSValueRef::IsList()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIList();
}

bool JSValueRef::IsPlainArray()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIPlainArray();
}

bool JSValueRef::IsQueue()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIQueue();
}

bool JSValueRef::IsStack()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIStack();
}

bool JSValueRef::IsTreeMap()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPITreeMap();
}

bool JSValueRef::IsTreeSet()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPITreeSet();
}

bool JSValueRef::IsVector()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIVector();
}

// ------------------------------------ JsiRuntimeCallInfo -----------------------------------------------
JsiRuntimeCallInfo::JsiRuntimeCallInfo(ecmascript::EcmaRuntimeCallInfo* ecmaInfo, void* data)
    : thread_(ecmaInfo->GetThread()), numArgs_(ecmaInfo->GetArgsNumber())
{
    stackArgs_ = ecmaInfo->GetArgs();
    data_ = data;
}

EcmaVM *JsiRuntimeCallInfo::GetVM() const
{
    return thread_->GetEcmaVM();
}

// ---------------------------------------HotPatch--HotReload-------------------------------------------
PatchErrorCode JSNApi::LoadPatch(EcmaVM *vm, const std::string &patchFileName, const std::string &baseFileName)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, PatchErrorCode::INTERNAL_ERROR);
    ecmascript::QuickFixManager *quickFixManager = vm->GetQuickFixManager();
    JSThread *thread = vm->GetJSThread();
    return quickFixManager->LoadPatch(thread, patchFileName, baseFileName);
}

PatchErrorCode JSNApi::LoadPatch(EcmaVM *vm,
                                 const std::string &patchFileName, const void *patchBuffer, size_t patchSize,
                                 const std::string &baseFileName, const void *baseBuffer, size_t baseSize)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, PatchErrorCode::INTERNAL_ERROR);
    ecmascript::QuickFixManager *quickFixManager = vm->GetQuickFixManager();
    JSThread *thread = vm->GetJSThread();
    return quickFixManager->LoadPatch(
        thread, patchFileName, patchBuffer, patchSize, baseFileName, baseBuffer, baseSize);
}

PatchErrorCode JSNApi::UnloadPatch(EcmaVM *vm, const std::string &patchFileName)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, PatchErrorCode::INTERNAL_ERROR);
    ecmascript::QuickFixManager *quickFixManager = vm->GetQuickFixManager();
    JSThread *thread = vm->GetJSThread();
    return quickFixManager->UnloadPatch(thread, patchFileName);
}

/*
 * check whether the exception is caused by quickfix methods.
 */
bool JSNApi::IsQuickFixCausedException(EcmaVM *vm, Local<ObjectRef> exception, const std::string &patchFileName)
{
    if (exception.IsEmpty()) {
        return false;
    }
    ecmascript::QuickFixManager *quickFixManager = vm->GetQuickFixManager();
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> exceptionInfo = JSNApiHelper::ToJSHandle(exception);
    return quickFixManager->IsQuickFixCausedException(thread, exceptionInfo, patchFileName);
}

/*
 * register quickfix query function.
 */
void JSNApi::RegisterQuickFixQueryFunc(EcmaVM *vm, std::function<bool(std::string baseFileName,
                        std::string &patchFileName,
                        void **patchBuffer,
                        size_t &patchSize)> callBack)
{
    CHECK_HAS_PENDING_EXCEPTION_WITHOUT_RETURN(vm);
    ecmascript::QuickFixManager *quickFixManager = vm->GetQuickFixManager();
    quickFixManager->RegisterQuickFixQueryFunc(callBack);
}

bool JSNApi::IsBundle(EcmaVM *vm)
{
    return vm->IsBundlePack();
}

void JSNApi::SetBundle(EcmaVM *vm, bool value)
{
    vm->SetIsBundlePack(value);
}

// note: The function SetAssetPath is a generic interface for previewing and physical machines.
void JSNApi::SetAssetPath(EcmaVM *vm, const std::string &assetPath)
{
    ecmascript::CString path = assetPath.c_str();
    vm->SetAssetPath(path);
}

void JSNApi::SetLoop(EcmaVM *vm, void *loop)
{
    vm->SetLoop(loop);
}

std::string JSNApi::GetAssetPath(EcmaVM *vm)
{
    return vm->GetAssetPath().c_str();
}

bool JSNApi::InitForConcurrentThread(EcmaVM *vm, ConcurrentCallback cb, void *data)
{
    vm->SetConcurrentCallback(cb, data);

    return true;
}

bool JSNApi::InitForConcurrentFunction(EcmaVM *vm, Local<JSValueRef> function, void *taskInfo)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, false);
    [[maybe_unused]] LocalScope scope(vm);
    JSHandle<JSTaggedValue> funcVal = JSNApiHelper::ToJSHandle(function);
    JSHandle<JSFunction> transFunc = JSHandle<JSFunction>::Cast(funcVal);
    if (transFunc->GetFunctionKind() != ecmascript::FunctionKind::CONCURRENT_FUNCTION) {
        LOG_ECMA(ERROR) << "Function is not concurrent";
        return false;
    }
    ecmascript::JSThread *thread = vm->GetJSThread();
    transFunc->SetFunctionExtraInfo(thread, nullptr, nullptr, taskInfo);
    JSHandle<Method> method(thread, transFunc->GetMethod());
    const JSPandaFile *jsPandaFile = method->GetJSPandaFile();
    if (jsPandaFile == nullptr) {
        LOG_ECMA(ERROR) << "JSPandaFile is nullptr";
        return false;
    }
    ecmascript::CString moduleName = jsPandaFile->GetJSPandaFileDesc();
    ecmascript::CString recordName = method->GetRecordName();

    // for debugger, to notify the script loaded and parsed which the concurrent function is in
    auto *notificationMgr = vm->GetJsDebuggerManager()->GetNotificationManager();
    notificationMgr->LoadModuleEvent(moduleName, recordName);

    // check ESM or CJS
    ecmascript::JSRecordInfo recordInfo;
    bool hasRecord = jsPandaFile->CheckAndGetRecordInfo(recordName, recordInfo);
    if (!hasRecord) {
        LOG_ECMA(ERROR) << "cannot find record '" << recordName << "', please check the request path.";
        return false;
    }
    if (!jsPandaFile->IsModule(recordInfo)) {
        LOG_ECMA(DEBUG) << "Current function is not from ES Module's file.";
        return true;
    }
    ecmascript::ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<ecmascript::JSTaggedValue> moduleRecord;
    // check compileMode
    if (!jsPandaFile->IsMergedPF()) {
        LOG_ECMA(DEBUG) << "CompileMode is jsbundle";
        moduleRecord = moduleManager->HostResolveImportedModule(moduleName);
    } else {
        LOG_ECMA(DEBUG) << "CompileMode is esmodule";
        moduleRecord = moduleManager->HostResolveImportedModuleWithMerge(moduleName, recordName);
    }
    ecmascript::SourceTextModule::Instantiate(thread, moduleRecord);
    JSHandle<ecmascript::SourceTextModule> module = JSHandle<ecmascript::SourceTextModule>::Cast(moduleRecord);
    module->SetStatus(ecmascript::ModuleStatus::INSTANTIATED);
    ecmascript::SourceTextModule::EvaluateForConcurrent(thread, module);
    transFunc->SetModule(thread, module);
    return true;
}

void* JSNApi::GetCurrentTaskInfo(const EcmaVM *vm)
{
    CHECK_HAS_PENDING_EXCEPTION(vm, nullptr);
    auto thread = vm->GetJSThread();
    JSTaggedType *current = const_cast<JSTaggedType *>(thread->GetCurrentFrame());
    ecmascript::FrameIterator it(current, thread);
    for (; !it.Done(); it.Advance<ecmascript::GCVisitedFlag::VISITED>()) {
        if (!it.IsJSFrame()) {
            continue;
        }
        auto method = it.CheckAndGetMethod();
        if (method == nullptr || method->IsNativeWithCallField() ||
            method->GetFunctionKind() != ecmascript::FunctionKind::CONCURRENT_FUNCTION) {
            continue;
        }
        auto functionObj = it.GetFunction();
        JSHandle<JSFunction> function(thread, functionObj);
        JSTaggedValue extraInfoValue = function->GetFunctionExtraInfo();
        if (!extraInfoValue.IsJSNativePointer()) {
            LOG_ECMA(DEBUG) << "Concurrent function donnot have taskInfo";
            continue;
        }
        JSHandle<JSNativePointer> extraInfo(thread, extraInfoValue);
        return extraInfo->GetData();
    }
    LOG_ECMA(ERROR) << "TaskInfo is nullptr";
    return nullptr;
}

void JSNApi::SetBundleName(EcmaVM *vm, const std::string &bundleName)
{
    ecmascript::CString name = bundleName.c_str();
    vm->SetBundleName(name);
}

std::string JSNApi::GetBundleName(EcmaVM *vm)
{
    return vm->GetBundleName().c_str();
}

void JSNApi::SetModuleName(EcmaVM *vm, const std::string &moduleName)
{
    ecmascript::CString name = moduleName.c_str();
    vm->SetModuleName(name);
}

std::string JSNApi::GetModuleName(EcmaVM *vm)
{
    return vm->GetModuleName().c_str();
}

// Enable cross thread execution.
void JSNApi::AllowCrossThreadExecution(EcmaVM *vm)
{
    vm->GetAssociatedJSThread()->EnableCrossThreadExecution();
}

void JSNApi::SynchronizVMInfo(EcmaVM *vm, const EcmaVM *hostVM)
{
    vm->SetBundleName(hostVM->GetBundleName());
    vm->SetModuleName(hostVM->GetModuleName());
    vm->SetAssetPath(hostVM->GetAssetPath());
    vm->SetIsBundlePack(hostVM->IsBundlePack());
    vm->GetAssociatedJSThread()->GetCurrentEcmaContext()->GetModuleManager()->SetExecuteMode(
        hostVM->GetAssociatedJSThread()->GetCurrentEcmaContext()->GetModuleManager()->GetCurrentMode());
    vm->SetResolveBufferCallback(hostVM->GetResolveBufferCallback());
}

bool JSNApi::IsProfiling(EcmaVM *vm)
{
    return vm->GetProfilerState();
}

void JSNApi::SetProfilerState(const EcmaVM *vm, bool value)
{
    const_cast<EcmaVM*>(vm)->SetProfilerState(value);
}
}  // namespace panda
