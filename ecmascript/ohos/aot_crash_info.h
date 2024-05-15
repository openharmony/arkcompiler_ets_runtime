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

#ifndef ECMASCRIPT_COMPILER_OHOS_AOT_CRASH_INFO_H
#define ECMASCRIPT_COMPILER_OHOS_AOT_CRASH_INFO_H

#include "ecmascript/base/string_helper.h"
#if defined(PANDA_TARGET_OHOS) && !defined(STANDALONE_MODE)
#include "parameters.h"
#endif

namespace panda::ecmascript::ohos {
#define CRASH_TYPE(V)                                                \
    V(AOT)                                                           \
    V(OTHERS)                                                        \
    V(NONE)                                                          \
    V(JIT)                                                           \
    V(JS)                                                            \

enum class CrashType {
    AOT,
    JIT,
    OTHERS,
    NONE,
    JS,
};
class AotCrashInfo {
    constexpr static const char *const SANDBOX_ARK_PROFILE_PATH = "/data/storage/ark-profile";
    constexpr static const char *const CRASH_FILE_NAME = "aot_crash.log";
    constexpr static const char *const RUNTIME_SO_PATH = "/system/lib64/platformsdk/libark_jsruntime.so";
    constexpr static const char *const SPLIT_STR = "|";
    constexpr static const char *const AOT_ESCAPE_DISABLE = "ark.aot.escape.disable";
    constexpr static int AOT_CRASH_COUNT = 1;
    constexpr static int OTHERS_CRASH_COUNT = 3;
    constexpr static int CRASH_LOG_SIZE = 3;
    constexpr static int NT_GNU_BUILD_ID = 3;
    constexpr static int JIT_CRASH_COUNT = 1;
    constexpr static int JS_CRASH_COUNT = 3;
public:
    
    explicit AotCrashInfo() = default;
    virtual ~AotCrashInfo() = default;

    static CrashType GetCrashType(std::string crashInfo)
    {
        std::vector<std::string> splitCrashInfo = base::StringHelper::SplitString(crashInfo, SPLIT_STR);
        if (splitCrashInfo.size() == CRASH_LOG_SIZE) {
            if (splitCrashInfo[CRASH_LOG_SIZE - 1] == GetCrashTypeStr(CrashType::AOT)) {
                return CrashType::AOT;
            } else if (splitCrashInfo[CRASH_LOG_SIZE - 1] == GetCrashTypeStr(CrashType::JIT)) {
                return CrashType::JIT;
            } else if (splitCrashInfo[CRASH_LOG_SIZE - 1] == GetCrashTypeStr(CrashType::JS)) {
                return CrashType::JS;
            } else if (splitCrashInfo[CRASH_LOG_SIZE - 1] == GetCrashTypeStr(CrashType::OTHERS)) {
                return CrashType::OTHERS;
            }
        }
        return CrashType::NONE;
    }

    static bool GetAotEscapeDisable()
    {
#if defined(PANDA_TARGET_OHOS) && !defined(STANDALONE_MODE)
        return OHOS::system::GetBoolParameter(AOT_ESCAPE_DISABLE, false);
#endif
        return false;
    }

    static std::string GetSandBoxPath()
    {
        return SANDBOX_ARK_PROFILE_PATH;
    }

    static std::string GetCrashFileName()
    {
        return CRASH_FILE_NAME;
    }

    static int GetAotCrashCount()
    {
        return AOT_CRASH_COUNT;
    }

    static int GetJitCrashCount()
    {
        return JIT_CRASH_COUNT;
    }

    static int GetJsCrashCount()
    {
        return JS_CRASH_COUNT;
    }

    static int GetOthersCrashCount()
    {
        return OTHERS_CRASH_COUNT;
    }

    static std::string GetCrashTypeStr(CrashType type)
    {
        const std::map<CrashType, const char *> strMap = {
#define CRASH_TYPE_MAP(name) { CrashType::name, #name },
        CRASH_TYPE(CRASH_TYPE_MAP)
#undef CRASH_TYPE_MAP
        };
        if (strMap.count(type) > 0) {
            return strMap.at(type);
        }
        return "";
    }

    static CrashType GetCrashTypeByStr(std::string crashType)
    {
        const std::map<std::string, CrashType> strMap = {
#define CRASH_TYPE_MAP(name) { #name, CrashType::name },
        CRASH_TYPE(CRASH_TYPE_MAP)
#undef CRASH_TYPE_MAP
        };
        if (strMap.count(crashType) > 0) {
            return strMap.at(crashType);
        }
        return CrashType::NONE;
    }

    static std::string BuildAotCrashInfo(std::string buildId, std::string timestamp, CrashType type)
    {
        return buildId + SPLIT_STR + timestamp + SPLIT_STR + GetCrashTypeStr(type);
    }

    static std::string GetBuildId(std::string crashInfo)
    {
        std::vector<std::string> splitCrashInfo = base::StringHelper::SplitString(crashInfo, SPLIT_STR);
        if (splitCrashInfo.size() == CRASH_LOG_SIZE) {
            return splitCrashInfo[0];
        }
        return "";
    }

    std::string GetRuntimeBuildId()
    {
        std::string realPath;
        if (!FileExist(RUNTIME_SO_PATH)) {
            return "";
        }
        std::string soPath = panda::os::file::File::GetExtendedFilePath(RUNTIME_SO_PATH);
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

private:
    std::string GetReadableBuildId(std::string &buildIdHex)
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

    uint64_t AlignValues(uint64_t val, uint64_t align)
    {
        return (val + AlignBytes(align)) & AlignMask(align);
    }

    uint64_t AlignMask(uint64_t align)
    {
        return ~(static_cast<uint64_t>((align) - 1));
    }

    uint64_t AlignBytes(uint64_t align)
    {
        return (align) - 1;
    }

    std::string ParseELFSectionsForBuildId()
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
        if (buildIdSize - sizeof(*nhdr) - AlignValues(nhdr->n_namesz, 4) < nhdr->n_descsz || nhdr->n_descsz == 0) {
            return "";
        }
        
        char *curShNameValueForNhdr = reinterpret_cast<char *>(addr + buildIdOffset + sizeof(*nhdr)
                                    + AlignValues(nhdr->n_namesz, 4));
        std::string curShNameValueForNhdrStr = curShNameValueForNhdr;
        return GetReadableBuildId(curShNameValueForNhdrStr);
    }

    ecmascript::MemMap fileMap;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_COMPILER_OHOS_AOT_CRASH_INFO_H
