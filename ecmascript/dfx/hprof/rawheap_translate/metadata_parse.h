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

#ifndef METADATA_JSON_PARSE_H
#define METADATA_JSON_PARSE_H

#include "cJSON.h"
#include "ecmascript/dfx/hprof/rawheap_translate/common.h"
#include "ecmascript/dfx/hprof/rawheap_translate/utils.h"

namespace rawheap_translate {
class MetaParser {
public:
    MetaParser() = default;
    ~MetaParser()
    {
        for (auto &meta : orderedMeta_) {
            delete meta;
        }
        meta_.clear();
        orderedMeta_.clear();
    }

    bool Parse(const cJSON *object);
    JSType GetJSTypeFromHClass(Node *hclass);
    JSType GetJSTypeFromTypeName(const std::string &name);
    NodeType GetNodeType(JSType type);
    uint32_t GetNativateSize(Node *node, JSType type);
    std::string GetTypeName(JSType type);
    MetaData* GetMetaData(const std::string &name);
    MetaData* GetMetaData(const JSType type);
    bool IsNativePointer(JSType type);
    bool IsString(JSType type);
    bool IsDictionaryMode(JSType type);
    bool IsJSObject(JSType type);
    bool IsGlobalEnv(JSType type);
    bool IsArray(JSType type);

    BitField* GetBitField()
    {
        return &bitField_;
    }

    std::string GetMetaVersion()
    {
        return version_;
    }

private:
    bool ParseTypeEnums(const cJSON *json);
    bool ParseTypeList(const cJSON *json);
    bool ParseTypeLayoutAndDesc(const cJSON *json);
    bool ParseVersion(const cJSON *json);
    void ParseParents(const cJSON *array, MetaData *metadata);
    void ParseOffsets(const cJSON *array, MetaData *metadata);
    void SetBitField(const std::string &metaName, const std::string &fieldName, Field &field);
    void FillMetaData(MetaData *parent, MetaData *meta);
    void GenerateMetaData();
    MetaData* FindOrCreateMetaData(const std::string &name);

    static void IterateJSONArray(const cJSON *array, const std::function<void(const cJSON *)> &visitor);
    static bool GetArray(const cJSON *json, const char *key, cJSON **value);
    static bool GetString(const cJSON *json, const char *key, std::string &value);
    static bool GetString(const cJSON *json, std::string &value);
    static bool GetUInt32(const cJSON *json, const char *key, uint32_t &value);
    static bool GetUInt32(const cJSON *json, uint32_t &value);

    std::string version_;
    std::unordered_map<std::string, MetaData *> meta_ {};
    std::vector<MetaData *> orderedMeta_ {};
    DictionaryLayout dictionaryLayout_;
    TypeRange typeRange_;
    BitField bitField_;
};
}  // namespace rawheap_translate
#endif  // METADATA_JSON_PARSE_H
