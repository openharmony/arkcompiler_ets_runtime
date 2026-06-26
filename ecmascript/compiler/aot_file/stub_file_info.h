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
#ifndef ECMASCRIPT_COMPILER_AOT_FILE_STUB_FILE_INFO_H
#define ECMASCRIPT_COMPILER_AOT_FILE_STUB_FILE_INFO_H

#include "ecmascript/compiler/aot_file/aot_file_info.h"
#include "ecmascript/compiler/aot_file/gdb_jit.h"

namespace panda::ecmascript {

#if defined(STUB_FUNCTION_REORDERING)
struct StubIndexMapping {
    uint32_t runtimePosition;   // Actual position in stub.an
    uint32_t compileTimeIndex;  // Original enum index
};
#endif
class PUBLIC_API StubFileInfo : public AOTFileInfo {
public:
    StubFileInfo() = default;
    ~StubFileInfo() override = default;
    void Save(const std::string &filename, Triple triple);

    void AddModuleDes(ModuleSectionDes &moduleDes)
    {
        des_.emplace_back(moduleDes);
        for (auto &s : moduleDes.GetSectionsInfo()) {
            auto sec = ElfSection(s.first);
            if (sec.isSequentialAOTSec()) {
                accumulateTotalSize(s.second.second);
            }
        }
        accumulateTotalSize(moduleDes.GetArkStackMapSize());
    }

    uint64_t GetAsmStubAddr() const
    {
        return reinterpret_cast<uint64_t>(asmStubAddr_);
    }

    uint32_t GetAsmStubSize() const
    {
        return static_cast<uint32_t>(asmStubSize_);
    }

    void SetAsmStubAddr(void *addr)
    {
        asmStubAddr_ = addr;
    }

    void SetAsmStubAddr(uintptr_t addr)
    {
        asmStubAddr_ = reinterpret_cast<void *>(addr);
    }

    void SetAsmStubSize(size_t size)
    {
        asmStubSize_ = size;
    }

    void FillAsmStubTempHolder(uint8_t *buffer, size_t bufferSize)
    {
        asmStubTempHolder_.resize(bufferSize);
        if (memcpy_s(asmStubTempHolder_.data(), bufferSize, buffer, bufferSize) != EOK) {
            LOG_FULL(FATAL) << "memcpy_s failed";
            return;
        }
        SetAsmStubAddr(asmStubTempHolder_.data());
        SetAsmStubSize(bufferSize);
    }

    void AddAsmStubELFInfo(const std::vector<const CallSignature*> &asmCallSigns,
                           const std::vector<size_t> &stubsOffset)
    {
        ASSERT(stubsOffset.size() > 0);
        if (asmCallSigns.size() != stubsOffset.size() - 1) {
            return;
        }
        size_t stubsNum = asmCallSigns.size();
        for (size_t i = 0; i < stubsNum; i++) {
            std::string name = asmCallSigns[i]->GetName();
            uint32_t offset = static_cast<uint32_t>(stubsOffset[i]);
            asmStubELFInfo_.emplace_back(std::make_pair(name, offset));
        }
        asmStubELFInfo_.emplace_back(std::make_pair("asm_stub", static_cast<uint32_t>(stubsOffset[stubsNum])));
    }

    void RegisterToDebugger()
    {
        if (fileMapMem_.GetOriginAddr() != nullptr) {
            jit_debug::RegisterStubAnToDebugger((const char *)fileMapMem_.GetOriginAddr());
        }
    }

    void UnregisterFromDebugger()
    {
        if (fileMapMem_.GetOriginAddr() != nullptr) {
            jit_debug::UnregisterStubAnFromDebugger((const char *)fileMapMem_.GetOriginAddr());
        }
    }

    void Dump() const DUMP_API_ATTR;

    void Destroy() override {};
#if defined(STUB_FUNCTION_REORDERING)
    void AddIndexMapping(uint32_t compileTimeIndex, uint32_t runtimePosition)
    {
        bcStubIndexMap_[compileTimeIndex] = runtimePosition;
#ifndef NDEBUG
        LOG_ECMA(DEBUG) << "Mapping Enum: " << compileTimeIndex << " pos: " << runtimePosition;
#endif
    };
    const std::unordered_map<uint32_t, uint32_t> &GetBCStubIndexMap() const
    {
        return bcStubIndexMap_;
    }
    uint32_t GetRuntimeStubIndex(uint32_t compileTimeIndex) const;
#endif

#if ENABLE_MEMORY_OPTIMIZATION
    uint64_t GetCodeAddr(uint32_t index) const
    {
        if (rawEntries_ != nullptr) {
            const FuncEntryDes &e = rawEntries_[index];
            if (e.IsGeneralRTStub()) {
                return e.codeAddr_ + GetAsmStubAddr();
            }
            return e.codeAddr_ + des_[e.moduleIndex_].GetSecAddr(ElfSecName::TEXT);
        }
        return 0;
    }
#endif
private:
#if defined(STUB_FUNCTION_REORDERING)
    std::unordered_map<uint32_t, uint32_t> bcStubIndexMap_;
#endif
    static constexpr uint32_t ASMSTUB_MODULE_NUM = 6;

    bool MmapLoad(const std::string &fileName);
#ifndef PANDA_TARGET_OHOS
    bool Load();
#endif
    const std::vector<ElfSecName> &GetDumpSectionNames();
    void *asmStubAddr_ {nullptr};
    size_t asmStubSize_ {0};
    std::vector<int> asmStubTempHolder_ {};
    std::vector<std::pair<std::string, uint32_t>> asmStubELFInfo_ {};

    friend class AnFileDataManager;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_COMPILER_AOT_FILE_STUB_FILE_INFO_H
