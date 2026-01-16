/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <chrono>
#include <thread>

#include "assembler/assembly-emitter.h"
#include "assembler/assembly-parser.h"
#include "ecmascript/builtins/builtins_ark_tools.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/full_gc.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/partial_gc.h"
#include "ecmascript/mem/gc_key_stats.h"
#include "ecmascript/mem/gc_stats.h"
#include "ecmascript/mem/long_gc_stats.h"
#include "ecmascript/platform/dfx_hisys_event.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/tests/ecma_test_common.h"
#include "ecmascript/mem/mem_controller.h"

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {

class GCKeyStatsTest : public BaseTestWithScope<false> {
public:
    void SetUp() override
    {
        JSRuntimeOptions options;
        instance = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        thread->ManagedCodeBegin();
        scope = new EcmaHandleScope(thread);
    }
};

HWTEST_F_L0(GCKeyStatsTest, ProcessLongGCEventTest001)
{
    JSRuntimeOptions& options = thread->GetEcmaVM()->GetJSOptions();
    options.SetEnableDFXHiSysEvent(false);
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    GCStats *gcStats = new GCStats(heap);
    LongGCStats *longGCStats = gcStats->GetLongGCStats();
    longGCStats->SetGCIsSensitive(true);
    longGCStats->SetGCIsInBackground(false);
    longGCStats->SetGCTotalTime(34.0f);
    gcStats->SetGCReason(GCReason::IDLE);
    auto gcKeyStats = std::make_unique<GCKeyStats>(heap, gcStats);
    gcKeyStats->ProcessLongGCEvent();
    EXPECT_TRUE(longGCStats->GetGCIsSensitive());
    delete gcStats;
}

HWTEST_F_L0(GCKeyStatsTest, ProcessLongGCEventTest002)
{
    JSRuntimeOptions& options = thread->GetEcmaVM()->GetJSOptions();
    options.SetEnableDFXHiSysEvent(true);
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    GCStats *gcStats = new GCStats(heap);
    LongGCStats *longGCStats = gcStats->GetLongGCStats();
    longGCStats->SetGCIsSensitive(true);
    longGCStats->SetGCIsInBackground(false);
    longGCStats->SetGCTotalTime(32.0f);
    gcStats->SetGCReason(GCReason::IDLE);
    auto gcKeyStats = std::make_unique<GCKeyStats>(heap, gcStats);
    gcKeyStats->ProcessLongGCEvent();
    EXPECT_TRUE(longGCStats->GetGCIsSensitive());
    delete gcStats;
}

HWTEST_F_L0(GCKeyStatsTest, ProcessLongGCEventTest003)
{
    JSRuntimeOptions& options = thread->GetEcmaVM()->GetJSOptions();
    options.SetEnableDFXHiSysEvent(true);
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    GCStats *gcStats = new GCStats(heap);
    LongGCStats *longGCStats = gcStats->GetLongGCStats();
    longGCStats->SetGCIsSensitive(false);
    longGCStats->SetGCIsInBackground(false);
    longGCStats->SetGCTotalTime(201.0f);
    gcStats->SetGCReason(GCReason::IDLE);
    auto gcKeyStats = std::make_unique<GCKeyStats>(heap, gcStats);
    gcKeyStats->ProcessLongGCEvent();
    EXPECT_FALSE(longGCStats->GetGCIsInBackground());
    delete gcStats;
}

HWTEST_F_L0(GCKeyStatsTest, ProcessLongGCEventTest004)
{
    JSRuntimeOptions& options = thread->GetEcmaVM()->GetJSOptions();
    options.SetEnableDFXHiSysEvent(true);
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    GCStats *gcStats = new GCStats(heap);
    LongGCStats *longGCStats = gcStats->GetLongGCStats();
    longGCStats->SetGCIsSensitive(false);
    longGCStats->SetGCIsInBackground(false);
    longGCStats->SetGCTotalTime(201.0f);
    gcStats->SetGCReason(GCReason::IDLE_NATIVE);
    auto gcKeyStats = std::make_unique<GCKeyStats>(heap, gcStats);
    gcKeyStats->ProcessLongGCEvent();
    EXPECT_FALSE(longGCStats->GetGCIsInBackground());
    delete gcStats;
}

HWTEST_F_L0(GCKeyStatsTest, ProcessLongGCEventTest005)
{
    JSRuntimeOptions& options = thread->GetEcmaVM()->GetJSOptions();
    options.SetEnableDFXHiSysEvent(true);
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    GCStats *gcStats = new GCStats(heap);
    LongGCStats *longGCStats = gcStats->GetLongGCStats();
    longGCStats->SetGCIsSensitive(false);
    longGCStats->SetGCIsInBackground(false);
    longGCStats->SetGCTotalTime(199.0f);
    gcStats->SetGCReason(GCReason::IDLE);
    auto gcKeyStats = std::make_unique<GCKeyStats>(heap, gcStats);
    gcKeyStats->ProcessLongGCEvent();
    EXPECT_FALSE(longGCStats->GetGCIsInBackground());
    delete gcStats;
}

HWTEST_F_L0(GCKeyStatsTest, ProcessLongGCEventTest006)
{
    JSRuntimeOptions& options = thread->GetEcmaVM()->GetJSOptions();
    options.SetEnableDFXHiSysEvent(true);
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    GCStats *gcStats = new GCStats(heap);
    LongGCStats *longGCStats = gcStats->GetLongGCStats();
    longGCStats->SetGCIsSensitive(false);
    longGCStats->SetGCIsInBackground(false);
    longGCStats->SetGCTotalTime(199.0f);
    gcStats->SetGCReason(GCReason::IDLE);
    auto gcKeyStats = std::make_unique<GCKeyStats>(heap, gcStats);
    gcKeyStats->ProcessLongGCEvent();
    EXPECT_FALSE(longGCStats->GetGCIsInBackground());
    delete gcStats;
}

HWTEST_F_L0(GCKeyStatsTest, ProcessLongGCEventTest007)
{
    JSRuntimeOptions& options = thread->GetEcmaVM()->GetJSOptions();
    options.SetEnableDFXHiSysEvent(true);
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    GCStats *gcStats = new GCStats(heap);
    LongGCStats *longGCStats = gcStats->GetLongGCStats();
    longGCStats->SetGCIsSensitive(false);
    longGCStats->SetGCIsInBackground(true);
    longGCStats->SetGCTotalTime(501.0f);
    gcStats->SetGCReason(GCReason::IDLE);
    auto gcKeyStats = std::make_unique<GCKeyStats>(heap, gcStats);
    gcKeyStats->ProcessLongGCEvent();
    EXPECT_FALSE(longGCStats->GetGCIsSensitive());
    delete gcStats;
}

HWTEST_F_L0(GCKeyStatsTest, ProcessLongGCEventTest008)
{
    JSRuntimeOptions& options = thread->GetEcmaVM()->GetJSOptions();
    options.SetEnableDFXHiSysEvent(true);
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    GCStats *gcStats = new GCStats(heap);
    LongGCStats *longGCStats = gcStats->GetLongGCStats();
    longGCStats->SetGCIsSensitive(false);
    longGCStats->SetGCIsInBackground(true);
    longGCStats->SetGCTotalTime(501.0f);
    gcStats->SetGCReason(GCReason::IDLE_NATIVE);
    auto gcKeyStats = std::make_unique<GCKeyStats>(heap, gcStats);
    gcKeyStats->ProcessLongGCEvent();
    EXPECT_FALSE(longGCStats->GetGCIsSensitive());
    delete gcStats;
}

HWTEST_F_L0(GCKeyStatsTest, ProcessLongGCEventTest009)
{
    JSRuntimeOptions& options = thread->GetEcmaVM()->GetJSOptions();
    options.SetEnableDFXHiSysEvent(true);
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    GCStats *gcStats = new GCStats(heap);
    LongGCStats *longGCStats = gcStats->GetLongGCStats();
    longGCStats->SetGCIsSensitive(false);
    longGCStats->SetGCIsInBackground(true);
    longGCStats->SetGCTotalTime(499.0f);
    gcStats->SetGCReason(GCReason::IDLE);
    auto gcKeyStats = std::make_unique<GCKeyStats>(heap, gcStats);
    gcKeyStats->ProcessLongGCEvent();
    EXPECT_TRUE(longGCStats->GetGCIsInBackground());
    delete gcStats;
}

HWTEST_F_L0(GCKeyStatsTest, ProcessLongGCEventTest010)
{
    JSRuntimeOptions& options = thread->GetEcmaVM()->GetJSOptions();
    options.SetEnableDFXHiSysEvent(true);
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    GCStats *gcStats = new GCStats(heap);
    LongGCStats *longGCStats = gcStats->GetLongGCStats();
    longGCStats->SetGCIsSensitive(false);
    longGCStats->SetGCIsInBackground(true);
    longGCStats->SetGCTotalTime(499.0f);
    gcStats->SetGCReason(GCReason::IDLE_NATIVE);
    auto gcKeyStats = std::make_unique<GCKeyStats>(heap, gcStats);
    gcKeyStats->ProcessLongGCEvent();
    EXPECT_TRUE(longGCStats->GetGCIsInBackground());
    delete gcStats;
}

HWTEST_F_L0(GCKeyStatsTest, ProcessLongGCEventTest011)
{
    JSRuntimeOptions& options = thread->GetEcmaVM()->GetJSOptions();
    options.SetEnableDFXHiSysEvent(true);
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    GCStats *gcStats = new GCStats(heap);
    LongGCStats *longGCStats = gcStats->GetLongGCStats();
    longGCStats->SetGCIsSensitive(false);
    longGCStats->SetGCIsInBackground(false);
    longGCStats->SetGCTotalTime(34.0f);
    gcStats->SetGCReason(GCReason::OTHER);
    auto gcKeyStats = std::make_unique<GCKeyStats>(heap, gcStats);
    gcKeyStats->ProcessLongGCEvent();
    EXPECT_FALSE(longGCStats->GetGCIsSensitive());
    delete gcStats;
}

HWTEST_F_L0(GCKeyStatsTest, ProcessLongGCEventTest012)
{
    JSRuntimeOptions& options = thread->GetEcmaVM()->GetJSOptions();
    options.SetEnableDFXHiSysEvent(true);
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    GCStats *gcStats = new GCStats(heap);
    LongGCStats *longGCStats = gcStats->GetLongGCStats();
    longGCStats->SetGCIsSensitive(false);
    longGCStats->SetGCIsInBackground(false);
    longGCStats->SetGCTotalTime(32.0f);
    gcStats->SetGCReason(GCReason::OTHER);
    auto gcKeyStats = std::make_unique<GCKeyStats>(heap, gcStats);
    gcKeyStats->ProcessLongGCEvent();
    EXPECT_FALSE(longGCStats->GetGCIsInBackground());
    delete gcStats;
}

HWTEST_F_L0(GCKeyStatsTest, ProcessLongGCEventTest013)
{
    JSRuntimeOptions& options = thread->GetEcmaVM()->GetJSOptions();
    options.SetEnableDFXHiSysEvent(true);
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    GCStats *gcStats = new GCStats(heap);
    LongGCStats *longGCStats = gcStats->GetLongGCStats();
    longGCStats->SetGCIsSensitive(false);
    longGCStats->SetGCIsInBackground(true);
    longGCStats->SetGCTotalTime(201.0f);
    gcStats->SetGCReason(GCReason::OTHER);
    auto gcKeyStats = std::make_unique<GCKeyStats>(heap, gcStats);
    gcKeyStats->ProcessLongGCEvent();
    EXPECT_FALSE(longGCStats->GetGCIsSensitive());
    delete gcStats;
}

HWTEST_F_L0(GCKeyStatsTest, ProcessLongGCEventTest014)
{
    JSRuntimeOptions& options = thread->GetEcmaVM()->GetJSOptions();
    options.SetEnableDFXHiSysEvent(true);
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    GCStats *gcStats = new GCStats(heap);
    LongGCStats *longGCStats = gcStats->GetLongGCStats();
    longGCStats->SetGCIsSensitive(false);
    longGCStats->SetGCIsInBackground(true);
    longGCStats->SetGCTotalTime(199.0f);
    gcStats->SetGCReason(GCReason::OTHER);
    auto gcKeyStats = std::make_unique<GCKeyStats>(heap, gcStats);
    gcKeyStats->ProcessLongGCEvent();
    EXPECT_TRUE(longGCStats->GetGCIsInBackground());
    delete gcStats;
}
}  // namespace panda::test
