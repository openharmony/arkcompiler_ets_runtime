/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_JIT_JIT_FORT_WRITE_SCOPE_H
#define ECMASCRIPT_JIT_JIT_FORT_WRITE_SCOPE_H

#include "ecmascript/jit/jit.h"
#include "ecmascript/log_wrapper.h"

#if defined(JIT_ENABLE_CODE_SIGN) && !defined(JIT_FORT_DISABLE) && defined(PANDA_TARGET_ARM64) && \
    defined(PANDA_TARGET_OHOS)
#include "ecmascript/mem/jit_fort.h"
#include "jit_fort_helper.h"
#endif

namespace panda::ecmascript {

// Opens the per-thread JitFort write window while installed machine code is patched.
class JitFortWriteScope {
public:
    JitFortWriteScope()
    {
#if defined(JIT_ENABLE_CODE_SIGN) && !defined(JIT_FORT_DISABLE) && defined(PANDA_TARGET_ARM64) && \
    defined(PANDA_TARGET_OHOS)
        if (Jit::GetInstance()->IsEnableJitFort() && !Jit::GetInstance()->IsDisableCodeSign() &&
            JitFort::IsResourceAvailable()) {
            opened_ = OHOS::Security::CodeSign::PrctlWrapper(
                JITFORT_PRCTL_OPTION, JITFORT_SWITCH_IN, 0) == 0;
            closeRequired_ = opened_;
        }
#endif
    }

    ~JitFortWriteScope()
    {
#if defined(JIT_ENABLE_CODE_SIGN) && !defined(JIT_FORT_DISABLE) && defined(PANDA_TARGET_ARM64) && \
    defined(PANDA_TARGET_OHOS)
        if (closeRequired_ && OHOS::Security::CodeSign::PrctlWrapper(
                                  JITFORT_PRCTL_OPTION, JITFORT_SWITCH_OUT, 0) != 0) {
            LOG_JIT(FATAL) << "Failed to close the JitFort write window";
        }
#endif
    }

    NO_COPY_SEMANTIC(JitFortWriteScope);
    NO_MOVE_SEMANTIC(JitFortWriteScope);

    bool Opened() const
    {
        return opened_;
    }

private:
    bool opened_ {true};
#if defined(JIT_ENABLE_CODE_SIGN) && !defined(JIT_FORT_DISABLE) && defined(PANDA_TARGET_ARM64) && \
    defined(PANDA_TARGET_OHOS)
    bool closeRequired_ {false};
#endif
};

}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JIT_JIT_FORT_WRITE_SCOPE_H
