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

namespace common {
void SetBaseAddress(uintptr_t base) {}
void JitFortUnProt(size_t size, void* base) {}
void FillFreeObject(void *object, size_t size) {}
void VisitDynamicRoots(const RefFieldVisitor &visitorFunc, bool isMark) {}
void VisitBaseRoots(const RefFieldVisitor &visitorFunc) {}
void VisitDynamicWeakRoots(const common::WeakRefFieldVisitor &visitorFunc) {}
void SweepThreadLocalJitFort() {}
void SynchronizeGCPhaseToJSThread(void *jsThread, GCPhase gcPhase) {}
void JSGCCallback(void *ecmaVM) {}
void VisitJSThread(void *jsThread, CommonRootVisitor visitor) {}

bool IsMachineCodeObject(uintptr_t objPtr)
{
    return false;
}
} // namespace common
