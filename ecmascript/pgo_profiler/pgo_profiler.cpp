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

#include "ecmascript/pgo_profiler/pgo_profiler.h"

#include <chrono>

#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/js_function.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/pgo_profiler/pgo_context.h"
#include "ecmascript/pgo_profiler/pgo_profiler_info.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/pgo_profiler/types/pgo_profile_type.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "macros.h"

namespace panda::ecmascript::pgo {
void PGOProfiler::ProfileCall(JSTaggedType callTarget, SampleMode mode, int32_t incCount)
{
    if (!isEnable_) {
        return;
    }
    DISALLOW_GARBAGE_COLLECTION;
    JSTaggedValue calleeFunc(callTarget);
    if (!calleeFunc.IsJSFunction()) {
        return;
    }
    if (!JSFunction::Cast(calleeFunc)->GetMethod().IsMethod()) {
        return;
    }
    auto calleeMethod = Method::Cast(JSFunction::Cast(calleeFunc)->GetMethod());
    JSTaggedValue calleeRecordNameValue = calleeMethod->GetRecordName();
    if (calleeRecordNameValue.IsHole()) {
        return;
    }
    CString calleeRecordName = ConvertToString(calleeRecordNameValue);
    ProfileType calleeRecordType = GetRecordProfileType(JSFunction::Cast(calleeFunc), calleeRecordName);
    recordInfos_->AddMethod(calleeRecordType, calleeMethod, mode, incCount);
}

void PGOProfiler::ProfileCreateObject(JSTaggedType object, ApEntityId abcId, int32_t traceId)
{
    if (!isEnable_) {
        return;
    }

    JSTaggedValue objectValue(object);
    if (objectValue.IsJSObject()) {
        auto hclass = objectValue.GetTaggedObject()->GetClass();
        hclass->SetParent(vm_->GetJSThread(), JSTaggedValue::Undefined());
        InsertLiteralTraceId(JSTaggedType(hclass), abcId, traceId);
    }
}

void PGOProfiler::ProfileDefineClass(JSTaggedType ctor)
{
    if (!isEnable_) {
        return;
    }
    auto ctorValue = JSTaggedValue(ctor);
    if (!ctorValue.IsJSFunction()) {
        return;
    }
    auto ctorFunc = JSFunction::Cast(ctorValue.GetTaggedObject());
    auto ctorMethodValue = ctorFunc->GetMethod();
    if (!ctorMethodValue.IsMethod()) {
        return;
    }
    auto ctorMethod = Method::Cast(ctorMethodValue);
    auto traceId = ctorMethod->GetMethodId().GetOffset();
    auto ctorMethodHClass = ctorFunc->GetClass();
    ctorMethodHClass->SetParent(vm_->GetJSThread(), JSTaggedValue::Undefined());
    InsertLiteralTraceId(JSTaggedType(ctorFunc->GetClass()), GetMethodAbcId(ctorFunc), traceId);

    auto prototype = ctorFunc->GetProtoOrHClass();
    if (prototype.IsJSObject()) {
        auto prototypeHClass = prototype.GetTaggedObject()->GetClass();
        prototypeHClass->SetParent(vm_->GetJSThread(), JSTaggedValue::Undefined());
        InsertLiteralTraceId(JSTaggedType(prototypeHClass), GetMethodAbcId(ctorFunc), traceId);
    }
}

void PGOProfiler::UpdateTrackInfo(JSTaggedValue trackInfoVal, ElementsKind newKind)
{
    if (trackInfoVal.IsHeapObject() && trackInfoVal.IsWeak()) {
        auto trackInfo = TrackInfo::Cast(trackInfoVal.GetWeakReferentUnChecked());
        auto oldKind = trackInfo->GetElementsKind();
        if (oldKind == newKind) {
            return;
        }
        trackInfo->SetElementsKind(newKind);
        auto func = trackInfo->GetCachedFunc();
        if (!func.IsJSFunction()) {
            return;
        }
        auto method = JSFunction::Cast(func)->GetMethod();
        auto profileTypeInfoVal = Method::Cast(method)->GetProfileTypeInfo();
        if (profileTypeInfoVal.IsUndefined()) {
            return;
        }
        auto profileTypeInfo = ProfileTypeInfo::Cast(profileTypeInfoVal.GetTaggedObject());
        if (!profileTypeInfo->IsProfileTypeInfoChanged()) {
            profileTypeInfo->ResetPeriodCount();
            PGOPreDump(JSTaggedType(func.GetTaggedObject()));
        }
    }
}

void PGOProfiler::PGODump(JSTaggedType func)
{
    if (!isEnable_) {
        return;
    }
    {
        LockHolder lock(mutex_);
        dumpWorkList_.emplace_back(func);
        if (state_ == State::STOP) {
            state_ = State::START;
            Taskpool::GetCurrentTaskpool()->PostTask(
                std::make_unique<PGOProfilerTask>(this, vm_->GetJSThread()->GetThreadId()));
        }
    }
}

void PGOProfiler::PausePGODump()
{
    if (!isEnable_) {
        return;
    }
    LockHolder lock(mutex_);
    if (state_ == State::START) {
        state_ = State::PAUSE;
        condition_.Wait(&mutex_);
    }
}

void PGOProfiler::ResumePGODump()
{
    if (!isEnable_) {
        return;
    }
    LockHolder lock(mutex_);
    if (state_ == State::PAUSE) {
        state_ = State::START;
        Taskpool::GetCurrentTaskpool()->PostTask(
            std::make_unique<PGOProfilerTask>(this, vm_->GetJSThread()->GetThreadId()));
    }
}

void PGOProfiler::WaitPGODumpFinish()
{
    if (!isEnable_) {
        return;
    }
    LockHolder lock(mutex_);
    while (state_ == State::START) {
        condition_.Wait(&mutex_);
    }
}

void PGOProfiler::PGOPreDump(JSTaggedType func)
{
    if (!isEnable_) {
        return;
    }
    LockHolder lock(mutex_);
    preDumpWorkList_.emplace(func);
}

void PGOProfiler::HandlePGOPreDump()
{
    if (!isEnable_) {
        return;
    }
    DISALLOW_GARBAGE_COLLECTION;
    CSet<JSTaggedType> preDumpWorkList;
    {
        LockHolder lock(mutex_);
        preDumpWorkList = preDumpWorkList_;
    }
    for (auto iter : preDumpWorkList) {
        auto funcValue = JSTaggedValue(iter);
        if (!funcValue.IsJSFunction()) {
            continue;
        }
        JSFunction *func = JSFunction::Cast(funcValue.GetTaggedObject());
        JSTaggedValue methodValue = func->GetMethod();
        if (!methodValue.IsMethod()) {
            continue;
        }
        JSTaggedValue recordNameValue = Method::Cast(methodValue)->GetRecordName();
        if (!recordNameValue.IsString()) {
            continue;
        }
        CString recordName = ConvertToString(recordNameValue);
        auto abcId = GetMethodAbcId(func);
        ProfileByteCode(abcId, recordName, methodValue);
    }
}

void PGOProfiler::HandlePGODump()
{
    if (!isEnable_) {
        return;
    }
    DISALLOW_GARBAGE_COLLECTION;
    JSTaggedValue current = PopFromProfileQueue();
    while (current.IsJSFunction()) {
        JSFunction *func = JSFunction::Cast(current.GetTaggedObject());
        JSTaggedValue methodValue = func->GetMethod();
        if (!methodValue.IsMethod()) {
            current = PopFromProfileQueue();
            continue;
        }
        JSTaggedValue recordNameValue = Method::Cast(methodValue)->GetRecordName();
        if (!recordNameValue.IsString()) {
            current = PopFromProfileQueue();
            continue;
        }
        CString recordName = ConvertToString(recordNameValue);
        auto abcId = GetMethodAbcId(func);
        ProfileByteCode(abcId, recordName, methodValue);
        methodCount_++;
        current = PopFromProfileQueue();
    }
    if (state_ == State::PAUSE) {
        return;
    }
    // Merged every 10 methods and merge interval greater than minimal interval
    auto interval = std::chrono::system_clock::now() - saveTimestamp_;
    if (methodCount_ >= MERGED_EVERY_COUNT && interval > MERGED_MIN_INTERVAL) {
        LOG_ECMA(DEBUG) << "Sample: post task to save profiler";
        PGOProfilerManager::GetInstance()->Merge(this);
        PGOProfilerManager::GetInstance()->AsynSave();
        SetSaveTimestamp(std::chrono::system_clock::now());
        methodCount_ = 0;
    }
}

JSTaggedValue PGOProfiler::PopFromProfileQueue()
{
    LockHolder lock(mutex_);
    auto result = JSTaggedValue::Undefined();
    while (result.IsUndefined()) {
        if (dumpWorkList_.empty()) {
            state_ = State::STOP;
            condition_.SignalAll();
            break;
        }
        if (state_ == State::PAUSE) {
            condition_.SignalAll();
            break;
        }
        auto func = dumpWorkList_.front();
        dumpWorkList_.pop_front();
        result = JSTaggedValue(func);
        auto iter = std::find(preDumpWorkList_.begin(), preDumpWorkList_.end(), func);
        if (iter != preDumpWorkList_.end()) {
            preDumpWorkList_.erase(iter);
        }
    }
    return result;
}

void PGOProfiler::ProfileByteCode(ApEntityId abcId, const CString &recordName, JSTaggedValue value)
{
    Method *method = Method::Cast(value.GetTaggedObject());
    JSTaggedValue profileTypeInfoVal = method->GetProfileTypeInfo();
    ASSERT(!profileTypeInfoVal.IsUndefined());
    auto profileTypeInfo = ProfileTypeInfo::Cast(profileTypeInfoVal.GetTaggedObject());
    auto methodId = method->GetMethodId();
    auto pcStart = method->GetBytecodeArray();
    auto codeSize = method->GetCodeSize();
    BytecodeInstruction bcIns(pcStart);
    auto bcInsLast = bcIns.JumpTo(codeSize);

    while (bcIns.GetAddress() != bcInsLast.GetAddress()) {
        auto opcode = bcIns.GetOpcode();
        auto bcOffset = bcIns.GetAddress() - pcStart;
        auto pc = bcIns.GetAddress();
        switch (opcode) {
            case EcmaOpcode::LDTHISBYNAME_IMM8_ID16:
            case EcmaOpcode::LDOBJBYNAME_IMM8_ID16: {
                uint8_t slotId = READ_INST_8_0();
                DumpICByName(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo, BCType::LOAD);
                break;
            }
            case EcmaOpcode::LDTHISBYNAME_IMM16_ID16:
            case EcmaOpcode::LDOBJBYNAME_IMM16_ID16: {
                uint16_t slotId = READ_INST_16_0();
                DumpICByName(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo, BCType::LOAD);
                break;
            }
            case EcmaOpcode::LDOBJBYVALUE_IMM8_V8:
            case EcmaOpcode::LDTHISBYVALUE_IMM8: {
                uint8_t slotId = READ_INST_8_0();
                DumpICByValue(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo, BCType::LOAD);
                break;
            }
            case EcmaOpcode::LDOBJBYVALUE_IMM16_V8:
            case EcmaOpcode::LDTHISBYVALUE_IMM16: {
                uint16_t slotId = READ_INST_16_0();
                DumpICByValue(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo, BCType::LOAD);
                break;
            }
            case EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8:
            case EcmaOpcode::STTHISBYNAME_IMM8_ID16: {
                uint8_t slotId = READ_INST_8_0();
                DumpICByName(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo, BCType::STORE);
                break;
            }
            case EcmaOpcode::STOBJBYNAME_IMM16_ID16_V8:
            case EcmaOpcode::STTHISBYNAME_IMM16_ID16: {
                uint16_t slotId = READ_INST_16_0();
                DumpICByName(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo, BCType::STORE);
                break;
            }
            case EcmaOpcode::STOBJBYVALUE_IMM8_V8_V8:
            case EcmaOpcode::STTHISBYVALUE_IMM8_V8: {
                uint8_t slotId = READ_INST_8_0();
                DumpICByValue(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo, BCType::STORE);
                break;
            }
            case EcmaOpcode::STOBJBYVALUE_IMM16_V8_V8:
            case EcmaOpcode::STTHISBYVALUE_IMM16_V8: {
                uint16_t slotId = READ_INST_16_0();
                DumpICByValue(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo, BCType::STORE);
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
            case EcmaOpcode::STRICTEQ_IMM8_V8: {
                uint8_t slotId = READ_INST_8_0();
                DumpOpType(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo);
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
                uint8_t slotId = READ_INST_8_0();
                DumpCall(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo);
                break;
            }
            case EcmaOpcode::WIDE_CALLRANGE_PREF_IMM16_V8:
            case EcmaOpcode::WIDE_CALLTHISRANGE_PREF_IMM16_V8: {
                uint16_t slotId = READ_INST_16_0();
                DumpCall(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo);
                break;
            }
            case EcmaOpcode::NEWOBJRANGE_IMM8_IMM8_V8:
            case EcmaOpcode::NEWOBJRANGE_IMM16_IMM8_V8:
            case EcmaOpcode::WIDE_NEWOBJRANGE_PREF_IMM16_V8: {
                break;
            }
            // Create object
            case EcmaOpcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8: {
                uint8_t slotId = READ_INST_8_0();
                DumpDefineClass(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo);
                break;
            }
            case EcmaOpcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8: {
                uint16_t slotId = READ_INST_16_0();
                DumpDefineClass(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo);
                break;
            }
            case EcmaOpcode::CREATEOBJECTWITHBUFFER_IMM8_ID16:
            case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
            case EcmaOpcode::CREATEEMPTYARRAY_IMM8: {
                auto header = method->GetJSPandaFile()->GetPandaFile()->GetHeader();
                auto traceId =
                    static_cast<int32_t>(reinterpret_cast<uintptr_t>(pc) - reinterpret_cast<uintptr_t>(header));
                uint8_t slotId = READ_INST_8_0();
                DumpCreateObject(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo, traceId);
                break;
            }
            case EcmaOpcode::CREATEOBJECTWITHBUFFER_IMM16_ID16:
            case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM16_ID16:
            case EcmaOpcode::CREATEEMPTYARRAY_IMM16: {
                auto header = method->GetJSPandaFile()->GetPandaFile()->GetHeader();
                auto traceId =
                    static_cast<int32_t>(reinterpret_cast<uintptr_t>(pc) - reinterpret_cast<uintptr_t>(header));
                uint16_t slotId = READ_INST_16_0();
                DumpCreateObject(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo, traceId);
                break;
            }
            case EcmaOpcode::DEFINEGETTERSETTERBYVALUE_V8_V8_V8_V8:
            default:
                break;
        }
        bcIns = bcIns.GetNext();
    }
}

void PGOProfiler::DumpICByName(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                               uint32_t slotId, ProfileTypeInfo *profileTypeInfo, BCType type)
{
    JSTaggedValue firstValue = profileTypeInfo->Get(slotId);
    if (!firstValue.IsHeapObject()) {
        return;
    }
    if (firstValue.IsWeak()) {
        TaggedObject *object = firstValue.GetWeakReferentUnChecked();
        if (object->GetClass()->IsHClass()) {
            JSTaggedValue secondValue = profileTypeInfo->Get(slotId + 1);
            JSHClass *hclass = JSHClass::Cast(object);
            DumpICByNameWithHandler(abcId, recordName, methodId, bcOffset, hclass, secondValue, type);
        }
        return;
    }
    DumpICByNameWithPoly(abcId, recordName, methodId, bcOffset, firstValue, type);
}

void PGOProfiler::DumpICByValue(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                                uint32_t slotId, ProfileTypeInfo *profileTypeInfo, BCType type)
{
    JSTaggedValue firstValue = profileTypeInfo->Get(slotId);
    if (!firstValue.IsHeapObject()) {
        return;
    }
    if (firstValue.IsWeak()) {
        TaggedObject *object = firstValue.GetWeakReferentUnChecked();
        if (object->GetClass()->IsHClass()) {
            JSTaggedValue secondValue = profileTypeInfo->Get(slotId + 1);
            JSHClass *hclass = JSHClass::Cast(object);
            DumpICByValueWithHandler(abcId, recordName, methodId, bcOffset, hclass, secondValue, type);
        }
        return;
    }
    // Check key
    if ((firstValue.IsString() || firstValue.IsSymbol())) {
        JSTaggedValue secondValue = profileTypeInfo->Get(slotId + 1);
        DumpICByNameWithPoly(abcId, recordName, methodId, bcOffset, secondValue, type);
        return;
    }
    // Check without key
    DumpICByValueWithPoly(abcId, recordName, methodId, bcOffset, firstValue, type);
}

void PGOProfiler::DumpICByNameWithPoly(ApEntityId abcId,
    const CString &recordName, EntityId methodId, int32_t bcOffset, JSTaggedValue cacheValue, BCType type)
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
        DumpICByNameWithHandler(abcId, recordName, methodId, bcOffset, hclass, handler, type);
    }
}

void PGOProfiler::DumpICByValueWithPoly(ApEntityId abcId,
    const CString &recordName, EntityId methodId, int32_t bcOffset, JSTaggedValue cacheValue, BCType type)
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
        DumpICByValueWithHandler(abcId, recordName, methodId, bcOffset, hclass, handler, type);
    }
}

void PGOProfiler::DumpICByNameWithHandler(ApEntityId abcId, const CString &recordName, EntityId methodId,
                                          int32_t bcOffset, JSHClass *hclass, JSTaggedValue secondValue, BCType type)
{
    if (type == BCType::LOAD) {
        if (secondValue.IsInt()) {
            auto handlerInfo = static_cast<uint32_t>(secondValue.GetInt());
            if (HandlerBase::IsNonExist(handlerInfo)) {
                return;
            }
            auto kind = PGOObjKind::LOCAL;
            AddObjectInfo(abcId, recordName, methodId, bcOffset, hclass, kind);
        }
        if (secondValue.IsPrototypeHandler()) {
            auto kind = PGOObjKind::PROTOTYPE;
            AddObjectInfo(abcId, recordName, methodId, bcOffset, hclass, kind);
        }
        // LoadGlobal
        return;
    }
    if (secondValue.IsInt()) {
        auto kind = PGOObjKind::LOCAL;
        AddObjectInfo(abcId, recordName, methodId, bcOffset, hclass, kind);
    }
    if (secondValue.IsTransitionHandler()) {
        auto kind = PGOObjKind::LOCAL;
        auto transitionHandler = TransitionHandler::Cast(secondValue.GetTaggedObject());
        auto transitionHClassVal = transitionHandler->GetTransitionHClass();
        if (transitionHClassVal.IsJSHClass()) {
            auto transitionHClass = JSHClass::Cast(transitionHClassVal.GetTaggedObject());
            AddObjectInfo(abcId, recordName, methodId, bcOffset, transitionHClass, kind);
        }
    }
    if (secondValue.IsTransWithProtoHandler()) {
        auto kind = PGOObjKind::PROTOTYPE;
        auto transWithProtoHandler = TransWithProtoHandler::Cast(secondValue.GetTaggedObject());
        auto transitionHClassVal = transWithProtoHandler->GetTransitionHClass();
        if (transitionHClassVal.IsJSHClass()) {
            auto transitionHClass = JSHClass::Cast(transitionHClassVal.GetTaggedObject());
            AddObjectInfo(abcId, recordName, methodId, bcOffset, transitionHClass, kind);
        }
    }
    if (secondValue.IsTransitionHandler()) {
        auto kind = PGOObjKind::LOCAL;
        auto transitionHandler = TransitionHandler::Cast(secondValue.GetTaggedObject());
        auto transitionHClassVal = transitionHandler->GetTransitionHClass();
        if (transitionHClassVal.IsJSHClass()) {
            auto transitionHClass = JSHClass::Cast(transitionHClassVal.GetTaggedObject());
            AddObjectInfo(abcId, recordName, methodId, bcOffset, transitionHClass, kind);
        }
    }
    if (secondValue.IsPrototypeHandler()) {
        auto kind = PGOObjKind::PROTOTYPE;
        AddObjectInfo(abcId, recordName, methodId, bcOffset, hclass, kind);
    }
    if (secondValue.IsPropertyBox()) {
        // StoreGlobal
    }
    if (secondValue.IsStoreTSHandler()) {
        StoreTSHandler *storeTSHandler = StoreTSHandler::Cast(secondValue.GetTaggedObject());
        auto cellValue = storeTSHandler->GetProtoCell();
        ASSERT(cellValue.IsProtoChangeMarker());
        ProtoChangeMarker *cell = ProtoChangeMarker::Cast(cellValue.GetTaggedObject());
        if (cell->GetHasChanged()) {
            return;
        }
        auto kind = PGOObjKind::LOCAL;
        AddObjectInfo(abcId, recordName, methodId, bcOffset, hclass, kind);
    }
}

void PGOProfiler::DumpICByValueWithHandler(ApEntityId abcId, const CString &recordName, EntityId methodId,
                                           int32_t bcOffset, JSHClass *hclass, JSTaggedValue secondValue, BCType type)
{
    if (type == BCType::LOAD) {
        if (secondValue.IsInt()) {
            auto handlerInfo = static_cast<uint32_t>(secondValue.GetInt());
            PGOObjKind kind = PGOObjKind::LOCAL;
            if (HandlerBase::IsJSArray(handlerInfo) || HandlerBase::IsTypedArrayElement(handlerInfo)) {
                kind = PGOObjKind::ELEMENT;
            }
            AddObjectInfo(abcId, recordName, methodId, bcOffset, hclass, kind);
        }
        return;
    }
    if (secondValue.IsInt()) {
        auto handlerInfo = static_cast<uint32_t>(secondValue.GetInt());
        PGOObjKind kind = PGOObjKind::LOCAL;
        if (HandlerBase::IsJSArray(handlerInfo) || HandlerBase::IsTypedArrayElement(handlerInfo)) {
            kind = PGOObjKind::ELEMENT;
        }
        AddObjectInfo(abcId, recordName, methodId, bcOffset, hclass, kind);
    } else if (secondValue.IsTransitionHandler()) {
        auto transitionHandler = TransitionHandler::Cast(secondValue.GetTaggedObject());
        auto transitionHClassVal = transitionHandler->GetTransitionHClass();

        auto handlerInfoValue = transitionHandler->GetHandlerInfo();
        ASSERT(handlerInfoValue.IsInt());
        auto handlerInfo = static_cast<uint32_t>(handlerInfoValue.GetInt());
        PGOObjKind kind = PGOObjKind::LOCAL;
        if (HandlerBase::IsJSArray(handlerInfo)) {
            kind = PGOObjKind::ELEMENT;
        }
        if (transitionHClassVal.IsJSHClass()) {
            auto transitionHClass = JSHClass::Cast(transitionHClassVal.GetTaggedObject());
            AddObjectInfo(abcId, recordName, methodId, bcOffset, transitionHClass, kind);
        }
    } else if (secondValue.IsTransWithProtoHandler()) {
        auto transWithProtoHandler = TransWithProtoHandler::Cast(secondValue.GetTaggedObject());
        auto transitionHClassVal = transWithProtoHandler->GetTransitionHClass();

        auto handlerInfoValue = transWithProtoHandler->GetHandlerInfo();
        ASSERT(handlerInfoValue.IsInt());
        auto handlerInfo = static_cast<uint32_t>(handlerInfoValue.GetInt());
        auto kind = PGOObjKind::PROTOTYPE;
        if (HandlerBase::IsJSArray(handlerInfo)) {
            kind = PGOObjKind::ELEMENT;
        }
        if (transitionHClassVal.IsJSHClass()) {
            auto transitionHClass = JSHClass::Cast(transitionHClassVal.GetTaggedObject());
            AddObjectInfo(abcId, recordName, methodId, bcOffset, transitionHClass, kind);
        }
    } else {
        ASSERT(secondValue.IsPrototypeHandler());
        PrototypeHandler *prototypeHandler = PrototypeHandler::Cast(secondValue.GetTaggedObject());
        auto cellValue = prototypeHandler->GetProtoCell();
        ASSERT(cellValue.IsProtoChangeMarker());
        ProtoChangeMarker *cell = ProtoChangeMarker::Cast(cellValue.GetTaggedObject());
        if (cell->GetHasChanged()) {
            return;
        }
        JSTaggedValue handlerInfoValue = prototypeHandler->GetHandlerInfo();
        ASSERT(handlerInfoValue.IsInt());
        auto handlerInfo = static_cast<uint32_t>(handlerInfoValue.GetInt());
        PGOObjKind kind = PGOObjKind::PROTOTYPE;
        if (HandlerBase::IsJSArray(handlerInfo)) {
            kind = PGOObjKind::ELEMENT;
        }
        AddObjectInfo(abcId, recordName, methodId, bcOffset, hclass, kind);
    }
}

void PGOProfiler::DumpOpType(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                             uint32_t slotId, ProfileTypeInfo *profileTypeInfo)
{
    JSTaggedValue slotValue = profileTypeInfo->Get(slotId);
    if (slotValue.IsInt()) {
        auto type = slotValue.GetInt();
        ProfileType recordType = GetRecordProfileType(abcId, recordName);
        recordInfos_->AddType(recordType, methodId, bcOffset, PGOSampleType(type));
    }
}

void PGOProfiler::DumpDefineClass(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                                  uint32_t slotId, ProfileTypeInfo *profileTypeInfo)
{
    JSTaggedValue slotValue = profileTypeInfo->Get(slotId);
    if (!slotValue.IsHeapObject() || !slotValue.IsWeak()) {
        return;
    }
    auto object = slotValue.GetWeakReferentUnChecked();
    if (object->GetClass()->IsJSFunction()) {
        JSFunction *ctorFunction = JSFunction::Cast(object);
        auto ctorMethod = ctorFunction->GetMethod();
        if (!ctorMethod.IsMethod()) {
            return;
        }
        ApEntityId ctorAbcId = GetMethodAbcId(ctorFunction);
        auto ctorJSMethod = Method::Cast(ctorMethod);
        int32_t ctorMethodId = static_cast<int32_t>(ctorJSMethod->GetMethodId().GetOffset());
        auto currentType = PGOSampleType::CreateProfileType(ctorAbcId, ctorMethodId);

        auto superFuncValue = ctorFunction->GetJSHClass()->GetPrototype();
        PGOSampleType superType(ProfileType::PROFILE_TYPE_NONE);
        if (superFuncValue.IsJSFunction()) {
            auto superFuncFunction = JSFunction::Cast(superFuncValue);
            if (superFuncFunction->GetMethod().IsMethod()) {
                ApEntityId superAbcId = GetMethodAbcId(superFuncFunction);
                auto superMethod = Method::Cast(superFuncFunction->GetMethod());
                auto superMethodId = superMethod->GetMethodId().GetOffset();
                superType = PGOSampleType::CreateProfileType(superAbcId, superMethodId);
            }
        }
        ProfileType recordType = GetRecordProfileType(abcId, recordName);
        recordInfos_->AddDefine(recordType, methodId, bcOffset, currentType, superType);

        auto hclassValue = ctorFunction->GetProtoOrHClass();
        if (!hclassValue.IsJSHClass()) {
            return;
        }
        auto hclass = JSHClass::Cast(hclassValue.GetTaggedObject());
        recordInfos_->AddLayout(currentType, JSTaggedType(hclass), PGOObjKind::LOCAL);

        auto ctorHClass = JSTaggedType(ctorFunction->GetJSHClass());
        recordInfos_->AddLayout(currentType, ctorHClass, PGOObjKind::CONSTRUCTOR);

        auto prototype = hclass->GetProto();
        if (!prototype.IsJSObject()) {
            return;
        }
        auto prototypeObj = JSObject::Cast(prototype);
        auto prototypeHClass = JSTaggedType(prototypeObj->GetClass());
        recordInfos_->AddLayout(currentType, prototypeHClass, PGOObjKind::PROTOTYPE);
    }
}

void PGOProfiler::DumpCreateObject(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                                   uint32_t slotId, ProfileTypeInfo *profileTypeInfo, int32_t traceId)
{
    JSTaggedValue slotValue = profileTypeInfo->Get(slotId);
    if (!slotValue.IsHeapObject()) {
        return;
    }
    ProfileType recordType = GetRecordProfileType(abcId, recordName);
    if (slotValue.IsWeak()) {
        auto object = slotValue.GetWeakReferentUnChecked();
        if (object->GetClass()->IsHClass()) {
            auto newHClass = JSHClass::Cast(object);
            ASSERT(newHClass->IsJSObject());
            auto iter = tracedProfiles_.find(JSTaggedType(newHClass));
            if (iter == tracedProfiles_.end()) {
                return;
            }
            ASSERT(iter->second.GetKind() == ProfileType::Kind::LiteralId);
            PGOSampleType currentType(iter->second);
            PGOSampleType superType(ProfileType::PROFILE_TYPE_NONE);
            recordInfos_->AddDefine(recordType, methodId, bcOffset, currentType, superType);
            PGOObjKind kind = PGOObjKind::LOCAL;
            recordInfos_->AddLayout(currentType, JSTaggedType(newHClass), kind);
        }
    } else if (slotValue.IsTrackInfoObject()) {
        auto currentType = PGOSampleType::CreateProfileType(abcId, traceId);
        PGOSampleType superType(ProfileType::PROFILE_TYPE_NONE);
        recordInfos_->AddDefine(recordType, methodId, bcOffset, currentType, superType);
        TrackInfo *trackInfo = TrackInfo::Cast(slotValue.GetTaggedObject());
        auto cachedHClass = trackInfo->GetCachedHClass();
        if (cachedHClass.IsJSHClass()) {
            auto hclass = JSHClass::Cast(cachedHClass.GetTaggedObject());
            PGOObjKind kind = PGOObjKind::ELEMENT;
            recordInfos_->AddLayout(currentType, JSTaggedType(hclass), kind);
        }
        auto elementsKind = trackInfo->GetElementsKind();
        recordInfos_->UpdateElementsKind(currentType, elementsKind);
    }
}

void PGOProfiler::DumpCall(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                           uint32_t slotId, ProfileTypeInfo *profileTypeInfo)
{
    JSTaggedValue slotValue = profileTypeInfo->Get(slotId);
    if (!slotValue.IsInt()) {
        return;
    }
    auto calleeMethodId = slotValue.GetInt();

    PGOSampleType type = PGOSampleType::CreateProfileType(abcId, calleeMethodId);
    ProfileType recordType = GetRecordProfileType(abcId, recordName);
    recordInfos_->AddCallTargetType(recordType, methodId, bcOffset, type);
}

void PGOProfiler::AddObjectInfo(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                                JSHClass *hclass, PGOObjKind kind)
{
    ProfileType recordType = GetRecordProfileType(abcId, recordName);
    if (hclass->IsClassConstructor()) {
        kind = PGOObjKind::CONSTRUCTOR;
        auto rootHClass = JSHClass::FindRootHClass(hclass);
        AddObjectInfoByTraceId(abcId, recordName, methodId, bcOffset, rootHClass, ProfileType::Kind::ClassId, kind);
    } else if (hclass->IsJSArray()) {
        if (kind == PGOObjKind::ELEMENT) {
            auto elementsKind = hclass->GetElementsKind();
            auto profileType = ProfileType(abcId, static_cast<uint32_t>(elementsKind), ProfileType::Kind::ElementId);
            PGOObjectInfo info(profileType, kind);
            recordInfos_->AddObjectInfo(recordType, methodId, bcOffset, info);
        }
    } else if (hclass->IsTypedArray()) {
        // Depend to TypedArray IC
        auto id = static_cast<uint32_t>(hclass->GetObjectType());
        PGOObjectInfo info(ProfileType(abcId, id, ProfileType::Kind::BuiltinsId), kind);
        recordInfos_->AddObjectInfo(recordType, methodId, bcOffset, info);
    } else {
        if (hclass->IsJSObject()) {
            // maybe object literal
            auto rootHClass = JSHClass::FindRootHClass(hclass);
            auto profileKind = ProfileType::Kind::LiteralId;
            if (AddObjectInfoByTraceId(abcId, recordName, methodId, bcOffset, rootHClass, profileKind, kind)) {
                return;
            }
        }
        auto prototype = hclass->GetProto();
        if (prototype.IsJSObject()) {
            // maybe class object
            auto prototypeHClass = prototype.GetTaggedObject()->GetClass();
            auto rootHClass = JSHClass::FindRootHClass(prototypeHClass);
            if (AddObjectInfoByTraceId(abcId, recordName, methodId, bcOffset, rootHClass, ProfileType::Kind::ClassId,
                                       kind)) {
                return;
            }
        }
        auto profileType = ProfileType(abcId, static_cast<uint32_t>(0), ProfileType::Kind::UnknowId);
        PGOObjectInfo info(profileType, kind);
        recordInfos_->AddObjectInfo(recordType, methodId, bcOffset, info);
    }
}

bool PGOProfiler::AddObjectInfoByTraceId(ApEntityId abcId, const CString &recordName, EntityId methodId,
                                         int32_t bcOffset, JSHClass *hclass, ProfileType::Kind classKind,
                                         PGOObjKind kind)
{
    ProfileType recordType = GetRecordProfileType(abcId, recordName);
    auto iter = tracedProfiles_.find(JSTaggedType(hclass));
    if (iter != tracedProfiles_.end()) {
        ProfileType traceType = iter->second;
        traceType.UpdateKind(classKind);
        PGOObjectInfo info(traceType, kind);
        recordInfos_->AddObjectInfo(recordType, methodId, bcOffset, info);
        return true;
    }
    return false;
}

ProfileType PGOProfiler::InsertLiteralTraceId(JSTaggedType hclass, ApEntityId abcId, int32_t traceId)
{
    ProfileType literalType(abcId, traceId, ProfileType::Kind::LiteralId);
    return InsertTraceId(hclass, literalType);
}

ProfileType PGOProfiler::InsertTraceId(JSTaggedType hclass, ProfileType traceType)
{
    if (!isEnable_) {
        return traceType;
    }
    auto result = tracedProfiles_.try_emplace(hclass, traceType);
    return result.first->second;
}

void PGOProfiler::ProcessReferences(const WeakRootVisitor &visitor)
{
    if (!isEnable_) {
        return;
    }
    for (auto iter = tracedProfiles_.begin(); iter != tracedProfiles_.end();) {
        JSTaggedType object = iter->first;
        auto fwd = visitor(reinterpret_cast<TaggedObject *>(object));
        if (fwd == nullptr) {
            iter = tracedProfiles_.erase(iter);
            continue;
        }
        if (fwd != reinterpret_cast<TaggedObject *>(object)) {
            UNREACHABLE();
        }
        ++iter;
    }

    std::vector<JSTaggedType> fwdVector;
    for (auto iter = preDumpWorkList_.begin(); iter != preDumpWorkList_.end();) {
        auto object = reinterpret_cast<TaggedObject *>(*iter);
        auto fwd = visitor(object);
        if (fwd == nullptr) {
            iter = preDumpWorkList_.erase(iter);
            continue;
        }
        if (fwd != object) {
            fwdVector.emplace_back(JSTaggedType(fwd));
            iter = preDumpWorkList_.erase(iter);
            continue;
        }
        ++iter;
    }
    for (auto iter : fwdVector) {
        preDumpWorkList_.emplace(iter);
    }
}

void PGOProfiler::Iterate(const RootVisitor &visitor)
{
    if (!isEnable_) {
        return;
    }
    // If the IC of the method is stable, the current design forces the dump data.
    // Must pause dump during GC.
    for (auto &iter : dumpWorkList_) {
        visitor(Root::ROOT_VM, ObjectSlot(ToUintPtr(&iter)));
    }
}

PGOProfiler::PGOProfiler(EcmaVM *vm, bool isEnable) : vm_(vm), isEnable_(isEnable)
{
    if (isEnable_) {
        recordInfos_ = std::make_unique<PGORecordDetailInfos>(0);
    }
};

PGOProfiler::~PGOProfiler()
{
    Reset(false);
}

void PGOProfiler::Reset(bool isEnable)
{
    isEnable_ = isEnable;
    methodCount_ = 0;
    if (recordInfos_) {
        recordInfos_->Clear();
    } else {
        if (isEnable_) {
            recordInfos_ = std::make_unique<PGORecordDetailInfos>(0);
        }
    }
}

ApEntityId PGOProfiler::GetMethodAbcId(JSFunction *jsFunction)
{
    CString pfName;
    auto jsMethod = jsFunction->GetMethod();
    if (jsMethod.IsMethod()) {
        const auto *pf = Method::Cast(jsMethod)->GetJSPandaFile();
        if (pf != nullptr) {
            pfName = pf->GetJSPandaFileDesc();
        }
    }
    ApEntityId abcId(0);
    if (!PGOProfilerManager::GetInstance()->GetPandaFileId(pfName, abcId) && !pfName.empty()) {
        LOG_ECMA(ERROR) << "Get method abc id failed. abcName: " << pfName;
    }
    return abcId;
}

ApEntityId PGOProfiler::GetRecordId(const CString &recordName)
{
    ApEntityId recordId(0);
    recordInfos_->GetRecordPool()->TryAdd(recordName, recordId);
    return recordId;
}

ProfileType PGOProfiler::GetRecordProfileType(JSFunction *jsFunction, const CString &recordName)
{
    return GetRecordProfileType(GetMethodAbcId(jsFunction), GetRecordId(recordName));
}

ProfileType PGOProfiler::GetRecordProfileType(ApEntityId abcId, const CString &recordName)
{
    return GetRecordProfileType(abcId, GetRecordId(recordName));
}

ProfileType PGOProfiler::GetRecordProfileType(ApEntityId abcId, ApEntityId recordId)
{
    return {abcId, recordId, ProfileType::Kind::LocalRecordId};
}
} // namespace panda::ecmascript::pgo
