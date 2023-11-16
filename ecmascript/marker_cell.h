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

#ifndef ECMASCRIPT_MARKER_CELL_H
#define ECMASCRIPT_MARKER_CELL_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"

namespace panda {
namespace ecmascript {
class MarkerCell : public TaggedObject {
public:
    static MarkerCell *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsMarkerCell());
        return static_cast<MarkerCell *>(object);
    }

    static constexpr size_t BIT_FIELD_OFFSET = TaggedObjectSize();
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);
    DECL_VISIT_PRIMITIVE_OBJECT()

    // define BitField
    static constexpr size_t IS_DETECTOR_INVALID_BITS = 1;
    FIRST_BIT_FIELD(BitField, IsDetectorInvalid, bool, IS_DETECTOR_INVALID_BITS);

    DECL_DUMP()

    inline void InvalidatePropertyDetector()
    {
        SetIsDetectorInvalid(true);
    }
};

}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_MARKER_CELL_H
