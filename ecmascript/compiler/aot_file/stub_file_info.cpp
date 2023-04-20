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

#include "ecmascript/compiler/aot_file/stub_file_info.h"
#include "ecmascript/js_file_path.h"
#include "ecmascript/platform/file.h"

extern const uint8_t _binary_stub_an_start[];
extern const uint32_t _binary_stub_an_length;

namespace panda::ecmascript {
void StubFileInfo::Save(const std::string &filename)
{
    std::string realPath;
    if (!RealPath(filename, realPath, false)) {
        return;
    }

    if (totalCodeSize_ == 0) {
        LOG_COMPILER(FATAL) << "error: code size of generated stubs is empty!";
    }

    std::ofstream file(realPath.c_str(), std::ofstream::binary);
    SetStubNum(entries_.size());
    file.write(reinterpret_cast<char *>(&entryNum_), sizeof(entryNum_));
    file.write(reinterpret_cast<char *>(entries_.data()), sizeof(FuncEntryDes) * entryNum_);
    uint32_t moduleNum = GetCodeUnitsNum();
    file.write(reinterpret_cast<char *>(&moduleNum), sizeof(moduleNum_));
    file.write(reinterpret_cast<char *>(&totalCodeSize_), sizeof(totalCodeSize_));
    uint32_t asmStubSize = GetAsmStubSize();
    file.write(reinterpret_cast<char *>(&asmStubSize), sizeof(asmStubSize));
    uint64_t asmStubAddr = GetAsmStubAddr();
    file.write(reinterpret_cast<char *>(asmStubAddr), asmStubSize);
    for (size_t i = 0; i < moduleNum; i++) {
        des_[i].SaveSectionsInfo(file);
    }
    file.close();
}

bool StubFileInfo::Load()
{
    if (_binary_stub_an_length <= 1) {
        LOG_FULL(FATAL) << "stub.an length <= 1, is default and invalid.";
        return false;
    }

    BinaryBufferParser binBufparser(const_cast<uint8_t *>(_binary_stub_an_start), _binary_stub_an_length);
    binBufparser.ParseBuffer(&entryNum_, sizeof(entryNum_));
    entries_.resize(entryNum_);
    binBufparser.ParseBuffer(entries_.data(), sizeof(FuncEntryDes) * entryNum_);
    binBufparser.ParseBuffer(&moduleNum_, sizeof(moduleNum_));
    des_.resize(moduleNum_);
    binBufparser.ParseBuffer(&totalCodeSize_, sizeof(totalCodeSize_));

    if (totalCodeSize_ == 0) {
        LOG_COMPILER(ERROR) << "error: code in the binary stub is empty!";
        return false;
    }

    ExecutedMemoryAllocator::AllocateBuf(totalCodeSize_, stubsMem_);
    uint64_t codeAddress = reinterpret_cast<uint64_t>(stubsMem_.addr_);
    uint32_t curUnitOffset = 0;
    uint32_t asmStubSize = 0;
    binBufparser.ParseBuffer(&asmStubSize, sizeof(asmStubSize));
    SetAsmStubSize(asmStubSize);
    binBufparser.ParseBuffer(reinterpret_cast<void *>(codeAddress), asmStubSize);
    SetAsmStubAddr(codeAddress);
    curUnitOffset += asmStubSize;

    for (size_t i = 0; i < moduleNum_; i++) {
        des_[i].LoadSectionsInfo(binBufparser, curUnitOffset, codeAddress);
    }

    for (auto &entry : entries_) {
        if (entry.IsGeneralRTStub()) {
            uint64_t begin = GetAsmStubAddr();
            entry.codeAddr_ += begin;
        } else {
            auto moduleDes = des_[entry.moduleIndex_];
            entry.codeAddr_ += moduleDes.GetSecAddr(ElfSecName::TEXT);
        }
    }
    LOG_COMPILER(INFO) << "loaded stub file successfully";
    PageProtect(stubsMem_.addr_, stubsMem_.size_, PAGE_PROT_EXEC_READ);
    return true;
}

void StubFileInfo::Dump() const
{
    uint64_t asmAddr = GetAsmStubAddr();
    uint64_t asmSize = GetAsmStubSize();

    LOG_COMPILER(ERROR) << "Stub file loading: ";
    LOG_COMPILER(ERROR) << " - asm stubs [0x" << std::hex << asmAddr << ", 0x" << std::hex << asmAddr + asmSize << "]";

    for (const ModuleSectionDes &d : des_) {
        for (const auto &s : d.GetSectionsInfo()) {
            std::string name = d.GetSecName(s.first);
            uint32_t size = d.GetSecSize(s.first);
            uint64_t addr = d.GetSecAddr(s.first);
            LOG_COMPILER(ERROR) << " - section = " << name << " [0x" << std::hex << addr << ", 0x" << std::hex
                                << addr + size << "]";
        }
    }
}
}  // namespace panda::ecmascript
