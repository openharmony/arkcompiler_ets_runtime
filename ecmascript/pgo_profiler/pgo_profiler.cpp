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
#include <memory>

#include "ecmascript/function_proto_transition_table.h"
#include "ecmascript/elements.h"
#include "ecmascript/enum_conversion.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jit/jit_profiler.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/pgo_profiler/pgo_context.h"
#include "ecmascript/pgo_profiler/pgo_profiler_info.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/pgo_profiler/pgo_trace.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/pgo_profiler/types/pgo_profile_type.h"
#include "ecmascript/pgo_profiler/types/pgo_type_generator.h"
#include "macros.h"

namespace panda::ecmascript::pgo {
void PGOProfiler::ProfileCreateObject(JSTaggedType object, ApEntityId abcId, int32_t traceId)
{
    if (!isEnable_) {
        return;
    }

    JSTaggedValue objectValue(object);
    if (objectValue.IsJSObject()) {
        auto hclass = objectValue.GetTaggedObject()->GetClass();
        hclass->SetParent(vm_->GetJSThread(), JSTaggedValue::Undefined());
        ProfileType traceType(abcId, traceId, ProfileType::Kind::ObjectLiteralId, true);
        InsertProfileTypeSafe(JSTaggedType(hclass), JSTaggedType(hclass), traceType);
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
    auto entityId = ctorMethod->GetMethodId().GetOffset();
    if (!InsertDefinedCtor(entityId)) {
        InsertSkipCtorMethodIdSafe(ctorMethod->GetMethodId());
        return;
    }

    auto ctorMethodHClass = ctorFunc->GetClass();
    ctorMethodHClass->SetParent(vm_->GetJSThread(), JSTaggedValue::Undefined());
    auto ctorRootHClass = JSTaggedType(ctorMethodHClass);
    if (GetProfileType(ctorRootHClass, ctorRootHClass).IsNone()) {
        ProfileType ctorProfileType(GetMethodAbcId(ctorFunc), entityId, ProfileType::Kind::ConstructorId, true);
        InsertProfileTypeSafe(ctorRootHClass, ctorRootHClass, ctorProfileType);
    }

    auto protoOrHClass = ctorFunc->GetProtoOrHClass();
    if (protoOrHClass.IsJSHClass()) {
        auto ihc = JSHClass::Cast(protoOrHClass.GetTaggedObject());
        ihc->SetParent(vm_->GetJSThread(), JSTaggedValue::Undefined());
        auto localRootHClass = JSTaggedType(ihc);
        ProfileType localProfileType(GetMethodAbcId(ctorFunc), entityId, ProfileType::Kind::ClassId, true);
        InsertProfileTypeSafe(localRootHClass, localRootHClass, localProfileType);
        protoOrHClass = ihc->GetProto();
    }
    if (protoOrHClass.IsJSObject()) {
        auto prototypeHClass = protoOrHClass.GetTaggedObject()->GetClass();
        prototypeHClass->SetParent(vm_->GetJSThread(), JSTaggedValue::Undefined());
        auto protoRootHClass = JSTaggedType(prototypeHClass);
        if (GetProfileType(protoRootHClass, protoRootHClass).IsNone()) {
            ProfileType protoProfileType(GetMethodAbcId(ctorFunc), entityId, ProfileType::Kind::PrototypeId, true);
            InsertProfileTypeSafe(protoRootHClass, protoRootHClass, protoProfileType);
        }
    }
}

void PGOProfiler::ProfileClassRootHClass(JSTaggedType ctor, JSTaggedType rootHcValue, ProfileType::Kind kind)
{
    if (!isEnable_) {
        return;
    }

    auto ctorValue = JSTaggedValue(ctor);
    if (!ctorValue.IsJSFunction()) {
        return;
    }
    auto ctorFunc = JSFunction::Cast(ctorValue.GetTaggedObject());
    if (!FunctionKindVerify(ctorFunc)) {
        return;
    }
    auto ctorMethodValue = ctorFunc->GetMethod();
    if (!ctorMethodValue.IsMethod()) {
        return;
    }
    auto ctorMethod = Method::Cast(ctorMethodValue);
    auto entityId = ctorMethod->GetMethodId().GetOffset();
    if (IsSkippableCtor(entityId)) {
        return;
    }

    auto rootHc = JSHClass::Cast(JSTaggedValue(rootHcValue).GetTaggedObject());
    rootHc->SetParent(vm_->GetJSThread(), JSTaggedValue::Undefined());
    if (GetProfileType(rootHcValue, rootHcValue).IsNone()) {
        ProfileType ihcProfileType(GetMethodAbcId(ctorFunc), entityId, kind, true);
        InsertProfileTypeSafe(rootHcValue, rootHcValue, ihcProfileType);
    }
}

void PGOProfiler::ProfileProtoTransitionClass(JSHandle<JSFunction> func,
                                              JSHandle<JSHClass> hclass,
                                              JSHandle<JSTaggedValue> proto)
{
    if (!isEnable_) {
        return;
    }
    auto thread = vm_->GetJSThread();
    JSHClass *phc = proto->GetTaggedObject()->GetClass();
    JSHClass *phcRoot = JSHClass::FindRootHClass(phc);
    auto *transitionTable = thread->GetCurrentEcmaContext()->GetFunctionProtoTransitionTable();
    JSTaggedType baseIhc = transitionTable->GetFakeParent(JSTaggedType(phcRoot));
    if (baseIhc == 0) {
        LOG_ECMA(DEBUG) << "fake parent not found!";
        ProfileClassRootHClass(func.GetTaggedType(), hclass.GetTaggedType());
        return;
    }
    JSTaggedType ihc = func->GetProtoTransRootHClass().GetRawData();
    if (JSTaggedValue(ihc).IsUndefined()) {
        LOG_ECMA(DEBUG) << "maybe the prototype of the current function is just the initial prototype!";
        ProfileClassRootHClass(func.GetTaggedType(), hclass.GetTaggedType());
        return;
    }
    [[maybe_unused]] bool success = transitionTable->TryInsertFakeParentItem(hclass.GetTaggedType(), ihc);
    ASSERT(success == true);  // ihc wont conflict
    // record original ihc type
    ProfileClassRootHClass(func.GetTaggedType(), ihc, ProfileType::Kind::ClassId);
    // record transition ihc type
    ProfileClassRootHClass(func.GetTaggedType(), hclass.GetTaggedType(), ProfileType::Kind::TransitionClassId);
}

void PGOProfiler::ProfileProtoTransitionPrototype(JSHandle<JSFunction> func,
                                                  JSHandle<JSTaggedValue> prototype,
                                                  JSHandle<JSTaggedValue> oldPrototype,
                                                  JSHandle<JSTaggedValue> baseIhc)
{
    if (!isEnable_) {
        return;
    }
    auto method = func->GetMethod();
    if (Method::Cast(method)->IsNativeWithCallField()) {
        return;
    }
    // set prototype once, and just skip this time
    if (!func->GetProtoTransRootHClass().IsUndefined()) {
        return;
    }
    auto thread = vm_->GetJSThread();
    // insert transition item
    JSHandle<JSTaggedValue> transIhc(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> transPhc(thread, prototype->GetTaggedObject()->GetClass());
    if (JSHandle<JSHClass>(baseIhc)->IsDictionaryMode() || JSHandle<JSHClass>(transPhc)->IsDictionaryMode()) {
        return;
    }
    auto *transitionTable = thread->GetCurrentEcmaContext()->GetFunctionProtoTransitionTable();
    bool success = transitionTable->TryInsertFakeParentItem(transPhc.GetTaggedType(), baseIhc.GetTaggedType());
    if (!success) {
        return;
    }
    // Do not generate ihc lazily, beacause it's used for the key of hash table
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> ihc = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT, oldPrototype);
    func->SetProtoTransRootHClass(thread, JSHandle<JSTaggedValue>(ihc));

    // record phc type
    JSHClass *phc0Root = JSHClass::FindRootHClass(oldPrototype->GetTaggedObject()->GetClass());
    ProfileClassRootHClass(func.GetTaggedType(), JSTaggedType(phc0Root), ProfileType::Kind::PrototypeId);
    ProfileClassRootHClass(func.GetTaggedType(), transPhc.GetTaggedType(), ProfileType::Kind::TransitionPrototypeId);
}

void PGOProfiler::ProfileDefineGetterSetter(JSHClass* receiverHClass,
                                            JSHClass* holderHClass,
                                            const JSHandle<JSTaggedValue>& func,
                                            int32_t pcOffset)
{
    if (!isEnable_) {
        return;
    }
    JSTaggedValue funcValue = JSTaggedValue(func.GetTaggedValue());
    if (!funcValue.IsJSFunction()) {
        return;
    }
    auto methodValue = JSFunction::Cast(funcValue)->GetMethod();
    if (!methodValue.IsMethod()) {
        return;
    }

    auto function = JSFunction::Cast(funcValue);
    WorkNode* workNode = reinterpret_cast<WorkNode*>(function->GetWorkNodePointer());
    if (workNode != nullptr) {
        workNode->SetValue(JSTaggedType(function));
        workNode->SetExtraProfileTypeInfo(pcOffset, receiverHClass, holderHClass);
    }
}

void PGOProfiler::UpdateRootProfileTypeSafe(JSHClass* oldHClass, JSHClass* newHClass)
{
    if (!isEnable_) {
        return;
    }
    auto oldRootHClass = JSHClass::FindRootHClass(oldHClass);
    auto iter = tracedProfiles_.find(JSTaggedType(oldRootHClass));
    if (iter == tracedProfiles_.end()) {
        return;
    }
    auto generator = iter->second;
    auto rootProfileType = generator->GetProfileType(JSTaggedType(oldRootHClass));
    {
        LockHolder lock(tracedProfilesMutex_);
        nativeAreaAllocator_->Delete(iter->second);
        tracedProfiles_.erase(iter);
    }
    if (rootProfileType.IsNone()) {
        return;
    }
    newHClass->SetParent(vm_->GetJSThread(), JSTaggedValue::Undefined());
    InsertProfileTypeSafe(JSTaggedType(newHClass), JSTaggedType(newHClass), rootProfileType);
}

void PGOProfiler::UpdateTrackElementsKind(JSTaggedValue trackInfoVal, ElementsKind newKind)
{
    if (trackInfoVal.IsHeapObject() && trackInfoVal.IsWeak()) {
        auto trackInfo = TrackInfo::Cast(trackInfoVal.GetWeakReferentUnChecked());
        auto oldKind = trackInfo->GetElementsKind();
        if (Elements::IsGeneric(oldKind) || oldKind == newKind) {
            return;
        }
        auto mixKind = Elements::MergeElementsKind(oldKind, newKind);
        if (mixKind == oldKind) {
            return;
        }
        trackInfo->SetElementsKind(mixKind);
        auto thread = vm_->GetJSThread();
        auto globalConst = thread->GlobalConstants();
        auto constantId = thread->GetArrayHClassIndexMap().at(mixKind);
        auto hclass = globalConst->GetGlobalConstantObject(static_cast<size_t>(constantId));
        trackInfo->SetCachedHClass(vm_->GetJSThread(), hclass);
        UpdateTrackInfo(JSTaggedValue(trackInfo));
    }
}

void PGOProfiler::UpdateTrackArrayLength(JSTaggedValue trackInfoVal, uint32_t newSize)
{
    if (trackInfoVal.IsHeapObject() && trackInfoVal.IsWeak()) {
        auto trackInfo = TrackInfo::Cast(trackInfoVal.GetWeakReferentUnChecked());
        uint32_t oldSize = trackInfo->GetArrayLength();
        if (oldSize >= newSize) {
            return;
        }
        trackInfo->SetArrayLength(newSize);
        UpdateTrackInfo(JSTaggedValue(trackInfo));
    }
}

void PGOProfiler::UpdateTrackSpaceFlag(TaggedObject *object, RegionSpaceFlag spaceFlag)
{
    if (!object->GetClass()->IsTrackInfoObject()) {
        return;
    }
    auto trackInfo = TrackInfo::Cast(object);
    RegionSpaceFlag oldFlag = trackInfo->GetSpaceFlag();
    if (oldFlag == RegionSpaceFlag::IN_YOUNG_SPACE) {
        trackInfo->SetSpaceFlag(spaceFlag);
    }
}

void PGOProfiler::UpdateTrackInfo(JSTaggedValue trackInfoVal)
{
    if (trackInfoVal.IsHeapObject()) {
        auto trackInfo = TrackInfo::Cast(trackInfoVal.GetTaggedObject());
        auto func = trackInfo->GetCachedFunc();
        if (!func.IsWeak()) {
            return;
        }
        TaggedObject *object = func.GetWeakReferentUnChecked();
        if (!object->GetClass()->IsJSFunction()) {
            return;
        }
        auto profileTypeInfoVal = JSFunction::Cast(object)->GetProfileTypeInfo();
        if (profileTypeInfoVal.IsUndefined()) {
            return;
        }
        auto profileTypeInfo = ProfileTypeInfo::Cast(profileTypeInfoVal.GetTaggedObject());
        if (profileTypeInfo->IsProfileTypeInfoWithBigMethod()) {
            return;
        }
        if (!profileTypeInfo->IsProfileTypeInfoPreDumped()) {
            profileTypeInfo->SetPreDumpPeriodIndex();
            PGOPreDump(JSTaggedType(object));
        }
    }
}

void PGOProfiler::PGODump(JSTaggedType func)
{
    if (!isEnable_ || !vm_->GetJSOptions().IsEnableProfileDump()) {
        return;
    }

    auto funcValue = JSTaggedValue(func);
    if (!funcValue.IsJSFunction()) {
        return;
    }
    auto methodValue = JSFunction::Cast(funcValue)->GetMethod();
    if (!methodValue.IsMethod()) {
        return;
    }
    auto function = JSFunction::Cast(funcValue);
    auto workNode = reinterpret_cast<WorkNode *>(function->GetWorkNodePointer());
    if (workNode == nullptr) {
        workNode = nativeAreaAllocator_->New<WorkNode>(JSTaggedType(function));
        function->SetWorkNodePointer(reinterpret_cast<uintptr_t>(workNode));
        LockHolder lock(mutex_);
        dumpWorkList_.PushBack(workNode);
    } else {
        workNode->SetValue(JSTaggedType(function));
        auto workList = workNode->GetWorkList();
        LockHolder lock(mutex_);
        if (workList == &preDumpWorkList_) {
            preDumpWorkList_.Remove(workNode);
        }
        if (workList != &dumpWorkList_) {
            dumpWorkList_.PushBack(workNode);
        }
    }
    StartPGODump();
}

void PGOProfiler::SuspendByGC()
{
    if (!isEnable_) {
        return;
    }
    LockHolder lock(mutex_);
    if (GetState() == State::START) {
        SetState(State::PAUSE);
        WaitingPGODump();
    } else if (GetState() == State::FORCE_SAVE) {
        SetState(State::FORCE_SAVE_PAUSE);
        WaitingPGODump();
    }
}

void PGOProfiler::ResumeByGC()
{
    if (!isEnable_) {
        return;
    }
    LockHolder lock(mutex_);
    if (GetState() == State::PAUSE) {
        SetState(State::START);
        DispatchPGODumpTask();
    } else if (GetState() == State::FORCE_SAVE_PAUSE) {
        SetState(State::FORCE_SAVE);
        DispatchPGODumpTask();
    }
}

void PGOProfiler::StopPGODump()
{
    LockHolder lock(mutex_);
    if (IsGCWaiting()) {
        NotifyGC("[StopPGODump::PAUSE]");
        return;
    }
    SetState(State::STOP);
    NotifyAll("[StopPGODump::STOP]");
}

void PGOProfiler::StartPGODump()
{
    if (GetState() == State::STOP) {
        SetState(State::START);
        DispatchPGODumpTask();
    }
}

void PGOProfiler::DispatchPGODumpTask()
{
    Taskpool::GetCurrentTaskpool()->PostTask(
        std::make_unique<PGOProfilerTask>(this, vm_->GetJSThread()->GetThreadId()));
}

PGOProfiler::State PGOProfiler::GetState()
{
    return state_.load(std::memory_order_acquire);
}

void PGOProfiler::SetState(State state)
{
    v_.AddLogWithDebugLog("[PGODumpStateChange] " + StateToString(GetState()) + " -> " + StateToString(state));
    state_.store(state, std::memory_order_release);
}

void PGOProfiler::NotifyGC(std::string tag)
{
    v_.AddLogWithDebugLog(tag + " notify GC");
    condition_.SignalAll();
}

void PGOProfiler::NotifyAll(std::string tag)
{
    v_.AddLogWithDebugLog(tag + " notify all");
    condition_.SignalAll();
}

void PGOProfiler::WaitingPGODump()
{
    condition_.Wait(&mutex_);
}

void PGOProfiler::WaitPGODumpFinish()
{
    if (!isEnable_) {
        return;
    }
    LockHolder lock(mutex_);
    while (GetState() == State::START) {
        WaitingPGODump();
    }
}

void PGOProfiler::DumpByForce()
{
    isForce_ = true;
    LockHolder lock(mutex_);
    if (GetState() == State::START) {
        SetState(State::FORCE_SAVE);
        WaitingPGODump();
    } else if (GetState() == State::STOP && !dumpWorkList_.IsEmpty()) {
        SetState(State::FORCE_SAVE);
        WaitingPGODump();
        DispatchPGODumpTask();
    } else if (GetState() == State::PAUSE) {
        SetState(State::FORCE_SAVE_PAUSE);
        WaitingPGODump();
    }
}

bool PGOProfiler::IsGCWaitingWithLock()
{
    if (GetState() == State::PAUSE) {
        LockHolder lock(mutex_);
        if (GetState() == State::PAUSE) {
            return true;
        }
    }
    return false;
}

bool PGOProfiler::IsGCWaiting()
{
    if (GetState() == State::PAUSE) {
        return true;
    }
    return false;
}

void PGOProfiler::PGOPreDump(JSTaggedType func)
{
    if (!isEnable_ || !vm_->GetJSOptions().IsEnableProfileDump()) {
        return;
    }

    auto funcValue = JSTaggedValue(func);
    if (!funcValue.IsJSFunction()) {
        return;
    }
    auto methodValue = JSFunction::Cast(funcValue)->GetMethod();
    if (!methodValue.IsMethod()) {
        return;
    }
    auto function = JSFunction::Cast(funcValue);
    auto workNode = reinterpret_cast<WorkNode *>(function->GetWorkNodePointer());
    if (workNode == nullptr) {
        workNode = nativeAreaAllocator_->New<WorkNode>(JSTaggedType(function));
        function->SetWorkNodePointer(reinterpret_cast<uintptr_t>(workNode));
        LockHolder lock(mutex_);
        preDumpWorkList_.PushBack(workNode);
    } else {
        workNode->SetValue(JSTaggedType(function));
        auto workList = workNode->GetWorkList();
        LockHolder lock(mutex_);
        if (workList == &dumpWorkList_) {
            workList->Remove(workNode);
        }
        if (workList != &preDumpWorkList_) {
            preDumpWorkList_.PushBack(workNode);
        }
    }
}

void PGOProfiler::UpdateExtraProfileTypeInfo(ApEntityId abcId,
                                             const CString& recordName,
                                             EntityId methodId,
                                             WorkNode* current)
{
    auto extraInfo = current->GetExtraProfileTypeInfo();
    for (auto& iter: extraInfo) {
        auto pcOffset = iter.first;
        auto info = iter.second;
        // skip when it is polymorphic
        if (info.IsHole()) {
            continue;
        }
        AddObjectInfo(abcId,
                      recordName,
                      methodId,
                      pcOffset,
                      info.GetReceiverHClass(),
                      info.GetReceiverHClass(),
                      info.GetHolderHClass());
    }
}

void PGOProfiler::HandlePGOPreDump()
{
    ConcurrentGuard guard(v_, "HandlePGOPreDump");
    if (!isEnable_ || !vm_->GetJSOptions().IsEnableProfileDump()) {
        return;
    }
    DISALLOW_GARBAGE_COLLECTION;
    preDumpWorkList_.Iterate([this](WorkNode* current) {
        JSTaggedValue funcValue = JSTaggedValue(current->GetValue());
        if (!funcValue.IsJSFunction()) {
            return;
        }
        auto func = JSFunction::Cast(funcValue);
        if (func->IsSendableFunction()) {
            return;
        }
        JSTaggedValue methodValue = func->GetMethod();
        if (!methodValue.IsMethod()) {
            return;
        }
        JSTaggedValue recordNameValue = func->GetRecordName();
        if (!recordNameValue.IsString()) {
            return;
        }
        CString recordName = ConvertToString(recordNameValue);
        auto abcId = GetMethodAbcId(func);

        if (current->HasExtraProfileTypeInfo()) {
            Method* method = Method::Cast(methodValue.GetTaggedObject());
            EntityId methodId = method->GetMethodId();
            UpdateExtraProfileTypeInfo(abcId, recordName, methodId, current);
        }

        ProfileType recordType = GetRecordProfileType(abcId, recordName);
        recordInfos_->AddMethod(recordType, Method::Cast(methodValue), SampleMode::HOTNESS_MODE);
        ProfileBytecode(abcId, recordName, funcValue);
        if (PGOTrace::GetInstance()->IsEnable()) {
            PGOTrace::GetInstance()->TryGetMethodData(methodValue, false);
        }
    });
}

void PGOProfiler::HandlePGODumpByDumpThread(bool force)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "PGOProfiler::HandlePGODumpByDumpThread");
    ConcurrentGuard guard(v_, "HandlePGODumpByDumpThread");
    if (!isEnable_ || !vm_->GetJSOptions().IsEnableProfileDump()) {
        return;
    }
    DISALLOW_GARBAGE_COLLECTION;
    auto current = PopFromProfileQueue();
    while (current != nullptr) {
        JSTaggedValue value = JSTaggedValue(current->GetValue());
        if (value.IsUndefined()) {
            current = PopFromProfileQueue();
            continue;
        }
        if (!value.IsJSFunction()) {
            current = PopFromProfileQueue();
            continue;
        }
        auto func = JSFunction::Cast(value);
        if (func->IsSendableFunction()) {
            current = PopFromProfileQueue();
            continue;
        }
        JSTaggedValue methodValue = func->GetMethod();
        if (!methodValue.IsMethod()) {
            current = PopFromProfileQueue();
            continue;
        }
        JSTaggedValue recordNameValue = func->GetRecordName();
        if (!recordNameValue.IsString()) {
            current = PopFromProfileQueue();
            continue;
        }
        CString recordName = ConvertToString(recordNameValue);
        auto abcId = GetMethodAbcId(func);

        if (current->HasExtraProfileTypeInfo()) {
            Method* method = Method::Cast(methodValue.GetTaggedObject());
            EntityId methodId = method->GetMethodId();
            UpdateExtraProfileTypeInfo(abcId, recordName, methodId, current);
        }

        ProfileType recordType = GetRecordProfileType(abcId, recordName);
        if (recordInfos_->AddMethod(recordType, Method::Cast(methodValue), SampleMode::HOTNESS_MODE)) {
            methodCount_++;
        }
        ProfileBytecode(abcId, recordName, value);
        current = PopFromProfileQueue();
        if (PGOTrace::GetInstance()->IsEnable()) {
            PGOTrace::GetInstance()->TryGetMethodData(methodValue, true);
        }
    }
    ASSERT(GetState() != State::STOP);
    if (IsGCWaitingWithLock()) {
        return;
    }
    MergeProfilerAndDispatchAsyncSaveTask(force);
}

void PGOProfiler::MergeProfilerAndDispatchAsyncSaveTask(bool force)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "PGOProfiler::MergeProfilerAndDispatchAsyncSaveTask");
    // Merged every 50 methods and merge interval greater than minimal interval
    auto interval = std::chrono::system_clock::now() - saveTimestamp_;
    auto minIntervalOption = vm_->GetJSOptions().GetPGOSaveMinInterval();
    auto mergeMinInterval = std::chrono::milliseconds(minIntervalOption * MS_PRE_SECOND);
    if ((methodCount_ >= MERGED_EVERY_COUNT && interval > mergeMinInterval) || (force && methodCount_ > 0)) {
        LOG_ECMA(DEBUG) << "Sample: post task to save profiler";
        {
            ClockScope start;
            PGOProfilerManager::GetInstance()->Merge(this);
            if (PGOTrace::GetInstance()->IsEnable()) {
                PGOTrace::GetInstance()->SetMergeTime(start.TotalSpentTime());
            }
        }
        if (!force) {
            PGOProfilerManager::GetInstance()->AsyncSave();
        }
        SetSaveTimestamp(std::chrono::system_clock::now());
        methodCount_ = 0;
    }
}

PGOProfiler::WorkNode* PGOProfiler::PopFromProfileQueue()
{
    LockHolder lock(mutex_);
    WorkNode* node = nullptr;
    while (node == nullptr) {
        if (IsGCWaiting()) {
            break;
        }
        if (dumpWorkList_.IsEmpty()) {
            break;
        }
        node = dumpWorkList_.PopFront();
    }
    return node;
}

void PGOProfiler::ProfileBytecode(ApEntityId abcId, const CString &recordName, JSTaggedValue funcValue)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "PGOProfiler::ProfileBytecode");
    ClockScope start;
    JSFunction *function = JSFunction::Cast(funcValue);
    if (function->IsSendableFunction()) {
        return;
    }
    Method *method = Method::Cast(function->GetMethod());
    JSTaggedValue profileTypeInfoVal = function->GetProfileTypeInfo();
    ASSERT(!profileTypeInfoVal.IsUndefined());
    auto profileTypeInfo = ProfileTypeInfo::Cast(profileTypeInfoVal.GetTaggedObject());
    auto methodId = method->GetMethodId();
    auto pcStart = method->GetBytecodeArray();
    auto codeSize = method->GetCodeSize();
    BytecodeInstruction bcIns(pcStart);
    auto bcInsLast = bcIns.JumpTo(codeSize);

    while (bcIns.GetAddress() != bcInsLast.GetAddress()) {
        if (IsGCWaitingWithLock()) {
            break;
        }
        auto opcode = bcIns.GetOpcode();
        auto bcOffset = bcIns.GetAddress() - pcStart;
        auto pc = bcIns.GetAddress();
        switch (opcode) {
            case EcmaOpcode::LDTHISBYNAME_IMM8_ID16:
            case EcmaOpcode::LDOBJBYNAME_IMM8_ID16:
            case EcmaOpcode::LDPRIVATEPROPERTY_IMM8_IMM16_IMM16: {
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
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
                CHECK_SLOTID_BREAK(slotId);
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
            case EcmaOpcode::STTHISBYNAME_IMM8_ID16:
            case EcmaOpcode::DEFINEPROPERTYBYNAME_IMM8_ID16_V8:
            case EcmaOpcode::STPRIVATEPROPERTY_IMM8_IMM16_IMM16_V8: {
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
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
            case EcmaOpcode::STOWNBYINDEX_IMM8_V8_IMM16:
            case EcmaOpcode::STTHISBYVALUE_IMM8_V8: {
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                DumpICByValue(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo, BCType::STORE);
                break;
            }
            case EcmaOpcode::STOBJBYVALUE_IMM16_V8_V8:
            case EcmaOpcode::STOWNBYINDEX_IMM16_V8_IMM16:
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
            case EcmaOpcode::STRICTEQ_IMM8_V8:
            case EcmaOpcode::TONUMERIC_IMM8: {
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                DumpOpType(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo);
                break;
            }
            case EcmaOpcode::CALLRUNTIME_ISTRUE_PREF_IMM8:
            case EcmaOpcode::CALLRUNTIME_ISFALSE_PREF_IMM8: {
                uint8_t slotId = READ_INST_8_1();
                CHECK_SLOTID_BREAK(slotId);
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
                CHECK_SLOTID_BREAK(slotId);
                DumpCall(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo);
                break;
            }
            case EcmaOpcode::CALLRUNTIME_CALLINIT_PREF_IMM8_V8: {
                uint8_t slotId = READ_INST_8_1();
                CHECK_SLOTID_BREAK(slotId);
                DumpCall(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo);
                break;
            }
            case EcmaOpcode::WIDE_CALLRANGE_PREF_IMM16_V8:
            case EcmaOpcode::WIDE_CALLTHISRANGE_PREF_IMM16_V8: {
                // no ic slot
                break;
            }
            case EcmaOpcode::NEWOBJRANGE_IMM8_IMM8_V8: {
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                DumpNewObjRange(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo);
                break;
            }
            case EcmaOpcode::NEWOBJRANGE_IMM16_IMM8_V8: {
                uint16_t slotId = READ_INST_16_0();
                DumpNewObjRange(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo);
                break;
            }
            case EcmaOpcode::WIDE_NEWOBJRANGE_PREF_IMM16_V8: {
                break;
            }
            // Create object
            case EcmaOpcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8: {
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                DumpDefineClass(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo);
                break;
            }
            case EcmaOpcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8: {
                uint16_t slotId = READ_INST_16_0();
                DumpDefineClass(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo);
                break;
            }
            case EcmaOpcode::DEFINEFUNC_IMM8_ID16_IMM8: {
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                DumpDefineClass(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo);
                break;
            }
            case EcmaOpcode::DEFINEFUNC_IMM16_ID16_IMM8: {
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
                CHECK_SLOTID_BREAK(slotId);
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
            case EcmaOpcode::GETITERATOR_IMM8: {
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                DumpGetIterator(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo);
                break;
            }
            case EcmaOpcode::GETITERATOR_IMM16: {
                uint16_t slotId = READ_INST_16_0();
                DumpGetIterator(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo);
                break;
            }
            // Others
            case EcmaOpcode::INSTANCEOF_IMM8_V8: {
                uint8_t slotId = READ_INST_8_0();
                CHECK_SLOTID_BREAK(slotId);
                DumpInstanceof(abcId, recordName, methodId, bcOffset, slotId, profileTypeInfo);
                break;
            }
            case EcmaOpcode::DEFINEGETTERSETTERBYVALUE_V8_V8_V8_V8:
            default:
                break;
        }
        bcIns = bcIns.GetNext();
    }
    if (PGOTrace::GetInstance()->IsEnable()) {
        auto methodData = PGOTrace::GetInstance()->TryGetMethodData(function->GetMethod());
        methodData->SetProfileBytecodeTime(start.TotalSpentTime());
    }
}

void PGOProfiler::DumpICByName(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                               uint32_t slotId, ProfileTypeInfo *profileTypeInfo, BCType type)
{
    JSTaggedValue firstValue = profileTypeInfo->Get(slotId);
    if (!firstValue.IsHeapObject()) {
        if (firstValue.IsHole()) {
            // Mega state
            AddObjectInfoWithMega(abcId, recordName, methodId, bcOffset);
        }
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
        if (firstValue.IsHole()) {
            // Mega state
            AddObjectInfoWithMega(abcId, recordName, methodId, bcOffset);
        }
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
        if (!DumpICByNameWithHandler(abcId, recordName, methodId, bcOffset, hclass, handler, type)) {
            AddObjectInfoWithMega(abcId, recordName, methodId, bcOffset);
            break;
        }
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

bool PGOProfiler::DumpICByNameWithHandler(ApEntityId abcId, const CString &recordName, EntityId methodId,
                                          int32_t bcOffset, JSHClass *hclass, JSTaggedValue secondValue, BCType type)
{
    TryDumpProtoTransitionType(hclass);
    if (type == BCType::LOAD) {
        return DumpICLoadByNameWithHandler(abcId, recordName, methodId, bcOffset, hclass, secondValue);
    }

    if (secondValue.IsInt()) {
        return AddObjectInfo(abcId, recordName, methodId, bcOffset, hclass, hclass, hclass);
    } else if (secondValue.IsTransitionHandler()) {
        auto transitionHandler = TransitionHandler::Cast(secondValue.GetTaggedObject());
        auto transitionHClassVal = transitionHandler->GetTransitionHClass();
        if (transitionHClassVal.IsJSHClass()) {
            auto transitionHClass = JSHClass::Cast(transitionHClassVal.GetTaggedObject());
            return AddObjectInfo(abcId, recordName, methodId, bcOffset, hclass, hclass, transitionHClass);
        }
    } else if (secondValue.IsTransWithProtoHandler()) {
        auto transWithProtoHandler = TransWithProtoHandler::Cast(secondValue.GetTaggedObject());
        auto cellValue = transWithProtoHandler->GetProtoCell();
        if (CheckProtoChangeMarker(cellValue)) {
            return false;
        }
        auto transitionHClassVal = transWithProtoHandler->GetTransitionHClass();
        if (transitionHClassVal.IsJSHClass()) {
            auto transitionHClass = JSHClass::Cast(transitionHClassVal.GetTaggedObject());
            return AddObjectInfo(abcId, recordName, methodId, bcOffset, hclass, hclass, transitionHClass);
        }
    } else if (secondValue.IsPrototypeHandler()) {
        auto prototypeHandler = PrototypeHandler::Cast(secondValue.GetTaggedObject());
        auto cellValue = prototypeHandler->GetProtoCell();
        if (CheckProtoChangeMarker(cellValue)) {
            return false;
        }
        auto holder = prototypeHandler->GetHolder();
        auto holderHClass = holder.GetTaggedObject()->GetClass();
        auto accessorMethodId = prototypeHandler->GetAccessorMethodId();
        return AddObjectInfo(
            abcId, recordName, methodId, bcOffset, hclass, holderHClass, holderHClass, accessorMethodId);
    } else if (secondValue.IsStoreTSHandler()) {
        StoreTSHandler *storeTSHandler = StoreTSHandler::Cast(secondValue.GetTaggedObject());
        auto cellValue = storeTSHandler->GetProtoCell();
        if (CheckProtoChangeMarker(cellValue)) {
            return false;
        }
        auto holder = storeTSHandler->GetHolder();
        auto holderHClass = holder.GetTaggedObject()->GetClass();
        return AddObjectInfo(abcId, recordName, methodId, bcOffset, hclass, holderHClass, holderHClass);
    }
    // StoreGlobal
    return false;
}

bool PGOProfiler::DumpICLoadByNameWithHandler(ApEntityId abcId, const CString &recordName, EntityId methodId,
                                              int32_t bcOffset, JSHClass *hclass, JSTaggedValue secondValue)
{
    bool ret = false;
    if (secondValue.IsInt()) {
        auto handlerInfo = static_cast<uint32_t>(secondValue.GetInt());
        if (HandlerBase::IsNonExist(handlerInfo)) {
            return ret;
        }
        if (HandlerBase::IsField(handlerInfo) || HandlerBase::IsAccessor(handlerInfo)) {
            if (AddObjectInfo(abcId, recordName, methodId, bcOffset, hclass, hclass, hclass)) {
                return true;
            }
        }
        return AddBuiltinsInfoByNameInInstance(abcId, recordName, methodId, bcOffset, hclass);
    } else if (secondValue.IsPrototypeHandler()) {
        auto prototypeHandler = PrototypeHandler::Cast(secondValue.GetTaggedObject());
        auto cellValue = prototypeHandler->GetProtoCell();
        if (CheckProtoChangeMarker(cellValue)) {
            return ret;
        }
        auto holder = prototypeHandler->GetHolder();
        auto holderHClass = holder.GetTaggedObject()->GetClass();
        JSTaggedValue handlerInfoVal = prototypeHandler->GetHandlerInfo();
        if (!handlerInfoVal.IsInt()) {
            return ret;
        }
        auto handlerInfo = static_cast<uint32_t>(handlerInfoVal.GetInt());
        if (HandlerBase::IsNonExist(handlerInfo)) {
            return ret;
        }
        auto accessorMethodId = prototypeHandler->GetAccessorMethodId();
        if (!AddObjectInfo(abcId, recordName, methodId, bcOffset, hclass, holderHClass,
                           holderHClass, accessorMethodId)) {
            return AddBuiltinsInfoByNameInProt(abcId, recordName, methodId, bcOffset, hclass, holderHClass);
        }
        return true;
    }
    // LoadGlobal
    return false;
}

void PGOProfiler::DumpICByValueWithHandler(ApEntityId abcId, const CString &recordName, EntityId methodId,
                                           int32_t bcOffset, JSHClass *hclass, JSTaggedValue secondValue, BCType type)
{
    TryDumpProtoTransitionType(hclass);
    if (type == BCType::LOAD) {
        if (secondValue.IsInt()) {
            auto handlerInfo = static_cast<uint32_t>(secondValue.GetInt());
            if (HandlerBase::IsNormalElement(handlerInfo) || HandlerBase::IsStringElement(handlerInfo)) {
                if (HandlerBase::NeedSkipInPGODump(handlerInfo)) {
                    return;
                }
                AddBuiltinsInfo(abcId, recordName, methodId, bcOffset, hclass, hclass);
                return;
            }

            if (HandlerBase::IsTypedArrayElement(handlerInfo)) {
                OnHeapMode onHeap =  HandlerBase::IsOnHeap(handlerInfo) ? OnHeapMode::ON_HEAP : OnHeapMode::NOT_ON_HEAP;
                AddBuiltinsInfo(abcId, recordName, methodId, bcOffset, hclass, hclass, onHeap);
                return;
            }

            AddObjectInfo(abcId, recordName, methodId, bcOffset, hclass, hclass, hclass);
        }
        return;
    }
    if (secondValue.IsInt()) {
        auto handlerInfo = static_cast<uint32_t>(secondValue.GetInt());
        if (HandlerBase::IsNormalElement(handlerInfo) || HandlerBase::IsStringElement(handlerInfo)) {
            AddBuiltinsInfo(abcId, recordName, methodId, bcOffset, hclass, hclass,
                            OnHeapMode::NONE, HandlerBase::IsStoreOutOfBounds(handlerInfo));
            return;
        }

        if (HandlerBase::IsTypedArrayElement(handlerInfo)) {
            OnHeapMode onHeap = HandlerBase::IsOnHeap(handlerInfo) ? OnHeapMode::ON_HEAP : OnHeapMode::NOT_ON_HEAP;
            AddBuiltinsInfo(abcId, recordName, methodId, bcOffset, hclass, hclass, onHeap,
                            HandlerBase::IsStoreOutOfBounds(handlerInfo));
            return;
        }

        AddObjectInfo(abcId, recordName, methodId, bcOffset, hclass, hclass, hclass);
    } else if (secondValue.IsTransitionHandler()) {
        auto transitionHandler = TransitionHandler::Cast(secondValue.GetTaggedObject());
        auto transitionHClassVal = transitionHandler->GetTransitionHClass();

        auto handlerInfoValue = transitionHandler->GetHandlerInfo();
        ASSERT(handlerInfoValue.IsInt());
        auto handlerInfo = static_cast<uint32_t>(handlerInfoValue.GetInt());
        if (transitionHClassVal.IsJSHClass()) {
            auto transitionHClass = JSHClass::Cast(transitionHClassVal.GetTaggedObject());
            if (HandlerBase::IsElement(handlerInfo)) {
                AddBuiltinsInfo(abcId, recordName, methodId, bcOffset, hclass, transitionHClass,
                                OnHeapMode::NONE, HandlerBase::IsStoreOutOfBounds(handlerInfo));
                return;
            }
            AddObjectInfo(abcId, recordName, methodId, bcOffset, hclass, hclass, transitionHClass);
        }
    } else if (secondValue.IsTransWithProtoHandler()) {
        auto transWithProtoHandler = TransWithProtoHandler::Cast(secondValue.GetTaggedObject());
        auto transitionHClassVal = transWithProtoHandler->GetTransitionHClass();

        auto handlerInfoValue = transWithProtoHandler->GetHandlerInfo();
        ASSERT(handlerInfoValue.IsInt());
        auto handlerInfo = static_cast<uint32_t>(handlerInfoValue.GetInt());
        if (transitionHClassVal.IsJSHClass()) {
            auto transitionHClass = JSHClass::Cast(transitionHClassVal.GetTaggedObject());
            if (HandlerBase::IsElement(handlerInfo)) {
                AddBuiltinsInfo(abcId, recordName, methodId, bcOffset, hclass, transitionHClass,
                                OnHeapMode::NONE, HandlerBase::IsStoreOutOfBounds(handlerInfo));
                return;
            }
            AddObjectInfo(abcId, recordName, methodId, bcOffset, hclass, hclass, transitionHClass);
        }
    } else {
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
            AddBuiltinsInfo(abcId, recordName, methodId, bcOffset, hclass, hclass,
                            OnHeapMode::NONE, HandlerBase::IsStoreOutOfBounds(handlerInfo));
            return;
        }
        auto holder = prototypeHandler->GetHolder();
        auto holderHClass = holder.GetTaggedObject()->GetClass();
        AddObjectInfo(abcId, recordName, methodId, bcOffset, hclass, holderHClass, holderHClass);
    }
}

void PGOProfiler::TryDumpProtoTransitionType(JSHClass *hclass)
{
    JSHClass *ihc1 = JSHClass::FindRootHClass(hclass);
    auto transitionType = GetProfileTypeSafe(JSTaggedType(ihc1), JSTaggedType(ihc1));
    if (transitionType.IsNone() || !transitionType.IsTransitionClassType()) {
        return;
    }
    ASSERT(transitionType.IsRootType());
    JSTaggedValue phc1Root = JSHClass::FindProtoRootHClass(ihc1);

    auto thread = vm_->GetJSThread();
    auto *transitionTable = thread->GetCurrentEcmaContext()->GetFunctionProtoTransitionTable();
    JSTaggedType ihc0 = transitionTable->GetFakeParent(JSTaggedType(ihc1));
    JSTaggedType baseIhc = transitionTable->GetFakeParent(phc1Root.GetRawData());
    if ((ihc0 == 0) || (baseIhc == 0)) {
        return;
    }
    UpdateLayout(JSHClass::Cast(JSTaggedValue(ihc0).GetTaggedObject()));
    UpdateLayout(ihc1);
    UpdateLayout(JSHClass::Cast(JSTaggedValue(baseIhc).GetTaggedObject()));

    auto ihc0RootType = GetProfileTypeSafe(ihc0, ihc0);
    ASSERT(ihc0RootType.IsRootType());
    auto transitionProtoType = GetProfileTypeSafe(phc1Root.GetRawData(), phc1Root.GetRawData());
    ASSERT(transitionProtoType.IsRootType());
    auto baseRootHClass = JSHClass::FindRootHClass(JSHClass::Cast(JSTaggedValue(baseIhc).GetTaggedObject()));
    auto baseRootType = GetProfileTypeSafe(JSTaggedType(baseRootHClass), JSTaggedType(baseRootHClass));
    if (!baseRootType.IsRootType()) {
        LOG_ECMA(WARN) << "Unsupported prototypes which cannot be recorded!";
        return;
    }
    auto baseType = GetProfileTypeSafe(JSTaggedType(baseRootHClass), baseIhc);
    ASSERT(!baseType.IsNone());
    PGOProtoTransitionType protoTransitionType(ihc0RootType);
    protoTransitionType.SetBaseType(baseRootType, baseType);
    protoTransitionType.SetTransitionType(transitionType);
    protoTransitionType.SetTransitionProtoPt(transitionProtoType);

    recordInfos_->GetProtoTransitionPool()->Add(protoTransitionType);
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

bool PGOProfiler::FunctionKindVerify(const JSFunction *ctorFunction)
{
    FunctionKind kind = Method::Cast(ctorFunction->GetMethod())->GetFunctionKind();
    return kind == FunctionKind::BASE_CONSTRUCTOR ||
           kind == FunctionKind::CLASS_CONSTRUCTOR ||
           kind == FunctionKind::DERIVED_CONSTRUCTOR;
}

void PGOProfiler::DumpDefineClass(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                                  uint32_t slotId, ProfileTypeInfo *profileTypeInfo)
{
    JSTaggedValue slotValue = profileTypeInfo->Get(slotId);
    if (!slotValue.IsProfileTypeInfoCell0()) {
        return;
    }
    JSTaggedValue handle = ProfileTypeInfoCell::Cast(slotValue)->GetHandle();
    if (!handle.IsHeapObject() || !handle.IsWeak()) {
        return;
    }
    auto object = handle.GetWeakReferentUnChecked();
    if (object->GetClass()->IsJSFunction()) {
        JSFunction *ctorFunction = JSFunction::Cast(object);
        auto ctorMethod = ctorFunction->GetMethod();
        if (!ctorMethod.IsMethod()) {
            return;
        }
        if (!FunctionKindVerify(ctorFunction)) {
            return;
        }
        ApEntityId ctorAbcId = GetMethodAbcId(ctorFunction);
        auto ctorJSMethod = Method::Cast(ctorMethod);
        auto ctorMethodId = ctorJSMethod->GetMethodId().GetOffset();
        ProfileType recordType = GetRecordProfileType(abcId, recordName);

        auto localType = PGOSampleType::CreateProfileType(ctorAbcId, ctorMethodId, ProfileType::Kind::ClassId, true);
        PGODefineOpType objDefType(localType.GetProfileType());
        auto protoOrHClass = ctorFunction->GetProtoOrHClass();
        if (protoOrHClass.IsJSHClass()) {
            auto hclass = JSHClass::Cast(protoOrHClass.GetTaggedObject());
            auto result = InsertProfileTypeSafe(JSTaggedType(hclass), JSTaggedType(hclass), localType.GetProfileType());
            if (!result) {
                LOG_COMPILER(DEBUG)
                    << "DumpDefineClass InsertFail, May already exist , Method Name "
                    << Method::Cast(ctorFunction->GetMethod())->GetMethodName();
            }
            recordInfos_->AddRootLayout(JSTaggedType(hclass), localType.GetProfileType());
            protoOrHClass = hclass->GetProto();
        }

        auto ctorHClass = ctorFunction->GetJSHClass();
        auto ctorRootHClass = JSTaggedType(JSHClass::FindRootHClass(ctorHClass));
        auto ctorType = GetProfileTypeSafe(ctorRootHClass, ctorRootHClass);
        if (ctorType.IsNone()) {
            LOG_ECMA(DEBUG) << "The profileType of constructor root hclass was not found.";
        } else {
            objDefType.SetCtorPt(ctorType);
            recordInfos_->AddRootLayout(ctorRootHClass, ctorType);
        }

        if (protoOrHClass.IsJSObject()) {
            auto prototypeObj = JSObject::Cast(protoOrHClass);
            auto prototypeHClass = prototypeObj->GetClass();
            auto prototypeRootHClass = JSTaggedType(JSHClass::FindRootHClass(prototypeHClass));
            auto prototypeType = GetProfileType(prototypeRootHClass, prototypeRootHClass);
            if (prototypeType.IsNone()) {
                LOG_ECMA(DEBUG) << "The profileType of prototype root hclass was not found.";
            } else {
                objDefType.SetProtoTypePt(prototypeType);
                recordInfos_->AddRootLayout(prototypeRootHClass, prototypeType);
            }
        }

        recordInfos_->AddDefine(recordType, methodId, bcOffset, objDefType);
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
            auto rootHClass = JSHClass::FindRootHClass(newHClass);
            ASSERT(rootHClass->IsJSObject());
            auto profileType = GetProfileTypeSafe(JSTaggedType(rootHClass), JSTaggedType(rootHClass));
            if (profileType.IsNone()) {
                return;
            }
            ASSERT(profileType.GetKind() == ProfileType::Kind::ObjectLiteralId);
            PGOSampleType currentType(profileType);
            PGODefineOpType objDefType(profileType);
            recordInfos_->AddDefine(recordType, methodId, bcOffset, objDefType);
            recordInfos_->AddRootLayout(JSTaggedType(rootHClass), profileType);
        }
    } else if (slotValue.IsTrackInfoObject()) {
        auto currentType = PGOSampleType::CreateProfileType(abcId, traceId, ProfileType::Kind::ArrayLiteralId, true);
        auto profileType = currentType.GetProfileType();
        PGODefineOpType objDefType(profileType);
        TrackInfo *trackInfo = TrackInfo::Cast(slotValue.GetTaggedObject());
        auto elementsKind = trackInfo->GetElementsKind();
        objDefType.SetElementsKind(elementsKind);
        objDefType.SetElementsLength(trackInfo->GetArrayLength());
        objDefType.SetSpaceFlag(trackInfo->GetSpaceFlag());
        recordInfos_->AddDefine(recordType, methodId, bcOffset, objDefType);
        auto cachedHClass = trackInfo->GetCachedHClass();
        if (cachedHClass.IsJSHClass()) {
            auto hclass = JSHClass::Cast(cachedHClass.GetTaggedObject());
            recordInfos_->AddRootLayout(JSTaggedType(hclass), profileType);
        }
    }
}

void PGOProfiler::DumpCall(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                           uint32_t slotId, ProfileTypeInfo *profileTypeInfo)
{
    JSTaggedValue slotValue = profileTypeInfo->Get(slotId);
    ProfileType::Kind kind;
    int calleeMethodId = 0;
    ApEntityId calleeAbcId = 0;
    if (slotValue.IsInt()) {
        calleeMethodId = slotValue.GetInt();
        calleeAbcId = abcId;
        ASSERT(calleeMethodId <= 0);
        if (calleeMethodId == 0) {
            return;
        }
        kind = ProfileType::Kind::BuiltinFunctionId;
    } else if (slotValue.IsJSFunction()) {
        JSFunction *callee = JSFunction::Cast(slotValue);
        Method *calleeMethod = Method::Cast(callee->GetMethod());
        calleeMethodId = static_cast<int>(calleeMethod->GetMethodId().GetOffset());
        calleeAbcId = GetMethodAbcId(callee->GetMethod());
        kind = ProfileType::Kind::MethodId;
    } else {
        return;
    }
    PGOSampleType type = PGOSampleType::CreateProfileType(calleeAbcId, std::abs(calleeMethodId), kind);
    ProfileType recordType = GetRecordProfileType(abcId, recordName);
    recordInfos_->AddCallTargetType(recordType, methodId, bcOffset, type);
}

void PGOProfiler::DumpGetIterator(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                                  uint32_t slotId, ProfileTypeInfo *profileTypeInfo)
{
    if (vm_->GetJSThread()->GetEnableLazyBuiltins()) {
        return;
    }
    JSTaggedValue value = profileTypeInfo->Get(slotId);
    if (!value.IsInt()) {
        return;
    }
    int iterKind = value.GetInt();
    ASSERT(iterKind <= 0);
    ProfileType::Kind pgoKind = ProfileType::Kind::BuiltinFunctionId;
    PGOSampleType type = PGOSampleType::CreateProfileType(abcId, std::abs(iterKind), pgoKind);
    ProfileType recordType = GetRecordProfileType(abcId, recordName);
    recordInfos_->AddCallTargetType(recordType, methodId, bcOffset, type);
}

void PGOProfiler::DumpNewObjRange(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                                  uint32_t slotId, ProfileTypeInfo *profileTypeInfo)
{
    JSTaggedValue slotValue = profileTypeInfo->Get(slotId);
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
    PGOSampleType type;
    if (ctorMethodId > 0) {
        type = PGOSampleType::CreateProfileType(abcId, ctorMethodId, ProfileType::Kind::ClassId, true);
    } else {
        auto kind = ProfileType::Kind::BuiltinFunctionId;
        type = PGOSampleType::CreateProfileType(abcId, std::abs(ctorMethodId), kind);
    }
    ProfileType recordType = GetRecordProfileType(abcId, recordName);
    recordInfos_->AddCallTargetType(recordType, methodId, bcOffset, type);
}

void PGOProfiler::DumpInstanceof(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                                 uint32_t slotId, ProfileTypeInfo *profileTypeInfo)
{
    JSTaggedValue firstValue = profileTypeInfo->Get(slotId);
    if (!firstValue.IsHeapObject()) {
        if (firstValue.IsHole()) {
            // Mega state
            AddObjectInfoWithMega(abcId, recordName, methodId, bcOffset);
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
            AddObjectInfo(abcId, recordName, methodId, bcOffset, hclass, hclass, hclass);
        }
        return;
    }
    // Poly Not Consider now
    return;
}

void PGOProfiler::UpdateLayout(JSHClass *hclass)
{
    auto parentHClass = hclass->GetParent();
    if (parentHClass.IsJSHClass()) {
        UpdateTransitionLayout(JSHClass::Cast(parentHClass.GetTaggedObject()), hclass);
    } else {
        auto rootHClass = JSHClass::FindRootHClass(hclass);
        auto rootHClassVal = JSTaggedType(rootHClass);
        auto rootType = GetProfileTypeSafe(rootHClassVal, rootHClassVal);
        if (rootType.IsNone()) {
            return;
        }

        auto prototypeHClass = JSHClass::FindProtoRootHClass(rootHClass);
        if (prototypeHClass.IsJSHClass()) {
            auto prototypeValue = prototypeHClass.GetRawData();
            auto prototypeType = GetProfileTypeSafe(prototypeValue, prototypeValue);
            if (!prototypeType.IsNone()) {
                recordInfos_->AddRootPtType(rootType, prototypeType);
                UpdateLayout(JSHClass::Cast(prototypeHClass.GetTaggedObject()));
            }
        }

        auto curType = GetOrInsertProfileTypeSafe(rootHClassVal, JSTaggedType(hclass));
        recordInfos_->UpdateLayout(rootType, JSTaggedType(hclass), curType);
    }
}

void PGOProfiler::UpdateTransitionLayout(JSHClass* parent, JSHClass* child)
{
    auto rootHClass = JSHClass::FindRootHClass(parent);
    auto rootHClassVal = JSTaggedType(rootHClass);
    auto rootType = GetProfileTypeSafe(rootHClassVal, rootHClassVal);
    if (rootType.IsNone()) {
        return;
    }
    auto curHClass = JSTaggedType(child);
    auto curType = GetOrInsertProfileTypeSafe(rootHClassVal, curHClass);
    CVector<JSTaggedType> hclassVec;
    CVector<ProfileType> typeVec;
    hclassVec.push_back(curHClass);
    typeVec.push_back(curType);

    auto parentHClass = JSTaggedValue(parent);
    auto parentHCValue = JSTaggedType(parent);
    auto parentType = GetOrInsertProfileTypeSafe(rootHClassVal, parentHCValue);
    while (parentHClass.IsJSHClass()) {
        parentHClass = JSHClass::Cast(parentHClass.GetTaggedObject())->GetParent();
        if (!parentHClass.IsJSHClass()) {
            break;
        }
        hclassVec.push_back(parentHCValue);
        typeVec.push_back(parentType);
        parentHCValue = JSTaggedType(parentHClass.GetTaggedObject());
        parentType = GetOrInsertProfileTypeSafe(rootHClassVal, parentHCValue);
    }

    auto prototypeHClass = JSHClass::FindProtoRootHClass(rootHClass);
    if (prototypeHClass.IsJSHClass()) {
        auto prototypeValue = prototypeHClass.GetRawData();
        auto prototypeType = GetProfileType(prototypeValue, prototypeValue);
        if (!prototypeType.IsNone()) {
            recordInfos_->AddRootPtType(rootType, prototypeType);
            UpdateLayout(JSHClass::Cast(prototypeHClass.GetTaggedObject()));
        }
    }

    int32_t size = static_cast<int32_t>(hclassVec.size());
    for (int32_t i = size - 1; i >= 0; i--) {
        curHClass = hclassVec[i];
        curType = typeVec[i];
        recordInfos_->UpdateTransitionLayout(rootType, parentHCValue, parentType, curHClass, curType);
        parentHCValue = curHClass;
        parentType = curType;
    }
}

bool PGOProfiler::AddTransitionObjectInfo(ProfileType recordType,
                                          EntityId methodId,
                                          int32_t bcOffset,
                                          JSHClass* receiver,
                                          JSHClass* hold,
                                          JSHClass* holdTra,
                                          PGOSampleType accessorMethod)
{
    auto receiverRootHClass = JSTaggedType(JSHClass::FindRootHClass(receiver));
    auto receiverRootType = GetProfileTypeSafe(receiverRootHClass, receiverRootHClass);
    if (receiverRootType.IsNone()) {
        return false;
    }
    auto receiverType = GetOrInsertProfileTypeSafe(receiverRootHClass, JSTaggedType(receiver));

    auto holdRootHClass = JSTaggedType(JSHClass::FindRootHClass(hold));
    auto holdRootType = GetProfileTypeSafe(holdRootHClass, holdRootHClass);
    if (holdRootType.IsNone()) {
        return true;
    }
    auto holdType = GetOrInsertProfileTypeSafe(holdRootHClass, JSTaggedType(hold));

    auto holdTraRootHClass = JSTaggedType(JSHClass::FindRootHClass(holdTra));
    auto holdTraRootType = GetProfileTypeSafe(holdTraRootHClass, holdTraRootHClass);
    if (holdTraRootType.IsNone()) {
        return true;
    }
    auto holdTraType = GetOrInsertProfileTypeSafe(holdTraRootHClass, JSTaggedType(holdTra));

    if (receiver != hold) {
        UpdateLayout(receiver);
    }

    if (holdType == holdTraType) {
        UpdateLayout(hold);
    } else {
        UpdateTransitionLayout(hold, holdTra);
    }

    PGOObjectInfo info(receiverRootType, receiverType, holdRootType, holdType, holdTraRootType, holdTraType,
                       accessorMethod);
    UpdatePrototypeChainInfo(receiver, hold, info);
    recordInfos_->AddObjectInfo(recordType, methodId, bcOffset, info);
    return true;
}

bool PGOProfiler::AddObjectInfo(ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset,
                                JSHClass *receiver, JSHClass *hold, JSHClass *holdTra, uint32_t accessorMethodId)
{
    PGOSampleType accessor = PGOSampleType::CreateProfileType(abcId, accessorMethodId, ProfileType::Kind::MethodId);
    ProfileType recordType = GetRecordProfileType(abcId, recordName);
    return AddTransitionObjectInfo(recordType, methodId, bcOffset, receiver, hold, holdTra, accessor);
}

void PGOProfiler::UpdatePrototypeChainInfo(JSHClass *receiver, JSHClass *holder, PGOObjectInfo &info)
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
        auto protoRootType = GetProfileTypeSafe(protoRootHClass, protoRootHClass);
        if (protoRootType.IsNone()) {
            break;
        }
        auto protoType = GetOrInsertProfileTypeSafe(protoRootHClass, JSTaggedType(protoHClass));
        protoChain.emplace_back(protoRootType, protoType);
        proto = JSHClass::FindProtoHClass(protoHClass);
    }
    if (!protoChain.empty()) {
        info.AddPrototypePt(protoChain);
    }
}

void PGOProfiler::AddObjectInfoWithMega(
    ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset)
{
    auto megaType = ProfileType::CreateMegaType();
    PGOObjectInfo info(megaType, megaType, megaType, megaType, megaType, megaType, PGOSampleType());
    ProfileType recordType = GetRecordProfileType(abcId, recordName);
    recordInfos_->AddObjectInfo(recordType, methodId, bcOffset, info);
}

bool PGOProfiler::AddBuiltinsInfoByNameInInstance(ApEntityId abcId, const CString &recordName, EntityId methodId,
    int32_t bcOffset, JSHClass *receiver)
{
    auto thread = vm_->GetJSThread();
    auto type = receiver->GetObjectType();
    const auto &ctorEntries = thread->GetCtorHclassEntries();
    auto entry = ctorEntries.find(receiver);
    if (entry != ctorEntries.end()) {
        AddBuiltinsGlobalInfo(abcId, recordName, methodId, bcOffset, entry->second);
        return true;
    }

    auto builtinsId = ToBuiltinsTypeId(type);
    if (!builtinsId.has_value()) {
        return false;
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
                AddBuiltinsGlobalInfo(abcId, recordName, methodId, bcOffset, globalsId);
                return true;
            }
        }
        return false;
    }

    return AddBuiltinsInfo(abcId, recordName, methodId, bcOffset, receiver, receiver);
}

bool PGOProfiler::AddBuiltinsInfoByNameInProt(ApEntityId abcId, const CString &recordName, EntityId methodId,
    int32_t bcOffset, JSHClass *receiver, JSHClass *hold)
{
    auto type = receiver->GetObjectType();
    auto builtinsId = ToBuiltinsTypeId(type);
    if (!builtinsId.has_value()) {
        return false;
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
            return false;
        }
    } else if (IsTypedArrayType(builtinsId.value())) {
        auto exceptRecvHClassOnHeap = thread->GetBuiltinExtraHClass(builtinsId.value());
        ASSERT_PRINT(exceptRecvHClassOnHeap == nullptr || exceptRecvHClassOnHeap->IsOnHeapFromBitField(),
                     "must be on heap");
        if (IsJSHClassNotEqual(receiver, hold, exceptRecvHClass, exceptRecvHClassOnHeap,
                               exceptHoldHClass, exceptPrototypeOfPrototypeHClass)) {
            return false;
        }
    } else if (exceptRecvHClass != receiver || exceptHoldHClass != hold) {
        return false;
    }

    return AddBuiltinsInfo(abcId, recordName, methodId, bcOffset, receiver, receiver);
}

bool PGOProfiler::IsJSHClassNotEqual(JSHClass *receiver, JSHClass *hold, JSHClass *exceptRecvHClass,
                                     JSHClass *exceptRecvHClassOnHeap, JSHClass *exceptHoldHClass,
                                     JSHClass *exceptPrototypeOfPrototypeHClass)
{
    //exceptRecvHClass = IHC, exceptRecvHClassOnHeap = IHC OnHeap
    //exceptHoldHClass = PHC, exceptPrototypeOfPrototypeHClass = HClass of X.prototype.prototype
    return ((exceptRecvHClass != receiver && exceptRecvHClassOnHeap != receiver) ||
            (exceptHoldHClass != hold && exceptPrototypeOfPrototypeHClass != hold));
}

bool PGOProfiler::CheckProtoChangeMarker(JSTaggedValue cellValue) const
{
    if (!cellValue.IsProtoChangeMarker()) {
        return true;
    }
    ProtoChangeMarker *cell = ProtoChangeMarker::Cast(cellValue.GetTaggedObject());
    return cell->GetHasChanged();
}

void PGOProfiler::AddBuiltinsGlobalInfo(ApEntityId abcId, const CString &recordName, EntityId methodId,
                                        int32_t bcOffset, GlobalIndex globalsId)
{
    ProfileType recordType = GetRecordProfileType(abcId, recordName);
    PGOObjectInfo info(ProfileType::CreateGlobals(abcId, globalsId));
    recordInfos_->AddObjectInfo(recordType, methodId, bcOffset, info);
}

bool PGOProfiler::AddBuiltinsInfo(
    ApEntityId abcId, const CString &recordName, EntityId methodId, int32_t bcOffset, JSHClass *receiver,
    JSHClass *transitionHClass, OnHeapMode onHeap, bool everOutOfBounds)
{
    ProfileType recordType = GetRecordProfileType(abcId, recordName);
    if (receiver->IsJSArray()) {
        auto type = receiver->GetObjectType();
        auto elementsKind = receiver->GetElementsKind();
        auto transitionElementsKind = transitionHClass->GetElementsKind();
        auto profileType = ProfileType::CreateBuiltinsArray(abcId, type, elementsKind, transitionElementsKind,
                                                            everOutOfBounds);
        PGOObjectInfo info(profileType);
        recordInfos_->AddObjectInfo(recordType, methodId, bcOffset, info);
    } else if (receiver->IsTypedArray()) {
        JSType jsType = receiver->GetObjectType();
        auto profileType = ProfileType::CreateBuiltinsTypedArray(abcId, jsType, onHeap, everOutOfBounds);
        PGOObjectInfo info(profileType);
        recordInfos_->AddObjectInfo(recordType, methodId, bcOffset, info);
    } else {
        auto type = receiver->GetObjectType();
        PGOObjectInfo info(ProfileType::CreateBuiltins(abcId, type));
        recordInfos_->AddObjectInfo(recordType, methodId, bcOffset, info);
    }
    return true;
}

bool PGOProfiler::IsRecoredTransRootType(ProfileType type)
{
    if (!type.IsRootType() || !type.IsTransitionType()) {
        return false;
    }
    if (std::find(recordedTransRootType_.begin(), recordedTransRootType_.end(), type) != recordedTransRootType_.end()) {
        LOG_ECMA(DEBUG) << "forbide to add more than 1 hclass for a root type!";
        return true;
    }
    recordedTransRootType_.emplace_back(type);
    return false;
}

bool PGOProfiler::InsertProfileTypeSafe(JSTaggedType root, JSTaggedType child, ProfileType traceType)
{
    if (!isEnable_) {
        return false;
    }
    if (IsRecoredTransRootType(traceType)) {
        return false;
    }
    auto iter = tracedProfiles_.find(root);
    if (iter != tracedProfiles_.end()) {
        auto generator = iter->second;
        return generator->InsertProfileType(child, traceType);
    } else {
        LockHolder lock(tracedProfilesMutex_);
        auto generator = nativeAreaAllocator_->New<PGOTypeGenerator>();
        generator->InsertProfileType(child, traceType);
        tracedProfiles_.emplace(root, generator);
        return true;
    }
}

ProfileType PGOProfiler::GetProfileType(JSTaggedType root, JSTaggedType child)
{
    auto iter = tracedProfiles_.find(root);
    if (iter == tracedProfiles_.end()) {
        return ProfileType::PROFILE_TYPE_NONE;
    }
    auto generator = iter->second;
    auto result = generator->GetProfileType(child);
    if (IsSkippableObjectTypeSafe(result)) {
        return ProfileType::PROFILE_TYPE_NONE;
    }
    return result;
}

ProfileType PGOProfiler::GetProfileTypeSafe(JSTaggedType root, JSTaggedType child)
{
    LockHolder lock(tracedProfilesMutex_);
    return GetProfileType(root, child);
}

ProfileType PGOProfiler::GetOrInsertProfileTypeSafe(JSTaggedType root, JSTaggedType child)
{
    LockHolder lock(tracedProfilesMutex_);
    auto iter = tracedProfiles_.find(root);
    if (iter == tracedProfiles_.end()) {
        return ProfileType::PROFILE_TYPE_NONE;
    }
    auto generator = iter->second;
    auto rootType = generator->GetProfileType(root);
    if (rootType.IsNone()) {
        return ProfileType::PROFILE_TYPE_NONE;
    }
    return generator->GenerateProfileType(rootType, child);
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
            nativeAreaAllocator_->Delete(iter->second);
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
    preDumpWorkList_.Iterate([this, &visitor](WorkNode *node) {
        auto object = reinterpret_cast<TaggedObject *>(node->GetValue());
        auto fwd = visitor(object);
        if (fwd == nullptr) {
            preDumpWorkList_.Remove(node);
            nativeAreaAllocator_->Delete(node);
            return;
        }
        if (fwd != object) {
            node->SetValue(JSTaggedType(fwd));
        }
        node->ProcessExtraProfileTypeInfo(visitor);
    });
}

void PGOProfiler::Iterate(const RootVisitor &visitor)
{
    if (!isEnable_) {
        return;
    }
    // If the IC of the method is stable, the current design forces the dump data.
    // Must pause dump during GC.
    dumpWorkList_.Iterate([&visitor](WorkNode* node) {
        visitor(Root::ROOT_VM, ObjectSlot(node->GetValueAddr()));
        node->IterateExtraProfileTypeInfo(visitor);
    });
    preDumpWorkList_.Iterate([&visitor](WorkNode* node) { node->IterateExtraProfileTypeInfo(visitor); });
}

PGOProfiler::PGOProfiler(EcmaVM* vm, bool isEnable)
    : nativeAreaAllocator_(std::make_unique<NativeAreaAllocator>()), vm_(vm), isEnable_(isEnable)
{
    if (isEnable_) {
        recordInfos_ = std::make_unique<PGORecordDetailInfos>(0);
    }
};

PGOProfiler::~PGOProfiler()
{
    Reset(false);
    for (auto iter : tracedProfiles_) {
        nativeAreaAllocator_->Delete(iter.second);
    }
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

ApEntityId PGOProfiler::GetMethodAbcId(JSTaggedValue jsMethod)
{
    ASSERT(jsMethod.IsMethod());
    CString pfName;

    const auto *pf = Method::Cast(jsMethod)->GetJSPandaFile();
    if (pf != nullptr) {
        pfName = pf->GetJSPandaFileDesc();
    }
    ApEntityId abcId(0);
    if (!PGOProfilerManager::GetInstance()->GetPandaFileId(pfName, abcId) && !pfName.empty()) {
        LOG_ECMA(ERROR) << "Get method abc id failed. abcName: " << pfName;
    }
    return abcId;
}
ApEntityId PGOProfiler::GetMethodAbcId(JSFunction *jsFunction)
{
    CString pfName;
    auto jsMethod = jsFunction->GetMethod();
    if (jsMethod.IsMethod()) {
        return GetMethodAbcId(jsMethod);
    }
    LOG_ECMA(ERROR) << "Get method abc id failed. Not a method.";
    UNREACHABLE();
}

ProfileType PGOProfiler::GetRecordProfileType(JSFunction *jsFunction, const CString &recordName)
{
    CString pfName;
    auto jsMethod = jsFunction->GetMethod();
    if (jsMethod.IsMethod()) {
        const auto *pf = Method::Cast(jsMethod)->GetJSPandaFile();
        if (pf != nullptr) {
            pfName = pf->GetJSPandaFileDesc();
        }
    }
    const auto &pf = JSPandaFileManager::GetInstance()->FindJSPandaFile(pfName);
    if (pf == nullptr) {
        LOG_ECMA(ERROR) << "Get record profile type failed. pf is null, pfName: " << pfName
                        << ", recordName: " << recordName;
        return ProfileType::PROFILE_TYPE_NONE;
    }
    return GetRecordProfileType(pf, GetMethodAbcId(jsFunction), recordName);
}

ProfileType PGOProfiler::GetRecordProfileType(ApEntityId abcId, const CString &recordName)
{
    CString pfDesc;
    PGOProfilerManager::GetInstance()->GetPandaFileDesc(abcId, pfDesc);
    const auto &pf = JSPandaFileManager::GetInstance()->FindJSPandaFile(pfDesc);
    if (pf == nullptr) {
        LOG_ECMA(ERROR) << "Get record profile type failed. pf is null, pfDesc: " << pfDesc
                        << ", recordName: " << recordName;
        return ProfileType::PROFILE_TYPE_NONE;
    }
    return GetRecordProfileType(pf, abcId, recordName);
}

ProfileType PGOProfiler::GetRecordProfileType(const std::shared_ptr<JSPandaFile> &pf, ApEntityId abcId,
                                              const CString &recordName)
{
    ASSERT(pf != nullptr);
    JSRecordInfo recordInfo;
    bool hasRecord = pf->CheckAndGetRecordInfo(recordName, recordInfo);
    if (!hasRecord) {
        LOG_ECMA(ERROR) << "Get recordInfo failed. recordName: " << recordName;
        return ProfileType::PROFILE_TYPE_NONE;
    }
    ProfileType recordType {0};
    if (pf->IsBundlePack()) {
        ASSERT(recordName == JSPandaFile::ENTRY_FUNCTION_NAME);
        recordType = CreateRecordProfileType(abcId, ProfileType::RECORD_ID_FOR_BUNDLE);
        recordInfos_->GetRecordPool()->Add(recordType, recordName);
        return recordType;
    }
    if (recordInfo.classId != JSPandaFile::CLASSID_OFFSET_NOT_FOUND) {
        recordType = CreateRecordProfileType(abcId, recordInfo.classId);
        recordInfos_->GetRecordPool()->Add(recordType, recordName);
        return recordType;
    }
    LOG_ECMA(ERROR) << "Invalid classId, skip it. recordName: " << recordName << ", isCjs: " << recordInfo.isCjs
                    << ", isJson: " << recordInfo.isJson;
    return ProfileType::PROFILE_TYPE_NONE;
}

void PGOProfiler::WorkList::PushBack(WorkNode *node)
{
    if (node == nullptr) {
        LOG_ECMA(FATAL) << "PGOProfiler::WorkList::PushBack:node is nullptr";
    }
    if (last_ == nullptr) {
        first_ = node;
        last_ = node;
    } else {
        last_->SetNext(node);
        node->SetPrev(last_);
        last_ = node;
    }
    node->SetWorkList(this);
}

PGOProfiler::WorkNode *PGOProfiler::WorkList::PopFront()
{
    WorkNode *result = nullptr;
    if (first_ != nullptr) {
        result = first_;
        if (first_->GetNext() != nullptr) {
            first_ = first_->GetNext();
            first_->SetPrev(nullptr);
        } else {
            first_ = nullptr;
            last_ = nullptr;
        }
        result->SetNext(nullptr);
        result->SetWorkList(nullptr);
    }
    return result;
}

void PGOProfiler::WorkList::Remove(WorkNode *node)
{
    if (node->GetPrev() != nullptr) {
        node->GetPrev()->SetNext(node->GetNext());
    }
    if (node->GetNext() != nullptr) {
        node->GetNext()->SetPrev(node->GetPrev());
    }
    if (node == first_) {
        first_ = node->GetNext();
    }
    if (node == last_) {
        last_ = node->GetPrev();
    }
    node->SetPrev(nullptr);
    node->SetNext(nullptr);
    node->SetWorkList(nullptr);
}

void PGOProfiler::WorkList::Iterate(Callback callback) const
{
    auto current = first_;
    while (current != nullptr) {
        auto next = current->GetNext();
        callback(current);
        current = next;
    }
}

ProfileType PGOProfiler::CreateRecordProfileType(ApEntityId abcId, ApEntityId classId)
{
    return {abcId, classId, ProfileType::Kind::RecordClassId};
}

JSTaggedValue PGOProfiler::TryFindKeyInPrototypeChain(TaggedObject *currObj, JSHClass *currHC, JSTaggedValue key)
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
void PGOProfiler::InitJITProfiler()
{
    jitProfiler_ = new JITProfiler(vm_);
    jitProfiler_->InitJITProfiler();
}

} // namespace panda::ecmascript::pgo
