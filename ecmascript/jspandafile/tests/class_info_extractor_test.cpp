/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ecmascript/global_env.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/lexical_env.h"
#define private public
#define protected public
#include "ecmascript/shared_objects/js_shared_object.h"
#include "ecmascript/shared_objects/js_shared_array.h"
#include "ecmascript/shared_objects/js_shared_map.h"
#include "ecmascript/shared_objects/js_shared_set.h"
#include "ecmascript/shared_objects/js_sendable_arraybuffer.h"
#include "ecmascript/shared_objects/js_shared_typed_array.h"
#include "ecmascript/jspandafile/class_info_extractor.h"
#include "ecmascript/js_api/js_api_bitvector.h"
#undef protected
#undef private

using namespace panda::ecmascript;
namespace panda::test {
class ClassInfoExtractorTest : public BaseTestWithScope<true> {
};

/**
 * @tc.name: MatchFieldType_ElseCondition
 * @tc.desc: Test MatchFieldType method to cover all switch case branches
 * @tc.type: FUNC
 * @tc.require: issue#12083
 */

HWTEST_F_L0(ClassInfoExtractorTest, MatchFieldType_ElseCondition)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // Test the else branch of MatchFieldType
    // This branch is executed when none of the previous conditions match
    // For example: if we have a SharedFieldType::NUMBER but value is not a number type

    // Test with NUMBER field type and STRING value (should return false)
    JSHandle<EcmaString> testString = factory->NewFromStdString("test_string");
    bool result1 = ClassHelper::MatchFieldType(
        SharedFieldType::NUMBER,
        testString.GetTaggedValue());
    EXPECT_FALSE(result1);  // Should return false, hitting the else branch

    // Test with BOOLEAN field type and OBJECT value (should return false)
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(
        JSHandle<JSFunction>(env->GetObjectFunction()),
        env->GetObjectFunction());
    bool result2 = ClassHelper::MatchFieldType(
        SharedFieldType::BOOLEAN,
        obj.GetTaggedValue());
    EXPECT_FALSE(result2);  // Should return false, hitting the else branch

    // Test with STRING field type and NUMBER value (should return false)
    bool result3 = ClassHelper::MatchFieldType(
        SharedFieldType::STRING,
        JSTaggedValue(42));
    EXPECT_FALSE(result3);  // Should return false, hitting the else branch

    // Test with BIG_INT field type and BOOLEAN value (should return false)
    bool result4 = ClassHelper::MatchFieldType(
        SharedFieldType::BIG_INT,
        JSTaggedValue(true));
    EXPECT_FALSE(result4);  // Should return false, hitting the else branch


    bool result5 = ClassHelper::MatchFieldType(
        SharedFieldType::UNDEFINED,
        JSTaggedValue::Undefined());
    EXPECT_TRUE(result5);  // Should return true, hitting the else branch
}

/**
 * @tc.name: StaticFieldTypeToString_AllBranches
 * @tc.desc: Test StaticFieldTypeToString method to cover all switch case branches
 * @tc.type: FUNC
 * @tc.require: issue#12067
 */
HWTEST_F_L0(ClassInfoExtractorTest, StaticFieldTypeToString_AllBranches)
{
    // Test NONE field type
    CString result1 = ClassHelper::StaticFieldTypeToString(
        static_cast<uint32_t>(SharedFieldType::NONE));
    EXPECT_STREQ(result1.c_str(), "[None]");

    // Test NUMBER field type
    CString result2 = ClassHelper::StaticFieldTypeToString(
        static_cast<uint32_t>(SharedFieldType::NUMBER));
    EXPECT_STREQ(result2.c_str(), "[Number]");

    // Test STRING field type
    CString result3 = ClassHelper::StaticFieldTypeToString(
        static_cast<uint32_t>(SharedFieldType::STRING));
    EXPECT_STREQ(result3.c_str(), "[String]");

    // Test BOOLEAN field type
    CString result4 = ClassHelper::StaticFieldTypeToString(
        static_cast<uint32_t>(SharedFieldType::BOOLEAN));
    EXPECT_STREQ(result4.c_str(), "[Boolean]");

    // Test SENDABLE field type
    CString result5 = ClassHelper::StaticFieldTypeToString(
        static_cast<uint32_t>(SharedFieldType::SENDABLE));
    EXPECT_STREQ(result5.c_str(), "[Sendable Object]");

    // Test BIG_INT field type
    CString result6 = ClassHelper::StaticFieldTypeToString(
        static_cast<uint32_t>(SharedFieldType::BIG_INT));
    EXPECT_STREQ(result6.c_str(), "[BigInt]");

    // Test GENERIC field type
    CString result7 = ClassHelper::StaticFieldTypeToString(
        static_cast<uint32_t>(SharedFieldType::GENERIC));
    EXPECT_STREQ(result7.c_str(), "[Generic]");

    // Test NULL_TYPE field type
    CString result8 = ClassHelper::StaticFieldTypeToString(
        static_cast<uint32_t>(SharedFieldType::NULL_TYPE));
    EXPECT_STREQ(result8.c_str(), "[Null]");

    // Test UNDEFINED field type
    CString result9 = ClassHelper::StaticFieldTypeToString(
        static_cast<uint32_t>(SharedFieldType::UNDEFINED));
    EXPECT_STREQ(result9.c_str(), "[Undefined]");

    // Test default case with an unknown field type
    CString result10 = ClassHelper::StaticFieldTypeToString(999); // Unknown type
    EXPECT_NE(result10.find("unknown type"), std::string::npos);
    EXPECT_NE(result10.find("999"), std::string::npos);
}

/**
 * @tc.name: GetSizeAndMaxInlineByType_AllBranches
 * @tc.desc: Test GetSizeAndMaxInlineByType method directly to cover all switch case branches and default condition
 * @tc.type: FUNC
 * @tc.require: issue#12068
 */
HWTEST_F_L0(ClassInfoExtractorTest, GetSizeAndMaxInlineByType_AllBranches)
{
    // Test JS_SHARED_OBJECT case
    auto result1 = SendableClassDefiner::GetSizeAndMaxInlineByType(
        JSType::JS_SHARED_OBJECT);
    EXPECT_EQ(result1.first, JSSharedObject::SIZE);
    EXPECT_EQ(result1.second, JSSharedObject::MAX_INLINE);

    // Test JS_SHARED_ARRAY case
    auto result2 = SendableClassDefiner::GetSizeAndMaxInlineByType(
        JSType::JS_SHARED_ARRAY);
    EXPECT_EQ(result2.first, JSSharedArray::SIZE);
    EXPECT_EQ(result2.second, JSSharedArray::MAX_INLINE);

    // Test JS_SHARED_MAP case
    auto result3 = SendableClassDefiner::GetSizeAndMaxInlineByType(
        JSType::JS_SHARED_MAP);
    EXPECT_EQ(result3.first, JSSharedMap::SIZE);
    EXPECT_EQ(result3.second, JSSharedMap::MAX_INLINE);

    // Test JS_SHARED_SET case
    auto result4 = SendableClassDefiner::GetSizeAndMaxInlineByType(
        JSType::JS_SHARED_SET);
    EXPECT_EQ(result4.first, JSSharedSet::SIZE);
    EXPECT_EQ(result4.second, JSSharedSet::MAX_INLINE);

    // Test JS_SENDABLE_ARRAY_BUFFER case
    auto result5 = SendableClassDefiner::GetSizeAndMaxInlineByType(
        JSType::JS_SENDABLE_ARRAY_BUFFER);
    EXPECT_EQ(result5.first, JSSendableArrayBuffer::SIZE);
    EXPECT_EQ(result5.second, JSSendableArrayBuffer::MAX_INLINE);

    // Test JS_API_BITVECTOR case
    auto result6 = SendableClassDefiner::GetSizeAndMaxInlineByType(
        JSType::JS_API_BITVECTOR);
    EXPECT_EQ(result6.first, JSAPIBitVector::SIZE);
    EXPECT_EQ(result6.second, JSAPIBitVector::MAX_INLINE);

    // Test the condition JSType::JS_SHARED_TYPED_ARRAY_FIRST < type && type <= JSType::JS_SHARED_TYPED_ARRAY_LAST
    auto result7 = SendableClassDefiner::GetSizeAndMaxInlineByType(
        static_cast<JSType>(static_cast<int>(JSType::JS_SHARED_TYPED_ARRAY_FIRST) + 1));
    EXPECT_EQ(result7.first, JSSharedTypedArray::SIZE);
    EXPECT_EQ(result7.second, JSSharedTypedArray::MAX_INLINE);
}

/**
 * @tc.name: BuildDictionaryProperties_AllBranches
 * @tc.desc: Test BuildDictionaryProperties method directly to cover all branches
 * @tc.type: FUNC
 * @tc.require: issue#12066
 */
HWTEST_F_L0(ClassInfoExtractorTest, BuildDictionaryProperties_AllBranches)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // Create keys and properties arrays with more than MAX_FAST_PROPS_CAPACITY elements to trigger dictionary mode
    uint32_t length = panda::ecmascript::PropertyAttributes::MAX_FAST_PROPS_CAPACITY + 5;
    JSHandle<TaggedArray> keys = factory->NewOldSpaceTaggedArray(length);
    JSHandle<TaggedArray> properties = factory->NewOldSpaceTaggedArray(length);

    // Set up static properties with required indices - testing the switch statement branches
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    keys->Set(thread, panda::ecmascript::ClassInfoExtractor::LENGTH_INDEX, globalConst->GetLengthString());
    keys->Set(thread, panda::ecmascript::ClassInfoExtractor::NAME_INDEX, globalConst->GetNameString());
    keys->Set(thread, panda::ecmascript::ClassInfoExtractor::PROTOTYPE_INDEX, globalConst->GetPrototypeString());

    // Set string value for name property (testing non-accessor name branch)
    JSHandle<EcmaString> nameStr = factory->NewFromStdString("TestClass");
    properties->Set(thread, panda::ecmascript::ClassInfoExtractor::NAME_INDEX, nameStr);
    properties->Set(thread, panda::ecmascript::ClassInfoExtractor::LENGTH_INDEX, JSTaggedValue(1));
    properties->Set(thread, panda::ecmascript::ClassInfoExtractor::PROTOTYPE_INDEX,
                    globalConst->GetHandledFunctionPrototypeAccessor());

    // Fill remaining slots with generic values
    for (uint32_t i = panda::ecmascript::ClassInfoExtractor::PROTOTYPE_INDEX + 1; i < length; i++) {
        JSHandle<EcmaString> key = factory->NewFromStdString("key" + std::to_string(i));
        keys->Set(thread, i, key);
        properties->Set(thread, i, JSTaggedValue(i));
    }

    // Create an object to pass to the method
    JSHandle<JSObject> object = factory->NewJSObjectByConstructor(
        JSHandle<JSFunction>(thread->GetEcmaVM()->GetGlobalEnv()->GetObjectFunction()),
        thread->GetEcmaVM()->GetGlobalEnv()->GetObjectFunction());

    // Call BuildDictionaryProperties directly (now possible due to friend declaration)
    JSHandle<panda::ecmascript::NameDictionary> staticDict =
        ClassHelper::BuildDictionaryProperties(thread, object, keys, properties,
                                                                    panda::ecmascript::ClassPropertyType::STATIC,
                                                                    JSHandle<JSTaggedValue>(object));

    EXPECT_FALSE(JSHandle<JSTaggedValue>::Cast(staticDict)->IsUndefined());
    EXPECT_GE(staticDict->EntriesCount(), 3); // At least the 3 special keys should be processed

    // Test with non-static property type (default attributes branch)
    JSHandle<panda::ecmascript::NameDictionary> nonStaticDict =
        ClassHelper::BuildDictionaryProperties(thread, object, keys, properties,
                                                                    panda::ecmascript::ClassPropertyType::NON_STATIC,
                                                                    JSHandle<JSTaggedValue>(object));

    EXPECT_FALSE(JSHandle<JSTaggedValue>::Cast(nonStaticDict)->IsUndefined());

    // Create function template to test function template branch
    JSHandle<JSFunction> func = factory->NewJSFunction(thread->GetEcmaVM()->GetGlobalEnv());
    JSHandle<Method> funcMethod(thread, func->GetMethod(thread));
    JSHandle<JSTaggedValue> handleUndefined(thread, JSTaggedValue::Undefined());
    JSHandle<panda::ecmascript::FunctionTemplate> funcTemplate =
        factory->NewFunctionTemplate(funcMethod, handleUndefined, 0);

    // Add function template to properties to trigger function template processing branch
    JSHandle<EcmaString> funcKey = factory->NewFromStdString("funcProperty");
    keys->Set(thread, 3, funcKey); // Use index 3 instead of NAME_INDEX to avoid conflicts
    properties->Set(thread, 3, funcTemplate);

    JSHandle<panda::ecmascript::NameDictionary> dictWithFunc =
        ClassHelper::BuildDictionaryProperties(thread, object, keys, properties,
                                                                    panda::ecmascript::ClassPropertyType::STATIC,
                                                                    JSHandle<JSTaggedValue>(object));

    EXPECT_FALSE(JSHandle<JSTaggedValue>::Cast(dictWithFunc)->IsUndefined());
}

} // namespace panda::test