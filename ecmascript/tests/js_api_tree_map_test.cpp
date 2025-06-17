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
#include "ecmascript/js_api/js_api_tree_map.h"
#include "ecmascript/js_api/js_api_tree_map_iterator.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_tree.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {
class JSAPITreeMapTest : public BaseTestWithScope<false> {
protected:
    JSAPITreeMap *CreateTreeMap()
    {
        return *EcmaContainerCommon::CreateTreeMap(thread);
    }
};

HWTEST_F_L0(JSAPITreeMapTest, TreeMapCreate)
{
    JSAPITreeMap *map = CreateTreeMap();
    EXPECT_TRUE(map != nullptr);
}

HWTEST_F_L0(JSAPITreeMapTest, TreeMapSetAndGet)
{
    constexpr int NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    // test JSAPITreeMap
    JSHandle<JSAPITreeMap> tmap(thread, CreateTreeMap());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPITreeMap::Set(thread, tmap, key, value);
    }
    EXPECT_EQ(tmap->GetSize(thread), NODE_NUMBERS);

    // test Set exception
    key.Update(JSTaggedValue::Hole());
    JSAPITreeMap::Set(thread, tmap, key, value);
    EXPECT_EXCEPTION();

    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        // test get
        JSTaggedValue gvalue = JSAPITreeMap::Get(thread, tmap, key);
        EXPECT_EQ(gvalue, value.GetTaggedValue());
    }
}

HWTEST_F_L0(JSAPITreeMapTest, TreeMapDeleteAndHas)
{
    constexpr int NODE_NUMBERS = 64;
    constexpr int REMOVE_SIZE = 48;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    // test JSAPITreeMap
    JSHandle<JSAPITreeMap> tmap(thread, CreateTreeMap());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPITreeMap::Set(thread, tmap, key, value);
    }
    EXPECT_EQ(tmap->GetSize(thread), NODE_NUMBERS);

    for (int i = 0; i < REMOVE_SIZE; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        [[maybe_unused]] JSTaggedValue dvalue = JSAPITreeMap::Delete(thread, tmap, key);
    }
    EXPECT_EQ(tmap->GetSize(thread), NODE_NUMBERS - REMOVE_SIZE);

    for (int i = 0; i < REMOVE_SIZE; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        // test has
        bool hasKey = JSAPITreeMap::HasKey(thread, tmap, key);
        EXPECT_EQ(hasKey, false);
        bool hasValue = tmap->HasValue(thread, value);
        EXPECT_EQ(hasValue, false);
    }

    for (int i = REMOVE_SIZE; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        // test has
        bool hasKey = JSAPITreeMap::HasKey(thread, tmap, key);
        EXPECT_EQ(hasKey, true);
        bool hasValue = tmap->HasValue(thread, value);
        EXPECT_EQ(hasValue, true);
    }
}

HWTEST_F_L0(JSAPITreeMapTest, TreeMapReplaceAndClear)
{
    constexpr int NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    // test TaggedTreeMap
    JSHandle<JSAPITreeMap> tmap(thread, CreateTreeMap());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPITreeMap::Set(thread, tmap, key, value);
    }
    EXPECT_EQ(tmap->GetSize(thread), NODE_NUMBERS);

    for (int i = 0; i < NODE_NUMBERS / 2; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i + 1);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        // test replace
        bool success = JSAPITreeMap::Replace(thread, tmap, key, value);
        EXPECT_EQ(success, true);
    }

    {
        std::string ikey = myKey + std::to_string(NODE_NUMBERS);
        std::string ivalue = myValue + std::to_string(NODE_NUMBERS + 1);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        bool success = JSAPITreeMap::Replace(thread, tmap, key, value);
        EXPECT_FALSE(success);
    }

    for (int i = 0; i < NODE_NUMBERS / 2; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i + 1);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        // test get
        JSTaggedValue gvalue = JSAPITreeMap::Get(thread, tmap, key);
        EXPECT_EQ(gvalue, value.GetTaggedValue());
    }

    for (int i = NODE_NUMBERS / 2; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        // test get
        JSTaggedValue gvalue = JSAPITreeMap::Get(thread, tmap, key);
        EXPECT_EQ(gvalue, value.GetTaggedValue());
    }

    for (int i = 0; i < NODE_NUMBERS / 2; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        [[maybe_unused]] JSTaggedValue dvalue = JSAPITreeMap::Delete(thread, tmap, key);
    }

    JSAPITreeMap::Clear(thread, tmap);
    EXPECT_EQ(tmap->GetSize(thread), 0);
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        // test get
        JSTaggedValue gvalue = JSAPITreeMap::Get(thread, tmap, key);
        EXPECT_EQ(gvalue, JSTaggedValue::Undefined());

        // test has
        bool hasKey = JSAPITreeMap::HasKey(thread, tmap, key);
        EXPECT_EQ(hasKey, false);
        bool hasValue = tmap->HasValue(thread, value);
        EXPECT_EQ(hasValue, false);
    }
}

HWTEST_F_L0(JSAPITreeMapTest, JSAPITreeMapIterator)
{
    constexpr int NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPITreeMap> tmap(thread, CreateTreeMap());

    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    for (int i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        value.Update(JSTaggedValue(i + 1));
        JSAPITreeMap::Set(thread, tmap, key, value);
    }

    // test key or value
    JSHandle<JSTaggedValue> keyIter(factory->NewJSAPITreeMapIterator(tmap, IterationKind::KEY));
    JSHandle<JSTaggedValue> valueIter(factory->NewJSAPITreeMapIterator(tmap, IterationKind::VALUE));
    JSMutableHandle<JSTaggedValue> keyIterResult(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> valueIterResult(thread, JSTaggedValue::Undefined());
    for (int i = 0; i < NODE_NUMBERS / 2; i++) {
        keyIterResult.Update(JSIterator::IteratorStep(thread, keyIter).GetTaggedValue());
        valueIterResult.Update(JSIterator::IteratorStep(thread, valueIter).GetTaggedValue());
        EXPECT_EQ(i, JSIterator::IteratorValue(thread, keyIterResult)->GetInt());
        EXPECT_EQ(i + 1, JSIterator::IteratorValue(thread, valueIterResult)->GetInt());
    }

    // test key and value
    JSHandle<JSTaggedValue> indexKey(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> elementKey(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> iter(factory->NewJSAPITreeMapIterator(tmap, IterationKind::KEY_AND_VALUE));
    JSMutableHandle<JSTaggedValue> iterResult(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    for (int i = 0; i < NODE_NUMBERS; i++) {
        iterResult.Update(JSIterator::IteratorStep(thread, iter).GetTaggedValue());
        result.Update(JSIterator::IteratorValue(thread, iterResult).GetTaggedValue());
        EXPECT_EQ(i, JSObject::GetProperty(thread, result, indexKey).GetValue()->GetInt());
        EXPECT_EQ(i + 1, JSObject::GetProperty(thread, result, elementKey).GetValue()->GetInt());
    }

    // test delete
    key.Update(JSTaggedValue(NODE_NUMBERS / 2));
    JSTaggedValue dvalue = JSAPITreeMap::Delete(thread, tmap, key);
    EXPECT_EQ(dvalue, JSTaggedValue(NODE_NUMBERS / 2 + 1));
    for (int i = NODE_NUMBERS / 2 + 1; i < NODE_NUMBERS; i++) {
        keyIterResult.Update(JSIterator::IteratorStep(thread, keyIter).GetTaggedValue());
        valueIterResult.Update(JSIterator::IteratorStep(thread, valueIter).GetTaggedValue());
        EXPECT_EQ(i, JSIterator::IteratorValue(thread, keyIterResult)->GetInt());
        EXPECT_EQ(i + 1, JSIterator::IteratorValue(thread, valueIterResult)->GetInt());
    }

    // test set
    key.Update(JSTaggedValue(NODE_NUMBERS));
    JSAPITreeMap::Set(thread, tmap, key, key);
    keyIterResult.Update(JSIterator::IteratorStep(thread, keyIter).GetTaggedValue());
    EXPECT_EQ(NODE_NUMBERS, JSIterator::IteratorValue(thread, keyIterResult)->GetInt());

    // test end
    keyIterResult.Update(JSIterator::IteratorStep(thread, keyIter).GetTaggedValue());
    EXPECT_EQ(JSTaggedValue::False(), keyIterResult.GetTaggedValue());
}

HWTEST_F_L0(JSAPITreeMapTest, TreeMapGetKeyAndGetValue)
{
    constexpr int NODE_NUMBERS = 8;
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    // init treemap
    JSHandle<JSAPITreeMap> tmap(thread, CreateTreeMap());
    for (int i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        if (i == NODE_NUMBERS / 2) {
            value.Update(JSTaggedValue::Hole());
        } else {
            value.Update(JSTaggedValue(i));
        }
        JSAPITreeMap::Set(thread, tmap, key, value);
    }
    EXPECT_EQ(tmap->GetSize(thread), NODE_NUMBERS);

    // test GetKey and GetValue
    for (int i = 0; i < NODE_NUMBERS; i++) {
        EXPECT_EQ(tmap->GetKey(thread, i), JSTaggedValue(i));
        if (i == NODE_NUMBERS / 2) {
            EXPECT_EQ(tmap->GetValue(thread, i), JSTaggedValue::Undefined());
        } else {
            EXPECT_EQ(tmap->GetValue(thread, i), JSTaggedValue(i));
        }
    }
    EXPECT_EQ(tmap->GetKey(thread, -1), JSTaggedValue::Undefined());
}
}  // namespace panda::test
