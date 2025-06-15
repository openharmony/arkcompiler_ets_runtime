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

#include "common_components/base_runtime/hooks.h"

#include <cstdint>

#include "ecmascript/base/config.h"
#include "ecmascript/free_object.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/mem/tagged_object.h"
#include "ecmascript/mem/tagged_state_word.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/runtime.h"
#include "objects/base_type.h"
#include "objects/composite_base_class.h"

namespace common {
using panda::ecmascript::ObjectXRay;
using panda::ecmascript::FreeObject;
using panda::ecmascript::ObjectSlot;
using panda::ecmascript::JSThread;

class CMCRootVisitor final : public panda::ecmascript::RootVisitor {
public:
    inline explicit CMCRootVisitor(const RefFieldVisitor &visitor): visitor_(visitor) {};
    ~CMCRootVisitor() override = default;

    inline void VisitRoot([[maybe_unused]] panda::ecmascript::Root type,
                          ObjectSlot slot) override
    {
        panda::ecmascript::JSTaggedValue value(slot.GetTaggedType());
        if (value.IsHeapObject()) {
            ASSERT(!value.IsWeak());

            visitor_(reinterpret_cast<RefField<>&>(*(slot.GetRefFieldAddr())));
        }
    }

    inline void VisitRangeRoot([[maybe_unused]] panda::ecmascript::Root type,
                               ObjectSlot start, ObjectSlot end) override
    {
        for (ObjectSlot slot = start; slot < end; slot++) {
            panda::ecmascript::JSTaggedValue value(slot.GetTaggedType());
            if (value.IsHeapObject()) {
                ASSERT(!value.IsWeak());

                visitor_(reinterpret_cast<RefField<>&>(*(slot.GetRefFieldAddr())));
            }
        }
    }

    inline void VisitBaseAndDerivedRoot([[maybe_unused]] panda::ecmascript::Root type,
                                        [[maybe_unused]] ObjectSlot base,
                                        [[maybe_unused]] ObjectSlot derived,
                                        [[maybe_unused]] uintptr_t baseOldObject) override
    {
        panda::ecmascript::JSTaggedValue baseVal(base.GetTaggedType());
        if (baseVal.IsHeapObject()) {
            derived.Update(base.GetTaggedType() + derived.GetTaggedType() - baseOldObject);
        }
    }

private:
    const RefFieldVisitor &visitor_;
};

class CMCWeakVisitor final : public panda::ecmascript::WeakVisitor {
public:
    inline explicit CMCWeakVisitor(const common::WeakRefFieldVisitor &visitor) : visitor_(visitor) {};
    ~CMCWeakVisitor() override = default;

    inline bool VisitRoot([[maybe_unused]] panda::ecmascript::Root type, ObjectSlot slot) override
    {
        panda::ecmascript::JSTaggedValue value(slot.GetTaggedType());
        if (value.IsHeapObject()) {
            ASSERT(!value.IsWeak());
            return visitor_(reinterpret_cast<RefField<> &>(*(slot.GetRefFieldAddr())));
        }
        return true;
    }

private:
    const common::WeakRefFieldVisitor &visitor_;
};

void VisitBaseRoots(const RefFieldVisitor &visitorFunc)
{
    BaseClassRoots &baseClassRoots = BaseRuntime::GetInstance()->GetBaseClassRoots();
    // When visit roots, the language of the object is not used, so using the visitorFunc will work for
    // both dynamic and static.
    baseClassRoots.IterateCompositeBaseClass(visitorFunc);
}

void VisitDynamicRoots(const RefFieldVisitor &visitorFunc, bool isMark)
{
    panda::ecmascript::VMRootVisitType type = isMark ? panda::ecmascript::VMRootVisitType::MARK :
                                                panda::ecmascript::VMRootVisitType::UPDATE_ROOT;
    CMCRootVisitor visitor(visitorFunc);
    OHOS_HITRACE(HITRACE_LEVEL_MAX, "CMCGC::VisitSharedRoot", "");
    // MarkSharedModule
    panda::ecmascript::SharedModuleManager::GetInstance()->Iterate(visitor);

    panda::ecmascript::Runtime *runtime = panda::ecmascript::Runtime::GetInstance();
    // MarkSerializeRoots
    runtime->IterateSerializeRoot(visitor);

    // MarkStringCache
    runtime->IterateCachedStringRoot(visitor);

    runtime->GCIterateThreadList([&](JSThread *thread) {
        auto vm = thread->GetEcmaVM();
        ObjectXRay::VisitVMRoots(vm, visitor, type);

        auto profiler = vm->GetPGOProfiler();
        if (profiler != nullptr) {
            profiler->IteratePGOPreFuncList(visitor);
        }
    });
}

void VisitDynamicWeakRoots(const common::WeakRefFieldVisitor &visitorFunc)
{
    OHOS_HITRACE(HITRACE_LEVEL_MAX, "CMCGC::VisitDynamicWeakRoots", "");
    CMCWeakVisitor visitor(visitorFunc);

    panda::ecmascript::SharedHeap::GetInstance()->IteratorNativePointerList(visitor);

    panda::ecmascript::Runtime *runtime = panda::ecmascript::Runtime::GetInstance();

    runtime->GetEcmaStringTable()->IterWeakRoot(visitorFunc);
    runtime->IteratorNativeDeleteInSharedGC(visitor);

    runtime->GCIterateThreadList([&](JSThread *thread) {
        auto vm = thread->GetEcmaVM();
        const_cast<panda::ecmascript::Heap *>(vm->GetHeap())->IteratorNativePointerList(visitor);
        thread->ClearVMCachedConstantPool();
        thread->IterateWeakEcmaGlobalStorage(visitor);
    });
}

void VisitJSThread(void *jsThread, CommonRootVisitor visitor)
{
    reinterpret_cast<JSThread *>(jsThread)->Visit(visitor);
}

void SynchronizeGCPhaseToJSThread(void *jsThread, GCPhase gcPhase)
{
    reinterpret_cast<JSThread *>(jsThread)->SetCMCGCPhase(gcPhase);
    if (panda::ecmascript::g_isEnableCMCGC) {
// forcely enable read barrier for read barrier DFX
#ifdef ENABLE_CMC_RB_DFX
        reinterpret_cast<JSThread *>(jsThread)->SetReadBarrierState(true);
        return;
#endif
        if (gcPhase >= GCPhase::GC_PHASE_PRECOPY) {
            reinterpret_cast<JSThread *>(jsThread)->SetReadBarrierState(true);
        } else {
            reinterpret_cast<JSThread *>(jsThread)->SetReadBarrierState(false);
        }
    }
}

void SweepThreadLocalJitFort()
{
    panda::ecmascript::Runtime* runtime = panda::ecmascript::Runtime::GetInstance();

    runtime->GCIterateThreadList([&](JSThread* thread) {
        if (thread->IsJSThread()) {
            auto vm = thread->GetEcmaVM();
            const_cast<panda::ecmascript::Heap*>(vm->GetHeap())->GetMachineCodeSpace()->Sweep();
        }
    });
}

void FillFreeObject(void *object, size_t size)
{
    panda::ecmascript::FreeObject::FillFreeObject(panda::ecmascript::SharedHeap::GetInstance(),
        reinterpret_cast<uintptr_t>(object), size);
}

void JSGCCallback(void *ecmaVM)
{
    panda::ecmascript::EcmaVM *vm = reinterpret_cast<panda::ecmascript::EcmaVM*>(ecmaVM);
    ASSERT(vm != nullptr);
    JSThread *thread = vm->GetAssociatedJSThread();
    if (thread != nullptr && thread->ReadyForGCIterating()) {
        panda::ecmascript::Heap *heap = const_cast<panda::ecmascript::Heap*>(vm->GetHeap());
        heap->ProcessGCCallback();
    }
}

void SetBaseAddress(uintptr_t base)
{
    // Please be careful about reentrant
    ASSERT(panda::ecmascript::TaggedStateWord::BASE_ADDRESS == 0);
    panda::ecmascript::TaggedStateWord::BASE_ADDRESS = base;
}

void JitFortUnProt(size_t size, void* base)
{
    panda::ecmascript::PageMap(size, PAGE_PROT_READWRITE, 0, base, PAGE_FLAG_MAP_FIXED);
}

bool IsMachineCodeObject(uintptr_t objPtr)
{
    JSTaggedValue value(objPtr);
    return value.IsMachineCodeObject();
}

} // namespace panda
