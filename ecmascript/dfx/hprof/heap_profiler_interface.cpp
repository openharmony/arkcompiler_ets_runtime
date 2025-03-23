/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/dfx/hprof/heap_profiler_interface.h"
#include "ecmascript/dfx/hprof/heap_profiler.h"


namespace panda::ecmascript {
HeapProfilerInterface *HeapProfilerInterface::GetInstance(EcmaVM *vm)
{
    return vm->GetOrNewHeapProfile();
}

void HeapProfilerInterface::Destroy(EcmaVM *vm)
{
    vm->DeleteHeapProfile();
}

HeapProfilerInterface *HeapProfilerInterface::CreateNewInstance(const EcmaVM *vm)
{
    return new HeapProfiler(vm);
}

void HeapProfilerInterface::DestroyInstance(HeapProfilerInterface *heapProfiler)
{
    delete heapProfiler;
}
}  // namespace panda::ecmascript
