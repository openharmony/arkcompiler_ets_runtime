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

#ifndef ECMASCRIPT_ELEMENTS_H
#define ECMASCRIPT_ELEMENTS_H

#include "ecmascript/global_env_constants.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/c_containers.h"

namespace panda::ecmascript {
enum class ElementsKind : uint8_t {
    NONE = 0x00UL,
    HOLE = 0x01UL,
    INT = 0x1UL << 1,      // 2
    DOUBLE = 0x1UL << 2,   // 4
    NUMBER = INT | DOUBLE, // 6
    STRING = 0x1UL << 3,   // 8
    OBJECT = 0x1UL << 4,   // 16
    TAGGED = 0x1EUL,       // 30
    HOLE_INT = HOLE | INT,
    HOLE_DOUBLE = HOLE | DOUBLE,
    HOLE_NUMBER = HOLE | NUMBER,
    HOLE_STRING = HOLE | STRING,
    HOLE_OBJECT = HOLE | OBJECT,
    HOLE_TAGGED = HOLE | TAGGED,
    GENERIC = HOLE_TAGGED,
};

class Elements {
public:
    static CMap<ElementsKind, ConstantIndex> InitializeHClassMap();

    static std::string GetString(ElementsKind kind);
    static bool IsInt(ElementsKind kind);
    static bool IsDouble(ElementsKind kind);
    static bool IsObject(ElementsKind kind);
    static bool IsHole(ElementsKind kind);
    static bool IsGeneric(ElementsKind kind)
    {
        return kind == ElementsKind::GENERIC;
    }

    static bool IsNone(ElementsKind kind)
    {
        return kind == ElementsKind::NONE;
    }

    static ElementsKind MergeElementsKind(ElementsKind curKind, ElementsKind newKind);
    static ElementsKind FixElementsKind(ElementsKind oldKind);
    static ElementsKind ToElementsKind(JSTaggedValue value, ElementsKind kind);
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_ELEMENTS_H
