/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
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
        vm_->GetJSThread()->GetCurrentEcmaContext()->SetRuntimeStatEnable(true);
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
    const int dumpFormat = static_cast<int>(ecmascript::DumpFormat::JSON);
    const std::string filePath = "DFXJSNApiTests_json_001.heapsnapshot";
    std::fstream outputString(filePath, std::ios::out);
    outputString.close();
    outputString.clear();

    bool isVmMode = true;
    bool isPrivate = false;
    bool captureNumericValue = false;
    std::fstream inputFile {};
    EXPECT_TRUE(inputFile.good());

    DFXJSNApi::DumpHeapSnapshot(vm_, dumpFormat, filePath, isVmMode, isPrivate, captureNumericValue);
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
    const int dumpFormat = static_cast<int>(ecmascript::DumpFormat::JSON);
    const std::string filePath = "DFXJSNApiTests_json_002.heapsnapshot";
    std::fstream outputString(filePath, std::ios::out);
    outputString.close();
    outputString.clear();

    ecmascript::FileStream stream(filePath);
    EXPECT_TRUE(stream.Good());

    ecmascript::Progress *progress = nullptr;
    bool isVmMode = true;
    bool isPrivate = false;
    bool captureNumericValue = false;
    std::fstream fStream {};
    EXPECT_TRUE(fStream.good());

    DFXJSNApi::DumpHeapSnapshot(vm_, dumpFormat, &stream, progress, isVmMode, isPrivate, captureNumericValue);
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
    EcmaRuntimeStat *ecmaRuntimeStat = vm_->GetJSThread()->GetCurrentEcmaContext()->GetRuntimeStat();
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
    size_t expectHeapUsedSize = heap->GetHeapObjectSize();
    EXPECT_EQ(heapUsedSize, expectHeapUsedSize);
}

HWTEST_F_L0(DFXJSNApiTests, NotifyApplicationState)
{
    auto heap = vm_->GetHeap();
    auto concurrentMarker = heap->GetConcurrentMarker();
    auto sweeper = heap->GetSweeper();

    DFXJSNApi::NotifyApplicationState(vm_, false);
#if !ECMASCRIPT_DISABLE_CONCURRENT_MARKING
    EXPECT_TRUE(!concurrentMarker->IsDisabled());
#endif
    EXPECT_TRUE(!sweeper->IsDisabled());

    const_cast<ecmascript::Heap *>(heap)->CollectGarbage(TriggerGCType::OLD_GC, GCReason::OTHER);
    DFXJSNApi::NotifyApplicationState(vm_, true);
    EXPECT_TRUE(concurrentMarker->IsDisabled());
    EXPECT_TRUE(sweeper->IsRequestDisabled());
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
} // namespace panda::test
