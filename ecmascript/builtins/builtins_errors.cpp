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

#include "ecmascript/builtins/builtins_errors.h"
#include "ecmascript/js_array.h"

namespace panda::ecmascript::builtins {
using ErrorHelper = base::ErrorHelper;
using ErrorType = base::ErrorType;
// Error
JSTaggedValue BuiltinsError::ErrorConstructor(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Error, ErrorConstructor);
    return ErrorHelper::ErrorCommonConstructor(argv, ErrorType::ERROR);
}

JSTaggedValue BuiltinsError::ToString(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Error, ErrorToString);
    return ErrorHelper::ErrorCommonToString(argv, ErrorType::ERROR);
}

// RangeError
JSTaggedValue BuiltinsRangeError::RangeErrorConstructor(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Error, RangeErrorConstructor);
    return ErrorHelper::ErrorCommonConstructor(argv, ErrorType::RANGE_ERROR);
}

JSTaggedValue BuiltinsRangeError::ToString(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Error, RangeErrorToString);
    return ErrorHelper::ErrorCommonToString(argv, ErrorType::RANGE_ERROR);
}

// ReferenceError
JSTaggedValue BuiltinsReferenceError::ReferenceErrorConstructor(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Error, ReferenceErrorConstructor);
    return ErrorHelper::ErrorCommonConstructor(argv, ErrorType::REFERENCE_ERROR);
}

JSTaggedValue BuiltinsReferenceError::ToString(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Error, ReferenceErrorToString);
    return ErrorHelper::ErrorCommonToString(argv, ErrorType::REFERENCE_ERROR);
}

// TypeError
JSTaggedValue BuiltinsTypeError::TypeErrorConstructor(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Error, TypeErrorConstructor);
    return ErrorHelper::ErrorCommonConstructor(argv, ErrorType::TYPE_ERROR);
}

JSTaggedValue BuiltinsTypeError::ToString(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Error, TypeErrorToString);
    return ErrorHelper::ErrorCommonToString(argv, ErrorType::TYPE_ERROR);
}

JSTaggedValue BuiltinsTypeError::ThrowTypeError(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Error, ThrowTypeError);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    THROW_TYPE_ERROR_AND_RETURN(thread, "type error", JSTaggedValue::Exception());
}

// URIError
JSTaggedValue BuiltinsURIError::URIErrorConstructor(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Error, URIErrorConstructor);
    return ErrorHelper::ErrorCommonConstructor(argv, ErrorType::URI_ERROR);
}

JSTaggedValue BuiltinsURIError::ToString(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Error, URIErrorToString);
    return ErrorHelper::ErrorCommonToString(argv, ErrorType::URI_ERROR);
}

// SyntaxError
JSTaggedValue BuiltinsSyntaxError::SyntaxErrorConstructor(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Error, SyntaxErrorConstructor);
    return ErrorHelper::ErrorCommonConstructor(argv, ErrorType::SYNTAX_ERROR);
}

JSTaggedValue BuiltinsSyntaxError::ToString(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Error, SyntaxErrorToString);
    return ErrorHelper::ErrorCommonToString(argv, ErrorType::SYNTAX_ERROR);
}

// EvalError
JSTaggedValue BuiltinsEvalError::EvalErrorConstructor(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Error, EvalErrorConstructor);
    return ErrorHelper::ErrorCommonConstructor(argv, ErrorType::EVAL_ERROR);
}

JSTaggedValue BuiltinsEvalError::ToString(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Error, EvalErrorToString);
    return ErrorHelper::ErrorCommonToString(argv, ErrorType::EVAL_ERROR);
}

// AggregateError
JSTaggedValue BuiltinsAggregateError::AggregateErrorConstructor(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(argv->GetThread(), Error, AggregateErrorConstructor);
    [[maybe_unused]] EcmaHandleScope scope(thread);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    // 1. If NewTarget is undefined, let newTarget be the active function object; else let newTarget be NewTarget.
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSMutableHandle<JSTaggedValue> newTarget(thread, GetNewTarget(argv));
    if (newTarget->IsUndefined()) {
        newTarget.Update(constructor.GetTaggedValue());
    }
    JSHandle<JSTaggedValue> errors = BuiltinsBase::GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> message = BuiltinsBase::GetCallArg(argv, 1);
    // 2. Let O be ? OrdinaryCreateFromConstructor(newTarget, "%AggregateError.prototype%", « [[ErrorData]] »).
    JSHandle<JSObject> objValues = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> taggedObj = JSHandle<JSTaggedValue>::Cast(objValues);
    // 3. If message is not undefined, then
    // a. Let msg be ? ToString(message).
    // b. Let msgDesc be the PropertyDescriptor
    // { [[Value]]: msg, [[Writable]]: true, [[Enumerable]]: false, [[Configurable]]: true }.
    // c. Perform ! DefinePropertyOrThrow(O, "message", msgDesc).
    JSHandle<JSTaggedValue> msgKey = globalConst->GetHandledMessageString();
    JSHandle<JSTaggedValue> errorsKey = globalConst->GetHandledErrorsString();
    if (!message->IsUndefined()) {
        JSHandle<EcmaString> handleStr = JSTaggedValue::ToString(thread, message);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        PropertyDescriptor msgDesc(thread, JSHandle<JSTaggedValue>::Cast(handleStr), true, false, true);
        JSTaggedValue::DefinePropertyOrThrow(thread, taggedObj, msgKey, msgDesc);
    }
    // InstallErrorCause
    JSHandle<JSTaggedValue> options = BuiltinsBase::GetCallArg(argv, 2); // 2 : Third parameter
    // If options is an Object and ? HasProperty(options, "cause") is true, then
    //   a. Let cause be ? Get(options, "cause").
    //   b. Perform CreateNonEnumerableDataPropertyOrThrow(O, "cause", cause).
    if (options->IsECMAObject()) {
        JSHandle<JSTaggedValue> causeKey = globalConst->GetHandledCauseString();
        bool causePresent = JSTaggedValue::HasProperty(thread, options, causeKey);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (causePresent) {
            JSHandle<JSTaggedValue> cause = JSObject::GetProperty(thread, options, causeKey).GetValue();
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            PropertyDescriptor causeDesc(thread, cause, true, false, true);
            JSTaggedValue::DefinePropertyOrThrow(thread, taggedObj, causeKey, causeDesc);
        }
    }
    // 4. Let errorsList be ? IterableToList(errors).
    JSHandle<JSTaggedValue> errorsList = JSObject::IterableToList(thread, errors);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 5. Perform ! DefinePropertyOrThrow(O, "errors", PropertyDescriptor { [[Configurable]]: true,
    //    [[Enumerable]]: false, [[Writable]]: true, [[Value]]: !CreateArrayFromList(errorsList) }).
    JSHandle<TaggedArray> errorsArray = JSArray::ToTaggedArray(thread, errorsList);
    JSHandle<JSTaggedValue> errorsValues(JSArray::CreateArrayFromList(thread, errorsArray));
    PropertyDescriptor msgDesc(thread, errorsValues, true, false, true);
    JSTaggedValue::DefinePropertyOrThrow(thread, taggedObj, errorsKey, msgDesc);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 6. Return O.
    return taggedObj.GetTaggedValue();
}

JSTaggedValue BuiltinsAggregateError::ToString(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Error, AggregateErrorToString);
    return ErrorHelper::ErrorCommonToString(argv, ErrorType::AGGREGATE_ERROR);
}

// OOMError
JSTaggedValue BuiltinsOOMError::OOMErrorConstructor(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Error, OOMErrorConstructor);
    return ErrorHelper::ErrorCommonConstructor(argv, ErrorType::OOM_ERROR);
}

JSTaggedValue BuiltinsOOMError::ToString(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Error, OOMErrorToString);
    return ErrorHelper::ErrorCommonToString(argv, ErrorType::OOM_ERROR);
}

// TerminationError
JSTaggedValue BuiltinsTerminationError::TerminationErrorConstructor(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Error, TerminationErrorConstructor);
    return ErrorHelper::ErrorCommonConstructor(argv, ErrorType::TERMINATION_ERROR);
}

JSTaggedValue BuiltinsTerminationError::ToString(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Error, TerminationErrorToString);
    return ErrorHelper::ErrorCommonToString(argv, ErrorType::TERMINATION_ERROR);
}
}  // namespace panda::ecmascript::builtins
