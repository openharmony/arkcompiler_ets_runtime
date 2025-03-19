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

#ifndef ECMASCRIPT_PLATFORM_DFX_HISYS_EVENT_H
#define ECMASCRIPT_PLATFORM_DFX_HISYS_EVENT_H

#include "ecmascript/mem/long_gc_stats.h"
#include "ecmascript/mem/mem_common.h"

namespace panda::ecmascript {
class DFXHiSysEvent {
    
public:
    static void SendLongGCEvent(LongGCStats *longGCStats);

    static bool IsEnableDFXHiSysEvent();

    static double GetCpuUsage();
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PLATFORM_DFX_HISYS_EVENT_H
