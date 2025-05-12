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


#include "ecmascript/enum_conversion.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jit/jit_profiler.h"
#include "ecmascript/pgo_profiler/pgo_trace.h"

namespace panda::ecmascript::pgo {
void PGOProfiler::RecordProfileType(JSHClass *hclass, JSPandaFile *pandaFile, int32_t traceId)
{
    if (!isEnable_) {
        return;
    }
    ProfileType traceType = GetProfileType(hclass);
    if (traceType.IsNone()) {
        pgo::ApEntityId abcId(0);
        pgo::PGOProfilerManager::GetInstance()->GetPandaFileId(pandaFile->GetJSPandaFileDesc(), abcId);
        SetRootProfileType(hclass, abcId, traceId, ProfileType::Kind::ObjectLiteralId);
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

    auto abcId = GetMethodAbcId(ctorFunc);
    auto chc = ctorFunc->GetClass();
    SetRootProfileType(chc, abcId, entityId, ProfileType::Kind::ConstructorId);

    auto protoOrHClass = ctorFunc->GetProtoOrHClass();
    if (protoOrHClass.IsJSHClass()) {
        auto ihc = JSHClass::Cast(protoOrHClass.GetTaggedObject());
        SetRootProfileType(ihc, abcId, entityId, ProfileType::Kind::ClassId);
        protoOrHClass = ihc->GetProto();
    }
    if (protoOrHClass.IsJSObject()) {
        auto phc = protoOrHClass.GetTaggedObject()->GetClass();
        SetRootProfileType(phc, abcId, entityId, ProfileType::Kind::PrototypeId);
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
    auto abcId = GetMethodAbcId(ctorFunc);
    SetRootProfileType(rootHc, abcId, entityId, kind);
}

void PGOProfiler::ProfileNapiRootHClass(JSTaggedType ctor, JSTaggedType rootHcValue, ProfileType::Kind kind)
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
    auto entityId = Method::Cast(ctorMethodValue)->GetMethodId().GetOffset();
    auto rootHc = JSHClass::Cast(JSTaggedValue(rootHcValue).GetTaggedObject());
    auto abcId = GetMethodAbcId(ctorFunc);
    SetRootProfileType(rootHc, abcId, entityId, kind);
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
    auto* transitionTable = vm_->GetFunctionProtoTransitionTable();
    JSTaggedType baseIhc = transitionTable->GetFakeParent(JSTaggedType(phcRoot));
    if (baseIhc == 0) {
        LOG_PGO(DEBUG) << "fake parent not found!";
        ProfileClassRootHClass(func.GetTaggedType(), hclass.GetTaggedType());
        return;
    }

    // In the case of `subclass.prototype = Object.create(superclass)`, the current PGO is
    // unable to collect the pgotype of Object.create(superclass). As a result, when subclass.prototype is modified,
    // ts hclass cannot be reconstructed during the AOT phase, leading to deoptimization.
    JSHClass *baseRoot = JSHClass::FindRootHClass(JSHClass::Cast(JSTaggedValue(baseIhc).GetTaggedObject()));
    if (GetProfileType(baseRoot).IsNone()) {
        return;
    }

    JSHandle<JSTaggedValue> keyHandle(thread, thread->GlobalConstants()->GetGlobalConstantObject(
        static_cast<size_t>(ConstantIndex::PROTO_TRANS_ROOT_HCLASS_SYMBOL_INDEX)));
    PropertyDescriptor desc(thread);
    JSObject::GetOwnProperty(thread, JSHandle<JSObject>::Cast(func), keyHandle, desc);
    JSTaggedType ihc = (desc.GetValue())->GetRawData();
    if (JSTaggedValue(ihc).IsUndefined()) {
        LOG_PGO(DEBUG) << "maybe the prototype of the current function is just the initial prototype!";
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

    // fuzz test modifies prototype explicitly, add check protection
    if (!oldPrototype->IsECMAObject()) {
        return;
    }

    auto method = func->GetMethod();
    if (Method::Cast(method)->IsNativeWithCallField()) {
        return;
    }
    // set prototype once, and just skip this time
    auto thread = vm_->GetJSThread();
    JSHandle<JSTaggedValue> keyHandle(thread, thread->GlobalConstants()->GetGlobalConstantObject(
        static_cast<size_t>(ConstantIndex::PROTO_TRANS_ROOT_HCLASS_SYMBOL_INDEX)));
    PropertyDescriptor desc(thread);
    JSObject::GetOwnProperty(thread, JSHandle<JSObject>::Cast(func), keyHandle, desc);
    if (!(desc.GetValue())->IsUndefined()) {
        return;
    }
    // insert transition item
    JSHandle<JSTaggedValue> transIhc(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> transPhc(thread, prototype->GetTaggedObject()->GetClass());
    if (JSHandle<JSHClass>(baseIhc)->IsDictionaryMode() || JSHandle<JSHClass>(transPhc)->IsDictionaryMode()) {
        return;
    }

    // In the case of `subclass.prototype = Object.create(superclass)`, the current PGO is
    // unable to collect the pgotype of Object.create(superclass). As a result, when subclass.prototype is modified,
    // ts hclass cannot be reconstructed during the AOT phase, leading to deoptimization.
    JSHClass *baseRoot = JSHClass::FindRootHClass(JSHClass::Cast(baseIhc.GetTaggedValue().GetTaggedObject()));
    if (GetProfileType(baseRoot).IsNone()) {
        return;
    }
    auto* transitionTable = vm_->GetFunctionProtoTransitionTable();
    bool success = transitionTable->TryInsertFakeParentItem(transPhc.GetTaggedType(), baseIhc.GetTaggedType());
    if (!success) {
        return;
    }
    // Do not generate ihc lazily, beacause it's used for the key of hash table
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> ihc = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT, oldPrototype);
    PropertyDescriptor definePropertyDesc(thread, JSHandle<JSTaggedValue>(ihc), false, false, false);
    JSObject::DefineOwnProperty(thread, JSHandle<JSObject>::Cast(func), keyHandle, definePropertyDesc);

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

    JSHandle<JSFunction> function(func);

    WorkNode* workNode = reinterpret_cast<WorkNode*>(function->GetWorkNodePointer());
    if (workNode != nullptr) {
        workNode->SetValue(JSTaggedType(JSFunction::Cast(funcValue)));
    }

    JSHandle<JSTaggedValue> key(vm_->GetJSThread(), JSTaggedValue(pcOffset));
    JSHandle<JSTaggedValue> receiverHClassHandle(vm_->GetJSThread(), receiverHClass);
    JSHandle<JSTaggedValue> holderHClassHandle(vm_->GetJSThread(), holderHClass);
    JSHandle<JSTaggedValue> profileTypeInfoValue(vm_->GetJSThread(), function->GetRawProfileTypeInfo());
    JSHandle<ProfileTypeInfoCell> profileTypeInfoCell(profileTypeInfoValue);

    if (function->HasProfileTypeInfo(vm_->GetJSThread())) {
        JSHandle<ProfileTypeInfo> profileTypeInfo(vm_->GetJSThread(), profileTypeInfoCell->GetValue());
        JSHandle<NumberDictionary> dictJShandle = ProfileTypeInfo::CreateOrGetExtraInfoMap(vm_->GetJSThread(),
                                                                                           profileTypeInfo);
        int entry = dictJShandle->FindEntry(key.GetTaggedValue());
        if (entry == -1) {
            ProfileTypeInfo::UpdateExtraInfoMap(vm_->GetJSThread(), dictJShandle, key, receiverHClassHandle,
                                                holderHClassHandle, profileTypeInfo);
            return;
        }
        ExtraProfileTypeInfo *mapInfoObj = ExtraProfileTypeInfo::Cast(dictJShandle->GetValue(entry).GetTaggedObject());
        if (mapInfoObj->GetReceiver() == receiverHClassHandle.GetTaggedValue().CreateAndGetWeakRef() &&
            mapInfoObj->GetHolder() == holderHClassHandle.GetTaggedValue()) {
            return;
        }

        ExtraProfileTypeInfo::Cast(dictJShandle->GetValue(entry).GetTaggedObject())->Clear(vm_->GetJSThread());
    }
}

void PGOProfiler::UpdateRootProfileTypeSafe(JSHClass* oldHClass, JSHClass* newHClass)
{
    if (!isEnable_) {
        return;
    }
    ProfileType oldPt = GetProfileType(oldHClass);
    if (oldPt.IsRootType()) {
        newHClass->SetProfileType(oldPt.GetRaw());
        oldHClass->SetProfileType(0);
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
        UpdateTrackInfo(JSTaggedValue(trackInfo));
    }
}

void PGOProfiler::UpdateTrackInfo(JSTaggedValue trackInfoVal)
{
    if (trackInfoVal.IsHeapObject()) {
        auto trackInfo = TrackInfo::Cast(trackInfoVal.GetTaggedObject());
        auto func = trackInfo->GetCachedFunc();
        auto thread = vm_->GetJSThread();
        if (!func.IsWeak()) {
            return;
        }
        TaggedObject *object = func.GetWeakReferentUnChecked();
        if (!object->GetClass()->IsJSFunction()) {
            return;
        }
        JSFunction* function = JSFunction::Cast(object);
        if (!function->HasProfileTypeInfo(thread)) {
            return;
        }
        auto profileTypeInfoVal = function->GetProfileTypeInfo();
        if (profileTypeInfoVal.IsUndefined() || !profileTypeInfoVal.IsTaggedArray()) {
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
        dumpWorkList_.PushBack(workNode);
    } else {
        workNode->SetValue(JSTaggedType(function));
        auto workList = workNode->GetWorkList();
        if (workList == &preDumpWorkList_) {
            preDumpWorkList_.Remove(workNode);
        }
        if (workList != &dumpWorkList_) {
            dumpWorkList_.PushBack(workNode);
        }
    }
    manager_->TryDispatchDumpTask(this);
}

void PGOProfiler::SuspendByGC()
{
    if (!isEnable_) {
        return;
    }
    state_->SuspendByGC();
}

void PGOProfiler::ResumeByGC()
{
    if (!isEnable_) {
        return;
    }
    state_->ResumeByGC(this);
}

bool PGOProfiler::SetStartIfStop()
{
    if (!isEnable_) {
        return false;
    }
    return state_->SetStartIfStop();
}

void PGOProfiler::SetStopAndNotify()
{
    if (!isEnable_) {
        return;
    }
    state_->SetStopAndNotify();
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
        preDumpWorkList_.PushBack(workNode);
    } else {
        workNode->SetValue(JSTaggedType(function));
        auto workList = workNode->GetWorkList();
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
    JSTaggedValue funcValue = JSTaggedValue(current->GetValue());
    if (!funcValue.IsJSFunction()) {
        return;
    }
    auto func = JSFunction::Cast(funcValue);
    if (!func->HasProfileTypeInfo(vm_->GetJSThread())) {
        return;
    }
    ProfileTypeInfoCell *cell = ProfileTypeInfoCell::Cast(func->GetRawProfileTypeInfo());
    ProfileTypeInfo *info = ProfileTypeInfo::Cast((cell->GetValue()).GetTaggedObject());
    if ((info->GetExtraInfoMap()).IsHole() || (info->GetExtraInfoMap()).IsUndefined()) {
        return;
    }
    NumberDictionary *dict = NumberDictionary::Cast(info->GetExtraInfoMap().GetTaggedObject());
    int size = dict->Size();
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key(dict->GetKey(hashIndex));
        if (!key.IsUndefined() && !key.IsHole()) {
            JSTaggedValue val(dict->GetValue(hashIndex));
            ExtraProfileTypeInfo *extraInfo = ExtraProfileTypeInfo::Cast(val.GetTaggedObject());
            if (!extraInfo->IsValid()) {
                continue;
            }
            AddObjectInfo(abcId,
                          recordName,
                          methodId,
                          key.GetInt(),
                          extraInfo->GetReceiverHClass(),
                          extraInfo->GetReceiverHClass(),
                          extraInfo->GetHolderHClass());
        }
    }
}

bool PGOProfiler::HasValidExtraProfileTypeInfo(JSFunction *func)
{
    if (!func->HasProfileTypeInfo(vm_->GetJSThread())) {
        return false;
    }
    ProfileTypeInfoCell *profileCell = ProfileTypeInfoCell::Cast(func->GetRawProfileTypeInfo());
    ProfileTypeInfo *profileInfo = ProfileTypeInfo::Cast((profileCell->GetValue()).GetTaggedObject());
    JSTaggedValue map = profileInfo->GetExtraInfoMap();
    if (map.IsHole() || map.IsUndefined()) {
        return false;
    }
    NumberDictionary *numberDict = NumberDictionary::Cast(map.GetTaggedObject());
    return numberDict->GetEntrySize() > 0;
}

void PGOProfiler::ProcessExtraProfileTypeInfo(JSFunction *func, ApEntityId abcId, const CString &recordName,
                                              JSTaggedValue methodValue, WorkNode *current)
{
    if (!HasValidExtraProfileTypeInfo(func)) {
        return;
    }
    Method* method = Method::Cast(methodValue.GetTaggedObject());
    EntityId methodId = method->GetMethodId();
    UpdateExtraProfileTypeInfo(abcId, recordName, methodId, current);
}

void PGOProfiler::DumpBeforeDestroy(JSThread *thread)
{
    if (!isEnable_ || !vm_->GetJSOptions().IsEnableProfileDump()) {
        return;
    }
    LOG_PGO(INFO) << "dump profiler before destroy: " << this;
    state_->StartDumpBeforeDestroy(thread);
    HandlePGODump();
    HandlePGOPreDump();
    state_->SetStopAndNotify();
}

void PGOProfiler::HandlePGOPreDump()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "PGOProfiler::HandlePGOPreDump");
    if (!isEnable_ || !vm_->GetJSOptions().IsEnableProfileDump()) {
        return;
    }
    LockHolder lock(PGOProfilerManager::GetPGOInfoMutex());
#ifdef USE_CMC_GC
    ThreadHolder::TryBindMutatorScope scope(holder_);
    holder_->TransferToRunning();
#endif
    DISALLOW_GARBAGE_COLLECTION;
    preDumpWorkList_.Iterate([this](WorkNode* current) {
        JSTaggedValue funcValue = JSTaggedValue(current->GetValue());
        if (!funcValue.IsJSFunction()) {
            return;
        }
        auto func = JSFunction::Cast(funcValue);
        if (func->IsSendableOrConcurrentFunction()) {
            return;
        }
        JSTaggedValue methodValue = func->GetMethod();
        if (!methodValue.IsMethod()) {
            return;
        }
        CString recordName = func->GetRecordName();
        if (recordName.empty()) {
            return;
        }
        auto abcId = GetMethodAbcId(func);

        ProcessExtraProfileTypeInfo(func, abcId, recordName, methodValue, current);
        ProfileType recordType = GetRecordProfileType(abcId, recordName);
        recordInfos_->AddMethod(recordType, Method::Cast(methodValue), SampleMode::HOTNESS_MODE);
        ProfileBytecode(abcId, recordName, funcValue);
        if (PGOTrace::GetInstance()->IsEnable()) {
            PGOTrace::GetInstance()->TryGetMethodData(methodValue, false);
        }
    });
#ifdef USE_CMC_GC
    holder_->TransferToNative();
#endif
}

void PGOProfiler::HandlePGODump()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "PGOProfiler::HandlePGODump");
    if (!isEnable_ || !vm_->GetJSOptions().IsEnableProfileDump()) {
        return;
    }
    LockHolder lock(PGOProfilerManager::GetPGOInfoMutex());
#ifdef USE_CMC_GC
    ThreadHolder::TryBindMutatorScope scope(holder_);
    holder_->TransferToRunning();
#endif
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
        if (func->IsSendableOrConcurrentFunction()) {
            current = PopFromProfileQueue();
            continue;
        }
        JSTaggedValue methodValue = func->GetMethod();
        if (!methodValue.IsMethod()) {
            current = PopFromProfileQueue();
            continue;
        }
        CString recordName = func->GetRecordName();
        if (recordName.empty()) {
            current = PopFromProfileQueue();
            continue;
        }
        auto abcId = GetMethodAbcId(func);

        ProcessExtraProfileTypeInfo(func, abcId, recordName, methodValue, current);

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
#ifdef USE_CMC_GC
    holder_->TransferToNative();
#endif
}

void PGOProfiler::TrySave()
{
    if (manager_->IsForceDump()) {
        return;
    }
    auto interval = std::chrono::system_clock::now() - saveTimestamp_;
    auto minIntervalOption = vm_->GetJSOptions().GetPGOSaveMinInterval();
    auto mergeMinInterval = std::chrono::milliseconds(minIntervalOption * MS_PRE_SECOND);
    // trigger save every 50 methods and duration greater than 30s
    if (methodCount_ >= MERGED_EVERY_COUNT && interval > mergeMinInterval) {
        LOG_PGO(INFO) << "trigger save task, methodCount_ = " << methodCount_;
        state_->SetSaveAndNotify();
        manager_->SavePGOInfo();
        SetSaveTimestamp(std::chrono::system_clock::now());
        methodCount_ = 0;
    }
}

PGOProfiler::WorkNode* PGOProfiler::PopFromProfileQueue()
{
    WorkNode* node = nullptr;
    while (node == nullptr) {
#ifdef USE_CMC_GC
        holder_->CheckSafepointIfSuspended();
#endif
        if (state_->GCIsWaiting()) {
            break;
        }
        if (dumpWorkList_.IsEmpty()) {
            break;
        }
        node = dumpWorkList_.PopFront();
    }
    return node;
}

void PGOProfiler::ProfileBytecode(ApEntityId abcId, const CString& recordName, JSTaggedValue funcValue)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "PGOProfiler::ProfileBytecode");
    ClockScope start;
    JSFunction *function = JSFunction::Cast(funcValue);
    if (function->IsSendableOrConcurrentFunction()) {
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
    bool isForceDump = vm_->GetJSOptions().IsPgoForceDump();

    while (bcIns.GetAddress() != bcInsLast.GetAddress()) {
        if (!isForceDump) {
#ifdef USE_CMC_GC
            holder_->CheckSafepointIfSuspended();
#endif
            if (state_->GCIsWaiting()) {
                break;
            }
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
            case EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8:
            case EcmaOpcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8: {
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
                if (method->GetJSPandaFile() == nullptr) {
                    break;
                }
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
                if (method->GetJSPandaFile() == nullptr) {
                    break;
                }
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
        JSTaggedValue secondValue = profileTypeInfo->Get(slotId + 1);
        DumpICByValueWithPoly(abcId, recordName, methodId, bcOffset, secondValue, type);
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
    // Check whether the cacheValue is TaggedArray
    if (!cacheValue.IsTaggedArray()) {
        return;
    }
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
    } else if (secondValue.IsStoreAOTHandler()) {
        StoreAOTHandler *storeAOTHandler = StoreAOTHandler::Cast(secondValue.GetTaggedObject());
        auto cellValue = storeAOTHandler->GetProtoCell();
        if (CheckProtoChangeMarker(cellValue)) {
            return false;
        }
        auto holder = storeAOTHandler->GetHolder();
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
        JSTaggedValue handlerInfoVal = prototypeHandler->GetHandlerInfo();
        if (!handlerInfoVal.IsInt()) {
            return ret;
        }
        auto handlerInfo = static_cast<uint32_t>(handlerInfoVal.GetInt());
        if (HandlerBase::IsNonExist(handlerInfo)) {
            return ret;
        }
        auto holder = prototypeHandler->GetHolder();
        auto holderHClass = holder.GetTaggedObject()->GetClass();
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
        if (!transitionHClassVal.IsJSHClass()) {
            return ;
        }
        auto handlerInfoValue = transitionHandler->GetHandlerInfo();
        auto transitionHClass = JSHClass::Cast(transitionHClassVal.GetTaggedObject());
        if (handlerInfoValue.IsInt()) {
            auto handlerInfo = static_cast<uint32_t>(handlerInfoValue.GetInt());
            if (HandlerBase::IsElement(handlerInfo)) {
                AddBuiltinsInfo(abcId, recordName, methodId, bcOffset, hclass, transitionHClass,
                                OnHeapMode::NONE, HandlerBase::IsStoreOutOfBounds(handlerInfo));
                return;
            }
        }
        AddObjectInfo(abcId, recordName, methodId, bcOffset, hclass, hclass, transitionHClass);
    } else if (secondValue.IsTransWithProtoHandler()) {
        auto transWithProtoHandler = TransWithProtoHandler::Cast(secondValue.GetTaggedObject());
        auto transitionHClassVal = transWithProtoHandler->GetTransitionHClass();
        if (!transitionHClassVal.IsJSHClass()) {
            return ;
        }
        auto handlerInfoValue = transWithProtoHandler->GetHandlerInfo();
        auto transitionHClass = JSHClass::Cast(transitionHClassVal.GetTaggedObject());
        if (handlerInfoValue.IsInt()) {
            auto handlerInfo = static_cast<uint32_t>(handlerInfoValue.GetInt());
            if (HandlerBase::IsElement(handlerInfo)) {
                AddBuiltinsInfo(abcId, recordName, methodId, bcOffset, hclass, transitionHClass,
                                OnHeapMode::NONE, HandlerBase::IsStoreOutOfBounds(handlerInfo));
                return;
            }
        }
        AddObjectInfo(abcId, recordName, methodId, bcOffset, hclass, hclass, transitionHClass);
    } else if (secondValue.IsPrototypeHandler()) {
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
        if (handlerInfoValue.IsInt()) {
            auto handlerInfo = static_cast<uint32_t>(handlerInfoValue.GetInt());
            if (HandlerBase::IsElement(handlerInfo)) {
                AddBuiltinsInfo(abcId, recordName, methodId, bcOffset, hclass, hclass,
                                OnHeapMode::NONE, HandlerBase::IsStoreOutOfBounds(handlerInfo));
                return;
            }
        }
        auto holder = prototypeHandler->GetHolder();
        auto holderHClass = holder.GetTaggedObject()->GetClass();
        AddObjectInfo(abcId, recordName, methodId, bcOffset, hclass, holderHClass, holderHClass);
    }
}

void PGOProfiler::TryDumpProtoTransitionType(JSHClass *hclass)
{
    JSHClass *ihc1 = JSHClass::FindRootHClass(hclass);
    auto transitionType = GetProfileType(ihc1, true);
    if (!transitionType.IsRootType() || !transitionType.IsTransitionClassType()) {
        return;
    }
    JSTaggedValue phc1Root = JSHClass::FindProtoRootHClass(ihc1);
    if (!phc1Root.IsJSHClass()) {
        LOG_PGO(DEBUG) << "Phc1Root is not a JSHclass!";
        return;
    }

    auto transitionPrototype = GetProfileType(JSHClass::Cast(phc1Root.GetTaggedObject()), true);
    if (!transitionPrototype.IsRootType()) {
        LOG_PGO(DEBUG) << "Set as the prototype of a function again after transition happened for this prototype!";
        return;
    }

    auto* transitionTable = vm_->GetFunctionProtoTransitionTable();
    JSTaggedType ihc0 = transitionTable->GetFakeParent(JSTaggedType(ihc1));
    JSTaggedType baseIhc = transitionTable->GetFakeParent(phc1Root.GetRawData());
    if ((ihc0 == 0) || (baseIhc == 0)) {
        return;
    }

    auto ihc0Obj = JSHClass::Cast(JSTaggedValue(ihc0).GetTaggedObject());
    auto baseIhcObj = JSHClass::Cast(JSTaggedValue(baseIhc).GetTaggedObject());
    UpdateLayout(ihc0Obj);
    UpdateLayout(ihc1);
    UpdateLayout(baseIhcObj);

    auto ihc0RootType = GetProfileType(ihc0Obj);
    ASSERT(ihc0RootType.IsRootType());
    auto baseRootHClass = JSHClass::FindRootHClass(baseIhcObj);
    auto baseRootType = GetProfileType(baseRootHClass, true);
    if (!baseRootType.IsRootType()) {
        LOG_PGO(DEBUG) << "Unsupported prototypes which cannot be recorded!";
        return;
    }
    auto baseType = GetProfileType(baseIhcObj);
    ASSERT(!baseType.IsNone());
    PGOProtoTransitionType protoTransitionType(ihc0RootType);
    protoTransitionType.SetBaseType(baseRootType, baseType);
    protoTransitionType.SetTransitionType(transitionType);
    protoTransitionType.SetTransitionProtoPt(transitionPrototype);

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
        if (!ctorMethod.IsMethod() || !FunctionKindVerify(ctorFunction)) {
            return;
        }
        ApEntityId ctorAbcId = GetMethodAbcId(ctorFunction);
        auto ctorJSMethod = Method::Cast(ctorMethod);
        auto ctorMethodId = ctorJSMethod->GetMethodId().GetOffset();

        auto localType = ProfileType(ctorAbcId, ctorMethodId, ProfileType::Kind::ClassId, true);
        if (IsSkippableObjectTypeSafe(localType)) {
            return;
        }
        PGODefineOpType objDefType(localType);
        auto protoOrHClass = ctorFunction->GetProtoOrHClass();
        if (protoOrHClass.IsJSHClass()) {
            auto ihc = JSHClass::Cast(protoOrHClass.GetTaggedObject());
            SetRootProfileType(ihc, ctorAbcId, ctorMethodId, ProfileType::Kind::ClassId);
            recordInfos_->AddRootLayout(JSTaggedType(ihc), localType);
            protoOrHClass = ihc->GetProto();
        }

        auto ctorRootHClass = JSHClass::FindRootHClass(ctorFunction->GetJSHClass());
        auto ctorType = GetProfileType(ctorRootHClass);
        if (!ctorType.IsRootType()) {
            LOG_PGO(DEBUG) << "The profileType of constructor root hclass was not found.";
        } else {
            objDefType.SetCtorPt(ctorType);
            recordInfos_->AddRootLayout(JSTaggedType(ctorRootHClass), ctorType);
        }

        if (protoOrHClass.IsJSObject()) {
            auto prototypeHClass = JSObject::Cast(protoOrHClass)->GetClass();
            auto prototypeRootHClass = JSHClass::FindRootHClass(prototypeHClass);
            ProfileType prototypeType = GetProfileType(prototypeRootHClass);
            if (!prototypeType.IsRootType()) {
                LOG_PGO(DEBUG) << "The profileType of prototype root hclass was not found.";
            } else {
                objDefType.SetPrototypePt(prototypeType);
                recordInfos_->AddRootLayout(JSTaggedType(prototypeRootHClass), prototypeType);
            }
        }

        ProfileType recordType = GetRecordProfileType(abcId, recordName);
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
            ProfileType profileType = GetProfileType(rootHClass);
            if (!profileType.IsRootType()) {
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
            kind = ProfileType::Kind::MethodId;
        } else {
            kind = ProfileType::Kind::BuiltinFunctionId;
        }
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
    if (!GetProfileType(hclass).IsRootType() && parentHClass.IsJSHClass()) {
        UpdateTransitionLayout(JSHClass::Cast(parentHClass.GetTaggedObject()), hclass);
    } else {
        auto rootHClass = JSHClass::FindRootHClass(hclass);
        ProfileType rootType = GetProfileType(rootHClass, true);
        if (!rootType.IsRootType()) {
            return;
        }

        auto prototypeHClass = JSHClass::FindProtoRootHClass(rootHClass);
        if (prototypeHClass.IsJSHClass()) {
            auto prototypeObject = JSHClass::Cast(prototypeHClass.GetTaggedObject());
            ProfileType prototypeType = GetProfileType(prototypeObject, true);
            if (prototypeType.IsRootType()) {
                recordInfos_->AddRootPtType(rootType, prototypeType);
                UpdateLayout(JSHClass::Cast(prototypeHClass.GetTaggedObject()));
            }
        }

        auto curType = GetOrInsertProfileType(hclass, rootType);
        recordInfos_->UpdateLayout(rootType, JSTaggedType(hclass), curType);
    }
}

void PGOProfiler::UpdateTransitionLayout(JSHClass* parent, JSHClass* child)
{
    auto rootHClass = JSHClass::FindRootHClass(parent);
    auto rootType = GetProfileType(rootHClass, true);
    if (!rootType.IsRootType()) {
        return;
    }
    // If the child hclass is set as a prototype, it will become the root hclass. Need to give up.
    if (GetProfileType(child).IsRootType()) {
        return;
    }
    CStack<JSHClass *> hclassVec;
    hclassVec.emplace(child);
    hclassVec.emplace(parent);

    while (!GetProfileType(parent).IsRootType()) {
        auto parentHCValue = parent->GetParent();
        if (!parentHCValue.IsJSHClass()) {
            break;
        }
        parent = JSHClass::Cast(parentHCValue.GetTaggedObject());
        hclassVec.emplace(parent);
    }

    auto prototypeRootHClassVal = JSHClass::FindProtoRootHClass(rootHClass);
    if (prototypeRootHClassVal.IsJSHClass()) {
        auto prototypeRootHClass = JSHClass::Cast(prototypeRootHClassVal.GetTaggedObject());
        auto prototypeType = GetProfileType(prototypeRootHClass);
        if (prototypeType.IsRootType()) {
            recordInfos_->AddRootPtType(rootType, prototypeType);
            UpdateLayout(prototypeRootHClass);
        }
    }

    parent = hclassVec.top();
    hclassVec.pop();
    auto parentType = GetProfileType(parent);
    while (!hclassVec.empty()) {
        child = hclassVec.top();
        hclassVec.pop();
        auto childType = GetOrInsertProfileType(child, rootType);
        recordInfos_->UpdateTransitionLayout(
            rootType, JSTaggedType(parent), parentType, JSTaggedType(child), childType);
        parentType = childType;
        parent = child;
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
    auto receiverRootType = FindRootProfileType(receiver);
    if (!receiverRootType.IsRootType()) {
        return false;
    }

    auto holdRootType = FindRootProfileType(hold);
    if (!holdRootType.IsRootType()) {
        return true;
    }

    auto receiverType = GetOrInsertProfileType(receiver, receiverRootType);
    auto holdType = GetOrInsertProfileType(hold, holdRootType);
    auto holdTraType = GetOrInsertProfileType(holdTra, holdRootType);

    if (receiver != hold) {
        UpdateLayout(receiver);
    }

    if (holdType == holdTraType) {
        UpdateLayout(hold);
    } else {
        UpdateTransitionLayout(hold, holdTra);
    }

    PGOObjectInfo info(receiverRootType, receiverType, holdRootType, holdType, holdRootType, holdTraType,
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
        auto protoRootType = FindRootProfileType(protoHClass);
        if (!protoRootType.IsRootType()) {
            break;
        }
        auto protoType = GetOrInsertProfileType(protoHClass, protoRootType);
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
        bool receiverIsPrototype = receiver->IsPrototype();
        exceptRecvHClass = thread->GetArrayInstanceHClass(receiver->GetElementsKind(), receiverIsPrototype);
    } else if (builtinsId == BuiltinTypeId::STRING) {
        exceptRecvHClass = receiver;
    } else {
        exceptRecvHClass = thread->GetBuiltinInstanceHClass(builtinsId.value());
    }

    if (exceptRecvHClass != receiver) {
        // When JSType cannot uniquely identify builtins object, it is necessary to
        // query the receiver on the global constants.
        if (builtinsId == BuiltinTypeId::OBJECT) {
            JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
            exceptRecvHClass = JSHClass::Cast(env->GetIteratorResultClass().GetTaggedValue().GetTaggedObject());
            if (exceptRecvHClass == receiver) {
                GlobalIndex globalsId;
                globalsId.UpdateGlobalEnvId(static_cast<size_t>(GlobalEnvField::ITERATOR_RESULT_CLASS_INDEX));
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
        bool receiverIsPrototype = receiver->IsPrototype();
        exceptRecvHClass = thread->GetArrayInstanceHClass(receiver->GetElementsKind(), receiverIsPrototype);
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
        LOG_PGO(DEBUG) << "forbide to add more than 1 hclass for a root type!";
        return true;
    }
    recordedTransRootType_.emplace_back(type);
    return false;
}

void PGOProfiler::SetRootProfileType(JSHClass *root, ApEntityId abcId, uint32_t type, ProfileType::Kind kind)
{
    ProfileType traceType(root->GetProfileType());
    if (traceType.IsNone()) {
        traceType = ProfileType(abcId, type, kind, true);
        if (IsRecoredTransRootType(traceType)) {
            return;
        }
        root->SetProfileType(traceType.GetRaw());
    }
}

ProfileType PGOProfiler::FindRootProfileType(JSHClass *hclass)
{
    auto rootHClass = JSHClass::FindRootHClass(hclass);
    return GetProfileType(rootHClass, true);
}

ProfileType PGOProfiler::GetOrInsertProfileType(JSHClass *child, ProfileType rootType)
{
    ProfileType childType = GetProfileType(child);
    if (childType.IsNone()) {
        ASSERT(rootType.IsRootType());
        childType = PGOTypeGenerator::GenerateProfileType(JSTaggedType(child), rootType);
        child->SetProfileType(childType.GetRaw());
    }
    return childType;
}

ProfileType PGOProfiler::GetProfileType(JSHClass *hclass, bool check)
{
    auto result = ProfileType(hclass->GetProfileType());
    if (check) {
        if (IsSkippableObjectTypeSafe(result)) {
            result = ProfileType::PROFILE_TYPE_NONE;
        }
    }
    return result;
}

void PGOProfiler::IteratePGOPreFuncList(RootVisitor& visitor) const
{
    if (!isEnable_) {
        return;
    }
    preDumpWorkList_.Iterate([this, &visitor](WorkNode *node) {
        visitor.VisitRoot(Root::ROOT_VM, ObjectSlot(node->GetValueAddr()));
    });
}

void PGOProfiler::ProcessReferences(const WeakRootVisitor &visitor)
{
    if (!isEnable_) {
        return;
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
    });
}

void PGOProfiler::Iterate(RootVisitor &visitor)
{
    if (!isEnable_) {
        return;
    }
    // If the IC of the method is stable, the current design forces the dump data.
    // Must pause dump during GC.
    dumpWorkList_.Iterate([&visitor](WorkNode* node) {
        visitor.VisitRoot(Root::ROOT_VM, ObjectSlot(node->GetValueAddr()));
    });
}

PGOProfiler::PGOProfiler(EcmaVM* vm, bool isEnable)
    : nativeAreaAllocator_(std::make_unique<NativeAreaAllocator>()), vm_(vm), isEnable_(isEnable)
{
    if (isEnable_) {
        manager_ = PGOProfilerManager::GetInstance();
        state_ = std::make_unique<PGOState>();
        recordInfos_ = manager_->GetPGOInfo()->GetRecordDetailInfosPtr();
        SetSaveTimestamp(std::chrono::system_clock::now());
#ifdef USE_CMC_GC
        holder_ = ThreadHolder::CreateAndRegisterNewThreadHolder(nullptr);
#endif
        LOG_PGO(INFO) << "constructing pgo profiler, pgo is enabled";
    } else {
        LOG_PGO(INFO) << "skipping pgo profiler construction, pgo is disabled";
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
    SetSaveTimestamp(std::chrono::system_clock::now());
    LockHolder lock(PGOProfilerManager::GetPGOInfoMutex());
    if (isEnable_) {
        manager_ = PGOProfilerManager::GetInstance();
        recordInfos_ = manager_->GetPGOInfo()->GetRecordDetailInfosPtr();
        state_ = std::make_unique<PGOState>();
#ifdef USE_CMC_GC
        if (holder_ == nullptr) {
            holder_ = ThreadHolder::CreateAndRegisterNewThreadHolder(nullptr);
        }
#endif
    } else {
        state_.reset();
        recordInfos_.reset();
        manager_ = nullptr;
#ifdef USE_CMC_GC
        if (holder_ != nullptr) {
            ThreadHolder::DestroyThreadHolder(holder_);
            holder_ = nullptr;
        }
#endif
    }
    LOG_PGO(INFO) << "reset pgo profiler, pgo profiler is " << (isEnable_ ? "enabled" : "disabled");
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
        LOG_PGO(ERROR) << "Get method abc id failed. abcName: " << pfName;
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
    LOG_PGO(ERROR) << "Get method abc id failed. Not a method.";
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
        LOG_PGO(ERROR) << "Get record profile type failed. pf is null, pfName: " << pfName
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
        LOG_PGO(ERROR) << "Get record profile type failed. pf is null, pfDesc: " << pfDesc
                       << ", recordName: " << recordName;
        return ProfileType::PROFILE_TYPE_NONE;
    }
    return GetRecordProfileType(pf, abcId, recordName);
}

ProfileType PGOProfiler::GetRecordProfileType(const std::shared_ptr<JSPandaFile> &pf, ApEntityId abcId,
                                              const CString &recordName)
{
    ASSERT(pf != nullptr);
    JSRecordInfo *recordInfo = pf->CheckAndGetRecordInfo(recordName);
    if (recordInfo == nullptr) {
        LOG_PGO(ERROR) << "Get recordInfo failed. recordName: " << recordName;
        return ProfileType::PROFILE_TYPE_NONE;
    }
    ProfileType recordType {0};
    if (pf->IsBundlePack()) {
        recordType = CreateRecordProfileType(abcId, ProfileType::RECORD_ID_FOR_BUNDLE);
        recordInfos_->GetRecordPool()->Add(recordType, recordName);
        return recordType;
    }
    if (recordInfo->classId != JSPandaFile::CLASSID_OFFSET_NOT_FOUND) {
        recordType = CreateRecordProfileType(abcId, recordInfo->classId);
        recordInfos_->GetRecordPool()->Add(recordType, recordName);
        return recordType;
    }
    LOG_PGO(ERROR) << "Invalid classId, skip it. recordName: " << recordName << ", isCjs: " << recordInfo->isCjs
                   << ", isJson: " << recordInfo->isJson;
    return ProfileType::PROFILE_TYPE_NONE;
}

bool PGOProfiler::WorkList::IsEmpty()
{
    LockHolder lock(workListMutex_);
    return first_ == nullptr;
}

void PGOProfiler::WorkList::PushBack(WorkNode *node)
{
    LockHolder lock(workListMutex_);
    if (node == nullptr) {
        LOG_PGO(FATAL) << "PGOProfiler::WorkList::PushBack:node is nullptr";
        UNREACHABLE();
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
    LockHolder lock(workListMutex_);
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
    LockHolder lock(workListMutex_);
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

void PGOProfiler::WorkList::Reset()
{
    LockHolder lock(workListMutex_);
    first_ = nullptr;
    last_ = nullptr;
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
        auto proto = currHC->GetProto();
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
void PGOProfiler::InitJITProfiler()
{
    jitProfiler_ = new JITProfiler(vm_);
    jitProfiler_->InitJITProfiler();
}

void PGOProfiler::InsertSkipCtorMethodIdSafe(EntityId ctorMethodId)
{
    LockHolder lock(skipCtorMethodIdMutex_);
    skipCtorMethodId_.insert(ctorMethodId.GetOffset());
}

void PGOProfiler::SetSaveTimestamp(std::chrono::system_clock::time_point timestamp)
{
    saveTimestamp_ = timestamp;
}

JITProfiler* PGOProfiler::GetJITProfile()
{
    return jitProfiler_;
}

bool PGOProfiler::IsSkippableObjectTypeSafe(ProfileType type)
{
    if (type.IsGeneralizedClassType() || type.IsConstructor() || type.IsGeneralizedPrototype()) {
        uint32_t ctorId = type.GetId();
        LockHolder lock(skipCtorMethodIdMutex_);
        return skipCtorMethodId_.find(ctorId) != skipCtorMethodId_.end();
    }
    return false;
}

bool PGOProfiler::IsSkippableCtor(uint32_t entityId)
{
    return entityId == 0 || skipCtorMethodId_.find(entityId) != skipCtorMethodId_.end();
}

bool PGOProfiler::InsertDefinedCtor(uint32_t entityId)
{
    if (definedCtorMethodId_.find(entityId) == definedCtorMethodId_.end()) {
        definedCtorMethodId_.insert(entityId);
        return true;
    }
    return false;
}
} // namespace panda::ecmascript::pgo
