/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_SHARED_HEAP_GLOBAL_GC_MARKER_H
#define ECMASCRIPT_MEM_SHARED_HEAP_GLOBAL_GC_MARKER_H

#include "ecmascript/mem/work_manager.h"

namespace panda::ecmascript {
class Heap;

class GlobalGCMarker {
public:
    GlobalGCMarker() = default;
    ~GlobalGCMarker() = default;

    void MarkRoots(Heap *heap, GlobalGCWorkNodeHolder *holder) const;
    void ProcessMarkStack(GlobalGCWorkManager *workManager, uint32_t threadIndex) const;
    void MarkJitCodeMaps(GlobalGCWorkManager *workManager, uint32_t threadIndex) const;
};

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SHARED_HEAP_GLOBAL_GC_MARKER_H
