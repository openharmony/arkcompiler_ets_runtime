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
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "libpandafile/class_data_accessor-inl.h"

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
    std::map<const uint8_t *, size_t> processedInsns;
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
                CollectMethodPcsFromBC(codeSize, insns, methodLiteral);
                processedInsns[insns] = methodPcInfos.size() - 1;
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

void BytecodeInfoCollector::CollectMethodPcsFromBC(const uint32_t insSz, const uint8_t *insArr,
                                                   const MethodLiteral *method)
{
    auto bcIns = BytecodeInst(insArr);
    auto bcInsLast = bcIns.JumpTo(insSz);
    auto &methodPcInfos = bytecodeInfo_.GetMethodPcInfos();
    methodPcInfos.emplace_back(MethodPcInfo { {}, insSz });
    auto &pcOffsets = methodPcInfos.back().pcOffsets;
    const uint8_t *curPc = bcIns.GetAddress();

    while (bcIns.GetAddress() != bcInsLast.GetAddress()) {
        CollectMethodInfoFromBC(bcIns, method);
        CollectConstantPoolIndexInfoFromBC(bcIns, method);
        curPc = bcIns.GetAddress();
        auto nextInst = bcIns.GetNext();
        bcIns = nextInst;
        pcOffsets.emplace_back(curPc);
    }
}

void BytecodeInfoCollector::SetMethodPcInfoIndex(uint32_t methodOffset, size_t index)
{
    auto &methodList = bytecodeInfo_.GetMethodList();
    auto iter = methodList.find(methodOffset);
    if (iter != methodList.end()) {
        iter->second.methodPcInfoIndex = index;
        return;
    }
    methodList.emplace(methodOffset, MethodInfo(GetMethodInfoID(), index, 0));
}

void BytecodeInfoCollector::CollectInnerMethods(const MethodLiteral *method, uint32_t innerMethodOffset)
{
    auto methodId = method->GetMethodId().GetOffset();
    CollectInnerMethods(methodId, innerMethodOffset);
}

void BytecodeInfoCollector::CollectInnerMethods(uint32_t methodId, uint32_t innerMethodOffset)
{
    auto &methodList = bytecodeInfo_.GetMethodList();
    size_t methodInfoId = 0;
    auto methodIter = methodList.find(methodId);
    if (methodIter != methodList.end()) {
        methodInfoId = methodIter->second.methodInfoIndex;
        methodIter->second.innerMethods.emplace_back(innerMethodOffset);
    } else {
        methodInfoId = GetMethodInfoID();
        methodList.emplace(methodId, MethodInfo(methodInfoId, 0, 0));
        methodList.at(methodId).innerMethods.emplace_back(innerMethodOffset);
    }

    auto innerMethodIter = methodList.find(innerMethodOffset);
    if (innerMethodIter != methodList.end()) {
        innerMethodIter->second.lexEnv.outmethodId = methodInfoId;
        return;
    }
    methodList.emplace(innerMethodOffset, MethodInfo(GetMethodInfoID(), 0, static_cast<uint32_t>(methodInfoId)));
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
        iter->second.lexEnv.lexVarTypes.resize(numOfLexVars, GateType::AnyType());
        iter->second.lexEnv.status = LexicalEnvStatus::REALITY_LEXENV;
        return;
    }
    methodList.emplace(methodOffset, MethodInfo(GetMethodInfoID(), 0, 0, numOfLexVars,
                                                LexicalEnvStatus::REALITY_LEXENV));
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
                                                    const MethodLiteral *method)
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
                methodId = entityId.GetOffset();
                CollectInnerMethods(method, methodId);
                auto literalId = pf->ResolveMethodIndex(method->GetMethodId(),
                    (bcIns.GetId <BytecodeInstruction::Format::IMM16_ID16_ID16_IMM16_V8, 1>()).AsRawValue());
                CollectInnerMethodsFromNewLiteral(method, literalId);
                break;
            }
            case BytecodeInstruction::Opcode::DEPRECATED_DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8: {
                methodId = pf->ResolveMethodIndex(method->GetMethodId(),
                                                  static_cast<uint16_t>(bcIns.GetId().AsRawValue())).GetOffset();
                auto imm = bcIns.GetImm<BytecodeInstruction::Format::PREF_ID16_IMM16_IMM16_V8_V8>();
                CollectInnerMethods(method, methodId);
                CollectInnerMethodsFromLiteral(method, imm);
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
                                                               const MethodLiteral *method)
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
            AddConstantPoolIndexToBCInfo(ConstantPoolIndexType::STRING, index);
            break;
        }
        case BytecodeInstruction::Opcode::DEFINEFUNC_IMM8_ID16_IMM8:
        case BytecodeInstruction::Opcode::DEFINEFUNC_IMM16_ID16_IMM8:
        case BytecodeInstruction::Opcode::DEFINEMETHOD_IMM8_ID16_IMM8:
        case BytecodeInstruction::Opcode::DEFINEMETHOD_IMM16_ID16_IMM8: {
            auto index = bcIns.GetId().AsRawValue();
            AddConstantPoolIndexToBCInfo(ConstantPoolIndexType::METHOD, index);
            break;
        }
        case BytecodeInstruction::Opcode::CREATEOBJECTWITHBUFFER_IMM8_ID16:
        case BytecodeInstruction::Opcode::CREATEOBJECTWITHBUFFER_IMM16_ID16: {
            auto index = bcIns.GetId().AsRawValue();
            AddConstantPoolIndexToBCInfo(ConstantPoolIndexType::OBJECT_LITERAL, index, methodOffset);
            break;
        }
        case BytecodeInstruction::Opcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
        case BytecodeInstruction::Opcode::CREATEARRAYWITHBUFFER_IMM16_ID16: {
            auto index = bcIns.GetId().AsRawValue();
            AddConstantPoolIndexToBCInfo(ConstantPoolIndexType::ARRAY_LITERAL, index, methodOffset);
            break;
        }
        case BytecodeInstruction::Opcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8: {
            auto methodIndex = (bcIns.GetId <BytecodeInstruction::Format::IMM8_ID16_ID16_IMM16_V8, 0>()).AsRawValue();
            AddConstantPoolIndexToBCInfo(ConstantPoolIndexType::METHOD, methodIndex);
            auto literalIndex = (bcIns.GetId <BytecodeInstruction::Format::IMM8_ID16_ID16_IMM16_V8, 1>()).AsRawValue();
            AddConstantPoolIndexToBCInfo(ConstantPoolIndexType::CLASS_LITERAL, literalIndex, methodOffset);
            break;
        }
        case BytecodeInstruction::Opcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8: {
            auto methodIndex = (bcIns.GetId <BytecodeInstruction::Format::IMM16_ID16_ID16_IMM16_V8, 0>()).AsRawValue();
            AddConstantPoolIndexToBCInfo(ConstantPoolIndexType::METHOD, methodIndex);
            auto literalIndex = (bcIns.GetId <BytecodeInstruction::Format::IMM16_ID16_ID16_IMM16_V8, 1>()).AsRawValue();
            AddConstantPoolIndexToBCInfo(ConstantPoolIndexType::CLASS_LITERAL, literalIndex, methodOffset);
            break;
        }
        default:
            break;
    }
}

LexEnvManager::LexEnvManager(BCInfo &bcInfo)
    : lexEnvs_(bcInfo.GetMethodList().size(), nullptr)
{
    auto &methodList = bcInfo.GetMethodList();
    for (auto &it : methodList) {
        LexEnv *lexEnv = &(it.second.lexEnv);
        lexEnvs_[it.second.methodInfoIndex] = lexEnv;
    }
}

void LexEnvManager::SetLexEnvElementType(uint32_t methodId, uint32_t level, uint32_t slot, const GateType &type)
{
    uint32_t offset = GetTargetLexEnv(methodId, level);
    auto &lexVarTypes = lexEnvs_[offset]->lexVarTypes;
    if (slot < lexVarTypes.size()) {
        lexVarTypes[slot] = type;
    }
}

GateType LexEnvManager::GetLexEnvElementType(uint32_t methodId, uint32_t level, uint32_t slot) const
{
    uint32_t offset = GetTargetLexEnv(methodId, level);
    auto &lexVarTypes = lexEnvs_[offset]->lexVarTypes;
    if (slot < lexVarTypes.size()) {
        return lexVarTypes[slot];
    }
    return GateType::AnyType();
}

uint32_t LexEnvManager::GetTargetLexEnv(uint32_t methodId, uint32_t level) const
{
    auto offset = methodId;
    auto status = lexEnvs_[offset]->status;
    while ((level > 0) || (status != LexicalEnvStatus::REALITY_LEXENV)) {
        offset = lexEnvs_[offset]->outmethodId;
        if (status == LexicalEnvStatus::REALITY_LEXENV && level != 0) {
            --level;
        }
        status = lexEnvs_[offset]->status;
    }
    return offset;
}

void ConstantPoolIndexInfo::AddConstantPoolIndex(ConstantPoolIndexType type, uint32_t index, uint32_t methodOffset)
{
    switch (type) {
        case ConstantPoolIndexType::STRING: {
            stringIndex_.insert(index);
            break;
        }
        case ConstantPoolIndexType::METHOD: {
            methodIndex_.insert(index);
            break;
        }
        case ConstantPoolIndexType::CLASS_LITERAL: {
            classLiteralIndex_.insert(std::make_pair(index, methodOffset));
            break;
        }
        case ConstantPoolIndexType::OBJECT_LITERAL: {
            objectLiteralIndex_.insert(std::make_pair(index, methodOffset));
            break;
        }
        case ConstantPoolIndexType::ARRAY_LITERAL: {
            arrayLiteralIndex_.insert(std::make_pair(index, methodOffset));
            break;
        }
        default:
            UNREACHABLE();
    }
}
}  // namespace panda::ecmascript::kungfu
