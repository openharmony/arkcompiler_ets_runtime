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

#include "ecmascript/dfx/hprof/rawheap_translate/metadata_parse.h"

namespace rawheap_translate {
bool Meta::Parse(const cJSON *object)
{
    return ParseVersion(object) &&
           ParseTypeEnums(object) &&
           ParseTypeList(object) &&
           ParseTypeLayout(object) &&
           SetObjTypeBitFieldOffset() &&
           SetNativatePointerBindingSizeOffset();
}

uint32_t Meta::GetNativateSize(char *obj, char *hclass)
{
    JSType typeId = GetObjTypeFromHClass(hclass);
    if (typeId != GetObjTypeFromTypeName("JS_NATIVE_POINTER")) {
        return 0;
    }
    return ByteToU32(obj + nativatePointerBindingSizeOffset_);
}

std::string Meta::GetTypeNameFromHClass(char *hclass)
{
    JSType typeId = GetObjTypeFromHClass(hclass);
    if (typeId < enumsVec_.size()) {
        return enumsVec_[typeId];
    }
    LOG_ERROR("Meta::GetTypeNameFromTypeId: typeId out of range, typeId=" + std::to_string(typeId));
    return "UNKNOWN_TYPE";
}

std::shared_ptr<Layout> Meta::GetObjectLayout(const std::string &name)
{
    auto layout = layout_.find(name);
    if (layout != layout_.end()) {
        return layout->second;
    }
    return nullptr;
}

bool Meta::IsString(char *hclass)
{
    JSType typeId = GetObjTypeFromHClass(hclass);
    JSType stringFirst = GetObjTypeFromTypeName(GetTypeDesc("string_first"));
    JSType stringLast = GetObjTypeFromTypeName(GetTypeDesc("string_last"));
    if (stringFirst <= typeId && typeId <= stringLast) {
        return true;
    }
    return false;
}

bool Meta::IsDictionaryMode(char *hclass)
{
    return GetObjTypeFromHClass(hclass) == GetObjTypeFromTypeName("TAGGED_DICTIONARY");
}

bool Meta::IsJSObject(char *hclass)
{
    JSType typeId = GetObjTypeFromHClass(hclass);
    JSType stringFirst = GetObjTypeFromTypeName(GetTypeDesc("js_object_first"));
    JSType stringLast = GetObjTypeFromTypeName(GetTypeDesc("js_object_last"));
    if (stringFirst <= typeId && typeId <= stringLast) {
        return true;
    }
    return false;
}

bool Meta::IsGlobalEnv(char *hclass)
{
    return GetObjTypeFromHClass(hclass) == GetObjTypeFromTypeName("GLOBAL_ENV");
}

NodeType Meta::GetNodeTypeFromHClass(char *hclass)
{
    std::string name = GetTypeNameFromHClass(hclass);
    auto type = enumsMapNodeType_.find(name);
    if (type != enumsMapNodeType_.end()) {
        return type->second;
    }
    return DEFAULT_NODETYPE;  // 8: means DEFAULT node type
}

void Meta::VisitObjectBody(const std::string &name, const ObjRangeVisitor &visitor, uint32_t &baseOffset)
{
    auto metadata = GetMetaData(name);
    if (!metadata) {
        return;
    }

    for (const auto &parent : metadata->parents) {
        VisitObjectBody(parent, visitor, baseOffset);
    }

    visitor(metadata, baseOffset);
    baseOffset += metadata->endOffset;
}

bool Meta::SetObjTypeBitFieldOffset()
{
    auto visitor = [this] (std::shared_ptr<MetaData> &metadata, int offset) {
        for (const auto &field : metadata->fields) {
            if (field->name == "BitField") {
                objTypeBitFieldOffset_ = offset + field->offset;
                objTypeBitFieldSize_ = field->size;
            }
        }
    };
    uint32_t baseOffset = 0;
    VisitObjectBody("HCLASS", visitor, baseOffset);
    LOG_INFO("Meta::SetObjTypeBitFieldOffset: offset=" + std::to_string(objTypeBitFieldOffset_));
    return true;
}

bool Meta::SetNativatePointerBindingSizeOffset()
{
    auto visitor = [this] (std::shared_ptr<MetaData> &metadata, int offset) {
        for (const auto &field : metadata->fields) {
            if (field->name == "BindingSize") {
                nativatePointerBindingSizeOffset_ = offset + field->offset;
                nativatePointerBindingSize_ = field->size;
            }
        }
    };
    uint32_t baseOffset = 0;
    VisitObjectBody("JS_NATIVE_POINTER", visitor, baseOffset);
    LOG_INFO("Meta::SetNativatePointerBindingSizeOffset: offset=" + std::to_string(nativatePointerBindingSizeOffset_));
    return true;
}

JSType Meta::GetObjTypeFromHClass(char *hclass)
{
    return ByteToU32(hclass + objTypeBitFieldOffset_) & 0xFF;  // lower 8-bits of 32-bit value means JSType
}

JSType Meta::GetObjTypeFromTypeName(const std::string &name)
{
    auto typeEnum = enumsMapJSType_.find(name);
    if (typeEnum == enumsMapJSType_.end()) {
        return 0;
    }
    return typeEnum->second;
}

std::string Meta::GetTypeDesc(const std::string &name)
{
    auto result = typeDesc_.find(name);
    if (result != typeDesc_.end()) {
        return result->second;
    }
    return "UNKNOWN_TYPE";
}

bool Meta::ParseTypeEnums(const cJSON *json)
{
    cJSON *typeEnums = cJSON_GetObjectItem(json, "type_enum");
    auto visit = [&typeEnums, this] (const cJSON *item, int index) {
        std::string name = item->string;
        uint32_t edgeType = 0;
        if (GetUInt32(typeEnums, name.c_str(), edgeType)) {
            enumsVec_.push_back(name);
            enumsMapJSType_.emplace(name, static_cast<JSType>(index));
            enumsMapNodeType_.emplace(name, static_cast<uint8_t>(edgeType));
        }
    };
    IterateJSONArray(typeEnums, visit);
    LOG_INFO("Meta::ParseTypeEnums: parse type enums, size=" + std::to_string(enumsVec_.size()));
    return true;
}

bool Meta::ParseTypeList(const cJSON *json)
{
    cJSON *metadatas {nullptr};
    std::vector<std::string> *curParents {nullptr};
    std::vector<std::shared_ptr<Field>> *curField {nullptr};
    if (!GetArray(json, "type_list", &metadatas)) {
        LOG_ERROR("Meta::ParseTypeList: type list parse failed!");
        return false;
    }

    auto parentVisitor = [&curParents] (const cJSON *item, [[maybe_unused]] int index) {
        std::string value {};
        if (GetString(item, value)) {
            curParents->push_back(value);
        }
    };

    auto offsetVisitor = [&curField] (const cJSON *item, [[maybe_unused]] int index) {
        auto field = std::make_shared<Field>();
        if (GetUInt32(item, "offset", field->offset) &&
            GetString(item, "name", field->name) &&
            GetUInt32(item, "size", field->size)) {
            curField->push_back(field);
        }
    };

    auto metaVisitor =
        [&curParents, &curField, &parentVisitor, &offsetVisitor, this] (const cJSON *item, [[maybe_unused]] int index) {
        cJSON *fields {nullptr};
        cJSON *parents {nullptr};
        if (!GetArray(item, "offsets", &fields) || !GetArray(item, "parents", &parents)) {
            return;
        }

        auto metadata = std::make_shared<MetaData>();
        curParents = &metadata->parents;
        curField = &metadata->fields;
        IterateJSONArray(fields, offsetVisitor);
        IterateJSONArray(parents, parentVisitor);

        if (GetString(item, "name", metadata->name) && GetUInt32(item, "end_offset", metadata->endOffset)) {
            metadata_.emplace(metadata->name, metadata);
        }

        GetString(item, "visit_type", metadata->visitType);
    };

    IterateJSONArray(metadatas, metaVisitor);
    LOG_INFO("Meta::ParseTypeList: parse type list, obj size = " + std::to_string(metadata_.size()));
    return true;
}

bool Meta::ParseTypeLayout(const cJSON *json)
{
    cJSON *object = cJSON_GetObjectItem(json, "type_layout");

    cJSON *dictLayout = cJSON_GetObjectItem(object, "Dictionary_layout");
    auto layout = std::make_shared<Layout>();
    GetString(dictLayout, "name", layout->name);
    GetUInt32(dictLayout, "key_index", layout->keyIndex);
    GetUInt32(dictLayout, "value_index", layout->valueIndex);
    GetUInt32(dictLayout, "detail_index", layout->detailIndex);
    GetUInt32(dictLayout, "entry_size", layout->entrySize);
    GetUInt32(dictLayout, "header_size", layout->headerSize);
    layout_.emplace(layout->name, layout);
    LOG_INFO("Meta::ParseTypeLayout: parse type layout, size=" + std::to_string(layout_.size()));

    cJSON *typeRange = cJSON_GetObjectItem(object, "Type_range");
    auto visit = [&typeRange, this] (const cJSON *item, [[maybe_unused]] int index) {
        std::string key = item->string;
        std::string value {};
        if (GetString(typeRange, key.c_str(), value)) {
            typeDesc_.emplace(key, value);
        }
    };
    IterateJSONArray(typeRange, visit);
    LOG_INFO("Meta::ParseTypeLayout: parse type desc, size=" + std::to_string(typeDesc_.size()));
    return true;
}

bool Meta::ParseVersion(const cJSON *json)
{
    std::string version {};
    if (!GetString(json, "version", version)) {
        LOG_ERROR("Meta::ParseVersion: version not found!");
        return false;
    }

    if (!CheckVersion(version)) {
        return false;
    }
    
    LOG_INFO("Meta::ParseVersion: current metadata version is " + version);
    return true;
}
std::shared_ptr<MetaData> Meta::GetMetaData(const std::string &name)
{
    auto metadata = metadata_.find(name);
    if (metadata != metadata_.end()) {
        return metadata->second;
    }
    return nullptr;
}

void Meta::IterateJSONArray(const cJSON *array, const std::function<void(const cJSON *, int)> &visitor)
{
    int size = cJSON_GetArraySize(array);
    for (int i = 0; i < size; i++) {
        cJSON *item = cJSON_GetArrayItem(array, i);
        visitor(item, i);
    }
}

bool Meta::GetArray(const cJSON *json, const char *key, cJSON **value)
{
    cJSON *array = cJSON_GetObjectItem(json, key);
    if (array == nullptr || cJSON_IsArray(array) == 0) {
        return false;
    }
    *value = array;
    return true;
}

bool Meta::GetString(const cJSON *json, const char *key, std::string &value)
{
    cJSON *item = cJSON_GetObjectItem(json, key);
    if (item == nullptr) {
        return false;
    }
    return GetString(item, value);
}

bool Meta::GetString(const cJSON *json, std::string &value)
{
    if (cJSON_IsString(json) == 0) {
        return false;
    }
    value = json->valuestring;
    return true;
}

bool Meta::GetUInt32(const cJSON *json, const char *key, uint32_t &value)
{
    cJSON *item = cJSON_GetObjectItem(json, key);
    if (item == nullptr) {
        return false;
    }
    return GetUInt32(item, value);
}

bool Meta::GetUInt32(const cJSON *json, uint32_t &value)
{
    if (cJSON_IsNumber(json) == 0) {
        return false;
    }
    if (json->valuedouble < 0 || json->valuedouble > static_cast<double>(UINT32_MAX)) {
        return false;
    }
    value = static_cast<uint32_t>(json->valuedouble);
    return true;
}
}  // namespace rawheap_translate