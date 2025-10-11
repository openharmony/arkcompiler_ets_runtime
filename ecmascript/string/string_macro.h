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

#ifndef ECMASCRIPT_STRING_STRING_MACRO_H
#define ECMASCRIPT_STRING_STRING_MACRO_H

#include <type_traits>
#include <utility>
#include "objects/base_object.h"
#include "ecmascript/string/base_string_class.h"

// CC-OFFNXT(C_RULE_ID_DEFINE_LENGTH_LIMIT) solid logic
// CC-OFFNXT(G.PRE.02) code readability
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define STRING_CAST_CHECK(CAST_TYPE, CHECK_METHOD)                                              \
    static inline CAST_TYPE *Cast(BaseObject *object)                                           \
    {                                                                                           \
        /* CC-OFFNXT(G.PRE.02) code readability */                                              \
        DCHECK_CC(reinterpret_cast<BaseStringClass*>(object->GetBaseClass())->CHECK_METHOD());  \
        /* CC-OFFNXT(G.PRE.05) C_RULE_ID_KEYWORD_IN_DEFINE */                                   \
        /* CC-OFFNXT(G.PRE.02) code readability */                                              \
        return reinterpret_cast<CAST_TYPE *>(object);                                           \
    }                                                                                           \
    static const inline CAST_TYPE *ConstCast(const BaseObject *object)                          \
    {                                                                                           \
        /* CC-OFFNXT(G.PRE.02) code readability */                                              \
        DCHECK_CC(reinterpret_cast<BaseStringClass*>(object->GetBaseClass())->CHECK_METHOD());  \
        /* CC-OFFNXT(G.PRE.05) C_RULE_ID_KEYWORD_IN_DEFINE */                                   \
        /* CC-OFFNXT(G.PRE.02) code readability */                                              \
        return reinterpret_cast<const CAST_TYPE *>(object);                                     \
    }
#endif  // ECMASCRIPT_STRING_STRING_MACRO_H