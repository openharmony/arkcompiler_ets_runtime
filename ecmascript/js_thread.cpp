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

#include "ecmascript/js_thread.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/platform/file.h"
#if defined(ENABLE_EXCEPTION_BACKTRACE)
#include "ecmascript/platform/backtrace.h"
#endif
#include "ecmascript/ecma_global_storage.h"
#include "ecmascript/ecma_param_configuration.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/ic/properties_cache.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/mem/mark_word.h"
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
    propertiesCache_ = new PropertiesCache();
    vmThreadControl_ = new VmThreadControl();
}

JSThread::~JSThread()
{
    for (auto n : handleStorageNodes_) {
        delete n;
    }
    handleStorageNodes_.clear();
    currentHandleStorageIndex_ = -1;
    handleScopeCount_ = 0;
    handleScopeStorageNext_ = handleScopeStorageEnd_ = nullptr;
    if (globalStorage_ != nullptr) {
        GetEcmaVM()->GetChunk()->Delete(globalStorage_);
        globalStorage_ = nullptr;
    }
    if (globalDebugStorage_) {
        GetEcmaVM()->GetChunk()->Delete(globalDebugStorage_);
        globalDebugStorage_ = nullptr;
    }

    GetNativeAreaAllocator()->Free(glueData_.frameBase_, sizeof(JSTaggedType) *
        vm_->GetEcmaParamConfiguration().GetMaxStackSize());
    glueData_.frameBase_ = nullptr;
    nativeAreaAllocator_ = nullptr;
    heapRegionAllocator_ = nullptr;
    if (propertiesCache_ != nullptr) {
        delete propertiesCache_;
        propertiesCache_ = nullptr;
    }
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
    if (propertiesCache_ != nullptr) {
        propertiesCache_->Clear();
    }

    if (!glueData_.exception_.IsHole()) {
        visitor(Root::ROOT_VM, ObjectSlot(ToUintPtr(&glueData_.exception_)));
    }
    // visit global Constant
    glueData_.globalConst_.VisitRangeSlot(rangeVisitor);
    // visit stack roots
    FrameHandler frameHandler(this);
    frameHandler.Iterate(visitor, rangeVisitor, derivedVisitor);
    // visit tagged handle storage roots
    if (vm_->GetJSOptions().EnableGlobalLeakCheck()) {
        IterateHandleWithCheck(visitor, rangeVisitor);
    } else {
        if (currentHandleStorageIndex_ != -1) {
            int32_t nid = currentHandleStorageIndex_;
            for (int32_t i = 0; i <= nid; ++i) {
                auto node = handleStorageNodes_.at(i);
                auto start = node->data();
                auto end = (i != nid) ? &(node->data()[NODE_BLOCK_SIZE]) : handleScopeStorageNext_;
                rangeVisitor(ecmascript::Root::ROOT_HANDLE, ObjectSlot(ToUintPtr(start)), ObjectSlot(ToUintPtr(end)));
            }
        }

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
    if (currentHandleStorageIndex_ != -1) {
        int32_t nid = currentHandleStorageIndex_;
        for (int32_t i = 0; i <= nid; ++i) {
            auto node = handleStorageNodes_.at(i);
            auto start = node->data();
            auto end = (i != nid) ? &(node->data()[NODE_BLOCK_SIZE]) : handleScopeStorageNext_;
            rangeVisitor(ecmascript::Root::ROOT_HANDLE, ObjectSlot(ToUintPtr(start)), ObjectSlot(ToUintPtr(end)));
            handleCount += (ToUintPtr(end) - ToUintPtr(start)) / sizeof(JSTaggedType);
        }
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
            auto secondPassCallback = node->GetSecondPassCallback();
            if (secondPassCallback) {
                weakNodeSecondPassCallbacks_.push_back(std::make_pair(secondPassCallback,
                                                                      node->GetReference()));
            }
            if (!node->CallFirstPassCallback()) {
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
    uintptr_t *result = nullptr;
    int32_t lastIndex = static_cast<int32_t>(handleStorageNodes_.size() - 1);
    if (currentHandleStorageIndex_ == lastIndex) {
        auto n = new std::array<JSTaggedType, NODE_BLOCK_SIZE>();
        handleStorageNodes_.push_back(n);
        currentHandleStorageIndex_++;
        result = reinterpret_cast<uintptr_t *>(&n->data()[0]);
        handleScopeStorageEnd_ = &n->data()[NODE_BLOCK_SIZE];
    } else {
        currentHandleStorageIndex_++;
        auto lastNode = handleStorageNodes_[currentHandleStorageIndex_];
        result = reinterpret_cast<uintptr_t *>(&lastNode->data()[0]);
        handleScopeStorageEnd_ = &lastNode->data()[NODE_BLOCK_SIZE];
    }

    return result;
}

void JSThread::ShrinkHandleStorage(int prevIndex)
{
    currentHandleStorageIndex_ = prevIndex;
    int32_t lastIndex = static_cast<int32_t>(handleStorageNodes_.size() - 1);
#if ECMASCRIPT_ENABLE_ZAP_MEM
    uintptr_t size = ToUintPtr(handleScopeStorageEnd_) - ToUintPtr(handleScopeStorageNext_);
    if (memset_s(handleScopeStorageNext_, size, 0, size) != EOK) {
        LOG_FULL(FATAL) << "memset_s failed";
        UNREACHABLE();
    }
    for (int32_t i = currentHandleStorageIndex_ + 1; i < lastIndex; i++) {
        if (memset_s(handleStorageNodes_[i],
                     NODE_BLOCK_SIZE * sizeof(JSTaggedType), 0,
                     NODE_BLOCK_SIZE * sizeof(JSTaggedType)) !=
                     EOK) {
            LOG_FULL(FATAL) << "memset_s failed";
            UNREACHABLE();
        }
    }
#endif

    if (lastIndex > MIN_HANDLE_STORAGE_SIZE && currentHandleStorageIndex_ < MIN_HANDLE_STORAGE_SIZE) {
        for (int i = MIN_HANDLE_STORAGE_SIZE; i < lastIndex; i++) {
            auto node = handleStorageNodes_.back();
            delete node;
            handleStorageNodes_.pop_back();
        }
    }
}

void JSThread::NotifyStableArrayElementsGuardians(JSHandle<JSObject> receiver)
{
    if (!receiver->GetJSHClass()->IsPrototype()) {
        return;
    }
    if (!stableArrayElementsGuardians_) {
        return;
    }
    auto env = GetEcmaVM()->GetGlobalEnv();
    if (receiver.GetTaggedValue() == env->GetObjectFunctionPrototype().GetTaggedValue() ||
        receiver.GetTaggedValue() == env->GetArrayPrototype().GetTaggedValue()) {
        SetStableArrayElementsGuardians(JSTaggedValue::False());
        stableArrayElementsGuardians_ = false;
    }
}

void JSThread::ResetGuardians()
{
    SetStableArrayElementsGuardians(JSTaggedValue::True());
    stableArrayElementsGuardians_ = true;
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

bool JSThread::CheckSafepoint() const
{
    if (vmThreadControl_->VMNeedSuspension()) {
        vmThreadControl_->SuspendVM();
    }
#ifndef NDEBUG
    if (vm_->GetJSOptions().EnableForceGC()) {
        GetEcmaVM()->CollectGarbage(TriggerGCType::FULL_GC);
        return true;
    }
#endif
    if (IsMarkFinished()) {
        auto heap = GetEcmaVM()->GetHeap();
        heap->GetConcurrentMarker()->HandleMarkingFinished();
        return true;
    }
    return false;
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

void JSThread::CollectBCOffsetInfo()
{
    FrameBcCollector collector(this);
    collector.CollectBCOffsetInfo();
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
    if (ret != 0) {
        LOG_ECMA(ERROR) << "Get current thread stack size failed";
        if (pthread_attr_destroy(&attr) != 0) {
            LOG_ECMA(ERROR) << "Destroy current thread attr failed";
        }
        return result;
    }

    uintptr_t threadStackLimit = reinterpret_cast<uintptr_t>(stackAddr);
    if (result < threadStackLimit) {
        result = threadStackLimit;
    }

    uintptr_t threadStackStart = threadStackLimit + size;
    LOG_INTERPRETER(INFO) << "Current thread stack start: " << reinterpret_cast<void *>(threadStackStart);
    LOG_INTERPRETER(INFO) << "Used stack before js stack start: "
                          << reinterpret_cast<void *>(threadStackStart - GetCurrentStackPosition());
    LOG_INTERPRETER(INFO) << "Current thread asm stack limit: " << reinterpret_cast<void *>(result);
    ret = pthread_attr_destroy(&attr);
    if (ret != 0) {
        LOG_ECMA(ERROR) << "Destroy current thread attr failed";
    }

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
}  // namespace panda::ecmascript
