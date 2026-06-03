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
        JSNApi::SetTrackGlobalRef(false);
        thread_->ClearGlobalRefMap();
        TestHelper::DestroyEcmaVMWithScope(ecmaVm_, scope_);
    }

    EcmaVM *ecmaVm_ {nullptr};
    EcmaHandleScope *scope_ {nullptr};
    JSThread *thread_ {nullptr};
};

#if defined(ENABLE_DUMP_IN_FAULTLOG)
HWTEST_F_L0(GlobalRefTrackTest, TestSetTrackGlobalRef)
{
    ASSERT_FALSE(JSNApi::IsTrackGlobalRefEnabled());
    JSNApi::SetTrackGlobalRef(true);
    ASSERT_TRUE(JSNApi::IsTrackGlobalRefEnabled());
    JSNApi::SetTrackGlobalRef(false);
    ASSERT_FALSE(JSNApi::IsTrackGlobalRefEnabled());
}
#endif

HWTEST_F_L0(GlobalRefTrackTest, TestStoreAndFindGlobalRefMapping)
{
    JSNApi::SetTrackGlobalRef(true);
    uintptr_t slotAddr = 0x12345678;
    void *fakeRef = reinterpret_cast<void *>(0xABCDEF00);
    thread_->StoreGlobalRefMapping(slotAddr, fakeRef);
    void *found = thread_->FindGlobalRefMapping(slotAddr);
    ASSERT_EQ(found, fakeRef);
    ASSERT_EQ(thread_->FindGlobalRefMapping(0x99999999), nullptr);
}

HWTEST_F_L0(GlobalRefTrackTest, TestEraseGlobalRefMapping)
{
    JSNApi::SetTrackGlobalRef(true);
    uintptr_t slotAddr = 0x12345678;
    void *fakeRef = reinterpret_cast<void *>(0xABCDEF00);
    thread_->StoreGlobalRefMapping(slotAddr, fakeRef);
    ASSERT_NE(thread_->FindGlobalRefMapping(slotAddr), nullptr);
    thread_->EraseGlobalRefMapping(slotAddr);
    ASSERT_EQ(thread_->FindGlobalRefMapping(slotAddr), nullptr);
}

HWTEST_F_L0(GlobalRefTrackTest, TestDisableClearsMap)
{
    JSNApi::SetTrackGlobalRef(true);
    uintptr_t slotAddr = 0x12345678;
    void *fakeRef = reinterpret_cast<void *>(0xABCDEF00);
    thread_->StoreGlobalRefMapping(slotAddr, fakeRef);
    ASSERT_NE(thread_->FindGlobalRefMapping(slotAddr), nullptr);
    JSNApi::SetTrackGlobalRef(false);
    thread_->ClearGlobalRefMap();
    ASSERT_EQ(thread_->FindGlobalRefMapping(slotAddr), nullptr);
}

HWTEST_F_L0(GlobalRefTrackTest, TestStoreWithoutTracking)
{
    ASSERT_FALSE(JSNApi::IsTrackGlobalRefEnabled());
    uintptr_t slotAddr = 0x12345678;
    void *fakeRef = reinterpret_cast<void *>(0xABCDEF00);
    JSNApi::StoreGlobalRefMapping(ecmaVm_, slotAddr, fakeRef);
    JSNApi::SetTrackGlobalRef(true);
    ASSERT_EQ(thread_->FindGlobalRefMapping(slotAddr), nullptr);
}

#if defined(ENABLE_DUMP_IN_FAULTLOG)
HWTEST_F_L0(GlobalRefTrackTest, TestJSNApiBridge)
{
    JSNApi::SetTrackGlobalRef(false);
    ASSERT_FALSE(JSNApi::IsTrackGlobalRefEnabled());
    JSNApi::SetTrackGlobalRef(true);
    ASSERT_TRUE(JSNApi::IsTrackGlobalRefEnabled());

    uintptr_t slotAddr = 0xAAAA0000;
    void *fakeRef = reinterpret_cast<void *>(0xBBBB0000);
    JSNApi::StoreGlobalRefMapping(ecmaVm_, slotAddr, fakeRef);
    ASSERT_EQ(JSNApi::FindGlobalRefMapping(ecmaVm_, slotAddr), fakeRef);

    JSNApi::EraseGlobalRefMapping(ecmaVm_, slotAddr);
    ASSERT_EQ(JSNApi::FindGlobalRefMapping(ecmaVm_, slotAddr), nullptr);

    JSNApi::SetTrackGlobalRef(false);
    ASSERT_FALSE(JSNApi::IsTrackGlobalRefEnabled());
}
#endif

HWTEST_F_L0(GlobalRefTrackTest, TestMappingWithRealGlobalHandle)
{
    JSNApi::SetTrackGlobalRef(true);

    ObjectFactory *factory = ecmaVm_->GetFactory();
    JSHandle<JSTaggedValue> obj(factory->CreateNapiObject());

    Global<ObjectRef> globalObj(ecmaVm_, Local<JSTaggedValue>(obj.GetAddress()));
    uintptr_t slotAddr = globalObj.GetSlotAddress();
    ASSERT_NE(slotAddr, 0U);

    int fakeRef = 0;
    void *refPtr = &fakeRef;
    thread_->StoreGlobalRefMapping(slotAddr, refPtr);

    void *found = thread_->FindGlobalRefMapping(slotAddr);
    ASSERT_EQ(found, refPtr);

    thread_->EraseGlobalRefMapping(slotAddr);
    ASSERT_EQ(thread_->FindGlobalRefMapping(slotAddr), nullptr);
}

HWTEST_F_L0(GlobalRefTrackTest, TestMultipleMappings)
{
    JSNApi::SetTrackGlobalRef(true);

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

    thread_->EraseGlobalRefMapping(g1.GetSlotAddress());
    ASSERT_EQ(thread_->FindGlobalRefMapping(g1.GetSlotAddress()), nullptr);
    ASSERT_EQ(thread_->FindGlobalRefMapping(g2.GetSlotAddress()), &ref2);
}

// Guard at JSNApi::StoreGlobalRefMapping must block stores while tracking is off,
// and let them through once tracking is on. Covers the API-level contract that
// the snapshot path relies on.
#if defined(ENABLE_DUMP_IN_FAULTLOG)
HWTEST_F_L0(GlobalRefTrackTest, TestGuardBlocksStoreWhenDisabled)
{
    ASSERT_FALSE(JSNApi::IsTrackGlobalRefEnabled());

    // Disabled: store via JSNApi is a no-op (guard returns early)
    uintptr_t slotA = 0x99990000;
    int refA = 42;
    JSNApi::StoreGlobalRefMapping(ecmaVm_, slotA, &refA);
    ASSERT_EQ(thread_->FindGlobalRefMapping(slotA), nullptr);

    // Enable: same store now goes through
    JSNApi::SetTrackGlobalRef(true);
    JSNApi::StoreGlobalRefMapping(ecmaVm_, slotA, &refA);
    ASSERT_EQ(thread_->FindGlobalRefMapping(slotA), &refA);

    // Disable: subsequent stores blocked again, existing entry untouched
    JSNApi::SetTrackGlobalRef(false);
    uintptr_t slotB = 0x88880000;
    int refB = 99;
    JSNApi::StoreGlobalRefMapping(ecmaVm_, slotB, &refB);
    ASSERT_EQ(thread_->FindGlobalRefMapping(slotB), nullptr);
    ASSERT_EQ(thread_->FindGlobalRefMapping(slotA), &refA);
}
#endif
}  // namespace panda::test
