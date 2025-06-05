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
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/concurrent_sweeper.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/dfx/cpu_profiler/samples_record.h"
#include "ecmascript/dfx/tracing/tracing.h"

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
#if defined(ENABLE_EXCEPTION_BACKTRACE)
    EXPECT_FALSE(stackTraceStr.empty());
    EXPECT_TRUE(result);
#else
    EXPECT_TRUE(stackTraceStr.empty());
    EXPECT_FALSE(result);
#endif
}

HWTEST_F_L0(DFXJSNApiTests, BuildJsStackTrace)
{
    std::string stackTraceStr = "stack_trace_str";
    bool result = DFXJSNApi::BuildJsStackTrace(vm_, stackTraceStr);
#if defined(ENABLE_EXCEPTION_BACKTRACE)
    EXPECT_FALSE(stackTraceStr.empty());
    EXPECT_TRUE(result);
#else
    EXPECT_TRUE(stackTraceStr.empty());
    EXPECT_FALSE(result);
#endif
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
    auto heap = vm_->GetHeap();
    size_t arrayBufferSize = DFXJSNApi::GetArrayBufferSize(vm_);
    size_t expectArrayBufferSize = heap->GetArrayBufferSize();
    EXPECT_EQ(arrayBufferSize, expectArrayBufferSize);

    size_t heapTotalSize = DFXJSNApi::GetHeapTotalSize(vm_);
    size_t expectHeapTotalSize = heap->GetCommittedSize();
    EXPECT_EQ(heapTotalSize, expectHeapTotalSize);

    size_t heapUsedSize = DFXJSNApi::GetHeapUsedSize(vm_);
    size_t expectHeapUsedSize = heap->GetLiveObjectSize();
    EXPECT_EQ(heapUsedSize, expectHeapUsedSize);

    size_t heapObjectSize = DFXJSNApi::GetHeapObjectSize(vm_);
    size_t expectHeapObjectSize = heap->GetHeapObjectSize();
    EXPECT_EQ(heapObjectSize, expectHeapObjectSize);

    size_t processHeapLimitSize = DFXJSNApi::GetProcessHeapLimitSize();
    EXPECT_GE(processHeapLimitSize, heap->GetEcmaParamConfiguration().GetMaxHeapSize());
    EXPECT_LE(processHeapLimitSize, MAX_MEM_POOL_CAPACITY);
}

HWTEST_F_L0(DFXJSNApiTests, DFXJSNApiForGCInfo)
{
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
        for (int i = 0; i < 10240; i++) {
            factory->NewTaggedArray(512, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
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

HWTEST_F_L0(DFXJSNApiTests, NotifyIdleStatusControl)
{
    bool receivedValue = false;
    std::function<void(bool)> cb = [&](bool value) {
        receivedValue = value;
    };
    DFXJSNApi::NotifyIdleStatusControl(vm_, cb);
    const_cast<ecmascript::Heap *>(vm_->GetHeap())->DisableNotifyIdle();
    EXPECT_TRUE(receivedValue);
}

HWTEST_F_L0(DFXJSNApiTests, NotifyIdleTime)
{
    if (g_isEnableCMCGC) {
        return;
    }
    auto heap = const_cast<ecmascript::Heap *>(vm_->GetHeap());
    heap->SetIdleTask(IdleTaskType::YOUNG_GC);
    DFXJSNApi::NotifyIdleTime(vm_, 10);
    EXPECT_EQ(vm_->GetEcmaGCStats()->GetGCReason(), GCReason::IDLE);
}

HWTEST_F_L0(DFXJSNApiTests, NotifyHighSensitive)
{
    auto heap = const_cast<ecmascript::Heap *>(vm_->GetHeap());
    DFXJSNApi::NotifyHighSensitive(vm_, true);
    EXPECT_TRUE(heap->GetSensitiveStatus() == AppSensitiveStatus::ENTER_HIGH_SENSITIVE);
    DFXJSNApi::NotifyHighSensitive(vm_, false);
    EXPECT_TRUE(heap->GetSensitiveStatus() == AppSensitiveStatus::EXIT_HIGH_SENSITIVE);
}

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
} // namespace panda::test
