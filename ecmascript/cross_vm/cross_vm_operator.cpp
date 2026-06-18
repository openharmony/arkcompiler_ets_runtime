/**
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/cross_vm/cross_vm_operator.h"

#include "ecmascript/cross_vm/unified_gc/unified_gc_marker.h"
#include "ecmascript/cross_vm/heap_hybrid-inl.h"
#include "ecmascript/cross_vm/unified_gc/unified_gc.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/js_file_path.h"
#include "ecmascript/js_function.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/mem/heap-inl.h"

namespace panda::ecmascript {

CrossVMOperator::CrossVMOperator(EcmaVM *vm) : vm_(vm)
{
    ecmaVMInterface_ = std::make_unique<EcmaVMInterfaceImpl>(vm);
}

/*static*/
void CrossVMOperator::DoHandshake(EcmaVM *vm, void *stsIface, void **ecmaIface)
{
    if (g_isEnableCMCGC) {
        auto vmOperator = vm->GetCrossVMOperator();
        *ecmaIface = vmOperator->ecmaVMInterface_.get();
        vmOperator->stsVMInterface_ = static_cast<arkplatform::STSVMInterface *>(stsIface);
        Runtime::GetInstance()->SetSTSVMInterface(vmOperator->stsVMInterface_);
        return;
    }
    auto vmOperator = vm->GetCrossVMOperator();
    *ecmaIface = vmOperator->ecmaVMInterface_.get();
    vmOperator->stsVMInterface_ = static_cast<arkplatform::STSVMInterface *>(stsIface);
    Runtime::GetInstance()->SetSTSVMInterface(vmOperator->stsVMInterface_);
    UnifiedGC *unifiedGC = SharedHeap::GetInstance()->GetUnifiedGC();
    if (unifiedGC->GetSTSVMInterface() == nullptr) {
        unifiedGC->SetSTSVMInterface(vmOperator->stsVMInterface_);
    }
}

void CrossVMOperator::MarkFromObject(JSTaggedType value, std::function<void(uintptr_t)> &visitor)
{
    JSTaggedValue taggedValue(value);
    if (!taggedValue.IsHeapObject()) {
        return;
    }
    TaggedObject *object = taggedValue.GetHeapObject();
    visitor(reinterpret_cast<uintptr_t>(object));  // This is only mark, so could pass a stack reference
}

void CrossVMOperator::MarkFromObject(JSTaggedType value)
{
    JSTaggedValue taggedValue(value);
    if (!taggedValue.IsHeapObject()) {
        return;
    }
    TaggedObject *object = taggedValue.GetHeapObject();
    auto heap = vm_->GetHeap();
    heap->GetUnifiedGCMarker()->MarkFromObject(object);
}

bool CrossVMOperator::IsObjectAlive(JSTaggedType value)
{
    JSTaggedValue taggedValue(value);
    if (!taggedValue.IsHeapObject()) {
        return false;
    }
    TaggedObject *object = taggedValue.GetHeapObject();
    return vm_->GetHeap()->IsAlive(object);
}

bool CrossVMOperator::IsValidHeapObject(JSTaggedType value)
{
    JSTaggedValue taggedValue(value);
    if (!taggedValue.IsHeapObject()) {
        return false;
    }
    TaggedObject *object = taggedValue.GetHeapObject();
    if (g_isEnableCMCGC) {
        return static_cast<BaseObject *>(object)->IsValidObject();
    }
    return vm_->GetHeap()->ContainObject(object);
}

bool CrossVMOperator::UnionStackIsEmpty(bool *isEmpty) const
{
    if (stsVMInterface_ == nullptr) {
        return false;
    }
    return stsVMInterface_->UnionStackIsEmpty(isEmpty);
}

bool CrossVMOperator::ForEachFrameInUnionStack(
    const std::function<void(const void *frame, bool isStaticFrame)> &callback) const
{
    if (stsVMInterface_ == nullptr) {
        return false;
    }
    return stsVMInterface_->ForEachFrameInUnionStack(callback);
}

bool CrossVMOperator::GetStaticFrameInfo(const void *frame, arkplatform::HybridFrameInfo &frameInfo) const
{
    if (stsVMInterface_ == nullptr) {
        return false;
    }
    return stsVMInterface_->GetStaticFrameInfo(frame, frameInfo);
}

bool CrossVMOperator::EcmaVMInterfaceImpl::StartXRefMarking()
{
    return SharedHeap::GetInstance()->TriggerUnifiedGCMark<TriggerGCType::UNIFIED_GC, GCReason::CROSSREF_CAUSE>(
        vm_->GetJSThread());
}

void CrossVMOperator::EcmaVMInterfaceImpl::NotifyXGCInterruption()
{
    UnifiedGC *unifiedGC = SharedHeap::GetInstance()->GetUnifiedGC();
    unifiedGC->SetInterruptUnifiedGC(true);
}

void FillFunctionName(JSThread *jsThread, FrameHandler *frameHandler, arkplatform::HybridFrameInfo &frameInfo)
{
    JSTaggedValue function = frameHandler->GetFunction();
    if (!function.IsJSFunction()) {
        return;
    }
    auto *jsFunction = JSFunction::Cast(function.GetTaggedObject());
    JSHandle<JSFunctionBase> funcHandle(jsThread, jsFunction);
    JSHandle<JSTaggedValue> funcName = JSFunctionBase::GetFunctionName(jsThread, funcHandle);
    if (funcName->IsString()) {
        EcmaString *str = EcmaString::Cast(funcName->GetTaggedObject());
        frameInfo.SetFunctionName(EcmaStringAccessor(str).ToCString(jsThread));
        return;
    }
    frameInfo.SetFunctionName("anonymous");
}

void FillModuleAndPosition(JSThread *jsThread, FrameHandler *frameHandler, arkplatform::HybridFrameInfo &frameInfo)
{
    auto *method = frameHandler->GetMethod();
    if (method == nullptr) {
        return;
    }
    const JSPandaFile *jsPandaFile = method->GetJSPandaFile(jsThread);
    if (jsPandaFile == nullptr) {
        return;
    }

    frameInfo.SetModuleName(method->GetRecordNameStr(jsThread));

    DebugInfoExtractor *debugExtractor = JSPandaFileManager::GetInstance()->GetJSPtExtractor(jsPandaFile);
    MethodLiteral *methodLiteral = method->GetMethodLiteral(jsThread);
    if (debugExtractor == nullptr || methodLiteral == nullptr) {
        return;
    }
    uint32_t bytecodeOffset = frameHandler->GetBytecodeOffset();
    frameInfo.bcOffset = bytecodeOffset;

    panda_file::File::EntityId methodId = methodLiteral->GetMethodId();
    const std::string &sourceFile = debugExtractor->GetSourceFile(methodId);
    frameInfo.SetUrl(sourceFile);
    frameInfo.fileId = methodId.GetOffset();
    int lineNumber = -1;
    auto callbackLineFunc = [&lineNumber](int32_t line) -> bool {
        lineNumber = line + 1;
        return true;
    };
    if (debugExtractor->MatchLineWithOffset(callbackLineFunc, methodId, bytecodeOffset)) {
        frameInfo.lineNumber = lineNumber;
    }
    int columnNumber = -1;
    auto callbackColumnFunc = [&columnNumber](int32_t column) -> bool {
        columnNumber = column + 1;
        return true;
    };
    if (debugExtractor->MatchColumnWithOffset(callbackColumnFunc, methodId, bytecodeOffset)) {
        frameInfo.columnNumber = columnNumber;
    }
}

bool CrossVMOperator::EcmaVMInterfaceImpl::GetDynamicFrameInfo(const void *dynamicFramePtr,
                                                               arkplatform::HybridFrameInfo &frameInfo)
{
    JSThread *jsThread = vm_->GetJSThread();
    if (jsThread == nullptr || dynamicFramePtr == nullptr) {
        return false;
    }

    panda::ecmascript::ThreadManagedScope managedScope(jsThread);
    auto *frameHandler = static_cast<panda::ecmascript::FrameHandler *>(const_cast<void *>(dynamicFramePtr));

    frameInfo.isStaticFrame = false;
    frameInfo.nativePtr = const_cast<void *>(dynamicFramePtr);
    FillFunctionName(jsThread, frameHandler, frameInfo);
    frameInfo.SetModuleName("javascript");
    frameInfo.lineNumber = -1;
    frameInfo.columnNumber = -1;
    FillModuleAndPosition(jsThread, frameHandler, frameInfo);
    return true;
}

JSTaggedType *CrossVMOperator::EcmaVMInterfaceImpl::GetTopFrameSPFromDynamic() const
{
    JSThread *jsThread = vm_->GetJSThread();
    FrameHandler frameHandler(jsThread);
    return frameHandler.GetSp();
}

bool CrossVMOperator::EcmaVMInterfaceImpl::ForEachDynamicFrame(void *currFrameSP, void *toFrameSP,
                                                               const std::function<void(const void *)> &cb) const
{
    auto *jsThread = vm_->GetJSThread();
    if (jsThread == nullptr) {
        return false;
    }
    ASSERT(currFrameSP != nullptr);
    FrameHandler currHandle(reinterpret_cast<JSTaggedType *>(currFrameSP), jsThread);
    SuspendAllScope suspendAllScope(jsThread);
    for (; currHandle.HasFrame(); currHandle.PrevJSFrame()) {
        if (toFrameSP != nullptr && currHandle.GetSp() == toFrameSP) {
            break;
        }
        if (currHandle.IsEntryFrame() || currHandle.IsBuiltinFrame()) {
            continue;
        }
        cb(static_cast<void *>(&currHandle));
    }
    return true;
}

const void *CrossVMOperator::EcmaVMInterfaceImpl::GetEcmaVM() const
{
    return static_cast<const void *>(vm_);
}
}  // namespace panda::ecmascript