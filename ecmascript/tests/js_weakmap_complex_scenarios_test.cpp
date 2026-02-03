/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <vector>

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/js_weak_ref.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {

class JSWeakMapComplexScenariosTest : public BaseTestWithScope<false> {
protected:
    JSHandle<JSWeakMap> CreateWeakMap()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSTaggedValue> constructor = globalEnv->GetBuiltinsWeakMapFunction();
        JSHandle<JSWeakMap> weakMap =
            JSHandle<JSWeakMap>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor),
                                                                        constructor));
        JSHandle<WeakLinkedHashMap> linkedMap = WeakLinkedHashMap::Create(thread);
        weakMap->SetWeakLinkedMap(thread, linkedMap);
        return weakMap;
    }

    JSHandle<JSWeakRef> CreateWeakRef(const JSHandle<JSTaggedValue> &target)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSTaggedValue> constructor = globalEnv->GetBuiltinsWeakRefFunction();
        JSHandle<JSWeakRef> weakRef =
            JSHandle<JSWeakRef>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor),
                                                                        constructor));
        weakRef->SetToWeak(thread, target.GetTaggedValue());
        return weakRef;
    }

    JSHandle<JSObject> CreateObject()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSTaggedValue> constructor = globalEnv->GetObjectFunction();
        return factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor);
    }

    void SetProperty(const JSHandle<JSObject> &obj, const CString &key, const JSHandle<JSTaggedValue> &value)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<EcmaString> keyStr = factory->NewFromASCII(key);
        JSTaggedValue keyNameValue = keyStr.GetTaggedValue();
        JSHandle<JSTaggedValue> keyName(thread, keyNameValue);
        JSObject::SetProperty(thread, obj, keyName, value);
    }

    bool IsCollected(const JSHandle<JSWeakRef> &weakRef)
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        JSTaggedValue result = JSWeakRef::WeakRefDeref(thread, weakRef);
        return result.IsUndefined();
    }

public:
    void SetUp() override
    {
        JSRuntimeOptions options;
        options.SetEnableForceGC(false);
        instance = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        thread->ManagedCodeBegin();
        scope = new EcmaHandleScope(thread);
        JSNApi::InitHybridVMEnv(thread->GetEcmaVM());
    }
};

// Test 1: Single WeakMap, V references K
// Expected: K and V should be collected
HWTEST_F_L0(JSWeakMapComplexScenariosTest, Test1_SingleWeakMapVReferencesK)
{
    auto weakMap = CreateWeakMap();

    JSMutableHandle<JSWeakRef> kWeakRef(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSWeakRef> vWeakRef(thread, JSTaggedValue::Undefined());

    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        auto k = CreateObject();
        auto v = CreateObject();

        // V references K
        SetProperty(v, "ref", JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()));

        JSWeakMap::Set(thread, weakMap, JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v.GetTaggedValue()));

        JSHandle<JSWeakRef> kWeakRefTemp = CreateWeakRef(JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()));
        JSHandle<JSWeakRef> vWeakRefTemp = CreateWeakRef(JSHandle<JSTaggedValue>(thread, v.GetTaggedValue()));

        kWeakRef.Update(kWeakRefTemp);
        vWeakRef.Update(vWeakRefTemp);
    }  // k and v go out of scope

    thread->GetEcmaVM()->ClearKeptObjects(thread);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);

    EXPECT_TRUE(IsCollected(kWeakRef));
    EXPECT_TRUE(IsCollected(vWeakRef));
}

// Test 2: Single WeakMap, cyclic references V1->K2, V2->K3, V3->K1
// Expected: All K and V should be collected
HWTEST_F_L0(JSWeakMapComplexScenariosTest, Test2_CyclicReferences)
{
    auto weakMap = CreateWeakMap();

    std::vector<JSMutableHandle<JSWeakRef>> weakRefs;
    for (int i = 0; i < 6; i++) {
        weakRefs.push_back(JSMutableHandle<JSWeakRef>(thread, JSTaggedValue::Undefined()));
    }

    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        auto k1 = CreateObject();
        auto k2 = CreateObject();
        auto k3 = CreateObject();
        auto v1 = CreateObject();
        auto v2 = CreateObject();
        auto v3 = CreateObject();

        // V1->K2, V2->K3, V3->K1
        SetProperty(v1, "ref", JSHandle<JSTaggedValue>(thread, k2.GetTaggedValue()));
        SetProperty(v2, "ref", JSHandle<JSTaggedValue>(thread, k3.GetTaggedValue()));
        SetProperty(v3, "ref", JSHandle<JSTaggedValue>(thread, k1.GetTaggedValue()));

        JSWeakMap::Set(thread, weakMap, JSHandle<JSTaggedValue>(thread, k1.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v1.GetTaggedValue()));
        JSWeakMap::Set(thread, weakMap, JSHandle<JSTaggedValue>(thread, k2.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v2.GetTaggedValue()));
        JSWeakMap::Set(thread, weakMap, JSHandle<JSTaggedValue>(thread, k3.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v3.GetTaggedValue()));

        weakRefs[0].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, k1.GetTaggedValue())));
        weakRefs[1].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, k2.GetTaggedValue())));
        weakRefs[2].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, k3.GetTaggedValue())));
        weakRefs[3].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v1.GetTaggedValue())));
        weakRefs[4].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v2.GetTaggedValue())));
        weakRefs[5].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v3.GetTaggedValue())));
    }  // all objects go out of scope

    thread->GetEcmaVM()->ClearKeptObjects(thread);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);

    EXPECT_TRUE(IsCollected(weakRefs[0]));  // k1
    EXPECT_TRUE(IsCollected(weakRefs[1]));  // k2
    EXPECT_TRUE(IsCollected(weakRefs[2]));  // k3
    EXPECT_TRUE(IsCollected(weakRefs[3]));  // v1
    EXPECT_TRUE(IsCollected(weakRefs[4]));  // v2
    EXPECT_TRUE(IsCollected(weakRefs[5]));  // v3
}

// Test 3: Two WeakMaps, cross-map cycle
// Expected: All K and V should be collected
HWTEST_F_L0(JSWeakMapComplexScenariosTest, Test3_TwoWeakMapsCrossMapCycle)
{
    auto weakMap1 = CreateWeakMap();
    auto weakMap2 = CreateWeakMap();

    std::vector<JSMutableHandle<JSWeakRef>> weakRefs;
    for (int i = 0; i < 4; i++) {
        weakRefs.push_back(JSMutableHandle<JSWeakRef>(thread, JSTaggedValue::Undefined()));
    }

    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        auto k1 = CreateObject();
        auto k2 = CreateObject();
        auto v1 = CreateObject();
        auto v2 = CreateObject();

        // V1->K2 (in different map), V2->K1 (in different map)
        SetProperty(v1, "ref", JSHandle<JSTaggedValue>(thread, k2.GetTaggedValue()));
        SetProperty(v2, "ref", JSHandle<JSTaggedValue>(thread, k1.GetTaggedValue()));

        JSWeakMap::Set(thread, weakMap1, JSHandle<JSTaggedValue>(thread, k1.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v1.GetTaggedValue()));
        JSWeakMap::Set(thread, weakMap2, JSHandle<JSTaggedValue>(thread, k2.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v2.GetTaggedValue()));

        weakRefs[0].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, k1.GetTaggedValue())));
        weakRefs[1].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, k2.GetTaggedValue())));
        weakRefs[2].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v1.GetTaggedValue())));
        weakRefs[3].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v2.GetTaggedValue())));
    }  // all objects go out of scope

    thread->GetEcmaVM()->ClearKeptObjects(thread);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);

    EXPECT_TRUE(IsCollected(weakRefs[0]));  // k1
    EXPECT_TRUE(IsCollected(weakRefs[1]));  // k2
    EXPECT_TRUE(IsCollected(weakRefs[2]));  // v1
    EXPECT_TRUE(IsCollected(weakRefs[3]));  // v2
}

// Test 4: Three WeakMaps, multi-map cycle
// Map1: {K1: V1}, V1->K2
// Map2: {K2: V2}, V2->K3
// Map3: {K3: V3}, V3->K1
// Expected: All K and V should be collected
HWTEST_F_L0(JSWeakMapComplexScenariosTest, Test4_ThreeWeakMapsMultiMapCycle)
{
    auto weakMap1 = CreateWeakMap();
    auto weakMap2 = CreateWeakMap();
    auto weakMap3 = CreateWeakMap();

    std::vector<JSMutableHandle<JSWeakRef>> weakRefs;
    for (int i = 0; i < 6; i++) {
        weakRefs.push_back(JSMutableHandle<JSWeakRef>(thread, JSTaggedValue::Undefined()));
    }

    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        auto k1 = CreateObject();
        auto k2 = CreateObject();
        auto k3 = CreateObject();
        auto v1 = CreateObject();
        auto v2 = CreateObject();
        auto v3 = CreateObject();

        // V1->K2, V2->K3, V3->K1 (cross-map cycle)
        SetProperty(v1, "ref", JSHandle<JSTaggedValue>(thread, k2.GetTaggedValue()));
        SetProperty(v2, "ref", JSHandle<JSTaggedValue>(thread, k3.GetTaggedValue()));
        SetProperty(v3, "ref", JSHandle<JSTaggedValue>(thread, k1.GetTaggedValue()));

        JSWeakMap::Set(thread, weakMap1, JSHandle<JSTaggedValue>(thread, k1.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v1.GetTaggedValue()));
        JSWeakMap::Set(thread, weakMap2, JSHandle<JSTaggedValue>(thread, k2.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v2.GetTaggedValue()));
        JSWeakMap::Set(thread, weakMap3, JSHandle<JSTaggedValue>(thread, k3.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v3.GetTaggedValue()));

        weakRefs[0].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, k1.GetTaggedValue())));
        weakRefs[1].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, k2.GetTaggedValue())));
        weakRefs[2].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, k3.GetTaggedValue())));
        weakRefs[3].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v1.GetTaggedValue())));
        weakRefs[4].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v2.GetTaggedValue())));
        weakRefs[5].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v3.GetTaggedValue())));
    }  // all objects go out of scope

    thread->GetEcmaVM()->ClearKeptObjects(thread);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);

    EXPECT_TRUE(IsCollected(weakRefs[0]));  // k1
    EXPECT_TRUE(IsCollected(weakRefs[1]));  // k2
    EXPECT_TRUE(IsCollected(weakRefs[2]));  // k3
    EXPECT_TRUE(IsCollected(weakRefs[3]));  // v1
    EXPECT_TRUE(IsCollected(weakRefs[4]));  // v2
    EXPECT_TRUE(IsCollected(weakRefs[5]));  // v3
}

// Test 5: V references K, K has external reference
// Expected: First GC: K and V should NOT be collected (K is alive via strongK)
//           Second GC (after strongK released): K and V should be collected
HWTEST_F_L0(JSWeakMapComplexScenariosTest, Test5_VReferencesKWithExternalRef)
{
    auto weakMap = CreateWeakMap();

    JSMutableHandle<JSWeakRef> kWeakRef(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSWeakRef> vWeakRef(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSObject> strongK(thread, JSTaggedValue::Undefined());

    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        auto k = CreateObject();
        auto v = CreateObject();

        // V references K
        SetProperty(v, "ref", JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()));

        JSWeakMap::Set(thread, weakMap, JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v.GetTaggedValue()));

        kWeakRef.Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, k.GetTaggedValue())));
        vWeakRef.Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v.GetTaggedValue())));
        strongK.Update(k);  // Keep external reference to k
    }  // local k and v go out of scope, but strongK still holds k

    // First GC: K should survive (has strongK), V should survive (K is alive)
    thread->GetEcmaVM()->ClearKeptObjects(thread);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_FALSE(IsCollected(kWeakRef));
    EXPECT_FALSE(IsCollected(vWeakRef));

    // Release strong reference
    strongK.Update(JSHandle<JSObject>(thread, JSTaggedValue::Undefined()));

    // Second GC: K and V should now be collected
    thread->GetEcmaVM()->ClearKeptObjects(thread);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(IsCollected(kWeakRef));
    EXPECT_TRUE(IsCollected(vWeakRef));
}

// Test 6: V has external strong reference, and V references K
// Expected: K should NOT be collected (V is alive and V->K), V should NOT be collected (has external reference)
HWTEST_F_L0(JSWeakMapComplexScenariosTest, Test6_ExternalVReferencesK)
{
    auto weakMap = CreateWeakMap();

    JSMutableHandle<JSWeakRef> kWeakRef(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSWeakRef> vWeakRef(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSObject> strongV(thread, JSTaggedValue::Undefined());

    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        auto k = CreateObject();
        auto v = CreateObject();

        // V references K
        SetProperty(v, "ref", JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()));

        JSWeakMap::Set(thread, weakMap, JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v.GetTaggedValue()));

        kWeakRef.Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, k.GetTaggedValue())));
        vWeakRef.Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v.GetTaggedValue())));
        strongV.Update(v);  // Keep external reference to v
    }  // local k and v go out of scope, but strongV still holds v

    thread->GetEcmaVM()->ClearKeptObjects(thread);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);

    // K should survive (V is alive and V->K), V should survive (has external ref)
    EXPECT_FALSE(IsCollected(kWeakRef));
    EXPECT_FALSE(IsCollected(vWeakRef));
}

// Test 7: V does not reference K
// Expected: Both K and V should be collected
HWTEST_F_L0(JSWeakMapComplexScenariosTest, Test7_VDoesNotReferenceK)
{
    auto weakMap = CreateWeakMap();

    JSMutableHandle<JSWeakRef> kWeakRef(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSWeakRef> vWeakRef(thread, JSTaggedValue::Undefined());

    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        auto k = CreateObject();
        auto v = CreateObject();
        // V does NOT reference K

        JSWeakMap::Set(thread, weakMap, JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v.GetTaggedValue()));

        kWeakRef.Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, k.GetTaggedValue())));
        vWeakRef.Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v.GetTaggedValue())));
    }  // k and v go out of scope

    thread->GetEcmaVM()->ClearKeptObjects(thread);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);

    EXPECT_TRUE(IsCollected(kWeakRef));
    EXPECT_TRUE(IsCollected(vWeakRef));
}

// Test 8: Mixed scenario - WeakMap strongly holds values when keys are alive
// Expected: First GC: K3, K4 collected (no external refs); V3, V4 collected (their keys were collected)
//                     K1, K2 survive (have external refs)
//                     V1, V2 survive (their keys are alive, WeakMap holds strong reference to values)
//           Second GC (after strongK1/K2 released): K1, K2 collected, then V1, V2 collected
HWTEST_F_L0(JSWeakMapComplexScenariosTest, Test8_MixedScenario)
{
    auto weakMap = CreateWeakMap();

    std::vector<JSMutableHandle<JSWeakRef>> weakRefs;
    for (int i = 0; i < 8; i++) {
        weakRefs.push_back(JSMutableHandle<JSWeakRef>(thread, JSTaggedValue::Undefined()));
    }

    JSMutableHandle<JSObject> strongK1(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSObject> strongK2(thread, JSTaggedValue::Undefined());

    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        auto k1 = CreateObject();  // Actual K1 with external ref
        auto k2 = CreateObject();  // Actual K2 with external ref
        auto k3 = CreateObject();
        auto k4 = CreateObject();

        auto v1 = CreateObject();
        auto v2 = CreateObject();
        auto v3 = CreateObject();
        auto v4 = CreateObject();

        // Update external references
        strongK1.Update(k1);
        strongK2.Update(k2);

        // K1 has external ref (strongK1), V1 does not reference K1
        // K2 has external ref (strongK2), V2 references K2
        // K3 no external ref, V3 references K4
        // K4 no external ref, V4 does not reference K4

        SetProperty(v2, "ref", JSHandle<JSTaggedValue>(thread, k2.GetTaggedValue()));  // V2 references K2
        SetProperty(v3, "ref", JSHandle<JSTaggedValue>(thread, k4.GetTaggedValue()));  // V3 references K4

        JSWeakMap::Set(thread, weakMap, JSHandle<JSTaggedValue>(thread, k1.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v1.GetTaggedValue()));
        JSWeakMap::Set(thread, weakMap, JSHandle<JSTaggedValue>(thread, k2.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v2.GetTaggedValue()));
        JSWeakMap::Set(thread, weakMap, JSHandle<JSTaggedValue>(thread, k3.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v3.GetTaggedValue()));
        JSWeakMap::Set(thread, weakMap, JSHandle<JSTaggedValue>(thread, k4.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v4.GetTaggedValue()));

        weakRefs[0].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, k3.GetTaggedValue())));
        weakRefs[1].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, k4.GetTaggedValue())));
        weakRefs[2].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, k1.GetTaggedValue())));
        weakRefs[3].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, k2.GetTaggedValue())));
        weakRefs[4].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v1.GetTaggedValue())));
        weakRefs[5].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v2.GetTaggedValue())));
        weakRefs[6].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v3.GetTaggedValue())));
        weakRefs[7].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v4.GetTaggedValue())));
    }  // local objects go out of scope, but strongK1 and strongK2 still hold references

    // First GC results:
    // - K3, K4 collected (no external refs)
    // - V3, V4 collected (their keys were collected)
    // - K1, K2 survive (have external refs via strongK1/strongK2)
    // - V1, V2 survive (their keys are alive, WeakMap strongly references values)
    thread->GetEcmaVM()->ClearKeptObjects(thread);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(IsCollected(weakRefs[0]));  // k3
    EXPECT_TRUE(IsCollected(weakRefs[1]));  // k4
    EXPECT_FALSE(IsCollected(weakRefs[2]));  // k1
    EXPECT_FALSE(IsCollected(weakRefs[3]));  // k2
    EXPECT_FALSE(IsCollected(weakRefs[4]));  // v1
    EXPECT_FALSE(IsCollected(weakRefs[5]));  // v2
    EXPECT_TRUE(IsCollected(weakRefs[6]));  // v3
    EXPECT_TRUE(IsCollected(weakRefs[7]));  // v4

    // Release strong references
    strongK1.Update(JSHandle<JSObject>(thread, JSTaggedValue::Undefined()));
    strongK2.Update(JSHandle<JSObject>(thread, JSTaggedValue::Undefined()));

    // Second GC: K1, K2 collected, then V1, V2 collected
    thread->GetEcmaVM()->ClearKeptObjects(thread);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(IsCollected(weakRefs[2]));  // k1
    EXPECT_TRUE(IsCollected(weakRefs[3]));  // k2
    EXPECT_TRUE(IsCollected(weakRefs[4]));  // v1
    EXPECT_TRUE(IsCollected(weakRefs[5]));  // v2
}

// Test 9: Deep nesting - V1->V2->V3->...->K
// Expected: All should be collected
HWTEST_F_L0(JSWeakMapComplexScenariosTest, Test9_DeepNesting)
{
    auto weakMap = CreateWeakMap();

    std::vector<JSMutableHandle<JSWeakRef>> weakRefs;
    for (int i = 0; i < 4; i++) {
        weakRefs.push_back(JSMutableHandle<JSWeakRef>(thread, JSTaggedValue::Undefined()));
    }

    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        auto k = CreateObject();
        auto v1 = CreateObject();
        auto v2 = CreateObject();
        auto v3 = CreateObject();

        // Create chain: V1->V2->V3->K
        SetProperty(v1, "ref", JSHandle<JSTaggedValue>(thread, v2.GetTaggedValue()));
        SetProperty(v2, "ref", JSHandle<JSTaggedValue>(thread, v3.GetTaggedValue()));
        SetProperty(v3, "ref", JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()));

        JSWeakMap::Set(thread, weakMap, JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v1.GetTaggedValue()));

        weakRefs[0].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, k.GetTaggedValue())));
        weakRefs[1].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v1.GetTaggedValue())));
        weakRefs[2].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v2.GetTaggedValue())));
        weakRefs[3].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v3.GetTaggedValue())));
    }  // all objects go out of scope

    thread->GetEcmaVM()->ClearKeptObjects(thread);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);

    EXPECT_TRUE(IsCollected(weakRefs[0]));  // k
    EXPECT_TRUE(IsCollected(weakRefs[1]));  // v1
    EXPECT_TRUE(IsCollected(weakRefs[2]));  // v2
    EXPECT_TRUE(IsCollected(weakRefs[3]));  // v3
}

// Test 10: Multiple Values referencing same Key, only one in WeakMap
// Expected: All should be collected
HWTEST_F_L0(JSWeakMapComplexScenariosTest, Test10_MultipleValuesSameKey)
{
    auto weakMap = CreateWeakMap();

    std::vector<JSMutableHandle<JSWeakRef>> weakRefs;
    for (int i = 0; i < 4; i++) {
        weakRefs.push_back(JSMutableHandle<JSWeakRef>(thread, JSTaggedValue::Undefined()));
    }

    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        auto k = CreateObject();
        auto v1 = CreateObject();
        auto v2 = CreateObject();
        auto v3 = CreateObject();

        // All V's reference the same K, but only v1 is in WeakMap
        SetProperty(v1, "ref", JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()));
        SetProperty(v2, "ref", JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()));
        SetProperty(v3, "ref", JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()));

        JSWeakMap::Set(thread, weakMap, JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v1.GetTaggedValue()));

        weakRefs[0].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, k.GetTaggedValue())));
        weakRefs[1].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v1.GetTaggedValue())));
        weakRefs[2].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v2.GetTaggedValue())));
        weakRefs[3].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v3.GetTaggedValue())));
    }  // all objects go out of scope

    thread->GetEcmaVM()->ClearKeptObjects(thread);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);

    EXPECT_TRUE(IsCollected(weakRefs[0]));  // k
    EXPECT_TRUE(IsCollected(weakRefs[1]));  // v1
    EXPECT_TRUE(IsCollected(weakRefs[2]));  // v2
    EXPECT_TRUE(IsCollected(weakRefs[3]));  // v3
}

// Test 11: Same Key in multiple WeakMaps
// Expected: When key is collected, all entries are removed from all maps
HWTEST_F_L0(JSWeakMapComplexScenariosTest, Test11_SameKeyMultipleWeakMaps)
{
    auto weakMap1 = CreateWeakMap();
    auto weakMap2 = CreateWeakMap();

    std::vector<JSMutableHandle<JSWeakRef>> weakRefs;
    for (int i = 0; i < 3; i++) {
        weakRefs.push_back(JSMutableHandle<JSWeakRef>(thread, JSTaggedValue::Undefined()));
    }

    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        auto k = CreateObject();
        auto v1 = CreateObject();
        auto v2 = CreateObject();

        // v1 and v2 both reference k
        SetProperty(v1, "ref", JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()));
        SetProperty(v2, "ref", JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()));

        // Same key in two different WeakMaps
        JSWeakMap::Set(thread, weakMap1, JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v1.GetTaggedValue()));
        JSWeakMap::Set(thread, weakMap2, JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v2.GetTaggedValue()));

        weakRefs[0].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, k.GetTaggedValue())));
        weakRefs[1].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v1.GetTaggedValue())));
        weakRefs[2].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v2.GetTaggedValue())));
    }  // all objects go out of scope

    thread->GetEcmaVM()->ClearKeptObjects(thread);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);

    EXPECT_TRUE(IsCollected(weakRefs[0]));  // k
    EXPECT_TRUE(IsCollected(weakRefs[1]));  // v1
    EXPECT_TRUE(IsCollected(weakRefs[2]));  // v2
}

// Test 12: WeakMap value is undefined
// Expected: Key should be collected (no external references)
HWTEST_F_L0(JSWeakMapComplexScenariosTest, Test12_ValueIsUndefined)
{
    auto weakMap = CreateWeakMap();

    JSMutableHandle<JSWeakRef> kWeakRef(thread, JSTaggedValue::Undefined());

    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        auto k = CreateObject();

        // Value is undefined (not referencing anything)
        JSWeakMap::Set(thread, weakMap, JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));

        kWeakRef.Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, k.GetTaggedValue())));
    }  // k goes out of scope

    thread->GetEcmaVM()->ClearKeptObjects(thread);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);

    EXPECT_TRUE(IsCollected(kWeakRef));
}

// Test 13: Non-WeakMap value references Key (edge case)
// Expected: Key should be collected (WeakMap value v1 does NOT reference k)
//           v1 should be collected (its key k was collected)
//           v2 should be collected (no external ref, not in WeakMap)
HWTEST_F_L0(JSWeakMapComplexScenariosTest, Test13_NonWeakMapValueReferencesKey)
{
    auto weakMap = CreateWeakMap();

    std::vector<JSMutableHandle<JSWeakRef>> weakRefs;
    for (int i = 0; i < 3; i++) {
        weakRefs.push_back(JSMutableHandle<JSWeakRef>(thread, JSTaggedValue::Undefined()));
    }

    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        auto k = CreateObject();
        auto v1 = CreateObject();  // This will be the WeakMap value
        auto v2 = CreateObject();  // This is NOT in WeakMap but references K

        // v1 does NOT reference k (so k can be collected)
        // v2 references k but v2 is NOT in WeakMap
        SetProperty(v2, "ref", JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()));

        // Only v1 is in WeakMap
        JSWeakMap::Set(thread, weakMap, JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v1.GetTaggedValue()));

        weakRefs[0].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, k.GetTaggedValue())));
        weakRefs[1].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v1.GetTaggedValue())));
        weakRefs[2].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v2.GetTaggedValue())));
    }  // all objects go out of scope

    thread->GetEcmaVM()->ClearKeptObjects(thread);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);

    EXPECT_TRUE(IsCollected(weakRefs[0]));  // k
    EXPECT_TRUE(IsCollected(weakRefs[1]));  // v1
    EXPECT_TRUE(IsCollected(weakRefs[2]));  // v2
}

// Test 14: Verify entry deletion after key collection
// Expected: After key is collected, entry is removed from WeakMap
HWTEST_F_L0(JSWeakMapComplexScenariosTest, Test14_EntryDeletion)
{
    auto weakMap = CreateWeakMap();

    JSMutableHandle<JSWeakRef> kWeakRef(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSWeakRef> vWeakRef(thread, JSTaggedValue::Undefined());

    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        auto k = CreateObject();
        auto v = CreateObject();
        SetProperty(v, "ref", JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()));

        JSWeakMap::Set(thread, weakMap, JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v.GetTaggedValue()));

        // Verify entry exists
        EXPECT_TRUE(weakMap->Has(thread, k.GetTaggedValue()));

        kWeakRef.Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, k.GetTaggedValue())));
        vWeakRef.Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v.GetTaggedValue())));
    }  // k and v go out of scope

    thread->GetEcmaVM()->ClearKeptObjects(thread);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);

    EXPECT_TRUE(IsCollected(kWeakRef));
    EXPECT_TRUE(IsCollected(vWeakRef));
}

// Test 15: Single WeakMap with very long reference chain (200 K-V pairs)
// Expected: All should be collected
HWTEST_F_L0(JSWeakMapComplexScenariosTest, Test15_LongChainSingleWeakMap)
{
    auto weakMap = CreateWeakMap();

    std::vector<JSMutableHandle<JSWeakRef>> weakRefs;
    for (int i = 0; i < 200; i++) {  // Test ALL keys to match JS
        weakRefs.push_back(JSMutableHandle<JSWeakRef>(thread, JSTaggedValue::Undefined()));
    }

    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        std::vector<JSHandle<JSObject>> keys;
        std::vector<JSHandle<JSObject>> values;

        // Create 200 K-V pairs
        for (int i = 0; i < 200; i++) {
            keys.push_back(CreateObject());
            values.push_back(CreateObject());
        }

        // Create chain: v1->k2, v2->k3, ..., v199->k0, v0->k1 (cycle)
        for (int i = 0; i < 200; i++) {
            int nextIndex = (i + 1) % 200;
            SetProperty(values[i], "ref", JSHandle<JSTaggedValue>(thread, keys[nextIndex].GetTaggedValue()));
            JSWeakMap::Set(thread, weakMap, JSHandle<JSTaggedValue>(thread, keys[i].GetTaggedValue()),
                          JSHandle<JSTaggedValue>(thread, values[i].GetTaggedValue()));
        }

        // Register ALL keys for verification (matches JS)
        for (int i = 0; i < 200; i++) {
            weakRefs[i].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, keys[i].GetTaggedValue())));
        }
    }  // all objects go out of scope

    thread->GetEcmaVM()->ClearKeptObjects(thread);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);

    for (int i = 0; i < 200; i++) {
        EXPECT_TRUE(IsCollected(weakRefs[i]));
    }
}

// Test 16: 5 WeakMaps with long cross-reference chains
// Expected: All should be collected
HWTEST_F_L0(JSWeakMapComplexScenariosTest, Test16_FiveWeakMapsCrossRef)
{
    std::vector<JSHandle<JSWeakMap>> weakMaps;
    for (int i = 0; i < 5; i++) {
        weakMaps.push_back(CreateWeakMap());
    }

    std::vector<JSMutableHandle<JSWeakRef>> weakRefs;
    for (int i = 0; i < 200; i++) {  // Test ALL keys to match JS
        weakRefs.push_back(JSMutableHandle<JSWeakRef>(thread, JSTaggedValue::Undefined()));
    }

    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        std::vector<JSHandle<JSObject>> keys;
        std::vector<JSHandle<JSObject>> values;

        // Create 200 K-V pairs (40 per map)
        for (int i = 0; i < 200; i++) {
            keys.push_back(CreateObject());
            values.push_back(CreateObject());
        }

        // Create cross-map chain and add to different maps
        for (int i = 0; i < 200; i++) {
            int nextIndex = (i + 1) % 200;
            SetProperty(values[i], "ref", JSHandle<JSTaggedValue>(thread, keys[nextIndex].GetTaggedValue()));
            int mapIndex = i / 40;  // 0-39 in map1, 40-79 in map2, etc.
            JSWeakMap::Set(thread, weakMaps[mapIndex], JSHandle<JSTaggedValue>(thread, keys[i].GetTaggedValue()),
                          JSHandle<JSTaggedValue>(thread, values[i].GetTaggedValue()));
        }

        // Register ALL keys for verification (matches JS)
        for (int i = 0; i < 200; i++) {
            weakRefs[i].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, keys[i].GetTaggedValue())));
        }
    }  // all objects go out of scope

    thread->GetEcmaVM()->ClearKeptObjects(thread);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);

    for (int i = 0; i < 200; i++) {
        EXPECT_TRUE(IsCollected(weakRefs[i]));
    }
}

// Test 17: 5 WeakMaps with deep nesting and cross references
// Expected: All should be collected
HWTEST_F_L0(JSWeakMapComplexScenariosTest, Test17_FiveWeakMapsDeepNesting)
{
    std::vector<JSHandle<JSWeakMap>> weakMaps;
    for (int i = 0; i < 5; i++) {
        weakMaps.push_back(CreateWeakMap());
    }

    std::vector<JSMutableHandle<JSWeakRef>> weakRefs;
    for (int i = 0; i < 200; i++) {  // Test ALL keys to match JS
        weakRefs.push_back(JSMutableHandle<JSWeakRef>(thread, JSTaggedValue::Undefined()));
    }

    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        std::vector<JSHandle<JSObject>> keys;
        std::vector<JSHandle<JSObject>> values;

        // Create 200 K-V pairs (40 per map)
        for (int i = 0; i < 200; i++) {
            keys.push_back(CreateObject());
            values.push_back(CreateObject());
        }

        // Create deep chains (10 levels) for each value, then reference next key
        for (int i = 0; i < 200; i++) {
            int nextIndex = (i + 1) % 200;

            // Build chain: temp10 -> temp9 -> ... -> temp1 -> values[i]
            // Then outermost temp references next key
            JSHandle<JSObject> chain = values[i];
            for (int j = 0; j < 10; j++) {
                auto temp = CreateObject();
                SetProperty(temp, "ref", JSHandle<JSTaggedValue>(thread, chain.GetTaggedValue()));
                chain = temp;
            }
            // Outermost temp in chain references next key
            SetProperty(chain, "ref", JSHandle<JSTaggedValue>(thread, keys[nextIndex].GetTaggedValue()));
            values[i] = chain;

            // Use chain (now values[i]) as the actual value in WeakMap
            int mapIndex = i / 40;  // 0-39 in map1, 40-79 in map2, etc.
            JSWeakMap::Set(thread, weakMaps[mapIndex], JSHandle<JSTaggedValue>(thread, keys[i].GetTaggedValue()),
                          JSHandle<JSTaggedValue>(thread, chain.GetTaggedValue()));
        }

        // Register ALL keys for verification (matches JS)
        for (int i = 0; i < 200; i++) {
            weakRefs[i].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, keys[i].GetTaggedValue())));
        }
    }  // all objects go out of scope

    thread->GetEcmaVM()->ClearKeptObjects(thread);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);

    for (int i = 0; i < 200; i++) {
        EXPECT_TRUE(IsCollected(weakRefs[i]));
    }
}

// Test 18: Very long cycle with 5 WeakMaps
// Expected: All should be collected
HWTEST_F_L0(JSWeakMapComplexScenariosTest, Test18_VeryLongCycleFiveWeakMaps)
{
    std::vector<JSHandle<JSWeakMap>> weakMaps;
    for (int i = 0; i < 5; i++) {
        weakMaps.push_back(CreateWeakMap());
    }

    std::vector<JSMutableHandle<JSWeakRef>> weakRefs;
    for (int i = 0; i < 250; i++) {  // Test ALL keys to match JS
        weakRefs.push_back(JSMutableHandle<JSWeakRef>(thread, JSTaggedValue::Undefined()));
    }

    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        std::vector<JSHandle<JSObject>> keys;
        std::vector<JSHandle<JSObject>> values;

        // Create 250 K-V pairs (50 per map)
        for (int i = 0; i < 250; i++) {
            keys.push_back(CreateObject());
            values.push_back(CreateObject());
        }

        // Create long cycle with 5-level chains
        for (int i = 0; i < 250; i++) {
            int nextIndex = (i + 1) % 250;

            // Build chain: temp5 -> temp4 -> temp3 -> temp2 -> temp1 -> values[i]
            // Then outermost temp references next key
            JSHandle<JSObject> chain = values[i];
            for (int j = 0; j < 5; j++) {
                auto temp = CreateObject();
                SetProperty(temp, "ref", JSHandle<JSTaggedValue>(thread, chain.GetTaggedValue()));
                chain = temp;
            }
            // Outermost temp in chain references next key
            SetProperty(chain, "ref", JSHandle<JSTaggedValue>(thread, keys[nextIndex].GetTaggedValue()));
            values[i] = chain;

            // Use chain (now values[i]) as the actual value in WeakMap
            int mapIndex = i / 50;  // 0-49 in map1, 50-99 in map2, etc.
            JSWeakMap::Set(thread, weakMaps[mapIndex], JSHandle<JSTaggedValue>(thread, keys[i].GetTaggedValue()),
                          JSHandle<JSTaggedValue>(thread, chain.GetTaggedValue()));
        }

        // Register ALL keys for verification (matches JS)
        for (int i = 0; i < 250; i++) {
            weakRefs[i].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, keys[i].GetTaggedValue())));
        }
    }  // all objects go out of scope

    thread->GetEcmaVM()->ClearKeptObjects(thread);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);

    for (int i = 0; i < 250; i++) {
        EXPECT_TRUE(IsCollected(weakRefs[i]));
    }
}

// Test 19: Long chain (200) with one key having external reference
// Expected: All should survive (because external reference keeps one key alive,
//           which keeps its value alive, which keeps the next key alive, etc.)
HWTEST_F_L0(JSWeakMapComplexScenariosTest, Test19_LongChainWithExternalRef)
{
    auto weakMap = CreateWeakMap();

    JSMutableHandle<JSWeakRef> strongKRef(thread, JSTaggedValue::Undefined());
    std::vector<JSMutableHandle<JSWeakRef>> weakRefs;
    for (int i = 0; i < 200; i++) {  // Test ALL keys to match JS
        weakRefs.push_back(JSMutableHandle<JSWeakRef>(thread, JSTaggedValue::Undefined()));
    }

    JSMutableHandle<JSObject> strongK(thread, JSTaggedValue::Undefined());  // External reference
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        std::vector<JSHandle<JSObject>> keys;
        std::vector<JSHandle<JSObject>> values;

        // Create 200 K-V pairs
        for (int i = 0; i < 200; i++) {
            keys.push_back(CreateObject());
            values.push_back(CreateObject());
        }

        // Set one key to have external reference (k100)
        strongK.Update(keys[100]);

        // Create chain: v[i] references k[i+1], wrapping around (v199->k0)
        for (int i = 0; i < 200; i++) {
            int nextIndex = (i + 1) % 200;
            SetProperty(values[i], "ref", JSHandle<JSTaggedValue>(thread, keys[nextIndex].GetTaggedValue()));
            JSWeakMap::Set(thread, weakMap, JSHandle<JSTaggedValue>(thread, keys[i].GetTaggedValue()),
                          JSHandle<JSTaggedValue>(thread, values[i].GetTaggedValue()));
        }

        strongKRef.Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, keys[100].GetTaggedValue())));
        // Register ALL keys for verification (matches JS)
        for (int i = 0; i < 200; i++) {
            weakRefs[i].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, keys[i].GetTaggedValue())));
        }
    }  // local handles go out of scope, but strongK still holds reference

    thread->GetEcmaVM()->ClearKeptObjects(thread);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);

    // All should survive because strongK keeps the chain alive
    EXPECT_FALSE(IsCollected(strongKRef));
    for (int i = 0; i < 200; i++) {
        EXPECT_FALSE(IsCollected(weakRefs[i]));
    }
}

// Test 20: 5 WeakMaps with one key having external reference
// Expected: All entries should survive
HWTEST_F_L0(JSWeakMapComplexScenariosTest, Test20_FiveWeakMapsWithExternalRef)
{
    std::vector<JSHandle<JSWeakMap>> weakMaps;
    for (int i = 0; i < 5; i++) {
        weakMaps.push_back(CreateWeakMap());
    }

    JSMutableHandle<JSWeakRef> strongKRef(thread, JSTaggedValue::Undefined());
    std::vector<JSMutableHandle<JSWeakRef>> weakRefs;
    for (int i = 0; i < 250; i++) {  // Test ALL keys to match JS
        weakRefs.push_back(JSMutableHandle<JSWeakRef>(thread, JSTaggedValue::Undefined()));
    }

    JSMutableHandle<JSObject> strongK(thread, JSTaggedValue::Undefined());  // External reference
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        std::vector<JSHandle<JSObject>> keys;
        std::vector<JSHandle<JSObject>> values;

        // Create 250 K-V pairs (50 per map)
        for (int i = 0; i < 250; i++) {
            keys.push_back(CreateObject());
            values.push_back(CreateObject());
        }

        // Set one key to have external reference (k125)
        strongK.Update(keys[125]);

        // Create chain and add to different maps
        for (int i = 0; i < 250; i++) {
            int nextIndex = (i + 1) % 250;
            SetProperty(values[i], "ref", JSHandle<JSTaggedValue>(thread, keys[nextIndex].GetTaggedValue()));
            int mapIndex = i / 50;  // 0-49 in map1, 50-99 in map2, etc.
            JSWeakMap::Set(thread, weakMaps[mapIndex], JSHandle<JSTaggedValue>(thread, keys[i].GetTaggedValue()),
                          JSHandle<JSTaggedValue>(thread, values[i].GetTaggedValue()));
        }

        strongKRef.Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, keys[125].GetTaggedValue())));
        // Register ALL keys for verification (matches JS)
        for (int i = 0; i < 250; i++) {
            weakRefs[i].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, keys[i].GetTaggedValue())));
        }
    }  // local handles go out of scope, but strongK still holds reference

    thread->GetEcmaVM()->ClearKeptObjects(thread);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);

    // All should survive because strongK keeps the chain alive
    EXPECT_FALSE(IsCollected(strongKRef));
    for (int i = 0; i < 250; i++) {
        EXPECT_FALSE(IsCollected(weakRefs[i]));
    }
}

// Test 21: Deep nesting with external reference
// Expected: All should survive
HWTEST_F_L0(JSWeakMapComplexScenariosTest, Test21_DeepNestingWithExternalRef)
{
    std::vector<JSHandle<JSWeakMap>> weakMaps;
    for (int i = 0; i < 5; i++) {
        weakMaps.push_back(CreateWeakMap());
    }

    JSMutableHandle<JSWeakRef> strongKRef(thread, JSTaggedValue::Undefined());
    std::vector<JSMutableHandle<JSWeakRef>> weakRefs;
    for (int i = 0; i < 200; i++) {  // Test ALL keys to match JS
        weakRefs.push_back(JSMutableHandle<JSWeakRef>(thread, JSTaggedValue::Undefined()));
    }

    JSMutableHandle<JSObject> strongK(thread, JSTaggedValue::Undefined());  // External reference
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        std::vector<JSHandle<JSObject>> keys;
        std::vector<JSHandle<JSObject>> values;

        // Create 200 K-V pairs
        for (int i = 0; i < 200; i++) {
            keys.push_back(CreateObject());
            values.push_back(CreateObject());
        }

        // Set one key to have external reference (k50)
        strongK.Update(keys[50]);

        // Create deep chains (10 levels) for each value, then reference next key
        for (int i = 0; i < 200; i++) {
            int nextIndex = (i + 1) % 200;

            // Build chain: temp10 -> temp9 -> ... -> temp1 -> values[i]
            // Then outermost temp references next key
            JSHandle<JSObject> chain = values[i];
            for (int j = 0; j < 10; j++) {
                auto temp = CreateObject();
                SetProperty(temp, "ref", JSHandle<JSTaggedValue>(thread, chain.GetTaggedValue()));
                chain = temp;
            }
            // Outermost temp in chain references next key
            SetProperty(chain, "ref", JSHandle<JSTaggedValue>(thread, keys[nextIndex].GetTaggedValue()));
            values[i] = chain;

            // Use chain (now values[i]) as the actual value in WeakMap
            int mapIndex = i / 40;  // 0-39 in map1, 40-79 in map2, etc.
            JSWeakMap::Set(thread, weakMaps[mapIndex], JSHandle<JSTaggedValue>(thread, keys[i].GetTaggedValue()),
                          JSHandle<JSTaggedValue>(thread, chain.GetTaggedValue()));
        }

        // Register ALL keys for verification (matches JS)
        strongKRef.Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, keys[50].GetTaggedValue())));
        for (int i = 0; i < 200; i++) {
            weakRefs[i].Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, keys[i].GetTaggedValue())));
        }
    }  // local handles go out of scope, but strongK still holds reference

    thread->GetEcmaVM()->ClearKeptObjects(thread);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);

    // All should survive because strongK keeps the chain alive
    EXPECT_FALSE(IsCollected(strongKRef));
    for (int i = 0; i < 200; i++) {
        EXPECT_FALSE(IsCollected(weakRefs[i]));
    }
}

HWTEST_F_L0(JSWeakMapComplexScenariosTest, Test_OldGC)
{
    auto weakMap = CreateWeakMap();

    JSMutableHandle<JSWeakRef> k1WeakRef(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSWeakRef> v1WeakRef(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSWeakRef> k2WeakRef(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSWeakRef> v2WeakRef(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSWeakRef> k3WeakRef(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSWeakRef> v3WeakRef(thread, JSTaggedValue::Undefined());

    JSMutableHandle<JSObject> strongK(thread, JSTaggedValue::Undefined());  // External reference

    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        // alive
        auto k = CreateObject();
        auto v = CreateObject();
        k1WeakRef.Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, k.GetTaggedValue())));
        v1WeakRef.Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v.GetTaggedValue())));

        strongK.Update(k);
        JSWeakMap::Set(thread, weakMap, JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v.GetTaggedValue()));

        // dead
        k = CreateObject();
        v = CreateObject();
        k2WeakRef.Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, k.GetTaggedValue())));
        v2WeakRef.Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v.GetTaggedValue())));

        JSWeakMap::Set(thread, weakMap, JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v.GetTaggedValue()));

        // delete
        k = CreateObject();
        v = CreateObject();
        k3WeakRef.Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, k.GetTaggedValue())));
        v3WeakRef.Update(CreateWeakRef(JSHandle<JSTaggedValue>(thread, v.GetTaggedValue())));

        JSWeakMap::Set(thread, weakMap, JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()),
                       JSHandle<JSTaggedValue>(thread, v.GetTaggedValue()));
        JSWeakMap::Delete(thread, weakMap, JSHandle<JSTaggedValue>(thread, k.GetTaggedValue()));
    }  // k and v go out of scope

    EXPECT_TRUE(WeakLinkedHashMap::Cast(weakMap->GetWeakLinkedMap(thread).GetTaggedObject())->VerifyLayout());
    thread->GetEcmaVM()->ClearKeptObjects(thread);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::OLD_GC);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::OLD_GC);

    EXPECT_FALSE(IsCollected(k1WeakRef));
    EXPECT_FALSE(IsCollected(v1WeakRef));
    EXPECT_TRUE(IsCollected(k2WeakRef));
    EXPECT_TRUE(IsCollected(v2WeakRef));
    EXPECT_TRUE(IsCollected(k3WeakRef));
    EXPECT_TRUE(IsCollected(v3WeakRef));
}

}  // namespace panda::test
