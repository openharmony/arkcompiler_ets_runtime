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

#include "ecmascript/global_env.h"
#include "ecmascript/accessor_data.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/object_operator.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class AccessorDataTest : public BaseTestWithScope<false> {
};

/**
 * @tc.name: Cast
 * @tc.desc: Convert an object of type TaggedObject to type AccessorData object.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(AccessorDataTest, AccessorData_Cast)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());

    JSHandle<JSHClass> accClassHandle =
        factory->NewEcmaHClass(JSObject::SIZE, JSType::ACCESSOR_DATA, nullHandle);
    TaggedObject *accObject = factory->NewObject(accClassHandle);
    AccessorData::Cast(accObject)->SetGetter(thread, JSTaggedValue::Undefined());
    AccessorData::Cast(accObject)->SetSetter(thread, JSTaggedValue::Undefined());
    EXPECT_TRUE(JSTaggedValue(accObject).IsAccessorData());
    AccessorData *acc = AccessorData::Cast(accObject);
    EXPECT_TRUE(JSTaggedValue(acc).IsAccessorData());

    JSHandle<JSHClass> internalAccClassHandle =
        factory->NewEcmaHClass(JSObject::SIZE, JSType::INTERNAL_ACCESSOR, nullHandle);
    TaggedObject *internalAccObject = factory->NewObject(internalAccClassHandle);
    InternalAccessor::Cast(internalAccObject)->SetGetter(nullptr);
    InternalAccessor::Cast(internalAccObject)->SetSetter(nullptr);
    EXPECT_TRUE(JSTaggedValue(internalAccObject).IsInternalAccessor());
    AccessorData *internalAcc = AccessorData::Cast(internalAccObject);
    EXPECT_TRUE(JSTaggedValue(internalAcc).IsInternalAccessor());
}

/**
 * @tc.name: IsInternal
 * @tc.desc: Judge whether the accessor is internal.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(AccessorDataTest, IsInternal)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<AccessorData> accHandle = factory->NewAccessorData();
    EXPECT_EQ(accHandle->IsInternal(), false);

    void *setter = nullptr;
    void *getter = nullptr;
    JSHandle<AccessorData> internalAccHancle = factory->NewInternalAccessor(setter, getter);
    EXPECT_EQ(internalAccHancle->IsInternal(), true);

    JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());
    JSHandle<JSHClass> accClass = factory->NewEcmaHClass(JSObject::SIZE, JSType::ACCESSOR_DATA, nullHandle);
    TaggedObject *accObject = factory->NewObject(accClass);
    AccessorData *acc = AccessorData::Cast(accObject);
    acc->SetGetter(thread, JSTaggedValue::Undefined());
    acc->SetSetter(thread, JSTaggedValue::Undefined());
    EXPECT_EQ(acc->IsInternal(), false);

    JSHandle<JSHClass> internalAccClass =
        factory->NewEcmaHClass(JSObject::SIZE, JSType::INTERNAL_ACCESSOR, nullHandle);
    TaggedObject *internalAccObject = factory->NewObject(internalAccClass);
    InternalAccessor::Cast(internalAccObject)->SetGetter(nullptr);
    InternalAccessor::Cast(internalAccObject)->SetSetter(nullptr);
    AccessorData *internalAcc = AccessorData::Cast(internalAccObject);
    EXPECT_EQ(internalAcc->IsInternal(), true);
}

/**
 * @tc.name: HasSetter
 * @tc.desc: Judge whether the accessor have a undefined type "Setter".
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(AccessorDataTest, HasSetter)
{
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> normalFunction = globalEnv->GetNormalFunctionClass();

    // 1.Create normal AccessorData object by NewAccessorData function.
    JSHandle<AccessorData> accHandle = factory->NewAccessorData();
    EXPECT_EQ(accHandle->HasSetter(thread), false);
    accHandle->SetSetter(thread, JSTaggedValue::Undefined());
    EXPECT_EQ(accHandle->HasSetter(thread), false);
    accHandle->SetSetter(thread, normalFunction);
    EXPECT_EQ(accHandle->HasSetter(thread), true);

    // 2.Create internal AccessorData object by NewInternalAccessor function.
    void *setter = nullptr;
    void *getter = nullptr;
    JSHandle<InternalAccessor> internalAccHandle = JSHandle<InternalAccessor>::Cast(
        factory->NewInternalAccessor(setter, getter));
    EXPECT_EQ(internalAccHandle->HasSetter(), false);
    internalAccHandle->SetSetter(nullptr);
    EXPECT_EQ(internalAccHandle->HasSetter(), false);
    internalAccHandle->SetSetter(JSFunction::PrototypeSetter);
    EXPECT_EQ(internalAccHandle->HasSetter(), true);

    // 3.Create normal AccessorData object from dynamic class.
    JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());
    JSHandle<JSHClass> accClass = factory->NewEcmaHClass(JSObject::SIZE, JSType::ACCESSOR_DATA, nullHandle);
    TaggedObject *accObject = factory->NewObject(accClass);
    AccessorData *acc = AccessorData::Cast(accObject);
    acc->SetGetter(thread, JSTaggedValue::Undefined());
    acc->SetSetter(thread, JSTaggedValue::Undefined());
    EXPECT_EQ(acc->HasSetter(thread), false);
    acc->SetSetter(thread, normalFunction);
    EXPECT_EQ(acc->HasSetter(thread), true);

    // 4.Create internal AccessorData object from dynamic class.
    JSHandle<JSHClass> internalAccClass =
        factory->NewEcmaHClass(JSObject::SIZE, JSType::INTERNAL_ACCESSOR, nullHandle);
    TaggedObject *internalAccObject = factory->NewObject(internalAccClass);
    InternalAccessor *internalAcc = InternalAccessor::Cast(internalAccObject);
    internalAcc->SetGetter(nullptr);
    internalAcc->SetSetter(nullptr);
    EXPECT_EQ(internalAcc->HasSetter(), false);
    internalAcc->SetSetter(JSFunction::PrototypeSetter);
    EXPECT_EQ(internalAcc->HasSetter(), true);
}

/**
 * @tc.name: CallInternalSet/CallInternalGet
 * @tc.desc: Call internal set & get function to set object prototype.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(AccessorDataTest, CallInternalSet)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();

    // Construct objects and specify specific prototypes.
    JSFunction *func1 = globalEnv->GetObjectFunction().GetObject<JSFunction>();
    Method::Cast(func1->GetMethod(thread).GetTaggedObject())->SetFunctionKind(FunctionKind::BASE_CONSTRUCTOR);
    JSHandle<JSFunction> funcTagVal1 =
        factory->CloneJSFunction(JSHandle<JSFunction>(thread, func1));

    // Call the CallInternalGet method to inspect prototype.
    JSHandle<JSTaggedValue> nullPrototypeHandle(thread, JSTaggedValue::Null());
    JSHandle<JSHClass> accClass1 =
        factory->NewEcmaHClass(JSObject::SIZE, JSType::INTERNAL_ACCESSOR, nullPrototypeHandle);
    JSHandle<InternalAccessor> internal(thread, factory->NewObject(accClass1));
    JSHandle<AccessorData> accObject1 = JSHandle<AccessorData>::Cast(internal);
    internal->SetGetter(JSFunction::PrototypeGetter);

    JSTaggedValue valNullPrototype = accObject1->CallInternalGet(thread, JSHandle<JSObject>::Cast(funcTagVal1));
    EXPECT_NE(valNullPrototype.GetRawData(), JSTaggedValue::Undefined().GetRawData());

    // Call the CallInternalSet method to set new prototype.
    JSHandle<JSTaggedValue> undefPrototypeHandle(thread, JSTaggedValue::Undefined());
    internal->SetSetter(JSFunction::PrototypeSetter);
    bool res1 = accObject1->CallInternalSet(thread, JSHandle<JSObject>::Cast(funcTagVal1), undefPrototypeHandle);
    EXPECT_TRUE(res1);

    // Call the CallInternalGet method to check the changed prototype.
    valNullPrototype = accObject1->CallInternalGet(thread, JSHandle<JSObject>::Cast(funcTagVal1));
    EXPECT_EQ(valNullPrototype.GetRawData(), JSTaggedValue::Undefined().GetRawData());
}

/**
 * @tc.name: Cast
 * @tc.desc: Convert an object of type TaggedObject to type CompletionRecord object.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(AccessorDataTest, CompletionRecord_Cast)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());

    JSHandle<JSHClass> comRecordClassHandle =
        factory->NewEcmaHClass(JSObject::SIZE, JSType::COMPLETION_RECORD, nullHandle);
    TaggedObject *comRecordObject = factory->NewObject(comRecordClassHandle);
    CompletionRecord::Cast(comRecordObject)->SetValue(thread, JSTaggedValue::Undefined());
    EXPECT_TRUE(JSTaggedValue(comRecordObject).IsCompletionRecord());
    CompletionRecord *comRecord = CompletionRecord::Cast(comRecordObject);
    EXPECT_TRUE(JSTaggedValue(comRecord).IsCompletionRecord());
}

/**
 * @tc.name: IsThrow
 * @tc.desc: Judge whether the completion record is THROW type.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(AccessorDataTest, IsThrow)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> exceptionHandle(thread, thread->GetException());

    JSHandle<CompletionRecord> normalComRecHandle =
        factory->NewCompletionRecord(CompletionRecordType::NORMAL, exceptionHandle);
    EXPECT_TRUE(!normalComRecHandle->IsThrow());

    JSHandle<CompletionRecord> throwComRecHandle =
        factory->NewCompletionRecord(CompletionRecordType::THROW, exceptionHandle);
    EXPECT_TRUE(throwComRecHandle->IsThrow());
}
}  // namespace panda::test
