/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_UNIFIED_GC_UNIFIED_GC_MARKER_H
#define ECMASCRIPT_MEM_UNIFIED_GC_UNIFIED_GC_MARKER_H

#include "ecmascript/ecma_global_storage.h"
#include "ecmascript/mem/parallel_marker-inl.h"
#ifdef PANDA_JS_ETS_HYBRID_MODE
#include "ecmascript/cross_vm/cross_vm_operator.h"
#endif // PANDA_JS_ETS_HYBRID_MODE

namespace panda::ecmascript {

class UnifiedGCMarker : public Marker {
public:
    UnifiedGCMarker(Heap *heap)
        : Marker(heap),
          dThread_(DaemonThread::GetInstance()) {}
    ~UnifiedGCMarker() override = default;

    void Mark();
    void MarkFromObject(TaggedObject *object);
    void ProcessMarkStack(uint32_t threadId) override;
#ifdef PANDA_JS_ETS_HYBRID_MODE
    void SetSTSVMInterface(arkplatform::STSVMInterface *stsIface)
    {
        stsVMInterface_ = stsIface;
    }
#endif // PANDA_JS_ETS_HYBRID_MODE

private:
    NO_COPY_SEMANTIC(UnifiedGCMarker);
    NO_MOVE_SEMANTIC(UnifiedGCMarker);

    class RecursionScope {
    public:
        explicit RecursionScope(UnifiedGCMarker* uMarker) : uMarker_(uMarker)
        {
            if (uMarker_->recursionDepth_++ != 0) {
                LOG_GC(FATAL) << "Recursion in UnifiedGCMarker Constructor, depth: "
                              << uMarker_->recursionDepth_;
            }
        }
        ~RecursionScope()
        {
            if (--uMarker_->recursionDepth_ != 0) {
                LOG_GC(FATAL) << "Recursion in UnifiedGCMarker Destructor, depth: "
                              << uMarker_->recursionDepth_;
            }
        }
    private:
        UnifiedGCMarker* uMarker_ {nullptr};
    };

    void SetDuration(double duration)
    {
        duration_ = duration;
    }

    void Initialize() override;
    void InitializeMarking(uint32_t threadId);
    void DoMarking(uint32_t threadId);
    void Finish(float spendTime);
    template <typename Callback>
    inline bool VisitBodyInObj(TaggedObject *root, ObjectSlot start, ObjectSlot end, Callback callback);
    inline void MarkValue(uint32_t threadId, ObjectSlot &slot);
    inline void MarkObject(uint32_t threadId, TaggedObject *object) override;
#ifdef PANDA_JS_ETS_HYBRID_MODE
    inline void HandleJSXRefObject(TaggedObject *object);
#endif // PANDA_JS_ETS_HYBRID_MODE
    inline void HandleRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot slot) override;
    inline void HandleRangeRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot start,
                                 ObjectSlot end) override;
    inline void HandleDerivedRoots(Root type, ObjectSlot base, ObjectSlot derived,
                                   uintptr_t baseOldObject) override;

    inline void HandleNewToEdenRSet(uint32_t threadId, Region *region) override;
    inline void HandleOldToNewRSet(uint32_t threadId, Region *region) override;

    bool initialized_ {false};
    Mutex initializeMutex_;
    DaemonThread *dThread_ {nullptr};
#ifdef PANDA_JS_ETS_HYBRID_MODE
    arkplatform::STSVMInterface *stsVMInterface_ {nullptr};
#endif // PANDA_JS_ETS_HYBRID_MODE
    int32_t recursionDepth_ {0};
    double duration_ {0.0};
};

class UnifiedGCMarkRootsScope {
public:
    explicit UnifiedGCMarkRootsScope(JSThread *jsThread): jsThread_(jsThread)
    {
        jsThread_->SetNodeKind(NodeKind::UNIFIED_NODE);
    }

    ~UnifiedGCMarkRootsScope()
    {
        jsThread_->SetNodeKind(NodeKind::NORMAL_NODE);
    }

private:
    JSThread *jsThread_;
    NO_COPY_SEMANTIC(UnifiedGCMarkRootsScope);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_UNIFIED_GC_UNIFIED_GC_MARKER_H