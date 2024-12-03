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
class JSHClass;
enum class Root;

enum class SharedMarkType : uint8_t {
    NOT_CONCURRENT_MARK,
    CONCURRENT_MARK_INITIAL_MARK,
    CONCURRENT_MARK_REMARK,
};

class SharedGCMarkerBase {
public:
    explicit SharedGCMarkerBase(SharedGCWorkManager *workManger) : sWorkManager_(workManger) {}
    virtual ~SharedGCMarkerBase() = default;

    void ResetWorkManager(SharedGCWorkManager *workManager);
    void MarkRoots(uint32_t threadId, SharedMarkType markType, VMRootVisitType type = VMRootVisitType::MARK);
    void MarkLocalVMRoots(uint32_t threadId, EcmaVM *localVm, SharedMarkType markType,
                          VMRootVisitType type = VMRootVisitType::MARK);
    void CollectLocalVMRSet(EcmaVM *localVm);
    void MarkStringCache(uint32_t threadId);
    void MarkSerializeRoots(uint32_t threadId);
    void MarkSharedModule(uint32_t threadId);
    inline void ProcessThenMergeBackRSetFromBoundJSThread(RSetWorkListHandler *handler);
    template<SharedMarkType markType>
    inline void DoMark(uint32_t threadId);
    template <typename Callback>
    inline bool VisitBodyInObj(TaggedObject *root, ObjectSlot start, ObjectSlot end, Callback callback);
    inline void HandleRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot slot);
    inline void HandleLocalRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot slot);
    inline void HandleLocalRangeRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot start,
                                      ObjectSlot end);
    inline void RecordWeakReference(uint32_t threadId, JSTaggedType *ref);
    void MergeBackAndResetRSetWorkListHandler();
    template<SharedMarkType markType>
    inline void ProcessVisitorOfDoMark(uint32_t threadId);
    inline void ProcessVisitor(RSetWorkListHandler *handler);
    inline bool MarkObjectOfProcessVisitor(void *mem, WorkNode *&localBuffer);

    inline void MarkObjectFromJSThread(WorkNode *&localBuffer, TaggedObject *object);

    virtual inline void MarkValue([[maybe_unused]] uint32_t threadId, [[maybe_unused]] ObjectSlot &slot)
    {
        LOG_GC(FATAL) << " can not call this method";
    }

    virtual inline void MarkObject([[maybe_unused]] uint32_t threadId, [[maybe_unused]] TaggedObject *object,
                                   [[maybe_unused]] ObjectSlot &slot)
    {
        LOG_GC(FATAL) << " can not call this method";
    }

    virtual inline void HandleLocalDerivedRoots([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base,
                                                [[maybe_unused]] ObjectSlot derived,
                                                [[maybe_unused]] uintptr_t baseOldObject)
    {
        LOG_GC(FATAL) << " can not call this method";
    }
    virtual void ProcessMarkStack([[maybe_unused]] uint32_t threadId)
    {
        LOG_GC(FATAL) << " can not call this method";
    }

protected:
    SharedGCWorkManager *sWorkManager_ {nullptr};

private:
    // This method is called within the GCIterateThreadList method,
    // so the thread lock problem does not need to be considered.
    inline void NotifyThreadProcessRsetStart(JSThread *localThread);
    inline void NotifyThreadProcessRsetFinished(JSThread *localThread);

    template<SharedMarkType markType>
    inline auto GenerateRSetVisitor(uint32_t threadId);
    inline void RecordObject(JSTaggedValue value, uint32_t threadId, void *mem);
    template<SharedMarkType markType>
    inline bool GetVisitor(JSTaggedValue value, uint32_t threadId, void *mem);

    std::vector<RSetWorkListHandler*> rSetHandlers_;
};

class SharedGCMarker : public SharedGCMarkerBase {
public:
    explicit SharedGCMarker(SharedGCWorkManager *workManger);
    ~SharedGCMarker() override = default;
    void ProcessMarkStack(uint32_t threadId) override;

protected:
    inline void MarkValue(uint32_t threadId, ObjectSlot &slot) override;
    inline void MarkObject(uint32_t threadId, TaggedObject *object, ObjectSlot &slot) override;
    inline void HandleLocalDerivedRoots(Root type, ObjectSlot base, ObjectSlot derived,
                                        uintptr_t baseOldObject) override;
};

class SharedGCMovableMarker : public SharedGCMarkerBase {
public:
    explicit SharedGCMovableMarker(SharedGCWorkManager *workManger, SharedHeap *sHeap);
    ~SharedGCMovableMarker() override = default;
    inline bool NeedEvacuate(Region *region);
    void ProcessMarkStack(uint32_t threadId) override;

protected:
    inline void HandleLocalDerivedRoots(Root type, ObjectSlot base, ObjectSlot derived,
                                        uintptr_t baseOldObject) override;
    inline void MarkValue(uint32_t threadId, ObjectSlot &slot) override;
    inline void MarkObject(uint32_t threadId, TaggedObject *object, ObjectSlot &slot) override;
    inline uintptr_t AllocateForwardAddress(uint32_t threadId, size_t size);
    inline void EvacuateObject(uint32_t threadId, TaggedObject *object, const MarkWord &markWord, ObjectSlot slot);
    inline uintptr_t AllocateDstSpace(uint32_t threadId, size_t size);
    inline void RawCopyObject(uintptr_t fromAddress, uintptr_t toAddress, size_t size, const MarkWord &markWord);
    inline void UpdateForwardAddressIfSuccess(uint32_t threadId, TaggedObject *object, JSHClass *klass,
                                              uintptr_t toAddress, size_t size, ObjectSlot slot);
    inline void UpdateForwardAddressIfFailed(TaggedObject *object, uintptr_t toAddress, size_t size, ObjectSlot slot);

private:
    SharedHeap *sHeap_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SHARED_HEAP_SHARED_GC_MARKER_H