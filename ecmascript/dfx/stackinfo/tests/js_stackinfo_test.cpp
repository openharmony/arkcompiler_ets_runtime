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

#include <cstdio>

#include "assembler/assembly-emitter.h"
#include "assembler/assembly-parser.h"
#include "libpandafile/class_data_accessor-inl.h"
#include "libziparchive/zip_archive.h"
#include "ecmascript/dfx/stackinfo/js_stackinfo.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class JsStackInfoTest : public testing::Test {
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

uintptr_t ToUintPtr(FrameType frame)
{
    return static_cast<uintptr_t>(frame);
}

bool ReadMemFunc([[maybe_unused]] void *ctx, uintptr_t addr, uintptr_t *value)
{
    *value = *(reinterpret_cast<uintptr_t *>(addr));
    return true;
}

/**
 * @tc.name: ArkFrameCheck
 * @tc.desc: Check whether the result returned through "ArkFrameCheck" function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsStackInfoTest, TestArkFrameCheck)
{
    for (uintptr_t i = 0; i < 25; i++) {
        bool ret = ArkFrameCheck(i);
        if (i == ToUintPtr(FrameType::OPTIMIZED_ENTRY_FRAME) ||
            i == ToUintPtr(FrameType::ASM_INTERPRETER_ENTRY_FRAME)) {
            EXPECT_TRUE(ret == true);
        } else {
            EXPECT_TRUE(ret == false);
        }
    }
}

/**
 * @tc.name: IsJsFunctionFrame
 * @tc.desc: Check whether the result returned through "IsJsFunctionFrame" function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsStackInfoTest, TestIsJsFunctionFrame)
{
    for (uintptr_t i = 0; i < 25; i++) {
        bool ret = IsJsFunctionFrame(i);
        if (i == ToUintPtr(FrameType::ASM_INTERPRETER_FRAME) ||
            i == ToUintPtr(FrameType::INTERPRETER_CONSTRUCTOR_FRAME) ||
            i == ToUintPtr(FrameType::INTERPRETER_FRAME) ||
            i == ToUintPtr(FrameType::INTERPRETER_FAST_NEW_FRAME)) {
            EXPECT_TRUE(ret == true);
        } else {
            EXPECT_TRUE(ret == false);
        }
    }
}

/**
 * @tc.name: IsFastJitFunctionFrame
 * @tc.desc: Check whether the result returned through "IsFastJitFunctionFrame" function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsStackInfoTest, TestIsFastJitFunctionFrame)
{
    for (uintptr_t i = 0; i < 25; i++) {
        bool ret = IsFastJitFunctionFrame(i);
        if (i == ToUintPtr(FrameType::FASTJIT_FUNCTION_FRAME) ||
            i == ToUintPtr(FrameType::FASTJIT_FAST_CALL_FUNCTION_FRAME)) {
            EXPECT_TRUE(ret == true);
        } else {
            EXPECT_TRUE(ret == false);
        }
    }
}

/**
 * @tc.name: IsNativeFunctionFrame
 * @tc.desc: Check whether the result returned through "IsNativeFunctionFrame" function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsStackInfoTest, TestIsNativeFunctionFrame)
{
    for (uintptr_t i = 0; i < 25; i++) {
        bool ret = IsNativeFunctionFrame(i);
        if (i == ToUintPtr(FrameType::OPTIMIZED_FRAME) ||
            i == ToUintPtr(FrameType::BASELINE_BUILTIN_FRAME) ||
            i == ToUintPtr(FrameType::ASM_BRIDGE_FRAME) ||
            i == ToUintPtr(FrameType::OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME) ||
            i == ToUintPtr(FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME) ||
            i == ToUintPtr(FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME) ||
            i == ToUintPtr(FrameType::OPTIMIZED_JS_FUNCTION_FRAME) ||
            i == ToUintPtr(FrameType::LEAVE_FRAME) ||
            i == ToUintPtr(FrameType::LEAVE_FRAME_WITH_ARGV) ||
            i == ToUintPtr(FrameType::BUILTIN_CALL_LEAVE_FRAME) ||
            i == ToUintPtr(FrameType::BUILTIN_FRAME) ||
            i == ToUintPtr(FrameType::BUILTIN_ENTRY_FRAME) ||
            i == ToUintPtr(FrameType::BUILTIN_FRAME_WITH_ARGV) ||
            i == ToUintPtr(FrameType::BUILTIN_FRAME_WITH_ARGV_STACK_OVER_FLOW_FRAME) ||
            i == ToUintPtr(FrameType::ASM_INTERPRETER_BRIDGE_FRAME)) {
            EXPECT_TRUE(ret == true);
        } else {
            EXPECT_TRUE(ret == false);
        }
    }
}

/**
 * @tc.name: IsAotFunctionFrame
 * @tc.desc: Check whether the result returned through "IsAotFunctionFrame" function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsStackInfoTest, TestIsAotFunctionFrame)
{
    for (uintptr_t i = 0; i < 25; i++) {
        bool ret = IsAotFunctionFrame(i);
        if (i == ToUintPtr(FrameType::OPTIMIZED_JS_FUNCTION_FRAME) ||
            i == ToUintPtr(FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME)) {
            EXPECT_TRUE(ret == true);
        } else {
            EXPECT_TRUE(ret == false);
        }
    }
}

/**
 * @tc.name: ReadMethodInfo
 * @tc.desc: Check whether the result returned through "ReadMethodInfo" function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsStackInfoTest, TestReadMethodInfo)
{
    const char *filename = "__JSPandaFileManagerTest1.pa";
    const char *data = R"(
        .function void foo() {
            return
        }
    )";
    pandasm::Parser parser;
    auto res = parser.Parse(data);
    auto file = pandasm::AsmEmitter::Emit(res.Value());
    auto jsPandaFile = std::make_shared<JSPandaFile>(file.release(), CString(filename));
    EXPECT_TRUE(jsPandaFile != nullptr);
    CVector<MethodInfo> result;
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    Span<const uint32_t> classIndexes = jsPandaFile->GetClasses();
    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (jsPandaFile->IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(*pf, classId);
        cda.EnumerateMethods([&result, jsPandaFile](panda_file::MethodDataAccessor &mda) {
            auto info = JSStackTrace::ReadMethodInfo(mda);
            if (!info) {
                return;
            }
            result.push_back(info.value());
        });
    }
    EXPECT_TRUE(result.size() > 0);
}

/**
 * @tc.name: ReadAllMethodInfos
 * @tc.desc: Check whether the result returned through "ReadAllMethodInfos" function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsStackInfoTest, TestReadAllMethodInfos)
{
    const char *filename = "__JsStackInfoTest.pa";
    const char *data = R"(
        .function void foo() {
            return
        }
    )";
    pandasm::Parser parser;
    auto res = parser.Parse(data);
    auto file = pandasm::AsmEmitter::Emit(res.Value());
    auto pf = std::make_shared<JSPandaFile>(file.release(), CString(filename));
    EXPECT_TRUE(pf != nullptr);
    auto methods = JSStackTrace::ReadAllMethodInfos(pf);
    EXPECT_TRUE(methods.size() > 0);
    pf = nullptr;
    methods = JSStackTrace::ReadAllMethodInfos(pf);
    EXPECT_TRUE(methods.size() == 0);
}

/**
 * @tc.name: TranslateByteCodePc
 * @tc.desc: Check whether the result returned through "TranslateByteCodePc" function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsStackInfoTest, TestTranslateByteCodePc)
{
    CVector<MethodInfo> vec = {
        {0, 0, 24},
        {1, 24, 30},
        {2, 54, 56},
        {3, 110, 60}
    };
    uintptr_t realPc = 115;

    auto ret = JSStackTrace::TranslateByteCodePc(realPc, vec);
    EXPECT_TRUE(ret != std::nullopt);

    vec.clear();
    ret = JSStackTrace::TranslateByteCodePc(realPc, vec);
    EXPECT_TRUE(ret == std::nullopt);

    vec.push_back({2, 54, 56});
    ret = JSStackTrace::TranslateByteCodePc(realPc, vec);
    EXPECT_TRUE(ret == std::nullopt);
}

/**
 * @tc.name: ParseJsFrameInfo
 * @tc.desc: Check whether the result returned through "ParseJsFrameInfo" function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsStackInfoTest, TestParseJsFrameInfo)
{
    const char *filename = "__JsStackInfoTest.pa";
    const char *data = R"(
        .function void foo() {
            return
        }
    )";
    pandasm::Parser parser;
    auto res = parser.Parse(data);
    auto file = pandasm::AsmEmitter::Emit(res.Value());
    auto jsPandaFile = std::make_shared<JSPandaFile>(file.release(), CString(filename));
    EXPECT_TRUE(jsPandaFile != nullptr);
    auto debugExtractor = std::make_unique<DebugInfoExtractor>(jsPandaFile.get());
    auto methods = JSStackTrace::ReadAllMethodInfos(jsPandaFile);
    uintptr_t offset = 0;
    JsFunction jsFunction;
    ParseJsFrameInfo(jsPandaFile.get(), debugExtractor.get(), EntityId(methods[0].methodId), offset, jsFunction);
    EXPECT_TRUE(std::string(jsFunction.functionName) == "foo");
}

/**
 * @tc.name: ArkParseJsFrameInfo
 * @tc.desc: Check whether the result returned through "ArkCreateJSSymbolExtractor" function is within expectations;
 *           Check whether the result returned through "ArkParseJsFrameInfo" function is within expectations;
 *           Check whether the result returned through "ArkDestoryJSSymbolExtractor" function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsStackInfoTest, TestArkParseJsFrameInfo)
{
    const char *filename1 = "__JsStackInfoTest1.pa";
    const char *filename2 = "__JsStackInfoTest2.pa";
    const char *pfdata1 = R"(
        .function void foo() {
            return
        }
    )";
    const char *pfdata2 = R"(
        .language ECMAScript
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            return
        }
    )";
    pandasm::Parser parser1;
    pandasm::Parser parser2;
    auto res1 = parser1.Parse(pfdata1);
    auto res2 = parser2.Parse(pfdata2);
    auto file1 = pandasm::AsmEmitter::Emit(res1.Value());
    auto file2 = pandasm::AsmEmitter::Emit(res2.Value());
    auto jsPandaFile1 = std::make_shared<JSPandaFile>(file1.release(), CString(filename1));
    auto jsPandaFile2 = std::make_shared<JSPandaFile>(file2.release(), CString(filename2));
    EXPECT_TRUE(jsPandaFile1 != nullptr);
    EXPECT_TRUE(jsPandaFile2 != nullptr);
    auto debugExtractor1 = std::make_unique<DebugInfoExtractor>(jsPandaFile1.get());
    auto debugExtractor2 = std::make_unique<DebugInfoExtractor>(jsPandaFile2.get());
    auto methods1 = JSStackTrace::ReadAllMethodInfos(jsPandaFile1);
    auto methods2 = JSStackTrace::ReadAllMethodInfos(jsPandaFile2);

    uintptr_t byteCodePc1 = methods1[0].codeBegin;
    uintptr_t byteCodePc2 = methods2[0].codeBegin;
    uintptr_t mapBase1 = reinterpret_cast<uintptr_t>(jsPandaFile1->GetHeader());
    uintptr_t mapBase2 = reinterpret_cast<uintptr_t>(jsPandaFile2->GetHeader());
    uintptr_t loadOffset1 = 0;
    uintptr_t loadOffset2 = 0;
    const uint8_t* data1 = jsPandaFile1->GetPandaFile()->GetBase();
    const uint8_t* data2 = jsPandaFile2->GetPandaFile()->GetBase();
    uint64_t dataSize1 = jsPandaFile1->GetFileSize();
    uint64_t dataSize2 = jsPandaFile2->GetFileSize();
    uintptr_t extractorptr1 = 0;
    uintptr_t extractorptr2 = 0;
    JsFunction jsFunction1;
    JsFunction jsFunction2;

    auto ret = ark_create_js_symbol_extractor(&extractorptr1);
    EXPECT_TRUE(ret == 1);
    EXPECT_TRUE(extractorptr1 != 0);
    ret = ark_create_js_symbol_extractor(&extractorptr2);
    EXPECT_TRUE(ret == 1);
    EXPECT_TRUE(extractorptr2 != 0);

    ret = ark_parse_js_frame_info(byteCodePc1, mapBase1, loadOffset1,
        reinterpret_cast<uint8_t *>(const_cast<char*>(pfdata1)),
        strlen(pfdata1) + 1, extractorptr1, &jsFunction1);
    EXPECT_TRUE(ret == -1);

    ret = ark_parse_js_frame_info(byteCodePc1, mapBase1, loadOffset1,
        const_cast<uint8_t*>(data1), dataSize1, extractorptr1, &jsFunction1);
    EXPECT_TRUE(ret == 1);
    EXPECT_TRUE(std::string(jsFunction1.functionName) == "foo");

    ret = ark_parse_js_frame_info(byteCodePc1, mapBase1, loadOffset1,
        const_cast<uint8_t*>(data1), dataSize1 + 1, 0, &jsFunction1);
    EXPECT_TRUE(ret == -1);

    ret = ark_parse_js_frame_info(byteCodePc1, mapBase1, loadOffset1,
        nullptr, 0, extractorptr1, &jsFunction1);
    EXPECT_TRUE(ret == -1);

    ret = ark_parse_js_frame_info(byteCodePc1, mapBase1, loadOffset1,
        const_cast<uint8_t*>(data2), dataSize2, extractorptr2, &jsFunction1);
    EXPECT_TRUE(ret == -1);

    ret = ark_parse_js_frame_info(byteCodePc2, mapBase1, loadOffset1,
        const_cast<uint8_t*>(data2), dataSize2, extractorptr2, &jsFunction1);
    EXPECT_TRUE(ret == -1);

    ret = ark_parse_js_frame_info(byteCodePc2, mapBase1, loadOffset1,
        const_cast<uint8_t*>(data2), dataSize2, extractorptr2, &jsFunction1);
    EXPECT_TRUE(ret == -1);

    ret = ark_parse_js_frame_info(byteCodePc2, mapBase2, loadOffset2,
        const_cast<uint8_t*>(data2), dataSize2, extractorptr2, &jsFunction2);
    EXPECT_TRUE(ret == 1);
    EXPECT_TRUE(std::string(jsFunction2.functionName) == "func_main_0");

    ret = ark_destory_js_symbol_extractor(extractorptr1);
    EXPECT_TRUE(ret == 1);

    ret = ark_destory_js_symbol_extractor(extractorptr2);
    EXPECT_TRUE(ret == 1);
}

/**
 * @tc.name: BuildJsStackInfo
 * @tc.desc: Check whether the result returned through "BuildJsStackInfo" function is within expectations;
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsStackInfoTest, TestBuildJsStackInfo)
{
    auto jsFrame = JsStackInfo::BuildJsStackInfo(thread);
    EXPECT_TRUE(jsFrame.empty());
}

/**
 * @tc.name: ark_destory_local
 * @tc.desc: Check whether the result returned through "ark_destory_local" function is within expectations;
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsStackInfoTest, TestArkDestoryLocal)
{
    auto ret = ark_destroy_local(); // Direct destruct without creating Pointers
    EXPECT_TRUE(ret);
    ret = ark_create_local(); // ref count = 1
    ret = ark_create_local(); // ref count = 2
    EXPECT_TRUE(ret);
    auto trace = JSStackTrace::GetInstance();
    EXPECT_TRUE(trace != nullptr);
    ret = ark_destroy_local(); // ref count = 1
    trace = JSStackTrace::GetInstance();
    EXPECT_TRUE(trace != nullptr);
    ret = ark_destroy_local(); // ref count = 0
    trace = JSStackTrace::GetInstance();
    EXPECT_TRUE(trace == nullptr);
    EXPECT_TRUE(ret);
    ret = ark_destroy_local(); // multiple destory
    EXPECT_TRUE(ret);

    // Create and destruct multiple times within the process
    ret = ark_create_local(); // ref count = 1
    trace = JSStackTrace::GetInstance();
    EXPECT_TRUE(trace != nullptr);
    ret = ark_destroy_local();
    EXPECT_TRUE(ret);
}

HWTEST_F_L0(JsStackInfoTest, TestStepArk__001)
{
    void *ctx = nullptr;
    uintptr_t sp = 0;
    uintptr_t pc = 0;
    bool isJsFrame = true;
    uintptr_t frame[10][3];
    uintptr_t fp[10];
    for (int i = 0; i < 10; i++) {
        frame[i][0] = 0;
        frame[i][1] = 0;
    }
    fp[0] = reinterpret_cast<uintptr_t>(&frame[0][2]) + 8;
    for (int i = 1; i < 10; i++) {
        fp[i] = fp[i-1] + 24;
    }
    ArkStepParam arkStepParam{ &fp[0], &sp, &pc, &isJsFrame };
    frame[0][2] = static_cast<uintptr_t>(FrameType::INTERPRETER_CONSTRUCTOR_FRAME);
    ASSERT_TRUE(step_ark(ctx, ReadMemFunc, &arkStepParam));
    frame[1][2] = static_cast<uintptr_t>(FrameType::INTERPRETER_FAST_NEW_FRAME);
    arkStepParam.fp = &fp[1];
    ASSERT_TRUE(step_ark(ctx, ReadMemFunc, &arkStepParam));
    frame[2][2] = static_cast<uintptr_t>(FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME);
    arkStepParam.fp = &fp[2];
    ASSERT_TRUE(step_ark(ctx, ReadMemFunc, &arkStepParam));
    frame[3][2] = static_cast<uintptr_t>(FrameType::ASM_INTERPRETER_ENTRY_FRAME);
    arkStepParam.fp = &fp[3];
    ASSERT_TRUE(step_ark(ctx, ReadMemFunc, &arkStepParam));
    frame[4][2] = static_cast<uintptr_t>(FrameType::BUILTIN_ENTRY_FRAME);
    arkStepParam.fp = &fp[4];
    ASSERT_TRUE(step_ark(ctx, ReadMemFunc, &arkStepParam));
    frame[5][2] = static_cast<uintptr_t>(FrameType::BUILTIN_FRAME_WITH_ARGV_STACK_OVER_FLOW_FRAME);
    arkStepParam.fp = &fp[5];
    ASSERT_TRUE(step_ark(ctx, ReadMemFunc, &arkStepParam));
    frame[6][2] = static_cast<uintptr_t>(FrameType::BASELINE_BUILTIN_FRAME);
    arkStepParam.fp = &fp[6];
    ASSERT_TRUE(step_ark(ctx, ReadMemFunc, &arkStepParam));
    frame[7][2] = static_cast<uintptr_t>(FrameType::ASM_BRIDGE_FRAME);
    arkStepParam.fp = &fp[7];
    ASSERT_TRUE(step_ark(ctx, ReadMemFunc, &arkStepParam));
    frame[8][2] = static_cast<uintptr_t>(FrameType::LEAVE_FRAME);
    arkStepParam.fp = &fp[8];
    ASSERT_TRUE(step_ark(ctx, ReadMemFunc, &arkStepParam));
    frame[9][2] = static_cast<uintptr_t>(FrameType::LEAVE_FRAME_WITH_ARGV);
    arkStepParam.fp = &fp[9];
    ASSERT_TRUE(step_ark(ctx, ReadMemFunc, &arkStepParam));
}

HWTEST_F_L0(JsStackInfoTest, TestStepArk__002)
{
    void *ctx = nullptr;
    uintptr_t sp = 0;
    uintptr_t pc = 0;
    bool isJsFrame = true;
    uintptr_t frame[30][3];
    uintptr_t fp[30];
    for (int i = 0; i < 30; i++) {
        frame[i][0] = 0;
        frame[i][1] = 0;
    }
    fp[0] = reinterpret_cast<uintptr_t>(&frame[0][2]) + 8;
    for (int i = 1; i < 30; i++) {
        fp[i] = fp[i-1] + 24;
    }
    ArkStepParam arkStepParam{ &fp[0], &sp, &pc, &isJsFrame };
    frame[0][2] = static_cast<uintptr_t>(FrameType::BUILTIN_CALL_LEAVE_FRAME);
    ASSERT_TRUE(step_ark(ctx, ReadMemFunc, &arkStepParam));
    frame[1][2] = static_cast<uintptr_t>(FrameType::OPTIMIZED_FRAME);
    arkStepParam.fp = &fp[1];
    ASSERT_TRUE(step_ark(ctx, ReadMemFunc, &arkStepParam));
    frame[2][2] = static_cast<uintptr_t>(FrameType::ASM_INTERPRETER_BRIDGE_FRAME);
    arkStepParam.fp = &fp[2];
    ASSERT_TRUE(step_ark(ctx, ReadMemFunc, &arkStepParam));
    frame[3][2] = static_cast<uintptr_t>(FrameType::OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME);
    arkStepParam.fp = &fp[3];
    ASSERT_TRUE(step_ark(ctx, ReadMemFunc, &arkStepParam));
    frame[4][2] = static_cast<uintptr_t>(FrameType::INTERPRETER_CONSTRUCTOR_FRAME);
    arkStepParam.fp = &fp[4];
    ASSERT_TRUE(step_ark(ctx, ReadMemFunc, &arkStepParam));
    frame[5][2] = static_cast<uintptr_t>(FrameType::OPTIMIZED_ENTRY_FRAME);
    arkStepParam.fp = &fp[5];
    ASSERT_TRUE(step_ark(ctx, ReadMemFunc, &arkStepParam));
    frame[6][2] = static_cast<uintptr_t>(FrameType::ASM_BRIDGE_FRAME);
    arkStepParam.fp = &fp[6];
    ASSERT_TRUE(step_ark(ctx, ReadMemFunc, &arkStepParam));
    frame[7][2] = static_cast<uintptr_t>(FrameType::OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME);
    arkStepParam.fp = &fp[7];
    ASSERT_TRUE(step_ark(ctx, ReadMemFunc, &arkStepParam));
    frame[8][2] = static_cast<uintptr_t>(FrameType::OPTIMIZED_JS_FUNCTION_FRAME);
    arkStepParam.fp = &fp[8];
    ASSERT_TRUE(step_ark(ctx, ReadMemFunc, &arkStepParam));
    frame[9][2] = 100;
    arkStepParam.fp = &fp[9];
    ASSERT_TRUE(step_ark(ctx, ReadMemFunc, &arkStepParam));
}

HWTEST_F_L0(JsStackInfoTest, TestLocalParseJsFrameInfo__001)
{
    const char *filename = "__JsStackInfoTest1.pa";
    const char *pfdata = R"(
        .function void foo() {
            return
        }
    )";

    auto pfManager = JSPandaFileManager::GetInstance();

    pandasm::Parser parser1;
    auto res = parser1.Parse(pfdata);
    auto file = pandasm::AsmEmitter::Emit(res.Value());
    auto jsPandaFile = pfManager->NewJSPandaFile(file.release(), CString(filename));
    pfManager->AddJSPandaFile(jsPandaFile);
    EXPECT_TRUE(jsPandaFile != nullptr);
    auto methods = JSStackTrace::ReadAllMethodInfos(jsPandaFile);

    uintptr_t byteCodePc = methods[0].codeBegin;
    uintptr_t mapBase = reinterpret_cast<uintptr_t>(jsPandaFile->GetHeader());
    uintptr_t loadOffset = 0;
    JsFunction jsFunction;

    // Incorrect invocation demonstration
    auto ret = ark_parse_js_frame_info_local(byteCodePc, mapBase, loadOffset, &jsFunction);
    EXPECT_TRUE(ret == -1);

    // Correct invocation demonstration
    ark_create_local();
    ret = ark_parse_js_frame_info_local(byteCodePc, mapBase, loadOffset, &jsFunction);
    EXPECT_TRUE(ret == 1);

    ret = ark_parse_js_frame_info_local(byteCodePc, mapBase, loadOffset, &jsFunction);
    EXPECT_TRUE(ret == 1);

    ark_destroy_local();

    pfManager->RemoveJSPandaFile(jsPandaFile.get());
}

HWTEST_F_L0(JsStackInfoTest, TestLocalParseJsFrameInfo__002)
{
    const char *filename = "__JsStackInfoTest1.pa";
    const char *pfdata = R"(
        .function void foo() {
            return
        }
    )";

    auto pfManager = JSPandaFileManager::GetInstance();

    pandasm::Parser parser1;
    auto res = parser1.Parse(pfdata);
    auto file = pandasm::AsmEmitter::Emit(res.Value());
    auto jsPandaFile = pfManager->NewJSPandaFile(file.release(), CString(filename));
    EXPECT_TRUE(jsPandaFile != nullptr);
    auto methods = JSStackTrace::ReadAllMethodInfos(jsPandaFile);

    uintptr_t byteCodePc = methods[0].codeBegin;
    uintptr_t mapBase = reinterpret_cast<uintptr_t>(jsPandaFile->GetHeader());
    uintptr_t loadOffset = 0;
    JsFunction jsFunction;

    ark_create_local();
    auto ret = ark_parse_js_frame_info_local(byteCodePc, mapBase, loadOffset, &jsFunction);
    ark_destroy_local();
    // pandafile manager can't find jsPandaFile
    EXPECT_TRUE(ret == -1);
}
}  // namespace panda::test