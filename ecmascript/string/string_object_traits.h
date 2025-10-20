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

// NOLINTBEGIN(readability-identifier-naming, cppcoreguidelines-macro-usage,
//             cppcoreguidelines-special-member-functions, modernize-deprecated-headers,
//             readability-else-after-return, readability-duplicate-include,
//             misc-non-private-member-variables-in-classes, cppcoreguidelines-pro-type-member-init,
//             google-explicit-constructor, cppcoreguidelines-pro-type-union-access,
//             modernize-use-auto, llvm-namespace-comment,
//             cppcoreguidelines-pro-type-vararg, modernize-avoid-c-arrays,
//             readability-implicit-bool-conversion)

#ifndef ECMASCRIPT_STRING_STRING_OBJECTS_TRAITS_H
#define ECMASCRIPT_STRING_STRING_OBJECTS_TRAITS_H

#include <type_traits>
#include <vector>
#include "ecmascript/string/base_string_class.h"
#include "objects/utils/objects_traits.h"

namespace panda::objects_traits {
// Allocator: U (size_t, CommonType)
template <typename F, typename U>
constexpr bool is_allocate_callable_v =
    common::objects_traits::is_heap_object_v<U> &&
        std::is_invocable_r_v<U, F, size_t, panda::ecmascript::EcmaStringType>;

template <typename F, typename U>
using enable_if_is_allocate = std::enable_if_t<is_allocate_callable_v<F, U>, int>;

} // namespace panda::objects_traits


#endif //ECMASCRIPT_STRING_STRING_OBJECTS_TRAITS_H
// NOLINTEND(readability-identifier-naming, cppcoreguidelines-macro-usage,
//           cppcoreguidelines-special-member-functions, modernize-deprecated-headers,
//           readability-else-after-return, readability-duplicate-include,
//           misc-non-private-member-variables-in-classes, cppcoreguidelines-pro-type-member-init,
//           google-explicit-constructor, cppcoreguidelines-pro-type-union-access,
//           modernize-use-auto, llvm-namespace-comment,
//           cppcoreguidelines-pro-type-vararg, modernize-avoid-c-arrays,
//           readability-implicit-bool-conversion)