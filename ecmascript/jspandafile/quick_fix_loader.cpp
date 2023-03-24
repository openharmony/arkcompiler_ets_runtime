/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "ecmascript/jspandafile/quick_fix_loader.h"

#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/mem/c_string.h"
#include "libpandafile/class_data_accessor.h"
#include "libpandafile/method_data_accessor.h"

namespace panda::ecmascript {
QuickFixLoader::~QuickFixLoader()
{
    ClearReservedInfo();
}

bool QuickFixLoader::LoadPatch(JSThread *thread, const CString &patchFileName, const CString &baseFileName)
{
    const JSPandaFile *baseFile = JSPandaFileManager::GetInstance()->FindJSPandaFile(baseFileName);
    if (baseFile == nullptr) {
        LOG_ECMA(ERROR) << "find base jsPandafile failed";
        return false;
    }

    // The entry point is not work for merge abc.
    const JSPandaFile *patchFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, patchFileName, JSPandaFile::ENTRY_MAIN_FUNCTION);
    if (patchFile == nullptr) {
        LOG_ECMA(ERROR) << "load patch jsPandafile failed";
        return false;
    }

    return LoadPatchInternal(thread, baseFile, patchFile);
}

bool QuickFixLoader::LoadPatch(JSThread *thread, const CString &patchFileName, const void *patchBuffer,
                               size_t patchSize, const CString &baseFileName)
{
    const JSPandaFile *baseFile = JSPandaFileManager::GetInstance()->FindJSPandaFile(baseFileName);
    if (baseFile == nullptr) {
        LOG_ECMA(ERROR) << "find base jsPandafile failed";
        return false;
    }

    const JSPandaFile *patchFile = JSPandaFileManager::GetInstance()->LoadJSPandaFile(
        thread, patchFileName, JSPandaFile::ENTRY_MAIN_FUNCTION, patchBuffer, patchSize);
    if (patchFile == nullptr) {
        LOG_ECMA(ERROR) << "load patch jsPandafile failed";
        return false;
    }

    return LoadPatchInternal(thread, baseFile, patchFile);
}

bool QuickFixLoader::LoadPatchInternal(JSThread *thread, const JSPandaFile *baseFile, const JSPandaFile *patchFile)
{
    // support multi constpool: baseFile->GetPandaFile()->GetHeader()->num_indexes
    EcmaVM *vm = thread->GetEcmaVM();

    // hot reload and hot patch only support merge-abc file.
    if (baseFile->IsBundlePack() || patchFile->IsBundlePack()) {
        LOG_ECMA(ERROR) << "base or patch is not merge abc!";
        return false;
    }

    // Get base constpool.
    ParseAllConstpoolWithMerge(thread, baseFile);
    JSTaggedValue baseConstpoolValue = vm->FindConstpool(baseFile, 0);
    if (baseConstpoolValue.IsHole()) {
        LOG_ECMA(ERROR) << "base constpool is hole";
        return false;
    }

    JSHandle<ConstantPool> baseConstpool = JSHandle<ConstantPool>(thread, baseConstpoolValue);

    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // Get patch constpool.
    [[maybe_unused]] CVector<JSHandle<Program>> patchPrograms = ParseAllConstpoolWithMerge(thread, patchFile);
    JSTaggedValue patchConstpoolValue = vm->FindConstpool(patchFile, 0);
    if (patchConstpoolValue.IsHole()) {
        LOG_ECMA(ERROR) << "patch constpool is hole";
        return false;
    }

    JSHandle<ConstantPool> patchConstpool = JSHandle<ConstantPool>(thread, patchConstpoolValue);

    // Only esmodule support check
    if (CheckIsInvalidPatch(baseFile, patchFile, vm)) {
        LOG_ECMA(ERROR) << "Invalid patch";
        return false;
    }

    if (!ReplaceMethod(thread, baseFile, patchFile, baseConstpool, patchConstpool)) {
        LOG_ECMA(ERROR) << "replace method failed";
        return false;
    }

    vm->GetJsDebuggerManager()->GetHotReloadManager()->NotifyPatchLoaded(baseFile, patchFile);

    baseFileName_ = baseFile->GetJSPandaFileDesc();
    LOG_ECMA(INFO) << "LoadPatch success!";
    return true;
}

CVector<JSHandle<Program>> QuickFixLoader::ParseAllConstpoolWithMerge(JSThread *thread, const JSPandaFile *jsPandaFile)
{
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<ConstantPool> constpool;
    bool isNewVersion = jsPandaFile->IsNewVersion();
    if (!isNewVersion) {
        JSTaggedValue constpoolVal = vm->FindConstpool(jsPandaFile, 0);
        if (constpoolVal.IsHole()) {
            constpool = PandaFileTranslator::ParseConstPool(vm, jsPandaFile);
            // old version dont support multi constpool
            vm->AddConstpool(jsPandaFile, constpool.GetTaggedValue());
        } else {
            constpool = JSHandle<ConstantPool>(thread, constpoolVal);
        }
    }

    CVector<JSHandle<Program>> programs;
    auto recordInfos = jsPandaFile->GetJSRecordInfo();
    const CString &fileName = jsPandaFile->GetJSPandaFileDesc();
    for (const auto &item : recordInfos) {
        const CString &recordName = item.first;

        vm->GetModuleManager()->HostResolveImportedModuleWithMerge(fileName, recordName);

        uint32_t mainMethodIndex = jsPandaFile->GetMainMethodIndex(recordName);
        ASSERT(mainMethodIndex != 0);
        if (!isNewVersion) {
            PandaFileTranslator::ParseFuncAndLiteralConstPool(vm, jsPandaFile, recordName, constpool);
        } else {
            constpool = vm->FindOrCreateConstPool(jsPandaFile, panda_file::File::EntityId(mainMethodIndex));
        }

        // Generate Program for every record.
        JSHandle<Program> program =
            PandaFileTranslator::GenerateProgramInternal(vm, jsPandaFile, mainMethodIndex, constpool);
        programs.emplace_back(program);
    }
    if (isNewVersion) {
        GenerateConstpoolCache(thread, jsPandaFile, constpool);
    }
    return programs;
}

void QuickFixLoader::GenerateConstpoolCache(JSThread *thread, const JSPandaFile *jsPandaFile,
                                            JSHandle<ConstantPool> constpool)
{
    ASSERT(jsPandaFile->IsNewVersion());
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    Span<const uint32_t> classIndexes = jsPandaFile->GetClasses();
    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (pf->IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(*pf, classId);
        CString entry = jsPandaFile->ParseEntryPoint(utf::Mutf8AsCString(cda.GetDescriptor()));
        cda.EnumerateMethods([pf, thread, constpool, &entry](panda_file::MethodDataAccessor &mda) {
            auto codeId = mda.GetCodeId();
            ASSERT(codeId.has_value());
            panda_file::CodeDataAccessor codeDataAccessor(*pf, codeId.value());
            uint32_t codeSize = codeDataAccessor.GetCodeSize();
            const uint8_t *insns = codeDataAccessor.GetInstructions();

            auto bcIns = BytecodeInstruction(insns);
            auto bcInsLast = bcIns.JumpTo(codeSize);
            while (bcIns.GetAddress() != bcInsLast.GetAddress()) {
                if (!bcIns.HasFlag(BytecodeInstruction::Flags::STRING_ID) ||
                    !BytecodeInstruction::HasId(BytecodeInstruction::GetFormat(bcIns.GetOpcode()), 0)) {
                    BytecodeInstruction::Opcode opcode = bcIns.GetOpcode();
                    switch (opcode) {
                        // add opcode about method
                        case EcmaOpcode::DEFINEMETHOD_IMM8_ID16_IMM8:
                            U_FALLTHROUGH;
                        case EcmaOpcode::DEFINEMETHOD_IMM16_ID16_IMM8:
                            U_FALLTHROUGH;
                        case EcmaOpcode::DEFINEFUNC_IMM8_ID16_IMM8:
                            U_FALLTHROUGH;
                        case EcmaOpcode::DEFINEFUNC_IMM16_ID16_IMM8: {
                            uint32_t id = bcIns.GetId().AsRawValue();
                            ConstantPool::GetMethodFromCache(thread, constpool.GetTaggedValue(), id);
                            break;
                        }
                        case EcmaOpcode::CREATEOBJECTWITHBUFFER_IMM8_ID16:
                            U_FALLTHROUGH;
                        case EcmaOpcode::CREATEOBJECTWITHBUFFER_IMM16_ID16: {
                            uint32_t id = bcIns.GetId().AsRawValue();
                            ConstantPool::GetLiteralFromCache<ConstPoolType::OBJECT_LITERAL>(
                                thread, constpool.GetTaggedValue(), id, entry);
                            break;
                        }
                        case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
                            U_FALLTHROUGH;
                        case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM16_ID16: {
                            uint32_t id = bcIns.GetId().AsRawValue();
                            ConstantPool::GetLiteralFromCache<ConstPoolType::ARRAY_LITERAL>(
                                thread, constpool.GetTaggedValue(), id, entry);
                            break;
                        }
                        case EcmaOpcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8:
                            U_FALLTHROUGH;
                        case EcmaOpcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8: {
                            uint32_t methodId = bcIns.GetId(0).AsRawValue();
                            uint32_t literalId = bcIns.GetId(1).AsRawValue();
                            ConstantPool::GetClassMethodFromCache(thread, constpool, methodId);
                            ConstantPool::GetClassLiteralFromCache(thread, constpool, literalId, entry);
                            break;
                        }
                        default:
                            break;
                    }
                }
                bcIns = bcIns.GetNext();
            }
        });
    }
}

bool QuickFixLoader::ReplaceMethod(JSThread *thread,
                                   const JSPandaFile *baseFile,
                                   const JSPandaFile *patchFile,
                                   const JSHandle<ConstantPool> &baseConstpool,
                                   const JSHandle<ConstantPool> &patchConstpool)
{
    CUnorderedMap<uint32_t, MethodLiteral *> patchMethodLiterals = patchFile->GetMethodLiteralMap();
    auto baseConstpoolSize = baseConstpool->GetCacheLength();

    // Loop patch methodLiterals and base constpool to find same methodName and recordName both in base and patch.
    for (const auto &item : patchMethodLiterals) {
        MethodLiteral *patch = item.second;
        auto methodId = patch->GetMethodId();
        const char *patchMethodName = MethodLiteral::GetMethodName(patchFile, methodId);
        // Skip func_main_0, patch_main_0 and patch_main_1.
        if (std::strcmp(patchMethodName, JSPandaFile::ENTRY_FUNCTION_NAME) == 0 ||
            std::strcmp(patchMethodName, JSPandaFile::PATCH_FUNCTION_NAME_0) == 0 ||
            std::strcmp(patchMethodName, JSPandaFile::PATCH_FUNCTION_NAME_1) == 0) {
            continue;
        }

        CString patchRecordName = GetRecordName(patchFile, methodId);
        for (uint32_t index = 0; index < baseConstpoolSize; index++) {
            JSTaggedValue constpoolValue = baseConstpool->GetObjectFromCache(index);
            // For class inner function modified.
            if (constpoolValue.IsClassLiteral()) {
                JSHandle<ClassLiteral> classLiteral(thread, constpoolValue);
                JSHandle<TaggedArray> literalArray(thread, classLiteral->GetArray());
                for (uint32_t i = 0; i < literalArray->GetLength(); i++) {
                    JSTaggedValue literalItem = literalArray->Get(thread, i);
                    if (!literalItem.IsJSFunctionBase()) {
                        continue;
                    }
                    // Skip method that already been replaced.
                    if (HasClassMethodReplaced(index, i)) {
                        continue;
                    }
                    JSFunctionBase *func = JSFunctionBase::Cast(literalItem.GetTaggedObject());
                    Method *baseMethod = Method::Cast(func->GetMethod().GetTaggedObject());
                    if (std::strcmp(patchMethodName, baseMethod->GetMethodName()) != 0) {
                        continue;
                    }
                    // For merge abc, method and record name must be same to base.
                    CString baseRecordName = GetRecordName(baseFile, baseMethod->GetMethodId());
                    if (patchRecordName != baseRecordName) {
                        continue;
                    }

                    // Save base MethodLiteral and literal index.
                    MethodLiteral *base = baseFile->FindMethodLiteral(baseMethod->GetMethodId().GetOffset());
                    ASSERT(base != nullptr);
                    InsertBaseClassMethodInfo(index, i, base);

                    ReplaceMethodInner(thread, baseMethod, patch, patchConstpool.GetTaggedValue());
                    LOG_ECMA(INFO) << "Replace class method: " << patchRecordName << ":" << patchMethodName;
                    break;
                }
            }
            // For normal function and class constructor modified.
            if (constpoolValue.IsMethod()) {
                // Skip method that already been replaced.
                if (HasNormalMethodReplaced(index)) {
                    continue;
                }
                Method *baseMethod = Method::Cast(constpoolValue.GetTaggedObject());
                if (std::strcmp(patchMethodName, baseMethod->GetMethodName()) != 0) {
                    continue;
                }
                // For merge abc, method and record name must be same to base.
                CString baseRecordName = GetRecordName(baseFile, baseMethod->GetMethodId());
                if (patchRecordName != baseRecordName) {
                    continue;
                }

                // Save base MethodLiteral and constpool index.
                MethodLiteral *base = baseFile->FindMethodLiteral(baseMethod->GetMethodId().GetOffset());
                ASSERT(base != nullptr);
                reservedBaseMethodInfo_.emplace(index, base);

                ReplaceMethodInner(thread, baseMethod, patch, patchConstpool.GetTaggedValue());
                LOG_ECMA(INFO) << "Replace normal method: " << patchRecordName << ":" << patchMethodName;
                break;
            }
        }
    }

    if (reservedBaseMethodInfo_.empty() && reservedBaseClassInfo_.empty()) {
        LOG_ECMA(ERROR) << "can not find same method to base";
        return false;
    }
    return true;
}

bool QuickFixLoader::ExecutePatchMain(
    JSThread *thread, const JSHandle<Program> &patchProgram, const JSPandaFile *patchFile)
{
    if (patchProgram->GetMainFunction().IsUndefined()) {
        return true;
    }

    // For add a new function, Call patch_main_0.
    JSHandle<JSTaggedValue> global = thread->GetEcmaVM()->GetGlobalEnv()->GetJSGlobalObject();
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSHandle<JSTaggedValue> func(thread, patchProgram->GetMainFunction());
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, func, global, undefined, 0);
    EcmaInterpreter::Execute(info);

    if (thread->HasPendingException()) {
        // clear exception and rollback.
        thread->ClearException();
        UnloadPatch(thread, patchFile->GetJSPandaFileDesc());
        LOG_ECMA(ERROR) << "execute patch main has exception";
        return false;
    }
    return true;
}

bool QuickFixLoader::ExecutePatchMain(
    JSThread *thread, const CVector<JSHandle<Program>> &programs, const JSPandaFile *patchFile)
{
    for (const auto &item : programs) {
        if (!ExecutePatchMain(thread, item, patchFile)) {
            return false;
        }
    }
    return true;
}

CString QuickFixLoader::GetRecordName(const JSPandaFile *jsPandaFile, EntityId methodId)
{
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    panda_file::MethodDataAccessor patchMda(*pf, methodId);
    panda_file::ClassDataAccessor patchCda(*pf, patchMda.GetClassId());
    CString desc = utf::Mutf8AsCString(patchCda.GetDescriptor());
    return jsPandaFile->ParseEntryPoint(desc);
}

bool QuickFixLoader::UnloadPatch(JSThread *thread, const CString &patchFileName)
{
    const JSPandaFile *baseFile = JSPandaFileManager::GetInstance()->FindJSPandaFile(baseFileName_);
    if (baseFile == nullptr) {
        LOG_ECMA(ERROR) << "find base jsPandafile failed";
        return false;
    }

    const JSPandaFile *patchFile = JSPandaFileManager::GetInstance()->FindJSPandaFile(patchFileName);
    if (patchFile == nullptr) {
        LOG_ECMA(ERROR) << "find patch jsPandafile failed";
        return false;
    }

    if (reservedBaseMethodInfo_.empty() && reservedBaseClassInfo_.empty()) {
        LOG_ECMA(ERROR) << "no method need to unload";
        return false;
    }

    EcmaVM *vm = thread->GetEcmaVM();
    JSTaggedValue baseConstpoolValue = vm->FindConstpool(baseFile, 0);
    if (baseConstpoolValue.IsHole()) {
        LOG_ECMA(ERROR) << "base constpool is hole";
        return false;
    }

    ConstantPool *baseConstpool = ConstantPool::Cast(baseConstpoolValue.GetTaggedObject());
    for (const auto& item : reservedBaseMethodInfo_) {
        uint32_t constpoolIndex = item.first;
        MethodLiteral *base = item.second;

        JSTaggedValue value = baseConstpool->GetObjectFromCache(constpoolIndex);
        ASSERT(value.IsMethod());
        Method *method = Method::Cast(value.GetTaggedObject());

        ReplaceMethodInner(thread, method, base, baseConstpoolValue);
        LOG_ECMA(INFO) << "Replace normal method: " << method->GetMethodName();
    }

    for (const auto& item : reservedBaseClassInfo_) {
        uint32_t constpoolIndex = item.first;
        CUnorderedMap<uint32_t, MethodLiteral *> classLiteralInfo = item.second;
        JSHandle<ClassLiteral> classLiteral(thread, baseConstpool->GetObjectFromCache(constpoolIndex));
        JSHandle<TaggedArray> literalArray(thread, classLiteral->GetArray());

        for (const auto& classItem : classLiteralInfo) {
            MethodLiteral *base = classItem.second;

            JSTaggedValue value = literalArray->Get(thread, classItem.first);
            ASSERT(value.IsJSFunctionBase());
            JSFunctionBase *func = JSFunctionBase::Cast(value.GetTaggedObject());
            Method *method = Method::Cast(func->GetMethod().GetTaggedObject());

            ReplaceMethodInner(thread, method, base, baseConstpoolValue);
            LOG_ECMA(INFO) << "Replace class method: " << method->GetMethodName();
        }
    }

    vm->GetJsDebuggerManager()->GetHotReloadManager()->NotifyPatchUnloaded(patchFile);

    ClearReservedInfo();
    ClearPatchInfo(thread, patchFileName);

    LOG_ECMA(INFO) << "UnloadPatch success!";
    return true;
}

void QuickFixLoader::ClearPatchInfo(JSThread *thread, const CString &patchFileName) const
{
    EcmaVM *vm = thread->GetEcmaVM();

    vm->GetGlobalEnv()->SetGlobalPatch(thread, vm->GetFactory()->EmptyArray());

    // For release patch constpool and JSPandaFile.
    vm->CollectGarbage(TriggerGCType::FULL_GC);

    const JSPandaFile *patchFile = JSPandaFileManager::GetInstance()->FindJSPandaFile(patchFileName);
    if (patchFile != nullptr) {
        LOG_ECMA(INFO) << "patch jsPandaFile is not nullptr";
    }
}

void QuickFixLoader::ReplaceMethodInner(JSThread *thread,
                                        Method* destMethod,
                                        MethodLiteral *srcMethodLiteral,
                                        JSTaggedValue srcConstpool)
{
    destMethod->SetCallField(srcMethodLiteral->GetCallField());
    destMethod->SetLiteralInfo(srcMethodLiteral->GetLiteralInfo());
    destMethod->SetCodeEntryOrLiteral(reinterpret_cast<uintptr_t>(srcMethodLiteral));
    destMethod->SetExtraLiteralInfo(srcMethodLiteral->GetExtraLiteralInfo());
    destMethod->SetNativePointerOrBytecodeArray(const_cast<void *>(srcMethodLiteral->GetNativePointer()));
    destMethod->SetConstantPool(thread, srcConstpool);
    destMethod->SetProfileTypeInfo(thread, JSTaggedValue::Undefined());
    destMethod->SetAotCodeBit(false);
}

bool QuickFixLoader::HasNormalMethodReplaced(uint32_t index) const
{
    if (reservedBaseMethodInfo_.find(index) != reservedBaseMethodInfo_.end()) {
        return true;
    }
    return false;
}

bool QuickFixLoader::HasClassMethodReplaced(uint32_t constpoolIndex, uint32_t literalIndex) const
{
    auto iter = reservedBaseClassInfo_.find(constpoolIndex);
    if (iter != reservedBaseClassInfo_.end()) {
        auto &classLiteralInfo = iter->second;
        if (classLiteralInfo.find(literalIndex) != classLiteralInfo.end()) {
            return true;
        }
    }
    return false;
}

void QuickFixLoader::InsertBaseClassMethodInfo(uint32_t constpoolIndex, uint32_t literalIndex, MethodLiteral *base)
{
    auto iter = reservedBaseClassInfo_.find(constpoolIndex);
    if (iter != reservedBaseClassInfo_.end()) {
        auto &literalInfo = iter->second;
        if (literalInfo.find(literalIndex) != literalInfo.end()) {
            return;
        }
        literalInfo.emplace(literalIndex, base);
    } else {
        CUnorderedMap<uint32_t, MethodLiteral *> classLiteralInfo {{literalIndex, base}};
        reservedBaseClassInfo_.emplace(constpoolIndex, classLiteralInfo);
    }
}

bool QuickFixLoader::CheckIsInvalidPatch(const JSPandaFile *baseFile, const JSPandaFile *patchFile, EcmaVM *vm) const
{
    DISALLOW_GARBAGE_COLLECTION;

    auto thread = vm->GetJSThread();
    auto moduleManager = vm->GetModuleManager();
    auto patchRecordInfos = patchFile->GetJSRecordInfo();
    [[maybe_unused]] auto baseRecordInfos = baseFile->GetJSRecordInfo();

    for (const auto &patchItem : patchRecordInfos) {
        const CString &patchRecordName = patchItem.first;
        CString baseRecordName = patchRecordName;
        ASSERT(baseRecordInfos.find(baseRecordName) != baseRecordInfos.end());

        JSHandle<SourceTextModule> patchModule =
            JSHandle<SourceTextModule>::Cast(moduleManager->ResolveModuleWithMerge(thread,
            patchFile, patchRecordName));
        JSHandle<SourceTextModule> baseModule = moduleManager->HostGetImportedModule(baseRecordName);

        if (CheckIsModuleMismatch(thread, patchModule, baseModule)) {
            return true;
        }
    }

    return false;
}

bool QuickFixLoader::CheckIsModuleMismatch(JSThread *thread, JSHandle<SourceTextModule> patchModule,
                                           JSHandle<SourceTextModule> baseModule)
{
    JSTaggedValue patch;
    JSTaggedValue base;

    // ImportEntries: array or undefined
    patch = patchModule->GetImportEntries();
    base = baseModule->GetImportEntries();
    if (CheckImportEntriesMismatch(thread, patch, base)) {
        return true;
    }

    // LocalExportEntries: array or LocalExportEntry or undefined
    patch = patchModule->GetLocalExportEntries();
    base = baseModule->GetLocalExportEntries();
    if (CheckLocalExportEntriesMismatch(thread, patch, base)) {
        return true;
    }

    // IndirectExportEntries: array or IndirectExportEntry or undefined
    patch = patchModule->GetIndirectExportEntries();
    base = baseModule->GetIndirectExportEntries();
    if (CheckIndirectExportEntriesMismatch(thread, patch, base)) {
        return true;
    }

    // StarExportEntries: array or StarExportEntry or undefined
    patch = patchModule->GetStarExportEntries();
    base = baseModule->GetStarExportEntries();
    if (CheckStarExportEntriesMismatch(thread, patch, base)) {
        return true;
    }

    return false;
}

bool QuickFixLoader::CheckImportEntriesMismatch(JSThread *thread, JSTaggedValue patch, JSTaggedValue base)
{
    if (patch.IsTaggedArray() && base.IsTaggedArray()) {
        auto patchArr = TaggedArray::Cast(patch);
        auto baseArr = TaggedArray::Cast(base);
        uint32_t size = patchArr->GetLength();
        if (size != baseArr->GetLength()) {
            LOG_ECMA(ERROR) << "ModuleMismatch of ImportEntries length";
            return true;
        }
        for (uint32_t i = 0; i < size; i++) {
            auto patchEntry = ImportEntry::Cast(patchArr->Get(thread, i));
            auto baseEntry = ImportEntry::Cast(baseArr->Get(thread, i));
            if (CheckImportEntryMismatch(patchEntry, baseEntry)) {
                return true;
            }
        }
    } else if (!patch.IsUndefined() || !base.IsUndefined()) {
        LOG_ECMA(ERROR) << "ModuleMismatch of ImportEntries type";
        return true;
    }

    return false;
}

bool QuickFixLoader::CheckLocalExportEntriesMismatch(JSThread *thread, JSTaggedValue patch, JSTaggedValue base)
{
    if (patch.IsTaggedArray() && base.IsTaggedArray()) {
        auto patchArr = TaggedArray::Cast(patch);
        auto baseArr = TaggedArray::Cast(base);
        uint32_t size = patchArr->GetLength();
        if (size != baseArr->GetLength()) {
            LOG_ECMA(ERROR) << "ModuleMismatch of LocalExportEntries length";
            return true;
        }
        for (uint32_t i = 0; i < size; i++) {
            auto patchEntry = LocalExportEntry::Cast(patchArr->Get(thread, i));
            auto baseEntry = LocalExportEntry::Cast(baseArr->Get(thread, i));
            if (CheckLocalExportEntryMismatch(patchEntry, baseEntry)) {
                return true;
            }
        }
    } else if (patch.IsLocalExportEntry() && base.IsLocalExportEntry()) {
        auto patchEntry = LocalExportEntry::Cast(patch);
        auto baseEntry = LocalExportEntry::Cast(base);
        if (CheckLocalExportEntryMismatch(patchEntry, baseEntry)) {
            return true;
        }
    } else if (!patch.IsUndefined() || !base.IsUndefined()) {
        LOG_ECMA(ERROR) << "ModuleMismatch of LocalExportEntries type";
        return true;
    }

    return false;
}

bool QuickFixLoader::CheckIndirectExportEntriesMismatch(JSThread *thread, JSTaggedValue patch, JSTaggedValue base)
{
    if (patch.IsTaggedArray() && base.IsTaggedArray()) {
        auto patchArr = TaggedArray::Cast(patch);
        auto baseArr = TaggedArray::Cast(base);
        uint32_t size = patchArr->GetLength();
        if (size != baseArr->GetLength()) {
            LOG_ECMA(ERROR) << "ModuleMismatch of IndirectExportEntries length";
            return true;
        }
        for (uint32_t i = 0; i < size; i++) {
            auto patchEntry = IndirectExportEntry::Cast(patchArr->Get(thread, i));
            auto baseEntry = IndirectExportEntry::Cast(baseArr->Get(thread, i));
            if (CheckIndirectExportEntryMismatch(patchEntry, baseEntry)) {
                return true;
            }
        }
    } else if (patch.IsIndirectExportEntry() && base.IsIndirectExportEntry()) {
        auto patchEntry = IndirectExportEntry::Cast(patch);
        auto baseEntry = IndirectExportEntry::Cast(base);
        if (CheckIndirectExportEntryMismatch(patchEntry, baseEntry)) {
            return true;
        }
    } else if (!patch.IsUndefined() || !base.IsUndefined()) {
        LOG_ECMA(ERROR) << "ModuleMismatch of IndirectExportEntries type";
        return true;
    }

    return false;
}

bool QuickFixLoader::CheckStarExportEntriesMismatch(JSThread *thread, JSTaggedValue patch, JSTaggedValue base)
{
    if (patch.IsTaggedArray() && base.IsTaggedArray()) {
        auto patchArr = TaggedArray::Cast(patch);
        auto baseArr = TaggedArray::Cast(base);
        uint32_t size = patchArr->GetLength();
        if (size != baseArr->GetLength()) {
            LOG_ECMA(ERROR) << "ModuleMismatch of StarExportEntries length";
            return true;
        }
        for (uint32_t i = 0; i < size; i++) {
            auto patchEntry = StarExportEntry::Cast(patchArr->Get(thread, i));
            auto baseEntry = StarExportEntry::Cast(baseArr->Get(thread, i));
            if (CheckStarExportEntryMismatch(patchEntry, baseEntry)) {
                return true;
            }
        }
    } else if (patch.IsStarExportEntry() && base.IsStarExportEntry()) {
        auto patchEntry = StarExportEntry::Cast(patch);
        auto baseEntry = StarExportEntry::Cast(base);
        if (CheckStarExportEntryMismatch(patchEntry, baseEntry)) {
            return true;
        }
    } else if (!patch.IsUndefined() || !base.IsUndefined()) {
        LOG_ECMA(ERROR) << "ModuleMismatch of StarExportEntries type";
        return true;
    }

    return false;
}

bool QuickFixLoader::CheckImportEntryMismatch(ImportEntry *patch, ImportEntry *base)
{
    auto patchModuleRequest = EcmaString::Cast(patch->GetModuleRequest());
    auto baseModuleRequest = EcmaString::Cast(base->GetModuleRequest());
    auto patchImportName = EcmaString::Cast(patch->GetImportName());
    auto baseImportName = EcmaString::Cast(base->GetImportName());
    auto patchLocalName = EcmaString::Cast(patch->GetLocalName());
    auto baseLocalName = EcmaString::Cast(base->GetLocalName());
    if (!EcmaStringAccessor::StringsAreEqual(patchModuleRequest, baseModuleRequest)) {
        LOG_ECMA(ERROR) << "ModuleMismatch of ImportEntry: "
                        << EcmaStringAccessor(patchModuleRequest).ToStdString()
                        << " vs "
                        << EcmaStringAccessor(baseModuleRequest).ToStdString();
        return true;
    }
    if (!EcmaStringAccessor::StringsAreEqual(patchImportName, baseImportName)) {
        LOG_ECMA(ERROR) << "ModuleMismatch of ImportEntry: "
                        << EcmaStringAccessor(patchImportName).ToStdString()
                        << " vs "
                        << EcmaStringAccessor(baseImportName).ToStdString();
        return true;
    }
    if (!EcmaStringAccessor::StringsAreEqual(patchLocalName, baseLocalName)) {
        LOG_ECMA(ERROR) << "ModuleMismatch of ImportEntry: "
                        << EcmaStringAccessor(patchLocalName).ToStdString()
                        << " vs "
                        << EcmaStringAccessor(baseLocalName).ToStdString();
        return true;
    }

    return false;
}

bool QuickFixLoader::CheckLocalExportEntryMismatch(LocalExportEntry *patch, LocalExportEntry *base)
{
    auto patchExportName = EcmaString::Cast(patch->GetExportName());
    auto baseExportName = EcmaString::Cast(base->GetExportName());
    auto patchLocalName = EcmaString::Cast(patch->GetLocalName());
    auto baseLocalName = EcmaString::Cast(base->GetLocalName());
    if (!EcmaStringAccessor::StringsAreEqual(patchExportName, baseExportName)) {
        LOG_ECMA(ERROR) << "ModuleMismatch of LocalExportEntry: "
                        << EcmaStringAccessor(patchExportName).ToStdString()
                        << " vs "
                        << EcmaStringAccessor(baseExportName).ToStdString();
        return true;
    }
    if (!EcmaStringAccessor::StringsAreEqual(patchLocalName, baseLocalName)) {
        LOG_ECMA(ERROR) << "ModuleMismatch of LocalExportEntry: "
                        << EcmaStringAccessor(patchLocalName).ToStdString()
                        << " vs "
                        << EcmaStringAccessor(baseLocalName).ToStdString();
        return true;
    }

    return false;
}

bool QuickFixLoader::CheckIndirectExportEntryMismatch(IndirectExportEntry *patch, IndirectExportEntry *base)
{
    auto patchExportName = EcmaString::Cast(patch->GetExportName());
    auto baseExportName = EcmaString::Cast(base->GetExportName());
    auto patchModuleRequest = EcmaString::Cast(patch->GetModuleRequest());
    auto baseModuleRequest = EcmaString::Cast(base->GetModuleRequest());
    auto patchImportName = EcmaString::Cast(patch->GetImportName());
    auto baseImportName = EcmaString::Cast(base->GetImportName());
    if (!EcmaStringAccessor::StringsAreEqual(patchExportName, baseExportName)) {
        LOG_ECMA(ERROR) << "ModuleMismatch of IndirectExportEntry: "
                        << EcmaStringAccessor(patchExportName).ToStdString()
                        << " vs "
                        << EcmaStringAccessor(baseExportName).ToStdString();
        return true;
    }
    if (!EcmaStringAccessor::StringsAreEqual(patchModuleRequest, baseModuleRequest)) {
        LOG_ECMA(ERROR) << "ModuleMismatch of IndirectExportEntry: "
                        << EcmaStringAccessor(patchModuleRequest).ToStdString()
                        << " vs "
                        << EcmaStringAccessor(baseModuleRequest).ToStdString();
        return true;
    }
    if (!EcmaStringAccessor::StringsAreEqual(patchImportName, baseImportName)) {
        LOG_ECMA(ERROR) << "ModuleMismatch of IndirectExportEntry: "
                        << EcmaStringAccessor(patchImportName).ToStdString()
                        << " vs "
                        << EcmaStringAccessor(baseImportName).ToStdString();
        return true;
    }

    return false;
}

bool QuickFixLoader::CheckStarExportEntryMismatch(StarExportEntry *patch, StarExportEntry *base)
{
    auto patchModuleRequest = EcmaString::Cast(patch->GetModuleRequest());
    auto baseModuleRequest = EcmaString::Cast(base->GetModuleRequest());
    if (!EcmaStringAccessor::StringsAreEqual(patchModuleRequest, baseModuleRequest)) {
        LOG_ECMA(ERROR) << "ModuleMismatch of StarExportEntry: "
                        << EcmaStringAccessor(patchModuleRequest).ToStdString()
                        << " vs "
                        << EcmaStringAccessor(baseModuleRequest).ToStdString();
        return true;
    }

    return false;
}
}  // namespace panda::ecmascript
