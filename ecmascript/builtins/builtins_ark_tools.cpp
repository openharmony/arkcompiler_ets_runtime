/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/builtins/builtins_ark_tools.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "ecmascript/js_function.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/mem/tagged_object-inl.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/mem/clock_scope.h"

namespace panda::ecmascript::builtins {
using StringHelper = base::StringHelper;

#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
constexpr char FILEDIR[] = "/data/storage/el2/base/files/";
#endif
JSTaggedValue BuiltinsArkTools::ObjectDump(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<EcmaString> str = JSTaggedValue::ToString(thread, GetCallArg(info, 0));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // The default log level of ace_engine and js_runtime is error
    LOG_ECMA(ERROR) << ": " << EcmaStringAccessor(str).ToStdString();

    uint32_t numArgs = info->GetArgsNumber();
    for (uint32_t i = 1; i < numArgs; i++) {
        JSHandle<JSTaggedValue> obj = GetCallArg(info, i);
        std::ostringstream oss;
        obj->Dump(oss);

        // The default log level of ace_engine and js_runtime is error
        LOG_ECMA(ERROR) << ": " << oss.str();
    }

    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::CompareHClass(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> obj1 = GetCallArg(info, 0);
    JSHandle<JSTaggedValue> obj2 = GetCallArg(info, 1);
    JSHClass *obj1Hclass = obj1->GetTaggedObject()->GetClass();
    JSHClass *obj2Hclass = obj2->GetTaggedObject()->GetClass();
    std::ostringstream oss;
    obj1Hclass->Dump(oss);
    obj2Hclass->Dump(oss);
    bool res = (obj1Hclass == obj2Hclass);
    if (!res) {
        LOG_ECMA(ERROR) << "These two object don't share the same hclass:" << oss.str();
    }
    return JSTaggedValue(res);
}

JSTaggedValue BuiltinsArkTools::DumpHClass(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> obj = GetCallArg(info, 0);
    JSHClass *objHclass = obj->GetTaggedObject()->GetClass();
    std::ostringstream oss;
    objHclass->Dump(oss);

    LOG_ECMA(ERROR) << "hclass:" << oss.str();
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::IsTSHClass(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ASSERT(info->GetArgsNumber() == 1);
    JSHandle<JSTaggedValue> object = GetCallArg(info, 0);
    JSHClass *hclass = object->GetTaggedObject()->GetClass();
    bool isTSHClass = hclass->IsTS();
    return GetTaggedBoolean(isTSHClass);
}

JSTaggedValue BuiltinsArkTools::GetHClass(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ASSERT(info->GetArgsNumber() == 1);
    JSHandle<JSTaggedValue> object = GetCallArg(info, 0);
    JSHClass *hclass = object->GetTaggedObject()->GetClass();
    return JSTaggedValue(hclass);
}

JSTaggedValue BuiltinsArkTools::HasTSSubtyping(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ASSERT(info->GetArgsNumber() == 1);
    JSHandle<JSTaggedValue> object = GetCallArg(info, 0);
    JSHClass *hclass = object->GetTaggedObject()->GetClass();
    return GetTaggedBoolean(hclass->HasTSSubtyping());
}

JSTaggedValue BuiltinsArkTools::IsNotHoleProperty(EcmaRuntimeCallInfo *info)
{
    [[maybe_unused]] DisallowGarbageCollection noGc;
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ASSERT(info->GetArgsNumber() == 2);  // 2 : object and key
    JSHandle<JSTaggedValue> object = GetCallArg(info, 0);
    JSTaggedValue key = GetCallArg(info, 1).GetTaggedValue();
    JSHClass *hclass = object->GetTaggedObject()->GetClass();
    int entry = JSHClass::FindPropertyEntry(thread, hclass, key);
    if (entry == -1) {
        return GetTaggedBoolean(false);
    }
    PropertyAttributes attr = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject())->GetAttr(entry);
    return GetTaggedBoolean(attr.IsNotHole());
}

JSTaggedValue BuiltinsArkTools::ExcutePendingJob(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    thread->GetCurrentEcmaContext()->ExecutePromisePendingJob();
    return JSTaggedValue::True();
}

JSTaggedValue BuiltinsArkTools::GetLexicalEnv(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ASSERT(info->GetArgsNumber() == 1);
    JSHandle<JSTaggedValue> object = GetCallArg(info, 0);
    if (object->IsHeapObject() && object->IsJSFunction()) {
        JSHandle<JSFunction> function = JSHandle<JSFunction>::Cast(object);
        return function->GetLexicalEnv();
    }
    return JSTaggedValue::Null();
}

JSTaggedValue BuiltinsArkTools::ForceFullGC(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    const_cast<Heap *>(info->GetThread()->GetEcmaVM()->GetHeap())->CollectGarbage(
        TriggerGCType::FULL_GC, GCReason::EXTERNAL_TRIGGER);
    return JSTaggedValue::True();
}

JSTaggedValue BuiltinsArkTools::RemoveAOTFlag(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ASSERT(info->GetArgsNumber() == 1);
    JSHandle<JSTaggedValue> object = GetCallArg(info, 0);
    if (object->IsHeapObject() && object->IsJSFunction()) {
        JSHandle<JSFunction> func = JSHandle<JSFunction>::Cast(object);
        JSHandle<Method> method = JSHandle<Method>(thread, func->GetMethod());
        method->SetAotCodeBit(false);
    }

    return JSTaggedValue::Undefined();
}

#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
JSTaggedValue BuiltinsArkTools::StartCpuProfiler(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    auto vm = thread->GetEcmaVM();

    // get file name
    JSHandle<JSTaggedValue> fileNameValue = GetCallArg(info, 0);
    std::string fileName = "";
    if (fileNameValue->IsString()) {
        JSHandle<EcmaString> str = JSTaggedValue::ToString(thread, fileNameValue);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        fileName = EcmaStringAccessor(str).ToStdString() + ".cpuprofile";
    } else {
        fileName = GetProfileName();
    }

    if (!CreateFile(fileName)) {
        LOG_ECMA(ERROR) << "CreateFile failed " << fileName;
    }

    // get sampling interval
    JSHandle<JSTaggedValue> samplingIntervalValue = GetCallArg(info, 1);
    uint32_t interval = 500; // 500:Default Sampling interval 500 microseconds
    if (samplingIntervalValue->IsNumber()) {
        interval = JSTaggedValue::ToUint32(thread, samplingIntervalValue);
    }

    DFXJSNApi::StartCpuProfilerForFile(vm, fileName, interval);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::StopCpuProfiler(EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto vm = thread->GetEcmaVM();
    DFXJSNApi::StopCpuProfilerForFile(vm);

    return JSTaggedValue::Undefined();
}

std::string BuiltinsArkTools::GetProfileName()
{
    char time1[16] = {0}; // 16:Time format length
    char time2[16] = {0}; // 16:Time format length
    time_t timep = std::time(nullptr);
    struct tm nowTime1;
    localtime_r(&timep, &nowTime1);
    size_t result = 0;
    result = strftime(time1, sizeof(time1), "%Y%m%d", &nowTime1);
    if (result == 0) {
        LOG_ECMA(ERROR) << "get time failed";
        return "";
    }
    result = strftime(time2, sizeof(time2), "%H%M%S", &nowTime1);
    if (result == 0) {
        LOG_ECMA(ERROR) << "get time failed";
        return "";
    }
    std::string profileName = "cpuprofile-";
    profileName += time1;
    profileName += "TO";
    profileName += time2;
    profileName += ".cpuprofile";
    return profileName;
}

bool BuiltinsArkTools::CreateFile(std::string &fileName)
{
    std::string path = FILEDIR + fileName;
    if (access(path.c_str(), F_OK) == 0) {
        if (access(path.c_str(), W_OK) == 0) {
            fileName = path;
            return true;
        }
        LOG_ECMA(ERROR) << "file create failed, W_OK false";
        return false;
    }
    const mode_t defaultMode = S_IRUSR | S_IWUSR | S_IRGRP; // -rw-r--
    int fd = creat(path.c_str(), defaultMode);
    if (fd == -1) {
        fd = creat(fileName.c_str(), defaultMode);
        if (fd == -1) {
            LOG_ECMA(ERROR) << "file create failed, errno = "<< errno;
            return false;
        }
        close(fd);
        return true;
    } else {
        fileName = path;
        close(fd);
        return true;
    }
}
#endif

// It is used to check whether an object is a proto, and this function can be
// used to check whether the state machine of IC is faulty.
JSTaggedValue BuiltinsArkTools::IsPrototype(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> obj = GetCallArg(info, 0);
    JSHClass *objHclass = obj->GetTaggedObject()->GetClass();
    return JSTaggedValue(objHclass->IsPrototype());
}

JSTaggedValue BuiltinsArkTools::TimeInUs([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    ClockScope scope;
    return JSTaggedValue(scope.GetCurTime());
}
}  // namespace panda::ecmascript::builtins
