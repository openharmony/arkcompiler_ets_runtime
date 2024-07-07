/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_CONTAINERS_CONTAINERS_ERROR_H
#define ECMASCRIPT_CONTAINERS_CONTAINERS_ERROR_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"

namespace panda::ecmascript::containers {
enum ErrorFlag {
    TYPE_ERROR = 401,
    RANGE_ERROR = 10200001,
    IS_EMPTY_ERROR = 10200010,
    BIND_ERROR = 10200011,
    IS_NULL_ERROR = 10200012,
    IS_NOT_EXIST_ERROR = 10200017,
    CONCURRENT_MODIFICATION_ERROR = 10200201,
    REFERENCE_ERROR = 10200301,
};
class ContainerError {
public:
    static PUBLIC_API JSTaggedValue BusinessError(JSThread *thread, int32_t errorCode, const char *msg);
    static PUBLIC_API JSTaggedValue BindError(JSThread *thread, const char *msg);
    static PUBLIC_API JSTaggedValue ParamError(JSThread *thread, const char *msg);
    static PUBLIC_API JSTaggedValue ReferenceError(JSThread *thread, const char *msg);
};
} // namespace panda::ecmascript::containers
#endif // ECMASCRIPT_CONTAINERS_CONTAINERS_ERROR_H
