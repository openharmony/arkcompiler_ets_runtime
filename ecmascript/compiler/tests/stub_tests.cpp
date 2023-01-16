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

#include <cstdint>
#include <unistd.h>

#include "ecmascript/builtins/builtins_promise_handler.h"
#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/compiler/assembler_module.h"
#include "ecmascript/compiler/binary_section.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/llvm_codegen.h"
#include "ecmascript/compiler/llvm_ir_builder.h"
#include "ecmascript/compiler/stub.h"
#include "ecmascript/compiler/scheduler.h"
#include "ecmascript/compiler/verifier.h"
#include "ecmascript/aot_file_manager.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/js_array.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/message_string.h"
#include "ecmascript/stackmap/llvm_stackmap_parser.h"
#include "ecmascript/stubs/runtime_stubs.h"
#include "ecmascript/tests/test_helper.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/SourceMgr.h"

#include "gtest/gtest.h"

namespace panda::test {
using namespace panda::ecmascript;
using namespace panda::ecmascript::kungfu;
using BuiltinsPromiseHandler = builtins::BuiltinsPromiseHandler;

class StubTest : public testing::Test {
public:
    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope, true);
        BytecodeStubCSigns::Initialize();
        CommonStubCSigns::Initialize();
        RuntimeStubCSigns::Initialize();
        auto logOpt = thread->GetEcmaVM()->GetJSOptions().GetCompilerLogOption();
        log = new CompilerLog(logOpt);
        stubModule.SetUpForCommonStubs();
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
        if (log != nullptr) {
            delete log;
        }
    }

    void PrintCircuitByBasicBlock([[maybe_unused]] const std::vector<std::vector<GateRef>> &cfg,
                                  [[maybe_unused]] const Circuit &netOfGates) const
    {
        if (thread->GetEcmaVM()->GetJSOptions().WasSetCompilerLogOption()) {
            GateAccessor acc(const_cast<Circuit*>(&netOfGates));
            for (size_t bbIdx = 0; bbIdx < cfg.size(); bbIdx++) {
                LOG_COMPILER(INFO) << (acc.GetMetaData(cfg[bbIdx].front())->IsCFGMerge() ? "MERGE_" : "BB_")
                                   << bbIdx << ":";
                for (size_t instIdx = cfg[bbIdx].size(); instIdx > 0; instIdx--) {
                    acc.Print(cfg[bbIdx][instIdx - 1]);
                }
            }
        }
    }

    JSTaggedValue NewAotFunction(uint32_t numArgs, uintptr_t codeEntry)
    {
        auto ecmaVm = thread->GetEcmaVM();
        ObjectFactory *factory = ecmaVm->GetFactory();
        JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
        JSHandle<Method> method = factory->NewMethodForNativeFunction(reinterpret_cast<void *>(codeEntry));
        method->SetAotCodeBit(true);
        method->SetNativeBit(false);
        method->SetNumArgsWithCallField(numArgs);
        method->SetCodeEntryOrLiteral(reinterpret_cast<uintptr_t>(codeEntry));
        JSHandle<JSFunction> jsfunc = factory->NewJSFunction(env, method);
        return jsfunc.GetTaggedValue();
    }

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
    CompilerLog *log {nullptr};
    LLVMModule stubModule {"stub_tests", "x86_64-unknown-linux-gnu"};
};

HWTEST_F_L0(StubTest, FastAddTest)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetFunction(CommonStubCSigns::Add);
    ecmascript::NativeAreaAllocator allocator;
    Circuit netOfGates(&allocator);
    CallSignature callSignature;
    AddCallSignature::Initialize(&callSignature);
    Stub stub(&callSignature, &netOfGates);
    AddStubBuilder optimizer(&callSignature, stub.GetEnvironment());
    stub.SetStubBuilder(&optimizer);
    stub.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    Scheduler::ControlFlowGraph cfg;
    Scheduler::Run(&netOfGates, cfg);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run(*log);
    // Testcase build and run
    auto fn = reinterpret_cast<JSTaggedValue (*)(uintptr_t, int64_t, int64_t)>(
        assembler.GetFuncPtrFromCompiledModule(function));
    auto resA = fn(thread->GetGlueAddr(), JSTaggedValue(1).GetRawData(),
        JSTaggedValue(1).GetRawData());
    auto resB = fn(thread->GetGlueAddr(), JSTaggedValue(2).GetRawData(),
        JSTaggedValue(2).GetRawData());     // 2 : test case
    auto resC = fn(thread->GetGlueAddr(), JSTaggedValue(11).GetRawData(),
        JSTaggedValue(11).GetRawData());  // 11 : test case
    LOG_COMPILER(INFO) << "res for FastAdd(1, 1) = " << resA.GetNumber();
    LOG_COMPILER(INFO) << "res for FastAdd(2, 2) = " << resB.GetNumber();
    LOG_COMPILER(INFO) << "res for FastAdd(11, 11) = " << resC.GetNumber();
    EXPECT_EQ(resA.GetNumber(), JSTaggedValue(2).GetNumber());
    EXPECT_EQ(resB.GetNumber(), JSTaggedValue(4).GetNumber());
    EXPECT_EQ(resC.GetNumber(), JSTaggedValue(22).GetNumber());
    int x1 = 2147483647;
    int y1 = 15;
    auto resG = fn(thread->GetGlueAddr(), JSTaggedValue(x1).GetRawData(), JSTaggedValue(y1).GetRawData());
    auto expectedG = SlowRuntimeStub::Add2(thread, JSTaggedValue(x1), JSTaggedValue(y1));
    EXPECT_EQ(resG, expectedG);
}

HWTEST_F_L0(StubTest, FastSubTest)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetFunction(CommonStubCSigns::Sub);
    ecmascript::NativeAreaAllocator allocator;
    Circuit netOfGates(&allocator);
    CallSignature callSignature;
    SubCallSignature::Initialize(&callSignature);
    Stub stub(&callSignature, &netOfGates);
    SubStubBuilder optimizer(&callSignature, stub.GetEnvironment());
    stub.SetStubBuilder(&optimizer);
    stub.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    Scheduler::ControlFlowGraph cfg;
    Scheduler::Run(&netOfGates, cfg);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run(*log);
    // Testcase build and run
    auto fn = reinterpret_cast<JSTaggedValue (*)(uintptr_t, int64_t, int64_t)>(
        assembler.GetFuncPtrFromCompiledModule(function));
    auto resA = fn(thread->GetGlueAddr(), JSTaggedValue(2).GetRawData(),
        JSTaggedValue(1).GetRawData());    // 2 : test case
    auto resB = fn(thread->GetGlueAddr(), JSTaggedValue(7).GetRawData(),
        JSTaggedValue(2).GetRawData());    // 7, 2 : test cases
    auto resC = fn(thread->GetGlueAddr(), JSTaggedValue(11).GetRawData(),
        JSTaggedValue(11).GetRawData());  // 11 : test case
    LOG_COMPILER(INFO) << "res for FastSub(2, 1) = " << resA.GetNumber();
    LOG_COMPILER(INFO) << "res for FastSub(7, 2) = " << resB.GetNumber();
    LOG_COMPILER(INFO) << "res for FastSub(11, 11) = " << resC.GetNumber();
    EXPECT_EQ(resA, JSTaggedValue(1));
    EXPECT_EQ(resB, JSTaggedValue(5));
    EXPECT_EQ(resC, JSTaggedValue(0));
}


HWTEST_F_L0(StubTest, FastMulTest)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetFunction(CommonStubCSigns::Mul);
    ecmascript::NativeAreaAllocator allocator;
    Circuit netOfGates(&allocator);
    CallSignature callSignature;
    MulCallSignature::Initialize(&callSignature);
    Stub stub(&callSignature, &netOfGates);
    MulStubBuilder optimizer(&callSignature, stub.GetEnvironment());
    stub.SetStubBuilder(&optimizer);
    stub.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    Scheduler::ControlFlowGraph cfg;
    Scheduler::Run(&netOfGates, cfg);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run(*log);
    // Testcase build and run
    auto fn = reinterpret_cast<JSTaggedValue (*)(uintptr_t, int64_t, int64_t)>(
        assembler.GetFuncPtrFromCompiledModule(function));
    auto resA = fn(thread->GetGlueAddr(), JSTaggedValue(-2).GetRawData(),
        JSTaggedValue(1).GetRawData());   // -2 : test case
    auto resB = fn(thread->GetGlueAddr(), JSTaggedValue(-7).GetRawData(),
        JSTaggedValue(-2).GetRawData());  // -7, -2 : test case
    auto resC = fn(thread->GetGlueAddr(), JSTaggedValue(11).GetRawData(),
        JSTaggedValue(11).GetRawData());  // 11 : test case
    LOG_COMPILER(INFO) << "res for FastMul(-2, 1) = " << std::dec << resA.GetNumber();
    LOG_COMPILER(INFO) << "res for FastMul(-7, -2) = " << std::dec << resB.GetNumber();
    LOG_COMPILER(INFO) << "res for FastMul(11, 11) = " << std::dec << resC.GetNumber();
    EXPECT_EQ(resA.GetNumber(), -2); // -2: test case
    EXPECT_EQ(resB.GetNumber(), 14); // 14: test case
    EXPECT_EQ(resC.GetNumber(), 121); // 121: test case
    int x = 7;
    double y = 1125899906842624;
    auto resD = fn(thread->GetGlueAddr(), JSTaggedValue(x).GetRawData(), JSTaggedValue(y).GetRawData());
    JSTaggedValue expectedD = FastRuntimeStub::FastMul(JSTaggedValue(x), JSTaggedValue(y));
    EXPECT_EQ(resD, expectedD);
    x = -1;
    y = 1.7976931348623157e+308;
    auto resE = fn(thread->GetGlueAddr(), JSTaggedValue(x).GetRawData(), JSTaggedValue(y).GetRawData());
    JSTaggedValue expectedE = FastRuntimeStub::FastMul(JSTaggedValue(x), JSTaggedValue(y));
    EXPECT_EQ(resE, expectedE);
    x = -1;
    y = -1 * std::numeric_limits<double>::infinity();
    auto resF = fn(thread->GetGlueAddr(), JSTaggedValue(x).GetRawData(), JSTaggedValue(y).GetRawData());
    JSTaggedValue expectedF = FastRuntimeStub::FastMul(JSTaggedValue(x), JSTaggedValue(y));
    EXPECT_EQ(resF, expectedF);
    int x1 = 2147483647;
    int y1 = 15;
    auto resG = fn(thread->GetGlueAddr(), JSTaggedValue(x1).GetRawData(), JSTaggedValue(y1).GetRawData());
    auto expectedG = FastRuntimeStub::FastMul(JSTaggedValue(x1), JSTaggedValue(y1));
    EXPECT_EQ(resG, expectedG);
}

HWTEST_F_L0(StubTest, FastDivTest)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetFunction(CommonStubCSigns::Div);
    ecmascript::NativeAreaAllocator allocator;
    Circuit netOfGates(&allocator);
    CallSignature callSignature;
    DivCallSignature::Initialize(&callSignature);
    Stub stub(&callSignature, &netOfGates);
    DivStubBuilder optimizer(&callSignature, stub.GetEnvironment());
    stub.SetStubBuilder(&optimizer);
    stub.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    Scheduler::ControlFlowGraph cfg;
    Scheduler::Run(&netOfGates, cfg);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run(*log);
    auto fn = reinterpret_cast<JSTaggedValue (*)(uintptr_t, int64_t, int64_t)>(
        assembler.GetFuncPtrFromCompiledModule(function));
    // test normal Division operation
    uint64_t x1 = JSTaggedValue(50).GetRawData();
    uint64_t y1 = JSTaggedValue(25).GetRawData();
    LOG_COMPILER(INFO) << "x1 = " << x1 << "  y1 = " << y1;
    auto res1 = fn(thread->GetGlueAddr(), x1, y1);
    LOG_COMPILER(INFO) << "res for FastDiv(50, 25) = " << res1.GetRawData();
    auto expectedG1 = FastRuntimeStub::FastDiv(JSTaggedValue(x1), JSTaggedValue(y1));
    EXPECT_EQ(res1, expectedG1);

    // test x == 0.0 or std::isnan(x)
    uint64_t x2 = JSTaggedValue(base::NAN_VALUE).GetRawData();
    uint64_t y2 = JSTaggedValue(0).GetRawData();
    LOG_COMPILER(INFO) << "x2 = " << x1 << "  y2 = " << y2;
    auto res2 = fn(thread->GetGlueAddr(), x2, y2);
    LOG_COMPILER(INFO) << "res for FastDiv(base::NAN_VALUE, 0) = " << res2.GetRawData();
    auto expectedG2 = FastRuntimeStub::FastDiv(JSTaggedValue(x2), JSTaggedValue(y2));
    EXPECT_EQ(res2, expectedG2);

    // test other
    uint64_t x3 = JSTaggedValue(7).GetRawData();
    uint64_t y3 = JSTaggedValue(0).GetRawData();
    LOG_COMPILER(INFO) << "x2 = " << x3 << "  y2 = " << y3;
    auto res3 = fn(thread->GetGlueAddr(), x3, y3);
    LOG_COMPILER(INFO) << "res for FastDiv(7, 0) = " << res3.GetRawData();
    auto expectedG3 = FastRuntimeStub::FastDiv(JSTaggedValue(x3), JSTaggedValue(y3));
    EXPECT_EQ(res3, expectedG3);
}

HWTEST_F_L0(StubTest, FastModTest)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetFunction(CommonStubCSigns::Mod);
    ecmascript::NativeAreaAllocator allocator;
    Circuit netOfGates(&allocator);
    CallSignature callSignature;
    ModCallSignature::Initialize(&callSignature);
    Stub stub(&callSignature, &netOfGates);
    ModStubBuilder optimizer(&callSignature, stub.GetEnvironment());
    stub.SetStubBuilder(&optimizer);
    stub.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    Scheduler::ControlFlowGraph cfg;
    Scheduler::Run(&netOfGates, cfg);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run(*log);
    auto fn = reinterpret_cast<JSTaggedValue (*)(uintptr_t, int64_t, int64_t)>(
        assembler.GetFuncPtrFromCompiledModule(function));
    // test left, right are all integer
    int x = 7;
    int y = 3;
    auto result = fn(thread->GetGlueAddr(), JSTaggedValue(x).GetRawData(), JSTaggedValue(y).GetRawData());
    JSTaggedValue expectRes = FastRuntimeStub::FastMod(JSTaggedValue(x), JSTaggedValue(y));
    EXPECT_EQ(result, expectRes);

    // test y == 0.0 || std::isnan(y) || std::isnan(x) || std::isinf(x) return NAN_VALUE
    double x2 = 7.3;
    int y2 = base::NAN_VALUE;
    auto result2 = fn(thread->GetGlueAddr(), JSTaggedValue(x2).GetRawData(), JSTaggedValue(y2).GetRawData());
    auto expectRes2 = FastRuntimeStub::FastMod(JSTaggedValue(x2), JSTaggedValue(y2));
    EXPECT_EQ(result2, expectRes2);
    LOG_COMPILER(INFO) << "result2 for FastMod(7, 'helloworld') = " << result2.GetRawData();
    LOG_COMPILER(INFO) << "expectRes2 for FastMod(7, 'helloworld') = " << expectRes2.GetRawData();

    // // test modular operation under normal conditions
    auto sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
    double x3 = 33.0;
    double y3 = 44.0;
    auto result3 = fn(thread->GetGlueAddr(), JSTaggedValue(x3).GetRawData(), JSTaggedValue(y3).GetRawData());
    auto expectRes3 = FastRuntimeStub::FastMod(JSTaggedValue(x3), JSTaggedValue(y3));
    EXPECT_EQ(result3, expectRes3);
    thread->SetCurrentSPFrame(sp);

    // test x == 0.0 || std::isinf(y) return x
    double x4 = base::NAN_VALUE;
    int y4 = 7;
    auto result4 = fn(thread->GetGlueAddr(), JSTaggedValue(x4).GetRawData(), JSTaggedValue(y4).GetRawData());
    auto expectRes4 = FastRuntimeStub::FastMod(JSTaggedValue(x4), JSTaggedValue(y4));

    LOG_COMPILER(INFO) << "result4 for FastMod(base::NAN_VALUE, 7) = " << result4.GetRawData();
    LOG_COMPILER(INFO) << "expectRes4 for FastMod(base::NAN_VALUE, 7) = " << expectRes4.GetRawData();
    EXPECT_EQ(result4, expectRes4);

    // test all non-conforming conditions
    int x5 = 7;
    auto *factory = thread->GetEcmaVM()->GetFactory();
    thread->SetLastLeaveFrame(nullptr);
    auto y5 = factory->NewFromASCII("hello world");
    auto result5 = FastRuntimeStub::FastMod(JSTaggedValue(x5), y5.GetTaggedValue());
    LOG_COMPILER(INFO) << "result1 for FastMod(7, 'helloworld') = " << result5.GetRawData();
    EXPECT_EQ(result5, JSTaggedValue::Hole());
}

HWTEST_F_L0(StubTest, TryLoadICByName)
{
    auto module = stubModule.GetModule();
    auto findFunction = stubModule.GetFunction(CommonStubCSigns::TryLoadICByName);
    ecmascript::NativeAreaAllocator allocator;
    Circuit netOfGates(&allocator);
    CallSignature callSignature;
    TryLoadICByNameCallSignature::Initialize(&callSignature);
    Stub stub(&callSignature, &netOfGates);
    TryLoadICByNameStubBuilder optimizer(&callSignature, stub.GetEnvironment());
    stub.SetStubBuilder(&optimizer);
    stub.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    Scheduler::ControlFlowGraph cfg;
    Scheduler::Run(&netOfGates, cfg);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, findFunction, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run(*log);
}

HWTEST_F_L0(StubTest, TryLoadICByValue)
{
    auto module = stubModule.GetModule();
    auto findFunction = stubModule.GetFunction(CommonStubCSigns::TryLoadICByValue);
    ecmascript::NativeAreaAllocator allocator;
    Circuit netOfGates(&allocator);
    CallSignature callSignature;
    TryLoadICByValueCallSignature::Initialize(&callSignature);
    Stub stub(&callSignature, &netOfGates);
    TryLoadICByValueStubBuilder optimizer(&callSignature, stub.GetEnvironment());
    stub.SetStubBuilder(&optimizer);
    stub.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    Scheduler::ControlFlowGraph cfg;
    Scheduler::Run(&netOfGates, cfg);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, findFunction, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run(*log);
}

HWTEST_F_L0(StubTest, TryStoreICByName)
{
    auto module = stubModule.GetModule();
    auto findFunction = stubModule.GetFunction(CommonStubCSigns::TryStoreICByName);
    ecmascript::NativeAreaAllocator allocator;
    Circuit netOfGates(&allocator);
    CallSignature callSignature;
    TryStoreICByNameCallSignature::Initialize(&callSignature);
    Stub stub(&callSignature, &netOfGates);
    TryStoreICByNameStubBuilder optimizer(&callSignature, stub.GetEnvironment());
    stub.SetStubBuilder(&optimizer);
    stub.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    Scheduler::ControlFlowGraph cfg;
    Scheduler::Run(&netOfGates, cfg);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, findFunction, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run(*log);
}

HWTEST_F_L0(StubTest, TryStoreICByValue)
{
    auto module = stubModule.GetModule();
    auto findFunction = stubModule.GetFunction(CommonStubCSigns::TryStoreICByValue);
    ecmascript::NativeAreaAllocator allocator;
    Circuit netOfGates(&allocator);
    CallSignature callSignature;
    TryStoreICByValueCallSignature::Initialize(&callSignature);
    Stub stub(&callSignature, &netOfGates);
    TryStoreICByValueStubBuilder optimizer(&callSignature, stub.GetEnvironment());
    stub.SetStubBuilder(&optimizer);
    stub.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    Scheduler::ControlFlowGraph cfg;
    Scheduler::Run(&netOfGates, cfg);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, findFunction, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run(*log);
}

struct ThreadTy {
    intptr_t magic;  // 0x11223344
    intptr_t fp;
};
class StubCallRunTimeThreadFpLock {
public:
    StubCallRunTimeThreadFpLock(struct ThreadTy *thread, intptr_t newFp) : oldRbp_(thread->fp), thread_(thread)
    {
        thread_->fp = *(reinterpret_cast<int64_t *>(newFp));
        LOG_COMPILER(INFO) << "StubCallRunTimeThreadFpLock newFp: " << newFp << " oldRbp_ : " << oldRbp_
                           << " thread_->fp:" << thread_->fp;
    }
    ~StubCallRunTimeThreadFpLock()
    {
        LOG_COMPILER(INFO) << "~StubCallRunTimeThreadFpLock oldRbp_: " << oldRbp_ << " thread_->fp:" << thread_->fp;
        thread_->fp = oldRbp_;
    }

private:
    intptr_t oldRbp_;
    struct ThreadTy *thread_;
};

extern "C" {
int64_t RuntimeFunc(struct ThreadTy *fpInfo)
{
    int64_t *rbp;
    asm("mov %%rbp, %0" : "=rm"(rbp));
    if (fpInfo->fp == *rbp) {
        return 1;
    }
    return 0;
}

int64_t (*g_stub2Func)(struct ThreadTy *) = nullptr;

int RuntimeFunc1(struct ThreadTy *fpInfo)
{
    LOG_COMPILER(INFO) << "RuntimeFunc1  -";
    int64_t newRbp;
    asm("mov %%rbp, %0" : "=rm"(newRbp));
    StubCallRunTimeThreadFpLock lock(fpInfo, newRbp);

    LOG_COMPILER(INFO) << std::hex << "g_stub2Func " << reinterpret_cast<uintptr_t>(g_stub2Func);
    if (g_stub2Func != nullptr) {
        g_stub2Func(fpInfo);
    }
    LOG_COMPILER(INFO) << "RuntimeFunc1  +";
    return 0;
}

int RuntimeFunc2(struct ThreadTy *fpInfo)
{
    LOG_COMPILER(INFO) << "RuntimeFunc2  -";
    // update thread.fp
    int64_t newRbp;
    asm("mov %%rbp, %0" : "=rm"(newRbp));
    StubCallRunTimeThreadFpLock lock(fpInfo, newRbp);
    auto rbp = reinterpret_cast<int64_t *>(fpInfo->fp);

    LOG_COMPILER(INFO) << " RuntimeFunc2 rbp:" << rbp;
    for (int i = 0; i < 40; i++) { // print 40 ptr value for debug
        LOG_COMPILER(INFO) << std::hex << &(rbp[i]) << " :" << rbp[i];
    }
    /* walk back
      stack frame:           0     pre rbp  <-- rbp
                            -8     type
                            -16    pre frame thread fp
    */
    int64_t *frameType = nullptr;
    int64_t *gcFp = nullptr;
    LOG_COMPILER(INFO) << "-----------------walkback----------------";
    do {
        frameType = rbp - 1;
        if (*frameType == 1) {
            gcFp = rbp - 2; // 2: 2 stack slot
        } else {
            gcFp = rbp;
        }
        rbp = reinterpret_cast<intptr_t *>(*gcFp);
        LOG_COMPILER(INFO) << std::hex << "frameType :" << *frameType << " gcFp:" << *gcFp;
    } while (*gcFp != 0);
    LOG_COMPILER(INFO) << "+++++++++++++++++walkback++++++++++++++++";
    LOG_COMPILER(INFO) << "call RuntimeFunc2 func ThreadTy fp: " << fpInfo->fp << " magic:" << fpInfo->magic;
    LOG_COMPILER(INFO) << "RuntimeFunc2  +";
    return 0;
}
}

/*
c++:main
  --> js (stub1(struct ThreadTy *))
        stack frame:           0     pre rbp  <-- rbp
                              -8     type
                              -16    pre frame thread fp
  --> c++(int RuntimeFunc1(struct ThreadTy *fpInfo))
  --> js (int stub2(struct ThreadTy *))
                                stack frame:           0     pre rbp  <-- rbp
                                -8     type
                                -16    pre frame thread fp
  --> js (int stub3(struct ThreadTy *))
                                stack frame:           0     pre rbp  <-- rbp
                                -8     type
  --> c++(int RuntimeFunc2(struct ThreadTy *fpInfo))

result:
-----------------walkback----------------
frameType :0 gcFp:7fffffffd780
frameType :1 gcFp:7fffffffd820
frameType :1 gcFp:0
+++++++++++++++++walkback++++++++++++++++
#0  __GI_raise (sig=sig@entry=6) at ../sysdeps/unix/sysv/linux/raise.c:40
#1  0x00007ffff03778b1 in __GI_abort () at abort.c:79
#2  0x0000555555610f1c in RuntimeFunc2 ()
#3  0x00007fffebf7b1fb in stub3 ()
#4  0x00007fffebf7b1ab in stub2 ()
#5  0x0000555555610afe in RuntimeFunc1 ()
#6  0x00007fffebf7b14e in stub1 ()
#7  0x000055555561197c in panda::test::StubTest_JSEntryTest_Test::TestBody() ()
*/

LLVMValueRef CallingFp(LLVMModuleRef &module, LLVMBuilderRef &builder)
{
    /* 0:calling 1:its caller */
    std::vector<LLVMValueRef> args = {LLVMConstInt(LLVMInt32Type(), 0, false)};
    auto fn = LLVMGetNamedFunction(module, "llvm.frameaddress.p0i8");
    if (!fn) {
        LOG_COMPILER(INFO) << "Could not find function ";
        return LLVMConstInt(LLVMInt64Type(), 0, false);
    }
    LLVMValueRef fAddrRet = LLVMBuildCall(builder, fn, args.data(), 1, "");
    LLVMValueRef frameAddr = LLVMBuildPtrToInt(builder, fAddrRet, LLVMInt64Type(), "cast_int64_t");
    return frameAddr;
}

HWTEST_F_L0(StubTest, GetPropertyByIndexStub)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetFunction(CommonStubCSigns::GetPropertyByIndex);
    ecmascript::NativeAreaAllocator allocator;
    Circuit netOfGates(&allocator);
    CallSignature callSignature;
    GetPropertyByIndexCallSignature::Initialize(&callSignature);
    Stub stub(&callSignature, &netOfGates);
    GetPropertyByIndexStubBuilder optimizer(&callSignature, stub.GetEnvironment());
    stub.SetStubBuilder(&optimizer);
    stub.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    Scheduler::ControlFlowGraph cfg;
    Scheduler::Run(&netOfGates, cfg);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run(*log);
    auto *getpropertyByIndex = reinterpret_cast<JSTaggedValue (*)(uintptr_t, JSTaggedValue, uint32_t)>(
        reinterpret_cast<uintptr_t>(assembler.GetFuncPtrFromCompiledModule(function)));
    auto *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    int x = 213;
    int y = 10;
    FastRuntimeStub::SetPropertyByIndex<true>(thread, obj.GetTaggedValue(), 1, JSTaggedValue(x));
    FastRuntimeStub::SetPropertyByIndex<true>(thread, obj.GetTaggedValue(), 10250, JSTaggedValue(y));
    JSTaggedValue resVal = getpropertyByIndex(thread->GetGlueAddr(), obj.GetTaggedValue(), 1);
    EXPECT_EQ(resVal.GetNumber(), x);
    resVal = getpropertyByIndex(thread->GetGlueAddr(), obj.GetTaggedValue(), 10250);
    EXPECT_EQ(resVal.GetNumber(), y);
}

HWTEST_F_L0(StubTest, GetPropertyByNameStub)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetFunction(CommonStubCSigns::DeprecatedGetPropertyByName);
    ecmascript::NativeAreaAllocator allocator;
    Circuit netOfGates(&allocator);
    CallSignature callSignature;
    DeprecatedGetPropertyByNameCallSignature::Initialize(&callSignature);
    Stub stub(&callSignature, &netOfGates);
    DeprecatedGetPropertyByNameStubBuilder optimizer(&callSignature, stub.GetEnvironment());
    stub.SetStubBuilder(&optimizer);
    stub.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    bool result = Verifier::Run(&netOfGates);
    ASSERT_TRUE(result);
    Scheduler::ControlFlowGraph cfg;
    Scheduler::Run(&netOfGates, cfg);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run(*log);
    auto *getPropertyByNamePtr = reinterpret_cast<JSTaggedValue (*)(uintptr_t, uint64_t, uint64_t)>(
        reinterpret_cast<uintptr_t>(assembler.GetFuncPtrFromCompiledModule(function)));
    auto *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    int x = 256;
    int y = 10;
    JSHandle<JSTaggedValue> strA(factory->NewFromASCII("a"));
    JSHandle<JSTaggedValue> strBig(factory->NewFromASCII("biggest"));
    FastRuntimeStub::SetPropertyByName(thread, obj.GetTaggedValue(), strA.GetTaggedValue(), JSTaggedValue(x));
    FastRuntimeStub::SetPropertyByName(thread, obj.GetTaggedValue(), strBig.GetTaggedValue(), JSTaggedValue(y));
    JSTaggedValue resVal = getPropertyByNamePtr(thread->GetGlueAddr(), obj.GetTaggedValue().GetRawData(),
        strA.GetTaggedValue().GetRawData());
    EXPECT_EQ(resVal.GetNumber(), x);
    resVal = getPropertyByNamePtr(thread->GetGlueAddr(), obj.GetTaggedValue().GetRawData(),
                                  strBig.GetTaggedValue().GetRawData());
    EXPECT_EQ(resVal.GetNumber(), y);
}

HWTEST_F_L0(StubTest, GetPropertyByValueStub)
{
    auto module = stubModule.GetModule();
    LLVMValueRef getPropertyByIndexfunction = stubModule.GetFunction(CommonStubCSigns::GetPropertyByIndex);
    ecmascript::NativeAreaAllocator allocator;
    Circuit netOfGates2(&allocator);
    CallSignature callSignature;
    GetPropertyByIndexCallSignature::Initialize(&callSignature);
    Stub stub(&callSignature, &netOfGates2);
    GetPropertyByIndexStubBuilder optimizer(&callSignature, stub.GetEnvironment());
    stub.SetStubBuilder(&optimizer);
    stub.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates2.PrintAllGates();
    Scheduler::ControlFlowGraph cfg;
    Scheduler::Run(&netOfGates2, cfg);
    LLVMIRBuilder llvmBuilder2(&cfg, &netOfGates2, &stubModule, getPropertyByIndexfunction,
        stubModule.GetCompilationConfig(),  CallSignature::CallConv::CCallConv);
    llvmBuilder2.Build();

    LLVMValueRef getPropertyByNamefunction = stubModule.GetFunction(CommonStubCSigns::DeprecatedGetPropertyByName);
    Circuit netOfGates1(&allocator);
    CallSignature callSignature1;
    DeprecatedGetPropertyByNameCallSignature::Initialize(&callSignature1);
    Stub stub1(&callSignature1, &netOfGates1);
    DeprecatedGetPropertyByNameStubBuilder getPropertyByNameStub(&callSignature, stub1.GetEnvironment());
    stub1.SetStubBuilder(&getPropertyByNameStub);
    stub1.GenerateCircuit(stubModule.GetCompilationConfig());
    bool result = Verifier::Run(&netOfGates1);
    ASSERT_TRUE(result);
    Scheduler::ControlFlowGraph cfg1;
    Scheduler::Run(&netOfGates1, cfg1);
    LLVMIRBuilder llvmBuilder1(&cfg1, &netOfGates1, &stubModule, getPropertyByNamefunction,
        stubModule.GetCompilationConfig(), CallSignature::CallConv::CCallConv);
    llvmBuilder1.Build();

    LLVMValueRef function = stubModule.GetFunction(CommonStubCSigns::DeprecatedGetPropertyByValue);
    Circuit netOfGates(&allocator);
    CallSignature callSignature2;
    DeprecatedGetPropertyByValueCallSignature::Initialize(&callSignature2);
    Stub stub2(&callSignature2, &netOfGates);
    DeprecatedGetPropertyByValueStubBuilder getPropertyByValueStub(&callSignature, stub2.GetEnvironment());
    stub2.SetStubBuilder(&getPropertyByValueStub);
    stub2.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    result = Verifier::Run(&netOfGates);
    ASSERT_TRUE(result);
    Scheduler::ControlFlowGraph cfg2;
    Scheduler::Run(&netOfGates, cfg2);
    PrintCircuitByBasicBlock(cfg2, netOfGates);

    LLVMIRBuilder llvmBuilder(&cfg2, &netOfGates, &stubModule, function, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run(*log);
    auto *getPropertyByValuePtr = reinterpret_cast<JSTaggedValue (*)(uintptr_t, uint64_t, uint64_t)>(
        reinterpret_cast<uintptr_t>(assembler.GetFuncPtrFromCompiledModule(function)));
    auto *getPropertyByNamePtr = reinterpret_cast<JSTaggedValue (*)(uintptr_t, uint64_t, uint64_t)>(
        reinterpret_cast<uintptr_t>(assembler.GetFuncPtrFromCompiledModule(getPropertyByNamefunction)));
    auto *getpropertyByIndexPtr = reinterpret_cast<JSTaggedValue (*)(uintptr_t, JSTaggedValue, uint32_t)>(
        reinterpret_cast<uintptr_t>(assembler.GetFuncPtrFromCompiledModule(getPropertyByIndexfunction)));

    thread->SetFastStubEntry(CommonStubCSigns::GetPropertyByIndex, reinterpret_cast<uintptr_t>(getpropertyByIndexPtr));
    thread->SetFastStubEntry(CommonStubCSigns::DeprecatedGetPropertyByName,
        reinterpret_cast<uintptr_t>(getPropertyByNamePtr));
    auto *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    int x = 213;
    int y = 10;
    auto sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
    FastRuntimeStub::SetPropertyByIndex<true>(thread, obj.GetTaggedValue(), 1, JSTaggedValue(x));
    FastRuntimeStub::SetPropertyByIndex<true>(thread, obj.GetTaggedValue(), 10250, JSTaggedValue(y));

    JSHandle<JSTaggedValue> strA(factory->NewFromASCII("a"));
    JSHandle<JSTaggedValue> strBig(factory->NewFromASCII("biggest"));
    JSHandle<JSTaggedValue> strDigit(factory->NewFromASCII("10250"));

    FastRuntimeStub::SetPropertyByName(thread, obj.GetTaggedValue(), strA.GetTaggedValue(), JSTaggedValue(x));
    FastRuntimeStub::SetPropertyByName(thread, obj.GetTaggedValue(), strBig.GetTaggedValue(), JSTaggedValue(y));
    JSTaggedValue resVal1 = getPropertyByNamePtr(thread->GetGlueAddr(), obj.GetTaggedValue().GetRawData(),
        strA.GetTaggedValue().GetRawData());
    EXPECT_EQ(resVal1.GetNumber(), x);
    JSTaggedValue resVal = getPropertyByValuePtr(thread->GetGlueAddr(), obj.GetTaggedValue().GetRawData(),
        strA.GetTaggedValue().GetRawData());
    EXPECT_EQ(resVal.GetNumber(), x);
    resVal = getPropertyByValuePtr(thread->GetGlueAddr(), obj.GetTaggedValue().GetRawData(),
                                   strBig.GetTaggedValue().GetRawData());
    EXPECT_EQ(resVal.GetNumber(), y);
    resVal = getpropertyByIndexPtr(thread->GetGlueAddr(), obj.GetTaggedValue(), 1);
    EXPECT_EQ(resVal.GetNumber(), x);
    resVal = getPropertyByValuePtr(thread->GetGlueAddr(), obj.GetTaggedValue().GetRawData(),
                                   JSTaggedValue(10250).GetRawData());
    EXPECT_EQ(resVal.GetNumber(), y);
    resVal = getPropertyByValuePtr(thread->GetGlueAddr(), obj.GetTaggedValue().GetRawData(),
                                   strDigit.GetTaggedValue().GetRawData());
    EXPECT_EQ(resVal.GetNumber(), y);
    thread->SetCurrentSPFrame(sp);
    thread->SetLastLeaveFrame(nullptr);
    JSHandle<JSTaggedValue> strHello(factory->NewFromASCII("hello world"));
    double key = 4.29497e+09;
    resVal = getPropertyByValuePtr(thread->GetGlueAddr(), strHello.GetTaggedValue().GetRawData(),
                                   JSTaggedValue(key).GetRawData());
    EXPECT_EQ(resVal.GetRawData(), JSTaggedValue::Hole().GetRawData());
}

HWTEST_F_L0(StubTest, FastTypeOfTest)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetFunction(CommonStubCSigns::TypeOf);
    ecmascript::NativeAreaAllocator allocator;
    Circuit netOfGates(&allocator);
    CallSignature callSignature;
    TypeOfCallSignature::Initialize(&callSignature);
    Stub stub(&callSignature, &netOfGates);
    TypeOfStubBuilder optimizer(&callSignature, stub.GetEnvironment());
    stub.SetStubBuilder(&optimizer);
    stub.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    bool verRes = Verifier::Run(&netOfGates);
    ASSERT_TRUE(verRes);
    Scheduler::ControlFlowGraph cfg;
    Scheduler::Run(&netOfGates, cfg);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    char *error = nullptr;
    LLVMVerifyModule(module, LLVMAbortProcessAction, &error);
    LLVMAssembler assembler(module);
    assembler.Run(*log);
    auto *typeOfPtr =
        reinterpret_cast<JSTaggedValue (*)(uintptr_t, uint64_t)>(assembler.GetFuncPtrFromCompiledModule(function));
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    // obj is JSTaggedValue::VALUE_TRUE
    JSTaggedValue resultVal = typeOfPtr(thread->GetGlueAddr(), JSTaggedValue::True().GetRawData());
    JSTaggedValue expectResult = FastRuntimeStub::FastTypeOf(thread, JSTaggedValue::True());
    EXPECT_EQ(resultVal, globalConst->GetBooleanString());
    EXPECT_EQ(resultVal, expectResult);

    // obj is JSTaggedValue::VALUE_FALSE
    JSTaggedValue resultVal2 = typeOfPtr(thread->GetGlueAddr(), JSTaggedValue::False().GetRawData());
    JSTaggedValue expectResult2 = FastRuntimeStub::FastTypeOf(thread, JSTaggedValue::False());
    EXPECT_EQ(resultVal2, globalConst->GetBooleanString());
    EXPECT_EQ(resultVal2, expectResult2);

    // obj is JSTaggedValue::VALUE_NULL
    JSTaggedValue resultVal3 = typeOfPtr(thread->GetGlueAddr(), JSTaggedValue::Null().GetRawData());
    JSTaggedValue expectResult3 = FastRuntimeStub::FastTypeOf(thread, JSTaggedValue::Null());
    EXPECT_EQ(resultVal3, globalConst->GetObjectString());
    EXPECT_EQ(resultVal3, expectResult3);

    // obj is JSTaggedValue::VALUE_UNDEFINED
    JSTaggedValue resultVal4 = typeOfPtr(thread->GetGlueAddr(), JSTaggedValue::Undefined().GetRawData());
    JSTaggedValue expectResult4 = FastRuntimeStub::FastTypeOf(thread, JSTaggedValue::Undefined());
    EXPECT_EQ(resultVal4, globalConst->GetUndefinedString());
    EXPECT_EQ(resultVal4, expectResult4);

    // obj is IsNumber
    JSTaggedValue resultVal5 = typeOfPtr(thread->GetGlueAddr(), JSTaggedValue(5).GetRawData());
    JSTaggedValue expectResult5 = FastRuntimeStub::FastTypeOf(thread, JSTaggedValue(5));
    EXPECT_EQ(resultVal5, globalConst->GetNumberString());
    EXPECT_EQ(resultVal5, expectResult5);

    // obj is String
    auto *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> str1 = factory->NewFromASCII("a");
    JSHandle<EcmaString> str2 = factory->NewFromASCII("a");
    JSTaggedValue expectResult6 = FastRuntimeStub::FastTypeOf(thread, str1.GetTaggedValue());
    JSTaggedValue resultVal6 = typeOfPtr(thread->GetGlueAddr(), str2.GetTaggedValue().GetRawData());
    EXPECT_EQ(resultVal6, globalConst->GetStringString());
    EXPECT_EQ(resultVal6, expectResult6);

    // obj is Symbol
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSTaggedValue symbol = globalEnv->GetIteratorSymbol().GetTaggedValue();
    JSTaggedValue expectResult7= FastRuntimeStub::FastTypeOf(thread, symbol);
    JSTaggedValue resultVal7 = typeOfPtr(thread->GetGlueAddr(), symbol.GetRawData());
    EXPECT_EQ(resultVal7, globalConst->GetSymbolString());
    EXPECT_EQ(resultVal7, expectResult7);

    // obj is callable
    JSHandle<JSPromiseReactionsFunction> resolveCallable = factory->CreateJSPromiseReactionsFunction(
        MethodIndex::BUILTINS_PROMISE_HANDLER_RESOLVE);
    JSTaggedValue expectResult8= FastRuntimeStub::FastTypeOf(thread, resolveCallable.GetTaggedValue());
    JSTaggedValue resultVal8 = typeOfPtr(thread->GetGlueAddr(), resolveCallable.GetTaggedValue().GetRawData());
    EXPECT_EQ(resultVal8, globalConst->GetFunctionString());
    EXPECT_EQ(resultVal8, expectResult8);

    // obj is heapObject
    JSHandle<JSObject> object = factory->NewEmptyJSObject();
    JSTaggedValue expectResult9= FastRuntimeStub::FastTypeOf(thread, object.GetTaggedValue());
    JSTaggedValue resultVal9 = typeOfPtr(thread->GetGlueAddr(), object.GetTaggedValue().GetRawData());
    EXPECT_EQ(resultVal9, globalConst->GetObjectString());
    EXPECT_EQ(resultVal9, expectResult9);
}

HWTEST_F_L0(StubTest, FastEqualTest)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetFunction(CommonStubCSigns::Equal);
    ecmascript::NativeAreaAllocator allocator;
    Circuit netOfGates(&allocator);
    CallSignature callSignature;
    EqualCallSignature::Initialize(&callSignature);
    Stub stub(&callSignature, &netOfGates);
    EqualStubBuilder optimizer(&callSignature, stub.GetEnvironment());
    stub.SetStubBuilder(&optimizer);
    stub.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    Scheduler::ControlFlowGraph cfg;
    Scheduler::Run(&netOfGates, cfg);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run(*log);
    auto fn = reinterpret_cast<JSTaggedValue (*)(uintptr_t, int64_t, int64_t)>(
        assembler.GetFuncPtrFromCompiledModule(function));
    // test for 1 == 1
    auto resA = fn(thread->GetGlueAddr(), JSTaggedValue(1).GetRawData(), JSTaggedValue(1).GetRawData());
    auto expectA = FastRuntimeStub::FastEqual(JSTaggedValue(1), JSTaggedValue(1));
    EXPECT_EQ(resA, expectA);

    // test for nan == nan
    double nan = std::numeric_limits<double>::quiet_NaN();
    auto resB = fn(thread->GetGlueAddr(), JSTaggedValue(nan).GetRawData(), JSTaggedValue(nan).GetRawData());
    auto expectB = FastRuntimeStub::FastEqual(JSTaggedValue(nan), JSTaggedValue(nan));
    EXPECT_EQ(resB, expectB);

    // test for undefined == null
    auto resC = fn(thread->GetGlueAddr(), JSTaggedValue::Undefined().GetRawData(), JSTaggedValue::Null().GetRawData());
    auto expectC = FastRuntimeStub::FastEqual(JSTaggedValue::Undefined(), JSTaggedValue::Null());
    EXPECT_EQ(resC, expectC);

    // test for "hello world" == undefined
    auto *factory = thread->GetEcmaVM()->GetFactory();
    auto str = factory->NewFromASCII("hello world");
    auto resD = fn(thread->GetGlueAddr(), str.GetTaggedValue().GetRawData(), JSTaggedValue::Undefined().GetRawData());
    auto expectD = FastRuntimeStub::FastEqual(str.GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(resD, expectD);

    // test for true == hole
    auto resE = fn(thread->GetGlueAddr(), JSTaggedValue::True().GetRawData(), JSTaggedValue::Hole().GetRawData());
    auto expectE = FastRuntimeStub::FastEqual(JSTaggedValue::True(), JSTaggedValue::Hole());
    EXPECT_EQ(resE, expectE);

    // test for "hello world" == "hello world"
    auto resF = fn(thread->GetGlueAddr(), str.GetTaggedValue().GetRawData(), str.GetTaggedValue().GetRawData());
    auto expectF = FastRuntimeStub::FastEqual(str.GetTaggedValue(), str.GetTaggedValue());
    EXPECT_EQ(resF, expectF);

    // test for 5.2 == 5.2
    auto resG = fn(thread->GetGlueAddr(), JSTaggedValue(5.2).GetRawData(), JSTaggedValue(5.2).GetRawData());
    auto expectG = FastRuntimeStub::FastEqual(JSTaggedValue(5.2), JSTaggedValue(5.2));
    EXPECT_EQ(resG, expectG);

    // test for false == false
    auto resH = fn(thread->GetGlueAddr(), JSTaggedValue::False().GetRawData(), JSTaggedValue::False().GetRawData());
    auto expectH = FastRuntimeStub::FastEqual(JSTaggedValue::False(), JSTaggedValue::False());
    EXPECT_EQ(resH, expectH);

    // test for obj == obj
    JSHandle<JSObject> obj1 = factory->NewEmptyJSObject();
    JSHandle<JSObject> obj2 = factory->NewEmptyJSObject();
    FastRuntimeStub::SetPropertyByIndex<true>(thread, obj1.GetTaggedValue(), 1, JSTaggedValue(1));
    FastRuntimeStub::SetPropertyByIndex<true>(thread, obj2.GetTaggedValue(), 1, JSTaggedValue(1));
    auto resI = fn(thread->GetGlueAddr(), obj1.GetTaggedValue().GetRawData(), obj2.GetTaggedValue().GetRawData());
    auto expectI = JSTaggedValue(FastRuntimeStub::FastStrictEqual(obj1.GetTaggedValue(), obj2.GetTaggedValue()));
    EXPECT_EQ(resI, expectI);
}

#ifndef NDEBUG
HWTEST_F_L0(StubTest, JSCallTest)
{
    auto fooEntry = thread->GetFastStubEntry(CommonStubCSigns::FooAOT);
    auto footarget = NewAotFunction(2, fooEntry);
    auto glue = thread->GetGlueAddr();
    int x = 1;
    int y = 2;
    JSTaggedType argV[5] = {
        footarget.GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue(x).GetRawData(),
        JSTaggedValue(y).GetRawData(),
    };
    auto entry = thread->GetRTInterface(kungfu::RuntimeStubCSigns::ID_JSFunctionEntry);

    auto result = reinterpret_cast<JSFunctionEntryType>(entry)(glue, 2, argV,
        reinterpret_cast<uintptr_t>(thread->GetCurrentSPFrame()),
        static_cast<size_t>(OptimizedEntryFrame::CallType::CALL_FUNC));
    EXPECT_EQ(result, JSTaggedValue(3.0));
}

HWTEST_F_L0(StubTest, JSCallTest1)
{
    auto foo2Entry = thread->GetFastStubEntry(CommonStubCSigns::Foo2AOT);
    auto foo2target = NewAotFunction(2, foo2Entry);
    auto glue = thread->GetGlueAddr();
    int x = 1;
    int y = 2;
    JSTaggedType argV[5] = {
        foo2target.GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue(x).GetRawData(),
        JSTaggedValue(y).GetRawData(),
    };
    auto entry = thread->GetRTInterface(kungfu::RuntimeStubCSigns::ID_JSFunctionEntry);
    auto result = reinterpret_cast<JSFunctionEntryType>(entry)(glue, 2, argV,
        reinterpret_cast<uintptr_t>(thread->GetCurrentSPFrame()),
        static_cast<size_t>(OptimizedEntryFrame::CallType::CALL_FUNC));
    EXPECT_EQ(result, JSTaggedValue(3.0));
}

HWTEST_F_L0(StubTest, JSCallTest2)
{
    auto foo1Entry = thread->GetFastStubEntry(CommonStubCSigns::Foo1AOT);
    auto foo1target = NewAotFunction(2, foo1Entry);
    auto glue = thread->GetGlueAddr();
    int x = 1;
    int y = 2;
    JSTaggedType argV[5] = {
        foo1target.GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue(x).GetRawData(),
        JSTaggedValue(y).GetRawData(),
    };

    auto entry = thread->GetRTInterface(kungfu::RuntimeStubCSigns::ID_JSFunctionEntry);
    auto result = reinterpret_cast<JSFunctionEntryType>(entry)(glue, 2, argV,
        reinterpret_cast<uintptr_t>(thread->GetCurrentSPFrame()),
        static_cast<size_t>(OptimizedEntryFrame::CallType::CALL_FUNC));
    EXPECT_EQ(result, JSTaggedValue(0x7ff9000000000000UL));
}

HWTEST_F_L0(StubTest, JSCallNativeTest)
{
    auto fooEntry = thread->GetFastStubEntry(CommonStubCSigns::FooNativeAOT);
    auto footarget = NewAotFunction(2, fooEntry);
    auto glue = thread->GetGlueAddr();
    int x = 1;
    int y = 2;
    JSTaggedType argV[5] = {
        footarget.GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue(x).GetRawData(),
        JSTaggedValue(y).GetRawData(),
    };
    auto entry = thread->GetRTInterface(kungfu::RuntimeStubCSigns::ID_JSFunctionEntry);
    auto result = reinterpret_cast<JSFunctionEntryType>(entry)(glue, 2, argV,
        reinterpret_cast<uintptr_t>(thread->GetCurrentSPFrame()),
        static_cast<size_t>(OptimizedEntryFrame::CallType::CALL_FUNC));
    EXPECT_EQ(result, JSTaggedValue::Undefined());
}

HWTEST_F_L0(StubTest, JSCallBoundTest)
{
    auto fooEntry = thread->GetFastStubEntry(CommonStubCSigns::FooBoundAOT);
    auto footarget = NewAotFunction(2, fooEntry);
    auto glue = thread->GetGlueAddr();
    int x = 1;
    int y = 2;
    JSTaggedType argV[5] = {
        footarget.GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue(x).GetRawData(),
        JSTaggedValue(y).GetRawData(),
    };

    auto entry = thread->GetRTInterface(kungfu::RuntimeStubCSigns::ID_JSFunctionEntry);
    auto result = reinterpret_cast<JSFunctionEntryType>(entry)(glue, 2, argV,
        reinterpret_cast<uintptr_t>(thread->GetCurrentSPFrame()),
        static_cast<size_t>(OptimizedEntryFrame::CallType::CALL_FUNC));
    EXPECT_EQ(result, JSTaggedValue(38.0));
}

// test for proxy method is undefined
HWTEST_F_L0(StubTest, JSCallTest3)
{
    auto fooProxyEntry = thread->GetFastStubEntry(CommonStubCSigns::FooProxyAOT);
    auto foo2target = NewAotFunction(2, fooProxyEntry);
    auto glue = thread->GetGlueAddr();
    int x = 1;
    int y = 2;
    JSTaggedType argV[6] = {
        foo2target.GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue(x).GetRawData(),
        JSTaggedValue(y).GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
    };
    JSThread::GlueData::GetCOStubEntriesOffset(false);
    JSThread::GlueData::GetCOStubEntriesOffset(true);
    auto entry = thread->GetRTInterface(kungfu::RuntimeStubCSigns::ID_JSFunctionEntry);
    auto result = reinterpret_cast<JSFunctionEntryType>(entry)(glue, 3, argV,
        reinterpret_cast<uintptr_t>(thread->GetCurrentSPFrame()),
        static_cast<size_t>(OptimizedEntryFrame::CallType::CALL_FUNC));
    EXPECT_EQ(result, JSTaggedValue(3.0));
}

// test for proxy method isn't undefined
HWTEST_F_L0(StubTest, JSCallTest4)
{
    auto fooProxyEntry = thread->GetFastStubEntry(CommonStubCSigns::FooProxy2AOT);
    auto foo2target = NewAotFunction(2, fooProxyEntry);
    auto glue = thread->GetGlueAddr();
    int x = 1;
    int y = 2;
    JSTaggedType argV[5] = {
        foo2target.GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue(x).GetRawData(),
        JSTaggedValue(y).GetRawData(),
    };
    JSThread::GlueData::GetCOStubEntriesOffset(false);
    JSThread::GlueData::GetCOStubEntriesOffset(true);
    auto entry = thread->GetRTInterface(kungfu::RuntimeStubCSigns::ID_JSFunctionEntry);
    [[maybe_unused]] auto result = reinterpret_cast<JSFunctionEntryType>(entry)(glue, 2, argV,
        reinterpret_cast<uintptr_t>(thread->GetCurrentSPFrame()),
        static_cast<size_t>(OptimizedEntryFrame::CallType::CALL_FUNC));
}

HWTEST_F_L0(StubTest, RelocateTest)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetFunction(CommonStubCSigns::TestAbsoluteAddressRelocation);
    ecmascript::NativeAreaAllocator allocator;
    Circuit netOfGates(&allocator);
    CallSignature callSignature;
    TestAbsoluteAddressRelocationCallSignature::Initialize(&callSignature);
    Stub stub(&callSignature, &netOfGates);
    TestAbsoluteAddressRelocationStubBuilder optimizer(&callSignature, stub.GetEnvironment());
    stub.SetStubBuilder(&optimizer);
    stub.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    bool verRes = Verifier::Run(&netOfGates);
    ASSERT_TRUE(verRes);
    Scheduler::ControlFlowGraph cfg;
    Scheduler::Run(&netOfGates, cfg);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    char *error = nullptr;
    LLVMVerifyModule(module, LLVMAbortProcessAction, &error);
    LLVMAssembler assembler(module);
    assembler.Run(*log);
    uint64_t input = 0x111;
    auto *ptr =
        reinterpret_cast<JSTaggedValue (*)(uint64_t)>(assembler.GetFuncPtrFromCompiledModule(function));
    auto loader = thread->GetEcmaVM()->GetAOTFileManager();
    auto dataSecAddr = assembler.GetSectionAddr(ElfSecName::DATA);
    auto dataSecSize = assembler.GetSectionSize(ElfSecName::DATA);
    std::vector<uint64_t> newData;
    newData.push_back(input);
    loader->RewriteDataSection(dataSecAddr, dataSecSize,
        reinterpret_cast<uintptr_t>(newData.data()), newData.size() * sizeof(uint64_t));
    auto res = ptr(input);
    uint64_t expect = 1;
    EXPECT_EQ(res.GetRawData(), expect);
}
#endif
}  // namespace panda::test
