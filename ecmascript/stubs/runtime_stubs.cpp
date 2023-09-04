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

#include <cmath>
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/log.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/stubs/runtime_stubs-inl.h"
#include "ecmascript/accessor_data.h"
#include "ecmascript/base/fast_json_stringifier.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/compiler/builtins/containers_stub_builder.h"
#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/compiler/ecma_opcode_des.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/dfx/vmstat/function_call_timer.h"
#include "ecmascript/dfx/vmstat/opt_code_profiler.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/frames.h"
#include "ecmascript/global_env.h"
#include "ecmascript/ic/ic_runtime.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/ic/properties_cache.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/interpreter/interpreter_assembly.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/layout_info.h"
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/message_string.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/pgo_profiler/pgo_profiler.h"
#include "ecmascript/stubs/runtime_stubs.h"
#include "ecmascript/subtyping_operator.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/tagged_node.h"
#include "ecmascript/ts_types/ts_manager.h"
#include "libpandafile/bytecode_instruction-inl.h"
#include "macros.h"
#ifdef ARK_SUPPORT_INTL
#include "ecmascript/js_collator.h"
#include "ecmascript/js_locale.h"
#else
#ifndef ARK_NOT_SUPPORT_INTL_GLOBAL
#include "ecmascript/intl/global_intl_helper.h"
#endif
#endif

namespace panda::ecmascript {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#define DEF_RUNTIME_STUBS(name) \
    JSTaggedType RuntimeStubs::name(uintptr_t argGlue, uint32_t argc, uintptr_t argv)

#define RUNTIME_STUBS_HEADER(name)                        \
    auto thread = JSThread::GlueToJSThread(argGlue);      \
    RUNTIME_TRACE(thread, name);                          \
    [[maybe_unused]] EcmaHandleScope handleScope(thread)  \

#define GET_ASM_FRAME(CurrentSp) \
    (reinterpret_cast<AsmInterpretedFrame *>(CurrentSp) - 1) // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

DEF_RUNTIME_STUBS(AddElementInternal)
{
    RUNTIME_STUBS_HEADER(AddElementInternal);
    JSHandle<JSObject> receiver = GetHArg<JSObject>(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue argIndex = GetArg(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    JSTaggedValue argAttr = GetArg(argv, argc, 3);  // 3: means the third parameter
    auto attr = static_cast<PropertyAttributes>(argAttr.GetInt());
    auto result = JSObject::AddElementInternal(thread, receiver, argIndex.GetInt(), value, attr);
    return JSTaggedValue(result).GetRawData();
}

DEF_RUNTIME_STUBS(AllocateInYoung)
{
    RUNTIME_STUBS_HEADER(AllocateInYoung);
    JSTaggedValue allocateSize = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    auto size = static_cast<size_t>(allocateSize.GetInt());
    auto heap = const_cast<Heap*>(thread->GetEcmaVM()->GetHeap());
    auto space = heap->GetNewSpace();
    ASSERT(size <= MAX_REGULAR_HEAP_OBJECT_SIZE);
    auto result = reinterpret_cast<TaggedObject *>(space->Allocate(size));
    if (result == nullptr) {
        result = heap->AllocateYoungOrHugeObject(size);
        ASSERT(result != nullptr);
    }
    return JSTaggedValue(result).GetRawData();
}

DEF_RUNTIME_STUBS(CallInternalGetter)
{
    RUNTIME_STUBS_HEADER(CallInternalGetter);
    JSTaggedType argAccessor = GetTArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSObject> argReceiver = GetHArg<JSObject>(argv, argc, 1);  // 1: means the first parameter

    auto accessor = AccessorData::Cast(reinterpret_cast<TaggedObject *>(argAccessor));
    return accessor->CallInternalGet(thread, argReceiver).GetRawData();
}

DEF_RUNTIME_STUBS(CallInternalSetter)
{
    RUNTIME_STUBS_HEADER(CallInternalSetter);
    JSHandle<JSObject> receiver = GetHArg<JSObject>(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedType argSetter = GetTArg(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 2);
    auto setter = AccessorData::Cast((reinterpret_cast<TaggedObject *>(argSetter)));
    auto result = setter->CallInternalSet(thread, receiver, value, true);
    if (!result) {
        return JSTaggedValue::Exception().GetRawData();
    }
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(Dump)
{
    RUNTIME_STUBS_HEADER(Dump);
    JSTaggedValue value = GetArg(argv, argc, 0);
    value.D();
    std::cout << "======================================================" << std::endl;
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(GetHash32)
{
    JSTaggedValue argKey = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue len = GetArg(argv, argc, 1);  // 1: means the first parameter
    int key = argKey.GetInt();
    auto pkey = reinterpret_cast<uint8_t *>(&key);
    uint32_t result = panda::GetHash32(pkey, len.GetInt());
    return JSTaggedValue(static_cast<uint64_t>(result)).GetRawData();
}

DEF_RUNTIME_STUBS(ComputeHashcode)
{
    JSTaggedType ecmaString = GetTArg(argv, argc, 0);  // 0: means the zeroth parameter
    auto string = reinterpret_cast<EcmaString *>(ecmaString);
    uint32_t result = EcmaStringAccessor(string).ComputeHashcode(0);
    return JSTaggedValue(static_cast<uint64_t>(result)).GetRawData();
}

void RuntimeStubs::PrintHeapReginInfo(uintptr_t argGlue)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    thread->GetEcmaVM()->GetHeap()->GetNewSpace()->EnumerateRegions([](Region *current) {
        LOG_ECMA(INFO) << "semispace region: " << current << std::endl;
    });
    thread->GetEcmaVM()->GetHeap()->GetOldSpace()->EnumerateRegions([](Region *current) {
        LOG_ECMA(INFO) << "GetOldSpace region: " << current << std::endl;
    });
    thread->GetEcmaVM()->GetHeap()->GetNonMovableSpace()->EnumerateRegions([](Region *current) {
        LOG_ECMA(INFO) << "GetNonMovableSpace region: " << current << std::endl;
    });
    thread->GetEcmaVM()->GetHeap()->GetMachineCodeSpace()->EnumerateRegions([](Region *current) {
        LOG_ECMA(INFO) << "GetMachineCodeSpace region: " << current << std::endl;
    });
}

DEF_RUNTIME_STUBS(GetTaggedArrayPtrTest)
{
    RUNTIME_STUBS_HEADER(GetTaggedArrayPtrTest);
    // this case static static JSHandle<TaggedArray> arr don't free in first call
    // second call trigger gc.
    // don't call EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSTaggedType array = GetTArg(argv, argc, 0);  // 0: means the zeroth parameter
    bool allocated = false;
    if (array == JSTaggedValue::VALUE_UNDEFINED) {
        // 2 : means construct 2 elements size taggedArray
        JSHandle<TaggedArray> arr = factory->NewTaggedArray(2);
        arr->Set(thread, 0, JSTaggedValue(3.5)); // 3.5: first element
        arr->Set(thread, 1, JSTaggedValue(4.5)); // 4.5: second element
        array = arr.GetTaggedValue().GetRawData();
        allocated = true;
    }
    JSHandle<TaggedArray> arr1(thread, JSTaggedValue(array));
#ifndef NDEBUG
    PrintHeapReginInfo(argGlue);
#endif
    if (!allocated) {
        thread->GetEcmaVM()->CollectGarbage(TriggerGCType::FULL_GC);
    }
    LOG_ECMA(INFO) << " arr->GetData() " << std::hex << "  " << arr1->GetData();
    return arr1.GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(NewInternalString)
{
    RUNTIME_STUBS_HEADER(NewInternalString);
    JSHandle<JSTaggedValue> keyHandle = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return JSTaggedValue(thread->GetEcmaVM()->GetFactory()->InternString(keyHandle)).GetRawData();
}

DEF_RUNTIME_STUBS(NewTaggedArray)
{
    RUNTIME_STUBS_HEADER(NewTaggedArray);
    JSTaggedValue length = GetArg(argv, argc, 0);  // 0: means the zeroth parameter

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->NewTaggedArray(length.GetInt()).GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(CopyArray)
{
    RUNTIME_STUBS_HEADER(CopyArray);
    JSHandle<TaggedArray> array = GetHArg<TaggedArray>(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue length = GetArg(argv, argc, 1);  // 1: means the first parameter
    JSTaggedValue capacity = GetArg(argv, argc, 2);  // 2: means the second parameter

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->CopyArray(array, length.GetInt(), capacity.GetInt()).GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(NameDictPutIfAbsent)
{
    RUNTIME_STUBS_HEADER(NameDictPutIfAbsent);
    JSTaggedType receiver = GetTArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedType array = GetTArg(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> keyHandle = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    JSHandle<JSTaggedValue> valueHandle = GetHArg<JSTaggedValue>(argv, argc, 3);  // 3: means the third parameter
    JSTaggedValue attr = GetArg(argv, argc, 4);   // 4: means the fourth parameter
    JSTaggedValue needTransToDict = GetArg(argv, argc, 5);  // 5: means the fifth parameter

    PropertyAttributes propAttr(attr.GetInt());
    if (needTransToDict.IsTrue()) {
        JSHandle<JSObject> objHandle(thread, JSTaggedValue(reinterpret_cast<TaggedObject *>(receiver)));
        JSHandle<NameDictionary> dictHandle(JSObject::TransitionToDictionary(thread, objHandle));
        return NameDictionary::
            PutIfAbsent(thread, dictHandle, keyHandle, valueHandle, propAttr).GetTaggedValue().GetRawData();
    } else {
        JSHandle<NameDictionary> dictHandle(thread, JSTaggedValue(reinterpret_cast<TaggedObject *>(array)));
        return NameDictionary::
            PutIfAbsent(thread, dictHandle, keyHandle, valueHandle, propAttr).GetTaggedValue().GetRawData();
    }
}

DEF_RUNTIME_STUBS(PropertiesSetValue)
{
    RUNTIME_STUBS_HEADER(PropertiesSetValue);
    JSHandle<JSObject> objHandle = GetHArg<JSObject>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> valueHandle = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<TaggedArray> arrayHandle = GetHArg<TaggedArray>(argv, argc, 2);   // 2: means the second parameter
    JSTaggedValue taggedCapacity = GetArg(argv, argc, 3);
    JSTaggedValue taggedIndex = GetArg(argv, argc, 4);
    int capacity = taggedCapacity.GetInt();
    int index = taggedIndex.GetInt();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> properties;
    if (capacity == 0) {
        properties = factory->NewTaggedArray(JSObject::MIN_PROPERTIES_LENGTH);
    } else {
        properties = factory->CopyArray(arrayHandle, capacity, JSObject::ComputePropertyCapacity(capacity));
    }
    properties->Set(thread, index, valueHandle);
    objHandle->SetProperties(thread, properties);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(TaggedArraySetValue)
{
    RUNTIME_STUBS_HEADER(TaggedArraySetValue);
    JSTaggedType argReceiver = GetTArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue value = GetArg(argv, argc, 1);  // 1: means the first parameter
    JSTaggedType argElement = GetTArg(argv, argc, 2);  // 2: means the second parameter
    JSTaggedValue taggedElementIndex = GetArg(argv, argc, 3);  // 3: means the third parameter
    JSTaggedValue taggedCapacity = GetArg(argv, argc, 4);  // 4: means the fourth parameter

    int elementIndex = taggedElementIndex.GetInt();
    int capacity = taggedCapacity.GetInt();
    auto elements = reinterpret_cast<TaggedArray *>(argElement);
    if (elementIndex >= capacity) {
        if (JSObject::ShouldTransToDict(capacity, elementIndex)) {
            return JSTaggedValue::Hole().GetRawData();
        }
        JSHandle<JSObject> receiverHandle(thread, reinterpret_cast<JSObject *>(argReceiver));
        JSHandle<JSTaggedValue> valueHandle(thread, value);
        elements = *JSObject::GrowElementsCapacity(thread, receiverHandle, elementIndex + 1);
        receiverHandle->SetElements(thread, JSTaggedValue(elements));
        elements->Set(thread, elementIndex, valueHandle);
        return JSTaggedValue::Undefined().GetRawData();
    }
    elements->Set(thread, elementIndex, value);
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(CheckAndCopyArray)
{
    RUNTIME_STUBS_HEADER(CheckAndCopyArray);
    JSTaggedType argReceiver = GetTArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSArray> receiverHandle(thread, reinterpret_cast<JSArray *>(argReceiver));
    JSArray::CheckAndCopyArray(thread, receiverHandle);
    return receiverHandle->GetElements().GetRawData();
}

DEF_RUNTIME_STUBS(NewEcmaHClass)
{
    RUNTIME_STUBS_HEADER(NewEcmaHClass);
    JSTaggedValue size = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue type = GetArg(argv, argc, 1);  // 1: means the first parameter
    JSTaggedValue inlinedProps = GetArg(argv, argc, 2);  // 2: means the second parameter
    return (thread->GetEcmaVM()->GetFactory()->NewEcmaHClass(
        size.GetInt(), JSType(type.GetInt()), inlinedProps.GetInt())).GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(UpdateLayOutAndAddTransition)
{
    RUNTIME_STUBS_HEADER(UpdateLayOutAndAddTransition);
    JSHandle<JSHClass> oldHClassHandle = GetHArg<JSHClass>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSHClass> newHClassHandle = GetHArg<JSHClass>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> keyHandle = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    JSTaggedValue attr = GetArg(argv, argc, 3);  // 3: means the third parameter

    auto factory = thread->GetEcmaVM()->GetFactory();
    PropertyAttributes attrValue(attr.GetInt());
    uint32_t offset = attrValue.GetOffset();
    newHClassHandle->IncNumberOfProps();

    {
        JSMutableHandle<LayoutInfo> layoutInfoHandle(thread, newHClassHandle->GetLayout());

        if (layoutInfoHandle->NumberOfElements() != static_cast<int>(offset)) {
            layoutInfoHandle.Update(factory->CopyAndReSort(layoutInfoHandle, offset, offset + 1));
            newHClassHandle->SetLayout(thread, layoutInfoHandle);
        } else if (layoutInfoHandle->GetPropertiesCapacity() <= static_cast<int>(offset)) {  // need to Grow
            layoutInfoHandle.Update(
                factory->ExtendLayoutInfo(layoutInfoHandle, offset));
            newHClassHandle->SetLayout(thread, layoutInfoHandle);
        }
        layoutInfoHandle->AddKey(thread, offset, keyHandle.GetTaggedValue(), attrValue);
    }

    // 5. Add newClass to old hclass's transitions.
    JSHClass::AddTransitions(thread, oldHClassHandle, newHClassHandle, keyHandle, attrValue);

    if (oldHClassHandle->HasTSSubtyping()) {
        SubtypingOperator::TryMaintainTSSubtyping(thread, oldHClassHandle, newHClassHandle, keyHandle);
    }
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(CopyAndUpdateObjLayout)
{
    RUNTIME_STUBS_HEADER(CopyAndUpdateObjLayout);
    JSHandle<JSHClass> newHClassHandle = GetHArg<JSHClass>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> keyHandle = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    JSTaggedValue attr = GetArg(argv, argc, 3);  // 3: means the third parameter

    auto factory = thread->GetEcmaVM()->GetFactory();
    PropertyAttributes attrValue(attr.GetInt());

    // 1. Copy
    JSHandle<LayoutInfo> oldLayout(thread, newHClassHandle->GetLayout());
    JSHandle<LayoutInfo> newLayout(factory->CopyLayoutInfo(oldLayout));
    newHClassHandle->SetLayout(thread, newLayout);

    // 2. Update attr
    auto hclass = JSHClass::Cast(newHClassHandle.GetTaggedValue().GetTaggedObject());
    int entry = JSHClass::FindPropertyEntry(thread, hclass, keyHandle.GetTaggedValue());
    ASSERT(entry != -1);
    newLayout->SetNormalAttr(thread, entry, attrValue);

    // 3. Maybe Transition And Maintain subtypeing check
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(UpdateHClassForElementsKind)
{
    RUNTIME_STUBS_HEADER(UpdateHClassForElementsKind);
    JSHandle<JSTaggedValue> receiver = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the first parameter
    JSTaggedType elementsKind = GetTArg(argv, argc, 1);        // 1: means the first parameter
    ElementsKind kind = Elements::FixElementsKind(static_cast<ElementsKind>(elementsKind));
    auto arrayIndexMap = thread->GetArrayHClassIndexMap();
    if (arrayIndexMap.find(kind) != arrayIndexMap.end()) {
        auto index = thread->GetArrayHClassIndexMap().at(kind);
        auto globalConst = thread->GlobalConstants();
        auto targetHClassValue = globalConst->GetGlobalConstantObject(static_cast<size_t>(index));
        auto hclass = JSHClass::Cast(targetHClassValue.GetTaggedObject());
        JSHandle<JSObject>(receiver)->SetClass(hclass);
    }
    return JSTaggedValue::Hole().GetRawData();
}

void RuntimeStubs::DebugPrint(int fmtMessageId, ...)
{
    std::string format = MessageString::GetMessageString(fmtMessageId);
    va_list args;
    va_start(args, fmtMessageId);
    std::string result = base::StringHelper::Vformat(format.c_str(), args);
    if (MessageString::IsBuiltinsStubMessageString(fmtMessageId)) {
        LOG_BUILTINS(ERROR) << result;
    } else {
        LOG_ECMA(ERROR) << result;
    }
    va_end(args);
}

void RuntimeStubs::DebugPrintCustom(uintptr_t fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::string result = base::StringHelper::Vformat(reinterpret_cast<const char*>(fmt), args);
    LOG_ECMA(ERROR) << result;
    va_end(args);
}

void RuntimeStubs::DebugPrintInstruction([[maybe_unused]] uintptr_t argGlue, const uint8_t *pc)
{
    BytecodeInstruction inst(pc);
    LOG_INTERPRETER(DEBUG) << inst;
}

void RuntimeStubs::Comment(uintptr_t argStr)
{
    std::string str(reinterpret_cast<char *>(argStr));
    LOG_ECMA(DEBUG) << str;
}

void RuntimeStubs::ProfileCall(uintptr_t argGlue, uintptr_t func, uintptr_t target, int32_t pcOffset, uint32_t incCount)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    thread->GetEcmaVM()->GetPGOProfiler()->ProfileCall(func, target, pcOffset, SampleMode::CALL_MODE, incCount);
}

void RuntimeStubs::ProfileOpType(uintptr_t argGlue, uintptr_t func, int32_t offset, int32_t type)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    thread->GetEcmaVM()->GetPGOProfiler()->ProfileOpType(func, offset, type);
}

void RuntimeStubs::ProfileDefineClass(uintptr_t argGlue, uintptr_t func, int32_t offset, uintptr_t ctor)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    thread->GetEcmaVM()->GetPGOProfiler()->ProfileDefineClass(thread, func, offset, ctor);
}

void RuntimeStubs::ProfileCreateObject(
    uintptr_t argGlue, JSTaggedType func, int32_t offset, JSTaggedType newObj, int32_t traceId)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    thread->GetEcmaVM()->GetPGOProfiler()->ProfileCreateObject(func, offset, newObj, traceId);
}

void RuntimeStubs::ProfileObjLayout(uintptr_t argGlue, uintptr_t func, int32_t offset, uintptr_t object, int32_t store)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    thread->GetEcmaVM()->GetPGOProfiler()->ProfileObjLayout(thread, func, offset, object, store != 0);
}

void RuntimeStubs::ProfileObjIndex(uintptr_t argGlue, uintptr_t func, int32_t offset, uintptr_t object)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    thread->GetEcmaVM()->GetPGOProfiler()->ProfileObjIndex(thread, func, offset, object);
}

void RuntimeStubs::FatalPrint(int fmtMessageId, ...)
{
    std::string format = MessageString::GetMessageString(fmtMessageId);
    va_list args;
    va_start(args, fmtMessageId);
    std::string result = base::StringHelper::Vformat(format.c_str(), args);
    LOG_FULL(FATAL) << result;
    va_end(args);
    LOG_ECMA(FATAL) << "this branch is unreachable";
    UNREACHABLE();
}

void RuntimeStubs::FatalPrintCustom(uintptr_t fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::string result = base::StringHelper::Vformat(reinterpret_cast<const char*>(fmt), args);
    LOG_FULL(FATAL) << result;
    va_end(args);
    LOG_ECMA(FATAL) << "this branch is unreachable";
    UNREACHABLE();
}

DEF_RUNTIME_STUBS(NoticeThroughChainAndRefreshUser)
{
    RUNTIME_STUBS_HEADER(NoticeThroughChainAndRefreshUser);
    JSHandle<JSHClass> oldHClassHandle = GetHArg<JSHClass>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSHClass> newHClassHandle = GetHArg<JSHClass>(argv, argc, 1);  // 1: means the first parameter

    JSHClass::NoticeThroughChain(thread, oldHClassHandle);
    JSHClass::RefreshUsers(thread, oldHClassHandle, newHClassHandle);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(Inc)
{
    RUNTIME_STUBS_HEADER(Inc);
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeInc(thread, value).GetRawData();
}

DEF_RUNTIME_STUBS(Dec)
{
    RUNTIME_STUBS_HEADER(Dec);
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeDec(thread, value).GetRawData();
}

DEF_RUNTIME_STUBS(CallGetPrototype)
{
    RUNTIME_STUBS_HEADER(CallGetPrototype);
    JSHandle<JSProxy> proxy = GetHArg<JSProxy>(argv, argc, 0);  // 0: means the zeroth parameter

    return JSProxy::GetPrototype(thread, proxy).GetRawData();
}

DEF_RUNTIME_STUBS(Exp)
{
    RUNTIME_STUBS_HEADER(Exp);
    JSTaggedValue baseValue = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue exponentValue = GetArg(argv, argc, 1);  // 1: means the first parameter

    if (baseValue.IsNumber() && exponentValue.IsNumber()) {
        // fast path
        double doubleBase = baseValue.IsInt() ? baseValue.GetInt() : baseValue.GetDouble();
        double doubleExponent = exponentValue.IsInt() ? exponentValue.GetInt() : exponentValue.GetDouble();
        if (std::abs(doubleBase) == 1 && std::isinf(doubleExponent)) {
            return JSTaggedValue(base::NAN_VALUE).GetRawData();
        }
        if ((doubleBase == 0 &&
            ((base::bit_cast<uint64_t>(doubleBase)) & base::DOUBLE_SIGN_MASK) == base::DOUBLE_SIGN_MASK) &&
            std::isfinite(doubleExponent) && base::NumberHelper::TruncateDouble(doubleExponent) == doubleExponent &&
            base::NumberHelper::TruncateDouble(doubleExponent / 2) + base::HALF ==  // 2 : half
            (doubleExponent / 2)) {  // 2 : half
            if (doubleExponent > 0) {
                return JSTaggedValue(-0.0).GetRawData();
            }
            if (doubleExponent < 0) {
                return JSTaggedValue(-base::POSITIVE_INFINITY).GetRawData();
            }
        }
        return JSTaggedValue(std::pow(doubleBase, doubleExponent)).GetRawData();
    }
    // Slow path
    JSTaggedValue res = RuntimeExp(thread, baseValue, exponentValue);
    return res.GetRawData();
}

DEF_RUNTIME_STUBS(IsIn)
{
    RUNTIME_STUBS_HEADER(IsIn);
    JSHandle<JSTaggedValue> prop = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeIsIn(thread, prop, obj).GetRawData();
}

DEF_RUNTIME_STUBS(InstanceOf)
{
    RUNTIME_STUBS_HEADER(InstanceOf);
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> target = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeInstanceof(thread, obj, target).GetRawData();
}

DEF_RUNTIME_STUBS(CreateGeneratorObj)
{
    RUNTIME_STUBS_HEADER(CreateGeneratorObj);
    JSHandle<JSTaggedValue> genFunc = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeCreateGeneratorObj(thread, genFunc).GetRawData();
}

DEF_RUNTIME_STUBS(CreateAsyncGeneratorObj)
{
    RUNTIME_STUBS_HEADER(CreateAsyncGeneratorObj);
    JSHandle<JSTaggedValue> genFunc = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeCreateAsyncGeneratorObj(thread, genFunc).GetRawData();
}

DEF_RUNTIME_STUBS(GetTemplateObject)
{
    RUNTIME_STUBS_HEADER(GetTemplateObject);
    JSHandle<JSTaggedValue> literal = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeGetTemplateObject(thread, literal).GetRawData();
}

DEF_RUNTIME_STUBS(GetNextPropName)
{
    RUNTIME_STUBS_HEADER(GetNextPropName);
    JSHandle<JSTaggedValue> iter = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeGetNextPropName(thread, iter).GetRawData();
}

DEF_RUNTIME_STUBS(IterNext)
{
    RUNTIME_STUBS_HEADER(IterNext);
    JSHandle<JSTaggedValue> iter = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeIterNext(thread, iter).GetRawData();
}

DEF_RUNTIME_STUBS(CloseIterator)
{
    RUNTIME_STUBS_HEADER(CloseIterator);
    JSHandle<JSTaggedValue> iter = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeCloseIterator(thread, iter).GetRawData();
}

DEF_RUNTIME_STUBS(SuperCallSpread)
{
    RUNTIME_STUBS_HEADER(SuperCallSpread);
    JSHandle<JSTaggedValue> func = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> array = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    auto sp = const_cast<JSTaggedType *>(thread->GetCurrentInterpretedFrame());
    JSTaggedValue function = InterpreterAssembly::GetNewTarget(sp);
    return RuntimeSuperCallSpread(thread, func, JSHandle<JSTaggedValue>(thread, function), array).GetRawData();
}

DEF_RUNTIME_STUBS(OptSuperCallSpread)
{
    RUNTIME_STUBS_HEADER(OptSuperCallSpread);
    JSHandle<JSTaggedValue> func = GetHArg<JSTaggedValue>(argv, argc, 0);
    JSHandle<JSTaggedValue> newTarget = GetHArg<JSTaggedValue>(argv, argc, 1);
    JSHandle<JSTaggedValue> array = GetHArg<JSTaggedValue>(argv, argc, 2);
    return RuntimeSuperCallSpread(thread, func, newTarget, array).GetRawData();
}

DEF_RUNTIME_STUBS(DelObjProp)
{
    RUNTIME_STUBS_HEADER(DelObjProp);
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> prop = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeDelObjProp(thread, obj, prop).GetRawData();
}

DEF_RUNTIME_STUBS(NewObjApply)
{
    RUNTIME_STUBS_HEADER(NewObjApply);
    JSHandle<JSTaggedValue> func = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> array = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeNewObjApply(thread, func, array).GetRawData();
}

DEF_RUNTIME_STUBS(CreateIterResultObj)
{
    RUNTIME_STUBS_HEADER(CreateIterResultObj);
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue flag = GetArg(argv, argc, 1);  // 1: means the first parameter
    return RuntimeCreateIterResultObj(thread, value, flag).GetRawData();
}

DEF_RUNTIME_STUBS(AsyncFunctionAwaitUncaught)
{
    RUNTIME_STUBS_HEADER(AsyncFunctionAwaitUncaught);
    JSHandle<JSTaggedValue> asyncFuncObj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeAsyncFunctionAwaitUncaught(thread, asyncFuncObj, value).GetRawData();
}

DEF_RUNTIME_STUBS(AsyncFunctionResolveOrReject)
{
    RUNTIME_STUBS_HEADER(AsyncFunctionResolveOrReject);
    JSHandle<JSTaggedValue> asyncFuncObj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSTaggedValue isResolve = GetArg(argv, argc, 2);  // 2: means the second parameter
    return RuntimeAsyncFunctionResolveOrReject(thread, asyncFuncObj, value, isResolve.IsTrue()).GetRawData();
}

DEF_RUNTIME_STUBS(AsyncGeneratorResolve)
{
    RUNTIME_STUBS_HEADER(AsyncGeneratorResolve);
    JSHandle<JSTaggedValue> asyncGenerator = GetHArg<JSTaggedValue>(argv, argc, 0); // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 1); // 1: means the first parameter
    JSTaggedValue flag = GetArg(argv, argc, 2); // 2: means the second parameter
    return RuntimeAsyncGeneratorResolve(thread, asyncGenerator, value, flag).GetRawData();
}

DEF_RUNTIME_STUBS(AsyncGeneratorReject)
{
    RUNTIME_STUBS_HEADER(AsyncGeneratorReject);
    JSHandle<JSTaggedValue> asyncGenerator = GetHArg<JSTaggedValue>(argv, argc, 0);
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 1);
    return RuntimeAsyncGeneratorReject(thread, asyncGenerator, value).GetRawData();
}

DEF_RUNTIME_STUBS(SetGeneratorState)
{
    RUNTIME_STUBS_HEADER(SetGeneratorState);
    JSHandle<JSTaggedValue> asyncGenerator = GetHArg<JSTaggedValue>(argv, argc, 0);
    JSTaggedValue index = GetArg(argv, argc, 1);
    RuntimeSetGeneratorState(thread, asyncGenerator, index.GetInt());
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(CopyDataProperties)
{
    RUNTIME_STUBS_HEADER(CopyDataProperties);
    JSHandle<JSTaggedValue> dst = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> src = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeCopyDataProperties(thread, dst, src).GetRawData();
}

DEF_RUNTIME_STUBS(StArraySpread)
{
    RUNTIME_STUBS_HEADER(StArraySpread);
    JSHandle<JSTaggedValue> dst = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue index = GetArg(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> src = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    return RuntimeStArraySpread(thread, dst, index, src).GetRawData();
}

DEF_RUNTIME_STUBS(GetIteratorNext)
{
    RUNTIME_STUBS_HEADER(GetIteratorNext);
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> method = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeGetIteratorNext(thread, obj, method).GetRawData();
}

DEF_RUNTIME_STUBS(SetObjectWithProto)
{
    RUNTIME_STUBS_HEADER(SetObjectWithProto);
    JSHandle<JSTaggedValue> proto = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSObject> obj = GetHArg<JSObject>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeSetObjectWithProto(thread, proto, obj).GetRawData();
}

DEF_RUNTIME_STUBS(LoadICByValue)
{
    RUNTIME_STUBS_HEADER(LoadICByValue);
    JSHandle<JSTaggedValue> profileTypeInfo = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> receiver = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> key = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    JSTaggedValue slotId = GetArg(argv, argc, 3);  // 3: means the third parameter

    if (profileTypeInfo->IsUndefined()) {
        return RuntimeLdObjByValue(thread, receiver, key, false, JSTaggedValue::Undefined()).GetRawData();
    }
    JSHandle<JSTaggedValue> propKey = JSTaggedValue::ToPropertyKey(thread, key);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
    LoadICRuntime icRuntime(thread, JSHandle<ProfileTypeInfo>::Cast(profileTypeInfo), slotId.GetInt(), ICKind::LoadIC);
    return icRuntime.LoadMiss(receiver, propKey).GetRawData();
}

DEF_RUNTIME_STUBS(StoreICByValue)
{
    RUNTIME_STUBS_HEADER(StoreICByValue);
    JSHandle<JSTaggedValue> profileTypeInfo = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> receiver = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> key = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 3);  // 3: means the third parameter
    JSTaggedValue slotId = GetArg(argv, argc, 4);   // 4: means the fourth parameter

    if (profileTypeInfo->IsUndefined()) {
        return RuntimeStObjByValue(thread, receiver, key, value).GetRawData();
    }
    JSHandle<JSTaggedValue> propKey = JSTaggedValue::ToPropertyKey(thread, key);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
    StoreICRuntime icRuntime(thread, JSHandle<ProfileTypeInfo>::Cast(profileTypeInfo), slotId.GetInt(),
                             ICKind::StoreIC);
    return icRuntime.StoreMiss(receiver, propKey, value).GetRawData();
}

DEF_RUNTIME_STUBS(StOwnByValue)
{
    RUNTIME_STUBS_HEADER(StOwnByValue);
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> key = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter

    return RuntimeStOwnByValue(thread, obj, key, value).GetRawData();
}

DEF_RUNTIME_STUBS(LdSuperByValue)
{
    RUNTIME_STUBS_HEADER(LdSuperByValue);
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> key = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    auto sp = const_cast<JSTaggedType *>(thread->GetCurrentInterpretedFrame());
    JSTaggedValue thisFunc = InterpreterAssembly::GetFunction(sp);
    return RuntimeLdSuperByValue(thread, obj, key, thisFunc).GetRawData();
}

DEF_RUNTIME_STUBS(OptLdSuperByValue)
{
    RUNTIME_STUBS_HEADER(OptLdSuperByValue);
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> key = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSTaggedValue thisFunc = GetArg(argv, argc, 2);  // 2: means the second parameter
    return RuntimeLdSuperByValue(thread, obj, key, thisFunc).GetRawData();
}

DEF_RUNTIME_STUBS(StSuperByValue)
{
    RUNTIME_STUBS_HEADER(StSuperByValue);
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> key = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    auto sp = const_cast<JSTaggedType *>(thread->GetCurrentInterpretedFrame());
    JSTaggedValue thisFunc = InterpreterAssembly::GetFunction(sp);
    return RuntimeStSuperByValue(thread, obj, key, value, thisFunc).GetRawData();
}

DEF_RUNTIME_STUBS(OptStSuperByValue)
{
    RUNTIME_STUBS_HEADER(OptStSuperByValue);
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> key = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    JSTaggedValue thisFunc = GetArg(argv, argc, 3);  // 3: means the third parameter
    return RuntimeStSuperByValue(thread, obj, key, value, thisFunc).GetRawData();
}

DEF_RUNTIME_STUBS(GetMethodFromCache)
{
    RUNTIME_STUBS_HEADER(GetMethodFromCache);
    JSHandle<JSTaggedValue> constpool = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue index = GetArg(argv, argc, 1);  // 1: means the first parameter
    return ConstantPool::GetMethodFromCache(
        thread, constpool.GetTaggedValue(), index.GetInt()).GetRawData();
}

DEF_RUNTIME_STUBS(GetStringFromCache)
{
    RUNTIME_STUBS_HEADER(GetStringFromCache);
    JSHandle<JSTaggedValue> constpool = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue index = GetArg(argv, argc, 1);  // 1: means the first parameter
    return ConstantPool::GetStringFromCache(
        thread, constpool.GetTaggedValue(), index.GetInt()).GetRawData();
}

DEF_RUNTIME_STUBS(GetObjectLiteralFromCache)
{
    RUNTIME_STUBS_HEADER(GetObjectLiteralFromCache);
    JSHandle<JSTaggedValue> constpool = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue index = GetArg(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> module = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    return ConstantPool::GetLiteralFromCache<ConstPoolType::OBJECT_LITERAL>(
        thread, constpool.GetTaggedValue(), index.GetInt(), module.GetTaggedValue()).GetRawData();
}

DEF_RUNTIME_STUBS(GetArrayLiteralFromCache)
{
    RUNTIME_STUBS_HEADER(GetArrayLiteralFromCache);
    JSHandle<JSTaggedValue> constpool = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue index = GetArg(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> module = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    return ConstantPool::GetLiteralFromCache<ConstPoolType::ARRAY_LITERAL>(
        thread, constpool.GetTaggedValue(), index.GetInt(), module.GetTaggedValue()).GetRawData();
}

DEF_RUNTIME_STUBS(LdObjByIndex)
{
    RUNTIME_STUBS_HEADER(LdObjByIndex);
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue idx = GetArg(argv, argc, 1);  // 1: means the first parameter
    JSTaggedValue callGetter = GetArg(argv, argc, 2);  // 2: means the second parameter
    JSTaggedValue receiver = GetArg(argv, argc, 3);  // 3: means the third parameter
    return RuntimeLdObjByIndex(thread, obj, idx.GetInt(), callGetter.IsTrue(), receiver).GetRawData();
}

DEF_RUNTIME_STUBS(StObjByIndex)
{
    RUNTIME_STUBS_HEADER(StObjByIndex);
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue idx = GetArg(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    return RuntimeStObjByIndex(thread, obj, idx.GetInt(), value).GetRawData();
}

DEF_RUNTIME_STUBS(StOwnByIndex)
{
    RUNTIME_STUBS_HEADER(StOwnByIndex);
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> idx = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    return RuntimeStOwnByIndex(thread, obj, idx, value).GetRawData();
}

DEF_RUNTIME_STUBS(StGlobalRecord)
{
    RUNTIME_STUBS_HEADER(StGlobalRecord);
    JSHandle<JSTaggedValue> prop = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSTaggedValue isConst = GetArg(argv, argc, 2);
    return RuntimeStGlobalRecord(thread, prop, value, isConst.IsTrue()).GetRawData();
}

DEF_RUNTIME_STUBS(Neg)
{
    RUNTIME_STUBS_HEADER(Neg);
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeNeg(thread, value).GetRawData();
}

DEF_RUNTIME_STUBS(Not)
{
    RUNTIME_STUBS_HEADER(Not);
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeNot(thread, value).GetRawData();
}

DEF_RUNTIME_STUBS(Shl2)
{
    RUNTIME_STUBS_HEADER(Shl2);
    JSTaggedValue left = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue right = GetArg(argv, argc, 1);  // 1: means the first parameter

    auto res = SlowRuntimeStub::Shl2(thread, left, right);
    return JSTaggedValue(res).GetRawData();
}

DEF_RUNTIME_STUBS(Shr2)
{
    RUNTIME_STUBS_HEADER(Shr2);
    JSTaggedValue left = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue right = GetArg(argv, argc, 1);  // 1: means the first parameter

    auto res = SlowRuntimeStub::Shr2(thread, left, right);
    return JSTaggedValue(res).GetRawData();
}

DEF_RUNTIME_STUBS(Ashr2)
{
    RUNTIME_STUBS_HEADER(Ashr2);
    JSTaggedValue left = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue right = GetArg(argv, argc, 1);  // 1: means the first parameter

    auto res = SlowRuntimeStub::Ashr2(thread, left, right);
    return JSTaggedValue(res).GetRawData();
}

DEF_RUNTIME_STUBS(And2)
{
    RUNTIME_STUBS_HEADER(And2);
    JSTaggedValue left = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue right = GetArg(argv, argc, 1);  // 1: means the first parameter

    auto res = SlowRuntimeStub::And2(thread, left, right);
    return JSTaggedValue(res).GetRawData();
}

DEF_RUNTIME_STUBS(Xor2)
{
    RUNTIME_STUBS_HEADER(Xor2);
    JSTaggedValue left = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue right = GetArg(argv, argc, 1);  // 1: means the first parameter

    auto res = SlowRuntimeStub::Xor2(thread, left, right);
    return JSTaggedValue(res).GetRawData();
}

DEF_RUNTIME_STUBS(Or2)
{
    RUNTIME_STUBS_HEADER(Or2);
    JSTaggedValue left = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue right = GetArg(argv, argc, 1);  // 1: means the first parameter

    auto res = SlowRuntimeStub::Or2(thread, left, right);
    return JSTaggedValue(res).GetRawData();
}

DEF_RUNTIME_STUBS(CreateClassWithBuffer)
{
    RUNTIME_STUBS_HEADER(CreateClassWithBuffer);
    JSHandle<JSTaggedValue> base = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> lexenv = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> constpool = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    JSTaggedValue methodId = GetArg(argv, argc, 3);  // 3: means the third parameter
    JSTaggedValue literalId = GetArg(argv, argc, 4);  // 4: means the four parameter
    JSHandle<JSTaggedValue> module = GetHArg<JSTaggedValue>(argv, argc, 5);  // 5: means the fifth parameter
    return RuntimeCreateClassWithBuffer(thread, base, lexenv, constpool,
                                        static_cast<uint16_t>(methodId.GetInt()),
                                        static_cast<uint16_t>(literalId.GetInt()), module).GetRawData();
}

DEF_RUNTIME_STUBS(SetClassConstructorLength)
{
    RUNTIME_STUBS_HEADER(SetClassConstructorLength);
    JSTaggedValue ctor = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue length = GetArg(argv, argc, 1);  // 1: means the first parameter
    return RuntimeSetClassConstructorLength(thread, ctor, length).GetRawData();
}

DEF_RUNTIME_STUBS(UpdateHotnessCounter)
{
    RUNTIME_STUBS_HEADER(UpdateHotnessCounter);
    JSHandle<JSFunction> thisFunc = GetHArg<JSFunction>(argv, argc, 0);  // 0: means the zeroth parameter
    thread->CheckSafepoint();
    JSHandle<Method> method(thread, thisFunc->GetMethod());
    auto profileTypeInfo = method->GetProfileTypeInfo();
    if (profileTypeInfo.IsUndefined()) {
        uint32_t slotSize = method->GetSlotSize();
        auto res = RuntimeNotifyInlineCache(thread, method, slotSize);
        return res.GetRawData();
    }
    return profileTypeInfo.GetRawData();
}

DEF_RUNTIME_STUBS(UpdateHotnessCounterWithProf)
{
    RUNTIME_STUBS_HEADER(UpdateHotnessCounterWithProf);
    JSHandle<JSFunction> thisFunc = GetHArg<JSFunction>(argv, argc, 0);  // 0: means the zeroth parameter
    thread->CheckSafepoint();
    JSHandle<Method> method(thread, thisFunc->GetMethod());
    auto profileTypeInfo = method->GetProfileTypeInfo();
    if (profileTypeInfo.IsUndefined()) {
        thread->GetEcmaVM()->GetPGOProfiler()->ProfileCall(
            JSTaggedValue::VALUE_UNDEFINED, thisFunc.GetTaggedType(), -1, SampleMode::HOTNESS_MODE);
        uint32_t slotSize = method->GetSlotSize();
        auto res = RuntimeNotifyInlineCache(thread, method, slotSize);
        return res.GetRawData();
    }
    return profileTypeInfo.GetRawData();
}

DEF_RUNTIME_STUBS(CheckSafePointAndStackOverflow)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    if (thread->HasCheckSafePoint()) {
        thread->CheckSafepoint();
        return JSTaggedValue::Undefined().GetRawData();
    }
    return RuntimeThrowStackOverflowException(thread).GetRawData();
}

DEF_RUNTIME_STUBS(LoadICByName)
{
    RUNTIME_STUBS_HEADER(LoadICByName);
    JSHandle<JSTaggedValue> profileHandle = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> receiverHandle = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> keyHandle = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    JSTaggedValue slotId = GetArg(argv, argc, 3);  // 3: means the third parameter

    if (profileHandle->IsUndefined()) {
        auto res = JSTaggedValue::GetProperty(thread, receiverHandle, keyHandle).GetValue().GetTaggedValue();
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
        return res.GetRawData();
    }
    LoadICRuntime icRuntime(
        thread, JSHandle<ProfileTypeInfo>::Cast(profileHandle), slotId.GetInt(), ICKind::NamedLoadIC);
    return icRuntime.LoadMiss(receiverHandle, keyHandle).GetRawData();
}

DEF_RUNTIME_STUBS(TryLdGlobalICByName)
{
    RUNTIME_STUBS_HEADER(TryLdGlobalICByName);
    JSHandle<JSTaggedValue> profileHandle = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> keyHandle = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSTaggedValue slotId = GetArg(argv, argc, 2);  // 2: means the third parameter

    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    JSHandle<JSTaggedValue> globalObj(thread, globalEnv->GetGlobalObject());
    if (profileHandle->IsUndefined()) {
        auto res = RuntimeTryLdGlobalByName(thread, globalObj, keyHandle);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
        return res.GetRawData();
    }
    LoadICRuntime icRuntime(
        thread, JSHandle<ProfileTypeInfo>::Cast(profileHandle), slotId.GetInt(), ICKind::NamedGlobalTryLoadIC);
    return icRuntime.LoadMiss(globalObj, keyHandle).GetRawData();
}

DEF_RUNTIME_STUBS(StoreICByName)
{
    RUNTIME_STUBS_HEADER(StoreICByName);
    JSHandle<JSTaggedValue> profileHandle = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> receiverHandle = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> keyHandle = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    JSHandle<JSTaggedValue> valueHandle = GetHArg<JSTaggedValue>(argv, argc, 3);  // 3: means the third parameter
    JSTaggedValue slotId = GetArg(argv, argc, 4);   // 4: means the fourth parameter

    if (profileHandle->IsUndefined()) {
        JSTaggedValue::SetProperty(thread, receiverHandle, keyHandle, valueHandle, true);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
        return JSTaggedValue::True().GetRawData();
    }
    StoreICRuntime icRuntime(
        thread, JSHandle<ProfileTypeInfo>::Cast(profileHandle), slotId.GetInt(), ICKind::NamedStoreIC);
    return icRuntime.StoreMiss(receiverHandle, keyHandle, valueHandle).GetRawData();
}

DEF_RUNTIME_STUBS(SetFunctionNameNoPrefix)
{
    RUNTIME_STUBS_HEADER(SetFunctionNameNoPrefix);
    JSTaggedType argFunc = GetTArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue argName = GetArg(argv, argc, 1);  // 1: means the first parameter
    JSFunction::SetFunctionNameNoPrefix(thread, reinterpret_cast<JSFunction *>(argFunc), argName);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(StOwnByValueWithNameSet)
{
    RUNTIME_STUBS_HEADER(StOwnByValueWithNameSet);
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> prop = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    return RuntimeStOwnByValueWithNameSet(thread, obj, prop, value).GetRawData();
}

DEF_RUNTIME_STUBS(StOwnByName)
{
    RUNTIME_STUBS_HEADER(StOwnByName);
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> prop = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    return RuntimeStOwnByName(thread, obj, prop, value).GetRawData();
}

DEF_RUNTIME_STUBS(StOwnByNameWithNameSet)
{
    RUNTIME_STUBS_HEADER(StOwnByNameWithNameSet);
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> prop = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    return RuntimeStOwnByValueWithNameSet(thread, obj, prop, value).GetRawData();
}

DEF_RUNTIME_STUBS(SuspendGenerator)
{
    RUNTIME_STUBS_HEADER(SuspendGenerator);
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeSuspendGenerator(thread, obj, value).GetRawData();
}

DEF_RUNTIME_STUBS(OptSuspendGenerator)
{
    RUNTIME_STUBS_HEADER(OptSuspendGenerator);
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeOptSuspendGenerator(thread, obj, value).GetRawData();
}

DEF_RUNTIME_STUBS(OptAsyncGeneratorResolve)
{
    RUNTIME_STUBS_HEADER(OptAsyncGeneratorResolve);
    JSHandle<JSTaggedValue> asyncGenerator = GetHArg<JSTaggedValue>(argv, argc, 0); // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 1); // 1: means the first parameter
    JSTaggedValue flag = GetArg(argv, argc, 2); // 2: means the second parameter
    return RuntimeOptAsyncGeneratorResolve(thread, asyncGenerator, value, flag).GetRawData();
}

DEF_RUNTIME_STUBS(OptCreateObjectWithExcludedKeys)
{
    RUNTIME_STUBS_HEADER(OptCreateObjectWithExcludedKeys);
    JSTaggedValue numKeys = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> objVal = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSTaggedValue firstArgRegIdx = GetArg(argv, argc, 2);  // 2: means the second parameter
    return RuntimeOptCreateObjectWithExcludedKeys(thread, static_cast<uint16_t>(numKeys.GetInt()), objVal,
        static_cast<uint16_t>(firstArgRegIdx.GetInt()), argv, argc).GetRawData();
}

DEF_RUNTIME_STUBS(UpFrame)
{
    RUNTIME_STUBS_HEADER(UpFrame);
    FrameHandler frameHandler(thread);
    uint32_t pcOffset = panda_file::INVALID_OFFSET;
    for (; frameHandler.HasFrame(); frameHandler.PrevJSFrame()) {
        if (frameHandler.IsEntryFrame() || frameHandler.IsBuiltinFrame()) {
            thread->SetCurrentFrame(frameHandler.GetSp());
            thread->SetLastFp(frameHandler.GetFp());
            return JSTaggedValue(static_cast<uint64_t>(0)).GetRawData();
        }
        auto method = frameHandler.GetMethod();
        pcOffset = method->FindCatchBlock(frameHandler.GetBytecodeOffset());
        if (pcOffset != INVALID_INDEX) {
            thread->SetCurrentFrame(frameHandler.GetSp());
            thread->SetLastFp(frameHandler.GetFp());
            uintptr_t pc = reinterpret_cast<uintptr_t>(method->GetBytecodeArray() + pcOffset);
            return JSTaggedValue(static_cast<uint64_t>(pc)).GetRawData();
        }
        if (!method->IsNativeWithCallField()) {
            auto *debuggerMgr = thread->GetEcmaVM()->GetJsDebuggerManager();
            debuggerMgr->GetNotificationManager()->MethodExitEvent(thread, method);
        }
    }
    LOG_FULL(FATAL) << "EXCEPTION: EntryFrame Not Found";
    UNREACHABLE();
}

DEF_RUNTIME_STUBS(GetModuleNamespaceByIndex)
{
    RUNTIME_STUBS_HEADER(GetModuleNamespaceByIndex);
    JSTaggedValue index = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeGetModuleNamespace(thread, index.GetInt()).GetRawData();
}

DEF_RUNTIME_STUBS(GetModuleNamespaceByIndexOnJSFunc)
{
    RUNTIME_STUBS_HEADER(GetModuleNamespaceByIndexOnJSFunc);
    JSTaggedValue index = GetArg(argv, argc, 0);
    JSTaggedValue jsFunc = GetArg(argv, argc, 1);
    return RuntimeGetModuleNamespace(thread, index.GetInt(), jsFunc).GetRawData();
}

DEF_RUNTIME_STUBS(GetModuleNamespace)
{
    RUNTIME_STUBS_HEADER(GetModuleNamespace);
    JSTaggedValue localName = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeGetModuleNamespace(thread, localName).GetRawData();
}

DEF_RUNTIME_STUBS(StModuleVarByIndex)
{
    RUNTIME_STUBS_HEADER(StModuleVar);
    JSTaggedValue index = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue value = GetArg(argv, argc, 1);  // 1: means the first parameter
    RuntimeStModuleVar(thread, index.GetInt(), value);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(StModuleVarByIndexOnJSFunc)
{
    RUNTIME_STUBS_HEADER(StModuleVarByIndexOnJSFunc);
    JSTaggedValue index = GetArg(argv, argc, 0);
    JSTaggedValue value = GetArg(argv, argc, 1);
    JSTaggedValue jsFunc = GetArg(argv, argc, 2);
    RuntimeStModuleVar(thread, index.GetInt(), value, jsFunc);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(StModuleVar)
{
    RUNTIME_STUBS_HEADER(StModuleVar);
    JSTaggedValue key = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue value = GetArg(argv, argc, 1);  // 1: means the first parameter
    RuntimeStModuleVar(thread, key, value);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(LdLocalModuleVarByIndex)
{
    RUNTIME_STUBS_HEADER(LdLocalModuleVarByIndex);
    JSTaggedValue index = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeLdLocalModuleVar(thread, index.GetInt()).GetRawData();
}

DEF_RUNTIME_STUBS(LdExternalModuleVarByIndex)
{
    RUNTIME_STUBS_HEADER(LdExternalModuleVarByIndex);
    JSTaggedValue index = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeLdExternalModuleVar(thread, index.GetInt()).GetRawData();
}

DEF_RUNTIME_STUBS(LdLocalModuleVarByIndexOnJSFunc)
{
    RUNTIME_STUBS_HEADER(LdLocalModuleVarByIndexOnJSFunc);
    JSTaggedValue index = GetArg(argv, argc, 0);
    JSTaggedValue jsFunc = GetArg(argv, argc, 1);
    return RuntimeLdLocalModuleVar(thread, index.GetInt(), jsFunc).GetRawData();
}

DEF_RUNTIME_STUBS(LdExternalModuleVarByIndexOnJSFunc)
{
    RUNTIME_STUBS_HEADER(LdExternalModuleVarByIndexOnJSFunc);
    JSTaggedValue index = GetArg(argv, argc, 0);
    JSTaggedValue jsFunc = GetArg(argv, argc, 1);
    return RuntimeLdExternalModuleVar(thread, index.GetInt(), jsFunc).GetRawData();
}

DEF_RUNTIME_STUBS(LdModuleVar)
{
    RUNTIME_STUBS_HEADER(LdModuleVar);
    JSTaggedValue key = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue taggedFlag = GetArg(argv, argc, 1);  // 1: means the first parameter
    bool innerFlag = taggedFlag.GetInt() != 0;
    return RuntimeLdModuleVar(thread, key, innerFlag).GetRawData();
}

DEF_RUNTIME_STUBS(GetPropIterator)
{
    RUNTIME_STUBS_HEADER(GetPropIterator);
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeGetPropIterator(thread, value).GetRawData();
}

DEF_RUNTIME_STUBS(AsyncFunctionEnter)
{
    RUNTIME_STUBS_HEADER(AsyncFunctionEnter);
    return RuntimeAsyncFunctionEnter(thread).GetRawData();
}

DEF_RUNTIME_STUBS(GetIterator)
{
    RUNTIME_STUBS_HEADER(GetIterator);
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeGetIterator(thread, obj).GetRawData();
}

DEF_RUNTIME_STUBS(GetAsyncIterator)
{
    RUNTIME_STUBS_HEADER(GetAsyncIterator);
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeGetAsyncIterator(thread, obj).GetRawData();
}

DEF_RUNTIME_STUBS(Throw)
{
    RUNTIME_STUBS_HEADER(Throw);
    JSTaggedValue value = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    RuntimeThrow(thread, value);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(ThrowThrowNotExists)
{
    RUNTIME_STUBS_HEADER(ThrowThrowNotExists);
    RuntimeThrowThrowNotExists(thread);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(ThrowPatternNonCoercible)
{
    RUNTIME_STUBS_HEADER(ThrowPatternNonCoercible);
    RuntimeThrowPatternNonCoercible(thread);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(ThrowDeleteSuperProperty)
{
    RUNTIME_STUBS_HEADER(ThrowDeleteSuperProperty);
    RuntimeThrowDeleteSuperProperty(thread);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(ThrowUndefinedIfHole)
{
    RUNTIME_STUBS_HEADER(ThrowUndefinedIfHole);
    JSHandle<EcmaString> obj = GetHArg<EcmaString>(argv, argc, 0);  // 0: means the zeroth parameter
    RuntimeThrowUndefinedIfHole(thread, obj);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(ThrowIfNotObject)
{
    RUNTIME_STUBS_HEADER(ThrowIfNotObject);
    RuntimeThrowIfNotObject(thread);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(ThrowConstAssignment)
{
    RUNTIME_STUBS_HEADER(ThrowConstAssignment);
    JSHandle<EcmaString> value = GetHArg<EcmaString>(argv, argc, 0);  // 0: means the zeroth parameter
    RuntimeThrowConstAssignment(thread, value);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(ThrowTypeError)
{
    RUNTIME_STUBS_HEADER(ThrowTypeError);
    JSTaggedValue argMessageStringId = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    std::string message = MessageString::GetMessageString(argMessageStringId.GetInt());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR, message.c_str());
    THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error.GetTaggedValue(), JSTaggedValue::Hole().GetRawData());
}

DEF_RUNTIME_STUBS(LoadMiss)
{
    RUNTIME_STUBS_HEADER(LoadMiss);
    JSTaggedType profileTypeInfo = GetTArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue receiver = GetArg(argv, argc, 1);  // 1: means the first parameter
    JSTaggedValue key = GetArg(argv, argc, 2);  // 2: means the second parameter
    JSTaggedValue slotId = GetArg(argv, argc, 3);  // 3: means the third parameter
    JSTaggedValue kind = GetArg(argv, argc, 4);   // 4: means the fourth parameter
    return ICRuntimeStub::LoadMiss(thread, reinterpret_cast<ProfileTypeInfo *>(profileTypeInfo), receiver, key,
        slotId.GetInt(), static_cast<ICKind>(kind.GetInt())).GetRawData();
}

DEF_RUNTIME_STUBS(StoreMiss)
{
    RUNTIME_STUBS_HEADER(StoreMiss);
    JSTaggedType profileTypeInfo = GetTArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue receiver = GetArg(argv, argc, 1);  // 1: means the first parameter
    JSTaggedValue key = GetArg(argv, argc, 2);  // 2: means the second parameter
    JSTaggedValue value = GetArg(argv, argc, 3);  // 3: means the third parameter
    JSTaggedValue slotId = GetArg(argv, argc, 4);  // 4: means the fourth parameter
    JSTaggedValue kind = GetArg(argv, argc, 5);  // 5: means the fifth parameter
    return ICRuntimeStub::StoreMiss(thread, reinterpret_cast<ProfileTypeInfo *>(profileTypeInfo), receiver, key, value,
        slotId.GetInt(), static_cast<ICKind>(kind.GetInt())).GetRawData();
}

DEF_RUNTIME_STUBS(TryUpdateGlobalRecord)
{
    RUNTIME_STUBS_HEADER(TryUpdateGlobalRecord);
    JSTaggedValue prop = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue value = GetArg(argv, argc, 1);  // 1: means the first parameter
    return RuntimeTryUpdateGlobalRecord(thread, prop, value).GetRawData();
}

DEF_RUNTIME_STUBS(ThrowReferenceError)
{
    RUNTIME_STUBS_HEADER(ThrowReferenceError);
    JSHandle<JSTaggedValue> prop = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeThrowReferenceError(thread, prop, " is not defined").GetRawData();
}

DEF_RUNTIME_STUBS(LdGlobalICVar)
{
    RUNTIME_STUBS_HEADER(LdGlobalICVar);
    JSHandle<JSTaggedValue> global = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> prop = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> profileHandle = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    JSTaggedValue slotId = GetArg(argv, argc, 3);  // 3: means the third parameter

    if (profileHandle->IsUndefined()) {
        return RuntimeLdGlobalVarFromProto(thread, global, prop).GetRawData();
    }
    LoadICRuntime icRuntime(
        thread, JSHandle<ProfileTypeInfo>::Cast(profileHandle), slotId.GetInt(), ICKind::NamedGlobalLoadIC);
    return icRuntime.LoadMiss(global, prop).GetRawData();
}

DEF_RUNTIME_STUBS(StGlobalVar)
{
    RUNTIME_STUBS_HEADER(StGlobalVar);
    JSHandle<JSTaggedValue> prop = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeStGlobalVar(thread, prop, value).GetRawData();
}

DEF_RUNTIME_STUBS(ToNumber)
{
    RUNTIME_STUBS_HEADER(ToNumber);
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeToNumber(thread, value).GetRawData();
}

DEF_RUNTIME_STUBS(ToBoolean)
{
    RUNTIME_STUBS_HEADER(ToBoolean);
    JSTaggedValue value = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    bool result = value.ToBoolean();
    return JSTaggedValue(result).GetRawData();
}

DEF_RUNTIME_STUBS(Eq)
{
    RUNTIME_STUBS_HEADER(Eq);
    JSHandle<JSTaggedValue> left = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> right = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeEq(thread, left, right).GetRawData();
}

DEF_RUNTIME_STUBS(NotEq)
{
    RUNTIME_STUBS_HEADER(NotEq);
    JSHandle<JSTaggedValue> left = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> right = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeNotEq(thread, left, right).GetRawData();
}

DEF_RUNTIME_STUBS(Less)
{
    RUNTIME_STUBS_HEADER(Less);
    JSHandle<JSTaggedValue> left = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> right = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeLess(thread, left, right).GetRawData();
}

DEF_RUNTIME_STUBS(LessEq)
{
    RUNTIME_STUBS_HEADER(LessEq);
    JSHandle<JSTaggedValue> left = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> right = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeLessEq(thread, left, right).GetRawData();
}

DEF_RUNTIME_STUBS(Greater)
{
    RUNTIME_STUBS_HEADER(Greater);
    JSHandle<JSTaggedValue> left = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> right = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeGreater(thread, left, right).GetRawData();
}

DEF_RUNTIME_STUBS(GreaterEq)
{
    RUNTIME_STUBS_HEADER(GreaterEq);
    JSHandle<JSTaggedValue> left = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> right = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeGreaterEq(thread, left, right).GetRawData();
}

DEF_RUNTIME_STUBS(Add2)
{
    RUNTIME_STUBS_HEADER(Add2);
    JSHandle<JSTaggedValue> left = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> right = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSTaggedValue res = RuntimeAdd2(thread, left, right);
    return res.GetRawData();
}

DEF_RUNTIME_STUBS(Sub2)
{
    RUNTIME_STUBS_HEADER(Sub2);
    JSHandle<JSTaggedValue> left = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> right = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeSub2(thread, left, right).GetRawData();
}

DEF_RUNTIME_STUBS(Mul2)
{
    RUNTIME_STUBS_HEADER(Mul2);
    JSHandle<JSTaggedValue> left = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> right = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeMul2(thread, left, right).GetRawData();
}

DEF_RUNTIME_STUBS(Div2)
{
    RUNTIME_STUBS_HEADER(Div2);
    JSHandle<JSTaggedValue> left = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> right = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeDiv2(thread, left, right).GetRawData();
}

DEF_RUNTIME_STUBS(Mod2)
{
    RUNTIME_STUBS_HEADER(Mod2);
    JSHandle<JSTaggedValue> left = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> right = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeMod2(thread, left, right).GetRawData();
}

DEF_RUNTIME_STUBS(JumpToCInterpreter)
{
#ifndef EXCLUDE_C_INTERPRETER
    RUNTIME_STUBS_HEADER(JumpToCInterpreter);
    JSTaggedValue constpool = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue profileTypeInfo = GetArg(argv, argc, 1);  // 1: means the first parameter
    JSTaggedValue acc = GetArg(argv, argc, 2);  // 2: means the second parameter
    JSTaggedValue hotnessCounter = GetArg(argv, argc, 3);  // 3: means the third parameter

    auto sp = const_cast<JSTaggedType *>(thread->GetCurrentInterpretedFrame());
    const uint8_t *currentPc = reinterpret_cast<const uint8_t*>(GET_ASM_FRAME(sp)->pc);

    uint8_t opcode = currentPc[0];
    asmDispatchTable[opcode](thread, currentPc, sp, constpool, profileTypeInfo, acc, hotnessCounter.GetInt());
    sp = const_cast<JSTaggedType *>(thread->GetCurrentInterpretedFrame());
    return JSTaggedValue(reinterpret_cast<uint64_t>(sp)).GetRawData();
#else
    return JSTaggedValue::Hole().GetRawData();
#endif
}

DEF_RUNTIME_STUBS(NotifyBytecodePcChanged)
{
    RUNTIME_STUBS_HEADER(NotifyBytecodePcChanged);
    FrameHandler frameHandler(thread);
    for (; frameHandler.HasFrame(); frameHandler.PrevJSFrame()) {
        if (frameHandler.IsEntryFrame() || frameHandler.IsBuiltinFrame()) {
            continue;
        }
        Method *method = frameHandler.GetMethod();
        // Skip builtins method
        if (method->IsNativeWithCallField()) {
            continue;
        }
        auto bcOffset = frameHandler.GetBytecodeOffset();
        auto *debuggerMgr = thread->GetEcmaVM()->GetJsDebuggerManager();
        debuggerMgr->GetNotificationManager()->BytecodePcChangedEvent(thread, method, bcOffset);
        return JSTaggedValue::Hole().GetRawData();
    }
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(NotifyDebuggerStatement)
{
    RUNTIME_STUBS_HEADER(NotifyDebuggerStatement);
    return RuntimeNotifyDebuggerStatement(thread).GetRawData();
}

DEF_RUNTIME_STUBS(MethodEntry)
{
    RUNTIME_STUBS_HEADER(MethodEntry);
    JSHandle<JSTaggedValue> func = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    if (func.GetTaggedValue().IsECMAObject()) {
        Method *method = ECMAObject::Cast(func.GetTaggedValue().GetTaggedObject())->GetCallTarget();
        if (method->IsNativeWithCallField()) {
            return JSTaggedValue::Hole().GetRawData();
        }
        JSHandle<JSFunction> funcObj = JSHandle<JSFunction>::Cast(func);
        FrameHandler frameHandler(thread);
        for (; frameHandler.HasFrame(); frameHandler.PrevJSFrame()) {
            if (frameHandler.IsEntryFrame() || frameHandler.IsBuiltinFrame()) {
                continue;
            }
            auto *debuggerMgr = thread->GetEcmaVM()->GetJsDebuggerManager();
            debuggerMgr->GetNotificationManager()->MethodEntryEvent(thread, method, funcObj->GetLexicalEnv());
            return JSTaggedValue::Hole().GetRawData();
        }
    }
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(MethodExit)
{
    RUNTIME_STUBS_HEADER(MethodExit);
    FrameHandler frameHandler(thread);
    for (; frameHandler.HasFrame(); frameHandler.PrevJSFrame()) {
        if (frameHandler.IsEntryFrame() || frameHandler.IsBuiltinFrame()) {
            continue;
        }
        Method *method = frameHandler.GetMethod();
        // Skip builtins method
        if (method->IsNativeWithCallField()) {
            continue;
        }
        auto *debuggerMgr = thread->GetEcmaVM()->GetJsDebuggerManager();
        debuggerMgr->GetNotificationManager()->MethodExitEvent(thread, method);
        return JSTaggedValue::Hole().GetRawData();
    }
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(CreateEmptyObject)
{
    RUNTIME_STUBS_HEADER(CreateEmptyObject);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    return RuntimeCreateEmptyObject(thread, factory, globalEnv).GetRawData();
}

DEF_RUNTIME_STUBS(CreateEmptyArray)
{
    RUNTIME_STUBS_HEADER(CreateEmptyArray);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    return RuntimeCreateEmptyArray(thread, factory, globalEnv).GetRawData();
}

DEF_RUNTIME_STUBS(GetSymbolFunction)
{
    RUNTIME_STUBS_HEADER(GetSymbolFunction);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    return globalEnv->GetSymbolFunction().GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(GetUnmapedArgs)
{
    RUNTIME_STUBS_HEADER(GetUnmapedArgs);
    auto sp = const_cast<JSTaggedType*>(thread->GetCurrentInterpretedFrame());
    uint32_t startIdx = 0;
    // 0: means restIdx
    uint32_t actualNumArgs = InterpreterAssembly::GetNumArgs(sp, 0, startIdx);
    return RuntimeGetUnmapedArgs(thread, sp, actualNumArgs, startIdx).GetRawData();
}

DEF_RUNTIME_STUBS(CopyRestArgs)
{
    RUNTIME_STUBS_HEADER(CopyRestArgs);
    JSTaggedValue restIdx = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    auto sp = const_cast<JSTaggedType*>(thread->GetCurrentInterpretedFrame());
    uint32_t startIdx = 0;
    uint32_t restNumArgs = InterpreterAssembly::GetNumArgs(sp, restIdx.GetInt(), startIdx);
    return RuntimeCopyRestArgs(thread, sp, restNumArgs, startIdx).GetRawData();
}

DEF_RUNTIME_STUBS(CreateArrayWithBuffer)
{
    RUNTIME_STUBS_HEADER(CreateArrayWithBuffer);
    JSHandle<JSTaggedValue> argArray = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    return RuntimeCreateArrayWithBuffer(thread, factory, argArray).GetRawData();
}

DEF_RUNTIME_STUBS(CreateObjectWithBuffer)
{
    RUNTIME_STUBS_HEADER(CreateObjectWithBuffer);
    JSHandle<JSObject> argObj = GetHArg<JSObject>(argv, argc, 0);  // 0: means the zeroth parameter
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    return RuntimeCreateObjectWithBuffer(thread, factory, argObj).GetRawData();
}

DEF_RUNTIME_STUBS(NewThisObject)
{
    RUNTIME_STUBS_HEADER(NewThisObject);
    JSHandle<JSFunction> ctor(GetHArg<JSTaggedValue>(argv, argc, 0));  // 0: means the zeroth parameter

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(ctor);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
    return obj.GetTaggedType();  // state is not set here
}

DEF_RUNTIME_STUBS(NewObjRange)
{
    RUNTIME_STUBS_HEADER(NewObjRange);
    JSHandle<JSTaggedValue> func = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> newTarget = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSTaggedValue firstArgIdx = GetArg(argv, argc, 2);  // 2: means the second parameter
    JSTaggedValue length = GetArg(argv, argc, 3);  // 3: means the third parameter
    return RuntimeNewObjRange(thread, func, newTarget, static_cast<uint16_t>(firstArgIdx.GetInt()),
        static_cast<uint16_t>(length.GetInt())).GetRawData();
}

DEF_RUNTIME_STUBS(DefineFunc)
{
    RUNTIME_STUBS_HEADER(DefineFunc);
    JSHandle<Method> method = GetHArg<Method>(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeDefinefunc(thread, method).GetRawData();
}

DEF_RUNTIME_STUBS(CreateRegExpWithLiteral)
{
    RUNTIME_STUBS_HEADER(CreateRegExpWithLiteral);
    JSHandle<JSTaggedValue> pattern = GetHArg<JSTaggedValue>(argv, argc, 0);
    JSTaggedValue flags = GetArg(argv, argc, 1);
    return RuntimeCreateRegExpWithLiteral(thread, pattern, static_cast<uint8_t>(flags.GetInt())).GetRawData();
}

DEF_RUNTIME_STUBS(ThrowIfSuperNotCorrectCall)
{
    RUNTIME_STUBS_HEADER(ThrowIfSuperNotCorrectCall);
    JSTaggedValue index = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue thisValue = GetArg(argv, argc, 1);  // 1: means the first parameter
    return RuntimeThrowIfSuperNotCorrectCall(thread, static_cast<uint16_t>(index.GetInt()), thisValue).GetRawData();
}

DEF_RUNTIME_STUBS(CreateObjectHavingMethod)
{
    RUNTIME_STUBS_HEADER(CreateObjectHavingMethod);
    JSHandle<JSObject> literal = GetHArg<JSObject>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> env = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    return RuntimeCreateObjectHavingMethod(thread, factory, literal, env).GetRawData();
}

DEF_RUNTIME_STUBS(CreateObjectWithExcludedKeys)
{
    RUNTIME_STUBS_HEADER(CreateObjectWithExcludedKeys);
    JSTaggedValue numKeys = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> objVal = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSTaggedValue firstArgRegIdx = GetArg(argv, argc, 2);  // 2: means the second parameter
    return RuntimeCreateObjectWithExcludedKeys(thread, static_cast<uint16_t>(numKeys.GetInt()), objVal,
        static_cast<uint16_t>(firstArgRegIdx.GetInt())).GetRawData();
}

DEF_RUNTIME_STUBS(DefineMethod)
{
    RUNTIME_STUBS_HEADER(DefineMethod);
    JSHandle<Method> method = GetHArg<Method>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> homeObject = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeDefineMethod(thread, method, homeObject).GetRawData();
}

DEF_RUNTIME_STUBS(CallSpread)
{
    RUNTIME_STUBS_HEADER(CallSpread);
    JSHandle<JSTaggedValue> func = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> array = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    return RuntimeCallSpread(thread, func, obj, array).GetRawData();
}

DEF_RUNTIME_STUBS(DefineGetterSetterByValue)
{
    RUNTIME_STUBS_HEADER(DefineGetterSetterByValue);
    JSHandle<JSObject> obj = GetHArg<JSObject>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> prop = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> getter = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    JSHandle<JSTaggedValue> setter = GetHArg<JSTaggedValue>(argv, argc, 3);  // 3: means the third parameter
    JSTaggedValue flag = GetArg(argv, argc, 4);  // 4: means the fourth parameter
    bool bFlag = flag.ToBoolean();
    return RuntimeDefineGetterSetterByValue(thread, obj, prop, getter, setter, bFlag).GetRawData();
}

DEF_RUNTIME_STUBS(SuperCall)
{
    RUNTIME_STUBS_HEADER(SuperCall);
    JSHandle<JSTaggedValue> func = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue firstVRegIdx = GetArg(argv, argc, 1);  // 1: means the first parameter
    JSTaggedValue length = GetArg(argv, argc, 2);  // 2: means the second parameter
    auto sp = const_cast<JSTaggedType*>(thread->GetCurrentInterpretedFrame());
    JSTaggedValue newTarget = InterpreterAssembly::GetNewTarget(sp);
    return RuntimeSuperCall(thread, func, JSHandle<JSTaggedValue>(thread, newTarget),
        static_cast<uint16_t>(firstVRegIdx.GetInt()),
        static_cast<uint16_t>(length.GetInt())).GetRawData();
}

DEF_RUNTIME_STUBS(OptSuperCall)
{
    RUNTIME_STUBS_HEADER(OptSuperCall);
    return RuntimeOptSuperCall(thread, argv, argc).GetRawData();
}

DEF_RUNTIME_STUBS(ThrowNotCallableException)
{
    RUNTIME_STUBS_HEADER(ThrowNotCallableException);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR, "is not callable");
    thread->SetException(error.GetTaggedValue());
    return JSTaggedValue::Exception().GetRawData();
}

DEF_RUNTIME_STUBS(ThrowSetterIsUndefinedException)
{
    RUNTIME_STUBS_HEADER(ThrowSetterIsUndefinedException);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR,
        "Cannot set property when setter is undefined");
    thread->SetException(error.GetTaggedValue());
    return JSTaggedValue::Exception().GetRawData();
}

DEF_RUNTIME_STUBS(ThrowCallConstructorException)
{
    RUNTIME_STUBS_HEADER(ThrowCallConstructorException);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR,
                                                   "class constructor cannot called without 'new'");
    thread->SetException(error.GetTaggedValue());
    return JSTaggedValue::Exception().GetRawData();
}

DEF_RUNTIME_STUBS(ThrowNonConstructorException)
{
    RUNTIME_STUBS_HEADER(ThrowNonConstructorException);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR,
                                                   "function is non-constructor");
    thread->SetException(error.GetTaggedValue());
    return JSTaggedValue::Exception().GetRawData();
}

DEF_RUNTIME_STUBS(ThrowStackOverflowException)
{
    RUNTIME_STUBS_HEADER(ThrowStackOverflowException);
    return RuntimeThrowStackOverflowException(thread).GetRawData();
}

DEF_RUNTIME_STUBS(ThrowDerivedMustReturnException)
{
    RUNTIME_STUBS_HEADER(ThrowDerivedMustReturnException);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR,
                                                   "Derived constructor must return object or undefined");
    thread->SetException(error.GetTaggedValue());
    return JSTaggedValue::Exception().GetRawData();
}

DEF_RUNTIME_STUBS(LdBigInt)
{
    RUNTIME_STUBS_HEADER(LdBigInt);
    JSHandle<JSTaggedValue> numberBigInt = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeLdBigInt(thread, numberBigInt).GetRawData();
}

DEF_RUNTIME_STUBS(ToNumeric)
{
    RUNTIME_STUBS_HEADER(ToNumeric);
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeToNumeric(thread, value).GetRawData();
}

DEF_RUNTIME_STUBS(DynamicImport)
{
    RUNTIME_STUBS_HEADER(DynamicImport);
    JSHandle<JSTaggedValue> specifier = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> currentFunc = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the zeroth parameter
    return RuntimeDynamicImport(thread, specifier, currentFunc).GetRawData();
}

DEF_RUNTIME_STUBS(NewLexicalEnvWithName)
{
    RUNTIME_STUBS_HEADER(NewLexicalEnvWithName);
    JSTaggedValue numVars = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue scopeId = GetArg(argv, argc, 1);  // 1: means the first parameter
    return RuntimeNewLexicalEnvWithName(thread,
        static_cast<uint16_t>(numVars.GetInt()),
        static_cast<uint16_t>(scopeId.GetInt())).GetRawData();
}

DEF_RUNTIME_STUBS(OptGetUnmapedArgs)
{
    RUNTIME_STUBS_HEADER(OptGetUnmapedArgs);
    JSTaggedValue actualNumArgs = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeOptGetUnmapedArgs(thread, actualNumArgs.GetInt()).GetRawData();
}

DEF_RUNTIME_STUBS(OptNewLexicalEnvWithName)
{
    RUNTIME_STUBS_HEADER(OptNewLexicalEnvWithName);
    JSTaggedValue taggedNumVars = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue taggedScopeId = GetArg(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> currentLexEnv = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    JSHandle<JSTaggedValue> func = GetHArg<JSTaggedValue>(argv, argc, 3);  // 3: means the third parameter
    uint16_t numVars = static_cast<uint16_t>(taggedNumVars.GetInt());
    uint16_t scopeId = static_cast<uint16_t>(taggedScopeId.GetInt());
    return RuntimeOptNewLexicalEnvWithName(thread, numVars, scopeId, currentLexEnv, func).GetRawData();
}

DEF_RUNTIME_STUBS(OptCopyRestArgs)
{
    RUNTIME_STUBS_HEADER(OptCopyRestArgs);
    JSTaggedValue actualArgc = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue restIndex = GetArg(argv, argc, 1);  // 1: means the first parameter
    return RuntimeOptCopyRestArgs(thread, actualArgc.GetInt(), restIndex.GetInt()).GetRawData();
}

DEF_RUNTIME_STUBS(OptNewObjRange)
{
    RUNTIME_STUBS_HEADER(OptNewObjRange);
    return RuntimeOptNewObjRange(thread, argv, argc).GetRawData();
}

DEF_RUNTIME_STUBS(GetTypeArrayPropertyByIndex)
{
    RUNTIME_STUBS_HEADER(GetTypeArrayPropertyByIndex);
    JSTaggedValue obj = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue idx = GetArg(argv, argc, 1);  // 1: means the first parameter
    JSTaggedValue jsType = GetArg(argv, argc, 2); // 2: means the second parameter
    return JSTypedArray::FastGetPropertyByIndex(thread, obj, idx.GetInt(), JSType(jsType.GetInt())).GetRawData();
}

DEF_RUNTIME_STUBS(SetTypeArrayPropertyByIndex)
{
    RUNTIME_STUBS_HEADER(SetTypeArrayPropertyByIndex);
    JSTaggedValue obj = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue idx = GetArg(argv, argc, 1);  // 1: means the first parameter
    JSTaggedValue value = GetArg(argv, argc, 2);  // 2: means the second parameter
    JSTaggedValue jsType = GetArg(argv, argc, 3); // 3: means the third parameter
    return JSTypedArray::FastSetPropertyByIndex(thread, obj, idx.GetInt(), value, JSType(jsType.GetInt())).GetRawData();
}

DEF_RUNTIME_STUBS(FastCopyElementToArray)
{
    RUNTIME_STUBS_HEADER(FastCopyElementToArray);
    JSHandle<JSTaggedValue> typedArray = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<TaggedArray> array = GetHArg<TaggedArray>(argv, argc, 1);  // 1: means the first parameter
    return JSTaggedValue(JSTypedArray::FastCopyElementToArray(thread, typedArray, array)).GetRawData();
}

DEF_RUNTIME_STUBS(DebugAOTPrint)
{
    RUNTIME_STUBS_HEADER(DebugAOTPrint);
    int ecmaOpcode = GetArg(argv, argc, 0).GetInt();
    int path = GetArg(argv, argc, 1).GetInt();
    std::string result = kungfu::GetEcmaOpcodeStr(static_cast<EcmaOpcode>(ecmaOpcode));
    std::string pathStr = path == 0 ? "slowpath " : "typedpath ";
    LOG_ECMA(INFO) << "aot " << pathStr << result;
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(ProfileOptimizedCode)
{
    RUNTIME_STUBS_HEADER(ProfileOptimizedCode);
    EcmaOpcode ecmaOpcode = static_cast<EcmaOpcode>(GetArg(argv, argc, 0).GetInt());
    OptCodeProfiler::Mode mode = static_cast<OptCodeProfiler::Mode>(GetArg(argv, argc, 1).GetInt());
    OptCodeProfiler *profiler = thread->GetCurrentEcmaContext()->GetOptCodeProfiler();
    profiler->Update(ecmaOpcode, mode);
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(VerifyVTableLoading)
{
    RUNTIME_STUBS_HEADER(VerifyVTableLoading);
    JSHandle<JSTaggedValue> receiver = GetHArg<JSTaggedValue>(argv, argc, 0);        // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> key = GetHArg<JSTaggedValue>(argv, argc, 1);             // 1: means the first parameter
    JSHandle<JSTaggedValue> typedPathValue = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter

    JSHandle<JSTaggedValue> verifiedPathValue = JSTaggedValue::GetProperty(thread, receiver, key).GetValue();
    if (UNLIKELY(!JSTaggedValue::SameValue(typedPathValue, verifiedPathValue))) {
        std::ostringstream oss;
        receiver->Dump(oss);
        LOG_ECMA(ERROR) << "Verify VTable Load Failed, receiver: " << oss.str();
        oss.str("");

        LOG_ECMA(ERROR) << "Verify VTable Load Failed, key: "
                        << EcmaStringAccessor(key.GetTaggedValue()).ToStdString();

        typedPathValue->Dump(oss);
        LOG_ECMA(ERROR) << "Verify VTable Load Failed, typed path value: " << oss.str();
        oss.str("");

        verifiedPathValue->Dump(oss);
        LOG_ECMA(ERROR) << "Verify VTable Load Failed, verified path value: " << oss.str();
    }
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(VerifyVTableStoring)
{
    RUNTIME_STUBS_HEADER(VerifyVTableStoring);
    JSHandle<JSTaggedValue> receiver = GetHArg<JSTaggedValue>(argv, argc, 0);    // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> key = GetHArg<JSTaggedValue>(argv, argc, 1);         // 1: means the first parameter
    JSHandle<JSTaggedValue> storeValue = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter

    JSHandle<JSTaggedValue> verifiedValue = JSTaggedValue::GetProperty(thread, receiver, key).GetValue();
    if (UNLIKELY(!JSTaggedValue::SameValue(storeValue, verifiedValue))) {
        std::ostringstream oss;
        receiver->Dump(oss);
        LOG_ECMA(ERROR) << "Verify VTable Store Failed, receiver: " << oss.str();
        oss.str("");

        LOG_ECMA(ERROR) << "Verify VTable Store Failed, key: "
                        << EcmaStringAccessor(key.GetTaggedValue()).ToStdString();

        storeValue->Dump(oss);
        LOG_ECMA(ERROR) << "Verify VTable Store Failed, typed path store value: " << oss.str();
        oss.str("");

        verifiedValue->Dump(oss);
        LOG_ECMA(ERROR) << "Verify VTable Store Failed, verified path load value: " << oss.str();
    }
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(JSObjectGetMethod)
{
    RUNTIME_STUBS_HEADER(JSObjectGetMethod);
    JSHandle<JSTaggedValue> obj(thread, GetArg(argv, argc, 0));
    JSHandle<JSTaggedValue> key(thread, GetArg(argv, argc, 1));
    JSHandle<JSTaggedValue> result = JSObject::GetMethod(thread, obj, key);
    return result->GetRawData();
}

DEF_RUNTIME_STUBS(BigIntEqual)
{
    RUNTIME_STUBS_HEADER(BigIntEqual);
    JSTaggedValue left = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue right = GetArg(argv, argc, 1);  // 1: means the first parameter
    if (BigInt::Equal(left, right)) {
        return JSTaggedValue::VALUE_TRUE;
    }
    return JSTaggedValue::VALUE_FALSE;
}

DEF_RUNTIME_STUBS(StringEqual)
{
    RUNTIME_STUBS_HEADER(StringEqual);
    JSHandle<EcmaString> left = GetHArg<EcmaString>(argv, argc, 0);
    JSHandle<EcmaString> right = GetHArg<EcmaString>(argv, argc, 1);
    EcmaVM *vm = thread->GetEcmaVM();
    left = JSHandle<EcmaString>(thread, EcmaStringAccessor::Flatten(vm, left));
    right = JSHandle<EcmaString>(thread, EcmaStringAccessor::Flatten(vm, right));
    if (EcmaStringAccessor::StringsAreEqualSameUtfEncoding(*left, *right)) {
        return JSTaggedValue::VALUE_TRUE;
    }
    return JSTaggedValue::VALUE_FALSE;
}

DEF_RUNTIME_STUBS(LdPatchVar)
{
    RUNTIME_STUBS_HEADER(LdPatchVar);
    JSTaggedValue idx = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeLdPatchVar(thread, idx.GetInt()).GetRawData();
}

DEF_RUNTIME_STUBS(StPatchVar)
{
    RUNTIME_STUBS_HEADER(StPatchVar);
    JSTaggedValue idx = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeStPatchVar(thread, idx.GetInt(), value).GetRawData();
}

DEF_RUNTIME_STUBS(NotifyConcurrentResult)
{
    RUNTIME_STUBS_HEADER(NotifyConcurrentResult);
    JSTaggedValue result = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue hint = GetArg(argv, argc, 1);  // 1: means the first parameter
    return RuntimeNotifyConcurrentResult(thread, result, hint).GetRawData();
}

DEF_RUNTIME_STUBS(ContainerRBTreeForEach)
{
    RUNTIME_STUBS_HEADER(ContainerRBTreeForEach);
    JSHandle<JSTaggedValue> node = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: param index
    JSHandle<JSTaggedValue> callbackFnHandle = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: param index
    JSHandle<JSTaggedValue> thisArgHandle = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: param index
    JSHandle<JSTaggedValue> thisHandle = GetHArg<JSTaggedValue>(argv, argc, 3);  // 3: param index
    JSHandle<JSTaggedValue> type = GetHArg<JSTaggedValue>(argv, argc, 4);  // 4: param index

    ASSERT(node->IsRBTreeNode());
    ASSERT(callbackFnHandle->IsCallable());
    ASSERT(type->IsInt());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    auto containersType = static_cast<kungfu::ContainersType>(type->GetInt());
    JSMutableHandle<TaggedQueue> queue(thread, thread->GetEcmaVM()->GetFactory()->NewTaggedQueue(0));
    JSMutableHandle<RBTreeNode> treeNode(thread, JSTaggedValue::Undefined());
    queue.Update(JSTaggedValue(TaggedQueue::Push(thread, queue, node)));
    while (!queue->Empty()) {
        treeNode.Update(queue->Pop(thread));
        EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle,
                                                                        undefined, 3); // 3: three args
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
        info->SetCallArg(containersType == kungfu::ContainersType::HASHSET_FOREACH ?
                         treeNode->GetKey() : treeNode->GetValue(), treeNode->GetKey(), thisHandle.GetTaggedValue());
        JSTaggedValue funcResult = JSFunction::Call(info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult.GetRawData());
        if (!treeNode->GetLeft().IsHole()) {
            JSHandle<JSTaggedValue> left(thread, treeNode->GetLeft());
            queue.Update(JSTaggedValue(TaggedQueue::Push(thread, queue, left)));
        }
        if (!treeNode->GetRight().IsHole()) {
            JSHandle<JSTaggedValue> right(thread, treeNode->GetRight());
            queue.Update(JSTaggedValue(TaggedQueue::Push(thread, queue, right)));
        }
    }
    return JSTaggedValue::True().GetRawData();
}

DEF_RUNTIME_STUBS(SlowFlattenString)
{
    RUNTIME_STUBS_HEADER(SlowFlattenString);
    JSHandle<TreeEcmaString> str = GetHArg<TreeEcmaString>(argv, argc, 0);  // 0: means the zeroth parameter
    return JSTaggedValue(EcmaStringAccessor::SlowFlatten(thread->GetEcmaVM(), str)).GetRawData();
}

JSTaggedType RuntimeStubs::CreateArrayFromList([[maybe_unused]] uintptr_t argGlue, int32_t argc,
                                               JSTaggedValue *argvPtr)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> taggedArray = factory->NewTaggedArray(argc - NUM_MANDATORY_JSFUNC_ARGS);
    for (int index = NUM_MANDATORY_JSFUNC_ARGS; index < argc; ++index) {
        taggedArray->Set(thread, index - NUM_MANDATORY_JSFUNC_ARGS, argvPtr[index]);
    }
    JSHandle<JSArray> arrHandle = JSArray::CreateArrayFromList(thread, taggedArray);
    return arrHandle.GetTaggedValue().GetRawData();
}

int32_t RuntimeStubs::FindElementWithCache(uintptr_t argGlue, JSTaggedType hclass,
                                           JSTaggedType key, int32_t num)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    auto cls = reinterpret_cast<JSHClass *>(hclass);
    JSTaggedValue propKey = JSTaggedValue(key);
    auto layoutInfo = LayoutInfo::Cast(cls->GetLayout().GetTaggedObject());
    PropertiesCache *cache = thread->GetPropertiesCache();
    int index = cache->Get(cls, propKey);
    if (index == PropertiesCache::NOT_FOUND) {
        index = layoutInfo->BinarySearch(propKey, num);
        cache->Set(cls, propKey, index);
    }
    return index;
}

JSTaggedType RuntimeStubs::GetActualArgvNoGC(uintptr_t argGlue)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    JSTaggedType *current = const_cast<JSTaggedType *>(thread->GetLastLeaveFrame());
    FrameIterator it(current, thread);
    ASSERT(it.IsOptimizedFrame());
    it.Advance<GCVisitedFlag::VISITED>();
    ASSERT(it.IsOptimizedJSFunctionFrame());
    auto optimizedJSFunctionFrame = it.GetFrame<OptimizedJSFunctionFrame>();
    return reinterpret_cast<uintptr_t>(optimizedJSFunctionFrame->GetArgv(it));
}

double RuntimeStubs::FloatMod(double x, double y)
{
    return std::fmod(x, y);
}

JSTaggedType RuntimeStubs::FloatSqrt(double x)
{
    double result = std::sqrt(x);
    return JSTaggedValue(result).GetRawData();
}

JSTaggedType RuntimeStubs::FloatCos(double x)
{
    double result = std::cos(x);
    return JSTaggedValue(result).GetRawData();
}

JSTaggedType RuntimeStubs::FloatSin(double x)
{
    double result = std::sin(x);
    return JSTaggedValue(result).GetRawData();
}

JSTaggedType RuntimeStubs::FloatACos(double x)
{
    double result = std::acos(x);
    return JSTaggedValue(result).GetRawData();
}

JSTaggedType RuntimeStubs::FloatATan(double x)
{
    double result = std::atan(x);
    return JSTaggedValue(result).GetRawData();
}

JSTaggedType RuntimeStubs::FloatFloor(double x)
{
    ASSERT(!std::isnan(x));
    double result = std::floor(x);
    return JSTaggedValue(result).GetRawData();
}

int32_t RuntimeStubs::DoubleToInt(double x)
{
    return base::NumberHelper::DoubleToInt(x, base::INT32_BITS);
}

JSTaggedType RuntimeStubs::DoubleToLength(double x)
{
    double length = base::NumberHelper::TruncateDouble(x);
    if (length < 0.0) {
        return JSTaggedNumber(static_cast<double>(0)).GetRawData();
    }
    if (length > SAFE_NUMBER) {
        return JSTaggedNumber(static_cast<double>(SAFE_NUMBER)).GetRawData();
    }
    return JSTaggedNumber(length).GetRawData();
}

void RuntimeStubs::InsertOldToNewRSet([[maybe_unused]] uintptr_t argGlue,
    uintptr_t object, size_t offset)
{
    Region *region = Region::ObjectAddressToRange(object);
    uintptr_t slotAddr = object + offset;
    return region->InsertOldToNewRSet(slotAddr);
}

void RuntimeStubs::MarkingBarrier([[maybe_unused]] uintptr_t argGlue,
    uintptr_t object, size_t offset, TaggedObject *value)
{
    uintptr_t slotAddr = object + offset;
    Region *objectRegion = Region::ObjectAddressToRange(object);
    Region *valueRegion = Region::ObjectAddressToRange(value);
#if ECMASCRIPT_ENABLE_BARRIER_CHECK
    if (!valueRegion->GetJSThread()->GetEcmaVM()->GetHeap()->IsAlive(JSTaggedValue(value).GetHeapObject())) {
        LOG_FULL(FATAL) << "RuntimeStubs::MarkingBarrier checked value:" << value << " is invalid!";
    }
#endif
    if (!valueRegion->IsMarking()) {
        return;
    }
    Barriers::Update(slotAddr, objectRegion, value, valueRegion);
}

void RuntimeStubs::StoreBarrier([[maybe_unused]] uintptr_t argGlue,
    uintptr_t object, size_t offset, TaggedObject *value)
{
    uintptr_t slotAddr = object + offset;
    Region *objectRegion = Region::ObjectAddressToRange(object);
    Region *valueRegion = Region::ObjectAddressToRange(value);
#if ECMASCRIPT_ENABLE_BARRIER_CHECK
    if (!valueRegion->GetJSThread()->GetEcmaVM()->GetHeap()->IsAlive(JSTaggedValue(value).GetHeapObject())) {
        LOG_FULL(FATAL) << "RuntimeStubs::StoreBarrier checked value:" << value << " is invalid!";
    }
#endif
    if (!objectRegion->InYoungSpace() && valueRegion->InYoungSpace()) {
        // Should align with '8' in 64 and 32 bit platform
        ASSERT((slotAddr % static_cast<uint8_t>(MemAlignment::MEM_ALIGN_OBJECT)) == 0);
        objectRegion->InsertOldToNewRSet(slotAddr);
    }
    if (!valueRegion->IsMarking()) {
        return;
    }
    Barriers::Update(slotAddr, objectRegion, value, valueRegion);
}

bool RuntimeStubs::StringsAreEquals(EcmaString *str1, EcmaString *str2)
{
    return EcmaStringAccessor::StringsAreEqualSameUtfEncoding(str1, str2);
}

bool RuntimeStubs::BigIntEquals(JSTaggedType left, JSTaggedType right)
{
    return BigInt::Equal(JSTaggedValue(left), JSTaggedValue(right));
}

double RuntimeStubs::TimeClip(double time)
{
    return JSDate::TimeClip(time);
}

double RuntimeStubs::SetDateValues(double year, double month, double day)
{
    if (std::isnan(year) || !std::isfinite(year) || std::isnan(month) || !std::isfinite(month) || std::isnan(day) ||
        !std::isfinite(day)) {
        return base::NAN_VALUE;
    }

    return JSDate::SetDateValues(static_cast<int64_t>(year), static_cast<int64_t>(month), static_cast<int64_t>(day));
}

JSTaggedValue RuntimeStubs::NewObject(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    JSHandle<JSTaggedValue> func(info->GetFunction());
    if (!func->IsHeapObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "function is nullptr", JSTaggedValue::Exception());
    }

    if (!func->IsJSFunction()) {
        if (func->IsBoundFunction()) {
            JSTaggedValue result = JSBoundFunction::ConstructInternal(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            return result;
        }

        if (func->IsJSProxy()) {
            JSTaggedValue jsObj = JSProxy::ConstructInternal(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            return jsObj;
        }
        THROW_TYPE_ERROR_AND_RETURN(thread, "Constructed NonConstructable", JSTaggedValue::Exception());
    }

    JSTaggedValue result = JSFunction::ConstructInternal(info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result;
}

void RuntimeStubs::SaveFrameToContext(JSThread *thread, JSHandle<GeneratorContext> context)
{
    FrameHandler frameHandler(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t nregs = frameHandler.GetNumberArgs();
    JSHandle<TaggedArray> regsArray = factory->NewTaggedArray(nregs);
    for (uint32_t i = 0; i < nregs; i++) {
        JSTaggedValue value = frameHandler.GetVRegValue(i);
        regsArray->Set(thread, i, value);
    }
    context->SetRegsArray(thread, regsArray.GetTaggedValue());
    JSHandle<JSFunction> function(thread, frameHandler.GetFunction());
    JSHandle<JSHClass> hclass(thread, function->GetClass());
    if (hclass->IsOptimized()) {
        FunctionKind kind = function->GetCallTarget()->GetFunctionKind();
        // instead of hclass by non_optimized hclass when method ClearAOTFlags
        JSHandle<JSHClass> newHClass = factory->GetNonOptimizedHclass(hclass, kind);
        function->SynchronizedSetClass(*newHClass);
    }
    context->SetMethod(thread, function.GetTaggedValue());
    context->SetThis(thread, frameHandler.GetThis());

    BytecodeInstruction ins(frameHandler.GetPc());
    auto offset = ins.GetSize();
    context->SetAcc(thread, frameHandler.GetAcc());
    context->SetLexicalEnv(thread, thread->GetCurrentLexenv());
    context->SetNRegs(nregs);
    context->SetBCOffset(frameHandler.GetBytecodeOffset() + offset);
}

JSTaggedValue RuntimeStubs::CallBoundFunction(EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    JSHandle<JSBoundFunction> boundFunc(info->GetFunction());
    JSHandle<JSFunction> targetFunc(thread, boundFunc->GetBoundTarget());
    if (targetFunc->IsClassConstructor()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "class constructor cannot called without 'new'",
                                    JSTaggedValue::Exception());
    }
    if (thread->IsPGOProfilerEnable()) {
        ECMAObject *callTarget = reinterpret_cast<ECMAObject*>(targetFunc.GetTaggedValue().GetTaggedObject());
        ASSERT(callTarget != nullptr);
        Method *method = callTarget->GetCallTarget();
        if (!method->IsNativeWithCallField()) {
            thread->GetEcmaVM()->GetPGOProfiler()->ProfileCall(
                JSTaggedValue::VALUE_UNDEFINED, targetFunc.GetTaggedType());
        }
    }
    JSHandle<TaggedArray> boundArgs(thread, boundFunc->GetBoundArguments());
    const uint32_t boundLength = boundArgs->GetLength();
    const uint32_t argsLength = info->GetArgsNumber() + boundLength;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *runtimeInfo = EcmaInterpreter::NewRuntimeCallInfo(thread, JSHandle<JSTaggedValue>(targetFunc),
        info->GetThis(), undefined, argsLength);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (boundLength == 0) {
        runtimeInfo->SetCallArg(argsLength, 0, info, 0);
    } else {
        // 0 ~ boundLength is boundArgs; boundLength ~ argsLength is args of EcmaRuntimeCallInfo.
        runtimeInfo->SetCallArg(boundLength, boundArgs);
        runtimeInfo->SetCallArg(argsLength, boundLength, info, 0);
    }
    return EcmaInterpreter::Execute(runtimeInfo);
}

DEF_RUNTIME_STUBS(DeoptHandler)
{
    RUNTIME_STUBS_HEADER(DeoptHandler);
    size_t depth = static_cast<size_t>(GetArg(argv, argc, 1).GetInt());
    Deoptimizier deopt(thread, depth);
    std::vector<kungfu::ARKDeopt> deoptBundle;
    deopt.CollectDeoptBundleVec(deoptBundle);
    ASSERT(!deoptBundle.empty());
    size_t shift = Deoptimizier::ComputeShift(depth);
    deopt.CollectVregs(deoptBundle, shift);
    kungfu::DeoptType type = static_cast<kungfu::DeoptType>(GetArg(argv, argc, 0).GetInt());
    deopt.UpdateAndDumpDeoptInfo(type);
    return deopt.ConstructAsmInterpretFrame();
}

DEF_RUNTIME_STUBS(AotInlineTrace)
{
    RUNTIME_STUBS_HEADER(AotInlineTrace);
    JSTaggedValue callerFunc = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue inlineFunc = GetArg(argv, argc, 1);  // 1: means the first parameter
    JSFunction *callerJSFunc = JSFunction::Cast(callerFunc);
    JSFunction *inlineJSFunc = JSFunction::Cast(inlineFunc);
    Method *callerMethod = Method::Cast(JSFunction::Cast(callerJSFunc)->GetMethod());
    Method *inlineMethod = Method::Cast(JSFunction::Cast(inlineJSFunc)->GetMethod());
    auto callerRecordName = callerMethod->GetRecordName();
    auto inlineRecordNanme = inlineMethod->GetRecordName();
    const std::string callerFuncName(callerMethod->GetMethodName());
    const std::string inlineFuncNanme(inlineMethod->GetMethodName());
    std::string callerFullName = callerFuncName + "@" + std::string(callerRecordName);
    std::string inlineFullName = inlineFuncNanme + "@" + std::string(inlineRecordNanme);

    LOG_TRACE(INFO) << "aot inline function name: " << inlineFullName << " caller function name: " << callerFullName;
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(LocaleCompare)
{
    RUNTIME_STUBS_HEADER(LocaleCompare);

    JSHandle<JSTaggedValue> thisTag = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> thatTag = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> locales = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    JSHandle<JSTaggedValue> options = GetHArg<JSTaggedValue>(argv, argc, 3);  // 3: means the third parameter

    JSHandle<JSTaggedValue> thisObj(JSTaggedValue::RequireObjectCoercible(thread, thisTag));
    [[maybe_unused]] JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisObj);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
    [[maybe_unused]] JSHandle<EcmaString> thatHandle = JSTaggedValue::ToString(thread, thatTag);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());

    [[maybe_unused]] bool cacheable = options->IsUndefined() && (locales->IsUndefined() || locales->IsString());
#ifdef ARK_SUPPORT_INTL
    if (cacheable) {
        auto collator = JSCollator::GetCachedIcuCollator(thread, locales);
        if (collator != nullptr) {
            JSTaggedValue result = JSCollator::CompareStrings(collator, thisHandle, thatHandle);
            return result.GetRawData();
        }
    }
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSTaggedValue> ctor = ecmaVm->GetGlobalEnv()->GetCollatorFunction();
    JSHandle<JSCollator> collator =
        JSHandle<JSCollator>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor)));
    JSHandle<JSCollator> initCollator =
        JSCollator::InitializeCollator(thread, collator, locales, options, cacheable, true);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
    icu::Collator *icuCollator = nullptr;
    if (cacheable) {
        icuCollator = JSCollator::GetCachedIcuCollator(thread, locales);
        ASSERT(icuCollator != nullptr);
    } else {
        icuCollator = initCollator->GetIcuCollator();
    }
    JSTaggedValue result = JSCollator::FastCompareStrings(thread, icuCollator, thisHandle, thatHandle);
    return result.GetRawData();
#else
#ifdef ARK_NOT_SUPPORT_INTL_GLOBAL
    ARK_SUPPORT_INTL_RETURN_JSVALUE(thread, "LocaleCompare");
#else
    intl::GlobalIntlHelper gh(thread, intl::GlobalFormatterType::Collator);
    auto collator = gh.GetGlobalObject<intl::GlobalCollator>(thread,
        locales, options, intl::GlobalFormatterType::Collator, cacheable);
    if (collator == nullptr) {
        LOG_ECMA(ERROR) << "BuiltinsString::LocaleCompare:collator is nullptr";
    }
    ASSERT(collator != nullptr);
    auto result = collator->Compare(EcmaStringAccessor(thisHandle).ToStdString(),
        EcmaStringAccessor(thatHandle).ToStdString());
    return JSTaggedValue(result).GetRawData();
#endif
#endif
}

DEF_RUNTIME_STUBS(ArraySort)
{
    RUNTIME_STUBS_HEADER(ArraySort);

    JSHandle<JSTaggedValue> thisHandle = GetHArg<JSTaggedValue>(argv, argc, 0);
    return RuntimeArraySort(thread, thisHandle).GetRawData();
}

JSTaggedValue RuntimeStubs::RuntimeArraySort(JSThread *thread, JSHandle<JSTaggedValue> thisHandle)
{
    // 1. Let obj be ToObject(this value).
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());

    // 2. Let len be ToLength(Get(obj, "length")).
    int64_t len = ArrayHelper::GetArrayLength(thread, JSHandle<JSTaggedValue>(thisObjHandle));
    // 3. ReturnIfAbrupt(len).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
    JSHandle<JSHClass> hclass(thread, thisObjHandle->GetClass());
    if (!hclass->IsDictionaryElement()) {
        JSHandle<TaggedArray> elements(thread, thisObjHandle->GetElements());
        // remove elements number check with pgo later and add int fast path at the same time
        if (len <= elements->GetLength() && CheckElementsNumber(elements, len)) {
            return ArrayNumberSort(thread, thisObjHandle, len);
        }
    }

    JSMutableHandle<JSTaggedValue> presentValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> middleValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> previousValue(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> callbackFnHandle(thread, JSTaggedValue::Undefined());
    for (int i = 1; i < len; i++) {
        int beginIndex = 0;
        int endIndex = i;
        presentValue.Update(
            ObjectFastOperator::FastGetPropertyByIndex<true>(thread, thisObjHandle.GetTaggedValue(), i));
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        while (beginIndex < endIndex) {
            int middleIndex = beginIndex + (endIndex - beginIndex) / 2; // 2 : half
            middleValue.Update(
                ObjectFastOperator::FastGetPropertyByIndex<true>(thread, thisObjHandle.GetTaggedValue(), middleIndex));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
            double compareResult = ArrayHelper::SortCompare(thread, callbackFnHandle, middleValue, presentValue);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
            if (compareResult > 0) {
                endIndex = middleIndex;
            } else {
                beginIndex = middleIndex + 1;
            }
        }

        if (endIndex >= 0 && endIndex < i) {
            for (int j = i; j > endIndex; j--) {
                previousValue.Update(
                    ObjectFastOperator::FastGetPropertyByIndex<true>(thread, thisObjHandle.GetTaggedValue(), j - 1));
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
                ObjectFastOperator::FastSetPropertyByIndex(thread, thisObjHandle.GetTaggedValue(), j,
                                                           previousValue.GetTaggedValue());
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
            }
            ObjectFastOperator::FastSetPropertyByIndex(thread, thisObjHandle.GetTaggedValue(), endIndex,
                                                       presentValue.GetTaggedValue());
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        }
    }

    return thisObjHandle.GetTaggedValue();
}

void RuntimeStubs::StartCallTimer(uintptr_t argGlue, JSTaggedType func, bool isAot)
{
    auto thread =  JSThread::GlueToJSThread(argGlue);
    JSTaggedValue callTarget(func);
    Method *method = Method::Cast(JSFunction::Cast(callTarget)->GetMethod());
    if (method->IsNativeWithCallField()) {
        return;
    }
    size_t methodId = method->GetMethodId().GetOffset();
    auto callTimer = thread->GetEcmaVM()->GetCallTimer();
    callTimer->InitialStatAndTimer(method, methodId, isAot);
    callTimer->StartCount(methodId, isAot);
}

void RuntimeStubs::EndCallTimer(uintptr_t argGlue, JSTaggedType func)
{
    auto thread =  JSThread::GlueToJSThread(argGlue);
    JSTaggedValue callTarget(func);
    Method *method = Method::Cast(JSFunction::Cast(callTarget)->GetMethod());
    if (method->IsNativeWithCallField()) {
        return;
    }
    auto callTimer = thread->GetEcmaVM()->GetCallTimer();
    callTimer->StopCount(method);
}

DEF_RUNTIME_STUBS(FastStringify)
{
    RUNTIME_STUBS_HEADER(FastStringify);
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 0);
    base::FastJsonStringifier fastJsonStringifier(thread);
    JSHandle<JSTaggedValue> result = fastJsonStringifier.Stringify(value);
    return result.GetTaggedValue().GetRawData();
}

void RuntimeStubs::Initialize(JSThread *thread)
{
#define DEF_RUNTIME_STUB(name) kungfu::RuntimeStubCSigns::ID_##name
#define INITIAL_RUNTIME_FUNCTIONS(name) \
    thread->RegisterRTInterface(DEF_RUNTIME_STUB(name), reinterpret_cast<uintptr_t>(name));
    RUNTIME_STUB_WITHOUT_GC_LIST(INITIAL_RUNTIME_FUNCTIONS)
    RUNTIME_STUB_WITH_GC_LIST(INITIAL_RUNTIME_FUNCTIONS)
    TEST_RUNTIME_STUB_GC_LIST(INITIAL_RUNTIME_FUNCTIONS)
#undef INITIAL_RUNTIME_FUNCTIONS
#undef DEF_RUNTIME_STUB
}

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}  // namespace panda::ecmascript
