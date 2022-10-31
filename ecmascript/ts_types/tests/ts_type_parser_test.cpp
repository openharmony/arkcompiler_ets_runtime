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

#include <thread>

#include "assembler/assembly-emitter.h"
#include "assembler/assembly-parser.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/ts_types/ts_obj_layout_info.h"
#include "ecmascript/ts_types/ts_type_parser.h"
#include "ecmascript/ts_types/ts_type_table.h"

using namespace panda::ecmascript;
using namespace panda::panda_file;
using namespace panda::pandasm;

namespace panda::test {
class TSTypeParserTest : public testing::Test {
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

    EcmaVM *ecmaVm = nullptr;
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};

protected:
    JSPandaFile *CreateJSPandaFile(const CString filename)
    {
        const char *source = R"(
            .function void foo() {}
        )";
        Parser parser;
        const std::string fn = "SRC.pa"; // test file name : "SRC.pa"
        auto res = parser.Parse(source, fn);
        EXPECT_EQ(parser.ShowError().err, Error::ErrorType::ERR_NONE);

        std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
        JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
        JSPandaFile *pf = pfManager->NewJSPandaFile(pfPtr.release(), filename);
        return pf;
    }
};

HWTEST_F_L0(TSTypeParserTest, ParseType)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    const CString fileName = "__TSTypeParserTest.pa";
    JSPandaFile *pf = CreateJSPandaFile(fileName);
    CVector<JSHandle<EcmaString>> recordImportModules {};
    TSTypeParser tsTypeParser(vm, pf, recordImportModules);
    JSHandle<TaggedArray> literal = factory->NewTaggedArray(8);
    literal->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeKind::INTERFACE_KIND)));
    literal->Set(thread, 1, JSTaggedValue(0));
    literal->Set(thread, 2, JSTaggedValue(0));
    literal->Set(thread, 3, JSTaggedValue(0));
    literal->Set(thread, 4, JSTaggedValue(0));
    literal->Set(thread, 5, JSTaggedValue(0));
    literal->Set(thread, 6, JSTaggedValue(0));
    literal->Set(thread, 7, JSTaggedValue(0));

    JSHandle<JSTaggedValue> type = tsTypeParser.ParseType(literal);
    EXPECT_TRUE(type->IsTSInterfaceType());

    literal->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeKind::ARRAY)));
    type = tsTypeParser.ParseType(literal);
    EXPECT_TRUE(type->IsTSArrayType());

    literal->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeKind::CLASS)));
    type = tsTypeParser.ParseType(literal);
    EXPECT_TRUE(type->IsTSClassType());

    literal->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeKind::CLASS_INSTANCE)));
    type = tsTypeParser.ParseType(literal);
    EXPECT_TRUE(type->IsTSClassInstanceType());

    literal->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeKind::FUNCTION)));
    type = tsTypeParser.ParseType(literal);
    EXPECT_TRUE(type->IsTSFunctionType());

    CString importVarAndPath = "#TSTypeParserTest#Test";
    JSHandle<EcmaString> importString = factory->NewFromUtf8(importVarAndPath);
    JSHandle<JSTaggedValue> importStringVal = JSHandle<JSTaggedValue>::Cast(importString);
    literal->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeKind::IMPORT)));
    literal->Set(thread, 1, importStringVal.GetTaggedValue());
    type = tsTypeParser.ParseType(literal);
    EXPECT_TRUE(type->IsTSImportType());

    literal->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeKind::OBJECT)));
    literal->Set(thread, 1, JSTaggedValue(0));
    type = tsTypeParser.ParseType(literal);
    EXPECT_TRUE(type->IsTSObjectType());

    literal->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeKind::UNION)));
    literal->Set(thread, 1, JSTaggedValue(3)); // 3 : num of union members
    type = tsTypeParser.ParseType(literal);
    EXPECT_TRUE(type->IsTSUnionType());
}

HWTEST_F_L0(TSTypeParserTest, SetTypeGT)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    uint32_t moduleId = 0;
    uint32_t localId = 0;
    const CString fileName = "__TSTypeParserTest.pa";
    JSPandaFile *pf = CreateJSPandaFile(fileName);
    CVector<JSHandle<EcmaString>> recordImportModules {};
    TSTypeParser tsTypeParser(vm, pf, recordImportModules);
    JSHandle<TaggedArray> literal = factory->NewTaggedArray(3);
    literal->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeKind::ARRAY)));
    literal->Set(thread, 1, JSTaggedValue(0));
    literal->Set(thread, 2, JSTaggedValue(0));
    JSHandle<JSTaggedValue> type = tsTypeParser.ParseType(literal);
    tsTypeParser.SetTypeGT(type, moduleId, localId);
    GlobalTSTypeRef resultGt = JSHandle<TSType>(type)->GetGT();
    GlobalTSTypeRef expectGt = GlobalTSTypeRef(moduleId, localId);
    EXPECT_EQ(resultGt, expectGt);
}

HWTEST_F_L0(TSTypeParserTest, CreateGT)
{
    auto vm = thread->GetEcmaVM();
    auto tsManager = vm->GetTSManager();
    const CString fileName = "__TSTypeParserTest.pa";
    JSPandaFile *pf = CreateJSPandaFile(fileName);
    CVector<JSHandle<EcmaString>> recordImportModules {};
    TSTypeParser tsTypeParser(vm, pf, recordImportModules);
    uint32_t typeId = TSTypeParser::BUILDIN_TYPE_OFFSET;
    GlobalTSTypeRef resultGT = tsTypeParser.CreateGT(typeId);
    EXPECT_EQ(resultGT, GlobalTSTypeRef(TSModuleTable::PRIMITIVE_TABLE_ID, typeId));

    typeId = TSTypeParser::USER_DEFINED_TYPE_OFFSET;
    resultGT = tsTypeParser.CreateGT(typeId);
    EXPECT_EQ(resultGT, GlobalTSTypeRef(TSModuleTable::BUILTINS_TABLE_ID, typeId - TSTypeParser::BUILDIN_TYPE_OFFSET));

    GlobalTSTypeRef offsetGt(TSModuleTable::INFER_TABLE_ID, typeId - TSTypeParser::BUILDIN_TYPE_OFFSET + 1);
    typeId = TSTypeParser::USER_DEFINED_TYPE_OFFSET + 1;
    panda_file::File::EntityId offset(typeId);
    tsManager->AddElementToLiteralOffsetGTMap(pf, offset, offsetGt);
    resultGT = tsTypeParser.CreateGT(typeId);
    EXPECT_EQ(resultGT, tsManager->GetGTFromOffset(pf, offset));
}

HWTEST_F_L0(TSTypeParserTest, GetImportModules)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    const CString fileName = "__TSTypeParserTest.pa";
    JSPandaFile *pf = CreateJSPandaFile(fileName);
    CVector<JSHandle<EcmaString>> recordImportModules {};
    TSTypeParser tsTypeParser(vm, pf, recordImportModules);
    JSHandle<EcmaString> importString = factory->NewFromUtf8("#A#TSTypeParserTest");
    JSHandle<TaggedArray> literal = factory->NewTaggedArray(2);
    literal->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeKind::IMPORT)));
    literal->Set(thread, 1, importString);
    tsTypeParser.ParseType(literal);
    CVector<JSHandle<EcmaString>> result = tsTypeParser.GetImportModules();
    CString importMdoule = ConvertToString(result.back().GetTaggedValue());
    EXPECT_STREQ(importMdoule.c_str(), "TSTypeParserTest.abc");
}
} // namespace panda::test
