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
#include "ecmascript/jit/jit_profiler.h"

#include "ecmascript/compiler/lazy_deopt_dependency.h"
#include "ecmascript/compiler/jit_compilation_env.h"
#include "ecmascript/compiler/pgo_type/pgo_type_manager.h"
#include "ecmascript/enum_conversion.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jit/jit.h"

namespace panda::ecmascript {
using namespace pgo;
JITProfiler::JITProfiler(EcmaVM *vm) : vm_(vm)
{
}

void JITProfiler::ProfileBytecode(JSThread *thread, const JSHandle<ProfileTypeInfo> &profileTypeInfo,
                                  ProfileTypeInfo *rawProfileTypeInfo,
                                  EntityId methodId, ApEntityId abcId, const uint8_t *pcStart, uint32_t codeSize,
                                  [[maybe_unused]]const panda_file::File::Header *header,
                                  JSHandle<JSFunction> jsFunction, JSHandle<GlobalEnv> env, bool useRawProfileTypeInfo)
{
    Clear();
    jsFunction_ = jsFunction;
    SetCurrentGlobalEnv(env);
    if (useRawProfileTypeInfo) {
        auto *jitCompilationEnv = static_cast<JitCompilationEnv*>(compilationEnv_);
        JSHandle<JSTaggedValue> newProfileTypeInfo = jitCompilationEnv->NewJSHandle(JSTaggedValue(rawProfileTypeInfo));
        profileTypeInfo_ = JSHandle<ProfileTypeInfo>::Cast(newProfileTypeInfo);
    } else {
        Jit::JitLockHolder lock(thread);
        profileTypeInfo_ = profileTypeInfo;
        if (profileTypeInfo_.GetTaggedType() == 0) {
            return;
        }
    }
    abcId_ = abcId;
    methodId_ = methodId;
    BytecodeInstruction bcIns(pcStart);
    auto bcInsLast = bcIns.JumpTo(codeSize);

    while (bcIns.GetAddress() != bcInsLast.GetAddress()) {
        auto opcode = bcIns.GetOpcode();
        auto bcOffset = bcIns.GetAddress() - pcStart;
        auto pc = bcIns.GetAddress();
        // Assuming that the assembly interpreter has executed all bytecode.
        SetBcOffsetBool(bcOffset);
        switch (opcode) {
            case EcmaOpcode::LDTHISBYNAME_IMM8_ID16:
            case EcmaOpcode::LDOBJBYNAME_IMM8_ID16: {
                Jit::JitLockHolder lock(thread);
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                ConvertICByName(bcOffset, slotId, BCType::LOAD);
                UpdateBcOffsetBoolWithNearSlotId(bcOffset, slotId);
                break;
            }
            case EcmaOpcode::LDTHISBYNAME_IMM16_ID16:
            case EcmaOpcode::LDOBJBYNAME_IMM16_ID16: {
                Jit::JitLockHolder lock(thread);
                uint16_t slotId = READ_INST_16_0();
                ConvertICByName(bcOffset, slotId, BCType::LOAD);
                UpdateBcOffsetBoolWithNearSlotId(bcOffset, slotId);
                break;
            }
            case EcmaOpcode::LDOBJBYVALUE_IMM8_V8:
            case EcmaOpcode::LDTHISBYVALUE_IMM8: {
                Jit::JitLockHolder lock(thread);
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                ConvertICByValue(bcOffset, slotId, BCType::LOAD);
                UpdateBcOffsetBoolWithNearSlotId(bcOffset, slotId);
                break;
            }
            case EcmaOpcode::LDOBJBYVALUE_IMM16_V8:
            case EcmaOpcode::LDTHISBYVALUE_IMM16: {
                Jit::JitLockHolder lock(thread);
                uint16_t slotId = READ_INST_16_0();
                ConvertICByValue(bcOffset, slotId, BCType::LOAD);
                UpdateBcOffsetBoolWithNearSlotId(bcOffset, slotId);
                break;
            }
            case EcmaOpcode::TRYLDGLOBALBYNAME_IMM8_ID16: {
                Jit::JitLockHolder lock(thread);
                uint32_t slotId = READ_INST_8_0();
                ASSERT(bcOffset >= 0);
                ConvertTryldGlobalByName(static_cast<uint32_t>(bcOffset), slotId);
                UpdateBcOffsetBool(bcOffset, slotId);
                break;
            }
            case EcmaOpcode::TRYLDGLOBALBYNAME_IMM16_ID16: {
                Jit::JitLockHolder lock(thread);
                uint32_t slotId = READ_INST_16_0();
                ASSERT(bcOffset >= 0);
                ConvertTryldGlobalByName(static_cast<uint32_t>(bcOffset), slotId);
                UpdateBcOffsetBool(bcOffset, slotId);
                break;
            }

            case EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8:
            case EcmaOpcode::STTHISBYNAME_IMM8_ID16:
            case EcmaOpcode::DEFINEPROPERTYBYNAME_IMM8_ID16_V8: {
                Jit::JitLockHolder lock(thread);
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                ConvertICByName(bcOffset, slotId, BCType::STORE);
                if (opcode != EcmaOpcode::DEFINEPROPERTYBYNAME_IMM8_ID16_V8) {
                    UpdateBcOffsetBoolWithNearSlotId(bcOffset, slotId);
                }
                break;
            }
            case EcmaOpcode::STOBJBYNAME_IMM16_ID16_V8:
            case EcmaOpcode::STTHISBYNAME_IMM16_ID16: {
                Jit::JitLockHolder lock(thread);
                uint16_t slotId = READ_INST_16_0();
                ConvertICByName(bcOffset, slotId, BCType::STORE);
                UpdateBcOffsetBoolWithNearSlotId(bcOffset, slotId);
                break;
            }
            case EcmaOpcode::STOBJBYVALUE_IMM8_V8_V8:
            case EcmaOpcode::STOWNBYINDEX_IMM8_V8_IMM16:
            case EcmaOpcode::STTHISBYVALUE_IMM8_V8: {
                Jit::JitLockHolder lock(thread);
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                ConvertICByValue(bcOffset, slotId, BCType::STORE);
                UpdateBcOffsetBoolWithNearSlotId(bcOffset, slotId);
                break;
            }
            case EcmaOpcode::STOBJBYVALUE_IMM16_V8_V8:
            case EcmaOpcode::STOWNBYINDEX_IMM16_V8_IMM16:
            case EcmaOpcode::STTHISBYVALUE_IMM16_V8: {
                Jit::JitLockHolder lock(thread);
                uint16_t slotId = READ_INST_16_0();
                ConvertICByValue(bcOffset, slotId, BCType::STORE);
                UpdateBcOffsetBoolWithNearSlotId(bcOffset, slotId);
                break;
            }
            case EcmaOpcode::LDEXTERNALMODULEVAR_IMM8: {
                Jit::JitLockHolder lock(thread);
                uint32_t index = READ_INST_8_0();
                ConvertExternalModuleVar(index, bcOffset);
                break;
            }
            case EcmaOpcode::WIDE_LDEXTERNALMODULEVAR_PREF_IMM16: {
                Jit::JitLockHolder lock(thread);
                uint32_t index = READ_INST_16_1();
                ConvertExternalModuleVar(index, bcOffset);
                break;
            }
            // Op
            case EcmaOpcode::ADD2_IMM8_V8:
            case EcmaOpcode::SUB2_IMM8_V8:
            case EcmaOpcode::MUL2_IMM8_V8:
            case EcmaOpcode::DIV2_IMM8_V8:
            case EcmaOpcode::MOD2_IMM8_V8:
            case EcmaOpcode::SHL2_IMM8_V8:
            case EcmaOpcode::SHR2_IMM8_V8:
            case EcmaOpcode::AND2_IMM8_V8:
            case EcmaOpcode::OR2_IMM8_V8:
            case EcmaOpcode::XOR2_IMM8_V8:
            case EcmaOpcode::ASHR2_IMM8_V8:
            case EcmaOpcode::NEG_IMM8:
            case EcmaOpcode::NOT_IMM8:
            case EcmaOpcode::INC_IMM8:
            case EcmaOpcode::DEC_IMM8:
            case EcmaOpcode::EQ_IMM8_V8:
            case EcmaOpcode::NOTEQ_IMM8_V8:
            case EcmaOpcode::LESS_IMM8_V8:
            case EcmaOpcode::LESSEQ_IMM8_V8:
            case EcmaOpcode::GREATER_IMM8_V8:
            case EcmaOpcode::GREATEREQ_IMM8_V8:
            case EcmaOpcode::STRICTNOTEQ_IMM8_V8:
            case EcmaOpcode::STRICTEQ_IMM8_V8: {
                Jit::JitLockHolder lock(thread);
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                ConvertOpType(slotId, bcOffset);
                UpdateBcOffsetBool(bcOffset, slotId);
                break;
            }
            case EcmaOpcode::EXP_IMM8_V8:
            case EcmaOpcode::TONUMERIC_IMM8: {
                Jit::JitLockHolder lock(thread);
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                ConvertOpType(slotId, bcOffset);
                break;
            }
            case EcmaOpcode::CALLRUNTIME_ISTRUE_PREF_IMM8:
            case EcmaOpcode::CALLRUNTIME_ISFALSE_PREF_IMM8: {
                Jit::JitLockHolder lock(thread);
                uint8_t slotId = READ_INST_8_1();
                CHECK_SLOTID_BREAK(slotId);
                ConvertOpType(slotId, bcOffset);
                UpdateBcOffsetBool(bcOffset, slotId);
                break;
            }
            // Call
            case EcmaOpcode::CALLARG0_IMM8:
            case EcmaOpcode::CALLARG1_IMM8_V8:
            case EcmaOpcode::CALLARGS2_IMM8_V8_V8:
            case EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8:
            case EcmaOpcode::CALLRANGE_IMM8_IMM8_V8:
            case EcmaOpcode::CALLTHIS0_IMM8_V8:
            case EcmaOpcode::CALLTHIS1_IMM8_V8_V8:
            case EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8:
            case EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
            case EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8:
            case EcmaOpcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8: {
                Jit::JitLockHolder lock(thread);
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                ConvertCall(slotId, bcOffset);
                UpdateBcOffsetBool(bcOffset, slotId);
                break;
            }
            case EcmaOpcode::CALLRUNTIME_CALLINIT_PREF_IMM8_V8: {
                Jit::JitLockHolder lock(thread);
                uint8_t slotId = READ_INST_8_1();
                CHECK_SLOTID_BREAK(slotId);
                ConvertCall(slotId, bcOffset);
                break;
            }
            case EcmaOpcode::WIDE_CALLRANGE_PREF_IMM16_V8:
            case EcmaOpcode::WIDE_CALLTHISRANGE_PREF_IMM16_V8: {
                // no ic slot
                break;
            }
            case EcmaOpcode::NEWOBJRANGE_IMM8_IMM8_V8: {
                Jit::JitLockHolder lock(thread);
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                ConvertNewObjRange(slotId, bcOffset);
                UpdateBcOffsetBool(bcOffset, slotId);
                break;
            }
            case EcmaOpcode::NEWOBJRANGE_IMM16_IMM8_V8: {
                Jit::JitLockHolder lock(thread);
                uint16_t slotId = READ_INST_16_0();
                ConvertNewObjRange(slotId, bcOffset);
                UpdateBcOffsetBool(bcOffset, slotId);
                break;
            }
            case EcmaOpcode::WIDE_NEWOBJRANGE_PREF_IMM16_V8: {
                break;
            }
            // Create object
            case EcmaOpcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8: {
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                (void) slotId;
                break;
            }
            case EcmaOpcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8: {
                uint16_t slotId = READ_INST_16_0();
                (void) slotId;
                break;
            }
            case EcmaOpcode::DEFINEFUNC_IMM8_ID16_IMM8: {
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                break;
            }
            case EcmaOpcode::DEFINEFUNC_IMM16_ID16_IMM8: {
                uint16_t slotId = READ_INST_16_0();
                (void) slotId;
                break;
            }
            case EcmaOpcode::CREATEOBJECTWITHBUFFER_IMM8_ID16:
            case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
            case EcmaOpcode::CREATEEMPTYARRAY_IMM8: {
                Jit::JitLockHolder lock(thread);
                auto traceId =
                    static_cast<int32_t>(reinterpret_cast<uintptr_t>(pc) - reinterpret_cast<uintptr_t>(header));
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                ConvertCreateObject(slotId, bcOffset, traceId);
                UpdateBcOffsetBool(bcOffset, slotId);
                break;
            }
            case EcmaOpcode::CREATEOBJECTWITHBUFFER_IMM16_ID16:
            case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM16_ID16:
            case EcmaOpcode::CREATEEMPTYARRAY_IMM16: {
                Jit::JitLockHolder lock(thread);
                auto traceId =
                    static_cast<int32_t>(reinterpret_cast<uintptr_t>(pc) - reinterpret_cast<uintptr_t>(header));
                uint16_t slotId = READ_INST_16_0();
                ConvertCreateObject(slotId, bcOffset, traceId);
                UpdateBcOffsetBool(bcOffset, slotId);
                break;
            }
            case EcmaOpcode::GETITERATOR_IMM8: {
                Jit::JitLockHolder lock(thread);
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                ConvertGetIterator(slotId, bcOffset);
                break;
            }
            case EcmaOpcode::GETITERATOR_IMM16: {
                Jit::JitLockHolder lock(thread);
                uint16_t slotId = READ_INST_16_0();
                ConvertGetIterator(slotId, bcOffset);
                break;
            }
            // Others
            case EcmaOpcode::INSTANCEOF_IMM8_V8: {
                Jit::JitLockHolder lock(thread);
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                ConvertInstanceof(bcOffset, slotId);
                break;
            }
            case EcmaOpcode::DEFINEGETTERSETTERBYVALUE_V8_V8_V8_V8:
            default:
                break;
        }
        bcIns = bcIns.GetNext();
    }
}

// PGOSampleType
void JITProfiler::ConvertOpType(uint32_t slotId, long bcOffset)
{
    JSTaggedValue slotValue = profileTypeInfo_->Get(mainThread_, slotId);
    if (slotValue.IsInt()) {
        auto type = slotValue.GetInt();
        UpdatePGOType(bcOffset, chunk_->New<PGOSampleType>(type));
    }
}

void JITProfiler::ConvertCall(uint32_t slotId, long bcOffset)
{
    JSTaggedValue slotValue = profileTypeInfo_->Get(mainThread_, slotId);
    ProfileType::Kind kind;
    int calleeMethodId = 0;
    ApEntityId calleeAbcId = 0;
    if (slotValue.IsInt()) {
        calleeMethodId = slotValue.GetInt();
        if (calleeMethodId == 0) {
            return;
        }
        calleeAbcId = abcId_;
        ASSERT(calleeMethodId <= 0);
        kind = ProfileType::Kind::BuiltinFunctionId;
    } else if (slotValue.IsJSFunction()) {
        JSFunction *callee = JSFunction::Cast(slotValue);
        Method *calleeMethod = Method::Cast(callee->GetMethod(mainThread_));
        compilationEnv_->ProcessMethod(calleeMethod->GetMethodLiteral(mainThread_),
                                       calleeMethod->GetJSPandaFile(mainThread_));
        calleeMethodId = static_cast<int>(calleeMethod->GetMethodId().GetOffset());
        if (compilationEnv_->SupportHeapConstant() &&
            calleeMethod->GetFunctionKind() != FunctionKind::ARROW_FUNCTION &&
            callee->IsCallable()) {
            auto *jitCompilationEnv = static_cast<JitCompilationEnv*>(compilationEnv_);
            JSHandle<JSTaggedValue> calleeHandle = jitCompilationEnv->NewJSHandle(JSTaggedValue(callee));
            auto heapConstantIndex = jitCompilationEnv->RecordHeapConstant(calleeHandle);
            if (calleeMethod->GetMethodLiteral(mainThread_)->IsTypedCall() && callee->IsCompiledCode()) {
                jitCompilationEnv->RecordCallMethodId2HeapConstantIndex(calleeMethodId, heapConstantIndex);
            } else {
                jitCompilationEnv->RecordOnlyInlineMethodId2HeapConstantIndex(calleeMethodId, heapConstantIndex);
            }
        }
        calleeAbcId = PGOProfiler::GetMethodAbcId(mainThread_, callee);
        static_cast<JitCompilationEnv *>(compilationEnv_)
            ->UpdateFuncSlotIdMap(calleeMethodId, methodId_.GetOffset(), slotId);
        kind = ProfileType::Kind::MethodId;
    } else {
        return;
    }
    PGOSampleType* type = chunk_->New<PGOSampleType>(ProfileType(abcId_, std::abs(calleeMethodId), kind));
    UpdatePGOType(bcOffset, type);
}

void JITProfiler::ConvertNewObjRange(uint32_t slotId, long bcOffset)
{
    JSTaggedValue slotValue = profileTypeInfo_->Get(mainThread_, slotId);
    int ctorMethodId = 0;
    JSHClass* hclass = nullptr;
    if (slotValue.IsInt()) {
        ctorMethodId = slotValue.GetInt();
        // JIT cannot optimize this scenario because it doesn't know the hclass
        if (ctorMethodId > 0) {
            return;
        }
    } else if (slotValue.IsJSFunction()) {
        JSFunction *callee = JSFunction::Cast(slotValue);
        Method *calleeMethod = Method::Cast(callee->GetMethod(mainThread_));
        compilationEnv_->ProcessMethod(calleeMethod->GetMethodLiteral(mainThread_),
                                       calleeMethod->GetJSPandaFile(mainThread_));
        ctorMethodId = static_cast<int>(calleeMethod->GetMethodId().GetOffset());
        if (compilationEnv_->SupportHeapConstant()) {
            auto *jitCompilationEnv = static_cast<JitCompilationEnv*>(compilationEnv_);
            JSHandle<JSTaggedValue> calleeHandle = jitCompilationEnv->NewJSHandle(JSTaggedValue(callee));
            auto heapConstantIndex = jitCompilationEnv->RecordHeapConstant(calleeHandle);
            jitCompilationEnv->RecordCtorMethodId2HeapConstantIndex(ctorMethodId, heapConstantIndex);
        }
        JSTaggedValue protoOrHClass = callee->GetProtoOrHClass(mainThread_);
        if (protoOrHClass.IsJSHClass()) {
            hclass = JSHClass::Cast(protoOrHClass.GetTaggedObject());
        } else {
            return;
        }
    } else {
        return;
    }
    if (ctorMethodId > 0) {
        ptManager_->RecordAndGetHclassIndexForJIT(hclass);
        auto pt = ProfileType(abcId_, std::abs(ctorMethodId), ProfileType::Kind::JITClassId, true);
        PGODefineOpType* type = chunk_->New<PGODefineOpType>(pt, hclass);
        UpdatePGOType(bcOffset, type);
    } else {
        auto kind = ProfileType::Kind::BuiltinFunctionId;
        auto type = chunk_->New<PGOSampleType>(ProfileType(abcId_, std::abs(ctorMethodId), kind));
        UpdatePGOType(bcOffset, type);
    }
}

void JITProfiler::ConvertGetIterator(uint32_t slotId, long bcOffset)
{
    if (mainThread_->GetEnableLazyBuiltins()) {
        return;
    }
    JSTaggedValue value = profileTypeInfo_->Get(mainThread_, slotId);
    if (!value.IsInt()) {
        return;
    }
    int iterKind = value.GetInt();
    ASSERT(iterKind <= 0);
    ProfileType::Kind pgoKind = ProfileType::Kind::BuiltinFunctionId;
    auto type = chunk_->New<PGOSampleType>(ProfileType(abcId_, std::abs(iterKind), pgoKind));
    UpdatePGOType(bcOffset, type);
}

void JITProfiler::ConvertCreateObject(uint32_t slotId, long bcOffset, [[maybe_unused]]int32_t traceId)
{
    JSTaggedValue slotValue = profileTypeInfo_->Get(mainThread_, slotId);
    if (!slotValue.IsHeapObject()) {
        return;
    }
    if (slotValue.IsWeak()) {
        auto object = slotValue.GetWeakReferentUnChecked();
        if (object->GetClass()->IsHClass()) {
            auto newHClass = JSHClass::Cast(object);
            PGODefineOpType* objDefType = chunk_->New<PGODefineOpType>(ProfileType::CreateJITType(), newHClass);
            ptManager_->RecordAndGetHclassIndexForJIT(newHClass);
            UpdatePGOType(bcOffset, objDefType);
        }
    } else if (slotValue.IsTrackInfoObject()) {
        TrackInfo *trackInfo = TrackInfo::Cast(slotValue.GetTaggedObject());
        auto hclass = JSHClass::Cast(trackInfo->GetCachedHClass(mainThread_).GetTaggedObject());
        PGODefineOpType* objDefType = chunk_->New<PGODefineOpType>(ProfileType::CreateJITType(), hclass);
        ptManager_->RecordAndGetHclassIndexForJIT(hclass);
        auto elementsKind = trackInfo->GetElementsKind();
        objDefType->SetElementsKind(elementsKind);
        objDefType->SetElementsLength(trackInfo->GetArrayLength());
        objDefType->SetSpaceFlag(trackInfo->GetSpaceFlag());
        UpdatePGOType(bcOffset, objDefType);
    }
}

void JITProfiler::ConvertICByName(int32_t bcOffset, uint32_t slotId, BCType type)
{
    ProfileTypeAccessorLockScope accessorLockScope(vm_->GetJSThreadNoCheck());
    JSTaggedValue firstValue = profileTypeInfo_->Get(mainThread_, slotId);
    if (!firstValue.IsHeapObject()) {
        JSTaggedValue secondValue = profileTypeInfo_->Get(mainThread_, slotId + 1);
        if (firstValue.IsHole() && secondValue.IsString()) {
            // Mega state
            AddObjectInfoWithMega(bcOffset);
        }
        return;
    }
    if (firstValue.IsWeak()) {
        TaggedObject *object = firstValue.GetWeakReferentUnChecked();
        if (object->GetClass()->IsHClass()) {
            JSTaggedValue secondValue = profileTypeInfo_->Get(mainThread_, slotId + 1);
            JSHClass *hclass = JSHClass::Cast(object);
            ConvertICByNameWithHandler(abcId_, bcOffset, hclass, secondValue, type, slotId + 1);
        }
        return;
    }
    ConvertICByNameWithPoly(abcId_, bcOffset, firstValue, type, slotId);
}

void JITProfiler::ConvertICByNameWithHandler(ApEntityId abcId, int32_t bcOffset,
                                             JSHClass *hclass,
                                             JSTaggedValue secondValue, BCType type, uint32_t slotId)
{
    if (type == BCType::LOAD) {
        HandleLoadType(abcId, bcOffset, hclass, secondValue, slotId);
        // LoadGlobal
        return;
    }
    HandleOtherTypes(abcId, bcOffset, hclass, secondValue, slotId);
}

void JITProfiler::HandleLoadType(ApEntityId &abcId, int32_t &bcOffset,
                                 JSHClass *hclass, JSTaggedValue &secondValue, uint32_t slotId)
{
    if (secondValue.IsInt()) {
        HandleLoadTypeInt(abcId, bcOffset, hclass, secondValue);
    } else if (secondValue.IsPrototypeHandler()) {
        HandleLoadTypePrototypeHandler(abcId, bcOffset, hclass, secondValue, slotId);
    }
}

void JITProfiler::HandleLoadTypeInt(ApEntityId &abcId, int32_t &bcOffset,
                                    JSHClass *hclass, JSTaggedValue &secondValue)
{
    auto handlerInfo = static_cast<uint32_t>(secondValue.GetInt());
    if (!HandlerBase::IsNonExist(handlerInfo)) {
        if (AddBuiltinsInfoByNameInInstance(abcId, bcOffset, hclass)) {
            return;
        }
    }
    if (HandlerBase::IsField(handlerInfo) || HandlerBase::IsAccessor(handlerInfo)) {
        if (HandlerBase::IsNonExist(handlerInfo)) {
            AddObjectInfo(abcId, bcOffset, hclass, nullptr, nullptr);
        } else {
            AddObjectInfo(abcId, bcOffset, hclass, hclass, hclass);
        }
    }
}

void JITProfiler::HandleLoadTypePrototypeHandler(ApEntityId &abcId, int32_t &bcOffset, JSHClass *hclass,
                                                 JSTaggedValue &secondValue, uint32_t slotId, JSTaggedValue name)
{
    auto prototypeHandler = PrototypeHandler::Cast(secondValue.GetTaggedObject());
    auto cellValue = prototypeHandler->GetProtoCell(mainThread_);
    if (cellValue.IsUndefined()) {
        return;
    }
    ProtoChangeMarker *cell = ProtoChangeMarker::Cast(cellValue.GetTaggedObject());
    if (cell->GetHasChanged()) {
        return;
    }
    JSTaggedValue handlerInfoVal = prototypeHandler->GetHandlerInfo(mainThread_);
    auto accessorMethodId = prototypeHandler->GetAccessorMethodId();
    auto accessor = prototypeHandler->GetAccessorJSFunction(mainThread_);
    if (!handlerInfoVal.IsInt()) {
        return;
    }
    auto handlerInfo = static_cast<uint32_t>(handlerInfoVal.GetInt());
    JSTaggedValue holder = prototypeHandler->GetHolder(mainThread_);
    JSHClass *holderHClass = nullptr;
    bool isNonExist = HandlerBase::IsNonExist(handlerInfo);
    if (holder.IsHeapObject()) {
        holderHClass = holder.GetTaggedObject()->GetClass();
    }
    if (!kungfu::LazyDeoptAllDependencies::CheckStableProtoChain(mainThread_, hclass, holderHClass,
                                                                 GetCurrentGlobalEnv().GetObject<GlobalEnv>())) {
        return;
    }
    if (accessor.IsJSFunction()) {
        auto accessorFunction = JSFunction::Cast(accessor);
        auto methodId = Method::Cast(accessorFunction->GetMethod(mainThread_))->GetMethodId().GetOffset();
        ASSERT(accessorMethodId == methodId);
        accessorMethodId = methodId;
        static_cast<JitCompilationEnv *>(compilationEnv_)
            ->UpdateFuncSlotIdMap(accessorMethodId, methodId_.GetOffset(), slotId);
    }
    if (AddBuiltinsInfoByNameInProt(abcId, bcOffset, hclass, holderHClass, isNonExist)) {
        return;
    }
    if (isNonExist) {
        AddObjectInfo(abcId, bcOffset, hclass, nullptr, nullptr);
        return;
    }
    if (compilationEnv_->SupportHeapConstant()) {
        auto *jitCompilationEnv = static_cast<JitCompilationEnv*>(compilationEnv_);
        JSHandle<JSTaggedValue> holderHandler = jitCompilationEnv->NewJSHandle(holder);
        uint32_t heapConstantIndex = jitCompilationEnv->RecordHeapConstant(holderHandler);
        int32_t holderHClassIndex = ptManager_->RecordAndGetHclassIndexForJIT(holderHClass);
        jitCompilationEnv->RecordHolderHClassIndex2HeapConstantIndex(holderHClassIndex, heapConstantIndex);
    }
    auto primitiveType = HandlerBase::TryGetPrimitiveType(handlerInfo);
    AddObjectInfo(abcId, bcOffset, hclass, holderHClass, holderHClass, accessorMethodId, primitiveType, name);
}

void JITProfiler::HandleOtherTypes(ApEntityId &abcId, int32_t &bcOffset,
                                   JSHClass *hclass, JSTaggedValue &secondValue, uint32_t slotId)
{
    if (secondValue.IsInt()) {
        AddObjectInfo(abcId, bcOffset, hclass, hclass, hclass);
    } else if (secondValue.IsTransitionHandler()) {
        HandleTransitionHandler(abcId, bcOffset, hclass, secondValue);
    } else if (secondValue.IsTransWithProtoHandler()) {
        HandleTransWithProtoHandler(abcId, bcOffset, hclass, secondValue);
    } else if (secondValue.IsPrototypeHandler()) {
        HandleOtherTypesPrototypeHandler(abcId, bcOffset, hclass, secondValue, slotId);
    } else if (secondValue.IsPropertyBox()) {
        // StoreGlobal
    } else if (secondValue.IsStoreAOTHandler()) {
        HandleStoreAOTHandler(abcId, bcOffset, hclass, secondValue);
    }
}

void JITProfiler::HandleTransitionHandler(ApEntityId &abcId, int32_t &bcOffset,
                                          JSHClass *hclass, JSTaggedValue &secondValue)
{
    auto transitionHandler = TransitionHandler::Cast(secondValue.GetTaggedObject());
    auto transitionHClassVal = transitionHandler->GetTransitionHClass(mainThread_);
    if (transitionHClassVal.IsJSHClass()) {
        auto transitionHClass = JSHClass::Cast(transitionHClassVal.GetTaggedObject());
        AddObjectInfo(abcId, bcOffset, hclass, hclass, transitionHClass);
    }
}

void JITProfiler::HandleTransWithProtoHandler(ApEntityId &abcId, int32_t &bcOffset,
                                              JSHClass *hclass, JSTaggedValue &secondValue)
{
    auto transWithProtoHandler = TransWithProtoHandler::Cast(secondValue.GetTaggedObject());
    auto cellValue = transWithProtoHandler->GetProtoCell(mainThread_);
    ASSERT(cellValue.IsProtoChangeMarker());
    ProtoChangeMarker *cell = ProtoChangeMarker::Cast(cellValue.GetTaggedObject());
    if (cell->GetHasChanged()) {
        return;
    }
    auto transitionHClassVal = transWithProtoHandler->GetTransitionHClass(mainThread_);
    if (transitionHClassVal.IsJSHClass()) {
        auto transitionHClass = JSHClass::Cast(transitionHClassVal.GetTaggedObject());
        AddObjectInfo(abcId, bcOffset, hclass, hclass, transitionHClass);
    }
}

void JITProfiler::HandleOtherTypesPrototypeHandler(ApEntityId &abcId, int32_t &bcOffset,
                                                   JSHClass *hclass, JSTaggedValue &secondValue, uint32_t slotId)
{
    auto prototypeHandler = PrototypeHandler::Cast(secondValue.GetTaggedObject());
    auto cellValue = prototypeHandler->GetProtoCell(mainThread_);
    if (cellValue.IsUndefined()) {
        return;
    }
    ProtoChangeMarker *cell = ProtoChangeMarker::Cast(cellValue.GetTaggedObject());
    if (cell->GetHasChanged()) {
        return;
    }
    auto holder = prototypeHandler->GetHolder(mainThread_);
    auto holderHClass = holder.GetTaggedObject()->GetClass();
    auto accessorMethodId = prototypeHandler->GetAccessorMethodId();
    auto accessor = prototypeHandler->GetAccessorJSFunction(mainThread_);
    if (accessor.IsJSFunction()) {
        auto accessorFunction = JSFunction::Cast(accessor);
        auto methodId = Method::Cast(accessorFunction->GetMethod(mainThread_))->GetMethodId().GetOffset();
        ASSERT(accessorMethodId == methodId);
        accessorMethodId = methodId;
        static_cast<JitCompilationEnv *>(compilationEnv_)
            ->UpdateFuncSlotIdMap(accessorMethodId, methodId_.GetOffset(), slotId);
    }
    AddObjectInfo(abcId, bcOffset, hclass, holderHClass, holderHClass, accessorMethodId);
}

void JITProfiler::HandleStoreAOTHandler(ApEntityId &abcId, int32_t &bcOffset,
                                        JSHClass *hclass, JSTaggedValue &secondValue)
{
    StoreAOTHandler *storeAOTHandler = StoreAOTHandler::Cast(secondValue.GetTaggedObject());
    auto cellValue = storeAOTHandler->GetProtoCell(mainThread_);
    ASSERT(cellValue.IsProtoChangeMarker());
    ProtoChangeMarker *cell = ProtoChangeMarker::Cast(cellValue.GetTaggedObject());
    if (cell->GetHasChanged()) {
        return;
    }
    auto holder = storeAOTHandler->GetHolder(mainThread_);
    auto holderHClass = holder.GetTaggedObject()->GetClass();
    AddObjectInfo(abcId, bcOffset, hclass, holderHClass, holderHClass);
}

void JITProfiler::ConvertICByNameWithPoly(ApEntityId abcId, int32_t bcOffset, JSTaggedValue cacheValue, BCType type,
                                          uint32_t slotId)
{
    if (cacheValue.IsWeak()) {
        return;
    }
    ASSERT(cacheValue.IsTaggedArray());
    auto array = TaggedArray::Cast(cacheValue);
    uint32_t length = array->GetLength();
    for (uint32_t i = 0; i < length; i += 2) { // 2 means one ic, two slot
        auto result = array->Get(mainThread_, i);
        auto handler = array->Get(mainThread_, i + 1);
        if (!result.IsHeapObject() || !result.IsWeak()) {
            continue;
        }
        TaggedObject *object = result.GetWeakReferentUnChecked();
        if (!object->GetClass()->IsHClass()) {
            continue;
        }
        JSHClass *hclass = JSHClass::Cast(object);
        ConvertICByNameWithHandler(abcId, bcOffset, hclass, handler, type, slotId);
    }
}

void JITProfiler::ConvertICByValue(int32_t bcOffset, uint32_t slotId, BCType type)
{
    ProfileTypeAccessorLockScope accessorLockScope(vm_->GetJSThreadNoCheck());
    JSTaggedValue firstValue = profileTypeInfo_->Get(mainThread_, slotId);
    if (!firstValue.IsHeapObject()) {
        if (firstValue.IsHole()) {
            // Mega state
            AddObjectInfoWithMega(bcOffset);
        }
        return;
    }
    if (firstValue.IsWeak()) {
        TaggedObject *object = firstValue.GetWeakReferentUnChecked();
        if (object->GetClass()->IsHClass()) {
            JSTaggedValue secondValue = profileTypeInfo_->Get(mainThread_, slotId + 1);
            JSHClass *hclass = JSHClass::Cast(object);
            ConvertICByValueWithHandler(abcId_, bcOffset, hclass, secondValue, type, slotId);
        }
        return;
    }
    // Check key
    if ((firstValue.IsString() || firstValue.IsSymbol())) {
        JSTaggedValue secondValue = profileTypeInfo_->Get(mainThread_, slotId + 1);
        ConvertICByValueWithPoly(abcId_, bcOffset, firstValue, secondValue, type, slotId);
        return;
    }
    // Check without key
    ConvertICByValueWithPoly(abcId_, bcOffset, firstValue, firstValue, type, slotId);
}

void JITProfiler::ConvertICByValueWithHandler(ApEntityId abcId, int32_t bcOffset,
                                              JSHClass *hclass, JSTaggedValue secondValue,
                                              BCType type, uint32_t slotId, JSTaggedValue name)
{
    if (type == BCType::LOAD) {
        if (secondValue.IsInt()) {
            auto handlerInfo = static_cast<uint32_t>(secondValue.GetInt());
            if (HandlerBase::IsNormalElement(handlerInfo) || HandlerBase::IsStringElement(handlerInfo)) {
                if (HandlerBase::NeedSkipInPGODump(handlerInfo)) {
                    return;
                }
                AddBuiltinsInfo(abcId, bcOffset, hclass, hclass);
                return;
            }
            if (HandlerBase::IsTypedArrayElement(handlerInfo)) {
                OnHeapMode onHeap =  HandlerBase::IsOnHeap(handlerInfo) ? OnHeapMode::ON_HEAP : OnHeapMode::NOT_ON_HEAP;
                AddBuiltinsInfo(abcId, bcOffset, hclass, hclass, onHeap);
                return;
            }
            AddObjectInfo(abcId, bcOffset, hclass, hclass, hclass, INVALID_METHOD_INDEX, PRIMITIVE_TYPE_INVALID, name);
        } else if (secondValue.IsPrototypeHandler()) {
            HandleLoadTypePrototypeHandler(abcId, bcOffset, hclass, secondValue, slotId, name);
        }
        return;
    }
    HandleStoreType(abcId, bcOffset, hclass, secondValue);
}

void JITProfiler::HandleStoreType(ApEntityId &abcId, int32_t &bcOffset,
                                  JSHClass *hclass, JSTaggedValue &secondValue)
{
    if (secondValue.IsInt()) {
        auto handlerInfo = static_cast<uint32_t>(secondValue.GetInt());
        if (HandlerBase::IsNormalElement(handlerInfo) || HandlerBase::IsStringElement(handlerInfo)) {
            AddBuiltinsInfo(abcId, bcOffset, hclass, hclass,
                            OnHeapMode::NONE, HandlerBase::IsStoreOutOfBounds(handlerInfo));
            return;
        }
        if (HandlerBase::IsTypedArrayElement(handlerInfo)) {
            OnHeapMode onHeap = HandlerBase::IsOnHeap(handlerInfo) ? OnHeapMode::ON_HEAP : OnHeapMode::NOT_ON_HEAP;
            AddBuiltinsInfo(abcId,  bcOffset, hclass, hclass, onHeap,
                            HandlerBase::IsStoreOutOfBounds(handlerInfo));
            return;
        }
        AddObjectInfo(abcId, bcOffset, hclass, hclass, hclass);
    } else if (secondValue.IsTransitionHandler()) {
        HandleTransition(abcId, bcOffset, hclass, secondValue);
    } else if (secondValue.IsTransWithProtoHandler()) {
        HandleTransWithProto(abcId, bcOffset, hclass, secondValue);
    } else if (secondValue.IsPrototypeHandler()) {
        HandlePrototypeHandler(abcId, bcOffset, hclass, secondValue);
    }
}

void JITProfiler::HandleTransition(ApEntityId &abcId, int32_t &bcOffset,
                                   JSHClass *hclass, JSTaggedValue &secondValue)
{
    auto transitionHandler = TransitionHandler::Cast(secondValue.GetTaggedObject());
    auto transitionHClassVal = transitionHandler->GetTransitionHClass(mainThread_);
    if (!transitionHClassVal.IsJSHClass()) {
        return ;
    }
    auto handlerInfoValue = transitionHandler->GetHandlerInfo(mainThread_);
    auto transitionHClass = JSHClass::Cast(transitionHClassVal.GetTaggedObject());
    if (handlerInfoValue.IsInt()) {
        auto handlerInfo = static_cast<uint32_t>(handlerInfoValue.GetInt());
        if (HandlerBase::IsElement(handlerInfo)) {
            AddBuiltinsInfo(abcId, bcOffset, hclass, transitionHClass,
                            OnHeapMode::NONE, HandlerBase::IsStoreOutOfBounds(handlerInfo));
            return;
        }
    }
    AddObjectInfo(abcId, bcOffset, hclass, hclass, transitionHClass);
}

void JITProfiler::HandleTransWithProto(ApEntityId &abcId, int32_t &bcOffset,
                                       JSHClass *hclass, JSTaggedValue &secondValue)
{
    auto transWithProtoHandler = TransWithProtoHandler::Cast(secondValue.GetTaggedObject());
    auto transitionHClassVal = transWithProtoHandler->GetTransitionHClass(mainThread_);
    if (!transitionHClassVal.IsJSHClass()) {
        return ;
    }
    auto handlerInfoValue = transWithProtoHandler->GetHandlerInfo(mainThread_);
    auto transitionHClass = JSHClass::Cast(transitionHClassVal.GetTaggedObject());
    if (handlerInfoValue.IsInt()) {
        auto handlerInfo = static_cast<uint32_t>(handlerInfoValue.GetInt());
        if (HandlerBase::IsElement(handlerInfo)) {
            AddBuiltinsInfo(abcId, bcOffset, hclass, transitionHClass,
                            OnHeapMode::NONE, HandlerBase::IsStoreOutOfBounds(handlerInfo));
            return;
        }
    }
    AddObjectInfo(abcId, bcOffset, hclass, hclass, transitionHClass);
}

void JITProfiler::HandlePrototypeHandler(ApEntityId &abcId, int32_t &bcOffset,
                                         JSHClass *hclass, JSTaggedValue &secondValue)
{
    PrototypeHandler *prototypeHandler = PrototypeHandler::Cast(secondValue.GetTaggedObject());
    auto cellValue = prototypeHandler->GetProtoCell(mainThread_);
    if (!cellValue.IsProtoChangeMarker()) {
        return;
    }
    ASSERT(cellValue.IsProtoChangeMarker());
    ProtoChangeMarker *cell = ProtoChangeMarker::Cast(cellValue.GetTaggedObject());
    if (cell->GetHasChanged()) {
        return;
    }
    JSTaggedValue handlerInfoValue = prototypeHandler->GetHandlerInfo(mainThread_);
    if (handlerInfoValue.IsInt()) {
        auto handlerInfo = static_cast<uint32_t>(handlerInfoValue.GetInt());
        if (HandlerBase::IsElement(handlerInfo)) {
            AddBuiltinsInfo(abcId, bcOffset, hclass, hclass,
                            OnHeapMode::NONE, HandlerBase::IsStoreOutOfBounds(handlerInfo));
            return;
        }
    }
    auto holder = prototypeHandler->GetHolder(mainThread_);
    auto holderHClass = holder.GetTaggedObject()->GetClass();
    AddObjectInfo(abcId, bcOffset, hclass, holderHClass, holderHClass);
}

void JITProfiler::ConvertICByValueWithPoly(ApEntityId abcId, int32_t bcOffset,
                                           JSTaggedValue name,
                                           JSTaggedValue cacheValue,
                                           BCType type, uint32_t slotId)
{
    if (cacheValue.IsWeak()) {
        return;
    }
    // Check whether the cacheValue is TaggedArray
    if (!cacheValue.IsTaggedArray()) {
        return;
    }
    auto array = TaggedArray::Cast(cacheValue);
    uint32_t length = array->GetLength();
    for (uint32_t i = 0; i < length; i += 2) { // 2 means one ic, two slot
        auto result = array->Get(mainThread_, i);
        auto handler = array->Get(mainThread_, i + 1);
        if (!result.IsHeapObject() || !result.IsWeak()) {
            continue;
        }
        TaggedObject *object = result.GetWeakReferentUnChecked();
        if (!object->GetClass()->IsHClass()) {
            continue;
        }
        JSHClass *hclass = JSHClass::Cast(object);
        ConvertICByValueWithHandler(abcId, bcOffset, hclass, handler, type, slotId, name);
    }
}

void JITProfiler::ConvertExternalModuleVar(uint32_t index, uint32_t bcOffset)
{
    auto *jitCompilationEnv = static_cast<JitCompilationEnv*>(compilationEnv_);
    auto jsfunc = *jsFunction_;
    // check resolve
    if (jsfunc == nullptr) {
        return;
    }
    if (!ModuleManager::CheckModuleValueOutterResolved(mainThread_, index, jsfunc)) {
        return;
    }

    jitCompilationEnv->SetLdExtModuleVarResolved(methodId_.GetOffset(), bcOffset);
}

void JITProfiler::ConvertInstanceof(int32_t bcOffset, uint32_t slotId)
{
    JSTaggedValue firstValue = profileTypeInfo_->Get(mainThread_, slotId);
    if (!firstValue.IsHeapObject()) {
        if (firstValue.IsHole()) {
            // Mega state
            AddObjectInfoWithMega(bcOffset);
        }
        return;
    }
    if (firstValue.IsWeak()) {
        TaggedObject *object = firstValue.GetWeakReferentUnChecked();
        if (object->GetClass()->IsHClass()) {
            JSHClass *hclass = JSHClass::Cast(object);
            // Since pgo does not support symbol, we choose to return if hclass having @@hasInstance
            JSTaggedValue key = GetCurrentGlobalEnv()->GetHasInstanceSymbol().GetTaggedValue();
            JSHClass *functionPrototypeHC =
                JSObject::Cast(GetCurrentGlobalEnv()->GetFunctionPrototype().GetTaggedValue())->GetClass();
            JSTaggedValue foundHClass = TryFindKeyInPrototypeChain(object, hclass, key);
            if (!foundHClass.IsUndefined() && JSHClass::Cast(foundHClass.GetTaggedObject()) != functionPrototypeHC) {
                return;
            }
            AddObjectInfo(abcId_, bcOffset, hclass, hclass, hclass);
        }
        return;
    }
    // Poly Not Consider now
    return;
}

void JITProfiler::ConvertTryldGlobalByName(uint32_t bcOffset, uint32_t slotId)
{
    auto *jitCompilationEnv = static_cast<JitCompilationEnv*>(compilationEnv_);
    if (!jitCompilationEnv->SupportHeapConstant()) {
        return;
    }
    JSTaggedValue handler = profileTypeInfo_->Get(mainThread_, slotId);
    if (handler.IsHeapObject()) {
        ASSERT(handler.IsPropertyBox());
        PropertyBox *cell = PropertyBox::Cast(handler.GetTaggedObject());
        if (cell->IsInvalid(mainThread_) || cell->GetValue(mainThread_).IsAccessorData()) {
            return;
        }
        JSHandle<JSTaggedValue> boxHandle = jitCompilationEnv->NewJSHandle(handler);
        uint32_t heapConstantIndex = jitCompilationEnv->RecordHeapConstant(boxHandle);
        if (heapConstantIndex != JitCompilationEnv::INVALID_HEAP_CONSTANT_INDEX) {
            jitCompilationEnv->RecordLdGlobalByNameBcOffset2HeapConstantIndex(methodId_.GetOffset(),
                bcOffset, heapConstantIndex);
        }
    }
}

JSTaggedValue JITProfiler::TryFindKeyInPrototypeChain(TaggedObject *currObj, JSHClass *currHC, JSTaggedValue key)
{
    // This is a temporary solution for Instanceof Only!
    // Do NOT use this function for other purpose.
    if (currHC->IsDictionaryMode()) {
        return JSTaggedValue(currHC);
    }
    while (!JSTaggedValue(currHC).IsUndefinedOrNull()) {
        if (LIKELY(!currHC->IsDictionaryMode())) {
            int entry = JSHClass::FindPropertyEntry(mainThread_, currHC, key);
            if (entry != -1) {
                return JSTaggedValue(currHC);
            }
        } else {
            TaggedArray *array =
                TaggedArray::Cast(JSObject::Cast(currObj)->GetProperties(mainThread_).GetTaggedObject());
            ASSERT(array->IsDictionaryMode());
            NameDictionary *dict = NameDictionary::Cast(array);
            int entry = dict->FindEntry(mainThread_, key);
            if (entry != -1) {
                return JSTaggedValue(currHC);
            }
        }
        auto proto = currHC->GetProto(mainThread_);
        if (!proto.IsHeapObject()) {
            return JSTaggedValue::Undefined();
        }
        currObj = proto.GetTaggedObject();
        if (JSTaggedValue(currObj).IsUndefinedOrNull()) {
            break;
        }
        currHC = currObj->GetClass();
    }
    return JSTaggedValue::Undefined();
}

void JITProfiler::AddObjectInfoWithMega(int32_t bcOffset)
{
    auto megaType = ProfileType::CreateMegaType();
    PGOObjectInfo info(megaType, megaType, megaType, megaType, megaType, megaType, PGOSampleType());
    AddObjectInfoImplement(bcOffset, info);
}

void JITProfiler::AddObjectInfoImplement(int32_t bcOffset, const PGOObjectInfo &info, JSTaggedValue name)
{
    PGORWOpType *cur = nullptr;
    if (bcOffsetPGORwTypeMap_.find(bcOffset) == bcOffsetPGORwTypeMap_.end()) {
        cur = chunk_->New<PGORWOpType>();
        bcOffsetPGORwTypeMap_[bcOffset] = cur;
    } else {
        cur = const_cast<PGORWOpType*>(bcOffsetPGORwTypeMap_.at(bcOffset));
    }
    if (cur != nullptr) {
        auto *jitCompilationEnv = static_cast<JitCompilationEnv*>(compilationEnv_);
        if (name != JSTaggedValue::Undefined() && compilationEnv_->SupportHeapConstant()) {
            JSHandle<JSTaggedValue> nameHandle = jitCompilationEnv->NewJSHandle(name);
            auto nameConstantIndex = jitCompilationEnv->RecordHeapConstant(nameHandle);
            cur->SetName(nameHandle);
            cur->SetNameIdx(nameConstantIndex);
        }
        cur->AddObjectInfo(info);
    }
}

bool JITProfiler::AddObjectInfo(ApEntityId abcId, int32_t bcOffset, JSHClass *receiver, JSHClass *hold,
    JSHClass *holdTra, uint32_t accessorMethodId, PrimitiveType primitiveType, JSTaggedValue name)
{
    PGOSampleType accessor = PGOSampleType::CreateProfileType(abcId, accessorMethodId, ProfileType::Kind::MethodId);
    // case: obj = Object.create(null) => LowerProtoChangeMarkerCheck Crash
    if (receiver->GetPrototype(mainThread_).IsNull()) {
        return false;
    }
    return AddTranstionObjectInfo(bcOffset, receiver, hold, holdTra, accessor, primitiveType, name);
}

bool JITProfiler::AddTranstionObjectInfo(int32_t bcOffset, JSHClass *receiver, JSHClass *hold,
    JSHClass *holdTra, PGOSampleType accessorMethod, PrimitiveType primitiveType, JSTaggedValue name)
{
    ptManager_->RecordAndGetHclassIndexForJIT(receiver);
    ptManager_->RecordAndGetHclassIndexForJIT(hold);
    ptManager_->RecordAndGetHclassIndexForJIT(holdTra);
    PGOObjectInfo info(ProfileType::CreateJITType(), receiver, hold, holdTra, accessorMethod, primitiveType);
    AddObjectInfoImplement(bcOffset, info, name);
    return true;
}

bool JITProfiler::IsTypedArrayRootHClass(JSType jsType, OnHeapMode mode, JSHClass *receiver)
{
    // The onHeap tag is used to describe the location where array data is stored.
    // When the target of load/store is not an element, onHeap is none.
    // At this point, it is necessary to query the hclass in two lists.
    if (OnHeap::IsOnHeap(mode) || OnHeap::IsNone(mode)) {
        JSHClass* rootHClass = GetCurrentGlobalEnv()->GetBuildinTypedArrayHClassByJSType(jsType, OnHeapMode::ON_HEAP);
        if (rootHClass != nullptr && rootHClass == receiver) {
            return true;
        }
    }
    if (OnHeap::IsNotOnHeap(mode) || OnHeap::IsNone(mode)) {
        JSHClass* rootHClass =
            GetCurrentGlobalEnv()->GetBuildinTypedArrayHClassByJSType(jsType, OnHeapMode::NOT_ON_HEAP);
        if (rootHClass != nullptr && rootHClass == receiver) {
            return true;
        }
    }
    return false;
}

void JITProfiler::AddBuiltinsInfo(ApEntityId abcId, int32_t bcOffset, JSHClass *receiver,
    JSHClass *transitionHClass, OnHeapMode onHeap, bool everOutOfBounds)
{
    if (receiver->IsJSArray()) {
        auto type = receiver->GetObjectType();
        auto elementsKind = receiver->GetElementsKind();
        auto transitionElementsKind = transitionHClass->GetElementsKind();
        auto profileType = ProfileType::CreateBuiltinsArray(abcId, type, elementsKind, transitionElementsKind,
                                                            everOutOfBounds);
        PGOObjectInfo info(profileType, receiver);
        AddObjectInfoImplement(bcOffset, info);
    } else if (receiver->IsTypedArray()) {
        JSType jsType = receiver->GetObjectType();
        auto profileType = IsTypedArrayRootHClass(jsType, onHeap, receiver) ?
                           ProfileType::CreateBuiltinsTypedArray(abcId, jsType, onHeap, everOutOfBounds) :
                           ProfileType::CreateInvalid(abcId);
        PGOObjectInfo info(profileType, receiver);
        AddObjectInfoImplement(bcOffset, info);
    } else {
        auto type = receiver->GetObjectType();
        PGOObjectInfo info(ProfileType::CreateBuiltins(abcId, type), receiver);
        AddObjectInfoImplement(bcOffset, info);
    }
}

void JITProfiler::AddBuiltinsGlobalInfo(ApEntityId abcId, int32_t bcOffset, GlobalIndex globalsId)
{
    PGOObjectInfo info(ProfileType::CreateGlobals(abcId, globalsId));
    AddObjectInfoImplement(bcOffset, info);
}

bool JITProfiler::AddBuiltinsInfoByNameInInstance(ApEntityId abcId, int32_t bcOffset, JSHClass *receiver)
{
    auto type = receiver->GetObjectType();
    const auto &ctorEntries = mainThread_->GetCtorHclassEntries();
    auto entry = ctorEntries.find(receiver);
    if (entry != ctorEntries.end()) {
        AddBuiltinsGlobalInfo(abcId, bcOffset, entry->second);
        return true;
    }

    auto builtinsId = ToBuiltinsTypeId(type);
    if (!builtinsId.has_value()) {
        return false;
    }
    JSHClass *exceptRecvHClass = nullptr;
    if (builtinsId == BuiltinTypeId::ARRAY) {
        bool receiverIsPrototype = receiver->IsPrototype();
        exceptRecvHClass = mainThread_->GetArrayInstanceHClass(GetCurrentGlobalEnv(),
                                                               receiver->GetElementsKind(),
                                                               receiverIsPrototype);
    } else if (builtinsId == BuiltinTypeId::STRING) {
        exceptRecvHClass = receiver;
    } else {
        exceptRecvHClass = mainThread_->GetBuiltinInstanceHClass(builtinsId.value());
    }

    if (exceptRecvHClass != receiver) {
        // When JSType cannot uniquely identify builtins object, it is necessary to
        // query the receiver on the global constants.
        if (builtinsId == BuiltinTypeId::OBJECT) {
            exceptRecvHClass =
                JSHClass::Cast(GetCurrentGlobalEnv()->GetIteratorResultClass().GetTaggedValue().GetTaggedObject());
            if (exceptRecvHClass == receiver) {
                GlobalIndex globalsId;
                globalsId.UpdateGlobalEnvId(static_cast<size_t>(GlobalEnvField::ITERATOR_RESULT_CLASS_INDEX));
                AddBuiltinsGlobalInfo(abcId, bcOffset, globalsId);
                return true;
            }
            return false;
        }
        return true;
    }
    AddBuiltinsInfo(abcId, bcOffset, receiver, receiver);
    return true;
}

bool JITProfiler::AddBuiltinsInfoByNameInProt(ApEntityId abcId, int32_t bcOffset,
                                              JSHClass *receiver, JSHClass *hold, bool isNonExist)
{
    auto type = receiver->GetObjectType();
    auto builtinsId = ToBuiltinsTypeId(type);
    if (!builtinsId.has_value()) {
        return false;
    }
    // Not support string not found ic now.
    if (isNonExist) {
        if (builtinsId == BuiltinTypeId::STRING) {
            return true;
        }
        return false;
    }
    JSHClass *exceptRecvHClass = nullptr;
    if (builtinsId == BuiltinTypeId::ARRAY) {
        bool receiverIsPrototype = receiver->IsPrototype();
        exceptRecvHClass = mainThread_->GetArrayInstanceHClass(GetCurrentGlobalEnv(),
                                                               receiver->GetElementsKind(),
                                                               receiverIsPrototype);
    } else if (builtinsId == BuiltinTypeId::STRING) {
        exceptRecvHClass = receiver;
    } else {
        exceptRecvHClass = mainThread_->GetBuiltinInstanceHClass(builtinsId.value());
    }

    auto exceptHoldHClass = mainThread_->GetBuiltinPrototypeHClass(builtinsId.value());
    auto exceptPrototypeOfPrototypeHClass =
        mainThread_->GetBuiltinPrototypeOfPrototypeHClass(builtinsId.value());
    // iterator needs to find two layers of prototype
    if (builtinsId == BuiltinTypeId::ARRAY_ITERATOR) {
        if ((exceptRecvHClass != receiver) ||
            (exceptHoldHClass != hold && exceptPrototypeOfPrototypeHClass != hold)) {
            return true;
        }
    } else if (IsTypedArrayType(builtinsId.value())) {
        auto exceptRecvHClassOnHeap = mainThread_->GetBuiltinExtraHClass(builtinsId.value());
        ASSERT_PRINT(exceptRecvHClassOnHeap == nullptr || exceptRecvHClassOnHeap->IsOnHeapFromBitField(),
                     "must be on heap");
        if (JITProfiler::IsJSHClassNotEqual(receiver, hold, exceptRecvHClass, exceptRecvHClassOnHeap, exceptHoldHClass,
                                            exceptPrototypeOfPrototypeHClass)) {
            return true;
        }
    } else if (exceptRecvHClass != receiver || exceptHoldHClass != hold) {
        if (builtinsId == BuiltinTypeId::OBJECT) {
            return false;
        } else {
            return true;
        }
    }
    AddBuiltinsInfo(abcId, bcOffset, receiver, receiver);
    return true;
}

bool JITProfiler::IsJSHClassNotEqual(JSHClass *receiver, JSHClass *hold, JSHClass *exceptRecvHClass,
                                     JSHClass *exceptRecvHClassOnHeap, JSHClass *exceptHoldHClass,
                                     JSHClass *exceptPrototypeOfPrototypeHClass)
{
    //exceptRecvHClass = IHC, exceptRecvHClassOnHeap = IHC OnHeap
    //exceptHoldHClass = PHC, exceptPrototypeOfPrototypeHClass = HClass of X.prototype.prototype
    return ((exceptRecvHClass != receiver && exceptRecvHClassOnHeap != receiver) ||
            (exceptHoldHClass != hold && exceptPrototypeOfPrototypeHClass != hold));
}

bool JITProfiler::IsIncompleteProfileTypeInfo()
{
    if (profileTypeInfo_.GetTaggedType() == 0 || profileTypeInfo_.GetTaggedValue().IsUndefined()) {
        return true;
    }
    // We may receive an incomplete profile typeinfo. During the execution of a larger function, when the upper part of
    // the function is executed, profiltypeinfo has not yet been created. When profiltypeinfo is created and Jit is
    // triggered, the first half of profiltypeinfo becomes empty.
    return profileTypeInfo_->Get(mainThread_, 0).IsUndefined();
}

bool JITProfiler::SlotValueIsUndefined(uint32_t slotId)
{
    return profileTypeInfo_->Get(mainThread_, slotId).IsUndefined();
}

void JITProfiler::UpdateBcOffsetBool(uint32_t offset, uint32_t slotId)
{
    if (IsIncompleteProfileTypeInfo()) {
        return;
    }
    SetBcOffsetBool(offset, SlotValueIsUndefined(slotId));
}

void JITProfiler::UpdateBcOffsetBoolWithNearSlotId(uint32_t offset, uint32_t slotId)
{
    if (IsIncompleteProfileTypeInfo()) {
        return;
    }
    bool isInsufficientPGO = SlotValueIsUndefined(slotId) && SlotValueIsUndefined(slotId + 1);
    SetBcOffsetBool(offset, isInsufficientPGO);
}

void JITProfiler::Clear()
{
    bcOffsetPGOOpTypeMap_.clear();
    bcOffsetPGODefOpTypeMap_.clear();
    bcOffsetPGORwTypeMap_.clear();
    bcOffsetBoolMap_.clear();
    abcId_ = 0;
    methodId_ = (EntityId)0;
}


JITProfiler::~JITProfiler()
{
}
}
