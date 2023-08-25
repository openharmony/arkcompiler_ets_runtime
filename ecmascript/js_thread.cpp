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

#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS) && !defined(PANDA_TARGET_IOS)
#include <sys/resource.h>
#endif

#if defined(ENABLE_EXCEPTION_BACKTRACE)
#include "ecmascript/platform/backtrace.h"
#endif
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#endif
#include "ecmascript/ecma_global_storage.h"
#include "ecmascript/ecma_param_configuration.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/ic/properties_cache.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/mark_word.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/stackmap/llvm_stackmap_parser.h"

namespace panda::ecmascript {
using CommonStubCSigns = panda::ecmascript::kungfu::CommonStubCSigns;
using BytecodeStubCSigns = panda::ecmascript::kungfu::BytecodeStubCSigns;

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

    if (jsThread->IsAsmInterpreter()) {
        jsThread->glueData_.stackLimit_ = GetAsmStackLimit();
        jsThread->glueData_.stackStart_ = GetCurrentStackPosition();
    }
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

JSThread::~JSThread()
{
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

    glueData_.frameBase_ = nullptr;
    nativeAreaAllocator_ = nullptr;
    heapRegionAllocator_ = nullptr;
    if (vmThreadControl_ != nullptr) {
        delete vmThreadControl_;
        vmThreadControl_ = nullptr;
    }
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
        globalStorage_->IterateUsageGlobal([visitor](Node *node) {
            JSTaggedValue value(node->GetObject());
            if (value.IsHeapObject()) {
                visitor(ecmascript::Root::ROOT_HANDLE, ecmascript::ObjectSlot(node->GetObjectAddress()));
            }
        });
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

void JSThread::IterateWeakEcmaGlobalStorage(const WeakRootVisitor &visitor)
{
    auto callBack = [this, visitor](WeakNode *node) {
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
            if (!node->CallFreeGlobalCallback()) {
                DisposeGlobalHandle(ToUintPtr(node));
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
        LOG_ECMA(ERROR) << "Stack overflow! Remaining stack size is: " << (sp - glueData_.frameBase_);
        if (LIKELY(!HasPendingException())) {
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
#define SWITCH_PGO_STUB_ENTRY(fromName, toName)                                                             \
        curAddress = GetBCStubEntry(BytecodeStubCSigns::ID_##fromName);                                     \
        SetBCStubEntry(BytecodeStubCSigns::ID_##fromName, GetBCStubEntry(BytecodeStubCSigns::ID_##toName)); \
        SetBCStubEntry(BytecodeStubCSigns::ID_##toName, curAddress);
        ASM_INTERPRETER_BC_PROFILER_STUB_LIST(SWITCH_PGO_STUB_ENTRY)
#undef SWITCH_PGO_STUB_ENTRY
    }
}

bool JSThread::CheckSafepoint()
{
    ResetCheckSafePointStatus();
    if (vmThreadControl_->VMNeedSuspension()) {
        vmThreadControl_->SuspendVM();
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
    if (IsMarkFinished() && GetEcmaVM()->GetHeap()->GetConcurrentMarker()->IsTriggeredConcurrentMark()) {
        auto heap = GetEcmaVM()->GetHeap();
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
        SetCurrentEcmaContext(context);
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
        if (IsAsmInterpreter()) {
            context->SetStackLimit(GetAsmStackLimit());
            context->SetStackStart(GetCurrentStackPosition());
        }
        EcmaInterpreter::InitStackFrame(context);
    }
}

void JSThread::PopContext()
{
    contexts_.pop_back();
    SwitchCurrentContext(contexts_.back());
}

void JSThread::SwitchCurrentContext(EcmaContext *currentContext, bool isInIterate)
{
    ASSERT(std::count(contexts_.begin(), contexts_.end(), currentContext));
    // If glueData_.currentContext_ == currentContext, don't need switch context
    if (glueData_.currentContext_ == currentContext) {
        return;
    }
    glueData_.currentContext_->SetFramePointers(const_cast<JSTaggedType *>(GetCurrentSPFrame()),
        const_cast<JSTaggedType *>(GetLastLeaveFrame()),
        const_cast<JSTaggedType *>(GetLastFp()));
    glueData_.currentContext_->SetFrameBase(glueData_.frameBase_);
    glueData_.currentContext_->SetGlobalEnv(GetGlueGlobalEnv());
    glueData_.currentContext_->GetGlobalEnv()->SetJSGlobalObject(this, glueData_.globalObject_);

    SetCurrentSPFrame(currentContext->GetCurrentFrame());
    SetLastLeaveFrame(currentContext->GetLeaveFrame());
    SetLastFp(currentContext->GetLastFp());
    glueData_.frameBase_ = currentContext->GetFrameBase();
    if (HasCheckSafePoint()) {
        // If CheckSafePointBit is true, stackLimit_ need to be stackStart_ like SetCheckSafePointStatus
        glueData_.stackLimit_ = currentContext->GetStackStart();
    } else {
        glueData_.stackLimit_ = currentContext->GetStackLimit();
    }
    glueData_.stackStart_ = currentContext->GetStackStart();
    if (!currentContext->GlobalEnvIsHole()) {
        SetGlueGlobalEnv(*(currentContext->GetGlobalEnv()));
        SetGlobalObject(currentContext->GetGlobalEnv()->GetGlobalObject());
    }
    if (!isInIterate) {
        // If isInIterate is true, it means it is in GC iterate and global variables are no need to change.
        glueData_.globalConst_ = const_cast<GlobalEnvConstants *>(currentContext->GlobalConstants());
    }

    SetCurrentEcmaContext(currentContext);
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
    for (auto item : contexts_) {
        if (!item->IsInitialized()) {
            return false;
        }
    }
    return true;
}

void JSThread::ResetCheckSafePointStatus()
{
    ASSERT(static_cast<uint8_t>(glueData_.interruptVector_ & CHECKSAFEPOINT_FLAG) <= 1);
    CheckSafePointBit::Set(false, &glueData_.interruptVector_);
    ASSERT(glueData_.currentContext_ != nullptr);
    SetStackLimit(glueData_.currentContext_->GetStackLimit());
}

void JSThread::SetCheckSafePointStatus()
{
    ASSERT(static_cast<uint8_t>(glueData_.interruptVector_ & CHECKSAFEPOINT_FLAG) <= 1);
    CheckSafePointBit::Set(true, &glueData_.interruptVector_);
    SetStackLimit(GetStackStart());
}
}  // namespace panda::ecmascript
