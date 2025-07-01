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

#include "ecmascript/template_map.h"
#include "ecmascript/tagged_hash_table.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class TemplateMapTest : public BaseTestWithScope<false> {
};

/*
 * @tc.name: CreateTemplateMap
 * @tc.desc: Create a TemplateMap through calling Create function with numberOfElements. Check whether the value
 *           returned through the TemplateMap's calling Get/GetKey/GetValue function is within expectations.Check
 *           whether the value returned through the TaggedArray's(transformed from the TemplateMap) calling GetLength
 *           function is within expectations.
 * expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TemplateMapTest, CreateTemplateMap)
{
    int numElementsTempMap = 128;
    uint32_t tagArrExpectLength = TemplateMap::HashTable::TABLE_HEADER_SIZE +
        numElementsTempMap * TemplateMap::ENTRY_SIZE;

    JSHandle<TemplateMap> templateMap = TemplateMap::Create(thread, numElementsTempMap);
    JSHandle<TaggedArray> handleTagArr(templateMap);

    EXPECT_EQ(handleTagArr->GetLength(), tagArrExpectLength);
    EXPECT_EQ(templateMap->EntriesCount(), 0);
    EXPECT_EQ(templateMap->HoleEntriesCount(), 0);
    EXPECT_EQ(templateMap->Size(), numElementsTempMap);
    for (int32_t i = 0; i < numElementsTempMap; i++) {
        EXPECT_EQ(templateMap->GetKey(thread, i), JSTaggedValue::Undefined());
        EXPECT_EQ(templateMap->GetValue(thread, i), JSTaggedValue::Undefined());
    }
}

/*
 * @tc.name: Insert
 * @tc.desc: Create a TemplateMap, call Insert function with the TemplateMap, a key(JSArray) and a value. Check whether
 *           the value(entry) returned through the TemplateMap's calling FindEntry function with the key(JSArray) is
 *           within expectations. Check whether the value returned through the TemplateMap's calling GetKey/GetValue
 *           function with the value(entry) is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TemplateMapTest, Insert)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TemplateMap> templateMap = TemplateMap::Create(thread);

    JSHandle<JSTaggedValue> tempMapKey(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSTaggedValue> tempMapKey1(JSArray::ArrayCreate(thread, JSTaggedNumber(1)));
    JSHandle<JSTaggedValue> templateArrVal(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> templateArrVal1(factory->NewFromASCII("key1"));

    TemplateMap::Insert(thread, templateMap, tempMapKey, templateArrVal);
    int keyEntry = templateMap->FindEntry(thread, tempMapKey.GetTaggedValue());
    EXPECT_EQ(templateMap->GetKey(thread, keyEntry), tempMapKey.GetTaggedValue());
    EXPECT_EQ(templateMap->GetValue(thread, keyEntry), templateArrVal.GetTaggedValue());

    TemplateMap::Insert(thread, templateMap, tempMapKey, templateArrVal1);
    int keyEntry1 = templateMap->FindEntry(thread, tempMapKey.GetTaggedValue());
    EXPECT_EQ(keyEntry, keyEntry1);
    EXPECT_EQ(templateMap->GetKey(thread, keyEntry1), tempMapKey.GetTaggedValue());
    EXPECT_EQ(templateMap->GetValue(thread, keyEntry1), templateArrVal1.GetTaggedValue());

    TemplateMap::Insert(thread, templateMap, tempMapKey1, templateArrVal1);
    int keyEntry2 = templateMap->FindEntry(thread, tempMapKey1.GetTaggedValue());
    EXPECT_NE(keyEntry1, keyEntry2);
    EXPECT_EQ(templateMap->GetKey(thread, keyEntry2), tempMapKey1.GetTaggedValue());
    EXPECT_EQ(templateMap->GetValue(thread, keyEntry2), templateArrVal1.GetTaggedValue());
    EXPECT_EQ(templateMap->GetValue(thread, keyEntry), templateArrVal1.GetTaggedValue());
}

/*
 * @tc.name: IncreaseEntries
 * @tc.desc: Create a TemplateMap, let it call IncreaseEntries function, check whether the value returned through
 *           calling EntriesCount function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */

HWTEST_F_L0(TemplateMapTest, IncreaseEntries)
{
    int entriesCount = 8;
    JSHandle<TemplateMap> templateMap = TemplateMap::Create(thread);
    for (int32_t i = 0; i < entriesCount; i++) {
        templateMap->IncreaseEntries(thread);
    }
    EXPECT_EQ(templateMap->EntriesCount(), entriesCount);
}

/*
 * @tc.name: IncreaseHoleEntriesCount
 * @tc.desc: Create a TemplateMap, let it call IncreaseEntries function and IncreaseHoleEntriesCount function, check
 *           whether the value returned through calling EntriesCount function and the value returned through calling
 *           HoleEntriesCount function are within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TemplateMapTest, IncreaseHoleEntriesCount)
{
    int entriesCount = 8;
    int holeEntriesCount = 3;
    JSHandle<TemplateMap> templateMap = TemplateMap::Create(thread);
    for (int32_t i = 0; i < entriesCount; i++) {
        templateMap->IncreaseEntries(thread);
    }
    templateMap->IncreaseHoleEntriesCount(thread, holeEntriesCount);

    EXPECT_EQ(templateMap->EntriesCount(), entriesCount - holeEntriesCount);
    EXPECT_EQ(templateMap->HoleEntriesCount(), holeEntriesCount);
}

/*
 * @tc.name: ComputeHashTableSize
 * @tc.desc: Check whether the value returned through a TemplateMap's calling ComputeHashTableSize function is within
 *           expectations.
 * expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TemplateMapTest, ComputeHashTableSize)
{
    JSHandle<TemplateMap> templateMap = TemplateMap::Create(thread);

    uint32_t atLeastSize = 85;
    uint32_t atLeastSize1 = atLeastSize + 1;
    // 128: eighty-five is close to the seventh power of two
    EXPECT_EQ(templateMap->ComputeHashTableSize(atLeastSize), 128);
    // 256: eighty-six is close to the  eighth power of two
    EXPECT_EQ(templateMap->ComputeHashTableSize(atLeastSize1), 256);
    uint32_t atLeastSize2 = 172;
    // 512: one hundred and seventy-two is close to the ninth power of two
    EXPECT_EQ(templateMap->ComputeHashTableSize(atLeastSize2), 512);
}

/*
 * @tc.name: IsNeedGrowHashTable
 * @tc.desc: Create a source TemplateMap, change it through calling IncreaseEntries/IncreaseHoleEntriesCount function.
 *           Check whether the bool returned through the source TemplateMap's calling IsNeedGrowHashTable function is
 *           within expectations.
 * expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TemplateMapTest, IsNeedGrowHashTable)
{
    // Test for the TemplateMap of which the size is 128
    int intNumElementsTempMap = 128;
    JSHandle<TemplateMap> templateMap = TemplateMap::Create(thread, intNumElementsTempMap);
    int atLeastSize = 85;
    int entriesCount = 22;
    int addEntriesCount = atLeastSize - entriesCount;
    for (int32_t i = 0; i < entriesCount; i++) {
        templateMap->IncreaseEntries(thread);
    }
    EXPECT_FALSE(templateMap->IsNeedGrowHashTable(addEntriesCount));
    EXPECT_TRUE(templateMap->IsNeedGrowHashTable(addEntriesCount + 1));
    // Test for the TemplateMap of which the size is 256
    intNumElementsTempMap = 256;
    JSHandle<TemplateMap> templateMap1 = TemplateMap::Create(thread, intNumElementsTempMap);
    atLeastSize = 170;
    entriesCount = 33;
    addEntriesCount = atLeastSize - entriesCount;
    for (int32_t i = 0; i < entriesCount; i++) {
        templateMap1->IncreaseEntries(thread);
    }
    EXPECT_FALSE(templateMap1->IsNeedGrowHashTable(addEntriesCount));
    EXPECT_FALSE(templateMap1->IsNeedGrowHashTable(addEntriesCount + 1));
    EXPECT_TRUE(templateMap1->IsNeedGrowHashTable(addEntriesCount + 2));
}

/*
 * @tc.name: GrowHashTable
 * @tc.desc: Create a source TemplateMap, change it through calling IncreaseEntries function. Check whether the value
 *           returned through the TemplateMap's(returned through calling GrowHashTable function) calling Size function
 *           is within expectations.
 * expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TemplateMapTest, GrowHashTable)
{
    // Test for the TemplateMap of which the size is 128
    int intNumElementsTempMap = 128;
    JSHandle<TemplateMap> templateMap = TemplateMap::Create(thread, intNumElementsTempMap);
    int atLeastSize = 85;
    int entriesCount = 55;
    int addEntriesCount = atLeastSize - entriesCount;
    for (int32_t i = 0; i < entriesCount; i++) {
        templateMap->IncreaseEntries(thread);
    }
    JSHandle<TemplateMap> templateMap1 = TemplateMap::GrowHashTable(thread, templateMap, addEntriesCount);
    EXPECT_EQ(templateMap1->Size(), intNumElementsTempMap);

    JSHandle<TemplateMap> templateMap2 = TemplateMap::GrowHashTable(thread, templateMap, addEntriesCount + 1);
    EXPECT_EQ(templateMap2->Size(), intNumElementsTempMap << 1);
    // Test for the TemplateMap of which the size is 256
    intNumElementsTempMap = 256;
    JSHandle<TemplateMap> templateMap3 = TemplateMap::Create(thread, intNumElementsTempMap);
    atLeastSize = 170;
    entriesCount = 66;
    addEntriesCount = atLeastSize - entriesCount;
    for (int32_t i = 0; i < entriesCount; i++) {
        templateMap3->IncreaseEntries(thread);
    }
    JSHandle<TemplateMap> templateMap4 = TemplateMap::GrowHashTable(thread, templateMap3, addEntriesCount);
    EXPECT_EQ(templateMap4->Size(), intNumElementsTempMap);

    JSHandle<TemplateMap> templateMap5 = TemplateMap::GrowHashTable(thread, templateMap3, addEntriesCount + 1);
    EXPECT_EQ(templateMap5->Size(), intNumElementsTempMap);
    // 2: means needing to grow
    JSHandle<TemplateMap> templateMap6 = TemplateMap::GrowHashTable(thread, templateMap3, addEntriesCount + 2);
    // 2: means twice as much
    EXPECT_EQ(templateMap6->Size(), intNumElementsTempMap << 2);
}

/*
 * @tc.name: RecalculateTableSize
 * @tc.desc: Check whether the value returned through calling RecalculateTableSize function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TemplateMapTest, RecalculateTableSize)
{
    int currentSize = 128, atLeastSize = 33;
    EXPECT_EQ(TemplateMap::RecalculateTableSize(currentSize, atLeastSize), currentSize);
    // 2: means half
    EXPECT_EQ(TemplateMap::RecalculateTableSize(currentSize, atLeastSize - 1), currentSize / 2);
    // 4: shift left by two digits
    currentSize = TemplateMap::MIN_SIZE * 4;
    for (int i = currentSize; i >= 0; i--) {
        EXPECT_EQ(TemplateMap::RecalculateTableSize(currentSize, i), currentSize);
    }
}

/*
 * @tc.name: GetAllKeys
 * @tc.desc: Create a source TemplateMap, change the source TemplateMap through calling Insert function some times.
 *           Create a target TaggedArray, change the target TaggedArray through the changed source TemplateMap's calling
 *           GetAllKeys function with the offset at which the copying of the elements start and the target TaggedArray
 *           Check whether the target TaggedArray is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TemplateMapTest, GetAllKeys)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    int numElements = 32;
    JSHandle<TemplateMap> templateMap = TemplateMap::Create(thread);
    // insert key and value
    for (int i = 0; i < numElements; i++) {
        JSHandle<JSTaggedValue> tempMapKey(JSArray::ArrayCreate(thread, JSTaggedNumber(i)));
        JSHandle<JSTaggedValue> tempMapValue(thread, JSTaggedValue(i));
        templateMap = TemplateMap::Insert(thread, templateMap, tempMapKey, tempMapValue);
    }

    JSHandle<TaggedArray> storeKeyArray = factory->NewTaggedArray(55);  // 55 : means the length of array
    templateMap->GetAllKeys(thread, 5, *storeKeyArray);  // 5: means the index of array
    for (int32_t i = 0; i < numElements; i++) {
        EXPECT_NE(templateMap->GetKey(thread, i), JSTaggedValue::Undefined());
        // 5: get value from subscript five
        EXPECT_EQ(templateMap->GetKey(thread, i), storeKeyArray->Get(thread, i + 5));
    }
}

/*
 * @tc.name: GetAllKeysIntoVector
 * @tc.desc: Create a source TemplateMap, change the source TemplateMap through calling Insert function some times.
 *           Create a target std::vector<JSTaggedValue>,change the target std::vector<JSTaggedValue> through the changed
 *           source TemplateMap's calling GetAllKeysIntoVector function with the target std::vector<JSTaggedValue>.Check
 *           whether the target std::vector<JSTaggedValue> is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TemplateMapTest, GetAllKeysIntoVector)
{
    int numElements = 33;
    JSHandle<TemplateMap> templateMap = TemplateMap::Create(thread);
    // insert key and value
    for (int i = 0; i < numElements; i++) {
        JSHandle<JSTaggedValue> tempMapKey(JSArray::ArrayCreate(thread, JSTaggedNumber(i)));
        JSHandle<JSTaggedValue> tempMapValue(thread, JSTaggedValue(i));
        templateMap = TemplateMap::Insert(thread, templateMap, tempMapKey, tempMapValue);
    }
    std::vector<JSTaggedValue> storeKeyVector = {};
    templateMap->GetAllKeysIntoVector(thread, storeKeyVector);
    for (int32_t i = 0; i < numElements; i++) {
        EXPECT_NE(templateMap->GetKey(thread, i), JSTaggedValue::Undefined());
        EXPECT_EQ(templateMap->GetKey(thread, i), storeKeyVector[i]);
    }
}

/*
 * @tc.name: FindInsertIndex
 * @tc.desc: Create a source TemplateMap, change the source TemplateMap through calling Insert function some times.
 *           Get the next InsertIndex through the changed source TemplateMap's calling FindInsertIndex function with the
 *           hash of the Key which exists in the changed TemplateMap. Check whether the JSTaggedValue returned through
 *           the changed source TemplateMap's calling GetKey function with the next InsertIndex is Undefined.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TemplateMapTest, FindInsertIndex)
{
    int numElements = 33;
    JSHandle<TemplateMap> templateMap = TemplateMap::Create(thread);
    // insert key and value
    for (int i = 0; i < numElements; i++) {
        JSHandle<JSTaggedValue> tempMapKey(JSArray::ArrayCreate(thread, JSTaggedNumber(i)));
        JSHandle<JSTaggedValue> tempMapValue(thread, JSTaggedValue(i));
        templateMap = TemplateMap::Insert(thread, templateMap, tempMapKey, tempMapValue);
    }

    for (int i = 0; i < numElements; i++) {
        JSHandle<JSTaggedValue> tempMapKey(JSArray::ArrayCreate(thread, JSTaggedNumber(i)));
        uint32_t hashValue = static_cast<uint32_t>(TemplateMap::Hash(thread, tempMapKey.GetTaggedValue()));
        EXPECT_EQ(JSTaggedValue::Undefined(), templateMap->GetKey(
            thread,
            templateMap->FindInsertIndex(thread, hashValue)));
    }
}
}  // namespace panda::test
