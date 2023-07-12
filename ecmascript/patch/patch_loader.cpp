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
    DISALLOW_GARBAGE_COLLECTION;
    EcmaVM *vm = thread->GetEcmaVM();

    // hot reload and hot patch only support merge-abc file.
    if (!baseFile->IsMergedPF() || !patchFile->IsMergedPF()) {
        LOG_ECMA(ERROR) << "base or patch is not merge abc!";
        return PatchErrorCode::PACKAGE_NOT_ESMODULE;
    }

    // Generate patchInfo for hot reload, hot patch and cold patch.
    patchInfo = PatchLoader::GeneratePatchInfo(patchFile);

    if (!thread->GetCurrentEcmaContext()->HasCachedConstpool(baseFile)) {
        LOG_ECMA(INFO) << "cold patch!";
        return PatchErrorCode::SUCCESS;
    }

    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // store base constpool in global object for avoid gc.
    uint32_t num = baseFile->GetConstpoolNum();
    GlobalHandleCollection gloalHandleCollection(thread);
    for (uint32_t idx = 0; idx < num; idx++) {
        JSTaggedValue constpool = thread->GetCurrentEcmaContext()->FindConstpool(baseFile, idx);
        if (!constpool.IsHole()) {
            JSHandle<JSTaggedValue> constpoolHandle =
                gloalHandleCollection.NewHandle<JSTaggedValue>(constpool.GetRawData());
            patchInfo.baseConstpools.emplace_back(constpoolHandle);
        }
    }

    // create empty patch constpool for replace method constpool.
    thread->GetCurrentEcmaContext()->CreateAllConstpool(patchFile);
    FindAndReplaceSameMethod(thread, baseFile, patchFile, patchInfo);

    if (!ExecutePatchMain(thread, patchFile, baseFile, patchInfo)) {
        LOG_ECMA(ERROR) << "Execute patch main failed";
        return PatchErrorCode::INTERNAL_ERROR;
    }

    vm->GetJsDebuggerManager()->GetHotReloadManager()->NotifyPatchLoaded(baseFile, patchFile);
    return PatchErrorCode::SUCCESS;
}

bool PatchLoader::ExecutePatchMain(JSThread *thread, const JSPandaFile *patchFile,
                                   const JSPandaFile *baseFile, PatchInfo &patchInfo)
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
            constpoolVal = thread->GetCurrentEcmaContext()->FindConstpool(patchFile, mainMethodId);
        } else {
            constpoolVal = thread->GetCurrentEcmaContext()->FindConstpool(patchFile, 0);
        }
        ASSERT(!constpoolVal.IsHole());
        JSHandle<ConstantPool> constpool(thread, constpoolVal);
        MethodLiteral *mainMethodLiteral = patchFile->FindMethodLiteral(mainMethodIndex);
        JSHandle<Program> program = PandaFileTranslator::GenerateProgramInternal(vm, mainMethodLiteral, constpool);

        if (program->GetMainFunction().IsUndefined()) {
            continue;
        }

        // For add a new function, Call patch_main_0.
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        JSHandle<JSFunction> func(thread, program->GetMainFunction());
        JSHandle<JSTaggedValue> module =
            thread->GetCurrentEcmaContext()->GetModuleManager()->HostResolveImportedModuleWithMerge(
                patchFile->GetJSPandaFileDesc(), recordName);
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
    std::shared_ptr<JSPandaFile> baseFile = JSPandaFileManager::GetInstance()->FindJSPandaFile(baseFileName);
    if (baseFile == nullptr) {
        LOG_ECMA(ERROR) << "find base jsPandafile failed";
        return PatchErrorCode::FILE_NOT_EXECUTED;
    }

    std::shared_ptr<JSPandaFile> patchFile = JSPandaFileManager::GetInstance()->FindJSPandaFile(patchFileName);
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
    auto baseConstpoolValues = thread->GetCurrentEcmaContext()->FindConstpools(baseFile.get());
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
            ClassLiteral *classLiteral = ClassLiteral::Cast(baseConstpool->GetObjectFromCache(constpoolIndex));
            TaggedArray *literalArray = TaggedArray::Cast(classLiteral->GetArray());
            JSTaggedValue value = literalArray->Get(thread, literalIndex);
            ASSERT(value.IsJSFunctionBase());
            JSFunctionBase *func = JSFunctionBase::Cast(value.GetTaggedObject());
            patchMethod = Method::Cast(func->GetMethod().GetTaggedObject());
        }

        MethodLiteral *baseMethodLiteral = item.second;
        JSTaggedValue baseConstpoolValue = thread->GetCurrentEcmaContext()->FindConstpool(
            baseFile.get(), baseMethodLiteral->GetMethodId());
        ReplaceMethod(thread, patchMethod, baseMethodLiteral, baseConstpoolValue);
        LOG_ECMA(INFO) << "Replace base method: "
                       << patchMethod->GetRecordName()
                       << ":" << patchMethod->GetMethodName();
    }

    vm->GetJsDebuggerManager()->GetHotReloadManager()->NotifyPatchUnloaded(patchFile.get());

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

    std::shared_ptr<JSPandaFile> patchFile = JSPandaFileManager::GetInstance()->FindJSPandaFile(patchFileName);
    if (patchFile != nullptr) {
        LOG_ECMA(INFO) << "patch jsPandaFile is not nullptr";
    }
}

void PatchLoader::ReplaceMethod(JSThread *thread,
                                Method *destMethod,
                                MethodLiteral *srcMethodLiteral,
                                JSTaggedValue srcConstpool)
{
    // Update destmethod exclude ExtraLiteralInfo(FunctionKind). Method FunctionKind will be set after
    // building class inheritance relationship or defining gettersetter by value.
    //
    // HotReload of class inheritance will be affected.
    destMethod->SetCallField(srcMethodLiteral->GetCallField());
    destMethod->SetLiteralInfo(srcMethodLiteral->GetLiteralInfo());
    destMethod->SetCodeEntryOrLiteral(reinterpret_cast<uintptr_t>(srcMethodLiteral));
    destMethod->SetNativePointerOrBytecodeArray(const_cast<void *>(srcMethodLiteral->GetNativePointer()));
    destMethod->SetConstantPool(thread, srcConstpool);
    destMethod->SetProfileTypeInfo(thread, JSTaggedValue::Undefined());
    destMethod->SetAotCodeBit(false);
}

void PatchLoader::FindAndReplaceSameMethod(JSThread *thread, const JSPandaFile *baseFile,
                                           const JSPandaFile *patchFile, PatchInfo &patchInfo)
{
    auto context = thread->GetCurrentEcmaContext();
    const CMap<int32_t, JSTaggedValue> &baseConstpoolValues = context->FindConstpools(baseFile).value();
    for (const auto &item : baseConstpoolValues) {
        if (item.second.IsHole()) {
            continue;
        }

        ConstantPool *baseConstpool = ConstantPool::Cast(item.second.GetTaggedObject());
        uint32_t constpoolNum = item.first;
        uint32_t baseConstpoolSize = baseConstpool->GetCacheLength();
        for (uint32_t constpoolIndex = 0; constpoolIndex < baseConstpoolSize; constpoolIndex++) {
            JSTaggedValue constpoolValue = baseConstpool->GetObjectFromCache(constpoolIndex);
            if (!constpoolValue.IsMethod() && !constpoolValue.IsClassLiteral()) {
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

                JSTaggedValue patchConstpoolValue = context->FindConstpool(patchFile,
                    patchMethodLiteral->GetMethodId());
                ReplaceMethod(thread, baseMethod, patchMethodLiteral, patchConstpoolValue);

                BaseMethodIndex indexs = {constpoolNum, constpoolIndex};
                SaveBaseMethodInfo(patchInfo, baseFile, baseMethodId, indexs);
            } else if (constpoolValue.IsClassLiteral()) {
                // For class literal.
                ClassLiteral *classLiteral = ClassLiteral::Cast(constpoolValue);
                TaggedArray *literalArray = TaggedArray::Cast(classLiteral->GetArray());
                uint32_t literalLength = literalArray->GetLength();
                for (uint32_t literalIndex = 0; literalIndex < literalLength; literalIndex++) {
                    JSTaggedValue literalItem = literalArray->Get(thread, literalIndex);
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

                    JSTaggedValue patchConstpoolValue = context->FindConstpool(patchFile,
                        patchMethodLiteral->GetMethodId());
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
    for (const auto &item : map) {
        MethodLiteral *methodLiteral = item.second;
        EntityId methodId = EntityId(item.first);
        CString methodName = MethodLiteral::GetMethodName(patchFile, methodId);
        if (methodName == JSPandaFile::ENTRY_FUNCTION_NAME ||
            methodName == JSPandaFile::PATCH_FUNCTION_NAME_0 ||
            methodName == JSPandaFile::PATCH_FUNCTION_NAME_1) {
            continue;
        }

        CString recordName = MethodLiteral::GetRecordName(patchFile, methodId);
        auto iter = methodLiterals.find(recordName);
        if (iter != methodLiterals.end()) {
            iter->second.emplace(methodName, methodLiteral);
        } else {
            CMap<CString, MethodLiteral*> methodNameInfo = {{methodName, methodLiteral}};
            methodLiterals.emplace(recordName, std::move(methodNameInfo));
        }
    }

    PatchInfo patchInfo;
    patchInfo.patchFileName = patchFile->GetJSPandaFileDesc();
    patchInfo.patchMethodLiterals = std::move(methodLiterals);
    return patchInfo;
}
}  // namespace panda::ecmascript