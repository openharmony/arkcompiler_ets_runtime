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

#include "cgfunc.h"
#if TARGAARCH64
#include "aarch64_ra_opt.h"
#elif TARGRISCV64
#include "riscv64_ra_opt.h"
#endif

namespace maplebe {
using namespace maple;

bool CgRaOpt::PhaseRun(maplebe::CGFunc &f)
{
    MemPool *memPool = GetPhaseMemPool();
    RaOpt *raOpt = nullptr;
#if TARGAARCH64
    raOpt = memPool->New<AArch64RaOpt>(f, *memPool);
#elif || TARGRISCV64
    raOpt = memPool->New<Riscv64RaOpt>(f, *memPool);
#endif

    if (raOpt) {
        raOpt->Run();
    }
    return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgRaOpt, raopt)
} /* namespace maplebe */
