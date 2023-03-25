/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <thread>

#include "ecmascript/ecma_vm.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/ts_types/tests/ts_type_test_helper.h"
#include "ecmascript/ts_types/ts_type_table_generator.h"

namespace panda::test {
using namespace panda::ecmascript;
using namespace panda::panda_file;
using namespace panda::pandasm;

class TSTypeTableGeneratorTest : public testing::Test {
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
        TestHelper::CreateEcmaVMWithScope(ecmaVm, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(ecmaVm, scope);
    }

    EcmaVM *ecmaVm {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(TSTypeTableGeneratorTest, TestTryGetModuleId)
{
    auto tsManager = ecmaVm->GetTSManager();
    tsManager->Initialize();
    TSTypeTableGenerator tableGenerator(tsManager);
    uint32_t primitiveTableId = tableGenerator.TryGetModuleId(TSTypeTable::PRIMITIVE_TABLE_NAME);
    uint32_t builtinTableId = tableGenerator.TryGetModuleId(TSTypeTable::BUILTINS_TABLE_NAME);
    uint32_t inferTableId = tableGenerator.TryGetModuleId(TSTypeTable::INFER_TABLE_NAME);
    uint32_t runtimeTableId = tableGenerator.TryGetModuleId(TSTypeTable::RUNTIME_TABLE_NAME);
    uint32_t firstUserModuleId = tableGenerator.TryGetModuleId("test");
    EXPECT_EQ(primitiveTableId, static_cast<uint32_t>(TSModuleTable::PRIMITIVE_TABLE_ID));
    EXPECT_EQ(builtinTableId, static_cast<uint32_t>(TSModuleTable::BUILTINS_TABLE_ID));
    EXPECT_EQ(inferTableId, static_cast<uint32_t>(TSModuleTable::INFER_TABLE_ID));
    EXPECT_EQ(runtimeTableId, static_cast<uint32_t>(TSModuleTable::RUNTIME_TABLE_ID));
    EXPECT_EQ(firstUserModuleId, 4U);
}

HWTEST_F_L0(TSTypeTableGeneratorTest, TestTryGetLocalId)
{
    auto tsManager = ecmaVm->GetTSManager();
    tsManager->Initialize();
    TSTypeTableGenerator tableGenerator(tsManager);
    JSHandle<TSTypeTable> table = ecmaVm->GetFactory()->NewTSTypeTable(0);
    table->SetNumberOfTypes(ecmaVm->GetJSThread());
    uint32_t localId = tableGenerator.TryGetLocalId(table);
    EXPECT_EQ(localId, 1U);
}

HWTEST_F_L0(TSTypeTableGeneratorTest, GetOrGenerateTSTypeTable)
{
    const char *source = R"(
        .language ECMAScript
        .record test {}
        .function void foo() {}
    )";
    pandasm::Parser parser;
    auto res = parser.Parse(source);
    EXPECT_EQ(parser.ShowError().err, Error::ErrorType::ERR_NONE);
    auto &program = res.Value();

    const std::string abcFileName("TSTypeTableTest.abc");
    TSTypeTestHelper::AddTypeSummary(program, {});
    TSTypeTestHelper::AddCommonJsField(program);
    std::map<std::string, size_t> *statp = nullptr;
    pandasm::AsmEmitter::PandaFileToPandaAsmMaps maps {};
    pandasm::AsmEmitter::PandaFileToPandaAsmMaps *mapsp = &maps;
    EXPECT_TRUE(pandasm::AsmEmitter::Emit(abcFileName, program, statp, mapsp, false));
    std::unique_ptr<const panda_file::File> pfPtr = panda_file::File::Open(abcFileName);
    EXPECT_NE(pfPtr.get(), nullptr);

    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    const CString fileName(abcFileName.c_str());
    const JSPandaFile *jsPandaFile = pfManager->NewJSPandaFile(pfPtr.release(), fileName);
    EXPECT_NE(jsPandaFile, nullptr);

    auto tsManager = ecmaVm->GetTSManager();
    tsManager->Initialize();
    TSTypeTableGenerator tableGenerator(tsManager);
    [[maybe_unused]] JSHandle<TSTypeTable> table = tableGenerator.GetOrGenerateTSTypeTable(jsPandaFile, "test", 0U);
    ASSERT(table->GetNumberOfTypes() == 0U);
}
}  // namespace panda::test
