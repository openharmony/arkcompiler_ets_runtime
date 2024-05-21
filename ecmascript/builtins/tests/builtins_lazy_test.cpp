/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/platform/time.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class BuiltinsLazyTest : public BaseTestWithScope<true> {
};

HWTEST_F_L0(BuiltinsLazyTest, SlowGetOwnProperty)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<JSTaggedValue> key(factory->NewFromUtf8("Date"));
    auto globalObj = JSHandle<JSObject>(thread, globalEnv->GetGlobalObject());
    PropertyDescriptor desc(thread);
    bool success = JSObject::GetOwnProperty(thread, globalObj, key, desc);
    ASSERT_TRUE(success);
    ASSERT_TRUE(desc.GetValue()->IsJSFunction());
}

HWTEST_F_L0(BuiltinsLazyTest, SlowSetProperty)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<JSTaggedValue> key(factory->NewFromUtf8("Date"));
    auto globalObj = JSHandle<JSTaggedValue>(thread, globalEnv->GetGlobalObject());
    JSHandle<JSTaggedValue> value(factory->NewFromUtf8("Value"));
    bool success = JSObject::SetProperty(thread, globalObj, key, value);
    ASSERT_TRUE(success);
    PropertyDescriptor desc(thread);
    JSObject::GetOwnProperty(thread, JSHandle<JSObject>::Cast(globalObj), key, desc);
    ASSERT_FALSE(desc.IsAccessorDescriptor());
}

HWTEST_F_L0(BuiltinsLazyTest, EnvGetDateConstructorTest)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> dateFunction = globalEnv->GetDateFunction();
    ASSERT_TRUE(dateFunction->IsJSFunction());
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<JSTaggedValue> key(factory->NewFromUtf8("now"));
    ASSERT_TRUE(JSTaggedValue::HasProperty(thread, dateFunction, key));
}

HWTEST_F_L0(BuiltinsLazyTest, GlobalGetDateConstructorTest)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();
    auto key = factory->NewFromUtf8("Date");
    auto globalObj = globalEnv->GetGlobalObject();
    auto dateFunction = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, key.GetTaggedValue());
    ASSERT_TRUE(dateFunction.IsJSFunction());
}

HWTEST_F_L0(BuiltinsLazyTest, EnvGetSetConstructorTest)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> setFunction = globalEnv->GetBuiltinsSetFunction();
    ASSERT_TRUE(setFunction->IsJSFunction());
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<JSTaggedValue> setObject(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(setFunction),
        setFunction));
    JSHandle<JSTaggedValue> key(factory->NewFromUtf8("add"));
    ASSERT_TRUE(JSTaggedValue::HasProperty(thread, setObject, key));
}

HWTEST_F_L0(BuiltinsLazyTest, GlobalGetSetConstructorTest)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSTaggedValue key = factory->NewFromUtf8("Set").GetTaggedValue();
    auto globalObj = globalEnv->GetGlobalObject();
    auto setFunction = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, key);
    ASSERT_TRUE(setFunction.IsJSFunction());
}

HWTEST_F_L0(BuiltinsLazyTest, EnvGetMapConstructorTest)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> mapFunction = globalEnv->GetBuiltinsMapFunction();
    ASSERT_TRUE(mapFunction->IsJSFunction());
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<JSTaggedValue> mapObject(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(mapFunction),
        mapFunction));
    JSHandle<JSTaggedValue> key(factory->NewFromUtf8("clear"));
    ASSERT_TRUE(JSTaggedValue::HasProperty(thread, mapObject, key));
}

HWTEST_F_L0(BuiltinsLazyTest, GlobalGetMapConstructorTest)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSTaggedValue key = factory->NewFromUtf8("Map").GetTaggedValue();
    auto globalObj = globalEnv->GetGlobalObject();
    auto mapFunction = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, key);
    ASSERT_TRUE(mapFunction.IsJSFunction());
}

HWTEST_F_L0(BuiltinsLazyTest, EnvGetWeakMapConstructorTest)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> weakMapFunction = globalEnv->GetBuiltinsWeakMapFunction();
    ASSERT_TRUE(weakMapFunction->IsJSFunction());
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<JSTaggedValue> weakMapObject(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(weakMapFunction),
        weakMapFunction));
    JSHandle<JSTaggedValue> key(factory->NewFromUtf8("delete"));
    ASSERT_TRUE(JSTaggedValue::HasProperty(thread, weakMapObject, key));
}

HWTEST_F_L0(BuiltinsLazyTest, GlobalGetWeakMapConstructorTest)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSTaggedValue key = factory->NewFromUtf8("WeakMap").GetTaggedValue();
    auto globalObj = globalEnv->GetGlobalObject();
    auto weakMapFunction = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, key);
    ASSERT_TRUE(weakMapFunction.IsJSFunction());
}

HWTEST_F_L0(BuiltinsLazyTest, EnvGetWeakSetConstructorTest)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> weakSetFunction = globalEnv->GetBuiltinsWeakSetFunction();
    ASSERT_TRUE(weakSetFunction->IsJSFunction());
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<JSTaggedValue> weakSetObject(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(weakSetFunction),
        weakSetFunction));
    JSHandle<JSTaggedValue> key(factory->NewFromUtf8("delete"));
    ASSERT_TRUE(JSTaggedValue::HasProperty(thread, weakSetObject, key));
}

HWTEST_F_L0(BuiltinsLazyTest, GlobalGetWeakSetConstructorTest)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSTaggedValue key = factory->NewFromUtf8("WeakSet").GetTaggedValue();
    auto globalObj = globalEnv->GetGlobalObject();
    auto weakSetFunction = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, key);
    ASSERT_TRUE(weakSetFunction.IsJSFunction());
}

HWTEST_F_L0(BuiltinsLazyTest, EnvGetWeakRefConstructorTest)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> weakRefFunction = globalEnv->GetBuiltinsWeakRefFunction();
    ASSERT_TRUE(weakRefFunction->IsJSFunction());
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<JSTaggedValue> weakRefObject(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(weakRefFunction),
        weakRefFunction));
    JSHandle<JSTaggedValue> key(factory->NewFromUtf8("deref"));
    ASSERT_TRUE(JSTaggedValue::HasProperty(thread, weakRefObject, key));
}

HWTEST_F_L0(BuiltinsLazyTest, GlobalGetWeakRefConstructorTest)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSTaggedValue key = factory->NewFromUtf8("WeakRef").GetTaggedValue();
    auto globalObj = globalEnv->GetGlobalObject();
    auto weakRefFunction = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, key);
    ASSERT_TRUE(weakRefFunction.IsJSFunction());
}

HWTEST_F_L0(BuiltinsLazyTest, EnvGetFinalizationRegistryConstructorTest)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> function = globalEnv->GetBuiltinsFinalizationRegistryFunction();
    ASSERT_TRUE(function->IsJSFunction());
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<JSTaggedValue> object(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(function), function));
    JSHandle<JSTaggedValue> key(factory->NewFromUtf8("register"));
    ASSERT_TRUE(JSTaggedValue::HasProperty(thread, object, key));
}

HWTEST_F_L0(BuiltinsLazyTest, GlobalGetFinalizationRegistryConstructorTest)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSTaggedValue key = factory->NewFromUtf8("FinalizationRegistry").GetTaggedValue();
    auto globalObj = globalEnv->GetGlobalObject();
    auto function = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, key);
    ASSERT_TRUE(function.IsJSFunction());
}

HWTEST_F_L0(BuiltinsLazyTest, EnvGetInt8ArrayConstructorTest)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> function = globalEnv->GetInt8ArrayFunction();
    ASSERT_TRUE(function->IsJSFunction());
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<JSTaggedValue> object(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(function), function));
    JSHandle<JSTaggedValue> key(factory->NewFromUtf8("BYTES_PER_ELEMENT"));
    ASSERT_TRUE(JSTaggedValue::HasProperty(thread, object, key));
}

HWTEST_F_L0(BuiltinsLazyTest, GlobalGetInt8ArrayConstructorTest)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSTaggedValue key = factory->NewFromUtf8("Int8Array").GetTaggedValue();
    auto globalObj = globalEnv->GetGlobalObject();
    auto function = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, key);
    ASSERT_TRUE(function.IsJSFunction());
}

HWTEST_F_L0(BuiltinsLazyTest, EnvGetArrayBufferConstructorTest)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> function = globalEnv->GetArrayBufferFunction();
    ASSERT_TRUE(function->IsJSFunction());
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<JSTaggedValue> object(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(function), function));
    JSHandle<JSTaggedValue> key(factory->NewFromUtf8("slice"));
    ASSERT_TRUE(JSTaggedValue::HasProperty(thread, object, key));
}

HWTEST_F_L0(BuiltinsLazyTest, GlobalGetArrayBufferConstructorTest)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSTaggedValue key = factory->NewFromUtf8("ArrayBuffer").GetTaggedValue();
    auto globalObj = globalEnv->GetGlobalObject();
    auto function = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, key);
    ASSERT_TRUE(function.IsJSFunction());
}

HWTEST_F_L0(BuiltinsLazyTest, EnvGetDataViewConstructorTest)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> function = globalEnv->GetDataViewFunction();
    ASSERT_TRUE(function->IsJSFunction());
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<JSTaggedValue> object(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(function), function));
    JSHandle<JSTaggedValue> key(factory->NewFromUtf8("getFloat32"));
    ASSERT_TRUE(JSTaggedValue::HasProperty(thread, object, key));
}

HWTEST_F_L0(BuiltinsLazyTest, GlobalGetDataViewConstructorTest)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSTaggedValue key = factory->NewFromUtf8("DataView").GetTaggedValue();
    auto globalObj = globalEnv->GetGlobalObject();
    auto function = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, key);
    ASSERT_TRUE(function.IsJSFunction());
}

HWTEST_F_L0(BuiltinsLazyTest, EnvGetSharedArrayBufferConstructorTest)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> function = globalEnv->GetSharedArrayBufferFunction();
    ASSERT_TRUE(function->IsJSFunction());
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<JSTaggedValue> object(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(function), function));
    JSHandle<JSTaggedValue> key(factory->NewFromUtf8("slice"));
    ASSERT_TRUE(JSTaggedValue::HasProperty(thread, object, key));
}

HWTEST_F_L0(BuiltinsLazyTest, GlobalGetSharedArrayBufferConstructorTest)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSTaggedValue key = factory->NewFromUtf8("SharedArrayBuffer").GetTaggedValue();
    auto globalObj = globalEnv->GetGlobalObject();
    auto function = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, key);
    ASSERT_TRUE(function.IsJSFunction());
}

HWTEST_F_L0(BuiltinsLazyTest, EnvGetLocaleConstructorTest)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> function = globalEnv->GetLocaleFunction();
    ASSERT_TRUE(function->IsJSFunction());
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<JSTaggedValue> object(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(function), function));
    JSHandle<JSTaggedValue> key(factory->NewFromUtf8("maximize"));
    ASSERT_TRUE(JSTaggedValue::HasProperty(thread, object, key));
}

HWTEST_F_L0(BuiltinsLazyTest, IntlGetLocaleFunctionConstructorTest)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSTaggedValue key = factory->NewFromUtf8("Locale").GetTaggedValue();
    auto intlObj = globalEnv->GetIntlFunction().GetTaggedValue();
    auto function = FastRuntimeStub::GetPropertyByName(thread, intlObj, key);
    ASSERT_TRUE(function.IsJSFunction());
}
}  // namespace panda::test
