/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef ECMASCRIPT_DFX_VMSTAT_JIT_PREHEAT_PROFILER_H
#define ECMASCRIPT_DFX_VMSTAT_JIT_PREHEAT_PROFILER_H

#include <unordered_map>
#include "ecmascript/mem/c_string.h"

namespace panda::ecmascript {
class JitPreheatProfiler {
public:
    static JitPreheatProfiler* GetInstance()
    {
        static JitPreheatProfiler profiler;
        return &profiler;
    }
    ~JitPreheatProfiler() {}
    
    std::unordered_map<CString, bool> profMap_;
private:
    JitPreheatProfiler() {}
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_DFX_VMSTAT_JIT_PREHEAT_PROFILER_H
