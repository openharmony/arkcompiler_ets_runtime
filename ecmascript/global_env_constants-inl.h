/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_GLOBAL_ENV_CONSTANTS_INL_H
#define ECMASCRIPT_GLOBAL_ENV_CONSTANTS_INL_H

#include "ecmascript/global_env_constants.h"

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_handle.h"

namespace panda::ecmascript {
inline const JSTaggedValue *GlobalEnvConstants::BeginSlot() const
{
    return &constants_[static_cast<int>(ConstantIndex::CONSTANT_BEGIN)];
}

inline const JSTaggedValue *GlobalEnvConstants::EndSlot() const
{
    return &constants_[static_cast<int>(ConstantIndex::CONSTANT_END)];
}

inline void GlobalEnvConstants::SetConstant(ConstantIndex index, JSTaggedValue value)
{
    DASSERT_PRINT(index >= ConstantIndex::CONSTANT_BEGIN && index < ConstantIndex::CONSTANT_END,
                  "Root Index out of bound");
    constants_[static_cast<int>(index)] = value;
}

template<typename T>
inline void GlobalEnvConstants::SetConstant(ConstantIndex index, JSHandle<T> value)
{
    DASSERT_PRINT(index >= ConstantIndex::CONSTANT_BEGIN && index < ConstantIndex::CONSTANT_END,
                  "Root Index out of bound");
    constants_[static_cast<int>(index)] = value.GetTaggedValue();
}

inline uintptr_t GlobalEnvConstants::GetGlobalConstantAddr(ConstantIndex index) const
{
    return ToUintPtr(this) + sizeof(JSTaggedValue) * static_cast<uint32_t>(index);
}

// clang-format off
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECL_GET_IMPL_COMMON(Type, Name, Index)                              \
    inline const Type GlobalEnvConstants::Get##Name() const                  \
    {                                                                        \
        return constants_[static_cast<int>(ConstantIndex::Index)];           \
    }                                                                        \
    inline const JSHandle<Type> GlobalEnvConstants::GetHandled##Name() const \
    {                                                                        \
        return JSHandle<Type>(reinterpret_cast<uintptr_t>(                   \
            &constants_[static_cast<int>(ConstantIndex::Index)]));           \
    }                                                                        \
    inline size_t GlobalEnvConstants::GetOffsetOf##Name()                    \
    {                                                                        \
        return sizeof(JSTaggedValue)                                         \
            * static_cast<int>(ConstantIndex::Index);                        \
    }

#define DECL_GET_IMPL_BUILTIN4(Method, Object, Unused, Index)                                 \
    DECL_GET_IMPL_COMMON(JSTaggedValue, Object##Method, Index##_INDEX)
#define DECL_GET_IMPL_BUILTIN6(Method, Object, Unused0, Unused1, Unused2, Index)              \
    DECL_GET_IMPL_COMMON(JSTaggedValue, Object##Method, Index##_INDEX)
#define DECL_GET_IMPL_STRING(Name, Index, Token) DECL_GET_IMPL_COMMON(JSTaggedValue, Name, Index)
#define DECL_GET_IMPL_WITH_TYPE(Type, Name, Index, Desc) DECL_GET_IMPL_COMMON(Type, Name, Index)
    SHARED_GLOBAL_ENV_CONSTANT_CLASS(DECL_GET_IMPL_WITH_TYPE)     // NOLINT(readability-const-return-type)
    SHARED_GLOBAL_ENV_CONSTANT_STRING(DECL_GET_IMPL_STRING)       // NOLINT(readability-const-return-type)
    SHARED_GLOBAL_ENV_CONSTANT_ACCESSOR(DECL_GET_IMPL_WITH_TYPE)  // NOLINT(readability-const-return-type)
    SHARED_GLOBAL_ENV_CONSTANT_SPECIAL(DECL_GET_IMPL_WITH_TYPE);  // NOLINT(readability-const-return-type)
    GLOBAL_ENV_CONSTANT_CLASS(DECL_GET_IMPL_WITH_TYPE)            // NOLINT(readability-const-return-type)
    GLOBAL_ENV_CONSTANT_SPECIAL(DECL_GET_IMPL_WITH_TYPE)          // NOLINT(readability-const-return-type)
    BUILTINS_METHOD_STUB_LIST(DECL_GET_IMPL_BUILTIN4, DECL_GET_IMPL_BUILTIN4,                 \
                              DECL_GET_IMPL_BUILTIN4, DECL_GET_IMPL_BUILTIN6)
    GLOBAL_ENV_INLINED_BUILTINS(DECL_GET_IMPL_WITH_TYPE)          // NOLINT(readability-const-return-type)
    GLOBAL_ENV_CONSTANT_CONSTANT(DECL_GET_IMPL_WITH_TYPE)         // NOLINT(readability-const-return-type)
    GLOBAL_ENV_CACHES(DECL_GET_IMPL_WITH_TYPE)                    // NOLINT(readability-const-return-type)
#undef DECL_GET_IMPL_WITH_TYPE
#undef DECL_GET_IMPL_STRING
#undef DECL_GET_IMPL_COMMON
#undef DECL_GET_IMPL_BUILTIN4
#undef DECL_GET_IMPL_BUILTIN6
// clang-format on
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_GLOBAL_ENV_CONSTANTS_INL_H
