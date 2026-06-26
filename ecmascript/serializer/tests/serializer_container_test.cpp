/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <cstdint>
#include <ctime>
#include <thread>

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

#include "ecmascript/containers/containers_private.h"
#include "ecmascript/containers/containers_treeset.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/js_api/js_api_tree_set.h"
#include "ecmascript/js_api/js_api_tree_set_iterator.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/containers/tests/containers_test_helper.h"

#include "ecmascript/containers/containers_arraylist.h"
#include "ecmascript/containers/containers_hashmap.h"
#include "ecmascript/containers/containers_hashset.h"
#include "ecmascript/containers/containers_list.h"
#include "ecmascript/containers/containers_linked_list.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/js_api/js_api_hashmap.h"
#include "ecmascript/js_api/js_api_hashset.h"
#include "ecmascript/js_api/js_api_list.h"
#include "ecmascript/js_api/js_api_linked_list.h"

#include "ecmascript/serializer/value_serializer.h"
#include "ecmascript/serializer/base_deserializer.h"
#include "ecmascript/js_api/js_api_arraylist_iterator.h"
#include "ecmascript/js_api/js_api_hashmap_iterator.h"
#include "ecmascript/js_api/js_api_hashset_iterator.h"
#include "ecmascript/js_api/js_api_list_iterator.h"
#include "ecmascript/js_api/js_api_linked_list_iterator.h"
#include "ecmascript/js_iterator.h"

using namespace panda::ecmascript;
using namespace testing::ext;
using namespace panda::ecmascript::containers;

namespace panda::test {

class JSDeserializerContainerTest {
public:
    static const int32_t econdArgv = 2;

    JSDeserializerContainerTest() : thread(nullptr), ecmaVm(nullptr), scope(nullptr) {}

    void Init()
    {
        JSRuntimeOptions options;
        options.SetEnableForceGC(true);
        ecmaVm = JSNApi::CreateEcmaVM(options);
        ecmaVm->SetEnableForceGC(true);
        EXPECT_TRUE(ecmaVm != nullptr) << "Cannot create Runtime";
        thread = ecmaVm->GetJSThread();
        scope = new EcmaHandleScope(thread);
        thread->ManagedCodeBegin();
    }

    void Destroy()
    {
        thread->ManagedCodeEnd();
        delete scope;
        scope = nullptr;
        ecmaVm->SetEnableForceGC(false);
        thread->ClearException();
        JSNApi::DestroyJSVM(ecmaVm);
    }

    // ---- TreeSet callback classes ----

    class JSTreeSetTestClass : public base::BuiltinsBase {
    public:
        static JSTaggedValue TestForEachFunc(EcmaRuntimeCallInfo *argv)
        {
            JSThread *thread = argv->GetThread();
            JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> key = GetCallArg(argv, 1);
            EXPECT_EQ(key.GetTaggedValue(), value.GetTaggedValue());
            JSHandle<JSAPITreeSet> set(GetCallArg(argv, econdArgv)); // 2 means the second arg
            JSAPITreeSet::Delete(thread, set, key);

            JSHandle<JSAPITreeSet> jsTreeSet(GetThis(argv));
            JSAPITreeSet::Add(thread, jsTreeSet, key);
            return JSTaggedValue::Undefined();
        }

        static JSTaggedValue TestCompareFunction(EcmaRuntimeCallInfo *argv)
        {
            JSThread *thread = argv->GetThread();
            JSHandle<JSTaggedValue> valueX = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> valueY = GetCallArg(argv, 1);

            if (valueX->IsString() && valueY->IsString()) {
                auto xHandle = JSHandle<EcmaString>(valueX);
                auto yHandle = JSHandle<EcmaString>(valueY);
                int result = EcmaStringAccessor::Compare(thread->GetEcmaVM(), xHandle, yHandle);
                if (result < 0) {
                    return JSTaggedValue(1);
                }
                if (result == 0) {
                    return JSTaggedValue(0);
                }
                return JSTaggedValue(-1);
            }

            if (valueX->IsNumber() && valueY->IsString()) {
                return JSTaggedValue(1);
            }
            if (valueX->IsString() && valueY->IsNumber()) {
                return JSTaggedValue(-1);
            }

            ComparisonResult res = ComparisonResult::UNDEFINED;
            if (valueX->IsNumber() && valueY->IsNumber()) {
                res = JSTaggedValue::StrictNumberCompare(valueY->GetNumber(), valueX->GetNumber());
            } else {
                res = JSTaggedValue::Compare(thread, valueY, valueX);
            }
            return res == ComparisonResult::GREAT ?
                JSTaggedValue(1) : (res == ComparisonResult::LESS ? JSTaggedValue(-1) : JSTaggedValue(0));
        }
    };

    // ---- ArrayList ForEach callback: (value, index, arrayList) ----

    class JSArrayListTestClass : public base::BuiltinsBase {
    public:
        static JSTaggedValue TestForEachFunc(EcmaRuntimeCallInfo *argv)
        {
            JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
            JSHandle<JSTaggedValue> arrayList = GetCallArg(argv, econdArgv);
            if (!arrayList->IsUndefined() && value->IsNumber()) {
                auto callInfo =
                    TestHelper::CreateEcmaRuntimeCallInfo(argv->GetThread(), JSTaggedValue::Undefined(), 6);
                callInfo->SetFunction(JSTaggedValue::Undefined());
                callInfo->SetThis(arrayList.GetTaggedValue());
                callInfo->SetCallArg(0, index.GetTaggedValue());
                [[maybe_unused]] auto prev = TestHelper::SetupFrame(argv->GetThread(), callInfo);
                JSTaggedValue getResult = ContainersArrayList::Get(callInfo);
                TestHelper::TearDownFrame(argv->GetThread(), prev);
                EXPECT_EQ(getResult, value.GetTaggedValue());
            }
            return JSTaggedValue::True();
        }
    };

    // ---- HashMap ForEach callback: (value, key, map) ----

    class JSHashMapTestClass : public base::BuiltinsBase {
    public:
        static JSTaggedValue TestForEachFunc(EcmaRuntimeCallInfo *argv)
        {
            JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> key = GetCallArg(argv, 1);
            EXPECT_EQ(value.GetTaggedValue(), key.GetTaggedValue());
            return JSTaggedValue::True();
        }
    };

    // ---- HashSet ForEach callback: (value, key, set) ----

    class JSHashSetTestClass : public base::BuiltinsBase {
    public:
        static JSTaggedValue TestForEachFunc(EcmaRuntimeCallInfo *argv)
        {
            JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> key = GetCallArg(argv, 1);
            EXPECT_EQ(value.GetTaggedValue(), key.GetTaggedValue());
            return JSTaggedValue::True();
        }
    };

    // ---- List ForEach callback: (value, index, list) ----

    class JSListTestClass : public base::BuiltinsBase {
    public:
        static JSTaggedValue TestForEachFunc(EcmaRuntimeCallInfo *argv)
        {
            JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
            JSHandle<JSTaggedValue> list = GetCallArg(argv, econdArgv);
            if (!list->IsUndefined() && index->IsNumber() && value->IsNumber()) {
                auto callInfo =
                    TestHelper::CreateEcmaRuntimeCallInfo(argv->GetThread(), JSTaggedValue::Undefined(), 6);
                callInfo->SetFunction(JSTaggedValue::Undefined());
                callInfo->SetThis(list.GetTaggedValue());
                callInfo->SetCallArg(0, index.GetTaggedValue());
                [[maybe_unused]] auto prev = TestHelper::SetupFrame(argv->GetThread(), callInfo);
                JSTaggedValue getResult = ContainersList::Get(callInfo);
                TestHelper::TearDownFrame(argv->GetThread(), prev);
                EXPECT_EQ(getResult, value.GetTaggedValue());
            }
            return JSTaggedValue::True();
        }
    };

    // ---- LinkedList ForEach callback: (value, index, list) ----

    class JSLinkedListTestClass : public base::BuiltinsBase {
    public:
        static JSTaggedValue TestForEachFunc(EcmaRuntimeCallInfo *argv)
        {
            JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
            JSHandle<JSTaggedValue> list = GetCallArg(argv, econdArgv);
            if (!list->IsUndefined() && index->IsNumber() && value->IsNumber()) {
                auto callInfo =
                    TestHelper::CreateEcmaRuntimeCallInfo(argv->GetThread(), JSTaggedValue::Undefined(), 6);
                callInfo->SetFunction(JSTaggedValue::Undefined());
                callInfo->SetThis(list.GetTaggedValue());
                callInfo->SetCallArg(0, index.GetTaggedValue());
                [[maybe_unused]] auto prev = TestHelper::SetupFrame(argv->GetThread(), callInfo);
                JSTaggedValue getResult = ContainersLinkedList::Get(callInfo);
                TestHelper::TearDownFrame(argv->GetThread(), prev);
                EXPECT_EQ(getResult, value.GetTaggedValue());
            }
            return JSTaggedValue::True();
        }
    };

    // ---- Constructor helpers ----

    JSTaggedValue InitializeTreeSetConstructor()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();

        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::TreeSet)));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        return result;
    }

    JSHandle<JSAPITreeSet> CreateJSAPITreeSet(JSTaggedValue compare = JSTaggedValue::Undefined())
    {
        JSHandle<JSTaggedValue> compareHandle(thread, compare);
        JSHandle<JSFunction> newTarget(thread, InitializeTreeSetConstructor());
        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);

        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());
        objCallInfo->SetCallArg(0, compareHandle.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersTreeSet::TreeSetConstructor(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPITreeSet> set(thread, result);
        return set;
    }

    // ---- TreeSet sub-tests ----

    void JSTreeSetNextTest(const JSHandle<JSAPITreeSet>& tset)
    {
        constexpr uint32_t nodeNumber = 8;
        auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo1->SetFunction(JSTaggedValue::Undefined());
        callInfo1->SetThis(tset.GetTaggedValue());

        [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1);
        JSHandle<JSTaggedValue> iterValues(thread, ContainersTreeSet::Values(callInfo1));
        TestHelper::TearDownFrame(thread, prev1);
        EXPECT_TRUE(iterValues->IsJSAPITreeSetIterator());

        {
            JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(iterValues.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            result.Update(JSAPITreeSetIterator::Next(callInfo));
            TestHelper::TearDownFrame(thread, prev);
            bool isDone = false;
            for (int i = 0; !isDone; i++) {
                EXPECT_EQ(i, JSIterator::IteratorValue(thread, result)->GetInt());
                // next
                auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
                callInfo->SetFunction(JSTaggedValue::Undefined());
                callInfo->SetThis(iterValues.GetTaggedValue());

                [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
                result.Update(JSAPITreeSetIterator::Next(callInfo));
                TestHelper::TearDownFrame(thread, prev);
                isDone = JSIterator::IteratorComplete(thread, result);
                EXPECT_EQ(i + 1 == nodeNumber, isDone);
            }
        }
    }

    void JSTreeSetEntriesTest(const JSHandle<JSAPITreeSet>& tset)
    {
        auto callInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo2->SetFunction(JSTaggedValue::Undefined());
        callInfo2->SetThis(tset.GetTaggedValue());

        [[maybe_unused]] auto prev2 = TestHelper::SetupFrame(thread, callInfo2);
        JSHandle<JSTaggedValue> iter(thread, ContainersTreeSet::Entries(callInfo2));
        TestHelper::TearDownFrame(thread, prev2);
        EXPECT_TRUE(iter->IsJSAPITreeSetIterator());
    }

    void JSTreeSetForeachTest(JSHandle<JSAPITreeSet>& tset)
    {
        constexpr uint32_t nodeNumber = 8;
        constexpr uint32_t step = 2;
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSAPITreeSet> dset = CreateJSAPITreeSet();
        {
            JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
            JSHandle<JSFunction> func =
                factory->NewJSFunction(env, reinterpret_cast<void *>(JSTreeSetTestClass::TestForEachFunc));

            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(tset.GetTaggedValue());
            callInfo->SetCallArg(0, func.GetTaggedValue());
            callInfo->SetCallArg(1, dset.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            ContainersTreeSet::ForEach(callInfo);
            TestHelper::TearDownFrame(thread, prev);
        }

        EXPECT_EQ(dset->GetSize(thread), nodeNumber / step);
        EXPECT_EQ(tset->GetSize(thread), nodeNumber / step);
        for (int i = 0; i < nodeNumber; i += step) {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(dset.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            JSTaggedValue result = ContainersTreeSet::Has(callInfo);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_TRUE(result.IsTrue());
        }

        // test add string
        JSTreeSetStringTest(tset, dset);
    }

    void JSTreeSetStringTest(JSHandle<JSAPITreeSet>& tset, JSHandle<JSAPITreeSet>& dset)
    {
        constexpr uint32_t nodeNumber = 8;
        constexpr uint32_t step = 2;
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
        std::string myKey("mykey");
        for (int i = 0; i < nodeNumber; i++) {
            std::string ikey = myKey + std::to_string(i);
            key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(tset.GetTaggedValue());
            callInfo->SetCallArg(0, key.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            JSTaggedValue result = ContainersTreeSet::Add(callInfo);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_TRUE(result.IsTrue());
            EXPECT_EQ(tset->GetSize(thread), nodeNumber / step + i + 1);
        }
        EXPECT_EQ(tset->GetSize(thread), nodeNumber / step + nodeNumber);
        {
            JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
            JSHandle<JSFunction> func =
                factory->NewJSFunction(env, reinterpret_cast<void *>(JSTreeSetTestClass::TestForEachFunc));

            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(tset.GetTaggedValue());
            callInfo->SetCallArg(0, func.GetTaggedValue());
            callInfo->SetCallArg(1, dset.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            ContainersTreeSet::ForEach(callInfo);
            TestHelper::TearDownFrame(thread, prev);
        }
        EXPECT_EQ(dset->GetSize(thread), nodeNumber + step);
        EXPECT_EQ(tset->GetSize(thread), nodeNumber - step);
        for (int i = 0; i < nodeNumber; i += step) {
            std::string ikey = myKey + std::to_string(i);
            key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(dset.GetTaggedValue());
            callInfo->SetCallArg(0, key.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            JSTaggedValue result = ContainersTreeSet::Has(callInfo);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_TRUE(result.IsTrue());
        }
    }

    // ---- TreeSet deserializer test ----

    void JSTreeSetTest(SerializeData* data)
    {
        Init();
        constexpr uint32_t nodeNumber = 8;
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize JSAPITreeSet fail";
        EXPECT_TRUE(res->IsJSAPITreeSet()) << "[NotJSAPITreeSet] Deserialize JSAPITreeSet fail";
        JSHandle<JSAPITreeSet> tset = JSHandle<JSAPITreeSet>::Cast(res);
        EXPECT_TRUE(tset->GetSize(thread) == nodeNumber) << "the treeset size Not equal";

        for (int i = 0; i < nodeNumber; i++) {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(tset.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            JSTaggedValue result = ContainersTreeSet::Has(callInfo);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_TRUE(result.IsTrue());
        }

        // test values next done
        JSTreeSetNextTest(tset);

        // test entries
        JSTreeSetEntriesTest(tset);

        // test foreach function with TestForEachFunc;
        JSTreeSetForeachTest(tset);

        Destroy();
    }

    // ---- ArrayList sub-tests ----

    void JSArrayListIteratorTest(const JSHandle<JSAPIArrayList>& list, uint32_t nodeNumber)
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSHandle<JSTaggedValue> iter(thread, ContainersArrayList::GetIteratorObj(callInfo));
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(iter->IsJSAPIArrayListIterator());

        JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
        for (uint32_t i = 0; i < nodeNumber; i++) {
            auto nextCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
            nextCallInfo->SetFunction(JSTaggedValue::Undefined());
            nextCallInfo->SetThis(iter.GetTaggedValue());

            [[maybe_unused]] auto nextPrev = TestHelper::SetupFrame(thread, nextCallInfo);
            result.Update(JSAPIArrayListIterator::Next(nextCallInfo));
            TestHelper::TearDownFrame(thread, nextPrev);
            JSHandle<JSTaggedValue> iterValue = JSIterator::IteratorValue(thread, result);
            EXPECT_EQ(iterValue.GetTaggedValue(), JSTaggedValue(i));
        }
    }

    void JSArrayListForeachTest(const JSHandle<JSAPIArrayList>& list)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> func =
            factory->NewJSFunction(env, reinterpret_cast<void *>(JSArrayListTestClass::TestForEachFunc));

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());
        callInfo->SetCallArg(0, func.GetTaggedValue());
        callInfo->SetCallArg(1, list.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersArrayList::ForEach(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }

    // ---- ArrayList deserializer test ----

    void JSArrayListTest(SerializeData* data)
    {
        Init();
        constexpr uint32_t nodeNumber = 10;
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize JSAPIArrayList fail";
        EXPECT_TRUE(res->IsJSAPIArrayList()) << "[NotJSAPIArrayList] Deserialize JSAPIArrayList fail";
        JSHandle<JSAPIArrayList> list = JSHandle<JSAPIArrayList>::Cast(res);
        EXPECT_TRUE(list->GetSize(thread) == nodeNumber) << "the arraylist size Not equal";

        for (uint32_t i = 0; i < nodeNumber; i++) {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(list.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(i));
            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            JSTaggedValue result = ContainersArrayList::Get(callInfo);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_EQ(result, JSTaggedValue(i));
        }

        JSArrayListIteratorTest(list, nodeNumber);
        JSArrayListForeachTest(list);

        Destroy();
    }

    // ---- HashMap sub-tests ----

    void JSHashMapValuesIteratorTest(const JSHandle<JSAPIHashMap>& map, uint32_t nodeNumber)
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(map.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSHandle<JSTaggedValue> iterValues(thread, ContainersHashMap::Values(callInfo));
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(iterValues->IsJSAPIHashMapIterator());

        JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
        for (uint32_t i = 0; i < nodeNumber; i++) {
            auto nextCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
            nextCallInfo->SetFunction(JSTaggedValue::Undefined());
            nextCallInfo->SetThis(iterValues.GetTaggedValue());

            [[maybe_unused]] auto nextPrev = TestHelper::SetupFrame(thread, nextCallInfo);
            result.Update(JSAPIHashMapIterator::Next(nextCallInfo));
            TestHelper::TearDownFrame(thread, nextPrev);
            JSHandle<JSTaggedValue> iterValue = JSIterator::IteratorValue(thread, result);
            JSTaggedValue valueFlag = JSAPIHashMap::HasValue(thread, map, iterValue);
            EXPECT_EQ(valueFlag, JSTaggedValue::True());
        }
    }

    void JSHashMapEntriesTest(const JSHandle<JSAPIHashMap>& map)
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(map.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSHandle<JSTaggedValue> iter(thread, ContainersHashMap::Entries(callInfo));
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(iter->IsJSAPIHashMapIterator());
    }

    void JSHashMapForeachTest(const JSHandle<JSAPIHashMap>& map)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> func =
            factory->NewJSFunction(env, reinterpret_cast<void *>(JSHashMapTestClass::TestForEachFunc));

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(map.GetTaggedValue());
        callInfo->SetCallArg(0, func.GetTaggedValue());
        callInfo->SetCallArg(1, map.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersHashMap::ForEach(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }

    // ---- HashMap deserializer test ----

    void JSHashMapTest(SerializeData* data)
    {
        Init();
        constexpr uint32_t nodeNumber = 8;
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize JSAPIHashMap fail";
        EXPECT_TRUE(res->IsJSAPIHashMap()) << "[NotJSAPIHashMap] Deserialize JSAPIHashMap fail";
        JSHandle<JSAPIHashMap> map = JSHandle<JSAPIHashMap>::Cast(res);
        EXPECT_TRUE(map->GetSize() == nodeNumber) << "the hashmap size Not equal";
        for (int i = 0; i < nodeNumber; i++) {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(map.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            JSTaggedValue hasResult = ContainersHashMap::HasKey(callInfo);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_EQ(hasResult, JSTaggedValue::True());

            auto callInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo2->SetFunction(JSTaggedValue::Undefined());
            callInfo2->SetThis(map.GetTaggedValue());
            callInfo2->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev2 = TestHelper::SetupFrame(thread, callInfo2);
            JSTaggedValue getResult = ContainersHashMap::Get(callInfo2);
            TestHelper::TearDownFrame(thread, prev2);
            EXPECT_EQ(getResult, JSTaggedValue(i));
        }

        JSHashMapValuesIteratorTest(map, nodeNumber);
        JSHashMapEntriesTest(map);
        JSHashMapForeachTest(map);

        Destroy();
    }

    // ---- HashSet sub-tests ----

    void JSHashSetValuesIteratorTest(const JSHandle<JSAPIHashSet>& set, uint32_t nodeNumber)
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(set.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSHandle<JSTaggedValue> iterValues(thread, ContainersHashSet::Values(callInfo));
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(iterValues->IsJSAPIHashSetIterator());

        JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
        for (uint32_t i = 0; i < nodeNumber; i++) {
            auto nextCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
            nextCallInfo->SetFunction(JSTaggedValue::Undefined());
            nextCallInfo->SetThis(iterValues.GetTaggedValue());

            [[maybe_unused]] auto nextPrev = TestHelper::SetupFrame(thread, nextCallInfo);
            result.Update(JSAPIHashSetIterator::Next(nextCallInfo));
            TestHelper::TearDownFrame(thread, nextPrev);
            EXPECT_TRUE(CheckHashSetHasValue(set, JSIterator::IteratorValue(thread, result)));
        }
    }

    bool CheckHashSetHasValue(const JSHandle<JSAPIHashSet>& set, const JSHandle<JSTaggedValue>& value)
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(set.GetTaggedValue());
        callInfo->SetCallArg(0, value.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue hasResult = ContainersHashSet::Has(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        return hasResult.IsTrue();
    }

    void JSHashSetEntriesTest(const JSHandle<JSAPIHashSet>& set)
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(set.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSHandle<JSTaggedValue> iter(thread, ContainersHashSet::Entries(callInfo));
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(iter->IsJSAPIHashSetIterator());
    }

    void JSHashSetForeachTest(const JSHandle<JSAPIHashSet>& set)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> func =
            factory->NewJSFunction(env, reinterpret_cast<void *>(JSHashSetTestClass::TestForEachFunc));

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(set.GetTaggedValue());
        callInfo->SetCallArg(0, func.GetTaggedValue());
        callInfo->SetCallArg(1, set.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersHashSet::ForEach(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }

    // ---- HashSet deserializer test ----

    void JSHashSetTest(SerializeData* data)
    {
        Init();
        constexpr uint32_t nodeNumber = 8;
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize JSAPIHashSet fail";
        EXPECT_TRUE(res->IsJSAPIHashSet()) << "[NotJSAPIHashSet] Deserialize JSAPIHashSet fail";
        JSHandle<JSAPIHashSet> set = JSHandle<JSAPIHashSet>::Cast(res);
        EXPECT_TRUE(set->GetSize() == nodeNumber) << "the hashset size Not equal";

        for (int i = 0; i < nodeNumber; i++) {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(set.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            JSTaggedValue result = ContainersHashSet::Has(callInfo);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_EQ(result, JSTaggedValue::True());
        }

        JSHashSetValuesIteratorTest(set, nodeNumber);
        JSHashSetEntriesTest(set);
        JSHashSetForeachTest(set);

        Destroy();
    }

    // ---- List sub-tests ----

    void JSListIteratorTest(const JSHandle<JSAPIList>& list, uint32_t nodeNumber)
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSHandle<JSTaggedValue> iter(thread, ContainersList::GetIteratorObj(callInfo));
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(iter->IsJSAPIListIterator());

        JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
        for (uint32_t i = 0; i < nodeNumber; i++) {
            auto nextCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
            nextCallInfo->SetFunction(JSTaggedValue::Undefined());
            nextCallInfo->SetThis(iter.GetTaggedValue());
            [[maybe_unused]] auto nextPrev = TestHelper::SetupFrame(thread, nextCallInfo);
            result.Update(JSAPIListIterator::Next(nextCallInfo));
            TestHelper::TearDownFrame(thread, nextPrev);
            JSHandle<JSTaggedValue> iterValue = JSIterator::IteratorValue(thread, result);
            EXPECT_EQ(iterValue.GetTaggedValue(), JSTaggedValue(i));
        }
    }

    void JSListForeachTest(const JSHandle<JSAPIList>& list)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> func =
            factory->NewJSFunction(env, reinterpret_cast<void *>(JSListTestClass::TestForEachFunc));

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());
        callInfo->SetCallArg(0, func.GetTaggedValue());
        callInfo->SetCallArg(1, list.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersList::ForEach(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }

    // ---- List deserializer test ----

    void JSListTest(SerializeData* data)
    {
        Init();
        constexpr uint32_t nodeNumber = 8;
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize JSAPIList fail";
        EXPECT_TRUE(res->IsJSAPIList()) << "[NotJSAPIList] Deserialize JSAPIList fail";
        JSHandle<JSAPIList> list = JSHandle<JSAPIList>::Cast(res);
        EXPECT_TRUE(list->Length(thread) == nodeNumber) << "the list size Not equal";
        for (int i = 0; i < nodeNumber; i++) {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(list.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(i));
            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            JSTaggedValue result = ContainersList::Get(callInfo);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_EQ(result, JSTaggedValue(i));
        }

        JSListIteratorTest(list, nodeNumber);
        JSListForeachTest(list);

        Destroy();
    }

    // ---- LinkedList sub-tests ----

    void JSLinkedListIteratorTest(const JSHandle<JSAPILinkedList>& list, uint32_t nodeNumber)
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSHandle<JSTaggedValue> iter(thread, ContainersLinkedList::GetIteratorObj(callInfo));
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(iter->IsJSAPILinkedListIterator());

        JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
        for (uint32_t i = 0; i < nodeNumber; i++) {
            auto nextCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
            nextCallInfo->SetFunction(JSTaggedValue::Undefined());
            nextCallInfo->SetThis(iter.GetTaggedValue());

            [[maybe_unused]] auto nextPrev = TestHelper::SetupFrame(thread, nextCallInfo);
            result.Update(JSAPILinkedListIterator::Next(nextCallInfo));
            TestHelper::TearDownFrame(thread, nextPrev);
            JSHandle<JSTaggedValue> iterValue = JSIterator::IteratorValue(thread, result);
            EXPECT_EQ(iterValue.GetTaggedValue(), JSTaggedValue(i));
        }
    }

    void JSLinkedListForeachTest(const JSHandle<JSAPILinkedList>& list)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> func =
            factory->NewJSFunction(env, reinterpret_cast<void *>(JSLinkedListTestClass::TestForEachFunc));

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());
        callInfo->SetCallArg(0, func.GetTaggedValue());
        callInfo->SetCallArg(1, list.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersLinkedList::ForEach(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }

    // ---- LinkedList deserializer test ----

    void JSLinkedListTest(SerializeData* data)
    {
        Init();
        constexpr uint32_t nodeNumber = 8;
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize JSAPILinkedList fail";
        EXPECT_TRUE(res->IsJSAPILinkedList()) << "[NotJSAPILinkedList] Deserialize JSAPILinkedList fail";
        JSHandle<JSAPILinkedList> list = JSHandle<JSAPILinkedList>::Cast(res);
        EXPECT_TRUE(list->Length(thread) == nodeNumber) << "the linkedlist size Not equal";
        for (int i = 0; i < nodeNumber; i++) {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(list.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(i));
            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            JSTaggedValue result = ContainersLinkedList::Get(callInfo);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_EQ(result, JSTaggedValue(i));
        }

        JSLinkedListIteratorTest(list, nodeNumber);
        JSLinkedListForeachTest(list);

        Destroy();
    }

    JSThread *thread {nullptr};
    EcmaVM *ecmaVm {nullptr};
    EcmaHandleScope *scope {nullptr};
};

class JSSerializerContainerTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(ecmaVm, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(ecmaVm, scope);
    }

    JSThread *thread {nullptr};
    EcmaVM *ecmaVm {nullptr};
    EcmaHandleScope *scope {nullptr};

protected:
    JSTaggedValue InitializeTreeSetConstructor()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();

        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::TreeSet)));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        return result;
    }

    JSHandle<JSAPITreeSet> CreateJSAPITreeSet(JSTaggedValue compare = JSTaggedValue::Undefined())
    {
        JSHandle<JSTaggedValue> compareHandle(thread, compare);
        JSHandle<JSFunction> newTarget(thread, InitializeTreeSetConstructor());

        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());
        objCallInfo->SetCallArg(0, compareHandle.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersTreeSet::TreeSetConstructor(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPITreeSet> set(thread, result);
        return set;
    }

    JSTaggedValue InitializeArrayListConstructor()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();

        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::ArrayList)));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        return result;
    }

    JSHandle<JSAPIArrayList> CreateJSAPIArrayList()
    {
        JSHandle<JSFunction> newTarget(thread, InitializeArrayListConstructor());
        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersArrayList::ArrayListConstructor(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPIArrayList> arrayList(thread, result);
        return arrayList;
    }

    JSTaggedValue InitializeHashMapConstructor()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();

        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::HashMap)));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        return result;
    }

    JSHandle<JSAPIHashMap> CreateJSAPIHashMap()
    {
        JSHandle<JSFunction> newTarget(thread, InitializeHashMapConstructor());
        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersHashMap::HashMapConstructor(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPIHashMap> map(thread, result);
        return map;
    }

    JSTaggedValue InitializeHashSetConstructor()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();

        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::HashSet)));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        return result;
    }

    JSHandle<JSAPIHashSet> CreateJSAPIHashSet()
    {
        JSHandle<JSFunction> newTarget(thread, InitializeHashSetConstructor());
        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersHashSet::HashSetConstructor(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPIHashSet> set(thread, result);
        return set;
    }

    JSTaggedValue InitializeListConstructor()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();

        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::List)));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        return result;
    }

    JSHandle<JSAPIList> CreateJSAPIList(JSTaggedValue compare = JSTaggedValue::Undefined())
    {
        JSHandle<JSTaggedValue> compareHandle(thread, compare);
        JSHandle<JSFunction> newTarget(thread, InitializeListConstructor());
        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());
        objCallInfo->SetCallArg(0, compareHandle.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersList::ListConstructor(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPIList> list(thread, result);
        return list;
    }

    JSTaggedValue InitializeLinkedListConstructor()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();

        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::LinkedList)));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        return result;
    }

    JSHandle<JSAPILinkedList> CreateJSAPILinkedList(JSTaggedValue compare = JSTaggedValue::Undefined())
    {
        JSHandle<JSTaggedValue> compareHandle(thread, compare);
        JSHandle<JSFunction> newTarget(thread, InitializeLinkedListConstructor());
        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());
        objCallInfo->SetCallArg(0, compareHandle.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersLinkedList::LinkedListConstructor(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPILinkedList> list(thread, result);
        return list;
    }
};

HWTEST_F_L0(JSSerializerContainerTest, SerializeJSTreeSet)
{
    constexpr int nodeNumber = 8;
    JSHandle<JSAPITreeSet> tset = CreateJSAPITreeSet();
    for (int i = 0; i < nodeNumber; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeSet::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tset->GetSize(thread), i + 1);
    }

    auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    callInfo1->SetFunction(JSTaggedValue::Undefined());
    callInfo1->SetThis(tset.GetTaggedValue());

    [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1);
    JSHandle<JSTaggedValue> iterValues(thread, ContainersTreeSet::Values(callInfo1));
    TestHelper::TearDownFrame(thread, prev1);
    EXPECT_TRUE(iterValues->IsJSAPITreeSetIterator());
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    bool isDone = false;
    for (int i = 0; !isDone; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(iterValues.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        result.Update(JSAPITreeSetIterator::Next(callInfo));
        TestHelper::TearDownFrame(thread, prev);
        isDone = JSIterator::IteratorComplete(thread, result);
        EXPECT_EQ(i == nodeNumber, isDone);
    }
    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(tset),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize JSTreeSet fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerContainerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerContainerTest::JSTreeSetTest, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerContainerTest, SerializeJSArrayList)
{
    constexpr int nodeNumber = 10;
    JSHandle<JSAPIArrayList> list = CreateJSAPIArrayList();
    for (int i = 0; i < nodeNumber; i++) {

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersArrayList::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(list->GetSize(thread), i + 1);
    }
    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(list),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize JSArrayList fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerContainerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerContainerTest::JSArrayListTest, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerContainerTest, SerializeJSHashMap)
{
    constexpr int nodeNumber = 8;
    JSHandle<JSAPIHashMap> map = CreateJSAPIHashMap();
    for (int i = 0; i < nodeNumber; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(map.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPIHashMap());
        EXPECT_EQ(JSAPIHashMap::Cast(result.GetTaggedObject())->GetSize(), i + 1);
    }
    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(map),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize JSHashMap fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerContainerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerContainerTest::JSHashMapTest, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerContainerTest, SerializeJSHashSet)
{
    constexpr int nodeNumber = 8;
    JSHandle<JSAPIHashSet> set = CreateJSAPIHashSet();
    for (int i = 0; i < nodeNumber; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(set.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashSet::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(set->GetSize(), i + 1);
    }
    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(set),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize JSHashSet fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerContainerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerContainerTest::JSHashSetTest, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerContainerTest, SerializeJSList)
{
    constexpr int nodeNumber = 8;
    JSHandle<JSAPIList> list = CreateJSAPIList();
    for (int i = 0; i < nodeNumber; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersList::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(list->Length(thread), static_cast<uint32_t>(i + 1));
    }
    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(list),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize JSList fail";

    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerContainerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerContainerTest::JSListTest, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerContainerTest, SerializeJSLinkedList)
{
    constexpr int nodeNumber = 8;
    JSHandle<JSAPILinkedList> list = CreateJSAPILinkedList();
    for (int i = 0; i < nodeNumber; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersLinkedList::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(list->Length(thread), static_cast<uint32_t>(i + 1));
    }
    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(list),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize JSLinkedList fail";

    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerContainerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerContainerTest::JSLinkedListTest, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};
}  // namespace panda::test
