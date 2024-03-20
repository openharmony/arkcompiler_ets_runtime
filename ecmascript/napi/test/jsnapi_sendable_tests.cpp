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
#include "gtest/gtest.h"

// index 0 is name/constructor, index 1 is property to test.
static constexpr int INITIAL_PROPERTY_LENGTH = 2;

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
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
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
    EcmaVM *vm, const char *instanceKey, const char *staticKey, const char *nonStaticKey, Local<FunctionRef> parent)
{
    Local<panda::ArrayRef> instanceKeys = panda::ArrayRef::New(vm, 1);
    Local<panda::ArrayRef> instanceValues = panda::ArrayRef::New(vm, 1);
    PropertyAttribute *instanceAttributes = new PropertyAttribute[1];
    // index 0 is name
    Local<panda::ArrayRef> staticKeys = panda::ArrayRef::New(vm, INITIAL_PROPERTY_LENGTH);
    Local<panda::ArrayRef> staticValues = panda::ArrayRef::New(vm, INITIAL_PROPERTY_LENGTH);
    PropertyAttribute *staticAttributes = new PropertyAttribute[INITIAL_PROPERTY_LENGTH];
    // index 0 is constructor
    Local<panda::ArrayRef> nonStaticKeys = panda::ArrayRef::New(vm, INITIAL_PROPERTY_LENGTH);
    Local<panda::ArrayRef> nonStaticValues = panda::ArrayRef::New(vm, INITIAL_PROPERTY_LENGTH);
    PropertyAttribute *nonStaticAttributes = new PropertyAttribute[INITIAL_PROPERTY_LENGTH];

    Local<StringRef> instanceStr = StringRef::NewFromUtf8(vm, instanceKey);
    instanceKeys->Set(vm, 0, instanceStr);
    instanceValues->Set(vm, 0, instanceStr);
    instanceAttributes[0] = PropertyAttribute(instanceStr, true, true, true);

    Local<StringRef> staticStr = StringRef::NewFromUtf8(vm, staticKey);
    staticKeys->Set(vm, 1, staticStr);
    staticValues->Set(vm, 1, staticStr);
    staticAttributes[1] = PropertyAttribute(staticStr, true, true, true);

    Local<StringRef> nonStaticStr = StringRef::NewFromUtf8(vm, nonStaticKey);
    nonStaticKeys->Set(vm, 1, nonStaticStr);
    nonStaticValues->Set(vm, 1, nonStaticStr);
    nonStaticAttributes[1] = PropertyAttribute(nonStaticStr, true, true, true);

    Local<StringRef> nameStr = StringRef::NewFromUtf8(vm, "name");
    Local<FunctionRef> constructor =
        FunctionRef::NewSendableClassFunction(vm, FunctionCallback, nullptr, nullptr, nameStr,
                                              {{instanceKeys, instanceValues, instanceAttributes},
                                               {staticKeys, staticValues, staticAttributes},
                                               {nonStaticKeys, nonStaticValues, nonStaticAttributes}},
                                              parent);

    return constructor;
}

HWTEST_F_L0(JSNApiTests, NewSendableClassFunction)
{
    LocalScope scope(vm_);
    Local<FunctionRef> constructor =
        GetNewSendableClassFunction(vm_, "instance", "static", "nonStatic", FunctionRef::Null(vm_));

    ASSERT_EQ("name", constructor->GetName(vm_)->ToString());
    ASSERT_TRUE(constructor->IsFunction());
    JSHandle<JSTaggedValue> jsConstructor = JSNApiHelper::ToJSHandle(constructor);
    ASSERT_TRUE(jsConstructor->IsClassConstructor());

    Local<JSValueRef> prototype = constructor->GetFunctionPrototype(vm_);
    ASSERT_TRUE(prototype->IsObject());
    JSHandle<JSTaggedValue> jsPrototype = JSNApiHelper::ToJSHandle(prototype);
    ASSERT_TRUE(jsPrototype->IsClassPrototype());
}

HWTEST_F_L0(JSNApiTests, NewSendableClassFunctionProperties)
{
    LocalScope scope(vm_);
    Local<FunctionRef> constructor =
        GetNewSendableClassFunction(vm_, "instance", "static", "nonStatic", FunctionRef::Null(vm_));
    Local<ObjectRef> prototype = constructor->GetFunctionPrototype(vm_);

    ASSERT_EQ("static", constructor->Get(vm_, staticKey)->ToString(vm_)->ToString());
    ASSERT_EQ("nonStatic", prototype->Get(vm_, nonStaticKey)->ToString(vm_)->ToString());
    ASSERT_EQ("undefined", constructor->Get(vm_, invalidKey)->ToString(vm_)->ToString());
    ASSERT_EQ("undefined", prototype->Get(vm_, invalidKey)->ToString(vm_)->ToString());

    // set static property on constructor
    constructor->Set(vm_, staticKey, StringRef::NewFromUtf8(vm_, "static0"));
    ASSERT_EQ("static0", constructor->Get(vm_, staticKey)->ToString(vm_)->ToString());

    // set non static property on prototype
    prototype->Set(vm_, nonStaticKey, StringRef::NewFromUtf8(vm_, "nonStatic0"));
    ASSERT_EQ("nonStatic0", prototype->Get(vm_, nonStaticKey)->ToString(vm_)->ToString());

    // set invalid property on constructor
    ASSERT_FALSE(vm_->GetJSThread()->HasPendingException());
    constructor->Set(vm_, invalidKey, StringRef::NewFromUtf8(vm_, "invalid"));
    ASSERT_TRUE(vm_->GetJSThread()->HasPendingException());
    JSNApi::GetAndClearUncaughtException(vm_);
    ASSERT_EQ("undefined", constructor->Get(vm_, invalidKey)->ToString(vm_)->ToString());

    // set invalid property on prototype
    ASSERT_FALSE(vm_->GetJSThread()->HasPendingException());
    prototype->Set(vm_, invalidKey, StringRef::NewFromUtf8(vm_, "invalid"));
    ASSERT_TRUE(vm_->GetJSThread()->HasPendingException());
    JSNApi::GetAndClearUncaughtException(vm_);
    ASSERT_EQ("undefined", prototype->Get(vm_, invalidKey)->ToString(vm_)->ToString());
}

HWTEST_F_L0(JSNApiTests, NewSendableClassFunctionInstance)
{
    LocalScope scope(vm_);
    Local<FunctionRef> constructor =
        GetNewSendableClassFunction(vm_, "instance", "static", "nonStatic", FunctionRef::Null(vm_));
    Local<JSValueRef> argv[1] = {NumberRef::New(vm_, 0)};
    Local<ObjectRef> obj = constructor->Constructor(vm_, argv, 0);
    Local<ObjectRef> obj0 = constructor->Constructor(vm_, argv, 0);

    ASSERT_TRUE(JSFunction::InstanceOf(thread_, JSNApiHelper::ToJSHandle(obj), JSNApiHelper::ToJSHandle(constructor)));
    ASSERT_TRUE(JSFunction::InstanceOf(thread_, JSNApiHelper::ToJSHandle(obj0), JSNApiHelper::ToJSHandle(constructor)));

    // set instance property
    ASSERT_EQ("undefined", obj->Get(vm_, instanceKey)->ToString(vm_)->ToString());
    ASSERT_EQ("undefined", obj0->Get(vm_, instanceKey)->ToString(vm_)->ToString());
    obj->Set(vm_, instanceKey, StringRef::NewFromUtf8(vm_, "instance"));
    ASSERT_EQ("instance", obj->Get(vm_, instanceKey)->ToString(vm_)->ToString());
    ASSERT_EQ("undefined", obj0->Get(vm_, instanceKey)->ToString(vm_)->ToString());

    // set non static property on prototype and get from instance
    ASSERT_EQ("nonStatic", obj->Get(vm_, nonStaticKey)->ToString(vm_)->ToString());
    ASSERT_EQ("nonStatic", obj0->Get(vm_, nonStaticKey)->ToString(vm_)->ToString());
    Local<ObjectRef> prototype = obj->GetPrototype(vm_);
    prototype->Set(vm_, nonStaticKey, StringRef::NewFromUtf8(vm_, "nonStatic0"));
    ASSERT_EQ("nonStatic0", obj->Get(vm_, nonStaticKey)->ToString(vm_)->ToString());
    ASSERT_EQ("nonStatic0", obj0->Get(vm_, nonStaticKey)->ToString(vm_)->ToString());

    // set non static property on instance
    ASSERT_FALSE(vm_->GetJSThread()->HasPendingException());
    obj->Set(vm_, nonStaticKey, StringRef::NewFromUtf8(vm_, "nonStatic1"));
    ASSERT_TRUE(vm_->GetJSThread()->HasPendingException());
    JSNApi::GetAndClearUncaughtException(vm_);
    ASSERT_EQ("nonStatic0", obj->Get(vm_, nonStaticKey)->ToString(vm_)->ToString());
    ASSERT_EQ("nonStatic0", obj0->Get(vm_, nonStaticKey)->ToString(vm_)->ToString());

    // set invalid property on instance
    ASSERT_EQ("undefined", obj->Get(vm_, invalidKey)->ToString(vm_)->ToString());
    ASSERT_FALSE(vm_->GetJSThread()->HasPendingException());
    obj->Set(vm_, invalidKey, StringRef::NewFromUtf8(vm_, "invalid"));
    ASSERT_TRUE(vm_->GetJSThread()->HasPendingException());
    JSNApi::GetAndClearUncaughtException(vm_);
    ASSERT_EQ("undefined", obj->Get(vm_, invalidKey)->ToString(vm_)->ToString());
}

HWTEST_F_L0(JSNApiTests, NewSendableClassFunctionInherit)
{
    LocalScope scope(vm_);
    Local<FunctionRef> parent =
        GetNewSendableClassFunction(vm_, "parentInstance", "parentStatic", "parentNonStatic", FunctionRef::Null(vm_));
    Local<FunctionRef> constructor = GetNewSendableClassFunction(vm_, "instance", "static", "nonStatic", parent);
    Local<JSValueRef> argv[1] = {NumberRef::New(vm_, 0)};
    Local<ObjectRef> obj = constructor->Constructor(vm_, argv, 0);
    Local<ObjectRef> obj0 = constructor->Constructor(vm_, argv, 0);

    ASSERT_TRUE(JSFunction::InstanceOf(thread_, JSNApiHelper::ToJSHandle(obj), JSNApiHelper::ToJSHandle(parent)));
    ASSERT_TRUE(JSFunction::InstanceOf(thread_, JSNApiHelper::ToJSHandle(obj0), JSNApiHelper::ToJSHandle(parent)));

    // set parent instance property on instance
    Local<StringRef> parentInstanceKey = StringRef::NewFromUtf8(vm_, "parentInstance");
    ASSERT_EQ("undefined", obj->Get(vm_, parentInstanceKey)->ToString(vm_)->ToString());
    ASSERT_FALSE(vm_->GetJSThread()->HasPendingException());
    obj->Set(vm_, parentInstanceKey, StringRef::NewFromUtf8(vm_, "parentInstance"));
    ASSERT_TRUE(vm_->GetJSThread()->HasPendingException());
    JSNApi::GetAndClearUncaughtException(vm_);
    ASSERT_EQ("undefined", obj->Get(vm_, parentInstanceKey)->ToString(vm_)->ToString());

    // get parent static property from constructor
    Local<StringRef> parentStaticKey = StringRef::NewFromUtf8(vm_, "parentStatic");
    ASSERT_EQ("parentStatic", constructor->Get(vm_, parentStaticKey)->ToString(vm_)->ToString());

    // get parent non static property form instance
    Local<StringRef> parentNonStaticKey = StringRef::NewFromUtf8(vm_, "parentNonStatic");
    ASSERT_EQ("parentNonStatic", obj->Get(vm_, parentNonStaticKey)->ToString(vm_)->ToString());
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
    ASSERT_EQ("funcResult", res->ToString(vm_)->ToString());
}

HWTEST_F_L0(JSNApiTests, NewSendableClassFunctionFunction)
{
    LocalScope scope(vm_);

    Local<panda::ArrayRef> instanceKeys = panda::ArrayRef::New(vm_, 0);
    Local<panda::ArrayRef> instanceValues = panda::ArrayRef::New(vm_, 0);
    PropertyAttribute *instanceAttributes = new PropertyAttribute[0];
    // index 0 is name
    Local<panda::ArrayRef> staticKeys = panda::ArrayRef::New(vm_, INITIAL_PROPERTY_LENGTH);
    Local<panda::ArrayRef> staticValues = panda::ArrayRef::New(vm_, INITIAL_PROPERTY_LENGTH);
    PropertyAttribute *staticAttributes = new PropertyAttribute[INITIAL_PROPERTY_LENGTH];
    // index 0 is constructor
    Local<panda::ArrayRef> nonStaticKeys = panda::ArrayRef::New(vm_, 1);
    Local<panda::ArrayRef> nonStaticValues = panda::ArrayRef::New(vm_, 1);
    PropertyAttribute *nonStaticAttributes = new PropertyAttribute[1];

    Local<FunctionRef> func = FunctionRef::NewSendable(
        vm_,
        [](JsiRuntimeCallInfo *runtimeInfo) -> JSValueRef {
            EcmaVM *vm = runtimeInfo->GetVM();
            LocalScope scope(vm);
            return **StringRef::NewFromUtf8(vm, "funcResult");
        },
        nullptr);
    staticKeys->Set(vm_, 1, staticKey);
    staticValues->Set(vm_, 1, func);
    staticAttributes[1] = PropertyAttribute(func, true, true, true);

    Local<FunctionRef> constructor = FunctionRef::NewSendableClassFunction(
        vm_, FunctionCallback, nullptr, nullptr, StringRef::NewFromUtf8(vm_, "name"),
        {{instanceKeys, instanceValues, instanceAttributes},
         {staticKeys, staticValues, staticAttributes},
         {nonStaticKeys, nonStaticValues, nonStaticAttributes}},
        FunctionRef::Null(vm_));

    Local<FunctionRef> staticValue = constructor->Get(vm_, staticKey);
    ASSERT_TRUE(staticValue->IsFunction());
    Local<JSValueRef> res = staticValue->Call(vm_, JSValueRef::Undefined(vm_), nullptr, 0);
    ASSERT_EQ("funcResult", res->ToString(vm_)->ToString());
}

HWTEST_F_L0(JSNApiTests, NewSendableClassFunctionGetterSetter)
{
    LocalScope scope(vm_);

    Local<panda::ArrayRef> instanceKeys = panda::ArrayRef::New(vm_, 0);
    Local<panda::ArrayRef> instanceValues = panda::ArrayRef::New(vm_, 0);
    PropertyAttribute *instanceAttributes = new PropertyAttribute[0];
    // index 0 is name
    Local<panda::ArrayRef> staticKeys = panda::ArrayRef::New(vm_, 3);
    Local<panda::ArrayRef> staticValues = panda::ArrayRef::New(vm_, 3);
    PropertyAttribute *staticAttributes = new PropertyAttribute[3];
    // index 0 is constructor
    Local<panda::ArrayRef> nonStaticKeys = panda::ArrayRef::New(vm_, 1);
    Local<panda::ArrayRef> nonStaticValues = panda::ArrayRef::New(vm_, 1);
    PropertyAttribute *nonStaticAttributes = new PropertyAttribute[1];

    Local<StringRef> getterSetter = StringRef::NewFromUtf8(vm_, "getterSetter");
    staticKeys->Set(vm_, 1, getterSetter);
    staticValues->Set(vm_, 1, getterSetter);
    staticAttributes[1] = PropertyAttribute(getterSetter, true, true, true);
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
    staticKeys->Set(vm_, 2, staticKey);
    staticValues->Set(vm_, 2, staticValue);
    staticAttributes[2] = PropertyAttribute(staticValue, true, true, true);

    Local<FunctionRef> constructor = FunctionRef::NewSendableClassFunction(
        vm_, FunctionCallback, nullptr, nullptr, StringRef::NewFromUtf8(vm_, "name"),
        {{instanceKeys, instanceValues, instanceAttributes},
         {staticKeys, staticValues, staticAttributes},
         {nonStaticKeys, nonStaticValues, nonStaticAttributes}},
        FunctionRef::Null(vm_));

    ASSERT_EQ("getterSetter", constructor->Get(vm_, staticKey)->ToString(vm_)->ToString());
    constructor->Set(vm_, staticKey, StringRef::NewFromUtf8(vm_, "getterSetter0"));
    ASSERT_EQ("getterSetter0", constructor->Get(vm_, staticKey)->ToString(vm_)->ToString());
}

}  // namespace panda::test
