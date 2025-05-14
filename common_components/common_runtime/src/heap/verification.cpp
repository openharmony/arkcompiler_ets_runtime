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

#include "verification.h"
#include "allocator/region_desc.h"
#include "allocator/region_space.h"
#include "common/mark_work_stack.h"
#include "common/type_def.h"
#include "common_components/log/log.h"
#include "common_interfaces/objects/base_object.h"
#include "common_interfaces/objects/ref_field.h"
#include "common_interfaces/objects/base_state_word.h"
#include "mutator/mutator_manager.h"
#include "w_collector/w_collector.h"
#include <unordered_set>

namespace panda {

void VisitRoots(const RefFieldVisitor &visitorFunc, bool isMark);
void VisitWeakRoots(const WeakRefFieldVisitor &visitorFunc);

void IsValidObject(BaseObject *obj)
{
    ASSERT_LOGF(Heap::IsHeapAddress(obj), "object is not in heap");

    auto region = RegionDesc::GetRegionDescAt(reinterpret_cast<MAddress>(obj));
    ASSERT_LOGF(region->GetRegionType() != RegionDesc::RegionType::GARBAGE_REGION, "object is in garbage region");
    ASSERT_LOGF(region->GetRegionType() != RegionDesc::RegionType::FREE_REGION, "object is in to free region");

    ASSERT_LOGF(!obj->IsForwarding() && !obj->IsForwarded(), "object is not in valid state");

    // check type info of object, but currently there is no common interface available

    ASSERT_LOGF(obj->GetSize() != 0, "object size is zero");
}

class VerifyVisitor {
public:
    virtual void VisitObject(BaseObject *obj) {}
    virtual void VisitRefField(RefField<> &ref) {}
};

class AfterMarkVisitor : public VerifyVisitor {
public:
    void VisitObject(BaseObject *obj) override
    {
        // check retraced objects, so they must be in one of the states below
        ASSERT_LOGF(RegionSpace::IsResurrectedObject(obj) || RegionSpace::IsMarkedObject(obj) ||
                        RegionSpace::IsNewObjectSinceTrace(obj),
                    "object is resurrected");

        IsValidObject(obj);
    }
};

class AfterForwardVisitor : public VerifyVisitor {
public:
    void VisitObject(BaseObject *obj) override
    {
        // check objects in from-space
        if (RegionSpace::IsMarkedObject(obj) || RegionSpace::IsResurrectedObject(obj)) {
            ASSERT_LOGF(obj->IsForwarded(), "object is not forwarded");

            auto toObj = obj->GetForwardingPointer();
            IsValidObject(toObj);
        }
    }
};

class AfterFixVisitor : public VerifyVisitor {
public:
    void VisitObject(BaseObject *obj) override
    {
        IsValidObject(obj);
        obj->ForEachRefField([&](panda::RefField<> &field) {
            auto value = field.GetFieldValue();
            if (!Heap::IsTaggedObject(value)) {
                return;
            }

            auto targetObj = field.GetTargetObject();
            IsValidObject(targetObj);
        });
    }
};

class VerifyIterator {
public:
    explicit VerifyIterator(RegionSpace &space) : space_(space) {}

    void IterateFromSpace(VerifyVisitor &visitor)
    {
        IterateRegionList(space_.GetRegionManager().fromRegionList_, visitor);
    }

    void IterateToSpace(VerifyVisitor &visitor)
    {
        IterateRegionList(space_.GetRegionManager().toRegionList_, visitor);
        IterateRegionList(space_.GetRegionManager().toRegionList_, visitor);
        IterateRegionList(space_.GetRegionManager().tlToRegionList_, visitor);
    }

    void IterateNoForwardSpace(VerifyVisitor &visitor)
    {
        IterateRegionList(space_.GetRegionManager().exemptedFromRegionList_, visitor);

        IterateRegionList(space_.GetRegionManager().recentFullRegionList_, visitor);
        IterateRegionList(space_.GetRegionManager().tlRegionList_, visitor);

        IterateRegionList(space_.GetRegionManager().recentLargeRegionList_, visitor);
        IterateRegionList(space_.GetRegionManager().oldLargeRegionList_, visitor);

        IterateRegionList(space_.GetRegionManager().recentPinnedRegionList_, visitor);
        IterateRegionList(space_.GetRegionManager().oldPinnedRegionList_, visitor);
    }

    void IterateGarbageSpace(VerifyVisitor &visitor)
    {
        IterateRegionList(space_.GetRegionManager().garbageRegionList_, visitor);
    }

    void IterateRoot(VerifyVisitor &visitor)
    {
        MarkStack<BaseObject *> roots;

        RefFieldVisitor refVisitor = [&](RefField<> &ref) { visitor.VisitRefField(ref); };
        VisitRoots(refVisitor, true);
    }

    void IterateWeakRoot(VerifyVisitor &visitor)
    {
        MarkStack<BaseObject *> roots;

        WeakRefFieldVisitor refVisitor = [&](RefField<> &ref) {
            visitor.VisitRefField(ref);
            return true;
        };
        VisitWeakRoots(refVisitor);
    }

    void IterateRetraced(VerifyVisitor &visitor)
    {
        MarkStack<BaseObject *> markStack;
        EnumStrongRoots(markStack);

        auto marked = std::unordered_set<BaseObject *>();
        while (!markStack.empty()) {
            auto obj = markStack.back();
            markStack.pop_back();

            if (marked.find(obj) != marked.end()) {
                continue;
            }

            marked.insert(obj);
            visitor.VisitObject(obj);
            count_++;
            obj->ForEachRefField([&markStack](RefField<> &field) {
                auto value = field.GetFieldValue();
                if (Heap::IsTaggedObject(value)) {
                    markStack.push_back(field.GetTargetObject());
                }
            });
        }
    }

    int IterCount()
    {
        return count_;
    }

private:
    void IterateRegionList(RegionList &list, VerifyVisitor &visitor)
    {
        list.VisitAllRegions([&](RegionDesc *region) {
            region->VisitAllObjects([&](BaseObject *obj) {
                count_++;
                visitor.VisitObject(obj);
            });
        });
    }

    void EnumStrongRoots(MarkStack<BaseObject *> &markStack)
    {
        RefFieldVisitor refVisitor = [&](RefField<> &ref) {
            auto value = ref.GetFieldValue();
            if (Heap::IsTaggedObject(value)) {
                markStack.push_back(ref.GetTargetObject());
            }
        };

        VisitRoots(refVisitor, true);
    }

    void Trace(MarkStack<BaseObject *> &markStack) {}

    RegionSpace &space_;
    int count_ = 0;
};

void WVerify::VerifyAfterMarkInternal(RegionSpace &space)
{
    ASSERT_LOGF(Heap::GetHeap().GetGCPhase() == GCPhase::GC_PHASE_POST_MARK,
                "Mark verification should be called after PostTrace()");

    auto iter = VerifyIterator(space);
    auto visitor = AfterMarkVisitor();
    iter.IterateRetraced(visitor);

    VLOG(DEBUGY, "VerifyMark: verified %d objects", iter.IterCount());
}

void WVerify::VerifyAfterMark(WCollector &collector)
{
#ifdef NDEBUG
    // skip verify in release mode
    return;
#endif
    RegionSpace &space = reinterpret_cast<RegionSpace&>(collector.GetAllocator());
    if (!MutatorManager::Instance().WorldStopped()) {
        ScopedStopTheWorld stw("WGC-verify-aftermark");
        VerifyAfterMarkInternal(space);
    } else {
        VerifyAfterMarkInternal(space);
    }
}

void WVerify::VerifyAfterForwardInternal(RegionSpace &space)
{
    ASSERT_LOGF(Heap::GetHeap().GetGCPhase() == GCPhase::GC_PHASE_COPY,
                "Forward verification should be called after ForwardFromSpace()");

    auto iter = VerifyIterator(space);
    auto visitor = AfterForwardVisitor();
    iter.IterateFromSpace(visitor);

    VLOG(DEBUGY, "VerifyForward: verified %d objects", iter.IterCount());
}

void WVerify::VerifyAfterForward(WCollector &collector)
{
#ifdef NDEBUG
    // skip verify in release mode
    return;
#endif
    RegionSpace &space = reinterpret_cast<RegionSpace&>(collector.GetAllocator());
    if (!MutatorManager::Instance().WorldStopped()) {
        ScopedStopTheWorld stw("WGC-verify-aftermark");
        VerifyAfterForwardInternal(space);
    } else {
        VerifyAfterForwardInternal(space);
    }
}

void WVerify::VerifyAfterFixInternal(RegionSpace &space)
{
    ASSERT_LOGF(Heap::GetHeap().GetGCPhase() == GCPhase::GC_PHASE_FIX, "Fix verification should be called after Fix()");

    auto iter = VerifyIterator(space);
    auto visitor = AfterFixVisitor();
    iter.IterateRetraced(visitor);

    VLOG(DEBUGY, "VerifyFix: verified %d objects", iter.IterCount());
}

void WVerify::VerifyAfterFix(WCollector &collector)
{
#ifdef NDEBUG
    // skip verify in release mode
    return;
#endif
    RegionSpace &space = reinterpret_cast<RegionSpace&>(collector.GetAllocator());
    if (!MutatorManager::Instance().WorldStopped()) {
        ScopedStopTheWorld stw("WGC-verify-aftermark");
        VerifyAfterFixInternal(space);
    } else {
        VerifyAfterFixInternal(space);
    }
}

}  // namespace panda
