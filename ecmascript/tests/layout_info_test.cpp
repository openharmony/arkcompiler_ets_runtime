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
#include "ecmascript/ic/properties_cache.h"
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
    EXPECT_EQ(layoutInfoHandle->GetKey(thread, 0), key.GetTaggedValue());
    EXPECT_EQ(layoutInfoHandle->GetAttr(thread, 0).GetNormalAttr(), static_cast<uint32_t>(infoLength));
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
    EXPECT_EQ(layoutInfoHandle->GetSortedIndex(thread, 0), 1U);
    EXPECT_EQ(layoutInfoHandle->GetSortedKey(thread, 0), key2.GetTaggedValue());
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
#if ENABLE_V70_OPTIMIZATION
    result = layoutInfoHandle->FindElement(thread, nullptr, key1.GetTaggedValue(), propertiesNumber);
#else
    result = layoutInfoHandle->FindElementWithCache(thread, nullptr, key1.GetTaggedValue(), propertiesNumber);
#endif
    EXPECT_EQ(result, 1); // 1: Index corresponding to key1
#if ENABLE_V70_OPTIMIZATION
    result = layoutInfoHandle->FindElement(thread, nullptr, key5.GetTaggedValue(), propertiesNumber);
#else
    result = layoutInfoHandle->FindElementWithCache(thread, nullptr, key5.GetTaggedValue(), propertiesNumber);
#endif
    EXPECT_EQ(result, -1); // -1: not find
    // extend layoutInfo
    JSHandle<LayoutInfo> newLayoutInfo = factory->ExtendLayoutInfo(layoutInfoHandle, newPropertiesLength);
    for (int i = 5; i < newPropertiesLength; i++) {
        JSHandle<JSTaggedValue> elements(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> elementsKey(JSTaggedValue::ToString(thread, elements));
        defaultAttr.SetOffset(i);
        newLayoutInfo->AddKey(thread, i, elementsKey.GetTaggedValue(), defaultAttr);
    }
#if ENABLE_V70_OPTIMIZATION
    result = newLayoutInfo->FindElement(thread, nullptr, key4.GetTaggedValue(), newPropertiesLength);
#else
    result = newLayoutInfo->FindElementWithCache(thread, nullptr, key4.GetTaggedValue(), newPropertiesLength);
#endif
    EXPECT_EQ(result, 4); // 4: Index corresponding to key4
#if ENABLE_V70_OPTIMIZATION
    result = newLayoutInfo->FindElement(thread, nullptr, key10.GetTaggedValue(), newPropertiesLength);
#else
    result = newLayoutInfo->FindElementWithCache(thread, nullptr, key10.GetTaggedValue(), newPropertiesLength);
#endif
    EXPECT_EQ(result, 10); // 10: Index corresponding to key10
#if ENABLE_V70_OPTIMIZATION
    result = newLayoutInfo->FindElement(thread, nullptr, key5.GetTaggedValue(), newPropertiesLength);
#else
    result = newLayoutInfo->FindElementWithCache(thread, nullptr, key5.GetTaggedValue(), newPropertiesLength);
#endif
    EXPECT_EQ(result, 5); // 5: Index corresponding to key5
#if ENABLE_V70_OPTIMIZATION
    result = newLayoutInfo->FindElement(thread, nullptr, key11.GetTaggedValue(), newPropertiesLength);
#else
    result = newLayoutInfo->FindElementWithCache(thread, nullptr, key11.GetTaggedValue(), newPropertiesLength);
#endif
    EXPECT_EQ(result, -1); // -1: not find
}

HWTEST_F_L0(LayoutInfoTest, FindElementNonSharedCache)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    PropertyAttributes defaultAttr = PropertyAttributes::Default();
    // Create a non-shared HClass
    JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());
    JSHandle<JSHClass> hclass = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT,
                                                        JSHandle<JSTaggedValue>(nullHandle));
    ASSERT_FALSE(hclass->IsJSShared());
    // Create layout info with 12 properties (> 9, exercises binary search path)
    int propsLen = 12;
    JSHandle<LayoutInfo> layoutInfo = factory->CreateLayoutInfo(propsLen);
    std::vector<JSHandle<JSTaggedValue>> keys;
    for (int i = 0; i < propsLen; i++) {
        std::string keyStr = "prop" + std::to_string(i);
        JSHandle<JSTaggedValue> key(factory->NewFromASCII(keyStr.c_str()));
        keys.push_back(key);
        defaultAttr.SetOffset(i);
        layoutInfo->AddKey(thread, i, key.GetTaggedValue(), defaultAttr);
    }
    hclass->SetLayout(thread, layoutInfo.GetTaggedValue());
    hclass->SetNumberOfProps(propsLen);
    int propertiesNumber = layoutInfo->NumberOfElements();
    JSHClass *rawHClass = *hclass;

    // Clear properties cache before test
    PropertiesCache *cache = thread->GetPropertiesCache();
    cache->Clear();
    constexpr int kNotFound = PropertiesCache::NOT_FOUND;

#if ENABLE_V70_OPTIMIZATION
    constexpr int kKeyAbsent = PropertiesCache::NOT_CACHED;

    // 1. First lookup: cache miss -> populates cache with found result
    int result = layoutInfo->FindElement(thread, rawHClass, keys[5].GetTaggedValue(), propertiesNumber);
    EXPECT_EQ(result, 5);
    // Verify cache was populated
    int cachedResult = cache->Get(thread, rawHClass, keys[5].GetTaggedValue());
    EXPECT_EQ(cachedResult, 5);

    // 2. Second lookup: should hit cache
    result = layoutInfo->FindElement(thread, rawHClass, keys[5].GetTaggedValue(), propertiesNumber);
    EXPECT_EQ(result, 5);

    // 3. NOT_FOUND lookup: for non-shared, NOT_FOUND should be cached
    JSHandle<JSTaggedValue> missingKey(factory->NewFromASCII("nonExistent"));
    result = layoutInfo->FindElement(thread, rawHClass, missingKey.GetTaggedValue(), propertiesNumber);
    EXPECT_EQ(result, kNotFound);
    cachedResult = cache->Get(thread, rawHClass, missingKey.GetTaggedValue());
    EXPECT_EQ(cachedResult, kNotFound);

    // 4. Repeated NOT_FOUND lookup: should hit cache
    result = layoutInfo->FindElement(thread, rawHClass, missingKey.GetTaggedValue(), propertiesNumber);
    EXPECT_EQ(result, kNotFound);

    // 5. Verify cache miss returns NOT_CACHED for uncached key
    JSHandle<JSTaggedValue> uncachedKey(factory->NewFromASCII("neverLookedUp"));
    cachedResult = cache->Get(thread, rawHClass, uncachedKey.GetTaggedValue());
    EXPECT_EQ(cachedResult, kKeyAbsent);

    // 6. Check all keys are findable
    for (int i = 0; i < propsLen; i++) {
        result = layoutInfo->FindElement(thread, rawHClass, keys[i].GetTaggedValue(), propertiesNumber);
        EXPECT_EQ(result, i);
    }
#else
    // Without V70 optimization, use FindElementWithCache with nullptr
    int result = layoutInfo->FindElementWithCache(thread, nullptr, keys[5].GetTaggedValue(), propertiesNumber);
    EXPECT_EQ(result, 5);

    JSHandle<JSTaggedValue> missingKey(factory->NewFromASCII("nonExistent"));
    result = layoutInfo->FindElementWithCache(thread, nullptr, missingKey.GetTaggedValue(), propertiesNumber);
    EXPECT_EQ(result, -1);
#endif
}

HWTEST_F_L0(LayoutInfoTest, FindElementSharedCache)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    PropertyAttributes defaultAttr = PropertyAttributes::Default();
    // Create an HClass and set it as shared
    JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());
    JSHandle<JSHClass> hclass = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT,
                                                        JSHandle<JSTaggedValue>(nullHandle));
    hclass->SetIsJSShared(true);
    ASSERT_TRUE(hclass->IsJSShared());
    // Create layout info with 12 properties (> 9, binary search path)
    int propsLen = 12;
    JSHandle<LayoutInfo> layoutInfo = factory->CreateLayoutInfo(propsLen);
    std::vector<JSHandle<JSTaggedValue>> keys;
    for (int i = 0; i < propsLen; i++) {
        std::string keyStr = "shared" + std::to_string(i);
        JSHandle<JSTaggedValue> key(factory->NewFromASCII(keyStr.c_str()));
        keys.push_back(key);
        defaultAttr.SetOffset(i);
        layoutInfo->AddKey(thread, i, key.GetTaggedValue(), defaultAttr);
    }
    hclass->SetLayout(thread, layoutInfo.GetTaggedValue());
    hclass->SetNumberOfProps(propsLen);
    int propertiesNumber = layoutInfo->NumberOfElements();
    JSHClass *rawHClass = *hclass;

    PropertiesCache *cache = thread->GetPropertiesCache();
    cache->Clear();
    constexpr int kNotFound = PropertiesCache::NOT_FOUND;

#if ENABLE_V70_OPTIMIZATION
    constexpr int kKeyAbsent = PropertiesCache::NOT_CACHED;

    // 1. Found result on shared HClass: should be cached
    int result = layoutInfo->FindElement(thread, rawHClass, keys[3].GetTaggedValue(), propertiesNumber);
    EXPECT_EQ(result, 3);
    int cachedResult = cache->Get(thread, rawHClass, keys[3].GetTaggedValue());
    EXPECT_EQ(cachedResult, 3);

    // 2. NOT_FOUND on shared HClass: should NOT be cached
    //    (shared hclass can add properties without creating a new hclass,
    //     so caching NOT_FOUND would give stale results)
    JSHandle<JSTaggedValue> missingKey(factory->NewFromASCII("sharedMissing"));
    result = layoutInfo->FindElement(thread, rawHClass, missingKey.GetTaggedValue(), propertiesNumber);
    EXPECT_EQ(result, kNotFound);
    cachedResult = cache->Get(thread, rawHClass, missingKey.GetTaggedValue());
    // NOT_FOUND should not have been cached for shared hclass
    EXPECT_EQ(cachedResult, kKeyAbsent);

    // 3. All existing keys should still be findable and cached
    for (int i = 0; i < propsLen; i++) {
        result = layoutInfo->FindElement(thread, rawHClass, keys[i].GetTaggedValue(), propertiesNumber);
        EXPECT_EQ(result, i);
        cachedResult = cache->Get(thread, rawHClass, keys[i].GetTaggedValue());
        EXPECT_EQ(cachedResult, i);
    }

    // 4. Multiple different NOT_FOUND keys on shared - none should be cached
    for (int i = 0; i < 5; i++) {
        std::string missStr = "miss" + std::to_string(i);
        JSHandle<JSTaggedValue> missKey(factory->NewFromASCII(missStr.c_str()));
        result = layoutInfo->FindElement(thread, rawHClass, missKey.GetTaggedValue(), propertiesNumber);
        EXPECT_EQ(result, kNotFound);
        cachedResult = cache->Get(thread, rawHClass, missKey.GetTaggedValue());
        EXPECT_EQ(cachedResult, kKeyAbsent);
    }
#else
    // Without V70 optimization, shared distinction doesn't exist in cache logic
    int result = layoutInfo->FindElementWithCache(thread, nullptr, keys[3].GetTaggedValue(), propertiesNumber);
    EXPECT_EQ(result, 3);

    JSHandle<JSTaggedValue> missingKey(factory->NewFromASCII("sharedMissing"));
    result = layoutInfo->FindElementWithCache(thread, nullptr, missingKey.GetTaggedValue(), propertiesNumber);
    EXPECT_EQ(result, -1);
#endif
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
        EXPECT_EQ(keyArray->Get(thread, 0), key3.GetTaggedValue());
        EXPECT_EQ(keys, 1U);
    } else {
        layoutInfoHandle->GetAllKeys(thread, infoLength, 0, *keyArray); // 0: offset
        layoutInfoHandle->GetAllKeysForSerialization(thread, infoLength, keyVector);
        EXPECT_EQ(keyArray->GetLength(), keyVector.size());

        for (int i = 0;i < infoLength; i++) {
            bool result = JSTaggedValue::SameValue(thread, keyArray->Get(thread, i), keyVector[i]);
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
