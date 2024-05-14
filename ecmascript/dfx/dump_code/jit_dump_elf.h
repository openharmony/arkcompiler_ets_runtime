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

#ifndef ECMASCRIPT_JIT_DUMP_ELF_H
#define ECMASCRIPT_JIT_DUMP_ELF_H

#include <vector>
#include <string>
#include <unistd.h>
#include <unordered_map>

#include "ecmascript/compiler/codegen/maple/maple_be/include/cg/elf_types.h"
#include "ecmascript/log_wrapper.h"

namespace panda::ecmascript {

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using uintptr = uintptr_t;

const uint8 kLeftShift4Bits = 4;
static const uint8 k8Bits = 8;

class Section {
public:
    Section(const std::string &name, maplebe::Word type, maplebe::Xword flags, maplebe::Xword align) : name(name)
    {
        sectionHeader.sh_type = type;
        sectionHeader.sh_flags = flags;
        sectionHeader.sh_addralign = align;
    }

    virtual ~Section() = default;
    virtual void GenerateData() = 0;
    virtual void WriteSection(int fd) = 0;

    virtual void ClearData()
    {
        return;
    }

    void SetIndex(maplebe::SectionIndex idx)
    {
        sectionIndex = idx;
    }

    maplebe::SectionIndex GetIndex() const
    {
        return sectionIndex;
    }

    void SetInfo(maplebe::Word value)
    {
        sectionHeader.sh_info = value;
    }

    void SetLink(const Section &section)
    {
        sectionHeader.sh_link = static_cast<maplebe::Word>(section.GetIndex());
    }

    void SetEntSize(maplebe::Xword value)
    {
        sectionHeader.sh_entsize = value;
    }

    void SetSectionSize(maplebe::Xword size)
    {
        sectionHeader.sh_size = size;
    }

    virtual maplebe::Xword GetSectionSize()
    {
        return sectionHeader.sh_size;
    }

    void SetAddr(maplebe::Address addr)
    {
        sectionHeader.sh_addr = addr;
    }

    maplebe::Address GetAddr() const
    {
        return sectionHeader.sh_addr;
    }

    maplebe::Xword GetFlags() const
    {
        return sectionHeader.sh_flags;
    }

    void SetOffset(maplebe::Offset value)
    {
        sectionHeader.sh_offset = value;
    }

    maplebe::Offset GetOffset() const
    {
        return sectionHeader.sh_offset;
    }

    maplebe::Xword GetAlign() const
    {
        return sectionHeader.sh_addralign;
    }

    const std::string &GetName() const
    {
        return name;
    }

    void SetSectionHeaderNameIndex(maplebe::Word index)
    {
        sectionHeader.sh_name = index;
    }

    maplebe::Word GetType() const
    {
        return sectionHeader.sh_type;
    }

    const maplebe::SectionHeader &GetSectionHeader() const
    {
        return sectionHeader;
    }

private:
    std::string name;
    maplebe::SectionIndex sectionIndex {};
    maplebe::SectionHeader sectionHeader {};
}; /* class Section */

class RelaSection : public Section {
public:
    RelaSection(const std::string &name, maplebe::Word type, maplebe::Xword flags, maplebe::Word info,
        maplebe::Xword align, const Section &link) : Section(name, type, flags, align)
    {
        SetEntSize(sizeof(maplebe::Rela));
        SetInfo(info);
        SetLink(link);
    }

    ~RelaSection() = default;

    void GenerateData() override
    {
        SetSectionSize(relas.size() * sizeof(maplebe::Rela));
    }

    void WriteSection(int fd) override
    {
        (void)write(fd, reinterpret_cast<const char *>(relas.data()), relas.size() * sizeof(maplebe::Rela));
    }

    void AppendRela(maplebe::Rela rela)
    {
        relas.push_back(rela);
    }

private:
    std::vector<maplebe::Rela> relas;
}; /* class RelaSection */

class SymbolSection : public Section {
public:
    SymbolSection(const std::string &name, maplebe::Word type, maplebe::Xword flags, maplebe::Xword align,
        const Section &link) : Section(name, type, flags, align)
    {
        SetEntSize(sizeof(maplebe::Symbol));
        SetLink(link);
        SetInfo(1);
        AppendSymbol({0, 0, 0, 0, 0, 0});
    }

    ~SymbolSection() = default;

    void GenerateData() override
    {
        SetSectionSize(symbols.size() * sizeof(maplebe::Symbol));
    }

    void WriteSection(int fd) override
    {
        (void)write(fd, reinterpret_cast<const char *>(symbols.data()), symbols.size() * sizeof(maplebe::Symbol));
    }

    void AppendSymbol(const maplebe::Symbol &symbol)
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
        symbolIdxMap[symIdx] = static_cast<uint64>(GetSymbolsSize() - 1);
    }

    bool ExistSymInSymbols(int64 symIdx)
    {
        return symbolIdxMap.count(symIdx) != 0;
    }

    uint32 GetDataSize() const
    {
        return symbols.size() * sizeof(maplebe::Symbol);
    }

    const char *GetAddr()
    {
        return reinterpret_cast<const char*>(symbols.data());
    }

    void ClearData() override
    {
        symbols.clear();
        symbolIdxMap.clear();
    }

private:
    std::vector<maplebe::Symbol> symbols;
    std::unordered_map<int64, uint64> symbolIdxMap;
}; /* class SymbolSection */

class DataSection : public Section {
public:
    DataSection(const std::string &name, maplebe::Word type, maplebe::Xword flags, maplebe::Xword align)
        : Section(name, type, flags, align) {}

    ~DataSection() = default;

    virtual void GenerateData() override
    {
        SetSectionSize(data.size());
    }

    virtual void WriteSection(int fd) override
    {
        (void)write(fd, reinterpret_cast<const char *>(data.data()), data.size());
    }

    void AppendData(const void *value, size_t size)
    {
        auto pdata = reinterpret_cast<const uint8 *>(value);
        data.insert(data.end(), pdata, pdata + size);
    }

    void AppendData(int64 value, size_t size)
    {
        for (size_t i = 0; i < size; i++) {
            auto pdata = static_cast<uint8>(value >> (i * k8Bits));
            data.push_back(pdata);
        }
    }

    void ClearData() override
    {
        data.clear();
    }

    uint32 GetDataSize() const
    {
        return data.size();
    }

    const std::vector<uint8> &GetData() const
    {
        return data;
    }

protected:
    std::vector<uint8> data;
}; /* class DataSection */

class StringSection : public DataSection {
public:
    StringSection(const std::string &name, maplebe::Word type, maplebe::Xword flags, maplebe::Xword align)
        : DataSection(name, type, flags, align)
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
}; /* class StringSection */

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

class JsJitDumpElf {
public:
    void Init();
    void SetFileOffset(int fd, uint64 offset);
    void Initx86ElfHeader();
    void InitArmElfHeader();
    void WriteJitElfFile(int fd);
    void LayoutSections();
    void UpdateSectionOffset(Section &section);
    void UpdateGlobalOffset(Section &section);
    void RegisterSection(Section &section);
    void AppendData(std::vector<uint8> &codeBuff);
    void AddSymToSymTab(const maplebe::Symbol &symbol, int64 symIdx);
    void AppendGlobalSymsToSymTabSec();
    void AppendSymbolToSymTab(int64 symIdx, uint64 funcSymValue, uint64 funcSymSize, const std::string &symbolName);
    void ClearData();
private:
    std::vector<std::pair<maplebe::Symbol, int64>> localSymTab;
    DataSection *textSection = nullptr;
    SymbolSection *symbolTabSection = nullptr;
    maplebe::FileHeader header {};
    StringSection *strTabSection = nullptr;
    std::vector<Section *> sections;
    maplebe::Offset globalOffset = 0; /* global offset of the elf file */
};

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JIT_DUMP_ELF_H
