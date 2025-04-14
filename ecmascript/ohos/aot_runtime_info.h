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
#include <fcntl.h>
#include <mutex>
#include <stdio.h>

#include "ecmascript/platform/directory.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/platform/map.h"
#include "libpandafile/file.h"
#include "llvm/BinaryFormat/ELF.h"
#include "ohos_constants.h"
#include "utils/json_parser.h"
#include "utils/json_builder.h"

namespace panda::ecmascript::ohos {
#define RUNTIME_INFO_TYPE(V)                                         \
    V(AOT_CRASH)                                                     \
    V(OTHERS)                                                        \
    V(NONE)                                                          \
    V(JIT)                                                           \
    V(JS)                                                            \
    V(AOT_BUILD)                                                     \

enum class RuntimeInfoType {
    AOT_CRASH,
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
    constexpr static const int MAX_LENGTH = 255;
    constexpr static const int BUFFER_SIZE = 4096;
    constexpr static const int TIME_STAMP_SIZE = 21;

    constexpr static const int RUNTIME_INDEX_BUILDID = 0;
    constexpr static const int RUNTIME_INDEX_TIMESTAMP = 1;
    constexpr static const int RUNTIME_INDEX_TYPE = 2;

    static AotRuntimeInfo &GetInstance()
    {
        static AotRuntimeInfo singleAotRuntimeInfo;
        return singleAotRuntimeInfo;
    }

    void BuildCompileRuntimeInfo(RuntimeInfoType type, const std::string &pgoPath)
    {
        std::unique_lock<std::mutex> lock(fileMutex_);
        static char soBuildId[NAME_MAX] = { '\0' };
        if (!GetRuntimeBuildId(soBuildId, NAME_MAX) || IsCharEmpty(soBuildId)) {
            LOG_ECMA(INFO) << "can't get so buildId.";
            return;
        }
        std::string realOutPath;
        if (!ecmascript::RealPath(pgoPath, realOutPath, false)) {
            LOG_ECMA(INFO) << "Build compile pgo path fail.";
            return;
        }
        static char lines[MAX_LENGTH][BUFFER_SIZE];
        for (int i = 0; i < MAX_LENGTH; i++) {
            memset_s(lines[i], BUFFER_SIZE, '\0', BUFFER_SIZE);
        }
        GetRuntimeInfoByPath(lines, realOutPath.c_str(), soBuildId);
        static char timestamp[TIME_STAMP_SIZE] = { '\0' };
        if (!GetMicrosecondsTimeStamp(timestamp, TIME_STAMP_SIZE)) {
            return;
        }

        int lineCount = getLength(lines, MAX_LENGTH);
        if (lineCount < MAX_LENGTH) {
            if (!BuildRuntimeInfoPart(lines[lineCount], soBuildId, timestamp, type)) {
                return;
            }
        }
        SetRuntimeInfo(realOutPath.c_str(), lines, MAX_LENGTH);
    }

    void BuildCrashRuntimeInfo(RuntimeInfoType type)
    {
        std::unique_lock<std::mutex> lock(fileMutex_);
        static char soBuildId[NAME_MAX] = { '\0' };
        if (!GetRuntimeBuildId(soBuildId, NAME_MAX) || IsCharEmpty(soBuildId)) {
            return;
        }
        static char lines[MAX_LENGTH][BUFFER_SIZE];
        for (int i = 0; i < MAX_LENGTH; i++) {
            memset_s(lines[i], BUFFER_SIZE, '\0', BUFFER_SIZE);
        }
        GetCrashRuntimeInfoList(lines);
        static char timestamp[TIME_STAMP_SIZE] = { '\0' };
        if (!GetMicrosecondsTimeStamp(timestamp, TIME_STAMP_SIZE)) {
            return;
        }
        int lineCount = getLength(lines, MAX_LENGTH);
        if (lineCount < MAX_LENGTH) {
            if (!BuildRuntimeInfoPart(lines[lineCount], soBuildId, timestamp, type)) {
                return;
            }
        }
        static char realOutPath[PATH_MAX] = { '\0' };
        if (!GetCrashSandBoxRealPath(realOutPath, PATH_MAX) || IsCharEmpty(realOutPath)) {
            return;
        }
        SetRuntimeInfo(realOutPath, lines, MAX_LENGTH);
    }

    int GetCompileCountByType(RuntimeInfoType type, const std::string &pgoRealPath = "")
    {
        std::map<RuntimeInfoType, int> escapeMap = CollectCrashSum(pgoRealPath);
        if (escapeMap.count(type) == 0) {
            return 0;
        }
        return escapeMap[type];
    }

    std::map<RuntimeInfoType, int> CollectCrashSum(const std::string &pgoRealPath = "")
    {
        if (IsLoadedMap()) {
            return escapeMap_;
        }
        char lines[MAX_LENGTH][BUFFER_SIZE];
        for (int i = 0; i < MAX_LENGTH; i++) {
            memset_s(lines[i], BUFFER_SIZE, '\0', BUFFER_SIZE);
        }
        if (IsCharEmpty(pgoRealPath.c_str())) {
            GetCrashRuntimeInfoList(lines);
        } else {
            GetRealPathRuntimeInfoList(lines, pgoRealPath);
        }
        char *typeChar = new char[NAME_MAX];
        for (int i = 0; i < MAX_LENGTH; i++) {
            if (lines[i][0] != '\0') {
                if (strcpy_s(typeChar, NAME_MAX, GetInfoFromBuffer(lines[i], RUNTIME_INDEX_TYPE)) !=0) {
                    continue;
                }
                std::string typeStr(typeChar);
                escapeMap_[GetRuntimeInfoTypeByStr(typeStr)]++;
            }
        }
        SetLoadedMap(true);
        delete[] typeChar;
        return escapeMap_;
    }

    const char *GetRuntimeInfoTypeStr(const RuntimeInfoType type) const
    {
        switch (type) {
            case RuntimeInfoType::AOT_CRASH:
                return "AOT_CRASH";
            case RuntimeInfoType::JIT:
                return "JIT";
            case RuntimeInfoType::OTHERS:
                return "OTHERS";
            case RuntimeInfoType::NONE:
                return "NONE";
            case RuntimeInfoType::JS:
                return "JS";
            case RuntimeInfoType::AOT_BUILD:
                return "AOT_BUILD";
            default:
                return "NONE";
        }
    }
    
    RuntimeInfoType GetRuntimeInfoTypeByStr(std::string &type) const
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

    virtual bool GetRuntimeBuildId(char *buildId, int length) const
    {
        if (!FileExist(OhosConstants::RUNTIME_SO_PATH)) {
            return false;
        }
        static char realPath[PATH_MAX] = { '\0' };
        if (!ecmascript::RealPathByChar(OhosConstants::RUNTIME_SO_PATH, realPath, PATH_MAX, false)) {
            return false;
        }
        if (!FileExist(realPath)) {
            return false;
        }
        ecmascript::MemMap fileMap = ecmascript::FileMap(realPath, FILE_RDONLY, PAGE_PROT_READ);
        if (fileMap.GetOriginAddr() == nullptr) {
            return false;
        }
        ParseELFSectionsForBuildId(fileMap, buildId, length);
        ecmascript::FileUnMap(fileMap);
        fileMap.Reset();
        return true;
    }

    virtual bool GetMicrosecondsTimeStamp(char *timestamp, size_t length) const
    {
        time_t current_time;
        if (time(&current_time) == -1) {
            return false;
        }
        struct tm *local_time = localtime(&current_time);
        if (local_time == NULL) {
            return false;
        }
        size_t result = strftime(timestamp, length, "%Y-%m-%d %H:%M:%S", local_time);
        if (result == 0) {
            return false;
        }
        return true;
    }

    virtual bool GetCrashSandBoxRealPath(char *realOutPath, size_t length) const
    {
        if (!ecmascript::RealPathByChar(OhosConstants::SANDBOX_ARK_PROFILE_PATH, realOutPath, length, false)) {
            return false;
        }
        if (strcat_s(realOutPath, NAME_MAX, OhosConstants::PATH_SEPARATOR) != 0) {
            return false;
        }
        if (strcat_s(realOutPath, NAME_MAX, OhosConstants::AOT_RUNTIME_INFO_NAME) !=0) {
            return false;
        }
        return true;
    }
protected:

    bool IsCharEmpty(const char *value) const
    {
        if (value == NULL || *value == '\0') {
            return true;
        }
        return false;
    }

    bool BuildRuntimeInfoPart(char *runtimeInfoPart, const char *soBuildId, const char *timestamp,
        RuntimeInfoType type) const
    {
        if (strcat_s(runtimeInfoPart, NAME_MAX, soBuildId) != 0) {
            return false;
        }
        if (strcat_s(runtimeInfoPart, NAME_MAX, OhosConstants::SPLIT_STR) != 0) {
            return false;
        }
        if (strcat_s(runtimeInfoPart, NAME_MAX, timestamp) != 0) {
            return false;
        }
        if (strcat_s(runtimeInfoPart, NAME_MAX, OhosConstants::SPLIT_STR) != 0) {
            return false;
        }
        if (strcat_s(runtimeInfoPart, NAME_MAX, GetRuntimeInfoTypeStr(type)) != 0) {
            return false;
        }
        return true;
    }

    const char *GetInfoFromBuffer(char *line, int index) const
    {
        char *saveptr;
        char buffer[BUFFER_SIZE] = { '\0' };
        if (strncpy_s(buffer, BUFFER_SIZE - 1, line, sizeof(buffer) - 1) != 0) {
            return "";
        }
        char *token = strtok_r(buffer, OhosConstants::SPLIT_STR, &saveptr);
        
        for (int i = 0; i < index; i++) {
            token = strtok_r(NULL, OhosConstants::SPLIT_STR, &saveptr);
        }
        return token;
    }

    int getLength(char lines[][BUFFER_SIZE], int maxInput) const
    {
        int count = 0;
        for (int i = 0; i < maxInput; i++) {
            if (lines[i][0] != '\0') {
                count++;
            }
        }
        return count;
    }

    void GetCrashRuntimeInfoList(char lines[][BUFFER_SIZE]) const
    {
        static char realOutPath[PATH_MAX] = { '\0' };
        if (!GetCrashSandBoxRealPath(realOutPath, PATH_MAX)) {
            return;
        }
        if (!FileExist(realOutPath)) {
            return;
        }
        static char soBuildId[NAME_MAX] = { '\0' };
        if (!GetRuntimeBuildId(soBuildId, NAME_MAX)) {
            return;
        }
        if (IsCharEmpty(soBuildId)) {
            return;
        }
        GetRuntimeInfoByPath(lines, realOutPath, soBuildId);
        return;
    }

    void GetRealPathRuntimeInfoList(char lines[][BUFFER_SIZE], const std::string &pgoRealPath) const
    {
        std::string realOutPath;
        if (!ecmascript::RealPath(pgoRealPath, realOutPath, false)) {
            return;
        }
        if (!FileExist(realOutPath.c_str())) {
            return;
        }
        char soBuildId[NAME_MAX] = { '\0' };
        if (!GetRuntimeBuildId(soBuildId, NAME_MAX)) {
            return;
        }
        if (IsCharEmpty(soBuildId)) {
            return;
        }
        GetRuntimeInfoByPath(lines, realOutPath.c_str(), soBuildId);
    }

    virtual void SetRuntimeInfo(const char *realOutPath, char lines[][BUFFER_SIZE], int length) const
    {
        int fd = open(realOutPath,  O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd == -1) {
            return;
        }
        FdsanExchangeOwnerTag(reinterpret_cast<fd_t>(fd));
        for (int i = 0; i < length && lines[i] != NULL; i++) {
            if (lines[i][0] != '\0') {
                write(fd, lines[i], strlen(lines[i]));
                write(fd, "\n", 1);
            }
        }
        Close(reinterpret_cast<fd_t>(fd));
    }

    void GetRuntimeInfoByPath(char lines[][BUFFER_SIZE], const char *realOutPath, const char *soBuildId) const
    {
        int fd = open(realOutPath, O_RDONLY);
        if (fd == -1) {
            return;
        }
        FdsanExchangeOwnerTag(reinterpret_cast<fd_t>(fd));
        char buffer[BUFFER_SIZE] = { '\0' };
        char *saveptr;
        char *token;
        ssize_t bytesRead;
        int lineCount = 0;
        while ((bytesRead = read(fd, buffer, BUFFER_SIZE - 1)) > 0) {
            token = strtok_r(buffer, "\n", &saveptr);
            while (token != NULL) {
                if (strcmp(GetInfoFromBuffer(token, RUNTIME_INDEX_BUILDID), soBuildId) == 0 &&
                    lineCount < MAX_LENGTH &&
                    strcpy_s(lines[lineCount], BUFFER_SIZE, token) == 0) {
                    lineCount++;
                }
                token = strtok_r(NULL, "\n", &saveptr);
            }
        }
        Close(reinterpret_cast<fd_t>(fd));
    }

    void ParseELFSectionsForBuildId(ecmascript::MemMap &fileMap, char *buildId, int length) const
    {
        llvm::ELF::Elf64_Ehdr *ehdr = reinterpret_cast<llvm::ELF::Elf64_Ehdr *>(fileMap.GetOriginAddr());
        char *addr = reinterpret_cast<char *>(ehdr);
        llvm::ELF::Elf64_Shdr *shdr = reinterpret_cast<llvm::ELF::Elf64_Shdr *>(addr + ehdr->e_shoff);
        ASSERT(ehdr->e_shstrndx != static_cast<llvm::ELF::Elf64_Half>(-1));
        llvm::ELF::Elf64_Shdr strdr = shdr[ehdr->e_shstrndx];
        int secId = -1;
        static const char sectionName[] = ".note.gnu.build-id";
        for (size_t i = 0; i < ehdr->e_shnum; i++) {
            llvm::ELF::Elf64_Word shName = shdr[i].sh_name;
            char *curShName = reinterpret_cast<char *>(addr) + shName + strdr.sh_offset;
            if (strcmp(sectionName, curShName) == 0) {
                secId = static_cast<int>(i);
                break;
            }
        }
        if (secId == -1) {
            return;
        }
        llvm::ELF::Elf64_Shdr secShdr = shdr[secId];
        uint64_t buildIdOffset = secShdr.sh_offset;
        uint64_t buildIdSize = secShdr.sh_size;
        llvm::ELF::Elf64_Nhdr *nhdr = reinterpret_cast<llvm::ELF::Elf64_Nhdr *>(addr + buildIdOffset);
        char *curShNameForNhdr = reinterpret_cast<char *>(addr + buildIdOffset + sizeof(*nhdr));
        if (buildIdSize - sizeof(*nhdr) < nhdr->n_namesz) {
            return;
        }

        static const char gnu[] = "GNU";
        if (strcmp(curShNameForNhdr, gnu) != 0 || nhdr->n_type != NT_GNU_BUILD_ID) {
            return;
        }
        if ((buildIdSize - sizeof(*nhdr) - AlignValues(nhdr->n_namesz, 4) < nhdr->n_descsz) || nhdr->n_descsz == 0) {
            return;
        }
        
        char *curShNameValueForNhdr = reinterpret_cast<char *>(addr + buildIdOffset + sizeof(*nhdr) +
            AlignValues(nhdr->n_namesz, 4));
        GetReadableBuildId(curShNameValueForNhdr, buildId, length);
    }

    void GetReadableBuildId(char *buildIdHex, char *buildId, int length) const
    {
        if (IsCharEmpty(buildIdHex)) {
            return;
        }
        static const char HEXTABLE[] = "0123456789abcdef";
        static const int HEXLENGTH = 16;
        static const int HEX_EXPAND_PARAM = 2;
        const int len = strlen(buildIdHex);

        for (int i = 0; i < len; i++) {
            int lowHexExpand = i * HEX_EXPAND_PARAM + 1;
            if (lowHexExpand >= length) {
                break;
            }
            unsigned int n = buildIdHex[i];
            buildId[lowHexExpand - 1] = HEXTABLE[(n >> 4) % HEXLENGTH]; // 4 : higher 4 bit of uint8
            buildId[lowHexExpand] = HEXTABLE[n % HEXLENGTH];
        }
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

    bool IsLoadedMap()
    {
        return isLoadedMap_;
    }

    void SetLoadedMap(bool value)
    {
        isLoadedMap_ = value;
    }

    std::map<RuntimeInfoType, int> escapeMap_;
    bool isLoadedMap_ = false;
    std::mutex fileMutex_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_COMPILER_OHOS_RUNTIME_BUILD_INFO_H