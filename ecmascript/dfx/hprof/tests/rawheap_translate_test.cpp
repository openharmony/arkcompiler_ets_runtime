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

#include "ecmascript/tests/test_helper.h"
#include "ecmascript/dfx/hprof/rawheap_translate/metadata_parse.h"
#include "ecmascript/dfx/hprof/rawheap_translate/utils.h"

using namespace panda::ecmascript;

namespace panda::test {
class RawHeapTranslateTest : public testing::Test {
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
        metaParser = std::make_unique<rawheap_translate::MetaParser>();
    }

    void TearDown() override
    {
    }

    std::unique_ptr<rawheap_translate::MetaParser> metaParser {nullptr};
};

HWTEST_F_L0(RawHeapTranslateTest, MetaDataParse)
{
    /*
    metadataJson a json is similar to the following example
    {
        "type_enum": {"INVALID": 8},
        "type_list": [
            {
                "name": "JS_OBJECT",
                "offsets": [
                    {
                        "name": "Properties",
                        "offset": 0,
                        "size": 8
                    },
                    {
                        "name": "Elements",
                        "offset": 8,
                        "size": 8
                    }
                ],
                "end_offset": 16,
                "parents": [
                    "ECMA_OBJECT"
                ]
            },
            {
                "name": "ECMA_OBJECT",
                "offsets": [],
                "end_offset": 8,
                "parents": [
                    "TAGGED_OBJECT"
                ]
            },
            {
                "name": "TAGGED_OBJECT",
                "offsets": [],
                "end_offset": 8,
                "parents": []
            }
        ],
        "type_layout": {
            "Dictionary_layout": {
                "name": "Dictionary",
                "key_index": 0,
                "value_index": 1,
                "detail_index": 2,
                "entry_size": 3,
                "header_size": 4
            },
            "Type_range": {
                "string_first": "LINE_STRING",
                "string_last": "TREE_STRING",
                "js_object_first": "JS_OBJECT",
                "js_object_last": "JS_GLOBAL_OBJECT"
            }
        },
        "version": "1.0.0"
    }
    */
    std::string metadataJson =
        "{\"type_enum\": {\"INVALID\": 8}, \"type_list\": [{\"name\": \"JS_OBJECT\","
        "\"offsets\": [{\"name\": \"Properties\", \"offset\": 0, \"size\": 8}, "
        "{\"name\": \"Elements\", \"offset\": 8, \"size\": 8}], \"end_offset\": 16, "
        "\"parents\": [\"ECMA_OBJECT\"]}, {\"name\": \"ECMA_OBJECT\", \"offsets\": [], "
        "\"end_offset\": 8, \"parents\": [\"TAGGED_OBJECT\"]}, {\"name\": \"TAGGED_OBJECT\", "
        "\"offsets\": [], \"end_offset\": 8, \"parents\": []}], \"type_layout\": {\"Dictionary_layout\": {"
        "\"name\": \"Dictionary\", \"key_index\": 0, \"value_index\": 1, \"detail_index\": 2, "
        "\"entry_size\": 3, \"header_size\": 4}, \"Type_range\": {\"string_first\": \"LINE_STRING\", "
        "\"string_last\": \"TREE_STRING\", \"js_object_first\": \"JS_OBJECT\", \"js_object_last\": "
        "\"JS_GLOBAL_OBJECT\"}}, \"version\": \"1.0.0\"}";

    cJSON *metadataCJson = cJSON_ParseWithOpts(metadataJson.c_str(), nullptr, true);
    ASSERT_TRUE(metadataCJson != nullptr);

    bool result = metaParser->Parse(metadataCJson);
    cJSON_Delete(metadataCJson);
    ASSERT_TRUE(result);

    rawheap_translate::MetaData *meta = metaParser->GetMetaData("JS_OBJECT");
    ASSERT_TRUE(meta != nullptr);
    ASSERT_EQ(meta->endOffset, 32);  // 32: 16 + 8 + 8 = 32, all parent endOffset count total
}

HWTEST_F_L0(RawHeapTranslateTest, BytesToNumber)
{
    char bytes[4] = {0x78, 0x56, 0x34, 0x12};  // write by little endian

    uint32_t u32 = rawheap_translate::ByteToU32(bytes);
    if (rawheap_translate::IsLittleEndian()) {
        ASSERT_TRUE(u32 == 0x12345678);
    } else {
        ASSERT_TRUE(u32 == 0x78563412);
    }
}
}  // namespace panda::test
