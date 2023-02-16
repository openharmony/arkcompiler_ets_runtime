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

#include "ecmascript/global_handle_collection.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/napi/include/jsnapi.h"

namespace panda::ecmascript {
PatchErrorCode PatchLoader::LoadPatchInternal(JSThread *thread, const JSPandaFile *baseFile,
                                              const JSPandaFile *patchFile, PatchInfo &patchInfo)
{
    EcmaVM *vm = thread->GetEcmaVM();

    // hot reload and hot patch only support merge-abc file.
    if (baseFile->IsBundlePack() || patchFile->IsBundlePack()) {
        LOG_ECMA(ERROR) << "base or patch is not merge abc!";
        return PatchErrorCode::PACKAGE_NOT_ESMODULE;
    }

    // Only esmodule support check
    if (CheckIsInvalidPatch(baseFile, patchFile, vm)) {
        LOG_ECMA(ERROR) << "Invalid patch";
        return PatchErrorCode::MODIFY_IMPORT_EXPORT_NOT_SUPPORT;
    }

    // Generate patchInfo for hot reload, hot patch and cold patch.
    patchInfo = PatchLoader::GeneratePatchInfo(patchFile);

    if (!vm->HasCachedConstpool(baseFile)) {
        LOG_ECMA(INFO) << "cold patch!";
        return PatchErrorCode::SUCCESS;
    }

    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // store base constpool in global object for avoid gc.
    uint32_t num = baseFile->GetConstpoolNum();
    GlobalHandleCollection gloalHandleCollection(thread);
    for (uint32_t idx = 0; idx < num; idx++) {
        JSTaggedValue constpool = vm->FindConstpool(baseFile, idx);
        if (!constpool.IsHole()) {
            JSHandle<JSTaggedValue> constpoolHandle =
                gloalHandleCollection.NewHandle<JSTaggedValue>(constpool.GetRawData());
            patchInfo.baseConstpools.emplace_back(constpoolHandle);
        }
    }

    // create empty patch constpool for replace method constpool.
    vm->CreateAllConstpool(patchFile);
    FindAndReplaceSameMethod(thread, baseFile, patchFile, patchInfo);

    if (!ExecutePatchMain(thread, patchFile, baseFile, patchInfo)) {
        LOG_ECMA(ERROR) << "Execute patch main failed";
        return PatchErrorCode::INTERNAL_ERROR;
    }

    vm->GetJsDebuggerManager()->GetHotReloadManager()->NotifyPatchLoaded(baseFile, patchFile);
    return PatchErrorCode::SUCCESS;
}

bool PatchLoader::ExecutePatchMain(JSThread *thread, const JSPandaFile *patchFile, const JSPandaFile *baseFile,
                                   PatchInfo &patchInfo)
{
    EcmaVM *vm = thread->GetEcmaVM();

    const auto &recordInfos = patchFile->GetJSRecordInfo();
    bool isHotPatch = false;
    bool isNewVersion = patchFile->IsNewVersion();
    for (const auto &item : recordInfos) {
        const CString &recordName = item.first;
        uint32_t mainMethodIndex = patchFile->GetMainMethodIndex(recordName);
        ASSERT(mainMethodIndex != 0);
        EntityId mainMethodId(mainMethodIndex);

        // For HotPatch, generate program and execute for every record.
        if (!isHotPatch) {
            CString mainMethodName = MethodLiteral::GetMethodName(patchFile, mainMethodId);
            if (mainMethodName != JSPandaFile::PATCH_FUNCTION_NAME_0) {
                LOG_ECMA(INFO) << "HotReload no need to execute patch main";
                return true;
            }
            isHotPatch = true;
        }

        JSTaggedValue constpoolVal = JSTaggedValue::Hole();
        if (isNewVersion) {
            constpoolVal = vm->FindConstpool(patchFile, mainMethodId);
        } else {
            constpoolVal = vm->FindConstpool(patchFile, 0);
        }
        ASSERT(!constpoolVal.IsHole());
        JSHandle<ConstantPool> constpool(thread, constpoolVal);
        JSHandle<Program> program =
            PandaFileTranslator::GenerateProgramInternal(vm, patchFile, mainMethodIndex, constpool);

        if (program->GetMainFunction().IsUndefined()) {
            continue;
        }

        // For add a new function, Call patch_main_0.
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        JSHandle<JSFunction> func(thread, program->GetMainFunction());
        JSHandle<JSTaggedValue> module =
            vm->GetModuleManager()->HostResolveImportedModuleWithMerge(patchFile->GetJSPandaFileDesc(), recordName);
        func->SetModule(thread, module);
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, JSHandle<JSTaggedValue>(func), undefined, undefined, 0);
        EcmaInterpreter::Execute(info);

        if (thread->HasPendingException()) {
            // clear exception and rollback.
            thread->ClearException();
            UnloadPatchInternal(thread, patchFile->GetJSPandaFileDesc(), baseFile->GetJSPandaFileDesc(), patchInfo);
            LOG_ECMA(ERROR) << "execute patch main has exception";
            return false;
        }
    }
    return true;
}

PatchErrorCode PatchLoader::UnloadPatchInternal(JSThread *thread, const CString &patchFileName,
                                                const CString &baseFileName, PatchInfo &patchInfo)
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

    const auto &baseMethodInfo = patchInfo.baseMethodInfo;
    if (baseMethodInfo.empty()) {
        LOG_ECMA(INFO) << "no method need to unload";
        return PatchErrorCode::SUCCESS;
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
        uint32_t literalIndex = methodIndex.literalIndex;
        Method *patchMethod = nullptr;
        if (literalIndex == UINT32_MAX) {
            JSTaggedValue value = baseConstpool->GetObjectFromCache(constpoolIndex);
            ASSERT(value.IsMethod());
            patchMethod = Method::Cast(value.GetTaggedObject());
        } else {
            TaggedArray *classLiteral = TaggedArray::Cast(baseConstpool->GetObjectFromCache(constpoolIndex));
            JSTaggedValue value = classLiteral->Get(thread, literalIndex);
            ASSERT(value.IsJSFunctionBase());
            JSFunctionBase *func = JSFunctionBase::Cast(value.GetTaggedObject());
            patchMethod = Method::Cast(func->GetMethod().GetTaggedObject());
        }

        MethodLiteral *baseMethodLiteral = item.second;
        JSTaggedValue baseConstpoolValue = vm->FindConstpool(baseFile, baseMethodLiteral->GetMethodId());
        ReplaceMethod(thread, patchMethod, baseMethodLiteral, baseConstpoolValue);
        LOG_ECMA(INFO) << "Replace base method: "
                       << patchMethod->GetRecordName()
                       << ":" << patchMethod->GetMethodName();
    }

    vm->GetJsDebuggerManager()->GetHotReloadManager()->NotifyPatchUnloaded(patchFile);

    // release base constpool.
    CVector<JSHandle<JSTaggedValue>> &baseConstpools = patchInfo.baseConstpools;
    GlobalHandleCollection gloalHandleCollection(thread);
    for (auto &item : baseConstpools) {
        gloalHandleCollection.Dispose(item);
    }

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

void PatchLoader::ReplaceMethod(JSThread *thread,
                                Method *destMethod,
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

bool PatchLoader::CheckIsInvalidPatch(const JSPandaFile *baseFile, const JSPandaFile *patchFile, EcmaVM *vm)
{
    DISALLOW_GARBAGE_COLLECTION;

    auto thread = vm->GetJSThread();
    auto moduleManager = vm->GetModuleManager();
    auto patchRecordInfos = patchFile->GetJSRecordInfo();
    [[maybe_unused]] auto baseRecordInfos = baseFile->GetJSRecordInfo();

    CString patchFileName = patchFile->GetJSPandaFileDesc();
    CString baseFileName = baseFile->GetJSPandaFileDesc();
    for (const auto &patchItem : patchRecordInfos) {
        const CString &patchRecordName = patchItem.first;
        ASSERT(baseRecordInfos.find(patchRecordName) != baseRecordInfos.end());

        JSHandle<JSTaggedValue> patchModule = moduleManager->ResolveModuleWithMerge(thread, patchFile, patchRecordName);
        JSHandle<JSTaggedValue> baseModule = moduleManager->ResolveModuleWithMerge(thread, baseFile, patchRecordName);

        if (CheckIsModuleMismatch(thread, JSHandle<SourceTextModule>(patchModule),
                                  JSHandle<SourceTextModule>(baseModule))) {
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

void PatchLoader::FindAndReplaceSameMethod(JSThread *thread, const JSPandaFile *baseFile,
                                           const JSPandaFile *patchFile, PatchInfo &patchInfo)
{
    EcmaVM *vm = thread->GetEcmaVM();
    const CMap<int32_t, JSTaggedValue> &baseConstpoolValues = vm->FindConstpools(baseFile).value();
    for (const auto &item : baseConstpoolValues) {
        if (item.second.IsHole()) {
            continue;
        }

        ConstantPool *baseConstpool = ConstantPool::Cast(item.second.GetTaggedObject());
        uint32_t constpoolNum = item.first;
        uint32_t baseConstpoolSize = baseConstpool->GetCacheLength();
        for (uint32_t constpoolIndex = 0; constpoolIndex < baseConstpoolSize; constpoolIndex++) {
            JSTaggedValue constpoolValue = baseConstpool->GetObjectFromCache(constpoolIndex);
            if (!constpoolValue.IsMethod() && !constpoolValue.IsTaggedArray()) {
                continue;
            }

            // For normal function and class constructor.
            if (constpoolValue.IsMethod()) {
                Method *baseMethod = Method::Cast(constpoolValue.GetTaggedObject());
                EntityId baseMethodId = baseMethod->GetMethodId();
                MethodLiteral *patchMethodLiteral = FindSameMethod(patchInfo, baseFile, baseMethodId);
                if (patchMethodLiteral == nullptr) {
                    continue;
                }

                JSTaggedValue patchConstpoolValue = vm->FindConstpool(patchFile, patchMethodLiteral->GetMethodId());
                ReplaceMethod(thread, baseMethod, patchMethodLiteral, patchConstpoolValue);

                BaseMethodIndex indexs = {constpoolNum, constpoolIndex};
                SaveBaseMethodInfo(patchInfo, baseFile, baseMethodId, indexs);
            } else if (constpoolValue.IsTaggedArray()) {
                // For class literal.
                TaggedArray *classLiteral = TaggedArray::Cast(constpoolValue.GetTaggedObject());
                uint32_t literalLength = classLiteral->GetLength();
                for (uint32_t literalIndex = 0; literalIndex < literalLength; literalIndex++) {
                    JSTaggedValue literalItem = classLiteral->Get(thread, literalIndex);
                    if (!literalItem.IsJSFunctionBase()) {
                        continue;
                    }

                    // Every record is the same in current class literal.
                    JSFunctionBase *func = JSFunctionBase::Cast(literalItem.GetTaggedObject());
                    Method *baseMethod = Method::Cast(func->GetMethod().GetTaggedObject());
                    EntityId baseMethodId = baseMethod->GetMethodId();
                    MethodLiteral *patchMethodLiteral = FindSameMethod(patchInfo, baseFile, baseMethodId);
                    if (patchMethodLiteral == nullptr) {
                        continue;
                    }

                    JSTaggedValue patchConstpoolValue = vm->FindConstpool(patchFile, patchMethodLiteral->GetMethodId());
                    ReplaceMethod(thread, baseMethod, patchMethodLiteral, patchConstpoolValue);

                    BaseMethodIndex indexs = {constpoolNum, constpoolIndex, literalIndex};
                    SaveBaseMethodInfo(patchInfo, baseFile, baseMethodId, indexs);
                }
            }
        }
    }
}

MethodLiteral* PatchLoader::FindSameMethod(PatchInfo &patchInfo, const JSPandaFile *baseFile, EntityId baseMethodId)
{
    const CMap<CString, CMap<CString, MethodLiteral*>> &patchMethodLiterals = patchInfo.patchMethodLiterals;
    CString baseRecordName = MethodLiteral::GetRecordName(baseFile, baseMethodId);
    auto recordIter = patchMethodLiterals.find(baseRecordName);
    if (recordIter == patchMethodLiterals.end()) {
        return nullptr;
    }

    CString baseMethodName = MethodLiteral::GetMethodName(baseFile, baseMethodId);
    auto methodIter = recordIter->second.find(baseMethodName);
    if (methodIter == recordIter->second.end()) {
        return nullptr;
    }

    LOG_ECMA(INFO) << "Replace base method: " << baseRecordName << ":" << baseMethodName;
    return methodIter->second;
}

void PatchLoader::SaveBaseMethodInfo(PatchInfo &patchInfo, const JSPandaFile *baseFile,
                                     EntityId baseMethodId, const BaseMethodIndex &indexs)
{
    CMap<BaseMethodIndex, MethodLiteral *> &baseMethodInfo = patchInfo.baseMethodInfo;
    MethodLiteral *baseMethodLiteral = baseFile->FindMethodLiteral(baseMethodId.GetOffset());
    ASSERT(baseMethodLiteral != nullptr);
    baseMethodInfo.emplace(indexs, baseMethodLiteral);
}

PatchInfo PatchLoader::GeneratePatchInfo(const JSPandaFile *patchFile)
{
    const auto &map = patchFile->GetMethodLiteralMap();
    CMap<CString, CMap<CString, MethodLiteral*>> methodLiterals;
    CMap<CString, MethodLiteral*> methodNameInfo;
    for (const auto &item : map) {
        MethodLiteral *methodLiteral = item.second;
        EntityId methodId = EntityId(item.first);
        CString methodName = MethodLiteral::GetMethodName(patchFile, methodId);
        if (methodName == JSPandaFile::ENTRY_FUNCTION_NAME ||
            methodName == JSPandaFile::PATCH_FUNCTION_NAME_0 ||
            methodName == JSPandaFile::PATCH_FUNCTION_NAME_1) {
            continue;
        }

        methodNameInfo.emplace(methodName, methodLiteral);
        CString recordName = MethodLiteral::GetRecordName(patchFile, methodId);
        if (methodLiterals.find(recordName) != methodLiterals.end()) {
            methodLiterals[recordName] = methodNameInfo;
        } else {
            methodLiterals.emplace(recordName, methodNameInfo);
        }
    }

    PatchInfo patchInfo;
    patchInfo.patchFileName = patchFile->GetJSPandaFileDesc();
    patchInfo.patchMethodLiterals = methodLiterals;
    return patchInfo;
}
}  // namespace panda::ecmascript