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

#include "offset_adjust.h"
#if TARGAARCH64
#include "aarch64_offset_adjust.h"
#elif TARGRISCV64
#include "riscv64_offset_adjust.h"
#endif
#if TARGARM32
#include "arm32_offset_adjust.h"
#endif

#include "cgfunc.h"

namespace maplebe {
using namespace maple;
bool CgFrameFinalize::PhaseRun(maplebe::CGFunc &f)
{
    FrameFinalize *offsetAdjustment = nullptr;
#if TARGAARCH64 || TARGRISCV64
    offsetAdjustment = GetPhaseAllocator()->New<AArch64FPLROffsetAdjustment>(f);
#endif
#if TARGARM32
    offsetAdjustment = GetPhaseAllocator()->New<Arm32FPLROffsetAdjustment>(f);
#endif
    offsetAdjustment->Run();
    return false;
}

} /* namespace maplebe */
