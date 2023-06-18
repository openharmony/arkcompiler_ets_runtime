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

#include "ecmascript/js_function.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"

namespace panda::ecmascript {
void PGOProfiler::ProfileCall(JSTaggedType value, SampleMode mode)
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
        auto checksum = PGOMethodInfo::CalcChecksum(jsMethod->GetMethodName(), jsMethod->GetBytecodeArray(),
                                                    jsMethod->GetCodeSize());
        if (recordInfos_->AddMethod(recordName, jsMethod->GetMethodId(), checksum, jsMethod->GetMethodName(), mode)) {
            methodCount_++;
        }
        // Merged every 10 methods
        if (methodCount_ >= MERGED_EVERY_COUNT) {
            LOG_ECMA(DEBUG) << "Sample: post task to save profiler";
            PGOProfilerManager::GetInstance()->Merge(this);
            PGOProfilerManager::GetInstance()->AsynSave();
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
        int32_t ctorMethodId = ctorJSMethod->GetMethodId().GetOffset();
        auto currentType = PGOSampleType::CreateClassType(ctorMethodId);

        auto superFuncValue = JSTaggedValue::GetPrototype(thread, ctorValue);
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
        recordInfos_->AddLayout(currentType, prototypeHClass, PGOObjLayoutKind::PROTOTYPE);

        auto ctorHClass = JSTaggedType(ctorFunction->GetJSHClass());
        recordInfos_->AddLayout(currentType, ctorHClass, PGOObjLayoutKind::CONSTRUCTOR);
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
        PGOObjLayoutKind kind = PGOObjLayoutKind::LOCAL;
        if (hclass->IsClassPrototype()) {
            ctor = JSObject::GetCtorFromPrototype(thread, holder);
            kind = PGOObjLayoutKind::PROTOTYPE;
        } else if (hclass->IsClassConstructor()) {
            ctor = holder;
            kind = PGOObjLayoutKind::CONSTRUCTOR;
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
            PGOSampleType type = PGOSampleType::CreateClassType(methodId.GetOffset());
            recordInfos_->AddType(recordName, jsMethod->GetMethodId(), offset, type);
            if (store) {
                recordInfos_->AddLayout(type, JSTaggedType(hclass), kind);
            }
        }
    }
}
} // namespace panda::ecmascript
