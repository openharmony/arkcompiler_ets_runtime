/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/dependent_infos.h"
#include "ecmascript/dfx/stackinfo/js_stackinfo.h"
#include "ecmascript/dfx/vmstat/opt_code_profiler.h"
#include "ecmascript/mem/verification.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/linked_hash_table.h"
#include "builtins_typedarray.h"
#include "ecmascript/jit/jit.h"

#if defined(PANDA_TARGET_ARM64)
    /* Note: If not open ArkTools option(set by `persist.ark.mem_config_property openArkTools`),  */
    /*       ArkTools return Empty Implementation                                                 */
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define RETURN_IF_DISALLOW_ARKTOOLS(thread)                                 \
        do {                                                                    \
            if (!((thread)->GetEcmaVM()->GetJSOptions().IsOpenArkTools())) {    \
                return JSTaggedValue::Undefined();                              \
            }                                                                   \
        } while (0)
#else
    #define RETURN_IF_DISALLOW_ARKTOOLS(thread) static_cast<void>(0) // NOLINT(cppcoreguidelines-macro-usage)
#endif

namespace panda::ecmascript::builtins {
using StringHelper = base::StringHelper;

#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
constexpr char FILEDIR[] = "/data/storage/el2/base/files/";
#endif

JSTaggedValue BuiltinsArkTools::ObjectDump(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<EcmaString> str = JSTaggedValue::ToString(thread, GetCallArg(info, 0));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // The default log level of ace_engine and js_runtime is error
    LOG_ECMA(ERROR) << ": " << EcmaStringAccessor(str).ToStdString(thread);

    uint32_t numArgs = info->GetArgsNumber();
    for (uint32_t i = 1; i < numArgs; i++) {
        JSHandle<JSTaggedValue> obj = GetCallArg(info, i);
        std::ostringstream oss;
        obj->Dump(thread, oss);

        // The default log level of ace_engine and js_runtime is error
        LOG_ECMA(ERROR) << ": " << oss.str();
    }

    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::CompareHClass(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> obj1 = GetCallArg(info, 0);
    JSHandle<JSTaggedValue> obj2 = GetCallArg(info, 1);
    JSHClass *obj1Hclass = obj1->GetTaggedObject()->GetClass();
    JSHClass *obj2Hclass = obj2->GetTaggedObject()->GetClass();
    std::ostringstream oss;
    obj1Hclass->Dump(thread, oss);
    obj2Hclass->Dump(thread, oss);
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
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> obj = GetCallArg(info, 0);
    JSHClass *objHclass = obj->GetTaggedObject()->GetClass();
    std::ostringstream oss;
    objHclass->Dump(thread, oss);

    LOG_ECMA(ERROR) << "hclass:" << oss.str();
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::GetInlinedPropertiesCount(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> obj = GetCallArg(info, 0);
    JSHClass *objHclass = obj->GetTaggedObject()->GetClass();
    return JSTaggedValue(objHclass->GetInlinedProperties());
}

JSTaggedValue BuiltinsArkTools::IsTSHClass(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ASSERT(info->GetArgsNumber() == 1);
    JSHandle<JSTaggedValue> object = GetCallArg(info, 0);
    JSHClass *hclass = object->GetTaggedObject()->GetClass();
    bool isAOTHClass = hclass->IsAOT();
    return GetTaggedBoolean(isAOTHClass);
}

JSTaggedValue BuiltinsArkTools::GetHClass(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ASSERT(info->GetArgsNumber() == 1);
    JSHandle<JSTaggedValue> object = GetCallArg(info, 0);
    JSHClass *hclass = object->GetTaggedObject()->GetClass();
    return JSTaggedValue(hclass);
}

JSTaggedValue BuiltinsArkTools::IsSlicedString(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ASSERT(info->GetArgsNumber() == 1);
    JSHandle<JSTaggedValue> str = GetCallArg(info, 0);
    return GetTaggedBoolean(str->IsSlicedString());
}

JSTaggedValue BuiltinsArkTools::IsTreeString(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ASSERT(info->GetArgsNumber() == 1);
    JSHandle<JSTaggedValue> str = GetCallArg(info, 0);
    return GetTaggedBoolean(str->IsTreeString());
}

JSTaggedValue BuiltinsArkTools::IsStableJsArray(EcmaRuntimeCallInfo *info)
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ASSERT(info->GetArgsNumber() == 1);
    JSHandle<JSTaggedValue> object = GetCallArg(info, 0);
    return (object->IsStableJSArray(thread)) ?
        GetTaggedBoolean(true) : GetTaggedBoolean(false);
}

JSTaggedValue BuiltinsArkTools::HasConstructor(EcmaRuntimeCallInfo *info)
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ASSERT(info->GetArgsNumber() == 1);
    JSHandle<JSTaggedValue> object = GetCallArg(info, 0);
    return (object->IsHeapObject() && object->GetHeapObject()->GetClass()->HasConstructor()) ?
        GetTaggedBoolean(true) : GetTaggedBoolean(false);
}

JSTaggedValue BuiltinsArkTools::IsNotHoleProperty(EcmaRuntimeCallInfo *info)
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ASSERT(info->GetArgsNumber() == 2);  // 2 : object and key
    JSHandle<JSTaggedValue> object = GetCallArg(info, 0);
    JSTaggedValue key = GetCallArg(info, 1).GetTaggedValue();
    JSHClass *hclass = object->GetTaggedObject()->GetClass();
    int entry = JSHClass::FindPropertyEntry(thread, hclass, key);
    if (entry == -1) {
        return GetTaggedBoolean(false);
    }
    PropertyAttributes attr = LayoutInfo::Cast(hclass->GetLayout(thread).GetTaggedObject())->GetAttr(thread, entry);
    return GetTaggedBoolean(attr.IsNotHole());
}

JSTaggedValue BuiltinsArkTools::HiddenStackSourceFile(EcmaRuntimeCallInfo *info)
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    thread->SetEnableStackSourceFile(false);
    return JSTaggedValue::True();
}

JSTaggedValue BuiltinsArkTools::ExcutePendingJob(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    thread->GetEcmaVM()->ExecutePromisePendingJob();
    return JSTaggedValue::True();
}

JSTaggedValue BuiltinsArkTools::GetLexicalEnv(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ASSERT(info->GetArgsNumber() == 1);
    JSHandle<JSTaggedValue> object = GetCallArg(info, 0);
    if (object->IsHeapObject() && object->IsJSFunction()) {
        JSHandle<JSFunction> function = JSHandle<JSFunction>::Cast(object);
        return function->GetLexicalEnv(thread);
    }
    return JSTaggedValue::Null();
}

JSTaggedValue BuiltinsArkTools::CurrentEnvIsGlobal(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ASSERT(info->GetArgsNumber() == 1);
    JSHandle<JSTaggedValue> object = GetCallArg(info, 0);
    if (object->IsHeapObject() && object->IsJSFunction()) {
        JSHandle<JSFunction> function = JSHandle<JSFunction>::Cast(object);
        if (function->GetLexicalEnv(thread).IsJSGlobalEnv()) {
            return JSTaggedValue::True();
        }
    }
    return JSTaggedValue::False();
}

JSTaggedValue BuiltinsArkTools::ForceFullGC(EcmaRuntimeCallInfo *info)
{
    if (g_isEnableCMCGC) {
        common::BaseRuntime::RequestGC(common::GcType::FULL);
        return JSTaggedValue::True();
    }
    ASSERT(info);
    JSThread *thread = info->GetThread();
    std::string data = JsStackInfo::BuildJsStackTrace(thread, true);
    LOG_ECMA(INFO) << "ArkTools ForceFullGC " << data;

    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->CollectGarbage(TriggerGCType::FULL_GC, GCReason::TRIGGER_BY_JS);
    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::TRIGGER_BY_JS>(
        thread);
    heap->GetHeapPrepare();
    return JSTaggedValue::True();
}

JSTaggedValue BuiltinsArkTools::ForceLazyDeopt(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ASSERT(info->GetArgsNumber() == 3);  // 3 : object, lazy-deopt-type, exception-flag

    JSHandle<JSTaggedValue> object = GetCallArg(info, 0);
    int type = GetCallArg(info, 1)->GetInt();
    if (!object->IsHeapObject()) {
        return JSTaggedValue::Undefined();
    }
    JSHClass *hclass = object->GetTaggedObject()->GetClass();
    JSTaggedValue infos = hclass->GetDependentInfos(thread);
    if (!infos.IsHeapObject()) {
        return JSTaggedValue::Undefined();
    }
    JSHandle<DependentInfos> infosHandle(thread, infos);
    DependentInfos::TriggerLazyDeoptimization(
        infosHandle, thread, static_cast<DependentInfos::DependentState>(type));
    if (GetCallArg(info, 2)->IsTrue()) {    // 2 : Has exception
        THROW_TYPE_ERROR_AND_RETURN(thread, "user-defined exception", JSTaggedValue::Exception());
    }
    return JSTaggedValue::True();
}

JSTaggedValue BuiltinsArkTools::HintGC(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    int value = 0;
    if (info->GetArgsNumber() == 1) {
        value = JSTaggedValue::ToInt8(thread, GetCallArg(info, 0));
        if (value < static_cast<int>(MemoryReduceDegree::LOW)
            || value > static_cast<int>(MemoryReduceDegree::HIGH)) {
            CString errorMsg = "ArkTools.hintGC parameter value should be larger than "
                               + ToCString(static_cast<int>(MemoryReduceDegree::LOW))
                               + "and less than "
                               + ToCString(static_cast<int>(MemoryReduceDegree::HIGH));
            THROW_NEW_ERROR_WITH_MSG_AND_RETURN_VALUE(thread, ErrorType::ERROR, errorMsg.c_str(),
                                                      JSTaggedValue::Exception());
        }
    }
    return JSTaggedValue(const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->
        CheckAndTriggerHintGC(MemoryReduceDegree(value), GCReason::TRIGGER_BY_JS));
}

JSTaggedValue BuiltinsArkTools::RemoveAOTFlag(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ASSERT(info->GetArgsNumber() == 1);
    JSHandle<JSTaggedValue> object = GetCallArg(info, 0);
    if (object->IsHeapObject() && object->IsJSFunction()) {
        JSHandle<JSFunction> func = JSHandle<JSFunction>::Cast(object);
        JSHandle<Method> method = JSHandle<Method>(thread, func->GetMethod(thread));
        method->SetAotCodeBit(false);
    }

    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::CheckCircularImport(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<EcmaString> str = JSTaggedValue::ToString(thread, GetCallArg(info, 0));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    bool printOtherCircular = false;
    if (info->GetArgsNumber() == 2) { // 2: input number
        printOtherCircular = GetCallArg(info, 1).GetTaggedValue().ToBoolean();
    }
    CList<CString> referenceList;
    // str: bundleName/moduleName/xxx/xxx
    CString string = ConvertToString(thread, str.GetTaggedValue());
    LOG_ECMA(INFO) << "checkCircularImport begin with: "<< string;
    SourceTextModule::CheckCircularImportTool(thread, string, referenceList, printOtherCircular);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::HashCode(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> key = GetCallArg(info, 0);
    return JSTaggedValue(LinkedHash::Hash(thread, key.GetTaggedValue()));
}

JSTaggedValue BuiltinsArkTools::PrintMegaICStat(EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    BUILTINS_API_TRACE(thread, Global, PrintMegaICStat);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // start vm runtime stat statistic
#if ECMASCRIPT_ENABLE_MEGA_PROFILER
    thread->PrintMegaICStat();
#endif
    return JSTaggedValue::Undefined();
}

#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
JSTaggedValue BuiltinsArkTools::StartCpuProfiler(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    auto vm = thread->GetEcmaVM();

    // get file name
    JSHandle<JSTaggedValue> fileNameValue = GetCallArg(info, 0);
    std::string fileName = "";
    if (fileNameValue->IsString()) {
        JSHandle<EcmaString> str = JSTaggedValue::ToString(thread, fileNameValue);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        fileName = EcmaStringAccessor(str).ToStdString(thread) + ".cpuprofile";
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
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
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
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> obj = GetCallArg(info, 0);
    JSHClass *objHclass = obj->GetTaggedObject()->GetClass();
    return JSTaggedValue(objHclass->IsPrototype());
}

// It is used to check whether a function is aot compiled.
JSTaggedValue BuiltinsArkTools::IsAOTCompiled(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> obj = GetCallArg(info, 0);
    JSHandle<JSFunction> func(thread, obj.GetTaggedValue());
    return JSTaggedValue(func->IsCompiledCode());
}

// It is used to check whether a function can be fastcall.
JSTaggedValue BuiltinsArkTools::IsFastCall(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> obj = GetCallArg(info, 0);
    JSHandle<JSFunction> func(thread, obj.GetTaggedValue());
    return JSTaggedValue(func->IsCompiledFastCall());
}

// It is used to check whether two functions have same profileTypeInfo
JSTaggedValue BuiltinsArkTools::IsSameProfileTypeInfo(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSFunction> func0 = JSHandle<JSFunction>::Cast(GetCallArg(info, 0));
    JSHandle<JSFunction> func1 = JSHandle<JSFunction>::Cast(GetCallArg(info, 1));
    return JSTaggedValue(func0->GetProfileTypeInfo(thread) == func1->GetProfileTypeInfo(thread));
}

// It is used to check whether a function has valid profileTypeInfo
JSTaggedValue BuiltinsArkTools::IsProfileTypeInfoValid(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSFunction> func = JSHandle<JSFunction>::Cast(GetCallArg(info, 0));
    return JSTaggedValue(func->GetProfileTypeInfo(thread).IsTaggedArray());
}

// It is used to print the IC state of a function with a specific slotId and icKind.
JSTaggedValue BuiltinsArkTools::GetICState(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSFunction> func = JSHandle<JSFunction>::Cast(GetCallArg(info, 0));
    int slotId = JSHandle<JSTaggedValue>(GetCallArg(info, 1))->GetInt();
    int icKind = JSHandle<JSTaggedValue>(GetCallArg(info, 2))->GetInt();
    JSHandle<ProfileTypeInfo> profileTypeInfo = JSHandle<ProfileTypeInfo>(thread, func->GetProfileTypeInfo(thread));
    if (profileTypeInfo.GetTaggedValue().IsUndefined()) {
        JSHandle<EcmaString> noProfileTypeInfo = factory->NewFromUtf8ReadOnly("No ProfileTypeInfo");
        return noProfileTypeInfo.GetTaggedValue();
    }
    ProfileTypeAccessor profileTypeAccessor(thread, profileTypeInfo, slotId, static_cast<ICKind>(icKind));
    auto state = profileTypeAccessor.ICStateToString(profileTypeAccessor.GetICState());
    JSHandle<EcmaString> stateString = factory->NewFromUtf8ReadOnly(state);
    return stateString.GetTaggedValue();
}

JSTaggedValue BuiltinsArkTools::IsOnHeap(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> obj = GetCallArg(info, 0);
    return JSTaggedValue(obj.GetTaggedValue().GetTaggedObject()->GetClass()->IsOnHeapFromBitField());
}

// It is used to check whether a function is aot compiled and deopted at runtime.
JSTaggedValue BuiltinsArkTools::IsAOTDeoptimized(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> obj = GetCallArg(info, 0);
    JSHandle<JSFunction> func(thread, obj.GetTaggedValue());
    bool isAotCompiled = func->IsCompiledCode();
    if (isAotCompiled) {
        Method *method = func->GetCallTarget(thread);
        uint32_t deoptedCount = method->GetDeoptThreshold();
        uint32_t deoptThreshold = thread->GetEcmaVM()->GetJSOptions().GetDeoptThreshold();
        return JSTaggedValue(deoptedCount != deoptThreshold);
    }

    return JSTaggedValue(false);
}

JSTaggedValue BuiltinsArkTools::CheckDeoptStatus(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> obj = GetCallArg(info, 0);
    JSHandle<JSFunction> func(thread, obj.GetTaggedValue());
    Method *method = func->GetCallTarget(thread);
    bool isAotCompiled = func->IsCompiledCode();
    uint16_t threshold = method->GetDeoptThreshold();
    if (threshold > 0) {
        return JSTaggedValue(isAotCompiled);
    }
    // check status before deopt
    JSHandle<JSTaggedValue> hasDeopt = GetCallArg(info, 1);
    if (hasDeopt->IsFalse()) {
        return JSTaggedValue(!isAotCompiled);
    }
    if (!hasDeopt->IsTrue()) {
        return JSTaggedValue(false);
    }
    // check status after deopt
    if (isAotCompiled ||
        func->IsCompiledFastCall() ||
        method->GetDeoptType() != kungfu::DeoptType::NONE ||
        method->GetCodeEntryOrLiteral() == 0) {
        return JSTaggedValue(false);
    }
    return JSTaggedValue(true);
}

JSTaggedValue BuiltinsArkTools::PrintTypedOpProfiler(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> opStrVal = GetCallArg(info, 0);
    std::string opStr = EcmaStringAccessor(opStrVal.GetTaggedValue()).ToStdString(thread);
    TypedOpProfiler* profiler = thread->GetEcmaVM()->GetTypedOpProfiler();
    if (profiler != nullptr) {
        profiler->Print(opStr);
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::ClearTypedOpProfiler(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    TypedOpProfiler* profiler = thread->GetEcmaVM()->GetTypedOpProfiler();
    if (profiler != nullptr) {
        profiler->Clear();
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::GetAPIVersion(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    return JSTaggedValue(thread->GetEcmaVM()->GetVMAPIVersion());
}

JSTaggedValue BuiltinsArkTools::SetAPIVersion(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> value = GetCallArg(info, 0);
    if (value->IsInt()) {
        thread->GetEcmaVM()->SetVMAPIVersion(value->GetInt());
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::GetElementsKind(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> obj = GetCallArg(info, 0);
    JSHClass *hclass = obj->GetTaggedObject()->GetClass();
    ElementsKind kind = hclass->GetElementsKind();
    return JSTaggedValue(Elements::ToUint(kind));
}

JSTaggedValue BuiltinsArkTools::IsRegExpReplaceDetectorValid(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    return JSTaggedValue(!env->GetRegExpReplaceDetector());
}

JSTaggedValue BuiltinsArkTools::IsRegExpFlagsDetectorValid(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    return JSTaggedValue(!env->GetRegExpFlagsDetector());
}

JSTaggedValue BuiltinsArkTools::IsNumberStringNotRegexpLikeDetectorValid(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    return JSTaggedValue(!env->GetNumberStringNotRegexpLikeDetector());
}

JSTaggedValue BuiltinsArkTools::IsSymbolIteratorDetectorValid(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> kind = GetCallArg(info, 0);
    if (!kind->IsString()) {
        return JSTaggedValue::Undefined();
    }
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> mapString = factory->NewFromUtf8ReadOnly("Map");
    if (JSTaggedValue::Equal(thread, kind, JSHandle<JSTaggedValue>(mapString))) {
        return JSTaggedValue(!env->GetMapIteratorDetector());
    }
    JSHandle<EcmaString> setString = factory->NewFromUtf8ReadOnly("Set");
    if (JSTaggedValue::Equal(thread, kind, JSHandle<JSTaggedValue>(setString))) {
        return JSTaggedValue(!env->GetSetIteratorDetector());
    }
    JSHandle<EcmaString> stringString = factory->NewFromUtf8ReadOnly("String");
    if (JSTaggedValue::Equal(thread, kind, JSHandle<JSTaggedValue>(stringString))) {
        return JSTaggedValue(!env->GetStringIteratorDetector());
    }
    JSHandle<EcmaString> arrayString = factory->NewFromUtf8ReadOnly("Array");
    if (JSTaggedValue::Equal(thread, kind, JSHandle<JSTaggedValue>(arrayString))) {
        return JSTaggedValue(!env->GetArrayIteratorDetector());
    }
    JSHandle<EcmaString> typedarrayString = factory->NewFromUtf8ReadOnly("TypedArray");
    if (JSTaggedValue::Equal(thread, kind, JSHandle<JSTaggedValue>(typedarrayString))) {
        return JSTaggedValue(!env->GetTypedArrayIteratorDetector());
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::TimeInUs([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    [[maybe_unused]] JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    ClockScope scope;
    return JSTaggedValue(scope.GetCurTime());
}

#if ECMASCRIPT_ENABLE_COLLECTING_OPCODES
JSTaggedValue BuiltinsArkTools::StartCollectingOpcodes([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    std::unordered_map<BytecodeInstruction::Opcode, int> bytecodeStatsMap;
    [[maybe_unused]] JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    EcmaVM *vm = thread->GetEcmaVM();
    vm->SetBytecodeStatsStack(bytecodeStatsMap);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<EcmaString> str = JSTaggedValue::ToString(thread, GetCallArg(info, 0));
    auto msg = EcmaStringAccessor(str).ToCString(thread);
    LOG_ECMA(ERROR) << msg.c_str();
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::StopCollectingOpcodes([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    [[maybe_unused]] JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    EcmaVM *vm = thread->GetEcmaVM();
    vm->PrintCollectedByteCode();
    std::stack<std::unordered_map<BytecodeInstruction::Opcode, int>> &bytecodeStatsStack_ =
            vm->GetBytecodeStatsStack();
    bytecodeStatsStack_.pop();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<EcmaString> str = JSTaggedValue::ToString(thread, GetCallArg(info, 0));
    auto msg = EcmaStringAccessor(str).ToCString(thread);
    LOG_ECMA(ERROR) << msg.c_str();
    return JSTaggedValue::Undefined();
}
#endif

#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
JSTaggedValue BuiltinsArkTools::StartScopeLockStats(EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    auto vm = thread->GetEcmaVM();
    vm->StartCollectingScopeLockStats();
    LOG_FULL(INFO) << "Start Collecting ArkCompiler Scope-Lock Stats";
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::StopScopeLockStats(EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    auto vm = thread->GetEcmaVM();
    LOG_FULL(INFO) << "Stop Collecting ArkCompiler Scope-Lock Stats: "
                   << " ThreadStateTransition count: " << vm->GetUpdateThreadStateTransCount()
                   << " , Entered Scope But NO State Transition count: " << (vm->GetEnterJsiNativeScopeCount() +
                                                                     vm->GetEnterFastNativeScopeCount() +
                                                                     vm->GetEnterThreadManagedScopeCount() -
                                                                     vm->GetUpdateThreadStateTransCount())
                   << " , String-Table Lock count: " << vm->GetStringTableLockCount();
    vm->ResetScopeLockStats();
    vm->StopCollectingScopeLockStats();
    return JSTaggedValue::Undefined();
}
#endif

static JSTaggedValue UnimplementedBuiltin(char const *name, [[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    RETURN_IF_DISALLOW_ARKTOOLS(info->GetThread());
    LOG_ECMA(DEBUG) << "Enter unimplemented ArkTools." << name;
    return JSTaggedValue::Undefined();
}

static JSTaggedValue BuiltinFail(JSThread *thread, char const *msg)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    THROW_NEW_ERROR_WITH_MSG_AND_RETURN_VALUE(thread, ErrorType::ERROR, msg, JSTaggedValue::Exception());
}

static JSTaggedValue NotSupportedBuiltin(char const *name, [[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    std::string msg = std::string(name) + " is not supported";
    return BuiltinFail(thread, msg.c_str());
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::PrepareFunctionForOptimization([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::OptimizeFunctionOnNextCall([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::OptimizeMaglevOnNextCall([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::DeoptimizeFunction([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::OptimizeOsr([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::NeverOptimizeFunction([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

JSTaggedValue BuiltinsArkTools::HeapObjectVerify([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    CHECK(info && info->GetArgsNumber() == 1);
    JSHandle<JSTaggedValue> arg = GetCallArg(info, 0);

    if (arg->IsHeapObject()) {
        JSHandle<TaggedObject> obj(arg);
        CHECK(obj->GetClass()->GetClass()->IsHClass());

        size_t failCount = 0;
        VerifyObjectVisitor heapVerifier(thread->GetEcmaVM()->GetHeap(), &failCount);
        heapVerifier(*obj);
        CHECK(failCount == 0);
    }
    return JSTaggedValue::True();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::DisableOptimizationFinalization([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::DeoptimizeNow([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::WaitForBackgroundOptimization([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

JSTaggedValue BuiltinsArkTools::Gc([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    TriggerGCType gctype = TriggerGCType::FULL_GC;

    if (info->GetArgsNumber() != 0) {
        JSHandle<JSTaggedValue> arg = GetCallArg(info, 0);
        if (arg->IsECMAObject()) {
            return BuiltinFail(thread, "ArkTools.gc object parameter is not supported");
        }
        gctype = TriggerGCType::YOUNG_GC;
    }
    thread->GetEcmaVM()->CollectGarbage(gctype, GCReason::EXTERNAL_TRIGGER);
    return JSTaggedValue::Undefined();
}

// empty function for pgoAssertType
JSTaggedValue BuiltinsArkTools::PGOAssertType([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter PGOAssertType";
    [[maybe_unused]] JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::ToLength([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> key = GetCallArg(info, 0);
    return JSTaggedValue::ToLength(thread, key);
}

template <typename Pred>
static JSTaggedValue TestElementsKind([[maybe_unused]] EcmaRuntimeCallInfo *info, Pred const &pred)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    CHECK(info && info->GetArgsNumber() == 1);
    JSHandle<JSTaggedValue> arg = base::BuiltinsBase::GetCallArg(info, 0);
    CHECK(thread->GetEcmaVM()->IsEnableElementsKind());
    CHECK(arg->IsJSObject());
    ElementsKind kind = JSHandle<JSObject>::Cast(arg)->GetClass()->GetElementsKind();
    return JSTaggedValue(pred(kind));
}

JSTaggedValue BuiltinsArkTools::HasDictionaryElements([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    CHECK(info && info->GetArgsNumber() == 1);
    JSHandle<JSTaggedValue> arg = base::BuiltinsBase::GetCallArg(info, 0);
    CHECK(thread->GetEcmaVM()->IsEnableElementsKind());
    CHECK(arg->IsJSObject());
    JSHandle<JSObject> obj(arg);
    bool isDict = obj->GetClass()->IsDictionaryElement();
    CHECK(isDict == ElementAccessor::IsDictionaryMode(thread, obj));
    CHECK(isDict == (obj->GetClass()->GetElementsKind() == ElementsKind::DICTIONARY));
    return JSTaggedValue(isDict);
}

JSTaggedValue BuiltinsArkTools::HasHoleyElements([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return TestElementsKind(info, [](ElementsKind kind) {
        return (helpers::ToUnderlying(kind) & helpers::ToUnderlying(ElementsKind::HOLE)) != 0;
    });
}

JSTaggedValue BuiltinsArkTools::HasSmiElements([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return TestElementsKind(info, [](ElementsKind kind) {
        return kind == ElementsKind::INT || kind == ElementsKind::HOLE_INT;
    });
}

JSTaggedValue BuiltinsArkTools::HasDoubleElements([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return TestElementsKind(info, [](ElementsKind kind) {
        return kind == ElementsKind::NUMBER || kind == ElementsKind::HOLE_NUMBER;
    });
}

JSTaggedValue BuiltinsArkTools::HasObjectElements([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return TestElementsKind(info, [](ElementsKind kind) {
        ElementsKind noHole = static_cast<ElementsKind>(helpers::ToUnderlying(kind)
            & ~helpers::ToUnderlying(ElementsKind::HOLE));
        return noHole == ElementsKind::STRING || noHole == ElementsKind::OBJECT || noHole == ElementsKind::TAGGED;
    });
}

JSTaggedValue BuiltinsArkTools::ArrayBufferDetach([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    CHECK(info && info->GetArgsNumber() == 1);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> obj1 = GetCallArg(info, 0);
    JSHandle<JSArrayBuffer> arrBuf = JSHandle<JSArrayBuffer>::Cast(obj1);
    arrBuf->Detach(thread);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::HaveSameMap([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    RETURN_IF_DISALLOW_ARKTOOLS(info->GetThread());
    CHECK(info && info->GetArgsNumber() == 2);  // 2 args
    JSHandle<JSTaggedValue> obj1 = GetCallArg(info, 0);
    JSHandle<JSTaggedValue> obj2 = GetCallArg(info, 1);
    CHECK(obj1->IsHeapObject() && obj2->IsHeapObject());
    return JSTaggedValue(obj1->GetTaggedObject()->GetClass() == obj2->GetTaggedObject()->GetClass());
}

JSTaggedValue BuiltinsArkTools::IsSameHeapObject([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    RETURN_IF_DISALLOW_ARKTOOLS(info->GetThread());
    CHECK(info && info->GetArgsNumber() == 2);  // 2 args
    JSHandle<JSTaggedValue> obj1 = GetCallArg(info, 0);
    JSHandle<JSTaggedValue> obj2 = GetCallArg(info, 1);
    if (obj1->IsDouble() && obj2->IsDouble()) {
        return JSTaggedValue(false); // mocked result
    }
    CHECK(obj1->IsHeapObject() && obj2->IsHeapObject());
    return JSTaggedValue(obj1->GetTaggedObject() == obj2->GetTaggedObject());
}

// mock builtin
JSTaggedValue BuiltinsArkTools::IsSmi([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    RETURN_IF_DISALLOW_ARKTOOLS(info->GetThread());
    CHECK(info && info->GetArgsNumber() == 1);
    return JSTaggedValue(info->GetCallArg(0)->IsInt());
}

JSTaggedValue BuiltinsArkTools::CreatePrivateSymbol([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    CHECK(info && info->GetArgsNumber() == 1);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> symbolName = GetCallArg(info, 0);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSSymbol> privateNameSymbol = factory->NewPrivateNameSymbol(symbolName);
    JSHandle<JSTaggedValue> symbolValue = JSHandle<JSTaggedValue>::Cast(privateNameSymbol);
    return symbolValue.GetTaggedValue();
}

JSTaggedValue BuiltinsArkTools::IsArray([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    RETURN_IF_DISALLOW_ARKTOOLS(info->GetThread());
    CHECK(info && info->GetArgsNumber() == 1);
    return JSTaggedValue(info->GetCallArg(0)->IsJSArray());
}

JSTaggedValue BuiltinsArkTools::CreateDataProperty([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    CHECK(info && info->GetArgsNumber() == 3);  // 3 args
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CHECK(GetCallArg(info, 0)->IsJSObject());
    JSHandle<JSObject> obj(GetCallArg(info, 0));          // 0: object
    JSHandle<JSTaggedValue> key = GetCallArg(info, 1);    // 1: property key
    JSHandle<JSTaggedValue> value = GetCallArg(info, 2);  // 2: property value
    JSObject::CreateDataPropertyOrThrow(thread, obj, key, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return value.GetTaggedValue();
}

JSTaggedValue BuiltinsArkTools::FunctionGetInferredName([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    CHECK(info && info->GetArgsNumber() == 1);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> obj = GetCallArg(info, 0);
    if (obj->IsJSFunction()) {
        JSHandle<JSFunction> funcObj = JSHandle<JSFunction>::Cast(obj);
        std::string name = Method::ConstCast(funcObj->GetMethod(thread).GetTaggedObject())->ParseFunctionName(thread);
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        return factory->NewFromStdString(name).GetTaggedValue();
    }
    return thread->GlobalConstants()->GetHandledEmptyString().GetTaggedValue();
}

JSTaggedValue BuiltinsArkTools::StringLessThan([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    CHECK(info && info->GetArgsNumber() == 2);  // 2 args
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> x = GetCallArg(info, 0);
    JSHandle<JSTaggedValue> y = GetCallArg(info, 1);
    ComparisonResult result = JSTaggedValue::Compare(thread, x, y);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue(ComparisonResult::LESS == result);
}

JSTaggedValue BuiltinsArkTools::StringMaxLength([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    RETURN_IF_DISALLOW_ARKTOOLS(info->GetThread());
    CHECK(info && info->GetArgsNumber() == 0);
    return JSTaggedValue(static_cast<uint32_t>(BaseString::MAX_STRING_LENGTH) - 1);
}

JSTaggedValue BuiltinsArkTools::ArrayBufferMaxByteLength([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    RETURN_IF_DISALLOW_ARKTOOLS(info->GetThread());
    CHECK(info && info->GetArgsNumber() == 0);
    return JSTaggedValue(INT_MAX);
}

JSTaggedValue BuiltinsArkTools::TypedArrayMaxLength([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    RETURN_IF_DISALLOW_ARKTOOLS(info->GetThread());
    CHECK(info && info->GetArgsNumber() == 0);
    return JSTaggedValue(BuiltinsTypedArray::MAX_ARRAY_INDEX);
}

JSTaggedValue BuiltinsArkTools::MaxSmi([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    RETURN_IF_DISALLOW_ARKTOOLS(info->GetThread());
    CHECK(info && info->GetArgsNumber() == 0);
    return JSTaggedValue(INT32_MAX);
}

JSTaggedValue BuiltinsArkTools::Is64Bit([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    RETURN_IF_DISALLOW_ARKTOOLS(info->GetThread());
    CHECK(info && info->GetArgsNumber() == 0);
    return JSTaggedValue(sizeof(void*) == 8);  // 8 is 64bit pointer size
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::FinalizeOptimization([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

JSTaggedValue BuiltinsArkTools::EnsureFeedbackVectorForFunction([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    CHECK(info && info->GetArgsNumber() == 1);
    CHECK(info->GetCallArg(0)->IsJSFunction());
    JSHandle<JSFunction> func(info->GetCallArg(0));
    auto prof = func->GetProfileTypeInfo(thread);
    CHECK(prof.IsUndefined() || prof.GetHeapObject()->GetClass()->IsTaggedArray());
    return JSTaggedValue(!prof.IsUndefined());
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::CompileBaseline([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

JSTaggedValue BuiltinsArkTools::DebugGetLoadedScriptIds([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

JSTaggedValue BuiltinsArkTools::ToFastProperties([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    CHECK(info && info->GetArgsNumber() == 1);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> arg = info->GetCallArg(0);

    if (arg->IsJSObject() && !arg->IsJSGlobalObject()) {
        JSHandle<JSObject> obj(arg);
        // NOTE: extracted from JSHClass::OptimizeAsFastElements
        if (obj->GetJSHClass()->IsDictionaryMode()) {
            JSObject::OptimizeAsFastProperties(thread, obj);
        } else {
            JSHClass::OptimizeAsFastProperties(thread, obj);
        }
    }
    return arg.GetTaggedValue();
}

JSTaggedValue BuiltinsArkTools::AbortJS([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    RETURN_IF_DISALLOW_ARKTOOLS(info->GetThread());
    CHECK(info && info->GetArgsNumber() == 1);
    CHECK(info->GetCallArg(0)->IsString());
    JSHandle<EcmaString> msg(info->GetCallArg(0));
    std::cerr << "AbortJS: " << EcmaStringAccessor(msg).ToCString(info->GetThread(), StringConvertedUsage::PRINT)
              << std::endl;
    panda::PrintStack(std::cerr);
    std::abort();
}

JSTaggedValue BuiltinsArkTools::InternalizeString([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    CHECK(info && info->GetArgsNumber() == 1);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CHECK(info->GetCallArg(0)->IsString());
    return JSTaggedValue(thread->GetEcmaVM()->GetFactory()->InternString(info->GetCallArg(0)));
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::HandleDebuggerStatement([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::SetAllocationTimeout([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

JSTaggedValue BuiltinsArkTools::HasFastProperties([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    RETURN_IF_DISALLOW_ARKTOOLS(info->GetThread());
    CHECK(info && info->GetArgsNumber() == 1);
    CHECK(info->GetCallArg(0)->IsJSObject());
    JSHandle<JSObject> obj(info->GetCallArg(0));
    return JSTaggedValue(!obj->GetClass()->IsDictionaryMode());
}

JSTaggedValue BuiltinsArkTools::HasOwnConstDataProperty([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    CHECK(info && info->GetArgsNumber() == 2);  // 2 args
    JSHandle<JSTaggedValue> rec = info->GetCallArg(0);
    JSHandle<JSTaggedValue> prop = info->GetCallArg(1);

    if (!(prop->IsNumber() || prop->IsString() || prop->IsSymbol())) {
        return JSTaggedValue::Undefined();
    }
    if (!rec->IsJSObject()) {
        return JSTaggedValue::Undefined();
    }

    JSHandle<JSObject> obj(rec);
    ObjectOperator op(thread, rec, rec, prop, OperatorType::OWN);
    if (!op.IsFound()) {
        return JSTaggedValue::False();
    }
    if (!op.IsAccessorDescriptor()) {
        return JSTaggedValue(op.GetAttr().IsConstProps());
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::GetHoleNaNUpper([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return NotSupportedBuiltin(__func__, info);
}

JSTaggedValue BuiltinsArkTools::GetHoleNaNLower([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return NotSupportedBuiltin(__func__, info);
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::SystemBreak([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::ScheduleBreak([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

JSTaggedValue BuiltinsArkTools::EnqueueMicrotask([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    CHECK(info && info->GetArgsNumber() == 1);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CHECK(info->GetCallArg(0)->IsJSFunction());
    JSHandle<JSFunction> func(info->GetCallArg(0));

    JSHandle<job::MicroJobQueue> queue = thread->GetEcmaVM()->GetMicroJobQueue();
    JSHandle<TaggedArray> argv(thread->GlobalConstants()->GetHandledEmptyArray());

    job::MicroJobQueue::EnqueueJob(thread, queue, job::QueueType::QUEUE_PROMISE, func, argv);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::DebugPrint([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::GetOptimizationStatus([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

JSTaggedValue BuiltinsArkTools::GetUndetectable([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    RETURN_IF_DISALLOW_ARKTOOLS(info->GetThread());
    ASSERT(info && info->GetArgsNumber() == 0);
    return JSTaggedValue::Undefined(); // undetectable is just undefined
}

JSTaggedValue BuiltinsArkTools::SetKeyedProperty([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    CHECK(info && info->GetArgsNumber() == 3);  // 3 args
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> obj = info->GetCallArg(0);  // 0: object
    JSHandle<JSTaggedValue> key = info->GetCallArg(1);  // 1: property key
    JSHandle<JSTaggedValue> val = info->GetCallArg(2);  // 2: property value

    JSTaggedValue::SetProperty(thread, obj, key, val, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::DisassembleFunction([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

JSTaggedValue BuiltinsArkTools::TryMigrateInstance([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return NotSupportedBuiltin(__func__, info);
}

JSTaggedValue BuiltinsArkTools::InLargeObjectSpace([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    RETURN_IF_DISALLOW_ARKTOOLS(info->GetThread());
    CHECK(info && info->GetArgsNumber() == 1);
    JSHandle<JSTaggedValue> arg = info->GetCallArg(0);
    CHECK(arg->IsHeapObject());
    Region *region = Region::ObjectAddressToRange(arg->GetTaggedObject());
    return JSTaggedValue(region->InHugeObjectSpace());
}

JSTaggedValue BuiltinsArkTools::PerformMicrotaskCheckpoint([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    ASSERT(info && info->GetArgsNumber() == 0);
    thread->GetEcmaVM()->ExecutePromisePendingJob();
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::IsJSReceiver([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    RETURN_IF_DISALLOW_ARKTOOLS(info->GetThread());
    ASSERT(info && info->GetArgsNumber() == 1);
    return JSTaggedValue(info->GetCallArg(0)->IsECMAObject());
}

JSTaggedValue BuiltinsArkTools::IsDictPropertyConstTrackingEnabled([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    RETURN_IF_DISALLOW_ARKTOOLS(info->GetThread());
    ASSERT(info && info->GetArgsNumber() == 0);
    return JSTaggedValue(false);
}

// mock builtin
JSTaggedValue BuiltinsArkTools::AllocateHeapNumber([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    RETURN_IF_DISALLOW_ARKTOOLS(info->GetThread());
    ASSERT(info && info->GetArgsNumber() == 0);
    return JSTaggedValue(0.0);
}

JSTaggedValue BuiltinsArkTools::ConstructConsString([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    CHECK(info && info->GetArgsNumber() == 2);  // 2 args
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CHECK(info->GetCallArg(0)->IsString());
    CHECK(info->GetCallArg(1)->IsString());
    JSHandle<EcmaString> str1(info->GetCallArg(0));
    JSHandle<EcmaString> str2(info->GetCallArg(1));

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->ConcatFromString(str1, str2).GetTaggedValue();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::CompleteInobjectSlackTracking([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

JSTaggedValue BuiltinsArkTools::NormalizeElements([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    CHECK(info && info->GetArgsNumber() == 1);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CHECK(info->GetCallArg(0)->IsJSObject());
    JSHandle<JSObject> obj(info->GetCallArg(0));
    JSObject::ElementsToDictionary(thread, obj);
    return obj.GetTaggedValue();
}

JSTaggedValue BuiltinsArkTools::Call([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    static constexpr uint32_t MIN_ARGS = 2;
    CHECK(info && info->GetArgsNumber() >= MIN_ARGS);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    uint32_t argc = info->GetArgsNumber() - MIN_ARGS;
    JSHandle<JSTaggedValue> callee = info->GetCallArg(0);
    JSHandle<JSTaggedValue> receiver = info->GetCallArg(1);

    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *calleeInfo = EcmaInterpreter::NewRuntimeCallInfo(thread, callee, receiver, undefined, argc);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    for (uint32_t i = 0; i < argc; ++i) {
        calleeInfo->SetCallArg(i, info->GetCallArg(i + MIN_ARGS).GetTaggedValue());
    }
    return JSFunction::Call(calleeInfo);
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::DebugPushPromise([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::SetForceSlowPath([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::OptimizeObjectForAddingMultipleProperties([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::IsBeingInterpreted([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::ClearFunctionFeedback([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    return UnimplementedBuiltin(__func__, info);
}

JSTaggedValue BuiltinsArkTools::JitCompileSync(EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> thisValue = GetCallArg(info, 0);
    if (!thisValue->IsJSFunction()) {
        return JSTaggedValue::False();
    }
    JSHandle<JSFunction> jsFunction(thisValue);
    Jit::Compile(thread->GetEcmaVM(), jsFunction, CompilerTier::Tier::FAST,
                 MachineCode::INVALID_OSR_OFFSET, JitCompileMode::Mode::SYNC);
    return JSTaggedValue::True();
}

JSTaggedValue BuiltinsArkTools::JitCompileAsync(EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> thisValue = GetCallArg(info, 0);
    if (!thisValue->IsJSFunction()) {
        return JSTaggedValue::False();
    }
    JSHandle<JSFunction> jsFunction(thisValue);
    Jit::Compile(thread->GetEcmaVM(), jsFunction, CompilerTier::Tier::FAST,
                 MachineCode::INVALID_OSR_OFFSET, JitCompileMode::Mode::ASYNC);
    return JSTaggedValue::True();
}

JSTaggedValue BuiltinsArkTools::WaitJitCompileFinish(EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> thisValue = GetCallArg(info, 0);
    if (!thisValue->IsJSFunction()) {
        return JSTaggedValue::False();
    }
    JSHandle<JSFunction> jsFunction(thisValue);

    auto jit = Jit::GetInstance();
    if (!jit->IsEnableFastJit()) {
        return JSTaggedValue::False();
    }
    while (!jsFunction->GetMachineCode(thread).IsMachineCodeObject()) {
        if (jsFunction->GetJitHotnessCnt(thread) == ProfileTypeInfo::JIT_DISABLE_FLAG) {
            // The current function is not compiled for some reason.
            break;
        }
        // just spin check
        thread->SetInstallMachineCode(true);
        thread->CheckSafepoint();
    }
    return JSTaggedValue::True();
}

JSTaggedValue BuiltinsArkTools::WaitAllJitCompileFinish(EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    auto jit = Jit::GetInstance();
    if (!jit->IsEnableFastJit()) {
        return JSTaggedValue::False();
    }
    while (Jit::GetInstance()->GetRunningTaskCnt(thread->GetEcmaVM())) {
        thread->SetInstallMachineCode(true);
        thread->CheckSafepoint();
    }
    thread->SetPGOProfilerEnable(false);
    thread->CheckOrSwitchPGOStubs();
    thread->GetEcmaVM()->GetJSOptions().SetEnablePGOProfiler(false);
    return JSTaggedValue::True();
}

JSTaggedValue BuiltinsArkTools::IsInFastJit(EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSTaggedType *currentFrame = const_cast<JSTaggedType*>(thread->GetCurrentFrame());
    for (FrameIterator it(currentFrame, thread); !it.Done(); it.Advance<GCVisitedFlag::VISITED>()) {
        if (!it.IsJSFrame()) {
            continue;
        }
        return (it.IsOptimizedJSFunctionFrame() || it.IsFastJitFunctionFrame() ?
            JSTaggedValue::True() : JSTaggedValue::False());
    }
    return JSTaggedValue::False();
}

JSTaggedValue BuiltinsArkTools::StartRuntimeStat(EcmaRuntimeCallInfo *msg)
{
    JSThread *thread = msg->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    BUILTINS_API_TRACE(thread, Global, StartRuntimeStat);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // start vm runtime stat statistic
    thread->GetEcmaVM()->SetRuntimeStatEnable(true);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::StopRuntimeStat(EcmaRuntimeCallInfo *msg)
{
    JSThread *thread = msg->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    BUILTINS_API_TRACE(thread, Global, StopRuntimeStat);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // stop vm runtime stat statistic
    thread->GetEcmaVM()->SetRuntimeStatEnable(false);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::IterateFrame(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSTaggedType *currentFrame = const_cast<JSTaggedType *>(thread->GetCurrentFrame());

    class DummyRootVisitor final : public RootVisitor {
    public:
        DummyRootVisitor() = default;
        ~DummyRootVisitor() = default;

        void VisitRoot([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot slot) override {}
        void VisitRangeRoot([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot start,
            [[maybe_unused]] ObjectSlot end) override {}
        void VisitBaseAndDerivedRoot([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base,
            [[maybe_unused]] ObjectSlot derived, [[maybe_unused]] uintptr_t baseOldObject) override {}
    };
    DummyRootVisitor visitor;

    for (FrameIterator it(currentFrame, thread); !it.Done(); it.Advance<GCVisitedFlag::VISITED>()) {
        bool ret = it.IteratorStackMapAndDeopt(visitor);
        FrameType type = it.GetFrameType();
        LOG_BUILTINS(INFO) << "IterateFrameType: " << (int)type;
        if (!ret) {
            break;
        }
    }

    for (FrameIterator it(currentFrame, thread); !it.Done(); it.Advance<GCVisitedFlag::DEOPT>()) {
        FrameType type = it.GetFrameType();
        LOG_BUILTINS(INFO) << "DeoptIterateFrameType: " << (int)type;
    }

    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::TriggerSharedGC(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    RETURN_IF_DISALLOW_ARKTOOLS(thread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSTaggedValue type = GetCallArg(info, 0).GetTaggedValue();
    auto globalConst = thread->GlobalConstants();
    SharedHeap *sHeap = SharedHeap::GetInstance();
    sHeap->WaitGCFinished(thread);
    if (JSTaggedValue::StrictEqual(thread, globalConst->GetSharedGcCause(), type)) {
        sHeap->TriggerConcurrentMarking<TriggerGCType::SHARED_GC, MarkReason::TRIGGER_BY_JS>(thread);
        sHeap->WaitGCFinished(thread);
    } else if (JSTaggedValue::StrictEqual(thread, globalConst->GetSharedPartialGcCause(), type)) {
        sHeap->TriggerConcurrentMarking<TriggerGCType::SHARED_PARTIAL_GC, MarkReason::TRIGGER_BY_JS>(thread);
        sHeap->WaitGCFinished(thread);
    } else if (JSTaggedValue::StrictEqual(thread, globalConst->GetSharedFullGcCause(), type)) {
        sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::TRIGGER_BY_JS>(thread);
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::InYoungSpace(EcmaRuntimeCallInfo *info)
{
    RETURN_IF_DISALLOW_ARKTOOLS(info->GetThread());
    CHECK(info && info->GetArgsNumber() == 1);
    JSHandle<JSTaggedValue> arg = info->GetCallArg(0);
    CHECK(arg->IsHeapObject());
    Region *region = Region::ObjectAddressToRange(arg->GetTaggedObject());
    return JSTaggedValue(region->InYoungSpace());
}

JSTaggedValue BuiltinsArkTools::InOldSpace(EcmaRuntimeCallInfo *info)
{
    RETURN_IF_DISALLOW_ARKTOOLS(info->GetThread());
    CHECK(info && info->GetArgsNumber() == 1);
    JSHandle<JSTaggedValue> arg = info->GetCallArg(0);
    CHECK(arg->IsHeapObject());
    Region *region = Region::ObjectAddressToRange(arg->GetTaggedObject());
    return JSTaggedValue(region->InOldSpace());
}

JSTaggedValue BuiltinsArkTools::CreateNapiObject(EcmaRuntimeCallInfo *msg)
{
    JSThread *thread = msg->GetThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> jsObject(factory->CreateNapiObject());
    return jsObject.GetTaggedValue();
}

JSTaggedValue BuiltinsArkTools::SetHotReloadPatchMain(EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    thread->SetStageOfHotReload(StageOfHotReload::LOAD_END_EXECUTE_PATCHMAIN);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::SetNoHotReloadPatchMain(EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    thread->SetStageOfHotReload(StageOfHotReload::UNLOAD_END_EXECUTE_PATCHMAIN);
    return JSTaggedValue::Undefined();
}
} // namespace panda::ecmascript::builtins
