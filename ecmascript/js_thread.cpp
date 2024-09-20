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

#include "ecmascript/runtime.h"
#include "ecmascript/debugger/js_debugger_manager.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"

#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS) && !defined(PANDA_TARGET_IOS)
#include <sys/resource.h>
#endif

#if defined(ENABLE_EXCEPTION_BACKTRACE)
#include "ecmascript/platform/backtrace.h"
#endif
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#endif
#include "ecmascript/dfx/vm_thread_control.h"
#include "ecmascript/ecma_global_storage.h"
#include "ecmascript/ic/properties_cache.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/jit/jit.h"

namespace panda::ecmascript {
using CommonStubCSigns = panda::ecmascript::kungfu::CommonStubCSigns;
using BytecodeStubCSigns = panda::ecmascript::kungfu::BytecodeStubCSigns;

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
        jsThread->UpdateState(ThreadState::NATIVE);
    }
}

void JSThread::UnregisterThread(JSThread *jsThread)
{
    if (currentThread == jsThread) {
        jsThread->UpdateState(ThreadState::TERMINATED);
        currentThread = nullptr;
    } else {
        // We have created this JSThread instance but hadn't forked it.
        ASSERT(jsThread->GetState() == ThreadState::CREATED);
        jsThread->UpdateState(ThreadState::TERMINATED);
    }
    Runtime::GetInstance()->UnregisterThread(jsThread);
}

// static
JSThread *JSThread::Create(EcmaVM *vm)
{
    auto jsThread = new JSThread(vm);

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
    jsThread->glueData_.isEnableElementsKind_ = vm->IsEnableElementsKind();
    jsThread->glueData_.isEnableForceIC_ = ecmascript::pgo::PGOProfilerManager::GetInstance()->IsEnableForceIC();
    jsThread->SetThreadId();

    RegisterThread(jsThread);
    return jsThread;
}

JSThread::JSThread(EcmaVM *vm) : id_(os::thread::GetCurrentThreadId()), vm_(vm)
{
    auto chunk = vm->GetChunk();
    if (!vm_->GetJSOptions().EnableGlobalLeakCheck()) {
        globalStorage_ = chunk->New<EcmaGlobalStorage<Node>>(this, vm->GetNativeAreaAllocator());
        newGlobalHandle_ = [this](JSTaggedType value) { return globalStorage_->NewGlobalHandle(value); };
        disposeGlobalHandle_ = [this](uintptr_t nodeAddr) { globalStorage_->DisposeGlobalHandle(nodeAddr); };
        setWeak_ = [this](uintptr_t nodeAddr, void *ref, WeakClearCallback freeGlobalCallBack,
                        WeakClearCallback nativeFinalizeCallBack) {
            return globalStorage_->SetWeak(nodeAddr, ref, freeGlobalCallBack, nativeFinalizeCallBack);
        };
        clearWeak_ = [this](uintptr_t nodeAddr) { return globalStorage_->ClearWeak(nodeAddr); };
        isWeak_ = [this](uintptr_t addr) { return globalStorage_->IsWeak(addr); };
    } else {
        globalDebugStorage_ = chunk->New<EcmaGlobalStorage<DebugNode>>(this, vm->GetNativeAreaAllocator());
        newGlobalHandle_ = [this](JSTaggedType value) { return globalDebugStorage_->NewGlobalHandle(value); };
        disposeGlobalHandle_ = [this](uintptr_t nodeAddr) { globalDebugStorage_->DisposeGlobalHandle(nodeAddr); };
        setWeak_ = [this](uintptr_t nodeAddr, void *ref, WeakClearCallback freeGlobalCallBack,
                        WeakClearCallback nativeFinalizeCallBack) {
            return globalDebugStorage_->SetWeak(nodeAddr, ref, freeGlobalCallBack, nativeFinalizeCallBack);
        };
        clearWeak_ = [this](uintptr_t nodeAddr) { return globalDebugStorage_->ClearWeak(nodeAddr); };
        isWeak_ = [this](uintptr_t addr) { return globalDebugStorage_->IsWeak(addr); };
    }
    vmThreadControl_ = new VmThreadControl(this);
    SetBCStubStatus(BCStubStatus::NORMAL_BC_STUB);
}

JSThread::JSThread(EcmaVM *vm, ThreadType threadType) : id_(os::thread::GetCurrentThreadId()),
                                                        vm_(vm), threadType_(threadType)
{
    ASSERT(threadType == ThreadType::JIT_THREAD);
    // jit thread no need GCIterating
    readyForGCIterating_ = false;
    RegisterThread(this);
};

JSThread::JSThread(ThreadType threadType) : threadType_(threadType)
{
    ASSERT(threadType == ThreadType::DAEMON_THREAD);
    // daemon thread no need GCIterating
    readyForGCIterating_ = false;
}

JSThread::~JSThread()
{
    readyForGCIterating_ = false;
    if (globalStorage_ != nullptr) {
        GetEcmaVM()->GetChunk()->Delete(globalStorage_);
        globalStorage_ = nullptr;
    }
    if (globalDebugStorage_ != nullptr) {
        GetEcmaVM()->GetChunk()->Delete(globalDebugStorage_);
        globalDebugStorage_ = nullptr;
    }

    for (auto item : contexts_) {
        GetNativeAreaAllocator()->Free(item->GetFrameBase(), sizeof(JSTaggedType) *
            vm_->GetEcmaParamConfiguration().GetMaxStackSize());
        item->SetFrameBase(nullptr);
        delete item;
    }
    contexts_.clear();
    GetNativeAreaAllocator()->FreeArea(regExpCache_);

    glueData_.frameBase_ = nullptr;
    nativeAreaAllocator_ = nullptr;
    heapRegionAllocator_ = nullptr;
    regExpCache_ = nullptr;
    if (vmThreadControl_ != nullptr) {
        delete vmThreadControl_;
        vmThreadControl_ = nullptr;
    }
    // DaemonThread will be unregistered when the binding std::thread release.
    if (!IsDaemonThread()) {
        UnregisterThread(this);
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

void JSThread::ClearException()
{
    glueData_.exception_ = JSTaggedValue::Hole();
}

void JSThread::SetEnableForceIC(bool isEnableForceIC)
{
    glueData_.isEnableForceIC_ = isEnableForceIC;
}

bool JSThread::IsEnableForceIC() const
{
    return glueData_.isEnableForceIC_;
}

JSTaggedValue JSThread::GetCurrentLexenv() const
{
    FrameHandler frameHandler(this);
    return frameHandler.GetEnv();
}

JSTaggedValue JSThread::GetCurrentFunction() const
{
    FrameHandler frameHandler(this);
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
        auto frameHandler = FrameHandler(this);
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

void JSThread::InvokeSharedNativePointerCallbacks()
{
    auto &callbacks = vm_->GetSharedNativePointerCallbacks();
    while (!callbacks.empty()) {
        auto callbackPair = callbacks.back();
        callbacks.pop_back();
        ASSERT(callbackPair.first != nullptr && callbackPair.second.first != nullptr &&
               callbackPair.second.second != nullptr);
        auto callback = callbackPair.first;
        (*callback)(env_, callbackPair.second.first, callbackPair.second.second);
    }
}

void JSThread::InvokeWeakNodeNativeFinalizeCallback()
{
    // the second callback may lead to another GC, if this, return directly;
    if (runningNativeFinalizeCallbacks_) {
        return;
    }
    runningNativeFinalizeCallbacks_ = true;
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "InvokeNativeFinalizeCallbacks num:"
        + std::to_string(weakNodeNativeFinalizeCallbacks_.size()));
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
    return GetEcmaVM()->GetJSOptions().IsStartGlobalLeakCheck();
}

bool JSThread::EnableGlobalObjectLeakCheck() const
{
    return GetEcmaVM()->GetJSOptions().EnableGlobalObjectLeakCheck();
}

bool JSThread::EnableGlobalPrimitiveLeakCheck() const
{
    return GetEcmaVM()->GetJSOptions().EnableGlobalPrimitiveLeakCheck();
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

void JSThread::Iterate(const RootVisitor &visitor, const RootRangeVisitor &rangeVisitor,
    const RootBaseAndDerivedVisitor &derivedVisitor)
{
    if (!glueData_.exception_.IsHole()) {
        visitor(Root::ROOT_VM, ObjectSlot(ToUintPtr(&glueData_.exception_)));
    }
    rangeVisitor(
        Root::ROOT_VM, ObjectSlot(glueData_.builtinEntries_.Begin()), ObjectSlot(glueData_.builtinEntries_.End()));

    EcmaContext *tempContext = glueData_.currentContext_;
    for (EcmaContext *context : contexts_) {
        // visit stack roots
        SwitchCurrentContext(context, true);
        FrameHandler frameHandler(this);
        frameHandler.Iterate(visitor, rangeVisitor, derivedVisitor);
        context->Iterate(visitor, rangeVisitor);
    }
    SwitchCurrentContext(tempContext, true);
    // visit tagged handle storage roots
    if (vm_->GetJSOptions().EnableGlobalLeakCheck()) {
        IterateHandleWithCheck(visitor, rangeVisitor);
    } else {
        size_t globalCount = 0;
        globalStorage_->IterateUsageGlobal([visitor, &globalCount](Node *node) {
            JSTaggedValue value(node->GetObject());
            if (value.IsHeapObject()) {
                visitor(ecmascript::Root::ROOT_HANDLE, ecmascript::ObjectSlot(node->GetObjectAddress()));
            }
            globalCount++;
        });
        static bool hasCheckedGlobalCount = false;
        static const size_t WARN_GLOBAL_COUNT = 100000;
        if (!hasCheckedGlobalCount && globalCount >= WARN_GLOBAL_COUNT) {
            LOG_ECMA(WARN) << "Global reference count is " << globalCount << ",It exceed the upper limit 100000!";
            hasCheckedGlobalCount = true;
        }
    }
}
void JSThread::IterateJitCodeMap(const JitCodeMapVisitor &jitCodeMapVisitor)
{
    jitCodeMapVisitor(jitCodeMaps_);
}

void JSThread::IterateHandleWithCheck(const RootVisitor &visitor, const RootRangeVisitor &rangeVisitor)
{
    size_t handleCount = 0;
    for (EcmaContext *context : contexts_) {
        handleCount += context->IterateHandle(rangeVisitor);
    }

    size_t globalCount = 0;
    static const int JS_TYPE_LAST = static_cast<int>(JSType::TYPE_LAST);
    int typeCount[JS_TYPE_LAST] = { 0 };
    int primitiveCount = 0;
    bool isStopObjectLeakCheck = EnableGlobalObjectLeakCheck() && !IsStartGlobalLeakCheck() && stackTraceFd_ > 0;
    bool isStopPrimitiveLeakCheck = EnableGlobalPrimitiveLeakCheck() && !IsStartGlobalLeakCheck() && stackTraceFd_ > 0;
    std::ostringstream buffer;
    globalDebugStorage_->IterateUsageGlobal([this, visitor, &globalCount, &typeCount, &primitiveCount,
        isStopObjectLeakCheck, isStopPrimitiveLeakCheck, &buffer](DebugNode *node) {
        node->MarkCount();
        JSTaggedValue value(node->GetObject());
        if (value.IsHeapObject()) {
            visitor(ecmascript::Root::ROOT_HANDLE, ecmascript::ObjectSlot(node->GetObjectAddress()));
            TaggedObject *object = value.GetTaggedObject();
            MarkWord word(value.GetTaggedObject());
            if (word.IsForwardingAddress()) {
                object = word.ToForwardingAddress();
            }
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
    });

    if (isStopObjectLeakCheck || isStopPrimitiveLeakCheck) {
        buffer << "Global leak check success!";
        WriteToStackTraceFd(buffer);
        CloseStackTraceFd();
    }
    // Determine whether memory leakage by checking handle and global count.
    LOG_ECMA(INFO) << "Iterate root handle count:" << handleCount << ", global handle count:" << globalCount;
    OPTIONAL_LOG(GetEcmaVM(), INFO) << "Global type Primitive count:" << primitiveCount;
    // Print global object type statistic.
    static const int MIN_COUNT_THRESHOLD = 50;
    for (int i = 0; i < JS_TYPE_LAST; i++) {
        if (typeCount[i] > MIN_COUNT_THRESHOLD) {
            OPTIONAL_LOG(GetEcmaVM(), INFO) << "Global type " << JSHClass::DumpJSType(JSType(i))
                                            << " count:" << typeCount[i];
        }
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
            ObjectFactory *factory = GetEcmaVM()->GetFactory();
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
            ObjectFactory *factory = GetEcmaVM()->GetFactory();
            JSHandle<JSObject> error = factory->GetJSError(base::ErrorType::RANGE_ERROR,
                                                           "Stack overflow!", StackCheck::NO);
            SetException(error.GetTaggedValue());
        }
        return true;
    }
    return false;
}

uintptr_t *JSThread::ExpandHandleStorage()
{
    return GetCurrentEcmaContext()->ExpandHandleStorage();
}

void JSThread::ShrinkHandleStorage(int prevIndex)
{
    GetCurrentEcmaContext()->ShrinkHandleStorage(prevIndex);
}

void JSThread::NotifyStableArrayElementsGuardians(JSHandle<JSObject> receiver, StableArrayChangeKind changeKind)
{
    if (!glueData_.stableArrayElementsGuardians_) {
        return;
    }
    if (!receiver->GetJSHClass()->IsPrototype() && !receiver->IsJSArray()) {
        return;
    }
    auto env = GetEcmaVM()->GetGlobalEnv();
    if (receiver.GetTaggedValue() == env->GetObjectFunctionPrototype().GetTaggedValue() ||
        receiver.GetTaggedValue() == env->GetArrayPrototype().GetTaggedValue()) {
        glueData_.stableArrayElementsGuardians_ = false;
        return;
    }
    if (changeKind == StableArrayChangeKind::PROTO && receiver->IsJSArray()) {
        glueData_.stableArrayElementsGuardians_ = false;
    }
}

void JSThread::ResetGuardians()
{
    glueData_.stableArrayElementsGuardians_ = true;
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

JSHClass *JSThread::GetArrayInstanceHClass(ElementsKind kind, bool isPrototype) const
{
    auto iter = GetArrayHClassIndexMap().find(kind);
    ASSERT(iter != GetArrayHClassIndexMap().end());
    auto index = isPrototype ? static_cast<size_t>(iter->second.second) : static_cast<size_t>(iter->second.first);
    auto exceptArrayHClass = GlobalConstants()->GetGlobalConstantObject(index);
    auto exceptRecvHClass = JSHClass::Cast(exceptArrayHClass.GetTaggedObject());
    ASSERT(exceptRecvHClass->IsJSArray());
    return exceptRecvHClass;
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

size_t JSThread::GetBuiltinHClassOffset(BuiltinTypeId type, bool isArch32)
{
    return GetGlueDataOffset() + GlueData::GetBuiltinHClassOffset(type, isArch32);
}

size_t JSThread::GetBuiltinPrototypeHClassOffset(BuiltinTypeId type, bool isArch32)
{
    return GetGlueDataOffset() + GlueData::GetBuiltinPrototypeHClassOffset(type, isArch32);
}

void JSThread::CheckSwitchDebuggerBCStub()
{
    auto isDebug = GetEcmaVM()->GetJsDebuggerManager()->IsDebugMode();
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
    if (IsPGOProfilerEnable()) {
        if (GetBCStubStatus() == BCStubStatus::NORMAL_BC_STUB) {
            SetBCStubStatus(BCStubStatus::PROFILE_BC_STUB);
            isSwitch = true;
        }
    } else {
        if (GetBCStubStatus() == BCStubStatus::PROFILE_BC_STUB) {
            SetBCStubStatus(BCStubStatus::NORMAL_BC_STUB);
            isSwitch = true;
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

void JSThread::TerminateExecution()
{
    // set the TERMINATE_ERROR to exception
    ObjectFactory *factory = GetEcmaVM()->GetFactory();
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::TERMINATION_ERROR,
        "Terminate execution!", StackCheck::NO);
    SetException(error.GetTaggedValue());
}

void JSThread::CheckAndPassActiveBarrier()
{
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

bool JSThread::CheckSafepoint()
{
    ResetCheckSafePointStatus();

    if (HasTerminationRequest()) {
        TerminateExecution();
        SetVMTerminated(true);
        SetTerminationRequest(false);
    }

    if (HasSuspendRequest()) {
        WaitSuspension();
    }

    // vmThreadControl_ 's thread_ is current JSThread's this.
    if (VMNeedSuspension()) {
        vmThreadControl_->SuspendVM();
    }
    if (HasInstallMachineCode()) {
        vm_->GetJit()->InstallTasks(this);
        SetInstallMachineCode(false);
    }

#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    if (needProfiling_.load() && !isProfiling_) {
        DFXJSNApi::StartCpuProfilerForFile(vm_, profileName_, CpuProfiler::INTERVAL_OF_INNER_START);
        SetNeedProfiling(false);
    }
#endif // ECMASCRIPT_SUPPORT_CPUPROFILER
    bool gcTriggered = false;
#ifndef NDEBUG
    if (vm_->GetJSOptions().EnableForceGC()) {
        GetEcmaVM()->CollectGarbage(TriggerGCType::FULL_GC);
        gcTriggered = true;
    }
#endif
    auto heap = const_cast<Heap *>(GetEcmaVM()->GetHeap());
    // Handle exit app senstive scene
    heap->HandleExitHighSensitiveEvent();

    // After concurrent mark finish, should trigger gc here to avoid create much floating garbage
    // except in serialize or high sensitive event
    if (IsMarkFinished() && heap->GetConcurrentMarker()->IsTriggeredConcurrentMark()
        && !heap->GetOnSerializeEvent() && !heap->InSensitiveStatus()) {
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
        !GetEcmaVM()->GetHeap()->IsAlive(reinterpret_cast<TaggedObject *>(value))) {
        LOG_FULL(FATAL) << "value:" << value << " is invalid!";
    }
}

bool JSThread::CpuProfilerCheckJSTaggedType(JSTaggedType value) const
{
    if (JSTaggedValue(value).IsHeapObject() &&
        !GetEcmaVM()->GetHeap()->IsAlive(reinterpret_cast<TaggedObject *>(value))) {
        return false;
    }
    return true;
}

// static
size_t JSThread::GetAsmStackLimit()
{
#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS) && !defined(PANDA_TARGET_IOS)
    // js stack limit
    size_t result = GetCurrentStackPosition() - EcmaParamConfiguration::GetDefalutStackSize();
    int ret = -1;
    void *stackAddr = nullptr;
    size_t size = 0;
#if defined(ENABLE_FFRT_INTERFACES)
    if (!ffrt_get_current_coroutine_stack(&stackAddr, &size)) {
        pthread_attr_t attr;
        ret = pthread_getattr_np(pthread_self(), &attr);
        if (ret != 0) {
            LOG_ECMA(ERROR) << "Get current thread attr failed";
            return result;
        }
        ret = pthread_attr_getstack(&attr, &stackAddr, &size);
        if (pthread_attr_destroy(&attr) != 0) {
            LOG_ECMA(ERROR) << "Destroy current thread attr failed";
        }
        if (ret != 0) {
            LOG_ECMA(ERROR) << "Get current thread stack size failed";
            return result;
        }
    }
#else
    pthread_attr_t attr;
    ret = pthread_getattr_np(pthread_self(), &attr);
    if (ret != 0) {
        LOG_ECMA(ERROR) << "Get current thread attr failed";
        return result;
    }
    ret = pthread_attr_getstack(&attr, &stackAddr, &size);
    if (pthread_attr_destroy(&attr) != 0) {
        LOG_ECMA(ERROR) << "Destroy current thread attr failed";
    }
    if (ret != 0) {
        LOG_ECMA(ERROR) << "Get current thread stack size failed";
        return result;
    }
#endif

    bool isMainThread = IsMainThread();
    uintptr_t threadStackLimit = reinterpret_cast<uintptr_t>(stackAddr);
    uintptr_t threadStackStart = threadStackLimit + size;
    if (isMainThread) {
        struct rlimit rl;
        ret = getrlimit(RLIMIT_STACK, &rl);
        if (ret != 0) {
            LOG_ECMA(ERROR) << "Get current thread stack size failed";
            return result;
        }
        if (rl.rlim_cur > DEFAULT_MAX_SYSTEM_STACK_SIZE) {
            LOG_ECMA(ERROR) << "Get current thread stack size exceed " << DEFAULT_MAX_SYSTEM_STACK_SIZE
                            << " : " << rl.rlim_cur;
            return result;
        }
        threadStackLimit = threadStackStart - rl.rlim_cur;
    }

    if (result < threadStackLimit) {
        result = threadStackLimit;
    }

    LOG_INTERPRETER(DEBUG) << "Current thread stack start: " << reinterpret_cast<void *>(threadStackStart);
    LOG_INTERPRETER(DEBUG) << "Used stack before js stack start: "
                           << reinterpret_cast<void *>(threadStackStart - GetCurrentStackPosition());
    LOG_INTERPRETER(DEBUG) << "Current thread asm stack limit: " << reinterpret_cast<void *>(result);

    // To avoid too much times of stack overflow checking, we only check stack overflow before push vregs or
    // parameters of variable length. So we need a reserved size of stack to make sure stack won't be overflowed
    // when push other data.
    result += EcmaParamConfiguration::GetDefaultReservedStackSize();
    if (threadStackStart <= result) {
        LOG_FULL(FATAL) << "Too small stackSize to run jsvm";
    }
    return result;
#else
    return 0;
#endif
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

void JSThread::PushContext(EcmaContext *context)
{
    const_cast<Heap *>(vm_->GetHeap())->WaitAllTasksFinished();
    contexts_.emplace_back(context);

    if (!glueData_.currentContext_) {
        // The first context in ecma vm.
        glueData_.currentContext_ = context;
        context->SetFramePointers(const_cast<JSTaggedType *>(GetCurrentSPFrame()),
            const_cast<JSTaggedType *>(GetLastLeaveFrame()),
            const_cast<JSTaggedType *>(GetLastFp()));
        context->SetFrameBase(glueData_.frameBase_);
        context->SetStackLimit(glueData_.stackLimit_);
        context->SetStackStart(glueData_.stackStart_);
    } else {
        // algin with 16
        size_t maxStackSize = vm_->GetEcmaParamConfiguration().GetMaxStackSize();
        context->SetFrameBase(static_cast<JSTaggedType *>(
            vm_->GetNativeAreaAllocator()->Allocate(sizeof(JSTaggedType) * maxStackSize)));
        context->SetFramePointers(context->GetFrameBase() + maxStackSize, nullptr, nullptr);
        context->SetStackLimit(GetAsmStackLimit());
        context->SetStackStart(GetCurrentStackPosition());
        EcmaInterpreter::InitStackFrame(context);
    }
}

void JSThread::PopContext()
{
    contexts_.pop_back();
    glueData_.currentContext_ = contexts_.back();
}

void JSThread::SwitchCurrentContext(EcmaContext *currentContext, bool isInIterate)
{
    ASSERT(std::count(contexts_.begin(), contexts_.end(), currentContext));

    glueData_.currentContext_->SetFramePointers(const_cast<JSTaggedType *>(GetCurrentSPFrame()),
        const_cast<JSTaggedType *>(GetLastLeaveFrame()),
        const_cast<JSTaggedType *>(GetLastFp()));
    glueData_.currentContext_->SetFrameBase(glueData_.frameBase_);
    glueData_.currentContext_->SetStackLimit(GetStackLimit());
    glueData_.currentContext_->SetStackStart(GetStackStart());
    glueData_.currentContext_->SetGlobalEnv(GetGlueGlobalEnv());
    // When the glueData_.currentContext_ is not fully initialized，glueData_.globalObject_ will be hole.
    // Assigning hole to JSGlobalObject could cause a mistake at builtins initalization.
    if (!glueData_.globalObject_.IsHole()) {
        glueData_.currentContext_->GetGlobalEnv()->SetJSGlobalObject(this, glueData_.globalObject_);
    }

    SetCurrentSPFrame(currentContext->GetCurrentFrame());
    SetLastLeaveFrame(currentContext->GetLeaveFrame());
    SetLastFp(currentContext->GetLastFp());
    glueData_.frameBase_ = currentContext->GetFrameBase();
    glueData_.stackLimit_ = currentContext->GetStackLimit();
    glueData_.stackStart_ = currentContext->GetStackStart();
    if (!currentContext->GlobalEnvIsHole()) {
        SetGlueGlobalEnv(*(currentContext->GetGlobalEnv()));
        /**
         * GlobalObject has two copies, one in GlueData and one in Context.GlobalEnv, when switch context, will save
         * GlobalObject in GlueData to CurrentContext.GlobalEnv(is this nessary?), and then switch to new context,
         * save the GlobalObject in NewContext.GlobalEnv to GlueData.
         * The initial value of GlobalObject in Context.GlobalEnv is Undefined, but in GlueData is Hole,
         * so if two SharedGC happened during the builtins initalization like this, maybe will cause incorrect scene:
         *
         * Default:
         * Slot for GlobalObject:              Context.GlobalEnv            GlueData
         * value:                                 Undefined                   Hole
         *
         * First SharedGC(JSThread::SwitchCurrentContext), Set GlobalObject from Context.GlobalEnv to GlueData:
         * Slot for GlobalObject:              Context.GlobalEnv            GlueData
         * value:                                 Undefined                 Undefined
         *
         * Builtins Initialize, Create GlobalObject and Set to Context.GlobalEnv:
         * Slot for GlobalObject:              Context.GlobalEnv            GlueData
         * value:                                    Obj                    Undefined
         *
         * Second SharedGC(JSThread::SwitchCurrentContext), Set GlobalObject from GlueData to Context.GlobalEnv:
         * Slot for GlobalObject:              Context.GlobalEnv            GlueData
         * value:                                 Undefined                 Undefined
         *
         * So when copy values between Context.GlobalEnv and GlueData, need to check if the value is Hole in GlueData,
         * and if is Undefined in Context.GlobalEnv, because the initial value is different.
        */
        if (!currentContext->GetGlobalEnv()->GetGlobalObject().IsUndefined()) {
            SetGlobalObject(currentContext->GetGlobalEnv()->GetGlobalObject());
        }
    }
    if (!isInIterate) {
        // If isInIterate is true, it means it is in GC iterate and global variables are no need to change.
        glueData_.globalConst_ = const_cast<GlobalEnvConstants *>(currentContext->GlobalConstants());
    }

    glueData_.currentContext_ = currentContext;
}

bool JSThread::EraseContext(EcmaContext *context)
{
    const_cast<Heap *>(vm_->GetHeap())->WaitAllTasksFinished();
    bool isCurrentContext = false;
    auto iter = std::find(contexts_.begin(), contexts_.end(), context);
    if (*iter == context) {
        if (glueData_.currentContext_ == context) {
            isCurrentContext = true;
        }
        contexts_.erase(iter);
        if (isCurrentContext) {
            SwitchCurrentContext(contexts_.back());
        }
        return true;
    }
    return false;
}

void JSThread::ClearContextCachedConstantPool()
{
    for (EcmaContext *context : contexts_) {
        context->ClearCachedConstantPool();
    }
}

PropertiesCache *JSThread::GetPropertiesCache() const
{
    return glueData_.currentContext_->GetPropertiesCache();
}

const GlobalEnvConstants *JSThread::GetFirstGlobalConst() const
{
    return contexts_[0]->GlobalConstants();
}

bool JSThread::IsAllContextsInitialized() const
{
    return contexts_.back()->IsInitialized();
}

bool JSThread::IsReadyToUpdateDetector() const
{
    return !GetEnableLazyBuiltins() && IsAllContextsInitialized();
}

Area *JSThread::GetOrCreateRegExpCache()
{
    if (regExpCache_ == nullptr) {
        regExpCache_ = nativeAreaAllocator_->AllocateArea(MAX_REGEXP_CACHE_SIZE);
    }
    return regExpCache_;
}

void JSThread::InitializeBuiltinObject(const std::string& key)
{
    BuiltinIndex& builtins = BuiltinIndex::GetInstance();
    auto index = builtins.GetBuiltinIndex(key);
    ASSERT(index != BuiltinIndex::NOT_FOUND);
    /*
        If using `auto globalObject = GetEcmaVM()->GetGlobalEnv()->GetGlobalObject()` here,
        it will cause incorrect result in multi-context environment. For example:

        ```ts
        let obj = {};
        print(obj instanceof Object); // instead of true, will print false
        ```
    */
    auto globalObject = contexts_.back()->GetGlobalEnv()->GetGlobalObject();
    auto jsObject = JSHandle<JSObject>(this, globalObject);
    auto box = jsObject->GetGlobalPropertyBox(this, key);
    if (box == nullptr) {
        return;
    }
    auto& entry = glueData_.builtinEntries_.builtin_[index];
    entry.box_ = JSTaggedValue::Cast(box);
    auto builtin = JSHandle<JSObject>(this, box->GetValue());
    auto hclass = builtin->GetJSHClass();
    entry.hClass_ = JSTaggedValue::Cast(hclass);
}

void JSThread::InitializeBuiltinObject()
{
    BuiltinIndex& builtins = BuiltinIndex::GetInstance();
    for (auto key: builtins.GetBuiltinKeys()) {
        InitializeBuiltinObject(key);
    }
}

bool JSThread::IsPropertyCacheCleared() const
{
    for (EcmaContext *context : contexts_) {
        if (!context->GetPropertiesCache()->IsCleared()) {
            return false;
        }
    }
    return true;
}

void JSThread::UpdateState(ThreadState newState)
{
    ThreadState oldState = GetState();
    if (oldState == ThreadState::RUNNING && newState != ThreadState::RUNNING) {
        TransferFromRunningToSuspended(newState);
    } else if (oldState != ThreadState::RUNNING && newState == ThreadState::RUNNING) {
        TransferToRunning();
    } else {
        // Here can be some extra checks...
        StoreState(newState);
    }
}

void JSThread::SuspendThread(bool internalSuspend, SuspendBarrier* barrier)
{
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
    constexpr int TIMEOUT = 100;
    ThreadState oldState = GetState();
    UpdateState(ThreadState::IS_SUSPENDED);
    {
        ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SuspendTime::WaitSuspension");
        LockHolder lock(suspendLock_);
        while (suspendCount_ > 0) {
            suspendCondVar_.TimedWait(&suspendLock_, TIMEOUT);
            // we need to do smth if Runtime is terminating at this point
        }
        ASSERT(!HasSuspendRequest());
    }
    UpdateState(oldState);
}

void JSThread::ManagedCodeBegin()
{
    ASSERT(!IsInManagedState());
    UpdateState(ThreadState::RUNNING);
}

void JSThread::ManagedCodeEnd()
{
    ASSERT(IsInManagedState());
    UpdateState(ThreadState::NATIVE);
}

void JSThread::TransferFromRunningToSuspended(ThreadState newState)
{
    ASSERT(currentThread == this);
    StoreSuspendedState(newState);
    CheckAndPassActiveBarrier();
}

void JSThread::TransferToRunning()
{
    ASSERT(!IsDaemonThread());
    ASSERT(currentThread == this);
    StoreRunningState(ThreadState::RUNNING);
    // Invoke free weak global callback when thread switch to running
    if (!weakNodeFreeGlobalCallbacks_.empty()) {
        InvokeWeakNodeFreeGlobalCallBack();
    }
    if (!vm_->GetSharedNativePointerCallbacks().empty()) {
        InvokeSharedNativePointerCallbacks();
    }
    if (fullMarkRequest_) {
        fullMarkRequest_ = const_cast<Heap*>(vm_->GetHeap())->TryTriggerFullMarkBySharedLimit();
    }
}

void JSThread::TransferDaemonThreadToRunning()
{
    ASSERT(IsDaemonThread());
    ASSERT(currentThread == this);
    StoreRunningState(ThreadState::RUNNING);
}

inline void JSThread::StoreState(ThreadState newState)
{
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

void JSThread::StoreRunningState(ThreadState newState)
{
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
            ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SuspendTime::StoreRunningState");
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
    ASSERT(newState != ThreadState::RUNNING);
    StoreState(newState);
}

void JSThread::PostFork()
{
    SetThreadId();
    if (currentThread == nullptr) {
        currentThread = this;
        ASSERT(GetState() == ThreadState::CREATED);
        UpdateState(ThreadState::NATIVE);
    } else {
        // We tried to call fork in the same thread
        ASSERT(currentThread == this);
        ASSERT(GetState() == ThreadState::NATIVE);
    }
}
#ifndef NDEBUG
bool JSThread::IsInManagedState() const
{
    ASSERT(this == JSThread::GetCurrent());
    return GetState() == ThreadState::RUNNING;
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
}  // namespace panda::ecmascript
