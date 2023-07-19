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
void PGOProfiler::ProfileCall(JSTaggedType value, SampleMode mode, int32_t incCount)
{
    if (!isEnable_) {
        return;
    }
    DISALLOW_GARBAGE_COLLECTION;
    JSTaggedValue jsValue(value);
    if (jsValue.IsJSFunction() && JSFunction::Cast(jsValue)->GetMethod().IsMethod()) {
        auto jsMethod = Method::Cast(JSFunction::Cast(jsValue)->GetMethod());
        JSTaggedValue recordNameValue = JSFunction::Cast(jsValue)->GetRecordName();
        if (recordNameValue.IsHole()) {
            return;
        }
        CString recordName = ConvertToString(recordNameValue);
        if (recordInfos_->AddMethod(recordName, jsMethod, mode, incCount)) {
            methodCount_++;
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

void PGOProfiler::ProfileCreateObject(JSTaggedType func, int32_t offset, JSTaggedType originObj, JSTaggedType newObj)
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

        auto originObjValue = JSTaggedValue(originObj);
        auto newObjValue = JSTaggedValue(newObj);
        if (!originObjValue.IsJSObject() || !newObjValue.IsJSObject()) {
            return;
        }
        auto originHclass = JSObject::Cast(originObjValue)->GetJSHClass();
        auto iter = literalIds_.find(JSTaggedType(originHclass));
        if (iter == literalIds_.end()) {
            return;
        }
        auto newHClass = JSObject::Cast(newObjValue) ->GetJSHClass();
        InsertLiteralId(JSTaggedType(newHClass), iter->second);

        auto currentType = PGOSampleType::CreateClassType(iter->second.GetOffset());
        auto superType = PGOSampleType::CreateClassType(0);
        recordInfos_->AddDefine(recordName, funcMethodId, offset, currentType, superType);
        recordInfos_->AddLayout(currentType, JSTaggedType(newHClass), PGOObjKind::LOCAL);
    }
}

void PGOProfiler::ProfileObjLayout(JSThread *thread, JSTaggedType func, int32_t offset, JSTaggedType object, bool store)
{
    if (!isEnable_) {
        return;
    }

    DISALLOW_GARBAGE_COLLECTION;
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
        } else if (hclass->IsLiteral()) {
            auto iter = literalIds_.find(JSTaggedType(hclass));
            if (iter != literalIds_.end()) {
                PGOObjectInfo info(ClassType(iter->second.GetOffset()), kind);
                auto methodId = jsMethod->GetMethodId();
                recordInfos_->AddObjectInfo(recordName, methodId, offset, info);
                if (store) {
                    auto type = PGOSampleType::CreateClassType(iter->second.GetOffset());
                    recordInfos_->AddLayout(type, JSTaggedType(hclass), kind);
                }
            }
            return;
        } else {
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

void PGOProfiler::InsertLiteralId(JSTaggedType hclass, EntityId literalId)
{
    if (!isEnable_) {
        return;
    }
    auto iter = literalIds_.find(hclass);
    if (iter != literalIds_.end()) {
        if (!(iter->second == literalId)) {
            literalIds_.erase(iter);
        }
    }
    literalIds_.emplace(hclass, literalId);
}
} // namespace panda::ecmascript
