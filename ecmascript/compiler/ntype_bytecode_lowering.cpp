/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/ntype_bytecode_lowering.h"

#include "ecmascript/compiler/lazy_deopt_dependency.h"
#include "ecmascript/compiler/type_info_accessors.h"
#include "ecmascript/dfx/vmstat/opt_code_profiler.h"
#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/jit/jit.h"
#include "ecmascript/lexical_env.h"

namespace panda::ecmascript::kungfu {

void NTypeBytecodeLowering::RunNTypeBytecodeLowering()
{
    std::vector<GateRef> gateList;
    circuit_->GetAllGates(gateList);
    for (const auto &gate : gateList) {
        auto op = acc_.GetOpCode(gate);
        if (op == OpCode::JS_BYTECODE) {
            Lower(gate);
        }
    }

    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After NTypeBytecodeLowering "
                           << "[" << GetMethodName() << "]"
                           << "===================="
                           << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
    }
}

void NTypeBytecodeLowering::Lower(GateRef gate)
{
    [[maybe_unused]] auto scopedGate = circuit_->VisitGateBegin(gate);
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    // initialize label manager
    Environment env(gate, circuit_, &builder_);
    switch (ecmaOpcode) {
        case EcmaOpcode::CREATEEMPTYARRAY_IMM8:
        case EcmaOpcode::CREATEEMPTYARRAY_IMM16:
            LowerNTypedCreateEmptyArray(gate);
            break;
        case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
        case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM16_ID16: {
                Jit::JitLockHolder lock(compilationEnv_, "LowerNTypedCreateArrayWithBuffer");
                LowerNTypedCreateArrayWithBuffer(gate);
            }
            break;
        case EcmaOpcode::COPYRESTARGS_IMM8:
        case EcmaOpcode::WIDE_COPYRESTARGS_PREF_IMM16:
            LowerNTypedCopyRestArgs(gate);
            break;
        case EcmaOpcode::GETUNMAPPEDARGS:
            LowerNTypedGetUnmappedArgs(gate);
            break;
        case EcmaOpcode::STOWNBYINDEX_IMM8_V8_IMM16:
        case EcmaOpcode::STOWNBYINDEX_IMM16_V8_IMM16:
        case EcmaOpcode::WIDE_STOWNBYINDEX_PREF_V8_IMM32:
            LowerNTypedStownByIndex(gate);
            break;
        case EcmaOpcode::THROW_UNDEFINEDIFHOLEWITHNAME_PREF_ID16:
            LowerThrowUndefinedIfHoleWithName(gate);
            break;
        case EcmaOpcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM8:
        case EcmaOpcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM16:
            LowerThrowIfSuperNotCorrectCall(gate);
            break;
        case EcmaOpcode::THROW_IFNOTOBJECT_PREF_V8:
            LowerThrowIfNotObject(gate);
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
        case EcmaOpcode::LDLOCALMODULEVAR_IMM8:
        case EcmaOpcode::WIDE_LDLOCALMODULEVAR_PREF_IMM16:
            LowerLdLocalMoudleVar(gate);
            break;
        case EcmaOpcode::LDEXTERNALMODULEVAR_IMM8:
        case EcmaOpcode::WIDE_LDEXTERNALMODULEVAR_PREF_IMM16:
            LowerLdExternalMoudleVar(gate);
            break;
        case EcmaOpcode::STMODULEVAR_IMM8:
        case EcmaOpcode::WIDE_STMODULEVAR_PREF_IMM16:
            LowerStModuleVar(gate);
            break;
        case EcmaOpcode::STOWNBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STOWNBYNAME_IMM16_ID16_V8: {
                Jit::JitLockHolder lock(compilationEnv_, "LowerNTypedStOwnByName");
                LowerNTypedStOwnByName(gate);
            }
            break;
        default:
            break;
    }
}

void NTypeBytecodeLowering::LowerThrowUndefinedIfHoleWithName(GateRef gate)
{
    AddProfiling(gate);
    GateRef value = acc_.GetValueIn(gate, 1); // 1: the second parameter
    builder_.LexVarIsHoleCheck(value);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), Circuit::NullGate());
}

void NTypeBytecodeLowering::LowerThrowIfSuperNotCorrectCall(GateRef gate)
{
    AddProfiling(gate);
    GateRef index = acc_.GetValueIn(gate, 0);
    GateRef value = acc_.GetValueIn(gate, 1);
    uint32_t indexValue = static_cast<uint32_t>(acc_.GetConstantValue(index));
    if (indexValue == CALL_SUPER_BEFORE_THIS_CHECK) {
        builder_.IsUndefinedOrHoleCheck(value);
    } else if (indexValue == FORBIDDEN_SUPER_REBIND_THIS_CHECK) {
        builder_.IsNotUndefinedOrHoleCheck(value);
    } else {
        UNREACHABLE();
    }
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), builder_.TaggedTrue());
}

void NTypeBytecodeLowering::LowerThrowIfNotObject(GateRef gate)
{
    AddProfiling(gate);
    GateRef value = acc_.GetValueIn(gate, 0); // 0: the first parameter
    builder_.EcmaObjectCheck(value);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), Circuit::NullGate());
}

void NTypeBytecodeLowering::LowerLdLexVar(GateRef gate)
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

void NTypeBytecodeLowering::LowerStLexVar(GateRef gate)
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

void NTypeBytecodeLowering::LowerNTypedCreateEmptyArray(GateRef gate)
{
    // in the future, the type of the elements in the array will be obtained through pgo,
    // and the type will be used to determine whether to create a typed-array.
    AddProfiling(gate);
    ElementsKind kind = acc_.TryGetElementsKind(gate);
    uint32_t length = acc_.TryGetArrayElementsLength(gate);
    RegionSpaceFlag flag = acc_.TryGetRegionSpaceFlag(gate);
    GateRef array = builder_.CreateArray(kind, 0, builder_.Int64(length), flag);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), array);
}

void NTypeBytecodeLowering::LowerNTypedCreateArrayWithBuffer(GateRef gate)
{
    // 2: number of CREATEARRAYWITHBUFFER_IMM16_ID16/CREATEARRAYWITHBUFFER_IMM8_ID16 inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef index = acc_.GetValueIn(gate, 0);
    auto methodOffset = acc_.TryGetMethodOffset(gate);
    uint32_t cpId = ptManager_->GetConstantPoolIDByMethodOffset(methodOffset);

    uint32_t constPoolIndex = static_cast<uint32_t>(acc_.GetConstantValue(index));
    JSTaggedValue arr = GetArrayLiteralValue(cpId, constPoolIndex);
    // Since jit has no lazy loading array from constantpool, arr might be undefined.
    if (arr.IsUndefined()) {
        return;
    }
    AddProfiling(gate);
    ElementsKind kind = acc_.TryGetElementsKind(gate);
    RegionSpaceFlag flag = acc_.TryGetRegionSpaceFlag(gate);
    GateRef cpIdGr = builder_.Int32(cpId);
    GateRef array =
        builder_.CreateArrayWithBuffer(kind, ArrayMetaDataAccessor::Mode::CREATE,
                                       cpIdGr, index, flag);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), array);
}

void NTypeBytecodeLowering::LowerNTypedCopyRestArgs(GateRef gate)
{
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef restIdx = acc_.GetValueIn(gate, 0);
    AddProfiling(gate);
    ElementsKind kind = acc_.TryGetElementsKind(gate);
    GateRef arguments =
        builder_.CreateArguments(kind, CreateArgumentsAccessor::Mode::REST_ARGUMENTS, restIdx);
    ReplaceGateWithPendingException(gate, builder_.GetState(), builder_.GetDepend(), arguments);
}

void NTypeBytecodeLowering::LowerNTypedGetUnmappedArgs(GateRef gate)
{
    ASSERT(acc_.GetNumValueIn(gate) == 0);
    GateRef restIdx = builder_.Int32(0);
    AddProfiling(gate);
    ElementsKind kind = acc_.TryGetElementsKind(gate);
    GateRef arguments =
        builder_.CreateArguments(kind, CreateArgumentsAccessor::Mode::UNMAPPED_ARGUMENTS, restIdx);
    ReplaceGateWithPendingException(gate, builder_.GetState(), builder_.GetDepend(), arguments);
}

void NTypeBytecodeLowering::LowerNTypedStownByIndex(GateRef gate)
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

void NTypeBytecodeLowering::AddProfiling(GateRef gate)
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

        GateRef func = builder_.Undefined();
        if (acc_.HasFrameState(gate)) {
            func = argAcc_->GetFrameArgsIn(gate, FrameArgIdx::FUNC);
        }

        GateRef bcIndex = builder_.Int32ToTaggedInt(builder_.Int32(acc_.TryGetBcIndex(gate)));
        EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
        auto ecmaOpcodeGate = builder_.Int32(static_cast<uint32_t>(ecmaOpcode));
        GateRef constOpcode = builder_.Int32ToTaggedInt(ecmaOpcodeGate);
        GateRef mode =
            builder_.Int32ToTaggedInt(builder_.Int32(static_cast<int32_t>(OptCodeProfiler::Mode::TYPED_PATH)));
        GateRef profiling = builder_.CallRuntime(glue_, RTSTUB_ID(ProfileOptimizedCode), acc_.GetDep(current),
            { func, bcIndex, constOpcode, mode }, gate);
        acc_.SetDep(current, profiling);
        builder_.SetDepend(acc_.GetDep(gate));  // set gate depend: profiling or STATE_SPLIT
    }
}

void NTypeBytecodeLowering::LowerLdLocalMoudleVar(GateRef gate)
{
    AddProfiling(gate);
    GateRef jsFunc = argAcc_->GetFrameArgsIn(gate, FrameArgIdx::FUNC);
    GateRef index = acc_.GetValueIn(gate, 0);
    GateRef result = builder_.LdLocalModuleVar(jsFunc, index);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void NTypeBytecodeLowering::LowerLdExternalMoudleVar(GateRef gate)
{
    if (!compilationEnv_->IsJitCompiler()) {
        return;
    }
    uint32_t pcOffset = acc_.TryGetPcOffset(gate);
    uint32_t methodOffset = acc_.TryGetMethodOffset(gate);
    if (methodOffset == 0) {
        return;
    }
    auto isResolved =
        static_cast<const JitCompilationEnv *>(compilationEnv_)->IsLdExtModuleVarResolved(methodOffset, pcOffset);
    if (!isResolved) {
        return;
    }

    if (enableLazyDeopt_) {
        if (!compilationEnv_->GetDependencies()->DependOnNotHotReloadPatchMain(compilationEnv_->GetHostThread())) {
            return;
        }
    } else {
        GateRef frameState = acc_.FindNearestFrameState(builder_.GetDepend());
        builder_.DeoptCheck(builder_.IsNotLdEndExecPatchMain(glue_), frameState, DeoptType::HOTRELOAD_PATCHMAIN);
    }

    GateRef jsFunc = argAcc_->GetFrameArgsIn(gate, FrameArgIdx::FUNC);
    GateRef index = acc_.GetValueIn(gate, 0);
    GateRef value = builder_.LdExternalModuleVar(jsFunc, index);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), value);
}

void NTypeBytecodeLowering::LowerStModuleVar(GateRef gate)
{
    AddProfiling(gate);
    GateRef jsFunc = argAcc_->GetFrameArgsIn(gate, FrameArgIdx::FUNC);
    GateRef index = acc_.GetValueIn(gate, 0);
    GateRef value = acc_.GetValueIn(gate, 1);
    builder_.StoreModuleVar(jsFunc, index, value);
    ReplaceGateWithPendingException(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void NTypeBytecodeLowering::LowerNTypedStOwnByName(GateRef gate)
{
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef receiver = acc_.GetValueIn(gate, 1); // 1: receiver
    if (acc_.GetOpCode(receiver) != OpCode::TYPED_CREATE_OBJ_WITH_BUFFER) {
        return;
    }

    GateRef hclassGate = acc_.GetValueIn(receiver, 2); // 2: hclass offset
    JSTaggedValue taggedHClass(acc_.GetConstantValue(hclassGate));
    GateRef stringId = acc_.GetValueIn(gate, 0);
    JSTaggedValue key = compilationEnv_->GetStringFromConstantPool(acc_.TryGetMethodOffset(gate),
                                                                   acc_.GetConstantValue(stringId));
    if (key.IsUndefined()) {
        return;
    }
    JSThread *thread = compilationEnv_->GetJSThread();
    JSHClass *hclass = JSHClass::Cast(taggedHClass.GetTaggedObject());
    int entry = JSHClass::FindPropertyEntry(thread, hclass, key);
    if (entry == -1) {
        return;
    }

    LayoutInfo *LayoutInfo = LayoutInfo::Cast(hclass->GetLayout(thread).GetTaggedObject());
    PropertyAttributes attr(LayoutInfo->GetAttr(thread, entry));
    if (attr.IsAccessor()) {
        return;
    }
    if (!attr.IsInlinedProps()) {
        return;
    }

    size_t offset = attr.GetOffset();
    GateRef accValue = acc_.GetValueIn(gate, 2); // 2: accValue
    builder_.StoreConstOffset(VariableType::JS_ANY(), receiver, hclass->GetInlinedPropertiesOffset(offset), accValue);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), Circuit::NullGate());
}

void NTypeBytecodeLowering::ReplaceGateWithPendingException(GateRef gate, GateRef state, GateRef depend, GateRef value)
{
    auto glue = glue_;
    auto condition = builder_.HasPendingException(glue, compilationEnv_);
    GateRef ifBranch = builder_.Branch(state, condition, 1, BranchWeight::DEOPT_WEIGHT, "checkException");
    GateRef ifTrue = builder_.IfTrue(ifBranch);
    GateRef ifFalse = builder_.IfFalse(ifBranch);
    GateRef eDepend = builder_.DependRelay(ifTrue, depend);
    GateRef sDepend = builder_.DependRelay(ifFalse, depend);

    StateDepend success(ifFalse, sDepend);
    StateDepend exception(ifTrue, eDepend);
    acc_.ReplaceHirWithIfBranch(gate, success, exception, value);
}
}
