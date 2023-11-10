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

#ifndef MAPLE_UTIL_INCLUDE_SAFE_CAST_H
#define MAPLE_UTIL_INCLUDE_SAFE_CAST_H
#include "utils/meta.h"

namespace maple {
template <typename ToT>
struct SafeCastCondition : public std::false_type {
};

#define REGISTER_SAFE_CAST(type, condition)                  \
    template <>                                              \
    struct SafeCastCondition<type> : public std::true_type { \
        template <typename FromT>                            \
        static inline bool DoIt(const FromT &from)           \
        {                                                    \
            return (condition);                              \
        }                                                    \
    }

namespace impl {
template <typename ToT, typename FromT, typename = std::enable_if_t<std::is_base_of<FromT, ToT>::value>>
struct InstanceOfImpl {
    static inline bool DoIt(const FromT &from)
    {
        return (SafeCastCondition<ToT>::DoIt(from));
    }
};

template <typename ToT, typename FromT>
struct InstanceOfImpl<ToT, FromT, typename std::enable_if_t<std::is_base_of<ToT, FromT>::value>> {
    static inline bool DoIt(const FromT &)
    {
        return true;
    }
};

template <typename ToT, typename FromT>
struct EnabledSafeCast : public utils::meta_or<std::is_base_of<ToT, FromT>, SafeCastCondition<ToT>>::type {
};
}  // namespace impl

template <typename ToT, typename FromT, typename = std::enable_if_t<impl::EnabledSafeCast<ToT, FromT>::value>>
inline bool instance_of(FromT &from)
{
    return impl::InstanceOfImpl<ToT, FromT>::DoIt(from);
}

template <typename ToT, typename FromT, typename = std::enable_if_t<impl::EnabledSafeCast<ToT, FromT>::value>>
inline bool instance_of(FromT *from)
{
    return (from != nullptr && instance_of<ToT>(*from));
}

template <typename ToT, typename FromT,
          typename RetT = std::conditional_t<
              std::is_const<FromT>::value || std::is_const<std::remove_pointer_t<ToT>>::value,
              std::add_pointer_t<std::add_const_t<std::remove_cv_t<ToT>>>, std::add_pointer_t<std::remove_cv_t<ToT>>>,
          typename = std::enable_if_t<impl::EnabledSafeCast<ToT, FromT>::value>>
inline RetT safe_cast(FromT &from)
{
    return (instance_of<ToT>(from) ? static_cast<RetT>(&from) : nullptr);
}

template <typename ToT, typename FromT,
          typename RetT = std::conditional_t<
              std::is_const<FromT>::value || std::is_const<std::remove_pointer_t<ToT>>::value,
              std::add_pointer_t<std::add_const_t<std::remove_cv_t<ToT>>>, std::add_pointer_t<std::remove_cv_t<ToT>>>,
          typename = std::enable_if_t<impl::EnabledSafeCast<ToT, FromT>::value>>
inline RetT safe_cast(FromT *from)
{
    return (instance_of<ToT>(from) ? static_cast<RetT>(from) : nullptr);
}
}  // namespace maple
#endif  // MAPLE_UTIL_INCLUDE_SAFE_CAST_H