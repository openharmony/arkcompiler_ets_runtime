/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/builtins/builtins_promise_handler.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_async_function.h"
#include "ecmascript/js_promise.h"

namespace panda::ecmascript::builtins {
// es6 25.4.1.3.2 Promise Resolve Functions
JSTaggedValue BuiltinsPromiseHandler::Resolve(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), PromiseHandler, Resolve);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Assert: F has a [[Promise]] internal slot whose value is an Object.
    JSHandle<JSPromiseReactionsFunction> resolve = JSHandle<JSPromiseReactionsFunction>::Cast(GetConstructor(argv));
    ASSERT_PRINT(resolve->GetPromise(thread).IsECMAObject(), "Resolve: promise must be js object");

    // 2. Let promise be the value of F's [[Promise]] internal slot.
    // 3. Let alreadyResolved be the value of F's [[AlreadyResolved]] internal slot.
    // 4. If alreadyResolved.[[value]] is true, return undefined.
    // 5. Set alreadyResolved.[[value]] to true.
    JSHandle<PromiseRecord> alreadyResolved(thread, resolve->GetAlreadyResolved(thread));
    if (alreadyResolved->GetValue(thread).IsTrue()) {
        return JSTaggedValue::Undefined();
    }
    alreadyResolved->SetValue(thread, JSTaggedValue::True());
    JSHandle<JSPromise> resolvePromise(thread, resolve->GetPromise(thread));
    JSHandle<JSTaggedValue> resolution = BuiltinsBase::GetCallArg(argv, 0);
    return InnerResolve(thread, resolvePromise, resolution);
}

// es6 25.4.1.3.1 Promise Reject Functions
JSTaggedValue BuiltinsPromiseHandler::Reject(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), PromiseHandler, Reject);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Assert: F has a [[Promise]] internal slot whose value is an Object.
    JSHandle<JSPromiseReactionsFunction> reject = JSHandle<JSPromiseReactionsFunction>::Cast(GetConstructor(argv));
    ASSERT_PRINT(reject->GetPromise(thread).IsECMAObject(), "Reject: promise must be js object");

    // 2. Let promise be the value of F's [[Promise]] internal slot.
    // 3. Let alreadyResolved be the value of F's [[AlreadyResolved]] internal slot.
    // 4. If alreadyResolved.[[value]] is true, return undefined.
    // 5. Set alreadyResolved.[[value]] to true.
    JSHandle<JSPromise> rejectPromise(thread, reject->GetPromise(thread));
    JSHandle<PromiseRecord> alreadyResolved(thread, reject->GetAlreadyResolved(thread));
    if (alreadyResolved->GetValue(thread).IsTrue()) {
        return JSTaggedValue::Undefined();
    }
    alreadyResolved->SetValue(thread, JSTaggedValue::True());

    // 6. Return RejectPromise(promise, reason).
    JSHandle<JSTaggedValue> reason = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> result(thread, JSPromise::RejectPromise(thread, rejectPromise, reason));
    return result.GetTaggedValue();
}

// es6 25.4.1.5.1 GetCapabilitiesExecutor Functions
JSTaggedValue BuiltinsPromiseHandler::Executor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), PromiseHandler, Executor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Assert: F has a [[Capability]] internal slot whose value is a PromiseCapability Record.
    JSHandle<JSPromiseExecutorFunction> executor = JSHandle<JSPromiseExecutorFunction>::Cast(GetConstructor(argv));
    ASSERT_PRINT(executor->GetCapability(thread).IsRecord(),
                 "Executor: F has a [[Capability]] internal slot whose value is a PromiseCapability Record.");

    // 2. Let promiseCapability be the value of F's [[Capability]] internal slot.
    // 3. If promiseCapability.[[Resolve]] is not undefined, throw a TypeError exception.
    JSHandle<PromiseCapability> promiseCapability(thread, executor->GetCapability(thread));
    if (!promiseCapability->GetResolve(thread).IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Executor: resolve should be undefine!", JSTaggedValue::Undefined());
    }
    // 4. If promiseCapability.[[Reject]] is not undefined, throw a TypeError exception.
    if (!promiseCapability->GetReject(thread).IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Executor: reject should be undefine!", JSTaggedValue::Undefined());
    }
    // 5. Set promiseCapability.[[Resolve]] to resolve.
    // 6. Set promiseCapability.[[Reject]] to reject.
    JSHandle<JSTaggedValue> resolve = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> reject = GetCallArg(argv, 1);
    promiseCapability->SetResolve(thread, resolve);
    promiseCapability->SetReject(thread, reject);
    // 7. Return undefined.
    return JSTaggedValue::Undefined();
}

// es6 25.4.4.1.2 Promise.all Resolve Element Functions
JSTaggedValue BuiltinsPromiseHandler::ResolveElementFunction(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), PromiseHandler, ResolveElementFunction);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSPromiseAllResolveElementFunction> func =
        JSHandle<JSPromiseAllResolveElementFunction>::Cast(GetConstructor(argv));
    // 1. Let alreadyCalled be the value of F's [[AlreadyCalled]] internal slot.
    JSHandle<PromiseRecord> alreadyCalled =
        JSHandle<PromiseRecord>::Cast(JSHandle<JSTaggedValue>(thread, func->GetAlreadyCalled(thread)));
    // 2. If alreadyCalled.[[value]] is true, return undefined.
    if (alreadyCalled->GetValue(thread).IsTrue()) {
        return JSTaggedValue::Undefined();
    }
    // 3. Set alreadyCalled.[[value]] to true.
    alreadyCalled->SetValue(thread, JSTaggedValue::True());
    // 4. Let index be the value of F's [[Index]] internal slot.
    JSHandle<JSTaggedValue> index(thread, func->GetIndex(thread));
    // 5. Let values be the value of F's [[Values]] internal slot.
    JSHandle<PromiseRecord> values =
        JSHandle<PromiseRecord>::Cast(JSHandle<JSTaggedValue>(thread, func->GetValues(thread)));
    // 6. Let promiseCapability be the value of F's [[Capabilities]] internal slot.
    JSHandle<PromiseCapability> capa =
        JSHandle<PromiseCapability>::Cast(JSHandle<JSTaggedValue>(thread, func->GetCapabilities(thread)));
    // 7. Let remainingElementsCount be the value of F's [[RemainingElements]] internal slot.
    JSHandle<PromiseRecord> remainCnt =
        JSHandle<PromiseRecord>::Cast(JSHandle<JSTaggedValue>(thread, func->GetRemainingElements(thread)));
    // 8. Set values[index] to x.
    JSHandle<TaggedArray> arrayValues =
        JSHandle<TaggedArray>::Cast(JSHandle<JSTaggedValue>(thread, values->GetValue(thread)));
    arrayValues->Set(thread, JSTaggedValue::ToUint32(thread, index), GetCallArg(argv, 0).GetTaggedValue());
    // 9. Set remainingElementsCount.[[value]] to remainingElementsCount.[[value]] - 1.
    remainCnt->SetValue(thread, --JSTaggedNumber(remainCnt->GetValue(thread)));
    // 10. If remainingElementsCount.[[value]] is 0,
    if (remainCnt->GetValue(thread).IsZero()) {
        // a. Let valuesArray be CreateArrayFromList(values).
        JSHandle<JSArray> jsArrayValues = JSArray::CreateArrayFromList(thread, arrayValues);
        // b. Return Call(promiseCapability.[[Resolve]], undefined, «valuesArray»).
        JSHandle<JSTaggedValue> capaResolve(thread, capa->GetResolve(thread));
        JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, capaResolve, undefined, undefined, 1);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(jsArrayValues.GetTaggedValue());
        return JSFunction::Call(info);
    }
    // 11. Return undefined.
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsPromiseHandler::AsyncAwaitFulfilled(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), PromiseHandler, AsyncAwaitFulfilled);
    [[maybe_unused]] EcmaHandleScope handleScope(argv->GetThread());

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSHandle<JSAsyncAwaitStatusFunction> func(GetConstructor(argv));
    return JSAsyncAwaitStatusFunction::AsyncFunctionAwaitFulfilled(argv->GetThread(), func, value).GetTaggedValue();
}

JSTaggedValue BuiltinsPromiseHandler::AsyncAwaitRejected(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), PromiseHandler, AsyncAwaitRejected);
    [[maybe_unused]] EcmaHandleScope handleScope(argv->GetThread());

    JSHandle<JSTaggedValue> reason = GetCallArg(argv, 0);
    JSHandle<JSAsyncAwaitStatusFunction> func(GetConstructor(argv));
    return JSAsyncAwaitStatusFunction::AsyncFunctionAwaitRejected(argv->GetThread(), func, reason).GetTaggedValue();
}

JSTaggedValue BuiltinsPromiseHandler::valueThunkFunction(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), PromiseHandler, valueThunkFunction);
    JSHandle<JSPromiseValueThunkOrThrowerFunction> valueThunk =
        JSHandle<JSPromiseValueThunkOrThrowerFunction>::Cast(GetConstructor(argv));
    return valueThunk->GetResult(argv->GetThread());
}

JSTaggedValue BuiltinsPromiseHandler::throwerFunction(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PromiseHandler, throwerFunction);
    JSHandle<JSPromiseValueThunkOrThrowerFunction> thrower =
        JSHandle<JSPromiseValueThunkOrThrowerFunction>::Cast(GetConstructor(argv));
    JSTaggedValue undefined = thread->GlobalConstants()->GetUndefined();
    THROW_NEW_ERROR_AND_RETURN_VALUE(thread, thrower->GetResult(thread), undefined);
}

JSTaggedValue BuiltinsPromiseHandler::ThenFinally(EcmaRuntimeCallInfo *argv)
{
    // 1. Let F be the active function object.
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PromiseHandler, ThenFinally);
    auto ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSPromiseFinallyFunction> thenFinally(GetConstructor(argv));
    JSHandle<JSTaggedValue> value = BuiltinsBase::GetCallArg(argv, 0);
    // 2. Let onFinally be F.[[OnFinally]].
    // 3. Assert: IsCallable(onFinally) is true.
    JSHandle<JSTaggedValue> onFinally(thread, thenFinally->GetOnFinally(thread));
    ASSERT_PRINT(onFinally->IsCallable(), "onFinally is not callable");
    // 4. Let result be ? Call(onFinally, undefined).
    JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
    EcmaRuntimeCallInfo *taggedInfo =
        EcmaInterpreter::NewRuntimeCallInfo(thread, onFinally, undefined, undefined, 0);
    JSTaggedValue result = JSFunction::Call(taggedInfo);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> resultHandle(thread, result);
    // 5. Let C be F.[[Constructor]].
    // 6. Assert: IsConstructor(C) is true.
    JSHandle<JSTaggedValue> thenFinallyConstructor(thread, thenFinally->GetConstructor(thread));
    ASSERT_PRINT(thenFinallyConstructor->IsConstructor(), "thenFinallyConstructor is not constructor");
    // 7. Let promise be ? PromiseResolve(C, result).
    JSHandle<JSTaggedValue> promiseHandle =
        BuiltinsPromiseHandler::PromiseResolve(thread, thenFinallyConstructor, resultHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 8. Let valueThunk be equivalent to a function that returns value.
    JSHandle<JSPromiseValueThunkOrThrowerFunction> valueThunk =
        factory->NewJSPromiseValueThunkFunction();
    valueThunk->SetResult(thread, value);
    JSHandle<JSTaggedValue> thenKey(globalConst->GetHandledPromiseThenString());
    EcmaRuntimeCallInfo *invokeInfo =
        EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, promiseHandle, undefined, 1);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    invokeInfo->SetCallArg(valueThunk.GetTaggedValue());
    // 9. Return ? Invoke(promise, "then", « valueThunk »).
    return JSFunction::Invoke(invokeInfo, thenKey);
}

JSTaggedValue BuiltinsPromiseHandler::CatchFinally(EcmaRuntimeCallInfo *argv)
{
    // 1. Let F be the active function object.
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PromiseHandler, CatchFinally);
    auto ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSPromiseFinallyFunction> catchFinally(GetConstructor(argv));
    // 2. Let onFinally be F.[[OnFinally]].
    // 3. Assert: IsCallable(onFinally) is true.
    JSHandle<JSTaggedValue> onFinally(thread, catchFinally->GetOnFinally(thread));
    ASSERT_PRINT(onFinally->IsCallable(), "thenOnFinally is not callable");
    // 4. Let result be ? Call(onFinally, undefined).
    JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, onFinally, undefined, undefined, 0);
    JSTaggedValue result = JSFunction::Call(info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> resultHandle(thread, result);
    // 5. Let C be F.[[Constructor]].
    // 6. Assert: IsConstructor(C) is true.
    JSHandle<JSTaggedValue> catchFinallyConstructor(thread, catchFinally->GetConstructor(thread));
    ASSERT_PRINT(catchFinallyConstructor->IsConstructor(), "catchFinallyConstructor is not constructor");
    // 7. Let promise be ? PromiseResolve(C, result).
    JSHandle<JSTaggedValue> promiseHandle =
        BuiltinsPromiseHandler::PromiseResolve(thread, catchFinallyConstructor, resultHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 8. Let thrower be equivalent to a function that throws reason.
    JSHandle<JSTaggedValue> reason = BuiltinsBase::GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> thenKey(globalConst->GetHandledPromiseThenString());
    JSHandle<JSPromiseValueThunkOrThrowerFunction> thrower =
        factory->NewJSPromiseThrowerFunction();
    thrower->SetResult(thread, reason);
    EcmaRuntimeCallInfo *invokeInfo =
        EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, promiseHandle, undefined, 1);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    invokeInfo->SetCallArg(thrower.GetTaggedValue());
    // 9. Return ? Invoke(promise, "then", « thrower »).
    return JSFunction::Invoke(invokeInfo, thenKey);
}

JSHandle<JSTaggedValue> BuiltinsPromiseHandler::PromiseResolve(JSThread *thread,
                                                               const JSHandle<JSTaggedValue> &constructor,
                                                               const JSHandle<JSTaggedValue> &xValue)
{
    BUILTINS_API_TRACE(thread, PromiseHandler, PromiseResolve);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    // 1. Assert: Type(C) is Object.
    ASSERT_PRINT(constructor->IsECMAObject(), "PromiseResolve : is not callable");
    // 2. If IsPromise(x) is true, then
    if (xValue->IsJSPromise()) {
        // a. Let xConstructor be ? Get(x, "constructor").
        JSHandle<JSTaggedValue> ctorKey(globalConst->GetHandledConstructorString());
        JSHandle<JSTaggedValue> ctorValue = JSObject::GetProperty(thread, xValue, ctorKey).GetValue();
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ctorValue);
        // b. If SameValue(xConstructor, C) is true, return x.
        if (JSTaggedValue::SameValue(thread, ctorValue, constructor)) {
            return xValue;
        }
    }
    if (constructor == env->GetPromiseFunction()) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSPromise> promise = factory->NewJSPromise();
        InnerResolve(thread, promise, xValue);
        return JSHandle<JSTaggedValue>(thread, promise.GetTaggedValue());
    }
    // 3. Let promiseCapability be ? NewPromiseCapability(C).
    // 4. ReturnIfAbrupt(promiseCapability)
    JSHandle<PromiseCapability> promiseCapability = JSPromise::NewPromiseCapability(thread, constructor);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    JSHandle<JSTaggedValue> promiseCapaHandle = JSHandle<JSTaggedValue>::Cast(promiseCapability);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, promiseCapaHandle);
    // 6. Let resolveResult be Call(promiseCapability.[[Resolve]], undefined, «x»).
    // 7. ReturnIfAbrupt(resolveResult).
    JSHandle<JSTaggedValue> resolve(thread, promiseCapability->GetResolve(thread));
    JSHandle<JSTaggedValue> undefined(globalConst->GetHandledUndefined());
    EcmaRuntimeCallInfo *info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, resolve, undefined, undefined, 1);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, promiseCapaHandle);
    info->SetCallArg(xValue.GetTaggedValue());
    JSTaggedValue resolveResult = JSFunction::Call(info);
    JSHandle<JSTaggedValue> resolveResultHandle(thread, resolveResult);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, resolveResultHandle);
    // 8. Return promiseCapability.[[Promise]].
    JSHandle<JSTaggedValue> promise(thread, promiseCapability->GetPromise(thread));
    return promise;
}

JSTaggedValue BuiltinsPromiseHandler::AllSettledResolveElementFunction(EcmaRuntimeCallInfo *argv)
{
    // 1. Let F be the active function object.
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PromiseHandler, AllSettledResolveElementFunction);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSPromiseAllSettledElementFunction> resolveElement =
        JSHandle<JSPromiseAllSettledElementFunction>::Cast((GetConstructor(argv)));
    // 2. Let alreadyCalled be F.[[AlreadyCalled]].
    JSHandle<PromiseRecord> alreadyCalled =
        JSHandle<PromiseRecord>::Cast(JSHandle<JSTaggedValue>(thread, resolveElement->GetAlreadyCalled(thread)));
    // 3. If alreadyCalled.[[Value]] is true, return undefined.
    if (alreadyCalled->GetValue(thread).IsTrue()) {
        return JSTaggedValue::Undefined();
    }
    // 4. Set alreadyCalled.[[Value]] to true.
    alreadyCalled->SetValue(thread, JSTaggedValue::True());
    // 5. Let index be F.[[Index]].
    uint32_t index = resolveElement->GetIndex();
    // 6. Let values be F.[[Values]].
    JSHandle<PromiseRecord> values =
        JSHandle<PromiseRecord>::Cast(JSHandle<JSTaggedValue>(thread, resolveElement->GetValues(thread)));
    // 7. Let promiseCapability be F.[[Capability]].
    JSHandle<PromiseCapability> capa =
        JSHandle<PromiseCapability>::Cast(JSHandle<JSTaggedValue>(thread, resolveElement->GetCapability(thread)));
    // 8. Let remainingElementsCount be F.[[RemainingElements]].
    JSHandle<PromiseRecord> remainCnt =
        JSHandle<PromiseRecord>::Cast(JSHandle<JSTaggedValue>(thread, resolveElement->GetRemainingElements(thread)));
    // 9. Let obj be ! OrdinaryObjectCreate(%Object.prototype%).
    JSHandle<JSTaggedValue> proto = env->GetObjectFunctionPrototype();
    JSHandle<JSObject> obj = thread->GetEcmaVM()->GetFactory()->OrdinaryNewJSObjectCreate(proto);
    // 10. Perform ! CreateDataPropertyOrThrow(obj, "status", "fulfilled").
    JSHandle<JSTaggedValue> statusKey = globalConst->GetHandledPromiseStatusString();
    JSHandle<JSTaggedValue> fulfilledKey = globalConst->GetHandledPromiseFulfilledString();
    JSObject::CreateDataPropertyOrThrow(thread, obj, statusKey, fulfilledKey);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 11. Perform ! CreateDataPropertyOrThrow(obj, "value", x).
    JSHandle<JSTaggedValue> valueKey = globalConst->GetHandledValueString();
    JSHandle<JSTaggedValue> xValue = GetCallArg(argv, 0);
    JSObject::CreateDataPropertyOrThrow(thread, obj, valueKey, xValue);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 12. Set values[index] to obj.
    JSHandle<TaggedArray> arrayValues =
        JSHandle<TaggedArray>::Cast(JSHandle<JSTaggedValue>(thread, values->GetValue(thread)));
    arrayValues->Set(thread, index, obj.GetTaggedValue());
    // 13. Set remainingElementsCount.[[Value]] to remainingElementsCount.[[Value]] - 1.
    remainCnt->SetValue(thread, --JSTaggedNumber(remainCnt->GetValue(thread)));
    // 14. If remainingElementsCount.[[Value]] is 0, then
    if (remainCnt->GetValue(thread).IsZero()) {
        // a. Let valuesArray be CreateArrayFromList(values).
        JSHandle<JSArray> jsArrayValues = JSArray::CreateArrayFromList(thread, arrayValues);
        // b. Return ? Call(promiseCapability.[[Resolve]], undefined, « valuesArray »).
        JSHandle<JSTaggedValue> capaResolve(thread, capa->GetResolve(thread));
        JSHandle<JSTaggedValue> undefined(globalConst->GetHandledUndefined());
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, capaResolve, undefined, undefined, 1);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(jsArrayValues.GetTaggedValue());
        return JSFunction::Call(info);
    }
    // 15. Return undefined.
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsPromiseHandler::AllSettledRejectElementFunction(EcmaRuntimeCallInfo *argv)
{
    // 1. Let F be the active function object.
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PromiseHandler, AllSettledRejectElementFunction);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSPromiseAllSettledElementFunction> rejectElement =
        JSHandle<JSPromiseAllSettledElementFunction>::Cast((GetConstructor(argv)));
    // 2. Let alreadyCalled be F.[[AlreadyCalled]].
    JSHandle<PromiseRecord> alreadyCalled =
        JSHandle<PromiseRecord>::Cast(JSHandle<JSTaggedValue>(thread, rejectElement->GetAlreadyCalled(thread)));
    // 3. If alreadyCalled.[[Value]] is true, return undefined.
    if (alreadyCalled->GetValue(thread).IsTrue()) {
        return JSTaggedValue::Undefined();
    }
    // 4. Set alreadyCalled.[[Value]] to true.
    alreadyCalled->SetValue(thread, JSTaggedValue::True());
    // 5. Let index be F.[[Index]].
    uint32_t index = rejectElement->GetIndex();
    // 6. Let values be F.[[Values]].
    JSHandle<PromiseRecord> values =
        JSHandle<PromiseRecord>::Cast(JSHandle<JSTaggedValue>(thread, rejectElement->GetValues(thread)));
    // 7. Let promiseCapability be F.[[Capability]].
    JSHandle<PromiseCapability> capa =
        JSHandle<PromiseCapability>::Cast(JSHandle<JSTaggedValue>(thread, rejectElement->GetCapability(thread)));
    // 8. Let remainingElementsCount be F.[[RemainingElements]].
    JSHandle<PromiseRecord> remainCnt =
        JSHandle<PromiseRecord>::Cast(JSHandle<JSTaggedValue>(thread, rejectElement->GetRemainingElements(thread)));
    // 9. Let obj be ! OrdinaryObjectCreate(%Object.prototype%).
    JSHandle<JSTaggedValue> proto = env->GetObjectFunctionPrototype();
    JSHandle<JSObject> obj = thread->GetEcmaVM()->GetFactory()->OrdinaryNewJSObjectCreate(proto);
    // 10. Perform ! CreateDataPropertyOrThrow(obj, "status", "rejected").
    JSHandle<JSTaggedValue> statusKey = globalConst->GetHandledPromiseStatusString();
    JSHandle<JSTaggedValue> rejectedKey = globalConst->GetHandledPromiseRejectedString();
    JSObject::CreateDataPropertyOrThrow(thread, obj, statusKey, rejectedKey);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 11. Perform ! CreateDataPropertyOrThrow(obj, "reason", x).
    JSHandle<JSTaggedValue> xReason = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> reasonKey = globalConst->GetHandledPromiseReasonString();
    JSObject::CreateDataPropertyOrThrow(thread, obj, reasonKey, xReason);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 12. Set values[index] to obj.
    JSHandle<TaggedArray> arrayValues =
        JSHandle<TaggedArray>::Cast(JSHandle<JSTaggedValue>(thread, values->GetValue(thread)));
    arrayValues->Set(thread, index, obj.GetTaggedValue());
    // 13. Set remainingElementsCount.[[Value]] to remainingElementsCount.[[Value]] - 1.
    remainCnt->SetValue(thread, --JSTaggedNumber(remainCnt->GetValue(thread)));
    // 14. If remainingElementsCount.[[Value]] is 0, then
    if (remainCnt->GetValue(thread).IsZero()) {
        // a. Let valuesArray be CreateArrayFromList(values).
        JSHandle<JSArray> jsArrayValues = JSArray::CreateArrayFromList(thread, arrayValues);
        // b. Return ? Call(promiseCapability.[[Resolve]], undefined, « valuesArray »).
        JSHandle<JSTaggedValue> capaResolve(thread, capa->GetResolve(thread));
        JSHandle<JSTaggedValue> undefined(globalConst->GetHandledUndefined());
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, capaResolve, undefined, undefined, 1);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(jsArrayValues.GetTaggedValue());
        return JSFunction::Call(info);
    }
    // 15. Return undefined.
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsPromiseHandler::AnyRejectElementFunction(EcmaRuntimeCallInfo *argv)
{
    // 1. Let F be the active function object.
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PromiseHandler, AnyRejectElementFunction);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSPromiseAnyRejectElementFunction> rejectElement =
        JSHandle<JSPromiseAnyRejectElementFunction>::Cast((GetConstructor(argv)));
    // 2. If F.[[AlreadyCalled]] is true, return undefined.
    JSTaggedValue alreadyCalled = rejectElement->GetAlreadyCalled(thread);
    if (alreadyCalled.IsTrue()) {
        return JSTaggedValue::Undefined();
    }
    // 3. Set F.[[AlreadyCalled]] to true.
    rejectElement->SetAlreadyCalled(thread, JSTaggedValue::True());
    // 4. Let index be F.[[Index]].
    uint32_t index = rejectElement->GetIndex();
    // 5. Let errors be F.[[Errors]].
    JSHandle<PromiseRecord> errors =
        JSHandle<PromiseRecord>::Cast(JSHandle<JSTaggedValue>(thread, rejectElement->GetErrors(thread)));
    // 6. Let promiseCapability be F.[[Capability]].
    JSHandle<PromiseCapability> capa =
        JSHandle<PromiseCapability>::Cast(JSHandle<JSTaggedValue>(thread, rejectElement->GetCapability(thread)));
    // 7. Let remainingElementsCount be F.[[RemainingElements]].
    JSHandle<PromiseRecord> remainCnt =
        JSHandle<PromiseRecord>::Cast(JSHandle<JSTaggedValue>(thread, rejectElement->GetRemainingElements(thread)));
    // 8. Set errors[index] to x.
    JSHandle<JSTaggedValue> xValue = GetCallArg(argv, 0);
    JSHandle<TaggedArray> errorsArray =
        JSHandle<TaggedArray>::Cast(JSHandle<JSTaggedValue>(thread, errors->GetValue(thread)));
    errorsArray->Set(thread, index, xValue.GetTaggedValue());
    // 9. Set remainingElementsCount.[[Value]] to remainingElementsCount.[[Value]] - 1.
    remainCnt->SetValue(thread, --JSTaggedNumber(remainCnt->GetValue(thread)));
    // 10. If remainingElementsCount.[[Value]] is 0, then
    if (remainCnt->GetValue(thread).IsZero()) {
        // a. Let error be a newly created AggregateError object.
        JSHandle<JSObject> error = factory->NewJSAggregateError();
        // b. Perform ! DefinePropertyOrThrow(error, "errors", PropertyDescriptor { [[Configurable]]: true,
        //    [[Enumerable]]: false, [[Writable]]: true, [[Value]]: ! CreateArrayFromList(errors) }).
        JSHandle<JSTaggedValue> errorsKey(thread, globalConst->GetErrorsString());
        JSHandle<JSTaggedValue> errorsValue(JSArray::CreateArrayFromList(thread, errorsArray));
        PropertyDescriptor msgDesc(thread, errorsValue, true, false, true);
        JSHandle<JSTaggedValue> errorTagged = JSHandle<JSTaggedValue>::Cast(error);
        JSTaggedValue::DefinePropertyOrThrow(thread, errorTagged, errorsKey, msgDesc);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // c. Return ? Call(promiseCapability.[[Reject]], undefined, « error »).
        JSHandle<JSTaggedValue> capaReject(thread, capa->GetReject(thread));
        JSHandle<JSTaggedValue> undefined(globalConst->GetHandledUndefined());
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, capaReject, undefined, undefined, 1);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(error.GetTaggedValue());
        return JSFunction::Call(info);
    }
    // 11. Return undefined.
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsPromiseHandler::InnerResolve(JSThread *thread, const JSHandle<JSPromise> &promise,
    const JSHandle<JSTaggedValue> &resolution)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    // 6. If SameValue(resolution, promise) is true, then
    //     a. Let selfResolutionError be a newly created TypeError object.
    //     b. Return RejectPromise(promise, selfResolutionError).
    if (JSTaggedValue::SameValue(thread, resolution.GetTaggedValue(), promise.GetTaggedValue())) {
        JSHandle<JSObject> resolutionError = factory->GetJSError(ErrorType::TYPE_ERROR,
            "Resolve: The promise and resolution cannot be the same.", StackCheck::NO);
        JSPromise::RejectPromise(thread, promise, JSHandle<JSTaggedValue>::Cast(resolutionError));
        return JSTaggedValue::Undefined();
    }
    // 7. If Type(resolution) is not Object, then
    //     a. Return FulfillPromise(promise, resolution).
    if (!resolution.GetTaggedValue().IsECMAObject()) {
        JSPromise::FulfillPromise(thread, promise, resolution);
        return JSTaggedValue::Undefined();
    }
    // 8. Let then be Get(resolution, "then").
    // 9. If then is an abrupt completion, then
    //     a. Return RejectPromise(promise, then.[[value]]).
    JSHandle<JSTaggedValue> thenKey(thread->GlobalConstants()->GetHandledPromiseThenString());
    JSHandle<JSTaggedValue> thenValue = JSObject::GetProperty(thread, resolution, thenKey).GetValue();
    if (thread->HasPendingException()) {
        if (!thenValue->IsJSError()) {
            thenValue = JSHandle<JSTaggedValue>(thread, thread->GetException());
        }
        thread->ClearException();
        return JSPromise::RejectPromise(thread, promise, thenValue);
    }
    // 10. Let thenAction be then.[[value]].
    // 11. If IsCallable(thenAction) is false, then
    //     a. Return FulfillPromise(promise, resolution).
    if (!thenValue->IsCallable()) {
        JSPromise::FulfillPromise(thread, promise, resolution);
        return JSTaggedValue::Undefined();
    }
    // 12. Perform EnqueueJob ("PromiseJobs", PromiseResolveThenableJob, «promise, resolution, thenAction»)
    JSHandle<TaggedArray> arguments = factory->NewTaggedArray(3);  // 3: 3 means three args stored in array
    arguments->Set(thread, 0, promise);
    arguments->Set(thread, 1, resolution);
    arguments->Set(thread, 2, thenValue);  // 2: 2 means index of array is 2

    JSHandle<JSFunction> promiseResolveThenableJob(env->GetPromiseResolveThenableJob());
    JSHandle<job::MicroJobQueue> job = thread->GetEcmaVM()->GetMicroJobQueue();
    job::MicroJobQueue::EnqueueJob(thread, job, job::QueueType::QUEUE_PROMISE, promiseResolveThenableJob, arguments);
        // 13. Return undefined.
    return JSTaggedValue::Undefined();
}
}  // namespace panda::ecmascript::builtins
