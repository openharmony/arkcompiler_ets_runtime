/*
* Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ecmascript/cross_vm/ecma_vm_hybrid.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/runtime.h"


namespace panda::ecmascript {
void EcmaVM::CreateHybridParam()
{
    if (Runtime::GetInstance()->IsHybridVm()) {
#ifdef PANDA_JS_ETS_HYBRID_MODE
        crossVMOperator_ = new CrossVMOperator(this);
#endif
        JSHClass *hClass = JSHClass::Cast(thread_->GlobalConstants()->GetHClassClass().GetTaggedObject());
        JSHandle<JSHClass> ecmaHclass = factory_->NewEcmaHClass(hClass, JSObject::SIZE, JSType::JS_XREF_OBJECT);
        CacheToGlobalConstants(ecmaHclass.GetTaggedValue(), ConstantIndex::XREF_OBJECT_HCLASS_INDEX);

        heap_->CreateUnifiedGCMarker();
        SharedHeap::GetInstance()->CreateUnifiedGC();
        thread_->SetupXRefFields();
    }
}
}  // namespace panda::ecmascript
