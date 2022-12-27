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

#include "ecmascript/async_generator_helper.h"
#include "ecmascript/builtins/builtins_promise.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/tagged_queue.h"

namespace panda::ecmascript {
using BuiltinsPromise = builtins::BuiltinsPromise;
JSTaggedValue AsyncGeneratorHelper::Next(JSThread *thread, const JSHandle<GeneratorContext> &genContext,
                                         JSTaggedValue value)
{
    JSHandle<JSAsyncGeneratorObject> genObject(thread, genContext->GetGeneratorObject());
    genObject->SetResumeResult(thread, value);
    genObject->SetAsyncGeneratorState(JSAsyncGeneratorState::EXECUTING);
    genObject->SetResumeMode(AsyncGeneratorResumeMode::NEXT);

    EcmaInterpreter::GeneratorReEnterInterpreter(thread, genContext);
    if (genObject->IsSuspendYield()) {
        return JSTaggedValue::Undefined();
    }
    genObject->SetAsyncGeneratorState(JSAsyncGeneratorState::COMPLETED);
    JSHandle<TaggedQueue> queue(thread, genObject->GetAsyncGeneratorQueue());
    JSHandle<JSTaggedValue> next(thread, value);
    if (!(queue->Empty())) {
        JSAsyncGeneratorObject::AsyncGeneratorResolve(thread, genObject, next, true);
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue AsyncGeneratorHelper::Throw(JSThread *thread, const JSHandle<GeneratorContext> &genContext,
                                          const JSHandle<CompletionRecord> completionRecord)
{
    JSHandle<JSAsyncGeneratorObject> genObject(thread, genContext->GetGeneratorObject());
    genObject->SetResumeResult(thread, completionRecord->GetValue());
    genObject->SetAsyncGeneratorState(JSAsyncGeneratorState::EXECUTING);
    genObject->SetResumeMode(AsyncGeneratorResumeMode::THROW);

    EcmaInterpreter::GeneratorReEnterInterpreter(thread, genContext);

    genObject->SetAsyncGeneratorState(JSAsyncGeneratorState::COMPLETED);

    JSHandle<JSTaggedValue> val(thread, completionRecord->GetValue());
    JSHandle<TaggedQueue> queue(thread, genObject->GetAsyncGeneratorQueue());
    if (!(queue->Empty())) {
        JSAsyncGeneratorObject::AsyncGeneratorReject(thread, genObject, val);
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue AsyncGeneratorHelper::Return(JSThread *thread, const JSHandle<GeneratorContext> &genContext,
                                           const JSHandle<CompletionRecord> completionRecord)
{
    JSHandle<JSTaggedValue> val(thread, completionRecord->GetValue());
    JSHandle<JSAsyncGeneratorObject> genObject(thread, genContext->GetGeneratorObject());

    genObject->SetResumeResult(thread, val);
    if (genObject->IsSuspendYield() || genObject->IsExecuting()) {
        genObject->SetAsyncGeneratorState(JSAsyncGeneratorState::COMPLETED);
    }
    genObject->SetAsyncGeneratorState(JSAsyncGeneratorState::AWAITING_RETURN);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSTaggedValue promise =
        JSAsyncGeneratorObject::PromiseResolve(thread, JSHandle<JSTaggedValue>::Cast(env->GetPromiseFunction()), val);
    JSHandle<JSPromise> handPromise(thread, promise);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAsyncGeneratorResNextRetProRstFtn> onFulfilled =
        factory->NewJSAsyGenResNextRetProRstFulfilledFtn();
    onFulfilled->SetAsyncGeneratorObject(thread, genObject);

    JSHandle<JSAsyncGeneratorResNextRetProRstFtn> onFulRejected =
        factory->NewJSAsyGenResNextRetProRstRejectedFtn();
    onFulRejected->SetAsyncGeneratorObject(thread, genObject);
    JSHandle<PromiseCapability> tcap =
        JSPromise::NewPromiseCapability(thread, JSHandle<JSTaggedValue>::Cast(env->GetPromiseFunction()));
    [[maybe_unused]] JSTaggedValue pres = BuiltinsPromise::PerformPromiseThen(
        thread, handPromise, JSHandle<JSTaggedValue>::Cast(onFulfilled),
        JSHandle<JSTaggedValue>::Cast(onFulRejected), tcap);

    return JSTaggedValue::Undefined();
}
}  // namespace panda::ecmascript
