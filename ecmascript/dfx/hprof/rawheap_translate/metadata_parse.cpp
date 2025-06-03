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
bool MetaParser::Parse(const cJSON *object)
{
    if (!ParseVersion(object) || !ParseTypeEnums(object) || !ParseTypeList(object) ||
        !ParseTypeLayoutAndDesc(object)) {
        return false;
    }

    GenerateMetaData();
    SetBitField("HCLASS", "BitField", bitField_.objectTypeField);
    SetBitField("JS_NATIVE_POINTER", "BindingSize", bitField_.nativePointerBindingSizeField);
    SetBitField("TAGGED_ARRAY", "Length", bitField_.taggedArrayLengthField);
    SetBitField("TAGGED_ARRAY", "Data", bitField_.taggedArrayDataField);
    return true;
}

JSType MetaParser::GetJSTypeFromHClass(Node *hclass)
{
    JSType type = static_cast<JSType>(ByteToU32(hclass->data + bitField_.objectTypeField.offset));
    if (type < orderedMeta_.size()) {
        // lower 8-bits of 32-bit value means JSType
        return type;
    }
    LOG_ERROR_ << "js type error, " << (int)type;
    return 0;
}

JSType MetaParser::GetJSTypeFromTypeName(const std::string &name)
{
    MetaData *meta = GetMetaData(name);
    if (meta == nullptr) {
        return 0;  // 0: INVALID type
    }
    return meta->type;
}

NodeType MetaParser::GetNodeType(JSType type)
{
    MetaData *meta = GetMetaData(type);
    if (meta == nullptr) {
        return DEFAULT_NODETYPE;
    }
    return meta->nodeType;
}

uint32_t MetaParser::GetNativateSize(Node *node, JSType type)
{
    if (!IsNativePointer(type)) {
        return 0;
    }
    return ByteToU32(node->data + bitField_.nativePointerBindingSizeField.offset);
}

std::string MetaParser::GetTypeName(JSType type)
{
    MetaData *meta = GetMetaData(type);
    if (meta != nullptr) {
        return meta->name;
    }
    return "UNKNOWN_TYPE";
}

MetaData* MetaParser::GetMetaData(const std::string &name)
{
    auto meta = meta_.find(name);
    if (meta != meta_.end()) {
        return meta->second;
    }
    return nullptr;
}

MetaData *MetaParser::GetMetaData(const JSType type)
{
    if (type < orderedMeta_.size()) {
        return orderedMeta_[type];
    }
    return nullptr;
}

bool MetaParser::IsNativePointer(JSType type)
{
    return type == GetJSTypeFromTypeName("JS_NATIVE_POINTER");
}

bool MetaParser::IsString(JSType type)
{
    return typeRange_.stringFirst <= type && type <= typeRange_.stringLast;
}

bool MetaParser::IsDictionaryMode(JSType type)
{
    return type == GetJSTypeFromTypeName("TAGGED_DICTIONARY");
}

bool MetaParser::IsJSObject(JSType type)
{
    return typeRange_.objectFirst <= type && type <= typeRange_.objectLast;
}

bool MetaParser::IsGlobalEnv(JSType type)
{
    return type == GetJSTypeFromTypeName("GLOBAL_ENV");
}

bool MetaParser::IsArray(JSType type)
{
    MetaData *meta = GetMetaData(type);
    if (meta != nullptr && meta->IsArray()) {
        return true;
    }
    return false;
}

bool MetaParser::ParseTypeEnums(const cJSON *json)
{
    cJSON *typeEnums = cJSON_GetObjectItem(json, "type_enum");
    JSType type = 0;
    auto visitor = [&type, this] (const cJSON *item) {
        if (item->type == cJSON_Number) {
            MetaData *meta = FindOrCreateMetaData(item->string);
            // valuedouble: The item's number, if type==cJSON_Number
            meta->nodeType = static_cast<NodeType>(item->valuedouble);
            meta->type = type++;
            orderedMeta_.push_back(meta);
        }
    };
    IterateJSONArray(typeEnums, visitor);
    LOG_INFO_ << "total JSType count " << orderedMeta_.size();
    return true;
}

bool MetaParser::ParseTypeList(const cJSON *json)
{
    cJSON *metadatas {nullptr};
    if (!GetArray(json, "type_list", &metadatas)) {
        LOG_ERROR_ << "get type list item failed!";
        return false;
    }

    auto metaVisitor = [this](const cJSON *item) {
        std::string name;
        GetString(item, "name", name);
        MetaData *meta = FindOrCreateMetaData(name);
        GetString(item, "visit_type", meta->visitType);
        GetUInt32(item, "end_offset", meta->endOffset);

        cJSON *parents {nullptr};
        cJSON *offsets {nullptr};
        if (GetArray(item, "parents", &parents) && GetArray(item, "offsets", &offsets)) {
            ParseParents(parents, meta);
            ParseOffsets(offsets, meta);
        }
    };

    IterateJSONArray(metadatas, metaVisitor);
    LOG_INFO_ << "total metadata count " << meta_.size();
    return true;
}

bool MetaParser::ParseTypeLayoutAndDesc(const cJSON *json)
{
    cJSON *object = cJSON_GetObjectItem(json, "type_layout");
    cJSON *dictLayout = cJSON_GetObjectItem(object, "Dictionary_layout");
    GetUInt32(dictLayout, "key_index", dictionaryLayout_.keyIndex);
    GetUInt32(dictLayout, "value_index", dictionaryLayout_.valueIndex);
    GetUInt32(dictLayout, "detail_index", dictionaryLayout_.detailIndex);
    GetUInt32(dictLayout, "entry_size", dictionaryLayout_.entrySize);
    GetUInt32(dictLayout, "header_size", dictionaryLayout_.headerSize);

    cJSON *typeRange = cJSON_GetObjectItem(object, "Type_range");
    std::string name;
    GetString(typeRange, "string_first", name);
    typeRange_.stringFirst = GetJSTypeFromTypeName(name);
    GetString(typeRange, "string_last", name);
    typeRange_.stringLast = GetJSTypeFromTypeName(name);
    GetString(typeRange, "js_object_first", name);
    typeRange_.objectFirst = GetJSTypeFromTypeName(name);
    GetString(typeRange, "js_object_last", name);
    typeRange_.objectLast = GetJSTypeFromTypeName(name);
    return true;
}

bool MetaParser::ParseVersion(const cJSON *json)
{
    std::string versionId;
    if (!GetString(json, "version", versionId)) {
        LOG_ERROR_ << "version not found!";
        return false;
    }

    if (!version_.Parse(versionId)) {
        return false;
    }

    LOG_INFO_ << "current meta version is " << version_.ToString();
    return true;
}

void MetaParser::ParseParents(const cJSON *array, MetaData *meta)
{
    if (cJSON_GetArraySize(array) <= 0) {
        return;
    }
    cJSON *item = cJSON_GetArrayItem(array, 0);
    if (item->type == cJSON_String) {
        meta->parent = FindOrCreateMetaData(item->valuestring);
    }
}

void MetaParser::ParseOffsets(const cJSON *array, MetaData *meta)
{
    auto visitor = [&meta](const cJSON *item) {
        Field field;
        GetString(item, "name", field.name);
        GetUInt32(item, "offset", field.offset);
        GetUInt32(item, "size", field.size);
        meta->fields.push_back(field);
    };
    IterateJSONArray(array, visitor);
}

void MetaParser::SetBitField(const std::string &metaName, const std::string &fieldName, Field &field)
{
    MetaData *meta = GetMetaData(metaName);
    if (meta == nullptr) {
        return;
    }

    for (const auto &field_ : meta->fields) {
        if (field_.name == fieldName) {
            field = field_;
            break;
        }
    }

    LOG_INFO_ << "set " << fieldName << " offset " << field.offset;
}

void MetaParser::FillMetaData(MetaData *parent, MetaData *meta)
{
    if (parent->parent != nullptr) {
        FillMetaData(parent->parent, meta);
    }
    
    for (const auto &field : parent->fields) {
        meta->fields.push_back({field.name, field.offset + meta->endOffset, field.size});
    }

    meta->endOffset += parent->endOffset;

    if (parent->IsArray()) {
        meta->visitType = parent->visitType;
    }
}

void MetaParser::GenerateMetaData()
{
    std::unordered_map<std::string, MetaData *> newMeta {};
    for (auto &it : meta_) {
        MetaData *meta = new MetaData();
        newMeta.emplace(it.first, meta);
        FillMetaData(it.second, meta);
    }

    for (auto &it : meta_) {
        MetaData *meta = newMeta[it.first];
        it.second->fields.swap(meta->fields);
        it.second->visitType = meta->visitType;
        it.second->endOffset = meta->endOffset;
        delete meta;
    }

    newMeta.clear();
}

MetaData *MetaParser::FindOrCreateMetaData(const std::string &name)
{
    MetaData *meta = GetMetaData(name);
    if (meta == nullptr) {
        meta = new MetaData();
        meta->name = name;
        meta_[name] = meta;
    }
    return meta;
}

void MetaParser::IterateJSONArray(const cJSON *array, const std::function<void(const cJSON *)> &visitor)
{
    int size = cJSON_GetArraySize(array);
    for (int i = 0; i < size; i++) {
        cJSON *item = cJSON_GetArrayItem(array, i);
        visitor(item);
    }
}

bool MetaParser::GetArray(const cJSON *json, const char *key, cJSON **value)
{
    cJSON *array = cJSON_GetObjectItem(json, key);
    if (array == nullptr || cJSON_IsArray(array) == 0) {
        return false;
    }
    *value = array;
    return true;
}

bool MetaParser::GetString(const cJSON *json, const char *key, std::string &value)
{
    cJSON *item = cJSON_GetObjectItem(json, key);
    if (item == nullptr) {
        return false;
    }
    return GetString(item, value);
}

bool MetaParser::GetString(const cJSON *json, std::string &value)
{
    if (cJSON_IsString(json) == 0) {
        return false;
    }
    value = json->valuestring;
    return true;
}

bool MetaParser::GetUInt32(const cJSON *json, const char *key, uint32_t &value)
{
    cJSON *item = cJSON_GetObjectItem(json, key);
    if (item == nullptr) {
        return false;
    }
    return GetUInt32(item, value);
}

bool MetaParser::GetUInt32(const cJSON *json, uint32_t &value)
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