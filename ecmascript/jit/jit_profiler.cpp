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

#include <chrono>
#include <cstdint>
#include <memory>

#include "ecmascript/compiler/codegen/maple/maple_util/include/profile_type.h"
#include "ecmascript/compiler/jit_compilation_env.h"
#include "ecmascript/compiler/pgo_type/pgo_type_manager.h"
#include "ecmascript/elements.h"
#include "ecmascript/enum_conversion.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/patch/patch_loader.h"
#include "ecmascript/pgo_profiler/pgo_context.h"
#include "ecmascript/pgo_profiler/pgo_profiler_info.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/pgo_profiler/types/pgo_profile_type.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"
#include "ecmascript/pgo_profiler/types/pgo_type_generator.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "macros.h"

namespace panda::ecmascript {
using namespace pgo;
JITProfiler::JITProfiler(EcmaVM *vm) : vm_(vm)
{
}

void JITProfiler::ProfileBytecode(JSThread *thread, const JSHandle<ProfileTypeInfo> &profileTypeInfo,
                                  ProfileTypeInfo *rawProfileTypeInfo,
                                  EntityId methodId, ApEntityId abcId, const uint8_t *pcStart, uint32_t codeSize,
                                  const panda_file::File::Header *header, bool useRawProfileTypeInfo)
{
    Clear();
    if (useRawProfileTypeInfo) {
        profileTypeInfo_ = rawProfileTypeInfo;
    }
    abcId_ = abcId;
    methodId_ = methodId;
    BytecodeInstruction bcIns(pcStart);
    auto bcInsLast = bcIns.JumpTo(codeSize);

    while (bcIns.GetAddress() != bcInsLast.GetAddress()) {
        auto opcode = bcIns.GetOpcode();
        auto bcOffset = bcIns.GetAddress() - pcStart;
        auto pc = bcIns.GetAddress();
        switch (opcode) {
            case EcmaOpcode::LDTHISBYNAME_IMM8_ID16:
            case EcmaOpcode::LDOBJBYNAME_IMM8_ID16: {
                Jit::JitLockHolder lock(thread);
                if (!useRawProfileTypeInfo) {
                    profileTypeInfo_ = *profileTypeInfo;
                }
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                ConvertICByName(bcOffset, slotId, BCType::LOAD);
                break;
            }
            case EcmaOpcode::LDTHISBYNAME_IMM16_ID16:
            case EcmaOpcode::LDOBJBYNAME_IMM16_ID16: {
                Jit::JitLockHolder lock(thread);
                if (!useRawProfileTypeInfo) {
                    profileTypeInfo_ = *profileTypeInfo;
                }
                uint16_t slotId = READ_INST_16_0();
                ConvertICByName(bcOffset, slotId, BCType::LOAD);
                break;
            }
            case EcmaOpcode::LDOBJBYVALUE_IMM8_V8:
            case EcmaOpcode::LDTHISBYVALUE_IMM8: {
                Jit::JitLockHolder lock(thread);
                if (!useRawProfileTypeInfo) {
                    profileTypeInfo_ = *profileTypeInfo;
                }
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                ConvertICByValue(bcOffset, slotId, BCType::LOAD);
                break;
            }
            case EcmaOpcode::LDOBJBYVALUE_IMM16_V8:
            case EcmaOpcode::LDTHISBYVALUE_IMM16: {
                Jit::JitLockHolder lock(thread);
                if (!useRawProfileTypeInfo) {
                    profileTypeInfo_ = *profileTypeInfo;
                }
                uint16_t slotId = READ_INST_16_0();
                ConvertICByValue(bcOffset, slotId, BCType::LOAD);
                break;
            }
            case EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8:
            case EcmaOpcode::STTHISBYNAME_IMM8_ID16: {
                Jit::JitLockHolder lock(thread);
                if (!useRawProfileTypeInfo) {
                    profileTypeInfo_ = *profileTypeInfo;
                }
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                ConvertICByName(bcOffset, slotId, BCType::STORE);
                break;
            }
            case EcmaOpcode::STOBJBYNAME_IMM16_ID16_V8:
            case EcmaOpcode::STTHISBYNAME_IMM16_ID16: {
                Jit::JitLockHolder lock(thread);
                if (!useRawProfileTypeInfo) {
                    profileTypeInfo_ = *profileTypeInfo;
                }
                uint16_t slotId = READ_INST_16_0();
                ConvertICByName(bcOffset, slotId, BCType::STORE);
                break;
            }
            case EcmaOpcode::STOBJBYVALUE_IMM8_V8_V8:
            case EcmaOpcode::STOWNBYINDEX_IMM8_V8_IMM16:
            case EcmaOpcode::STTHISBYVALUE_IMM8_V8: {
                Jit::JitLockHolder lock(thread);
                if (!useRawProfileTypeInfo) {
                    profileTypeInfo_ = *profileTypeInfo;
                }
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                ConvertICByValue(bcOffset, slotId, BCType::STORE);
                break;
            }
            case EcmaOpcode::STOBJBYVALUE_IMM16_V8_V8:
            case EcmaOpcode::STOWNBYINDEX_IMM16_V8_IMM16:
            case EcmaOpcode::STTHISBYVALUE_IMM16_V8: {
                Jit::JitLockHolder lock(thread);
                if (!useRawProfileTypeInfo) {
                    profileTypeInfo_ = *profileTypeInfo;
                }
                uint16_t slotId = READ_INST_16_0();
                ConvertICByValue(bcOffset, slotId, BCType::STORE);
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
            case EcmaOpcode::EXP_IMM8_V8:
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
            case EcmaOpcode::STRICTEQ_IMM8_V8:
            case EcmaOpcode::TONUMERIC_IMM8: {
                Jit::JitLockHolder lock(thread);
                if (!useRawProfileTypeInfo) {
                    profileTypeInfo_ = *profileTypeInfo;
                }
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                ConvertOpType(slotId, bcOffset);
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
            case EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8: {
                Jit::JitLockHolder lock(thread);
                if (!useRawProfileTypeInfo) {
                    profileTypeInfo_ = *profileTypeInfo;
                }
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                ConvertCall(slotId, bcOffset);
                break;
            }
            case EcmaOpcode::CALLRUNTIME_CALLINIT_PREF_IMM8_V8: {
                Jit::JitLockHolder lock(thread);
                if (!useRawProfileTypeInfo) {
                    profileTypeInfo_ = *profileTypeInfo;
                }
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
                if (!useRawProfileTypeInfo) {
                    profileTypeInfo_ = *profileTypeInfo;
                }
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                ConvertNewObjRange(slotId, bcOffset);
                break;
            }
            case EcmaOpcode::NEWOBJRANGE_IMM16_IMM8_V8: {
                Jit::JitLockHolder lock(thread);
                if (!useRawProfileTypeInfo) {
                    profileTypeInfo_ = *profileTypeInfo;
                }
                uint16_t slotId = READ_INST_16_0();
                ConvertNewObjRange(slotId, bcOffset);
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
                if (!useRawProfileTypeInfo) {
                    profileTypeInfo_ = *profileTypeInfo;
                }
                auto traceId =
                    static_cast<int32_t>(reinterpret_cast<uintptr_t>(pc) - reinterpret_cast<uintptr_t>(header));
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                ConvertCreateObject(slotId, bcOffset, traceId);
                break;
            }
            case EcmaOpcode::CREATEOBJECTWITHBUFFER_IMM16_ID16:
            case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM16_ID16:
            case EcmaOpcode::CREATEEMPTYARRAY_IMM16: {
                Jit::JitLockHolder lock(thread);
                if (!useRawProfileTypeInfo) {
                    profileTypeInfo_ = *profileTypeInfo;
                }
                auto traceId =
                    static_cast<int32_t>(reinterpret_cast<uintptr_t>(pc) - reinterpret_cast<uintptr_t>(header));
                uint16_t slotId = READ_INST_16_0();
                ConvertCreateObject(slotId, bcOffset, traceId);
                break;
            }
            case EcmaOpcode::GETITERATOR_IMM8: {
                Jit::JitLockHolder lock(thread);
                if (!useRawProfileTypeInfo) {
                    profileTypeInfo_ = *profileTypeInfo;
                }
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                ConvertGetIterator(slotId, bcOffset);
                break;
            }
            case EcmaOpcode::GETITERATOR_IMM16: {
                Jit::JitLockHolder lock(thread);
                if (!useRawProfileTypeInfo) {
                    profileTypeInfo_ = *profileTypeInfo;
                }
                uint16_t slotId = READ_INST_16_0();
                ConvertGetIterator(slotId, bcOffset);
                break;
            }
            // Others
            case EcmaOpcode::INSTANCEOF_IMM8_V8: {
                Jit::JitLockHolder lock(thread);
                if (!useRawProfileTypeInfo) {
                    profileTypeInfo_ = *profileTypeInfo;
                }
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
    JSTaggedValue slotValue = profileTypeInfo_->Get(slotId);
    if (slotValue.IsInt()) {
        auto type = slotValue.GetInt();
        UpdatePGOType(bcOffset, new PGOSampleType(type));
    }
}

void JITProfiler::ConvertCall(uint32_t slotId, long bcOffset)
{
    JSTaggedValue slotValue = profileTypeInfo_->Get(slotId);
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
    }  else if (slotValue.IsJSFunction()) {
        JSFunction *callee = JSFunction::Cast(slotValue);
        Method *calleeMethod = Method::Cast(callee->GetMethod());
        calleeMethodId = calleeMethod->GetMethodId().GetOffset();
        calleeAbcId = PGOProfiler::GetMethodAbcId(callee);
        static_cast<JitCompilationEnv *>(compilationEnv_)
            ->UpdateFuncSlotIdMap(calleeMethodId, methodId_.GetOffset(), slotId);
        kind = ProfileType::Kind::MethodId;
    } else {
        return;
    }
    PGOSampleType* type = new PGOSampleType(ProfileType(abcId_, std::abs(calleeMethodId), kind));
    UpdatePGOType(bcOffset, type);
}

void JITProfiler::ConvertNewObjRange(uint32_t slotId, long bcOffset)
{
    JSTaggedValue slotValue = profileTypeInfo_->Get(slotId);
    int ctorMethodId = 0;
    if (slotValue.IsInt()) {
        ctorMethodId = slotValue.GetInt();
    } else if (slotValue.IsJSFunction()) {
        JSFunction *callee = JSFunction::Cast(slotValue);
        Method *calleeMethod = Method::Cast(callee->GetMethod());
        ctorMethodId = static_cast<int>(calleeMethod->GetMethodId().GetOffset());
    } else {
        return;
    }
    if (ctorMethodId > 0) {
        auto type = new PGOSampleType(ProfileType(abcId_, ctorMethodId, ProfileType::Kind::ClassId, true));
        UpdatePGOType(bcOffset, type);
    }
}

void JITProfiler::ConvertGetIterator(uint32_t slotId, long bcOffset)
{
    if (vm_->GetJSThread()->GetEnableLazyBuiltins()) {
        return;
    }
    JSTaggedValue value = profileTypeInfo_->Get(slotId);
    if (!value.IsInt()) {
        return;
    }
    int iterKind = value.GetInt();
    ASSERT(iterKind <= 0);
    ProfileType::Kind pgoKind = ProfileType::Kind::BuiltinFunctionId;
    auto type = new PGOSampleType(ProfileType(abcId_, std::abs(iterKind), pgoKind));
    UpdatePGOType(bcOffset, type);
}

void JITProfiler::ConvertCreateObject(uint32_t slotId, long bcOffset, int32_t traceId)
{
    JSTaggedValue slotValue = profileTypeInfo_->Get(slotId);
    if (!slotValue.IsHeapObject()) {
        return;
    }
    if (slotValue.IsWeak()) {
        auto object = slotValue.GetWeakReferentUnChecked();
        if (object->GetClass()->IsHClass()) {
            auto newHClass = JSHClass::Cast(object);
            auto rootHClass = JSHClass::FindRootHClass(newHClass);
            ASSERT(rootHClass->IsJSObject());
            auto profileType = GetProfileType(JSTaggedType(rootHClass), JSTaggedType(rootHClass));
            if (profileType.IsNone()) {
                return;
            }
            ASSERT(profileType.GetKind() == ProfileType::Kind::LiteralId);
            PGODefineOpType* objDefType = new PGODefineOpType(profileType);
            UpdatePGOType(bcOffset, objDefType);
        }
    } else if (slotValue.IsTrackInfoObject()) {
        auto currentType = PGOSampleType::CreateProfileType(abcId_, traceId, ProfileType::Kind::LiteralId, true);
        auto profileType = currentType.GetProfileType();
        PGODefineOpType* objDefType = new PGODefineOpType(profileType);
        TrackInfo *trackInfo = TrackInfo::Cast(slotValue.GetTaggedObject());
        auto elementsKind = trackInfo->GetElementsKind();
        objDefType->SetElementsKind(elementsKind);
        objDefType->SetElementsLength(trackInfo->GetArrayLength());
        objDefType->SetSpaceFlag(trackInfo->GetSpaceFlag());
        UpdatePGOType(bcOffset, objDefType);
    }
}

void JITProfiler::ConvertICByName(int32_t bcOffset, uint32_t slotId, BCType type)
{
    ProfileTypeAccessorLockScope accessorLockScope(vm_->GetJSThread());
    JSTaggedValue firstValue = profileTypeInfo_->Get(slotId);
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
            JSTaggedValue secondValue = profileTypeInfo_->Get(slotId + 1);
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
    if (HandlerBase::IsNonExist(handlerInfo)) {
        return;
    }
    if (HandlerBase::IsField(handlerInfo) || HandlerBase::IsAccessor(handlerInfo)) {
        AddObjectInfo(abcId, bcOffset, hclass, hclass, hclass);
    }
    AddBuiltinsInfoByNameInInstance(abcId, bcOffset, hclass);
}

void JITProfiler::HandleLoadTypePrototypeHandler(ApEntityId &abcId, int32_t &bcOffset,
                                                 JSHClass *hclass, JSTaggedValue &secondValue, uint32_t slotId)
{
    auto prototypeHandler = PrototypeHandler::Cast(secondValue.GetTaggedObject());
    auto cellValue = prototypeHandler->GetProtoCell();
    if (cellValue.IsUndefined()) {
        return;
    }
    ProtoChangeMarker *cell = ProtoChangeMarker::Cast(cellValue.GetTaggedObject());
    if (cell->GetHasChanged()) {
        return;
    }
    auto holder = prototypeHandler->GetHolder();
    auto holderHClass = holder.GetTaggedObject()->GetClass();
    JSTaggedValue handlerInfoVal = prototypeHandler->GetHandlerInfo();
    if (!handlerInfoVal.IsInt()) {
        return;
    }
    auto handlerInfo = static_cast<uint32_t>(handlerInfoVal.GetInt());
    if (HandlerBase::IsNonExist(handlerInfo)) {
        return;
    }
    auto accessorMethodId = prototypeHandler->GetAccessorMethodId();
    auto accessor = prototypeHandler->GetAccessorJSFunction();
    if (accessor.IsJSFunction()) {
        auto accessorFunction = JSFunction::Cast(accessor);
        auto methodId = Method::Cast(accessorFunction->GetMethod())->GetMethodId().GetOffset();
        ASSERT(accessorMethodId == methodId);
        accessorMethodId = methodId;
        static_cast<JitCompilationEnv *>(compilationEnv_)
            ->UpdateFuncSlotIdMap(accessorMethodId, methodId_.GetOffset(), slotId);
    }
    if (!AddObjectInfo(abcId, bcOffset, hclass, holderHClass, holderHClass, accessorMethodId)) {
        return AddBuiltinsInfoByNameInProt(abcId, bcOffset, hclass, holderHClass);
    }
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
    } else if (secondValue.IsStoreTSHandler()) {
        HandleStoreTSHandler(abcId, bcOffset, hclass, secondValue);
    }
}

void JITProfiler::HandleTransitionHandler(ApEntityId &abcId, int32_t &bcOffset,
                                          JSHClass *hclass, JSTaggedValue &secondValue)
{
    auto transitionHandler = TransitionHandler::Cast(secondValue.GetTaggedObject());
    auto transitionHClassVal = transitionHandler->GetTransitionHClass();
    if (transitionHClassVal.IsJSHClass()) {
        auto transitionHClass = JSHClass::Cast(transitionHClassVal.GetTaggedObject());
        AddObjectInfo(abcId, bcOffset, hclass, hclass, transitionHClass);
    }
}

void JITProfiler::HandleTransWithProtoHandler(ApEntityId &abcId, int32_t &bcOffset,
                                              JSHClass *hclass, JSTaggedValue &secondValue)
{
    auto transWithProtoHandler = TransWithProtoHandler::Cast(secondValue.GetTaggedObject());
    auto cellValue = transWithProtoHandler->GetProtoCell();
    ASSERT(cellValue.IsProtoChangeMarker());
    ProtoChangeMarker *cell = ProtoChangeMarker::Cast(cellValue.GetTaggedObject());
    if (cell->GetHasChanged()) {
        return;
    }
    auto transitionHClassVal = transWithProtoHandler->GetTransitionHClass();
    if (transitionHClassVal.IsJSHClass()) {
        auto transitionHClass = JSHClass::Cast(transitionHClassVal.GetTaggedObject());
        AddObjectInfo(abcId, bcOffset, hclass, hclass, transitionHClass);
    }
}

void JITProfiler::HandleOtherTypesPrototypeHandler(ApEntityId &abcId, int32_t &bcOffset,
                                                   JSHClass *hclass, JSTaggedValue &secondValue, uint32_t slotId)
{
    auto prototypeHandler = PrototypeHandler::Cast(secondValue.GetTaggedObject());
    auto cellValue = prototypeHandler->GetProtoCell();
    if (cellValue.IsUndefined()) {
        return;
    }
    ProtoChangeMarker *cell = ProtoChangeMarker::Cast(cellValue.GetTaggedObject());
    if (cell->GetHasChanged()) {
        return;
    }
    auto holder = prototypeHandler->GetHolder();
    auto holderHClass = holder.GetTaggedObject()->GetClass();
    auto accessorMethodId = prototypeHandler->GetAccessorMethodId();
    auto accessor = prototypeHandler->GetAccessorJSFunction();
    if (accessor.IsJSFunction()) {
        auto accessorFunction = JSFunction::Cast(accessor);
        auto methodId = Method::Cast(accessorFunction->GetMethod())->GetMethodId().GetOffset();
        ASSERT(accessorMethodId == methodId);
        accessorMethodId = methodId;
        static_cast<JitCompilationEnv *>(compilationEnv_)
            ->UpdateFuncSlotIdMap(accessorMethodId, methodId_.GetOffset(), slotId);
    }
    AddObjectInfo(abcId, bcOffset, hclass, holderHClass, holderHClass, accessorMethodId);
}

void JITProfiler::HandleStoreTSHandler(ApEntityId &abcId, int32_t &bcOffset,
                                       JSHClass *hclass, JSTaggedValue &secondValue)
{
    StoreTSHandler *storeTSHandler = StoreTSHandler::Cast(secondValue.GetTaggedObject());
    auto cellValue = storeTSHandler->GetProtoCell();
    ASSERT(cellValue.IsProtoChangeMarker());
    ProtoChangeMarker *cell = ProtoChangeMarker::Cast(cellValue.GetTaggedObject());
    if (cell->GetHasChanged()) {
        return;
    }
    auto holder = storeTSHandler->GetHolder();
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
        auto result = array->Get(i);
        auto handler = array->Get(i + 1);
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
    ProfileTypeAccessorLockScope accessorLockScope(vm_->GetJSThread());
    JSTaggedValue firstValue = profileTypeInfo_->Get(slotId);
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
            JSTaggedValue secondValue = profileTypeInfo_->Get(slotId + 1);
            JSHClass *hclass = JSHClass::Cast(object);
            ConvertICByValueWithHandler(abcId_, bcOffset, hclass, secondValue, type);
        }
        return;
    }
    // Check key
    if ((firstValue.IsString() || firstValue.IsSymbol())) {
        return;
    }
    // Check without key
    ConvertICByValueWithPoly(abcId_, bcOffset, firstValue, type);
}

void JITProfiler::ConvertICByValueWithHandler(ApEntityId abcId, int32_t bcOffset,
                                              JSHClass *hclass, JSTaggedValue secondValue,
                                              BCType type)
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
                OnHeapMode onHeap = HandlerBase::IsOnHeap(handlerInfo) ? OnHeapMode::ON_HEAP : OnHeapMode::NOT_ON_HEAP;
                AddBuiltinsInfo(abcId, bcOffset, hclass, hclass, onHeap);
                return;
            }
            AddObjectInfo(abcId, bcOffset, hclass, hclass, hclass);
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
    } else {
        HandlePrototypeHandler(abcId, bcOffset, hclass, secondValue);
    }
}

void JITProfiler::HandleTransition(ApEntityId &abcId, int32_t &bcOffset,
                                   JSHClass *hclass, JSTaggedValue &secondValue)
{
    auto transitionHandler = TransitionHandler::Cast(secondValue.GetTaggedObject());
    auto transitionHClassVal = transitionHandler->GetTransitionHClass();

    auto handlerInfoValue = transitionHandler->GetHandlerInfo();
    ASSERT(handlerInfoValue.IsInt());
    auto handlerInfo = static_cast<uint32_t>(handlerInfoValue.GetInt());
    if (transitionHClassVal.IsJSHClass()) {
        auto transitionHClass = JSHClass::Cast(transitionHClassVal.GetTaggedObject());
        if (HandlerBase::IsElement(handlerInfo)) {
            AddBuiltinsInfo(abcId, bcOffset, hclass, transitionHClass,
                            OnHeapMode::NONE, HandlerBase::IsStoreOutOfBounds(handlerInfo));
        } else {
            AddObjectInfo(abcId, bcOffset, hclass, hclass, transitionHClass);
        }
    }
}

void JITProfiler::HandleTransWithProto(ApEntityId &abcId, int32_t &bcOffset,
                                       JSHClass *hclass, JSTaggedValue &secondValue)
{
    auto transWithProtoHandler = TransWithProtoHandler::Cast(secondValue.GetTaggedObject());
    auto transitionHClassVal = transWithProtoHandler->GetTransitionHClass();
    auto handlerInfoValue = transWithProtoHandler->GetHandlerInfo();
    ASSERT(handlerInfoValue.IsInt());
    auto handlerInfo = static_cast<uint32_t>(handlerInfoValue.GetInt());
    if (transitionHClassVal.IsJSHClass()) {
        auto transitionHClass = JSHClass::Cast(transitionHClassVal.GetTaggedObject());
        if (HandlerBase::IsElement(handlerInfo)) {
            AddBuiltinsInfo(abcId, bcOffset, hclass, transitionHClass,
                            OnHeapMode::NONE, HandlerBase::IsStoreOutOfBounds(handlerInfo));
        } else {
            AddObjectInfo(abcId, bcOffset, hclass, hclass, transitionHClass);
        }
    }
}

void JITProfiler::HandlePrototypeHandler(ApEntityId &abcId, int32_t &bcOffset,
                                         JSHClass *hclass, JSTaggedValue &secondValue)
{
    ASSERT(secondValue.IsPrototypeHandler());
    PrototypeHandler *prototypeHandler = PrototypeHandler::Cast(secondValue.GetTaggedObject());
    auto cellValue = prototypeHandler->GetProtoCell();
    if (!cellValue.IsProtoChangeMarker()) {
        return;
    }
    ASSERT(cellValue.IsProtoChangeMarker());
    ProtoChangeMarker *cell = ProtoChangeMarker::Cast(cellValue.GetTaggedObject());
    if (cell->GetHasChanged()) {
        return;
    }
    JSTaggedValue handlerInfoValue = prototypeHandler->GetHandlerInfo();
    ASSERT(handlerInfoValue.IsInt());
    auto handlerInfo = static_cast<uint32_t>(handlerInfoValue.GetInt());
    if (HandlerBase::IsElement(handlerInfo)) {
        AddBuiltinsInfo(abcId, bcOffset, hclass, hclass,
                        OnHeapMode::NONE, HandlerBase::IsStoreOutOfBounds(handlerInfo));
        return;
    }
    auto holder = prototypeHandler->GetHolder();
    auto holderHClass = holder.GetTaggedObject()->GetClass();
    AddObjectInfo(abcId, bcOffset, hclass, holderHClass, holderHClass);
}

void JITProfiler::ConvertICByValueWithPoly(ApEntityId abcId, int32_t bcOffset, JSTaggedValue cacheValue, BCType type)
{
    if (cacheValue.IsWeak()) {
        return;
    }
    ASSERT(cacheValue.IsTaggedArray());
    auto array = TaggedArray::Cast(cacheValue);
    uint32_t length = array->GetLength();
    for (uint32_t i = 0; i < length; i += 2) { // 2 means one ic, two slot
        auto result = array->Get(i);
        auto handler = array->Get(i + 1);
        if (!result.IsHeapObject() || !result.IsWeak()) {
            continue;
        }
        TaggedObject *object = result.GetWeakReferentUnChecked();
        if (!object->GetClass()->IsHClass()) {
            continue;
        }
        JSHClass *hclass = JSHClass::Cast(object);
        ConvertICByValueWithHandler(abcId, bcOffset, hclass, handler, type);
    }
}

void JITProfiler::ConvertInstanceof(int32_t bcOffset, uint32_t slotId)
{
    JSTaggedValue firstValue = profileTypeInfo_->Get(slotId);
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
            JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
            JSTaggedValue key = env->GetHasInstanceSymbol().GetTaggedValue();
            JSHClass *functionPrototypeHC = JSObject::Cast(env->GetFunctionPrototype().GetTaggedValue())->GetClass();
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

JSTaggedValue JITProfiler::TryFindKeyInPrototypeChain(TaggedObject *currObj, JSHClass *currHC, JSTaggedValue key)
{
    // This is a temporary solution for Instanceof Only!
    // Do NOT use this function for other purpose.
    if (currHC->IsDictionaryMode()) {
        return JSTaggedValue(currHC);
    }
    while (!JSTaggedValue(currHC).IsUndefinedOrNull()) {
        if (LIKELY(!currHC->IsDictionaryMode())) {
            int entry = JSHClass::FindPropertyEntry(vm_->GetJSThread(), currHC, key);
            if (entry != -1) {
                return JSTaggedValue(currHC);
            }
        } else {
            TaggedArray *array = TaggedArray::Cast(JSObject::Cast(currObj)->GetProperties().GetTaggedObject());
            ASSERT(array->IsDictionaryMode());
            NameDictionary *dict = NameDictionary::Cast(array);
            int entry = dict->FindEntry(key);
            if (entry != -1) {
                return JSTaggedValue(currHC);
            }
        }
        currObj = currHC->GetProto().GetTaggedObject();
        if (JSTaggedValue(currObj).IsUndefinedOrNull()) {
            break;
        }
        currHC = currObj->GetClass();
    }
    return JSTaggedValue::Undefined();
}

void JITProfiler::InsertProfileType(JSTaggedType root, JSTaggedType child, ProfileType rootType, ProfileType childType,
                                    bool update)
{
    ptManager_->RecordHClass(rootType, childType, child, update);
    LockHolder lock(mutex_);
    auto iter = tracedProfiles_.find(root);
    if (iter != tracedProfiles_.end()) {
        auto generator = iter->second;
        generator->InsertProfileType(child, childType);
    } else {
        auto generator = vm_->GetNativeAreaAllocator()->New<PGOTypeGenerator>();
        generator->InsertProfileType(child, childType);
        tracedProfiles_.emplace(root, generator);
    }
}

ProfileType JITProfiler::GetProfileType(JSTaggedType root, JSTaggedType child)
{
    PGOTypeGenerator *generator = nullptr;
    {
        LockHolder lock(mutex_);
        auto iter = tracedProfiles_.find(root);
        if (iter == tracedProfiles_.end()) {
            return ProfileType::PROFILE_TYPE_NONE;
        }
        generator = iter->second;
    }
    return generator->GetProfileType(child);
}

ProfileType JITProfiler::GetOrInsertProfileType(JSTaggedType root, JSTaggedType child)
{
    PGOTypeGenerator *generator = nullptr;
    {
        LockHolder lock(mutex_);
        auto iter = tracedProfiles_.find(root);
        if (iter == tracedProfiles_.end()) {
            return ProfileType::PROFILE_TYPE_NONE;
        }
        generator = iter->second;
    }
    auto rootType = generator->GetProfileType(root);
    if (rootType.IsNone()) {
        return ProfileType::PROFILE_TYPE_NONE;
    }
    bool generateNewType = false;
    auto type = generator->GenerateProfileType(rootType, child, generateNewType);
    if (generateNewType) {
        ptManager_->RecordHClass(rootType, type, child, true);
    }
    return type;
}

void JITProfiler::UpdateRootProfileType(JSHClass *oldHClass, JSHClass *newHClass)
{
    LockHolder lock(mutex_);
    auto oldRootHClass = JSHClass::FindRootHClass(oldHClass);
    auto iter = tracedProfiles_.find(JSTaggedType(oldRootHClass));
    if (iter == tracedProfiles_.end()) {
        return;
    }
    auto generator = iter->second;
    auto rootProfileType = generator->GetProfileType(JSTaggedType(oldRootHClass));
    {
        vm_->GetNativeAreaAllocator()->Delete(iter->second);
        tracedProfiles_.erase(iter);
    }
    InsertProfileType(JSTaggedType(newHClass), JSTaggedType(newHClass), rootProfileType, rootProfileType, true);
}

void JITProfiler::AddObjectInfoWithMega(int32_t bcOffset)
{
    auto megaType = ProfileType::CreateMegaType();
    PGOObjectInfo info(megaType, megaType, megaType, megaType, megaType, megaType, PGOSampleType());
    AddObjectInfoImplement(bcOffset, info);
}

void JITProfiler::AddObjectInfoImplement(int32_t bcOffset, const PGOObjectInfo &info)
{
    PGORWOpType *cur = nullptr;
    if (bcOffsetPGORwTypeMap_.find(bcOffset) == bcOffsetPGORwTypeMap_.end()) {
        cur = new PGORWOpType();
        bcOffsetPGORwTypeMap_[bcOffset] = cur;
    } else {
        cur = const_cast<PGORWOpType*>(bcOffsetPGORwTypeMap_.at(bcOffset));
    }
    if (cur != nullptr) {
        cur->AddObjectInfo(info);
    }
}

bool JITProfiler::AddObjectInfo(ApEntityId abcId, int32_t bcOffset,
                                JSHClass *receiver, JSHClass *hold, JSHClass *holdTra, uint32_t accessorMethodId)
{
    PGOSampleType accessor = PGOSampleType::CreateProfileType(abcId, accessorMethodId, ProfileType::Kind::MethodId);
    return AddTranstionObjectInfo(bcOffset, receiver, hold, holdTra, accessor);
}

bool JITProfiler::AddTranstionObjectInfo(
    int32_t bcOffset, JSHClass *receiver, JSHClass *hold, JSHClass *holdTra, PGOSampleType accessorMethod)
{
    auto receiverRootHClass = JSTaggedType(JSHClass::FindRootHClass(receiver));
    auto receiverRootType = GetProfileType(receiverRootHClass, receiverRootHClass);
    if (receiverRootType.IsNone()) {
        return false;
    }
    auto receiverType = GetOrInsertProfileType(receiverRootHClass, JSTaggedType(receiver));

    auto holdRootHClass = JSTaggedType(JSHClass::FindRootHClass(hold));
    auto holdRootType = GetProfileType(holdRootHClass, holdRootHClass);
    if (holdRootType.IsNone()) {
        return true;
    }
    auto holdType = GetOrInsertProfileType(holdRootHClass, JSTaggedType(hold));

    auto holdTraRootHClass = JSTaggedType(JSHClass::FindRootHClass(holdTra));
    auto holdTraRootType = GetProfileType(holdTraRootHClass, holdTraRootHClass);
    if (holdTraRootType.IsNone()) {
        return true;
    }
    auto holdTraType = GetOrInsertProfileType(holdTraRootHClass, JSTaggedType(holdTra));

    if (receiver != hold) {
        UpdateLayout(receiver);
    }

    if (holdType == holdTraType) {
        UpdateLayout(hold);
    } else {
        UpdateTranstionLayout(hold, holdTra);
    }

    PGOObjectInfo info(receiverRootType, receiverType, holdRootType, holdType, holdTraRootType, holdTraType,
                       accessorMethod);
    UpdatePrototypeChainInfo(receiver, hold, info);
    AddObjectInfoImplement(bcOffset, info);
    return true;
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
        PGOObjectInfo info(profileType);
        AddObjectInfoImplement(bcOffset, info);
    } else if (receiver->IsTypedArray()) {
        JSType jsType = receiver->GetObjectType();
        auto profileType = ProfileType::CreateBuiltinsTypedArray(abcId, jsType, onHeap, everOutOfBounds);
        PGOObjectInfo info(profileType);
        AddObjectInfoImplement(bcOffset, info);
    } else {
        auto type = receiver->GetObjectType();
        PGOObjectInfo info(ProfileType::CreateBuiltins(abcId, type));
        AddObjectInfoImplement(bcOffset, info);
    }
}

void JITProfiler::AddBuiltinsGlobalInfo(ApEntityId abcId, int32_t bcOffset, GlobalIndex globalsId)
{
    PGOObjectInfo info(ProfileType::CreateGlobals(abcId, globalsId));
    AddObjectInfoImplement(bcOffset, info);
}

void JITProfiler::AddBuiltinsInfoByNameInInstance(ApEntityId abcId, int32_t bcOffset, JSHClass *receiver)
{
    auto thread = vm_->GetJSThread();
    auto type = receiver->GetObjectType();
    const auto &ctorEntries = thread->GetCtorHclassEntries();
    auto entry = ctorEntries.find(receiver);
    if (entry != ctorEntries.end()) {
        AddBuiltinsGlobalInfo(abcId, bcOffset, entry->second);
        return ;
    }

    auto builtinsId = ToBuiltinsTypeId(type);
    if (!builtinsId.has_value()) {
        return;
    }
    JSHClass *exceptRecvHClass = nullptr;
    if (builtinsId == BuiltinTypeId::ARRAY) {
        exceptRecvHClass = thread->GetArrayInstanceHClass(receiver->GetElementsKind());
    } else if (builtinsId == BuiltinTypeId::STRING) {
        exceptRecvHClass = receiver;
    } else {
        exceptRecvHClass = thread->GetBuiltinInstanceHClass(builtinsId.value());
    }

    if (exceptRecvHClass != receiver) {
        // When JSType cannot uniquely identify builtins object, it is necessary to
        // query the receiver on the global constants.
        if (builtinsId == BuiltinTypeId::OBJECT) {
            exceptRecvHClass = JSHClass::Cast(thread->GlobalConstants()->GetIteratorResultClass().GetTaggedObject());
            if (exceptRecvHClass == receiver) {
                GlobalIndex globalsId;
                globalsId.UpdateGlobalConstId(static_cast<size_t>(ConstantIndex::ITERATOR_RESULT_CLASS));
                AddBuiltinsGlobalInfo(abcId, bcOffset, globalsId);
            }
        }
        return ;
    }
    AddBuiltinsInfo(abcId, bcOffset, receiver, receiver);
}

void JITProfiler::AddBuiltinsInfoByNameInProt(ApEntityId abcId, int32_t bcOffset, JSHClass *receiver, JSHClass *hold)
{
    auto type = receiver->GetObjectType();
    auto builtinsId = ToBuiltinsTypeId(type);
    if (!builtinsId.has_value()) {
        return;
    }
    auto thread = vm_->GetJSThread();
    JSHClass *exceptRecvHClass = nullptr;
    if (builtinsId == BuiltinTypeId::ARRAY) {
        exceptRecvHClass = thread->GetArrayInstanceHClass(receiver->GetElementsKind());
    } else if (builtinsId == BuiltinTypeId::STRING) {
        exceptRecvHClass = receiver;
    } else {
        exceptRecvHClass = thread->GetBuiltinInstanceHClass(builtinsId.value());
    }

    auto exceptHoldHClass = thread->GetBuiltinPrototypeHClass(builtinsId.value());
    auto exceptPrototypeOfPrototypeHClass =
        thread->GetBuiltinPrototypeOfPrototypeHClass(builtinsId.value());
    // iterator needs to find two layers of prototype
    if (builtinsId == BuiltinTypeId::ARRAY_ITERATOR) {
        if ((exceptRecvHClass != receiver) ||
            (exceptHoldHClass != hold && exceptPrototypeOfPrototypeHClass != hold)) {
            return;
        }
    } else if (IsTypedArrayType(builtinsId.value())) {
        auto exceptRecvHClassOnHeap = thread->GetBuiltinExtraHClass(builtinsId.value());
        ASSERT_PRINT(exceptRecvHClassOnHeap == nullptr || exceptRecvHClassOnHeap->IsOnHeapFromBitField(),
                     "must be on heap");
        if (JITProfiler::IsJSHClassNotEqual(receiver, hold, exceptRecvHClass, exceptRecvHClassOnHeap, exceptHoldHClass,
                                            exceptPrototypeOfPrototypeHClass)) {
            return;
        }
    } else if (exceptRecvHClass != receiver || exceptHoldHClass != hold) {
        return;
    }
    AddBuiltinsInfo(abcId, bcOffset, receiver, receiver);
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

void JITProfiler::UpdateLayout(JSHClass *hclass)
{
    auto parentHClass = hclass->GetParent();
    if (parentHClass.IsJSHClass()) {
        UpdateTranstionLayout(JSHClass::Cast(parentHClass.GetTaggedObject()), hclass);
    } else {
        auto rootHClass = JSHClass::FindRootHClass(hclass);
        auto rootHClassVal = JSTaggedType(rootHClass);
        auto rootType = GetProfileType(rootHClassVal, rootHClassVal);
        if (rootType.IsNone()) {
            return;
        }

        auto prototypeHClass = JSHClass::FindProtoRootHClass(rootHClass);
        if (prototypeHClass.IsJSHClass()) {
            auto prototypeValue = prototypeHClass.GetRawData();
            auto prototypeType = GetProfileType(prototypeValue, prototypeValue);
            if (!prototypeType.IsNone()) {
                UpdateLayout(JSHClass::Cast(prototypeHClass.GetTaggedObject()));
            }
        }

        auto curType = GetOrInsertProfileType(rootHClassVal, JSTaggedType(hclass));
        (void) curType;
    }
}

void JITProfiler::UpdateTranstionLayout(JSHClass *parent, JSHClass *child)
{
    auto rootHClass = JSHClass::FindRootHClass(parent);
    auto rootHClassVal = JSTaggedType(rootHClass);
    auto rootType = GetProfileType(rootHClassVal, rootHClassVal);
    if (rootType.IsNone()) {
        return;
    }
    auto curHClass = JSTaggedType(child);
    auto curType = GetOrInsertProfileType(rootHClassVal, curHClass);
    CVector<JSTaggedType> hclassVec;
    CVector<ProfileType> typeVec;
    hclassVec.push_back(curHClass);
    typeVec.push_back(curType);

    auto parentHClass = JSTaggedValue(parent);
    auto parentHCValue = JSTaggedType(parent);
    auto parentType = GetOrInsertProfileType(rootHClassVal, parentHCValue);
    while (parentHClass.IsJSHClass()) {
        parentHClass = JSHClass::Cast(parentHClass.GetTaggedObject())->GetParent();
        if (!parentHClass.IsJSHClass()) {
            break;
        }
        hclassVec.push_back(parentHCValue);
        typeVec.push_back(parentType);
        parentHCValue = JSTaggedType(parentHClass.GetTaggedObject());
        parentType = GetOrInsertProfileType(rootHClassVal, parentHCValue);
    }

    auto prototypeHClass = JSHClass::FindProtoRootHClass(rootHClass);
    if (prototypeHClass.IsJSHClass()) {
        auto prototypeValue = prototypeHClass.GetRawData();
        auto prototypeType = GetProfileType(prototypeValue, prototypeValue);
        if (!prototypeType.IsNone()) {
            UpdateLayout(JSHClass::Cast(prototypeHClass.GetTaggedObject()));
        }
    }

    int32_t size = static_cast<int32_t>(hclassVec.size());
    for (int32_t i = size - 1; i >= 0; i--) {
        curHClass = hclassVec[i];
        curType = typeVec[i];
        parentHCValue = curHClass;
        parentType = curType;
    }
}

void JITProfiler::UpdatePrototypeChainInfo(JSHClass *receiver, JSHClass *holder, PGOObjectInfo &info)
{
    if (receiver == holder) {
        return;
    }

    std::vector<std::pair<ProfileType, ProfileType>> protoChain;
    JSTaggedValue proto = JSHClass::FindProtoHClass(receiver);
    while (proto.IsJSHClass()) {
        auto protoHClass = JSHClass::Cast(proto.GetTaggedObject());
        if (protoHClass == holder) {
            break;
        }
        auto protoRootHClass = JSTaggedType(JSHClass::FindRootHClass(protoHClass));
        auto protoRootType = GetProfileType(protoRootHClass, protoRootHClass);
        if (protoRootType.IsNone()) {
            break;
        }
        auto protoType = GetOrInsertProfileType(protoRootHClass, JSTaggedType(protoHClass));
        protoChain.emplace_back(protoRootType, protoType);
        proto = JSHClass::FindProtoHClass(protoHClass);
    }
    if (!protoChain.empty()) {
        info.AddPrototypePt(protoChain);
    }
}

void JITProfiler::ProcessReferences(const WeakRootVisitor &visitor)
{
    for (auto iter = tracedProfiles_.begin(); iter != tracedProfiles_.end();) {
        JSTaggedType object = iter->first;
        auto fwd = visitor(reinterpret_cast<TaggedObject *>(object));
        if (fwd == nullptr) {
            vm_->GetNativeAreaAllocator()->Delete(iter->second);
            iter = tracedProfiles_.erase(iter);
            continue;
        }
        if (fwd != reinterpret_cast<TaggedObject *>(object)) {
            UNREACHABLE();
        }
        auto generator = iter->second;
        generator->ProcessReferences(visitor);
        ++iter;
    }
}

JITProfiler::~JITProfiler()
{
    for (auto iter : tracedProfiles_) {
        vm_->GetNativeAreaAllocator()->Delete(iter.second);
    }
}
}
