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

#include "ecmascript/js_function.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"

namespace panda::ecmascript {
void PGOProfiler::ProfileCall(JSTaggedType func, JSTaggedType callTarget, int32_t pcOffset, SampleMode mode,
                              int32_t incCount)
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
    JSTaggedValue calleeRecordNameValue = JSFunction::Cast(calleeFunc)->GetRecordName();
    if (calleeRecordNameValue.IsHole()) {
        return;
    }
    CString calleeRecordName = ConvertToString(calleeRecordNameValue);
    if (recordInfos_->AddMethod(calleeRecordName, calleeMethod, mode, incCount)) {
        methodCount_++;
    }
    JSTaggedValue currentFunc(func);
    if (pcOffset > 0 && currentFunc.IsJSFunction() && JSFunction::Cast(currentFunc)->GetMethod().IsMethod()) {
        auto currentMethod = Method::Cast(JSFunction::Cast(currentFunc)->GetMethod());
        JSTaggedValue currentRecordNameValue = JSFunction::Cast(currentFunc)->GetRecordName();
        if (currentRecordNameValue.IsHole()) {
            return;
        }
        if (calleeMethod->IsNativeWithCallField()) {
            return;
        }
        CString currentRecordName = ConvertToString(currentRecordNameValue);
        // Only mark the call in the same record now
        if (currentRecordName != calleeRecordName) {
            return;
        }
        auto currentMethodId = currentMethod->GetMethodId();
        PGOSampleType calleeMethodOffset = PGOSampleType::CreateClassType(calleeMethod->GetMethodId().GetOffset());
        recordInfos_->AddCallTargetType(currentRecordName, currentMethodId, pcOffset, calleeMethodOffset);
    }
    auto interval = std::chrono::system_clock::now() - saveTimestamp_;
    // Merged every 10 methods and merge interval greater than minimal interval
    if (methodCount_ >= MERGED_EVERY_COUNT && interval > MERGED_MIN_INTERVAL) {
        LOG_ECMA(DEBUG) << "Sample: post task to save profiler";
        PGOProfilerManager::GetInstance()->Merge(this);
        PGOProfilerManager::GetInstance()->AsynSave();
        SetSaveTimestamp(std::chrono::system_clock::now());
        methodCount_ = 0;
    }
}

void PGOProfiler::ProfileOpType(JSTaggedType func, int32_t offset, uint32_t type)
{
    if (!isEnable_) {
        return;
    }

    DISALLOW_GARBAGE_COLLECTION;
    JSTaggedValue funcValue(func);
    if (funcValue.IsJSFunction() && JSFunction::Cast(funcValue)->GetMethod().IsMethod()) {
        auto jsMethod = Method::Cast(JSFunction::Cast(funcValue)->GetMethod());
        JSTaggedValue recordNameValue = JSFunction::Cast(funcValue)->GetRecordName();
        if (recordNameValue.IsHole()) {
            return;
        }
        CString recordName = ConvertToString(recordNameValue);
        recordInfos_->AddType(recordName, jsMethod->GetMethodId(), offset, PGOSampleType(type));
    }
}

void PGOProfiler::ProfileDefineClass(JSThread *thread, JSTaggedType func, int32_t offset, JSTaggedType ctor)
{
    if (!isEnable_) {
        return;
    }

    DISALLOW_GARBAGE_COLLECTION;
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSTaggedValue funcValue(func);
    if (funcValue.IsJSFunction()) {
        JSFunction *funcFunction = JSFunction::Cast(funcValue);
        JSTaggedValue recordNameValue = funcFunction->GetRecordName();
        if (recordNameValue.IsHole()) {
            return;
        }
        CString recordName = ConvertToString(recordNameValue);

        auto method = funcFunction->GetMethod();
        if (!method.IsMethod()) {
            return;
        }
        auto jsMethod = Method::Cast(method);
        auto funcMethodId = jsMethod->GetMethodId();

        JSHandle<JSTaggedValue> ctorValue(thread, JSTaggedValue(ctor));
        if (!ctorValue->IsJSFunction()) {
            return;
        }
        JSFunction *ctorFunction = JSFunction::Cast(ctorValue->GetTaggedObject());
        auto ctorMethod = ctorFunction->GetMethod();
        if (!ctorMethod.IsMethod()) {
            return;
        }
        auto ctorJSMethod = Method::Cast(ctorMethod);
        int32_t ctorMethodId = static_cast<int32_t>(ctorJSMethod->GetMethodId().GetOffset());
        auto currentType = PGOSampleType::CreateClassType(ctorMethodId);

        auto superFuncValue = JSTaggedValue::GetPrototype(thread, ctorValue);
        RETURN_IF_ABRUPT_COMPLETION(thread);
        PGOSampleType superType = PGOSampleType::CreateClassType(0);
        if (superFuncValue.IsJSFunction()) {
            auto superFuncFunction = JSFunction::Cast(superFuncValue);
            if (superFuncFunction->GetMethod().IsMethod()) {
                auto superMethod = Method::Cast(superFuncFunction->GetMethod());
                auto superMethodId = superMethod->GetMethodId().GetOffset();
                superType = PGOSampleType::CreateClassType(superMethodId);
            }
        }
        recordInfos_->AddDefine(recordName, funcMethodId, offset, currentType, superType);

        auto prototype = ctorFunction->GetProtoOrHClass();
        if (!prototype.IsJSObject()) {
            return;
        }
        auto prototypeObj = JSObject::Cast(prototype);
        auto prototypeHClass = JSTaggedType(prototypeObj->GetClass());
        recordInfos_->AddLayout(currentType, prototypeHClass, PGOObjKind::PROTOTYPE);

        auto ctorHClass = JSTaggedType(ctorFunction->GetJSHClass());
        recordInfos_->AddLayout(currentType, ctorHClass, PGOObjKind::CONSTRUCTOR);
    }
}

void PGOProfiler::ProfileCreateObject(JSTaggedType func, int32_t offset, JSTaggedType newObj, int32_t traceId)
{
    if (!isEnable_) {
        return;
    }

    DISALLOW_GARBAGE_COLLECTION;
    JSTaggedValue funcValue(func);
    if (funcValue.IsJSFunction()) {
        JSFunction *funcFunction = JSFunction::Cast(funcValue);
        JSTaggedValue recordNameValue = funcFunction->GetRecordName();
        if (recordNameValue.IsHole()) {
            return;
        }
        CString recordName = ConvertToString(recordNameValue);

        auto method = funcFunction->GetMethod();
        if (!method.IsMethod()) {
            return;
        }
        auto jsMethod = Method::Cast(method);
        auto funcMethodId = jsMethod->GetMethodId();

        auto newObjValue = JSTaggedValue(newObj);
        if (!newObjValue.IsJSObject()) {
            return;
        }
        auto newHClass = JSObject::Cast(newObjValue) ->GetJSHClass();
        if (newHClass->IsJSArray()) {
            auto array = JSArray::Cast(newObjValue);
            auto currentType = PGOSampleType::CreateClassType(array->GetTraceIndex(), ClassType::Kind::ElementId);
            auto superType = PGOSampleType::CreateClassType(0);
            recordInfos_->AddDefine(recordName, funcMethodId, offset, currentType, superType);
            PGOObjKind kind = PGOObjKind::ELEMENT;
            recordInfos_->AddLayout(currentType, JSTaggedType(newHClass), kind);
        } else {
            traceId = InsertLiteralId(JSTaggedType(newHClass), traceId);
            auto currentType = PGOSampleType::CreateClassType(traceId, ClassType::Kind::LiteralId);
            auto superType = PGOSampleType::CreateClassType(0);
            recordInfos_->AddDefine(recordName, funcMethodId, offset, currentType, superType);
            PGOObjKind kind = PGOObjKind::LOCAL;
            recordInfos_->AddLayout(currentType, JSTaggedType(newHClass), kind);
        }
    }
}

void PGOProfiler::ProfileObjLayout(JSThread *thread, JSTaggedType func, int32_t offset, JSTaggedType object, bool store)
{
    if (!isEnable_) {
        return;
    }

    DISALLOW_GARBAGE_COLLECTION;
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSTaggedValue funcValue(func);
    if (funcValue.IsJSFunction()) {
        JSFunction *funcFunction = JSFunction::Cast(funcValue);
        auto method = funcFunction->GetMethod();
        if (!method.IsMethod()) {
            return;
        }
        auto jsMethod = Method::Cast(method);
        JSTaggedValue recordNameValue = funcFunction->GetRecordName();
        if (recordNameValue.IsHole()) {
            return;
        }
        CString recordName = ConvertToString(recordNameValue);

        auto holder = JSTaggedValue(object);
        auto hclass = holder.GetTaggedObject()->GetClass();
        auto ctor = JSTaggedValue::Undefined();
        PGOObjKind kind = PGOObjKind::LOCAL;
        if (hclass->IsClassPrototype()) {
            ctor = JSObject::GetCtorFromPrototype(thread, holder);
            kind = PGOObjKind::PROTOTYPE;
        } else if (hclass->IsClassConstructor()) {
            ctor = holder;
            kind = PGOObjKind::CONSTRUCTOR;
        } else if (hclass->IsJSArray()) {
            kind = PGOObjKind::ELEMENT;
            auto array = JSArray::Cast(holder);
            if (array->GetTraceIndex() != 0) {
                auto elementsKind = hclass->GetElementsKind();
                PGOObjectInfo info(ClassType(static_cast<uint32_t>(elementsKind), ClassType::Kind::ElementId), kind);
                auto methodId = jsMethod->GetMethodId();
                recordInfos_->AddObjectInfo(recordName, methodId, offset, info);
                if (store) {
                    auto type = PGOSampleType::CreateClassType(array->GetTraceIndex(), ClassType::Kind::ElementId);
                    recordInfos_->AddLayout(type, JSTaggedType(hclass), kind);
                }
            }
            return;
        } else if (hclass->IsTypedArray()) {
            auto id = static_cast<uint32_t>(hclass->GetObjectType());
            PGOObjectInfo info(ClassType(id, ClassType::Kind::BuiltinsId), kind);
            recordInfos_->AddObjectInfo(recordName, jsMethod->GetMethodId(), offset, info);
            return;
        } else {
            if (hclass->IsJSObject()) {
                auto iter = traceIds_.find(JSTaggedType(hclass));
                if (iter != traceIds_.end()) {
                    PGOObjectInfo info(ClassType(iter->second, ClassType::Kind::LiteralId), kind);
                    auto methodId = jsMethod->GetMethodId();
                    recordInfos_->AddObjectInfo(recordName, methodId, offset, info);
                    if (store) {
                        auto type = PGOSampleType::CreateClassType(iter->second, ClassType::Kind::LiteralId);
                        recordInfos_->AddLayout(type, JSTaggedType(hclass), kind);
                    }
                    return;
                }
            }
            auto prototype = hclass->GetProto();
            ctor = JSObject::GetCtorFromPrototype(thread, prototype);
        }

        if (ctor.IsJSFunction()) {
            auto ctorFunc = JSFunction::Cast(ctor);
            auto ctorMethod = ctorFunc->GetMethod();
            if (!ctorMethod.IsMethod()) {
                return;
            }
            auto ctorJSMethod = Method::Cast(ctorMethod);
            auto methodId = ctorJSMethod->GetMethodId();
            PGOObjectInfo info(ClassType(methodId.GetOffset()), kind);
            recordInfos_->AddObjectInfo(recordName, jsMethod->GetMethodId(), offset, info);
            if (store) {
                PGOSampleType type = PGOSampleType::CreateClassType(methodId.GetOffset());
                recordInfos_->AddLayout(type, JSTaggedType(hclass), kind);
            }
        }
    }
}

void PGOProfiler::ProfileObjIndex(JSThread *thread, JSTaggedType func, int32_t offset, JSTaggedType object)
{
    if (!isEnable_) {
        return;
    }

    DISALLOW_GARBAGE_COLLECTION;
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSTaggedValue funcValue(func);
    if (funcValue.IsJSFunction()) {
        JSFunction *funcFunction = JSFunction::Cast(funcValue);
        auto method = funcFunction->GetMethod();
        if (!method.IsMethod()) {
            return;
        }
        auto jsMethod = Method::Cast(method);
        JSTaggedValue recordNameValue = funcFunction->GetRecordName();
        if (recordNameValue.IsHole()) {
            return;
        }
        CString recordName = ConvertToString(recordNameValue);

        auto holder = JSTaggedValue(object);
        auto hclass = holder.GetTaggedObject()->GetClass();
        if (hclass->IsTypedArray()) {
            auto id = static_cast<uint32_t>(hclass->GetObjectType());
            PGOObjectInfo info(ClassType(id, ClassType::Kind::BuiltinsId), PGOObjKind::ELEMENT);
            recordInfos_->AddObjectInfo(recordName, jsMethod->GetMethodId(), offset, info);
        }
    }
}

int32_t PGOProfiler::InsertLiteralId(JSTaggedType hclass, int32_t traceId)
{
    if (!isEnable_) {
        return traceId;
    }
    auto iter = traceIds_.find(hclass);
    if (iter != traceIds_.end()) {
        return iter->second;
    }
    traceIds_.emplace(hclass, traceId);
    return traceId;
}

void PGOProfiler::ProcessReferences(const WeakRootVisitor &visitor)
{
    if (!isEnable_) {
        return;
    }
    for (auto iter = traceIds_.begin(); iter != traceIds_.end();) {
        JSTaggedType object = iter->first;
        auto fwd = visitor(reinterpret_cast<TaggedObject *>(object));
        if (fwd == nullptr) {
            iter = traceIds_.erase(iter);
            continue;
        }
        if (fwd != reinterpret_cast<TaggedObject *>(object)) {
            UNREACHABLE();
        }
        ++iter;
    }
}
} // namespace panda::ecmascript
