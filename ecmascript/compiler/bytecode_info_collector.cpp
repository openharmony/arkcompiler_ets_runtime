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

#include "ecmascript/compiler/bytecode_info_collector.h"

#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/pgo_profiler/pgo_profiler_decoder.h"
#include "libpandafile/code_data_accessor.h"

namespace panda::ecmascript::kungfu {
template<class T, class... Args>
static T *InitializeMemory(T *mem, Args... args)
{
    return new (mem) T(std::forward<Args>(args)...);
}

BytecodeInfoCollector::BytecodeInfoCollector(CompilationEnv *env, JSPandaFile *jsPandaFile,
                                             PGOProfilerDecoder &pfDecoder,
                                             size_t maxAotMethodSize)
    : compilationEnv_(env),
      jsPandaFile_(jsPandaFile),
      bytecodeInfo_(maxAotMethodSize),
      pfDecoder_(pfDecoder),
      snapshotCPData_(new SnapshotConstantPoolData(env->GetEcmaVM(), jsPandaFile, &pfDecoder))
{
    ASSERT(env->IsAotCompiler());
    ProcessClasses();
}

BytecodeInfoCollector::BytecodeInfoCollector(CompilationEnv *env, JSPandaFile *jsPandaFile,
                                             PGOProfilerDecoder &pfDecoder)
    : compilationEnv_(env),
      jsPandaFile_(jsPandaFile),
      // refactor: jit max method size
      bytecodeInfo_(env->GetJSOptions().GetMaxAotMethodSize()),
      pfDecoder_(pfDecoder),
      snapshotCPData_(nullptr) // jit no need
{
    ASSERT(env->IsJitCompiler());
    ProcessCurrMethod();
}

void BytecodeInfoCollector::ProcessClasses()
{
    ASSERT(jsPandaFile_ != nullptr && jsPandaFile_->GetMethodLiterals() != nullptr);
    MethodLiteral *methods = jsPandaFile_->GetMethodLiterals();
    const panda_file::File *pf = jsPandaFile_->GetPandaFile();
    size_t methodIdx = 0;
    std::map<uint32_t, std::pair<size_t, uint32_t>> processedMethod;
    Span<const uint32_t> classIndexes = jsPandaFile_->GetClasses();

    auto &recordNames = bytecodeInfo_.GetRecordNames();
    auto &methodPcInfos = bytecodeInfo_.GetMethodPcInfos();
    std::vector<panda_file::File::EntityId> methodIndexes;
    std::vector<panda_file::File::EntityId> classConstructIndexes;
    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (jsPandaFile_->IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(*pf, classId);
        CString desc = utf::Mutf8AsCString(cda.GetDescriptor());
        const CString recordName = JSPandaFile::ParseEntryPoint(desc);
        cda.EnumerateMethods([this, methods, &methodIdx, pf, &processedMethod,
            &recordNames, &methodPcInfos, &recordName,
            &methodIndexes, &classConstructIndexes] (panda_file::MethodDataAccessor &mda) {
            auto methodId = mda.GetMethodId();
            methodIndexes.emplace_back(methodId);

            // Generate all constpool
            compilationEnv_->FindOrCreateConstPool(jsPandaFile_, methodId);

            auto methodOffset = methodId.GetOffset();
            CString name = reinterpret_cast<const char *>(jsPandaFile_->GetStringData(mda.GetNameId()).data);
            if (JSPandaFile::IsEntryOrPatch(name)) {
                jsPandaFile_->UpdateMainMethodIndex(methodOffset, recordName);
                recordNames.emplace_back(recordName);
            }

            MethodLiteral *methodLiteral = methods + (methodIdx++);
            InitializeMemory(methodLiteral, methodId);
            methodLiteral->Initialize(jsPandaFile_);

            ASSERT(jsPandaFile_->IsNewVersion());
            panda_file::IndexAccessor indexAccessor(*pf, methodId);
            panda_file::FunctionKind funcKind = indexAccessor.GetFunctionKind();
            FunctionKind kind = JSPandaFile::GetFunctionKind(funcKind);
            methodLiteral->SetFunctionKind(kind);

            auto codeId = mda.GetCodeId();
            ASSERT(codeId.has_value());
            panda_file::CodeDataAccessor codeDataAccessor(*pf, codeId.value());
            uint32_t codeSize = codeDataAccessor.GetCodeSize();
            const uint8_t *insns = codeDataAccessor.GetInstructions();
            auto it = processedMethod.find(methodOffset);
            if (it == processedMethod.end()) {
                CollectMethodPcsFromBC(codeSize, insns, methodLiteral,
                    recordName, methodOffset, classConstructIndexes);
                processedMethod[methodOffset] = std::make_pair(methodPcInfos.size() - 1, methodOffset);
            }

            SetMethodPcInfoIndex(methodOffset, processedMethod[methodOffset]);
            jsPandaFile_->SetMethodLiteralToMap(methodLiteral);
            pfDecoder_.MatchAndMarkMethod(jsPandaFile_, recordName, name.c_str(), methodId);
        });
    }
    // class Construct need to use new target, can not fastcall
    for (auto index : classConstructIndexes) {
        MethodLiteral *method = jsPandaFile_->GetMethodLiteralByIndex(index.GetOffset());
        if (method != nullptr) {
            method->SetFunctionKind(FunctionKind::CLASS_CONSTRUCTOR);
        }
    }
    // Collect import(infer-needed) and export relationship among all records.
    CollectRecordReferenceREL();
    RearrangeInnerMethods();
    LOG_COMPILER(INFO) << "Total number of methods in file: "
                       << jsPandaFile_->GetJSPandaFileDesc()
                       << " is: "
                       << methodIdx;
}

void BytecodeInfoCollector::ProcessCurrMethod()
{
    ProcessMethod(compilationEnv_->GetMethodLiteral());
}

void BytecodeInfoCollector::ProcessMethod(MethodLiteral *methodLiteral)
{
    panda_file::File::EntityId methodIdx = methodLiteral->GetMethodId();
    auto methodOffset = methodIdx.GetOffset();
    if (processedMethod_.find(methodOffset) != processedMethod_.end()) {
        return;
    }

    auto &recordNames = bytecodeInfo_.GetRecordNames();
    auto &methodPcInfos = bytecodeInfo_.GetMethodPcInfos();
    const CString recordName = jsPandaFile_->GetRecordNameWithBundlePack(methodIdx);
    recordNames.emplace_back(recordName);
    ASSERT(jsPandaFile_->IsNewVersion());

    const panda_file::File *pf = jsPandaFile_->GetPandaFile();
    panda_file::MethodDataAccessor mda(*pf, methodIdx);
    auto codeId = mda.GetCodeId();
    ASSERT(codeId.has_value());
    panda_file::CodeDataAccessor codeDataAccessor(*pf, codeId.value());
    uint32_t codeSize = codeDataAccessor.GetCodeSize();
    const uint8_t *insns = codeDataAccessor.GetInstructions();
    std::vector<panda_file::File::EntityId> classConstructIndexes;

    CollectMethodPcsFromBC(codeSize, insns, methodLiteral,
        recordName, methodOffset, classConstructIndexes);
    SetMethodPcInfoIndex(methodOffset, {methodPcInfos.size() - 1, methodOffset});
    processedMethod_.emplace(methodOffset);
}

void BytecodeInfoCollector::CollectMethodPcsFromBC(const uint32_t insSz, const uint8_t *insArr,
    MethodLiteral *method, const CString &recordName, uint32_t methodOffset,
    std::vector<panda_file::File::EntityId> &classConstructIndexes)
{
    auto bcIns = BytecodeInst(insArr);
    auto bcInsLast = bcIns.JumpTo(insSz);
    int32_t bcIndex = 0;
    auto &methodPcInfos = bytecodeInfo_.GetMethodPcInfos();
    methodPcInfos.emplace_back(MethodPcInfo { {}, insSz });
    auto &pcOffsets = methodPcInfos.back().pcOffsets;
    const uint8_t *curPc = bcIns.GetAddress();
    bool canFastCall = true;
    bool noGC = true;
    bool debuggerStmt = false;

    while (bcIns.GetAddress() != bcInsLast.GetAddress()) {
        bool fastCallFlag = true;
        CollectMethodInfoFromBC(bcIns, method, bcIndex, classConstructIndexes, &fastCallFlag);
        if (!fastCallFlag) {
            canFastCall = false;
        }
        if (compilationEnv_->IsAotCompiler()) {
            CollectModuleInfoFromBC(bcIns, method, recordName);
        }
        if (snapshotCPData_ != nullptr) {
            snapshotCPData_->Record(bcIns, bcIndex, recordName, method);
        }
        pgoBCInfo_.Record(bcIns, bcIndex, recordName, method);
        if (noGC && !bytecodes_.GetBytecodeMetaData(curPc).IsNoGC()) {
            noGC = false;
        }
        if (!debuggerStmt && bytecodes_.GetBytecodeMetaData(curPc).HasDebuggerStmt()) {
            debuggerStmt = true;
        }
        curPc = bcIns.GetAddress();
        auto nextInst = bcIns.GetNext();
        bcIns = nextInst;
        pcOffsets.emplace_back(curPc);
        bcIndex++;
    }
    bytecodeInfo_.SetMethodOffsetToFastCallInfo(methodOffset, canFastCall, noGC);
    method->SetIsFastCall(canFastCall);
    method->SetNoGCBit(noGC);
    method->SetHasDebuggerStmtBit(debuggerStmt);
}

void BytecodeInfoCollector::SetMethodPcInfoIndex(uint32_t methodOffset,
                                                 const std::pair<size_t, uint32_t> &processedMethodInfo)
{
    auto processedMethodPcInfoIndex = processedMethodInfo.first;
    auto processedMethodOffset = processedMethodInfo.second;
    auto &methodList = bytecodeInfo_.GetMethodList();
    std::set<uint32_t> indexSet{};
    // Methods with the same instructions in abc files have the same static information. Since
    // information from bytecodes is collected only once, methods other than the processed method
    // will obtain static information from the processed method.
    auto processedIter = methodList.find(processedMethodOffset);
    if (processedIter != methodList.end()) {
        const MethodInfo &processedMethod = processedIter->second;
        indexSet = processedMethod.GetImportIndexes();
    }

    auto iter = methodList.find(methodOffset);
    if (iter != methodList.end()) {
        MethodInfo &methodInfo = iter->second;
        methodInfo.SetMethodPcInfoIndex(processedMethodPcInfoIndex);
        // if these methods have the same bytecode, their import indexs must be the same.
        methodInfo.CopyImportIndex(indexSet);
        return;
    }
    MethodInfo info(GetMethodInfoID(), processedMethodPcInfoIndex,
        MethodInfo::DEFAULT_OUTMETHOD_OFFSET);
    info.CopyImportIndex(indexSet);
    methodList.emplace(methodOffset, info);
}

void BytecodeInfoCollector::CollectInnerMethods(const MethodLiteral *method,
                                                uint32_t innerMethodOffset,
                                                bool isConstructor)
{
    auto methodId = method->GetMethodId().GetOffset();
    CollectInnerMethods(methodId, innerMethodOffset, isConstructor);
}

void BytecodeInfoCollector::CollectInnerMethods(uint32_t methodId, uint32_t innerMethodOffset, bool isConstructor)
{
    auto &methodList = bytecodeInfo_.GetMethodList();
    uint32_t methodInfoId = 0;
    auto methodIter = methodList.find(methodId);
    if (methodIter != methodList.end()) {
        MethodInfo &methodInfo = methodIter->second;
        methodInfoId = methodInfo.GetMethodInfoIndex();
        methodInfo.AddInnerMethod(innerMethodOffset, isConstructor);
    } else {
        methodInfoId = GetMethodInfoID();
        MethodInfo info(methodInfoId, 0);
        methodList.emplace(methodId, info);
        methodList.at(methodId).AddInnerMethod(innerMethodOffset, isConstructor);
    }

    auto innerMethodIter = methodList.find(innerMethodOffset);
    if (innerMethodIter != methodList.end()) {
        innerMethodIter->second.SetOutMethodId(methodInfoId);
        innerMethodIter->second.SetOutMethodOffset(methodId);
        return;
    }
    MethodInfo innerInfo(GetMethodInfoID(), 0, methodInfoId, methodId);
    methodList.emplace(innerMethodOffset, innerInfo);
}

void BytecodeInfoCollector::CollectInnerMethodsFromLiteral(const MethodLiteral *method, uint64_t index)
{
    std::vector<uint32_t> methodOffsets;
    LiteralDataExtractor::GetMethodOffsets(jsPandaFile_, index, methodOffsets);
    for (auto methodOffset : methodOffsets) {
        CollectInnerMethods(method, methodOffset);
    }
}

void BytecodeInfoCollector::CollectInnerMethodsFromNewLiteral(const MethodLiteral *method,
                                                              panda_file::File::EntityId literalId)
{
    std::vector<uint32_t> methodOffsets;
    LiteralDataExtractor::GetMethodOffsets(jsPandaFile_, literalId, methodOffsets);
    for (auto methodOffset : methodOffsets) {
        CollectInnerMethods(method, methodOffset);
    }
}

void BytecodeInfoCollector::CollectMethodInfoFromBC(const BytecodeInstruction &bcIns, const MethodLiteral *method,
    int32_t bcIndex, std::vector<panda_file::File::EntityId> &classConstructIndexes, bool *canFastCall)
{
    if (!(bcIns.HasFlag(BytecodeInstruction::Flags::STRING_ID) &&
        BytecodeInstruction::HasId(BytecodeInstruction::GetFormat(bcIns.GetOpcode()), 0))) {
        BytecodeInstruction::Opcode opcode = static_cast<BytecodeInstruction::Opcode>(bcIns.GetOpcode());
        switch (opcode) {
            uint32_t methodId;
            case BytecodeInstruction::Opcode::DEFINEFUNC_IMM8_ID16_IMM8:
            case BytecodeInstruction::Opcode::DEFINEFUNC_IMM16_ID16_IMM8: {
                methodId = jsPandaFile_->ResolveMethodIndex(method->GetMethodId(),
                    static_cast<uint16_t>(bcIns.GetId().AsRawValue())).GetOffset();
                CollectInnerMethods(method, methodId);
                break;
            }
            case BytecodeInstruction::Opcode::DEFINEMETHOD_IMM8_ID16_IMM8:
            case BytecodeInstruction::Opcode::DEFINEMETHOD_IMM16_ID16_IMM8: {
                methodId = jsPandaFile_->ResolveMethodIndex(method->GetMethodId(),
                    static_cast<uint16_t>(bcIns.GetId().AsRawValue())).GetOffset();
                CollectInnerMethods(method, methodId);
                break;
            }
            case BytecodeInstruction::Opcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8:{
                auto entityId = jsPandaFile_->ResolveMethodIndex(method->GetMethodId(),
                    (bcIns.GetId <BytecodeInstruction::Format::IMM8_ID16_ID16_IMM16_V8, 0>()).AsRawValue());
                classConstructIndexes.emplace_back(entityId);
                classDefBCIndexes_.insert(bcIndex);
                methodId = entityId.GetOffset();
                CollectInnerMethods(method, methodId, true);
                auto literalId = jsPandaFile_->ResolveMethodIndex(method->GetMethodId(),
                    (bcIns.GetId <BytecodeInstruction::Format::IMM8_ID16_ID16_IMM16_V8, 1>()).AsRawValue());
                CollectInnerMethodsFromNewLiteral(method, literalId);
                break;
            }
            case BytecodeInstruction::Opcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8: {
                auto entityId = jsPandaFile_->ResolveMethodIndex(method->GetMethodId(),
                    (bcIns.GetId <BytecodeInstruction::Format::IMM16_ID16_ID16_IMM16_V8, 0>()).AsRawValue());
                classConstructIndexes.emplace_back(entityId);
                classDefBCIndexes_.insert(bcIndex);
                methodId = entityId.GetOffset();
                CollectInnerMethods(method, methodId, true);
                auto literalId = jsPandaFile_->ResolveMethodIndex(method->GetMethodId(),
                    (bcIns.GetId <BytecodeInstruction::Format::IMM16_ID16_ID16_IMM16_V8, 1>()).AsRawValue());
                CollectInnerMethodsFromNewLiteral(method, literalId);
                break;
            }
            case BytecodeInstruction::Opcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
            case BytecodeInstruction::Opcode::CREATEARRAYWITHBUFFER_IMM16_ID16: {
                auto literalId = jsPandaFile_->ResolveMethodIndex(method->GetMethodId(),
                    static_cast<uint16_t>(bcIns.GetId().AsRawValue()));
                CollectInnerMethodsFromNewLiteral(method, literalId);
                break;
            }
            case BytecodeInstruction::Opcode::DEPRECATED_CREATEARRAYWITHBUFFER_PREF_IMM16: {
                auto imm = bcIns.GetImm<BytecodeInstruction::Format::PREF_IMM16>();
                CollectInnerMethodsFromLiteral(method, imm);
                break;
            }
            case BytecodeInstruction::Opcode::CREATEOBJECTWITHBUFFER_IMM8_ID16:
            case BytecodeInstruction::Opcode::CREATEOBJECTWITHBUFFER_IMM16_ID16: {
                auto literalId = jsPandaFile_->ResolveMethodIndex(method->GetMethodId(),
                    static_cast<uint16_t>(bcIns.GetId().AsRawValue()));
                CollectInnerMethodsFromNewLiteral(method, literalId);
                break;
            }
            case BytecodeInstruction::Opcode::DEPRECATED_CREATEOBJECTWITHBUFFER_PREF_IMM16: {
                auto imm = bcIns.GetImm<BytecodeInstruction::Format::PREF_IMM16>();
                CollectInnerMethodsFromLiteral(method, imm);
                break;
            }
            case EcmaOpcode::RESUMEGENERATOR:
            case EcmaOpcode::SUSPENDGENERATOR_V8:
            case EcmaOpcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8:
            case EcmaOpcode::WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8:
            case EcmaOpcode::SUPERCALLARROWRANGE_IMM8_IMM8_V8:
            case EcmaOpcode::WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8:
            case EcmaOpcode::SUPERCALLSPREAD_IMM8_V8:
            case EcmaOpcode::GETUNMAPPEDARGS:
            case EcmaOpcode::COPYRESTARGS_IMM8:
            case EcmaOpcode::WIDE_COPYRESTARGS_PREF_IMM16: {
                *canFastCall = false;
                return;
            }
            default:
                break;
        }
    }
}

void BytecodeInfoCollector::CollectModuleInfoFromBC(const BytecodeInstruction &bcIns,
                                                    const MethodLiteral *method,
                                                    const CString &recordName)
{
    auto methodOffset = method->GetMethodId().GetOffset();
    // For records without tsType, we don't need to collect its export info.
    if (jsPandaFile_->HasTSTypes(recordName) && !(bcIns.HasFlag(BytecodeInstruction::Flags::STRING_ID) &&
        BytecodeInstruction::HasId(BytecodeInstruction::GetFormat(bcIns.GetOpcode()), 0))) {
        BytecodeInstruction::Opcode opcode = static_cast<BytecodeInstruction::Opcode>(bcIns.GetOpcode());
        switch (opcode) {
            case BytecodeInstruction::Opcode::STMODULEVAR_IMM8: {
                auto imm = bcIns.GetImm<BytecodeInstruction::Format::IMM8>();
                // The export syntax only exists in main function.
                if (jsPandaFile_->GetMainMethodIndex(recordName) == methodOffset) {
                    CollectExportIndexs(recordName, imm);
                }
                break;
            }
            case BytecodeInstruction::Opcode::WIDE_STMODULEVAR_PREF_IMM16: {
                auto imm = bcIns.GetImm<BytecodeInstruction::Format::PREF_IMM16>();
                if (jsPandaFile_->GetMainMethodIndex(recordName) == methodOffset) {
                    CollectExportIndexs(recordName, imm);
                }
                break;
            }
            case BytecodeInstruction::Opcode::LDEXTERNALMODULEVAR_IMM8:{
                auto imm = bcIns.GetImm<BytecodeInstruction::Format::IMM8>();
                CollectImportIndexs(methodOffset, imm);
                break;
            }
            case BytecodeInstruction::Opcode::WIDE_LDEXTERNALMODULEVAR_PREF_IMM16:{
                auto imm = bcIns.GetImm<BytecodeInstruction::Format::PREF_IMM16>();
                CollectImportIndexs(methodOffset, imm);
                break;
            }
            default:
                break;
        }
    }
}

void BytecodeInfoCollector::CollectImportIndexs(uint32_t methodOffset, uint32_t index)
{
    auto &methodList = bytecodeInfo_.GetMethodList();
    auto iter = methodList.find(methodOffset);
    if (iter != methodList.end()) {
        MethodInfo &methodInfo = iter->second;
        // Collect import indexs of each method in its MethodInfo to do accurate Pgo compilation analysis.
        methodInfo.AddImportIndex(index);
        return;
    }
    MethodInfo info(GetMethodInfoID(), 0);
    info.AddImportIndex(index);
    methodList.emplace(methodOffset, info);
}

void BytecodeInfoCollector::CollectExportIndexs(const CString &recordName, uint32_t index)
{
    ASSERT(compilationEnv_->IsAotCompiler());
    JSThread *thread = compilationEnv_->GetJSThread();
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    CString exportLocalName = "*default*";
    ObjectFactory *objFactory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    JSHandle<SourceTextModule> currentModule = moduleManager->HostGetImportedModule(recordName);
    if (currentModule->GetLocalExportEntries().IsUndefined()) {
        return;
    }
    // localExportEntries contain all local element info exported in this record.
    JSHandle<TaggedArray> localExportArray(thread, currentModule->GetLocalExportEntries());
    ASSERT(index < localExportArray->GetLength());
    JSHandle<LocalExportEntry> currentExportEntry(thread, localExportArray->Get(index));
    JSHandle<JSTaggedValue> exportName(thread, currentExportEntry->GetExportName());
    JSHandle<JSTaggedValue> localName(thread, currentExportEntry->GetLocalName());

    JSHandle<JSTaggedValue> exportLocalNameHandle =
        JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(exportLocalName));
    JSHandle<JSTaggedValue> defaultName = thread->GlobalConstants()->GetHandledDefaultString();
    /* if current exportName is "default", but localName not "*default*" like "export default class A{},
     * localName is A, exportName is default in exportEntry". this will be recorded as "A:classType" in
     * exportTable in typeSystem. At this situation, we will use localName to judge whether it has a actual
     * Type record. Otherwise, we will use exportName.
     */
    if (JSTaggedValue::SameValue(exportName, defaultName) &&
        !JSTaggedValue::SameValue(localName, exportLocalNameHandle)) {
        exportName = localName;
    }

    bytecodeInfo_.AddExportIndexToRecord(recordName, index);
}

void BytecodeInfoCollector::CollectRecordReferenceREL()
{
    auto &recordNames = bytecodeInfo_.GetRecordNames();
    for (auto &record : recordNames) {
        JSRecordInfo info = jsPandaFile_->FindRecordInfo(record);
        if (jsPandaFile_->HasTSTypes(info) && jsPandaFile_->IsModule(info)) {
            CollectRecordImportInfo(record);
            CollectRecordExportInfo(record);
        }
    }
}

/* Each import index is corresponded to a ResolvedIndexBinding in the Environment of its module.
 * Through ResolvedIndexBinding, we can get the export module and its export index. Only when the
 * export index is in the non-type-record set which we have collected in CollectExportIndexs function,
 * this export element can be infer-needed. We will collect the map as (key: import index , value: (exportRecord,
 * exportIndex)) for using in pgo analysis and type infer.
 */
void BytecodeInfoCollector::CollectRecordImportInfo(const CString &recordName)
{
    ASSERT(compilationEnv_->IsAotCompiler());
    auto thread = compilationEnv_->GetJSThread();
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    JSHandle<SourceTextModule> currentModule = moduleManager->HostGetImportedModule(recordName);
    // Collect Import Info
    JSTaggedValue moduleEnvironment = currentModule->GetEnvironment();
    if (moduleEnvironment.IsUndefined()) {
        return;
    }
    ASSERT(moduleEnvironment.IsTaggedArray());
    JSHandle<TaggedArray> moduleArray(thread, moduleEnvironment);
    auto length = moduleArray->GetLength();
    for (size_t index = 0; index < length; index++) {
        JSTaggedValue resolvedBinding = moduleArray->Get(index);
        // if resolvedBinding.IsHole(), means that importname is * or it belongs to empty Aot module.
        if (!resolvedBinding.IsResolvedIndexBinding()) {
            continue;
        }
        ResolvedIndexBinding *binding = ResolvedIndexBinding::Cast(resolvedBinding.GetTaggedObject());
        CString resolvedRecord = ModuleManager::GetRecordName(binding->GetModule());
        auto bindingIndex = binding->GetIndex();
        if (bytecodeInfo_.HasExportIndexToRecord(resolvedRecord, bindingIndex)) {
            bytecodeInfo_.AddImportRecordInfoToRecord(recordName, resolvedRecord, index, bindingIndex);
        }
    }
}

// For type infer under retranmission (export * from "xxx"), we collect the star export records in this function.
void BytecodeInfoCollector::CollectRecordExportInfo(const CString &recordName)
{
    ASSERT(compilationEnv_->IsAotCompiler());
    auto thread = compilationEnv_->GetJSThread();
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    JSHandle<SourceTextModule> currentModule = moduleManager->HostGetImportedModule(recordName);
    // Collect Star Export Info
    JSTaggedValue starEntries = currentModule->GetStarExportEntries();
    if (starEntries.IsUndefined()) {
        return;
    }
    ASSERT(starEntries.IsTaggedArray());
    JSHandle<TaggedArray> starEntriesArray(thread, starEntries);
    auto starLength = starEntriesArray->GetLength();
    JSMutableHandle<StarExportEntry> starExportEntry(thread, JSTaggedValue::Undefined());
    for (size_t index = 0; index < starLength; index++) {
        starExportEntry.Update(starEntriesArray->Get(index));
        JSTaggedValue moduleRequest = starExportEntry->GetModuleRequest();
        CString moduleRequestName = ConvertToString(EcmaString::Cast(moduleRequest.GetTaggedObject()));
        if (ModulePathHelper::IsNativeModuleRequest(moduleRequestName)) {
            return;
        }
        CString baseFileName = jsPandaFile_->GetJSPandaFileDesc();
        CString entryPoint = ModulePathHelper::ConcatFileNameWithMerge(thread, jsPandaFile_,
            baseFileName, recordName, moduleRequestName);
        if (jsPandaFile_->HasTypeSummaryOffset(entryPoint)) {
            bytecodeInfo_.AddStarExportToRecord(recordName, entryPoint);
        }
    }
}

void BytecodeInfoCollector::RearrangeInnerMethods()
{
    auto &methodList = bytecodeInfo_.GetMethodList();
    for (auto &it : methodList) {
        MethodInfo &methodInfo = it.second;
        methodInfo.RearrangeInnerMethods();
    }
}
}  // namespace panda::ecmascript::kungfu
