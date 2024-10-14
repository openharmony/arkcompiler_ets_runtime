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

#include "ecmascript/js_async_generator_object.h"
#include "ecmascript/async_generator_helper.h"
#include "ecmascript/builtins/builtins_promise.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_iterator.h"


namespace panda::ecmascript {
using BuiltinsPromise = builtins::BuiltinsPromise;

// AsyncGeneratorValidate ( generator, generatorBrand )
void JSAsyncGeneratorObject::AsyncGeneratorValidate(JSThread *thread, const JSHandle<JSTaggedValue> &gen,
                                                    const JSTaggedValue &val)
{
    // 1. Perform ? RequireInternalSlot(generator, [[AsyncGeneratorContext]]).
    // 2. Perform ? RequireInternalSlot(generator, [[AsyncGeneratorState]]).
    // 3. Perform ? RequireInternalSlot(generator, [[AsyncGeneratorQueue]]).
    if (!gen->IsAsyncGeneratorObject()) {
        THROW_TYPE_ERROR(thread, "Not a asyncgenerator object");
    }
    // 4. If generator.[[GeneratorBrand]] is not the same value as generatorBrand, throw a TypeError exception.
    JSHandle<JSObject> obj = JSTaggedValue::ToObject(thread, gen);
    RETURN_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSAsyncGeneratorObject> generator = JSHandle<JSAsyncGeneratorObject>::Cast(obj);
    if (!JSTaggedValue::SameValue(generator->GetGeneratorBrand(), val)) {
        THROW_TYPE_ERROR(thread, "Results are not equal");
    }
}

JSTaggedValue JSAsyncGeneratorObject::AsyncGeneratorResolve(JSThread *thread,
                                                            const JSHandle<JSAsyncGeneratorObject> &generator,
                                                            const JSHandle<JSTaggedValue> value, bool done)
{
    // 1. Assert: generator is an AsyncGenerator instance.
    ASSERT(generator->IsAsyncGeneratorObject());
    // 2. Let queue be generator.[[AsyncGeneratorQueue]].
    JSHandle<TaggedQueue> queue(thread, generator->GetAsyncGeneratorQueue());
    // 3. Assert: queue is not an empty List.
    ASSERT(!(queue->Empty()));
    // 4. Let next be the first element of queue.
    JSHandle<AsyncGeneratorRequest> next(thread, queue->Front());
    // 5. Remove the first element from queue.
    queue->Pop(thread);
    // 6. Let promiseCapability be next.[[Capability]].
    JSHandle<PromiseCapability> capability(thread, next->GetCapability());
    // 7. Let iteratorResult be ! CreateIterResultObject(value, done).
    JSHandle<JSObject> iteratorResult = JSIterator::CreateIterResultObject(thread, value, done);
    // 8. Perform ! Call(promiseCapability.[[Resolve]], undefined, « iteratorResult »).
    JSHandle<JSTaggedValue> its = JSHandle<JSTaggedValue>::Cast(iteratorResult);
    JSHandle<JSTaggedValue> resolve(thread, capability->GetResolve());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo* info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, resolve, undefined, undefined, 1, StackCheck::NO);
    info->SetCallArg(its.GetTaggedValue());
    [[maybe_unused]] JSTaggedValue res = JSFunction::Call(info);
    if ((thread)->HasPendingException()) {
        [[maybe_unused]] JSType errorType = thread->GetException().GetTaggedObject()->GetClass()->GetObjectType();
        ASSERT(errorType == JSType::JS_RANGE_ERROR);
        thread->ClearException();
        return JSTaggedValue::Undefined();
    }
    // 9. Perform ! AsyncGeneratorResumeNext(generator).
    AsyncGeneratorResumeNext(thread, generator);
    // 10. Return undefined.
    return JSTaggedValue::Undefined();
}

JSTaggedValue JSAsyncGeneratorObject::AsyncGeneratorReject(JSThread *thread,
                                                           const JSHandle<JSAsyncGeneratorObject> &generator,
                                                           const JSHandle<JSTaggedValue> value)
{
   // 1. Assert: generator is an AsyncGenerator instance.
    ASSERT(generator->IsAsyncGeneratorObject());
    // 2. Let queue be generator.[[AsyncGeneratorQueue]].
    JSHandle<TaggedQueue> queue(thread, generator->GetAsyncGeneratorQueue());
    // 3. Assert: queue is not an empty List.
    ASSERT(!(queue->Empty()));
    // 4. Let next be the first element of queue.
    JSHandle<JSTaggedValue> val(thread, queue->Front());
    JSHandle<AsyncGeneratorRequest> next = JSHandle<AsyncGeneratorRequest>::Cast(val);
    // 5. Remove the first element from queue.
    queue->Pop(thread);
    // 6. Let promiseCapability be next.[[Capability]].
    JSHandle<PromiseCapability> capability(thread, next->GetCapability());
    // 7. Perform ! Call(promiseCapability.[[Reject]], undefined, ? exception ?).
    JSHandle<JSTaggedValue> reject(thread, capability->GetReject());
    const GlobalEnvConstants *constants = thread->GlobalConstants();
    const JSHandle<JSTaggedValue> thisArg = constants->GetHandledUndefined();
    const JSHandle<JSTaggedValue> undefined = constants->GetHandledUndefined();
    EcmaRuntimeCallInfo* info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, reject, thisArg, undefined, 1);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    info->SetCallArg(value.GetTaggedValue());
    [[maybe_unused]] JSTaggedValue res = JSFunction::Call(info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 8. Perform ! AsyncGeneratorResumeNext(generator).
    AsyncGeneratorResumeNext(thread, generator);
    // 9. Return undefined.
    return JSTaggedValue::Undefined();
}

JSTaggedValue JSAsyncGeneratorObject::AsyncGeneratorResumeNext(JSThread *thread,
                                                               const JSHandle<JSAsyncGeneratorObject> &generator)
{
    // 1. Assert: generator is an AsyncGenerator instance.
    ASSERT(generator->IsAsyncGeneratorObject());
    // 2. Let state be generator.[[AsyncGeneratorState]].
    JSAsyncGeneratorState state = generator->GetAsyncGeneratorState();
    // 3. Assert: state is not executing.
    ASSERT(state != JSAsyncGeneratorState::EXECUTING);
    // 4. If state is awaiting-return, return undefined.
    if (state == JSAsyncGeneratorState::AWAITING_RETURN) {
        return JSTaggedValue::Undefined();
    }
    // 5. Let queue be generator.[[AsyncGeneratorQueue]].
    JSHandle<TaggedQueue> queue(thread, generator->GetAsyncGeneratorQueue());
    // 6. If queue is an empty List, return undefined.
    if (queue->Empty()) {
        return JSTaggedValue::Undefined();
    }
    // 7. Let next be the value of the first element of queue.
    JSHandle<AsyncGeneratorRequest> next(thread, queue->Front());
    // 8. Assert: next is an AsyncGeneratorRequest record.
    ASSERT(next->GetClass()->IsAsyncGeneratorRequest());
    // 9. Let completion be next.[[Completion]].
    JSTaggedValue rcd = next->GetCompletion();
    JSHandle<CompletionRecord> completion(thread, rcd);
    CompletionRecordType type = completion->GetType();
    // 10. If completion is an abrupt completion, then
    if (thread->HasPendingException() || type != CompletionRecordType::NORMAL) {
        // a. If state is suspendedStart, then
        if (state == JSAsyncGeneratorState::SUSPENDED_START) {
            // i. Set generator.[[AsyncGeneratorState]] to completed.
            // ii. Set state to completed.
            state = JSAsyncGeneratorState::COMPLETED;
            generator->SetAsyncGeneratorState(state);
        }
        // b. If state is completed, then
        if (state == JSAsyncGeneratorState::COMPLETED) {
            // i. If completion.[[Type]] is return, then
            if (completion->GetType() == CompletionRecordType::RETURN) {
                // 1. Set generator.[[AsyncGeneratorState]] to awaiting-return.
                generator->SetAsyncGeneratorState(JSAsyncGeneratorState::AWAITING_RETURN);
                // 2. Let promise be ? PromiseResolve(%Promise%, completion.[[Value]]).
                JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
                JSHandle<JSTaggedValue> val(thread, completion->GetValue());
                JSTaggedValue promise = PromiseResolve(thread,
                                                       JSHandle<JSTaggedValue>::Cast(env->GetPromiseFunction()), val);
                JSHandle<JSPromise> handPromise(thread, promise);
                // 3. Let stepsFulfilled be the algorithm steps defined in
                //    AsyncGeneratorResumeNext Return Processor Fulfilled Functions.
                // 4. Let lengthFulfilled be the number of non-optional parameters of the
                //    function definition in AsyncGeneratorResumeNext Return Processor Fulfilled Functions.
                // 5. Let onFulfilled be ! CreateBuiltinFunction(stepsFulfilled,
                //    lengthFulfilled, "", « [[Generator]] »).
                // 7. Let stepsRejected be the algorithm steps defined in AsyncGeneratorResumeNext
                //    Return Processor Rejected Functions.
                // 8. Let lengthRejected be the number of non-optional parameters of the function definition in
                //    AsyncGeneratorResumeNext Return Processor Rejected Functions.
                // 9. Let onRejected be ! CreateBuiltinFunction(stepsRejected, lengthRejected, "", « [[Generator]] »).
                ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
                JSHandle<JSAsyncGeneratorResNextRetProRstFtn> onFulfilled =
                    factory->NewJSAsyGenResNextRetProRstFulfilledFtn();
                onFulfilled->SetAsyncGeneratorObject(thread, generator);

                JSHandle<JSAsyncGeneratorResNextRetProRstFtn> onFulRejected =
                    factory->NewJSAsyGenResNextRetProRstRejectedFtn();
                onFulRejected->SetAsyncGeneratorObject(thread, generator);

                // 11. Perform ! PerformPromiseThen(promise, onFulfilled, onRejected).
                JSHandle<PromiseCapability> tcap =
                    JSPromise::NewPromiseCapability(thread, JSHandle<JSTaggedValue>::Cast(env->GetPromiseFunction()));
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                [[maybe_unused]] JSTaggedValue pres = BuiltinsPromise::PerformPromiseThen(
                    thread, handPromise, JSHandle<JSTaggedValue>::Cast(onFulfilled),
                    JSHandle<JSTaggedValue>::Cast(onFulRejected), tcap);
                // 12. Return undefined.
                return JSTaggedValue::Undefined();
            } else {
                // 1. Assert: completion.[[Type]] is throw.
                ASSERT(completion->GetType() == CompletionRecordType::THROW);
                // 2. Perform ! AsyncGeneratorReject(generator, completion.[[Value]]).
                JSHandle<JSTaggedValue> comVal(thread, completion->GetValue());
                AsyncGeneratorReject(thread, generator, comVal);
                // 3. Return undefined.
                return JSTaggedValue::Undefined();
            }
        }
    // 11. Else if state is completed, return ! AsyncGeneratorResolve(generator, undefined, true).
    } else if (state == JSAsyncGeneratorState::COMPLETED) {
        JSHandle<JSTaggedValue> comVal(thread, JSTaggedValue::Undefined());
        return AsyncGeneratorResolve(thread, generator, comVal, true);
    }
    // 12. Assert: state is either suspendedStart or suspendedYield.
    ASSERT((state == JSAsyncGeneratorState::SUSPENDED_START) ||
           (state == JSAsyncGeneratorState::SUSPENDED_YIELD));
    // 13. Let genContext be generator.[[AsyncGeneratorContext]].
    JSTaggedValue val = generator->GetGeneratorContext();

    JSHandle<GeneratorContext> genContext(thread, val);
    // 14. Let callerContext be the running execution context.
    // 15. Suspend callerContext.
    // 16. Set generator.[[AsyncGeneratorState]] to executing.
    // 17. Push genContext onto the execution context stack; genContext is now the running execution context.
    // 18. Resume the suspended evaluation of genContext using completion as the result of the operation that
    //     suspended it. Let result be the completion record returned by the resumed computation.
    // 19. Assert: result is never an abrupt completion.
    // 20. Assert: When we return here, genContext has already been removed from the execution context stack and
    //     callerContext is the currently running execution context.
    // 21. Return undefined.
    generator->SetAsyncGeneratorState(JSAsyncGeneratorState::EXECUTING);

    if (completion->GetType() == CompletionRecordType::NORMAL) {
        AsyncGeneratorHelper::Next(thread, genContext, completion->GetValue());
    }
    if (completion->GetType() == CompletionRecordType::RETURN) {
        AsyncGeneratorHelper::Return(thread, genContext, completion);
    }
    if (completion->GetType() == CompletionRecordType::THROW) {
        AsyncGeneratorHelper::Throw(thread, genContext, completion);
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue JSAsyncGeneratorObject::AsyncGeneratorEnqueue(JSThread *thread, const JSHandle<JSTaggedValue> &gen,
                                                            const JSHandle<CompletionRecord> completionRecord)
{
    // 1. Let promiseCapability be ! NewPromiseCapability(%Promise%).
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<PromiseCapability> pcap =
        JSPromise::NewPromiseCapability(thread, JSHandle<JSTaggedValue>::Cast(env->GetPromiseFunction()));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 2. Let check be AsyncGeneratorValidate(generator, generatorBrand).
    AsyncGeneratorValidate(thread, gen, JSTaggedValue::Undefined());
    // 3. If check is an abrupt completion, then
    if (thread->HasPendingException()) {
        thread->ClearException();
        // a. Let badGeneratorError be a newly created TypeError object.
        JSHandle<JSObject> resolutionError = factory->GetJSError(ErrorType::TYPE_ERROR,
            "Resolve: The promise and resolution cannot be the same.", StackCheck::NO);
        // b. Perform ! Call(promiseCapability.[[Reject]], undefined, « badGeneratorError »).
        const GlobalEnvConstants *constants = thread->GlobalConstants();
        JSHandle<JSTaggedValue> rstErr = JSHandle<JSTaggedValue>::Cast(resolutionError);
        JSHandle<JSTaggedValue> reject(thread, pcap->GetReject());
        JSHandle<JSTaggedValue> thisArg = constants->GetHandledUndefined();
        JSHandle<JSTaggedValue> undefined = constants->GetHandledUndefined();
        EcmaRuntimeCallInfo* info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, reject, thisArg, undefined, 1);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(rstErr.GetTaggedValue());
        [[maybe_unused]] JSTaggedValue res = JSFunction::Call(info);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

        // c. Return promiseCapability.[[Promise]].
        JSHandle<JSObject> promise(thread, pcap->GetPromise());
        return promise.GetTaggedValue();
    }
    // 4. Let queue be generator.[[AsyncGeneratorQueue]].
    JSHandle<JSObject> obj = JSTaggedValue::ToObject(thread, gen);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSAsyncGeneratorObject> generator = JSHandle<JSAsyncGeneratorObject>::Cast(obj);
    JSHandle<TaggedQueue> queue(thread, generator->GetAsyncGeneratorQueue());
    // 5. Let request be AsyncGeneratorRequest { [[Completion]]: completion, [[Capability]]: promiseCapability }.
    ObjectFactory *fty = thread->GetEcmaVM()->GetFactory();
    JSHandle<AsyncGeneratorRequest> asyncGeneratorRst = fty->NewAsyncGeneratorRequest();

    asyncGeneratorRst->SetCompletion(thread, completionRecord);
    asyncGeneratorRst->SetCapability(thread, pcap);
    // 6. Append request to the end of queue.

    TaggedQueue *newQueue = TaggedQueue::Push(thread, queue, JSHandle<JSTaggedValue>::Cast(asyncGeneratorRst));
    generator->SetAsyncGeneratorQueue(thread, JSTaggedValue(newQueue));

    // 7. Let state be generator.[[AsyncGeneratorState]].
    JSAsyncGeneratorState state = generator->GetAsyncGeneratorState();
    // 8. If state is not executing, then
    if (state != JSAsyncGeneratorState::EXECUTING) {
        // a. Perform ! AsyncGeneratorResumeNext(generator).
        AsyncGeneratorResumeNext(thread, generator);
    }
    // 9. Return promiseCapability.[[Promise]].
    JSHandle<JSObject> promise(thread, pcap->GetPromise());
    return promise.GetTaggedValue();
}

JSTaggedValue JSAsyncGeneratorObject::PromiseResolve(JSThread *thread, const JSHandle<JSTaggedValue> promise,
                                                     const JSHandle<JSTaggedValue> value)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ASSERT(promise->IsECMAObject());
    if (value->IsJSPromise()) {
        JSHandle<JSTaggedValue> ctorKey(globalConst->GetHandledConstructorString());
        JSHandle<JSTaggedValue> ctorValue = JSObject::GetProperty(thread, value, ctorKey).GetValue();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (JSTaggedValue::SameValue(ctorValue.GetTaggedValue(), promise.GetTaggedValue())) {
            return value.GetTaggedValue();
        }
    }
    JSHandle<PromiseCapability> promiseCapability = JSPromise::NewPromiseCapability(thread, promise);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> resolve(thread, promiseCapability->GetResolve());
    JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
    JSHandle<JSTaggedValue> thisArg = globalConst->GetHandledUndefined();
    EcmaRuntimeCallInfo* info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, resolve, thisArg, undefined, 1);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    info->SetCallArg(value.GetTaggedValue());
    [[maybe_unused]] JSTaggedValue res = JSFunction::Call(info);

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSPromise> promiseObj(thread, promiseCapability->GetPromise());
    return promiseObj.GetTaggedValue();
}

JSTaggedValue JSAsyncGeneratorObject::ProcessorFulfilledFunc(EcmaRuntimeCallInfo *argv)
{
    // 1. Let F be the active function object.
    JSThread *thread = argv->GetThread();
    JSHandle<JSAsyncGeneratorResNextRetProRstFtn> asyncResNextRtnPro =
        JSHandle<JSAsyncGeneratorResNextRetProRstFtn>::Cast(base::BuiltinsBase::GetConstructor(argv));
    JSHandle<JSAsyncGeneratorObject> asyncGen(thread, asyncResNextRtnPro->GetAsyncGeneratorObject());

    // 2. Set F.[[Generator]].[[AsyncGeneratorState]] to completed.
    asyncGen->SetAsyncGeneratorState(JSAsyncGeneratorState::COMPLETED);

    // 3. Return ! AsyncGeneratorResolve(F.[[Generator]], value, true).
    JSHandle<JSTaggedValue> value = base::BuiltinsBase::GetCallArg(argv, 0);
    return AsyncGeneratorResolve(thread, asyncGen, value, true);
}

JSTaggedValue JSAsyncGeneratorObject::ProcessorRejectedFunc(EcmaRuntimeCallInfo *argv)
{
    // 1. Let F be the active function object.
    JSThread *thread = argv->GetThread();
    JSHandle<JSAsyncGeneratorResNextRetProRstFtn> asyncResNextRtnPro =
        JSHandle<JSAsyncGeneratorResNextRetProRstFtn>::Cast(base::BuiltinsBase::GetConstructor(argv));
    JSHandle<JSAsyncGeneratorObject> asyncGen(thread, asyncResNextRtnPro->GetAsyncGeneratorObject());
    // 2. Set F.[[Generator]].[[AsyncGeneratorState]] to completed.
    asyncGen->SetAsyncGeneratorState(JSAsyncGeneratorState::COMPLETED);
    // 3. Return ! AsyncGeneratorReject(F.[[Generator]], reason).
    JSHandle<JSTaggedValue> value = base::BuiltinsBase::GetCallArg(argv, 0);
    return AsyncGeneratorReject(thread, asyncGen, value);
}
}  // namespace panda::ecmascript
