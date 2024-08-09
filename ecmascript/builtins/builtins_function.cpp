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

#include "ecmascript/builtins/builtins_function.h"

#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/object_fast_operator-inl.h"

namespace panda::ecmascript::builtins {
// ecma 19.2.1 Function (p1, p2, ... , pn, body)
JSTaggedValue BuiltinsFunction::FunctionConstructor(EcmaRuntimeCallInfo *argv)
{
    // not support
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Function, Constructor);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    THROW_TYPE_ERROR_AND_RETURN(thread, "Not support eval. Forbidden using new Function()/Function().",
                                JSTaggedValue::Exception());
}

// ecma 19.2.3 The Function prototype object is itself a built-in function object.
//             When invoked, it accepts any arguments and returns undefined.
JSTaggedValue BuiltinsFunction::FunctionPrototypeInvokeSelf([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Function, PrototypeInvokeSelf);
    return JSTaggedValue::Undefined();
}
namespace {
static size_t MakeArgListWithHole(JSThread *thread, TaggedArray *argv, int length)
{
    if (length <= 0) {
        return 0;
    }
    uint32_t inputLength = static_cast<uint32_t>(length);
    uint32_t arrayLength = argv->GetLength();
    if (inputLength > arrayLength) {
        inputLength = arrayLength;
    }
    for (uint32_t index = 0; index < inputLength; ++index) {
        JSTaggedValue value = argv->Get(thread, index);
        if (value.IsHole()) {
            argv->Set(thread, index, JSTaggedValue::Undefined());
        }
    }
    return static_cast<size_t>(inputLength);
}

static std::pair<TaggedArray*, size_t> BuildArgumentsListFast(JSThread *thread,
                                                              const JSHandle<JSTaggedValue> &arrayObj)
{
    if (!arrayObj->HasStableElements(thread)) {
        return std::make_pair(nullptr, 0);
    }
    if (arrayObj->IsStableJSArguments(thread)) {
        JSHandle<JSArguments> argList = JSHandle<JSArguments>::Cast(arrayObj);
        TaggedArray *elements = TaggedArray::Cast(argList->GetElements().GetTaggedObject());
        auto env = thread->GetEcmaVM()->GetGlobalEnv();
        if (argList->GetClass() != env->GetArgumentsClass().GetObject<JSHClass>()) {
            return std::make_pair(nullptr, 0);
        }
        auto result = argList->GetPropertyInlinedProps(JSArguments::LENGTH_INLINE_PROPERTY_INDEX);
        if (!result.IsInt()) {
            return std::make_pair(nullptr, 0);
        }
        auto length = result.GetInt();
        size_t res = MakeArgListWithHole(thread, elements, length);
        return std::make_pair(elements, res);
    } else if (arrayObj->IsStableJSArray(thread)) {
        JSHandle<JSArray> argList = JSHandle<JSArray>::Cast(arrayObj);
        TaggedArray *elements = nullptr;
        if (argList->GetElements().IsMutantTaggedArray()) {
            JSHandle<JSObject> obj(arrayObj);
            int elementsLength = static_cast<int>(ElementAccessor::GetElementsLength(obj));
            JSHandle<TaggedArray> newElements = thread->GetEcmaVM()->GetFactory()->
                                                NewTaggedArray(elementsLength, JSTaggedValue::Undefined());
            for (int i = 0; i < elementsLength; ++i) {
                JSTaggedValue value = ElementAccessor::Get(obj, i);
                newElements->Set(thread, i, value);
            }
            elements = *newElements;
        } else {
            elements = TaggedArray::Cast(argList->GetElements().GetTaggedObject());
        }
        size_t length = argList->GetArrayLength();
        if (elements->GetLength() == 0 && length != 0) {
            JSHandle<TaggedArray> array =
                thread->GetEcmaVM()->GetFactory()->NewTaggedArray(length, JSTaggedValue::Undefined());
            return std::make_pair(*array, length);
        }
        size_t res = MakeArgListWithHole(thread, elements, length);
        return std::make_pair(elements, res);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}
}  // anonymous namespace

// ecma 19.2.3.1 Function.prototype.apply (thisArg, argArray)
JSTaggedValue BuiltinsFunction::FunctionPrototypeApply(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Function, PrototypeApply);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> func = GetThis(argv);
    JSHandle<JSTaggedValue> thisArg = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> arrayObj = GetCallArg(argv, 1);
    return FunctionPrototypeApplyInternal(thread, func, thisArg, arrayObj);
}

JSTaggedValue BuiltinsFunction::FunctionPrototypeApplyInternal(JSThread *thread, JSHandle<JSTaggedValue> func,
                                                               JSHandle<JSTaggedValue> thisArg,
                                                               JSHandle<JSTaggedValue> arrayObj)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. If IsCallable(func) is false, throw a TypeError exception.
    if (!func->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "apply target is not callable", JSTaggedValue::Exception());
    }

    // 2. If argArray is null or undefined, then
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    if (arrayObj->IsUndefined()) {  // null will also get undefined
        // a. Return Call(func, thisArg).
        EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, func, thisArg, undefined, 0);
        return JSFunction::Call(info);
    }
    // 3. Let argList be CreateListFromArrayLike(argArray).
    std::pair<TaggedArray*, size_t> argumentsList = BuildArgumentsListFast(thread, arrayObj);
    if (!argumentsList.first) {
        JSHandle<JSTaggedValue> num = JSObject::CreateListFromArrayLike(thread, arrayObj);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<TaggedArray> argList = JSHandle<TaggedArray>::Cast(num);
        // 4. ReturnIfAbrupt(argList).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        const uint32_t argsLength = argList->GetLength();
        EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, func, thisArg, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(argsLength, argList);
        return JSFunction::Call(info);
    }
    // 6. Return Call(func, thisArg, argList).
    const uint32_t argsLength = static_cast<uint32_t>(argumentsList.second);
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, func, thisArg, undefined, argsLength);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    info->SetCallArg(argsLength, argumentsList.first);
    return JSFunction::Call(info);
}

// ecma 19.2.3.2 Function.prototype.bind (thisArg , ...args)
JSTaggedValue BuiltinsFunction::FunctionPrototypeBind(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Function, PrototypeBind);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. Let Target be the this value.
    JSHandle<JSTaggedValue> target = GetThis(argv);

    JSHandle<JSTaggedValue> thisArg = GetCallArg(argv, 0);
    uint32_t argsLength = 0;
    if (argv->GetArgsNumber() > 1) {
        argsLength = argv->GetArgsNumber() - 1;
    }

    // 3. Let args be a new (possibly empty) List consisting of all of the argument
    //    values provided after thisArg in order.
    JSHandle<TaggedArray> argsArray = factory->NewTaggedArray(argsLength);
    for (uint32_t index = 0; index < argsLength; ++index) {
        argsArray->Set(thread, index, GetCallArg(argv, index + 1));
    }

    return FunctionPrototypeBindInternal(thread, target, thisArg, argsArray);
}

JSTaggedValue BuiltinsFunction::FunctionPrototypeBindInternal(JSThread *thread, JSHandle<JSTaggedValue> target,
                                                              JSHandle<JSTaggedValue> thisArg,
                                                              JSHandle<TaggedArray> argsArray)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 2. If IsCallable(Target) is false, throw a TypeError exception.
    if (!target->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "bind target is not callable", JSTaggedValue::Exception());
    }

    // 4. Let F be BoundFunctionCreate(Target, thisArg, args).
    JSHandle<JSBoundFunction> boundFunction = factory->NewJSBoundFunction(target, thisArg, argsArray);
    // 5. ReturnIfAbrupt(F)
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    auto globalConst = thread->GlobalConstants();
    JSTaggedValue functionLengthAccessor = globalConst->GetFunctionLengthAccessor();
    JSHandle<JSTaggedValue> lengthKey = globalConst->GetHandledLengthString();
    JSTaggedValue lengthProperty;
    if (target->IsBoundFunction() || target->IsJSFunction()) { // fastpath
        JSHandle<JSObject> obj(thread, target.GetTaggedValue());
        uint32_t numberOfInlinedProps = obj->GetJSHClass()->GetInlinedProperties();
        if (JSFunction::LENGTH_INLINE_PROPERTY_INDEX < numberOfInlinedProps) {
            lengthProperty = obj->GetPropertyInlinedProps(JSFunction::LENGTH_INLINE_PROPERTY_INDEX);
        }
    }
    if (lengthProperty.IsHole()) {
        lengthProperty = ObjectFastOperator::FastGetPropertyByName(
            thread, target.GetTaggedValue(), lengthKey.GetTaggedValue());
    }

    if (!lengthProperty.IsAccessor() || functionLengthAccessor != lengthProperty) {
        // 6. Let targetHasLength be HasOwnProperty(Target, "length").
        bool targetHasLength = JSTaggedValue::HasOwnProperty(thread, target, lengthKey);
        // 7. ReturnIfAbrupt(targetHasLength).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

        double lengthValue = 0.0;
        // 8. If targetHasLength is true, then
        if (targetHasLength) {
            // a. Let targetLen be Get(Target, "length").
            JSHandle<JSTaggedValue> targetLen = JSTaggedValue::GetProperty(thread, target, lengthKey).GetValue();
            // b. ReturnIfAbrupt(targetLen).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

            // c. If Type(targetLen) is not Number, let L be 0.
            // d. Else,
            //    i. Let targetLen be ToInteger(targetLen).
            //    ii. Let L be the larger of 0 and the result of targetLen minus the number of elements of args.
            if (targetLen->IsNumber()) {
                // argv include thisArg
                lengthValue =
                    std::max(0.0, JSTaggedValue::ToNumber(thread, targetLen).GetNumber() -
                             static_cast<double>(argsArray->GetLength()));
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
        }
        // 9. Else let L be 0.

        // 10. Let status be DefinePropertyOrThrow(F, "length", PropertyDescriptor {[[Value]]: L,
        //     [[Writable]]: false, [[Enumerable]]: false, [[Configurable]]: true}).
        PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(lengthValue)),
                                false, false, true);
        [[maybe_unused]] bool status =
            JSTaggedValue::DefinePropertyOrThrow(thread, JSHandle<JSTaggedValue>(boundFunction), lengthKey, desc);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // 11. Assert: status is not an abrupt completion.
        ASSERT_PRINT(status, "DefinePropertyOrThrow failed");
    }

    JSTaggedValue functionNameAccessor = globalConst->GetFunctionNameAccessor();
    JSHandle<JSTaggedValue> nameKey = globalConst->GetHandledNameString();
    JSTaggedValue nameProperty;
    if (target->IsBoundFunction() || target->IsJSFunction()) { // fastpath
        JSHandle<JSObject> obj(thread, target.GetTaggedValue());
        uint32_t numberOfInlinedProps = obj->GetJSHClass()->GetInlinedProperties();
        if (JSFunction::NAME_INLINE_PROPERTY_INDEX < numberOfInlinedProps) {
            nameProperty = obj->GetPropertyInlinedProps(JSFunction::NAME_INLINE_PROPERTY_INDEX);
        }
    }
    if (nameProperty.IsHole()) {
        nameProperty = ObjectFastOperator::FastGetPropertyByName(
            thread, target.GetTaggedValue(), nameKey.GetTaggedValue());
    }

    if (!nameProperty.IsAccessor() || functionNameAccessor != nameProperty) {
        // 12. Let targetName be Get(Target, "name").
        JSHandle<JSTaggedValue> targetName = JSObject::GetProperty(thread, target, nameKey).GetValue();
        // 13. ReturnIfAbrupt(targetName).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

        JSHandle<JSTaggedValue> boundName = thread->GlobalConstants()->GetHandledBoundString();
        // 14. If Type(targetName) is not String, let targetName be the empty string.
        // 15. Perform SetFunctionName(F, targetName, "bound").
        [[maybe_unused]] bool status;
        if (!targetName->IsString()) {
            JSHandle<JSTaggedValue> emptyString(factory->GetEmptyString());
            status = JSFunction::SetFunctionName(thread, JSHandle<JSFunctionBase>(boundFunction),
                                                 emptyString, boundName);
        } else {
            status = JSFunction::SetFunctionName(thread, JSHandle<JSFunctionBase>(boundFunction),
                                                 targetName, boundName);
        }
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // Assert: status is not an abrupt completion.
        ASSERT_PRINT(status, "DefinePropertyOr failed");
    }

    // 16. Return F.
    return boundFunction.GetTaggedValue();
}

// ecma 19.2.3.3 Function.prototype.call (thisArg , ...args)
JSTaggedValue BuiltinsFunction::FunctionPrototypeCall(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Function, PrototypeCall);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. If IsCallable(func) is false, throw a TypeError exception.
    if (!GetThis(argv)->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "call target is not callable", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> func = GetThis(argv);
    JSHandle<JSTaggedValue> thisArg = GetCallArg(argv, 0);
    uint32_t argsLength = 0;
    if (argv->GetArgsNumber() > 1) {
        argsLength = argv->GetArgsNumber() - 1;
    }
    // 2. Let argList be an empty List.
    // 3. If this method was called with more than one argument then in left to right order,
    //    starting with the second argument, append each argument as the last element of argList.
    // 5. Return Call(func, thisArg, argList).
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, func, thisArg, undefined, argsLength);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    info->SetCallArg(argsLength, 0, argv, 1);
    return JSFunction::Call(info);
}

// ecma 19.2.3.5 Function.prototype.toString ()
JSTaggedValue BuiltinsFunction::FunctionPrototypeToString(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Function, PrototypeToString);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);
    if (thisValue->IsJSObject() && thisValue->IsCallable()) {
        JSHandle<Method> method;
        if (thisValue->IsBoundFunction()) {
            std::string str = "function () { [native code] }";
            return GetTaggedString(thread, str.c_str());
        } else {
            JSHandle<JSFunction> func = JSHandle<JSFunction>::Cast(thisValue);
            JSHandle<JSTaggedValue> methodHandle(thread, func->GetMethod());
            method = JSHandle<Method>::Cast(methodHandle);
        }
        if (method->IsNativeWithCallField()) {
            JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();
            JSHandle<JSTaggedValue> name = JSObject::GetProperty(thread, thisValue, nameKey).GetValue();
            JSHandle<EcmaString> methodName = JSTaggedValue::ToString(thread, name);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            std::string nameStr = EcmaStringAccessor(methodName).ToStdString();
            std::string startStr = "function ";
            std::string endStr = "() { [native code] }";
            startStr.append(nameStr).append(endStr);
            return GetTaggedString(thread, startStr.c_str());
        }
        DebugInfoExtractor *debugExtractor =
                JSPandaFileManager::GetInstance()->GetJSPtExtractor(method->GetJSPandaFile());
        const std::string &sourceCode = debugExtractor->GetSourceCode(method->GetMethodId());
        if (!sourceCode.empty()) {
            return GetTaggedString(thread, sourceCode.c_str());
        } else {
            return GetTaggedString(thread, "Cannot get source code of funtion");
        }
    }

    THROW_TYPE_ERROR_AND_RETURN(thread,
        "function.toString() target is incompatible object", JSTaggedValue::Exception());
}

// ecma 19.2.3.6 Function.prototype[@@hasInstance] (V)
JSTaggedValue BuiltinsFunction::FunctionPrototypeHasInstance(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Function, PrototypeHasInstance);
    [[maybe_unused]] EcmaHandleScope handleScope(argv->GetThread());
    // 1. Let F be the this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);
    // 2. Return OrdinaryHasInstance(F, V).
    JSHandle<JSTaggedValue> arg = GetCallArg(argv, 0);
    return JSFunction::OrdinaryHasInstance(argv->GetThread(), thisValue, arg) ? GetTaggedBoolean(true)
                                                                              : GetTaggedBoolean(false);
}
}  // namespace panda::ecmascript::builtins
