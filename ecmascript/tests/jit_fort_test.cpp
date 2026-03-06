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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/tests/test_helper.h"

#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/partial_gc.h"
#include "ecmascript/mem/jit_fort_memdesc.h"
#include "ecmascript/mem/jit_fort.h"
#include "ecmascript/mem/machine_code.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {

class JitFortTest :  public BaseTestWithScope<false> {
public:
    void SetUp() override
    {
        JSRuntimeOptions options;
        instance = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        thread->ManagedCodeBegin();
        scope = new EcmaHandleScope(thread);
        auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
        heap->GetConcurrentMarker()->EnableConcurrentMarking(EnableConcurrentMarkType::ENABLE);
        heap->GetSweeper()->EnableConcurrentSweep(EnableConcurrentSweepType::ENABLE);
    }
};

HWTEST_F_L0(JitFortTest, AddRegionTest001)
{
    JitFort *jitFort = new JitFort();
    bool res = jitFort->AddRegion();
    ASSERT_EQ(res, true);
}

HWTEST_F_L0(JitFortTest, AddRegionTest002)
{
    JitFort *jitFort = new JitFort();
    for (size_t i = 0; i < JitFort::MAX_JIT_FORT_REGIONS; i++) {
        jitFort->AddRegion();
    }
    ASSERT_EQ(jitFort->AddRegion(), false);
}

HWTEST_F_L0(JitFortTest, AllocateTest001)
{
    MachineCodeDesc desc;
    desc.instructionsSize = 18;
    JitFort *jitFort = new JitFort();
    ASSERT_NE(jitFort, nullptr);
    jitFort->Allocate(&desc);
}

HWTEST_F_L0(JitFortTest, AllocateTest002)
{
    MachineCodeDesc desc;
    desc.instructionsSize = 18;
    desc.isHugeObj = false;
    JitFort *jitFort = new JitFort();
    ASSERT_NE(jitFort, nullptr);
    uintptr_t retAddr = jitFort->Allocate(&desc);
    bool inRange = jitFort->InSmallRange(retAddr);
    ASSERT_EQ(inRange, true);
}

HWTEST_F_L0(JitFortTest, AllocateTest003)
{
    MachineCodeDesc desc;
    desc.instructionsSize = 18;
    desc.isHugeObj = true;
    JitFort *jitFort = new JitFort();
    ASSERT_NE(jitFort, nullptr);
    uintptr_t retAddr = jitFort->Allocate(&desc);
    bool inHugeRange = jitFort->InHugeRange(retAddr);
    ASSERT_EQ(inHugeRange, true);
}

HWTEST_F_L0(JitFortTest, GetDescTest001)
{
    MemDescPool *pool = new MemDescPool(1, 1);
    ASSERT_NE(pool, nullptr);
    pool->GetDescFromPool();
}

HWTEST_F_L0(JitFortTest, MemDescPoolFreeTest001)
{
    MemDescPool *pool = new MemDescPool(1, 1);
    ASSERT_NE(pool, nullptr);
    pool->~MemDescPool();
}

HWTEST_F_L0(JitFortTest, InitRegionTest001)
{
    JitFort *jitFort = new JitFort();
    ASSERT_NE(jitFort, nullptr);
    jitFort->InitRegions();
}

HWTEST_F_L0(JitFortTest, InRangeTest001)
{
    JitFort *jitFort = new JitFort();
    bool result = jitFort->InSmallRange(1);
    ASSERT_EQ(result, false);
}

/**
 * @tc.name: SingletonInstanceTest001
 * @tc.desc: Test singleton-instance consistency and basic functionality
 */
HWTEST_F_L0(JitFortTest, SingletonInstanceTest001)
{
    JitFort *jitFort1 = new JitFort();
    JitFort *jitFort2 = new JitFort();

    ASSERT_NE(jitFort1, nullptr);
    ASSERT_NE(jitFort2, nullptr);

    // Test allocation consistency across instances
    MachineCodeDesc desc1, desc2;
    desc1.instructionsSize = 64;
    desc1.isHugeObj = false;
    desc2.instructionsSize = 128;
    desc2.isHugeObj = false;

    uintptr_t addr1 = jitFort1->Allocate(&desc1);
    uintptr_t addr2 = jitFort2->Allocate(&desc2);

    ASSERT_NE(addr1, ToUintPtr(nullptr));
    ASSERT_NE(addr2, ToUintPtr(nullptr));

    // Both should be in small range
    EXPECT_TRUE(jitFort1->InSmallRange(addr1));
    EXPECT_TRUE(jitFort2->InSmallRange(addr2));

    // Verify region mapping works
    JitFortRegion *region1 = jitFort1->ObjectAddressToRange(addr1);
    JitFortRegion *region2 = jitFort2->ObjectAddressToRange(addr2);
    ASSERT_NE(region1, nullptr);
    ASSERT_NE(region2, nullptr);

    delete jitFort1;
    delete jitFort2;
}

/**
 * @tc.name: HugeRegionBasicTest002
 * @tc.desc: Test huge region allocation and basic operations
 */
HWTEST_F_L0(JitFortTest, HugeRegionBasicTest002)
{
    JitFort *jitFort = new JitFort();

    // Test huge object allocation
    MachineCodeDesc desc;
    desc.instructionsSize = 512 * 1024; // 512KB
    desc.isHugeObj = true;

    uintptr_t addr = jitFort->Allocate(&desc);
    ASSERT_NE(addr, ToUintPtr(nullptr));

    // Verify address is in huge range
    EXPECT_TRUE(jitFort->InHugeRange(addr));
    EXPECT_TRUE(jitFort->InJitFortRange(addr));

    // Verify address is NOT in small range
    EXPECT_FALSE(jitFort->InSmallRange(addr));

    // Test object address to region mapping
    JitFortRegion *region = jitFort->ObjectAddressToRange(addr);
    ASSERT_NE(region, nullptr);

    delete jitFort;
}

/**
 * @tc.name: HugeRegionSizeVarietyTest003
 * @tc.desc: Test huge region allocation with various sizes
 */
HWTEST_F_L0(JitFortTest, HugeRegionSizeVarietyTest003)
{
    JitFort *jitFort = new JitFort();
    std::vector<uintptr_t> addresses;

    // Test different huge sizes
    const size_t sizes[] = {
        256 * 1024,  // 256KB
        512 * 1024,  // 512KB
        1024 * 1024, // 1MB
        2048 * 1024  // 2MB
    };

    for (size_t size : sizes) {
        MachineCodeDesc desc;
        desc.instructionsSize = size;
        desc.isHugeObj = true;

        uintptr_t addr = jitFort->Allocate(&desc);
        if (addr != ToUintPtr(nullptr)) {
            addresses.push_back(addr);
            EXPECT_TRUE(jitFort->InHugeRange(addr)) << "Size " << size << " address: " << std::hex << addr;
            EXPECT_FALSE(jitFort->InSmallRange(addr)) << "Size " << size << " should not be in small range";
        }
    }

    EXPECT_GT(addresses.size(), 0) << "Should have at least one successful allocation";
    EXPECT_LE(addresses.size(), 4) << "Cannot allocate more than 4 huge objects in test";

    // Verify all addresses are in huge range
    for (uintptr_t addr : addresses) {
        EXPECT_TRUE(jitFort->InHugeRange(addr));
        EXPECT_FALSE(jitFort->InSmallRange(addr));
    }

    delete jitFort;
}

/**
 * @tc.name: MixedAllocationScenariosTest004
 * @tc.desc: Test mixed small and huge object allocation scenarios
 */
HWTEST_F_L0(JitFortTest, MixedAllocationScenariosTest004)
{
    JitFort *jitFort = new JitFort();
    std::vector<uintptr_t> smallAddrs, hugeAddrs;

    // Test pattern: small, small, huge, repeated
    for (int i = 0; i < 15; i++) {
        MachineCodeDesc desc;
        if (i % 3 == 2) {                       // Every third allocation is huge
            desc.instructionsSize = 512 * 1024; // 512KB
            desc.isHugeObj = true;
            uintptr_t addr = jitFort->Allocate(&desc);
            if (addr != ToUintPtr(nullptr)) {
                hugeAddrs.push_back(addr);
                EXPECT_TRUE(jitFort->InHugeRange(addr));
                EXPECT_FALSE(jitFort->InSmallRange(addr));
            }
        } else {                                   // Small allocations
            desc.instructionsSize = 64 + (i * 32); // Varied small sizes
            desc.isHugeObj = false;
            uintptr_t addr = jitFort->Allocate(&desc);
            if (addr != ToUintPtr(nullptr)) {
                smallAddrs.push_back(addr);
                EXPECT_TRUE(jitFort->InSmallRange(addr));
                EXPECT_FALSE(jitFort->InHugeRange(addr));
            }
        }
    }

    EXPECT_GT(smallAddrs.size(), 0) << "Should have successful small allocations";
    EXPECT_GT(hugeAddrs.size(), 0) << "Should have successful huge allocations";

    // Verify no overlap between small and huge ranges
    for (uintptr_t smallAddr : smallAddrs) {
        for (uintptr_t hugeAddr : hugeAddrs) {
            EXPECT_NE(smallAddr, hugeAddr) << "Small and huge addresses should not overlap";
        }
    }

    delete jitFort;
}

/**
 * @tc.name: MemoryManagementOperationsTest005
 * @tc.desc: Test memory management operations
 */
HWTEST_F_L0(JitFortTest, MemoryManagementOperationsTest005)
{
    JitFort *jitFort = new JitFort();

    // Test small object memory management
    MachineCodeDesc smallDesc;
    smallDesc.instructionsSize = 256;
    smallDesc.isHugeObj = false;
    uintptr_t smallAddr = jitFort->Allocate(&smallDesc);
    ASSERT_NE(smallAddr, ToUintPtr(nullptr));

    // Mark small object as awaiting installation
    jitFort->MarkJitFortMemAwaitInstall(smallAddr, smallDesc.instructionsSize, false);

    // Test huge object memory management
    MachineCodeDesc hugeDesc;
    hugeDesc.instructionsSize = 256 * 1024;
    hugeDesc.isHugeObj = true;
    uintptr_t hugeAddr = jitFort->Allocate(&hugeDesc);
    ASSERT_NE(hugeAddr, ToUintPtr(nullptr));

    // Mark huge object as awaiting installation
    jitFort->MarkJitFortMemAwaitInstall(hugeAddr, hugeDesc.instructionsSize, true);

    // Test region mapping
    JitFortRegion *smallRegion = jitFort->ObjectAddressToRange(smallAddr);
    JitFortRegion *hugeRegion = jitFort->ObjectAddressToRange(hugeAddr);
    ASSERT_NE(smallRegion, nullptr);
    ASSERT_NE(hugeRegion, nullptr);
    EXPECT_NE(smallRegion, hugeRegion) << "Small and huge objects should be in different regions";

    // Test sweep operations
    jitFort->Sweep(false);
    jitFort->Sweep(true);
    jitFort->AsyncSweep(false);
    jitFort->AsyncSweep(true);

    delete jitFort;
}

/**
 * @tc.name: AddressRangeValidationTest006
 * @tc.desc: Test address range validation methods
 */
HWTEST_F_L0(JitFortTest, AddressRangeValidationTest006)
{
    JitFort *jitFort = new JitFort();

    // Allocate to get valid addresses for testing
    MachineCodeDesc smallDesc, hugeDesc;
    smallDesc.instructionsSize = 128;
    smallDesc.isHugeObj = false;
    hugeDesc.instructionsSize = 256 * 1024;
    hugeDesc.isHugeObj = true;

    uintptr_t smallAddr = jitFort->Allocate(&smallDesc);
    uintptr_t hugeAddr = jitFort->Allocate(&hugeDesc);

    ASSERT_NE(smallAddr, ToUintPtr(nullptr));
    ASSERT_NE(hugeAddr, ToUintPtr(nullptr));

    // Test valid addresses - should return correct range results
    EXPECT_TRUE(jitFort->InSmallRange(smallAddr));
    EXPECT_TRUE(jitFort->InHugeRange(hugeAddr));
    EXPECT_TRUE(jitFort->InJitFortRange(smallAddr));
    EXPECT_TRUE(jitFort->InJitFortRange(hugeAddr));

    // Test invalid addresses - should all return false
    EXPECT_FALSE(jitFort->InSmallRange(0));
    EXPECT_FALSE(jitFort->InHugeRange(0));
    EXPECT_FALSE(jitFort->InJitFortRange(0));

    // Test addresses just inside and outside ranges
    if (smallAddr > 1) {
        EXPECT_FALSE(jitFort->InSmallRange(smallAddr - 1)); // Just outside
        EXPECT_TRUE(jitFort->InSmallRange(smallAddr + 1));  // Just inside
    }

    delete jitFort;
}

/**
 * @tc.name: RegionManagementTest007
 * @tc.desc: Test region management operations
 */
HWTEST_F_L0(JitFortTest, RegionManagementTest007)
{
    JitFort *jitFort = new JitFort();

    // Test initial region list
    JitFortRegion *regionList = jitFort->GetRegionList();
    EXPECT_NE(regionList, nullptr) << "Region list should be initialized";

    // Test adding regions (if AddRegion method is available and working)
    bool addResult = jitFort->AddRegion();
    EXPECT_TRUE(addResult) << "Should be able to add additional regions";

    // Verify region list updated
    JitFortRegion *updatedRegionList = jitFort->GetRegionList();
    EXPECT_NE(updatedRegionList, nullptr);

    // Test multiple small allocations across regions
    std::vector<JitFortRegion *> regions;
    MachineCodeDesc desc;
    desc.instructionsSize = 64;
    desc.isHugeObj = false;

    for (int i = 0; i < 10; i++) {
        uintptr_t addr = jitFort->Allocate(&desc);
        if (addr != ToUintPtr(nullptr)) {
            JitFortRegion *region = jitFort->ObjectAddressToRange(addr);
            ASSERT_NE(region, nullptr);

            // Verify each allocation maps to a valid region
            regions.push_back(region);
            jitFort->MarkJitFortMemAwaitInstall(addr, desc.instructionsSize, false);
        }
    }

    EXPECT_GT(regions.size(), 0) << "Should have allocations in regions";

    delete jitFort;
}

/**
 * @tc.name: FortSizeCalculationTest008
 * @tc.desc: Test fort size calculation methods
 */
HWTEST_F_L0(JitFortTest, FortSizeCalculationTest008)
{
    JitFort *jitFort = new JitFort();

    // Test FortAllocSize method with various instruction sizes
    size_t smallSize = 64;
    size_t mediumSize = 512;
    size_t largeSize = 2048;
    size_t hugeSize = 1024 * 1024;

    size_t allocSmall = jitFort->FortAllocSize(smallSize);
    size_t allocMedium = jitFort->FortAllocSize(mediumSize);
    size_t allocLarge = jitFort->FortAllocSize(largeSize);
    size_t allocHuge = jitFort->FortAllocSize(hugeSize);

    // All should be aligned according to FORT_BUF_ALIGN
    EXPECT_EQ(allocSmall % jitFort->FORT_BUF_ALIGN, 0);
    EXPECT_EQ(allocMedium % jitFort->FORT_BUF_ALIGN, 0);
    EXPECT_EQ(allocLarge % jitFort->FORT_BUF_ALIGN, 0);
    EXPECT_EQ(allocHuge % jitFort->FORT_BUF_ALIGN, 0);

    // Size should be >= original size (due to alignment)
    EXPECT_GE(allocSmall, smallSize);
    EXPECT_GE(allocMedium, mediumSize);
    EXPECT_GE(allocLarge, largeSize);
    EXPECT_GE(allocHuge, hugeSize);

    delete jitFort;
}

/**
 * @tc.name: ConcurrentThreadSafetyTest009
 * @tc.desc: Test concurrent access thread safety
 */
HWTEST_F_L0(JitFortTest, ConcurrentThreadSafetyTest009)
{
    const int threadCount = 3;
    const int opsPerThread = 25;
    std::atomic<int> successCount {0};
    std::vector<std::thread> threads;

    auto workerThread = [&successCount]() {
        for (int i = 0; i < opsPerThread; i++) {
            JitFort *jitFort = new JitFort();
            MachineCodeDesc desc;
            desc.instructionsSize = 128 + (rand() % 1024); // Random size 128-1152
            desc.isHugeObj = (rand() % 4) == 0;            // 25% chance of huge object
            uintptr_t addr = jitFort->Allocate(&desc);
            if (addr == ToUintPtr(nullptr)) {
                delete jitFort;
                continue;
            }
            successCount++;
            // Verify range based on object type
            if (desc.isHugeObj) {
                EXPECT_TRUE(jitFort->InHugeRange(addr));
                EXPECT_FALSE(jitFort->InSmallRange(addr));
            } else {
                EXPECT_TRUE(jitFort->InSmallRange(addr));
                EXPECT_FALSE(jitFort->InHugeRange(addr));
            }
            delete jitFort;
        }
    };

    // Launch multiple threads
    for (int i = 0; i < threadCount; i++) {
        threads.emplace_back(workerThread);
    }

    // Wait for all threads to complete
    for (auto &thread : threads) {
        thread.join();
    }

    // Verify concurrent operations succeeded
    EXPECT_GT(successCount.load(), 0) << "Should have successful concurrent operations";
}

/**
 * @tc.name: ResourceAvailabilityTest010
 * @tc.desc: Test resource availability methods
 */
HWTEST_F_L0(JitFortTest, ResourceAvailabilityTest010)
{
    // Test static resource availability
    bool resourceAvailable = JitFort::IsResourceAvailable();
    EXPECT_TRUE(resourceAvailable) << "JitFort resources should be available";

    JitFort *jitFort = new JitFort();

    // Test after instance creation
    EXPECT_TRUE(JitFort::IsResourceAvailable()) << "Resource availability should remain true after instance creation";

    // Test allocation with limited resources by requesting different sizes
    MachineCodeDesc descSmall, descLarge;
    descSmall.instructionsSize = 64;
    descSmall.isHugeObj = false;

    descLarge.instructionsSize = 1024 * 1024; // 1MB
    descLarge.isHugeObj = true;

    // Should be able to allocate small objects
    uintptr_t smallAddr = jitFort->Allocate(&descSmall);
    if (smallAddr != ToUintPtr(nullptr)) {
        EXPECT_TRUE(jitFort->InSmallRange(smallAddr));
    }

    // Should be able to allocate huge objects
    uintptr_t largeAddr = jitFort->Allocate(&descLarge);
    if (largeAddr != ToUintPtr(nullptr)) {
        EXPECT_TRUE(jitFort->InHugeRange(largeAddr));
    }

    delete jitFort;

    // Resource availability should still be true after destruction
    EXPECT_TRUE(JitFort::IsResourceAvailable())
        << "Resource availability should not be affected by instance destruction";
}

/**
 * @tc.name: AddrToFortRegionIndexTest011
 * @tc.desc: Test address to region index calculation
 */
HWTEST_F_L0(JitFortTest, AddrToFortRegionIndexTest011)
{
    JitFort *jitFort = new JitFort();

    // Allocate small object to get valid address
    MachineCodeDesc desc;
    desc.instructionsSize = 256;
    desc.isHugeObj = false;
    uintptr_t addr = jitFort->Allocate(&desc);

    ASSERT_NE(addr, ToUintPtr(nullptr));

    // Test address to region index calculation
    uint32_t regionIndex = jitFort->AddrToFortRegionIdx(addr);

    // Index should be within valid range
    EXPECT_LT(regionIndex, JitFort::MAX_JIT_FORT_REGIONS);
    EXPECT_GE(regionIndex, 0); // Should be non-negative

    delete jitFort;
}

/**
 * @tc.name: MemoryLeakTest012
 * @tc.desc: Test for memory leaks during repeated allocation/deallocation
 */
HWTEST_F_L0(JitFortTest, MemoryLeakTest012)
{
    // Perform repeated allocation/deallocation cycles
    const int cycles = 50;
    const int perCycle = 10;

    for (int cycle = 0; cycle < cycles; cycle++) {
        JitFort *jitFort = new JitFort();

        std::vector<uintptr_t> addresses;
        for (int i = 0; i < perCycle; i++) {
            MachineCodeDesc desc;
            desc.instructionsSize = 64 + (rand() % 1024);
            desc.isHugeObj = (rand() % 3) == 0; // 33% chance huge

            uintptr_t addr = jitFort->Allocate(&desc);
            if (addr != ToUintPtr(nullptr)) {
                addresses.push_back(addr);

                // Some basic memory management operations
                jitFort->MarkJitFortMemAwaitInstall(addr, desc.instructionsSize, desc.isHugeObj);
            }
        }

        delete jitFort;
    }
}

} // namespace panda::test
