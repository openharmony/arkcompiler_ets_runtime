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

#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_api/js_api_hashmap.h"
#include "ecmascript/js_api/js_api_hashmap_iterator.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSAPIHashMapTest : public BaseTestWithScope<false> {
protected:
    JSAPIHashMap *CreateHashMap()
    {
        return EcmaContainerCommon::CreateHashMap(thread);
    }

    void Update(JSHandle<JSAPIHashMap>& hashMap, JSMutableHandle<JSTaggedValue>& key,
        JSMutableHandle<JSTaggedValue>& value, std::pair<std::string, std::string> myKeyVal, uint32_t numbers)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        for (uint32_t i = 0; i < numbers; i++) {
            std::string iKey = myKeyVal.first + std::to_string(i);
            std::string iValue = myKeyVal.second + std::to_string(i);
            key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
            value.Update(factory->NewFromStdString(iValue).GetTaggedValue());
            JSAPIHashMap::Set(thread, hashMap, key, value);
        }
    }
};

HWTEST_F_L0(JSAPIHashMapTest, HashMapCreate)
{
    JSAPIHashMap *map = CreateHashMap();
    EXPECT_TRUE(map != nullptr);
}

HWTEST_F_L0(JSAPIHashMapTest, HashMapSetAndGet)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    // test JSAPIHashMap
    JSHandle<JSAPIHashMap> hashMap(thread, CreateHashMap());

    // test IsEmpty
    EXPECT_EQ(hashMap->IsEmpty(), JSTaggedValue::True());

    // test Set exception
    key.Update(JSTaggedValue::Undefined());
    JSAPIHashMap::Set(thread, hashMap, key, value);
    EXPECT_EXCEPTION();

    std::string myKey("mykey");
    std::string myValue("myvalue");
    auto pair = std::make_pair(myKey, myValue);
    Update(hashMap, key, value, pair, NODE_NUMBERS);
    EXPECT_EQ(hashMap->GetSize(), NODE_NUMBERS);

    // test isEmpty
    EXPECT_EQ(hashMap->IsEmpty(), JSTaggedValue::False());

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iKey = myKey + std::to_string(i);
        std::string iValue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());

        // test get
        JSTaggedValue gValue = hashMap->Get(thread, key.GetTaggedValue());
        EXPECT_EQ(gValue, value.GetTaggedValue());
    }
}

HWTEST_F_L0(JSAPIHashMapTest, HashMapRemoveAndHas)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    // test JSAPIHashMap
    JSHandle<JSAPIHashMap> hashMap(thread, CreateHashMap());

    // test Remove Hole
    JSTaggedValue undefined = JSAPIHashMap::Remove(thread, hashMap, JSTaggedValue::Hole());
    EXPECT_EQ(undefined, JSTaggedValue::Undefined());

    // test Remove empty hashmap
    JSTaggedValue undefined1 = JSAPIHashMap::Remove(thread, hashMap, JSTaggedValue(0));
    EXPECT_EQ(undefined1, JSTaggedValue::Undefined());

    std::string myKey("mykey");
    std::string myValue("myvalue");
    auto pair = std::make_pair(myKey, myValue);
    Update(hashMap, key, value, pair, NODE_NUMBERS);
    EXPECT_EQ(hashMap->GetSize(), NODE_NUMBERS);

    // test Remove non-existent
    JSTaggedValue undefined2 = JSAPIHashMap::Remove(thread, hashMap, JSTaggedValue(0));
    EXPECT_EQ(undefined2, JSTaggedValue::Undefined());

    for (uint32_t i = 0; i < NODE_NUMBERS / 2; i++) {
        std::string iKey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        [[maybe_unused]] JSTaggedValue rValue = JSAPIHashMap::Remove(thread, hashMap, key.GetTaggedValue());
    }
    EXPECT_EQ(hashMap->GetSize(), NODE_NUMBERS / 2);

    for (uint32_t i = 0; i < NODE_NUMBERS / 2; i++) {
        std::string iKey = myKey + std::to_string(i);
        std::string iValue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());

        // test has
        JSTaggedValue hasKey = hashMap->HasKey(thread, key.GetTaggedValue());
        EXPECT_EQ(hasKey, JSTaggedValue::False());
        JSTaggedValue hasValue = JSAPIHashMap::HasValue(thread, hashMap, value);
        EXPECT_EQ(hasValue, JSTaggedValue::False());
    }

    for (uint32_t i = NODE_NUMBERS / 2; i < NODE_NUMBERS; i++) {
        std::string iKey = myKey + std::to_string(i);
        std::string iValue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());

        // test has
        JSTaggedValue hasKey = hashMap->HasKey(thread, key.GetTaggedValue());
        EXPECT_EQ(hasKey, JSTaggedValue::True());
        JSTaggedValue hasValue = JSAPIHashMap::HasValue(thread, hashMap, value);
        EXPECT_EQ(hasValue, JSTaggedValue::True());
    }
}

HWTEST_F_L0(JSAPIHashMapTest, HashMapReplaceAndClear)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    // test TaggedHashMap
    JSHandle<JSAPIHashMap> hashMap(thread, CreateHashMap());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    auto pair = std::make_pair(myKey, myValue);
    Update(hashMap, key, value, pair, NODE_NUMBERS);
    EXPECT_EQ(hashMap->GetSize(), NODE_NUMBERS);
    for (uint32_t i = 0; i < NODE_NUMBERS / 2; i++) {
        std::string iKey = myKey + std::to_string(i);
        std::string iValue = myValue + std::to_string(i + 1);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());
        // test replace
        bool success = hashMap->Replace(thread, key.GetTaggedValue(), value.GetTaggedValue());
        EXPECT_EQ(success, true);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS / 2; i++) {
        std::string iKey = myKey + std::to_string(i);
        std::string iValue = myValue + std::to_string(i + 1);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());
        // test get
        JSTaggedValue gValue = hashMap->Get(thread, key.GetTaggedValue());
        EXPECT_EQ(gValue, value.GetTaggedValue());
    }
    for (uint32_t i = NODE_NUMBERS / 2; i < NODE_NUMBERS; i++) {
        std::string iKey = myKey + std::to_string(i);
        std::string iValue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());
        // test get
        JSTaggedValue gValue = hashMap->Get(thread, key.GetTaggedValue());
        EXPECT_EQ(gValue, value.GetTaggedValue());
    }
    for (uint32_t i = 0; i < NODE_NUMBERS / 2; i++) {
        std::string iKey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        [[maybe_unused]] JSTaggedValue rValue = JSAPIHashMap::Remove(thread, hashMap, key.GetTaggedValue());
    }
    hashMap->Clear(thread);
    EXPECT_EQ(hashMap->GetSize(), (uint32_t)0);
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iKey = myKey + std::to_string(i);
        std::string iValue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());
        // test get
        JSTaggedValue gValue = hashMap->Get(thread, key.GetTaggedValue());
        EXPECT_EQ(gValue, JSTaggedValue::Undefined());
        // test has
        JSTaggedValue hasKey = hashMap->HasKey(thread, key.GetTaggedValue());
        EXPECT_EQ(hasKey, JSTaggedValue::False());
        JSTaggedValue hasValue = JSAPIHashMap::HasValue(thread, hashMap, value);
        EXPECT_EQ(hasValue, JSTaggedValue::False());
    }
}

HWTEST_F_L0(JSAPIHashMapTest, JSAPIHashMapIterator)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIHashMap> hashMap(thread, CreateHashMap());
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        value.Update(JSTaggedValue(i));
        JSAPIHashMap::Set(thread, hashMap, key, value);
    }
    // test key or value
    JSHandle<JSTaggedValue> keyIter(factory->NewJSAPIHashMapIterator(hashMap, IterationKind::KEY));
    JSHandle<JSTaggedValue> valueIter(factory->NewJSAPIHashMapIterator(hashMap, IterationKind::VALUE));
    JSMutableHandle<JSTaggedValue> keyIterResult(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> valueIterResult(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS / 2; i++) {
        keyIterResult.Update(JSIterator::IteratorStep(thread, keyIter).GetTaggedValue());
        valueIterResult.Update(JSIterator::IteratorStep(thread, valueIter).GetTaggedValue());
        JSHandle<JSTaggedValue> tmpIterKey = JSIterator::IteratorValue(thread, keyIterResult);
        JSTaggedValue iterKeyFlag = hashMap->HasKey(thread, tmpIterKey.GetTaggedValue());
        EXPECT_EQ(JSTaggedValue::True(), iterKeyFlag);
        JSHandle<JSTaggedValue> tmpIterValue = JSIterator::IteratorValue(thread, valueIterResult);
        JSTaggedValue iterValueFlag = JSAPIHashMap::HasValue(thread, hashMap, tmpIterValue);
        EXPECT_EQ(JSTaggedValue::True(), iterValueFlag);
    }
    // test key and value
    JSHandle<JSTaggedValue> indexKey(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> elementKey(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> iter(factory->NewJSAPIHashMapIterator(hashMap, IterationKind::KEY_AND_VALUE));
    JSMutableHandle<JSTaggedValue> iterResult(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        iterResult.Update(JSIterator::IteratorStep(thread, iter).GetTaggedValue());
        result.Update(JSIterator::IteratorValue(thread, iterResult).GetTaggedValue());
        JSHandle<JSTaggedValue> tmpKey = JSObject::GetProperty(thread, result, indexKey).GetValue();
        JSTaggedValue iterKeyFlag = hashMap->HasKey(thread, tmpKey.GetTaggedValue());
        EXPECT_EQ(JSTaggedValue::True(), iterKeyFlag);
        JSHandle<JSTaggedValue> tmpValue = JSObject::GetProperty(thread, result, elementKey).GetValue();
        JSTaggedValue iterValueFlag = JSAPIHashMap::HasValue(thread, hashMap, tmpValue);
        EXPECT_EQ(JSTaggedValue::True(), iterValueFlag);
    }
    // test delete
    key.Update(JSTaggedValue(NODE_NUMBERS / 2));
    JSTaggedValue rValue = JSAPIHashMap::Remove(thread, hashMap, key.GetTaggedValue());
    EXPECT_EQ(rValue, JSTaggedValue(NODE_NUMBERS / 2));
    for (uint32_t i = NODE_NUMBERS / 2 + 1; i < NODE_NUMBERS; i++) {
        keyIterResult.Update(JSIterator::IteratorStep(thread, keyIter).GetTaggedValue());
        valueIterResult.Update(JSIterator::IteratorStep(thread, valueIter).GetTaggedValue());
        JSHandle<JSTaggedValue> tmpIterKey = JSIterator::IteratorValue(thread, keyIterResult);
        JSTaggedValue iterKeyFlag = hashMap->HasKey(thread, tmpIterKey.GetTaggedValue());
        EXPECT_EQ(JSTaggedValue::True(), iterKeyFlag);
        JSHandle<JSTaggedValue> tmpIterValue = JSIterator::IteratorValue(thread, valueIterResult);
        JSTaggedValue iterValueFlag = JSAPIHashMap::HasValue(thread, hashMap, tmpIterValue);
        EXPECT_EQ(JSTaggedValue::True(), iterValueFlag);
    }
    // test set
    key.Update(JSTaggedValue(NODE_NUMBERS));
    JSAPIHashMap::Set(thread, hashMap, key, key);
    keyIterResult.Update(JSIterator::IteratorStep(thread, keyIter).GetTaggedValue());
    JSHandle<JSTaggedValue> tmpIterKey = JSIterator::IteratorValue(thread, keyIterResult);
    JSTaggedValue iterKeyFlag = hashMap->HasKey(thread, tmpIterKey.GetTaggedValue());
    EXPECT_EQ(JSTaggedValue::True(), iterKeyFlag);
    EXPECT_EQ(hashMap->GetSize(), NODE_NUMBERS);
    keyIterResult.Update(JSIterator::IteratorStep(thread, keyIter).GetTaggedValue());
    EXPECT_EQ(JSTaggedValue::False(), keyIterResult.GetTaggedValue());
}

HWTEST_F_L0(JSAPIHashMapTest, JSAPIHashMapIteratorRBTreeTest)
{
    constexpr uint32_t NODE_NUMBERS = 11;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIHashMap> hashMap(thread, CreateHashMap());
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> valueStr = thread->GlobalConstants()->GetHandledValueString();
    std::vector<int> hashCollisionVector = {1013, 1015, 1021, 1023, 1045, 1047, 1053, 1055, 1077, 1079, 1085};

    JSHandle<TaggedHashArray> hashArray(thread, hashMap->GetTable(thread));
    hashArray = TaggedHashArray::Resize(thread, hashArray, NODE_NUMBERS);
    hashMap->SetTable(thread, hashArray);

    for (size_t i = 0; i < hashCollisionVector.size(); i++) {
        key.Update(JSTaggedValue(hashCollisionVector[i]));
        value.Update(JSTaggedValue(hashCollisionVector[i]));
        JSAPIHashMap::Set(thread, hashMap, key, value);
    }

    JSHandle<JSAPIHashMapIterator> hashmapIterator = factory->NewJSAPIHashMapIterator(hashMap, IterationKind::VALUE);
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(hashmapIterator.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = JSAPIHashMapIterator::Next(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSObject> resultObj(thread, result);
        if (i <= NODE_NUMBERS - 1U) {
            EXPECT_TRUE(JSObject::GetProperty(thread, resultObj, valueStr).GetValue()->IsInt());
        }
    }
}

HWTEST_F_L0(JSAPIHashMapTest, JSAPIHashMapIteratorDiffNodeTest)
{
    constexpr uint32_t NODE_NUMBERS = 1 << 4;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIHashMap> hashMap(thread, CreateHashMap());
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<TaggedHashArray> hashArray(thread, hashMap->GetTable(thread));
    hashArray = TaggedHashArray::Resize(thread, hashArray, NODE_NUMBERS);
    hashMap->SetTable(thread, hashArray);
    std::vector<int> treeNumsVector = {1224, 1285, 1463, 4307, 5135, 5903, 6603, 6780, 8416, 9401, 9740};

    int32_t totalNums = 0;
    // create RBTree Node
    for (size_t i = 0; i < treeNumsVector.size(); i++) {
        key.Update(JSTaggedValue(treeNumsVector[i]));
        value.Update(JSTaggedValue(treeNumsVector[i]));
        if (!TaggedHashArray::SetVal(thread, hashArray, 0, key, value).IsUndefined()) {
            totalNums++;
        }
    }

    // create LinkList Node
    std::vector<int> linkListVector = {1224, 1285, 1463, 4307};
    for (uint32_t hash = 1; hash < NODE_NUMBERS; hash++) {
        for (size_t i = 0; i < linkListVector.size(); i++) {
            key.Update(JSTaggedValue(linkListVector[i]));
            value.Update(JSTaggedValue(linkListVector[i]));
            if (!TaggedHashArray::SetVal(thread, hashArray, hash, key, value).IsUndefined()) {
                totalNums++;
            }
        }
    }

    JSHandle<JSAPIHashMapIterator> hashmapIterator = factory->NewJSAPIHashMapIterator(hashMap, IterationKind::VALUE);
    int32_t count = 0;
    while (!hashmapIterator->GetIteratedHashMap(thread).IsUndefined()) {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(hashmapIterator.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSAPIHashMapIterator::Next(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        count++;
    }
    EXPECT_EQ(count - 1, totalNums);
}

HWTEST_F_L0(JSAPIHashMapTest, JSAPIHashMapIteratorDiffNodeRandomlyTest)
{
    constexpr uint32_t NODE_NUMBERS = 1 << 4;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIHashMap> hashMap(thread, CreateHashMap());
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<TaggedHashArray> hashArray(thread, hashMap->GetTable(thread));
    hashArray = TaggedHashArray::Resize(thread, hashArray, NODE_NUMBERS);
    hashMap->SetTable(thread, hashArray);
    std::vector<int> treeNumsVector = {1224, 1285, 1463, 4307, 5135, 5903, 6603, 6780, 8416, 9401, 9740};
    std::random_device rd;
    std::mt19937 g(rd());
    std::vector<int>p;
    for (uint32_t i = 1; i <= NODE_NUMBERS; i++) {
        p.push_back(i);
    }
    std::shuffle(p.begin(), p.end(), g);
    int32_t totalNums = 0;
    for (uint32_t index = 0; index < p.size(); index++) {
        if (!(g() % 3)) {
            // create RBTree Node
            for (size_t i = 0; i < treeNumsVector.size(); i++) {
                key.Update(JSTaggedValue(treeNumsVector[i]));
                value.Update(JSTaggedValue(treeNumsVector[i]));
                totalNums += (TaggedHashArray::SetVal(thread, hashArray, p[index], key, value).IsUndefined() ? 0 : 1);
            }
            continue;
        }
        // create LinkList Node
        std::vector<int> linkListVector = {1224, 1285, 1463, 4307};
        for (size_t i = 0; i < linkListVector.size(); i++) {
            key.Update(JSTaggedValue(linkListVector[i]));
            value.Update(JSTaggedValue(linkListVector[i]));
            totalNums += (TaggedHashArray::SetVal(thread, hashArray, p[index], key, value).IsUndefined() ? 0 : 1);
        }
    }
    JSHandle<JSAPIHashMapIterator> hashmapIterator = factory->NewJSAPIHashMapIterator(hashMap, IterationKind::VALUE);
    int32_t count = 0;
    while (!hashmapIterator->GetIteratedHashMap(thread).IsUndefined()) {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(hashmapIterator.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSAPIHashMapIterator::Next(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        count++;
    }
    EXPECT_EQ(count - 1, totalNums);
}

HWTEST_F_L0(JSAPIHashMapTest, JSAPIHashMapRBTreeHasValueReplaceGet)
{
    std::vector<int> hashCollisionVector = {1224, 1285, 1463, 4307, 5135, 5903, 6780, 8416, 9401, 9740, 6603};
    uint32_t NODE_NUMBERS = static_cast<uint32_t>(hashCollisionVector.size());
    JSHandle<JSAPIHashMap> hashMap(thread, CreateHashMap());
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    for (uint32_t i = 0; i < NODE_NUMBERS - 1; i++) {
        key.Update(JSTaggedValue(hashCollisionVector[i]));
        value.Update(JSTaggedValue(hashCollisionVector[i]));
        JSAPIHashMap::Set(thread, hashMap, key, value);
    }

    // test RBTree HasValue
    for (uint32_t i = 0; i < NODE_NUMBERS - 1; i++) {
        value.Update(JSTaggedValue(hashCollisionVector[i]));
        JSTaggedValue hasValue = JSAPIHashMap::HasValue(thread, hashMap, value);
        EXPECT_EQ(hasValue, JSTaggedValue::True());
    }
    value.Update(JSTaggedValue(hashCollisionVector[NODE_NUMBERS - 1]));
    JSTaggedValue hasValue = JSAPIHashMap::HasValue(thread, hashMap, value);
    EXPECT_EQ(hasValue, JSTaggedValue::False());

    // test RBTree Replace and Get
    for (uint32_t i = 0; i < NODE_NUMBERS - 1; i++) {
        bool replaceResult = hashMap->Replace(
            thread, JSTaggedValue(hashCollisionVector[i]), JSTaggedValue(hashCollisionVector[i] * 2));
        EXPECT_EQ(replaceResult, true);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS - 1; i++) {
        JSTaggedValue replaceResult = hashMap->Get(
            thread, JSTaggedValue(hashCollisionVector[i]));
        EXPECT_EQ(replaceResult, JSTaggedValue(hashCollisionVector[i] * 2));
    }
}

HWTEST_F_L0(JSAPIHashMapTest, JSAPIHashMapRBTreeSetAllRemove)
{
    std::vector<int> hashCollisionVector = {1224, 1285, 1463, 4307, 5135, 5903, 6780, 8416, 9401, 9740, 6603};
    uint32_t NODE_NUMBERS = static_cast<uint32_t>(hashCollisionVector.size());
    uint32_t REMOVE_NUMBERS = 4;
    JSHandle<JSAPIHashMap> dstHashMap(thread, CreateHashMap());
    JSHandle<JSAPIHashMap> srcHashMap(thread, CreateHashMap());
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> dstValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> srcValue(thread, JSTaggedValue::Undefined());

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(hashCollisionVector[i]));
        dstValue.Update(JSTaggedValue(hashCollisionVector[i]));
        srcValue.Update(JSTaggedValue(hashCollisionVector[i] * 2));
        JSAPIHashMap::Set(thread, dstHashMap, key, dstValue);
        JSAPIHashMap::Set(thread, srcHashMap, key, srcValue);
    }

    // test SetAll and Get
    JSAPIHashMap::SetAll(thread, dstHashMap, srcHashMap);
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        JSTaggedValue replaceResult = dstHashMap->Get(
            thread, JSTaggedValue(hashCollisionVector[i]));
        EXPECT_EQ(replaceResult, JSTaggedValue(hashCollisionVector[i] * 2));
    }

    // test Remove RBTree
    for (uint32_t i = 0; i < REMOVE_NUMBERS; i++) {
        key.Update(JSTaggedValue(hashCollisionVector[i]));
        JSAPIHashMap::Remove(thread, dstHashMap, key.GetTaggedValue());
    }
    EXPECT_EQ(dstHashMap->GetSize(), NODE_NUMBERS - REMOVE_NUMBERS);

    for (uint32_t i = 0; i < REMOVE_NUMBERS; i++) {
        JSTaggedValue getResult = dstHashMap->Get(thread, JSTaggedValue(hashCollisionVector[i]));
        EXPECT_EQ(getResult, JSTaggedValue::Undefined());
    }

    for (uint32_t i = REMOVE_NUMBERS; i < NODE_NUMBERS; i++) {
        JSTaggedValue getResult = dstHashMap->Get(thread, JSTaggedValue(hashCollisionVector[i]));
        EXPECT_EQ(getResult, JSTaggedValue(hashCollisionVector[i] * 2));
    }
}
}  // namespace panda::test
