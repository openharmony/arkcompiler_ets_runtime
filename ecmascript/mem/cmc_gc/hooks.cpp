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
#include <mutex>

#include "ecmascript/ecma_vm.h"
#include "ecmascript/free_object.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/mem/tagged_object.h"
#include "ecmascript/mem/tagged_state_word.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/runtime.h"
#include "objects/base_type.h"

namespace panda {
using ecmascript::ObjectXRay;
using ecmascript::FreeObject;
using ecmascript::ObjectSlot;

// A fake EcmaVM which will never used, only for unify the BaseRuntime::Init&Fini
static ecmascript::EcmaVM *g_fakeEcmaVM = nullptr;
static std::mutex g_rtMutexForStatic;

class CMCRootVisitor final : public ecmascript::RootVisitor {
public:
    inline explicit CMCRootVisitor(const RefFieldVisitor &visitor): visitor_(visitor) {};
    ~CMCRootVisitor() override = default;

    inline void VisitRoot([[maybe_unused]] ecmascript::Root type,
                          ObjectSlot slot) override
    {
        ecmascript::JSTaggedValue value(slot.GetTaggedType());
        if (value.IsHeapObject()) {
            ASSERT(!value.IsWeak());

            visitor_(reinterpret_cast<RefField<>&>(*(slot.GetRefFieldAddr())));
        }
    }

    inline void VisitRangeRoot([[maybe_unused]] ecmascript::Root type,
                               ObjectSlot start, ObjectSlot end) override
    {
        for (ObjectSlot slot = start; slot < end; slot++) {
            ecmascript::JSTaggedValue value(slot.GetTaggedType());
            if (value.IsHeapObject()) {
                ASSERT(!value.IsWeak());

                visitor_(reinterpret_cast<RefField<>&>(*(slot.GetRefFieldAddr())));
            }
        }
    }

    inline void VisitBaseAndDerivedRoot([[maybe_unused]] ecmascript::Root type,
                                        [[maybe_unused]] ObjectSlot base,
                                        [[maybe_unused]] ObjectSlot derived,
                                        [[maybe_unused]] uintptr_t baseOldObject) override
    {
        ecmascript::JSTaggedValue baseVal(base.GetTaggedType());
        if (baseVal.IsHeapObject()) {
            derived.Update(base.GetTaggedType() + derived.GetTaggedType() - baseOldObject);
        }
    }

private:
    const RefFieldVisitor &visitor_;
};

class CMCWeakVisitor final : public ecmascript::WeakVisitor {
public:
    inline explicit CMCWeakVisitor(const WeakRefFieldVisitor &visitor) : visitor_(visitor) {};
    ~CMCWeakVisitor() override = default;

    inline bool VisitRoot([[maybe_unused]] ecmascript::Root type, ObjectSlot slot) override
    {
        ecmascript::JSTaggedValue value(slot.GetTaggedType());
        if (value.IsHeapObject()) {
            ASSERT(!value.IsWeak());
            return visitor_(reinterpret_cast<RefField<> &>(*(slot.GetRefFieldAddr())));
        }
        return true;
    }

private:
    const WeakRefFieldVisitor &visitor_;
};

void VisitDynamicRoots(const RefFieldVisitor &visitorFunc, bool isMark)
{
    if (!ecmascript::Runtime::HasInstance()) {
        return;
    }

    ecmascript::VMRootVisitType type = isMark ? ecmascript::VMRootVisitType::MARK :
                                                ecmascript::VMRootVisitType::UPDATE_ROOT;
    CMCRootVisitor visitor(visitorFunc);

    // MarkSharedModule
    ecmascript::SharedModuleManager::GetInstance()->Iterate(visitor);

    ecmascript::Runtime *runtime = ecmascript::Runtime::GetInstance();
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

void VisitDynamicWeakRoots(const WeakRefFieldVisitor &visitorFunc)
{
    if (!ecmascript::Runtime::HasInstance()) {
        return;
    }

    CMCWeakVisitor visitor(visitorFunc);

    ecmascript::SharedHeap::GetInstance()->IteratorNativePointerList(visitor);

    ecmascript::Runtime *runtime = ecmascript::Runtime::GetInstance();

    runtime->GetEcmaStringTable()->IterWeakRoot(visitor);
    runtime->IteratorNativeDeleteInSharedGC(visitor);

    runtime->GCIterateThreadList([&](JSThread *thread) {
        auto vm = thread->GetEcmaVM();
        const_cast<ecmascript::Heap *>(vm->GetHeap())->IteratorNativePointerList(visitor);
        thread->ClearContextCachedConstantPool();
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
#ifdef USE_READ_BARRIER
    if (gcPhase >= GCPhase::GC_PHASE_PRECOPY) {
        reinterpret_cast<JSThread *>(jsThread)->SetReadBarrierState(true);
    } else {
        reinterpret_cast<JSThread *>(jsThread)->SetReadBarrierState(false);
    }
#endif
}

void SweepThreadLocalJitFort()
{
    ecmascript::Runtime* runtime = ecmascript::Runtime::GetInstance();

    runtime->GCIterateThreadList([&](JSThread* thread) {
        if (thread->IsJSThread()) {
            auto vm = thread->GetEcmaVM();
            const_cast<ecmascript::Heap*>(vm->GetHeap())->GetMachineCodeSpace()->Sweep();
        }
    });
}

void FillFreeObject(void *object, size_t size)
{
    ecmascript::FreeObject::FillFreeObject(ecmascript::SharedHeap::GetInstance(),
        reinterpret_cast<uintptr_t>(object), size);
}

void JSGCCallback(void *ecmaVM)
{
    ecmascript::EcmaVM *vm = reinterpret_cast<ecmascript::EcmaVM*>(ecmaVM);
    ASSERT(vm != nullptr);
    ecmascript::JSThread *thread = vm->GetAssociatedJSThread();
    if (thread != nullptr && thread->ReadyForGCIterating()) {
        ecmascript::Heap *heap = const_cast<ecmascript::Heap*>(vm->GetHeap());
        heap->ProcessGCCallback();
    }
}

void CheckAndInitBaseRuntime(const RuntimeParam &param)
{
    std::lock_guard<std::mutex> guard(g_rtMutexForStatic);
    if (BaseRuntime::GetInstance()->HasBeenInitialized()) {
        // BaseRuntime is inited from Dynamic
        return;
    }
    ecmascript::JSRuntimeOptions options(param);
    g_fakeEcmaVM = ecmascript::EcmaVM::Create(options);
}

void CheckAndFiniBaseRuntime()
{
    std::lock_guard<std::mutex> guard(g_rtMutexForStatic);
    if (g_fakeEcmaVM == nullptr) {
        // BaseRuntime is inited from Dynamic
        return;
    }
    g_fakeEcmaVM->GetJSThread()->ManagedCodeBegin();
    ecmascript::EcmaVM::Destroy(g_fakeEcmaVM);
    g_fakeEcmaVM = nullptr;
}

void SetBaseAddress(uintptr_t base)
{
    // Please be careful about reentrant
    ASSERT(ecmascript::TaggedStateWord::BASE_ADDRESS == 0);
    ecmascript::TaggedStateWord::BASE_ADDRESS = base;
}

void JitFortUnProt(size_t size, void* base)
{
    ecmascript::PageMap(size, PAGE_PROT_READWRITE, 0, base, PAGE_FLAG_MAP_FIXED);
}

bool IsMachineCodeObject(uintptr_t objPtr)
{
    JSTaggedValue value(objPtr);
    return value.IsMachineCodeObject();
}

} // namespace panda
