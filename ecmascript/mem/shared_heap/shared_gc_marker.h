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
class SharedFullGCMarkRootVisitor;
class SharedFullGCMarkObjectVisitor;
enum class Root;

enum class SharedMarkType : uint8_t {
    NOT_CONCURRENT_MARK,
    CONCURRENT_MARK_INITIAL_MARK,
    CONCURRENT_MARK_REMARK,
};

class SharedGCMarkerBase {
public:
    explicit SharedGCMarkerBase(SharedGCWorkManager *sWorkManger) : sWorkManager_(sWorkManger) {}
    virtual ~SharedGCMarkerBase() = default;

    void ResetWorkManager(SharedGCWorkManager *workManager);
    void MarkRoots(RootVisitor &visitor, SharedMarkType markType);
    void MarkGlobalRoots(RootVisitor &visitor);
    void MarkAllLocalRoots(RootVisitor &visitor, SharedMarkType markType);
    void MarkLocalVMRoots(RootVisitor &visitor, EcmaVM *localVm, SharedMarkType markType);
    void CollectLocalVMRSet(EcmaVM *localVm);
    void MarkSendableGlobalStorage(RootVisitor &visitor);
    void MarkStringCache(RootVisitor &visitor);
    void MarkSerializeRoots(RootVisitor &visitor);
    void MarkSharedModule(RootVisitor &visitor);
    void ProcessThenMergeBackRSetFromBoundJSThread(RSetWorkListHandler *handler);
    template <typename LocalToShareRSetVisitor>
    void ProcessLocalToShareRSet(LocalToShareRSetVisitor &&visitor);
    void MergeBackAndResetRSetWorkListHandler();

    inline void MarkObjectFromJSThread(WorkNode *&localBuffer, TaggedObject *object);

    virtual void ProcessMarkStack(uint32_t threadId) = 0;

protected:
    SharedGCWorkManager *sWorkManager_ {nullptr};

private:
    // This method is called within the GCIterateThreadList method,
    // so the thread lock problem does not need to be considered.
    inline void NotifyThreadProcessRsetStart(JSThread *localThread);
    inline void NotifyThreadProcessRsetFinished(JSThread *localThread);

    std::vector<RSetWorkListHandler*> rSetHandlers_;
};

class SharedGCMarker final : public SharedGCMarkerBase {
public:
    explicit SharedGCMarker(SharedGCWorkManager *sWorkManger);
    ~SharedGCMarker() override = default;

    void ProcessMarkStack(uint32_t threadId) override;
};

class SharedGCMovableMarker final : public SharedGCMarkerBase {
public:
    explicit SharedGCMovableMarker(SharedGCWorkManager *sWorkManger);
    ~SharedGCMovableMarker() override = default;

    void ProcessMarkStack(uint32_t threadId) override;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SHARED_HEAP_SHARED_GC_MARKER_H