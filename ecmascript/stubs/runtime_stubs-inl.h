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

#ifndef ECMASCRIPT_STUBS_RUNTIME_STUBS_INL_H
#define ECMASCRIPT_STUBS_RUNTIME_STUBS_INL_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/stubs/runtime_stubs.h"

#include "ecmascript/base/array_helper.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/debugger/js_debugger_manager.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/js_async_function.h"
#include "ecmascript/js_async_generator_object.h"
#include "ecmascript/js_bigint.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/jspandafile/class_info_extractor.h"
#include "ecmascript/jspandafile/literal_data_extractor.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/jspandafile/scope_info_extractor.h"
#include "ecmascript/message_string.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/module_value_accessor.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/patch/quick_fix_helper.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/runtime.h"
#include "ecmascript/stackmap/llvm/llvm_stackmap_parser.h"
#include "ecmascript/sendable_env.h"
#include "ecmascript/template_string.h"

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
    if ((std::abs(doubleBase) == 1 && std::isinf(doubleExponent)) || std::isnan(doubleExponent)) {
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
    JSTaggedValue res = JSForInIterator::NextInternal(thread, JSHandle<JSForInIterator>::Cast(iter));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res;
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
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
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
    if (!superFunc->IsJSFunction()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Super constructor is not JSFunction", JSTaggedValue::Exception());
    }

    JSHandle<TaggedArray> argv(thread, RuntimeGetCallSpreadArgs(thread, array));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
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

JSTaggedValue RuntimeStubs::RuntimeOptSuperCallSpread(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                                      const JSHandle<JSTaggedValue> &newTarget,
                                                      const JSHandle<JSTaggedValue> &taggedArray)
{
    JSHandle<JSTaggedValue> superFunc(thread, JSTaggedValue::GetPrototype(thread, func));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!superFunc->IsJSFunction()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Super constructor is not JSFunction", JSTaggedValue::Exception());
    }

    JSHandle<TaggedArray> argv(thread, taggedArray.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
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

JSTaggedValue RuntimeStubs::RuntimeSuperCallForwardAllArgs(JSThread *thread, JSTaggedType *sp,
                                                           const JSHandle<JSTaggedValue> &superFunc,
                                                           const JSHandle<JSTaggedValue> &newTarget,
                                                           uint32_t restNumArgs, uint32_t startIdx)
{
    if (!superFunc->IsConstructor()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Super constructor is not a constructor", JSTaggedValue::Exception());
    }

    // prepare callinfo
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, superFunc, undefined, newTarget, restNumArgs);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // set args for callinfo
    for (uint32_t i = 0; i < restNumArgs; ++i) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        info->SetCallArg(i, JSTaggedValue(sp[startIdx + i]));
    }
    // execute super ctor with call info
    JSTaggedValue res = JSFunction::Construct(info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res;
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
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
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

    if (thread->GetEcmaVM()->GetJSOptions().EnablePendingCheak()) {
        thread->GetEcmaVM()->RemoveAsyncStackTrace(promise);
    }
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
                desc.SetWritable(true);
                desc.SetConfigurable(true);
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
    JSHandle<JSTaggedValue> undefinedHandle(thread, JSTaggedValue::Undefined());
    ASSERT(dst->IsJSArray());
    if (dst->IsJSArray()) {
        if (src->IsNull() || src->IsUndefined()) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "src is not iterable", JSTaggedValue::Exception());
        }
    } else {
        THROW_TYPE_ERROR_AND_RETURN(thread, "dst is not iterable", JSTaggedValue::Exception());
    }
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
        JSHandle<JSTaggedValue> length(thread, JSTaggedValue(dstLen + strLen));
        if (strLen == 0U) {
            JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
            JSTaggedValue::SetProperty(thread, dst, lengthKey, length);
        }
        return length.GetTaggedValue();
    }

    if (index.GetInt() == 0 && src->IsStableJSArray(thread)) {
        JSHandle<TaggedArray> srcElements(thread, JSHandle<JSObject>::Cast(src)->GetElements());
        uint32_t length = JSHandle<JSArray>::Cast(src)->GetArrayLength();
        JSHandle<TaggedArray> dstElements = factory->NewTaggedArray(length);
        JSHandle<JSArray> dstArray = JSHandle<JSArray>::Cast(dst);
        dstArray->SetElements(thread, dstElements);
        dstArray->SetArrayLength(thread, length);
        JSHandle<JSObject> srcObj(src);
        JSHandle<JSObject> dstObj(dst);
        ElementAccessor::CopyJSArrayObject(thread, srcObj, dstObj, length);
        for (uint32_t i = 0; i < length; i++) {
            JSHandle<JSTaggedValue> reg(thread, ElementAccessor::Get(thread, srcObj, i));
            if (reg->IsHole()) {
                JSHandle<JSTaggedValue> reg2(thread, JSArray::FastGetPropertyByValue(thread, src, i).GetTaggedValue());
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                if (reg2->IsHole()) {
                    ElementAccessor::Set(thread, dstObj, i, undefinedHandle, true);
                } else {
                    ElementAccessor::Set(thread, dstObj, i, reg2, true);
                }
            } else {
                ElementAccessor::Set(thread, dstObj, i, reg, true);
            }
        }
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
    uint32_t srcLen = 0U;
    do {
        iterResult = JSIterator::IteratorStep(thread, iter);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (iterResult->IsFalse()) {
            break;
        }
        bool success = JSTaggedValue::GetOwnProperty(thread, iterResult, valueStr, desc);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (success && desc.IsEnumerable()) {
            JSTaggedValue::DefineOwnProperty(thread, dst, indexHandle, desc);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            int tmp = indexHandle->GetInt();
            indexHandle.Update(JSTaggedValue(tmp + 1));
            ++srcLen;
        }
    } while (true);
    if (srcLen == 0U) {
        JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
        JSTaggedValue::SetProperty(thread, dst, lengthKey, indexHandle);
    }
    return indexHandle.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeGetIteratorNext(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                   const JSHandle<JSTaggedValue> &method)
{
    ASSERT(method->IsCallable());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, method, obj, undefined, 0);
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
    if (UNLIKELY(proto->IsJSShared())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, GET_MESSAGE_STRING(SetProtoWithSendable), JSTaggedValue::Exception());
    }
    JSObject::SetPrototype(thread, obj, proto, true);
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
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
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
    JSHandle<JSTaggedValue> superBase(thread, JSTaggedValue::GetSuperBase(thread, homeObject));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedValue::RequireObjectCoercible(thread, superBase);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> propKey(JSTaggedValue::ToPropertyKey(thread, key));
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
    JSHandle<JSTaggedValue> superBase(thread, JSTaggedValue::GetSuperBase(thread, homeObject));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedValue::RequireObjectCoercible(thread, superBase);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> propKey(JSTaggedValue::ToPropertyKey(thread, key));
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
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    if (thread->GetEcmaVM()->GetJSOptions().IsEnableLoweringBuiltin()) {
        BuiltinIndex& builtinIndex = BuiltinIndex::GetInstance();
        auto index = builtinIndex.GetBuiltinIndex(prop.GetTaggedValue());
        if (index != BuiltinIndex::NOT_FOUND) {
            auto box = factory->NewPropertyBox(JSHandle<JSTaggedValue>(thread, JSTaggedValue::Hole()));
            thread->GetBuiltinEntriesPointer()->ClearByIndex(index, box.GetTaggedValue());
        }
    }

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
    ASSERT(literalBufferLength > 0);

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
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread, cloneClass, cloneClassPrototype.GetTaggedValue());
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

bool RuntimeStubs::ShouldUseAOTHClass(const JSHandle<JSTaggedValue> &ihc,
                                      const JSHandle<JSTaggedValue> &chc,
                                      const JSHandle<ClassLiteral> &classLiteral)
{
    // In the case of incomplete data collection in PGO, AOT may not create ihc and chc at the same time.
    // Therefore, there is no need to check for the existence of both at this point.
    return (!ihc->IsUndefined() || !chc->IsUndefined()) && !classLiteral->GetIsAOTUsed();
}
// clone class may need re-set inheritance relationship due to extends may be a variable.
JSTaggedValue RuntimeStubs::RuntimeCreateClassWithBuffer(JSThread *thread,
                                                         const JSHandle<JSTaggedValue> &base,
                                                         const JSHandle<JSTaggedValue> &lexenv,
                                                         const JSHandle<JSTaggedValue> &constpool,
                                                         uint16_t methodId, uint16_t literalId,
                                                         const JSHandle<JSTaggedValue> &module,
                                                         const JSHandle<JSTaggedValue> &length)
{
    if (base->IsJSShared()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, GET_MESSAGE_STRING(NotSendableSubClass), JSTaggedValue::Exception());
    }
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    CString entry = ModuleManager::GetRecordName(module.GetTaggedValue());

    // For class constructor.
    auto methodObj = ConstantPool::GetMethodFromCache(thread, constpool.GetTaggedValue(), methodId);
    JSHandle<JSTaggedValue> method(thread, methodObj);
    JSHandle<ConstantPool> constpoolHandle = JSHandle<ConstantPool>::Cast(constpool);
    JSHandle<JSFunction> cls;
    JSMutableHandle<JSTaggedValue> ihc(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> chc(thread, JSTaggedValue::Undefined());

    JSHandle<ConstantPool> cp(thread,
        thread->GetEcmaVM()->FindOrCreateUnsharedConstpool(constpoolHandle.GetTaggedValue()));
    JSTaggedValue val = cp->GetObjectFromCache(literalId);
    if (val.IsAOTLiteralInfo()) {
        JSHandle<AOTLiteralInfo> aotLiteralInfo(thread, val);
        ihc.Update(aotLiteralInfo->GetIhc());
        chc.Update(aotLiteralInfo->GetChc());
    }

    JSTaggedValue literalObj = ConstantPool::GetClassLiteralFromCache(thread, cp, literalId, entry);

    JSHandle<ClassLiteral> classLiteral(thread, literalObj);
    JSHandle<TaggedArray> arrayHandle(thread, classLiteral->GetArray());
    JSHandle<ClassInfoExtractor> extractor = factory->NewClassInfoExtractor(method);
    auto literalLength = arrayHandle->GetLength();
    ClassInfoExtractor::BuildClassInfoExtractorFromLiteral(thread, extractor, arrayHandle, literalLength);

    if (ShouldUseAOTHClass(ihc, chc, classLiteral)) {
        classLiteral->SetIsAOTUsed(true);
        cls = ClassHelper::DefineClassWithIHClass(thread, base, extractor, lexenv, ihc, chc);
    } else {
        cls = ClassHelper::DefineClassFromExtractor(thread, base, extractor, lexenv);
    }

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    RuntimeSetClassInheritanceRelationship(thread, JSHandle<JSTaggedValue>(cls), base);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    cls->SetLexicalEnv(thread, lexenv.GetTaggedValue());
    cls->SetModule(thread, module.GetTaggedValue());
    RuntimeSetClassConstructorLength(thread, cls.GetTaggedValue(), length.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return cls.GetTaggedValue();
}

void RuntimeStubs::SetProfileTypeInfoCellToFunction(JSThread *thread, const JSHandle<JSFunction> &jsFunc,
                                                    const JSHandle<JSFunction> &definedFunc, uint16_t slotId)
{
    if (slotId == ProfileTypeInfo::INVALID_SLOT_INDEX) {  // do nothing for invalid slotId
        return;
    }

    ASSERT(definedFunc->IsJSFunction());
    ASSERT(!definedFunc->IsSharedFunction());
    ASSERT(jsFunc->IsJSFunction());
    ASSERT(!jsFunc->IsSharedFunction());

    auto profileTypeInfo = jsFunc->GetProfileTypeInfo();
    if (profileTypeInfo.IsUndefined()) {
        JSHandle<Method> method(thread, jsFunc->GetMethod());
        uint32_t slotSize = method->GetSlotSize();
        profileTypeInfo = RuntimeNotifyInlineCache(thread, jsFunc, slotSize);
    }

    ASSERT(!profileTypeInfo.IsUndefined());

    JSHandle<ProfileTypeInfo> profileTypeArray(thread, profileTypeInfo);
    JSTaggedValue slotValue = profileTypeArray->Get(slotId);
    if (slotValue.IsUndefined()) {
        JSHandle<JSTaggedValue> handleUndefined(thread, JSTaggedValue::Undefined());
        JSHandle<ProfileTypeInfoCell> newProfileTypeInfoCell =
            thread->GetEcmaVM()->GetFactory()->NewProfileTypeInfoCell(handleUndefined);
        profileTypeArray->Set(thread, slotId, newProfileTypeInfoCell);
        definedFunc->SetRawProfileTypeInfo(thread, newProfileTypeInfoCell);
    } else {
        auto cellPtr = ProfileTypeInfoCell::Cast(slotValue.GetTaggedObject());
        cellPtr->UpdateProfileTypeInfoCellType(thread);
        definedFunc->SetRawProfileTypeInfo(thread, slotValue);
    }
}

JSTaggedValue RuntimeStubs::RuntimeCreateSharedClass(JSThread *thread,
                                                     const JSHandle<JSTaggedValue> &base,
                                                     const JSHandle<JSTaggedValue> &constpool,
                                                     uint16_t methodId, uint16_t literalId,
                                                     uint16_t length, const JSHandle<JSTaggedValue> &module)
{
    if (!base->IsJSShared() && !base->IsHole()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, GET_MESSAGE_STRING(ClassNotDerivedFromShared), JSTaggedValue::Exception());
    }
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    CString entry = ModuleManager::GetRecordName(module.GetTaggedValue());

    auto methodObj = ConstantPool::GetMethodFromCache(thread, constpool.GetTaggedValue(), methodId);
    JSHandle<JSTaggedValue> method(thread, methodObj);
    JSHandle<Method>(method)->SetIsSendable(true);
    JSHandle<ConstantPool> constpoolHandle = JSHandle<ConstantPool>::Cast(constpool);

    JSHandle<JSTaggedValue> sendableEnv(thread, JSTaggedValue::Undefined());
    if (module->IsSourceTextModule()) {
        JSHandle<SourceTextModule> moduleRecord = JSHandle<SourceTextModule>::Cast(module);
        sendableEnv = JSHandle<JSTaggedValue>(thread, moduleRecord->GetSendableEnv());
    }
    auto literalObj = ConstantPool::GetClassLiteralFromCache(
        thread, constpoolHandle, literalId, entry, sendableEnv, ClassKind::SENDABLE);
    JSHandle<ClassLiteral> classLiteral(thread, literalObj);
    JSHandle<TaggedArray> arrayHandle(thread, classLiteral->GetArray());
    auto literalLength = arrayHandle->GetLength();
    ASSERT(literalLength > 0);
    // fieldTypeId is the last element in literal buffer
    auto fieldTypeId = static_cast<uint32_t>(arrayHandle->Get(literalLength - 1).GetInt());
    // Don't trim array, because define class maybe called on muilt-time in the same vm or diferrent vm
    JSHandle<ClassInfoExtractor> extractor = factory->NewClassInfoExtractor(method);
    ClassInfoExtractor::BuildClassInfoExtractorFromLiteral(thread, extractor, arrayHandle,
                                                           literalLength - 1, ClassKind::SENDABLE);

    JSHandle<TaggedArray> fieldTypeArray = ConstantPool::GetFieldLiteral(thread, constpoolHandle, fieldTypeId, entry);
    JSHandle<TaggedArray> staticFieldArray = SendableClassDefiner::ExtractStaticFieldTypeArray(thread, fieldTypeArray);
    JSHandle<JSFunction> cls =
        SendableClassDefiner::DefineSendableClassFromExtractor(thread, extractor, staticFieldArray);
    ModuleManager *moduleManager = thread->GetModuleManager();
    JSHandle<JSTaggedValue> sendableClsModule = moduleManager->GenerateSendableFuncModule(module);
    if (sendableClsModule->IsSourceTextModule()) {
        JSHandle<SourceTextModule> sendableClsModuleRecord(sendableClsModule);
        sendableClsModuleRecord->SetSendableEnv(thread, sendableEnv);
    }
    cls->SetModule(thread, sendableClsModule.GetTaggedValue());
    RuntimeSetClassConstructorLength(thread, cls.GetTaggedValue(), JSTaggedValue(length));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    RuntimeSetClassInheritanceRelationship(thread, JSHandle<JSTaggedValue>(cls), base, ClassKind::SENDABLE);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    uint32_t arrayLength = fieldTypeArray->GetLength();
    ASSERT(arrayLength > 0);
    auto instanceFieldNums = static_cast<uint32_t>(fieldTypeArray->Get(arrayLength - 1).GetInt());
    // Don't trim array, because define class maybe called on muilt-time in the same vm or diferrent vm
    uint32_t instanceLength = instanceFieldNums * 2;  // 2: key and value
    ASSERT(instanceLength < arrayLength);
    SendableClassDefiner::DefineSendableInstanceHClass(thread, fieldTypeArray, instanceLength, cls, base);
    return cls.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeSetClassInheritanceRelationship(JSThread *thread,
                                                                   const JSHandle<JSTaggedValue> &ctor,
                                                                   const JSHandle<JSTaggedValue> &base,
                                                                   ClassKind kind)
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
        parentPrototype =
            (kind == ClassKind::SENDABLE) ? env->GetSObjectFunctionPrototype() : env->GetObjectFunctionPrototype();
        parent = (kind == ClassKind::SENDABLE) ? env->GetSFunctionPrototype() : env->GetFunctionPrototype();
    } else if (parent->IsNull()) {
        method->SetFunctionKind(FunctionKind::DERIVED_CONSTRUCTOR);
        parentPrototype = JSHandle<JSTaggedValue>(thread, JSTaggedValue::Null());
        parent = (kind == ClassKind::SENDABLE) ? env->GetSFunctionPrototype() : env->GetFunctionPrototype();
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

    ctor->GetTaggedObject()->GetClass()->SetPrototype(thread, parent); // __proto__

    JSHandle<JSObject> clsPrototype(thread, JSHandle<JSFunction>(ctor)->GetFunctionPrototype());
    clsPrototype->GetClass()->SetPrototype(thread, parentPrototype);

    // ctor -> hclass -> EnableProtoChangeMarker
    auto constructor = JSFunction::Cast(ctor.GetTaggedValue().GetTaggedObject());
    if (constructor->GetClass()->IsAOT()) {
        JSHClass::EnableProtoChangeMarker(thread, JSHandle<JSHClass>(thread, constructor->GetClass()));
        // prototype -> hclass -> EnableProtoChangeMarker
        JSHClass::EnableProtoChangeMarker(thread,
            JSHandle<JSHClass>(thread, constructor->GetFunctionPrototype().GetTaggedObject()->GetClass()));
    }

    // by enableing the ProtoChangeMarker, the IHC generated in the Aot stage
    // is registered into the listener of its prototype. In this way, it is ensured
    // that when the prototype changes, the affected IHC can be notified.
    JSTaggedValue protoOrHClass = JSHandle<JSFunction>(ctor)->GetProtoOrHClass();
    if (protoOrHClass.IsJSHClass()) {
        JSHClass *ihc = JSHClass::Cast(protoOrHClass.GetTaggedObject());
        if (ihc->IsAOT()) {
            JSHandle<JSHClass> ihcHandle(thread, ihc);
            JSHClass::EnableProtoChangeMarker(thread, ihcHandle);
        }
    } else {
        JSHandle<JSObject> protoHandle(thread, protoOrHClass);
        if (protoHandle->GetJSHClass()->IsAOT()) {
            JSHClass::EnablePHCProtoChangeMarker(thread, JSHandle<JSHClass>(thread, protoHandle->GetJSHClass()));
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

JSTaggedValue RuntimeStubs::RuntimeNotifyInlineCache(JSThread *thread, const JSHandle<JSFunction> &function,
                                                     uint32_t icSlotSize)
{
    // max ic slot index is 0xffff, the size of ic slot could extend more
    ASSERT(icSlotSize <= ProfileTypeInfo::MAX_SLOT_INDEX + MethodLiteral::EXTEND_SLOT_SIZE);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ProfileTypeInfo> profileTypeInfo;
    if (function->IsSharedFunction()) {
        return JSTaggedValue::Undefined();
    } else {
        profileTypeInfo = factory->NewProfileTypeInfo(icSlotSize);
    }
    Method* method = Method::Cast(function->GetMethod());
    auto codeSize = method->GetCodeSize();
    if (pgo::PGOProfilerManager::GetInstance()->IsBigMethod(codeSize)) {
        LOG_ECMA(DEBUG) << "method size is too large, skip pgo. code size: " << codeSize
                        << ", max code size: " << pgo::PGOProfilerManager::GetInstance()->GetMaxAotMethodSize();
        profileTypeInfo->SetBigMethodPeriodIndex();
    }
    JSFunction::SetProfileTypeInfo(thread, function, JSHandle<JSTaggedValue>::Cast(profileTypeInfo));
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
        // set TaskInfo for TaskPool
        generatorObjectHandle->SetTaskInfo(thread->GetTaskInfo());
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
    return ModuleValueAccessor::GetModuleNamespace(thread, index);
}

JSTaggedValue RuntimeStubs::RuntimeGetModuleNamespace(JSThread *thread, int32_t index,
                                                      JSTaggedValue jsFunc)
{
    return ModuleValueAccessor::GetModuleNamespace(thread, index, jsFunc);
}

JSTaggedValue RuntimeStubs::RuntimeGetModuleNamespace(JSThread *thread, JSTaggedValue localName)
{
    return DeprecatedModuleValueAccessor::GetModuleNamespace(thread, localName);
}

JSTaggedValue RuntimeStubs::RuntimeGetModuleNamespace(JSThread *thread, JSTaggedValue localName,
                                                      JSTaggedValue jsFunc)
{
    return DeprecatedModuleValueAccessor::GetModuleNamespace(thread, localName, jsFunc);
}

void RuntimeStubs::RuntimeStModuleVar(JSThread *thread, int32_t index, JSTaggedValue value)
{
    ModuleValueAccessor::StoreModuleValue(thread, index, value);
}

void RuntimeStubs::RuntimeStModuleVar(JSThread *thread, int32_t index, JSTaggedValue value,
                                      JSTaggedValue jsFunc)
{
    ModuleValueAccessor::StoreModuleValue(thread, index, value, jsFunc);
}

void RuntimeStubs::RuntimeStModuleVar(JSThread *thread, JSTaggedValue key, JSTaggedValue value)
{
    DeprecatedModuleValueAccessor::StoreModuleValue(thread, key, value);
}

void RuntimeStubs::RuntimeStModuleVar(JSThread *thread, JSTaggedValue key, JSTaggedValue value,
                                      JSTaggedValue jsFunc)
{
    DeprecatedModuleValueAccessor::StoreModuleValue(thread, key, value, jsFunc);
}

JSTaggedValue RuntimeStubs::RuntimeLdLocalModuleVar(JSThread *thread, int32_t index)
{
    return ModuleValueAccessor::GetModuleValueInner(thread, index);
}

inline JSTaggedValue RuntimeStubs::RuntimeLdLocalModuleVarWithModule(JSThread* thread, int32_t index,
    JSHandle<JSTaggedValue> moduleHdl)
{
    JSTaggedValue module = moduleHdl.GetTaggedValue();
    ModuleManager* mmgr = thread->GetModuleManager();
    if (SourceTextModule::IsSendableFunctionModule(module)) {
        const CString recordNameStr = SourceTextModule::GetModuleName(module);
        module = mmgr->HostGetImportedModule(recordNameStr).GetTaggedValue();
    }
    return SourceTextModule::Cast(module)->GetModuleValue(thread, index, false);
}

JSTaggedValue RuntimeStubs::RuntimeLdLocalModuleVar(JSThread *thread, int32_t index, JSTaggedValue jsFunc)
{
    return ModuleValueAccessor::GetModuleValueInner(thread, index, jsFunc);
}

JSTaggedValue RuntimeStubs::RuntimeLdExternalModuleVar(JSThread *thread, int32_t index)
{
    return ModuleValueAccessor::GetModuleValueOuter(thread, index);
}

inline JSTaggedValue RuntimeStubs::RuntimeLdExternalModuleVarWithModule(JSThread* thread, int32_t index,
    JSHandle<JSTaggedValue> moduleHdl)
{
    JSTaggedValue module = moduleHdl.GetTaggedValue();
    ModuleManager* mmgr = thread->GetModuleManager();
    if (SourceTextModule::IsSendableFunctionModule(module)) {
        const CString recordNameStr = SourceTextModule::GetModuleName(module);
        module = mmgr->HostGetImportedModule(recordNameStr).GetTaggedValue();
        moduleHdl = JSHandle<JSTaggedValue>(thread, module);
    }
    return ModuleValueAccessor::GetModuleValueOuter(thread, index, moduleHdl);
}

JSTaggedValue RuntimeStubs::RuntimeLdSendableExternalModuleVar(JSThread *thread, int32_t index, JSTaggedValue jsFunc)
{
    return ModuleValueAccessor::GetSendableModuleValueOuter(thread, index, jsFunc);
}

JSTaggedValue RuntimeStubs::RuntimeLdSendableLocalModuleVar(JSThread* thread, int32_t index, JSTaggedValue jsFunc)
{
    return ModuleValueAccessor::GetSendableModuleValueInner(thread, index, jsFunc);
}

JSTaggedValue RuntimeStubs::RuntimeLdExternalModuleVar(JSThread *thread, int32_t index, JSTaggedValue jsFunc)
{
    return ModuleValueAccessor::GetModuleValueOuter(thread, index, jsFunc);
}

JSTaggedValue RuntimeStubs::RuntimeLdLazyExternalModuleVar(JSThread *thread, int32_t index, JSTaggedValue jsFunc)
{
    return ModuleValueAccessor::GetLazyModuleValueOuter(thread, index, jsFunc);
}

JSTaggedValue RuntimeStubs::RuntimeLdLazySendableExternalModuleVar(
    JSThread *thread, int32_t index, JSTaggedValue jsFunc)
{
    return ModuleValueAccessor::GetLazySendableModuleValueOuter(thread, index, jsFunc);
}

JSTaggedValue RuntimeStubs::RuntimeLdModuleVar(JSThread *thread, JSTaggedValue key, bool inner)
{
    if (inner) {
        JSTaggedValue moduleValue = DeprecatedModuleValueAccessor::GetModuleValueInner(thread, key);
        return moduleValue;
    }

    return DeprecatedModuleValueAccessor::GetModuleValueOuter(thread, key);
}

JSTaggedValue RuntimeStubs::RuntimeLdModuleVar(JSThread *thread, JSTaggedValue key, bool inner,
                                               JSTaggedValue jsFunc)
{
    if (inner) {
        JSTaggedValue moduleValue = DeprecatedModuleValueAccessor::GetModuleValueInner(thread, key, jsFunc);
        return moduleValue;
    }

    return DeprecatedModuleValueAccessor::GetModuleValueOuter(thread, key, jsFunc);
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
    if (thread->GetEcmaVM()->GetJSOptions().EnablePendingCheak()) {
        thread->GetEcmaVM()->InsertAsyncStackTrace(promiseObject);
    }
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

JSTaggedValue RuntimeStubs::RuntimeLdPrivateProperty(JSThread *thread, JSTaggedValue lexicalEnv,
    uint32_t levelIndex, uint32_t slotIndex, JSTaggedValue obj)
{
    JSTaggedValue currentLexicalEnv = lexicalEnv;
    for (uint32_t i = 0; i < levelIndex; i++) {
        currentLexicalEnv = LexicalEnv::Cast(currentLexicalEnv.GetTaggedObject())->GetParentEnv();
        ASSERT(!currentLexicalEnv.IsUndefined());
    }
    JSTaggedValue key = LexicalEnv::Cast(currentLexicalEnv.GetTaggedObject())->GetProperties(slotIndex);
    // private property is invisible for proxy
    JSHandle<JSTaggedValue> handleObj(thread, obj.IsJSProxy() ? JSProxy::Cast(obj)->GetPrivateField() : obj);
    JSHandle<JSTaggedValue> handleKey(thread, key);
    if (handleKey->IsJSFunction()) {  // getter
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        // 0: getter has 0 arg
        EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, handleKey, handleObj, undefined, 0);
        JSTaggedValue resGetter = JSFunction::Call(info);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return resGetter;
    }
    PropertyDescriptor desc(thread);
    if (!JSTaggedValue::IsPropertyKey(handleKey) ||
        !JSTaggedValue::GetOwnProperty(thread, handleObj, handleKey, desc)) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "invalid or cannot find private key", JSTaggedValue::Exception());
    }
    JSTaggedValue res = desc.GetValue().GetTaggedValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res;
}

JSTaggedValue RuntimeStubs::RuntimeStPrivateProperty(JSThread *thread, JSTaggedValue lexicalEnv,
    uint32_t levelIndex, uint32_t slotIndex, JSTaggedValue obj, JSTaggedValue value)
{
    JSTaggedValue currentLexicalEnv = lexicalEnv;
    for (uint32_t i = 0; i < levelIndex; i++) {
        currentLexicalEnv = LexicalEnv::Cast(currentLexicalEnv.GetTaggedObject())->GetParentEnv();
        ASSERT(!currentLexicalEnv.IsUndefined());
    }
    JSTaggedValue key = LexicalEnv::Cast(currentLexicalEnv.GetTaggedObject())->GetProperties(slotIndex);
    // private property is invisible for proxy
    JSHandle<JSTaggedValue> handleObj(thread, obj.IsJSProxy() ? JSProxy::Cast(obj)->GetPrivateField() : obj);
    JSHandle<JSTaggedValue> handleKey(thread, key);
    JSHandle<JSTaggedValue> handleValue(thread, value);
    if (handleKey->IsJSFunction()) {  // setter
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        // 1: setter has 1 arg
        EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, handleKey, handleObj, undefined, 1);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(handleValue.GetTaggedValue());
        JSTaggedValue resSetter = JSFunction::Call(info);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return resSetter;
    }
    PropertyDescriptor desc(thread);
    if (!JSTaggedValue::IsPropertyKey(handleKey) ||
        !JSTaggedValue::GetOwnProperty(thread, handleObj, handleKey, desc)) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "invalid or cannot find private key", JSTaggedValue::Exception());
    }
    desc.SetValue(handleValue);
    bool res = JSTaggedValue::DefineOwnProperty(thread, handleObj, handleKey, desc);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue(res);
}

JSTaggedValue RuntimeStubs::RuntimeTestIn(JSThread *thread, JSTaggedValue lexicalEnv,
    uint32_t levelIndex, uint32_t slotIndex, JSTaggedValue obj)
{
    if (!obj.IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot use 'in' operator in Non-Object", JSTaggedValue::Exception());
    }
    JSTaggedValue currentLexicalEnv = lexicalEnv;
    for (uint32_t i = 0; i < levelIndex; i++) {
        currentLexicalEnv = LexicalEnv::Cast(currentLexicalEnv.GetTaggedObject())->GetParentEnv();
        ASSERT(!currentLexicalEnv.IsUndefined());
    }
    JSTaggedValue key = LexicalEnv::Cast(currentLexicalEnv.GetTaggedObject())->GetProperties(slotIndex);
    JSHandle<JSTaggedValue> handleObj(thread, obj.IsJSProxy() ? JSProxy::Cast(obj)->GetPrivateField() : obj);
    JSHandle<JSTaggedValue> handleKey(thread, key);
    bool res = JSTaggedValue::IsPropertyKey(handleKey) && JSTaggedValue::HasProperty(thread, handleObj, handleKey);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue(res);
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
    THROW_NEW_ERROR_AND_RETURN(thread,
        factory->NewJSError(base::ErrorType::TYPE_ERROR, msg, StackCheck::NO).GetTaggedValue());
}

void RuntimeStubs::RuntimeThrowDeleteSuperProperty(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> info = factory->NewFromASCII("Can not delete super property");
    JSHandle<JSObject> errorObj = factory->NewJSError(base::ErrorType::REFERENCE_ERROR, info,  StackCheck::NO);
    THROW_NEW_ERROR_AND_RETURN(thread, errorObj.GetTaggedValue());
}

void RuntimeStubs::RuntimeThrowUndefinedIfHole(JSThread *thread, const JSHandle<EcmaString> &obj)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> info(thread->GlobalConstants()->GetHandledNotInitializedString());

    JSHandle<EcmaString> msg = factory->ConcatFromString(obj, info);
    THROW_NEW_ERROR_AND_RETURN(thread, factory->NewJSError(base::ErrorType::REFERENCE_ERROR,
        msg, StackCheck::NO).GetTaggedValue());
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
    THROW_NEW_ERROR_AND_RETURN(thread, factory->NewJSError(base::ErrorType::TYPE_ERROR,
                                                           msg, StackCheck::NO).GetTaggedValue());
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
        factory->NewJSError(base::ErrorType::REFERENCE_ERROR, msg, StackCheck::NO).GetTaggedValue(),
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
    JSFunction *function = JSFunction::Cast(func->GetTaggedObject());
    const JSPandaFile *jsPandaFile = function->GetCallTarget()->GetJSPandaFile();
    ASSERT(jsPandaFile != nullptr);
    CString currentfilename = jsPandaFile->GetJSPandaFileDesc();

    JSMutableHandle<JSTaggedValue> dirPath(thread, thread->GlobalConstants()->GetUndefined());
    JSMutableHandle<JSTaggedValue> recordName(thread, thread->GlobalConstants()->GetUndefined());
    if (jsPandaFile->IsBundlePack()) {
        dirPath.Update(factory->NewFromUtf8(currentfilename).GetTaggedValue());
    } else {
        recordName.Update(factory->NewFromUtf8(function->GetRecordName()));
        dirPath.Update(factory->NewFromUtf8(currentfilename).GetTaggedValue());
    }

    // 4. Let promiseCapability be !NewPromiseCapability(%Promise%).
    JSHandle<JSTaggedValue> promiseFunc = env->GetPromiseFunction();
    JSHandle<PromiseCapability> promiseCapability = JSPromise::NewPromiseCapability(thread, promiseFunc);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<job::MicroJobQueue> job = ecmaVm->GetMicroJobQueue();

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
    return JSArray::ArrayCreateWithInit(
        thread, restNumArgs, [thread, sp, startIdx] (const JSHandle<TaggedArray> &newElements, uint32_t length) {
        for (uint32_t i = 0; i < length; ++i) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            newElements->Set(thread, i, JSTaggedValue(sp[startIdx + i]));
        }
    });
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
    JSTaggedValue currentEnv = thread->GetCurrentLexenv();
    // currentEnv is LexicalEnv/GlobalEnv for normal function, and is SFunctionEnv for SharedFunction.
    JSTaggedValue globalEnv = BaseEnv::Cast(currentEnv.GetTaggedObject())->GetGlobalEnv();
    if LIKELY(!globalEnv.IsHole()) {
        newEnv->SetGlobalEnv(thread, globalEnv);
    } else {
        newEnv->SetGlobalEnv(thread, thread->GetGlueGlobalEnv());
    }
    newEnv->SetParentEnv(thread, currentEnv);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return newEnv.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeNewSendableEnv(JSThread *thread, uint16_t numVars)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SendableEnv> newEnv = factory->NewSendableEnv(numVars);

    JSTaggedValue module = JSFunction::Cast(thread->GetCurrentFunction())->GetModule();
    JSHandle<SourceTextModule> moduleHandle(thread, module);
    newEnv->SetParentEnv(thread, moduleHandle->GetSendableEnv());
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

void RuntimeStubs::DefineFuncTryUseAOTHClass(JSThread* thread,
                                             const JSHandle<JSFunction>& func,
                                             const JSHandle<JSTaggedValue>& ihc,
                                             const JSHandle<AOTLiteralInfo>& aotLiteralInfo)
{
    FunctionKind kind = Method::Cast(func->GetMethod())->GetFunctionKind();
    // The HClass of AOT comes from .ai deserialization
    if (!ihc->IsUndefined() && kind == FunctionKind::BASE_CONSTRUCTOR) {
        ASSERT(!aotLiteralInfo.GetTaggedValue().IsHole());
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        const GlobalEnvConstants *globalConst = thread->GlobalConstants();
        func->SetProtoOrHClass(thread, ihc);

        // build inheritance
        JSHandle<JSTaggedValue> parentPrototype = env->GetObjectFunctionPrototype();
        JSHandle<JSObject> clsPrototype(thread, func->GetFunctionPrototype());
        clsPrototype->GetClass()->SetPrototype(thread, parentPrototype);

        // set "constructor" in prototype
        JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
        PropertyDescriptor descriptor(thread, JSHandle<JSTaggedValue>::Cast(func), true, false, true);
        JSObject::DefineOwnProperty(thread, clsPrototype, constructorKey, descriptor);

        // enable prototype change marker
        JSHClass::EnablePHCProtoChangeMarker(thread,
            JSHandle<JSHClass>(thread, parentPrototype->GetTaggedObject()->GetClass()));
        if (ihc->IsJSHClass()) {
            JSHClass::EnableProtoChangeMarker(thread, JSHandle<JSHClass>(ihc));
        } else {
            JSHClass::EnablePHCProtoChangeMarker(thread, JSHandle<JSHClass>(thread, clsPrototype->GetClass()));
        }
        //avoid one thread uses ihc twice or more times
        aotLiteralInfo->SetIhc(JSTaggedValue::Undefined());
    }
}

JSTaggedValue RuntimeStubs::RuntimeDefinefunc(JSThread *thread, const JSHandle<JSTaggedValue> &constpool,
                                              uint16_t methodId, const JSHandle<JSTaggedValue> &module,
                                              uint16_t length, const JSHandle<JSTaggedValue> &envHandle,
                                              const JSHandle<JSTaggedValue> &homeObject)
{
    JSHandle<ConstantPool> constpoolHandle = JSHandle<ConstantPool>::Cast(constpool);
    JSMutableHandle<JSTaggedValue> ihc(thread, JSTaggedValue::Undefined());
    JSMutableHandle<AOTLiteralInfo> aotLiteralInfo(thread, JSTaggedValue::Hole());
    //AOT ihc infos always in unshareConstpool
    //If is runing on AOT,unshareConstpool is definitely not a hole
    //So wo can skip if unshareConstpool is hole
    JSTaggedValue unsharedCp = thread->GetEcmaVM()->FindUnsharedConstpool(constpoolHandle.GetTaggedValue());
    if (!unsharedCp.IsHole()) {
        JSHandle<ConstantPool> unsharedCpHandle(thread, unsharedCp);
        JSTaggedValue val = unsharedCpHandle->GetObjectFromCache(methodId);
        if (val.IsAOTLiteralInfo()) {
            aotLiteralInfo.Update(val);
            ihc.Update(aotLiteralInfo->GetIhc());
        }
    }
    JSTaggedValue method = ConstantPool::GetMethodFromCache(thread, constpoolHandle.GetTaggedValue(), methodId);
    const JSHandle<Method> methodHandle(thread, method);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> result;
    if (methodHandle->IsSendableMethod()) {
        result = factory->NewJSSendableFunction(methodHandle);
        ModuleManager *moduleManager = thread->GetModuleManager();
        JSHandle<JSTaggedValue> sendableFuncModule = moduleManager->GenerateSendableFuncModule(module);
        if (module->IsSourceTextModule()) {
            JSHandle<SourceTextModule> sendableFuncModuleRecord(sendableFuncModule);
            sendableFuncModuleRecord->SetSendableEnv(
                thread, JSHandle<SourceTextModule>::Cast(module)->GetSendableEnv());
        }
        result->SetModule(thread, sendableFuncModule.GetTaggedValue());
        auto globalConst = thread->GlobalConstants();
        JSHandle<JSTaggedValue> emptySFunctionEnvHandle = globalConst->GetHandledEmptySFunctionEnv();
        result->SetLexicalEnv(thread, emptySFunctionEnvHandle.GetTaggedValue(), SKIP_BARRIER);
    } else {
        result = factory->NewJSFunction(methodHandle);
        result->SetModule(thread, module.GetTaggedValue());
        result->SetLexicalEnv(thread, envHandle.GetTaggedValue());
        result->SetHomeObject(thread, homeObject.GetTaggedValue());
    }
    DefineFuncTryUseAOTHClass(thread, result, ihc, aotLiteralInfo);

    result->SetLength(length);
    return result.GetTaggedValue();
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

JSTaggedValue RuntimeStubs::RuntimeOptCreateObjectWithExcludedKeys(JSThread *thread, uintptr_t argv, uint32_t argc)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const JSHandle<JSTaggedValue> &objVal = GetHArg<JSTaggedValue>(argv, argc, 0); // 0: means the objVal
    uint32_t firstArgRegIdx = 1; // 1: means the firstArgRegIdx
    uint32_t numExcludedKeys = 0;
    uint32_t numKeys = argc - 1;
    JSHandle<TaggedArray> excludedKeys = factory->NewTaggedArray(numKeys);
    JSTaggedValue excludedKey = GetArg(argv, argc, firstArgRegIdx);
    if (!excludedKey.IsUndefined()) {
        numExcludedKeys = numKeys;
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

    JSHandle<JSTaggedValue> finalVal = objVal;
    if (finalVal->CheckIsJSProxy()) {
        JSHandle<JSProxy> proxyVal(thread, finalVal.GetTaggedValue());
        finalVal = proxyVal->GetSourceTarget(thread);
    }

    return CommonCreateObjectWithExcludedKeys(thread, finalVal, numExcludedKeys, excludedKeys);
}

JSTaggedValue RuntimeStubs::RuntimeDefineMethod(JSThread *thread, const JSHandle<Method> &methodHandle,
                                                const JSHandle<JSTaggedValue> &homeObject, uint16_t length,
                                                const JSHandle<JSTaggedValue> &env,
                                                const JSHandle<JSTaggedValue> &module)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> func = factory->NewJSFunction(methodHandle, homeObject);
    func->SetLength(length);
    func->SetLexicalEnv(thread, env);
    func->SetModule(thread, module);
    return func.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeLdSendableClass(const JSHandle<JSTaggedValue> &env, uint16_t level)
{
    JSTaggedValue currentEnv(env.GetTaggedValue());
    for (uint32_t i = 0; i < level; i++) {
        ASSERT(currentEnv.IsLexicalEnv());
        currentEnv = LexicalEnv::Cast(currentEnv.GetTaggedObject())->GetParentEnv();
    }
    ASSERT(currentEnv.IsSFunctionEnv());
    return SFunctionEnv::Cast(currentEnv.GetTaggedObject())->GetConstructor();
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

void RuntimeStubs::RuntimeSetPatchModule(JSThread *thread, const JSHandle<JSFunction> &func)
{
    JSHandle<Method> methodHandle(thread, Method::Cast(func->GetMethod()));
    const JSHandle<JSTaggedValue> coldReloadRecordName =
            thread->GetEcmaVM()->FindPatchModule(MethodLiteral::GetRecordName(
                methodHandle->GetJSPandaFile(), methodHandle->GetMethodId()));
    if (!coldReloadRecordName->IsHole()) {
        func->SetModule(thread, coldReloadRecordName.GetTaggedValue());
    }
}

JSTaggedValue RuntimeStubs::RuntimeDefineGetterSetterByValue(JSThread *thread, const JSHandle<JSObject> &obj,
                                                             const JSHandle<JSTaggedValue> &prop,
                                                             const JSHandle<JSTaggedValue> &getter,
                                                             const JSHandle<JSTaggedValue> &setter, bool flag,
                                                             const JSHandle<JSTaggedValue> &func,
                                                             int32_t pcOffset)
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

    auto receiverHClass = obj->GetJSHClass();

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
    auto holderTraHClass = obj->GetJSHClass();
    if (receiverHClass != holderTraHClass) {
        if (holderTraHClass->IsAOT()) {
            JSHandle<JSHClass> phcHandle(thread, holderTraHClass);
            JSHClass::EnablePHCProtoChangeMarker(thread, phcHandle);
        }
        if (thread->GetEcmaVM()->IsEnablePGOProfiler()) {
            if (!func->IsUndefined()) {
                thread->GetEcmaVM()->GetPGOProfiler()->ProfileDefineGetterSetter(
                    receiverHClass, holderTraHClass, func, pcOffset);
            }
        }
    }
    return obj.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeSuperCall(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                             const JSHandle<JSTaggedValue> &newTarget, uint16_t firstVRegIdx,
                                             uint16_t length)
{
    JSHandle<JSTaggedValue> superFunc(thread, JSTaggedValue::GetPrototype(thread, func));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

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

JSTaggedValue RuntimeStubs::RuntimeOptSuperCall(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                                const JSHandle<JSTaggedValue> &newTarget,
                                                const JSHandle<TaggedArray> &argv,
                                                uint16_t length)
{
    JSHandle<JSTaggedValue> superFunc(thread, JSTaggedValue::GetPrototype(thread, func));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, superFunc, undefined, newTarget, length);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    info->SetCallArg(length, argv);
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
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // Fast path when array is stablearray and Iterator not change.
    if (jsArray->IsStableJSArray(thread) && itor->IsJSArrayIterator()) {
        JSHandle<JSObject> jsArrayObj(jsArray);
        ElementAccessor::CopyJSArrayToTaggedArray(thread, jsArrayObj, argv, argvMayMaxLength);
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

JSTaggedValue RuntimeStubs::RuntimeCallBigIntAsIntN(JSThread *thread, JSTaggedValue bits, JSTaggedValue bigint)
{
    auto biginteger = JSHandle<BigInt>(thread, bigint);
    JSTaggedNumber bitness = JSTaggedValue::ToNumber(thread, bits);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return BigInt::AsintN(thread, bitness, biginteger);
}

JSTaggedValue RuntimeStubs::RuntimeCallBigIntAsUintN(JSThread *thread, JSTaggedValue bits, JSTaggedValue bigint)
{
    auto biginteger = JSHandle<BigInt>(thread, bigint);
    JSTaggedNumber bitness = JSTaggedValue::ToNumber(thread, bits);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return BigInt::AsUintN(thread, bitness, biginteger);
}

JSTaggedValue RuntimeStubs::RuntimeNewLexicalEnvWithName(JSThread *thread, uint16_t numVars, uint16_t scopeId)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<LexicalEnv> newEnv = factory->NewLexicalEnv(numVars);
    JSTaggedValue currentEnv = thread->GetCurrentLexenv();
    // currentEnv is LexicalEnv/GlobalEnv for normal function, and is SFunctionEnv for SharedFunction.
    JSTaggedValue globalEnv = BaseEnv::Cast(currentEnv.GetTaggedObject())->GetGlobalEnv();
    if LIKELY(!globalEnv.IsHole()) {
        newEnv->SetGlobalEnv(thread, globalEnv);
    } else {
        newEnv->SetGlobalEnv(thread, thread->GetGlueGlobalEnv());
    }
    newEnv->SetParentEnv(thread, currentEnv);
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
    obj->SetPropertyInlinedPropsWithSize<JSArguments::SIZE, JSArguments::LENGTH_INLINE_PROPERTY_INDEX>(
        thread, JSTaggedValue(len));
    // 5. Let index be 0.
    // 6. Repeat while index < len,
    //    a. Let val be argumentsList[index].
    //    b. Perform CreateDataProperty(obj, ToString(index), val).
    //    c. Let index be index + 1
    obj->SetElements(thread, argumentsList.GetTaggedValue());
    // 7. Perform DefinePropertyOrThrow(obj, @@iterator, PropertyDescriptor
    // {[[Value]]:%ArrayProto_values%,
    // [[Writable]]: true, [[Enumerable]]: false, [[Configurable]]: true}).
    obj->SetPropertyInlinedPropsWithSize<JSArguments::SIZE, JSArguments::ITERATOR_INLINE_PROPERTY_INDEX>(
        thread, globalEnv->GetArrayProtoValuesFunction().GetTaggedValue());
    // 8. Perform DefinePropertyOrThrow(obj, "caller", PropertyDescriptor {[[Get]]: %ThrowTypeError%,
    // [[Set]]: %ThrowTypeError%, [[Enumerable]]: false, [[Configurable]]: false}).
    JSHandle<JSTaggedValue> accessorCaller = globalEnv->GetArgumentsCallerAccessor();
    obj->SetPropertyInlinedPropsWithSize<JSArguments::SIZE, JSArguments::CALLER_INLINE_PROPERTY_INDEX>(
        thread, accessorCaller.GetTaggedValue());
    // 9. Perform DefinePropertyOrThrow(obj, "callee", PropertyDescriptor {[[Get]]: %ThrowTypeError%,
    // [[Set]]: %ThrowTypeError%, [[Enumerable]]: false, [[Configurable]]: false}).
    JSHandle<JSTaggedValue> accessorCallee = globalEnv->GetArgumentsCalleeAccessor();
    obj->SetPropertyInlinedPropsWithSize<JSArguments::SIZE, JSArguments::CALLEE_INLINE_PROPERTY_INDEX>(
        thread, accessorCallee.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 11. Return obj
    return obj.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeOptNewLexicalEnvWithName(JSThread *thread, uint16_t numVars, uint16_t scopeId,
                                                            JSHandle<JSTaggedValue> &currentEnv,
                                                            JSHandle<JSTaggedValue> &func)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<LexicalEnv> newEnv = factory->NewLexicalEnv(numVars);
    // currentEnv is LexicalEnv/GlobalEnv for normal function, and is SFunctionEnv for SharedFunction.
    TaggedObject* obj = currentEnv.GetTaggedValue().GetTaggedObject();
    JSTaggedValue globalEnv = BaseEnv::Cast(obj)->GetGlobalEnv();
    if LIKELY(!globalEnv.IsHole()) {
        newEnv->SetGlobalEnv(thread, globalEnv);
    } else {
        newEnv->SetGlobalEnv(thread, thread->GetGlueGlobalEnv());
    }
    newEnv->SetParentEnv(thread, currentEnv.GetTaggedValue());
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
        // set TaskInfo for TaskPool
        generatorObjectHandle->SetTaskInfo(thread->GetTaskInfo());
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
    STACK_LIMIT_CHECK(thread, JSTaggedValue::Exception());
    // step 1 ~ 4 get ProxyHandler and ProxyTarget
    JSHandle<JSTaggedValue> handler(thread, ctor->GetHandler());
    if (handler->IsNull()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Constructor: handler is null", JSTaggedValue::Exception());
    }
    ASSERT(handler->IsECMAObject());
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
    JSHandle<JSArray> newArr = JSArray::CreateArrayFromList(thread, arr);

    // step 8 ~ 9 Call(trap, handler, «target, argArray, newTarget »).
    const uint32_t argsLength = 3;  // 3: «target, argArray, newTarget »
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, method, handler, undefined, argsLength);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    info->SetCallArg(target.GetTaggedValue(), newArr.GetTaggedValue(), newTgt.GetTaggedValue());
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
    if (!target->IsConstructor()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Constructor is false", JSTaggedValue::Exception());
    }

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

JSTaggedValue RuntimeStubs::GetResultValue(JSThread *thread, bool isAotMethod, JSHandle<JSFunction> ctor,
    CVector<JSTaggedType> &values, JSHandle<JSTaggedValue> newTgt, uint32_t &size, JSHandle<JSTaggedValue> obj)
{
    JSTaggedValue resultValue;
    if (isAotMethod) {
        uint32_t numArgs = ctor->GetCallTarget()->GetNumArgsWithCallField();
        bool needPushArgv = numArgs != size;
        const JSTaggedType *prevFp = thread->GetLastLeaveFrame();
        if (ctor->IsCompiledFastCall()) {
            if (needPushArgv) {
                values.reserve(numArgs + NUM_MANDATORY_JSFUNC_ARGS - 1);
                for (uint32_t i = size; i < numArgs; i++) {
                    values.emplace_back(JSTaggedValue::VALUE_UNDEFINED);
                }
                size = numArgs;
            }
            resultValue = thread->GetEcmaVM()->FastCallAot(size, values.data(), prevFp);
        } else {
            resultValue = thread->GetEcmaVM()->ExecuteAot(size, values.data(), prevFp, needPushArgv);
        }
    } else {
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, JSHandle<JSTaggedValue>(ctor), obj, newTgt, size);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(size, values.data());
        resultValue = EcmaInterpreter::Execute(info);
    }
    return resultValue;
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
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }

    uint32_t preArgsSize = preArgs->IsUndefined() ? 0 : JSHandle<TaggedArray>::Cast(preArgs)->GetLength();
    const uint32_t argsCount = args->GetLength();
    uint32_t size = preArgsSize + argsCount;
    CVector<JSTaggedType> values;
    bool isCompiledCode = ctor->IsCompiledCode();
    if (isCompiledCode) {
        if (ctor->IsCompiledFastCall()) {
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
    JSTaggedValue resultValue = RuntimeStubs::GetResultValue(thread, isCompiledCode, ctor, values, newTgt, size, obj);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 9.3.2 [[Construct]] (argumentsList, newTarget)
    if (resultValue.IsECMAObject()) {
        if (resultValue.IsJSShared()) {
            JSObject::Cast(resultValue.GetTaggedObject())->GetJSHClass()->SetExtensible(false);
        }
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
    ASSERT(argc > 0);
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
    ASSERT(it.IsAotOrJitFunctionFrame());
    if (it.IsFastJitFunctionFrame()) {
        auto optimizedJSJITFunctionFrame = it.GetFrame<FASTJITFunctionFrame>();
        return optimizedJSJITFunctionFrame->GetArgv(it);
    } else {
        auto optimizedJSFunctionFrame = it.GetFrame<OptimizedJSFunctionFrame>();
        return optimizedJSFunctionFrame->GetArgv(it);
    }
}

JSTaggedType *RuntimeStubs::GetActualArgvFromStub(JSThread *thread)
{
    JSTaggedType *current = const_cast<JSTaggedType *>(thread->GetLastLeaveFrame());
    FrameIterator it(current, thread);
    ASSERT(it.IsLeaveFrame());
    it.Advance<GCVisitedFlag::VISITED>();
    ASSERT(it.IsOptimizedFrame());
    it.Advance<GCVisitedFlag::VISITED>();
    ASSERT(it.IsAotOrJitFunctionFrame());
    if (it.IsFastJitFunctionFrame()) {
        auto optimizedJSJITFunctionFrame = it.GetFrame<FASTJITFunctionFrame>();
        return optimizedJSJITFunctionFrame->GetArgv(it);
    }
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

JSTaggedValue RuntimeStubs::RuntimeUpdateAOTHClass(JSThread *thread,
    const JSHandle<JSHClass> &oldhclass, const JSHandle<JSHClass> &newhclass, JSTaggedValue key)
{
#if ECMASCRIPT_ENABLE_IC
    JSHClass::ProcessAotHClassTransition(thread, oldhclass, newhclass, key);
#endif
    return JSTaggedValue::Undefined();
}

JSTaggedValue RuntimeStubs::RuntimeDefineField(JSThread *thread, JSTaggedValue obj,
                                               JSTaggedValue propKey, JSTaggedValue value)
{
    // Do the same thing as Object.defineProperty(obj, propKey, {value: value,
    //                                                           writable:true, enumerable: true, configurable: true})
    if (!obj.IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "DefineField: obj is not Object", JSTaggedValue::Exception());
    }
    JSHandle<JSObject> handleObj(thread, obj);
    JSHandle<JSTaggedValue> handleValue(thread, value);
    JSHandle<JSTaggedValue> handleKey = JSTaggedValue::ToPropertyKey(thread, JSHandle<JSTaggedValue>(thread, propKey));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSObject::CreateDataPropertyOrThrow(thread, handleObj, handleKey, handleValue, SCheckMode::CHECK);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return JSTaggedValue::Undefined();
}

JSTaggedValue RuntimeStubs::RuntimeCreatePrivateProperty(JSThread *thread, JSTaggedValue lexicalEnv,
    uint32_t count, JSTaggedValue constpool, uint32_t literalId, JSTaggedValue module)
{
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<LexicalEnv> handleLexicalEnv(thread, lexicalEnv);
    JSHandle<ConstantPool> handleConstpool(thread, constpool);
    JSHandle<JSTaggedValue> handleModule(thread, module);
    JSHandle<ConstantPool> unsharedConstpoolHandle(
        thread, thread->GetEcmaVM()->FindOrCreateUnsharedConstpool(constpool));
    CString entry = ModuleManager::GetRecordName(handleModule.GetTaggedValue());
    uint32_t length = handleLexicalEnv->GetLength() - LexicalEnv::RESERVED_ENV_LENGTH;
    uint32_t startIndex = 0;
    while (startIndex < length && !handleLexicalEnv->GetProperties(startIndex).IsHole()) {
        startIndex++;
    }

    JSTaggedValue aotSymbolInfo = unsharedConstpoolHandle->GetAotSymbolInfo();
    JSHandle<TaggedArray> aotSymbolInfoHandle(thread, aotSymbolInfo);
    FrameHandler frameHandler(thread);
    uint32_t abcId = frameHandler.GetAbcId();
    ASSERT(startIndex + count <= length);
    for (uint32_t i = 0; i < count; i++) {
        auto index = startIndex + i;
        uint64_t id = JSSymbol::GeneratePrivateId(abcId, literalId, index);
        JSHandle<JSSymbol> symbolHandle;
        JSTaggedValue symbol = ConstantPool::GetSymbolFromSymbolInfo(aotSymbolInfoHandle, id);
        if (ConstantPool::IsAotSymbolInfoExist(aotSymbolInfoHandle, symbol)) {
            symbolHandle = JSHandle<JSSymbol>(thread, symbol);
        } else {
            symbolHandle = factory->NewJSSymbol();
            symbolHandle->SetPrivateId(id);
            symbolHandle->SetPrivate();
        }
        handleLexicalEnv->SetProperties(thread, index, symbolHandle.GetTaggedValue());
    }

    JSTaggedValue literalObj =
        ConstantPool::GetClassLiteralFromCache(thread, unsharedConstpoolHandle, literalId, entry);
    JSHandle<ClassLiteral> classLiteral(thread, literalObj);
    JSHandle<TaggedArray> literalBuffer(thread, classLiteral->GetArray());
    uint32_t literalBufferLength = literalBuffer->GetLength();
    if (literalBufferLength == 0) {
        return JSTaggedValue::Undefined();
    }
    // instace property number is hidden in the last index of literal buffer
    uint32_t instacePropertyCount = static_cast<uint32_t>(literalBuffer->Get(literalBufferLength - 1).GetInt());
    ASSERT(startIndex + count + literalBufferLength - (instacePropertyCount == 0) <= length);

    JSMutableHandle<JSTaggedValue> literalValue(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < literalBufferLength - 1; i++) {
        literalValue.Update(literalBuffer->Get(i));
        if (LIKELY(literalValue->IsFunctionTemplate())) {
            auto literalFunc = JSHandle<FunctionTemplate>::Cast(literalValue);
            JSHandle<JSFunction> func = factory->CreateJSFunctionFromTemplate(literalFunc);
            func->SetLexicalEnv(thread, handleLexicalEnv.GetTaggedValue());
            func->SetModule(thread, handleModule.GetTaggedValue());
            literalValue.Update(func);
        }
        handleLexicalEnv->SetProperties(thread, startIndex + count + i, literalValue.GetTaggedValue());
    }
    if (instacePropertyCount > 0) {
        auto index = startIndex + count + literalBufferLength - 1;
        uint64_t id = JSSymbol::GeneratePrivateId(abcId, literalId, index);
        JSHandle<JSSymbol> symbolHandle;
        JSTaggedValue symbol = ConstantPool::GetSymbolFromSymbolInfo(aotSymbolInfoHandle, id);
        if (ConstantPool::IsAotSymbolInfoExist(aotSymbolInfoHandle, symbol)) {
            symbolHandle = JSHandle<JSSymbol>(thread, symbol);
        } else {
            symbolHandle = factory->NewPublicSymbolWithChar("method");
            symbolHandle->SetPrivateId(id);
            symbolHandle->SetPrivate();
        }
        handleLexicalEnv->SetProperties(thread, index, symbolHandle.GetTaggedValue());
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue RuntimeStubs::RuntimeDefinePrivateProperty(JSThread *thread, JSTaggedValue lexicalEnv,
    uint32_t levelIndex, uint32_t slotIndex, JSTaggedValue obj, JSTaggedValue value)
{
    JSTaggedValue currentLexicalEnv = lexicalEnv;
    for (uint32_t i = 0; i < levelIndex; i++) {
        currentLexicalEnv = LexicalEnv::Cast(currentLexicalEnv.GetTaggedObject())->GetParentEnv();
        ASSERT(!currentLexicalEnv.IsUndefined());
    }
    JSTaggedValue key = LexicalEnv::Cast(currentLexicalEnv.GetTaggedObject())->GetProperties(slotIndex);
    // private property is invisible for proxy
    JSHandle<JSTaggedValue> handleObj(thread, obj.IsJSProxy() ? JSProxy::Cast(obj)->GetPrivateField() : obj);
    JSHandle<JSTaggedValue> handleKey(thread, key);
    JSHandle<JSTaggedValue> handleValue(thread, value);
    PropertyDescriptor desc(thread);
    if (!JSTaggedValue::IsPropertyKey(handleKey) &&
        JSTaggedValue::GetOwnProperty(thread, handleObj, handleKey, desc)) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "invalid private key or already exists", JSTaggedValue::Exception());
    }
    bool extensible = handleObj->IsExtensible(thread);
    if (handleObj->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "DefinePrivateProperty obj is undefined", JSTaggedValue::Exception());
    }
    if (handleObj->IsJSShared()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "shared obj cannot use # to define private property",
                                    JSTaggedValue::Exception());
    }
    if (!extensible) {
        // private key should be always extensible
        handleObj->GetTaggedObject()->GetClass()->SetExtensible(true);
    }
    bool result = JSObject::CreateDataPropertyOrThrow(thread, JSHandle<JSObject>::Cast(handleObj),
                                                      handleKey, handleValue);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!extensible) {
        handleObj->GetTaggedObject()->GetClass()->SetExtensible(false);
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue(result);
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

JSTaggedType RuntimeStubs::RuntimeTryGetInternString(uintptr_t argGlue, const JSHandle<EcmaString> &string)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
#if ENABLE_NEXT_OPTIMIZATION
    EcmaString *str =
        thread->GetEcmaVM()->GetEcmaStringTable()->TryGetInternString(string);
#else
    EcmaString *str =
        thread->GetEcmaVM()->GetEcmaStringTable()->TryGetInternString(thread, string);
#endif
    if (str == nullptr) {
        return JSTaggedValue::Hole().GetRawData();
    }
    return JSTaggedValue::Cast(static_cast<void *>(str));
}

uint32_t RuntimeStubs::RuntimeGetBytecodePcOfstForBaseline(const JSHandle<JSFunction> &func, uintptr_t nativePc)
{
    // Compute current bytecodePc according to nativePc of returnAddress
    LOG_BASELINEJIT(DEBUG) << "nativePc address: " << std::hex << nativePc;
    const MachineCode *machineCode = MachineCode::Cast(func->GetBaselineCode().GetTaggedObject());
    const uintptr_t nativePcStart = machineCode->GetFuncAddr();
    LOG_BASELINEJIT(DEBUG) << "baselineCode nativeStart address: " << std::hex << nativePcStart;
    const Method *thisMethod = Method::Cast(func->GetMethod().GetTaggedObject());
    const uint8_t *bytecodeStart = thisMethod->GetBytecodeArray();
    const uint8_t *bytecodeEnd = bytecodeStart + thisMethod->GetCodeSize();
    LOG_BASELINEJIT(DEBUG) << "bytecodePc start: " << reinterpret_cast<uintptr_t>(bytecodeStart);
    LOG_BASELINEJIT(DEBUG) << "bytecodePc end: " << reinterpret_cast<uintptr_t>(bytecodeEnd);
    const uint8_t *offsetTableAddr = machineCode->GetStackMapOrOffsetTableAddress();
    const uint32_t offsetTableSize = machineCode->GetStackMapOrOffsetTableSize();
    uintptr_t nativePcEnd = nativePcStart;
    uint32_t pcOffsetIndex = 0;
    auto opcode = kungfu::Bytecodes::GetOpcode(bytecodeStart);
    while (nativePcEnd < nativePc && pcOffsetIndex < offsetTableSize) {
        nativePcEnd += static_cast<uintptr_t>(offsetTableAddr[pcOffsetIndex++]);
        opcode = kungfu::Bytecodes::GetOpcode(bytecodeStart);
        bytecodeStart += BytecodeInstruction::Size(opcode);
    }
    // Since the nativePc is returnAddress, we need to take the previous bytecode
    bytecodeStart -= BytecodeInstruction::Size(opcode);
    if (nativePcEnd < nativePc) {
        LOG_ECMA(FATAL) <<
            "invalid nativePc or pcOffsetTable for getting bytecode pc in baseline code, the nativePcEnd is " <<
            std::hex << nativePcEnd;
    }
    if (bytecodeStart > bytecodeEnd) {
        LOG_ECMA(FATAL) <<
            "out of bytecodeArray range for getting bytecode pc in baseline code, the bytecodePc is " <<
            reinterpret_cast<uintptr_t>(bytecodeStart);
    }
    auto bytecodePcOffset = static_cast<uint32_t>(bytecodeStart - thisMethod->GetBytecodeArray());
    LOG_BASELINEJIT(DEBUG) << "current bytecodePc offset: " << bytecodePcOffset;
    return bytecodePcOffset;
}

uintptr_t RuntimeStubs::RuntimeGetNativePcOfstForBaseline(const JSHandle<JSFunction> &func, uint64_t bytecodePos)
{
    const uint8_t *bytecodePc = reinterpret_cast<uint8_t*>(bytecodePos);
    // Compute current nativePc according to bytecodePc
    LOG_BASELINEJIT(DEBUG) << "bytecodePc address: " << std::hex << reinterpret_cast<uintptr_t>(bytecodePc);
    auto opcode = kungfu::Bytecodes::GetOpcode(bytecodePc);
    LOG_BASELINEJIT(DEBUG) << "bytecode: " << kungfu::GetEcmaOpcodeStr(opcode);
    const Method *thisMethod = Method::Cast(func->GetMethod().GetTaggedObject());
    LOG_TRACE(INFO) << "deopt to baseline method name: " << thisMethod->GetMethodName();
    const uint8_t *bytecodeStart = thisMethod->GetBytecodeArray();
    const uint8_t *bytecodeEnd = bytecodeStart + thisMethod->GetCodeSize();
    ASSERT(bytecodeStart < bytecodeEnd);
    LOG_BASELINEJIT(DEBUG) << "bytecodePc start: " << std::hex << reinterpret_cast<uintptr_t>(bytecodeStart);
    LOG_BASELINEJIT(DEBUG) << "bytecodePc end: " << std::hex << reinterpret_cast<uintptr_t>(bytecodeEnd);
    ASSERT(bytecodeEnd >= bytecodeStart && bytecodePc <= bytecodeEnd);

    const MachineCode *machineCode = MachineCode::Cast(func->GetBaselineCode().GetTaggedObject());
    const uintptr_t nativePcStart = machineCode->GetFuncAddr();
    LOG_BASELINEJIT(DEBUG) << "baselineCode nativeStart address: " << std::hex << nativePcStart;
    const uint8_t *offsetTableAddr = machineCode->GetStackMapOrOffsetTableAddress();
    const uint32_t offsetTableSize = machineCode->GetStackMapOrOffsetTableSize();
    uintptr_t nativePc = nativePcStart;
    uint32_t pcOffsetIndex = 0;
    uint8_t *bytecodetmp = const_cast<uint8_t*>(bytecodeStart);
    while (bytecodetmp < bytecodePc && pcOffsetIndex < offsetTableSize) {
        nativePc += static_cast<uintptr_t>(offsetTableAddr[pcOffsetIndex++]);
        opcode = kungfu::Bytecodes::GetOpcode(bytecodetmp);
        bytecodetmp += BytecodeInstruction::Size(opcode);
    }
    LOG_BASELINEJIT(DEBUG) << "baselineCode nativePc address: " << std::hex << nativePc;
    return nativePc;
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_STUBS_RUNTIME_STUBS_INL_H
