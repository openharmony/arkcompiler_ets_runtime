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

#include "aarch64_targetinfo.h"
#include "target_registry.h"
#include "aarch64_cg.h"
#include "aarch64_emitter.h"
#include "aarch64_targetmachine.h"
#include <iostream>
namespace maplebe {

Target &getTheAArch64Target()
{
    static Target TheAArch64Target;
    return TheAArch64Target;
}

extern "C" void MAPLEInitializeAArch64TargetInfo(MemPool *m)
{
    RegisterTarget X(getTheAArch64Target(), "aarch64");
    RegisterCGFUnc<AArch64CG> Y(getTheAArch64Target());
    RegisterEmitter<AArch64AsmEmitter> Z(getTheAArch64Target(), m);
    RegisterTargetMachine<AArch64TargetMachine> A(getTheAArch64Target(), m);
}

}
