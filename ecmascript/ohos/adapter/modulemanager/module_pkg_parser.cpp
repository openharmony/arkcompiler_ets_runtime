/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "ecmascript/ohos/adapter/modulemanager/module_pkg_parser.h"

namespace panda::ecmascript::ohos {

static const std::string PACKAGE_NAME = "packageName";
static const std::string BUNDLE_NAME = "bundleName";
static const std::string MODULE_NAME = "moduleName";
static const std::string VERSION = "version";
static const std::string ENTRY_PATH = "entryPath";
static const std::string IS_SO = "isSO";
static const std::string DEPENDENCY_ALIAS = "dependencyAlias";
static const std::string OH_EXPORT = "oh-exports";

bool ModulePkgParser::ParseModulePkgJson(const EcmaVM *vm, const std::unordered_map<std::string,
    std::pair<std::unique_ptr<uint8_t[]>, size_t>> &pkgInfoMap,
    CMap<CString, CMap<CString, CVector<CString>>> &pkgContextInfoList,
    CMap<CString, CString> &pkgAliasList,
    CUnorderedMap<CString, CUnorderedMap<CString, CUnorderedSet<CString>>> &ohExportsMap)
{
#ifndef CROSS_PLATFORM
    for (auto it = pkgInfoMap.begin(); it != pkgInfoMap.end(); ++it) {
        CString moduleName(it->first);
        const std::unique_ptr<uint8_t[]> &data = it->second.first;
        size_t dataLen = it->second.second;
        CMap<CString, CVector<CString>> modulePkgInfo;
        CUnorderedMap<CString, CUnorderedSet<CString>> moduleOhExportsMap;
        if (!ParsePkgContextInfoJson(vm, moduleName, data.get(), dataLen, modulePkgInfo, pkgAliasList,
            moduleOhExportsMap)) {
            LOG_ECMA(ERROR) << "ModulePkgParser::ParseModulePkgJson parse module context info failed, moduleName is "
                            << moduleName;
            return false;
        }
        pkgContextInfoList.emplace(moduleName, modulePkgInfo);
        ohExportsMap.emplace(moduleName, moduleOhExportsMap);
    }
#endif
    return true;
}

#ifndef CROSS_PLATFORM
bool ModulePkgParser::ParsePkgContextInfoJson(const EcmaVM *vm, const CString &moduleName, const uint8_t *data,
    size_t dataLen, CMap<CString, CVector<CString>> &modulePkgInfo, CMap<CString, CString> &pkgAliasList,
    CUnorderedMap<CString, CUnorderedSet<CString>> &moduleOhExportsMap)
{
    auto jsonObject = nlohmann::json::parse(data, data + dataLen, nullptr, false);
    if (jsonObject.is_discarded()) {
        LOG_ECMA(ERROR) << "ModulePkgParser::ParseModulePkgJson parse json failed, moduleName is "
                        << moduleName;
        return false;
    }
    for (auto jsonIt = jsonObject.begin(); jsonIt != jsonObject.end(); ++jsonIt) {
        CVector<CString> items;
        CUnorderedSet<CString> ohExportsSet;
        nlohmann::json itemObject = jsonIt.value();
        if (!itemObject.is_object()) {
            LOG_ECMA(ERROR) << "ModulePkgParser::ParseModulePkgJson item is not object, moduleName is "
                            << moduleName;
            return false;
        }
        ParsePkgContextInfoItemJson(itemObject, PACKAGE_NAME, items);
        ParsePkgContextInfoItemJson(itemObject, BUNDLE_NAME, items);
        ParsePkgContextInfoItemJson(itemObject, MODULE_NAME, items);
        ParsePkgContextInfoItemJson(itemObject, VERSION, items);
        ParsePkgContextInfoItemJson(itemObject, ENTRY_PATH, items);
        ParsePkgContextInfoSoJson(itemObject, items);

        CString packageName(jsonIt.key());
        ParsePkgContextInfoAliasJson(itemObject, packageName, pkgAliasList);
        modulePkgInfo.emplace(packageName, items);
        if (ParsePkgContextInfoOhExports(itemObject, ohExportsSet)) {
            moduleOhExportsMap.emplace(packageName, ohExportsSet);
        }
    }
    return true;
}

void ModulePkgParser::ParsePkgContextInfoItemJson(const nlohmann::json &itemObject, const std::string &key,
    CVector<CString> &items)
{
    CString keyStr(key);
    items.emplace_back(keyStr);
    if (itemObject[key].is_null() || !itemObject[key].is_string()) {
        items.emplace_back("");
    } else {
        CString valueStr(itemObject[key].get<std::string>());
        items.emplace_back(valueStr);
    }
}

void ModulePkgParser::ParsePkgContextInfoSoJson(const nlohmann::json &itemObject, CVector<CString> &items)
{
    items.emplace_back(IS_SO);
    if (itemObject[IS_SO].is_null() || !itemObject[IS_SO].is_boolean()) {
        items.emplace_back("false");
    } else {
        bool isSo = itemObject[IS_SO].get<bool>();
        if (isSo) {
            items.emplace_back("true");
        } else {
            items.emplace_back("false");
        }
    }
}

void ModulePkgParser::ParsePkgContextInfoAliasJson(const nlohmann::json &itemObject, const CString &packageName,
    CMap<CString, CString> &pkgAliasList)
{
    if (!itemObject.is_null() && itemObject[DEPENDENCY_ALIAS].is_string()) {
        std::string pkgAlias = itemObject[DEPENDENCY_ALIAS].get<std::string>();
        if (!pkgAlias.empty()) {
            CString pkgAliasStr(pkgAlias);
            pkgAliasList[pkgAliasStr] = packageName;
        }
    }
}

bool ModulePkgParser::ParsePkgContextInfoOhExports(const nlohmann::json &itemObject, CUnorderedSet<CString> &result)
{
    if (!itemObject.contains(OH_EXPORT) || itemObject[OH_EXPORT].is_null() || !itemObject[OH_EXPORT].is_object()) {
        return false;
    }
    const auto& ohExportsJson = itemObject[OH_EXPORT];
    for (auto jsonIt = ohExportsJson.begin(); jsonIt != ohExportsJson.end(); jsonIt++) {
        if (jsonIt.value().is_string()) {
            CString ohmurl(jsonIt.value().get<std::string>());
            result.emplace(ohmurl);
        }
    }
    return true;
}
#endif
}  // namespace panda::ecmascript::ohos