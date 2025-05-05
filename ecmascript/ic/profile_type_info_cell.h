/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_PROFILE_TYPE_INFO_CELL_H
#define ECMASCRIPT_PROFILE_TYPE_INFO_CELL_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/barriers.h"
#include "ecmascript/mem/visitor.h"

namespace panda {
namespace ecmascript {
class ProfileTypeInfoCell : public TaggedObject {
public:
    CAST_CHECK(ProfileTypeInfoCell, IsProfileTypeInfoCell);

    static constexpr size_t VALUE_OFFSET = TaggedObjectSize();
    ACCESSORS(Value, VALUE_OFFSET, MACHINE_CODE_OFFSET);
    ACCESSORS(MachineCode, MACHINE_CODE_OFFSET, BASELINE_CODE_OFFSET);
    ACCESSORS(BaselineCode, BASELINE_CODE_OFFSET, HANDLE_OFFSET);
    ACCESSORS(Handle, HANDLE_OFFSET, LAST_OFFSET);
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_VISIT_OBJECT(VALUE_OFFSET, LAST_OFFSET);

    bool IsEmptyProfileTypeInfoCell(const JSThread *thread) const
    {
        return this == thread->GlobalConstants()->GetEmptyProfileTypeInfoCell().GetTaggedObject();
    }

    void UpdateProfileTypeInfoCellType(const JSThread *thread)
    {
        // ProfileTypeInfoCell0 -> Cell1 -> CellN
        JSType jsType = GetClass()->GetObjectType();
        if (jsType == JSType::PROFILE_TYPE_INFO_CELL_0) {
            SetClassWithoutBarrier(
                JSHClass::Cast(thread->GlobalConstants()->GetProfileTypeInfoCell1Class().GetTaggedObject()));
        } else if (jsType == JSType::PROFILE_TYPE_INFO_CELL_1) {
            SetClassWithoutBarrier(
                JSHClass::Cast(thread->GlobalConstants()->GetProfileTypeInfoCellNClass().GetTaggedObject()));
        } else {
            ASSERT(jsType == JSType::PROFILE_TYPE_INFO_CELL_N);
        }
    }

    DECL_DUMP()
};

}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_PROFILE_TYPE_INFO_CELL_H
