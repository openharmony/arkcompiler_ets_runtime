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

#include "ecmascript/ecma_vm.h"

namespace panda::ecmascript {

CrossVMOperator::CrossVMOperator()
{
    ecmaVMInterface_ = std::make_unique<EcmaVMInterfaceImpl>();
}

/*static*/
void CrossVMOperator::DoHandshake(EcmaVM *vm, void *stsIface, void **ecmaIface)
{
    auto& vmOperator = vm->GetCrossVMOperator();
    *ecmaIface = vmOperator.ecmaVMInterface_.get();
    vmOperator.stsVMInterface_ = reinterpret_cast<arkplatform::STSVMInterface *>(stsIface);
}

}  // namespace panda::ecmascript