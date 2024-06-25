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

#ifndef ECMASCRIPT_MEM_SHARED_HEAP_SHARED_GC_MARKER_H
#define ECMASCRIPT_MEM_SHARED_HEAP_SHARED_GC_MARKER_H

#include "ecmascript/js_hclass.h"
#include "ecmascript/mem/rset_worklist_handler-inl.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/work_manager.h"

namespace panda::ecmascript {
class Region;
class TaggedObject;
class SharedGCMarker;

enum class SharedMarkType : uint8_t {
    NOT_CONCURRENT_MARK,
    CONCURRENT_MARK_INITIAL_MARK,
    CONCURRENT_MARK_REMARK,
};

class SharedGCMarker {
public:
    explicit SharedGCMarker(SharedGCWorkManager *workManger) : sWorkManager_(workManger) {}
    ~SharedGCMarker() = default;

    void ResetWorkManager(SharedGCWorkManager *workManager);
    void MarkRoots(uint32_t threadId, SharedMarkType markType);
    void MarkLocalVMRoots(uint32_t threadId, EcmaVM *localVm);
    void CollectLocalVMRSet(EcmaVM *localVm);
    void MarkStringCache(uint32_t threadId);
    void MarkSerializeRoots(uint32_t threadId);
    void MarkSharedModule(uint32_t threadId);
    void ProcessMarkStack(uint32_t threadId);
    inline void ProcessThenMergeBackRSetFromBoundJSThread(RSetWorkListHandler *handler);
    template<SharedMarkType markType>
    inline void DoMark(uint32_t threadId);
    template <typename Callback>
    inline bool VisitBodyInObj(TaggedObject *root, ObjectSlot start, ObjectSlot end, Callback callback);
    inline void MarkValue(uint32_t threadId, ObjectSlot &slot);
    inline void MarkObject(uint32_t threadId, TaggedObject *object);
    inline void HandleRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot slot);
    inline void HandleLocalRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot slot);
    inline void HandleLocalRangeRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot start,
                                      ObjectSlot end);
    inline void HandleLocalDerivedRoots(Root type, ObjectSlot base, ObjectSlot derived,
                                        uintptr_t baseOldObject);
    inline void RecordWeakReference(uint32_t threadId, JSTaggedType *ref);
    void MergeBackAndResetRSetWorkListHandler();

private:
    inline void MarkObjectFromJSThread(WorkNode *&localBuffer, TaggedObject *object);
    template<SharedMarkType markType>
    inline auto GenerateRSetVisitor(uint32_t threadId);

    SharedGCWorkManager *sWorkManager_ {nullptr};
    std::vector<RSetWorkListHandler*> rSetHandlers_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SHARED_HEAP_SHARED_GC_MARKER_H