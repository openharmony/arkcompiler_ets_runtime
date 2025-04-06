/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <climits>
#include <cassert>
#include "ecmascript/compiler/bytecode_info_collector.h"
#include "ecmascript/compiler/jit_compiler.h"
#ifdef JIT_ENABLE_CODE_SIGN
#include "jit_buffer_integrity.h"
#include "ecmascript/compiler/jit_signcode.h"
#endif

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;
#ifdef JIT_ENABLE_CODE_SIGN
using namespace kungfu;
using namespace OHOS::Security::CodeSign;
#endif

#define LOG_INST() LOG_BASELINEJIT(DEBUG)

#define ADVANCE_PC(opcode) bytecodeArray += BytecodeInstruction::Size(opcode)
#define BYTECODE_BASELINE_HANDLER_IMPLEMENT(name)   \
void BaselineCompiler::Handle##name(const uint8_t *bytecodeArray)

#define READ_INST_OP() READ_INST_8(0)               // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_4_0() (READ_INST_8(1) & 0xf)      // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_4_1() (READ_INST_8(1) >> 4 & 0xf) // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_4_2() (READ_INST_8(2) & 0xf)      // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_4_3() (READ_INST_8(2) >> 4 & 0xf) // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_0() READ_INST_8(1)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_1() READ_INST_8(2)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_2() READ_INST_8(3)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_3() READ_INST_8(4)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_4() READ_INST_8(5)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_5() READ_INST_8(6)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_6() READ_INST_8(7)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_7() READ_INST_8(8)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_8() READ_INST_8(9)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_9() READ_INST_8(10)             // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8(offset) (*(bytecodeArray + (offset)))
#define MOVE_AND_READ_INST_8(currentInst, offset) \
    (currentInst) <<= 8;                          \
    (currentInst) += READ_INST_8(offset);         \

#define READ_INST_16_0() READ_INST_16(2)
#define READ_INST_16_1() READ_INST_16(3)
#define READ_INST_16_2() READ_INST_16(4)
#define READ_INST_16_3() READ_INST_16(5)
#define READ_INST_16_4() READ_INST_16(6)
#define READ_INST_16_5() READ_INST_16(7)
#define READ_INST_16_6() READ_INST_16(8)
#define READ_INST_16_7() READ_INST_16(9)
#define READ_INST_16(offset)                            \
    ({                                                  \
        uint16_t currentInst = READ_INST_8(offset);     \
        MOVE_AND_READ_INST_8(currentInst, (offset) - 1) \
    })

#define READ_INST_32_0() READ_INST_32(4)
#define READ_INST_32_1() READ_INST_32(5)
#define READ_INST_32_2() READ_INST_32(6)
#define READ_INST_32(offset)                            \
    ({                                                  \
        uint32_t currentInst = READ_INST_8(offset);     \
        MOVE_AND_READ_INST_8(currentInst, (offset) - 1) \
        MOVE_AND_READ_INST_8(currentInst, (offset) - 2) \
        MOVE_AND_READ_INST_8(currentInst, (offset) - 3) \
    })

#define READ_INST_64_0()                       \
    ({                                         \
        uint64_t currentInst = READ_INST_8(8); \
        MOVE_AND_READ_INST_8(currentInst, 7)   \
        MOVE_AND_READ_INST_8(currentInst, 6)   \
        MOVE_AND_READ_INST_8(currentInst, 5)   \
        MOVE_AND_READ_INST_8(currentInst, 4)   \
        MOVE_AND_READ_INST_8(currentInst, 3)   \
        MOVE_AND_READ_INST_8(currentInst, 2)   \
        MOVE_AND_READ_INST_8(currentInst, 1)   \
    })

#define CALL_BASELINE_UPDATE_HOTNESS()                                                                \
    std::vector<BaselineParameter> parameters;                                                        \
    parameters.emplace_back(BaselineSpecialParameter::GLUE);                                          \
    parameters.emplace_back(BaselineSpecialParameter::SP);                                            \
    parameters.emplace_back(static_cast<int32_t>(offset));                                            \
    auto *thread = vm->GetAssociatedJSThread();                                                       \
    Address builtinAddress = thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineUpdateHotness); \
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);                                   \

void BaselineCompiler::SetPfHeaderAddr(const JSPandaFile *jsPandaFile)
{
    const panda_file::File *pandaFile = jsPandaFile->GetPandaFile();
    pfHeaderAddr = pandaFile->GetBase();
}

static ARK_INLINE void SetupCodeSigner([[maybe_unused]] EcmaVM *vm)
{
#ifdef JIT_ENABLE_CODE_SIGN
    bool enableCodeSign = !vm->GetJSOptions().GetDisableCodeSign();
    if (enableCodeSign && JitFort::IsResourceAvailable()) {
        JitSignCode *singleton = JitSignCode::GetInstance();
        singleton->Reset();
        JitCodeSigner *jitSigner = CreateJitCodeSigner();
        singleton->SetCodeSigner(jitSigner);
        LOG_INST() << "  Created Code Signer for baseline compilation: " << std::hex << (uintptr_t)jitSigner << "\n";
    }
#endif
    return;
}

void BaselineCompiler::Compile(const JSPandaFile *jsPandaFile, const MethodLiteral *methodLiteral)
{
    std::string tripleStr = vm->GetJSOptions().GetTargetTriple();
    const uint8_t *bytecodeArray = methodLiteral->GetBytecodeArray();
    auto codeSize = MethodLiteral::GetCodeSize(jsPandaFile, methodLiteral->GetMethodId());
    const uint8_t *methodBytecodeLast = bytecodeArray + codeSize;
    StackOffsetDescriptor stackOffsetDescriptor(methodLiteral->GetCallField());
    GetBaselineAssembler().SetStackOffsetDescriptor(stackOffsetDescriptor);
    SetPfHeaderAddr(jsPandaFile);
    firstPC = bytecodeArray;

    SetupCodeSigner(vm);

    auto *thread = vm->GetAssociatedJSThread();
    Address bcAddr = thread->GetRTInterface(RuntimeStubCSigns::ID_CallArg1AndCheckToBaseline);
    LOG_INST() << "    ID_CallArg1AndCheckToBaseline Address: " << std::hex << bcAddr;

    std::unordered_set<size_t> jumpToOffsets;
    GetJumpToOffsets(bytecodeArray, methodBytecodeLast, jumpToOffsets);

    while (bytecodeArray < methodBytecodeLast) {
        auto opCode = Bytecodes::GetOpcode(bytecodeArray);
        LOG_INST() << "processing opCode: " << GetEcmaOpcodeStr(Bytecodes::GetOpcode(bytecodeArray));
        LOG_INST() << "current bytecodePc offset: " <<
            static_cast<uint32_t>(bytecodeArray - methodLiteral->GetBytecodeArray());
        LOG_INST() << "current relative nativePc diff: " <<
            static_cast<uint64_t>(GetBaselineAssembler().GetBufferSize() - nativePcOffsetTable.GetPrevNativePc());

        if (jumpToOffsets.count(bytecodeOffset) != 0) {
            if (jumpMap.count(bytecodeOffset) != 0) {
                JumpLabel *label = jumpMap.at(bytecodeOffset);
                GetBaselineAssembler().Bind(*label);
            } else {
                JumpLabel *label = new JumpLabel();
                GetBaselineAssembler().Bind(*label);
                jumpMap.insert({bytecodeOffset, label});
            }
        }

        switch (opCode) {
#define BYTECODE_SWITCH_CASE(name)                                      \
            case EcmaOpcode::name:                                      \
                Handle##name(bytecodeArray);                            \
                break;
    ECMA_OPCODE_LIST(BYTECODE_SWITCH_CASE)
#undef BYTECODE_SWITCH_CASE
            default:
                break;
        }
        ADVANCE_PC(opCode);
        bytecodeOffset += BytecodeInstruction::Size(opCode);
        nativePcOffsetTable.AddPosition(static_cast<uint64_t>(GetBaselineAssembler().GetBufferSize()));
    }
}

bool BaselineCompiler::CollectMemoryCodeInfos(MachineCodeDesc &codeDesc)
{
    codeDesc.codeAddr = reinterpret_cast<uint64_t>(GetBaselineAssembler().GetBuffer());
    codeDesc.codeSize = GetBaselineAssembler().GetBufferSize();
    codeDesc.codeType = MachineCodeType::BASELINE_CODE;
    codeDesc.stackMapOrOffsetTableAddr = reinterpret_cast<uint64_t>(nativePcOffsetTable.GetData());
    codeDesc.stackMapOrOffsetTableSize = nativePcOffsetTable.GetSize();
    if (vm->GetJSOptions().GetTargetTriple() == TARGET_AARCH64) {
        codeDesc.archType = MachineCodeArchType::AArch64;
    } else {
        codeDesc.archType = MachineCodeArchType::X86;
    }
#ifdef JIT_ENABLE_CODE_SIGN
    codeDesc.codeSigner = 0;
    JitSignCode *singleton = JitSignCode::GetInstance();
    if (singleton->GetCodeSigner() != nullptr) {
        LOG_INST() << "In CollectMemoryCodeInfos, signer = " << singleton->GetCodeSigner();
        LOG_INST() << "     Kind = " << singleton->GetKind();
        codeDesc.codeSigner = reinterpret_cast<uintptr_t>(singleton->GetCodeSigner());
    }
#endif
    if (Jit::GetInstance()->IsEnableJitFort() && Jit::GetInstance()->IsEnableAsyncCopyToFort() &&
        JitCompiler::AllocFromFortAndCopy(*compilationEnv, codeDesc, GetBaselineAssembler().GetRelocInfo()) == false) {
        return false;
    }
    return true;
}

void BaselineCompiler::CollectBLInfo(RelocMap &relocInfo)
{
    relocInfo = GetBaselineAssembler().GetRelocInfo();
}

void BaselineCompiler::GetJumpToOffsets(const uint8_t *start, const uint8_t *end,
                                        std::unordered_set<size_t> &jumpToOffsets) const
{
    size_t offset = 0;
    while (start < end) {
        auto opCode = Bytecodes::GetOpcode(start);
        switch (opCode) {
            case EcmaOpcode::JEQZ_IMM8:
            case EcmaOpcode::JNEZ_IMM8:
            case EcmaOpcode::JMP_IMM8: {
                int8_t jumpOffset = *(start + 1); // 1: get one byte in bytecodes
                size_t jumpTo = offset + jumpOffset;
                jumpToOffsets.insert(jumpTo);
                break;
            }
            case EcmaOpcode::JEQZ_IMM16:
            case EcmaOpcode::JNEZ_IMM16:
            case EcmaOpcode::JMP_IMM16: {
                int16_t jumpOffset = *(start + 2);                              // 2: get two bytes in bytecodes
                uint16_t tmpValue = static_cast<uint16_t>(jumpOffset) << 8;    // 8: left shift 8 bits
                tmpValue += static_cast<uint8_t>(*(start + 1));                 // 1: get one byte in bytecodes
                size_t jumpTo = offset + static_cast<int16_t>(tmpValue);
                jumpToOffsets.insert(jumpTo);
                break;
            }
            case EcmaOpcode::JEQZ_IMM32:
            case EcmaOpcode::JNEZ_IMM32:
            case EcmaOpcode::JMP_IMM32: {
                int32_t jumpOffset = *(start + 4); // 4: get four bytes in bytecodes
                uint32_t tmpValue = static_cast<uint32_t>(jumpOffset) << 8;    // 8: left shift 8 bits
                tmpValue += static_cast<uint8_t>(*(start + 3));                 // 3: get three bytes in bytecodes
                tmpValue <<= 8;                                                 // 8: left shift 8 bits
                tmpValue += static_cast<uint8_t>(*(start + 2));                 // 2: get two bytes in bytecodes
                tmpValue <<= 8;                                                 // 8: left shift 8 bits
                tmpValue += static_cast<uint8_t>(*(start + 1));                 // 1: get one byte in bytecodes
                size_t jumpTo = static_cast<size_t>(static_cast<int32_t>(offset) + static_cast<int32_t>(tmpValue));
                jumpToOffsets.insert(jumpTo);
                break;
            }
            default:
                break;
        }

        start += BytecodeInstruction::Size(opCode);
        offset += BytecodeInstruction::Size(opCode);
    }
}

// ------- parse bytecodes about reading special variables -------
BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDFUNCTION)
{
    (void)bytecodeArray;
    LOG_INST() << "    ldfunction ";

    GetBaselineAssembler().Move(SpecialRegister::ACC_REGISTER, SpecialRegister::FUNC);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDNEWTARGET)
{
    (void)bytecodeArray;
    LOG_INST() << "    ldnewtarget ";

    // check hasNewTarget?
    GetBaselineAssembler().Move(SpecialRegister::ACC_REGISTER, SpecialRegister::NEW_TARGET);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDTHIS)
{
    (void)bytecodeArray;
    LOG_INST() << "    ldthis ";

    GetBaselineAssembler().Move(SpecialRegister::ACC_REGISTER, SpecialRegister::THIS_OBJECT);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDUNDEFINED)
{
    (void)bytecodeArray;
    LOG_INST() << "    ldundefined " << std::hex << Immediate(JSTaggedValue::VALUE_UNDEFINED).GetValue();

    GetBaselineAssembler().Move(SpecialRegister::ACC_REGISTER, Immediate(JSTaggedValue::VALUE_UNDEFINED));
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDNAN)
{
    (void)bytecodeArray;
    LOG_INST() << "    ldnan " << std::hex << base::NAN_VALUE;

    GetBaselineAssembler().Move(SpecialRegister::ACC_REGISTER, Immediate(JSTaggedValue(base::NAN_VALUE).GetRawData()));
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDINFINITY)
{
    (void)bytecodeArray;

    LOG_INST() << "    ldinfinity " << std::hex << base::POSITIVE_INFINITY;
    GetBaselineAssembler().Move(SpecialRegister::ACC_REGISTER,
                                Immediate(JSTaggedValue(base::POSITIVE_INFINITY).GetRawData()));
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDNULL)
{
    (void)bytecodeArray;

    LOG_INST() << "    ldnull " << std::hex << Immediate(JSTaggedValue::VALUE_NULL).GetValue();
    GetBaselineAssembler().Move(SpecialRegister::ACC_REGISTER, Immediate(JSTaggedValue::VALUE_NULL));
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDTRUE)
{
    (void)bytecodeArray;

    LOG_INST() << "    ldtrue " << std::hex << Immediate(JSTaggedValue::VALUE_TRUE).GetValue();
    GetBaselineAssembler().Move(SpecialRegister::ACC_REGISTER, Immediate(JSTaggedValue::VALUE_TRUE));
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDFALSE)
{
    (void)bytecodeArray;

    LOG_INST() << "    ldfalse " << std::hex << Immediate(JSTaggedValue::VALUE_FALSE).GetValue();
    GetBaselineAssembler().Move(SpecialRegister::ACC_REGISTER, Immediate(JSTaggedValue::VALUE_FALSE));
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDHOLE)
{
    (void)bytecodeArray;

    LOG_INST() << "    ldhole " << std::hex << Immediate(JSTaggedValue::VALUE_HOLE).GetValue();
    GetBaselineAssembler().Move(SpecialRegister::ACC_REGISTER, Immediate(JSTaggedValue::VALUE_HOLE));
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(NEWLEXENV_IMM8)
{
    uint8_t numVars = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineNewlexenvImm8);
    LOG_INST() << "    BaselineNewlexenvImm8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      numVars: " << static_cast<int32_t>(numVars);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(static_cast<int32_t>(numVars));
    parameters.emplace_back(BaselineSpecialParameter::SP);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(NEWLEXENVWITHNAME_IMM8_ID16)
{
    uint8_t numVars = READ_INST_8_0();
    int16_t literalId = READ_INST_16_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineNewlexenvwithnameImm8Id16);
    LOG_INST() << "    BaselineNewlexenvwithnameImm8Id16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      numVars: " << static_cast<int16_t>(numVars);
    LOG_INST() << "      literalId: " << literalId;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(static_cast<int16_t>(numVars));
    parameters.emplace_back(literalId);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDLEXVAR_IMM4_IMM4)
{
    uint8_t level = static_cast<uint8_t>(READ_INST_4_0());
    uint8_t slot = static_cast<uint8_t>(READ_INST_4_1());

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLdlexvarImm4Imm4);
    LOG_INST() << "    BaselineLdlexvarImm4Imm4 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      level: " << static_cast<int32_t>(level);
    LOG_INST() << "      solt: " << static_cast<int32_t>(slot);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(level));
    parameters.emplace_back(static_cast<int32_t>(slot));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STLEXVAR_IMM4_IMM4)
{
    uint8_t level = static_cast<uint8_t>(READ_INST_4_0());
    uint8_t slot = static_cast<uint8_t>(READ_INST_4_1());

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStlexvarImm4Imm4);
    LOG_INST() << "    BaselineStlexvarImm4Imm4 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      level: " << static_cast<int32_t>(level);
    LOG_INST() << "      solt: " << static_cast<int32_t>(slot);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(static_cast<int32_t>(level));
    parameters.emplace_back(static_cast<int32_t>(slot));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDLEXVAR_IMM8_IMM8)
{
    uint8_t level = READ_INST_8_0();
    uint8_t slot = READ_INST_8_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLdlexvarImm8Imm8);
    LOG_INST() << "    BaselineLdlexvarImm8Imm8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      level: " << static_cast<int32_t>(level);
    LOG_INST() << "      solt: " << static_cast<int32_t>(slot);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(static_cast<int32_t>(level));
    parameters.emplace_back(static_cast<int32_t>(slot));
    parameters.emplace_back(BaselineSpecialParameter::SP);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STLEXVAR_IMM8_IMM8)
{
    uint8_t level = READ_INST_8_0();
    uint8_t slot = READ_INST_8_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStlexvarImm8Imm8);
    LOG_INST() << "    BaselineStlexvarImm8Imm8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      level: " << static_cast<int32_t>(level);
    LOG_INST() << "      solt: " << static_cast<int32_t>(slot);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(static_cast<int32_t>(level));
    parameters.emplace_back(static_cast<int32_t>(slot));
    parameters.emplace_back(BaselineSpecialParameter::SP);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDBIGINT_ID16)
{
    uint16_t stringId = READ_INST_16_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLdbigintId16);
    LOG_INST() << "    BaselineLdbigintId16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(stringId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}
// ------- End parse bytecodes about reading special variables -------

// ------- parse bytecodes about reading and storing general variables -------
BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDSYMBOL)
{
    (void)bytecodeArray;

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLdsymbol);
    LOG_INST() << "    BaselineLdsymbol Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDGLOBAL)
{
    (void)bytecodeArray;

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLdglobal);
    LOG_INST() << "    BaselineLdglobal Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDGLOBALVAR_IMM16_ID16)
{
    uint16_t slotId = READ_INST_16_0();
    uint16_t stringId = READ_INST_16_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLdglobalvarImm16Id16);
    LOG_INST() << "    BaselineLdglobalvarImm16Id16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    parameters.emplace_back(static_cast<int32_t>(stringId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STGLOBALVAR_IMM16_ID16)
{
    uint16_t slotId = READ_INST_16_0();
    uint16_t stringId = READ_INST_16_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStglobalvarImm16Id16);
    LOG_INST() << "    BaselineStglobalvarImm16Id16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    parameters.emplace_back(static_cast<int32_t>(stringId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(TRYLDGLOBALBYNAME_IMM8_ID16)
{
    uint8_t slotId = READ_INST_8_0();
    uint16_t stringId = READ_INST_16_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineTryLdGLobalByNameImm8ID16);
    LOG_INST() << "    BaselineTryLdGLobalByNameImm8ID16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(stringId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(TRYSTGLOBALBYNAME_IMM8_ID16)
{
    uint8_t slotId = READ_INST_8_0();
    uint16_t stringId = READ_INST_16_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineTrystglobalbynameImm8Id16);
    LOG_INST() << "    BaselineTrystglobalbynameImm8Id16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(stringId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(TRYLDGLOBALBYNAME_IMM16_ID16)
{
    uint16_t slotId = READ_INST_16_0();
    uint16_t stringId = READ_INST_16_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineTryldglobalbynameImm16Id16);
    LOG_INST() << "    BaselineTryldglobalbynameImm16Id16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    parameters.emplace_back(static_cast<int32_t>(stringId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(TRYSTGLOBALBYNAME_IMM16_ID16)
{
    uint16_t slotId = READ_INST_16_0();
    uint16_t stringId = READ_INST_16_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineTrystglobalbynameImm16Id16);
    LOG_INST() << "    BaselineTrystglobalbynameImm16Id16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    parameters.emplace_back(static_cast<int32_t>(stringId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDOBJBYNAME_IMM8_ID16)
{
    uint8_t slotId = READ_INST_8_0();
    uint16_t stringId = READ_INST_16_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLdobjbynameImm8Id16);
    LOG_INST() << "    BaselineLdobjbynameImm8Id16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    parameters.emplace_back(static_cast<int32_t>(stringId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STOBJBYNAME_IMM8_ID16_V8)
{
    uint8_t slotId = READ_INST_8_0();
    uint16_t stringId = READ_INST_16_1();
    int8_t vObjId = READ_INST_8_3();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStobjbynameImm8Id16V8);
    LOG_INST() << "    BaselineStobjbynameImm8Id16V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    parameters.emplace_back(static_cast<int32_t>(stringId));
    parameters.emplace_back(VirtualRegister(vObjId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDOBJBYNAME_IMM16_ID16)
{
    uint16_t slotId = READ_INST_16_0();
    uint16_t stringId = READ_INST_16_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLdobjbynameImm16Id16);
    LOG_INST() << "    BaselineLdobjbynameImm16Id16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    parameters.emplace_back(static_cast<int32_t>(stringId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STOBJBYNAME_IMM16_ID16_V8)
{
    uint16_t slotId = READ_INST_16_0();
    uint16_t stringId = READ_INST_16_2();
    uint8_t vObjId = READ_INST_8_4();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStobjbynameImm16Id16V8);
    LOG_INST() << "    BaselineStobjbynameImm16Id16V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    parameters.emplace_back(static_cast<int32_t>(stringId));
    parameters.emplace_back(VirtualRegister(vObjId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDOBJBYVALUE_IMM8_V8)
{
    uint8_t slotId = READ_INST_8_0();
    uint8_t vObjId = READ_INST_8_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLdobjbyvalueImm8V8);
    LOG_INST() << "    BaselineLdobjbyvalueImm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vObjId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STOBJBYVALUE_IMM8_V8_V8)
{
    uint8_t slotId = READ_INST_8_0();
    uint8_t vObjId = READ_INST_8_1();
    uint8_t vKeyId = READ_INST_8_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStobjbyvalueImm8V8V8);
    LOG_INST() << "    BaselineStobjbyvalueImm8V8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vObjId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    parameters.emplace_back(VirtualRegister(vKeyId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDOBJBYVALUE_IMM16_V8)
{
    uint16_t slotId = READ_INST_16_0();
    uint8_t vObjId = READ_INST_8_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLdobjbyvalueImm16V8);
    LOG_INST() << "    BaselineLdobjbyvalueImm16V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vObjId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STOBJBYVALUE_IMM16_V8_V8)
{
    uint16_t slotId = READ_INST_16_0();
    int8_t vObjId = READ_INST_8_2();
    int8_t vKeyId = READ_INST_8_3();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStobjbyvalueImm16V8V8);
    LOG_INST() << "    BaselineStobjbyvalueImm16V8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vObjId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    parameters.emplace_back(VirtualRegister(vKeyId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDOBJBYINDEX_IMM8_IMM16)
{
    uint8_t slotId = READ_INST_8_0();
    uint16_t index = READ_INST_16_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLdobjbyindexImm8Imm16);
    LOG_INST() << "    BaselineLdobjbyindexImm8Imm16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      index: " << static_cast<int32_t>(index);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(index));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STOBJBYINDEX_IMM8_V8_IMM16)
{
    int8_t vObjId = READ_INST_8_1();
    uint16_t index = READ_INST_16_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStobjbyindexImm8V8Imm16);
    LOG_INST() << "    BaselineStobjbyindexImm8V8Imm16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      index: " << static_cast<int32_t>(index);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vObjId));
    parameters.emplace_back(static_cast<int32_t>(index));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDOBJBYINDEX_IMM16_IMM16)
{
    uint16_t slotId = READ_INST_16_0();
    uint16_t index = READ_INST_16_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLdobjbyindexImm16Imm16);
    LOG_INST() << "    BaselineLdobjbyindexImm16Imm16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      index: " << static_cast<int32_t>(index);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(index));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STOBJBYINDEX_IMM16_V8_IMM16)
{
    uint16_t slotId = READ_INST_16_0();
    uint8_t vObjId = READ_INST_8_2();
    uint16_t index = READ_INST_16_3();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStobjbyindexImm16V8Imm16);
    LOG_INST() << "    BaselineStobjbyindexImm16V8Imm16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      index: " << static_cast<int32_t>(index);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vObjId));
    parameters.emplace_back(static_cast<int32_t>(index));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDTHISBYNAME_IMM8_ID16)
{
    uint8_t slotId = READ_INST_8_0();
    uint16_t stringId = READ_INST_16_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLdthisbynameImm8Id16);
    LOG_INST() << "    BaselineLdthisbynameImm8Id16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(stringId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STTHISBYNAME_IMM8_ID16)
{
    uint8_t slotId = READ_INST_8_0();
    uint16_t stringId = READ_INST_16_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStthisbynameImm8Id16);
    LOG_INST() << "    BaselineStthisbynameImm8Id16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(stringId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDTHISBYNAME_IMM16_ID16)
{
    uint16_t slotId = READ_INST_16_0();
    uint16_t stringId = READ_INST_16_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLdthisbynameImm16Id16);
    LOG_INST() << "    BaselineLdthisbynameImm16Id16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(stringId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STTHISBYNAME_IMM16_ID16)
{
    uint16_t slotId = READ_INST_16_0();
    uint16_t stringId = READ_INST_16_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStthisbynameImm16Id16);
    LOG_INST() << "    BaselineStthisbynameImm16Id16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(stringId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDTHISBYVALUE_IMM8)
{
    uint8_t slotId = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLdthisbyvalueImm8);
    LOG_INST() << "    BaselineLdthisbyvalueImm8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STTHISBYVALUE_IMM8_V8)
{
    uint8_t slotId = READ_INST_8_0();
    uint8_t propKey = READ_INST_8_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStthisbyvalueImm8V8);
    LOG_INST() << "    BaselineStthisbyvalueImm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    parameters.emplace_back(VirtualRegister(propKey));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDTHISBYVALUE_IMM16)
{
    uint16_t slotId = READ_INST_16_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLdthisbyvalueImm16);
    LOG_INST() << "    BaselineLdthisbyvalueImm16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STTHISBYVALUE_IMM16_V8)
{
    uint16_t slotId = READ_INST_16_0();
    uint8_t propKey = READ_INST_8_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStthisbyvalueImm16V8);
    LOG_INST() << "    BaselineStthisbyvalueImm16V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    parameters.emplace_back(VirtualRegister(propKey));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDSUPERBYVALUE_IMM8_V8)
{
    int8_t object = READ_INST_8_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLdsuperbyvalueImm8V8);
    LOG_INST() << "    BaselineLdsuperbyvalueImm8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(VirtualRegister(object));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STSUPERBYVALUE_IMM8_V8_V8)
{
    int8_t object = READ_INST_8_1();
    int8_t propKey = READ_INST_8_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStsuperbyvalueImm8V8V8);
    LOG_INST() << "    BaselineStsuperbyvalueImm8V8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(object));
    parameters.emplace_back(VirtualRegister(propKey));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDSUPERBYVALUE_IMM16_V8)
{
    int8_t object = READ_INST_8_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLdsuperbyvalueImm16V8);
    LOG_INST() << "    BaselineLdsuperbyvalueImm16V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(VirtualRegister(object));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STSUPERBYVALUE_IMM16_V8_V8)
{
    int8_t object = READ_INST_8_2();
    int8_t propKey = READ_INST_8_3();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStsuperbyvalueImm16V8V8);
    LOG_INST() << "    BaselineStsuperbyvalueImm16V8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(object));
    parameters.emplace_back(VirtualRegister(propKey));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDSUPERBYNAME_IMM8_ID16)
{
    uint16_t stringId = READ_INST_16_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLdsuperbynameImm8Id16);
    LOG_INST() << "    BaselineLdsuperbynameImm8Id16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(stringId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STSUPERBYNAME_IMM8_ID16_V8)
{
    uint16_t stringId = READ_INST_16_1();
    int8_t object = READ_INST_8_3();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStsuperbynameImm8Id16V8);
    LOG_INST() << "    BaselineStsuperbynameImm8Id16V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(VirtualRegister(object));
    parameters.emplace_back(static_cast<int32_t>(stringId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDSUPERBYNAME_IMM16_ID16)
{
    uint16_t stringId = READ_INST_16_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLdsuperbynameImm16Id16);
    LOG_INST() << "    BaselineLdsuperbynameImm16Id16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(stringId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STSUPERBYNAME_IMM16_ID16_V8)
{
    uint16_t stringId = READ_INST_16_2();
    int8_t object = READ_INST_8_4();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStsuperbynameImm16Id16V8);
    LOG_INST() << "    BaselineStsuperbynameImm16Id16V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(VirtualRegister(object));
    parameters.emplace_back(static_cast<int32_t>(stringId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STA_V8)
{
    uint8_t vdst = READ_INST_8_0();
    LOG_INST() << "    sta v" << static_cast<int16_t>(vdst);
    baselineAssembler.Move(VirtualRegister(vdst), SpecialRegister::ACC_REGISTER);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDAI_IMM32)
{
    int32_t imm = static_cast<int32_t>(READ_INST_32_0());
    LOG_INST() << "    ldai " << static_cast<int16_t>(imm);
    uint64_t value = JSTaggedValue(imm).GetRawData();
    baselineAssembler.Move(SpecialRegister::ACC_REGISTER, Immediate(value));
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDA_V8)
{
    uint8_t vsrc = READ_INST_8_0();
    LOG_INST() << "    lda v" << static_cast<int16_t>(vsrc);
    baselineAssembler.Move(SpecialRegister::ACC_REGISTER, VirtualRegister(vsrc));
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STTOGLOBALRECORD_IMM16_ID16)
{
    uint16_t stringId = READ_INST_16_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStToGlobalRecordImm16ID16);
    LOG_INST() << "    BaselineStToGlobalRecordImm16ID16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(static_cast<int32_t>(stringId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDA_STR_ID16)
{
    uint16_t stringId = READ_INST_16_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLdaStrID16);
    LOG_INST() << "    BaselineLdaStrID16 Address: " << std::hex << builtinAddress << std::endl;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(stringId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(FLDAI_IMM64)
{
    int64_t imm = static_cast<int64_t>(READ_INST_64_0());

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineFldaiImm64);
    LOG_INST() << "    BaselineFldaiImm64 Address: " << std::hex << builtinAddress << std::endl;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(imm);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(GETUNMAPPEDARGS)
{
    (void)bytecodeArray;

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineGetunmappedargs);
    LOG_INST() << "    BaselineGetunmappedargs Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(GETPROPITERATOR)
{
    (void)bytecodeArray;

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineGetpropiterator);
    LOG_INST() << "    BaselineGetpropiterator Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(GETITERATOR_IMM8)
{
    uint8_t slotId = READ_INST_8_0();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineGetiteratorImm8);
    LOG_INST() << "    BaselineGetiteratorImm8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(GETITERATOR_IMM16)
{
    uint16_t slotId = READ_INST_16_0();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineGetiteratorImm16);
    LOG_INST() << "    BaselineGetiteratorImm16 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CLOSEITERATOR_IMM8_V8)
{
    uint8_t iter = READ_INST_8_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCloseiteratorImm8V8);
    LOG_INST() << "    BaselineCloseiteratorImm8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(iter));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CLOSEITERATOR_IMM16_V8)
{
    uint8_t iter = READ_INST_8_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCloseiteratorImm16V8);
    LOG_INST() << "    BaselineCloseiteratorImm16V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(iter));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(ASYNCGENERATORRESOLVE_V8_V8_V8)
{
    uint8_t generator = READ_INST_8_0();
    uint8_t object = READ_INST_8_1();
    uint8_t flag = READ_INST_8_2();
    auto offset = static_cast<int32_t>(bytecodeArray - firstPC);

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineAsyncgeneratorresolveV8V8V8);
    LOG_INST() << "    BaselineAsyncgeneratorresolveV8V8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(offset);
    parameters.emplace_back(generator);
    parameters.emplace_back(object);
    parameters.emplace_back(flag);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CREATEEMPTYOBJECT)
{
    uint8_t slotId = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCreateemptyobject);
    LOG_INST() << "    BaselineCreateemptyobject Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CREATEEMPTYARRAY_IMM8)
{
    uint8_t slotId = READ_INST_8_0();
    ASSERT((bytecodeArray - pfHeaderAddr) >= std::numeric_limits<int32_t>::min());
    ASSERT((bytecodeArray - pfHeaderAddr) <= std::numeric_limits<int32_t>::max());
    auto traceId = static_cast<int32_t>(bytecodeArray - pfHeaderAddr);

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCreateemptyarrayImm8);
    LOG_INST() << "    BaselineCreateemptyarrayImm8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      traceId: " << std::hex << traceId;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(traceId);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CREATEEMPTYARRAY_IMM16)
{
    uint16_t slotId = READ_INST_16_0();
    ASSERT((bytecodeArray - pfHeaderAddr) >= std::numeric_limits<int32_t>::min());
    ASSERT((bytecodeArray - pfHeaderAddr) <= std::numeric_limits<int32_t>::max());
    auto traceId = static_cast<int32_t>(bytecodeArray - pfHeaderAddr);

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCreateemptyarrayImm16);
    LOG_INST() << "    BaselineCreateemptyarrayImm16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      traceId: " << std::hex << traceId;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(traceId);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CREATEGENERATOROBJ_V8)
{
    int8_t funcObj = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCreategeneratorobjV8);
    LOG_INST() << "    BaselineCreategeneratorobjV8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(funcObj));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CREATEITERRESULTOBJ_V8_V8)
{
    int8_t object = READ_INST_8_0();
    int8_t flag = READ_INST_8_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCreateiterresultobjV8V8);
    LOG_INST() << "    BaselineCreateiterresultobjV8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(object));
    parameters.emplace_back(VirtualRegister(flag));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CREATEOBJECTWITHEXCLUDEDKEYS_IMM8_V8_V8)
{
    uint8_t numKeys = READ_INST_8_0();
    int8_t object = READ_INST_8_1();
    uint8_t firstKeyIdx = READ_INST_8_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCreateobjectwithexcludedkeysImm8V8V8);
    LOG_INST() << "    BaselineCreateobjectwithexcludedkeysImm8V8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      numKeys: " << static_cast<int16_t>(numKeys);
    LOG_INST() << "      firstKeyIdx: " << static_cast<int16_t>(firstKeyIdx);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int16_t>(numKeys));
    parameters.emplace_back(VirtualRegister(object));
    parameters.emplace_back(static_cast<int16_t>(firstKeyIdx));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CREATEOBJECTWITHBUFFER_IMM8_ID16)
{
    uint8_t slotId = READ_INST_8_0();
    uint16_t buffer = READ_INST_16_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCreateobjectwithbufferImm8Id16);
    LOG_INST() << "    BaselineCreateobjectwithbufferImm8Id16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      buffer: " << static_cast<int32_t>(buffer);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(buffer));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CREATEOBJECTWITHBUFFER_IMM16_ID16)
{
    uint16_t slotId = READ_INST_16_0();
    uint16_t buffer = READ_INST_16_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCreateobjectwithbufferImm16Id16);
    LOG_INST() << "    BaselineCreateobjectwithbufferImm16Id16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      buffer: " << static_cast<int32_t>(buffer);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(buffer));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CREATEREGEXPWITHLITERAL_IMM8_ID16_IMM8)
{
    uint16_t stringId = READ_INST_16_1();
    int8_t flag = READ_INST_8_3();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCreateregexpwithliteralImm8Id16Imm8);
    LOG_INST() << "    BaselineCreateregexpwithliteralImm8Id16Imm8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(stringId));
    parameters.emplace_back(flag);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CREATEREGEXPWITHLITERAL_IMM16_ID16_IMM8)
{
    uint16_t stringId = READ_INST_16_1();
    int8_t flag = READ_INST_8_4();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCreateregexpwithliteralImm16Id16Imm8);
    LOG_INST() << "    BaselineCreateregexpwithliteralImm16Id16Imm8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(stringId));
    parameters.emplace_back(flag);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CREATEARRAYWITHBUFFER_IMM8_ID16)
{
    uint8_t slotId = READ_INST_8_0();
    uint16_t literalId = READ_INST_16_1();
    ASSERT((bytecodeArray - pfHeaderAddr) >= std::numeric_limits<int32_t>::min());
    ASSERT((bytecodeArray - pfHeaderAddr) <= std::numeric_limits<int32_t>::max());
    auto traceId = static_cast<int32_t>(bytecodeArray - pfHeaderAddr);

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCreatearraywithbufferImm8Id16);
    LOG_INST() << "    BaselineCreatearraywithbufferImm8Id16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);
    LOG_INST() << "      literalId: " << static_cast<int32_t>(literalId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(traceId);
    parameters.emplace_back(static_cast<int32_t>(literalId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CREATEARRAYWITHBUFFER_IMM16_ID16)
{
    uint16_t slotId = READ_INST_16_0();
    uint16_t literalId = READ_INST_16_2();
    ASSERT((bytecodeArray - pfHeaderAddr) >= std::numeric_limits<int32_t>::min());
    ASSERT((bytecodeArray - pfHeaderAddr) <= std::numeric_limits<int32_t>::max());
    auto traceId = static_cast<int32_t>(bytecodeArray - pfHeaderAddr);

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCreatearraywithbufferImm16Id16);
    LOG_INST() << "    BaselineCreatearraywithbufferImm16Id16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);
    LOG_INST() << "      literalId: " << static_cast<int32_t>(literalId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(traceId);
    parameters.emplace_back(static_cast<int32_t>(literalId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(NEWOBJAPPLY_IMM8_V8)
{
    int8_t classId = READ_INST_8_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineNewobjapplyImm8V8);
    LOG_INST() << "    BaselineNewobjapplyImm8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(VirtualRegister(classId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(NEWOBJAPPLY_IMM16_V8)
{
    int8_t classId = READ_INST_8_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineNewobjapplyImm16V8);
    LOG_INST() << "    BaselineNewobjapplyImm16V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(VirtualRegister(classId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, NUM_ARGS, FIRST_ARG_REG_IDX, SLOT_ID
BYTECODE_BASELINE_HANDLER_IMPLEMENT(NEWOBJRANGE_IMM8_IMM8_V8)
{
    uint8_t slotId = READ_INST_8_0();
    uint8_t numArgs = READ_INST_8_1();
    uint8_t firstArgRegIdx = READ_INST_8_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineNewobjrangeImm8Imm8V8);
    LOG_INST() << "    BaselineNewobjrangeImm8Imm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      numArgs: " << static_cast<int16_t>(numArgs);
    LOG_INST() << "      firstArgRegIdx: " << static_cast<int16_t>(firstArgRegIdx);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(numArgs));
    parameters.emplace_back(static_cast<int32_t>(firstArgRegIdx));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(NEWOBJRANGE_IMM16_IMM8_V8)
{
    uint16_t slotId = READ_INST_16_0();
    uint8_t numArgs = READ_INST_8_2();
    uint8_t firstArgRegIdx = READ_INST_8_3();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineNewobjrangeImm16Imm8V8);
    LOG_INST() << "    BaselineNewobjrangeImm16Imm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      numArgs: " << static_cast<int16_t>(numArgs);
    LOG_INST() << "      firstArgRegIdx: " << static_cast<int16_t>(firstArgRegIdx);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int16_t>(numArgs));
    parameters.emplace_back(static_cast<int16_t>(firstArgRegIdx));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(TYPEOF_IMM8)
{
    (void)bytecodeArray;

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineTypeofImm8);
    LOG_INST() << "    BaselineTypeofImm8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(TYPEOF_IMM16)
{
    (void)bytecodeArray;

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineTypeofImm16);
    LOG_INST() << "    BaselineTypeofImm16 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDEXTERNALMODULEVAR_IMM8)
{
    int8_t index = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLdexternalmodulevarImm8);
    LOG_INST() << "    BaselineLdexternalmodulevarImm8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(index);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(SETOBJECTWITHPROTO_IMM8_V8)
{
    int8_t proto = READ_INST_8_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineSetobjectwithprotoImm8V8);
    LOG_INST() << "    BaselineSetobjectwithprotoImm8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(VirtualRegister(proto));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(SETOBJECTWITHPROTO_IMM16_V8)
{
    int8_t proto = READ_INST_8_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineSetobjectwithprotoImm16V8);
    LOG_INST() << "    BaselineSetobjectwithprotoImm16V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(VirtualRegister(proto));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STARRAYSPREAD_V8_V8)
{
    int8_t array = READ_INST_8_0();
    int8_t index = READ_INST_8_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStarrayspreadV8V8);
    LOG_INST() << "    BaselineStarrayspreadV8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(VirtualRegister(array));
    parameters.emplace_back(VirtualRegister(index));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDLOCALMODULEVAR_IMM8)
{
    int8_t index = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLdlocalmodulevarImm8);
    LOG_INST() << "    BaselineLdlocalmodulevarImm8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(index);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STMODULEVAR_IMM8)
{
    int8_t index = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStmodulevarImm8);
    LOG_INST() << "    BaselineStmodulevarImm8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(index);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STOWNBYVALUE_IMM8_V8_V8)
{
    uint8_t slotId = READ_INST_8_0();
    uint8_t objectId = READ_INST_8_1();
    uint8_t propKeyId = READ_INST_8_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStownbyvalueImm8V8V8);
    LOG_INST() << "    BaselineStownbyvalueImm8V8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(objectId));
    parameters.emplace_back(static_cast<int32_t>(propKeyId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STOWNBYVALUE_IMM16_V8_V8)
{
    uint16_t slotId = READ_INST_16_0();
    uint8_t object = READ_INST_8_2();
    uint8_t propKey = READ_INST_8_3();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStownbyvalueImm16V8V8);
    LOG_INST() << "    BaselineStownbyvalueImm16V8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(object));
    parameters.emplace_back(VirtualRegister(propKey));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STOWNBYINDEX_IMM8_V8_IMM16)
{
    uint8_t slotId = READ_INST_8_0();
    uint8_t object = READ_INST_8_1();
    uint16_t propKey = READ_INST_16_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStownbyindexImm8V8Imm16);
    LOG_INST() << "    BaselineStownbyindexImm8V8Imm16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);
    LOG_INST() << "      propKey: " << static_cast<int32_t>(propKey);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(object));
    parameters.emplace_back(static_cast<int32_t>(propKey));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STOWNBYINDEX_IMM16_V8_IMM16)
{
    uint16_t slotId = READ_INST_16_0();
    uint8_t object = READ_INST_8_2();
    uint16_t propKey = READ_INST_16_3();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStownbyindexImm16V8Imm16);
    LOG_INST() << "    BaselineStownbyindexImm16V8Imm16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);
    LOG_INST() << "      propKey: " << static_cast<int32_t>(propKey);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(object));
    parameters.emplace_back(static_cast<int32_t>(propKey));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STOWNBYNAME_IMM8_ID16_V8)
{
    uint8_t slotId = READ_INST_8_0();
    uint16_t stringId = READ_INST_16_1();
    uint8_t objectId = READ_INST_8_3();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStownbynameImm8Id16V8);
    LOG_INST() << "    BaselineStownbynameImm8Id16V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(objectId));
    parameters.emplace_back(static_cast<int32_t>(stringId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STOWNBYNAME_IMM16_ID16_V8)
{
    uint16_t slotId = READ_INST_16_0();
    uint16_t stringId = READ_INST_16_2();
    uint8_t object = READ_INST_8_4();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStownbynameImm16Id16V8);
    LOG_INST() << "    BaselineStownbynameImm16Id16V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(object));
    parameters.emplace_back(static_cast<int32_t>(stringId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STOWNBYVALUEWITHNAMESET_IMM8_V8_V8)
{
    uint8_t slotId = READ_INST_8_0();
    uint8_t objectId = READ_INST_8_1();
    uint8_t propKeyId = READ_INST_8_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStownbyvaluewithnamesetImm8V8V8);
    LOG_INST() << "    BaselineStownbyvaluewithnamesetImm8V8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(objectId));
    parameters.emplace_back(static_cast<int32_t>(propKeyId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STOWNBYVALUEWITHNAMESET_IMM16_V8_V8)
{
    uint16_t slotId = READ_INST_16_0();
    uint8_t object = READ_INST_8_2();
    uint8_t propKey = READ_INST_8_3();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStownbyvaluewithnamesetImm16V8V8);
    LOG_INST() << "    BaselineStownbyvaluewithnamesetImm16V8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(object));
    parameters.emplace_back(VirtualRegister(propKey));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8)
{
    uint8_t slotId = READ_INST_8_0();
    uint16_t stringId = READ_INST_16_1();
    uint8_t objectId = READ_INST_8_3();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStownbynamewithnamesetImm8Id16V8);
    LOG_INST() << "    BaselineStownbynamewithnamesetImm8Id16V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(objectId));
    parameters.emplace_back(static_cast<int32_t>(stringId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STOWNBYNAMEWITHNAMESET_IMM16_ID16_V8)
{
    uint16_t slotId = READ_INST_16_0();
    uint16_t stringId = READ_INST_16_2();
    uint8_t object = READ_INST_8_4();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStownbynamewithnamesetImm16Id16V8);
    LOG_INST() << "    BaselineStownbynamewithnamesetImm16Id16V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(stringId));
    parameters.emplace_back(VirtualRegister(object));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STCONSTTOGLOBALRECORD_IMM16_ID16)
{
    uint16_t stringId = READ_INST_16_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStconsttoglobalrecordImm16Id16);
    LOG_INST() << "    BaselineStconsttoglobalrecordImm16Id16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(static_cast<int32_t>(stringId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}
// ------- End parse bytecodes about reading and storing general variables -------

// ------- parse bytecodes about functions -------
BYTECODE_BASELINE_HANDLER_IMPLEMENT(ASYNCFUNCTIONENTER)
{
    (void)bytecodeArray;

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineAsyncfunctionenter);
    LOG_INST() << "    BaselineAsyncfunctionenter Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEFINEFUNC_IMM8_ID16_IMM8)
{
    uint8_t slotId = READ_INST_8_0();
    uint16_t methodId = READ_INST_16_1();
    uint8_t length = READ_INST_8_3();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDefinefuncImm8Id16Imm8);
    LOG_INST() << "    BaselineDefinefuncImm8Id16Imm8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      methodId: " << static_cast<int32_t>(methodId);
    LOG_INST() << "      length: " << static_cast<int32_t>(length);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(methodId));
    parameters.emplace_back(static_cast<int32_t>(length));
    parameters.emplace_back(static_cast<int32_t>(slotId));

    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEFINEFUNC_IMM16_ID16_IMM8)
{
    uint16_t slotId = READ_INST_16_0();
    uint16_t methodId = READ_INST_16_2();
    uint8_t length = READ_INST_8_4();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDefinefuncImm16Id16Imm8);
    LOG_INST() << "    BaselineDefinefuncImm16Id16Imm8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      methodId: " << static_cast<int32_t>(methodId);
    LOG_INST() << "      length: " << static_cast<int32_t>(length);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(methodId));
    parameters.emplace_back(static_cast<int32_t>(length));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEFINEMETHOD_IMM8_ID16_IMM8)
{
    uint16_t methodId = READ_INST_16_1();
    int8_t length = READ_INST_8_3();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDefinemethodImm8Id16Imm8);
    LOG_INST() << "    BaselineDefinemethodImm8Id16Imm8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      methodId: " << static_cast<int32_t>(methodId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(static_cast<int32_t>(methodId));
    parameters.emplace_back(length);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEFINEMETHOD_IMM16_ID16_IMM8)
{
    uint16_t methodId = READ_INST_16_2();
    int8_t length = READ_INST_8_4();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDefinemethodImm16Id16Imm8);
    LOG_INST() << "    BaselineDefinemethodImm16Id16Imm8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      methodId: " << static_cast<int32_t>(methodId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(static_cast<int32_t>(methodId));
    parameters.emplace_back(length);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLARG0_IMM8)
{
    uint8_t slotId = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCallarg0Imm8);
    LOG_INST() << "    BaselineCallarg0Imm8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLARG1_IMM8_V8)
{
    uint8_t slotId = READ_INST_8_0();
    uint8_t argNo = READ_INST_8_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCallArg1Imm8V8);
    LOG_INST() << "    BaselineCallArg1Imm8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(argNo));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLARGS2_IMM8_V8_V8)
{
    uint8_t slotId = READ_INST_8_0();
    uint8_t arg1No = READ_INST_8_1();
    uint8_t arg2No = READ_INST_8_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCallargs2Imm8V8V8);
    LOG_INST() << "    BaselineCallargs2Imm8V8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(arg1No));
    parameters.emplace_back(static_cast<int32_t>(arg2No));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLARGS3_IMM8_V8_V8_V8)
{
    uint8_t slotId = READ_INST_8_0();
    uint8_t arg1No = READ_INST_8_1();
    uint8_t arg2No = READ_INST_8_2();
    uint8_t arg3No = READ_INST_8_3();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCallargs3Imm8V8V8V8);
    LOG_INST() << "    BaselineCallargs3Imm8V8V8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(arg1No));
    parameters.emplace_back(static_cast<int32_t>(arg2No));
    parameters.emplace_back(static_cast<int32_t>(arg3No));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRANGE_IMM8_IMM8_V8)
{
    uint8_t slotId = READ_INST_8_0();
    uint8_t numArgs = READ_INST_8_1();
    uint8_t arg = READ_INST_8_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCallrangeImm8Imm8V8);
    LOG_INST() << "    BaselineCallrangeImm8Imm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      numArgs: " << static_cast<int32_t>(numArgs);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(numArgs));
    parameters.emplace_back(static_cast<int32_t>(arg));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLTHIS0_IMM8_V8)
{
    uint8_t slotId = READ_INST_8_0();
    uint8_t thisObjNo = READ_INST_8_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCallthis0Imm8V8);
    LOG_INST() << "    BaselineCallthis0Imm8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(thisObjNo));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLTHIS1_IMM8_V8_V8)
{
    uint8_t slotId = READ_INST_8_0();
    uint8_t thisObjId = READ_INST_8_1();
    uint8_t argId = READ_INST_8_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCallthis1Imm8V8V8);
    LOG_INST() << "    BaselineCallthis1Imm8V8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      thisObjId: " << static_cast<int16_t>(thisObjId);
    LOG_INST() << "      argId: " << static_cast<int16_t>(argId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(thisObjId));
    parameters.emplace_back(static_cast<int32_t>(argId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLTHIS2_IMM8_V8_V8_V8)
{
    uint8_t slotId = READ_INST_8_0();
    uint8_t thisObjId = READ_INST_8_1();
    uint8_t arg1Id = READ_INST_8_2();
    uint8_t arg2Id = READ_INST_8_3();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCallthis2Imm8V8V8V8);
    LOG_INST() << "    BaselineCallthis2Imm8V8V8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(thisObjId));
    parameters.emplace_back(static_cast<int32_t>(arg1Id));
    parameters.emplace_back(static_cast<int32_t>(arg2Id));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLTHIS3_IMM8_V8_V8_V8_V8)
{
    uint8_t slotId = READ_INST_8_0();
    uint8_t thisObjId = READ_INST_8_1();
    uint8_t arg1Id = READ_INST_8_2();
    uint8_t arg2Id = READ_INST_8_3();
    uint8_t arg3Id = READ_INST_8_4();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCallthis3Imm8V8V8V8V8);
    LOG_INST() << "    BaselineCallthis3Imm8V8V8V8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(thisObjId));
    uint32_t argIds = static_cast<uint32_t>(arg1Id) |
                      (static_cast<uint32_t>(arg2Id) << ONE_BYTE_SIZE) |
                      (static_cast<uint32_t>(arg3Id) << TWO_BYTE_SIZE);
    parameters.emplace_back(static_cast<int32_t>(argIds));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLTHISRANGE_IMM8_IMM8_V8)
{
    uint8_t slotId = READ_INST_8_0();
    uint8_t numArgs = READ_INST_8_1();
    uint8_t thisObj = READ_INST_8_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCallthisrangeImm8Imm8V8);
    LOG_INST() << "    BaselineCallthisrangeImm8Imm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      numArgs: " << static_cast<int32_t>(numArgs);
    LOG_INST() << "      thisObjId: " << static_cast<int16_t>(thisObj);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(numArgs));
    parameters.emplace_back(static_cast<int32_t>(thisObj));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(SUPERCALLTHISRANGE_IMM8_IMM8_V8)
{
    uint8_t range = READ_INST_8_1();
    uint8_t v0 = READ_INST_8_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineSupercallthisrangeImm8Imm8V8);
    LOG_INST() << "    BaselineSupercallthisrangeImm8Imm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      range: " << static_cast<int16_t>(range);
    LOG_INST() << "      v0: " << static_cast<int16_t>(v0);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int16_t>(range));
    parameters.emplace_back(static_cast<int16_t>(v0));
    parameters.emplace_back(BaselineSpecialParameter::HOTNESS_COUNTER);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(SUPERCALLARROWRANGE_IMM8_IMM8_V8)
{
    int8_t range = READ_INST_8_1();
    uint8_t v0 = READ_INST_8_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineSupercallarrowrangeImm8Imm8V8);
    LOG_INST() << "    BaselineSupercallarrowrangeImm8Imm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      v0: " << static_cast<int16_t>(v0);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(range);
    parameters.emplace_back(static_cast<int16_t>(v0));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_SUPERCALLFORWARDALLARGS_PREF_V8)
{
    uint8_t v0 = READ_INST_8_1();
    auto *thread = vm->GetAssociatedJSThread();

    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCallRuntimeSupercallforwardallargsPrefV8);
    LOG_INST() << "    BaselineCallRuntimeSupercallforwardallargsPrefV8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      v0: " << static_cast<int16_t>(v0);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(v0);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(SUPERCALLSPREAD_IMM8_V8)
{
    int8_t argList = READ_INST_8_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineSupercallspreadImm8V8);
    LOG_INST() << "    BaselineSupercallspreadImm8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(VirtualRegister(argList));
    parameters.emplace_back(BaselineSpecialParameter::HOTNESS_COUNTER);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(APPLY_IMM8_V8_V8)
{
    int8_t object = READ_INST_8_1();
    int8_t argList = READ_INST_8_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineApplyImm8V8V8);
    LOG_INST() << "    BaselineApplyImm8V8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(VirtualRegister(object));
    parameters.emplace_back(VirtualRegister(argList));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEFINEGETTERSETTERBYVALUE_V8_V8_V8_V8)
{
    uint8_t objectVregId = READ_INST_8_0();
    uint8_t propKeyVregId = READ_INST_8_1();
    uint8_t getterVregId = READ_INST_8_2();
    uint8_t setterVregId = READ_INST_8_3();
    auto offset = static_cast<int32_t>(bytecodeArray - firstPC);

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDefinegettersetterbyvalueV8V8V8V8);
    LOG_INST() << "    BaselineDefinegettersetterbyvalueV8V8V8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    uint32_t vRegIds = static_cast<uint32_t>(objectVregId) |
                       (static_cast<uint32_t>(propKeyVregId) << ONE_BYTE_SIZE) |
                       (static_cast<uint32_t>(getterVregId) << TWO_BYTE_SIZE) |
                       (static_cast<uint32_t>(setterVregId) << THREE_BYTE_SIZE);
    parameters.emplace_back(offset);
    parameters.emplace_back(static_cast<int32_t>(vRegIds));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8)
{
    uint8_t slotId = READ_INST_8_0();
    int16_t methodId = READ_INST_16_1();
    int16_t literalId = READ_INST_16_3();
    int16_t count = READ_INST_16_5();
    int8_t v0 = READ_INST_8_7();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDefineclasswithbufferImm8Id16Id16Imm16V8);
    LOG_INST() << "    BaselineDefineclasswithbufferImm8Id16Id16Imm16V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    uint32_t methodAndLiteralId = static_cast<uint32_t>(methodId) | (static_cast<uint32_t>(literalId) << TWO_BYTE_SIZE);
    parameters.emplace_back(static_cast<int32_t>(methodAndLiteralId));
    parameters.emplace_back(count);
    parameters.emplace_back(v0);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8)
{
    uint16_t slotId = READ_INST_16_0();
    uint16_t methodId = READ_INST_16_2();
    uint16_t literalId = READ_INST_16_4();
    uint16_t count = READ_INST_16_6();
    uint8_t superClass = READ_INST_8_8();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDefineclasswithbufferImm16Id16Id16Imm16V8);
    LOG_INST() << "    BaselineDefineclasswithbufferImm16Id16Id16Imm16V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    uint32_t methodAndLiteralId = static_cast<uint32_t>(methodId) | (static_cast<uint32_t>(literalId) << TWO_BYTE_SIZE);
    uint32_t countAndSuperClassId = static_cast<uint32_t>(count) | (static_cast<uint32_t>(superClass) << TWO_BYTE_SIZE);
    parameters.emplace_back(static_cast<int32_t>(methodAndLiteralId));
    parameters.emplace_back(static_cast<int32_t>(countAndSuperClassId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(GETRESUMEMODE)
{
    (void)bytecodeArray;

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineGetresumemod);
    LOG_INST() << "    BaselineGetresumemod Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(GETTEMPLATEOBJECT_IMM8)
{
    (void)bytecodeArray;

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineGettemplateobjectImm8);
    LOG_INST() << "    BaselineGettemplateobjectImm8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(GETTEMPLATEOBJECT_IMM16)
{
    (void)bytecodeArray;

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineGettemplateobjectImm16);
    LOG_INST() << "    BaselineGettemplateobjectImm16 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(GETNEXTPROPNAME_V8)
{
    int8_t iter = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineGetnextpropnameV8);
    LOG_INST() << "    BaselineGetnextpropnameV8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(iter));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(GETMODULENAMESPACE_IMM8)
{
    uint8_t index = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineGetmodulenamespaceImm8);
    LOG_INST() << "    BaselineGetmodulenamespaceImm8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      index: " << static_cast<int32_t>(index);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(static_cast<int32_t>(index));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(ASYNCFUNCTIONAWAITUNCAUGHT_V8)
{
    int8_t object = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineAsyncfunctionawaituncaughtV8);
    LOG_INST() << "    BaselineAsyncfunctionawaituncaughtV8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(VirtualRegister(object));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(ASYNCFUNCTIONRESOLVE_V8)
{
    int8_t object = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineAsyncfunctionresolveV8);
    LOG_INST() << "    BaselineAsyncfunctionresolveV8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(VirtualRegister(object));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(ASYNCFUNCTIONREJECT_V8)
{
    int8_t object = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineAsyncfunctionrejectV8);
    LOG_INST() << "    BaselineAsyncfunctionrejectV8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(VirtualRegister(object));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(COPYDATAPROPERTIES_V8)
{
    int8_t object = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCopydatapropertiesV8);
    LOG_INST() << "    BaselineCopydatapropertiesV8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(VirtualRegister(object));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(COPYRESTARGS_IMM8)
{
    uint8_t restIdx = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCopyrestargsImm8);
    LOG_INST() << "    BaselineCopyrestargsImm8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      restIdx: " << static_cast<int32_t>(restIdx);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(static_cast<int32_t>(restIdx));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(DYNAMICIMPORT)
{
    (void)bytecodeArray;

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDynamicimport);
    LOG_INST() << "    BaselineDynamicimport Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(RESUMEGENERATOR)
{
    (void)bytecodeArray;

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineResumegenerator);
    LOG_INST() << "    BaselineResumegenerator Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(RETURNUNDEFINED)
{
    auto offset = static_cast<int32_t>(bytecodeArray - firstPC);
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineReturnundefined);
    LOG_INST() << "    BaselineReturnundefined Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(offset);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(RETURN)
{
    auto offset = static_cast<int32_t>(bytecodeArray - firstPC);

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineReturn);
    LOG_INST() << "    BaselineReturn Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(offset);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}
// ------- End parse bytecodes about functions -------

// ------- parse bytecodes about arithmetic  -------
BYTECODE_BASELINE_HANDLER_IMPLEMENT(ADD2_IMM8_V8)
{
    uint8_t vLeftId = READ_INST_8_1();
    uint8_t slotId = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineAdd2Imm8V8);
    LOG_INST() << "    BaselineAdd2Imm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vLeftId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(SUB2_IMM8_V8)
{
    uint8_t vLeftId = READ_INST_8_1();
    uint8_t slotId = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineSub2Imm8V8);
    LOG_INST() << "    BaselineSub2Imm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vLeftId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(MUL2_IMM8_V8)
{
    uint8_t vLeftId = READ_INST_8_1();
    uint8_t slotId = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineMul2Imm8V8);
    LOG_INST() << "    BaselineMul2Imm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vLeftId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(DIV2_IMM8_V8)
{
    uint8_t vLeftId = READ_INST_8_1();
    uint8_t slotId = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDiv2Imm8V8);
    LOG_INST() << "    BaselineDiv2Imm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vLeftId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(MOD2_IMM8_V8)
{
    uint8_t vLeftId = READ_INST_8_1();
    uint8_t slotId = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineMod2Imm8V8);
    LOG_INST() << "    BaselineMod2Imm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vLeftId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(SHL2_IMM8_V8)
{
    uint8_t vLeftId = READ_INST_8_1();
    uint8_t slotId = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineShl2Imm8V8);
    LOG_INST() << "    BaselineShl2Imm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vLeftId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(SHR2_IMM8_V8)
{
    uint8_t vLeftId = READ_INST_8_1();
    uint8_t slotId = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineShr2Imm8V8);
    LOG_INST() << "    BaselineShr2Imm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vLeftId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(ASHR2_IMM8_V8)
{
    uint8_t vLeftId = READ_INST_8_1();
    uint8_t slotId = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineAshr2Imm8V8);
    LOG_INST() << "    BaselineAshr2Imm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vLeftId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(AND2_IMM8_V8)
{
    uint8_t vLeftId = READ_INST_8_1();
    uint8_t slotId = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineAnd2Imm8V8);
    LOG_INST() << "    BaselineAnd2Imm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vLeftId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(OR2_IMM8_V8)
{
    uint8_t vLeftId = READ_INST_8_1();
    uint8_t slotId = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineOr2Imm8V8);
    LOG_INST() << "    BaselineOr2Imm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vLeftId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(XOR2_IMM8_V8)
{
    uint8_t vLeftId = READ_INST_8_1();
    uint8_t slotId = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineXor2Imm8V8);
    LOG_INST() << "    BaselineXor2Imm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vLeftId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(EXP_IMM8_V8)
{
    uint8_t vBaseId = READ_INST_8_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineExpImm8V8);
    LOG_INST() << "    BaselineExpImm8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vBaseId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(NEG_IMM8)
{
    uint8_t slotId = READ_INST_8_0();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineNegImm8);
    LOG_INST() << "    BaselineNegImm8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(NOT_IMM8)
{
    uint8_t slotId = READ_INST_8_0();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineNotImm8);
    LOG_INST() << "    BaselineNotImm8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(INC_IMM8)
{
    uint8_t slotId = READ_INST_8_0();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineIncImm8);
    LOG_INST() << "    BaselineIncImm8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEC_IMM8)
{
    uint8_t slotId = READ_INST_8_0();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDecImm8);
    LOG_INST() << "    BaselineDecImm8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(TONUMBER_IMM8)
{
    (void)bytecodeArray;

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineTonumberImm8);
    LOG_INST() << "    BaselineTonumberImm8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(TONUMERIC_IMM8)
{
    (void)bytecodeArray;

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineTonumericImm8);
    LOG_INST() << "    BaselineTonumericImm8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}
// ------- End parse bytecodes about arithmetic  -------

// ------- parse bytecodes about comparison and check -------
BYTECODE_BASELINE_HANDLER_IMPLEMENT(EQ_IMM8_V8)
{
    uint8_t vLeftId = READ_INST_8_1();
    uint8_t slotId = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineEqImm8V8);
    LOG_INST() << "    BaselineEqImm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vLeftId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(NOTEQ_IMM8_V8)
{
    uint8_t vLeftId = READ_INST_8_1();
    uint8_t slotId = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineNoteqImm8V8);
    LOG_INST() << "    BaselineNoteqImm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vLeftId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LESS_IMM8_V8)
{
    uint8_t vLeftId = READ_INST_8_1();
    uint8_t slotId = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLessImm8V8);
    LOG_INST() << "    BaselineLessImm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vLeftId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(LESSEQ_IMM8_V8)
{
    uint8_t vLeftId = READ_INST_8_1();
    uint8_t slotId = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLesseqImm8V8);
    LOG_INST() << "    BaselineLesseqImm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vLeftId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(GREATER_IMM8_V8)
{
    uint8_t vLeftId = READ_INST_8_1();
    uint8_t slotId = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineGreaterImm8V8);
    LOG_INST() << "    BaselineGreaterImm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vLeftId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(GREATEREQ_IMM8_V8)
{
    uint8_t vLeftId = READ_INST_8_1();
    uint8_t slotId = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineGreatereqImm8V8);
    LOG_INST() << "    BaselineGreatereqImm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vLeftId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(ISIN_IMM8_V8)
{
    uint8_t vSrcId = READ_INST_8_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineIsinImm8V8);
    LOG_INST() << "    BaselineIsinImm8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(VirtualRegister(vSrcId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(INSTANCEOF_IMM8_V8)
{
    uint8_t slotId = READ_INST_8_0();
    uint8_t vObjId = READ_INST_8_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineInstanceofImm8V8);
    LOG_INST() << "    BaselineInstanceofImm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(vObjId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STRICTNOTEQ_IMM8_V8)
{
    uint8_t slotId = READ_INST_8_0();
    uint8_t vLeftId = READ_INST_8_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStrictnoteqImm8V8);
    LOG_INST() << "    BaselineStrictnoteqImm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vLeftId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(STRICTEQ_IMM8_V8)
{
    uint8_t slotId = READ_INST_8_0();
    uint8_t vLeftId = READ_INST_8_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStricteqImm8V8);
    LOG_INST() << "    BaselineStricteqImm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(vLeftId));
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(ISTRUE)
{
    (void)bytecodeArray;

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineIstrue);
    LOG_INST() << "    BaselineIstrue Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_ISTRUE_PREF_IMM8)
{
    uint8_t slotId = READ_INST_8_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCallRuntimeIstruePrefImm8);
    LOG_INST() << "    BaselineCallRuntimeIstruePrefImm8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(ISFALSE)
{
    (void)bytecodeArray;

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineIsfalse);
    LOG_INST() << "    BaselineIsfalse Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_ISFALSE_PREF_IMM8)
{
    uint8_t slotId = READ_INST_8_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCallRuntimeIsfalsePrefImm8);
    LOG_INST() << "    BaselineCallRuntimeIsfalsePrefImm8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// ------- End parse bytecodes about comparison  -------

// ------- parse bytecodes about control flow  -------
BYTECODE_BASELINE_HANDLER_IMPLEMENT(JEQZ_IMM8)
{
    int8_t offset = static_cast<int8_t>(READ_INST_8_0());
    size_t pos = offset + bytecodeOffset;

    JumpLabel *jump = nullptr;
    if (jumpMap.count(pos) != 0) {
        jump = jumpMap.at(pos);
    } else {
        jump = new JumpLabel();
        jumpMap.insert({pos, jump});
    }

    GetBaselineAssembler().Cmp(SpecialRegister::ACC_REGISTER, Immediate(JSTaggedValue::VALUE_FALSE));
    JumpLabel exitLabel;
    GetBaselineAssembler().Jnz(exitLabel);
    CALL_BASELINE_UPDATE_HOTNESS();
    GetBaselineAssembler().Jump(*jump);
    GetBaselineAssembler().Bind(exitLabel);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JEQZ_IMM16)
{
    int16_t offset = READ_INST_16_0();
    size_t pos = offset + bytecodeOffset;

    JumpLabel *jump = nullptr;
    if (jumpMap.count(pos) != 0) {
        jump = jumpMap.at(pos);
    } else {
        jump = new JumpLabel();
        jumpMap.insert({pos, jump});
    }

    GetBaselineAssembler().Cmp(SpecialRegister::ACC_REGISTER, Immediate(JSTaggedValue::VALUE_FALSE));
    JumpLabel exitLabel;
    GetBaselineAssembler().Jnz(exitLabel);
    CALL_BASELINE_UPDATE_HOTNESS();
    GetBaselineAssembler().Jump(*jump);
    GetBaselineAssembler().Bind(exitLabel);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JEQZ_IMM32)
{
    int32_t offset = static_cast<int32_t>(READ_INST_32_0());
    size_t pos = static_cast<size_t>(offset + static_cast<int32_t>(bytecodeOffset));

    JumpLabel *jump = nullptr;
    if (jumpMap.count(pos) != 0) {
        jump = jumpMap.at(pos);
    } else {
        jump = new JumpLabel();
        jumpMap.insert({pos, jump});
    }

    JumpLabel exitLabel;
    GetBaselineAssembler().Cmp(SpecialRegister::ACC_REGISTER, Immediate(JSTaggedValue::VALUE_FALSE));
    GetBaselineAssembler().Jnz(exitLabel);
    CALL_BASELINE_UPDATE_HOTNESS();
    GetBaselineAssembler().Jump(*jump);
    GetBaselineAssembler().Bind(exitLabel);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JNEZ_IMM8)
{
    int8_t offset = READ_INST_8_0();
    size_t pos = offset + bytecodeOffset;

    JumpLabel *jump = nullptr;
    if (jumpMap.count(pos) != 0) {
        jump = jumpMap.at(pos);
    } else {
        jump = new JumpLabel();
        jumpMap.insert({pos, jump});
    }

    GetBaselineAssembler().Cmp(SpecialRegister::ACC_REGISTER, Immediate(JSTaggedValue::VALUE_FALSE));
    JumpLabel labelExit;
    GetBaselineAssembler().Jz(labelExit);

    CALL_BASELINE_UPDATE_HOTNESS();
    GetBaselineAssembler().Jump(*jump);

    GetBaselineAssembler().Bind(labelExit);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JNEZ_IMM16)
{
    int16_t offset = READ_INST_16_0();
    size_t pos = offset + bytecodeOffset;

    JumpLabel *jump = nullptr;
    if (jumpMap.count(pos) != 0) {
        jump = jumpMap.at(pos);
    } else {
        jump = new JumpLabel();
        jumpMap.insert({pos, jump});
    }

    GetBaselineAssembler().Cmp(SpecialRegister::ACC_REGISTER, Immediate(JSTaggedValue::VALUE_FALSE));
    JumpLabel labelExit;
    GetBaselineAssembler().Jz(labelExit);

    CALL_BASELINE_UPDATE_HOTNESS();
    GetBaselineAssembler().Jump(*jump);

    GetBaselineAssembler().Bind(labelExit);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JNEZ_IMM32)
{
    int32_t offset = static_cast<int32_t>(READ_INST_32_0());
    size_t pos = static_cast<size_t>(offset + static_cast<int32_t>(bytecodeOffset));

    JumpLabel *jump = nullptr;
    if (jumpMap.count(pos) != 0) {
        jump = jumpMap.at(pos);
    } else {
        jump = new JumpLabel();
        jumpMap.insert({pos, jump});
    }

    GetBaselineAssembler().Cmp(SpecialRegister::ACC_REGISTER, Immediate(JSTaggedValue::VALUE_FALSE));
    JumpLabel labelExit;
    GetBaselineAssembler().Jz(labelExit);

    CALL_BASELINE_UPDATE_HOTNESS();
    GetBaselineAssembler().Jump(*jump);

    GetBaselineAssembler().Bind(labelExit);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JEQ_V8_IMM8)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JEQ_V8_IMM16)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JNE_V8_IMM8)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JNE_V8_IMM16)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JMP_IMM8)
{
    int8_t offset = READ_INST_8_0();
    size_t pos = offset + bytecodeOffset;

    JumpLabel *jump = nullptr;
    if (jumpMap.count(pos) != 0) {
        jump = jumpMap.at(pos);
    } else {
        jump = new JumpLabel();
        jumpMap.insert({pos, jump});
    }

    CALL_BASELINE_UPDATE_HOTNESS();
    GetBaselineAssembler().Jump(*jump);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JMP_IMM16)
{
    int16_t offset = READ_INST_16_0();
    size_t pos = offset + bytecodeOffset;

    JumpLabel *jump = nullptr;
    if (jumpMap.count(pos) != 0) {
        jump = jumpMap.at(pos);
    } else {
        jump = new JumpLabel();
        jumpMap.insert({pos, jump});
    }

    CALL_BASELINE_UPDATE_HOTNESS();
    GetBaselineAssembler().Jump(*jump);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JMP_IMM32)
{
    int32_t offset = static_cast<int32_t>(READ_INST_32_0());
    size_t pos = static_cast<size_t>(offset + static_cast<int32_t>(bytecodeOffset));

    JumpLabel *jump = nullptr;
    if (jumpMap.count(pos) != 0) {
        jump = jumpMap.at(pos);
    } else {
        jump = new JumpLabel();
        jumpMap.insert({pos, jump});
    }

    CALL_BASELINE_UPDATE_HOTNESS();
    GetBaselineAssembler().Jump(*jump);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JEQNULL_IMM8)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JEQNULL_IMM16)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JNENULL_IMM8)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JNENULL_IMM16)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JSTRICTEQZ_IMM8)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JSTRICTEQZ_IMM16)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JNSTRICTEQZ_IMM8)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JNSTRICTEQZ_IMM16)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JSTRICTEQNULL_IMM8)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JSTRICTEQNULL_IMM16)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JNSTRICTEQNULL_IMM8)
{
    (void)bytecodeArray;
}
// ------- End parse bytecodes about control flow  -------

// ------- parse other bytecodes -------
BYTECODE_BASELINE_HANDLER_IMPLEMENT(POPLEXENV)
{
    (void)bytecodeArray;

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselinePoplexenv);
    LOG_INST() << "    BaselinePoplexenv Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP); // state
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CREATEASYNCGENERATOROBJ_V8)
{
    uint8_t func = READ_INST_8_0();
    LOG_INST() << "    createasyncgeneratorobj v" << std::dec << func;

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCreateasyncgeneratorobjV8);
    LOG_INST() << "    BaselineCreateasyncgeneratorobjV8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(func));
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(DELOBJPROP_V8)
{
    int8_t object = READ_INST_8_0();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDelobjpropV8);
    LOG_INST() << "    BaselineDelobjpropV8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(VirtualRegister(object));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(SUSPENDGENERATOR_V8)
{
    uint8_t v0 = READ_INST_8_0();
    auto offset = static_cast<int32_t>(bytecodeArray - firstPC);
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineSuspendgeneratorV8);
    LOG_INST() << "    BaselineSuspendgeneratorV8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(offset);
    parameters.emplace_back(static_cast<int32_t>(v0));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEBUGGER)
{
    (void)bytecodeArray;

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDebugger);
    LOG_INST() << "    BaselineDebugger Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

// ------- parse wide bytecodes -------
// GLUE, SP, V0, INDEX, SLOT_ID
BYTECODE_BASELINE_HANDLER_IMPLEMENT(WIDE_STOBJBYINDEX_PREF_V8_IMM32)
{
    uint8_t slotId = READ_INST_8_0();
    uint8_t v0 = READ_INST_8_1();
    int32_t index = static_cast<int32_t>(READ_INST_32_2());
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineWideStobjbyindexPrefV8Imm32);
    LOG_INST() << "    BaselineWideStobjbyindexPrefV8Imm32 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(v0));
    parameters.emplace_back(index);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

// GLUE, SP, ACC, V0, INDEX
BYTECODE_BASELINE_HANDLER_IMPLEMENT(WIDE_STOWNBYINDEX_PREF_V8_IMM32)
{
    uint8_t slotId = READ_INST_8_0();
    uint8_t v0 = READ_INST_8_1();
    int32_t index = static_cast<int32_t>(READ_INST_32_2());
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineWideStownbyindexPrefV8Imm32);
    LOG_INST() << "    BaselineWideStownbyindexPrefV8Imm32 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(v0));
    parameters.emplace_back(index);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

// GLUE, SP, INDEX
BYTECODE_BASELINE_HANDLER_IMPLEMENT(WIDE_COPYRESTARGS_PREF_IMM16)
{
    uint16_t index = READ_INST_16_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineWideCopyrestargsPrefImm16);
    LOG_INST() << "    BaselineWideCopyrestargsPrefImm16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      index: " << static_cast<int32_t>(index);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(index));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, LEVEL, SLOT
BYTECODE_BASELINE_HANDLER_IMPLEMENT(WIDE_LDLEXVAR_PREF_IMM16_IMM16)
{
    uint16_t level = READ_INST_16_1();
    uint16_t slot = READ_INST_16_3();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineWideLdlexvarPrefImm16Imm16);
    LOG_INST() << "    BaselineWideLdlexvarPrefImm16Imm16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      level: " << static_cast<int32_t>(level);
    LOG_INST() << "      slot: " << static_cast<int32_t>(slot);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(level));
    parameters.emplace_back(static_cast<int32_t>(slot));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, ACC, LEVEL, SLOT
BYTECODE_BASELINE_HANDLER_IMPLEMENT(WIDE_STLEXVAR_PREF_IMM16_IMM16)
{
    uint16_t level = READ_INST_16_1();
    uint16_t slot = READ_INST_16_3();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineWideStlexvarPrefImm16Imm16);
    LOG_INST() << "    BaselineWideStlexvarPrefImm16Imm16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      level: " << static_cast<int32_t>(level);
    LOG_INST() << "      slot: " << static_cast<int32_t>(slot);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(static_cast<int32_t>(level));
    parameters.emplace_back(static_cast<int32_t>(slot));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

// GLUE, ACC, INDEX
BYTECODE_BASELINE_HANDLER_IMPLEMENT(WIDE_GETMODULENAMESPACE_PREF_IMM16)
{
    int16_t index = READ_INST_16_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineWideGetmodulenamespacePrefImm16);
    LOG_INST() << "    BaselineWideGetmodulenamespacePrefImm16 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(index);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, ACC, INDEX
BYTECODE_BASELINE_HANDLER_IMPLEMENT(WIDE_STMODULEVAR_PREF_IMM16)
{
    int16_t index = READ_INST_16_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineWideStmodulevarPrefImm16);
    LOG_INST() << "    BaselineWideStmodulevarPrefImm16 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(index);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

// GLUE, INDEX
BYTECODE_BASELINE_HANDLER_IMPLEMENT(WIDE_LDLOCALMODULEVAR_PREF_IMM16)
{
    int16_t index = READ_INST_16_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineWideLdlocalmodulevarPrefImm16);
    LOG_INST() << "    BaselineWideLdlocalmodulevarPrefImm16 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(index);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, INDEX
BYTECODE_BASELINE_HANDLER_IMPLEMENT(WIDE_LDEXTERNALMODULEVAR_PREF_IMM16)
{
    int16_t index = READ_INST_16_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineWideLdexternalmodulevarPrefImm16);
    LOG_INST() << "    BaselineWideLdexternalmodulevarPrefImm16 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(index);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, INDEX
BYTECODE_BASELINE_HANDLER_IMPLEMENT(WIDE_LDPATCHVAR_PREF_IMM16)
{
    int16_t index = READ_INST_16_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineWideLdpatchvarPrefImm16);
    LOG_INST() << "    BaselineWideLdpatchvarPrefImm16 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(index);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, ACC, INDEX
BYTECODE_BASELINE_HANDLER_IMPLEMENT(WIDE_STPATCHVAR_PREF_IMM16)
{
    int16_t index = READ_INST_16_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineWideStpatchvarPrefImm16);
    LOG_INST() << "    BaselineWideStpatchvarPrefImm16 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(index);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

// GLUE, ACC, INDEX
BYTECODE_BASELINE_HANDLER_IMPLEMENT(WIDE_LDOBJBYINDEX_PREF_IMM32)
{
    uint8_t slotId = READ_INST_8_0();
    int32_t index = static_cast<int32_t>(READ_INST_32_1());
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineWideLdobjbyindexPrefImm32);
    LOG_INST() << "    BaselineWideLdobjbyindexPrefImm32 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(index);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, ACC, RANGE, V0_I8
BYTECODE_BASELINE_HANDLER_IMPLEMENT(WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8)
{
    int16_t range = READ_INST_16_1();
    uint8_t v0 = READ_INST_8_3();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineWideSupercallarrowrangePrefImm16V8);
    LOG_INST() << "    BaselineWideSupercallarrowrangePrefImm16V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      range: " << range;
    LOG_INST() << "      v0: " << static_cast<int16_t>(v0);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(range);
    parameters.emplace_back(static_cast<int16_t>(v0));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

// GLUE, SP, RANGE, V0_I8
BYTECODE_BASELINE_HANDLER_IMPLEMENT(WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8)
{
    int16_t range = READ_INST_16_1();
    uint8_t v0 = READ_INST_8_3();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineWideSupercallthisrangePrefImm16V8);
    LOG_INST() << "    BaselineWideSupercallthisrangePrefImm16V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      range: " << range;
    LOG_INST() << "      v0: " << static_cast<int16_t>(v0);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(range);
    parameters.emplace_back(static_cast<int16_t>(v0));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, ACC, ACTUAL_NUM_ARGS, VREG_ID, HOTNESS_COUNTER
BYTECODE_BASELINE_HANDLER_IMPLEMENT(WIDE_CALLTHISRANGE_PREF_IMM16_V8)
{
    uint16_t actualNumArgs = READ_INST_16_1();
    int8_t vregId = READ_INST_8_3();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineWideCallthisrangePrefImm16V8);
    LOG_INST() << "    BaselineWideCallthisrangePrefImm16V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      actualNumArgs: " << static_cast<int32_t>(actualNumArgs);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(static_cast<int32_t>(actualNumArgs));
    parameters.emplace_back(vregId);
    parameters.emplace_back(BaselineSpecialParameter::HOTNESS_COUNTER);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, ACC, ACTUAL_NUM_ARGS, VREG_ID, HOTNESS_COUNTER
BYTECODE_BASELINE_HANDLER_IMPLEMENT(WIDE_CALLRANGE_PREF_IMM16_V8)
{
    uint16_t actualNumArgs = READ_INST_16_1();
    int8_t vregId = READ_INST_8_2();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineWideCallrangePrefImm16V8);
    LOG_INST() << "    BaselineWideCallrangePrefImm16V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      actualNumArgs: " << static_cast<int32_t>(actualNumArgs);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(static_cast<int32_t>(actualNumArgs));
    parameters.emplace_back(vregId);
    parameters.emplace_back(BaselineSpecialParameter::HOTNESS_COUNTER);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, ACC, NUM_VARS, SCOPE_ID
BYTECODE_BASELINE_HANDLER_IMPLEMENT(WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16)
{
    int16_t numVars = READ_INST_16_1();
    int16_t scopeId = READ_INST_16_3();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineWideNewlexenvwithnamePrefImm16Id16);
    LOG_INST() << "    BaselineWideNewlexenvwithnamePrefImm16Id16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      numVars: " << numVars;
    LOG_INST() << "      scopeId: " << scopeId;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(numVars);
    parameters.emplace_back(scopeId);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, ACC, NUM_VARS
BYTECODE_BASELINE_HANDLER_IMPLEMENT(WIDE_NEWLEXENV_PREF_IMM16)
{
    uint16_t numVars = READ_INST_16_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineWideNewlexenvPrefImm16);
    LOG_INST() << "    BaselineWideNewlexenvPrefImm16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      numVars: " << static_cast<int32_t>(numVars);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(static_cast<int32_t>(numVars));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, NUM_ARGS, IDX, HOTNESS_COUNTER, SLOT_ID
BYTECODE_BASELINE_HANDLER_IMPLEMENT(WIDE_NEWOBJRANGE_PREF_IMM16_V8)
{
    uint16_t slotId = READ_INST_16_0();
    int16_t numArgs = READ_INST_16_1();
    uint8_t firstArgRegIdx = READ_INST_8_3();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineWideNewobjrangePrefImm16V8);
    LOG_INST() << "    BaselineWideNewobjrangePrefImm16V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      numArgs: " << numArgs;
    LOG_INST() << "      firstArgRegIdx: " << static_cast<int16_t>(firstArgRegIdx);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(numArgs);
    parameters.emplace_back(static_cast<int16_t>(firstArgRegIdx));
    parameters.emplace_back(BaselineSpecialParameter::HOTNESS_COUNTER);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, V0, V1, V2
BYTECODE_BASELINE_HANDLER_IMPLEMENT(WIDE_CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8)
{
    int16_t numKeys = READ_INST_16_1();
    int8_t objId = READ_INST_8_3();
    uint8_t firstArgRegIdx = READ_INST_8_4();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineWideCreateobjectwithexcludedkeysPrefImm16V8V8);
    LOG_INST() << "    BaselineWideCreateobjectwithexcludedkeysPrefImm16V8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      numKeys: " << numKeys;
    LOG_INST() << "      objId: " << static_cast<int16_t>(objId);
    LOG_INST() << "      firstArgRegIdx: " << static_cast<int16_t>(firstArgRegIdx);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(numKeys);
    parameters.emplace_back(objId);
    parameters.emplace_back(static_cast<int16_t>(firstArgRegIdx));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// ------- End parse wide bytecodes  -------

// ------- parse throw bytecodes -------

// GLUE, SP
BYTECODE_BASELINE_HANDLER_IMPLEMENT(THROW_PATTERNNONCOERCIBLE_PREF_NONE)
{
    (void)bytecodeArray;
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineThrowPatternnoncoerciblePrefNone);
    LOG_INST() << "    BaselineThrowPatternnoncoerciblePrefNone Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(THROW_NOTEXISTS_PREF_NONE)
{
    (void)bytecodeArray;
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineThrowNotexistsPrefNone);
    LOG_INST() << "    BaselineThrowNotexistsPrefNone Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

// GLUE, SP
BYTECODE_BASELINE_HANDLER_IMPLEMENT(THROW_DELETESUPERPROPERTY_PREF_NONE)
{
    (void)bytecodeArray;
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineThrowDeletesuperpropertyPrefNone);
    LOG_INST() << "    BaselineThrowDeletesuperpropertyPrefNone Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

// GLUE, ACC, IMM
BYTECODE_BASELINE_HANDLER_IMPLEMENT(THROW_IFSUPERNOTCORRECTCALL_PREF_IMM16)
{
    int16_t imm = READ_INST_16_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineThrowIfsupernotcorrectcallPrefImm16);
    LOG_INST() << "    BaselineThrowIfsupernotcorrectcallPrefImm16 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(imm);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

// GLUE, ACC, IMM
BYTECODE_BASELINE_HANDLER_IMPLEMENT(THROW_IFSUPERNOTCORRECTCALL_PREF_IMM8)
{
    int8_t imm = READ_INST_8_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineThrowIfsupernotcorrectcallPrefImm8);
    LOG_INST() << "    BaselineThrowIfsupernotcorrectcallPrefImm8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(imm);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

// GLUE, ACC, SP, STRING_ID
BYTECODE_BASELINE_HANDLER_IMPLEMENT(THROW_UNDEFINEDIFHOLEWITHNAME_PREF_ID16)
{
    uint16_t stringId = READ_INST_16_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineThrowUndefinedifholewithnamePrefId16);
    LOG_INST() << "    BaselineThrowUndefinedifholewithnamePrefId16 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(stringId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

// GLUE, SP, V0, V1
BYTECODE_BASELINE_HANDLER_IMPLEMENT(THROW_UNDEFINEDIFHOLE_PREF_V8_V8)
{
    int8_t v0 = READ_INST_8_1();
    int8_t v1 = READ_INST_8_2();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineThrowUndefinedifholePrefV8V8);
    LOG_INST() << "    BaselineThrowUndefinedifholePrefV8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(v0);
    parameters.emplace_back(v1);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

// GLUE, SP, V0
BYTECODE_BASELINE_HANDLER_IMPLEMENT(THROW_IFNOTOBJECT_PREF_V8)
{
    int8_t v0 = READ_INST_8_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineThrowIfnotobjectPrefV8);
    LOG_INST() << "    BaselineThrowIfnotobjectPrefV8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(v0));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(THROW_CONSTASSIGNMENT_PREF_V8)
{
    int8_t v0 = READ_INST_8_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineThrowConstassignmentPrefV8);
    LOG_INST() << "    BaselineThrowConstassignmentPrefV8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(VirtualRegister(v0));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

// GLUE, SP, ACC
BYTECODE_BASELINE_HANDLER_IMPLEMENT(THROW_PREF_NONE)
{
    (void)bytecodeArray;
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineThrowPrefNone);
    LOG_INST() << "    BaselineThrowPrefNone Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

// ------- End parse throw bytecodes  -------

// ------- parse SET/GET bytecodes -------

// GLUE, SP, ACC, INDEX0, INDEX1, ENV
BYTECODE_BASELINE_HANDLER_IMPLEMENT(TESTIN_IMM8_IMM16_IMM16)
{
    uint16_t levelIndex = READ_INST_16_1();
    uint16_t slotIndex = READ_INST_16_3();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineTestInImm8Imm16Imm16);
    LOG_INST() << "    BaselineTestInImm8Imm16Imm16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      levelIndex: " << static_cast<int32_t>(levelIndex);
    LOG_INST() << "      slotIndex: " << static_cast<int32_t>(slotIndex);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(static_cast<int32_t>(levelIndex));
    parameters.emplace_back(static_cast<int32_t>(slotIndex));
    parameters.emplace_back(BaselineSpecialParameter::ENV);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, INDEX0, INDEX1, INDEX2
BYTECODE_BASELINE_HANDLER_IMPLEMENT(STPRIVATEPROPERTY_IMM8_IMM16_IMM16_V8)
{
    uint16_t levelIndex = READ_INST_16_1();
    uint16_t slotIndex = READ_INST_16_3();
    int8_t objId = READ_INST_8_5();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineStPrivatePropertyImm8Imm16Imm16V8);
    LOG_INST() << "    BaselineStPrivatePropertyImm8Imm16Imm16V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      levelIndex: " << static_cast<int32_t>(levelIndex);
    LOG_INST() << "      slotIndex: " << static_cast<int32_t>(slotIndex);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(levelIndex));
    parameters.emplace_back(static_cast<int32_t>(slotIndex));
    parameters.emplace_back(objId);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, ACC, INDEX0, INDEX1, ENV
BYTECODE_BASELINE_HANDLER_IMPLEMENT(LDPRIVATEPROPERTY_IMM8_IMM16_IMM16)
{
    uint16_t levelIndex = READ_INST_16_1();
    uint16_t slotIndex = READ_INST_16_3();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineLdPrivatePropertyImm8Imm16Imm16);
    LOG_INST() << "    BaselineLdPrivatePropertyImm8Imm16Imm16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      levelIndex: " << static_cast<int32_t>(levelIndex);
    LOG_INST() << "      slotIndex: " << static_cast<int32_t>(slotIndex);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(static_cast<int32_t>(levelIndex));
    parameters.emplace_back(static_cast<int32_t>(slotIndex));
    parameters.emplace_back(BaselineSpecialParameter::ENV);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, ACC
BYTECODE_BASELINE_HANDLER_IMPLEMENT(GETASYNCITERATOR_IMM8)
{
    (void)bytecodeArray;
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineGetasynciteratorImm8);
    LOG_INST() << "    BaselineGetasynciteratorImm8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, ACC, INDEX
BYTECODE_BASELINE_HANDLER_IMPLEMENT(SETGENERATORSTATE_IMM8)
{
    uint8_t v0 = READ_INST_8_0();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineSetgeneratorstateImm8);
    LOG_INST() << "    BaselineSetgeneratorstateImm8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      v0: " << static_cast<int32_t>(v0);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(static_cast<int32_t>(v0));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

// ------- End parse SET/GET bytecodes  -------

// ------- parse JUMP bytecodes -------

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JNSTRICTEQNULL_IMM16)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JEQUNDEFINED_IMM8)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JEQUNDEFINED_IMM16)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JNEUNDEFINED_IMM8)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JNEUNDEFINED_IMM16)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JSTRICTEQUNDEFINED_IMM8)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JSTRICTEQUNDEFINED_IMM16)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JNSTRICTEQUNDEFINED_IMM8)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JNSTRICTEQUNDEFINED_IMM16)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JSTRICTEQ_V8_IMM8)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JSTRICTEQ_V8_IMM16)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JNSTRICTEQ_V8_IMM8)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(JNSTRICTEQ_V8_IMM16)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(NOP)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(ASYNCGENERATORREJECT_V8)
{
    int8_t v0 = READ_INST_8_0();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineAsyncgeneratorrejectV8);
    LOG_INST() << "    BaselineAsyncgeneratorrejectV8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(v0);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(MOV_V16_V16)
{
    uint16_t vdst = READ_INST_16_0();
    uint16_t vsrc = READ_INST_16_2();
    LOG_INST() << "mov v" << static_cast<VRegIDType>(vdst) << ", v" << static_cast<VRegIDType>(vsrc);
    GetBaselineAssembler().Move(VirtualRegister(static_cast<VRegIDType>(vdst)),
                                VirtualRegister(static_cast<VRegIDType>(vsrc)));
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(MOV_V8_V8)
{
    uint8_t vdst = READ_INST_8_0();
    uint8_t vsrc = READ_INST_8_1();
    LOG_INST() << "mov v" << static_cast<VRegIDType>(vdst) << ", v" << static_cast<VRegIDType>(vsrc);
    GetBaselineAssembler().Move(VirtualRegister(static_cast<VRegIDType>(vdst)),
                                VirtualRegister(static_cast<VRegIDType>(vsrc)));
}

// GLUE, SP, VDST, VSRC
BYTECODE_BASELINE_HANDLER_IMPLEMENT(MOV_V4_V4)
{
    uint8_t vdst = READ_INST_4_0();
    uint8_t vsrc = READ_INST_4_1();
    LOG_INST() << "mov v" << static_cast<VRegIDType>(vdst) << ", v" << static_cast<VRegIDType>(vsrc);
    GetBaselineAssembler().Move(VirtualRegister(static_cast<VRegIDType>(vdst)),
                                VirtualRegister(static_cast<VRegIDType>(vsrc)));
}

// ------- End parse JUMP bytecodes  -------

// ------- parse deprecated bytecodes -------

// GLUE, SP, ACC, PROFILE_TYPE_INFO, HOTNESS_COUNTER, V0, V1
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_SUSPENDGENERATOR_PREF_V8_V8)
{
    int8_t v0 = READ_INST_8_1();
    int8_t v1 = READ_INST_8_2();
    auto offset = static_cast<int32_t>(bytecodeArray - firstPC);
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedSuspendgeneratorPrefV8V8);
    LOG_INST() << "    BaselineDeprecatedSuspendgeneratorPrefV8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(offset);
    parameters.emplace_back(v0);
    parameters.emplace_back(v1);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

// ACC, SP
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_LDLEXENV_PREF_NONE)
{
    (void)bytecodeArray;
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedLdlexenvPrefNone);
    LOG_INST() << "    BaselineDeprecatedLdlexenvPrefNone Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_POPLEXENV_PREF_NONE)
{
    (void)bytecodeArray;
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedPoplexenvPrefNone);
    LOG_INST() << "    BaselineDeprecatedPoplexenvPrefNone Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

// GLUE, SP, V0, V1
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_GETITERATORNEXT_PREF_V8_V8)
{
    int8_t v0 = READ_INST_8_1();
    int8_t v1 = READ_INST_8_2();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedGetiteratornextPrefV8V8);
    LOG_INST() << "    BaselineDeprecatedGetiteratornextPrefV8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(v0);
    parameters.emplace_back(v1);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, IMM_I16, FUNC, SLOT_ID_I8, PROFILE_TYPE_INFO, PC
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_CREATEARRAYWITHBUFFER_PREF_IMM16)
{
    int16_t immI16 = READ_INST_16_1();
    uint8_t slotId = READ_INST_8_0();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedCreatearraywithbufferPrefImm16);
    LOG_INST() << "    BaselineDeprecatedCreatearraywithbufferPrefImm16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      immI16: " << immI16;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(immI16);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    parameters.emplace_back(BaselineSpecialParameter::PROFILE_TYPE_INFO);
    parameters.emplace_back(BaselineSpecialParameter::PC);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, IMM_I16, SP, FUNC
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_CREATEOBJECTWITHBUFFER_PREF_IMM16)
{
    int16_t immI16 = READ_INST_16_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedCreateobjectwithbufferPrefImm16);
    LOG_INST() << "    BaselineDeprecatedCreateobjectwithbufferPrefImm16 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(immI16);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, V0
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_TONUMBER_PREF_V8)
{
    int8_t v0 = READ_INST_8_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedTonumberPrefV8);
    LOG_INST() << "    BaselineDeprecatedTonumberPrefV8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(v0);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, V0
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_TONUMERIC_PREF_V8)
{
    int8_t v0 = READ_INST_8_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedTonumericPrefV8);
    LOG_INST() << "    BaselineDeprecatedTonumericPrefV8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(v0);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, V0
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_NEG_PREF_V8)
{
    int8_t v0 = READ_INST_8_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedNegPrefV8);
    LOG_INST() << "    BaselineDeprecatedNegPrefV8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(v0);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, V0
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_INC_PREF_V8)
{
    int8_t v0 = READ_INST_8_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedIncPrefV8);
    LOG_INST() << "    BaselineDeprecatedIncPrefV8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(v0);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, INDEX
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_NOT_PREF_V8)
{
    int8_t index = READ_INST_8_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedNotPrefV8);
    LOG_INST() << "    BaselineDeprecatedNotPrefV8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(index);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, INDEX
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_DEC_PREF_V8)
{
    int8_t index = READ_INST_8_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedDecPrefV8);
    LOG_INST() << "    BaselineDeprecatedDecPrefV8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(index);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, FUNC_REG, HOTNESS_COUNTER
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_CALLARG0_PREF_V8)
{
    int8_t funcReg = READ_INST_8_1();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedCallarg0PrefV8);
    LOG_INST() << "    BaselineDeprecatedCallarg0PrefV8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(funcReg);
    parameters.emplace_back(BaselineSpecialParameter::HOTNESS_COUNTER);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, FUNC_REG, A0, HOTNESS_COUNTER
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_CALLARG1_PREF_V8_V8)
{
    int8_t funcReg = READ_INST_8_1();
    int8_t v0 = READ_INST_8_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedCallarg1PrefV8V8);
    LOG_INST() << "    BaselineDeprecatedCallarg1PrefV8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(funcReg);
    parameters.emplace_back(v0);
    parameters.emplace_back(BaselineSpecialParameter::HOTNESS_COUNTER);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, FUNC_REG, A0, A1, HOTNESS_COUNTER
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_CALLARGS2_PREF_V8_V8_V8)
{
    int8_t funcReg = READ_INST_8_1();
    int8_t v0 = READ_INST_8_2();
    int8_t v1 = READ_INST_8_3();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedCallargs2PrefV8V8V8);
    LOG_INST() << "    BaselineDeprecatedCallargs2PrefV8V8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(funcReg);
    parameters.emplace_back(v0);
    parameters.emplace_back(v1);
    parameters.emplace_back(BaselineSpecialParameter::HOTNESS_COUNTER);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, FUNC_REG, A0, A1, A2, HOTNESS_COUNTER
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_CALLARGS3_PREF_V8_V8_V8_V8)
{
    int8_t funcReg = READ_INST_8_1();
    int8_t v0 = READ_INST_8_2();
    int8_t v1 = READ_INST_8_3();
    int8_t v2 = READ_INST_8_4();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedCallargs3PrefV8V8V8V8);
    LOG_INST() << "    BaselineDeprecatedCallargs3PrefV8V8V8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(funcReg);
    parameters.emplace_back(v0);
    parameters.emplace_back(v1);
    parameters.emplace_back(v2);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, V0, V1, V2
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_CALLSPREAD_PREF_V8_V8_V8)
{
    int8_t v0 = READ_INST_8_1();
    int8_t v1 = READ_INST_8_2();
    int8_t v2 = READ_INST_8_3();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedCallspreadPrefV8V8V8);
    LOG_INST() << "    BaselineDeprecatedCallspreadPrefV8V8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(v0);
    parameters.emplace_back(v1);
    parameters.emplace_back(v2);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

//  GLUE, SP, INDEX, FUNC_REG, HOTNESS_COUNTER
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_CALLRANGE_PREF_IMM16_V8)
{
    uint16_t actualNumArgs = READ_INST_16_1();
    int8_t funcReg = READ_INST_8_3();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedCallrangePrefImm16V8);
    LOG_INST() << "    BaselineDeprecatedCallrangePrefImm16V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      actualNumArgs: " << static_cast<int32_t>(actualNumArgs);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(actualNumArgs));
    parameters.emplace_back(funcReg);
    parameters.emplace_back(BaselineSpecialParameter::HOTNESS_COUNTER);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, INDEX, FUNC_REG, HOTNESS_COUNTER
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_CALLTHISRANGE_PREF_IMM16_V8)
{
    uint16_t index = READ_INST_16_1();
    int8_t funcReg = READ_INST_8_3();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedCallthisrangePrefImm16V8);
    LOG_INST() << "    BaselineDeprecatedCallthisrangePrefImm16V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      index: " << static_cast<int32_t>(index);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(index));
    parameters.emplace_back(funcReg);
    parameters.emplace_back(BaselineSpecialParameter::HOTNESS_COUNTER);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, ACC, METHOD_ID, LITERAL_ID, LENGTH, V0, V1, FUNC
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8)
{
    uint16_t methodId = READ_INST_16_1();
    uint16_t literalId = READ_INST_16_3();
    uint16_t length = READ_INST_16_5();
    uint8_t v0 = READ_INST_8_7();
    uint8_t v1 = READ_INST_8_8();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress = thread->GetBaselineStubEntry(
        BaselineStubCSigns::BaselineDeprecatedDefineclasswithbufferPrefId16Imm16Imm16V8V8);
    LOG_INST() << "    BaselineDeprecatedDefineclasswithbufferPrefId16Imm16Imm16V8V8 Address: "
               << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(methodId));
    parameters.emplace_back(static_cast<int32_t>(literalId));
    parameters.emplace_back(static_cast<int32_t>(length));
    uint32_t vregIds = static_cast<uint32_t>(v0) | (static_cast<uint32_t>(v1) << ONE_BYTE_SIZE);
    parameters.emplace_back(static_cast<int32_t>(vregIds));
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, ACC, V0, FUNC
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_RESUMEGENERATOR_PREF_V8)
{
    int8_t v0 = READ_INST_8_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedResumegeneratorPrefV8);
    LOG_INST() << "    BaselineDeprecatedResumegeneratorPrefV8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(v0);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// SP, ACC, V0
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_GETRESUMEMODE_PREF_V8)
{
    int8_t v0 = READ_INST_8_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedGetresumemodePrefV8);
    LOG_INST() << "    BaselineDeprecatedGetresumemodePrefV8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(v0);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, V0
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_GETTEMPLATEOBJECT_PREF_V8)
{
    int8_t v0 = READ_INST_8_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedGettemplateobjectPrefV8);
    LOG_INST() << "    BaselineDeprecatedGettemplateobjectPrefV8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(v0);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, V0, V1
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_DELOBJPROP_PREF_V8_V8)
{
    int8_t v0 = READ_INST_8_1();
    int8_t v1 = READ_INST_8_2();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedDelobjpropPrefV8V8);
    LOG_INST() << "    BaselineDeprecatedDelobjpropPrefV8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(v0);
    parameters.emplace_back(v1);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, V0, V1
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8)
{
    int8_t v0 = READ_INST_8_1();
    int8_t v1 = READ_INST_8_2();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedAsyncfunctionawaituncaughtPrefV8V8);
    LOG_INST() << "    BaselineDeprecatedAsyncfunctionawaituncaughtPrefV8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(v0);
    parameters.emplace_back(v1);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, V0, V1
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_COPYDATAPROPERTIES_PREF_V8_V8)
{
    int8_t v0 = READ_INST_8_1();
    int8_t v1 = READ_INST_8_2();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedCopydatapropertiesPrefV8V8);
    LOG_INST() << "    BaselineDeprecatedCopydatapropertiesPrefV8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(v0);
    parameters.emplace_back(v1);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, ACC, V0, V1
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_SETOBJECTWITHPROTO_PREF_V8_V8)
{
    int8_t v0 = READ_INST_8_1();
    int8_t v1 = READ_INST_8_2();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedSetobjectwithprotoPrefV8V8);
    LOG_INST() << "    BaselineDeprecatedSetobjectwithprotoPrefV8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(v0);
    parameters.emplace_back(v1);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_LDOBJBYVALUE_PREF_V8_V8)
{
    int8_t v0 = READ_INST_8_1();
    int8_t v1 = READ_INST_8_2();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedLdobjbyvaluePrefV8V8);
    LOG_INST() << "    BaselineDeprecatedLdobjbyvaluePrefV8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(v0);
    parameters.emplace_back(v1);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, V0, V1
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_LDSUPERBYVALUE_PREF_V8_V8)
{
    int8_t v0 = READ_INST_8_1();
    int8_t v1 = READ_INST_8_2();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedLdsuperbyvaluePrefV8V8);
    LOG_INST() << "    BaselineDeprecatedLdsuperbyvaluePrefV8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(v0);
    parameters.emplace_back(v1);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, V0, INDEX
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_LDOBJBYINDEX_PREF_V8_IMM32)
{
    int8_t v0 = READ_INST_8_1();
    int32_t index = static_cast<int32_t>(READ_INST_32_2());
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedLdobjbyindexPrefV8Imm32);
    LOG_INST() << "    BaselineDeprecatedLdobjbyindexPrefV8Imm32 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(v0);
    parameters.emplace_back(index);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, V0, V1
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8)
{
    int8_t v0 = READ_INST_8_1();
    int8_t v1 = READ_INST_8_3();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedAsyncfunctionresolvePrefV8V8V8);
    LOG_INST() << "    BaselineDeprecatedAsyncfunctionresolvePrefV8V8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(v0);
    parameters.emplace_back(v1);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, V0, V1
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_ASYNCFUNCTIONREJECT_PREF_V8_V8_V8)
{
    int8_t v0 = READ_INST_8_1();
    int8_t v1 = READ_INST_8_3();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedAsyncfunctionrejectPrefV8V8V8);
    LOG_INST() << "    BaselineDeprecatedAsyncfunctionrejectPrefV8V8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(v0);
    parameters.emplace_back(v1);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, LEVEL, SLOT, V0
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_STLEXVAR_PREF_IMM4_IMM4_V8)
{
    uint8_t level = READ_INST_4_2();
    uint8_t slot = READ_INST_4_3();
    int8_t v0 = READ_INST_8_2();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedStlexvarPrefImm4Imm4V8);
    LOG_INST() << "    BaselineDeprecatedStlexvarPrefImm4Imm4V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      level: " << static_cast<int32_t>(level);
    LOG_INST() << "      slot: " << static_cast<int32_t>(slot);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(level));
    parameters.emplace_back(static_cast<int32_t>(slot));
    parameters.emplace_back(v0);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

// GLUE, SP, LEVEL, SLOT, V0
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_STLEXVAR_PREF_IMM8_IMM8_V8)
{
    uint8_t level = READ_INST_8_1();
    uint8_t slot = READ_INST_8_2();
    int8_t v0 = READ_INST_8_3();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedStlexvarPrefImm8Imm8V8);
    LOG_INST() << "    BaselineDeprecatedStlexvarPrefImm8Imm8V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      level: " << static_cast<int32_t>(level);
    LOG_INST() << "      slot: " << static_cast<int32_t>(slot);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(level));
    parameters.emplace_back(static_cast<int32_t>(slot));
    parameters.emplace_back(v0);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

// GLUE, SP, LEVEL, SLOT, V0
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_STLEXVAR_PREF_IMM16_IMM16_V8)
{
    uint16_t level = READ_INST_16_1();
    uint16_t slot = READ_INST_16_3();
    int8_t v0 = READ_INST_8_5();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedStlexvarPrefImm16Imm16V8);
    LOG_INST() << "    BaselineDeprecatedStlexvarPrefImm16Imm16V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      level: " << static_cast<int32_t>(level);
    LOG_INST() << "      slot: " << static_cast<int32_t>(slot);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(level));
    parameters.emplace_back(static_cast<int32_t>(slot));
    parameters.emplace_back(v0);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

// GLUE, STRING_ID, SP
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_GETMODULENAMESPACE_PREF_ID32)
{
    int32_t stringId = static_cast<int32_t>(READ_INST_32_1());
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedGetmodulenamespacePrefId32);
    LOG_INST() << "    BaselineDeprecatedGetmodulenamespacePrefId32 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(stringId);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

// GLUE, ACC, STRING_ID, SP
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_STMODULEVAR_PREF_ID32)
{
    int32_t stringId = static_cast<int32_t>(READ_INST_32_1());
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedStmodulevarPrefId32);
    LOG_INST() << "    BaselineDeprecatedStmodulevarPrefId32 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(stringId);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_LDOBJBYNAME_PREF_ID32_V8)
{
    int32_t stringId = static_cast<int32_t>(READ_INST_32_1());
    int8_t v0 = READ_INST_8_5();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedLdobjbynamePrefId32V8);
    LOG_INST() << "    BaselineDeprecatedLdobjbynamePrefId32V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(v0);
    parameters.emplace_back(stringId);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_LDSUPERBYNAME_PREF_ID32_V8)
{
    int32_t stringId = static_cast<int32_t>(READ_INST_32_1());
    int8_t v0 = READ_INST_8_5();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedLdsuperbynamePrefId32V8);
    LOG_INST() << "    BaselineDeprecatedLdsuperbynamePrefId32V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(stringId);
    parameters.emplace_back(v0);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_LDMODULEVAR_PREF_ID32_IMM8)
{
    int32_t stringId = static_cast<int32_t>(READ_INST_32_1());
    int8_t flagI8 = READ_INST_8_5();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedLdmodulevarPrefId32Imm8);
    LOG_INST() << "    BaselineDeprecatedLdmodulevarPrefId32Imm8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(stringId);
    parameters.emplace_back(flagI8);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_STCONSTTOGLOBALRECORD_PREF_ID32)
{
    int32_t stringId = static_cast<int32_t>(READ_INST_32_1());
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedStconsttoglobalrecordPrefId32);
    LOG_INST() << "    BaselineDeprecatedStconsttoglobalrecordPrefId32 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(stringId);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, ACC, STRING_ID, SP
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_STLETTOGLOBALRECORD_PREF_ID32)
{
    int32_t stringId = static_cast<int32_t>(READ_INST_32_1());
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedStlettoglobalrecordPrefId32);
    LOG_INST() << "    BaselineDeprecatedStlettoglobalrecordPrefId32 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(stringId);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, ACC, STRING_ID, SP
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_STCLASSTOGLOBALRECORD_PREF_ID32)
{
    int32_t stringId = static_cast<int32_t>(READ_INST_32_1());
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedStclasstoglobalrecordPrefId32);
    LOG_INST() << "    BaselineDeprecatedStclasstoglobalrecordPrefId32 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(stringId);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_LDHOMEOBJECT_PREF_NONE)
{
    (void)bytecodeArray;
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedLdhomeobjectPrefNone);
    LOG_INST() << "    BaselineDeprecatedLdhomeobjectPrefNone Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, ACC, CONSTPOOL, IMM_I16, FUNC
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_CREATEOBJECTHAVINGMETHOD_PREF_IMM16)
{
    int16_t immI16 = READ_INST_16_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedCreateobjecthavingmethodPrefImm16);
    LOG_INST() << "    BaselineDeprecatedCreateobjecthavingmethodPrefImm16 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(immI16);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEPRECATED_DYNAMICIMPORT_PREF_V8)
{
    int8_t vregId = READ_INST_8_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDeprecatedDynamicimportPrefV8);
    LOG_INST() << "    BaselineDeprecatedDynamicimportPrefV8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(VirtualRegister(vregId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// ------- End deprecated  -------

// GLUE, SP, SLOT_ID, STRING_ID, V0
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEFINEFIELDBYNAME_IMM8_ID16_V8)
{
    uint8_t slotId = READ_INST_8_0();
    uint16_t stringId = READ_INST_16_1();
    uint8_t v0 = READ_INST_8_3();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDefineFieldByNameImm8Id16V8);
    LOG_INST() << "    BaselineDefineFieldByNameImm8Id16V8 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      slotId: " << static_cast<int32_t>(slotId);
    LOG_INST() << "      stringId: " << static_cast<int32_t>(stringId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    parameters.emplace_back(static_cast<int32_t>(stringId));
    parameters.emplace_back(static_cast<int32_t>(v0));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, ACC, PROFILE_TYPE_INFO, SLOT_ID_I8, STRING_ID, V0
BYTECODE_BASELINE_HANDLER_IMPLEMENT(DEFINEPROPERTYBYNAME_IMM8_ID16_V8)
{
    uint8_t slotId = READ_INST_8_0();
    uint16_t stringId = READ_INST_16_1();
    uint8_t v0 = READ_INST_8_3();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineDefinePropertyByNameImm8Id16V8);
    LOG_INST() << "    BaselineDefinePropertyByNameImm8Id16V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    parameters.emplace_back(static_cast<int32_t>(stringId));
    parameters.emplace_back(static_cast<int32_t>(v0));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_NOTIFYCONCURRENTRESULT_PREF_NONE)
{
    (void)bytecodeArray;
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
        thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCallRuntimeNotifyConcurrentResultPrefNone);
    LOG_INST() << "    BaselineCallRuntimeNotifyConcurrentResultPrefNone Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_DEFINEFIELDBYVALUE_PREF_IMM8_V8_V8)
{
    int8_t v0 = READ_INST_8_2();
    int8_t v1 = READ_INST_8_3();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCallRuntimeDefineFieldByValuePrefImm8V8V8);
    LOG_INST() << "    BaselineCallRuntimeDefineFieldByValuePrefImm8V8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(v0);
    parameters.emplace_back(v1);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, ACC, INDEX, V0
BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_DEFINEFIELDBYINDEX_PREF_IMM8_IMM32_V8)
{
    int32_t index = static_cast<int32_t>(READ_INST_32_2());
    int8_t v0 = READ_INST_8_6();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCallRuntimeDefineFieldByIndexPrefImm8Imm32V8);
    LOG_INST() << "    BaselineCallRuntimeDefineFieldByIndexPrefImm8Imm32V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(index);
    parameters.emplace_back(v0);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_TOPROPERTYKEY_PREF_NONE)
{
    (void)bytecodeArray;
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCallRuntimeToPropertyKeyPrefNone);
    LOG_INST() << "    BaselineCallRuntimeToPropertyKeyPrefNone Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

// GLUE, SP, COUNT, LITERAL_ID
BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_CREATEPRIVATEPROPERTY_PREF_IMM16_ID16)
{
    uint16_t count = READ_INST_16_1();
    uint16_t literalId = READ_INST_16_3();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCallRuntimeCreatePrivatePropertyPrefImm16Id16);
    LOG_INST() << "    BaselineCallRuntimeCreatePrivatePropertyPrefImm16Id16 Address: " << std::hex << builtinAddress;
    LOG_INST() << "      count: " << static_cast<int32_t>(count);
    LOG_INST() << "      literalId: " << static_cast<int32_t>(literalId);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(static_cast<int32_t>(count));
    parameters.emplace_back(static_cast<int32_t>(literalId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_DEFINEPRIVATEPROPERTY_PREF_IMM8_IMM16_IMM16_V8)
{
    uint16_t levelIndex = READ_INST_16_2();
    uint16_t slotIndex = READ_INST_16_4();
    int8_t v0 = READ_INST_8_6();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress = thread->GetBaselineStubEntry(
        BaselineStubCSigns::BaselineCallRuntimeDefinePrivatePropertyPrefImm8Imm16Imm16V8);
    LOG_INST() << "    BaselineCallRuntimeDefinePrivatePropertyPrefImm8Imm16Imm16V8 Address: "
               << std::hex << builtinAddress;
    LOG_INST() << "      levelIndex: " << static_cast<int32_t>(levelIndex);
    LOG_INST() << "      slotIndex: " << static_cast<int32_t>(slotIndex);

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(static_cast<int32_t>(levelIndex));
    parameters.emplace_back(static_cast<int32_t>(slotIndex));
    parameters.emplace_back(v0);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_CALLINIT_PREF_IMM8_V8)
{
    uint8_t slotId = READ_INST_8_1();
    int8_t v0 = READ_INST_8_2();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCallRuntimeCallInitPrefImm8V8);
    LOG_INST() << "    BaselineCallRuntimeCallInitPrefImm8V8 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(BaselineSpecialParameter::ACC);
    parameters.emplace_back(v0);
    parameters.emplace_back(BaselineSpecialParameter::HOTNESS_COUNTER);
    parameters.emplace_back(static_cast<int32_t>(slotId));
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_DEFINESENDABLECLASS_PREF_IMM16_ID16_ID16_IMM16_V8)
{
    int16_t methodId = READ_INST_16_3();
    int16_t literalId = READ_INST_16_5();
    int16_t length = READ_INST_16_7();
    int8_t v0 = READ_INST_8_9();

    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress = thread->GetBaselineStubEntry(
        BaselineStubCSigns::BaselineCallRuntimeDefineSendableClassPrefImm16Id16Id16Imm16V8);
    LOG_INST() << "    BaselineCallRuntimeDefineSendableClassPrefImm16Id16Id16Imm16V8 Address: "
               << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(methodId);
    parameters.emplace_back(literalId);
    parameters.emplace_back(length);
    parameters.emplace_back(v0);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_LDSENDABLECLASS_PREF_IMM16)
{
    int16_t level = READ_INST_16_1();
    auto *thread = vm->GetAssociatedJSThread();
    Address builtinAddress =
            thread->GetBaselineStubEntry(BaselineStubCSigns::BaselineCallRuntimeLdSendableClassPrefImm16);
    LOG_INST() << "    BaselineCallRuntimeLdSendableClassPrefImm16 Address: " << std::hex << builtinAddress;

    std::vector<BaselineParameter> parameters;
    parameters.emplace_back(BaselineSpecialParameter::GLUE);
    parameters.emplace_back(BaselineSpecialParameter::SP);
    parameters.emplace_back(level);
    GetBaselineAssembler().CallBuiltin(builtinAddress, parameters);
    GetBaselineAssembler().SaveResultIntoAcc();
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_LDSENDABLEEXTERNALMODULEVAR_PREF_IMM8)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_LDSENDABLEVAR_PREF_IMM4_IMM4)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_LDSENDABLELOCALMODULEVAR_PREF_IMM8)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_WIDELDSENDABLELOCALMODULEVAR_PREF_IMM16)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_NEWSENDABLEENV_PREF_IMM8)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_STSENDABLEVAR_PREF_IMM8_IMM8)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_WIDELDSENDABLEVAR_PREF_IMM16_IMM16)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_STSENDABLEVAR_PREF_IMM4_IMM4)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_WIDESTSENDABLEVAR_PREF_IMM16_IMM16)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_LDSENDABLEVAR_PREF_IMM8_IMM8)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_WIDENEWSENDABLEENV_PREF_IMM16)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_WIDELDSENDABLEEXTERNALMODULEVAR_PREF_IMM16)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_LDLAZYMODULEVAR_PREF_IMM8)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_WIDELDLAZYMODULEVAR_PREF_IMM16)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_LDLAZYSENDABLEMODULEVAR_PREF_IMM8)
{
    (void)bytecodeArray;
}

BYTECODE_BASELINE_HANDLER_IMPLEMENT(CALLRUNTIME_WIDELDLAZYSENDABLEMODULEVAR_PREF_IMM16)
{
    (void)bytecodeArray;
}
#undef LOG_INST
}  // namespace panda::ecmascript::kungfu
