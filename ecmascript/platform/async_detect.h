/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_PLATFORM_ASYNC_DETECT_H
#define ECMASCRIPT_PLATFORM_ASYNC_DETECT_H

#include <cstdint>
#include <string>
#if defined(PANDA_TARGET_OHOS) && !defined(STANDALONE_MODE) && !defined(CROSS_PLATFORM)
#include <uv.h>
#endif

#include "ecmascript/ecma_vm.h"

namespace panda::ecmascript {
void RegisterAsyncDetectCallBack(EcmaVM *vm);
#if defined(PANDA_TARGET_OHOS) && !defined(STANDALONE_MODE) && !defined(CROSS_PLATFORM)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
    static void AsyncDetectCallBack(uv_timer_t* handle);
#pragma GCC diagnostic pop
#endif
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_PLATFORM_ASYNC_DETECT_H