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

#include "ecmascript/layout_info-inl.h"
#include "ecmascript/property_attributes.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class LayoutInfoTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(LayoutInfoTest, SetNumberOfElements)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    int32_t infoLength = 2; // 2: len
    JSHandle<LayoutInfo> layoutInfoHandle = factory->CreateLayoutInfo(infoLength);
    EXPECT_TRUE(*layoutInfoHandle != nullptr);

    layoutInfoHandle->SetNumberOfElements(thread, 100);
    EXPECT_EQ(layoutInfoHandle->NumberOfElements(), 100);
}

HWTEST_F_L0(LayoutInfoTest, SetPropertyInit)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    int32_t infoLength = 3;
    PropertyAttributes defaultAttr = PropertyAttributes::Default();
    defaultAttr.SetNormalAttr(infoLength);
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("key"));
    JSHandle<LayoutInfo> layoutInfoHandle = factory->CreateLayoutInfo(infoLength);
    EXPECT_TRUE(*layoutInfoHandle != nullptr);

    layoutInfoHandle->SetPropertyInit(thread, 0, key.GetTaggedValue(), defaultAttr);
    EXPECT_EQ(layoutInfoHandle->GetKey(0), key.GetTaggedValue());
    EXPECT_EQ(layoutInfoHandle->GetAttr(0).GetNormalAttr(), static_cast<uint32_t>(infoLength));
}

HWTEST_F_L0(LayoutInfoTest, SetSortedIndex)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    int32_t infoLength = 5;
    PropertyAttributes defaultAttr = PropertyAttributes::Default();
    defaultAttr.SetNormalAttr(infoLength);
    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("hello"));
    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("world"));
    JSHandle<LayoutInfo> layoutInfoHandle = factory->CreateLayoutInfo(infoLength);
    EXPECT_TRUE(*layoutInfoHandle != nullptr);

    layoutInfoHandle->SetPropertyInit(thread, 0, key1.GetTaggedValue(), defaultAttr);
    layoutInfoHandle->SetPropertyInit(thread, 1, key2.GetTaggedValue(), defaultAttr);
    layoutInfoHandle->SetSortedIndex(thread, 0, infoLength - 4);
    EXPECT_EQ(layoutInfoHandle->GetSortedIndex(0), 1U);
    EXPECT_EQ(layoutInfoHandle->GetSortedKey(0), key2.GetTaggedValue());
}

HWTEST_F_L0(LayoutInfoTest, FindElementWithCache)
{
    int infoLength = 5;
    int newPropertiesLength = 11;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    PropertyAttributes defaultAttr = PropertyAttributes::Default();
    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("1"));
    JSHandle<JSTaggedValue> key4(factory->NewFromASCII("4"));
    JSHandle<JSTaggedValue> key5(factory->NewFromASCII("5"));
    JSHandle<JSTaggedValue> key10(factory->NewFromASCII("10"));
    JSHandle<JSTaggedValue> key11(factory->NewFromASCII("11"));
    JSHandle<LayoutInfo> layoutInfoHandle = factory->CreateLayoutInfo(infoLength);
    EXPECT_TRUE(*layoutInfoHandle != nullptr);
    for (int i = 0; i < infoLength; i++) {
        JSHandle<JSTaggedValue> elements(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> elementsKey(JSTaggedValue::ToString(thread, elements));
        defaultAttr.SetOffset(i);
        layoutInfoHandle->AddKey(thread, i, elementsKey.GetTaggedValue(), defaultAttr);
    }
    int propertiesNumber = layoutInfoHandle->NumberOfElements();
    int result = 0;
    result = layoutInfoHandle->FindElementWithCache(thread, nullptr, key1.GetTaggedValue(), propertiesNumber);
    EXPECT_EQ(result, 1); // 1: Index corresponding to key1
    result = layoutInfoHandle->FindElementWithCache(thread, nullptr, key5.GetTaggedValue(), propertiesNumber);
    EXPECT_EQ(result, -1); // -1: not find
    // extend layoutInfo
    JSHandle<LayoutInfo> newLayoutInfo = factory->ExtendLayoutInfo(layoutInfoHandle, newPropertiesLength);
    for (int i = 5; i < newPropertiesLength; i++) {
        JSHandle<JSTaggedValue> elements(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> elementsKey(JSTaggedValue::ToString(thread, elements));
        defaultAttr.SetOffset(i);
        newLayoutInfo->AddKey(thread, i, elementsKey.GetTaggedValue(), defaultAttr);
    }
    result = newLayoutInfo->FindElementWithCache(thread, nullptr, key4.GetTaggedValue(), newPropertiesLength);
    EXPECT_EQ(result, 4); // 4: Index corresponding to key4
    result = newLayoutInfo->FindElementWithCache(thread, nullptr, key10.GetTaggedValue(), newPropertiesLength);
    EXPECT_EQ(result, 10); // 10: Index corresponding to key10
    result = newLayoutInfo->FindElementWithCache(thread, nullptr, key5.GetTaggedValue(), newPropertiesLength);
    EXPECT_EQ(result, 5); // 5: Index corresponding to key5
    result = newLayoutInfo->FindElementWithCache(thread, nullptr, key11.GetTaggedValue(), newPropertiesLength);
    EXPECT_EQ(result, -1); // -1: not find
}

void GetAllKeysCommon(JSThread *thread, bool enumKeys = false)
{
    int infoLength = 5;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    PropertyAttributes defaultAttr = PropertyAttributes::Default();
    JSHandle<JSTaggedValue> key3(factory->NewFromASCII("3"));

    JSHandle<TaggedArray> keyArray = factory->NewTaggedArray(infoLength);
    JSHandle<LayoutInfo> layoutInfoHandle = factory->CreateLayoutInfo(infoLength);
    EXPECT_TRUE(*layoutInfoHandle != nullptr);
    std::vector<JSTaggedValue> keyVector;
    // Add key to layout info
    for (int i = 0; i < infoLength; i++) {
        JSHandle<JSTaggedValue> elements(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> elementsKey(JSTaggedValue::ToString(thread, elements));
        defaultAttr.SetOffset(i);
        if (enumKeys) {
            if (i != 3) { // 3: index
                defaultAttr.SetEnumerable(false);
            } else {
                defaultAttr.SetEnumerable(true);
            }
        }
        layoutInfoHandle->AddKey(thread, i, elementsKey.GetTaggedValue(), defaultAttr);
    }
    if (enumKeys) {
        uint32_t keys = 0;
        layoutInfoHandle->GetAllEnumKeys(thread, infoLength, 0, keyArray, &keys); // 0: offset
        EXPECT_EQ(keyArray->Get(0), key3.GetTaggedValue());
        EXPECT_EQ(keys, 1U);
    } else {
        layoutInfoHandle->GetAllKeys(thread, infoLength, 0, *keyArray); // 0: offset
        layoutInfoHandle->GetAllKeysForSerialization(infoLength, keyVector);
        EXPECT_EQ(keyArray->GetLength(), keyVector.size());

        for (int i = 0;i < infoLength; i++) {
            bool result = JSTaggedValue::SameValue(keyArray->Get(i), keyVector[i]);
            EXPECT_TRUE(result);
        }
    }
}

HWTEST_F_L0(LayoutInfoTest, GetAllKeys)
{
    GetAllKeysCommon(thread);
}

HWTEST_F_L0(LayoutInfoTest, GetAllEnumKeys)
{
    GetAllKeysCommon(thread, true);
}
}  // namespace panda::ecmascript
