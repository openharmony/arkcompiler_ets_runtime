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
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/interpreter/interpreter-inl.h"

namespace panda::ecmascript::kungfu {
BytecodeMetaData BytecodeMetaData::InitBytecodeMetaData(const uint8_t *pc)
{
    BytecodeInstruction inst(pc);
    uint32_t flags = 0;
    BytecodeKind kind = BytecodeKind::GENERAL;
    if (inst.HasFlag(BytecodeInstruction::Flags::ACC_READ)) {
        flags |= BytecodeFlags::READ_ACC;
    }
    if (inst.HasFlag(BytecodeInstruction::Flags::ACC_WRITE)) {
        flags |= BytecodeFlags::WRITE_ACC;
    }

    EcmaOpcode opcode = inst.GetOpcode();
    InitBytecodeFlags(opcode, flags);
    InitBytecodeKind(opcode, kind);

    if (kind == BytecodeKind::GENERAL ||
        kind == BytecodeKind::THROW_BC ||
        kind == BytecodeKind::RESUME ||
        kind == BytecodeKind::SUSPEND ||
        kind == BytecodeKind::GENERATOR_RESOLVE ||
        kind == BytecodeKind::CALL_BC ||
        kind == BytecodeKind::ACCESSOR_BC) {
        flags |= BytecodeFlags::GENERAL_BC;
    }
    auto size = inst.GetSize();
    auto vregCount = GetVRegCount(inst);
    uint64_t value = SizeField::Encode(size) | KindField::Encode(kind) |
                     FlagsField::Encode(static_cast<BytecodeFlags>(flags)) |
                     OpcodeField::Encode(inst.GetOpcode()) | VRegCountField::Encode(vregCount);
    return BytecodeMetaData(value);
}

void BytecodeMetaData::InitBytecodeFlags(EcmaOpcode &opcode, uint32_t &flags)
{
    InitNoSideEffectFlag(opcode, flags);
    InitNoGCFlag(opcode, flags);
    InitNoThrowFlag(opcode, flags);
    InitReadThisObjectFlag(opcode, flags);
    InitSupportDeoptFlag(opcode, flags);
    InitReadACCFlag(opcode, flags);
    InitDebuggerFlag(opcode, flags);
    InitReadFuncFlag(opcode, flags);
    InitWriteEnvFlag(opcode, flags);
    InitReadEnvFlag(opcode, flags);
    InitReadNewTargetFlag(opcode, flags);
    InitReadArgcFlag(opcode, flags);
}

void BytecodeMetaData::InitReadArgcFlag(EcmaOpcode &opcode, uint32_t &flags)
{
    switch (opcode) {
        case EcmaOpcode::GETUNMAPPEDARGS:
        case EcmaOpcode::COPYRESTARGS_IMM8:
        case EcmaOpcode::WIDE_COPYRESTARGS_PREF_IMM16:
            flags |= BytecodeFlags::READ_ARGC;
            break;
        default:
            break;
    }
}

void BytecodeMetaData::InitReadNewTargetFlag(EcmaOpcode &opcode, uint32_t &flags)
{
    switch (opcode) {
        case EcmaOpcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8:
        case EcmaOpcode::SUPERCALLARROWRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8:
        case EcmaOpcode::SUPERCALLSPREAD_IMM8_V8:
        case EcmaOpcode::CALLRUNTIME_SUPERCALLFORWARDALLARGS_PREF_V8:
            flags |= BytecodeFlags::READ_NEWTARGET;
            break;
        default:
            break;
    }
}

void BytecodeMetaData::InitReadEnvFlag(EcmaOpcode &opcode, uint32_t &flags)
{
    switch (opcode) {
        case EcmaOpcode::NEWLEXENV_IMM8:
        case EcmaOpcode::WIDE_NEWLEXENV_PREF_IMM16:
        case EcmaOpcode::NEWLEXENVWITHNAME_IMM8_ID16:
        case EcmaOpcode::WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16:
        case EcmaOpcode::POPLEXENV:
        case EcmaOpcode::LDLEXVAR_IMM4_IMM4:
        case EcmaOpcode::LDLEXVAR_IMM8_IMM8:
        case EcmaOpcode::WIDE_LDLEXVAR_PREF_IMM16_IMM16:
        case EcmaOpcode::CREATEOBJECTWITHBUFFER_IMM8_ID16:
        case EcmaOpcode::CREATEOBJECTWITHBUFFER_IMM16_ID16:
        case EcmaOpcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8:
        case EcmaOpcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8:
        case EcmaOpcode::DEFINEFUNC_IMM8_ID16_IMM8:
        case EcmaOpcode::DEFINEFUNC_IMM16_ID16_IMM8:
        case EcmaOpcode::DEFINEMETHOD_IMM8_ID16_IMM8:
        case EcmaOpcode::DEFINEMETHOD_IMM16_ID16_IMM8:
        case EcmaOpcode::LDPRIVATEPROPERTY_IMM8_IMM16_IMM16:
        case EcmaOpcode::STPRIVATEPROPERTY_IMM8_IMM16_IMM16_V8:
        case EcmaOpcode::TESTIN_IMM8_IMM16_IMM16:
        case EcmaOpcode::CALLRUNTIME_CREATEPRIVATEPROPERTY_PREF_IMM16_ID16:
        case EcmaOpcode::CALLRUNTIME_DEFINEPRIVATEPROPERTY_PREF_IMM8_IMM16_IMM16_V8:
        case EcmaOpcode::CALLRUNTIME_LDSENDABLECLASS_PREF_IMM16:
            flags |= BytecodeFlags::READ_ENV;
            break;
        default:
            break;
    }
}

void BytecodeMetaData::InitWriteEnvFlag(EcmaOpcode &opcode, uint32_t &flags)
{
    switch (opcode) {
        case EcmaOpcode::NEWLEXENV_IMM8:
        case EcmaOpcode::WIDE_NEWLEXENV_PREF_IMM16:
        case EcmaOpcode::NEWLEXENVWITHNAME_IMM8_ID16:
        case EcmaOpcode::WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16:
        case EcmaOpcode::POPLEXENV:
            flags |= BytecodeFlags::WRITE_ENV;
            break;
        default:
            break;
    }
}

void BytecodeMetaData::InitReadFuncFlag(EcmaOpcode &opcode, uint32_t &flags)
{
    switch (opcode) {
        case EcmaOpcode::CALLRUNTIME_NOTIFYCONCURRENTRESULT_PREF_NONE:
        case EcmaOpcode::SUSPENDGENERATOR_V8:
        case EcmaOpcode::RESUMEGENERATOR:
        case EcmaOpcode::TRYLDGLOBALBYNAME_IMM8_ID16:
        case EcmaOpcode::TRYLDGLOBALBYNAME_IMM16_ID16:
        case EcmaOpcode::STGLOBALVAR_IMM16_ID16:
        case EcmaOpcode::THROW_UNDEFINEDIFHOLEWITHNAME_PREF_ID16:
        case EcmaOpcode::DEFINEFUNC_IMM8_ID16_IMM8:
        case EcmaOpcode::DEFINEFUNC_IMM16_ID16_IMM8:
        case EcmaOpcode::DEFINEMETHOD_IMM8_ID16_IMM8:
        case EcmaOpcode::DEFINEMETHOD_IMM16_ID16_IMM8:
        case EcmaOpcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8:
        case EcmaOpcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8:
        case EcmaOpcode::INSTANCEOF_IMM8_V8:
        case EcmaOpcode::CREATEOBJECTWITHBUFFER_IMM8_ID16:
        case EcmaOpcode::CREATEOBJECTWITHBUFFER_IMM16_ID16:
        case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
        case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM16_ID16:
        case EcmaOpcode::STMODULEVAR_IMM8:
        case EcmaOpcode::WIDE_STMODULEVAR_PREF_IMM16:
        case EcmaOpcode::SETGENERATORSTATE_IMM8:
        case EcmaOpcode::DYNAMICIMPORT:
        case EcmaOpcode::LDEXTERNALMODULEVAR_IMM8:
        case EcmaOpcode::WIDE_LDEXTERNALMODULEVAR_PREF_IMM16:
        case EcmaOpcode::GETMODULENAMESPACE_IMM8:
        case EcmaOpcode::WIDE_GETMODULENAMESPACE_PREF_IMM16:
        case EcmaOpcode::NEWLEXENVWITHNAME_IMM8_ID16:
        case EcmaOpcode::WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16:
        case EcmaOpcode::LDSUPERBYVALUE_IMM8_V8:
        case EcmaOpcode::LDSUPERBYVALUE_IMM16_V8:
        case EcmaOpcode::STSUPERBYVALUE_IMM16_V8_V8:
        case EcmaOpcode::STSUPERBYVALUE_IMM8_V8_V8:
        case EcmaOpcode::TRYSTGLOBALBYNAME_IMM8_ID16:
        case EcmaOpcode::TRYSTGLOBALBYNAME_IMM16_ID16:
        case EcmaOpcode::LDGLOBALVAR_IMM16_ID16:
        case EcmaOpcode::LDOBJBYNAME_IMM8_ID16:
        case EcmaOpcode::LDOBJBYNAME_IMM16_ID16:
        case EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STOBJBYNAME_IMM16_ID16_V8:
        case EcmaOpcode::LDOBJBYVALUE_IMM8_V8:
        case EcmaOpcode::LDOBJBYVALUE_IMM16_V8:
        case EcmaOpcode::LDTHISBYVALUE_IMM8:
        case EcmaOpcode::LDTHISBYVALUE_IMM16:
        case EcmaOpcode::STOBJBYVALUE_IMM8_V8_V8:
        case EcmaOpcode::STOBJBYVALUE_IMM16_V8_V8:
        case EcmaOpcode::STTHISBYVALUE_IMM8_V8:
        case EcmaOpcode::STTHISBYVALUE_IMM16_V8:
        case EcmaOpcode::LDSUPERBYNAME_IMM8_ID16:
        case EcmaOpcode::LDSUPERBYNAME_IMM16_ID16:
        case EcmaOpcode::STSUPERBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STSUPERBYNAME_IMM16_ID16_V8:
        case EcmaOpcode::LDLOCALMODULEVAR_IMM8:
        case EcmaOpcode::WIDE_LDLOCALMODULEVAR_PREF_IMM16:
        case EcmaOpcode::LDTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::LDTHISBYNAME_IMM16_ID16:
        case EcmaOpcode::STTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::STTHISBYNAME_IMM16_ID16:
        case EcmaOpcode::ASYNCGENERATORRESOLVE_V8_V8_V8:
        case EcmaOpcode::CALLARG0_IMM8:
        case EcmaOpcode::CALLARG1_IMM8_V8:
        case EcmaOpcode::CALLARGS2_IMM8_V8_V8:
        case EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8:
        case EcmaOpcode::CALLRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_CALLRANGE_PREF_IMM16_V8:
        case EcmaOpcode::CALLTHIS0_IMM8_V8:
        case EcmaOpcode::CALLTHIS1_IMM8_V8_V8:
        case EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8:
        case EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
        case EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_CALLTHISRANGE_PREF_IMM16_V8:
        case EcmaOpcode::LDA_STR_ID16:
        case EcmaOpcode::STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8:
        case EcmaOpcode::STOWNBYNAMEWITHNAMESET_IMM16_ID16_V8:
        case EcmaOpcode::STTOGLOBALRECORD_IMM16_ID16:
        case EcmaOpcode::STCONSTTOGLOBALRECORD_IMM16_ID16:
        case EcmaOpcode::STOWNBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STOWNBYNAME_IMM16_ID16_V8:
        case EcmaOpcode::CREATEREGEXPWITHLITERAL_IMM8_ID16_IMM8:
        case EcmaOpcode::CREATEREGEXPWITHLITERAL_IMM16_ID16_IMM8:
        case EcmaOpcode::LDBIGINT_ID16:
        case EcmaOpcode::DEFINEFIELDBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::DEFINEPROPERTYBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::CALLRUNTIME_DEFINEFIELDBYVALUE_PREF_IMM8_V8_V8:
        case EcmaOpcode::CALLRUNTIME_DEFINEFIELDBYINDEX_PREF_IMM8_IMM32_V8:
        case EcmaOpcode::CALLRUNTIME_CREATEPRIVATEPROPERTY_PREF_IMM16_ID16:
        case EcmaOpcode::CALLRUNTIME_DEFINEPRIVATEPROPERTY_PREF_IMM8_IMM16_IMM16_V8:
        case EcmaOpcode::CALLRUNTIME_CALLINIT_PREF_IMM8_V8:
        case EcmaOpcode::CALLRUNTIME_DEFINESENDABLECLASS_PREF_IMM16_ID16_ID16_IMM16_V8:
        case EcmaOpcode::CALLRUNTIME_LDSENDABLEEXTERNALMODULEVAR_PREF_IMM8:
        case EcmaOpcode::CALLRUNTIME_WIDELDSENDABLEEXTERNALMODULEVAR_PREF_IMM16:
        case EcmaOpcode::CALLRUNTIME_LDSENDABLELOCALMODULEVAR_PREF_IMM8:
        case EcmaOpcode::CALLRUNTIME_WIDELDSENDABLELOCALMODULEVAR_PREF_IMM16:
        case EcmaOpcode::CALLRUNTIME_STSENDABLEVAR_PREF_IMM4_IMM4:
        case EcmaOpcode::CALLRUNTIME_STSENDABLEVAR_PREF_IMM8_IMM8:
        case EcmaOpcode::CALLRUNTIME_WIDESTSENDABLEVAR_PREF_IMM16_IMM16:
        case EcmaOpcode::CALLRUNTIME_LDSENDABLEVAR_PREF_IMM4_IMM4:
        case EcmaOpcode::CALLRUNTIME_LDSENDABLEVAR_PREF_IMM8_IMM8:
        case EcmaOpcode::CALLRUNTIME_WIDELDSENDABLEVAR_PREF_IMM16_IMM16:
        case EcmaOpcode::CALLRUNTIME_NEWSENDABLEENV_PREF_IMM8:
        case EcmaOpcode::CALLRUNTIME_WIDENEWSENDABLEENV_PREF_IMM16:
        case EcmaOpcode::CALLRUNTIME_LDLAZYMODULEVAR_PREF_IMM8:
        case EcmaOpcode::CALLRUNTIME_WIDELDLAZYMODULEVAR_PREF_IMM16:
        case EcmaOpcode::CALLRUNTIME_LDLAZYSENDABLEMODULEVAR_PREF_IMM8:
        case EcmaOpcode::CALLRUNTIME_WIDELDLAZYSENDABLEMODULEVAR_PREF_IMM16:
        case EcmaOpcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8:
            flags |= BytecodeFlags::READ_FUNC;
            break;
        default:
            break;
    }
}

void BytecodeMetaData::InitDebuggerFlag(EcmaOpcode &opcode, uint32_t &flags)
{
    switch (opcode) {
        case EcmaOpcode::DEBUGGER:
            flags |= BytecodeFlags::DEBUGGER_STMT;
            break;
        default:
            break;
    }
}

void BytecodeMetaData::InitReadACCFlag(EcmaOpcode &opcode, uint32_t &flags)
{
    switch (opcode) {
        case EcmaOpcode::RETURN:
        case EcmaOpcode::ASYNCGENERATORRESOLVE_V8_V8_V8:
            flags |= BytecodeFlags::READ_ACC;
            break;
        default:
            break;
    }
}

void BytecodeMetaData::InitSupportDeoptFlag(EcmaOpcode &opcode, uint32_t &flags)
{
    switch (opcode) {
        case EcmaOpcode::LDTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::LDTHISBYNAME_IMM16_ID16:
        case EcmaOpcode::STTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::STTHISBYNAME_IMM16_ID16:
        case EcmaOpcode::ADD2_IMM8_V8:
        case EcmaOpcode::SUB2_IMM8_V8:
        case EcmaOpcode::MUL2_IMM8_V8:
        case EcmaOpcode::DIV2_IMM8_V8:
        case EcmaOpcode::EQ_IMM8_V8:
        case EcmaOpcode::NOTEQ_IMM8_V8:
        case EcmaOpcode::LESS_IMM8_V8:
        case EcmaOpcode::LESSEQ_IMM8_V8:
        case EcmaOpcode::GREATER_IMM8_V8:
        case EcmaOpcode::GREATEREQ_IMM8_V8:
        case EcmaOpcode::STRICTEQ_IMM8_V8:
        case EcmaOpcode::STRICTNOTEQ_IMM8_V8:
        case EcmaOpcode::TONUMERIC_IMM8:
        case EcmaOpcode::ISTRUE:
        case EcmaOpcode::ISFALSE:
        case EcmaOpcode::CALLRUNTIME_ISTRUE_PREF_IMM8:
        case EcmaOpcode::CALLRUNTIME_ISFALSE_PREF_IMM8:
        case EcmaOpcode::INC_IMM8:
        case EcmaOpcode::DEC_IMM8:
        case EcmaOpcode::NEG_IMM8:
        case EcmaOpcode::NOT_IMM8:
        case EcmaOpcode::MOD2_IMM8_V8:
        case EcmaOpcode::SHL2_IMM8_V8:
        case EcmaOpcode::SHR2_IMM8_V8:
        case EcmaOpcode::ASHR2_IMM8_V8:
        case EcmaOpcode::AND2_IMM8_V8:
        case EcmaOpcode::OR2_IMM8_V8:
        case EcmaOpcode::XOR2_IMM8_V8:
        case EcmaOpcode::LDOBJBYNAME_IMM8_ID16:
        case EcmaOpcode::LDOBJBYNAME_IMM16_ID16:
        case EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STOBJBYNAME_IMM16_ID16_V8:
        case EcmaOpcode::LDOBJBYINDEX_IMM8_IMM16:
        case EcmaOpcode::LDOBJBYINDEX_IMM16_IMM16:
        case EcmaOpcode::WIDE_LDOBJBYINDEX_PREF_IMM32:
        case EcmaOpcode::STOBJBYINDEX_IMM8_V8_IMM16:
        case EcmaOpcode::STOBJBYINDEX_IMM16_V8_IMM16:
        case EcmaOpcode::WIDE_STOBJBYINDEX_PREF_V8_IMM32:
        case EcmaOpcode::LDOBJBYVALUE_IMM8_V8:
        case EcmaOpcode::LDOBJBYVALUE_IMM16_V8:
        case EcmaOpcode::STOBJBYVALUE_IMM8_V8_V8:
        case EcmaOpcode::STOBJBYVALUE_IMM16_V8_V8:
        case EcmaOpcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8:
        case EcmaOpcode::LDPRIVATEPROPERTY_IMM8_IMM16_IMM16:
        case EcmaOpcode::STPRIVATEPROPERTY_IMM8_IMM16_IMM16_V8:
        case EcmaOpcode::TESTIN_IMM8_IMM16_IMM16:
        case EcmaOpcode::DEFINEFIELDBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::DEFINEPROPERTYBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::CALLRUNTIME_DEFINEFIELDBYVALUE_PREF_IMM8_V8_V8:
        case EcmaOpcode::CALLRUNTIME_DEFINEFIELDBYINDEX_PREF_IMM8_IMM32_V8:
        case EcmaOpcode::CALLRUNTIME_TOPROPERTYKEY_PREF_NONE:
        case EcmaOpcode::CALLRUNTIME_CREATEPRIVATEPROPERTY_PREF_IMM16_ID16:
        case EcmaOpcode::CALLRUNTIME_DEFINEPRIVATEPROPERTY_PREF_IMM8_IMM16_IMM16_V8:
        case EcmaOpcode::CALLTHIS1_IMM8_V8_V8:
        case EcmaOpcode::CALLARG0_IMM8:
        case EcmaOpcode::CALLARG1_IMM8_V8:
        case EcmaOpcode::CALLARGS2_IMM8_V8_V8:
        case EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8:
        case EcmaOpcode::CALLRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::CALLTHIS0_IMM8_V8:
        case EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8:
        case EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
        case EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::CALLRUNTIME_CALLINIT_PREF_IMM8_V8:
        case EcmaOpcode::NEWOBJRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::NEWOBJRANGE_IMM16_IMM8_V8:
        case EcmaOpcode::WIDE_NEWOBJRANGE_PREF_IMM16_V8:
            flags |= BytecodeFlags::SUPPORT_DEOPT;
            break;
        default:
            break;
    }
}

void BytecodeMetaData::InitReadThisObjectFlag(EcmaOpcode &opcode, uint32_t &flags)
{
    switch (opcode) {
        case EcmaOpcode::LDTHISBYVALUE_IMM8:
        case EcmaOpcode::LDTHISBYVALUE_IMM16:
        case EcmaOpcode::STTHISBYVALUE_IMM8_V8:
        case EcmaOpcode::STTHISBYVALUE_IMM16_V8:
        case EcmaOpcode::LDTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::LDTHISBYNAME_IMM16_ID16:
        case EcmaOpcode::STTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::STTHISBYNAME_IMM16_ID16:
        case EcmaOpcode::SUSPENDGENERATOR_V8:
        case EcmaOpcode::DEPRECATED_SUSPENDGENERATOR_PREF_V8_V8:
        case EcmaOpcode::ASYNCGENERATORRESOLVE_V8_V8_V8:
            flags |= BytecodeFlags::READ_THIS_OBJECT;
            break;
        default:
            break;
    }
}

void BytecodeMetaData::InitNoSideEffectFlag(EcmaOpcode &opcode, uint32_t &flags)
{
    switch (opcode) {
        case EcmaOpcode::GETPROPITERATOR:
        case EcmaOpcode::TYPEOF_IMM8:
        case EcmaOpcode::TYPEOF_IMM16:
        case EcmaOpcode::LDSYMBOL:
        case EcmaOpcode::LDGLOBAL:
        case EcmaOpcode::LDBIGINT_ID16:
        case EcmaOpcode::LDEXTERNALMODULEVAR_IMM8:
        case EcmaOpcode::WIDE_LDEXTERNALMODULEVAR_PREF_IMM16:
        case EcmaOpcode::GETMODULENAMESPACE_IMM8:
        case EcmaOpcode::WIDE_GETMODULENAMESPACE_PREF_IMM16:
        case EcmaOpcode::ISTRUE:
        case EcmaOpcode::ISFALSE:
        case EcmaOpcode::CALLRUNTIME_ISTRUE_PREF_IMM8:
        case EcmaOpcode::CALLRUNTIME_ISFALSE_PREF_IMM8:
        case EcmaOpcode::LDGLOBALVAR_IMM16_ID16:
        case EcmaOpcode::LDOBJBYINDEX_IMM8_IMM16:
        case EcmaOpcode::LDOBJBYINDEX_IMM16_IMM16:
        case EcmaOpcode::WIDE_LDOBJBYINDEX_PREF_IMM32:
        case EcmaOpcode::LDLEXVAR_IMM4_IMM4:
        case EcmaOpcode::LDLEXVAR_IMM8_IMM8:
        case EcmaOpcode::WIDE_LDLEXVAR_PREF_IMM16_IMM16:
        case EcmaOpcode::CALLRUNTIME_LDSENDABLEVAR_PREF_IMM4_IMM4:
        case EcmaOpcode::CALLRUNTIME_LDSENDABLEVAR_PREF_IMM8_IMM8:
        case EcmaOpcode::CALLRUNTIME_WIDELDSENDABLEVAR_PREF_IMM16_IMM16:
        case EcmaOpcode::WIDE_LDPATCHVAR_PREF_IMM16:
        case EcmaOpcode::LDLOCALMODULEVAR_IMM8:
        case EcmaOpcode::WIDE_LDLOCALMODULEVAR_PREF_IMM16:
        case EcmaOpcode::LDA_STR_ID16:
        case EcmaOpcode::CALLRUNTIME_LDSENDABLECLASS_PREF_IMM16:
        case EcmaOpcode::MOV_V4_V4:
        case EcmaOpcode::MOV_V8_V8:
        case EcmaOpcode::MOV_V16_V16:
        case EcmaOpcode::STA_V8:
        case EcmaOpcode::LDA_V8:
        case EcmaOpcode::LDNAN:
        case EcmaOpcode::LDINFINITY:
        case EcmaOpcode::LDUNDEFINED:
        case EcmaOpcode::LDNULL:
        case EcmaOpcode::LDTRUE:
        case EcmaOpcode::LDFALSE:
        case EcmaOpcode::LDHOLE:
        case EcmaOpcode::LDAI_IMM32:
        case EcmaOpcode::FLDAI_IMM64:
        case EcmaOpcode::LDFUNCTION:
        case EcmaOpcode::RETURN:
        case EcmaOpcode::RETURNUNDEFINED:
        case EcmaOpcode::NOP:
        case EcmaOpcode::JEQZ_IMM8:
        case EcmaOpcode::JEQZ_IMM16:
        case EcmaOpcode::JEQZ_IMM32:
        case EcmaOpcode::JNEZ_IMM8:
        case EcmaOpcode::JNEZ_IMM16:
        case EcmaOpcode::JNEZ_IMM32:
        case EcmaOpcode::JMP_IMM8:
        case EcmaOpcode::JMP_IMM16:
        case EcmaOpcode::JMP_IMM32:
            flags |= BytecodeFlags::NO_SIDE_EFFECTS;
            break;
        default:
            break;
    }
}

void BytecodeMetaData::InitNoGCFlag(EcmaOpcode &opcode, uint32_t &flags)
{
    switch (opcode) {
        case EcmaOpcode::MOV_V4_V4:
        case EcmaOpcode::MOV_V8_V8:
        case EcmaOpcode::MOV_V16_V16:
        case EcmaOpcode::STA_V8:
        case EcmaOpcode::LDA_V8:
        case EcmaOpcode::LDHOLE:
        case EcmaOpcode::LDAI_IMM32:
        case EcmaOpcode::FLDAI_IMM64:
        case EcmaOpcode::LDFUNCTION:
        case EcmaOpcode::TYPEOF_IMM8:
        case EcmaOpcode::TYPEOF_IMM16:
        case EcmaOpcode::LDNAN:
        case EcmaOpcode::LDINFINITY:
        case EcmaOpcode::LDUNDEFINED:
        case EcmaOpcode::LDNULL:
        case EcmaOpcode::LDTRUE:
        case EcmaOpcode::LDFALSE:
        case EcmaOpcode::LDSYMBOL:
        case EcmaOpcode::LDGLOBAL:
        case EcmaOpcode::LDBIGINT_ID16:
        case EcmaOpcode::LDLEXVAR_IMM4_IMM4:
        case EcmaOpcode::LDLEXVAR_IMM8_IMM8:
        case EcmaOpcode::WIDE_LDLEXVAR_PREF_IMM16_IMM16:
        case EcmaOpcode::CALLRUNTIME_LDSENDABLEVAR_PREF_IMM4_IMM4:
        case EcmaOpcode::CALLRUNTIME_LDSENDABLEVAR_PREF_IMM8_IMM8:
        case EcmaOpcode::CALLRUNTIME_WIDELDSENDABLEVAR_PREF_IMM16_IMM16:
        case EcmaOpcode::WIDE_LDPATCHVAR_PREF_IMM16:
        case EcmaOpcode::LDA_STR_ID16:
        case EcmaOpcode::CALLRUNTIME_LDSENDABLECLASS_PREF_IMM16:
        case EcmaOpcode::RETURN:
        case EcmaOpcode::RETURNUNDEFINED:
            flags |= BytecodeFlags::NO_GC;
            break;
        default:
            break;
    }
}

void BytecodeMetaData::InitNoThrowFlag(EcmaOpcode &opcode, uint32_t &flags)
{
    switch (opcode) {
        case EcmaOpcode::MOV_V4_V4:
        case EcmaOpcode::MOV_V8_V8:
        case EcmaOpcode::MOV_V16_V16:
        case EcmaOpcode::STA_V8:
        case EcmaOpcode::LDA_V8:
        case EcmaOpcode::LDNAN:
        case EcmaOpcode::LDINFINITY:
        case EcmaOpcode::LDUNDEFINED:
        case EcmaOpcode::LDNULL:
        case EcmaOpcode::LDTRUE:
        case EcmaOpcode::LDFALSE:
        case EcmaOpcode::LDHOLE:
        case EcmaOpcode::LDAI_IMM32:
        case EcmaOpcode::FLDAI_IMM64:
        case EcmaOpcode::LDFUNCTION:
        case EcmaOpcode::LDA_STR_ID16:
        case EcmaOpcode::TYPEOF_IMM8:
        case EcmaOpcode::TYPEOF_IMM16:
        case EcmaOpcode::ISTRUE:
        case EcmaOpcode::ISFALSE:
        case EcmaOpcode::CALLRUNTIME_ISTRUE_PREF_IMM8:
        case EcmaOpcode::CALLRUNTIME_ISFALSE_PREF_IMM8:
        case EcmaOpcode::JEQZ_IMM8:
        case EcmaOpcode::JEQZ_IMM16:
        case EcmaOpcode::JEQZ_IMM32:
        case EcmaOpcode::JNEZ_IMM8:
        case EcmaOpcode::JNEZ_IMM16:
        case EcmaOpcode::JNEZ_IMM32:
        case EcmaOpcode::JMP_IMM8:
        case EcmaOpcode::JMP_IMM16:
        case EcmaOpcode::JMP_IMM32:
        case EcmaOpcode::STMODULEVAR_IMM8:
        case EcmaOpcode::WIDE_STMODULEVAR_PREF_IMM16:
        case EcmaOpcode::LDEXTERNALMODULEVAR_IMM8:
        case EcmaOpcode::WIDE_LDEXTERNALMODULEVAR_PREF_IMM16:
        case EcmaOpcode::NEWLEXENV_IMM8:
        case EcmaOpcode::WIDE_NEWLEXENV_PREF_IMM16:
        case EcmaOpcode::CALLRUNTIME_NEWSENDABLEENV_PREF_IMM8:
        case EcmaOpcode::CALLRUNTIME_WIDENEWSENDABLEENV_PREF_IMM16:
        case EcmaOpcode::POPLEXENV:
        case EcmaOpcode::NEWLEXENVWITHNAME_IMM8_ID16:
        case EcmaOpcode::WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16:
        case EcmaOpcode::ASYNCFUNCTIONENTER:
        case EcmaOpcode::SETGENERATORSTATE_IMM8:
        case EcmaOpcode::GETRESUMEMODE:
        case EcmaOpcode::RESUMEGENERATOR:
        case EcmaOpcode::RETURN:
        case EcmaOpcode::RETURNUNDEFINED:
        case EcmaOpcode::LDLEXVAR_IMM4_IMM4:
        case EcmaOpcode::LDLEXVAR_IMM8_IMM8:
        case EcmaOpcode::WIDE_LDLEXVAR_PREF_IMM16_IMM16:
        case EcmaOpcode::CALLRUNTIME_LDSENDABLEVAR_PREF_IMM4_IMM4:
        case EcmaOpcode::CALLRUNTIME_LDSENDABLEVAR_PREF_IMM8_IMM8:
        case EcmaOpcode::CALLRUNTIME_WIDELDSENDABLEVAR_PREF_IMM16_IMM16:
        case EcmaOpcode::STLEXVAR_IMM4_IMM4:
        case EcmaOpcode::STLEXVAR_IMM8_IMM8:
        case EcmaOpcode::WIDE_STLEXVAR_PREF_IMM16_IMM16:
        case EcmaOpcode::CALLRUNTIME_STSENDABLEVAR_PREF_IMM4_IMM4:
        case EcmaOpcode::CALLRUNTIME_STSENDABLEVAR_PREF_IMM8_IMM8:
        case EcmaOpcode::CALLRUNTIME_WIDESTSENDABLEVAR_PREF_IMM16_IMM16:
        case EcmaOpcode::STRICTNOTEQ_IMM8_V8:
        case EcmaOpcode::STRICTEQ_IMM8_V8:
        case EcmaOpcode::CREATEEMPTYARRAY_IMM8:
        case EcmaOpcode::CREATEEMPTYARRAY_IMM16:
        case EcmaOpcode::CREATEEMPTYOBJECT:
        case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
        case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM16_ID16:
        case EcmaOpcode::CREATEITERRESULTOBJ_V8_V8:
        case EcmaOpcode::DEFINEFUNC_IMM8_ID16_IMM8:
        case EcmaOpcode::DEFINEFUNC_IMM16_ID16_IMM8:
        case EcmaOpcode::DEFINEMETHOD_IMM8_ID16_IMM8:
        case EcmaOpcode::DEFINEMETHOD_IMM16_ID16_IMM8:
        case EcmaOpcode::CALLRUNTIME_LDSENDABLECLASS_PREF_IMM16:
        case EcmaOpcode::DEBUGGER:
        case EcmaOpcode::NOP:
            flags |= BytecodeFlags::NO_THROW;
            break;
        default:
            break;
    }
}

void BytecodeMetaData::InitBytecodeKind(EcmaOpcode &opcode, BytecodeKind &kind)
{
    if (InitMovKind(opcode, kind)) {
        return;
    }
    if (InitSetConstantKind(opcode, kind)) {
        return;
    }
    if (InitCallBCKind(opcode, kind)) {
        return;
    }
    if (InitRetrunKind(opcode, kind)) {
        return;
    }
    if (InitSuspendKind(opcode, kind)) {
        return;
    }
    if (InitResumeKind(opcode, kind)) {
        return;
    }
    if (InitDiscardedKind(opcode, kind)) {
        return;
    }
    if (InitThrowKind(opcode, kind)) {
        return;
    }
    if (InitConditionJumpKind(opcode, kind)) {
        return;
    }
    if (InitJumpIMMKind(opcode, kind)) {
        return;
    }
    if (InitGeneratorResolveKind(opcode, kind)) {
        return;
    }
    InitAccessorKind(opcode, kind);
}

bool BytecodeMetaData::InitAccessorKind(EcmaOpcode &opcode, BytecodeKind &kind)
{
    switch (opcode) {
        case EcmaOpcode::LDOBJBYNAME_IMM8_ID16:
        case EcmaOpcode::LDOBJBYNAME_IMM16_ID16:
        case EcmaOpcode::LDTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::LDTHISBYNAME_IMM16_ID16:
        case EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STOBJBYNAME_IMM16_ID16_V8:
        case EcmaOpcode::STTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::STTHISBYNAME_IMM16_ID16:
        case EcmaOpcode::DEFINEFIELDBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::DEFINEPROPERTYBYNAME_IMM8_ID16_V8:
            kind = BytecodeKind::ACCESSOR_BC;
            return true;
        default:
            return false;
    }
    return false;
}

bool BytecodeMetaData::InitGeneratorResolveKind(EcmaOpcode &opcode, BytecodeKind &kind)
{
    switch (opcode) {
        case EcmaOpcode::ASYNCGENERATORRESOLVE_V8_V8_V8:
            kind = BytecodeKind::GENERATOR_RESOLVE;
            return true;
        default:
            return false;
    }
    return false;
}

bool BytecodeMetaData::InitJumpIMMKind(EcmaOpcode &opcode, BytecodeKind &kind)
{
    switch (opcode) {
        case EcmaOpcode::JMP_IMM8:
        case EcmaOpcode::JMP_IMM16:
        case EcmaOpcode::JMP_IMM32:
            kind = BytecodeKind::JUMP_IMM;
            return true;
        default:
            return false;
    }
    return false;
}

bool BytecodeMetaData::InitConditionJumpKind(EcmaOpcode &opcode, BytecodeKind &kind)
{
    switch (opcode) {
        case EcmaOpcode::JEQZ_IMM8:
        case EcmaOpcode::JEQZ_IMM16:
        case EcmaOpcode::JEQZ_IMM32:
        case EcmaOpcode::JNEZ_IMM8:
        case EcmaOpcode::JNEZ_IMM16:
        case EcmaOpcode::JNEZ_IMM32:
            kind = BytecodeKind::CONDITIONAL_JUMP;
            return true;
        default:
            return false;
    }
    return false;
}

bool BytecodeMetaData::InitThrowKind(EcmaOpcode &opcode, BytecodeKind &kind)
{
    switch (opcode) {
        case EcmaOpcode::THROW_PREF_NONE:
        case EcmaOpcode::THROW_NOTEXISTS_PREF_NONE:
        case EcmaOpcode::THROW_PATTERNNONCOERCIBLE_PREF_NONE:
        case EcmaOpcode::THROW_DELETESUPERPROPERTY_PREF_NONE:
        case EcmaOpcode::THROW_CONSTASSIGNMENT_PREF_V8:
            kind = BytecodeKind::THROW_BC;
            return true;
        default:
            return false;
    }
    return false;
}

bool BytecodeMetaData::InitDiscardedKind(EcmaOpcode &opcode, BytecodeKind &kind)
{
    switch (opcode) {
        case EcmaOpcode::NOP:
            kind = BytecodeKind::DISCARDED;
            return true;
        default:
            return false;
    }
    return false;
}

bool BytecodeMetaData::InitMovKind(EcmaOpcode &opcode, BytecodeKind &kind)
{
    switch (opcode) {
        case EcmaOpcode::MOV_V4_V4:
        case EcmaOpcode::MOV_V8_V8:
        case EcmaOpcode::MOV_V16_V16:
        case EcmaOpcode::STA_V8:
        case EcmaOpcode::LDA_V8:
            kind = BytecodeKind::MOV;
            return true;
        default:
            return false;
    }
    return false;
}

bool BytecodeMetaData::InitSetConstantKind(EcmaOpcode &opcode, BytecodeKind &kind)
{
    switch (opcode) {
        case EcmaOpcode::LDNAN:
        case EcmaOpcode::LDINFINITY:
        case EcmaOpcode::LDUNDEFINED:
        case EcmaOpcode::LDNULL:
        case EcmaOpcode::LDTRUE:
        case EcmaOpcode::LDFALSE:
        case EcmaOpcode::LDHOLE:
        case EcmaOpcode::LDAI_IMM32:
        case EcmaOpcode::FLDAI_IMM64:
        case EcmaOpcode::LDFUNCTION:
            kind = BytecodeKind::SET_CONSTANT;
            return true;
        default:
            return false;
    }
    return false;
}

bool BytecodeMetaData::InitCallBCKind(EcmaOpcode &opcode, BytecodeKind &kind)
{
    switch (opcode) {
        case EcmaOpcode::CALLTHIS1_IMM8_V8_V8:
        case EcmaOpcode::CALLARG0_IMM8:
        case EcmaOpcode::CALLARG1_IMM8_V8:
        case EcmaOpcode::CALLARGS2_IMM8_V8_V8:
        case EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8:
        case EcmaOpcode::CALLRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::CALLTHIS0_IMM8_V8:
        case EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8:
        case EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
        case EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::CALLRUNTIME_CALLINIT_PREF_IMM8_V8:
        case EcmaOpcode::NEWOBJRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::NEWOBJRANGE_IMM16_IMM8_V8:
        case EcmaOpcode::WIDE_NEWOBJRANGE_PREF_IMM16_V8:
        case EcmaOpcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8:
            kind = BytecodeKind::CALL_BC;
            return true;
        default:
            return false;
    }
    return false;
}

bool BytecodeMetaData::InitRetrunKind(EcmaOpcode &opcode, BytecodeKind &kind)
{
    switch (opcode) {
        case EcmaOpcode::RETURN:
        case EcmaOpcode::RETURNUNDEFINED:
            kind = BytecodeKind::RETURN_BC;
            return true;
        default:
            return false;
    }
    return false;
}

bool BytecodeMetaData::InitSuspendKind(EcmaOpcode &opcode, BytecodeKind &kind)
{
    switch (opcode) {
        case EcmaOpcode::SUSPENDGENERATOR_V8:
        case EcmaOpcode::DEPRECATED_SUSPENDGENERATOR_PREF_V8_V8:
            kind = BytecodeKind::SUSPEND;
            return true;
        default:
            return false;
    }
    return false;
}

bool BytecodeMetaData::InitResumeKind(EcmaOpcode &opcode, BytecodeKind &kind)
{
    switch (opcode) {
        case EcmaOpcode::RESUMEGENERATOR:
            kind = BytecodeKind::RESUME;
            return true;
        default:
            return false;
    }
    return false;
}

// static
size_t BytecodeMetaData::GetVRegCount(const BytecodeInstruction &inst)
{
    size_t idx = 0;
    BytecodeInstruction::Format format = inst.GetFormat();
    while (inst.HasVReg(format, idx)) {
        idx++;
    }
    return idx;
}

Bytecodes::Bytecodes()
{
    for (uint8_t pc = 0; pc <= static_cast<uint8_t>(Bytecodes::LAST_OPCODE); pc++) {
        auto info = BytecodeMetaData::InitBytecodeMetaData(&pc);
        bytecodes_[pc] = info;
    }
    auto last = (static_cast<uint16_t>(Bytecodes::LAST_CALLRUNTIME_OPCODE) & OPCODE_MASK) >> BYTE_SIZE;
    for (uint8_t pc = 0; pc <= last; pc++) {
        std::array<uint8_t, 2> bytecode{CALLRUNTIME_PREFIX_OPCODE_INDEX, pc}; // 2: 2 opcode
        auto info = BytecodeMetaData::InitBytecodeMetaData(&bytecode[0]);
        callRuntimeBytecodes_[pc] = info;
    }
    last = (static_cast<uint16_t>(Bytecodes::LAST_DEPRECATED_OPCODE) & OPCODE_MASK) >> BYTE_SIZE;
    for (uint8_t pc = 0; pc <= last; pc++) {
        std::array<uint8_t, 2> bytecode{DEPRECATED_PREFIX_OPCODE_INDEX, pc}; // 2: 2 opcode
        auto info = BytecodeMetaData::InitBytecodeMetaData(&bytecode[0]);
        deprecatedBytecodes_[pc] = info;
    }
    last = (static_cast<uint16_t>(Bytecodes::LAST_WIDE_OPCODE) & OPCODE_MASK) >> BYTE_SIZE;
    for (uint8_t pc = 0; pc <= last; pc++) {
        std::array<uint8_t, 2> bytecode{WIDE_PREFIX_OPCODE_INDEX, pc}; // 2: 2 opcode
        auto info = BytecodeMetaData::InitBytecodeMetaData(&bytecode[0]);
        wideBytecodes_[pc] = info;
    }
    last = (static_cast<uint16_t>(Bytecodes::LAST_THROW_OPCODE) & OPCODE_MASK) >> BYTE_SIZE;
    for (uint8_t pc = 0; pc <= last; pc++) {
        std::array<uint8_t, 2> bytecode{THROW_PREFIX_OPCODE_INDEX, pc}; // 2: 2 opcode
        auto info = BytecodeMetaData::InitBytecodeMetaData(&bytecode[0]);
        throwBytecodes_[pc] = info;
    }
}

void BytecodeInfo::InitBytecodeInfo(BytecodeCircuitBuilder *builder,
                                    BytecodeInfo &info, const uint8_t *pc)
{
    auto opcode = info.GetOpcode();
    info.SetInsufficientProfile(builder, pc);
    switch (opcode) {
        case EcmaOpcode::MOV_V4_V4: {
            uint16_t vdst = READ_INST_4_0();
            uint16_t vsrc = READ_INST_4_1();
            info.vregOut.emplace_back(vdst);
            info.inputs.emplace_back(VirtualRegister(vsrc));
            break;
        }
        case EcmaOpcode::MOV_V8_V8: {
            uint16_t vdst = READ_INST_8_0();
            uint16_t vsrc = READ_INST_8_1();
            info.vregOut.emplace_back(vdst);
            info.inputs.emplace_back(VirtualRegister(vsrc));
            break;
        }
        case EcmaOpcode::MOV_V16_V16: {
            uint16_t vdst = READ_INST_16_0();
            uint16_t vsrc = READ_INST_16_2();
            info.vregOut.emplace_back(vdst);
            info.inputs.emplace_back(VirtualRegister(vsrc));
            break;
        }
        case EcmaOpcode::LDA_STR_ID16: {
            uint16_t stringId = READ_INST_16_0();
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            break;
        }
        case EcmaOpcode::LDA_V8: {
            uint16_t vsrc = READ_INST_8_0();
            info.inputs.emplace_back(VirtualRegister(vsrc));
            break;
        }
        case EcmaOpcode::STA_V8: {
            uint16_t vdst = READ_INST_8_0();
            info.vregOut.emplace_back(vdst);
            break;
        }
        case EcmaOpcode::LDAI_IMM32: {
            info.inputs.emplace_back(Immediate(READ_INST_32_0()));
            break;
        }
        case EcmaOpcode::FLDAI_IMM64: {
            info.inputs.emplace_back(Immediate(READ_INST_64_0()));
            break;
        }
        case EcmaOpcode::CALLARG1_IMM8_V8: {
            uint32_t a0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(a0));
            break;
        }
        case EcmaOpcode::CALLTHIS1_IMM8_V8_V8: {
            uint32_t startReg = READ_INST_8_1(); // this
            uint32_t a0 = READ_INST_8_2();
            info.inputs.emplace_back(VirtualRegister(startReg));
            info.inputs.emplace_back(VirtualRegister(a0));
            break;
        }
        case EcmaOpcode::CALLARGS2_IMM8_V8_V8: {
            uint32_t a0 = READ_INST_8_1();
            uint32_t a1 = READ_INST_8_2();
            info.inputs.emplace_back(VirtualRegister(a0));
            info.inputs.emplace_back(VirtualRegister(a1));
            break;
        }
        case EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8: {
            uint32_t a0 = READ_INST_8_1();
            uint32_t a1 = READ_INST_8_2();
            uint32_t a2 = READ_INST_8_3();
            info.inputs.emplace_back(VirtualRegister(a0));
            info.inputs.emplace_back(VirtualRegister(a1));
            info.inputs.emplace_back(VirtualRegister(a2));
            break;
        }
        case EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8: {
            uint32_t actualNumArgs = READ_INST_8_1();
            uint32_t startReg = READ_INST_8_2();
            info.inputs.emplace_back(VirtualRegister(startReg));
            for (size_t i = 1; i <= actualNumArgs; i++) {
                info.inputs.emplace_back(VirtualRegister(startReg + i));
            }
            break;
        }
        case EcmaOpcode::WIDE_CALLTHISRANGE_PREF_IMM16_V8: {
            uint32_t actualNumArgs = READ_INST_16_1();
            uint32_t startReg = READ_INST_8_3();
            info.inputs.emplace_back(VirtualRegister(startReg));
            for (size_t i = 1; i <= actualNumArgs; i++) {
                info.inputs.emplace_back(VirtualRegister(startReg + i));
            }
            break;
        }
        case EcmaOpcode::CALLTHIS0_IMM8_V8: {
            int32_t startReg = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(startReg));
            break;
        }
        case EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8: {
            int32_t startReg = READ_INST_8_1();
            uint32_t a0 = READ_INST_8_2();
            uint32_t a1 = READ_INST_8_3();
            info.inputs.emplace_back(VirtualRegister(startReg));
            info.inputs.emplace_back(VirtualRegister(a0));
            info.inputs.emplace_back(VirtualRegister(a1));
            break;
        }
        case EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8: {
            int32_t startReg = READ_INST_8_1();
            uint32_t a0 = READ_INST_8_2();
            uint32_t a1 = READ_INST_8_3();
            uint32_t a2 = READ_INST_8_4();
            info.inputs.emplace_back(VirtualRegister(startReg));
            info.inputs.emplace_back(VirtualRegister(a0));
            info.inputs.emplace_back(VirtualRegister(a1));
            info.inputs.emplace_back(VirtualRegister(a2));
            break;
        }
        case EcmaOpcode::CALLRUNTIME_CALLINIT_PREF_IMM8_V8: {
            int32_t startReg = READ_INST_8_2();
            info.inputs.emplace_back(VirtualRegister(startReg));
            break;
        }

        case EcmaOpcode::APPLY_IMM8_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::CALLRANGE_IMM8_IMM8_V8: {
            int32_t actualNumArgs = READ_INST_8_1();
            int32_t startReg = READ_INST_8_2();
            for (int i = 0; i < actualNumArgs; i++) {
                info.inputs.emplace_back(VirtualRegister(startReg + i));
            }
            break;
        }
        case EcmaOpcode::WIDE_CALLRANGE_PREF_IMM16_V8: {
            int32_t actualNumArgs = READ_INST_16_1();
            int32_t startReg = READ_INST_8_3();
            for (int i = 0; i < actualNumArgs; i++) {
                info.inputs.emplace_back(VirtualRegister(startReg + i));
            }
            break;
        }
        case EcmaOpcode::THROW_CONSTASSIGNMENT_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::THROW_IFNOTOBJECT_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::THROW_UNDEFINEDIFHOLE_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::THROW_UNDEFINEDIFHOLEWITHNAME_PREF_ID16: {
            uint16_t stringId = READ_INST_16_1();
            info.inputs.emplace_back(ICSlotId(stringId));
            break;
        }
        case EcmaOpcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM8: {
            uint8_t imm = READ_INST_8_1();
            info.inputs.emplace_back(Immediate(imm));
            break;
        }
        case EcmaOpcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM16: {
            uint16_t imm = READ_INST_16_1();
            info.inputs.emplace_back(Immediate(imm));
            break;
        }
        case EcmaOpcode::CLOSEITERATOR_IMM8_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::ADD2_IMM8_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::SUB2_IMM8_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::MUL2_IMM8_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::DIV2_IMM8_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::MOD2_IMM8_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::EQ_IMM8_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::NOTEQ_IMM8_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::LESS_IMM8_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::LESSEQ_IMM8_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::GREATER_IMM8_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::GREATEREQ_IMM8_V8: {
            uint16_t vs = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(vs));
            break;
        }
        case EcmaOpcode::SHL2_IMM8_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::SHR2_IMM8_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::ASHR2_IMM8_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::AND2_IMM8_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::OR2_IMM8_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::XOR2_IMM8_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::EXP_IMM8_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::ISIN_IMM8_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::INSTANCEOF_IMM8_V8: {
            uint16_t slotId = READ_INST_8_0();
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(ICSlotId(slotId));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::STRICTNOTEQ_IMM8_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::STRICTEQ_IMM8_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::DELOBJPROP_V8: {
            uint16_t v0 = READ_INST_8_0();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::DEFINEFUNC_IMM8_ID16_IMM8: {
            uint16_t slotId = READ_INST_8_0();
            uint16_t methodId = READ_INST_16_1();
            uint16_t length = READ_INST_8_3();
            info.inputs.emplace_back(ICSlotId(slotId));
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::MethodIDType, methodId));
            info.inputs.emplace_back(Immediate(length));
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            break;
        }
        case EcmaOpcode::DEFINEFUNC_IMM16_ID16_IMM8: {
            uint16_t slotId = READ_INST_16_0();
            uint16_t methodId = READ_INST_16_2();
            uint16_t length = READ_INST_8_4();
            info.inputs.emplace_back(ICSlotId(slotId));
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::MethodIDType, methodId));
            info.inputs.emplace_back(Immediate(length));
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            break;
        }
        case EcmaOpcode::DEFINEMETHOD_IMM8_ID16_IMM8: {
            uint16_t slotId = READ_INST_8_0();
            uint16_t methodId = READ_INST_16_1();
            uint16_t length = READ_INST_8_3();
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::MethodIDType, methodId));
            info.inputs.emplace_back(Immediate(length));
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            info.inputs.emplace_back(ICSlotId(slotId));
            break;
        }
        case EcmaOpcode::DEFINEMETHOD_IMM16_ID16_IMM8: {
            uint16_t slotId = READ_INST_16_0();
            uint16_t methodId = READ_INST_16_2();
            uint16_t length = READ_INST_8_4();
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::MethodIDType, methodId));
            info.inputs.emplace_back(Immediate(length));
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            info.inputs.emplace_back(ICSlotId(slotId));
            break;
        }
        case EcmaOpcode::NEWOBJRANGE_IMM8_IMM8_V8: {
            uint16_t numArgs = READ_INST_8_1();
            uint16_t firstArgRegIdx = READ_INST_8_2();
            for (uint16_t i = 0; i < numArgs; ++i) {
                info.inputs.emplace_back(VirtualRegister(firstArgRegIdx + i));
            }
            break;
        }
        case EcmaOpcode::NEWOBJRANGE_IMM16_IMM8_V8: {
            uint16_t numArgs = READ_INST_8_2();
            uint16_t firstArgRegIdx = READ_INST_8_3();
            for (uint16_t i = 0; i < numArgs; ++i) {
                info.inputs.emplace_back(VirtualRegister(firstArgRegIdx + i));
            }
            break;
        }
        case EcmaOpcode::WIDE_NEWOBJRANGE_PREF_IMM16_V8: {
            uint16_t numArgs = READ_INST_16_1();
            uint16_t firstArgRegIdx = READ_INST_8_3();
            for (uint16_t i = 0; i < numArgs; ++i) {
                info.inputs.emplace_back(VirtualRegister(firstArgRegIdx + i));
            }
            break;
        }
        case EcmaOpcode::LDLEXVAR_IMM4_IMM4: {
            uint16_t level = READ_INST_4_0();
            uint16_t slot = READ_INST_4_1();
            info.inputs.emplace_back(Immediate(level));
            info.inputs.emplace_back(Immediate(slot));
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            break;
        }
        case EcmaOpcode::LDLEXVAR_IMM8_IMM8: {
            uint16_t level = READ_INST_8_0();
            uint16_t slot = READ_INST_8_1();
            info.inputs.emplace_back(Immediate(level));
            info.inputs.emplace_back(Immediate(slot));
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            break;
        }
        case EcmaOpcode::WIDE_LDLEXVAR_PREF_IMM16_IMM16: {
            uint16_t level = READ_INST_16_1();
            uint16_t slot = READ_INST_16_3();
            info.inputs.emplace_back(Immediate(level));
            info.inputs.emplace_back(Immediate(slot));
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            break;
        }
        case EcmaOpcode::CALLRUNTIME_LDSENDABLEVAR_PREF_IMM4_IMM4: {
            uint16_t level = READ_INST_4_2();
            uint16_t slot = READ_INST_4_3();
            info.inputs.emplace_back(Immediate(level));
            info.inputs.emplace_back(Immediate(slot));
            break;
        }
        case EcmaOpcode::CALLRUNTIME_LDSENDABLEVAR_PREF_IMM8_IMM8: {
            uint16_t level = READ_INST_8_1();
            uint16_t slot = READ_INST_8_2();
            info.inputs.emplace_back(Immediate(level));
            info.inputs.emplace_back(Immediate(slot));
            break;
        }
        case EcmaOpcode::CALLRUNTIME_WIDELDSENDABLEVAR_PREF_IMM16_IMM16: {
            uint16_t level = READ_INST_16_1();
            uint16_t slot = READ_INST_16_3();
            info.inputs.emplace_back(Immediate(level));
            info.inputs.emplace_back(Immediate(slot));
            break;
        }
        case EcmaOpcode::STLEXVAR_IMM4_IMM4: {
            uint16_t level = READ_INST_4_0();
            uint16_t slot = READ_INST_4_1();
            info.inputs.emplace_back(Immediate(level));
            info.inputs.emplace_back(Immediate(slot));
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            break;
        }
        case EcmaOpcode::STLEXVAR_IMM8_IMM8: {
            uint16_t level = READ_INST_8_0();
            uint16_t slot = READ_INST_8_1();
            info.inputs.emplace_back(Immediate(level));
            info.inputs.emplace_back(Immediate(slot));
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            break;
        }
        case EcmaOpcode::WIDE_STLEXVAR_PREF_IMM16_IMM16: {
            uint16_t level = READ_INST_16_1();
            uint16_t slot = READ_INST_16_3();
            info.inputs.emplace_back(Immediate(level));
            info.inputs.emplace_back(Immediate(slot));
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            break;
        }
        case EcmaOpcode::CALLRUNTIME_STSENDABLEVAR_PREF_IMM4_IMM4: {
            uint16_t level = READ_INST_4_2();
            uint16_t slot = READ_INST_4_3();
            info.inputs.emplace_back(Immediate(level));
            info.inputs.emplace_back(Immediate(slot));
            break;
        }
        case EcmaOpcode::CALLRUNTIME_STSENDABLEVAR_PREF_IMM8_IMM8: {
            uint16_t level = READ_INST_8_1();
            uint16_t slot = READ_INST_8_2();
            info.inputs.emplace_back(Immediate(level));
            info.inputs.emplace_back(Immediate(slot));
            break;
        }
        case EcmaOpcode::CALLRUNTIME_WIDESTSENDABLEVAR_PREF_IMM16_IMM16: {
            uint16_t level = READ_INST_16_1();
            uint16_t slot = READ_INST_16_3();
            info.inputs.emplace_back(Immediate(level));
            info.inputs.emplace_back(Immediate(slot));
            break;
        }
        case EcmaOpcode::NEWLEXENV_IMM8: {
            uint8_t numVars = READ_INST_8_0();
            info.inputs.emplace_back(Immediate(numVars));
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            info.vregOut.emplace_back(builder->GetEnvVregIdx());
            break;
        }
        case EcmaOpcode::WIDE_NEWLEXENV_PREF_IMM16: {
            uint16_t numVars = READ_INST_16_1();
            info.inputs.emplace_back(Immediate(numVars));
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            info.vregOut.emplace_back(builder->GetEnvVregIdx());
            break;
        }
        case EcmaOpcode::CALLRUNTIME_NEWSENDABLEENV_PREF_IMM8: {
            uint8_t numVars = READ_INST_8_1();
            info.inputs.emplace_back(Immediate(numVars));
            break;
        }
        case EcmaOpcode::CALLRUNTIME_WIDENEWSENDABLEENV_PREF_IMM16: {
            uint16_t numVars = READ_INST_16_1();
            info.inputs.emplace_back(Immediate(numVars));
            break;
        }
        case EcmaOpcode::NEWLEXENVWITHNAME_IMM8_ID16: {
            uint16_t numVars = READ_INST_8_0();
            uint16_t scopeId = READ_INST_16_1();
            info.inputs.emplace_back(Immediate(numVars));
            info.inputs.emplace_back(Immediate(scopeId));
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            info.vregOut.emplace_back(builder->GetEnvVregIdx());
            break;
        }
        case EcmaOpcode::WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16: {
            uint16_t numVars = READ_INST_16_1();
            uint16_t scopeId = READ_INST_16_3();
            info.inputs.emplace_back(Immediate(numVars));
            info.inputs.emplace_back(Immediate(scopeId));
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            info.vregOut.emplace_back(builder->GetEnvVregIdx());
            break;
        }
        case EcmaOpcode::CREATEITERRESULTOBJ_V8_V8: {
            uint16_t v0 = READ_INST_8_0();
            uint16_t v1 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::SUSPENDGENERATOR_V8: {
            uint16_t v0 = READ_INST_8_0();
            uint32_t offset = builder->GetPcOffset(pc);
            offset += BytecodeInstruction::Size(EcmaOpcode::SUSPENDGENERATOR_V8); // skip suspend bc
            info.inputs.emplace_back(Immediate(offset)); // Save the pc offset when suspend
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::ASYNCFUNCTIONAWAITUNCAUGHT_V8: {
            uint16_t v0 = READ_INST_8_0();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::ASYNCFUNCTIONRESOLVE_V8: {
            uint16_t v0 = READ_INST_8_0();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::ASYNCFUNCTIONREJECT_V8: {
            uint16_t v0 = READ_INST_8_0();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::CLOSEITERATOR_IMM16_V8: {
            uint16_t v0 = READ_INST_8_2();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::NEWOBJAPPLY_IMM8_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::NEWOBJAPPLY_IMM16_V8: {
            uint16_t v0 = READ_INST_8_2();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::STOWNBYNAME_IMM8_ID16_V8: {
            uint16_t stringId = READ_INST_16_1();
            uint32_t v0 = READ_INST_8_3();
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::STOWNBYNAME_IMM16_ID16_V8: {
            uint16_t stringId = READ_INST_16_2();
            uint32_t v0 = READ_INST_8_4();
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::CREATEREGEXPWITHLITERAL_IMM8_ID16_IMM8: {
            uint16_t stringId = READ_INST_16_1();
            uint8_t flags = READ_INST_8_3();
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            info.inputs.emplace_back(Immediate(flags));
            break;
        }
        case EcmaOpcode::CREATEREGEXPWITHLITERAL_IMM16_ID16_IMM8: {
            uint16_t stringId = READ_INST_16_2();
            uint8_t flags = READ_INST_8_4();
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            info.inputs.emplace_back(Immediate(flags));
            break;
        }
        case EcmaOpcode::GETNEXTPROPNAME_V8: {
            uint16_t v0 = READ_INST_8_0();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::CREATEOBJECTWITHBUFFER_IMM8_ID16: {
            uint16_t imm = READ_INST_16_1();
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::ObjectLiteralIDType, imm));
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            break;
        }
        case EcmaOpcode::CREATEOBJECTWITHBUFFER_IMM16_ID16: {
            uint16_t imm = READ_INST_16_2();
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::ObjectLiteralIDType, imm));
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            break;
        }
        case EcmaOpcode::SETOBJECTWITHPROTO_IMM8_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::SETOBJECTWITHPROTO_IMM16_V8: {
            uint16_t v0 = READ_INST_8_2();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM8_ID16: {
            uint16_t imm = READ_INST_16_1();
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::ArrayLiteralIDType, imm));
            break;
        }
        case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM16_ID16: {
            uint16_t imm = READ_INST_16_2();
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::ArrayLiteralIDType, imm));
            break;
        }
        case EcmaOpcode::GETMODULENAMESPACE_IMM8: {
            int32_t index = READ_INST_8_0();
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::WIDE_GETMODULENAMESPACE_PREF_IMM16: {
            int32_t index = READ_INST_16_1();
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::STMODULEVAR_IMM8: {
            int32_t index = READ_INST_8_0();
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::SETGENERATORSTATE_IMM8: {
            int32_t index = READ_INST_8_0();
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::WIDE_STMODULEVAR_PREF_IMM16: {
            int32_t index = READ_INST_16_1();
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::LDLOCALMODULEVAR_IMM8: {
            int32_t index = READ_INST_8_0();
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::WIDE_LDLOCALMODULEVAR_PREF_IMM16: {
            int32_t index = READ_INST_16_1();
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::LDEXTERNALMODULEVAR_IMM8: {
            int32_t index = READ_INST_8_0();
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::WIDE_LDEXTERNALMODULEVAR_PREF_IMM16: {
            int32_t index = READ_INST_16_1();
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::CALLRUNTIME_LDSENDABLEEXTERNALMODULEVAR_PREF_IMM8: {
            int32_t index = READ_INST_8_1();
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::CALLRUNTIME_WIDELDSENDABLEEXTERNALMODULEVAR_PREF_IMM16: {
            int32_t index = READ_INST_16_1();
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::CALLRUNTIME_LDSENDABLELOCALMODULEVAR_PREF_IMM8: {
            int32_t index = READ_INST_8_1();
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::CALLRUNTIME_WIDELDSENDABLELOCALMODULEVAR_PREF_IMM16: {
            int32_t index = READ_INST_16_1();
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::CALLRUNTIME_LDLAZYMODULEVAR_PREF_IMM8: {
            int32_t index = READ_INST_8_1();
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::CALLRUNTIME_WIDELDLAZYMODULEVAR_PREF_IMM16: {
            int32_t index = READ_INST_16_1();
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::CALLRUNTIME_LDLAZYSENDABLEMODULEVAR_PREF_IMM8: {
            int32_t index = READ_INST_8_1();
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::CALLRUNTIME_WIDELDLAZYSENDABLEMODULEVAR_PREF_IMM16: {
            int32_t index = READ_INST_16_1();
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::STCONSTTOGLOBALRECORD_IMM16_ID16: {
            uint16_t stringId = READ_INST_16_2();
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            break;
        }
        case EcmaOpcode::GETTEMPLATEOBJECT_IMM8:
        case EcmaOpcode::GETTEMPLATEOBJECT_IMM16:
            break;
        case EcmaOpcode::COPYDATAPROPERTIES_V8: {
            uint16_t v0 = READ_INST_8_0();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::STOWNBYINDEX_IMM8_V8_IMM16: {
            uint8_t v0 = READ_INST_8_1();
            uint16_t index = READ_INST_16_2();
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::STOWNBYINDEX_IMM16_V8_IMM16: {
            uint8_t v0 = READ_INST_8_2();
            uint16_t index = READ_INST_16_3();
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::WIDE_STOWNBYINDEX_PREF_V8_IMM32: {
            uint32_t v0 = READ_INST_8_1();
            uint32_t index = READ_INST_32_2();
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::STOWNBYVALUE_IMM8_V8_V8: {
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::STOWNBYVALUE_IMM16_V8_V8: {
            uint32_t v0 = READ_INST_8_2();
            uint32_t v1 = READ_INST_8_3();
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::CREATEOBJECTWITHEXCLUDEDKEYS_IMM8_V8_V8: {
            uint8_t numKeys = READ_INST_8_0();
            uint16_t v0 = READ_INST_8_1();
            uint16_t firstArgRegIdx = READ_INST_8_2();
            info.inputs.emplace_back(VirtualRegister(v0));
            for (int i = 0; i <= numKeys; i++) {
                info.inputs.emplace_back(VirtualRegister(firstArgRegIdx + i));
            }
            break;
        }
        case EcmaOpcode::WIDE_CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8: {
            uint16_t numKeys = READ_INST_16_1();
            uint16_t v0 = READ_INST_8_3();
            uint16_t firstArgRegIdx = READ_INST_8_4();
            info.inputs.emplace_back(VirtualRegister(v0));
            for (int i = 0; i <= numKeys; i++) {
                info.inputs.emplace_back(VirtualRegister(firstArgRegIdx + i));
            }
            break;
        }
        case EcmaOpcode::COPYRESTARGS_IMM8: {
            uint16_t restIdx = READ_INST_8_0();
            info.inputs.emplace_back(Immediate(restIdx));
            break;
        }
        case EcmaOpcode::WIDE_COPYRESTARGS_PREF_IMM16: {
            uint16_t restIdx = READ_INST_16_1();
            info.inputs.emplace_back(Immediate(restIdx));
            break;
        }
        case EcmaOpcode::DEFINEGETTERSETTERBYVALUE_V8_V8_V8_V8: {
            uint16_t v0 = READ_INST_8_0();
            uint16_t v1 = READ_INST_8_1();
            uint16_t v2 = READ_INST_8_2();
            uint16_t v3 = READ_INST_8_3();
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            info.inputs.emplace_back(VirtualRegister(v2));
            info.inputs.emplace_back(VirtualRegister(v3));
            break;
        }
        case EcmaOpcode::LDOBJBYINDEX_IMM8_IMM16: {
            uint32_t idx = READ_INST_16_1();
            info.inputs.emplace_back(Immediate(idx));
            break;
        }
        case EcmaOpcode::LDOBJBYINDEX_IMM16_IMM16: {
            uint32_t idx = READ_INST_16_2();
            info.inputs.emplace_back(Immediate(idx));
            break;
        }
        case EcmaOpcode::WIDE_LDOBJBYINDEX_PREF_IMM32: {
            uint32_t idx = READ_INST_32_1();
            info.inputs.emplace_back(Immediate(idx));
            break;
        }
        case EcmaOpcode::STOBJBYINDEX_IMM8_V8_IMM16: {
            uint8_t v0 = READ_INST_8_1();
            uint16_t index = READ_INST_16_2();
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::STOBJBYINDEX_IMM16_V8_IMM16: {
            uint8_t v0 = READ_INST_8_2();
            uint16_t index = READ_INST_16_3();
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::WIDE_STOBJBYINDEX_PREF_V8_IMM32: {
            uint8_t v0 = READ_INST_8_1();
            uint32_t index = READ_INST_32_2();
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::LDOBJBYVALUE_IMM8_V8: {
            uint16_t slotId = READ_INST_8_0();
            uint32_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(ICSlotId(slotId));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::LDOBJBYVALUE_IMM16_V8: {
            uint16_t slotId = READ_INST_16_0();
            uint32_t v0 = READ_INST_8_2();
            info.inputs.emplace_back(ICSlotId(slotId));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::STOBJBYVALUE_IMM8_V8_V8: {
            uint16_t slotId = READ_INST_8_0();
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.inputs.emplace_back(ICSlotId(slotId));
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::STOBJBYVALUE_IMM16_V8_V8: {
            uint16_t slotId = READ_INST_16_0();
            uint32_t v0 = READ_INST_8_2();
            uint32_t v1 = READ_INST_8_3();
            info.inputs.emplace_back(ICSlotId(slotId));
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::LDSUPERBYVALUE_IMM8_V8: {
            uint32_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::LDSUPERBYVALUE_IMM16_V8: {
            uint32_t v0 = READ_INST_8_2();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::STSUPERBYVALUE_IMM8_V8_V8: {
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::STSUPERBYVALUE_IMM16_V8_V8: {
            uint32_t v0 = READ_INST_8_2();
            uint32_t v1 = READ_INST_8_3();
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::TRYLDGLOBALBYNAME_IMM8_ID16: {
            uint16_t slotId = READ_INST_8_0();
            uint16_t stringId = READ_INST_16_1();
            info.inputs.emplace_back(ICSlotId(slotId));
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            break;
        }
        case EcmaOpcode::TRYLDGLOBALBYNAME_IMM16_ID16: {
            uint16_t slotId = READ_INST_16_0();
            uint16_t stringId = READ_INST_16_2();
            info.inputs.emplace_back(ICSlotId(slotId));
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            break;
        }
        case EcmaOpcode::TRYSTGLOBALBYNAME_IMM8_ID16: {
            uint16_t slotId = READ_INST_8_0();
            uint16_t stringId = READ_INST_16_1();
            info.inputs.emplace_back(ICSlotId(slotId));
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            break;
        }
        case EcmaOpcode::TRYSTGLOBALBYNAME_IMM16_ID16: {
            uint16_t slotId = READ_INST_16_0();
            uint16_t stringId = READ_INST_16_2();
            info.inputs.emplace_back(ICSlotId(slotId));
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            break;
        }
        case EcmaOpcode::STTOGLOBALRECORD_IMM16_ID16: {
            uint16_t stringId = READ_INST_16_2();
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            break;
        }
        case EcmaOpcode::STOWNBYVALUEWITHNAMESET_IMM8_V8_V8: {
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::STOWNBYVALUEWITHNAMESET_IMM16_V8_V8: {
            uint32_t v0 = READ_INST_8_2();
            uint32_t v1 = READ_INST_8_3();
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8: {
            uint16_t stringId = READ_INST_16_1();
            uint32_t v0 = READ_INST_8_3();
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::STOWNBYNAMEWITHNAMESET_IMM16_ID16_V8: {
            uint16_t stringId = READ_INST_16_2();
            uint32_t v0 = READ_INST_8_4();
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::STTHISBYVALUE_IMM8_V8: {
            uint16_t slotId = READ_INST_8_0();
            uint32_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(ICSlotId(slotId));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::STTHISBYVALUE_IMM16_V8: {
            uint16_t slotId = READ_INST_16_0();
            uint32_t v0 = READ_INST_8_2();
            info.inputs.emplace_back(ICSlotId(slotId));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        // not implement
        case EcmaOpcode::JSTRICTEQZ_IMM8:
        case EcmaOpcode::JSTRICTEQZ_IMM16:
        case EcmaOpcode::JNSTRICTEQZ_IMM8:
        case EcmaOpcode::JNSTRICTEQZ_IMM16:
        case EcmaOpcode::JEQNULL_IMM8:
        case EcmaOpcode::JEQNULL_IMM16:
        case EcmaOpcode::JNENULL_IMM8:
        case EcmaOpcode::JNENULL_IMM16:
        case EcmaOpcode::JSTRICTEQNULL_IMM8:
        case EcmaOpcode::JSTRICTEQNULL_IMM16:
        case EcmaOpcode::JNSTRICTEQNULL_IMM8:
        case EcmaOpcode::JNSTRICTEQNULL_IMM16:
        case EcmaOpcode::JEQUNDEFINED_IMM8:
        case EcmaOpcode::JEQUNDEFINED_IMM16:
        case EcmaOpcode::JNEUNDEFINED_IMM8:
        case EcmaOpcode::JNEUNDEFINED_IMM16:
        case EcmaOpcode::JSTRICTEQUNDEFINED_IMM8:
        case EcmaOpcode::JSTRICTEQUNDEFINED_IMM16:
        case EcmaOpcode::JNSTRICTEQUNDEFINED_IMM8:
        case EcmaOpcode::JNSTRICTEQUNDEFINED_IMM16:
        case EcmaOpcode::JEQ_V8_IMM8:
        case EcmaOpcode::JEQ_V8_IMM16:
        case EcmaOpcode::JNE_V8_IMM8:
        case EcmaOpcode::JNE_V8_IMM16:
        case EcmaOpcode::JSTRICTEQ_V8_IMM8:
        case EcmaOpcode::JSTRICTEQ_V8_IMM16:
        case EcmaOpcode::JNSTRICTEQ_V8_IMM8:
        case EcmaOpcode::JNSTRICTEQ_V8_IMM16:
        case EcmaOpcode::LDTHIS:
            break;
        case EcmaOpcode::LDTHISBYNAME_IMM8_ID16: {
            uint16_t slotId = READ_INST_8_0();
            uint16_t stringId = READ_INST_16_1();
            info.inputs.emplace_back(ICSlotId(slotId));
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            break;
        }
        case EcmaOpcode::LDTHISBYNAME_IMM16_ID16: {
            uint16_t slotId = READ_INST_16_0();
            uint16_t stringId = READ_INST_16_2();
            info.inputs.emplace_back(ICSlotId(slotId));
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            break;
        }
        case EcmaOpcode::STTHISBYNAME_IMM8_ID16: {
            uint16_t slotId = READ_INST_8_0();
            uint16_t stringId = READ_INST_16_1();
            info.inputs.emplace_back(ICSlotId(slotId));
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            break;
        }
        case EcmaOpcode::STTHISBYNAME_IMM16_ID16: {
            uint16_t slotId = READ_INST_16_0();
            uint16_t stringId = READ_INST_16_2();
            info.inputs.emplace_back(ICSlotId(slotId));
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            break;
        }
        case EcmaOpcode::LDGLOBALVAR_IMM16_ID16: {
            uint16_t slotId = READ_INST_16_0();
            uint16_t stringId = READ_INST_16_2();
            info.inputs.emplace_back(ICSlotId(slotId));
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            break;
        }
        case EcmaOpcode::LDOBJBYNAME_IMM8_ID16: {
            uint16_t slotId = READ_INST_8_0();
            uint16_t stringId = READ_INST_16_1();
            info.inputs.emplace_back(ICSlotId(slotId));
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            break;
        }
        case EcmaOpcode::LDOBJBYNAME_IMM16_ID16: {
            uint16_t slotId = READ_INST_16_0();
            uint16_t stringId = READ_INST_16_2();
            info.inputs.emplace_back(ICSlotId(slotId));
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            break;
        }
        case EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8: {
            uint16_t slotId = READ_INST_8_0();
            uint16_t stringId = READ_INST_16_1();
            uint32_t v0 = READ_INST_8_3();
            info.inputs.emplace_back(ICSlotId(slotId));
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::STOBJBYNAME_IMM16_ID16_V8: {
            uint16_t slotId = READ_INST_16_0();
            uint16_t stringId = READ_INST_16_2();
            uint32_t v0 = READ_INST_8_4();
            info.inputs.emplace_back(ICSlotId(slotId));
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::LDSUPERBYNAME_IMM8_ID16: {
            uint16_t stringId = READ_INST_16_1();
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            break;
        }
        case EcmaOpcode::LDSUPERBYNAME_IMM16_ID16: {
            uint16_t stringId = READ_INST_16_2();
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            break;
        }
        case EcmaOpcode::STSUPERBYNAME_IMM8_ID16_V8: {
            uint16_t stringId = READ_INST_16_1();
            uint32_t v0 = READ_INST_8_3();
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::STSUPERBYNAME_IMM16_ID16_V8: {
            uint16_t stringId = READ_INST_16_2();
            uint32_t v0 = READ_INST_8_4();
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::STGLOBALVAR_IMM16_ID16: {
            uint16_t slotId = READ_INST_16_0();
            uint32_t stringId = READ_INST_16_2();
            info.inputs.emplace_back(ICSlotId(slotId));
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            break;
        }
        case EcmaOpcode::CREATEGENERATOROBJ_V8: {
            uint16_t v0 = READ_INST_8_0();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::CREATEASYNCGENERATOROBJ_V8: {
            uint16_t v0 = READ_INST_8_0();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::ASYNCGENERATORRESOLVE_V8_V8_V8: {
            uint16_t v0 = READ_INST_8_0();
            uint16_t v1 = READ_INST_8_1();
            uint16_t v2 = READ_INST_8_2();
            uint32_t offset = builder->GetPcOffset(pc);
            offset += BytecodeInstruction::Size(EcmaOpcode::ASYNCGENERATORRESOLVE_V8_V8_V8); // skip suspend bc
            info.inputs.emplace_back(Immediate(offset)); // Save the pc offset
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            info.inputs.emplace_back(VirtualRegister(v2));
            break;
        }
        case EcmaOpcode::ASYNCGENERATORREJECT_V8: {
            uint16_t v0 = READ_INST_8_0();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::STARRAYSPREAD_V8_V8: {
            uint16_t v0 = READ_INST_8_0();
            uint16_t v1 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8: {
            uint16_t slotId = READ_INST_8_0();
            uint16_t methodId = READ_INST_16_1();
            uint16_t literaId = READ_INST_16_3();
            uint16_t length = READ_INST_16_5();
            uint16_t v0 = READ_INST_8_7();
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::MethodIDType, methodId));
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::ClassLiteralIDType, literaId));
            info.inputs.emplace_back(Immediate(length));
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            info.inputs.emplace_back(ICSlotId(slotId));
            break;
        }
        case EcmaOpcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8: {
            uint16_t slotId = READ_INST_16_0();
            uint16_t methodId = READ_INST_16_2();
            uint16_t literaId = READ_INST_16_4();
            uint16_t length = READ_INST_16_6();
            uint16_t v0 = READ_INST_8_8();
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::MethodIDType, methodId));
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::ClassLiteralIDType, literaId));
            info.inputs.emplace_back(Immediate(length));
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            info.inputs.emplace_back(ICSlotId(slotId));
            break;
        }
        case EcmaOpcode::LDFUNCTION: {
            break;
        }
        case EcmaOpcode::LDBIGINT_ID16: {
            uint32_t stringId = READ_INST_16_0();
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            break;
        }
        case EcmaOpcode::DYNAMICIMPORT: {
            break;
        }
        case EcmaOpcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::SUPERCALLARROWRANGE_IMM8_IMM8_V8: {
            uint16_t range = READ_INST_8_1();
            uint16_t v0 = READ_INST_8_2();
            for (size_t i = 0; i < range; i++) {
                info.inputs.emplace_back(VirtualRegister(v0 + i));
            }
            break;
        }
        case EcmaOpcode::WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8:
        case EcmaOpcode::WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8: {
            uint16_t range = READ_INST_16_1();
            uint16_t v0 = READ_INST_8_3();
            for (size_t i = 0; i < range; i++) {
                info.inputs.emplace_back(VirtualRegister(v0 + i));
            }
            break;
        }
        case EcmaOpcode::SUPERCALLSPREAD_IMM8_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::CALLRUNTIME_SUPERCALLFORWARDALLARGS_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::POPLEXENV: {
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            info.vregOut.emplace_back(builder->GetEnvVregIdx());
            break;
        }
        case EcmaOpcode::LDPRIVATEPROPERTY_IMM8_IMM16_IMM16: {
            uint32_t slotId = READ_INST_8_0();
            uint32_t levelIndex = READ_INST_16_1();
            uint32_t slotIndex = READ_INST_16_3();
            info.inputs.emplace_back(Immediate(slotId));
            info.inputs.emplace_back(Immediate(levelIndex));
            info.inputs.emplace_back(Immediate(slotIndex));
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            break;
        }
        case EcmaOpcode::STPRIVATEPROPERTY_IMM8_IMM16_IMM16_V8: {
            uint32_t slotId = READ_INST_8_0();
            uint32_t levelIndex = READ_INST_16_1();
            uint32_t slotIndex = READ_INST_16_3();
            uint32_t v0 = READ_INST_8_5();
            info.inputs.emplace_back(Immediate(slotId));
            info.inputs.emplace_back(Immediate(levelIndex));
            info.inputs.emplace_back(Immediate(slotIndex));
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            break;
        }
        case EcmaOpcode::TESTIN_IMM8_IMM16_IMM16: {
            uint32_t slotId = READ_INST_8_0();
            uint32_t levelIndex = READ_INST_16_1();
            uint32_t slotIndex = READ_INST_16_3();
            info.inputs.emplace_back(Immediate(slotId));
            info.inputs.emplace_back(Immediate(levelIndex));
            info.inputs.emplace_back(Immediate(slotIndex));
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            break;
        }
        case EcmaOpcode::DEFINEPROPERTYBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::DEFINEFIELDBYNAME_IMM8_ID16_V8: {
            uint16_t slotId = READ_INST_8_0();
            uint16_t stringId = READ_INST_16_1();
            uint32_t v0 = READ_INST_8_3();
            info.inputs.emplace_back(ICSlotId(slotId));
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::StringIDType, stringId));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::CALLRUNTIME_DEFINEFIELDBYVALUE_PREF_IMM8_V8_V8: {
            uint32_t v0 = READ_INST_8_2();
            uint32_t v1 = READ_INST_8_3();
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::CALLRUNTIME_DEFINEFIELDBYINDEX_PREF_IMM8_IMM32_V8: {
            uint32_t index = READ_INST_32_2();
            uint32_t v0 = READ_INST_8_6();
            info.inputs.emplace_back(Immediate(index));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::CALLRUNTIME_CREATEPRIVATEPROPERTY_PREF_IMM16_ID16: {
            uint32_t count = READ_INST_16_1();
            uint32_t literalId = READ_INST_16_3();
            info.inputs.emplace_back(Immediate(count));
            info.inputs.emplace_back(Immediate(literalId));
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            break;
        }
        case EcmaOpcode::CALLRUNTIME_DEFINEPRIVATEPROPERTY_PREF_IMM8_IMM16_IMM16_V8: {
            uint32_t levelIndex = READ_INST_16_2();
            uint32_t slotIndex = READ_INST_16_4();
            uint32_t v0 = READ_INST_8_6();
            info.inputs.emplace_back(Immediate(levelIndex));
            info.inputs.emplace_back(Immediate(slotIndex));
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            break;
        }
        case EcmaOpcode::CALLRUNTIME_DEFINESENDABLECLASS_PREF_IMM16_ID16_ID16_IMM16_V8: {
            uint16_t methodId = READ_INST_16_3();
            uint16_t literaId = READ_INST_16_5();
            uint16_t length = READ_INST_16_7();
            uint16_t v0 = READ_INST_8_9();
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::MethodIDType, methodId));
            info.inputs.emplace_back(ConstDataId(ConstDataIDType::ClassLiteralIDType, literaId));
            info.inputs.emplace_back(Immediate(length));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::CALLRUNTIME_LDSENDABLECLASS_PREF_IMM16: {
            uint16_t level = READ_INST_16_1();
            info.inputs.emplace_back(Immediate(level));
            info.inputs.emplace_back(VirtualRegister(builder->GetEnvVregIdx()));
            break;
        }
        case EcmaOpcode::TONUMERIC_IMM8:
        case EcmaOpcode::INC_IMM8:
        case EcmaOpcode::DEC_IMM8:
        case EcmaOpcode::NOT_IMM8:
        case EcmaOpcode::NEG_IMM8:
        case EcmaOpcode::JMP_IMM8:
        case EcmaOpcode::JMP_IMM16:
        case EcmaOpcode::JMP_IMM32:
        case EcmaOpcode::JEQZ_IMM8:
        case EcmaOpcode::JEQZ_IMM16:
        case EcmaOpcode::JEQZ_IMM32:
        case EcmaOpcode::JNEZ_IMM8:
        case EcmaOpcode::JNEZ_IMM16:
        case EcmaOpcode::JNEZ_IMM32:
        case EcmaOpcode::RETURN:
        case EcmaOpcode::RETURNUNDEFINED:
        case EcmaOpcode::LDNAN:
        case EcmaOpcode::LDINFINITY:
        case EcmaOpcode::LDNEWTARGET:
        case EcmaOpcode::LDUNDEFINED:
        case EcmaOpcode::LDNULL:
        case EcmaOpcode::LDSYMBOL:
        case EcmaOpcode::LDGLOBAL:
        case EcmaOpcode::LDTRUE:
        case EcmaOpcode::LDFALSE:
        case EcmaOpcode::LDHOLE:
        case EcmaOpcode::CALLARG0_IMM8:
        case EcmaOpcode::GETUNMAPPEDARGS:
        case EcmaOpcode::ASYNCFUNCTIONENTER:
        case EcmaOpcode::TYPEOF_IMM8:
        case EcmaOpcode::TYPEOF_IMM16:
        case EcmaOpcode::TONUMBER_IMM8:
        case EcmaOpcode::THROW_PREF_NONE:
        case EcmaOpcode::GETPROPITERATOR:
        case EcmaOpcode::GETRESUMEMODE:
        case EcmaOpcode::CREATEEMPTYARRAY_IMM8:
        case EcmaOpcode::CREATEEMPTYARRAY_IMM16:
        case EcmaOpcode::CREATEEMPTYOBJECT:
        case EcmaOpcode::DEBUGGER:
        case EcmaOpcode::ISTRUE:
        case EcmaOpcode::ISFALSE:
        case EcmaOpcode::CALLRUNTIME_ISTRUE_PREF_IMM8:
        case EcmaOpcode::CALLRUNTIME_ISFALSE_PREF_IMM8:
        case EcmaOpcode::NOP:
        case EcmaOpcode::GETITERATOR_IMM8:
        case EcmaOpcode::GETITERATOR_IMM16:
        case EcmaOpcode::GETASYNCITERATOR_IMM8:
        case EcmaOpcode::THROW_NOTEXISTS_PREF_NONE:
        case EcmaOpcode::THROW_PATTERNNONCOERCIBLE_PREF_NONE:
        case EcmaOpcode::THROW_DELETESUPERPROPERTY_PREF_NONE:
        case EcmaOpcode::RESUMEGENERATOR:
        case EcmaOpcode::CALLRUNTIME_NOTIFYCONCURRENTRESULT_PREF_NONE:
        case EcmaOpcode::CALLRUNTIME_TOPROPERTYKEY_PREF_NONE:
            break;
        case EcmaOpcode::LDTHISBYVALUE_IMM8: {
            uint16_t slotId = READ_INST_8_0();
            info.inputs.emplace_back(ICSlotId(slotId));
            break;
        }
        case EcmaOpcode::LDTHISBYVALUE_IMM16: {
            uint16_t slotId = READ_INST_16_0();
            info.inputs.emplace_back(ICSlotId(slotId));
            break;
        }
        case EcmaOpcode::WIDE_LDPATCHVAR_PREF_IMM16: {
            uint16_t index = READ_INST_16_1();
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::WIDE_STPATCHVAR_PREF_IMM16: {
            uint16_t index = READ_INST_16_1();
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        default: {
            LOG_COMPILER(FATAL) << "Error bytecode: " << static_cast<uint16_t>(opcode);
            UNREACHABLE();
            break;
        }
    }
}

void BytecodeInfo::SetInsufficientProfile(BytecodeCircuitBuilder *builder, const uint8_t *pc)
{
    if (builder->IsJitCompile()) {
        isInsufficientProfile_ = builder->GetPGOTypeRecorder()->IsInsufficientProfile(builder->GetPcOffset(pc));
    }
}

const BytecodeInfo &BytecodeIterator::GetBytecodeInfo() const
{
    return builder_->GetBytecodeInfo(index_);
}

const uint8_t *BytecodeIterator::PeekNextPc(size_t i) const
{
    ASSERT(index_ + static_cast<int32_t>(i) <= end_);
    return builder_->GetPCByIndex(static_cast<uint32_t>(index_ + i));
}

const uint8_t *BytecodeIterator::PeekPrevPc(size_t i) const
{
    ASSERT(index_ - static_cast<int32_t>(i) >= start_);
    return builder_->GetPCByIndex(static_cast<uint32_t>(index_ - i));
}
} // panda::ecmascript::kungfu
