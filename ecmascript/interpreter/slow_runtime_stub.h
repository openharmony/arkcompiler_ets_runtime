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

#ifndef ECMASCRIPT_INTERPRETER_SLOW_RUNTIME_STUB_H
#define ECMASCRIPT_INTERPRETER_SLOW_RUNTIME_STUB_H

#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {
class GlobalEnv;
class JSArray;

class SlowRuntimeStub {
public:
    static JSTaggedValue CallSpread(JSThread *thread, JSTaggedValue func, JSTaggedValue obj, JSTaggedValue array);
    static JSTaggedValue Neg(JSThread *thread, JSTaggedValue value);
    static JSTaggedValue AsyncFunctionEnter(JSThread *thread);
    static JSTaggedValue ToNumber(JSThread *thread, JSTaggedValue value);
    static JSTaggedValue ToNumeric(JSThread *thread, JSTaggedValue value);
    static JSTaggedValue Not(JSThread *thread, JSTaggedValue value);
    static JSTaggedValue Inc(JSThread *thread, JSTaggedValue value);
    static JSTaggedValue Dec(JSThread *thread, JSTaggedValue value);
    static void Throw(JSThread *thread, JSTaggedValue value);
    static JSTaggedValue GetPropIterator(JSThread *thread, JSTaggedValue value);
    static void ThrowConstAssignment(JSThread *thread, JSTaggedValue value);
    static JSTaggedValue Add2(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue Sub2(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue Mul2(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue Div2(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue Mod2(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue Eq(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue NotEq(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue Less(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue LessEq(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue Greater(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue GreaterEq(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue Shl2(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue Shr2(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue Ashr2(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue And2(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue Or2(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue Xor2(JSThread *thread, JSTaggedValue left, JSTaggedValue right);

    static JSTaggedValue ToJSTaggedValueWithInt32(JSThread *thread, JSTaggedValue value);
    static JSTaggedValue ToJSTaggedValueWithUint32(JSThread *thread, JSTaggedValue value);

    static JSTaggedValue DelObjProp(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop);
    static JSTaggedValue NewObjRange(JSThread *thread, JSTaggedValue func, JSTaggedValue newTarget,
                                        uint16_t firstArgIdx, uint16_t length);
    static JSTaggedValue CreateObjectWithExcludedKeys(JSThread *thread, uint16_t numKeys, JSTaggedValue objVal,
                                                      uint16_t firstArgRegIdx);
    static JSTaggedValue Exp(JSThread *thread, JSTaggedValue base, JSTaggedValue exponent);
    static JSTaggedValue IsIn(JSThread *thread, JSTaggedValue prop, JSTaggedValue obj);
    static JSTaggedValue Instanceof(JSThread *thread, JSTaggedValue obj, JSTaggedValue target);
    static JSTaggedValue InstanceofByHandler(JSThread *thread, JSTaggedValue target, JSTaggedValue object,
                                             JSTaggedValue instOfHandler);

    static JSTaggedValue NewLexicalEnv(JSThread *thread, uint16_t numVars);
    static JSTaggedValue NewSendableEnv(JSThread *thread, uint16_t numVars);
    static JSTaggedValue NewLexicalEnvWithName(JSThread *thread, uint16_t numVars, uint16_t scopeId);
    static JSTaggedValue CreateIterResultObj(JSThread *thread, JSTaggedValue value, JSTaggedValue flag);

    static JSTaggedValue CreateGeneratorObj(JSThread *thread, JSTaggedValue genFunc);
    static JSTaggedValue SuspendGenerator(JSThread *thread, JSTaggedValue genObj, JSTaggedValue value);
    static void SetGeneratorState(JSThread *thread, JSTaggedValue genObj, int32_t index);
    static JSTaggedValue AsyncFunctionAwaitUncaught(JSThread *thread, JSTaggedValue asyncFuncObj, JSTaggedValue value);
    static JSTaggedValue AsyncFunctionResolveOrReject(JSThread *thread, JSTaggedValue asyncFuncObj, JSTaggedValue value,
                                                      bool is_resolve);

    static JSTaggedValue NewObjApply(JSThread *thread, JSTaggedValue func, JSTaggedValue array);
    static void ThrowUndefinedIfHole(JSThread *thread, JSTaggedValue obj);
    static void ThrowIfNotObject(JSThread *thread);
    static void ThrowThrowNotExists(JSThread *thread);
    static void ThrowPatternNonCoercible(JSThread *thread);
    static JSTaggedValue ThrowIfSuperNotCorrectCall(JSThread *thread, uint16_t index, JSTaggedValue thisValue);
    static void ThrowDeleteSuperProperty(JSThread *thread);

    static JSTaggedValue StOwnByName(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop, JSTaggedValue value);
    static JSTaggedValue StOwnByNameWithNameSet(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop,
                                                JSTaggedValue value);
    static JSTaggedValue StOwnByIndex(JSThread *thread, JSTaggedValue obj, uint32_t idx, JSTaggedValue value);
    static JSTaggedValue StOwnByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue key, JSTaggedValue value);
    static JSTaggedValue StOwnByValueWithNameSet(JSThread *thread, JSTaggedValue obj, JSTaggedValue key,
                                                 JSTaggedValue value);
    static JSTaggedValue CreateEmptyArray(JSThread *thread, ObjectFactory *factory, JSHandle<GlobalEnv> globalEnv);
    static JSTaggedValue CreateEmptyObject(JSThread *thread, ObjectFactory *factory, JSHandle<GlobalEnv> globalEnv);
    static JSTaggedValue CreateObjectWithBuffer(JSThread *thread, ObjectFactory *factory, JSObject *literal);
    static JSTaggedValue CreateObjectHavingMethod(JSThread *thread, ObjectFactory *factory, JSObject *literal,
                                                  JSTaggedValue env);
    static JSTaggedValue SetObjectWithProto(JSThread *thread, JSTaggedValue proto, JSTaggedValue obj);
    static JSTaggedValue CreateArrayWithBuffer(JSThread *thread, ObjectFactory *factory, JSArray *literal);

    static JSTaggedValue GetTemplateObject(JSThread *thread, JSTaggedValue literal);
    static JSTaggedValue GetNextPropName(JSThread *thread, JSTaggedValue iter);
    static JSTaggedValue CopyDataProperties(JSThread *thread, JSTaggedValue dst, JSTaggedValue src);

    static JSTaggedValue GetUnmapedArgs(JSThread *thread, JSTaggedType *sp, uint32_t actualNumArgs, uint32_t startIdx);
    static JSTaggedValue CopyRestArgs(JSThread *thread, JSTaggedType *sp, uint32_t restNumArgs, uint32_t startIdx);
    static JSTaggedValue GetIterator(JSThread *thread, JSTaggedValue obj);
    static JSTaggedValue GetAsyncIterator(JSThread *thread, JSTaggedValue obj);
    static JSTaggedValue IterNext(JSThread *thread, JSTaggedValue iter);
    static JSTaggedValue CloseIterator(JSThread *thread, JSTaggedValue iter);
    static void StModuleVar(JSThread *thread, JSTaggedValue key, JSTaggedValue value);
    static JSTaggedValue LdModuleVar(JSThread *thread, JSTaggedValue key, bool inner);
    static void StModuleVar(JSThread *thread, int32_t index, JSTaggedValue value);
    static JSTaggedValue LdLocalModuleVar(JSThread *thread, int32_t index);
    static JSTaggedValue LdExternalModuleVar(JSThread *thread, int32_t index);
    static JSTaggedValue LdSendableExternalModuleVar(JSThread *thread, int32_t index, JSTaggedValue thisFunc);
    static JSTaggedValue LdLazyExternalModuleVar(JSThread *thread, int32_t index, JSTaggedValue thisFunc);
    static JSTaggedValue LdLazySendableExternalModuleVar(JSThread *thread, int32_t index, JSTaggedValue thisFunc);
    static JSTaggedValue CreateRegExpWithLiteral(JSThread *thread, JSTaggedValue pattern, uint8_t flags);
    static JSTaggedValue GetIteratorNext(JSThread *thread, JSTaggedValue obj, JSTaggedValue method);

    static JSTaggedValue DefineGetterSetterByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop,
                                                   JSTaggedValue getter, JSTaggedValue setter, bool flag);

    static JSTaggedValue LdObjByIndex(JSThread *thread, JSTaggedValue obj, uint32_t idx, bool callGetter,
                                      JSTaggedValue receiver);
    static JSTaggedValue StObjByIndex(JSThread *thread, JSTaggedValue obj, uint32_t idx, JSTaggedValue value);
    static JSTaggedValue LdObjByName(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop, bool callGetter,
                                     JSTaggedValue receiver);
    static JSTaggedValue StObjByName(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop, JSTaggedValue value);
    static JSTaggedValue LdObjByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop, bool callGetter,
                                      JSTaggedValue receiver);
    static JSTaggedValue StObjByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop, JSTaggedValue value);
    static JSTaggedValue TryLdGlobalByNameFromGlobalProto(JSThread *thread, JSTaggedValue global, JSTaggedValue prop);
    static JSTaggedValue TryStGlobalByName(JSThread *thread, JSTaggedValue prop);
    static JSTaggedValue LdGlobalVarFromGlobalProto(JSThread *thread, JSTaggedValue global, JSTaggedValue prop);
    static JSTaggedValue StGlobalVar(JSThread *thread, JSTaggedValue prop, JSTaggedValue value);
    static JSTaggedValue StGlobalRecord(JSThread *thread, JSTaggedValue prop, JSTaggedValue value, bool isConst);
    static JSTaggedValue LdGlobalRecord(JSThread *thread, JSTaggedValue key);
    static JSTaggedValue TryUpdateGlobalRecord(JSThread *thread, JSTaggedValue prop, JSTaggedValue value);
    static JSTaggedValue StArraySpread(JSThread *thread, JSTaggedValue dst, JSTaggedValue index, JSTaggedValue src);

    static JSTaggedValue DefineFunc(JSThread *thread, JSTaggedValue constPool, uint16_t methodId,
                                    JSTaggedValue module, uint16_t length, JSTaggedValue envHandle,
                                    JSTaggedValue homeObject);
    static JSTaggedValue GetSuperConstructor(JSThread *thread, JSTaggedValue ctor);
    static JSTaggedValue SuperCall(JSThread *thread, JSTaggedValue func, JSTaggedValue newTarget, uint16_t firstVRegIdx,
                                   uint16_t length);
    static JSTaggedValue SuperCallSpread(JSThread *thread, JSTaggedValue func, JSTaggedValue newTarget,
                                         JSTaggedValue array);
    static JSTaggedValue SuperCallForwardAllArgs(JSThread *thread, JSTaggedType *sp, JSTaggedValue func,
                                                 JSTaggedValue newTarget, uint32_t restNumArgs, uint32_t startIdx);
    static JSTaggedValue DynamicImport(JSThread *thread, JSTaggedValue specifier, JSTaggedValue func);
    static JSTaggedValue DefineMethod(JSThread *thread, Method *method, JSTaggedValue homeObject,
                                      uint16_t length, JSTaggedValue env, JSTaggedValue module);
    static JSTaggedValue LdSendableClass(JSThread *thread, JSTaggedValue env, uint16_t level);
    static JSTaggedValue LdSuperByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue key, JSTaggedValue thisFunc);
    static JSTaggedValue StSuperByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue key, JSTaggedValue value,
                                        JSTaggedValue thisFunc);
    static JSTaggedValue NotifyInlineCache(JSThread *thread, JSFunction *function);
    static JSTaggedValue ThrowReferenceError(JSThread *thread, JSTaggedValue prop, const char *desc);

    static JSTaggedValue ResolveClass(JSThread *thread, JSTaggedValue ctor, TaggedArray *literal, JSTaggedValue base,
                                      JSTaggedValue lexenv);
    static JSTaggedValue CloneClassFromTemplate(JSThread *thread, JSTaggedValue ctor, JSTaggedValue base,
                                                JSTaggedValue lexenv);
    static JSTaggedValue CreateClassWithBuffer(JSThread *thread, JSTaggedValue base,
                                               JSTaggedValue lexenv, JSTaggedValue constpool,
                                               uint16_t methodId, uint16_t literalId, JSTaggedValue module,
                                               JSTaggedValue length);
    static JSTaggedValue CreateSharedClass(JSThread *thread, JSTaggedValue base,
                                           JSTaggedValue constpool, uint16_t methodId, uint16_t literalId,
                                           uint16_t length, JSTaggedValue module);
    static JSTaggedValue SetClassConstructorLength(JSThread *thread, JSTaggedValue ctor, JSTaggedValue length);
    static JSTaggedValue GetModuleNamespace(JSThread *thread, JSTaggedValue localName);
    static JSTaggedValue GetModuleNamespace(JSThread *thread, int32_t index);
    static JSTaggedValue LdBigInt(JSThread *thread, JSTaggedValue numberBigInt);
    static JSTaggedValue ThrowTypeError(JSThread *thread, const char *message);
    static JSTaggedValue SetClassInheritanceRelationship(JSThread *thread, JSTaggedValue ctor, JSTaggedValue base);

    static JSTaggedValue AsyncGeneratorResolve(JSThread *thread, JSTaggedValue asyncFuncObj,
                                               const JSTaggedValue value, JSTaggedValue flag);
    static JSTaggedValue AsyncGeneratorReject(JSThread *thread, JSTaggedValue asyncFuncObj,
                                              const JSTaggedValue value);
    static JSTaggedValue CreateAsyncGeneratorObj(JSThread *thread, JSTaggedValue genFunc);

    static JSTaggedValue LdPatchVar(JSThread *thread, uint32_t index);
    static JSTaggedValue StPatchVar(JSThread *thread, uint32_t index, JSTaggedValue value);

    static JSTaggedValue NotifyConcurrentResult(JSThread *thread, JSTaggedValue result, JSTaggedValue hint);
    static JSTaggedValue DefineField(JSThread *thread, JSTaggedValue obj, JSTaggedValue propKey, JSTaggedValue value);
    static JSTaggedValue CreatePrivateProperty(JSThread *thread, JSTaggedValue lexicalEnv,
        uint32_t count, JSTaggedValue constpool, uint32_t literalId, JSTaggedValue module);
    static JSTaggedValue DefinePrivateProperty(JSThread *thread, JSTaggedValue lexicalEnv,
        uint32_t levelIndex, uint32_t slotIndex, JSTaggedValue obj, JSTaggedValue value);
    static JSTaggedValue LdPrivateProperty(JSThread *thread, JSTaggedValue lexicalEnv,
        uint32_t levelIndex, uint32_t slotIndex, JSTaggedValue obj);
    static JSTaggedValue StPrivateProperty(JSThread *thread, JSTaggedValue lexicalEnv,
        uint32_t levelIndex, uint32_t slotIndex, JSTaggedValue obj, JSTaggedValue value);
    static JSTaggedValue TestIn(JSThread *thread, JSTaggedValue lexicalEnv,
        uint32_t levelIndex, uint32_t slotIndex, JSTaggedValue obj);
    static JSTaggedValue UpdateAOTHClass(JSThread *thread, JSTaggedValue jshclass,
                                         JSTaggedValue newjshclass, JSTaggedValue key);

private:
    static JSTaggedValue ThrowSyntaxError(JSThread *thread, const char *message);
    static JSTaggedValue GetCallSpreadArgs(JSThread *thread, JSTaggedValue array);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_INTERPRETER_SLOW_RUNTIME_STUB_H
