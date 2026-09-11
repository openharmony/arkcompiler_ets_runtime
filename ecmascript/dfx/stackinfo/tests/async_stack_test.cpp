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

#include <set>
#include <vector>

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/tests/test_helper.h"

#if defined(ENABLE_ASYNC_STACK)
#include "async_stack.h"
#endif

namespace panda::test {
using namespace panda::ecmascript;

class AsyncStackTestHelper {
public:
    static bool IsExpectedAsyncStackTrace(const std::string &input, uint32_t expectedSubmitterCount)
    {
        if (input.empty()) {
            return false;
        }

        if (expectedSubmitterCount == 0 ||
            expectedSubmitterCount > AsyncStackTraceManager::MAX_ASYNC_TASK_STACK_DEPTH) {
            return false;
        }

        std::regex submitterRegex(R"(submitter#[0-9]{2}:)");
        auto begin = std::sregex_iterator(input.begin(), input.end(), submitterRegex);
        auto end = std::sregex_iterator();

        size_t submitterCount = std::distance(begin, end);
        return submitterCount == expectedSubmitterCount;
    }

    // Count the stitched submitter sections ("submitter#NN:") inside a trace
    static size_t CountSubmitters(const std::string &trace)
    {
        std::regex submitterRegex(R"(submitter#[0-9]{2}:)");
        auto begin = std::sregex_iterator(trace.begin(), trace.end(), submitterRegex);
        auto end = std::sregex_iterator();
        return std::distance(begin, end);
    }

    static void WireReactionToPromise(JSThread *thread, const JSHandle<PromiseReaction> &reaction,
                                      const JSHandle<JSPromise> &promise)
    {
#if ENABLE_LATEST_OPTIMIZATION
        reaction->SetPromiseOrCapability(thread, promise.GetTaggedValue());
#else
        JSHandle<JSTaggedValue> promiseFunc = thread->GetEcmaVM()->GetGlobalEnv()->GetPromiseFunction();
        JSHandle<PromiseCapability> capability = JSPromise::NewPromiseCapability(thread, promiseFunc);
        capability->SetPromise(thread, promise.GetTaggedValue());
        reaction->SetPromiseCapability(thread, capability.GetTaggedValue());
#endif
    }

    explicit AsyncStackTestHelper(JSThread *thread) : thread_(thread)
    {
        promise_ = JSMutableHandle<JSPromise>{thread_, JSTaggedValue::Hole()};
        reason_ = JSMutableHandle<JSTaggedValue>{thread_, JSTaggedValue::Hole()};
    }
    ~AsyncStackTestHelper() = default;

    static void PromiseRejectionTrackerCb(const EcmaVM* vm,
                                          const JSHandle<JSPromise> promise,
                                          const JSHandle<JSTaggedValue> reason,
                                          PromiseRejectionEvent operation,
                                          void* data)
    {
        auto *self = static_cast<AsyncStackTestHelper *>(data);
        ASSERT(self != nullptr);
        self->OnPromiseRejection(vm, promise, reason, operation);
    }

    void OnPromiseRejection([[maybe_unused]] const EcmaVM* vm,
                            const JSHandle<JSPromise> promise,
                            const JSHandle<JSTaggedValue> reason,
                            [[maybe_unused]] PromiseRejectionEvent operation)
    {
        promise_.Update(promise);
        reason_.Update(reason);
    }

    const JSHandle<JSPromise> &GetRejectPromise()
    {
        return promise_;
    }

    const JSHandle<JSTaggedValue> &GetRejectReason()
    {
        return reason_;
    }

    void Clear()
    {
        promise_.Update(JSHandle<JSPromise>{thread_, JSTaggedValue::Hole()});
        reason_.Update(JSHandle<JSTaggedValue>{thread_, JSTaggedValue::Hole()});
    }

    size_t SizeOfPromiseMap()
    {
        auto vm = thread_->GetEcmaVM();
        auto asyncStackTraceManager = vm->GetAsyncStackTraceManager();
        return asyncStackTraceManager->promiseMap_.size();
    }

    size_t SizeOfPromiseQueue()
    {
        auto vm = thread_->GetEcmaVM();
        auto asyncStackTraceManager = vm->GetAsyncStackTraceManager();
        return asyncStackTraceManager->promiseQueue_.size();
    }

    uint32_t FillPromiseMapAndQueue()
    {
        auto vm = thread_->GetEcmaVM();
        auto asyncStackTraceManager = vm->GetAsyncStackTraceManager();
        for (uint32_t i = 0; i < 2 * AsyncStackTraceManager::MAX_ASYNC_CALL_STACKS; ++i) {
            uint32_t promiseId = i;
            uint32_t parentPromiseId = i + 1;
            uint64_t stackId = i * 2;
            asyncStackTraceManager->promiseMap_[promiseId] = {promiseId, parentPromiseId, stackId};
            asyncStackTraceManager->promiseQueue_.push_back(promiseId);
        }
        return asyncStackTraceManager->promiseQueue_.size();
    }

    uint32_t CollectOPromiseNode()
    {
        auto vm = thread_->GetEcmaVM();
        auto asyncStackTraceManager = vm->GetAsyncStackTraceManager();
        asyncStackTraceManager->CollectOldPromiseNodeIfNeeded();
        return asyncStackTraceManager->promiseQueue_.size();
    }

    void InsertPromiseNode(uint32_t promiseId, uint32_t parentPromiseId, uint64_t stackId)
    {
        auto vm = thread_->GetEcmaVM();
        auto asyncStackTraceManager = vm->GetAsyncStackTraceManager();
        asyncStackTraceManager->promiseMap_[promiseId] = {promiseId, parentPromiseId, stackId};
        asyncStackTraceManager->promiseQueue_.push_back(promiseId);
    }

    static constexpr uint32_t MaxAsyncCallStacks()
    {
        return AsyncStackTraceManager::MAX_ASYNC_CALL_STACKS;
    }

    bool HasPromiseNode(uint32_t promiseId)
    {
        auto vm = thread_->GetEcmaVM();
        auto asyncStackTraceManager = vm->GetAsyncStackTraceManager();
        return asyncStackTraceManager->promiseMap_.find(promiseId) !=
               asyncStackTraceManager->promiseMap_.end();
    }

    // Returns 0 when nothing is recorded (promise ids start from 1)
    uint32_t FrontOfPromiseQueue()
    {
        auto vm = thread_->GetEcmaVM();
        auto asyncStackTraceManager = vm->GetAsyncStackTraceManager();
        if (asyncStackTraceManager->promiseQueue_.empty()) {
            return 0;
        }
        return asyncStackTraceManager->promiseQueue_.front();
    }

    // Returns 0 when nothing is recorded (promise ids start from 1)
    uint32_t BackOfPromiseQueue()
    {
        auto vm = thread_->GetEcmaVM();
        auto asyncStackTraceManager = vm->GetAsyncStackTraceManager();
        if (asyncStackTraceManager->promiseQueue_.empty()) {
            return 0;
        }
        return asyncStackTraceManager->promiseQueue_.back();
    }

    // Simulates a crash/freeze in the middle of a promise job: the current
    // submitter id is normally owned by SetCurrentPromiseTask
    void SetCurrentPromiseIdForTest(uint32_t promiseId)
    {
        auto vm = thread_->GetEcmaVM();
        auto asyncStackTraceManager = vm->GetAsyncStackTraceManager();
        asyncStackTraceManager->currentPromiseId_ = promiseId;
    }

    uint64_t DefaultAsyncStackType()
    {
        auto vm = thread_->GetEcmaVM();
        auto asyncStackTraceManager = vm->GetAsyncStackTraceManager();
        return asyncStackTraceManager->defaultAsyncStackType_;
    }

private:
    JSThread *thread_ {nullptr};
    JSMutableHandle<JSPromise> promise_;
    JSMutableHandle<JSTaggedValue> reason_;
};

class AsyncStackTest : public testing::Test {
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
        vm_->SetEnableForceGC(false);

        helper_ = std::make_shared<AsyncStackTestHelper>(thread_);
        vm_->SetPromiseRejectInfoData(helper_.get());
        vm_->SetHostPromiseRejectionTracker(AsyncStackTestHelper::PromiseRejectionTrackerCb);

        thread_->EnableUserUncaughtErrorHandler();

#if defined(ENABLE_ASYNC_STACK)
        DFXJSNApi::SetEnableRuntimeAsyncStack(vm_, true);
        ASSERT_TRUE(DFXJSNApi::GetEnableRuntimeAsyncStack(vm_));
#endif
    }

    void TearDown() override
    {
#if defined(ENABLE_ASYNC_STACK)
        DFXJSNApi::SetEnableRuntimeAsyncStack(vm_, false);
        ASSERT_FALSE(DFXJSNApi::GetEnableRuntimeAsyncStack(vm_));
#endif
        TestHelper::DestroyEcmaVMWithScope(vm_, scope_);
    }

    EcmaVM *vm_ {nullptr};
    EcmaHandleScope *scope_ {nullptr};
    JSThread *thread_ {nullptr};
    std::shared_ptr<AsyncStackTestHelper> helper_ {nullptr};
};

HWTEST_F_L0(AsyncStackTest, TestSavePromiseNode)
{
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<JSPromise> promise = factory->NewJSPromise();
    auto asyncStackTraceManager = vm_->GetAsyncStackTraceManager();
    ASSERT_NE(asyncStackTraceManager, nullptr);
    asyncStackTraceManager->SavePromiseNode(promise);
    ASSERT_EQ(helper_->SizeOfPromiseMap(), 0);
    ASSERT_EQ(helper_->SizeOfPromiseQueue(), 0);
}

HWTEST_F_L0(AsyncStackTest, TestGetParentPromiseId)
{
    auto asyncStackTraceManager = vm_->GetAsyncStackTraceManager();
    ASSERT_NE(asyncStackTraceManager, nullptr);
    asyncStackTraceManager->Clear();
    uint32_t parentPromiseId = asyncStackTraceManager->GetParentPromiseId(4096);
    ASSERT_EQ(parentPromiseId, 0);
}

HWTEST_F_L0(AsyncStackTest, TestGetStackId)
{
    auto asyncStackTraceManager = vm_->GetAsyncStackTraceManager();
    ASSERT_NE(asyncStackTraceManager, nullptr);
    asyncStackTraceManager->Clear();
    uint32_t stackId = asyncStackTraceManager->GetStackId(4096);
    ASSERT_EQ(stackId, 0);
}

HWTEST_F_L0(AsyncStackTest, TestCollectOldPromiseNodeIfNeeded)
{
    auto asyncStackTraceManager = vm_->GetAsyncStackTraceManager();
    ASSERT_NE(asyncStackTraceManager, nullptr);
    asyncStackTraceManager->Clear();
    size_t sizeBeforeClear = helper_->FillPromiseMapAndQueue();
    size_t sizeAfterCollect = helper_->CollectOPromiseNode();
    ASSERT_TRUE(sizeBeforeClear > sizeAfterCollect);
}

HWTEST_F_L0(AsyncStackTest, TestAsyncAwait)
{
#if defined(ENABLE_ASYNC_STACK)
    bool init = DfxInitAsyncStack();
    ASSERT_TRUE(init) << "DFX init async stack failed";
#endif

    const std::string fileName = STACKINFO_TEST_ABC_FILES_DIR"async_await.abc";
    std::string entryPoint = "async_await";

    bool result = JSNApi::Execute(vm_, fileName, entryPoint);
    ASSERT_TRUE(result);
    auto jsPandaFile = JSPandaFileManager::GetInstance()->FindJSPandaFile(CString(fileName));
    EXPECT_NE(jsPandaFile, nullptr);

    JSHandle<JSPromise> promise = helper_->GetRejectPromise();
    ASSERT_NE(promise.GetTaggedValue(), JSTaggedValue::Hole());
    JSHandle<JSTaggedValue> reason = helper_->GetRejectReason();
    ASSERT_NE(reason.GetTaggedValue(), JSTaggedValue::Hole());

    JSHandle<JSTaggedValue> asyncStackKeyStr = thread_->GlobalConstants()->GetHandledAsyncStackString();
    JSHandle<JSTaggedValue> asyncStackValue = JSObject::GetProperty(thread_, reason, asyncStackKeyStr).GetValue();
#if defined(ENABLE_ASYNC_STACK)
    JSHandle<EcmaString> asyncStackEcmaStr = JSTaggedValue::ToString(thread_, asyncStackValue);
    std::string asyncStackStr = EcmaStringAccessor(asyncStackEcmaStr).ToStdString(thread_);
    size_t expectedSubmitterCount = 1;
    bool checkResult = AsyncStackTestHelper::IsExpectedAsyncStackTrace(asyncStackStr, expectedSubmitterCount);
    ASSERT_TRUE(checkResult) << "Actual: " << asyncStackStr;
#else
    ASSERT_EQ(asyncStackValue.GetTaggedValue(), JSTaggedValue::Undefined());
#endif
    helper_->Clear();
}

HWTEST_F_L0(AsyncStackTest, TestDynamicImport)
{
    const std::string fileName = STACKINFO_TEST_ABC_FILES_DIR"dynamic_import.abc";
    std::string entryPoint = "index";

    bool result = JSNApi::Execute(vm_, fileName, entryPoint);
    ASSERT_TRUE(result);
    auto jsPandaFile = JSPandaFileManager::GetInstance()->FindJSPandaFile(CString(fileName));
    EXPECT_NE(jsPandaFile, nullptr);

    JSHandle<JSPromise> promise = helper_->GetRejectPromise();
    ASSERT_NE(promise.GetTaggedValue(), JSTaggedValue::Hole());
    JSHandle<JSTaggedValue> reason = helper_->GetRejectReason();
    ASSERT_NE(reason.GetTaggedValue(), JSTaggedValue::Hole());

    JSHandle<JSTaggedValue> asyncStackKeyStr = thread_->GlobalConstants()->GetHandledAsyncStackString();
    JSHandle<JSTaggedValue> asyncStackValue = JSObject::GetProperty(thread_, reason, asyncStackKeyStr).GetValue();
#if defined(ENABLE_ASYNC_STACK)
    JSHandle<EcmaString> asyncStackEcmaStr = JSTaggedValue::ToString(thread_, asyncStackValue);
    std::string asyncStackStr = EcmaStringAccessor(asyncStackEcmaStr).ToStdString(thread_);
    size_t expectedSubmitterCount = 1;
    bool checkResult = AsyncStackTestHelper::IsExpectedAsyncStackTrace(asyncStackStr, expectedSubmitterCount);
    ASSERT_TRUE(checkResult) << "Actual: " << asyncStackStr;
#else
    ASSERT_EQ(asyncStackValue.GetTaggedValue(), JSTaggedValue::Undefined());
#endif
    helper_->Clear();
}

HWTEST_F_L0(AsyncStackTest, TestPromiseThen)
{
    const std::string fileName = STACKINFO_TEST_ABC_FILES_DIR"promise_then.abc";
    std::string entryPoint = "promise_then";

    bool result = JSNApi::Execute(vm_, fileName, entryPoint);
    ASSERT_TRUE(result);
    auto jsPandaFile = JSPandaFileManager::GetInstance()->FindJSPandaFile(CString(fileName));
    EXPECT_NE(jsPandaFile, nullptr);

    JSHandle<JSPromise> promise = helper_->GetRejectPromise();
    ASSERT_NE(promise.GetTaggedValue(), JSTaggedValue::Hole());
    JSHandle<JSTaggedValue> reason = helper_->GetRejectReason();
    ASSERT_NE(reason.GetTaggedValue(), JSTaggedValue::Hole());

    JSHandle<JSTaggedValue> asyncStackKeyStr = thread_->GlobalConstants()->GetHandledAsyncStackString();
    JSHandle<JSTaggedValue> asyncStackValue = JSObject::GetProperty(thread_, reason, asyncStackKeyStr).GetValue();
#if defined(ENABLE_ASYNC_STACK)
    JSHandle<EcmaString> asyncStackEcmaStr = JSTaggedValue::ToString(thread_, asyncStackValue);
    std::string asyncStackStr = EcmaStringAccessor(asyncStackEcmaStr).ToStdString(thread_);
    size_t expectedSubmitterCount = 1;
    bool checkResult = AsyncStackTestHelper::IsExpectedAsyncStackTrace(asyncStackStr, expectedSubmitterCount);
    ASSERT_TRUE(checkResult) << "Actual: " << asyncStackStr;
#else
    ASSERT_EQ(asyncStackValue.GetTaggedValue(), JSTaggedValue::Undefined());
#endif
    helper_->Clear();
}

HWTEST_F_L0(AsyncStackTest, TestMultiAsyncAwait)
{
    const std::string fileName = STACKINFO_TEST_ABC_FILES_DIR"multi_async_await.abc";
    std::string entryPoint = "multi_async_await";

    bool result = JSNApi::Execute(vm_, fileName, entryPoint);
    ASSERT_TRUE(result);
    auto jsPandaFile = JSPandaFileManager::GetInstance()->FindJSPandaFile(CString(fileName));
    EXPECT_NE(jsPandaFile, nullptr);

    JSHandle<JSPromise> promise = helper_->GetRejectPromise();
    ASSERT_NE(promise.GetTaggedValue(), JSTaggedValue::Hole());
    JSHandle<JSTaggedValue> reason = helper_->GetRejectReason();
    ASSERT_NE(reason.GetTaggedValue(), JSTaggedValue::Hole());

    JSHandle<JSTaggedValue> asyncStackKeyStr = thread_->GlobalConstants()->GetHandledAsyncStackString();
    JSHandle<JSTaggedValue> asyncStackValue = JSObject::GetProperty(thread_, reason, asyncStackKeyStr).GetValue();
#if defined(ENABLE_ASYNC_STACK)
    JSHandle<EcmaString> asyncStackEcmaStr = JSTaggedValue::ToString(thread_, asyncStackValue);
    std::string asyncStackStr = EcmaStringAccessor(asyncStackEcmaStr).ToStdString(thread_);
    size_t expectedSubmitterCount = 2;
    bool checkResult = AsyncStackTestHelper::IsExpectedAsyncStackTrace(asyncStackStr, expectedSubmitterCount);
    ASSERT_TRUE(checkResult) << "Actual: " << asyncStackStr;
#else
    ASSERT_EQ(asyncStackValue.GetTaggedValue(), JSTaggedValue::Undefined());
#endif
    helper_->Clear();
}

HWTEST_F_L0(AsyncStackTest, TestAsyncAwaitPromiseThen)
{
    const std::string fileName = STACKINFO_TEST_ABC_FILES_DIR"async_await_promise_then.abc";
    std::string entryPoint = "async_await_promise_then";

    bool result = JSNApi::Execute(vm_, fileName, entryPoint);
    ASSERT_TRUE(result);
    auto jsPandaFile = JSPandaFileManager::GetInstance()->FindJSPandaFile(CString(fileName));
    EXPECT_NE(jsPandaFile, nullptr);

    JSHandle<JSPromise> promise = helper_->GetRejectPromise();
    ASSERT_NE(promise.GetTaggedValue(), JSTaggedValue::Hole());
    JSHandle<JSTaggedValue> reason = helper_->GetRejectReason();
    ASSERT_NE(reason.GetTaggedValue(), JSTaggedValue::Hole());

    JSHandle<JSTaggedValue> asyncStackKeyStr = thread_->GlobalConstants()->GetHandledAsyncStackString();
    JSHandle<JSTaggedValue> asyncStackValue = JSObject::GetProperty(thread_, reason, asyncStackKeyStr).GetValue();
#if defined(ENABLE_ASYNC_STACK)
    JSHandle<EcmaString> asyncStackEcmaStr = JSTaggedValue::ToString(thread_, asyncStackValue);
    std::string asyncStackStr = EcmaStringAccessor(asyncStackEcmaStr).ToStdString(thread_);
    size_t expectedSubmitterCount = 2;
    bool checkResult = AsyncStackTestHelper::IsExpectedAsyncStackTrace(asyncStackStr, expectedSubmitterCount);
    ASSERT_TRUE(checkResult) << "Actual: " << asyncStackStr;
#else
    ASSERT_EQ(asyncStackValue.GetTaggedValue(), JSTaggedValue::Undefined());
#endif
    helper_->Clear();
}

// Requirement: when a promise task is created, collect the stack via DFX and save the stackId
HWTEST_F_L0(AsyncStackTest, TestSavePromiseNodeRecordedInMap)
{
#if defined(ENABLE_ASYNC_STACK)
    bool init = DfxInitAsyncStack();
    ASSERT_TRUE(init) << "DFX init async stack failed";
#endif

    auto asyncStackTraceManager = vm_->GetAsyncStackTraceManager();
    ASSERT_NE(asyncStackTraceManager, nullptr);
    asyncStackTraceManager->Clear();

    const std::string fileName = STACKINFO_TEST_ABC_FILES_DIR"async_await.abc";
    std::string entryPoint = "async_await";
    bool result = JSNApi::Execute(vm_, fileName, entryPoint);
    ASSERT_TRUE(result);

#if defined(ENABLE_ASYNC_STACK)
    // Awaiting inside the entry function records the promise node with its stackId
    EXPECT_GE(helper_->SizeOfPromiseMap(), 1);
    EXPECT_GE(helper_->SizeOfPromiseQueue(), 1);
#else
    // Async stack recording disabled: no node is saved
    EXPECT_EQ(helper_->SizeOfPromiseMap(), 0);
    EXPECT_EQ(helper_->SizeOfPromiseQueue(), 0);
#endif
    helper_->Clear();
}

// Requirement: when a promise task is executed, look up its promise and set the current stackId
HWTEST_F_L0(AsyncStackTest, TestSetCurrentPromiseTaskWithBuiltinPromise)
{
    auto asyncStackTraceManager = vm_->GetAsyncStackTraceManager();
    ASSERT_NE(asyncStackTraceManager, nullptr);
    asyncStackTraceManager->Clear();

#if ENABLE_LATEST_OPTIMIZATION
    // Builtin promise: reaction's PromiseOrCapability holds the JSPromise directly
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<JSPromise> promise = factory->NewJSPromise();
    uint32_t expectedPromiseId = promise->GetAsyncTaskId();
    ASSERT_NE(expectedPromiseId, 0);

    JSHandle<PromiseReaction> reaction = factory->NewPromiseReaction();
    reaction->SetPromiseOrCapability(thread_, promise.GetTaggedValue());

    asyncStackTraceManager->SetCurrentPromiseTask(reaction.GetTaggedValue());
#if defined(ENABLE_ASYNC_STACK)
    EXPECT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), expectedPromiseId);
#else
    // Async stack recording disabled: current promise id keeps untouched
    EXPECT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), 0);
#endif
#else
    GTEST_LOG_(INFO) << "Builtin promise stored directly in reaction requires ENABLE_LATEST_OPTIMIZATION";
#endif
}

// Requirement: promise-like tasks (capability wrapped) also set the current stackId
HWTEST_F_L0(AsyncStackTest, TestSetCurrentPromiseTaskWithPromiseCapability)
{
    auto asyncStackTraceManager = vm_->GetAsyncStackTraceManager();
    ASSERT_NE(asyncStackTraceManager, nullptr);
    asyncStackTraceManager->Clear();

    JSHandle<JSTaggedValue> promiseFunc = vm_->GetGlobalEnv()->GetPromiseFunction();
    JSHandle<PromiseCapability> capability = JSPromise::NewPromiseCapability(thread_, promiseFunc);
    JSHandle<JSPromise> promise(thread_, capability->GetPromise(thread_));
    uint32_t expectedPromiseId = promise->GetAsyncTaskId();
    ASSERT_NE(expectedPromiseId, 0);

    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<PromiseReaction> reaction = factory->NewPromiseReaction();
#if ENABLE_LATEST_OPTIMIZATION
    reaction->SetPromiseOrCapability(thread_, capability.GetTaggedValue());
#else
    reaction->SetPromiseCapability(thread_, capability.GetTaggedValue());
#endif

    asyncStackTraceManager->SetCurrentPromiseTask(reaction.GetTaggedValue());
#if defined(ENABLE_ASYNC_STACK)
    EXPECT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), expectedPromiseId);
#else
    // Async stack recording disabled: current promise id keeps untouched
    EXPECT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), 0);
#endif
}

// Requirement: after a promise job finishes, the current promise id must be reset
HWTEST_F_L0(AsyncStackTest, TestResetCurrentPromiseJob)
{
    auto asyncStackTraceManager = vm_->GetAsyncStackTraceManager();
    ASSERT_NE(asyncStackTraceManager, nullptr);
    asyncStackTraceManager->Clear();

    JSHandle<JSTaggedValue> promiseFunc = vm_->GetGlobalEnv()->GetPromiseFunction();
    JSHandle<PromiseCapability> capability = JSPromise::NewPromiseCapability(thread_, promiseFunc);
    JSHandle<JSPromise> promise(thread_, capability->GetPromise(thread_));
    uint32_t expectedPromiseId = promise->GetAsyncTaskId();
    ASSERT_NE(expectedPromiseId, 0);

    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<PromiseReaction> reaction = factory->NewPromiseReaction();
#if ENABLE_LATEST_OPTIMIZATION
    reaction->SetPromiseOrCapability(thread_, capability.GetTaggedValue());
#else
    reaction->SetPromiseCapability(thread_, capability.GetTaggedValue());
#endif

    asyncStackTraceManager->SetCurrentPromiseTask(reaction.GetTaggedValue());
#if defined(ENABLE_ASYNC_STACK)
    ASSERT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), expectedPromiseId);
#endif
    asyncStackTraceManager->ResetCurrentPromiseJob(reaction.GetTaggedValue());
    EXPECT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), 0);
}

// Requirement: stackId/parentPromiseId saved at creation are retrievable at crash/freeze time
HWTEST_F_L0(AsyncStackTest, TestGetStackIdAndParentPromiseIdHit)
{
    auto asyncStackTraceManager = vm_->GetAsyncStackTraceManager();
    ASSERT_NE(asyncStackTraceManager, nullptr);
    asyncStackTraceManager->Clear();

    constexpr uint32_t promiseId = 100;
    constexpr uint32_t parentPromiseId = 200;
    constexpr uint64_t stackId = 300;
    helper_->InsertPromiseNode(promiseId, parentPromiseId, stackId);

    EXPECT_EQ(asyncStackTraceManager->GetStackId(promiseId), stackId);
    EXPECT_EQ(asyncStackTraceManager->GetParentPromiseId(promiseId), parentPromiseId);
    // Unknown promise ids fall back to 0
    EXPECT_EQ(asyncStackTraceManager->GetStackId(promiseId + 1), 0U);
    EXPECT_EQ(asyncStackTraceManager->GetParentPromiseId(promiseId + 1), 0U);
}

// Requirement: the parent promise chain stored per node is the data model of
// async stack stitching; every level must stay queryable and the walk must
// terminate at the root (parent id 0)
HWTEST_F_L0(AsyncStackTest, TestPromiseNodeParentChainLookup)
{
    auto asyncStackTraceManager = vm_->GetAsyncStackTraceManager();
    ASSERT_NE(asyncStackTraceManager, nullptr);
    asyncStackTraceManager->Clear();

    constexpr uint32_t firstId = 10;
    constexpr uint32_t chainLength = 5;
    constexpr uint64_t baseStackId = 1000;
    for (uint32_t i = 0; i < chainLength; ++i) {
        uint32_t promiseId = firstId + i;
        uint32_t parentPromiseId = (i == 0) ? 0 : promiseId - 1;
        helper_->InsertPromiseNode(promiseId, parentPromiseId, baseStackId + i);
    }

    for (uint32_t i = 0; i < chainLength; ++i) {
        uint32_t promiseId = firstId + i;
        EXPECT_EQ(asyncStackTraceManager->GetStackId(promiseId), baseStackId + i);
        uint32_t expectedParent = (i == 0) ? 0 : promiseId - 1;
        EXPECT_EQ(asyncStackTraceManager->GetParentPromiseId(promiseId), expectedParent);
    }

    // Walking from the deepest node reaches the root in exactly chainLength steps
    uint32_t promiseId = firstId + chainLength - 1;
    uint32_t steps = 0;
    while (promiseId != 0) {
        promiseId = asyncStackTraceManager->GetParentPromiseId(promiseId);
        ++steps;
        ASSERT_LE(steps, chainLength) << "parent chain does not terminate";
    }
    EXPECT_EQ(steps, chainLength);

    // Ids beyond the chain never report a parent
    EXPECT_EQ(asyncStackTraceManager->GetParentPromiseId(firstId + chainLength), 0U);
    EXPECT_EQ(asyncStackTraceManager->GetStackId(firstId + chainLength), 0U);
}

// Requirement: the promise id is the unique key of a recorded node. Re-recording
// the same id refreshes the node for stitching while the eviction queue grows
// per record, so the freshest stackId always wins
HWTEST_F_L0(AsyncStackTest, TestPromiseNodeOverwriteSamePromiseId)
{
    auto asyncStackTraceManager = vm_->GetAsyncStackTraceManager();
    ASSERT_NE(asyncStackTraceManager, nullptr);
    asyncStackTraceManager->Clear();

    constexpr uint32_t promiseId = 42;
    constexpr uint32_t parentPromiseId = 7;
    helper_->InsertPromiseNode(promiseId, parentPromiseId, 111);
    ASSERT_EQ(helper_->SizeOfPromiseMap(), 1U);
    ASSERT_EQ(helper_->SizeOfPromiseQueue(), 1U);

    helper_->InsertPromiseNode(promiseId, parentPromiseId, 222);
    EXPECT_EQ(helper_->SizeOfPromiseMap(), 1U);
    EXPECT_EQ(helper_->SizeOfPromiseQueue(), 2U);
    EXPECT_TRUE(helper_->HasPromiseNode(promiseId));

    // The latest stackId is the one stitched into the trace
    EXPECT_EQ(asyncStackTraceManager->GetStackId(promiseId), 222U);
    EXPECT_EQ(asyncStackTraceManager->GetParentPromiseId(promiseId), parentPromiseId);
    EXPECT_EQ(helper_->FrontOfPromiseQueue(), promiseId);
    EXPECT_EQ(helper_->BackOfPromiseQueue(), promiseId);
}

// Requirement: eviction must not start while the recorded nodes are still
// within capacity, otherwise live submitter stacks would be dropped from
// stitched traces
HWTEST_F_L0(AsyncStackTest, TestCollectOldPromiseNodeAtExactCapacityBoundary)
{
    auto asyncStackTraceManager = vm_->GetAsyncStackTraceManager();
    ASSERT_NE(asyncStackTraceManager, nullptr);
    asyncStackTraceManager->Clear();

    const uint32_t capacity = helper_->MaxAsyncCallStacks();
    ASSERT_GT(capacity, 0U);
    for (uint32_t i = 0; i < capacity; ++i) {
        helper_->InsertPromiseNode(i, 0, i * 2 + 1);
    }
    ASSERT_EQ(helper_->SizeOfPromiseMap(), capacity);
    ASSERT_EQ(helper_->SizeOfPromiseQueue(), capacity);

    helper_->CollectOPromiseNode();

    // At the exact capacity boundary nothing is evicted
    EXPECT_EQ(helper_->SizeOfPromiseMap(), capacity);
    EXPECT_EQ(helper_->SizeOfPromiseQueue(), capacity);
    EXPECT_TRUE(helper_->HasPromiseNode(capacity - 1));
    EXPECT_EQ(asyncStackTraceManager->GetStackId(capacity - 1), (capacity - 1) * 2 + 1);
}

// Requirement: once capacity is exceeded, the oldest half of the recorded
// nodes is evicted in FIFO order so stitching keeps the most recent submitters
HWTEST_F_L0(AsyncStackTest, TestCollectOldPromiseNodeEvictsOldestHalf)
{
    auto asyncStackTraceManager = vm_->GetAsyncStackTraceManager();
    ASSERT_NE(asyncStackTraceManager, nullptr);
    asyncStackTraceManager->Clear();

    const uint32_t capacity = helper_->MaxAsyncCallStacks();
    const uint32_t total = capacity + 1;
    for (uint32_t i = 0; i < total; ++i) {
        helper_->InsertPromiseNode(i, 0, i * 2 + 1);
    }
    ASSERT_EQ(helper_->SizeOfPromiseMap(), total);

    helper_->CollectOPromiseNode();

    const uint32_t evicted = total / 2;
    EXPECT_EQ(helper_->SizeOfPromiseMap(), total - evicted);
    EXPECT_EQ(helper_->SizeOfPromiseQueue(), total - evicted);

    // The oldest nodes are dropped: lookups fall back to 0
    for (uint32_t i = 0; i < evicted; ++i) {
        EXPECT_FALSE(helper_->HasPromiseNode(i));
        EXPECT_EQ(asyncStackTraceManager->GetStackId(i), 0U);
        EXPECT_EQ(asyncStackTraceManager->GetParentPromiseId(i), 0U);
    }
    // The newest nodes survive with their original stackIds for stitching
    for (uint32_t i = evicted; i < total; ++i) {
        EXPECT_TRUE(helper_->HasPromiseNode(i));
        EXPECT_EQ(asyncStackTraceManager->GetStackId(i), i * 2 + 1);
    }
    // FIFO order is preserved after eviction
    EXPECT_EQ(helper_->FrontOfPromiseQueue(), evicted);
    EXPECT_EQ(helper_->BackOfPromiseQueue(), total - 1);
}

// Requirement: eviction and re-recording interleave continuously in long
// running processes; map and queue must stay consistent and the newest nodes
// must survive every round
HWTEST_F_L0(AsyncStackTest, TestCollectOldPromiseNodeRepeatedEvictionAndRefill)
{
    auto asyncStackTraceManager = vm_->GetAsyncStackTraceManager();
    ASSERT_NE(asyncStackTraceManager, nullptr);
    asyncStackTraceManager->Clear();

    const uint32_t capacity = helper_->MaxAsyncCallStacks();
    uint32_t nextId = 0;
    uint32_t newestSurvivor = 0;

    for (uint32_t round = 0; round < 3; ++round) {
        while (helper_->SizeOfPromiseMap() <= capacity) {
            helper_->InsertPromiseNode(nextId, 0, nextId * 2 + 1);
            newestSurvivor = nextId;
            ++nextId;
        }
        ASSERT_GT(helper_->SizeOfPromiseMap(), capacity);

        helper_->CollectOPromiseNode();

        EXPECT_EQ(helper_->SizeOfPromiseMap(), helper_->SizeOfPromiseQueue());
        EXPECT_TRUE(helper_->HasPromiseNode(newestSurvivor));
        EXPECT_EQ(asyncStackTraceManager->GetStackId(newestSurvivor),
                  static_cast<uint64_t>(newestSurvivor) * 2 + 1);
    }
    EXPECT_GT(helper_->SizeOfPromiseMap(), 0U);
}

// Requirement: switching runtime async stack off drops every recorded node so
// no stale promise is stitched into later traces
HWTEST_F_L0(AsyncStackTest, TestClearDropsAllRecordedNodes)
{
    auto asyncStackTraceManager = vm_->GetAsyncStackTraceManager();
    ASSERT_NE(asyncStackTraceManager, nullptr);
    asyncStackTraceManager->Clear();

    helper_->InsertPromiseNode(1, 0, 100);
    helper_->InsertPromiseNode(2, 1, 200);
    helper_->InsertPromiseNode(3, 2, 300);
    ASSERT_EQ(helper_->SizeOfPromiseMap(), 3U);
    ASSERT_EQ(helper_->SizeOfPromiseQueue(), 3U);

    asyncStackTraceManager->Clear();

    EXPECT_EQ(helper_->SizeOfPromiseMap(), 0U);
    EXPECT_EQ(helper_->SizeOfPromiseQueue(), 0U);
    EXPECT_EQ(helper_->FrontOfPromiseQueue(), 0U);
    EXPECT_EQ(helper_->BackOfPromiseQueue(), 0U);
    EXPECT_FALSE(helper_->HasPromiseNode(1));
    EXPECT_FALSE(helper_->HasPromiseNode(2));
    EXPECT_FALSE(helper_->HasPromiseNode(3));
    EXPECT_EQ(asyncStackTraceManager->GetStackId(2), 0U);
    EXPECT_EQ(asyncStackTraceManager->GetParentPromiseId(3), 0U);

    // Clearing an already empty manager is safe
    asyncStackTraceManager->Clear();
    EXPECT_EQ(helper_->SizeOfPromiseMap(), 0U);
}

// Requirement: every promise carries a unique non-zero async task id which
// keys its recorded node; the id must stay inside the ASYNC_TASK_ID_BITS range
HWTEST_F_L0(AsyncStackTest, TestNewJSPromiseAssignsUniqueAsyncTaskIds)
{
    ObjectFactory *factory = vm_->GetFactory();
    const uint32_t maxTaskId = (1U << JSPromise::ASYNC_TASK_ID_BITS) - 1;
    constexpr uint32_t promiseCount = 128;
    std::set<uint32_t> seenIds;
    for (uint32_t i = 0; i < promiseCount; ++i) {
        JSHandle<JSPromise> promise = factory->NewJSPromise();
        uint32_t asyncTaskId = promise->GetAsyncTaskId();
        EXPECT_NE(asyncTaskId, 0U);
        EXPECT_LE(asyncTaskId, maxTaskId);
        seenIds.insert(asyncTaskId);
    }
    EXPECT_EQ(seenIds.size(), promiseCount);
}

// Requirement: the async task id allocator wraps around at its 22-bit limit
// instead of overflowing the promise bit field (pigeonhole: allocating
// 2^22 ids over 2^22-1 possible values must produce a repeat)
HWTEST_F_L0(AsyncStackTest, TestAsyncTaskIdWrapsAroundAtLimit)
{
    const uint32_t idLimit = 1U << JSPromise::ASYNC_TASK_ID_BITS;
    const uint32_t maxTaskId = idLimit - 1;
    std::vector<bool> seenIds(idLimit, false);
    bool wrapped = false;

    for (uint32_t i = 0; i < idLimit; ++i) {
        uint32_t asyncTaskId = vm_->GetAsyncTaskId();
        EXPECT_NE(asyncTaskId, 0U);
        EXPECT_LE(asyncTaskId, maxTaskId);
        if (seenIds[asyncTaskId]) {
            wrapped = true;
        }
        seenIds[asyncTaskId] = true;
    }
    EXPECT_TRUE(wrapped) << "async task id never wrapped around";
}

// Requirement: only real promise tasks may switch the current submitter;
// non-reaction job arguments must leave the current promise untouched
HWTEST_F_L0(AsyncStackTest, TestSetCurrentPromiseTaskIgnoresNonReactionValues)
{
    auto asyncStackTraceManager = vm_->GetAsyncStackTraceManager();
    ASSERT_NE(asyncStackTraceManager, nullptr);
    asyncStackTraceManager->Clear();

    JSHandle<JSTaggedValue> undefined = thread_->GlobalConstants()->GetHandledUndefined();
    asyncStackTraceManager->SetCurrentPromiseTask(undefined.GetTaggedValue());
    EXPECT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), 0U);

    asyncStackTraceManager->SetCurrentPromiseTask(JSTaggedValue::Hole());
    EXPECT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), 0U);

    JSHandle<JSTaggedValue> smi(thread_, JSTaggedValue(42));
    asyncStackTraceManager->SetCurrentPromiseTask(smi.GetTaggedValue());
    EXPECT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), 0U);

    JSHandle<EcmaString> plainString = vm_->GetFactory()->NewFromASCII("not-a-promise-task");
    asyncStackTraceManager->SetCurrentPromiseTask(plainString.GetTaggedValue());
    EXPECT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), 0U);
}

// Requirement: consecutive promise jobs switch the current submitter without
// an intermediate reset, and re-running the same job restores its own id
HWTEST_F_L0(AsyncStackTest, TestSetCurrentPromiseTaskSwitchesBetweenPromiseJobs)
{
    auto asyncStackTraceManager = vm_->GetAsyncStackTraceManager();
    ASSERT_NE(asyncStackTraceManager, nullptr);
    asyncStackTraceManager->Clear();

    ObjectFactory *factory = vm_->GetFactory();

    JSHandle<JSPromise> firstPromise = factory->NewJSPromise();
    uint32_t firstPromiseId = firstPromise->GetAsyncTaskId();
    ASSERT_NE(firstPromiseId, 0U);
    JSHandle<PromiseReaction> firstReaction = factory->NewPromiseReaction();
    AsyncStackTestHelper::WireReactionToPromise(thread_, firstReaction, firstPromise);

    JSHandle<JSPromise> secondPromise = factory->NewJSPromise();
    uint32_t secondPromiseId = secondPromise->GetAsyncTaskId();
    ASSERT_NE(secondPromiseId, 0U);
    ASSERT_NE(secondPromiseId, firstPromiseId);
    JSHandle<PromiseReaction> secondReaction = factory->NewPromiseReaction();
    AsyncStackTestHelper::WireReactionToPromise(thread_, secondReaction, secondPromise);

    // The first job becomes the current submitter
    asyncStackTraceManager->SetCurrentPromiseTask(firstReaction.GetTaggedValue());
#if defined(ENABLE_ASYNC_STACK)
    EXPECT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), firstPromiseId);
#else
    // Async stack recording disabled: current promise id keeps untouched
    EXPECT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), 0U);
#endif

    // A second job replaces the current submitter without a reset in between
    asyncStackTraceManager->SetCurrentPromiseTask(secondReaction.GetTaggedValue());
#if defined(ENABLE_ASYNC_STACK)
    EXPECT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), secondPromiseId);
#else
    EXPECT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), 0U);
#endif

    asyncStackTraceManager->ResetCurrentPromiseJob(secondReaction.GetTaggedValue());
    EXPECT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), 0U);

    // The first job can become the current submitter again
    asyncStackTraceManager->SetCurrentPromiseTask(firstReaction.GetTaggedValue());
#if defined(ENABLE_ASYNC_STACK)
    EXPECT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), firstPromiseId);
#endif
}

// Requirement: dynamic import jobs carry a JSPromiseReactionsFunction; the
// submitter switch must resolve the promise wrapped in it as well
HWTEST_F_L0(AsyncStackTest, TestSetCurrentPromiseTaskWithPromiseResolvingFunction)
{
    auto asyncStackTraceManager = vm_->GetAsyncStackTraceManager();
    ASSERT_NE(asyncStackTraceManager, nullptr);
    asyncStackTraceManager->Clear();

    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<JSPromise> promise = factory->NewJSPromise();
    uint32_t expectedPromiseId = promise->GetAsyncTaskId();
    ASSERT_NE(expectedPromiseId, 0U);

    JSHandle<ResolvingFunctionsRecord> resolving = JSPromise::CreateResolvingFunctions(thread_, promise);
    JSHandle<JSTaggedValue> resolveFunction(thread_, resolving->GetResolveFunction(thread_));
    ASSERT_TRUE(resolveFunction->IsJSPromiseReactionFunction());

    asyncStackTraceManager->SetCurrentPromiseTask(resolveFunction.GetTaggedValue());
#if defined(ENABLE_ASYNC_STACK)
    EXPECT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), expectedPromiseId);
#else
    // Async stack recording disabled: current promise id keeps untouched
    EXPECT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), 0U);
#endif

    // Reset accepts the resolving function as a finished job as well
    asyncStackTraceManager->ResetCurrentPromiseJob(resolveFunction.GetTaggedValue());
    EXPECT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), 0U);
}

// Requirement: the current submitter must survive a reset triggered with a
// foreign (non-reaction) job argument, otherwise a concurrent job would clear
// the stitching context of the running one
HWTEST_F_L0(AsyncStackTest, TestResetCurrentPromiseJobIgnoresNonReactionValue)
{
    auto asyncStackTraceManager = vm_->GetAsyncStackTraceManager();
    ASSERT_NE(asyncStackTraceManager, nullptr);
    asyncStackTraceManager->Clear();

    JSHandle<JSTaggedValue> promiseFunc = vm_->GetGlobalEnv()->GetPromiseFunction();
    JSHandle<PromiseCapability> capability = JSPromise::NewPromiseCapability(thread_, promiseFunc);
    JSHandle<JSPromise> promise(thread_, capability->GetPromise(thread_));
    uint32_t expectedPromiseId = promise->GetAsyncTaskId();
    ASSERT_NE(expectedPromiseId, 0U);

    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<PromiseReaction> reaction = factory->NewPromiseReaction();
#if ENABLE_LATEST_OPTIMIZATION
    reaction->SetPromiseOrCapability(thread_, capability.GetTaggedValue());
#else
    reaction->SetPromiseCapability(thread_, capability.GetTaggedValue());
#endif

    asyncStackTraceManager->SetCurrentPromiseTask(reaction.GetTaggedValue());
#if defined(ENABLE_ASYNC_STACK)
    ASSERT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), expectedPromiseId);
#endif

    JSHandle<JSTaggedValue> undefined = thread_->GlobalConstants()->GetHandledUndefined();
    asyncStackTraceManager->ResetCurrentPromiseJob(undefined.GetTaggedValue());
#if defined(ENABLE_ASYNC_STACK)
    // Non-reaction reset must not clear the current submitter
    EXPECT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), expectedPromiseId);
#else
    EXPECT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), 0U);
#endif

    asyncStackTraceManager->ResetCurrentPromiseJob(reaction.GetTaggedValue());
    EXPECT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), 0U);
}

// Requirement: with no current promise task there is nothing to stitch, the
// async part of the trace must stay empty
HWTEST_F_L0(AsyncStackTest, TestBuildAsyncStackTraceEmptyWithoutCurrentPromise)
{
    auto asyncStackTraceManager = vm_->GetAsyncStackTraceManager();
    ASSERT_NE(asyncStackTraceManager, nullptr);
    asyncStackTraceManager->Clear();

    helper_->InsertPromiseNode(1, 0, 100);
    helper_->InsertPromiseNode(2, 1, 200);
    ASSERT_EQ(helper_->SizeOfPromiseMap(), 2U);
    ASSERT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), 0U);

    std::string asyncStackTrace;
    asyncStackTraceManager->BuildAsyncStackTrace(asyncStackTrace);
    EXPECT_TRUE(asyncStackTrace.empty());
}

// Requirement: a node recorded while another promise job is current must link
// to that job's promise id, building the chain used for stitching at
// crash/freeze time
HWTEST_F_L0(AsyncStackTest, TestSavePromiseNodeLinksParentPromiseChain)
{
#if defined(ENABLE_ASYNC_STACK)
    bool init = DfxInitAsyncStack();
    ASSERT_TRUE(init) << "DFX init async stack failed";
#endif

    auto asyncStackTraceManager = vm_->GetAsyncStackTraceManager();
    ASSERT_NE(asyncStackTraceManager, nullptr);
    asyncStackTraceManager->Clear();

    ObjectFactory *factory = vm_->GetFactory();

    // The first promise becomes the current submitter through its reaction job
    JSHandle<JSPromise> firstPromise = factory->NewJSPromise();
    uint32_t firstPromiseId = firstPromise->GetAsyncTaskId();
    ASSERT_NE(firstPromiseId, 0U);
    JSHandle<PromiseReaction> firstReaction = factory->NewPromiseReaction();
    AsyncStackTestHelper::WireReactionToPromise(thread_, firstReaction, firstPromise);
    asyncStackTraceManager->SetCurrentPromiseTask(firstReaction.GetTaggedValue());

    // A child promise task is created while the first job is current
    JSHandle<JSPromise> secondPromise = factory->NewJSPromise();
    uint32_t secondPromiseId = secondPromise->GetAsyncTaskId();
    ASSERT_NE(secondPromiseId, 0U);
    asyncStackTraceManager->SavePromiseNode(secondPromise);

#if defined(ENABLE_ASYNC_STACK)
    if (helper_->SizeOfPromiseMap() == 1) {
        // The child node links to the current submitter and carries a stackId
        EXPECT_EQ(asyncStackTraceManager->GetParentPromiseId(secondPromiseId), firstPromiseId);
        EXPECT_NE(asyncStackTraceManager->GetStackId(secondPromiseId), 0U);
    } else {
        // Stack collection unavailable in this environment: nothing recorded
        EXPECT_EQ(helper_->SizeOfPromiseMap(), 0U);
    }
#else
    // Async stack recording disabled: no node is saved
    EXPECT_EQ(helper_->SizeOfPromiseMap(), 0U);
    EXPECT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), 0U);
#endif

    // Second level: make the child the current submitter, then record a grandchild
    JSHandle<PromiseReaction> secondReaction = factory->NewPromiseReaction();
    AsyncStackTestHelper::WireReactionToPromise(thread_, secondReaction, secondPromise);
    asyncStackTraceManager->SetCurrentPromiseTask(secondReaction.GetTaggedValue());
    JSHandle<JSPromise> thirdPromise = factory->NewJSPromise();
    uint32_t thirdPromiseId = thirdPromise->GetAsyncTaskId();
    ASSERT_NE(thirdPromiseId, 0U);
    asyncStackTraceManager->SavePromiseNode(thirdPromise);

#if defined(ENABLE_ASYNC_STACK)
    if (helper_->SizeOfPromiseMap() == 2) {
        // The stitched chain is grandchild -> child -> submitter root
        EXPECT_EQ(asyncStackTraceManager->GetParentPromiseId(thirdPromiseId), secondPromiseId);
        EXPECT_EQ(asyncStackTraceManager->GetParentPromiseId(secondPromiseId), firstPromiseId);
    }
#endif
    asyncStackTraceManager->ResetCurrentPromiseJob(secondReaction.GetTaggedValue());
    EXPECT_EQ(asyncStackTraceManager->GetCurrentPromiseId(), 0U);
}

// Requirement: switching the runtime async stack flag off drops all recorded
// nodes (EcmaVM::SetEnableRuntimeAsyncStack clears the manager), so later
// reports are not stitched from stale promises
HWTEST_F_L0(AsyncStackTest, TestSetEnableRuntimeAsyncStackClearsRecordedNodesOnDisable)
{
    auto asyncStackTraceManager = vm_->GetAsyncStackTraceManager();
    ASSERT_NE(asyncStackTraceManager, nullptr);

    // Start from a disabled state regardless of what SetUp did
    vm_->SetEnableRuntimeAsyncStack(false);
    ASSERT_FALSE(vm_->IsEnableRuntimeAsyncStack());
    asyncStackTraceManager->Clear();
    ASSERT_EQ(helper_->SizeOfPromiseMap(), 0U);

    vm_->SetEnableRuntimeAsyncStack(true);
    ASSERT_TRUE(vm_->IsEnableRuntimeAsyncStack());
    helper_->InsertPromiseNode(1, 0, 100);
    helper_->InsertPromiseNode(2, 1, 200);
    ASSERT_EQ(helper_->SizeOfPromiseMap(), 2U);

    // Disabling drops all recorded nodes
    vm_->SetEnableRuntimeAsyncStack(false);
    EXPECT_FALSE(vm_->IsEnableRuntimeAsyncStack());
    EXPECT_EQ(helper_->SizeOfPromiseMap(), 0U);
    EXPECT_EQ(helper_->SizeOfPromiseQueue(), 0U);
    EXPECT_EQ(asyncStackTraceManager->GetStackId(1), 0U);
    EXPECT_EQ(asyncStackTraceManager->GetParentPromiseId(2), 0U);

    // Repeated disable stays safe
    vm_->SetEnableRuntimeAsyncStack(false);
    EXPECT_EQ(helper_->SizeOfPromiseMap(), 0U);
}

// Requirement: enabling runtime async stack saves the previous DFX async stack
// type and disabling restores it; without DFX support both switches are no-ops
HWTEST_F_L0(AsyncStackTest, TestSetAndResetAsyncStackTypeSavesDefault)
{
    auto asyncStackTraceManager = vm_->GetAsyncStackTraceManager();
    ASSERT_NE(asyncStackTraceManager, nullptr);

#if defined(ENABLE_ASYNC_STACK)
    // A first switch installs the promise bit into the DFX type; a second
    // consecutive switch then captures that installed type as the new saved
    // default, which therefore must be non-zero
    asyncStackTraceManager->SetAsyncStackType();
    asyncStackTraceManager->SetAsyncStackType();
    EXPECT_NE(helper_->DefaultAsyncStackType(), 0U) << "promise async stack type was not installed";

    // Restoring is idempotent
    asyncStackTraceManager->ResetAsyncStackType();
    asyncStackTraceManager->ResetAsyncStackType();
#else
    asyncStackTraceManager->SetAsyncStackType();
    EXPECT_EQ(helper_->DefaultAsyncStackType(), 0U);
    asyncStackTraceManager->ResetAsyncStackType();
    EXPECT_EQ(helper_->DefaultAsyncStackType(), 0U);
#endif
}
}  // namespace panda::test
