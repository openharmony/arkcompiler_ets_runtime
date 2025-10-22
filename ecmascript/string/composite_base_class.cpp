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

#include <atomic>
#include <cstdint>

#include "ecmascript/string/composite_base_class.h"
#include "objects/base_object.h"

namespace panda::ecmascript {
void BaseClassRoots::InitializeCompositeBaseClass(CompositeBaseClassAllocator &allocator)
{
    if (initialized_.exchange(true)) {
        return;
    }
    CreateCompositeBaseClass(EcmaStringType::LINE_STRING, allocator);
    CreateCompositeBaseClass(EcmaStringType::SLICED_STRING, allocator);
    CreateCompositeBaseClass(EcmaStringType::TREE_STRING, allocator);
    CreateCompositeBaseClass(EcmaStringType::CACHED_EXTERNAL_STRING, allocator);
}

void BaseClassRoots::CreateCompositeBaseClass(EcmaStringType type, CompositeBaseClassAllocator& allocator)
{
    CompositeBaseClass* classObject = allocator();
    classObject->class_.ClearBitField();
    classObject->class_.SetEcmaStringType(type);
    size_t index = TypeToIndex[static_cast<size_t>(type)];
    compositeBaseClasses_[index] = classObject;
    baseClasses_[index] = &classObject->class_;
}

BaseStringClass* BaseClassRoots::GetBaseClass(EcmaStringType type) const
{
    return baseClasses_[TypeToIndex[static_cast<size_t>(type)]];
}

void BaseClassRoots::IterateCompositeBaseClass(const common::RefFieldVisitor& visitorFunc)
{
    if (!initialized_) {
        return;
    }
    for (auto& it : compositeBaseClasses_) {
        visitorFunc(reinterpret_cast<common::RefField<>&>(it));
    }
}

} // namespace panda::ecmascript
