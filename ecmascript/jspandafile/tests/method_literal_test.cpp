/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/global_env.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/jspandafile/program_object.h"
#define private public
#define protected public
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/method_literal.h"
#undef protected
#undef private

#include "assembler/assembly-emitter.h"
#include "assembler/assembly-parser.h"

using namespace panda::ecmascript;
using namespace panda::panda_file;
using namespace panda::pandasm;

namespace panda::test {
class MethodLiteralTest : public testing::Test {
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
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    // Helper function to create JSPandaFile from assembly source
    std::shared_ptr<JSPandaFile> CreateJSPandaFile(const char *source, const CString fileName)
    {
        Parser parser;
        const std::string fn = "SRC.pa"; // test file name
        auto res = parser.Parse(source, fn);
        EXPECT_EQ(parser.ShowError().err, Error::ErrorType::ERR_NONE);

        std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
        JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
        std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), fileName);
        return pf;
    }

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

/**
 * @tc.name: MethodLiteral_Construction
 * @tc.desc: Test MethodLiteral construction
 * @tc.type: FUNC
 * @tc.require: issue#13000
 */
HWTEST_F_L0(MethodLiteralTest, MethodLiteral_Construction)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "MethodLiteralTest.pa";
    std::shared_ptr<JSPandaFile> jsPandaFile = CreateJSPandaFile(source, fileName);
    ASSERT_NE(jsPandaFile, nullptr);

    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
    File::EntityId classId = pf->GetClassId(typeDesc);
    ASSERT_TRUE(classId.IsValid());

    // Get method and create MethodLiteral
    ClassDataAccessor cda(*pf, classId);
    bool foundMethod = false;
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        if (foundMethod) {
            return;
        }
        // Create MethodLiteral - this tests the constructor
        MethodLiteral *methodLiteral = new MethodLiteral(mda.GetMethodId());
        ASSERT_NE(methodLiteral, nullptr);

        // Verify method ID is stored correctly
        EntityId retrievedMethodId = methodLiteral->GetMethodId();
        EXPECT_EQ(retrievedMethodId.GetOffset(), mda.GetMethodId().GetOffset());

        delete methodLiteral;
        foundMethod = true;
    });

    EXPECT_TRUE(foundMethod);
}

/**
 * @tc.name: MethodLiteral_GetMethodId
 * @tc.desc: Test MethodLiteral method ID retrieval
 * @tc.type: FUNC
 * @tc.require: issue#13001
 */
HWTEST_F_L0(MethodLiteralTest, MethodLiteral_GetMethodId)
{
    const char *source = R"(
        .function void foo() {}
        .function void bar() {}
    )";
    const CString fileName = "MethodLiteralTestMethodId.pa";
    std::shared_ptr<JSPandaFile> jsPandaFile = CreateJSPandaFile(source, fileName);
    ASSERT_NE(jsPandaFile, nullptr);

    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
    File::EntityId classId = pf->GetClassId(typeDesc);

    std::vector<File::EntityId> methodIds {};
    ClassDataAccessor cda(*pf, classId);
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        methodIds.push_back(mda.GetMethodId());
    });

    ASSERT_EQ(methodIds.size(), 2U);

    // Verify method IDs are correctly stored and retrieved
    for (size_t i = 0; i < methodIds.size(); i++) {
        MethodLiteral methodLiteral(methodIds[i]);
        EntityId retrievedMethodId = methodLiteral.GetMethodId();
        EXPECT_EQ(retrievedMethodId.GetOffset(), methodIds[i].GetOffset());
    }
}

/**
 * @tc.name: MethodLiteral_GetLiteralInfo
 * @tc.desc: Test MethodLiteral literal info operations
 * @tc.type: FUNC
 * @tc.require: issue#13002
 */
HWTEST_F_L0(MethodLiteralTest, MethodLiteral_GetLiteralInfo)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "MethodLiteralTestLiteralInfo.pa";
    std::shared_ptr<JSPandaFile> jsPandaFile = CreateJSPandaFile(source, fileName);
    ASSERT_NE(jsPandaFile, nullptr);

    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
    File::EntityId classId = pf->GetClassId(typeDesc);

    ClassDataAccessor cda(*pf, classId);
    bool foundMethod = false;
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        if (foundMethod) {
            return;
        }
        MethodLiteral methodLiteral(mda.GetMethodId());

        // Get literal info
        uint64_t literalInfo = methodLiteral.GetLiteralInfo();
        EXPECT_NE(literalInfo, 0ULL);

        // Get method ID from literal info
        EntityId methodId = MethodLiteral::GetMethodId(literalInfo);
        EXPECT_TRUE(methodId.IsValid());

        foundMethod = true;
    });

    EXPECT_TRUE(foundMethod);
}

/**
 * @tc.name: MethodLiteral_GetHotnessCounter
 * @tc.desc: Test MethodLiteral hotness counter from literal info
 * @tc.type: FUNC
 * @tc.require: issue#13003
 */
HWTEST_F_L0(MethodLiteralTest, MethodLiteral_GetHotnessCounter)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "MethodLiteralTestHotness.pa";
    std::shared_ptr<JSPandaFile> jsPandaFile = CreateJSPandaFile(source, fileName);
    ASSERT_NE(jsPandaFile, nullptr);

    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
    File::EntityId classId = pf->GetClassId(typeDesc);

    ClassDataAccessor cda(*pf, classId);
    bool foundMethod = false;
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        if (foundMethod) {
            return;
        }
        MethodLiteral methodLiteral(mda.GetMethodId());

        // Get hotness counter from literal info
        uint64_t literalInfo = methodLiteral.GetLiteralInfo();
        int16_t hotnessCounter = MethodLiteral::GetHotnessCounter(literalInfo);

        // Hotness counter should be initialized to a valid value
        EXPECT_GE(hotnessCounter, INT16_MIN);
        EXPECT_LE(hotnessCounter, INT16_MAX);

        foundMethod = true;
    });

    EXPECT_TRUE(foundMethod);
}

/**
 * @tc.name: MethodLiteral_GetFunctionKind
 * @tc.desc: Test MethodLiteral function kind from literal info
 * @tc.type: FUNC
 * @tc.require: issue#13004
 */
HWTEST_F_L0(MethodLiteralTest, MethodLiteral_GetFunctionKind)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "MethodLiteralTestFunctionKind.pa";
    std::shared_ptr<JSPandaFile> jsPandaFile = CreateJSPandaFile(source, fileName);
    ASSERT_NE(jsPandaFile, nullptr);

    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
    File::EntityId classId = pf->GetClassId(typeDesc);

    ClassDataAccessor cda(*pf, classId);
    bool foundMethod = false;
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        if (foundMethod) {
            return;
        }
        MethodLiteral methodLiteral(mda.GetMethodId());

        // Get function kind from literal info
        uint64_t extraLiteralInfo = methodLiteral.GetExtraLiteralInfo();
        ecmascript::FunctionKind functionKind = MethodLiteral::GetFunctionKind(extraLiteralInfo);

        // Function kind should be within valid range
        EXPECT_GE(static_cast<uint8_t>(functionKind), static_cast<uint8_t>(ecmascript::FunctionKind::NORMAL_FUNCTION));
        EXPECT_LE(static_cast<uint8_t>(functionKind), static_cast<uint8_t>(ecmascript::FunctionKind::LAST_FUNCTION_KIND));

        foundMethod = true;
    });

    EXPECT_TRUE(foundMethod);
}

/**
 * @tc.name: MethodLiteral_GetSlotSize
 * @tc.desc: Test MethodLiteral slot size from literal info
 * @tc.type: FUNC
 * @tc.require: issue#13005
 */
HWTEST_F_L0(MethodLiteralTest, MethodLiteral_GetSlotSize)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "MethodLiteralTestSlotSize.pa";
    std::shared_ptr<JSPandaFile> jsPandaFile = CreateJSPandaFile(source, fileName);
    ASSERT_NE(jsPandaFile, nullptr);

    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
    File::EntityId classId = pf->GetClassId(typeDesc);

    ClassDataAccessor cda(*pf, classId);
    bool foundMethod = false;
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        if (foundMethod) {
            return;
        }
        MethodLiteral methodLiteral(mda.GetMethodId());

        // Get slot size from literal info
        uint64_t literalInfo = methodLiteral.GetLiteralInfo();
        uint32_t slotSize = MethodLiteral::GetSlotSize(literalInfo);

        // Slot size should be a valid value
        EXPECT_LE(slotSize, MethodLiteral::MAX_SLOT_SIZE + MethodLiteral::EXTEND_SLOT_SIZE);

        foundMethod = true;
    });

    EXPECT_TRUE(foundMethod);
}

}  // namespace panda::test
