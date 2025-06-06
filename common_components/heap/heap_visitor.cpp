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

#include "common_interfaces/heap/heap_visitor.h"

#include "common_components/base_runtime/hooks.h"

namespace panda {
VisitStaticRootsHookFunc g_visitStaticRootsHook = nullptr;
UpdateStaticRootsHookFunc g_updateStaticRootsHook = nullptr;
SweepStaticRootsHookFunc g_sweepStaticRootsHook = nullptr;

void RegisterVisitStaticRootsHook(VisitStaticRootsHookFunc func)
{
    g_visitStaticRootsHook = func;
}

void RegisterUpdateStaticRootsHook(UpdateStaticRootsHookFunc func)
{
    g_updateStaticRootsHook = func;
}

void RegisterSweepStaticRootsHook(SweepStaticRootsHookFunc func)
{
    g_sweepStaticRootsHook = func;
}

void VisitRoots(const RefFieldVisitor &visitor, bool isMark)
{
    VisitDynamicRoots(visitor, isMark);
    if (isMark) {
        if (g_visitStaticRootsHook != nullptr) {
            g_visitStaticRootsHook(visitor);
        }
    } else {
        if (g_updateStaticRootsHook != nullptr) {
            g_updateStaticRootsHook(visitor);
        }
    }
}

void VisitWeakRoots(const WeakRefFieldVisitor &visitor)
{
    VisitDynamicWeakRoots(visitor);
    if (g_updateStaticRootsHook != nullptr) {
        g_updateStaticRootsHook(visitor);
    }
    if (g_sweepStaticRootsHook != nullptr) {
        g_sweepStaticRootsHook(visitor);
    }
}
}  // namespace panda
