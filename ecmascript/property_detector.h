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

#ifndef ECMASCRIPT_PROPERTY_DETECTOR_H
#define ECMASCRIPT_PROPERTY_DETECTOR_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"

namespace panda {
namespace ecmascript {
class GlobalEnv;
class PropertyDetector {
public:
#define GLOBAL_ENV_DETECTOR_FIELDS(V)                                                   \
    V(JSTaggedValue, RegExpReplaceDetector, REGEXP_REPLACE_DETECTOR_INDEX)              \
    V(JSTaggedValue, MapIteratorDetector, MAP_ITERATOR_DETECTOR_INDEX)                  \
    V(JSTaggedValue, SetIteratorDetector, SET_ITERATOR_DETECTOR_INDEX)                  \
    V(JSTaggedValue, StringIteratorDetector, STRING_ITERATOR_DETECTOR_INDEX)            \
    V(JSTaggedValue, ArrayIteratorDetector, ARRAY_ITERATOR_DETECTOR_INDEX)              \
    V(JSTaggedValue, TypedArrayIteratorDetector, TYPED_ARRAY_ITERATOR_DETECTOR_INDEX)   \
    V(JSTaggedValue, TypedArraySpeciesProtectDetector, TYPED_ARRAY_SPECIES_PROTECT_DETECTOR_INDEX)    \
    V(JSTaggedValue, NumberStringNotRegexpLikeDetector, NUMBER_STRING_NOT_REGEXP_LIKE_DETECTOR_INDEX) \
    V(JSTaggedValue, RegExpFlagsDetector, REGEXP_FLAGS_DETECTOR_INDEX)                                \
    V(JSTaggedValue, RegExpSpeciesDetector, REGEXP_SPECIES_DETECTOR_INDEX)

#define DECLARE_DETECTOR(type, name, index)                        \
    static inline bool Is##name##Valid(JSHandle<GlobalEnv> env);   \
    static inline void Invalidate##name(JSHandle<GlobalEnv> env);
    GLOBAL_ENV_DETECTOR_FIELDS(DECLARE_DETECTOR)
#undef DECLARE_DETECTOR

#define DETECTOR_SYMBOL_LIST(V)                      \
    V(ReplaceSymbol,  "Symbol.replace",   replace )  \
    V(SplitSymbol,    "Symbol.split",     split   )  \
    V(MatchAllSymbol, "Symbol.matchAll",  matchAll)  \
    V(IteratorSymbol, "Symbol.iterator",  iterator)  \
    V(SpeciesSymbol,  "Symbol.species",   species)
};

}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_PROPERTY_DETECTOR_H
