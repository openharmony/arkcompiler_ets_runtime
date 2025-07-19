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
#ifndef ECMASCRIPT_CROSS_VM_ECMA_VM_HYBRID_H
#define ECMASCRIPT_CROSS_VM_ECMA_VM_HYBRID_H

#define  ECMAVM_PUBLIC_HYBRID_MODE_EXTENSION()          \
    CrossVMOperator* GetCrossVMOperator() const         \
    {                                                   \
        return crossVMOperator_;                        \
    }

#define ECMAVM_PUBLIC_HYBRID_EXTENSION()                 \
    void CreateHybridParam()

#define ECMAVM_PROTECTED_HYBRID_EXTENSION()                                     \
    void PrintJSErrorInfo(const JSHandle<JSTaggedValue> &exceptionInfo) const

#define ECMAVM_PRIVATE_HYBRID_EXTENSION()                \
    CrossVMOperator* crossVMOperator_ {nullptr}
#endif  // ECMASCRIPT_CROSS_VM_ECMA_VM_HYBRID_H
