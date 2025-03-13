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

#include "ecmascript/platform/dfx_hisys_event.h"
#include "ecmascript/mem/long_gc_stats.h"
#include "ecmascript/log.h"
#include "ecmascript/log_wrapper.h"

namespace panda::ecmascript {

void DFXHiSysEvent::SendLongGCEvent([[maybe_unused]]LongGCStats *longGCStats)
{
    LOG_GC(FATAL) << "Send LongGCEvent is not support in windows.";
}

bool DFXHiSysEvent::IsEnableDFXHiSysEvent()
{
    return false;
}

double DFXHiSysEvent::GetCpuUsage()
{
    LOG_GC(WARN) << "Get CpuUsage is not support in windows.";
    return -1.0;
}
}  // namespace panda::ecmascript
