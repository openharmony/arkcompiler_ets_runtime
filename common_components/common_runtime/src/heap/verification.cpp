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
#include "common_interfaces/objects/base_state_word.h"
#include "common_interfaces/objects/ref_field.h"
#include "macros.h"
#include "mutator/mutator_manager.h"
#include "securec.h"
#include "thread/mutator_base.h"
#include "w_collector/w_collector.h"
#include <iomanip>
#include <sstream>
#include <unordered_set>

namespace panda {

void VisitRoots(const RefFieldVisitor& visitorFunc, bool isMark);
void VisitWeakRoots(const WeakRefFieldVisitor& visitorFunc);

#define CONTEXT " at " << __FILE__ << ":" << __LINE__ << std::endl

std::string HexDump(const void* address, size_t length)
{
    static constexpr size_t hexDigitsPerByte = 2;
    static constexpr size_t wordSize = sizeof(uint64_t);
    static constexpr size_t addressWidth = sizeof(void*) * hexDigitsPerByte;

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(address);

    for (size_t i = 0; i < length; i += wordSize) {
        // Print address
        oss << "0x" << std::setw(addressWidth) << reinterpret_cast<uintptr_t>(ptr + i) << ": ";

        // Print content
        uint64_t word = 0;
        size_t bytesToRead = std::min(wordSize, length - i);
        memcpy_s(&word, sizeof(uint64_t), ptr + i, bytesToRead);

        oss << "0x" << std::setw(wordSize * hexDigitsPerByte) << word << std::endl;
    }

    return oss.str();
}

std::string GetObjectInfo(const BaseObject* obj)
{
    constexpr size_t defaultInfoLength = 64;

    std::ostringstream s;
    s << std::hex << std::endl << "[object address: 0x" << obj << "]" << std::endl;

    if (obj == nullptr) {
        s << "Skip: nullptr" << std::endl;
    } else {
        s << std::hex << HexDump((void*) obj, defaultInfoLength) << std::endl;
    }

    s << "[Region Info]" << std::endl;
    if (!Heap::IsHeapAddress(obj)) {
        s << "Skip: Object is not in heap range" << std::endl;
    } else {
        auto region = RegionDesc::GetRegionDescAt(reinterpret_cast<MAddress>(obj));
        s << std::hex << "Type: " << (int) region->GetRegionType() << ", "
          << "Start: " << region->GetRegionStart() << ", "
          << "End: " << region->GetRegionEnd() << ", "
          << "AllocPtr: " << region->GetRegionAllocPtr() << ", "
          << "TraceLine: " << region->GetTraceLine() << ", "
          << "CopyLine: " << region->GetCopyLine() << ", "
          << "FixLine: " << region->GetFixLine();
    }

    return s.str();
}

void IsValidRef(const BaseObject* obj, const RefField<>& ref)
{
    // Maybe we need to check ref later
    // ...

    // check referenee
    auto refObj = ref.GetTargetObject();

    CHECKF(Heap::IsHeapAddress(refObj))
        << CONTEXT << std::hex
        << "Object address: 0x" << reinterpret_cast<MAddress>(refObj) << ","
        << "Heap range: [0x" << Heap::heapStartAddr << ", 0x" << Heap::heapCurrentEnd << "]";

    auto region = RegionDesc::GetRegionDescAt(reinterpret_cast<MAddress>(refObj));
    CHECKF(region->GetRegionType() != RegionDesc::RegionType::GARBAGE_REGION)
        << CONTEXT
        << "From object: " << GetObjectInfo(obj) << std::endl
        << "Current object: " << GetObjectInfo(refObj) << std::endl;
    CHECKF(region->GetRegionType() != RegionDesc::RegionType::FREE_REGION)
        << CONTEXT
        << "From object: " << GetObjectInfo(obj) << std::endl
        << "Current object: " << GetObjectInfo(refObj) << std::endl;

    CHECKF(!refObj->IsForwarding() && !refObj->IsForwarded())
        << CONTEXT
        << "From object: " << GetObjectInfo(obj) << std::endl
        << "Current object: " << GetObjectInfo(refObj) << std::endl;

    CHECKF(refObj->IsValidObject() != 0)
        << CONTEXT
        << "From object: " << GetObjectInfo(obj) << std::endl
        << "Current object: " << GetObjectInfo(refObj) << std::endl;

    CHECKF(refObj->GetSize() != 0)
        << CONTEXT
        << "From object: " << GetObjectInfo(obj) << std::endl
        << "Current object: " << GetObjectInfo(refObj) << std::endl;
}

class VerifyVisitor {
public:
    virtual void VerifyRef(const BaseObject* obj, RefField<>& ref)
    {
        VerifyRef(obj, static_cast<const RefField<>&>(ref));
    }

    virtual void VerifyRef(const BaseObject* obj, const RefField<>& ref)
    {
        UNREACHABLE();
    }
};

class AfterMarkVisitor : public VerifyVisitor {
public:
    void VerifyRef(const BaseObject* obj, const RefField<>& ref) override
    {
        // check retraced objects, so they must be in one of the states below
        auto refObj = ref.GetTargetObject();
        CHECKF(RegionSpace::IsResurrectedObject(refObj) || RegionSpace::IsMarkedObject(refObj) ||
               RegionSpace::IsNewObjectSinceTrace(refObj))
            << CONTEXT
            << "More info: " << GetObjectInfo(refObj);

        IsValidRef(obj, ref);
    }
};

class AfterForwardVisitor : public VerifyVisitor {
public:
    void VerifyRef(const BaseObject* obj, const RefField<>& ref) override
    {
        // check objects in from-space, only alive objects are forwarded
        auto refObj = ref.GetTargetObject();
        if (RegionSpace::IsMarkedObject(refObj) || RegionSpace::IsResurrectedObject(refObj)) {
            CHECKF(refObj->IsForwarded())
                << CONTEXT
                << "More info: " << GetObjectInfo(refObj);

            auto toObj = refObj->GetForwardingPointer();
            IsValidRef(obj, RefField<>(toObj));
        }
    }
};

class AfterFixVisitor : public VerifyVisitor {
public:
    void VerifyRef(const BaseObject* obj, const RefField<>& ref) override
    {
        auto refRegion = RegionDesc::GetRegionDescAt(reinterpret_cast<MAddress>(ref.GetTargetObject()));
        CHECKF(refRegion->GetRegionType() != RegionDesc::RegionType::FROM_REGION)
            << CONTEXT
            << "From object: " << GetObjectInfo(obj) << std::endl
            << "Current object: " << GetObjectInfo(ref.GetTargetObject()) << std::endl;

        IsValidRef(obj, ref);
    }
};

class ReadBarrierVisitor : public VerifyVisitor {
protected:
    constexpr static MAddress TAG_RB_DFX_SHIFT = 47;
    constexpr static MAddress TAG_RB_DFX = 0x1ULL << TAG_RB_DFX_SHIFT;
};

class ReadBarrierSetter : public ReadBarrierVisitor {
public:
    void VerifyRef(const BaseObject* obj, RefField<>& ref) override
    {
        if (obj == nullptr) {
            // skip roots
            return;
        }

        auto regionType =
            RegionDesc::GetRegionDescAt(reinterpret_cast<MAddress>(ref.GetTargetObject()))->GetRegionType();
        if (regionType == RegionDesc::RegionType::RECENT_PINNED_REGION ||
            regionType == RegionDesc::RegionType::FULL_PINNED_REGION ||
            regionType == RegionDesc::RegionType::FIXED_PINNED_REGION ||
            regionType == RegionDesc::RegionType::FULL_FIXED_PINNED_REGION) {
            // Read barrier for pinned objects might be optimized out, so don't set dfx tag
            return;
        }

        auto newRefValue = ref.GetFieldValue() | TAG_RB_DFX;
        ref.SetFieldValue(newRefValue);
    }
};

class ReadBarrierUnsetter : public ReadBarrierVisitor {
public:
    void VerifyRef(const BaseObject* obj, RefField<>& ref) override
    {
        auto newRefValue = ref.GetFieldValue() & (~TAG_RB_DFX);
        ref.SetFieldValue(newRefValue);
    }

    static BaseObject* GetTargetObject(const RefField<>& ref)
    {
        // Get the target object from the reference field, ignoring the read barrier tag
        BaseObject* targetObj = ref.GetTargetObject();
        return reinterpret_cast<BaseObject*>(reinterpret_cast<MAddress>(targetObj) & (~TAG_RB_DFX));
    }
};

class VerifyIterator {
public:
    explicit VerifyIterator(RegionSpace& space) : space_(space) {}

    void IterateFromSpace(VerifyVisitor& visitor)
    {
        IterateRegionList(space_.GetRegionManager().fromRegionList_, visitor);
    }

    void IterateRoot(VerifyVisitor& visitor)
    {
        MarkStack<BaseObject*> roots;

        RefFieldVisitor refVisitor = [&](RefField<>& ref) { visitor.VerifyRef(nullptr, ref); };
        VisitRoots(refVisitor, true);
    }

    void IterateWeakRoot(VerifyVisitor& visitor)
    {
        MarkStack<BaseObject*> roots;

        WeakRefFieldVisitor refVisitor = [&](RefField<>& ref) {
            visitor.VerifyRef(nullptr, ref);
            return true;
        };
        VisitWeakRoots(refVisitor);
    }

    void IterateRetraced(VerifyVisitor& visitor, bool forRBDFX = false)
    {
        MarkStack<BaseObject*> markStack;
        std::unordered_set<BaseObject*> markSet;
        BaseObject* obj = nullptr;

        auto markFunc = [this, &visitor, &markStack, &markSet, &obj, &forRBDFX](RefField<>& field) {
            visitor.VerifyRef(obj, field);
            count_++;

            BaseObject* refObj = nullptr;
            if (forRBDFX) {
                refObj = ReadBarrierUnsetter::GetTargetObject(field);
            } else {
                refObj = field.GetTargetObject();
            }

            if (markSet.find(refObj) != markSet.end()) {
                return;
            }
            markSet.insert(refObj);
            markStack.push_back(refObj);
        };

        EnumStrongRoots(markFunc);
        while (!markStack.empty()) {
            obj = markStack.back();
            markStack.pop_back();

            obj->ForEachRefField(markFunc);
        }
    }

    int IterCount()
    {
        return count_;
    }

private:
    void IterateRegionList(RegionList& list, VerifyVisitor& visitor)
    {
        list.VisitAllRegions([&](RegionDesc* region) {
            region->VisitAllObjects([&](BaseObject* obj) { visitor.VerifyRef(nullptr, RefField<>(obj)); });
        });
    }

    void EnumStrongRoots(const std::function<void(RefField<>&)>& markFunc)
    {
        VisitRoots(markFunc, true);
    }

    void Trace(MarkStack<BaseObject*>& markStack) {}

    RegionSpace& space_;
    int count_ = 0;
};

void WVerify::VerifyAfterMarkInternal(RegionSpace& space)
{
    CHECKF(Heap::GetHeap().GetGCPhase() == GCPhase::GC_PHASE_POST_MARK)
        << CONTEXT << "Mark verification should be called after PostTrace()";

    auto iter = VerifyIterator(space);
    auto visitor = AfterMarkVisitor();
    iter.IterateRetraced(visitor);

    LOG_COMMON(DEBUG) << "[WVerify]: VerifyAfterMark verified object number: " << iter.IterCount();
}

void WVerify::VerifyAfterMark(WCollector& collector)
{
#if !defined(ENABLE_CMC_VERIFY)
    return;
#endif
    RegionSpace& space = reinterpret_cast<RegionSpace&>(collector.GetAllocator());
    if (!MutatorManager::Instance().WorldStopped()) {
        ScopedStopTheWorld stw("WGC-verify-aftermark");
        VerifyAfterMarkInternal(space);
    } else {
        VerifyAfterMarkInternal(space);
    }
}

void WVerify::VerifyAfterForwardInternal(RegionSpace& space)
{
    CHECKF(Heap::GetHeap().GetGCPhase() == GCPhase::GC_PHASE_COPY)
        << CONTEXT << "Forward verification should be called after ForwardFromSpace()";

    auto iter = VerifyIterator(space);
    auto visitor = AfterForwardVisitor();
    iter.IterateFromSpace(visitor);

    LOG_COMMON(DEBUG) << "[WVerify]: VerifyAfterForward verified object number: " << iter.IterCount();
}

void WVerify::VerifyAfterForward(WCollector& collector)
{
#if !defined(ENABLE_CMC_VERIFY)
    return;
#endif
    RegionSpace& space = reinterpret_cast<RegionSpace&>(collector.GetAllocator());
    if (!MutatorManager::Instance().WorldStopped()) {
        ScopedStopTheWorld stw("WGC-verify-aftermark");
        VerifyAfterForwardInternal(space);
    } else {
        VerifyAfterForwardInternal(space);
    }
}

void WVerify::VerifyAfterFixInternal(RegionSpace& space)
{
    CHECKF(Heap::GetHeap().GetGCPhase() == GCPhase::GC_PHASE_FIX)
        << CONTEXT << "Fix verification should be called after Fix()";

    auto iter = VerifyIterator(space);
    auto visitor = AfterFixVisitor();
    iter.IterateRetraced(visitor);

    LOG_COMMON(DEBUG) << "[WVerify]: VerifyAfterFix verified object number: " << iter.IterCount();
}

void WVerify::VerifyAfterFix(WCollector& collector)
{
#if !defined(ENABLE_CMC_VERIFY)
    return;
#endif
    RegionSpace& space = reinterpret_cast<RegionSpace&>(collector.GetAllocator());
    if (!MutatorManager::Instance().WorldStopped()) {
        ScopedStopTheWorld stw("WGC-verify-aftermark");
        VerifyAfterFixInternal(space);
    } else {
        VerifyAfterFixInternal(space);
    }
}

void WVerify::EnableReadBarrierDFXInternal(RegionSpace& space)
{
    auto iter = VerifyIterator(space);
    auto setter = ReadBarrierSetter();
    auto unsetter = ReadBarrierUnsetter();

    iter.IterateRetraced(setter, true);
    // some slots of heap object are also roots, so we need to unset them
    iter.IterateRoot(unsetter);
}

void WVerify::EnableReadBarrierDFX(WCollector& collector)
{
#if !defined(ENABLE_CMC_RB_DFX)
    return;
#endif
    RegionSpace& space = reinterpret_cast<RegionSpace&>(collector.GetAllocator());
    if (!MutatorManager::Instance().WorldStopped()) {
        ScopedStopTheWorld stw("WGC-verify-enable-rb-dfx");
        EnableReadBarrierDFXInternal(space);
    } else {
        EnableReadBarrierDFXInternal(space);
    }
}

void WVerify::DisableReadBarrierDFXInternal(RegionSpace& space)
{
    auto iter = VerifyIterator(space);
    auto unsetter = ReadBarrierUnsetter();

    iter.IterateRetraced(unsetter, true);
}

void WVerify::DisableReadBarrierDFX(WCollector& collector)
{
#if !defined(ENABLE_CMC_RB_DFX)
    return;
#endif
    RegionSpace& space = reinterpret_cast<RegionSpace&>(collector.GetAllocator());
    if (!MutatorManager::Instance().WorldStopped()) {
        ScopedStopTheWorld stw("WGC-verify-disable-rb-dfx");
        DisableReadBarrierDFXInternal(space);
    } else {
        DisableReadBarrierDFXInternal(space);
    }
}

}  // namespace panda
