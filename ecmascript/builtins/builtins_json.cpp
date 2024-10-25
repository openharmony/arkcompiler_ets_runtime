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

#include "ecmascript/base/json_parser.h"
#include "ecmascript/base/json_stringifier.h"

namespace panda::ecmascript::builtins {
namespace {
using TransformType = base::JsonHelper::TransformType;
using ParseOptions =  base::JsonHelper::ParseOptions;
using ParseReturnType = base::JsonHelper::ParseReturnType;
using BigIntMode = base::JsonHelper::BigIntMode;

void InitWithTransformType(JSHandle<GlobalEnv> &env, TransformType transformType,
                           JSMutableHandle<JSFunction> &constructor, SCheckMode &sCheckMode)
{
    if (transformType == TransformType::NORMAL || transformType == TransformType::BIGINT) {
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
    uint32_t argc = argv->GetArgsNumber();
    if (argc >= 2) { // 2: two args
        JSHandle<JSTaggedValue> reviverVal = GetCallArg(argv, 1);
        if (!reviverVal->IsUndefined()) {
            THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), GET_MESSAGE_STRING(ReviverOnlySupportUndefined),
                                        JSTaggedValue::Exception());
        }
    }
    return BuiltinsJson::ParseWithTransformType(argv, TransformType::SENDABLE);
}

JSTaggedValue BuiltinsBigIntJson::Parse(EcmaRuntimeCallInfo *argv)
{
    return BuiltinsJson::ParseWithTransformType(argv, TransformType::BIGINT);
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
        JSHandle<JSObject> syntaxError = factory->GetJSError(base::ErrorType::SYNTAX_ERROR,
                                                             "arg is empty", StackCheck::NO);
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, syntaxError.GetTaggedValue(), JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSMutableHandle<JSTaggedValue> reviverVal(thread, JSTaggedValue::Undefined());
    ParseOptions parseOptions;
    if (argc == 2) {  // 2: two args
        reviverVal.Update(GetCallArg(argv, 1));
    } else if (argc == 3 && base::JsonHelper::IsTypeSupportBigInt(transformType)) { // 3: three args
        reviverVal.Update(GetCallArg(argv, 1));
        JSHandle<JSTaggedValue> options = GetCallArg(argv, 2); // 2: two args
        JSHandle<JSTaggedValue> modeKey(factory->NewFromStdString("bigIntMode"));
        JSHandle<JSTaggedValue> typeKey(factory->NewFromStdString("parseReturnType"));
        if (options->IsECMAObject()) {
            JSHandle<JSTaggedValue> type = JSTaggedValue::GetProperty(thread, options, typeKey).GetValue();
            if (transformType == TransformType::SENDABLE && type->IsInt() && type->GetInt() == 1) { // 1: map
                parseOptions.returnType = ParseReturnType::MAP;
            }
            JSHandle<JSTaggedValue> modeValue = JSTaggedValue::GetProperty(thread, options, modeKey).GetValue();
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (modeValue->IsInt()) {
                int val = modeValue->GetInt();
                if (val == 2) { // 2: bigIntMode
                    parseOptions.bigIntMode = BigIntMode::ALWAYS_PARSE_AS_BIGINT;
                } else if (val == 1) {
                    parseOptions.bigIntMode = BigIntMode::PARSE_AS_BIGINT;
                }
            }
        }
    }
    return ParseWithTransformType(thread->GetEcmaVM(), msg, reviverVal, transformType, parseOptions);
}

JSTaggedValue BuiltinsJson::ParseWithTransformType(const EcmaVM *vm, JSHandle<JSTaggedValue> &msg,
                                                   JSHandle<JSTaggedValue> &reviverVal, TransformType transformType,
                                                   ParseOptions options)
{
    JSThread *thread = vm->GetJSThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> parseString = JSTaggedValue::ToString(thread, msg);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> result;
    if (EcmaStringAccessor(parseString).IsUtf8()) {
        panda::ecmascript::base::Utf8JsonParser parser(thread, transformType, options);
        result = parser.Parse(parseString);
    } else {
        panda::ecmascript::base::Utf16JsonParser parser(thread, transformType, options);
        result = parser.Parse(*parseString);
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

JSTaggedValue BuiltinsBigIntJson::Stringify(EcmaRuntimeCallInfo *argv)
{
    return BuiltinsJson::StringifyWithTransformType(argv, TransformType::BIGINT);
}

JSTaggedValue BuiltinsJson::StringifyWithTransformType(EcmaRuntimeCallInfo *argv, TransformType transformType)
{
    BUILTINS_API_TRACE(argv->GetThread(), Json, Stringify);
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    uint32_t argc = argv->GetArgsNumber();
    JSTaggedValue value = GetCallArg(argv, 0).GetTaggedValue();
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
    panda::ecmascript::base::JsonStringifier stringifier(thread, transformType);
    JSHandle<JSTaggedValue> result = stringifier.Stringify(handleValue, handleReplacer, handleGap);

    return result.GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins
