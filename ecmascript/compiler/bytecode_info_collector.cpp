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
#include "ecmascript/compiler/ecma_bytecode_des.h"
#include "libpandafile/class_data_accessor-inl.h"

namespace panda::ecmascript::kungfu {
const JSPandaFile *BytecodeInfoCollector::LoadInfoFromPf(const CString &filename, const std::string_view &entryPoint,
                                                         std::vector<MethodPcInfo>& methodPcInfos)
{
    JSPandaFileManager *jsPandaFileManager = JSPandaFileManager::GetInstance();
    JSPandaFile *jsPandaFile = jsPandaFileManager->OpenJSPandaFile(filename);
    if (jsPandaFile == nullptr) {
        LOG_ECMA(ERROR) << "open file " << filename << " error";
        return nullptr;
    }

    JSPandaFileManager::GetInstance()->InsertJSPandaFile(jsPandaFile);

    CString methodName;
    auto pos = entryPoint.find_last_of("::");
    if (pos != std::string_view::npos) {
        methodName = entryPoint.substr(pos + 1);
    } else {
        // default use func_main_0 as entryPoint
        methodName = JSPandaFile::ENTRY_FUNCTION_NAME;
    }

    ProcessClasses(jsPandaFile, methodName, methodPcInfos);

    return jsPandaFile;
}

void BytecodeInfoCollector::ProcessClasses(JSPandaFile *jsPandaFile, const CString &methodName,
                                           std::vector<MethodPcInfo> &methodPcInfos)
{
    ASSERT(jsPandaFile != nullptr && jsPandaFile->GetMethodLiterals() != nullptr);
    MethodLiteral *methods = jsPandaFile->GetMethodLiterals();
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    size_t methodIdx = 0;
    panda_file::File::StringData sd = {static_cast<uint32_t>(methodName.size()),
                                       reinterpret_cast<const uint8_t *>(methodName.c_str())};
    std::map<const uint8_t *, size_t> processedInsns;
    Span<const uint32_t> classIndexes = jsPandaFile->GetClasses();

    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (pf->IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(*pf, classId);
        cda.EnumerateMethods([methods, &methodIdx, pf, &processedInsns, jsPandaFile, &methodPcInfos, &sd, methodName]
            (panda_file::MethodDataAccessor &mda) {
            auto codeId = mda.GetCodeId();
            ASSERT(codeId.has_value());

            MethodLiteral *methodLiteral = methods + (methodIdx++);
            panda_file::CodeDataAccessor codeDataAccessor(*pf, codeId.value());
            uint32_t codeSize = codeDataAccessor.GetCodeSize();

            uint32_t mainMethodIndex = jsPandaFile->GetMainMethodIndex();
            if (mainMethodIndex == 0 && pf->GetStringData(mda.GetNameId()) == sd) {
                jsPandaFile->UpdateMainMethodIndex(mda.GetMethodId().GetOffset(), methodName);
            }

            new (methodLiteral) MethodLiteral(jsPandaFile, mda.GetMethodId());
            methodLiteral->SetHotnessCounter(EcmaInterpreter::GetHotnessCounter(codeSize));
            methodLiteral->InitializeCallField(
                jsPandaFile, codeDataAccessor.GetNumVregs(), codeDataAccessor.GetNumArgs());
            const uint8_t *insns = codeDataAccessor.GetInstructions();
            auto it = processedInsns.find(insns);
            if (it == processedInsns.end()) {
                CollectMethodPcs(jsPandaFile, codeSize, insns, methodLiteral, methodPcInfos);
                processedInsns[insns] = methodPcInfos.size() - 1;
            } else {
                methodPcInfos[it->second].methods.emplace_back(methodLiteral);
            }
            jsPandaFile->SetMethodLiteralToMap(methodLiteral);
        });
    }
    LOG_COMPILER(INFO) << "Total number of methods in file: " << jsPandaFile->GetJSPandaFileDesc()
        << " is: " << methodIdx;
}

void BytecodeInfoCollector::CollectMethodPcs(JSPandaFile *jsPandaFile, const uint32_t insSz, const uint8_t *insArr,
                                             const MethodLiteral* method, std::vector<MethodPcInfo> &methodPcInfos)
{
    bool isNewVersion = jsPandaFile->IsNewVersion();
    auto bcIns = OldBytecodeInst(insArr);
    auto bcInsLast = bcIns.JumpTo(insSz);

    methodPcInfos.emplace_back(MethodPcInfo { std::vector<const MethodLiteral *>(1, method), {}, {}, {} });

    int32_t offsetIndex = 1;
    uint8_t *curPc = nullptr;
    uint8_t *prePc = nullptr;
    while (bcIns.GetAddress() != bcInsLast.GetAddress()) {
        auto pc = const_cast<uint8_t *>(bcIns.GetAddress());
        auto nextInst = bcIns.GetNext();
        if (!isNewVersion) {
            TranslateBCIns(jsPandaFile, bcIns, method);
            FixOpcode(bcIns);
            UpdateEcmaBytecodeICOffset(const_cast<MethodLiteral *>(method), pc);
        }
        bcIns = nextInst;
        auto &bytecodeBlockInfos = methodPcInfos.back().bytecodeBlockInfos;
        auto &byteCodeCurPrePc = methodPcInfos.back().byteCodeCurPrePc;
        auto &pcToBCOffset = methodPcInfos.back().pcToBCOffset;

        if (offsetIndex == 1) {
            curPc = prePc = pc;
            bytecodeBlockInfos.emplace_back(curPc, SplitKind::START, std::vector<uint8_t *>(1, curPc));
            byteCodeCurPrePc[curPc] = prePc;
            pcToBCOffset[curPc] = offsetIndex++;
        } else {
            curPc = pc;
            byteCodeCurPrePc[curPc] = prePc;
            pcToBCOffset[curPc] = offsetIndex++;
            prePc = curPc;
            BytecodeCircuitBuilder::CollectBytecodeBlockInfo(curPc, bytecodeBlockInfos);
        }
    }
    auto emptyPc = const_cast<uint8_t *>(bcInsLast.GetAddress());
    methodPcInfos.back().byteCodeCurPrePc[emptyPc] = prePc;
    methodPcInfos.back().pcToBCOffset[emptyPc] = offsetIndex++;
}

#define ADD_NOP_INST(pc, oldLen, newOpcode)                              \
do {                                                                     \
    int newLen = static_cast<int>(BytecodeInstruction::Size(newOpcode)); \
    int paddingSize = static_cast<int>(oldLen) - newLen;                 \
    for (int i = 0; i < paddingSize; i++) {                              \
        *(pc + newLen + i) = static_cast<uint8_t>(EcmaOpcode::NOP);      \
    }                                                                    \
} while (false)

void BytecodeInfoCollector::FixOpcode(const OldBytecodeInst &inst)
{
    auto opcode = inst.GetOpcode();
    auto pc = const_cast<uint8_t *>(inst.GetAddress());
    switch (opcode) {
        case OldBytecodeInst::Opcode::MOV_V4_V4:
            *pc = static_cast<uint8_t>(EcmaBytecode::MOV_V4_V4);
            break;
        case OldBytecodeInst::Opcode::MOV_DYN_V8_V8:
            *pc = static_cast<uint8_t>(EcmaBytecode::MOV_DYN_V8_V8);
            break;
        case OldBytecodeInst::Opcode::MOV_DYN_V16_V16:
            *pc = static_cast<uint8_t>(EcmaBytecode::MOV_DYN_V16_V16);
            break;
        case OldBytecodeInst::Opcode::LDA_STR_ID32:
            *pc = static_cast<uint8_t>(EcmaBytecode::LDA_STR_ID32);
            break;
        case OldBytecodeInst::Opcode::JMP_IMM8:
            *pc = static_cast<uint8_t>(EcmaBytecode::JMP_IMM8);
            break;
        case OldBytecodeInst::Opcode::JMP_IMM16:
            *pc = static_cast<uint8_t>(EcmaBytecode::JMP_IMM16);
            break;
        case OldBytecodeInst::Opcode::JMP_IMM32:
            *pc = static_cast<uint8_t>(EcmaBytecode::JMP_IMM32);
            break;
        case OldBytecodeInst::Opcode::JEQZ_IMM8:
            *pc = static_cast<uint8_t>(EcmaBytecode::JEQZ_IMM8);
            break;
        case OldBytecodeInst::Opcode::JEQZ_IMM16:
            *pc = static_cast<uint8_t>(EcmaBytecode::JEQZ_IMM16);
            break;
        case OldBytecodeInst::Opcode::JNEZ_IMM8:
            *pc = static_cast<uint8_t>(EcmaBytecode::JNEZ_IMM8);
            break;
        case OldBytecodeInst::Opcode::JNEZ_IMM16:
            *pc = static_cast<uint8_t>(EcmaBytecode::JNEZ_IMM16);
            break;
        case OldBytecodeInst::Opcode::LDA_DYN_V8:
            *pc = static_cast<uint8_t>(EcmaBytecode::LDA_DYN_V8);
            break;
        case OldBytecodeInst::Opcode::STA_DYN_V8:
            *pc = static_cast<uint8_t>(EcmaBytecode::STA_DYN_V8);
            break;
        case OldBytecodeInst::Opcode::LDAI_DYN_IMM32:
            *pc = static_cast<uint8_t>(EcmaBytecode::LDAI_DYN_IMM32);
            break;
        case OldBytecodeInst::Opcode::FLDAI_DYN_IMM64:
            *pc = static_cast<uint8_t>(EcmaBytecode::FLDAI_DYN_IMM64);
            break;
        case OldBytecodeInst::Opcode::RETURN_DYN:
            *pc = static_cast<uint8_t>(EcmaBytecode::RETURN_DYN);
            break;
        case OldBytecodeInst::Opcode::ECMA_DEFINEASYNCFUNC_PREF_ID16_IMM16_V8:
            U_FALLTHROUGH;
        case OldBytecodeInst::Opcode::ECMA_DEFINEGENERATORFUNC_PREF_ID16_IMM16_V8:
            U_FALLTHROUGH;
        case OldBytecodeInst::Opcode::ECMA_DEFINENCFUNCDYN_PREF_ID16_IMM16_V8:
            U_FALLTHROUGH;
        case OldBytecodeInst::Opcode::ECMA_DEFINEFUNCDYN_PREF_ID16_IMM16_V8:
            U_FALLTHROUGH;
        case OldBytecodeInst::Opcode::ECMA_DEFINEASYNCGENERATORFUNC_PREF_ID16_IMM16_V8:
            *pc = static_cast<uint8_t>(EcmaBytecode::DEFINEFUNCDYN_PREF_ID16_IMM16_V8);
            *(pc + 1) = 0xFF;
            break;
        default: {
            if (*pc != static_cast<uint8_t>(OldBytecodeInst::Opcode::ECMA_LDNAN_PREF_NONE)) {
                LOG_FULL(FATAL) << "Is not an Ecma Opcode opcode: " << static_cast<uint16_t>(opcode);
                UNREACHABLE();
            }
            *pc = *(pc + 1);
            *(pc + 1) = 0xFF;
            break;
        }
    }
}

void BytecodeInfoCollector::UpdateEcmaBytecodeICOffset(MethodLiteral* method, uint8_t *pc)
{
    uint8_t offset = MethodLiteral::INVALID_IC_SLOT;
    auto opcode = static_cast<EcmaBytecode>(*pc);
    switch (opcode) {
        case EcmaBytecode::TRYLDGLOBALBYNAME_PREF_ID32:
        case EcmaBytecode::TRYSTGLOBALBYNAME_PREF_ID32:
        case EcmaBytecode::LDGLOBALVAR_PREF_ID32:
        case EcmaBytecode::STGLOBALVAR_PREF_ID32:
            offset = method->UpdateSlotSizeWith8Bit(1);
            break;
        case EcmaBytecode::LDOBJBYVALUE_PREF_V8_V8:
        case EcmaBytecode::STOBJBYVALUE_PREF_V8_V8:
        case EcmaBytecode::STOWNBYVALUE_PREF_V8_V8:
        case EcmaBytecode::LDOBJBYNAME_PREF_ID32_V8:
        case EcmaBytecode::STOBJBYNAME_PREF_ID32_V8:
        case EcmaBytecode::STOWNBYNAME_PREF_ID32_V8:
        case EcmaBytecode::LDOBJBYINDEX_PREF_V8_IMM32:
        case EcmaBytecode::STOBJBYINDEX_PREF_V8_IMM32:
        case EcmaBytecode::STOWNBYINDEX_PREF_V8_IMM32:
        case EcmaBytecode::LDSUPERBYVALUE_PREF_V8_V8:
        case EcmaBytecode::STSUPERBYVALUE_PREF_V8_V8:
        case EcmaBytecode::LDSUPERBYNAME_PREF_ID32_V8:
        case EcmaBytecode::STSUPERBYNAME_PREF_ID32_V8:
        case EcmaBytecode::LDMODULEVAR_PREF_ID32_IMM8:
        case EcmaBytecode::STMODULEVAR_PREF_ID32:
            offset = method->UpdateSlotSizeWith8Bit(2); // 2: occupy two ic slot
            break;
        default:
            return;
    }

    *(pc + 1) = offset;
}

void BytecodeInfoCollector::FixOpcode(MethodLiteral *method, const OldBytecodeInst &inst)
{
    auto opcode = inst.GetOpcode();
    EcmaOpcode newOpcode;
    auto oldLen = OldBytecodeInst::Size(OldBytecodeInst::GetFormat(opcode));
    auto pc = const_cast<uint8_t *>(inst.GetAddress());

    // First level opcode
    if (static_cast<uint16_t>(opcode) < 236) {  // 236: second level bytecode index
        switch (opcode) {
            case OldBytecodeInst::Opcode::MOV_V4_V4: {
                *pc = static_cast<uint8_t>(EcmaOpcode::MOV_V4_V4);
                break;
            }
            case OldBytecodeInst::Opcode::MOV_DYN_V8_V8: {
                *pc = static_cast<uint8_t>(EcmaOpcode::MOV_V8_V8);
                break;
            }
            case OldBytecodeInst::Opcode::MOV_DYN_V16_V16: {
                *pc = static_cast<uint8_t>(EcmaOpcode::MOV_V16_V16);
                break;
            }
            case OldBytecodeInst::Opcode::LDA_STR_ID32: {
                newOpcode = EcmaOpcode::LDA_STR_ID16;
                uint32_t id = inst.GetId();
                LOG_ECMA_IF(id > std::numeric_limits<uint16_t>::max(), FATAL) << "Cannot translate to 16 bits: " << id;
                *pc = static_cast<uint8_t>(newOpcode);
                uint16_t newId = static_cast<uint16_t>(id);
                if (memcpy_s(pc + 1, sizeof(uint16_t), &newId, sizeof(uint16_t)) != EOK) {
                    LOG_FULL(FATAL) << "FixOpcode memcpy_s fail";
                    UNREACHABLE();
                }
                ADD_NOP_INST(pc, oldLen, newOpcode);
                break;
            }
            case OldBytecodeInst::Opcode::JMP_IMM8: {
                *pc = static_cast<uint8_t>(EcmaOpcode::JMP_IMM8);
                break;
            }
            case OldBytecodeInst::Opcode::JMP_IMM16: {
                *pc = static_cast<uint8_t>(EcmaOpcode::JMP_IMM16);
                break;
            }
            case OldBytecodeInst::Opcode::JMP_IMM32: {
                *pc = static_cast<uint8_t>(EcmaOpcode::JMP_IMM32);
                break;
            }
            case OldBytecodeInst::Opcode::JEQZ_IMM8: {
                *pc = static_cast<uint8_t>(EcmaOpcode::JEQZ_IMM8);
                break;
            }
            case OldBytecodeInst::Opcode::JEQZ_IMM16: {
                *pc = static_cast<uint8_t>(EcmaOpcode::JEQZ_IMM16);
                break;
            }
            case OldBytecodeInst::Opcode::JNEZ_IMM8: {
                *pc = static_cast<uint8_t>(EcmaOpcode::JNEZ_IMM8);
                break;
            }
            case OldBytecodeInst::Opcode::JNEZ_IMM16: {
                *pc = static_cast<uint8_t>(EcmaOpcode::JNEZ_IMM16);
                break;
            }
            case OldBytecodeInst::Opcode::LDA_DYN_V8: {
                *pc = static_cast<uint8_t>(EcmaOpcode::LDA_V8);
                break;
            }
            case OldBytecodeInst::Opcode::STA_DYN_V8: {
                *pc = static_cast<uint8_t>(EcmaOpcode::STA_V8);
                break;
            }
            case OldBytecodeInst::Opcode::LDAI_DYN_IMM32: {
                *pc = static_cast<uint8_t>(EcmaOpcode::LDAI_IMM32);
                break;
            }
            case OldBytecodeInst::Opcode::FLDAI_DYN_IMM64: {
                *pc = static_cast<uint8_t>(EcmaOpcode::FLDAI_IMM64);
                break;
            }
            case OldBytecodeInst::Opcode::RETURN_DYN: {
                *pc = static_cast<uint8_t>(EcmaOpcode::RETURN);
                break;
            }
            default:
                LOG_FULL(FATAL) << "FixOpcode fail: " << static_cast<uint32_t>(opcode);
                UNREACHABLE();
        }
        return;
    }

    // New second level bytecode translate
    constexpr uint8_t opShifLen = 8;
    constexpr EcmaOpcode throwPrefOp = EcmaOpcode::THROW_PREF_NONE;
    constexpr EcmaOpcode widePrefOp = EcmaOpcode::WIDE_CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8;
    constexpr EcmaOpcode deprecatedPrefOp = EcmaOpcode::DEPRECATED_LDLEXENV_PREF_NONE;
    switch (opcode) {
        // Translate to throw
        case OldBytecodeInst::Opcode::ECMA_THROWIFSUPERNOTCORRECTCALL_PREF_IMM16: {
            newOpcode = EcmaOpcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM16;
            *pc = static_cast<uint8_t>(throwPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_THROWUNDEFINEDIFHOLE_PREF_V8_V8: {
            newOpcode = EcmaOpcode::THROW_UNDEFINEDIFHOLE_PREF_V8_V8;
            *pc = static_cast<uint8_t>(throwPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_THROWIFNOTOBJECT_PREF_V8: {
            newOpcode = EcmaOpcode::THROW_IFNOTOBJECT_PREF_V8;
            *pc = static_cast<uint8_t>(throwPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_THROWCONSTASSIGNMENT_PREF_V8: {
            newOpcode = EcmaOpcode::THROW_CONSTASSIGNMENT_PREF_V8;
            *pc = static_cast<uint8_t>(throwPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_THROWDELETESUPERPROPERTY_PREF_NONE: {
            newOpcode = EcmaOpcode::THROW_DELETESUPERPROPERTY_PREF_NONE;
            *pc = static_cast<uint8_t>(throwPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_THROWPATTERNNONCOERCIBLE_PREF_NONE: {
            newOpcode = EcmaOpcode::THROW_PATTERNNONCOERCIBLE_PREF_NONE;
            *pc = static_cast<uint8_t>(throwPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_THROWTHROWNOTEXISTS_PREF_NONE: {
            newOpcode = EcmaOpcode::THROW_NOTEXISTS_PREF_NONE;
            *pc = static_cast<uint8_t>(throwPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_THROWDYN_PREF_NONE: {
            newOpcode = EcmaOpcode::THROW_PREF_NONE;
            *pc = static_cast<uint8_t>(throwPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        // Translate to wide
        case OldBytecodeInst::Opcode::ECMA_LDLEXVARDYN_PREF_IMM16_IMM16: {
            newOpcode = EcmaOpcode::WIDE_LDLEXVAR_PREF_IMM16_IMM16;
            *pc = static_cast<uint8_t>(widePrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_COPYRESTARGS_PREF_IMM16: {
            newOpcode = EcmaOpcode::WIDE_COPYRESTARGS_PREF_IMM16;
            *pc = static_cast<uint8_t>(widePrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_STOWNBYINDEX_PREF_V8_IMM32: {
            newOpcode = EcmaOpcode::WIDE_STOWNBYINDEX_PREF_V8_IMM32;
            *pc = static_cast<uint8_t>(widePrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_STOBJBYINDEX_PREF_V8_IMM32: {
            newOpcode = EcmaOpcode::WIDE_STOBJBYINDEX_PREF_V8_IMM32;
            *pc = static_cast<uint8_t>(widePrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_NEWLEXENVWITHNAMEDYN_PREF_IMM16_IMM16: {
            newOpcode = EcmaOpcode::WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16;
            *pc = static_cast<uint8_t>(widePrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_NEWLEXENVDYN_PREF_IMM16: {
            newOpcode = EcmaOpcode::WIDE_NEWLEXENV_PREF_IMM16;
            *pc = static_cast<uint8_t>(widePrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8: {
            newOpcode = EcmaOpcode::WIDE_CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8;
            *pc = static_cast<uint8_t>(widePrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_SUPERCALL_PREF_IMM16_V8: {
            newOpcode = EcmaOpcode::WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8;
            *pc = static_cast<uint8_t>(widePrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LDPATCHVAR_PREF_IMM16: {
            newOpcode = EcmaOpcode::WIDE_LDPATCHVAR_PREF_IMM16;
            *pc = static_cast<uint8_t>(widePrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_STPATCHVAR_PREF_IMM16: {
            newOpcode = EcmaOpcode::WIDE_STPATCHVAR_PREF_IMM16;
            *pc = static_cast<uint8_t>(widePrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        // Translate to deprecated
        case OldBytecodeInst::Opcode::ECMA_STCLASSTOGLOBALRECORD_PREF_ID32: {
            newOpcode = EcmaOpcode::DEPRECATED_STCLASSTOGLOBALRECORD_PREF_ID32;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_STLETTOGLOBALRECORD_PREF_ID32: {
            newOpcode = EcmaOpcode::DEPRECATED_STLETTOGLOBALRECORD_PREF_ID32;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_STCONSTTOGLOBALRECORD_PREF_ID32: {
            newOpcode = EcmaOpcode::DEPRECATED_STCONSTTOGLOBALRECORD_PREF_ID32;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LDMODULEVAR_PREF_ID32_IMM8: {
            newOpcode = EcmaOpcode::DEPRECATED_LDMODULEVAR_PREF_ID32_IMM8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LDSUPERBYNAME_PREF_ID32_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_LDSUPERBYNAME_PREF_ID32_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LDOBJBYNAME_PREF_ID32_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_LDOBJBYNAME_PREF_ID32_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_STMODULEVAR_PREF_ID32: {
            newOpcode = EcmaOpcode::DEPRECATED_STMODULEVAR_PREF_ID32;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_GETMODULENAMESPACE_PREF_ID32: {
            newOpcode = EcmaOpcode::DEPRECATED_GETMODULENAMESPACE_PREF_ID32;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_STLEXVARDYN_PREF_IMM16_IMM16_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_STLEXVAR_PREF_IMM16_IMM16_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_STLEXVARDYN_PREF_IMM8_IMM8_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_STLEXVAR_PREF_IMM8_IMM8_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_STLEXVARDYN_PREF_IMM4_IMM4_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_STLEXVAR_PREF_IMM4_IMM4_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_ASYNCFUNCTIONREJECT_PREF_V8_V8_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_ASYNCFUNCTIONREJECT_PREF_V8_V8_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LDOBJBYINDEX_PREF_V8_IMM32: {
            newOpcode = EcmaOpcode::DEPRECATED_LDOBJBYINDEX_PREF_V8_IMM32;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LDSUPERBYVALUE_PREF_V8_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_LDSUPERBYVALUE_PREF_V8_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LDOBJBYVALUE_PREF_V8_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_LDOBJBYVALUE_PREF_V8_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_SETOBJECTWITHPROTO_PREF_V8_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_SETOBJECTWITHPROTO_PREF_V8_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_COPYDATAPROPERTIES_PREF_V8_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_COPYDATAPROPERTIES_PREF_V8_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_SUSPENDGENERATOR_PREF_V8_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_SUSPENDGENERATOR_PREF_V8_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_DELOBJPROP_PREF_V8_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_DELOBJPROP_PREF_V8_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_GETTEMPLATEOBJECT_PREF_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_GETTEMPLATEOBJECT_PREF_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_GETRESUMEMODE_PREF_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_GETRESUMEMODE_PREF_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_RESUMEGENERATOR_PREF_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_RESUMEGENERATOR_PREF_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_CALLTHISRANGEDYN_PREF_IMM16_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_CALLTHISRANGE_PREF_IMM16_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_CALLSPREADDYN_PREF_V8_V8_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_CALLSPREAD_PREF_V8_V8_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_CALLRANGEDYN_PREF_IMM16_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_CALLRANGE_PREF_IMM16_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_CALLARGS3DYN_PREF_V8_V8_V8_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_CALLARGS3_PREF_V8_V8_V8_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_CALLARGS2DYN_PREF_V8_V8_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_CALLARGS2_PREF_V8_V8_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_CALLARG1DYN_PREF_V8_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_CALLARG1_PREF_V8_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_CALLARG0DYN_PREF_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_CALLARG0_PREF_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_DECDYN_PREF_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_DEC_PREF_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_INCDYN_PREF_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_INC_PREF_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_NOTDYN_PREF_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_NOT_PREF_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_NEGDYN_PREF_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_NEG_PREF_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_TONUMERIC_PREF_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_TONUMERIC_PREF_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_TONUMBER_PREF_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_TONUMBER_PREF_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_CREATEOBJECTWITHBUFFER_PREF_IMM16: {
            newOpcode = EcmaOpcode::DEPRECATED_CREATEOBJECTWITHBUFFER_PREF_IMM16;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_CREATEARRAYWITHBUFFER_PREF_IMM16: {
            newOpcode = EcmaOpcode::DEPRECATED_CREATEARRAYWITHBUFFER_PREF_IMM16;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_GETITERATORNEXT_PREF_V8_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_GETITERATORNEXT_PREF_V8_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_POPLEXENVDYN_PREF_NONE: {
            newOpcode = EcmaOpcode::DEPRECATED_POPLEXENV_PREF_NONE;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LDLEXENVDYN_PREF_NONE: {
            newOpcode = EcmaOpcode::DEPRECATED_LDLEXENV_PREF_NONE;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LDHOMEOBJECT_PREF_NONE: {
            newOpcode = EcmaOpcode::DEPRECATED_LDHOMEOBJECT_PREF_NONE;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_CREATEOBJECTHAVINGMETHOD_PREF_IMM16: {
            newOpcode = EcmaOpcode::DEPRECATED_CREATEOBJECTHAVINGMETHOD_PREF_IMM16;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_DYNAMICIMPORT_PREF_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_DYNAMICIMPORT_PREF_V8;
            *pc = static_cast<uint8_t>(deprecatedPrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;
            break;
        }
        // The same format has IC
        case OldBytecodeInst::Opcode::ECMA_TYPEOFDYN_PREF_NONE: {
            newOpcode = EcmaOpcode::TYPEOF_IMM8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_INSTANCEOFDYN_PREF_V8: {
            newOpcode = EcmaOpcode::INSTANCEOF_IMM8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_CREATEEMPTYARRAY_PREF_NONE: {
            newOpcode = EcmaOpcode::CREATEEMPTYARRAY_IMM8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_GETITERATOR_PREF_NONE: {
            newOpcode = EcmaOpcode::GETITERATOR_IMM8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_ADD2DYN_PREF_V8: {
            newOpcode = EcmaOpcode::ADD2_IMM8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_SUB2DYN_PREF_V8: {
            newOpcode = EcmaOpcode::SUB2_IMM8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_MUL2DYN_PREF_V8: {
            newOpcode = EcmaOpcode::MUL2_IMM8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_DIV2DYN_PREF_V8: {
            newOpcode = EcmaOpcode::DIV2_IMM8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_MOD2DYN_PREF_V8: {
            newOpcode = EcmaOpcode::MOD2_IMM8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_EQDYN_PREF_V8: {
            newOpcode = EcmaOpcode::EQ_IMM8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_NOTEQDYN_PREF_V8: {
            newOpcode = EcmaOpcode::NOTEQ_IMM8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LESSDYN_PREF_V8: {
            newOpcode = EcmaOpcode::LESS_IMM8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LESSEQDYN_PREF_V8: {
            newOpcode = EcmaOpcode::LESSEQ_IMM8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_GREATERDYN_PREF_V8: {
            newOpcode = EcmaOpcode::GREATER_IMM8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_GREATEREQDYN_PREF_V8: {
            newOpcode = EcmaOpcode::GREATEREQ_IMM8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_SHL2DYN_PREF_V8: {
            newOpcode = EcmaOpcode::SHL2_IMM8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_ASHR2DYN_PREF_V8: {
            newOpcode = EcmaOpcode::SHR2_IMM8_V8;  // old instruction was wrong
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_SHR2DYN_PREF_V8: {
            newOpcode = EcmaOpcode::ASHR2_IMM8_V8;  // old instruction was wrong
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_AND2DYN_PREF_V8: {
            newOpcode = EcmaOpcode::AND2_IMM8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_OR2DYN_PREF_V8: {
            newOpcode = EcmaOpcode::OR2_IMM8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_XOR2DYN_PREF_V8: {
            newOpcode = EcmaOpcode::XOR2_IMM8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_EXPDYN_PREF_V8: {
            newOpcode = EcmaOpcode::EXP_IMM8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_ISINDYN_PREF_V8: {
            newOpcode = EcmaOpcode::ISIN_IMM8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_STRICTNOTEQDYN_PREF_V8: {
            newOpcode = EcmaOpcode::STRICTNOTEQ_IMM8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_STRICTEQDYN_PREF_V8: {
            newOpcode = EcmaOpcode::STRICTEQ_IMM8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_ITERNEXT_PREF_V8: {
            LOG_FULL(FATAL) << "Need Add ITERNEXT Deprecated";
            return;
        }
        case OldBytecodeInst::Opcode::ECMA_CLOSEITERATOR_PREF_V8: {
            newOpcode = EcmaOpcode::CLOSEITERATOR_IMM8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_SUPERCALLSPREAD_PREF_V8: {
            newOpcode = EcmaOpcode::SUPERCALLSPREAD_IMM8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_STOBJBYVALUE_PREF_V8_V8: {
            newOpcode = EcmaOpcode::STOBJBYVALUE_IMM8_V8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_STOWNBYVALUE_PREF_V8_V8: {
            newOpcode = EcmaOpcode::STOWNBYVALUE_IMM8_V8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_STSUPERBYVALUE_PREF_V8_V8: {
            newOpcode = EcmaOpcode::STSUPERBYVALUE_IMM8_V8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_STOWNBYVALUEWITHNAMESET_PREF_V8_V8: {
            newOpcode = EcmaOpcode::STOWNBYVALUEWITHNAMESET_IMM8_V8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            break;
        }
        // The same format no IC
        case OldBytecodeInst::Opcode::ECMA_ASYNCFUNCTIONENTER_PREF_NONE: {
            newOpcode = EcmaOpcode::ASYNCFUNCTIONENTER;
            *pc = static_cast<uint8_t>(newOpcode);
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_ASYNCGENERATORRESOLVE_PREF_V8_V8_V8: {
            newOpcode = EcmaOpcode::ASYNCGENERATORRESOLVE_V8_V8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            auto newLen = BytecodeInstruction::Size(newOpcode);
            if (memmove_s(pc + 1, newLen - 1, pc + 2, oldLen - 2) != EOK) {  // 2: skip second level inst and pref
                LOG_FULL(FATAL) << "FixOpcode memmove_s fail";
                UNREACHABLE();
            }
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_CREATEASYNCGENERATOROBJ_PREF_V8: {
            newOpcode = EcmaOpcode::CREATEASYNCGENERATOROBJ_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            auto newLen = BytecodeInstruction::Size(newOpcode);
            if (memmove_s(pc + 1, newLen - 1, pc + 2, oldLen - 2) != EOK) {  // 2: skip second level inst and pref
                LOG_FULL(FATAL) << "FixOpcode memmove_s fail";
                UNREACHABLE();
            }
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_CREATEEMPTYOBJECT_PREF_NONE: {
            newOpcode = EcmaOpcode::CREATEEMPTYOBJECT;
            *pc = static_cast<uint8_t>(newOpcode);
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_CREATEGENERATOROBJ_PREF_V8: {
            newOpcode = EcmaOpcode::CREATEGENERATOROBJ_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            auto newLen = BytecodeInstruction::Size(newOpcode);
            if (memmove_s(pc + 1, newLen - 1, pc + 2, oldLen - 2) != EOK) {  // 2: skip second level inst and pref
                LOG_FULL(FATAL) << "FixOpcode memmove_s fail";
                UNREACHABLE();
            }
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_CREATEITERRESULTOBJ_PREF_V8_V8: {
            newOpcode = EcmaOpcode::CREATEITERRESULTOBJ_V8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            auto newLen = BytecodeInstruction::Size(newOpcode);
            if (memmove_s(pc + 1, newLen - 1, pc + 2, oldLen - 2) != EOK) {  // 2: skip second level inst and pref
                LOG_FULL(FATAL) << "FixOpcode memmove_s fail";
                UNREACHABLE();
            }
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_DEBUGGER_PREF_NONE: {
            newOpcode = EcmaOpcode::DEBUGGER;
            *pc = static_cast<uint8_t>(newOpcode);
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_DEFINEGETTERSETTERBYVALUE_PREF_V8_V8_V8_V8: {
            newOpcode = EcmaOpcode::DEFINEGETTERSETTERBYVALUE_V8_V8_V8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            auto newLen = BytecodeInstruction::Size(newOpcode);
            if (memmove_s(pc + 1, newLen - 1, pc + 2, oldLen - 2) != EOK) {  // 2: skip second level inst and pref
                LOG_FULL(FATAL) << "FixOpcode memmove_s fail";
                UNREACHABLE();
            }
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_GETNEXTPROPNAME_PREF_V8: {
            newOpcode = EcmaOpcode::GETNEXTPROPNAME_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            auto newLen = BytecodeInstruction::Size(newOpcode);
            if (memmove_s(pc + 1, newLen - 1, pc + 2, oldLen - 2) != EOK) {  // 2: skip second level inst and pref
                LOG_FULL(FATAL) << "FixOpcode memmove_s fail";
                UNREACHABLE();
            }
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_GETPROPITERATOR_PREF_NONE: {
            newOpcode = EcmaOpcode::GETPROPITERATOR;
            *pc = static_cast<uint8_t>(newOpcode);
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_GETUNMAPPEDARGS_PREF_NONE: {
            newOpcode = EcmaOpcode::GETUNMAPPEDARGS;
            *pc = static_cast<uint8_t>(newOpcode);
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_ISFALSE_PREF_NONE: {
            newOpcode = EcmaOpcode::ISFALSE;
            *pc = static_cast<uint8_t>(newOpcode);
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_ISTRUE_PREF_NONE: {
            newOpcode = EcmaOpcode::ISTRUE;
            *pc = static_cast<uint8_t>(newOpcode);
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LDFALSE_PREF_NONE: {
            newOpcode = EcmaOpcode::LDFALSE;
            *pc = static_cast<uint8_t>(newOpcode);
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LDTRUE_PREF_NONE: {
            newOpcode = EcmaOpcode::LDTRUE;
            *pc = static_cast<uint8_t>(newOpcode);
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LDFUNCTION_PREF_NONE: {
            newOpcode = EcmaOpcode::LDFUNCTION;
            *pc = static_cast<uint8_t>(newOpcode);
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LDGLOBALTHIS_PREF_NONE: {
            newOpcode = EcmaOpcode::LDGLOBAL;
            *pc = static_cast<uint8_t>(newOpcode);
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LDGLOBAL_PREF_NONE: {
            newOpcode = EcmaOpcode::LDGLOBAL;
            *pc = static_cast<uint8_t>(newOpcode);
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LDHOLE_PREF_NONE: {
            newOpcode = EcmaOpcode::LDHOLE;
            *pc = static_cast<uint8_t>(newOpcode);
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LDNULL_PREF_NONE: {
            newOpcode = EcmaOpcode::LDNULL;
            *pc = static_cast<uint8_t>(newOpcode);
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LDSYMBOL_PREF_NONE: {
            newOpcode = EcmaOpcode::LDSYMBOL;
            *pc = static_cast<uint8_t>(newOpcode);
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LDUNDEFINED_PREF_NONE: {
            newOpcode = EcmaOpcode::LDUNDEFINED;
            *pc = static_cast<uint8_t>(newOpcode);
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LDNAN_PREF_NONE: {
            newOpcode = EcmaOpcode::LDNAN;
            *pc = static_cast<uint8_t>(newOpcode);
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LDINFINITY_PREF_NONE: {
            newOpcode = EcmaOpcode::LDINFINITY;
            *pc = static_cast<uint8_t>(newOpcode);
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_RETURNUNDEFINED_PREF_NONE: {
            newOpcode = EcmaOpcode::RETURNUNDEFINED;
            *pc = static_cast<uint8_t>(newOpcode);
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_NEWOBJDYNRANGE_PREF_IMM16_V8: {
            newOpcode = EcmaOpcode::WIDE_NEWOBJRANGE_PREF_IMM16_V8;
            *pc = static_cast<uint8_t>(widePrefOp);
            *(pc + 1) = static_cast<uint16_t>(newOpcode) >> opShifLen;

            uint16_t imm = static_cast<uint16_t>(inst.GetImm<OldBytecodeInst::Format::PREF_IMM16_V8>() - 1);
            if (memcpy_s(pc + 2, sizeof(uint16_t), &imm, sizeof(uint16_t)) != EOK) {    // 2: skip opcode and ic slot
                LOG_FULL(FATAL) << "FixOpcode memcpy_s fail";
                UNREACHABLE();
            }
            *(pc + 4) = *(pc + 4) + 1;
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LDLEXVARDYN_PREF_IMM4_IMM4: {
            newOpcode = EcmaOpcode::LDLEXVAR_IMM4_IMM4;
            *pc = static_cast<uint8_t>(newOpcode);
            auto newLen = BytecodeInstruction::Size(newOpcode);
            if (memmove_s(pc + 1, newLen - 1, pc + 2, oldLen - 2) != EOK) {  // 2: skip second level inst and pref
                LOG_FULL(FATAL) << "FixOpcode memmove_s fail";
                UNREACHABLE();
            }
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LDLEXVARDYN_PREF_IMM8_IMM8: {
            newOpcode = EcmaOpcode::LDLEXVAR_IMM8_IMM8;
            *pc = static_cast<uint8_t>(newOpcode);
            auto newLen = BytecodeInstruction::Size(newOpcode);
            if (memmove_s(pc + 1, newLen - 1, pc + 2, oldLen - 2) != EOK) {  // 2: skip second level inst and pref
                LOG_FULL(FATAL) << "FixOpcode memmove_s fail";
                UNREACHABLE();
            }
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_STARRAYSPREAD_PREF_V8_V8: {
            newOpcode = EcmaOpcode::STARRAYSPREAD_V8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            auto newLen = BytecodeInstruction::Size(newOpcode);
            // 2: skip opcode and second level pref
            if (memmove_s(pc + 1, newLen - 1, pc + 2, oldLen - 2) != EOK) {
                LOG_FULL(FATAL) << "FixOpcode memmove_s fail";
                UNREACHABLE();
            }
            break;
        }
        // ID32 to ID16 has IC (PREF_ID32)
        case OldBytecodeInst::Opcode::ECMA_TRYLDGLOBALBYNAME_PREF_ID32: {
            newOpcode = EcmaOpcode::TRYLDGLOBALBYNAME_IMM8_ID16;
            uint32_t id = inst.GetId();
            LOG_ECMA_IF(id > std::numeric_limits<uint16_t>::max(), FATAL) << "Cannot translate to 16 bits: " << id;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            uint16_t newId = static_cast<uint16_t>(id);
            if (memcpy_s(pc + 2, sizeof(uint16_t), &newId, sizeof(uint16_t)) != EOK) {    // 2: skip opcode and ic slot
                LOG_FULL(FATAL) << "FixOpcode memcpy_s fail";
                UNREACHABLE();
            }
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_TRYSTGLOBALBYNAME_PREF_ID32: {
            newOpcode = EcmaOpcode::TRYSTGLOBALBYNAME_IMM8_ID16;
            uint32_t id = inst.GetId();
            LOG_ECMA_IF(id > std::numeric_limits<uint16_t>::max(), FATAL) << "Cannot translate to 16 bits: " << id;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            uint16_t newId = static_cast<uint16_t>(id);
            if (memcpy_s(pc + 2, sizeof(uint16_t), &newId, sizeof(uint16_t)) != EOK) {    // 2: skip opcode and ic slot
                LOG_FULL(FATAL) << "FixOpcode memcpy_s fail";
                UNREACHABLE();
            }
            break;
        }
        // ID32 to ID16 has IC (ID32_V8 & ID32_IMM8)
        case OldBytecodeInst::Opcode::ECMA_STOBJBYNAME_PREF_ID32_V8: {
            newOpcode = EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8;
            uint32_t id = inst.GetId();
            LOG_ECMA_IF(id > std::numeric_limits<uint16_t>::max(), FATAL) << "Cannot translate to 16 bits: " << id;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            uint16_t newId = static_cast<uint16_t>(id);
            if (memcpy_s(pc + 2, sizeof(uint16_t), &newId, sizeof(uint16_t)) != EOK) {  // 2: skip opcode and ic slot
                LOG_FULL(FATAL) << "FixOpcode memcpy_s fail";
                UNREACHABLE();
            }
            *(pc + 4) = *(pc + 6);  // 4: index of new opcode; 6: index of old opcode
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_STOWNBYNAME_PREF_ID32_V8: {
            newOpcode = EcmaOpcode::STOWNBYNAME_IMM8_ID16_V8;
            uint32_t id = inst.GetId();
            LOG_ECMA_IF(id > std::numeric_limits<uint16_t>::max(), FATAL) << "Cannot translate to 16 bits: " << id;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            uint16_t newId = static_cast<uint16_t>(id);
            if (memcpy_s(pc + 2, sizeof(uint16_t), &newId, sizeof(uint16_t)) != EOK) {  // 2: skip opcode and ic slot
                LOG_FULL(FATAL) << "FixOpcode memcpy_s fail";
                UNREACHABLE();
            }
            *(pc + 4) = *(pc + 6);  // 4: index of new opcode; 6: index of old opcode
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_STSUPERBYNAME_PREF_ID32_V8: {
            newOpcode = EcmaOpcode::STSUPERBYNAME_IMM8_ID16_V8;
            uint32_t id = inst.GetId();
            LOG_ECMA_IF(id > std::numeric_limits<uint16_t>::max(), FATAL) << "Cannot translate to 16 bits: " << id;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            uint16_t newId = static_cast<uint16_t>(id);
            if (memcpy_s(pc + 2, sizeof(uint16_t), &newId, sizeof(uint16_t)) != EOK) {  // 2: skip opcode and ic slot
                LOG_FULL(FATAL) << "FixOpcode memcpy_s fail";
                UNREACHABLE();
            }
            *(pc + 4) = *(pc + 6);  // 4: index of new opcode; 6: index of old opcode
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_STOWNBYNAMEWITHNAMESET_PREF_ID32_V8: {
            newOpcode = EcmaOpcode::STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8;
            uint32_t id = inst.GetId();
            LOG_ECMA_IF(id > std::numeric_limits<uint16_t>::max(), FATAL) << "Cannot translate to 16 bits: " << id;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            uint16_t newId = static_cast<uint16_t>(id);
            if (memcpy_s(pc + 2, sizeof(uint16_t), &newId, sizeof(uint16_t)) != EOK) {  // 2: skip opcode and ic slot
                LOG_FULL(FATAL) << "FixOpcode memcpy_s fail";
                UNREACHABLE();
            }
            *(pc + 4) = *(pc + 6);  // 4: index of new opcode; 6: index of old opcode
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_CREATEREGEXPWITHLITERAL_PREF_ID32_IMM8: {
            newOpcode = EcmaOpcode::CREATEREGEXPWITHLITERAL_IMM8_ID16_IMM8;
            uint32_t id = inst.GetId();
            LOG_ECMA_IF(id > std::numeric_limits<uint16_t>::max(), FATAL) << "Cannot translate to 16 bits: " << id;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            uint16_t newId = static_cast<uint16_t>(id);
            if (memcpy_s(pc + 2, sizeof(uint16_t), &newId, sizeof(uint16_t)) != EOK) {  // 2: skip opcode and ic slot
                LOG_FULL(FATAL) << "FixOpcode memcpy_s fail";
                UNREACHABLE();
            }
            *(pc + 4) = *(pc + 6);  // 4: index of new opcode; 6: index of old opcode
            break;
        }
        // ID32 to ID16 no IC (PREF_ID32)
        case OldBytecodeInst::Opcode::ECMA_LDBIGINT_PREF_ID32: {
            newOpcode = EcmaOpcode::LDBIGINT_ID16;
            uint32_t id = inst.GetId();
            LOG_ECMA_IF(id > std::numeric_limits<uint16_t>::max(), FATAL) << "Cannot translate to 16 bits: " << id;
            *pc = static_cast<uint8_t>(newOpcode);
            uint16_t newId = static_cast<uint16_t>(id);
            if (memcpy_s(pc + 1, sizeof(uint16_t), &newId, sizeof(uint16_t)) != EOK) {
                LOG_FULL(FATAL) << "FixOpcode memcpy_s fail";
                UNREACHABLE();
            }
            break;
        }
        // Translate to other first level opcode
        case OldBytecodeInst::Opcode::ECMA_NEWOBJSPREADDYN_PREF_V8_V8: {
            newOpcode = EcmaOpcode::NEWOBJAPPLY_IMM8_V8;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            *(pc + 2) = *(pc + 3);  // 2 & 3: skip newtarget, so move vreg1 to vreg0
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_LDGLOBALVAR_PREF_ID32: {
            newOpcode = EcmaOpcode::LDGLOBALVAR_IMM16_ID16;
            uint32_t id = inst.GetId();
            LOG_ECMA_IF(id > std::numeric_limits<uint16_t>::max(), FATAL) << "Cannot translate to 16 bits: " << id;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            *(pc + 2) = 0x00;
            uint16_t newId = static_cast<uint16_t>(id);
            if (memcpy_s(pc + 3, sizeof(uint16_t), &newId, sizeof(uint16_t)) != EOK) {  // 3: offset of id
                LOG_FULL(FATAL) << "FixOpcode memcpy_s fail";
                UNREACHABLE();
            }
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_STGLOBALVAR_PREF_ID32: {
            newOpcode = EcmaOpcode::STGLOBALVAR_IMM16_ID16;
            uint32_t id = inst.GetId();
            LOG_ECMA_IF(id > std::numeric_limits<uint16_t>::max(), FATAL) << "Cannot translate to 16 bits: " << id;
            *pc = static_cast<uint8_t>(newOpcode);
            *(pc + 1) = 0x00;
            *(pc + 2) = 0x00;
            uint16_t newId = static_cast<uint16_t>(id);
            if (memcpy_s(pc + 3, sizeof(uint16_t), &newId, sizeof(uint16_t)) != EOK) {  // 3: offset of id
                LOG_FULL(FATAL) << "FixOpcode memcpy_s fail";
                UNREACHABLE();
            }
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_DEFINEMETHOD_PREF_ID16_IMM16_V8: {
            newOpcode = EcmaOpcode::DEFINEMETHOD_IMM8_ID16_IMM8;
            *pc = static_cast<uint8_t>(newOpcode);
            uint16_t imm = static_cast<uint16_t>(inst.GetImm<OldBytecodeInst::Format::PREF_ID16_IMM16_V8>());
            uint8_t newImm = static_cast<uint8_t>(imm);
            if (memcpy_s(pc + 4, sizeof(uint8_t), &newImm, sizeof(uint8_t)) != EOK) {  // 4: offset of imm
                LOG_FULL(FATAL) << "FixOpcode memcpy_s fail";
                UNREACHABLE();
            }
            break;
        }
        case OldBytecodeInst::Opcode::ECMA_DEFINEASYNCFUNC_PREF_ID16_IMM16_V8:
            U_FALLTHROUGH;
        case OldBytecodeInst::Opcode::ECMA_DEFINEGENERATORFUNC_PREF_ID16_IMM16_V8:
            U_FALLTHROUGH;
        case OldBytecodeInst::Opcode::ECMA_DEFINENCFUNCDYN_PREF_ID16_IMM16_V8:
            U_FALLTHROUGH;
        case OldBytecodeInst::Opcode::ECMA_DEFINEFUNCDYN_PREF_ID16_IMM16_V8:
            U_FALLTHROUGH;
        case OldBytecodeInst::Opcode::ECMA_DEFINEASYNCGENERATORFUNC_PREF_ID16_IMM16_V8: {
            newOpcode = EcmaOpcode::DEFINEFUNC_IMM8_ID16_IMM8;
            *pc = static_cast<uint8_t>(newOpcode);
            uint16_t imm = static_cast<uint16_t>(inst.GetImm<OldBytecodeInst::Format::PREF_ID16_IMM16_V8>());
            uint8_t newImm = static_cast<uint8_t>(imm);
            if (memcpy_s(pc + 4, sizeof(uint8_t), &newImm, sizeof(uint8_t)) != EOK) {  // 4: offset of imm
                LOG_FULL(FATAL) << "FixOpcode memcpy_s fail";
                UNREACHABLE();
            }
            break;
        }
        default:
            LOG_FULL(FATAL) << "Is not an Ecma Opcode opcode: " << static_cast<uint32_t>(opcode);
            UNREACHABLE();
            break;
    }
    ADD_NOP_INST(pc, oldLen, newOpcode);
    UpdateICOffset(method, pc);
}

// reuse prefix 8bits to store slotid
void BytecodeInfoCollector::UpdateICOffset(MethodLiteral* methodLiteral, uint8_t *pc)
{
    uint8_t offset = MethodLiteral::INVALID_IC_SLOT;
    auto opcode = static_cast<EcmaOpcode>(*pc);
    switch (opcode) {
        case EcmaOpcode::TRYLDGLOBALBYNAME_IMM8_ID16:
            U_FALLTHROUGH;
        case EcmaOpcode::TRYSTGLOBALBYNAME_IMM8_ID16:
            U_FALLTHROUGH;
        case EcmaOpcode::LDGLOBALVAR_IMM16_ID16:
            U_FALLTHROUGH;
        case EcmaOpcode::STGLOBALVAR_IMM16_ID16:
            offset = methodLiteral->UpdateSlotSizeWith8Bit(1);
            break;
        case EcmaOpcode::STOBJBYVALUE_IMM8_V8_V8:
            U_FALLTHROUGH;
        case EcmaOpcode::STOWNBYVALUE_IMM8_V8_V8:
            U_FALLTHROUGH;
        case EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8:
            U_FALLTHROUGH;
        case EcmaOpcode::STOWNBYNAME_IMM8_ID16_V8:
            U_FALLTHROUGH;
        case EcmaOpcode::STSUPERBYVALUE_IMM8_V8_V8:
            U_FALLTHROUGH;
        case EcmaOpcode::STSUPERBYNAME_IMM8_ID16_V8:
            offset = methodLiteral->UpdateSlotSizeWith8Bit(2); // 2: occupy two ic slot
            break;
        default:
            return;
    }

    if (opcode == EcmaOpcode::LDGLOBALVAR_IMM16_ID16 || opcode == EcmaOpcode::STGLOBALVAR_IMM16_ID16) {
        uint16_t icSlot = static_cast<uint16_t>(offset);
        if (memcpy_s(pc + 1, sizeof(uint16_t), &icSlot, sizeof(uint16_t)) != EOK) {
            LOG_FULL(FATAL) << "UpdateICOffset memcpy_s fail";
            UNREACHABLE();
        }
    } else {
        *(pc + 1) = offset;
    }
}

void BytecodeInfoCollector::FixInstructionId32(const OldBytecodeInst &inst, uint32_t index, uint32_t fixOrder)
{
    // NOLINTNEXTLINE(hicpp-use-auto)
    auto pc = const_cast<uint8_t *>(inst.GetAddress());
    switch (OldBytecodeInst::GetFormat(inst.GetOpcode())) {
        case OldBytecodeInst::Format::ID32: {
            uint8_t size = sizeof(uint32_t);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (memcpy_s(pc + FixInsIndex::FIX_ONE, size, &index, size) != EOK) {
                LOG_FULL(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
            break;
        }
        case OldBytecodeInst::Format::PREF_ID16_IMM16_V8: {
            uint16_t u16Index = index;
            uint8_t size = sizeof(uint16_t);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (memcpy_s(pc + FixInsIndex::FIX_TWO, size, &u16Index, size) != EOK) {
                LOG_FULL(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
            break;
        }
        case OldBytecodeInst::Format::PREF_ID32:
            // fall through
        case OldBytecodeInst::Format::PREF_ID32_V8:
            // fall through
        case OldBytecodeInst::Format::PREF_ID32_IMM8: {
            uint8_t size = sizeof(uint32_t);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (memcpy_s(pc + FixInsIndex::FIX_TWO, size, &index, size) != EOK) {
                LOG_FULL(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
            break;
        }
        case OldBytecodeInst::Format::PREF_IMM16: {
            ASSERT(static_cast<uint16_t>(index) == index);
            uint16_t u16Index = index;
            uint8_t size = sizeof(uint16_t);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (memcpy_s(pc + FixInsIndex::FIX_TWO, size, &u16Index, size) != EOK) {
                LOG_FULL(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
            break;
        }
        case OldBytecodeInst::Format::PREF_ID16_IMM16_IMM16_V8_V8: {
            // Usually, we fix one part of instruction one time. But as for instruction DefineClassWithBuffer,
            // which use both method id and literal buffer id.Using fixOrder indicates fix Location.
            if (fixOrder == 0) {
                uint8_t size = sizeof(uint16_t);
                ASSERT(static_cast<uint16_t>(index) == index);
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                if (memcpy_s(pc + FixInsIndex::FIX_TWO, size, &index, size) != EOK) {
                    LOG_FULL(FATAL) << "memcpy_s failed";
                    UNREACHABLE();
                }
                break;
            }
            if (fixOrder == 1) {
                ASSERT(static_cast<uint16_t>(index) == index);
                uint16_t u16Index = index;
                uint8_t size = sizeof(uint16_t);
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                if (memcpy_s(pc + FixInsIndex::FIX_FOUR, size, &u16Index, size) != EOK) {
                    LOG_FULL(FATAL) << "memcpy_s failed";
                    UNREACHABLE();
                }
                break;
            }
            break;
        }
        default:
            UNREACHABLE();
    }
}

void BytecodeInfoCollector::TranslateBCIns(JSPandaFile *jsPandaFile, const OldBytecodeInst &bcIns,
                                           const MethodLiteral *method)
{
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    if (bcIns.HasFlag(OldBytecodeInst::Flags::STRING_ID) &&
        OldBytecodeInst::HasId(OldBytecodeInst::GetFormat(bcIns.GetOpcode()), 0)) {
        auto index = jsPandaFile->GetOrInsertConstantPool(
            ConstPoolType::STRING, bcIns.GetId());
        FixInstructionId32(bcIns, index);
    } else {
        OldBytecodeInst::Opcode opcode = static_cast<OldBytecodeInst::Opcode>(bcIns.GetOpcode());
        switch (opcode) {
            uint32_t index;
            uint32_t methodId;
            case OldBytecodeInst::Opcode::ECMA_DEFINEFUNCDYN_PREF_ID16_IMM16_V8:
                methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId()).GetOffset();
                index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::BASE_FUNCTION, methodId);
                FixInstructionId32(bcIns, index);
                break;
            case OldBytecodeInst::Opcode::ECMA_DEFINENCFUNCDYN_PREF_ID16_IMM16_V8:
                methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId()).GetOffset();
                index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::NC_FUNCTION, methodId);
                FixInstructionId32(bcIns, index);
                break;
            case OldBytecodeInst::Opcode::ECMA_DEFINEGENERATORFUNC_PREF_ID16_IMM16_V8:
                methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId()).GetOffset();
                index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::GENERATOR_FUNCTION, methodId);
                FixInstructionId32(bcIns, index);
                break;
            case OldBytecodeInst::Opcode::ECMA_DEFINEASYNCFUNC_PREF_ID16_IMM16_V8:
                methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId()).GetOffset();
                index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::ASYNC_FUNCTION, methodId);
                FixInstructionId32(bcIns, index);
                break;
            case OldBytecodeInst::Opcode::ECMA_DEFINEMETHOD_PREF_ID16_IMM16_V8:
                methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId()).GetOffset();
                index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::METHOD, methodId);
                FixInstructionId32(bcIns, index);
                break;
            case OldBytecodeInst::Opcode::ECMA_CREATEOBJECTWITHBUFFER_PREF_IMM16:
            case OldBytecodeInst::Opcode::ECMA_CREATEOBJECTHAVINGMETHOD_PREF_IMM16: {
                auto imm = bcIns.GetImm<OldBytecodeInst::Format::PREF_IMM16>();
                index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::OBJECT_LITERAL,
                    static_cast<uint16_t>(imm));
                FixInstructionId32(bcIns, index);
                break;
            }
            case OldBytecodeInst::Opcode::ECMA_CREATEARRAYWITHBUFFER_PREF_IMM16: {
                auto imm = bcIns.GetImm<OldBytecodeInst::Format::PREF_IMM16>();
                index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::ARRAY_LITERAL,
                    static_cast<uint16_t>(imm));
                FixInstructionId32(bcIns, index);
                break;
            }
            case OldBytecodeInst::Opcode::ECMA_DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8: {
                methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId()).GetOffset();
                index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::CLASS_FUNCTION, methodId);
                FixInstructionId32(bcIns, index);
                auto imm = bcIns.GetImm<OldBytecodeInst::Format::PREF_ID16_IMM16_IMM16_V8_V8>();
                index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::CLASS_LITERAL,
                    static_cast<uint16_t>(imm));
                FixInstructionId32(bcIns, index, 1);
                break;
            }
            default:
                break;
        }
    }
}
}  // namespace panda::ecmascript::kungfu