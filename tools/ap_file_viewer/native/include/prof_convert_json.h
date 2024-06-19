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

#ifndef NATIVE_CONVERT_JSON_H
#define NATIVE_CONVERT_JSON_H

#include "ecmascript/log_wrapper.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "third_party/cJSON/cJSON.h"

namespace native {
using namespace panda::ecmascript::pgo;
class JsonConverter {
public:
    cJSON* ConvertStr(const std::vector<ProfileType::StringMap>& data) const
    {
        return HandleMapVector(data);
    }

    cJSON* Convert(const std::vector<ProfileType::jModuleType>& data) const
    {
        cJSON* jsonArray = cJSON_CreateArray();
        if (jsonArray == nullptr) {
            return nullptr;
        }
        for (const auto& complexMap : data) {
            cJSON* jsonObject = cJSON_CreateObject();
            if (jsonObject == nullptr) {
                return nullptr;
            }
            for (const auto& [key, value] : complexMap) {
                cJSON* jsonValue = HandleVariant(value);
                if (jsonValue == nullptr) {
                    return nullptr;
                }
                cJSON_AddItemToObject(jsonObject, key.c_str(), jsonValue);
            }
            cJSON_AddItemToArray(jsonArray, jsonObject);
        }
        return jsonArray;
    }

private:
    cJSON* HandleVariant(const std::variant<std::string, std::vector<ProfileType::VariantMap>>& value) const
    {
        if (std::holds_alternative<std::string>(value)) {
            return cJSON_CreateString(std::get<std::string>(value).c_str());
        } else if (std::holds_alternative<std::vector<ProfileType::VariantMap>>(value)) {
            return HandleVariantMap(std::get<std::vector<ProfileType::VariantMap>>(value));
        }
        return nullptr;
    }

    cJSON* HandleVariantMap(const std::vector<ProfileType::VariantMap>& vaMap) const
    {
        cJSON* jsonArray = cJSON_CreateArray();
        if (jsonArray == nullptr) {
            return nullptr;
        }
        for (const auto& nestedItem : vaMap) {
            cJSON* mapObject = cJSON_CreateObject();
            if (mapObject == nullptr) {
                return nullptr;
            }
            for (const auto& [key, value] : nestedItem) {
                cJSON* jsonValue = HandleVariantNext(value);
                if (jsonValue == nullptr) {
                    return nullptr;
                }
                cJSON_AddItemToObject(mapObject, key.c_str(), jsonValue);
            }
            cJSON_AddItemToArray(jsonArray, mapObject);
        }
        return jsonArray;
    }

    cJSON* HandleVariantNext(const std::variant<std::string, ProfileType::VariantVector>& value) const
    {
        if (std::holds_alternative<std::string>(value)) {
            return cJSON_CreateString(std::get<std::string>(value).c_str());
        } else if (std::holds_alternative<ProfileType::VariantVector>(value)) {
            return HandleVariantVector(std::get<ProfileType::VariantVector>(value));
        }
        return nullptr;
    }

    cJSON* HandleVariantVector(const ProfileType::VariantVector& vec) const
    {
        cJSON* jsonArray = cJSON_CreateArray();
        if (jsonArray == nullptr) {
            return nullptr;
        }
        for (const auto& nestedItem : vec) {
            if (std::holds_alternative<ProfileType::StringMap>(nestedItem)) {
                cJSON* mapObject = HandleStringMap(std::get<ProfileType::StringMap>(nestedItem));
                cJSON_AddItemToArray(jsonArray, mapObject);
            } else if (std::holds_alternative<ProfileType::MapVector>(nestedItem)) {
                cJSON* innerInnerArray = HandleNestedMapVector(std::get<ProfileType::MapVector>(nestedItem));
                cJSON_AddItemToArray(jsonArray, innerInnerArray);
            } else if (std::holds_alternative<std::vector<ProfileType::StringMap>>(nestedItem)) {
                cJSON* arrayOfMaps = HandleMapVector(std::get<std::vector<ProfileType::StringMap>>(nestedItem));
                cJSON_AddItemToArray(jsonArray, arrayOfMaps);
            }
        }
        return jsonArray;
    }

    cJSON* HandleStringMap(const ProfileType::StringMap& stringMap) const
    {
        cJSON* mapObject = cJSON_CreateObject();
        if (mapObject == nullptr) {
            return nullptr;
        }
        for (const auto& [mapKey, mapValue] : stringMap) {
            cJSON_AddStringToObject(mapObject, mapKey.c_str(), mapValue.c_str());
        }
        return mapObject;
    }

    cJSON* HandleMapVector(const std::vector<ProfileType::StringMap>& vecMap) const
    {
        cJSON* arrayOfMaps = cJSON_CreateArray();
        if (arrayOfMaps == nullptr) {
            return nullptr;
        }
        for (const auto& stringMap : vecMap) {
            cJSON* mapObject = HandleStringMap(stringMap);
            if (mapObject == nullptr) {
                return nullptr;
            }
            cJSON_AddItemToArray(arrayOfMaps, mapObject);
        }
        return arrayOfMaps;
    }

    cJSON* HandleNestedMapVector(const ProfileType::MapVector& vecVec) const
    {
        cJSON* innerInnerArray = cJSON_CreateArray();
        if (innerInnerArray == nullptr) {
            return nullptr;
        }
        for (const auto& vec1 : vecVec) {
            cJSON* innerArray = cJSON_CreateArray();
            if (innerArray == nullptr) {
                return nullptr;
            }
            for (const auto& stringMap : vec1) {
                cJSON* mapObject = HandleStringMap(stringMap);
                if (mapObject == nullptr) {
                    return nullptr;
                }
                cJSON_AddItemToArray(innerArray, mapObject);
            }
            cJSON_AddItemToArray(innerInnerArray, innerArray);
        }
        return innerInnerArray;
    }
};
} // namespace native
#endif // NATIVE_CONVERT_JSON_H
