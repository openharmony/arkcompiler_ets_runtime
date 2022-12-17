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

#include "assembler/assembly-parser.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/ts_types/ts_type_parser.h"

namespace panda::test {
using namespace panda::ecmascript;
using namespace panda::panda_file;
using namespace panda::pandasm;
using LiteralValueType = std::variant<uint8_t, uint32_t, std::string>;

static constexpr uint16_t FIRST_USER_DEFINE_MODULE_ID = TSModuleTable::DEFAULT_NUMBER_OF_TABLES;
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

    void AddLiteral(pandasm::Program &program, const std::string &literalId,
                    const std::vector<panda_file::LiteralTag> &tags,
                    const std::vector<LiteralValueType> &values);
    void AddTagValue(std::vector<LiteralArray::Literal> &literalArray,
                     const panda_file::LiteralTag tag,
                     const LiteralValueType &value);
    void AddTypeSummary(pandasm::Program &program, const std::vector<std::string> &typeIds);
    void AddSummaryLiteral(pandasm::Program &program, const std::string &typeSummaryId,
                           const std::vector<std::string> &typeIds);
    void AddCommonJsField(pandasm::Program &program);

    EcmaVM *ecmaVm {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

void TSTypeParserTest::AddLiteral(pandasm::Program &program, const std::string &literalId,
                                  const std::vector<panda_file::LiteralTag> &tags,
                                  const std::vector<LiteralValueType> &values)
{
    EXPECT_EQ(tags.size(), values.size());
    std::vector<pandasm::LiteralArray::Literal> literal {};
    for (uint32_t i = 0; i < tags.size(); i++) {
        AddTagValue(literal, tags[i], values[i]);
    }
    pandasm::LiteralArray literalArray(literal);
    program.literalarray_table.emplace(literalId, literalArray);
}

void TSTypeParserTest::AddTagValue(std::vector<LiteralArray::Literal> &literalArray,
                                   const panda_file::LiteralTag tag,
                                   const LiteralValueType &value)
{
    pandasm::LiteralArray::Literal literalTag;
    literalTag.tag_ = panda_file::LiteralTag::TAGVALUE;
    literalTag.value_ = static_cast<uint8_t>(tag);
    literalArray.emplace_back(std::move(literalTag));

    pandasm::LiteralArray::Literal literalValue;
    literalValue.tag_ = tag;

    if (tag == panda_file::LiteralTag::INTEGER) {
        literalValue.value_ = std::get<uint32_t>(value);
    } else if (tag == panda_file::LiteralTag::BUILTINTYPEINDEX) {
        literalValue.value_ = std::get<uint8_t>(value);
    } else if (tag == panda_file::LiteralTag::STRING) {
        literalValue.value_ = std::get<std::string>(value);
    } else if (tag == panda_file::LiteralTag::LITERALARRAY) {
        literalValue.value_ = std::get<std::string>(value);
    } else {
        EXPECT_FALSE(true);
    }

    literalArray.emplace_back(std::move(literalValue));
}

void TSTypeParserTest::AddTypeSummary(pandasm::Program &program, const std::vector<std::string> &typeIds)
{
    const std::string typeSummaryId("test_0");
    AddSummaryLiteral(program, typeSummaryId, typeIds);

    const std::string testStr("test");
    auto iter = program.record_table.find(testStr);
    EXPECT_NE(iter, program.record_table.end());
    if (iter != program.record_table.end()) {
        auto &rec = iter->second;
        auto typeSummaryIndexField = pandasm::Field(pandasm::extensions::Language::ECMASCRIPT);
        typeSummaryIndexField.name = "typeSummaryOffset";
        typeSummaryIndexField.type = pandasm::Type("u32", 0);
        typeSummaryIndexField.metadata->SetValue(
            pandasm::ScalarValue::Create<pandasm::Value::Type::LITERALARRAY>(typeSummaryId));
        rec.field_list.emplace_back(std::move(typeSummaryIndexField));
    }
}

void TSTypeParserTest::AddSummaryLiteral(pandasm::Program &program, const std::string &typeSummaryId,
                                         const std::vector<std::string> &typeIds)
{
    uint32_t numOfTypes = typeIds.size();
    std::vector<panda_file::LiteralTag> typeSummaryTags { panda_file::LiteralTag::INTEGER };
    std::vector<LiteralValueType> typeSummaryValues { numOfTypes };
    for (uint32_t i = 0; i < numOfTypes; i++) {
        typeSummaryTags.emplace_back(panda_file::LiteralTag::LITERALARRAY);
        typeSummaryValues.emplace_back(typeIds[i]);
    }
    typeSummaryTags.emplace_back(panda_file::LiteralTag::INTEGER);
    typeSummaryValues.emplace_back(static_cast<uint32_t>(0U));
    AddLiteral(program, typeSummaryId, typeSummaryTags, typeSummaryValues);
}

void TSTypeParserTest::AddCommonJsField(pandasm::Program &program)
{
    const std::string testStr("test");
    auto iter = program.record_table.find(testStr);
    EXPECT_NE(iter, program.record_table.end());
    if (iter != program.record_table.end()) {
        auto &rec = iter->second;
        auto isCommonJsField = pandasm::Field(pandasm::extensions::Language::ECMASCRIPT);
        isCommonJsField.name = "isCommonjs";
        isCommonJsField.type = pandasm::Type("u8", 0);
        isCommonJsField.metadata->SetValue(
            pandasm::ScalarValue::Create<pandasm::Value::Type::U8>(static_cast<uint8_t>(false)));
        rec.field_list.emplace_back(std::move(isCommonJsField));
    }
}

HWTEST_F_L0(TSTypeParserTest, TestPrimetiveType)
{
    auto tsManager = ecmaVm->GetTSManager();
    TSTypeParser tsTypeParser(tsManager);
    JSPandaFile *jsPandaFile = nullptr;
    const CString recordName("");
    uint32_t primetiveTypeId = 0U;
    GlobalTSTypeRef resultGT = tsTypeParser.CreateGT(jsPandaFile, recordName, primetiveTypeId);
    EXPECT_EQ(resultGT, GlobalTSTypeRef(TSModuleTable::PRIMITIVE_TABLE_ID, primetiveTypeId));
}

HWTEST_F_L0(TSTypeParserTest, TestBuiltinType)
{
    auto tsManager = ecmaVm->GetTSManager();
    TSTypeParser tsTypeParser(tsManager);
    JSPandaFile *jsPandaFile = nullptr;
    const CString recordName("");
    uint32_t builtinTypeId = 50U;
    GlobalTSTypeRef builtinGT = tsTypeParser.CreateGT(jsPandaFile, recordName, builtinTypeId);
    EXPECT_EQ(builtinGT, GlobalTSTypeRef(TSModuleTable::BUILTINS_TABLE_ID, builtinTypeId));
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
    AddLiteral(program, classId, classTags, classValues);

    const std::string abcFileName("TSClassTypeTest.abc");
    AddTypeSummary(program, { classId });
    AddCommonJsField(program);
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
    const JSPandaFile *jsPandaFile = pfManager->NewJSPandaFile(pfPtr.release(), fileName);
    EXPECT_NE(jsPandaFile, nullptr);

    auto tsManager = ecmaVm->GetTSManager();
    TSTypeParser tsTypeParser(tsManager);
    const CString recordName("test");
    GlobalTSTypeRef resultGT = tsTypeParser.CreateGT(jsPandaFile, recordName, testTypeOffset);
    EXPECT_EQ(resultGT, GlobalTSTypeRef(FIRST_USER_DEFINE_MODULE_ID, FIRST_USER_DEFINE_LOCAL_ID));
    EXPECT_TRUE(tsManager->IsClassTypeKind(resultGT));
    JSHandle<JSTaggedValue> type = tsManager->GetTSType(resultGT);
    EXPECT_TRUE(type->IsTSClassType());

    JSHandle<TSClassType> classType(type);
    EXPECT_EQ(resultGT, classType->GetGT());
    auto factory = ecmaVm->GetFactory();
    JSHandle<EcmaString> propertyName = factory->NewFromStdString(valueStr);
    GlobalTSTypeRef propGT = TSClassType::GetPropTypeGT(thread, classType, propertyName);
    EXPECT_EQ(propGT,
              GlobalTSTypeRef(TSModuleTable::PRIMITIVE_TABLE_ID, static_cast<uint16_t>(TSPrimitiveType::NUMBER)));
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
    AddLiteral(program, functionId, functionTags, functionValues);

    const std::string abcFileName("TSFunctionTypeTest.abc");
    AddTypeSummary(program, { functionId });
    AddCommonJsField(program);
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
    const JSPandaFile *jsPandaFile = pfManager->NewJSPandaFile(pfPtr.release(), fileName);
    EXPECT_NE(jsPandaFile, nullptr);

    auto tsManager = ecmaVm->GetTSManager();
    TSTypeParser tsTypeParser(tsManager);
    const CString recordName("test");
    GlobalTSTypeRef resultGT = tsTypeParser.CreateGT(jsPandaFile, recordName, testTypeOffset);
    EXPECT_EQ(resultGT, GlobalTSTypeRef(FIRST_USER_DEFINE_MODULE_ID, FIRST_USER_DEFINE_LOCAL_ID));
    EXPECT_TRUE(tsManager->IsFunctionTypeKind(resultGT));
    JSHandle<JSTaggedValue> type = tsManager->GetTSType(resultGT);
    EXPECT_TRUE(type->IsTSFunctionType());

    JSHandle<TSFunctionType> functionType(type);
    EXPECT_EQ(resultGT, functionType->GetGT());
    EXPECT_EQ(functionType->GetLength(), numOfParas);
    EXPECT_EQ(functionType->GetParameterTypeGT(0),
              GlobalTSTypeRef(TSModuleTable::PRIMITIVE_TABLE_ID, static_cast<uint16_t>(TSPrimitiveType::NUMBER)));
    EXPECT_EQ(functionType->GetParameterTypeGT(1),
              GlobalTSTypeRef(TSModuleTable::PRIMITIVE_TABLE_ID, static_cast<uint16_t>(TSPrimitiveType::STRING)));
    EXPECT_EQ(functionType->GetReturnGT(),
              GlobalTSTypeRef(TSModuleTable::PRIMITIVE_TABLE_ID, static_cast<uint16_t>(TSPrimitiveType::BOOLEAN)));
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
    AddLiteral(program, unionId, unionTags, unionValues);

    const std::string abcFileName("TSUnionTypeTest.abc");
    AddTypeSummary(program, { unionId });
    AddCommonJsField(program);
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
    const JSPandaFile *jsPandaFile = pfManager->NewJSPandaFile(pfPtr.release(), fileName);
    EXPECT_NE(jsPandaFile, nullptr);

    auto tsManager = ecmaVm->GetTSManager();
    TSTypeParser tsTypeParser(tsManager);
    const CString recordName("test");
    GlobalTSTypeRef resultGT = tsTypeParser.CreateGT(jsPandaFile, recordName, testTypeOffset);
    EXPECT_EQ(resultGT, GlobalTSTypeRef(FIRST_USER_DEFINE_MODULE_ID, FIRST_USER_DEFINE_LOCAL_ID));
    EXPECT_TRUE(tsManager->IsUnionTypeKind(resultGT));
    JSHandle<JSTaggedValue> type = tsManager->GetTSType(resultGT);
    EXPECT_TRUE(type->IsTSUnionType());

    JSHandle<TSUnionType> unionType(type);
    EXPECT_EQ(resultGT, unionType->GetGT());
    EXPECT_EQ(tsManager->GetUnionTypeLength(resultGT), numOfTypes);
    EXPECT_EQ(tsManager->GetUnionTypeByIndex(resultGT, 0),
              GlobalTSTypeRef(TSModuleTable::PRIMITIVE_TABLE_ID, static_cast<uint16_t>(TSPrimitiveType::NUMBER)));
    EXPECT_EQ(tsManager->GetUnionTypeByIndex(resultGT, 1),
              GlobalTSTypeRef(TSModuleTable::PRIMITIVE_TABLE_ID, static_cast<uint16_t>(TSPrimitiveType::STRING)));
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
    AddLiteral(program, arrayId, arrayTags, arrayValues);

    const std::string abcFileName("TSArrayTypeTest.abc");
    AddTypeSummary(program, { arrayId });
    AddCommonJsField(program);
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
    const JSPandaFile *jsPandaFile = pfManager->NewJSPandaFile(pfPtr.release(), fileName);
    EXPECT_NE(jsPandaFile, nullptr);

    auto tsManager = ecmaVm->GetTSManager();
    TSTypeParser tsTypeParser(tsManager);
    const CString recordName("test");
    GlobalTSTypeRef resultGT = tsTypeParser.CreateGT(jsPandaFile, recordName, testTypeOffset);
    EXPECT_EQ(resultGT, GlobalTSTypeRef(FIRST_USER_DEFINE_MODULE_ID, FIRST_USER_DEFINE_LOCAL_ID));
    EXPECT_TRUE(tsManager->IsArrayTypeKind(resultGT));
    JSHandle<JSTaggedValue> type = tsManager->GetTSType(resultGT);
    EXPECT_TRUE(type->IsTSArrayType());

    JSHandle<TSArrayType> arrayType(type);
    EXPECT_EQ(resultGT, arrayType->GetGT());
    EXPECT_EQ(arrayType->GetElementGT(),
              GlobalTSTypeRef(TSModuleTable::PRIMITIVE_TABLE_ID, static_cast<uint16_t>(TSPrimitiveType::NUMBER)));
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
    AddLiteral(program, objectId, objectTags, objectValues);

    const std::string abcFileName("TSObjectTypeTest.abc");
    AddTypeSummary(program, { objectId });
    AddCommonJsField(program);
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
    const JSPandaFile *jsPandaFile = pfManager->NewJSPandaFile(pfPtr.release(), fileName);
    EXPECT_NE(jsPandaFile, nullptr);

    auto tsManager = ecmaVm->GetTSManager();
    TSTypeParser tsTypeParser(tsManager);
    const CString recordName("test");
    GlobalTSTypeRef resultGT = tsTypeParser.CreateGT(jsPandaFile, recordName, testTypeOffset);
    EXPECT_EQ(resultGT, GlobalTSTypeRef(FIRST_USER_DEFINE_MODULE_ID, FIRST_USER_DEFINE_LOCAL_ID));
    EXPECT_TRUE(tsManager->IsObjectTypeKind(resultGT));
    JSHandle<JSTaggedValue> type = tsManager->GetTSType(resultGT);
    EXPECT_TRUE(type->IsTSObjectType());

    JSHandle<TSObjectType> objectType(type);
    EXPECT_EQ(resultGT, objectType->GetGT());
    auto factory = ecmaVm->GetFactory();
    JSHandle<EcmaString> propName = factory->NewFromStdString(ageStr);
    GlobalTSTypeRef propGT = TSObjectType::GetPropTypeGT(objectType, propName);
    EXPECT_EQ(propGT,
              GlobalTSTypeRef(TSModuleTable::PRIMITIVE_TABLE_ID, static_cast<uint16_t>(TSPrimitiveType::NUMBER)));
}
} // namespace panda::test
