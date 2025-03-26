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

#ifndef ECMASCRIPT_MEM_UNIFIED_GC_UNIFIED_GC_H
#define ECMASCRIPT_MEM_UNIFIED_GC_UNIFIED_GC_H

#include <atomic>

#ifdef PANDA_JS_ETS_HYBRID_MODE
#include "ecmascript/cross_vm/cross_vm_operator.h"
#endif // PANDA_JS_ETS_HYBRID_MODE

namespace panda::ecmascript {
class UnifiedGC {
public:
    explicit UnifiedGC() {}
    ~UnifiedGC() = default;

    void RunPhases();
    void Initialize();
    void Mark();
    void Finish();

#ifdef PANDA_JS_ETS_HYBRID_MODE
    void SetSTSVMInterface(arkplatform::STSVMInterface *stsIface)
    {
        stsVMInterface_ = stsIface;
    }

    arkplatform::STSVMInterface *GetSTSVMInterface() const
    {
        return stsVMInterface_;
    }

    void SetInterruptUnifiedGC(bool isInterruptUnifiedGC)
    {
        isInterruptUnifiedGC_.store(isInterruptUnifiedGC, std::memory_order_relaxed);
    }

    bool StartXGCBarrier()
    {
        static const auto StartXGCCheck = []() -> bool {
            return !isInterruptUnifiedGC_.load(std::memory_order_acquire);
        };
        return stsVMInterface_->StartXGCBarrier(StartXGCCheck);
    }

    void FinishXGCBarrier()
    {
        stsVMInterface_->FinishXGCBarrier();
    }
#endif // PANDA_JS_ETS_HYBRID_MODE

private:
#ifdef PANDA_JS_ETS_HYBRID_MODE
    arkplatform::STSVMInterface *stsVMInterface_ {nullptr};
    static std::atomic<bool> isInterruptUnifiedGC_;
#endif // PANDA_JS_ETS_HYBRID_MODE
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_UNIFIED_GC_UNIFIED_GC_H