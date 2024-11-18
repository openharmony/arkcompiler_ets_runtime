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
#include "ecmascript/dfx/hprof/rawheap_translate/utils.h"

namespace rawheap_translate {
struct MetaData;
using JSType = uint8_t;
using NodeType = uint8_t;
using ObjRangeVisitor = std::function<void(std::shared_ptr<MetaData> &, int)>;

struct Field {
    std::string name;
    int offset;
    int size;

    Field() : offset(0), size(0) {}
};

struct MetaData {
    std::string name;
    std::vector<std::shared_ptr<Field>> fields;
    int endOffset;
    std::vector<std::string> parents;
    std::string visitType;

    MetaData() : endOffset(0) {}

    bool IsArray()
    {
        return visitType == "Array";
    }
};

struct Layout {
    std::string name;
    int keyIndex;
    int valueIndex;
    int detailIndex;
    int entrySize;
    int headerSize;

    Layout() : keyIndex(0), valueIndex(0), detailIndex(0), entrySize(0), headerSize(0) {}
};

class Meta {
public:
    Meta() = default;
    ~Meta()
    {
        metadata_.clear();
        enumsMapJSType_.clear();
        enumsMapNodeType_.clear();
        layout_.clear();
        typeDesc_.clear();
        enumsVec_.clear();
    }

    bool Parse(const cJSON *object);
    void VisitObjectBody(const std::string &name, const ObjRangeVisitor &visitor, uint32_t &baseOffset);
    uint32_t GetNativateSize(char *obj, char *hclass);
    NodeType GetNodeTypeFromHClass(char *hclass);
    std::string GetTypeNameFromHClass(char *hclass);
    std::shared_ptr<Layout> GetObjectLayout(const std::string &name);
    bool IsString(char *hclass);
    bool IsDictionaryMode(char *hclass);
    bool IsJSObject(char *hclass);
    bool IsGlobalEnv(char *hclass);

private:
    bool ParseTypeEnums(const cJSON *json);
    bool ParseTypeList(const cJSON *json);
    bool ParseTypeLayout(const cJSON *json);
    bool ParseVersion(const cJSON *json);
    bool SetObjTypeBitFieldOffset();
    bool SetNativatePointerBindingSizeOffset();
    std::shared_ptr<MetaData> GetMetaData(const std::string &name);
    JSType GetObjTypeFromHClass(char *hclass);
    JSType GetObjTypeFromTypeName(const std::string &name);
    std::string GetTypeDesc(const std::string &name);

    static void IterateJSONArray(const cJSON *array, const std::function<void(const cJSON *, int)> &visitor);
    static bool GetArray(const cJSON *json, const char *key, cJSON **value);
    static bool GetString(const cJSON *json, const char *key, std::string &value);
    static bool GetString(const cJSON *json, std::string &value);
    static bool GetInt(const cJSON *json, const char *key, int &value);
    static bool GetInt(const cJSON *json, int &value);

    static constexpr NodeType DEFAULT_NODETYPE = 8;

    std::unordered_map<std::string, std::shared_ptr<MetaData>> metadata_ {};
    std::unordered_map<std::string, JSType> enumsMapJSType_ {};
    std::unordered_map<std::string, uint8_t> enumsMapNodeType_ {};
    std::unordered_map<std::string, std::shared_ptr<Layout>> layout_ {};
    std::unordered_map<std::string, std::string> typeDesc_ {};
    std::vector<std::string> enumsVec_ {};
    int objTypeBitFieldOffset_ {0};
    int objTypeBitFieldSize_ {0};
    int nativatePointerBindingSizeOffset_ {0};
    int nativatePointerBindingSize_ {0};
};
}  // namespace rawheap_translate
#endif  // METADATA_JSON_PARSE_H
