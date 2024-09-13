/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <csetjmp>
#include <csignal>

#include "ecmascript/ecma_handle_scope.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/mem/barriers.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/verification.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/tests/test_helper.h"
#include "gtest/gtest.h"

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {
class HandleLeakTest : public BaseTestWithScope<false> {
public:
    void SetUp() override
    {
        JSRuntimeOptions options;
        options.SetEnableForceGC(false);
        options.SetLogLevel("info");
        instance = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        thread->ManagedCodeBegin();
        scope = new EcmaHandleScope(thread);
    }
};

static sigjmp_buf env;
static bool segmentFaultFlag = false;
class HandleLeakTestManager {
public:
    static void ProcessHandleLeakSegmentFault(int sig)
    {
        segmentFaultFlag = true;
        siglongjmp(env, sig);
    }

    static int RegisterSignal()
    {
        segmentFaultFlag = false;
        struct sigaction act;
        act.sa_handler = ProcessHandleLeakSegmentFault;
        sigemptyset(&act.sa_mask);
        sigaddset(&act.sa_mask, SIGQUIT);
        act.sa_flags = SA_RESETHAND;
        return sigaction(SIGSEGV, &act, nullptr);
    }
};

HWTEST_F_L0(HandleLeakTest, HandleLeakCheck)
{
    EcmaHandleScope scope(thread);
    std::vector<Global<ArrayRef>> result;
    for (int i = 0; i < 75000; i++) {
        result.emplace_back(Global<ArrayRef>(instance, ArrayRef::New(instance, 100)));
    }
}

HWTEST_F_L0(HandleLeakTest, InitializeCheckOneProperty)
{
    EcmaHandleScope scope(thread);
    JSHandle<Program> newProgram(thread, const_cast<Heap *>(instance->GetHeap())->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread->GlobalConstants()->GetProgramClass().GetTaggedObject())));

    newProgram->SetMainFunction(thread, JSTaggedValue::Undefined());

    size_t failCount = 0;
    VerifyObjectVisitor verifier(instance->GetHeap(), &failCount);
    verifier(*newProgram);
    ASSERT_TRUE(newProgram.GetTaggedValue().IsProgram());
    ASSERT_TRUE(failCount == 0);
}

static void HeandleLeakTestCommon(const EcmaVM *instance, JSHandle<TaggedArray>& newArray)
{
    size_t failCount = 0;
    auto ret = sigsetjmp(env, 1);
    if (ret != SIGSEGV) {
        VerifyObjectVisitor verifier(instance->GetHeap(), &failCount);
        verifier(*newArray);
        ASSERT_TRUE(false);
    } else {
        // catch signal SIGSEGV caused by uninitialize
        EXPECT_TRUE(segmentFaultFlag);
        ASSERT_TRUE(failCount == 0);
    }
}

HWTEST_F_L0(HandleLeakTest, PartInitializeCheckMoreProperty)
{
    EcmaHandleScope scope(thread);
    JSHandle<JSHClass> arrayClass(thread->GlobalConstants()->GetHandledArrayClass());
    static constexpr int SIZE = 100;
    JSHandle<TaggedArray> newArray(thread, const_cast<Heap *>(instance->GetHeap())->AllocateNonMovableOrHugeObject(
        *arrayClass, TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), SIZE)));
    newArray->SetLength(SIZE);
    for (uint32_t i = 0; i < SIZE / 2; i++) {
        size_t offset = JSTaggedValue::TaggedTypeSize() * i;
        ecmascript::Barriers::SetPrimitive(newArray->GetData(), offset, JSTaggedValue::Undefined());
    }

    if (HandleLeakTestManager::RegisterSignal() == -1) {
        perror("sigaction error");
        exit(1);
    }
    HeandleLeakTestCommon(instance, newArray);
}

HWTEST_F_L0(HandleLeakTest, InitializeCheckMoreProperty)
{
    EcmaHandleScope scope(thread);
    JSHandle<JSHClass> arrayClass(thread->GlobalConstants()->GetHandledArrayClass());
    static constexpr int SIZE = 100;
    JSHandle<TaggedArray> newArray(thread, const_cast<Heap *>(instance->GetHeap())->AllocateNonMovableOrHugeObject(
        *arrayClass, TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), SIZE)));

    newArray->InitializeWithSpecialValue(JSTaggedValue::Hole(), SIZE);
    size_t failCount = 0;
    VerifyObjectVisitor verifier(instance->GetHeap(), &failCount);
    verifier(*newArray);
    ASSERT_TRUE(newArray.GetTaggedValue().IsTaggedArray());
    ASSERT_TRUE(failCount == 0);
}
}  // namespace panda::test
