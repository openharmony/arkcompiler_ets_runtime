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

#include "ecmascript/ic/ic_info.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class ProfileTypeInfoTest : public testing::Test {
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
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

/**
 * @tc.name: ICKindToString
 * @tc.desc: Convert its name into a string according to different types of IC.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, ICKindToString)
{
    EXPECT_STREQ(CString(ICKindToString(ICKind::NamedLoadIC)).c_str(), "NamedLoadIC");
    EXPECT_STREQ(CString(ICKindToString(ICKind::LoadIC)).c_str(), "LoadIC");
    EXPECT_STREQ(CString(ICKindToString(ICKind::NamedGlobalLoadIC)).c_str(), "NamedGlobalLoadIC");
    EXPECT_STREQ(CString(ICKindToString(ICKind::GlobalLoadIC)).c_str(), "GlobalLoadIC");
}

/**
 * @tc.name: GetICState
 * @tc.desc: Define a TaggedArray object with a length of six. Set different values and convert it into a
 *           profiletypeinfo object.Then define different profiletypeaccessor objects according to each
 *           element in the array, and get the IC state through the "GetICState" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, GetICState)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> newValue(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> newArray(factory->NewTaggedArray(2)); // 2 : test case
    JSHandle<JSTaggedValue> newBox(factory->NewPropertyBox(newValue));

    uint32_t arrayLength = 11;
    JSHandle<JSTaggedValue> HandleKey0(factory->NewFromASCII("key0"));
    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = factory->NewProfileTypeInfo(arrayLength);
    handleProfileTypeInfo->Set(thread, 1, JSTaggedValue::Hole());
    handleProfileTypeInfo->Set(thread, 2, JSTaggedValue::Hole());
    handleProfileTypeInfo->Set(thread, 3, newArray.GetTaggedValue());
    handleProfileTypeInfo->Set(thread, 4, newArray.GetTaggedValue());
    handleProfileTypeInfo->Set(thread, 5, newBox.GetTaggedValue());
    handleProfileTypeInfo->Set(thread, 6, newArray.GetTaggedValue());
    handleProfileTypeInfo->Set(thread, 7, JSTaggedValue::Hole());
    handleProfileTypeInfo->Set(thread, 8, JSTaggedValue::Undefined());
    handleProfileTypeInfo->Set(thread, 9, JSTaggedValue::Hole());
    handleProfileTypeInfo->Set(thread, 10, HandleKey0.GetTaggedValue());

    EXPECT_TRUE(*handleProfileTypeInfo != nullptr);
    // slotId = 0
    JSHandle<ICInfo> csHandle(handleProfileTypeInfo);
    IcAccessor handleIcAccessor0(thread, csHandle, 0, ICKind::StoreIC);
    // slotId = 1,2
    IcAccessor handleIcAccessor1(thread, csHandle, 1, ICKind::GlobalLoadIC);
    // slotId = 3
    IcAccessor handleIcAccessor2(thread, csHandle, 3, ICKind::NamedLoadIC);
    // slotId = 4
    IcAccessor handleIcAccessor3(thread, csHandle, 4, ICKind::LoadIC);
    // slotId = 5
    IcAccessor handleIcAccessor4(thread, csHandle, 5, ICKind::NamedGlobalLoadIC);
    // slotId = 6
    IcAccessor handleIcAccessor5(thread, csHandle, 6, ICKind::GlobalStoreIC);
    // slotId = 7,8
    IcAccessor handleIcAccessor6(thread, csHandle, 7, ICKind::GlobalLoadIC);
    // slotId = 9,10
    IcAccessor handleIcAccessor7(thread, csHandle, 9, ICKind::LoadIC);

    EXPECT_EQ(handleIcAccessor0.GetICState(), IcAccessor::ICState::UNINIT);
    EXPECT_EQ(handleIcAccessor1.GetICState(), IcAccessor::ICState::MEGA);
    EXPECT_EQ(handleIcAccessor2.GetICState(), IcAccessor::ICState::POLY);
    EXPECT_EQ(handleIcAccessor3.GetICState(), IcAccessor::ICState::MONO);
    EXPECT_EQ(handleIcAccessor4.GetICState(), IcAccessor::ICState::MONO);
    EXPECT_EQ(handleIcAccessor5.GetICState(), IcAccessor::ICState::MONO);
    EXPECT_EQ(handleIcAccessor6.GetICState(), IcAccessor::ICState::MEGA);
    EXPECT_EQ(handleIcAccessor7.GetICState(), IcAccessor::ICState::IC_MEGA);
}

/**
 * @tc.name: AddHandlerWithoutKey
 * @tc.desc: Define a TaggedArray object with a length of two.Set values and convert it into a profiletypeinfo
 *           object.Then define profiletypeaccessor objects according to element in the array,profiletypeaccessor
 *           object call "AddHandlerWithoutKey" function to add Handler,This function cannot pass the key value.
 *           When different profiletypeaccessor objects call the AddHandlerWithoutKey function,the handlers stored
 *           in the array are different.Check whether the data stored in the array is the same as expected.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, AddHandlerWithoutKey)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> handleObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);

    JSHandle<JSTaggedValue> objClassVal(thread, handleObj->GetJSHClass());
    JSHandle<JSTaggedValue> HandlerValue(thread, JSTaggedValue(2));

    JSHandle<TaggedArray> handleTaggedArray = factory->NewTaggedArray(2);
    handleTaggedArray->Set(thread, 0, JSTaggedValue(1));
    handleTaggedArray->Set(thread, 1, JSTaggedValue(2));

    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = factory->NewProfileTypeInfo(4);
    handleProfileTypeInfo->Set(thread, 2, JSTaggedValue::Hole());

    uint32_t slotId = 0; // test profileData is Undefined
    IcAccessor handleIcAccessor0(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    handleIcAccessor0.AddHandlerWithoutKey(objClassVal, HandlerValue);
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, slotId).IsWeak());
    EXPECT_EQ(handleProfileTypeInfo->Get(thread, slotId + 1).GetInt(), 2);

    slotId = 1; // test POLY
    handleProfileTypeInfo->Set(thread, slotId, handleTaggedArray.GetTaggedValue()); // Reset Value
    IcAccessor handleIcAccessor1(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    handleIcAccessor1.AddHandlerWithoutKey(objClassVal, HandlerValue);
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, slotId).IsTaggedArray());
    JSHandle<TaggedArray> resultArr1(thread, handleProfileTypeInfo->Get(thread, slotId));
    EXPECT_EQ(resultArr1->GetLength(), 4U); // 4 = 2 + 2
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, slotId + 1).IsHole());

    slotId = 2; // test MONO to POLY
    handleProfileTypeInfo->Set(thread, slotId, HandlerValue.GetTaggedValue()); // Reset Value
    IcAccessor handleIcAccessor2(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    handleIcAccessor2.AddHandlerWithoutKey(objClassVal, HandlerValue);
    JSHandle<TaggedArray> resultArr2(thread, handleProfileTypeInfo->Get(thread, slotId));
    EXPECT_EQ(resultArr2->GetLength(), 4U); // POLY_DEFAULT_LEN
    EXPECT_EQ(resultArr2->Get(thread, 0).GetInt(), 2);
    EXPECT_TRUE(resultArr2->Get(thread, 1).IsUndefined());
    EXPECT_TRUE(resultArr2->Get(thread, 2).IsWeak());
    EXPECT_EQ(resultArr2->Get(thread, 3).GetInt(), 2);
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, slotId + 1).IsHole());
}

/**
 * @tc.name: AddElementHandler
 * @tc.desc: Define a TaggedArray object with a length of four.Set values and convert it into a profiletypeinfo
 *           object.Then define profiletypeaccessor objects according to element in the array,profiletypeaccessor
 *           object call "AddElementHandler" function to add Handler,This function pass the class value/key value.
 *           When different profiletypeaccessor objects call the AddHandlerWithoutKey function,the handlers stored
 *           in the array are different.Check whether the data stored in the array is the same as expected.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, AddElementHandler)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> handleObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);

    JSHandle<JSTaggedValue> objClassVal(thread, handleObj->GetJSHClass());
    JSHandle<JSTaggedValue> HandlerValue(thread, JSTaggedValue(3));

    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = factory->NewProfileTypeInfo(4);

    uint32_t slotId = 0;
    IcAccessor handleIcAccessor0(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    handleIcAccessor0.AddElementHandler(objClassVal, HandlerValue);
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, 0).IsWeak());
    EXPECT_EQ(handleProfileTypeInfo->Get(thread, 1).GetInt(), 3);
}

/**
 * @tc.name: AddHandlerWithKey
 * @tc.desc: Define a TaggedArray object with a length of two.Set values and convert it into a profiletypeinfo
 *           object.Then define profiletypeaccessor objects according to element in the array,profiletypeaccessor
 *           object call "AddHandlerWithKey" function to add Handler,This function pass the key value/class value
 *           and handler value.When different profiletypeaccessor objects call the AddHandlerWithoutKey function,
 *           the handlers stored in the array are different.Check whether the data stored in the array is the same
 *           as expected.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, AddHandlerWithKey)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> handleObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);

    JSHandle<JSTaggedValue> objClassVal(thread, handleObj->GetJSHClass());
    JSHandle<JSTaggedValue> HandlerValue(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> HandleKey0(factory->NewFromASCII("key0"));
    JSHandle<JSTaggedValue> HandleKey1(factory->NewFromASCII("key1"));

    JSHandle<TaggedArray> handleTaggedArray = factory->NewTaggedArray(2);
    handleTaggedArray->Set(thread, 0, JSTaggedValue(1));
    handleTaggedArray->Set(thread, 1, JSTaggedValue(2));

    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = factory->NewProfileTypeInfo(4);
    handleProfileTypeInfo->Set(thread, 2, handleTaggedArray.GetTaggedValue());
    handleProfileTypeInfo->Set(thread, 3, handleTaggedArray.GetTaggedValue());

    uint32_t slotId = 0; // test profileData is Undefined
    IcAccessor handleIcAccessor0(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    handleIcAccessor0.AddHandlerWithKey(HandleKey1, objClassVal, HandlerValue);
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, 0).IsString());
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, 1).IsTaggedArray());

    slotId = 1; // test profileData is equal the key
    handleProfileTypeInfo->Set(thread, slotId, HandleKey1.GetTaggedValue()); // Reset Value
    IcAccessor handleIcAccessor1(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    handleIcAccessor1.AddHandlerWithKey(HandleKey1, objClassVal, HandlerValue);
    JSHandle<TaggedArray> resultArr1(thread, handleProfileTypeInfo->Get(thread, slotId + 1));
    EXPECT_EQ(resultArr1->GetLength(), 4U); // 4 = 2 + 2

    slotId = 2; // test profileData is not equal the key
    IcAccessor handleIcAccessor2(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    handleIcAccessor2.AddHandlerWithKey(HandleKey0, objClassVal, HandlerValue);
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, slotId).IsHole());
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, slotId + 1).IsHole());
}

/**
 * @tc.name: AddGlobalHandlerKey
 * @tc.desc: Define a TaggedArray object with a length of two.Set values and convert it into a profiletypeinfo
 *           object.Then define profiletypeaccessor objects according to element in the array,different
 *           IcAccessor objects call the "AddGlobalHandlerKey" function,the data stored in the array in
 *           the object is different.This function pass the key value/handler value,Check whether the data stored
 *           in the array is the same as expected.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, AddGlobalHandlerKey)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> HandlerValue(thread, JSTaggedValue(222));
    JSHandle<TaggedArray> handleTaggedArray = factory->NewTaggedArray(2);
    handleTaggedArray->Set(thread, 0, JSTaggedValue(111));
    handleTaggedArray->Set(thread, 1, JSTaggedValue(333));
    JSHandle<JSTaggedValue> arrayKey(factory->NewFromASCII("array"));

    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = factory->NewProfileTypeInfo(2);
    handleProfileTypeInfo->Set(thread, 1, handleTaggedArray.GetTaggedValue());

    uint32_t slotId = 0;
    IcAccessor handleIcAccessor0(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    handleIcAccessor0.AddGlobalHandlerKey(arrayKey, HandlerValue);
    JSHandle<TaggedArray> resultArr1(thread, handleProfileTypeInfo->Get(thread, slotId));
    EXPECT_EQ(resultArr1->GetLength(), 2U);
    EXPECT_TRUE(resultArr1->Get(thread, 0).IsWeak());
    EXPECT_EQ(resultArr1->Get(thread, 1).GetInt(), 222);

    slotId = 1;
    IcAccessor handleIcAccessor1(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    handleIcAccessor1.AddGlobalHandlerKey(arrayKey, HandlerValue);
    JSHandle<TaggedArray> resultArr2(thread, handleProfileTypeInfo->Get(thread, slotId));
    EXPECT_EQ(resultArr2->GetLength(), 4U);
    EXPECT_TRUE(resultArr2->Get(thread, 0).IsWeak());
    EXPECT_EQ(resultArr2->Get(thread, 1).GetInt(), 222);
    EXPECT_EQ(resultArr2->Get(thread, 2).GetInt(), 111);
    EXPECT_EQ(resultArr2->Get(thread, 3).GetInt(), 333);
}

/**
 * @tc.name: AddGlobalRecordHandler
 * @tc.desc: Define a TaggedArray object with a length of two.Set no values and convert it into a profiletypeinfo
 *           object.Then define profiletypeaccessor objects according to element in the array,different
 *           IcAccessor objects call the "AddGlobalRecordHandler" function,the data stored in the array in
 *           the object is the same.This function pass the handler value,Check whether the data stored
 *           in the array is the same as expected.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, AddGlobalRecordHandler)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> HandlerValue1(thread, JSTaggedValue(232));
    JSHandle<JSTaggedValue> HandlerValue2(thread, JSTaggedValue(5));

    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = factory->NewProfileTypeInfo(2);
    uint32_t slotId = 0;
    IcAccessor handleIcAccessor(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    handleIcAccessor.AddGlobalRecordHandler(HandlerValue1);
    EXPECT_EQ(handleProfileTypeInfo->Get(thread, slotId).GetInt(), 232);
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, slotId + 1).IsUndefined());

    handleIcAccessor.AddGlobalRecordHandler(HandlerValue2);
    EXPECT_EQ(handleProfileTypeInfo->Get(thread, slotId).GetInt(), 5);
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, slotId + 1).IsUndefined());
}

/**
 * @tc.name: SetAsMega
 * @tc.desc: The function of the "SetAsMega" function is to set the element of the array in the handleProfileTypeInfo
 *           object as hole,so call the "SetAsMega" function to check whether the element of the array is hole.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, SetAsMega)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t arrayLength = 2;
    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = factory->NewProfileTypeInfo(arrayLength);
    handleProfileTypeInfo->Set(thread, 0, JSTaggedValue(111));
    handleProfileTypeInfo->Set(thread, 1, JSTaggedValue(222));

    EXPECT_TRUE(*handleProfileTypeInfo != nullptr);
    uint32_t slotId = 0;
    IcAccessor handleIcAccessor(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    handleIcAccessor.SetAsMega();

    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, 0).IsHole());
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, 1).IsHole());
}

/**
 * @tc.name: GetWeakRef
 * @tc.desc: The function of this function is to create and get the weakref for The elements in the array of the defined
 *           handleProfileTypeInfo object.call the "GetWeakRef" function is to check whether the elements in the array
 *           have weak data.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, GetWeakRef)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> handleObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSTaggedValue weakRefValue(handleObj.GetTaggedValue());

    uint32_t arrayLength = 2;
    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = factory->NewProfileTypeInfo(arrayLength);
    EXPECT_TRUE(*handleProfileTypeInfo != nullptr);

    uint32_t slotId = 0;
    IcAccessor handleIcAccessor(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    EXPECT_TRUE(handleIcAccessor.GetWeakRef(weakRefValue).IsWeak());
}

/**
 * @tc.name: GetRefFromWeak
 * @tc.desc: The function of this function is to get the weakref.The elements in the array of the defined
 *           handleProfileTypeInfo object call the "CreateWeakRef" function to create the data of the weakref.
 *           call the "GetRefFromWeak" function is to check whether the elements in the array have data of calling
 *           "CreateWeakRef".
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, GetRefFromWeak)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();

    JSHandle<JSObject> handleProfileTypeValue = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSTaggedValue handleProfileType(handleProfileTypeValue.GetTaggedValue());
    handleProfileType.CreateWeakRef();

    uint32_t arrayLength = 2;
    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = factory->NewProfileTypeInfo(arrayLength);
    EXPECT_TRUE(*handleProfileTypeInfo != nullptr);

    uint32_t slotId = 0;
    IcAccessor handleIcAccessor(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    EXPECT_EQ(handleIcAccessor.GetRefFromWeak(handleProfileType).GetTaggedObject(),
                                                                           handleProfileType.GetTaggedWeakRef());
}

/**
 * @tc.name: ICKindUtilityFunctions
 * @tc.desc: Verify every ICKind classifier function returns correct results for all ICKind values.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, ICKindUtilityFunctions)
{
    // IsNamedGlobalIC: true for NamedGlobal{Load,Store,TryLoad,TryStore}IC
    EXPECT_TRUE(IsNamedGlobalIC(ICKind::NamedGlobalLoadIC));
    EXPECT_TRUE(IsNamedGlobalIC(ICKind::NamedGlobalStoreIC));
    EXPECT_TRUE(IsNamedGlobalIC(ICKind::NamedGlobalTryLoadIC));
    EXPECT_TRUE(IsNamedGlobalIC(ICKind::NamedGlobalTryStoreIC));
    EXPECT_FALSE(IsNamedGlobalIC(ICKind::NamedLoadIC));
    EXPECT_FALSE(IsNamedGlobalIC(ICKind::LoadIC));
    EXPECT_FALSE(IsNamedGlobalIC(ICKind::GlobalLoadIC));

    // IsValueGlobalIC: true only for Global{Load,Store}IC
    EXPECT_TRUE(IsValueGlobalIC(ICKind::GlobalLoadIC));
    EXPECT_TRUE(IsValueGlobalIC(ICKind::GlobalStoreIC));
    EXPECT_FALSE(IsValueGlobalIC(ICKind::LoadIC));
    EXPECT_FALSE(IsValueGlobalIC(ICKind::NamedGlobalLoadIC));

    // IsValueNormalIC: true only for Load/StoreIC
    EXPECT_TRUE(IsValueNormalIC(ICKind::LoadIC));
    EXPECT_TRUE(IsValueNormalIC(ICKind::StoreIC));
    EXPECT_FALSE(IsValueNormalIC(ICKind::NamedLoadIC));
    EXPECT_FALSE(IsValueNormalIC(ICKind::GlobalLoadIC));

    // IsValueIC: union of ValueNormal and ValueGlobal
    EXPECT_TRUE(IsValueIC(ICKind::LoadIC));
    EXPECT_TRUE(IsValueIC(ICKind::StoreIC));
    EXPECT_TRUE(IsValueIC(ICKind::GlobalLoadIC));
    EXPECT_TRUE(IsValueIC(ICKind::GlobalStoreIC));
    EXPECT_FALSE(IsValueIC(ICKind::NamedLoadIC));
    EXPECT_FALSE(IsValueIC(ICKind::NamedGlobalLoadIC));

    // IsNamedNormalIC: true only for Named{Load,Store}IC
    EXPECT_TRUE(IsNamedNormalIC(ICKind::NamedLoadIC));
    EXPECT_TRUE(IsNamedNormalIC(ICKind::NamedStoreIC));
    EXPECT_FALSE(IsNamedNormalIC(ICKind::LoadIC));
    EXPECT_FALSE(IsNamedNormalIC(ICKind::NamedGlobalLoadIC));

    // IsNamedIC: union of NamedNormal and NamedGlobal
    EXPECT_TRUE(IsNamedIC(ICKind::NamedLoadIC));
    EXPECT_TRUE(IsNamedIC(ICKind::NamedStoreIC));
    EXPECT_TRUE(IsNamedIC(ICKind::NamedGlobalLoadIC));
    EXPECT_TRUE(IsNamedIC(ICKind::NamedGlobalStoreIC));
    EXPECT_TRUE(IsNamedIC(ICKind::NamedGlobalTryLoadIC));
    EXPECT_TRUE(IsNamedIC(ICKind::NamedGlobalTryStoreIC));
    EXPECT_FALSE(IsNamedIC(ICKind::LoadIC));
    EXPECT_FALSE(IsNamedIC(ICKind::GlobalLoadIC));

    // IsGlobalLoadIC: NamedGlobalLoadIC, GlobalLoadIC, NamedGlobalTryLoadIC
    EXPECT_TRUE(IsGlobalLoadIC(ICKind::NamedGlobalLoadIC));
    EXPECT_TRUE(IsGlobalLoadIC(ICKind::GlobalLoadIC));
    EXPECT_TRUE(IsGlobalLoadIC(ICKind::NamedGlobalTryLoadIC));
    EXPECT_FALSE(IsGlobalLoadIC(ICKind::NamedGlobalStoreIC));
    EXPECT_FALSE(IsGlobalLoadIC(ICKind::LoadIC));
    EXPECT_FALSE(IsGlobalLoadIC(ICKind::GlobalStoreIC));

    // IsGlobalStoreIC: NamedGlobalStoreIC, GlobalStoreIC, NamedGlobalTryStoreIC
    EXPECT_TRUE(IsGlobalStoreIC(ICKind::NamedGlobalStoreIC));
    EXPECT_TRUE(IsGlobalStoreIC(ICKind::GlobalStoreIC));
    EXPECT_TRUE(IsGlobalStoreIC(ICKind::NamedGlobalTryStoreIC));
    EXPECT_FALSE(IsGlobalStoreIC(ICKind::NamedGlobalLoadIC));
    EXPECT_FALSE(IsGlobalStoreIC(ICKind::LoadIC));
    EXPECT_FALSE(IsGlobalStoreIC(ICKind::GlobalLoadIC));

    // IsGlobalIC: union of ValueGlobal and NamedGlobal
    EXPECT_TRUE(IsGlobalIC(ICKind::GlobalLoadIC));
    EXPECT_TRUE(IsGlobalIC(ICKind::GlobalStoreIC));
    EXPECT_TRUE(IsGlobalIC(ICKind::NamedGlobalLoadIC));
    EXPECT_TRUE(IsGlobalIC(ICKind::NamedGlobalStoreIC));
    EXPECT_TRUE(IsGlobalIC(ICKind::NamedGlobalTryLoadIC));
    EXPECT_TRUE(IsGlobalIC(ICKind::NamedGlobalTryStoreIC));
    EXPECT_FALSE(IsGlobalIC(ICKind::LoadIC));
    EXPECT_FALSE(IsGlobalIC(ICKind::StoreIC));
    EXPECT_FALSE(IsGlobalIC(ICKind::NamedLoadIC));
    EXPECT_FALSE(IsGlobalIC(ICKind::NamedStoreIC));
}

/**
 * @tc.name: ICKindToStringComplete
 * @tc.desc: Verify ICKindToString covers all ICKind enum values (extending the existing test).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, ICKindToStringComplete)
{
    EXPECT_STREQ(CString(ICKindToString(ICKind::NamedLoadIC)).c_str(), "NamedLoadIC");
    EXPECT_STREQ(CString(ICKindToString(ICKind::NamedStoreIC)).c_str(), "NamedStoreIC");
    EXPECT_STREQ(CString(ICKindToString(ICKind::LoadIC)).c_str(), "LoadIC");
    EXPECT_STREQ(CString(ICKindToString(ICKind::StoreIC)).c_str(), "StoreIC");
    EXPECT_STREQ(CString(ICKindToString(ICKind::NamedGlobalLoadIC)).c_str(), "NamedGlobalLoadIC");
    EXPECT_STREQ(CString(ICKindToString(ICKind::NamedGlobalStoreIC)).c_str(), "NamedGlobalStoreIC");
    EXPECT_STREQ(CString(ICKindToString(ICKind::NamedGlobalTryLoadIC)).c_str(), "NamedGlobalTryLoadIC");
    EXPECT_STREQ(CString(ICKindToString(ICKind::NamedGlobalTryStoreIC)).c_str(), "NamedGlobalTryStoreIC");
    EXPECT_STREQ(CString(ICKindToString(ICKind::GlobalLoadIC)).c_str(), "GlobalLoadIC");
    EXPECT_STREQ(CString(ICKindToString(ICKind::GlobalStoreIC)).c_str(), "GlobalStoreIC");
}

/**
 * @tc.name: ICInfoSafeCast
 * @tc.desc: Verify ICInfo::SafeCast returns empty handle for non-ICInfo heap objects
 *           and valid handle for real ICInfo objects.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, ICInfoSafeCast)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // Valid ICInfo should cast successfully
    JSHandle<ICInfo> icInfo = factory->NewICInfo(2);
    JSHandle<JSTaggedValue> icInfoVal(icInfo);
    JSHandle<ICInfo> castResult = ICInfo::SafeCast(icInfoVal);
    EXPECT_FALSE(castResult.IsEmpty());

    // Non-ICInfo heap object (a plain TaggedArray) should fail SafeCast
    JSHandle<TaggedArray> plainArray = factory->NewTaggedArray(2);
    JSHandle<JSTaggedValue> plainVal(plainArray);
    JSHandle<ICInfo> failedCast = ICInfo::SafeCast(plainVal);
    EXPECT_TRUE(failedCast.IsEmpty());

    // Non-heap value (integer) should fail SafeCast
    JSHandle<JSTaggedValue> intVal(thread, JSTaggedValue(42));
    JSHandle<ICInfo> intCast = ICInfo::SafeCast(intVal);
    EXPECT_TRUE(intCast.IsEmpty());
}

/**
 * @tc.name: ICInfoSlotOperations
 * @tc.desc: Verify ICInfo slot get/set operations and SetMultiIcSlotLocked.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, ICInfoSlotOperations)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ICInfo> icInfo = factory->NewICInfo(2);

    // Initial slots should be undefined
    EXPECT_TRUE(icInfo->GetIcSlot(thread, 0).IsUndefined());
    EXPECT_TRUE(icInfo->GetIcSlot(thread, 1).IsUndefined());
    EXPECT_EQ(icInfo->GetIcSlotLength(), 2U);

    // Set individual slots
    icInfo->SetIcSlot(thread, 0, JSTaggedValue(100));
    icInfo->SetIcSlot(thread, 1, JSTaggedValue(200));
    EXPECT_EQ(icInfo->GetIcSlot(thread, 0).GetInt(), 100);
    EXPECT_EQ(icInfo->GetIcSlot(thread, 1).GetInt(), 200);

    // SetMultiIcSlotLocked atomically sets both slots
    icInfo->SetMultiIcSlotLocked(thread, 0, JSTaggedValue::Hole(), 1, JSTaggedValue::Hole());
    EXPECT_TRUE(icInfo->GetIcSlot(thread, 0).IsHole());
    EXPECT_TRUE(icInfo->GetIcSlot(thread, 1).IsHole());
}

/**
 * @tc.name: IcAccessorWithICInfo
 * @tc.desc: Verify IcAccessor works correctly with the ICInfo constructor (not ProfileTypeInfo).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, IcAccessorWithICInfo)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ICInfo> icInfo = factory->NewICInfo(2);

    // UNINIT state
    IcAccessor accessor(thread, icInfo, 0, ICKind::LoadIC);
    EXPECT_EQ(accessor.GetICState(), IcAccessor::ICState::UNINIT);
    EXPECT_EQ(accessor.GetKind(), ICKind::LoadIC);
    EXPECT_EQ(accessor.GetSlotId(), 0U);

    // SetAsMega with non-global IC should set both slots to Hole
    accessor.SetAsMega();
    EXPECT_TRUE(icInfo->GetIcSlot(thread, 0).IsHole());
    EXPECT_TRUE(icInfo->GetIcSlot(thread, 1).IsHole());
    EXPECT_EQ(accessor.GetICState(), IcAccessor::ICState::MEGA);
}

/**
 * @tc.name: IcAccessorSetAsMegaGlobalIC
 * @tc.desc: Verify SetAsMega behavior differs for global IC kinds (single-slot).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, IcAccessorSetAsMegaGlobalIC)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t arrayLength = 4;
    JSHandle<ProfileTypeInfo> profileInfo = factory->NewProfileTypeInfo(arrayLength);
    profileInfo->Set(thread, 0, JSTaggedValue(111));
    profileInfo->Set(thread, 1, JSTaggedValue(222));
    profileInfo->Set(thread, 2, JSTaggedValue(333));
    profileInfo->Set(thread, 3, JSTaggedValue(444));

    // Global IC: SetAsMega should only set the single slot to Hole
    IcAccessor globalAccessor(thread, profileInfo, 0, ICKind::GlobalLoadIC);
    globalAccessor.SetAsMega();
    EXPECT_TRUE(profileInfo->Get(thread, 0).IsHole());
    // Slot 1 is independent — should remain unchanged
    EXPECT_EQ(profileInfo->Get(thread, 1).GetInt(), 222);

    // GlobalStoreIC
    IcAccessor storeAccessor(thread, profileInfo, 2, ICKind::GlobalStoreIC);
    storeAccessor.SetAsMega();
    EXPECT_TRUE(profileInfo->Get(thread, 2).IsHole());
    EXPECT_EQ(profileInfo->Get(thread, 3).GetInt(), 444);
}

/**
 * @tc.name: IcAccessorICStateToString
 * @tc.desc: Verify ICStateToString covers all IC state enum values.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, IcAccessorICStateToString)
{
    EXPECT_STREQ(IcAccessor::ICStateToString(IcAccessor::ICState::UNINIT).c_str(), "uninit");
    EXPECT_STREQ(IcAccessor::ICStateToString(IcAccessor::ICState::MONO).c_str(), "mono");
    EXPECT_STREQ(IcAccessor::ICStateToString(IcAccessor::ICState::POLY).c_str(), "poly");
    EXPECT_STREQ(IcAccessor::ICStateToString(IcAccessor::ICState::MEGA).c_str(), "mega");
    EXPECT_STREQ(IcAccessor::ICStateToString(IcAccessor::ICState::IC_MEGA).c_str(), "ic_mega");
}

/**
 * @tc.name: IcAccessorSetAsMegaIfUndefined
 * @tc.desc: Verify SetAsMegaIfUndefined only transitions Undefined slots to MEGA.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, IcAccessorSetAsMegaIfUndefined)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ICInfo> icInfo = factory->NewICInfo(2);

    // Slots start as Undefined, so SetAsMegaIfUndefined should set them to Hole
    IcAccessor accessor(thread, icInfo, 0, ICKind::StoreIC);
    accessor.SetAsMegaIfUndefined();
    EXPECT_TRUE(icInfo->GetIcSlot(thread, 0).IsHole());
    EXPECT_TRUE(icInfo->GetIcSlot(thread, 1).IsHole());

    // Reset to non-Undefined (Hole = MEGA state)
    // SetAsMegaIfUndefined on non-Undefined should be a no-op
    JSHandle<ICInfo> icInfo2 = factory->NewICInfo(2);
    icInfo2->SetIcSlot(thread, 0, JSTaggedValue(42));
    IcAccessor accessor2(thread, icInfo2, 0, ICKind::StoreIC);
    accessor2.SetAsMegaIfUndefined();
    // Should NOT have changed — slot 0 was not Undefined
    EXPECT_EQ(icInfo2->GetIcSlot(thread, 0).GetInt(), 42);
}
} // namespace panda::test
