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

enum ValidateFilePath { ABC, ETS_MODULES };

class ModulePathHelper {
public:
    static constexpr char EXT_NAME_ABC[] = ".abc";
    static constexpr char EXT_NAME_ETS[] = ".ets";
    static constexpr char EXT_NAME_TS[] = ".ts";
    static constexpr char EXT_NAME_JS[] = ".js";
    static constexpr char EXT_NAME_JSON[] = ".json";
    static constexpr char EXT_NAME_Z_SO[] = ".z.so";
    static constexpr char EXT_NAME_D_TS[] = ".d.ts";
    static constexpr char EXT_NAME_MJS[] = ".mjs";
    static constexpr char EXT_NAME_HQF[] = ".hqf";
    static constexpr char PREFIX_NORMALIZED[] = "@normalized:";
    static constexpr char PREFIX_NORMALIZED_SO[] = "@normalized:Y";
    static constexpr char PREFIX_NORMALIZED_NOT_SO[] = "@normalized:N";
    static constexpr char PREFIX_NORMALIZED_NOT_CROSS_HAP_FILE[] = "@normalized:N&&&";
    static constexpr char PREFIX_BUNDLE[] = "@bundle:";
    static constexpr char PREFIX_MODULE[] = "@module:";
    static constexpr char PREFIX_PACKAGE[] = "@package:";
    static constexpr char PREFIX_ETS[] = "ets/";
    static constexpr char PREFIX_LIB[] = "lib";
    static constexpr char REQUIRE_NAITVE_MODULE_PREFIX[] = "@native:";
    static constexpr char REQUIRE_NAPI_OHOS_PREFIX[] = "@ohos:";
    static constexpr char REQUIRE_NAPI_APP_PREFIX[] = "@app:";
    static constexpr char RAW_ARKUIX_PREFIX[] = "@arkui-x.";
    static constexpr char NPM_PATH_SEGMENT[] = "node_modules";
    static constexpr char PACKAGE_PATH_SEGMENT[] = "pkg_modules";
    static constexpr char PACKAGE_ENTRY_FILE[] = "/index";
    static constexpr char BUNDLE_INSTALL_PATH[] = "/data/storage/el1/bundle/";
    static constexpr char MERGE_ABC_ETS_MODULES[] = "/ets/modules.abc";
    static constexpr char ABC[] = ".abc";
    static constexpr char MODULE_DEFAULE_ETS[] = "/ets/";
    static constexpr char BUNDLE_SUB_INSTALL_PATH[] = "/data/storage/el1/";
    static constexpr char PREVIEW_OF_ACROSS_HAP_FLAG[] = "[preview]";
    static constexpr char PREVIER_TEST_DIR[] = ".test";
    static constexpr char PHYCICAL_FILE_PATH[] = "/src/main";
    static constexpr char VMA_NAME_ARKTS_CODE[] = "ArkTS Code";
    static constexpr char ENTRY_MAIN_FUNCTION[] = "_GLOBAL::func_main_0";
    static constexpr char ENTRY_FUNCTION_NAME[] = "func_main_0";
    static constexpr char TRUE_FLAG[] = "true";

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
    static constexpr size_t PREFIX_NORMALIZED_LEN = 15;
    static constexpr size_t PACKAGE_PATH_SEGMENT_LEN = 11;
    static constexpr size_t NATIVE_PREFIX_SIZE = 8;
    static constexpr size_t OHOS_PREFIX_SIZE = 6;
    static constexpr size_t APP_PREFIX_SIZE = 5;
    static constexpr size_t BUNDLE_INSTALL_PATH_LEN = 25;
    static constexpr size_t PHYCICAL_FILE_PATH_LEN = 10;
    static constexpr size_t NORMALIZED_OHMURL_ARGS_NUM = 5;
    static constexpr size_t NORMALIZED_MODULE_NAME_INDEX = 1;
    static constexpr size_t NORMALIZED_BUNDLE_NAME_INDEX = 2;
    static constexpr size_t NORMALIZED_IMPORT_PATH_INDEX = 3;
    static constexpr size_t NORMALIZED_VERSION_INDEX = 4;
    static constexpr size_t CURRENT_DIREATORY_TAG_LEN = 2;
    static constexpr size_t SO_PREFIX_LEN = 3;
    static constexpr size_t SO_SUFFIX_LEN = 3;

    static constexpr size_t PKGINFO_PACKAGE_NAME_INDEX = 1;
    static constexpr size_t PKGINFO_BUDNLE_NAME_INDEX = 3;
    static constexpr size_t PKGINFO_MODULE_NAME_INDEX = 5;
    static constexpr size_t PKGINFO_VERSION_INDEX = 7;
    static constexpr size_t PKGINFO_ENTRY_PATH_INDEX = 9;
    static constexpr size_t PKGINFO_IS_SO_INDEX = 11;

    static CString PUBLIC_API ConcatFileNameWithMerge(JSThread *thread, const JSPandaFile *jsPandaFile,
        CString &baseFileName, const CString &recordName, const CString &requestName);
    static void ParseAbcPathAndOhmUrl(EcmaVM *vm, const CString &inputFileName, CString &outBaseFileName,
                                      CString &outEntryPoint);
    static CString ConcatUnifiedOhmUrl(const CString &bundleName, const CString &pkgname, const CString &entryPath,
                                       const CString &path, const CString &version);
    static CString ConcatUnifiedOhmUrl(const CString &bundleName, const CString &normalizedpath,
        const CString &version);
    static CString ConcatPreviewTestUnifiedOhmUrl(const CString &bundleName, const CString &pkgname,
        const CString &path, const CString &version);
    static CString ConcatHspFileNameCrossBundle(const CString &bundleName, const CString &moduleName);
    static CString ConcatHspFileName(const CString &moduleName);
    static CString ThrowInvalidOhmurlError(EcmaVM *vm, const CString &oldEntryPoint);
    static CString TransformToNormalizedOhmUrl(EcmaVM *vm, const CString &inputFileName, const CString &baseFileName,
        const CString &oldEntryPoint);
    static CString ParseUrl(EcmaVM *vm, const CString &recordName);
    static CString ParsePrefixBundle(JSThread *thread, const JSPandaFile *jsPandaFile,
        [[maybe_unused]] CString &baseFileName, CString moduleRequestName, [[maybe_unused]] CString recordName);
    static CString ParseNormalizedOhmUrl(JSThread *thread, CString &baseFileName, const CString &recordName,
                                         CString requestName);
    static CString MakeNewRecord(JSThread *thread, const JSPandaFile *jsPandaFile, CString &baseFileName,
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
    static void ResolveCurrentPath(CString &dirPath, CString &fileName, const JSPandaFile *jsPandaFile);
    static CString FindNpmEntryPoint(const JSPandaFile *jsPandaFile, const CString &packageEntryPoint);
    static CString FindPackageInTopLevel(const JSPandaFile *jsPandaFile, const CString &requestName,
                                         const CString &packagePath);
    static bool IsImportFile(const CString &moduleRequestName);
    static CString RemoveSuffix(const CString &requestName);
    static bool NeedTranstale(const CString &requestName);
    static bool NeedTranslateToNormalized(const CString &requestName);
    static void TranstaleExpressionInput(const JSPandaFile *jsPandaFile, CString &requestPath);
    static CString GetModuleNameWithBaseFile(const CString &baseFileName);
    static CString TranslateExpressionInputWithEts(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                   CString &baseFileName, const CString &requestName);
    static void ParseCrossModuleFile(const JSPandaFile *jsPandaFile, CString &requestPath);
    static CString ReformatPath(CString requestName);
    static CString TranslateExpressionToNormalized(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                   [[maybe_unused]] CString &baseFileName, const CString &recordName,
                                                   CString &requestPath);
    static CVector<CString> GetPkgContextInfoListElements(EcmaVM *vm, CString &moduleName,
                                                          CString &packageName);
    static CString TranslateNapiFileRequestPath(JSThread *thread, const CString &modulePath,
                                                const CString &requestName);
    static CVector<CString> SplitNormalizedOhmurl(const CString &ohmurl);
    static CString ConcatImportFileNormalizedOhmurl(const CString &recordPath, const CString &requestName,
                                                    const CString &version = "");
    static CString ConcatNativeSoNormalizedOhmurl(const CString &moduleName, const CString &bundleName,
                                                  const CString &pkgName, const CString &version);
    static CString ConcatNotSoNormalizedOhmurl(const CString &moduleName, const CString &bundleName,
                                               const CString &pkgName, const CString &entryPath,
                                               const CString &version);
    static CString ConcatMergeFileNameToNormalized(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                   CString &baseFileName, const CString &recordName,
                                                   CString requestName);
    static CVector<CString> SplitNormalizedRecordName(const CString &recordName);
    static CString ConcatImportFileNormalizedOhmurlWithRecordName(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                                  CString &baseFileName, const CString &recordName,
                                                                  const CString &requestName);
    static void ConcatOtherNormalizedOhmurl(EcmaVM *vm, const JSPandaFile *jsPandaFile,
                                            [[maybe_unused]] CString &baseFileName, CString &requestPath);
    static CString ConcatNormalizedOhmurlWithData(CVector<CString> &data, CString &pkgName, CString &entryPath);
    static CString GetBundleNameWithRecordName(EcmaVM *vm, const CString &recordName);
    static CString Utf8ConvertToString(JSTaggedValue str);

    static CString ParseFileNameToVMAName(const CString &filename);
    static CString ConcatOtherNormalizedOhmurlWithFilePath(EcmaVM *vm, size_t filePathPos, CString &moduleName,
                                                           const CString &requestPath);
    static bool IsOhmUrl(const CString &str);
    static bool CheckAndGetRecordName(JSThread *thread, const CString &ohmUrl, CString &recordName);
    static bool ValidateAbcPath(const CString &baseFileName, ValidateFilePath checkMode);
    /*
     * Before: /data/storage/el1/bundle/moduleName/ets/modules.abc
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
        entryPoint = PathHelper::NormalizePath(entryPoint);
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

    /*
     * Before: bundleName/moduleName/ets/xxx/xxx
     * After:  moduleName
     */
    inline static CString GetModuleName(const CString recordName)
    {
        size_t pos1 = recordName.find(PathHelper::SLASH_TAG);
        if (pos1 != CString::npos) {
            pos1++;
            size_t pos2 = recordName.find(PathHelper::SLASH_TAG, pos1);
            if (pos2 != CString::npos) {
                CString moduleName = recordName.substr(pos1, pos2 - pos1);
                PathHelper::DeleteNamespace(moduleName);
                return moduleName;
            }
        }
        return CString();
    }

    /*
     * Before: &moduleName/src/xxx
     * After:  moduleName
     */
    inline static CString GetModuleNameWithNormalizedName(const CString recordName)
    {
        size_t pos1 = recordName.find(PathHelper::NORMALIZED_OHMURL_TAG);
        if (pos1 != CString::npos) {
            pos1++;
            size_t pos2 = recordName.find(PathHelper::SLASH_TAG, pos1);
            if (pos2 != CString::npos) {
                CString moduleName = recordName.substr(pos1, pos2 - pos1);
                return moduleName;
            }
        }
        return CString();
    }

    /*
     * Before: bundleName/moduleName
     * After:  moduleName
     */
    inline static CString GetModuleNameWithPath(const CString modulePath)
    {
        size_t pos1 = modulePath.find(PathHelper::SLASH_TAG);
        if (pos1 != CString::npos) {
            pos1++;
            return modulePath.substr(pos1, modulePath.size() - pos1 + 1);
        }
        return CString();
    }
    /*
     * Before: @xxx.
     * After:  @xxx:
     */
    inline static bool ChangeTag(CString &path)
    {
        if (path[0] == PathHelper::NAME_SPACE_TAG) {
            size_t pos = path.find(PathHelper::POINT_TAG);
            if (pos != CString::npos) {
                path.replace(pos, 1, PathHelper::COLON_TAG); // 1: length
                return true;
            }
        }
        return false;
    }

    /*
     * Before: moduleName
     * After:  data/storage/el1/bundle/moduleName/ets/modules.abc
     */
    inline static CString ConcatPandaFilePath(const CString &moduleName)
    {
        if (moduleName.size() == 0) {
            return CString();
        }
        return BUNDLE_INSTALL_PATH + moduleName + MERGE_ABC_ETS_MODULES;
    }

    inline static CString GetBundleNameFromNormalized(const EcmaVM *vm, const CString &moduleName)
    {
        CVector<CString> res = SplitNormalizedOhmurl(moduleName);
        if (res.size() != NORMALIZED_OHMURL_ARGS_NUM) {
            LOG_FULL(ERROR) << "GetBundleNameFromNormalized Invalid normalized ohmurl";
            return "";
        }
        CString bundleName = res[NORMALIZED_BUNDLE_NAME_INDEX];
        if (bundleName.size() == 0) {
            return vm->GetBundleName();
        }
        return bundleName;
    }

    inline static CString GetNormalizedPathFromOhmUrl(const CString &moduleName)
    {
        CVector<CString> res = SplitNormalizedOhmurl(moduleName);
        if (res.size() != NORMALIZED_OHMURL_ARGS_NUM) {
            LOG_FULL(ERROR) << "GetNormalizedPathFromOhmUrl Invalid normalized ohmurl";
            return "";
        }
        CString soName = res[NORMALIZED_IMPORT_PATH_INDEX];
        // Delete the prefix "lib" and suffix ".so".
        soName = soName.substr(SO_PREFIX_LEN, soName.size() - SO_PREFIX_LEN - SO_SUFFIX_LEN);
        return soName;
    }
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_MODULE_MODULE_PATH_HELPER_H