/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef BUILTIN_TEST_UTIL_H
#define BUILTIN_TEST_UTIL_H
#include "ecmascript/builtins/builtins_arraybuffer.h"
#include "ecmascript/builtins/builtins_date_time_format.h"
#include "ecmascript/builtins/builtins_list_format.h"
#include "ecmascript/builtins/builtins_sendable_arraybuffer.h"
#include "ecmascript/builtins/builtins_sharedarraybuffer.h"
#include "ecmascript/builtins/builtins_shared_typedarray.h"
#include "ecmascript/builtins/builtins_typedarray.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/shared_objects/js_shared_array.h"
#include "ecmascript/shared_objects/js_shared_typed_array.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/tests/test_helper.h"
namespace panda::test {
using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;
class BuiltTestUtil {
public:
    static JSTaggedValue CreateBuiltinsArrayBuffer(JSThread *thread, int32_t length)
    {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> arrayBuffer(thread, env->GetArrayBufferFunction().GetTaggedValue());
        JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
        // 6 : test case
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, arrayBuffer.GetTaggedValue(), 6);
        ecmaRuntimeCallInfo->SetFunction(arrayBuffer.GetTaggedValue());
        ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(length));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = BuiltinsArrayBuffer::ArrayBufferConstructor(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        return result;
    }

    static JSTaggedValue CreateBuiltinsSendableArrayBuffer(JSThread *thread, int32_t length)
    {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> arrayBuffer(thread, env->GetSBuiltininArrayBufferFunction().GetTaggedValue());
        JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
        // 6 : test case
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, arrayBuffer.GetTaggedValue(), 6);
        ecmaRuntimeCallInfo->SetFunction(arrayBuffer.GetTaggedValue());
        ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(length));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = BuiltinsSendableArrayBuffer::ArrayBufferConstructor(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        return result;
    }

    static JSTaggedValue CreateBuiltinsSharedArrayBuffer(JSThread *thread, int32_t length)
    {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> sharedArrayBuffer(thread, env->GetSharedArrayBufferFunction().GetTaggedValue());
        JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
        // 6 : test case
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, sharedArrayBuffer.GetTaggedValue(), 6);
        ecmaRuntimeCallInfo->SetFunction(sharedArrayBuffer.GetTaggedValue());
        ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(length));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = BuiltinsSharedArrayBuffer::SharedArrayBufferConstructor(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        return result;
    }

    static JSTypedArray *CreateTypedArray(JSThread *thread, const JSHandle<TaggedArray> &array)
    {
        auto ecmaVM = thread->GetEcmaVM();
        JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

        JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
        JSHandle<JSFunction> int8_array(env->GetInt8ArrayFunction());
        JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
        //  6 : test case
        auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*int8_array), 6);
        ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue(*int8_array));
        ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
        ecmaRuntimeCallInfo1->SetCallArg(0, jsarray.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
        JSTaggedValue result = BuiltinsTypedArray::Int8ArrayConstructor(ecmaRuntimeCallInfo1);
        TestHelper::TearDownFrame(thread, prev);

        EXPECT_TRUE(result.IsECMAObject());
        JSTypedArray *int8arr = JSTypedArray::Cast(reinterpret_cast<TaggedObject *>(result.GetRawData()));
        return int8arr;
    }

    static JSSharedTypedArray *CreateSharedTypedArray(JSThread *thread, const JSHandle<TaggedArray> &array)
    {
        auto ecmaVM = thread->GetEcmaVM();
        JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

        JSHandle<JSTaggedValue> jsarray(JSSharedArray::CreateArrayFromList(thread, array));
        JSHandle<JSFunction> int8_array(env->GetSharedInt8ArrayFunction());
        JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
        //  6 : test case
        auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*int8_array), 6);
        ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue(*int8_array));
        ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
        ecmaRuntimeCallInfo1->SetCallArg(0, jsarray.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
        JSTaggedValue result = BuiltinsSharedTypedArray::Int8ArrayConstructor(ecmaRuntimeCallInfo1);
        TestHelper::TearDownFrame(thread, prev);

        EXPECT_TRUE(result.IsECMAObject());
        JSSharedTypedArray *int8arr = JSSharedTypedArray::Cast(reinterpret_cast<TaggedObject *>(result.GetRawData()));
        return int8arr;
    }

    static JSHandle<TaggedArray> DateTimeGlobalSet(JSThread *thread)
    {
        auto globalConst = thread->GlobalConstants();
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

        JSHandle<TaggedArray> keyArray = factory->NewTaggedArray(6); // 6 : 6 length
        keyArray->Set(thread, 0, globalConst->GetHandledYearString()); // 0 : 0 first position
        keyArray->Set(thread, 1, globalConst->GetHandledMonthString()); // 1 : 1 second position
        keyArray->Set(thread, 2, globalConst->GetHandledDayString()); // 2 : 2 third position
        keyArray->Set(thread, 3, globalConst->GetHandledHourString()); // 3 : 3 fourth position
        keyArray->Set(thread, 4, globalConst->GetHandledMinuteString()); // 4 : 4 fifth position
        keyArray->Set(thread, 5, globalConst->GetHandledSecondString()); // 5 : 5 sixth position
        return keyArray;
    }

    static JSTaggedValue BuiltinsDateTimeOptionsSet(JSThread *thread)
    {
        auto globalConst = thread->GlobalConstants();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

        JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
        JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);

        JSHandle<JSTaggedValue> weekDay = globalConst->GetHandledWeekdayString();
        JSHandle<JSTaggedValue> dayPeriod = globalConst->GetHandledDayPeriodString();
        JSHandle<JSTaggedValue> hourCycle = globalConst->GetHandledHourCycleString();
        JSHandle<JSTaggedValue> timeZone = globalConst->GetHandledTimeZoneString();
        JSHandle<JSTaggedValue> numicValue(factory->NewFromASCII("numeric")); // test numeric
        JSHandle<JSTaggedValue> weekDayValue(factory->NewFromASCII("short")); // test short
        JSHandle<JSTaggedValue> dayPeriodValue(factory->NewFromASCII("long")); // test long
        JSHandle<JSTaggedValue> hourCycleValue(factory->NewFromASCII("h24")); // test h24
        JSHandle<JSTaggedValue> timeZoneValue(factory->NewFromASCII("UTC")); // test UTC

        JSHandle<TaggedArray> keyArray = DateTimeGlobalSet(thread);

        uint32_t arrayLen = keyArray->GetLength();
        JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
        for (uint32_t i = 0; i < arrayLen; i++) {
            key.Update(keyArray->Get(thread, i));
            JSObject::SetProperty(thread, optionsObj, key, numicValue);
        }
        JSObject::SetProperty(thread, optionsObj, weekDay, weekDayValue);
        JSObject::SetProperty(thread, optionsObj, dayPeriod, dayPeriodValue);
        JSObject::SetProperty(thread, optionsObj, hourCycle, hourCycleValue);
        JSObject::SetProperty(thread, optionsObj, timeZone, timeZoneValue);
        return optionsObj.GetTaggedValue();
    }

    static JSTaggedValue JSDateTimeFormatCreateWithLocaleTest(JSThread *thread, JSHandle<JSTaggedValue> &locale)
    {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> newTarget(env->GetDateTimeFormatFunction());
        JSHandle<JSObject> optionsObj(thread, BuiltTestUtil::BuiltinsDateTimeOptionsSet(thread));

        JSHandle<JSTaggedValue> localesString = locale;
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8);
        ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
        ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetCallArg(0, localesString.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(1, optionsObj.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = BuiltinsDateTimeFormat::DateTimeFormatConstructor(ecmaRuntimeCallInfo);
        EXPECT_TRUE(result.IsJSDateTimeFormat());
        TestHelper::TearDownFrame(thread, prev);
        return result;
    }

    static JSTaggedValue JSListFormatCreateWithOptionTest(JSThread *thread, JSHandle<JSTaggedValue> &locale,
                                                      JSHandle<JSTaggedValue> &typeValue)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> newTarget(env->GetListFormatFunction());
        JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
        JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);

        JSHandle<JSTaggedValue> typeKey = thread->GlobalConstants()->GetHandledTypeString();
        JSObject::SetProperty(thread, optionsObj, typeKey, typeValue);

        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8);
        ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
        ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetCallArg(0, locale.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(1, optionsObj.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = BuiltinsListFormat::ListFormatConstructor(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        EXPECT_TRUE(result.IsJSListFormat());
        return result;
    }
};
};  // namespace panda::test
#endif
