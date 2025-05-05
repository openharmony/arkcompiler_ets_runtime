/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/serializer/base_deserializer.h"
#include "ecmascript/serializer/value_serializer.h"
#include "gtest/gtest.h"
#include "jsnapi_expo.h"

using namespace panda::ecmascript;

namespace panda::test {
class JSNApiTests : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        vm_ = JSNApi::CreateJSVM(option);
        ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";
        thread_ = vm_->GetJSThread();
        vm_->SetEnableForceGC(true);
        thread_->ManagedCodeBegin();
        staticKey = StringRef::NewFromUtf8(vm_, "static");
        nonStaticKey = StringRef::NewFromUtf8(vm_, "nonStatic");
        instanceKey = StringRef::NewFromUtf8(vm_, "instance");
        getterSetterKey = StringRef::NewFromUtf8(vm_, "getterSetter");
        invalidKey = StringRef::NewFromUtf8(vm_, "invalid");
    }

    void TearDown() override
    {
        thread_->ManagedCodeEnd();
        vm_->SetEnableForceGC(false);
        JSNApi::DestroyJSVM(vm_);
    }

    template <typename T>
    void TestNumberRef(T val, TaggedType expected)
    {
        LocalScope scope(vm_);
        Local<NumberRef> obj = NumberRef::New(vm_, val);
        ASSERT_TRUE(obj->IsNumber());
        JSTaggedType res = JSNApiHelper::ToJSTaggedValue(*obj).GetRawData();
        ASSERT_EQ(res, expected);
        if constexpr (std::is_floating_point_v<T>) {
            if (std::isnan(val)) {
                ASSERT_TRUE(std::isnan(obj->Value()));
            } else {
                ASSERT_EQ(obj->Value(), val);
            }
        } else if constexpr (sizeof(T) >= sizeof(int32_t)) {
            ASSERT_EQ(obj->IntegerValue(vm_), val);
        } else if constexpr (std::is_signed_v<T>) {
            ASSERT_EQ(obj->Int32Value(vm_), val);
        } else {
            ASSERT_EQ(obj->Uint32Value(vm_), val);
        }
    }

    TaggedType ConvertDouble(double val)
    {
        return base::bit_cast<JSTaggedType>(val) + JSTaggedValue::DOUBLE_ENCODE_OFFSET;
    }

protected:
    JSThread *thread_ = nullptr;
    EcmaVM *vm_ = nullptr;
    Local<StringRef> staticKey;
    Local<StringRef> nonStaticKey;
    Local<StringRef> instanceKey;
    Local<StringRef> getterSetterKey;
    Local<StringRef> invalidKey;
};

panda::JSValueRef FunctionCallback(JsiRuntimeCallInfo *info)
{
    EcmaVM *vm = info->GetVM();
    Local<JSValueRef> jsThisRef = info->GetThisRef();
    Local<ObjectRef> thisRef = jsThisRef->ToObject(vm);
    return **thisRef;
}

Local<FunctionRef> GetNewSendableClassFunction(
    EcmaVM *vm, Local<FunctionRef> parent, bool isDict = false, bool duplicated = false, bool isParent = false)
{
    FunctionRef::SendablePropertiesInfos infos;

    if (isDict) {
        uint32_t maxInline = std::max(JSSharedFunction::MAX_INLINE, JSSharedObject::MAX_INLINE);
        for (uint32_t i = 0; i < maxInline; ++i) {
            Local<StringRef> tempStr = StringRef::NewFromUtf8(vm, std::to_string(i).c_str());
            infos.instancePropertiesInfo.keys.push_back(tempStr);
            infos.instancePropertiesInfo.types.push_back(FunctionRef::SendableType::NONE);
            infos.instancePropertiesInfo.attributes.push_back(PropertyAttribute(tempStr, true, true, true));
            infos.staticPropertiesInfo.keys.push_back(tempStr);
            infos.staticPropertiesInfo.types.push_back(FunctionRef::SendableType::NONE);
            infos.staticPropertiesInfo.attributes.push_back(PropertyAttribute(tempStr, true, true, true));
            infos.nonStaticPropertiesInfo.keys.push_back(tempStr);
            infos.nonStaticPropertiesInfo.types.push_back(FunctionRef::SendableType::NONE);
            infos.nonStaticPropertiesInfo.attributes.push_back(PropertyAttribute(tempStr, true, true, true));
        }
    }

    std::string instanceKey = "instance";
    std::string staticKey = "static";
    std::string nonStaticKey = "nonStatic";

    if (isParent) {
        instanceKey = "parentInstance";
        staticKey = "parentStatic";
        nonStaticKey = "parentNonStatic";
    }

    Local<StringRef> instanceStr = StringRef::NewFromUtf8(vm, instanceKey.c_str());
    infos.instancePropertiesInfo.keys.push_back(instanceStr);
    infos.instancePropertiesInfo.types.push_back(FunctionRef::SendableType::NONE);
    infos.instancePropertiesInfo.attributes.push_back(PropertyAttribute(instanceStr, true, true, true));

    Local<StringRef> staticStr = StringRef::NewFromUtf8(vm, staticKey.c_str());
    infos.staticPropertiesInfo.keys.push_back(staticStr);
    infos.staticPropertiesInfo.types.push_back(FunctionRef::SendableType::NONE);
    infos.staticPropertiesInfo.attributes.push_back(PropertyAttribute(staticStr, true, true, true));

    Local<StringRef> nonStaticStr = StringRef::NewFromUtf8(vm, nonStaticKey.c_str());
    infos.nonStaticPropertiesInfo.keys.push_back(nonStaticStr);
    infos.nonStaticPropertiesInfo.types.push_back(FunctionRef::SendableType::NONE);
    infos.nonStaticPropertiesInfo.attributes.push_back(PropertyAttribute(nonStaticStr, true, true, true));

    if (duplicated) {
        Local<StringRef> duplicatedKey = StringRef::NewFromUtf8(vm, "parentInstance");
        Local<NumberRef> duplicatedValue = NumberRef::New(vm, 0);
        infos.instancePropertiesInfo.keys.push_back(duplicatedKey);
        infos.instancePropertiesInfo.types.push_back(FunctionRef::SendableType::NONE);
        infos.instancePropertiesInfo.attributes.push_back(PropertyAttribute(duplicatedValue, true, true, true));
    }

    Local<FunctionRef> constructor = FunctionRef::NewSendableClassFunction(
        vm, FunctionCallback, nullptr, nullptr, StringRef::NewFromUtf8(vm, "name"), infos, parent);

    return constructor;
}

HWTEST_F_L0(JSNApiTests, NewSendableClassFunction)
{
    LocalScope scope(vm_);
    Local<FunctionRef> constructor = GetNewSendableClassFunction(vm_, JSValueRef::Hole(vm_));

    ASSERT_EQ("name", constructor->GetName(vm_)->ToString(vm_));
    ASSERT_TRUE(constructor->IsFunction(vm_));
    JSHandle<JSTaggedValue> jsConstructor = JSNApiHelper::ToJSHandle(constructor);
    ASSERT_TRUE(jsConstructor->IsClassConstructor());

    Local<JSValueRef> functionPrototype = constructor->GetFunctionPrototype(vm_);
    ASSERT_TRUE(functionPrototype->IsObject(vm_));
    JSHandle<JSTaggedValue> jsPrototype = JSNApiHelper::ToJSHandle(functionPrototype);
    ASSERT_TRUE(jsPrototype->IsClassPrototype());

    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    auto prototype = JSTaggedValue::GetProperty(
                         thread_, JSNApiHelper::ToJSHandle(constructor), globalConst->GetHandledPrototypeString())
                         .GetValue();
    ASSERT_FALSE(prototype->IsUndefined());
    ASSERT_TRUE(prototype->IsECMAObject() || prototype->IsNull());
}

HWTEST_F_L0(JSNApiTests, NewSendableClassFunctionProperties)
{
    LocalScope scope(vm_);
    Local<FunctionRef> constructor = GetNewSendableClassFunction(vm_, JSValueRef::Hole(vm_));
    Local<ObjectRef> prototype = constructor->GetFunctionPrototype(vm_);

    ASSERT_EQ("static", constructor->Get(vm_, staticKey)->ToString(vm_)->ToString(vm_));
    ASSERT_EQ("nonStatic", prototype->Get(vm_, nonStaticKey)->ToString(vm_)->ToString(vm_));
    ASSERT_EQ("undefined", constructor->Get(vm_, invalidKey)->ToString(vm_)->ToString(vm_));
    ASSERT_EQ("undefined", prototype->Get(vm_, invalidKey)->ToString(vm_)->ToString(vm_));

    // set static property on constructor
    constructor->Set(vm_, staticKey, StringRef::NewFromUtf8(vm_, "static0"));
    ASSERT_EQ("static0", constructor->Get(vm_, staticKey)->ToString(vm_)->ToString(vm_));

    // set non static property on prototype
    prototype->Set(vm_, nonStaticKey, StringRef::NewFromUtf8(vm_, "nonStatic0"));
    ASSERT_EQ("nonStatic0", prototype->Get(vm_, nonStaticKey)->ToString(vm_)->ToString(vm_));

    // set invalid property on constructor
    ASSERT_FALSE(vm_->GetJSThread()->HasPendingException());
    constructor->Set(vm_, invalidKey, StringRef::NewFromUtf8(vm_, "invalid"));
    ASSERT_TRUE(vm_->GetJSThread()->HasPendingException());
    JSNApi::GetAndClearUncaughtException(vm_);
    ASSERT_EQ("undefined", constructor->Get(vm_, invalidKey)->ToString(vm_)->ToString(vm_));

    // set invalid property on prototype
    ASSERT_FALSE(vm_->GetJSThread()->HasPendingException());
    prototype->Set(vm_, invalidKey, StringRef::NewFromUtf8(vm_, "invalid"));
    ASSERT_TRUE(vm_->GetJSThread()->HasPendingException());
    JSNApi::GetAndClearUncaughtException(vm_);
    ASSERT_EQ("undefined", prototype->Get(vm_, invalidKey)->ToString(vm_)->ToString(vm_));
}

HWTEST_F_L0(JSNApiTests, NewSendableClassFunctionDictProperties)
{
    LocalScope scope(vm_);
    Local<FunctionRef> constructor = GetNewSendableClassFunction(vm_, JSValueRef::Hole(vm_), true);
    Local<ObjectRef> prototype = constructor->GetFunctionPrototype(vm_);

    ASSERT_EQ("static", constructor->Get(vm_, staticKey)->ToString(vm_)->ToString(vm_));
    ASSERT_EQ("nonStatic", prototype->Get(vm_, nonStaticKey)->ToString(vm_)->ToString(vm_));
    ASSERT_EQ("undefined", constructor->Get(vm_, invalidKey)->ToString(vm_)->ToString(vm_));
    ASSERT_EQ("undefined", prototype->Get(vm_, invalidKey)->ToString(vm_)->ToString(vm_));

    // set static property on constructor
    constructor->Set(vm_, staticKey, StringRef::NewFromUtf8(vm_, "static0"));
    ASSERT_EQ("static0", constructor->Get(vm_, staticKey)->ToString(vm_)->ToString(vm_));

    // set non static property on prototype
    prototype->Set(vm_, nonStaticKey, StringRef::NewFromUtf8(vm_, "nonStatic0"));
    ASSERT_EQ("nonStatic0", prototype->Get(vm_, nonStaticKey)->ToString(vm_)->ToString(vm_));

    // set invalid property on constructor
    ASSERT_FALSE(vm_->GetJSThread()->HasPendingException());
    constructor->Set(vm_, invalidKey, StringRef::NewFromUtf8(vm_, "invalid"));
    ASSERT_TRUE(vm_->GetJSThread()->HasPendingException());
    JSNApi::GetAndClearUncaughtException(vm_);
    ASSERT_EQ("undefined", constructor->Get(vm_, invalidKey)->ToString(vm_)->ToString(vm_));

    // set invalid property on prototype
    ASSERT_FALSE(vm_->GetJSThread()->HasPendingException());
    prototype->Set(vm_, invalidKey, StringRef::NewFromUtf8(vm_, "invalid"));
    ASSERT_TRUE(vm_->GetJSThread()->HasPendingException());
    JSNApi::GetAndClearUncaughtException(vm_);
    ASSERT_EQ("undefined", prototype->Get(vm_, invalidKey)->ToString(vm_)->ToString(vm_));
}

HWTEST_F_L0(JSNApiTests, NewSendableClassFunctionInstance)
{
    LocalScope scope(vm_);
    Local<FunctionRef> constructor = GetNewSendableClassFunction(vm_, JSValueRef::Hole(vm_));
    Local<ObjectRef> obj = constructor->Constructor(vm_, nullptr, 0);
    Local<ObjectRef> obj0 = constructor->Constructor(vm_, nullptr, 0);

    ASSERT_TRUE(JSFunction::InstanceOf(thread_, JSNApiHelper::ToJSHandle(obj), JSNApiHelper::ToJSHandle(constructor)));
    ASSERT_TRUE(JSFunction::InstanceOf(thread_, JSNApiHelper::ToJSHandle(obj0), JSNApiHelper::ToJSHandle(constructor)));

    // set instance property
    ASSERT_EQ("undefined", obj->Get(vm_, instanceKey)->ToString(vm_)->ToString(vm_));
    ASSERT_EQ("undefined", obj0->Get(vm_, instanceKey)->ToString(vm_)->ToString(vm_));
    obj->Set(vm_, instanceKey, StringRef::NewFromUtf8(vm_, "instance"));
    ASSERT_EQ("instance", obj->Get(vm_, instanceKey)->ToString(vm_)->ToString(vm_));
    ASSERT_EQ("undefined", obj0->Get(vm_, instanceKey)->ToString(vm_)->ToString(vm_));

    // set non static property on prototype and get from instance
    ASSERT_EQ("nonStatic", obj->Get(vm_, nonStaticKey)->ToString(vm_)->ToString(vm_));
    ASSERT_EQ("nonStatic", obj0->Get(vm_, nonStaticKey)->ToString(vm_)->ToString(vm_));
    Local<ObjectRef> prototype = obj->GetPrototype(vm_);
    prototype->Set(vm_, nonStaticKey, StringRef::NewFromUtf8(vm_, "nonStatic0"));
    ASSERT_EQ("nonStatic0", obj->Get(vm_, nonStaticKey)->ToString(vm_)->ToString(vm_));
    ASSERT_EQ("nonStatic0", obj0->Get(vm_, nonStaticKey)->ToString(vm_)->ToString(vm_));

    // set non static property on instance
    ASSERT_FALSE(vm_->GetJSThread()->HasPendingException());
    obj->Set(vm_, nonStaticKey, StringRef::NewFromUtf8(vm_, "nonStatic1"));
    ASSERT_TRUE(vm_->GetJSThread()->HasPendingException());
    JSNApi::GetAndClearUncaughtException(vm_);
    ASSERT_EQ("nonStatic0", obj->Get(vm_, nonStaticKey)->ToString(vm_)->ToString(vm_));
    ASSERT_EQ("nonStatic0", obj0->Get(vm_, nonStaticKey)->ToString(vm_)->ToString(vm_));

    // set invalid property on instance
    ASSERT_EQ("undefined", obj->Get(vm_, invalidKey)->ToString(vm_)->ToString(vm_));
    ASSERT_FALSE(vm_->GetJSThread()->HasPendingException());
    obj->Set(vm_, invalidKey, StringRef::NewFromUtf8(vm_, "invalid"));
    ASSERT_TRUE(vm_->GetJSThread()->HasPendingException());
    JSNApi::GetAndClearUncaughtException(vm_);
    ASSERT_EQ("undefined", obj->Get(vm_, invalidKey)->ToString(vm_)->ToString(vm_));
}

HWTEST_F_L0(JSNApiTests, NewSendableClassFunctionDictInstance)
{
    LocalScope scope(vm_);
    Local<FunctionRef> constructor = GetNewSendableClassFunction(vm_, JSValueRef::Hole(vm_), true);
    Local<ObjectRef> obj = constructor->Constructor(vm_, nullptr, 0);
    Local<ObjectRef> obj0 = constructor->Constructor(vm_, nullptr, 0);

    ASSERT_TRUE(JSFunction::InstanceOf(thread_, JSNApiHelper::ToJSHandle(obj), JSNApiHelper::ToJSHandle(constructor)));
    ASSERT_TRUE(JSFunction::InstanceOf(thread_, JSNApiHelper::ToJSHandle(obj0), JSNApiHelper::ToJSHandle(constructor)));

    // set instance property
    ASSERT_EQ("undefined", obj->Get(vm_, instanceKey)->ToString(vm_)->ToString(vm_));
    ASSERT_EQ("undefined", obj0->Get(vm_, instanceKey)->ToString(vm_)->ToString(vm_));
    obj->Set(vm_, instanceKey, StringRef::NewFromUtf8(vm_, "instance"));
    ASSERT_EQ("instance", obj->Get(vm_, instanceKey)->ToString(vm_)->ToString(vm_));
    ASSERT_EQ("undefined", obj0->Get(vm_, instanceKey)->ToString(vm_)->ToString(vm_));

    // set non static property on prototype and get from instance
    ASSERT_EQ("nonStatic", obj->Get(vm_, nonStaticKey)->ToString(vm_)->ToString(vm_));
    ASSERT_EQ("nonStatic", obj0->Get(vm_, nonStaticKey)->ToString(vm_)->ToString(vm_));
    Local<ObjectRef> prototype = obj->GetPrototype(vm_);
    prototype->Set(vm_, nonStaticKey, StringRef::NewFromUtf8(vm_, "nonStatic0"));
    ASSERT_EQ("nonStatic0", obj->Get(vm_, nonStaticKey)->ToString(vm_)->ToString(vm_));
    ASSERT_EQ("nonStatic0", obj0->Get(vm_, nonStaticKey)->ToString(vm_)->ToString(vm_));

    // set non static property on instance
    ASSERT_FALSE(vm_->GetJSThread()->HasPendingException());
    obj->Set(vm_, nonStaticKey, StringRef::NewFromUtf8(vm_, "nonStatic1"));
    ASSERT_TRUE(vm_->GetJSThread()->HasPendingException());
    JSNApi::GetAndClearUncaughtException(vm_);
    ASSERT_EQ("nonStatic0", obj->Get(vm_, nonStaticKey)->ToString(vm_)->ToString(vm_));
    ASSERT_EQ("nonStatic0", obj0->Get(vm_, nonStaticKey)->ToString(vm_)->ToString(vm_));

    // set invalid property on instance
    ASSERT_EQ("undefined", obj->Get(vm_, invalidKey)->ToString(vm_)->ToString(vm_));
    ASSERT_FALSE(vm_->GetJSThread()->HasPendingException());
    obj->Set(vm_, invalidKey, StringRef::NewFromUtf8(vm_, "invalid"));
    ASSERT_TRUE(vm_->GetJSThread()->HasPendingException());
    JSNApi::GetAndClearUncaughtException(vm_);
    ASSERT_EQ("undefined", obj->Get(vm_, invalidKey)->ToString(vm_)->ToString(vm_));
}

HWTEST_F_L0(JSNApiTests, NewSendableClassFunctionInherit)
{
    LocalScope scope(vm_);
    Local<FunctionRef> parent = GetNewSendableClassFunction(vm_, JSValueRef::Hole(vm_), false, false, true);
    Local<FunctionRef> constructor = GetNewSendableClassFunction(vm_, parent);
    Local<ObjectRef> obj = constructor->Constructor(vm_, nullptr, 0);

    ASSERT_TRUE(JSFunction::InstanceOf(thread_, JSNApiHelper::ToJSHandle(obj), JSNApiHelper::ToJSHandle(constructor)));
    ASSERT_TRUE(JSFunction::InstanceOf(thread_, JSNApiHelper::ToJSHandle(obj), JSNApiHelper::ToJSHandle(parent)));

    // set parent instance property on instance
    Local<StringRef> parentInstanceKey = StringRef::NewFromUtf8(vm_, "parentInstance");
    ASSERT_EQ("undefined", obj->Get(vm_, parentInstanceKey)->ToString(vm_)->ToString(vm_));
    obj->Set(vm_, parentInstanceKey, StringRef::NewFromUtf8(vm_, "parentInstance"));
    ASSERT_EQ("parentInstance", obj->Get(vm_, parentInstanceKey)->ToString(vm_)->ToString(vm_));

    // get parent static property from constructor
    Local<StringRef> parentStaticKey = StringRef::NewFromUtf8(vm_, "parentStatic");
    ASSERT_EQ("parentStatic", constructor->Get(vm_, parentStaticKey)->ToString(vm_)->ToString(vm_));

    // get parent non static property form instance
    Local<StringRef> parentNonStaticKey = StringRef::NewFromUtf8(vm_, "parentNonStatic");
    ASSERT_EQ("parentNonStatic", obj->Get(vm_, parentNonStaticKey)->ToString(vm_)->ToString(vm_));
}

HWTEST_F_L0(JSNApiTests, NewSendableClassFunctionDictInherit)
{
    LocalScope scope(vm_);
    Local<FunctionRef> parent = GetNewSendableClassFunction(vm_, JSValueRef::Hole(vm_), true, false, true);
    Local<FunctionRef> constructor = GetNewSendableClassFunction(vm_, parent);
    Local<ObjectRef> obj = constructor->Constructor(vm_, nullptr, 0);

    ASSERT_TRUE(JSFunction::InstanceOf(thread_, JSNApiHelper::ToJSHandle(obj), JSNApiHelper::ToJSHandle(constructor)));
    ASSERT_TRUE(JSFunction::InstanceOf(thread_, JSNApiHelper::ToJSHandle(obj), JSNApiHelper::ToJSHandle(parent)));

    // set parent instance property on instance
    Local<StringRef> parentInstanceKey = StringRef::NewFromUtf8(vm_, "parentInstance");
    ASSERT_EQ("undefined", obj->Get(vm_, parentInstanceKey)->ToString(vm_)->ToString(vm_));
    obj->Set(vm_, parentInstanceKey, StringRef::NewFromUtf8(vm_, "parentInstance"));
    ASSERT_EQ("parentInstance", obj->Get(vm_, parentInstanceKey)->ToString(vm_)->ToString(vm_));

    // get parent static property from constructor
    Local<StringRef> parentStaticKey = StringRef::NewFromUtf8(vm_, "parentStatic");
    ASSERT_EQ("parentStatic", constructor->Get(vm_, parentStaticKey)->ToString(vm_)->ToString(vm_));

    // get parent non static property form instance
    Local<StringRef> parentNonStaticKey = StringRef::NewFromUtf8(vm_, "parentNonStatic");
    ASSERT_EQ("parentNonStatic", obj->Get(vm_, parentNonStaticKey)->ToString(vm_)->ToString(vm_));
}

HWTEST_F_L0(JSNApiTests, NewSendableClassFunctionInheritWithDuplicatedKey)
{
    LocalScope scope(vm_);
    Local<FunctionRef> parent = GetNewSendableClassFunction(vm_, JSValueRef::Hole(vm_), false, false, true);
    Local<FunctionRef> constructor = GetNewSendableClassFunction(vm_, parent, false, true);
    Local<ObjectRef> obj = constructor->Constructor(vm_, nullptr, 0);

    ASSERT_TRUE(JSFunction::InstanceOf(thread_, JSNApiHelper::ToJSHandle(obj), JSNApiHelper::ToJSHandle(constructor)));
    ASSERT_TRUE(JSFunction::InstanceOf(thread_, JSNApiHelper::ToJSHandle(obj), JSNApiHelper::ToJSHandle(parent)));

    // set duplicated instance property on instance
    Local<StringRef> parentInstanceKey = StringRef::NewFromUtf8(vm_, "parentInstance");
    ASSERT_EQ("undefined", obj->Get(vm_, parentInstanceKey)->ToString(vm_)->ToString(vm_));
    obj->Set(vm_, parentInstanceKey, NumberRef::New(vm_, 0));
    EXPECT_TRUE(NumberRef::New(vm_, 0)->IsStrictEquals(vm_, obj->Get(vm_, parentInstanceKey)));
}

HWTEST_F_L0(JSNApiTests, NewSendable)
{
    LocalScope scope(vm_);
    Local<FunctionRef> func = FunctionRef::NewSendable(
        vm_,
        [](JsiRuntimeCallInfo *runtimeInfo) -> JSValueRef {
            EcmaVM *vm = runtimeInfo->GetVM();
            LocalScope scope(vm);
            return **StringRef::NewFromUtf8(vm, "funcResult");
        },
        nullptr);
    Local<JSValueRef> res = func->Call(vm_, JSValueRef::Undefined(vm_), nullptr, 0);
    ASSERT_EQ("funcResult", res->ToString(vm_)->ToString(vm_));
}

HWTEST_F_L0(JSNApiTests, NewSendableClassFunctionFunction)
{
    LocalScope scope(vm_);

    FunctionRef::SendablePropertiesInfos infos;

    Local<FunctionRef> func = FunctionRef::NewSendable(
        vm_,
        [](JsiRuntimeCallInfo *runtimeInfo) -> JSValueRef {
            EcmaVM *vm = runtimeInfo->GetVM();
            LocalScope scope(vm);
            return **StringRef::NewFromUtf8(vm, "funcResult");
        },
        nullptr);
    infos.staticPropertiesInfo.keys.push_back(staticKey);
    infos.staticPropertiesInfo.types.push_back(FunctionRef::SendableType::OBJECT);
    infos.staticPropertiesInfo.attributes.push_back(PropertyAttribute(func, true, true, true));

    Local<FunctionRef> constructor = FunctionRef::NewSendableClassFunction(
        vm_, FunctionCallback, nullptr, nullptr, StringRef::NewFromUtf8(vm_, "name"), infos, JSValueRef::Hole(vm_));

    Local<FunctionRef> staticValue = constructor->Get(vm_, staticKey);
    ASSERT_TRUE(staticValue->IsFunction(vm_));
    Local<JSValueRef> res = staticValue->Call(vm_, JSValueRef::Undefined(vm_), nullptr, 0);
    ASSERT_EQ("funcResult", res->ToString(vm_)->ToString(vm_));
}

HWTEST_F_L0(JSNApiTests, NewSendableClassFunctionGetterSetter)
{
    LocalScope scope(vm_);

    FunctionRef::SendablePropertiesInfos infos;

    Local<StringRef> getterSetter = StringRef::NewFromUtf8(vm_, "getterSetter");
    infos.staticPropertiesInfo.keys.push_back(getterSetter);
    infos.staticPropertiesInfo.types.push_back(FunctionRef::SendableType::NONE);
    infos.staticPropertiesInfo.attributes.push_back(PropertyAttribute(getterSetter, true, true, true));
    Local<FunctionRef> staticGetter = FunctionRef::NewSendable(
        vm_,
        [](JsiRuntimeCallInfo *info) -> JSValueRef {
            Local<JSValueRef> value = info->GetThisRef();
            Local<ObjectRef> obj = value->ToObject(info->GetVM());
            Local<JSValueRef> temp = obj->Get(info->GetVM(), StringRef::NewFromUtf8(info->GetVM(), "getterSetter"));
            return **temp->ToString(info->GetVM());
        },
        nullptr);
    Local<FunctionRef> staticSetter = FunctionRef::NewSendable(
        vm_,
        [](JsiRuntimeCallInfo *info) -> JSValueRef {
            Local<JSValueRef> arg = info->GetCallArgRef(0);
            Local<JSValueRef> value = info->GetThisRef();
            Local<ObjectRef> obj = value->ToObject(info->GetVM());
            obj->Set(info->GetVM(), StringRef::NewFromUtf8(info->GetVM(), "getterSetter"), arg);
            return **JSValueRef::Undefined(info->GetVM());
        },
        nullptr);
    Local<JSValueRef> staticValue = panda::ObjectRef::CreateSendableAccessorData(vm_, staticGetter, staticSetter);
    infos.staticPropertiesInfo.keys.push_back(staticKey);
    infos.staticPropertiesInfo.types.push_back(FunctionRef::SendableType::OBJECT);
    infos.staticPropertiesInfo.attributes.push_back(PropertyAttribute(staticValue, true, true, true));

    Local<FunctionRef> constructor = FunctionRef::NewSendableClassFunction(
        vm_, FunctionCallback, nullptr, nullptr, StringRef::NewFromUtf8(vm_, "name"), infos, JSValueRef::Hole(vm_));

    ASSERT_EQ("getterSetter", constructor->Get(vm_, getterSetter)->ToString(vm_)->ToString(vm_));
    ASSERT_EQ("getterSetter", constructor->Get(vm_, staticKey)->ToString(vm_)->ToString(vm_));
    constructor->Set(vm_, staticKey, StringRef::NewFromUtf8(vm_, "getterSetter0"));
    ASSERT_EQ("getterSetter0", constructor->Get(vm_, getterSetter)->ToString(vm_)->ToString(vm_));
    ASSERT_EQ("getterSetter0", constructor->Get(vm_, staticKey)->ToString(vm_)->ToString(vm_));
}

HWTEST_F_L0(JSNApiTests, NewSendableClassFunctionStringify)
{
    LocalScope scope(vm_);

    FunctionRef::SendablePropertiesInfos infos;
    Local<FunctionRef> constructor = FunctionRef::NewSendableClassFunction(
        vm_, FunctionCallback, nullptr, nullptr, StringRef::NewFromUtf8(vm_, "name"), infos, JSValueRef::Hole(vm_));

    Local<ObjectRef> obj = constructor->Constructor(vm_, nullptr, 0);
    ASSERT_EQ("[object Object]", obj->ToString(vm_)->ToString(vm_));
}

HWTEST_F_L0(JSNApiTests, NewObjectWithProperties)
{
    LocalScope scope(vm_);

    FunctionRef::SendablePropertiesInfo info;
    Local<StringRef> str = StringRef::NewFromUtf8(vm_, "str");
    info.keys.push_back(str);
    info.types.push_back(FunctionRef::SendableType::NONE);
    info.attributes.push_back(PropertyAttribute(str, true, true, true));

    Local<ObjectRef> object = ObjectRef::NewSWithProperties(vm_, info);
    Local<JSValueRef> value = object->Get(vm_, str);
    EXPECT_TRUE(str->IsStrictEquals(vm_, value));
}

HWTEST_F_L0(JSNApiTests, SendableMapRef_GetSize_GetTotalElements_Get_GetKey_GetValue)
{
    LocalScope scope(vm_);
    Local<SendableMapRef> map = SendableMapRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
    map->Set(vm_, key, value);
    Local<JSValueRef> res = map->Get(vm_, key);
    ASSERT_EQ(res->ToString(vm_)->ToString(vm_), value->ToString(vm_)->ToString(vm_));
    int32_t num = map->GetSize(vm_);
    int32_t num1 = map->GetTotalElements(vm_);
    ASSERT_EQ(num, 1);
    ASSERT_EQ(num1, 1);
    Local<JSValueRef> res1 = map->GetKey(vm_, 0);
    ASSERT_EQ(res1->ToString(vm_)->ToString(vm_), key->ToString(vm_)->ToString(vm_));
    Local<JSValueRef> res2 = map->GetValue(vm_, 0);
    ASSERT_EQ(res2->ToString(vm_)->ToString(vm_), value->ToString(vm_)->ToString(vm_));
}

HWTEST_F_L0(JSNApiTests, SendableSetRef_GetSize_GetTotalElements_GetValue)
{
    LocalScope scope(vm_);
    Local<SendableSetRef> set = SendableSetRef::New(vm_);
    EXPECT_TRUE(set->IsSharedSet(vm_));
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
    set->Add(vm_, value);
    int32_t num = set->GetSize(vm_);
    int32_t num1 = set->GetTotalElements(vm_);
    ASSERT_EQ(num, 1);
    ASSERT_EQ(num1, 1);
    Local<JSValueRef> res = set->GetValue(vm_, 0);
    ASSERT_EQ(res->ToString(vm_)->ToString(vm_), value->ToString(vm_)->ToString(vm_));
}

HWTEST_F_L0(JSNApiTests, SendableArrayRef_New_Len_GetVal_SetPro)
{
    LocalScope scope(vm_);
    uint32_t length = 4;
    Local<SendableArrayRef> array = SendableArrayRef::New(vm_, length);
    EXPECT_TRUE(array->IsSharedArray(vm_));
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
    uint32_t arrayLength = array->Length(vm_);
    ASSERT_EQ(arrayLength, length);
    Local<JSValueRef> res = array->GetValueAt(vm_, value, length);
    uint32_t index = 1;
    bool flag = array->SetProperty(vm_, res, index, value);
    EXPECT_FALSE(flag);
}

HWTEST_F_L0(JSNApiTests, WeakSetRef_New_add)
{
    LocalScope scope(vm_);
    Local<WeakSetRef> object = WeakSetRef::New(vm_);
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
    object->Add(vm_, value);
    ASSERT_EQ(object->GetSize(vm_), 1);
}

HWTEST_F_L0(JSNApiTests, JSValueRef_IsWeakMap_Set_Has)
{
    LocalScope scope(vm_);
    Local<WeakMapRef> map = WeakMapRef::New(vm_);
    EXPECT_TRUE(map->IsWeakMap(vm_));
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
    map->Set(vm_, key, value);
    EXPECT_TRUE(map->Has(vm_, key));
}

HWTEST_F_L0(JSNApiTests, SetRef_Add)
{
    LocalScope scope(vm_);
    Local<SetRef> set = SetRef::New(vm_);
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
    set->Add(vm_, value);
    ASSERT_EQ(set->GetSize(vm_), 1);
}

HWTEST_F_L0(JSNApiTests, Promise_GetPromiseResult)
{
    LocalScope scope(vm_);
    Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm_);

    Local<PromiseRef> promise = capability->GetPromise(vm_);
    promise->GetPromiseResult(vm_);
    ASSERT_TRUE(promise->IsPromise(vm_));
}

HWTEST_F_L0(JSNApiTests, Promise_GetPromiseState)
{
    LocalScope scope(vm_);
    Local<PromiseCapabilityRef> cap = PromiseCapabilityRef::New(vm_);

    Local<PromiseRef> promise = cap->GetPromise(vm_);
    promise->GetPromiseState(vm_);
    ASSERT_TRUE(promise->IsPromise(vm_));
}

HWTEST_F_L0(JSNApiTests, NapiHasOwnProperty)
{
    Local<ObjectRef> object = ObjectRef::New(vm_);
    JSTaggedValue value(0);
    JSHandle<JSTaggedValue> handle(thread_, value);
    Local<JSValueRef> key = JSNApiHelper::ToLocal<JSValueRef>(handle);
    const char* utf8Key = "TestKeyForOwnProperty";
    Local<JSValueRef> key2 = StringRef::NewFromUtf8(vm_, utf8Key);
    Local<JSValueRef> value2 = ObjectRef::New(vm_);
    object->Set(vm_, key, value2);
    object->Set(vm_, key2, value2);
    
    Local<JSValueRef> flag = JSNApi::NapiHasOwnProperty(vm_, reinterpret_cast<uintptr_t>(*object),
                                                     reinterpret_cast<uintptr_t>(*key));
    ASSERT_TRUE(flag->BooleaValue(vm_));
    flag = JSNApi::NapiHasOwnProperty(vm_, reinterpret_cast<uintptr_t>(*object), reinterpret_cast<uintptr_t>(*key2));
    ASSERT_TRUE(flag->BooleaValue(vm_));

    flag = JSNApi::NapiDeleteProperty(vm_, reinterpret_cast<uintptr_t>(*object), reinterpret_cast<uintptr_t>(*key));
    ASSERT_TRUE(flag->BooleaValue(vm_));
    flag = JSNApi::NapiDeleteProperty(vm_, reinterpret_cast<uintptr_t>(*object), reinterpret_cast<uintptr_t>(*key2));
    ASSERT_TRUE(flag->BooleaValue(vm_));

    flag = JSNApi::NapiHasOwnProperty(vm_, reinterpret_cast<uintptr_t>(*object), reinterpret_cast<uintptr_t>(*key));
    ASSERT_FALSE(flag->BooleaValue(vm_));
    flag = JSNApi::NapiHasOwnProperty(vm_, reinterpret_cast<uintptr_t>(*object), reinterpret_cast<uintptr_t>(*key2));
    ASSERT_FALSE(flag->BooleaValue(vm_));
}

HWTEST_F_L0(JSNApiTests, DeleteSerializationData)
{
    LocalScope scope(vm_);
    void *data = JSNApi::SerializeValue(vm_, StringRef::NewFromUtf8(vm_, "testData"),
        StringRef::NewFromUtf8(vm_, "testTransfer"), JSValueRef::Undefined(vm_));
    JSNApi::DeleteSerializationData(data);
    ASSERT_EQ(reinterpret_cast<ecmascript::SerializeData *>(data), nullptr);
}

HWTEST_F_L0(JSNApiTests, JSNApi_DeserializeValue_String)
{
    LocalScope scope(vm_);
    vm_->SetEnableForceGC(true);
    void *recoder = JSNApi::SerializeValue(vm_, StringRef::NewFromUtf8(vm_, "testData"), JSValueRef::Undefined(vm_),
        JSValueRef::Undefined(vm_));
    void *hint = nullptr;
    Local<JSValueRef> local = JSNApi::DeserializeValue(vm_, recoder, hint);
    ASSERT_FALSE(local->IsObject(vm_));
}

HWTEST_F_L0(JSNApiTests, NewSharedFloat64Array)
{
    LocalScope scope(vm_);
    Local<SendableArrayBufferRef> buffer = SendableArrayBufferRef::New(vm_, 57);
    ASSERT_TRUE(buffer->IsSendableArrayBuffer(vm_));

    Local<SharedFloat64ArrayRef> typedArray = SharedFloat64ArrayRef::New(vm_, buffer, 8, 6);
    ASSERT_TRUE(typedArray->IsJSSharedFloat64Array(vm_));
    ASSERT_EQ(typedArray->ByteLength(vm_), 48U); // 48 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 8U);  // 8 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U); // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_), buffer->GetBuffer(vm_));
}

HWTEST_F_L0(JSNApiTests, NewSharedBigInt64Array)
{
    LocalScope scope(vm_);
    Local<SendableArrayBufferRef> buffer = SendableArrayBufferRef::New(vm_, 57);
    ASSERT_TRUE(buffer->IsSendableArrayBuffer(vm_));

    Local<SharedBigInt64ArrayRef> typedArray = SharedBigInt64ArrayRef::New(vm_, buffer, 8, 6);
    ASSERT_TRUE(typedArray->IsJSSharedBigInt64Array(vm_));
    ASSERT_EQ(typedArray->ByteLength(vm_), 48U); // 48 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 8U);  // 8 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U); // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_), buffer->GetBuffer(vm_));
}

HWTEST_F_L0(JSNApiTests, NewSharedBigUint64Array)
{
    LocalScope scope(vm_);
    Local<SendableArrayBufferRef> buffer = SendableArrayBufferRef::New(vm_, 57);
    ASSERT_TRUE(buffer->IsSendableArrayBuffer(vm_));

    Local<SharedBigUint64ArrayRef> typedArray = SharedBigUint64ArrayRef::New(vm_, buffer, 8, 6);
    ASSERT_TRUE(typedArray->IsJSSharedBigUint64Array(vm_));
    ASSERT_EQ(typedArray->ByteLength(vm_), 48U); // 48 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 8U);  // 8 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U); // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_), buffer->GetBuffer(vm_));
}

}  // namespace panda::test
