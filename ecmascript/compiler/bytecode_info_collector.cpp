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

#include "ecmascript/compiler/type_recorder.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jspandafile/type_literal_extractor.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/pgo_profiler/pgo_profiler_decoder.h"
#include "ecmascript/ts_types/ts_type_parser.h"
#include "libpandafile/code_data_accessor.h"

namespace panda::ecmascript::kungfu {
template<class T, class... Args>
static T *InitializeMemory(T *mem, Args... args)
{
    return new (mem) T(std::forward<Args>(args)...);
}

BytecodeInfoCollector::BytecodeInfoCollector(EcmaVM *vm, JSPandaFile *jsPandaFile, PGOProfilerDecoder &pfDecoder,
                                             size_t maxAotMethodSize, bool enableCollectLiteralInfo)
    : vm_(vm),
      jsPandaFile_(jsPandaFile),
      bytecodeInfo_(maxAotMethodSize, jsPandaFile),
      pfDecoder_(pfDecoder),
      enableCollectLiteralInfo_(enableCollectLiteralInfo)
{
    vm_->GetJSThread()->GetCurrentEcmaContext()->GetTSManager()->SetBytecodeInfoCollector(this);
    ProcessClasses();
    ProcessEnvs();
}

BytecodeInfoCollector::~BytecodeInfoCollector()
{
    if (envManager_ != nullptr) {
        delete envManager_;
        envManager_ = nullptr;
    }
    auto tsManager = vm_->GetJSThread()->GetCurrentEcmaContext()->GetTSManager();
    tsManager->PrintTypeInfo(jsPandaFile_);
    tsManager->SetBytecodeInfoCollector(nullptr);
}

void BytecodeInfoCollector::ProcessEnvs()
{
    if (envManager_ == nullptr) {
        envManager_ = new LexEnvManager(bytecodeInfo_);
    }
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
            CollectFunctionTypeId(methodId);

            // Generate all constpool
            vm_->GetJSThread()->GetCurrentEcmaContext()->FindOrCreateConstPool(jsPandaFile_, methodId);

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
                std::vector<std::string> classNameVec;
                CollectMethodPcsFromBC(codeSize, insns, methodLiteral, classNameVec,
                    recordName, methodOffset, classConstructIndexes);
                processedMethod[methodOffset] = std::make_pair(methodPcInfos.size() - 1, methodOffset);
                // collect className and literal offset for type infer
                if (EnableCollectLiteralInfo()) {
                    CollectClassLiteralInfo(methodLiteral, classNameVec);
                }
            }

            SetMethodPcInfoIndex(methodOffset, processedMethod[methodOffset]);
            jsPandaFile_->SetMethodLiteralToMap(methodLiteral);
            pfDecoder_.MatchAndMarkMethod(recordName, name.c_str(), methodId);
        });
    }
    // class Construct need to use new target, can not fastcall
    for (auto index : classConstructIndexes) {
        MethodLiteral *method = jsPandaFile_->GetMethodLiteralByIndex(index.GetOffset());
        if (method != nullptr) {
            method->SetFunctionKind(FunctionKind::CLASS_CONSTRUCTOR);
            method->SetIsFastCall(false);
            bytecodeInfo_.ModifyMethodOffsetToCanFastCall(index.GetOffset(), false);
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

void BytecodeInfoCollector::CollectClassLiteralInfo(const MethodLiteral *method,
                                                    const std::vector<std::string> &classNameVec)
{
    std::vector<uint32_t> classOffsetVec;
    IterateLiteral(method, classOffsetVec);

    if (classOffsetVec.size() == classNameVec.size()) {
        for (uint32_t i = 0; i < classOffsetVec.size(); i++) {
            vm_->GetJSThread()->GetCurrentEcmaContext()->GetTSManager()->AddElementToClassNameMap(
                jsPandaFile_, classOffsetVec[i], classNameVec[i]);
        }
    }
}

void BytecodeInfoCollector::CollectFunctionTypeId(panda_file::File::EntityId fieldId)
{
    uint32_t offset = fieldId.GetOffset();
    TypeAnnotationExtractor annoExtractor(jsPandaFile_, offset);
    uint32_t typeId = annoExtractor.GetMethodTypeOffset();
    if (typeId != 0) {
        bytecodeInfo_.SetFunctionTypeIDAndMethodOffset(typeId, offset);
    }
    if (annoExtractor.IsNamespace()) {
        MarkMethodNamespace(offset);
    }
}

void BytecodeInfoCollector::CollectInnerFuncType(const MethodLiteral *method, uint32_t innerMethodId, int32_t bcIndex)
{
    auto &methodList = bytecodeInfo_.GetMethodList();
    auto methodId = method->GetMethodId().GetOffset();
    auto methodIter = methodList.find(methodId);
    if (methodIter == methodList.end()) {
        return;
    }
    TypeAnnotationExtractor annoExtractor(jsPandaFile_, innerMethodId);
    uint32_t innerFuncType = annoExtractor.GetMethodTypeOffset();
    if (innerFuncType != 0) {
        methodIter->second.AddBcToTypeId(bcIndex, innerFuncType);
    }
}

void BytecodeInfoCollector::IterateLiteral(const MethodLiteral *method,
                                           std::vector<uint32_t> &classOffsetVector)
{
    panda_file::File::EntityId fieldId = method->GetMethodId();
    uint32_t defineMethodOffset = fieldId.GetOffset();
    TypeAnnotationExtractor annoExtractor(jsPandaFile_, defineMethodOffset);
    std::map<int32_t, uint32_t> offsetTypeMap;
    annoExtractor.EnumerateInstsAndTypes(
        [this, &offsetTypeMap, defineMethodOffset](const int32_t bcOffset, const uint32_t typeOffset) {
            if (classDefBCIndexes_.find(bcOffset) != classDefBCIndexes_.end() ||
                classDefBCIndexes_.find(bcOffset - 1) != classDefBCIndexes_.end()) { // for getter setter
                bytecodeInfo_.SetClassTypeOffsetAndDefMethod(typeOffset, defineMethodOffset);
            }
            if (bcOffset != TypeRecorder::METHOD_ANNOTATION_THIS_TYPE_OFFSET &&
                TSTypeParser::IsUserDefinedType(typeOffset)) {
                offsetTypeMap.insert(std::make_pair(bcOffset, typeOffset));
            }
        });

    for (auto item : offsetTypeMap) {
        uint32_t typeOffset = item.second;
        StoreClassTypeOffset(typeOffset, classOffsetVector);
    }

    classDefBCIndexes_.clear();
}

void BytecodeInfoCollector::StoreClassTypeOffset(const uint32_t typeOffset, std::vector<uint32_t> &classOffsetVector)
{
    TypeLiteralExtractor typeLiteralExtractor(jsPandaFile_, typeOffset);
    if (typeLiteralExtractor.GetTypeKind() != TSTypeKind::CLASS) {
        return;
    }

    if (classOffsetVector.empty() || typeOffset != classOffsetVector.back()) {
        classOffsetVector.emplace_back(typeOffset);
    }
}

void BytecodeInfoCollector::CollectMethodPcsFromBC(const uint32_t insSz, const uint8_t *insArr,
    MethodLiteral *method, std::vector<std::string> &classNameVec, const CString &recordName,
    uint32_t methodOffset, std::vector<panda_file::File::EntityId> &classConstructIndexes)
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

    while (bcIns.GetAddress() != bcInsLast.GetAddress()) {
        bool fastCallFlag = true;
        CollectMethodInfoFromBC(bcIns, method, classNameVec, bcIndex, classConstructIndexes, &fastCallFlag);
        if (!fastCallFlag) {
            canFastCall = false;
        }
        CollectModuleInfoFromBC(bcIns, method, recordName);
        CollectConstantPoolIndexInfoFromBC(bcIns, method, bcIndex);
        pgoBCInfo_.Record(bcIns, bcIndex, recordName, method);
        if (noGC && !bytecodes_.GetBytecodeMetaData(curPc).IsNoGC()) {
            noGC = false;
        }
        curPc = bcIns.GetAddress();
        auto nextInst = bcIns.GetNext();
        bcIns = nextInst;
        pcOffsets.emplace_back(curPc);
        bcIndex++;
    }
    pcOffsets.emplace_back(bcInsLast.GetAddress());
    bytecodeInfo_.SetMethodOffsetToFastCallInfo(methodOffset, canFastCall, noGC);
    method->SetIsFastCall(canFastCall);
    method->SetNoGCBit(noGC);
}

void BytecodeInfoCollector::SetMethodPcInfoIndex(uint32_t methodOffset,
                                                 const std::pair<size_t, uint32_t> &processedMethodInfo)
{
    auto processedMethodPcInfoIndex = processedMethodInfo.first;
    auto processedMethodOffset = processedMethodInfo.second;
    uint32_t numOfLexVars = 0;
    LexicalEnvStatus status = LexicalEnvStatus::VIRTUAL_LEXENV;
    auto &methodList = bytecodeInfo_.GetMethodList();
    std::set<uint32_t> indexSet{};
    // Methods with the same instructions in abc files have the same static information. Since
    // information from bytecodes is collected only once, methods other than the processed method
    // will obtain static information from the processed method.
    auto processedIter = methodList.find(processedMethodOffset);
    if (processedIter != methodList.end()) {
        const MethodInfo &processedMethod = processedIter->second;
        numOfLexVars = processedMethod.GetNumOfLexVars();
        status = processedMethod.GetLexEnvStatus();
        indexSet = processedMethod.GetImportIndexes();
    }

    auto iter = methodList.find(methodOffset);
    if (iter != methodList.end()) {
        MethodInfo &methodInfo = iter->second;
        methodInfo.SetMethodPcInfoIndex(processedMethodPcInfoIndex);
        methodInfo.SetNumOfLexVars(numOfLexVars);
        methodInfo.SetLexEnvStatus(status);
        // if these methods have the same bytecode, their import indexs must be the same.
        methodInfo.CopyImportIndex(indexSet);
        return;
    }
    MethodInfo info(GetMethodInfoID(), processedMethodPcInfoIndex, LexEnv::DEFAULT_ROOT,
        MethodInfo::DEFAULT_OUTMETHOD_OFFSET, numOfLexVars, status);
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
        MethodInfo info(methodInfoId, 0, LexEnv::DEFAULT_ROOT);
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

void BytecodeInfoCollector::MarkMethodNamespace(const uint32_t methodOffset)
{
    auto &methodList = bytecodeInfo_.GetMethodList();
    auto iter = methodList.find(methodOffset);
    if (iter != methodList.end()) {
        MethodInfo &methodInfo = iter->second;
        methodInfo.MarkMethodNamespace();
        return;
    }
    MethodInfo info(GetMethodInfoID(), 0, LexEnv::DEFAULT_ROOT, MethodInfo::DEFAULT_OUTMETHOD_OFFSET,
        0, LexicalEnvStatus::VIRTUAL_LEXENV, true);
    methodList.emplace(methodOffset, info);
}

void BytecodeInfoCollector::CollectInnerMethodsFromLiteral(const MethodLiteral *method, uint64_t index)
{
    std::vector<uint32_t> methodOffsets;
    LiteralDataExtractor::GetMethodOffsets(jsPandaFile_, index, methodOffsets);
    for (auto methodOffset : methodOffsets) {
        CollectInnerMethods(method, methodOffset);
    }
}

void BytecodeInfoCollector::NewLexEnvWithSize(const MethodLiteral *method, uint64_t numOfLexVars)
{
    auto &methodList = bytecodeInfo_.GetMethodList();
    auto methodOffset = method->GetMethodId().GetOffset();
    auto iter = methodList.find(methodOffset);
    if (iter != methodList.end()) {
        MethodInfo &methodInfo = iter->second;
        methodInfo.SetNumOfLexVars(numOfLexVars);
        methodInfo.SetLexEnvStatus(LexicalEnvStatus::REALITY_LEXENV);
        return;
    }
    MethodInfo info(GetMethodInfoID(), 0, LexEnv::DEFAULT_ROOT, MethodInfo::DEFAULT_OUTMETHOD_OFFSET,
        numOfLexVars, LexicalEnvStatus::REALITY_LEXENV);
    methodList.emplace(methodOffset, info);
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

void BytecodeInfoCollector::CollectMethodInfoFromBC(const BytecodeInstruction &bcIns,
    const MethodLiteral *method, std::vector<std::string> &classNameVec, int32_t bcIndex,
    std::vector<panda_file::File::EntityId> &classConstructIndexes, bool *canFastCall)
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
                CollectInnerFuncType(method, methodId, bcIndex);
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
                classNameVec.emplace_back(GetClassName(entityId));
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
                classNameVec.emplace_back(GetClassName(entityId));
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
            case BytecodeInstruction::Opcode::NEWLEXENV_IMM8: {
                auto imm = bcIns.GetImm<BytecodeInstruction::Format::IMM8>();
                NewLexEnvWithSize(method, imm);
                break;
            }
            case BytecodeInstruction::Opcode::NEWLEXENVWITHNAME_IMM8_ID16: {
                auto imm = bcIns.GetImm<BytecodeInstruction::Format::IMM8_ID16>();
                NewLexEnvWithSize(method, imm);
                break;
            }
            case BytecodeInstruction::Opcode::WIDE_NEWLEXENV_PREF_IMM16: {
                auto imm = bcIns.GetImm<BytecodeInstruction::Format::PREF_IMM16>();
                NewLexEnvWithSize(method, imm);
                break;
            }
            case BytecodeInstruction::Opcode::WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16: {
                auto imm = bcIns.GetImm<BytecodeInstruction::Format::PREF_IMM16_ID16>();
                NewLexEnvWithSize(method, imm);
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
    MethodInfo info(GetMethodInfoID(), 0, LexEnv::DEFAULT_ROOT);
    info.AddImportIndex(index);
    methodList.emplace(methodOffset, info);
}

void BytecodeInfoCollector::CollectExportIndexs(const CString &recordName, uint32_t index)
{
    JSThread *thread = vm_->GetJSThread();
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    CString exportLocalName = "*default*";
    ObjectFactory *objFactory = vm_->GetFactory();
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

    JSHandle<EcmaString> exportNameStr(thread, EcmaString::Cast(exportName->GetTaggedObject()));
    // In order to reduce redundant compilation, when a export element satisfies one of the following conditions,
    // it will be added to the ExportRecordInfo of this record.
    // 1) its name is not recorded in exportTypeTable, or
    // 2) its type is classType or any.
    if (!CheckExportNameAndClassType(recordName, exportNameStr)) {
        bytecodeInfo_.AddExportIndexToRecord(recordName, index);
    }
}

bool BytecodeInfoCollector::CheckExportNameAndClassType(const CString &recordName,
                                                        const JSHandle<EcmaString> &exportStr)
{
    auto tsManager = vm_->GetJSThread()->GetCurrentEcmaContext()->GetTSManager();
    JSHandle<TaggedArray> exportTypeTable = tsManager->GetExportTableFromLiteral(jsPandaFile_, recordName);
    uint32_t length = exportTypeTable->GetLength();
    for (uint32_t i = 0; i < length; i = i + 2) { // 2: skip a pair of key and value
        EcmaString *valueString = EcmaString::Cast(exportTypeTable->Get(i).GetTaggedObject());
        if (!EcmaStringAccessor::StringsAreEqual(*exportStr, valueString)) {
            continue;
        }
        uint32_t typeId = static_cast<uint32_t>(exportTypeTable->Get(i + 1).GetInt());
        if (TSTypeParser::IsUserDefinedType(typeId)) {
            TypeLiteralExtractor typeExtractor(jsPandaFile_, typeId);
            if (typeExtractor.GetTypeKind() == TSTypeKind::CLASS) {
                return false;
            }
        }
        if (typeId != 0) {
            return true;
        }
    }
    return false;
}

void BytecodeInfoCollector::CollectRecordReferenceREL()
{
    auto &recordNames = bytecodeInfo_.GetRecordNames();
    for (auto &record : recordNames) {
        JSRecordInfo info = jsPandaFile_->FindRecordInfo(record);
        if (jsPandaFile_->HasTSTypes(info)|| jsPandaFile_->IsModule(info)) {
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
    auto thread = vm_->GetJSThread();
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
        CString resolvedRecord = SourceTextModule::GetRecordName(binding->GetModule());
        auto bindingIndex = binding->GetIndex();
        if (bytecodeInfo_.HasExportIndexToRecord(resolvedRecord, bindingIndex)) {
            bytecodeInfo_.AddImportRecordInfoToRecord(recordName, resolvedRecord, index, bindingIndex);
        }
    }
}

// For type infer under retranmission (export * from "xxx"), we collect the star export records in this function.
void BytecodeInfoCollector::CollectRecordExportInfo(const CString &recordName)
{
    auto thread = vm_->GetJSThread();
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

void BytecodeInfoCollector::CollectConstantPoolIndexInfoFromBC(const BytecodeInstruction &bcIns,
                                                               const MethodLiteral *method, uint32_t bcIndex)
{
    BytecodeInstruction::Opcode opcode = static_cast<BytecodeInstruction::Opcode>(bcIns.GetOpcode());
    uint32_t methodOffset = method->GetMethodId().GetOffset();
    switch (opcode) {
        case BytecodeInstruction::Opcode::LDA_STR_ID16:
        case BytecodeInstruction::Opcode::STOWNBYNAME_IMM8_ID16_V8:
        case BytecodeInstruction::Opcode::STOWNBYNAME_IMM16_ID16_V8:
        case BytecodeInstruction::Opcode::CREATEREGEXPWITHLITERAL_IMM8_ID16_IMM8:
        case BytecodeInstruction::Opcode::CREATEREGEXPWITHLITERAL_IMM16_ID16_IMM8:
        case BytecodeInstruction::Opcode::STCONSTTOGLOBALRECORD_IMM16_ID16:
        case BytecodeInstruction::Opcode::TRYLDGLOBALBYNAME_IMM8_ID16:
        case BytecodeInstruction::Opcode::TRYLDGLOBALBYNAME_IMM16_ID16:
        case BytecodeInstruction::Opcode::TRYSTGLOBALBYNAME_IMM8_ID16:
        case BytecodeInstruction::Opcode::TRYSTGLOBALBYNAME_IMM16_ID16:
        case BytecodeInstruction::Opcode::STTOGLOBALRECORD_IMM16_ID16:
        case BytecodeInstruction::Opcode::STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8:
        case BytecodeInstruction::Opcode::STOWNBYNAMEWITHNAMESET_IMM16_ID16_V8:
        case BytecodeInstruction::Opcode::LDTHISBYNAME_IMM8_ID16:
        case BytecodeInstruction::Opcode::LDTHISBYNAME_IMM16_ID16:
        case BytecodeInstruction::Opcode::STTHISBYNAME_IMM8_ID16:
        case BytecodeInstruction::Opcode::STTHISBYNAME_IMM16_ID16:
        case BytecodeInstruction::Opcode::LDGLOBALVAR_IMM16_ID16:
        case BytecodeInstruction::Opcode::LDOBJBYNAME_IMM8_ID16:
        case BytecodeInstruction::Opcode::LDOBJBYNAME_IMM16_ID16:
        case BytecodeInstruction::Opcode::STOBJBYNAME_IMM8_ID16_V8:
        case BytecodeInstruction::Opcode::STOBJBYNAME_IMM16_ID16_V8:
        case BytecodeInstruction::Opcode::LDSUPERBYNAME_IMM8_ID16:
        case BytecodeInstruction::Opcode::LDSUPERBYNAME_IMM16_ID16:
        case BytecodeInstruction::Opcode::STSUPERBYNAME_IMM8_ID16_V8:
        case BytecodeInstruction::Opcode::STSUPERBYNAME_IMM16_ID16_V8:
        case BytecodeInstruction::Opcode::STGLOBALVAR_IMM16_ID16:
        case BytecodeInstruction::Opcode::LDBIGINT_ID16: {
            auto index = bcIns.GetId().AsRawValue();
            AddConstantPoolIndexToBCInfo(ConstantPoolInfo::ItemType::STRING, index, methodOffset, bcIndex);
            break;
        }
        case BytecodeInstruction::Opcode::DEFINEFUNC_IMM8_ID16_IMM8:
        case BytecodeInstruction::Opcode::DEFINEFUNC_IMM16_ID16_IMM8:
        case BytecodeInstruction::Opcode::DEFINEMETHOD_IMM8_ID16_IMM8:
        case BytecodeInstruction::Opcode::DEFINEMETHOD_IMM16_ID16_IMM8: {
            auto index = bcIns.GetId().AsRawValue();
            AddConstantPoolIndexToBCInfo(ConstantPoolInfo::ItemType::METHOD, index, methodOffset, bcIndex);
            break;
        }
        case BytecodeInstruction::Opcode::CREATEOBJECTWITHBUFFER_IMM8_ID16:
        case BytecodeInstruction::Opcode::CREATEOBJECTWITHBUFFER_IMM16_ID16: {
            auto index = bcIns.GetId().AsRawValue();
            AddConstantPoolIndexToBCInfo(ConstantPoolInfo::ItemType::OBJECT_LITERAL, index, methodOffset, bcIndex);
            break;
        }
        case BytecodeInstruction::Opcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
        case BytecodeInstruction::Opcode::CREATEARRAYWITHBUFFER_IMM16_ID16: {
            auto index = bcIns.GetId().AsRawValue();
            AddConstantPoolIndexToBCInfo(ConstantPoolInfo::ItemType::ARRAY_LITERAL, index, methodOffset, bcIndex);
            break;
        }
        case BytecodeInstruction::Opcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8: {
            auto methodIndex = (bcIns.GetId <BytecodeInstruction::Format::IMM8_ID16_ID16_IMM16_V8, 0>()).AsRawValue();
            AddConstantPoolIndexToBCInfo(ConstantPoolInfo::ItemType::METHOD, methodIndex, methodOffset, bcIndex);
            auto literalIndex = (bcIns.GetId <BytecodeInstruction::Format::IMM8_ID16_ID16_IMM16_V8, 1>()).AsRawValue();
            AddConstantPoolIndexToBCInfo(ConstantPoolInfo::ItemType::CLASS_LITERAL, literalIndex,
                                         methodOffset, bcIndex);
            break;
        }
        case BytecodeInstruction::Opcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8: {
            auto methodIndex = (bcIns.GetId <BytecodeInstruction::Format::IMM16_ID16_ID16_IMM16_V8, 0>()).AsRawValue();
            AddConstantPoolIndexToBCInfo(ConstantPoolInfo::ItemType::METHOD, methodIndex, methodOffset, bcIndex);
            auto literalIndex = (bcIns.GetId <BytecodeInstruction::Format::IMM16_ID16_ID16_IMM16_V8, 1>()).AsRawValue();
            AddConstantPoolIndexToBCInfo(ConstantPoolInfo::ItemType::CLASS_LITERAL, literalIndex,
                                         methodOffset, bcIndex);
            break;
        }
        default:
            break;
    }
}

LexEnvManager::LexEnvManager(BCInfo &bcInfo)
    : lexEnvs_(bcInfo.GetMethodList().size())
{
    const auto &methodList = bcInfo.GetMethodList();
    for (const auto &it : methodList) {
        const MethodInfo &methodInfo = it.second;
        lexEnvs_[methodInfo.GetMethodInfoIndex()].Inilialize(methodInfo.GetOutMethodId(),
                                                             methodInfo.GetNumOfLexVars(),
                                                             methodInfo.GetLexEnvStatus());
    }
}

void LexEnvManager::SetLexEnvElementType(uint32_t methodId, uint32_t level, uint32_t slot, const GateType &type)
{
    uint32_t offset = GetTargetLexEnv(methodId, level);
    lexEnvs_[offset].SetLexVarType(slot, type);
}

GateType LexEnvManager::GetLexEnvElementType(uint32_t methodId, uint32_t level, uint32_t slot) const
{
    uint32_t offset = GetTargetLexEnv(methodId, level);
    return lexEnvs_[offset].GetLexVarType(slot);
}

uint32_t LexEnvManager::GetTargetLexEnv(uint32_t methodId, uint32_t level) const
{
    auto offset = methodId;
    auto status = GetLexEnvStatus(offset);
    while (!HasDefaultRoot(offset) && ((level > 0) || (status != LexicalEnvStatus::REALITY_LEXENV))) {
        offset = GetOutMethodId(offset);
        if (HasDefaultRoot(offset)) {
            break;
        }
        if (status == LexicalEnvStatus::REALITY_LEXENV && level != 0) {
            --level;
        }
        status = GetLexEnvStatus(offset);
    }
    return offset;
}

uint64_t ConstantPoolInfo::GetItemKey(uint32_t index, uint32_t methodOffset)
{
    panda_file::IndexAccessor indexAccessor(*jsPandaFile_->GetPandaFile(),
                                             panda_file::File::EntityId(methodOffset));
    uint64_t result = 0;
    result = static_cast<uint32_t>(indexAccessor.GetHeaderIndex());
    result = result << CONSTPOOL_MASK;
    result |= index;
    return result;
}

void ConstantPoolInfo::AddIndexToCPItem(ItemType type, uint32_t index, uint32_t methodOffset, uint32_t bcIndex)
{
    uint64_t key = GetItemKey(index, methodOffset);
    Item &item = GetCPItem(type);
    if (item.find(key) != item.end()) {
        return;
    }
    item.insert({key, ItemData {index, methodOffset, nullptr, bcIndex}});
}
}  // namespace panda::ecmascript::kungfu
