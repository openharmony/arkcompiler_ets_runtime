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

#include <ostream>

#include "heap/visitor.h"

namespace panda {
using CommonRootVisitor = void (*)(void *root);

extern "C" void VisitJSThread(void *jsThread, CommonRootVisitor visitor)
{
}

void VisitRoots(const RefFieldVisitor& visitorFunc, bool isMark)
{
}

void VisitWeakRoots(const panda::WeakRefFieldVisitor& visitorFunc)
{
}

std::vector<uintptr_t> GetStacktrace()
{
    return {};
}

std::ostream &PrintStack(const std::vector<uintptr_t> &stacktrace, std::ostream &out)
{
    return out;
}
}  // panda