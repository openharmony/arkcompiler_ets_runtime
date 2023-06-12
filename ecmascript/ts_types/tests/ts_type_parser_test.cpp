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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/ts_types/tests/ts_type_test_helper.h"
#include "ecmascript/ts_types/ts_type_parser.h"

namespace panda::test {
using namespace panda::ecmascript;
using namespace panda::panda_file;
using namespace panda::pandasm;
using LiteralValueType = std::variant<uint8_t, uint32_t, std::string>;

static constexpr uint16_t FIRST_USER_DEFINE_MODULE_ID = static_cast<uint32_t>(ModuleTableIdx::NUM_OF_DEFAULT_TABLES);
static constexpr uint16_t FIRST_USER_DEFINE_LOCAL_ID = 1;

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

    EcmaVM *ecmaVm {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(TSTypeParserTest, TestPrimetiveType)
{
    auto tsManager = ecmaVm->GetJSThread()->GetCurrentEcmaContext()->GetTSManager();
    tsManager->Initialize();
    TSTypeParser tsTypeParser(tsManager);
    JSPandaFile *jsPandaFile = nullptr;
    const CString recordName("");
    uint32_t primetiveTypeId = 0U;
    GlobalTSTypeRef resultGT = tsTypeParser.CreateGT(jsPandaFile, recordName, primetiveTypeId);
    EXPECT_EQ(resultGT, GlobalTSTypeRef(static_cast<uint32_t>(ModuleTableIdx::PRIMITIVE), primetiveTypeId));
}

HWTEST_F_L0(TSTypeParserTest, TestBuiltinType)
{
    auto tsManager = ecmaVm->GetJSThread()->GetCurrentEcmaContext()->GetTSManager();
    tsManager->Initialize();
    TSTypeParser tsTypeParser(tsManager);
    JSPandaFile *jsPandaFile = nullptr;
    const CString recordName("");
    uint32_t builtinTypeId = 50U;
    GlobalTSTypeRef builtinGT = tsTypeParser.CreateGT(jsPandaFile, recordName, builtinTypeId);
    EXPECT_EQ(builtinGT, GlobalTSTypeRef(static_cast<uint32_t>(ModuleTableIdx::BUILTIN), builtinTypeId));
}

HWTEST_F_L0(TSTypeParserTest, TestTSClassType)
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

    const std::string classId("test_1");
    const std::string valueStr("value");
    std::vector<panda_file::LiteralTag> classTags { panda_file::LiteralTag::INTEGER,
                                                    panda_file::LiteralTag::INTEGER,
                                                    panda_file::LiteralTag::BUILTINTYPEINDEX,
                                                    panda_file::LiteralTag::INTEGER,
                                                    panda_file::LiteralTag::INTEGER,
                                                    panda_file::LiteralTag::INTEGER,
                                                    panda_file::LiteralTag::INTEGER,
                                                    panda_file::LiteralTag::STRING,
                                                    panda_file::LiteralTag::BUILTINTYPEINDEX,
                                                    panda_file::LiteralTag::INTEGER,
                                                    panda_file::LiteralTag::INTEGER,
                                                    panda_file::LiteralTag::INTEGER };
    std::vector<LiteralValueType> classValues { static_cast<uint32_t>(1),
                                                static_cast<uint32_t>(0),
                                                static_cast<uint8_t>(0),
                                                static_cast<uint32_t>(0),
                                                static_cast<uint32_t>(0),
                                                static_cast<uint32_t>(0),
                                                static_cast<uint32_t>(1),
                                                valueStr,
                                                static_cast<uint8_t>(1),
                                                static_cast<uint32_t>(0),
                                                static_cast<uint32_t>(0),
                                                static_cast<uint32_t>(0) };
    TSTypeTestHelper::AddLiteral(program, classId, classTags, classValues);

    const std::string abcFileName("TSClassTypeTest.abc");
    TSTypeTestHelper::AddTypeSummary(program, { classId });
    TSTypeTestHelper::AddCommonJsField(program);
    std::map<std::string, size_t> *statp = nullptr;
    pandasm::AsmEmitter::PandaFileToPandaAsmMaps maps {};
    pandasm::AsmEmitter::PandaFileToPandaAsmMaps *mapsp = &maps;
    EXPECT_TRUE(pandasm::AsmEmitter::Emit(abcFileName, program, statp, mapsp, false));
    std::unique_ptr<const panda_file::File> pfPtr = panda_file::File::Open(abcFileName);
    EXPECT_NE(pfPtr.get(), nullptr);

    Span<const uint32_t> literalArrays = pfPtr.get()->GetLiteralArrays();
    EXPECT_TRUE(literalArrays.size() >= 2);
    // typeIds in order from largest to smallest and the last one is typeSummary literal
    uint32_t testTypeIndex = literalArrays.size() - 2;
    uint32_t testTypeOffset = literalArrays[testTypeIndex];

    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    const CString fileName(abcFileName.c_str());
    const auto jsPandaFile = pfManager->NewJSPandaFile(pfPtr.release(), fileName);
    EXPECT_NE(jsPandaFile, nullptr);

    auto tsManager = ecmaVm->GetJSThread()->GetCurrentEcmaContext()->GetTSManager();
    tsManager->Initialize();
    TSTypeParser tsTypeParser(tsManager);
    const CString recordName("test");
    GlobalTSTypeRef resultGT = tsTypeParser.CreateGT(jsPandaFile.get(), recordName, testTypeOffset);
    EXPECT_EQ(resultGT, GlobalTSTypeRef(FIRST_USER_DEFINE_MODULE_ID, FIRST_USER_DEFINE_LOCAL_ID));
    EXPECT_TRUE(tsManager->IsClassTypeKind(resultGT));
    JSHandle<JSTaggedValue> type = tsManager->GetTSType(resultGT);
    EXPECT_TRUE(type->IsTSClassType());

    JSHandle<TSClassType> classType(type);
    EXPECT_EQ(resultGT, classType->GetGT());
    auto factory = ecmaVm->GetFactory();
    JSHandle<JSTaggedValue> propertyName(factory->NewFromStdString(valueStr));
    GlobalTSTypeRef propGT = TSClassType::GetPropTypeGT(thread, classType, propertyName);
    EXPECT_EQ(propGT, GlobalTSTypeRef(static_cast<uint32_t>(ModuleTableIdx::PRIMITIVE),
                                      static_cast<uint32_t>(TSPrimitiveType::NUMBER)));
}

HWTEST_F_L0(TSTypeParserTest, TestTSFunctionType)
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

    const std::string functionId("test_1");
    const std::string functionName("foo");
    const uint32_t numOfParas = 2;
    std::vector<panda_file::LiteralTag> functionTags { panda_file::LiteralTag::INTEGER,
                                                       panda_file::LiteralTag::INTEGER,
                                                       panda_file::LiteralTag::STRING,
                                                       panda_file::LiteralTag::INTEGER,
                                                       panda_file::LiteralTag::INTEGER,
                                                       panda_file::LiteralTag::BUILTINTYPEINDEX,
                                                       panda_file::LiteralTag::BUILTINTYPEINDEX,
                                                       panda_file::LiteralTag::BUILTINTYPEINDEX };
    std::vector<LiteralValueType> functionValues { static_cast<uint32_t>(3),
                                                   static_cast<uint32_t>(0),
                                                   functionName,
                                                   static_cast<uint32_t>(0),
                                                   numOfParas,
                                                   static_cast<uint8_t>(1),
                                                   static_cast<uint8_t>(4),
                                                   static_cast<uint8_t>(2) };
    TSTypeTestHelper::AddLiteral(program, functionId, functionTags, functionValues);

    const std::string abcFileName("TSFunctionTypeTest.abc");
    TSTypeTestHelper::AddTypeSummary(program, { functionId });
    TSTypeTestHelper::AddCommonJsField(program);
    std::map<std::string, size_t> *statp = nullptr;
    pandasm::AsmEmitter::PandaFileToPandaAsmMaps maps {};
    pandasm::AsmEmitter::PandaFileToPandaAsmMaps *mapsp = &maps;
    EXPECT_TRUE(pandasm::AsmEmitter::Emit(abcFileName, program, statp, mapsp, false));
    std::unique_ptr<const panda_file::File> pfPtr = panda_file::File::Open(abcFileName);
    EXPECT_NE(pfPtr.get(), nullptr);

    Span<const uint32_t> literalArrays = pfPtr.get()->GetLiteralArrays();
    EXPECT_TRUE(literalArrays.size() >= 2);
    // typeIds in order from largest to smallest and the last one is typeSummary literal
    uint32_t testTypeIndex = literalArrays.size() - 2;
    uint32_t testTypeOffset = literalArrays[testTypeIndex];

    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    const CString fileName(abcFileName.c_str());
    const auto jsPandaFile = pfManager->NewJSPandaFile(pfPtr.release(), fileName);
    EXPECT_NE(jsPandaFile, nullptr);

    auto tsManager = ecmaVm->GetJSThread()->GetCurrentEcmaContext()->GetTSManager();
    tsManager->Initialize();
    TSTypeParser tsTypeParser(tsManager);
    const CString recordName("test");
    GlobalTSTypeRef resultGT = tsTypeParser.CreateGT(jsPandaFile.get(), recordName, testTypeOffset);
    EXPECT_EQ(resultGT, GlobalTSTypeRef(FIRST_USER_DEFINE_MODULE_ID, FIRST_USER_DEFINE_LOCAL_ID));
    EXPECT_TRUE(tsManager->IsFunctionTypeKind(resultGT));
    JSHandle<JSTaggedValue> type = tsManager->GetTSType(resultGT);
    EXPECT_TRUE(type->IsTSFunctionType());

    JSHandle<TSFunctionType> functionType(type);
    EXPECT_EQ(resultGT, functionType->GetGT());
    EXPECT_EQ(functionType->GetLength(), numOfParas);
    EXPECT_EQ(functionType->GetParameterTypeGT(0), GlobalTSTypeRef(static_cast<uint32_t>(ModuleTableIdx::PRIMITIVE),
                                                                   static_cast<uint32_t>(TSPrimitiveType::NUMBER)));
    EXPECT_EQ(functionType->GetParameterTypeGT(1), GlobalTSTypeRef(static_cast<uint32_t>(ModuleTableIdx::PRIMITIVE),
                                                                   static_cast<uint32_t>(TSPrimitiveType::STRING)));
    EXPECT_EQ(functionType->GetReturnGT(), GlobalTSTypeRef(static_cast<uint32_t>(ModuleTableIdx::PRIMITIVE),
                                                           static_cast<uint32_t>(TSPrimitiveType::BOOLEAN)));
}

HWTEST_F_L0(TSTypeParserTest, TestTSUnionType)
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

    const std::string unionId("test_1");
    const uint32_t numOfTypes = 2;
    std::vector<panda_file::LiteralTag> unionTags { panda_file::LiteralTag::INTEGER,
                                                    panda_file::LiteralTag::INTEGER,
                                                    panda_file::LiteralTag::BUILTINTYPEINDEX,
                                                    panda_file::LiteralTag::BUILTINTYPEINDEX };
    std::vector<LiteralValueType> unionValues { static_cast<uint32_t>(4),
                                                numOfTypes,
                                                static_cast<uint8_t>(1),
                                                static_cast<uint8_t>(4) };
    TSTypeTestHelper::AddLiteral(program, unionId, unionTags, unionValues);

    const std::string abcFileName("TSUnionTypeTest.abc");
    TSTypeTestHelper::AddTypeSummary(program, { unionId });
    TSTypeTestHelper::AddCommonJsField(program);
    std::map<std::string, size_t> *statp = nullptr;
    pandasm::AsmEmitter::PandaFileToPandaAsmMaps maps {};
    pandasm::AsmEmitter::PandaFileToPandaAsmMaps *mapsp = &maps;
    EXPECT_TRUE(pandasm::AsmEmitter::Emit(abcFileName, program, statp, mapsp, false));
    std::unique_ptr<const panda_file::File> pfPtr = panda_file::File::Open(abcFileName);
    EXPECT_NE(pfPtr.get(), nullptr);

    Span<const uint32_t> literalArrays = pfPtr.get()->GetLiteralArrays();
    EXPECT_TRUE(literalArrays.size() >= 2);
    // typeIds in order from largest to smallest and the last one is typeSummary literal
    uint32_t testTypeIndex = literalArrays.size() - 2;
    uint32_t testTypeOffset = literalArrays[testTypeIndex];

    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    const CString fileName(abcFileName.c_str());
    const auto jsPandaFile = pfManager->NewJSPandaFile(pfPtr.release(), fileName);
    EXPECT_NE(jsPandaFile, nullptr);

    auto tsManager = ecmaVm->GetJSThread()->GetCurrentEcmaContext()->GetTSManager();
    tsManager->Initialize();
    TSTypeParser tsTypeParser(tsManager);
    const CString recordName("test");
    GlobalTSTypeRef resultGT = tsTypeParser.CreateGT(jsPandaFile.get(), recordName, testTypeOffset);
    EXPECT_EQ(resultGT, GlobalTSTypeRef(FIRST_USER_DEFINE_MODULE_ID, FIRST_USER_DEFINE_LOCAL_ID));
    EXPECT_TRUE(tsManager->IsUnionTypeKind(resultGT));
    JSHandle<JSTaggedValue> type = tsManager->GetTSType(resultGT);
    EXPECT_TRUE(type->IsTSUnionType());

    JSHandle<TSUnionType> unionType(type);
    EXPECT_EQ(resultGT, unionType->GetGT());
    EXPECT_EQ(tsManager->GetUnionTypeLength(resultGT), numOfTypes);
    EXPECT_EQ(tsManager->GetUnionTypeByIndex(resultGT, 0),
        GlobalTSTypeRef(static_cast<uint32_t>(ModuleTableIdx::PRIMITIVE),
                        static_cast<uint32_t>(TSPrimitiveType::NUMBER)));
    EXPECT_EQ(tsManager->GetUnionTypeByIndex(resultGT, 1),
        GlobalTSTypeRef(static_cast<uint32_t>(ModuleTableIdx::PRIMITIVE),
                        static_cast<uint32_t>(TSPrimitiveType::STRING)));
}

HWTEST_F_L0(TSTypeParserTest, TestTSArrayType)
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

    const std::string arrayId("test_1");
    std::vector<panda_file::LiteralTag> arrayTags { panda_file::LiteralTag::INTEGER,
                                                    panda_file::LiteralTag::BUILTINTYPEINDEX };
    std::vector<LiteralValueType> arrayValues { static_cast<uint32_t>(5),
                                                static_cast<uint8_t>(1) };
    TSTypeTestHelper::AddLiteral(program, arrayId, arrayTags, arrayValues);

    const std::string abcFileName("TSArrayTypeTest.abc");
    TSTypeTestHelper::AddTypeSummary(program, { arrayId });
    TSTypeTestHelper::AddCommonJsField(program);
    std::map<std::string, size_t> *statp = nullptr;
    pandasm::AsmEmitter::PandaFileToPandaAsmMaps maps {};
    pandasm::AsmEmitter::PandaFileToPandaAsmMaps *mapsp = &maps;
    EXPECT_TRUE(pandasm::AsmEmitter::Emit(abcFileName, program, statp, mapsp, false));
    std::unique_ptr<const panda_file::File> pfPtr = panda_file::File::Open(abcFileName);
    EXPECT_NE(pfPtr.get(), nullptr);

    Span<const uint32_t> literalArrays = pfPtr.get()->GetLiteralArrays();
    EXPECT_TRUE(literalArrays.size() >= 2);
    // typeIds in order from largest to smallest and the last one is typeSummary literal
    uint32_t testTypeIndex = literalArrays.size() - 2;
    uint32_t testTypeOffset = literalArrays[testTypeIndex];

    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    const CString fileName(abcFileName.c_str());
    const auto jsPandaFile = pfManager->NewJSPandaFile(pfPtr.release(), fileName);
    EXPECT_NE(jsPandaFile, nullptr);

    auto tsManager = ecmaVm->GetJSThread()->GetCurrentEcmaContext()->GetTSManager();
    tsManager->Initialize();
    TSTypeParser tsTypeParser(tsManager);
    const CString recordName("test");
    GlobalTSTypeRef resultGT = tsTypeParser.CreateGT(jsPandaFile.get(), recordName, testTypeOffset);
    EXPECT_EQ(resultGT, GlobalTSTypeRef(FIRST_USER_DEFINE_MODULE_ID, FIRST_USER_DEFINE_LOCAL_ID));
    EXPECT_TRUE(tsManager->IsArrayTypeKind(resultGT));
    JSHandle<JSTaggedValue> type = tsManager->GetTSType(resultGT);
    EXPECT_TRUE(type->IsTSArrayType());

    JSHandle<TSArrayType> arrayType(type);
    EXPECT_EQ(resultGT, arrayType->GetGT());
    EXPECT_EQ(arrayType->GetElementGT(), GlobalTSTypeRef(static_cast<uint32_t>(ModuleTableIdx::PRIMITIVE),
                                                         static_cast<uint32_t>(TSPrimitiveType::NUMBER)));
}

HWTEST_F_L0(TSTypeParserTest, TestTSObjectType)
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

    const std::string objectId("test_1");
    const std::string ageStr("age");
    const std::string funStr("fun");
    std::vector<panda_file::LiteralTag> objectTags { panda_file::LiteralTag::INTEGER,
                                                     panda_file::LiteralTag::INTEGER,
                                                     panda_file::LiteralTag::STRING,
                                                     panda_file::LiteralTag::BUILTINTYPEINDEX,
                                                     panda_file::LiteralTag::STRING,
                                                     panda_file::LiteralTag::BUILTINTYPEINDEX };
    std::vector<LiteralValueType> objectValues { static_cast<uint32_t>(6),
                                                 static_cast<uint32_t>(2),
                                                 ageStr,
                                                 static_cast<uint8_t>(1),
                                                 funStr,
                                                 static_cast<uint8_t>(1) };
    TSTypeTestHelper::AddLiteral(program, objectId, objectTags, objectValues);

    const std::string abcFileName("TSObjectTypeTest.abc");
    TSTypeTestHelper::AddTypeSummary(program, { objectId });
    TSTypeTestHelper::AddCommonJsField(program);
    std::map<std::string, size_t> *statp = nullptr;
    pandasm::AsmEmitter::PandaFileToPandaAsmMaps maps {};
    pandasm::AsmEmitter::PandaFileToPandaAsmMaps *mapsp = &maps;
    EXPECT_TRUE(pandasm::AsmEmitter::Emit(abcFileName, program, statp, mapsp, false));
    std::unique_ptr<const panda_file::File> pfPtr = panda_file::File::Open(abcFileName);
    EXPECT_NE(pfPtr.get(), nullptr);

    Span<const uint32_t> literalArrays = pfPtr.get()->GetLiteralArrays();
    EXPECT_TRUE(literalArrays.size() >= 2);
    // typeIds in order from largest to smallest and the last one is typeSummary literal
    uint32_t testTypeIndex = literalArrays.size() - 2;
    uint32_t testTypeOffset = literalArrays[testTypeIndex];

    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    const CString fileName(abcFileName.c_str());
    const auto jsPandaFile = pfManager->NewJSPandaFile(pfPtr.release(), fileName);
    EXPECT_NE(jsPandaFile, nullptr);

    auto tsManager = ecmaVm->GetJSThread()->GetCurrentEcmaContext()->GetTSManager();
    tsManager->Initialize();
    TSTypeParser tsTypeParser(tsManager);
    const CString recordName("test");
    GlobalTSTypeRef resultGT = tsTypeParser.CreateGT(jsPandaFile.get(), recordName, testTypeOffset);
    EXPECT_EQ(resultGT, GlobalTSTypeRef(FIRST_USER_DEFINE_MODULE_ID, FIRST_USER_DEFINE_LOCAL_ID));
    EXPECT_TRUE(tsManager->IsObjectTypeKind(resultGT));
    JSHandle<JSTaggedValue> type = tsManager->GetTSType(resultGT);
    EXPECT_TRUE(type->IsTSObjectType());

    JSHandle<TSObjectType> objectType(type);
    EXPECT_EQ(resultGT, objectType->GetGT());
    auto factory = ecmaVm->GetFactory();
    JSHandle<JSTaggedValue> propName(factory->NewFromStdString(ageStr));
    GlobalTSTypeRef propGT = TSObjectType::GetPropTypeGT(thread, objectType, propName);
    EXPECT_EQ(propGT, GlobalTSTypeRef(static_cast<uint32_t>(ModuleTableIdx::PRIMITIVE),
                                      static_cast<uint32_t>(TSPrimitiveType::NUMBER)));
}
}  // namespace panda::test
