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

#ifndef ECMASCRIPT_MEM_PARALLEL_MARKER_H
#define ECMASCRIPT_MEM_PARALLEL_MARKER_H

#include "ecmascript/js_hclass.h"
#include "ecmascript/mem/gc_bitset.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/work_manager.h"

namespace panda::ecmascript {
class Heap;
class Region;
class TaggedObject;

class Marker {
public:
    explicit Marker(Heap *heap);
    virtual ~Marker() = default;

    virtual void Initialize()
    {
        LOG_GC(DEBUG) << "Marker::Initialize do nothing";
    }

    void MarkRoots(RootVisitor &rootVisitor);

    virtual void MarkJitCodeMap([[maybe_unused]] uint32_t threadId)
    {
        LOG_GC(FATAL) << "can not call this method";
    }

    virtual void ProcessMarkStack([[maybe_unused]] uint32_t threadId)
    {
        LOG_GC(FATAL) << "can not call this method";
    }

    virtual void ProcessIncrementalMarkStack([[maybe_unused]] uint32_t threadId,
                                             [[maybe_unused]] uint32_t markStepSize)
    {
        LOG_GC(FATAL) << "can not call this method";
    }

protected:
    Heap *heap_ {nullptr};
    WorkManager *workManager_ {nullptr};

    friend class Heap;
    friend class FullGCMarkRootVisitor;
};

class NonMovableMarker final : public Marker {
public:
    explicit NonMovableMarker(Heap *heap) : Marker(heap) {}
    ~NonMovableMarker() override = default;

    void ProcessOldToNew(uint32_t threadId);                    // for non-concurrent YoungGC
    void ProcessOldToNewNoMarkStack(uint32_t threadId);         // for concurrent YoungGC
    void ProcessSnapshotRSet(uint32_t threadId);                // for non-concurrent YoungGC
    void ProcessSnapshotRSetNoMarkStack(uint32_t threadId);     // for concurrent YoungGC

protected:
    void ProcessMarkStack(uint32_t threadId) override;
    void MarkJitCodeMap(uint32_t threadId) override;

    void ProcessIncrementalMarkStack(uint32_t threadId, uint32_t markStepSize) override;
private:
    void ProcessYoungGCMarkStack(uint32_t threadId);
    void ProcessOldGCMarkStack(uint32_t threadId);
};

class CompressGCMarker final : public Marker {
public:
    explicit CompressGCMarker(Heap *heap) : Marker(heap) {}
    ~CompressGCMarker() override = default;

    void SetAppSpawn(bool flag)
    {
        isAppSpawn_ = flag;
    }

protected:
    void MarkJitCodeMap(uint32_t threadId) override;
    void ProcessMarkStack(uint32_t threadId) override;

private:
    bool isAppSpawn_ {false};
    Mutex mutex_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_PARALLEL_MARKER_H
