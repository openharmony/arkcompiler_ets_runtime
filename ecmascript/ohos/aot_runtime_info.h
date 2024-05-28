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

#ifndef ECMASCRIPT_COMPILER_OHOS_RUNTIME_BUILD_INFO_H
#define ECMASCRIPT_COMPILER_OHOS_RUNTIME_BUILD_INFO_H

#include <sys/time.h>

#include "ecmascript/platform/directory.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/platform/map.h"
#include "ecmascript/ohos/aot_crash_info.h"
#include "libpandafile/file.h"
#include "llvm/BinaryFormat/ELF.h"
#include "ohos_constants.h"
#include "utils/json_parser.h"
#include "utils/json_builder.h"

namespace panda::ecmascript::ohos {
#define RUNTIME_INFO_TYPE(V)                                         \
    V(AOT)                                                           \
    V(OTHERS)                                                        \
    V(NONE)                                                          \
    V(JIT)                                                           \
    V(JS)                                                            \
    V(AOT_BUILD)                                                     \

enum class RuntimeInfoType {
    AOT,
    JIT,
    OTHERS,
    NONE,
    JS,
    AOT_BUILD,
};
struct RuntimeInfoPart {
    std::string buildId;
    std::string timestamp;
    std::string type;
    
    RuntimeInfoPart() = default;
    RuntimeInfoPart(const std::string &buildId, const std::string &timestamp, const std::string &type)
        : buildId(buildId), timestamp(timestamp), type(type)
    {
    }
};
/*
 * The JSON format is as follows
 * type: "AOT_BUILD" Used at compile time,
 *       "AOT", "JIT", "OTHERS", "JS" Used at crash time.
 * {"key":
 *      [
 *          {"buildId":"xxxxxx", "timestamp":"123456789", "type":"AOT"},
 *          {"buildId":"xxxxxx", "timestamp":"123456789", "type":"AOT"}
 *      ]
 * }
 */
class AotRuntimeInfo {
public:
    constexpr static const int USEC_PER_SEC = 1000 * 1000;
    constexpr static const int NSEC_PER_USEC = 1000;
    constexpr static const int NT_GNU_BUILD_ID = 3;
    constexpr static const int CRASH_INFO_SIZE = 3;
    
    constexpr static const char *const RUNTIME_INFO_ARRAY_KEY = "runtime_info_array";
    constexpr static const char *const RUNTIME_KEY_BUILDID = "buildId";
    constexpr static const char *const RUNTIME_KEY_TIMESTAMP = "timestamp";
    constexpr static const char *const RUNTIME_KEY_TYPE = "type";
    uint64_t GetMicrosecondsTimeStamp() const
    {
        struct timespec time;
        clock_gettime(CLOCK_MONOTONIC, &time);
        return time.tv_sec * USEC_PER_SEC + time.tv_nsec / NSEC_PER_USEC;
    }

    std::vector<RuntimeInfoPart> GetCrashRuntimeInfoList() const
    {
        std::string realOutPath = GetCrashSandBoxRealPath();
        std::vector<RuntimeInfoPart> list;
        if (!FileExist(realOutPath.c_str())) {
            LOG_ECMA(INFO) << "Get crash sanbox path fail.";
            return list;
        }
        std::string soBuildId = GetRuntimeBuildId();
        if (soBuildId == "") {
            LOG_ECMA(INFO) << "can't get so buildId.";
            return list;
        }
        list = GetRuntimeInfoByPath(realOutPath, soBuildId);
        return list;
    }

    std::vector<RuntimeInfoPart> GetRealPathRuntimeInfoList(const std::string &pgoRealPath) const
    {
        std::vector<RuntimeInfoPart> list;
        std::string realOutPath;
        std::string runtimePgoRealPath = pgoRealPath + OhosConstants::PATH_SEPARATOR +
            OhosConstants::AOT_RUNTIME_INFO_NAME;
        if (!ecmascript::RealPath(runtimePgoRealPath, realOutPath, false)) {
            return list;
        }
        if (!FileExist(realOutPath.c_str())) {
            LOG_ECMA(INFO) << "Get pgo real path fail.";
            return list;
        }
        std::string soBuildId = GetRuntimeBuildId();
        if (soBuildId == "") {
            LOG_ECMA(INFO) << "can't get so buildId.";
            return list;
        }
        list = GetRuntimeInfoByPath(realOutPath, soBuildId);
        return list;
    }

    void BuildCompileRuntimeInfo(const std::string &type, const std::string &pgoPath) const
    {
        std::string soBuildId = GetRuntimeBuildId();
        if (soBuildId == "") {
            LOG_ECMA(INFO) << "can't get so buildId.";
            return;
        }
        std::string realOutPath;
        std::string runtimePgoRealPath = pgoPath + OhosConstants::PATH_SEPARATOR + OhosConstants::AOT_RUNTIME_INFO_NAME;
        if (!ecmascript::RealPath(runtimePgoRealPath, realOutPath, false)) {
            LOG_ECMA(INFO) << "Build compile pgo path fail.";
            return;
        }
        std::vector<RuntimeInfoPart> lines = GetRuntimeInfoByPath(realOutPath, soBuildId);
        uint64_t timestamp = GetMicrosecondsTimeStamp();
        RuntimeInfoPart buildLine = RuntimeInfoPart(soBuildId, std::to_string(timestamp), type);
        lines.emplace_back(buildLine);
        SetRuntimeInfo(realOutPath, lines);
    }

    void BuildCrashRuntimeInfo(const std::string &type) const
    {
        std::string soBuildId = GetRuntimeBuildId();
        if (soBuildId == "") {
            LOG_ECMA(INFO) << "can't get so buildId.";
            return;
        }
        std::vector<RuntimeInfoPart> lines = GetCrashRuntimeInfoList();
        uint64_t timestamp = GetMicrosecondsTimeStamp();
        RuntimeInfoPart buildLine = RuntimeInfoPart(soBuildId, std::to_string(timestamp), type);
        lines.emplace_back(buildLine);
        std::string realOutPath = GetCrashSandBoxRealPath();
        if (realOutPath == "") {
            LOG_ECMA(INFO) << "Get crash sanbox path fail.";
            return;
        }
        SetRuntimeInfo(realOutPath, lines);
    }

    int GetCompileCountByType(RuntimeInfoType type)
    {
        std::map<RuntimeInfoType, int> escapeMap = CollectCrashSum();
        if (escapeMap.count(type) == 0) {
            return 0;
        }
        return escapeMap[type];
    }

    std::map<RuntimeInfoType, int> CollectCrashSum(const std::string &pgoRealPath = "")
    {
        std::map<RuntimeInfoType, int> escapeMap;
        std::vector<RuntimeInfoPart> runtimeInfoParts;
        if (pgoRealPath == "") {
            runtimeInfoParts = GetCrashRuntimeInfoList();
        } else {
            runtimeInfoParts = GetRealPathRuntimeInfoList(pgoRealPath);
        }
        for (const auto &runtimeInfoPart: runtimeInfoParts) {
            RuntimeInfoType runtimeInfoType = GetRuntimeInfoTypeByStr(runtimeInfoPart.type);
            escapeMap[runtimeInfoType]++;
        }
        return escapeMap;
    }

    std::string GetRuntimeBuildId() const
    {
        std::string realPath;
        if (!FileExist(OhosConstants::RUNTIME_SO_PATH)) {
            return "";
        }
        std::string soPath = panda::os::file::File::GetExtendedFilePath(OhosConstants::RUNTIME_SO_PATH);
        if (!ecmascript::RealPath(soPath, realPath, false)) {
            return "";
        }
        if (!FileExist(realPath.c_str())) {
            return "";
        }
        ecmascript::MemMap fileMap = ecmascript::FileMap(realPath.c_str(), FILE_RDONLY, PAGE_PROT_READ);
        if (fileMap.GetOriginAddr() == nullptr) {
            LOG_ECMA(INFO) << "runtime info file mmap failed";
            return "";
        }
        ecmascript::PagePreRead(fileMap.GetOriginAddr(), fileMap.GetSize());
        std::string buildId = ParseELFSectionsForBuildId(fileMap);
        ecmascript::FileUnMap(fileMap);
        fileMap.Reset();
        return buildId;
    }

    static std::string GetRuntimeInfoTypeStr(RuntimeInfoType type)
    {
        const std::map<RuntimeInfoType, const char *> strMap = {
#define RUNTIME_INFO_TYPE_MAP(name) { RuntimeInfoType::name, #name },
        RUNTIME_INFO_TYPE(RUNTIME_INFO_TYPE_MAP)
#undef RUNTIME_INFO_TYPE_MAP
        };
        if (strMap.count(type) > 0) {
            return strMap.at(type);
        }
        return "";
    }

    static RuntimeInfoType GetRuntimeInfoTypeByStr(std::string type)
    {
        const std::map<std::string, RuntimeInfoType> strMap = {
#define RUNTIME_INFO_TYPE_MAP(name) { #name, RuntimeInfoType::name },
        RUNTIME_INFO_TYPE(RUNTIME_INFO_TYPE_MAP)
#undef RUNTIME_INFO_TYPE_MAP
        };
        if (strMap.count(type) > 0) {
            return strMap.at(type);
        }
        return RuntimeInfoType::NONE;
    }
private:

    void SetRuntimeInfo(const std::string &realOutPath, std::vector<RuntimeInfoPart> &runtimeInfoParts) const
    {
        std::ofstream file(realOutPath.c_str(), std::ofstream::out);
        JsonObjectBuilder objBuilder;
        auto arrayObject = [runtimeInfoParts](JsonArrayBuilder &arrayBuilder) {
            for (const auto &runtimeInfoPart : runtimeInfoParts) {
                auto addObj = [runtimeInfoPart](JsonObjectBuilder &obj) {
                    obj.AddProperty(RUNTIME_KEY_BUILDID, runtimeInfoPart.buildId);
                    obj.AddProperty(RUNTIME_KEY_TIMESTAMP, runtimeInfoPart.timestamp);
                    obj.AddProperty(RUNTIME_KEY_TYPE, runtimeInfoPart.type);
                };
                arrayBuilder.Add(addObj);
            }
        };
        objBuilder.AddProperty(RUNTIME_INFO_ARRAY_KEY, arrayObject);
        if (file.is_open()) {
            file << std::move(objBuilder).Build();
            file.close();
        } else {
            LOG_ECMA(INFO) << "open file to set runtime info fail :" << realOutPath.c_str();
        }
    }

    std::vector<RuntimeInfoPart> GetRuntimeInfoByPath(const std::string &realOutPath,
        const std::string &soBuildId) const
    {
        std::ifstream ifile(realOutPath.c_str());
        std::vector<RuntimeInfoPart> infoParts;
        if (ifile.is_open()) {
            std::string jsonStr((std::istreambuf_iterator<char>(ifile)), std::istreambuf_iterator<char>());
            JsonObject obj(jsonStr);
            if (obj.GetValue<JsonObject::ArrayT>(RUNTIME_INFO_ARRAY_KEY) == nullptr) {
                ifile.close();
                return infoParts;
            }
            auto &mainArray = *obj.GetValue<JsonObject::ArrayT>(RUNTIME_INFO_ARRAY_KEY);
            for (size_t i = 0; i < mainArray.size(); i++) {
                if (mainArray[i].Get<JsonObject::JsonObjPointer>() == nullptr) {
                    continue;
                }
                auto &innerObj = *(mainArray[i]).Get<JsonObject::JsonObjPointer>()->get();
                auto buildId = innerObj.GetValue<JsonObject::StringT>(RUNTIME_KEY_BUILDID);
                auto timestamp = innerObj.GetValue<JsonObject::StringT>(RUNTIME_KEY_TIMESTAMP);
                auto type = innerObj.GetValue<JsonObject::StringT>(RUNTIME_KEY_TYPE);
                if (buildId == nullptr || timestamp == nullptr || type == nullptr) {
                    LOG_ECMA(INFO) << "runtime info get wrong json object info.";
                    continue;
                }
                std::string buildIdStr = *buildId;
                if (buildIdStr != soBuildId) {
                    continue;
                }
                infoParts.emplace_back(RuntimeInfoPart(*buildId, *timestamp, *type));
            }
            ifile.close();
        }
        return infoParts;
    }

    std::string GetCrashSandBoxRealPath() const
    {
        std::string realOutPath;
        std::string arkProfilePath = AotCrashInfo::GetSandBoxPath();
        std::string sanboxPath = panda::os::file::File::GetExtendedFilePath(arkProfilePath);
        if (!ecmascript::RealPath(sanboxPath, realOutPath, false)) {
            return "";
        }
        realOutPath = realOutPath + OhosConstants::PATH_SEPARATOR + OhosConstants::AOT_RUNTIME_INFO_NAME;
        return realOutPath;
    }

    std::string ParseELFSectionsForBuildId(ecmascript::MemMap &fileMap) const
    {
        llvm::ELF::Elf64_Ehdr *ehdr = reinterpret_cast<llvm::ELF::Elf64_Ehdr *>(fileMap.GetOriginAddr());
        char *addr = reinterpret_cast<char *>(ehdr);
        llvm::ELF::Elf64_Shdr *shdr = reinterpret_cast<llvm::ELF::Elf64_Shdr *>(addr + ehdr->e_shoff);
        ASSERT(ehdr->e_shstrndx != static_cast<llvm::ELF::Elf64_Half>(-1));
        llvm::ELF::Elf64_Shdr strdr = shdr[ehdr->e_shstrndx];
        int secId = -1;
        std::string sectionName = ".note.gnu.build-id";
        for (size_t i = 0; i < ehdr->e_shnum; i++) {
            llvm::ELF::Elf64_Word shName = shdr[i].sh_name;
            char *curShName = reinterpret_cast<char *>(addr) + shName + strdr.sh_offset;
            if (sectionName.compare(curShName) == 0) {
                secId = static_cast<int>(i);
                break;
            }
        }
        if (secId == -1) {
            return "";
        }
        llvm::ELF::Elf64_Shdr secShdr = shdr[secId];
        uint64_t buildIdOffset = secShdr.sh_offset;
        uint64_t buildIdSize = secShdr.sh_size;
        llvm::ELF::Elf64_Nhdr *nhdr = reinterpret_cast<llvm::ELF::Elf64_Nhdr *>(addr + buildIdOffset);
        char *curShNameForNhdr = reinterpret_cast<char *>(addr + buildIdOffset + sizeof(*nhdr));
        if (buildIdSize - sizeof(*nhdr) < nhdr->n_namesz) {
            return "";
        }
        std::string curShNameStr = curShNameForNhdr;
        if (curShNameStr.back() == '\0') {
            curShNameStr.resize(curShNameStr.size() - 1);
        }

        if (curShNameStr != "GNU" || nhdr->n_type != NT_GNU_BUILD_ID) {
            return "";
        }
        if ((buildIdSize - sizeof(*nhdr) - AlignValues(nhdr->n_namesz, 4) < nhdr->n_descsz) || nhdr->n_descsz == 0) {
            return "";
        }
        
        char *curShNameValueForNhdr = reinterpret_cast<char *>(addr + buildIdOffset + sizeof(*nhdr) +
            AlignValues(nhdr->n_namesz, 4));
        std::string curShNameValueForNhdrStr = curShNameValueForNhdr;
        return GetReadableBuildId(curShNameValueForNhdrStr);
    }

    std::string GetReadableBuildId(std::string &buildIdHex) const
    {
        if (buildIdHex.empty()) {
            return "";
        }
        static const char HEXTABLE[] = "0123456789abcdef";
        static const int HEXLENGTH = 16;
        static const int HEX_EXPAND_PARAM = 2;
        const size_t len = buildIdHex.length();
        std::string buildId(len * HEX_EXPAND_PARAM, '\0');

        for (size_t i = 0; i < len; i++) {
            unsigned int n = buildIdHex[i];
            buildId[i * HEX_EXPAND_PARAM] = HEXTABLE[(n >> 4) % HEXLENGTH]; // 4 : higher 4 bit of uint8
            buildId[i * HEX_EXPAND_PARAM + 1] = HEXTABLE[n % HEXLENGTH];
        }
        return buildId;
    }

    uint64_t AlignValues(uint64_t val, uint64_t align) const
    {
        return (val + AlignBytes(align)) & AlignMask(align);
    }

    uint64_t AlignMask(uint64_t align) const
    {
        return ~(static_cast<uint64_t>((align) - 1));
    }

    uint64_t AlignBytes(uint64_t align) const
    {
        return (align) - 1;
    }
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_COMPILER_OHOS_RUNTIME_BUILD_INFO_H