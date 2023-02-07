/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/file_generators.h"

#include "ecmascript/compiler/llvm_ir_builder.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/stackmap/ark_stackmap_builder.h"
#include "ecmascript/stackmap/llvm_stackmap_parser.h"

namespace panda::ecmascript::kungfu {
void Module::CollectStackMapDes(ModuleSectionDes& des) const
{
    uint32_t stackmapSize = des.GetSecSize(ElfSecName::LLVM_STACKMAP);
    std::unique_ptr<uint8_t[]> stackmapPtr(std::make_unique<uint8_t[]>(stackmapSize));
    uint64_t addr = des.GetSecAddr(ElfSecName::LLVM_STACKMAP);
    if (addr == 0) { // assembler stub don't existed llvm stackmap
        return;
    }
    uint64_t textAddr = des.GetSecAddr(ElfSecName::TEXT);
    if (memcpy_s(stackmapPtr.get(), stackmapSize, reinterpret_cast<void *>(addr), stackmapSize) != EOK) {
        LOG_FULL(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    }
    std::shared_ptr<uint8_t> ptr = nullptr;
    uint32_t size = 0;
    ArkStackMapBuilder builder;
    std::tie(ptr, size) = builder.Run(std::move(stackmapPtr), textAddr);
    des.EraseSec(ElfSecName::LLVM_STACKMAP);
    des.SetArkStackMapPtr(ptr);
    des.SetArkStackMapSize(size);
}

void StubFileGenerator::CollectAsmStubCodeInfo(std::map<uintptr_t, std::string> &addr2name,
                                               uint32_t bridgeModuleIdx)
{
    uint32_t funSize = 0;
    for (size_t i = 0; i < asmModule_.GetFunctionCount(); i++) {
        auto cs = asmModule_.GetCSign(i);
        auto entryOffset = asmModule_.GetFunction(cs->GetID());
        if (i < asmModule_.GetFunctionCount() - 1) {
            auto nextcs = asmModule_.GetCSign(i + 1);
            funSize = asmModule_.GetFunction(nextcs->GetID()) - entryOffset;
        } else {
            funSize = asmModule_.GetBufferSize() - entryOffset;
        }
        stubInfo_.AddEntry(cs->GetTargetKind(), false, cs->GetID(), entryOffset, bridgeModuleIdx, 0, funSize);
        ASSERT(!cs->GetName().empty());
        auto curSecBegin = asmModule_.GetBuffer();
        uintptr_t entry = reinterpret_cast<uintptr_t>(curSecBegin) + entryOffset;
        addr2name[entry] = cs->GetName();
    }
}

void StubFileGenerator::CollectCodeInfo()
{
    std::map<uintptr_t, std::string> addr2name;
    for (size_t i = 0; i < modulePackage_.size(); i++) {
        modulePackage_[i].CollectFuncEntryInfo(addr2name, stubInfo_, i, GetLog());
        ModuleSectionDes des;
        modulePackage_[i].CollectModuleSectionDes(des, true);
        stubInfo_.AddModuleDes(des);
    }
    // idx for bridge module is the one after last module in modulePackage
    CollectAsmStubCodeInfo(addr2name, modulePackage_.size());
    DisassembleEachFunc(addr2name);
}

void AOTFileGenerator::CollectCodeInfo()
{
    std::map<uintptr_t, std::string> addr2name;
    for (size_t i = 0; i < modulePackage_.size(); i++) {
        modulePackage_[i].CollectFuncEntryInfo(addr2name, aotInfo_, i, GetLog());
        ModuleSectionDes des;
        modulePackage_[i].CollectModuleSectionDes(des);
        aotInfo_.AddModuleDes(des);
    }
    DisassembleEachFunc(addr2name);
}

void StubFileGenerator::RunAsmAssembler()
{
    NativeAreaAllocator allocator;
    Chunk chunk(&allocator);
    asmModule_.Run(&cfg_, &chunk);

    auto buffer = asmModule_.GetBuffer();
    auto bufferSize = asmModule_.GetBufferSize();
    if (bufferSize == 0U) {
        return;
    }
    stubInfo_.FillAsmStubTempHolder(buffer, bufferSize);
    stubInfo_.accumulateTotalSize(bufferSize);
}

void StubFileGenerator::SaveStubFile(const std::string &filename)
{
    RunLLVMAssembler();
    RunAsmAssembler();
    CollectCodeInfo();
    stubInfo_.Save(filename);
}

void AOTFileGenerator::SaveAOTFile(const std::string &filename)
{
    TimeScope timescope("LLVMCodeGenPass-AN", const_cast<CompilerLog *>(log_));
    RunLLVMAssembler();
    CollectCodeInfo();
    GenerateMethodToEntryIndexMap();
    aotInfo_.Save(filename, cfg_.GetTriple());
    DestoryModule();
}

void AOTFileGenerator::SaveSnapshotFile()
{
    TimeScope timescope("LLVMCodeGenPass-AI", const_cast<CompilerLog *>(log_));
    Snapshot snapshot(vm_);
    const CString snapshotPath(vm_->GetJSOptions().GetAOTOutputFile().c_str());
    vm_->GetTSManager()->ResolveSnapshotConstantPool(methodToEntryIndexMap_);
    snapshot.Serialize(snapshotPath + AOTFileManager::FILE_EXTENSION_AI);
}
}  // namespace panda::ecmascript::kungfu
