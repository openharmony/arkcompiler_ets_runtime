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

#include "ecmascript/js_function.h"

#include "ecmascript/base/error_type.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/jspandafile/class_info_extractor.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/require/js_require_manager.h"

namespace panda::ecmascript {
void JSFunction::InitializeJSFunction(JSThread *thread, const JSHandle<JSFunction> &func, FunctionKind kind)
{
    func->SetProtoOrHClass(thread, JSTaggedValue::Hole(), SKIP_BARRIER);
    func->SetHomeObject(thread, JSTaggedValue::Undefined(), SKIP_BARRIER);
    func->SetLexicalEnv(thread, JSTaggedValue::Undefined(), SKIP_BARRIER);
    func->SetModule(thread, JSTaggedValue::Undefined(), SKIP_BARRIER);
    func->SetMethod(thread, JSTaggedValue::Undefined(), SKIP_BARRIER);

    auto globalConst = thread->GlobalConstants();
    if (HasPrototype(kind)) {
        JSHandle<JSTaggedValue> accessor = globalConst->GetHandledFunctionPrototypeAccessor();
        if (kind == FunctionKind::BASE_CONSTRUCTOR || kind == FunctionKind::GENERATOR_FUNCTION ||
            kind == FunctionKind::ASYNC_GENERATOR_FUNCTION) {
            func->SetPropertyInlinedProps(thread, PROTOTYPE_INLINE_PROPERTY_INDEX, accessor.GetTaggedValue());
            accessor = globalConst->GetHandledFunctionNameAccessor();
            func->SetPropertyInlinedProps(thread, NAME_INLINE_PROPERTY_INDEX, accessor.GetTaggedValue());
            JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            JSHandle<JSFunction> objFun(env->GetObjectFunction());
            JSHandle<JSObject> initialGeneratorFuncPrototype = factory->NewJSObjectByConstructor(objFun);
            if (kind == FunctionKind::ASYNC_GENERATOR_FUNCTION) {
                JSObject::SetPrototype(thread, initialGeneratorFuncPrototype, env->GetAsyncGeneratorPrototype());
                func->SetProtoOrHClass(thread, initialGeneratorFuncPrototype);
            }
            if (kind == FunctionKind::GENERATOR_FUNCTION) {
                JSObject::SetPrototype(thread, initialGeneratorFuncPrototype, env->GetGeneratorPrototype());
                func->SetProtoOrHClass(thread, initialGeneratorFuncPrototype);
            }
        } else if (!JSFunction::IsClassConstructor(kind)) {  // class ctor do nothing
            PropertyDescriptor desc(thread, accessor, kind != FunctionKind::BUILTIN_CONSTRUCTOR, false, false);
            [[maybe_unused]] bool success = JSObject::DefineOwnProperty(thread, JSHandle<JSObject>(func),
                                                                        globalConst->GetHandledPrototypeString(), desc);
            ASSERT(success);
        }
    } else if (HasAccessor(kind)) {
        JSHandle<JSTaggedValue> accessor = globalConst->GetHandledFunctionNameAccessor();
        func->SetPropertyInlinedProps(thread, NAME_INLINE_PROPERTY_INDEX, accessor.GetTaggedValue());
    }
}

JSHandle<JSObject> JSFunction::NewJSFunctionPrototype(JSThread *thread, const JSHandle<JSFunction> &func)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSFunction> objFun(env->GetObjectFunction());
    JSHandle<JSObject> funPro = thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(objFun);
    func->SetFunctionPrototype(thread, funPro.GetTaggedValue());

    // set "constructor" in prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    PropertyDescriptor descriptor(thread, JSHandle<JSTaggedValue>::Cast(func), true, false, true);
    JSObject::DefineOwnProperty(thread, funPro, constructorKey, descriptor);

    return funPro;
}

JSHClass *JSFunction::GetOrCreateInitialJSHClass(JSThread *thread, const JSHandle<JSFunction> &fun)
{
    JSTaggedValue protoOrHClass(fun->GetProtoOrHClass());
    if (protoOrHClass.IsJSHClass()) {
        return reinterpret_cast<JSHClass *>(protoOrHClass.GetTaggedObject());
    }

    JSHandle<JSTaggedValue> proto;
    if (!fun->HasFunctionPrototype()) {
        proto = JSHandle<JSTaggedValue>::Cast(NewJSFunctionPrototype(thread, fun));
    } else {
        proto = JSHandle<JSTaggedValue>(thread, fun->GetProtoOrHClass());
    }

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> hclass = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT, proto);
    fun->SetProtoOrHClass(thread, hclass);
    return *hclass;
}

JSTaggedValue JSFunction::PrototypeGetter(JSThread *thread, const JSHandle<JSObject> &self)
{
    JSHandle<JSFunction> func = JSHandle<JSFunction>::Cast(self);
    if (!func->HasFunctionPrototype()) {
        NewJSFunctionPrototype(thread, func);
    }
    return JSFunction::Cast(*self)->GetFunctionPrototype();
}

bool JSFunction::PrototypeSetter(JSThread *thread, const JSHandle<JSObject> &self, const JSHandle<JSTaggedValue> &value,
                                 [[maybe_unused]] bool mayThrow)
{
    JSHandle<JSFunction> func(self);
    JSTaggedValue protoOrHClass = func->GetProtoOrHClass();
    if (protoOrHClass.IsJSHClass()) {
        // need transition
        JSHandle<JSHClass> hclass(thread, protoOrHClass);
        JSHandle<JSHClass> newClass = JSHClass::TransitionProto(thread, hclass, value);
        if (value->IsECMAObject()) {
            JSObject::Cast(value->GetTaggedObject())->GetJSHClass()->SetIsPrototype(true);
        }
        func->SetProtoOrHClass(thread, newClass);
    } else {
        func->SetFunctionPrototype(thread, value.GetTaggedValue());
    }
    return true;
}

JSTaggedValue JSFunction::NameGetter(JSThread *thread, const JSHandle<JSObject> &self)
{
    JSTaggedValue method = JSHandle<JSFunction>::Cast(self)->GetMethod();
    if (method.IsUndefined()) {
        return JSTaggedValue::Undefined();
    }
    Method *target = Method::Cast(method.GetTaggedObject());
    if (target->IsNativeWithCallField()) {
        return JSTaggedValue::Undefined();
    }
    std::string funcName = target->ParseFunctionName();
    if (funcName.empty()) {
        return thread->GlobalConstants()->GetEmptyString();
    }
    if (JSHandle<JSFunction>::Cast(self)->GetFunctionKind() == FunctionKind::GETTER_FUNCTION) {
        funcName.insert(0, "get ");
    }
    if (JSHandle<JSFunction>::Cast(self)->GetFunctionKind() == FunctionKind::SETTER_FUNCTION) {
        funcName.insert(0, "set ");
    }

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->NewFromStdString(funcName).GetTaggedValue();
}

bool JSFunction::OrdinaryHasInstance(JSThread *thread, const JSHandle<JSTaggedValue> &constructor,
                                     const JSHandle<JSTaggedValue> &obj)
{
    // 1. If IsCallable(C) is false, return false.
    if (!constructor->IsCallable()) {
        return false;
    }

    // 2. If C has a [[BoundTargetFunction]] internal slot, then
    //    a. Let BC be the value of C's [[BoundTargetFunction]] internal slot.
    //    b. Return InstanceofOperator(O,BC)  (see 12.9.4).
    if (constructor->IsBoundFunction()) {
        JSHandle<JSBoundFunction> boundFunction(thread, JSBoundFunction::Cast(constructor->GetTaggedObject()));
        JSTaggedValue boundTarget = boundFunction->GetBoundTarget();
        return JSObject::InstanceOf(thread, obj, JSHandle<JSTaggedValue>(thread, boundTarget));
    }
    // 3. If Type(O) is not Object, return false
    if (!obj->IsECMAObject()) {
        return false;
    }

    // 4. Let P be Get(C, "prototype").
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> constructorPrototype =
        JSTaggedValue::GetProperty(thread, constructor, globalConst->GetHandledPrototypeString()).GetValue();

    // 5. ReturnIfAbrupt(P).
    // no throw exception, so needn't return
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    // 6. If Type(P) is not Object, throw a TypeError exception.
    if (!constructorPrototype->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "HasInstance: is not Object", false);
    }

    // 7. Repeat
    //    a.Let O be O.[[GetPrototypeOf]]().
    //    b.ReturnIfAbrupt(O).
    //    c.If O is null, return false.
    //    d.If SameValue(P, O) is true, return true.
    JSMutableHandle<JSTaggedValue> object(thread, obj.GetTaggedValue());
    while (!object->IsNull()) {
        if (JSTaggedValue::SameValue(object, constructorPrototype)) {
            return true;
        }
        object.Update(JSTaggedValue::GetPrototype(thread, object));
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    }
    return false;
}

bool JSFunction::MakeConstructor(JSThread *thread, const JSHandle<JSFunction> &func,
                                 const JSHandle<JSTaggedValue> &proto, bool writable)
{
    ASSERT_PRINT(proto->IsHeapObject() || proto->IsUndefined(), "proto must be JSObject or Undefined");
    ASSERT_PRINT(func->IsConstructor(), "func must be Constructor type");
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();

    ASSERT_PRINT(func->GetProtoOrHClass().IsHole() && func->IsExtensible(),
                 "function doesn't has proto_type property and is extensible object");
    ASSERT_PRINT(JSObject::HasProperty(thread, JSHandle<JSObject>(func), constructorKey),
                 "function must have constructor");

    // proto.constructor = func
    bool status = false;
    if (proto->IsUndefined()) {
        // Let prototype be ObjectCreate(%ObjectPrototype%).
        JSHandle<JSTaggedValue> objPrototype = env->GetObjectFunctionPrototype();
        PropertyDescriptor constructorDesc(thread, JSHandle<JSTaggedValue>::Cast(func), writable, false, true);
        status = JSTaggedValue::DefinePropertyOrThrow(thread, objPrototype, constructorKey, constructorDesc);
    } else {
        PropertyDescriptor constructorDesc(thread, JSHandle<JSTaggedValue>::Cast(func), writable, false, true);
        status = JSTaggedValue::DefinePropertyOrThrow(thread, proto, constructorKey, constructorDesc);
    }
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    ASSERT_PRINT(status, "DefineProperty construct failed");
    // func.prototype = proto
    // Let status be DefinePropertyOrThrow(F, "prototype", PropertyDescriptor{[[Value]]:
    // prototype, [[Writable]]: writablePrototype, [[Enumerable]]: false, [[Configurable]]: false}).
    func->SetFunctionPrototype(thread, proto.GetTaggedValue());

    ASSERT_PRINT(status, "DefineProperty proto_type failed");
    return status;
}

JSTaggedValue JSFunction::Call(EcmaRuntimeCallInfo *info)
{
    if (info == nullptr) {
        return JSTaggedValue::Exception();
    }

    JSThread *thread = info->GetThread();
    // 1. ReturnIfAbrupt(F).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> func = info->GetFunction();
    // 2. If argumentsList was not passed, let argumentsList be a new empty List.
    // 3. If IsCallable(F) is false, throw a TypeError exception.
    if (!func->IsCallable()) {
        RETURN_STACK_BEFORE_THROW_IF_ASM(thread);
        THROW_TYPE_ERROR_AND_RETURN(thread, "Callable is false", JSTaggedValue::Exception());
    }

    auto *hclass = func->GetTaggedObject()->GetClass();
    if (hclass->IsClassConstructor()) {
        RETURN_STACK_BEFORE_THROW_IF_ASM(thread);
        THROW_TYPE_ERROR_AND_RETURN(thread, "class constructor cannot call", JSTaggedValue::Exception());
    }
    return EcmaInterpreter::Execute(info);
}

JSTaggedValue JSFunction::Construct(EcmaRuntimeCallInfo *info)
{
    if (info == nullptr) {
        return JSTaggedValue::Exception();
    }

    JSThread *thread = info->GetThread();
    JSHandle<JSTaggedValue> func(info->GetFunction());
    JSHandle<JSTaggedValue> target = info->GetNewTarget();
    if (target->IsUndefined()) {
        target = func;
        info->SetNewTarget(target.GetTaggedValue());
    }
    if (!(func->IsConstructor() && target->IsConstructor())) {
        RETURN_STACK_BEFORE_THROW_IF_ASM(thread);
        THROW_TYPE_ERROR_AND_RETURN(thread, "Constructor is false", JSTaggedValue::Exception());
    }

    if (func->IsJSFunction()) {
        return JSFunction::ConstructInternal(info);
    } else if (func->IsJSProxy()) {
        return JSProxy::ConstructInternal(info);
    } else {
        ASSERT(func->IsBoundFunction());
        return JSBoundFunction::ConstructInternal(info);
    }
}

JSTaggedValue JSFunction::Invoke(EcmaRuntimeCallInfo *info, const JSHandle<JSTaggedValue> &key)
{
    if (info == nullptr) {
        return JSTaggedValue::Exception();
    }

    ASSERT(JSTaggedValue::IsPropertyKey(key));
    JSThread *thread = info->GetThread();
    JSHandle<JSTaggedValue> thisArg = info->GetThis();
    JSHandle<JSTaggedValue> func(JSTaggedValue::GetProperty(thread, thisArg, key).GetValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    info->SetFunction(func.GetTaggedValue());
    return JSFunction::Call(info);
}

JSTaggedValue JSFunction::InvokeOptimizedEntrypoint(JSThread *thread, JSHandle<JSFunction> mainFunc,
    JSHandle<JSTaggedValue> &thisArg, std::string_view entryPoint, CJSInfo* cjsInfo)
{
    if (mainFunc->IsClassConstructor()) {
        {
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            JSHandle<JSObject> error =
                factory->GetJSError(ErrorType::TYPE_ERROR, "class constructor cannot called without 'new'");
            thread->SetException(error.GetTaggedValue());
        }
        return thread->GetException();
    }
    Method *method = mainFunc->GetCallTarget();
    size_t actualNumArgs = method->GetNumArgs();
    const JSTaggedType *prevFp = thread->GetLastLeaveFrame();
    JSTaggedValue res;
    std::vector<JSTaggedType> args;
#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    RuntimeStubs::StartCallTimer(thread->GetGlueAddr(), mainFunc.GetTaggedType(), true);
#endif
    if (method->IsFastCall()) {
        // do not modify this log to INFO, this will call many times
        LOG_ECMA(DEBUG) << "start to execute aot entry: " << entryPoint;
        args = JSFunction::GetArgsData(true, thisArg, mainFunc, cjsInfo);
        res = thread->GetEcmaVM()->FastCallAot(actualNumArgs, args.data(), prevFp);
    } else {
        args = JSFunction::GetArgsData(false, thisArg, mainFunc, cjsInfo);
        // do not modify this log to INFO, this will call many times
        LOG_ECMA(DEBUG) << "start to execute aot entry: " << entryPoint;
        res = thread->GetCurrentEcmaContext()->ExecuteAot(actualNumArgs, args.data(), prevFp, false);
    }
#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    RuntimeStubs::EndCallTimer(thread->GetGlueAddr(), mainFunc.GetTaggedType());
#endif
    if (thread->HasPendingException()) {
        return thread->GetException();
    }
    return res;
}

std::vector<JSTaggedType> JSFunction::GetArgsData(bool isFastCall, JSHandle<JSTaggedValue> &thisArg,
    JSHandle<JSFunction> mainFunc, CJSInfo* cjsInfo)
{
    size_t argsNum;
    uint32_t mandatoryNum;
    Method *method = mainFunc->GetCallTarget();
    size_t actualNumArgs = method->GetNumArgs();
    if (isFastCall) {
        argsNum = actualNumArgs + NUM_MANDATORY_JSFUNC_ARGS - 1;
        mandatoryNum = NUM_MANDATORY_JSFUNC_ARGS - 1;
    } else {
        argsNum = actualNumArgs + NUM_MANDATORY_JSFUNC_ARGS;
        mandatoryNum = NUM_MANDATORY_JSFUNC_ARGS;
    }
    std::vector<JSTaggedType> args(argsNum, JSTaggedValue::Undefined().GetRawData());
    args[0] = mainFunc.GetTaggedValue().GetRawData();
    if (isFastCall) {
        args[1] = thisArg.GetTaggedValue().GetRawData(); // 1: args number
    } else {
        args[2] = thisArg.GetTaggedValue().GetRawData(); // 2: args number
    }
    if (cjsInfo != nullptr) {
        args[mandatoryNum++] = cjsInfo->exportsHdl.GetTaggedValue().GetRawData();
        args[mandatoryNum++] = cjsInfo->requireHdl.GetTaggedValue().GetRawData();
        args[mandatoryNum++] = cjsInfo->moduleHdl.GetTaggedValue().GetRawData();
        args[mandatoryNum++] = cjsInfo->filenameHdl.GetTaggedValue().GetRawData();
        args[mandatoryNum] = cjsInfo->dirnameHdl.GetTaggedValue().GetRawData();
    }
    return args;
}

JSTaggedValue JSFunction::InvokeOptimizedEntrypoint(JSThread *thread, JSHandle<JSFunction> func,
    EcmaRuntimeCallInfo *info)
{
    Method *method = func->GetCallTarget();
    JSTaggedValue resultValue;
    uint32_t numArgs = method->GetNumArgsWithCallField();
    bool needPushUndefined = numArgs > info->GetArgsNumber();
    const JSTaggedType *prevFp = thread->GetLastLeaveFrame();
#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    RuntimeStubs::StartCallTimer(thread->GetGlueAddr(), func.GetTaggedType(), true);
#endif
    if (method->IsFastCall()) {
        if (needPushUndefined) {
            info = EcmaInterpreter::ReBuildRuntimeCallInfo(thread, info, numArgs);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        JSTaggedType *stackArgs = info->GetArgs();
        stackArgs[1] = stackArgs[0];
        resultValue = thread->GetEcmaVM()->FastCallAot(info->GetArgsNumber(), stackArgs + 1, prevFp);
    } else {
        resultValue = thread->GetCurrentEcmaContext()->ExecuteAot(info->GetArgsNumber(),
            info->GetArgs(), prevFp, needPushUndefined);
    }
#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    RuntimeStubs::EndCallTimer(thread->GetGlueAddr(), func.GetTaggedType());
#endif
    return resultValue;
}

// [[Construct]]
JSTaggedValue JSFunction::ConstructInternal(EcmaRuntimeCallInfo *info)
{
    if (info == nullptr) {
        return JSTaggedValue::Exception();
    }

    JSThread *thread = info->GetThread();
    JSHandle<JSFunction> func(info->GetFunction());
    JSHandle<JSTaggedValue> newTarget(info->GetNewTarget());
    ASSERT(newTarget->IsECMAObject());
    if (!func->IsConstructor()) {
        RETURN_STACK_BEFORE_THROW_IF_ASM(thread);
        THROW_TYPE_ERROR_AND_RETURN(thread, "Constructor is false", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> obj(thread, JSTaggedValue::Undefined());
    if (func->IsBase()) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        obj = JSHandle<JSTaggedValue>(factory->NewJSObjectByConstructor(func, newTarget));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }

    JSTaggedValue resultValue;
    info->SetThis(obj.GetTaggedValue());
    Method *method = func->GetCallTarget();
    if (method->IsAotWithCallField() && func->IsClassConstructor()) {
        resultValue = InvokeOptimizedEntrypoint(thread, func, info);
        const JSTaggedType *curSp = thread->GetCurrentSPFrame();
        InterpretedEntryFrame *entryState = InterpretedEntryFrame::GetFrameFromSp(curSp);
        JSTaggedType *prevSp = entryState->base.prev;
        thread->SetCurrentSPFrame(prevSp);
    } else {
        method->SetAotCodeBit(false); // if Construct is not ClassConstructor, don't run aot
        resultValue = EcmaInterpreter::Execute(info);
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 9.3.2 [[Construct]] (argumentsList, newTarget)
    if (resultValue.IsECMAObject()) {
        return resultValue;
    }

    if (func->IsBase()) {
        return obj.GetTaggedValue();
    }

    // derived ctor(sub class) return the obj which created by base ctor(parent class)
    if (func->IsDerivedConstructor()) {
        return resultValue;
    }

    if (!resultValue.IsUndefined()) {
        RETURN_STACK_BEFORE_THROW_IF_ASM(thread);
        THROW_TYPE_ERROR_AND_RETURN(thread, "function is non-constructor", JSTaggedValue::Exception());
    }
    return obj.GetTaggedValue();
}

JSHandle<JSTaggedValue> JSFunctionBase::GetFunctionName(JSThread *thread, const JSHandle<JSFunctionBase> &func)
{
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    return JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(func), nameKey).GetValue();
}

bool JSFunctionBase::SetFunctionName(JSThread *thread, const JSHandle<JSFunctionBase> &func,
                                     const JSHandle<JSTaggedValue> &name, const JSHandle<JSTaggedValue> &prefix)
{
    ASSERT_PRINT(func->IsExtensible(), "Function must be extensible");
    ASSERT_PRINT(name->IsStringOrSymbol(), "name must be string or symbol");
    bool needPrefix = false;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    if (!prefix->IsUndefined()) {
        ASSERT_PRINT(prefix->IsString(), "prefix must be string");
        needPrefix = true;
    }
    // If Type(name) is Symbol, then
    // Let description be name’s [[Description]] value.
    // If description is undefined, let name be the empty String.
    // Else, let name be the concatenation of "[", description, and "]".
    JSHandle<EcmaString> functionName;
    if (name->IsSymbol()) {
        JSTaggedValue description = JSHandle<JSSymbol>::Cast(name)->GetDescription();
        JSHandle<EcmaString> descriptionHandle(thread, description);
        if (description.IsUndefined()) {
            functionName = factory->GetEmptyString();
        } else {
            JSHandle<EcmaString> leftBrackets(globalConst->GetHandledLeftSquareBracketString());
            JSHandle<EcmaString> rightBrackets(globalConst->GetHandledRightSquareBracketString());
            functionName = factory->ConcatFromString(leftBrackets, descriptionHandle);
            functionName = factory->ConcatFromString(functionName, rightBrackets);
        }
    } else {
        functionName = JSHandle<EcmaString>::Cast(name);
    }
    EcmaString *newString;
    if (needPrefix) {
        JSHandle<EcmaString> handlePrefixString = JSTaggedValue::ToString(thread, prefix);
        JSHandle<EcmaString> spaceString(globalConst->GetHandledSpaceString());
        JSHandle<EcmaString> concatString = factory->ConcatFromString(handlePrefixString, spaceString);
        newString = *factory->ConcatFromString(concatString, functionName);
    } else {
        newString = *functionName;
    }
    JSHandle<JSTaggedValue> nameHandle(thread, newString);
    JSHandle<JSTaggedValue> nameKey = globalConst->GetHandledNameString();
    PropertyDescriptor nameDesc(thread, nameHandle, false, false, true);
    JSHandle<JSTaggedValue> funcHandle(func);
    return JSTaggedValue::DefinePropertyOrThrow(thread, funcHandle, nameKey, nameDesc);
}

bool JSFunction::SetFunctionLength(JSThread *thread, const JSHandle<JSFunction> &func, JSTaggedValue length, bool cfg)
{
    ASSERT_PRINT(func->IsExtensible(), "Function must be extensible");
    ASSERT_PRINT(length.IsInteger(), "length must be integer");
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    ASSERT_PRINT(!JSTaggedValue::Less(thread, JSHandle<JSTaggedValue>(thread, length),
                                      JSHandle<JSTaggedValue>(thread, JSTaggedValue(0))),
                 "length must be non negative integer");
    PropertyDescriptor lengthDesc(thread, JSHandle<JSTaggedValue>(thread, length), false, false, cfg);
    JSHandle<JSTaggedValue> funcHandle(func);
    return JSTaggedValue::DefinePropertyOrThrow(thread, funcHandle, lengthKeyHandle, lengthDesc);
}

// 9.4.1.2[[Construct]](argumentsList, newTarget)
JSTaggedValue JSBoundFunction::ConstructInternal(EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    JSHandle<JSBoundFunction> func(info->GetFunction());
    JSHandle<JSTaggedValue> target(thread, func->GetBoundTarget());
    ASSERT(target->IsConstructor());
    JSHandle<JSTaggedValue> newTarget = info->GetNewTarget();
    JSMutableHandle<JSTaggedValue> newTargetMutable(thread, newTarget.GetTaggedValue());
    if (JSTaggedValue::SameValue(func.GetTaggedValue(), newTarget.GetTaggedValue())) {
        newTargetMutable.Update(target.GetTaggedValue());
    }

    JSHandle<TaggedArray> boundArgs(thread, func->GetBoundArguments());
    const uint32_t boundLength = boundArgs->GetLength();
    const uint32_t argsLength = info->GetArgsNumber() + boundLength;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *runtimeInfo =
        EcmaInterpreter::NewRuntimeCallInfo(thread, target, undefined, newTargetMutable, argsLength);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (boundLength == 0) {
        runtimeInfo->SetCallArg(argsLength, 0, info, 0);
    } else {
        // 0 ~ boundLength is boundArgs; boundLength ~ argsLength is args of EcmaRuntimeCallInfo.
        runtimeInfo->SetCallArg(boundLength, boundArgs);
        runtimeInfo->SetCallArg(argsLength, boundLength, info, 0);
    }
    return JSFunction::Construct(runtimeInfo);
}

void JSProxyRevocFunction::ProxyRevocFunctions(const JSThread *thread, const JSHandle<JSProxyRevocFunction> &revoker)
{
    // 1.Let p be the value of F’s [[RevocableProxy]] internal slot.
    JSTaggedValue proxy = revoker->GetRevocableProxy();
    // 2.If p is null, return undefined.
    if (proxy.IsNull()) {
        return;
    }

    // 3.Set the value of F’s [[RevocableProxy]] internal slot to null.
    revoker->SetRevocableProxy(thread, JSTaggedValue::Null());

    // 4.Assert: p is a Proxy object.
    ASSERT(proxy.IsJSProxy());
    JSHandle<JSProxy> proxyHandle(thread, proxy);

    // 5 ~ 6 Set internal slot of p to null.
    proxyHandle->SetTarget(thread, JSTaggedValue::Null());
    proxyHandle->SetHandler(thread, JSTaggedValue::Null());
    proxyHandle->SetIsRevoked(true);
}

JSTaggedValue JSFunction::AccessCallerArgumentsThrowTypeError(EcmaRuntimeCallInfo *argv)
{
    THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(),
                                "Under strict mode, 'caller' and 'arguments' properties must not be accessed.",
                                JSTaggedValue::Exception());
}

JSTaggedValue JSIntlBoundFunction::IntlNameGetter(JSThread *thread, [[maybe_unused]] const JSHandle<JSObject> &self)
{
    return thread->GlobalConstants()->GetEmptyString();
}

void JSFunction::SetFunctionNameNoPrefix(JSThread *thread, JSFunction *func, JSTaggedValue name)
{
    ASSERT_PRINT(func->IsExtensible(), "Function must be extensible");
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    JSHandle<JSTaggedValue> funcHandle(thread, func);
    {
        JSMutableHandle<JSTaggedValue> nameHandle(thread, JSTaggedValue::Undefined());
        if (!name.IsSymbol()) {
            nameHandle.Update(name);
        } else {
            JSHandle<JSTaggedValue> nameBegin(thread, name);
            JSTaggedValue description = JSSymbol::Cast(name.GetTaggedObject())->GetDescription();
            if (description.IsUndefined()) {
                nameHandle.Update(globalConst->GetEmptyString());
            } else {
                JSHandle<EcmaString> leftBrackets(globalConst->GetHandledLeftSquareBracketString());
                JSHandle<EcmaString> rightBrackets(globalConst->GetHandledRightSquareBracketString());
                JSHandle<EcmaString> concatName = factory->ConcatFromString(leftBrackets,
                    JSHandle<EcmaString>(thread, JSSymbol::Cast(nameBegin->GetTaggedObject())->GetDescription()));
                concatName = factory->ConcatFromString(concatName, rightBrackets);
                nameHandle.Update(concatName.GetTaggedValue());
            }
        }
        PropertyDescriptor nameDesc(thread, nameHandle, false, false, true);
        JSTaggedValue::DefinePropertyOrThrow(thread, funcHandle, globalConst->GetHandledNameString(), nameDesc);
    }
}

JSHandle<JSHClass> JSFunction::GetInstanceJSHClass(JSThread *thread, JSHandle<JSFunction> constructor,
                                                   JSHandle<JSTaggedValue> newTarget)
{
    JSHandle<JSHClass> ctorInitialJSHClass(thread, JSFunction::GetOrCreateInitialJSHClass(thread, constructor));
    // newTarget is construct itself
    if (newTarget.GetTaggedValue() == constructor.GetTaggedValue()) {
        return ctorInitialJSHClass;
    }

    // newTarget is derived-class of constructor
    if (newTarget->IsJSFunction()) {
        JSHandle<JSFunction> newTargetFunc = JSHandle<JSFunction>::Cast(newTarget);
        if (newTargetFunc->IsDerivedConstructor()) {
            JSMutableHandle<JSTaggedValue> mutableNewTarget(thread, newTarget.GetTaggedValue());
            JSMutableHandle<JSTaggedValue> mutableNewTargetProto(thread, JSTaggedValue::Undefined());
            while (!mutableNewTargetProto->IsNull()) {
                mutableNewTargetProto.Update(JSTaggedValue::GetPrototype(thread, mutableNewTarget));
                if (mutableNewTargetProto.GetTaggedValue() == constructor.GetTaggedValue()) {
                    return GetOrCreateDerivedJSHClass(thread, newTargetFunc, ctorInitialJSHClass);
                }
                mutableNewTarget.Update(mutableNewTargetProto.GetTaggedValue());
            }
        }
    }

    // ECMA2015 9.1.15 3.Let proto be Get(constructor, "prototype").
    JSMutableHandle<JSTaggedValue> prototype(thread, JSTaggedValue::Undefined());
    if (newTarget->IsJSFunction()) {
        JSHandle<JSFunction> newTargetFunc = JSHandle<JSFunction>::Cast(newTarget);
        FunctionKind kind = newTargetFunc->GetFunctionKind();
        if (HasPrototype(kind)) {
            prototype.Update(PrototypeGetter(thread, JSHandle<JSObject>::Cast(newTargetFunc)));
        }
    } else {
        // Such case: bound function and define a "prototype" property.
        JSHandle<JSTaggedValue> customizePrototype =
            JSTaggedValue::GetProperty(thread, newTarget, thread->GlobalConstants()->GetHandledPrototypeString())
                .GetValue();
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSHClass, thread);
        prototype.Update(customizePrototype.GetTaggedValue());
        // Reload JSHClass of constructor, where the lookup of 'prototype' property may change it.
        ctorInitialJSHClass = JSHandle<JSHClass>(thread, JSFunction::GetOrCreateInitialJSHClass(thread, constructor));
    }

    if (!prototype->IsECMAObject()) {
        prototype.Update(constructor->GetFunctionPrototype());
    }

    JSHandle<JSHClass> newJSHClass = JSHClass::Clone(thread, ctorInitialJSHClass);
    newJSHClass->SetPrototype(thread, prototype);

    return newJSHClass;
}

JSHandle<JSHClass> JSFunction::GetOrCreateDerivedJSHClass(JSThread *thread, JSHandle<JSFunction> derived,
                                                          JSHandle<JSHClass> ctorInitialJSHClass)
{
    JSTaggedValue protoOrHClass(derived->GetProtoOrHClass());
    // has cached JSHClass, return directly
    if (protoOrHClass.IsJSHClass()) {
        return JSHandle<JSHClass>(thread, protoOrHClass);
    }

    JSHandle<JSHClass> newJSHClass = JSHClass::Clone(thread, ctorInitialJSHClass);
    // guarante derived has function prototype
    JSHandle<JSTaggedValue> prototype(thread, derived->GetProtoOrHClass());
    ASSERT(!prototype->IsHole());
    newJSHClass->SetPrototype(thread, prototype);
    derived->SetProtoOrHClass(thread, newJSHClass);
    return newJSHClass;
}

// Those interface below is discarded
void JSFunction::InitializeJSFunction(JSThread *thread, [[maybe_unused]] const JSHandle<GlobalEnv> &env,
                                      const JSHandle<JSFunction> &func, FunctionKind kind)
{
    InitializeJSFunction(thread, func, kind);
}

bool JSFunction::NameSetter(JSThread *thread, const JSHandle<JSObject> &self, const JSHandle<JSTaggedValue> &value,
                            [[maybe_unused]] bool mayThrow)
{
    if (self->IsPropertiesDict()) {
        // replace setter with value
        JSHandle<JSTaggedValue> nameString = thread->GlobalConstants()->GetHandledNameString();
        return self->UpdatePropertyInDictionary(thread, nameString.GetTaggedValue(), value.GetTaggedValue());
    }
    self->SetPropertyInlinedProps(thread, NAME_INLINE_PROPERTY_INDEX, value.GetTaggedValue());
    return true;
}

void JSFunction::SetFunctionExtraInfo(JSThread *thread, void *nativeFunc,
                                      const DeleteEntryPoint &deleter, void *data, size_t nativeBindingsize)
{
    JSTaggedType hashField = Barriers::GetValue<JSTaggedType>(this, HASH_OFFSET);
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(hashField));
    JSHandle<ECMAObject> obj(thread, this);
    JSHandle<JSNativePointer> pointer = vm->GetFactory()->NewJSNativePointer(nativeFunc, deleter, data,
        false, nativeBindingsize);
    if (!obj->HasHash()) {
        Barriers::SetObject<true>(thread, *obj, HASH_OFFSET, pointer.GetTaggedValue().GetRawData());
        return;
    }
    if (value->IsHeapObject()) {
        if (value->IsJSNativePointer()) {
            Barriers::SetObject<true>(thread, *obj, HASH_OFFSET, pointer.GetTaggedValue().GetRawData());
            return;
        }
        JSHandle<TaggedArray> array(value);

        uint32_t nativeFieldCount = array->GetExtraLength();
        if (array->GetLength() >= nativeFieldCount + RESOLVED_MAX_SIZE) {
            array->Set(thread, nativeFieldCount + FUNCTION_EXTRA_INDEX, pointer);
        } else {
            JSHandle<TaggedArray> newArray = vm->GetFactory()->NewTaggedArray(nativeFieldCount + RESOLVED_MAX_SIZE);
            newArray->SetExtraLength(nativeFieldCount);
            for (uint32_t i = 0; i < nativeFieldCount; i++) {
                newArray->Set(thread, i, array->Get(i));
            }
            newArray->Set(thread, nativeFieldCount + HASH_INDEX, array->Get(nativeFieldCount + HASH_INDEX));
            newArray->Set(thread, nativeFieldCount + FUNCTION_EXTRA_INDEX, pointer);
            Barriers::SetObject<true>(thread, *obj, HASH_OFFSET, newArray.GetTaggedValue().GetRawData());
        }
    } else {
        JSHandle<TaggedArray> newArray = vm->GetFactory()->NewTaggedArray(RESOLVED_MAX_SIZE);
        newArray->SetExtraLength(0);
        newArray->Set(thread, HASH_INDEX, value);
        newArray->Set(thread, FUNCTION_EXTRA_INDEX, pointer);
        Barriers::SetObject<true>(thread, *obj, HASH_OFFSET, newArray.GetTaggedValue().GetRawData());
    }
}

JSTaggedValue JSFunction::GetFunctionExtraInfo() const
{
    JSTaggedType hashField = Barriers::GetValue<JSTaggedType>(this, HASH_OFFSET);
    JSTaggedValue value(hashField);
    if (value.IsHeapObject()) {
        if (value.IsTaggedArray()) {
            TaggedArray *array = TaggedArray::Cast(value.GetTaggedObject());
            uint32_t nativeFieldCount = array->GetExtraLength();
            if (array->GetLength() >= nativeFieldCount + RESOLVED_MAX_SIZE) {
                return array->Get(nativeFieldCount + FUNCTION_EXTRA_INDEX);
            }
        }
        if (value.IsJSNativePointer()) {
            return value;
        }
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue JSFunction::GetNativeFunctionExtraInfo() const
{
    JSTaggedType hashField = Barriers::GetValue<JSTaggedType>(this, HASH_OFFSET);
    JSTaggedValue value(hashField);
    if (value.CheckIsJSNativePointer()) {
        return value;
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue JSFunction::GetRecordName() const
{
    JSTaggedValue module = GetModule();
    if (module.IsSourceTextModule()) {
        JSTaggedValue recordName = SourceTextModule::Cast(module.GetTaggedObject())->GetEcmaModuleRecordName();
        if (!recordName.IsString()) {
            LOG_INTERPRETER(DEBUG) << "module record name is undefined";
            return JSTaggedValue::Hole();
        }
        return recordName;
    }
    if (module.IsString()) {
        return module;
    }
    LOG_INTERPRETER(DEBUG) << "record name is undefined";
    return JSTaggedValue::Hole();
}
}  // namespace panda::ecmascript
