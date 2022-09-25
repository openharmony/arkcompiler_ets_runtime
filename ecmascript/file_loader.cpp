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
#include "ecmascript/file_loader.h"

#ifdef PANDA_TARGET_WINDOWS
#include "shlwapi.h"
#endif

#include "ecmascript/ark_stackmap_parser.h"
#include "ecmascript/base/config.h"
#include "ecmascript/compiler/bc_call_signature.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/llvm_stackmap_parser.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/message_string.h"
#include "ecmascript/jspandafile/constpool_value.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/base/mem_mmap.h"

extern const uint8_t _binary_stub_an_start[];
extern const uint32_t _binary_stub_an_length;

namespace panda::ecmascript {
void ModuleSectionDes::SaveSectionsInfo(std::ofstream &file)
{
    uint32_t secInfoSize = GetSecInfosSize();
    file.write(reinterpret_cast<char *>(&secInfoSize), sizeof(secInfoSize));
    for (auto &s : sectionsInfo_) {
        uint8_t secName = static_cast<uint8_t>(s.first);
        uint32_t curSecSize = GetSecSize(s.first);
        uint64_t curSecAddr = GetSecAddr(s.first);
        file.write(reinterpret_cast<char *>(&secName), sizeof(secName));
        file.write(reinterpret_cast<char *>(&curSecSize), sizeof(curSecSize));
        file.write(reinterpret_cast<char *>(curSecAddr), curSecSize);
    }
    std::shared_ptr<uint8_t> ptr = GetArkStackMapSharePtr();
    uint32_t size = GetArkStackMapSize();
    file.write(reinterpret_cast<char *>(&size), sizeof(size));
    file.write(reinterpret_cast<char *>(ptr.get()), size);

    uint32_t index = GetStartIndex();
    uint32_t cnt = GetFuncCount();
    file.write(reinterpret_cast<char *>(&index), sizeof(index));
    file.write(reinterpret_cast<char *>(&cnt), sizeof(cnt));
}

void ModuleSectionDes::LoadStackMapSection(BinaryBufferParser &parser, uintptr_t secBegin, uint32_t &curUnitOffset)
{
    uint32_t size = 0;
    parser.ParseBuffer(&size, sizeof(size));
    parser.ParseBuffer(reinterpret_cast<void *>(secBegin), size);
    SetArkStackMapSize(size);
    SetArkStackMapPtr(reinterpret_cast<uint8_t *>(secBegin));
    curUnitOffset += size;
    uint32_t index = 0;
    uint32_t cnt = 0;
    parser.ParseBuffer(&index, sizeof(index));
    parser.ParseBuffer(&cnt, sizeof(cnt));
    SetStartIndex(index);
    SetFuncCount(cnt);
}

void ModuleSectionDes::LoadSectionsInfo(BinaryBufferParser &parser,
    uint32_t &curUnitOffset, uint64_t codeAddress)
{
    uint32_t secInfoSize = 0;
    parser.ParseBuffer(&secInfoSize, sizeof(secInfoSize));
    auto secBegin = codeAddress + static_cast<uintptr_t>(curUnitOffset);
    for (uint8_t i = 0; i < secInfoSize; i++) {
        uint8_t secName = 0;
        parser.ParseBuffer(&secName, sizeof(secName));
        auto secEnumName = static_cast<ElfSecName>(secName);
        uint32_t secSize = 0;
        parser.ParseBuffer(&secSize, sizeof(secSize));
        SetSecSize(secSize, secEnumName);
        parser.ParseBuffer(reinterpret_cast<void *>(secBegin), secSize);
        curUnitOffset += secSize;
        SetSecAddr(secBegin, secEnumName);
        secBegin += secSize;
    }
    LoadStackMapSection(parser, secBegin, curUnitOffset);
}

void ModuleSectionDes::LoadStackMapSection(std::ifstream &file, uintptr_t secBegin, uint32_t &curUnitOffset)
{
    uint32_t size;
    file.read(reinterpret_cast<char *>(&size), sizeof(size));
    file.read(reinterpret_cast<char *>(secBegin), size);
    SetArkStackMapSize(size);
    SetArkStackMapPtr(reinterpret_cast<uint8_t *>(secBegin));
    curUnitOffset += size;
    uint32_t index;
    uint32_t cnt;
    file.read(reinterpret_cast<char *>(&index), sizeof(index));
    file.read(reinterpret_cast<char *>(&cnt), sizeof(cnt));
    SetStartIndex(index);
    SetFuncCount(cnt);
}

void ModuleSectionDes::LoadSectionsInfo(std::ifstream &file,
    uint32_t &curUnitOffset, uint64_t codeAddress)
{
    uint32_t secInfoSize;
    file.read(reinterpret_cast<char *>(&secInfoSize), sizeof(secInfoSize));
    auto secBegin = codeAddress + static_cast<uintptr_t>(curUnitOffset);
    for (uint8_t i = 0; i < secInfoSize; i++) {
        uint8_t secName;
        file.read(reinterpret_cast<char *>(&secName), sizeof(secName));
        auto secEnumName = static_cast<ElfSecName>(secName);
        uint32_t secSize;
        file.read(reinterpret_cast<char *>(&secSize), sizeof(secSize));
        SetSecSize(secSize, secEnumName);
        file.read(reinterpret_cast<char *>(secBegin), secSize);
        curUnitOffset += secSize;
        SetSecAddr(secBegin, secEnumName);
        secBegin += secSize;
    }
    LoadStackMapSection(file, secBegin, curUnitOffset);
}

void StubModulePackInfo::Save(const std::string &filename)
{
    if (!VerifyFilePath(filename, true)) {
        return;
    }

    std::ofstream file(filename.c_str(), std::ofstream::binary);
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

bool StubModulePackInfo::Load(EcmaVM *vm)
{
    //  now MachineCode is non movable, code and stackmap sperately is saved to MachineCode
    // by calling NewMachineCodeObject.
    //  then MachineCode will support movable, code is saved to MachineCode and stackmap is saved
    // to different heap which will be freed when stackmap is parsed by EcmaVM is started.
    if (_binary_stub_an_length <= 1) {
        LOG_FULL(FATAL) << "stub.an length <= 1, is default and invalid.";
        return false;
    }
    BinaryBufferParser binBufparser((uint8_t *)_binary_stub_an_start, _binary_stub_an_length);
    binBufparser.ParseBuffer(&entryNum_, sizeof(entryNum_));
    entries_.resize(entryNum_);
    binBufparser.ParseBuffer(entries_.data(), sizeof(FuncEntryDes) * entryNum_);
    binBufparser.ParseBuffer(&moduleNum_, sizeof(moduleNum_));
    des_.resize(moduleNum_);
    uint32_t totalCodeSize = 0;
    binBufparser.ParseBuffer(&totalCodeSize, sizeof(totalCodeSize_));
    int prot = PROT_READ | PROT_WRITE | PROT_EXEC;
    auto pool = MemMapAllocator::GetInstance()->Allocate(AlignUp(totalCodeSize, 256_KB), 0, false, prot);
    vm->GetFileLoader()->SetStubmmap(pool.GetMem(), pool.GetSize());
    uint64_t codeAddress = reinterpret_cast<uint64_t>(pool.GetMem());
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
    LOG_COMPILER(INFO) << "Load stub file success";
    return true;
}

void AOTModulePackInfo::Save(const std::string &filename)
{
    if (!VerifyFilePath(filename, true)) {
        return;
    }
    std::ofstream file(filename.c_str(), std::ofstream::binary);
    SetStubNum(entries_.size());
    file.write(reinterpret_cast<char *>(&entryNum_), sizeof(entryNum_));
    file.write(reinterpret_cast<char *>(entries_.data()), sizeof(FuncEntryDes) * entryNum_);
    uint32_t moduleNum = GetCodeUnitsNum();
    file.write(reinterpret_cast<char *>(&moduleNum), sizeof(moduleNum_));
    file.write(reinterpret_cast<char *>(&totalCodeSize_), sizeof(totalCodeSize_));
    file.write(reinterpret_cast<char *>(aotFileHashs_.data()), sizeof(uint32_t) * moduleNum);
    for (size_t i = 0; i < moduleNum; i++) {
        des_[i].SaveSectionsInfo(file);
    }
    file.close();
}

bool AOTModulePackInfo::Load(EcmaVM *vm, const std::string &filename)
{
    if (!VerifyFilePath(filename)) {
        LOG_COMPILER(ERROR) << "Can not load aot file from path [ "  << filename << " ], "
            << "please execute ark_aot_compiler with options --aot-file.";
        return false;
    }
    std::ifstream file(filename.c_str(), std::ofstream::binary);
    if (!file.good()) {
        file.close();
        return false;
    }
    file.read(reinterpret_cast<char *>(&entryNum_), sizeof(entryNum_));
    entries_.resize(entryNum_);
    file.read(reinterpret_cast<char *>(entries_.data()), sizeof(FuncEntryDes) * entryNum_);
    file.read(reinterpret_cast<char *>(&moduleNum_), sizeof(moduleNum_));
    des_.resize(moduleNum_);
    aotFileHashs_.resize(moduleNum_);
    uint32_t totalCodeSize = 0;
    file.read(reinterpret_cast<char *>(&totalCodeSize), sizeof(totalCodeSize_));
    [[maybe_unused]] EcmaHandleScope handleScope(vm->GetAssociatedJSThread());
    int prot = PROT_READ | PROT_WRITE | PROT_EXEC;
    auto pool = MemMapAllocator::GetInstance()->Allocate(AlignUp(totalCodeSize, 256_KB), 0, false, prot);
    vm->GetFileLoader()->SetAOTmmap(pool.GetMem(), pool.GetSize());
    uint64_t codeAddress = reinterpret_cast<uint64_t>(pool.GetMem());
    file.read(reinterpret_cast<char *>(aotFileHashs_.data()), sizeof(uint32_t) * moduleNum_);
    uint32_t curUnitOffset = 0;
    for (size_t i = 0; i < moduleNum_; i++) {
        des_[i].LoadSectionsInfo(file, curUnitOffset, codeAddress);
    }
    for (size_t i = 0; i < entries_.size(); i++) {
        auto des = des_[entries_[i].moduleIndex_];
        entries_[i].codeAddr_ += des.GetSecAddr(ElfSecName::TEXT);
        auto curFileHash = aotFileHashs_[entries_[i].moduleIndex_];
        auto curMethodId = entries_[i].indexInKind_;
        vm->GetFileLoader()->SaveAOTFuncEntry(curFileHash, curMethodId, entries_[i].codeAddr_);
    }
    file.close();
    LOG_COMPILER(INFO) << "Load aot file success";
    return true;
}

void ModulePackInfo::Iterate(const RootVisitor &v)
{
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&machineCodeObj_)));
}

bool ModulePackInfo::VerifyFilePath([[maybe_unused]] const std::string &filePath,
    [[maybe_unused]] bool toGenerate) const
{
#ifndef PANDA_TARGET_WINDOWS
    if (filePath.size() > PATH_MAX) {
        return false;
    }

    std::vector<char> resolvedPath(PATH_MAX);
    auto result = realpath(filePath.c_str(), resolvedPath.data());
    if (toGenerate && errno == ENOENT) {
        return true;
    }
    if (result == nullptr) {
        return false;
    }
    return true;
#else
    return false;
#endif
}

void FileLoader::LoadStubFile()
{
    if (!stubPackInfo_.Load(vm_)) {
        return;
    }
    auto stubs = stubPackInfo_.GetStubs();
    InitializeStubEntries(stubs);
}

void FileLoader::LoadAOTFile(const std::string &fileName)
{
    AOTModulePackInfo aotPackInfo_;
    if (!aotPackInfo_.Load(vm_, fileName)) {
        return;
    }
    AddAOTPackInfo(aotPackInfo_);
}

void FileLoader::LoadSnapshotFile()
{
    CString snapshotArg(vm_->GetJSOptions().GetAOTOutputFile().c_str());
    CString snapshotPath = snapshotArg + ".etso";
    Snapshot snapshot(vm_);
#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS)
    snapshot.Deserialize(SnapshotType::ETSO, snapshotPath);
#endif
}

bool FileLoader::hasLoaded(const JSPandaFile *jsPandaFile)
{
    auto fileHash = jsPandaFile->GetFileUniqId();
    return hashToEntryMap_.find(fileHash) != hashToEntryMap_.end();
}

void FileLoader::UpdateJSMethods(JSHandle<JSFunction> mainFunc, const JSPandaFile *jsPandaFile)
{
    // get main func method
    auto mainFuncMethodId = jsPandaFile->GetMainMethodIndex();
    auto fileHash = jsPandaFile->GetFileUniqId();
    auto mainEntry = GetAOTFuncEntry(fileHash, mainFuncMethodId);
    MethodLiteral *mainMethod = jsPandaFile->FindMethodLiteral(mainFuncMethodId);
    mainMethod->SetAotCodeBit(true);
    mainMethod->SetNativeBit(false);
    mainMethod->SetCodeEntry(reinterpret_cast<uintptr_t>(mainEntry));
    Method *method = mainFunc->GetCallTarget();
    method->SetCodeEntryAndMarkAOT(reinterpret_cast<uintptr_t>(mainEntry));
}

void FileLoader::SetAOTFuncEntry(const JSPandaFile *jsPandaFile, Method *method)
{
    uint32_t methodId = method->GetMethodId().GetOffset();
    auto codeEntry = GetAOTFuncEntry(jsPandaFile->GetFileUniqId(), methodId);
    if (!codeEntry) {
        return;
    }
    method->SetCodeEntryAndMarkAOT(codeEntry);
}

void FileLoader::SetAOTFuncEntryForLiteral(const JSPandaFile *jsPandaFile, const JSHandle<TaggedArray> &obj)
{
    JSThread *thread = vm_->GetJSThread();
    JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue::Undefined());
    size_t elementsLen = obj->GetLength();
    for (size_t i = 0; i < elementsLen; i++) {
        valueHandle.Update(obj->Get(i));
        if (valueHandle->IsJSFunction()) {
            SetAOTFuncEntry(jsPandaFile, JSHandle<JSFunction>(valueHandle)->GetCallTarget());
        }
    }
}

kungfu::ArkStackMapParser* FileLoader::GetStackMapParser() const
{
    return arkStackMapParser_;
}

void FileLoader::AdjustBCStubAndDebuggerStubEntries(JSThread *thread,
    const std::vector<ModulePackInfo::FuncEntryDes> &stubs,
    const AsmInterParsedOption &asmInterOpt)
{
    auto defaultBCStubDes = stubs[BytecodeStubCSigns::SingleStepDebugging];
    auto defaultBCDebuggerStubDes = stubs[BytecodeStubCSigns::BCDebuggerEntry];
    auto defaultBCDebuggerExceptionStubDes = stubs[BytecodeStubCSigns::BCDebuggerExceptionEntry];
    ASSERT(defaultBCStubDes.kind_ == CallSignature::TargetKind::BYTECODE_HELPER_HANDLER);
    if (asmInterOpt.handleStart >= 0 && asmInterOpt.handleStart <= asmInterOpt.handleEnd) {
        for (int i = asmInterOpt.handleStart; i <= asmInterOpt.handleEnd; i++) {
            thread->SetBCStubEntry(static_cast<size_t>(i), defaultBCStubDes.codeAddr_);
        }
#define DISABLE_SINGLE_STEP_DEBUGGING(name) \
        thread->SetBCStubEntry(BytecodeStubCSigns::ID_##name, stubs[BytecodeStubCSigns::ID_##name].codeAddr_);
        INTERPRETER_DISABLE_SINGLE_STEP_DEBUGGING_BC_STUB_LIST(DISABLE_SINGLE_STEP_DEBUGGING)
#undef DISABLE_SINGLE_STEP_DEBUGGING
    }
    for (size_t i = 0; i < BCStubEntries::EXISTING_BC_HANDLER_STUB_ENTRIES_COUNT; i++) {
        if (i == BytecodeStubCSigns::ID_ExceptionHandler) {
            thread->SetBCDebugStubEntry(i, defaultBCDebuggerExceptionStubDes.codeAddr_);
            continue;
        }
        thread->SetBCDebugStubEntry(i, defaultBCDebuggerStubDes.codeAddr_);
    }
}

void FileLoader::InitializeStubEntries(const std::vector<AOTModulePackInfo::FuncEntryDes>& stubs)
{
    auto thread = vm_->GetAssociatedJSThread();
    for (size_t i = 0; i < stubs.size(); i++) {
        auto des = stubs[i];
        if (des.IsCommonStub()) {
            thread->SetFastStubEntry(des.indexInKind_, des.codeAddr_);
        } else if (des.IsBCStub()) {
            thread->SetBCStubEntry(des.indexInKind_, des.codeAddr_);
#if ECMASCRIPT_ENABLE_ASM_FILE_LOAD_LOG
            auto start = MessageString::ASM_INTERPRETER_START;
            std::string format = MessageString::GetMessageString(des.indexInKind_ + start);
            LOG_ECMA(DEBUG) << "bytecode-" << des.indexInKind_ << " :" << format
                << " addr: 0x" << std::hex << des.codeAddr_;
#endif
        } else if (des.IsBuiltinsStub()) {
            thread->SetBuiltinStubEntry(des.indexInKind_, des.codeAddr_);
        } else {
            thread->RegisterRTInterface(des.indexInKind_, des.codeAddr_);
#if ECMASCRIPT_ENABLE_ASM_FILE_LOAD_LOG
                LOG_ECMA(DEBUG) << "runtime index: " << std::dec << des.indexInKind_
                    << " addr: 0x" << std::hex << des.codeAddr_;
#endif
        }
    }
    AsmInterParsedOption asmInterOpt = vm_->GetJSOptions().GetAsmInterParsedOption();
    AdjustBCStubAndDebuggerStubEntries(thread, stubs, asmInterOpt);
}

bool FileLoader::RewriteDataSection(uintptr_t dataSec, size_t size,
    uintptr_t newData, size_t newSize)
{
    if (memcpy_s(reinterpret_cast<void *>(dataSec), size,
                 reinterpret_cast<void *>(newData), newSize) != EOK) {
        LOG_FULL(FATAL) << "memset failed";
        return false;
    }
    return true;
}

void FileLoader::RuntimeRelocate()
{
    auto desVector = stubPackInfo_.GetModuleSectionDes();
    for (auto &des : desVector) {
        auto dataSec = des.GetSecAddr(ElfSecName::DATA);
        auto dataSecSize = des.GetSecSize(ElfSecName::DATA);
        (void)RewriteDataSection(dataSec, dataSecSize, 0, 0);
    }
}

FileLoader::~FileLoader()
{
    if (arkStackMapParser_ != nullptr) {
        delete arkStackMapParser_;
        arkStackMapParser_ = nullptr;
    }
    for (size_t i = 0; i < aotAddrs_.size(); i++) {
        MemMapAllocator::GetInstance()->Free(aotAddrs_[i].first, aotAddrs_[i].second, false);
    }
    for (size_t i = 0; i < stubAddrs_.size(); i++) {
        MemMapAllocator::GetInstance()->Free(stubAddrs_[i].first, stubAddrs_[i].second, false);
    }
}

FileLoader::FileLoader(EcmaVM *vm) : vm_(vm), factory_(vm->GetFactory())
{
    bool enableLog = vm->GetJSOptions().WasSetCompilerLogOption();
    arkStackMapParser_ = new kungfu::ArkStackMapParser(enableLog);
}

JSTaggedValue FileLoader::GetAbsolutePath(JSThread *thread, JSTaggedValue relativePathVal)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    CString relativePath = ConvertToString(relativePathVal);
    CString absPath;
    if (!GetAbsolutePath(relativePath, absPath)) {
        LOG_FULL(FATAL) << "Get Absolute Path failed";
        return JSTaggedValue::Hole();
    }
    JSTaggedValue absPathVal = factory->NewFromUtf8(absPath).GetTaggedValue();
    return absPathVal;
}

bool FileLoader::GetAbsolutePath(const CString &relativePathCstr, CString &absPathCstr)
{
    std::string relativePath = CstringConvertToStdString(relativePathCstr);
    std::string absPath = "";
    if (GetAbsolutePath(relativePath, absPath)) {
        absPathCstr = ConvertToString(absPath);
        return true;
    }
    return false;
}

bool FileLoader::GetAbsolutePath(const std::string &relativePath, std::string &absPath)
{
    if (relativePath.size() >= PATH_MAX) {
        return false;
    }
    char buffer[PATH_MAX] = {0};
#ifndef PANDA_TARGET_WINDOWS
    auto path = realpath(relativePath.c_str(), buffer);
    if (path == nullptr) {
        return false;
    }
    absPath = std::string(path);
    return true;
#else
    auto path = _fullpath(buffer, relativePath.c_str(), sizeof(buffer) - 1);
    if (path == nullptr) {
        return false;
    }
    absPath = std::string(buffer);
    return true;
#endif
}

void BinaryBufferParser::ParseBuffer(void *dst, uint32_t count)
{
    if (count > 0 && count + offset_ <= length_) {
        if (memcpy_s(dst, count, buffer_ + offset_, count) != EOK) {
            LOG_FULL(FATAL) << "memcpy_s failed";
        }
        offset_ = offset_ + count;
    } else {
        LOG_FULL(FATAL) << "parse buffer error, length is 0 or overflow";
    }
}

void BinaryBufferParser::ParseBuffer(uint8_t *dst, uint32_t count, uint8_t *src)
{
    if (src >= buffer_ && src + count <= buffer_ + length_) {
        if (memcpy_s(dst, count, src, count) != EOK) {
            LOG_FULL(FATAL) << "memcpy_s failed";
        }
    } else {
        LOG_FULL(FATAL) << "parse buffer error, length is 0 or overflow";
    }
}

bool ModulePackInfo::CalCallSiteInfo(uintptr_t retAddr, std::tuple<uint64_t, uint8_t *, int>& ret) const
{
    uint64_t textStart = 0;
    uint8_t *stackmapAddr = nullptr;
    int delta = 0;
    auto& des = GetCodeUnits();
    auto& funcEntryDes = GetStubs();

    auto cmp = [](const ModulePackInfo::FuncEntryDes &a, const ModulePackInfo::FuncEntryDes &b) {
                    return a.codeAddr_ < b.codeAddr_;
                };
    for (size_t i = 0; i < des.size(); i++) {
        auto d = des[i];
        uint64_t addr = d.GetSecAddr(ElfSecName::TEXT);
        uint32_t size = d.GetSecSize(ElfSecName::TEXT);
        if (retAddr < addr || retAddr >= addr + size) {
            continue;
        }
        stackmapAddr = d.GetArkStackMapRawPtr();
        ASSERT(stackmapAddr != nullptr);
        textStart = addr;
        auto startIndex = d.GetStartIndex();
        auto funcCount = d.GetFuncCount();
        auto s = funcEntryDes.begin() + startIndex;
        auto t = funcEntryDes.begin() + startIndex + funcCount;
        ModulePackInfo::FuncEntryDes target;
        target.codeAddr_ = retAddr;
        auto it = std::upper_bound(s, t, target, cmp);
        --it;
        ASSERT(it != t);
        ASSERT((it->codeAddr_ <= target.codeAddr_) && (target.codeAddr_ < it->codeAddr_ + it->funcSize_));
        delta = it->fpDeltaPrevFrameSp_;
        ret = std::make_tuple(textStart, stackmapAddr, delta);
        return true;
    }
    return false;
}
}