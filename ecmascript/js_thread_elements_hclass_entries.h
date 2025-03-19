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

#ifndef ECMASCRIPT_JS_THREAD_ELEMENTS_HCLASS_ENTRIES_H
#define ECMASCRIPT_JS_THREAD_ELEMENTS_HCLASS_ENTRIES_H

#include "ecmascript/elements.h"

namespace panda::ecmascript {
struct ElementsHClassEntries {
    static constexpr size_t N_ENTRIES = static_cast<size_t>(Elements::KIND_COUNT);
    struct Entry {
        ConstantIndex hclassIdx = ConstantIndex::INVALID;
        ConstantIndex hclassWithProtoIdx = ConstantIndex::INVALID;
    };
    Entry entries[N_ENTRIES];

    static constexpr size_t SizeArch32 = sizeof(entries);
    static constexpr size_t SizeArch64 = sizeof(entries);

    ElementsHClassEntries()
    {
        for (uint32_t i = 0; i < Elements::KIND_COUNT; ++i) {
            if ((i & 1) != 0) {
                entries[i].hclassIdx = ConstantIndex::ELEMENT_HOLE_TAGGED_HCLASS_INDEX;
                entries[i].hclassWithProtoIdx = ConstantIndex::ELEMENT_HOLE_TAGGED_PROTO_HCLASS_INDEX;
            } else {
                entries[i].hclassIdx = ConstantIndex::ELEMENT_TAGGED_HCLASS_INDEX;
                entries[i].hclassWithProtoIdx = ConstantIndex::ELEMENT_TAGGED_PROTO_HCLASS_INDEX;
            }
        }
#define INIT_ARRAY_HCLASS_INDEX_ARRAYS(name)                                                                       \
        entries[Elements::ToUint(ElementsKind::name)] =                                                            \
            {ConstantIndex::ELEMENT_##name##_HCLASS_INDEX, ConstantIndex::ELEMENT_##name##_PROTO_HCLASS_INDEX};
        ELEMENTS_KIND_INIT_HCLASS_LIST(INIT_ARRAY_HCLASS_INDEX_ARRAYS)
#undef INIT_ARRAY_HCLASS_INDEX_ARRAYS
    }

    ConstantIndex GetArrayInstanceHClassIndex(ElementsKind kind, bool isPrototype) const
    {
        ASSERT(Elements::ToUint(kind) <= Elements::ToUint(ElementsKind::GENERIC));
        auto entry = entries[Elements::ToUint(kind)];
        return isPrototype ? entry.hclassWithProtoIdx : entry.hclassIdx;
    }
};

STATIC_ASSERT_EQ_ARCH(sizeof(ElementsHClassEntries), ElementsHClassEntries::SizeArch32,
                      ElementsHClassEntries::SizeArch64);
}
// namespace panda::ecmascript

#endif // ECMASCRIPT_JS_THREAD_ELEMENTS_HCLASS_ENTRIES_H
