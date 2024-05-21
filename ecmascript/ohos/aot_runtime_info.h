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
class AotRuntimeInfo {
public:
    constexpr static const int USEC_PER_SEC = 1000 * 1000;
    constexpr static const int NSEC_PER_USEC = 1000;
    constexpr static const int NT_GNU_BUILD_ID = 3;
    constexpr static const int CRASH_INFO_SIZE = 3;

    uint64_t GetMicrosecondsTimeStamp() const
    {
        struct timespec time;
        clock_gettime(CLOCK_MONOTONIC, &time);
        return time.tv_sec * USEC_PER_SEC + time.tv_nsec / NSEC_PER_USEC;
    }

    std::string BuildSingleInfo(const std::string &buildId, const std::string &timestamp, const std::string &type) const
    {
        return buildId + OhosConstants::SPLIT_STR + timestamp + OhosConstants::SPLIT_STR + type;
    }

    std::vector<std::string> GetCrashRuntimeInfoList(const std::string &soBuildId) const
    {
        std::string realOutPath = GetCrashSandBoxRealPath();
        std::vector<std::string> list;
        if (realOutPath == "") {
            LOG_ECMA(INFO) << "Get crash sanbox path fail.";
            return list;
        }
        list = GetRuntimeInfoByPath(realOutPath, soBuildId);
        return list;
    }

    void BuildCompileRuntimeInfo(const std::string &type, const std::string &pgoPath)
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
        std::vector<std::string> lines = GetRuntimeInfoByPath(realOutPath, soBuildId);
        uint64_t timestamp = GetMicrosecondsTimeStamp();
        std::string buildLine = BuildSingleInfo(soBuildId, std::to_string(timestamp), type);
        lines.emplace_back(buildLine);
        SetRuntimeInfo(realOutPath, lines);
    }

    void BuildCrashRuntimeInfo(const std::string &type)
    {
        std::string soBuildId = GetRuntimeBuildId();
        if (soBuildId == "") {
            LOG_ECMA(INFO) << "can't get so buildId.";
            return;
        }
        std::vector<std::string> lines = GetCrashRuntimeInfoList(soBuildId);
        uint64_t timestamp = GetMicrosecondsTimeStamp();
        std::string buildLine = BuildSingleInfo(soBuildId, std::to_string(timestamp), type);
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

    std::map<RuntimeInfoType, int> CollectCrashSum()
    {
        std::map<RuntimeInfoType, int> escapeMap;
        std::string soBuildId = GetRuntimeBuildId();
        if (soBuildId == "") {
            LOG_ECMA(INFO) << "can't get so buildId.";
            return escapeMap;
        }
        std::vector<std::string> lines = GetCrashRuntimeInfoList(soBuildId);
        for (const std::string &line: lines) {
            std::string type = GetType(line);
            RuntimeInfoType runtimeInfoType = GetRuntimeInfoTypeByStr(type);
            escapeMap[runtimeInfoType]++;
        }
        return escapeMap;
    }

    std::string GetRuntimeBuildId()
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
        fileMap = ecmascript::FileMap(realPath.c_str(), FILE_RDONLY, PAGE_PROT_READ);
        if (fileMap.GetOriginAddr() == nullptr) {
            return "";
        }
        ecmascript::PagePreRead(fileMap.GetOriginAddr(), fileMap.GetSize());
        return ParseELFSectionsForBuildId();
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

    void SetRuntimeInfo(const std::string &realOutPath, std::vector<std::string> lines) const
    {
        std::ofstream file(realOutPath.c_str(), std::ofstream::out);
        for (const std::string &line: lines) {
            file << line << "\n";
        }
        file.close();
    }

    std::vector<std::string> GetRuntimeInfoByPath(const std::string &realOutPath, const std::string &soBuildId) const
    {
        std::ifstream ifile(realOutPath.c_str());
        std::vector<std::string> lines;
        if (ifile.is_open()) {
            std::string iline;
            while (ifile >> iline) {
                std::string buildId = GetBuildId(iline);
                if (buildId != soBuildId) {
                    continue;
                }
                lines.emplace_back(iline);
            }
            ifile.close();
        }
        return lines;
    }
    
    std::string GetBuildId(std::string crashInfo) const
    {
        std::vector<std::string> splitCrashInfo = base::StringHelper::SplitString(crashInfo, OhosConstants::SPLIT_STR);
        if (splitCrashInfo.size() == CRASH_INFO_SIZE) {
            return splitCrashInfo[0];
        }
        return "";
    }

    std::string GetType(std::string crashInfo) const
    {
        std::vector<std::string> splitCrashInfo = base::StringHelper::SplitString(crashInfo, OhosConstants::SPLIT_STR);
        if (splitCrashInfo.size() == CRASH_INFO_SIZE) {
            return splitCrashInfo[2];
        }
        return "";
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

    std::string ParseELFSectionsForBuildId() const
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

    ecmascript::MemMap fileMap;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_COMPILER_OHOS_RUNTIME_BUILD_INFO_H