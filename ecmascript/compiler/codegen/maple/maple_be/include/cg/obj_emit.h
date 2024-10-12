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

#ifndef MAPLEBE_INCLUDE_CG_OBJ_EMIT_H
#define MAPLEBE_INCLUDE_CG_OBJ_EMIT_H

#include "emit.h"
#include "ifile.h"
#include "string_utils.h"
#ifdef JIT_ENABLE_CODE_SIGN
#include "jit_buffer_integrity.h"
#include "jit_signcode.h"
#endif

namespace maplebe {
#ifdef JIT_ENABLE_CODE_SIGN
using namespace OHOS::Security::CodeSign;
using namespace panda::ecmascript::kungfu;
#endif
enum FixupKind : uint32 {
    kFKNone,
    kExceptFixup,
    kEhTypeDefFixup,
    kEhTypeUndefFixup,
    kFirstTargetFixupKind = 64, /* the kind in subclass start from 64 */
};

class Fixup {
public:
    Fixup(const std::string &label, uint32 relOffsetVal, uint32 offsetVal, FixupKind fixupKind)
        : labelName(label), relOffset(relOffsetVal), offset(offsetVal), kind(fixupKind)
    {
    }

    ~Fixup() = default;

    const std::string &GetLabel() const
    {
        return labelName;
    }

    uint32 GetRelOffset() const
    {
        return relOffset;
    }

    void SetOffset(uint32 value)
    {
        offset = value;
    }

    uint32 GetOffset() const
    {
        return offset;
    }

    FixupKind GetFixupKind() const
    {
        return kind;
    }

private:
    std::string labelName; /* target label name */
    uint32 relOffset;      /* offset to target label */
    uint32 offset;         /* record where to fix up */
    FixupKind kind;        /* record how to fix up */
};

class LocalFixup {
public:
    LocalFixup(uint32 label, uint32 offsetVal, FixupKind fixupkind)
        : labelIndex(label), offset(offsetVal), kind(fixupkind)
    {
    }

    ~LocalFixup() = default;

    uint32 GetLabelIndex() const
    {
        return labelIndex;
    }

    uint32 GetOffset() const
    {
        return offset;
    }

    FixupKind GetFixupKind() const
    {
        return kind;
    }

private:
    uint32 labelIndex; /* target label index */
    uint32 offset;     /* record where to fix up */
    FixupKind kind;    /* record how to fix up */
};

enum SymbolKind : uint32 {
    kStFunc,
    kStNone,
};

class ObjSymbol {
public:
    ObjSymbol(const std::string &name, SymbolKind kind, uint32 pos) : symbolName(name), symbolKind(kind), offset(pos) {}
    ~ObjSymbol() = default;

    std::string GetSymbolName() const
    {
        return symbolName;
    }

    SymbolKind GetSymbolKind() const
    {
        return symbolKind;
    }

    uint32 GetOffset() const
    {
        return offset;
    }

private:
    std::string symbolName;
    SymbolKind symbolKind;
    uint32 offset;
};

class ObjFuncEmitInfo : public FuncEmitInfo {
public:
    ObjFuncEmitInfo(CGFunc &func, MemPool &inputMemPool)
        : FuncEmitInfo(func),
          memPool(inputMemPool),
          alloc(&memPool),
          localFixups(alloc.Adapter()),
          globalFixups(alloc.Adapter()),
          relocations(alloc.Adapter()),
          textData(alloc.Adapter()),
          label2Order(alloc.Adapter()),
          switchTableOffset(alloc.Adapter()),
          offset2StackMapInfo(alloc.Adapter()),
          funcName(&memPool)
    {
    }

    virtual ~ObjFuncEmitInfo() = default;

    struct StackMapInfo {
        const std::vector<uint8> referenceMap;
        const std::vector<uint8> deoptInfo;
    };

    uint32 GetEndOffset() const
    {
        return endOffset;
    }

    void SetEndOffset(uint32 offset)
    {
        endOffset = offset;
    }

    uint32 GetStartOffset() const
    {
        return startOffset;
    }

    void SetStartOffset(uint32 offset)
    {
        startOffset = offset;
    }

    uint32 GetExceptStartOffset() const
    {
        return exceptStartOffset;
    }

    void SetExceptStartOffset(uint32 offset)
    {
        exceptStartOffset = offset;
    }

    ALWAYS_INLINE void AppendLocalFixups(LocalFixup &fixup)
    {
        localFixups.push_back(&fixup);
#ifdef JIT_ENABLE_CODE_SIGN
        if (CGOptions::UseJitCodeSign()) {
            WillFixUp(JitSignCode::GetInstance()->GetCodeSigner());
        }
#endif
    }

    ALWAYS_INLINE void AppendGlobalFixups(Fixup &fixup)
    {
        globalFixups.push_back(&fixup);
#ifdef JIT_ENABLE_CODE_SIGN
        if (CGOptions::UseJitCodeSign()) {
            WillFixUp(JitSignCode::GetInstance()->GetCodeSigner());
        }
#endif
    }

    void AppendRelocations(Fixup &fixup)
    {
        relocations.push_back(&fixup);
    }

    const MapleVector<Fixup *> &GetGlobalFixups() const
    {
        return globalFixups;
    }

    virtual void AppendContents(uint64 binInsn, uint32 byteSize)
    {
        (void)binInsn;
        (void)byteSize;
        CHECK_FATAL(false, "this function should be implemented in subclass");
    }

    virtual void HandleLocalBranchFixup(const std::vector<uint32> &label2Offset,
                                        const std::vector<uint32> &symbol2Offset)
    {
        (void)label2Offset;
        CHECK_FATAL(false, "this fucntion should be implemented in subclass");
    }

    const MapleString &GetFuncName() const
    {
        return funcName;
    }

    void SetFuncName(const std::string &name)
    {
        funcName = name;
    }

    MapleVector<uint8> GetTextData() const
    {
        return textData;
    }

    size_t GetTextDataSize() const
    {
        return textData.size();
    }

    ALWAYS_INLINE void AppendTextData(const void *data, uint32 byteSize)
    {
        auto pdata = reinterpret_cast<const uint8 *>(data);  // data:0xa9be7c1d pdata:1d 7c be a9
        (void)textData.insert(textData.end(), pdata, pdata + byteSize);
#ifdef JIT_ENABLE_CODE_SIGN
        if (CGOptions::UseJitCodeSign()) {
            JitSignCode *singleton = JitSignCode::GetInstance();
            RegisterTmpBuffer(singleton->GetCodeSigner(), textData.data());
            AppendData(singleton->GetCodeSigner(), pdata, byteSize);
            singleton->signTableSize_ += 1;
        }
#endif
    }

    ALWAYS_INLINE void AppendTextData(uint64 data, uint32 byteSize)
    {
        for (size_t i = 0; i < byteSize; i++) {
            textData.push_back(static_cast<uint8>(data >> (i << k8BitShift)));
        }
#ifdef JIT_ENABLE_CODE_SIGN
        if (CGOptions::UseJitCodeSign()) {
            JitSignCode *singleton = JitSignCode::GetInstance();
            RegisterTmpBuffer(singleton->GetCodeSigner(), textData.data());
            AppendData(singleton->GetCodeSigner(), &data, byteSize);
            singleton->signTableSize_ += 1;
        }
#endif
    }

    uint32 GetTextDataElem32(size_t index)
    {
        uint32 value = 0;
        errno_t res = memcpy_s(&value, sizeof(uint32), textData.data() + index, sizeof(uint32));
        CHECK_FATAL(res == EOK, "call memcpy_s failed");
        return value;
    }

    uint64 GetTextDataElem64(size_t index)
    {
        uint64 value = 0;
        errno_t res = memcpy_s(&value, sizeof(uint64), textData.data() + index, sizeof(uint64));
        CHECK_FATAL(res == EOK, "call memcpy_s failed");
        return value;
    }

    ALWAYS_INLINE void SwapTextData(const void *value, size_t index, size_t byteSize)
    {
        errno_t res = memcpy_s(textData.data() + index, byteSize, value, byteSize);
        CHECK_FATAL(res == EOK, "call memcpy_s failed");
#ifdef JIT_ENABLE_CODE_SIGN
        if (CGOptions::UseJitCodeSign()) {
            JitSignCode *singleton = JitSignCode::GetInstance();
            RegisterTmpBuffer(singleton->GetCodeSigner(), textData.data());
            res = PatchData(singleton->GetCodeSigner(), index, textData.data() + index, byteSize);
        }
#endif
    }

    void FillTextDataPadding(uint32 padding)
    {
        for (uint32 i = 0; i < padding; ++i) {
            textData.push_back(0);
        }
    }

    void FillTextDataNop(uint32 padding)
    {
        DEBUG_ASSERT(padding % k4ByteSize == 0, "padding is not a multiple of 4!\n");
        uint32 nopNum = padding >> k2BitSize;
        for (uint32 i = 0; i < nopNum; i++) {
            AppendTextData(0xd503201f, k4ByteSize);
        }
    }

    void SetSwitchTableOffset(const std::string &name, uint32 offset)
    {
        MapleString switchTableName(name, &memPool);
        switchTableOffset[switchTableName] = offset;
    }

    const MapleMap<MapleString, uint32> &GetSwitchTableOffset() const
    {
        return switchTableOffset;
    }

    const MethodHeader &GetMethodHeader() const
    {
        return methodHeader;
    }

    void UpdateMethodCodeSize()
    {
        methodHeader.codeSize = static_cast<uint32>(GetTextDataSize());
    }

    void AppendLabel2Order(uint32 label)
    {
        (void)label2Order.insert(std::make_pair(label, order));
        order++;
    }

    uint32 GetLabelOrder(uint32 label) const
    {
        auto itr = label2Order.find(label);
        CHECK_FATAL(itr != label2Order.end(), "not found label");
        return itr->second;
    }

    void RecordOffset2StackMapInfo(size_t offset, const std::vector<uint8> &referenceMap,
                                   const std::vector<uint8> deoptInfo)
    {
        offset2StackMapInfo.insert(std::pair<size_t, StackMapInfo>(offset, {referenceMap, deoptInfo}));
    }

    MapleUnorderedMap<size_t, StackMapInfo> &GetOffset2StackMapInfo()
    {
        return offset2StackMapInfo;
    }

protected:
    MemPool &memPool;
    MapleAllocator alloc;
    MapleVector<LocalFixup *> localFixups;
    MapleVector<Fixup *> globalFixups;
    MapleVector<Fixup *> relocations;
    MapleVector<uint8> textData;
    MapleMap<uint32, uint32> label2Order; /* this is used to sort callsite */
    MapleMap<MapleString, uint32> switchTableOffset;
    MapleUnorderedMap<size_t, StackMapInfo> offset2StackMapInfo;
    uint32 endOffset = 0;
    uint32 startOffset = 0;
    uint32 exceptStartOffset = 0;
    MapleString funcName;
    MethodHeader methodHeader;
    uint32 order = 0;
};

class ObjEmitter : public Emitter {
public:
    ObjEmitter(CG &cg, const std::string &objFileName)
        : Emitter(cg, objFileName), alloc(memPool), sections(alloc.Adapter()), contents(alloc.Adapter())
    {
        const auto &emitMemoryManager = maplebe::CGOptions::GetInstance().GetEmitMemoryManager();
        if (emitMemoryManager.codeSpace == nullptr) {
            fileStream.open(objFileName, std::ios::trunc | std::ios::binary);
        }

        uint32 funcNum = 0;
        for (auto func : cg.GetMIRModule()->GetFunctionList()) {
            if (func->GetBody() != nullptr) {
                funcNum++;
            }
        }
        contents.resize(funcNum);
    }

    virtual ~ObjEmitter() = default;

    void EmitFuncBinaryCode(ObjFuncEmitInfo &objFuncEmitInfo);
    void EmitInstructions(ObjFuncEmitInfo &objFuncEmitInfo, std::vector<uint32> &label2Offset);
    void EmitLocalFloatValue(ObjFuncEmitInfo &objFuncEmitInfo);
    void EmitFullLSDA(ObjFuncEmitInfo &objFuncEmitInfo, const std::vector<uint32> &label2Offset);
    void EmitSwitchTable(ObjFuncEmitInfo &objFuncEmitInfo, const std::vector<uint32> &symbol2Offset);
    void WriteObjFile();

    void HandleGlobalFixup()
    {
        for (auto *section : sections) {
            section->HandleGlobalFixup(globalLabel2Offset);
        }
    }

    void Run(FuncEmitInfo &funcEmitInfo);
    void EmitFuncBuffer(CGFunc &cgFunc);

    FuncEmitInfo &CreateFuncEmitInfo(CGFunc &cgFunc)
    {
        CHECK_FATAL(false, "this function should be implemented in subclass");
        MemPool *memPool = cgFunc.GetCG()->GetMIRModule()->GetMemPool();
        return *memPool->New<ObjFuncEmitInfo>(cgFunc, *memPool);
    }

    void InitELFHeader();
    void AddFuncSymbol(const MapleString &name, Word size, Address value);
    void ClearData();
    void HandleExceptFixup();

    void UpdateSectionOffsetAddr(Section *section)
    {
        if (section->GetType() != SHT_NOBITS) {
            section->SetOffset(globalOffset);
        } else {
            section->SetOffset(0);
        }
    }

    void UpdateGlobalOffsetAddr(Section *section)
    {
        if ((section->GetFlags() & SHF_ALLOC) != 0) {
            globalAddr += section->GetDataSize();
        }
        if (section->GetType() != SHT_NOBITS) {
            globalOffset += section->GetDataSize();
        }
    }

    void RegisterSection(Section *section)
    {
        sections.push_back(section);
        DEBUG_ASSERT(sections.size() > 0, "sections not empty");
        section->SetIndex(sections.size() - 1);
    }

    void RegisterGlobalLabel(const std::string labelName, ObjLabel label)
    {
        (void)globalLabel2Offset.insert(std::make_pair(labelName, label));
    }

    size_t AddSectionName(const std::string &name)
    {
        return name.empty() ? 0 : shStrSection->AddString(name);
    }

    void Finish() override
    {
        InitSections();
        AppendGlobalLabel();
        AppendSymsToSymTabSec();
        HandleTextSectionGlobalFixup();
        AppendTextSectionData();
        LayoutSections();
        WriteObjFile();
        ClearData();
    }

    void CloseOutput() override
    {
        if (fileStream.is_open()) {
            fileStream << outStream.str();
            fileStream.close();
        }
    }

    virtual void EncodeInstruction(const Insn &insn, const std::vector<uint32> &label2Offset,
                                   ObjFuncEmitInfo &objFuncEmitInfo) = 0;
    virtual uint32 GetInsnSize(const Insn &insn) const = 0;
    virtual void HandleTextSectionGlobalFixup() = 0;
    virtual void AppendTextSectionData() = 0;
    virtual void AppendGlobalLabel() = 0;
    virtual void AppendSymsToSymTabSec() = 0;
    virtual void InitSections() = 0;
    virtual void LayoutSections() = 0;
    virtual void UpdateMachineAndFlags(FileHeader &header) = 0;

    MapleVector<ObjFuncEmitInfo *> &GetContents()
    {
        return contents;
    }

    size_t GetBeforeTextDataSize(ObjFuncEmitInfo &objFuncEmitInfo) const
    {
        size_t textDataSize = 0;
        for (auto *content : contents) {
            if (content == nullptr) {
                continue;
            }
            textDataSize += content->GetTextDataSize();
            if (content->GetFuncName() == objFuncEmitInfo.GetFuncName()) {
                break;
            }
        }
        return textDataSize;
    }

    void EmitFunctionSymbolTable(ObjFuncEmitInfo &objFuncEmitInfo, std::vector<uint32> &symbol2Offset);
    void EmitStr16Const(ObjFuncEmitInfo &objFuncEmitInfo, const MIRSymbol &str16Symbol);
    void EmitStrConst(ObjFuncEmitInfo &objFuncEmitInfo, const MIRSymbol &strSymbol);

protected:
    virtual void InsertNopInsn(ObjFuncEmitInfo &objFuncEmitInfo) const = 0;
    virtual void EmitIntrinsicInsn(const Insn &insn, ObjFuncEmitInfo &objFuncEmitInfo) = 0;
    virtual void EmitSpinIntrinsicInsn(const Insn &insn, ObjFuncEmitInfo &objFuncEmitInfo) = 0;

    MapleString fileName;
    MapleAllocator alloc;
    MapleVector<Section *> sections;
    Offset globalOffset = 0; /* global offset of the ifile */
    Address globalAddr = 0;  /* global adress of the ifile */
    FileHeader header {};
    StringSection *shStrSection = nullptr;
    StringSection *strTabSection = nullptr;
    SymbolSection *symbolTabSection = nullptr;
    DataSection *textSection = nullptr;
    DataSection *dataSection = nullptr;
    DataSection *rodataSection = nullptr;
    RelaSection *relaSection = nullptr;
    MapleVector<ObjFuncEmitInfo *> contents; /* each item is the code info of a cgfunc */
    Label2OffsetMap globalLabel2Offset;      /* record global info */
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_OBJ_EMIT_H */
