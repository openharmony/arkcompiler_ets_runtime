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
#ifndef ECMASCRIPT_BASE_PATH_HELPER_H
#define ECMASCRIPT_BASE_PATH_HELPER_H

#include "ecmascript/aot_file_manager.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/jspandafile/js_pandafile.h"

namespace panda::ecmascript::base {
class PathHelper {
public:
    static constexpr char EXT_NAME_ABC[] = ".abc";
    static constexpr char EXT_NAME_ETS[] = ".ets";
    static constexpr char EXT_NAME_TS[] = ".ts";
    static constexpr char EXT_NAME_JS[] = ".js";
    static constexpr char EXT_NAME_JSON[] = ".json";
    static constexpr char PREFIX_BUNDLE[] = "@bundle:";
    static constexpr char PREFIX_MODULE[] = "@module:";
    static constexpr char PREFIX_PACKAGE[] = "@package:";
    static constexpr char NPM_PATH_SEGMENT[] = "node_modules";
    static constexpr char PACKAGE_PATH_SEGMENT[] = "pkg_modules";
    static constexpr char PACKAGE_ENTRY_FILE[] = "/index";
    static constexpr char BUNDLE_INSTALL_PATH[] = "/data/storage/el1/bundle/";
    static constexpr char MERGE_ABC_ETS_MODULES[] = "/ets/modules.abc";
    static constexpr char MODULE_DEFAULE_ETS[] = "/ets/";
    static constexpr char BUNDLE_SUB_INSTALL_PATH[] = "/data/storage/el1/";
    static constexpr char PREVIEW_OF_ACROSS_HAP_FLAG[] = "[preview]";
    static constexpr char NAME_SPACE_TAG[] = "@";

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

    static void ResolveCurrentPath(JSThread *thread,
                                   JSMutableHandle<JSTaggedValue> &dirPath,
                                   JSMutableHandle<JSTaggedValue> &fileName,
                                   const JSPandaFile *jsPandaFile)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        CString fullName = jsPandaFile->GetJSPandaFileDesc();
        // find last '/'
        int foundPos = static_cast<int>(fullName.find_last_of("/\\"));
        if (foundPos == -1) {
            RETURN_IF_ABRUPT_COMPLETION(thread);
        }
        CString dirPathStr = fullName.substr(0, foundPos + 1);
        JSHandle<EcmaString> dirPathName = factory->NewFromUtf8(dirPathStr);
        dirPath.Update(dirPathName.GetTaggedValue());

        // Get filename from JSPandaFile
        JSHandle<EcmaString> cbFileName = factory->NewFromUtf8(fullName);
        fileName.Update(cbFileName.GetTaggedValue());
    }

    static JSHandle<EcmaString> ResolveDirPath(JSThread *thread,
                                               JSHandle<JSTaggedValue> fileName)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        CString fullName = ConvertToString(fileName.GetTaggedValue());
        // find last '/'
        int foundPos = static_cast<int>(fullName.find_last_of("/\\"));
        if (foundPos == -1) {
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(EcmaString, thread);
        }
        CString dirPathStr = fullName.substr(0, foundPos + 1);
        return factory->NewFromUtf8(dirPathStr);
    }

    static CString NormalizePath(const CString &fileName)
    {
        if (fileName.find("//") == CString::npos && fileName.find("./") == CString::npos &&
            fileName[fileName.size() - 1] != '/') {
            return fileName;
        }
        const char delim = '/';
        CString res = "";
        size_t prev = 0;
        size_t curr = fileName.find(delim);
        CVector<CString> elems;
        while (curr != CString::npos) {
            if (curr > prev) {
                CString elem = fileName.substr(prev, curr - prev);
                if (elem == ".." && !elems.empty()) {
                    elems.pop_back();
                } else if (elem != ".") {
                    elems.push_back(elem);
                }
            }
            prev = curr + 1;
            curr = fileName.find(delim, prev);
        }
        if (prev != fileName.size()) {
            elems.push_back(fileName.substr(prev));
        }
        for (auto e : elems) {
            if (res.size() == 0 && fileName.at(0) != delim) {
                res.append(e);
                continue;
            }
            res.append(1, delim).append(e);
        }
        return res;
    }

    static CString ParseOhmUrl(EcmaVM *vm, const CString &inputFileName, CString &outFileName)
    {
        CString bundleInstallName(BUNDLE_INSTALL_PATH);
        size_t startStrLen = bundleInstallName.length();
        size_t pos = CString::npos;

        if (inputFileName.length() > startStrLen && inputFileName.compare(0, startStrLen, bundleInstallName) == 0) {
            pos = startStrLen;
        }
        CString entryPoint;
        if (pos != CString::npos) {
            pos = inputFileName.find('/', startStrLen);
            if (pos == CString::npos) {
                LOG_FULL(FATAL) << "Invalid Ohm url, please check.";
            }
            CString moduleName = inputFileName.substr(startStrLen, pos - startStrLen);
            outFileName = BUNDLE_INSTALL_PATH + moduleName + MERGE_ABC_ETS_MODULES;
            entryPoint = vm->GetBundleName() + "/" + inputFileName.substr(startStrLen);
        } else {
            // Temporarily handle the relative path sent by arkui
            if (StringHelper::StringStartWith(inputFileName, PREFIX_BUNDLE)) {
                entryPoint = inputFileName.substr(PREFIX_BUNDLE_LEN);
                outFileName = ParseUrl(vm, entryPoint);
            } else {
#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS)
                entryPoint = vm->GetBundleName() + "/" +  inputFileName;
#else
                entryPoint = vm->GetBundleName() + "/" + vm->GetModuleName() + MODULE_DEFAULE_ETS + inputFileName;
#endif
            }
        }
        if (StringHelper::StringEndWith(entryPoint, EXT_NAME_ABC)) {
            entryPoint.erase(entryPoint.length() - EXT_NAME_ABC_LEN, EXT_NAME_ABC_LEN);
        }
        return entryPoint;
    }

    static void CropNamespaceIfAbsent(CString &moduleName)
    {
        size_t pos = moduleName.find(NAME_SPACE_TAG);
        if (pos == CString::npos) {
            return;
        }
        moduleName.erase(pos, moduleName.size() - pos);
    }

    // current ohmUrl format : @bundle:bundlename/modulename@namespace/entry/src/index
    static CString ParseUrl(EcmaVM *vm, const CString &entryPoint)
    {
        CVector<CString> vec;
        StringHelper::SplitString(entryPoint, vec, 0, SEGMENTS_LIMIT_TWO);
        if (vec.size() < SEGMENTS_LIMIT_TWO) {
            LOG_ECMA(DEBUG) << "ParseUrl SplitString filed, please check Url" << entryPoint;
            return CString();
        }
        CString bundleName = vec[0];
        CString moduleName = vec[1];
        CropNamespaceIfAbsent(moduleName);

        CString baseFileName;
        if (bundleName != vm->GetBundleName()) {
            // Cross-application
            baseFileName =
                BUNDLE_INSTALL_PATH + bundleName + "/" + moduleName + "/" + moduleName + MERGE_ABC_ETS_MODULES;
        } else {
            // Intra-application cross hap
            baseFileName = BUNDLE_INSTALL_PATH + moduleName + MERGE_ABC_ETS_MODULES;
        }
        return baseFileName;
    }

    static std::string ParseHapPath(const CString &fileName)
    {
        CString bundleSubInstallName(BUNDLE_SUB_INSTALL_PATH);
        size_t startStrLen = bundleSubInstallName.length();
        if (fileName.length() > startStrLen && fileName.compare(0, startStrLen, bundleSubInstallName) == 0) {
            CString hapPath = fileName.substr(startStrLen);
            size_t pos = hapPath.find(MERGE_ABC_ETS_MODULES);
            if (pos != CString::npos) {
                return hapPath.substr(0, pos).c_str();
            }
        }
        return std::string();
    }

    static void CroppingRecord(CString &recordName)
    {
        size_t pos = recordName.find('/');
        if (pos != CString::npos) {
            pos = recordName.find('/', pos + 1);
            if (pos != CString::npos) {
                recordName = recordName.substr(pos + 1);
            }
        }
    }

    static CString ParsePrefixBundle(JSThread *thread, const JSPandaFile *jsPandaFile,
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
            CropNamespaceIfAbsent(moduleName);

#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS)
            if (bundleName != vm->GetBundleName()) {
                baseFileName =
                    BUNDLE_INSTALL_PATH + bundleName + '/' + moduleName + '/' + moduleName + MERGE_ABC_ETS_MODULES;
            } else {
                baseFileName = BUNDLE_INSTALL_PATH + moduleName + MERGE_ABC_ETS_MODULES;
            }
#else
            CVector<CString> currentVec;
            StringHelper::SplitString(recordName, currentVec, 0, SEGMENTS_LIMIT_TWO);
            if (vec.size() < SEGMENTS_LIMIT_TWO) {
                LOG_ECMA(INFO) << "SplitString filed, please check moduleRequestName";
                return CString();
            }
            CString currentModuleName = currentVec[1];
            CropNamespaceIfAbsent(currentModuleName);
            if (bundleName != vm->GetBundleName() || moduleName != currentModuleName) {
                entryPoint = PREVIEW_OF_ACROSS_HAP_FLAG;
                LOG_NO_TAG(ERROR) << "[ArkRuntime Log] Importing shared package is not supported in the Previewer.";
            }
#endif
        } else {
            CroppingRecord(entryPoint);
        }
        return entryPoint;
    }

    static CString MakeNewRecord(const JSPandaFile *jsPandaFile, CString &baseFileName, const CString &recordName,
                                 const CString &requestName)
    {
        CString entryPoint;
        CString moduleRequestName = RemoveSuffix(requestName);
        size_t pos = moduleRequestName.find("./");
        if (pos == 0) {
            moduleRequestName = moduleRequestName.substr(2); // 2 means jump "./"
        }
        pos = recordName.rfind('/');
        if (pos != CString::npos) {
            entryPoint = recordName.substr(0, pos + 1) + moduleRequestName;
        } else {
            entryPoint = moduleRequestName;
        }
        entryPoint = NormalizePath(entryPoint);
        entryPoint = ConfirmLoadingIndexOrNot(jsPandaFile, entryPoint);
        if (!entryPoint.empty()) {
            return entryPoint;
        }
        // the package name may have a '.js' suffix, try to parseThirdPartyPackage
        entryPoint = ParseThirdPartyPackage(jsPandaFile, recordName, requestName, NPM_PATH_SEGMENT);
        if (!entryPoint.empty()) {
            return entryPoint;
        }
        // Execute abc locally
        pos = baseFileName.rfind('/');
        if (pos != CString::npos) {
            baseFileName = baseFileName.substr(0, pos + 1) + moduleRequestName + EXT_NAME_ABC;
        } else {
            baseFileName = moduleRequestName + EXT_NAME_ABC;
        }
        pos = moduleRequestName.rfind('/');
        if (pos != CString::npos) {
            entryPoint = moduleRequestName.substr(pos + 1);
        } else {
            entryPoint = moduleRequestName;
        }
        return entryPoint;
    }

    static CString ConfirmLoadingIndexOrNot(const JSPandaFile *jsPandaFile, const CString &packageEntryPoint)
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

    static CString FindNpmEntryPoint(const JSPandaFile *jsPandaFile, const CString &packageEntryPoint)
    {
        // if we are currently importing a specific file or directory, we will get the entryPoint here
        CString entryPoint = ConfirmLoadingIndexOrNot(jsPandaFile, packageEntryPoint);
        if (!entryPoint.empty()) {
            return entryPoint;
        }
        // When you come here, must import a packageName
        return jsPandaFile->GetEntryPoint(packageEntryPoint);
    }

    static CString FindPackageInTopLevel(const JSPandaFile *jsPandaFile, const CString& requestName,
                                         const CString &packagePath)
    {
        // we find node_modules/0/xxx or node_modules/1/xxx
        CString entryPoint;
        for (size_t level = 0; level <= MAX_PACKAGE_LEVEL; ++level) {
            CString levelStr = std::to_string(level).c_str();
            CString key = packagePath + "/" + levelStr + '/' + requestName;
            entryPoint = FindNpmEntryPoint(jsPandaFile, key);
            if (!entryPoint.empty()) {
                return entryPoint;
            }
        }
        return CString();
    }

    static CString FindOhpmEntryPoint(const JSPandaFile *jsPandaFile, const CString& ohpmPath,
                                      const CString& requestName)
    {
        CVector<CString> vec;
        StringHelper::SplitString(requestName, vec, 0);
        size_t maxIndex = vec.size() - 1;
        CString ohpmKey;
        size_t index = 0;
        // first we find the ohpmKey by splicing the requestName
        while (index <= maxIndex) {
            CString maybeKey = ohpmPath + "/" + StringHelper::JoinString(vec, 0, index);
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
            entryPoint = jsPandaFile->GetEntryPoint(ohpmKey);
        } else {
            // import a specific file or directory
            ohpmKey = ohpmKey + "/" + StringHelper::JoinString(vec, index + 1, maxIndex);
            entryPoint = ConfirmLoadingIndexOrNot(jsPandaFile, ohpmKey);
        }
        return entryPoint;
    }

    static CString FindPackageInTopLevelWithNamespace(const JSPandaFile *jsPandaFile, const CString& requestName,
                                                      const CString &recordName)
    {
        // find in current module <PACKAGE_PATH_SEGMENT>@[moduleName|namespace]/<requestName>
        CString entryPoint;
        CString ohpmPath;
        if (StringHelper::StringStartWith(recordName, PACKAGE_PATH_SEGMENT)) {
            size_t pos = recordName.find('/');
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
            size_t pos = moduleName.find(NAME_SPACE_TAG);
            if (pos != CString::npos) {
                moduleName = moduleName.substr(pos + 1);
            }
            ohpmPath = CString(PACKAGE_PATH_SEGMENT) + NAME_SPACE_TAG + moduleName;
            entryPoint = FindOhpmEntryPoint(jsPandaFile, ohpmPath, requestName);
        }
        if (!entryPoint.empty()) {
            return entryPoint;
        }
        // find in project directory <packagePath>/<requestName>
        return FindOhpmEntryPoint(jsPandaFile, PACKAGE_PATH_SEGMENT, requestName);
    }

    static CString ParseOhpmPackage(const JSPandaFile *jsPandaFile, const CString &recordName,
                                    const CString &requestName)
    {
        CString entryPoint;
        if (StringHelper::StringStartWith(recordName, PACKAGE_PATH_SEGMENT)) {
            //this way is thirdPartyPackage import ThirdPartyPackage
            auto info = const_cast<JSPandaFile *>(jsPandaFile)->FindRecordInfo(recordName);
            CString packageName = info.npmPackageName;
            size_t pos = packageName.rfind(PACKAGE_PATH_SEGMENT);
            if (pos != CString::npos) {
                packageName.erase(pos, packageName.size() - pos);
                CString ohpmPath = packageName + PACKAGE_PATH_SEGMENT;
                entryPoint = FindOhpmEntryPoint(jsPandaFile, ohpmPath, requestName);
                if (!entryPoint.empty()) {
                    return entryPoint;
                }
            }
        }
        // Import packages under the current module or project directory
        return FindPackageInTopLevelWithNamespace(jsPandaFile, requestName, recordName);
    }

    static CString ParseThirdPartyPackage(const JSPandaFile *jsPandaFile, const CString &recordName,
                                          const CString &requestName, const CString &packagePath)
    {
        CString entryPoint;
        if (StringHelper::StringStartWith(recordName, packagePath)) {
            auto info = const_cast<JSPandaFile *>(jsPandaFile)->FindRecordInfo(recordName);
            CString packageName = info.npmPackageName;
            size_t pos = 0;
            while (true) {
                CString key = packageName + '/' + packagePath + "/" + requestName;
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

    static CString ParseThirdPartyPackage(const JSPandaFile *jsPandaFile, const CString &recordName,
                                          const CString &requestName)
    {
        static CVector<CString> packagePaths = {CString(PACKAGE_PATH_SEGMENT), CString(NPM_PATH_SEGMENT)};
        // We need to deal with scenarios like this 'json5/' -> 'json5'
        CString normalizeRequestName = NormalizePath(requestName);
        CString entryPoint = ParseOhpmPackage(jsPandaFile, recordName, normalizeRequestName);
        if (!entryPoint.empty()) {
            return entryPoint;
        }
        // Package compatible with old soft link format
        for (size_t i = 0; i < packagePaths.size(); ++i) {
            entryPoint = ParseThirdPartyPackage(jsPandaFile, recordName, normalizeRequestName, packagePaths[i]);
            if (!entryPoint.empty()) {
                return entryPoint;
            }
        }
        return CString();
    }

    static bool IsImportFile(CString &moduleRequestName)
    {
        if (moduleRequestName[0] == '.') {
            return true;
        }
        size_t pos = moduleRequestName.rfind('.');
        if (pos != CString::npos) {
            CString suffix = moduleRequestName.substr(pos);
            if (suffix == EXT_NAME_JS || suffix == EXT_NAME_TS || suffix == EXT_NAME_ETS || suffix == EXT_NAME_JSON) {
                return true;
            }
        }
        return false;
    }

    static CString RemoveSuffix(const CString &requestName)
    {
        CString res = requestName;
        size_t pos = res.rfind('.');
        if (pos != CString::npos) {
            CString suffix = res.substr(pos);
            if (suffix == EXT_NAME_JS || suffix == EXT_NAME_TS || suffix == EXT_NAME_ETS || suffix == EXT_NAME_JSON) {
                res.erase(pos, suffix.length());
            }
        }
        return res;
    }

    static CString ConcatFileNameWithMerge(JSThread *thread, const JSPandaFile *jsPandaFile, CString &baseFileName,
                                           CString recordName, CString requestName)
    {
        CString entryPoint;
        if (StringHelper::StringStartWith(requestName, PREFIX_BUNDLE)) {
            entryPoint = ParsePrefixBundle(thread, jsPandaFile, baseFileName, requestName, recordName);
        } else if (StringHelper::StringStartWith(requestName, PREFIX_PACKAGE)) {
            entryPoint = requestName.substr(PREFIX_PACKAGE_LEN);
        } else if (IsImportFile(requestName)) { // load a relative pathName.
            entryPoint = MakeNewRecord(jsPandaFile, baseFileName, recordName, requestName);
        } else {
            entryPoint = ParseThirdPartyPackage(jsPandaFile, recordName, requestName);
        }
        if (entryPoint.empty()) {
            LOG_ECMA(ERROR) << "Failed to resolve the requested entryPoint. baseFileName : '" << baseFileName <<
                "'. RecordName : '" <<  recordName << "'. RequestName : '" <<  requestName << "'.";
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            CString msg = "failed to load module'" + requestName + "' which imported by '" +
                          recordName + "'. Please check the target path.";
            JSTaggedValue error = factory->GetJSError(ErrorType::REFERENCE_ERROR, msg.c_str()).GetTaggedValue();
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, entryPoint);
        }
        return entryPoint;
    }
};
}  // namespace panda::ecmascript::base
#endif  // ECMASCRIPT_BASE_PATH_HELPER_H
