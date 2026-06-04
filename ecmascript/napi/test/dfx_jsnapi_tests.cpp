/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/dfx/hprof/heap_profiler_interface.h"
#include "ecmascript/dfx/stackinfo/js_stackinfo.h"
#include "ecmascript/dfx/vmstat/runtime_stat.h"
#include "ecmascript/dfx/hprof/heap_profiler.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/concurrent_sweeper.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/dfx/cpu_profiler/samples_record.h"
#include "ecmascript/dfx/tracing/tracing.h"
#include "ecmascript/platform/backtrace.h"
#include "ecmascript/platform/debug_signal.h"
#include <csignal>
#include <thread>

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
using FunctionForRef = Local<JSValueRef> (*)(JsiRuntimeCallInfo *);
class DFXJSNApiTests : public testing::Test {
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
        TestHelper::CreateEcmaVMWithScope(vm_, thread_, scope_);
        vm_->GetJSThread()->GetEcmaVM()->SetRuntimeStatEnable(true);
        vm_->SetEnableForceGC(false);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(vm_, scope_);
    }

protected:
    EcmaVM *vm_ {nullptr};
    JSThread *thread_ = {nullptr};
    EcmaHandleScope *scope_ {nullptr};
};

class TestableDFXJSNApiTestsGCStats : public GCStats {
public:
    explicit TestableDFXJSNApiTestsGCStats(const Heap *heap) : GCStats(heap) {}

    using GCStats::GetGCStatistic;
    using GCStats::RecordGCStatisticEnd;
    using GCStats::RecordGCStatisticStart;
    using GCStats::SetRecordData;
    using GCStats::SetRecordDuration;

    void SetGCTypeForTest(GCType type)
    {
        gcType_ = type;
    }
};

bool MatchJSONLineHeader(std::fstream &fs, const std::string filePath, int lineNum, CString lineContent)
{
    CString tempLineContent = "";
    int lineCount = 1;
    fs.open(filePath.c_str(), std::ios::in);
    while (getline(fs, tempLineContent)) {
        if (lineNum == lineCount && tempLineContent.find(lineContent) != CString::npos) {
            fs.close();
            fs.clear();
            return true;
        }
        lineCount++;
    }
    fs.close();
    fs.clear();
    return false;
}

HWTEST_F_L0(DFXJSNApiTests, DumpHeapSnapshot_001)
{
    const std::string filePath = "DFXJSNApiTests_json_001.heapsnapshot";
    std::fstream outputString(filePath, std::ios::out);
    outputString.close();
    outputString.clear();

    std::fstream inputFile {};
    EXPECT_TRUE(inputFile.good());

    DumpSnapShotOption dumpOption;
    dumpOption.dumpFormat = ecmascript::DumpFormat::JSON;
    dumpOption.isVmMode = true;
    dumpOption.isPrivate = false;
    dumpOption.captureNumericValue = false;
    DFXJSNApi::DumpHeapSnapshot(vm_, filePath, dumpOption);
    EXPECT_TRUE(MatchJSONLineHeader(inputFile, filePath, 1, "{\"snapshot\":"));
    EXPECT_TRUE(MatchJSONLineHeader(inputFile, filePath, 2, "{\"meta\":"));
    EXPECT_TRUE(MatchJSONLineHeader(inputFile, filePath, 3, "{\"node_fields\":"));
    EXPECT_TRUE(MatchJSONLineHeader(inputFile, filePath, 4, "\"node_types\":"));
    EXPECT_TRUE(MatchJSONLineHeader(inputFile, filePath, 5, "\"edge_fields\":"));
    EXPECT_TRUE(MatchJSONLineHeader(inputFile, filePath, 6, "\"edge_types\":"));
    EXPECT_TRUE(MatchJSONLineHeader(inputFile, filePath, 7, "\"trace_function_info_fields\":"));
    EXPECT_TRUE(MatchJSONLineHeader(inputFile, filePath, 8, "\"trace_node_fields\":"));
    EXPECT_TRUE(MatchJSONLineHeader(inputFile, filePath, 9, "\"sample_fields\":"));
    EXPECT_TRUE(MatchJSONLineHeader(inputFile, filePath, 10, "\"location_fields\":"));
    std::remove(filePath.c_str());
}

HWTEST_F_L0(DFXJSNApiTests, DumpHeapSnapshot_002)
{
    const std::string filePath = "DFXJSNApiTests_json_002.heapsnapshot";
    std::fstream outputString(filePath, std::ios::out);
    outputString.close();
    outputString.clear();

    ecmascript::FileStream stream(filePath);
    EXPECT_TRUE(stream.Good());

    ecmascript::Progress *progress = nullptr;
    std::fstream fStream {};
    EXPECT_TRUE(fStream.good());

    DumpSnapShotOption dumpOption;
    dumpOption.dumpFormat = ecmascript::DumpFormat::JSON;
    dumpOption.isVmMode = true;
    dumpOption.isPrivate = false;
    dumpOption.captureNumericValue = false;
    DFXJSNApi::DumpHeapSnapshot(vm_, &stream, dumpOption, progress);
    EXPECT_TRUE(MatchJSONLineHeader(fStream, filePath, 1, "{\"snapshot\":"));
    EXPECT_TRUE(MatchJSONLineHeader(fStream, filePath, 2, "{\"meta\":"));
    EXPECT_TRUE(MatchJSONLineHeader(fStream, filePath, 3, "{\"node_fields\":"));
    EXPECT_TRUE(MatchJSONLineHeader(fStream, filePath, 4, "\"node_types\":"));
    EXPECT_TRUE(MatchJSONLineHeader(fStream, filePath, 5, "\"edge_fields\":"));
    EXPECT_TRUE(MatchJSONLineHeader(fStream, filePath, 6, "\"edge_types\":"));
    EXPECT_TRUE(MatchJSONLineHeader(fStream, filePath, 7, "\"trace_function_info_fields\":"));
    EXPECT_TRUE(MatchJSONLineHeader(fStream, filePath, 8, "\"trace_node_fields\":"));
    EXPECT_TRUE(MatchJSONLineHeader(fStream, filePath, 9, "\"sample_fields\":"));
    EXPECT_TRUE(MatchJSONLineHeader(fStream, filePath, 10, "\"location_fields\":"));
    std::remove(filePath.c_str());
}

HWTEST_F_L0(DFXJSNApiTests, BuildNativeAndJsStackTrace)
{
    bool result = false;
    std::string stackTraceStr = "stack_trace_str";
    result = DFXJSNApi::BuildNativeAndJsStackTrace(vm_, stackTraceStr);
    EXPECT_TRUE(stackTraceStr.empty());
    EXPECT_FALSE(result);
}

HWTEST_F_L0(DFXJSNApiTests, BuildJsStackTrace)
{
    std::string stackTraceStr = "stack_trace_str";
    bool result = DFXJSNApi::BuildJsStackTrace(vm_, stackTraceStr);
    EXPECT_TRUE(stackTraceStr.empty());
    EXPECT_FALSE(result);
}

HWTEST_F_L0(DFXJSNApiTests, Start_Stop_HeapTracking_001)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread_);
    vm_->SetEnableForceGC(false);

    auto factory = vm_->GetFactory();
    bool isVmMode = true;
    bool traceAllocation = false;
    double timeInterval = 50; // 50 : time interval 50 ms
    ecmascript::FileStream *stream = nullptr;
    bool startResult = false;
    startResult = DFXJSNApi::StartHeapTracking(vm_, timeInterval, isVmMode, stream, traceAllocation);
    EXPECT_TRUE(startResult);

    sleep(1);
    int count = 300;
    while (count-- > 0) {
        JSHandle<JSTaggedValue> undefined = thread_->GlobalConstants()->GetHandledUndefined();
        JSHandle<EcmaString> string = factory->NewFromASCII("Start_Stop_HeapTracking_001_TestString");
        factory->NewJSString(JSHandle<JSTaggedValue>(string), undefined);
    }
    const std::string filePath = "Start_Stop_HeapTracking_001.heaptimeline";
    std::fstream outputString(filePath, std::ios::out);
    outputString.close();
    outputString.clear();

    bool stopResult = DFXJSNApi::StopHeapTracking(vm_, filePath);
    EXPECT_TRUE(stopResult);

    std::fstream inputStream(filePath, std::ios::in);
    std::string line;
    std::string emptySample = "\"samples\":";
    std::string firstSample = "\"samples\":[0, ";
    bool isFind = false;
    while (getline(inputStream, line)) {
        if (line.substr(0U, emptySample.size()) == emptySample) {
            EXPECT_TRUE(line.substr(0, firstSample.size()) == firstSample);
            isFind = true;
        }
    }
    EXPECT_TRUE(isFind);

    inputStream.close();
    inputStream.clear();
    std::remove(filePath.c_str());
    vm_->SetEnableForceGC(true);
}

HWTEST_F_L0(DFXJSNApiTests, Start_Stop_HeapTracking_002)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread_);
    vm_->SetEnableForceGC(false);

    auto factory = vm_->GetFactory();
    bool isVmMode = true;
    bool traceAllocation = false;
    double timeInterval = 50; // 50 : time interval 50 ms
    ecmascript::FileStream *stream = nullptr;
    bool startResult = false;
    startResult = DFXJSNApi::StartHeapTracking(vm_, timeInterval, isVmMode, stream, traceAllocation);
    EXPECT_TRUE(startResult);

    sleep(1);
    int count = 300;
    while (count-- > 0) {
        factory->NewJSAsyncFuncObject();
        factory->NewJSSymbol();
    }
    const std::string filePath = "Start_Stop_HeapTracking_002.heaptimeline";
    std::fstream outputString(filePath, std::ios::out);
    outputString.close();
    outputString.clear();

    ecmascript::FileStream fileStream(filePath);
    bool stopResult = DFXJSNApi::StopHeapTracking(vm_, &fileStream);
    EXPECT_TRUE(stopResult);

    std::fstream inputStream(filePath, std::ios::in);
    std::string line;
    std::string emptySample = "\"samples\":";
    std::string firstSample = "\"samples\":[0, ";
    bool isFind = false;
    while (getline(inputStream, line)) {
        if (line.substr(0U, emptySample.size()) == emptySample) {
            EXPECT_TRUE(line.substr(0, firstSample.size()) == firstSample);
            isFind = true;
        }
    }
    EXPECT_TRUE(isFind);

    inputStream.close();
    inputStream.clear();
    std::remove(filePath.c_str());
    vm_->SetEnableForceGC(true);
}

HWTEST_F_L0(DFXJSNApiTests, Start_Stop_RuntimeStat)
{
    EcmaRuntimeStat *ecmaRuntimeStat = vm_->GetRuntimeStat();
    EXPECT_TRUE(ecmaRuntimeStat != nullptr);

    ecmaRuntimeStat->SetRuntimeStatEnabled(false);
    EXPECT_TRUE(!ecmaRuntimeStat->IsRuntimeStatEnabled());

    DFXJSNApi::StartRuntimeStat(vm_);
    EXPECT_TRUE(ecmaRuntimeStat->IsRuntimeStatEnabled());

    DFXJSNApi::StopRuntimeStat(vm_);
    EXPECT_TRUE(!ecmaRuntimeStat->IsRuntimeStatEnabled());
}

HWTEST_F_L0(DFXJSNApiTests, GetArrayBufferSize_GetHeapTotalSize_GetHeapUsedSize)
{
    size_t arrayBufferSize = DFXJSNApi::GetArrayBufferSize(vm_);
    size_t heapTotalSize = DFXJSNApi::GetHeapTotalSize(vm_);
    size_t heapUsedSize = DFXJSNApi::GetHeapUsedSize(vm_);
    size_t heapObjectSize = DFXJSNApi::GetHeapObjectSize(vm_);
    size_t processHeapLimitSize = DFXJSNApi::GetProcessHeapLimitSize();
    size_t sharedHeapObjectSize = DFXJSNApi::GetSharedHeapSize();

    size_t expectArrayBufferSize = 0;
    size_t expectHeapTotalSize = 0;
    size_t expectHeapUsedSize = 0;
    size_t expectHeapObjectSize = 0;
    size_t expectProcessHeapLimitSize = 0;
    size_t expectSharedHeapObjectSize = 0;

    if (g_isEnableCMCGC) {
        expectHeapTotalSize = common::Heap::GetHeap().GetCurrentCapacity();
        expectHeapUsedSize = common::Heap::GetHeap().GetSurvivedSize();
        expectHeapObjectSize = common::Heap::GetHeap().GetAllocatedSize();
        expectProcessHeapLimitSize = common::Heap::GetHeap().GetMaxCapacity();
        expectSharedHeapObjectSize = 0;
    } else {
        auto heap = vm_->GetHeap();
        expectArrayBufferSize = heap->GetArrayBufferSize();
        expectHeapTotalSize = heap->GetCommittedSize();
        expectHeapUsedSize = heap->GetLiveObjectSize();
        expectHeapObjectSize = heap->GetHeapObjectSize();
        expectProcessHeapLimitSize = heap->GetEcmaParamConfiguration().GetMaxHeapSize();
        expectSharedHeapObjectSize = SharedHeap::GetInstance()->GetHeapObjectSize();
        EXPECT_EQ(arrayBufferSize, expectArrayBufferSize);
        EXPECT_LE(processHeapLimitSize, MAX_MEM_POOL_CAPACITY);
    }

    EXPECT_GE(arrayBufferSize, 0);
    EXPECT_EQ(heapTotalSize, expectHeapTotalSize);
    EXPECT_EQ(heapUsedSize, expectHeapUsedSize);
    EXPECT_EQ(heapObjectSize, expectHeapObjectSize);
    EXPECT_GE(processHeapLimitSize, expectProcessHeapLimitSize);
    EXPECT_EQ(sharedHeapObjectSize, expectSharedHeapObjectSize);
}

HWTEST_F_L0(DFXJSNApiTests, DFXJSNApiForGCInfo)
{
    // fixme: adapt to cms
    if constexpr (G_USE_CMS_GC) {
        return;
    }
    if (g_isEnableCMCGC) {
        return;
    }
    size_t oldGCCount = DFXJSNApi::GetGCCount(vm_);
    size_t expectGCCount = vm_->GetEcmaGCStats()->GetGCCount() +
        ecmascript::SharedHeap::GetInstance()->GetEcmaGCStats()->GetGCCount();
    EXPECT_EQ(oldGCCount, expectGCCount);

    size_t oldGCDuration = DFXJSNApi::GetGCDuration(vm_);
    size_t expectGCDuration = vm_->GetEcmaGCStats()->GetGCDuration() +
        ecmascript::SharedHeap::GetInstance()->GetEcmaGCStats()->GetGCDuration();
    EXPECT_EQ(oldGCDuration, expectGCDuration);

    size_t oldAllocateSize = DFXJSNApi::GetAccumulatedAllocateSize(vm_);
    size_t expectAllocateSize = vm_->GetEcmaGCStats()->GetAccumulatedAllocateSize() +
        ecmascript::SharedHeap::GetInstance()->GetEcmaGCStats()->GetAccumulatedAllocateSize();
    EXPECT_EQ(oldAllocateSize, expectAllocateSize);

    size_t oldFreeSize = DFXJSNApi::GetAccumulatedFreeSize(vm_);
    size_t expectFreeSize = vm_->GetEcmaGCStats()->GetAccumulatedFreeSize() +
        ecmascript::SharedHeap::GetInstance()->GetEcmaGCStats()->GetAccumulatedFreeSize();
    EXPECT_EQ(oldFreeSize, expectFreeSize);

    size_t oldLongTimeCount = DFXJSNApi::GetFullGCLongTimeCount(vm_);
    size_t expectLongTimeCount = vm_->GetEcmaGCStats()->GetFullGCLongTimeCount() +
        ecmascript::SharedHeap::GetInstance()->GetEcmaGCStats()->GetFullGCLongTimeCount();
    EXPECT_EQ(oldLongTimeCount, expectLongTimeCount);

    ObjectFactory *factory = vm_->GetFactory();
    auto heap = const_cast<Heap *>(vm_->GetHeap());
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread_);
        for (int i = 0; i < 512; i++) {
            factory->NewTaggedArray(10 * 1024, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        }
        size_t newAllocateSize = DFXJSNApi::GetAccumulatedAllocateSize(vm_);
        EXPECT_TRUE(oldAllocateSize < newAllocateSize);
    }
    ecmascript::SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread_);
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    size_t newFreeSize = DFXJSNApi::GetAccumulatedFreeSize(vm_);
    EXPECT_TRUE(oldFreeSize < newFreeSize);
    size_t newGCCount = DFXJSNApi::GetGCCount(vm_);
    EXPECT_TRUE(oldGCCount < newGCCount);
    size_t newGCDuration = DFXJSNApi::GetGCDuration(vm_);
    EXPECT_TRUE(oldGCDuration < newGCDuration);
}

HWTEST_F_L0(DFXJSNApiTests, NotifyApplicationState)
{
    auto heap = vm_->GetHeap();
    [[maybe_unused]] auto concurrentMarker = heap->GetConcurrentMarker();
    auto sweeper = heap->GetSweeper();

    DFXJSNApi::NotifyApplicationState(vm_, false);
#if !ECMASCRIPT_DISABLE_CONCURRENT_MARKING
    EXPECT_TRUE(!concurrentMarker->IsDisabled());
#endif
    EXPECT_TRUE(!sweeper->IsDisabled());

    const_cast<ecmascript::Heap *>(heap)->CollectGarbage(TriggerGCType::OLD_GC, GCReason::OTHER);
    DFXJSNApi::NotifyApplicationState(vm_, true);
#if !ECMASCRIPT_DISABLE_CONCURRENT_MARKING
    EXPECT_TRUE(!concurrentMarker->IsDisabled());
#endif
    EXPECT_TRUE(!sweeper->IsDisabled());
}

HWTEST_F_L0(DFXJSNApiTests, NotifyMemoryPressure)
{
    auto heap = vm_->GetHeap();
    bool inHighMemoryPressure = true;
    DFXJSNApi::NotifyMemoryPressure(vm_, inHighMemoryPressure);
    EXPECT_EQ(heap->GetMemGrowingType(), MemGrowingType::PRESSURE);

    inHighMemoryPressure = false;
    DFXJSNApi::NotifyMemoryPressure(vm_, inHighMemoryPressure);
    EXPECT_EQ(heap->GetMemGrowingType(), MemGrowingType::CONSERVATIVE);
}

HWTEST_F_L0(DFXJSNApiTests, BuildJsStackInfoList)
{
    uint32_t hostTid = vm_->GetJSThread()->GetThreadId();
    std::vector<ecmascript::JsFrameInfo> jsFrameInfo;
    bool result = DFXJSNApi::BuildJsStackInfoList(vm_, hostTid, jsFrameInfo);
    EXPECT_FALSE(result);
}

HWTEST_F_L0(DFXJSNApiTests, StartSampling)
{
    uint64_t samplingInterval = 32768;
    bool result = DFXJSNApi::StartSampling(vm_, samplingInterval);
    EXPECT_TRUE(result);
    result = DFXJSNApi::StartSampling(vm_, samplingInterval);
    EXPECT_FALSE(result);
}

HWTEST_F_L0(DFXJSNApiTests, StopSampling)
{
    uint64_t samplingInterval = 32768;
    bool result = DFXJSNApi::StartSampling(vm_, samplingInterval);
    EXPECT_TRUE(result);
    DFXJSNApi::StopSampling(vm_);
    result = DFXJSNApi::StartSampling(vm_, samplingInterval);
    EXPECT_TRUE(result);
}

HWTEST_F_L0(DFXJSNApiTests, GetAllocationProfile)
{
    const SamplingInfo *result = DFXJSNApi::GetAllocationProfile(vm_);
    EXPECT_TRUE(result == nullptr);
    uint64_t samplingInterval = 32768;
    DFXJSNApi::StartSampling(vm_, samplingInterval);
    result = DFXJSNApi::GetAllocationProfile(vm_);
    EXPECT_TRUE(result != nullptr);
}

HWTEST_F_L0(DFXJSNApiTests, NotifyHighSensitive)
{
    auto heap = const_cast<ecmascript::Heap *>(vm_->GetHeap());
    DFXJSNApi::NotifyHighSensitive(vm_, true);
    EXPECT_TRUE(heap->GetSensitiveStatus() == AppSensitiveStatus::ENTER_HIGH_SENSITIVE);
    DFXJSNApi::NotifyHighSensitive(vm_, false);
    EXPECT_TRUE(heap->GetSensitiveStatus() == AppSensitiveStatus::EXIT_HIGH_SENSITIVE);
}

HWTEST_F_L0(DFXJSNApiTests, NotifyWarmStart)
{
    auto heap = const_cast<ecmascript::Heap *>(vm_->GetHeap());
    if (!g_isEnableCMCGC) {
        DFXJSNApi::NotifyWarmStart(vm_);
        EXPECT_TRUE(heap->OnStartupEvent());
        std::this_thread::sleep_for(std::chrono::seconds(3));
        if (!heap->OnStartupEvent()) {
            StartupStatus startupStatus = heap->GetStartupStatus();
            EXPECT_TRUE(startupStatus == StartupStatus::JUST_FINISH_STARTUP);
        }
    } else {
        DFXJSNApi::NotifyWarmStart(vm_);
        EXPECT_TRUE(g_isEnableCMCGC);
    }
}

#ifdef USE_CMC_GC
HWTEST_F_L0(DFXJSNApiTests, NotifyWarmStartCMC)
{
    if (!g_isEnableCMCGC) {
        g_isEnableCMCGC = true;
        DFXJSNApi::NotifyWarmStart(vm_);
        EXPECT_TRUE(g_isEnableCMCGC);
        g_isEnableCMCGC = false;
    }
}
#endif

HWTEST_F_L0(DFXJSNApiTests, GetGCCount)
{
    vm_->GetJSOptions().SetIsWorker(true);
    size_t count = DFXJSNApi::GetGCCount(vm_);
    ASSERT_EQ(count, vm_->GetEcmaGCStats()->GetGCCount());

    vm_->GetJSOptions().SetIsWorker(false);
    count = DFXJSNApi::GetGCCount(vm_);
    ASSERT_EQ(count, vm_->GetEcmaGCStats()->GetGCCount() +
        ecmascript::SharedHeap::GetInstance()->GetEcmaGCStats()->GetGCCount());
}

HWTEST_F_L0(DFXJSNApiTests, GetGCDuration)
{
    vm_->GetJSOptions().SetIsWorker(true);
    size_t duration = DFXJSNApi::GetGCDuration(vm_);
    ASSERT_EQ(duration, vm_->GetEcmaGCStats()->GetGCDuration());

    vm_->GetJSOptions().SetIsWorker(false);
    duration = DFXJSNApi::GetGCDuration(vm_);
    ASSERT_EQ(duration, vm_->GetEcmaGCStats()->GetGCDuration() +
        ecmascript::SharedHeap::GetInstance()->GetEcmaGCStats()->GetGCDuration());
}

HWTEST_F_L0(DFXJSNApiTests, GetAccumulatedAllocateSize)
{
    if (g_isEnableCMCGC) {
        size_t size = DFXJSNApi::GetAccumulatedAllocateSize(vm_);
        ASSERT_EQ(size, common::Heap::GetHeap().GetAccumulatedAllocateSize());
        return;
    }
    vm_->GetJSOptions().SetIsWorker(true);
    size_t size = DFXJSNApi::GetAccumulatedAllocateSize(vm_);
    ASSERT_EQ(size, vm_->GetEcmaGCStats()->GetAccumulatedAllocateSize());

    vm_->GetJSOptions().SetIsWorker(false);
    size = DFXJSNApi::GetAccumulatedAllocateSize(vm_);
    ASSERT_EQ(size, vm_->GetEcmaGCStats()->GetAccumulatedAllocateSize() +
        ecmascript::SharedHeap::GetInstance()->GetEcmaGCStats()->GetAccumulatedAllocateSize());
}

HWTEST_F_L0(DFXJSNApiTests, GetAccumulatedFreeSize)
{
    if (g_isEnableCMCGC) {
        size_t size = DFXJSNApi::GetAccumulatedFreeSize(vm_);
        ASSERT_EQ(size, common::Heap::GetHeap().GetAccumulatedFreeSize());
        return;
    }
    vm_->GetJSOptions().SetIsWorker(true);
    size_t size = DFXJSNApi::GetAccumulatedFreeSize(vm_);
    ASSERT_EQ(size, vm_->GetEcmaGCStats()->GetAccumulatedFreeSize());

    vm_->GetJSOptions().SetIsWorker(false);
    size = DFXJSNApi::GetAccumulatedFreeSize(vm_);
    ASSERT_EQ(size, vm_->GetEcmaGCStats()->GetAccumulatedFreeSize() +
        ecmascript::SharedHeap::GetInstance()->GetEcmaGCStats()->GetAccumulatedFreeSize());
}

HWTEST_F_L0(DFXJSNApiTests, StopCpuProfilerForColdStart)
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    ASSERT_FALSE(DFXJSNApi::StopCpuProfilerForColdStart(vm_));

    vm_->GetJSOptions().SetArkProperties(ArkProperties::CPU_PROFILER_COLD_START_MAIN_THREAD);
    ASSERT_TRUE(DFXJSNApi::StopCpuProfilerForColdStart(vm_));

    vm_->GetJSOptions().SetArkProperties(ArkProperties::CPU_PROFILER_COLD_START_WORKER_THREAD);
    ASSERT_TRUE(DFXJSNApi::StopCpuProfilerForColdStart(vm_));
#else
    ASSERT_FALSE(DFXJSNApi::StopCpuProfilerForColdStart(vm_));
#endif
}

HWTEST_F_L0(DFXJSNApiTests, CpuProfilerSamplingAnyTime)
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    ASSERT_FALSE(DFXJSNApi::CpuProfilerSamplingAnyTime(vm_));
#else
    ASSERT_FALSE(DFXJSNApi::CpuProfilerSamplingAnyTime(vm_));
#endif
}

HWTEST_F_L0(DFXJSNApiTests, StartCpuProfilerForFile)
{
    int interval = 32768;
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    int illegalInterval = 0;
    ASSERT_FALSE(DFXJSNApi::StartCpuProfilerForFile(vm_, "StartCpuProfilerForFile", illegalInterval));

    ASSERT_FALSE(DFXJSNApi::StartCpuProfilerForFile(nullptr, "StartCpuProfilerForFile", interval));

    ASSERT_FALSE(DFXJSNApi::StartCpuProfilerForFile(vm_, "StartCpuProfilerForFile", interval));
#else
    ASSERT_FALSE(DFXJSNApi::StartCpuProfilerForFile(vm_, "StartCpuProfilerForFile", interval));
#endif
}

HWTEST_F_L0(DFXJSNApiTests, StartCpuProfilerForInfo)
{
    int interval = 32768;
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    ASSERT_FALSE(DFXJSNApi::StartCpuProfilerForInfo(nullptr, interval));

    int illegalInterval = 0;
    ASSERT_FALSE(DFXJSNApi::StartCpuProfilerForInfo(vm_, illegalInterval));

    ASSERT_TRUE(DFXJSNApi::StartCpuProfilerForInfo(vm_, interval));
    ASSERT_NE(DFXJSNApi::StopCpuProfilerForInfo(vm_), nullptr);
#else
    ASSERT_FALSE(DFXJSNApi::StartCpuProfilerForInfo(vm_, interval));
#endif
}

HWTEST_F_L0(DFXJSNApiTests, StopCpuProfilerForInfo)
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    ASSERT_EQ(DFXJSNApi::StopCpuProfilerForInfo(nullptr), nullptr);

    vm_->SetProfiler(nullptr);
    ASSERT_EQ(DFXJSNApi::StopCpuProfilerForInfo(vm_), nullptr);
#else
    ASSERT_EQ(DFXJSNApi::StopCpuProfilerForInfo(vm_), nullptr);
#endif
}

HWTEST_F_L0(DFXJSNApiTests, SuspendVM)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    ASSERT_FALSE(DFXJSNApi::SuspendVM(vm_));
#else
    ASSERT_FALSE(DFXJSNApi::SuspendVM(vm_));
#endif
}

HWTEST_F_L0(DFXJSNApiTests, IsSuspended)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    ASSERT_FALSE(DFXJSNApi::IsSuspended(vm_));
#else
    ASSERT_FALSE(DFXJSNApi::IsSuspended(vm_));
#endif
}

HWTEST_F_L0(DFXJSNApiTests, CheckSafepoint)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    ASSERT_FALSE(DFXJSNApi::CheckSafepoint(vm_));
#else
    ASSERT_FALSE(DFXJSNApi::CheckSafepoint(vm_));
#endif
}

HWTEST_F_L0(DFXJSNApiTests, BuildJsStackInfoList_2)
{
    std::vector<ecmascript::JsFrameInfo> jsFrames;
    uint32_t tid = vm_->GetAssociatedJSThread()->GetThreadId();
    ASSERT_FALSE(DFXJSNApi::BuildJsStackInfoList(vm_, tid + 1, jsFrames));

    ASSERT_FALSE(DFXJSNApi::BuildJsStackInfoList(vm_, tid, jsFrames));
}

HWTEST_F_L0(DFXJSNApiTests, StartProfiler)
{
    DFXJSNApi::ProfilerOption option;
    option.profilerType = DFXJSNApi::ProfilerType::CPU_PROFILER;
    DebuggerPostTask debuggerPostTask;
    uint32_t tid = vm_->GetAssociatedJSThread()->GetThreadId();
    int32_t instanceId = 1;
    ASSERT_FALSE(DFXJSNApi::StartProfiler(nullptr, option, tid, instanceId, debuggerPostTask, true));

    option.profilerType = DFXJSNApi::ProfilerType::HEAP_PROFILER;
    ASSERT_FALSE(DFXJSNApi::StartProfiler(nullptr, option, tid, instanceId, debuggerPostTask, false));
}

HWTEST_F_L0(DFXJSNApiTests, SuspendVMById)
{
    uint32_t tid = vm_->GetAssociatedJSThread()->GetThreadId();
    ASSERT_FALSE(DFXJSNApi::SuspendVMById(vm_, tid + 1));

#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    ASSERT_FALSE(DFXJSNApi::SuspendVMById(vm_, tid));
#else
    ASSERT_FALSE(DFXJSNApi::SuspendVMById(vm_, tid));
#endif
}

HWTEST_F_L0(DFXJSNApiTests, StartTracing)
{
    std::string categories = "StartTracing";
#if defined(ECMASCRIPT_SUPPORT_TRACING)
    ASSERT_FALSE(DFXJSNApi::StartTracing(nullptr, categories));

    vm_->SetTracing(nullptr);
    ASSERT_TRUE(DFXJSNApi::StartTracing(vm_, categories));
    ASSERT_NE(DFXJSNApi::StopTracing(vm_), nullptr);
#else
    ASSERT_FALSE(DFXJSNApi::StartTracing(vm_, categories));
#endif
}

HWTEST_F_L0(DFXJSNApiTests, StopTracing)
{
#if defined(ECMASCRIPT_SUPPORT_TRACING)
    ASSERT_EQ(DFXJSNApi::StopTracing(nullptr), nullptr);

    vm_->SetTracing(nullptr);
    ASSERT_EQ(DFXJSNApi::StopTracing(vm_), nullptr);
#else
    ASSERT_EQ(DFXJSNApi::StopTracing(vm_), nullptr);
#endif
}

HWTEST_F_L0(DFXJSNApiTests, TranslateJSStackInfo)
{
    std::string resultUrl = "";
    auto cb = [&resultUrl](std::string& url, int& line, int& column, std::string& packageName) -> bool {
        line = 0;
        column = 0;
        packageName = "name";
        if (url.find("TranslateJSStackInfo", 0) != std::string::npos) {
            resultUrl = "true";
            return true;
        }
        resultUrl = "false";
        return false;
    };

    vm_->SetSourceMapTranslateCallback(nullptr);
    std::string url = "TranslateJSStackInfo";
    int32_t line = 0;
    int32_t column = 0;
    std::string packageName = "";
    DFXJSNApi::TranslateJSStackInfo(vm_, url, line, column, packageName);

    vm_->SetSourceMapTranslateCallback(cb);
    url = "Translate";
    DFXJSNApi::TranslateJSStackInfo(vm_, url, line, column, packageName);
    ASSERT_STREQ(resultUrl.c_str(), "false");

    url = "TranslateJSStackInfo";
    DFXJSNApi::TranslateJSStackInfo(vm_, url, line, column, packageName);
    ASSERT_STREQ(resultUrl.c_str(), "true");
}

HWTEST_F_L0(DFXJSNApiTests, GetCurrentThreadId)
{
    ASSERT_EQ(DFXJSNApi::GetCurrentThreadId(), JSThread::GetCurrentThreadId());
}

Local<JSValueRef> FunctionCallback(JsiRuntimeCallInfo *info)
{
    EscapeLocalScope scope(info->GetVM());
    return scope.Escape(ArrayRef::New(info->GetVM(), info->GetArgsNumber()));
}

HWTEST_F_L0(DFXJSNApiTests, GetObjectHashCode_1)
{
    LocalScope scope(vm_);
    Local<FunctionRef> functioncallback = FunctionRef::New(vm_, FunctionCallback);
    struct Data {
        int32_t length;
    };
    const int32_t length = 15;
    Data *data = new Data();
    data->length = length;
    functioncallback->SetData(vm_, data);
    auto hash = DFXJSNApi::GetObjectHashCode(vm_, functioncallback);
    ASSERT_TRUE(hash != 0);
}

HWTEST_F_L0(DFXJSNApiTests, GetObjectHashCode_2)
{
    Local<ObjectRef> object = ObjectRef::New(vm_);
    object->SetNativePointerFieldCount(vm_, 10);
    auto hash = DFXJSNApi::GetObjectHashCode(vm_, object);
    ASSERT_TRUE(hash != 0);
}

HWTEST_F_L0(DFXJSNApiTests, GetObjectHash_3)
{
    Local<ObjectRef> object = ObjectRef::New(vm_);
    auto hash = DFXJSNApi::GetObjectHash(vm_, object);
    ASSERT_TRUE(hash != 0);
}

HWTEST_F_L0(DFXJSNApiTests, GetObjectHashCode_3)
{
    Local<ObjectRef> object = ObjectRef::New(vm_);
    auto hash = DFXJSNApi::GetObjectHashCode(vm_, object);
    ASSERT_TRUE(hash != 0);
}

HWTEST_F_L0(DFXJSNApiTests, GetObjectHash_4)
{
    Local<ObjectRef> object = ObjectRef::New(vm_);
    NativePointerCallback callBack = nullptr;
    void *vp1 = static_cast<void *>(new std::string("test"));
    void *vp2 = static_cast<void *>(new std::string("test"));
    object->SetNativePointerField(vm_, 33, vp1, callBack, vp2);
    auto hash = DFXJSNApi::GetObjectHash(vm_, object);
    ASSERT_TRUE(hash != 0);
}

HWTEST_F_L0(DFXJSNApiTests, GetObjectHashCode_4)
{
    Local<ObjectRef> object = ObjectRef::New(vm_);
    NativePointerCallback callBack = nullptr;
    void *vp1 = static_cast<void *>(new std::string("test"));
    void *vp2 = static_cast<void *>(new std::string("test"));
    object->SetNativePointerField(vm_, 33, vp1, callBack, vp2);
    auto hash = DFXJSNApi::GetObjectHashCode(vm_, object);
    ASSERT_TRUE(hash != 0);
}

HWTEST_F_L0(DFXJSNApiTests, GetMainThreadStackTrace_1)
{
    std::string stackTraceStr;
    DFXJSNApi::GetMainThreadStackTrace(vm_, stackTraceStr);
    ASSERT_TRUE(stackTraceStr.empty());
}

HWTEST_F_L0(DFXJSNApiTests, SetMultithreadingDetectionEnabled_1)
{
    bool prevCheck = EcmaVM::GetMultiThreadCheck();
    bool prevCountApi = EcmaVM::GetCheckCountApi();
    bool prevAbort = EcmaVM::GetDetectionConfig().abort.load();
    uint64_t prevFreq = EcmaVM::GetDetectionConfig().frequency.load();
    uint64_t prevInterval = EcmaVM::GetDetectionConfig().interval.load();
    DFXJSNApi::MultithreadingDetectionOptions options(true, 100, 5);
    DFXJSNApi::SetMultithreadingDetectionEnabled(vm_, true, options);
    ASSERT_TRUE(EcmaVM::GetMultiThreadCheck());
    ASSERT_TRUE(EcmaVM::GetCheckCountApi());
    ASSERT_TRUE(EcmaVM::GetDetectionConfig().abort.load());
    ASSERT_EQ(EcmaVM::GetDetectionConfig().frequency.load(), 100U);
    ASSERT_EQ(EcmaVM::GetDetectionConfig().interval.load(), 5U);
    EcmaVM::SetMultiThreadCheck(prevCheck);
    EcmaVM::SetCheckCountApi(prevCountApi);
    EcmaVM::SetDetectionConfig(DFXJSNApi::MultithreadingDetectionOptions(prevAbort, prevFreq, prevInterval));
}

HWTEST_F_L0(DFXJSNApiTests, SetMultithreadingDetectionEnabled_2)
{
    bool prevCheck = EcmaVM::GetMultiThreadCheck();
    bool prevCountApi = EcmaVM::GetCheckCountApi();
    DFXJSNApi::MultithreadingDetectionOptions options(false, 200, 10);
    DFXJSNApi::SetMultithreadingDetectionEnabled(vm_, false, options);
    ASSERT_FALSE(EcmaVM::GetMultiThreadCheck());
    ASSERT_FALSE(EcmaVM::GetCheckCountApi());
    EcmaVM::SetMultiThreadCheck(prevCheck);
    EcmaVM::SetCheckCountApi(prevCountApi);
}

HWTEST_F_L0(DFXJSNApiTests, SetMultithreadingDetectionEnabled_WithAbortFalse)
{
    bool prevCheck = EcmaVM::GetMultiThreadCheck();
    bool prevAbort = EcmaVM::GetDetectionConfig().abort.load();
    uint64_t prevFreq = EcmaVM::GetDetectionConfig().frequency.load();
    uint64_t prevInterval = EcmaVM::GetDetectionConfig().interval.load();
    DFXJSNApi::MultithreadingDetectionOptions options(false, 100, 5);
    DFXJSNApi::SetMultithreadingDetectionEnabled(vm_, true, options);
    ASSERT_TRUE(EcmaVM::GetMultiThreadCheck());
    ASSERT_FALSE(EcmaVM::GetDetectionConfig().abort.load());
    EcmaVM::SetMultiThreadCheck(prevCheck);
    EcmaVM::SetDetectionConfig(DFXJSNApi::MultithreadingDetectionOptions(prevAbort, prevFreq, prevInterval));
}

HWTEST_F_L0(DFXJSNApiTests, SetMultithreadingDetectionEnabled_DisabledNoOptionsApplied)
{
    bool prevAbort = EcmaVM::GetDetectionConfig().abort.load();
    uint64_t prevFreq = EcmaVM::GetDetectionConfig().frequency.load();
    uint64_t prevInterval = EcmaVM::GetDetectionConfig().interval.load();
    EcmaVM::SetDetectionConfig(DFXJSNApi::MultithreadingDetectionOptions(true, 150, 8));
    DFXJSNApi::MultithreadingDetectionOptions options(false, 50, 1);
    DFXJSNApi::SetMultithreadingDetectionEnabled(vm_, false, options);
    ASSERT_FALSE(EcmaVM::GetMultiThreadCheck());
    ASSERT_TRUE(EcmaVM::GetDetectionConfig().abort.load());
    ASSERT_EQ(EcmaVM::GetDetectionConfig().frequency.load(), 150U);
    ASSERT_EQ(EcmaVM::GetDetectionConfig().interval.load(), 8U);
    EcmaVM::SetDetectionConfig(DFXJSNApi::MultithreadingDetectionOptions(prevAbort, prevFreq, prevInterval));
}

HWTEST_F_L0(DFXJSNApiTests, SetDetectionConfig_ValidValue)
{
    bool prevAbort = EcmaVM::GetDetectionConfig().abort.load();
    uint64_t prevFreq = EcmaVM::GetDetectionConfig().frequency.load();
    uint64_t prevInterval = EcmaVM::GetDetectionConfig().interval.load();
    EcmaVM::SetDetectionConfig(DFXJSNApi::MultithreadingDetectionOptions(true, 100, 0));
    ASSERT_TRUE(EcmaVM::GetDetectionConfig().abort.load());
    ASSERT_EQ(EcmaVM::GetDetectionConfig().frequency.load(), 100U);
    ASSERT_EQ(EcmaVM::GetDetectionConfig().interval.load(), 0U);
    EcmaVM::SetDetectionConfig(DFXJSNApi::MultithreadingDetectionOptions(true, 200, 1440));
    ASSERT_EQ(EcmaVM::GetDetectionConfig().frequency.load(), 200U);
    ASSERT_EQ(EcmaVM::GetDetectionConfig().interval.load(), 1440U);
    EcmaVM::SetDetectionConfig(DFXJSNApi::MultithreadingDetectionOptions(prevAbort, prevFreq, prevInterval));
}

HWTEST_F_L0(DFXJSNApiTests, SetDetectionConfig_InvalidFrequency)
{
    uint64_t originalFreq = EcmaVM::GetDetectionConfig().frequency.load();
    EcmaVM::SetDetectionConfig(DFXJSNApi::MultithreadingDetectionOptions(true, -1, 5));
    ASSERT_EQ(EcmaVM::GetDetectionConfig().frequency.load(), originalFreq);
    EcmaVM::SetDetectionConfig(DFXJSNApi::MultithreadingDetectionOptions(true, 99, 5));
    ASSERT_EQ(EcmaVM::GetDetectionConfig().frequency.load(), originalFreq);
}

HWTEST_F_L0(DFXJSNApiTests, SetDetectionConfig_InvalidInterval)
{
    uint64_t originalInterval = EcmaVM::GetDetectionConfig().interval.load();
    EcmaVM::SetDetectionConfig(DFXJSNApi::MultithreadingDetectionOptions(true, 100, 14401));
    ASSERT_EQ(EcmaVM::GetDetectionConfig().interval.load(), originalInterval);
    EcmaVM::SetDetectionConfig(DFXJSNApi::MultithreadingDetectionOptions(true, 100, -1));
    ASSERT_EQ(EcmaVM::GetDetectionConfig().interval.load(), originalInterval);
}

HWTEST_F_L0(DFXJSNApiTests, CheckCountNum_TrueAndFalse)
{
    bool prevCheckCountApi = EcmaVM::GetCheckCountApi();
    bool prevAbort = EcmaVM::GetDetectionConfig().abort.load();
    uint64_t prevFreq = EcmaVM::GetDetectionConfig().frequency.load();
    uint64_t prevInterval = EcmaVM::GetDetectionConfig().interval.load();
    EcmaVM::SetCheckCountApi(false);
    EcmaVM::SetDetectionConfig(DFXJSNApi::MultithreadingDetectionOptions(true, 100, 5));
    while (!vm_->CheckCountNum()) {}
    ASSERT_FALSE(vm_->CheckCountNum());
    for (int i = 0; i < 98; i++) {
        vm_->CheckCountNum();
    }
    ASSERT_TRUE(vm_->CheckCountNum());
    EcmaVM::SetCheckCountApi(prevCheckCountApi);
    EcmaVM::SetDetectionConfig(DFXJSNApi::MultithreadingDetectionOptions(prevAbort, prevFreq, prevInterval));
}

HWTEST_F_L0(DFXJSNApiTests, CheckTimeInterval_LastTimeZeroReturnsTrue)
{
    ASSERT_TRUE(vm_->CheckTimeInterval());
}

HWTEST_F_L0(DFXJSNApiTests, GetTimeStamp_ReturnsNonZero)
{
    uint64_t ts = vm_->GetTimeStamp();
    ASSERT_GT(ts, 0U);
}

HWTEST_F_L0(DFXJSNApiTests, MultithreadingDetectionOptions_DefaultValues)
{
    DFXJSNApi::MultithreadingDetectionOptions opts(true, 100, 5);
    ASSERT_TRUE(opts.abort);
    ASSERT_EQ(opts.frequency, 100U);
    ASSERT_EQ(opts.interval, 5U);
}

HWTEST_F_L0(DFXJSNApiTests, FindFunctionForHook_1)
{
    std::string baseFileName = ABC_PATH "module_export.abc";
    JSNApi::EnableUserUncaughtErrorHandler(vm_);
    bool result = JSNApi::Execute(vm_, baseFileName, "module_export");
    EXPECT_TRUE(result);
    std::string recordName = "module";
    std::string namespaceName = "TestNamespace2";
    std::string className = "TestClass1";
    std::string funcName = "testFunction";
    JSHandle<JSTaggedValue> functionFound = DFXJSNApi::FindFunctionForHook(vm_, recordName,
        namespaceName, className, funcName);
    EXPECT_TRUE(functionFound->IsUndefined());
}

HWTEST_F_L0(DFXJSNApiTests, FindFunctionForHook_2)
{
    std::string baseFileName = ABC_PATH "module_export.abc";
    JSNApi::EnableUserUncaughtErrorHandler(vm_);
    bool result = JSNApi::Execute(vm_, baseFileName, "module_export");
    EXPECT_TRUE(result);
    std::string recordName = "module_export";
    std::string namespaceName = "TestNamespace2";
    std::string className = "TestClass1";
    std::string funcName = "testFunction1";
    JSHandle<JSTaggedValue> functionFound = DFXJSNApi::FindFunctionForHook(vm_, recordName,
        namespaceName, className, funcName);
    EXPECT_TRUE(functionFound->IsUndefined());
}

HWTEST_F_L0(DFXJSNApiTests, FindFunctionForHook_3)
{
    std::string baseFileName = ABC_PATH "module_export.abc";
    JSNApi::EnableUserUncaughtErrorHandler(vm_);
    bool result = JSNApi::Execute(vm_, baseFileName, "module_export");
    EXPECT_TRUE(result);
    std::string recordName = "module_export";
    std::string namespaceName = "TestNamespace2";
    std::string className = "TestClass2";
    std::string funcName = "testFunction";
    JSHandle<JSTaggedValue> functionFound = DFXJSNApi::FindFunctionForHook(vm_, recordName,
        namespaceName, className, funcName);
    EXPECT_TRUE(functionFound->IsUndefined());
}

HWTEST_F_L0(DFXJSNApiTests, FindFunctionForHook_4)
{
    std::string baseFileName = ABC_PATH "module_export.abc";
    JSNApi::EnableUserUncaughtErrorHandler(vm_);
    bool result = JSNApi::Execute(vm_, baseFileName, "module_export");
    EXPECT_TRUE(result);
    std::string recordName = "module_export";
    std::string namespaceName = "";
    std::string className = "TestClass2";
    std::string funcName = "testFunction1";
    JSHandle<JSTaggedValue> functionFound = DFXJSNApi::FindFunctionForHook(vm_, recordName,
        namespaceName, className, funcName);
    EXPECT_TRUE(functionFound->IsUndefined());
}

HWTEST_F_L0(DFXJSNApiTests, FindFunctionForHook_5)
{
    std::string baseFileName = ABC_PATH "module_export.abc";
    JSNApi::EnableUserUncaughtErrorHandler(vm_);
    bool result = JSNApi::Execute(vm_, baseFileName, "module_export");
    EXPECT_TRUE(result);
    std::string recordName = "module_export";
    std::string namespaceName = "TestNamespace2";
    std::string className = "";
    std::string funcName = "testFunction";
    JSHandle<JSTaggedValue> functionFound = DFXJSNApi::FindFunctionForHook(vm_, recordName,
        namespaceName, className, funcName);
    EXPECT_TRUE(functionFound->IsUndefined());
}

HWTEST_F_L0(DFXJSNApiTests, FindFunctionForHook_6)
{
    std::string baseFileName = ABC_PATH "module_export.abc";
    JSNApi::EnableUserUncaughtErrorHandler(vm_);
    bool result = JSNApi::Execute(vm_, baseFileName, "module_export");
    EXPECT_TRUE(result);
    std::string recordName = "module_export";
    std::string namespaceName = "TestNamespace1";
    std::string className = "";
    std::string funcName = "testFunction";
    JSHandle<JSTaggedValue> functionFound = DFXJSNApi::FindFunctionForHook(vm_, recordName,
        namespaceName, className, funcName);
    EXPECT_TRUE(functionFound->IsJSFunction());
    JSFunction *function = JSFunction::Cast(functionFound->GetTaggedObject());
    Method *method = Method::Cast(function->GetMethod(thread_));
    EXPECT_EQ("testFunction", method->ParseFunctionName(thread_));
}

HWTEST_F_L0(DFXJSNApiTests, FindFunctionForHook_7)
{
    std::string baseFileName = ABC_PATH "module_export.abc";
    JSNApi::EnableUserUncaughtErrorHandler(vm_);
    bool result = JSNApi::Execute(vm_, baseFileName, "module_export");
    EXPECT_TRUE(result);
    std::string recordName = "module_export";
    std::string namespaceName = "TestNamespace2";
    std::string className = "TestClass1";
    std::string funcName = "testFunction";
    JSHandle<JSTaggedValue> functionFound = DFXJSNApi::FindFunctionForHook(vm_, recordName,
        namespaceName, className, funcName);
    EXPECT_TRUE(functionFound->IsJSFunction());
    JSFunction *function = JSFunction::Cast(functionFound->GetTaggedObject());
    Method *method = Method::Cast(function->GetMethod(thread_));
    EXPECT_EQ("testFunction", method->ParseFunctionName(thread_));
}

HWTEST_F_L0(DFXJSNApiTests, FindFunctionForHook_8)
{
    std::string baseFileName = ABC_PATH "module_export.abc";
    JSNApi::EnableUserUncaughtErrorHandler(vm_);
    bool result = JSNApi::Execute(vm_, baseFileName, "module_export");
    EXPECT_TRUE(result);
    std::string recordName = "module_export";
    std::string namespaceName = "";
    std::string className = "TestClass2";
    std::string funcName = "testFunction";
    JSHandle<JSTaggedValue> functionFound = DFXJSNApi::FindFunctionForHook(vm_, recordName,
        namespaceName, className, funcName);
    EXPECT_TRUE(functionFound->IsJSFunction());
    JSFunction *function = JSFunction::Cast(functionFound->GetTaggedObject());
    Method *method = Method::Cast(function->GetMethod(thread_));
    EXPECT_EQ("testFunction", method->ParseFunctionName(thread_));
}

HWTEST_F_L0(DFXJSNApiTests, FindFunctionForHook_9)
{
    std::string baseFileName = ABC_PATH "module_export.abc";
    JSNApi::EnableUserUncaughtErrorHandler(vm_);
    bool result = JSNApi::Execute(vm_, baseFileName, "module_export");
    EXPECT_TRUE(result);
    std::string recordName = "module_export";
    std::string namespaceName = "";
    std::string className = "";
    std::string funcName = "testFunction";
    JSHandle<JSTaggedValue> functionFound = DFXJSNApi::FindFunctionForHook(vm_, recordName,
        namespaceName, className, funcName);
    EXPECT_TRUE(functionFound->IsJSFunction());
    JSFunction *function = JSFunction::Cast(functionFound->GetTaggedObject());
    Method *method = Method::Cast(function->GetMethod(thread_));
    EXPECT_EQ("testFunction", method->ParseFunctionName(thread_));
}

static JSTaggedValue TestForFunction1([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    return JSTaggedValue::True();
}

static JSTaggedValue TestForFunction2([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    return JSTaggedValue::False();
}

static JSTaggedValue TestForFunction3([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    return JSTaggedValue::False();
}

HWTEST_F_L0(DFXJSNApiTests, ReplaceFunctionForHook_1) {
    LocalScope scope(vm_);
    vm_->SetEnableForceGC(false);
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> targetFunction = factory->NewJSFunction(env, reinterpret_cast<void *>(TestForFunction1));
    JSHandle<LexicalEnv> lexicalEnv1 = thread_->GetEcmaVM()->GetFactory()->NewLexicalEnv(0);
    targetFunction->SetLexicalEnv(thread_, lexicalEnv1.GetTaggedValue());
    JSHandle<JSFunction> hookFunction = factory->NewJSFunction(env, reinterpret_cast<void *>(TestForFunction2));
    JSHandle<LexicalEnv> lexicalEnv2 = thread_->GetEcmaVM()->GetFactory()->NewLexicalEnv(1);
    hookFunction->SetLexicalEnv(thread_, lexicalEnv2.GetTaggedValue());
    JSHandle<JSFunction> backupFunction = factory->NewJSFunction(env, reinterpret_cast<void *>(TestForFunction3));
    JSHandle<LexicalEnv> lexicalEnv3 = thread_->GetEcmaVM()->GetFactory()->NewLexicalEnv(2);
    backupFunction->SetLexicalEnv(thread_, lexicalEnv3.GetTaggedValue());
    EXPECT_FALSE(targetFunction->GetLexicalEnv(thread_) == hookFunction->GetLexicalEnv(thread_));
    EXPECT_FALSE(targetFunction->GetCodeEntryOrNativePointer() == hookFunction->GetCodeEntryOrNativePointer());
    JSHandle<JSTaggedValue> target = JSHandle<JSTaggedValue>(targetFunction);
    JSHandle<JSTaggedValue> hook = JSHandle<JSTaggedValue>(hookFunction);
    JSHandle<JSTaggedValue> backup = JSHandle<JSTaggedValue>(backupFunction);
    DFXJSNApi::ReplaceFunctionForHook(vm_, target, hook, backup);
    JSHandle<JSFunction> targetFunc = JSHandle<JSFunction>::Cast(target);
    JSHandle<JSFunction> hookFunc = JSHandle<JSFunction>::Cast(hook);
    EXPECT_TRUE(targetFunc->GetLexicalEnv(thread_) == targetFunc->GetLexicalEnv(thread_));
    EXPECT_TRUE(hookFunc->GetCodeEntryOrNativePointer() == hookFunc->GetCodeEntryOrNativePointer());
}

HWTEST_F_L0(DFXJSNApiTests, LoadHookModule_1)
{
    LocalScope scope(vm_);
    EXPECT_FALSE(DFXJSNApi::LoadHookModule(vm_));
}

HWTEST_F_L0(DFXJSNApiTests, GetHybridStackTrace1) {
    RuntimeOption option;
    std::thread t1([&]() {
        auto vm1 = JSNApi::CreateJSVM(option);
        int size = BacktraceHybrid(vm1->GetPcVectorData());
        vm1->SetPcVectorSize(size);
        std::string stackTraceStr;
        DFXJSNApi::GetHybridStackTrace(vm1, stackTraceStr);
#if defined(ENABLE_BACKTRACE_LOCAL)
        EXPECT_TRUE(!stackTraceStr.empty());
#else
        EXPECT_TRUE(stackTraceStr.empty());
#endif
        JSNApi::DestroyJSVM(vm1);
    });
    t1.join();
}

HWTEST_F_L0(DFXJSNApiTests, GetHybridStackTrace2) {
    std::string stackTraceStr;
    int size = BacktraceHybrid(vm_->GetPcVectorData());
    vm_->SetPcVectorSize(size);
    DFXJSNApi::GetHybridStackTrace(vm_, stackTraceStr);
#if defined(ENABLE_BACKTRACE_LOCAL)
    EXPECT_TRUE(!stackTraceStr.empty());
#else
    EXPECT_TRUE(stackTraceStr.empty());
#endif
}

HWTEST_F_L0(DFXJSNApiTests, GetHybridStackTrace3) {
    std::string stackTraceStr;
    DFXJSNApi::GetHybridStackTrace(vm_, stackTraceStr);
    EXPECT_TRUE(stackTraceStr.empty());
}

HWTEST_F_L0(DFXJSNApiTests, GetGCStatisticMainThread001)
{
    auto expectStats = GCStats::MergeGCStatistic(vm_->GetEcmaGCStats()->GetGCStatistic(),
        ecmascript::SharedHeap::GetInstance()->GetEcmaGCStats()->GetGCStatistic());
    EXPECT_EQ(expectStats.count, 0U);

    auto gcStats = DFXJSNApi::GetGCStatistic(vm_);
    EXPECT_EQ(gcStats.count, expectStats.count);
    EXPECT_FLOAT_EQ(gcStats.maxPause, expectStats.maxPause);
    EXPECT_FLOAT_EQ(gcStats.minPause, expectStats.minPause);
    EXPECT_FLOAT_EQ(gcStats.averagePause, 0.0f);
    EXPECT_EQ(gcStats.lastStartTime, expectStats.lastStartTime);
    EXPECT_EQ(gcStats.lastEndTime, expectStats.lastEndTime);
    EXPECT_STREQ(gcStats.lastType, expectStats.lastType);
}

HWTEST_F_L0(DFXJSNApiTests, GetGCStatisticMainThread002)
{
    if constexpr (G_USE_CMS_GC) {
        return;
    }
    if (g_isEnableCMCGC) {
        return;
    }

    auto heap = const_cast<Heap *>(vm_->GetHeap());
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    ecmascript::SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread_);

    auto expectStats = GCStats::MergeGCStatistic(vm_->GetEcmaGCStats()->GetGCStatistic(),
        ecmascript::SharedHeap::GetInstance()->GetEcmaGCStats()->GetGCStatistic());
    EXPECT_GT(expectStats.count, 0U);

    auto gcStats = DFXJSNApi::GetGCStatistic(vm_);
    EXPECT_EQ(gcStats.count, expectStats.count);
    EXPECT_FLOAT_EQ(gcStats.maxPause, expectStats.maxPause);
    EXPECT_FLOAT_EQ(gcStats.minPause, expectStats.minPause);
    EXPECT_FLOAT_EQ(gcStats.averagePause, expectStats.totalPause / expectStats.count);
    EXPECT_EQ(gcStats.lastStartTime, expectStats.lastStartTime);
    EXPECT_EQ(gcStats.lastEndTime, expectStats.lastEndTime);
    EXPECT_STREQ(gcStats.lastType, expectStats.lastType);
}

HWTEST_F_L0(DFXJSNApiTests, GetGCStatisticMainThread003)
{
    auto expectStats = GCStats::MergeGCStatistic(vm_->GetEcmaGCStats()->GetGCStatistic(),
        ecmascript::SharedHeap::GetInstance()->GetEcmaGCStats()->GetGCStatistic());
    auto gcStats = DFXJSNApi::GetGCStatistic(vm_);

    EXPECT_EQ(gcStats.count, expectStats.count);
    EXPECT_EQ(gcStats.lastStartTime, expectStats.lastStartTime);
    EXPECT_EQ(gcStats.lastEndTime, expectStats.lastEndTime);
    if (gcStats.count == 0) {
        EXPECT_FLOAT_EQ(gcStats.averagePause, 0.0f);
    } else {
        EXPECT_FLOAT_EQ(gcStats.averagePause, expectStats.totalPause / expectStats.count);
    }
}

HWTEST_F_L0(DFXJSNApiTests, GetGCStatisticMainThread004)
{
    if constexpr (G_USE_CMS_GC) {
        return;
    }
    if (g_isEnableCMCGC) {
        return;
    }

    auto heap = const_cast<Heap *>(vm_->GetHeap());
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    auto expectStats = GCStats::MergeGCStatistic(vm_->GetEcmaGCStats()->GetGCStatistic(),
        ecmascript::SharedHeap::GetInstance()->GetEcmaGCStats()->GetGCStatistic());
    auto gcStats = DFXJSNApi::GetGCStatistic(vm_);

    EXPECT_EQ(gcStats.count, expectStats.count);
    EXPECT_FLOAT_EQ(gcStats.maxPause, expectStats.maxPause);
    EXPECT_FLOAT_EQ(gcStats.minPause, expectStats.minPause);
    EXPECT_FLOAT_EQ(gcStats.averagePause, expectStats.totalPause / expectStats.count);
    EXPECT_EQ(gcStats.lastStartTime, expectStats.lastStartTime);
    EXPECT_EQ(gcStats.lastEndTime, expectStats.lastEndTime);
    EXPECT_STREQ(gcStats.lastType, expectStats.lastType);
}

HWTEST_F_L0(DFXJSNApiTests, GetGCStatisticMainThread005)
{
    if constexpr (G_USE_CMS_GC) {
        return;
    }
    if (g_isEnableCMCGC) {
        return;
    }

    auto heap = const_cast<Heap *>(vm_->GetHeap());
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    auto firstStats = DFXJSNApi::GetGCStatistic(vm_);
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    auto secondStats = DFXJSNApi::GetGCStatistic(vm_);

    EXPECT_GE(secondStats.count, firstStats.count);
    EXPECT_GE(secondStats.maxPause, 0.0f);
    EXPECT_GE(secondStats.minPause, 0.0f);
    EXPECT_GE(secondStats.averagePause, 0.0f);
    EXPECT_GE(secondStats.lastStartTime, firstStats.lastStartTime);
    EXPECT_GE(secondStats.lastEndTime, firstStats.lastEndTime);
}

HWTEST_F_L0(DFXJSNApiTests, GetGCStatisticMainThread006)
{
    if constexpr (G_USE_CMS_GC) {
        return;
    }
    if (g_isEnableCMCGC) {
        return;
    }

    auto heap = const_cast<Heap *>(vm_->GetHeap());
    ecmascript::SharedHeap *sharedHeap = ecmascript::SharedHeap::GetInstance();
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    auto localStats = vm_->GetEcmaGCStats()->GetGCStatistic();
    auto sharedStats = sharedHeap->GetEcmaGCStats()->GetGCStatistic();
    auto mergedStats = GCStats::MergeGCStatistic(localStats, sharedStats);
    auto gcStats = DFXJSNApi::GetGCStatistic(vm_);

    EXPECT_EQ(gcStats.count, localStats.count + sharedStats.count);
    EXPECT_EQ(gcStats.count, mergedStats.count);
    EXPECT_FLOAT_EQ(gcStats.maxPause, mergedStats.maxPause);
    EXPECT_FLOAT_EQ(gcStats.minPause, mergedStats.minPause);
    EXPECT_FLOAT_EQ(gcStats.averagePause, mergedStats.totalPause / mergedStats.count);
}

HWTEST_F_L0(DFXJSNApiTests, GetGCStatisticMainThread007)
{
    auto expectStats = GCStats::MergeGCStatistic(vm_->GetEcmaGCStats()->GetGCStatistic(),
        ecmascript::SharedHeap::GetInstance()->GetEcmaGCStats()->GetGCStatistic());
    auto gcStats = DFXJSNApi::GetGCStatistic(vm_);

    EXPECT_STREQ(gcStats.lastType, expectStats.lastType);
    EXPECT_TRUE(std::strcmp(gcStats.lastType, "Local GC") == 0 ||
        std::strcmp(gcStats.lastType, "Shared GC") == 0 ||
        std::strcmp(gcStats.lastType, "UnknownType") == 0);
}

HWTEST_F_L0(DFXJSNApiTests, GetGCStatisticMainThread008)
{
    auto gcStats = DFXJSNApi::GetGCStatistic(vm_);

    EXPECT_GE(gcStats.count, 0U);
    EXPECT_GE(gcStats.maxPause, 0.0f);
    EXPECT_GE(gcStats.minPause, 0.0f);
    EXPECT_GE(gcStats.averagePause, 0.0f);
    EXPECT_GE(gcStats.lastStartTime, 0U);
    EXPECT_GE(gcStats.lastEndTime, 0U);
    EXPECT_TRUE(gcStats.lastType != nullptr);
}

HWTEST_F_L0(DFXJSNApiTests, GetGCStatisticMainThread009)
{
    if constexpr (G_USE_CMS_GC) {
        return;
    }
    if (g_isEnableCMCGC) {
        return;
    }

    auto heap = const_cast<Heap *>(vm_->GetHeap());
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    auto gcStats = DFXJSNApi::GetGCStatistic(vm_);

    EXPECT_GT(gcStats.count, 0U);
    EXPECT_GE(gcStats.maxPause, gcStats.minPause);
    EXPECT_GE(gcStats.averagePause, 0.0f);
    EXPECT_LE(gcStats.minPause, gcStats.maxPause);
    EXPECT_LE(gcStats.lastStartTime, gcStats.lastEndTime);
}

HWTEST_F_L0(DFXJSNApiTests, GetGCStatisticMainThread010)
{
    if constexpr (G_USE_CMS_GC) {
        return;
    }
    if (g_isEnableCMCGC) {
        return;
    }

    auto sharedHeap = ecmascript::SharedHeap::GetInstance();
    sharedHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread_);
    auto expectStats = GCStats::MergeGCStatistic(vm_->GetEcmaGCStats()->GetGCStatistic(),
        sharedHeap->GetEcmaGCStats()->GetGCStatistic());
    auto gcStats = DFXJSNApi::GetGCStatistic(vm_);

    EXPECT_EQ(gcStats.count, expectStats.count);
    EXPECT_FLOAT_EQ(gcStats.maxPause, expectStats.maxPause);
    EXPECT_FLOAT_EQ(gcStats.minPause, expectStats.minPause);
    EXPECT_FLOAT_EQ(gcStats.averagePause, expectStats.totalPause / expectStats.count);
    EXPECT_EQ(gcStats.lastStartTime, expectStats.lastStartTime);
    EXPECT_EQ(gcStats.lastEndTime, expectStats.lastEndTime);
    EXPECT_STREQ(gcStats.lastType, expectStats.lastType);
}

HWTEST_F_L0(DFXJSNApiTests, GetGCStatisticMainThread011)
{
    auto beforeStats = DFXJSNApi::GetGCStatistic(vm_);
    auto afterStats = DFXJSNApi::GetGCStatistic(vm_);

    EXPECT_EQ(afterStats.count, beforeStats.count);
    EXPECT_FLOAT_EQ(afterStats.maxPause, beforeStats.maxPause);
    EXPECT_FLOAT_EQ(afterStats.minPause, beforeStats.minPause);
    EXPECT_FLOAT_EQ(afterStats.averagePause, beforeStats.averagePause);
    EXPECT_EQ(afterStats.lastStartTime, beforeStats.lastStartTime);
    EXPECT_EQ(afterStats.lastEndTime, beforeStats.lastEndTime);
    EXPECT_STREQ(afterStats.lastType, beforeStats.lastType);
}

HWTEST_F_L0(DFXJSNApiTests, GetGCStatisticMainThread012)
{
    auto expectStats = GCStats::MergeGCStatistic(vm_->GetEcmaGCStats()->GetGCStatistic(),
        ecmascript::SharedHeap::GetInstance()->GetEcmaGCStats()->GetGCStatistic());
    auto gcStats = DFXJSNApi::GetGCStatistic(vm_);

    float expectedAveragePause = expectStats.count == 0 ? 0.0f : (expectStats.totalPause / expectStats.count);
    EXPECT_FLOAT_EQ(gcStats.averagePause, expectedAveragePause);
    EXPECT_EQ(gcStats.count == 0, expectedAveragePause == 0.0f);
}

HWTEST_F_L0(DFXJSNApiTests, GetGCStatisticDataLocalAggregation001)
{
    auto heap = const_cast<Heap *>(vm_->GetHeap());
    TestableDFXJSNApiTestsGCStats stats(heap);
    stats.SetRecordData(RecordData::YOUNG_COUNT, 2);
    stats.SetRecordDuration(RecordDuration::YOUNG_MIN_PAUSE, 2.0f);
    stats.SetRecordDuration(RecordDuration::YOUNG_MAX_PAUSE, 6.0f);
    stats.SetRecordDuration(RecordDuration::YOUNG_TOTAL_PAUSE, 7.0f);
    stats.SetRecordData(RecordData::OLD_COUNT, 1);
    stats.SetRecordDuration(RecordDuration::OLD_MIN_PAUSE, 4.0f);
    stats.SetRecordDuration(RecordDuration::OLD_MAX_PAUSE, 5.0f);
    stats.SetRecordDuration(RecordDuration::OLD_TOTAL_PAUSE, 5.0f);
    stats.SetGCTypeForTest(GCType::PARTIAL_OLD_GC);
    stats.RecordGCStatisticStart();
    stats.RecordGCStatisticEnd();

    auto gcStatistic = stats.GetGCStatistic();
    EXPECT_EQ(gcStatistic.count, 3U);
    EXPECT_FLOAT_EQ(gcStatistic.maxPause, 6.0f);
    EXPECT_FLOAT_EQ(gcStatistic.minPause, 2.0f);
    EXPECT_FLOAT_EQ(gcStatistic.totalPause, 12.0f);
    EXPECT_STREQ(gcStatistic.lastType, "Local GC");
}

HWTEST_F_L0(DFXJSNApiTests, GetGCStatisticDataSharedAggregation001)
{
    auto heap = const_cast<Heap *>(vm_->GetHeap());
    TestableDFXJSNApiTestsGCStats stats(heap);
    stats.SetRecordData(RecordData::SHARED_COUNT, 2);
    stats.SetRecordDuration(RecordDuration::SHARED_MIN_PAUSE, 1.0f);
    stats.SetRecordDuration(RecordDuration::SHARED_MAX_PAUSE, 8.0f);
    stats.SetRecordDuration(RecordDuration::SHARED_TOTAL_PAUSE, 9.0f);
    stats.SetRecordData(RecordData::SWEEP_COUNT, 3);
    stats.SetRecordDuration(RecordDuration::SWEEP_MIN_PAUSE, 2.0f);
    stats.SetRecordDuration(RecordDuration::SWEEP_MAX_PAUSE, 7.0f);
    stats.SetRecordDuration(RecordDuration::SWEEP_TOTAL_PAUSE, 12.0f);
    stats.SetGCTypeForTest(GCType::SHARED_FULL_GC);
    stats.RecordGCStatisticStart();
    stats.RecordGCStatisticEnd();

    auto gcStatistic = stats.GetGCStatistic();
    EXPECT_EQ(gcStatistic.count, 5U);
    EXPECT_FLOAT_EQ(gcStatistic.maxPause, 8.0f);
    EXPECT_FLOAT_EQ(gcStatistic.minPause, 1.0f);
    EXPECT_FLOAT_EQ(gcStatistic.totalPause, 21.0f);
    EXPECT_STREQ(gcStatistic.lastType, "Shared GC");
}

HWTEST_F_L0(DFXJSNApiTests, GetGCStatisticDataUnknownType001)
{
    auto heap = const_cast<Heap *>(vm_->GetHeap());
    TestableDFXJSNApiTestsGCStats stats(heap);
    stats.SetGCTypeForTest(GCType::OTHER);
    stats.RecordGCStatisticStart();
    stats.RecordGCStatisticEnd();

    auto gcStatistic = stats.GetGCStatistic();
    EXPECT_EQ(gcStatistic.count, 0U);
    EXPECT_FLOAT_EQ(gcStatistic.maxPause, 0.0f);
    EXPECT_FLOAT_EQ(gcStatistic.minPause, 0.0f);
    EXPECT_FLOAT_EQ(gcStatistic.totalPause, 0.0f);
    EXPECT_STREQ(gcStatistic.lastType, "UnknownType");
}

HWTEST_F_L0(DFXJSNApiTests, MergeGCStatisticForDFX001)
{
    GCStatisticData localStats;
    localStats.count = 2;
    localStats.maxPause = 6.0f;
    localStats.minPause = 2.0f;
    localStats.totalPause = 8.0f;
    localStats.lastStartTime = 10;
    localStats.lastEndTime = 30;
    localStats.lastType = "Local GC";

    GCStatisticData sharedStats;
    sharedStats.count = 4;
    sharedStats.maxPause = 7.0f;
    sharedStats.minPause = 1.0f;
    sharedStats.totalPause = 20.0f;
    sharedStats.lastStartTime = 50;
    sharedStats.lastEndTime = 60;
    sharedStats.lastType = "Shared GC";

    auto merged = GCStats::MergeGCStatistic(localStats, sharedStats);
    float averagePause = merged.count == 0 ? 0.0f : (merged.totalPause / merged.count);
    EXPECT_EQ(merged.count, 6U);
    EXPECT_FLOAT_EQ(merged.maxPause, 7.0f);
    EXPECT_FLOAT_EQ(merged.minPause, 1.0f);
    EXPECT_FLOAT_EQ(merged.totalPause, 28.0f);
    EXPECT_FLOAT_EQ(averagePause, 28.0f / 6.0f);
    EXPECT_STREQ(merged.lastType, "Shared GC");
}

HWTEST_F_L0(DFXJSNApiTests, MergeGCStatisticForDFX002)
{
    GCStatisticData localStats;
    localStats.count = 0;
    localStats.maxPause = 0.0f;
    localStats.minPause = 0.0f;
    localStats.totalPause = 0.0f;
    localStats.lastStartTime = 0;
    localStats.lastEndTime = 0;
    localStats.lastType = "UnknownType";

    GCStatisticData sharedStats;
    sharedStats.count = 0;
    sharedStats.maxPause = 0.0f;
    sharedStats.minPause = 0.0f;
    sharedStats.totalPause = 0.0f;
    sharedStats.lastStartTime = 0;
    sharedStats.lastEndTime = 0;
    sharedStats.lastType = "UnknownType";

    auto merged = GCStats::MergeGCStatistic(localStats, sharedStats);
    float averagePause = merged.count == 0 ? 0.0f : (merged.totalPause / merged.count);
    EXPECT_EQ(merged.count, 0U);
    EXPECT_FLOAT_EQ(merged.maxPause, 0.0f);
    EXPECT_FLOAT_EQ(merged.minPause, 0.0f);
    EXPECT_FLOAT_EQ(merged.totalPause, 0.0f);
    EXPECT_FLOAT_EQ(averagePause, 0.0f);
    EXPECT_STREQ(merged.lastType, "UnknownType");
}
} // namespace panda::test
