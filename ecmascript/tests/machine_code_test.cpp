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

#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/machine_code.h"
#include "ecmascript/jit/jit.h"

#include "ecmascript/global_env.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_array.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {

class MachineCodeTest : public BaseTestWithScope<false> {
public:
    void SetUp() override
    {
        JSRuntimeOptions options;
        options.SetEnableEdenGC(true);
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

HWTEST_F_L0(MachineCodeTest, SetMachineCodeObjectDataTest001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> func(thread->GetEcmaVM()->GetGlobalEnv()->GetArrayFunction());
    JSHandle<Method> method(thread, func->GetMethod());
    MachineCodeDesc desc;
    desc.codeType = MachineCodeType::BASELINE_CODE;
    desc.instructionsSize = 100;
    desc.instructionsAddr = 1000;
    desc.stackMapSizeAlign = 100;
    TaggedObject *machineCode = factory->NewMachineCodeObject(100, desc);
    factory->SetMachineCodeObjectData(machineCode, 100, desc, method);
}

HWTEST_F_L0(MachineCodeTest, SetMachineCodeObjectDataTest004)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> func(thread->GetEcmaVM()->GetGlobalEnv()->GetArrayFunction());
    JSHandle<Method> method(thread, func->GetMethod());
    MachineCodeDesc desc;
    desc.codeType = MachineCodeType::FAST_JIT_CODE;
    desc.instructionsSize = 100;
    desc.instructionsAddr = 1000;
    desc.stackMapSizeAlign = 100;
    desc.rodataSizeAfterTextAlign = 0;
    desc.rodataSizeBeforeTextAlign = 0;
    desc.isAsyncCompileMode = true;
    Jit::GetInstance()->SetEnableAsyncCopyToFort(true);
    Jit::GetInstance()->SetEnableJitFort(true);
    ASSERT_EQ(Jit::GetInstance()->IsEnableAsyncCopyToFort(), true);
    TaggedObject *machineCode = factory->NewMachineCodeObject(100, desc);
    factory->SetMachineCodeObjectData(machineCode, 100, desc, method);
}

HWTEST_F_L0(MachineCodeTest, SetMachineCodeObjectDataTest006)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> func(thread->GetEcmaVM()->GetGlobalEnv()->GetArrayFunction());
    JSHandle<Method> method(thread, func->GetMethod());
    MachineCodeDesc desc;
    desc.codeType = MachineCodeType::FAST_JIT_CODE;
    desc.instructionsSize = 100;
    desc.instructionsAddr = 1000;
    desc.stackMapSizeAlign = 100;
    desc.rodataSizeAfterTextAlign = 0;
    desc.rodataSizeBeforeTextAlign = 0;
    desc.codeSizeAlign = 0;
    desc.isAsyncCompileMode = true;
    desc.funcEntryDesSizeAlign = 0;
    desc.stackMapOrOffsetTableSize = 100;
    Jit::GetInstance()->SetEnableJitFort(true);
    Jit::GetInstance()->SetEnableAsyncCopyToFort(true);
    TaggedObject *machineCode = factory->NewMachineCodeObject(100, desc);
    MachineCode *code = MachineCode::Cast(machineCode);
    const char src[] = "hello";
    const char src2[] = "world";
    desc.stackMapOrOffsetTableAddr = reinterpret_cast<uintptr_t>(src);
    desc.funcEntryDesAddr = reinterpret_cast<uintptr_t>(src2);
    desc.funcEntryDesSize = 100;
    ASSERT_EQ(code->SetData(desc, method, 100), true);
}

HWTEST_F_L0(MachineCodeTest, SetMachineCodeObjectDataTest007)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> func(thread->GetEcmaVM()->GetGlobalEnv()->GetArrayFunction());
    JSHandle<Method> method(thread, func->GetMethod());
    MachineCodeDesc desc;
    desc.codeType = MachineCodeType::FAST_JIT_CODE;
    desc.instructionsSize = 100;
    desc.instructionsAddr = 1000;
    desc.stackMapSizeAlign = -100;
    desc.rodataSizeAfterTextAlign = 0;
    desc.rodataSizeBeforeTextAlign = 0;
    desc.codeSizeAlign = 0;
    desc.isAsyncCompileMode = true;
    desc.funcEntryDesSizeAlign = 0;
    desc.stackMapOrOffsetTableSize = 100;
    Jit::GetInstance()->SetEnableJitFort(true);
    Jit::GetInstance()->SetEnableAsyncCopyToFort(true);
    TaggedObject *machineCode = factory->NewMachineCodeObject(100, desc);
    MachineCode *code = MachineCode::Cast(machineCode);
    const char src[] = "hello";
    const char src2[] = "world";
    desc.stackMapOrOffsetTableAddr = reinterpret_cast<uintptr_t>(src);
    desc.funcEntryDesAddr = reinterpret_cast<uintptr_t>(src2);
    desc.funcEntryDesSize = 100;
    ASSERT_EQ(code->SetData(desc, method, -100), true);
}

}