/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/js_thread.h"

#include "ecmascript/base/config.h"
#include "ecmascript/mem/tagged_state_word.h"
#include "ecmascript/runtime.h"
#include "ecmascript/debugger/js_debugger_manager.h"
#include "ecmascript/dependent_infos.h"
#include "ecmascript/ic/mega_ic_cache.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/module/module_logger.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/runtime_call_id.h"
#include "macros.h"

#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS) && !defined(PANDA_TARGET_IOS)
#include <sys/resource.h>
#endif

#if defined(ENABLE_EXCEPTION_BACKTRACE)
#include "ecmascript/platform/backtrace.h"
#endif
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#endif
#include "common_components/heap/allocator/region_desc.h"
#include "ecmascript/dfx/vm_thread_control.h"
#include "ecmascript/ecma_global_storage.h"
#include "ecmascript/ic/properties_cache.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/jit/jit.h"
#include "common_interfaces/thread/thread_holder_manager.h"
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/platform/asm_stack.h"

namespace panda::ecmascript {
uintptr_t TaggedStateWord::BASE_ADDRESS = 0;
using CommonStubCSigns = panda::ecmascript::kungfu::CommonStubCSigns;
using BytecodeStubCSigns = panda::ecmascript::kungfu::BytecodeStubCSigns;
using BuiltinsStubCSigns = panda::ecmascript::kungfu::BuiltinsStubCSigns;

void SuspendBarrier::Wait()
{
    while (true) {
        int32_t curCount = passBarrierCount_.load(std::memory_order_relaxed);
        if (LIKELY(curCount > 0)) {
#if defined(PANDA_USE_FUTEX)
            int32_t *addr = reinterpret_cast<int32_t*>(&passBarrierCount_);
            if (futex(addr, FUTEX_WAIT_PRIVATE, curCount, nullptr, nullptr, 0) != 0) {
                if (errno != EAGAIN && errno != EINTR) {
                    LOG_GC(FATAL) << "SuspendBarrier::Wait failed, errno = " << errno;
                    UNREACHABLE();
                }
            }
#else
            sched_yield();
#endif
        } else {
            // Use seq_cst to synchronize memory.
            curCount = passBarrierCount_.load(std::memory_order_seq_cst);
            ASSERT(curCount == 0);
            break;
        }
    }
}

thread_local JSThread *currentThread = nullptr;

JSThread *JSThread::GetCurrent()
{
    return currentThread;
}

// static
void JSThread::RegisterThread(JSThread *jsThread)
{
    Runtime::GetInstance()->RegisterThread(jsThread);
    // If it is not true, we created a new thread for future fork
    if (currentThread == nullptr) {
        currentThread = jsThread;
        if (LIKELY(!g_isEnableCMCGC)) {
            jsThread->UpdateState(ThreadState::NATIVE);
        } else {
            jsThread->GetThreadHolder()->TransferToNative();
        }
    }
}

void JSThread::UnregisterThread(JSThread *jsThread)
{
    if (currentThread == jsThread) {
        if (LIKELY(!g_isEnableCMCGC)) {
            jsThread->UpdateState(ThreadState::TERMINATED);
        } else {
            jsThread->GetThreadHolder()->TransferToNative();
            jsThread->SetAllocBuffer(nullptr);
        }
        currentThread = nullptr;
    } else {
        if (LIKELY(!g_isEnableCMCGC)) {
            // We have created this JSThread instance but hadn't forked it.
            ASSERT(jsThread->GetState() == ThreadState::CREATED);
            jsThread->UpdateState(ThreadState::TERMINATED);
        } else {
            jsThread->GetThreadHolder()->TransferToNative();
            jsThread->SetAllocBuffer(nullptr);
        }
    }
    Runtime::GetInstance()->UnregisterThread(jsThread);
}

// static
JSThread *JSThread::Create(EcmaVM *vm)
{
    auto jsThread = new JSThread(vm);
    vm->SetJSThread(jsThread);
    AsmInterParsedOption asmInterOpt = vm->GetJSOptions().GetAsmInterParsedOption();
    if (asmInterOpt.enableAsm) {
        jsThread->EnableAsmInterpreter();
    }

    jsThread->nativeAreaAllocator_ = vm->GetNativeAreaAllocator();
    jsThread->heapRegionAllocator_ = vm->GetHeapRegionAllocator();
    // algin with 16
    size_t maxStackSize = vm->GetEcmaParamConfiguration().GetMaxStackSize();
    jsThread->glueData_.frameBase_ = static_cast<JSTaggedType *>(
        vm->GetNativeAreaAllocator()->Allocate(sizeof(JSTaggedType) * maxStackSize));
    jsThread->glueData_.currentFrame_ = jsThread->glueData_.frameBase_ + maxStackSize;
    EcmaInterpreter::InitStackFrame(jsThread);

    jsThread->glueData_.stackLimit_ = GetAsmStackLimit();
    jsThread->glueData_.stackStart_ = GetCurrentStackPosition();
    jsThread->glueData_.isEnableMutantArray_ = vm->IsEnableMutantArray();
    jsThread->glueData_.IsEnableElementsKind_ = vm->IsEnableElementsKind();
    jsThread->SetThreadId();

    if (UNLIKELY(g_isEnableCMCGC)) {
        jsThread->glueData_.threadHolder_ = ToUintPtr(ThreadHolder::CreateAndRegisterNewThreadHolder(vm));
    }

    RegisterThread(jsThread);
    return jsThread;
}

JSThread::JSThread(EcmaVM *vm) : id_(os::thread::GetCurrentThreadId()), vm_(vm)
{
    auto chunk = vm->GetChunk();
    if (!vm_->GetJSOptions().EnableGlobalLeakCheck()) {
        globalStorage_ = chunk->New<EcmaGlobalStorage<Node>>(this, vm->GetNativeAreaAllocator());
        newGlobalHandle_ = [this](JSTaggedType value) {
            return globalStorage_->NewGlobalHandle<NodeKind::NORMAL_NODE>(value);
        };
        disposeGlobalHandle_ = [this](uintptr_t nodeAddr) {
            globalStorage_->DisposeGlobalHandle<NodeKind::NORMAL_NODE>(nodeAddr);
        };
        if (Runtime::GetInstance()->IsHybridVm()) {
            newXRefGlobalHandle_ = [this](JSTaggedType value) {
                return globalStorage_->NewGlobalHandle<NodeKind::UNIFIED_NODE>(value);
            };
            disposeXRefGlobalHandle_ = [this](uintptr_t nodeAddr) {
                globalStorage_->DisposeGlobalHandle<NodeKind::UNIFIED_NODE>(nodeAddr);
            };
            setNodeKind_ = [this](NodeKind nodeKind) { globalStorage_->SetNodeKind(nodeKind); };
        }
        setWeak_ = [this](uintptr_t nodeAddr, void *ref, WeakClearCallback freeGlobalCallBack,
                        WeakClearCallback nativeFinalizeCallBack) {
            return globalStorage_->SetWeak(nodeAddr, ref, freeGlobalCallBack, nativeFinalizeCallBack);
        };
        clearWeak_ = [this](uintptr_t nodeAddr) { return globalStorage_->ClearWeak(nodeAddr); };
        isWeak_ = [this](uintptr_t addr) { return globalStorage_->IsWeak(addr); };
    } else {
        globalDebugStorage_ = chunk->New<EcmaGlobalStorage<DebugNode>>(this, vm->GetNativeAreaAllocator());
        newGlobalHandle_ = [this](JSTaggedType value) {
            return globalDebugStorage_->NewGlobalHandle<NodeKind::NORMAL_NODE>(value);
        };
        disposeGlobalHandle_ = [this](uintptr_t nodeAddr) {
            globalDebugStorage_->DisposeGlobalHandle<NodeKind::NORMAL_NODE>(nodeAddr);
        };
        setWeak_ = [this](uintptr_t nodeAddr, void *ref, WeakClearCallback freeGlobalCallBack,
                        WeakClearCallback nativeFinalizeCallBack) {
            return globalDebugStorage_->SetWeak(nodeAddr, ref, freeGlobalCallBack, nativeFinalizeCallBack);
        };
        clearWeak_ = [this](uintptr_t nodeAddr) { return globalDebugStorage_->ClearWeak(nodeAddr); };
        isWeak_ = [this](uintptr_t addr) { return globalDebugStorage_->IsWeak(addr); };
        if (Runtime::GetInstance()->IsHybridVm()) {
            newXRefGlobalHandle_ = [this](JSTaggedType value) {
                return globalDebugStorage_->NewGlobalHandle<NodeKind::UNIFIED_NODE>(value);
            };
            disposeXRefGlobalHandle_ = [this](uintptr_t nodeAddr) {
                globalDebugStorage_->DisposeGlobalHandle<NodeKind::UNIFIED_NODE>(nodeAddr);
            };
            setNodeKind_ = [this](NodeKind nodeKind) { globalDebugStorage_->SetNodeKind(nodeKind); };
        }
    }
    vmThreadControl_ = new VmThreadControl(this);
    SetBCStubStatus(BCStubStatus::NORMAL_BC_STUB);
    dateUtils_ = new DateUtils();

    glueData_.propertiesCache_ = new PropertiesCache();
    if (vm_->GetJSOptions().IsEnableMegaIC()) {
        glueData_.loadMegaICCache_ = new MegaICCache();
        glueData_.storeMegaICCache_ = new MegaICCache();
    }

    glueData_.globalConst_ = new GlobalEnvConstants();
    glueData_.baseAddress_ = TaggedStateWord::BASE_ADDRESS;
    glueData_.isEnableCMCGC_ = g_isEnableCMCGC;
}

JSThread::JSThread(EcmaVM *vm, ThreadType threadType) : id_(os::thread::GetCurrentThreadId()),
                                                        vm_(vm), threadType_(threadType)
{
    ASSERT(threadType == ThreadType::JIT_THREAD);
    // jit thread no need GCIterating
    readyForGCIterating_ = false;
    glueData_.isEnableCMCGC_ = g_isEnableCMCGC;
    if (UNLIKELY(g_isEnableCMCGC)) {
        glueData_.threadHolder_ = ToUintPtr(ThreadHolder::CreateAndRegisterNewThreadHolder(nullptr));
    }
    RegisterThread(this);
};

JSThread::JSThread(ThreadType threadType) : threadType_(threadType)
{
    ASSERT(threadType == ThreadType::DAEMON_THREAD);
    // daemon thread no need GCIterating
    readyForGCIterating_ = false;
    glueData_.isEnableCMCGC_ = g_isEnableCMCGC;
}

JSThread::~JSThread()
{
    readyForGCIterating_ = false;
    if (globalStorage_ != nullptr) {
        vm_->GetChunk()->Delete(globalStorage_);
        globalStorage_ = nullptr;
    }
    if (globalDebugStorage_ != nullptr) {
        vm_->GetChunk()->Delete(globalDebugStorage_);
        globalDebugStorage_ = nullptr;
    }

    if (glueData_.loadMegaICCache_ != nullptr) {
        delete glueData_.loadMegaICCache_;
        glueData_.loadMegaICCache_ = nullptr;
    }
    if (glueData_.storeMegaICCache_ != nullptr) {
        delete glueData_.storeMegaICCache_;
        glueData_.storeMegaICCache_ = nullptr;
    }
    if (glueData_.propertiesCache_ != nullptr) {
        delete glueData_.propertiesCache_;
        glueData_.propertiesCache_ = nullptr;
    }

    if (threadType_ == ThreadType::JS_THREAD) {
        GetNativeAreaAllocator()->Free(glueData_.frameBase_, sizeof(JSTaggedType) *
                                       vm_->GetEcmaParamConfiguration().GetMaxStackSize());
    }
    GetNativeAreaAllocator()->FreeArea(regExpCacheArea_);

    glueData_.frameBase_ = nullptr;
    nativeAreaAllocator_ = nullptr;
    heapRegionAllocator_ = nullptr;
    regExpCacheArea_ = nullptr;
    if (vmThreadControl_ != nullptr) {
        delete vmThreadControl_;
        vmThreadControl_ = nullptr;
    }
    // DaemonThread will be unregistered when the binding std::thread release.
    if (!IsDaemonThread()) {
        UnregisterThread(this);
    }
    if (dateUtils_ != nullptr) {
        delete dateUtils_;
        dateUtils_ = nullptr;
    }
    if (glueData_.moduleLogger_ != nullptr) {
        delete glueData_.moduleLogger_;
        glueData_.moduleLogger_ = nullptr;
    }
    if (glueData_.globalConst_ != nullptr) {
        delete glueData_.globalConst_;
        glueData_.globalConst_ = nullptr;
    }
}

ThreadId JSThread::GetCurrentThreadId()
{
    return GetCurrentThreadOrTaskId();
}

void JSThread::SetException(JSTaggedValue exception)
{
    glueData_.exception_ = exception;
#if defined(ENABLE_EXCEPTION_BACKTRACE)
    if (vm_->GetJSOptions().EnableExceptionBacktrace()) {
        LOG_ECMA(INFO) << "SetException:" << exception.GetRawData();
        std::ostringstream stack;
        Backtrace(stack);
        LOG_ECMA(INFO) << stack.str();
    }
#endif
}

void JSThread::HandleUncaughtException(JSTaggedValue exception)
{
    [[maybe_unused]] EcmaHandleScope handleScope(this);
    JSHandle<JSTaggedValue> exceptionHandle(this, exception);
    if (isUncaughtExceptionRegistered_) {
        if (vm_->GetJSThread()->IsMainThread()) {
            return;
        }
        auto callback = GetOnErrorCallback();
        if (callback) {
            ClearException();
            Local<ObjectRef> exceptionRef = JSNApiHelper::ToLocal<ObjectRef>(exceptionHandle);
            ThreadNativeScope nativeScope(this);
            callback(exceptionRef, GetOnErrorData());
        }
    }
    // if caught exceptionHandle type is JSError
    ClearException();
    if (exceptionHandle->IsJSError()) {
        base::ErrorHelper::PrintJSErrorInfo(this, exceptionHandle);
        return;
    }
    JSHandle<EcmaString> result = JSTaggedValue::ToString(this, exceptionHandle);
    LOG_NO_TAG(ERROR) << ConvertToString(this, *result);
}

void JSThread::HandleUncaughtException()
{
    if (!HasPendingException()) {
        return;
    }
    JSTaggedValue exception = GetException();
    HandleUncaughtException(exception);
}

JSTaggedValue JSThread::GetCurrentLexenv() const
{
    FrameHandler frameHandler(const_cast<JSThread*>(this));
    return frameHandler.GetEnv();
}

JSTaggedValue JSThread::GetCurrentFunction() const
{
    FrameHandler frameHandler(const_cast<JSThread*>(this));
    return frameHandler.GetFunction();
}

const JSTaggedType *JSThread::GetCurrentFrame() const
{
    if (IsAsmInterpreter()) {
        return GetLastLeaveFrame();
    }
    return GetCurrentSPFrame();
}

void JSThread::SetCurrentFrame(JSTaggedType *sp)
{
    if (IsAsmInterpreter()) {
        return SetLastLeaveFrame(sp);
    }
    return SetCurrentSPFrame(sp);
}

const JSTaggedType *JSThread::GetCurrentInterpretedFrame() const
{
    if (IsAsmInterpreter()) {
        auto frameHandler = FrameHandler(const_cast<JSThread*>(this));
        return frameHandler.GetSp();
    }
    return GetCurrentSPFrame();
}

void JSThread::InvokeWeakNodeFreeGlobalCallBack()
{
    while (!weakNodeFreeGlobalCallbacks_.empty()) {
        auto callbackPair = weakNodeFreeGlobalCallbacks_.back();
        weakNodeFreeGlobalCallbacks_.pop_back();
        ASSERT(callbackPair.first != nullptr && callbackPair.second != nullptr);
        auto callback = callbackPair.first;
        (*callback)(callbackPair.second);
    }
}

void JSThread::InvokeWeakNodeNativeFinalizeCallback()
{
    if (ShouldIgnoreFinalizeCallback()) {
        weakNodeNativeFinalizeCallbacks_.clear();
        return;
    }
    // the second callback may lead to another GC, if this, return directly;
    if (runningNativeFinalizeCallbacks_) {
        return;
    }
    runningNativeFinalizeCallbacks_ = true;
    TRACE_GC(GCStats::Scope::ScopeId::InvokeNativeFinalizeCallbacks, vm_->GetEcmaGCStats());
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, ("InvokeNativeFinalizeCallbacks num:"
        + std::to_string(weakNodeNativeFinalizeCallbacks_.size())).c_str(), "");
    while (!weakNodeNativeFinalizeCallbacks_.empty()) {
        auto callbackPair = weakNodeNativeFinalizeCallbacks_.back();
        weakNodeNativeFinalizeCallbacks_.pop_back();
        ASSERT(callbackPair.first != nullptr && callbackPair.second != nullptr);
        auto callback = callbackPair.first;
        (*callback)(callbackPair.second);
    }
    if (finalizeTaskCallback_ != nullptr) {
        finalizeTaskCallback_();
    }
    runningNativeFinalizeCallbacks_ = false;
}

bool JSThread::IsStartGlobalLeakCheck() const
{
    return vm_->GetJSOptions().IsStartGlobalLeakCheck();
}

bool JSThread::EnableGlobalObjectLeakCheck() const
{
    return vm_->GetJSOptions().EnableGlobalObjectLeakCheck();
}

bool JSThread::EnableGlobalPrimitiveLeakCheck() const
{
    return vm_->GetJSOptions().EnableGlobalPrimitiveLeakCheck();
}

bool JSThread::IsInRunningStateOrProfiling() const
{
    bool result = IsInRunningState();
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    result |= vm_->GetHeapProfile() != nullptr;
#endif
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    result |= GetIsProfiling();
#endif
    return result;
}

void JSThread::WriteToStackTraceFd(std::ostringstream &buffer) const
{
    if (stackTraceFd_ < 0) {
        return;
    }
    buffer << std::endl;
    DPrintf(reinterpret_cast<fd_t>(stackTraceFd_), buffer.str());
    buffer.str("");
}

void JSThread::SetStackTraceFd(int32_t fd)
{
    FdsanExchangeOwnerTag(reinterpret_cast<fd_t>(fd));
    stackTraceFd_ = fd;
}

void JSThread::CloseStackTraceFd()
{
    if (stackTraceFd_ != -1) {
        FSync(reinterpret_cast<fd_t>(stackTraceFd_));
        Close(reinterpret_cast<fd_t>(stackTraceFd_));
        stackTraceFd_ = -1;
    }
}

void JSThread::SetJitCodeMap(JSTaggedType exception,  MachineCode* machineCode, std::string &methodName,
    uintptr_t offset)
{
    auto it = jitCodeMaps_.find(exception);
    if (it != jitCodeMaps_.end()) {
        it->second->push_back(std::make_tuple(machineCode, methodName, offset));
    } else {
        JitCodeVector *jitCode = new JitCodeVector {std::make_tuple(machineCode, methodName, offset)};
        jitCodeMaps_.emplace(exception, jitCode);
    }
}

void JSThread::IterateMegaIC(RootVisitor &v)
{
    if (glueData_.loadMegaICCache_ != nullptr) {
        glueData_.loadMegaICCache_->Iterate(v);
    }
    if (glueData_.storeMegaICCache_ != nullptr) {
        glueData_.storeMegaICCache_->Iterate(v);
    }
}

void JSThread::Iterate(RootVisitor &visitor)
{
    if (!glueData_.exception_.IsHole()) {
        visitor.VisitRoot(Root::ROOT_VM, ObjectSlot(ToUintPtr(&glueData_.exception_)));
    }
    if (!glueData_.currentEnv_.IsHole()) {
        visitor.VisitRoot(Root::ROOT_VM, ObjectSlot(ToUintPtr(&glueData_.currentEnv_)));
    }
    if (!hotReloadDependInfo_.IsUndefined()) {
        visitor.VisitRoot(Root::ROOT_VM, ObjectSlot(ToUintPtr(&hotReloadDependInfo_)));
    }
    visitor.VisitRangeRoot(Root::ROOT_VM,
        ObjectSlot(glueData_.builtinEntries_.Begin()), ObjectSlot(glueData_.builtinEntries_.End()));

    // visit stack roots
    FrameHandler frameHandler(this);
    frameHandler.Iterate(visitor);
    // visit tagged handle storage roots
    if (vm_->GetJSOptions().EnableGlobalLeakCheck()) {
        IterateHandleWithCheck(visitor);
    } else {
        OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::VisitRootGlobalRefHandle", "");
        size_t globalCount = 0;
        auto callback = [&visitor, &globalCount](Node *node) {
            JSTaggedValue value(node->GetObject());
            if (value.IsHeapObject()) {
                visitor.VisitRoot(Root::ROOT_HANDLE, ecmascript::ObjectSlot(node->GetObjectAddress()));
            }
            globalCount++;
        };
        globalStorage_->IterateUsageGlobal(callback);
        static bool hasCheckedGlobalCount = false;
        static const size_t WARN_GLOBAL_COUNT = 100000;
        if (!hasCheckedGlobalCount && globalCount >= WARN_GLOBAL_COUNT) {
            LOG_ECMA(WARN) << "Global reference count is " << globalCount << ",It exceed the upper limit 100000!";
            hasCheckedGlobalCount = true;
        }
    }

    IterateMegaIC(visitor);

    if (glueData_.propertiesCache_ != nullptr) {
        glueData_.propertiesCache_->Clear();
    }

    if (glueData_.globalConst_ != nullptr) {
        glueData_.globalConst_->Iterate(visitor);
    }
}

void JSThread::ClearCache()
{
    JSHandle<GlobalEnv> env = GetGlobalEnv();
    if (!env.GetTaggedValue().IsHole()) {
        env->ClearCache(this);
    }
}

void JSThread::IterateJitCodeMap(const JitCodeMapVisitor &jitCodeMapVisitor)
{
    jitCodeMapVisitor(jitCodeMaps_);
}

void JSThread::IterateHandleWithCheck(RootVisitor &visitor)
{
    size_t handleCount = vm_->IterateHandle(visitor);

    size_t globalCount = 0;
    static const int JS_TYPE_SUM = static_cast<int>(JSType::TYPE_LAST) + 1;
    int typeCount[JS_TYPE_SUM] = { 0 };
    int primitiveCount = 0;
    bool isStopObjectLeakCheck = EnableGlobalObjectLeakCheck() && !IsStartGlobalLeakCheck() && stackTraceFd_ > 0;
    bool isStopPrimitiveLeakCheck = EnableGlobalPrimitiveLeakCheck() && !IsStartGlobalLeakCheck() && stackTraceFd_ > 0;
    std::ostringstream buffer;
    auto callback = [this, &visitor, &globalCount, &typeCount, &primitiveCount,
        isStopObjectLeakCheck, isStopPrimitiveLeakCheck, &buffer](DebugNode *node) {
        node->MarkCount();
        JSTaggedValue value(node->GetObject());
        if (value.IsHeapObject()) {
            visitor.VisitRoot(Root::ROOT_HANDLE, ecmascript::ObjectSlot(node->GetObjectAddress()));
            auto object = reinterpret_cast<TaggedObject *>(node->GetObject());
            typeCount[static_cast<int>(object->GetClass()->GetObjectType())]++;

            // Print global information about possible memory leaks.
            // You can print the global new stack within the range of the leaked global number.
            if (isStopObjectLeakCheck && node->GetGlobalNumber() > 0 && node->GetMarkCount() > 0) {
                buffer << "Global maybe leak object address:" << std::hex << object <<
                    ", type:" << JSHClass::DumpJSType(JSType(object->GetClass()->GetObjectType())) <<
                    ", node address:" << node << ", number:" << std::dec <<  node->GetGlobalNumber() <<
                    ", markCount:" << node->GetMarkCount();
                WriteToStackTraceFd(buffer);
            }
        } else {
            primitiveCount++;
            if (isStopPrimitiveLeakCheck && node->GetGlobalNumber() > 0 && node->GetMarkCount() > 0) {
                buffer << "Global maybe leak primitive:" << std::hex << value.GetRawData() <<
                    ", node address:" << node << ", number:" << std::dec <<  node->GetGlobalNumber() <<
                    ", markCount:" << node->GetMarkCount();
                WriteToStackTraceFd(buffer);
            }
        }
        globalCount++;
    };
    globalDebugStorage_->IterateUsageGlobal(callback);
    if (isStopObjectLeakCheck || isStopPrimitiveLeakCheck) {
        buffer << "Global leak check success!";
        WriteToStackTraceFd(buffer);
        CloseStackTraceFd();
    }
    // Determine whether memory leakage by checking handle and global count.
    LOG_ECMA(INFO) << "Iterate root handle count:" << handleCount << ", global handle count:" << globalCount;
    OPTIONAL_LOG(vm_, INFO) << "Global type Primitive count:" << primitiveCount;
    // Print global object type statistic.
    static const int MIN_COUNT_THRESHOLD = 50;
    for (int i = 0; i < JS_TYPE_SUM; i++) {
        if (typeCount[i] > MIN_COUNT_THRESHOLD) {
            OPTIONAL_LOG(vm_, INFO) << "Global type " << JSHClass::DumpJSType(JSType(i))
                                            << " count:" << typeCount[i];
        }
    }
}

void JSThread::IterateWeakEcmaGlobalStorage(WeakVisitor &visitor)
{
    auto callBack = [this, &visitor](WeakNode *node) {
        JSTaggedValue value(node->GetObject());
        if (!value.IsHeapObject()) {
            return;
        };
        bool isAlive = visitor.VisitRoot(Root::ROOT_VM, ecmascript::ObjectSlot(node->GetObjectAddress()));
        if (!isAlive) {
            node->SetObject(JSTaggedValue::Undefined().GetRawData());
            auto nativeFinalizeCallback = node->GetNativeFinalizeCallback();
            if (nativeFinalizeCallback) {
                weakNodeNativeFinalizeCallbacks_.push_back(std::make_pair(nativeFinalizeCallback,
                                                                          node->GetReference()));
            }
            auto freeGlobalCallBack = node->GetFreeGlobalCallback();
            if (!freeGlobalCallBack) {
                // If no callback, dispose global immediately
                DisposeGlobalHandle(ToUintPtr(node));
            } else {
                weakNodeFreeGlobalCallbacks_.push_back(std::make_pair(freeGlobalCallBack, node->GetReference()));
            }
        }
    };

    if (!vm_->GetJSOptions().EnableGlobalLeakCheck()) {
        globalStorage_->IterateWeakUsageGlobal(callBack);
    } else {
        globalDebugStorage_->IterateWeakUsageGlobal(callBack);
    }
}

void JSThread::IterateWeakEcmaGlobalStorage(const WeakRootVisitor &visitor, GCKind gcKind)
{
    auto callBack = [this, visitor, gcKind](WeakNode *node) {
        JSTaggedValue value(node->GetObject());
        if (!value.IsHeapObject()) {
            return;
        }
        auto object = value.GetTaggedObject();
        auto fwd = visitor(object);
        if (fwd == nullptr) {
            // undefind
            node->SetObject(JSTaggedValue::Undefined().GetRawData());
            auto nativeFinalizeCallback = node->GetNativeFinalizeCallback();
            if (nativeFinalizeCallback) {
                weakNodeNativeFinalizeCallbacks_.push_back(std::make_pair(nativeFinalizeCallback,
                                                                          node->GetReference()));
            }
            auto freeGlobalCallBack = node->GetFreeGlobalCallback();
            if (!freeGlobalCallBack) {
                // If no callback, dispose global immediately
                DisposeGlobalHandle(ToUintPtr(node));
            } else if (gcKind == GCKind::SHARED_GC) {
                // For shared GC, free global should defer execute in its own thread
                weakNodeFreeGlobalCallbacks_.push_back(std::make_pair(freeGlobalCallBack, node->GetReference()));
            } else {
                node->CallFreeGlobalCallback();
            }
        } else if (fwd != object) {
            // update
            node->SetObject(JSTaggedValue(fwd).GetRawData());
        }
    };
    if (!vm_->GetJSOptions().EnableGlobalLeakCheck()) {
        globalStorage_->IterateWeakUsageGlobal(callBack);
    } else {
        globalDebugStorage_->IterateWeakUsageGlobal(callBack);
    }
}

void JSThread::UpdateJitCodeMapReference(const WeakRootVisitor &visitor)
{
    auto it = jitCodeMaps_.begin();
    while (it != jitCodeMaps_.end()) {
        auto obj = reinterpret_cast<TaggedObject *>(it->first);
        auto fwd = visitor(obj);
        if (fwd == nullptr) {
            delete it->second;
            it = jitCodeMaps_.erase(it);
        } else if (fwd != obj) {
            jitCodeMaps_.emplace(JSTaggedValue(fwd).GetRawData(), it->second);
            it = jitCodeMaps_.erase(it);
        } else {
            ++it;
        }
    }
}

bool JSThread::DoStackOverflowCheck(const JSTaggedType *sp)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (UNLIKELY(!IsCrossThreadExecutionEnable() && sp <= glueData_.frameBase_ + RESERVE_STACK_SIZE)) {
        vm_->CheckThread();
        LOG_ECMA(ERROR) << "Stack overflow! Remaining stack size is: " << (sp - glueData_.frameBase_);
        if (LIKELY(!HasPendingException())) {
            ObjectFactory *factory = vm_->GetFactory();
            JSHandle<JSObject> error = factory->GetJSError(base::ErrorType::RANGE_ERROR,
                                                           "Stack overflow!", StackCheck::NO);
            SetException(error.GetTaggedValue());
        }
        return true;
    }
    return false;
}

bool JSThread::DoStackLimitCheck()
{
    if (UNLIKELY(!IsCrossThreadExecutionEnable() && GetCurrentStackPosition() < GetStackLimit())) {
        vm_->CheckThread();
        LOG_ECMA(ERROR) << "Stack overflow! current:" << GetCurrentStackPosition() << " limit:" << GetStackLimit();
        if (LIKELY(!HasPendingException())) {
            ObjectFactory *factory = vm_->GetFactory();
            JSHandle<JSObject> error = factory->GetJSError(base::ErrorType::RANGE_ERROR,
                                                           "Stack overflow!", StackCheck::NO);
            SetException(error.GetTaggedValue());
        }
        return true;
    }
    return false;
}

void JSThread::SetInitialBuiltinHClass(
    BuiltinTypeId type, JSHClass *builtinHClass, JSHClass *instanceHClass,
    JSHClass *prototypeHClass, JSHClass *prototypeOfPrototypeHClass, JSHClass *extraHClass)
{
    size_t index = BuiltinHClassEntries::GetEntryIndex(type);
    auto &entry = glueData_.builtinHClassEntries_.entries[index];
    LOG_ECMA(DEBUG) << "JSThread::SetInitialBuiltinHClass: "
                    << "Builtin = " << ToString(type)
                    << ", builtinHClass = " << builtinHClass
                    << ", instanceHClass = " << instanceHClass
                    << ", prototypeHClass = " << prototypeHClass
                    << ", prototypeOfPrototypeHClass = " << prototypeOfPrototypeHClass
                    << ", extraHClass = " << extraHClass;
    entry.builtinHClass = builtinHClass;
    entry.instanceHClass = instanceHClass;
    entry.prototypeHClass = prototypeHClass;
    entry.prototypeOfPrototypeHClass = prototypeOfPrototypeHClass;
    entry.extraHClass = extraHClass;
}

void JSThread::SetInitialBuiltinGlobalHClass(
    JSHClass *builtinHClass, GlobalIndex globalIndex)
{
    auto &map = ctorHclassEntries_;
    map[builtinHClass] = globalIndex;
}

JSHClass *JSThread::GetBuiltinHClass(BuiltinTypeId type) const
{
    size_t index = BuiltinHClassEntries::GetEntryIndex(type);
    return glueData_.builtinHClassEntries_.entries[index].builtinHClass;
}

JSHClass *JSThread::GetBuiltinInstanceHClass(BuiltinTypeId type) const
{
    size_t index = BuiltinHClassEntries::GetEntryIndex(type);
    return glueData_.builtinHClassEntries_.entries[index].instanceHClass;
}

JSHClass *JSThread::GetBuiltinExtraHClass(BuiltinTypeId type) const
{
    size_t index = BuiltinHClassEntries::GetEntryIndex(type);
    return glueData_.builtinHClassEntries_.entries[index].extraHClass;
}

JSHClass *JSThread::GetBuiltinPrototypeHClass(BuiltinTypeId type) const
{
    size_t index = BuiltinHClassEntries::GetEntryIndex(type);
    return glueData_.builtinHClassEntries_.entries[index].prototypeHClass;
}

JSHClass *JSThread::GetBuiltinPrototypeOfPrototypeHClass(BuiltinTypeId type) const
{
    size_t index = BuiltinHClassEntries::GetEntryIndex(type);
    return glueData_.builtinHClassEntries_.entries[index].prototypeOfPrototypeHClass;
}

void JSThread::CheckSwitchDebuggerBCStub()
{
    auto isDebug = vm_->GetJsDebuggerManager()->IsDebugMode();
    if (LIKELY(!isDebug)) {
        if (glueData_.bcStubEntries_.Get(0) == glueData_.bcStubEntries_.Get(1)) {
            for (size_t i = 0; i < BCStubEntries::BC_HANDLER_COUNT; i++) {
                auto stubEntry = glueData_.bcDebuggerStubEntries_.Get(i);
                auto debuggerStubEbtry = glueData_.bcStubEntries_.Get(i);
                glueData_.bcStubEntries_.Set(i, stubEntry);
                glueData_.bcDebuggerStubEntries_.Set(i, debuggerStubEbtry);
            }
        }
    } else {
        if (glueData_.bcDebuggerStubEntries_.Get(0) == glueData_.bcDebuggerStubEntries_.Get(1)) {
            for (size_t i = 0; i < BCStubEntries::BC_HANDLER_COUNT; i++) {
                auto stubEntry = glueData_.bcStubEntries_.Get(i);
                auto debuggerStubEbtry = glueData_.bcDebuggerStubEntries_.Get(i);
                glueData_.bcDebuggerStubEntries_.Set(i, stubEntry);
                glueData_.bcStubEntries_.Set(i, debuggerStubEbtry);
            }
        }
    }
}

void JSThread::CheckOrSwitchPGOStubs()
{
    bool isSwitch = false;
    bool isSwitchToNormal = false;
    if (IsPGOProfilerEnable()) {
        if (GetBCStubStatus() == BCStubStatus::NORMAL_BC_STUB) {
            SetBCStubStatus(BCStubStatus::PROFILE_BC_STUB);
            isSwitch = true;
        } else if (GetBCStubStatus() == BCStubStatus::STW_COPY_BC_STUB) {
            SwitchStwCopyBCStubs(false);
            ASSERT(GetBCStubStatus() == BCStubStatus::NORMAL_BC_STUB);
            SetBCStubStatus(BCStubStatus::PROFILE_BC_STUB);
            isSwitch = true;
        }
    } else {
        if (GetBCStubStatus() == BCStubStatus::PROFILE_BC_STUB) {
            SetBCStubStatus(BCStubStatus::NORMAL_BC_STUB);
            isSwitch = true;
            isSwitchToNormal = true;
        }
    }
    if (isSwitch) {
        Address curAddress;
#define SWITCH_PGO_STUB_ENTRY(fromName, toName, ...)                                                        \
        curAddress = GetBCStubEntry(BytecodeStubCSigns::ID_##fromName);                                     \
        SetBCStubEntry(BytecodeStubCSigns::ID_##fromName, GetBCStubEntry(BytecodeStubCSigns::ID_##toName)); \
        SetBCStubEntry(BytecodeStubCSigns::ID_##toName, curAddress);
        ASM_INTERPRETER_BC_PROFILER_STUB_LIST(SWITCH_PGO_STUB_ENTRY)
#undef SWITCH_PGO_STUB_ENTRY
    }
    if (isSwitchToNormal && !g_isEnableCMCGC) {
        SwitchStwCopyBCStubs(true);
    }
}

void JSThread::SwitchJitProfileStubs(bool isEnablePgo)
{
    if (isEnablePgo) {
        SetPGOProfilerEnable(true);
        CheckOrSwitchPGOStubs();
        return;
    }
    bool isSwitch = false;
    if (GetBCStubStatus() == BCStubStatus::NORMAL_BC_STUB) {
        SetBCStubStatus(BCStubStatus::JIT_PROFILE_BC_STUB);
        isSwitch = true;
    } else if (GetBCStubStatus() == BCStubStatus::STW_COPY_BC_STUB) {
        SwitchStwCopyBCStubs(false);
        ASSERT(GetBCStubStatus() == BCStubStatus::NORMAL_BC_STUB);
        SetBCStubStatus(BCStubStatus::JIT_PROFILE_BC_STUB);
        isSwitch = true;
    }
    if (isSwitch) {
        Address curAddress;
#define SWITCH_PGO_STUB_ENTRY(fromName, toName, ...)                                                        \
        curAddress = GetBCStubEntry(BytecodeStubCSigns::ID_##fromName);                                     \
        SetBCStubEntry(BytecodeStubCSigns::ID_##fromName, GetBCStubEntry(BytecodeStubCSigns::ID_##toName)); \
        SetBCStubEntry(BytecodeStubCSigns::ID_##toName, curAddress);
        ASM_INTERPRETER_BC_JIT_PROFILER_STUB_LIST(SWITCH_PGO_STUB_ENTRY)
#undef SWITCH_PGO_STUB_ENTRY
    }
}

void JSThread::SwitchStwCopyBCStubs(bool isStwCopy)
{
    bool isSwitch = false;
    if (isStwCopy && GetBCStubStatus() == BCStubStatus::NORMAL_BC_STUB) {
        SetBCStubStatus(BCStubStatus::STW_COPY_BC_STUB);
        isSwitch = true;
    } else if (!isStwCopy && GetBCStubStatus() == BCStubStatus::STW_COPY_BC_STUB) {
        SetBCStubStatus(BCStubStatus::NORMAL_BC_STUB);
        isSwitch = true;
    }
    if (isSwitch) {
        Address curAddress;
#define SWITCH_STW_COPY_STUB_ENTRY(base)                                                                    \
        curAddress = GetBCStubEntry(BytecodeStubCSigns::ID_##base);                                         \
        SetBCStubEntry(BytecodeStubCSigns::ID_##base,                                                       \
                       GetBCStubEntry(BytecodeStubCSigns::ID_##base##StwCopy));                             \
        SetBCStubEntry(BytecodeStubCSigns::ID_##base##StwCopy, curAddress);
        ASM_INTERPRETER_BC_STW_COPY_STUB_LIST(SWITCH_STW_COPY_STUB_ENTRY)
#undef SWITCH_STW_COPY_STUB_ENTRY
    }
}

void JSThread::SwitchStwCopyCommonStubs(bool isStwCopy)
{
    bool isSwitch = false;
    if (isStwCopy && GetCommonStubStatus() == CommonStubStatus::NORMAL_COMMON_STUB) {
        SetCommonStubStatus(CommonStubStatus::STW_COPY_COMMON_STUB);
        isSwitch = true;
    } else if (!isStwCopy && GetCommonStubStatus() == CommonStubStatus::STW_COPY_COMMON_STUB) {
        SetCommonStubStatus(CommonStubStatus::NORMAL_COMMON_STUB);
        isSwitch = true;
    }
    if (isSwitch) {
        Address curAddress;
#define SWITCH_STW_COPY_STUB_ENTRY(base)                                                                    \
        curAddress = GetFastStubEntry(CommonStubCSigns::base);                                              \
        SetFastStubEntry(CommonStubCSigns::base,                                                            \
                         GetFastStubEntry(CommonStubCSigns::base##StwCopy));                                \
        SetFastStubEntry(CommonStubCSigns::base##StwCopy, curAddress);
        COMMON_STW_COPY_STUB_LIST(SWITCH_STW_COPY_STUB_ENTRY)
#undef SWITCH_STW_COPY_STUB_ENTRY
    }
}

void JSThread::SwitchStwCopyBuiltinsStubs(bool isStwCopy)
{
    bool isSwitch = false;
    if (isStwCopy && GetBuiltinsStubStatus() == BuiltinsStubStatus::NORMAL_BUILTINS_STUB) {
        SetBuiltinsStubStatus(BuiltinsStubStatus::STW_COPY_BUILTINS_STUB);
        isSwitch = true;
    } else if (!isStwCopy && GetBuiltinsStubStatus() == BuiltinsStubStatus::STW_COPY_BUILTINS_STUB) {
        SetBuiltinsStubStatus(BuiltinsStubStatus::NORMAL_BUILTINS_STUB);
        isSwitch = true;
    }
    if (isSwitch) {
        Address curAddress;
#define SWITCH_STW_COPY_STUB_ENTRY(base)                                                                    \
        curAddress = GetBuiltinStubEntry(BuiltinsStubCSigns::SubID::base);                                  \
        SetBuiltinStubEntry(BuiltinsStubCSigns::SubID::base,                                                \
                            GetBuiltinStubEntry(BuiltinsStubCSigns::SubID::base##StwCopy));                 \
        SetBuiltinStubEntry(BuiltinsStubCSigns::SubID::base##StwCopy, curAddress);

#define SWITCH_STW_COPY_STUB_ENTRY_DYN(base, type, ...)                                                     \
        SWITCH_STW_COPY_STUB_ENTRY(type##base)

        BUILTINS_STW_COPY_STUB_LIST(SWITCH_STW_COPY_STUB_ENTRY, SWITCH_STW_COPY_STUB_ENTRY_DYN, \
            SWITCH_STW_COPY_STUB_ENTRY)
#undef SWITCH_STW_COPY_STUB_ENTRY_DYN
#undef SWITCH_STW_COPY_STUB_ENTRY
    }
}

void JSThread::TerminateExecution()
{
    // set the TERMINATE_ERROR to exception
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::TERMINATION_ERROR,
        "Terminate execution!", StackCheck::NO);
    SetException(error.GetTaggedValue());
}

void JSThread::CheckAndPassActiveBarrier()
{
    ASSERT(!g_isEnableCMCGC);
    ThreadStateAndFlags oldStateAndFlags;
    oldStateAndFlags.asNonvolatileInt = glueData_.stateAndFlags_.asInt;
    if ((oldStateAndFlags.asNonvolatileStruct.flags & ThreadFlag::ACTIVE_BARRIER) != 0) {
        PassSuspendBarrier();
    }
}

bool JSThread::PassSuspendBarrier()
{
    // Use suspendLock_ to avoid data-race between suspend-all-thread and suspended-threads.
    LockHolder lock(suspendLock_);
    if (suspendBarrier_ != nullptr) {
        suspendBarrier_->PassStrongly();
        suspendBarrier_ = nullptr;
        ClearFlag(ThreadFlag::ACTIVE_BARRIER);
        return true;
    }
    return false;
}

bool JSThread::ShouldHandleMarkingFinishedInSafepoint()
{
    auto heap = const_cast<Heap *>(vm_->GetHeap());
    return IsMarkFinished() && heap->GetConcurrentMarker()->IsTriggeredConcurrentMark() &&
           !heap->GetOnSerializeEvent() && !heap->InSensitiveStatus() && !heap->CheckIfNeedStopCollectionByStartup();
}

bool JSThread::CheckSafepoint()
{
    ResetCheckSafePointStatus();

    if UNLIKELY(HasTerminationRequest()) {
        TerminateExecution();
        SetVMTerminated(true);
        SetTerminationRequest(false);
    }

    if UNLIKELY(HasSuspendRequest()) {
        WaitSuspension();
    }

    // vmThreadControl_ 's thread_ is current JSThread's this.
    if UNLIKELY(VMNeedSuspension()) {
        vmThreadControl_->SuspendVM();
    }
    if (HasInstallMachineCode()) {
        Jit::JitGCLockHolder lock(this);
        vm_->GetJit()->InstallTasks(this);
        SetInstallMachineCode(false);
    }

#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    if UNLIKELY(needProfiling_.load() && !isProfiling_) {
        DFXJSNApi::StartCpuProfilerForFile(vm_, profileName_, CpuProfiler::INTERVAL_OF_INNER_START);
        SetNeedProfiling(false);
    }
#endif // ECMASCRIPT_SUPPORT_CPUPROFILER
    bool gcTriggered = false;
#ifndef NDEBUG
    if (vm_->GetJSOptions().EnableForceGC()) {
        if (g_isEnableCMCGC) {
            common::BaseRuntime::RequestGC(common::GC_REASON_USER, false,
                                           common::GC_TYPE_FULL);  // Trigger Full CMC here
        } else {
            vm_->CollectGarbage(TriggerGCType::FULL_GC);
        }
        gcTriggered = true;
    }
#endif
    auto heap = const_cast<Heap *>(vm_->GetHeap());
    // Handle exit app senstive scene
    heap->HandleExitHighSensitiveEvent();

    // Do not trigger local gc during the shared gc processRset process.
    if UNLIKELY(IsProcessingLocalToSharedRset()) {
        return false;
    }
    // After concurrent mark finish, should trigger gc here to avoid create much floating garbage
    // except in serialize or high sensitive event
    if UNLIKELY(ShouldHandleMarkingFinishedInSafepoint()) {
        heap->SetCanThrowOOMError(false);
        heap->GetConcurrentMarker()->HandleMarkingFinished();
        heap->SetCanThrowOOMError(true);
        gcTriggered = true;
    }
    return gcTriggered;
}

void JSThread::CheckJSTaggedType(JSTaggedType value) const
{
    if (JSTaggedValue(value).IsHeapObject() &&
        !vm_->GetHeap()->IsAlive(reinterpret_cast<TaggedObject *>(value))) {
        LOG_FULL(FATAL) << "value:" << value << " is invalid!";
    }
}

bool JSThread::CpuProfilerCheckJSTaggedType(JSTaggedType value) const
{
    if (JSTaggedValue(value).IsHeapObject() &&
        !vm_->GetHeap()->IsAlive(reinterpret_cast<TaggedObject *>(value))) {
        return false;
    }
    return true;
}


uintptr_t JSThread::GetAndClearCallSiteReturnAddr(uintptr_t callSiteSp)
{
    auto iter = callSiteSpToReturnAddrTable_.find(callSiteSp);
    ASSERT(iter != callSiteSpToReturnAddrTable_.end());
    uintptr_t returnAddr = iter->second;
    callSiteSpToReturnAddrTable_.erase(iter);
    return returnAddr;
}

bool JSThread::IsLegalAsmSp(uintptr_t sp) const
{
    uint64_t bottom = GetStackLimit() - EcmaParamConfiguration::GetDefaultReservedStackSize();
    uint64_t top = GetStackStart() + EcmaParamConfiguration::GetAllowedUpperStackDiff();
    return (bottom <= sp && sp <= top);
}

bool JSThread::IsLegalThreadSp(uintptr_t sp) const
{
    uintptr_t bottom = reinterpret_cast<uintptr_t>(glueData_.frameBase_);
    size_t maxStackSize = vm_->GetEcmaParamConfiguration().GetMaxStackSize();
    uintptr_t top = bottom + maxStackSize;
    return (bottom <= sp && sp <= top);
}

bool JSThread::IsLegalSp(uintptr_t sp) const
{
    return IsLegalAsmSp(sp) || IsLegalThreadSp(sp);
}

bool JSThread::IsMainThread()
{
#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS) && !defined(PANDA_TARGET_IOS)
    return getpid() == syscall(SYS_gettid);
#else
    return true;
#endif
}

void JSThread::ClearVMCachedConstantPool()
{
    vm_->ClearCachedConstantPool();
}

JSHandle<GlobalEnv> JSThread::GetGlobalEnv() const
{
    // currentEnv is GlobalEnv now
    return JSHandle<GlobalEnv>(ToUintPtr(&glueData_.currentEnv_));
}

JSHandle<DependentInfos> JSThread::GetDependentInfo() const
{
    return JSHandle<DependentInfos>(ToUintPtr(&hotReloadDependInfo_));
}

void JSThread::SetDependentInfo(JSTaggedValue info)
{
    hotReloadDependInfo_ = info;
}

JSHandle<DependentInfos> JSThread::GetOrCreateThreadDependentInfo()
{
    if (hotReloadDependInfo_.IsUndefined()) {
        return GetEcmaVM()->GetFactory()->NewDependentInfos(0);
    }
    return GetDependentInfo();
}

void JSThread::NotifyHotReloadDeoptimize()
{
    if (!hotReloadDependInfo_.IsHeapObject()) {
        return;
    }
    DependentInfos::TriggerLazyDeoptimization(GetDependentInfo(),
        this, DependentInfos::DependentState::HOTRELOAD_PATCHMAIN);
    hotReloadDependInfo_ = JSTaggedValue::Undefined();
}

PropertiesCache *JSThread::GetPropertiesCache() const
{
    return glueData_.propertiesCache_;
}

MegaICCache *JSThread::GetLoadMegaICCache() const
{
    return glueData_.loadMegaICCache_;
}

MegaICCache *JSThread::GetStoreMegaICCache() const
{
    return glueData_.storeMegaICCache_;
}

bool JSThread::IsReadyToUpdateDetector() const
{
    return !GetEnableLazyBuiltins() && !InGlobalEnvInitialize();
}

Area *JSThread::GetOrCreateRegExpCacheArea()
{
    if (regExpCacheArea_ == nullptr) {
        regExpCacheArea_ = nativeAreaAllocator_->AllocateArea(MAX_REGEXP_CACHE_SIZE);
    }
    return regExpCacheArea_;
}

void JSThread::InitializeBuiltinObject(const JSHandle<GlobalEnv>& env, const std::string& key)
{
    BuiltinIndex& builtins = BuiltinIndex::GetInstance();
    auto index = builtins.GetBuiltinIndex(key);
    ASSERT(index != BuiltinIndex::NOT_FOUND);
    /*
        If using `auto globalObject = vm_->GetGlobalEnv()->GetGlobalObject()` here,
        it will cause incorrect result in multi-context environment. For example:

        ```ts
        let obj = {};
        print(obj instanceof Object); // instead of true, will print false
        ```
    */
    auto globalObject = env->GetGlobalObject();
    auto jsObject = JSHandle<JSObject>(this, globalObject);
    auto box = jsObject->GetGlobalPropertyBox(this, key);
    if (box == nullptr) {
        return;
    }
    auto& entry = glueData_.builtinEntries_.builtin_[index];
    entry.box_ = JSTaggedValue::Cast(box);
    auto builtin = JSHandle<JSObject>(this, box->GetValue(this));
    auto hclass = builtin->GetJSHClass();
    entry.hClass_ = JSTaggedValue::Cast(hclass);
}

void JSThread::InitializeBuiltinObject(const JSHandle<GlobalEnv>& env)
{
    BuiltinIndex& builtins = BuiltinIndex::GetInstance();
    for (auto key: builtins.GetBuiltinKeys()) {
        InitializeBuiltinObject(env, key);
    }
}

bool JSThread::IsPropertyCacheCleared() const
{
    if (!GetPropertiesCache()->IsCleared()) {
        return false;
    }
    return true;
}

ThreadState JSThread::UpdateState(ThreadState newState)
{
    ASSERT(!IsEnableCMCGC());
    ThreadState oldState = GetState();
    if (LIKELY(oldState != newState)) {
        if (oldState == ThreadState::RUNNING) {
            TransferFromRunningToSuspended(newState);
        } else if (newState == ThreadState::RUNNING) {
            TransferToRunning();
        } else {
            // Here can be some extra checks...
            StoreState(newState);
        }
    }
    return oldState;
}

void JSThread::SuspendThread(bool internalSuspend, SuspendBarrier* barrier)
{
    ASSERT(!g_isEnableCMCGC);
    LockHolder lock(suspendLock_);
    if (!internalSuspend) {
        // do smth here if we want to combine internal and external suspension
    }

    uint32_t old_count = suspendCount_++;
    if (old_count == 0) {
        SetFlag(ThreadFlag::SUSPEND_REQUEST);
        SetCheckSafePointStatus();
    }

    if (barrier != nullptr) {
        ASSERT(suspendBarrier_ == nullptr);
        suspendBarrier_ = barrier;
        SetFlag(ThreadFlag::ACTIVE_BARRIER);
        SetCheckSafePointStatus();
    }
}

void JSThread::ResumeThread(bool internalSuspend)
{
    ASSERT(!g_isEnableCMCGC);
    LockHolder lock(suspendLock_);
    if (!internalSuspend) {
        // do smth here if we want to combine internal and external suspension
    }
    if (suspendCount_ > 0) {
        suspendCount_--;
        if (suspendCount_ == 0) {
            ClearFlag(ThreadFlag::SUSPEND_REQUEST);
            ResetCheckSafePointStatus();
        }
    }
    suspendCondVar_.Signal();
}

void JSThread::WaitSuspension()
{
    if (UNLIKELY(g_isEnableCMCGC)) {
        GetThreadHolder()->WaitSuspension();
    } else {
        constexpr int TIMEOUT = 100;
        ThreadState oldState = GetState();
        UpdateState(ThreadState::IS_SUSPENDED);
        {
            OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "SuspendTime::WaitSuspension", "");
            LockHolder lock(suspendLock_);
            while (suspendCount_ > 0) {
                suspendCondVar_.TimedWait(&suspendLock_, TIMEOUT);
                // we need to do smth if Runtime is terminating at this point
            }
        }
        UpdateState(oldState);
    }
}

void JSThread::ManagedCodeBegin()
{
    ASSERT(!IsInManagedState());
    if (LIKELY(!g_isEnableCMCGC)) {
        UpdateState(ThreadState::RUNNING);
    } else {
        GetThreadHolder()->TransferToRunning();
    }
}

void JSThread::ManagedCodeEnd()
{
    ASSERT(IsInManagedState());
    if (LIKELY(!g_isEnableCMCGC)) {
        UpdateState(ThreadState::NATIVE);
    } else {
        GetThreadHolder()->TransferToNative();
    }
}

void JSThread::TransferFromRunningToSuspended(ThreadState newState)
{
    ASSERT(!g_isEnableCMCGC);
    ASSERT(currentThread == this);
    StoreSuspendedState(newState);
    CheckAndPassActiveBarrier();
}

void JSThread::UpdateStackInfo(void *stackInfo, StackInfoOpKind opKind)
{
    switch (opKind) {
        case SwitchToSubStackInfo: {
            StackInfo *subStackInfo = reinterpret_cast<StackInfo*>(stackInfo);
            if (subStackInfo == nullptr) {
                LOG_ECMA(ERROR) << "fatal error, subStack not exist";
                break;
            }
            // process stackLimit
            mainStackInfo_.stackLimit = glueData_.stackLimit_;
            glueData_.stackLimit_ = subStackInfo->stackLimit;
            // process lastLeaveFrame
            mainStackInfo_.lastLeaveFrame = reinterpret_cast<uint64_t>(glueData_.leaveFrame_);
            glueData_.leaveFrame_ =
                reinterpret_cast<uint64_t *>(subStackInfo->lastLeaveFrame);
            isInSubStack_ = true;

            LOG_ECMA(DEBUG) << "Switch to subStack: "
                            << ", stack limit: " << glueData_.stackLimit_
                            << ", stack lastLeaveFrame: " << glueData_.leaveFrame_;
            break;
        }
        case SwitchToMainStackInfo: {
            // process stackLimit
            glueData_.stackLimit_ = mainStackInfo_.stackLimit;
            // process lastLeaveFrame
            glueData_.leaveFrame_ = reinterpret_cast<uint64_t *>(mainStackInfo_.lastLeaveFrame);
            isInSubStack_ = false;

            LOG_ECMA(DEBUG) << "Switch to mainStack: "
                            << ", main stack limit: " << mainStackInfo_.stackLimit
                            << ", main stack lastLeaveFrame: " << mainStackInfo_.lastLeaveFrame;
            break;
        }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

void JSThread::TransferToRunning()
{
    ASSERT(!g_isEnableCMCGC);
    ASSERT(!IsDaemonThread());
    ASSERT(currentThread == this);
    StoreRunningState(ThreadState::RUNNING);
    // Invoke free weak global callback when thread switch to running
    if (!weakNodeFreeGlobalCallbacks_.empty()) {
        InvokeWeakNodeFreeGlobalCallBack();
    }
    if (fullMarkRequest_) {
        fullMarkRequest_ = const_cast<Heap*>(vm_->GetHeap())->TryTriggerFullMarkBySharedLimit();
    }
}

void JSThread::TransferDaemonThreadToRunning()
{
    ASSERT(!g_isEnableCMCGC);
    ASSERT(IsDaemonThread());
    ASSERT(currentThread == this);
    StoreRunningState(ThreadState::RUNNING);
}

void JSThread::StoreState(ThreadState newState)
{
    ASSERT(!g_isEnableCMCGC);
    while (true) {
        ThreadStateAndFlags oldStateAndFlags;
        oldStateAndFlags.asNonvolatileInt = glueData_.stateAndFlags_.asInt;

        ThreadStateAndFlags newStateAndFlags;
        newStateAndFlags.asNonvolatileStruct.flags = oldStateAndFlags.asNonvolatileStruct.flags;
        newStateAndFlags.asNonvolatileStruct.state = newState;

        bool done = glueData_.stateAndFlags_.asAtomicInt.compare_exchange_weak(oldStateAndFlags.asNonvolatileInt,
                                                                               newStateAndFlags.asNonvolatileInt,
                                                                               std::memory_order_release);
        if (LIKELY(done)) {
            break;
        }
    }
}

void JSThread::StoreRunningState([[maybe_unused]] ThreadState newState)
{
    ASSERT(!g_isEnableCMCGC);
    ASSERT(newState == ThreadState::RUNNING);
    while (true) {
        ThreadStateAndFlags oldStateAndFlags;
        oldStateAndFlags.asNonvolatileInt = glueData_.stateAndFlags_.asInt;
        ASSERT(oldStateAndFlags.asNonvolatileStruct.state != ThreadState::RUNNING);

        if (LIKELY(oldStateAndFlags.asNonvolatileStruct.flags == ThreadFlag::NO_FLAGS)) {
            ThreadStateAndFlags newStateAndFlags;
            newStateAndFlags.asNonvolatileStruct.flags = oldStateAndFlags.asNonvolatileStruct.flags;
            newStateAndFlags.asNonvolatileStruct.state = newState;

            if (glueData_.stateAndFlags_.asAtomicInt.compare_exchange_weak(oldStateAndFlags.asNonvolatileInt,
                                                                           newStateAndFlags.asNonvolatileInt,
                                                                           std::memory_order_release)) {
                break;
            }
        } else if ((oldStateAndFlags.asNonvolatileStruct.flags & ThreadFlag::ACTIVE_BARRIER) != 0) {
            PassSuspendBarrier();
        } else if ((oldStateAndFlags.asNonvolatileStruct.flags & ThreadFlag::SUSPEND_REQUEST) != 0) {
            constexpr int TIMEOUT = 100;
            OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "SuspendTime::StoreRunningState", "");
            LockHolder lock(suspendLock_);
            while (suspendCount_ > 0) {
                suspendCondVar_.TimedWait(&suspendLock_, TIMEOUT);
            }
            ASSERT(!HasSuspendRequest());
        }
    }
}

inline void JSThread::StoreSuspendedState(ThreadState newState)
{
    ASSERT(!g_isEnableCMCGC);
    ASSERT(newState != ThreadState::RUNNING);
    StoreState(newState);
}

void JSThread::PostFork()
{
    SetThreadId();
    if (currentThread == nullptr) {
        currentThread = this;
        if (LIKELY(!g_isEnableCMCGC)) {
            ASSERT(GetState() == ThreadState::CREATED);
            UpdateState(ThreadState::NATIVE);
        } else {
            GetThreadHolder()->TransferToNative();
        }
    } else {
        // We tried to call fork in the same thread
        ASSERT(currentThread == this);
        if (LIKELY(!g_isEnableCMCGC)) {
            ASSERT(GetState() == ThreadState::NATIVE);
        } else {
            GetThreadHolder()->TransferToNative();
        }
    }
}
#ifndef NDEBUG
bool JSThread::IsInManagedState() const
{
    ASSERT(this == JSThread::GetCurrent());
    return IsInRunningState();
}

MutatorLock::MutatorLockState JSThread::GetMutatorLockState() const
{
    return mutatorLockState_;
}

void JSThread::SetMutatorLockState(MutatorLock::MutatorLockState newState)
{
    mutatorLockState_ = newState;
}
#endif

JSHClass *JSThread::GetArrayInstanceHClass(ElementsKind kind, bool isPrototype) const
{
    JSHandle<GlobalEnv> env = GetGlobalEnv();
    return GetArrayInstanceHClass(env, kind, isPrototype);
}

JSHClass *JSThread::GetArrayInstanceHClass(JSHandle<GlobalEnv> env, ElementsKind kind, bool isPrototype) const
{
    GlobalEnvField index = glueData_.arrayHClassIndexes_.GetArrayInstanceHClassIndex(kind, isPrototype);
    auto exceptArrayHClass = env->GetGlobalEnvObjectByIndex(static_cast<size_t>(index)).GetTaggedValue();
    auto exceptRecvHClass = JSHClass::Cast(exceptArrayHClass.GetTaggedObject());
    ASSERT(exceptRecvHClass->IsJSArray());
    return exceptRecvHClass;
}

ModuleManager *JSThread::GetModuleManager() const
{
    JSHandle<GlobalEnv> globalEnv = GetGlobalEnv();
    JSHandle<JSNativePointer> nativePointer(globalEnv->GetModuleManagerNativePointer());
    ModuleManager *moduleManager = reinterpret_cast<ModuleManager *>(nativePointer->GetExternalPointer());
    return moduleManager;
}

JSTaggedValue JSThread::GetCurrentGlobalEnv(JSTaggedValue currentEnv)
{
    auto globalEnv = BaseEnv::Cast(currentEnv.GetTaggedObject())->GetGlobalEnv(this);
    if (globalEnv.IsHole()) {
        return GetGlueGlobalEnv();
    }
    return globalEnv;
}
}  // namespace panda::ecmascript
