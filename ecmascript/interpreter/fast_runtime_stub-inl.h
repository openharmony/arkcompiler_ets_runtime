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

#ifndef ECMASCRIPT_INTERPRETER_FAST_RUNTIME_STUB_INL_H
#define ECMASCRIPT_INTERPRETER_FAST_RUNTIME_STUB_INL_H

#include "ecmascript/interpreter/fast_runtime_stub.h"

#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/object_fast_operator-inl.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CHECK_IS_ON_PROTOTYPE_CHAIN(receiver, holder) \
    if (UNLIKELY((receiver) != (holder))) {           \
        return JSTaggedValue::Hole();                 \
    }

JSTaggedValue FastRuntimeStub::FastMul(JSTaggedValue left, JSTaggedValue right)
{
    if (left.IsNumber() && right.IsNumber()) {
        return JSTaggedValue(left.GetNumber() * right.GetNumber());
    }

    return JSTaggedValue::Hole();
}

JSTaggedValue FastRuntimeStub::FastDiv(JSTaggedValue left, JSTaggedValue right)
{
    if (left.IsNumber() && right.IsNumber()) {
        double dLeft = left.IsInt() ? left.GetInt() : left.GetDouble();
        double dRight = right.IsInt() ? right.GetInt() : right.GetDouble();
        if (UNLIKELY(dRight == 0.0)) {
            if (dLeft == 0.0 || std::isnan(dLeft)) {
                return JSTaggedValue(base::NAN_VALUE);
            }
            uint64_t flagBit = ((base::bit_cast<uint64_t>(dLeft)) ^ (base::bit_cast<uint64_t>(dRight))) &
                               base::DOUBLE_SIGN_MASK;
            return JSTaggedValue(base::bit_cast<double>(
                flagBit ^ (base::bit_cast<uint64_t>(base::POSITIVE_INFINITY))));
        }
        return JSTaggedValue(dLeft / dRight);
    }
    return JSTaggedValue::Hole();
}

JSTaggedValue FastRuntimeStub::FastMod(JSTaggedValue left, JSTaggedValue right)
{
    if (right.IsInt() && left.IsInt()) {
        int iRight = right.GetInt();
        int iLeft = left.GetInt();
        if (iRight > 0 && iLeft > 0) {
            return JSTaggedValue(iLeft % iRight);
        }
    }
    if (left.IsNumber() && right.IsNumber()) {
        double dLeft = left.IsInt() ? left.GetInt() : left.GetDouble();
        double dRight = right.IsInt() ? right.GetInt() : right.GetDouble();
        if (dRight == 0.0 || std::isnan(dRight) || std::isnan(dLeft) || std::isinf(dLeft)) {
            return JSTaggedValue(base::NAN_VALUE);
        }
        if (dLeft == 0.0 || std::isinf(dRight)) {
            return JSTaggedValue(dLeft);
        }
        return JSTaggedValue(std::fmod(dLeft, dRight));
    }
    return JSTaggedValue::Hole();
}

JSTaggedValue FastRuntimeStub::FastEqual(JSTaggedValue left, JSTaggedValue right)
{
    if (left == right) {
        if (UNLIKELY(left.IsDouble())) {
            return JSTaggedValue(!std::isnan(left.GetDouble()));
        }
        return JSTaggedValue::True();
    }
    if (left.IsNumber()) {
        if (left.IsInt() && right.IsInt()) {
            return JSTaggedValue::False();
        }
    }
    if (right.IsUndefinedOrNull()) {
        if (left.IsUndefinedOrNull()) {
            return JSTaggedValue::True();
        }
        return JSTaggedValue::False();
    }
    if (left.IsUndefinedOrNull()) {
        return JSTaggedValue::False();
    }
    if (left.IsBoolean()) {
        if (right.IsSpecial()) {
            return JSTaggedValue::False();
        }
    }
    if (left.IsBigInt() && right.IsBigInt()) {
        return JSTaggedValue(BigInt::Equal(left, right));
    }
    return JSTaggedValue::Hole();
}

JSTaggedValue FastRuntimeStub::FastStrictEqual(JSTaggedValue left, JSTaggedValue right)
{
    if (left.IsNumber()) {
        if (right.IsNumber()) {
            double dLeft = left.IsInt() ? left.GetInt() : left.GetDouble();
            double dRight = right.IsInt() ? right.GetInt() : right.GetDouble();
            return JSTaggedValue::StrictNumberEquals(dLeft, dRight) ? JSTaggedValue::True() : JSTaggedValue::False();
        }
        return JSTaggedValue::False();
    }
    if (right.IsNumber()) {
        return JSTaggedValue::False();
    }
    if (left == right) {
        return JSTaggedValue::True();
    }
    if (left.IsString() && right.IsString()) {
        auto leftStr = static_cast<EcmaString *>(left.GetTaggedObject());
        auto rightStr = static_cast<EcmaString *>(right.GetTaggedObject());
        if (EcmaStringAccessor(leftStr).IsFlat() && EcmaStringAccessor(rightStr).IsFlat()) {
            return EcmaStringAccessor::StringsAreEqual(static_cast<EcmaString *>(left.GetTaggedObject()),
                                                       static_cast<EcmaString *>(right.GetTaggedObject())) ?
                JSTaggedValue::True() : JSTaggedValue::False();
        }
        return JSTaggedValue::Hole();
    }
    if (left.IsBigInt()) {
        if (right.IsBigInt()) {
            return BigInt::Equal(left, right) ? JSTaggedValue::True() : JSTaggedValue::False();
        }
        return JSTaggedValue::False();
    }
    if (right.IsBigInt()) {
        return JSTaggedValue::False();
    }
    return JSTaggedValue::False();
}

JSTaggedValue FastRuntimeStub::CallGetter(JSThread *thread, JSTaggedValue receiver, JSTaggedValue holder,
                                          JSTaggedValue value)
{
    return ObjectFastOperator::CallGetter(thread, receiver, holder, value);
}

JSTaggedValue FastRuntimeStub::CallSetter(JSThread *thread, JSTaggedValue receiver, JSTaggedValue value,
                                          JSTaggedValue accessorValue)
{
    return ObjectFastOperator::CallSetter(thread, receiver, value, accessorValue);
}

template<ObjectFastOperator::Status status>
JSTaggedValue FastRuntimeStub::GetPropertyByIndex(JSThread *thread, JSTaggedValue receiver, uint32_t index)
{
    return ObjectFastOperator::GetPropertyByIndex<status>(thread, receiver, index);
}

template<ObjectFastOperator::Status status>
JSTaggedValue FastRuntimeStub::GetPropertyByValue(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key)
{
    return ObjectFastOperator::GetPropertyByValue<status>(thread, receiver, key);
}

template<ObjectFastOperator::Status status>
JSTaggedValue FastRuntimeStub::GetPropertyByName(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key)
{
    return ObjectFastOperator::GetPropertyByName<status>(thread, receiver, key);
}

template<ObjectFastOperator::Status status>
JSTaggedValue FastRuntimeStub::SetPropertyByName(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                                 JSTaggedValue value)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    return ObjectFastOperator::SetPropertyByName<status>(thread, receiver, key, value);
}

template<ObjectFastOperator::Status status>
JSTaggedValue FastRuntimeStub::SetPropertyByIndex(JSThread *thread, JSTaggedValue receiver, uint32_t index,
                                                  JSTaggedValue value)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    return ObjectFastOperator::SetPropertyByIndex<status>(thread, receiver, index, value);
}

template<ObjectFastOperator::Status status>
JSTaggedValue FastRuntimeStub::SetPropertyByValue(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                                  JSTaggedValue value)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    return ObjectFastOperator::SetPropertyByValue<status>(thread, receiver, key, value);
}

JSTaggedValue FastRuntimeStub::GetGlobalOwnProperty(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key)
{
    JSObject *obj = JSObject::Cast(receiver);
    TaggedArray *properties = TaggedArray::Cast(obj->GetProperties().GetTaggedObject());
    GlobalDictionary *dict = GlobalDictionary::Cast(properties);
    int entry = dict->FindEntry(key);
    if (entry != -1) {
        auto value = dict->GetValue(entry);
        if (UNLIKELY(value.IsAccessor())) {
            return CallGetter(thread, receiver, receiver, value);
        }
        ASSERT(!value.IsAccessor());
        return value;
    }
    return JSTaggedValue::Hole();
}

JSTaggedValue FastRuntimeStub::FastTypeOf(JSThread *thread, JSTaggedValue obj)
{
    INTERPRETER_TRACE(thread, FastTypeOf);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    switch (obj.GetRawData()) {
        case JSTaggedValue::VALUE_TRUE:
        case JSTaggedValue::VALUE_FALSE:
            return globalConst->GetBooleanString();
        case JSTaggedValue::VALUE_NULL:
            return globalConst->GetObjectString();
        case JSTaggedValue::VALUE_UNDEFINED:
            return globalConst->GetUndefinedString();
        default:
            if (obj.IsHeapObject()) {
                if (obj.IsString()) {
                    return globalConst->GetStringString();
                }
                if (obj.IsSymbol()) {
                    return globalConst->GetSymbolString();
                }
                if (obj.IsCallable()) {
                    return globalConst->GetFunctionString();
                }
                if (obj.IsBigInt()) {
                    return globalConst->GetBigIntString();
                }
                if (obj.IsNativeModuleFailureInfo()) {
                    return globalConst->GetNativeModuleFailureInfoString();
                }
                return globalConst->GetObjectString();
            }
            if (obj.IsNumber()) {
                return globalConst->GetNumberString();
            }
    }
    return globalConst->GetUndefinedString();
}

JSTaggedValue FastRuntimeStub::NewLexicalEnv(JSThread *thread, ObjectFactory *factory, uint16_t numVars)
{
    INTERPRETER_TRACE(thread, NewLexicalEnv);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    LexicalEnv *newEnv = factory->InlineNewLexicalEnv(numVars);
    if (UNLIKELY(newEnv == nullptr)) {
        return JSTaggedValue::Hole();
    }
    JSTaggedValue currentEnv = thread->GetCurrentLexenv();
    // currentEnv is LexicalEnv/GlobalEnv for normal function, and is SFunctionEnv for SharedFunction.
    JSTaggedValue globalEnv = BaseEnv::Cast(currentEnv.GetTaggedObject())->GetGlobalEnv();
    if LIKELY(!globalEnv.IsHole()) {
        newEnv->SetGlobalEnv(thread, globalEnv);
    } else {
        newEnv->SetGlobalEnv(thread, thread->GetGlueGlobalEnv());
    }
    newEnv->SetParentEnv(thread, currentEnv);
    return JSTaggedValue(newEnv);
}

JSTaggedValue FastRuntimeStub::NewThisObject(JSThread *thread, JSTaggedValue ctor, JSTaggedValue newTarget,
                                             InterpretedFrame *state)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSFunction> ctorHandle(thread, ctor);
    JSHandle<JSTaggedValue> newTargetHandle(thread, newTarget);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(ctorHandle, newTargetHandle);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());

    Method *method = Method::Cast(ctorHandle->GetMethod().GetTaggedObject());
    state->function = ctorHandle.GetTaggedValue();
    state->constpool = method->GetConstantPool();
    state->profileTypeInfo = ctorHandle->GetProfileTypeInfo();
    state->env = ctorHandle->GetLexicalEnv();

    return obj.GetTaggedValue();
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_INTERPRETER_FAST_RUNTIME_STUB_INL_H
