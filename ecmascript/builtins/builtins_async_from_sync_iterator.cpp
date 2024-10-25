/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/builtins/builtins_async_from_sync_iterator.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_async_from_sync_iterator.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_promise.h"

namespace panda::ecmascript::builtins {

JSTaggedValue BuiltinsAsyncFromSyncIterator::Next(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, AsyncFromSyncIterator, Next);
    [[maybe_unused]] EcmaHandleScope scope(thread);
    auto vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    // 1.Let O be the this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);
    // 2.Assert: Type(O) is Object and O has a [[SyncIteratorRecord]] internal slot.
    if (!thisValue->IsAsyncFromSyncIterator()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "is not AsyncFromSyncIterator", JSTaggedValue::Exception());
    }
    // 3.Let promiseCapability be ! NewPromiseCapability(%Promise%).
    JSHandle<PromiseCapability> pcap =
        JSPromise::NewPromiseCapability(thread, JSHandle<JSTaggedValue>::Cast(env->GetPromiseFunction()));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 4.Let syncIteratorRecord be O.[[SyncIteratorRecord]].
    JSHandle<JSAsyncFromSyncIterator> asyncIterator(thisValue);
    JSHandle<AsyncIteratorRecord> syncIteratorRecord(thread, asyncIterator->GetSyncIteratorRecord());
    // 5.If value is present, then
    // a.Let result be IteratorNext(syncIteratorRecord, value).
    // 6.Else,
    // a.Let result be IteratorNext(syncIteratorRecord).
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> result;
    if (value->IsNull()) {
        result = JSIterator::IteratorNext(thread, syncIteratorRecord);
    } else {
        result = JSIterator::IteratorNext(thread, syncIteratorRecord, value);
    }
    // 7.IfAbruptRejectPromise(result, promiseCapability).
    RETURN_REJECT_PROMISE_IF_ABRUPT(thread, result, pcap);
    // 8.Return ! AsyncFromSyncIteratorContinuation(result, promiseCapability).
    return JSAsyncFromSyncIterator::AsyncFromSyncIteratorContinuation(thread, result, pcap);
}

JSTaggedValue BuiltinsAsyncFromSyncIterator::Throw(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, AsyncFromSyncIterator, Throw);
    [[maybe_unused]] EcmaHandleScope scope(thread);
    auto vm = thread->GetEcmaVM();
    const GlobalEnvConstants *globalConstant = thread->GlobalConstants();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1.Let O be the this value.
    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));
    JSHandle<JSAsyncFromSyncIterator> asyncIterator(input);
    // 3.Let promiseCapability be ! NewPromiseCapability(%Promise%).
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<PromiseCapability> pcap =
        JSPromise::NewPromiseCapability(thread, JSHandle<JSTaggedValue>::Cast(env->GetPromiseFunction()));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 4.Let syncIterator be O.[[SyncIteratorRecord]].[[Iterator]].
    JSHandle<AsyncIteratorRecord> syncIteratorRecord(thread, asyncIterator->GetSyncIteratorRecord());
    JSHandle<JSTaggedValue> syncIterator(thread, syncIteratorRecord->GetIterator());
    // 5.Let return be GetMethod(syncIterator, "throw").
    JSHandle<JSTaggedValue> throwString = globalConstant->GetHandledThrowString();
    JSHandle<JSTaggedValue> throwResult = JSObject::GetMethod(thread, syncIterator, throwString);
    RETURN_REJECT_PROMISE_IF_ABRUPT(thread, throwString, pcap);
    JSHandle<JSTaggedValue> value = base::BuiltinsBase::GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> undefinedValue = globalConstant->GetHandledUndefined();
    // 7.If throw is undefined, then
    if (throwResult->IsUndefined()) {
        JSHandle<JSObject> iterResult = JSIterator::CreateIterResultObject(thread, value, true);
        JSHandle<JSTaggedValue> reject(thread, pcap->GetReject());
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, reject, undefinedValue, undefinedValue, 1);
        info->SetCallArg(iterResult.GetTaggedValue());
        return pcap->GetPromise();
    }
    JSTaggedValue ret;
    // 8.If value is present, then
    if (value->IsNull()) {
        EcmaRuntimeCallInfo *callInfo =
            EcmaInterpreter::NewRuntimeCallInfo(thread, throwResult, syncIterator, undefinedValue, 0);
        RETURN_REJECT_PROMISE_IF_ABRUPT(thread, throwResult, pcap);
        ret = JSFunction::Call(callInfo);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    } else {
        EcmaRuntimeCallInfo *callInfo =
            EcmaInterpreter::NewRuntimeCallInfo(thread, throwResult, syncIterator, undefinedValue, 1);
        RETURN_REJECT_PROMISE_IF_ABRUPT(thread, throwResult, pcap);
        callInfo->SetCallArg(value.GetTaggedValue());
        ret = JSFunction::Call(callInfo);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    JSHandle<JSTaggedValue> result(thread, ret);
    // 11.If Type(result) is not Object, then
    if (!result->IsECMAObject()) {
        // a.Perform ! Call(promiseCapability.[[Reject]], undefined, « a newly created TypeError object »).
        JSHandle<JSObject> resolutionError =
            factory->GetJSError(ErrorType::TYPE_ERROR,
                                "AsyncFromSyncIteratorPrototype.throw: is not Object.", StackCheck::NO);
        JSHandle<JSTaggedValue> reject(thread, pcap->GetReject());
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, reject, undefinedValue, undefinedValue, 1);
        info->SetCallArg(resolutionError.GetTaggedValue());
        JSFunction::Call(info);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

        // b.Return promiseCapability.[[Promise]].
        JSHandle<JSObject> promise(thread, pcap->GetPromise());
        return promise.GetTaggedValue();
    }
    // 12.Return ! AsyncFromSyncIteratorContinuation(result, promiseCapability).
    return JSAsyncFromSyncIterator::AsyncFromSyncIteratorContinuation(thread, result, pcap);
}

JSTaggedValue BuiltinsAsyncFromSyncIterator::Return(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, AsyncFromSyncIterator, Return);
    [[maybe_unused]] EcmaHandleScope scope(thread);
    auto vm = thread->GetEcmaVM();
    const GlobalEnvConstants *globalConstant = thread->GlobalConstants();
    ObjectFactory *factory = vm->GetFactory();
    // 1.Let O be the this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);
    if (!thisValue->IsAsyncFromSyncIterator()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "is not AsyncFromSyncIterator", JSTaggedValue::Exception());
    }
    
    // 3.Let promiseCapability be ! NewPromiseCapability(%Promise%).
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<PromiseCapability> pcap =
        JSPromise::NewPromiseCapability(thread, JSHandle<JSTaggedValue>::Cast(env->GetPromiseFunction()));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 4.Let syncIterator be O.[[SyncIteratorRecord]].[[Iterator]].
    JSHandle<JSAsyncFromSyncIterator> asyncIterator(thisValue);
    JSHandle<AsyncIteratorRecord> syncIteratorRecord(thread, asyncIterator->GetSyncIteratorRecord());
    JSHandle<JSTaggedValue> syncIterator(thread, syncIteratorRecord->GetIterator());
    // 5.Let return be GetMethod(syncIterator, "return").
    JSHandle<JSTaggedValue> returnString = globalConstant->GetHandledReturnString();
    JSHandle<JSTaggedValue> returnResult = JSObject::GetMethod(thread, syncIterator, returnString);
    RETURN_REJECT_PROMISE_IF_ABRUPT(thread, returnResult, pcap);
    JSHandle<JSTaggedValue> value = base::BuiltinsBase::GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> undefinedValue = globalConstant->GetHandledUndefined();
    // 7.If return is undefined, then
    if (returnResult->IsUndefined()) {
        JSHandle<JSObject> iterResult = JSIterator::CreateIterResultObject(thread, value, true);
        JSHandle<JSTaggedValue> its = JSHandle<JSTaggedValue>::Cast(iterResult);
        JSHandle<JSTaggedValue> resolve(thread, pcap->GetResolve());
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, resolve, undefinedValue, undefinedValue, 1);
        info->SetCallArg(its.GetTaggedValue());
        JSHandle<JSObject> promise(thread, pcap->GetPromise());
        return promise.GetTaggedValue();
    }
    JSTaggedValue ret;
    // 8.If value is present, then
    if (value->IsNull()) {
        EcmaRuntimeCallInfo *callInfo =
            EcmaInterpreter::NewRuntimeCallInfo(thread, returnResult, syncIterator, undefinedValue, 0);
        RETURN_REJECT_PROMISE_IF_ABRUPT(thread, returnResult, pcap);
        ret = JSFunction::Call(callInfo);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    } else {
        EcmaRuntimeCallInfo *callInfo =
            EcmaInterpreter::NewRuntimeCallInfo(thread, returnResult, syncIterator, undefinedValue, 1);
        RETURN_REJECT_PROMISE_IF_ABRUPT(thread, returnResult, pcap);
        callInfo->SetCallArg(value.GetTaggedValue());
        ret = JSFunction::Call(callInfo);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    JSHandle<JSTaggedValue> result(thread, ret);
    // 11.If Type(result) is not Object, then
    if (!result->IsECMAObject()) {
        // a.Perform ! Call(promiseCapability.[[Reject]], undefined, « a newly created TypeError object »).
        JSHandle<JSObject> resolutionError = factory->GetJSError(ErrorType::TYPE_ERROR,
            "AsyncFromSyncIteratorPrototype.return: is not Object.", StackCheck::NO);
        JSHandle<JSTaggedValue> rstErr = JSHandle<JSTaggedValue>::Cast(resolutionError);
        JSHandle<JSTaggedValue> reject(thread, pcap->GetReject());
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, reject, undefinedValue, undefinedValue, 1);
        info->SetCallArg(rstErr.GetTaggedValue());
        JSFunction::Call(info);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

        // b.Return promiseCapability.[[Promise]].
        JSHandle<JSObject> promise(thread, pcap->GetPromise());
        return promise.GetTaggedValue();
    }
    // 12.Return ! AsyncFromSyncIteratorContinuation(result, promiseCapability).
    return JSAsyncFromSyncIterator::AsyncFromSyncIteratorContinuation(thread, result, pcap);
}
}  // namespace panda::ecmascript::builtins
