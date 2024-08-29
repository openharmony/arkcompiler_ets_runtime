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

#include "ecmascript/interpreter/slow_runtime_stub.h"

#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/stubs/runtime_stubs-inl.h"

namespace panda::ecmascript {
JSTaggedValue SlowRuntimeStub::CallSpread(JSThread *thread, JSTaggedValue func, JSTaggedValue obj,
                                          JSTaggedValue array)
{
    INTERPRETER_TRACE(thread, CallSpread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> jsFunc(thread, func);
    JSHandle<JSTaggedValue> jsArray(thread, array);
    JSHandle<JSTaggedValue> taggedObj(thread, obj);
    return RuntimeStubs::RuntimeCallSpread(thread, jsFunc, taggedObj, jsArray);
}

JSTaggedValue SlowRuntimeStub::Neg(JSThread *thread, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, Neg);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> inputTag(thread, value);
    return RuntimeStubs::RuntimeNeg(thread, inputTag);
}

JSTaggedValue SlowRuntimeStub::AsyncFunctionEnter(JSThread *thread)
{
    INTERPRETER_TRACE(thread, AsyncFunctionEnter);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    return RuntimeStubs::RuntimeAsyncFunctionEnter(thread);
}

JSTaggedValue SlowRuntimeStub::ToNumber(JSThread *thread, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, Tonumber);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> number(thread, value);
    // may return exception
    return RuntimeStubs::RuntimeToNumber(thread, number);
}

JSTaggedValue SlowRuntimeStub::ToNumeric(JSThread *thread, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, Tonumeric);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> numeric(thread, value);
    // may return exception
    return RuntimeStubs::RuntimeToNumeric(thread, numeric);
}

JSTaggedValue SlowRuntimeStub::Not(JSThread *thread, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, Not);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> inputTag(thread, value);
    return RuntimeStubs::RuntimeNot(thread, inputTag);
}

JSTaggedValue SlowRuntimeStub::Inc(JSThread *thread, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, Inc);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> inputTag(thread, value);
    return RuntimeStubs::RuntimeInc(thread, inputTag);
}

JSTaggedValue SlowRuntimeStub::Dec(JSThread *thread, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, Dec);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> inputTag(thread, value);
    return RuntimeStubs::RuntimeDec(thread, inputTag);
}

void SlowRuntimeStub::Throw(JSThread *thread, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, Throw);
    RuntimeStubs::RuntimeThrow(thread, value);
}

JSTaggedValue SlowRuntimeStub::GetPropIterator(JSThread *thread, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, GetPropIterator);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, value);
    return RuntimeStubs::RuntimeGetPropIterator(thread, objHandle);
}

void SlowRuntimeStub::ThrowConstAssignment(JSThread *thread, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, ThrowConstAssignment);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<EcmaString> name(thread, value.GetTaggedObject());
    return RuntimeStubs::RuntimeThrowConstAssignment(thread, name);
}

JSTaggedValue SlowRuntimeStub::Add2(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Add2);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftValue(thread, left);
    JSHandle<JSTaggedValue> rightValue(thread, right);
    return RuntimeStubs::RuntimeAdd2(thread, leftValue, rightValue);
}

JSTaggedValue SlowRuntimeStub::Sub2(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Sub2);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftTag(thread, left);
    JSHandle<JSTaggedValue> rightTag(thread, right);
    return RuntimeStubs::RuntimeSub2(thread, leftTag, rightTag);
}

JSTaggedValue SlowRuntimeStub::Mul2(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Mul2);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftTag(thread, left);
    JSHandle<JSTaggedValue> rightTag(thread, right);
    return RuntimeStubs::RuntimeMul2(thread, leftTag, rightTag);
}

JSTaggedValue SlowRuntimeStub::Div2(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Div2);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftTag(thread, left);
    JSHandle<JSTaggedValue> rightTag(thread, right);
    return RuntimeStubs::RuntimeDiv2(thread, leftTag, rightTag);
}

JSTaggedValue SlowRuntimeStub::Mod2(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Mod2);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftTag(thread, left);
    JSHandle<JSTaggedValue> rightTag(thread, right);
    return RuntimeStubs::RuntimeMod2(thread, leftTag, rightTag);
}

JSTaggedValue SlowRuntimeStub::Eq(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Eq);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftValue(thread, left);
    JSHandle<JSTaggedValue> rightValue(thread, right);
    return RuntimeStubs::RuntimeEq(thread, leftValue, rightValue);
}

JSTaggedValue SlowRuntimeStub::NotEq(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, NotEq);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftValue(thread, left);
    JSHandle<JSTaggedValue> rightValue(thread, right);
    return RuntimeStubs::RuntimeNotEq(thread, leftValue, rightValue);
}

JSTaggedValue SlowRuntimeStub::Less(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Less);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftValue(thread, left);
    JSHandle<JSTaggedValue> rightValue(thread, right);
    return RuntimeStubs::RuntimeLess(thread, leftValue, rightValue);
}

JSTaggedValue SlowRuntimeStub::LessEq(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, LessEq);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftValue(thread, left);
    JSHandle<JSTaggedValue> rightValue(thread, right);
    return RuntimeStubs::RuntimeLessEq(thread, leftValue, rightValue);
}

JSTaggedValue SlowRuntimeStub::Greater(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Greater);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftValue(thread, left);
    JSHandle<JSTaggedValue> rightValue(thread, right);
    return RuntimeStubs::RuntimeGreater(thread, leftValue, rightValue);
}

JSTaggedValue SlowRuntimeStub::GreaterEq(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, GreaterEq);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftValue(thread, left);
    JSHandle<JSTaggedValue> rightValue(thread, right);
    return RuntimeStubs::RuntimeGreaterEq(thread, leftValue, rightValue);
}

JSTaggedValue SlowRuntimeStub::Shl2(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Shl2);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftTag(thread, left);
    JSHandle<JSTaggedValue> rightTag(thread, right);
    return RuntimeStubs::RuntimeShl2(thread, leftTag, rightTag);
}

JSTaggedValue SlowRuntimeStub::Shr2(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Shr2);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftTag(thread, left);
    JSHandle<JSTaggedValue> rightTag(thread, right);
    return RuntimeStubs::RuntimeShr2(thread, leftTag, rightTag);
}

JSTaggedValue SlowRuntimeStub::Ashr2(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Ashr2);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftTag(thread, left);
    JSHandle<JSTaggedValue> rightTag(thread, right);
    return RuntimeStubs::RuntimeAshr2(thread, leftTag, rightTag);
}

JSTaggedValue SlowRuntimeStub::And2(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, And2);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> leftTag(thread, left);
    JSHandle<JSTaggedValue> rightTag(thread, right);
    return RuntimeStubs::RuntimeAnd2(thread, leftTag, rightTag);
}

JSTaggedValue SlowRuntimeStub::Or2(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Or2);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftTag(thread, left);
    JSHandle<JSTaggedValue> rightTag(thread, right);
    return RuntimeStubs::RuntimeOr2(thread, leftTag, rightTag);
}

JSTaggedValue SlowRuntimeStub::Xor2(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Xor2);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftTag(thread, left);
    JSHandle<JSTaggedValue> rightTag(thread, right);
    return RuntimeStubs::RuntimeXor2(thread, leftTag, rightTag);
}

JSTaggedValue SlowRuntimeStub::ToJSTaggedValueWithInt32(JSThread *thread, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, ToJSTaggedValueWithInt32);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    return RuntimeStubs::RuntimeToJSTaggedValueWithInt32(thread, valueHandle);
}

JSTaggedValue SlowRuntimeStub::ToJSTaggedValueWithUint32(JSThread *thread, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, ToJSTaggedValueWithUint32);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    return RuntimeStubs::RuntimeToJSTaggedValueWithUint32(thread, valueHandle);
}

JSTaggedValue SlowRuntimeStub::DelObjProp(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop)
{
    INTERPRETER_TRACE(thread, Delobjprop);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> propHandle(thread, prop);
    return RuntimeStubs::RuntimeDelObjProp(thread, objHandle, propHandle);
}

JSTaggedValue SlowRuntimeStub::NewObjRange(JSThread *thread, JSTaggedValue func, JSTaggedValue newTarget,
                                           uint16_t firstArgIdx, uint16_t length)
{
    INTERPRETER_TRACE(thread, NewobjRange);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> funcHandle(thread, func);
    JSHandle<JSTaggedValue> newTargetHandle(thread, newTarget);
    return RuntimeStubs::RuntimeNewObjRange(thread, funcHandle, newTargetHandle, firstArgIdx, length);
}

JSTaggedValue SlowRuntimeStub::CreateObjectWithExcludedKeys(JSThread *thread, uint16_t numKeys, JSTaggedValue objVal,
                                                            uint16_t firstArgRegIdx)
{
    INTERPRETER_TRACE(thread, CreateObjectWithExcludedKeys);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> obj(thread, objVal);
    return RuntimeStubs::RuntimeCreateObjectWithExcludedKeys(thread, numKeys, obj, firstArgRegIdx);
}

JSTaggedValue SlowRuntimeStub::Exp(JSThread *thread, JSTaggedValue base, JSTaggedValue exponent)
{
    INTERPRETER_TRACE(thread, Exp);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    return RuntimeStubs::RuntimeExp(thread, base, exponent);
}

JSTaggedValue SlowRuntimeStub::IsIn(JSThread *thread, JSTaggedValue prop, JSTaggedValue obj)
{
    INTERPRETER_TRACE(thread, IsIn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> propHandle(thread, prop);
    JSHandle<JSTaggedValue> objHandle(thread, obj);
    return RuntimeStubs::RuntimeIsIn(thread, propHandle, objHandle);
}

JSTaggedValue SlowRuntimeStub::Instanceof(JSThread *thread, JSTaggedValue obj, JSTaggedValue target)
{
    INTERPRETER_TRACE(thread, Instanceof);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> targetHandle(thread, target);
    return RuntimeStubs::RuntimeInstanceof(thread, objHandle, targetHandle);
}

JSTaggedValue SlowRuntimeStub::InstanceofByHandler(JSThread *thread, JSTaggedValue target, JSTaggedValue object,
                                                   JSTaggedValue instOfHandler)
{
    INTERPRETER_TRACE(thread, InstanceofByHandler);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objectHandle(thread, object);
    JSHandle<JSTaggedValue> targetHandle(thread, target);
    JSHandle<JSTaggedValue> instOfHandle(thread, instOfHandler);

    return RuntimeStubs::RuntimeInstanceofByHandler(thread, targetHandle, objectHandle, instOfHandle);
}

JSTaggedValue SlowRuntimeStub::NewLexicalEnv(JSThread *thread, uint16_t numVars)
{
    INTERPRETER_TRACE(thread, Newlexenv);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    return RuntimeStubs::RuntimeNewLexicalEnv(thread, numVars);
}

JSTaggedValue SlowRuntimeStub::NewSendableEnv(JSThread *thread, uint16_t numVars)
{
    INTERPRETER_TRACE(thread, NewSendableEnv);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    return RuntimeStubs::RuntimeNewSendableEnv(thread, numVars);
}

JSTaggedValue SlowRuntimeStub::NewLexicalEnvWithName(JSThread *thread, uint16_t numVars, uint16_t scopeId)
{
    INTERPRETER_TRACE(thread, NewlexenvwithName);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    return RuntimeStubs::RuntimeNewLexicalEnvWithName(thread, numVars, scopeId);
}

JSTaggedValue SlowRuntimeStub::CreateIterResultObj(JSThread *thread, JSTaggedValue value, JSTaggedValue flag)
{
    INTERPRETER_TRACE(thread, CreateIterResultObj);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> valueHandle(thread, value);
    return RuntimeStubs::RuntimeCreateIterResultObj(thread, valueHandle, flag);
}

JSTaggedValue SlowRuntimeStub::CreateGeneratorObj(JSThread *thread, JSTaggedValue genFunc)
{
    INTERPRETER_TRACE(thread, CreateGeneratorObj);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> generatorFunction(thread, genFunc);
    return RuntimeStubs::RuntimeCreateGeneratorObj(thread, generatorFunction);
}

JSTaggedValue SlowRuntimeStub::CreateAsyncGeneratorObj(JSThread *thread, JSTaggedValue genFunc)
{
    INTERPRETER_TRACE(thread, CreateAsyncGeneratorObj);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> asyncGeneratorFunction(thread, genFunc);
    return RuntimeStubs::RuntimeCreateAsyncGeneratorObj(thread, asyncGeneratorFunction);
}

JSTaggedValue SlowRuntimeStub::SuspendGenerator(JSThread *thread, JSTaggedValue genObj, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, SuspendGenerator);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> genObjHandle(thread, genObj);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    return RuntimeStubs::RuntimeSuspendGenerator(thread, genObjHandle, valueHandle);
}

void SlowRuntimeStub::SetGeneratorState(JSThread *thread, JSTaggedValue genObj, int32_t index)
{
    INTERPRETER_TRACE(thread, SetGeneratorState);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> genObjHandle(thread, genObj);
    return RuntimeStubs::RuntimeSetGeneratorState(thread, genObjHandle, index);
}

JSTaggedValue SlowRuntimeStub::AsyncFunctionAwaitUncaught(JSThread *thread, JSTaggedValue asyncFuncObj,
                                                          JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, AsyncFunctionAwaitUncaught);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> asyncFuncObjHandle(thread, asyncFuncObj);
    JSHandle<JSTaggedValue> valueHandle(thread, value);

    return RuntimeStubs::RuntimeAsyncFunctionAwaitUncaught(thread, asyncFuncObjHandle, valueHandle);
}

JSTaggedValue SlowRuntimeStub::AsyncFunctionResolveOrReject(JSThread *thread, JSTaggedValue asyncFuncObj,
                                                            JSTaggedValue value, bool is_resolve)
{
    INTERPRETER_TRACE(thread, AsyncFunctionResolveOrReject);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> asyncFuncObjHandle(thread, asyncFuncObj);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    return RuntimeStubs::RuntimeAsyncFunctionResolveOrReject(thread, asyncFuncObjHandle, valueHandle, is_resolve);
}

JSTaggedValue SlowRuntimeStub::NewObjApply(JSThread *thread, JSTaggedValue func, JSTaggedValue array)
{
    INTERPRETER_TRACE(thread, NewObjApply);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> funcHandle(thread, func);
    JSHandle<JSTaggedValue> jsArray(thread, array);
    return RuntimeStubs::RuntimeNewObjApply(thread, funcHandle, jsArray);
}

void SlowRuntimeStub::ThrowUndefinedIfHole(JSThread *thread, JSTaggedValue obj)
{
    INTERPRETER_TRACE(thread, ThrowUndefinedIfHole);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<EcmaString> name(thread, obj);
    return RuntimeStubs::RuntimeThrowUndefinedIfHole(thread, name);
}

JSTaggedValue SlowRuntimeStub::ThrowIfSuperNotCorrectCall(JSThread *thread, uint16_t index, JSTaggedValue thisValue)
{
    INTERPRETER_TRACE(thread, ThrowIfSuperNotCorrectCall);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    return RuntimeStubs::RuntimeThrowIfSuperNotCorrectCall(thread, index, thisValue);
}

void SlowRuntimeStub::ThrowIfNotObject(JSThread *thread)
{
    INTERPRETER_TRACE(thread, ThrowIfNotObject);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    THROW_TYPE_ERROR(thread, "Inner return result is not object");
}

void SlowRuntimeStub::ThrowThrowNotExists(JSThread *thread)
{
    INTERPRETER_TRACE(thread, ThrowThrowNotExists);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    return RuntimeStubs::RuntimeThrowIfNotObject(thread);
}

void SlowRuntimeStub::ThrowPatternNonCoercible(JSThread *thread)
{
    INTERPRETER_TRACE(thread, ThrowPatternNonCoercible);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    return RuntimeStubs::RuntimeThrowPatternNonCoercible(thread);
}

JSTaggedValue SlowRuntimeStub::StOwnByName(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, StOwnByName);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> propHandle(thread, prop);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    return RuntimeStubs::RuntimeStOwnByName(thread, objHandle, propHandle, valueHandle);
}

JSTaggedValue SlowRuntimeStub::StOwnByNameWithNameSet(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop,
                                                      JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, StOwnByName);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> propHandle(thread, prop);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    return RuntimeStubs::RuntimeStOwnByNameWithNameSet(thread, objHandle, propHandle, valueHandle);
}

JSTaggedValue SlowRuntimeStub::StOwnByIndex(JSThread *thread, JSTaggedValue obj, uint32_t idx, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, StOwnById);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> idxHandle(thread, JSTaggedValue(idx));
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    return RuntimeStubs::RuntimeStOwnByIndex(thread, objHandle, idxHandle, valueHandle);
}

JSTaggedValue SlowRuntimeStub::StOwnByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue key, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, StOwnByValue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> keyHandle(thread, key);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    return RuntimeStubs::RuntimeStOwnByValue(thread, objHandle, keyHandle, valueHandle);
}

JSTaggedValue SlowRuntimeStub::StOwnByValueWithNameSet(JSThread *thread, JSTaggedValue obj, JSTaggedValue key,
                                                       JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, StOwnByValue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> keyHandle(thread, key);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    return RuntimeStubs::RuntimeStOwnByValueWithNameSet(thread, objHandle, keyHandle, valueHandle);
}

JSTaggedValue SlowRuntimeStub::CreateEmptyArray(JSThread *thread, ObjectFactory *factory, JSHandle<GlobalEnv> globalEnv)
{
    INTERPRETER_TRACE(thread, CreateEmptyArray);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    return RuntimeStubs::RuntimeCreateEmptyArray(thread, factory, globalEnv);
}

JSTaggedValue SlowRuntimeStub::CreateEmptyObject(JSThread *thread, ObjectFactory *factory,
                                                 JSHandle<GlobalEnv> globalEnv)
{
    INTERPRETER_TRACE(thread, CreateEmptyObject);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    return RuntimeStubs::RuntimeCreateEmptyObject(thread, factory, globalEnv);
}

JSTaggedValue SlowRuntimeStub::CreateObjectWithBuffer(JSThread *thread, ObjectFactory *factory, JSObject *literal)
{
    INTERPRETER_TRACE(thread, CreateObjectWithBuffer);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSObject> obj(thread, literal);
    return RuntimeStubs::RuntimeCreateObjectWithBuffer(thread, factory, obj);
}

JSTaggedValue SlowRuntimeStub::CreateObjectHavingMethod(JSThread *thread, ObjectFactory *factory, JSObject *literal,
                                                        JSTaggedValue env)
{
    INTERPRETER_TRACE(thread, CreateObjectHavingMethod);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSObject> obj(thread, literal);
    JSHandle<JSTaggedValue> envi(thread, env);
    return RuntimeStubs::RuntimeCreateObjectHavingMethod(thread, factory, obj, envi);
}

JSTaggedValue SlowRuntimeStub::SetObjectWithProto(JSThread *thread, JSTaggedValue proto, JSTaggedValue obj)
{
    INTERPRETER_TRACE(thread, SetObjectWithProto);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> protoHandle(thread, proto);
    JSHandle<JSObject> objHandle(thread, obj);
    return RuntimeStubs::RuntimeSetObjectWithProto(thread, protoHandle, objHandle);
}

JSTaggedValue SlowRuntimeStub::IterNext(JSThread *thread, JSTaggedValue iter)
{
    INTERPRETER_TRACE(thread, IterNext);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> iterHandle(thread, iter);
    return RuntimeStubs::RuntimeIterNext(thread, iterHandle);
}

JSTaggedValue SlowRuntimeStub::CloseIterator(JSThread *thread, JSTaggedValue iter)
{
    INTERPRETER_TRACE(thread, CloseIterator);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> iterHandle(thread, iter);
    return RuntimeStubs::RuntimeCloseIterator(thread, iterHandle);
}

void SlowRuntimeStub::StModuleVar(JSThread *thread, int32_t index, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, StModuleVar);
    [[maybe_unused]] EcmaHandleScope scope(thread);

    return RuntimeStubs::RuntimeStModuleVar(thread, index, value);
}

void SlowRuntimeStub::StModuleVar(JSThread *thread, JSTaggedValue key, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, StModuleVar);
    [[maybe_unused]] EcmaHandleScope scope(thread);

    return RuntimeStubs::RuntimeStModuleVar(thread, key, value);
}

JSTaggedValue SlowRuntimeStub::LdLocalModuleVar(JSThread *thread, int32_t index)
{
    RUNTIME_TRACE(thread, LdLocalModuleVarByIndex);
    [[maybe_unused]] EcmaHandleScope scope(thread);

    return RuntimeStubs::RuntimeLdLocalModuleVar(thread, index);
}

JSTaggedValue SlowRuntimeStub::LdExternalModuleVar(JSThread *thread, int32_t index)
{
    RUNTIME_TRACE(thread, LdExternalModuleVarByIndex);
    [[maybe_unused]] EcmaHandleScope scope(thread);

    return RuntimeStubs::RuntimeLdExternalModuleVar(thread, index);
}

JSTaggedValue SlowRuntimeStub::LdModuleVar(JSThread *thread, JSTaggedValue key, bool inner)
{
    INTERPRETER_TRACE(thread, LdModuleVar);
    [[maybe_unused]] EcmaHandleScope scope(thread);

    return RuntimeStubs::RuntimeLdModuleVar(thread, key, inner);
}

JSTaggedValue SlowRuntimeStub::CreateRegExpWithLiteral(JSThread *thread, JSTaggedValue pattern, uint8_t flags)
{
    INTERPRETER_TRACE(thread, CreateRegExpWithLiteral);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> patternHandle(thread, pattern);
    return RuntimeStubs::RuntimeCreateRegExpWithLiteral(thread, patternHandle, flags);
}

JSTaggedValue SlowRuntimeStub::CreateArrayWithBuffer(JSThread *thread, ObjectFactory *factory, JSArray *literal)
{
    INTERPRETER_TRACE(thread, CreateArrayWithBuffer);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> array(thread, literal);
    return RuntimeStubs::RuntimeCreateArrayWithBuffer(thread, factory, array);
}

JSTaggedValue SlowRuntimeStub::GetTemplateObject(JSThread *thread, JSTaggedValue literal)
{
    INTERPRETER_TRACE(thread, GetTemplateObject);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> templateLiteral(thread, literal);
    return RuntimeStubs::RuntimeGetTemplateObject(thread, templateLiteral);
}

JSTaggedValue SlowRuntimeStub::GetNextPropName(JSThread *thread, JSTaggedValue iter)
{
    INTERPRETER_TRACE(thread, GetNextPropName);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> iterator(thread, iter);
    return RuntimeStubs::RuntimeGetNextPropName(thread, iterator);
}

JSTaggedValue SlowRuntimeStub::CopyDataProperties(JSThread *thread, JSTaggedValue dst, JSTaggedValue src)
{
    INTERPRETER_TRACE(thread, CopyDataProperties);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> dstHandle(thread, dst);
    JSHandle<JSTaggedValue> srcHandle(thread, src);
    return RuntimeStubs::RuntimeCopyDataProperties(thread, dstHandle, srcHandle);
}

JSTaggedValue SlowRuntimeStub::GetIteratorNext(JSThread *thread, JSTaggedValue obj, JSTaggedValue method)
{
    RUNTIME_TRACE(thread, GetIteratorNext);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> iter(thread, obj);
    JSHandle<JSTaggedValue> next(thread, method);
    return RuntimeStubs::RuntimeGetIteratorNext(thread, iter, next);
}

JSTaggedValue SlowRuntimeStub::GetUnmapedArgs(JSThread *thread, JSTaggedType *sp, uint32_t actualNumArgs,
                                              uint32_t startIdx)
{
    INTERPRETER_TRACE(thread, GetUnmapedArgs);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    return RuntimeStubs::RuntimeGetUnmapedArgs(thread, sp, actualNumArgs, startIdx);
}

JSTaggedValue SlowRuntimeStub::CopyRestArgs(JSThread *thread, JSTaggedType *sp, uint32_t restNumArgs, uint32_t startIdx)
{
    INTERPRETER_TRACE(thread, Copyrestargs);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    return RuntimeStubs::RuntimeCopyRestArgs(thread, sp, restNumArgs, startIdx);
}

JSTaggedValue SlowRuntimeStub::GetIterator(JSThread *thread, JSTaggedValue obj)
{
    INTERPRETER_TRACE(thread, GetIterator);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    return RuntimeStubs::RuntimeGetIterator(thread, objHandle);
}

JSTaggedValue SlowRuntimeStub::GetAsyncIterator(JSThread *thread, JSTaggedValue obj)
{
    INTERPRETER_TRACE(thread, GetAsyncIterator);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    return RuntimeStubs::RuntimeGetAsyncIterator(thread, objHandle);
}

JSTaggedValue SlowRuntimeStub::DefineGetterSetterByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop,
                                                         JSTaggedValue getter, JSTaggedValue setter, bool flag)
{
    INTERPRETER_TRACE(thread, DefineGetterSetterByValue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSObject> objHandle(thread, obj);
    JSHandle<JSTaggedValue> propHandle(thread, prop);
    JSHandle<JSTaggedValue> getterHandle(thread, getter);
    JSHandle<JSTaggedValue> setterHandle(thread, setter);
    JSHandle<JSTaggedValue> func(thread, JSTaggedValue::Undefined());
    return RuntimeStubs::RuntimeDefineGetterSetterByValue(thread, objHandle, propHandle,
                                                          getterHandle, setterHandle, flag,
                                                          func, 0);
}

JSTaggedValue SlowRuntimeStub::LdObjByIndex(JSThread *thread, JSTaggedValue obj, uint32_t idx, bool callGetter,
                                            JSTaggedValue receiver)
{
    INTERPRETER_TRACE(thread, LdObjByIndex);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    return RuntimeStubs::RuntimeLdObjByIndex(thread, objHandle, idx, callGetter, receiver);
}

JSTaggedValue SlowRuntimeStub::StObjByIndex(JSThread *thread, JSTaggedValue obj, uint32_t idx, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, StObjByIndex);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    return RuntimeStubs::RuntimeStObjByIndex(thread, objHandle, idx, valueHandle);
}

JSTaggedValue SlowRuntimeStub::LdObjByName(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop, bool callGetter,
                                           JSTaggedValue receiver)
{
    INTERPRETER_TRACE(thread, LdObjByName);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    return RuntimeStubs::RuntimeLdObjByName(thread, obj, prop, callGetter, receiver);
}

JSTaggedValue SlowRuntimeStub::StObjByName(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, StObjByName);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> propHandle(thread, prop);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    return RuntimeStubs::RuntimeStObjByName(thread, objHandle, propHandle, valueHandle);
}

JSTaggedValue SlowRuntimeStub::LdObjByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop, bool callGetter,
                                            JSTaggedValue receiver)
{
    INTERPRETER_TRACE(thread, LdObjByValue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> propHandle(thread, prop);
    return RuntimeStubs::RuntimeLdObjByValue(thread, objHandle, propHandle, callGetter, receiver);
}

JSTaggedValue SlowRuntimeStub::StObjByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop,
                                            JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, StObjByValue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> propHandle(thread, prop);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    return RuntimeStubs::RuntimeStObjByValue(thread, objHandle, propHandle, valueHandle);
}

JSTaggedValue SlowRuntimeStub::TryLdGlobalByNameFromGlobalProto(JSThread *thread, JSTaggedValue global,
                                                                JSTaggedValue prop)
{
    INTERPRETER_TRACE(thread, Trygetobjprop);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> obj(thread, global.GetTaggedObject()->GetClass()->GetPrototype());
    JSHandle<JSTaggedValue> propHandle(thread, prop);
    return RuntimeStubs::RuntimeTryLdGlobalByName(thread, obj, propHandle);
}

JSTaggedValue SlowRuntimeStub::TryStGlobalByName(JSThread *thread, JSTaggedValue prop)
{
    INTERPRETER_TRACE(thread, TryStGlobalByName);
    // If fast path is fail, not need slow path, just throw error.
    return ThrowReferenceError(thread, prop, " is not defined");
}

JSTaggedValue SlowRuntimeStub::LdGlobalVarFromGlobalProto(JSThread *thread, JSTaggedValue global, JSTaggedValue prop)
{
    INTERPRETER_TRACE(thread, LdGlobalVar);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, global);
    JSHandle<JSTaggedValue> propHandle(thread, prop);
    return RuntimeStubs::RuntimeLdGlobalVarFromProto(thread, objHandle, propHandle); // After checked global itself.
}

JSTaggedValue SlowRuntimeStub::StGlobalVar(JSThread *thread, JSTaggedValue prop, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, StGlobalVar);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> propHandle(thread, prop);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    return RuntimeStubs::RuntimeStGlobalVar(thread, propHandle, valueHandle);
}

JSTaggedValue SlowRuntimeStub::TryUpdateGlobalRecord(JSThread *thread, JSTaggedValue prop, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, TryUpdateGlobalRecord);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    return RuntimeStubs::RuntimeTryUpdateGlobalRecord(thread, prop, value);
}

// return box
JSTaggedValue SlowRuntimeStub::LdGlobalRecord(JSThread *thread, JSTaggedValue key)
{
    INTERPRETER_TRACE(thread, LdGlobalRecord);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    return RuntimeStubs::RuntimeLdGlobalRecord(thread, key);
}

JSTaggedValue SlowRuntimeStub::StGlobalRecord(JSThread *thread, JSTaggedValue prop, JSTaggedValue value, bool isConst)
{
    INTERPRETER_TRACE(thread, StGlobalRecord);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> propHandle(thread, prop);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    return RuntimeStubs::RuntimeStGlobalRecord(thread, propHandle, valueHandle, isConst);
}

JSTaggedValue SlowRuntimeStub::ThrowReferenceError(JSThread *thread, JSTaggedValue prop, const char *desc)
{
    INTERPRETER_TRACE(thread, ThrowReferenceError);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> propHandle(thread, prop);
    return RuntimeStubs::RuntimeThrowReferenceError(thread, propHandle, desc);
}

JSTaggedValue SlowRuntimeStub::ThrowTypeError(JSThread *thread, const char *message)
{
    INTERPRETER_TRACE(thread, ThrowTypeError);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    return RuntimeStubs::RuntimeThrowTypeError(thread, message);
}

JSTaggedValue SlowRuntimeStub::ThrowSyntaxError(JSThread *thread, const char *message)
{
    INTERPRETER_TRACE(thread, ThrowSyntaxError);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    return RuntimeStubs::RuntimeThrowSyntaxError(thread, message);
}

JSTaggedValue SlowRuntimeStub::StArraySpread(JSThread *thread, JSTaggedValue dst, JSTaggedValue index,
                                             JSTaggedValue src)
{
    INTERPRETER_TRACE(thread, StArraySpread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> dstHandle(thread, dst);
    JSHandle<JSTaggedValue> srcHandle(thread, src);
    return RuntimeStubs::RuntimeStArraySpread(thread, dstHandle, index, srcHandle);
}

JSTaggedValue SlowRuntimeStub::DefineFunc(JSThread *thread, JSTaggedValue constPool, uint16_t methodId,
                                          JSTaggedValue module, uint16_t length, JSTaggedValue env,
                                          JSTaggedValue homeObject)
{
    INTERPRETER_TRACE(thread, DefineFunc);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> constpoolHandle(thread, constPool);
    JSHandle<JSTaggedValue> moduleHandle(thread, module);
    JSHandle<JSTaggedValue> envHandle(thread, env);
    JSHandle<JSTaggedValue> homeObjectHandle(thread, homeObject);
    return RuntimeStubs::RuntimeDefinefunc(thread, constpoolHandle, methodId, moduleHandle,
                                           length, envHandle, homeObjectHandle);
}

JSTaggedValue SlowRuntimeStub::GetSuperConstructor(JSThread *thread, JSTaggedValue ctor)
{
    INTERPRETER_TRACE(thread, GetSuperConstructor);
    JSHandle<JSTaggedValue> ctorHandle(thread, ctor);
    JSHandle<JSTaggedValue> superConstructor(thread, JSTaggedValue::GetPrototype(thread, ctorHandle));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return superConstructor.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::SuperCall(JSThread *thread, JSTaggedValue func, JSTaggedValue newTarget,
                                         uint16_t firstVRegIdx, uint16_t length)
{
    INTERPRETER_TRACE(thread, SuperCall);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> funcHandle(thread, func);
    JSHandle<JSTaggedValue> newTargetHandle(thread, newTarget);
    return RuntimeStubs::RuntimeSuperCall(thread, funcHandle, newTargetHandle, firstVRegIdx, length);
}

JSTaggedValue SlowRuntimeStub::DynamicImport(JSThread *thread, JSTaggedValue specifier, JSTaggedValue currentFunc)
{
    INTERPRETER_TRACE(thread, DynamicImport);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> specifierHandle(thread, specifier);
    JSHandle<JSTaggedValue> currentFuncHandle(thread, currentFunc);
    return RuntimeStubs::RuntimeDynamicImport(thread, specifierHandle, currentFuncHandle);
}

JSTaggedValue SlowRuntimeStub::SuperCallSpread(JSThread *thread, JSTaggedValue func, JSTaggedValue newTarget,
                                               JSTaggedValue array)
{
    INTERPRETER_TRACE(thread, SuperCallSpread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> funcHandle(thread, func);
    JSHandle<JSTaggedValue> newTargetHandle(thread, newTarget);
    JSHandle<JSTaggedValue> jsArray(thread, array);
    return RuntimeStubs::RuntimeSuperCallSpread(thread, funcHandle, newTargetHandle, jsArray);
}

JSTaggedValue SlowRuntimeStub::SuperCallForwardAllArgs(JSThread *thread, JSTaggedType *sp, JSTaggedValue func,
                                                       JSTaggedValue newTarget, uint32_t restNumArgs, uint32_t startIdx)
{
    JSHandle<JSTaggedValue> superFunc(thread, GetSuperConstructor(thread, func));
    JSHandle<JSTaggedValue> newTargetHandle(thread, newTarget);
    return RuntimeStubs::RuntimeSuperCallForwardAllArgs(thread, sp, superFunc, newTargetHandle, restNumArgs, startIdx);
}

JSTaggedValue SlowRuntimeStub::DefineMethod(JSThread *thread, Method *method, JSTaggedValue homeObject,
                                            uint16_t length, JSTaggedValue env, JSTaggedValue module)
{
    INTERPRETER_TRACE(thread, DefineMethod);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<Method> methodHandle(thread, method);
    JSHandle<JSTaggedValue> homeObjectHandle(thread, homeObject);
    JSHandle<JSTaggedValue> envHandle(thread, env);
    JSHandle<JSTaggedValue> moduleHandle(thread, module);
    return RuntimeStubs::RuntimeDefineMethod(thread, methodHandle, homeObjectHandle, length, envHandle, moduleHandle);
}

JSTaggedValue SlowRuntimeStub::LdSendableClass(JSThread *thread, JSTaggedValue env, uint16_t level)
{
    INTERPRETER_TRACE(thread, LdSendableClass);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> envHandle(thread, env);
    return RuntimeStubs::RuntimeLdSendableClass(envHandle, level);
}

JSTaggedValue SlowRuntimeStub::LdSendableExternalModuleVar(JSThread *thread, int32_t index, JSTaggedValue thisFunc)
{
    RUNTIME_TRACE(thread, LdSendableExternalModuleVarByIndex);
    [[maybe_unused]] EcmaHandleScope scope(thread);

    return RuntimeStubs::RuntimeLdSendableExternalModuleVar(thread, index, thisFunc);
}

JSTaggedValue SlowRuntimeStub::LdLazyExternalModuleVar(JSThread *thread, int32_t index, JSTaggedValue thisFunc)
{
    RUNTIME_TRACE(thread, LdLazyExternalModuleVarByIndex);
    [[maybe_unused]] EcmaHandleScope scope(thread);
    return RuntimeStubs::RuntimeLdLazyExternalModuleVar(thread, index, thisFunc);
}

JSTaggedValue SlowRuntimeStub::LdLazySendableExternalModuleVar(JSThread *thread, int32_t index, JSTaggedValue thisFunc)
{
    RUNTIME_TRACE(thread, LdLazySendableExternalModuleVarByIndex);
    [[maybe_unused]] EcmaHandleScope scope(thread);

    return RuntimeStubs::RuntimeLdLazySendableExternalModuleVar(thread, index, thisFunc);
}

JSTaggedValue SlowRuntimeStub::LdSuperByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue key,
                                              JSTaggedValue thisFunc)
{
    INTERPRETER_TRACE(thread, LdSuperByValue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> propHandle(thread, key);
    return RuntimeStubs::RuntimeLdSuperByValue(thread, objHandle, propHandle, thisFunc);
}

JSTaggedValue SlowRuntimeStub::StSuperByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue key,
                                              JSTaggedValue value, JSTaggedValue thisFunc)
{
    INTERPRETER_TRACE(thread, StSuperByValue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> propHandle(thread, key);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    return RuntimeStubs::RuntimeStSuperByValue(thread, objHandle, propHandle, valueHandle, thisFunc);
}

JSTaggedValue SlowRuntimeStub::GetCallSpreadArgs(JSThread *thread, JSTaggedValue array)
{
    INTERPRETER_TRACE(thread, GetCallSpreadArgs);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> jsArray(thread, array);
    return RuntimeStubs::RuntimeGetCallSpreadArgs(thread, jsArray);
}

void SlowRuntimeStub::ThrowDeleteSuperProperty(JSThread *thread)
{
    INTERPRETER_TRACE(thread, ThrowDeleteSuperProperty);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    return RuntimeStubs::RuntimeThrowDeleteSuperProperty(thread);
}

JSTaggedValue SlowRuntimeStub::NotifyInlineCache(JSThread *thread, JSFunction *function)
{
    INTERPRETER_TRACE(thread, NotifyInlineCache);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSFunction> functionHandle(thread, function);
    uint32_t slotSize = functionHandle->GetCallTarget()->GetSlotSize();
    return RuntimeStubs::RuntimeNotifyInlineCache(thread, functionHandle, slotSize);
}

JSTaggedValue SlowRuntimeStub::ResolveClass(JSThread *thread, JSTaggedValue ctor, TaggedArray *literal,
                                            JSTaggedValue base, JSTaggedValue lexenv)
{
    JSHandle<JSFunction> cls(thread, ctor);
    JSHandle<TaggedArray> literalBuffer(thread, literal);
    JSHandle<JSTaggedValue> baseHandle(thread, base);
    JSHandle<JSTaggedValue> lexicalEnv(thread, lexenv);
    return RuntimeStubs::RuntimeResolveClass(thread, cls, literalBuffer, baseHandle, lexicalEnv);
}

// clone class may need re-set inheritance relationship due to extends may be a variable.
JSTaggedValue SlowRuntimeStub::CloneClassFromTemplate(JSThread *thread, JSTaggedValue ctor, JSTaggedValue base,
                                                      JSTaggedValue lexenv)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> lexenvHandle(thread, lexenv);
    JSHandle<JSTaggedValue> baseHandle(thread, base);
    JSHandle<JSFunction> cls(thread, ctor);
    return RuntimeStubs::RuntimeCloneClassFromTemplate(thread, cls, baseHandle, lexenvHandle);
}

// clone class may need re-set inheritance relationship due to extends may be a variable.
JSTaggedValue SlowRuntimeStub::CreateClassWithBuffer(JSThread *thread, JSTaggedValue base,
                                                     JSTaggedValue lexenv, JSTaggedValue constpool,
                                                     uint16_t methodId, uint16_t literalId, JSTaggedValue module,
                                                     JSTaggedValue length)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> baseHandle(thread, base);
    JSHandle<JSTaggedValue> lexenvHandle(thread, lexenv);
    JSHandle<JSTaggedValue> constpoolHandle(thread, constpool);
    JSHandle<JSTaggedValue> moduleHandle(thread, module);
    JSHandle<JSTaggedValue> lengthHandle(thread, length);
    return RuntimeStubs::RuntimeCreateClassWithBuffer(thread, baseHandle, lexenvHandle,
                                                      constpoolHandle, methodId, literalId,
                                                      moduleHandle, lengthHandle);
}

JSTaggedValue SlowRuntimeStub::CreateSharedClass(JSThread *thread, JSTaggedValue base,
                                                 JSTaggedValue constpool, uint16_t methodId, uint16_t literalId,
                                                 uint16_t length, JSTaggedValue module)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> baseHandle(thread, base);
    JSHandle<JSTaggedValue> constpoolHandle(thread, constpool);
    JSHandle<JSTaggedValue> moduleHandle(thread, module);
    return RuntimeStubs::RuntimeCreateSharedClass(thread, baseHandle, constpoolHandle, methodId,
                                                  literalId, length, moduleHandle);
}

JSTaggedValue SlowRuntimeStub::SetClassInheritanceRelationship(JSThread *thread, JSTaggedValue ctor, JSTaggedValue base)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> cls(thread, ctor);
    JSHandle<JSTaggedValue> parent(thread, base);
    return RuntimeStubs::RuntimeSetClassInheritanceRelationship(thread, cls, parent);
}

JSTaggedValue SlowRuntimeStub::SetClassConstructorLength(JSThread *thread, JSTaggedValue ctor, JSTaggedValue length)
{
    return RuntimeStubs::RuntimeSetClassConstructorLength(thread, ctor, length);
}

JSTaggedValue SlowRuntimeStub::GetModuleNamespace(JSThread *thread, int32_t index)
{
    return RuntimeStubs::RuntimeGetModuleNamespace(thread, index);
}

JSTaggedValue SlowRuntimeStub::GetModuleNamespace(JSThread *thread, JSTaggedValue localName)
{
    return RuntimeStubs::RuntimeGetModuleNamespace(thread, localName);
}

JSTaggedValue SlowRuntimeStub::LdBigInt(JSThread *thread, JSTaggedValue numberBigInt)
{
    INTERPRETER_TRACE(thread, LdBigInt);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> bigint(thread, numberBigInt);
    return RuntimeStubs::RuntimeLdBigInt(thread, bigint);
}
JSTaggedValue SlowRuntimeStub::AsyncGeneratorResolve(JSThread *thread, JSTaggedValue asyncFuncObj,
                                                     const JSTaggedValue value, JSTaggedValue flag)
{
    INTERPRETER_TRACE(thread, AsyncGeneratorResolve);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> genObjHandle(thread, asyncFuncObj);
    JSHandle<JSTaggedValue> valueHandle(thread, value);

    return RuntimeStubs::RuntimeAsyncGeneratorResolve(thread, genObjHandle, valueHandle, flag);
}

JSTaggedValue SlowRuntimeStub::AsyncGeneratorReject(JSThread *thread, JSTaggedValue asyncFuncObj,
                                                    const JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, AsyncGeneratorReject);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSAsyncGeneratorObject> asyncFuncObjHandle(thread, asyncFuncObj);
    JSHandle<JSTaggedValue> valueHandle(thread, value);

    return JSAsyncGeneratorObject::AsyncGeneratorReject(thread, asyncFuncObjHandle, valueHandle);
}

JSTaggedValue SlowRuntimeStub::LdPatchVar(JSThread *thread, uint32_t index)
{
    INTERPRETER_TRACE(thread, LdPatchVar);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    return RuntimeStubs::RuntimeLdPatchVar(thread, index);
}

JSTaggedValue SlowRuntimeStub::StPatchVar(JSThread *thread, uint32_t index, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, StPatchVar);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> valueHandle(thread, value);
    return RuntimeStubs::RuntimeStPatchVar(thread, index, valueHandle);
}

JSTaggedValue SlowRuntimeStub::NotifyConcurrentResult(JSThread *thread, JSTaggedValue result, JSTaggedValue hint)
{
    INTERPRETER_TRACE(thread, NotifyConcurrentResult);
    return RuntimeStubs::RuntimeNotifyConcurrentResult(thread, result, hint);
}

JSTaggedValue SlowRuntimeStub::UpdateAOTHClass(JSThread *thread, JSTaggedValue jshclass,
                                               JSTaggedValue newjshclass, JSTaggedValue key)
{
    INTERPRETER_TRACE(thread, UpdateAOTHClass);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSHClass> oldhclass(thread, jshclass);
    JSHandle<JSHClass> newhclass(thread, newjshclass);
    return RuntimeStubs::RuntimeUpdateAOTHClass(thread, oldhclass, newhclass, key);
}

JSTaggedValue SlowRuntimeStub::DefineField(JSThread *thread, JSTaggedValue obj,
                                           JSTaggedValue propKey, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, DefineField);
    return RuntimeStubs::RuntimeDefineField(thread, obj, propKey, value);
}

JSTaggedValue SlowRuntimeStub::CreatePrivateProperty(JSThread *thread, JSTaggedValue lexicalEnv,
    uint32_t count, JSTaggedValue constpool, uint32_t literalId, JSTaggedValue module)
{
    INTERPRETER_TRACE(thread, CreatePrivateProperty);
    return RuntimeStubs::RuntimeCreatePrivateProperty(thread, lexicalEnv, count, constpool, literalId, module);
}

JSTaggedValue SlowRuntimeStub::DefinePrivateProperty(JSThread *thread, JSTaggedValue lexicalEnv,
    uint32_t levelIndex, uint32_t slotIndex, JSTaggedValue obj, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, DefinePrivateProperty);
    return RuntimeStubs::RuntimeDefinePrivateProperty(thread, lexicalEnv, levelIndex, slotIndex, obj, value);
}

JSTaggedValue SlowRuntimeStub::LdPrivateProperty(JSThread *thread, JSTaggedValue lexicalEnv,
    uint32_t levelIndex, uint32_t slotIndex, JSTaggedValue obj)
{
    INTERPRETER_TRACE(thread, LdPrivateProperty);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    return RuntimeStubs::RuntimeLdPrivateProperty(thread, lexicalEnv, levelIndex, slotIndex, obj);
}

JSTaggedValue SlowRuntimeStub::StPrivateProperty(JSThread *thread, JSTaggedValue lexicalEnv,
    uint32_t levelIndex, uint32_t slotIndex, JSTaggedValue obj, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, StPrivateProperty);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    return RuntimeStubs::RuntimeStPrivateProperty(thread, lexicalEnv, levelIndex, slotIndex, obj, value);
}

JSTaggedValue SlowRuntimeStub::TestIn(JSThread *thread, JSTaggedValue lexicalEnv,
    uint32_t levelIndex, uint32_t slotIndex, JSTaggedValue obj)
{
    INTERPRETER_TRACE(thread, TestIn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    return RuntimeStubs::RuntimeTestIn(thread, lexicalEnv, levelIndex, slotIndex, obj);
}
}  // namespace panda::ecmascript
