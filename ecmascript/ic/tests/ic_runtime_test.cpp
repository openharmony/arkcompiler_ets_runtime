/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/ic/ic_runtime.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/global_env.h"
#include "ecmascript/object_operator.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class ICRunTimeTest : public testing::Test {
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

HWTEST_F_L0(ICRunTimeTest, UpdateLoadHandler)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();
    uint32_t arrayLength = 5;

    JSHandle<JSTaggedValue> objFun = env->GetArrayFunction();
    JSHandle<JSTaggedValue> handleReceiver(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));
    JSHandle<JSTaggedValue> handleKeyWithElement(factory->NewFromASCII("2"));
    JSHandle<JSTaggedValue> handleKeyWithString(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleStoreArray(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> undefinedVal;

    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = factory->NewProfileTypeInfo(arrayLength);
    for (uint32_t i = 0; i < arrayLength; i++) {
        undefinedVal = globalConst->GetHandledUndefinedString();
        if (i == static_cast<uint32_t>(ICKind::NamedLoadIC) || i == static_cast<uint32_t>(ICKind::LoadIC)) {
            undefinedVal = handleStoreArray;
        }
        else if (i == static_cast<uint32_t>(ICKind::NamedLoadIC) + 1 ||
                 i == static_cast<uint32_t>(ICKind::LoadIC) + 1) {
            undefinedVal = JSHandle<JSTaggedValue>(thread, JSTaggedValue::Hole());
        }
        handleProfileTypeInfo->Set(thread, i, undefinedVal.GetTaggedValue());
    }
    // test op is Element
    ObjectOperator handleOp1(thread, handleKeyWithElement);
    uint32_t slotId = 2;
    ICRuntime icRuntime(thread, handleProfileTypeInfo, slotId, ICKind::LoadIC);
    icRuntime.UpdateLoadHandler(handleOp1, handleKeyWithElement, handleReceiver);
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, slotId).IsHole());
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, slotId + 1).IsHole());
    // test op is not Element
    ObjectOperator handleOp2(thread, handleKeyWithString);
    slotId = 0;
    ICRuntime icRuntime1(thread, handleProfileTypeInfo, slotId, ICKind::NamedLoadIC);
    icRuntime1.UpdateLoadHandler(handleOp2, handleKeyWithString, handleReceiver);
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, slotId).IsTaggedArray());
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, slotId + 1).IsHole());
}

HWTEST_F_L0(ICRunTimeTest, UpdateStoreHandler)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();
    uint32_t arrayLength = 5;

    JSHandle<JSTaggedValue> objFun = env->GetArrayFunction();
    JSHandle<JSTaggedValue> handleReceiver(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));
    JSHandle<JSTaggedValue> handleKeyWithElement(factory->NewFromASCII("2"));
    JSHandle<JSTaggedValue> handleKeyWithString(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleStoreArray(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> undefinedVal;

    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = factory->NewProfileTypeInfo(arrayLength);
    for (uint32_t i = 0; i < arrayLength; i++) {
        undefinedVal = globalConst->GetHandledUndefinedString();
        if (i == static_cast<uint32_t>(ICKind::NamedStoreIC) || i == static_cast<uint32_t>(ICKind::StoreIC)) {
            undefinedVal = JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined());
        }
        else if (i == static_cast<uint32_t>(ICKind::NamedStoreIC) + 1 ||
                 i == static_cast<uint32_t>(ICKind::StoreIC) + 1) {
            undefinedVal = handleStoreArray;
        }
        handleProfileTypeInfo->Set(thread, i, undefinedVal.GetTaggedValue());
    }
    // test op is Element
    uint32_t slotId = 3;
    ObjectOperator handleOp1(thread, handleKeyWithElement);
    handleOp1.SetIsOnPrototype(true);
    handleOp1.SetIndex(slotId);
    ICRuntime icRuntime(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    icRuntime.UpdateReceiverHClass(JSHandle<JSTaggedValue>(thread, JSHandle<JSObject>(handleReceiver)->GetJSHClass()));
    icRuntime.UpdateStoreHandler(handleOp1, handleKeyWithElement, handleReceiver);
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, slotId).IsWeak());
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, slotId + 1).IsPrototypeHandler());
    // test op is Transition
    slotId = 1;
    ObjectOperator handleOp2(thread, handleReceiver, handleKeyWithString, OperatorType::OWN);
    handleOp2.SetIsTransition(true);
    handleOp2.SetFastMode(true);
    handleOp2.SetIndex(slotId);
    ICRuntime icRuntime1(thread, handleProfileTypeInfo, slotId, ICKind::NamedStoreIC);
    icRuntime1.UpdateReceiverHClass(env->GetArgumentsClass());
    icRuntime1.UpdateStoreHandler(handleOp2, handleKeyWithString, handleReceiver);
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, slotId).IsWeak());
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, slotId + 1).IsTransitionHandler());
}

HWTEST_F_L0(ICRunTimeTest, TraceIC)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    uint32_t arrayLength = 5;
    JSHandle<JSTaggedValue> objFun = env->GetArrayFunction();
    JSHandle<JSTaggedValue> handleReceiver(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));
    JSHandle<JSTaggedValue> handleKeyWithString(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleKeyWithElement(thread, JSTaggedValue(2));

    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = factory->NewProfileTypeInfo(arrayLength);

    ICRuntime icRuntime(thread, handleProfileTypeInfo, 4, ICKind::NamedGlobalLoadIC);  // 4: means the NamedGlobalLoadIC
    icRuntime.TraceIC(thread, handleReceiver, handleKeyWithString);
    icRuntime.TraceIC(thread, handleReceiver, handleKeyWithElement);
}

HWTEST_F_L0(ICRunTimeTest, StoreMiss)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    uint32_t arrayLength = 2;

    JSHandle<JSTaggedValue> objFun = env->GetTypedArrayFunction();
    JSHandle<JSTaggedValue> handleReceiver(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));
    JSHandle<JSTaggedValue> handleReceiver1(factory->NewJSArray());
    JSHandle<JSTaggedValue> handleKeyWithString(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleValueWithElement(thread, JSTaggedValue(2));

    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = factory->NewProfileTypeInfo(arrayLength);
    handleProfileTypeInfo->Set(thread, 0, JSTaggedValue::Hole());
    handleProfileTypeInfo->Set(thread, 1, JSTaggedValue::Hole());
    StoreICRuntime storeICRuntime(thread, handleProfileTypeInfo, 0, ICKind::NamedGlobalStoreIC);
    storeICRuntime.StoreMiss(handleReceiver, handleKeyWithString, handleValueWithElement);
    EXPECT_EQ(JSObject::GetProperty(thread, handleReceiver, handleKeyWithString).GetValue(), handleValueWithElement);
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, 0).IsHole());
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, 1).IsHole());

    SlowRuntimeStub::StGlobalRecord(thread, handleKeyWithString.GetTaggedValue(),
                                    handleKeyWithString.GetTaggedValue(), false);
    handleProfileTypeInfo->Set(thread, 0, JSTaggedValue::Undefined());
    storeICRuntime.StoreMiss(handleReceiver1, handleKeyWithString, handleValueWithElement);
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, 0).IsPropertyBox());
}

HWTEST_F_L0(ICRunTimeTest, LoadMiss)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    uint32_t arrayLength = 2;

    JSHandle<JSTaggedValue> objFun = env->GetTypedArrayFunction();
    JSHandle<JSTaggedValue> handleReceiver(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));
    JSHandle<JSTaggedValue> handleReceiver1(factory->NewJSArray());
    JSHandle<JSTaggedValue> handleKeyWithString(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleValueWithElement(thread, JSTaggedValue(2));
    JSObject::SetProperty(thread, handleReceiver, handleKeyWithString, handleValueWithElement);

    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = factory->NewProfileTypeInfo(arrayLength);
    handleProfileTypeInfo->Set(thread, 0, JSTaggedValue::Hole());
    handleProfileTypeInfo->Set(thread, 1, JSTaggedValue::Hole());
    LoadICRuntime loadICRuntime(thread, handleProfileTypeInfo, 0, ICKind::NamedGlobalStoreIC);
    EXPECT_EQ(loadICRuntime.LoadMiss(handleReceiver, handleKeyWithString), handleValueWithElement.GetTaggedValue());
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, 0).IsHole());
    EXPECT_TRUE(handleProfileTypeInfo->Get(thread, 1).IsHole());
}
}  // namespace panda::test
