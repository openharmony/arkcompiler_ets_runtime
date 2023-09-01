/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
#include "ecmascript/compiler/aot_file/aot_file_manager.h"

#include "ecmascript/base/config.h"
#include "ecmascript/base/file_header.h"
#include "ecmascript/compiler/aot_file/an_file_data_manager.h"
#include "ecmascript/compiler/aot_file/elf_builder.h"
#include "ecmascript/compiler/aot_file/elf_reader.h"
#include "ecmascript/compiler/bc_call_signature.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/compiler_log.h"
#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_file_path.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/jspandafile/constpool_value.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/message_string.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/stackmap/ark_stackmap_parser.h"
#include "ecmascript/stackmap/llvm_stackmap_parser.h"

namespace panda::ecmascript {
using CommonStubCSigns = kungfu::CommonStubCSigns;
using BytecodeStubCSigns = kungfu::BytecodeStubCSigns;

void AOTFileManager::Iterate(const RootVisitor &v)
{
    for (auto &iter : desCPs_) {
        for (auto &curCP : iter.second) {
            v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&iter.second.at(curCP.first))));
        }
    }
}

void AOTFileManager::DumpAOTInfo()
{
    AnFileDataManager *m = AnFileDataManager::GetInstance();
    m->Dump();
}

void AOTFileManager::LoadStubFile(const std::string &fileName)
{
    AnFileDataManager *anFileDataManager = AnFileDataManager::GetInstance();
    if (!anFileDataManager->SafeLoad(fileName, AnFileDataManager::Type::STUB)) {
        return;
    }
    auto info = anFileDataManager->SafeGetStubFileInfo();
    auto stubs = info->GetStubs();
    InitializeStubEntries(stubs);
}

bool AOTFileManager::LoadAnFile(const std::string &fileName)
{
    AnFileDataManager *anFileDataManager = AnFileDataManager::GetInstance();
    return anFileDataManager->SafeLoad(fileName, AnFileDataManager::Type::AOT);
}

bool AOTFileManager::LoadAiFile([[maybe_unused]] const std::string &filename)
{
    Snapshot snapshot(vm_);
#if !WIN_OR_MAC_OR_IOS_PLATFORM
    return snapshot.Deserialize(SnapshotType::AI, filename.c_str());
#else
    return true;
#endif
}

bool AOTFileManager::LoadAiFile(const JSPandaFile *jsPandaFile)
{
    uint32_t anFileInfoIndex = GetAnFileIndex(jsPandaFile);
    // this abc file does not have corresponding an file
    if (anFileInfoIndex == INVALID_INDEX) {
        return false;
    }

    auto iter = desCPs_.find(anFileInfoIndex);
    // already loaded
    if (iter != desCPs_.end()) {
        return false;
    }

    AnFileDataManager *anFileDataManager = AnFileDataManager::GetInstance();
    std::string aiFilename = anFileDataManager->GetDir();
    aiFilename += JSFilePath::GetHapName(jsPandaFile) + AOTFileManager::FILE_EXTENSION_AI;
    LoadAiFile(aiFilename);
    return true;
}

const std::shared_ptr<AnFileInfo> AOTFileManager::GetAnFileInfo(const JSPandaFile *jsPandaFile) const
{
    uint32_t index = GetAnFileIndex(jsPandaFile);
    if (index == INVALID_INDEX) {
        return nullptr;
    }
    AnFileDataManager *anFileDataManager = AnFileDataManager::GetInstance();
    return anFileDataManager->SafeGetAnFileInfo(index);
}

bool AOTFileManager::IsLoad(const JSPandaFile *jsPandaFile) const
{
    if (!AnFileDataManager::GetInstance()->IsEnable()) {
        return false;
    }

    const std::shared_ptr<AnFileInfo> anFileInfo = GetAnFileInfo(jsPandaFile);
    if (anFileInfo == nullptr) {
        return false;
    }
    return anFileInfo->IsLoad();
}

bool AOTFileManager::IsLoadMain(const JSPandaFile *jsPandaFile, const CString &entry) const
{
    if (!jsPandaFile->IsLoadedAOT()) {
        return false;
    }

    const std::shared_ptr<AnFileInfo> anFileInfo = GetAnFileInfo(jsPandaFile);
    if (anFileInfo == nullptr) {
        return false;
    }

    return anFileInfo->IsLoadMain(jsPandaFile, entry);
}

uint32_t AOTFileManager::GetAnFileIndex(const JSPandaFile *jsPandaFile) const
{
    AnFileDataManager *anFileDataManager = AnFileDataManager::GetInstance();

    // run via command line
    if (vm_->GetJSOptions().WasAOTOutputFileSet()) {
        std::string jsPandaFileDesc = jsPandaFile->GetJSPandaFileDesc().c_str();
        std::string baseName = JSFilePath::GetFileName(jsPandaFileDesc);
        if (baseName.empty()) {
            return INVALID_INDEX;
        }
        std::string anFileName = baseName + FILE_EXTENSION_AN;
        return anFileDataManager->SafeGetFileInfoIndex(anFileName);
    }

    // run from app hap
    std::string hapName = JSFilePath::GetHapName(jsPandaFile);
    if (hapName.empty()) {
        return INVALID_INDEX;
    }
    std::string anFileName = hapName + FILE_EXTENSION_AN;
    return anFileDataManager->SafeGetFileInfoIndex(anFileName);
}

bool AOTFileManager::TryReadLock()
{
    AnFileDataManager *anFileDataManager = AnFileDataManager::GetInstance();
    return anFileDataManager->SafeTryReadLock();
}

bool AOTFileManager::InsideStub(uintptr_t pc)
{
    AnFileDataManager *anFileDataManager = AnFileDataManager::GetInstance();
    return anFileDataManager->SafeInsideStub(pc);
}

bool AOTFileManager::InsideAOT(uintptr_t pc)
{
    AnFileDataManager *anFileDataManager = AnFileDataManager::GetInstance();
    return anFileDataManager->SafeInsideAOT(pc);
}

AOTFileInfo::CallSiteInfo AOTFileManager::CalCallSiteInfo(uintptr_t retAddr)
{
    AnFileDataManager *anFileDataManager = AnFileDataManager::GetInstance();
    return anFileDataManager->SafeCalCallSiteInfo(retAddr);
}

void AOTFileManager::PrintAOTEntry(const JSPandaFile *file, const Method *method, uintptr_t entry)
{
    uint32_t mId = method->GetMethodId().GetOffset();
    std::string mName = method->GetMethodName(file);
    std::string fileName = file->GetFileName();
    LOG_COMPILER(INFO) << "Bind " << mName << "@" << mId << "@" << fileName
                       << " -> AOT-Entry = " << reinterpret_cast<void *>(entry);
}

void AOTFileManager::SetAOTMainFuncEntry(JSHandle<JSFunction> mainFunc, const JSPandaFile *jsPandaFile,
                                         std::string_view entryPoint)
{
    AnFileDataManager *anFileDataManager = AnFileDataManager::GetInstance();
    uint32_t anFileInfoIndex = jsPandaFile->GetAOTFileInfoIndex();
    const std::shared_ptr<AnFileInfo> anFileInfo = anFileDataManager->SafeGetAnFileInfo(anFileInfoIndex);
    // get main func method
    auto mainFuncMethodId = jsPandaFile->GetMainMethodIndex(entryPoint.data());
    uint64_t mainEntry;
    bool isFastCall;
    std::tie(mainEntry, isFastCall) = anFileInfo->GetMainFuncEntry(mainFuncMethodId);
    MethodLiteral *mainMethod = jsPandaFile->FindMethodLiteral(mainFuncMethodId);
    mainMethod->SetAotCodeBit(true);
    mainMethod->SetNativeBit(false);
    Method *method = mainFunc->GetCallTarget();
    method->SetDeoptThreshold(vm_->GetJSOptions().GetDeoptThreshold());
    method->SetCodeEntryAndMarkAOT(static_cast<uintptr_t>(mainEntry));
    method->SetIsFastCall(isFastCall);
#ifndef NDEBUG
    PrintAOTEntry(jsPandaFile, method, mainEntry);
#endif

    MethodLiteral *methodLiteral = method->GetMethodLiteral();
    methodLiteral->SetAotCodeBit(true);
    methodLiteral->SetIsFastCall(isFastCall);
}

void AOTFileManager::SetAOTFuncEntry(const JSPandaFile *jsPandaFile, Method *method,
                                     uint32_t entryIndex, bool *canFastCall)
{
    AnFileDataManager *anFileDataManager = AnFileDataManager::GetInstance();
    uint32_t anFileInfoIndex = jsPandaFile->GetAOTFileInfoIndex();
    const std::shared_ptr<AnFileInfo> anFileInfo = anFileDataManager->SafeGetAnFileInfo(anFileInfoIndex);
    const AOTFileInfo::FuncEntryDes &entry = anFileInfo->GetStubDes(entryIndex);
    uint64_t codeEntry = entry.codeAddr_;
#ifndef NDEBUG
    PrintAOTEntry(jsPandaFile, method, codeEntry);
#endif
    if (!codeEntry) {
        return;
    }
    method->SetDeoptThreshold(vm_->GetJSOptions().GetDeoptThreshold());
    method->SetCodeEntryAndMarkAOT(codeEntry);
    method->SetIsFastCall(entry.isFastCall_);
    if (canFastCall != nullptr) {
        *canFastCall = entry.isFastCall_;
    }

    MethodLiteral *methodLiteral = method->GetMethodLiteral();
    methodLiteral->SetAotCodeBit(true);
    methodLiteral->SetIsFastCall(entry.isFastCall_);
}

kungfu::ArkStackMapParser *AOTFileManager::GetStackMapParser() const
{
    return arkStackMapParser_;
}

void AOTFileManager::AdjustBCStubAndDebuggerStubEntries(JSThread *thread,
                                                        const std::vector<AOTFileInfo::FuncEntryDes> &stubs,
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

void AOTFileManager::InitializeStubEntries(const std::vector<AnFileInfo::FuncEntryDes> &stubs)
{
    auto thread = vm_->GetAssociatedJSThread();
    size_t len = stubs.size();
    for (size_t i = 0; i < len; i++) {
        auto des = stubs[i];
        if (des.IsCommonStub()) {
            thread->SetFastStubEntry(des.indexInKindOrMethodId_, des.codeAddr_);
        } else if (des.IsBCStub()) {
            thread->SetBCStubEntry(des.indexInKindOrMethodId_, des.codeAddr_);
#if ECMASCRIPT_ENABLE_ASM_FILE_LOAD_LOG
            auto start = GET_MESSAGE_STRING_ID(HandleLdundefined);
            std::string format = MessageString::GetMessageString(des.indexInKindOrMethodId_ + start);
            LOG_ECMA(DEBUG) << "bytecode index: " << des.indexInKindOrMethodId_ << " :" << format << " addr: 0x"
                            << std::hex << des.codeAddr_;
#endif
        } else if (des.IsBuiltinsStub()) {
            thread->SetBuiltinStubEntry(des.indexInKindOrMethodId_, des.codeAddr_);
#if ECMASCRIPT_ENABLE_ASM_FILE_LOAD_LOG
            int start = GET_MESSAGE_STRING_ID(CharCodeAt);
            std::string format = MessageString::GetMessageString(des.indexInKindOrMethodId_ + start - 1);  // -1: NONE
            LOG_ECMA(DEBUG) << "builtins index: " << std::dec << des.indexInKindOrMethodId_ << " :" << format
                            << " addr: 0x" << std::hex << des.codeAddr_;
#endif
        } else {
            thread->RegisterRTInterface(des.indexInKindOrMethodId_, des.codeAddr_);
#if ECMASCRIPT_ENABLE_ASM_FILE_LOAD_LOG
            int start = GET_MESSAGE_STRING_ID(CallRuntime);
            std::string format = MessageString::GetMessageString(des.indexInKindOrMethodId_ + start);
            LOG_ECMA(DEBUG) << "runtime index: " << std::dec << des.indexInKindOrMethodId_ << " :" << format
                            << " addr: 0x" << std::hex << des.codeAddr_;
#endif
        }
    }
    thread->CheckOrSwitchPGOStubs();
    AsmInterParsedOption asmInterOpt = vm_->GetJSOptions().GetAsmInterParsedOption();
    AdjustBCStubAndDebuggerStubEntries(thread, stubs, asmInterOpt);
}

bool AOTFileManager::RewriteDataSection(uintptr_t dataSec, size_t size, uintptr_t newData, size_t newSize)
{
    if (memcpy_s(reinterpret_cast<void *>(dataSec), size, reinterpret_cast<void *>(newData), newSize) != EOK) {
        LOG_FULL(FATAL) << "memset failed";
        return false;
    }
    return true;
}

void AOTFileManager::AddConstantPool(const CString &snapshotFileName, JSTaggedValue deserializedCPList)
{
    AnFileDataManager *anFileDataManager = AnFileDataManager::GetInstance();
    std::string baseName = JSFilePath::GetFileName(snapshotFileName.c_str());
    uint32_t anFileInfoIndex = anFileDataManager->SafeGetFileInfoIndex(baseName + FILE_EXTENSION_AN);

    desCPs_.insert({anFileInfoIndex, CMap<int32_t, JSTaggedValue> {}});
    CMap<int32_t, JSTaggedValue> &cpMap = desCPs_[anFileInfoIndex];

    JSHandle<TaggedArray> cpList(vm_->GetJSThread(), deserializedCPList);
    uint32_t len = cpList->GetLength();
    for (uint32_t pos = 0; pos < len; pos += DESERI_CP_ITEM_SIZE) {
        int32_t constantPoolID = cpList->Get(pos).GetInt();
        JSTaggedValue cp = cpList->Get(pos + 1);
        cpMap.insert({constantPoolID, cp});
    }
}

JSHandle<JSTaggedValue> AOTFileManager::GetDeserializedConstantPool(const JSPandaFile *jsPandaFile, int32_t cpID)
{
    // The deserialization of the 'ai' data used by the multi-work
    // is not implemented yet, so there may be a case where
    // desCPs_ is empty, in which case the Hole will be returned
    if (desCPs_.empty()) {
        return JSHandle<JSTaggedValue>(vm_->GetJSThread(), JSTaggedValue::Hole());
    }
    uint32_t anFileInfoIndex = jsPandaFile->GetAOTFileInfoIndex();
    CMap<int32_t, JSTaggedValue> &cpMap = desCPs_.at(anFileInfoIndex);
    auto iter = cpMap.find(cpID);
    if (iter == cpMap.end()) {
        LOG_COMPILER(FATAL) << "can not find deserialized constantpool in anFileInfo, constantPoolID is " << cpID;
        UNREACHABLE();
    }
    return JSHandle<JSTaggedValue>(uintptr_t(&iter->second));
}

AOTFileManager::~AOTFileManager()
{
    if (arkStackMapParser_ != nullptr) {
        delete arkStackMapParser_;
        arkStackMapParser_ = nullptr;
    }
}

AOTFileManager::AOTFileManager(EcmaVM *vm) : vm_(vm), factory_(vm->GetFactory())
{
    bool enableLog = vm->GetJSOptions().WasSetCompilerLogOption();
    arkStackMapParser_ = new kungfu::ArkStackMapParser(enableLog);
}

JSTaggedValue AOTFileManager::GetAbsolutePath(JSThread *thread, JSTaggedValue relativePathVal)
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

bool AOTFileManager::GetAbsolutePath(const CString &relativePathCstr, CString &absPathCstr)
{
    std::string relativePath = ConvertToStdString(relativePathCstr);
    std::string absPath;
    if (RealPath(relativePath, absPath)) {
        absPathCstr = ConvertToString(absPath);
        return true;
    }
    return false;
}

const Heap *AOTFileManager::GetHeap()
{
    if (vm_ == nullptr) {
        return nullptr;
    }
    return vm_->GetHeap();
}
}  // namespace panda::ecmascript
