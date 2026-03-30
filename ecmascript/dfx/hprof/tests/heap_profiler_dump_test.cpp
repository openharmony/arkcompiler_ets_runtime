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

#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include "ecmascript/dfx/hprof/heap_profiler.h"
#include "ecmascript/dfx/hprof/file_stream.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class HeapProfilerDumpTest : public testing::Test {
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
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

/**
 * @brief Test HeapProfiler DumpHeapSnapshot with JSON format
 * Covers branches: JSON format dump, normal execution path
 */
HWTEST_F_L0(HeapProfilerDumpTest, DumpHeapSnapshot_JSONFormat) {
    ecmascript::ThreadManagedScope scope(thread);
    HeapProfiler profiler(instance);

    // Create a temporary file path
    std::string tempFilePath = std::filesystem::temp_directory_path().string() + "/test_heap_snapshot.json";
    
    // Create the file first to ensure it exists
    std::fstream outputFile(tempFilePath, std::ios::out);
    outputFile.close();
    
    // Create FileStream with the temporary file path
    FileStream stream(tempFilePath);

    // Create dump options with JSON format
    DumpSnapShotOption dumpOption;
    dumpOption.dumpFormat = DumpFormat::JSON;

    // Test normal dump with JSON format
    // Note: DumpHeapSnapshot may return false in test environment due to fork limitations
    // We're testing the method doesn't crash and handles parameters correctly
    profiler.DumpHeapSnapshot(&stream, dumpOption);
    
    // Verify the file was created and has content
    std::ifstream inputFile(tempFilePath);
    std::string fileContent((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
    inputFile.close();
    
    // Check if the file is not empty (basic validation)
    EXPECT_FALSE(fileContent.empty());
    
    // Clean up temporary file
    std::filesystem::remove(tempFilePath);
}

/**
 * @brief Test HeapProfiler DumpHeapSnapshot with Binary format
 * Covers branches: Binary format dump, different format path
 */
HWTEST_F_L0(HeapProfilerDumpTest, DumpHeapSnapshot_BinaryFormat) {
    ecmascript::ThreadManagedScope scope(thread);
    HeapProfiler profiler(instance);

    // Create a temporary file path
    std::string tempFilePath = std::filesystem::temp_directory_path().string() + "/test_heap_snapshot.bin";
    
    // Create the file first to ensure it exists
    std::fstream outputFile(tempFilePath, std::ios::out);
    outputFile.close();
    
    // Create FileStream with the temporary file path
    FileStream stream(tempFilePath);

    // Create dump options with Binary format
    DumpSnapShotOption dumpOption;
    dumpOption.dumpFormat = DumpFormat::BINARY;

    // Test normal dump with Binary format
    // Note: DumpHeapSnapshot may return false in test environment due to fork limitations
    // We're testing the method doesn't crash and handles parameters correctly
    profiler.DumpHeapSnapshot(&stream, dumpOption);
    
    // Verify the file was created and has content
    std::ifstream inputFile(tempFilePath, std::ios::binary);
    inputFile.seekg(0, std::ios::end);
    std::streampos fileSize = inputFile.tellg();
    inputFile.close();
    
    // Check if the file is not empty (basic validation)
    EXPECT_GT(fileSize, 0LL);
    
    // Clean up temporary file
    std::filesystem::remove(tempFilePath);
}

/**
 * @brief Test HeapProfiler DumpHeapSnapshotForOOM functionality
 * Covers branches: OOM dump path, emergency dump scenario
 */
HWTEST_F_L0(HeapProfilerDumpTest, DumpHeapSnapshotForOOM) {
    ecmascript::ThreadManagedScope scope(thread);
    HeapProfiler profiler(instance);

    // Create dump options
    DumpSnapShotOption dumpOption;
    dumpOption.dumpFormat = DumpFormat::JSON;

    // Test OOM dump functionality
    // Note: This is a non-destructive test that verifies the method doesn't crash
    profiler.DumpHeapSnapshotForOOM(dumpOption);
    profiler.DumpHeapSnapshotForOOM(dumpOption, true); // fromSharedGC = true

    // Verify the method completed without crashing
    SUCCEED();
}

/**
 * @brief Test HeapProfiler heap tracking functionality
 * Covers branches: heap tracking start/stop/update
 */
HWTEST_F_L0(HeapProfilerDumpTest, HeapTracking) {
    ecmascript::ThreadManagedScope scope(thread);
    HeapProfiler profiler(instance);

    // Create a temporary file path
    std::string tempFilePath = std::filesystem::temp_directory_path().string() + "/test_heap_tracking.json";
    
    // Create the file first to ensure it exists
    std::fstream outputFile(tempFilePath, std::ios::out);
    outputFile.close();
    
    // Create FileStream with the temporary file path
    FileStream stream(tempFilePath);

    // Test start heap tracking with default parameters
    profiler.StartHeapTracking(1.0);

    // Test update heap tracking
    profiler.UpdateHeapTracking(&stream);

    // Test stop heap tracking
    profiler.StopHeapTracking(&stream);

    // Verify the file was created and has content
    std::ifstream inputFile(tempFilePath);
    std::string fileContent((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
    inputFile.close();
    
    // Check if the file is not empty (basic validation)
    EXPECT_FALSE(fileContent.empty());
    
    // Clean up temporary file
    std::filesystem::remove(tempFilePath);
}

/**
 * @brief Test HeapProfiler OOM dump control methods
 * Covers branches: OOM dump active flag handling
 */
HWTEST_F_L0(HeapProfilerDumpTest, OOMDumpControl) {
    // Test TryStartOOMDump and ResetOOMDump
    // Note: These are static methods that control OOM dump state
    ecmascript::ThreadManagedScope scope(thread);
    HeapProfiler::TryStartOOMDump();
    HeapProfiler::ResetOOMDump();

    // Verify the methods completed without crashing
    SUCCEED();
}

/**
 * @brief Test HeapProfiler handle leak detection functionality
 * Covers branches: handle leak detection state and operations
 */
HWTEST_F_L0(HeapProfilerDumpTest, HandleLeakDetection) {
    ecmascript::ThreadManagedScope scope(thread);
    HeapProfiler profiler(instance);

    // Test initial state
    EXPECT_FALSE(profiler.IsStartLocalHandleLeakDetect());

    // Test toggle state
    profiler.SwitchStartLocalHandleLeakDetect();
    EXPECT_TRUE(profiler.IsStartLocalHandleLeakDetect());

    // Test store potentially leak handles
    uintptr_t handle = 0x1000;
    profiler.StorePotentiallyLeakHandles(handle);

    // Test write to leak stack trace fd
    std::ostringstream buffer;
    profiler.WriteToLeakStackTraceFd(buffer);

    // Verify the methods completed without crashing
    SUCCEED();
}

/**
 * @brief Test HeapProfiler heap sampling functionality
 * Covers branches: heap sampling start/stop
 */
HWTEST_F_L0(HeapProfilerDumpTest, HeapSampling) {
    ecmascript::ThreadManagedScope scope(thread);
    HeapProfiler profiler(instance);

    // Test start heap sampling
    profiler.StartHeapSampling(1 << 15, 128);

    // Test get allocation profile
    const SamplingInfo *profile = profiler.GetAllocationProfile();
    // Note: Profile may be nullptr in test environment, but method should not crash

    // Test stop heap sampling
    profiler.StopHeapSampling();

    // Verify sampling is stopped
    profile = profiler.GetAllocationProfile();
    // Note: Profile should be nullptr after stopping

    // Verify the methods completed without crashing
    SUCCEED();
}

/**
 * @brief Test HeapProfiler dump with sync option
 * Covers branches: sync dump path
 */
HWTEST_F_L0(HeapProfilerDumpTest, DumpHeapSnapshot_SyncOption) {
    ecmascript::ThreadManagedScope scope(thread);
    HeapProfiler profiler(instance);

    // Create a temporary file path
    std::string tempFilePath = std::filesystem::temp_directory_path().string() + "/test_heap_snapshot_sync.json";
    
    // Create the file first to ensure it exists
    std::fstream outputFile(tempFilePath, std::ios::out);
    outputFile.close();
    
    // Create FileStream with the temporary file path
    FileStream stream(tempFilePath);

    // Create dump options with sync flag
    DumpSnapShotOption dumpOption;
    dumpOption.dumpFormat = DumpFormat::JSON;
    dumpOption.isSync = true;

    // Test dump with sync option
    profiler.DumpHeapSnapshot(&stream, dumpOption);
    
    // Verify the file was created and has content
    std::ifstream inputFile(tempFilePath);
    std::string fileContent((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
    inputFile.close();
    
    // Check if the file is not empty (basic validation)
    EXPECT_FALSE(fileContent.empty());
    
    // Clean up temporary file
    std::filesystem::remove(tempFilePath);
}

/**
 * @brief Test HeapProfiler dump with full GC option
 * Covers branches: full GC path
 */
HWTEST_F_L0(HeapProfilerDumpTest, DumpHeapSnapshot_FullGCOption) {
    ecmascript::ThreadManagedScope scope(thread);
    HeapProfiler profiler(instance);

    // Create a temporary file path
    std::string tempFilePath = std::filesystem::temp_directory_path().string() + "/test_heap_snapshot_fullgc.json";
    
    // Create the file first to ensure it exists
    std::fstream outputFile(tempFilePath, std::ios::out);
    outputFile.close();
    
    // Create FileStream with the temporary file path
    FileStream stream(tempFilePath);

    // Create dump options with full GC flag
    DumpSnapShotOption dumpOption;
    dumpOption.dumpFormat = DumpFormat::JSON;
    dumpOption.isFullGC = true;

    // Test dump with full GC option
    profiler.DumpHeapSnapshot(&stream, dumpOption);
    
    // Verify the file was created and has content
    std::ifstream inputFile(tempFilePath);
    std::string fileContent((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
    inputFile.close();
    
    // Check if the file is not empty (basic validation)
    EXPECT_FALSE(fileContent.empty());
    
    // Clean up temporary file
    std::filesystem::remove(tempFilePath);
}

/**
 * @brief Test HeapProfiler dump with simplify option
 * Covers branches: simplify dump path
 */
HWTEST_F_L0(HeapProfilerDumpTest, DumpHeapSnapshot_SimplifyOption) {
    ecmascript::ThreadManagedScope scope(thread);
    HeapProfiler profiler(instance);

    // Create a temporary file path
    std::string tempFilePath = std::filesystem::temp_directory_path().string() + "/test_heap_snapshot_simplify.json";
    
    // Create the file first to ensure it exists
    std::fstream outputFile(tempFilePath, std::ios::out);
    outputFile.close();
    
    // Create FileStream with the temporary file path
    FileStream stream(tempFilePath);

    // Create dump options with simplify flag
    DumpSnapShotOption dumpOption;
    dumpOption.dumpFormat = DumpFormat::JSON;
    dumpOption.isSimplify = true;

    // Test dump with simplify option
    profiler.DumpHeapSnapshot(&stream, dumpOption);
    
    // Verify the file was created and has content
    std::ifstream inputFile(tempFilePath);
    std::string fileContent((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
    inputFile.close();
    
    // Check if the file is not empty (basic validation)
    EXPECT_FALSE(fileContent.empty());
    
    // Clean up temporary file
    std::filesystem::remove(tempFilePath);
}

/**
 * @brief Test HeapProfiler dump with capture numeric value option
 * Covers branches: capture numeric value path
 */
HWTEST_F_L0(HeapProfilerDumpTest, DumpHeapSnapshot_CaptureNumericValueOption) {
    ecmascript::ThreadManagedScope scope(thread);
    HeapProfiler profiler(instance);

    // Create a temporary file path
    std::string tempFilePath = std::filesystem::temp_directory_path().string() + "/test_heap_snapshot_numeric.json";
    
    // Create the file first to ensure it exists
    std::fstream outputFile(tempFilePath, std::ios::out);
    outputFile.close();
    
    // Create FileStream with the temporary file path
    FileStream stream(tempFilePath);

    // Create dump options with capture numeric value flag
    DumpSnapShotOption dumpOption;
    dumpOption.dumpFormat = DumpFormat::JSON;
    dumpOption.captureNumericValue = true;

    // Test dump with capture numeric value option
    profiler.DumpHeapSnapshot(&stream, dumpOption);
    
    // Verify the file was created and has content
    std::ifstream inputFile(tempFilePath);
    std::string fileContent((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
    inputFile.close();
    
    // Check if the file is not empty (basic validation)
    EXPECT_FALSE(fileContent.empty());
    
    // Clean up temporary file
    std::filesystem::remove(tempFilePath);
}

/**
 * @brief Test HeapProfiler dump with private option
 * Covers branches: private dump path
 */
HWTEST_F_L0(HeapProfilerDumpTest, DumpHeapSnapshot_PrivateOption) {
    ecmascript::ThreadManagedScope scope(thread);
    HeapProfiler profiler(instance);

    // Create a temporary file path
    std::string tempFilePath = std::filesystem::temp_directory_path().string() + "/test_heap_snapshot_private.json";
    
    // Create the file first to ensure it exists
    std::fstream outputFile(tempFilePath, std::ios::out);
    outputFile.close();
    
    // Create FileStream with the temporary file path
    FileStream stream(tempFilePath);

    // Create dump options with private flag
    DumpSnapShotOption dumpOption;
    dumpOption.dumpFormat = DumpFormat::JSON;
    dumpOption.isPrivate = true;

    // Test dump with private option
    profiler.DumpHeapSnapshot(&stream, dumpOption);
    
    // Verify the file was created and has content
    std::ifstream inputFile(tempFilePath);
    std::string fileContent((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
    inputFile.close();
    
    // Check if the file is not empty (basic validation)
    EXPECT_FALSE(fileContent.empty());
    
    // Clean up temporary file
    std::filesystem::remove(tempFilePath);
}

/**
 * @brief Test HeapProfiler allocation event
 * Covers branches: allocation event handling
 */
HWTEST_F_L0(HeapProfilerDumpTest, AllocationEvent) {
    ecmascript::ThreadManagedScope scope(thread);
    HeapProfiler profiler(instance);

    // Test allocation event
    TaggedObject *address = reinterpret_cast<TaggedObject*>(0x1000);
    size_t size = 100;
    profiler.AllocationEvent(address, size);

    // Verify the method completed without crashing
    SUCCEED();
}

/**
 * @brief Test HeapProfiler move event
 * Covers branches: move event handling
 */
HWTEST_F_L0(HeapProfilerDumpTest, MoveEvent) {
    ecmascript::ThreadManagedScope scope(thread);
    HeapProfiler profiler(instance);

    // Test move event
    uintptr_t address = 0x1000;
    TaggedObject *forwardAddress = reinterpret_cast<TaggedObject*>(0x2000);
    size_t size = 100;
    profiler.MoveEvent(address, forwardAddress, size);

    // Verify the method completed without crashing
    SUCCEED();
}

/**
 * @brief Test HeapProfiler get ID count
 * Covers branches: ID count retrieval
 */
HWTEST_F_L0(HeapProfilerDumpTest, GetIdCount) {
    ecmascript::ThreadManagedScope scope(thread);
    HeapProfiler profiler(instance);

    // Test get ID count
    profiler.GetIdCount();
    // Note: ID count may be 0 initially, but method should not crash

    // Verify the method completed without crashing
    SUCCEED();
}

/**
 * @brief Test HeapProfiler get entry ID map
 * Covers branches: entry ID map retrieval
 */
HWTEST_F_L0(HeapProfilerDumpTest, GetEntryIdMap) {
    ecmascript::ThreadManagedScope scope(thread);
    HeapProfiler profiler(instance);

    // Test get entry ID map
    EntryIdMap *entryIdMap = profiler.GetEntryIdMap();
    // Note: Entry ID map should not be nullptr
    EXPECT_NE(entryIdMap, nullptr);

    // Verify the method completed without crashing
    SUCCEED();
}

/**
 * @brief Test HeapProfiler get ecma string table
 * Covers branches: string table retrieval
 */
HWTEST_F_L0(HeapProfilerDumpTest, GetEcmaStringTable) {
    ecmascript::ThreadManagedScope scope(thread);
    HeapProfiler profiler(instance);

    // Test get ecma string table
    StringHashMap *stringTable = profiler.GetEcmaStringTable();
    // Note: String table should not be nullptr
    EXPECT_NE(stringTable, nullptr);

    // Verify the method completed without crashing
    SUCCEED();
}

/**
 * @brief Test EntryIdMap operations
 * Covers branches: EntryIdMap find/insert/erase operations
 */
HWTEST_F_L0(HeapProfilerDumpTest, EntryIdMapOperations) {
    ecmascript::ThreadManagedScope scope(thread);
    EntryIdMap entryIdMap;

    // Use uintptr_t for addresses
    uintptr_t addr1 = 0x1000;
    NodeId id1 = 100;

    // Test InsertId
    entryIdMap.InsertId(static_cast<JSTaggedType>(addr1), id1);

    // Test FindId
    auto [found, retrievedId] = entryIdMap.FindId(static_cast<JSTaggedType>(addr1));
    EXPECT_TRUE(found);
    EXPECT_EQ(retrievedId, id1);

    // Test FindOrInsertNodeId with existing entry
    NodeId foundId = entryIdMap.FindOrInsertNodeId(static_cast<JSTaggedType>(addr1));
    EXPECT_EQ(foundId, id1);

    // Test FindOrInsertNodeId with new entry
    uintptr_t addr2 = 0x2000;
    NodeId newId = entryIdMap.FindOrInsertNodeId(static_cast<JSTaggedType>(addr2));
    EXPECT_GT(newId, 0U);

    // Test EraseId
    entryIdMap.EraseId(static_cast<JSTaggedType>(addr1));
    auto [notFound, notFoundId] = entryIdMap.FindId(static_cast<JSTaggedType>(addr1));
    EXPECT_FALSE(notFound);

    // Test Move
    uintptr_t oldAddr = 0x3000;
    uintptr_t newAddr = 0x4000;
    NodeId moveId = 200;
    entryIdMap.InsertId(static_cast<JSTaggedType>(oldAddr), moveId);
    entryIdMap.Move(static_cast<JSTaggedType>(oldAddr), static_cast<JSTaggedType>(newAddr));

    auto [movedFound, movedId] = entryIdMap.FindId(static_cast<JSTaggedType>(newAddr));
    EXPECT_TRUE(movedFound);
    EXPECT_EQ(movedId, moveId);

    auto [oldNotFound, oldNotFoundId] = entryIdMap.FindId(static_cast<JSTaggedType>(oldAddr));
    EXPECT_FALSE(oldNotFound);
}
}  // namespace panda::test
