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

#ifndef PANDA_ECMASCRIPT_DFX_HPROF_HEAP_DUMP_SESSION_H
#define PANDA_ECMASCRIPT_DFX_HPROF_HEAP_DUMP_SESSION_H

#include "ecmascript/platform/mutex.h"

namespace panda::ecmascript {

/**
 * @brief Serializes parent-process preparation for dynamic heap dumps.
 *
 * A dump-all request can schedule the main hybrid runtime and dynamic-only
 * worker runtimes on different event loops. Their GC, suspension and heap
 * preparation must not overlap. A contending JS thread enters a non-running
 * state while waiting, so it cannot block a shared GC that needs to suspend
 * all JS threads. Forked children operate on private snapshots.
 */
class HeapDumpSession final {
public:
    HeapDumpSession();
    ~HeapDumpSession();

    HeapDumpSession(const HeapDumpSession &) = delete;
    HeapDumpSession &operator=(const HeapDumpSession &) = delete;
    HeapDumpSession(HeapDumpSession &&) = delete;
    HeapDumpSession &operator=(HeapDumpSession &&) = delete;

private:
    static Mutex mutex_;
    int ownerPid_ {-1};
};

}  // namespace panda::ecmascript

#endif  // PANDA_ECMASCRIPT_DFX_HPROF_HEAP_DUMP_SESSION_H
