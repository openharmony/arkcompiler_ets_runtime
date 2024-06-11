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

#include "ecmascript/builtins/builtins_weak_set.h"

#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
using BuiltinsWeakSet = ecmascript::builtins::BuiltinsWeakSet;
using JSWeakSet = ecmascript::JSWeakSet;

class BuiltinsWeakSetTest : public BaseTestWithScope<false> {
};

static JSObject *JSObjectTestCreate(JSThread *thread)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> jsFunc = globalEnv->GetObjectFunction();
    JSHandle<JSObject> newObj =
        thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(jsFunc), jsFunc);
    return *newObj;
}

JSWeakSet *CreateBuiltinsWeakSet(JSThread *thread)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetBuiltinsWeakSetFunction());

    // 4 : test case
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 4);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsWeakSet::WeakSetConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsECMAObject());
    JSWeakSet *jsWeakSet = JSWeakSet::Cast(reinterpret_cast<TaggedObject *>(result.GetRawData()));
    return jsWeakSet;
}

HWTEST_F_L0(BuiltinsWeakSetTest, CreateAndGetSize)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetBuiltinsWeakSetFunction());
    JSHandle<JSWeakSet> weakSet(thread, CreateBuiltinsWeakSet(thread));

    JSHandle<TaggedArray> array(factory->NewTaggedArray(5));
    for (int i = 0; i < 5; i++) {
        JSHandle<JSTaggedValue> key(thread, JSObjectTestCreate(thread));
        array->Set(thread, i, key.GetTaggedValue());
    }

    JSHandle<JSArray> values = JSArray::CreateArrayFromList(thread, array);
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(weakSet.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, values.GetTaggedValue());
    ecmaRuntimeCallInfo->SetNewTarget(newTarget.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);

    JSTaggedValue result1 = BuiltinsWeakSet::WeakSetConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<JSWeakSet> weakSetResult(thread,
                                        JSWeakSet::Cast(reinterpret_cast<TaggedObject *>(result1.GetRawData())));
    EXPECT_EQ(weakSetResult->GetSize(), 5);
}

HWTEST_F_L0(BuiltinsWeakSetTest, AddAndHas)
{
    // create jsWeakSet
    JSHandle<JSWeakSet> weakSet(thread, CreateBuiltinsWeakSet(thread));
    JSHandle<JSTaggedValue> key(thread, JSObjectTestCreate(thread));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(weakSet.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, key.GetTaggedValue());

    JSWeakSet *jsWeakSet;
    {
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result1 = BuiltinsWeakSet::Has(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        EXPECT_EQ(result1.GetRawData(), JSTaggedValue::False().GetRawData());

        // test Add()
        JSTaggedValue result2 = BuiltinsWeakSet::Add(ecmaRuntimeCallInfo);
        EXPECT_TRUE(result2.IsECMAObject());
        jsWeakSet = JSWeakSet::Cast(reinterpret_cast<TaggedObject *>(result2.GetRawData()));
        EXPECT_EQ(jsWeakSet->GetSize(), 1);
    }

    // test Has()
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(jsWeakSet));
    ecmaRuntimeCallInfo1->SetCallArg(0, key.GetTaggedValue());
    {
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
        JSTaggedValue result3 = BuiltinsWeakSet::Has(ecmaRuntimeCallInfo1);
        TestHelper::TearDownFrame(thread, prev);

        EXPECT_EQ(result3.GetRawData(), JSTaggedValue::True().GetRawData());
    }
}

void AddCommon(JSThread* thread, JSHandle<JSWeakSet>& weakSet, JSHandle<JSTaggedValue>& key, int i)
{
    auto ecmaRuntimeCallInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6); // 6 means 1 call arg
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(weakSet.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, key.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    // add
    JSTaggedValue result1 = BuiltinsWeakSet::Add(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result1.IsECMAObject());
    JSWeakSet *jsWeakSet = JSWeakSet::Cast(reinterpret_cast<TaggedObject *>(result1.GetRawData()));
    EXPECT_EQ(jsWeakSet->GetSize(), i + 1);
}

void HasAndDeleteCommon(JSThread* thread, JSHandle<JSWeakSet>& weakSet, JSTaggedValue& lastKey)
{
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(weakSet.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, lastKey);

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result2 = BuiltinsWeakSet::Has(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(result2.GetRawData(), JSTaggedValue::True().GetRawData());

    // delete
    JSTaggedValue result3 = BuiltinsWeakSet::Delete(ecmaRuntimeCallInfo1);

    EXPECT_EQ(result3.GetRawData(), JSTaggedValue::True().GetRawData());

    // check deleteKey is deleted
    JSTaggedValue result4 = BuiltinsWeakSet::Has(ecmaRuntimeCallInfo1);

    EXPECT_EQ(result4.GetRawData(), JSTaggedValue::False().GetRawData());
}

HWTEST_F_L0(BuiltinsWeakSetTest, DeleteAndRemove)
{
    // create jsSet
    JSHandle<JSWeakSet> weakSet(thread, CreateBuiltinsWeakSet(thread));

    // add 40 keys
    JSTaggedValue lastKey(JSTaggedValue::Undefined());
    for (int i = 0; i < 40; i++) {
        JSHandle<JSTaggedValue> key(thread, JSObjectTestCreate(thread));

        AddCommon(thread, weakSet, key, i);
        lastKey = key.GetTaggedValue();
    }
    // whether jsWeakSet has delete lastKey

    HasAndDeleteCommon(thread, weakSet, lastKey);
}

HWTEST_F_L0(BuiltinsWeakSetTest, SymbolKey)
{
    // create jsSet
    JSHandle<JSWeakSet> weakSet(thread, CreateBuiltinsWeakSet(thread));

    // add 2 keys
    JSTaggedValue lastKey(JSTaggedValue::Undefined());
    for (int i = 0; i < 2; i++) {
        JSHandle<JSSymbol> symbolKey = thread->GetEcmaVM()->GetFactory()->NewJSSymbol();
        JSHandle<JSTaggedValue> key(symbolKey);

        AddCommon(thread, weakSet, key, i);
        lastKey = key.GetTaggedValue();
    }
    // whether jsWeakSet has delete lastKey

    HasAndDeleteCommon(thread, weakSet, lastKey);
}
}  // namespace panda::test
