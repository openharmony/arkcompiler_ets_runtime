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

#ifndef ECMASCRIPT_JSPANDAFILE_CLASS_LITERAL_H
#define ECMASCRIPT_JSPANDAFILE_CLASS_LITERAL_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/barriers.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/visitor.h"

namespace panda::ecmascript {
class ClassLiteral : public TaggedObject {
public:
    CAST_CHECK(ClassLiteral, IsClassLiteral);

    static constexpr size_t ARRAY_OFFSET = TaggedObjectSize();

    ACCESSORS(Array, ARRAY_OFFSET, IS_AOT_USED_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(IsAOTUsed, bool, IS_AOT_USED_OFFSET, LAST_OFFSET);

    DEFINE_ALIGN_SIZE(LAST_OFFSET);
    DECL_VISIT_OBJECT(ARRAY_OFFSET, IS_AOT_USED_OFFSET);
    DECL_DUMP()
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JSPANDAFILE_CLASS_LITERAL_H