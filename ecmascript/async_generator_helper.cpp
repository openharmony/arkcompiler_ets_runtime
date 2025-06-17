/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/async_generator_helper.h"
#include "ecmascript/builtins/builtins_promise.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_async_generator_object.h"

namespace panda::ecmascript {
using BuiltinsPromise = builtins::BuiltinsPromise;
JSTaggedValue AsyncGeneratorHelper::Next(JSThread *thread, const JSHandle<GeneratorContext> &genContext,
                                         JSTaggedValue value)
{
    JSHandle<JSAsyncGeneratorObject> genObject(thread, genContext->GetGeneratorObject(thread));
    genObject->SetResumeResult(thread, value);
    genObject->SetResumeMode(AsyncGeneratorResumeMode::NEXT);

    EcmaInterpreter::GeneratorReEnterInterpreter(thread, genContext);
    return JSTaggedValue::Undefined();
}

JSTaggedValue AsyncGeneratorHelper::Throw(JSThread *thread, const JSHandle<GeneratorContext> &genContext,
                                          const JSHandle<CompletionRecord> completionRecord)
{
    JSHandle<JSAsyncGeneratorObject> genObject(thread, genContext->GetGeneratorObject(thread));
    genObject->SetResumeResult(thread, completionRecord->GetValue(thread));
    genObject->SetResumeMode(AsyncGeneratorResumeMode::THROW);

    EcmaInterpreter::GeneratorReEnterInterpreter(thread, genContext);
    return JSTaggedValue::Undefined();
}

JSTaggedValue AsyncGeneratorHelper::Return(JSThread *thread, const JSHandle<GeneratorContext> &genContext,
                                           const JSHandle<CompletionRecord> completionRecord)
{
    JSHandle<JSAsyncGeneratorObject> genObject(thread, genContext->GetGeneratorObject(thread));
    genObject->SetResumeResult(thread, completionRecord->GetValue(thread));
    genObject->SetResumeMode(AsyncGeneratorResumeMode::RETURN);

    EcmaInterpreter::GeneratorReEnterInterpreter(thread, genContext);
    return JSTaggedValue::Undefined();
}
}  // namespace panda::ecmascript
