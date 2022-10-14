/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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
#ifndef ECMASCRIPT_COMPILER_FILE_LOADER_H
#define ECMASCRIPT_COMPILER_FILE_LOADER_H

#include "ecmascript/ark_stackmap.h"
#include "ecmascript/calleeReg.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/compiler/binary_section.h"

namespace panda::ecmascript {
class JSpandafile;
class JSThread;

class BinaryBufferParser {
public:
    BinaryBufferParser(uint8_t *buffer, uint32_t length) : buffer_(buffer), length_(length) {}
    ~BinaryBufferParser() = default;
    void ParseBuffer(void *dst, uint32_t count);
    void ParseBuffer(uint8_t *dst, uint32_t count, uint8_t *src);

private:
    uint8_t *buffer_ {nullptr};
    uint32_t length_ {0};
    uint32_t offset_ {0};
};

struct ModuleSectionDes {
    std::map<ElfSecName, std::pair<uint64_t, uint32_t>> sectionsInfo_ {};
    uint32_t startIndex_ {static_cast<uint32_t>(-1)}; // record current module first function index in PackInfo
    uint32_t funcCount_ {0};
    std::shared_ptr<uint8_t> arkStackMapPtr_ {nullptr};
    uint32_t arkStackMapSize_ {0};
    uint8_t *arkStackMapRawPtr_ {nullptr};

    void SetArkStackMapPtr(std::shared_ptr<uint8_t> ptr)
    {
        arkStackMapPtr_ = ptr;
    }

    std::shared_ptr<uint8_t> GetArkStackMapSharePtr()
    {
        return std::move(arkStackMapPtr_);
    }

    void SetArkStackMapPtr(uint8_t *ptr)
    {
        arkStackMapRawPtr_ = ptr;
    }

    uint8_t* GetArkStackMapRawPtr()
    {
        return arkStackMapRawPtr_;
    }

    void SetArkStackMapSize(uint32_t size)
    {
        arkStackMapSize_ = size;
    }

    uint32_t GetArkStackMapSize() const
    {
        return arkStackMapSize_;
    }

    void SetStartIndex(uint32_t index)
    {
        startIndex_ = index;
    }

    uint32_t GetStartIndex() const
    {
        return startIndex_;
    }

    void SetFuncCount(uint32_t cnt)
    {
        funcCount_ = cnt;
    }

    uint32_t GetFuncCount() const
    {
        return funcCount_;
    }

    ModuleSectionDes() = default;

    void SetSecAddr(uint64_t addr, ElfSecName idx)
    {
        sectionsInfo_[idx].first = addr;
    }

    uint64_t GetSecAddr(const ElfSecName idx) const
    {
        auto it = sectionsInfo_.find(idx);
        return it == sectionsInfo_.end() ? 0 : it->second.first;
    }

    void EraseSec(ElfSecName idx)
    {
        sectionsInfo_.erase(idx);
    }

    void SetSecSize(uint32_t size, ElfSecName idx)
    {
        sectionsInfo_[idx].second = size;
    }

    uint32_t GetSecSize(const ElfSecName idx) const
    {
        auto it = sectionsInfo_.find(idx);
        return it == sectionsInfo_.end() ? 0 : it->second.second;
    }

    uint32_t GetSecInfosSize()
    {
        return sectionsInfo_.size();
    }

    void SaveSectionsInfo(std::ofstream &file);
    void LoadSectionsInfo(BinaryBufferParser &parser, uint32_t &curUnitOffset,
        uint64_t codeAddress);
    void LoadStackMapSection(BinaryBufferParser &parser, uintptr_t secBegin, uint32_t &curUnitOffset);
    void LoadSectionsInfo(std::ifstream &file, uint32_t &curUnitOffset,
        uint64_t codeAddress);
    void LoadStackMapSection(std::ifstream &file, uintptr_t secBegin, uint32_t &curUnitOffset);
};

class PUBLIC_API ModulePackInfo {
public:
    using CallSignature = kungfu::CallSignature;
    ModulePackInfo() = default;
    virtual ~ModulePackInfo() = default;
    bool VerifyFilePath([[maybe_unused]] const std::string &filePath, [[maybe_unused]] bool toGenerate = false) const;

    struct FuncEntryDes {
        uint64_t codeAddr_;
        CallSignature::TargetKind kind_;
        uint32_t indexInKind_;
        uint32_t moduleIndex_;
        int fpDeltaPrevFrameSp_;
        uint32_t funcSize_;
        [[maybe_unused]] uint32_t calleeRegisterNum_;
        int32_t CalleeReg2Offset_[2 * kungfu::MAX_CALLEE_SAVE_REIGISTER_NUM];
        bool IsStub() const
        {
            return CallSignature::TargetKind::STUB_BEGIN <= kind_ && kind_ < CallSignature::TargetKind::STUB_END;
        }

        bool IsBCStub() const
        {
            return CallSignature::TargetKind::BCHANDLER_BEGIN <= kind_ &&
                   kind_ < CallSignature::TargetKind::BCHANDLER_END;
        }

        bool IsBCHandlerStub() const
        {
            return (kind_ == CallSignature::TargetKind::BYTECODE_HANDLER);
        }

        bool IsBuiltinsStub() const
        {
            return (kind_ == CallSignature::TargetKind::BUILTINS_STUB);
        }

        bool IsCommonStub() const
        {
            return (kind_ == CallSignature::TargetKind::COMMON_STUB);
        }

        bool IsGeneralRTStub() const
        {
            return (kind_ >= CallSignature::TargetKind::RUNTIME_STUB &&
                kind_ <= CallSignature::TargetKind::DEOPT_STUB);
        }
    };

    const FuncEntryDes& GetStubDes(int index) const
    {
        return entries_[index];
    }

    uint32_t GetEntrySize() const
    {
        return entries_.size();
    }

    const std::vector<FuncEntryDes>& GetStubs() const
    {
        return entries_;
    }

    const std::vector<ModuleSectionDes>& GetCodeUnits() const
    {
        return des_;
    }

    void Iterate(const RootVisitor &v);

    void SetCode(JSHandle<MachineCode> code)
    {
        machineCodeObj_ = code.GetTaggedValue();
    }

    JSHandle<MachineCode> GetCode()
    {
        return JSHandle<MachineCode>(reinterpret_cast<uintptr_t>(&machineCodeObj_));
    }

    bool isCodeHole()
    {
        return machineCodeObj_.IsHole();
    }

    uint32_t GetStubNum() const
    {
        return entryNum_;
    }

    void SetStubNum(uint32_t n)
    {
        entryNum_ = n;
    }

    void AddStubEntry(CallSignature::TargetKind kind, int indexInKind, uint64_t offset,
                      uint32_t moduleIndex, int delta, uint32_t size, kungfu::CalleeRegAndOffsetVec info = {})
    {
        FuncEntryDes des;
        if (memset_s(&des, sizeof(des), 0, sizeof(des)) != EOK) {
            LOG_FULL(FATAL) << "memset failed";
            return;
        }
        des.kind_ = kind;
        des.indexInKind_ = static_cast<uint32_t>(indexInKind);
        des.codeAddr_ = offset;
        des.moduleIndex_ = moduleIndex;
        des.fpDeltaPrevFrameSp_ = delta;
        des.funcSize_ = size;
        des.calleeRegisterNum_ = info.size();
        kungfu::DwarfRegType reg = 0;
        kungfu::OffsetType regOffset = 0;
        for (size_t i = 0; i < info.size(); i ++) {
            std::tie(reg, regOffset) = info[i];
            des.CalleeReg2Offset_[2 * i] = static_cast<int32_t>(reg);
            des.CalleeReg2Offset_[2 * i + 1] = static_cast<int32_t>(regOffset);
        }
        entries_.emplace_back(des);
    }

    const std::vector<ModuleSectionDes> &GetModuleSectionDes() const
    {
        return des_;
    }

    size_t GetCodeUnitsNum()
    {
        return des_.size();
    }

    void accumulateTotalSize(uint32_t size)
    {
        totalCodeSize_ += size;
    }

    using CallSiteInfo = std::tuple<uint64_t, uint8_t *, int, kungfu::CalleeRegAndOffsetVec>;
    bool CalCallSiteInfo(uintptr_t retAddr, CallSiteInfo& ret) const;

protected:
    uint32_t entryNum_ {0};
    uint32_t moduleNum_ {0};
    uint32_t totalCodeSize_ {0};
    std::vector<FuncEntryDes> entries_ {};
    std::vector<ModuleSectionDes> des_ {};
    // NOTE: code object is non movable(code space) currently.
    // The thought use of code->GetDataOffsetAddress() as data base rely on this assumption.
    // A stable technique or mechanism for code object against other GC situation is future work.
    JSTaggedValue machineCodeObj_ {JSTaggedValue::Hole()};
};

class PUBLIC_API AOTModulePackInfo : public ModulePackInfo {
public:
    AOTModulePackInfo() = default;
    ~AOTModulePackInfo() override = default;
    void Save(const std::string &filename);
    bool Load(EcmaVM *vm, const std::string &filename);
    void AddModuleDes(ModuleSectionDes &moduleDes, uint32_t hash)
    {
        des_.emplace_back(moduleDes);
        for (auto &s : moduleDes.sectionsInfo_) {
            auto sec = ElfSection(s.first);
            if (sec.isSequentialAOTSec()) {
                accumulateTotalSize(s.second.second);
            }
        }
        accumulateTotalSize(moduleDes.GetArkStackMapSize());
        aotFileHashs_.emplace_back(hash);
    }

private:
    std::vector<uint32_t> aotFileHashs_ {};
};

class PUBLIC_API StubModulePackInfo : public ModulePackInfo {
public:
    StubModulePackInfo() = default;
    ~StubModulePackInfo() override = default;
    void Save(const std::string &filename);
    bool Load(EcmaVM *vm);

    void AddModuleDes(ModuleSectionDes &moduleDes)
    {
        des_.emplace_back(moduleDes);
        for (auto &s : moduleDes.sectionsInfo_) {
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

private:
    void *asmStubAddr_ {nullptr};
    size_t asmStubSize_ {0};
    std::vector<int> asmStubTempHolder_ {};
};

class FileLoader {
public:
    explicit FileLoader(EcmaVM *vm);
    virtual ~FileLoader();
    void LoadStubFile();
    void LoadAOTFile(const std::string &fileName);
    ModulePackInfo::CallSiteInfo CalCallSiteInfo(uintptr_t retAddr) const;
    bool InsideStub(uint64_t pc) const;

    void Iterate(const RootVisitor &v)
    {
        if (!stubPackInfo_.isCodeHole()) {
            stubPackInfo_.Iterate(v);
        }
        if (aotPackInfos_.size() != 0) {
            for (auto &packInfo : aotPackInfos_) {
                packInfo.Iterate(v);
            }
        }
    }

    void SaveAOTFuncEntry(uint32_t hash, uint32_t methodId, uint64_t funcEntry)
    {
        hashToEntryMap_[hash][methodId] = funcEntry;
    }

    void UpdateJSMethods(JSHandle<JSFunction> mainFunc, const JSPandaFile *jsPandaFile);
    bool hasLoaded(const JSPandaFile *jsPandaFile);
    void SetAOTFuncEntry(const JSPandaFile *jsPandaFile, Method *method);
    void SetAOTFuncEntryForLiteral(const JSPandaFile *jsPandaFile, const JSHandle<TaggedArray> &obj);
    void LoadSnapshotFile([[maybe_unused]]const std::string& filename);
    kungfu::ArkStackMapParser* GetStackMapParser() const;
    static JSTaggedValue GetAbsolutePath(JSThread *thread, JSTaggedValue relativePathVal);
    static bool GetAbsolutePath(const std::string &relativePath, std::string &absPath);
    static bool GetAbsolutePath(const CString &relativePathCstr, CString &absPathCstr);
    bool RewriteDataSection(uintptr_t dataSec, size_t size, uintptr_t newData, size_t newSize);
    bool RewriteGotSection();

private:
    void SetAOTmmap(void *addr, size_t totalCodeSize)
    {
        aotAddrs_.emplace_back(std::make_pair(addr, totalCodeSize));
    }

    void SetStubmmap(void *addr, size_t totalCodeSize)
    {
        stubAddrs_.emplace_back(std::make_pair(addr, totalCodeSize));
    }

    const StubModulePackInfo& GetStubPackInfo() const
    {
        return stubPackInfo_;
    }

    void AddAOTPackInfo(AOTModulePackInfo packInfo)
    {
        aotPackInfos_.emplace_back(packInfo);
    }

    uintptr_t GetAOTFuncEntry(uint32_t hash, uint32_t methodId)
    {
        auto m = hashToEntryMap_[hash];
        auto it = m.find(methodId);
        if (it == m.end()) {
            return 0;
        }
        return static_cast<uintptr_t>(it->second);
    }
    void PrintAOTEntry(const JSPandaFile *file, const Method *method, uintptr_t entry);
    void InitializeStubEntries(const std::vector<AOTModulePackInfo::FuncEntryDes>& stubs);
    void AdjustBCStubAndDebuggerStubEntries(JSThread *thread, const std::vector<ModulePackInfo::FuncEntryDes> &stubs,
                                            const AsmInterParsedOption &asmInterOpt);

    std::vector<std::pair<void *, size_t>> aotAddrs_;
    std::vector<std::pair<void *, size_t>> stubAddrs_;
    EcmaVM *vm_ {nullptr};
    ObjectFactory *factory_ {nullptr};
    StubModulePackInfo stubPackInfo_ {};
    std::vector<AOTModulePackInfo> aotPackInfos_ {};
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint64_t>> hashToEntryMap_ {};
    kungfu::ArkStackMapParser *arkStackMapParser_ {nullptr};

    friend class AOTModulePackInfo;
    friend class StubModulePackInfo;
};
}
#endif // ECMASCRIPT_COMPILER_FILE_LOADER_H
