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

#ifndef ECMASCRIPT_PLATFORM_ECMA_CONTEXT_H
#define ECMASCRIPT_PLATFORM_ECMA_CONTEXT_H

#include "ecmascript/ecma_context.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {
    Expected<JSTaggedValue, bool> GetPendingExceptionResult([[maybe_unused]] JSTaggedValue result);
    void CheckHasPendingException([[maybe_unused]] EcmaContext *ecmaContext, [[maybe_unused]] JSThread *thread);
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_PLATFORM_ECMA_CONTEXT_H