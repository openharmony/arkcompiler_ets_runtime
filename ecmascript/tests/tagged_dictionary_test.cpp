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

#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/global_env.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class TaggedDictionaryTest : public BaseTestWithScope<false> {
};

static JSHandle<GlobalEnv> GetGlobalEnv(JSThread *thread)
{
    EcmaVM *ecma = thread->GetEcmaVM();
    return ecma->GetGlobalEnv();
}

/**
 * @tc.name: CreateNameDictionary
 * @tc.desc: Call "NameDictionary::Create" function Create a name dictionary.then, check whether the dictionary
 *           is created successfully.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TaggedDictionaryTest, CreateNameDictionary)
{
    int numOfElement = 64;
    JSHandle<NameDictionary> nameDict = NameDictionary::Create(thread, numOfElement);
    EXPECT_TRUE(*nameDict != nullptr);
}

/**
 * @tc.name: NameDictionary_addKeyAndValue
 * @tc.desc: Create a name dictionary, set a key value pair to the dictionary.The key is a string type, call the
 *           "PutIfAbsent" function to add the key and value to the dictionary then,check whether the key is found in
 *           the dictionary.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TaggedDictionaryTest, NameDictionary_addKeyAndValue)
{
    // mock object needed in test
    int numOfElement = 64;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<NameDictionary> dictJShandle(NameDictionary::Create(thread, numOfElement));
    EXPECT_TRUE(*dictJShandle != nullptr);
    JSMutableHandle<NameDictionary> dictHandle(thread, dictJShandle);
    JSHandle<JSTaggedValue> objFun = GetGlobalEnv(thread)->GetObjectFunction();

    // create key and values
    char keyArray[] = "hello";
    JSHandle<EcmaString> stringKey1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII(keyArray);
    JSHandle<JSTaggedValue> key1(stringKey1);
    JSHandle<JSTaggedValue> value1(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));
    PropertyAttributes metaData1;

    char key2Array[] = "hello2";
    JSHandle<EcmaString> stringKey2 = thread->GetEcmaVM()->GetFactory()->NewFromASCII(key2Array);
    JSHandle<JSTaggedValue> key2(stringKey2);
    JSHandle<JSTaggedValue> value2(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));
    PropertyAttributes metaData2;

    // test insert()
    JSHandle<NameDictionary> dict(NameDictionary::PutIfAbsent(thread, dictHandle, key1, value1, metaData1));
    dictHandle.Update(dict.GetTaggedValue());
    EXPECT_EQ(dict->EntriesCount(), 1);

    // test find() and lookup()
    int entry1 = dict->FindEntry(thread, key1.GetTaggedValue());
    EXPECT_EQ(key1.GetTaggedValue(), JSTaggedValue(dict->GetKey(thread, entry1).GetRawData()));
    EXPECT_EQ(value1.GetTaggedValue(), JSTaggedValue(dict->GetValue(thread, entry1).GetRawData()));

    JSHandle<NameDictionary> dict2(NameDictionary::PutIfAbsent(thread, dictHandle, key2, value2, metaData2));
    EXPECT_EQ(dict2->EntriesCount(), 2);
    // test remove()
    dict = NameDictionary::Remove(thread, dictHandle, entry1);
    EXPECT_EQ(-1, dict->FindEntry(thread, key1.GetTaggedValue()));
    EXPECT_EQ(dict->EntriesCount(), 1);
}

/**
 * @tc.name: NameDictionary_GrowCapacity
 * @tc.desc: Create a name dictionary, set a key value pair to the dictionary.The key is a string type, call the
 *           "PutIfAbsent" function to glow the capacity of the dictionary,then check whether the key number and size
 *           of the dictionary have become bigger.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TaggedDictionaryTest, NameDictionary_GrowCapacity)
{
    int numOfElement = 8;
    JSHandle<NameDictionary> dictHandle(NameDictionary::Create(thread, numOfElement));
    EXPECT_TRUE(*dictHandle != nullptr);
    // create key and values
    char keyArray[7] = "hello";
    for (int i = 0; i < 9; i++) {
        JSHandle<NameDictionary> tempHandle = dictHandle;
        keyArray[5] = '1' + static_cast<uint32_t>(i);
        keyArray[6] = 0;
        JSHandle<EcmaString> stringKey = thread->GetEcmaVM()->GetFactory()->NewFromASCII(keyArray);
        JSHandle<JSTaggedValue> key(stringKey);
        JSHandle<JSTaggedValue> keyHandle(key);
        JSHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue(i));
        PropertyAttributes metaData;
        // test insert()
        dictHandle = NameDictionary::PutIfAbsent(thread, tempHandle, keyHandle, valueHandle, metaData);
    }
    EXPECT_EQ(dictHandle->EntriesCount(), 9);
    EXPECT_EQ(dictHandle->Size(), 16);
}

/**
 * @tc.name: NameDictionary_ShrinkCapacity
 * @tc.desc: Create a name dictionary, set a key value pair to the dictionary.The key is a string type, call the
 *           "Remove" function to shrink the capacity of the dictionary,then check whether the key number and size of
 *           the dictionary have become smaller.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TaggedDictionaryTest, NameDictionary_ShrinkCapacity)
{
    int numOfElement = 64;
    JSMutableHandle<NameDictionary> dictHandle(thread, NameDictionary::Create(thread, numOfElement));
    EXPECT_TRUE(*dictHandle != nullptr);
    // create key and values
    uint8_t keyArray[7] = "hello";

    auto stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    for (int i = 0; i < 10; i++) {
        keyArray[5] = '0' + static_cast<uint32_t>(i);
        keyArray[6] = 0;
        JSHandle<JSTaggedValue> key(thread,
                                    stringTable->GetOrInternString(instance, keyArray, utf::Mutf8Size(keyArray), true));
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        PropertyAttributes metaData;
        // test insert()
        JSHandle<NameDictionary> newDict = NameDictionary::PutIfAbsent(thread, dictHandle, key, value, metaData);
        dictHandle.Update(newDict.GetTaggedValue());
    }

    keyArray[5] = '2';
    keyArray[6] = 0;
    JSHandle<JSTaggedValue> arrayHandle(
        thread, stringTable->GetOrInternString(instance, keyArray, utf::Mutf8Size(keyArray), true));
    int entry = dictHandle->FindEntry(thread, arrayHandle.GetTaggedValue());
    EXPECT_NE(entry, -1);

    JSHandle<NameDictionary> newDict1 = NameDictionary::Remove(thread, dictHandle, entry);
    dictHandle.Update(newDict1.GetTaggedValue());
    EXPECT_EQ(dictHandle->EntriesCount(), 9);
    EXPECT_EQ(dictHandle->Size(), 16);
}

/**
 * @tc.name: CreateNumberDictionary
 * @tc.desc: Call "NumberDictionary::Create" function Create a number dictionary.then, check whether the dictionary
 *           is created successfully.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TaggedDictionaryTest, CreateNumberDictionary)
{
    int numOfElement = 64;
    JSHandle<NumberDictionary> numberDict = NumberDictionary::Create(thread, numOfElement);
    EXPECT_TRUE(*numberDict != nullptr);
}

/**
 * @tc.name: NumberDictionary_addKeyAndValue
 * @tc.desc: Create a number dictionary, set a key value pair to the dictionary.The key is a numeric type, call the
 *           "PutIfAbsent" function to add the key and value to the dictionary then,check whether the key is found in
 *           the dictionary.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TaggedDictionaryTest, NumberDictionary_addKeyAndValue)
{
    int numOfElement = 64;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<NumberDictionary> dictJShandle = NumberDictionary::Create(thread, numOfElement);
    EXPECT_TRUE(*dictJShandle != nullptr);
    JSMutableHandle<NumberDictionary> dictHandle(thread, dictJShandle);
    JSHandle<JSTaggedValue> objFun = GetGlobalEnv(thread)->GetObjectFunction();

    // create key and values
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value1(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));
    PropertyAttributes metaData1;

    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2)); // 2: number key
    JSHandle<JSTaggedValue> value2(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));
    PropertyAttributes metaData2;

    // add key and values
    JSHandle<NumberDictionary> dict = NumberDictionary::PutIfAbsent(thread, dictHandle, key1, value1, metaData1);
    dictHandle.Update(dict.GetTaggedValue());
    EXPECT_EQ(dict->EntriesCount(), 1);

    // test find() and lookup()
    int entry1 = dict->FindEntry(thread, key1.GetTaggedValue());
    EXPECT_EQ(key1.GetTaggedValue(), JSTaggedValue(dict->GetKey(thread, entry1).GetRawData()));
    EXPECT_EQ(value1.GetTaggedValue(), JSTaggedValue(dict->GetValue(thread, entry1).GetRawData()));

    JSHandle<NumberDictionary> dict2 = NumberDictionary::PutIfAbsent(thread, dictHandle, key2, value2, metaData2);
    EXPECT_EQ(dict2->EntriesCount(), 2);
    // test remove()
    dict = NumberDictionary::Remove(thread, dictHandle, entry1);
    EXPECT_EQ(-1, dict->FindEntry(thread, key1.GetTaggedValue()));
    EXPECT_EQ(dict->EntriesCount(), 1);
}

/**
 * @tc.name: NumberDictionary_GetAllKey
 * @tc.desc: Create a number dictionary, set a key value pair to the dictionary.The key is a numeric type, call the
 *           "GetAllKeys" function to get the key of the dictionary to the taggedarray,then check whether the length
 *           of the array is equal to the number of keys, and whether the array value is equal to the key value.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TaggedDictionaryTest, NumberDictionary_GetAllKey)
{
    int numOfElement = 64;
    int keyNumbers = 10;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<NumberDictionary> dictHandle = NumberDictionary::Create(thread, numOfElement);
    EXPECT_TRUE(*dictHandle != nullptr);
    JSHandle<TaggedArray> storeKeyArray = factory->NewTaggedArray(keyNumbers);
    // create key and values
    for (int i = keyNumbers - 2; i >= 0; i--) {
        JSHandle<NumberDictionary> tempHandle = dictHandle;
        JSHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> valueHandle(JSTaggedValue::ToString(thread, keyHandle));
        PropertyAttributes metaData;
        dictHandle = NumberDictionary::PutIfAbsent(thread, tempHandle, keyHandle, valueHandle, metaData);
    }
    // get key
    NumberDictionary::GetAllKeys(thread, dictHandle, 1, storeKeyArray); // 1: store to the second position of the array
    for (int i = 1; i < keyNumbers; i++) {
        JSHandle<JSTaggedValue> numberKey(thread, JSTaggedValue(i-1));
        JSHandle<EcmaString> stringKey(JSTaggedValue::ToString(thread, numberKey));
        EXPECT_EQ(storeKeyArray->Get(thread, i), stringKey.GetTaggedValue());
    }
    EXPECT_TRUE(storeKeyArray->Get(thread, 0).IsHole());
}

/**
 * @tc.name: NumberDictionary_GrowCapacity
 * @tc.desc: Create a number dictionary, set a key value pair to the dictionary.The key is a numeric type, call the
 *           "PutIfAbsent" function to glow the capacity of the dictionary,then check whether the key number and size
 *           of the dictionary have become bigger.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TaggedDictionaryTest, NumberDictionary_GrowCapacity)
{
    int numOfElement = 8;
    JSHandle<NumberDictionary> dictHandle = NumberDictionary::Create(thread, numOfElement);
    EXPECT_TRUE(*dictHandle != nullptr);
    // create key and values
    for (int i = 0; i < 9; i++) {
        JSHandle<NumberDictionary> tempHandle = dictHandle;
        JSHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> valueHandle(JSTaggedValue::ToString(thread, keyHandle));
        PropertyAttributes metaData;
        // test insert()
        dictHandle = NumberDictionary::PutIfAbsent(thread, tempHandle, keyHandle, valueHandle, metaData);
    }
    EXPECT_EQ(dictHandle->EntriesCount(), 9);
    EXPECT_EQ(dictHandle->Size(), 16);
}

/**
 * @tc.name: NumberDictionary_ShrinkCapacity
 * @tc.desc: Create a number dictionary, set a key value pair to the dictionary.The key is a numeric type, call the
 *           "Remove" function to shrink the capacity of the dictionary,then check whether the key number and size of
 *           the dictionary have become smaller.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TaggedDictionaryTest, NumberDictionary_ShrinkCapacity)
{
    int numOfElement = 64;
    JSMutableHandle<NumberDictionary> dictHandle(thread, NumberDictionary::Create(thread, numOfElement));
    EXPECT_TRUE(*dictHandle != nullptr);
    // create key and values
    for (int i = 0; i < 10; i++) {
        JSHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> valueHandle(JSTaggedValue::ToString(thread, keyHandle));
        PropertyAttributes metaData;
        JSHandle<NumberDictionary> newDict =
            NumberDictionary::PutIfAbsent(thread, dictHandle, keyHandle, valueHandle, metaData);
        dictHandle.Update(newDict.GetTaggedValue());
    }

    JSHandle<JSTaggedValue> arrayHandle(thread, JSTaggedValue(9));
    int entry = dictHandle->FindEntry(thread, arrayHandle.GetTaggedValue());
    EXPECT_NE(entry, -1);

    JSHandle<NumberDictionary> newDict1 = NumberDictionary::Remove(thread, dictHandle, entry);
    dictHandle.Update(newDict1.GetTaggedValue());
    EXPECT_EQ(dictHandle->EntriesCount(), 9);
    EXPECT_EQ(dictHandle->Size(), 16);
}

/**
 * @tc.name: NumberDictionary_IsMatch
 * @tc.desc: Call the "IsMatch" function to determine whether the key in the dictionary is equal to the defined key.
 *           If it is equal, return true, otherwise return false.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TaggedDictionaryTest, NumberDictionary_IsMatch)
{
    JSTaggedValue numberKey1(0);
    JSTaggedValue numberKey2(1);
    bool result = false;
    // key must be integer
    result = NumberDictionary::IsMatch(thread, numberKey1, numberKey2);
    EXPECT_TRUE(!result);

    result = NumberDictionary::IsMatch(thread, numberKey1, numberKey1);
    EXPECT_TRUE(result);
}
}  // namespace panda::test
