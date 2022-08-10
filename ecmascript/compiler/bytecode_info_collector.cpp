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

#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/compiler/bytecode_circuit_builder.h"
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
    ASSERT(jsPandaFile != nullptr && jsPandaFile->GetMethods() != nullptr);
    JSMethod *methods = jsPandaFile->GetMethods();
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
        cda.EnumerateMethods([methods, &methodIdx, pf, &processedInsns, jsPandaFile, &methodPcInfos, &sd]
            (panda_file::MethodDataAccessor &mda) {
            auto codeId = mda.GetCodeId();
            ASSERT(codeId.has_value());

            JSMethod *method = methods + (methodIdx++);
            panda_file::CodeDataAccessor codeDataAccessor(*pf, codeId.value());
            uint32_t codeSize = codeDataAccessor.GetCodeSize();

            uint32_t mainMethodIndex = jsPandaFile->GetMainMethodIndex();
            if (mainMethodIndex == 0 && pf->GetStringData(mda.GetNameId()) == sd) {
                jsPandaFile->UpdateMainMethodIndex(mda.GetMethodId().GetOffset());
            }

            new (method) JSMethod(jsPandaFile, mda.GetMethodId());
            method->SetHotnessCounter(EcmaInterpreter::METHOD_HOTNESS_THRESHOLD);
            method->InitializeCallField(codeDataAccessor.GetNumVregs(), codeDataAccessor.GetNumArgs());
            const uint8_t *insns = codeDataAccessor.GetInstructions();
            auto it = processedInsns.find(insns);
            if (it == processedInsns.end()) {
                CollectMethodPcs(jsPandaFile, codeSize, insns, method, methodPcInfos);
                processedInsns[insns] = methodPcInfos.size() - 1;
            } else {
                methodPcInfos[it->second].methods.emplace_back(method);
            }
            jsPandaFile->SetMethodToMap(method);
        });
    }
}

void BytecodeInfoCollector::FixOpcode(uint8_t *pc)
{
    auto opcode = static_cast<BytecodeInstruction::Opcode>(*pc);

    switch (opcode) {
        case BytecodeInstruction::Opcode::MOV_V4_V4:
            *pc = static_cast<uint8_t>(EcmaOpcode::MOV_V4_V4);
            break;
        case BytecodeInstruction::Opcode::MOV_DYN_V8_V8:
            *pc = static_cast<uint8_t>(EcmaOpcode::MOV_DYN_V8_V8);
            break;
        case BytecodeInstruction::Opcode::MOV_DYN_V16_V16:
            *pc = static_cast<uint8_t>(EcmaOpcode::MOV_DYN_V16_V16);
            break;
        case BytecodeInstruction::Opcode::LDA_STR_ID32:
            *pc = static_cast<uint8_t>(EcmaOpcode::LDA_STR_ID32);
            break;
        case BytecodeInstruction::Opcode::JMP_IMM8:
            *pc = static_cast<uint8_t>(EcmaOpcode::JMP_IMM8);
            break;
        case BytecodeInstruction::Opcode::JMP_IMM16:
            *pc = static_cast<uint8_t>(EcmaOpcode::JMP_IMM16);
            break;
        case BytecodeInstruction::Opcode::JMP_IMM32:
            *pc = static_cast<uint8_t>(EcmaOpcode::JMP_IMM32);
            break;
        case BytecodeInstruction::Opcode::JEQZ_IMM8:
            *pc = static_cast<uint8_t>(EcmaOpcode::JEQZ_IMM8);
            break;
        case BytecodeInstruction::Opcode::JEQZ_IMM16:
            *pc = static_cast<uint8_t>(EcmaOpcode::JEQZ_IMM16);
            break;
        case BytecodeInstruction::Opcode::JNEZ_IMM8:
            *pc = static_cast<uint8_t>(EcmaOpcode::JNEZ_IMM8);
            break;
        case BytecodeInstruction::Opcode::JNEZ_IMM16:
            *pc = static_cast<uint8_t>(EcmaOpcode::JNEZ_IMM16);
            break;
        case BytecodeInstruction::Opcode::LDA_DYN_V8:
            *pc = static_cast<uint8_t>(EcmaOpcode::LDA_DYN_V8);
            break;
        case BytecodeInstruction::Opcode::STA_DYN_V8:
            *pc = static_cast<uint8_t>(EcmaOpcode::STA_DYN_V8);
            break;
        case BytecodeInstruction::Opcode::LDAI_DYN_IMM32:
            *pc = static_cast<uint8_t>(EcmaOpcode::LDAI_DYN_IMM32);
            break;
        case BytecodeInstruction::Opcode::FLDAI_DYN_IMM64:
            *pc = static_cast<uint8_t>(EcmaOpcode::FLDAI_DYN_IMM64);
            break;
        case BytecodeInstruction::Opcode::RETURN_DYN:
            *pc = static_cast<uint8_t>(EcmaOpcode::RETURN_DYN);
            break;
        default:
            if (*pc != static_cast<uint8_t>(BytecodeInstruction::Opcode::ECMA_LDNAN_PREF_NONE)) {
                LOG_FULL(FATAL) << "Is not an Ecma Opcode opcode: " << static_cast<uint16_t>(opcode);
                UNREACHABLE();
            }
            *pc = *(pc + 1);
            *(pc + 1) = 0xFF;
            break;
    }
}

void BytecodeInfoCollector::UpdateICOffset(JSMethod *method, uint8_t *pc)
{
    uint8_t offset = JSMethod::MAX_SLOT_SIZE;
    auto opcode = static_cast<EcmaOpcode>(*pc);
    switch (opcode) {
        case EcmaOpcode::TRYLDGLOBALBYNAME_PREF_ID32:
        case EcmaOpcode::TRYSTGLOBALBYNAME_PREF_ID32:
        case EcmaOpcode::LDGLOBALVAR_PREF_ID32:
        case EcmaOpcode::STGLOBALVAR_PREF_ID32:
        case EcmaOpcode::ADD2DYN_PREF_V8:
        case EcmaOpcode::SUB2DYN_PREF_V8:
        case EcmaOpcode::MUL2DYN_PREF_V8:
        case EcmaOpcode::DIV2DYN_PREF_V8:
        case EcmaOpcode::MOD2DYN_PREF_V8:
        case EcmaOpcode::SHL2DYN_PREF_V8:
        case EcmaOpcode::SHR2DYN_PREF_V8:
        case EcmaOpcode::ASHR2DYN_PREF_V8:
        case EcmaOpcode::AND2DYN_PREF_V8:
        case EcmaOpcode::OR2DYN_PREF_V8:
        case EcmaOpcode::XOR2DYN_PREF_V8:
        case EcmaOpcode::EQDYN_PREF_V8:
        case EcmaOpcode::NOTEQDYN_PREF_V8:
        case EcmaOpcode::LESSDYN_PREF_V8:
        case EcmaOpcode::LESSEQDYN_PREF_V8:
        case EcmaOpcode::GREATERDYN_PREF_V8:
        case EcmaOpcode::GREATEREQDYN_PREF_V8:
            offset = method->UpdateSlotSize(1);
            break;
        case EcmaOpcode::LDOBJBYVALUE_PREF_V8_V8:
        case EcmaOpcode::STOBJBYVALUE_PREF_V8_V8:
        case EcmaOpcode::STOWNBYVALUE_PREF_V8_V8:
        case EcmaOpcode::LDOBJBYNAME_PREF_ID32_V8:
        case EcmaOpcode::STOBJBYNAME_PREF_ID32_V8:
        case EcmaOpcode::STOWNBYNAME_PREF_ID32_V8:
        case EcmaOpcode::LDOBJBYINDEX_PREF_V8_IMM32:
        case EcmaOpcode::STOBJBYINDEX_PREF_V8_IMM32:
        case EcmaOpcode::STOWNBYINDEX_PREF_V8_IMM32:
        case EcmaOpcode::LDSUPERBYVALUE_PREF_V8_V8:
        case EcmaOpcode::STSUPERBYVALUE_PREF_V8_V8:
        case EcmaOpcode::LDSUPERBYNAME_PREF_ID32_V8:
        case EcmaOpcode::STSUPERBYNAME_PREF_ID32_V8:
        case EcmaOpcode::LDMODULEVAR_PREF_ID32_IMM8:
        case EcmaOpcode::STMODULEVAR_PREF_ID32:
            offset = method->UpdateSlotSize(2); // 2: occupy two ic slot
            break;
        default:
            return;
    }

    *(pc + 1) = offset;
}

void BytecodeInfoCollector::FixInstructionId32(const BytecodeInstruction &inst, uint32_t index,
                                               uint32_t fixOrder)
{
    // NOLINTNEXTLINE(hicpp-use-auto)
    auto pc = const_cast<uint8_t *>(inst.GetAddress());
    switch (inst.GetFormat()) {
        case BytecodeInstruction::Format::ID32: {
            uint8_t size = sizeof(uint32_t);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (memcpy_s(pc + FixInsIndex::FIX_ONE, size, &index, size) != EOK) {
                LOG_FULL(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
            break;
        }
        case BytecodeInstruction::Format::PREF_ID16_IMM16_V8: {
            uint16_t u16Index = index;
            uint8_t size = sizeof(uint16_t);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (memcpy_s(pc + FixInsIndex::FIX_TWO, size, &u16Index, size) != EOK) {
                LOG_FULL(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
            break;
        }
        case BytecodeInstruction::Format::PREF_ID32:
        case BytecodeInstruction::Format::PREF_ID32_V8:
        case BytecodeInstruction::Format::PREF_ID32_IMM8: {
            uint8_t size = sizeof(uint32_t);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (memcpy_s(pc + FixInsIndex::FIX_TWO, size, &index, size) != EOK) {
                LOG_FULL(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
            break;
        }
        case BytecodeInstruction::Format::PREF_IMM16: {
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
        case BytecodeInstruction::Format::PREF_ID16_IMM16_IMM16_V8_V8: {
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

void BytecodeInfoCollector::TranslateBCIns(JSPandaFile *jsPandaFile, const panda::BytecodeInstruction &bcIns,
                                           const JSMethod *method)
{
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    if (bcIns.HasFlag(BytecodeInstruction::Flags::STRING_ID) &&
        BytecodeInstruction::HasId(bcIns.GetFormat(), 0)) {
        auto index = jsPandaFile->GetOrInsertConstantPool(
            ConstPoolType::STRING, bcIns.GetId().AsFileId().GetOffset());
        FixInstructionId32(bcIns, index);
    } else {
        BytecodeInstruction::Opcode opcode = static_cast<BytecodeInstruction::Opcode>(bcIns.GetOpcode());
        switch (opcode) {
            uint32_t index;
            uint32_t methodId;
            case BytecodeInstruction::Opcode::ECMA_DEFINEFUNCDYN_PREF_ID16_IMM16_V8:
                methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId().AsIndex()).GetOffset();
                index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::BASE_FUNCTION, methodId);
                FixInstructionId32(bcIns, index);
                break;
            case BytecodeInstruction::Opcode::ECMA_DEFINENCFUNCDYN_PREF_ID16_IMM16_V8:
                methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId().AsIndex()).GetOffset();
                index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::NC_FUNCTION, methodId);
                FixInstructionId32(bcIns, index);
                break;
            case BytecodeInstruction::Opcode::ECMA_DEFINEGENERATORFUNC_PREF_ID16_IMM16_V8:
                methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId().AsIndex()).GetOffset();
                index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::GENERATOR_FUNCTION, methodId);
                FixInstructionId32(bcIns, index);
                break;
            case BytecodeInstruction::Opcode::ECMA_DEFINEASYNCFUNC_PREF_ID16_IMM16_V8:
                methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId().AsIndex()).GetOffset();
                index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::ASYNC_FUNCTION, methodId);
                FixInstructionId32(bcIns, index);
                break;
            case BytecodeInstruction::Opcode::ECMA_DEFINEMETHOD_PREF_ID16_IMM16_V8:
                methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId().AsIndex()).GetOffset();
                index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::METHOD, methodId);
                FixInstructionId32(bcIns, index);
                break;
            case BytecodeInstruction::Opcode::ECMA_CREATEOBJECTWITHBUFFER_PREF_IMM16:
            case BytecodeInstruction::Opcode::ECMA_CREATEOBJECTHAVINGMETHOD_PREF_IMM16: {
                auto imm = bcIns.GetImm<BytecodeInstruction::Format::PREF_IMM16>();
                index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::OBJECT_LITERAL,
                    static_cast<uint16_t>(imm));
                FixInstructionId32(bcIns, index);
                break;
            }
            case BytecodeInstruction::Opcode::ECMA_CREATEARRAYWITHBUFFER_PREF_IMM16: {
                auto imm = bcIns.GetImm<BytecodeInstruction::Format::PREF_IMM16>();
                index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::ARRAY_LITERAL,
                    static_cast<uint16_t>(imm));
                FixInstructionId32(bcIns, index);
                break;
            }
            case BytecodeInstruction::Opcode::ECMA_DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8: {
                methodId = pf->ResolveMethodIndex(method->GetMethodId(), bcIns.GetId().AsIndex()).GetOffset();
                index = jsPandaFile->GetOrInsertConstantPool(ConstPoolType::CLASS_FUNCTION, methodId);
                FixInstructionId32(bcIns, index);
                auto imm = bcIns.GetImm<BytecodeInstruction::Format::PREF_ID16_IMM16_IMM16_V8_V8>();
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

void BytecodeInfoCollector::CollectMethodPcs(JSPandaFile *jsPandaFile, const uint32_t insSz, const uint8_t *insArr,
                                             const JSMethod *method, std::vector<MethodPcInfo> &methodPcInfos)
{
    auto bcIns = BytecodeInstruction(insArr);
    auto bcInsLast = bcIns.JumpTo(insSz);

    methodPcInfos.emplace_back(MethodPcInfo { std::vector<const JSMethod *>(1, method), {}, {}, {} });

    int32_t offsetIndex = 1;
    uint8_t *curPc = nullptr;
    uint8_t *prePc = nullptr;
    while (bcIns.GetAddress() != bcInsLast.GetAddress()) {
        TranslateBCIns(jsPandaFile, bcIns, method);

        auto pc = const_cast<uint8_t *>(bcIns.GetAddress());
        bcIns = bcIns.GetNext();

        FixOpcode(pc);
        UpdateICOffset(const_cast<JSMethod *>(method), pc);

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
}  // namespace panda::ecmascript::kungfu