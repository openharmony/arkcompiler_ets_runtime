/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/unified_gc/unified_gc_marker.h"

namespace panda::ecmascript {

CrossVMOperator::CrossVMOperator(EcmaVM *vm)
{
    ecmaVMInterface_ = std::make_unique<EcmaVMInterfaceImpl>(vm);
}

/*static*/
void CrossVMOperator::DoHandshake(EcmaVM *vm, void *stsIface, void **ecmaIface)
{
    auto vmOperator = vm->GetCrossVMOperator();
    *ecmaIface = vmOperator->ecmaVMInterface_.get();
    vmOperator->stsVMInterface_ = reinterpret_cast<arkplatform::STSVMInterface *>(stsIface);
    auto heap = vm->GetHeap();
    heap->GetUnifiedGCMarker()->SetSTSVMInterface(vmOperator->stsVMInterface_);
}

void CrossVMOperator::EcmaVMInterfaceImpl::MarkFromObject(void *objAddress)
{
    ASSERT(objAddress != nullptr);
    JSTaggedType object = *(reinterpret_cast<JSTaggedType *>(objAddress));
    JSTaggedValue value(object);
    if (value.IsHeapObject()) {
        vm_->GetHeap()->GetUnifiedGCMarker()->MarkFromObject(value.GetHeapObject());
    }
}

bool CrossVMOperator::EcmaVMInterfaceImpl::StartXRefMarking()
{
    return vm_->GetHeap()->TriggerUnifiedGCMark<TriggerGCType::UNIFIED_GC, GCReason::CROSSREF_CAUSE>();
}

}  // namespace panda::ecmascript