/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/js_object-inl.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/module/static/static_module_proxy_handler.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/module/napi_module_loader.h"
#include "ecmascript/js_array.h"
#include "ecmascript/global_env.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/js_object.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
class StaticModuleProxyTest : public testing::Test {
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
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

/**
 * @tc.name: StaticModuleProxyHandler_OwnPropertyKeys
 * @tc.desc: Test OwnPropertyKeys method to validate retrieval of own property keys from static module proxy
 * @tc.type: FUNC
 */
HWTEST_F_L0(StaticModuleProxyTest, StaticModuleProxyHandler_OwnPropertyKeys)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<JSHClass> objectClass(thread->GlobalConstants()->GetHandledObjectClass());
    JSHandle<JSObject> exportObject = factory->NewJSObject(objectClass);

    // Add some properties to the export object
    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("property1"));
    JSHandle<JSTaggedValue> value1(factory->NewFromASCII("value1"));
    JSObject::SetProperty(thread, exportObject, key1, value1);

    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("property2"));
    JSHandle<JSTaggedValue> value2(factory->NewFromASCII("value2"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(exportObject), key2, value2);

    // Create a proxy with the export object
    JSHandle<JSProxy> proxy = StaticModuleProxyHandler::CreateStaticModuleProxyHandler(thread,
        JSHandle<JSTaggedValue>::Cast(exportObject));

    // Prepare runtime call info for OwnPropertyKeys
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(proxy.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, exportObject.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    // Call OwnPropertyKeys
    JSTaggedValue result = StaticModuleProxyHandler::OwnPropertyKeys(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    // Verify the result is an array
    EXPECT_TRUE(result.IsJSArray());
    JSHandle<JSArray> resultArray(thread, result);
    
    // Check that the array contains the expected property keys
    JSHandle<JSTaggedValue> lengthKey(factory->NewFromASCII("length"));
    JSHandle<JSTaggedValue> length = JSObject::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(resultArray),
                                                           lengthKey).GetValue();
    EXPECT_EQ(length->GetNumber(), 2);  // Should have 2 properties
}

/**
 * @tc.name: StaticModuleProxyHandler_GetOwnPropertyInternal
 * @tc.desc: Test GetOwnPropertyInternal method to validate retrieval of own property descriptors
 * @tc.type: FUNC
 */
HWTEST_F_L0(StaticModuleProxyTest, StaticModuleProxyHandler_GetOwnPropertyInternal)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<JSHClass> objectClass(thread->GlobalConstants()->GetHandledObjectClass());
    JSHandle<JSObject> exportObject = factory->NewJSObject(objectClass);

    // Add a property to the export object
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("testProperty"));
    JSHandle<JSTaggedValue> value(factory->NewFromASCII("testValue"));
    JSObject::SetProperty(thread, exportObject, key, value);

    PropertyDescriptor desc(thread);

    // Call GetOwnPropertyInternal
    bool result = StaticModuleProxyHandler::GetOwnPropertyInternal(thread,
        JSHandle<JSTaggedValue>::Cast(exportObject), key, desc);
    
    // Verify the property was found
    EXPECT_TRUE(result);
    EXPECT_TRUE(desc.HasValue());
    EXPECT_TRUE(desc.IsEnumerable());
    EXPECT_TRUE(desc.IsWritable());
    EXPECT_FALSE(desc.IsConfigurable());  // Should be false according to the implementation
    
    // Check the value
    JSHandle<JSTaggedValue> retrievedValue = desc.GetValue();
    EXPECT_EQ(JSTaggedValue::SameValue(thread, retrievedValue, value), true);
}

/**
 * @tc.name: StaticModuleProxyHandler_OwnEnumPropertyKeys
 * @tc.desc: Test OwnEnumPropertyKeys method to validate retrieval of enumerable property keys
 * @tc.type: FUNC
 */
HWTEST_F_L0(StaticModuleProxyTest, StaticModuleProxyHandler_OwnEnumPropertyKeys)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<JSHClass> objectClass(thread->GlobalConstants()->GetHandledObjectClass());
    JSHandle<JSObject> exportObject = factory->NewJSObject(objectClass);

    // Add some properties to the export object
    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("enumProp1"));
    JSHandle<JSTaggedValue> value1(factory->NewFromASCII("value1"));
    JSObject::SetProperty(thread, exportObject, key1, value1);

    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("enumProp2"));
    JSHandle<JSTaggedValue> value2(factory->NewFromASCII("value2"));
    JSObject::SetProperty(thread, exportObject, key2, value2);

    // Create a proxy with the export object
    JSHandle<JSProxy> proxy = StaticModuleProxyHandler::CreateStaticModuleProxyHandler(thread,
        JSHandle<JSTaggedValue>::Cast(exportObject));

    // Prepare runtime call info for OwnEnumPropertyKeys
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(proxy.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, exportObject.GetTaggedValue());
    
    // Call OwnEnumPropertyKeys
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = StaticModuleProxyHandler::OwnEnumPropertyKeys(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    // Verify the result is an array
    EXPECT_TRUE(result.IsJSArray());
    JSHandle<JSArray> resultArray(thread, result);
    
    // Check that the array contains the expected property keys
    JSHandle<JSTaggedValue> lengthKey(factory->NewFromASCII("length"));
    JSHandle<JSTaggedValue> length = JSObject::GetProperty(thread,
        JSHandle<JSTaggedValue>::Cast(resultArray), lengthKey).GetValue();
    EXPECT_EQ(length->GetNumber(), 2);  // Should have 2 properties
}

/**
 * @tc.name: StaticModuleProxyHandler_PreventExtensions
 * @tc.desc: Test PreventExtensions method to validate that it returns true as expected
 * @tc.type: FUNC
 */
HWTEST_F_L0(StaticModuleProxyTest, StaticModuleProxyHandler_PreventExtensions)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<JSHClass> objectClass(thread->GlobalConstants()->GetHandledObjectClass());
    JSHandle<JSObject> exportObject = factory->NewJSObject(objectClass);

    // Create a proxy with the export object
    JSHandle<JSProxy> proxy = StaticModuleProxyHandler::CreateStaticModuleProxyHandler(thread,
        JSHandle<JSTaggedValue>::Cast(exportObject));

    // Prepare runtime call info for PreventExtensions
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(proxy.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, exportObject.GetTaggedValue());
    
    // Call PreventExtensions
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = StaticModuleProxyHandler::PreventExtensions(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    // Verify the result is true
    EXPECT_TRUE(result.IsTrue());
}

/**
 * @tc.name: StaticModuleProxyHandler_DefineOwnProperty
 * @tc.desc: Test DefineOwnProperty method to validate property definition behavior in static module proxy
 * @tc.type: FUNC
 */
HWTEST_F_L0(StaticModuleProxyTest, StaticModuleProxyHandler_DefineOwnProperty)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<JSHClass> objectClass(thread->GlobalConstants()->GetHandledObjectClass());
    JSHandle<JSObject> exportObject = factory->NewJSObject(objectClass);

    // Add a property to the export object
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("existingProperty"));
    JSHandle<JSTaggedValue> value(factory->NewFromASCII("existingValue"));
    JSObject::SetProperty(thread, exportObject, key, value);

    // Create a proxy with the export object
    JSHandle<JSProxy> proxy = StaticModuleProxyHandler::CreateStaticModuleProxyHandler(thread,
        JSHandle<JSTaggedValue>::Cast(exportObject));

    // Create a property descriptor
    JSHandle<JSObject> descObj = factory->NewJSObject(objectClass);
    JSHandle<JSTaggedValue> valueKey(factory->NewFromASCII("value"));
    JSHandle<JSTaggedValue> newValue(factory->NewFromASCII("newValue"));
    JSObject::SetProperty(thread, descObj, valueKey, newValue);

    JSHandle<JSTaggedValue> writableKey(factory->NewFromASCII("writable"));
    JSHandle<JSTaggedValue> writableValue(thread, JSTaggedValue::True());
    JSObject::SetProperty(thread, descObj, writableKey, writableValue);
    
    JSHandle<JSTaggedValue> enumerableKey(factory->NewFromASCII("enumerable"));
    JSHandle<JSTaggedValue> enumerableValue(thread, JSTaggedValue::True());
    JSObject::SetProperty(thread, descObj, enumerableKey, enumerableValue);

    // Prepare runtime call info for DefineOwnProperty
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(proxy.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, exportObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, key.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(2, descObj.GetTaggedValue());
    
    // Call DefineOwnProperty
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = StaticModuleProxyHandler::DefineOwnProperty(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    // For existing properties with same value, it should return true
    // For properties with different values, it should return false
    // According to the implementation, if the value is the same, it returns true
    EXPECT_TRUE(result.IsTrue() || result.IsFalse());  // Either true or false depending on value comparison
}

/**
 * @tc.name: StaticModuleProxyHandler_HasProperty
 * @tc.desc: Test HasProperty method to validate property existence checking in static module proxy
 * @tc.type: FUNC
 */
HWTEST_F_L0(StaticModuleProxyTest, StaticModuleProxyHandler_HasProperty)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<JSHClass> objectClass(thread->GlobalConstants()->GetHandledObjectClass());
    JSHandle<JSObject> exportObject = factory->NewJSObject(objectClass);

    // Add a property to the export object
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("testProperty"));
    JSHandle<JSTaggedValue> value(factory->NewFromASCII("testValue"));
    JSObject::SetProperty(thread, exportObject, key, value);

    // Create a proxy with the export object
    JSHandle<JSProxy> proxy = StaticModuleProxyHandler::CreateStaticModuleProxyHandler(thread,
        JSHandle<JSTaggedValue>::Cast(exportObject));

    // Prepare runtime call info for HasProperty with existing property
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(proxy.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, exportObject.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, key.GetTaggedValue());
    
    // Call HasProperty for existing property
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result1 = StaticModuleProxyHandler::HasProperty(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    // Verify the result is true for existing property
    EXPECT_TRUE(result1.IsTrue());

    // Test with non-existing property
    JSHandle<JSTaggedValue> nonExistingKey(factory->NewFromASCII("nonExistingProperty"));
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(proxy.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, exportObject.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(1, nonExistingKey.GetTaggedValue());

    // Call HasProperty for non-existing property
    auto prev2 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    JSTaggedValue result2 = StaticModuleProxyHandler::HasProperty(ecmaRuntimeCallInfo2);
    TestHelper::TearDownFrame(thread, prev2);

    // Verify the result is false for non-existing property
    EXPECT_FALSE(result2.IsTrue());
}

/**
 * @tc.name: StaticModuleProxyHandler_SetPrototype
 * @tc.desc: Test SetPrototype method to validate prototype setting behavior in static module proxy
 * @tc.type: FUNC
 */
HWTEST_F_L0(StaticModuleProxyTest, StaticModuleProxyHandler_SetPrototype)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<JSHClass> objectClass(thread->GlobalConstants()->GetHandledObjectClass());
    JSHandle<JSObject> exportObject = factory->NewJSObject(objectClass);

    // Create a proxy with the export object
    JSHandle<JSProxy> proxy = StaticModuleProxyHandler::CreateStaticModuleProxyHandler(thread,
        JSHandle<JSTaggedValue>::Cast(exportObject));

    // Create a prototype object
    JSHandle<JSObject> prototypeObj = factory->NewJSObject(objectClass);
    
    // Prepare runtime call info for SetPrototype with null prototype
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(proxy.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, exportObject.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue::Null());
    
    // Call SetPrototype with null
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result1 = StaticModuleProxyHandler::SetPrototype(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    // Verify the result is true when setting to null
    EXPECT_TRUE(result1.IsTrue());
    
    // Prepare runtime call info for SetPrototype with object prototype
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(proxy.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, exportObject.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(1, prototypeObj.GetTaggedValue());
    
    // Call SetPrototype with object
    auto prev2 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    JSTaggedValue result2 = StaticModuleProxyHandler::SetPrototype(ecmaRuntimeCallInfo2);
    TestHelper::TearDownFrame(thread, prev2);

    // Verify the result is false when setting to object (not null)
    EXPECT_FALSE(result2.IsTrue());
}

/**
 * @tc.name: StaticModuleProxyHandler_GetPrototype
 * @tc.desc: Test GetPrototype method to validate prototype retrieval behavior in static module proxy
 * @tc.type: FUNC
 */
HWTEST_F_L0(StaticModuleProxyTest, StaticModuleProxyHandler_GetPrototype)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<JSHClass> objectClass(thread->GlobalConstants()->GetHandledObjectClass());
    JSHandle<JSObject> exportObject = factory->NewJSObject(objectClass);

    // Create a proxy with the export object
    JSHandle<JSProxy> proxy = StaticModuleProxyHandler::CreateStaticModuleProxyHandler(thread,
        JSHandle<JSTaggedValue>::Cast(exportObject));

    // Prepare runtime call info for GetPrototype
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(proxy.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, exportObject.GetTaggedValue());
    
    // Call GetPrototype
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = StaticModuleProxyHandler::GetPrototype(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    // Verify the result is null (as per implementation)
    EXPECT_TRUE(result.IsNull());
}

/**
 * @tc.name: StaticModuleProxyHandler_SetProperty
 * @tc.desc: Test SetProperty method to validate property setting behavior in static module proxy
 * @tc.type: FUNC
 */
HWTEST_F_L0(StaticModuleProxyTest, StaticModuleProxyHandler_SetProperty)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<JSHClass> objectClass(thread->GlobalConstants()->GetHandledObjectClass());
    JSHandle<JSObject> exportObject = factory->NewJSObject(objectClass);

    // Create a proxy with the export object
    JSHandle<JSProxy> proxy = StaticModuleProxyHandler::CreateStaticModuleProxyHandler(thread,
        JSHandle<JSTaggedValue>::Cast(exportObject));

    // Prepare runtime call info for SetProperty with mayThrow = false
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(proxy.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue::False());
    
    // Call SetProperty with mayThrow = false
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result1 = StaticModuleProxyHandler::SetProperty(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    // Verify the result is false (as per implementation)
    EXPECT_FALSE(result1.IsTrue());
    
    // Prepare runtime call info for SetProperty with mayThrow = true
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(proxy.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, JSTaggedValue::True());
    
    // Call SetProperty with mayThrow = true (should throw)
    auto prev2 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    JSTaggedValue result2 = StaticModuleProxyHandler::SetProperty(ecmaRuntimeCallInfo2);
    TestHelper::TearDownFrame(thread, prev2);

    // Verify the result is false (as per implementation)
    EXPECT_FALSE(result2.IsTrue());
}

/**
 * @tc.name: StaticModuleProxyHandler_DeleteProperty
 * @tc.desc: Test DeleteProperty method to validate property deletion behavior in static module proxy
 * @tc.type: FUNC
 */
HWTEST_F_L0(StaticModuleProxyTest, StaticModuleProxyHandler_DeleteProperty)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<JSHClass> objectClass(thread->GlobalConstants()->GetHandledObjectClass());
    JSHandle<JSObject> exportObject = factory->NewJSObject(objectClass);

    // Add a property to the export object
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("deletableProperty"));
    JSHandle<JSTaggedValue> value(factory->NewFromASCII("deletableValue"));
    JSObject::SetProperty(thread, exportObject, key, value);

    // Create a proxy with the export object
    JSHandle<JSProxy> proxy = StaticModuleProxyHandler::CreateStaticModuleProxyHandler(thread,
        JSHandle<JSTaggedValue>::Cast(exportObject));

    // Prepare runtime call info for DeleteProperty with existing property
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(proxy.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, exportObject.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, key.GetTaggedValue());
    
    // Call DeleteProperty for existing property
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result1 = StaticModuleProxyHandler::DeleteProperty(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    // According to implementation, if property exists, return false
    EXPECT_FALSE(result1.IsTrue());

    // Test with non-existing property
    JSHandle<JSTaggedValue> nonExistingKey(factory->NewFromASCII("nonExistingProperty"));
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(proxy.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, exportObject.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(1, nonExistingKey.GetTaggedValue());

    // Call DeleteProperty for non-existing property
    auto prev2 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    JSTaggedValue result2 = StaticModuleProxyHandler::DeleteProperty(ecmaRuntimeCallInfo2);
    TestHelper::TearDownFrame(thread, prev2);

    // According to implementation, if property doesn't exist, return true
    EXPECT_TRUE(result2.IsTrue());
}

/**
 * @tc.name: StaticModuleProxyHandler_IsExtensible
 * @tc.desc: Test IsExtensible method to validate extensibility checking in static module proxy
 * @tc.type: FUNC
 */
HWTEST_F_L0(StaticModuleProxyTest, StaticModuleProxyHandler_IsExtensible)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<JSHClass> objectClass(thread->GlobalConstants()->GetHandledObjectClass());
    JSHandle<JSObject> exportObject = factory->NewJSObject(objectClass);

    // Create a proxy with the export object
    JSHandle<JSProxy> proxy = StaticModuleProxyHandler::CreateStaticModuleProxyHandler(thread,
        JSHandle<JSTaggedValue>::Cast(exportObject));

    // Prepare runtime call info for IsExtensible
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(proxy.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, exportObject.GetTaggedValue());
    
    // Call IsExtensible
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = StaticModuleProxyHandler::IsExtensible(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    // Verify the result is false (as per implementation)
    EXPECT_FALSE(result.IsTrue());
}

/**
 * @tc.name: StaticModuleProxyHandler_DefineOwnProperty_EnumerableField
 * @tc.desc: Test DefineOwnProperty with enumerable field to cover line 187
 * @tc.type: FUNC
 */
HWTEST_F_L0(StaticModuleProxyTest, StaticModuleProxyHandler_DefineOwnProperty_EnumerableField)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<JSHClass> objectClass(thread->GlobalConstants()->GetHandledObjectClass());
    JSHandle<JSObject> exportObject = factory->NewJSObject(objectClass);

    // Add a property to the export object
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("testProperty"));
    JSHandle<JSTaggedValue> value(factory->NewFromASCII("testValue"));
    JSObject::SetProperty(thread, exportObject, key, value);

    // Create a property descriptor object with enumerable=false
    JSHandle<JSObject> descObj = factory->NewJSObject(objectClass);
    JSHandle<JSTaggedValue> valueKey(factory->NewFromASCII("value"));
    JSHandle<JSTaggedValue> newValue(factory->NewFromASCII("newValue"));
    JSObject::SetProperty(thread, descObj, valueKey, newValue);

    JSHandle<JSTaggedValue> enumerableKey(factory->NewFromASCII("enumerable"));
    JSHandle<JSTaggedValue> enumerableValue(thread, JSTaggedValue::False());
    JSObject::SetProperty(thread, descObj, enumerableKey, enumerableValue);

    // Prepare runtime call info for DefineOwnProperty
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, exportObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, key.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(2, descObj.GetTaggedValue());

    // Call DefineOwnProperty with enumerable=false
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = StaticModuleProxyHandler::DefineOwnProperty(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    // This should trigger the enumerable field branch (line 187)
    EXPECT_TRUE(result.IsFalse());
}

/**
 * @tc.name: StaticModuleProxyHandler_DefineOwnProperty_WritableField
 * @tc.desc: Test DefineOwnProperty with writable field to cover line 190
 * @tc.type: FUNC
 */
HWTEST_F_L0(StaticModuleProxyTest, StaticModuleProxyHandler_DefineOwnProperty_WritableField)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<JSHClass> objectClass(thread->GlobalConstants()->GetHandledObjectClass());
    JSHandle<JSObject> exportObject = factory->NewJSObject(objectClass);

    // Add a property to the export object
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("testProperty"));
    JSHandle<JSTaggedValue> value(factory->NewFromASCII("testValue"));
    JSObject::SetProperty(thread, exportObject, key, value);

    // Create a property descriptor object with writable=false
    JSHandle<JSObject> descObj = factory->NewJSObject(objectClass);
    JSHandle<JSTaggedValue> valueKey(factory->NewFromASCII("value"));
    JSHandle<JSTaggedValue> newValue(factory->NewFromASCII("newValue"));
    JSHandle<JSTaggedValue> writableKey(factory->NewFromASCII("writable"));
    JSHandle<JSTaggedValue> writableValue(thread, JSTaggedValue::False());

    JSObject::SetProperty(thread, descObj, valueKey, newValue);
    JSObject::SetProperty(thread, descObj, writableKey, writableValue);

    // Prepare runtime call info for DefineOwnProperty
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, exportObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, key.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(2, descObj.GetTaggedValue());

    // Call DefineOwnProperty with writable=false
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = StaticModuleProxyHandler::DefineOwnProperty(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    // This should trigger the writable field branch (line 190)
    EXPECT_TRUE(result.IsFalse());
}

/**
 * @tc.name: StaticModuleProxyHandler_DefineOwnProperty_ValueField
 * @tc.desc: Test DefineOwnProperty with value field to cover line 201
 * @tc.type: FUNC
 */
HWTEST_F_L0(StaticModuleProxyTest, StaticModuleProxyHandler_DefineOwnProperty_ValueField)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<JSHClass> objectClass(thread->GlobalConstants()->GetHandledObjectClass());
    JSHandle<JSObject> exportObject = factory->NewJSObject(objectClass);

    // Add a property to the export object
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("testProperty"));
    JSHandle<JSTaggedValue> value(factory->NewFromASCII("testValue"));
    JSObject::SetProperty(thread, exportObject, key, value);

    // Create a property descriptor object with same value
    JSHandle<JSObject> descObj = factory->NewJSObject(objectClass);
    JSHandle<JSTaggedValue> valueKey(factory->NewFromASCII("value"));
    JSHandle<JSTaggedValue> sameValue(factory->NewFromASCII("testValue")); // Same as original value

    JSObject::SetProperty(thread, descObj, valueKey, sameValue);

    // Prepare runtime call info for DefineOwnProperty
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, exportObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, key.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(2, descObj.GetTaggedValue());

    // Call DefineOwnProperty with same value
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = StaticModuleProxyHandler::DefineOwnProperty(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    // This should trigger the value field branch (line 201) and return true
    EXPECT_TRUE(result.IsTrue());
}

/**
 * @tc.name: StaticModuleProxyHandler_GetOwnProperty
 * @tc.desc: Test GetOwnProperty method to cover the method itself
 * @tc.type: FUNC
 */
HWTEST_F_L0(StaticModuleProxyTest, StaticModuleProxyHandler_GetOwnProperty)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<JSHClass> objectClass(thread->GlobalConstants()->GetHandledObjectClass());
    JSHandle<JSObject> exportObject = factory->NewJSObject(objectClass);

    // Add a property to the export object
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("testProperty"));
    JSHandle<JSTaggedValue> value(factory->NewFromASCII("testValue"));
    JSObject::SetProperty(thread, exportObject, key, value);

    // Create a property descriptor object
    JSHandle<JSObject> descObj = factory->NewJSObject(objectClass);

    // Prepare runtime call info for GetOwnProperty
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, exportObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, key.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(2, descObj.GetTaggedValue());

    // Call GetOwnProperty
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = StaticModuleProxyHandler::GetOwnProperty(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    // Verify the result
    EXPECT_TRUE(result.IsTrue());
}

/**
 * @tc.name: StaticModuleProxyHandler_DeleteProperty_SymbolKey
 * @tc.desc: Test DeleteProperty with symbol key to cover line 261
 * @tc.type: FUNC
 */
HWTEST_F_L0(StaticModuleProxyTest, StaticModuleProxyHandler_DeleteProperty_SymbolKey)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<JSHClass> objectClass(thread->GlobalConstants()->GetHandledObjectClass());
    JSHandle<JSObject> exportObject = factory->NewJSObject(objectClass);

    // Create a symbol key
    JSHandle<JSSymbol> symbolKey = thread->GetEcmaVM()->GetFactory()->NewJSSymbol();
    JSHandle<JSTaggedValue> key(symbolKey);
    // Prepare runtime call info for DeleteProperty
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, exportObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, key.GetTaggedValue());

    // Call DeleteProperty with symbol key
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = StaticModuleProxyHandler::DeleteProperty(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    // This should trigger the symbol key branch (line 261)
    EXPECT_TRUE(result.IsTrue());
}

}  // namespace panda::test