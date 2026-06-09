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

#ifndef ECMASCRIPT_DFX_HPROF_HYBRID_HYBRID_HEAP_PROFILER_H
#define ECMASCRIPT_DFX_HPROF_HYBRID_HYBRID_HEAP_PROFILER_H

#include <memory>

#include "ecmascript/dfx/hprof/heap_profiler.h"
#include "ecmascript/dfx/hprof/string_hashmap.h"
#include "hybrid/sts_vm_interface.h"

namespace panda::ecmascript {

struct DumpSnapShotOption;
class RuntimeLockHolder;
template<typename T> class SuspendAllScope;
class Stream;
class HybridHeapSnapshot;

class HybridHeapProfiler {
public:
    explicit HybridHeapProfiler(EcmaVM *vm);
    ~HybridHeapProfiler() = default;

    NO_COPY_SEMANTIC(HybridHeapProfiler);
    NO_MOVE_SEMANTIC(HybridHeapProfiler);

    static HybridHeapProfiler *GetInstance();

    bool Dump(EcmaVM *vm, Stream *stream, DumpSnapShotOption &dumpOption);

    EntryIdMap *GetEntryIdMap()
    {
        return &entryIdMap_;
    }

    arkplatform::STSVMInterface *GetSTSInterface() const
    {
        return stsInterface_;
    }

    bool HasSTSInterface() const
    {
        return stsInterface_ != nullptr;
    }

    friend class HybridHeapProfilerTestHelper;

private:
    class EtsSuspendAllScope {
    public:
        explicit EtsSuspendAllScope(arkplatform::STSVMInterface *stsInterface, bool enable)
            : stsInterface_(stsInterface), enabled_(enable)
        {
            ASSERT(stsInterface_ != nullptr);
            if (enabled_) {
                stsInterface_->SuspendEtsThreads();
            }
        }
        ~EtsSuspendAllScope()
        {
            if (enabled_) {
                stsInterface_->ResumeEtsThreads();
            }
        }
        EtsSuspendAllScope(const EtsSuspendAllScope&) = delete;
        EtsSuspendAllScope& operator=(const EtsSuspendAllScope&) = delete;
    private:
        arkplatform::STSVMInterface *stsInterface_;
        bool enabled_;
    };

    class JSSuspendAllScope {
    public:
        JSSuspendAllScope(EcmaVM *vm, bool enabled);
        ~JSSuspendAllScope();
        JSSuspendAllScope(const JSSuspendAllScope&) = delete;
        JSSuspendAllScope& operator=(const JSSuspendAllScope&) = delete;
    private:
        EcmaVM *vm_;
        JSThread *thread_ {nullptr};
        bool enabled_;
        std::unique_ptr<RuntimeLockHolder> locker_;
        std::unique_ptr<SuspendAllScope<JSThread>> suspendScope_;
    };

    static void JSForceFullGC(const EcmaVM *vm);
    static void WaitForJSGCFinish(const EcmaVM *vm, const DumpSnapShotOption &dumpOption);
    void ForceFullGC(EcmaVM *vm, const DumpSnapShotOption &dumpOption);
    static bool SetAppFreezeFilter();
    pid_t ForkAndPerformDump(EcmaVM *vm, Stream *stream, const DumpSnapShotOption &dumpOption);
    bool BuildAndSerializeSnapshot(EcmaVM *vm, Stream *stream, const DumpSnapShotOption &dumpOption);
    void UpdateEntryIdMap(HybridHeapSnapshot *snapshot);
    void FillIdMap(EcmaVM *vm, const DumpSnapShotOption &dumpOption);
    int32_t AcquireDumpStream(const DumpSnapShotOption &dumpOption);

    EcmaVM *mainVm_ {nullptr};
    EntryIdMap entryIdMap_;
    StringHashMap stringTable_;
    arkplatform::STSVMInterface *stsInterface_ {nullptr};
};

}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_DFX_HPROF_HYBRID_HYBRID_HEAP_PROFILER_H
