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

#include <cstdint>
#include <stack>
#include <unistd.h>

#include "allocator/region_manager.h"
#include "common_components/common_runtime/src/common/type_def.h"
#if defined(__RTOS__)
#include <private/futex.h>
#elif defined(_WIN64)
#define NOGDI
#include <windows.h>
#endif
#include "common_components/common_runtime/src/heap/collector/trace_collector.h"
#include "common_components/common_runtime/src/common/scoped_object_access.h"
#include "common_components/common_runtime/src/mutator/mutator_manager.h"
#ifdef _WIN64
#include "common_components/common_runtime/src/os/windows/win_module_manager.h"
#endif

namespace panda {
JSGCCallbackHookType g_jsGCCallbackHook = nullptr;

extern "C" PUBLIC_API void ArkRegisterJSGCCallbackHook(JSGCCallbackHookType hook)
{
    g_jsGCCallbackHook = hook;
}

ThreadLocalData *ArkCommonGetThreadLocalData()
{
    uintptr_t tlDataAddr = reinterpret_cast<uintptr_t>(ThreadLocal::GetThreadLocalData());
#if defined(__aarch64__) && not defined(__RTOS__)
    if (Heap::GetHeap().IsGcStarted()) {
        // Since the TBI(top bit ignore) feature in Aarch64,
        // set gc phase to high 8-bit of ThreadLocalData Address for gc barrier fast path.
        // 56: make gcphase value shift left 56 bit to set the high 8-bit
        tlDataAddr = tlDataAddr | (static_cast<uint64_t>(Heap::GetHeap().GetGCPhase()) << 56);
    }
#endif
    return reinterpret_cast<ThreadLocalData *>(tlDataAddr);
}

// Ensure that mutator phase is changed only once by mutator itself or GC
bool MutatorBase::TransitionGCPhase(bool bySelf)
{
    do {
        GCPhaseTransitionState state = transitionState.load();
        // If this mutator phase transition has finished, just return
        if (state == FINISH_TRANSITION) {
            bool result = mutatorPhase.load() == Heap::GetHeap().GetGCPhase();
            if (!bySelf && !result) { // why check bySelf?
                LOG_COMMON(FATAL) << "Unresolved fatal";
                UNREACHABLE_CC();
            }
            return result;
        }

        // If this mutator is executing phase transition by other thread, mutator should wait but GC just return
        if (state == IN_TRANSITION) {
            if (bySelf) {
                WaitForPhaseTransition();
                return true;
            } else {
                return false;
            }
        }

        if (!bySelf && state == NO_TRANSITION) {
            return true;
        }

        // Current thread set atomic variable to ensure atomicity of phase transition
        CHECK_CC(state == NEED_TRANSITION);
        if (transitionState.compare_exchange_weak(state, IN_TRANSITION)) {
            TransitionToGCPhaseExclusive(Heap::GetHeap().GetGCPhase());
            transitionState.store(FINISH_TRANSITION, std::memory_order_release);
            return true;
        }
    } while (true);
}

void MutatorBase::HandleSuspensionRequest()
{
    for (;;) {
        SetInSaferegion(SAFE_REGION_TRUE);
        if (HasSuspensionRequest(SUSPENSION_FOR_STW)) {
            SuspendForStw();
            if (HasSuspensionRequest(SUSPENSION_FOR_GC_PHASE)) {
                TransitionGCPhase(true);
            } else if (HasSuspensionRequest(SUSPENSION_FOR_CPU_PROFILE)) {
                TransitionToCpuProfile(true);
            }
        } else if (HasSuspensionRequest(SUSPENSION_FOR_GC_PHASE)) {
            TransitionGCPhase(true);
        } else if (HasSuspensionRequest(SUSPENSION_FOR_CPU_PROFILE)) {
            TransitionToCpuProfile(true);
        } else if (HasSuspensionRequest(SUSPENSION_FOR_EXIT)) {
            while (true) {
                sleep(INT_MAX);
            }
        }
        SetInSaferegion(SAFE_REGION_FALSE);
        // Leave saferegion if current mutator has no suspend request, otherwise try again
        if (LIKELY_CC(!HasAnySuspensionRequest() && !HasObserver())) {
            return;
        }
    }
}

void MutatorBase::HandleJSGCCallback()
{
    if (mutator != nullptr) {
        void *vm = reinterpret_cast<Mutator*>(mutator)->GetEcmaVMPtr();
        if (vm != nullptr) {
            g_jsGCCallbackHook(vm);
        }
    }
}

void MutatorBase::SuspendForStw()
{
    ClearSuspensionFlag(SUSPENSION_FOR_STW);
    // wait until StartTheWorld
    int curCount = static_cast<int>(MutatorManager::Instance().GetStwFutexWordValue());
    // Avoid losing wake-ups
    if (curCount > 0) {
#if defined(_WIN64) || defined(__APPLE__)
        MutatorManager::Instance().MutatorWait();
#else
        int* countAddr = MutatorManager::Instance().GetStwFutexWord();
        // FUTEX_WAIT may fail when gc thread wakes up all threads before the current thread reaches this position.
        // But it is not important because there won't be data race between the current thread and the gc thread,
        // and it also won't be frozen since gc thread also modifies the value at countAddr before its waking option.
        (void)Futex(countAddr, FUTEX_WAIT, curCount);
#endif
    }
    SetInSaferegion(SAFE_REGION_FALSE);
    if (MutatorManager::Instance().StwTriggered()) {
        // entering this branch means a second request has been broadcasted, we need to reset this flag to avoid
        // missing the request. And this must be after the behaviour that set saferegion state to false, because
        // we need to make sure that the mutator can always perceive the gc request when the mutator is not in
        // safe region.
        SetSuspensionFlag(SUSPENSION_FOR_STW);
    }
}

#if defined(GCINFO_DEBUG) && GCINFO_DEBUG
void Mutator::CreateCurrentGCInfo() { gcInfos_.CreateCurrentGCInfo(); }
#endif

void Mutator::VisitStackRoots(const RootVisitor& func)
{
    LOG_COMMON(FATAL) << "Unresolved fatal";
    UNREACHABLE_CC();
}

void Mutator::VisitExceptionRoots(const RootVisitor& func)
{
    LOG_COMMON(FATAL) << "Unresolved fatal";
    UNREACHABLE_CC();
}

void Mutator::VisitRawObjects(const RootVisitor& func)
{
    if (rawObject_.object != nullptr) {
        func(rawObject_);
    }
}

Mutator* Mutator::GetMutator() noexcept
{
    return ThreadLocal::GetMutator();
}

void Mutator::StackGuardExpand() const
{
    LOG_COMMON(FATAL) << "Unresolved fatal";
    UNREACHABLE_CC();
}

void Mutator::StackGuardRecover() const
{
    LOG_COMMON(FATAL) << "Unresolved fatal";
    UNREACHABLE_CC();
}

bool Mutator::IsStackAddr(uintptr_t addr)
{
    LOG_COMMON(FATAL) << "Unresolved fatal";
    UNREACHABLE_CC();
}

void Mutator::RecordStackPtrs(std::set<BaseObject**>& resSet)
{
    LOG_COMMON(FATAL) << "Unresolved fatal";
    UNREACHABLE_CC();
}

intptr_t Mutator::FixExtendedStack(intptr_t frameBase, uint32_t adjustedSize __attribute__((unused)), void* ip)
{
    LOG_COMMON(FATAL) << "Unresolved fatal";
    UNREACHABLE_CC();
}

inline void CheckAndPush(BaseObject* obj, std::set<BaseObject*>& rootSet, std::stack<BaseObject*>& rootStack)
{
    auto search = rootSet.find(obj);
    if (search == rootSet.end()) {
        rootSet.insert(obj);
        if (obj->IsValidObject() && obj->HasRefField()) {
            rootStack.push(obj);
        }
    }
}

EnumThreadStackRootType g_enumThreadStackRootHook = nullptr;
extern "C" PUBLIC_API void ArkRegisterEnumThreadStackRootHook(EnumThreadStackRootType hook) {
    g_enumThreadStackRootHook = hook;
}

inline void MutatorBase::GcPhaseEnum(GCPhase newPhase)
{
     // below function is registed by arkts
    AllocBufferAddHookType allocBufferAddHook = [](uint64_t *obj) {
        AllocationBuffer* buffer = AllocationBuffer::GetOrCreateAllocBuffer();
        buffer->PushRoot(obj);
    };
}

// comment all
inline void MutatorBase::GCPhasePreForward(GCPhase newPhase)
{
}

inline void MutatorBase::HandleGCPhase(GCPhase newPhase)
{
    if (newPhase == GCPhase::GC_PHASE_POST_MARK) {
        std::lock_guard<std::mutex> lg(mutatorBaseLock);
        Mutator *actMutator = reinterpret_cast<Mutator*>(mutator);
        if (actMutator->satbNode_ != nullptr) {
            DCHECK_CC(actMutator->satbNode_->IsEmpty());
            SatbBuffer::Instance().RetireNode(actMutator->satbNode_);
            actMutator->satbNode_ = nullptr;
        }
    } else if (newPhase == GCPhase::GC_PHASE_ENUM) {
        GcPhaseEnum(newPhase);
    } else if (newPhase == GCPhase::GC_PHASE_PRECOPY) {
        GCPhasePreForward(newPhase);
    } else if (newPhase == GCPhase::GC_PHASE_REMARK_SATB || newPhase == GCPhase::GC_PHASE_FINAL_MARK) {
        std::lock_guard<std::mutex> lg(mutatorBaseLock);
        Mutator *actMutator = reinterpret_cast<Mutator*>(mutator);
        if (actMutator->satbNode_ != nullptr) {
            SatbBuffer::Instance().RetireNode(actMutator->satbNode_);
            actMutator->satbNode_ = nullptr;
        }
    } else if (newPhase == GCPhase::GC_PHASE_FIX) {
        RegionManager& manager = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator()).GetRegionManager();
        manager.PrepareFix();
    }
}

void MutatorBase::TransitionToGCPhaseExclusive(GCPhase newPhase)
{
    HandleGCPhase(newPhase);
    SetSafepointActive(false);
    // Clear mutator's suspend request after phase transition
    ClearSuspensionFlag(SUSPENSION_FOR_GC_PHASE);
    mutatorPhase.store(newPhase, std::memory_order_release); // handshake between muator & mainGC thread
}

inline void MutatorBase::HandleCpuProfile()
{
    LOG_COMMON(FATAL) << "Unresolved fatal";
    UNREACHABLE_CC();
}

void MutatorBase::TransitionToCpuProfileExclusive()
{
    HandleCpuProfile();
    SetSafepointActive(false);
    ClearSuspensionFlag(SUSPENSION_FOR_CPU_PROFILE);
}

extern "C" void ArkCommonPreRunManagedCode(Mutator* mutator, int layers, ThreadLocalData* threadData)
{
    if (UNLIKELY_CC(MutatorManager::Instance().StwTriggered())) {
        mutator->SetSuspensionFlag(Mutator::SuspensionType::SUSPENSION_FOR_STW);
        mutator->EnterSaferegion(false);
    }
    mutator->LeaveSaferegion();
    mutator->SetMutatorPhase(Heap::GetHeap().GetGCPhase());
}

} // namespace panda
