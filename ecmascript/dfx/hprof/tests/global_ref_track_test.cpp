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

#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/include/jsnapi_expo.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class GlobalRefTrackTest : public testing::Test {
public:
    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(ecmaVm_, thread_, scope_);
        ecmaVm_->SetEnableForceGC(false);
    }

    void TearDown() override
    {
        thread_->SetTrackGlobalRef(false);
        TestHelper::DestroyEcmaVMWithScope(ecmaVm_, scope_);
    }

    EcmaVM *ecmaVm_ {nullptr};
    EcmaHandleScope *scope_ {nullptr};
    JSThread *thread_ {nullptr};
};

HWTEST_F_L0(GlobalRefTrackTest, TestSetTrackGlobalRef)
{
    ASSERT_FALSE(thread_->IsTrackGlobalRefEnabled());
    thread_->SetTrackGlobalRef(true);
    ASSERT_TRUE(thread_->IsTrackGlobalRefEnabled());
    thread_->SetTrackGlobalRef(false);
    ASSERT_FALSE(thread_->IsTrackGlobalRefEnabled());
}

HWTEST_F_L0(GlobalRefTrackTest, TestStoreAndFindGlobalRefMapping)
{
    thread_->SetTrackGlobalRef(true);
    uintptr_t slotAddr = 0x12345678;
    void *fakeRef = reinterpret_cast<void *>(0xABCDEF00);
    thread_->StoreGlobalRefMapping(slotAddr, fakeRef);
    void *found = thread_->FindGlobalRefMapping(slotAddr);
    ASSERT_EQ(found, fakeRef);
    ASSERT_EQ(thread_->FindGlobalRefMapping(0x99999999), nullptr);
}

HWTEST_F_L0(GlobalRefTrackTest, TestEraseGlobalRefMapping)
{
    thread_->SetTrackGlobalRef(true);
    uintptr_t slotAddr = 0x12345678;
    void *fakeRef = reinterpret_cast<void *>(0xABCDEF00);
    thread_->StoreGlobalRefMapping(slotAddr, fakeRef);
    ASSERT_NE(thread_->FindGlobalRefMapping(slotAddr), nullptr);
    thread_->EraseGlobalRefMapping(slotAddr);
    ASSERT_EQ(thread_->FindGlobalRefMapping(slotAddr), nullptr);
}

HWTEST_F_L0(GlobalRefTrackTest, TestDisableClearsMap)
{
    thread_->SetTrackGlobalRef(true);
    uintptr_t slotAddr = 0x12345678;
    void *fakeRef = reinterpret_cast<void *>(0xABCDEF00);
    thread_->StoreGlobalRefMapping(slotAddr, fakeRef);
    ASSERT_NE(thread_->FindGlobalRefMapping(slotAddr), nullptr);
    thread_->SetTrackGlobalRef(false);
    ASSERT_EQ(thread_->FindGlobalRefMapping(slotAddr), nullptr);
}

HWTEST_F_L0(GlobalRefTrackTest, TestStoreWithoutTracking)
{
    ASSERT_FALSE(thread_->IsTrackGlobalRefEnabled());
    uintptr_t slotAddr = 0x12345678;
    void *fakeRef = reinterpret_cast<void *>(0xABCDEF00);
    thread_->StoreGlobalRefMapping(slotAddr, fakeRef);
    thread_->SetTrackGlobalRef(true);
    ASSERT_EQ(thread_->FindGlobalRefMapping(slotAddr), nullptr);
}

HWTEST_F_L0(GlobalRefTrackTest, TestJSNApiBridge)
{
    ASSERT_FALSE(panda::JSNApi::IsTrackGlobalRefEnabled(ecmaVm_));
    panda::JSNApi::SetTrackGlobalRef(ecmaVm_, true);
    ASSERT_TRUE(panda::JSNApi::IsTrackGlobalRefEnabled(ecmaVm_));

    uintptr_t slotAddr = 0xAAAA0000;
    void *fakeRef = reinterpret_cast<void *>(0xBBBB0000);
    panda::JSNApi::StoreGlobalRefMapping(ecmaVm_, slotAddr, fakeRef);
    ASSERT_EQ(panda::JSNApi::FindGlobalRefMapping(ecmaVm_, slotAddr), fakeRef);

    panda::JSNApi::EraseGlobalRefMapping(ecmaVm_, slotAddr);
    ASSERT_EQ(panda::JSNApi::FindGlobalRefMapping(ecmaVm_, slotAddr), nullptr);

    panda::JSNApi::SetTrackGlobalRef(ecmaVm_, false);
    ASSERT_FALSE(panda::JSNApi::IsTrackGlobalRefEnabled(ecmaVm_));
}

// Store mapping for a real global handle slot → verify FindGlobalRefMapping returns the ref
HWTEST_F_L0(GlobalRefTrackTest, TestMappingWithRealGlobalHandleSlot)
{
    thread_->SetTrackGlobalRef(true);

    ObjectFactory *factory = ecmaVm_->GetFactory();
    JSHandle<JSTaggedValue> obj(factory->CreateNapiObject());

    // Create a real global handle — this allocates a slot in EcmaGlobalStorage
    Global<ObjectRef> globalObj(ecmaVm_, Local<JSTaggedValue>(obj.GetAddress()));
    uintptr_t slotAddr = globalObj.GetSlotAddress();
    ASSERT_NE(slotAddr, 0U);

    // Store a mapping for this real slot address
    int fakeRef = 0;
    void *refPtr = &fakeRef;
    thread_->StoreGlobalRefMapping(slotAddr, refPtr);

    // Verify mapping is retrievable
    void *found = thread_->FindGlobalRefMapping(slotAddr);
    ASSERT_EQ(found, refPtr);

    // Erase and verify
    thread_->EraseGlobalRefMapping(slotAddr);
    ASSERT_EQ(thread_->FindGlobalRefMapping(slotAddr), nullptr);
}

// Verify multiple mappings work correctly
HWTEST_F_L0(GlobalRefTrackTest, TestMultipleMappings)
{
    thread_->SetTrackGlobalRef(true);

    ObjectFactory *factory = ecmaVm_->GetFactory();
    JSHandle<JSTaggedValue> obj1(factory->CreateNapiObject());
    JSHandle<JSTaggedValue> obj2(factory->CreateNapiObject());

    Global<ObjectRef> g1(ecmaVm_, Local<JSTaggedValue>(obj1.GetAddress()));
    Global<ObjectRef> g2(ecmaVm_, Local<JSTaggedValue>(obj2.GetAddress()));

    int ref1 = 1;
    int ref2 = 2;
    thread_->StoreGlobalRefMapping(g1.GetSlotAddress(), &ref1);
    thread_->StoreGlobalRefMapping(g2.GetSlotAddress(), &ref2);

    ASSERT_EQ(thread_->FindGlobalRefMapping(g1.GetSlotAddress()), &ref1);
    ASSERT_EQ(thread_->FindGlobalRefMapping(g2.GetSlotAddress()), &ref2);

    // Erase one, other still present
    thread_->EraseGlobalRefMapping(g1.GetSlotAddress());
    ASSERT_EQ(thread_->FindGlobalRefMapping(g1.GetSlotAddress()), nullptr);
    ASSERT_EQ(thread_->FindGlobalRefMapping(g2.GetSlotAddress()), &ref2);
}
}  // namespace panda::test
