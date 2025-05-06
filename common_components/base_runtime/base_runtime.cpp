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

#include "common_interfaces/base_runtime.h"

#include "common_components/common_runtime/src/heap/heap.h"
#include "common_components/common_runtime/src/heap_manager.h"
#include "common_interfaces/thread/thread_state_transition.h"

namespace panda {
std::mutex BaseRuntime::vmCreationLock_;
BaseRuntime *BaseRuntime::baseRuntimeInstance_ = nullptr;

BaseRuntime *BaseRuntime::GetInstance()
{
    if (UNLIKELY_CC(baseRuntimeInstance_ == nullptr)) {
        std::unique_lock<std::mutex> lock(vmCreationLock_);
        if (baseRuntimeInstance_ == nullptr) {
            baseRuntimeInstance_ = new BaseRuntime();
        }
    }
    return baseRuntimeInstance_;
}

void BaseRuntime::DestroyInstance()
{
    std::unique_lock<std::mutex> lock(vmCreationLock_);
    delete baseRuntimeInstance_;
    baseRuntimeInstance_ = nullptr;
}

void BaseRuntime::PreFork(ThreadHolder *holder)
{
    // Need appspawn space and compress gc.
    heap_.RequestGC(GcType::FULL);
    {
        ThreadNativeScope scope(holder);
        HeapManager::StopRuntimeThreads();
    }
}

void BaseRuntime::PostFork()
{
    HeapManager::StartRuntimeThreads();
}

void BaseRuntime::WriteBarrier(void* obj, void* field, void* ref)
{
    Heap::GetBarrier().WriteBarrier(reinterpret_cast<BaseObject*>(obj),
        *reinterpret_cast<RefField<>*>(field), reinterpret_cast<BaseObject*>(ref));
}

void* BaseRuntime::ReadBarrier(void* obj, void* field)
{
    return reinterpret_cast<void*>(Heap::GetBarrier().ReadRefField(reinterpret_cast<BaseObject*>(obj),
        *reinterpret_cast<RefField<false>*>(field)));
}

void* BaseRuntime::ReadBarrier(void* field)
{
    return reinterpret_cast<void*>(Heap::GetBarrier().ReadStaticRef(*reinterpret_cast<RefField<false>*>(field)));
}

void* BaseRuntime::AtomicReadBarrier(void* obj, void* field, std::memory_order order)
{
    return reinterpret_cast<void*>(Heap::GetBarrier().AtomicReadRefField(reinterpret_cast<BaseObject*>(obj),
        *reinterpret_cast<RefField<true>*>(field), order));
}
}  // namespace panda
