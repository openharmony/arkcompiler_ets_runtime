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

#ifndef ECMASCRIPT_OHOS_OHOS_PKG_VERIFIER_H
#define ECMASCRIPT_OHOS_OHOS_PKG_VERIFIER_H

#include <fcntl.h>

#include "ecmascript/compiler/aot_compiler_preprocessor.h"
#include "ecmascript/ohos/ohos_pkg_args.h"
#include "ecmascript/platform/file.h"

namespace panda::ecmascript::ohos {
using AotCompilerPreprocessor = kungfu::AotCompilerPreprocessor;
using CompilationOptions = kungfu::CompilationOptions;
using OhosPkgArgs = kungfu::OhosPkgArgs;
using ParseBundleNameAndAppIdentifierFunc = int32_t (*)(const int32_t, std::string&, std::string&);

class OhosPkgVerifier {
public:
    constexpr static const char *const LIB_HAPVERIFY_SO_NAME = "libhapverify.z.so";

    static bool VerifyPkgInfo(AotCompilerPreprocessor &cPreprocessor, CompilationOptions &cOptions)
    {
        // Processing only for application packages
        if (!cPreprocessor.GetMainPkgArgs()) {
            return true;
        }
        if (CheckBundleNameAndAppIdentifier(cPreprocessor) &&
            CheckArkProfile(cPreprocessor) &&
            CheckAOTOutputFilePath(cPreprocessor, cOptions)) {
            return true;
        }
        return false;
    }

    static bool CheckArkProfile(AotCompilerPreprocessor &cPreprocessor)
    {
        std::regex pattern("/data/app/el1/\\d+/aot_compiler/ark_profile.*");
        std::string pgoDir = cPreprocessor.GetMainPkgArgs()->GetPgoDir();
        std::string bundleName = cPreprocessor.GetMainPkgArgs()->GetBundleName();
        if (!std::regex_search(pgoDir, pattern) ||
            pgoDir.find(bundleName) == std::string::npos) {
            LOG_COMPILER(ERROR) << "verify ark-profile path wrong";
            return false;
        }
        return true;
    }

    static bool CheckAOTOutputFilePath(AotCompilerPreprocessor &cPreprocessor, CompilationOptions &cOptions)
    {
        std::string arkCachePath = "/data/app/el1/public/aot_compiler/ark_cache/";
        if (cOptions.outputFileName_.length() < arkCachePath.length()) {
            LOG_COMPILER(ERROR) << "outputFileName_ is too short";
            return false;
        }
        if (cOptions.outputFileName_.substr(0, arkCachePath.length()) != arkCachePath) {
            LOG_COMPILER(ERROR) << "aot file name wrong";
            return false;
        }
        std::shared_ptr<OhosPkgArgs> mainPkgArgs = cPreprocessor.GetMainPkgArgs();
        if (!mainPkgArgs) {
            LOG_COMPILER(ERROR) << "mainPkgArgs is nullptr";
            return false;
        }
        const std::string bundleName = mainPkgArgs->GetBundleName();
        if (bundleName.empty()) {
            LOG_COMPILER(ERROR) << "bundleName is empty";
            return false;
        }
        if (cOptions.outputFileName_.find(bundleName) == std::string::npos) {
            LOG_COMPILER(ERROR) << "verify main pkg bundleName wrong";
            return false;
        }
        return true;
    }

    static bool CheckBundleNameAndAppIdentifier([[maybe_unused]] AotCompilerPreprocessor &cPreprocessor)
    {
#if defined(PANDA_TARGET_OHOS)
        void* handle = dlopen(LIB_HAPVERIFY_SO_NAME, RTLD_NOW);
        if (handle == nullptr) {
            LOG_COMPILER(ERROR) << "dlopen hapverify so fail";
            return false;
        }
        ParseBundleNameAndAppIdentifierFunc parseBundleNameAndAppIdentifierFunc =
            reinterpret_cast<ParseBundleNameAndAppIdentifierFunc>(dlsym(handle, "ParseBundleNameAndAppIdentifier"));
        if (parseBundleNameAndAppIdentifierFunc == nullptr) {
            LOG_COMPILER(ERROR) << "dlsym ParseBundleNameAndAppIdentifier fail";
            dlclose(handle);
            return false;
        }
        for (const auto& pkg : cPreprocessor.GetPkgsArgs()) {
            std::string realPath;
            if (!RealPath(pkg.second->GetPath().c_str(), realPath, true)) {
                LOG_ECMA(ERROR) << "Fail to get realPath: " << pkg.second->GetPath().c_str();
                dlclose(handle);
                return false;
            }
            fd_t fd = open(realPath.c_str(), FILE_RDONLY);
            if (fd == INVALID_FD) {
                LOG_ECMA(ERROR) << realPath.c_str() << " file open failed";
                dlclose(handle);
                return false;
            }
            FdsanExchangeOwnerTag(fd);
            pkg.second->SetPkgFd(fd);
            std::string bundleName;
            std::string appIdentifier;
            int32_t res = parseBundleNameAndAppIdentifierFunc(fd, bundleName, appIdentifier);
            if (res != 0 || bundleName != pkg.second->GetBundleName()) {
                dlclose(handle);
                LOG_COMPILER(ERROR) << "parse bundleName: " << bundleName.c_str() << " fail, res: " << res;
                return false;
            }
            // Only mainpkg carries appindetifier
            if (IsMainPkg(cPreprocessor, pkg) && appIdentifier != pkg.second->GetAppSignature()) {
                dlclose(handle);
                LOG_COMPILER(ERROR) << "parse mainPkgName: " << cPreprocessor.GetMainPkgName().c_str() <<
                    " appIdentifier fail";
                return false;
            }
        }
        dlclose(handle);
        return true;
#endif
        return false;
    }

    static bool IsMainPkg(AotCompilerPreprocessor &cPreprocessor, std::pair<const std::string,
        std::shared_ptr<OhosPkgArgs>> pkg)
    {
        if (cPreprocessor.GetMainPkgName() == pkg.first) {
            return true;
        }
        return false;
    }
};
}  // namespace panda::ecmascript::ohos
#endif
