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
#include "ecmascript/patch/patch_loader.h"

#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/napi/include/jsnapi.h"

#include "libpandafile/class_data_accessor.h"
#include "libpandafile/method_data_accessor.h"

namespace panda::ecmascript {

PatchErrorCode PatchLoader::LoadPatchInternal(JSThread *thread, const JSPandaFile *baseFile,
                                              const JSPandaFile *patchFile,
                                              CMap<BaseMethodIndex, MethodLiteral *> &baseMethodInfo)
{
    EcmaVM *vm = thread->GetEcmaVM();

    // hot reload and hot patch only support merge-abc file.
    if (baseFile->IsBundlePack() || patchFile->IsBundlePack()) {
        LOG_ECMA(ERROR) << "base or patch is not merge abc!";
        return PatchErrorCode::PACKAGE_NOT_ESMODULE;
    }

    // Get base constpool.
    ParseAllConstpoolWithMerge(thread, baseFile);
    auto baseConstpoolValues = vm->FindConstpools(baseFile);
    if (!baseConstpoolValues.has_value()) {
        LOG_ECMA(ERROR) << "base constpool is empty";
        return PatchErrorCode::INTERNAL_ERROR;
    }

    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // Get patch constpool.
    ParseAllConstpoolWithMerge(thread, patchFile);
    auto patchConstpoolValues = vm->FindConstpools(patchFile);
    if (!patchConstpoolValues.has_value()) {
        LOG_ECMA(ERROR) << "patch constpool is empty";
        return PatchErrorCode::INTERNAL_ERROR;
    }

    // Only esmodule support check
    if (CheckIsInvalidPatch(baseFile, patchFile, vm)) {
        LOG_ECMA(ERROR) << "Invalid patch";
        return PatchErrorCode::MODIFY_IMPORT_EXPORT_NOT_SUPPORT;
    }

    if (!ReplaceMethod(thread, baseFile, patchFile, baseConstpoolValues.value(), baseMethodInfo)) {
        LOG_ECMA(ERROR) << "Replace method failed";
        return PatchErrorCode::INTERNAL_ERROR;
    }

    if (!ExecutePatchMain(thread, patchFile, baseFile, baseMethodInfo)) {
        LOG_ECMA(ERROR) << "Execute patch main failed";
        return PatchErrorCode::INTERNAL_ERROR;
    }

    vm->GetJsDebuggerManager()->GetHotReloadManager()->NotifyPatchLoaded(baseFile, patchFile);
    return PatchErrorCode::SUCCESS;
}

void PatchLoader::ParseAllConstpoolWithMerge(JSThread *thread, const JSPandaFile *jsPandaFile)
{
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<ConstantPool> constpool;
    bool isNewVersion = jsPandaFile->IsNewVersion();
    if (!isNewVersion) {
        auto constpoolVal = vm->FindConstpool(jsPandaFile, 0);
        if (constpoolVal.IsHole()) {
            constpool = PandaFileTranslator::ParseConstPool(vm, jsPandaFile);
            // old version dont support multi constpool
            vm->AddConstpool(jsPandaFile, constpool.GetTaggedValue());
        } else {
            constpool = JSHandle<ConstantPool>(thread, constpoolVal);
        }
    }

    const auto &recordInfos = jsPandaFile->GetJSRecordInfo();
    const CString &fileName = jsPandaFile->GetJSPandaFileDesc();
    for (const auto &item : recordInfos) {
        const CString &recordName = item.first;

        vm->GetModuleManager()->HostResolveImportedModuleWithMerge(fileName, recordName);

        uint32_t mainMethodIndex = jsPandaFile->GetMainMethodIndex(recordName);
        ASSERT(mainMethodIndex != 0);
        panda_file::File::EntityId mainMethodId(mainMethodIndex);

        if (!isNewVersion) {
            PandaFileTranslator::ParseFuncAndLiteralConstPool(vm, jsPandaFile, recordName, constpool);
        } else {
            constpool = vm->FindOrCreateConstPool(jsPandaFile, mainMethodId);
        }
    }
    if (isNewVersion) {
        GenerateConstpoolCache(thread, jsPandaFile);
    }
}

void PatchLoader::GenerateConstpoolCache(JSThread *thread, const JSPandaFile *jsPandaFile)
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
        cda.EnumerateMethods([pf, jsPandaFile, thread, &entry](panda_file::MethodDataAccessor &mda) {
            auto methodId = mda.GetMethodId();
            EcmaVM *vm = thread->GetEcmaVM();
            JSHandle<ConstantPool> constpool =
                vm->FindOrCreateConstPool(jsPandaFile, panda_file::File::EntityId(methodId));

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
                            uint32_t memberMethodId = bcIns.GetId(0).AsRawValue();
                            uint32_t literalId = bcIns.GetId(1).AsRawValue();
                            ConstantPool::GetClassMethodFromCache(thread, constpool, memberMethodId);
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

bool PatchLoader::ReplaceMethod(JSThread *thread,
                                const JSPandaFile *baseFile,
                                const JSPandaFile *patchFile,
                                const CMap<int32_t, JSTaggedValue> &baseConstpoolValues,
                                CMap<BaseMethodIndex, MethodLiteral *> &baseMethodInfo)
{
    const CUnorderedMap<CString, BaseMethodIndex> cachedMethods =
        GenerateCachedMethods(thread, baseFile, patchFile, baseConstpoolValues);

    CUnorderedMap<uint32_t, MethodLiteral *> patchMethodLiterals = patchFile->GetMethodLiteralMap();
    for (const auto &item : patchMethodLiterals) {
        MethodLiteral *patch = item.second;
        auto methodId = patch->GetMethodId();
        const char *patchMethodName = MethodLiteral::GetMethodName(patchFile, methodId);
        CString patchRecordName = GetRecordName(patchFile, methodId);

        // Skip func_main_0, patch_main_0 and patch_main_1.
        if (std::strcmp(patchMethodName, JSPandaFile::ENTRY_FUNCTION_NAME) == 0 ||
            std::strcmp(patchMethodName, JSPandaFile::PATCH_FUNCTION_NAME_0) == 0 ||
            std::strcmp(patchMethodName, JSPandaFile::PATCH_FUNCTION_NAME_1) == 0) {
            continue;
        }

        CString methodKey = patchRecordName + "::" + CString(patchMethodName);
        if (cachedMethods.find(methodKey) == cachedMethods.end()) {
            continue;
        }

        BaseMethodIndex indexSet = cachedMethods.at(methodKey);
        uint32_t constpoolNum = indexSet.constpoolNum;
        uint32_t constpoolIndex = indexSet.constpoolIndex;
        uint32_t literalIndex = indexSet.literalIndex;
        JSHandle<ConstantPool> baseConstpool(thread, baseConstpoolValues.at(static_cast<int32_t>(constpoolNum)));
        JSTaggedValue constpoolValue = baseConstpool->GetObjectFromCache(constpoolIndex);

        Method *baseMethod = nullptr;
        if (literalIndex < UINT32_MAX) {
            // For class inner function modified.
            TaggedArray *classLiteral = TaggedArray::Cast(constpoolValue);
            JSTaggedValue literalItem = classLiteral->Get(thread, literalIndex);
            JSFunctionBase *func = JSFunctionBase::Cast(literalItem.GetTaggedObject());
            baseMethod = Method::Cast(func->GetMethod().GetTaggedObject());
        } else {
            // For normal function and class constructor modified.
            baseMethod = Method::Cast(constpoolValue.GetTaggedObject());
        }

        // Save base MethodLiteral and base method index.
        MethodLiteral *base = baseFile->FindMethodLiteral(baseMethod->GetMethodId().GetOffset());
        ASSERT(base != nullptr);
        InsertMethodInfo(indexSet, base, baseMethodInfo);

        JSTaggedValue patchConstpoolValue = thread->GetEcmaVM()->FindConstpool(patchFile, methodId);

        ReplaceMethodInner(thread, baseMethod, patch, patchConstpoolValue);
        LOG_ECMA(INFO) << "Replace method: " << patchRecordName << ":" << patchMethodName;
    }

    if (baseMethodInfo.empty()) {
        LOG_ECMA(ERROR) << "can not find same method to base";
        return false;
    }
    return true;
}

bool PatchLoader::ExecutePatchMain(JSThread *thread, const JSPandaFile *patchFile, const JSPandaFile *baseFile,
                                   const CMap<BaseMethodIndex, MethodLiteral *> &baseMethodInfo)
{
    EcmaVM *vm = thread->GetEcmaVM();

    const auto &recordInfos = patchFile->GetJSRecordInfo();
    bool isUpdateHotPatch = false;
    bool isHotPatch = false;
    bool isNewVersion = patchFile->IsNewVersion();
    for (const auto &item : recordInfos) {
        const CString &recordName = item.first;
        uint32_t mainMethodIndex = patchFile->GetMainMethodIndex(recordName);
        ASSERT(mainMethodIndex != 0);
        panda_file::File::EntityId mainMethodId(mainMethodIndex);

        // For HotPatch, generate program and execute for every record.
        if (!isUpdateHotPatch) {
            const char *mainMethodName = MethodLiteral::GetMethodName(patchFile, mainMethodId);
            if (std::strcmp(mainMethodName, JSPandaFile::PATCH_FUNCTION_NAME_0) == 0) {
                isHotPatch = true;
            }
            isUpdateHotPatch = true;
        }

        if (!isHotPatch) {
            LOG_ECMA(INFO) << "HotReload no need to execute patch main";
            return true;
        }

        JSTaggedValue constpoolVal = JSTaggedValue::Hole();
        if (isNewVersion) {
            constpoolVal = vm->FindConstpool(patchFile, mainMethodId);
        } else {
            constpoolVal = vm->FindConstpool(patchFile, 0);
        }
        ASSERT(!constpoolVal.IsHole());
        JSHandle<ConstantPool> constpool = JSHandle<ConstantPool>(thread, constpoolVal);
        JSHandle<Program> program =
            PandaFileTranslator::GenerateProgramInternal(vm, patchFile, mainMethodIndex, constpool);

        if (program->GetMainFunction().IsUndefined()) {
            continue;
        }

        // For add a new function, Call patch_main_0.
        JSHandle<JSTaggedValue> global = thread->GetEcmaVM()->GetGlobalEnv()->GetJSGlobalObject();
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        JSHandle<JSTaggedValue> func(thread, program->GetMainFunction());
        EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, func, global, undefined, 0);
        EcmaInterpreter::Execute(info);

        if (thread->HasPendingException()) {
            // clear exception and rollback.
            thread->ClearException();
            UnloadPatchInternal(
                thread, patchFile->GetJSPandaFileDesc(), baseFile->GetJSPandaFileDesc(), baseMethodInfo);
            LOG_ECMA(ERROR) << "execute patch main has exception";
            return false;
        }
    }
    return true;
}

CString PatchLoader::GetRecordName(const JSPandaFile *jsPandaFile, EntityId methodId)
{
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    panda_file::MethodDataAccessor patchMda(*pf, methodId);
    panda_file::ClassDataAccessor patchCda(*pf, patchMda.GetClassId());
    CString desc = utf::Mutf8AsCString(patchCda.GetDescriptor());
    return jsPandaFile->ParseEntryPoint(desc);
}

PatchErrorCode PatchLoader::UnloadPatchInternal(JSThread *thread, const CString &patchFileName,
                                                const CString &baseFileName,
                                                const CMap<BaseMethodIndex, MethodLiteral *> &baseMethodInfo)
{
    const JSPandaFile *baseFile = JSPandaFileManager::GetInstance()->FindJSPandaFile(baseFileName);
    if (baseFile == nullptr) {
        LOG_ECMA(ERROR) << "find base jsPandafile failed";
        return PatchErrorCode::FILE_NOT_EXECUTED;
    }

    const JSPandaFile *patchFile = JSPandaFileManager::GetInstance()->FindJSPandaFile(patchFileName);
    if (patchFile == nullptr) {
        LOG_ECMA(ERROR) << "find patch jsPandafile failed";
        return PatchErrorCode::FILE_NOT_FOUND;
    }

    if (baseMethodInfo.empty()) {
        LOG_ECMA(ERROR) << "no method need to unload";
        return PatchErrorCode::INTERNAL_ERROR;
    }

    EcmaVM *vm = thread->GetEcmaVM();
    auto baseConstpoolValues = vm->FindConstpools(baseFile);
    if (!baseConstpoolValues.has_value()) {
        LOG_ECMA(ERROR) << "base constpool is empty";
        return PatchErrorCode::INTERNAL_ERROR;
    }

    for (const auto &item : baseMethodInfo) {
        const auto &methodIndex = item.first;
        ConstantPool *baseConstpool = ConstantPool::Cast(
            (baseConstpoolValues.value().get()[methodIndex.constpoolNum]).GetTaggedObject());

        uint32_t constpoolIndex = methodIndex.constpoolIndex;
        MethodLiteral *base = item.second;

        uint32_t literalIndex = methodIndex.literalIndex;
        Method *method = nullptr;
        if (literalIndex == UINT32_MAX) {
            JSTaggedValue value = baseConstpool->GetObjectFromCache(constpoolIndex);
            ASSERT(value.IsMethod());
            method = Method::Cast(value.GetTaggedObject());
        } else {
            TaggedArray *classLiteral = TaggedArray::Cast(baseConstpool->GetObjectFromCache(constpoolIndex));
            JSTaggedValue value = classLiteral->Get(thread, literalIndex);
            ASSERT(value.IsJSFunctionBase());
            JSFunctionBase *func = JSFunctionBase::Cast(value.GetTaggedObject());
            method = Method::Cast(func->GetMethod().GetTaggedObject());
        }

        JSTaggedValue baseConstpoolValue = vm->FindConstpool(baseFile, base->GetMethodId());
        ReplaceMethodInner(thread, method, base, baseConstpoolValue);
        LOG_ECMA(INFO) << "Replace method: " << method->GetMethodName();
    }

    vm->GetJsDebuggerManager()->GetHotReloadManager()->NotifyPatchUnloaded(patchFile);

    ClearPatchInfo(thread, patchFileName);
    return PatchErrorCode::SUCCESS;
}

void PatchLoader::ClearPatchInfo(JSThread *thread, const CString &patchFileName)
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

void PatchLoader::ReplaceMethodInner(JSThread *thread,
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

void PatchLoader::InsertMethodInfo(BaseMethodIndex methodIndex, MethodLiteral *base,
                                   CMap<BaseMethodIndex, MethodLiteral *> &baseMethodInfo)
{
    if (baseMethodInfo.find(methodIndex) == baseMethodInfo.end()) {
        baseMethodInfo.emplace(methodIndex, base);
    }
}

bool PatchLoader::CheckIsInvalidPatch(const JSPandaFile *baseFile, const JSPandaFile *patchFile, EcmaVM *vm)
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
            moduleManager->ResolveModuleWithMerge(thread, patchFile, patchRecordName);
        JSHandle<SourceTextModule> baseModule = moduleManager->HostGetImportedModule(baseRecordName);

        if (CheckIsModuleMismatch(thread, patchModule, baseModule)) {
            return true;
        }
    }

    return false;
}

bool PatchLoader::CheckIsModuleMismatch(JSThread *thread, JSHandle<SourceTextModule> patchModule,
                                        JSHandle<SourceTextModule> baseModule)
{
    // ImportEntries: array or undefined
    JSTaggedValue patch = patchModule->GetImportEntries();
    JSTaggedValue base = baseModule->GetImportEntries();
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

bool PatchLoader::CheckImportEntriesMismatch(JSThread *thread, JSTaggedValue patch, JSTaggedValue base)
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

bool PatchLoader::CheckLocalExportEntriesMismatch(JSThread *thread, JSTaggedValue patch, JSTaggedValue base)
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

bool PatchLoader::CheckIndirectExportEntriesMismatch(JSThread *thread, JSTaggedValue patch, JSTaggedValue base)
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

bool PatchLoader::CheckStarExportEntriesMismatch(JSThread *thread, JSTaggedValue patch, JSTaggedValue base)
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

bool PatchLoader::CheckImportEntryMismatch(ImportEntry *patch, ImportEntry *base)
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

bool PatchLoader::CheckLocalExportEntryMismatch(LocalExportEntry *patch, LocalExportEntry *base)
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

bool PatchLoader::CheckIndirectExportEntryMismatch(IndirectExportEntry *patch, IndirectExportEntry *base)
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

bool PatchLoader::CheckStarExportEntryMismatch(StarExportEntry *patch, StarExportEntry *base)
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

CUnorderedMap<CString, BaseMethodIndex> PatchLoader::GenerateCachedMethods(JSThread *thread,
    const JSPandaFile *baseFile, const JSPandaFile *patchFile, const CMap<int32_t, JSTaggedValue> &baseConstpoolValues)
{
    // key :recordName + "::" + methodName
    // value :indexInfo
    CUnorderedMap<CString, BaseMethodIndex> cachedMethods {};

    const CUnorderedMap<uint32_t, MethodLiteral *> &patchMethodLiterals = patchFile->GetMethodLiteralMap();
    CUnorderedMap<CString, CUnorderedSet<CString>> patchMethodNames {}; // key :recordname; value :methodname
    for (const auto &iter : patchMethodLiterals) {
        MethodLiteral *patch = iter.second;
        auto patchMethodId = patch->GetMethodId();
        const char *patchMethodName = MethodLiteral::GetMethodName(patchFile, patchMethodId);
        CString patchRecordName = GetRecordName(patchFile, patchMethodId);
        if (patchMethodNames.find(patchRecordName) != patchMethodNames.end()) {
            patchMethodNames[patchRecordName].insert(CString(patchMethodName));
        } else {
            CUnorderedSet<CString> patchMethodNameSet {CString(patchMethodName)};
            patchMethodNames.emplace(std::move(patchRecordName), std::move(patchMethodNameSet));
        }
    }

    JSMutableHandle<ConstantPool> baseConstpool(thread, JSTaggedValue::Hole());
    for (const auto &iter : baseConstpoolValues) {
        baseConstpool.Update(iter.second);
        uint32_t constpoolNum = iter.first;
        uint32_t baseConstpoolSize = baseConstpool->GetCacheLength();
        for (uint32_t constpoolIndex = 0; constpoolIndex < baseConstpoolSize; constpoolIndex++) {
            JSTaggedValue constpoolValue = baseConstpool->GetObjectFromCache(constpoolIndex);
            if (constpoolValue.IsMethod()) {
                Method *baseMethod = Method::Cast(constpoolValue.GetTaggedObject());
                auto baseMethodId = baseMethod->GetMethodId();
                const char *baseMethodName = MethodLiteral::GetMethodName(baseFile, baseMethodId);
                CString baseRecordName = GetRecordName(baseFile, baseMethodId);

                auto item = patchMethodNames.find(baseRecordName);
                if (item != patchMethodNames.end() && item->second.count(CString(baseMethodName))) {
                    BaseMethodIndex indexs = {constpoolNum, constpoolIndex};
                    CString methodKey = baseRecordName + "::" + CString(baseMethodName);
                    cachedMethods.emplace(std::move(methodKey), std::move(indexs));
                }
            }

            if (constpoolValue.IsTaggedArray()) {
                TaggedArray *classLiteral = TaggedArray::Cast(constpoolValue);
                uint32_t literalLength = classLiteral->GetLength();
                for (uint32_t literalIndex = 0; literalIndex < literalLength; literalIndex++) {
                    JSTaggedValue literalItem = classLiteral->Get(thread, literalIndex);
                    if (!literalItem.IsJSFunctionBase()) {
                        continue;
                    }

                    JSFunctionBase *func = JSFunctionBase::Cast(literalItem.GetTaggedObject());
                    Method *baseMethod = Method::Cast(func->GetMethod().GetTaggedObject());
                    auto baseMethodId = baseMethod->GetMethodId();
                    const char *baseMethodName = MethodLiteral::GetMethodName(baseFile, baseMethodId);
                    CString baseRecordName = GetRecordName(baseFile, baseMethodId);

                    auto item = patchMethodNames.find(baseRecordName);
                    if (item != patchMethodNames.end() && item->second.count(CString(baseMethodName))) {
                        BaseMethodIndex indexs = {constpoolNum, constpoolIndex, literalIndex};
                        CString methodKey = baseRecordName + "::" + CString(baseMethodName);
                        cachedMethods.emplace(std::move(methodKey), std::move(indexs));
                    }
                }
            }
        }
    }
    return cachedMethods;
}
}  // namespace panda::ecmascript