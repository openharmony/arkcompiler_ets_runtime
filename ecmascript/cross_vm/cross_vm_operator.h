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

#ifndef ETS_RUNTIME_ECMASCRIPT_CROSS_VM_CROSS_VM_OPERATOR_H
#define ETS_RUNTIME_ECMASCRIPT_CROSS_VM_CROSS_VM_OPERATOR_H

#include <memory>

#include "hybrid/ecma_vm_interface.h"
#include "hybrid/sts_vm_interface.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript {

using JSTaggedType = uint64_t;
class EcmaVM;

class CrossVMOperator {
    static_assert(PANDA_JS_ETS_HYBRID_MODE);
public:
    CrossVMOperator(EcmaVM *vm);
    ~CrossVMOperator() = default;
    NO_COPY_SEMANTIC(CrossVMOperator);
    NO_MOVE_SEMANTIC(CrossVMOperator);

    static void DoHandshake(EcmaVM *vm, void *stsIface, void **ecmaIface);

    arkplatform::STSVMInterface *GetSTSVMInterface() const
    {
        return stsVMInterface_;
    }

    arkplatform::EcmaVMInterface *GetEcmaVMInterface() const
    {
        return ecmaVMInterface_.get();
    }

    void MarkFromObject(JSTaggedType value);
    bool IsObjectAlive(JSTaggedType value);
    bool IsValidHeapObject(JSTaggedType value);
private:
    class EcmaVMInterfaceImpl final : public arkplatform::EcmaVMInterface {
    public:
        NO_COPY_SEMANTIC(EcmaVMInterfaceImpl);
        NO_MOVE_SEMANTIC(EcmaVMInterfaceImpl);
        EcmaVMInterfaceImpl(EcmaVM *vm): vm_(vm) {};
        ~EcmaVMInterfaceImpl() override = default;

        bool StartXRefMarking() override;
        void NotifyXGCInterruption() override;
    private:
        EcmaVM *vm_ {nullptr};
    };

    EcmaVM *vm_ {nullptr};
    arkplatform::STSVMInterface *stsVMInterface_ = nullptr;
    std::unique_ptr<arkplatform::EcmaVMInterface> ecmaVMInterface_;
};

} // namespace panda::ecmascript

#endif // ETS_RUNTIME_ECMASCRIPT_CROSS_VM_CROSS_VM_OPERATOR_H