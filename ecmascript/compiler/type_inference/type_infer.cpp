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

#include "ecmascript/compiler/type_inference/type_infer.h"

#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/ts_types/ts_type_parser.h"

namespace panda::ecmascript::kungfu {
void TypeInfer::TraverseCircuit()
{
    std::vector<GateRef> gateList;
    circuit_->GetAllGates(gateList);
    for (auto gate : gateList) {
        pendingQueue_.push(gate);
    }
    // init jsgateToBytecode
    BytecodeIterator iterator(builder_, 0, builder_->GetLastBcIndex());
    for (iterator.GotoStart(); !iterator.Done(); ++iterator) {
        auto index = iterator.Index();
        GateRef gate = builder_->GetGateByBcIndex(index);
        if (gate != Circuit::NullGate()) {
            jsgateToBytecode_[gate] = index;
        }
    }
    TraverseInfer();

    if (IsLogEnabled()) {
        PrintAllByteCodesTypes();
    }

    if (tsManager_->AssertTypes()) {
        Verify();
    }

    if (tsManager_->PrintAnyTypes()) {
        FilterAnyTypeGates();
    }
    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After ts type infer "
                           << "[" << GetMethodName() << "]"
                           << "===================="
                           << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
    }
}

void TypeInfer::TraverseInfer()
{
    bool needUpdateForLoopPhi = true;
    // main type infer for all gates
    while (!pendingQueue_.empty()) {
        auto curGate = pendingQueue_.front();
        inQueue_[gateAccessor_.GetId(curGate)] = false;
        pendingQueue_.pop();
        auto uses = gateAccessor_.ConstUses(curGate);
        for (auto useIt = uses.begin(); useIt != uses.end(); useIt++) {
            auto gateId = gateAccessor_.GetId(*useIt);
            if (Infer(*useIt) && !inQueue_[gateId]) {
                inQueue_[gateId] = true;
                pendingQueue_.push(*useIt);
            }
        }
        if (pendingQueue_.empty() && needUpdateForLoopPhi) {
            // only for loop begin phi
            UpdateQueueForLoopPhi();
            needUpdateForLoopPhi = false;
        }
    }
}

void TypeInfer::UpdateQueueForLoopPhi()
{
    for (auto it = loopPhiState_.begin(); it != loopPhiState_.end(); it++) {
        auto curGate = it->first;
        auto loopType = gateAccessor_.GetGateType(curGate);
        auto loopBackGate = gateAccessor_.GetValueIn(curGate, 1);
        auto loopBackType = gateAccessor_.GetGateType(loopBackGate);
        // if loopBack Gate is finally AnyType, loop-begin phi gate should be changed to any
        if (!loopType.IsAnyType() && loopBackType.IsAnyType()) {
            gateAccessor_.SetGateType(curGate, GateType::AnyType());
            loopPhiState_[curGate] = InferState::ANY_INFERED;
            pendingQueue_.push(curGate);
            inQueue_[gateAccessor_.GetId(curGate)] = true;
        }
        // if loopBack Gate is finally not same number Type, loop-begin phi gate should be promoted
        if (loopType.IsNumberType() && loopBackType.IsNumberType() && loopType != loopBackType) {
            gateAccessor_.SetGateType(curGate, GateType::NumberType());
            loopPhiState_[curGate] = InferState::NUMBER_INFERED;
            pendingQueue_.push(curGate);
            inQueue_[gateAccessor_.GetId(curGate)] = true;
        }
    }
}

bool TypeInfer::UpdateType(GateRef gate, const GateType type)
{
    auto preType = gateAccessor_.GetGateType(gate);
    if (type != preType) {
        gateAccessor_.SetGateType(gate, type);
        return true;
    }
    return false;
}

bool TypeInfer::UpdateType(GateRef gate, const GlobalTSTypeRef &typeRef)
{
    auto type = GateType(typeRef);
    return UpdateType(gate, type);
}

bool TypeInfer::IsNewLexEnv(EcmaOpcode opcode) const
{
    switch (opcode) {
        case EcmaOpcode::NEWLEXENV_IMM8:
        case EcmaOpcode::NEWLEXENVWITHNAME_IMM8_ID16:
        case EcmaOpcode::WIDE_NEWLEXENV_PREF_IMM16:
        case EcmaOpcode::WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16:
            return true;
        default:
            return false;
    }
}

bool TypeInfer::ShouldInfer(const GateRef gate) const
{
    auto opcode = gateAccessor_.GetOpCode(gate);
    // handle phi gates
    if (opcode == OpCode::VALUE_SELECTOR) {
        return true;
    }
    /* Handle constant gates (like ldnull and ldtrue), return gates and gates generated by ecma.* bytecodes (not
     * including jump and newlexenv). Jump instructions are skipped because they have no intrinsic type information.
     * And newlexenv instructions are used to create runtime lexical env objects which have no TS types associated. As
     * for the type inference on lexical variables, their type information is recorded in objects of class
     * panda::ecmascript::kungfu::LexEnv which are created during the building of IR. So in the type inference,
     * newlexenv is ignored.
     */
    if (opcode != OpCode::CONSTANT && opcode != OpCode::RETURN && opcode != OpCode::JS_BYTECODE) {
        return false;
    }
    if (jsgateToBytecode_.find(gate) == jsgateToBytecode_.end()) {
        return false;
    }
    auto &bytecodeInfo = GetByteCodeInfo(gate);
    return !bytecodeInfo.IsJump() && !IsNewLexEnv(bytecodeInfo.GetOpcode());
}

bool TypeInfer::Infer(GateRef gate)
{
    if (!ShouldInfer(gate)) {
        return false;
    }
    if (gateAccessor_.GetOpCode(gate) == OpCode::VALUE_SELECTOR) {
        return InferPhiGate(gate);
    }
    // infer ecma.* bytecode gates
    auto &bytecodeInfo = GetByteCodeInfo(gate);
    switch (bytecodeInfo.GetOpcode()) {
        case EcmaOpcode::LDNAN:
        case EcmaOpcode::LDINFINITY:
        case EcmaOpcode::MOD2_IMM8_V8:
        case EcmaOpcode::AND2_IMM8_V8:
        case EcmaOpcode::OR2_IMM8_V8:
        case EcmaOpcode::XOR2_IMM8_V8:
        case EcmaOpcode::TONUMBER_IMM8:
        case EcmaOpcode::TONUMERIC_IMM8:
        case EcmaOpcode::NEG_IMM8:
        case EcmaOpcode::EXP_IMM8_V8:
        case EcmaOpcode::STARRAYSPREAD_V8_V8:
            return SetNumberType(gate);
        case EcmaOpcode::SHL2_IMM8_V8:
        case EcmaOpcode::ASHR2_IMM8_V8:
        case EcmaOpcode::SHR2_IMM8_V8:
        case EcmaOpcode::NOT_IMM8:
            return SetIntType(gate);
        case EcmaOpcode::LDBIGINT_ID16:
            return SetBigIntType(gate);
        case EcmaOpcode::LDTRUE:
        case EcmaOpcode::LDFALSE:
        case EcmaOpcode::EQ_IMM8_V8:
        case EcmaOpcode::NOTEQ_IMM8_V8:
        case EcmaOpcode::LESS_IMM8_V8:
        case EcmaOpcode::LESSEQ_IMM8_V8:
        case EcmaOpcode::GREATER_IMM8_V8:
        case EcmaOpcode::GREATEREQ_IMM8_V8:
        case EcmaOpcode::ISIN_IMM8_V8:
        case EcmaOpcode::INSTANCEOF_IMM8_V8:
        case EcmaOpcode::STRICTNOTEQ_IMM8_V8:
        case EcmaOpcode::STRICTEQ_IMM8_V8:
        case EcmaOpcode::ISTRUE:
        case EcmaOpcode::ISFALSE:
        case EcmaOpcode::SETOBJECTWITHPROTO_IMM8_V8:
        case EcmaOpcode::SETOBJECTWITHPROTO_IMM16_V8:
        case EcmaOpcode::DELOBJPROP_V8:
            return SetBooleanType(gate);
        case EcmaOpcode::LDUNDEFINED:
            return InferLdUndefined(gate);
        case EcmaOpcode::LDNULL:
            return InferLdNull(gate);
        case EcmaOpcode::LDAI_IMM32:
            return InferLdai(gate);
        case EcmaOpcode::FLDAI_IMM64:
            return InferFLdai(gate);
        case EcmaOpcode::LDSYMBOL:
            return InferLdSymbol(gate);
        case EcmaOpcode::THROW_PREF_NONE:
            return InferThrow(gate);
        case EcmaOpcode::TYPEOF_IMM8:
        case EcmaOpcode::TYPEOF_IMM16:
            return InferTypeOf(gate);
        case EcmaOpcode::ADD2_IMM8_V8:
            return InferAdd2(gate);
        case EcmaOpcode::SUB2_IMM8_V8:
            return InferSub2(gate);
        case EcmaOpcode::MUL2_IMM8_V8:
            return InferMul2(gate);
        case EcmaOpcode::DIV2_IMM8_V8:
            return InferDiv2(gate);
        case EcmaOpcode::INC_IMM8:
        case EcmaOpcode::DEC_IMM8:
            return InferIncDec(gate);
        case EcmaOpcode::LDOBJBYINDEX_IMM8_IMM16:
        case EcmaOpcode::LDOBJBYINDEX_IMM16_IMM16:
        case EcmaOpcode::WIDE_LDOBJBYINDEX_PREF_IMM32:
            return InferLdObjByIndex(gate);
        case EcmaOpcode::STGLOBALVAR_IMM16_ID16:
        case EcmaOpcode::TRYSTGLOBALBYNAME_IMM8_ID16:
        case EcmaOpcode::TRYSTGLOBALBYNAME_IMM16_ID16:
            return SetStGlobalBcType(gate, true);
        case EcmaOpcode::STTOGLOBALRECORD_IMM16_ID16:
        case EcmaOpcode::STCONSTTOGLOBALRECORD_IMM16_ID16:
            return SetStGlobalBcType(gate, false);
        case EcmaOpcode::LDGLOBALVAR_IMM16_ID16:
            return InferLdGlobalVar(gate);
        case EcmaOpcode::RETURNUNDEFINED:
            return InferReturnUndefined(gate);
        case EcmaOpcode::RETURN:
            return InferReturn(gate);
        case EcmaOpcode::LDOBJBYNAME_IMM8_ID16:
        case EcmaOpcode::LDOBJBYNAME_IMM16_ID16:
            return InferLdObjByName(gate);
        case EcmaOpcode::LDA_STR_ID16:
            return InferLdStr(gate);
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
        case EcmaOpcode::APPLY_IMM8_V8_V8:
            return InferCallFunction(gate);
        case EcmaOpcode::LDOBJBYVALUE_IMM8_V8:
        case EcmaOpcode::LDOBJBYVALUE_IMM16_V8:
            return InferLdObjByValue(gate);
        case EcmaOpcode::GETNEXTPROPNAME_V8:
            return InferGetNextPropName(gate);
        case EcmaOpcode::DEFINEGETTERSETTERBYVALUE_V8_V8_V8_V8:
            return InferDefineGetterSetterByValue(gate);
        case EcmaOpcode::NEWOBJRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::NEWOBJRANGE_IMM16_IMM8_V8:
        case EcmaOpcode::WIDE_NEWOBJRANGE_PREF_IMM16_V8:
        case EcmaOpcode::NEWOBJAPPLY_IMM8_V8:
        case EcmaOpcode::NEWOBJAPPLY_IMM16_V8:
            return InferNewObject(gate);
        case EcmaOpcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8:
        case EcmaOpcode::SUPERCALLARROWRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8:
        case EcmaOpcode::SUPERCALLSPREAD_IMM8_V8:
            return InferSuperCall(gate);
        case EcmaOpcode::LDSUPERBYNAME_IMM8_ID16:
        case EcmaOpcode::LDSUPERBYNAME_IMM16_ID16:
            return InferSuperPropertyByName(gate);
        case EcmaOpcode::LDSUPERBYVALUE_IMM8_V8:
        case EcmaOpcode::LDSUPERBYVALUE_IMM16_V8:
            return InferSuperPropertyByValue(gate);
        case EcmaOpcode::TRYLDGLOBALBYNAME_IMM8_ID16:
        case EcmaOpcode::TRYLDGLOBALBYNAME_IMM16_ID16:
            return InferTryLdGlobalByName(gate);
        case EcmaOpcode::LDLEXVAR_IMM4_IMM4:
        case EcmaOpcode::LDLEXVAR_IMM8_IMM8:
        case EcmaOpcode::WIDE_LDLEXVAR_PREF_IMM16_IMM16:
            return InferLdLexVarDyn(gate);
        case EcmaOpcode::STLEXVAR_IMM4_IMM4:
        case EcmaOpcode::STLEXVAR_IMM8_IMM8:
        case EcmaOpcode::WIDE_STLEXVAR_PREF_IMM16_IMM16:
            return InferStLexVarDyn(gate);
        case EcmaOpcode::GETITERATOR_IMM8:
        case EcmaOpcode::GETITERATOR_IMM16:
            return InferGetIterator(gate);
        case EcmaOpcode::STMODULEVAR_IMM8:
        case EcmaOpcode::WIDE_STMODULEVAR_PREF_IMM16:
            return InferStModuleVar(gate);
        case EcmaOpcode::LDLOCALMODULEVAR_IMM8:
        case EcmaOpcode::WIDE_LDLOCALMODULEVAR_PREF_IMM16:
            return InferLdLocalModuleVar(gate);
        default:
            break;
    }
    return false;
}

bool TypeInfer::InferPhiGate(GateRef gate)
{
    ASSERT(gateAccessor_.GetOpCode(gate) == OpCode::VALUE_SELECTOR);
    CVector<GlobalTSTypeRef> typeList;
    std::set<GlobalTSTypeRef> numberTypeSet;
    auto ins = gateAccessor_.ConstIns(gate);
    for (auto it =  ins.begin(); it != ins.end(); it++) {
        // assuming that VALUE_SELECTOR is NO_DEPEND and NO_ROOT
        if (gateAccessor_.GetOpCode(*it) == OpCode::MERGE) {
            continue;
        }
        if (gateAccessor_.GetOpCode(*it) == OpCode::LOOP_BEGIN) {
            return InferLoopBeginPhiGate(gate);
        }
        auto valueInType = gateAccessor_.GetGateType(*it);
        if (valueInType.IsAnyType()) {
            return UpdateType(gate, valueInType);
        }
        if (valueInType.IsNumberType()) {
            numberTypeSet.insert(valueInType.GetGTRef());
        } else {
            typeList.emplace_back(valueInType.GetGTRef());
        }
    }
    // deduplicate
    std::sort(typeList.begin(), typeList.end());
    auto deduplicateIndex = std::unique(typeList.begin(), typeList.end());
    typeList.erase(deduplicateIndex, typeList.end());
    if (numberTypeSet.size() == 1) {
        typeList.emplace_back(*(numberTypeSet.begin()));
    } else if (numberTypeSet.size() > 1) {
        typeList.emplace_back(GateType::NumberType().GetGTRef());
    }
    if (typeList.size() > 1) {
        auto unionType = tsManager_->GetOrCreateUnionType(typeList);
        return UpdateType(gate, unionType);
    }
    auto type = typeList.at(0);
    return UpdateType(gate, type);
}

bool TypeInfer::SetIntType(GateRef gate)
{
    auto intType = GateType::IntType();
    return UpdateType(gate, intType);
}

bool TypeInfer::SetNumberType(GateRef gate)
{
    auto numberType = GateType::NumberType();
    return UpdateType(gate, numberType);
}

bool TypeInfer::SetBigIntType(GateRef gate)
{
    auto bigIntType = GateType::BigIntType();
    return UpdateType(gate, bigIntType);
}

bool TypeInfer::SetBooleanType(GateRef gate)
{
    auto booleanType = GateType::BooleanType();
    return UpdateType(gate, booleanType);
}

bool TypeInfer::InferLdUndefined(GateRef gate)
{
    auto undefinedType = GateType::UndefinedType();
    return UpdateType(gate, undefinedType);
}

bool TypeInfer::InferLdNull(GateRef gate)
{
    auto nullType = GateType::NullType();
    return UpdateType(gate, nullType);
}

bool TypeInfer::InferLdai(GateRef gate)
{
    auto intType = GateType::IntType();
    return UpdateType(gate, intType);
}

bool TypeInfer::InferFLdai(GateRef gate)
{
    auto doubleType = GateType::DoubleType();
    return UpdateType(gate, doubleType);
}

bool TypeInfer::InferLdSymbol(GateRef gate)
{
    auto symbolType = GateType::SymbolType();
    return UpdateType(gate, symbolType);
}

bool TypeInfer::InferThrow(GateRef gate)
{
    ASSERT(gateAccessor_.GetNumValueIn(gate) == 1);
    auto gateType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 0));
    return UpdateType(gate, gateType);
}

bool TypeInfer::InferTypeOf(GateRef gate)
{
    ASSERT(gateAccessor_.GetNumValueIn(gate) == 1);
    auto gateType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 0));
    return UpdateType(gate, gateType);
}

/*
 * Type Infer rule(satisfy commutative law):
 * number + number = number
 * int    + number = number
 * double + number = double
 * int    + int    = int
 * int    + double = double
 * double + double = double
 * string + string = string
 */
bool TypeInfer::InferAdd2(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(gateAccessor_.GetNumValueIn(gate) == 2);
    auto firInType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 0));
    auto secInType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 1));
    if (firInType.IsStringType() || secInType.IsStringType()) {
        return UpdateType(gate, GateType::StringType());
    }
    if ((firInType.IsNumberType() && secInType.IsDoubleType()) ||
        (firInType.IsDoubleType() && secInType.IsNumberType())) {
        return UpdateType(gate, GateType::DoubleType());
    }
    if ((firInType.IsIntType() && secInType.IsIntType())) {
        return UpdateType(gate, GateType::IntType());
    }
    if (firInType.IsNumberType() && secInType.IsNumberType()) {
        return UpdateType(gate, GateType::NumberType());
    }
    return UpdateType(gate, GateType::AnyType());
}

/*
 * Type Infer rule(satisfy commutative law):
 * number - number = number
 * int    - number = number
 * double - number = double
 * int    - int    = int
 * int    - double = double
 * double - double = double
 */
bool TypeInfer::InferSub2(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(gateAccessor_.GetNumValueIn(gate) == 2);
    auto firInType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 0));
    auto secInType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 1));
    if ((firInType.IsNumberType() && secInType.IsDoubleType()) ||
        (firInType.IsDoubleType() && secInType.IsNumberType())) {
        return UpdateType(gate, GateType::DoubleType());
    }
    if ((firInType.IsIntType() && secInType.IsIntType())) {
        return UpdateType(gate, GateType::IntType());
    }
    return UpdateType(gate, GateType::NumberType());
}

/*
 * Type Infer rule(satisfy commutative law):
 * number * number = number
 * int    * number = number
 * double * number = double
 * int    * int    = int
 * int    * double = double
 * double * double = double
 */
bool TypeInfer::InferMul2(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(gateAccessor_.GetNumValueIn(gate) == 2);
    auto firInType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 0));
    auto secInType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 1));
    if ((firInType.IsNumberType() && secInType.IsDoubleType()) ||
        (firInType.IsDoubleType() && secInType.IsNumberType())) {
        return UpdateType(gate, GateType::DoubleType());
    }
    if ((firInType.IsIntType() && secInType.IsIntType())) {
        return UpdateType(gate, GateType::IntType());
    }
    return UpdateType(gate, GateType::NumberType());
}

/*
 * Type Infer rule(satisfy commutative law):
 * in type lowering, both elements will be changed to float64 firstly.
 * number / number = double
 * int    / number = double
 * double / number = double
 * int    / int    = double
 * int    / double = double
 * double / double = double
 * any    / any    = number
 * any    / number = number
 * number / any    = number
 */
bool TypeInfer::InferDiv2(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(gateAccessor_.GetNumValueIn(gate) == 2);
    auto firInType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 0));
    auto secInType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 1));
    if (firInType.IsNumberType() && secInType.IsNumberType()) {
        return UpdateType(gate, GateType::DoubleType());
    }
    return UpdateType(gate, GateType::NumberType());
}

/*
 * Type Infer rule:
 * number++ = number
 * number-- = number
 * int++    = int
 * int--    = int
 * double++ = double
 * double-- = double
 */
bool TypeInfer::InferIncDec(GateRef gate)
{
    ASSERT(gateAccessor_.GetNumValueIn(gate) == 1);
    auto firInType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 0));
    if (firInType.IsDoubleType()) {
        return UpdateType(gate, GateType::DoubleType());
    }
    if (firInType.IsIntType()) {
        return UpdateType(gate, GateType::IntType());
    }
    return UpdateType(gate, GateType::NumberType());
}

bool TypeInfer::InferLdObjByIndex(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(gateAccessor_.GetNumValueIn(gate) == 2);
    auto inValueType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 1));
    if (tsManager_->IsArrayTypeKind(inValueType)) {
        auto type = tsManager_->GetArrayParameterTypeGT(inValueType);
        return UpdateType(gate, type);
    }

    if (tsManager_->IsTypedArrayType(inValueType)) {
        return UpdateType(gate, GateType::NumberType());
    }

    if (ShouldInferWithLdObjByValue(inValueType)) {
        auto key = gateAccessor_.GetConstantValue((gateAccessor_.GetValueIn(gate, 0)));
        auto type = GetPropType(inValueType, key);
        return UpdateType(gate, type);
    }
    return false;
}

bool TypeInfer::SetStGlobalBcType(GateRef gate, bool hasIC)
{
    auto &byteCodeInfo = GetByteCodeInfo(gate);
    uint16_t stringId = 0;
    GateType inValueType;
    if (hasIC) {
        // 2: number of value inputs
        ASSERT(byteCodeInfo.inputs.size() == 2);
        stringId = std::get<ConstDataId>(byteCodeInfo.inputs[1]).GetId();
        // 3: number of value inputs
        ASSERT(gateAccessor_.GetNumValueIn(gate) == 3);
        // 2: value input
        inValueType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 2));
    } else {
        ASSERT(byteCodeInfo.inputs.size() == 1);
        stringId = std::get<ConstDataId>(byteCodeInfo.inputs[0]).GetId();
        // 2: number of value inputs
        ASSERT(gateAccessor_.GetNumValueIn(gate) == 2);
        inValueType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 1));
    }
    if (stringIdToGateType_.find(stringId) != stringIdToGateType_.end()) {
        stringIdToGateType_[stringId] = inValueType;
    } else {
        stringIdToGateType_.emplace(stringId, inValueType);
    }
    return UpdateType(gate, inValueType);
}

bool TypeInfer::InferLdGlobalVar(GateRef gate)
{
    auto &byteCodeInfo = GetByteCodeInfo(gate);
    ASSERT(byteCodeInfo.inputs.size() == 2);  // 2: number of value inputs
    auto stringId = std::get<ConstDataId>(byteCodeInfo.inputs[1]).GetId();
    auto iter = stringIdToGateType_.find(stringId);
    if (iter != stringIdToGateType_.end()) {
        return UpdateType(gate, iter->second);
    }
    return false;
}

bool TypeInfer::InferReturnUndefined(GateRef gate)
{
    auto undefinedType = GateType::UndefinedType();
    return UpdateType(gate, undefinedType);
}

bool TypeInfer::InferReturn(GateRef gate)
{
    ASSERT(gateAccessor_.GetNumValueIn(gate) == 1);
    auto gateType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 0));
    return UpdateType(gate, gateType);
}

bool TypeInfer::InferLdObjByName(GateRef gate)
{
    // 3: number of value inputs
    ASSERT(gateAccessor_.GetNumValueIn(gate) == 3);
    auto objType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 2));  // 2: the third parameter is receiver
    if (objType.IsAnyType()) {
        return false;
    }

    if (ShouldConvertToBuiltinArray(objType)) {
        GlobalTSTypeRef builtinGt = ConvertPrimitiveToBuiltin(objType);
        auto builtinInstanceType = tsManager_->CreateClassInstanceType(builtinGt);
        objType = GateType(builtinInstanceType);
    }
    if (tsManager_->IsPrimitiveTypeKind(objType)) {
        GlobalTSTypeRef builtinGt = ConvertPrimitiveToBuiltin(objType);
        if (builtinGt.GetModuleId() == TSModuleTable::BUILTINS_TABLE_ID) {
            auto builtinInstanceType = tsManager_->CreateClassInstanceType(builtinGt);
            objType = GateType(builtinInstanceType);
        }
    }
    // If this object has no gt type, we cannot get its internal property type
    if (ShouldInferWithLdObjByName(objType)) {
        uint16_t index = gateAccessor_.GetConstDataId(gateAccessor_.GetValueIn(gate, 1)).GetId();
        return GetObjPropWithName(gate, objType, index);
    }
    return false;
}

bool TypeInfer::InferNewObject(GateRef gate)
{
    auto objType = gateAccessor_.GetGateType(gate);
    if (!tsManager_->IsClassInstanceTypeKind(objType) && !tsManager_->IsArrayTypeKind(objType)) {
        ASSERT(gateAccessor_.GetNumValueIn(gate) > 0);
        auto classType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 0));
        if (tsManager_->IsClassTypeKind(classType)) {
            auto classInstanceType = tsManager_->CreateClassInstanceType(classType);
            return UpdateType(gate, classInstanceType);
        }
    }
    return false;
}

bool TypeInfer::InferLdStr(GateRef gate)
{
    auto stringType = GateType::StringType();
    return UpdateType(gate, stringType);
}

bool TypeInfer::GetObjPropWithName(GateRef gate, GateType objType, uint64_t index)
{
    auto thread = tsManager_->GetEcmaVM()->GetJSThread();
    JSHandle<ConstantPool> constantPool(tsManager_->GetConstantPool());
    JSTaggedValue name = ConstantPool::GetStringFromCache(thread, constantPool.GetTaggedValue(), index);
    if (tsManager_->IsBuiltinArrayType(objType) || tsManager_->IsTypedArrayType(objType)) {
        JSTaggedValue lengthKey = thread->GlobalConstants()->GetLengthString();
        if (JSTaggedValue::SameValue(name, lengthKey)) {
            return SetIntType(gate);
        }
    }
    auto type = GetPropType(objType, name);
    if (tsManager_->IsGetterSetterFunc(type)) {
        auto returnGt = tsManager_->GetFuncReturnValueTypeGT(type);
        return UpdateType(gate, returnGt);
    }
    return UpdateType(gate, type);
}

bool TypeInfer::InferCallFunction(GateRef gate)
{
    // 1: last one elem is function
    size_t funcIndex = gateAccessor_.GetNumValueIn(gate) - 1;
    auto funcType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, funcIndex));
    if (tsManager_->IsFunctionTypeKind(funcType)) {
        auto returnType = tsManager_->GetFuncReturnValueTypeGT(funcType);
        return UpdateType(gate, returnType);
    }

    if (tsManager_->IsIteratorInstanceTypeKind(funcType)) {
        GlobalTSTypeRef elementGT = tsManager_->GetIteratorInstanceElementGt(funcType);
        GlobalTSTypeRef iteratorResultInstanceType = tsManager_->GetOrCreateTSIteratorInstanceType(
            TSRuntimeType::ITERATOR_RESULT, elementGT);
        return UpdateType(gate, iteratorResultInstanceType);
    }
    /* According to the ECMAScript specification, user-defined classes can only be instantiated by constructing (with
     * new keyword). However, a few builtin types can be called like a function. Upon the results of calling and
     * constructing, there are 4 categories of builtin types:
     *
     * Category 1: non-callable, objects of such a type can only be created by constructing.
     * Category 2: non-constructable, such types can only be called.
     * Category 3: simple, calling and constructing are equivalent.
     * Category 4: complex, a type can be called and constructed, but the results differ.
     *
     * Constructing a builtin type always create objects of the type if supported. So in this function, we focus on the
     * builtin types which are callable. While the majority of the callable builtin types have the same calling behavior
     * as constructing, here are some special cases:
     *
     * | Type    | Call              | Category |
     * | ------- | ----------------- | -------- |
     * | BigInt  | primitive bigint  | 2        |
     * | Boolean | primitive boolean | 4        |
     * | Date    | primitive string  | 4        |
     * | Number  | primitive number  | 4        |
     * | String  | primitive string  | 4        |
     *
     * See the list of builtin types' constructors at:
     *     https://tc39.es/ecma262/2021/#sec-constructor-properties-of-the-global-object
     */
    if (tsManager_->IsClassTypeKind(funcType) && tsManager_->IsBuiltin(funcType)) {
        // For simplicity, calling and constructing are considered equivalent.
        return UpdateType(gate, tsManager_->CreateClassInstanceType(funcType));
    }
    return false;
}

bool TypeInfer::InferLdObjByValue(GateRef gate)
{
    auto objType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 1));
    // handle array
    if (tsManager_->IsArrayTypeKind(objType)) {
        auto elementType = tsManager_->GetArrayParameterTypeGT(objType);
        return UpdateType(gate, elementType);
    }
    // handle object
    if (ShouldInferWithLdObjByValue(objType)) {
        auto valueGate = gateAccessor_.GetValueIn(gate, 2);  // 2: value input slot
        if (gateAccessor_.GetOpCode(valueGate) == OpCode::CONSTANT) {
            auto value = gateAccessor_.GetConstantValue(valueGate);
            auto type = GetPropType(objType, value);
            return UpdateType(gate, type);
        }
        if (IsByteCodeGate(valueGate) && GetByteCodeInfo(valueGate).IsBc(EcmaOpcode::LDA_STR_ID16)) {
            ConstDataId dataId = gateAccessor_.GetConstDataId(valueGate);
            auto index = dataId.GetId();
            return GetObjPropWithName(gate, objType, index);
        }
    }
    return false;
}

bool TypeInfer::InferGetNextPropName(GateRef gate)
{
    auto stringType = GateType::StringType();
    return UpdateType(gate, stringType);
}

bool TypeInfer::InferDefineGetterSetterByValue(GateRef gate)
{
    // 0 : the index of obj
    auto objType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 0));
    return UpdateType(gate, objType);
}

bool TypeInfer::InferSuperCall(GateRef gate)
{
    ArgumentAccessor argAcc(circuit_);
    auto newTarget = argAcc.GetCommonArgGate(CommonArgIdx::NEW_TARGET);
    auto classType = gateAccessor_.GetGateType(newTarget);
    if (tsManager_->IsClassTypeKind(classType)) {
        auto classInstanceType = tsManager_->CreateClassInstanceType(classType);
        return UpdateType(gate, classInstanceType);
    }
    return false;
}

bool TypeInfer::InferSuperPropertyByName(GateRef gate)
{
    uint16_t index = gateAccessor_.GetConstDataId(gateAccessor_.GetValueIn(gate, 0)).GetId();
    return GetSuperProp(gate, index);
}

bool TypeInfer::InferSuperPropertyByValue(GateRef gate)
{
    auto valueGate = gateAccessor_.GetValueIn(gate, 1);
    if (IsByteCodeGate(valueGate) && GetByteCodeInfo(valueGate).IsBc(EcmaOpcode::LDA_STR_ID16)) {
        ConstDataId dataId = gateAccessor_.GetConstDataId(valueGate);
        auto index = dataId.GetId();
        return GetSuperProp(gate, index);
    }
    if (gateAccessor_.GetOpCode(valueGate) == OpCode::CONSTANT) {
        auto index = gateAccessor_.GetConstantValue(valueGate);

        return GetSuperProp(gate, index, false);
    }
    return false;
}

bool TypeInfer::GetSuperProp(GateRef gate, uint64_t index, bool isString)
{
    ArgumentAccessor argAcc(circuit_);
    auto func = argAcc.GetCommonArgGate(CommonArgIdx::FUNC);
    auto newTarget = argAcc.GetCommonArgGate(CommonArgIdx::NEW_TARGET);
    auto funcType = gateAccessor_.GetGateType(func);
    auto classType = gateAccessor_.GetGateType(newTarget);
    if (!funcType.IsAnyType() && !classType.IsAnyType()) {
        auto thread = tsManager_->GetEcmaVM()->GetJSThread();
        GlobalTSTypeRef type = GlobalTSTypeRef::Default();
        bool isStatic = tsManager_->IsStaticFunc(funcType.GetGTRef());
        auto propType = isStatic ? PropertyType::STATIC : PropertyType::NORMAL;
        JSHandle<ConstantPool> constantPool(tsManager_->GetConstantPool());
        type = isString ? tsManager_->GetSuperPropType(classType.GetGTRef(),
            ConstantPool::GetStringFromCache(thread, constantPool.GetTaggedValue(), index), propType) :
            tsManager_->GetSuperPropType(classType.GetGTRef(), index, propType);
        if (tsManager_->IsGetterSetterFunc(type)) {
            auto returnGt = tsManager_->GetFuncReturnValueTypeGT(type);
            return UpdateType(gate, returnGt);
        }
        return UpdateType(gate, type);
    }
    return false;
}

bool TypeInfer::InferGetIterator(GateRef gate)
{
    ASSERT(gateAccessor_.GetNumValueIn(gate) == 1);
    GateType inValueType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 0));

    GlobalTSTypeRef elementGt = GlobalTSTypeRef::Default();
    if (tsManager_->IsArrayTypeKind(inValueType)) {
        elementGt = tsManager_->GetArrayParameterTypeGT(inValueType);
    } else if (inValueType.IsStringType()) {
        elementGt.SetType(GateType::StringType().Value());
    } else {
        return false;
    }
    GlobalTSTypeRef iteratorInstanceType = tsManager_->GetOrCreateTSIteratorInstanceType(
        TSRuntimeType::ITERATOR, elementGt);
    return UpdateType(gate, iteratorInstanceType);
}

bool TypeInfer::InferTryLdGlobalByName(GateRef gate)
{
    // todo by hongtao, should consider function of .d.ts
    auto &byteCodeInfo = GetByteCodeInfo(gate);
    ASSERT(byteCodeInfo.inputs.size() == 2);  // 2: number of parameter
    auto stringId = std::get<ConstDataId>(byteCodeInfo.inputs[1]).GetId();
    auto iter = stringIdToGateType_.find(stringId);
    if (iter != stringIdToGateType_.end()) {
        return UpdateType(gate, iter->second);
    }
    return false;
}

bool TypeInfer::InferLdLexVarDyn(GateRef gate)
{
    auto level = gateAccessor_.GetConstantValue(gateAccessor_.GetValueIn(gate, 0));
    auto slot = gateAccessor_.GetConstantValue(gateAccessor_.GetValueIn(gate, 1));
    auto type = lexEnvManager_->GetLexEnvElementType(methodId_, level, slot);
    return UpdateType(gate, type);
}

bool TypeInfer::InferStLexVarDyn(GateRef gate)
{
    auto level = gateAccessor_.GetConstantValue(gateAccessor_.GetValueIn(gate, 0));
    auto slot = gateAccessor_.GetConstantValue(gateAccessor_.GetValueIn(gate, 1));
    auto type = lexEnvManager_->GetLexEnvElementType(methodId_, level, slot);
    if (type.IsAnyType() || type.IsUndefinedType()) {
        auto valueType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 3));
        if (!valueType.IsAnyType()) {
            lexEnvManager_->SetLexEnvElementType(methodId_, level, slot, valueType);
            return true;
        }
    }
    return false;
}

bool TypeInfer::InferStModuleVar(GateRef gate)
{
    auto index = gateAccessor_.GetConstantValue(gateAccessor_.GetValueIn(gate, 0));
    const JSPandaFile *jsPandaFile = builder_->GetJSPandaFile();
    auto defineGate = gateAccessor_.GetValueIn(gate, 1);
    auto defineType = gateAccessor_.GetGateType(defineGate);
    if (!defineType.IsAnyType()) {
        tsManager_->AddTypeToLocalModuleVarGtMap(jsPandaFile, recordName_, index, defineType.GetGTRef());
        return true;
    }
    return false;
}

bool TypeInfer::InferLdLocalModuleVar(GateRef gate)
{
    auto index = gateAccessor_.GetConstantValue(gateAccessor_.GetValueIn(gate, 0));
    const JSPandaFile *jsPandaFile = builder_->GetJSPandaFile();
    if (!tsManager_->HasExportGT(jsPandaFile, recordName_, index)) {
        return UpdateType(gate, GateType::AnyType());
    }
    auto type = tsManager_->GetGTFromModuleMap(jsPandaFile, recordName_, index);
    return UpdateType(gate, type);
}

bool TypeInfer::InferLoopBeginPhiGate(GateRef gate)
{
    // loop-begin phi gate has 3 ins: loop_begin(stateWire), loopInGate(valueWire), loopBackGate(valueWire)
    auto loopInGate = gateAccessor_.GetValueIn(gate);
    auto loopInType = gateAccessor_.GetGateType(loopInGate);
    // loop-begin phi will be initialized as loopInTytpe
    // type of loop-back phi should be infered correctly only after loop-begin has actual type
    // if loop-in phi is actual any type, loop-begin phi must be any
    if (loopPhiState_.find(gate) == loopPhiState_.end()) {
        if (!loopInType.IsAnyType()) {
            loopPhiState_[gate] = InferState::NORMAL_INFERED;
        }
        return UpdateType(gate, loopInType);
    }
    // if loop phi has been marked as ANY_INFERED, it's in the second round infer for loop
    if (loopPhiState_[gate] == InferState::ANY_INFERED) {
        return UpdateType(gate, GateType::AnyType());
    }
    // if loopInType and loopBackType both have non-any type, we need special treatment for the situation
    // in which loopInType and loopBackType both are numberType(int/double/number).
    // However, we should avoid excessive type promotion which may cause endless loop in few IR situations.
    if (loopPhiState_[gate] == InferState::NUMBER_INFERED) {
        return UpdateType(gate, GateType::NumberType());
    }
    return UpdateType(gate, loopInType);
}

GlobalTSTypeRef TypeInfer::ConvertPrimitiveToBuiltin(const GateType &gateType)
{
    GlobalTSTypeRef builtinGt = GlobalTSTypeRef::Default();
    if (!tsManager_->IsBuiltinsDTSEnabled()) {
        return builtinGt;
    }

    const JSPandaFile *builtinjsPandaFile = tsManager_->GetBuiltinPandaFile();
    if (builtinjsPandaFile == nullptr) {
        LOG_COMPILER(FATAL) << "load lib_ark_builtins.d.ts failed";
    }
    const CString &builtinsRecordName = tsManager_->GetBuiltinRecordName();
    TSTypeParser typeParser(tsManager_);

    if (tsManager_->IsArrayTypeKind(gateType)) {
        return typeParser.CreateGT(builtinjsPandaFile, builtinsRecordName,
                                   static_cast<uint32_t>(BuiltinTypeId::ARRAY));
    }

    const GlobalTSTypeRef gt = GlobalTSTypeRef(gateType.Value());
    uint32_t l = gt.GetLocalId();
    switch (l) {
        case static_cast<uint32_t>(TSPrimitiveType::SYMBOL):
            builtinGt = typeParser.CreateGT(builtinjsPandaFile, builtinsRecordName,
                                            static_cast<uint32_t>(BuiltinTypeId::SYMBOL));
            break;
        case static_cast<uint32_t>(TSPrimitiveType::INT):
        case static_cast<uint32_t>(TSPrimitiveType::DOUBLE):
        case static_cast<uint32_t>(TSPrimitiveType::NUMBER):
            builtinGt = typeParser.CreateGT(builtinjsPandaFile, builtinsRecordName,
                                            static_cast<uint32_t>(BuiltinTypeId::NUMBER));
            break;
        case static_cast<uint32_t>(TSPrimitiveType::BOOLEAN):
            builtinGt = typeParser.CreateGT(builtinjsPandaFile, builtinsRecordName,
                                            static_cast<uint32_t>(BuiltinTypeId::BOOLEAN));
            break;
        case static_cast<uint32_t>(TSPrimitiveType::STRING):
            builtinGt = typeParser.CreateGT(builtinjsPandaFile, builtinsRecordName,
                                            static_cast<uint32_t>(BuiltinTypeId::STRING));
            break;
        default:
            builtinGt = GlobalTSTypeRef::Default();
    }
    return builtinGt;
}

void TypeInfer::PrintAllByteCodesTypes() const
{
    std::vector<GateRef> gateList;
    circuit_->GetAllGates(gateList);

    const JSPandaFile *jsPandaFile = builder_->GetJSPandaFile();
    const MethodLiteral *methodLiteral = builder_->GetMethod();
    const std::string functionName = methodLiteral->ParseFunctionName(jsPandaFile, methodLiteral->GetMethodId());

    LOG_COMPILER(INFO) << "print bytecode types:";
    LOG_COMPILER(INFO) << ".recordName " + recordName_;
    LOG_COMPILER(INFO) << ".function " + functionName + "() {";
    uint32_t lastBcIndex = builder_->GetLastBcIndex();
    for (uint32_t bcIndex = 0; bcIndex < lastBcIndex; bcIndex++) {  // ignore last element
        const uint8_t *pc = builder_->GetPCByIndex(bcIndex);
        BytecodeInstruction inst(pc);
        GateRef gate = builder_->GetGateByBcIndex(bcIndex);
        if (gate != Circuit::NullGate()) {
            GateType type = gateAccessor_.GetGateType(gate);
            if (!tsManager_->IsPrimitiveTypeKind(type)) {
                GlobalTSTypeRef gt = type.GetGTRef();
                LOG_COMPILER(INFO) << "    " << inst << ", type: " + tsManager_->GetTypeStr(type)
                                   << ", [moduleId: " + std::to_string(gt.GetModuleId())
                                   << ", localId: " + std::to_string(gt.GetLocalId()) + "]";
            } else {
                LOG_COMPILER(INFO) << "    " << inst << ", type: " + tsManager_->GetTypeStr(type);
            }
        }
    }
    LOG_COMPILER(INFO) << "}";
}

void TypeInfer::Verify() const
{
    std::vector<GateRef> gateList;
    circuit_->GetAllGates(gateList);
    for (const auto &gate : gateList) {
        auto op = gateAccessor_.GetOpCode(gate);
        if (op == OpCode::JS_BYTECODE) {
            TypeCheck(gate);
        }
    }
}

/*
 * Let v be a variable in one ts-file and t be a type. To check whether the type of v is t after
 * type inferenece, one should declare a function named "AssertType(value:any, type:string):void"
 * in ts-file and call it with arguments v and t, where t is the expected type string.
 * The following interface performs such a check at compile time.
 */
void TypeInfer::TypeCheck(GateRef gate) const
{
    auto &info = GetByteCodeInfo(gate);
    if (!info.IsBc(EcmaOpcode::CALLARGS2_IMM8_V8_V8)) {
        return;
    }
    auto func = gateAccessor_.GetValueIn(gate, 2); // 2: acc
    auto &funcInfo = GetByteCodeInfo(func);
    if (!funcInfo.IsBc(EcmaOpcode::TRYLDGLOBALBYNAME_IMM8_ID16) &&
        !funcInfo.IsBc(EcmaOpcode::TRYLDGLOBALBYNAME_IMM16_ID16)) {
        return;
    }
    auto funcName = gateAccessor_.GetValueIn(func, 1);
    auto thread = tsManager_->GetEcmaVM()->GetJSThread();
    JSHandle<ConstantPool> constantPool(tsManager_->GetConstantPool());
    uint16_t funcNameStrId = gateAccessor_.GetConstDataId(funcName).GetId();
    ConstantPool::GetStringFromCache(thread, constantPool.GetTaggedValue(), funcNameStrId);
    auto funcNameString = constantPool->GetStdStringByIdx(funcNameStrId);
    if (funcNameString == "AssertType") {
        GateRef expectedGate = gateAccessor_.GetValueIn(gate, 1);
        uint16_t strId = gateAccessor_.GetConstDataId(expectedGate).GetId();
        ConstantPool::GetStringFromCache(thread, constantPool.GetTaggedValue(), strId);
        auto expectedTypeStr = constantPool->GetStdStringByIdx(strId);
        GateRef valueGate = gateAccessor_.GetValueIn(gate, 0);
        auto type = gateAccessor_.GetGateType(valueGate);
        if (expectedTypeStr != tsManager_->GetTypeStr(type)) {
            const JSPandaFile *jsPandaFile = builder_->GetJSPandaFile();
            const MethodLiteral *methodLiteral = builder_->GetMethod();
            EntityId methodId = builder_->GetMethod()->GetMethodId();
            DebugInfoExtractor *debugExtractor = JSPandaFileManager::GetInstance()->GetJSPtExtractor(jsPandaFile);
            const std::string &sourceFileName = debugExtractor->GetSourceFile(methodId);
            const std::string functionName = methodLiteral->ParseFunctionName(jsPandaFile, methodId);

            std::string log = CollectGateTypeLogInfo(valueGate, debugExtractor, "[TypeAssertion] ");
            log += "[TypeAssertion] but expected type: " + expectedTypeStr + "\n";

            LOG_COMPILER(ERROR) << "[TypeAssertion] [" << sourceFileName << ":" << functionName << "] begin:";
            LOG_COMPILER(FATAL) << log << "[compiler] [TypeAssertion] end";
        }
    }
}

void TypeInfer::FilterAnyTypeGates() const
{
    const JSPandaFile *jsPandaFile = builder_->GetJSPandaFile();
    const MethodLiteral *methodLiteral = builder_->GetMethod();
    EntityId methodId = methodLiteral->GetMethodId();

    DebugInfoExtractor *debugExtractor = JSPandaFileManager::GetInstance()->GetJSPtExtractor(jsPandaFile);
    const std::string &sourceFileName = debugExtractor->GetSourceFile(methodId);
    const std::string functionName = methodLiteral->ParseFunctionName(jsPandaFile, methodId);

    std::vector<GateRef> gateList;
    circuit_->GetAllGates(gateList);
    std::string log;
    for (const auto &gate : gateList) {
        GateType type = gateAccessor_.GetGateType(gate);
        if (ShouldInfer(gate) && type.IsAnyType()) {
            log += CollectGateTypeLogInfo(gate, debugExtractor, "[TypeFilter] ");
        }
    }

    LOG_COMPILER(INFO) << "[TypeFilter] [" << sourceFileName << ":" << functionName << "] begin:";
    LOG_COMPILER(INFO) << log << "[TypeFilter] end";
}

std::string TypeInfer::CollectGateTypeLogInfo(GateRef gate, DebugInfoExtractor *debugExtractor,
                                              const std::string &logPreFix) const
{
    std::string log(logPreFix);
    log += "gate id: "+ std::to_string(gateAccessor_.GetId(gate)) + ", ";
    OpCode op = gateAccessor_.GetOpCode(gate);
    log += "op: " + GateMetaData::Str(op) + ", ";
    if (op == OpCode::ARG) {
        log += "arg gate, ";
    } else if (op != OpCode::VALUE_SELECTOR) {
        auto &bytecodeInfo = GetByteCodeInfo(gate);
        // handle ByteCode gate: print gate id, bytecode and line number in source code.
        log += "bytecode: " + GetEcmaOpcodeStr(bytecodeInfo.GetOpcode()) + ", ";

        int32_t lineNumber = 0;
        auto callbackLineFunc = [&lineNumber](int32_t line) -> bool {
            lineNumber = line + 1;
            return true;
        };

        const auto bcIndex = jsgateToBytecode_.at(gate);
        auto offset = builder_->GetPcOffset(bcIndex);
        const MethodLiteral *methodLiteral = builder_->GetMethod();
        debugExtractor->MatchLineWithOffset(callbackLineFunc, methodLiteral->GetMethodId(), offset);

        log += "at line: " + std::to_string(lineNumber) + ", ";
    } else {
    // handle phi gate: print gate id and input gates id list.
        log += "phi gate, ins: ";
        auto ins = gateAccessor_.ConstIns(gate);
        for (auto it =  ins.begin(); it != ins.end(); it++) {
            log += std::to_string(gateAccessor_.GetId(*it)) + " ";
        }
    }

    GateType type = gateAccessor_.GetGateType(gate);
    log += "type: " + tsManager_->GetTypeStr(type) + ", ";
    if (!tsManager_->IsPrimitiveTypeKind(type)) {
        GlobalTSTypeRef gt = type.GetGTRef();
        log += "[moduleId: " + std::to_string(gt.GetModuleId()) + ", ";
        log += "localId: " + std::to_string(gt.GetLocalId()) + "], ";
    }

    log += "\n[compiler] ";
    return log;
}
}  // namespace panda::ecmascript
