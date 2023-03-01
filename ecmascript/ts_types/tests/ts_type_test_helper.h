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

#ifndef ECMASCRIPT_TS_TYPES_TESTS_TS_TYPE_TEST_HELPER_H
#define ECMASCRIPT_TS_TYPES_TESTS_TS_TYPE_TEST_HELPER_H

#include "assembler/assembly-parser.h"
#include "ecmascript/tests/test_helper.h"

namespace panda::test {
using namespace panda::ecmascript;
using namespace panda::panda_file;
using namespace panda::pandasm;
using LiteralValueType = std::variant<uint8_t, uint32_t, std::string>;

class TSTypeTestHelper {
public:
    static void AddLiteral(pandasm::Program &program, const std::string &literalId,
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

    static void AddTypeSummary(pandasm::Program &program, const std::vector<std::string> &typeIds)
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

    static void AddCommonJsField(pandasm::Program &program)
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

private:
    static void AddTagValue(std::vector<LiteralArray::Literal> &literalArray,
                            const panda_file::LiteralTag tag,
                            const LiteralValueType &value)
    {
        pandasm::LiteralArray::Literal literalTag;
        literalTag.tag_ = panda_file::LiteralTag::TAGVALUE;
        literalTag.value_ = static_cast<uint8_t>(tag);
        literalArray.emplace_back(std::move(literalTag));

        pandasm::LiteralArray::Literal literalValue;
        literalValue.tag_ = tag;

        switch (tag) {
            case panda_file::LiteralTag::INTEGER: {
                literalValue.value_ = std::get<uint32_t>(value);
                break;
            }
            case panda_file::LiteralTag::BUILTINTYPEINDEX: {
                literalValue.value_ = std::get<uint8_t>(value);
                break;
            }
            case panda_file::LiteralTag::STRING: {
                literalValue.value_ = std::get<std::string>(value);
                break;
            }
            case panda_file::LiteralTag::LITERALARRAY: {
                literalValue.value_ = std::get<std::string>(value);
                break;
            }
            default: {
                EXPECT_FALSE(true);
            }
        }

        literalArray.emplace_back(std::move(literalValue));
    }

    static void AddSummaryLiteral(pandasm::Program &program, const std::string &typeSummaryId,
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
};
}  // namespace panda::test
#endif  // ECMASCRIPT_TS_TYPES_TESTS_TS_TYPE_TEST_HELPER_H
