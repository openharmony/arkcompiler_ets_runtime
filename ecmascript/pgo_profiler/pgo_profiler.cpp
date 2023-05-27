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
#include "ecmascript/log_wrapper.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"

namespace panda::ecmascript {
void PGOProfiler::Sample(JSTaggedType value, SampleMode mode)
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
        if (recordInfos_->AddMethod(recordName, jsMethod->GetMethodId(), jsMethod->GetMethodName(), mode)) {
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

void PGOProfiler::TypeSample(JSTaggedType func, int32_t offset, uint32_t type)
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

void PGOProfiler::DefineSample(JSTaggedType func, int32_t offset, int32_t methodId)
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
        auto funcMethodId = jsMethod->GetMethodId();
        auto classType = PGOSampleType::CreateClassType(methodId);
        recordInfos_->AddDefine(recordName, funcMethodId, offset, classType);
    }
}

void PGOProfiler::LayoutSample(JSThread *thread, JSTaggedType func, int32_t offset, JSTaggedType hclass, bool store)
{
    if (!isEnable_) {
        return;
    }

    DISALLOW_GARBAGE_COLLECTION;
    if (!JSTaggedValue(hclass).IsJSHClass()) {
        return;
    }
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

        auto hclassObj = JSHClass::Cast(JSTaggedValue(hclass).GetTaggedObject());
        JSHandle<JSTaggedValue> prototype(thread, hclassObj->GetProto());
        if (!prototype->IsJSObject()) {
            return;
        }
        JSHandle<JSTaggedValue> ctorKey = thread->GlobalConstants()->GetHandledConstructorString();
        JSHandle<JSTaggedValue> ctorObj(JSObject::GetProperty(thread, prototype, ctorKey).GetValue());
        if (thread->HasPendingException()) {
            thread->ClearException();
            return;
        }
        if (ctorObj->IsJSFunction()) {
            auto ctorFunc = JSFunction::Cast(ctorObj.GetTaggedValue());
            auto ctorMethod = ctorFunc->GetMethod();
            if (!ctorMethod.IsMethod()) {
                return;
            }
            auto ctorJSMethod = Method::Cast(ctorMethod);
            auto methodId = ctorJSMethod->GetMethodId();
            PGOSampleType type = PGOSampleType::CreateClassType(methodId.GetOffset());
            recordInfos_->AddType(recordName, jsMethod->GetMethodId(), offset, type);
            if (store) {
                recordInfos_->AddLayout(recordName, type, hclass);
            }
        }
    }
}
} // namespace panda::ecmascript
