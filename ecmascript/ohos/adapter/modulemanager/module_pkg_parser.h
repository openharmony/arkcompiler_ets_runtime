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
#ifndef ECMASCRIPT_OHOS_ADAPTER_MODULEMANAGER_MODULE_PKG_PARSER_H
#define ECMASCRIPT_OHOS_ADAPTER_MODULEMANAGER_MODULE_PKG_PARSER_H

#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <unordered_map>

#include "ecmascript/ecma_vm.h"

namespace panda::ecmascript::ohos {
class ModulePkgParser {
public:
    static bool ParseModulePkgJson(const EcmaVM *vm, const std::unordered_map<std::string,
        std::pair<std::unique_ptr<uint8_t[]>, size_t>> &pkgInfoMap,
        CMap<CString, CMap<CString, CVector<CString>>> &pkgContextInfoList,
        CMap<CString, CString> &pkgAliasList,
        CUnorderedMap<CString, CUnorderedMap<CString, CUnorderedSet<CString>>> &ohExportsMap);
private:
    static bool ParsePkgContextInfoJson(const EcmaVM *vm, const CString &moduleName, const uint8_t *data,
        size_t dataLen, CMap<CString, CVector<CString>> &modulePkgInfo, CMap<CString, CString> &pkgAliasList,
        CUnorderedMap<CString, CUnorderedSet<CString>> &moduleOhExportsMap);
    static void ParsePkgContextInfoItemJson(const nlohmann::json &itemObject, const std::string &key,
        CVector<CString> &items);
    static void ParsePkgContextInfoSoJson(const nlohmann::json &itemObject, CVector<CString> &items);
    static void ParsePkgContextInfoAliasJson(const nlohmann::json &itemObject, const CString &packageName,
        CMap<CString, CString> &pkgAliasList);
    static bool ParsePkgContextInfoOhExports(const nlohmann::json &itemObject, CUnorderedSet<CString> &result);
};

}
#endif  // ECMASCRIPT_OHOS_ADAPTER_MODULEMANAGER_MODULE_PKG_PARSER_H