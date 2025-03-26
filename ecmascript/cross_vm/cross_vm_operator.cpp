/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "ecmascript/cross_vm/cross_vm_operator.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/unified_gc/unified_gc.h"
#include "ecmascript/mem/unified_gc/unified_gc_marker.h"

namespace panda::ecmascript {

CrossVMOperator::CrossVMOperator(EcmaVM *vm) : vm_(vm)
{
    ecmaVMInterface_ = std::make_unique<EcmaVMInterfaceImpl>(vm);
}

/*static*/
void CrossVMOperator::DoHandshake(EcmaVM *vm, void *stsIface, void **ecmaIface)
{
    auto vmOperator = vm->GetCrossVMOperator();
    *ecmaIface = vmOperator->ecmaVMInterface_.get();
    vmOperator->stsVMInterface_ = static_cast<arkplatform::STSVMInterface *>(stsIface);
    UnifiedGC *unifiedGC = SharedHeap::GetInstance()->GetUnifiedGC();
    if (unifiedGC->GetSTSVMInterface() == nullptr) {
        unifiedGC->SetSTSVMInterface(vmOperator->stsVMInterface_);
    }
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
    return vm_->GetHeap()->ContainObject(object);
}

bool CrossVMOperator::EcmaVMInterfaceImpl::StartXRefMarking()
{
    return SharedHeap::GetInstance()->TriggerUnifiedGCMark<
        TriggerGCType::UNIFIED_GC, GCReason::CROSSREF_CAUSE>(vm_->GetJSThread());
}

void CrossVMOperator::EcmaVMInterfaceImpl::NotifyXGCInterruption()
{
    UnifiedGC *unifiedGC = SharedHeap::GetInstance()->GetUnifiedGC();
    unifiedGC->SetInterruptUnifiedGC(true);
}

}  // namespace panda::ecmascript