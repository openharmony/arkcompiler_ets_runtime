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

#ifndef MAPLEBE_INCLUDE_CG_IFILE_H
#define MAPLEBE_INCLUDE_CG_IFILE_H

#include <fstream>
#include <sstream>
#include "elf_types.h"
#include "types_def.h"
#include "common_utils.h"
#include "mempool.h"
#include "mempool_allocator.h"
#include "maple_string.h"
#include "mir_symbol.h"
#include "cg.h"

namespace maplebe {
class ObjEmitter;
class Fixup;

class Alignment {
public:
    template <typename T>
    static T Align(T offset, T align)
    {
        if (align <= 1) {
            return offset;
        }
        return (offset + align - 1) & (~(align - 1));
    }
}; /* class Alignment */

struct ObjLabel {
    Offset offset;
    size_t size;
};

enum LabelFixupKind : uint8 {
    kLabelFixupDirect,
    kLabelFixupInDirect,
    kLabelFixupInDirectAdd,
    kLabelFixupSize,
    kLabelFixupDirect64,
    kLabelFixupInDirect64,
    kLabelFixupInDirectAdd64,
    kLabelFixupSize64,
    kLabelFixupGctib,
};

enum IFileClassStatus : uint32 { kIFileClassResolved, kIFileClassUnresolved, kIFileClassUnknown };

class LabelFixup {
public:
    LabelFixup(const std::string &label, uint32 offset, LabelFixupKind kind) : label(label), offset(offset), kind(kind)
    {
    }
    ~LabelFixup() = default;

    const std::string GetLabel() const
    {
        return label;
    }

    uint32 GetOffset() const
    {
        return offset;
    }

    LabelFixupKind GetFixupKind() const
    {
        return kind;
    }

private:
    std::string label;
    uint32 offset;
    LabelFixupKind kind;
};

using Label2OffsetMap = std::unordered_map<std::string, ObjLabel>;
using LabelFixupVec = MapleVector<LabelFixup *>;

struct SectionDesc {
    uint32 sectionOffset = 0;
    uint32 sectionSize = 0;
};

class Section {
public:
    Section(std::string name, Word type, Word flags, uint32 align, ObjEmitter &emitter, MemPool &memPool);

    virtual ~Section() = default;
    virtual void GenerateData() = 0;
    virtual void WriteSection(std::ostringstream &outStream) = 0;
    virtual void ClearData()
    {
        return;
    }

    virtual void HandleGlobalFixup(const Label2OffsetMap &globalLabel2Offset)
    {
        (void)globalLabel2Offset;
        return;
    }

    virtual void HandleLocalFixup() {}

    virtual void Layout();

    void SetIndex(uint32 idx)
    {
        index = idx;
    }

    void SetInfo(uint32 value)
    {
        secHeader.sh_info = value;
    }

    void SetLink(const Section &section)
    {
        secHeader.sh_link = section.GetIndex();
    }

    void SetEntSize(uint32 value)
    {
        secHeader.sh_entsize = value;
    }

    void SetDataSize(Word size)
    {
        secHeader.sh_size = size;
    }

    virtual uint32 GetDataSize()
    {
        return secHeader.sh_size;
    }

    void SetAddr(Address addr)
    {
        secHeader.sh_addr = addr;
    }

    Address GetAddr() const
    {
        return secHeader.sh_addr;
    }

    Word GetFlags() const
    {
        return secHeader.sh_flags;
    }

    void SetOffset(Offset value)
    {
        secHeader.sh_offset = value;
    }

    Offset GetOffset() const
    {
        return secHeader.sh_offset;
    }

    SectionIndex GetIndex() const
    {
        return index;
    }

    Word GetAlign() const
    {
        return secHeader.sh_addralign;
    }

    const MapleString &GetName() const
    {
        return name;
    }

    void SetSectionHeaderNameIndex(size_t index)
    {
        secHeader.sh_name = index;
    }

    Word GetType() const
    {
        return secHeader.sh_type;
    }

    const SectionHeader &GetSectionHeader() const
    {
        return secHeader;
    }

protected:
    ObjEmitter &emitter;

private:
    MapleString name;
    SectionIndex index {};
    SectionHeader secHeader {};
};

class DataSection : public Section {
public:
    DataSection(const std::string &name, Word type, Word flags, Word align, ObjEmitter &emitter, MemPool &inputMemPool)
        : Section(name, type, flags, align, emitter, inputMemPool),
          memPool(inputMemPool),
          alloc(&memPool),
          data(alloc.Adapter())
    {
    }

    ~DataSection() = default;

    virtual void GenerateData() override
    {
        SetDataSize(data.size());
        if (GetName() == ".ifile.hex") {
            SetDataSize(k8BitSize);
        }
    }

    virtual void ClearData() override
    {
        data.clear();
    }

    virtual void HandleGlobalFixup(const Label2OffsetMap &globalLabel2Offset) override
    {
        (void)globalLabel2Offset;
        return;
    }

    virtual void WriteSection(std::ostringstream &outStream) override
    {
        outStream.write(reinterpret_cast<const char *>(data.data()), data.size());
    }

    void AppendData(const void *value, size_t size)
    {
        auto pdata = reinterpret_cast<const uint8 *>(value);
        data.insert(data.end(), pdata, pdata + size);
    }

    void AppendData(MapleVector<uint8> value)
    {
        data.insert(data.end(), value.begin(), value.end());
    }

    uint32 GetDataSize() override
    {
        return data.size();
    }

    void FillPadding(uint32 paddingNum)
    {
        if (paddingNum == 0) {
            return;
        }
        std::vector<uint8> paddingData(paddingNum, 0);
        auto pdata = reinterpret_cast<const uint8 *>(paddingData.data());
        data.insert(data.end(), pdata, pdata + paddingNum);
    }

    uint32 GetDataElem32(size_t index)
    {
        uint32 value = 0;
        errno_t res = memcpy_s(&value, sizeof(uint32), data.data() + index, sizeof(uint32));
        CHECK_FATAL(res == EOK, "call memcpy_s failed");
        return value;
    }

    uint64 GetDataElem64(size_t index)
    {
        uint64 value = 0;
        errno_t res = memcpy_s(&value, sizeof(uint64), data.data() + index, sizeof(uint64));
        CHECK_FATAL(res == EOK, "call memcpy_s failed");
        return value;
    }

    const MapleVector<uint8> &GetData() const
    {
        return data;
    }

    void Swap(const void *value, size_t index, size_t size)
    {
        errno_t res = memcpy_s(data.data() + index, size, value, size);
        CHECK_FATAL(res == EOK, "call memcpy_s failed");
    }

    static void AddLabel2Offset(Label2OffsetMap &label2Offsets, const std::string &name, ObjLabel &objLabel)
    {
        label2Offsets.insert(std::make_pair(name, objLabel));
    }

    static void AddLabelFixup(LabelFixupVec &labelFixups, LabelFixup &labelFixup)
    {
        labelFixups.emplace_back(&labelFixup);
    }

protected:
    MemPool &memPool;
    MapleAllocator alloc;
    MapleVector<uint8> data;
};

struct MethodHeader {
    uint32 methodMetaOffset = 0;
    uint16 localRefOffset = 0;
    uint16 localRefNumber = 0;
    uint32 codeInfoOffset = 0;
    uint32 codeSize = 0;
};

class StringSection : public DataSection {
public:
    StringSection(const std::string &name, Word type, Word flags, Word align, ObjEmitter &emitter, MemPool &memPool)
        : DataSection(name, type, flags, align, emitter, memPool)
    {
        AddString("\0");
    }

    ~StringSection() = default;

    size_t AddString(const std::string &str)
    {
        size_t pos = data.size();
        AppendData(str.c_str(), str.size() + 1);
        return pos;
    }

    size_t AddString(const MapleString &str)
    {
        size_t pos = data.size();
        AppendData(str.c_str(), str.length() + 1);
        return pos;
    }
};

class SymbolSection : public Section {
public:
    SymbolSection(const std::string &name, Word type, Word flags, Word align, ObjEmitter &emitter, MemPool &memPool,
                  const Section &link)
        : Section(name, type, flags, align, emitter, memPool), alloc(&memPool), symbols(alloc.Adapter())
    {
        SetEntSize(sizeof(Symbol));
        SetLink(link);
        SetInfo(1);
        AppendSymbol({0, 0, 0, 0, 0, 0});
    }

    ~SymbolSection() = default;

    void GenerateData() override
    {
        SetDataSize(symbols.size() * sizeof(Symbol));
    }

    void WriteSection(std::ostringstream &outStream) override
    {
        outStream.write(reinterpret_cast<const char *>(symbols.data()), symbols.size() * sizeof(Symbol));
    }

    void AppendSymbol(Symbol symbol)
    {
        symbols.push_back(symbol);
    }

    uint32 GetSymbolsSize() const
    {
        return symbols.size();
    }

    uint64 GetIdxInSymbols(int64 symIdx) const
    {
        return symbolIdxMap.at(symIdx);
    }

    void AppendIdxInSymbols(int64 symIdx)
    {
        CHECK_FATAL(GetSymbolsSize() > 0, "must not be zero");
        symbolIdxMap[symIdx] = static_cast<uint64>(GetSymbolsSize() - 1);
    }

    bool ExistSymInSymbols(int64 symIdx)
    {
        return symbolIdxMap.count(symIdx) != 0;
    }

    const char *GetAddr()
    {
        return reinterpret_cast<const char*>(symbols.data());
    }
private:
    MapleAllocator alloc;
    MapleVector<Symbol> symbols;
    std::unordered_map<int64, uint64> symbolIdxMap;
};

class RelaSection : public Section {
public:
    RelaSection(const std::string &name, Word type, Word flags, Word info, Word align, const Section &link,
                ObjEmitter &emitter, MemPool &memPool)
        : Section(name, type, flags, align, emitter, memPool), alloc(&memPool), relas(alloc.Adapter())
    {
        SetEntSize(sizeof(Rela));
        SetInfo(info);
        SetLink(link);
    }

    ~RelaSection() = default;

    void GenerateData() override
    {
        SetDataSize(relas.size() * sizeof(Rela));
    }

    void WriteSection(std::ostringstream &outStream) override
    {
        outStream.write(reinterpret_cast<const char *>(relas.data()), relas.size() * sizeof(Rela));
    }

    void AppendRela(Rela rela)
    {
        relas.push_back(rela);
    }

private:
    MapleAllocator alloc;
    MapleVector<Rela> relas;
}; /* class RelaSection */

struct EmitInfo {
    MIRConst &elemConst;
    Offset &offset;
    Label2OffsetMap &label2Offsets;
    LabelFixupVec &labelFixups;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_IFILE_H */
