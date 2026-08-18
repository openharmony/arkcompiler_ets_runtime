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
 * WITHOUT WARRANTIES OR CONDITIONS of ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PANDA_ECMASCRIPT_DFX_HPROF_DYNAMIC_DUMP_H
#define PANDA_ECMASCRIPT_DFX_HPROF_DYNAMIC_DUMP_H

#include "ecmascript/dfx/hprof/heap_dump_session.h"
#include "ecmascript/dfx/hprof/rawheap_dump.h"              // RawHeapDump, ObjectMarker
#include "ecmascript/dfx/hprof/heap_profiler_interface.h"   // DumpSnapShotOption, DumpFormat
#include "ecmascript/dfx/hprof/string_hashmap.h"            // StringHashMap
#include "ecmascript/dfx/hprof/heap_snapshot.h"              // HeapSnapshot
#include "ecmascript/dfx/hprof/heap_profiler.h"              // EntryIdMap
#include "profiler/heap_dump.h"

#include <memory>

namespace panda::ecmascript {

using common::dump::AbstractDumper;
using common::dump::DumpRequest;
using common::dump::DumpResult;

class DynamicDumpTestHelper;

/**
 * @brief Owns the dynamic side of a hybrid binary heap dump.
 *
 * This class enables the dynamic (JS/ArkTS) binary dump to participate in
 * HeapDumpCoordinator's unified lifecycle orchestration while preserving the
 * existing V1/V2 rawheap binary format.
 *
 * Runtime suspension, descriptor acquisition, stream/writer construction, and
 * V1/V2 serialization remain internal to this participant.
 *
 * The participant is created for an explicit EcmaVM through EcmaVMInterface.
 * It does not participate in any process-global factory registration.
 */
class DynamicDump : public AbstractDumper {
public:
    enum class HeapProfilerOwnership : uint8_t {
        NONE,
        VM,
        STANDALONE,
    };

    DynamicDump(EcmaVM *vm, const DumpRequest &request);
    ~DynamicDump() override;

    static std::unique_ptr<AbstractDumper> Create(EcmaVM *vm, const DumpRequest &request);

    // -- AbstractDumper interface --

    /**
     * @brief Full GC: local heap CollectGarbage(FULL_GC) +
     * shared heap CollectGarbage(SHARED_GC) + WaitAllTaskFinished.
     */
    void TriggerGC() override;

    /// Waits for asynchronous local-heap reclamation started by XGC.
    void CompleteCrossRuntimeGC() override;

    void PrepareSession() override;
    bool AcquireOutput() override;
    int GetOutputFd() const override
    {
        return outputFd_;
    }
    DumpResult Dump() override;

    /**
     * @brief Return opaque EcmaVM* for XRef context.
     * The ETS coordinator obtains the VM through the runtime-neutral dumper
     * contract without exposing dynamic runtime types.
     */
    void *GetCurrentVM() override
    {
        return static_cast<void *>(vm_);
    }

    /**
     * @brief Resolve a JS heap address to its dynamic node ID without mutating
     * the ID map.
     * Returns 0 when the address was not included in the dump.
     */
    uint32_t GetNodeId(uint64_t addr) const override
    {
        return (rawHeapDump_ != nullptr) ? rawHeapDump_->FindNodeId(addr) : 0;
    }

    /// Enable cross-thread execution in the child process after fork.
    void PrepareForkChild() override
    {
        vm_->GetAssociatedJSThread()->SetCrossThreadExecution(true);
    }

private:
    class RuntimeScope;
    class CrossThreadExecutionScope;

    bool InitializeHeapProfiler();
    bool IsDynamicOOM() const;
    DumpSnapShotOption CreateDumpOption() const;
    bool CreateOutputStream();
    bool CreateRawHeapDump();
    bool Execute();

    HeapDumpSession dumpSession_;
    EcmaVM                    *vm_;
    RawHeapDump               *rawHeapDump_ {nullptr};  // owned, deleted before its dependencies
    std::unique_ptr<Stream> outputStream_;  // owns fd/path stream; reset after rawHeapDump_
    int outputFd_ {-1};
    std::unique_ptr<StringHashMap> stringTable_;  // owned, destroyed after rawHeapDump_
    std::unique_ptr<HeapSnapshot>  snapshot_;      // owned, destroyed after rawHeapDump_
    HeapProfiler *heapProfiler_ {nullptr};
    HeapProfilerOwnership heapProfilerOwnership_ {HeapProfilerOwnership::NONE};
    DumpRequest request_ {};
    std::unique_ptr<RuntimeScope> runtimeScope_;

    friend class DynamicDumpTestHelper;
};

}  // namespace panda::ecmascript

#endif  // PANDA_ECMASCRIPT_DFX_HPROF_DYNAMIC_DUMP_H
