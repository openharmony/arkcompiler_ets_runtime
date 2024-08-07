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

#include "ecmascript/builtins/builtins_async_generator.h"
#include "ecmascript/js_async_generator_object.h"

namespace panda::ecmascript::builtins {
// ecma 27.6.1.1
JSTaggedValue BuiltinsAsyncGenerator::AsyncGeneratorFunctionConstructor(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), AsyncGenerator, Constructor);
    // not support
    THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "Not support eval. Forbidden using new AsyncGeneratorFunction().",
                                JSTaggedValue::Exception());
}

// ecma 27.6.1.2 AsyncGenerator.prototype.next
JSTaggedValue BuiltinsAsyncGenerator::AsyncGeneratorPrototypeNext(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, AsyncGenerator, PrototypeNext);
    // 1.Let g be the this value.
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetThis(argv);

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    // 2. Let completion be NormalCompletion(value).
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<CompletionRecord> completionRecord =
        factory->NewCompletionRecord(CompletionRecordType::NORMAL, value);
    // 3. Return ! AsyncGeneratorEnqueue(generator, completion, empty).
    return JSAsyncGeneratorObject::AsyncGeneratorEnqueue(thread, msg, completionRecord);
}

// ecma 27.6.1.3 AsyncGenerator.prototype.return
JSTaggedValue BuiltinsAsyncGenerator::AsyncGeneratorPrototypeReturn(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, AsyncGenerator, PrototypeReturn);
    // 1.Let generator be the this value.
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetThis(argv);
    // 2.Let completion be Completion { [[Type]]: return, [[Value]]: value, [[Target]]: empty }.
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<CompletionRecord> completionRecord =
        factory->NewCompletionRecord(CompletionRecordType::RETURN, value);
    // 3.Return ! AsyncGeneratorEnqueue(generator, completion, empty).
    return JSAsyncGeneratorObject::AsyncGeneratorEnqueue(thread, msg, completionRecord);
}

// ecma 27.6.1.4 AsyncGenerator.prototype.throw
JSTaggedValue BuiltinsAsyncGenerator::AsyncGeneratorPrototypeThrow(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, AsyncGenerator, PrototypeThrow);
    // 1.Let generator be the this value.
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetThis(argv);
    // 2.Let completion be ThrowCompletion(exception).
    JSHandle<JSTaggedValue> exception = GetCallArg(argv, 0);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<CompletionRecord> completionRecord =
        factory->NewCompletionRecord(CompletionRecordType::THROW, exception);
    // 3.Return ! AsyncGeneratorEnqueue(generator, completion, empty).
    return JSAsyncGeneratorObject::AsyncGeneratorEnqueue(thread, msg, completionRecord);
}
}  // namespace panda::ecmascript::builtins
