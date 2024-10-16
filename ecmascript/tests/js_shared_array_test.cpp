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

#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/shared_objects/js_shared_array.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {
class JSSharedArrayTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(JSSharedArrayTest, SetCapacityTest001)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0))
                                                 .GetTaggedValue()
                                                 .GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSTaggedValue> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, obj, lengthKeyHandle, SCheckMode::SKIP).GetValue()->GetInt(), 0);

    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<TaggedArray> values(factory->NewTaggedArray(5));
    for (int i = 0; i < 5; i++)
    {
        values->Set(thread, i, JSTaggedValue(i + 1));
    }
    JSHandle<TaggedArray> result(JSSharedArray::SetCapacity(thread, values, 10U));
    JSHandle<JSSharedArray> sharedArray(JSSharedArray::CreateArrayFromList(thread, result));
    EXPECT_EQ(sharedArray->GetArrayLength(), 10U);
}

HWTEST_F_L0(JSSharedArrayTest, SetCapacityTest002)
{
    JSSharedArray *sharedArray = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(5))
                                                         .GetTaggedValue()
                                                         .GetTaggedObject());
    EXPECT_TRUE(sharedArray != nullptr);
    JSHandle<JSObject> obj(thread, sharedArray);
    JSSharedArray::SetCapacity(thread, obj, 5U, 10U);
    EXPECT_EQ(sharedArray->GetArrayLength(), 10U);
}

HWTEST_F_L0(JSSharedArrayTest, SetCapacityTest003)
{
    JSSharedArray *sharedArray = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(5))
                                                         .GetTaggedValue()
                                                         .GetTaggedObject());
    EXPECT_TRUE(sharedArray != nullptr);
    JSHandle<JSObject> obj(thread, sharedArray);
    JSSharedArray::SetCapacity(thread, obj, 10U, 5U);
    EXPECT_EQ(sharedArray->GetArrayLength(), 5U);
}

HWTEST_F_L0(JSSharedArrayTest, PropertyKeyToArrayIndex)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<TaggedArray> values(factory->NewTaggedArray(5));
    for (int i = 0; i < 5; i++)
    {
        values->Set(thread, i, JSTaggedValue(i + 1));
    }
    JSHandle<JSTaggedValue> key(thread, JSTaggedValue(5));
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(-1));
    uint32_t output = 5;
    ASSERT_TRUE(JSSharedArray::PropertyKeyToArrayIndex(thread, key, &output));
    ASSERT_FALSE(JSSharedArray::PropertyKeyToArrayIndex(thread, key1, &output));
}

HWTEST_F_L0(JSSharedArrayTest, ArraySetLength)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(1))
                                                 .GetTaggedValue()
                                                 .GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(0)), true, true, true);
    ASSERT_FALSE(JSSharedArray::ArraySetLength(thread, obj, desc1));
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP)
                  .GetValue()
                  ->GetInt(),
              1);

    std::vector<int> vals{1, 2, 3, 4, 5};
    std::vector<JSHandle<JSTaggedValue>> keys;
    for (size_t i = 0; i < vals.size(); i++)
    {
        keys.push_back(JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<int>(i))));
        PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(vals[i])), true, true, true);
        EXPECT_TRUE(desc0.IsWritable());
        JSSharedArray::DefineOwnProperty(thread, obj, keys[i], desc0);
    }
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(6)), true, true, true);
    EXPECT_TRUE(desc1.IsWritable());
    ASSERT_FALSE(JSSharedArray::ArraySetLength(thread, obj, desc2));
}

HWTEST_F_L0(JSSharedArrayTest, ArraySetLengthTest001)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(1))
                                                 .GetTaggedValue()
                                                 .GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    PropertyDescriptor desc(thread);
    ASSERT_TRUE(JSSharedArray::ArraySetLength(thread, obj, desc));
}

HWTEST_F_L0(JSSharedArrayTest, ArraySetLengthTest002)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(2))
                                                 .GetTaggedValue()
                                                 .GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(6)));
    ASSERT_TRUE(JSSharedArray::ArraySetLength(thread, obj, desc));
}

HWTEST_F_L0(JSSharedArrayTest, ArraySetLengthTest003)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(1))
                                                 .GetTaggedValue()
                                                 .GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(0)), false, true, false);
    
    ASSERT_FALSE(JSSharedArray::ArraySetLength(thread, obj, desc1));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(0)), false, true, true);
    
    ASSERT_FALSE(JSSharedArray::ArraySetLength(thread, obj, desc2));
}

HWTEST_F_L0(JSSharedArrayTest, ArraySetLengthTest004)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(2))
                                                 .GetTaggedValue()
                                                 .GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    PropertyDescriptor desc(thread);
    JSSharedArray::DefineOwnProperty(thread, obj,
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<int>(0))), desc);
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, false, false);
    ASSERT_TRUE(JSSharedArray::ArraySetLength(thread, obj, desc1));
}

HWTEST_F_L0(JSSharedArrayTest, ArrayCreateTest001)
{
    double len = (double)std::numeric_limits<uint32_t>::max() + 1000;
    JSHandle<JSTaggedValue> result = JSSharedArray::ArrayCreate(thread, JSTaggedNumber(len));
    EXPECT_EQ(result->GetRawData(), JSTaggedValue::Exception().GetRawData());
}

HWTEST_F_L0(JSSharedArrayTest, ArrayCreateTest002)
{
    JSHandle<JSTaggedValue> result = JSSharedArray::ArrayCreate( \
        thread, JSTaggedNumber(10), ArrayMode::LITERAL);
    JSSharedArray *arr = JSSharedArray::Cast(result.GetTaggedValue().GetTaggedObject());
    EXPECT_EQ(arr->GetArrayLength(), 10);
}

HWTEST_F_L0(JSSharedArrayTest, LengthSetterTest001)
{
    JSHandle<JSObject> self;
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(static_cast<int32_t>(-64)));
    bool result = JSSharedArray::LengthSetter(thread, self, value, true);
    EXPECT_EQ(result, false);
}

HWTEST_F_L0(JSSharedArrayTest, LengthSetterTest003)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(15))
                                                 .GetTaggedValue()
                                                 .GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> self(thread, arr);
    auto *hclass = self->GetJSHClass();
    LayoutInfo *layoutInfo = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
    PropertyAttributes attr(layoutInfo->GetAttr(JSArray::LENGTH_INLINE_PROPERTY_INDEX));
    attr.SetWritable(false);
    layoutInfo->SetNormalAttr(thread, 0, attr);
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(10));
    bool result = JSSharedArray::LengthSetter(thread, self, value, true);
    EXPECT_EQ(result, false);
}

HWTEST_F_L0(JSSharedArrayTest, ArraySpeciesCreateTest001)
{
    JSArray *arr = JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetObject<JSArray>();
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    JSSharedArray::ArraySpeciesCreate(thread, obj, JSTaggedNumber(10));
}

HWTEST_F_L0(JSSharedArrayTest, LengthSetterTest004)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(15))
                                                 .GetTaggedValue()
                                                 .GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> self(thread, arr);
    auto *hclass = self->GetJSHClass();
    LayoutInfo *layoutInfo = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
    PropertyAttributes attr(layoutInfo->GetAttr(JSArray::LENGTH_INLINE_PROPERTY_INDEX));
    attr.SetWritable(false);
    layoutInfo->SetNormalAttr(thread, 0, attr);
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(10));
    bool result = JSSharedArray::LengthSetter(thread, self, value, false);
    EXPECT_EQ(result, false);
}

HWTEST_F_L0(JSSharedArrayTest, ArraySetLengthTest005)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(2))
                                                 .GetTaggedValue()
                                                 .GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    PropertyDescriptor desc(thread);
    JSSharedArray::DefineOwnProperty(thread, obj,
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<int>(0))), desc);
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread,
                        JSTaggedValue(std::numeric_limits<uint64_t>::max())), true, false, false);
    ASSERT_FALSE(JSSharedArray::ArraySetLength(thread, obj, desc1));
}

HWTEST_F_L0(JSSharedArrayTest, LengthSetterTest002)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0))
                                                 .GetTaggedValue()
                                                 .GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> self(thread, arr);
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(static_cast<int32_t>(0)));
    bool result = JSSharedArray::LengthSetter(thread, self, value, false);
    EXPECT_EQ(result, true);
}

HWTEST_F_L0(JSSharedArrayTest, CheckAndCopyArray)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSSharedArray> jsArray = factory->NewJSSArray();
    JSHandle<TaggedArray> prop(thread, jsArray->GetProperties());
    JSHClass* hclass = JSHandle<TaggedObject>(prop)->GetClass();
    hclass->SetObjectType(JSType::COW_TAGGED_ARRAY);
    ASSERT_EQ(hclass->GetObjectType(), JSType::COW_TAGGED_ARRAY);
    JSSharedArray::CheckAndCopyArray(thread, jsArray);
}
} // namespace panda::test