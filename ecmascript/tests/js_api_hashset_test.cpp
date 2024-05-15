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
#include "ecmascript/js_api/js_api_hashset.h"
#include "ecmascript/js_api/js_api_hashset_iterator.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSAPIHashSetTest : public BaseTestWithScope<false> {
protected:
    JSAPIHashSet *CreateHashSet()
    {
        return EcmaContainerCommon::CreateHashSet(thread);
    }

    void Update(JSHandle<JSAPIHashSet>& hashSet, JSMutableHandle<JSTaggedValue> value, std::string& myValue,
        uint32_t numbers)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        for (uint32_t i = 0; i < numbers; i++) {
            std::string iValue = myValue + std::to_string(i);
            value.Update(factory->NewFromStdString(iValue).GetTaggedValue());
            JSAPIHashSet::Add(thread, hashSet, value);
        }
        EXPECT_EQ(hashSet->GetSize(), numbers);
    }
};

HWTEST_F_L0(JSAPIHashSetTest, HashSetCreate)
{
    JSAPIHashSet *set = CreateHashSet();
    EXPECT_TRUE(set != nullptr);
}

HWTEST_F_L0(JSAPIHashSetTest, HashSetAddAndHas)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSAPIHashSet> hashSet(thread, CreateHashSet());
    std::string myValue("myvalue");
    Update(hashSet, value, myValue, NODE_NUMBERS);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iValue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());

        // test has
        JSTaggedValue bHas = hashSet->Has(thread, value.GetTaggedValue());
        EXPECT_EQ(bHas, JSTaggedValue::True());
    }

    // test add exception
    JSHandle<JSTaggedValue> hole(thread, JSTaggedValue::Hole());
    JSAPIHashSet::Add(thread, hashSet, hole);
    EXPECT_EXCEPTION();

    // test Has exception
    JSTaggedValue exceptionHas = hashSet->Has(thread, JSTaggedValue::Hole());
    EXPECT_EQ(exceptionHas, JSTaggedValue::Exception());
    EXPECT_EXCEPTION();
}

HWTEST_F_L0(JSAPIHashSetTest, HashSetRemoveAndHas)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // test JSAPIHashSet
    JSHandle<JSAPIHashSet> hashSet(thread, CreateHashSet());
    std::string myValue("myvalue");
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    Update(hashSet, value, myValue, NODE_NUMBERS);

    for (uint32_t i = 0; i < NODE_NUMBERS / 2; i++) {
        std::string iValue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());
        [[maybe_unused]] JSTaggedValue rvalue = JSAPIHashSet::Remove(thread, hashSet, value.GetTaggedValue());
    }
    EXPECT_EQ(hashSet->GetSize(), NODE_NUMBERS / 2);

    for (uint32_t i = 0; i < NODE_NUMBERS / 2; i++) {
        std::string iValue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());

        // test has
        JSTaggedValue has = hashSet->Has(thread, value.GetTaggedValue());
        EXPECT_EQ(has, JSTaggedValue::False());
    }

    for (uint32_t i = NODE_NUMBERS / 2; i < NODE_NUMBERS; i++) {
        std::string iValue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());

        // test has
        JSTaggedValue has = hashSet->Has(thread, value.GetTaggedValue());
        EXPECT_EQ(has, JSTaggedValue::True());
    }

    // test Remove exception
    JSTaggedValue exceptionValue = JSAPIHashSet::Remove(thread, hashSet, JSTaggedValue::Hole());
    EXPECT_EQ(exceptionValue, JSTaggedValue::Exception());
    EXPECT_EXCEPTION();
}

HWTEST_F_L0(JSAPIHashSetTest, JSAPIHashSetRemoveRBTreeTest)
{
    std::vector<int> hashCollisionVector = {1224, 1285, 1463, 4307, 5135, 5903, 6603, 6780, 8416, 9401, 9740};
    uint32_t NODE_NUMBERS = static_cast<uint32_t>(hashCollisionVector.size());
    constexpr uint32_t REMOVE_NUMBERS = 4;
    JSHandle<JSAPIHashSet> hashSet(thread, CreateHashSet());

    // test Remove empty
    JSTaggedValue emptyValue = JSAPIHashSet::Remove(thread, hashSet, JSTaggedValue(0));
    EXPECT_EQ(emptyValue, JSTaggedValue::False());

    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    for (size_t i = 0; i < hashCollisionVector.size(); i++) {
        value.Update(JSTaggedValue(hashCollisionVector[i]));
        JSAPIHashSet::Add(thread, hashSet, value);
    }

    // test Remove non-existent value
    JSTaggedValue nonExistentValue = JSAPIHashSet::Remove(thread, hashSet, JSTaggedValue(0));
    EXPECT_EQ(nonExistentValue, JSTaggedValue::False());

    // test Remove RBTree
    for (uint32_t i = 0; i < REMOVE_NUMBERS; i++) {
        value.Update(JSTaggedValue(hashCollisionVector[i]));
        JSAPIHashSet::Remove(thread, hashSet, value.GetTaggedValue());
    }
    EXPECT_EQ(hashSet->GetSize(), NODE_NUMBERS - REMOVE_NUMBERS);

    for (uint32_t i = 0; i < REMOVE_NUMBERS; i++) {
        value.Update(JSTaggedValue(hashCollisionVector[i]));
        JSTaggedValue has = hashSet->Has(thread, value.GetTaggedValue());
        EXPECT_EQ(has, JSTaggedValue::False());
    }

    for (uint32_t i = REMOVE_NUMBERS; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(hashCollisionVector[i]));
        JSTaggedValue has = hashSet->Has(thread, value.GetTaggedValue());
        EXPECT_EQ(has, JSTaggedValue::True());
    }
}

HWTEST_F_L0(JSAPIHashSetTest, HashSetClearAddHas)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myValue("myvalue");
    JSHandle<JSAPIHashSet> hashSet(thread, CreateHashSet());
    Update(hashSet, value, myValue, NODE_NUMBERS);

    hashSet->Clear(thread);
    JSTaggedValue isEmpty = hashSet->IsEmpty();
    EXPECT_EQ(isEmpty, JSTaggedValue::True());
}

HWTEST_F_L0(JSAPIHashSetTest, JSAPIHashSetIterator)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIHashSet> hashSet(thread, CreateHashSet());

    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i));
        JSAPIHashSet::Add(thread, hashSet, value);
    }

    // test value
    JSHandle<JSTaggedValue> valueIter(factory->NewJSAPIHashSetIterator(hashSet, IterationKind::VALUE));
    JSMutableHandle<JSTaggedValue> valueIterResult(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        valueIterResult.Update(JSIterator::IteratorStep(thread, valueIter).GetTaggedValue());
        JSHandle<JSTaggedValue> tmpIterValue = JSIterator::IteratorValue(thread, valueIterResult);
        JSTaggedValue iterValueFlag = hashSet->Has(thread, tmpIterValue.GetTaggedValue());
        EXPECT_EQ(JSTaggedValue::True(), iterValueFlag);
    }
    // test end
    valueIterResult.Update(JSIterator::IteratorStep(thread, valueIter).GetTaggedValue());
    EXPECT_EQ(JSTaggedValue::False(), valueIterResult.GetTaggedValue());

    // test key and value
    JSHandle<JSTaggedValue> indexKey(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> elementKey(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> iter(factory->NewJSAPIHashSetIterator(hashSet, IterationKind::KEY_AND_VALUE));
    JSMutableHandle<JSTaggedValue> iterResult(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        iterResult.Update(JSIterator::IteratorStep(thread, iter).GetTaggedValue());
        result.Update(JSIterator::IteratorValue(thread, iterResult).GetTaggedValue());
        EXPECT_EQ(JSTaggedValue(i), JSObject::GetProperty(thread, result, indexKey).GetValue().GetTaggedValue());
        JSHandle<JSTaggedValue> tmpValue = JSObject::GetProperty(thread, result, elementKey).GetValue();
        JSTaggedValue iterValueFlag = hashSet->Has(thread, tmpValue.GetTaggedValue());
        EXPECT_EQ(JSTaggedValue::True(), iterValueFlag);
    }
}

HWTEST_F_L0(JSAPIHashSetTest, JSAPIHashSetIteratorRBTreeTest)
{
    constexpr uint32_t NODE_NUMBERS = 11;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIHashSet> hashSet(thread, CreateHashSet());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> valueStr = thread->GlobalConstants()->GetHandledValueString();
    std::vector<int> hashCollisionVector = {1224, 1285, 1463, 4307, 5135, 5903, 6603, 6780, 8416, 9401, 9740};

    for (size_t i = 0; i < hashCollisionVector.size(); i++) {
        value.Update(JSTaggedValue(hashCollisionVector[i]));
        JSAPIHashSet::Add(thread, hashSet, value);
    }

    JSHandle<JSAPIHashSetIterator> hashsetIterator = factory->NewJSAPIHashSetIterator(hashSet, IterationKind::VALUE);
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(hashsetIterator.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = JSAPIHashSetIterator::Next(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSObject> resultObj(thread, result);
        if (i <= NODE_NUMBERS - 1U) {
            EXPECT_TRUE(JSObject::GetProperty(thread, resultObj, valueStr).GetValue()->IsInt());
        }
    }
}
}  // namespace panda::test
