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
#include "ecmascript/module/module_path_helper.h"

namespace panda::ecmascript {
/*
 * This function use recordName, requestName to get baseFileName and entryPoint.
 */
CString ModulePathHelper::ConcatFileNameWithMerge(JSThread *thread, const JSPandaFile *jsPandaFile,
    CString &baseFileName, CString recordName, CString requestName)
{
    CString entryPoint;
    if (StringHelper::StringStartWith(requestName, PREFIX_BUNDLE)) {
        // requestName: @bundle:bundleName/moduleName@namespace/ets/xxx/xxx
        entryPoint = ParsePrefixBundle(thread, jsPandaFile, baseFileName, requestName, recordName);
    } else if (StringHelper::StringStartWith(requestName, PREFIX_PACKAGE)) {
        // requestName: @package:pkg_modules@namespace/xxx/Index
        entryPoint = requestName.substr(PREFIX_PACKAGE_LEN);
    } else if (IsImportFile(requestName)) {
        // this branch save for cjs, dynamic import and old-version's sdk
        // load a relative pathName.
        // requestName: ./ || ./xxx/xxx.js || ../xxx/xxx.js || ./xxx/xxx
        entryPoint = MakeNewRecord(jsPandaFile, baseFileName, recordName, requestName);
    } else {
        // this branch save for cjs, dynamic import and old-version's sdk
        // requestName: requestPkgName
        entryPoint = ParseThirdPartyPackage(jsPandaFile, recordName, requestName);
    }
    if (entryPoint.empty() && thread->GetEcmaVM()->EnableReportModuleResolvingFailure()) {
        LOG_ECMA(ERROR) << "Failed to resolve the requested entryPoint. baseFileName : '" << baseFileName <<
            "'. RecordName : '" <<  recordName << "'. RequestName : '" <<  requestName << "'.";
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        CString msg = "failed to load module'" + requestName + "' which imported by '" +
            recordName + "'. Please check the target path.";
        JSTaggedValue error = factory->GetJSError(ErrorType::REFERENCE_ERROR, msg.c_str()).GetTaggedValue();
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, entryPoint);
    }
    LOG_ECMA(DEBUG) << "Concated baseFileName : '" << baseFileName <<
        "'. EntryPoint : '" <<  entryPoint << "'.";
    return entryPoint;
}

/*
 * Before: inputFileName: 1. /data/storage/el1/bundle/moduleName/ets/xxx/xxx.abc
                          2. @bundle:bundleName/moduleName/ets/xxx/xxx.abc
                          3. moduleName/ets/xxx/xxx.abc
                          4. .test/xxx/xxx.abc
                          5. xxx/xxx.abc
 * After:  outBaseFileName: /data/storage/el1/bundle/moduleName/ets/modules.abc
 *         outEntryPoint: bundleName/moduleName/ets/xxx/xxx
 */
void ModulePathHelper::ParseOhmUrl(EcmaVM *vm, const CString &inputFileName,
    CString &outBaseFileName, CString &outEntryPoint)
{
    CString bundleInstallName(BUNDLE_INSTALL_PATH);
    size_t startStrLen = bundleInstallName.length();
    size_t pos = CString::npos;

    if (inputFileName.length() > startStrLen && inputFileName.compare(0, startStrLen, bundleInstallName) == 0) {
        pos = startStrLen;
    }
    if (pos != CString::npos) {
        // inputFileName: /data/storage/el1/bundle/moduleName/ets/xxx/xxx.abc
        pos = inputFileName.find(PathHelper::SLASH_TAG, startStrLen);
        if (pos == CString::npos) {
            LOG_FULL(FATAL) << "Invalid Ohm url, please check.";
        }
        CString moduleName = inputFileName.substr(startStrLen, pos - startStrLen);
        outBaseFileName = BUNDLE_INSTALL_PATH + moduleName + MERGE_ABC_ETS_MODULES;
        outEntryPoint = vm->GetBundleName() + PathHelper::SLASH_TAG + inputFileName.substr(startStrLen);
    } else {
        // Temporarily handle the relative path sent by arkui
        // inputFileName: @bundle:bundleName/moduleName/ets/xxx/xxx.abc
        if (StringHelper::StringStartWith(inputFileName, PREFIX_BUNDLE)) {
            outEntryPoint = inputFileName.substr(PREFIX_BUNDLE_LEN);
            outBaseFileName = ParseUrl(vm, outEntryPoint);
        } else {
#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS)
            // inputFileName: moduleName/ets/xxx/xxx.abc
            outEntryPoint = vm->GetBundleName() + PathHelper::SLASH_TAG + inputFileName;
#else
            // if the inputFileName starts with '.test', the preview test page is started.
            // in this case, the path ets does not need to be combined.
            // inputFileName: .test/xxx/xxx.abc
            if (StringHelper::StringStartWith(inputFileName, PREVIER_TEST_DIR)) {
                outEntryPoint = vm->GetBundleName() + PathHelper::SLASH_TAG + vm->GetModuleName() +
                                PathHelper::SLASH_TAG + inputFileName;
            } else {
                // inputFileName: xxx/xxx.abc
                outEntryPoint = vm->GetBundleName() + PathHelper::SLASH_TAG + vm->GetModuleName() +
                                MODULE_DEFAULE_ETS + inputFileName;
            }
#endif
        }
    }
    if (StringHelper::StringEndWith(outEntryPoint, EXT_NAME_ABC)) {
        outEntryPoint.erase(outEntryPoint.length() - EXT_NAME_ABC_LEN, EXT_NAME_ABC_LEN);
    }
}

/*
 * Before: recordName: bundleName/moduleName@namespace/moduleName/xxx/xxx.abc
 * After: Intra-application cross hap:   /data/storage/el1/bundle/bundleName/ets/modules.abc
 *        Cross-application:             /data/storage/el1/bundle/bundleName/moduleName/moduleName/ets/modules.abc
 */
CString ModulePathHelper::ParseUrl(EcmaVM *vm, const CString &recordName)
{
    CVector<CString> vec;
    StringHelper::SplitString(recordName, vec, 0, SEGMENTS_LIMIT_TWO);
    if (vec.size() < SEGMENTS_LIMIT_TWO) {
        LOG_ECMA(DEBUG) << "ParseUrl SplitString filed, please check Url" << recordName;
        return CString();
    }
    CString bundleName = vec[0];
    CString moduleName = vec[1];
    PathHelper::DeleteNamespace(moduleName);

    CString baseFileName;
    if (bundleName != vm->GetBundleName()) {
        // Cross-application
        baseFileName = BUNDLE_INSTALL_PATH + bundleName + PathHelper::SLASH_TAG + moduleName +
                       PathHelper::SLASH_TAG + moduleName + MERGE_ABC_ETS_MODULES;
    } else {
        // Intra-application cross hap
        baseFileName = BUNDLE_INSTALL_PATH + moduleName + MERGE_ABC_ETS_MODULES;
    }
    return baseFileName;
}

/*
 * Before: moduleRequestName: @bundle:bundleName/moduleName@namespace/ets/xxx
 * After:  baseFileName: 1./data/storage/el1/bundle/bundleName/ets/modules.abc
 *                       2./data/storage/el1/bundle/bundleName/moduleName/moduleName/ets/modules.abc
 *         entryPoint:   bundleName/moduleName@namespace/ets/xxx
 */
CString ModulePathHelper::ParsePrefixBundle(JSThread *thread, const JSPandaFile *jsPandaFile,
    [[maybe_unused]] CString &baseFileName, CString moduleRequestName, [[maybe_unused]] CString recordName)
{
    EcmaVM *vm = thread->GetEcmaVM();
    moduleRequestName = moduleRequestName.substr(PREFIX_BUNDLE_LEN);
    CString entryPoint = moduleRequestName;
    if (jsPandaFile->IsRecordWithBundleName()) {
        CVector<CString> vec;
        StringHelper::SplitString(moduleRequestName, vec, 0, SEGMENTS_LIMIT_TWO);
        if (vec.size() < SEGMENTS_LIMIT_TWO) {
            LOG_ECMA(INFO) << "SplitString filed, please check moduleRequestName";
            return CString();
        }
        CString bundleName = vec[0];
        CString moduleName = vec[1];
        PathHelper::DeleteNamespace(moduleName);

#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS)
        if (bundleName != vm->GetBundleName()) {
            baseFileName = BUNDLE_INSTALL_PATH + bundleName + PathHelper::SLASH_TAG + moduleName +
                           PathHelper::SLASH_TAG + moduleName + MERGE_ABC_ETS_MODULES;
        } else if (moduleName != vm->GetModuleName()) {
            baseFileName = BUNDLE_INSTALL_PATH + moduleName + MERGE_ABC_ETS_MODULES;
        } else {
            // Support multi-module card service
            baseFileName = vm->GetAssetPath();
        }
#else
        CVector<CString> currentVec;
        StringHelper::SplitString(recordName, currentVec, 0, SEGMENTS_LIMIT_TWO);
        if (currentVec.size() < SEGMENTS_LIMIT_TWO) {
            LOG_ECMA(INFO) << "SplitString filed, please check recordName";
            return CString();
        }
        CString currentModuleName = currentVec[1];
        PathHelper::DeleteNamespace(currentModuleName);
        if (bundleName != vm->GetBundleName() || moduleName != currentModuleName) {
            entryPoint = PREVIEW_OF_ACROSS_HAP_FLAG;
            if (vm->EnableReportModuleResolvingFailure()) {
                LOG_NO_TAG(ERROR) << "[ArkRuntime Log] Importing shared package is not supported in the Previewer.";
            }
        }
#endif
    } else {
        PathHelper::AdaptOldIsaRecord(entryPoint);
    }
    return entryPoint;
}

/*
 * Before: requestName: ../xxx1/xxx2 || ./b
 *         recordName: pkg_modules/.ohpm/pkgName/pkg_modules/pkgName/xxx1/xxx3 || a
 * After:  entryPoint: pkg_modules/.ohpm/pkgName/pkg_modules/pkgName/xxx1/xxx2 || b
 *         baseFileName: /data/storage/el1/bundle/moduleName/ets/modules.abc || /home/user/src/a
 */
CString ModulePathHelper::MakeNewRecord(const JSPandaFile *jsPandaFile, CString &baseFileName,
    const CString &recordName, const CString &requestName)
{
    CString entryPoint;
    CString moduleRequestName = RemoveSuffix(requestName);
    size_t pos = moduleRequestName.find(PathHelper::CURRENT_DIREATORY_TAG);
    if (pos == 0) {
        moduleRequestName = moduleRequestName.substr(2); // 2 means jump "./"
    }
    pos = recordName.rfind(PathHelper::SLASH_TAG);
    if (pos != CString::npos) {
        // entryPoint: pkg_modules/.ohpm/pkgName/pkg_modules/pkgName/xxx1/../xxx1/xxx2
        entryPoint = recordName.substr(0, pos + 1) + moduleRequestName;
    } else {
        entryPoint = moduleRequestName;
    }
    // entryPoint: pkg_modules/.ohpm/pkgName/pkg_modules/pkgName/xxx1/xxx2
    entryPoint = PathHelper::NormalizePath(entryPoint);
    entryPoint = ConfirmLoadingIndexOrNot(jsPandaFile, entryPoint);
    if (!entryPoint.empty()) {
        return entryPoint;
    }
    // the package name may have a '.js' suffix, try to parseThirdPartyPackage
    entryPoint = ParseThirdPartyPackage(jsPandaFile, recordName, requestName);
    if (!entryPoint.empty()) {
        return entryPoint;
    }
    // Execute abc locally
    pos = baseFileName.rfind(PathHelper::SLASH_TAG);
    if (pos != CString::npos) {
        baseFileName = baseFileName.substr(0, pos + 1) + moduleRequestName + EXT_NAME_ABC;
    } else {
        baseFileName = moduleRequestName + EXT_NAME_ABC;
    }
    pos = moduleRequestName.rfind(PathHelper::SLASH_TAG);
    if (pos != CString::npos) {
        entryPoint = moduleRequestName.substr(pos + 1);
    } else {
        entryPoint = moduleRequestName;
    }
    return entryPoint;
}

/*
 * Before: ohpmPath:      pkg_modules/.ohpm/pkgName/pkg_modules
 *         requestName:   requestPkgName
 * After:  entryPoint：   pkg_modules/.ohpm/requestPkgName/pkg_modules/requestPkgName/xxx
 */
CString ModulePathHelper::FindOhpmEntryPoint(const JSPandaFile *jsPandaFile,
    const CString& ohpmPath, const CString& requestName)
{
    CVector<CString> vec;
    StringHelper::SplitString(requestName, vec, 0);
    size_t maxIndex = vec.size() - 1;
    CString ohpmKey;
    size_t index = 0;
    // first we find the ohpmKey by splicing the requestName
    while (index <= maxIndex) {
        // maybeKey: pkg_modules/.ohpm/pkgName/pkg_modules/requestPkgName
        CString maybeKey = ohpmPath + PathHelper::SLASH_TAG + StringHelper::JoinString(vec, 0, index);
        // ohpmKey: pkg_modules/.ohpm/requestPkgName/pkg_modules/requestPkgName
        ohpmKey = jsPandaFile->GetNpmEntries(maybeKey);
        if (!ohpmKey.empty()) {
            break;
        }
        ++index;
    }
    if (ohpmKey.empty()) {
        return CString();
    }
    // second If the ohpmKey is not empty, we will use it to obtain the real entrypoint
    CString entryPoint;
    if (index == maxIndex) {
        // requestName is a packageName
        // entryPoint: pkg_modules/.ohpm/requestPkgName/pkg_modules/requestPkgName/xxx
        entryPoint = jsPandaFile->GetEntryPoint(ohpmKey);
    } else {
        // import a specific file or directory
        ohpmKey = ohpmKey + PathHelper::SLASH_TAG + StringHelper::JoinString(vec, index + 1, maxIndex);
        entryPoint = ConfirmLoadingIndexOrNot(jsPandaFile, ohpmKey);
    }
    return entryPoint;
}

CString ModulePathHelper::FindPackageInTopLevelWithNamespace(const JSPandaFile *jsPandaFile,
    const CString& requestName, const CString &recordName)
{
    // find in current module <PACKAGE_PATH_SEGMENT>@[moduleName|namespace]/<requestName>
    CString entryPoint;
    CString ohpmPath;
    if (StringHelper::StringStartWith(recordName, PACKAGE_PATH_SEGMENT)) {
        size_t pos = recordName.find(PathHelper::SLASH_TAG);
        if (pos == CString::npos) {
            LOG_ECMA(DEBUG) << "wrong recordname : " << recordName;
            return CString();
        }
        ohpmPath = recordName.substr(0, pos);
        entryPoint = FindOhpmEntryPoint(jsPandaFile, recordName.substr(0, pos), requestName);
    } else {
        CVector<CString> vec;
        StringHelper::SplitString(recordName, vec, 0, SEGMENTS_LIMIT_TWO);
        if (vec.size() < SEGMENTS_LIMIT_TWO) {
            LOG_ECMA(DEBUG) << "SplitString filed, please check moduleRequestName";
            return CString();
        }
        CString moduleName = vec[1];
        // If namespace exists, use namespace as moduleName
        size_t pos = moduleName.find(PathHelper::NAME_SPACE_TAG);
        if (pos != CString::npos) {
            moduleName = moduleName.substr(pos + 1);
        }
        ohpmPath = CString(PACKAGE_PATH_SEGMENT) + PathHelper::NAME_SPACE_TAG + moduleName;
        entryPoint = FindOhpmEntryPoint(jsPandaFile, ohpmPath, requestName);
    }
    if (!entryPoint.empty()) {
        return entryPoint;
    }
    // find in project directory <packagePath>/<requestName>
    return FindOhpmEntryPoint(jsPandaFile, PACKAGE_PATH_SEGMENT, requestName);
}

/*
 * Before: requestName:  requestPkgName
 *         recordName:   pkg_modules/.ohpm/pkgName/pkg_modules/pkgName/xxx
 * After:  entryPoint:   pkg_modules/.ohpm/requestPkgName/pkg_modules/requestPkgName/xxx
 */
CString ModulePathHelper::ParseOhpmPackage(const JSPandaFile *jsPandaFile,
    const CString &recordName, const CString &requestName)
{
    CString entryPoint;
    if (StringHelper::StringStartWith(recordName, PACKAGE_PATH_SEGMENT)) {
        // this way is thirdPartyPackage import ThirdPartyPackage
        auto info = const_cast<JSPandaFile *>(jsPandaFile)->FindRecordInfo(recordName);
        // packageName: pkg_modules/.ohpm/pkgName/pkg_modules/pkgName
        CString packageName = info.npmPackageName;
        size_t pos = packageName.rfind(PACKAGE_PATH_SEGMENT);
        if (pos != CString::npos) {
            packageName.erase(pos, packageName.size() - pos);
            // ohpmPath: pkg_modules/.ohpm/pkgName/pkg_modules
            CString ohpmPath = packageName + PACKAGE_PATH_SEGMENT;
            // entryPoint: pkg_modules/.ohpm/requestPkgName/pkg_modules/requestPkgName/xxx
            entryPoint = FindOhpmEntryPoint(jsPandaFile, ohpmPath, requestName);
            if (!entryPoint.empty()) {
                return entryPoint;
            }
        }
    }
    // Import packages under the current module or project directory
    return FindPackageInTopLevelWithNamespace(jsPandaFile, requestName, recordName);
}

/*
 * Before: requestName:  requestPkgName
 *         recordName:   pkg_modules/.ohpm/pkgName/pkg_modules/pkgName/xxx
 *         packagePath:  pkg_modules || node_modules
 * After:  entryPoint:   pkg_modules/.ohpm/requestPkgName/pkg_modules/requestPkgName/xxx
 */
CString ModulePathHelper::ParseThirdPartyPackage(const JSPandaFile *jsPandaFile,
    const CString &recordName, const CString &requestName, const CString &packagePath)
{
    CString entryPoint;
    if (StringHelper::StringStartWith(recordName, packagePath)) {
        auto info = const_cast<JSPandaFile *>(jsPandaFile)->FindRecordInfo(recordName);
        CString packageName = info.npmPackageName; // pkg_modules/.ohpm/pkgName/pkg_modules/pkgName
        size_t pos = 0;
        while (true) {
            CString key = packageName + PathHelper::SLASH_TAG + packagePath + PathHelper::SLASH_TAG + requestName;
            entryPoint = FindNpmEntryPoint(jsPandaFile, key);
            if (!entryPoint.empty()) {
                return entryPoint;
            }
            pos = packageName.rfind(packagePath) - 1;
            if (pos == CString::npos || pos < 0) {
                break;
            }
            packageName.erase(pos, packageName.size() - pos);
        }
    }
    return FindPackageInTopLevel(jsPandaFile, requestName, packagePath);
}

/*
 * Before: requestName:  requestPkgName
 *         recordName:   pkg_modules/.ohpm/pkgName/pkg_modules/pkgName/xxx
 * After:  entryPoint:   pkg_modules/.ohpm/requestPkgName/pkg_modules/requestPkgName/xxx
 */
CString ModulePathHelper::ParseThirdPartyPackage(const JSPandaFile *jsPandaFile,
    const CString &recordName, const CString &requestName)
{
    // We need to deal with scenarios like this 'json5/' -> 'json5'
    CString normalizeRequestName = PathHelper::NormalizePath(requestName);
    CString entryPoint = ParseOhpmPackage(jsPandaFile, recordName, normalizeRequestName);
    if (!entryPoint.empty()) {
        return entryPoint;
    }

    static CVector<CString> packagePaths = {CString(PACKAGE_PATH_SEGMENT), CString(NPM_PATH_SEGMENT)};
    // Package compatible with old soft link format
    for (size_t i = 0; i < packagePaths.size(); ++i) {
        entryPoint = ParseThirdPartyPackage(jsPandaFile, recordName, normalizeRequestName, packagePaths[i]);
        if (!entryPoint.empty()) {
            return entryPoint;
        }
    }
    return CString();
}

/*
 * Before: dirPath: Undefined
 *         fileName: Undefined
 * After:  dirPath: pkg_modules/.ohpm/pkgName/pkg_modules/pkgName/xxx/
 *         fileName: pkg_modules/.ohpm/pkgName/pkg_modules/pkgName/xxx/xxx.abc
 */
void ModulePathHelper::ResolveCurrentPath(JSThread *thread, JSMutableHandle<JSTaggedValue> &dirPath,
    JSMutableHandle<JSTaggedValue> &fileName, const JSPandaFile *jsPandaFile)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    CString fullName = jsPandaFile->GetJSPandaFileDesc();
    JSHandle<EcmaString> dirPathName = PathHelper::ResolveDirPath(thread, fullName);
    dirPath.Update(dirPathName.GetTaggedValue());

    // Get filename from JSPandaFile
    JSHandle<EcmaString> cbFileName = factory->NewFromUtf8(fullName);
    fileName.Update(cbFileName.GetTaggedValue());
}

CString ModulePathHelper::FindNpmEntryPoint(const JSPandaFile *jsPandaFile, const CString &packageEntryPoint)
{
    // if we are currently importing a specific file or directory, we will get the entryPoint here
    CString entryPoint = ConfirmLoadingIndexOrNot(jsPandaFile, packageEntryPoint);
    if (!entryPoint.empty()) {
        return entryPoint;
    }
    // When you come here, must import a packageName
    return jsPandaFile->GetEntryPoint(packageEntryPoint);
}

/*
 * Before: requestName:  requestPkgName
 *         packagePath:  pkg_modules || node_modules
 * After:  entryPoint:   pkg_modules/.ohpm/requestPkgName/pkg_modules/requestPkgName/xxx
 */
CString ModulePathHelper::FindPackageInTopLevel(const JSPandaFile *jsPandaFile,
    const CString& requestName, const CString &packagePath)
{
    // we find packagePath/0/xxx or packagePath/1/xxx
    CString entryPoint;
    for (size_t level = 0; level <= MAX_PACKAGE_LEVEL; ++level) {
        CString levelStr = std::to_string(level).c_str();
        CString key = packagePath + PathHelper::SLASH_TAG + levelStr + PathHelper::SLASH_TAG + requestName;
        entryPoint = FindNpmEntryPoint(jsPandaFile, key);
        if (!entryPoint.empty()) {
            return entryPoint;
        }
    }
    return CString();
}

bool ModulePathHelper::IsImportFile(const CString &moduleRequestName)
{
    if (moduleRequestName[0] == PathHelper::POINT_TAG) {
        return true;
    }
    size_t pos = moduleRequestName.rfind(PathHelper::POINT_TAG);
    if (pos != CString::npos) {
        CString suffix = moduleRequestName.substr(pos);
        if (suffix == EXT_NAME_JS || suffix == EXT_NAME_TS || suffix == EXT_NAME_ETS || suffix == EXT_NAME_JSON) {
            return true;
        }
    }
    return false;
}

/*
 * Before: xxx/xxx.js || xxx/xxx.ts || xxx/xxx.ets ||xxx/xxx.json
 * After:  xxx/xxx
 */
CString ModulePathHelper::RemoveSuffix(const CString &requestName)
{
    CString res = requestName;
    size_t pos = res.rfind(PathHelper::POINT_TAG);
    if (pos != CString::npos) {
        CString suffix = res.substr(pos);
        if (suffix == EXT_NAME_JS || suffix == EXT_NAME_TS || suffix == EXT_NAME_ETS || suffix == EXT_NAME_JSON) {
            res.erase(pos, suffix.length());
        }
    }
    return res;
}
}  // namespace panda::ecmascript::base