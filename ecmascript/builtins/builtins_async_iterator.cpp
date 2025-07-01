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

#include "ecmascript/builtins/builtins_async_iterator.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_promise.h"

namespace panda::ecmascript::builtins {
JSTaggedValue BuiltinsAsyncIterator::AsyncIteratorConstructor([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), AsyncIterator, Constructor);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsAsyncIterator::Next([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), AsyncIterator, Next);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsAsyncIterator::Throw([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), AsyncIterator, Throw);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsAsyncIterator::Return(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, AsyncIterator, Return);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> promiseFunc = env->GetPromiseFunction();
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSHandle<PromiseCapability> pcap = JSPromise::NewPromiseCapability(thread, promiseFunc);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSObject> iterResult = JSIterator::CreateIterResultObject(thread, value, true);
    JSHandle<JSTaggedValue> iterResultVal(iterResult);
    JSHandle<JSTaggedValue> resolve(thread, pcap->GetResolve(thread));
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo* info = EcmaInterpreter::NewRuntimeCallInfo(thread, resolve, undefined, undefined, 1);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    info->SetCallArg(iterResultVal.GetTaggedValue());
    JSFunction::Call(info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return pcap->GetPromise(thread);
}

JSTaggedValue BuiltinsAsyncIterator::GetAsyncIteratorObj(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), AsyncIterator, GetObj);
    return base::BuiltinsBase::GetThis(argv).GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins
