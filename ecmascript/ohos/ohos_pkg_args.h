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

#ifndef ECMASCRIPT_OHOS_OHOS_PKG_ARGS_H
#define ECMASCRIPT_OHOS_OHOS_PKG_ARGS_H

#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <fcntl.h>

#include "ecmascript/base/json_helper.h"
#include "ecmascript/base/json_parser.h"
#include "ecmascript/compiler/aot_compiler_preprocessor.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/log.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/ohos/enable_aot_list_helper.h"
#include "ecmascript/ohos/framework_helper.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/platform/file.h"
#if defined(CODE_ENCRYPTION_ENABLE)
#include "ecmascript/ohos/code_decrypt.h"
#endif

namespace panda::ecmascript::kungfu {
using TransformType = ecmascript::base::JsonHelper::TransformType;
class OhosPkgArgs {
public:
    constexpr static const char *const KEY_BUNDLE_NAME = "bundleName";
    constexpr static const char *const KEY_MODULE_NAME = "moduleName";
    constexpr static const char *const KEY_PKG_PATH = "pkgPath";
    constexpr static const char *const KEY_FILE_NAME = "abcName";
    constexpr static const char *const KEY_ABC_OFFSET = "abcOffset";
    constexpr static const char *const KEY_ABC_SIZE = "abcSize";
    constexpr static const char *const KEY_PGO_DIR = "pgoDir";
    constexpr static const char *const KEY_PROCESS_UID = "processUid";
    constexpr static const char *const KEY_BUNDLE_UID = "bundleUid";
    constexpr static const char *const IS_ENCRYPTED_BUNDLE = "isEncryptedBundle";
    constexpr static const char *const APP_IDENTIFIER = "appIdentifier";

    OhosPkgArgs() = default;
    ~OhosPkgArgs()
    {
        if (GetPkgFd() != -1) {
            Close(reinterpret_cast<fd_t>(GetPkgFd()));
            SetPkgFd(-1);
        }
    }

    static bool ParseArgs(AotCompilerPreprocessor &preProcessor, CompilationOptions &cOptions)
    {
        ASSERT(preProcessor.runtimeOptions_.IsTargetCompilerMode());
        std::shared_ptr<OhosPkgArgs> pkgArgs = std::make_shared<OhosPkgArgs>();
        if (!preProcessor.runtimeOptions_.GetCompilerPkgJsonInfo().empty()) {
            if (!pkgArgs->ParseFromJson(preProcessor.vm_, preProcessor.runtimeOptions_.GetCompilerPkgJsonInfo())) {
                return false;
            }
            LOG_COMPILER(INFO) << "Parse main pkg info success.";
            preProcessor.mainPkgName_ = pkgArgs->GetFullName();
            preProcessor.pkgsArgs_[preProcessor.mainPkgName_] = pkgArgs;
            if (!ParseProfilerPath(pkgArgs, preProcessor, cOptions)) {
                return false;
            }
        }
        if (preProcessor.runtimeOptions_.GetCompilerEnableExternalPkg() &&
            !preProcessor.runtimeOptions_.GetCompilerExternalPkgJsonInfo().empty()) {
            OhosPkgArgs::ParseListFromJson(preProcessor.vm_,
                                           preProcessor.runtimeOptions_.GetCompilerExternalPkgJsonInfo(),
                                           preProcessor.pkgsArgs_);
        }
        for (const auto &pkgInfo : preProcessor.pkgsArgs_) {
            preProcessor.pandaFileNames_.emplace_back(pkgInfo.first);
            pkgInfo.second->Dump();
        }
        JSThread *thread = preProcessor.vm_->GetJSThread();
        FrameworkHelper frameworkHelper(thread);
        auto &frameworkAbcFiles = frameworkHelper.GetFrameworkAbcFiles();
        for (const auto &abcPath : frameworkAbcFiles) {
            preProcessor.pandaFileNames_.emplace_back(abcPath);
        }
        return true;
    }
#if defined(CODE_ENCRYPTION_ENABLE)
    void DecryptSetKey(int fd) const
    {
        if (GetIsEncryptedBundle() <= 0) {
            return;
        }
        if (ohos::DecryptSetKey(fd, static_cast<int>(GetBundleUid())) < 0) {
            LOG_ECMA(ERROR) << "set key error!";
        }
        if (ohos::DecryptAssociateKey(fd, static_cast<int>(GetProcessUid()),
                                      static_cast<int>(GetBundleUid())) < 0) {
            LOG_ECMA(ERROR) << "associate key error!";
        }
    }

    void DecryptRemoveKey(int fd) const
    {
        if (GetIsEncryptedBundle() <= 0) {
            return;
        }
        if (ohos::DecrypRemoveKey(fd, static_cast<int>(GetProcessUid())) < 0
         || ohos::DecrypRemoveKey(fd, static_cast<int>(GetBundleUid())) < 0) {
            LOG_ECMA(ERROR) << "remove key error!";
        }
    }

#endif
    bool GetJSPandaFileinfo(const JSRuntimeOptions &runtimeOptions, std::string &hapPath,
                            uint32_t &offset, uint32_t &size, std::string &realPath) const
    {
        if (Valid()) {
            hapPath = GetPath();
            offset = GetOffset();
            size = GetSize();
        } else {
            // for legacy params
            hapPath = runtimeOptions.GetHapPath();
            offset = runtimeOptions.GetHapAbcOffset();
            size = runtimeOptions.GetHapAbcSize();
        }
        if (size == 0) {
            LOG_ECMA(ERROR) << "buffer is empty in target compiler mode!";
            return false;
        }
        if (!RealPath(hapPath, realPath, false)) {
            LOG_ECMA(ERROR) << "realpath for hap path failed!";
            return false;
        }
        return true;
    }

    bool GetJSPandaFile(const JSRuntimeOptions &runtimeOptions, std::shared_ptr<JSPandaFile> &pf,
        [[maybe_unused]] int HapVerifyFd) const
    {
        std::string hapPath;
        uint32_t offset {};
        uint32_t size {};
        std::string realPath;
        if (!GetJSPandaFileinfo(runtimeOptions, hapPath, offset, size, realPath)) {
            return false;
        }
#if defined(CODE_ENCRYPTION_ENABLE)
        int fd = open(DEV_APP_CRYPTO_PATH, O_RDONLY);
        FdsanExchangeOwnerTag(reinterpret_cast<fd_t>(fd));
        DecryptSetKey(fd);
        uint32_t offStart = offset;
        offStart &= -PAGE_SIZE;
        MemMap fileMapMem = FileMapForAlignAddressByFd(HapVerifyFd, PAGE_PROT_READ, offset, offStart);
        offset = offset - offStart;
        if (fileMapMem.GetOriginAddr() == nullptr) {
            Close(reinterpret_cast<fd_t>(fd));
        }
#else
        MemMap fileMapMem = FileMap(realPath.c_str(), FILE_RDONLY, PAGE_PROT_READ);
#endif
        if (fileMapMem.GetOriginAddr() == nullptr) {
            LOG_ECMA(ERROR) << "File mmap failed";
            return false;
        }
        uint8_t *buffer = reinterpret_cast<uint8_t *>(fileMapMem.GetOriginAddr()) + offset;
        JSPandaFileManager *jsPandaFileManager = JSPandaFileManager::GetInstance();
        pf = jsPandaFileManager->OpenJSPandaFileFromBuffer(buffer, size, GetFullName().c_str());
        FileUnMap(fileMapMem);
        fileMapMem.Reset();
#if defined(CODE_ENCRYPTION_ENABLE)
        DecryptRemoveKey(fd);
        Close(reinterpret_cast<fd_t>(fd));
#endif
        return true;
    }

    static bool ParseListFromJson(EcmaVM *vm, const std::string &jsonInfo,
                                  std::map<std::string, std::shared_ptr<OhosPkgArgs>> &argsMap)
    {
        LocalScope scope(vm);
        ObjectFactory *factory = vm->GetFactory();
        auto *jsThread = vm->GetJSThread();
        ecmascript::base::Utf8JsonParser parser(jsThread, TransformType::NORMAL);

        JSHandle<EcmaString> handleStr = factory->NewFromASCII(jsonInfo.c_str());  // JSON Object
        JSHandle<JSTaggedValue> result = parser.Parse(handleStr);
        JSTaggedValue resultValue(static_cast<JSTaggedType>(result->GetRawData()));
        if (!resultValue.IsArray(jsThread)) {
            LOG_COMPILER(ERROR) << "Pkg list info parse failed. result is not an array. jsonData: " << jsonInfo.c_str();
            return false;
        }
        JSHandle<JSArray> valueHandle(jsThread, resultValue);
        JSHandle<TaggedArray> elements(jsThread, valueHandle->GetElements());
        JSMutableHandle<JSTaggedValue> entry(jsThread, JSTaggedValue::Undefined());
        JSMutableHandle<JSObject> entryHandle(jsThread, JSTaggedValue::Undefined());
        for (uint32_t i = 0; i < elements->GetLength(); i++) {
            entry.Update(elements->Get(i));
            if (entry->IsHole()) {
                continue;
            }
            std::shared_ptr<OhosPkgArgs> pkgInfo = std::make_shared<OhosPkgArgs>();
            JSTaggedValue entryValue(static_cast<JSTaggedType>(entry->GetRawData()));
            entryHandle.Update(entryValue);
            if (!pkgInfo->ParseFromJsObject(vm, entryHandle)) {
                LOG_COMPILER(ERROR) << "Pkg list entry info parse failed. jsonData: " << jsonInfo.c_str();
                return false;
            }
            argsMap[pkgInfo->GetFullName()] = pkgInfo;
        }
        return true;
    }

    bool ParseFromJson(EcmaVM *vm, const std::string &jsonInfo)
    {
        LocalScope scope(vm);
        ObjectFactory *factory = vm->GetFactory();
        auto *jsThread = vm->GetJSThread();
        ecmascript::base::Utf8JsonParser parser(jsThread, TransformType::NORMAL);

        JSHandle<EcmaString> handleStr(factory->NewFromASCII(jsonInfo.c_str()));  // JSON Object
        JSHandle<JSTaggedValue> result = parser.Parse(handleStr);
        JSTaggedValue resultValue(static_cast<JSTaggedType>(result->GetRawData()));
        if (!resultValue.IsECMAObject()) {
            LOG_COMPILER(ERROR) << "Pkg info parse failed. result is not an object. jsonData: " << jsonInfo.c_str();
            return false;
        }
        JSHandle<JSObject> valueHandle(jsThread, resultValue);
        return ParseFromJsObject(vm, valueHandle);
    }

    bool ParseFromJsObject(EcmaVM *vm, JSHandle<JSObject> &valueHandle)
    {
        LocalScope scope(vm);
        auto *jsThread = vm->GetJSThread();
        JSHandle<TaggedArray> nameList(JSObject::EnumerableOwnNames(jsThread, valueHandle));
        JSMutableHandle<JSTaggedValue> key(jsThread, JSTaggedValue::Undefined());
        JSMutableHandle<JSTaggedValue> value(jsThread, JSTaggedValue::Undefined());
        for (uint32_t i = 0; i < nameList->GetLength(); i++) {
            key.Update(nameList->Get(i));
            value.Update(JSObject::GetProperty(jsThread, valueHandle, key).GetValue());
            if (!key->IsString() || !value->IsString()) {
                LOG_COMPILER(ERROR) << "Pkg info parse from js object failed. key and value must be string type.";
                return false;
            }
            UpdateProperty(ConvertToString(*JSTaggedValue::ToString(jsThread, key)).c_str(),
                           ConvertToString(*JSTaggedValue::ToString(jsThread, value)).c_str());
        }
        return Valid();
    }

    void UpdateProperty(const char *key, const char *value)
    {
        if (strcmp(key, KEY_BUNDLE_NAME) == 0) {
            bundleName_ = value;
        } else if (strcmp(key, KEY_MODULE_NAME) == 0) {
            moduleName_ = value;
        } else if (strcmp(key, KEY_PKG_PATH) == 0) {
            pkgPath_ = value;
        } else if (strcmp(key, KEY_FILE_NAME) == 0) {
            abcName_ = value;
        } else if (strcmp(key, KEY_ABC_OFFSET) == 0) {
            char *str = nullptr;
            abcOffset_ = static_cast<uint32_t>(strtol(value, &str, 0));
        } else if (strcmp(key, KEY_ABC_SIZE) == 0) {
            char *str = nullptr;
            abcSize_ = static_cast<uint32_t>(strtol(value, &str, 0));
        } else if (strcmp(key, KEY_PGO_DIR) == 0) {
            pgoDir_ = value;
        } else if (strcmp(key, KEY_BUNDLE_UID) == 0) {
            char *str = nullptr;
            bundleUid_ = static_cast<uint32_t>(strtol(value, &str, 0));
        } else if (strcmp(key, KEY_PROCESS_UID) == 0) {
            char *str = nullptr;
            processUid_ = static_cast<uint32_t>(strtol(value, &str, 0));
        } else if (strcmp(key, IS_ENCRYPTED_BUNDLE) == 0) {
            char *str = nullptr;
            IsEncryptedBundle_ = static_cast<uint32_t>(strtol(value, &str, 0));
        } else if (strcmp(key, APP_IDENTIFIER) == 0) {
            appSignature_ = value;
        } else {
            LOG_COMPILER(ERROR) << "Unknown keyword when parse pkg info. key: " << key << ", value: " << value;
        }
    }

    bool Valid() const
    {
        if (!base::StringHelper::EndsWith(abcName_, ".abc")) {
            LOG_COMPILER(ERROR) << KEY_FILE_NAME << " must be abc file, but now is: " << abcName_;
            return false;
        }
        return !bundleName_.empty() && !moduleName_.empty() && !pkgPath_.empty() && (abcOffset_ != INVALID_VALUE) &&
               (abcSize_ != INVALID_VALUE);
    }

    void Dump() const
    {
        LOG_COMPILER(INFO) << "PkgInfo: "
                           << KEY_BUNDLE_NAME << ": " << bundleName_ << ", "
                           << KEY_MODULE_NAME << ": " << moduleName_ << ", "
                           << KEY_PKG_PATH << ": " << pkgPath_ << ", "
                           << KEY_ABC_OFFSET << ": " << std::hex << abcOffset_ << ", "
                           << KEY_ABC_SIZE << ": " << abcSize_ << ", "
                           << KEY_PGO_DIR << ": " << pgoDir_ << ", "
                           << KEY_BUNDLE_UID << ": " << bundleUid_ << ", "
                           << KEY_PROCESS_UID << ": " << processUid_ << ", "
                           << IS_ENCRYPTED_BUNDLE << ": " << IsEncryptedBundle_
                           << APP_IDENTIFIER << ": " << appSignature_;
    }

    const std::string &GetBundleName() const
    {
        return bundleName_;
    }

    const std::string &GetModuleName() const
    {
        return moduleName_;
    }

    const std::string &GetPath() const
    {
        return pkgPath_;
    }

    const std::string &GetAppSignature() const
    {
        return appSignature_;
    }

    std::string GetFullName() const
    {
        return pkgPath_ + GetPathSeparator() + moduleName_ + GetPathSeparator() + abcName_;
    }

    uint32_t GetOffset() const
    {
        return abcOffset_;
    }

    uint32_t GetSize() const
    {
        return abcSize_;
    }

    uint32_t GetBundleUid() const
    {
        return bundleUid_;
    }

    uint32_t GetProcessUid() const
    {
        return processUid_;
    }

    bool GetIsEncryptedBundle() const
    {
        return IsEncryptedBundle_;
    }

    const std::string &GetPgoDir() const
    {
        return pgoDir_;
    }

    void SetPgoDir(const std::string &pgoDir)
    {
        pgoDir_ = pgoDir;
    }

    void SetPkgFd(int fd)
    {
        pkgFd_ = fd;
    }

    int GetPkgFd() const
    {
        return pkgFd_;
    }

    void GetPgoPaths(bool isEnableBaselinePgo, std::string &pgoPaths, bool &needMerge) const
    {
        // 1. collect runtime ap and merged ap
        pgoPaths.clear();
        needMerge = false;
        pgoPaths = GetTargetApPaths();
        if (!pgoPaths.empty()) {
            needMerge = true;
            return;
        }

        // 2. Use the baseline AP if the runtime AP or merge ap do not exist and when install or update the application
        auto baselineAp = pgoDir_ + '/' + pgo::ApNameUtils::GetOhosPkgApName(moduleName_);
        if (isEnableBaselinePgo && FileExist(baselineAp.c_str())) {
            pgoPaths = baselineAp;
            LOG_COMPILER(DEBUG) << "Do not support base line ap now, please waiting. baseline ap: " << baselineAp;
        }
    }

    std::string GetRuntimeApPath() const
    {
        auto runtimeAp = pgoDir_ + '/' + pgo::ApNameUtils::GetRuntimeApName(moduleName_);
        if (!FileExist(runtimeAp.c_str())) {
            return "";
        }
        return runtimeAp;
    }

    std::string GetMergedApPathWithoutCheck() const
    {
        return pgoDir_ + '/' + pgo::ApNameUtils::GetMergedApName(moduleName_);
    }

    std::string GetMergedApPath() const
    {
        auto mergedAp = GetMergedApPathWithoutCheck();
        if (!FileExist(mergedAp.c_str())) {
            return "";
        }
        return mergedAp;
    }
private:
    static bool ParseProfilerPath(std::shared_ptr<OhosPkgArgs> &pkgArgs, AotCompilerPreprocessor &preProcessor,
                                  CompilationOptions &cOptions)
    {
        if (!preProcessor.runtimeOptions_.IsPartialCompilerMode()) {
            return true;
        }
        if (pkgArgs->GetPgoDir().empty() && !cOptions.profilerIn_.empty()) {
            // try get pgo dir from --compiler-pgo-profiler-path
            arg_list_t apFileNames = base::StringHelper::SplitString(cOptions.profilerIn_, GetFileDelimiter());
            ASSERT(!apFileNames.empty());
            // just parse the first ap's dir
            pkgArgs->SetPgoDir(ResolveDirPath(apFileNames.at(0)));
        }
        // reset profilerIn from pgo dir
        pkgArgs->GetPgoPaths(cOptions.isEnableBaselinePgo_, cOptions.profilerIn_, cOptions.needMerge_);
        if (cOptions.profilerIn_.empty()) {
            LOG_COMPILER(WARN) << "No available ap files found in " << pkgArgs->GetPgoDir();
        }
        return true;
    }

    /*
    * Before: xxx/xxx
    * After:  xxx
    */
    static std::string ResolveDirPath(const std::string &fileName)
    {
        // find last '/', '\\'
        auto foundPos = fileName.find_last_of("/\\");
        if (foundPos == std::string::npos) {
            return "";
        }
        return fileName.substr(0, foundPos);
    }

    std::string GetTargetApPaths() const
    {
        // handle merged ap
        std::string pgoPaths = GetMergedApPath();

        // handle runtime ap
        auto runtimeAp = GetRuntimeApPath();
        if (!runtimeAp.empty()) {
            if (!pgoPaths.empty()) {
                pgoPaths += GetFileDelimiter();
            }
            pgoPaths += runtimeAp;
        }
        return pgoPaths;
    }

    static constexpr uint32_t INVALID_VALUE = std::numeric_limits<uint32_t>::max();
    std::string bundleName_{""};
    std::string moduleName_{""};
    std::string pkgPath_{""};
    std::string abcName_{""};
    std::string pgoDir_{""};
    std::string appSignature_{""};
    uint32_t abcOffset_ {INVALID_VALUE};
    uint32_t abcSize_ {INVALID_VALUE};
    uint32_t bundleUid_ {INVALID_VALUE};
    uint32_t processUid_ {INVALID_VALUE};
    bool IsEncryptedBundle_{false};
    int pkgFd_ {-1};
};
}  // namespace panda::ecmascript::kungfu
#endif
