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

#include <cstdio>
#include <fstream>
#include <fcntl.h>
#include <sstream>

#include "ecmascript/dfx/hprof/heap_profiler_interface.h"
#include "ecmascript/js_object.h"
#include "ecmascript/dfx/hprof/heap_profiler.h"
#include "ecmascript/dfx/hprof/heap_snapshot_json_serializer.h"
#include "ecmascript/dfx/hprof/heap_snapshot.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/global_env.h"

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/dfx/hprof/file_stream.h"

using namespace panda::ecmascript;

namespace panda::ecmascript {
class TestProgress : public Progress {
public:
    TestProgress() = default;
    ~TestProgress() = default;

    void ReportProgress([[maybe_unused]] int32_t done, [[maybe_unused]] int32_t total) override {}
};

class TestStream : public Stream {
public:
    TestStream() = default;
    ~TestStream() = default;

    void EndOfStream() override {}
    int GetSize() override
    {
        static const int HEAP_PROFILER_CHUNK_SIZE = 100_KB;
        return HEAP_PROFILER_CHUNK_SIZE;
    }
    bool WriteChunk([[maybe_unused]] char *data, [[maybe_unused]] int32_t size) override
    {
        return true;
    }
    bool WriteBinBlock(char *data, int32_t size) override
    {
        return WriteChunk(data, size);
    }
    bool Good() override
    {
        return testStream_.good();
    }

    void UpdateHeapStats([[maybe_unused]] HeapStat* updateData, [[maybe_unused]] int32_t count) override
    {
    }

    void UpdateLastSeenObjectId([[maybe_unused]] int32_t lastSeenObjectId, [[maybe_unused]]int64_t timeStampUs) override
    {
    }

    void Clear()
    {
        testStream_.clear(std::ios::badbit);
    }

private:
    std::fstream testStream_;
};
}

namespace panda::test {
class HeapTrackerTest : public testing::Test {
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
        instance->SetEnableForceGC(false);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

// ---------------------------------------------------------------------------
// Helpers for JSWrappedNapiObject native-reference scenario
struct MockNativeRef {
    void *data;
};

static void *MockGetNativeReferenceData(void *ref)
{
    return reinterpret_cast<MockNativeRef *>(ref)->data;
}
// ---------------------------------------------------------------------------

HWTEST_F_L0(HeapTrackerTest, HeapTracker)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    HeapProfilerInterface *heapProfile = HeapProfilerInterface::GetInstance(instance);
    heapProfile->StartHeapTracking(50);
    sleep(1);
    int count = 100;
    while (count-- > 0) {
        instance->GetFactory()->NewJSAsyncFuncObject();
    }
    sleep(1);
    count = 100;
    while (count-- > 0) {
        instance->GetFactory()->NewJSSymbol();
    }
    sleep(1);
    count = 100;
    while (count-- > 0) {
        JSHandle<JSTaggedValue> undefined = instance->GetJSThread()->GlobalConstants()->GetHandledUndefined();
        JSHandle<EcmaString> string = instance->GetFactory()->NewFromASCII("Hello World");
        instance->GetFactory()->NewJSString(JSHandle<JSTaggedValue>(string), undefined);
    }

    // Scenario: JSWrappedNapiObject is NOT referenced by any root, and the
    // MockNativeRef it points to is deleted before the snapshot is taken.
    // After FULL_GC the unreachable object is collected; the final snapshot
    // must NOT contain the dangling native address.
    static int wrappedNativeObj = 0xABCD;
    MockNativeRef *mockRef = new MockNativeRef { reinterpret_cast<void *>(&wrappedNativeObj) };
    {
        // Nested scope: JSHandle<JSObject> expires when innerScope is destroyed,
        // leaving the JSWrappedNapiObject unreachable from roots.
        EcmaHandleScope innerScope(thread);
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<JSObject> wrappedObject = factory->CreateNapiObject(true);
        JSObject::SetNativePointerFieldCount(thread, wrappedObject, 1);
        JSObject::SetNativePointerField(thread, wrappedObject, 0,
                                        reinterpret_cast<void *>(mockRef),
                                        nullptr, nullptr);
        instance->SetNativeReferenceDataGetter(
            reinterpret_cast<ecmascript::NativeReferenceDataCallbackGetter>(
                MockGetNativeReferenceData));
    } // innerScope destroyed: wrappedObject handle gone, object now unreachable

    // Safe to delete now: GC already collected the object that held this pointer.
    delete mockRef;
    mockRef = nullptr;

    std::stringstream nativeAddrSS;
    nativeAddrSS << "0x" << std::hex << reinterpret_cast<uintptr_t>(&wrappedNativeObj);
    std::string nativeAddrStr = nativeAddrSS.str();

    // Create file test.heaptimeline
    std::string fileName = "test.heaptimeline";
    fstream outputString(fileName, std::ios::out);
    outputString.close();
    outputString.clear();

    FileStream stream(fileName.c_str());
    heapProfile->StopHeapTracking(&stream, nullptr);
    HeapProfilerInterface::Destroy(instance);

    // Check
    fstream inputStream(fileName, std::ios::in);
    std::string line;
    std::string emptySample = "\"samples\":";
    std::string firstSample = "\"samples\":[0, ";
    uint32_t emptySize = emptySample.size();
    bool isFind = false;
    bool isWrappedFind = false;
    bool isNativeAddrFind = false;
    while (getline(inputStream, line)) {
        if (line.substr(0U, emptySize) == emptySample) {
            ASSERT_TRUE(line.substr(0, firstSample.size()) == firstSample);
            isFind = true;
        }
        if (line.find("JSWrappedNapiObject") != std::string::npos) {
            isWrappedFind = true;
        }
        if (line.find(nativeAddrStr) != std::string::npos) {
            isNativeAddrFind = true;
        }
    }
    ASSERT_TRUE(isFind);
    ASSERT_FALSE(isWrappedFind) << "GC'd object must not appear in timeline snapshot";
    ASSERT_FALSE(isNativeAddrFind) << "Deleted native ref address must not appear: " << nativeAddrStr;

    inputStream.close();
    inputStream.clear();
    std::remove(fileName.c_str());
}

HWTEST_F_L0(HeapTrackerTest, HeapTrackerTraceAllocation)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    HeapProfilerInterface *heapProfile = HeapProfilerInterface::GetInstance(instance);
    TestStream testStream;
    heapProfile->StartHeapTracking(50, true, &testStream, true);
    sleep(1);
    int count = 100;
    while (count-- > 0) {
        instance->GetFactory()->NewJSAsyncFuncObject();
    }
    sleep(1);
    count = 100;
    while (count-- > 0) {
        instance->GetFactory()->NewJSSymbol();
    }
    sleep(1);
    count = 100;
    while (count-- > 0) {
        JSHandle<JSTaggedValue> undefined = instance->GetJSThread()->GlobalConstants()->GetHandledUndefined();
        JSHandle<EcmaString> string = instance->GetFactory()->NewFromASCII("Hello World");
        instance->GetFactory()->NewJSString(JSHandle<JSTaggedValue>(string), undefined);
    }

    // Scenario: JSWrappedNapiObject is NOT referenced by any root, and the
    // MockNativeRef it points to is deleted before the snapshot is taken.
    // After FULL_GC the unreachable object is collected; the final snapshot
    // must NOT contain the dangling native address.
    static int wrappedNativeObj = 0xF00D;
    MockNativeRef *mockRef = new MockNativeRef { reinterpret_cast<void *>(&wrappedNativeObj) };
    {
        // Nested scope: JSHandle<JSObject> expires when innerScope is destroyed,
        // leaving the JSWrappedNapiObject unreachable from roots.
        EcmaHandleScope innerScope(thread);
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<JSObject> wrappedObject = factory->CreateNapiObject(true);
        JSObject::SetNativePointerFieldCount(thread, wrappedObject, 1);
        JSObject::SetNativePointerField(thread, wrappedObject, 0,
                                        reinterpret_cast<void *>(mockRef),
                                        nullptr, nullptr);
        instance->SetNativeReferenceDataGetter(
            reinterpret_cast<ecmascript::NativeReferenceDataCallbackGetter>(
                MockGetNativeReferenceData));
    } // innerScope destroyed: wrappedObject handle gone, object now unreachable

    // Safe to delete now: GC already collected the object that held this pointer.
    delete mockRef;
    mockRef = nullptr;

    std::stringstream nativeAddrSS;
    nativeAddrSS << "0x" << std::hex << reinterpret_cast<uintptr_t>(&wrappedNativeObj);
    std::string nativeAddrStr = nativeAddrSS.str();

    // Create file test.heaptimeline
    std::string fileName = "test.heaptimeline";
    fstream outputString(fileName, std::ios::out);
    outputString.close();
    outputString.clear();

    FileStream stream(fileName.c_str());
    TestProgress testProgress;
    heapProfile->StopHeapTracking(&stream, &testProgress);
    HeapProfilerInterface::Destroy(instance);

    // Check
    fstream inputStream(fileName, std::ios::in);
    std::string line;
    std::string emptyTraceFunctionInfo = "\"trace_function_infos\":[";
    std::string firstTraceFunctionInfo = "\"trace_function_infos\":[0,";
    uint32_t emptyTraceFunctionInfoSize = emptyTraceFunctionInfo.size();
    bool traceFunctionInfoIsFind = false;
    bool isWrappedFind = false;
    bool isNativeAddrFind = false;
    while (getline(inputStream, line)) {
        if (line.substr(0U, emptyTraceFunctionInfoSize) == emptyTraceFunctionInfo) {
            ASSERT_TRUE(line.substr(0, firstTraceFunctionInfo.size()) == firstTraceFunctionInfo);
            traceFunctionInfoIsFind = true;
        }
        if (line.find("JSWrappedNapiObject") != std::string::npos) {
            isWrappedFind = true;
        }
        if (line.find(nativeAddrStr) != std::string::npos) {
            isNativeAddrFind = true;
        }
    }
    ASSERT_TRUE(traceFunctionInfoIsFind);
    ASSERT_FALSE(isWrappedFind) << "GC'd object must not appear in timeline snapshot";
    ASSERT_FALSE(isNativeAddrFind) << "Deleted native ref address must not appear: " << nativeAddrStr;

    inputStream.close();
    inputStream.clear();
    std::remove(fileName.c_str());
}

HWTEST_F_L0(HeapTrackerTest, DumpHeapSnapshot)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    HeapProfilerInterface *heapProfile = HeapProfilerInterface::GetInstance(instance);

    sleep(1);
    int count = 100;
    while (count-- > 0) {
        instance->GetFactory()->NewJSAsyncFuncObject();
    }
    sleep(1);
    count = 100;
    while (count-- > 0) {
        instance->GetFactory()->NewJSSymbol();
    }
    sleep(1);
    count = 100;
    while (count-- > 0) {
        JSHandle<JSTaggedValue> undefined = instance->GetJSThread()->GlobalConstants()->GetHandledUndefined();
        JSHandle<EcmaString> string = instance->GetFactory()->NewFromASCII("Hello World");
        instance->GetFactory()->NewJSString(JSHandle<JSTaggedValue>(string), undefined);
    }

    // Create file test.heaptimeline
    std::string fileName = "HeapTrackerTest1.heapsnapshot";
    fstream outputString(fileName, std::ios::out);
    outputString.close();
    outputString.clear();

    FileStream stream(fileName.c_str());
    DumpSnapShotOption dumpOption = { DumpFormat::JSON, true, true };
    heapProfile->DumpHeapSnapshot(&stream, dumpOption);
    HeapProfilerInterface::Destroy(instance);

    // Check
    fstream inputStream(fileName, std::ios::in);
    std::string line;
    std::string nodes = "\"nodes\":[";
    std::string sample = "\"samples\":[]";
    uint32_t nodesSize = nodes.size();
    uint32_t sampleSize = sample.size();
    bool isNodesFind = false;
    bool isSampleFind = false;
    while (getline(inputStream, line)) {
        if (line.substr(0U, nodesSize) == nodes) {
            isNodesFind = true;
        }

        if (line.substr(0U, sampleSize) == sample) {
            isSampleFind = true;
        }
    }
    ASSERT_TRUE(isNodesFind);
    ASSERT_TRUE(isSampleFind);

    inputStream.close();
    inputStream.clear();
    std::remove(fileName.c_str());
}

HWTEST_F_L0(HeapTrackerTest, HeapSnapshotBuildUp)
{
    bool isVmMode = true;
    bool isPrivate = false;
    bool traceAllocation = false;
    bool captureNumericValue = false;
    HeapProfiler heapProfiler(instance);
    DumpSnapShotOption dumpOption;
    dumpOption.isVmMode = isVmMode;
    dumpOption.isPrivate = isPrivate;
    dumpOption.captureNumericValue = captureNumericValue;
    HeapSnapshot heapSnapshot(instance, heapProfiler.GetEcmaStringTable(), dumpOption, traceAllocation,
                              heapProfiler.GetEntryIdMap());
    EXPECT_TRUE(heapSnapshot.BuildUp());
}

HWTEST_F_L0(HeapTrackerTest, HeapSnapshotUpdateNode)
{
    bool isVmMode = true;
    bool isPrivate = false;
    bool traceAllocation = false;
    bool captureNumericValue = false;
    HeapProfiler heapProfiler(instance);
    DumpSnapShotOption dumpOption;
    dumpOption.isVmMode = isVmMode;
    dumpOption.isPrivate = isPrivate;
    dumpOption.captureNumericValue = captureNumericValue;
    HeapSnapshot heapSnapshot(instance, heapProfiler.GetEcmaStringTable(), dumpOption, traceAllocation,
                              heapProfiler.GetEntryIdMap());
    size_t beginNode = heapSnapshot.GetNodeCount();
    instance->GetHeap()->WaitAndHandleCCFinished();
    heapSnapshot.UpdateNodes();
    size_t endNode = heapSnapshot.GetNodeCount();
    EXPECT_TRUE(beginNode != endNode);
}
}  // namespace panda::test
