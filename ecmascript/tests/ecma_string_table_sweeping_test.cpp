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

#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/ecma_string_table_optimization-inl.h"
#include "ecmascript/dfx/hprof/heap_profiler_interface.h"
#include "ecmascript/dfx/hprof/file_stream.h"
#include "ecmascript/dfx/hprof/progress.h"
#include "ecmascript/mem/barriers.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/mem/shared_heap/shared_gc.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_sweeper.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/runtime_lock.h"
#include "ecmascript/string/hashtriemap.h"
#include "ecmascript/string/hashtriemap-inl.h"
#include "ecmascript/tests/test_helper.h"
#include "common_components/taskpool/taskpool.h"

using namespace panda::ecmascript;

namespace panda::test {

using HashTrieMapType = DisableCMCGCConcurrentSweepTrait::HashTrieMapType;
using HashTrieMapTypeNormal = DisableCMCGCNormalTrait::HashTrieMapType;
using HashTrieMapInUseScopeType = DisableCMCGCConcurrentSweepTrait::HashTrieMapInUseScopeType;
using HashTrieMapInUseScopeTypeNormal = DisableCMCGCNormalTrait::HashTrieMapInUseScopeType;
using HashTrieMapOperationNormalType = DisableCMCGCNormalTrait::HashTrieMapOperationType;
using HashTrieMapOperationConcurrentSweepType = DisableCMCGCConcurrentSweepTrait::HashTrieMapOperationType;

class EcmaStringTableSweepingTest : public BaseTestWithScope<false> {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "EcmaStringTableSweepingTest SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "EcmaStringTableSweepingTest TearDownTestCase";
    }

protected:
    EcmaString *CreateUtf8String(const char *name)
    {
        return EcmaStringAccessor::CreateFromUtf8(thread->GetEcmaVM(),
            reinterpret_cast<const uint8_t*>(name), strlen(name), true);
    }

    EcmaString *CreateAndInternUtf8String(EcmaStringTable *stringTable, const char *name)
    {
        EcmaString *str = CreateUtf8String(name);
        return stringTable->GetOrInternString(thread->GetEcmaVM(), str);
    }

    void InternMultipleUtf8Strings(EcmaStringTable *stringTable, int count, const char *prefix, int round = -1)
    {
        char nameBuf[64];
        for (int i = 0; i < count; ++i) {
            int ret;
            if (round >= 0) {
                ret = snprintf_s(nameBuf, sizeof(nameBuf), sizeof(nameBuf) - 1,
                                 "%s_%d_%d", prefix, round, i);
            } else {
                ret = snprintf_s(nameBuf, sizeof(nameBuf), sizeof(nameBuf) - 1,
                                 "%s_%d", prefix, i);
            }
            if (ret < 0) {
                LOG_ECMA(ERROR) << "snprintf_s failed in InternMultipleUtf8Strings";
                continue;
            }
            ASSERT_NE(CreateAndInternUtf8String(stringTable, nameBuf), nullptr);
        }
    }

    void InternMultipleUtf8Strings(EcmaStringTable *stringTable, int count, const char *prefix,
                                   int phase, int round)
    {
        char nameBuf[64];
        for (int i = 0; i < count; ++i) {
            int ret = snprintf_s(nameBuf, sizeof(nameBuf), sizeof(nameBuf) - 1,
                                 "%s_%d_%d_%d", prefix, phase, round, i);
            if (ret < 0) {
                LOG_ECMA(ERROR) << "snprintf_s failed in InternMultipleUtf8Strings";
                continue;
            }
            ASSERT_NE(CreateAndInternUtf8String(stringTable, nameBuf), nullptr);
        }
    }

    template<typename OperationType, bool needIntern = true>
    BaseString *LoadOrStoreUtf8String(OperationType &operation, const uint8_t *utf8Data, uint32_t utf8Len)
    {
        uint32_t hashcode = EcmaStringAccessor::ComputeHashcodeUtf8(utf8Data, utf8Len, true);

        auto loaderCallback = [this, utf8Data, utf8Len, hashcode]() {
            EcmaString* value = EcmaStringAccessor::CreateFromUtf8(thread->GetEcmaVM(),
                utf8Data, utf8Len, true, MemSpaceType::SHARED_OLD_SPACE);
            value->SetMixHashcode(hashcode);
            JSHandle<EcmaString> stringHandle(thread, value);
            return stringHandle;
        };

        auto equalsCallback = [this, utf8Data, utf8Len](BaseString *foundString) {
            JSThread *jsThread = thread;
            return EcmaStringAccessor::StringIsEqualUint8Data(jsThread,
                EcmaString::FromBaseString(foundString), utf8Data, utf8Len, true);
        };

        return operation.template LoadOrStore<needIntern>(thread, hashcode, loaderCallback, equalsCallback);
    }

    template<typename OperationType, bool needIntern = true>
    BaseString *LoadOrStoreUtf16String(OperationType &operation, const uint16_t *utf16Data, uint32_t utf16Len)
    {
        uint32_t hashcode = EcmaStringAccessor::ComputeHashcodeUtf16(utf16Data, utf16Len);

        auto loaderCallback = [this, utf16Data, utf16Len, hashcode]() {
            EcmaString* value = EcmaStringAccessor::CreateFromUtf16(thread->GetEcmaVM(),
                utf16Data, utf16Len, true, MemSpaceType::SHARED_OLD_SPACE);
            value->SetMixHashcode(hashcode);
            JSHandle<EcmaString> stringHandle(thread, value);
            return stringHandle;
        };

        auto equalsCallback = [this, utf16Data, utf16Len](BaseString *foundString) {
            JSThread *jsThread = thread;
            return EcmaStringAccessor::StringsAreEqualUtf16(jsThread,
                EcmaString::FromBaseString(foundString), utf16Data, utf16Len);
        };

        return operation.template LoadOrStore<needIntern>(thread, hashcode, loaderCallback, equalsCallback);
    }

    template<typename OperationType, bool needIntern = true>
    BaseString *LoadOrStoreUtf16StringWithAtomicMark(OperationType &operation,
                                                      const uint16_t *utf16Data, uint32_t utf16Len)
    {
        uint32_t hashcode = EcmaStringAccessor::ComputeHashcodeUtf16(utf16Data, utf16Len);

        auto loaderCallback = [this, utf16Data, utf16Len, hashcode]() {
            EcmaString* value = EcmaStringAccessor::CreateFromUtf16(thread->GetEcmaVM(),
                utf16Data, utf16Len, true, MemSpaceType::SHARED_OLD_SPACE);
            value->SetMixHashcode(hashcode);
            Region *region = Region::ObjectAddressToRange(reinterpret_cast<JSTaggedType>(value));
            if (region != nullptr) {
                region->AtomicMark(value);
            }
            JSHandle<EcmaString> stringHandle(thread, value);
            return stringHandle;
        };

        auto equalsCallback = [this, utf16Data, utf16Len](BaseString *foundString) {
            JSThread *jsThread = thread;
            return EcmaStringAccessor::StringsAreEqualUtf16(jsThread,
                EcmaString::FromBaseString(foundString), utf16Data, utf16Len);
        };

        return operation.template LoadOrStore<needIntern>(thread, hashcode, loaderCallback, equalsCallback);
    }

    template<typename OperationType, bool needIntern = true>
    BaseString *LoadOrStoreUtf8StringWithAtomicMark(OperationType &operation,
                                                     const uint8_t *utf8Data, uint32_t utf8Len)
    {
        uint32_t hashcode = EcmaStringAccessor::ComputeHashcodeUtf8(utf8Data, utf8Len, true);

        auto loaderCallback = [this, utf8Data, utf8Len, hashcode]() {
            EcmaString* value = EcmaStringAccessor::CreateFromUtf8(thread->GetEcmaVM(),
                utf8Data, utf8Len, true, MemSpaceType::SHARED_OLD_SPACE);
            value->SetMixHashcode(hashcode);
            Region *region = Region::ObjectAddressToRange(reinterpret_cast<JSTaggedType>(value));
            if (region != nullptr) {
                region->AtomicMark(value);
            }
            JSHandle<EcmaString> stringHandle(thread, value);
            return stringHandle;
        };

        auto equalsCallback = [this, utf8Data, utf8Len](BaseString *foundString) {
            JSThread *jsThread = thread;
            return EcmaStringAccessor::StringIsEqualUint8Data(jsThread,
                EcmaString::FromBaseString(foundString), utf8Data, utf8Len, true);
        };

        return operation.template LoadOrStore<needIntern>(thread, hashcode, loaderCallback, equalsCallback);
    }

    uint32_t MakeUtf16DataFromAsciiWithNumber(uint16_t *buffer, const char *prefix, int number)
    {
        uint32_t len = 0;
        while (*prefix != '\0') {
            buffer[len++] = static_cast<uint16_t>(*prefix);
            prefix++;
        }
        buffer[len++] = static_cast<uint16_t>('0' + (number / 10));  // 10: decimal base for digit extraction
        buffer[len++] = static_cast<uint16_t>('0' + (number % 10));  // 10: decimal base for digit extraction
        return len;
    }

    uint32_t MakeUtf16DataFromAscii(uint16_t *buffer, const char *str)
    {
        uint32_t len = 0;
        while (*str != '\0') {
            buffer[len++] = static_cast<uint16_t>(*str);
            str++;
        }
        return len;
    }

    template<typename VisitorFunc>
    void DoSweepingWithClearNode(HashTrieMapType &hashTrieMap, VisitorFunc visitor)
    {
        RuntimeLockHolder locker(thread, SharedHeap::GetInstance()->GetSuspensionRequestMutex());
        hashTrieMap.StartSweeping();

        HashTrieMapOperationConcurrentSweepType concurrentSweepOp(&hashTrieMap);

        std::vector<HashTrieMapEntry*> waitDeleteEntries;
        std::vector<HashTrieMapSlotCheckInfo> waitCheckAndFreeHeadEntries;

        for (uint32_t i = 0; i < TrieMapConfig::ROOT_SIZE; ++i) {
            auto root = hashTrieMap.GetRoot(i).load(std::memory_order_acquire);
            if (root != nullptr) {
                for (uint32_t j = 0; j < TrieMapConfig::INDIRECT_SIZE; ++j) {
                    concurrentSweepOp.ClearNodeFromGC(root, j, visitor, waitDeleteEntries,
                                                      waitCheckAndFreeHeadEntries);
                }
            }
        }

        for (HashTrieMapEntry* entry : waitDeleteEntries) {
            delete entry;
        }
        waitDeleteEntries.clear();

        hashTrieMap.CheckAndFreeHeadEntries(waitCheckAndFreeHeadEntries);

        hashTrieMap.FinishSweeping();
        hashTrieMap.ClearToSpaceTagForFreshEntries();
        hashTrieMap.CleanUp();
    }

    void DoSweepingWithClearNodeKeepAll(HashTrieMapType &hashTrieMap)
    {
        auto visitorKeep = [](TaggedObject *header) -> TaggedObject* {
            return header;
        };
        DoSweepingWithClearNode(hashTrieMap, visitorKeep);
    }

    void DoSweepingWithClearNodeClearAll(HashTrieMapType &hashTrieMap)
    {
        auto visitorClear = [](TaggedObject *header) -> TaggedObject* {
            return nullptr;
        };
        DoSweepingWithClearNode(hashTrieMap, visitorClear);
    }
};

/**
 * @tc.name: StringTable_EnableDisableConcurrentSweepWithGCTest
 * @tc.desc: Test enabling and disabling concurrent sweep during GC cycles. Verify that
 *           the concurrent sweep flag state is correctly maintained after each GC cycle.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, StringTable_EnableDisableConcurrentSweepWithGCTest)
{
    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    EcmaStringTableCleaner *cleaner = stringTable->GetCleaner();
    ASSERT_NE(cleaner, nullptr);

    SharedHeap *sharedHeap = SharedHeap::GetInstance();
    ASSERT_NE(sharedHeap, nullptr);

    for (int round = 0; round < 10; ++round) {
        bool enable = (round % 2 == 0);
        cleaner->SetEnableConcurrentSweep(enable);

        InternMultipleUtf8Strings(stringTable, 100, "enable_gc", round);

        sharedHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);

        ASSERT_EQ(cleaner->IsEnableConcurrentSweep(), enable);
    }

    cleaner->SetEnableConcurrentSweep(false);
}

/**
 * @tc.name: StringTable_ParallelGCEnabledConcurrentSweepTest
 * @tc.desc: Test string table concurrent sweep with parallel GC enabled. Verify that
 *           strings can be interned correctly when parallel GC and concurrent sweep are both active.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, StringTable_ParallelGCEnabledConcurrentSweepTest)
{
    SharedHeap *sharedHeap = SharedHeap::GetInstance();
    ASSERT_NE(sharedHeap, nullptr);

    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    EcmaStringTableCleaner *cleaner = stringTable->GetCleaner();
    ASSERT_NE(cleaner, nullptr);

    InternMultipleUtf8Strings(stringTable, 100, "parallel_gc");

    cleaner->SetEnableConcurrentSweep(true);
    sharedHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);

    InternMultipleUtf8Strings(stringTable, 100, "parallel_concurrent");

    cleaner->SetEnableConcurrentSweep(false);
}

/**
 * @tc.name: SharedHeap_SharedPartialGCWithManyStringsTest
 * @tc.desc: Test shared partial GC with many strings interned. Verify that string table
 *           handles partial GC cycles correctly while interning strings.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, SharedHeap_SharedPartialGCWithManyStringsTest)
{
    SharedHeap *sharedHeap = SharedHeap::GetInstance();
    ASSERT_NE(sharedHeap, nullptr);

    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    for (int gcCycle = 0; gcCycle < 10; ++gcCycle) {
        InternMultipleUtf8Strings(stringTable, 100, "partial_gc_str", gcCycle);
        sharedHeap->CollectGarbage<TriggerGCType::SHARED_PARTIAL_GC, GCReason::OTHER>(thread);
        InternMultipleUtf8Strings(stringTable, 100, "partial_concurrent", gcCycle);
    }
}

/**
 * @tc.name: SharedHeap_AlternatingGCtypesWithStringTableTest
 * @tc.desc: Test alternating GC types (SHARED_GC and SHARED_PARTIAL_GC) with string table.
 * Verify that string table handles both GC types correctly while interning strings.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, SharedHeap_AlternatingGCtypesWithStringTableTest)
{
    SharedHeap *sharedHeap = SharedHeap::GetInstance();
    ASSERT_NE(sharedHeap, nullptr);

    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    for (int round = 0; round < 10; ++round) {
        InternMultipleUtf8Strings(stringTable, 100, "alt_gc", round);
        if (round % 2 == 0) {
            sharedHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
        } else {
            sharedHeap->CollectGarbage<TriggerGCType::SHARED_PARTIAL_GC, GCReason::OTHER>(thread);
        }
        InternMultipleUtf8Strings(stringTable, 100, "alt_concurrent", round);
    }
}

/**
 * @tc.name: HashTrieMap_SweepingFlagConsistencyTest
 * @tc.desc: Test HashTrieMap sweeping flag consistency. Verify that IsSweeping() returns
           correct state before, during, and after sweeping operations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, HashTrieMap_SweepingFlagConsistencyTest)
{
    HashTrieMapType hashTrieMap;

    for (int round = 0; round < 100; ++round) {
        ASSERT_FALSE(hashTrieMap.IsSweeping());

        hashTrieMap.StartSweeping();
        ASSERT_TRUE(hashTrieMap.IsSweeping());

        hashTrieMap.FinishSweeping();
        ASSERT_FALSE(hashTrieMap.IsSweeping());
    }
}

/**
 * @tc.name: HashTrieMapInUseScope_StackUnwindingTest
 * @tc.desc: Test HashTrieMapInUseScope stack unwinding behavior. Verify that in-use scope
           correctly manages the in-use flag during normal execution and exception scenarios.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, HashTrieMapInUseScope_StackUnwindingTest)
{
    HashTrieMapType hashTrieMap;

    for (int round = 0; round < 100; ++round) {
        uint32_t outerCount = hashTrieMap.GetInuseCount();

        HashTrieMapInUseScopeType outerScope(&hashTrieMap);
        ASSERT_EQ(hashTrieMap.GetInuseCount(), outerCount + 1);

        {
            HashTrieMapInUseScopeType innerScope1(&hashTrieMap);
            ASSERT_EQ(hashTrieMap.GetInuseCount(), outerCount + 2);

            {
                HashTrieMapInUseScopeType innerScope2(&hashTrieMap);
                ASSERT_EQ(hashTrieMap.GetInuseCount(), outerCount + 3);
            }

            ASSERT_EQ(hashTrieMap.GetInuseCount(), outerCount + 2);
        }

        ASSERT_EQ(hashTrieMap.GetInuseCount(), outerCount + 1);
    }

    ASSERT_EQ(hashTrieMap.GetInuseCount(), 0U);
}

/**
 * @tc.name: SharedHeap_CollectGarbageWithStringsTest
 * @tc.desc: Test shared heap garbage collection with strings. Verify that strings
           interned before and after GC cycles are correctly handled.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, SharedHeap_CollectGarbageWithStringsTest)
{
    SharedHeap *sharedHeap = SharedHeap::GetInstance();
    ASSERT_NE(sharedHeap, nullptr);

    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    for (int round = 0; round < 10; ++round) {
        InternMultipleUtf8Strings(stringTable, 100, "collect_gc", round);

        sharedHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);

        InternMultipleUtf8Strings(stringTable, 100, "collect_after", round);
    }
}

/**
 * @tc.name: StringTable_ReadBarrierMultipleObjectsTest
 * @tc.desc: Test read barrier with multiple objects. Verify that read barrier
           correctly handles multiple string objects during GC.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, StringTable_ReadBarrierMultipleObjectsTest)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    for (int round = 0; round < 100; ++round) {
        JSHandle<EcmaString> str = factory->NewFromASCIISkippingStringTable("multi_rb_test");
        JSTaggedType value = str.GetTaggedType();

        Region *region = Region::ObjectAddressToRange(value);
        ASSERT_NE(region, nullptr);

        region->AtomicMark(reinterpret_cast<void *>(value));
        JSTaggedType result1 = Barriers::ReadBarrierForStringTableSlot(value);
        ASSERT_EQ(result1, value);

        region->ClearMark(reinterpret_cast<void *>(value));
        JSTaggedType result2 = Barriers::ReadBarrierForStringTableSlot(value);
        ASSERT_EQ(result2, reinterpret_cast<JSTaggedType>(nullptr));

        region->AtomicMark(reinterpret_cast<void *>(value));
        JSTaggedType result3 = Barriers::ReadBarrierForStringTableSlot(value);
        ASSERT_EQ(result3, value);
    }
}

/**
 * @tc.name: HashTrieMapEntry_ToSpaceTagBasicTest
 * @tc.desc: Test HashTrieMapEntry ToSpace tag basic operations. Verify that
           ToSpace tag can be set, cleared, and checked correctly.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, HashTrieMapEntry_ToSpaceTagBasicTest)
{
    char nameBuf[32];

    for (int round = 0; round < 100; ++round) {
        int ret = snprintf_s(nameBuf, sizeof(nameBuf), sizeof(nameBuf) - 1, "tospace_%d", round);
        if (ret < 0) {
            LOG_ECMA(ERROR) << "snprintf_s failed in HashTrieMapEntry_ToSpaceTagBasicTest";
            continue;
        }
        EcmaString *str = EcmaStringAccessor::CreateFromUtf8(thread->GetEcmaVM(),
            reinterpret_cast<const uint8_t*>(nameBuf), strlen(nameBuf), true);

        HashTrieMapEntry entry(str->ToBaseString(), true);
        ASSERT_TRUE(entry.IsToSpaceObject());

        entry.ClearToSpaceTag();
        ASSERT_FALSE(entry.IsToSpaceObject());

        HashTrieMapEntry entry2(str->ToBaseString(), false);
        ASSERT_FALSE(entry2.IsToSpaceObject());
    }
}

/**
 * @tc.name: StringTableConcurrentSweep_InternUtf8Test
 * @tc.desc: Test interning UTF8 strings during concurrent sweep. Verify that
           strings can be interned correctly while sweeping is in progress.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, StringTableConcurrentSweep_InternUtf8Test)
{
    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    EcmaStringTableCleaner *cleaner = stringTable->GetCleaner();
    ASSERT_NE(cleaner, nullptr);
    cleaner->SetEnableConcurrentSweep(true);

    HashTrieMapType *hashTrieMap = reinterpret_cast<HashTrieMapType *>(stringTable->GetHashTrieMap());

    for (int round = 0; round < 10; ++round) {
        {
            RuntimeLockHolder locker(thread, SharedHeap::GetInstance()->GetSuspensionRequestMutex());
            hashTrieMap->StartSweeping();
        }

        InternMultipleUtf8Strings(stringTable, 100, "utf8_sweep", round);

        {
            RuntimeLockHolder locker(thread, SharedHeap::GetInstance()->GetSuspensionRequestMutex());
            hashTrieMap->FinishSweeping();
        }

        hashTrieMap->ClearToSpaceTagForFreshEntries();
        hashTrieMap->CleanUp();
    }

    cleaner->SetEnableConcurrentSweep(false);
}

/**
 * @tc.name: StringTableConcurrentSweep_CheckValidityTest
 * @tc.desc: Test string table validity check during concurrent sweep. Verify that
           string table remains valid while strings are interned during sweeping.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, StringTableConcurrentSweep_CheckValidityTest)
{
    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    EcmaStringTableCleaner *cleaner = stringTable->GetCleaner();
    ASSERT_NE(cleaner, nullptr);
    cleaner->SetEnableConcurrentSweep(true);

    HashTrieMapType *hashTrieMap = reinterpret_cast<HashTrieMapType *>(stringTable->GetHashTrieMap());

    for (int round = 0; round < 10; ++round) {
        InternMultipleUtf8Strings(stringTable, 100, "validity", round);

        {
            RuntimeLockHolder locker(thread, SharedHeap::GetInstance()->GetSuspensionRequestMutex());
            hashTrieMap->StartSweeping();
        }

        InternMultipleUtf8Strings(stringTable, 100, "validity_sweep", round);

        {
            RuntimeLockHolder locker(thread, SharedHeap::GetInstance()->GetSuspensionRequestMutex());
            hashTrieMap->FinishSweeping();
        }

        hashTrieMap->ClearToSpaceTagForFreshEntries();
        ASSERT_TRUE(stringTable->CheckStringTableValidity(thread));
        hashTrieMap->CleanUp();
    }

    cleaner->SetEnableConcurrentSweep(false);
}

/**
 * @tc.name: StringTable_HasStringTest
 * @tc.desc: Test StringTable HasString functionality. Verify that interned strings
           can be correctly found in the string table.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, StringTable_HasStringTest)
{
    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    char nameBuf[64];

    for (int round = 0; round < 100; ++round) {
        int ret = snprintf_s(nameBuf, sizeof(nameBuf), sizeof(nameBuf) - 1, "has_string_%d", round);
        if (ret < 0) {
            LOG_ECMA(ERROR) << "snprintf_s failed in HasStringTest";
            continue;
        }
        EcmaString *str = CreateUtf8String(nameBuf);
        stringTable->GetOrInternString(thread->GetEcmaVM(), str);

        EcmaString *found = stringTable->GetOrInternString(thread->GetEcmaVM(),
            reinterpret_cast<const uint8_t*>(nameBuf), strlen(nameBuf), true);
        ASSERT_NE(found, nullptr);
    }
}

/**
 * @tc.name: StringTable_InternAfterMultipleGCRoundsTest
 * @tc.desc: Test interning strings after multiple GC rounds. Verify that
           existing strings remain valid and new strings can be interned after multiple GC cycles.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, StringTable_InternAfterMultipleGCRoundsTest)
{
    SharedHeap *sharedHeap = SharedHeap::GetInstance();
    ASSERT_NE(sharedHeap, nullptr);

    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    std::vector<EcmaString *> existingStrings;

    for (int i = 0; i < 100; ++i) {
        char nameBuf[64];
        int ret = snprintf_s(nameBuf, sizeof(nameBuf), sizeof(nameBuf) - 1, "existing_%d", i);
        if (ret < 0) {
            LOG_ECMA(ERROR) << "snprintf_s failed in InternAfterMultipleGCRoundsTest";
            continue;
        }
        EcmaString *str = EcmaStringAccessor::CreateFromUtf8(thread->GetEcmaVM(),
            reinterpret_cast<const uint8_t*>(nameBuf), strlen(nameBuf), true);
        existingStrings.push_back(stringTable->GetOrInternString(thread->GetEcmaVM(), str));
    }

    for (int gcRound = 0; gcRound < 10; ++gcRound) {
        sharedHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
        InternMultipleUtf8Strings(stringTable, 100, "after_gc", gcRound);
    }
}

/**
 * @tc.name: StringTable_InternThenLookupUtf8WithGCTest
 * @tc.desc: Test interning UTF8 strings then looking them up with GC. Verify that
 *           UTF8 strings interned before GC can still be found after GC.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, StringTable_InternThenLookupUtf8WithGCTest)
{
    SharedHeap *sharedHeap = SharedHeap::GetInstance();
    ASSERT_NE(sharedHeap, nullptr);

    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    for (int round = 0; round < 10; ++round) {
        InternMultipleUtf8Strings(stringTable, 100, "lookup_utf8", round);
        sharedHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
        InternMultipleUtf8Strings(stringTable, 100, "lookup_utf8", round);
    }
}

/**
 * @tc.name: StringTable_InternThenLookupUtf16WithGCTest
 * @tc.desc: Test interning UTF16 strings then looking them up with GC. Verify that
 *           UTF16 strings interned before GC can still be found after GC.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, StringTable_InternThenLookupUtf16WithGCTest)
{
    SharedHeap *sharedHeap = SharedHeap::GetInstance();
    ASSERT_NE(sharedHeap, nullptr);

    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    uint16_t utf16Data[] = {0x4E2D, 0x6587, 0x6D4B, 0x8BD5, 0x5B57, 0x7B26};

    for (int round = 0; round < 10; ++round) {
        for (int i = 0; i < 100; ++i) {
            EcmaString *str = stringTable->GetOrInternString(thread->GetEcmaVM(),
                utf16Data, sizeof(utf16Data) / sizeof(uint16_t), false);
            ASSERT_NE(str, nullptr);
        }
        sharedHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
        for (int i = 0; i < 100; ++i) {
            EcmaString *str = stringTable->GetOrInternString(thread->GetEcmaVM(),
                utf16Data, sizeof(utf16Data) / sizeof(uint16_t), false);
            ASSERT_NE(str, nullptr);
        }
    }
}

/**
 * @tc.name: StringTable_InternThenLookupFlattenWithGCTest
 * @tc.desc: Test interning flatten strings then looking them up with GC. Verify that
 *           flatten strings interned before GC can still be found after GC.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, StringTable_InternThenLookupFlattenWithGCTest)
{
    SharedHeap *sharedHeap = SharedHeap::GetInstance();
    ASSERT_NE(sharedHeap, nullptr);

    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    for (int round = 0; round < 10; ++round) {
        for (int i = 0; i < 100; ++i) {
            char nameBuf[64];
            int ret = snprintf_s(nameBuf, sizeof(nameBuf), sizeof(nameBuf) - 1,
                                 "flatten_lookup_%d_%d", round, i);
            if (ret < 0) {
                LOG_ECMA(ERROR) << "snprintf_s failed in FlattenWithGCTest";
                continue;
            }
            EcmaString *str = CreateUtf8String(nameBuf);
            EcmaString *flattened = EcmaStringAccessor::Flatten(thread->GetEcmaVM(),
                JSHandle<EcmaString>(thread, str));
            EcmaString *interned = stringTable->GetOrInternFlattenString(thread->GetEcmaVM(), flattened);
            ASSERT_NE(interned, nullptr);
        }
        sharedHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
        for (int i = 0; i < 100; ++i) {
            char nameBuf[64];
            int ret = snprintf_s(nameBuf, sizeof(nameBuf), sizeof(nameBuf) - 1,
                                 "flatten_lookup_%d_%d", round, i);
            if (ret < 0) {
                LOG_ECMA(ERROR) << "snprintf_s failed in FlattenWithGCTest";
                continue;
            }
            EcmaString *str = CreateUtf8String(nameBuf);
            EcmaString *flattened = EcmaStringAccessor::Flatten(thread->GetEcmaVM(),
                JSHandle<EcmaString>(thread, str));
            EcmaString *interned = stringTable->GetOrInternFlattenString(thread->GetEcmaVM(), flattened);
            ASSERT_NE(interned, nullptr);
        }
    }
}

/**
 * @tc.name: StringTable_InternThenLookupConcatWithGCTest
 * @tc.desc: Test interning concat strings then looking them up with GC. Verify that
 *           concat strings interned before GC can still be found after GC.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, StringTable_InternThenLookupConcatWithGCTest)
{
    SharedHeap *sharedHeap = SharedHeap::GetInstance();
    ASSERT_NE(sharedHeap, nullptr);

    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    char nameBuf[64];

    for (int round = 0; round < 10; ++round) {
        int ret = snprintf_s(nameBuf, sizeof(nameBuf), sizeof(nameBuf) - 1,
                             "concat_lookup_%d_", round);
        if (ret < 0) {
            LOG_ECMA(ERROR) << "snprintf_s failed in ConcatWithGCTest";
            continue;
        }
        JSHandle<EcmaString> baseFirst = thread->GetEcmaVM()->GetFactory()->NewFromASCII(nameBuf);
        JSHandle<EcmaString> baseSecond = thread->GetEcmaVM()->GetFactory()->NewFromASCII(nameBuf);

        for (int i = 0; i < 100; ++i) {
            EcmaString *concatenated = stringTable->GetOrInternString(thread->GetEcmaVM(), baseFirst, baseSecond);
            ASSERT_NE(concatenated, nullptr);
        }
        sharedHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
        for (int i = 0; i < 100; ++i) {
            EcmaString *concatenated = stringTable->GetOrInternString(thread->GetEcmaVM(), baseFirst, baseSecond);
            ASSERT_NE(concatenated, nullptr);
        }
    }
}

/**
 * @tc.name: ConcurrentSweep_InternUtf8StringDuringSweepingTest
 * @tc.desc: Test interning UTF8 strings during concurrent sweep. Verify that
 *           UTF8 strings can be correctly interned while sweeping is active.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, ConcurrentSweep_InternUtf8StringDuringSweepingTest)
{
    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    EcmaStringTableCleaner *cleaner = stringTable->GetCleaner();
    ASSERT_NE(cleaner, nullptr);
    cleaner->SetEnableConcurrentSweep(true);

    HashTrieMapType *hashTrieMap = reinterpret_cast<HashTrieMapType *>(stringTable->GetHashTrieMap());

    const char *utf8Data = "utf8_sweep_test";

    for (int round = 0; round < 10; ++round) {
        {
            RuntimeLockHolder locker(thread, SharedHeap::GetInstance()->GetSuspensionRequestMutex());
            hashTrieMap->StartSweeping();
        }

        EcmaString *firstIntern = nullptr;
        for (int i = 0; i < 100; ++i) {
            EcmaString *str = stringTable->GetOrInternString(thread->GetEcmaVM(),
                reinterpret_cast<const uint8_t*>(utf8Data), strlen(utf8Data), true);
            if (firstIntern == nullptr) {
                firstIntern = str;
            }
            ASSERT_EQ(str, firstIntern);
        }

        {
            RuntimeLockHolder locker(thread, SharedHeap::GetInstance()->GetSuspensionRequestMutex());
            hashTrieMap->FinishSweeping();
        }

        hashTrieMap->ClearToSpaceTagForFreshEntries();
        hashTrieMap->CleanUp();
    }

    cleaner->SetEnableConcurrentSweep(false);
}

/**
 * @tc.name: ConcurrentSweep_InternUtf16StringDuringSweepingTest
 * @tc.desc: Test interning UTF16 strings during concurrent sweep. Verify that
           UTF16 strings can be correctly interned while sweeping is active.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, ConcurrentSweep_InternUtf16StringDuringSweepingTest)
{
    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    EcmaStringTableCleaner *cleaner = stringTable->GetCleaner();
    ASSERT_NE(cleaner, nullptr);
    cleaner->SetEnableConcurrentSweep(true);

    HashTrieMapType *hashTrieMap = reinterpret_cast<HashTrieMapType *>(stringTable->GetHashTrieMap());

    uint16_t utf16Data[] = {0x4E2D, 0x6587, 0x6D4B, 0x8BD5, 0x5B57, 0x7B26};

    for (int round = 0; round < 10; ++round) {
        {
            RuntimeLockHolder locker(thread, SharedHeap::GetInstance()->GetSuspensionRequestMutex());
            hashTrieMap->StartSweeping();
        }

        EcmaString *firstIntern = nullptr;
        for (int i = 0; i < 100; ++i) {
            EcmaString *str = stringTable->GetOrInternString(thread->GetEcmaVM(),
                utf16Data, sizeof(utf16Data) / sizeof(uint16_t), false);
            if (firstIntern == nullptr) {
                firstIntern = str;
            }
            ASSERT_EQ(str, firstIntern);
        }

        {
            RuntimeLockHolder locker(thread, SharedHeap::GetInstance()->GetSuspensionRequestMutex());
            hashTrieMap->FinishSweeping();
        }

        hashTrieMap->ClearToSpaceTagForFreshEntries();
        hashTrieMap->CleanUp();
    }

    cleaner->SetEnableConcurrentSweep(false);
}

/**
 * @tc.name: ConcurrentSweep_InternFlattenStringDuringSweepingTest
 * @tc.desc: Test interning flatten strings during concurrent sweep. Verify that
           flattened strings can be correctly interned while sweeping is in progress.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, ConcurrentSweep_InternFlattenStringDuringSweepingTest)
{
    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    EcmaStringTableCleaner *cleaner = stringTable->GetCleaner();
    ASSERT_NE(cleaner, nullptr);
    cleaner->SetEnableConcurrentSweep(true);

    HashTrieMapType *hashTrieMap = reinterpret_cast<HashTrieMapType *>(stringTable->GetHashTrieMap());

    char nameBuf[64];

    for (int round = 0; round < 10; ++round) {
        {
            RuntimeLockHolder locker(thread, SharedHeap::GetInstance()->GetSuspensionRequestMutex());
            hashTrieMap->StartSweeping();
        }

        for (int i = 0; i < 100; ++i) {
            int ret = snprintf_s(nameBuf, sizeof(nameBuf), sizeof(nameBuf) - 1,
                                 "flatten_sweep_%d_%d", round, i);
            if (ret < 0) {
                LOG_ECMA(ERROR) << "snprintf_s failed in FlattenStringDuringSweepingTest";
                continue;
            }
            EcmaString *str = CreateUtf8String(nameBuf);
            EcmaString *flattened = EcmaStringAccessor::Flatten(thread->GetEcmaVM(),
                JSHandle<EcmaString>(thread, str));
            EcmaString *interned = stringTable->GetOrInternFlattenString(thread->GetEcmaVM(), flattened);
            ASSERT_NE(interned, nullptr);
        }

        {
            RuntimeLockHolder locker(thread, SharedHeap::GetInstance()->GetSuspensionRequestMutex());
            hashTrieMap->FinishSweeping();
        }

        hashTrieMap->ClearToSpaceTagForFreshEntries();
        hashTrieMap->CleanUp();
    }

    cleaner->SetEnableConcurrentSweep(false);
}

/**
 * @tc.name: ConcurrentSweep_InternConcatStringDuringSweepingTest
 * @tc.desc: Test interning concat strings during concurrent sweep. Verify that
           concatenated strings can be correctly interned while sweeping is active.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, ConcurrentSweep_InternConcatStringDuringSweepingTest)
{
    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    EcmaStringTableCleaner *cleaner = stringTable->GetCleaner();
    ASSERT_NE(cleaner, nullptr);
    cleaner->SetEnableConcurrentSweep(true);

    HashTrieMapType *hashTrieMap = reinterpret_cast<HashTrieMapType *>(stringTable->GetHashTrieMap());

    char nameBuf[64];

    for (int round = 0; round < 10; ++round) {
        int ret = snprintf_s(nameBuf, sizeof(nameBuf), sizeof(nameBuf) - 1, "concat_base_%d_", round);
        if (ret < 0) {
            LOG_ECMA(ERROR) << "snprintf_s failed in ConcatStringDuringSweepingTest";
            continue;
        }
        JSHandle<EcmaString> baseFirst = thread->GetEcmaVM()->GetFactory()->NewFromASCII(nameBuf);
        JSHandle<EcmaString> baseSecond = thread->GetEcmaVM()->GetFactory()->NewFromASCII(nameBuf);

        {
            RuntimeLockHolder locker(thread, SharedHeap::GetInstance()->GetSuspensionRequestMutex());
            hashTrieMap->StartSweeping();
        }

        for (int i = 0; i < 100; ++i) {
            EcmaString *concatenated = stringTable->GetOrInternString(thread->GetEcmaVM(), baseFirst, baseSecond);
            ASSERT_NE(concatenated, nullptr);
        }

        {
            RuntimeLockHolder locker(thread, SharedHeap::GetInstance()->GetSuspensionRequestMutex());
            hashTrieMap->FinishSweeping();
        }

        hashTrieMap->ClearToSpaceTagForFreshEntries();
        hashTrieMap->CleanUp();
    }

    cleaner->SetEnableConcurrentSweep(false);
}

/**
 * @tc.name: ConcurrentSweep_InternAfterMultipleSweepRoundsTest
 * @tc.desc: Test interning strings after multiple sweep rounds. Verify that
           strings can be interned correctly across multiple concurrent sweep cycles.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, ConcurrentSweep_InternAfterMultipleSweepRoundsTest)
{
    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    EcmaStringTableCleaner *cleaner = stringTable->GetCleaner();
    ASSERT_NE(cleaner, nullptr);
    cleaner->SetEnableConcurrentSweep(true);

    HashTrieMapType *hashTrieMap = reinterpret_cast<HashTrieMapType *>(stringTable->GetHashTrieMap());

    std::vector<EcmaString *> existingStrings;

    for (int i = 0; i < 100; ++i) {
        char nameBuf[64];
        int ret = snprintf_s(nameBuf, sizeof(nameBuf), sizeof(nameBuf) - 1, "sweep_existing_%d", i);
        if (ret < 0) {
            LOG_ECMA(ERROR) << "snprintf_s failed in InternAfterMultipleSweepRoundsTest";
            continue;
        }
        EcmaString *str = EcmaStringAccessor::CreateFromUtf8(thread->GetEcmaVM(),
            reinterpret_cast<const uint8_t*>(nameBuf), strlen(nameBuf), true);
        existingStrings.push_back(stringTable->GetOrInternString(thread->GetEcmaVM(), str));
    }

    for (int sweepRound = 0; sweepRound < 10; ++sweepRound) {
        {
            RuntimeLockHolder locker(thread, SharedHeap::GetInstance()->GetSuspensionRequestMutex());
            hashTrieMap->StartSweeping();
        }

        InternMultipleUtf8Strings(stringTable, 100, "sweep_after", sweepRound);

        {
            RuntimeLockHolder locker(thread, SharedHeap::GetInstance()->GetSuspensionRequestMutex());
            hashTrieMap->FinishSweeping();
        }

        hashTrieMap->ClearToSpaceTagForFreshEntries();
        hashTrieMap->CleanUp();
    }

    cleaner->SetEnableConcurrentSweep(false);
}

/**
 * @tc.name: ConcurrentSweep_ReadBarrierDuringSweepingTest
 * @tc.desc: Test read barrier during sweeping operations. Verify that
           read barrier correctly handles string access during concurrent sweep.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, ConcurrentSweep_ReadBarrierDuringSweepingTest)
{
    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    EcmaStringTableCleaner *cleaner = stringTable->GetCleaner();
    ASSERT_NE(cleaner, nullptr);
    cleaner->SetEnableConcurrentSweep(true);

    HashTrieMapType *hashTrieMap = reinterpret_cast<HashTrieMapType *>(stringTable->GetHashTrieMap());

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    for (int round = 0; round < 10; ++round) {
        {
            RuntimeLockHolder locker(thread, SharedHeap::GetInstance()->GetSuspensionRequestMutex());
            hashTrieMap->StartSweeping();
        }

        for (int i = 0; i < 100; ++i) {
            JSHandle<EcmaString> strHandle = factory->NewFromASCIISkippingStringTable("rb_sweep_test");
            JSTaggedType value = strHandle.GetTaggedType();
            Region *region = Region::ObjectAddressToRange(value);
            if (region != nullptr) {
                region->AtomicMark(reinterpret_cast<void *>(value));
                JSTaggedType result = Barriers::ReadBarrierForStringTableSlot(value);
                ASSERT_TRUE(result == value || result == reinterpret_cast<JSTaggedType>(nullptr));
            }
        }

        {
            RuntimeLockHolder locker(thread, SharedHeap::GetInstance()->GetSuspensionRequestMutex());
            hashTrieMap->FinishSweeping();
        }

        hashTrieMap->ClearToSpaceTagForFreshEntries();
        hashTrieMap->CleanUp();
    }

    cleaner->SetEnableConcurrentSweep(false);
}

/**
 * @tc.name: ConcurrentSweep_InternEmptyStringTest
 * @tc.desc: Test interning empty strings during concurrent sweep. Verify that
           empty strings are correctly handled during sweeping operations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, ConcurrentSweep_InternEmptyStringTest)
{
    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    EcmaStringTableCleaner *cleaner = stringTable->GetCleaner();
    ASSERT_NE(cleaner, nullptr);
    cleaner->SetEnableConcurrentSweep(true);

    HashTrieMapType *hashTrieMap = reinterpret_cast<HashTrieMapType *>(stringTable->GetHashTrieMap());

    for (int round = 0; round < 10; ++round) {
        {
            RuntimeLockHolder locker(thread, SharedHeap::GetInstance()->GetSuspensionRequestMutex());
            hashTrieMap->StartSweeping();
        }

        EcmaString *emptyStr = EcmaStringAccessor::CreateEmptyString(thread->GetEcmaVM());
        EcmaString *firstIntern = stringTable->GetOrInternString(thread->GetEcmaVM(), emptyStr);
        ASSERT_NE(firstIntern, nullptr);

        for (int i = 0; i < 100; ++i) {
            EcmaString *emptyStr2 = EcmaStringAccessor::CreateEmptyString(thread->GetEcmaVM());
            EcmaString *interned = stringTable->GetOrInternString(thread->GetEcmaVM(), emptyStr2);
            ASSERT_EQ(interned, firstIntern);
        }

        {
            RuntimeLockHolder locker(thread, SharedHeap::GetInstance()->GetSuspensionRequestMutex());
            hashTrieMap->FinishSweeping();
        }

        hashTrieMap->ClearToSpaceTagForFreshEntries();
        hashTrieMap->CleanUp();
    }

    cleaner->SetEnableConcurrentSweep(false);
}

/**
 * @tc.name: SharedGC_ParallelGCEnableDisableTest
 * @tc.desc: Test shared GC with parallel GC enable/disable. Verify that
           string table handles parallel GC state changes correctly.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, SharedGC_ParallelGCEnableDisableTest)
{
    SharedHeap *sharedHeap = SharedHeap::GetInstance();
    ASSERT_NE(sharedHeap, nullptr);

    Heap *heap = thread->GetEcmaVM()->GetHeap();
    ASSERT_NE(heap, nullptr);

    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    EcmaStringTableCleaner *cleaner = stringTable->GetCleaner();
    ASSERT_NE(cleaner, nullptr);

    bool originalParallelGC = heap->IsParallelGCEnabled();

    for (int phase = 0; phase < 10; ++phase) {
        bool enableParallel = (phase % 2 == 0);
        heap->SetParallelGCEnabled(enableParallel);

        for (int round = 0; round < 10; ++round) {
            InternMultipleUtf8Strings(stringTable, 10, "parallel_test", phase, round);

            cleaner->SetEnableConcurrentSweep(true);
            sharedHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);

            InternMultipleUtf8Strings(stringTable, 10, "parallel_concurrent", phase, round);

            cleaner->SetEnableConcurrentSweep(false);
        }
    }

    heap->SetParallelGCEnabled(originalParallelGC);
}

/**
 * @tc.name: StringTable_IsSweepingTest
 * @tc.desc: Test StringTable IsSweeping functionality. Verify that IsSweeping() returns
           correct state during concurrent sweep operations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, StringTable_IsSweepingTest)
{
    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    EcmaStringTableCleaner *cleaner = stringTable->GetCleaner();
    ASSERT_NE(cleaner, nullptr);
    cleaner->SetEnableConcurrentSweep(true);

    HashTrieMapType *hashTrieMap = reinterpret_cast<HashTrieMapType *>(stringTable->GetHashTrieMap());

    for (int round = 0; round < 10; ++round) {
        ASSERT_FALSE(stringTable->IsSweeping());

        {
            RuntimeLockHolder locker(thread, SharedHeap::GetInstance()->GetSuspensionRequestMutex());
            hashTrieMap->StartSweeping();
        }
        ASSERT_TRUE(stringTable->IsSweeping());

        InternMultipleUtf8Strings(stringTable, 100, "sweeping_test", round);

        {
            RuntimeLockHolder locker(thread, SharedHeap::GetInstance()->GetSuspensionRequestMutex());
            hashTrieMap->FinishSweeping();
        }
        ASSERT_FALSE(stringTable->IsSweeping());

        hashTrieMap->ClearToSpaceTagForFreshEntries();
        hashTrieMap->CleanUp();
    }

    cleaner->SetEnableConcurrentSweep(false);
}

/**
 * @tc.name: StringTable_IsInUseTest
 * @tc.desc: Test StringTable IsInUse functionality. Verify that IsInUse() correctly
           reflects the in-use state during HashTrieMapInUseScope.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, StringTable_IsInUseTest)
{
    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    HashTrieMapType *hashTrieMap = reinterpret_cast<HashTrieMapType *>(stringTable->GetHashTrieMap());

    for (int round = 0; round < 10; ++round) {
        ASSERT_FALSE(stringTable->IsInUse());

        {
            HashTrieMapInUseScopeType scope(hashTrieMap);
            ASSERT_TRUE(stringTable->IsInUse());

            InternMultipleUtf8Strings(stringTable, 100, "inuse_test", round);

            ASSERT_TRUE(stringTable->IsInUse());
        }

        ASSERT_FALSE(stringTable->IsInUse());
    }
}

/**
 * @tc.name: StringTable_EnableStringTableConcurrentSweepPropertyTest
 * @tc.desc: Test StringTable concurrent sweep property configuration. Verify that
           the concurrent sweep property can be correctly enabled and disabled via ArkProperties.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, StringTable_EnableStringTableConcurrentSweepPropertyTest)
{
    SharedHeap *sharedHeap = SharedHeap::GetInstance();
    ASSERT_NE(sharedHeap, nullptr);

    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    int64_t originalArkProperties = thread->GetEcmaVM()->GetJSOptions().GetArkProperties();

    for (int phase = 0; phase < 10; ++phase) {
        bool enableConcurrentSweep = (phase % 2 == 0);
        int64_t newProperties = originalArkProperties;
        if (enableConcurrentSweep) {
            newProperties &= ~ArkProperties::DISABLE_STRING_TABLE_CONCURRENT_SWEEP;
        } else {
            newProperties |= ArkProperties::DISABLE_STRING_TABLE_CONCURRENT_SWEEP;
        }
        thread->GetEcmaVM()->GetJSOptions().SetArkProperties(newProperties);

        bool actualValue = thread->GetEcmaVM()->GetJSOptions().EnableStringTableConcurrentSweep();
        ASSERT_EQ(actualValue, enableConcurrentSweep);

        InternMultipleUtf8Strings(stringTable, 100, "property_test", phase);

        sharedHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);

        InternMultipleUtf8Strings(stringTable, 100, "property_after_gc", phase);
    }

    thread->GetEcmaVM()->GetJSOptions().SetArkProperties(originalArkProperties);
}

/**
 * @tc.name: HeapProfiler_DumpHeapSnapshotWithSharedGCTest
 * @tc.desc: Test heap profiler dump snapshot with shared GC. Verify that
           heap snapshot can be dumped correctly while interning strings and performing GC.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, HeapProfiler_DumpHeapSnapshotWithSharedGCTest)
{
    SharedHeap *sharedHeap = SharedHeap::GetInstance();
    ASSERT_NE(sharedHeap, nullptr);

    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    HeapProfilerInterface *heapProfiler = HeapProfilerInterface::GetInstance(thread->GetEcmaVM());
    ASSERT_NE(heapProfiler, nullptr);

    for (int round = 0; round < 10; ++round) {
        InternMultipleUtf8Strings(stringTable, 100, "dump_gc", round);

        DumpSnapShotOption dumpOption;
        dumpOption.isFullGC = false;
        dumpOption.isSync = true;
        dumpOption.dumpFormat = DumpFormat::JSON;

        std::string fileName = "test_dump_gc_" + std::to_string(round) + ".heapsnapshot";
        FileStream stream(fileName.c_str());
        heapProfiler->DumpHeapSnapshot(&stream, dumpOption);

        sharedHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);

        InternMultipleUtf8Strings(stringTable, 100, "dump_after_gc", round);
    }

    HeapProfilerInterface::Destroy(thread->GetEcmaVM());
}

/**
 * @tc.name: HeapProfiler_StartHeapTrackingWithSharedGCTest
 * @tc.desc: Test heap profiler start tracking with shared GC. Verify that
           heap tracking can be started correctly during string interning and GC operations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, HeapProfiler_StartHeapTrackingWithSharedGCTest)
{
    SharedHeap *sharedHeap = SharedHeap::GetInstance();
    ASSERT_NE(sharedHeap, nullptr);

    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    HeapProfilerInterface *heapProfiler = HeapProfilerInterface::GetInstance(thread->GetEcmaVM());
    ASSERT_NE(heapProfiler, nullptr);

    for (int round = 0; round < 10; ++round) {
        InternMultipleUtf8Strings(stringTable, 100, "tracking_gc", round);

        sharedHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);

        std::string fileName = "test_tracking_gc_" + std::to_string(round) + ".heaptimeline";
        FileStream stream(fileName.c_str());
        heapProfiler->StartHeapTracking(50.0, true, &stream, false, false);

        InternMultipleUtf8Strings(stringTable, 100, "tracking_after", round);

        heapProfiler->StopHeapTracking(&stream, nullptr, false);
    }

    HeapProfilerInterface::Destroy(thread->GetEcmaVM());
}

/**
 * @tc.name: HeapProfiler_UpdateHeapTrackingWithSharedGCTest
 * @tc.desc: Test heap profiler update tracking with shared GC. Verify that
           heap tracking updates correctly during string interning and GC cycles.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, HeapProfiler_UpdateHeapTrackingWithSharedGCTest)
{
    SharedHeap *sharedHeap = SharedHeap::GetInstance();
    ASSERT_NE(sharedHeap, nullptr);

    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    HeapProfilerInterface *heapProfiler = HeapProfilerInterface::GetInstance(thread->GetEcmaVM());
    ASSERT_NE(heapProfiler, nullptr);

    std::string fileName = "test_update_tracking.heaptimeline";
    FileStream stream(fileName.c_str());
    heapProfiler->StartHeapTracking(50.0, true, &stream, false, false);

    for (int round = 0; round < 10; ++round) {
        InternMultipleUtf8Strings(stringTable, 100, "update_tracking", round);

        sharedHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);

        heapProfiler->UpdateHeapTracking(&stream);

        InternMultipleUtf8Strings(stringTable, 100, "update_after", round);
    }

    heapProfiler->StopHeapTracking(&stream, nullptr, false);
    HeapProfilerInterface::Destroy(thread->GetEcmaVM());
}

/**
 * @tc.name: HeapProfiler_StopHeapTrackingWithSharedGCTest
 * @tc.desc: Test heap profiler stop tracking with shared GC. Verify that
           heap tracking can be stopped correctly after string interning and GC operations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, HeapProfiler_StopHeapTrackingWithSharedGCTest)
{
    SharedHeap *sharedHeap = SharedHeap::GetInstance();
    ASSERT_NE(sharedHeap, nullptr);

    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    HeapProfilerInterface *heapProfiler = HeapProfilerInterface::GetInstance(thread->GetEcmaVM());
    ASSERT_NE(heapProfiler, nullptr);

    for (int round = 0; round < 10; ++round) {
        std::string fileName = "test_stop_tracking_" + std::to_string(round) + ".heaptimeline";
        FileStream stream(fileName.c_str());
        heapProfiler->StartHeapTracking(50.0, true, &stream, false, false);

        InternMultipleUtf8Strings(stringTable, 100, "stop_tracking", round);

        sharedHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);

        InternMultipleUtf8Strings(stringTable, 100, "stop_before", round);

        heapProfiler->StopHeapTracking(&stream, nullptr, false);
    }

    HeapProfilerInterface::Destroy(thread->GetEcmaVM());
}

/**
 * @tc.name: SharedHeap_WaitAllTasksFinishedTest
 * @tc.desc: Test SharedHeap WaitAllTasksFinished functionality. Verify that
           all shared heap tasks complete correctly after GC and string interning.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, SharedHeap_WaitAllTasksFinishedTest)
{
    SharedHeap *sharedHeap = SharedHeap::GetInstance();
    ASSERT_NE(sharedHeap, nullptr);

    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    EcmaStringTableCleaner *cleaner = stringTable->GetCleaner();
    ASSERT_NE(cleaner, nullptr);

    for (int round = 0; round < 10; ++round) {
        cleaner->SetEnableConcurrentSweep(true);

        InternMultipleUtf8Strings(stringTable, 100, "wait_all", round);

        sharedHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);

        sharedHeap->WaitAllTasksFinished(thread);

        ASSERT_FALSE(stringTable->IsSweeping());

        cleaner->SetEnableConcurrentSweep(false);
    }
}

/**
 * @tc.name: SharedHeap_PrepareTest
 * @tc.desc: Test SharedHeap Prepare functionality. Verify that
           shared heap preparation works correctly with string table operations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, SharedHeap_PrepareTest)
{
    SharedHeap *sharedHeap = SharedHeap::GetInstance();
    ASSERT_NE(sharedHeap, nullptr);

    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    EcmaStringTableCleaner *cleaner = stringTable->GetCleaner();
    ASSERT_NE(cleaner, nullptr);

    for (int round = 0; round < 10; ++round) {
        cleaner->SetEnableConcurrentSweep(true);

        InternMultipleUtf8Strings(stringTable, 100, "prepare_test", round);

        sharedHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);

        sharedHeap->PrepareByJSThread(thread, false);

        ASSERT_FALSE(stringTable->IsSweeping());

        cleaner->SetEnableConcurrentSweep(false);
    }
}

/**
 * @tc.name: SharedHeap_PrepareByJSThreadTest
 * @tc.desc: Test SharedHeap PrepareByJSThread functionality. Verify that
           shared heap preparation via JSThread works correctly with string table.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, SharedHeap_PrepareByJSThreadTest)
{
    SharedHeap *sharedHeap = SharedHeap::GetInstance();
    ASSERT_NE(sharedHeap, nullptr);

    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    ASSERT_NE(stringTable, nullptr);

    EcmaStringTableCleaner *cleaner = stringTable->GetCleaner();
    ASSERT_NE(cleaner, nullptr);

    for (int round = 0; round < 10; ++round) {
        cleaner->SetEnableConcurrentSweep(true);

        InternMultipleUtf8Strings(stringTable, 100, "prepare_js", round);

        sharedHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);

        sharedHeap->PrepareByJSThread(thread, false);

        ASSERT_FALSE(stringTable->IsSweeping());

        cleaner->SetEnableConcurrentSweep(false);
    }
}

/**
 * @tc.name: HashTrieMap_ClearTest
 * @tc.desc: Test HashTrieMap Clear functionality. Verify that
           HashTrieMap can be cleared correctly after inserting entries.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, HashTrieMap_ClearTest)
{
    HashTrieMapTypeNormal hashTrieMap;
    HashTrieMapOperationNormalType operation(&hashTrieMap);
    HashTrieMapInUseScopeTypeNormal inUseScope(&hashTrieMap);

    for (int i = 0; i < 100; ++i) {
        char nameBuf[32];
        int ret = snprintf_s(nameBuf, sizeof(nameBuf), sizeof(nameBuf) - 1, "clear_test_%d", i);
        if (ret < 0) {
            LOG_ECMA(ERROR) << "snprintf_s failed in HashTrieMap_ClearTest";
            continue;
        }
        uint8_t utf8Data[32];
        errno_t memcpyRet = memcpy_s(utf8Data, sizeof(utf8Data), nameBuf, strlen(nameBuf));
        if (memcpyRet != EOK) {
            LOG_ECMA(ERROR) << "memcpy_s failed in HashTrieMap_ClearTest";
            continue;
        }
        uint32_t utf8Len = strlen(nameBuf);

        BaseString *result = LoadOrStoreUtf8String(operation, utf8Data, utf8Len);
        ASSERT_NE(result, nullptr);
    }

    bool hasNonEmptyRoot = false;
    for (uint32_t i = 0; i < TrieMapConfig::ROOT_SIZE; ++i) {
        auto root = hashTrieMap.GetRoot(i).load(std::memory_order_acquire);
        if (root != nullptr) {
            hasNonEmptyRoot = true;
            break;
        }
    }
    ASSERT_TRUE(hasNonEmptyRoot);

    hashTrieMap.Clear();

    for (uint32_t i = 0; i < TrieMapConfig::ROOT_SIZE; ++i) {
        auto root = hashTrieMap.GetRoot(i).load(std::memory_order_acquire);
        ASSERT_EQ(root, nullptr);
    }
}

/**
 * @tc.name: HashTrieMap_GetOrCreateRootTest
 * @tc.desc: Test HashTrieMap GetOrCreateRoot functionality. Verify that
           roots can be created and retrieved correctly.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, HashTrieMap_GetOrCreateRootTest)
{
    HashTrieMapType hashTrieMap;

    for (int round = 0; round < 100; ++round) {
        for (uint32_t i = 0; i < TrieMapConfig::ROOT_SIZE; ++i) {
            auto root = hashTrieMap.GetOrCreateRoot(i);
            ASSERT_NE(root, nullptr);
            ASSERT_EQ(root, hashTrieMap.GetRoot(i).load(std::memory_order_acquire));
        }

        hashTrieMap.Clear();
    }
}

/**
 * @tc.name: HashTrieMap_ClearNodeFromGC_NormalInsertUtf8
 * @tc.desc: Test ClearNodeFromGC with Normal mode insertion for UTF8 strings. Use Normal mode
 *           (NoSlotBarrierDynamic) HashTrieMapOperation to insert UTF8 strings via LoadOrStore,
 *           these entries have no ToSpace tag. Then use ConcurrentSweep mode (NeedSlotBarrier)
 *           HashTrieMapOperation to call ClearNodeFromGC.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, HashTrieMap_ClearNodeFromGC_NormalInsertUtf8Test)
{
    HashTrieMapType hashTrieMap;
    HashTrieMapInUseScopeType inUseScope(&hashTrieMap);

    HashTrieMapOperationNormalType normalOp(&hashTrieMap);

    for (int i = 0; i < 100; ++i) {
        char nameBuf[32];
        int ret = snprintf_s(nameBuf, sizeof(nameBuf), sizeof(nameBuf) - 1, "normal_insert_%d", i);
        if (ret < 0) {
            LOG_ECMA(ERROR) << "snprintf_s failed in NormalInsertTest";
            continue;
        }
        uint8_t utf8Data[32];
        errno_t memcpyRet = memcpy_s(utf8Data, sizeof(utf8Data), nameBuf, strlen(nameBuf));
        if (memcpyRet != EOK) {
            LOG_ECMA(ERROR) << "memcpy_s failed in NormalInsertTest";
            continue;
        }
        uint32_t utf8Len = strlen(nameBuf);

        BaseString *result = LoadOrStoreUtf8String(normalOp, utf8Data, utf8Len);
        ASSERT_NE(result, nullptr);
    }

    DoSweepingWithClearNodeKeepAll(hashTrieMap);

    hashTrieMap.Clear();
}

/**
 * @tc.name: HashTrieMap_ClearNodeFromGC_ConcurrentSweepInsertUtf8
 * @tc.desc: Test ClearNodeFromGC with ConcurrentSweep mode insertion for UTF8 strings. Start sweeping first,
 *           then use ConcurrentSweep mode (NeedSlotBarrier) HashTrieMapOperation to insert UTF8 strings
 *           via LoadOrStore. These entries are created with ToSpace tag during sweeping.
 *           Then call ClearNodeFromGC to clear these entries.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, HashTrieMap_ClearNodeFromGC_ConcurrentSweepInsertUtf8Test)
{
    HashTrieMapType hashTrieMap;
    HashTrieMapInUseScopeType inUseScope(&hashTrieMap);

    HashTrieMapOperationConcurrentSweepType concurrentSweepOp(&hashTrieMap);

    for (int i = 0; i < 100; ++i) {
        char nameBuf[32];
        int ret = snprintf_s(nameBuf, sizeof(nameBuf), sizeof(nameBuf) - 1, "concurrent_insert_%d", i);
        if (ret < 0) {
            LOG_ECMA(ERROR) << "snprintf_s failed in ConcurrentSweepInsertTest";
            continue;
        }
        uint8_t utf8Data[32];
        errno_t memcpyRet = memcpy_s(utf8Data, sizeof(utf8Data), nameBuf, strlen(nameBuf));
        if (memcpyRet != EOK) {
            LOG_ECMA(ERROR) << "memcpy_s failed in ConcurrentSweepInsertTest";
            continue;
        }
        uint32_t utf8Len = strlen(nameBuf);

        BaseString *result = LoadOrStoreUtf8String(concurrentSweepOp, utf8Data, utf8Len);
        ASSERT_NE(result, nullptr);
    }

    DoSweepingWithClearNodeKeepAll(hashTrieMap);

    hashTrieMap.Clear();
}

/**
 * @tc.name: HashTrieMap_ClearNodeFromGC_MixedInsertUtf8
 * @tc.desc: Test ClearNodeFromGC with mixed mode insertion for UTF8 strings. First use Normal mode
 *           (NoSlotBarrierDynamic) to insert UTF8 entries without ToSpace tag, then start sweeping and
 *           use ConcurrentSweep mode (NeedSlotBarrier) to insert UTF8 entries with ToSpace tag.
 *           Finally call ClearNodeFromGC.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, HashTrieMap_ClearNodeFromGC_MixedInsertUtf8Test)
{
    HashTrieMapType hashTrieMap;
    HashTrieMapInUseScopeType inUseScope(&hashTrieMap);

    HashTrieMapOperationNormalType normalOp(&hashTrieMap);

    for (int i = 0; i < 50; ++i) {
        char nameBuf[32];
        int ret = snprintf_s(nameBuf, sizeof(nameBuf), sizeof(nameBuf) - 1, "normal_mixed_%d", i);
        if (ret < 0) {
            LOG_ECMA(ERROR) << "snprintf_s failed in MixedInsertTest";
            continue;
        }
        uint8_t utf8Data[32];
        errno_t memcpyRet = memcpy_s(utf8Data, sizeof(utf8Data), nameBuf, strlen(nameBuf));
        if (memcpyRet != EOK) {
            LOG_ECMA(ERROR) << "memcpy_s failed in MixedInsertTest";
            continue;
        }
        uint32_t utf8Len = strlen(nameBuf);

        BaseString *result = LoadOrStoreUtf8String(normalOp, utf8Data, utf8Len);
        ASSERT_NE(result, nullptr);
    }

    HashTrieMapOperationConcurrentSweepType concurrentSweepOp(&hashTrieMap);

    for (int i = 50; i < 100; ++i) {
        char nameBuf[32];
        int ret = snprintf_s(nameBuf, sizeof(nameBuf), sizeof(nameBuf) - 1, "concurrent_mixed_%d", i);
        if (ret < 0) {
            LOG_ECMA(ERROR) << "snprintf_s failed in MixedInsertTest";
            continue;
        }
        uint8_t utf8Data[32];
        errno_t memcpyRet = memcpy_s(utf8Data, sizeof(utf8Data), nameBuf, strlen(nameBuf));
        if (memcpyRet != EOK) {
            LOG_ECMA(ERROR) << "memcpy_s failed in MixedInsertTest";
            continue;
        }
        uint32_t utf8Len = strlen(nameBuf);

        BaseString *result = LoadOrStoreUtf8String(concurrentSweepOp, utf8Data, utf8Len);
        ASSERT_NE(result, nullptr);
    }

    DoSweepingWithClearNodeKeepAll(hashTrieMap);

    hashTrieMap.Clear();
}

/**
 * @tc.name: HashTrieMap_ClearNodeFromGC_NormalInsertUtf16
 * @tc.desc: Test ClearNodeFromGC with Normal mode insertion for UTF16 strings. Use Normal mode
 *           (NoSlotBarrierDynamic) HashTrieMapOperation to insert UTF16 strings via LoadOrStore,
 *           these entries have no ToSpace tag. Then use ConcurrentSweep mode (NeedSlotBarrier)
 *           HashTrieMapOperation to call ClearNodeFromGC.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, HashTrieMap_ClearNodeFromGC_NormalInsertUtf16Test)
{
    HashTrieMapType hashTrieMap;
    HashTrieMapInUseScopeType inUseScope(&hashTrieMap);

    HashTrieMapOperationNormalType normalOp(&hashTrieMap);

    for (int i = 0; i < 100; ++i) {
        uint16_t utf16Data[16];
        uint32_t utf16Len = MakeUtf16DataFromAsciiWithNumber(utf16Data, "normal_utf16_", i);

        BaseString *result = LoadOrStoreUtf16String(normalOp, utf16Data, utf16Len);
        ASSERT_NE(result, nullptr);
    }

    DoSweepingWithClearNodeKeepAll(hashTrieMap);

    hashTrieMap.Clear();
}

/**
 * @tc.name: HashTrieMap_ClearNodeFromGC_ConcurrentSweepInsertUtf16
 * @tc.desc: Test ClearNodeFromGC with ConcurrentSweep mode insertion for UTF16 strings. Start sweeping first,
 *           then use ConcurrentSweep mode (NeedSlotBarrier) HashTrieMapOperation to insert UTF16 strings
 *           via LoadOrStore. These entries are created with ToSpace tag during sweeping.
 *           Then call ClearNodeFromGC to clear these entries.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, HashTrieMap_ClearNodeFromGC_ConcurrentSweepInsertUtf16Test)
{
    HashTrieMapType hashTrieMap;
    HashTrieMapInUseScopeType inUseScope(&hashTrieMap);

    HashTrieMapOperationConcurrentSweepType concurrentSweepOp(&hashTrieMap);

    for (int i = 0; i < 100; ++i) {
        uint16_t utf16Data[20];
        uint32_t utf16Len = MakeUtf16DataFromAsciiWithNumber(utf16Data, "concurrent_utf16_", i);

        BaseString *result = LoadOrStoreUtf16String(concurrentSweepOp, utf16Data, utf16Len);
        ASSERT_NE(result, nullptr);
    }

    DoSweepingWithClearNodeKeepAll(hashTrieMap);

    hashTrieMap.Clear();
}

/**
 * @tc.name: HashTrieMap_ClearNodeFromGC_MixedInsertUtf16
 * @tc.desc: Test ClearNodeFromGC with mixed mode insertion for UTF16 strings. First use Normal mode
 *           (NoSlotBarrierDynamic) to insert UTF16 entries without ToSpace tag, then start sweeping and
 *           use ConcurrentSweep mode (NeedSlotBarrier) to insert UTF16 entries with ToSpace tag.
 *           Finally call ClearNodeFromGC.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, HashTrieMap_ClearNodeFromGC_MixedInsertUtf16Test)
{
    HashTrieMapType hashTrieMap;
    HashTrieMapInUseScopeType inUseScope(&hashTrieMap);

    HashTrieMapOperationNormalType normalOp(&hashTrieMap);

    for (int i = 0; i < 50; ++i) {
        uint16_t utf16Data[16];
        uint32_t utf16Len = MakeUtf16DataFromAsciiWithNumber(utf16Data, "normal_mix_", i);

        BaseString *result = LoadOrStoreUtf16String(normalOp, utf16Data, utf16Len);
        ASSERT_NE(result, nullptr);
    }

    HashTrieMapOperationConcurrentSweepType concurrentSweepOp(&hashTrieMap);

    for (int i = 50; i < 100; ++i) {
        uint16_t utf16Data[20];
        uint32_t utf16Len = MakeUtf16DataFromAsciiWithNumber(utf16Data, "concurrent_mix_", i);

        BaseString *result = LoadOrStoreUtf16String(concurrentSweepOp, utf16Data, utf16Len);
        ASSERT_NE(result, nullptr);
    }

    DoSweepingWithClearNodeKeepAll(hashTrieMap);

    hashTrieMap.Clear();
}

/**
 * @tc.name: HashTrieMap_ClearNodeFromGC_NoAtomicMarkUtf8
 * @tc.desc: Test ClearNodeFromGC without AtomicMark for UTF8 strings. Use Normal mode (NoSlotBarrierDynamic)
 *           HashTrieMapOperation to insert UTF8 strings via LoadOrStore without calling AtomicMark
 *           on the string objects. The visitor returns nullptr to simulate unmarked objects
 *           being cleared during GC.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, HashTrieMap_ClearNodeFromGC_NoAtomicMarkUtf8Test)
{
    HashTrieMapType hashTrieMap;
    HashTrieMapInUseScopeType inUseScope(&hashTrieMap);

    HashTrieMapOperationNormalType normalOp(&hashTrieMap);

    for (int i = 0; i < 100; ++i) {
        char nameBuf[32];
        int ret = snprintf_s(nameBuf, sizeof(nameBuf), sizeof(nameBuf) - 1, "no_mark_utf8_%d", i);
        if (ret < 0) {
            LOG_ECMA(ERROR) << "snprintf_s failed in NoAtomicMarkUtf8Test";
            continue;
        }
        uint8_t utf8Data[32];
        errno_t memcpyRet = memcpy_s(utf8Data, sizeof(utf8Data), nameBuf, strlen(nameBuf));
        if (memcpyRet != EOK) {
            LOG_ECMA(ERROR) << "memcpy_s failed in NoAtomicMarkUtf8Test";
            continue;
        }
        uint32_t utf8Len = strlen(nameBuf);

        BaseString *result = LoadOrStoreUtf8String(normalOp, utf8Data, utf8Len);
        ASSERT_NE(result, nullptr);
    }

    DoSweepingWithClearNodeClearAll(hashTrieMap);

    hashTrieMap.Clear();
}

/**
 * @tc.name: HashTrieMap_ClearNodeFromGC_AllAtomicMarkUtf8
 * @tc.desc: Test ClearNodeFromGC with all entries AtomicMarked for UTF8 strings. Use Normal mode
 *           (NoSlotBarrierDynamic) HashTrieMapOperation to insert UTF8 strings via LoadOrStore,
 *           and call AtomicMark on all string objects. The visitor checks the mark bit and returns
 *           the object if marked, simulating all objects surviving during GC.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, HashTrieMap_ClearNodeFromGC_AllAtomicMarkUtf8Test)
{
    HashTrieMapType hashTrieMap;
    HashTrieMapInUseScopeType inUseScope(&hashTrieMap);

    HashTrieMapOperationNormalType normalOp(&hashTrieMap);

    hashTrieMap.GetMutex().Lock();
    for (int i = 0; i < 100; ++i) {
        char nameBuf[32];
        int ret = snprintf_s(nameBuf, sizeof(nameBuf), sizeof(nameBuf) - 1, "all_mark_utf8_%d", i);
        if (ret < 0) {
            LOG_ECMA(ERROR) << "snprintf_s failed in AllAtomicMarkUtf8Test";
            continue;
        }
        uint8_t utf8Data[32];
        errno_t memcpyRet = memcpy_s(utf8Data, sizeof(utf8Data), nameBuf, strlen(nameBuf));
        if (memcpyRet != EOK) {
            LOG_ECMA(ERROR) << "memcpy_s failed in AllAtomicMarkUtf8Test";
            continue;
        }
        uint32_t utf8Len = strlen(nameBuf);

        BaseString *result = LoadOrStoreUtf8StringWithAtomicMark<
            HashTrieMapOperationNormalType, false>(normalOp, utf8Data, utf8Len);
        ASSERT_NE(result, nullptr);
    }
    hashTrieMap.GetMutex().Unlock();

    DoSweepingWithClearNodeKeepAll(hashTrieMap);

    hashTrieMap.Clear();
}

/**
 * @tc.name: HashTrieMap_ClearNodeFromGC_PartialAtomicMarkUtf8
 * @tc.desc: Test ClearNodeFromGC with partial entries AtomicMarked for UTF8 strings. Use Normal mode
 *           (NoSlotBarrierDynamic) HashTrieMapOperation to insert UTF8 strings via LoadOrStore.
 *           Half of the entries are AtomicMarked and half are not. The visitor checks the mark bit,
 *           simulating partial objects surviving during GC while others are cleared.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, HashTrieMap_ClearNodeFromGC_PartialAtomicMarkUtf8Test)
{
    HashTrieMapType hashTrieMap;
    HashTrieMapInUseScopeType inUseScope(&hashTrieMap);

    HashTrieMapOperationNormalType normalOp(&hashTrieMap);

    hashTrieMap.GetMutex().Lock();
    for (int i = 0; i < 50; ++i) {
        char nameBuf[32];
        int ret = snprintf_s(nameBuf, sizeof(nameBuf), sizeof(nameBuf) - 1, "partial_yes_utf8_%d", i);
        if (ret < 0) {
            LOG_ECMA(ERROR) << "snprintf_s failed in PartialAtomicMarkUtf8Test";
            continue;
        }
        uint8_t utf8Data[32];
        errno_t memcpyRet = memcpy_s(utf8Data, sizeof(utf8Data), nameBuf, strlen(nameBuf));
        if (memcpyRet != EOK) {
            LOG_ECMA(ERROR) << "memcpy_s failed in PartialAtomicMarkUtf8Test";
            continue;
        }
        uint32_t utf8Len = strlen(nameBuf);

        BaseString *result = LoadOrStoreUtf8StringWithAtomicMark<
            HashTrieMapOperationNormalType, false>(normalOp, utf8Data, utf8Len);
        ASSERT_NE(result, nullptr);
    }

    for (int i = 50; i < 100; ++i) {
        char nameBuf[32];
        int ret = snprintf_s(nameBuf, sizeof(nameBuf), sizeof(nameBuf) - 1, "partial_no_utf8_%d", i);
        if (ret < 0) {
            LOG_ECMA(ERROR) << "snprintf_s failed in PartialAtomicMarkUtf8Test";
            continue;
        }
        uint8_t utf8Data[32];
        errno_t memcpyRet = memcpy_s(utf8Data, sizeof(utf8Data), nameBuf, strlen(nameBuf));
        if (memcpyRet != EOK) {
            LOG_ECMA(ERROR) << "memcpy_s failed in PartialAtomicMarkUtf8Test";
            continue;
        }
        uint32_t utf8Len = strlen(nameBuf);

        BaseString *result = LoadOrStoreUtf8String<
            HashTrieMapOperationNormalType, false>(normalOp, utf8Data, utf8Len);
        ASSERT_NE(result, nullptr);
    }
    hashTrieMap.GetMutex().Unlock();

    DoSweepingWithClearNodeKeepAll(hashTrieMap);

    hashTrieMap.Clear();
}

/**
 * @tc.name: HashTrieMap_ClearNodeFromGC_NoAtomicMarkUtf16
 * @tc.desc: Test ClearNodeFromGC without AtomicMark for UTF16 strings. Use Normal mode (NoSlotBarrierDynamic)
 *           HashTrieMapOperation to insert UTF16 strings via LoadOrStore without calling AtomicMark
 *           on the string objects. The visitor returns nullptr to simulate unmarked objects
 *           being cleared during GC.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, HashTrieMap_ClearNodeFromGC_NoAtomicMarkUtf16Test)
{
    HashTrieMapType hashTrieMap;
    HashTrieMapInUseScopeType inUseScope(&hashTrieMap);

    HashTrieMapOperationNormalType normalOp(&hashTrieMap);

    for (int i = 0; i < 100; ++i) {
        uint16_t utf16Data[8];
        uint32_t utf16Len = MakeUtf16DataFromAsciiWithNumber(utf16Data, "n", i);

        BaseString *result = LoadOrStoreUtf16String(normalOp, utf16Data, utf16Len);
        ASSERT_NE(result, nullptr);
    }

    DoSweepingWithClearNodeClearAll(hashTrieMap);

    hashTrieMap.Clear();
}

/**
 * @tc.name: HashTrieMap_ClearNodeFromGC_AllAtomicMarkUtf16
 * @tc.desc: Test ClearNodeFromGC with all entries AtomicMarked for UTF16 strings. Use Normal mode
 *           (NoSlotBarrierDynamic) HashTrieMapOperation to insert UTF16 strings via LoadOrStore,
 *           and call AtomicMark on all string objects. The visitor checks the mark bit and returns
 *           the object if marked, simulating all objects surviving during GC.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, HashTrieMap_ClearNodeFromGC_AllAtomicMarkUtf16Test)
{
    HashTrieMapType hashTrieMap;
    HashTrieMapInUseScopeType inUseScope(&hashTrieMap);

    HashTrieMapOperationNormalType normalOp(&hashTrieMap);

    hashTrieMap.GetMutex().Lock();
    for (int i = 0; i < 100; ++i) {
        uint16_t utf16Data[16];
        uint32_t utf16Len = MakeUtf16DataFromAsciiWithNumber(utf16Data, "all_", i);

        BaseString *result = LoadOrStoreUtf16StringWithAtomicMark<
            HashTrieMapOperationNormalType, false>(normalOp, utf16Data, utf16Len);
        ASSERT_NE(result, nullptr);
    }
    hashTrieMap.GetMutex().Unlock();

    DoSweepingWithClearNodeKeepAll(hashTrieMap);

    hashTrieMap.Clear();
}

/**
 * @tc.name: HashTrieMap_ClearNodeFromGC_PartialAtomicMarkUtf16
 * @tc.desc: Test ClearNodeFromGC with partial entries AtomicMarked for UTF16 strings. Use Normal mode
 *           (NoSlotBarrierDynamic) HashTrieMapOperation to insert UTF16 strings via LoadOrStore.
 *           Half of the entries are AtomicMarked and half are not. The visitor checks the mark bit,
 *           simulating partial objects surviving during GC while others are cleared.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableSweepingTest, HashTrieMap_ClearNodeFromGC_PartialAtomicMarkUtf16Test)
{
    HashTrieMapType hashTrieMap;
    HashTrieMapInUseScopeType inUseScope(&hashTrieMap);

    HashTrieMapOperationNormalType normalOp(&hashTrieMap);

    hashTrieMap.GetMutex().Lock();
    for (int i = 0; i < 50; ++i) {
        uint16_t utf16Data[24];
        uint32_t utf16Len = MakeUtf16DataFromAsciiWithNumber(utf16Data, "partial_yes_", i);

        BaseString *result = LoadOrStoreUtf16StringWithAtomicMark<
            HashTrieMapOperationNormalType, false>(normalOp, utf16Data, utf16Len);
        ASSERT_NE(result, nullptr);
    }

    for (int i = 50; i < 100; ++i) {
        uint16_t utf16Data[24];
        uint32_t utf16Len = MakeUtf16DataFromAsciiWithNumber(utf16Data, "partial_no_", i);

        BaseString *result = LoadOrStoreUtf16String<
            HashTrieMapOperationNormalType, false>(normalOp, utf16Data, utf16Len);
        ASSERT_NE(result, nullptr);
    }
    hashTrieMap.GetMutex().Unlock();

    DoSweepingWithClearNodeKeepAll(hashTrieMap);

    hashTrieMap.Clear();
}

}  // namespace panda::test
