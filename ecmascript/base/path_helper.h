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
    static constexpr char NPM_ENTRY_FILE[] = "/index";
    static constexpr char BUNDLE_INSTALL_PATH[] = "/data/storage/el1/bundle/";
    static constexpr char MERGE_ABC_ETS_MODULES[] = "/ets/modules.abc";
    static constexpr char MODULE_DEFAULE_ETS[] = "/ets/";
    static constexpr char BUNDLE_SUB_INSTALL_PATH[] = "/data/storage/el1/";
    static constexpr char PREVIEW_OF_ACROSS_HAP_FLAG[] = "[preview]";

    static constexpr size_t MAX_PACKGE_LEVEL = 1;
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
        if (fileName.find("//") == CString::npos && fileName.find("../") == CString::npos) {
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
                if (elem.compare("..") == 0 && !elems.empty()) {
                    elems.pop_back();
                    prev = curr + 1;
                    curr = fileName.find(delim, prev);
                    continue;
                }
                elems.push_back(elem);
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
            ASSERT(pos != CString::npos);
            CString moduleName = inputFileName.substr(startStrLen, pos - startStrLen);
            if (moduleName != vm->GetModuleName()) {
                outFileName = BUNDLE_INSTALL_PATH + moduleName + MERGE_ABC_ETS_MODULES;
            }
            entryPoint = vm->GetBundleName() + "/" + inputFileName.substr(startStrLen);
        } else {
            // Temporarily handle the relative path sent by arkui
            if (StringStartWith(inputFileName, PREFIX_BUNDLE, PREFIX_BUNDLE_LEN)) {
                entryPoint = inputFileName.substr(PREFIX_BUNDLE_LEN);
                outFileName = ParseNewPagesUrl(vm, entryPoint);
            } else {
                entryPoint = vm->GetBundleName() + "/" + vm->GetModuleName() + MODULE_DEFAULE_ETS + inputFileName;
            }
        }
        if (StringEndWith(entryPoint, EXT_NAME_ABC, EXT_NAME_ABC_LEN)) {
            entryPoint.erase(entryPoint.length() - EXT_NAME_ABC_LEN, EXT_NAME_ABC_LEN);
        }
        return entryPoint;
    }

    static CString ParseNewPagesUrl(EcmaVM *vm, const CString &entryPoint)
    {
        auto errorFun = [entryPoint](const size_t &pos) {
            if (pos == CString::npos) {
                LOG_ECMA(FATAL) << "ParseNewPagesUrl failed, please check Url " << entryPoint;
            }
        };
        size_t bundleEndPos = entryPoint.find('/');
        errorFun(bundleEndPos);
        CString bundleName = entryPoint.substr(0, bundleEndPos);
        size_t moduleStartPos = bundleEndPos + 1;
        size_t moduleEndPos = entryPoint.find('/', moduleStartPos);
        errorFun(moduleEndPos);
        CString moduleName = entryPoint.substr(moduleStartPos, moduleEndPos - moduleStartPos);
        CString baseFileName;
        if (bundleName != vm->GetBundleName()) {
            // Cross-application
            baseFileName =
                BUNDLE_INSTALL_PATH + bundleName + "/" + moduleName + "/" + moduleName + MERGE_ABC_ETS_MODULES;
        } else if (moduleName != vm->GetModuleName()) {
            // Intra-application cross hap
            baseFileName = BUNDLE_INSTALL_PATH + moduleName + MERGE_ABC_ETS_MODULES;
        } else {
            baseFileName = "";
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
        return "";
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

    static bool StringStartWith(const CString& str, const CString& startStr, size_t startStrLen)
    {
        return ((str.length() >= startStrLen) && (str.compare(0, startStrLen, startStr) == 0));
    }

    static bool StringEndWith(const CString& str, const CString& endStr, size_t endStrLen)
    {
        size_t len = str.length();
        return ((len >= endStrLen) && (str.compare(len - endStrLen, endStrLen, endStr) == 0));
    }

    static void SplitString(const CString& str, CVector<CString>& out, size_t startPos, size_t times, char c = '/')
    {
        size_t left = startPos;
        size_t pos = 0;
        size_t index = 0;
        while ((pos = str.find(c, left)) != CString::npos) {
            if (index >= times) {
                return;
            }
            out.emplace_back(str.substr(left, pos - left));
            left = pos + 1;
            index++;
        }
        if (index < times && left < str.length()) {
            out.emplace_back(str.substr(left));
        }
    }

    static CString ParsePreixBundle(const JSPandaFile *jsPandaFile, CString &baseFilename, CString moduleRecordName,
                                    CString moduleRequestName)
    {
        CString entryPoint;
        moduleRequestName = moduleRequestName.substr(PREFIX_BUNDLE_LEN);
        entryPoint = moduleRequestName;
        if (jsPandaFile->IsNewRecord()) {
            CVector<CString> vec;
            size_t index = 0;
            SplitString(moduleRequestName, vec, 0, SEGMENTS_LIMIT_TWO);
            if (vec.size() < SEGMENTS_LIMIT_TWO) {
                LOG_ECMA(ERROR) << "SplitString filed, please check moduleRequestName";
                return CString();
            }
            CString bundleName = vec[index++];
            if (!StringStartWith(moduleRecordName, bundleName, bundleName.size())) {
                CString moduleName = vec[index];
                baseFilename = BUNDLE_INSTALL_PATH + bundleName + '/' + moduleName + '/' + moduleName +
                    MERGE_ABC_ETS_MODULES;
            }
        } else {
            CroppingRecord(entryPoint);
        }
        return entryPoint;
    }

    static CString ParsePreixModule([[maybe_unused]] CString &baseFilename, [[maybe_unused]] CString moduleRecordName,
                                    [[maybe_unused]] CString moduleRequestName)
    {
        CString entryPoint;
#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS)
        moduleRequestName = moduleRequestName.substr(PREFIX_MODULE_LEN);
        CVector<CString> vec;
        SplitString(moduleRecordName, vec, 0, 1);
        SplitString(moduleRequestName, vec, 0, 1);
        if (vec.size() < SEGMENTS_LIMIT_TWO) {
            LOG_ECMA(ERROR) << "SplitString filed, please check moduleRequestName and moduleRecordName";
            return CString();
        }
        size_t index = 0;
        CString bundleName = vec[index++];
        CString moduleName = vec[index];
        baseFilename = BUNDLE_INSTALL_PATH + moduleName + MERGE_ABC_ETS_MODULES;
        entryPoint = bundleName + '/' + moduleRequestName;
#else
        entryPoint = PREVIEW_OF_ACROSS_HAP_FLAG;
        LOG_NO_TAG(ERROR) << "[ArkRuntime Log] Importing shared package is not supported in the Previewer.";
#endif
        return entryPoint;
    }

    static CString MakeNewRecord(const JSPandaFile *jsPandaFile, CString &baseFilename, const CString &recordName,
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
        if (jsPandaFile->HasRecord(entryPoint)) {
            return entryPoint;
        }
        // Possible import directory
        entryPoint += NPM_ENTRY_FILE;
        if (jsPandaFile->HasRecord(entryPoint)) {
            return entryPoint;
        }
        entryPoint = ParseThirdPartyPackge(jsPandaFile, recordName, requestName);
        if (jsPandaFile->HasRecord(entryPoint)) {
            return entryPoint;
        }
        // Execute abc locally
        pos = baseFilename.rfind('/');
        if (pos != CString::npos) {
            baseFilename = baseFilename.substr(0, pos + 1) + moduleRequestName + EXT_NAME_ABC;
        } else {
            baseFilename = moduleRequestName + EXT_NAME_ABC;
        }
        pos = moduleRequestName.rfind('/');
        if (pos != CString::npos) {
            entryPoint = moduleRequestName.substr(pos + 1);
        } else {
            entryPoint = moduleRequestName;
        }
        return entryPoint;
    }

    static CString FindNpmEntryPoint(const JSPandaFile *jsPandaFile, CString &npmPackage)
    {
        CString entryPoint = jsPandaFile->FindNpmEntryPoint(npmPackage);
        if (!entryPoint.empty()) {
            return entryPoint;
        }
        npmPackage += NPM_ENTRY_FILE;
        entryPoint = jsPandaFile->FindNpmEntryPoint(npmPackage);
        if (!entryPoint.empty()) {
            return entryPoint;
        }
        return CString();
    }

    static CString FindPackageInTopLevel(const JSPandaFile *jsPandaFile, const CString& requestName,
                                         const CString &packagePath)
    {
        CString entryPoint;
        for (size_t level = 0; level <= MAX_PACKGE_LEVEL; ++level) {
            CString levelStr = std::to_string(level).c_str();
            CString key = packagePath + "/" + levelStr + '/' + requestName;
            entryPoint = FindNpmEntryPoint(jsPandaFile, key);
            if (!entryPoint.empty()) {
                return entryPoint;
            }
        }
        return CString();
    }

    static CString ParseThirdPartyPackge(const JSPandaFile *jsPandaFile, const CString &recordName,
                                         const CString &requestName, const CString &packagePath)
    {
        CString entryPoint;
        size_t pos = recordName.find(packagePath);
        CString key = "";
        if (pos != CString::npos) {
            auto info = const_cast<JSPandaFile *>(jsPandaFile)->FindRecordInfo(recordName);
            CString PackageName = info.npmPackageName;
            while ((pos = PackageName.rfind(packagePath)) != CString::npos) {
                key = PackageName + '/' + packagePath + "/" + requestName;
                entryPoint = FindNpmEntryPoint(jsPandaFile, key);
                if (!entryPoint.empty()) {
                    return entryPoint;
                }
                PackageName = PackageName.substr(0, pos > 0 ? pos - 1 : 0);
            }
        }
        entryPoint = FindPackageInTopLevel(jsPandaFile, requestName, packagePath);
        return entryPoint;
    }

    static CString ParseThirdPartyPackge(const JSPandaFile *jsPandaFile, const CString &recordName,
                                         const CString &requestName)
    {
        CString entryPoint = ParseThirdPartyPackge(jsPandaFile, recordName, requestName, PACKAGE_PATH_SEGMENT);
        if (!entryPoint.empty()) {
            return entryPoint;
        }
        return ParseThirdPartyPackge(jsPandaFile, recordName, requestName, NPM_PATH_SEGMENT);
    }

    static bool IsImportFile(const CString &moduleRequestName)
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

    static CString ConcatFileNameWithMerge(const JSPandaFile *jsPandaFile, CString &baseFilename,
                                           CString recordName, CString requestName)
    {
        CString entryPoint;
        if (StringStartWith(requestName, PREFIX_BUNDLE, PREFIX_BUNDLE_LEN)) {
            entryPoint = ParsePreixBundle(jsPandaFile, baseFilename, recordName, requestName);
        } else if (StringStartWith(requestName, PREFIX_MODULE, PREFIX_MODULE_LEN)) {
            entryPoint = ParsePreixModule(baseFilename, recordName, requestName);
        } else if (StringStartWith(requestName, PREFIX_PACKAGE, PREFIX_PACKAGE_LEN)) {
            entryPoint = requestName.substr(PREFIX_PACKAGE_LEN);
        } else if (IsImportFile(requestName)) {
            entryPoint = MakeNewRecord(jsPandaFile, baseFilename, recordName, requestName);
        } else {
            entryPoint = ParseThirdPartyPackge(jsPandaFile, recordName, requestName);
        }
        if (!entryPoint.empty()) {
            return entryPoint;
        }
        LOG_ECMA(FATAL) << "Failed to resolve the requested entryPoint. BaseFilename : '" << baseFilename <<
                           "'. RecordName : '" <<  recordName <<
                           "'. RequestName : '" <<  requestName << "'.";
        UNREACHABLE();
    }
};
}  // namespace panda::ecmascript::base
#endif  // ECMASCRIPT_BASE_PATH_HELPER_H