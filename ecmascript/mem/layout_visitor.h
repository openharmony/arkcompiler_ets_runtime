/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_LAYOUT_VISITOR_H
#define ECMASCRIPT_MEM_LAYOUT_VISITOR_H

#include <functional>

#include "ecmascript/js_hclass.h"
#include "ecmascript/mem/slots.h"

namespace panda::ecmascript {
template <VisitType visitType, size_t size>
class JSObjectBodyIterator {
public:
    static inline void IterateBody(TaggedObject *root, const EcmaObjectRangeVisitor& visitor)
    {
        auto hclass = root->SynchronizedGetClass();
        auto objSize = hclass->GetObjectSize();
        if (objSize > size) {
            if (hclass->IsAllTaggedProp()) {
                IteratorRange(root, visitor, size, objSize, VisitObjectArea::NORMAL);
            } else {
                IteratorRange(root, visitor, size, objSize, VisitObjectArea::IN_OBJECT);
            }
        }
    }

    static inline void IteratorRange(TaggedObject *root, const EcmaObjectRangeVisitor& visitor,
        size_t start, size_t end, VisitObjectArea area)
    {
        visitor(root, ObjectSlot(ToUintPtr(root) + start), ObjectSlot(ToUintPtr(root) + end), area);
    }
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_LAYOUT_VISITOR_H
