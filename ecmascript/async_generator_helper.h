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

#ifndef ECMASCRIPT_ASYNC_GENERATOR_HELPER_H
#define ECMASCRIPT_ASYNC_GENERATOR_HELPER_H

#include "ecmascript/js_generator_object.h"

namespace panda::ecmascript {
class AsyncGeneratorHelper {
public:
    static JSTaggedValue Next(JSThread *thread, const JSHandle<GeneratorContext> &genContext,
                              const JSTaggedValue value);
    static JSTaggedValue Throw(JSThread *thread, const JSHandle<GeneratorContext> &genContext,
                               const JSHandle<CompletionRecord> completionRecord);
    static JSTaggedValue Return(JSThread *thread, const JSHandle<GeneratorContext> &genContext,
                                const JSHandle<CompletionRecord> completionRecord);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_ASYNC_GENERATOR_HELPER_H
