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

#ifndef ECMASCRIPT_NAPI_INTERNALS_INCLUDE_JSNAPI_H
#define ECMASCRIPT_NAPI_INTERNALS_INCLUDE_JSNAPI_H

#include <cstdint>

#include "ecmascript/js_tagged_value_internals.h"

namespace panda {
enum class PatchErrorCode : uint8_t {
    SUCCESS = 0,
    PATCH_HAS_LOADED,
    PATCH_NOT_LOADED,
    FILE_NOT_EXECUTED,
    FILE_NOT_FOUND,
    PACKAGE_NOT_ESMODULE,
    MODIFY_IMPORT_EXPORT_NOT_SUPPORT,
    INTERNAL_ERROR
};

using JSValueRefInternals = ecmascript::JSTaggedValueInternals;
using NativePointerCallback = void (*)(void *env, void* data, void* hint);

} // namespace panda

#endif // ECMASCRIPT_NAPI_INTERNALS_INCLUDE_JSNAPI_H