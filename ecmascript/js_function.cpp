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

#include "ecmascript/js_function.h"

#include "ecmascript/debugger/js_debugger_manager.h"
#include "ecmascript/ecma_context.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/pgo_profiler/pgo_profiler.h"
#include "ecmascript/require/js_require_manager.h"

namespace panda::ecmascript {
void JSFunction::InitializeJSFunction(JSThread *thread, const JSHandle<JSFunction> &func, FunctionKind kind)
{
    InitializeWithDefaultValue(thread, func);
    auto globalConst = thread->GlobalConstants();
    if (HasPrototype(kind)) {
        JSHandle<JSTaggedValue> accessor = globalConst->GetHandledFunctionPrototypeAccessor();
        if (kind == FunctionKind::BASE_CONSTRUCTOR || kind == FunctionKind::GENERATOR_FUNCTION ||
            kind == FunctionKind::ASYNC_GENERATOR_FUNCTION || kind == FunctionKind::NONE_FUNCTION) {
            func->SetPropertyInlinedProps(thread, PROTOTYPE_INLINE_PROPERTY_INDEX, accessor.GetTaggedValue());
            accessor = globalConst->GetHandledFunctionNameAccessor();
            func->SetPropertyInlinedProps(thread, NAME_INLINE_PROPERTY_INDEX, accessor.GetTaggedValue());
            accessor = globalConst->GetHandledFunctionLengthAccessor();
            func->SetPropertyInlinedProps(thread, LENGTH_INLINE_PROPERTY_INDEX, accessor.GetTaggedValue());
            if (kind == FunctionKind::ASYNC_GENERATOR_FUNCTION) {
                // Not duplicate codes, it will slow the performace if combining and put outside!
                JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
                ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
                JSHandle<JSFunction> objFun(env->GetObjectFunction());
                JSHandle<JSObject> initialGeneratorFuncPrototype = factory->NewJSObjectByConstructor(objFun);
                JSObject::SetPrototype(thread, initialGeneratorFuncPrototype, env->GetAsyncGeneratorPrototype());
                func->SetProtoOrHClass(thread, initialGeneratorFuncPrototype);
            }
            if (kind == FunctionKind::GENERATOR_FUNCTION) {
                // Not duplicate codes, it will slow the performace if combining and put outside!
                JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
                ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
                JSHandle<JSFunction> objFun(env->GetObjectFunction());
                JSHandle<JSObject> initialGeneratorFuncPrototype = factory->NewJSObjectByConstructor(objFun);
                JSObject::SetPrototype(thread, initialGeneratorFuncPrototype, env->GetGeneratorPrototype());
                func->SetProtoOrHClass(thread, initialGeneratorFuncPrototype);
            }
        } else if (!JSFunction::IsClassConstructor(kind)) {  // class ctor do nothing
            PropertyDescriptor desc(thread, accessor, kind != FunctionKind::BUILTIN_CONSTRUCTOR, false, false);
            [[maybe_unused]] bool success = JSObject::DefineOwnProperty(
                thread, JSHandle<JSObject>(func), globalConst->GetHandledPrototypeString(), desc, SCheckMode::SKIP);
            ASSERT(success);
        }
    } else if (HasAccessor(kind)) {
        JSHandle<JSTaggedValue> accessor = globalConst->GetHandledFunctionNameAccessor();
        func->SetPropertyInlinedProps(thread, NAME_INLINE_PROPERTY_INDEX, accessor.GetTaggedValue());
        accessor = globalConst->GetHandledFunctionLengthAccessor();
        func->SetPropertyInlinedProps(thread, LENGTH_INLINE_PROPERTY_INDEX, accessor.GetTaggedValue());
    }
}

void JSFunction::InitializeSFunction(JSThread *thread, const JSHandle<JSFunction> &func, FunctionKind kind)
{
    InitializeWithDefaultValue(thread, func);
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> accessor = globalConst->GetHandledFunctionPrototypeAccessor();
    if (HasAccessor(kind) || IsBaseConstructorKind(kind)) {
        if (IsBaseConstructorKind(kind)) {
            func->SetPropertyInlinedProps(thread, PROTOTYPE_INLINE_PROPERTY_INDEX, accessor.GetTaggedValue());
        }
        accessor = globalConst->GetHandledFunctionNameAccessor();
        func->SetPropertyInlinedProps(thread, NAME_INLINE_PROPERTY_INDEX, accessor.GetTaggedValue());
        accessor = globalConst->GetHandledFunctionLengthAccessor();
        func->SetPropertyInlinedProps(thread, LENGTH_INLINE_PROPERTY_INDEX, accessor.GetTaggedValue());
    }
}

void JSFunction::InitializeWithDefaultValue(JSThread *thread, const JSHandle<JSFunction> &func)
{
    func->SetProtoOrHClass(thread, JSTaggedValue::Hole(), SKIP_BARRIER);
    func->SetHomeObject(thread, JSTaggedValue::Undefined(), SKIP_BARRIER);
    func->SetWorkNodePointer(reinterpret_cast<uintptr_t>(nullptr));
    func->SetLexicalEnv(thread, JSTaggedValue::Undefined(), SKIP_BARRIER);
    func->SetMachineCode(thread, JSTaggedValue::Undefined(), SKIP_BARRIER);
    func->SetBaselineCode(thread, JSTaggedValue::Undefined(), SKIP_BARRIER);
    func->SetRawProfileTypeInfo(thread, thread->GlobalConstants()->GetEmptyProfileTypeInfoCell(), SKIP_BARRIER);
    func->SetMethod(thread, JSTaggedValue::Undefined(), SKIP_BARRIER);
    func->SetModule(thread, JSTaggedValue::Undefined(), SKIP_BARRIER);
    func->SetProtoTransRootHClass(thread, JSTaggedValue::Undefined(), SKIP_BARRIER);
    func->SetCodeEntry(reinterpret_cast<uintptr_t>(nullptr));
    func->ClearCompiledCodeFlags();
    func->SetTaskConcurrentFuncFlag(0); // 0 : default value
}

JSHandle<JSObject> JSFunction::NewJSFunctionPrototype(JSThread *thread, const JSHandle<JSFunction> &func)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSFunction> objFun;
    if (func->IsSharedFunction()) {
        objFun = JSHandle<JSFunction>(env->GetSObjectFunction());
    } else {
        objFun = JSHandle<JSFunction>(env->GetObjectFunction());
    }
    JSHandle<JSObject> funPro = thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(objFun);
    SetFunctionPrototypeOrInstanceHClass(thread, func, funPro.GetTaggedValue());

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
    bool needProfileTransition = false;
    if (!fun->HasFunctionPrototype()) {
        proto = JSHandle<JSTaggedValue>::Cast(NewJSFunctionPrototype(thread, fun));
        if (thread->GetEcmaVM()->IsEnablePGOProfiler()) {
            thread->GetEcmaVM()->GetPGOProfiler()->ProfileClassRootHClass(fun.GetTaggedType(),
                JSTaggedType(proto->GetTaggedObject()->GetClass()), pgo::ProfileType::Kind::PrototypeId);
        }
    } else {
        proto = JSHandle<JSTaggedValue>(thread, fun->GetProtoOrHClass());
        needProfileTransition = proto->IsECMAObject();
    }

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> hclass = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT, proto);
    fun->SetProtoOrHClass(thread, hclass);
    if (thread->GetEcmaVM()->IsEnablePGOProfiler()) {
        if (!needProfileTransition) {
            thread->GetEcmaVM()->GetPGOProfiler()->ProfileClassRootHClass(fun.GetTaggedType(), hclass.GetTaggedType());
        } else {
            thread->GetEcmaVM()->GetPGOProfiler()->ProfileProtoTransitionClass(fun, hclass, proto);
        }
    }
    return *hclass;
}

JSTaggedValue JSFunction::PrototypeGetter(JSThread *thread, const JSHandle<JSObject> &self)
{
    JSHandle<JSFunction> func = JSHandle<JSFunction>::Cast(self);
    if (!func->HasFunctionPrototype()) {
        JSHandle<JSTaggedValue> proto = JSHandle<JSTaggedValue>::Cast(NewJSFunctionPrototype(thread, func));
        if (thread->GetEcmaVM()->IsEnablePGOProfiler()) {
            thread->GetEcmaVM()->GetPGOProfiler()->ProfileClassRootHClass(func.GetTaggedType(),
                JSTaggedType(proto->GetTaggedObject()->GetClass()), pgo::ProfileType::Kind::PrototypeId);
        }
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
        JSHandle<JSHClass> hclass(thread, JSHClass::Cast(protoOrHClass.GetTaggedObject()));
        JSHandle<JSHClass> newClass = JSHClass::SetPrototypeWithNotification(thread, hclass, value);
        func->SetProtoOrHClass(thread, newClass);
        // Forbide to profile for changing the function prototype after an instance of the function has been created
        if (!hclass->IsTS() && thread->GetEcmaVM()->IsEnablePGOProfiler()) {
            EntityId ctorMethodId = Method::Cast(func->GetMethod().GetTaggedObject())->GetMethodId();
            thread->GetEcmaVM()->GetPGOProfiler()->InsertSkipCtorMethodIdSafe(ctorMethodId);
        }
    } else {
        if (!value->IsECMAObject()) {
            func->SetProtoOrHClass(thread, value.GetTaggedValue());
            return true;
        }

        bool enablePgo = thread->GetEcmaVM()->IsEnablePGOProfiler();
        JSMutableHandle<JSTaggedValue> oldPrototype(thread, func->GetProtoOrHClass());
        // For pgo, we need the oldPrototype to record the old ihc and phc.
        if (enablePgo && oldPrototype->IsHole()) {
            oldPrototype.Update(JSHandle<JSTaggedValue>::Cast(NewJSFunctionPrototype(thread, func)));
        }
        JSHandle<JSTaggedValue> baseIhc(thread, value->GetTaggedObject()->GetClass());
        func->SetProtoOrHClass(thread, value.GetTaggedValue());
        JSHClass::OptimizePrototypeForIC(thread, value, true);
        if (thread->GetEcmaVM()->IsEnablePGOProfiler()) {
            thread->GetEcmaVM()->GetPGOProfiler()->ProfileProtoTransitionPrototype(func, value, oldPrototype, baseIhc);
        }
    }
    return true;
}

void JSFunction::SetFunctionPrototypeOrInstanceHClass(const JSThread *thread, const JSHandle<JSFunction> &fun,
                                                      JSTaggedValue protoOrHClass)
{
    JSHandle<JSTaggedValue> protoHandle(thread, protoOrHClass);
    fun->SetProtoOrHClass(thread, protoHandle.GetTaggedValue());
    if (protoHandle->IsJSHClass()) {
        protoHandle = JSHandle<JSTaggedValue>(thread,
                                              JSHClass::Cast(protoHandle->GetTaggedObject())->GetPrototype());
    }
    if (protoHandle->IsECMAObject()) {
        JSHClass::OptimizePrototypeForIC(thread, protoHandle);
    }
}

EcmaString* JSFunction::GetFunctionNameString(JSThread *thread, JSHandle<EcmaString> concatString,
                                              ObjectFactory *factory, JSHandle<JSTaggedValue> target)
{
    JSTaggedValue method = JSHandle<JSFunction>::Cast(target)->GetMethod();
    JSHandle<EcmaString> newString = factory->ConcatFromString(concatString, factory->GetEmptyString());
    if (!method.IsUndefined()) {
        Method *targetObj = Method::Cast(method.GetTaggedObject());
        std::string funcName = targetObj->ParseFunctionName();
        if (!funcName.empty()) {
            JSHandle<JSTaggedValue> methodName(thread,
                                               factory->NewFromStdString(funcName).GetTaggedValue());
            JSHandle<EcmaString> functionName = JSHandle<EcmaString>::Cast(methodName);
            newString = factory->ConcatFromString(concatString, functionName);
        }
    }
    return *newString;
}

JSTaggedValue JSFunction::NameGetter(JSThread *thread, const JSHandle<JSObject> &self)
{
    if (self->IsBoundFunction()) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        const GlobalEnvConstants *globalConst = thread->GlobalConstants();
        JSHandle<JSBoundFunction> boundFunction(self);
        JSHandle<JSTaggedValue> target(thread, boundFunction->GetBoundTarget());

        JSHandle<JSTaggedValue> nameKey = globalConst->GetHandledNameString();
        JSHandle<JSTaggedValue> boundName = thread->GlobalConstants()->GetHandledBoundString();
        JSHandle<JSTaggedValue> targetName = JSObject::GetProperty(thread, target, nameKey).GetValue();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

        JSHandle<EcmaString> handlePrefixString = JSTaggedValue::ToString(thread, boundName);
        JSHandle<EcmaString> spaceString(globalConst->GetHandledSpaceString());
        JSHandle<EcmaString> concatString = factory->ConcatFromString(handlePrefixString, spaceString);

        EcmaString *newString;
        if (!targetName->IsString()) {
            newString = GetFunctionNameString(thread, concatString, factory, target);
        } else {
            JSHandle<EcmaString> functionName = JSHandle<EcmaString>::Cast(targetName);
            newString = *factory->ConcatFromString(concatString, functionName);
        }

        return JSTaggedValue(newString);
    }

    JSTaggedValue method = JSHandle<JSFunction>::Cast(self)->GetMethod();
    if (method.IsUndefined()) {
        return JSTaggedValue::Undefined();
    }
    Method *target = Method::Cast(method.GetTaggedObject());
    if (target->IsNativeWithCallField()) {
        return JSTaggedValue::Undefined();
    }
    auto [funcName, isASCII] = target->ParseFunctionNameView();
    if (funcName.empty()) {
        return thread->GlobalConstants()->GetEmptyString();
    }
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    const JSHandle<EcmaString> nameHdl = factory->NewFromUtf8(funcName, isASCII);
    if (JSHandle<JSFunction>::Cast(self)->GetFunctionKind() == FunctionKind::GETTER_FUNCTION) {
        return factory->ConcatFromString(
            JSHandle<EcmaString>(thread->GlobalConstants()->GetHandledGetWithSpaceString()), nameHdl).GetTaggedValue();
    }
    if (JSHandle<JSFunction>::Cast(self)->GetFunctionKind() == FunctionKind::SETTER_FUNCTION) {
        return factory->ConcatFromString(
            JSHandle<EcmaString>(thread->GlobalConstants()->GetHandledSetWithSpaceString()), nameHdl).GetTaggedValue();
    }
    return nameHdl.GetTaggedValue();
}

JSTaggedValue JSFunction::LengthGetter(JSThread *thread, const JSHandle<JSObject> &self)
{
    // LengthGetter only support BoundFunction
    if (self->IsBoundFunction()) {
        JSMutableHandle<JSBoundFunction> boundFunction(thread, self.GetTaggedValue());
        JSHandle<JSTaggedValue> arguments(thread, boundFunction->GetBoundArguments());
        uint32_t argsLength = TaggedArray::Cast(arguments->GetTaggedObject())->GetLength();
        while (boundFunction->GetBoundTarget().IsBoundFunction()) {
            boundFunction.Update(boundFunction->GetBoundTarget());
            argsLength += TaggedArray::Cast(boundFunction->GetBoundArguments())->GetLength();
        }

        JSHandle<JSTaggedValue> target(thread, boundFunction->GetBoundTarget());
        JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();

        bool targetHasLength =
            JSTaggedValue::HasOwnProperty(thread, target, lengthKey);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        uint32_t lengthValue = 0;
        if (targetHasLength) {
            JSHandle<JSTaggedValue> targetLength = JSTaggedValue::GetProperty(thread, target, lengthKey).GetValue();
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (targetLength->IsNumber()) {
                lengthValue =
                    std::max(0.0, JSTaggedValue::ToNumber(thread, targetLength).GetNumber() -
                             static_cast<double>(argsLength));
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
        }
        return JSTaggedValue(lengthValue);
    }

    JSHandle<JSFunction> func(self);
    return JSTaggedValue(func->GetLength());
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
        STACK_LIMIT_CHECK(thread, false);
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
    JSHandle<JSTaggedValue> prototypeString = globalConst->GetHandledPrototypeString();
    JSMutableHandle<JSTaggedValue> constructorPrototype(thread, JSTaggedValue::Undefined());
    if (constructor->IsJSFunction()) {
        JSHandle<JSFunction> ctor(thread, constructor->GetTaggedObject());
        JSHandle<JSTaggedValue> ctorProtoOrHclass(thread, ctor->GetProtoOrHClass());
        if (!ctorProtoOrHclass->IsHole()) {
            if (!ctorProtoOrHclass->IsJSHClass()) {
                constructorPrototype.Update(ctorProtoOrHclass);
            } else {
                JSTaggedValue ctorProto = JSHClass::Cast(ctorProtoOrHclass->GetTaggedObject())->GetProto();
                constructorPrototype.Update(ctorProto);
            }
        } else {
            constructorPrototype.Update(JSTaggedValue::GetProperty(thread, constructor, prototypeString).GetValue());
        }
    } else {
        constructorPrototype.Update(JSTaggedValue::GetProperty(thread, constructor, prototypeString).GetValue());
    }

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
        object.Update(JSTaggedValue::GetPrototype(thread, object));
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        if (JSTaggedValue::SameValue(object, constructorPrototype)) {
            return true;
        }
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
    SetFunctionPrototypeOrInstanceHClass(thread, func, proto.GetTaggedValue());

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
    JSHandle<JSTaggedValue> &thisArg, CJSInfo* cjsInfo)
{
    ASSERT(thread->IsInManagedState());
    if (mainFunc->IsClassConstructor()) {
        {
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR,
                "class constructor cannot called without 'new'", StackCheck::NO);
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
    if (mainFunc->IsCompiledFastCall()) {
        // entry of aot
        args = JSFunction::GetArgsData(true, thisArg, mainFunc, cjsInfo);
        res = thread->GetEcmaVM()->FastCallAot(actualNumArgs, args.data(), prevFp);
    } else {
        args = JSFunction::GetArgsData(false, thisArg, mainFunc, cjsInfo);
        // entry of aot
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
    ASSERT(thread->IsInManagedState());
    Method *method = func->GetCallTarget();
    JSTaggedValue resultValue;
    size_t numArgs = method->GetNumArgsWithCallField();
    bool needPushArgv = numArgs > info->GetArgsNumber();
    const JSTaggedType *prevFp = thread->GetLastLeaveFrame();
#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    RuntimeStubs::StartCallTimer(thread->GetGlueAddr(), func.GetTaggedType(), true);
#endif
    if (func->IsCompiledFastCall()) {
        if (needPushArgv) {
            info = EcmaInterpreter::ReBuildRuntimeCallInfo(thread, info, numArgs);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        JSTaggedType *stackArgs = info->GetArgs();
        stackArgs[1] = stackArgs[0];
        resultValue = thread->GetEcmaVM()->FastCallAot(info->GetArgsNumber(), stackArgs + 1, prevFp);
    } else {
        resultValue = thread->GetCurrentEcmaContext()->ExecuteAot(info->GetArgsNumber(),
            info->GetArgs(), prevFp, needPushArgv);
    }
#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    RuntimeStubs::EndCallTimer(thread->GetGlueAddr(), func.GetTaggedType());
#endif
    return resultValue;
}

// [[Construct]]
JSTaggedValue JSFunction::ConstructInternal(EcmaRuntimeCallInfo *info)
{
    ASSERT(info != nullptr);
    JSThread *thread = info->GetThread();
    // func need to create a new handle, because optimized EcmaRuntimeCallInfo may overwrite this position.
    JSHandle<JSFunction> func(thread, info->GetFunction().GetTaggedValue());
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
    if (func->IsCompiledCode()) {
        resultValue = InvokeOptimizedEntrypoint(thread, func, info);
        const JSTaggedType *curSp = thread->GetCurrentSPFrame();
        InterpretedEntryFrame *entryState = InterpretedEntryFrame::GetFrameFromSp(curSp);
        JSTaggedType *prevSp = entryState->base.prev;
        thread->SetCurrentSPFrame(prevSp);
    } else {
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
        if (!resultValue.IsECMAObject() && !resultValue.IsUndefined()) {
            THROW_TYPE_ERROR_AND_RETURN(thread,
                "derived class constructor must return an object or undefined", JSTaggedValue::Exception());
        }
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
    // String.prototype.trimLeft.name shoud be trimStart
    if (!nameHandle.IsEmpty()
        && nameHandle.GetTaggedValue() == globalConst->GetHandledTrimLeftString().GetTaggedValue()) {
        nameHandle = globalConst->GetHandledTrimStartString();
    }
    // String.prototype.trimRight.name shoud be trimEnd
    if (!nameHandle.IsEmpty()
        && nameHandle.GetTaggedValue() == globalConst->GetHandledTrimRightString().GetTaggedValue()) {
        nameHandle = globalConst->GetHandledTrimEndString();
    }
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
    if (!target->IsConstructor()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Constructor is false", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> newTarget = info->GetNewTarget();
    JSMutableHandle<JSTaggedValue> newTargetMutable(thread, newTarget.GetTaggedValue());
    if (JSTaggedValue::SameValue(func.GetTaggedValue(), newTarget.GetTaggedValue())) {
        newTargetMutable.Update(target.GetTaggedValue());
    }

    JSHandle<TaggedArray> boundArgs(thread, func->GetBoundArguments());
    const uint32_t boundLength = boundArgs->GetLength();
    const uint32_t argsLength = info->GetArgsNumber() + boundLength;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    uint32_t argc = info->GetArgsNumber();
    std::vector<JSTaggedType> argArray(argc);
    for (uint32_t index = 0; index < argc; ++index) {
        argArray[index] = info->GetCallArgValue(index).GetRawData();
    }
    JSTaggedType *currentSp = reinterpret_cast<JSTaggedType *>(info);
    InterpretedEntryFrame *currentEntryState = InterpretedEntryFrame::GetFrameFromSp(currentSp);
    JSTaggedType *prevSp = currentEntryState->base.prev;
    thread->SetCurrentSPFrame(prevSp);
    EcmaRuntimeCallInfo *runtimeInfo =
        EcmaInterpreter::NewRuntimeCallInfo(thread, target, undefined, newTargetMutable, argsLength);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (boundLength != 0) {
        runtimeInfo->SetCallArg(boundLength, boundArgs);
    }
    for (uint32_t index = 0; index < argc; index++) {
        runtimeInfo->SetCallArg(static_cast<uint32_t>(index + boundLength), JSTaggedValue(argArray[index]));
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
                RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSHClass, thread);
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
    newJSHClass->SetElementsKind(ElementsKind::GENERIC);
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
    newJSHClass->SetElementsKind(ElementsKind::GENERIC);
    // guarante derived has function prototype
    JSHandle<JSTaggedValue> prototype(thread, derived->GetProtoOrHClass());
    ASSERT(!prototype->IsHole());
    newJSHClass->SetPrototype(thread, prototype);
    derived->SetProtoOrHClass(thread, newJSHClass);

    if (thread->GetEcmaVM()->IsEnablePGOProfiler()) {
        thread->GetEcmaVM()->GetPGOProfiler()->ProfileClassRootHClass(derived.GetTaggedType(),
            newJSHClass.GetTaggedType());
    }

    return newJSHClass;
}

CString JSFunction::GetRecordName() const
{
    JSTaggedValue module = GetModule();
    if (module.IsSourceTextModule()) {
        return SourceTextModule::GetModuleName(module);
    }
    if (module.IsString()) {
        return ConvertToString(module);
    }
    LOG_INTERPRETER(DEBUG) << "record name is undefined";
    return "";
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

// NOTE: move to INL file?
void JSFunction::SetFunctionLength(const JSThread *thread, JSTaggedValue length)
{
    ASSERT(!IsPropertiesDict());
    SetPropertyInlinedProps(thread, LENGTH_INLINE_PROPERTY_INDEX, length);
}

void JSFunction::SetFunctionExtraInfo(JSThread *thread, void *nativeFunc, const NativePointerCallback &deleter,
                                      void *data, size_t nativeBindingsize, Concurrent isConcurrent)
{
    JSTaggedType hashField = Barriers::GetValue<JSTaggedType>(this, HASH_OFFSET);
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(hashField));
    JSHandle<ECMAObject> obj(thread, this);
    JSHandle<JSNativePointer> pointer = vm->GetFactory()->NewJSNativePointer(nativeFunc, deleter, data,
        false, nativeBindingsize, isConcurrent);
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

void JSFunction::SetSFunctionExtraInfo(
    JSThread *thread, void *nativeFunc, const NativePointerCallback &deleter, void *data, size_t nativeBindingsize)
{
    JSTaggedType hashField = Barriers::GetValue<JSTaggedType>(this, HASH_OFFSET);
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(hashField));
    JSHandle<ECMAObject> obj(thread, this);
    JSHandle<JSNativePointer> pointer =
        vm->GetFactory()->NewSJSNativePointer(nativeFunc, deleter, data, false, nativeBindingsize);
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
            JSHandle<TaggedArray> newArray =
                vm->GetFactory()->NewSTaggedArrayWithoutInit(nativeFieldCount + RESOLVED_MAX_SIZE);
            newArray->SetExtraLength(nativeFieldCount);
            for (uint32_t i = 0; i < nativeFieldCount; i++) {
                newArray->Set(thread, i, array->Get(i));
            }
            newArray->Set(thread, nativeFieldCount + HASH_INDEX, array->Get(nativeFieldCount + HASH_INDEX));
            newArray->Set(thread, nativeFieldCount + FUNCTION_EXTRA_INDEX, pointer);
            Barriers::SetObject<true>(thread, *obj, HASH_OFFSET, newArray.GetTaggedValue().GetRawData());
        }
    } else {
        JSHandle<TaggedArray> newArray = vm->GetFactory()->NewSTaggedArrayWithoutInit(RESOLVED_MAX_SIZE);
        newArray->SetExtraLength(0);
        newArray->Set(thread, HASH_INDEX, value);
        newArray->Set(thread, FUNCTION_EXTRA_INDEX, pointer);
        Barriers::SetObject<true>(thread, *obj, HASH_OFFSET, newArray.GetTaggedValue().GetRawData());
    }
}

void JSFunction::SetProfileTypeInfo(const JSThread *thread, const JSHandle<JSFunction> &func,
                                    const JSHandle<JSTaggedValue> &value, BarrierMode mode)
{
    JSHandle<ProfileTypeInfoCell> handleRaw(thread, func->GetRawProfileTypeInfo());
    if (handleRaw->IsEmptyProfileTypeInfoCell(thread)) {
        JSHandle<ProfileTypeInfoCell> handleProfileTypeInfoCell =
            thread->GetEcmaVM()->GetFactory()->NewProfileTypeInfoCell(value);
        func->SetRawProfileTypeInfo(thread, handleProfileTypeInfoCell, WRITE_BARRIER);
        return;
    }
    handleRaw->SetValue(thread, value, mode);
}

void JSFunction::UpdateProfileTypeInfoCell(JSThread *thread, JSHandle<FunctionTemplate> literalFunc,
                                           JSHandle<JSFunction> targetFunc)
{
    auto profileTypeInfoCellVal = literalFunc->GetRawProfileTypeInfo();
    ASSERT(profileTypeInfoCellVal.IsProfileTypeInfoCell());
    auto profileTypeInfoCell = ProfileTypeInfoCell::Cast(profileTypeInfoCellVal);
    if (profileTypeInfoCell->IsEmptyProfileTypeInfoCell(thread)) {
        JSHandle<JSTaggedValue> handleUndefined(thread, JSTaggedValue::Undefined());
        JSHandle<ProfileTypeInfoCell> newProfileTypeInfoCell =
            thread->GetEcmaVM()->GetFactory()->NewProfileTypeInfoCell(handleUndefined);
        literalFunc->SetRawProfileTypeInfo(thread, newProfileTypeInfoCell);
        targetFunc->SetRawProfileTypeInfo(thread, newProfileTypeInfoCell);
    } else {
        ProfileTypeInfoCell::Cast(profileTypeInfoCell)->UpdateProfileTypeInfoCellType(thread);
        targetFunc->SetRawProfileTypeInfo(thread, profileTypeInfoCellVal);
    }
}

void JSFunction::SetJitMachineCodeCache(const JSThread *thread, const JSHandle<MachineCode> &machineCode)
{
    JSHandle<ProfileTypeInfoCell> handleRaw(thread, GetRawProfileTypeInfo());
    ASSERT(!handleRaw->IsEmptyProfileTypeInfoCell(thread));
    handleRaw->SetMachineCode(thread, machineCode.GetTaggedValue().CreateAndGetWeakRef(), WRITE_BARRIER);
}

JSTaggedValue JSFunctionBase::GetFunctionExtraInfo() const
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
        JSType valueType = value.GetTaggedObject()->GetClass()->GetObjectType();
        JSType thisType = this->GetClass()->GetObjectType();
        LOG_ECMA(FATAL) << "this branch is unreachable, thisType is " << static_cast<uint8_t>(thisType)
                        << ", value type is " << static_cast<uint8_t>(valueType);
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

void JSFunction::InitializeForConcurrentFunction(JSThread *thread, JSHandle<JSFunction> &func)
{
    JSHandle<Method> method(thread, func->GetMethod());
    JSMutableHandle<JSTaggedValue> sendableEnv(thread, JSTaggedValue::Undefined());
    if (func->IsSharedFunction() && !func->GetModule().IsUndefined()) {
        sendableEnv.Update(SourceTextModule::Cast(func->GetModule())->GetSendableEnv());
    }
    const JSPandaFile *jsPandaFile = method->GetJSPandaFile();
    if (jsPandaFile == nullptr) {
        LOG_ECMA(ERROR) << "JSPandaFile is nullptr";
        return;
    }
    ecmascript::CString moduleName = jsPandaFile->GetJSPandaFileDesc();
    ecmascript::CString recordName = method->GetRecordNameStr();

    // check ESM or CJS
    ecmascript::JSRecordInfo *recordInfo = nullptr;
    bool hasRecord = jsPandaFile->CheckAndGetRecordInfo(recordName, &recordInfo);
    if (!hasRecord) {
        CString msg = "Cannot find module '" + recordName + "' , which is application Entry Point";
        LOG_ECMA(ERROR) << msg;
        return;
    }
    if (!jsPandaFile->IsModule(recordInfo)) {
        LOG_ECMA(DEBUG) << "Current function is not from ES Module's file.";
        return;
    }
    ecmascript::ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<ecmascript::JSTaggedValue> moduleRecord;
    // check compileMode
    if (jsPandaFile->IsBundlePack()) {
        LOG_ECMA(DEBUG) << "CompileMode is jsbundle";
        moduleRecord = moduleManager->HostResolveImportedModule(moduleName);
    } else {
        LOG_ECMA(DEBUG) << "CompileMode is esmodule";
        moduleRecord = moduleManager->HostResolveImportedModuleWithMerge(moduleName, recordName);
    }
    RETURN_IF_ABRUPT_COMPLETION(thread);
    ecmascript::SourceTextModule::Instantiate(thread, moduleRecord);
    JSHandle<ecmascript::SourceTextModule> module = JSHandle<ecmascript::SourceTextModule>::Cast(moduleRecord);
    module->SetStatus(ecmascript::ModuleStatus::INSTANTIATED);
    ecmascript::SourceTextModule::EvaluateForConcurrent(thread, module, method);
    if (!jsPandaFile->IsBundlePack() && func->IsSharedFunction()) {
        JSHandle<JSTaggedValue> sendableClassRecord = moduleManager->GenerateSendableFuncModule(moduleRecord);
        SourceTextModule::Cast(sendableClassRecord.GetTaggedValue())->SetSendableEnv(thread, sendableEnv);
        func->SetModule(thread, sendableClassRecord);
    } else {
        func->SetModule(thread, moduleRecord);
    }

    // for debugger, to notify the script loaded and parsed which the concurrent function is in
    auto *notificationMgr = thread->GetEcmaVM()->GetJsDebuggerManager()->GetNotificationManager();
    notificationMgr->LoadModuleEvent(moduleName, recordName);
}

bool JSFunction::IsSendableOrConcurrentFunction() const
{
    if (this->GetClass()->IsJSSharedFunction() || this->GetClass()->IsJSSharedAsyncFunction() ||
        this->GetFunctionKind() == ecmascript::FunctionKind::CONCURRENT_FUNCTION) {
        return true;
    }
    return false;
}

bool JSFunction::IsSharedFunction() const
{
    if (this->GetClass()->IsJSSharedFunction() || this->GetClass()->IsJSSharedAsyncFunction()) {
        return true;
    }
    return false;
}

void JSFunctionBase::SetCompiledFuncEntry(uintptr_t codeEntry, bool isFastCall)
{
    ASSERT(codeEntry != 0);
    SetCodeEntry(codeEntry);
    SetIsCompiledFastCall(isFastCall);
    SetCompiledCodeBit(true);
}

void JSFunction::SetJitCompiledFuncEntry(JSThread *thread, JSHandle<MachineCode> &machineCode, bool isFastCall)
{
    uintptr_t codeEntry = machineCode->GetFuncAddr();
    ASSERT(codeEntry != 0);

    SetMachineCode(thread, machineCode);
    SetCompiledFuncEntry(codeEntry, isFastCall);
}

void JSFunctionBase::ClearCompiledCodeFlags()
{
    SetCompiledCodeBit(false);
    SetIsCompiledFastCall(false);
}

void JSFunction::ClearMachineCode(const JSThread *thread)
{
    SetMachineCode(thread, JSTaggedValue::Undefined());
}
}  // namespace panda::ecmascript
