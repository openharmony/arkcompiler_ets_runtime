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

#include "ecmascript/js_thread.h"

#include "ecmascript/runtime.h"
#include "ecmascript/builtin_entries.h"
#include "ecmascript/enum_conversion.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/ts_types/builtin_type_id.h"

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
#include "ecmascript/ecma_param_configuration.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/ic/properties_cache.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/mark_word.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/stackmap/llvm/llvm_stackmap_parser.h"
#include "ecmascript/builtin_entries.h"
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

    RegisterThread(jsThread);
    return jsThread;
}

JSThread::JSThread(EcmaVM *vm) : id_(os::thread::GetCurrentThreadId()), vm_(vm)
{
    auto chunk = vm->GetChunk();
    if (!vm_->GetJSOptions().EnableGlobalLeakCheck()) {
        globalStorage_ = chunk->New<EcmaGlobalStorage<Node>>(this, vm->GetNativeAreaAllocator());
        newGlobalHandle_ = std::bind(&EcmaGlobalStorage<Node>::NewGlobalHandle, globalStorage_, std::placeholders::_1);
        disposeGlobalHandle_ = std::bind(&EcmaGlobalStorage<Node>::DisposeGlobalHandle, globalStorage_,
            std::placeholders::_1);
        setWeak_ = std::bind(&EcmaGlobalStorage<Node>::SetWeak, globalStorage_, std::placeholders::_1,
            std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
        clearWeak_ = std::bind(&EcmaGlobalStorage<Node>::ClearWeak, globalStorage_, std::placeholders::_1);
        isWeak_ = std::bind(&EcmaGlobalStorage<Node>::IsWeak, globalStorage_, std::placeholders::_1);
    } else {
        globalDebugStorage_ =
            chunk->New<EcmaGlobalStorage<DebugNode>>(this, vm->GetNativeAreaAllocator());
        newGlobalHandle_ = std::bind(&EcmaGlobalStorage<DebugNode>::NewGlobalHandle, globalDebugStorage_,
            std::placeholders::_1);
        disposeGlobalHandle_ = std::bind(&EcmaGlobalStorage<DebugNode>::DisposeGlobalHandle, globalDebugStorage_,
            std::placeholders::_1);
        setWeak_ = std::bind(&EcmaGlobalStorage<DebugNode>::SetWeak, globalDebugStorage_, std::placeholders::_1,
            std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
        clearWeak_ = std::bind(&EcmaGlobalStorage<DebugNode>::ClearWeak, globalDebugStorage_, std::placeholders::_1);
        isWeak_ = std::bind(&EcmaGlobalStorage<DebugNode>::IsWeak, globalDebugStorage_, std::placeholders::_1);
    }
    vmThreadControl_ = new VmThreadControl(this);
    SetBCStubStatus(BCStubStatus::NORMAL_BC_STUB);
}

JSThread::JSThread(EcmaVM *vm, bool isJit) : id_(os::thread::GetCurrentThreadId()), vm_(vm), isJitThread_(isJit)
{
    ASSERT(isJit);
    RegisterThread(this);
};

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
    UnregisterThread(this);
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

JSTaggedValue JSThread::GetCurrentLexenv() const
{
    FrameHandler frameHandler(this);
    return frameHandler.GetEnv();
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
        (*callback)(callbackPair.second.first, callbackPair.second.second);
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
    return result || vm_->GetHeapProfile() != nullptr;
#else
    return result;
#endif
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

bool JSThread::DoStackOverflowCheck(const JSTaggedType *sp)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (UNLIKELY(sp <= glueData_.frameBase_ + RESERVE_STACK_SIZE)) {
        vm_->CheckThread();
        LOG_ECMA(ERROR) << "Stack overflow! Remaining stack size is: " << (sp - glueData_.frameBase_);
        if (!IsCrossThreadExecutionEnable() && LIKELY(!HasPendingException())) {
            ObjectFactory *factory = GetEcmaVM()->GetFactory();
            JSHandle<JSObject> error = factory->GetJSError(base::ErrorType::RANGE_ERROR, "Stack overflow!", false);
            SetException(error.GetTaggedValue());
        }
        return true;
    }
    return false;
}

bool JSThread::DoStackLimitCheck()
{
    if (UNLIKELY(GetCurrentStackPosition() < GetStackLimit())) {
        vm_->CheckThread();
        LOG_ECMA(ERROR) << "Stack overflow! current:" << GetCurrentStackPosition() << " limit:" << GetStackLimit();
        if (!IsCrossThreadExecutionEnable() && LIKELY(!HasPendingException())) {
            ObjectFactory *factory = GetEcmaVM()->GetFactory();
            JSHandle<JSObject> error = factory->GetJSError(base::ErrorType::RANGE_ERROR, "Stack overflow!", false);
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

JSHClass *JSThread::GetArrayInstanceHClass(ElementsKind kind) const
{
    auto iter = GetArrayHClassIndexMap().find(kind);
    ASSERT(iter != GetArrayHClassIndexMap().end());
    auto index = static_cast<size_t>(iter->second);
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
    if (isDebug &&
        glueData_.bcDebuggerStubEntries_.Get(0) == glueData_.bcDebuggerStubEntries_.Get(1)) {
        for (size_t i = 0; i < BCStubEntries::BC_HANDLER_COUNT; i++) {
            auto stubEntry = glueData_.bcStubEntries_.Get(i);
            auto debuggerStubEbtry = glueData_.bcDebuggerStubEntries_.Get(i);
            glueData_.bcDebuggerStubEntries_.Set(i, stubEntry);
            glueData_.bcStubEntries_.Set(i, debuggerStubEbtry);
        }
    } else if (!isDebug &&
        glueData_.bcStubEntries_.Get(0) == glueData_.bcStubEntries_.Get(1)) {
        for (size_t i = 0; i < BCStubEntries::BC_HANDLER_COUNT; i++) {
            auto stubEntry = glueData_.bcDebuggerStubEntries_.Get(i);
            auto debuggerStubEbtry = glueData_.bcStubEntries_.Get(i);
            glueData_.bcStubEntries_.Set(i, stubEntry);
            glueData_.bcDebuggerStubEntries_.Set(i, debuggerStubEbtry);
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

void JSThread::SwitchJitProfileStubs()
{
    // if jit enable pgo, use pgo stub
    if (GetEcmaVM()->GetJSOptions().IsEnableJITPGO()) {
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
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::TERMINATION_ERROR, "Terminate execution!");
    SetException(error.GetTaggedValue());
}

bool JSThread::CheckSafepoint()
{
    ResetCheckSafePointStatus();

    if (HasTerminationRequest()) {
        TerminateExecution();
        SetVMTerminated(true);
        SetTerminationRequest(false);
    }

    if (IsSuspended()) {
        WaitSuspension();
    }

    // vmThreadControl_ 's thread_ is current JSThread's this.
    if (VMNeedSuspension()) {
        vmThreadControl_->SuspendVM();
    }
    if (HasInstallMachineCode()) {
        vm_->GetJit()->InstallTasks(GetThreadId());
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

    if (IsMarkFinished() && heap->GetConcurrentMarker()->IsTriggeredConcurrentMark()
        && !heap->GetOnSerializeEvent()) {
        heap->GetConcurrentMarker()->HandleMarkingFinished();
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
    pthread_attr_t attr;
    int ret = pthread_getattr_np(pthread_self(), &attr);
    if (ret != 0) {
        LOG_ECMA(ERROR) << "Get current thread attr failed";
        return result;
    }

    void *stackAddr = nullptr;
    size_t size = 0;
    ret = pthread_attr_getstack(&attr, &stackAddr, &size);
    if (pthread_attr_destroy(&attr) != 0) {
        LOG_ECMA(ERROR) << "Destroy current thread attr failed";
    }
    if (ret != 0) {
        LOG_ECMA(ERROR) << "Get current thread stack size failed";
        return result;
    }

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
    uint64_t top = GetStackStart();
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
        SetGlobalObject(currentContext->GetGlobalEnv()->GetGlobalObject());
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
        StoreState(newState, false);
    }
}

void JSThread::SuspendThread(bool internalSuspend)
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
        LockHolder lock(suspendLock_);
        while (suspendCount_ > 0) {
            suspendCondVar_.TimedWait(&suspendLock_, TIMEOUT);
            // we need to do smth if Runtime is terminating at this point
        }
        ASSERT(!IsSuspended());
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
    StoreState(newState, false);
    ASSERT(Runtime::GetInstance()->GetMutatorLock()->HasLock());
    Runtime::GetInstance()->GetMutatorLock()->Unlock();
}

void JSThread::TransferToRunning()
{
    ASSERT(currentThread == this);
    StoreState(ThreadState::RUNNING, true);
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

void JSThread::StoreState(ThreadState newState, bool lockMutatorLock)
{
    while (true) {
        ThreadStateAndFlags oldStateAndFlags;
        oldStateAndFlags.asInt = glueData_.stateAndFlags_.asInt;
        if (lockMutatorLock && oldStateAndFlags.asStruct.flags != ThreadFlag::NO_FLAGS) {
            WaitSuspension();
            continue;
        }
        ThreadStateAndFlags newStateAndFlags;
        newStateAndFlags.asStruct.flags = oldStateAndFlags.asStruct.flags;
        newStateAndFlags.asStruct.state = newState;

        if (lockMutatorLock) {
            Runtime::GetInstance()->GetMutatorLock()->ReadLock();
        }

        if (glueData_.stateAndFlags_.asAtomicInt.compare_exchange_weak(oldStateAndFlags.asNonvolatileInt,
            newStateAndFlags.asNonvolatileInt, std::memory_order_release)) {
            break;
        }

        // CAS failed. Unlock mutator lock
        if (lockMutatorLock) {
            ASSERT(Runtime::GetInstance()->GetMutatorLock()->HasLock());
            Runtime::GetInstance()->GetMutatorLock()->Unlock();
        }
    }
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
    return GetMutatorLockState() == MutatorLock::MutatorLockState::RDLOCK && GetState() == ThreadState::RUNNING;
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
