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

#include "ecmascript/builtins/builtins_json.h"

#include "ecmascript/base/fast_json_stringifier.h"
#include "ecmascript/base/senable_fast_json_stringifier.h"
#include "ecmascript/base/json_helper.h"
#include "ecmascript/base/json_parser.h"
#include "ecmascript/base/json_stringifier.h"
#include "ecmascript/base/senable_json_stringifier.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/base/sendable_json_parse.h"

namespace panda::ecmascript::builtins {
namespace {
using TransformType = base::JsonHelper::TransformType;

void InitWithTransformType(JSHandle<GlobalEnv> &env, TransformType transformType,
                           JSMutableHandle<JSFunction> &constructor, SCheckMode &sCheckMode)
{
    if (transformType == TransformType::NORMAL) {
        sCheckMode = SCheckMode::CHECK;
        constructor.Update(env->GetObjectFunction());
    } else {
        sCheckMode = SCheckMode::SKIP;
        constructor.Update(env->GetSObjectFunction());
    }
}
}  // namespace

using Internalize = base::Internalize;

JSTaggedValue BuiltinsJson::Parse(EcmaRuntimeCallInfo *argv)
{
    return ParseWithTransformType(argv, TransformType::NORMAL);
}

JSTaggedValue BuiltinsSendableJson::Parse(EcmaRuntimeCallInfo *argv)
{
    return BuiltinsJson::ParseWithTransformType(argv, TransformType::SENDABLE);
}

// 24.5.1
JSTaggedValue BuiltinsJson::ParseWithTransformType(EcmaRuntimeCallInfo *argv, TransformType transformType)
{
    BUILTINS_API_TRACE(argv->GetThread(), Json, Parse);
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    uint32_t argc = argv->GetArgsNumber();
    if (argc == 0) {
        JSHandle<JSObject> syntaxError = factory->GetJSError(base::ErrorType::SYNTAX_ERROR, "arg is empty");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, syntaxError.GetTaggedValue(), JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSMutableHandle<JSTaggedValue> reviverVal(thread, JSTaggedValue::Undefined());
    if (argc == 2) {  // 2: 2 args
        reviverVal.Update(GetCallArg(argv, 1));
    }
    return ParseWithTransformType(thread->GetEcmaVM(), msg, reviverVal, transformType);
}

JSTaggedValue BuiltinsJson::ParseWithTransformType(const EcmaVM *vm, JSHandle<JSTaggedValue> &msg,
                                                   JSHandle<JSTaggedValue> &reviverVal, TransformType transformType)
{
    JSThread *thread = vm->GetJSThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> parseString = JSTaggedValue::ToString(thread, msg);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> result;
    if (transformType ==  TransformType::SENDABLE) {
        if (EcmaStringAccessor(parseString).IsUtf8()) {
            panda::ecmascript::base::Utf8SendableJsonParser parser(thread, transformType);
            result = parser.Parse(parseString);
        } else {
            panda::ecmascript::base::Utf16SendableJsonParser parser(thread, transformType);
            result = parser.Parse(*parseString);
        }
    } else {
        if (EcmaStringAccessor(parseString).IsUtf8()) {
            panda::ecmascript::base::Utf8JsonParser parser(thread, transformType);
            result = parser.Parse(parseString);
        } else {
            panda::ecmascript::base::Utf16JsonParser parser(thread, transformType);
            result = parser.Parse(*parseString);
        }
    }

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedValue reviver = JSTaggedValue::Undefined();
    if (reviverVal->IsJSFunction()) {
        reviver = reviverVal.GetTaggedValue();
        if (reviver.IsCallable()) {
            JSHandle<JSTaggedValue> callbackfnHandle(thread, reviver);
            // Let root be ! OrdinaryObjectCreate(%Object.prototype%).
            JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
            JSMutableHandle<JSFunction> constructor(thread, JSTaggedValue::Undefined());
            SCheckMode sCheckMode = SCheckMode::CHECK;
            InitWithTransformType(env, transformType, constructor, sCheckMode);
            JSHandle<JSObject> root = factory->NewJSObjectByConstructor(constructor);
            // Let rootName be the empty String.
            JSHandle<JSTaggedValue> rootName(factory->GetEmptyString());
            // Perform ! CreateDataPropertyOrThrow(root, rootName, unfiltered).
            bool success = JSObject::CreateDataProperty(thread, root, rootName, result, sCheckMode);
            if (success) {
                result = Internalize::InternalizeJsonProperty(thread, root, rootName, callbackfnHandle, transformType);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
        }
    }
    if (transformType == TransformType::SENDABLE) {
        if (result->IsHeapObject() && !result->IsJSShared() && !result->IsString()) {
            THROW_TYPE_ERROR_AND_RETURN(thread, GET_MESSAGE_STRING(ClassNotDerivedFromShared),
                                        JSTaggedValue::Exception());
        }
    }
    return result.GetTaggedValue();
}

// 24.5.2
JSTaggedValue BuiltinsJson::Stringify(EcmaRuntimeCallInfo *argv)
{
    return BuiltinsJson::StringifyWithTransformType(argv, TransformType::NORMAL);
}

JSTaggedValue BuiltinsSendableJson::Stringify(EcmaRuntimeCallInfo *argv)
{
    return BuiltinsJson::StringifyWithTransformType(argv, TransformType::SENDABLE);
}

JSTaggedValue BuiltinsJson::StringifyWithTransformType(EcmaRuntimeCallInfo *argv, TransformType transformType)
{
    BUILTINS_API_TRACE(argv->GetThread(), Json, Stringify);
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    uint32_t argc = argv->GetArgsNumber();
    JSTaggedValue value = GetCallArg(argv, 0).GetTaggedValue();
    if (value.IsJSSharedJSONValue()) {
        value = JSSharedJSONValue::Cast(value)->GetValue();
    }
    if (transformType == TransformType::SENDABLE) {
        if (value.IsHeapObject() && !value.IsJSShared() && !value.IsString()) {
            THROW_TYPE_ERROR_AND_RETURN(thread, GET_MESSAGE_STRING(StringifySendableObject),
                                        JSTaggedValue::Exception());
        }
    }
    if (argc == 1 && thread->GetCurrentEcmaContext()->IsAotEntry()) {
        JSHandle<JSTaggedValue> handleValue(thread, value);
        JSHandle<JSTaggedValue> result;
        if (transformType == TransformType::SENDABLE) {
            panda::ecmascript::base::SendableFastJsonStringifier stringifier(thread);
            result = stringifier.Stringify(handleValue);
        } else {
            panda::ecmascript::base::FastJsonStringifier stringifier(thread);
            result = stringifier.Stringify(handleValue);
        }
        return result.GetTaggedValue();
    }
    JSTaggedValue replacer = JSTaggedValue::Undefined();
    JSTaggedValue gap = JSTaggedValue::Undefined();

    if (argc == 2) {  // 2: 2 args
        replacer = GetCallArg(argv, 1).GetTaggedValue();
    } else if (argc == 3) {  // 3: 3 args
        replacer = GetCallArg(argv, 1).GetTaggedValue();
        gap = GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD).GetTaggedValue();
    }

    JSHandle<JSTaggedValue> handleValue(thread, value);
    JSHandle<JSTaggedValue> handleReplacer(thread, replacer);
    JSHandle<JSTaggedValue> handleGap(thread, gap);
    JSHandle<JSTaggedValue> result;
    if (transformType == TransformType::SENDABLE) {
        panda::ecmascript::base::SendableJsonStringifier stringifier(thread);
        result = stringifier.Stringify(handleValue, handleReplacer, handleGap);
    } else {
        panda::ecmascript::base::JsonStringifier stringifier(thread);
        result = stringifier.Stringify(handleValue, handleReplacer, handleGap);
    }

    return result.GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins
