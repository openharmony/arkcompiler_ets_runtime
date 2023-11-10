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

#ifndef MAPLE_UTIL_INCLUDE_UTILS_META_H
#define MAPLE_UTIL_INCLUDE_UTILS_META_H
#include <type_traits>

namespace maple {
namespace utils {
template <typename T, typename U>
struct meta_and : public std::conditional_t<T::value, U, T> {
};

template <typename T, typename U>
struct meta_or : public std::conditional_t<T::value, T, U> {
};

template <typename T>
struct meta_not : public std::integral_constant<bool, !static_cast<bool>(T::value)>::type {
};

template <typename...>
struct is_signed;

template <typename T>
struct is_signed<T> : public std::is_signed<T>::type {
};

template <typename T, typename U>
struct is_signed<T, U> : public meta_and<std::is_signed<T>, std::is_signed<U>>::type {
};

template <typename... T>
constexpr bool is_signed_v = is_signed<T...>::value;

template <typename...>
struct is_unsigned;

template <typename T>
struct is_unsigned<T> : public std::is_unsigned<T>::type {
};

template <typename T, typename U>
struct is_unsigned<T, U> : public meta_and<std::is_unsigned<T>, std::is_unsigned<U>>::type {
};

template <typename... T>
constexpr bool is_unsigned_v = is_unsigned<T...>::value;

template <typename T, typename U>
struct is_same_sign : public meta_or<is_signed<T, U>, is_unsigned<T, U>>::type {
};

template <typename T, typename U>
struct is_diff_sign : public meta_not<is_same_sign<T, U>>::type {
};

template <typename...>
struct is_pointer;

template <typename T>
struct is_pointer<T> : public std::is_pointer<T>::type {
};

template <typename T, typename U>
struct is_pointer<T, U> : public meta_and<is_pointer<T>, is_pointer<U>>::type {
};

template <typename... T>
constexpr bool is_pointer_v = is_pointer<T...>::value;

template <typename T, typename U>
struct const_of : public meta_and<std::is_const<U>, std::is_same<std::add_const_t<T>, U>>::type {
};

template <typename T, typename U>
constexpr bool const_of_v = const_of<T, U>::value;

template <typename T, typename U>
struct is_ncv_same : public std::is_same<std::remove_cv_t<T>, std::remove_cv_t<U>>::type {
};

template <typename T, typename U>
constexpr bool is_ncv_same_v = is_ncv_same<T, U>::value;

namespace ptr {
template <typename T, typename U, typename = std::enable_if_t<is_pointer_v<T, U>>>
struct const_of : public utils::const_of<std::remove_pointer_t<T>, std::remove_pointer_t<U>>::type {
};

template <typename T, typename U, typename = std::enable_if_t<is_pointer_v<T, U>>>
constexpr bool const_of_v = const_of<T, U>::value;

template <typename T, typename U, typename = std::enable_if_t<is_pointer_v<T, U>>>
struct is_ncv_same : public utils::is_ncv_same<std::remove_pointer_t<T>, std::remove_pointer_t<U>>::type {
};

template <typename T, typename U, typename = std::enable_if_t<is_pointer_v<T, U>>>
constexpr bool is_ncv_same_v = is_ncv_same<T, U>::value;
}  // namespace ptr
}  // namespace utils
}  // namespace maple
#endif  // MAPLE_UTIL_INCLUDE_UTILS_META_H
