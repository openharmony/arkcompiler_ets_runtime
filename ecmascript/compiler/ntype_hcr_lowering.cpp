/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/ntype_hcr_lowering.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/dfx/vmstat/opt_code_profiler.h"

namespace panda::ecmascript::kungfu {

GateRef NTypeHCRLowering::VisitGate(GateRef gate)
{
    auto op = acc_.GetOpCode(gate);
    if (op == OpCode::JS_BYTECODE) {
        Lower(gate);
    }
    return Circuit::NullGate();
}

void NTypeHCRLowering::Lower(GateRef gate)
{
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    // initialize label manager
    Environment env(gate, circuit_, &builder_);
    switch (ecmaOpcode) {
        case EcmaOpcode::CREATEEMPTYARRAY_IMM8:
        case EcmaOpcode::CREATEEMPTYARRAY_IMM16:
            LowerNTypedCreateEmptyArray(gate);
            break;
        case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
        case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM16_ID16:
            LowerNTypedCreateArrayWithBuffer(gate);
            break;
        case EcmaOpcode::STOWNBYINDEX_IMM8_V8_IMM16:
        case EcmaOpcode::STOWNBYINDEX_IMM16_V8_IMM16:
        case EcmaOpcode::WIDE_STOWNBYINDEX_PREF_V8_IMM32:
            LowerNTypedStownByIndex(gate);
            break;
        case EcmaOpcode::STOWNBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STOWNBYNAME_IMM16_ID16_V8:
            LowerNTypedStOwnByName(gate);
            break;
        case EcmaOpcode::THROW_UNDEFINEDIFHOLEWITHNAME_PREF_ID16:
            LowerThrowUndefinedIfHoleWithName(gate);
            break;
        case EcmaOpcode::LDLEXVAR_IMM4_IMM4:
        case EcmaOpcode::LDLEXVAR_IMM8_IMM8:
        case EcmaOpcode::WIDE_LDLEXVAR_PREF_IMM16_IMM16:
            LowerLdLexVar(gate);
            break;
        case EcmaOpcode::STLEXVAR_IMM4_IMM4:
        case EcmaOpcode::STLEXVAR_IMM8_IMM8:
        case EcmaOpcode::WIDE_STLEXVAR_PREF_IMM16_IMM16:
            LowerStLexVar(gate);
            break;
        default:
            break;
    }
}

void NTypeHCRLowering::LowerThrowUndefinedIfHoleWithName(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 1); // 1: the second parameter
    builder_.LexVarIsHoleCheck(value);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), Circuit::NullGate());
}

void NTypeHCRLowering::LowerLdLexVar(GateRef gate)
{
    AddProfiling(gate);
    GateRef level = acc_.GetValueIn(gate, 0); // 0: first parameter
    GateRef index = acc_.GetValueIn(gate, 1); // 1: the second parameter
    GateRef currentEnv = acc_.GetValueIn(gate, 2); // 2: the third parameter

    uint32_t levelValue = static_cast<uint32_t>(acc_.GetConstantValue(level));
    uint32_t indexValue = static_cast<uint32_t>(acc_.GetConstantValue(index));
    indexValue += LexicalEnv::RESERVED_ENV_LENGTH;
    GateRef result = Circuit::NullGate();
    if (levelValue == 0) {
        result = builder_.LoadFromTaggedArray(currentEnv, indexValue);
    } else if (levelValue == 1) { // 1: level 1
        auto parentEnv = builder_.LoadFromTaggedArray(currentEnv, LexicalEnv::PARENT_ENV_INDEX);
        result = builder_.LoadFromTaggedArray(parentEnv, indexValue);
    } else {
        // level > 1, go slowpath
        return;
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void NTypeHCRLowering::LowerStLexVar(GateRef gate)
{
    AddProfiling(gate);
    GateRef level = acc_.GetValueIn(gate, 0); // 0: first parameter
    GateRef index = acc_.GetValueIn(gate, 1); // 1: the second parameter
    GateRef currentEnv = acc_.GetValueIn(gate, 2); // 2: the third parameter
    GateRef value = acc_.GetValueIn(gate, 3); // 3: the fourth parameter

    uint32_t levelValue = static_cast<uint32_t>(acc_.GetConstantValue(level));
    uint32_t indexValue = static_cast<uint32_t>(acc_.GetConstantValue(index));
    indexValue += LexicalEnv::RESERVED_ENV_LENGTH;
    GateRef result = Circuit::NullGate();
    if (levelValue == 0) {
        result = builder_.StoreToTaggedArray(currentEnv, indexValue, value);
    } else if (levelValue == 1) { // 1: level 1
        auto parentEnv = builder_.LoadFromTaggedArray(currentEnv, LexicalEnv::PARENT_ENV_INDEX);
        result = builder_.StoreToTaggedArray(parentEnv, indexValue, value);
    } else {
        // level > 1, go slowpath
        return;
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void NTypeHCRLowering::LowerNTypedCreateEmptyArray(GateRef gate)
{
    // in the future, the type of the elements in the array will be obtained through pgo,
    // and the type will be used to determine whether to create a typed-array.
    AddProfiling(gate);
    auto thread = tsManager_->GetEcmaVM()->GetJSThread();
    uint64_t bcAbsoluteOffset = GetBcAbsoluteOffset(gate);
    ElementsKind kind = acc_.TryGetElementsKind(gate);
    auto hclassIdx = thread->GetArrayHClassIndexMap().at(kind);
    tsManager_->AddArrayTSConstantIndex(bcAbsoluteOffset, JSTaggedValue(static_cast<int64_t>(hclassIdx)));
    GateRef array = builder_.CreateArray(kind, 0);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), array);
}

void NTypeHCRLowering::LowerNTypedCreateArrayWithBuffer(GateRef gate)
{
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef index = acc_.GetValueIn(gate, 0);
    auto thread = tsManager_->GetEcmaVM()->GetJSThread();
    uint64_t bcAbsoluteOffset = GetBcAbsoluteOffset(gate);
    uint32_t cpIdx = static_cast<uint32_t>(acc_.GetConstantValue(index));
    JSHandle<ConstantPool> constpoolHandle(tsManager_->GetConstantPool());
    JSTaggedValue arr = ConstantPool::GetLiteralFromCache<ConstPoolType::ARRAY_LITERAL>(
        thread, constpoolHandle.GetTaggedValue(), cpIdx, recordName_);
    JSHandle<JSArray> arrayHandle(thread, arr);

    ElementsKind kind = acc_.TryGetElementsKind(gate);
    auto hclassIdx = thread->GetArrayHClassIndexMap().at(kind);
    GateType gateType = acc_.GetGateType(gate);
    panda_file::File::EntityId id = ConstantPool::GetIdFromCache(constpoolHandle.GetTaggedValue(), cpIdx);
    tsManager_->AddArrayTSConstantIndex(bcAbsoluteOffset, JSTaggedValue(static_cast<int64_t>(hclassIdx)));
    tsManager_->AddArrayTSElements(id, arrayHandle->GetElements());
    tsManager_->AddArrayTSElementsKind(id, JSTaggedValue(static_cast<int64_t>(kind)));
    gateType = tsManager_->TryNarrowUnionType(gateType);

    int elementIndex = -1;
    if (tsManager_->IsArrayTypeKind(gateType)) {
        elementIndex = tsManager_->GetElementsIndexByArrayType(gateType, id);
    }
    if (elementIndex == -1) { // slowpath
        return;
    }

    AddProfiling(gate);
    GateRef elementIndexGate = builder_.IntPtr(elementIndex);
    GateRef array = builder_.CreateArrayWithBuffer(kind, ArrayMetaDataAccessor::Mode::CREATE, index, elementIndexGate);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), array);
}

void NTypeHCRLowering::LowerNTypedStownByIndex(GateRef gate)
{
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef value = acc_.GetValueIn(gate, 2);
    if (acc_.GetOpCode(receiver) != OpCode::CREATE_ARRAY &&
        acc_.GetOpCode(receiver) != OpCode::CREATE_ARRAY_WITH_BUFFER) {
        return;
    }
    builder_.COWArrayCheck(receiver);

    AddProfiling(gate);
    uint32_t indexValue = static_cast<uint32_t>(acc_.GetConstantValue(index));
    uint32_t arraySize = acc_.GetArraySize(receiver);
    if (indexValue > arraySize) {
        acc_.TrySetElementsKind(receiver, ElementsKind::HOLE);
    }
    acc_.SetArraySize(receiver, std::max(arraySize, indexValue + 1));
    index = builder_.Int32(indexValue);
    builder_.StoreElement<TypedStoreOp::ARRAY_STORE_ELEMENT>(receiver, index, value);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), Circuit::NullGate());
}

void NTypeHCRLowering::LowerNTypedStOwnByName(GateRef gate)
{
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    auto constData = acc_.GetValueIn(gate, 0);
    uint16_t propIndex = acc_.GetConstantValue(constData);
    auto thread = tsManager_->GetEcmaVM()->GetJSThread();
    auto propKey = tsManager_->GetStringFromConstantPool(propIndex);

    GateRef receiver = acc_.GetValueIn(gate, 1);
    GateRef value = acc_.GetValueIn(gate, 2);

    GateType receiverType = acc_.GetGateType(receiver);
    receiverType = tsManager_->TryNarrowUnionType(receiverType);

    int hclassIndex = -1;
    if (tsManager_->IsObjectTypeKind(receiverType)) {
        hclassIndex = tsManager_->GetHClassIndexByObjectType(receiverType);
    }
    if (hclassIndex == -1) { // slowpath
        return;
    }
    JSHClass *hclass = JSHClass::Cast(tsManager_->GetValueFromCache(hclassIndex).GetTaggedObject());

    PropertyLookupResult plr = JSHClass::LookupPropertyInAotHClass(thread, hclass, propKey);
    if (!plr.IsFound() || !plr.IsLocal() || plr.IsAccessor() || !plr.IsWritable()) {  // slowpath
        return;
    }
    AddProfiling(gate);

    GateRef pfrGate = builder_.Int32(plr.GetData());
    builder_.StoreProperty(receiver, pfrGate, value);

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), Circuit::NullGate());
}

uint64_t NTypeHCRLowering::GetBcAbsoluteOffset(GateRef gate) const
{
    uint64_t pcOffset = acc_.TryGetPcOffset(gate);
    uint64_t pfOffset = reinterpret_cast<uint64_t>(jsPandaFile_->GetHeader());
    uint64_t methodOffset = reinterpret_cast<uint64_t>(methodLiteral_->GetBytecodeArray());
    uint64_t bcAbsoluteOffset = methodOffset - pfOffset + pcOffset;
    return bcAbsoluteOffset;
}

void NTypeHCRLowering::AddProfiling(GateRef gate)
{
    if (IsTraceBC()) {
        // see stateSplit as a part of JSByteCode if exists
        GateRef maybeStateSplit = acc_.GetDep(gate);
        GateRef current = Circuit::NullGate();
        if (acc_.GetOpCode(maybeStateSplit) == OpCode::STATE_SPLIT) {
            current = maybeStateSplit;
        } else {
            current = gate;
        }

        EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
        auto ecmaOpcodeGate = builder_.Int32(static_cast<uint32_t>(ecmaOpcode));
        GateRef constOpcode = builder_.Int32ToTaggedInt(ecmaOpcodeGate);
        GateRef typedPath = builder_.Int32ToTaggedInt(builder_.Int32(1));
        GateRef traceGate = builder_.CallRuntime(glue_, RTSTUB_ID(DebugAOTPrint), acc_.GetDep(current),
                                                 { constOpcode, typedPath }, gate);
        acc_.SetDep(current, traceGate);
        builder_.SetDepend(acc_.GetDep(gate));  // set gate depend: trace or STATE_SPLIT
    }

    if (IsProfiling()) {
        // see stateSplit as a part of JSByteCode if exists
        GateRef maybeStateSplit = acc_.GetDep(gate);
        GateRef current = Circuit::NullGate();
        if (acc_.GetOpCode(maybeStateSplit) == OpCode::STATE_SPLIT) {
            current = maybeStateSplit;
        } else {
            current = gate;
        }

        EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
        auto ecmaOpcodeGate = builder_.Int32(static_cast<uint32_t>(ecmaOpcode));
        GateRef constOpcode = builder_.Int32ToTaggedInt(ecmaOpcodeGate);
        GateRef mode =
            builder_.Int32ToTaggedInt(builder_.Int32(static_cast<int32_t>(OptCodeProfiler::Mode::TYPED_PATH)));
        GateRef profiling = builder_.CallRuntime(glue_, RTSTUB_ID(ProfileOptimizedCode), acc_.GetDep(current),
                                                 { constOpcode, mode }, gate);
        acc_.SetDep(current, profiling);
        builder_.SetDepend(acc_.GetDep(gate));  // set gate depend: profiling or STATE_SPLIT
    }
}
}
