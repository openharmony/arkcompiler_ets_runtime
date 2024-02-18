/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CFI_GENERATOR_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CFI_GENERATOR_H

#include "cfi_generator.h"

namespace maplebe {
class AArch64GenCfi : public GenCfi {
public:
    explicit AArch64GenCfi(CGFunc &func) : GenCfi(func)
    {
        useFP = func.UseFP();
        if (func.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc) {
            stackBaseReg = RFP;
        } else {
            stackBaseReg = useFP ? R29 : RSP;
        }
    }
    ~AArch64GenCfi() override = default;

private:
    void GenerateRegisterSaveDirective(BB &bb, Insn &stackDefInsn) override;
    void GenerateRegisterRestoreDirective(BB &bb, Insn &stackRevertInsn) override;

    /* frame pointer(x29) is available as a general-purpose register if useFP is set as false */
    AArch64reg stackBaseReg = RFP;
};
} /* namespace maplebe */
#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CFI_GENERATOR_H */
