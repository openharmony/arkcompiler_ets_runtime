/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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
#include "ecmascript/method.h"
#include "ecmascript/module/module_resolver.h"

#include "common_interfaces/base_runtime.h"

namespace panda::ecmascript {
PatchErrorCode PatchLoader::LoadPatchInternal(JSThread *thread, const JSPandaFile *baseFile,
                                              const JSPandaFile *patchFile, PatchInfo &patchInfo,
                                              const CMap<uint32_t, CString> &baseClassInfo)
{
    EcmaVM *vm = thread->GetEcmaVM();
    {
        DISALLOW_GARBAGE_COLLECTION;
        // hot reload and hot patch only support merge-abc file.
        if (baseFile->IsBundlePack() || patchFile->IsBundlePack()) {
            LOG_ECMA(ERROR) << "base or patch is not merge abc!";
            return PatchErrorCode::PACKAGE_NOT_ESMODULE;
        }
        // Generate patchInfo for hot reload, hot patch and cold patch.
        patchInfo = PatchLoader::GeneratePatchInfo(patchFile);
        if (!vm->HasCachedConstpool(baseFile)) {
            LOG_ECMA(INFO) << "cold patch!";
            vm->GetJsDebuggerManager()->GetHotReloadManager()->NotifyPatchLoaded(baseFile, patchFile);
            return PatchErrorCode::SUCCESS;
        }
    }

    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // store base constpool in global object for avoid gc.
    GlobalHandleCollection gloalHandleCollection(thread);
    for (uint32_t idx = 0; idx < baseFile->GetConstpoolNum(); idx++) {
        JSTaggedValue constpool = vm->FindConstpool(baseFile, idx);
        if (!constpool.IsHole()) {
            JSHandle<JSTaggedValue> constpoolHandle =
                gloalHandleCollection.NewHandle<JSTaggedValue>(constpool.GetRawData());
            patchInfo.baseConstpools.emplace_back(constpoolHandle);
        }
    }

    // create empty patch constpool for replace method constpool.
    vm->CreateAllConstpool(patchFile);
    FindAndReplaceSameMethod(thread, baseFile, patchFile, patchInfo, baseClassInfo);

    // cached patch modules can only be clear before load patch.
    vm->ClearPatchModules();
    // execute patch func_main_0 for hot reload, and patch_main_0 for hot patch.
    ExecuteFuncOrPatchMain(thread, patchFile, patchInfo);
    UpdateJSFunction(thread, patchInfo);

    vm->GetJsDebuggerManager()->GetHotReloadManager()->NotifyPatchLoaded(baseFile, patchFile);
    return PatchErrorCode::SUCCESS;
}

void PatchLoader::ExecuteFuncOrPatchMain(
    JSThread *thread, const JSPandaFile *jsPandaFile, const PatchInfo &patchInfo, bool loadPatch)
{
    ThreadManagedScope managedScope(thread);
    LOG_ECMA(DEBUG) << "execute main begin";
    thread->SetStageOfHotReload(StageOfHotReload::BEGIN_EXECUTE_PATCHMAIN);

    const auto &replacedRecordNames = patchInfo.replacedRecordNames;

    // Resolve all patch module records.
    CMap<CString, JSHandle<JSTaggedValue>> moduleRecords {};

    CString fileName = jsPandaFile->GetJSPandaFileDesc();
    for (const auto &recordName : replacedRecordNames) {
        JSHandle<JSTaggedValue> moduleRecord =
            ModuleResolver::HostResolveImportedModuleForHotReload(thread, fileName, recordName, ExecuteTypes::STATIC);
        RETURN_IF_ABRUPT_COMPLETION(thread);
        moduleRecords.emplace(recordName, moduleRecord);
    }

    for (const auto &recordName : replacedRecordNames) {
        LOG_ECMA(DEBUG) << "func main record name " << recordName;
        JSHandle<Program> program =
            JSPandaFileManager::GetInstance()->GenerateProgram(thread->GetEcmaVM(), jsPandaFile, recordName);
        if (program.IsEmpty()) {
            LOG_ECMA(ERROR) << "program is empty, invoke entrypoint failed";
            continue;
        }

        JSHandle<JSTaggedValue> moduleRecord = moduleRecords[recordName];
        SourceTextModule::Instantiate(thread, moduleRecord, ExecuteTypes::STATIC);
        RETURN_IF_ABRUPT_COMPLETION(thread);
        JSHandle<SourceTextModule> module = JSHandle<SourceTextModule>::Cast(moduleRecord);
        SourceTextModule::Evaluate(thread, module);
        RETURN_IF_ABRUPT_COMPLETION(thread);
    }

    if (loadPatch) {
        thread->SetStageOfHotReload(StageOfHotReload::LOAD_END_EXECUTE_PATCHMAIN);
    } else {
        thread->SetStageOfHotReload(StageOfHotReload::UNLOAD_END_EXECUTE_PATCHMAIN);
    }
    LOG_ECMA(DEBUG) << "execute main end";
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
    auto baseConstpoolValues = vm->FindConstpools(baseFile.get());
    if (!baseConstpoolValues.has_value()) {
        LOG_ECMA(ERROR) << "base constpool is empty";
        return PatchErrorCode::INTERNAL_ERROR;
    }

    patchInfo.replacedPatchMethods.clear();
    for (const auto &item : baseMethodInfo) {
        const auto &methodIndex = item.first;
        JSTaggedValue baseConstpool = baseConstpoolValues.value().get()[methodIndex.constpoolNum];

        Method *patchMethod = GetPatchMethod(thread, methodIndex, baseConstpool);

        MethodLiteral *baseMethodLiteral = item.second;
        EntityId baseMethodId = baseMethodLiteral->GetMethodId();
        JSTaggedValue baseConstpoolValue = vm->FindConstpool(baseFile.get(), baseMethodId);
        // After the method replaced, the methodId is baseMethodId,
        // and the JSPandaFile of replaced method is baseFile
        ReplacedMethod replacedMethod {baseMethodId, baseFileName};
        patchInfo.replacedPatchMethods.emplace(replacedMethod, patchMethod->GetRecordNameStr(thread));
        ReplaceMethod(thread, patchMethod, baseMethodLiteral, baseConstpoolValue);
        LOG_ECMA(DEBUG) << "Replace base method: "
                       << patchMethod->GetRecordNameStr(thread)
                       << ":" << patchMethod->GetMethodName(thread);
    }

    vm->ClearPatchModules();
    // execute base func_main_0 for recover global object.
    ExecuteFuncOrPatchMain(thread, baseFile.get(), patchInfo, false);
    UpdateJSFunction(thread, patchInfo);

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

Method *PatchLoader::GetPatchMethod(JSThread *thread,
    const BaseMethodIndex &methodIndex, const JSTaggedValue constpoolVal)
{
    uint32_t constpoolIndex = methodIndex.constpoolIndex;
    uint32_t literalIndex = methodIndex.literalIndex;
    Method *patchMethod = nullptr;
    ConstantPool *baseConstpool = ConstantPool::Cast(constpoolVal.GetTaggedObject());
    if (literalIndex == UINT32_MAX) {
        JSTaggedValue value = baseConstpool->GetObjectFromCache(thread, constpoolIndex);
        ASSERT(value.IsMethod());
        patchMethod = Method::Cast(value.GetTaggedObject());
    } else {
        ClassLiteral *classLiteral;
        if (baseConstpool->GetObjectFromCache(thread, constpoolIndex).IsHole()) {
            JSTaggedValue unsharedBaseConstpool = thread->GetEcmaVM()->FindOrCreateUnsharedConstpool(constpoolVal);
            classLiteral = ClassLiteral::Cast(ConstantPool::Cast(unsharedBaseConstpool.GetTaggedObject())
                                                  ->GetObjectFromCache(thread, constpoolIndex));
        } else {
            classLiteral = ClassLiteral::Cast(baseConstpool->GetObjectFromCache(thread, constpoolIndex));
        }
        TaggedArray *literalArray = TaggedArray::Cast(classLiteral->GetArray(thread));
        JSTaggedValue value = literalArray->Get(thread, literalIndex);
        ASSERT(value.IsFunctionTemplate());
        FunctionTemplate *func = FunctionTemplate::Cast(value.GetTaggedObject());
        patchMethod = Method::Cast(func->GetMethod(thread).GetTaggedObject());
    }
    return patchMethod;
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
    destMethod->SetAotCodeBit(false);
    destMethod->SetFpDelta(0);
}

// Iterator heap to update module in JSFunction.
void PatchLoader::UpdateJSFunction(JSThread *thread, PatchInfo &patchInfo)
{
    auto &replacedPatchMethods = patchInfo.replacedPatchMethods;
    auto heapVisitor = [&replacedPatchMethods, thread]([[maybe_unused]] BaseObject *baseObj) {
        TaggedObject *obj = static_cast<TaggedObject *>(baseObj);
        if (JSTaggedValue(obj).IsJSFunction()) {
            JSFunction *function = JSFunction::Cast(obj);
            EntityId methodId = Method::Cast(function->GetMethod(thread))->GetMethodId();
            const JSPandaFile *jsPandaFile = Method::Cast(function->GetMethod(thread))->GetJSPandaFile(thread);
            CString fileName {};
            if (jsPandaFile != nullptr) {
                fileName = jsPandaFile->GetJSPandaFileDesc();
            }
            ReplacedMethod replacedMethod {methodId, fileName};
            if (replacedPatchMethods.count(replacedMethod) > 0) {
                JSHandle<JSTaggedValue> moduleRecord =
                    thread->GetEcmaVM()->FindPatchModule(replacedPatchMethods[replacedMethod]);
                function->SetModule(thread, moduleRecord.GetTaggedValue());
                function->SetRawProfileTypeInfo<SKIP_BARRIER>(thread,
                    thread->GlobalConstants()->GetEmptyProfileTypeInfoCell());
            }
        } else if (JSTaggedValue(obj).IsFunctionTemplate()) {
            auto funcTemp = FunctionTemplate::Cast(obj);
            EntityId methodId = Method::Cast(funcTemp->GetMethod(thread))->GetMethodId();
            const JSPandaFile *jsPandaFile = Method::Cast(funcTemp->GetMethod(thread))->GetJSPandaFile(thread);
            CString fileName {};
            if (jsPandaFile != nullptr) {
                fileName = jsPandaFile->GetJSPandaFileDesc();
            }
            ReplacedMethod replacedMethod {methodId, fileName};
            if (replacedPatchMethods.count(replacedMethod) > 0) {
                JSHandle<JSTaggedValue> moduleRecord =
                    thread->GetEcmaVM()->FindPatchModule(replacedPatchMethods[replacedMethod]);
                funcTemp->SetModule(thread, moduleRecord.GetTaggedValue());
                funcTemp->SetRawProfileTypeInfo<SKIP_BARRIER>(thread,
                    thread->GlobalConstants()->GetEmptyProfileTypeInfoCell());
            }
        }
    };
    if (g_isEnableCMCGC) {
        common::BaseRuntime::ForEachObj(heapVisitor, false);
    } else {
        const Heap *heap = thread->GetEcmaVM()->GetHeap();
        heap->GetSweeper()->EnsureAllTaskFinished();
        heap->IterateOverObjects(heapVisitor);
    }
}

void PatchLoader::UpdateModuleForColdPatch(JSThread *thread, EntityId methodId, CString &recordName, bool hasModule)
{
    auto heapVisitor = [methodId, &recordName, hasModule, thread]([[maybe_unused]] BaseObject *baseObj) {
        TaggedObject *obj = static_cast<TaggedObject *>(baseObj);
        if (JSTaggedValue(obj).IsJSFunction()) {
            JSFunction *function = nullptr;
            function = JSFunction::Cast(obj);
            EntityId methodIdLoop = Method::Cast(function->GetMethod(thread))->GetMethodId();
            if (methodId == methodIdLoop) {
                JSHandle<JSTaggedValue> moduleRecord =
                thread->GetEcmaVM()->FindPatchModule(recordName);
                if (hasModule) {
                    function->SetModule(thread, moduleRecord.GetTaggedValue());
                } else {
                    function->SetModule(thread, JSTaggedValue::Undefined());
                }
            }
        } else if (JSTaggedValue(obj).IsFunctionTemplate()) {
            auto funcTemp = FunctionTemplate::Cast(obj);
            EntityId methodIdLoop = Method::Cast(funcTemp->GetMethod(thread))->GetMethodId();
            if (methodId == methodIdLoop) {
                JSHandle<JSTaggedValue> moduleRecord =
                thread->GetEcmaVM()->FindPatchModule(recordName);
                if (hasModule) {
                    funcTemp->SetModule(thread, moduleRecord.GetTaggedValue());
                } else {
                    funcTemp->SetModule(thread, JSTaggedValue::Undefined());
                }
            }
        }
    };
    if (g_isEnableCMCGC) {
        common::BaseRuntime::ForEachObj(heapVisitor, false);
    } else {
        const Heap *heap = thread->GetEcmaVM()->GetHeap();
        heap->GetSweeper()->EnsureAllTaskFinished();
        heap->IterateOverObjects(heapVisitor);
    }
}

void PatchLoader::FindAndReplaceSameMethod(JSThread *thread, const JSPandaFile *baseFile,
                                           const JSPandaFile *patchFile, PatchInfo &patchInfo,
                                           const CMap<uint32_t, CString> &baseClassInfo)
{
    auto vm = thread->GetEcmaVM();
    const CMap<int32_t, JSTaggedValue> &baseConstpoolValues = vm->FindConstpools(baseFile).value();
    for (const auto &item : baseConstpoolValues) {
        if (item.second.IsHole()) {
            continue;
        }

        ConstantPool *baseConstpool = ConstantPool::Cast(item.second.GetTaggedObject());
        uint32_t constpoolNum = item.first;
        uint32_t baseConstpoolSize = baseConstpool->GetCacheLength();
        for (uint32_t constpoolIndex = 0; constpoolIndex < baseConstpoolSize; constpoolIndex++) {
            JSTaggedValue constpoolValue = baseConstpool->GetObjectFromCache(thread, constpoolIndex);
            if (!constpoolValue.IsMethod() && !constpoolValue.IsClassLiteral()) {
                continue;
            }

            // For normal function and class constructor.
            if (constpoolValue.IsMethod()) {
                Method *baseMethod = Method::Cast(constpoolValue.GetTaggedObject());
                EntityId baseMethodId = baseMethod->GetMethodId();
                MethodLiteral *patchMethodLiteral =
                    FindSameMethod(patchInfo, baseFile, baseMethodId, baseClassInfo);
                if (patchMethodLiteral == nullptr) {
                    continue;
                }

                EntityId patchMethodId = patchMethodLiteral->GetMethodId();
                // After the method replaced, the methodId is patchMethodId,
                // and the JSPandaFile of replaced method is patchFile
                CString patchFileName = patchFile->GetJSPandaFileDesc();
                ReplacedMethod replacedMethod {patchMethodId, patchFileName};
                JSTaggedValue patchConstpoolValue = vm->FindConstpool(patchFile, patchMethodId);
                patchInfo.replacedPatchMethods.emplace(replacedMethod, baseMethod->GetRecordNameStr(thread));
                ReplaceMethod(thread, baseMethod, patchMethodLiteral, patchConstpoolValue);
                LOG_ECMA(DEBUG) << "Replace base method: "
                                << baseMethod->GetRecordNameStr(thread) << ": "
                                << baseMethod->GetMethodName(thread);

                BaseMethodIndex indexs = {constpoolNum, constpoolIndex};
                SaveBaseMethodInfo(patchInfo, baseFile, baseMethodId, indexs);
            } else if (constpoolValue.IsClassLiteral()) {
                // For class literal.
                FindAndReplaceClassLiteral(thread, baseFile, patchFile, constpoolValue, patchInfo,
                    constpoolIndex, constpoolNum, baseClassInfo);
            }
        }

        JSTaggedValue unsharedConstpool = vm->FindOrCreateUnsharedConstpool(item.second);
        ConstantPool *baseUnsharedConstpool = ConstantPool::Cast(unsharedConstpool.GetTaggedObject());
        const uint32_t len = baseUnsharedConstpool->GetCacheLength();
        for (uint32_t constpoolIndex = 0; constpoolIndex < len; constpoolIndex++) {
            JSTaggedValue constpoolValue = baseUnsharedConstpool->GetObjectFromCache(thread, constpoolIndex);
            if (!constpoolValue.IsMethod() && !constpoolValue.IsClassLiteral()) {
                continue;
            }
            if (constpoolValue.IsClassLiteral()) {
                FindAndReplaceClassLiteral(thread, baseFile, patchFile, constpoolValue, patchInfo,
                    constpoolIndex, constpoolNum, baseClassInfo);
            }
        }
    }
}
void PatchLoader::FindAndReplaceClassLiteral(JSThread *thread, const JSPandaFile *baseFile,
                                             const JSPandaFile *patchFile, JSTaggedValue constpoolValue,
                                             PatchInfo &patchInfo, uint32_t constpoolIndex,
                                             uint32_t constpoolNum, const CMap<uint32_t, CString> &baseClassInfo)
{
    // For class literal.
    ClassLiteral *classLiteral = ClassLiteral::Cast(constpoolValue);
    TaggedArray *literalArray = TaggedArray::Cast(classLiteral->GetArray(thread));
    uint32_t literalLength = literalArray->GetLength();
    for (uint32_t literalIndex = 0; literalIndex < literalLength; literalIndex++) {
        JSTaggedValue literalItem = literalArray->Get(thread, literalIndex);
        if (!literalItem.IsFunctionTemplate()) {
            continue;
        }

        // Every record is the same in current class literal.
        FunctionTemplate *func = FunctionTemplate::Cast(literalItem.GetTaggedObject());
        Method *baseMethod = Method::Cast(func->GetMethod(thread).GetTaggedObject());
        EntityId baseMethodId = baseMethod->GetMethodId();
        MethodLiteral *patchMethodLiteral =
            FindSameMethod(patchInfo, baseFile, baseMethodId, baseClassInfo);
        if (patchMethodLiteral == nullptr) {
            continue;
        }

        EntityId patchMethodId = patchMethodLiteral->GetMethodId();
        CString patchFileName = patchFile->GetJSPandaFileDesc();
        // After the method replaced, the methodId is patchMethodId,
        // and the JSPandaFile of replaced method is patchFile
        ReplacedMethod replacedMethod {patchMethodId, patchFileName};
        JSTaggedValue patchConstpoolValue = thread->GetEcmaVM()->FindConstpool(patchFile, patchMethodId);
        patchInfo.replacedPatchMethods.emplace(replacedMethod, baseMethod->GetRecordNameStr(thread));
        ReplaceMethod(thread, baseMethod, patchMethodLiteral, patchConstpoolValue);
        LOG_ECMA(DEBUG) << "Replace base method: "
                        << baseMethod->GetRecordNameStr(thread) << ": "
                        << baseMethod->GetMethodName(thread);

        BaseMethodIndex indexs = {constpoolNum, constpoolIndex, literalIndex};
        SaveBaseMethodInfo(patchInfo, baseFile, baseMethodId, indexs);
    }
}

MethodLiteral* PatchLoader::FindSameMethod(PatchInfo &patchInfo, const JSPandaFile *baseFile,
    EntityId baseMethodId, const CMap<uint32_t, CString> &baseClassInfo)
{
    const CUnorderedMap<PatchMethodIndex, MethodLiteral*, PatchMethodIndex::Hash> &patchMethodLiterals =
        patchInfo.patchMethodLiterals;
    if (!baseMethodId.IsValid() || baseMethodId.GetOffset() >= baseFile->GetFileSize()) {
        return nullptr;
    }
    CString baseRecordName = MethodLiteral::GetRecordName(baseFile, baseMethodId);
    CString baseClassName = "default";
    auto iter = baseClassInfo.find(baseMethodId.GetOffset());
    if (iter != baseClassInfo.end()) {
        baseClassName = iter->second;
    }
    CString baseMethodName = GetRealName(baseFile, baseMethodId, baseClassName);
    PatchMethodIndex patchMethodIndex = {baseRecordName, baseClassName, baseMethodName};
    auto methodIter = patchMethodLiterals.find(patchMethodIndex);
    if (methodIter == patchMethodLiterals.end()) {
        return nullptr;
    }

    // Reserved for HotPatch.
    patchInfo.replacedRecordNames.emplace(baseRecordName);
    return methodIter->second;
}

void PatchLoader::SaveBaseMethodInfo(PatchInfo &patchInfo, const JSPandaFile *baseFile,
                                     EntityId baseMethodId, const BaseMethodIndex &indexs)
{
    CUnorderedMap<BaseMethodIndex, MethodLiteral *, BaseMethodIndex::Hash> &baseMethodInfo = patchInfo.baseMethodInfo;
    MethodLiteral *baseMethodLiteral = baseFile->FindMethodLiteral(baseMethodId.GetOffset());
    CHECK_INPUT_NULLPTR(baseMethodLiteral, "SaveBaseMethodInfo:baseMethodLiteral is nullptr, offset: "
                                            + std::to_string(baseMethodId.GetOffset()));
    baseMethodInfo.emplace(indexs, baseMethodLiteral);
}

PatchInfo PatchLoader::GeneratePatchInfo(const JSPandaFile *patchFile)
{
    CMap<uint32_t, CString> patchClassInfo = CollectClassInfo(patchFile);

    const auto &map = patchFile->GetMethodLiteralMap();
    CUnorderedMap<PatchMethodIndex, MethodLiteral*, PatchMethodIndex::Hash> patchMethodLiterals;
    PatchInfo patchInfo;
    for (const auto &item : map) {
        MethodLiteral *methodLiteral = item.second;
        EntityId methodId = EntityId(item.first);
        CString className = "default"; // for normal method and constructor.
        auto iter = patchClassInfo.find(methodId.GetOffset());
        if (iter!= patchClassInfo.end()) {
            className = iter->second;
        }
        CString methodName = GetRealName(patchFile, methodId, className);
        if (methodName == JSPandaFile::PATCH_FUNCTION_NAME_0 ||
            methodName == JSPandaFile::PATCH_FUNCTION_NAME_1) {
            continue;
        }

        // if patchFile only include varibales, add recordName specially.
        CString recordName = MethodLiteral::GetRecordName(patchFile, methodId);
        if (methodName == JSPandaFile::ENTRY_FUNCTION_NAME) {
            patchInfo.replacedRecordNames.emplace(recordName);
        }

        PatchMethodIndex patchMethodIndex = {recordName, className, methodName};
        if (patchMethodLiterals.find(patchMethodIndex) == patchMethodLiterals.end()) {
            patchMethodLiterals.emplace(patchMethodIndex, methodLiteral);
        }
    }

    patchInfo.patchFileName = patchFile->GetJSPandaFileDesc();
    patchInfo.patchMethodLiterals = std::move(patchMethodLiterals);
    return patchInfo;
}

CMap<uint32_t, CString> PatchLoader::CollectClassInfo(const JSPandaFile *jsPandaFile)
{
    CMap<uint32_t, CString> classInfo {};
    auto &pandaFile = *jsPandaFile->GetPandaFile();
    auto classes = jsPandaFile->GetClasses();
    const auto &map = jsPandaFile->GetMethodLiteralMap();
    for (size_t i = 0; i < classes.Size(); i++) {
        EntityId classId(classes[i]);
        if (!classId.IsValid() || jsPandaFile->IsExternal(classId)) {
            continue;
        }

        panda_file::ClassDataAccessor cda(pandaFile, classId);
        cda.EnumerateMethods([&pandaFile, &map, &classInfo, jsPandaFile](panda_file::MethodDataAccessor &mda) {
            EntityId methodId = mda.GetMethodId();
            auto iter = map.find(methodId.GetOffset());
            MethodLiteral *methodLiteral = nullptr;
            if (iter != map.end()) {
                methodLiteral = iter->second;
            }

            auto codeId = mda.GetCodeId();
            ASSERT(codeId.has_value());
            panda_file::CodeDataAccessor codeDataAccessor(pandaFile, codeId.value());
            uint32_t codeSize = codeDataAccessor.GetCodeSize();
            const uint8_t *insns = codeDataAccessor.GetInstructions();

            auto bcIns = BytecodeInst(insns);
            auto bcInsLast = bcIns.JumpTo(codeSize);
            while (bcIns.GetAddress() != bcInsLast.GetAddress()) {
                BytecodeInstruction::Opcode opcode = static_cast<BytecodeInstruction::Opcode>(bcIns.GetOpcode());
                if (opcode == BytecodeInstruction::Opcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8 ||
                    opcode == BytecodeInstruction::Opcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8) {
                    EntityId entityId;
                    EntityId literalId;
                    if (opcode == BytecodeInstruction::Opcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8) {
                        entityId = jsPandaFile->ResolveMethodIndex(methodLiteral->GetMethodId(),
                            (bcIns.GetId <BytecodeInstruction::Format::IMM8_ID16_ID16_IMM16_V8, 0>()).AsRawValue());
                        literalId = jsPandaFile->ResolveMethodIndex(methodLiteral->GetMethodId(),
                            (bcIns.GetId <BytecodeInstruction::Format::IMM8_ID16_ID16_IMM16_V8, 1>()).AsRawValue());
                    } else if (opcode == BytecodeInstruction::Opcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8) {
                        entityId = jsPandaFile->ResolveMethodIndex(methodLiteral->GetMethodId(),
                            (bcIns.GetId <BytecodeInstruction::Format::IMM16_ID16_ID16_IMM16_V8, 0>()).AsRawValue());
                        literalId = jsPandaFile->ResolveMethodIndex(methodLiteral->GetMethodId(),
                            (bcIns.GetId <BytecodeInstruction::Format::IMM16_ID16_ID16_IMM16_V8, 1>()).AsRawValue());
                    }
                    CString className = "";
                    className = GetRealName(jsPandaFile, entityId, className);
                    CString recordName = MethodLiteral::GetRecordName(jsPandaFile, methodId);

                    LiteralDataAccessor lda = jsPandaFile->GetLiteralDataAccessor();
                    lda.EnumerateLiteralVals(literalId, [&classInfo, className]
                        (const LiteralValue &value, const LiteralTag &tag) {
                            switch (tag) {
                                case LiteralTag::METHOD:
                                case LiteralTag::GENERATORMETHOD: {
                                    uint32_t methodOffset = std::get<uint32_t>(value);
                                    classInfo.emplace(methodOffset, std::move(className));
                                    break;
                                }
                                default: {
                                    break;
                                }
                            }
                    });
                }
                auto nextInst = bcIns.GetNext();
                bcIns = nextInst;
            }
        });
    }
    return classInfo;
}

CString PatchLoader::GetRealName(const JSPandaFile *jsPandaFile, EntityId entityId, CString &className)
{
    std::string methodName(MethodLiteral::GetMethodName(jsPandaFile, entityId));
    size_t poiIndex = methodName.find_last_of('#');
    ASSERT(methodName.size() > 0);
    if (poiIndex != std::string::npos && poiIndex < methodName.size() - 1 && className != "default") {
        methodName = methodName.substr(poiIndex + 1);  // #...#functionName
        if (methodName.find('^') != std::string::npos) {
            poiIndex = methodName.find_last_of('^');
            methodName = methodName.substr(0, poiIndex);  // #...#functionName^1
        }
    }
    return ConvertToString(methodName);
}
}  // namespace panda::ecmascript
