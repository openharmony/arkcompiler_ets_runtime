/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {
class WeakLinkedHashMapTest : public BaseTestWithScope<false> {};

HWTEST_F_L0(WeakLinkedHashMapTest, MapCreate)
{
    int numOfElement = 64;
    JSHandle<WeakLinkedHashMap> dict = WeakLinkedHashMap::Create(thread, numOfElement);
    EXPECT_TRUE(*dict != nullptr);
    EXPECT_TRUE(dict.GetTaggedValue().IsWeakLinkedHashMap());
}

HWTEST_F_L0(WeakLinkedHashMapTest, addKeyAndValue)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // mock object needed in test
    int numOfElement = 64;
    JSHandle<WeakLinkedHashMap> dictHandle = WeakLinkedHashMap::Create(thread, numOfElement);
    EXPECT_TRUE(*dictHandle != nullptr);
    JSHandle<JSTaggedValue> objFun = thread->GetGlobalEnv()->GetObjectFunction();

    char keyArray[] = "hello";
    JSHandle<EcmaString> stringKey1 = factory->NewFromASCII(keyArray);
    JSHandle<JSTaggedValue> key1(stringKey1);
    JSHandle<JSTaggedValue> value1(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));

    char key2Array[] = "hello2";
    JSHandle<EcmaString> stringKey2 = factory->NewFromASCII(key2Array);
    JSHandle<JSTaggedValue> key2(stringKey2);
    JSHandle<JSTaggedValue> value2(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));

    char key3Array[] = "hello3";
    JSHandle<EcmaString> stringKey3 = factory->NewFromASCII(key3Array);
    JSHandle<JSTaggedValue> key3(stringKey3);

    // test set()
    dictHandle = WeakLinkedHashMap::SetWeakRef(thread, dictHandle, key1, value1);
    EXPECT_EQ(dictHandle->NumberOfElements(), 1);

    // test find()
    int entry1 = dictHandle->FindElement(thread, key1.GetTaggedValue());
    EXPECT_EQ(value1.GetTaggedValue(), dictHandle->GetValue(thread, entry1));

    dictHandle = WeakLinkedHashMap::SetWeakRef(thread, dictHandle, key2, value2);
    EXPECT_EQ(dictHandle->NumberOfElements(), 2);
    // test remove()
    dictHandle = WeakLinkedHashMap::Delete(thread, dictHandle, key1);
    EXPECT_EQ(-1, dictHandle->FindElement(thread, key1.GetTaggedValue()));
    EXPECT_EQ(dictHandle->NumberOfElements(), 1);

    dictHandle = WeakLinkedHashMap::SetWeakRef(thread, dictHandle, key3, value1);
    int entry2 = dictHandle->FindElement(thread, key3.GetTaggedValue());
    EXPECT_EQ(value1.GetTaggedValue(), dictHandle->GetValue(thread, entry2));
}
}  // namespace panda::test