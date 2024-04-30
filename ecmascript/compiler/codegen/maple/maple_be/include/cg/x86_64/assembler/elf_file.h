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

#ifndef X64_ASSEMBLER_ELF_FILE_H
#define X64_ASSEMBLER_ELF_FILE_H

#include <vector>
#include "elf_types.h"
#include "operand.h"

namespace assembler {
using namespace maplebe;

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

class Section {
public:
    Section(const std::string &name, Word type, Xword flags, Xword align) : name(name)
    {
        sectionHeader.sh_type = type;
        sectionHeader.sh_flags = flags;
        sectionHeader.sh_addralign = align;
    }

    virtual ~Section() = default;
    virtual void GenerateData() = 0;
    virtual void WriteSection(std::ofstream &outStream) = 0;

    virtual void ClearData()
    {
        return;
    }

    void SetIndex(SectionIndex idx)
    {
        index = idx;
    }

    SectionIndex GetIndex() const
    {
        return index;
    }

    void SetInfo(Word value)
    {
        sectionHeader.sh_info = value;
    }

    void SetLink(const Section &section)
    {
        sectionHeader.sh_link = static_cast<Word>(section.GetIndex());
    }

    void SetEntSize(Xword value)
    {
        sectionHeader.sh_entsize = value;
    }

    void SetSectionSize(Xword size)
    {
        sectionHeader.sh_size = size;
    }

    virtual Xword GetSectionSize()
    {
        return sectionHeader.sh_size;
    }

    void SetAddr(Address addr)
    {
        sectionHeader.sh_addr = addr;
    }

    Address GetAddr() const
    {
        return sectionHeader.sh_addr;
    }

    Xword GetFlags() const
    {
        return sectionHeader.sh_flags;
    }

    void SetOffset(Offset value)
    {
        sectionHeader.sh_offset = value;
    }

    Offset GetOffset() const
    {
        return sectionHeader.sh_offset;
    }

    Xword GetAlign() const
    {
        return sectionHeader.sh_addralign;
    }

    const std::string &GetName() const
    {
        return name;
    }

    void SetSectionHeaderNameIndex(Word index)
    {
        sectionHeader.sh_name = index;
    }

    Word GetType() const
    {
        return sectionHeader.sh_type;
    }

    const SectionHeader &GetSectionHeader() const
    {
        return sectionHeader;
    }

private:
    std::string name;
    SectionIndex index {};
    SectionHeader sectionHeader {};
}; /* class Section */

class DataSection : public Section {
public:
    DataSection(const std::string &name, Word type, Xword flags, Xword align) : Section(name, type, flags, align) {}

    ~DataSection() = default;

    virtual void GenerateData() override
    {
        SetSectionSize(data.size());
    }

    virtual void WriteSection(std::ofstream &outStream) override
    {
        outStream.write(reinterpret_cast<const char *>(data.data()), data.size());
    }

    void AppendData(const void *value, size_t size)
    {
        auto pdata = reinterpret_cast<const uint8 *>(value);
        data.insert(data.end(), pdata, pdata + size);
    }

    void AppendData(int64 value, size_t size)
    {
        for (size_t i = 0; i < size; i++) {
            auto pdata = static_cast<uint8>(static_cast<uint64>(value) >> (i * k8Bits));
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
    StringSection(const std::string &name, Word type, Xword flags, Xword align) : DataSection(name, type, flags, align)
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

class RelaSection : public Section {
public:
    RelaSection(const std::string &name, Word type, Xword flags, Word info, Xword align, const Section &link)
        : Section(name, type, flags, align)
    {
        SetEntSize(sizeof(Rela));
        SetInfo(info);
        SetLink(link);
    }

    ~RelaSection() = default;

    void GenerateData() override
    {
        SetSectionSize(relas.size() * sizeof(Rela));
    }

    void WriteSection(std::ofstream &outStream) override
    {
        outStream.write(reinterpret_cast<const char *>(relas.data()), relas.size() * sizeof(Rela));
    }

    void AppendRela(Rela rela)
    {
        relas.push_back(rela);
    }

private:
    std::vector<Rela> relas;
}; /* class RelaSection */

class SymbolSection : public Section {
public:
    SymbolSection(const std::string &name, Word type, Xword flags, Xword align, const Section &link)
        : Section(name, type, flags, align)
    {
        SetEntSize(sizeof(Symbol));
        SetLink(link);
        SetInfo(1);
        AppendSymbol({0, 0, 0, 0, 0, 0});
    }

    ~SymbolSection() = default;

    void GenerateData() override
    {
        SetSectionSize(symbols.size() * sizeof(Symbol));
    }

    void WriteSection(std::ofstream &outStream) override
    {
        outStream.write(reinterpret_cast<const char *>(symbols.data()), symbols.size() * sizeof(Symbol));
    }

    void AppendSymbol(const Symbol &symbol)
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

    uint32 GetDataSize() const
    {
        return symbols.size() * sizeof(Symbol);
    }

    const char *GetAddr()
    {
        return reinterpret_cast<const char*>(symbols.data());
    }

private:
    std::vector<Symbol> symbols;
    std::unordered_map<int64, uint64> symbolIdxMap;
}; /* class SymbolSection */
} /* namespace assembler */

#endif /* X64_ASSEMBLER_ELF_FILE_H */
