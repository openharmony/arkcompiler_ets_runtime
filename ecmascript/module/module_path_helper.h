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
#ifndef ECMASCRIPT_MODULE_MODULE_PATH_HELPER_H
#define ECMASCRIPT_MODULE_MODULE_PATH_HELPER_H

#include "ecmascript/base/path_helper.h"

#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/jspandafile/js_pandafile.h"
/*
 * Intra-application cross hap:
 * baseFileName = 'data/storage/el1/bundle/moduleName/ets/modules.abc';
 * cross-application:
 * baseFileName = 'data/storage/el1/bundle/bundleName/moduleName/moduleName/ets/modules.abc';
 * recordName = bundleName/moduleName/xxx(entry)/xxx(ets)/xxx(pages)/xxx  specific abc file
 *
 * ohmUrl: It's an index information that can uniquely identify module files.
 * Current ohmUrl has the following five different prefixs:
 * 1. @bundle:... Identify OpenHarmony modules.
 *    {project_path}\entry\src\main\ets\pages\Index --> @bundle:bundleName/moduleName/ets/pages/Index
 *    @namespace: needs to add when import local har or ohosTest import entry file.
 *    {project_path}\namespace\src\main\ets\pages\Index --> @bundle:bundleName/moduleName@namespace/ets/pages/Index
 *
 * 2. @package:... Identify open source third party modules.
 *    {project_path}\node_modules.ohpm\pkgName\oh_modules\pkgName\xxx\xxx
 *    --> @package:pkg_modules/.ohpm/pkgName/pkg_modules/pkgName/xxx/xxx
 *
 * 3. @app:... Identify c++ modules in application.
 *    libxxx.so --> @app:bundleName/moduleName/xxx
 *
 * 4. @native:... Identify system builtin modules.
 *    system.app --> @native:system.app
 *
 * 5. @ohos:... Identify ohos builtin modules.
 *    @ohos:hilog --> @ohos:hilog
 */

namespace panda::ecmascript {
using PathHelper = base::PathHelper;
using StringHelper = base::StringHelper;

class ModulePathHelper {
public:
    static constexpr char EXT_NAME_ABC[] = ".abc";
    static constexpr char EXT_NAME_ETS[] = ".ets";
    static constexpr char EXT_NAME_TS[] = ".ts";
    static constexpr char EXT_NAME_JS[] = ".js";
    static constexpr char EXT_NAME_JSON[] = ".json";
    static constexpr char PREFIX_BUNDLE[] = "@bundle:";
    static constexpr char PREFIX_MODULE[] = "@module:";
    static constexpr char PREFIX_PACKAGE[] = "@package:";
    static constexpr char REQUIRE_NAITVE_MODULE_PREFIX[] = "@native:";
    static constexpr char REQUIRE_NAPI_OHOS_PREFIX[] = "@ohos:";
    static constexpr char REQUIRE_NAPI_APP_PREFIX[] = "@app:";
    static constexpr char NPM_PATH_SEGMENT[] = "node_modules";
    static constexpr char PACKAGE_PATH_SEGMENT[] = "pkg_modules";
    static constexpr char PACKAGE_ENTRY_FILE[] = "/index";
    static constexpr char BUNDLE_INSTALL_PATH[] = "/data/storage/el1/bundle/";
    static constexpr char MERGE_ABC_ETS_MODULES[] = "/ets/modules.abc";
    static constexpr char MODULE_DEFAULE_ETS[] = "/ets/";
    static constexpr char BUNDLE_SUB_INSTALL_PATH[] = "/data/storage/el1/";
    static constexpr char PREVIEW_OF_ACROSS_HAP_FLAG[] = "[preview]";
    static constexpr char PREVIER_TEST_DIR[] = ".test";

    static constexpr size_t MAX_PACKAGE_LEVEL = 1;
    static constexpr size_t SEGMENTS_LIMIT_TWO = 2;
    static constexpr size_t EXT_NAME_ABC_LEN = 4;
    static constexpr size_t EXT_NAME_ETS_LEN = 4;
    static constexpr size_t EXT_NAME_TS_LEN = 3;
    static constexpr size_t EXT_NAME_JS_LEN = 3;
    static constexpr size_t EXT_NAME_JSON_LEN = 5;
    static constexpr size_t PREFIX_BUNDLE_LEN = 8;
    static constexpr size_t PREFIX_MODULE_LEN = 8;
    static constexpr size_t PREFIX_PACKAGE_LEN = 9;
    static constexpr size_t NATIVE_PREFIX_SIZE = 8;
    static constexpr size_t OHOS_PREFIX_SIZE = 6;
    static constexpr size_t APP_PREFIX_SIZE = 5;

    static CString ConcatFileNameWithMerge(JSThread *thread, const JSPandaFile *jsPandaFile,
                                           CString &baseFileName, CString recordName, CString requestName);
    static void ParseOhmUrl(EcmaVM *vm, const CString &inputFileName,
                            CString &outBaseFileName, CString &outEntryPoint);
    static CString ParseUrl(EcmaVM *vm, const CString &recordName);
    static CString ParsePrefixBundle(JSThread *thread, const JSPandaFile *jsPandaFile,
        [[maybe_unused]] CString &baseFileName, CString moduleRequestName, [[maybe_unused]] CString recordName);
    static CString MakeNewRecord(const JSPandaFile *jsPandaFile, CString &baseFileName,
                                 const CString &recordName, const CString &requestName);
    static CString FindOhpmEntryPoint(const JSPandaFile *jsPandaFile, const CString &ohpmPath,
                                      const CString &requestName);
    static CString FindPackageInTopLevelWithNamespace(const JSPandaFile *jsPandaFile, const CString &requestName,
                                                      const CString &recordName);
    static CString ParseOhpmPackage(const JSPandaFile *jsPandaFile, const CString &recordName,
                                    const CString &requestName);
    static CString ParseThirdPartyPackage(const JSPandaFile *jsPandaFile, const CString &recordName,
                                          const CString &requestName, const CString &packagePath);
    static CString ParseThirdPartyPackage(const JSPandaFile *jsPandaFile, const CString &recordName,
                                          const CString &requestName);
    static void ResolveCurrentPath(JSThread *thread, JSMutableHandle<JSTaggedValue> &dirPath,
                                   JSMutableHandle<JSTaggedValue> &fileName, const JSPandaFile *jsPandaFile);
    static CString FindNpmEntryPoint(const JSPandaFile *jsPandaFile, const CString &packageEntryPoint);
    static CString FindPackageInTopLevel(const JSPandaFile *jsPandaFile, const CString &requestName,
                                         const CString &packagePath);
    static bool IsImportFile(const CString &moduleRequestName);
    static CString RemoveSuffix(const CString &requestName);

    /*
     * Before: data/storage/el1/bundle/moduleName/ets/modules.abc
     * After:  bundle/moduleName
     */
    inline static std::string ParseHapPath(const CString &baseFileName)
    {
        CString bundleSubInstallName(BUNDLE_SUB_INSTALL_PATH);
        size_t startStrLen = bundleSubInstallName.length();
        if (baseFileName.length() > startStrLen && baseFileName.compare(0, startStrLen, bundleSubInstallName) == 0) {
            CString hapPath = baseFileName.substr(startStrLen);
            size_t pos = hapPath.find(MERGE_ABC_ETS_MODULES);
            if (pos != CString::npos) {
                return hapPath.substr(0, pos).c_str();
            }
        }
        return std::string();
    }

    /*
     * Before: xxx
     * After:  xxx || xxx/index
     */
    inline static CString ConfirmLoadingIndexOrNot(const JSPandaFile *jsPandaFile, const CString &packageEntryPoint)
    {
        CString entryPoint = packageEntryPoint;
        if (jsPandaFile->HasRecord(entryPoint)) {
            return entryPoint;
        }
        // Possible import directory
        entryPoint += PACKAGE_ENTRY_FILE;
        if (jsPandaFile->HasRecord(entryPoint)) {
            return entryPoint;
        }
        return CString();
    }

    inline static bool IsNativeModuleRequest(const CString &requestName)
    {
        if (requestName[0] != PathHelper::NAME_SPACE_TAG) {
            return false;
        }
        if (StringHelper::StringStartWith(requestName, ModulePathHelper::REQUIRE_NAPI_OHOS_PREFIX) ||
            StringHelper::StringStartWith(requestName, ModulePathHelper::REQUIRE_NAPI_APP_PREFIX) ||
            StringHelper::StringStartWith(requestName, ModulePathHelper::REQUIRE_NAITVE_MODULE_PREFIX)) {
            return true;
        }
        return false;
    }
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_MODULE_MODULE_PATH_HELPER_H