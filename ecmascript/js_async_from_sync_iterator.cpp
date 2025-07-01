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

#include "ecmascript/js_async_from_sync_iterator.h"

#include "ecmascript/builtins/builtins_promise.h"
#include "ecmascript/builtins/builtins_promise_handler.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_iterator.h"

namespace panda::ecmascript {
JSHandle<JSTaggedValue> JSAsyncFromSyncIterator::CreateAsyncFromSyncIterator(JSThread *thread,
    JSHandle<AsyncIteratorRecord> &syncIteratorRecord)
{
    // 1.Let asyncIterator be ! OrdinaryObjectCreate(%AsyncFromSyncIteratorPrototype%, « [[SyncIteratorRecord]] »).
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> IterFunc(env->GetAsyncFromSyncIterator());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAsyncFromSyncIterator> asyncIterator(factory->NewJSObjectByConstructor(IterFunc));

    // 2.Set asyncIterator.[[SyncIteratorRecord]] to syncIteratorRecord.
    asyncIterator->SetSyncIteratorRecord(thread, syncIteratorRecord);

    // 3.Let nextMethod be ! Get(asyncIterator, "next").
    JSHandle<JSTaggedValue> nextStr = thread->GlobalConstants()->GetHandledNextString();
    JSHandle<JSTaggedValue> tmpAsyncIterator(thread, asyncIterator.GetTaggedValue());
    JSHandle<JSTaggedValue> nextMethod = JSTaggedValue::GetProperty(thread, tmpAsyncIterator, nextStr).GetValue();
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // 4.Let iteratorRecord be the Record {[[Iterator]]: asyncIterator, [[NextMethod]]: nextMethod, [[Done]]: false}.
    JSHandle<AsyncIteratorRecord> iteratorRecord = factory->NewAsyncIteratorRecord(tmpAsyncIterator, nextMethod, false);
    return JSHandle<JSTaggedValue>(thread, iteratorRecord->GetIterator(thread));
}

JSTaggedValue JSAsyncFromSyncIterator::AsyncFromSyncIteratorContinuation(JSThread *thread,
                                                                         JSHandle<JSTaggedValue> &result,
                                                                         JSHandle<PromiseCapability> &promiseCapability)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1.Let done be IteratorComplete(result).
    bool done = JSIterator::IteratorComplete(thread, result);
    // 2.IfAbruptRejectPromise(done, promiseCapability).
    JSHandle<JSTaggedValue> tmpDone(thread, JSTaggedValue(done));
    RETURN_REJECT_PROMISE_IF_ABRUPT(thread, tmpDone, promiseCapability);
    // 3.Let value be IteratorValue(result).
    JSHandle<JSTaggedValue> value = JSIterator::IteratorValue(thread, result);
    // 4.IfAbruptRejectPromise(value, promiseCapability).
    RETURN_REJECT_PROMISE_IF_ABRUPT(thread, value, promiseCapability);
    // 5.Let valueWrapper be PromiseResolve(%Promise%, value).
    // 6.IfAbruptRejectPromise(valueWrapper, promiseCapability).
    // 7.Let steps be the algorithm steps defined in Async-from-Sync Iterator Value Unwrap Functions.
    // 8.Let length be the number of non-optional parameters of the function definition
    // in Async-from-Sync Iterator Value Unwrap Functions.
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> valueWrapper =
        builtins::BuiltinsPromiseHandler::PromiseResolve(thread,
                                                         JSHandle<JSTaggedValue>::Cast(env->GetPromiseFunction()),
                                                         value);
    RETURN_REJECT_PROMISE_IF_ABRUPT(thread, valueWrapper, promiseCapability);
    JSHandle<JSObject> promise = JSHandle<JSObject>::Cast(valueWrapper);

    // 9.Let onFulfilled be ! CreateBuiltinFunction(steps, length, "", « [[Done]] »).
    JSHandle<JSAsyncFromSyncIterUnwarpFunction> onFulfilled = factory->NewJSAsyncFromSyncIterUnwarpFunction();
    // 10.Set onFulfilled.[[Done]] to done.

    onFulfilled->SetDone(thread, JSTaggedValue(done));
    // 11.Perform ! PerformPromiseThen(valueWrapper, onFulfilled, undefined, promiseCapability).
    JSHandle<JSTaggedValue> onFulRejected(thread, JSTaggedValue::Undefined());
    builtins::BuiltinsPromise::PerformPromiseThen(thread, JSHandle<JSPromise>::Cast(promise),
                                                  JSHandle<JSTaggedValue>::Cast(onFulfilled),
                                                  onFulRejected, promiseCapability);
    // 12.Return promiseCapability.[[Promise]].
    return promiseCapability->GetPromise(thread);
}

JSTaggedValue JSAsyncFromSyncIterator::AsyncFromSyncIterUnwarpFunction(EcmaRuntimeCallInfo *argv)
{
    // 1. Let F be the active function object.
    JSThread *thread = argv->GetThread();
    JSHandle<JSAsyncFromSyncIterUnwarpFunction> unwarpFunction =
        JSHandle<JSAsyncFromSyncIterUnwarpFunction>::Cast((base::BuiltinsBase::GetConstructor(argv)));

    // 2.Return ! CreateIterResultObject(value, F.[[Done]]).
    JSHandle<JSTaggedValue> value = base::BuiltinsBase::GetCallArg(argv, 0);
    bool done = unwarpFunction->GetDone(thread).ToBoolean();
    return JSIterator::CreateIterResultObject(thread, value, done).GetTaggedValue();
}
}  // namespace panda::ecmascript
