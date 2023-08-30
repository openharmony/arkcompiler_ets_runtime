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

#ifndef ECMASCRIPT_STUBS_RUNTIME_STUBS_INL_H
#define ECMASCRIPT_STUBS_RUNTIME_STUBS_INL_H

#include "ecmascript/stubs/runtime_stubs.h"

#include "ecmascript/base/array_helper.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/js_async_function.h"
#include "ecmascript/js_async_generator_object.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/jspandafile/class_info_extractor.h"
#include "ecmascript/jspandafile/literal_data_extractor.h"
#include "ecmascript/jspandafile/scope_info_extractor.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/stackmap/llvm_stackmap_parser.h"
#include "ecmascript/template_string.h"
#include "ecmascript/ts_types/ts_manager.h"

namespace panda::ecmascript {
using ArrayHelper = base::ArrayHelper;
JSTaggedValue RuntimeStubs::RuntimeInc(JSThread *thread, const JSHandle<JSTaggedValue> &value)
{
    JSHandle<JSTaggedValue> inputVal = JSTaggedValue::ToNumeric(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (inputVal->IsBigInt()) {
        JSHandle<BigInt> bigValue(inputVal);
        return BigInt::BigintAddOne(thread, bigValue).GetTaggedValue();
    }
    JSTaggedNumber number(inputVal.GetTaggedValue());
    return JSTaggedValue(++number);
}

JSTaggedValue RuntimeStubs::RuntimeDec(JSThread *thread, const JSHandle<JSTaggedValue> &value)
{
    JSHandle<JSTaggedValue> inputVal = JSTaggedValue::ToNumeric(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (inputVal->IsBigInt()) {
        JSHandle<BigInt> bigValue(inputVal);
        return BigInt::BigintSubOne(thread, bigValue).GetTaggedValue();
    }
    JSTaggedNumber number(inputVal.GetTaggedValue());
    return JSTaggedValue(--number);
}

JSTaggedValue RuntimeStubs::RuntimeExp(JSThread *thread, JSTaggedValue base, JSTaggedValue exponent)
{
    JSHandle<JSTaggedValue> baseTag(thread, base);
    JSHandle<JSTaggedValue> exponentTag(thread, exponent);
    JSHandle<JSTaggedValue> valBase = JSTaggedValue::ToNumeric(thread, baseTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valExponent = JSTaggedValue::ToNumeric(thread, exponentTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (valBase->IsBigInt() || valExponent->IsBigInt()) {
        if (valBase->IsBigInt() && valExponent->IsBigInt()) {
            JSHandle<BigInt> bigBaseVale(valBase);
            JSHandle<BigInt> bigExponentValue(valExponent);
            return BigInt::Exponentiate(thread, bigBaseVale, bigExponentValue).GetTaggedValue();
        }
        THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot mix BigInt and other types, use explicit conversions",
                                    JSTaggedValue::Exception());
    }
    double doubleBase = valBase->GetNumber();
    double doubleExponent = valExponent->GetNumber();
    if (std::abs(doubleBase) == 1 && std::isinf(doubleExponent)) {
        return JSTaggedValue(base::NAN_VALUE);
    }
    if (((doubleBase == 0) &&
        ((base::bit_cast<uint64_t>(doubleBase)) & base::DOUBLE_SIGN_MASK) == base::DOUBLE_SIGN_MASK) &&
        std::isfinite(doubleExponent) && base::NumberHelper::TruncateDouble(doubleExponent) == doubleExponent &&
        base::NumberHelper::TruncateDouble(doubleExponent / 2) + base::HALF == (doubleExponent / 2)) {  // 2: half
        if (doubleExponent > 0) {
            return JSTaggedValue(-0.0);
        }
        if (doubleExponent < 0) {
            return JSTaggedValue(-base::POSITIVE_INFINITY);
        }
    }
    return JSTaggedValue(std::pow(doubleBase, doubleExponent));
}

JSTaggedValue RuntimeStubs::RuntimeIsIn(JSThread *thread, const JSHandle<JSTaggedValue> &prop,
                                        const JSHandle<JSTaggedValue> &obj)
{
    if (!obj->IsECMAObject()) {
        return RuntimeThrowTypeError(thread, "Cannot use 'in' operator in Non-Object");
    }
    JSHandle<JSTaggedValue> propKey = JSTaggedValue::ToPropertyKey(thread, prop);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    bool ret = JSTaggedValue::HasProperty(thread, obj, propKey);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue(ret);
}

JSTaggedValue RuntimeStubs::RuntimeInstanceof(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                              const JSHandle<JSTaggedValue> &target)
{
    bool ret = JSObject::InstanceOf(thread, obj, target);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue(ret);
}

JSTaggedValue RuntimeStubs::RuntimeInstanceofByHandler(JSThread *thread, JSHandle<JSTaggedValue> target,
                                                       JSHandle<JSTaggedValue> object,
                                                       JSHandle<JSTaggedValue> instOfHandler)
{
    // 3. ReturnIfAbrupt(instOfHandler).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 4. If instOfHandler is not undefined, then
    if (!instOfHandler->IsUndefined()) {
        // a. Return ! ToBoolean(? Call(instOfHandler, target, «object»)).
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSTaggedValue function = env->GetTaggedHasInstanceFunction();
        JSTaggedValue instOf = instOfHandler.GetTaggedValue();
        // slowpath
        if (function != instOf) {
            JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
            EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, instOfHandler, target,
                                                                            undefined, 1);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(object.GetTaggedValue());
            JSTaggedValue tagged = JSFunction::Call(info);

            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            return tagged;
        }
    }
    // 5. If IsCallable(target) is false, throw a TypeError exception.
    if (!target->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "InstanceOf error when target is not Callable", JSTaggedValue::Exception());
    }
    // fastpath
    // 6. Return ? OrdinaryHasInstance(target, object).
    bool res = JSFunction::OrdinaryHasInstance(thread, target, object);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue(res);
}

JSTaggedValue RuntimeStubs::RuntimeCreateGeneratorObj(JSThread *thread, const JSHandle<JSTaggedValue> &genFunc)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSGeneratorObject> obj = factory->NewJSGeneratorObject(genFunc);
    JSHandle<GeneratorContext> context = factory->NewGeneratorContext();
    context->SetGeneratorObject(thread, obj.GetTaggedValue());

    // change state to SUSPENDED_START
    obj->SetGeneratorState(JSGeneratorState::SUSPENDED_START);
    obj->SetGeneratorContext(thread, context);

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return obj.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeCreateAsyncGeneratorObj(JSThread *thread, const JSHandle<JSTaggedValue> &genFunc)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAsyncGeneratorObject> obj = factory->NewJSAsyncGeneratorObject(genFunc);
    JSHandle<GeneratorContext> context = factory->NewGeneratorContext();
    context->SetGeneratorObject(thread, obj.GetTaggedValue());

    // change state to SUSPENDED_START
    obj->SetAsyncGeneratorState(JSAsyncGeneratorState::SUSPENDED_START);
    obj->SetGeneratorContext(thread, context);

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return obj.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeGetTemplateObject(JSThread *thread, const JSHandle<JSTaggedValue> &literal)
{
    JSHandle<JSTaggedValue> templateObj = TemplateString::GetTemplateObject(thread, literal);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return templateObj.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeGetNextPropName(JSThread *thread, const JSHandle<JSTaggedValue> &iter)
{
    ASSERT(iter->IsForinIterator());
    std::pair<JSTaggedValue, bool> res =
        JSForInIterator::NextInternal(thread, JSHandle<JSForInIterator>::Cast(iter));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res.first;
}

JSTaggedValue RuntimeStubs::RuntimeIterNext(JSThread *thread, const JSHandle<JSTaggedValue> &iter)
{
    JSHandle<JSTaggedValue> resultObj = JSIterator::IteratorNext(thread, iter);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return resultObj.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeCloseIterator(JSThread *thread, const JSHandle<JSTaggedValue> &iter)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    JSHandle<JSTaggedValue> record;
    if (thread->HasPendingException()) {
        record = JSHandle<JSTaggedValue>(factory->NewCompletionRecord(
            CompletionRecordType::THROW, JSHandle<JSTaggedValue>(thread, thread->GetException())));
    } else {
        JSHandle<JSTaggedValue> undefinedVal = globalConst->GetHandledUndefined();
        record = JSHandle<JSTaggedValue>(factory->NewCompletionRecord(CompletionRecordType::NORMAL, undefinedVal));
    }
    JSHandle<JSTaggedValue> result = JSIterator::IteratorClose(thread, iter, record);
    if (result->IsCompletionRecord()) {
        return CompletionRecord::Cast(result->GetTaggedObject())->GetValue();
    }
    return result.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeSuperCallSpread(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                                   const JSHandle<JSTaggedValue> &newTarget,
                                                   const JSHandle<JSTaggedValue> &array)
{
    JSHandle<JSTaggedValue> superFunc(thread, JSTaggedValue::GetPrototype(thread, func));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    ASSERT(superFunc->IsJSFunction());

    JSHandle<TaggedArray> argv(thread, RuntimeGetCallSpreadArgs(thread, array));
    const uint32_t argsLength = argv->GetLength();
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, superFunc, undefined, newTarget, argsLength);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    info->SetCallArg(argsLength, argv);
    JSTaggedValue result = JSFunction::Construct(info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return result;
}

JSTaggedValue RuntimeStubs::RuntimeDelObjProp(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                              const JSHandle<JSTaggedValue> &prop)
{
    JSHandle<JSTaggedValue> jsObj(JSTaggedValue::ToObject(thread, obj));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> propKey = JSTaggedValue::ToPropertyKey(thread, prop);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    bool ret = JSTaggedValue::DeletePropertyOrThrow(thread, jsObj, propKey);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue(ret);
}

JSTaggedValue RuntimeStubs::RuntimeNewObjApply(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                               const JSHandle<JSTaggedValue> &array)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (!array->IsJSArray()) {
        return RuntimeThrowTypeError(thread, "Cannot Newobjspread");
    }

    uint32_t length = JSHandle<JSArray>::Cast(array)->GetArrayLength();
    JSHandle<TaggedArray> argsArray = factory->NewTaggedArray(length);
    for (uint32_t i = 0; i < length; ++i) {
        auto prop = JSTaggedValue::GetProperty(thread, array, i).GetValue();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        argsArray->Set(thread, i, prop);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, func, undefined, func, length);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    info->SetCallArg(length, argsArray);
    return NewObject(info);
}

JSTaggedValue RuntimeStubs::RuntimeCreateIterResultObj(JSThread *thread, const JSHandle<JSTaggedValue> &value,
                                                       JSTaggedValue flag)
{
    ASSERT(flag.IsBoolean());
    bool done = flag.IsTrue();
    JSHandle<JSObject> iter = JSIterator::CreateIterResultObject(thread, value, done);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return iter.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeAsyncFunctionAwaitUncaught(JSThread *thread,
                                                              const JSHandle<JSTaggedValue> &asyncFuncObj,
                                                              const JSHandle<JSTaggedValue> &value)
{
    JSAsyncFunction::AsyncFunctionAwait(thread, asyncFuncObj, value);
    if (asyncFuncObj->IsAsyncGeneratorObject()) {
        JSHandle<JSObject> obj = JSTaggedValue::ToObject(thread, asyncFuncObj);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<JSAsyncGeneratorObject> generator = JSHandle<JSAsyncGeneratorObject>::Cast(obj);
        JSHandle<TaggedQueue> queue(thread, generator->GetAsyncGeneratorQueue());
        if (queue->Empty()) {
            return JSTaggedValue::Undefined();
        }
        JSHandle<AsyncGeneratorRequest> next(thread, queue->Front());
        JSHandle<PromiseCapability> completion(thread, next->GetCapability());
        JSHandle<JSPromise> promise(thread, completion->GetPromise());
        return promise.GetTaggedValue();
    }
    JSHandle<JSAsyncFuncObject> asyncFuncObjHandle(asyncFuncObj);
    JSHandle<JSPromise> promise(thread, asyncFuncObjHandle->GetPromise());

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return promise.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeAsyncFunctionResolveOrReject(JSThread *thread,
    const JSHandle<JSTaggedValue> &asyncFuncObj, const JSHandle<JSTaggedValue> &value, bool is_resolve)
{
    JSHandle<JSAsyncFuncObject> asyncFuncObjHandle(asyncFuncObj);
    JSHandle<JSPromise> promise(thread, asyncFuncObjHandle->GetPromise());

    // ActivePromise
    JSHandle<ResolvingFunctionsRecord> reactions = JSPromise::CreateResolvingFunctions(thread, promise);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> thisArg = globalConst->GetHandledUndefined();
    JSHandle<JSTaggedValue> activeFunc;
    if (is_resolve) {
        activeFunc = JSHandle<JSTaggedValue>(thread, reactions->GetResolveFunction());
    } else {
        activeFunc = JSHandle<JSTaggedValue>(thread, reactions->GetRejectFunction());
    }
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, activeFunc, thisArg, undefined, 1);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    info->SetCallArg(value.GetTaggedValue());
    [[maybe_unused]] JSTaggedValue res = JSFunction::Call(info);

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return promise.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeAsyncGeneratorResolve(JSThread *thread, JSHandle<JSTaggedValue> asyncFuncObj,
                                                         JSHandle<JSTaggedValue> value, JSTaggedValue flag)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSAsyncGeneratorObject> asyncGeneratorObjHandle(asyncFuncObj);
    JSHandle<JSTaggedValue> valueHandle(value);
    JSHandle<GeneratorContext> genContextHandle(thread, asyncGeneratorObjHandle->GetGeneratorContext());
    // save stack, should copy cur_frame, function execute over will free cur_frame
    SaveFrameToContext(thread, genContextHandle);

    ASSERT(flag.IsBoolean());
    bool done = flag.IsTrue();

    return JSAsyncGeneratorObject::AsyncGeneratorResolve(thread, asyncGeneratorObjHandle, valueHandle, done);
}

JSTaggedValue RuntimeStubs::RuntimeAsyncGeneratorReject(JSThread *thread, JSHandle<JSTaggedValue> asyncFuncObj,
                                                        JSHandle<JSTaggedValue> value)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSAsyncGeneratorObject> asyncGeneratorObjHandle(asyncFuncObj);
    JSHandle<JSTaggedValue> valueHandle(value);

    return JSAsyncGeneratorObject::AsyncGeneratorReject(thread, asyncGeneratorObjHandle, valueHandle);
}

JSTaggedValue RuntimeStubs::RuntimeCopyDataProperties(JSThread *thread, const JSHandle<JSTaggedValue> &dst,
                                                      const JSHandle<JSTaggedValue> &src)
{
    if (!src->IsNull() && !src->IsUndefined()) {
        // 2. Let from be ! ToObject(source).
        JSHandle<JSTaggedValue> from(JSTaggedValue::ToObject(thread, src));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<TaggedArray> keys = JSTaggedValue::GetOwnPropertyKeys(thread, from);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

        JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
        uint32_t keysLen = keys->GetLength();
        for (uint32_t i = 0; i < keysLen; i++) {
            PropertyDescriptor desc(thread);
            key.Update(keys->Get(i));
            bool success = JSTaggedValue::GetOwnProperty(thread, from, key, desc);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

            if (success && desc.IsEnumerable()) {
                JSTaggedValue::DefineOwnProperty(thread, dst, key, desc);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
        }
    }
    return dst.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeStArraySpread(JSThread *thread, const JSHandle<JSTaggedValue> &dst,
                                                 JSTaggedValue index, const JSHandle<JSTaggedValue> &src)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    ASSERT(dst->IsJSArray() && !src->IsNull() && !src->IsUndefined());
    if (src->IsString()) {
        JSHandle<EcmaString> srcString = JSTaggedValue::ToString(thread, src);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<EcmaString> srcFlat = JSHandle<EcmaString>(thread,
            EcmaStringAccessor::Flatten(thread->GetEcmaVM(), srcString));
        uint32_t dstLen = static_cast<uint32_t>(index.GetInt());
        uint32_t strLen = EcmaStringAccessor(srcFlat).GetLength();
        for (uint32_t i = 0; i < strLen; i++) {
            uint16_t res = EcmaStringAccessor(srcFlat).Get<false>(i);
            JSHandle<JSTaggedValue> strValue(factory->NewFromUtf16Literal(&res, 1));
            JSTaggedValue::SetProperty(thread, dst, dstLen + i, strValue, true);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        return JSTaggedValue(dstLen + strLen);
    }

    if (index.GetInt() == 0 && src->IsStableJSArray(thread)) {
        JSHandle<TaggedArray> srcElements(thread, JSHandle<JSObject>::Cast(src)->GetElements());
        uint32_t length = JSHandle<JSArray>::Cast(src)->GetArrayLength();
        JSHandle<TaggedArray> dstElements = factory->NewTaggedArray(length);
        JSHandle<JSArray> dstArray = JSHandle<JSArray>::Cast(dst);
        dstArray->SetElements(thread, dstElements);
        dstArray->SetArrayLength(thread, length);
        TaggedArray::CopyTaggedArrayElement(thread, srcElements, dstElements, length);
        return JSTaggedValue(length);
    }

    JSHandle<JSTaggedValue> iter;
    auto globalConst = thread->GlobalConstants();
    if (src->IsJSArrayIterator() || src->IsJSMapIterator() || src->IsJSSetIterator() ||
        src->IsIterator()) {
        iter = src;
    } else if (src->IsJSArray()) {
        JSHandle<JSTaggedValue> valuesStr = globalConst->GetHandledValuesString();
        JSHandle<JSTaggedValue> valuesMethod = JSObject::GetMethod(thread, src, valuesStr);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        iter = JSIterator::GetIterator(thread, src, valuesMethod);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    } else {
        iter = JSIterator::GetIterator(thread, src);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }

    JSMutableHandle<JSTaggedValue> indexHandle(thread, index);
    JSHandle<JSTaggedValue> valueStr = globalConst->GetHandledValueString();
    PropertyDescriptor desc(thread);
    JSHandle<JSTaggedValue> iterResult;
    do {
        iterResult = JSIterator::IteratorStep(thread, iter);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (iterResult->IsFalse()) {
            break;
        }
        bool success = JSTaggedValue::GetOwnProperty(thread, iterResult, valueStr, desc);
        if (success && desc.IsEnumerable()) {
            JSTaggedValue::DefineOwnProperty(thread, dst, indexHandle, desc);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            int tmp = indexHandle->GetInt();
            indexHandle.Update(JSTaggedValue(tmp + 1));
        }
    } while (true);

    return indexHandle.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeGetIteratorNext(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                   const JSHandle<JSTaggedValue> &method)
{
    ASSERT(method->IsCallable());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, method, obj, undefined, 0);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedValue ret = JSFunction::Call(info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!ret.IsECMAObject()) {
        return RuntimeThrowTypeError(thread, "the Iterator is not an ecmaobject.");
    }
    return ret;
}

JSTaggedValue RuntimeStubs::RuntimeSetObjectWithProto(JSThread *thread, const JSHandle<JSTaggedValue> &proto,
                                                      const JSHandle<JSObject> &obj)
{
    if (!proto->IsECMAObject() && !proto->IsNull()) {
        return JSTaggedValue::False();
    }
    JSObject::SetPrototype(thread, obj, proto);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeLdObjByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                const JSHandle<JSTaggedValue> &prop, bool callGetter,
                                                JSTaggedValue receiver)
{
    // Ecma Spec 2015 12.3.2.1
    // 7. Let bv be RequireObjectCoercible(baseValue).
    // 8. ReturnIfAbrupt(bv).
    JSHandle<JSTaggedValue> object =
        JSTaggedValue::RequireObjectCoercible(thread, obj, "Cannot load property of null or undefined");
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSTaggedValue res;
    if (callGetter) {
        res = JSObject::CallGetter(thread, AccessorData::Cast(receiver.GetTaggedObject()), object);
    } else {
        JSHandle<JSTaggedValue> propKey = JSTaggedValue::ToPropertyKey(thread, prop);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        res = JSTaggedValue::GetProperty(thread, object, propKey).GetValue().GetTaggedValue();
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res;
}

JSTaggedValue RuntimeStubs::RuntimeStObjByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                const JSHandle<JSTaggedValue> &prop,
                                                const JSHandle<JSTaggedValue> &value)
{
    // Ecma Spec 2015 12.3.2.1
    // 7. Let bv be RequireObjectCoercible(baseValue).
    // 8. ReturnIfAbrupt(bv).
    JSHandle<JSTaggedValue> object =
        JSTaggedValue::RequireObjectCoercible(thread, obj, "Cannot store property of null or undefined");
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSTaggedValue> propKey(JSTaggedValue::ToPropertyKey(thread, prop));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // strict mode is true
    JSTaggedValue::SetProperty(thread, object, propKey, value, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeStOwnByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                const JSHandle<JSTaggedValue> &key,
                                                const JSHandle<JSTaggedValue> &value)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    if (obj->IsClassConstructor() &&
        JSTaggedValue::SameValue(key, globalConst->GetHandledPrototypeString())) {
        return RuntimeThrowTypeError(thread, "In a class, static property named 'prototype' throw a TypeError");
    }

    // property in class is non-enumerable
    bool enumerable = !(obj->IsClassPrototype() || obj->IsClassConstructor());

    PropertyDescriptor desc(thread, value, true, enumerable, true);
    JSHandle<JSTaggedValue> propKey = JSTaggedValue::ToPropertyKey(thread, key);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    bool ret = JSTaggedValue::DefineOwnProperty(thread, obj, propKey, desc);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!ret) {
        return RuntimeThrowTypeError(thread, "StOwnByValue failed");
    }
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeLdSuperByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                  const JSHandle<JSTaggedValue> &key, JSTaggedValue thisFunc)
{
    ASSERT(thisFunc.IsJSFunction());
    // get Homeobject form function
    JSHandle<JSTaggedValue> homeObject(thread, JSFunction::Cast(thisFunc.GetTaggedObject())->GetHomeObject());

    if (obj->IsUndefined()) {
        return RuntimeThrowReferenceError(thread, obj, "this is uninitialized.");
    }

    JSHandle<JSTaggedValue> propKey(JSTaggedValue::ToPropertyKey(thread, key));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> superBase(thread, JSTaggedValue::GetSuperBase(thread, homeObject));
    JSTaggedValue::RequireObjectCoercible(thread, superBase);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSTaggedValue res = JSTaggedValue::GetProperty(thread, superBase, propKey, obj).GetValue().GetTaggedValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res;
}

JSTaggedValue RuntimeStubs::RuntimeStSuperByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                  const JSHandle<JSTaggedValue> &key,
                                                  const JSHandle<JSTaggedValue> &value, JSTaggedValue thisFunc)
{
    ASSERT(thisFunc.IsJSFunction());
    // get Homeobject form function
    JSHandle<JSTaggedValue> homeObject(thread, JSFunction::Cast(thisFunc.GetTaggedObject())->GetHomeObject());

    if (obj->IsUndefined()) {
        return RuntimeThrowReferenceError(thread, obj, "this is uninitialized.");
    }

    JSHandle<JSTaggedValue> propKey(JSTaggedValue::ToPropertyKey(thread, key));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> superBase(thread, JSTaggedValue::GetSuperBase(thread, homeObject));
    JSTaggedValue::RequireObjectCoercible(thread, superBase);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // check may_throw is false?
    JSTaggedValue::SetProperty(thread, superBase, propKey, value, obj, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeLdObjByIndex(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                uint32_t idx, bool callGetter, JSTaggedValue receiver)
{
    JSTaggedValue res;
    if (callGetter) {
        res = JSObject::CallGetter(thread, AccessorData::Cast(receiver.GetTaggedObject()), obj);
    } else {
        res = JSTaggedValue::GetProperty(thread, obj, idx).GetValue().GetTaggedValue();
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res;
}

JSTaggedValue RuntimeStubs::RuntimeLdObjByName(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop,
                                               bool callGetter, JSTaggedValue receiver)
{
    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSTaggedValue res;
    if (callGetter) {
        res = JSObject::CallGetter(thread, AccessorData::Cast(receiver.GetTaggedObject()), objHandle);
    } else {
        JSHandle<JSTaggedValue> propHandle(thread, prop);
        res = JSTaggedValue::GetProperty(thread, objHandle, propHandle).GetValue().GetTaggedValue();
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res;
}

JSTaggedValue RuntimeStubs::RuntimeStObjByName(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                               const JSHandle<JSTaggedValue> &prop,
                                               const JSHandle<JSTaggedValue> &value)
{
    JSTaggedValue::SetProperty(thread, obj, prop, value, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeStObjByIndex(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                uint32_t idx, const JSHandle<JSTaggedValue> &value)
{
    JSTaggedValue::SetProperty(thread, obj, idx, value, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeStOwnByIndex(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                const JSHandle<JSTaggedValue> &idx,
                                                const JSHandle<JSTaggedValue> &value)
{
    // property in class is non-enumerable
    bool enumerable = !(obj->IsClassPrototype() || obj->IsClassConstructor());

    PropertyDescriptor desc(thread, value, true, enumerable, true);
    bool ret = JSTaggedValue::DefineOwnProperty(thread, obj, idx, desc);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!ret) {
        return RuntimeThrowTypeError(thread, "SetOwnByIndex failed");
    }
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeStGlobalRecord(JSThread *thread, const JSHandle<JSTaggedValue> &prop,
                                                  const JSHandle<JSTaggedValue> &value, bool isConst)
{
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    GlobalDictionary *dict = GlobalDictionary::Cast(env->GetGlobalRecord()->GetTaggedObject());

    // cross files global record name binding judgment
    int entry = dict->FindEntry(prop.GetTaggedValue());
    if (entry != -1) {
        return RuntimeThrowSyntaxError(thread, "Duplicate identifier");
    }

    PropertyAttributes attributes;
    if (isConst) {
        attributes.SetIsConstProps(true);
    }
    JSHandle<GlobalDictionary> dictHandle(thread, dict);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<PropertyBox> box = factory->NewPropertyBox(value);
    PropertyBoxType boxType = value->IsUndefined() ? PropertyBoxType::UNDEFINED : PropertyBoxType::CONSTANT;
    attributes.SetBoxType(boxType);

    dict = *GlobalDictionary::PutIfAbsent(thread, dictHandle, prop, JSHandle<JSTaggedValue>(box), attributes);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    env->SetGlobalRecord(thread, JSTaggedValue(dict));
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeNeg(JSThread *thread, const JSHandle<JSTaggedValue> &value)
{
    JSHandle<JSTaggedValue> inputVal = JSTaggedValue::ToNumeric(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (inputVal->IsBigInt()) {
        JSHandle<BigInt> bigValue(inputVal);
        return BigInt::UnaryMinus(thread, bigValue).GetTaggedValue();
    }
    JSTaggedNumber number(inputVal.GetTaggedValue());
    if (number.IsInt()) {
        int32_t intValue = number.GetInt();
        if (intValue == 0) {
            return JSTaggedValue(-0.0);
        }
        if (intValue == INT32_MIN) {
            return JSTaggedValue(-static_cast<double>(INT32_MIN));
        }
        return JSTaggedValue(-intValue);
    }
    if (number.IsDouble()) {
        return JSTaggedValue(-number.GetDouble());
    }
    LOG_ECMA(FATAL) << "this branch is unreachable";
    UNREACHABLE();
}

JSTaggedValue RuntimeStubs::RuntimeNot(JSThread *thread, const JSHandle<JSTaggedValue> &value)
{
    JSHandle<JSTaggedValue> inputVal = JSTaggedValue::ToNumeric(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (inputVal->IsBigInt()) {
        JSHandle<BigInt> bigValue(inputVal);
        return BigInt::BitwiseNOT(thread, bigValue).GetTaggedValue();
    }
    int32_t number = JSTaggedValue::ToInt32(thread, inputVal);
    return JSTaggedValue(~number); // NOLINT(hicpp-signed-bitwise)
}

JSTaggedValue RuntimeStubs::RuntimeResolveClass(JSThread *thread, const JSHandle<JSFunction> &ctor,
                                                const JSHandle<TaggedArray> &literal,
                                                const JSHandle<JSTaggedValue> &base,
                                                const JSHandle<JSTaggedValue> &lexenv)
{
    ASSERT(ctor.GetTaggedValue().IsClassConstructor());

    FrameHandler frameHandler(thread);
    JSTaggedValue currentFunc = frameHandler.GetFunction();
    JSHandle<JSTaggedValue> ecmaModule(thread, JSFunction::Cast(currentFunc.GetTaggedObject())->GetModule());

    RuntimeSetClassInheritanceRelationship(thread, JSHandle<JSTaggedValue>(ctor), base);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    uint32_t literalBufferLength = literal->GetLength();

    // only traverse the value of key-value pair
    for (uint32_t index = 1; index < literalBufferLength - 1; index += 2) {  // 2: key-value pair
        JSTaggedValue value = literal->Get(index);
        if (LIKELY(value.IsJSFunction())) {
            JSFunction *func = JSFunction::Cast(value.GetTaggedObject());
            func->SetLexicalEnv(thread, lexenv.GetTaggedValue());
            func->SetModule(thread, ecmaModule);
        }
    }

    return ctor.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeCloneClassFromTemplate(JSThread *thread, const JSHandle<JSFunction> &ctor,
                                                          const JSHandle<JSTaggedValue> &base,
                                                          const JSHandle<JSTaggedValue> &lexenv)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    ASSERT(ctor.GetTaggedValue().IsClassConstructor());
    JSHandle<JSObject> clsPrototype(thread, ctor->GetFunctionPrototype());

    bool canShareHClass = false;
    JSHandle<JSFunction> cloneClass = factory->CloneClassCtor(ctor, lexenv, canShareHClass);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSObject> cloneClassPrototype = factory->CloneObjectLiteral(JSHandle<JSObject>(clsPrototype), lexenv,
                                                                         canShareHClass);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // After clone both, reset "constructor" and "prototype" properties.
    cloneClass->SetFunctionPrototype(thread, cloneClassPrototype.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    PropertyDescriptor ctorDesc(thread, JSHandle<JSTaggedValue>(cloneClass), true, false, true);
    JSTaggedValue::DefinePropertyOrThrow(thread, JSHandle<JSTaggedValue>(cloneClassPrototype),
                                         globalConst->GetHandledConstructorString(), ctorDesc);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    cloneClass->SetHomeObject(thread, cloneClassPrototype);

    if (!canShareHClass) {
        RuntimeSetClassInheritanceRelationship(thread, JSHandle<JSTaggedValue>(cloneClass), base);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }

    return cloneClass.GetTaggedValue();
}

// clone class may need re-set inheritance relationship due to extends may be a variable.
JSTaggedValue RuntimeStubs::RuntimeCreateClassWithBuffer(JSThread *thread,
                                                         const JSHandle<JSTaggedValue> &base,
                                                         const JSHandle<JSTaggedValue> &lexenv,
                                                         const JSHandle<JSTaggedValue> &constpool,
                                                         uint16_t methodId, uint16_t literalId,
                                                         const JSHandle<JSTaggedValue> &module)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    CString entry = SourceTextModule::GetRecordName(module.GetTaggedValue());

    // For class constructor.
    auto methodObj = ConstantPool::GetMethodFromCache(thread, constpool.GetTaggedValue(), methodId);
    JSHandle<JSTaggedValue> method(thread, methodObj);
    JSHandle<ConstantPool> constpoolHandle = JSHandle<ConstantPool>::Cast(constpool);
    JSHandle<JSFunction> cls;
    JSMutableHandle<JSTaggedValue> ihc(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> chc(thread, JSTaggedValue::Undefined());

    JSTaggedValue val = constpoolHandle->GetObjectFromCache(literalId);
    if (val.IsAOTLiteralInfo()) {
        JSHandle<AOTLiteralInfo> aotLiteralInfo(thread, val);
        ihc.Update(aotLiteralInfo->GetIhc());
        chc.Update(aotLiteralInfo->GetChc());
    }
    auto literalObj = ConstantPool::GetClassLiteralFromCache(thread, constpoolHandle, literalId, entry);
    JSHandle<ClassLiteral> classLiteral(thread, literalObj);
    JSHandle<TaggedArray> arrayHandle(thread, classLiteral->GetArray());
    JSHandle<ClassInfoExtractor> extractor = factory->NewClassInfoExtractor(method);
    ClassInfoExtractor::BuildClassInfoExtractorFromLiteral(thread, extractor, arrayHandle);

    if ((ihc->IsUndefined() && chc->IsUndefined()) ||
        (classLiteral->GetIsAOTUsed())) {
        cls = ClassHelper::DefineClassFromExtractor(thread, base, extractor, lexenv);
    } else {
        classLiteral->SetIsAOTUsed(true);
        JSHandle<JSHClass> ihclass(ihc);
        JSHandle<JSHClass> chclass(chc);
        cls = ClassHelper::DefineClassWithIHClass(thread, extractor,
                                                  lexenv, ihclass, chclass);
    }

    RuntimeSetClassInheritanceRelationship(thread, JSHandle<JSTaggedValue>(cls), base);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return cls.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeSetClassInheritanceRelationship(JSThread *thread,
                                                                   const JSHandle<JSTaggedValue> &ctor,
                                                                   const JSHandle<JSTaggedValue> &base)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    ASSERT(ctor->IsJSFunction());
    JSHandle<JSTaggedValue> parent = base;

    /*
     *         class A / class A extends null                             class A extends B
     *                                       a                                                 a
     *                                       |                                                 |
     *                                       |  __proto__                                      |  __proto__
     *                                       |                                                 |
     *       A            ---->         A.prototype                  A             ---->    A.prototype
     *       |                               |                       |                         |
     *       |  __proto__                    |  __proto__            |  __proto__              |  __proto__
     *       |                               |                       |                         |
     *   Function.prototype       Object.prototype / null            B             ---->    B.prototype
     */

    JSHandle<JSTaggedValue> parentPrototype;
    // hole means parent is not present
    Method *method = Method::Cast(JSHandle<JSFunction>::Cast(ctor)->GetMethod().GetTaggedObject());
    if (parent->IsHole()) {
        method->SetFunctionKind(FunctionKind::CLASS_CONSTRUCTOR);
        parentPrototype = env->GetObjectFunctionPrototype();
        parent = env->GetFunctionPrototype();
    } else if (parent->IsNull()) {
        method->SetFunctionKind(FunctionKind::DERIVED_CONSTRUCTOR);
        parentPrototype = JSHandle<JSTaggedValue>(thread, JSTaggedValue::Null());
        parent = env->GetFunctionPrototype();
    } else if (!parent->IsConstructor()) {
        return RuntimeThrowTypeError(thread, "parent class is not constructor");
    } else {
        method->SetFunctionKind(FunctionKind::DERIVED_CONSTRUCTOR);
        parentPrototype = JSTaggedValue::GetProperty(thread, parent,
            globalConst->GetHandledPrototypeString()).GetValue();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (!parentPrototype->IsECMAObject() && !parentPrototype->IsNull()) {
            return RuntimeThrowTypeError(thread, "parent class have no valid prototype");
        }
    }

    ctor->GetTaggedObject()->GetClass()->SetPrototype(thread, parent);

    JSHandle<JSObject> clsPrototype(thread, JSHandle<JSFunction>(ctor)->GetFunctionPrototype());
    clsPrototype->GetClass()->SetPrototype(thread, parentPrototype);

    // by enableing the ProtoChangeMarker, the IHC generated in the Aot stage
    // is registered into the listener of its prototype. In this way, it is ensured
    // that when the prototype changes, the affected IHC can be notified.
    JSTaggedValue protoOrHClass = JSHandle<JSFunction>(ctor)->GetProtoOrHClass();
    if (protoOrHClass.IsJSHClass()) {
        JSHClass *ihc = JSHClass::Cast(protoOrHClass.GetTaggedObject());
        if (ihc->IsTS()) {
            JSHandle<JSHClass> ihcHandle(thread, ihc);
            JSHClass::EnableProtoChangeMarker(thread, ihcHandle);
        }
    }

    return JSTaggedValue::Undefined();
}

JSTaggedValue RuntimeStubs::RuntimeSetClassConstructorLength(JSThread *thread, JSTaggedValue ctor,
                                                             JSTaggedValue length)
{
    ASSERT(ctor.IsClassConstructor());

    JSFunction* cls = JSFunction::Cast(ctor.GetTaggedObject());
    if (LIKELY(!cls->GetClass()->IsDictionaryMode())) {
        cls->SetPropertyInlinedProps(thread, JSFunction::LENGTH_INLINE_PROPERTY_INDEX, length);
    } else {
        const GlobalEnvConstants *globalConst = thread->GlobalConstants();
        cls->UpdatePropertyInDictionary(thread, globalConst->GetLengthString(), length);
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue RuntimeStubs::RuntimeNotifyInlineCache(JSThread *thread, const JSHandle<Method> &method,
                                                     uint32_t icSlotSize)
{
    if (icSlotSize == 0) {
        return JSTaggedValue::Undefined();
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ProfileTypeInfo> profileTypeInfo = factory->NewProfileTypeInfo(icSlotSize);
    // overflow 8bit
    if (icSlotSize > ProfileTypeInfo::INVALID_SLOT_INDEX) {
        // set as mega
        profileTypeInfo->Set(thread, ProfileTypeInfo::INVALID_SLOT_INDEX, JSTaggedValue::Hole());
        ASSERT(icSlotSize <= ProfileTypeInfo::MAX_SLOT_INDEX + 1);
    }
    method->SetProfileTypeInfo(thread, profileTypeInfo.GetTaggedValue());
    return profileTypeInfo.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeStOwnByValueWithNameSet(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                           const JSHandle<JSTaggedValue> &key,
                                                           const JSHandle<JSTaggedValue> &value)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    if (obj->IsClassConstructor() &&
        JSTaggedValue::SameValue(key, globalConst->GetHandledPrototypeString())) {
        return RuntimeThrowTypeError(thread, "In a class, static property named 'prototype' throw a TypeError");
    }

    // property in class is non-enumerable
    bool enumerable = !(obj->IsClassPrototype() || obj->IsClassConstructor());

    PropertyDescriptor desc(thread, value, true, enumerable, true);
    JSHandle<JSTaggedValue> propKey = JSTaggedValue::ToPropertyKey(thread, key);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    bool ret = JSTaggedValue::DefineOwnProperty(thread, obj, propKey, desc);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!ret) {
        return RuntimeThrowTypeError(thread, "StOwnByValueWithNameSet failed");
    }
    if (value->IsJSFunction()) {
        if (propKey->IsNumber()) {
            propKey = JSHandle<JSTaggedValue>(base::NumberHelper::NumberToString(thread, propKey.GetTaggedValue()));
        }
        JSFunctionBase::SetFunctionName(thread, JSHandle<JSFunctionBase>::Cast(value), propKey,
                                        JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    }
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeStOwnByName(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                               const JSHandle<JSTaggedValue> &prop,
                                               const JSHandle<JSTaggedValue> &value)
{
    ASSERT(prop->IsStringOrSymbol());

    // property in class is non-enumerable
    bool enumerable = !(obj->IsClassPrototype() || obj->IsClassConstructor());

    PropertyDescriptor desc(thread, value, true, enumerable, true);
    bool ret = JSTaggedValue::DefineOwnProperty(thread, obj, prop, desc);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!ret) {
        return RuntimeThrowTypeError(thread, "SetOwnByName failed");
    }
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeStOwnByNameWithNameSet(JSThread *thread,
                                                          const JSHandle<JSTaggedValue> &objHandle,
                                                          const JSHandle<JSTaggedValue> &propHandle,
                                                          const JSHandle<JSTaggedValue> &valueHandle)
{
    ASSERT(propHandle->IsStringOrSymbol());
    JSHandle<JSTaggedValue> propKey = JSTaggedValue::ToPropertyKey(thread, propHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // property in class is non-enumerable
    bool enumerable = !(objHandle->IsClassPrototype() || objHandle->IsClassConstructor());

    PropertyDescriptor desc(thread, valueHandle, true, enumerable, true);
    bool ret = JSTaggedValue::DefineOwnProperty(thread, objHandle, propHandle, desc);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!ret) {
        return RuntimeThrowTypeError(thread, "SetOwnByNameWithNameSet failed");
    }
    JSFunctionBase::SetFunctionName(thread, JSHandle<JSFunctionBase>::Cast(valueHandle), propKey,
                                    JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeSuspendGenerator(JSThread *thread, const JSHandle<JSTaggedValue> &genObj,
                                                    const JSHandle<JSTaggedValue> &value)
{
    if (genObj->IsAsyncGeneratorObject()) {
        JSHandle<JSAsyncGeneratorObject> generatorObjectHandle(genObj);
        JSHandle<GeneratorContext> genContextHandle(thread, generatorObjectHandle->GetGeneratorContext());
        // save stack, should copy cur_frame, function execute over will free cur_frame
        SaveFrameToContext(thread, genContextHandle);

        // change state to SuspendedYield
        if (generatorObjectHandle->IsExecuting()) {
            return value.GetTaggedValue();
        }
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return generatorObjectHandle.GetTaggedValue();
    }

    if (genObj->IsGeneratorObject()) {
        JSHandle<JSGeneratorObject> generatorObjectHandle(genObj);
        JSHandle<GeneratorContext> genContextHandle(thread, generatorObjectHandle->GetGeneratorContext());
        // save stack, should copy cur_frame, function execute over will free cur_frame
        SaveFrameToContext(thread, genContextHandle);

        // change state to SuspendedYield
        if (generatorObjectHandle->IsExecuting()) {
            generatorObjectHandle->SetGeneratorState(JSGeneratorState::SUSPENDED_YIELD);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            return value.GetTaggedValue();
        }
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return generatorObjectHandle.GetTaggedValue();
    }

    return RuntimeThrowTypeError(thread, "RuntimeSuspendGenerator failed");
}

void RuntimeStubs::RuntimeSetGeneratorState(JSThread *thread, const JSHandle<JSTaggedValue> &genObj,
                                            const int32_t index)
{
    JSHandle<JSAsyncGeneratorObject> generatorObjectHandle(genObj);
    JSHandle<GeneratorContext> genContextHandle(thread, generatorObjectHandle->GetGeneratorContext());

    // change state
    switch (index) {
        case static_cast<int32_t>(JSAsyncGeneratorState::SUSPENDED_START):
            generatorObjectHandle->SetAsyncGeneratorState(JSAsyncGeneratorState::SUSPENDED_START);
            break;
        case static_cast<int32_t>(JSAsyncGeneratorState::SUSPENDED_YIELD):
            generatorObjectHandle->SetAsyncGeneratorState(JSAsyncGeneratorState::SUSPENDED_YIELD);
            break;
        case static_cast<int32_t>(JSAsyncGeneratorState::EXECUTING):
            generatorObjectHandle->SetAsyncGeneratorState(JSAsyncGeneratorState::EXECUTING);
            break;
        case static_cast<int32_t>(JSAsyncGeneratorState::COMPLETED):
            generatorObjectHandle->SetAsyncGeneratorState(JSAsyncGeneratorState::COMPLETED);
            break;
        case static_cast<int32_t>(JSAsyncGeneratorState::AWAITING_RETURN):
            generatorObjectHandle->SetAsyncGeneratorState(JSAsyncGeneratorState::AWAITING_RETURN);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

JSTaggedValue RuntimeStubs::RuntimeGetModuleNamespace(JSThread *thread, int32_t index)
{
    return thread->GetCurrentEcmaContext()->GetModuleManager()->GetModuleNamespace(index);
}

JSTaggedValue RuntimeStubs::RuntimeGetModuleNamespace(JSThread *thread, int32_t index,
                                                      JSTaggedValue jsFunc)
{
    return thread->GetCurrentEcmaContext()->GetModuleManager()->GetModuleNamespace(index, jsFunc);
}

JSTaggedValue RuntimeStubs::RuntimeGetModuleNamespace(JSThread *thread, JSTaggedValue localName)
{
    return thread->GetCurrentEcmaContext()->GetModuleManager()->GetModuleNamespace(localName);
}

JSTaggedValue RuntimeStubs::RuntimeGetModuleNamespace(JSThread *thread, JSTaggedValue localName,
                                                      JSTaggedValue jsFunc)
{
    return thread->GetCurrentEcmaContext()->GetModuleManager()->GetModuleNamespace(localName, jsFunc);
}

void RuntimeStubs::RuntimeStModuleVar(JSThread *thread, int32_t index, JSTaggedValue value)
{
    thread->GetCurrentEcmaContext()->GetModuleManager()->StoreModuleValue(index, value);
}

void RuntimeStubs::RuntimeStModuleVar(JSThread *thread, int32_t index, JSTaggedValue value,
                                      JSTaggedValue jsFunc)
{
    thread->GetCurrentEcmaContext()->GetModuleManager()->StoreModuleValue(index, value, jsFunc);
}

void RuntimeStubs::RuntimeStModuleVar(JSThread *thread, JSTaggedValue key, JSTaggedValue value)
{
    thread->GetCurrentEcmaContext()->GetModuleManager()->StoreModuleValue(key, value);
}

void RuntimeStubs::RuntimeStModuleVar(JSThread *thread, JSTaggedValue key, JSTaggedValue value,
                                      JSTaggedValue jsFunc)
{
    thread->GetCurrentEcmaContext()->GetModuleManager()->StoreModuleValue(key, value, jsFunc);
}

JSTaggedValue RuntimeStubs::RuntimeLdLocalModuleVar(JSThread *thread, int32_t index)
{
    return thread->GetCurrentEcmaContext()->GetModuleManager()->GetModuleValueInner(index);
}

JSTaggedValue RuntimeStubs::RuntimeLdLocalModuleVar(JSThread *thread, int32_t index, JSTaggedValue jsFunc)
{
    return thread->GetCurrentEcmaContext()->GetModuleManager()->GetModuleValueInner(index, jsFunc);
}

JSTaggedValue RuntimeStubs::RuntimeLdExternalModuleVar(JSThread *thread, int32_t index)
{
    return thread->GetCurrentEcmaContext()->GetModuleManager()->GetModuleValueOutter(index);
}

JSTaggedValue RuntimeStubs::RuntimeLdExternalModuleVar(JSThread *thread, int32_t index, JSTaggedValue jsFunc)
{
    return thread->GetCurrentEcmaContext()->GetModuleManager()->GetModuleValueOutter(index, jsFunc);
}

JSTaggedValue RuntimeStubs::RuntimeLdModuleVar(JSThread *thread, JSTaggedValue key, bool inner)
{
    if (inner) {
        JSTaggedValue moduleValue = thread->GetCurrentEcmaContext()->GetModuleManager()->GetModuleValueInner(key);
        return moduleValue;
    }

    return thread->GetCurrentEcmaContext()->GetModuleManager()->GetModuleValueOutter(key);
}

JSTaggedValue RuntimeStubs::RuntimeLdModuleVar(JSThread *thread, JSTaggedValue key, bool inner,
                                               JSTaggedValue jsFunc)
{
    if (inner) {
        JSTaggedValue moduleValue =
            thread->GetCurrentEcmaContext()->GetModuleManager()->GetModuleValueInner(key, jsFunc);
        return moduleValue;
    }

    return thread->GetCurrentEcmaContext()->GetModuleManager()->GetModuleValueOutter(key, jsFunc);
}

JSTaggedValue RuntimeStubs::RuntimeGetPropIterator(JSThread *thread, const JSHandle<JSTaggedValue> &value)
{
    JSHandle<JSForInIterator> iteratorHandle = JSObject::EnumerateObjectProperties(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return iteratorHandle.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeAsyncFunctionEnter(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. create promise
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> promiseFunc(globalEnv->GetPromiseFunction());

    JSHandle<JSPromise> promiseObject(factory->NewJSObjectByConstructor(promiseFunc));
    promiseObject->SetPromiseState(PromiseState::PENDING);
    // 2. create asyncfuncobj
    JSHandle<JSAsyncFuncObject> asyncFuncObj = factory->NewJSAsyncFuncObject();
    asyncFuncObj->SetPromise(thread, promiseObject);

    JSHandle<GeneratorContext> context = factory->NewGeneratorContext();
    context->SetGeneratorObject(thread, asyncFuncObj);

    // change state to EXECUTING
    asyncFuncObj->SetGeneratorState(JSGeneratorState::EXECUTING);
    asyncFuncObj->SetGeneratorContext(thread, context);

    // 3. return asyncfuncobj
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return asyncFuncObj.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeGetIterator(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> valuesFunc =
        JSTaggedValue::GetProperty(thread, obj, env->GetIteratorSymbol()).GetValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!valuesFunc->IsCallable()) {
        return valuesFunc.GetTaggedValue();
    }

    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, valuesFunc, obj, undefined, 0);
    return EcmaInterpreter::Execute(info);
}

JSTaggedValue RuntimeStubs::RuntimeGetAsyncIterator(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    JSHandle<JSTaggedValue> asyncit = JSIterator::GetAsyncIterator(thread, obj);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return asyncit.GetTaggedValue();
}

void RuntimeStubs::RuntimeThrow(JSThread *thread, JSTaggedValue value)
{
    thread->SetException(value);
}

void RuntimeStubs::RuntimeThrowThrowNotExists(JSThread *thread)
{
    THROW_TYPE_ERROR(thread, "Throw method is not defined");
}

void RuntimeStubs::RuntimeThrowPatternNonCoercible(JSThread *thread)
{
    JSHandle<EcmaString> msg(thread->GlobalConstants()->GetHandledObjNotCoercibleString());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    THROW_NEW_ERROR_AND_RETURN(thread, factory->NewJSError(base::ErrorType::TYPE_ERROR, msg).GetTaggedValue());
}

void RuntimeStubs::RuntimeThrowDeleteSuperProperty(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> info = factory->NewFromASCII("Can not delete super property");
    JSHandle<JSObject> errorObj = factory->NewJSError(base::ErrorType::REFERENCE_ERROR, info);
    THROW_NEW_ERROR_AND_RETURN(thread, errorObj.GetTaggedValue());
}

void RuntimeStubs::RuntimeThrowUndefinedIfHole(JSThread *thread, const JSHandle<EcmaString> &obj)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> info = factory->NewFromASCII(" is not initialized");

    JSHandle<EcmaString> msg = factory->ConcatFromString(obj, info);
    THROW_NEW_ERROR_AND_RETURN(thread, factory->NewJSError(base::ErrorType::REFERENCE_ERROR, msg).GetTaggedValue());
}

void RuntimeStubs::RuntimeThrowIfNotObject(JSThread *thread)
{
    THROW_TYPE_ERROR(thread, "Inner return result is not object");
}

void RuntimeStubs::RuntimeThrowConstAssignment(JSThread *thread, const JSHandle<EcmaString> &value)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<EcmaString> info = factory->NewFromASCII("Assignment to const variable ");

    JSHandle<EcmaString> msg = factory->ConcatFromString(info, value);
    THROW_NEW_ERROR_AND_RETURN(thread, factory->NewJSError(base::ErrorType::TYPE_ERROR, msg).GetTaggedValue());
}

JSTaggedValue RuntimeStubs::RuntimeLdGlobalRecord(JSThread *thread, JSTaggedValue key)
{
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    GlobalDictionary *dict = GlobalDictionary::Cast(env->GetGlobalRecord()->GetTaggedObject());
    int entry = dict->FindEntry(key);
    if (entry != -1) {
        return JSTaggedValue(dict->GetBox(entry));
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue RuntimeStubs::RuntimeTryLdGlobalByName(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                     const JSHandle<JSTaggedValue> &prop)
{
    OperationResult res = JSTaggedValue::GetProperty(thread, obj, prop);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!res.GetPropertyMetaData().IsFound()) {
        return RuntimeThrowReferenceError(thread, prop, " is not defined");
    }
    return res.GetValue().GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeTryUpdateGlobalRecord(JSThread *thread, JSTaggedValue prop,
                                                         JSTaggedValue value)
{
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    GlobalDictionary *dict = GlobalDictionary::Cast(env->GetGlobalRecord()->GetTaggedObject());
    int entry = dict->FindEntry(prop);
    ASSERT(entry != -1);

    if (dict->GetAttributes(entry).IsConstProps()) {
        return RuntimeThrowTypeError(thread, "const variable can not be modified");
    }

    PropertyBox *box = dict->GetBox(entry);
    box->SetValue(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeThrowReferenceError(JSThread *thread, const JSHandle<JSTaggedValue> &prop,
                                                       const char *desc)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> propName = JSTaggedValue::ToString(thread, prop);
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> info = factory->NewFromUtf8(desc);
    JSHandle<EcmaString> msg = factory->ConcatFromString(propName, info);
    THROW_NEW_ERROR_AND_RETURN_VALUE(thread,
                                     factory->NewJSError(base::ErrorType::REFERENCE_ERROR, msg).GetTaggedValue(),
                                     JSTaggedValue::Exception());
}

JSTaggedValue RuntimeStubs::RuntimeLdGlobalVarFromProto(JSThread *thread, const JSHandle<JSTaggedValue> &globalObj,
                                                        const JSHandle<JSTaggedValue> &prop)
{
    ASSERT(globalObj->IsJSGlobalObject());
    JSHandle<JSObject> global(globalObj);
    JSHandle<JSTaggedValue> obj(thread, JSObject::GetPrototype(global));
    OperationResult res = JSTaggedValue::GetProperty(thread, obj, prop);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res.GetValue().GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeStGlobalVar(JSThread *thread, const JSHandle<JSTaggedValue> &prop,
                                               const JSHandle<JSTaggedValue> &value)
{
    JSHandle<JSTaggedValue> global(thread, thread->GetEcmaVM()->GetGlobalEnv()->GetGlobalObject());

    JSObject::GlobalSetProperty(thread, prop, value, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeToNumber(JSThread *thread, const JSHandle<JSTaggedValue> &value)
{
    return JSTaggedValue::ToNumber(thread, value);
}

JSTaggedValue RuntimeStubs::RuntimeToNumeric(JSThread *thread, const JSHandle<JSTaggedValue> &value)
{
    return JSTaggedValue::ToNumeric(thread, value).GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeDynamicImport(JSThread *thread, const JSHandle<JSTaggedValue> &specifier,
    const JSHandle<JSTaggedValue> &func)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    ObjectFactory *factory = ecmaVm->GetFactory();

    // get current filename
    Method *method = JSFunction::Cast(func->GetTaggedObject())->GetCallTarget();
    const JSPandaFile *jsPandaFile = method->GetJSPandaFile();
    CString currentfilename = jsPandaFile->GetJSPandaFileDesc();

    JSMutableHandle<JSTaggedValue> dirPath(thread, thread->GlobalConstants()->GetUndefined());
    JSMutableHandle<JSTaggedValue> recordName(thread, thread->GlobalConstants()->GetUndefined());
    if (!jsPandaFile->IsMergedPF()) {
        dirPath.Update(factory->NewFromUtf8(currentfilename).GetTaggedValue());
    } else {
        JSFunction *function = JSFunction::Cast(func.GetTaggedValue().GetTaggedObject());
        recordName.Update(function->GetRecordName());
        dirPath.Update(factory->NewFromUtf8(currentfilename).GetTaggedValue());
    }

    // 4. Let promiseCapability be !NewPromiseCapability(%Promise%).
    JSHandle<JSTaggedValue> promiseFunc = env->GetPromiseFunction();
    JSHandle<PromiseCapability> promiseCapability = JSPromise::NewPromiseCapability(thread, promiseFunc);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<job::MicroJobQueue> job = thread->GetCurrentEcmaContext()->GetMicroJobQueue();

    JSHandle<TaggedArray> argv = factory->NewTaggedArray(5); // 5: 5 means parameters stored in array
    argv->Set(thread, 0, promiseCapability->GetResolve());
    argv->Set(thread, 1, promiseCapability->GetReject());    // 1 : reject method
    argv->Set(thread, 2, dirPath);                           // 2 : current file path(containing file name)
    argv->Set(thread, 3, specifier);                         // 3 : request module's path
    argv->Set(thread, 4, recordName);                        // 4 : js recordName or undefined

    JSHandle<JSFunction> dynamicImportJob(env->GetDynamicImportJob());
    job::MicroJobQueue::EnqueueJob(thread, job, job::QueueType::QUEUE_PROMISE, dynamicImportJob, argv);

    return promiseCapability->GetPromise();
}

JSTaggedValue RuntimeStubs::RuntimeEq(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                         const JSHandle<JSTaggedValue> &right)
{
    bool ret = JSTaggedValue::Equal(thread, left, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return (ret ? JSTaggedValue::True() : JSTaggedValue::False());
}

JSTaggedValue RuntimeStubs::RuntimeNotEq(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                            const JSHandle<JSTaggedValue> &right)
{
    bool ret = JSTaggedValue::Equal(thread, left, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return (ret ? JSTaggedValue::False() : JSTaggedValue::True());
}

JSTaggedValue RuntimeStubs::RuntimeLess(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                           const JSHandle<JSTaggedValue> &right)
{
    bool ret = JSTaggedValue::Compare(thread, left, right) == ComparisonResult::LESS;
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return (ret ? JSTaggedValue::True() : JSTaggedValue::False());
}

JSTaggedValue RuntimeStubs::RuntimeLessEq(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                             const JSHandle<JSTaggedValue> &right)
{
    bool ret = JSTaggedValue::Compare(thread, left, right) <= ComparisonResult::EQUAL;
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return (ret ? JSTaggedValue::True() : JSTaggedValue::False());
}

JSTaggedValue RuntimeStubs::RuntimeGreater(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                              const JSHandle<JSTaggedValue> &right)
{
    bool ret = JSTaggedValue::Compare(thread, left, right) == ComparisonResult::GREAT;
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return (ret ? JSTaggedValue::True() : JSTaggedValue::False());
}

JSTaggedValue RuntimeStubs::RuntimeGreaterEq(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                                const JSHandle<JSTaggedValue> &right)
{
    ComparisonResult comparison = JSTaggedValue::Compare(thread, left, right);
    bool ret = (comparison == ComparisonResult::GREAT) || (comparison == ComparisonResult::EQUAL);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return (ret ? JSTaggedValue::True() : JSTaggedValue::False());
}

JSTaggedValue RuntimeStubs::RuntimeAdd2(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                           const JSHandle<JSTaggedValue> &right)
{
    if (left->IsString() && right->IsString()) {
        EcmaString *resultStr = EcmaStringAccessor::Concat(
            thread->GetEcmaVM(), JSHandle<EcmaString>(left), JSHandle<EcmaString>(right));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return JSTaggedValue(resultStr);
    }
    JSHandle<JSTaggedValue> primitiveA0(thread, JSTaggedValue::ToPrimitive(thread, left));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> primitiveA1(thread, JSTaggedValue::ToPrimitive(thread, right));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // contain string
    if (primitiveA0->IsString() || primitiveA1->IsString()) {
        JSHandle<EcmaString> stringA0 = JSTaggedValue::ToString(thread, primitiveA0);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<EcmaString> stringA1 = JSTaggedValue::ToString(thread, primitiveA1);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        EcmaString *resultStr = EcmaStringAccessor::Concat(thread->GetEcmaVM(), stringA0, stringA1);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return JSTaggedValue(resultStr);
    }
    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, primitiveA0);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, primitiveA1);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            JSHandle<BigInt> bigLeft(valLeft);
            JSHandle<BigInt> bigRight(valRight);
            return BigInt::Add(thread, bigLeft, bigRight).GetTaggedValue();
        }
        return RuntimeThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    double doubleA0 = valLeft->GetNumber();
    double doubleA1 = valRight->GetNumber();
    return JSTaggedValue(doubleA0 + doubleA1);
}

JSTaggedValue RuntimeStubs::RuntimeShl2(JSThread *thread,
                                        const JSHandle<JSTaggedValue> &left,
                                        const JSHandle<JSTaggedValue> &right)
{
    JSHandle<JSTaggedValue> leftValue = JSTaggedValue::ToNumeric(thread, left);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> rightValue = JSTaggedValue::ToNumeric(thread, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (leftValue->IsBigInt() || rightValue->IsBigInt()) {
        if (leftValue->IsBigInt() && rightValue->IsBigInt()) {
            JSHandle<BigInt> leftBigint(leftValue);
            JSHandle<BigInt> rightBigint(rightValue);
            return BigInt::LeftShift(thread, leftBigint, rightBigint).GetTaggedValue();
        }
        return RuntimeThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    JSTaggedValue taggedNumber0 = RuntimeToJSTaggedValueWithInt32(thread, leftValue);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedValue taggedNumber1 = RuntimeToJSTaggedValueWithInt32(thread, rightValue);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int32_t opNumber0 = taggedNumber0.GetInt();
    int32_t opNumber1 = taggedNumber1.GetInt();
    uint32_t shift =
            static_cast<uint32_t>(opNumber1) & 0x1f;  // NOLINT(hicpp-signed-bitwise, readability-magic-numbers)
    using unsigned_type = std::make_unsigned_t<int32_t>;
    auto ret =
            static_cast<int32_t>(static_cast<unsigned_type>(opNumber0) << shift);  // NOLINT(hicpp-signed-bitwise)
    return JSTaggedValue(ret);
}

JSTaggedValue RuntimeStubs::RuntimeShr2(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                        const JSHandle<JSTaggedValue> &right)
{
    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, left);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            return BigInt::UnsignedRightShift(thread);
        }
        return RuntimeThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    JSTaggedValue taggedNumber0 = RuntimeToJSTaggedValueWithInt32(thread, valLeft);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedValue taggedNumber1 = RuntimeToJSTaggedValueWithInt32(thread, valRight);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int32_t opNumber0 = taggedNumber0.GetInt();
    int32_t opNumber1 = taggedNumber1.GetInt();
    uint32_t shift = static_cast<uint32_t>(opNumber1) & 0x1f; // NOLINT(hicpp-signed-bitwise, readability-magic-numbers)
    using unsigned_type = std::make_unsigned_t<uint32_t>;
    auto ret =
            static_cast<uint32_t>(static_cast<unsigned_type>(opNumber0) >> shift); // NOLINT(hicpp-signed-bitwise)
    return JSTaggedValue(ret);
}

JSTaggedValue RuntimeStubs::RuntimeSub2(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                        const JSHandle<JSTaggedValue> &right)
{
    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, left);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            JSHandle<BigInt> bigLeft(valLeft);
            JSHandle<BigInt> bigRight(valRight);
            return BigInt::Subtract(thread, bigLeft, bigRight).GetTaggedValue();
        }
        return RuntimeThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    JSTaggedNumber number0(valLeft.GetTaggedValue());
    JSTaggedNumber number1(valRight.GetTaggedValue());
    return number0 - number1;
}

JSTaggedValue RuntimeStubs::RuntimeMul2(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                        const JSHandle<JSTaggedValue> &right)
{
    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, left);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 9. ReturnIfAbrupt(rnum).
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            JSHandle<BigInt> bigLeft(valLeft);
            JSHandle<BigInt> bigRight(valRight);
            return BigInt::Multiply(thread, bigLeft, bigRight).GetTaggedValue();
        }
        return RuntimeThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    // 12.6.3.1 Applying the * Operator
    JSTaggedNumber number0(valLeft.GetTaggedValue());
    JSTaggedNumber number1(valRight.GetTaggedValue());
    return number0 * number1;
}

JSTaggedValue RuntimeStubs::RuntimeDiv2(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                        const JSHandle<JSTaggedValue> &right)
{
    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, left);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            JSHandle<BigInt> bigLeft(valLeft);
            JSHandle<BigInt> bigRight(valRight);
            return BigInt::Divide(thread, bigLeft, bigRight).GetTaggedValue();
        }
        return RuntimeThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    double dLeft = valLeft->GetNumber();
    double dRight = valRight->GetNumber();
    if (dRight == 0) {
        if (dLeft == 0 || std::isnan(dLeft)) {
            return JSTaggedValue(base::NAN_VALUE);
        }
        bool positive = (((base::bit_cast<uint64_t>(dRight)) & base::DOUBLE_SIGN_MASK) ==
                         ((base::bit_cast<uint64_t>(dLeft)) & base::DOUBLE_SIGN_MASK));
        return JSTaggedValue(positive ? base::POSITIVE_INFINITY : -base::POSITIVE_INFINITY);
    }
    return JSTaggedValue(dLeft / dRight);
}

JSTaggedValue RuntimeStubs::RuntimeMod2(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                        const JSHandle<JSTaggedValue> &right)
{
    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, left);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 12.6.3.3 Applying the % Operator
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            JSHandle<BigInt> leftBigint(valLeft);
            JSHandle<BigInt> rightBigint(valRight);
            return BigInt::Remainder(thread, leftBigint, rightBigint).GetTaggedValue();
        }
        return RuntimeThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    double dLeft = valLeft->GetNumber();
    double dRight = valRight->GetNumber();
    // 12.6.3.3 Applying the % Operator
    if ((dRight == 0.0) || std::isnan(dRight) || std::isnan(dLeft) || std::isinf(dLeft)) {
        return JSTaggedValue(base::NAN_VALUE);
    }
    if ((dLeft == 0.0) || std::isinf(dRight)) {
        return JSTaggedValue(dLeft);
    }
    return JSTaggedValue(std::fmod(dLeft, dRight));
}

JSTaggedValue RuntimeStubs::RuntimeAshr2(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                         const JSHandle<JSTaggedValue> &right)
{
    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, left);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            JSHandle<BigInt> bigLeft(valLeft);
            JSHandle<BigInt> bigRight(valRight);
            return BigInt::SignedRightShift(thread, bigLeft, bigRight).GetTaggedValue();
        }
        return RuntimeThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    JSTaggedValue taggedNumber0 = RuntimeToJSTaggedValueWithInt32(thread, valLeft);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedValue taggedNumber1 = RuntimeToJSTaggedValueWithInt32(thread, valRight);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int32_t opNumber0 = taggedNumber0.GetInt();
    int32_t opNumber1 = taggedNumber1.GetInt();
    uint32_t shift = static_cast<uint32_t>(opNumber1) & 0x1f; // NOLINT(hicpp-signed-bitwise, readability-magic-numbers)
    auto ret = static_cast<int32_t>(opNumber0 >> shift); // NOLINT(hicpp-signed-bitwise)
    return JSTaggedValue(ret);
}

JSTaggedValue RuntimeStubs::RuntimeAnd2(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                        const JSHandle<JSTaggedValue> &right)
{
    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, left);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            JSHandle<BigInt> leftBigint(valLeft);
            JSHandle<BigInt> rightBigint(valRight);
            return BigInt::BitwiseAND(thread, leftBigint, rightBigint).GetTaggedValue();
        }
        return RuntimeThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    JSTaggedValue taggedNumber0 = RuntimeToJSTaggedValueWithInt32(thread, valLeft);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedValue taggedNumber1 = RuntimeToJSTaggedValueWithInt32(thread, valRight);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int32_t opNumber0 = taggedNumber0.GetInt();
    int32_t opNumber1 = taggedNumber1.GetInt();
    // NOLINT(hicpp-signed-bitwise)
    auto ret = static_cast<uint32_t>(opNumber0) & static_cast<uint32_t>(opNumber1);
    return JSTaggedValue(static_cast<int32_t>(ret));
}

JSTaggedValue RuntimeStubs::RuntimeOr2(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                       const JSHandle<JSTaggedValue> &right)
{
    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, left);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            JSHandle<BigInt> leftBigint(valLeft);
            JSHandle<BigInt> rightBigint(valRight);
            return BigInt::BitwiseOR(thread, leftBigint, rightBigint).GetTaggedValue();
        }
        return RuntimeThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    JSTaggedValue taggedNumber0 = RuntimeToJSTaggedValueWithInt32(thread, valLeft);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedValue taggedNumber1 = RuntimeToJSTaggedValueWithInt32(thread, valRight);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int32_t opNumber0 = taggedNumber0.GetInt();
    int32_t opNumber1 = taggedNumber1.GetInt();
    // NOLINT(hicpp-signed-bitwise)
    auto ret = static_cast<uint32_t>(opNumber0) | static_cast<uint32_t>(opNumber1);
    return JSTaggedValue(static_cast<int32_t>(ret));
}

JSTaggedValue RuntimeStubs::RuntimeXor2(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                        const JSHandle<JSTaggedValue> &right)
{
    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, left);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            JSHandle<BigInt> leftBigint(valLeft);
            JSHandle<BigInt> rightBigint(valRight);
            return BigInt::BitwiseXOR(thread, leftBigint, rightBigint).GetTaggedValue();
        }
        return RuntimeThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    JSTaggedValue taggedNumber0 = RuntimeToJSTaggedValueWithInt32(thread, valLeft);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedValue taggedNumber1 = RuntimeToJSTaggedValueWithInt32(thread, valRight);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int32_t opNumber0 = taggedNumber0.GetInt();
    int32_t opNumber1 = taggedNumber1.GetInt();
    // NOLINT(hicpp-signed-bitwise)
    auto ret = static_cast<uint32_t>(opNumber0) ^ static_cast<uint32_t>(opNumber1);
    return JSTaggedValue(static_cast<int32_t>(ret));
}

JSTaggedValue RuntimeStubs::RuntimeToJSTaggedValueWithInt32(JSThread *thread,
                                                            const JSHandle<JSTaggedValue> &value)
{
    int32_t res = JSTaggedValue::ToInt32(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue(res);
}

JSTaggedValue RuntimeStubs::RuntimeToJSTaggedValueWithUint32(JSThread *thread, const JSHandle<JSTaggedValue> &value)
{
    uint32_t res = JSTaggedValue::ToUint32(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue(res);
}

JSTaggedValue RuntimeStubs::RuntimeCreateEmptyObject([[maybe_unused]] JSThread *thread, ObjectFactory *factory,
                                                     JSHandle<GlobalEnv> globalEnv)
{
    JSHandle<JSFunction> builtinObj(globalEnv->GetObjectFunction());
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(builtinObj);
    return obj.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeCreateEmptyArray([[maybe_unused]] JSThread *thread, ObjectFactory *factory,
                                                    JSHandle<GlobalEnv> globalEnv)
{
    JSHandle<JSFunction> builtinObj(globalEnv->GetArrayFunction());
    JSHandle<JSObject> arr = factory->NewJSObjectByConstructor(builtinObj);
    return arr.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeGetUnmapedArgs(JSThread *thread, JSTaggedType *sp, uint32_t actualNumArgs,
                                                  uint32_t startIdx)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> argumentsList = factory->NewTaggedArray(actualNumArgs);
    for (uint32_t i = 0; i < actualNumArgs; ++i) {
        argumentsList->Set(thread, i,
                           JSTaggedValue(sp[startIdx + i]));  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    return RuntimeGetUnmapedJSArgumentObj(thread, argumentsList);
}

JSTaggedValue RuntimeStubs::RuntimeCopyRestArgs(JSThread *thread, JSTaggedType *sp, uint32_t restNumArgs,
                                                uint32_t startIdx)
{
    JSHandle<JSTaggedValue> restArray = JSArray::ArrayCreate(thread, JSTaggedNumber(restNumArgs));

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSMutableHandle<JSTaggedValue> element(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < restNumArgs; ++i) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        element.Update(JSTaggedValue(sp[startIdx + i]));
        JSObject::SetProperty(thread, restArray, i, element, true);
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return restArray.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeCreateArrayWithBuffer(JSThread *thread, ObjectFactory *factory,
                                                         const JSHandle<JSTaggedValue> &literal)
{
    JSHandle<JSArray> array(literal);
    JSHandle<JSArray> arrLiteral = factory->CloneArrayLiteral(array);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return arrLiteral.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeCreateObjectWithBuffer(JSThread *thread, ObjectFactory *factory,
                                                          const JSHandle<JSObject> &literal)
{
    JSHandle<JSObject> objLiteral = factory->CloneObjectLiteral(literal);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return objLiteral.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeNewLexicalEnv(JSThread *thread, uint16_t numVars)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<LexicalEnv> newEnv = factory->NewLexicalEnv(numVars);

    JSTaggedValue currentLexenv = thread->GetCurrentLexenv();
    newEnv->SetParentEnv(thread, currentLexenv);
    newEnv->SetScopeInfo(thread, JSTaggedValue::Hole());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return newEnv.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeNewObjRange(JSThread *thread, const JSHandle<JSTaggedValue> &func,
    const JSHandle<JSTaggedValue> &newTarget, uint16_t firstArgIdx, uint16_t length)
{
    JSHandle<JSTaggedValue> preArgs(thread, JSTaggedValue::Undefined());
    JSHandle<TaggedArray> args = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(length);
    FrameHandler frameHandler(thread);
    for (uint16_t i = 0; i < length; ++i) {
        JSTaggedValue value = frameHandler.GetVRegValue(firstArgIdx + i);
        args->Set(thread, i, value);
    }
    auto tagged = RuntimeOptConstruct(thread, func, newTarget, preArgs, args);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return tagged;
}

JSTaggedValue RuntimeStubs::RuntimeDefinefunc(JSThread *thread, const JSHandle<Method> &methodHandle)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->NewJSFunction(methodHandle).GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeCreateRegExpWithLiteral(JSThread *thread,
                                                           const JSHandle<JSTaggedValue> &pattern, uint8_t flags)
{
    JSHandle<JSTaggedValue> flagsHandle(thread, JSTaggedValue(flags));
    return builtins::BuiltinsRegExp::RegExpCreate(thread, pattern, flagsHandle);
}

JSTaggedValue RuntimeStubs::RuntimeThrowIfSuperNotCorrectCall(JSThread *thread, uint16_t index,
                                                              JSTaggedValue thisValue)
{
    if (index == 0 && (thisValue.IsUndefined() || thisValue.IsHole())) {
        return RuntimeThrowReferenceError(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                       "sub-class must call super before use 'this'");
    }
    if (index == 1 && !thisValue.IsUndefined() && !thisValue.IsHole()) {
        return RuntimeThrowReferenceError(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                       "super() forbidden re-bind 'this'");
    }
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeCreateObjectHavingMethod(JSThread *thread, ObjectFactory *factory,
                                                            const JSHandle<JSObject> &literal,
                                                            const JSHandle<JSTaggedValue> &env)
{
    JSHandle<JSObject> objLiteral = factory->CloneObjectLiteral(literal, env);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return objLiteral.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::CommonCreateObjectWithExcludedKeys(JSThread *thread,
                                                               const JSHandle<JSTaggedValue> &objVal,
                                                               uint32_t numExcludedKeys,
                                                               JSHandle<TaggedArray> excludedKeys)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> restObj = factory->NewEmptyJSObject();
    if (objVal->IsNull() || objVal->IsUndefined()) {
        return restObj.GetTaggedValue();
    }
    JSHandle<JSObject> obj(JSTaggedValue::ToObject(thread, objVal));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<TaggedArray> allKeys = JSObject::GetOwnPropertyKeys(thread, obj);
    uint32_t numAllKeys = allKeys->GetLength();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < numAllKeys; i++) {
        key.Update(allKeys->Get(i));
        bool isExcludedKey = false;
        for (uint32_t j = 0; j < numExcludedKeys; j++) {
            if (JSTaggedValue::Equal(thread, key, JSHandle<JSTaggedValue>(thread, excludedKeys->Get(j)))) {
                isExcludedKey = true;
                break;
            }
        }
        if (!isExcludedKey) {
            PropertyDescriptor desc(thread);
            bool success = JSObject::GetOwnProperty(thread, obj, key, desc);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (success && desc.IsEnumerable()) {
                JSHandle<JSTaggedValue> value = JSObject::GetProperty(thread, obj, key).GetValue();
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                JSObject::SetProperty(thread, restObj, key, value, true);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
        }
    }
    return restObj.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeOptCreateObjectWithExcludedKeys(JSThread *thread, uint16_t numKeys,
                                                                   const JSHandle<JSTaggedValue> &objVal,
                                                                   uint16_t firstArgRegIdx,
                                                                   uintptr_t argv, uint32_t argc)
{
    firstArgRegIdx += 4; // firstArgRegIdx + 4: means the remain parameter
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t numExcludedKeys = 0;
    JSHandle<TaggedArray> excludedKeys = factory->NewTaggedArray(numKeys + 1);
    JSTaggedValue excludedKey = GetArg(argv, argc, firstArgRegIdx);
    if (!excludedKey.IsUndefined()) {
        numExcludedKeys = numKeys + 1;
        excludedKeys->Set(thread, 0, excludedKey);
        for (uint32_t i = 1; i < numExcludedKeys; i++) {
            excludedKey = GetArg(argv, argc, firstArgRegIdx + i);
            excludedKeys->Set(thread, i, excludedKey);
        }
    }
    return CommonCreateObjectWithExcludedKeys(thread, objVal, numExcludedKeys, excludedKeys);
}

JSTaggedValue RuntimeStubs::RuntimeCreateObjectWithExcludedKeys(JSThread *thread, uint16_t numKeys,
                                                                const JSHandle<JSTaggedValue> &objVal,
                                                                uint16_t firstArgRegIdx)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t numExcludedKeys = 0;
    JSHandle<TaggedArray> excludedKeys = factory->NewTaggedArray(numKeys + 1);
    FrameHandler frameHandler(thread);
    JSTaggedValue excludedKey = frameHandler.GetVRegValue(firstArgRegIdx);
    if (!excludedKey.IsUndefined()) {
        numExcludedKeys = numKeys + 1;
        excludedKeys->Set(thread, 0, excludedKey);
        for (uint32_t i = 1; i < numExcludedKeys; i++) {
            excludedKey = frameHandler.GetVRegValue(firstArgRegIdx + i);
            excludedKeys->Set(thread, i, excludedKey);
        }
    }
    return CommonCreateObjectWithExcludedKeys(thread, objVal, numExcludedKeys, excludedKeys);
}

JSTaggedValue RuntimeStubs::RuntimeDefineMethod(JSThread *thread, const JSHandle<Method> &methodHandle,
                                                const JSHandle<JSTaggedValue> &homeObject)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->NewJSFunction(methodHandle, homeObject).GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeCallSpread(JSThread *thread,
                                              const JSHandle<JSTaggedValue> &func,
                                              const JSHandle<JSTaggedValue> &obj,
                                              const JSHandle<JSTaggedValue> &array)
{
    if ((!obj->IsUndefined() && !obj->IsECMAObject()) || !func->IsCallable() || !array->IsJSArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "cannot Callspread", JSTaggedValue::Exception());
    }

    JSHandle<TaggedArray> coretypesArray(thread, RuntimeGetCallSpreadArgs(thread, array));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    uint32_t length = coretypesArray->GetLength();
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, func, obj, undefined, length);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    info->SetCallArg(length, coretypesArray);
    return EcmaInterpreter::Execute(info);
}

JSTaggedValue RuntimeStubs::RuntimeDefineGetterSetterByValue(JSThread *thread, const JSHandle<JSObject> &obj,
                                                             const JSHandle<JSTaggedValue> &prop,
                                                             const JSHandle<JSTaggedValue> &getter,
                                                             const JSHandle<JSTaggedValue> &setter, bool flag)
{
    JSHandle<JSTaggedValue> propKey = JSTaggedValue::ToPropertyKey(thread, prop);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    auto globalConst = thread->GlobalConstants();
    if (obj.GetTaggedValue().IsClassConstructor() &&
        JSTaggedValue::SameValue(propKey, globalConst->GetHandledPrototypeString())) {
        return RuntimeThrowTypeError(
            thread,
            "In a class, computed property names for static getter that are named 'prototype' throw a TypeError");
    }

    if (flag) {
        if (!getter->IsUndefined()) {
            if (propKey->IsNumber()) {
                propKey =
                    JSHandle<JSTaggedValue>::Cast(base::NumberHelper::NumberToString(thread, propKey.GetTaggedValue()));
            }
            JSFunctionBase::SetFunctionName(thread, JSHandle<JSFunctionBase>::Cast(getter), propKey,
                                            JSHandle<JSTaggedValue>(thread, globalConst->GetGetString()));
        }

        if (!setter->IsUndefined()) {
            if (propKey->IsNumber()) {
                propKey =
                    JSHandle<JSTaggedValue>::Cast(base::NumberHelper::NumberToString(thread, propKey.GetTaggedValue()));
            }
            JSFunctionBase::SetFunctionName(thread, JSHandle<JSFunctionBase>::Cast(setter), propKey,
                                            JSHandle<JSTaggedValue>(thread, globalConst->GetSetString()));
        }
    }

    // set accessor
    bool enumerable =
        !(obj.GetTaggedValue().IsClassPrototype() || obj.GetTaggedValue().IsClassConstructor());
    PropertyDescriptor desc(thread, true, enumerable, true);
    if (!getter->IsUndefined()) {
        Method *method = Method::Cast(JSHandle<JSFunction>::Cast(getter)->GetMethod().GetTaggedObject());
        method->SetFunctionKind(FunctionKind::GETTER_FUNCTION);
        desc.SetGetter(getter);
    }
    if (!setter->IsUndefined()) {
        Method *method = Method::Cast(JSHandle<JSFunction>::Cast(setter)->GetMethod().GetTaggedObject());
        method->SetFunctionKind(FunctionKind::SETTER_FUNCTION);
        desc.SetSetter(setter);
    }
    JSObject::DefineOwnProperty(thread, obj, propKey, desc);

    return obj.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeSuperCall(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                             const JSHandle<JSTaggedValue> &newTarget, uint16_t firstVRegIdx,
                                             uint16_t length)
{
    JSHandle<JSTaggedValue> superFunc(thread, JSTaggedValue::GetPrototype(thread, func));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    ASSERT(superFunc->IsJSFunction());

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> argv = factory->NewTaggedArray(length);
    FrameHandler frameHandler(thread);
    for (size_t i = 0; i < length; ++i) {
        argv->Set(thread, i, frameHandler.GetVRegValue(firstVRegIdx + i));
    }

    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, superFunc, undefined, newTarget, length);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    info->SetCallArg(length, argv);
    JSTaggedValue result = JSFunction::Construct(info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return result;
}

JSTaggedValue RuntimeStubs::RuntimeOptSuperCall(JSThread *thread, uintptr_t argv, uint32_t argc)
{
    constexpr size_t fixNums = 2;
    JSHandle<JSTaggedValue> func = GetHArg<JSTaggedValue>(argv, argc, 0);
    JSHandle<JSTaggedValue> newTarget = GetHArg<JSTaggedValue>(argv, argc, 1);
    JSHandle<JSTaggedValue> superFunc(thread, JSTaggedValue::GetPrototype(thread, func));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    ASSERT(superFunc->IsJSFunction());
    uint16_t length = argc - fixNums;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, superFunc, undefined, newTarget, length);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    for (size_t i = 0; i < length; ++i) {
        JSTaggedType arg = reinterpret_cast<JSTaggedType *>(argv)[i + fixNums];
        info->SetCallArg(i, JSTaggedValue(arg));
    }
    JSTaggedValue result = JSFunction::Construct(info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return result;
}

JSTaggedValue RuntimeStubs::RuntimeThrowTypeError(JSThread *thread, const char *message)
{
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    THROW_TYPE_ERROR_AND_RETURN(thread, message, JSTaggedValue::Exception());
}

JSTaggedValue RuntimeStubs::RuntimeGetCallSpreadArgs(JSThread *thread, const JSHandle<JSTaggedValue> &jsArray)
{
    uint32_t argvMayMaxLength = JSHandle<JSArray>::Cast(jsArray)->GetArrayLength();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> argv = factory->NewTaggedArray(argvMayMaxLength);
    JSHandle<JSTaggedValue> itor = JSIterator::GetIterator(thread, jsArray);

    // Fast path when array is stablearray and Iterator not change.
    if (jsArray->IsStableJSArray(thread) && itor->IsJSArrayIterator()) {
        JSHandle<TaggedArray> srcElements(thread, JSHandle<JSObject>::Cast(jsArray)->GetElements());
        TaggedArray::CopyTaggedArrayElement(thread, srcElements, argv, argvMayMaxLength);
        return argv.GetTaggedValue();
    }

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSMutableHandle<JSTaggedValue> next(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> nextArg(thread, JSTaggedValue::Undefined());
    size_t argvIndex = 0;
    while (true) {
        next.Update(JSIterator::IteratorStep(thread, itor).GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (JSTaggedValue::SameValue(next.GetTaggedValue(), JSTaggedValue::False())) {
            break;
        }
        nextArg.Update(JSIterator::IteratorValue(thread, next).GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (UNLIKELY(argvIndex + 1 >= argvMayMaxLength)) {
            argvMayMaxLength = argvMayMaxLength + (argvMayMaxLength >> 1U) + 1U;
            argv = argv->SetCapacity(thread, argv, argvMayMaxLength);
        }
        argv->Set(thread, argvIndex++, nextArg);
    }
    argv = factory->CopyArray(argv, argvMayMaxLength, argvIndex);
    return argv.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeThrowSyntaxError(JSThread *thread, const char *message)
{
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    THROW_SYNTAX_ERROR_AND_RETURN(thread, message, JSTaggedValue::Exception());
}

JSTaggedValue RuntimeStubs::RuntimeLdBigInt(JSThread *thread, const JSHandle<JSTaggedValue> &numberBigInt)
{
    return JSTaggedValue::ToBigInt(thread, numberBigInt);
}

JSTaggedValue RuntimeStubs::RuntimeNewLexicalEnvWithName(JSThread *thread, uint16_t numVars, uint16_t scopeId)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<LexicalEnv> newEnv = factory->NewLexicalEnv(numVars);

    JSTaggedValue currentLexenv = thread->GetCurrentLexenv();
    newEnv->SetParentEnv(thread, currentLexenv);
    JSTaggedValue scopeInfo = ScopeInfoExtractor::GenerateScopeInfo(thread, scopeId);
    newEnv->SetScopeInfo(thread, scopeInfo);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return newEnv.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeOptGetUnmapedArgs(JSThread *thread, uint32_t actualNumArgs)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> argumentsList = factory->NewTaggedArray(actualNumArgs - NUM_MANDATORY_JSFUNC_ARGS);

    auto argv = GetActualArgvFromStub(thread);
    int idx = 0;
    for (uint32_t i = NUM_MANDATORY_JSFUNC_ARGS; i < actualNumArgs; ++i) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        JSTaggedType arg = reinterpret_cast<JSTaggedType *>(argv)[i];
        JSTaggedValue args = JSTaggedValue(arg);
        argumentsList->Set(thread, idx++, args);
    }
    return RuntimeGetUnmapedJSArgumentObj(thread, argumentsList);
}

JSTaggedValue RuntimeStubs::RuntimeGetUnmapedJSArgumentObj(JSThread *thread, const JSHandle<TaggedArray> &argumentsList)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    // 1. Let len be the number of elements in argumentsList
    uint32_t len = argumentsList->GetLength();
    // 2. Let obj be ObjectCreate(%ObjectPrototype%, «[[ParameterMap]]»).
    // 3. Set obj’s [[ParameterMap]] internal slot to undefined.
    // [[ParameterMap]] setted as undifined.
    JSHandle<JSArguments> obj = factory->NewJSArguments();
    // 4. Perform DefinePropertyOrThrow(obj, "length", PropertyDescriptor{[[Value]]: len, [[Writable]]: true,
    // [[Enumerable]]: false, [[Configurable]]: true}).
    obj->SetPropertyInlinedProps(thread, JSArguments::LENGTH_INLINE_PROPERTY_INDEX, JSTaggedValue(len));
    // 5. Let index be 0.
    // 6. Repeat while index < len,
    //    a. Let val be argumentsList[index].
    //    b. Perform CreateDataProperty(obj, ToString(index), val).
    //    c. Let index be index + 1
    obj->SetElements(thread, argumentsList.GetTaggedValue());
    // 7. Perform DefinePropertyOrThrow(obj, @@iterator, PropertyDescriptor
    // {[[Value]]:%ArrayProto_values%,
    // [[Writable]]: true, [[Enumerable]]: false, [[Configurable]]: true}).
    obj->SetPropertyInlinedProps(thread, JSArguments::ITERATOR_INLINE_PROPERTY_INDEX,
                                 globalEnv->GetArrayProtoValuesFunction().GetTaggedValue());
    // 8. Perform DefinePropertyOrThrow(obj, "caller", PropertyDescriptor {[[Get]]: %ThrowTypeError%,
    // [[Set]]: %ThrowTypeError%, [[Enumerable]]: false, [[Configurable]]: false}).
    JSHandle<JSTaggedValue> accessorCaller = globalEnv->GetArgumentsCallerAccessor();
    obj->SetPropertyInlinedProps(thread, JSArguments::CALLER_INLINE_PROPERTY_INDEX, accessorCaller.GetTaggedValue());
    // 9. Perform DefinePropertyOrThrow(obj, "callee", PropertyDescriptor {[[Get]]: %ThrowTypeError%,
    // [[Set]]: %ThrowTypeError%, [[Enumerable]]: false, [[Configurable]]: false}).
    JSHandle<JSTaggedValue> accessorCallee = globalEnv->GetArgumentsCalleeAccessor();
    obj->SetPropertyInlinedProps(thread, JSArguments::CALLEE_INLINE_PROPERTY_INDEX, accessorCallee.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 11. Return obj
    return obj.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeOptNewLexicalEnvWithName(JSThread *thread, uint16_t numVars, uint16_t scopeId,
                                                            JSHandle<JSTaggedValue> &currentLexEnv,
                                                            JSHandle<JSTaggedValue> &func)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<LexicalEnv> newEnv = factory->NewLexicalEnv(numVars);

    newEnv->SetParentEnv(thread, currentLexEnv.GetTaggedValue());
    JSTaggedValue scopeInfo = RuntimeOptGenerateScopeInfo(thread, scopeId, func.GetTaggedValue());
    newEnv->SetScopeInfo(thread, scopeInfo);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedValue taggedEnv = newEnv.GetTaggedValue();
    return taggedEnv;
}

JSTaggedValue RuntimeStubs::RuntimeOptCopyRestArgs(JSThread *thread, uint32_t actualArgc, uint32_t restIndex)
{
    // when only have three fixed args, restIndex in bytecode maybe not zero, but it actually should be zero.
    uint32_t actualRestNum = 0;
    if (actualArgc > NUM_MANDATORY_JSFUNC_ARGS + restIndex) {
        actualRestNum = actualArgc - NUM_MANDATORY_JSFUNC_ARGS - restIndex;
    }
    JSHandle<JSTaggedValue> restArray = JSArray::ArrayCreate(thread, JSTaggedNumber(actualRestNum));

    auto argv = GetActualArgv(thread);
    int idx = 0;
    JSMutableHandle<JSTaggedValue> element(thread, JSTaggedValue::Undefined());

    for (uint32_t i = NUM_MANDATORY_JSFUNC_ARGS + restIndex; i < actualArgc; ++i) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        JSTaggedType arg = reinterpret_cast<JSTaggedType *>(argv)[i];
        element.Update(JSTaggedValue(arg));
        JSObject::SetProperty(thread, restArray, idx++, element, true);
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return restArray.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeOptSuspendGenerator(JSThread *thread, const JSHandle<JSTaggedValue> &genObj,
                                                       const JSHandle<JSTaggedValue> &value)
{
    if (genObj->IsAsyncGeneratorObject()) {
        JSHandle<JSAsyncGeneratorObject> generatorObjectHandle(genObj);
        // change state to SuspendedYield
        if (generatorObjectHandle->IsExecuting()) {
            return value.GetTaggedValue();
        }
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return generatorObjectHandle.GetTaggedValue();
    }

    if (genObj->IsGeneratorObject()) {
        JSHandle<JSGeneratorObject> generatorObjectHandle(genObj);
        // change state to SuspendedYield
        if (generatorObjectHandle->IsExecuting()) {
            generatorObjectHandle->SetGeneratorState(JSGeneratorState::SUSPENDED_YIELD);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            return value.GetTaggedValue();
        }
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return generatorObjectHandle.GetTaggedValue();
    }

    return RuntimeThrowTypeError(thread, "RuntimeSuspendGenerator failed");
}

JSTaggedValue RuntimeStubs::RuntimeOptAsyncGeneratorResolve(JSThread *thread, JSHandle<JSTaggedValue> asyncFuncObj,
                                                            JSHandle<JSTaggedValue> value, JSTaggedValue flag)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSAsyncGeneratorObject> asyncGeneratorObjHandle(asyncFuncObj);
    JSHandle<JSTaggedValue> valueHandle(value);
    JSHandle<GeneratorContext> genContextHandle(thread, asyncGeneratorObjHandle->GetGeneratorContext());
    ASSERT(flag.IsBoolean());
    bool done = flag.IsTrue();
    return JSAsyncGeneratorObject::AsyncGeneratorResolve(thread, asyncGeneratorObjHandle, valueHandle, done);
}

JSTaggedValue RuntimeStubs::RuntimeOptConstruct(JSThread *thread, JSHandle<JSTaggedValue> ctor,
                                                JSHandle<JSTaggedValue> newTarget, JSHandle<JSTaggedValue> preArgs,
                                                JSHandle<TaggedArray> args)
{
    if (newTarget->IsUndefined()) {
        newTarget = ctor;
    }

    if (!(newTarget->IsConstructor() && ctor->IsConstructor())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Constructor is false", JSTaggedValue::Exception());
    }

    if (ctor->IsJSFunction()) {
        return RuntimeOptConstructGeneric(thread, JSHandle<JSFunction>::Cast(ctor), newTarget, preArgs, args);
    }
    if (ctor->IsBoundFunction()) {
        return RuntimeOptConstructBoundFunction(
            thread, JSHandle<JSBoundFunction>::Cast(ctor), newTarget, preArgs, args);
    }
    if (ctor->IsJSProxy()) {
        return RuntimeOptConstructProxy(thread, JSHandle<JSProxy>::Cast(ctor), newTarget, preArgs, args);
    }
    THROW_TYPE_ERROR_AND_RETURN(thread, "Constructor NonConstructor", JSTaggedValue::Exception());
}

JSTaggedValue RuntimeStubs::RuntimeOptConstructProxy(JSThread *thread, JSHandle<JSProxy> ctor,
                                                     JSHandle<JSTaggedValue> newTgt, JSHandle<JSTaggedValue> preArgs,
                                                     JSHandle<TaggedArray> args)
{
    // step 1 ~ 4 get ProxyHandler and ProxyTarget
    JSHandle<JSTaggedValue> handler(thread, ctor->GetHandler());
    if (handler->IsNull()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Constructor: handler is null", JSTaggedValue::Exception());
    }
    ASSERT(handler->IsJSObject());
    JSHandle<JSTaggedValue> target(thread, ctor->GetTarget());

    // 5.Let trap be GetMethod(handler, "construct").
    JSHandle<JSTaggedValue> key(thread->GlobalConstants()->GetHandledProxyConstructString());
    JSHandle<JSTaggedValue> method = JSObject::GetMethod(thread, handler, key);

    // 6.ReturnIfAbrupt(trap).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 7.If trap is undefined, then
    //   a.Assert: target has a [[Construct]] internal method.
    //   b.Return Construct(target, argumentsList, newTarget).
    if (method->IsUndefined()) {
        ASSERT(target->IsConstructor());
        return RuntimeOptConstruct(thread, target, newTgt, preArgs, args);
    }

    // 8.Let argArray be CreateArrayFromList(argumentsList).
    uint32_t preArgsSize = preArgs->IsUndefined() ? 0 : JSHandle<TaggedArray>::Cast(preArgs)->GetLength();
    const uint32_t argsCount = args->GetLength();
    const uint32_t size = preArgsSize + argsCount;
    JSHandle<TaggedArray> arr = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(size);
    if (preArgsSize > 0) {
        JSHandle<TaggedArray> tgaPreArgs = JSHandle<TaggedArray>::Cast(preArgs);
        for (uint32_t i = 0; i < preArgsSize; ++i) {
            JSTaggedValue value = tgaPreArgs->Get(i);
            arr->Set(thread, i, value);
        }
    }

    for (uint32_t i = 0; i < argsCount; ++i) {
        JSTaggedValue value = args->Get(i);
        arr->Set(thread, i + preArgsSize, value);
    }

    // step 8 ~ 9 Call(trap, handler, «target, argArray, newTarget »).
    const uint32_t argsLength = 3;  // 3: «target, argArray, newTarget »
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, method, handler, undefined, argsLength);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    info->SetCallArg(target.GetTaggedValue(), arr.GetTaggedValue(), newTgt.GetTaggedValue());
    JSTaggedValue newObjValue = JSFunction::Call(info);
    // 10.ReturnIfAbrupt(newObj).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 11.If Type(newObj) is not Object, throw a TypeError exception.
    if (!newObjValue.IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "new object is not object", JSTaggedValue::Exception());
    }
    // 12.Return newObj.
    return newObjValue;
}

JSTaggedValue RuntimeStubs::RuntimeOptConstructBoundFunction(JSThread *thread, JSHandle<JSBoundFunction> ctor,
                                                             JSHandle<JSTaggedValue> newTgt,
                                                             JSHandle<JSTaggedValue> preArgs,
                                                             JSHandle<TaggedArray> args)
{
    JSHandle<JSTaggedValue> target(thread, ctor->GetBoundTarget());
    ASSERT(target->IsConstructor());

    JSHandle<TaggedArray> boundArgs(thread, ctor->GetBoundArguments());
    JSMutableHandle<JSTaggedValue> newPreArgs(thread, preArgs.GetTaggedValue());
    if (newPreArgs->IsUndefined()) {
        newPreArgs.Update(boundArgs.GetTaggedValue());
    } else {
        newPreArgs.Update(
            TaggedArray::Append(thread, boundArgs, JSHandle<TaggedArray>::Cast(preArgs)).GetTaggedValue());
    }
    JSMutableHandle<JSTaggedValue> newTargetMutable(thread, newTgt.GetTaggedValue());
    if (JSTaggedValue::SameValue(ctor.GetTaggedValue(), newTgt.GetTaggedValue())) {
        newTargetMutable.Update(target.GetTaggedValue());
    }
    return RuntimeOptConstruct(thread, target, newTargetMutable, newPreArgs, args);
}

JSTaggedValue RuntimeStubs::RuntimeOptConstructGeneric(JSThread *thread, JSHandle<JSFunction> ctor,
                                                       JSHandle<JSTaggedValue> newTgt,
                                                       JSHandle<JSTaggedValue> preArgs, JSHandle<TaggedArray> args)
{
    if (!ctor->IsConstructor()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Constructor is false", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> obj(thread, JSTaggedValue::Undefined());
    if (ctor->IsBase()) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        obj = JSHandle<JSTaggedValue>(factory->NewJSObjectByConstructor(ctor, newTgt));
    }

    uint32_t preArgsSize = preArgs->IsUndefined() ? 0 : JSHandle<TaggedArray>::Cast(preArgs)->GetLength();
    const uint32_t argsCount = args->GetLength();
    uint32_t size = preArgsSize + argsCount;
    CVector<JSTaggedType> values;
    Method *method = ctor->GetCallTarget();
    bool isAotMethod = method->IsAotWithCallField();
    if (isAotMethod && ctor->IsClassConstructor()) {
        if (method->IsFastCall()) {
            values.reserve(size + NUM_MANDATORY_JSFUNC_ARGS - 1);
            values.emplace_back(ctor.GetTaggedValue().GetRawData());
            values.emplace_back(obj.GetTaggedValue().GetRawData());
        } else {
            values.reserve(size + NUM_MANDATORY_JSFUNC_ARGS);
            values.emplace_back(ctor.GetTaggedValue().GetRawData());
            values.emplace_back(newTgt.GetTaggedValue().GetRawData());
            values.emplace_back(obj.GetTaggedValue().GetRawData());
        }
    } else {
        values.reserve(size);
    }

    if (preArgsSize > 0) {
        JSHandle<TaggedArray> tgaPreArgs = JSHandle<TaggedArray>::Cast(preArgs);
        for (uint32_t i = 0; i < preArgsSize; ++i) {
            JSTaggedValue value = tgaPreArgs->Get(i);
            values.emplace_back(value.GetRawData());
        }
        for (uint32_t i = 0; i < argsCount; ++i) {
            values.emplace_back(args->Get(i).GetRawData());
        }
    } else {
        for (uint32_t i = 0; i < argsCount; ++i) {
            values.emplace_back(args->Get(i).GetRawData());
        }
    }
    JSTaggedValue resultValue;
    if (isAotMethod && ctor->IsClassConstructor()) {
        uint32_t numArgs = ctor->GetCallTarget()->GetNumArgsWithCallField();
        bool needPushUndefined = numArgs > size;
        const JSTaggedType *prevFp = thread->GetLastLeaveFrame();
        if (ctor->GetCallTarget()->IsFastCall()) {
            if (needPushUndefined) {
                values.reserve(numArgs + NUM_MANDATORY_JSFUNC_ARGS - 1);
                for (uint32_t i = size; i < numArgs; i++) {
                    values.emplace_back(JSTaggedValue::VALUE_UNDEFINED);
                }
                size = numArgs;
            }
            resultValue = thread->GetEcmaVM()->FastCallAot(size, values.data(), prevFp);
        } else {
            resultValue = thread->GetCurrentEcmaContext()->ExecuteAot(size, values.data(), prevFp, needPushUndefined);
        }
    } else {
        ctor->GetCallTarget()->SetAotCodeBit(false); // if Construct is not ClassConstructor, don't run aot
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, JSHandle<JSTaggedValue>(ctor), obj, newTgt, size);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(size, values.data());
        resultValue = EcmaInterpreter::Execute(info);
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 9.3.2 [[Construct]] (argumentsList, newTarget)
    if (resultValue.IsECMAObject()) {
        return resultValue;
    }

    if (ctor->IsBase()) {
        return obj.GetTaggedValue();
    }
    if (!resultValue.IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "function is non-constructor", JSTaggedValue::Exception());
    }
    return obj.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeOptNewObjRange(JSThread *thread, uintptr_t argv, uint32_t argc)
{
    JSHandle<JSTaggedValue> ctor = GetHArg<JSTaggedValue>(argv, argc, 0);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    const size_t firstArgOffset = 1;
    size_t arrLength = argc - firstArgOffset;
    JSHandle<TaggedArray> args = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(arrLength);
    for (size_t i = 0; i < arrLength; ++i) {
        args->Set(thread, i, GetArg(argv, argc, i + firstArgOffset));
    }
    JSTaggedValue object = RuntimeOptConstruct(thread, ctor, ctor, undefined, args);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!object.IsUndefined() && !object.IsECMAObject() && !JSHandle<JSFunction>(ctor)->IsBase()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Derived constructor must return object or undefined",
                                    JSTaggedValue::Exception());
    }
    return object;
}

JSTaggedValue RuntimeStubs::RuntimeOptGenerateScopeInfo(JSThread *thread, uint16_t scopeId, JSTaggedValue func)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    Method *method = ECMAObject::Cast(func.GetTaggedObject())->GetCallTarget();
    const JSPandaFile *jsPandaFile = method->GetJSPandaFile();
    JSHandle<ConstantPool> constpool(thread, method->GetConstantPool());
    panda_file::File::EntityId id = constpool->GetEntityId(scopeId);
    JSHandle<TaggedArray> elementsLiteral =
        LiteralDataExtractor::GetDatasIgnoreType(thread, jsPandaFile, id, constpool);

    ASSERT(elementsLiteral->GetLength() > 0);

    auto *scopeDebugInfo = ecmaVm->GetNativeAreaAllocator()->New<ScopeDebugInfo>();
    if (scopeDebugInfo == nullptr) {
        return JSTaggedValue::Hole();
    }

    size_t length = elementsLiteral->GetLength();
    for (size_t i = 1; i < length; i += 2) {  // 2: Each literal buffer contains a pair of key-value.
        JSTaggedValue val = elementsLiteral->Get(i);
        ASSERT(val.IsString());
        CString name = ConvertToString(EcmaString::Cast(val.GetTaggedObject()));
        int32_t slot = elementsLiteral->Get(i + 1).GetInt();
        scopeDebugInfo->scopeInfo.emplace(name, slot);
    }

    JSHandle<JSNativePointer> pointer = factory->NewJSNativePointer(
        scopeDebugInfo, NativeAreaAllocator::FreeObjectFunc<ScopeDebugInfo>, ecmaVm->GetNativeAreaAllocator());
    return pointer.GetTaggedValue();
}

JSTaggedType *RuntimeStubs::GetActualArgv(JSThread *thread)
{
    JSTaggedType *current = const_cast<JSTaggedType *>(thread->GetLastLeaveFrame());
    FrameIterator it(current, thread);
    ASSERT(it.IsLeaveFrame());
    it.Advance<GCVisitedFlag::VISITED>();
    ASSERT(it.IsOptimizedJSFunctionFrame());
    auto optimizedJSFunctionFrame = it.GetFrame<OptimizedJSFunctionFrame>();
    return optimizedJSFunctionFrame->GetArgv(it);
}

JSTaggedType *RuntimeStubs::GetActualArgvFromStub(JSThread *thread)
{
    JSTaggedType *current = const_cast<JSTaggedType *>(thread->GetLastLeaveFrame());
    FrameIterator it(current, thread);
    ASSERT(it.IsLeaveFrame());
    it.Advance<GCVisitedFlag::VISITED>();
    ASSERT(it.IsOptimizedFrame());
    it.Advance<GCVisitedFlag::VISITED>();
    ASSERT(it.IsOptimizedJSFunctionFrame());
    auto optimizedJSFunctionFrame = it.GetFrame<OptimizedJSFunctionFrame>();
    return optimizedJSFunctionFrame->GetArgv(it);
}

OptimizedJSFunctionFrame *RuntimeStubs::GetOptimizedJSFunctionFrame(JSThread *thread)
{
    JSTaggedType *current = const_cast<JSTaggedType *>(thread->GetLastLeaveFrame());
    FrameIterator it(current, thread);
    ASSERT(it.IsLeaveFrame());
    it.Advance();
    ASSERT(it.IsOptimizedJSFunctionFrame());
    return it.GetFrame<OptimizedJSFunctionFrame>();
}

OptimizedJSFunctionFrame *RuntimeStubs::GetOptimizedJSFunctionFrameNoGC(JSThread *thread)
{
    JSTaggedType *current = const_cast<JSTaggedType *>(thread->GetLastLeaveFrame());
    FrameIterator it(current, thread);
    ASSERT(it.IsOptimizedFrame());
    it.Advance();
    ASSERT(it.IsOptimizedJSFunctionFrame());
    return it.GetFrame<OptimizedJSFunctionFrame>();
}

JSTaggedValue RuntimeStubs::RuntimeLdPatchVar(JSThread *thread, uint32_t index)
{
    JSHandle<TaggedArray> globalPatch =
        JSHandle<TaggedArray>::Cast(thread->GetEcmaVM()->GetGlobalEnv()->GetGlobalPatch());

    return globalPatch->Get(thread, index);
}

JSTaggedValue RuntimeStubs::RuntimeStPatchVar(JSThread *thread, uint32_t index, const JSHandle<JSTaggedValue> &value)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<TaggedArray> globalPatch = JSHandle<TaggedArray>::Cast(env->GetGlobalPatch());
    if (index >= globalPatch->GetLength()) {
        globalPatch = TaggedArray::SetCapacity(thread, globalPatch, index + 1);
    }
    globalPatch->Set(thread, index, value);
    env->SetGlobalPatch(thread, globalPatch.GetTaggedValue());
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeNotifyConcurrentResult(JSThread *thread, JSTaggedValue result, JSTaggedValue hint)
{
    thread->GetEcmaVM()->TriggerConcurrentCallback(result, hint);
    return JSTaggedValue::Undefined();
}

JSTaggedValue RuntimeStubs::RuntimeNotifyDebuggerStatement(JSThread *thread)
{
    FrameHandler frameHandler(thread);
    for (; frameHandler.HasFrame(); frameHandler.PrevJSFrame()) {
        if (frameHandler.IsEntryFrame() || frameHandler.IsBuiltinFrame()) {
            continue;
        }
        Method *method = frameHandler.GetMethod();
        if (method->IsNativeWithCallField()) {
            continue;
        }
        auto bcOffset = frameHandler.GetBytecodeOffset();
        auto *debuggerMgr = thread->GetEcmaVM()->GetJsDebuggerManager();
        debuggerMgr->GetNotificationManager()->DebuggerStmtEvent(thread, method, bcOffset);
        return JSTaggedValue::Hole();
    }
    return JSTaggedValue::Hole();
}

JSTaggedValue RuntimeStubs::RuntimeThrowStackOverflowException(JSThread *thread)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    // Multi-thread could cause stack-overflow-check failed too,
    // so check thread here to distinguish it with the actual stack overflow.
    ecmaVm->CheckThread();
    if (LIKELY(!thread->HasPendingException())) {
        ObjectFactory *factory = ecmaVm->GetFactory();
        JSHandle<JSObject> error = factory->GetJSError(ErrorType::RANGE_ERROR, "Stack overflow!", false);
        thread->SetException(error.GetTaggedValue());
    }
    return JSTaggedValue::Exception();
}

bool RuntimeStubs::CheckElementsNumber(JSHandle<TaggedArray> elements, uint32_t len)
{
    ASSERT(len <= elements->GetLength());
    for (uint32_t i = 0; i < len; i++) {
        if (!elements->Get(i).IsNumber()) {
            return false;
        }
    }
    return true;
}

JSHandle<JSTaggedValue> RuntimeStubs::GetOrCreateNumberString(JSThread *thread, JSHandle<JSTaggedValue> presentValue,
                                                              std::map<uint64_t, JSHandle<JSTaggedValue>> &cachedString)
{
    JSMutableHandle<JSTaggedValue> presentString(thread, JSTaggedValue::Undefined());
    auto iter = cachedString.find(presentValue->GetRawData());
    if (iter != cachedString.end()) {
        presentString.Update(iter->second);
    } else {
        presentString.Update(JSTaggedValue::ToString(thread, presentValue).GetTaggedValue());
        cachedString[presentValue->GetRawData()] = presentString;
    }
    return presentString;
}

JSTaggedValue RuntimeStubs::TryCopyCOWArray(JSThread *thread, JSHandle<JSArray> holderHandler, bool &isCOWArray)
{
    if (isCOWArray) {
        JSArray::CheckAndCopyArray(thread, holderHandler);
        isCOWArray = false;
    }
    return holderHandler->GetElements();
}

JSTaggedValue RuntimeStubs::ArrayNumberSort(JSThread *thread, JSHandle<JSObject> thisObj, uint32_t len)
{
    JSMutableHandle<JSTaggedValue> presentValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> middleValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> previousValue(thread, JSTaggedValue::Undefined());
    bool isCOWArray = JSHandle<JSTaggedValue>(thisObj)->IsJSCOWArray();
    JSMutableHandle<TaggedArray> elements(thread, thisObj->GetElements());
    std::map<uint64_t, JSHandle<JSTaggedValue>> cachedString;
    for (uint32_t i = 1; i < len; i++) {
        uint32_t beginIndex = 0;
        uint32_t endIndex = i;
        presentValue.Update(elements->Get(i));
        JSHandle<JSTaggedValue> presentString = GetOrCreateNumberString(thread, presentValue, cachedString);
        while (beginIndex < endIndex) {
            uint32_t middleIndex = beginIndex + (endIndex - beginIndex) / 2; // 2 : half
            middleValue.Update(elements->Get(middleIndex));
            JSHandle<JSTaggedValue> middleString = GetOrCreateNumberString(thread, middleValue, cachedString);
            double compareResult = ArrayHelper::StringSortCompare(thread, middleString, presentString);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
            if (compareResult > 0) {
                endIndex = middleIndex;
            } else {
                beginIndex = middleIndex + 1;
            }
        }
        if (endIndex >= 0 && endIndex < i) {
            for (uint32_t j = i; j > endIndex; j--) {
                previousValue.Update(elements->Get(j - 1));
                elements.Update(TryCopyCOWArray(thread, JSHandle<JSArray>(thisObj), isCOWArray));
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
                elements->Set(thread, j, previousValue);
            }
            elements.Update(TryCopyCOWArray(thread, JSHandle<JSArray>(thisObj), isCOWArray));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
            elements->Set(thread, endIndex, presentValue);
        }
    }
    return thisObj.GetTaggedValue();
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_STUBS_RUNTIME_STUBS_INL_H
