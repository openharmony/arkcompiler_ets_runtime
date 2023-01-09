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
#include "ecmascript/ts_types/ts_type_parser.h"
#include "libpandafile/code_data_accessor.h"

namespace panda::ecmascript::kungfu {
template<class T, class... Args>
static T *InitializeMemory(T *mem, Args... args)
{
    return new (mem) T(std::forward<Args>(args)...);
}

void BytecodeInfoCollector::ProcessClasses()
{
    ASSERT(jsPandaFile_ != nullptr && jsPandaFile_->GetMethodLiterals() != nullptr);
    MethodLiteral *methods = jsPandaFile_->GetMethodLiterals();
    const panda_file::File *pf = jsPandaFile_->GetPandaFile();
    size_t methodIdx = 0;
    std::map<const uint8_t *, std::pair<size_t, uint32_t>> processedInsns;
    Span<const uint32_t> classIndexes = jsPandaFile_->GetClasses();

    auto &mainMethodIndexes = bytecodeInfo_.GetMainMethodIndexes();
    auto &recordNames = bytecodeInfo_.GetRecordNames();
    auto &methodPcInfos = bytecodeInfo_.GetMethodPcInfos();

    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (pf->IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(*pf, classId);
        CString desc = utf::Mutf8AsCString(cda.GetDescriptor());
        cda.EnumerateMethods([this, methods, &methodIdx, pf, &processedInsns, &desc,
            &mainMethodIndexes, &recordNames, &methodPcInfos] (panda_file::MethodDataAccessor &mda) {
            auto codeId = mda.GetCodeId();
            auto methodId = mda.GetMethodId();
            ASSERT(codeId.has_value());

            // Generate all constpool
            vm_->FindOrCreateConstPool(jsPandaFile_, methodId);

            MethodLiteral *methodLiteral = methods + (methodIdx++);
            panda_file::CodeDataAccessor codeDataAccessor(*pf, codeId.value());
            uint32_t codeSize = codeDataAccessor.GetCodeSize();
            auto methodOffset = methodId.GetOffset();
            CString name = reinterpret_cast<const char *>(pf->GetStringData(mda.GetNameId()).data);
            if (JSPandaFile::IsEntryOrPatch(name)) {
                const CString recordName = jsPandaFile_->ParseEntryPoint(desc);
                jsPandaFile_->UpdateMainMethodIndex(methodOffset, recordName);
                mainMethodIndexes.emplace_back(methodOffset);
                recordNames.emplace_back(recordName);
            }

            InitializeMemory(methodLiteral, jsPandaFile_, methodId);
            methodLiteral->SetHotnessCounter(EcmaInterpreter::GetHotnessCounter(codeSize));
            methodLiteral->Initialize(
                jsPandaFile_, codeDataAccessor.GetNumVregs(), codeDataAccessor.GetNumArgs());
            const uint8_t *insns = codeDataAccessor.GetInstructions();
            ASSERT(jsPandaFile_->IsNewVersion());
            panda_file::IndexAccessor indexAccessor(*pf, methodId);
            panda_file::FunctionKind funcKind = indexAccessor.GetFunctionKind();
            FunctionKind kind = JSPandaFile::GetFunctionKind(funcKind);
            methodLiteral->SetFunctionKind(kind);
            auto it = processedInsns.find(insns);
            if (it == processedInsns.end()) {
                std::vector<std::string> classNameVec;
                CollectMethodPcsFromBC(codeSize, insns, methodLiteral, classNameVec);
                processedInsns[insns] = std::make_pair(methodPcInfos.size() - 1, methodOffset);
                // collect className and literal offset for type infer
                if (EnableCollectLiteralInfo()) {
                    CollectClassLiteralInfo(methodLiteral, classNameVec);
                }
            }

            SetMethodPcInfoIndex(methodOffset, processedInsns[insns]);
            jsPandaFile_->SetMethodLiteralToMap(methodLiteral);
        });
    }
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
            vm_->GetTSManager()->AddElementToClassNameMap(jsPandaFile_, classOffsetVec[i], classNameVec[i]);
        }
    }
}

void BytecodeInfoCollector::IterateLiteral(const MethodLiteral *method,
                                           std::vector<uint32_t> &classOffsetVector)
{
    const panda_file::File *pf = jsPandaFile_->GetPandaFile();
    panda_file::File::EntityId fieldId = method->GetMethodId();
    uint32_t defineMethodOffset = fieldId.GetOffset();
    panda_file::MethodDataAccessor methodDataAccessor(*pf, fieldId);

    methodDataAccessor.EnumerateAnnotations([&](panda_file::File::EntityId annotation_id) {
        panda_file::AnnotationDataAccessor ada(*pf, annotation_id);
        std::string annotationName = std::string(utf::Mutf8AsCString(pf->GetStringData(ada.GetClassId()).data));
        if (annotationName.compare("L_ESTypeAnnotation;") != 0) {
            return;
        }
        uint32_t length = ada.GetCount();
        for (uint32_t i = 0; i < length; i++) {
            panda_file::AnnotationDataAccessor::Elem adae = ada.GetElement(i);
            std::string elemName = std::string(utf::Mutf8AsCString(pf->GetStringData(adae.GetNameId()).data));
            if (elemName.compare("_TypeOfInstruction") != 0) {
                continue;
            }

            panda_file::ScalarValue sv = adae.GetScalarValue();
            panda_file::File::EntityId literalOffset(sv.GetValue());
            JSHandle<TaggedArray> typeOfInstruction =
                LiteralDataExtractor::GetTypeLiteral(vm_->GetJSThread(), jsPandaFile_, literalOffset);
            std::map<int32_t, uint32_t> offsetTypeMap;
            for (uint32_t j = 0; j < typeOfInstruction->GetLength(); j = j + 2) {
                int32_t bcOffset = typeOfInstruction->Get(j).GetInt();
                uint32_t typeOffset = static_cast<uint32_t>(typeOfInstruction->Get(j + 1).GetInt());
                if (classDefBCIndexes_.find(bcOffset) != classDefBCIndexes_.end() ||
                    classDefBCIndexes_.find(bcOffset - 1) != classDefBCIndexes_.end()) { // for getter setter
                    bytecodeInfo_.SetClassTypeOffsetAndDefMethod(typeOffset, defineMethodOffset);
                }
                if (bcOffset != TypeRecorder::METHOD_ANNOTATION_THIS_TYPE_OFFSET &&
                    typeOffset > TSTypeParser::USER_DEFINED_TYPE_OFFSET) {
                    offsetTypeMap.insert(std::make_pair(bcOffset, typeOffset));
                }
            }

            for (auto item : offsetTypeMap) {
                panda_file::File::EntityId offset(item.second);
                JSHandle<TaggedArray> literal =
                    LiteralDataExtractor::GetTypeLiteral(vm_->GetJSThread(), jsPandaFile_, offset);
                int typeKind = literal->Get(0).GetInt();
                if (typeKind == static_cast<int>(TSTypeKind::CLASS)) {
                    classOffsetVector.push_back(item.second);
                }
            }
        }
    });
    classDefBCIndexes_.clear();
}

void BytecodeInfoCollector::CollectMethodPcsFromBC(const uint32_t insSz, const uint8_t *insArr,
                                                   const MethodLiteral *method, std::vector<std::string> &classNameVec)
{
    auto bcIns = BytecodeInst(insArr);
    auto bcInsLast = bcIns.JumpTo(insSz);
    int32_t bcIndex = 0;
    auto &methodPcInfos = bytecodeInfo_.GetMethodPcInfos();
    methodPcInfos.emplace_back(MethodPcInfo { {}, insSz });
    auto &pcOffsets = methodPcInfos.back().pcOffsets;
    const uint8_t *curPc = bcIns.GetAddress();

    while (bcIns.GetAddress() != bcInsLast.GetAddress()) {
        CollectMethodInfoFromBC(bcIns, method, classNameVec, bcIndex);
        CollectConstantPoolIndexInfoFromBC(bcIns, method);
        curPc = bcIns.GetAddress();
        auto nextInst = bcIns.GetNext();
        bcIns = nextInst;
        pcOffsets.emplace_back(curPc);
        bcIndex++;
    }
}

void BytecodeInfoCollector::SetMethodPcInfoIndex(uint32_t methodOffset,
                                                 const std::pair<size_t, uint32_t> &processedMethodInfo)
{
    auto processedMethodPcInfoIndex = processedMethodInfo.first;
    auto processedMethodOffset = processedMethodInfo.second;
    uint32_t numOfLexVars = 0;
    LexicalEnvStatus status = LexicalEnvStatus::VIRTUAL_LEXENV;
    auto &methodList = bytecodeInfo_.GetMethodList();
    // Methods with the same instructions in abc files have the same static information. Since
    // information from bytecodes is collected only once, methods other than the processed method
    // will obtain static information from the processed method.
    auto processedIter = methodList.find(processedMethodOffset);
    if (processedIter != methodList.end()) {
        const MethodInfo &processedMethod = processedIter->second;
        numOfLexVars = processedMethod.GetNumOfLexVars();
        status = processedMethod.GetLexEnvStatus();
    }

    auto iter = methodList.find(methodOffset);
    if (iter != methodList.end()) {
        MethodInfo &methodInfo = iter->second;
        methodInfo.SetMethodPcInfoIndex(processedMethodPcInfoIndex);
        methodInfo.SetNumOfLexVars(numOfLexVars);
        methodInfo.SetLexEnvStatus(status);
        return;
    }
    MethodInfo info(GetMethodInfoID(), processedMethodPcInfoIndex, LexEnv::DEFAULT_ROOT,
        MethodInfo::DEFAULT_OUTMETHOD_OFFSET, numOfLexVars, status);
    methodList.emplace(methodOffset, info);
}

void BytecodeInfoCollector::CollectInnerMethods(const MethodLiteral *method, uint32_t innerMethodOffset)
{
    auto methodId = method->GetMethodId().GetOffset();
    CollectInnerMethods(methodId, innerMethodOffset);
}

void BytecodeInfoCollector::CollectInnerMethods(uint32_t methodId, uint32_t innerMethodOffset)
{
    auto &methodList = bytecodeInfo_.GetMethodList();
    uint32_t methodInfoId = 0;
    auto methodIter = methodList.find(methodId);
    if (methodIter != methodList.end()) {
        MethodInfo &methodInfo = methodIter->second;
        methodInfoId = methodInfo.GetMethodInfoIndex();
        methodInfo.AddInnerMethod(innerMethodOffset);
    } else {
        methodInfoId = GetMethodInfoID();
        MethodInfo info(methodInfoId, 0, LexEnv::DEFAULT_ROOT);
        methodList.emplace(methodId, info);
        methodList.at(methodId).AddInnerMethod(innerMethodOffset);
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
                                                    const MethodLiteral *method, std::vector<std::string> &classNameVec,
                                                    int32_t bcIndex)
{
    const panda_file::File *pf = jsPandaFile_->GetPandaFile();
    if (!(bcIns.HasFlag(BytecodeInstruction::Flags::STRING_ID) &&
        BytecodeInstruction::HasId(BytecodeInstruction::GetFormat(bcIns.GetOpcode()), 0))) {
        BytecodeInstruction::Opcode opcode = static_cast<BytecodeInstruction::Opcode>(bcIns.GetOpcode());
        switch (opcode) {
            uint32_t methodId;
            case BytecodeInstruction::Opcode::DEFINEFUNC_IMM8_ID16_IMM8:
            case BytecodeInstruction::Opcode::DEFINEFUNC_IMM16_ID16_IMM8: {
                methodId = pf->ResolveMethodIndex(method->GetMethodId(),
                                                  static_cast<uint16_t>(bcIns.GetId().AsRawValue())).GetOffset();
                CollectInnerMethods(method, methodId);
                break;
            }
            case BytecodeInstruction::Opcode::DEFINEMETHOD_IMM8_ID16_IMM8:
            case BytecodeInstruction::Opcode::DEFINEMETHOD_IMM16_ID16_IMM8: {
                methodId = pf->ResolveMethodIndex(method->GetMethodId(),
                                                  static_cast<uint16_t>(bcIns.GetId().AsRawValue())).GetOffset();
                CollectInnerMethods(method, methodId);
                break;
            }
            case BytecodeInstruction::Opcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8:{
                auto entityId = pf->ResolveMethodIndex(method->GetMethodId(),
                    (bcIns.GetId <BytecodeInstruction::Format::IMM8_ID16_ID16_IMM16_V8, 0>()).AsRawValue());
                classNameVec.emplace_back(GetClassName(entityId));
                classDefBCIndexes_.insert(bcIndex);
                methodId = entityId.GetOffset();
                CollectInnerMethods(method, methodId);
                auto literalId = pf->ResolveMethodIndex(method->GetMethodId(),
                    (bcIns.GetId <BytecodeInstruction::Format::IMM8_ID16_ID16_IMM16_V8, 1>()).AsRawValue());
                CollectInnerMethodsFromNewLiteral(method, literalId);
                break;
            }
            case BytecodeInstruction::Opcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8: {
                auto entityId = pf->ResolveMethodIndex(method->GetMethodId(),
                    (bcIns.GetId <BytecodeInstruction::Format::IMM16_ID16_ID16_IMM16_V8, 0>()).AsRawValue());
                classNameVec.emplace_back(GetClassName(entityId));
                classDefBCIndexes_.insert(bcIndex);
                methodId = entityId.GetOffset();
                CollectInnerMethods(method, methodId);
                auto literalId = pf->ResolveMethodIndex(method->GetMethodId(),
                    (bcIns.GetId <BytecodeInstruction::Format::IMM16_ID16_ID16_IMM16_V8, 1>()).AsRawValue());
                CollectInnerMethodsFromNewLiteral(method, literalId);
                break;
            }
            case BytecodeInstruction::Opcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
            case BytecodeInstruction::Opcode::CREATEARRAYWITHBUFFER_IMM16_ID16: {
                auto literalId = pf->ResolveMethodIndex(method->GetMethodId(),
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
                auto literalId = pf->ResolveMethodIndex(method->GetMethodId(),
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
            default:
                break;
        }
    }
}

void BytecodeInfoCollector::CollectConstantPoolIndexInfoFromBC(const BytecodeInstruction &bcIns,
                                                               const MethodLiteral *methodliteral)
{
    BytecodeInstruction::Opcode opcode = static_cast<BytecodeInstruction::Opcode>(bcIns.GetOpcode());
    uint32_t methodOffset = methodliteral->GetMethodId().GetOffset();
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
            AddConstantPoolIndexToBCInfo(ConstantPoolInfo::ItemType::STRING, index, methodOffset);
            break;
        }
        case BytecodeInstruction::Opcode::DEFINEFUNC_IMM8_ID16_IMM8:
        case BytecodeInstruction::Opcode::DEFINEFUNC_IMM16_ID16_IMM8:
        case BytecodeInstruction::Opcode::DEFINEMETHOD_IMM8_ID16_IMM8:
        case BytecodeInstruction::Opcode::DEFINEMETHOD_IMM16_ID16_IMM8: {
            auto index = bcIns.GetId().AsRawValue();
            AddConstantPoolIndexToBCInfo(ConstantPoolInfo::ItemType::METHOD, index, methodOffset);
            break;
        }
        case BytecodeInstruction::Opcode::CREATEOBJECTWITHBUFFER_IMM8_ID16:
        case BytecodeInstruction::Opcode::CREATEOBJECTWITHBUFFER_IMM16_ID16: {
            auto index = bcIns.GetId().AsRawValue();
            AddConstantPoolIndexToBCInfo(ConstantPoolInfo::ItemType::OBJECT_LITERAL, index, methodOffset);
            break;
        }
        case BytecodeInstruction::Opcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
        case BytecodeInstruction::Opcode::CREATEARRAYWITHBUFFER_IMM16_ID16: {
            auto index = bcIns.GetId().AsRawValue();
            AddConstantPoolIndexToBCInfo(ConstantPoolInfo::ItemType::ARRAY_LITERAL, index, methodOffset);
            break;
        }
        case BytecodeInstruction::Opcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8: {
            auto methodIndex = (bcIns.GetId <BytecodeInstruction::Format::IMM8_ID16_ID16_IMM16_V8, 0>()).AsRawValue();
            AddConstantPoolIndexToBCInfo(ConstantPoolInfo::ItemType::METHOD, methodIndex, methodOffset);
            auto literalIndex = (bcIns.GetId <BytecodeInstruction::Format::IMM8_ID16_ID16_IMM16_V8, 1>()).AsRawValue();
            AddConstantPoolIndexToBCInfo(ConstantPoolInfo::ItemType::CLASS_LITERAL, literalIndex, methodOffset);
            break;
        }
        case BytecodeInstruction::Opcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8: {
            auto methodIndex = (bcIns.GetId <BytecodeInstruction::Format::IMM16_ID16_ID16_IMM16_V8, 0>()).AsRawValue();
            AddConstantPoolIndexToBCInfo(ConstantPoolInfo::ItemType::METHOD, methodIndex, methodOffset);
            auto literalIndex = (bcIns.GetId <BytecodeInstruction::Format::IMM16_ID16_ID16_IMM16_V8, 1>()).AsRawValue();
            AddConstantPoolIndexToBCInfo(ConstantPoolInfo::ItemType::CLASS_LITERAL, literalIndex, methodOffset);
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

void ConstantPoolInfo::AddIndexToCPItem(ItemType type, uint32_t index, uint32_t methodOffset)
{
    Item &item = GetCPItem(type);
    if (item.find(index) != item.end()) {
        return;
    }
    item.insert({index, ItemData {index, methodOffset, nullptr}});
}
}  // namespace panda::ecmascript::kungfu
