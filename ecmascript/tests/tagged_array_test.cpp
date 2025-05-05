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

#include "ecmascript/tagged_array.h"
#include "ecmascript/global_env.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class TaggedArrayTest : public BaseTestWithScope<false> {
};

static JSObject *JSArrayTestCreate(JSThread *thread)
{
    [[maybe_unused]] ecmascript::EcmaHandleScope scope(thread);
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    auto globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> jsFunc = globalEnv->GetArrayFunction();
    JSHandle<JSObject> newArray =
        ecmaVM->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(jsFunc), jsFunc);
    return *newArray;
}

HWTEST_F_L0(TaggedArrayTest, Create)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t arrayLength = 2;
    JSHandle<TaggedArray> taggedArray = factory->NewTaggedArray(arrayLength);
    EXPECT_TRUE(*taggedArray != nullptr);
    // every element of the taggedarray is JSTaggedValue::Hole()
    for (uint32_t i = 0; i < arrayLength; i++) {
        EXPECT_TRUE(taggedArray->Get(i).IsHole());
    }
}

HWTEST_F_L0(TaggedArrayTest, SetAndGetIdx)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t arrayLength = 2;
    JSHandle<TaggedArray> taggedArray = factory->NewTaggedArray(arrayLength);

    JSHandle<JSObject> newObj1(thread, JSArrayTestCreate(thread));
    JSObject *newObj2 = JSArrayTestCreate(thread);
    JSTaggedValue objValue(newObj2);
    taggedArray->Set(thread, 0, newObj1);
    taggedArray->Set(thread, 1, objValue);

    EXPECT_EQ(taggedArray->GetIdx(newObj1.GetTaggedValue()), 0U);
    EXPECT_EQ(taggedArray->GetIdx(objValue), 1U);
    // trigger gc
    ecmaVM->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_EQ(taggedArray->GetIdx(newObj1.GetTaggedValue()), 0U);
#ifndef USE_CMC_GC
    // cmc gc may not evacuate tl region.
    EXPECT_EQ(taggedArray->GetIdx(objValue), TaggedArray::MAX_ARRAY_INDEX);
#endif
}

HWTEST_F_L0(TaggedArrayTest, Append)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    ecmaVM->SetEnableForceGC(false);  // turn off GC
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t arrayLength = 2;
    JSHandle<TaggedArray> taggedArray1 = factory->NewTaggedArray(arrayLength);
    JSHandle<TaggedArray> taggedArray2 = factory->NewTaggedArray(arrayLength);
    // create value of the taggedarray
    JSHandle<JSObject> newObj1(thread, JSArrayTestCreate(thread));
    JSObject *newObj2 = JSArrayTestCreate(thread);
    JSTaggedValue objValue(newObj2);
    // set value to the taggedarray
    taggedArray1->Set(thread, 0, newObj1);
    taggedArray1->Set(thread, 1, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    taggedArray2->Set(thread, 0, objValue);
    taggedArray2->Set(thread, 1, JSTaggedValue::Undefined());
    // append two taggedarray
    JSHandle<TaggedArray> appendArray = TaggedArray::Append(thread, taggedArray1, taggedArray2);
    EXPECT_EQ(appendArray->GetLength(), arrayLength * 2);
    EXPECT_EQ(appendArray->Get(0), newObj1.GetTaggedValue());
    EXPECT_EQ(appendArray->Get(2), objValue); // 2: the second index
    EXPECT_EQ(appendArray->Get(1), appendArray->Get(3)); // 3: the third index
    ecmaVM->SetEnableForceGC(true);  // turn on GC
}

HWTEST_F_L0(TaggedArrayTest, AppendSkipHole)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t arrayLength = 2;
    uint32_t twoArrayLength = arrayLength * 2 + 1;
    JSHandle<TaggedArray> taggedArray1 = factory->NewTaggedArray(arrayLength + 1);
    JSHandle<TaggedArray> taggedArray2 = factory->NewTaggedArray(arrayLength);
    // create value of the taggedarray
    JSHandle<JSObject> newObj1(thread, JSArrayTestCreate(thread));
    // set value to the taggedarray
    taggedArray1->Set(thread, 0, newObj1);
    taggedArray1->Set(thread, 1, newObj1.GetTaggedValue());
    taggedArray1->Set(thread, 2, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    taggedArray2->Set(thread, 1, newObj1.GetTaggedValue());
    // append two taggedarray
    JSHandle<TaggedArray> appendArray = TaggedArray::AppendSkipHole(thread, taggedArray1, taggedArray2, twoArrayLength);
    EXPECT_EQ(appendArray->GetLength(), twoArrayLength);
    EXPECT_EQ(appendArray->Get(0), appendArray->Get(1));
    EXPECT_TRUE(appendArray->Get(2).IsUndefined()); // 2: the second index
    EXPECT_TRUE(appendArray->Get(3).IsHole()); // 3: the third index
    EXPECT_TRUE(appendArray->Get(4).IsHole()); // 4: the fourth index
}

HWTEST_F_L0(TaggedArrayTest, HasDuplicateEntry)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();
    uint32_t arrayLength = 10;
    JSHandle<TaggedArray> taggedArray = factory->NewTaggedArray(arrayLength);
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(1)); // 1: means value
    JSHandle<JSTaggedValue> value3(thread, JSTaggedValue(3)); // 3: means value

    JSHandle<JSObject> newObj1(thread, JSArrayTestCreate(thread));
    JSObject *newObj2 = JSArrayTestCreate(thread);
    JSTaggedValue objValue(newObj2);
    for (uint32_t i = 0; i < arrayLength; i++) {
        JSHandle<JSTaggedValue> arrayValue(thread, JSTaggedValue(i));
        taggedArray->Set(thread, i, arrayValue.GetTaggedValue());
    }
    EXPECT_FALSE(taggedArray->HasDuplicateEntry());
    // set value that is the same as the thrid index
    taggedArray->Set(thread, 1, value3.GetTaggedValue());
    EXPECT_TRUE(taggedArray->HasDuplicateEntry());
    // resert value in the second index
    taggedArray->Set(thread, 1, value1.GetTaggedValue());
    taggedArray->Set(thread, 5, newObj1);  // 5: the fifth index
    taggedArray->Set(thread, 6, newObj1.GetTaggedValue());  // 6: the sixth index
    EXPECT_TRUE(taggedArray->HasDuplicateEntry());
}

HWTEST_F_L0(TaggedArrayTest, Trim)
{
    uint32_t oldArrayLength = 10;
    uint32_t newArrayLength = 5;
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> indexValue(thread, JSTaggedValue(6));
    JSHandle<JSTaggedValue> undefinedValue(thread, JSTaggedValue::Undefined());

    JSHandle<JSTaggedValue> jsFunc = globalEnv->GetObjectFunction();
    JSHandle<JSObject> TaggedArrayObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(jsFunc), jsFunc);
    // Create taggedarray with oldArrayLength
    JSHandle<TaggedArray> taggedArray = factory->NewTaggedArray(oldArrayLength);
    for (uint32_t i = 0; i < oldArrayLength; i++) {
        JSHandle<JSTaggedValue> arrayValue(thread, JSTaggedValue(i));
        taggedArray->Set(thread, i, arrayValue.GetTaggedValue());
    }
    TaggedArrayObj->SetElements(thread, taggedArray);
    EXPECT_EQ(JSObject::GetProperty(thread, TaggedArrayObj, indexValue).GetValue(), indexValue);
    // trim taggedarray
    taggedArray->Trim(thread, newArrayLength);
    for (uint32_t i = 0; i < newArrayLength; i++) {
        JSHandle<JSTaggedValue> arrayValue(thread, JSTaggedValue(i));
        EXPECT_EQ(taggedArray->Get(i), arrayValue.GetTaggedValue());
    }
    EXPECT_EQ(taggedArray->GetLength(), newArrayLength);
    EXPECT_EQ(JSObject::GetProperty(thread, TaggedArrayObj, indexValue).GetValue(), undefinedValue);
}

HWTEST_F_L0(TaggedArrayTest, RemoveElementByIndex)
{
    constexpr uint32_t ELEMENT_NUMS = 20;
    constexpr uint32_t REMOVE_INDEX = 0;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> semiTaggedarray = factory->NewTaggedArray(ELEMENT_NUMS, JSTaggedValue::Hole(),
                                                                    MemSpaceType::SEMI_SPACE);
    JSHandle<TaggedArray> oldTaggedarray = factory->NewTaggedArray(ELEMENT_NUMS, JSTaggedValue::Hole(),
                                                                   MemSpaceType::OLD_SPACE);
    // init tagggedArray
    for (uint32_t i = 0; i < ELEMENT_NUMS; i++) {
        semiTaggedarray->Set(thread, i, JSTaggedValue(i));
        oldTaggedarray->Set(thread, i, JSTaggedValue(i));
    }
    TaggedArray::RemoveElementByIndex(thread, semiTaggedarray, REMOVE_INDEX, ELEMENT_NUMS);
    TaggedArray::RemoveElementByIndex(thread, oldTaggedarray, REMOVE_INDEX, ELEMENT_NUMS);

    for (uint32_t i = 0; i < ELEMENT_NUMS - 1; i++) {
        EXPECT_EQ(semiTaggedarray->Get(thread, i), JSTaggedValue(i + 1));
        EXPECT_EQ(oldTaggedarray->Get(thread, i), JSTaggedValue(i + 1));
    }
    EXPECT_EQ(semiTaggedarray->Get(thread, ELEMENT_NUMS - 1), JSTaggedValue::Hole());
    EXPECT_EQ(oldTaggedarray->Get(thread, ELEMENT_NUMS - 1), JSTaggedValue::Hole());
}

HWTEST_F_L0(TaggedArrayTest, InsertElementByIndex)
{
    constexpr uint32_t ELEMENT_NUMS = 20;
    constexpr uint32_t INSERT_INDEX = 0;
    JSHandle<JSTaggedValue> insertValue(thread, JSTaggedValue(ELEMENT_NUMS));
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> semiTaggedarray = factory->NewTaggedArray(ELEMENT_NUMS, JSTaggedValue::Hole(),
                                                                    MemSpaceType::SEMI_SPACE);
    JSHandle<TaggedArray> oldTaggedarray = factory->NewTaggedArray(ELEMENT_NUMS, JSTaggedValue::Hole(),
                                                                   MemSpaceType::OLD_SPACE);
    // init tagggedArray
    for (uint32_t i = 0; i < ELEMENT_NUMS - 1; i++) {
        semiTaggedarray->Set(thread, i, JSTaggedValue(i));
        oldTaggedarray->Set(thread, i, JSTaggedValue(i));
    }
    TaggedArray::InsertElementByIndex(thread, semiTaggedarray, insertValue, INSERT_INDEX, ELEMENT_NUMS - 1);
    TaggedArray::InsertElementByIndex(thread, oldTaggedarray, insertValue, INSERT_INDEX, ELEMENT_NUMS - 1);
    // check
    EXPECT_EQ(semiTaggedarray->Get(thread, 0), insertValue.GetTaggedValue());
    EXPECT_EQ(oldTaggedarray->Get(thread, 0), insertValue.GetTaggedValue());
    for (uint32_t i = 1; i < ELEMENT_NUMS; i++) {
        EXPECT_EQ(semiTaggedarray->Get(thread, i), JSTaggedValue(i - 1));
        EXPECT_EQ(oldTaggedarray->Get(thread, i), JSTaggedValue(i - 1));
    }
}

HWTEST_F_L0(TaggedArrayTest, CopyTaggedArrayElement)
{
    constexpr uint32_t ELEMENT_NUMS = 20;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> srcSemiTaggedarray = factory->NewTaggedArray(ELEMENT_NUMS, JSTaggedValue::Hole(),
                                                                       MemSpaceType::SEMI_SPACE);
    JSHandle<TaggedArray> srcOldTaggedarray = factory->NewTaggedArray(ELEMENT_NUMS, JSTaggedValue::Hole(),
                                                                      MemSpaceType::OLD_SPACE);
    JSHandle<TaggedArray> dstSemiTaggedarray = factory->NewTaggedArray(ELEMENT_NUMS);
    JSHandle<TaggedArray> dstOldTaggedarray = factory->NewTaggedArray(ELEMENT_NUMS);
    // init tagggedArray
    for (uint32_t i = 0; i < ELEMENT_NUMS; i++) {
        srcSemiTaggedarray->Set(thread, i, JSTaggedValue(i));
        srcOldTaggedarray->Set(thread, i, JSTaggedValue(i));
        dstSemiTaggedarray->Set(thread, i, JSTaggedValue(-i));
        dstOldTaggedarray->Set(thread, i, JSTaggedValue(-i));
    }
    TaggedArray::CopyTaggedArrayElement(thread, srcSemiTaggedarray, dstSemiTaggedarray, ELEMENT_NUMS);
    TaggedArray::CopyTaggedArrayElement(thread, srcOldTaggedarray, dstOldTaggedarray, ELEMENT_NUMS);

    for (uint32_t i = 0; i < ELEMENT_NUMS; i++) {
        EXPECT_EQ(srcSemiTaggedarray->Get(thread, i), dstSemiTaggedarray->Get(thread, i));
        EXPECT_EQ(srcOldTaggedarray->Get(thread, i), dstOldTaggedarray->Get(thread, i));
    }
}
}  // namespace panda::ecmascript