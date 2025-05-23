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

#ifndef COMMON_COMPONENTS_BASE_RUNTIME_HOOKS_H
#define COMMON_COMPONENTS_BASE_RUNTIME_HOOKS_H

#include <cstdint>

#include "common_interfaces/heap/heap_visitor.h"

// Visitor that iterate all `RefField`s in a TaggedObject and add them to
// `WorkStack` Should be moved to BaseRT and panda namespace later
namespace panda {
// Dynamic VM Roots scanning
void VisitDynamicRoots(const RefFieldVisitor &visitor, bool isMark);
void VisitDynamicWeakRoots(const WeakRefFieldVisitor &visitorFunc);

void VisitJSThread(void *jsThread, CommonRootVisitor visitor);

// CMC-GC dependent interface
void FillFreeObject(void *object, size_t size);
void SetBaseAddress(uintptr_t base);
void JSGCCallback(void *ecmaVM);
} // namespace panda
#endif // COMMON_COMPONENTS_BASE_RUNTIME_HOOKS_H
