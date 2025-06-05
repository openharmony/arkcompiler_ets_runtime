/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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


#include "ecmascript/compiler/aot_snapshot/aot_snapshot_constants.h"
#include "ecmascript/js_file_path.h"
#include "ecmascript/message_string.h"
#include "ecmascript/ohos/framework_helper.h"
#include "ecmascript/ohos/ohos_preload_app_info.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/stackmap/ark_stackmap_parser.h"

namespace panda::ecmascript {
using CommonStubCSigns = kungfu::CommonStubCSigns;
using BytecodeStubCSigns = kungfu::BytecodeStubCSigns;
using SnapshotGlobalData = kungfu::SnapshotGlobalData;

JsAotReaderCallback AOTFileManager::jsAotReader_ = nullptr;

void AOTLiteralInfo::InitializeWithSpecialValue(JSTaggedValue initValue, uint32_t capacity, uint32_t extraLength)
{
    TaggedArray::InitializeWithSpecialValue(initValue, capacity + RESERVED_LENGTH, extraLength);
    SetIhc(JSTaggedValue::Undefined());
    SetChc(JSTaggedValue::Undefined());
    SetLiteralType(JSTaggedValue(INVALID_LITERAL_TYPE));
}

void AOTLiteralInfo::SetObjectToCache(JSThread *thread, uint32_t index, JSTaggedValue value)
{
    Set(thread, index, value);
}

void AOTFileManager::Iterate(RootVisitor &v)
{
    for (auto &iter : aiDatum_) {
        auto &aiData = iter.second;
        for (auto &eachFileData : aiData) {
            auto &cpMap = eachFileData.second.multiCpsMap_;
            for (auto &eachCpPair : cpMap) {
                v.VisitRoot(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&eachCpPair.second)));
            }
        }
    }
}

void AOTFileManager::SetJsAotReader(JsAotReaderCallback cb)
{
    jsAotReader_ = cb;
}

JsAotReaderCallback AOTFileManager::GetJsAotReader()
{
    return jsAotReader_;
}

bool AOTFileManager::AOTFileExist(const std::string &aotFileName, const std::string &extension)
{
    std::string realPath;
    std::string filename = aotFileName + extension;
    if (!RealPath(filename, realPath, false)) {
        return false;
    }
    return FileExist(realPath.c_str());
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
#if defined(CROSS_PLATFORM) && defined(ANDROID_PLATFORM)
    return anFileDataManager->SafeLoad(fileName, AnFileDataManager::Type::AOT, GetJsAotReader());
#else
    return anFileDataManager->SafeLoad(fileName, AnFileDataManager::Type::AOT);
#endif
}

bool AOTFileManager::LoadAiFile([[maybe_unused]] const std::string &filename)
{
    Snapshot snapshot(vm_);
#if !WIN_OR_MAC_OR_IOS_PLATFORM
    #if defined(CROSS_PLATFORM) && defined(ANDROID_PLATFORM)
        return snapshot.Deserialize(SnapshotType::AI, filename.c_str(), GetJsAotReader());
    #else
        return snapshot.Deserialize(SnapshotType::AI, filename.c_str());
    #endif
#else
    return true;
#endif
}

bool AOTFileManager::LoadAiFile(const JSPandaFile *jsPandaFile, EcmaVM *vm)
{
    uint32_t anFileInfoIndex = jsPandaFile->GetAOTFileInfoIndex();
    // this abc file does not have corresponding an file
    if (anFileInfoIndex == INVALID_INDEX) {
        return false;
    }

    auto iter = aiDatum_.find(anFileInfoIndex);
    // already loaded
    if (iter != aiDatum_.end()) {
        return false;
    }

    std::string aiFilename = "";
    AnFileDataManager *anFileDataManager = AnFileDataManager::GetInstance();
    // device side aot compile success
    if (anFileDataManager->IsEnable()) {
        std::string moduleName = anFileDataManager->SafeGetAnFileNameNoSuffix(anFileInfoIndex);
        aiFilename = GetAOTFileFullPath(vm, moduleName);
    } else {
        std::string moduleName = JSFilePath::GetHapName(jsPandaFile);
        std::string hapPath = jsPandaFile->GetJSPandaFileHapPath().c_str();
        aiFilename = OhosPreloadAppInfo::GetPreloadAOTFileName(hapPath, moduleName);
    }

    if (aiFilename.empty()) {
        LOG_ECMA(INFO) << "current thread can not find ai file";
        return false;
    }
    aiFilename += FILE_EXTENSION_AI;
    LoadAiFile(aiFilename);
    return true;
}

// static
std::string AOTFileManager::GetAOTFileFullPath(EcmaVM *vm, const std::string &moduleName)
{
    if (vm->GetJSOptions().WasAOTOutputFileSet()) {
        return vm->GetJSOptions().GetAOTOutputFile();
    }
#if defined(CROSS_PLATFORM) && defined(ANDROID_PLATFORM)
    else if (vm->GetJSOptions().GetEnableAOT())
#else
    else if (ecmascript::AnFileDataManager::GetInstance()->IsEnable())
#endif
    {
        return ecmascript::AnFileDataManager::GetInstance()->GetDir() + moduleName;
    } else {
        std::string hapPath = "";
        ecmascript::SearchHapPathCallBack callback = vm->GetSearchHapPathCallBack();
        if (callback) {
            callback(moduleName, hapPath);
        }
        return ecmascript::OhosPreloadAppInfo::GetPreloadAOTFileName(hapPath, moduleName);
    }
}

const std::shared_ptr<AnFileInfo> AOTFileManager::GetAnFileInfo(const JSPandaFile *jsPandaFile) const
{
    uint32_t index = jsPandaFile->GetAOTFileInfoIndex();
    if (index == INVALID_INDEX) {
        return nullptr;
    }
    AnFileDataManager *anFileDataManager = AnFileDataManager::GetInstance();
    return anFileDataManager->SafeGetAnFileInfo(index);
}

uint32_t AOTFileManager::GetFileIndex(uint32_t anFileInfoIndex, CString abcNormalizedName) const
{
    auto fileIndex = INVALID_INDEX;
    if (abcNormalizedName.find(JSFilePath::GetBaseName(STUB_AN_FILE)) == std::string::npos) {
        auto aiDatumIter = aiDatum_.find(anFileInfoIndex);
        if (aiDatumIter == aiDatum_.end()) {
            return INVALID_INDEX;
        }

        auto fileIter = aiDatumIter->second.find(abcNormalizedName);
        if (fileIter == aiDatumIter->second.end()) {
            return INVALID_INDEX;
        }
        fileIndex = fileIter->second.fileIndex_;
    } else {
        fileIndex = STUB_FILE_INDEX;
    }
    return fileIndex;
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

    auto fileIndex = GetFileIndex(jsPandaFile->GetAOTFileInfoIndex(), jsPandaFile->GetNormalizedFileDesc().c_str());
    if (fileIndex == INVALID_INDEX) {
        return false;
    }
    return anFileInfo->IsLoadMain(fileIndex, jsPandaFile, entry);
}

std::list<CString> AOTFileManager::GetPandaFiles(uint32_t aotFileInfoIndex)
{
    std::list<CString> abcFilesList {};
    auto aiDatumIter = aiDatum_.find(aotFileInfoIndex);
    if (aiDatumIter == aiDatum_.end()) {
        return abcFilesList;
    }
    for (const auto& nameIter : aiDatumIter->second) {
        abcFilesList.push_back(nameIter.first);
    }
    return abcFilesList;
}

void AOTFileManager::BindPreloadedPandaFilesInAotFile(const std::string &moduleName)
{
    AnFileDataManager *anFileDataManager = AnFileDataManager::GetInstance();
    uint32_t aotFileInfoIndex = anFileDataManager->SafeGetFileInfoIndex(moduleName + FILE_EXTENSION_AN);
    if (aotFileInfoIndex == INVALID_INDEX) {
        return;
    }
    auto abcFiles = GetPandaFiles(aotFileInfoIndex);
    for (const auto &abcNormalizedName : abcFiles) {
        const auto abcFile = JSPandaFileManager::GetInstance()->FindJSPandaFileByNormalizedName(abcNormalizedName);
        if (!abcFile) {
            LOG_ECMA(WARN) << "Can not find file: " << abcNormalizedName << " in module: " << moduleName;
            continue;
        }
        if (!abcFile->IsLoadedAOT()) {
            if (!anFileDataManager->SafeCheckFilenameToChecksum(abcNormalizedName, abcFile->GetChecksum())) {
                LOG_ECMA(ERROR) << "BindPreloadedPandaFilesInAotFile failed because of different checksum: "
                                << abcNormalizedName;
                continue;
            }
            abcFile->SetAOTFileInfoIndex(aotFileInfoIndex);
            LOG_ECMA(INFO) << "Bind file: " << abcNormalizedName << ", aotFileInfoIndex: " << aotFileInfoIndex
                           << " in module: " << moduleName;
        }
    }
}

bool AOTFileManager::HasPandaFile(uint32_t aotFileInfoIndex, const CString &abcNormalizedName) const
{
    auto aiDatumIter = aiDatum_.find(aotFileInfoIndex);
    if (aiDatumIter == aiDatum_.end()) {
        return false;
    }
    auto pandaCPIter = aiDatumIter->second.find(abcNormalizedName);
    return pandaCPIter != aiDatumIter->second.end();
}

void AOTFileManager::BindPandaFileInAotFile(const std::string &aotFileBaseName, JSPandaFile *jsPandaFile) const
{
    if (jsPandaFile->IsLoadedAOT()) {
        // already loaded.
        return;
    }

    AnFileDataManager *anFileDataManager = AnFileDataManager::GetInstance();
    if (!anFileDataManager->IsEnable()) {
        return;
    }
    uint32_t aotFileInfoIndex = anFileDataManager->SafeGetFileInfoIndex(aotFileBaseName + FILE_EXTENSION_AN);
    if (aotFileInfoIndex == INVALID_INDEX) {
        LOG_ECMA(WARN) << "Bind panda file to AOT failed. AOT file not found for " << aotFileBaseName;
        return;
    }
    CString abcNormalizedName(jsPandaFile->GetNormalizedFileDesc());
    if (!HasPandaFile(aotFileInfoIndex, abcNormalizedName)) {
        // not existed in an file.
        LOG_ECMA(WARN) << "Bind panda file to AOT failed. " << abcNormalizedName << " not found for "
                       << aotFileBaseName;
        return;
    }
    if (!anFileDataManager->SafeCheckFilenameToChecksum(abcNormalizedName, jsPandaFile->GetChecksum())) {
        LOG_ECMA(ERROR) << "checksum is different,BindPandaFileInAotFile failed " << abcNormalizedName;
        return;
    }
    jsPandaFile->SetAOTFileInfoIndex(aotFileInfoIndex);
    LOG_ECMA(INFO) << "Bind file: " << abcNormalizedName << ", aotFileInfoIndex: " << aotFileInfoIndex
                   << " in aotFileBaseName: " << aotFileBaseName;
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

bool AOTFileManager::IsEnableAOT() const
{
    AnFileDataManager *anFileDataManager = AnFileDataManager::GetInstance();
    return anFileDataManager->IsEnable();
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

AOTFileInfo::CallSiteInfo AOTFileManager::CalCallSiteInfo(uintptr_t retAddr, bool isDeopt)
{
    AnFileDataManager *anFileDataManager = AnFileDataManager::GetInstance();
    return anFileDataManager->SafeCalCallSiteInfo(retAddr, isDeopt);
}

void AOTFileManager::PrintAOTEntry(const JSPandaFile *file, const Method *method, uintptr_t entry)
{
    uint32_t mId = method->GetMethodId().GetOffset();
    std::string mName = method->GetMethodName(file);
    auto &fileName = file->GetJSPandaFileDesc();
    LOG_COMPILER(INFO) << "Bind " << mName << "@" << mId << "@" << fileName
                       << " -> AOT-Entry = " << reinterpret_cast<void *>(entry);
}

void AOTFileManager::SetAOTMainFuncEntry(JSHandle<JSFunction> mainFunc, const JSPandaFile *jsPandaFile,
                                         std::string_view entryPoint)
{
    AnFileDataManager *anFileDataManager = AnFileDataManager::GetInstance();
    uint32_t anFileInfoIndex = jsPandaFile->GetAOTFileInfoIndex();
    const std::shared_ptr<AnFileInfo> anFileInfo = anFileDataManager->SafeGetAnFileInfo(anFileInfoIndex);
    auto aiDatumIter = aiDatum_.find(anFileInfoIndex);
    if (aiDatumIter == aiDatum_.end()) {
        LOG_ECMA(FATAL) << "can not find aiData by anFileInfoIndex " << anFileInfoIndex;
        UNREACHABLE();
    }
    uint32_t fileIndex = GetFileIndex(jsPandaFile->GetAOTFileInfoIndex(), jsPandaFile->GetNormalizedFileDesc().c_str());
    if (fileIndex == INVALID_INDEX) {
        LOG_ECMA(FATAL) << "can not find aiData by anFileInfoIndex " << anFileInfoIndex
                        << ", normalizedDesc: " << jsPandaFile->GetNormalizedFileDesc();
        UNREACHABLE();
    }
    // get main func method
    auto mainFuncMethodId = jsPandaFile->GetMainMethodIndex(entryPoint.data());
    MainFuncEntry mainFuncEntry = anFileInfo->GetMainFuncEntry(fileIndex, mainFuncMethodId);
    uint64_t mainEntry = mainFuncEntry.mainEntry;
    int32_t fpDelta = mainFuncEntry.fpDelta;
    bool isFastCall = mainFuncEntry.isFastCall;
    MethodLiteral *mainMethod = jsPandaFile->FindMethodLiteral(mainFuncMethodId);
    if (mainMethod == nullptr) {
        LOG_ECMA(FATAL) << "empty main method literal";
        UNREACHABLE();
    }
    mainMethod->SetAotCodeBit(true);
    mainMethod->SetNativeBit(false);
    Method *method = mainFunc->GetCallTarget();
    method->SetDeoptThreshold(vm_->GetJSOptions().GetDeoptThreshold());
    method->SetCodeEntryAndMarkAOTWhenBinding(static_cast<uintptr_t>(mainEntry));
    method->SetFpDelta(fpDelta);
    method->SetIsFastCall(isFastCall);
    mainFunc->SetCompiledFuncEntry(static_cast<uintptr_t>(mainEntry), isFastCall);
#ifndef NDEBUG
    PrintAOTEntry(jsPandaFile, method, mainEntry);
#endif

    MethodLiteral *methodLiteral = method->GetMethodLiteral();
    ASSERT(methodLiteral != nullptr);
    methodLiteral->SetAotCodeBit(true);
    methodLiteral->SetIsFastCall(isFastCall);
}

void AOTFileManager::SetAOTFuncEntry(const JSPandaFile *jsPandaFile, JSFunction *function,
                                     Method *method, uint32_t entryIndex, bool *canFastCall)
{
    uint64_t methodCodeEntry = method->GetCodeEntryOrLiteral();
    if (function != nullptr && methodCodeEntry != reinterpret_cast<uintptr_t>(nullptr)) {
        function->SetCompiledFuncEntry(methodCodeEntry, method->IsFastCall());
        return;
    }
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
    method->SetCodeEntryAndMarkAOTWhenBinding(codeEntry);
    method->SetIsFastCall(entry.isFastCall_);
    method->SetFpDelta(entry.fpDeltaPrevFrameSp_);
    if (canFastCall != nullptr) {
        *canFastCall = entry.isFastCall_;
    }

    MethodLiteral *methodLiteral = method->GetMethodLiteral();
    ASSERT(methodLiteral != nullptr);
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
    ASSERT(defaultBCStubDes.kind_ == kungfu::CallSignature::TargetKind::BYTECODE_HELPER_HANDLER);
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

void AOTFileManager::LoadingCommonStubsLog(size_t id, Address entry)
{
    auto start = GET_MESSAGE_STRING_ID(Add);
    std::string format = MessageString::GetMessageString(id + start);
    LOG_ECMA(INFO) << "common index: " << id << " :" << format<< " addr: 0x"
                    << std::hex <<entry;
}

void AOTFileManager::LoadingByteCodeStubsLog(size_t id, Address entry)
{
    auto start = GET_MESSAGE_STRING_ID(HandleLdundefined);
    std::string format = MessageString::GetMessageString(id + start);
    LOG_ECMA(INFO) << "bytecode index: " << id << " :" << format << " addr: 0x"
                    << std::hex << entry;
}

void AOTFileManager::LoadingBuiltinsStubsLog(size_t id, Address entry)
{
    int start = GET_MESSAGE_STRING_ID(StringCharCodeAt);
    std::string format = MessageString::GetMessageString(id + start - 1);  // -1: NONE
    LOG_ECMA(INFO) << "builtins index: " << std::dec << id << " :" << format
                    << " addr: 0x" << std::hex << entry;
}

void AOTFileManager::LoadingBaselineStubsLog(size_t id, Address entry)
{
    int start = GET_MESSAGE_STRING_ID(BaselineTryLdGLobalByNameImm8ID16);
    std::string format = MessageString::GetMessageString(id + start - 1);  // -1: NONE
    LOG_ECMA(INFO) << "baseline stub index: " << std::dec << id << " :" << format
                    << " addr: 0x" << std::hex << entry;
}

void AOTFileManager::LoadingRuntimeStubsLog(size_t id, Address entry)
{
    int start = GET_MESSAGE_STRING_ID(CallRuntime);
    std::string format = MessageString::GetMessageString(id + start);
    LOG_ECMA(INFO) << "runtime index: " << std::dec << id << " :" << format
                    << " addr: 0x" << std::hex << entry;
}

void AOTFileManager::InitializeStubEntries(const std::vector<AnFileInfo::FuncEntryDes> &stubs)
{
    auto thread = vm_->GetAssociatedJSThread();
    size_t len = stubs.size();
    for (size_t i = 0; i < len; i++) {
        auto des = stubs[i];
        if (des.IsCommonStub()) {
            thread->SetFastStubEntry(des.indexInKindOrMethodId_, des.codeAddr_);
            if (vm_->GetJSOptions().EnableLoadingStubsLog()) {
                LoadingCommonStubsLog(des.indexInKindOrMethodId_, des.codeAddr_);
            }
        } else if (des.IsBCStub()) {
            thread->SetBCStubEntry(des.indexInKindOrMethodId_, des.codeAddr_);
            if (vm_->GetJSOptions().EnableLoadingStubsLog()) {
                LoadingByteCodeStubsLog(des.indexInKindOrMethodId_, des.codeAddr_);
            }
        } else if (des.IsBuiltinsStub()) {
            thread->SetBuiltinStubEntry(des.indexInKindOrMethodId_, des.codeAddr_);
            if (vm_->GetJSOptions().EnableLoadingStubsLog()) {
                LoadingBuiltinsStubsLog(des.indexInKindOrMethodId_, des.codeAddr_);
            }
        } else if (des.IsBaselineStub()) {
            thread->SetBaselineStubEntry(des.indexInKindOrMethodId_, des.codeAddr_);
            if (vm_->GetJSOptions().EnableLoadingStubsLog()) {
                LoadingBaselineStubsLog(des.indexInKindOrMethodId_, des.codeAddr_);
            }
        } else {
            thread->RegisterRTInterface(des.indexInKindOrMethodId_, des.codeAddr_);
            if (vm_->GetJSOptions().EnableLoadingStubsLog()) {
                LoadingRuntimeStubsLog(des.indexInKindOrMethodId_, des.codeAddr_);
            }
        }
    }
    thread->CheckOrSwitchPGOStubs();
    if (!g_isEnableCMCGC) {
        thread->SwitchStwCopyStubs(true);
    }
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

void AOTFileManager::ParseDeserializedData(const CString &snapshotFileName, JSTaggedValue deserializedData)
{
    AnFileDataManager *anFileDataManager = AnFileDataManager::GetInstance();
    std::string baseName = JSFilePath::GetFileName(snapshotFileName.c_str());
    uint32_t anFileInfoIndex = anFileDataManager->SafeGetFileInfoIndex(baseName + FILE_EXTENSION_AN);

    JSThread *thread = vm_->GetJSThread();
    FrameworkHelper frameworkHelper(thread);
    JSHandle<TaggedArray> aiData(thread, deserializedData);
    uint32_t aiDataLen = aiData->GetLength();
    ASSERT(aiDataLen % AOTSnapshotConstants::SNAPSHOT_DATA_ITEM_SIZE  == 0);
    auto aiDatumResult = aiDatum_.try_emplace(anFileInfoIndex);
    FileNameToMultiConstantPoolMap &fileNameToMulCpMap = aiDatumResult.first->second;

    JSMutableHandle<TaggedArray> fileInfo(thread, JSTaggedValue::Undefined());
    JSMutableHandle<TaggedArray> cpList(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < aiDataLen; i += AOTSnapshotConstants::SNAPSHOT_DATA_ITEM_SIZE) {
        // handle file info
        fileInfo.Update(aiData->Get(i + SnapshotGlobalData::Cast(SnapshotGlobalData::CP_TOP_ITEM::PANDA_INFO_ID)));
        auto nameOffset = SnapshotGlobalData::Cast(SnapshotGlobalData::CP_PANDA_INFO_ITEM::NAME_ID);
        auto indexOffset = SnapshotGlobalData::Cast(SnapshotGlobalData::CP_PANDA_INFO_ITEM::INDEX_ID);
        CString fileNameCStr = EcmaStringAccessor(fileInfo->Get(nameOffset)).ToCString();
        std::string fileNameStr = EcmaStringAccessor(fileInfo->Get(nameOffset)).ToStdString();
        uint32_t fileIndex = static_cast<uint32_t>(fileInfo->Get(indexOffset).GetInt());
        // handle constant pool
        cpList.Update(aiData->Get(i + SnapshotGlobalData::Cast(SnapshotGlobalData::CP_TOP_ITEM::CP_ARRAY_ID)));
        uint32_t cpLen = cpList->GetLength();
        ASSERT(cpLen % AOTSnapshotConstants::SNAPSHOT_CP_ARRAY_ITEM_SIZE == 0);
        auto &PandaCpInfoInserted = fileNameToMulCpMap.try_emplace(fileNameCStr).first->second;
        PandaCpInfoInserted.fileIndex_ = fileIndex;
        MultiConstantPoolMap& cpMap = PandaCpInfoInserted.multiCpsMap_;
        if (cpLen > 0) {
            JSTaggedValue cp = cpList->Get(AOTSnapshotConstants::SNAPSHOT_CP_ARRAY_ITEM_SIZE - 1);  // first constpool
            vm_->LoadProtoTransitionTable(cp);
        }
        JSMutableHandle<ConstantPool> cpHandle(thread, JSTaggedValue::Undefined());
        for (uint32_t pos = 0; pos < cpLen; pos += AOTSnapshotConstants::SNAPSHOT_CP_ARRAY_ITEM_SIZE) {
            int32_t constantPoolID = cpList->Get(pos).GetInt();
            cpHandle.Update(cpList->Get(pos + 1));
            vm_->ResetProtoTransitionTableOnConstpool(cpHandle.GetTaggedValue());
            cpMap.insert({constantPoolID, cpHandle.GetTaggedValue()});
            // the arkui framework abc file constpool was patched here
            if (frameworkHelper.IsFrameworkAbcFile(fileNameStr)) {
                vm_->UpdateConstpoolWhenDeserialAI(fileNameStr, cpHandle, constantPoolID);
            }
        }
    }
}

JSHandle<JSTaggedValue> AOTFileManager::GetDeserializedConstantPool(const JSPandaFile *jsPandaFile, int32_t cpID)
{
    // The deserialization of the 'ai' data used by the multi-work
    // is not implemented yet, so there may be a case where
    // aiDatum_ is empty, in which case the Hole will be returned
    if (aiDatum_.empty()) {
        return JSHandle<JSTaggedValue>(vm_->GetJSThread(), JSTaggedValue::Hole());
    }
    uint32_t anFileInfoIndex = jsPandaFile->GetAOTFileInfoIndex();
    auto aiDatumIter = aiDatum_.find(anFileInfoIndex);
    if (aiDatumIter == aiDatum_.end()) {
        LOG_COMPILER(FATAL) << "can not find aiData by anFileInfoIndex " << anFileInfoIndex;
        UNREACHABLE();
    }
    const auto &fileNameToMulCpMap = aiDatumIter->second;
    auto cpMapIter = fileNameToMulCpMap.find(jsPandaFile->GetNormalizedFileDesc());
    if (cpMapIter == fileNameToMulCpMap.end()) {
        LOG_COMPILER(FATAL) << "can not find constpools by fileName " << jsPandaFile->GetNormalizedFileDesc().c_str();
        UNREACHABLE();
    }
    const CMap<int32_t, JSTaggedValue> &cpMap = cpMapIter->second.multiCpsMap_;
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
