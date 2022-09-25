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
    baseFile_ = JSPandaFileManager::GetInstance()->FindJSPandaFile(baseFileName);
    if (baseFile_ == nullptr) {
        LOG_FULL(ERROR) << "find base jsPandafile failed";
        return false;
    }

    // The entry point is not work for merge abc.
    patchFile_ = JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, patchFileName,
                                                                    JSPandaFile::PATCH_ENTRY_FUNCTION);
    if (patchFile_ == nullptr) {
        LOG_FULL(ERROR) << "load patch jsPandafile failed";
        return false;
    }

    return LoadPatchInternal(thread);
}

bool QuickFixLoader::LoadPatch(JSThread *thread, const CString &patchFileName, const void *patchBuffer,
                               size_t patchSize, const CString &baseFileName)
{
    // Get base constpool.
    baseFile_ = JSPandaFileManager::GetInstance()->FindJSPandaFile(baseFileName);
    if (baseFile_ == nullptr) {
        LOG_FULL(ERROR) << "find base jsPandafile failed";
        return false;
    }

    patchFile_ = JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, patchFileName,
                                                                    JSPandaFile::PATCH_ENTRY_FUNCTION,
                                                                    patchBuffer, patchSize);
    if (patchFile_ == nullptr) {
        LOG_FULL(ERROR) << "load patch jsPandafile failed";
        return false;
    }

    return LoadPatchInternal(thread);
}

bool QuickFixLoader::LoadPatchInternal(JSThread *thread)
{
    EcmaVM *vm = thread->GetEcmaVM();
    if (baseFile_->IsBundlePack() != patchFile_->IsBundlePack()) {
        LOG_FULL(ERROR) << "base and patch is not the same type hap!";
        return false;
    }

    if (!baseFile_->IsBundlePack()) {
        // Get base constpool.
        ParseAllConstpoolWithMerge(thread, baseFile_);
        JSTaggedValue baseConstpoolValue = vm->FindConstpool(baseFile_, 0);
        if (baseConstpoolValue.IsHole()) {
            LOG_FULL(ERROR) << "base constpool is hole";
            return false;
        }
        JSHandle<ConstantPool> baseConstpool = JSHandle<ConstantPool>(thread, baseConstpoolValue);

        // Get patch constpool.
        [[maybe_unused]] CVector<JSHandle<Program>> patchPrograms = ParseAllConstpoolWithMerge(thread, patchFile_);
        JSTaggedValue patchConstpoolValue = vm->FindConstpool(patchFile_, 0);
        if (patchConstpoolValue.IsHole()) {
            LOG_FULL(ERROR) << "patch constpool is hole";
            return false;
        }
        JSHandle<ConstantPool> patchConstpool = JSHandle<ConstantPool>(thread, patchConstpoolValue);
        // TODO: execute patch_main_0 for add function.
        if (!ReplaceMethod(thread, baseConstpool, patchConstpool)) {
            LOG_FULL(ERROR) << "replace method failed";
            return false;
        }
    } else {
        // Get base constpool.
        JSTaggedValue baseConstpoolValue = vm->FindConstpool(baseFile_, 0);
        if (baseConstpoolValue.IsHole()) {
            LOG_FULL(ERROR) << "base constpool is hole";
            return false;
        }
        JSHandle<ConstantPool> baseConstpool = JSHandle<ConstantPool>(thread, baseConstpoolValue);

        // Get patch constpool.
        vm->GetModuleManager()->ResolveModule(thread, patchFile_);
        JSHandle<Program> patchProgram =
            JSPandaFileManager::GetInstance()->GenerateProgram(vm, patchFile_, JSPandaFile::PATCH_ENTRY_FUNCTION);
        JSTaggedValue patchConstpoolValue = vm->FindConstpool(patchFile_, 0);
        if (patchConstpoolValue.IsHole()) {
            LOG_FULL(ERROR) << "patch constpool is hole";
            return false;
        }
        JSHandle<ConstantPool> patchConstpool = JSHandle<ConstantPool>(thread, patchConstpoolValue);

        if (!ReplaceMethod(thread, baseConstpool, patchConstpool)) {
            LOG_FULL(ERROR) << "replace method failed";
            return false;
        }
        if (!ExecutePatchMain(thread, patchProgram)) {
            LOG_FULL(ERROR) << "execute patch main failed";
            return false;
        }
    }
    return true;
}

CVector<JSHandle<Program>> QuickFixLoader::ParseAllConstpoolWithMerge(JSThread *thread, const JSPandaFile *jsPandaFile)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(vm->GetGlobalEnv()->GetFunctionClassWithProto());

    JSHandle<ConstantPool> constpool;
    JSTaggedValue constpoolValue = vm->FindConstpool(jsPandaFile, 0);
    if (constpoolValue.IsHole()) {
        constpool = PandaFileTranslator::ParseConstPool(vm, jsPandaFile);
        int32_t index = 0;
        int32_t total = 1;
        vm->AddConstpool(jsPandaFile, constpool.GetTaggedValue(), index, total);
    } else {
        constpool = JSHandle<ConstantPool>(thread, constpoolValue);
    }

    CVector<JSHandle<Program>> programs;
    auto recordInfos = jsPandaFile->GetJSRecordInfo();
    CString fileName = jsPandaFile->GetJSPandaFileDesc();
    for (const auto &item : recordInfos) {
        CString recordName = item.first;
        LOG_FULL(DEBUG) << "Parse constpool: " << fileName << ":" << recordName;
        vm->GetModuleManager()->HostResolveImportedModuleWithMerge(fileName, recordName);
        PandaFileTranslator::ParseFuncAndLiteralConstPool(vm, jsPandaFile, recordName, constpool);

        // Generate Program for every record.
        uint32_t mainMethodIndex = jsPandaFile->GetMainMethodIndex(recordName);
        auto methodLiteral = jsPandaFile->FindMethodLiteral(mainMethodIndex);
        JSHandle<Program> program = factory->NewProgram();
        if (methodLiteral == nullptr) {
            program->SetMainFunction(thread, JSTaggedValue::Undefined());
        } else {
            JSHandle<Method> method = factory->NewMethod(methodLiteral);
            JSHandle<JSFunction> mainFunc = factory->NewJSFunctionByHClass(method, hclass);

            program->SetMainFunction(thread, mainFunc.GetTaggedValue());
            method->SetConstantPool(thread, constpool);
            programs.emplace_back(program);
        }
    }
    return programs;
}

bool QuickFixLoader::ReplaceMethod(JSThread *thread,
                                   const JSHandle<ConstantPool> &baseConstpool,
                                   const JSHandle<ConstantPool> &patchConstpool)
{
    CUnorderedMap<uint32_t, MethodLiteral *> patchMethodLiterals = patchFile_->GetMethodLiteralMap();
    auto baseConstpoolSize = baseConstpool->GetCacheLength();

    // Loop patch methodLiterals and base constpool to find same methodName and recordName both in base and patch.
    for (const auto &item : patchMethodLiterals) {
        MethodLiteral *patch = item.second;
        auto methodId = patch->GetMethodId();
        const char *patchMethodName = MethodLiteral::GetMethodName(patchFile_, methodId);
        // Skip func_main_0, patch_main_0 and patch_main_1.
        if (std::strcmp(patchMethodName, JSPandaFile::ENTRY_FUNCTION_NAME) == 0 ||
            std::strcmp(patchMethodName, JSPandaFile::PATCH_FUNCTION_NAME_0) == 0 ||
            std::strcmp(patchMethodName, JSPandaFile::PATCH_FUNCTION_NAME_1) == 0) {
            continue;
        }

        CString patchRecordName = GetRecordName(patchFile_, methodId);
        for (uint32_t index = 0; index < baseConstpoolSize; index++) {
            JSTaggedValue constpoolValue = baseConstpool->GetObjectFromCache(index);
            // For class inner function modified.
            if (constpoolValue.IsTaggedArray()) {
                JSHandle<TaggedArray> classLiteral(thread, constpoolValue);
                for (uint32_t i = 0; i < classLiteral->GetLength(); i++) {
                    JSTaggedValue literalItem = classLiteral->Get(thread, i);
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
                    CString baseRecordName = GetRecordName(baseFile_, baseMethod->GetMethodId());
                    if (patchRecordName != baseRecordName) {
                        continue;
                    }

                    // Save base MethodLiteral and literal index.
                    MethodLiteral *base = baseFile_->FindMethodLiteral(baseMethod->GetMethodId().GetOffset());
                    ASSERT(base != nullptr);
                    InsertBaseClassMethodInfo(index, i, base);

                    ReplaceMethodInner(thread, baseMethod, patch, patchConstpool.GetTaggedValue());
                    LOG_FULL(INFO) << "Replace class method: " << patchRecordName << ":" << patchMethodName;
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
                CString baseRecordName = GetRecordName(baseFile_, baseMethod->GetMethodId());
                if (patchRecordName != baseRecordName) {
                    continue;
                }

                // Save base MethodLiteral and constpool index.
                MethodLiteral *base = baseFile_->FindMethodLiteral(baseMethod->GetMethodId().GetOffset());
                ASSERT(base != nullptr);
                reservedBaseMethodInfo_.emplace(index, base);

                ReplaceMethodInner(thread, baseMethod, patch, patchConstpool.GetTaggedValue());
                LOG_FULL(INFO) << "Replace normal method: " << patchRecordName << ":" << patchMethodName;
                break;
            }
        }
    }

    if (reservedBaseMethodInfo_.empty() && reservedBaseClassInfo_.empty()) {
        LOG_FULL(ERROR) << "can not find same method to base";
        return false;
    }
    return true;
}

bool QuickFixLoader::ExecutePatchMain(JSThread *thread, const JSHandle<Program> &patchProgram)
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
        UnloadPatch(thread, patchFile_->GetJSPandaFileDesc());
        LOG_FULL(ERROR) << "execute patch main has exception";
        return false;
    }
    return true;
}

bool QuickFixLoader::ExecutePatchMain(JSThread *thread, const CVector<JSHandle<Program>> &programs)
{
    for (const auto &item : programs) {
        if (!ExecutePatchMain(thread, item)) {
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
    return JSPandaFile::ParseEntryPoint(desc);
}

bool QuickFixLoader::UnloadPatch(JSThread *thread, const CString &patchFileName)
{
    if (patchFile_ == nullptr || patchFileName != patchFile_->GetJSPandaFileDesc()) {
        LOG_FULL(ERROR) << "patch file is null or has not been loaded";
        return false;
    }

    if (reservedBaseMethodInfo_.empty() && reservedBaseClassInfo_.empty()) {
        LOG_FULL(ERROR) << "no method need to unload";
        return false;
    }

    EcmaVM *vm = thread->GetEcmaVM();
    JSTaggedValue baseConstpoolValue = vm->FindConstpool(baseFile_, 0);
    if (baseConstpoolValue.IsHole()) {
        LOG_FULL(ERROR) << "base constpool is hole";
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
    }

    for (const auto& item : reservedBaseClassInfo_) {
        uint32_t constpoolIndex = item.first;
        CUnorderedMap<uint32_t, MethodLiteral *> classLiteralInfo = item.second;
        JSHandle<TaggedArray> classLiteral(thread, baseConstpool->GetObjectFromCache(constpoolIndex));

        for (const auto& classItem : classLiteralInfo) {
            MethodLiteral *base = classItem.second;

            JSTaggedValue value = classLiteral->Get(thread, classItem.first);
            ASSERT(value.IsJSFunctionBase());
            JSFunctionBase *func = JSFunctionBase::Cast(value.GetTaggedObject());
            Method *method = Method::Cast(func->GetMethod().GetTaggedObject());

            ReplaceMethodInner(thread, method, base, baseConstpoolValue);
        }
    }

    ClearReservedInfo();
    return true;
}

void QuickFixLoader::ReplaceMethodInner(JSThread *thread,
                                        Method* destMethod,
                                        MethodLiteral *srcMethodLiteral,
                                        JSTaggedValue srcConstpool)
{
    destMethod->SetCallField(srcMethodLiteral->GetCallField());
    destMethod->SetLiteralInfo(srcMethodLiteral->GetLiteralInfo());
    destMethod->SetNativePointerOrBytecodeArray(const_cast<void *>(srcMethodLiteral->GetNativePointer()));
    destMethod->SetConstantPool(thread, srcConstpool);
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
}  // namespace panda::ecmascript
