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

#include "ecmascript/ecma_vm.h"
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
}  // namespace panda::test
