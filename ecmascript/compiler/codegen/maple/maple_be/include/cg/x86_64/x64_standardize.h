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

#ifndef MAPLEBE_INCLUDE_X64_STANDARDIZE_H
#define MAPLEBE_INCLUDE_X64_STANDARDIZE_H

#include "standardize.h"

namespace maplebe {
class X64Standardize : public Standardize {
public:
    explicit X64Standardize(CGFunc &f) : Standardize(f)
    {
        SetAddressMapping(true);
    }

    ~X64Standardize() override = default;

private:
    void StdzMov(Insn &insn) override;
    void StdzStrLdr(Insn &insn) override;
    void StdzBasicOp(Insn &insn) override;
    void StdzUnaryOp(Insn &insn, CGFunc &cgFunc) override;
    void StdzCvtOp(Insn &insn, CGFunc &cgFunc) override;
    void StdzShiftOp(Insn &insn, CGFunc &cgFunc) override;
    void StdzFloatingNeg(Insn &insn, CGFunc &cgFunc);
};
}  // namespace maplebe
#endif /* MAPLEBE_INCLUDEX_64_STANDARDIZE_H */
