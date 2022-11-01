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

    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (pf->IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(*pf, classId);
        CString desc = utf::Mutf8AsCString(cda.GetDescriptor());
        cda.EnumerateMethods([this, methods, &methodIdx, pf, &processedInsns, &desc]
            (panda_file::MethodDataAccessor &mda) {
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
                bytecodeInfo_.mainMethodIndexes.emplace_back(methodOffset);
                bytecodeInfo_.recordNames.emplace_back(recordName);
            }

            InitializeMemory(methodLiteral, jsPandaFile_, methodId);
            methodLiteral->SetHotnessCounter(EcmaInterpreter::GetHotnessCounter(codeSize));
            methodLiteral->Initialize(
                jsPandaFile_, codeDataAccessor.GetNumVregs(), codeDataAccessor.GetNumArgs());
            const uint8_t *insns = codeDataAccessor.GetInstructions();
            if (jsPandaFile_->IsNewVersion()) {
                panda_file::IndexAccessor indexAccessor(*pf, methodId);
                panda_file::FunctionKind funcKind = indexAccessor.GetFunctionKind();
                FunctionKind kind;
                switch (funcKind) {
                    case panda_file::FunctionKind::NONE:
                    case panda_file::FunctionKind::FUNCTION:
                        kind = FunctionKind::BASE_CONSTRUCTOR;
                        break;
                    case panda_file::FunctionKind::NC_FUNCTION:
                        kind = FunctionKind::ARROW_FUNCTION;
                        break;
                    case panda_file::FunctionKind::GENERATOR_FUNCTION:
                        kind = FunctionKind::GENERATOR_FUNCTION;
                        break;
                    case panda_file::FunctionKind::ASYNC_FUNCTION:
                        kind = FunctionKind::ASYNC_FUNCTION;
                        break;
                    case panda_file::FunctionKind::ASYNC_GENERATOR_FUNCTION:
                        kind = FunctionKind::ASYNC_GENERATOR_FUNCTION;
                        break;
                    case panda_file::FunctionKind::ASYNC_NC_FUNCTION:
                        kind = FunctionKind::ASYNC_ARROW_FUNCTION;
                        break;
                    default:
                        UNREACHABLE();
                }
                methodLiteral->SetFunctionKind(kind);
            }
            auto it = processedInsns.find(insns);
            if (it == processedInsns.end()) {
                if (jsPandaFile_->IsNewVersion()) {
                    CollectMethodPcsFromNewBc(codeSize, insns, methodLiteral);
                } else {
                    CollectMethodPcs(codeSize, insns, methodLiteral, jsPandaFile_->ParseEntryPoint(desc));
                }
                processedInsns[insns] = bytecodeInfo_.methodPcInfos.size() - 1;
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

void BytecodeInfoCollector::CollectMethodPcsFromNewBc(const uint32_t insSz, const uint8_t *insArr,
                                                      const MethodLiteral *method)
{
    auto bcIns = BytecodeInst(insArr);
    auto bcInsLast = bcIns.JumpTo(insSz);
    bytecodeInfo_.methodPcInfos.emplace_back(MethodPcInfo { {}, {}, {}, insSz });
    int32_t offsetIndex = 1;
    uint8_t *curPc = nullptr;
    uint8_t *prePc = nullptr;
    while (bcIns.GetAddress() != bcInsLast.GetAddress()) {
        CollectMethodInfoFromNewBC(bcIns, method);
        auto pc = const_cast<uint8_t *>(bcIns.GetAddress());
        auto nextInst = bcIns.GetNext();
        bcIns = nextInst;

        auto &bytecodeBlockInfos = bytecodeInfo_.methodPcInfos.back().bytecodeBlockInfos;
        auto &byteCodeCurPrePc = bytecodeInfo_.methodPcInfos.back().byteCodeCurPrePc;
        auto &pcToBCOffset = bytecodeInfo_.methodPcInfos.back().pcToBCOffset;
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

    auto &bytecodeBlockInfos = bytecodeInfo_.methodPcInfos.back().bytecodeBlockInfos;
    bytecodeBlockInfos.emplace_back(curPc, SplitKind::END, std::vector<uint8_t *>(1, curPc));

    auto emptyPc = const_cast<uint8_t *>(bcInsLast.GetAddress());
    bytecodeInfo_.methodPcInfos.back().byteCodeCurPrePc[emptyPc] = prePc;
    bytecodeInfo_.methodPcInfos.back().pcToBCOffset[emptyPc] = offsetIndex++;
}

void BytecodeInfoCollector::CollectMethodPcs(const uint32_t insSz, const uint8_t *insArr,
                                             const MethodLiteral *method, const CString &entryPoint)
{
    [[maybe_unused]]bool isNewVersion = jsPandaFile_->IsNewVersion();
    auto bcIns = OldBytecodeInst(insArr);
    auto bcInsLast = bcIns.JumpTo(insSz);

    bytecodeInfo_.methodPcInfos.emplace_back(MethodPcInfo { {}, {}, {}, insSz });

    int32_t offsetIndex = 1;
    uint8_t *curPc = nullptr;
    uint8_t *prePc = nullptr;
    while (bcIns.GetAddress() != bcInsLast.GetAddress()) {
        TranslateBCIns(bcIns, method, entryPoint);
        auto pc = const_cast<uint8_t *>(bcIns.GetAddress());
        auto nextInst = bcIns.GetNext();
        FixOpcode(const_cast<MethodLiteral *>(method), bcIns);
        bcIns = nextInst;

        auto &bytecodeBlockInfos = bytecodeInfo_.methodPcInfos.back().bytecodeBlockInfos;
        auto &byteCodeCurPrePc = bytecodeInfo_.methodPcInfos.back().byteCodeCurPrePc;
        auto &pcToBCOffset = bytecodeInfo_.methodPcInfos.back().pcToBCOffset;
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
    bytecodeInfo_.methodPcInfos.back().byteCodeCurPrePc[emptyPc] = prePc;
    bytecodeInfo_.methodPcInfos.back().pcToBCOffset[emptyPc] = offsetIndex++;
}

#define ADD_NOP_INST(pc, oldLen, newOpcode)                              \
do {                                                                     \
    int newLen = static_cast<int>(BytecodeInstruction::Size(newOpcode)); \
    int paddingSize = static_cast<int>(oldLen) - newLen;                 \
    for (int i = 0; i < paddingSize; i++) {                              \
        *(pc + newLen + i) = static_cast<uint8_t>(EcmaOpcode::NOP);      \
    }                                                                    \
} while (false)

void BytecodeInfoCollector::FixOpcode(MethodLiteral *method, const OldBytecodeInst &inst)
{
    auto opcode = inst.GetOpcode();
    auto pc = const_cast<uint8_t *>(inst.GetAddress());
    EcmaOpcode newOpcode;
    auto oldLen = OldBytecodeInst::Size(OldBytecodeInst::GetFormat(opcode));
    pc = const_cast<uint8_t *>(inst.GetAddress());

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
        case OldBytecodeInst::Opcode::ECMA_ASYNCGENERATORREJECT_PREF_V8_V8: {
            newOpcode = EcmaOpcode::DEPRECATED_ASYNCGENERATORREJECT_PREF_V8_V8;
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
    BytecodeInstruction inst(pc);
    auto opcode = inst.GetOpcode();
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

void BytecodeInfoCollector::TranslateBCIns(const OldBytecodeInst &bcIns, const MethodLiteral *method,
                                           const CString &entryPoint)
{
    const panda_file::File *pf = jsPandaFile_->GetPandaFile();
    const CUnorderedMap<uint32_t, uint64_t> *constpoolMap = jsPandaFile_->GetConstpoolMapByReocrd(entryPoint);
    if (bcIns.HasFlag(OldBytecodeInst::Flags::STRING_ID) &&
        OldBytecodeInst::HasId(OldBytecodeInst::GetFormat(bcIns.GetOpcode()), 0)) {
        auto index = jsPandaFile_->GetOrInsertConstantPool(ConstPoolType::STRING, bcIns.GetId());
        FixInstructionId32(bcIns, index);
    } else {
        OldBytecodeInst::Opcode opcode = static_cast<OldBytecodeInst::Opcode>(bcIns.GetOpcode());
        switch (opcode) {
            uint32_t index;
            uint32_t methodId;
            case OldBytecodeInst::Opcode::ECMA_DEFINEFUNCDYN_PREF_ID16_IMM16_V8:
                methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId()).GetOffset();
                CollectInnerMethods(method, methodId);
                index = jsPandaFile_->GetOrInsertConstantPool(ConstPoolType::BASE_FUNCTION, methodId, constpoolMap);
                FixInstructionId32(bcIns, index);
                break;
            case OldBytecodeInst::Opcode::ECMA_DEFINENCFUNCDYN_PREF_ID16_IMM16_V8:
                methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId()).GetOffset();
                CollectInnerMethods(method, methodId);
                index = jsPandaFile_->GetOrInsertConstantPool(ConstPoolType::NC_FUNCTION, methodId, constpoolMap);
                FixInstructionId32(bcIns, index);
                break;
            case OldBytecodeInst::Opcode::ECMA_DEFINEGENERATORFUNC_PREF_ID16_IMM16_V8:
                methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId()).GetOffset();
                CollectInnerMethods(method, methodId);
                index = jsPandaFile_->GetOrInsertConstantPool(ConstPoolType::GENERATOR_FUNCTION, methodId,
                                                              constpoolMap);
                FixInstructionId32(bcIns, index);
                break;
            case OldBytecodeInst::Opcode::ECMA_DEFINEASYNCGENERATORFUNC_PREF_ID16_IMM16_V8:
                methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId()).GetOffset();
                CollectInnerMethods(method, methodId);
                index = jsPandaFile_->GetOrInsertConstantPool(ConstPoolType::ASYNC_GENERATOR_FUNCTION, methodId);
                FixInstructionId32(bcIns, index);
                break;
            case OldBytecodeInst::Opcode::ECMA_DEFINEASYNCFUNC_PREF_ID16_IMM16_V8:
                methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId()).GetOffset();
                CollectInnerMethods(method, methodId);
                index = jsPandaFile_->GetOrInsertConstantPool(ConstPoolType::ASYNC_FUNCTION, methodId, constpoolMap);
                FixInstructionId32(bcIns, index);
                break;
            case OldBytecodeInst::Opcode::ECMA_DEFINEMETHOD_PREF_ID16_IMM16_V8:
                methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId()).GetOffset();
                CollectInnerMethods(method, methodId);
                index = jsPandaFile_->GetOrInsertConstantPool(ConstPoolType::METHOD, methodId, constpoolMap);
                FixInstructionId32(bcIns, index);
                break;
            case OldBytecodeInst::Opcode::ECMA_CREATEOBJECTWITHBUFFER_PREF_IMM16:
            case OldBytecodeInst::Opcode::ECMA_CREATEOBJECTHAVINGMETHOD_PREF_IMM16: {
                auto imm = bcIns.GetImm<OldBytecodeInst::Format::PREF_IMM16>();
                CollectInnerMethodsFromLiteral(method, imm);
                index = jsPandaFile_->GetOrInsertConstantPool(ConstPoolType::OBJECT_LITERAL,
                    static_cast<uint16_t>(imm), constpoolMap);
                FixInstructionId32(bcIns, index);
                break;
            }
            case OldBytecodeInst::Opcode::ECMA_CREATEARRAYWITHBUFFER_PREF_IMM16: {
                auto imm = bcIns.GetImm<OldBytecodeInst::Format::PREF_IMM16>();
                CollectInnerMethodsFromLiteral(method, imm);
                index = jsPandaFile_->GetOrInsertConstantPool(ConstPoolType::ARRAY_LITERAL,
                    static_cast<uint16_t>(imm), constpoolMap);
                FixInstructionId32(bcIns, index);
                break;
            }
            case OldBytecodeInst::Opcode::ECMA_DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8: {
                methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId()).GetOffset();
                index = jsPandaFile_->GetOrInsertConstantPool(ConstPoolType::CLASS_FUNCTION, methodId, constpoolMap);
                FixInstructionId32(bcIns, index);
                auto imm = bcIns.GetImm<OldBytecodeInst::Format::PREF_ID16_IMM16_IMM16_V8_V8>();
                CollectInnerMethods(method, methodId);
                CollectInnerMethodsFromLiteral(method, imm);
                index = jsPandaFile_->GetOrInsertConstantPool(ConstPoolType::CLASS_LITERAL,
                    static_cast<uint16_t>(imm), constpoolMap);
                FixInstructionId32(bcIns, index, 1);
                break;
            }
            case OldBytecodeInst::Opcode::ECMA_NEWLEXENVDYN_PREF_IMM16: {
                auto imm = bcIns.GetImm<OldBytecodeInst::Format::PREF_IMM16>();
                NewLexEnvWithSize(method, imm);
                break;
            }
            case OldBytecodeInst::Opcode::ECMA_NEWLEXENVWITHNAMEDYN_PREF_IMM16_IMM16: {
                auto imm = bcIns.GetImm<OldBytecodeInst::Format::PREF_IMM16_IMM16>();
                NewLexEnvWithSize(method, imm);
                break;
            }
            default:
                break;
        }
    }
}

void BytecodeInfoCollector::SetMethodPcInfoIndex(uint32_t methodOffset, size_t index)
{
    auto &methodList = bytecodeInfo_.methodList;
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
    auto &methodList = bytecodeInfo_.methodList;
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
    auto &methodList = bytecodeInfo_.methodList;
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

void BytecodeInfoCollector::CollectMethodInfoFromNewBC(const BytecodeInstruction &bcIns,
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

LexEnvManager::LexEnvManager(BCInfo &bcInfo)
    : lexEnvs_(bcInfo.methodList.size(), nullptr)
{
    auto &methodList = bcInfo.methodList;
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
}  // namespace panda::ecmascript::kungfu
