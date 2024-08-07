/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/builtins/builtins_iterator.h"
#include "ecmascript/js_iterator.h"

namespace panda::ecmascript::builtins {
JSTaggedValue BuiltinsIterator::IteratorConstructor([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Iterator, Constructor);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsIterator::Next([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Iterator, Next);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsIterator::Throw([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Iterator, Throw);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsIterator::Return(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Iterator, Return);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisValue = GetCallArg(argv, 0);
    return ReturnInternal(thread, thisValue);
}

JSTaggedValue BuiltinsIterator::ReturnInternal(JSThread *thread, JSHandle<JSTaggedValue> thisValue)
{
    JSHandle<JSObject> iterResult = JSIterator::CreateIterResultObject(thread, thisValue, true);
    return iterResult.GetTaggedValue();
}

JSTaggedValue BuiltinsIterator::GetIteratorObj(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Iterator, GetObj);
    return base::BuiltinsBase::GetThis(argv).GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins
