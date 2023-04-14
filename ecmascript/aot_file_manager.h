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
#ifndef ECMASCRIPT_AOT_FILE_MANAGER_H
#define ECMASCRIPT_AOT_FILE_MANAGER_H

#include "ecmascript/compiler/binary_section.h"
#include "ecmascript/deoptimizer/calleeReg.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/stackmap/ark_stackmap.h"

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

class ExecutedMemoryAllocator {
public:
    struct ExeMem {
        void *addr_ {nullptr};
        size_t size_ {0};
    };

    static void AllocateBuf(uint32_t size, ExeMem &exeMem) {
        MemMap buf = MachineCodePageMap(AlignUp(size, PageSize()), PAGE_PROT_EXEC_READWRITE);
        exeMem.addr_ = buf.GetMem();
        exeMem.size_ = buf.GetSize();
    }

    static void DestoryBuf(ExeMem &exeMem) {
        if (exeMem.addr_ != nullptr) {
            MachineCodePageUnmap(MemMap(exeMem.addr_, exeMem.size_));
            exeMem.addr_ = nullptr;
            exeMem.size_ = 0;
        }
    }
};

struct ModuleSectionDes {
    std::map<ElfSecName, std::pair<uint64_t, uint32_t>> sectionsInfo_ {};
    uint32_t startIndex_ {static_cast<uint32_t>(-1)}; // record current module first function index in AOTFileInfo
    uint32_t funcCount_ {0};
    std::shared_ptr<uint8_t> arkStackMapPtr_ {nullptr};
    uint32_t arkStackMapSize_ {0};
    uint8_t *arkStackMapRawPtr_ {nullptr};

    std::string GetSecName(const ElfSecName idx) const
    {
        switch (idx) {
            case ElfSecName::RODATA:
                return "rodata";
            case ElfSecName::RODATA_CST4:
                return "rodata.cst4";
            case ElfSecName::RODATA_CST8:
                return "rodata.cst8";
            case ElfSecName::RODATA_CST16:
                return "rodata.cst16";
            case ElfSecName::RODATA_CST32:
                return "rodata.cst32";
            case ElfSecName::TEXT:
                return "text";
            case ElfSecName::DATA:
                return "data";
            case ElfSecName::GOT:
                return "got";
            case ElfSecName::RELATEXT:
                return "rela.text";
            case ElfSecName::STRTAB:
                return "strtab";
            case ElfSecName::SYMTAB:
                return "symtab";
            case ElfSecName::LLVM_STACKMAP:
                return "llvm_stackmaps";
            default: {
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
            }
        }
    }

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

    bool ContainCode(uintptr_t pc) const
    {
        uint64_t stubStartAddr = GetSecAddr(ElfSecName::TEXT);
        uint64_t stubEndAddr = stubStartAddr + GetSecSize(ElfSecName::TEXT);
        return (pc >= stubStartAddr && pc <= stubEndAddr);
    }

    void SaveSectionsInfo(std::ofstream &file);
    void LoadSectionsInfo(BinaryBufferParser &parser, uint32_t &curUnitOffset,
        uint64_t codeAddress);
    void LoadStackMapSection(BinaryBufferParser &parser, uintptr_t secBegin, uint32_t &curUnitOffset);
    void LoadSectionsInfo(std::ifstream &file, uint32_t &curUnitOffset,
        uint64_t codeAddress);
    void LoadStackMapSection(std::ifstream &file, uintptr_t secBegin, uint32_t &curUnitOffset);

private:
    static constexpr int DECIMAL_LENS = 2;
    static constexpr int HUNDRED_TIME = 100;
    static constexpr int PERCENT_LENS = 4;
};

class PUBLIC_API AOTFileInfo {
public:
    using CallSignature = kungfu::CallSignature;
    AOTFileInfo() = default;
    virtual ~AOTFileInfo() = default;

    struct FuncEntryDes {
        uint64_t codeAddr_;
        CallSignature::TargetKind kind_;
        bool isMainFunc_;
        uint32_t indexInKindOrMethodId_;
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
            return (kind_ == CallSignature::TargetKind::BUILTINS_STUB ||
                    kind_ == CallSignature::TargetKind::BUILTINS_WITH_ARGV_STUB);
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

    uint32_t GetStubNum() const
    {
        return entryNum_;
    }

    void SetStubNum(uint32_t n)
    {
        entryNum_ = n;
    }

    void AddEntry(CallSignature::TargetKind kind, bool isMainFunc, int indexInKind, uint64_t offset,
                      uint32_t moduleIndex, int delta, uint32_t size, kungfu::CalleeRegAndOffsetVec info = {})
    {
        FuncEntryDes des;
        if (memset_s(&des, sizeof(des), 0, sizeof(des)) != EOK) {
            LOG_FULL(FATAL) << "memset_s failed";
            return;
        }
        des.kind_ = kind;
        des.isMainFunc_ = isMainFunc;
        des.indexInKindOrMethodId_ = static_cast<uint32_t>(indexInKind);
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

    virtual void Destroy();

protected:
    ExecutedMemoryAllocator::ExeMem& GetExeMem() {
        return exeMem_;
    }

    uint32_t entryNum_ {0};
    uint32_t moduleNum_ {0};
    uint32_t totalCodeSize_ {0};
    std::vector<FuncEntryDes> entries_ {};
    std::vector<ModuleSectionDes> des_ {};
    ExecutedMemoryAllocator::ExeMem exeMem_ {};
};

class PUBLIC_API AnFileInfo : public AOTFileInfo {
public:
    AnFileInfo() = default;
    ~AnFileInfo() override = default;
    void Save(const std::string &filename, kungfu::Triple triple);
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

    uintptr_t GetMainFuncEntry(uint32_t methodId) const
    {
        auto it = mainEntryMap_.find(methodId);
        if (it == mainEntryMap_.end()) {
            return 0;
        }
        return static_cast<uintptr_t>(it->second);
    }

    bool IsLoadMain(const JSPandaFile *jsPandaFile, const CString &entry) const;

    bool IsLoad() const
    {
        return isLoad_;
    }

    void Destroy() override;

    bool RewriteRelcateDeoptHandler(EcmaVM *vm);

    Elf64_Ehdr GetHeader()
    {
        return header_;
    }

private:
    bool Load(const std::string &filename);
    bool RewriteRelcateTextSection(const char* symbol, uintptr_t patchAddr);
    std::unordered_map<uint32_t, uint64_t> mainEntryMap_ {};
    bool isLoad_ {false};
    Elf64_Ehdr header_;

    friend class AnFileDataManager;
};

class PUBLIC_API StubFileInfo : public AOTFileInfo {
public:
    StubFileInfo() = default;
    ~StubFileInfo() override = default;
    void Save(const std::string &filename);

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
    bool Load();
    void *asmStubAddr_ {nullptr};
    size_t asmStubSize_ {0};
    std::vector<int> asmStubTempHolder_ {};

    friend class AnFileDataManager;
};

class AOTLiteralInfo : public TaggedArray {
public:
    static AOTLiteralInfo *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsTaggedArray());
        return static_cast<AOTLiteralInfo *>(object);
    }
};

class AnFileDataManager {
public:
    enum class Type : uint8_t {
        STUB = 0,
        AOT,
    };

    static AnFileDataManager *GetInstance();
    ~AnFileDataManager();

    bool SafeLoad(const std::string &fileName, Type type, EcmaVM *vm = nullptr);
    uint32_t SafeGetFileInfoIndex(const std::string &fileName);
    std::shared_ptr<AnFileInfo> SafeGetAnFileInfo(uint32_t index);
    std::shared_ptr<StubFileInfo> SafeGetStubFileInfo();
    bool SafeTryReadLock();
    bool SafeInsideStub(uintptr_t pc);
    bool SafeInsideAOT(uintptr_t pc);
    AOTFileInfo::CallSiteInfo SafeCalCallSiteInfo(uintptr_t retAddr);
    void SafeDestoryAllData();
    void SafeDestroyAnData(const std::string &fileName);

    const std::string& GetDir() const
    {
        return anDir_;
    }

    bool IsEnable() const
    {
        return anEnable_;
    }

    // only main thread call this, only call once, no need to lock
    void SetDir(std::string dir)
    {
        anDir_ = std::move(dir);
    }

    void SetEnable(bool enable)
    {
        anEnable_ = enable;
    }

private:
    AnFileDataManager() = default;
    std::shared_ptr<AnFileInfo> UnsafeFind(const std::string &fileName) const;
    bool UnsafeLoadFromAOT(const std::string &fileName, EcmaVM *vm);
    bool UnsafeLoadFromStub();
    uint32_t UnSafeGetFileInfoIndex(const std::string &fileName);
    std::shared_ptr<AnFileInfo> UnSafeGetAnFileInfo(uint32_t index)
    {
        return loadedAn_.at(index);
    }

    os::memory::RWLock lock_;
    std::unordered_map<std::string, uint32_t> anFileNameToIndexMap_;
    std::vector<std::shared_ptr<AnFileInfo>> loadedAn_ {};
    std::shared_ptr<StubFileInfo> loadedStub_ {nullptr};
    std::string anDir_;
    bool anEnable_ {false};
};

class AOTFileManager {
public:
    explicit AOTFileManager(EcmaVM *vm);
    virtual ~AOTFileManager();

    static constexpr char FILE_EXTENSION_AN[] = ".an";
    static constexpr char FILE_EXTENSION_AI[] = ".ai";
    static constexpr uint8_t DESERI_CP_ITEM_SIZE = 2;

    void LoadStubFile(const std::string &fileName);
    bool LoadAnFile(const std::string &fileName);
    AOTFileInfo::CallSiteInfo CalCallSiteInfo(uintptr_t retAddr) const;
    bool TryReadLock() const;
    bool InsideStub(uintptr_t pc) const;
    bool InsideAOT(uintptr_t pc) const;
    void Iterate(const RootVisitor &v);

    const std::shared_ptr<AnFileInfo> GetAnFileInfo(const JSPandaFile *jsPandaFile) const;
    bool IsLoad(const JSPandaFile *jsPandaFile) const;
    bool IsLoadMain(const JSPandaFile *jsPandaFile, const CString &entry) const;
    uint32_t GetAnFileIndex(const JSPandaFile *jsPandaFile) const;
    void SetAOTMainFuncEntry(JSHandle<JSFunction> mainFunc, const JSPandaFile *jsPandaFile,
                             std::string_view entryPoint);
    void SetAOTFuncEntry(const JSPandaFile *jsPandaFile, Method *method, uint32_t entryIndex);
    void SetAOTFuncEntryForLiteral(const JSPandaFile *jsPandaFile, const TaggedArray *literal,
                                   const AOTLiteralInfo *entryIndexes);
    void LoadAiFile([[maybe_unused]] const std::string &filename);
    void LoadAiFile(const JSPandaFile *jsPandaFile);
    kungfu::ArkStackMapParser* GetStackMapParser() const;
    static JSTaggedValue GetAbsolutePath(JSThread *thread, JSTaggedValue relativePathVal);
    static bool GetAbsolutePath(const CString &relativePathCstr, CString &absPathCstr);
    bool RewriteDataSection(uintptr_t dataSec, size_t size, uintptr_t newData, size_t newSize);
    void AddConstantPool(const CString &snapshotFileName, JSTaggedValue deserializedCPList);
    JSHandle<JSTaggedValue> GetDeserializedConstantPool(const JSPandaFile *jsPandaFile, int32_t cpID);
    std::string GetAotFileName(EcmaVM *vm, const JSPandaFile *jsPandaFile, const std::string &extensionName) const;

private:

    void RewriteRelcateDeoptHandler(EcmaVM *vm, AnFileInfo AOTFileInfo)
    {
        AOTFileInfo.RewriteRelcateDeoptHandler(vm);
    }

    void PrintAOTEntry(const JSPandaFile *file, const Method *method, uintptr_t entry);
    void InitializeStubEntries(const std::vector<AnFileInfo::FuncEntryDes>& stubs);
    void AdjustBCStubAndDebuggerStubEntries(JSThread *thread, const std::vector<AOTFileInfo::FuncEntryDes> &stubs,
                                            const AsmInterParsedOption &asmInterOpt);

    EcmaVM *vm_ {nullptr};
    ObjectFactory *factory_ {nullptr};
    std::unordered_map<uint32_t, CMap<int32_t, JSTaggedValue>> desCPs_ {};
    kungfu::ArkStackMapParser *arkStackMapParser_ {nullptr};

    friend class AnFileInfo;
    friend class StubFileInfo;
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_AOT_FILE_MANAGER_H
