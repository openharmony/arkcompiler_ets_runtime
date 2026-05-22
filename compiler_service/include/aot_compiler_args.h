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

#ifndef OHOS_ARKCOMPILER_AOT_COMPILER_ARGS_H
#define OHOS_ARKCOMPILER_AOT_COMPILER_ARGS_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "parcel.h"

namespace OHOS::ArkCompiler {

// HSP external package info
struct HspModuleInfo : public Parcelable {
    std::string bundleName;
    std::string moduleName;
    std::string hapPath;
    std::string moduleArkTSMode;

    bool Marshalling(Parcel &parcel) const override
    {
        if (!parcel.WriteString(bundleName)) {
            return false;
        }
        if (!parcel.WriteString(moduleName)) {
            return false;
        }
        if (!parcel.WriteString(hapPath)) {
            return false;
        }
        if (!parcel.WriteString(moduleArkTSMode)) {
            return false;
        }
        return true;
    }

    static HspModuleInfo *Unmarshalling(Parcel &parcel)
    {
        auto info = std::make_unique<HspModuleInfo>();
        if (!parcel.ReadString(info->bundleName) ||
            !parcel.ReadString(info->moduleName) ||
            !parcel.ReadString(info->hapPath) ||
            !parcel.ReadString(info->moduleArkTSMode)) {
            return nullptr;
        }
        return info.release();
    }
};

// AOT compilation request arguments
struct AotCompilerArgs : public Parcelable {
    // === Identity ===
    bool isSysComp = false;
    std::string compileMode;       // "partial" / "full"
    std::string moduleArkTSMode;   // "dynamic" / "static" / "hybrid"

    // === Package info ===
    std::string bundleName;
    std::string hostBundleName;    // Host bundle name for host-private shared HSP AOT
    std::string moduleName;
    std::string appIdentifier;     // Code sign ownerID
    int32_t bundleUid = -1;
    int32_t bundleGid = -1;
    int32_t processUid = -1;       // Caller process UID

    // === File paths ===
    std::string hapPath;           // HAP package path
    std::string abcPath;           // ABC file full path
    std::string anFileName;        // .an file full path
    std::string outputPath;        // ark_cache output root dir
    std::string sysCompPath;       // System component ABC path (isSysComp=true)
    std::string pgoDir;            // PGO directory

    // === Compilation options ===
    std::string optBCRangeList;
    uint32_t isScreenOff = 0;
    uint32_t isEncryptedBundle = 0;
    uint32_t isEnableBaselinePgo = 0;

    // === Extra int fields (Static AOT, may be 0 if unused) ===
    int32_t bundleType = 0;               // BundleType enum value
    int32_t triggerType = 0;              // AotTriggerType enum value

    // === External packages (HSP) ===
    std::vector<HspModuleInfo> hspModules;

    bool Marshalling(Parcel &parcel) const override
    {
        return WriteIdentity(parcel) && WritePackageInfo(parcel) &&
               WriteFilePaths(parcel) &&
               WriteCompilationOptions(parcel) && WriteExtraInts(parcel) &&
               WriteHspModules(parcel);
    }

    static AotCompilerArgs *Unmarshalling(Parcel &parcel)
    {
        auto args = std::make_unique<AotCompilerArgs>();
        if (!ReadIdentity(parcel, args.get()) ||
            !ReadPackageInfo(parcel, args.get()) ||
            !ReadFilePaths(parcel, args.get()) ||
            !ReadCompilationOptions(parcel, args.get()) ||
            !ReadExtraInts(parcel, args.get()) ||
            !ReadHspModules(parcel, args.get())) {
            return nullptr;
        }
        return args.release();
    }

private:
    bool WriteIdentity(Parcel &parcel) const
    {
        return parcel.WriteBool(isSysComp) &&
               parcel.WriteString(compileMode) &&
               parcel.WriteString(moduleArkTSMode);
    }

    bool WritePackageInfo(Parcel &parcel) const
    {
        return parcel.WriteString(bundleName) &&
               parcel.WriteString(hostBundleName) &&
               parcel.WriteString(moduleName) &&
               parcel.WriteString(appIdentifier) &&
               parcel.WriteInt32(bundleUid) &&
               parcel.WriteInt32(bundleGid) &&
               parcel.WriteInt32(processUid);
    }

    bool WriteFilePaths(Parcel &parcel) const
    {
        return parcel.WriteString(hapPath) &&
               parcel.WriteString(abcPath) &&
               parcel.WriteString(anFileName) &&
               parcel.WriteString(outputPath) &&
               parcel.WriteString(sysCompPath) &&
               parcel.WriteString(pgoDir);
    }

    bool WriteCompilationOptions(Parcel &parcel) const
    {
        return parcel.WriteString(optBCRangeList) &&
               parcel.WriteUint32(isScreenOff) &&
               parcel.WriteUint32(isEncryptedBundle) &&
               parcel.WriteUint32(isEnableBaselinePgo);
    }

    bool WriteExtraInts(Parcel &parcel) const
    {
        return parcel.WriteInt32(bundleType) &&
               parcel.WriteInt32(triggerType);
    }

    bool WriteHspModules(Parcel &parcel) const
    {
        if (!parcel.WriteInt32(static_cast<int32_t>(hspModules.size()))) {
            return false;
        }
        for (const auto &hsp : hspModules) {
            if (!hsp.Marshalling(parcel)) {
                return false;
            }
        }
        return true;
    }

    static bool ReadIdentity(Parcel &parcel, AotCompilerArgs *args)
    {
        return parcel.ReadBool(args->isSysComp) &&
               parcel.ReadString(args->compileMode) &&
               parcel.ReadString(args->moduleArkTSMode);
    }

    static bool ReadPackageInfo(Parcel &parcel, AotCompilerArgs *args)
    {
        return parcel.ReadString(args->bundleName) &&
               parcel.ReadString(args->hostBundleName) &&
               parcel.ReadString(args->moduleName) &&
               parcel.ReadString(args->appIdentifier) &&
               parcel.ReadInt32(args->bundleUid) &&
               parcel.ReadInt32(args->bundleGid) &&
               parcel.ReadInt32(args->processUid);
    }

    static bool ReadFilePaths(Parcel &parcel, AotCompilerArgs *args)
    {
        return parcel.ReadString(args->hapPath) &&
               parcel.ReadString(args->abcPath) &&
               parcel.ReadString(args->anFileName) &&
               parcel.ReadString(args->outputPath) &&
               parcel.ReadString(args->sysCompPath) &&
               parcel.ReadString(args->pgoDir);
    }

    static bool ReadCompilationOptions(Parcel &parcel, AotCompilerArgs *args)
    {
        return parcel.ReadString(args->optBCRangeList) &&
               parcel.ReadUint32(args->isScreenOff) &&
               parcel.ReadUint32(args->isEncryptedBundle) &&
               parcel.ReadUint32(args->isEnableBaselinePgo);
    }

    static bool ReadExtraInts(Parcel &parcel, AotCompilerArgs *args)
    {
        return parcel.ReadInt32(args->bundleType) &&
               parcel.ReadInt32(args->triggerType);
    }

    static bool ReadHspModules(Parcel &parcel, AotCompilerArgs *args)
    {
        constexpr int32_t MAX_HSP_MODULE_SIZE = 1024;
        int32_t hspSize = 0;
        if (!parcel.ReadInt32(hspSize)) {
            return false;
        }
        if (hspSize < 0 || hspSize > MAX_HSP_MODULE_SIZE) {
            return false;
        }
        for (int32_t i = 0; i < hspSize; ++i) {
            auto hsp = HspModuleInfo::Unmarshalling(parcel);
            if (hsp == nullptr) {
                return false;
            }
            args->hspModules.push_back(*hsp);
            delete hsp;
        }
        return true;
    }
};

} // namespace OHOS::ArkCompiler
#endif // OHOS_ARKCOMPILER_AOT_COMPILER_ARGS_H
