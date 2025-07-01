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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_symbol.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/property_attributes.h"
#include "ecmascript/tagged_hash_table.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/transitions_dictionary.h"

using namespace panda::ecmascript;

namespace panda::test {
class TransitionsDictionaryTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(TransitionsDictionaryTest, IsMatch)
{
    JSHandle<JSTaggedValue> key(thread, JSTaggedValue::True());
    JSHandle<JSTaggedValue> otherKey(thread, JSTaggedValue::False());
    JSHandle<JSTaggedValue> metaData(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> otherDetails(thread, JSTaggedValue::Null());
    bool result = TransitionsDictionary::IsMatch(key.GetTaggedValue(), metaData.GetTaggedValue(),
        otherKey.GetTaggedValue(), otherDetails.GetTaggedValue());
    EXPECT_FALSE(result);

    result = TransitionsDictionary::IsMatch(key.GetTaggedValue(), metaData.GetTaggedValue(),
        key.GetTaggedValue(), metaData.GetTaggedValue());
    EXPECT_TRUE(result);
}

HWTEST_F_L0(TransitionsDictionaryTest, Hash)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();

    // test when key is string.
    JSHandle<JSTaggedValue> key1(factory->NewFromStdString("k"));
    JSHandle<JSTaggedValue> metaData1(thread, JSTaggedValue(1)); // test metaData : 1
    int hash = TransitionsDictionary::Hash(thread, key1.GetTaggedValue(), metaData1.GetTaggedValue());
    // "k" : 107, hashSeed : 0, shift : 5, metaData : 1
    EXPECT_EQ(hash, 108); // 108 : (0 << 5) - 0 + 107 + 1

    JSHandle<JSTaggedValue> key2(factory->NewFromStdString("key"));
    hash = TransitionsDictionary::Hash(thread, key2.GetTaggedValue(), metaData1.GetTaggedValue());
    EXPECT_EQ(hash, 106080);

    // test when key is symbol.
    JSHandle<JSTaggedValue> symbolName(factory->NewFromStdString("s"));
    JSHandle<JSSymbol> privateNameSymbol = factory->NewPrivateNameSymbol(symbolName);
    JSHandle<JSTaggedValue> symbolValue = JSHandle<JSTaggedValue>::Cast(privateNameSymbol);
    JSHandle<JSTaggedValue> metaData2(thread, JSTaggedValue(2)); // test metaData : 2
    hash = TransitionsDictionary::Hash(thread, symbolValue.GetTaggedValue(), metaData2.GetTaggedValue());
    EXPECT_EQ(hash, 117); // 117 : 115 + 2
}

HWTEST_F_L0(TransitionsDictionaryTest, GetKeyIndex)
{
    int entry = 10;
    EXPECT_EQ(TransitionsDictionary::GetKeyIndex(entry), 33); // 33 : 3 + 10 * 3 + 0
}

HWTEST_F_L0(TransitionsDictionaryTest, GetValueIndex)
{
    int entry = 10;
    EXPECT_EQ(TransitionsDictionary::GetValueIndex(entry), 34); // 34 : 3 + 10 * 3 + 1
}

HWTEST_F_L0(TransitionsDictionaryTest, GetEntryIndex)
{
    int entry = 10;
    EXPECT_EQ(TransitionsDictionary::GetEntryIndex(entry), 33); // 33 : 3 + 10 * 3
}

HWTEST_F_L0(TransitionsDictionaryTest, Create)
{
    int numberOfElements = 8;
    JSHandle<TransitionsDictionary> transDic = TransitionsDictionary::Create(thread, numberOfElements);
    EXPECT_EQ(transDic->GetLength(), 27U); // 27 : 3 + 8 * 3
    EXPECT_EQ(transDic->EntriesCount(), 0);
    EXPECT_EQ(transDic->HoleEntriesCount(), 0);
}

HWTEST_F_L0(TransitionsDictionaryTest, Shrink)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    int numberOfElements = 64;
    JSHandle<TransitionsDictionary> transDic = TransitionsDictionary::Create(thread, numberOfElements);
    EXPECT_EQ(transDic->GetLength(), 195U); // 195 : 3 + 64 * 3
    EXPECT_EQ(transDic->EntriesCount(), 0);

    JSHandle<JSTaggedValue> metaData(thread, JSTaggedValue::Undefined());
    int eleNum = 7;
    for (int index = 0; index < eleNum; index++) {
        std::string keyStr = "key" + std::to_string(index);
        JSHandle<JSTaggedValue> key(factory->NewFromStdString(keyStr));
        JSHandle<JSTaggedValue> value(factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT));
        TransitionsDictionary::PutIfAbsent(thread, transDic, key, value, metaData);
    }
    JSHandle<TransitionsDictionary> transDicAfterShink = TransitionsDictionary::Shrink(thread, transDic);
    EXPECT_EQ(transDicAfterShink->GetLength(), 51U); // (1 << (32 - Clz((7 + (7 >> 1)) - 1))) * 3 + 3
    EXPECT_EQ(transDic->EntriesCount(), eleNum);
}

HWTEST_F_L0(TransitionsDictionaryTest, Get_Set_Attributes)
{
    int numberOfElements = 8;
    JSHandle<TransitionsDictionary> transDic = TransitionsDictionary::Create(thread, numberOfElements);
    uint32_t length = transDic->GetLength();
    EXPECT_EQ(length, 27U); // 27 : 3 + 8 * 3

    for (int index = 0; index < numberOfElements; index++) {
        transDic->SetAttributes(thread, index, JSTaggedValue(index));
        JSTaggedValue value = transDic->GetAttributes(thread, index);
        EXPECT_EQ(value, JSTaggedValue(index));
    }
}

using TestCommonCB = std::function<void(JSThread *, int index,
                                        JSHandle<JSTaggedValue> &, JSHandle<JSTaggedValue> &)>;
void TestCommon(JSThread *thread, int numberOfElements, TestCommonCB cb)
{
    auto factory = thread->GetEcmaVM()->GetFactory();
    
    for (int index = 0; index < numberOfElements; index++) {
        std::string keyStr = "key" + std::to_string(index);
        JSHandle<JSTaggedValue> key(factory->NewFromStdString(keyStr));
        JSHandle<JSTaggedValue> value(factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT));
        cb(thread, index, key, value);
    }
}

HWTEST_F_L0(TransitionsDictionaryTest, SetEntry)
{
    int numberOfElements = 8;
    JSHandle<TransitionsDictionary> transDic = TransitionsDictionary::Create(thread, numberOfElements);

    JSHandle<JSTaggedValue> metaData(thread, JSTaggedValue::Undefined());
    TestCommon(thread, numberOfElements,
                [&](JSThread *thread, int index, JSHandle<JSTaggedValue> &key, JSHandle<JSTaggedValue> &value) {
                    transDic->SetEntry(thread, index, key.GetTaggedValue(), value.GetTaggedValue(),
                                       metaData.GetTaggedValue());
                    EXPECT_EQ(transDic->GetKey(thread, index), key.GetTaggedValue());
                });
}

HWTEST_F_L0(TransitionsDictionaryTest, FindEntry)
{
    int numberOfElements = 8;
    JSHandle<TransitionsDictionary> transDic = TransitionsDictionary::Create(thread, numberOfElements);

    JSHandle<JSTaggedValue> metaData(thread, JSTaggedValue::Undefined());
    TestCommon(thread, numberOfElements,
               [&](JSThread *thread, int index, JSHandle<JSTaggedValue> &key, JSHandle<JSTaggedValue> &value) {
                   transDic = TransitionsDictionary::PutIfAbsent(thread, transDic, key, value, metaData);
                   int foundEntry = transDic->FindEntry(thread, key.GetTaggedValue(), metaData.GetTaggedValue());
                   EXPECT_EQ(index + 3, foundEntry);  // 3 : table header size
               });
}

HWTEST_F_L0(TransitionsDictionaryTest, RemoveElement)
{
    int numberOfElements = 8;
    JSHandle<TransitionsDictionary> transDic = TransitionsDictionary::Create(thread, numberOfElements);
    JSHandle<JSTaggedValue> metaData(thread, JSTaggedValue::Undefined());
    TestCommon(thread, numberOfElements,
               [&](JSThread *thread, int index, JSHandle<JSTaggedValue> &key, JSHandle<JSTaggedValue> &value) {
                   transDic = TransitionsDictionary::PutIfAbsent(thread, transDic, key, value, metaData);
               });
    auto factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> key7(factory->NewFromStdString("key7")); // test remove element by "key7"
    int foundEntry = transDic->FindEntry(thread, key7.GetTaggedValue(), metaData.GetTaggedValue());
    EXPECT_EQ(foundEntry, 7 + 3);
    EXPECT_EQ(transDic->EntriesCount(), 8);

    transDic->RemoveElement(thread, foundEntry);
    foundEntry = transDic->FindEntry(thread, key7.GetTaggedValue(), metaData.GetTaggedValue());
    EXPECT_EQ(foundEntry, -1); // -1 : not found entry
    EXPECT_EQ(transDic->EntriesCount(), 7);
}

HWTEST_F_L0(TransitionsDictionaryTest, PutIfAbsent)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    int numberOfElements = 8;
    JSHandle<TransitionsDictionary> transDic = TransitionsDictionary::Create(thread, numberOfElements);
    JSHandle<JSTaggedValue> metaData(thread, JSTaggedValue::Undefined());
    vm->SetEnableForceGC(false);
    for (int index = 0; index < numberOfElements; index++) {
        std::string keyStr = "key" + std::to_string(index);
        JSHandle<JSTaggedValue> key(factory->NewFromStdString(keyStr));
        JSHandle<JSTaggedValue> value(factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT));
        transDic = TransitionsDictionary::PutIfAbsent(thread, transDic, key, value, metaData);
        int foundEntry = transDic->FindEntry(thread, key.GetTaggedValue(), metaData.GetTaggedValue());
        EXPECT_EQ(foundEntry, index + 3);

        JSTaggedValue foundValue = transDic->GetValue(thread, foundEntry);
        JSTaggedValue weakValue = value->CreateAndGetWeakRef();
        EXPECT_EQ(foundValue, weakValue);
    }
    vm->SetEnableForceGC(true);
}

HWTEST_F_L0(TransitionsDictionaryTest, Remove)
{
    int numberOfElements = 64;
    int eleNum = 7;
    JSHandle<TransitionsDictionary> transDic = TransitionsDictionary::Create(thread, numberOfElements);
    JSHandle<JSTaggedValue> metaData(thread, JSTaggedValue::Undefined());
    TestCommon(thread, eleNum,
               [&](JSThread *thread, int index, JSHandle<JSTaggedValue> &key, JSHandle<JSTaggedValue> &value) {
                   transDic = TransitionsDictionary::PutIfAbsent(thread, transDic, key, value, metaData);
               });
    auto factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> key6(factory->NewFromStdString("key6")); // test remove element by "key6"
    EXPECT_EQ(transDic->EntriesCount(), 7);
    EXPECT_EQ(transDic->GetLength(), 195U); // 195 : 3 + 64 * 3

    transDic = TransitionsDictionary::Remove(thread, transDic, key6, metaData.GetTaggedValue());
    EXPECT_EQ(transDic->EntriesCount(), 6); // 6 : 7 - 1
    EXPECT_EQ(transDic->GetLength(), 51U);  // (1 << (32 - Clz((6 + (6 >> 1)) - 1))) * 3 + 3
}

HWTEST_F_L0(TransitionsDictionaryTest, Rehash)
{
    int numberOfElements = 64;
    int eleNum = 7;
    JSHandle<TransitionsDictionary> transDic = TransitionsDictionary::Create(thread, numberOfElements);
    JSHandle<JSTaggedValue> metaData(thread, JSTaggedValue::Undefined());
    TestCommon(thread, eleNum,
               [&](JSThread *thread, int index, JSHandle<JSTaggedValue> &key, JSHandle<JSTaggedValue> &value) {
                   transDic = TransitionsDictionary::PutIfAbsent(thread, transDic, key, value, metaData);
               });
    EXPECT_EQ(transDic->HoleEntriesCount(), 0);

    int lastEntry = 7 + 3;
    transDic->RemoveElement(thread, lastEntry); // remove one element
    EXPECT_EQ(transDic->HoleEntriesCount(), 1);

    transDic->Rehash(thread, *transDic);
    EXPECT_EQ(transDic->HoleEntriesCount(), 0);
}
}  // namespace panda::test