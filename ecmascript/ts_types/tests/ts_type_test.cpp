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
class TSTypeTest : public testing::Test {
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

HWTEST_F_L0(TSTypeTest, UnionType)
{
    auto factory = ecmaVm->GetFactory();

    const uint32_t literalLength = 4;
    const uint32_t unionLength = 2;
    JSHandle<TaggedArray> literal = factory->NewTaggedArray(literalLength);
    literal->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeKind::UNION)));
    literal->Set(thread, 1, JSTaggedValue(unionLength));
    literal->Set(thread, 2, JSTaggedValue(1));
    literal->Set(thread, 3, JSTaggedValue(7));

    CVector<JSHandle<EcmaString>> recordImportModules {};
    TSTypeParser typeParser(ecmaVm, nullptr, recordImportModules);
    JSHandle<JSTaggedValue> type = typeParser.ParseType(literal);
    ASSERT_TRUE(type->IsTSUnionType());

    JSHandle<TSUnionType> unionType = JSHandle<TSUnionType>(type);
    ASSERT_TRUE(unionType->GetComponents().IsTaggedArray());

    JSHandle<TaggedArray> unionArray(thread, unionType->GetComponents());
    ASSERT_EQ(unionArray->GetLength(), unionLength);
    ASSERT_EQ(unionArray->Get(0).GetInt(), 1);
    ASSERT_EQ(unionArray->Get(1).GetInt(), 7);
}

HWTEST_F_L0(TSTypeTest, ImportType)
{
    auto factory = ecmaVm->GetFactory();
    TSManager *tsManager = ecmaVm->GetTSManager();

    JSHandle<TSTypeTable> exportTable = factory->NewTSTypeTable(3);
    JSHandle<TSTypeTable> importTable = factory->NewTSTypeTable(2);
    JSHandle<TSTypeTable> redirectImportTable = factory->NewTSTypeTable(3);
    GlobalTSTypeRef gt(0);

    /* Construct literal buffer of test_import.abc */
    const int ImportLiteralLength = 2;
    CString importFile = "test_import.abc";
    JSHandle<EcmaString> importFileHandle = factory->NewFromUtf8(importFile);
    /* A -> import User-Define type */
    CString importVarAndPathA = "#A#test_redirect_import";
    JSHandle<EcmaString> importStringA = factory->NewFromUtf8(importVarAndPathA);
    JSHandle<TaggedArray> importLiteralA = factory->NewTaggedArray(ImportLiteralLength);
    importLiteralA->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeKind::IMPORT)));
    importLiteralA->Set(thread, 1, importStringA);
    /* B -> import Primitive type */
    CString importVarAndPathB = "#B#test_redirect_import";
    JSHandle<EcmaString> importStringB = factory->NewFromUtf8(importVarAndPathB);
    JSHandle<TaggedArray> importLiteralB = factory->NewTaggedArray(ImportLiteralLength);
    importLiteralB->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeKind::IMPORT)));
    importLiteralB->Set(thread, 1, importStringB);
    /* C -> import Builtin type */
    CString importVarAndPathC = "#C#test_redirect_import";
    JSHandle<EcmaString> importStringC = factory->NewFromUtf8(importVarAndPathC);
    JSHandle<TaggedArray> importLiteralC = factory->NewTaggedArray(ImportLiteralLength);
    importLiteralC->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeKind::IMPORT)));
    importLiteralC->Set(thread, 1, importStringC);

    /* Parse the literal buffer */
    JSPandaFile *jsPandaFile = CreateJSPandaFile(importFile);
    CVector<JSHandle<EcmaString>> recordImportModules {};
    TSTypeParser importTypeParser(ecmaVm, jsPandaFile, recordImportModules);
    JSHandle<TSImportType> importTypeA =
        JSHandle<TSImportType>(importTypeParser.ParseType(importLiteralA));
    JSHandle<TSImportType> importTypeB =
        JSHandle<TSImportType>(importTypeParser.ParseType(importLiteralB));
    JSHandle<TSImportType> importTypeC =
        JSHandle<TSImportType>(importTypeParser.ParseType(importLiteralC));

    recordImportModules = importTypeParser.GetImportModules();
    CString importMdoule = ConvertToString(recordImportModules.back().GetTaggedValue());
    recordImportModules.pop_back();
    ASSERT_EQ(importMdoule, "test_redirect_import.abc");

    ASSERT_TRUE(importTypeA.GetTaggedValue().IsTSImportType());
    ASSERT_TRUE(importTypeB.GetTaggedValue().IsTSImportType());
    ASSERT_TRUE(importTypeC.GetTaggedValue().IsTSImportType());

    /* Construct type-table of test_import.abc */
    gt.SetModuleId(tsManager->GetNextModuleId());
    gt.SetLocalId(1);
    importTypeA->SetGT(gt);
    gt.SetLocalId(2);
    importTypeB->SetGT(gt);
    gt.SetLocalId(3);
    importTypeC->SetGT(gt);

    importTable->Set(thread, 0, JSTaggedValue(3));
    importTable->Set(thread, 1, JSHandle<JSTaggedValue>(importTypeA));
    importTable->Set(thread, 2, JSHandle<JSTaggedValue>(importTypeB));
    importTable->Set(thread, 3, JSHandle<JSTaggedValue>(importTypeC));
    /* GT of three import-types of test_import.abc */
    GlobalTSTypeRef importGTA = importTypeA->GetGT();
    GlobalTSTypeRef importGTB = importTypeB->GetGT();
    GlobalTSTypeRef importGTC = importTypeC->GetGT();
    /* Target GT should be default(0) since Link() haven't been executed */
    ASSERT_EQ(importTypeA->GetTargetGT().GetType(), 0ULL);
    ASSERT_EQ(importTypeB->GetTargetGT().GetType(), 0ULL);
    ASSERT_EQ(importTypeC->GetTargetGT().GetType(), 0ULL);

    tsManager->AddTypeTable(JSHandle<JSTaggedValue>(importTable), importFileHandle);

    /* Construct literal buffer of test_redirect_import.abc */
    const int redirectImportLiteralLength = 2;
    CString redirectImportFile = "test_redirect_import.abc";
    JSHandle<EcmaString> redirectImportFileHandle = factory->NewFromUtf8(redirectImportFile);
    /* A -> import User-Define type */
    CString redirectImportVarAndPathA = "#A#test";
    JSHandle<EcmaString> redirectImportStringA = factory->NewFromUtf8(redirectImportVarAndPathA);
    JSHandle<TaggedArray> redirectImportLiteralA = factory->NewTaggedArray(redirectImportLiteralLength);
    redirectImportLiteralA->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeKind::IMPORT)));
    redirectImportLiteralA->Set(thread, 1, redirectImportStringA);
    /* B -> import Primitive type */
    CString redirectImportVarAndPathB = "#B#test";
    JSHandle<EcmaString> redirectImportStringB = factory->NewFromUtf8(redirectImportVarAndPathB);
    JSHandle<TaggedArray> redirectImportLiteralB = factory->NewTaggedArray(redirectImportLiteralLength);
    redirectImportLiteralB->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeKind::IMPORT)));
    redirectImportLiteralB->Set(thread, 1, redirectImportStringB);
    /* C -> import Builtin type */
    CString redirectImportVarAndPathC = "#C#test";
    JSHandle<EcmaString> redirectImportStringC = factory->NewFromUtf8(redirectImportVarAndPathC);
    JSHandle<TaggedArray> redirectImportLiteralC = factory->NewTaggedArray(redirectImportLiteralLength);
    redirectImportLiteralC->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeKind::IMPORT)));
    redirectImportLiteralC->Set(thread, 1, redirectImportStringC);

    /* Parse the literal buffer */
    JSPandaFile *jsPandaFile1 = CreateJSPandaFile(redirectImportFile);
    TSTypeParser exportTypeParser(ecmaVm, jsPandaFile1, recordImportModules);
    JSHandle<TSImportType> redirectImportTypeA =
        JSHandle<TSImportType>(exportTypeParser.ParseType(redirectImportLiteralA));
    JSHandle<TSImportType> redirectImportTypeB =
        JSHandle<TSImportType>(exportTypeParser.ParseType(redirectImportLiteralB));
    JSHandle<TSImportType> redirectImportTypeC =
        JSHandle<TSImportType>(exportTypeParser.ParseType(redirectImportLiteralC));
    recordImportModules = exportTypeParser.GetImportModules();
    importMdoule = ConvertToString(recordImportModules.back().GetTaggedValue());
    recordImportModules.pop_back();
    ASSERT_EQ(importMdoule, "test.abc");
    ASSERT_TRUE(redirectImportTypeA.GetTaggedValue().IsTSImportType());
    ASSERT_TRUE(redirectImportTypeB.GetTaggedValue().IsTSImportType());
    ASSERT_TRUE(redirectImportTypeC.GetTaggedValue().IsTSImportType());
    gt.Clear();
    gt.SetModuleId(tsManager->GetNextModuleId());
    gt.SetLocalId(1);
    redirectImportTypeA->SetGT(gt);
    gt.SetLocalId(2);
    redirectImportTypeB->SetGT(gt);
    gt.SetLocalId(3);
    redirectImportTypeC->SetGT(gt);

    /* GT of three import-types of test_redirect_import.abc */
    GlobalTSTypeRef redirectImportGTA = redirectImportTypeA->GetGT();
    GlobalTSTypeRef redirectImportGTB = redirectImportTypeB->GetGT();
    GlobalTSTypeRef redirectImportGTC = redirectImportTypeC->GetGT();

    /* Construct export-table of test_redirect_import.abc */
    JSHandle<TaggedArray> redirectExportTableHandle = factory->NewTaggedArray(6);
    JSHandle<EcmaString> exportValA = factory->NewFromASCII("A");
    JSHandle<EcmaString> exportValB = factory->NewFromASCII("B");
    JSHandle<EcmaString> exportValC = factory->NewFromASCII("C");
    redirectExportTableHandle->Set(thread, 0, exportValA);
    redirectExportTableHandle->Set(thread, 1, JSTaggedValue(redirectImportGTA.GetType()));
    redirectExportTableHandle->Set(thread, 2, exportValB);
    redirectExportTableHandle->Set(thread, 3, JSTaggedValue(redirectImportGTB.GetType()));
    redirectExportTableHandle->Set(thread, 4, exportValC);
    redirectExportTableHandle->Set(thread, 5, JSTaggedValue(redirectImportGTC.GetType()));

    /* Construct type-table of test_redirect_import.abc */
    redirectImportTable->Set(thread, 0, JSTaggedValue(3));
    redirectImportTable->Set(thread, 1, JSHandle<JSTaggedValue>(redirectImportTypeA));
    redirectImportTable->Set(thread, 2, JSHandle<JSTaggedValue>(redirectImportTypeB));
    redirectImportTable->Set(thread, 3, JSHandle<JSTaggedValue>(redirectImportTypeC));
    redirectImportTable->Set(thread, redirectImportTable->GetLength() - 1, redirectExportTableHandle);
    /* Target GT should be default(0) since Link() haven't been executed */
    ASSERT_EQ(redirectImportTypeA->GetTargetGT().GetType(), 0ULL);
    ASSERT_EQ(redirectImportTypeB->GetTargetGT().GetType(), 0ULL);
    ASSERT_EQ(redirectImportTypeC->GetTargetGT().GetType(), 0ULL);

    tsManager->AddTypeTable(JSHandle<JSTaggedValue>(redirectImportTable), redirectImportFileHandle);

    /* Construct literal buffer of test.abc */
    /* Notice that there is only one literal buffer,
    since the other two types(Primitive and Builtin) have no literal buffer */
    const uint32_t literalLength = 4;
    const uint32_t unionLength = 2;
    CString fileName = "test.abc";
    JSHandle<EcmaString> fileNameHandle = factory->NewFromUtf8(fileName);

    JSHandle<TaggedArray> literal = factory->NewTaggedArray(literalLength);
    literal->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeKind::UNION)));
    literal->Set(thread, 1, JSTaggedValue(unionLength));
    literal->Set(thread, 2, JSTaggedValue(1));
    literal->Set(thread, 3, JSTaggedValue(7));

    /* Parse the literal buffer */
    JSHandle<JSTaggedValue> type = importTypeParser.ParseType(literal);
    ASSERT_TRUE(type->IsTSUnionType());
    JSHandle<TSUnionType> unionType = JSHandle<TSUnionType>(type);

    gt.Clear();
    gt.SetModuleId(tsManager->GetNextModuleId());
    gt.SetLocalId(1);
    /* Construct export-table of test.abc */
    JSHandle<TaggedArray> exportValueTableHandle = factory->NewTaggedArray(6);
    exportValueTableHandle->Set(thread, 0, exportValA);
    exportValueTableHandle->Set(thread, 1, JSTaggedValue(gt.GetType()));
    /* Manually set export-data of the Primitive export type(B) and the Builtin export Type(C) */
    GlobalTSTypeRef gtB = GlobalTSTypeRef();
    gtB.SetModuleId(0);
    gtB.SetLocalId(3);
    exportValueTableHandle->Set(thread, 2, exportValB);
    exportValueTableHandle->Set(thread, 3, JSTaggedValue(gtB.GetType()));
    GlobalTSTypeRef gtC = GlobalTSTypeRef();
    gtC.SetModuleId(1);
    gtC.SetLocalId(21);
    exportValueTableHandle->Set(thread, 4, exportValC);
    exportValueTableHandle->Set(thread, 5, JSTaggedValue(gtC.GetType()));

    /* Construct type-table of test.abc */
    unionType->SetGT(gt);
    exportTable->Set(thread, 0, JSTaggedValue(1));
    exportTable->Set(thread, 1, JSHandle<JSTaggedValue>(unionType));
    exportTable->Set(thread, exportTable->GetLength() - 1, exportValueTableHandle);
    GlobalTSTypeRef unionTypeGT = unionType->GetGT();

    tsManager->AddTypeTable(JSHandle<JSTaggedValue>(exportTable), fileNameHandle);

    tsManager->Link();
    GlobalTSTypeRef linkimportGTA = tsManager->GetImportTypeTargetGT(importGTA);
    GlobalTSTypeRef linkredirectImportGTA = tsManager->GetImportTypeTargetGT(redirectImportGTA);
    GlobalTSTypeRef linkimportGTB = tsManager->GetImportTypeTargetGT(importGTB);
    GlobalTSTypeRef linkredirectImportGTB = tsManager->GetImportTypeTargetGT(redirectImportGTB);
    GlobalTSTypeRef linkimportGTC = tsManager->GetImportTypeTargetGT(importGTC);
    GlobalTSTypeRef linkredirectImportGTC = tsManager->GetImportTypeTargetGT(redirectImportGTC);
    ASSERT_EQ(linkimportGTA.GetType(), unionTypeGT.GetType());
    ASSERT_EQ(linkredirectImportGTA.GetType(), unionTypeGT.GetType());
    ASSERT_EQ(linkimportGTB.GetType(), gtB.GetType());
    ASSERT_EQ(linkredirectImportGTB.GetType(), gtB.GetType());
    ASSERT_EQ(linkimportGTC.GetType(), gtC.GetType());
    ASSERT_EQ(linkredirectImportGTC.GetType(), gtC.GetType());

    uint32_t length = tsManager->GetUnionTypeLength(unionTypeGT);
    ASSERT_EQ(length, unionLength);
    JSPandaFileManager::RemoveJSPandaFile(jsPandaFile);
    JSPandaFileManager::RemoveJSPandaFile(jsPandaFile1);
}

HWTEST_F_L0(TSTypeTest, InterfaceType)
{
    auto factory = ecmaVm->GetFactory();

    const int literalLength = 12;
    const uint32_t propsNum = 2;
    JSHandle<TaggedArray> literal = factory->NewTaggedArray(literalLength);
    JSHandle<EcmaString> propsNameA = factory->NewFromASCII("propsA");
    JSHandle<EcmaString> propsNameB = factory->NewFromASCII("propsB");
    literal->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeKind::INTERFACE_KIND)));
    literal->Set(thread, 1, JSTaggedValue(0));
    literal->Set(thread, 2, JSTaggedValue(propsNum));
    literal->Set(thread, 3, propsNameA.GetTaggedValue());
    literal->Set(thread, 4, JSTaggedValue(static_cast<int>(TSPrimitiveType::STRING)));
    literal->Set(thread, 5, JSTaggedValue(0));
    literal->Set(thread, 6, JSTaggedValue(0));
    literal->Set(thread, 7, propsNameB.GetTaggedValue());
    literal->Set(thread, 8, JSTaggedValue(static_cast<int>(TSPrimitiveType::NUMBER)));
    literal->Set(thread, 9, JSTaggedValue(0));
    literal->Set(thread, 10, JSTaggedValue(0));
    literal->Set(thread, 11, JSTaggedValue(0));

    CVector<JSHandle<EcmaString>> recordImportModules {};
    TSTypeParser typeParser(ecmaVm, nullptr, recordImportModules);
    JSHandle<JSTaggedValue> type = typeParser.ParseType(literal);
    ASSERT_TRUE(type->IsTSInterfaceType());
    JSHandle<TSInterfaceType> interfaceType = JSHandle<TSInterfaceType>(type);
    JSHandle<TSObjectType> fields =  JSHandle<TSObjectType>(thread, interfaceType->GetFields());
    TSObjLayoutInfo *fieldsTypeInfo = TSObjLayoutInfo::Cast(fields->GetObjLayoutInfo().GetTaggedObject());

    ASSERT_EQ(fieldsTypeInfo->GetPropertiesCapacity(), propsNum);
    ASSERT_EQ(fieldsTypeInfo->GetKey(0), propsNameA.GetTaggedValue());
    ASSERT_EQ(fieldsTypeInfo->GetTypeId(0).GetInt(), static_cast<int>(TSPrimitiveType::STRING));
    ASSERT_EQ(fieldsTypeInfo->GetKey(1), propsNameB.GetTaggedValue());
    ASSERT_EQ(fieldsTypeInfo->GetTypeId(1).GetInt(), static_cast<int>(TSPrimitiveType::NUMBER));
}

HWTEST_F_L0(TSTypeTest, ClassType)
{
    auto factory = ecmaVm->GetFactory();

    const int literalLength = 18;
    const uint32_t propsNum = 2;
    JSHandle<TaggedArray> literal = factory->NewTaggedArray(literalLength);
    JSHandle<EcmaString> propsNameA = factory->NewFromASCII("propsA");
    JSHandle<EcmaString> propsNameB = factory->NewFromASCII("propsB");
    JSHandle<EcmaString> funcName = factory->NewFromASCII("constructor");
    literal->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeKind::CLASS)));
    literal->Set(thread, 1, JSTaggedValue(0));
    literal->Set(thread, 2, JSTaggedValue(0));
    literal->Set(thread, 3, JSTaggedValue(0));
    literal->Set(thread, 4, JSTaggedValue(propsNum));
    literal->Set(thread, 5, propsNameA.GetTaggedValue());
    literal->Set(thread, 6, JSTaggedValue(static_cast<int>(TSPrimitiveType::STRING)));
    literal->Set(thread, 7, JSTaggedValue(0));
    literal->Set(thread, 8, JSTaggedValue(0));
    literal->Set(thread, 9, propsNameB.GetTaggedValue());
    literal->Set(thread, 10, JSTaggedValue(static_cast<int>(TSPrimitiveType::NUMBER)));
    literal->Set(thread, 11, JSTaggedValue(0));
    literal->Set(thread, 12, JSTaggedValue(0));
    literal->Set(thread, 13, JSTaggedValue(1));
    literal->Set(thread, 14, funcName.GetTaggedValue());
    literal->Set(thread, 15, JSTaggedValue(static_cast<int>(TSTypeKind::FUNCTION)));
    literal->Set(thread, 16, JSTaggedValue(0));
    literal->Set(thread, 17, JSTaggedValue(0));

    CVector<JSHandle<EcmaString>> recordImportModules {};
    TSTypeParser typeParser(ecmaVm, nullptr, recordImportModules);
    JSHandle<JSTaggedValue> type = typeParser.ParseType(literal);
    ASSERT_TRUE(type->IsTSClassType());
    JSHandle<TSClassType> classType = JSHandle<TSClassType>(type);
    JSHandle<TSObjectType> fields =  JSHandle<TSObjectType>(thread, classType->GetInstanceType());
    TSObjLayoutInfo *fieldsTypeInfo = TSObjLayoutInfo::Cast(fields->GetObjLayoutInfo().GetTaggedObject());

    ASSERT_EQ(fieldsTypeInfo->GetPropertiesCapacity(), propsNum);
    ASSERT_EQ(fieldsTypeInfo->GetKey(0), propsNameA.GetTaggedValue());
    ASSERT_EQ(fieldsTypeInfo->GetTypeId(0).GetInt(), static_cast<int>(TSPrimitiveType::STRING));
    ASSERT_EQ(fieldsTypeInfo->GetKey(1), propsNameB.GetTaggedValue());
    ASSERT_EQ(fieldsTypeInfo->GetTypeId(1).GetInt(), static_cast<int>(TSPrimitiveType::NUMBER));
}

HWTEST_F_L0(TSTypeTest, ClassInstanceType)
{
    auto factory = ecmaVm->GetFactory();
    JSHandle<TSClassType> classType = factory->NewTSClassType();
    classType->SetGT(GlobalTSTypeRef(1));
    const int literalLength = 2;
    JSHandle<TaggedArray> literal = factory->NewTaggedArray(literalLength);

    literal->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeKind::CLASS_INSTANCE)));
    literal->Set(thread, 1, JSTaggedValue(1));

    CVector<JSHandle<EcmaString>> recordImportModules {};
    TSTypeParser typeParser(ecmaVm, nullptr, recordImportModules);
    JSHandle<JSTaggedValue> type = typeParser.ParseType(literal);
    ASSERT_TRUE(type->IsTSClassInstanceType());
    JSHandle<TSClassInstanceType> classInstanceType = JSHandle<TSClassInstanceType>(type);

    ASSERT_EQ(classInstanceType->GetClassGT().GetType(), classType->GetGT().GetType());
}

HWTEST_F_L0(TSTypeTest, FuntionType)
{
    auto factory = ecmaVm->GetFactory();

    const uint32_t literalLength = 8;
    const uint32_t parameterLength = 2;
    const uint32_t bitFiled = 8;
    const uint32_t thisFlag = 0;
    JSHandle<TaggedArray> literal = factory->NewTaggedArray(literalLength);
    JSHandle<EcmaString> functionName = factory->NewFromASCII("testFunction");
    literal->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeKind::FUNCTION)));
    literal->Set(thread, 1, JSTaggedValue(bitFiled));
    literal->Set(thread, 2, functionName);
    literal->Set(thread, 3, JSTaggedValue(thisFlag));
    literal->Set(thread, 4, JSTaggedValue(parameterLength));
    literal->Set(thread, 5, JSTaggedValue(1));
    literal->Set(thread, 6, JSTaggedValue(2));
    literal->Set(thread, 7, JSTaggedValue(6));

    CVector<JSHandle<EcmaString>> recordImportModules {};
    TSTypeParser typeParser(ecmaVm, nullptr, recordImportModules);
    JSHandle<JSTaggedValue> type = typeParser.ParseType(literal);
    ASSERT_TRUE(type->IsTSFunctionType());

    JSHandle<TSFunctionType> functionType = JSHandle<TSFunctionType>(type);
    ASSERT_TRUE(functionType->GetParameterTypes().IsTaggedArray());
    ASSERT_EQ(functionType->GetLength(), parameterLength);
    ASSERT_EQ(functionType->GetParameterTypeGT(0).GetType(), 1ULL);
    ASSERT_EQ(functionType->GetParameterTypeGT(1).GetType(), 2ULL);
    ASSERT_EQ(functionType->GetReturnGT().GetType(), 6ULL);
    ASSERT_TRUE(functionType->GetAsync());
}

HWTEST_F_L0(TSTypeTest, ObjectType)
{
    auto factory = ecmaVm->GetFactory();

    const int literalLength = 6;
    const uint32_t propsNum = 2;
    JSHandle<TaggedArray> literal = factory->NewTaggedArray(literalLength);
    JSHandle<EcmaString> propsNameA = factory->NewFromASCII("1");
    JSHandle<EcmaString> propsNameB = factory->NewFromASCII("name");
    literal->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeKind::OBJECT)));
    literal->Set(thread, 1, JSTaggedValue(propsNum));
    literal->Set(thread, 2, propsNameA.GetTaggedValue());
    literal->Set(thread, 3, JSTaggedValue(static_cast<int>(TSPrimitiveType::STRING)));
    literal->Set(thread, 4, propsNameB.GetTaggedValue());
    literal->Set(thread, 5, JSTaggedValue(static_cast<int>(TSPrimitiveType::STRING)));

    CVector<JSHandle<EcmaString>> recordImportModules {};
    TSTypeParser typeParser(ecmaVm, nullptr, recordImportModules);
    JSHandle<JSTaggedValue> type = typeParser.ParseType(literal);
    ASSERT_TRUE(type->IsTSObjectType());
    JSHandle<TSObjectType> objectType = JSHandle<TSObjectType>(type);
    TSObjLayoutInfo *fieldsTypeInfo = TSObjLayoutInfo::Cast(objectType->GetObjLayoutInfo().GetTaggedObject());

    ASSERT_EQ(fieldsTypeInfo->GetPropertiesCapacity(), propsNum);
    ASSERT_EQ(fieldsTypeInfo->GetKey(0), propsNameA.GetTaggedValue());
    ASSERT_EQ(fieldsTypeInfo->GetTypeId(0).GetInt(), static_cast<int>(TSPrimitiveType::STRING));
    ASSERT_EQ(fieldsTypeInfo->GetKey(1), propsNameB.GetTaggedValue());
    ASSERT_EQ(fieldsTypeInfo->GetTypeId(1).GetInt(), static_cast<int>(TSPrimitiveType::STRING));
}

HWTEST_F_L0(TSTypeTest, ArrayType)
{
    auto factory = ecmaVm->GetFactory();

    const int literalLength = 2;
    JSHandle<TaggedArray> literal = factory->NewTaggedArray(literalLength);

    literal->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeKind::ARRAY)));
    literal->Set(thread, 1, JSTaggedValue(2));

    CVector<JSHandle<EcmaString>> recordImportModules {};
    TSTypeParser typeParser(ecmaVm, nullptr, recordImportModules);
    JSHandle<JSTaggedValue> type = typeParser.ParseType(literal);
    ASSERT_TRUE(type->IsTSArrayType());
    JSHandle<TSArrayType> arrayType = JSHandle<TSArrayType>(type);

    ASSERT_EQ(arrayType->GetElementGT().GetType(), 2ULL);
}
} // namespace panda::test
