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
#include "ecmascript/element_accessor-inl.h"
#include "ecmascript/js_function.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/mem/tagged_object-inl.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/property_detector-inl.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/linked_hash_table.h"
#include "builtins_typedarray.h"
#include "ecmascript/jit/jit.h"

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

JSTaggedValue BuiltinsArkTools::IsSlicedString(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ASSERT(info->GetArgsNumber() == 1);
    JSHandle<JSTaggedValue> str = GetCallArg(info, 0);
    return GetTaggedBoolean(str->IsSlicedString());
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

JSTaggedValue BuiltinsArkTools::HiddenStackSourceFile(EcmaRuntimeCallInfo *info)
{
    [[maybe_unused]] DisallowGarbageCollection noGc;
    ASSERT(info);
    JSThread *thread = info->GetThread();
    thread->SetEnableStackSourceFile(false);
    return JSTaggedValue::True();
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
    auto heap = const_cast<Heap *>(info->GetThread()->GetEcmaVM()->GetHeap());
    heap->CollectGarbage(
        TriggerGCType::FULL_GC, GCReason::EXTERNAL_TRIGGER);
    SharedHeap::GetInstance()->CollectGarbage(info->GetThread(), TriggerGCType::SHARED_GC, GCReason::EXTERNAL_TRIGGER);
    heap->GetHeapPrepare();
    return JSTaggedValue::True();
}

JSTaggedValue BuiltinsArkTools::HintGC(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    return JSTaggedValue(const_cast<Heap *>(info->GetThread()->GetEcmaVM()->GetHeap())->
        CheckAndTriggerHintGC());
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

JSTaggedValue BuiltinsArkTools::CheckCircularImport(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<EcmaString> str = JSTaggedValue::ToString(thread, GetCallArg(info, 0));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    bool printOtherCircular = false;
    if (info->GetArgsNumber() == 2) { // 2: input number
        printOtherCircular = GetCallArg(info, 1).GetTaggedValue().ToBoolean();
    }
    CList<CString> referenceList;
    // str: bundleName/moduleName/xxx/xxx
    CString string = ConvertToString(str.GetTaggedValue());
    LOG_ECMA(INFO) << "checkCircularImport begin with: "<< string;
    SourceTextModule::CheckCircularImportTool(thread, string, referenceList, printOtherCircular);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::HashCode(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> key = GetCallArg(info, 0);
    return JSTaggedValue(LinkedHash::Hash(thread, key.GetTaggedValue()));
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

// It is used to check whether a function is aot compiled.
JSTaggedValue BuiltinsArkTools::IsAOTCompiled(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> obj = GetCallArg(info, 0);
    JSHandle<JSFunction> func(thread, obj.GetTaggedValue());
    Method *method = func->GetCallTarget();
    return JSTaggedValue(method->IsAotWithCallField());
}

JSTaggedValue BuiltinsArkTools::IsOnHeap(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> obj = GetCallArg(info, 0);
    return JSTaggedValue(obj.GetTaggedValue().GetTaggedObject()->GetClass()->IsOnHeapFromBitField());
}

// It is used to check whether a function is aot compiled and deopted at runtime.
JSTaggedValue BuiltinsArkTools::IsAOTDeoptimized(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> obj = GetCallArg(info, 0);
    JSHandle<JSFunction> func(thread, obj.GetTaggedValue());
    Method *method = func->GetCallTarget();
    bool isAotCompiled = method->IsAotWithCallField();
    if (isAotCompiled) {
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
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> obj = GetCallArg(info, 0);
    JSHandle<JSFunction> func(thread, obj.GetTaggedValue());
    Method *method = func->GetCallTarget();
    bool isAotCompiled = method->IsAotWithCallField();
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
        method->IsFastCall() ||
        method->GetDeoptType() != kungfu::DeoptType::NOTCHECK ||
        method->GetCodeEntryOrLiteral() == 0) {
        return JSTaggedValue(false);
    }
    return JSTaggedValue(true);
}

JSTaggedValue BuiltinsArkTools::PrintTypedOpProfilerAndReset(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> opStrVal = GetCallArg(info, 0);
    std::string opStr = EcmaStringAccessor(opStrVal.GetTaggedValue()).ToStdString();
    TypedOpProfiler *profiler = thread->GetCurrentEcmaContext()->GetTypdOpProfiler();
    if (profiler != nullptr) {
        profiler->PrintAndReset(opStr);
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::GetElementsKind(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> obj = GetCallArg(info, 0);
    JSHandle<JSObject> receiver(thread, obj.GetTaggedValue());
    ElementsKind kind = receiver->GetClass()->GetElementsKind();
    return JSTaggedValue(static_cast<uint32_t>(kind));
}

JSTaggedValue BuiltinsArkTools::IsRegExpReplaceDetectorValid(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    return JSTaggedValue(PropertyDetector::IsRegExpReplaceDetectorValid(env));
}

JSTaggedValue BuiltinsArkTools::IsRegExpFlagsDetectorValid(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    return JSTaggedValue(PropertyDetector::IsRegExpFlagsDetectorValid(env));
}

JSTaggedValue BuiltinsArkTools::IsNumberStringNotRegexpLikeDetectorValid(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    return JSTaggedValue(PropertyDetector::IsNumberStringNotRegexpLikeDetectorValid(env));
}

JSTaggedValue BuiltinsArkTools::IsSymbolIteratorDetectorValid(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> kind = GetCallArg(info, 0);
    if (!kind->IsString()) {
        return JSTaggedValue::Undefined();
    }
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> mapString = factory->NewFromUtf8ReadOnly("Map");
    if (JSTaggedValue::Equal(thread, kind, JSHandle<JSTaggedValue>(mapString))) {
        return JSTaggedValue(PropertyDetector::IsMapIteratorDetectorValid(env));
    }
    JSHandle<EcmaString> setString = factory->NewFromUtf8ReadOnly("Set");
    if (JSTaggedValue::Equal(thread, kind, JSHandle<JSTaggedValue>(setString))) {
        return JSTaggedValue(PropertyDetector::IsSetIteratorDetectorValid(env));
    }
    JSHandle<EcmaString> stringString = factory->NewFromUtf8ReadOnly("String");
    if (JSTaggedValue::Equal(thread, kind, JSHandle<JSTaggedValue>(stringString))) {
        return JSTaggedValue(PropertyDetector::IsStringIteratorDetectorValid(env));
    }
    JSHandle<EcmaString> arrayString = factory->NewFromUtf8ReadOnly("Array");
    if (JSTaggedValue::Equal(thread, kind, JSHandle<JSTaggedValue>(arrayString))) {
        return JSTaggedValue(PropertyDetector::IsArrayIteratorDetectorValid(env));
    }
    JSHandle<EcmaString> typedarrayString = factory->NewFromUtf8ReadOnly("TypedArray");
    if (JSTaggedValue::Equal(thread, kind, JSHandle<JSTaggedValue>(typedarrayString))) {
        return JSTaggedValue(PropertyDetector::IsTypedArrayIteratorDetectorValid(env));
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::TimeInUs([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    ClockScope scope;
    return JSTaggedValue(scope.GetCurTime());
}
// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::PrepareFunctionForOptimization([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter PrepareFunctionForOptimization()";
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::OptimizeFunctionOnNextCall([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter OptimizeFunctionOnNextCall()";
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::OptimizeMaglevOnNextCall([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter OptimizeMaglevOnNextCall()";
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::DeoptimizeFunction([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter DeoptimizeFunction()";
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::OptimizeOsr([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter OptimizeOsr()";
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::NeverOptimizeFunction([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter NeverOptimizeFunction()";
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::HeapObjectVerify([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter HeapObjectVerify()";
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::DisableOptimizationFinalization([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter DisableOptimizationFinalization()";
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::DeoptimizeNow([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter DeoptimizeNow()";
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::WaitForBackgroundOptimization([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter WaitForBackgroundOptimization()";
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::Gc([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter Gc()";
    return JSTaggedValue::Undefined();
}

// empty function for pgoAssertType
JSTaggedValue BuiltinsArkTools::PGOAssertType([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter PGOAssertType";
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::ToLength([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> key = GetCallArg(info, 0);
    return JSTaggedValue::ToLength(thread, key);
}

JSTaggedValue BuiltinsArkTools::HasHoleyElements([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter HasHoleyElements()";
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> array = GetCallArg(info, 0);
    if (!array->IsJSArray()) {
        return JSTaggedValue::False();
    }
    JSHandle<JSObject> obj(array);
    uint32_t len = JSHandle<JSArray>::Cast(array)->GetArrayLength();
    for (uint32_t i = 0; i < len; i++) {
        if (ElementAccessor::Get(obj, i).IsHole()) {
            return JSTaggedValue::True();
        }
    }
    return JSTaggedValue::False();
}

JSTaggedValue BuiltinsArkTools::HasDictionaryElements([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter HasDictionaryElements()";
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> objValue = GetCallArg(info, 0);
    JSHandle<JSObject> obj = JSHandle<JSObject>::Cast(objValue);
    return JSTaggedValue(obj->GetJSHClass()->IsDictionaryMode());
}

JSTaggedValue BuiltinsArkTools::HasSmiElements([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter HasSmiElements()";
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> array = GetCallArg(info, 0);
    if (!array->IsJSArray()) {
        return JSTaggedValue::False();
    }
    JSHandle<JSObject> obj(array);
    uint32_t len = JSHandle<JSArray>::Cast(array)->GetArrayLength();
    for (uint32_t i = 0; i < len; i++) {
        if (ElementAccessor::Get(obj, i).IsInt()) {
            return JSTaggedValue::True();
        }
    }
    return JSTaggedValue::False();
}

JSTaggedValue BuiltinsArkTools::HasDoubleElements([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter HasDoubleElements()";
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> array = GetCallArg(info, 0);
    if (!array->IsJSArray()) {
        return JSTaggedValue::False();
    }
    JSHandle<JSObject> obj(array);
    uint32_t len = JSHandle<JSArray>::Cast(array)->GetArrayLength();
    for (uint32_t i = 0; i < len; i++) {
        if (ElementAccessor::Get(obj, i).IsDouble() && !ElementAccessor::Get(obj, i).IsZero()) {
            return JSTaggedValue::True();
        }
    }
    return JSTaggedValue::False();
}

JSTaggedValue BuiltinsArkTools::HasObjectElements([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter HasObjectElements()";
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> array = GetCallArg(info, 0);
    if (!array->IsJSArray()) {
        return JSTaggedValue::False();
    }
    JSHandle<JSObject> obj(array);
    uint32_t len = JSHandle<JSArray>::Cast(array)->GetArrayLength();
    for (uint32_t i = 0; i < len; i++) {
        if (ElementAccessor::Get(obj, i).IsObject()) {
            return JSTaggedValue::True();
        }
    }
    return JSTaggedValue::False();
}

JSTaggedValue BuiltinsArkTools::ArrayBufferDetach([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> obj1 = GetCallArg(info, 0);
    JSHandle<JSArrayBuffer> arrBuf = JSHandle<JSArrayBuffer>::Cast(obj1);
    arrBuf->Detach(thread);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::HaveSameMap([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> obj1 = GetCallArg(info, 0);
    JSHandle<JSTaggedValue> obj2 = GetCallArg(info, 1);
    JSHandle<TaggedArray> keys1 = JSTaggedValue::GetOwnPropertyKeys(thread, obj1);
    JSHandle<TaggedArray> keys2 = JSTaggedValue::GetOwnPropertyKeys(thread, obj2);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    uint32_t len = keys1->GetLength();
    if (len != keys2->GetLength()) {
        return JSTaggedValue::False();
    }
    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < len; i++) {
        if (keys1->Get(i) != keys2->Get(i)) {
            return JSTaggedValue::False();
        }
        keyHandle.Update(keys1->Get(i));
        OperationResult result = JSObject::GetProperty(thread, obj1, keyHandle);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSTaggedValue value1 = result.GetValue().GetTaggedValue();
        result = JSObject::GetProperty(thread, obj2, keyHandle);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSTaggedValue value2 = result.GetValue().GetTaggedValue();
        if (FastRuntimeStub::FastTypeOf(thread, value1) !=
            FastRuntimeStub::FastTypeOf(thread, value2)) {
            return JSTaggedValue::False();
        }
    }
    return JSTaggedValue::True();
}

JSTaggedValue BuiltinsArkTools::CreatePrivateSymbol([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> symbolName = GetCallArg(info, 0);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSSymbol> privateNameSymbol = factory->NewPrivateNameSymbol(symbolName);
    JSHandle<JSTaggedValue> symbolValue = JSHandle<JSTaggedValue>::Cast(privateNameSymbol);
    return symbolValue.GetTaggedValue();
}

JSTaggedValue BuiltinsArkTools::IsArray([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> array = GetCallArg(info, 0);
    return JSTaggedValue(array->IsJSArray());
}

JSTaggedValue BuiltinsArkTools::CreateDataProperty([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    uint32_t secondArg = 2;
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> key = GetCallArg(info, 1);
    JSHandle<JSTaggedValue> value = GetCallArg(info, secondArg);
    JSHandle<JSObject> obj = JSHandle<JSObject>::Cast(GetCallArg(info, 0));
    JSObject::CreateDataPropertyOrThrow(thread, obj, key, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return value.GetTaggedValue();
}

JSTaggedValue BuiltinsArkTools::FunctionGetInferredName([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> obj = GetCallArg(info, 0);
    if (obj->IsJSFunction()) {
        JSHandle<JSFunction> funcObj = JSHandle<JSFunction>::Cast(obj);
        JSHandle<JSTaggedValue> funcName = JSFunction::GetFunctionName(thread, JSHandle<JSFunctionBase>(funcObj));
        return funcName.GetTaggedValue();
    }
    return thread->GlobalConstants()->GetHandledEmptyString().GetTaggedValue();
}

JSTaggedValue BuiltinsArkTools::StringLessThan([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> x = GetCallArg(info, 0);
    JSHandle<JSTaggedValue> y = GetCallArg(info, 1);
    ComparisonResult result = JSTaggedValue::Compare(thread, x, y);
    return JSTaggedValue(ComparisonResult::LESS == result);
}

JSTaggedValue BuiltinsArkTools::StringMaxLength([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter StringMaxLength()";
    ASSERT(info);
    return JSTaggedValue(static_cast<uint32_t>(EcmaString::MAX_STRING_LENGTH) - 1);
}

JSTaggedValue BuiltinsArkTools::ArrayBufferMaxByteLength([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter ArrayBufferMaxByteLength()";
    ASSERT(info);
    return JSTaggedValue(INT_MAX);
}

JSTaggedValue BuiltinsArkTools::TypedArrayMaxLength([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter TypedArrayMaxLength()";
    ASSERT(info);
    return JSTaggedValue(BuiltinsTypedArray::MAX_ARRAY_INDEX);
}

JSTaggedValue BuiltinsArkTools::MaxSmi([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter MaxSmi()";
    return JSTaggedValue(INT_MAX);
}

JSTaggedValue BuiltinsArkTools::Is64Bit([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter Is64Bit()";
    bool is64Bit = sizeof(void*) == 8;
    return JSTaggedValue(is64Bit);
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::FinalizeOptimization([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter FinalizeOptimization()";
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::EnsureFeedbackVectorForFunction([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter EnsureFeedbackVectorForFunction()";
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::CompileBaseline([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter CompileBaseline()";
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::DebugGetLoadedScriptIds([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter DebugGetLoadedScriptIds()";
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::ToFastProperties([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter ToFastProperties()";
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::AbortJS([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(FATAL) << "AbortJS()";
    ASSERT(info);
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::InternalizeString([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter InternalizeString()";
    ASSERT(info);
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::HandleDebuggerStatement([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter HandleDebuggerStatement()";
    ASSERT(info);
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::SetAllocationTimeout([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter SetAllocationTimeout()";
    ASSERT(info);
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::HasFastProperties([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter HasFastProperties()";
    ASSERT(info);
    return JSTaggedValue::True();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::HasOwnConstDataProperty([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter HasOwnConstDataProperty()";
    ASSERT(info);
    return JSTaggedValue::True();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::GetHoleNaNUpper([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter GetHoleNaNUpper()";
    ASSERT(info);
    return JSTaggedValue::Null();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::GetHoleNaNLower([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter GetHoleNaNLower()";
    ASSERT(info);
    return JSTaggedValue::Null();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::SystemBreak([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter SystemBreak()";
    ASSERT(info);
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::ScheduleBreak([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter ScheduleBreak()";
    ASSERT(info);
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::EnqueueMicrotask([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter EnqueueMicrotask()";
    ASSERT(info);
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::DebugPrint([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter DebugPrint()";
    ASSERT(info);
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::GetOptimizationStatus([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter GetOptimizationStatus()";
    ASSERT(info);
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::GetUndetectable([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter GetUndetectable()";
    ASSERT(info);
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::SetKeyedProperty([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter SetKeyedProperty()";
    ASSERT(info);
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::DisassembleFunction([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter DisassembleFunction()";
    ASSERT(info);
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::TryMigrateInstance([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter TryMigrateInstance()";
    ASSERT(info);
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::InLargeObjectSpace([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter InLargeObjectSpace()";
    ASSERT(info);
    return JSTaggedValue::True();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::PerformMicrotaskCheckpoint([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter PerformMicrotaskCheckpoint()";
    ASSERT(info);
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::IsJSReceiver([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter IsJSReceiver()";
    ASSERT(info);
    return JSTaggedValue::False();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::IsDictPropertyConstTrackingEnabled([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter IsDictPropertyConstTrackingEnabled()";
    ASSERT(info);
    return JSTaggedValue::False();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::AllocateHeapNumber([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter AllocateHeapNumber()";
    ASSERT(info);
    return JSTaggedValue(0);
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::ConstructConsString([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter ConstructConsString()";
    ASSERT(info);
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::CompleteInobjectSlackTracking([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter CompleteInobjectSlackTracking()";
    ASSERT(info);
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::NormalizeElements([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter NormalizeElements()";
    ASSERT(info);
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::Call([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter Call()";
    ASSERT(info);
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::DebugPushPromise([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter DebugPushPromise()";
    ASSERT(info);
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::SetForceSlowPath([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter SetForceSlowPath()";
    ASSERT(info);
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::NotifyContextDisposed([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter NotifyContextDisposed()";
    ASSERT(info);
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::OptimizeObjectForAddingMultipleProperties([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter OptimizeObjectForAddingMultipleProperties()";
    ASSERT(info);
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::IsBeingInterpreted([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter OptimizeObjectForAddingMultipleProperties()";
    ASSERT(info);
    return JSTaggedValue::Undefined();
}

// empty function for regress-xxx test cases
JSTaggedValue BuiltinsArkTools::ClearFunctionFeedback([[maybe_unused]] EcmaRuntimeCallInfo *info)
{
    LOG_ECMA(DEBUG) << "Enter ClearFunctionFeedback()";
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::JitCompileSync(EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> thisValue = GetCallArg(info, 0);
    if (!thisValue->IsJSFunction()) {
        return JSTaggedValue::False();
    }
    JSHandle<JSFunction> jsFunction(thisValue);
    Jit::Compile(thread->GetEcmaVM(), jsFunction, CompilerTier::FAST,
                 MachineCode::INVALID_OSR_OFFSET, JitCompileMode::SYNC);
    return JSTaggedValue::True();
}

JSTaggedValue BuiltinsArkTools::JitCompileAsync(EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> thisValue = GetCallArg(info, 0);
    if (!thisValue->IsJSFunction()) {
        return JSTaggedValue::False();
    }
    JSHandle<JSFunction> jsFunction(thisValue);
    Jit::Compile(thread->GetEcmaVM(), jsFunction, CompilerTier::FAST,
                 MachineCode::INVALID_OSR_OFFSET, JitCompileMode::ASYNC);
    return JSTaggedValue::True();
}

JSTaggedValue BuiltinsArkTools::WaitJitCompileFinish(EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
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
    if (jsFunction->GetMachineCode() == JSTaggedValue::Undefined()) {
        return JSTaggedValue::False();
    }
    while (jsFunction->GetMachineCode() == JSTaggedValue::Hole()) {
        // just spin check
        thread->CheckSafepoint();
    }
    return JSTaggedValue::True();
}
}  // namespace panda::ecmascript::builtins
