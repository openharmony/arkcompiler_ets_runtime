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


#include "ecmascript/stubs/runtime_optimized_stubs-inl.h"
#include "ecmascript/stubs/runtime_stubs-inl.h"
#include "ecmascript/base/json_stringifier.h"
#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/builtins/builtins_array.h"
#include "ecmascript/js_stable_array.h"
#include "ecmascript/builtins/builtins_bigint.h"
#include "ecmascript/builtins/builtins_function.h"
#include "ecmascript/builtins/builtins_iterator.h"
#include "ecmascript/builtins/builtins_reflect.h"
#include "ecmascript/builtins/builtins_string_iterator.h"
#include "ecmascript/compiler/builtins/containers_stub_builder.h"
#include "ecmascript/builtins/builtins_array.h"
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#include "ecmascript/dfx/stackinfo/js_stackinfo.h"
#include "ecmascript/dfx/vmstat/function_call_timer.h"
#include "ecmascript/dfx/vmstat/opt_code_profiler.h"
#include "ecmascript/ic/ic_runtime_stub-inl.h"
#include "ecmascript/interpreter/interpreter_assembly.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/jit/jit.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_string_iterator.h"
#include "ecmascript/js_stable_array.h"
#include "ecmascript/stubs/runtime_stubs.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/builtins/builtins_object.h"
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
    JSTaggedValue attr = GetArg(argv, argc, 3);  // 3: means the third parameter
    PropertyAttributes attrValue(attr);
    auto result = JSObject::AddElementInternal(thread, receiver, argIndex.GetInt(), value, attrValue);
    return JSTaggedValue(result).GetRawData();
}

DEF_RUNTIME_STUBS(InitializeGeneratorFunction)
{
    RUNTIME_STUBS_HEADER(InitializeGeneratorFunction);
    FunctionKind kind = static_cast<FunctionKind>(GetTArg(argv, argc, 0)); // 1: means the first parameter
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> objFun(env->GetObjectFunction());
    JSHandle<JSObject> initialGeneratorFuncPrototype = factory->NewJSObjectByConstructor(objFun);
    if (kind == FunctionKind::ASYNC_GENERATOR_FUNCTION) {
        JSObject::SetPrototype(thread, initialGeneratorFuncPrototype, env->GetAsyncGeneratorPrototype());
    } else if (kind == FunctionKind::GENERATOR_FUNCTION) {
        JSObject::SetPrototype(thread, initialGeneratorFuncPrototype, env->GetGeneratorPrototype());
    }
    return initialGeneratorFuncPrototype.GetTaggedType();
}

DEF_RUNTIME_STUBS(FunctionDefineOwnProperty)
{
    RUNTIME_STUBS_HEADER(FunctionDefineOwnProperty);
    JSHandle<JSFunction> func(GetHArg<JSTaggedValue>(argv, argc, 0));
    JSHandle<JSTaggedValue> accessor = GetHArg<JSTaggedValue>(argv, argc, 1); // 1: means the first parameter
    FunctionKind kind = static_cast<FunctionKind>(GetTArg(argv, argc, 2)); // 2: means the second parameter
    PropertyDescriptor desc(thread, accessor, kind != FunctionKind::BUILTIN_CONSTRUCTOR, false, false);
    JSObject::DefineOwnProperty(thread, JSHandle<JSObject>(func),
                                thread->GlobalConstants()->GetHandledPrototypeString(), desc);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(HeapAlloc)
{
    RUNTIME_STUBS_HEADER(HeapAlloc);
    JSTaggedValue allocateSize = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    auto size = static_cast<size_t>(allocateSize.GetLargeUInt());
    JSHandle<JSHClass> hclassHandle = GetHArg<JSHClass>(argv, argc, 1);  // 1: means the first parameter
    auto type = static_cast<RegionSpaceFlag>(GetArg(argv, argc, 2).GetInt());
    MemSpaceType mtype;
    switch (type) {
        case RegionSpaceFlag::IN_YOUNG_SPACE:
            mtype = MemSpaceType::SEMI_SPACE;
            break;
        case RegionSpaceFlag::IN_OLD_SPACE:
            mtype = MemSpaceType::OLD_SPACE;
            break;
        case RegionSpaceFlag::IN_NON_MOVABLE_SPACE:
            mtype = MemSpaceType::NON_MOVABLE;
            break;
        case RegionSpaceFlag::IN_SHARED_OLD_SPACE:
            mtype = MemSpaceType::SHARED_OLD_SPACE;
            break;
        case RegionSpaceFlag::IN_SHARED_NON_MOVABLE:
            mtype = MemSpaceType::SHARED_NON_MOVABLE;
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    auto hclass = JSHClass::Cast(hclassHandle.GetTaggedValue().GetTaggedObject());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto result = factory->AllocObjectWithSpaceType(size, hclass, mtype);
    return JSTaggedValue(result).GetRawData();
}

DEF_RUNTIME_STUBS(AllocateInYoung)
{
    RUNTIME_STUBS_HEADER(AllocateInYoung);
    JSTaggedValue allocateSize = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    auto size = static_cast<size_t>(allocateSize.GetLargeUInt());
    auto heap = const_cast<Heap*>(thread->GetEcmaVM()->GetHeap());
    auto result = heap->AllocateYoungOrHugeObject(size);
    ASSERT(result != nullptr);
    if (argc > 1) { // 1: means the first parameter
        JSHandle<JSHClass> hclassHandle = GetHArg<JSHClass>(argv, argc, 1);  // 1: means the first parameter
        auto hclass = JSHClass::Cast(hclassHandle.GetTaggedValue().GetTaggedObject());
        heap->SetHClassAndDoAllocateEvent(thread, result, hclass, size);
    }
    return JSTaggedValue(result).GetRawData();
}

DEF_RUNTIME_STUBS(AllocateInOld)
{
    RUNTIME_STUBS_HEADER(AllocateInOld);
    JSTaggedValue allocateSize = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    auto size = static_cast<size_t>(allocateSize.GetLargeUInt());
    auto heap = const_cast<Heap*>(thread->GetEcmaVM()->GetHeap());
    auto result = heap->AllocateOldOrHugeObject(size);
    ASSERT(result != nullptr);
    if (argc > 1) { // 1: means the first parameter
        JSHandle<JSHClass> hclassHandle = GetHArg<JSHClass>(argv, argc, 1);  // 1: means the first parameter
        auto hclass = JSHClass::Cast(hclassHandle.GetTaggedValue().GetTaggedObject());
        heap->SetHClassAndDoAllocateEvent(thread, result, hclass, size);
    }
    return JSTaggedValue(result).GetRawData();
}

#define ALLOCATE_IN_SHARED_HEAP(SPACE)                                                     \
    DEF_RUNTIME_STUBS(AllocateInS##SPACE)                                                  \
    {                                                                                      \
        RUNTIME_STUBS_HEADER(AllocateInS##SPACE);                                          \
        JSTaggedValue allocateSize = GetArg(argv, argc, 0);                                \
        auto size = static_cast<size_t>(allocateSize.GetInt());                            \
        auto sharedHeap = const_cast<SharedHeap*>(SharedHeap::GetInstance());              \
        ASSERT(size <= MAX_REGULAR_HEAP_OBJECT_SIZE);                                      \
        auto result = sharedHeap->Allocate##SPACE##OrHugeObject(thread, size);             \
        ASSERT(result != nullptr);                                                         \
        if (argc > 1) {                                                                    \
            JSHandle<JSHClass> hclassHandle = GetHArg<JSHClass>(argv, argc, 1);            \
            auto hclass = JSHClass::Cast(hclassHandle.GetTaggedValue().GetTaggedObject()); \
            sharedHeap->SetHClassAndDoAllocateEvent(thread, result, hclass, size);         \
        }                                                                                  \
        return JSTaggedValue(result).GetRawData();                                         \
    }

#undef ALLOCATE_IN_SHARED_HEAP

DEF_RUNTIME_STUBS(AllocateInSNonMovable)
{
    RUNTIME_STUBS_HEADER(AllocateInSNonMovable);
    JSTaggedValue allocateSize = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    auto size = static_cast<size_t>(allocateSize.GetInt());
    auto heap = const_cast<Heap*>(thread->GetEcmaVM()->GetHeap());
    auto result = heap->AllocateSharedNonMovableSpaceFromTlab(thread, size);
    if (result != nullptr) {
        return JSTaggedValue(result).GetRawData();
    }
    auto sharedHeap = const_cast<SharedHeap*>(SharedHeap::GetInstance());
    result = sharedHeap->AllocateNonMovableOrHugeObject(thread, size);
    ASSERT(result != nullptr);
    if (argc > 1) { // 1: means the first parameter
        JSHandle<JSHClass> hclassHandle = GetHArg<JSHClass>(argv, argc, 1);  // 1: means the first parameter
        auto hclass = JSHClass::Cast(hclassHandle.GetTaggedValue().GetTaggedObject());
        sharedHeap->SetHClassAndDoAllocateEvent(thread, result, hclass, size);
    }
    return JSTaggedValue(result).GetRawData();
}

DEF_RUNTIME_STUBS(DefineOwnProperty)
{
    RUNTIME_STUBS_HEADER(DefineOwnProperty);
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 0);
    JSHandle<JSTaggedValue> key = GetHArg<JSTaggedValue>(argv, argc, 1); // 1: means the first parameter
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 2); // 2: means the second parameter
    PropertyDescriptor desc(thread, value, true, true, true);
    bool res = JSTaggedValue::DefineOwnProperty(thread, obj, key, desc);
    return JSTaggedValue(res).GetRawData();
}

DEF_RUNTIME_STUBS(AllocateInSOld)
{
    RUNTIME_STUBS_HEADER(AllocateInSOld);
    JSTaggedValue allocateSize = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    auto size = static_cast<size_t>(allocateSize.GetInt());
    auto heap = const_cast<Heap*>(thread->GetEcmaVM()->GetHeap());
    auto result = heap->AllocateSharedOldSpaceFromTlab(thread, size);
    if (result != nullptr) {
        return JSTaggedValue(result).GetRawData();
    }
    auto sharedHeap = const_cast<SharedHeap*>(SharedHeap::GetInstance());
    result = sharedHeap->AllocateOldOrHugeObject(thread, size);
    ASSERT(result != nullptr);
    if (argc > 1) { // 1: means the first parameter
        JSHandle<JSHClass> hclassHandle = GetHArg<JSHClass>(argv, argc, 1);  // 1: means the first parameter
        auto hclass = JSHClass::Cast(hclassHandle.GetTaggedValue().GetTaggedObject());
        sharedHeap->SetHClassAndDoAllocateEvent(thread, result, hclass, size);
    }
    return JSTaggedValue(result).GetRawData();
}

DEF_RUNTIME_STUBS(TypedArraySpeciesCreate)
{
    RUNTIME_STUBS_HEADER(TypedArraySpeciesCreate);
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 0);    // 0: means the zeroth parameter
    JSHandle<JSTypedArray> thisObj(obj);
    JSTaggedValue indexValue = GetArg(argv, argc, 1);   // 1: means the first parameter
    uint32_t index = static_cast<uint32_t>(indexValue.GetInt());
    JSTaggedValue arrayLen = GetArg(argv, argc, 2); // 2: means the second parameter
    uint32_t length = static_cast<uint32_t>(arrayLen.GetInt());
    JSTaggedType args[1] = {JSTaggedValue(length).GetRawData()};
    JSHandle<JSObject> newArr = base::TypedArrayHelper::TypedArraySpeciesCreate(thread, thisObj, index, args);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
    return newArr.GetTaggedValue().GetRawData();
}

void RuntimeStubs::CopyTypedArrayBuffer(JSTypedArray *srcArray, JSTypedArray *targetArray, int32_t srcStartPos,
    int32_t tarStartPos, int32_t count, int32_t elementSize)
{
    DISALLOW_GARBAGE_COLLECTION;
    if (count <= 0) {
        return;
    }
    JSTaggedValue srcBuffer = srcArray->GetViewedArrayBufferOrByteArray();
    JSTaggedValue targetBuffer = targetArray->GetViewedArrayBufferOrByteArray();
    uint32_t srcByteIndex = static_cast<uint32_t>(srcStartPos * elementSize) + srcArray->GetByteOffset();
    uint32_t targetByteIndex = static_cast<uint32_t>(tarStartPos * elementSize) + targetArray->GetByteOffset();
    uint8_t *srcBuf = (uint8_t *)builtins::BuiltinsArrayBuffer::GetDataPointFromBuffer(srcBuffer, srcByteIndex);
    uint8_t *targetBuf = (uint8_t *)builtins::BuiltinsArrayBuffer::GetDataPointFromBuffer(targetBuffer,
                                                                                          targetByteIndex);
    if (memmove_s(targetBuf, elementSize * count, srcBuf, elementSize * count) != EOK) {
        LOG_FULL(FATAL) << "memmove_s failed";
        UNREACHABLE();
    }
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
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 2); // 2: means the second parameter
    auto setter = AccessorData::Cast((reinterpret_cast<TaggedObject *>(argSetter)));
    auto result = setter->CallInternalSet(thread, receiver, value, true);
    if (!result) {
        return JSTaggedValue::Exception().GetRawData();
    }
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
    uint32_t result = EcmaStringAccessor(string).ComputeHashcode();
    return JSTaggedValue(static_cast<uint64_t>(result)).GetRawData();
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

DEF_RUNTIME_STUBS(RTSubstitution)
{
    RUNTIME_STUBS_HEADER(RTSubstitution);
    JSHandle<EcmaString> matched = GetHArg<EcmaString>(argv, argc, 0);     // 0: means the zeroth parameter
    JSHandle<EcmaString> srcString = GetHArg<EcmaString>(argv, argc, 1);   // 1: means the first parameter
    int position = GetArg(argv, argc, 2).GetInt();                         // 2: means the second parameter
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> captureList = factory->EmptyArray();
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSHandle<EcmaString> replacement = GetHArg<EcmaString>(argv, argc, 3); // 3: means the third parameter
    JSTaggedValue result = builtins::BuiltinsString::GetSubstitution(thread, matched, srcString, position,
        captureList, undefined, replacement);
    return result.GetRawData();
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

    PropertyAttributes propAttr(attr);
    if (needTransToDict.IsTrue()) {
        JSHandle<JSObject> objHandle(thread, JSTaggedValue(reinterpret_cast<TaggedObject *>(receiver)));
        JSHandle<NameDictionary> dictHandle(JSObject::TransitionToDictionary(thread, objHandle));
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
        return NameDictionary::
            PutIfAbsent(thread, dictHandle, keyHandle, valueHandle, propAttr).GetTaggedValue().GetRawData();
    } else {
        JSHandle<NameDictionary> dictHandle(thread, JSTaggedValue(reinterpret_cast<TaggedObject *>(array)));
        return NameDictionary::
            PutIfAbsent(thread, dictHandle, keyHandle, valueHandle, propAttr).GetTaggedValue().GetRawData();
    }
}

DEF_RUNTIME_STUBS(NumberDictionaryPut)
{
    RUNTIME_STUBS_HEADER(NumberDictionaryPut);
    JSTaggedType receiver = GetTArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedType array = GetTArg(argv, argc, 1);  // 1: means the first parameter
    JSTaggedValue key = GetArg(argv, argc, 2);  // 2: means the second parameter
    JSHandle<JSTaggedValue> valueHandle = GetHArg<JSTaggedValue>(argv, argc, 3);  // 3: means the third parameter
    JSTaggedValue attr = GetArg(argv, argc, 4);   // 4: means the fourth parameter
    JSTaggedValue needTransToDict = GetArg(argv, argc, 5);  // 5: means the fifth parameter

    JSHandle<JSTaggedValue> keyHandle(thread, key);
    PropertyAttributes propAttr(attr);
    JSHandle<JSObject> objHandle(thread, JSTaggedValue(reinterpret_cast<TaggedObject *>(receiver)));
    if (needTransToDict.IsTrue()) {
        JSObject::ElementsToDictionary(thread, objHandle);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
        JSHandle<NumberDictionary> dict(thread, objHandle->GetElements());
        return NumberDictionary::Put(thread, dict, keyHandle, valueHandle, propAttr).GetTaggedValue().GetRawData();
    } else {
        JSHandle<NumberDictionary> dict(thread, JSTaggedValue(reinterpret_cast<TaggedObject *>(array)));
        return NumberDictionary::Put(thread, dict, keyHandle, valueHandle, propAttr).GetTaggedValue().GetRawData();
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
        uint32_t maxNonInlinedFastPropsCapacity = objHandle->GetNonInlinedFastPropsCapacity();
        uint32_t newLen = JSObject::ComputeNonInlinedFastPropsCapacity(thread, capacity,
                                                                       maxNonInlinedFastPropsCapacity);
        properties = factory->CopyArray(arrayHandle, capacity, newLen);
    }
    properties->Set(thread, index, valueHandle);
    objHandle->SetProperties(thread, properties);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(CheckAndCopyArray)
{
    RUNTIME_STUBS_HEADER(CheckAndCopyArray);
    JSTaggedType argReceiver = GetTArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSArray> receiverHandle(thread, reinterpret_cast<JSArray *>(argReceiver));
    JSArray::CheckAndCopyArray(thread, receiverHandle);
    return receiverHandle->GetElements().GetRawData();
}

DEF_RUNTIME_STUBS(JSArrayReduceUnStable)
{
    RUNTIME_STUBS_HEADER(JSArrayReduceUnStable);
    JSHandle<JSTaggedValue> thisHandle = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> thisObjVal = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the one parameter
    JSTaggedType taggedValueK = GetTArg(argv, argc, 2);  // 2: means the two parameter
    int64_t k = JSTaggedNumber(JSTaggedValue(taggedValueK)).GetNumber();
    JSTaggedType taggedValueLen = GetTArg(argv, argc, 3);  // 3: means the three parameter
    int64_t len = JSTaggedNumber(JSTaggedValue(taggedValueLen)).GetNumber();
    JSMutableHandle<JSTaggedValue> accumulator = JSMutableHandle<JSTaggedValue>(thread,
        GetHArg<JSTaggedValue>(argv, argc, 4));  // 4: means the four parameter
    JSHandle<JSTaggedValue> callbackFnHandle = GetHArg<JSTaggedValue>(argv, argc, 5);  // 5: means the five parameter

    JSTaggedValue ret = builtins::BuiltinsArray::ReduceUnStableJSArray(thread, thisHandle, thisObjVal, k, len,
        accumulator, callbackFnHandle);
    return ret.GetRawData();
}

DEF_RUNTIME_STUBS(JSObjectGrowElementsCapacity)
{
    RUNTIME_STUBS_HEADER(JSObjectGrowElementsCapacity);
    JSHandle<JSObject> elements = GetHArg<JSObject>(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue length = GetArg(argv, argc, 1);                    // 1: means the zeroth parameter
    uint32_t newLength = static_cast<uint32_t>(length.GetInt());
    JSHandle<TaggedArray> newElements = JSObject::GrowElementsCapacity(thread, elements, newLength, true);
    return newElements.GetTaggedValue().GetRawData();
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

DEF_RUNTIME_STUBS(JSArrayFilterUnStable)
{
    RUNTIME_STUBS_HEADER(JSArrayFilterUnStable);
    JSHandle<JSTaggedValue> thisArgHandle = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> thisObjVal = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the one parameter
    JSTaggedType taggedValueK = GetTArg(argv, argc, 2);  // 2: means the two parameter
    int64_t k = JSTaggedNumber(JSTaggedValue(taggedValueK)).GetNumber();
    JSTaggedType taggedValueLen = GetTArg(argv, argc, 3);  // 3: means the three parameter
    int64_t len = JSTaggedNumber(JSTaggedValue(taggedValueLen)).GetNumber();
    JSTaggedType toIndexValue = GetTArg(argv, argc, 4);  // 4: means the three parameter
    int32_t toIndex = JSTaggedNumber(JSTaggedValue(toIndexValue)).GetNumber();
    JSHandle<JSObject> newArrayHandle = JSMutableHandle<JSObject>(thread,
        GetHArg<JSObject>(argv, argc, 5));  // 5: means the four parameter
    JSHandle<JSTaggedValue> callbackFnHandle = GetHArg<JSTaggedValue>(argv, argc, 6);  // 6: means the five parameter

    JSTaggedValue ret = builtins::BuiltinsArray::FilterUnStableJSArray(thread, thisArgHandle, thisObjVal, k, len,
        toIndex, newArrayHandle, callbackFnHandle);
    return ret.GetRawData();
}

DEF_RUNTIME_STUBS(JSArrayMapUnStable)
{
    RUNTIME_STUBS_HEADER(JSArrayMapUnStable);
    JSHandle<JSTaggedValue> thisArgHandle = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> thisObjVal = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the one parameter
    JSTaggedType taggedValueK = GetTArg(argv, argc, 2);  // 2: means the two parameter
    int64_t k = JSTaggedNumber(JSTaggedValue(taggedValueK)).GetNumber();
    JSTaggedType taggedValueLen = GetTArg(argv, argc, 3);  // 3: means the three parameter
    int64_t len = JSTaggedNumber(JSTaggedValue(taggedValueLen)).GetNumber();
    JSHandle<JSObject> newArrayHandle =
        JSMutableHandle<JSObject>(thread, GetHArg<JSObject>(argv, argc, 4));  // 4: means the four parameter
    JSHandle<JSTaggedValue> callbackFnHandle = GetHArg<JSTaggedValue>(argv, argc, 5);  // 5: means the five parameter

    JSTaggedValue ret = builtins::BuiltinsArray::MapUnStableJSArray(thread, thisArgHandle, thisObjVal, k, len,
        newArrayHandle, callbackFnHandle);
    return ret.GetRawData();
}

DEF_RUNTIME_STUBS(UpdateLayOutAndAddTransition)
{
    RUNTIME_STUBS_HEADER(UpdateLayOutAndAddTransition);
    JSHandle<JSHClass> oldHClassHandle = GetHArg<JSHClass>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSHClass> newHClassHandle = GetHArg<JSHClass>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> keyHandle = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    JSTaggedValue attr = GetArg(argv, argc, 3);  // 3: means the third parameter

    PropertyAttributes attrValue(attr);

    JSHClass::AddPropertyToNewHClass(thread, oldHClassHandle, newHClassHandle, keyHandle, attrValue);

    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(CopyAndUpdateObjLayout)
{
    RUNTIME_STUBS_HEADER(CopyAndUpdateObjLayout);
    JSHandle<JSHClass> newHClassHandle = GetHArg<JSHClass>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> keyHandle = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    JSTaggedValue attr = GetArg(argv, argc, 3);  // 3: means the third parameter

    auto factory = thread->GetEcmaVM()->GetFactory();
    PropertyAttributes attrValue(attr);

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
    ASSERT(receiver->IsJSArray());
    ElementsKind kind = Elements::FixElementsKind(static_cast<ElementsKind>(elementsKind));
    auto array = JSHandle<JSArray>(receiver);
    ASSERT(JSHClass::IsInitialArrayHClassWithElementsKind(thread, receiver->GetTaggedObject()->GetClass(),
                                                          receiver->GetTaggedObject()->GetClass()->GetElementsKind()));
    if (!JSHClass::TransitToElementsKindUncheck(thread, JSHandle<JSObject>(array), kind)) {
        return JSTaggedValue::Hole().GetRawData();
    }

    if (thread->IsEnableElementsKind() || thread->IsPGOProfilerEnable()) {
        // Update TrackInfo
        JSHandle<JSArray>(receiver)->UpdateTrackInfo(thread);
    }

    if (!thread->IsPGOProfilerEnable()) {
        return JSTaggedValue::Hole().GetRawData();
    }
    JSTaggedValue trackInfoVal = JSHandle<JSArray>(receiver)->GetTrackInfo();
    if (trackInfoVal.IsHeapObject() && trackInfoVal.IsWeak()) {
        TrackInfo *trackInfo = TrackInfo::Cast(trackInfoVal.GetWeakReferentUnChecked());
        thread->GetEcmaVM()->GetPGOProfiler()->UpdateTrackInfo(JSTaggedValue(trackInfo));
    }
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(NewMutantTaggedArray)
{
    RUNTIME_STUBS_HEADER(NewMutantTaggedArray);
    JSTaggedValue length = GetArg(argv, argc, 0);  // 0: means the zeroth parameter

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->NewMutantTaggedArray(length.GetInt()).GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(NewCOWMutantTaggedArray)
{
    RUNTIME_STUBS_HEADER(NewCOWMutantTaggedArray);
    JSTaggedValue length = GetArg(argv, argc, 0);  // 0: means the zeroth parameter

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->NewCOWMutantTaggedArray(length.GetInt()).GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(NewCOWTaggedArray)
{
    RUNTIME_STUBS_HEADER(NewCOWTaggedArray);
    JSTaggedValue length = GetArg(argv, argc, 0);  // 0: means the zeroth parameter

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->NewCOWMutantTaggedArray(length.GetInt()).GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(ForceGC)
{
    RUNTIME_STUBS_HEADER(ForceGC);
    if (!thread->GetEcmaVM()->GetJSOptions().EnableForceGC()) {
        return JSTaggedValue::Hole().GetRawData();
    }
    thread->GetEcmaVM()->CollectGarbage(TriggerGCType::FULL_GC);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(RuntimeDump)
{
    RUNTIME_STUBS_HEADER(RuntimeDump);
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(GetHArg<JSTaggedValue>(argv, argc, 0));
    {
        std::ostringstream oss;
        obj->Dump(oss);
        LOG_ECMA(ERROR) << "RuntimeDump: " << oss.str();
    }

    LOG_ECMA(ERROR) << "---------- before force gc ---------------";
    {
        thread->GetEcmaVM()->CollectGarbage(TriggerGCType::FULL_GC);
    }
    LOG_ECMA(ERROR) << "---------- end force gc ---------------";
    return JSTaggedValue::Hole().GetRawData();
}

void RuntimeStubs::Dump(JSTaggedType rawValue)
{
    DISALLOW_GARBAGE_COLLECTION;
    std::ostringstream oss;
    auto value = JSTaggedValue(rawValue);
    value.Dump(oss);
    LOG_ECMA(INFO) << "dump log for read-only crash " << oss.str();
}

void RuntimeStubs::DebugDump(JSTaggedType rawValue)
{
    DISALLOW_GARBAGE_COLLECTION;
    DebugDumpWithHint(reinterpret_cast<uintptr_t>(nullptr), rawValue);
}

void RuntimeStubs::DumpWithHint(uintptr_t hintStrAddress, JSTaggedType rawValue)
{
    DISALLOW_GARBAGE_COLLECTION;
    const char *origHintStr = reinterpret_cast<const char*>(hintStrAddress); // May be nullptr
    const char *hintStr = (origHintStr == nullptr) ? "" : origHintStr;
    DumpToStreamWithHint(std::cout, hintStr, JSTaggedValue(rawValue));
    std::cout << std::endl; // New line
}

void RuntimeStubs::DebugDumpWithHint(uintptr_t hintStrAddress, JSTaggedType rawValue)
{
    DISALLOW_GARBAGE_COLLECTION;
    const char *origHintStr = reinterpret_cast<const char*>(hintStrAddress); // May be nullptr
    const char *hintStr = (origHintStr == nullptr) ? "" : origHintStr;
    // The immediate lambda expression call is not evaluated when the logger is unabled.
    LOG_ECMA(DEBUG) << [](const char *hintStr, JSTaggedType rawValue) {
        std::ostringstream out;
        DumpToStreamWithHint(out, hintStr, JSTaggedValue(rawValue));
        return out.str();
    }(hintStr, rawValue);
}

void RuntimeStubs::DumpToStreamWithHint(std::ostream &out, std::string_view hint, JSTaggedValue value)
{
    constexpr std::string_view dumpDelimiterLine = "================";
    // Begin line
    out << dumpDelimiterLine << " Begin dump: " << hint << ' ' << dumpDelimiterLine << std::endl;
    // Dumps raw data
    out << "(Raw value = 0x" << std::setw(base::INT64_HEX_DIGITS) << std::hex
        << std::setfill('0') << value.GetRawData() << ") ";
    out << std::dec << std::setfill(' '); // Recovers integer radix & fill character
    // Dumps tagged value
    value.Dump(out);
    // End line
    out << dumpDelimiterLine << "   End dump: " << hint << ' ' << dumpDelimiterLine;
}

void RuntimeStubs::DebugPrint(int fmtMessageId, ...)
{
    std::string format = MessageString::GetMessageString(fmtMessageId);
    va_list args;
    va_start(args, fmtMessageId);
    std::string result = base::StringHelper::Vformat(format.c_str(), args);
    if (MessageString::IsBuiltinsStubMessageString(fmtMessageId)) {
        LOG_BUILTINS(DEBUG) << result;
    } else {
        LOG_ECMA(DEBUG) << result;
    }
    va_end(args);
}

void RuntimeStubs::DebugPrintCustom(uintptr_t fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::string result = base::StringHelper::Vformat(reinterpret_cast<const char*>(fmt), args);
    LOG_ECMA(DEBUG) << result;
    va_end(args);
}

void RuntimeStubs::DebugPrintInstruction([[maybe_unused]] uintptr_t argGlue, const uint8_t *pc)
{
    BytecodeInstruction inst(pc);
    LOG_INTERPRETER(DEBUG) << inst;
}

void RuntimeStubs::CollectingOpcodes([[maybe_unused]] uintptr_t argGlue, const uint8_t *pc)
{
#if ECMASCRIPT_ENABLE_COLLECTING_OPCODES
    BytecodeInstruction inst(pc);
    auto thread = JSThread::GlueToJSThread(argGlue);
    auto vm = thread->GetEcmaVM();
    auto opcode = inst.GetOpcode();
    std::stack<std::unordered_map<BytecodeInstruction::Opcode, int>> &bytecodeStatsStack_ =
            vm->GetBytecodeStatsStack();
    if (bytecodeStatsStack_.empty()) {
        return;
    }
    std::unordered_map<BytecodeInstruction::Opcode, int> &bytecodeStatsMap_ = bytecodeStatsStack_.top();
    auto foundInst = bytecodeStatsMap_.find(opcode);
    if (foundInst != bytecodeStatsMap_.end()) {
        ++foundInst->second;
    } else {
        bytecodeStatsMap_[opcode] = 1;
    }
    LOG_INTERPRETER(DEBUG) << inst;
#endif
}

void RuntimeStubs::DebugOsrEntry([[maybe_unused]] uintptr_t argGlue, const uint8_t *codeEntry)
{
    LOG_JIT(DEBUG) << "[OSR]: Enter OSR Code: " << reinterpret_cast<const void*>(codeEntry);
}

void RuntimeStubs::Comment(uintptr_t argStr)
{
    std::string str(reinterpret_cast<char *>(argStr));
    LOG_ECMA(DEBUG) << str;
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

DEF_RUNTIME_STUBS(GetNativePcOfstForBaseline)
{
    RUNTIME_STUBS_HEADER(GetNativePcOfstForBaseline);
    JSHandle<JSFunction> func = GetHArg<JSFunction>(argv, argc, 0);  // 0: means the zeroth parameter
    uint64_t bytecodePc = static_cast<uint64_t>(GetTArg(argv, argc, 1));  // 1: means the first parameter
    return RuntimeGetNativePcOfstForBaseline(func, bytecodePc);
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

DEF_RUNTIME_STUBS(RegularJSObjDeletePrototype)
{
    RUNTIME_STUBS_HEADER(RegularJSObjDeletePrototype);
    JSHandle<JSObject> tagged = GetHArg<JSObject>(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue value = GetArg(argv, argc, 1);
    uint32_t index = 0;
    if (value.IsString()) {
        auto string = JSHandle<EcmaString>(thread, value);
        if (EcmaStringAccessor(string).ToElementIndex(&index)) {
            value = JSTaggedValue(index);
        } else if (!EcmaStringAccessor(string).IsInternString()) {
            JSTaggedValue key(RuntimeTryGetInternString(argGlue, string));
            if (key.IsHole()) {
                return JSTaggedValue::True().GetRawData();
            } else {
                value = key;
            }
        }
    }
    auto result = JSObject::DeleteProperty(thread, tagged, JSHandle<JSTaggedValue>(thread, value));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
    if (!result) {
        auto factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR, "Cannot delete property", StackCheck::NO);
        thread->SetException(error.GetTaggedValue());
        return JSTaggedValue::Exception().GetRawData();
    }
    return JSTaggedValue::True().GetRawData();
}

DEF_RUNTIME_STUBS(CallJSObjDeletePrototype)
{
    RUNTIME_STUBS_HEADER(CallJSObjDeletePrototype);
    JSHandle<JSTaggedValue> tagged = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 1);
    auto result = JSTaggedValue::DeleteProperty(thread, tagged, value);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
    if (!result) {
        auto factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR, "Cannot delete property", StackCheck::NO);
        thread->SetException(error.GetTaggedValue());
        return JSTaggedValue::Exception().GetRawData();
    }
    return JSTaggedValue::True().GetRawData();
}

DEF_RUNTIME_STUBS(ToPropertyKey)
{
    RUNTIME_STUBS_HEADER(ToPropertyKey);
    JSHandle<JSTaggedValue> key = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue res = JSTaggedValue::ToPropertyKey(thread, key).GetTaggedValue();
    return res.GetRawData();
}

DEF_RUNTIME_STUBS(ToPropertyKeyValue)
{
    RUNTIME_STUBS_HEADER(ToPropertyKeyValue);
    std::string string_key = "value";
    JSHandle<JSTaggedValue> key = JSHandle<JSTaggedValue>(thread,
        base::BuiltinsBase::GetTaggedString(thread, string_key.c_str()));
    JSTaggedValue res = JSTaggedValue::ToPropertyKey(thread, key).GetTaggedValue();
    return res.GetRawData();
}

DEF_RUNTIME_STUBS(ToPropertyKeyWritable)
{
    RUNTIME_STUBS_HEADER(ToPropertyKeyWritable);
    std::string string_key = "writable";
    JSHandle<JSTaggedValue> key = JSHandle<JSTaggedValue>(thread,
        base::BuiltinsBase::GetTaggedString(thread, string_key.c_str()));
    JSTaggedValue res = JSTaggedValue::ToPropertyKey(thread, key).GetTaggedValue();
    return res.GetRawData();
}

DEF_RUNTIME_STUBS(ToPropertyKeyEnumerable)
{
    RUNTIME_STUBS_HEADER(ToPropertyKeyEnumerable);
    std::string string_key = "enumerable";
    JSHandle<JSTaggedValue> key = JSHandle<JSTaggedValue>(thread,
        base::BuiltinsBase::GetTaggedString(thread, string_key.c_str()));
    JSTaggedValue res = JSTaggedValue::ToPropertyKey(thread, key).GetTaggedValue();
    return res.GetRawData();
}

DEF_RUNTIME_STUBS(ToPropertyKeyConfigurable)
{
    RUNTIME_STUBS_HEADER(ToPropertyKeyConfigurable);
    std::string string_key = "configurable";
    JSHandle<JSTaggedValue> key = JSHandle<JSTaggedValue>(thread,
        base::BuiltinsBase::GetTaggedString(thread, string_key.c_str()));
    JSTaggedValue res = JSTaggedValue::ToPropertyKey(thread, key).GetTaggedValue();
    return res.GetRawData();
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
        if ((std::abs(doubleBase) == 1 && std::isinf(doubleExponent)) || std::isnan(doubleExponent)) {
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

DEF_RUNTIME_STUBS(DumpObject)
{
    RUNTIME_STUBS_HEADER(DumpObject);
    JSHandle<JSTaggedValue> target = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> targetId = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    LOG_ECMA(INFO) << "InstanceOf Stability Testing Num: " << targetId->GetInt();
    std::ostringstream oss;
    target->Dump(oss);
    LOG_ECMA(INFO) << "dump log for instance of target: " << oss.str();
    return JSTaggedValue::True().GetRawData();
}

DEF_RUNTIME_STUBS(BigIntConstructor)
{
    RUNTIME_STUBS_HEADER(BigIntConstructor);
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 0);
    return builtins::BuiltinsBigInt::BigIntConstructorInternal(thread, value).GetRawData();
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

DEF_RUNTIME_STUBS(CreateStringIterator)
{
    RUNTIME_STUBS_HEADER(CreateStringIterator);
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return JSStringIterator::CreateStringIterator(thread, JSHandle<EcmaString>(obj)).GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(NewJSArrayIterator)
{
    RUNTIME_STUBS_HEADER(NewJSArrayIterator);
    JSHandle<JSObject> obj = GetHArg<JSObject>(argv, argc, 0);  // 0: means the zeroth parameter
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->NewJSArrayIterator(obj, IterationKind::VALUE).GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(NewJSTypedArrayIterator)
{
    RUNTIME_STUBS_HEADER(NewJSArrayIterator);
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    base::TypedArrayHelper::ValidateTypedArray(thread, obj);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSArrayIterator> iter(factory->NewJSArrayIterator(JSHandle<JSObject>(obj), IterationKind::VALUE));
    return iter.GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(MapIteratorNext)
{
    RUNTIME_STUBS_HEADER(MapIteratorNext);
    JSHandle<JSTaggedValue> thisObj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return JSMapIterator::NextInternal(thread, thisObj).GetRawData();
}

DEF_RUNTIME_STUBS(SetIteratorNext)
{
    RUNTIME_STUBS_HEADER(SetIteratorNext);
    JSHandle<JSTaggedValue> thisObj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return JSSetIterator::NextInternal(thread, thisObj).GetRawData();
}

DEF_RUNTIME_STUBS(StringIteratorNext)
{
    RUNTIME_STUBS_HEADER(StringIteratorNext);
    JSHandle<JSTaggedValue> thisObj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return builtins::BuiltinsStringIterator::NextInternal(thread, thisObj).GetRawData();
}

DEF_RUNTIME_STUBS(ArrayIteratorNext)
{
    RUNTIME_STUBS_HEADER(ArrayIteratorNext);
    JSHandle<JSTaggedValue> thisObj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return JSArrayIterator::NextInternal(thread, thisObj).GetRawData();
}

DEF_RUNTIME_STUBS(IteratorReturn)
{
    RUNTIME_STUBS_HEADER(IteratorReturn);
    JSHandle<JSTaggedValue> thisObj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return builtins::BuiltinsIterator::ReturnInternal(thread, thisObj).GetRawData();
}

DEF_RUNTIME_STUBS(GetNextPropName)
{
    RUNTIME_STUBS_HEADER(GetNextPropName);
    JSHandle<JSTaggedValue> iter = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeGetNextPropName(thread, iter).GetRawData();
}

DEF_RUNTIME_STUBS(GetNextPropNameSlowpath)
{
    RUNTIME_STUBS_HEADER(GetNextPropNameSlowpath);
    JSHandle<JSTaggedValue> iter = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    ASSERT(iter->IsForinIterator());
    JSTaggedValue res = JSForInIterator::NextInternalSlowpath(thread, JSHandle<JSForInIterator>::Cast(iter));
    return res.GetRawData();
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
    JSHandle<JSTaggedValue> func = GetHArg<JSTaggedValue>(argv, argc, 0); // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> newTarget = GetHArg<JSTaggedValue>(argv, argc, 1); // 1: means the first parameter
    JSHandle<JSTaggedValue> taggedArray = GetHArg<JSTaggedValue>(argv, argc, 2); // 2: means the second parameter
    return RuntimeOptSuperCallSpread(thread, func, newTarget, taggedArray).GetRawData();
}

DEF_RUNTIME_STUBS(SuperCallForwardAllArgs)
{
    RUNTIME_STUBS_HEADER(SuperCallForwardAllArgs);
    auto sp = const_cast<JSTaggedType *>(thread->GetCurrentInterpretedFrame());
    JSHandle<JSTaggedValue> func = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: index of child constructor
    JSHandle<JSTaggedValue> superFunc(thread, JSTaggedValue::GetPrototype(thread, func));
    auto newTarget = JSHandle<JSTaggedValue>(thread, InterpreterAssembly::GetNewTarget(sp));
    uint32_t startIdx = 0;
    uint32_t restNumArgs = InterpreterAssembly::GetNumArgs(sp, 0, startIdx);  // 0: rest args start idx
    return RuntimeSuperCallForwardAllArgs(thread, sp, superFunc, newTarget, restNumArgs, startIdx).GetRawData();
}

DEF_RUNTIME_STUBS(OptSuperCallForwardAllArgs)
{
    RUNTIME_STUBS_HEADER(OptSuperCallForwardAllArgs);
    JSTaggedType *sp = reinterpret_cast<JSTaggedType *>(GetActualArgv(thread));
    JSHandle<JSTaggedValue> superFunc = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: index of super constructor
    JSHandle<JSTaggedValue> newTarget = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: index of newTarget
    int actualArgc = GetArg(argv, argc, 2).GetInt();                            // 2: index of actual argc
    ASSERT(actualArgc >= 0);
    uint32_t convertedActualArgc = static_cast<uint32_t>(actualArgc);
    ASSERT(convertedActualArgc >= NUM_MANDATORY_JSFUNC_ARGS);
    uint32_t restNumArgs = convertedActualArgc - NUM_MANDATORY_JSFUNC_ARGS;
    uint32_t startIdx = NUM_MANDATORY_JSFUNC_ARGS;
    return RuntimeSuperCallForwardAllArgs(thread, sp, superFunc, newTarget, restNumArgs, startIdx).GetRawData();
}

DEF_RUNTIME_STUBS(GetCallSpreadArgs)
{
    RUNTIME_STUBS_HEADER(GetCallSpreadArgs);
    JSHandle<JSTaggedValue> jsArray = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeGetCallSpreadArgs(thread, jsArray).GetRawData();
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

    JSTaggedValue::RequireObjectCoercible(thread, receiver, "Cannot load property of null or undefined");
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());

    if (profileTypeInfo->IsUndefined()) {
        return RuntimeLdObjByValue(thread, receiver, key, false, JSTaggedValue::Undefined()).GetRawData();
    }
    JSHandle<JSTaggedValue> propKey = JSTaggedValue::ToPropertyKey(thread, key);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
    LoadICRuntime icRuntime(thread, JSHandle<ProfileTypeInfo>::Cast(profileTypeInfo), slotId.GetInt(), ICKind::LoadIC);
    return icRuntime.LoadValueMiss(receiver, propKey).GetRawData();
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

DEF_RUNTIME_STUBS(StoreOwnICByValue)
{
    RUNTIME_STUBS_HEADER(StoreOwnICByValue);
    JSHandle<JSTaggedValue> profileTypeInfo = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> receiver = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> key = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 3);  // 3: means the third parameter
    JSTaggedValue slotId = GetArg(argv, argc, 4);   // 4: means the fourth parameter
    if (profileTypeInfo->IsUndefined()) {
        return RuntimeStOwnByIndex(thread, receiver, key, value).GetRawData();
    }
    JSHandle<JSTaggedValue> propKey = JSTaggedValue::ToPropertyKey(thread, key);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
    StoreICRuntime icRuntime(thread, JSHandle<ProfileTypeInfo>::Cast(profileTypeInfo), slotId.GetInt(),
                             ICKind::StoreIC);
    return icRuntime.StoreMiss(receiver, propKey, value, true).GetRawData();
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
    JSTaggedValue cp = thread->GetCurrentEcmaContext()->FindOrCreateUnsharedConstpool(constpool.GetTaggedValue());
    return ConstantPool::GetLiteralFromCache<ConstPoolType::OBJECT_LITERAL>(
        thread, cp, index.GetInt(), module.GetTaggedValue()).GetRawData();
}

DEF_RUNTIME_STUBS(GetArrayLiteralFromCache)
{
    RUNTIME_STUBS_HEADER(GetArrayLiteralFromCache);
    JSHandle<JSTaggedValue> constpool = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue index = GetArg(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> module = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    JSTaggedValue cp = thread->GetCurrentEcmaContext()->FindOrCreateUnsharedConstpool(constpool.GetTaggedValue());
    return ConstantPool::GetLiteralFromCache<ConstPoolType::ARRAY_LITERAL>(
        thread, cp, index.GetInt(), module.GetTaggedValue()).GetRawData();
}

DEF_RUNTIME_STUBS(StObjByValue)
{
    RUNTIME_STUBS_HEADER(StObjByValue);
    JSHandle<JSTaggedValue> receiver = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> key = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    return RuntimeStObjByValue(thread, receiver, key, value).GetRawData();
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
    JSHandle<JSTaggedValue> length = GetHArg<JSTaggedValue>(argv, argc, 6);  // 6: means the sixth parameter

    auto res = RuntimeCreateClassWithBuffer(thread, base, lexenv, constpool,
                                            static_cast<uint16_t>(methodId.GetInt()),
                                            static_cast<uint16_t>(literalId.GetInt()),
                                            module, length);
#if ECMASCRIPT_ENABLE_IC
    const uint32_t INDEX_OF_SLOT_ID = 7; // 7: index of slotId in argv
    if (argc > INDEX_OF_SLOT_ID) {
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
        uint16_t slotId = static_cast<uint16_t>(GetArg(argv, argc, INDEX_OF_SLOT_ID).GetInt());
        const uint32_t INDEX_OF_JS_FUNC = 8;  // 8: index of jsFunc in argv
        ASSERT(argc > INDEX_OF_JS_FUNC);
        JSHandle<JSFunction> jsFuncHandle = GetHArg<JSFunction>(argv, argc, INDEX_OF_JS_FUNC);
        JSHandle<JSFunction> resHandle(thread, res);
        SetProfileTypeInfoCellToFunction(thread, jsFuncHandle, resHandle, slotId);
        res = resHandle.GetTaggedValue();
    }
#endif
    return res.GetRawData();
}

DEF_RUNTIME_STUBS(CreateSharedClass)
{
    RUNTIME_STUBS_HEADER(CreateSharedClass);
    JSHandle<JSTaggedValue> base = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> constpool = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSTaggedValue methodId = GetArg(argv, argc, 2);  // 2: means the second parameter
    JSTaggedValue literalId = GetArg(argv, argc, 3);  // 3: means the third parameter
    JSTaggedValue length = GetArg(argv, argc, 4);  // 4: means the fourth parameter
    JSHandle<JSTaggedValue> module = GetHArg<JSTaggedValue>(argv, argc, 5);  // 5: means the fifth parameter
    return RuntimeCreateSharedClass(thread, base, constpool,
                                    static_cast<uint16_t>(methodId.GetInt()),
                                    static_cast<uint16_t>(literalId.GetInt()),
                                    static_cast<uint16_t>(length.GetInt()), module).GetRawData();
}

DEF_RUNTIME_STUBS(LdSendableClass)
{
    RUNTIME_STUBS_HEADER(LdSendableClass);
    JSHandle<JSTaggedValue> env = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    uint16_t level = static_cast<uint16_t>(GetArg(argv, argc, 1).GetInt());  // 1: means the first parameter
    return RuntimeLdSendableClass(env, level).GetRawData();
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
    auto profileTypeInfo = thisFunc->GetProfileTypeInfo();
    if (profileTypeInfo.IsUndefined()) {
        uint32_t slotSize = method->GetSlotSize();
        auto res = RuntimeNotifyInlineCache(thread, thisFunc, slotSize);
        return res.GetRawData();
    }
    return profileTypeInfo.GetRawData();
}

DEF_RUNTIME_STUBS(PGODump)
{
    RUNTIME_STUBS_HEADER(PGODump);
    JSHandle<JSFunction> thisFunc = GetHArg<JSFunction>(argv, argc, 0);  // 0: means the zeroth parameter
    thread->GetEcmaVM()->GetPGOProfiler()->PGODump(thisFunc.GetTaggedType());
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(PGOPreDump)
{
    RUNTIME_STUBS_HEADER(PGOPreDump);
    JSHandle<JSFunction> thisFunc = GetHArg<JSFunction>(argv, argc, 0);  // 0: means the zeroth parameter
    thread->GetEcmaVM()->GetPGOProfiler()->PGOPreDump(thisFunc.GetTaggedType());
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(UpdateHotnessCounterWithProf)
{
    RUNTIME_STUBS_HEADER(UpdateHotnessCounterWithProf);
    JSHandle<JSFunction> thisFunc = GetHArg<JSFunction>(argv, argc, 0);  // 0: means the zeroth parameter
    thread->CheckSafepoint();
    auto profileTypeInfo = thisFunc->GetProfileTypeInfo();
    if (profileTypeInfo.IsUndefined()) {
        uint32_t slotSize = thisFunc->GetCallTarget()->GetSlotSize();
        auto res = RuntimeNotifyInlineCache(thread, thisFunc, slotSize);
        return res.GetRawData();
    }
    return profileTypeInfo.GetRawData();
}

DEF_RUNTIME_STUBS(JitCompile)
{
    RUNTIME_STUBS_HEADER(JitCompile);
    JSHandle<JSFunction> thisFunc = GetHArg<JSFunction>(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue offset = GetArg(argv, argc, 1);  // 1: means the first parameter
    Jit::Compile(thread->GetEcmaVM(), thisFunc, CompilerTier::Tier::FAST, offset.GetInt(), JitCompileMode::Mode::ASYNC);
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(BaselineJitCompile)
{
    RUNTIME_STUBS_HEADER(BaselineJitCompile);
    JSHandle<JSFunction> thisFunc = GetHArg<JSFunction>(argv, argc, 0);  // 0: means the zeroth parameter
    Jit::Compile(thread->GetEcmaVM(), thisFunc, CompilerTier::Tier::BASELINE,
                 MachineCode::INVALID_OSR_OFFSET, JitCompileMode::Mode::ASYNC);
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(CountInterpExecFuncs)
{
    RUNTIME_STUBS_HEADER(CountInterpExecFuncs);
    JSHandle thisFunc = GetHArg<JSFunction>(argv, argc, 0); // 0: means the zeroth parameter
    Jit::CountInterpExecFuncs(thisFunc);
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(CheckSafePoint)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    thread->CheckSafepoint();
    return JSTaggedValue::Undefined().GetRawData();
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
    return RuntimeOptCreateObjectWithExcludedKeys(thread, argv, argc).GetRawData();
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
        uint32_t curBytecodePcOfst = INVALID_INDEX;
        if (reinterpret_cast<uintptr_t>(frameHandler.GetPc()) == std::numeric_limits<uintptr_t>::max()) {
            // For baselineJit
            uintptr_t curNativePc = frameHandler.GetBaselineNativePc();
            ASSERT(curNativePc != 0);
            LOG_BASELINEJIT(DEBUG) << "current native pc in UpFrame: " << std::hex <<
                reinterpret_cast<void*>(curNativePc);
            JSHandle<JSTaggedValue> funcVal = JSHandle<JSTaggedValue>(thread, frameHandler.GetFunction());
            JSHandle<JSFunction> func = JSHandle<JSFunction>::Cast(funcVal);
            curBytecodePcOfst = RuntimeGetBytecodePcOfstForBaseline(func, curNativePc);
        } else {
            curBytecodePcOfst = frameHandler.GetBytecodeOffset();
        }
        pcOffset = method->FindCatchBlock(curBytecodePcOfst);
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

DEF_RUNTIME_STUBS(LdLocalModuleVarByIndexWithModule)
{
    RUNTIME_STUBS_HEADER(LdLocalModuleVarByIndexWithModule);
    JSTaggedValue index = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> module = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeLdLocalModuleVarWithModule(thread, index.GetInt(), module).GetRawData();
}

DEF_RUNTIME_STUBS(LdExternalModuleVarByIndex)
{
    RUNTIME_STUBS_HEADER(LdExternalModuleVarByIndex);
    JSTaggedValue index = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeLdExternalModuleVar(thread, index.GetInt()).GetRawData();
}

DEF_RUNTIME_STUBS(LdExternalModuleVarByIndexWithModule)
{
    RUNTIME_STUBS_HEADER(LdExternalModuleVarByIndexWithModule);
    JSTaggedValue index = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> module = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return RuntimeLdExternalModuleVarWithModule(thread, index.GetInt(), module).GetRawData();
}

DEF_RUNTIME_STUBS(LdSendableExternalModuleVarByIndex)
{
    RUNTIME_STUBS_HEADER(LdSendableExternalModuleVarByIndex);
    JSTaggedValue index = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue jsFunc = GetArg(argv, argc, 1); // 1: means the first parameter
    return RuntimeLdSendableExternalModuleVar(thread, index.GetInt(), jsFunc).GetRawData();
}

DEF_RUNTIME_STUBS(LdSendableLocalModuleVarByIndex)
{
    RUNTIME_STUBS_HEADER(LdSendableLocalModuleVarByIndex);
    JSTaggedValue index = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue jsFunc = GetArg(argv, argc, 1); // 1: means the first parameter
    return RuntimeLdSendableLocalModuleVar(thread, index.GetInt(), jsFunc).GetRawData();
}

DEF_RUNTIME_STUBS(LdLazyExternalModuleVarByIndex)
{
    RUNTIME_STUBS_HEADER(LdLazyExternalModuleVarByIndex);
    JSTaggedValue index = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue jsFunc = GetArg(argv, argc, 1); // 1: means the first parameter
    return RuntimeLdLazyExternalModuleVar(thread, index.GetInt(), jsFunc).GetRawData();
}

DEF_RUNTIME_STUBS(LdLazySendableExternalModuleVarByIndex)
{
    RUNTIME_STUBS_HEADER(LdLazySendableExternalModuleVarByIndex);
    JSTaggedValue index = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue jsFunc = GetArg(argv, argc, 1); // 1: means the first parameter
    return RuntimeLdLazySendableExternalModuleVar(thread, index.GetInt(), jsFunc).GetRawData();
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

DEF_RUNTIME_STUBS(GetPropIteratorSlowpath)
{
    RUNTIME_STUBS_HEADER(GetPropIteratorSlowpath);
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return JSObject::LoadEnumerateProperties(thread, value).GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(PrimitiveStringCreate)
{
    RUNTIME_STUBS_HEADER(PrimitiveStringCreate);
    JSHandle<JSTaggedValue> str = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> newTarget = thread->GlobalConstants()->GetHandledUndefined();
    return JSPrimitiveRef::StringCreate(thread, str, newTarget).GetTaggedValue().GetRawData();
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

DEF_RUNTIME_STUBS(LdPrivateProperty)
{
    RUNTIME_STUBS_HEADER(LdPrivateProperty);
    JSTaggedValue lexicalEnv = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    uint32_t levelIndex = static_cast<uint32_t>(GetArg(argv, argc, 1).GetInt());  // 1: means the first parameter
    uint32_t slotIndex = static_cast<uint32_t>(GetArg(argv, argc, 2).GetInt());  // 2: means the second parameter
    JSTaggedValue obj = GetArg(argv, argc, 3);  // 3: means the third parameter
    return RuntimeLdPrivateProperty(thread, lexicalEnv, levelIndex, slotIndex, obj).GetRawData();
}

DEF_RUNTIME_STUBS(StPrivateProperty)
{
    RUNTIME_STUBS_HEADER(StPrivateProperty);
    JSTaggedValue lexicalEnv = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    uint32_t levelIndex = static_cast<uint32_t>(GetArg(argv, argc, 1).GetInt());  // 1: means the first parameter
    uint32_t slotIndex = static_cast<uint32_t>(GetArg(argv, argc, 2).GetInt());  // 2: means the second parameter
    JSTaggedValue obj = GetArg(argv, argc, 3);  // 3: means the third parameter
    JSTaggedValue value = GetArg(argv, argc, 4);  // 4: means the fourth parameter
    return RuntimeStPrivateProperty(thread, lexicalEnv, levelIndex, slotIndex, obj, value).GetRawData();
}

DEF_RUNTIME_STUBS(TestIn)
{
    RUNTIME_STUBS_HEADER(TestIn);
    JSTaggedValue lexicalEnv = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    uint32_t levelIndex = static_cast<uint32_t>(GetArg(argv, argc, 1).GetInt());  // 1: means the first parameter
    uint32_t slotIndex = static_cast<uint32_t>(GetArg(argv, argc, 2).GetInt());  // 2: means the second parameter
    JSTaggedValue obj = GetArg(argv, argc, 3);  // 3: means the third parameter
    return RuntimeTestIn(thread, lexicalEnv, levelIndex, slotIndex, obj).GetRawData();
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
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR, message.c_str(), StackCheck::NO);
    THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error.GetTaggedValue(), JSTaggedValue::Hole().GetRawData());
}

DEF_RUNTIME_STUBS(MismatchError)
{
    RUNTIME_STUBS_HEADER(MismatchError);
    JSTaggedValue shareFieldType = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue value = GetArg(argv, argc, 1);  // 1: means the first parameter
    std::stringstream oss;
    value.DumpTaggedValueType(oss);
    LOG_ECMA(ERROR) << "Sendable obj Match field type fail. expected type: " <<
        ClassHelper::StaticFieldTypeToString(shareFieldType.GetInt()) << ", actual type: " << oss.str();
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(NewJSPrimitiveRef)
{
    RUNTIME_STUBS_HEADER(NewJSPrimitiveRef);
    JSHandle<JSFunction> thisFunc = GetHArg<JSFunction>(argv, argc, 0); // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue> (argv, argc, 1);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->NewJSPrimitiveRef(thisFunc, obj).GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(ThrowRangeError)
{
    RUNTIME_STUBS_HEADER(ThrowRangeError);
    JSTaggedValue argMessageStringId = GetArg(argv, argc, 0);
    std::string message = MessageString::GetMessageString(argMessageStringId.GetInt());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::RANGE_ERROR, message.c_str(), StackCheck::NO);
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

DEF_RUNTIME_STUBS(ToIndex)
{
    RUNTIME_STUBS_HEADER(ToIndex);
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return JSTaggedValue::ToIndex(thread, value).GetRawData();
}

DEF_RUNTIME_STUBS(NewJSObjectByConstructor)
{
    RUNTIME_STUBS_HEADER(NewJSObjectByConstructor);
    JSHandle<JSFunction> constructor = GetHArg<JSFunction>(argv, argc, 0);      // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> newTarget = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(constructor, newTarget);
    return obj.GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(CloneHclass)
{
    RUNTIME_STUBS_HEADER(CloneHclass);
    JSHandle<JSHClass> objHclass = GetHArg<JSHClass>(argv, argc, 0);      // 0: means the zeroth parameter
    return JSHClass::Clone(thread, objHclass).GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(AllocateTypedArrayBuffer)
{
    RUNTIME_STUBS_HEADER(AllocateTypedArrayBuffer);
    JSHandle<JSObject> obj = GetHArg<JSObject>(argv, argc, 0);    // 0: means the zeroth parameter
    JSTaggedValue length = GetArg(argv, argc, 1);  // 1: means the first parameter
    return base::TypedArrayHelper::AllocateTypedArrayBuffer(thread, obj, length.GetNumber(),
        base::TypedArrayHelper::GetType(JSHandle<JSTypedArray>(obj))).GetTaggedValue().GetRawData();
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
    JSHandle<JSTaggedValue> newTarget(GetHArg<JSTaggedValue>(argv, argc, 1));  // 1: means the first parameter

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj;
    if (newTarget->IsUndefined()) {
        obj = factory->NewJSObjectByConstructor(ctor);
    } else {
        obj = factory->NewJSObjectByConstructor(ctor, newTarget);
    }
    if (obj.GetTaggedValue().IsJSShared()) {
        obj->GetJSHClass()->SetExtensible(false);
    }
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
    JSHandle<JSTaggedValue> constpool = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue methodId = GetArg(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> module = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    uint16_t length = static_cast<uint16_t>(GetArg(argv, argc, 3).GetInt()); // 3: means the third parameter
    JSHandle<JSTaggedValue> env = GetHArg<JSTaggedValue>(argv, argc, 4); // 4: means the fourth parameter
    JSHandle<JSTaggedValue> homeObject = GetHArg<JSTaggedValue>(argv, argc, 5); // 5: means the fifth parameter
    return RuntimeDefinefunc(thread, constpool, static_cast<uint16_t>(methodId.GetInt()), module,
        length, env, homeObject).GetRawData();
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
    uint16_t length = static_cast<uint16_t>(GetArg(argv, argc, 2).GetInt()); // 2: means the second parameter
    JSHandle<JSTaggedValue> env = GetHArg<JSTaggedValue>(argv, argc, 3); // 3: means the third parameter
    JSHandle<JSTaggedValue> module = GetHArg<JSTaggedValue>(argv, argc, 4); // 4: means the fourth parameter
    auto res = RuntimeDefineMethod(thread, method, homeObject, length, env, module);
#if ECMASCRIPT_ENABLE_IC
    const uint32_t INDEX_OF_SLOT_ID = 5; // 5: index of slotId in argv
    if (argc > INDEX_OF_SLOT_ID) {
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
        uint16_t slotId = static_cast<uint16_t>(GetArg(argv, argc, INDEX_OF_SLOT_ID).GetInt());
        const uint32_t INDEX_OF_JS_FUNC = 6;  // 6: index of jsFunc in argv
        ASSERT(argc > INDEX_OF_JS_FUNC);
        JSHandle<JSFunction> jsFuncHandle = GetHArg<JSFunction>(argv, argc, INDEX_OF_JS_FUNC);
        JSHandle<JSFunction> resHandle(thread, res);
        SetProfileTypeInfoCellToFunction(thread, jsFuncHandle, resHandle, slotId);
        res = resHandle.GetTaggedValue();
    }
#endif
    return res.GetRawData();
}

DEF_RUNTIME_STUBS(SetPatchModule)
{
    RUNTIME_STUBS_HEADER(SetPatchModule);
    JSHandle<JSFunction> func = GetHArg<JSFunction>(argv, argc, 0);  // 0: means the zeroth parameter
    RuntimeSetPatchModule(thread, func);
    return JSTaggedValue::Hole().GetRawData();
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
    JSHandle<JSTaggedValue> func = GetHArg<JSTaggedValue>(argv, argc, 5);  // 5: means the sixth parameter
    int32_t pcOffset = GetArg(argv, argc, 6).GetInt();  // 6: means the seventh parameter
    bool bFlag = flag.ToBoolean();
    return RuntimeDefineGetterSetterByValue(thread, obj, prop, getter, setter, bFlag, func, pcOffset).GetRawData();
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
    JSHandle<JSTaggedValue> func = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> newTarget = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<TaggedArray> taggedArray(thread, GetArg(argv, argc, 2));  // 2: means the second parameter
    JSTaggedValue length = GetArg(argv, argc, 3);  // 3: means the third parameter
    return RuntimeOptSuperCall(thread, func, newTarget, taggedArray,
                               static_cast<uint16_t>(length.GetInt())).GetRawData();
}

DEF_RUNTIME_STUBS(ThrowNotCallableException)
{
    RUNTIME_STUBS_HEADER(ThrowNotCallableException);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR, "is not callable", StackCheck::NO);
    thread->SetException(error.GetTaggedValue());
    return JSTaggedValue::Exception().GetRawData();
}

DEF_RUNTIME_STUBS(ThrowSetterIsUndefinedException)
{
    RUNTIME_STUBS_HEADER(ThrowSetterIsUndefinedException);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR,
        "Cannot set property when setter is undefined", StackCheck::NO);
    thread->SetException(error.GetTaggedValue());
    return JSTaggedValue::Exception().GetRawData();
}

DEF_RUNTIME_STUBS(ThrowCallConstructorException)
{
    RUNTIME_STUBS_HEADER(ThrowCallConstructorException);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR,
                                                   "class constructor cannot called without 'new'", StackCheck::NO);
    thread->SetException(error.GetTaggedValue());
    return JSTaggedValue::Exception().GetRawData();
}

DEF_RUNTIME_STUBS(ThrowNonConstructorException)
{
    RUNTIME_STUBS_HEADER(ThrowNonConstructorException);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR,
                                                   "function is non-constructor", StackCheck::NO);
    thread->SetException(error.GetTaggedValue());
    return JSTaggedValue::Exception().GetRawData();
}

DEF_RUNTIME_STUBS(ThrowStackOverflowException)
{
    RUNTIME_STUBS_HEADER(ThrowStackOverflowException);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    // Multi-thread could cause stack-overflow-check failed too,
    // so check thread here to distinguish it with the actual stack overflow.
    ecmaVm->CheckThread();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::RANGE_ERROR, "Stack overflow!", StackCheck::NO);
    if (LIKELY(!thread->HasPendingException())) {
        thread->SetException(error.GetTaggedValue());
    }
    return JSTaggedValue::Exception().GetRawData();
}

DEF_RUNTIME_STUBS(ThrowDerivedMustReturnException)
{
    RUNTIME_STUBS_HEADER(ThrowDerivedMustReturnException);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR,
        "Derived constructor must return object or undefined", StackCheck::NO);
    thread->SetException(error.GetTaggedValue());
    return JSTaggedValue::Exception().GetRawData();
}

DEF_RUNTIME_STUBS(LdBigInt)
{
    RUNTIME_STUBS_HEADER(LdBigInt);
    JSHandle<JSTaggedValue> numberBigInt = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeLdBigInt(thread, numberBigInt).GetRawData();
}

DEF_RUNTIME_STUBS(CallBigIntAsIntN)
{
    RUNTIME_STUBS_HEADER(CallBigIntAsIntN);
    JSTaggedValue bits = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue bigint = GetArg(argv, argc, 1);  // 1: means the first parameter
    return RuntimeCallBigIntAsIntN(thread, bits, bigint).GetRawData();
}

DEF_RUNTIME_STUBS(CallBigIntAsUintN)
{
    RUNTIME_STUBS_HEADER(CallBigIntAsUintN);
    JSTaggedValue bits = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue bigint = GetArg(argv, argc, 1);  // 1: means the first parameter
    return RuntimeCallBigIntAsUintN(thread, bits, bigint).GetRawData();
}

DEF_RUNTIME_STUBS(ToNumeric)
{
    RUNTIME_STUBS_HEADER(ToNumeric);
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeToNumeric(thread, value).GetRawData();
}

DEF_RUNTIME_STUBS(ToNumericConvertBigInt)
{
    RUNTIME_STUBS_HEADER(ToNumericConvertBigInt);
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> numericVal(thread, RuntimeToNumeric(thread, value));
    if (numericVal->IsBigInt()) {
        JSHandle<BigInt> bigNumericVal(numericVal);
        return BigInt::BigIntToNumber(bigNumericVal).GetRawData();
    }
    return numericVal->GetRawData();
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

DEF_RUNTIME_STUBS(NewSendableEnv)
{
    RUNTIME_STUBS_HEADER(NewSendableEnv);
    JSTaggedValue numVars = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeNewSendableEnv(thread, static_cast<uint16_t>(numVars.GetInt())).GetRawData();
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

DEF_RUNTIME_STUBS(GetPropertyByName)
{
    RUNTIME_STUBS_HEADER(GetPropertyByName);
    JSHandle<JSTaggedValue> target = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> key = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    return JSTaggedValue::GetProperty(thread, target, key).GetValue()->GetRawData();
}

DEF_RUNTIME_STUBS(DebugAOTPrint)
{
    RUNTIME_STUBS_HEADER(DebugAOTPrint);
    int ecmaOpcode = GetArg(argv, argc, 0).GetInt();
    int path = GetArg(argv, argc, 1).GetInt();
    std::string pathStr = path == 0 ? "slow path  " : "TYPED path ";

    std::string data = JsStackInfo::BuildJsStackTrace(thread, true);
    std::string opcode = kungfu::GetEcmaOpcodeStr(static_cast<EcmaOpcode>(ecmaOpcode));
    LOG_ECMA(INFO) << "AOT " << pathStr << ": " << opcode << "@ " << data;
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(ProfileOptimizedCode)
{
    RUNTIME_STUBS_HEADER(ProfileOptimizedCode);
    JSHandle<JSTaggedValue> func = GetHArg<JSTaggedValue>(argv, argc, 0);
    int bcIndex = GetArg(argv, argc, 1).GetInt();
    EcmaOpcode ecmaOpcode = static_cast<EcmaOpcode>(GetArg(argv, argc, 2).GetInt());
    OptCodeProfiler::Mode mode = static_cast<OptCodeProfiler::Mode>(GetArg(argv, argc, 3).GetInt());
    OptCodeProfiler *profiler = thread->GetCurrentEcmaContext()->GetOptCodeProfiler();
    profiler->Update(func, bcIndex, ecmaOpcode, mode);
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(ProfileTypedOp)
{
    RUNTIME_STUBS_HEADER(ProfileOptimizedCode);
    kungfu::OpCode opcode = static_cast<kungfu::OpCode>(GetArg(argv, argc, 0).GetInt());
    TypedOpProfiler *profiler = thread->GetCurrentEcmaContext()->GetTypdOpProfiler();
    if (profiler != nullptr) {
        profiler->Update(opcode);
    }
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(VerifyVTableLoading)
{
    RUNTIME_STUBS_HEADER(VerifyVTableLoading);
    JSHandle<JSTaggedValue> receiver = GetHArg<JSTaggedValue>(argv, argc, 0);        // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> key = GetHArg<JSTaggedValue>(argv, argc, 1);             // 1: means the first parameter
    JSHandle<JSTaggedValue> typedPathValue = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter

    JSHandle<JSTaggedValue> verifiedPathValue = JSTaggedValue::GetProperty(thread, receiver, key).GetValue();
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
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
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
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
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
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
    if (EcmaStringAccessor::StringsAreEqualDiffUtfEncoding(*left, *right)) {
        return JSTaggedValue::VALUE_TRUE;
    }
    return JSTaggedValue::VALUE_FALSE;
}

DEF_RUNTIME_STUBS(StringIndexOf)
{
    RUNTIME_STUBS_HEADER(StringIndexOf);
    JSHandle<JSTaggedValue> receiver = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> searchElement = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    uint32_t from = static_cast<uint32_t>(GetArg(argv, argc, 2).GetInt());  // 2: means the second parameter
    uint32_t len = static_cast<uint32_t>(GetArg(argv, argc, 3).GetInt());  // 3: means the third parameter

    return JSStableArray::IndexOf(thread, receiver, searchElement, from, len).GetRawData();
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

DEF_RUNTIME_STUBS(UpdateAOTHClass)
{
    RUNTIME_STUBS_HEADER(UpdateAOTHClass);
    JSHandle<JSHClass> oldhclass = GetHArg<JSHClass>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSHClass> newhclass = GetHArg<JSHClass>(argv, argc, 1);  // 1: means the first parameter
    JSTaggedValue key = GetArg(argv, argc, 2);  // 2: means the second parameter
    return RuntimeUpdateAOTHClass(thread, oldhclass, newhclass, key).GetRawData();
}

DEF_RUNTIME_STUBS(DefineField)
{
    RUNTIME_STUBS_HEADER(DefineField);
    JSTaggedValue obj = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue propKey = GetArg(argv, argc, 1);  // 1: means the first parameter
    JSTaggedValue value = GetArg(argv, argc, 2);  // 2: means the second parameter
    return RuntimeDefineField(thread, obj, propKey, value).GetRawData();
}

DEF_RUNTIME_STUBS(CreatePrivateProperty)
{
    RUNTIME_STUBS_HEADER(CreatePrivateProperty);
    JSTaggedValue lexicalEnv = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    uint32_t count = static_cast<uint32_t>(GetArg(argv, argc, 1).GetInt());  // 1: means the first parameter
    JSTaggedValue constpool = GetArg(argv, argc, 2);  // 2: means the second parameter
    uint32_t literalId = static_cast<uint32_t>(GetArg(argv, argc, 3).GetInt());  // 3: means the third parameter
    JSTaggedValue module = GetArg(argv, argc, 4);  // 4: means the fourth parameter
    return RuntimeCreatePrivateProperty(thread, lexicalEnv, count, constpool, literalId, module).GetRawData();
}

DEF_RUNTIME_STUBS(DefinePrivateProperty)
{
    RUNTIME_STUBS_HEADER(DefinePrivateProperty);
    JSTaggedValue lexicalEnv = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    uint32_t levelIndex = static_cast<uint32_t>(GetArg(argv, argc, 1).GetInt());  // 1: means the first parameter
    uint32_t slotIndex = static_cast<uint32_t>(GetArg(argv, argc, 2).GetInt());  // 2: means the second parameter
    JSTaggedValue obj = GetArg(argv, argc, 3);  // 3: means the third parameter
    JSTaggedValue value = GetArg(argv, argc, 4);  // 4: means the fourth parameter
    return RuntimeDefinePrivateProperty(thread, lexicalEnv, levelIndex, slotIndex, obj, value).GetRawData();
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

DEF_RUNTIME_STUBS(InsertStringToTable)
{
    RUNTIME_STUBS_HEADER(InsertStringToTable);
    JSHandle<EcmaString> str = GetHArg<EcmaString>(argv, argc, 0);  // 0: means the zeroth parameter
    return JSTaggedValue::Cast(
        static_cast<void *>(thread->GetEcmaVM()->GetEcmaStringTable()->InsertStringToTable(thread->GetEcmaVM(), str)));
}

DEF_RUNTIME_STUBS(SlowFlattenString)
{
    RUNTIME_STUBS_HEADER(SlowFlattenString);
    JSHandle<EcmaString> str = GetHArg<EcmaString>(argv, argc, 0);  // 0: means the zeroth parameter
    return JSTaggedValue(EcmaStringAccessor::SlowFlatten(thread->GetEcmaVM(), str)).GetRawData();
}

DEF_RUNTIME_STUBS(TryGetInternString)
{
    RUNTIME_STUBS_HEADER(TryGetInternString);
    JSHandle<EcmaString> string = GetHArg<EcmaString>(argv, argc, 0);  // 0: means the zeroth parameter
    return RuntimeTryGetInternString(argGlue, string);
}

DEF_RUNTIME_STUBS(FastCopyFromArrayToTypedArray)
{
    RUNTIME_STUBS_HEADER(FastCopyFromArrayToTypedArray);
    JSHandle<JSTypedArray> targetArray = GetHArg<JSTypedArray>(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue len = GetArg(argv, argc, 1);  // 1: param index
    JSHandle<JSObject> obj = GetHArg<JSObject>(argv, argc, 2);  // 2: param index
    return JSStableArray::FastCopyFromArrayToTypedArray(thread, targetArray,
        base::TypedArrayHelper::GetType(targetArray), 0, len.GetInt(), obj).GetRawData();
}

DEF_RUNTIME_STUBS(DecodeURIComponent)
{
    RUNTIME_STUBS_HEADER(DecodeURIComponent);
    JSHandle<JSTaggedValue> arg = GetHArg<JSTaggedValue>(argv, argc, 0);
    JSHandle<EcmaString> string = JSTaggedValue::ToString(thread, arg);
    if (thread->HasPendingException()) {
        return JSTaggedValue::VALUE_EXCEPTION;
    }
    if (EcmaStringAccessor(string).IsTreeString()) {
        string = JSHandle<EcmaString>(thread, EcmaStringAccessor::Flatten(thread->GetEcmaVM(), string));
    }
    auto stringAcc = EcmaStringAccessor(string);
    JSTaggedValue result;
    if (stringAcc.IsLineString()) {
        // line string or flatten tree string
        if (!stringAcc.IsUtf16()) {
            result = RuntimeDecodeURIComponent<uint8_t>(thread, string, stringAcc.GetDataUtf8());
        } else {
            result = RuntimeDecodeURIComponent<uint16_t>(thread, string, stringAcc.GetDataUtf16());
        }
    } else if (stringAcc.IsConstantString()) {
        ASSERT(stringAcc.IsUtf8());
        result = RuntimeDecodeURIComponent<uint8_t>(thread, string, stringAcc.GetDataUtf8());
    } else {
        ASSERT(stringAcc.IsSlicedString());
        auto parent = SlicedString::Cast(string.GetTaggedValue())->GetParent();
        auto parentStrAcc = EcmaStringAccessor(parent);
        auto startIndex = SlicedString::Cast(string.GetTaggedValue())->GetStartIndex();
        if (parentStrAcc.IsLineString()) {
            if (parentStrAcc.IsUtf8()) {
                result = RuntimeDecodeURIComponent<uint8_t>(thread, string,
                                                            parentStrAcc.GetDataUtf8() + startIndex);
            } else {
                result = RuntimeDecodeURIComponent<uint16_t>(thread, string,
                                                             parentStrAcc.GetDataUtf16() + startIndex);
            }
        } else {
            result = RuntimeDecodeURIComponent<uint8_t>(thread, string, parentStrAcc.GetDataUtf8() + startIndex);
        }
    }
    return result.GetRawData();
}

void RuntimeStubs::UpdateFieldType(JSTaggedType hclass, uint64_t value)
{
    auto cls = reinterpret_cast<JSHClass *>(hclass);
    PropertyAttributes attrValue(value);
    JSHClass::UpdateFieldType(cls, attrValue);
}

JSTaggedType RuntimeStubs::GetActualArgvNoGC(uintptr_t argGlue)
{
    DISALLOW_GARBAGE_COLLECTION;
    auto thread = JSThread::GlueToJSThread(argGlue);
    JSTaggedType *current = const_cast<JSTaggedType *>(thread->GetLastLeaveFrame());
    FrameIterator it(current, thread);
    ASSERT(it.IsOptimizedFrame());
    it.Advance<GCVisitedFlag::VISITED>();
    ASSERT(it.IsAotOrJitFunctionFrame());
    if (it.IsFastJitFunctionFrame()) {
        auto fastJitFunctionFrame = it.GetFrame<FASTJITFunctionFrame>();
        return reinterpret_cast<uintptr_t>(fastJitFunctionFrame->GetArgv(it));
    } else {
        auto optimizedJSFunctionFrame = it.GetFrame<OptimizedJSFunctionFrame>();
        return reinterpret_cast<uintptr_t>(optimizedJSFunctionFrame->GetArgv(it));
    }
}

double RuntimeStubs::FloatMod(double x, double y)
{
    return std::fmod(x, y);
}

double RuntimeStubs::FloatAcos(double x)
{
    return std::acos(x);
}

double RuntimeStubs::FloatAcosh(double x)
{
    return std::acosh(x);
}

double RuntimeStubs::FloatAsin(double x)
{
    return std::asin(x);
}

double RuntimeStubs::FloatAsinh(double x)
{
    return std::asinh(x);
}

double RuntimeStubs::FloatAtan(double x)
{
    return std::atan(x);
}

double RuntimeStubs::FloatAtan2(double y, double x)
{
    return std::atan2(y, x);
}

double RuntimeStubs::FloatAtanh(double x)
{
    return std::atanh(x);
}

double RuntimeStubs::FloatCos(double x)
{
    return std::cos(x);
}

double RuntimeStubs::FloatCosh(double x)
{
    return std::cosh(x);
}

double RuntimeStubs::FloatSin(double x)
{
    return std::sin(x);
}

double RuntimeStubs::FloatSinh(double x)
{
    return std::sinh(x);
}

double RuntimeStubs::FloatTan(double x)
{
    return std::tan(x);
}

double RuntimeStubs::FloatTanh(double x)
{
    return std::tanh(x);
}

double RuntimeStubs::FloatCbrt(double x)
{
    return std::cbrt(x);
}

double RuntimeStubs::FloatTrunc(double x)
{
    return std::trunc(x);
}

double RuntimeStubs::FloatCeil(double x)
{
    return std::ceil(x);
}

double RuntimeStubs::FloatFloor(double x)
{
    return std::floor(x);
}

double RuntimeStubs::FloatLog(double x)
{
    return std::log(x);
}

double RuntimeStubs::FloatLog2(double x)
{
    return std::log2(x);
}

double RuntimeStubs::FloatLog10(double x)
{
    return std::log10(x);
}

double RuntimeStubs::FloatLog1p(double x)
{
    return std::log1p(x);
}

double RuntimeStubs::FloatExp(double x)
{
    return std::exp(x);
}

double RuntimeStubs::FloatExpm1(double x)
{
    return std::expm1(x);
}

double RuntimeStubs::FloatPow(double base, double exp)
{
    return std::pow(base, exp);
}

double RuntimeStubs::CallDateNow()
{
    // time from now is in ms.
    int64_t ans;
    struct timeval tv {
    };
    gettimeofday(&tv, nullptr);
    ans = static_cast<int64_t>(tv.tv_sec) * MS_PER_SECOND + (tv.tv_usec / MS_PER_SECOND);
    return static_cast<double>(ans);
}

int32_t RuntimeStubs::DoubleToInt(double x, size_t bits)
{
    return base::NumberHelper::DoubleToInt(x, bits);
}

int32_t RuntimeStubs::SaturateTruncDoubleToInt32(double x)
{
    return base::NumberHelper::SaturateTruncDoubleToInt32(x);
}

uint8_t RuntimeStubs::LrInt(double x)
{
    DISALLOW_GARBAGE_COLLECTION;
    return static_cast<uint8_t>(std::lrint(x));
}

void RuntimeStubs::InsertNewToEdenRSet([[maybe_unused]] uintptr_t argGlue,
    uintptr_t object, size_t offset)
{
    Region *region = Region::ObjectAddressToRange(object);
    uintptr_t slotAddr = object + offset;
    return region->InsertNewToEdenRSet(slotAddr);
}

void RuntimeStubs::InsertOldToNewRSet([[maybe_unused]] uintptr_t argGlue,
    uintptr_t object, size_t offset)
{
    Region *region = Region::ObjectAddressToRange(object);
    uintptr_t slotAddr = object + offset;
    return region->InsertOldToNewRSet(slotAddr);
}

void RuntimeStubs::InsertLocalToShareRSet([[maybe_unused]] uintptr_t argGlue,
    uintptr_t object, size_t offset)
{
    Region *region = Region::ObjectAddressToRange(object);
    uintptr_t slotAddr = object + offset;
    region->InsertLocalToShareRSet(slotAddr);
}

void RuntimeStubs::SetBitAtomic(GCBitset::GCBitsetWord *word, GCBitset::GCBitsetWord mask,
                                GCBitset::GCBitsetWord oldValue)
{
    volatile auto atomicWord = reinterpret_cast<volatile std::atomic<GCBitset::GCBitsetWord> *>(word);
    GCBitset::GCBitsetWord oldValueBeforeCAS = oldValue;
    std::atomic_compare_exchange_strong_explicit(atomicWord, &oldValue, oldValue | mask,
        std::memory_order_release, std::memory_order_relaxed);
    while (oldValue != oldValueBeforeCAS) {
        if (oldValue & mask) {
            return;
        }
        oldValueBeforeCAS = oldValue;
        std::atomic_compare_exchange_strong_explicit(atomicWord, &oldValue, oldValue | mask,
            std::memory_order_release, std::memory_order_relaxed);
    }
}

void RuntimeStubs::MarkingBarrier([[maybe_unused]] uintptr_t argGlue,
    uintptr_t object, size_t offset, TaggedObject *value)
{
    uintptr_t slotAddr = object + offset;
    Region *objectRegion = Region::ObjectAddressToRange(object);
    Region *valueRegion = Region::ObjectAddressToRange(value);
    ASSERT(!valueRegion->InSharedHeap());
    auto thread = JSThread::GlueToJSThread(argGlue);
#if ECMASCRIPT_ENABLE_BARRIER_CHECK
    if (!thread->GetEcmaVM()->GetHeap()->IsAlive(JSTaggedValue(value).GetHeapObject())) {
        LOG_FULL(FATAL) << "RuntimeStubs::MarkingBarrier checked value:" << value << " is invalid!";
    }
#endif
    ASSERT(thread->IsConcurrentMarkingOrFinished());
    Barriers::UpdateWithoutEden(thread, slotAddr, objectRegion, value, valueRegion);
}

void RuntimeStubs::MarkingBarrierWithEden([[maybe_unused]] uintptr_t argGlue,
    uintptr_t object, size_t offset, TaggedObject *value)
{
    uintptr_t slotAddr = object + offset;
    Region *objectRegion = Region::ObjectAddressToRange(object);
    Region *valueRegion = Region::ObjectAddressToRange(value);
    ASSERT(!valueRegion->InSharedHeap());
    auto thread = JSThread::GlueToJSThread(argGlue);
#if ECMASCRIPT_ENABLE_BARRIER_CHECK
    if (!thread->GetEcmaVM()->GetHeap()->IsAlive(JSTaggedValue(value).GetHeapObject())) {
        LOG_FULL(FATAL) << "RuntimeStubs::MarkingBarrierWithEden checked value:" << value << " is invalid!";
    }
#endif
    ASSERT(thread->IsConcurrentMarkingOrFinished());
    Barriers::Update(thread, slotAddr, objectRegion, value, valueRegion);
}

void RuntimeStubs::SharedGCMarkingBarrier([[maybe_unused]] uintptr_t argGlue, TaggedObject *value)
{
    Region *valueRegion = Region::ObjectAddressToRange(value);
    ASSERT(valueRegion->InSharedSweepableSpace());
    auto thread = JSThread::GlueToJSThread(argGlue);
#if ECMASCRIPT_ENABLE_BARRIER_CHECK
    if (!thread->GetEcmaVM()->GetHeap()->IsAlive(JSTaggedValue(value).GetHeapObject())) {
        LOG_FULL(FATAL) << "RuntimeStubs::SharedGCMarkingBarrier checked value:" << value << " is invalid!";
    }
#endif
    ASSERT(thread->IsSharedConcurrentMarkingOrFinished());
    Barriers::UpdateShared(thread, value, valueRegion);
}

bool RuntimeStubs::BigIntEquals(JSTaggedType left, JSTaggedType right)
{
    DISALLOW_GARBAGE_COLLECTION;
    return BigInt::Equal(JSTaggedValue(left), JSTaggedValue(right));
}

bool RuntimeStubs::BigIntSameValueZero(JSTaggedType left, JSTaggedType right)
{
    DISALLOW_GARBAGE_COLLECTION;
    return BigInt::SameValueZero(JSTaggedValue(left), JSTaggedValue(right));
}

JSTaggedValue RuntimeStubs::JSHClassFindProtoTransitions(JSHClass *cls, JSTaggedValue key, JSTaggedValue proto)
{
    DISALLOW_GARBAGE_COLLECTION;
    return JSTaggedValue(cls->FindProtoTransitions(key, proto));
}

JSTaggedValue RuntimeStubs::NumberHelperStringToDouble(EcmaString *numberString)
{
    DISALLOW_GARBAGE_COLLECTION;
    CVector<uint8_t> buf;
    Span<const uint8_t> str = EcmaStringAccessor(numberString).ToUtf8Span(buf);
    if (base::NumberHelper::IsEmptyString(str.begin(), str.end())) {
        return base::BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    }
    double result = base::NumberHelper::StringToDouble(str.begin(), str.end(), 0, base::IGNORE_TRAILING);
    return base::BuiltinsBase::GetTaggedDouble(result);
}

JSTaggedValue RuntimeStubs::GetStringToListCacheArray(uintptr_t argGlue)
{
    DISALLOW_GARBAGE_COLLECTION;
    auto thread = JSThread::GlueToJSThread(argGlue);
    return thread->GetCurrentEcmaContext()->GetStringToListResultCache().GetTaggedValue();
}

double RuntimeStubs::TimeClip(double time)
{
    DISALLOW_GARBAGE_COLLECTION;
    return JSDate::TimeClip(time);
}

double RuntimeStubs::SetDateValues(double year, double month, double day)
{
    DISALLOW_GARBAGE_COLLECTION;
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
        RETURN_STACK_BEFORE_THROW_IF_ASM(thread);
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
    JSTaggedValue function = frameHandler.GetFunction();
    JSFunction *func = JSFunction::Cast(function.GetTaggedObject());
    Method *method = func->GetCallTarget();
    if (func->IsCompiledCode()) {
        bool isFastCall = func->IsCompiledFastCall();  // get this flag before clear it
        uintptr_t entry = isFastCall ? thread->GetRTInterface(kungfu::RuntimeStubCSigns::ID_FastCallToAsmInterBridge)
                                     : thread->GetRTInterface(kungfu::RuntimeStubCSigns::ID_AOTCallToAsmInterBridge);
        func->SetCodeEntry(entry);
        method->ClearAOTStatusWhenDeopt(entry);
        func->ClearCompiledCodeFlags();
    }
    context->SetMethod(thread, function);
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
    if (boundFunc->GetBoundTarget().IsJSFunction()) {
        JSHandle<JSFunction> targetFunc(thread, boundFunc->GetBoundTarget());
        if (targetFunc->IsClassConstructor()) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "class constructor cannot called without 'new'",
                                        JSTaggedValue::Exception());
        }
    }
    JSHandle<TaggedArray> boundArgs(thread, boundFunc->GetBoundArguments());
    const uint32_t boundLength = boundArgs->GetLength();
    const uint32_t argsLength = info->GetArgsNumber() + boundLength;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *runtimeInfo = EcmaInterpreter::NewRuntimeCallInfo(thread,
        JSHandle<JSTaggedValue>(thread, boundFunc->GetBoundTarget()),
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
    auto callerRecordName = callerMethod->GetRecordNameStr();
    auto inlineRecordNanme = inlineMethod->GetRecordNameStr();
    const std::string callerFuncName(callerMethod->GetMethodName());
    const std::string inlineFuncNanme(inlineMethod->GetMethodName());
    std::string callerFullName = callerFuncName + "@" + std::string(callerRecordName);
    std::string inlineFullName = inlineFuncNanme + "@" + std::string(inlineRecordNanme);

    LOG_TRACE(INFO) << "aot inline function name: " << inlineFullName << " caller function name: " << callerFullName;
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(AotInlineBuiltinTrace)
{
    RUNTIME_STUBS_HEADER(AotInlineBuiltinTrace);
    JSTaggedValue callerFunc = GetArg(argv, argc, 0);
    JSFunction *callerJSFunc = JSFunction::Cast(callerFunc);
    Method *callerMethod = Method::Cast(callerJSFunc->GetMethod());
    auto callerRecordName = callerMethod->GetRecordNameStr();
    const std::string callerFuncName(callerMethod->GetMethodName());
    std::string callerFullName = callerFuncName + "@" + std::string(callerRecordName);

    auto builtinId = static_cast<kungfu::BuiltinsStubCSigns::ID>(GetArg(argv, argc, 1).GetInt());
    LOG_TRACE(INFO) << "aot inline builtin: " << kungfu::BuiltinsStubCSigns::GetBuiltinName(builtinId)
                    << ", caller function name:" << callerFullName;
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(AotCallBuiltinTrace)
{
    RUNTIME_STUBS_HEADER(AotCallBuiltinTrace);
    JSTaggedValue callerFunc = GetArg(argv, argc, 0);
    JSFunction *callerJSFunc = JSFunction::Cast(callerFunc);
    Method *callerMethod = Method::Cast(callerJSFunc->GetMethod());
    auto callerRecordName = callerMethod->GetRecordNameStr();
    const std::string callerFuncName(callerMethod->GetMethodName());
    std::string callerFullName = callerFuncName + "@" + std::string(callerRecordName);

    auto builtinId = static_cast<kungfu::BuiltinsStubCSigns::ID>(GetArg(argv, argc, 1).GetInt());
    LOG_TRACE(INFO) << "aot call builtin: " << kungfu::BuiltinsStubCSigns::GetBuiltinName(builtinId)
                    << ", caller function name:" << callerFullName;
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
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
    [[maybe_unused]] JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisObj);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
    [[maybe_unused]] JSHandle<EcmaString> thatHandle = JSTaggedValue::ToString(thread, thatTag);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());

    return builtins::BuiltinsString::DoLocaleCompare(thread, thisHandle, thatHandle, locales, options).GetRawData();
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

    JSHandle<JSTaggedValue> callbackFnHandle(thread, JSTaggedValue::Undefined());
    JSArray::Sort(thread, JSHandle<JSTaggedValue>::Cast(thisObjHandle), callbackFnHandle);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
    return thisObjHandle.GetTaggedValue();
}

DEF_RUNTIME_STUBS(HClassCloneWithAddProto)
{
    RUNTIME_STUBS_HEADER(HClassCloneWithAddProto);
    JSHandle<JSHClass> jshclass = GetHArg<JSHClass>(argv, argc, 0);            // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> key = GetHArg<JSTaggedValue>(argv, argc, 1);       // 1: means the first parameter
    JSHandle<JSTaggedValue> proto = GetHArg<JSTaggedValue>(argv, argc, 2);     // 2: means the second parameter
    return JSHClass::CloneWithAddProto(thread, jshclass, key, proto).GetTaggedValue().GetRawData();
}

void RuntimeStubs::StartCallTimer(uintptr_t argGlue, JSTaggedType func, bool isAot)
{
    DISALLOW_GARBAGE_COLLECTION;
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
    DISALLOW_GARBAGE_COLLECTION;
    auto thread =  JSThread::GlueToJSThread(argGlue);
    JSTaggedValue callTarget(func);
    Method *method = Method::Cast(JSFunction::Cast(callTarget)->GetMethod());
    if (method->IsNativeWithCallField()) {
        return;
    }
    auto callTimer = thread->GetEcmaVM()->GetCallTimer();
    callTimer->StopCount(method);
}

int32_t RuntimeStubs::StringGetStart(bool isUtf8, EcmaString *srcString, int32_t length, int32_t startIndex)
{
    DISALLOW_GARBAGE_COLLECTION;
    if (isUtf8) {
        Span<const uint8_t> data(EcmaStringAccessor(srcString).GetDataUtf8() + startIndex, length);
        return static_cast<int32_t>(base::StringHelper::GetStart(data, length));
    } else {
        Span<const uint16_t> data(EcmaStringAccessor(srcString).GetDataUtf16() + startIndex, length);
        return static_cast<int32_t>(base::StringHelper::GetStart(data, length));
    }
}

int32_t RuntimeStubs::StringGetEnd(bool isUtf8, EcmaString *srcString,
    int32_t start, int32_t length, int32_t startIndex)
{
    DISALLOW_GARBAGE_COLLECTION;
    if (isUtf8) {
        Span<const uint8_t> data(EcmaStringAccessor(srcString).GetDataUtf8() + startIndex, length);
        return base::StringHelper::GetEnd(data, start, length);
    } else {
        Span<const uint16_t> data(EcmaStringAccessor(srcString).GetDataUtf16() + startIndex, length);
        return base::StringHelper::GetEnd(data, start, length);
    }
}

DEF_RUNTIME_STUBS(FastStringify)
{
    RUNTIME_STUBS_HEADER(FastStringify);
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 0);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    base::JsonStringifier jsonStringifier(thread);
    JSHandle<JSTaggedValue> result = jsonStringifier.Stringify(value, undefined, undefined);
    return result.GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(GetLinkedHash)
{
    RUNTIME_STUBS_HEADER(GetLinkedHash);
    JSTaggedValue key = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    return JSTaggedValue(LinkedHash::Hash(thread, key)).GetRawData();
}

DEF_RUNTIME_STUBS(LinkedHashMapComputeCapacity)
{
    RUNTIME_STUBS_HEADER(LinkedHashMapComputeCapacity);
    JSTaggedValue value = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    return JSTaggedValue(LinkedHashMap::ComputeCapacity(value.GetInt())).GetRawData();
}

DEF_RUNTIME_STUBS(LinkedHashSetComputeCapacity)
{
    RUNTIME_STUBS_HEADER(LinkedHashSetComputeCapacity);
    JSTaggedValue value = GetArg(argv, argc, 0);  // 0: means the zeroth parameter
    return JSTaggedValue(LinkedHashSet::ComputeCapacity(value.GetInt())).GetRawData();
}

DEF_RUNTIME_STUBS(ObjectSlowAssign)
{
    RUNTIME_STUBS_HEADER(ObjectSlowAssign);
    JSHandle<JSObject> toAssign = GetHArg<JSObject>(argv, argc, 0);            // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> source = GetHArg<JSTaggedValue>(argv, argc, 1);    // 1: means the first parameter
    return builtins::BuiltinsObject::AssignTaggedValue(thread, source, toAssign).GetRawData();
}

DEF_RUNTIME_STUBS(NameDictionaryGetAllEnumKeys)
{
    RUNTIME_STUBS_HEADER(NameDictionaryGetAllEnumKeys);
    JSHandle<JSObject> object = GetHArg<JSObject>(argv, argc, 0);            // 0: means the zeroth parameter
    JSTaggedValue argKeys = GetArg(argv, argc, 1);    // 1: means the first parameter
    int numOfKeys = argKeys.GetInt();
    uint32_t keys = 0;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> keyArray = factory->NewTaggedArray(numOfKeys);
    NameDictionary *dict = NameDictionary::Cast(object->GetProperties().GetTaggedObject());
    dict->GetAllEnumKeys(thread, 0, keyArray, &keys);
    if (keys < keyArray->GetLength()) {
        keyArray->Trim(thread, keys);
    }
    return keyArray.GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(NumberDictionaryGetAllEnumKeys)
{
    RUNTIME_STUBS_HEADER(NumberDictionaryGetAllEnumKeys);
    JSHandle<TaggedArray> array = GetHArg<TaggedArray>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<TaggedArray> elementArray = GetHArg<TaggedArray>(argv, argc, 1);  // 1: means the first parameter
    JSTaggedValue argKeys = GetArg(argv, argc, 2);  // 2: means the second parameter
    int elementIndex = argKeys.GetInt();
    uint32_t keys = elementIndex;
    NumberDictionary::GetAllEnumKeys(
        thread, JSHandle<NumberDictionary>(array), elementIndex, elementArray, &keys);
    if (keys < elementArray->GetLength()) {
        elementArray->Trim(thread, keys);
    }
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(NumberToString)
{
    RUNTIME_STUBS_HEADER(NumberToString);
    JSTaggedValue argKeys = GetArg(argv, argc, 0);
    return JSHandle<JSTaggedValue>::Cast(base::NumberHelper::NumberToString(thread,
        argKeys)).GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(IntToString)
{
    RUNTIME_STUBS_HEADER(IntToString);
    JSTaggedValue argKeys = GetArg(argv, argc, 0);
    return JSHandle<JSTaggedValue>::Cast(base::NumberHelper::IntToEcmaString(thread,
        argKeys.GetInt())).GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(LocaleCompareWithGc)
{
    RUNTIME_STUBS_HEADER(LocaleCompareWithGc);
    JSHandle<JSTaggedValue> locales = GetHArg<JSTaggedValue>(argv, argc, 0); // 0: means the zeroth parameter
    JSHandle<EcmaString> thisHandle = GetHArg<EcmaString>(argv, argc, 1);    // 1: means the first parameter
    JSHandle<EcmaString> thatHandle = GetHArg<EcmaString>(argv, argc, 2);    // 2: means the second parameter
    JSHandle<JSTaggedValue> options = GetHArg<JSTaggedValue>(argv, argc, 3); // 3: means the third parameter
    bool cacheable = options->IsUndefined() && (locales->IsUndefined() || locales->IsString());
    const CompareStringsOption csOption = JSCollator::CompareStringsOptionFor(thread, locales, options);
    return builtins::BuiltinsString::LocaleCompareGC(thread, thisHandle, thatHandle, locales,
        options, csOption, cacheable).GetRawData();
}

DEF_RUNTIME_STUBS(ParseInt)
{
    RUNTIME_STUBS_HEADER(ParseInt);
    JSHandle<JSTaggedValue> msg = GetHArg<JSTaggedValue>(argv, argc, 0); // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> arg2 = GetHArg<JSTaggedValue>(argv, argc, 1);    // 1: means the first parameter

    int32_t radix = 0;
    // 1. Let inputString be ToString(string).
    JSHandle<EcmaString> numberString = JSTaggedValue::ToString(thread, msg);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
    if (!arg2->IsUndefined()) {
        // 7. Let R = ToInt32(radix).
        radix = JSTaggedValue::ToInt32(thread, arg2);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
    }

    return base::NumberHelper::StringToNumber(*numberString, radix).GetRawData();
}

int RuntimeStubs::FastArraySort(JSTaggedType x, JSTaggedType y)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSTaggedValue xValue = JSTaggedValue(x);
    JSTaggedValue yValue = JSTaggedValue(y);
    if (xValue.IsInt() && yValue.IsInt()) {
        return JSTaggedValue::IntLexicographicCompare(xValue, yValue);
    }
    if (xValue.IsDouble() && yValue.IsDouble()) {
        return JSTaggedValue::DoubleLexicographicCompare(xValue, yValue);
    }
    return -1;
}

int RuntimeStubs::FastArraySortString(uintptr_t argGlue, JSTaggedValue x, JSTaggedValue y)
{
    DISALLOW_GARBAGE_COLLECTION;
    auto thread = JSThread::GlueToJSThread(argGlue);
    JSHandle<EcmaString> valueX(thread, x);
    JSHandle<EcmaString> valueY(thread, y);
    return static_cast<int>(EcmaStringAccessor::Compare(thread->GetEcmaVM(), valueX, valueY));
}

DEF_RUNTIME_STUBS(LocaleCompareCacheable)
{
    RUNTIME_STUBS_HEADER(LocaleCompareCacheable);
    JSHandle<JSTaggedValue> locales = GetHArg<JSTaggedValue>(argv, argc, 0); // 0: means the zeroth parameter
    JSHandle<EcmaString> thisHandle = GetHArg<EcmaString>(argv, argc, 1);    // 1: means the first parameter
    JSHandle<EcmaString> thatHandle = GetHArg<EcmaString>(argv, argc, 2);    // 2: means the second parameter
#if ENABLE_NEXT_OPTIMIZATION
    const CompareStringsOption csOption = JSCollator::CompareStringsOptionFor(thread, locales);
    JSTaggedValue result = JSCollator::FastCachedCompareStrings(thread, locales, thisHandle, thatHandle, csOption);
#else
    auto collator = JSCollator::GetCachedIcuCollator(thread, locales);
    JSTaggedValue result = JSTaggedValue::Undefined();
    if (collator != nullptr) {
        [[maybe_unused]]const CompareStringsOption csOption = JSCollator::CompareStringsOptionFor(
            thread, locales);
        result = JSCollator::CompareStrings(thread, collator, thisHandle, thatHandle, csOption);
    }
#endif
    return result.GetRawData();
}

JSTaggedValue RuntimeStubs::StringToNumber(JSTaggedType numberString, int32_t radix)
{
    DISALLOW_GARBAGE_COLLECTION;
    auto input = EcmaString::Cast(JSTaggedValue(numberString));
    return base::NumberHelper::StringToNumber(input, radix);
}

void RuntimeStubs::ArrayTrim(uintptr_t argGlue, TaggedArray *array, int64_t newLength)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t length = static_cast<uint32_t>(newLength);
    auto thread = JSThread::GlueToJSThread(argGlue);
    array->Trim(thread, length);
}

bool RuntimeStubs::IsFastRegExp(uintptr_t argGlue, JSTaggedValue thisValue)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return builtins::BuiltinsRegExp::IsFastRegExp(thread, thisValue);
}

RememberedSet* RuntimeStubs::CreateLocalToShare(Region* region)
{
    return region->CreateLocalToShareRememberedSet();
}

RememberedSet* RuntimeStubs::CreateOldToNew(Region* region)
{
    return region->CreateOldToNewRememberedSet();
}

template <typename T>
static bool CompareFloat(T x, T y)
{
    if (x < y) {
        return true;
    }
    if (x > y) {
        return false;
    }
    if constexpr (!std::is_integral<T>::value) {
        double doubleX = x, doubleY = y;
        if (x == 0 && x == y) {
            /* -0.0 is less than +0.0 */
            return std::signbit(doubleX) && !std::signbit(doubleY);
        }
        if (!std::isnan(doubleX) && std::isnan(doubleY)) {
            /* number is less than NaN */
            return true;
        }
    }
    return false;
}

void RuntimeStubs::SortTypedArray(JSTypedArray *typedArray)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSHClass *hclass = typedArray->GetClass();
    const JSType jsType = hclass->GetObjectType();
    JSTaggedValue buffer = typedArray->GetViewedArrayBufferOrByteArray();
    const uint32_t len = typedArray->GetArrayLength();
    void *pointer = builtins::BuiltinsArrayBuffer::GetDataPointFromBuffer(buffer);
    switch (jsType) {
        case JSType::JS_INT8_ARRAY:
            std::sort(static_cast<int8_t*>(pointer), static_cast<int8_t*>(pointer) + len);
            break;
        case JSType::JS_UINT8_ARRAY:
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            std::sort(static_cast<uint8_t*>(pointer), static_cast<uint8_t*>(pointer) + len);
            break;
        case JSType::JS_INT16_ARRAY:
            std::sort(static_cast<int16_t*>(pointer), static_cast<int16_t*>(pointer) + len);
            break;
        case JSType::JS_UINT16_ARRAY:
            std::sort(static_cast<uint16_t*>(pointer), static_cast<uint16_t*>(pointer) + len);
            break;
        case JSType::JS_INT32_ARRAY:
            std::sort(static_cast<int32_t*>(pointer), static_cast<int32_t*>(pointer) + len);
            break;
        case JSType::JS_UINT32_ARRAY:
            std::sort(static_cast<uint32_t*>(pointer), static_cast<uint32_t*>(pointer) + len);
            break;
        case JSType::JS_FLOAT32_ARRAY:
            std::sort(static_cast<float*>(pointer), static_cast<float*>(pointer) + len, CompareFloat<float>);
            break;
        case JSType::JS_FLOAT64_ARRAY:
            std::sort(static_cast<double*>(pointer), static_cast<double*>(pointer) + len, CompareFloat<double>);
            break;
        default:
            UNREACHABLE();
    }
}

void RuntimeStubs::ReverseTypedArray(JSTypedArray *typedArray)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSHClass *hclass = typedArray->GetClass();
    const JSType jsType = hclass->GetObjectType();
    JSTaggedValue buffer = typedArray->GetViewedArrayBufferOrByteArray();
    const uint32_t len = typedArray->GetArrayLength();
    void *pointer = builtins::BuiltinsArrayBuffer::GetDataPointFromBuffer(buffer);
    switch (jsType) {
        case JSType::JS_INT8_ARRAY:
            std::reverse(static_cast<int8_t*>(pointer), static_cast<int8_t*>(pointer) + len);
            break;
        case JSType::JS_UINT8_ARRAY:
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            std::reverse(static_cast<uint8_t*>(pointer), static_cast<uint8_t*>(pointer) + len);
            break;
        case JSType::JS_INT16_ARRAY:
            std::reverse(static_cast<int16_t*>(pointer), static_cast<int16_t*>(pointer) + len);
            break;
        case JSType::JS_UINT16_ARRAY:
            std::reverse(static_cast<uint16_t*>(pointer), static_cast<uint16_t*>(pointer) + len);
            break;
        case JSType::JS_INT32_ARRAY:
            std::reverse(static_cast<int32_t*>(pointer), static_cast<int32_t*>(pointer) + len);
            break;
        case JSType::JS_UINT32_ARRAY:
            std::reverse(static_cast<uint32_t*>(pointer), static_cast<uint32_t*>(pointer) + len);
            break;
        case JSType::JS_FLOAT32_ARRAY:
            std::reverse(static_cast<float*>(pointer), static_cast<float*>(pointer) + len);
            break;
        case JSType::JS_FLOAT64_ARRAY:
            std::reverse(static_cast<double*>(pointer), static_cast<double*>(pointer) + len);
            break;
        default:
            UNREACHABLE();
    }
}

void RuntimeStubs::FinishObjSizeTracking(JSHClass *cls)
{
    uint32_t finalInObjPropsNum = JSHClass::VisitTransitionAndFindMaxNumOfProps(cls);
    if (finalInObjPropsNum < cls->GetInlinedProperties()) {
        // UpdateObjSize with finalInObjPropsNum
        JSHClass::VisitTransitionAndUpdateObjSize(cls, finalInObjPropsNum);
    }
}

void RuntimeStubs::FillObject(JSTaggedType *dst, JSTaggedType value, uint32_t count)
{
    DISALLOW_GARBAGE_COLLECTION;
    std::fill_n(dst, count, value);
}

DEF_RUNTIME_STUBS(TraceLoadSlowPath)
{
#if ECMASCRIPT_ENABLE_TRACE_LOAD
    auto thread = JSThread::GlueToJSThread(argGlue);
    auto target_bundle_name = thread->GetEcmaVM()->GetJSOptions().GetTraceLoadBundleName();
    if (thread->GetEcmaVM()->GetBundleName() != ConvertToString(target_bundle_name)) {
        return JSTaggedValue::Undefined().GetRawData();
    }

    ECMA_BYTRACE_START_TRACE(HITRACE_TAG_ARK, "[dfx]TraceLoadSlowPath");
#endif
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(TraceLoadGetter)
{
#if ECMASCRIPT_ENABLE_TRACE_LOAD
    auto thread = JSThread::GlueToJSThread(argGlue);
    auto target_bundle_name = thread->GetEcmaVM()->GetJSOptions().GetTraceLoadBundleName();
    if (thread->GetEcmaVM()->GetBundleName() != ConvertToString(target_bundle_name)) {
        return JSTaggedValue::Undefined().GetRawData();
    }

    ECMA_BYTRACE_START_TRACE(HITRACE_TAG_ARK, "[DFX]TraceLoadGetter");
#endif
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(TraceLoadDetail)
{
#if ECMASCRIPT_ENABLE_TRACE_LOAD
    auto thread = JSThread::GlueToJSThread(argGlue);
    auto target_bundle_name = thread->GetEcmaVM()->GetJSOptions().GetTraceLoadBundleName();
    if (thread->GetEcmaVM()->GetBundleName() != ConvertToString(target_bundle_name)) {
        return JSTaggedValue::Undefined().GetRawData();
    }

    JSHandle<JSTaggedValue> receiver = GetHArg<JSTaggedValue>(argv, argc, 0);
    JSHandle<JSTaggedValue> profile = GetHArg<JSTaggedValue>(argv, argc, 1);
    JSTaggedValue slotId = GetArg(argv, argc, 2);
    CString msg = "[DFX]Trace Load Detail: ";
    if (profile->IsUndefined()) {
        msg += "ProfileTypeInfo Undefine";
    } else {
        auto prof = JSHandle<ProfileTypeInfo>::Cast(profile);
        auto slot = slotId.GetInt();
        auto first = prof->GetIcSlot(slot);
        if (first.IsHole()) {
            auto second = prof->GetIcSlot(slot + 1);
            if (second.IsHole()) {
                msg += "other-mega, ";
            // 1: Call SetAsMegaDFX and set it to 1 (for placeholder purposes)..
            } else if (second == JSTaggedValue(ProfileTypeAccessor::MegaState::NOTFOUND_MEGA)) {
                msg += "not_found-mage, ";
            // 2: Call SetAsMegaDFX and set it to 2 (for placeholder purposes).
            } else if (second == JSTaggedValue(ProfileTypeAccessor::MegaState::DICT_MEGA)) {
                msg += "dictionary-mega, ";
            } else if (second.IsString()) {
                msg += "ic-mega, ";
            } else {
                msg += "unkown-mega, ";
            }
        } else if (first.IsUndefined()) {
            msg += "undedfine slot, ";
        } else if (first.IsWeak()) {
            msg += "mono, ";
        } else {
            msg += "poly, ";
        }
    }
    if (!receiver->IsHeapObject()) {
        msg += "prim_obj";
    } else {
        msg += "heap_obj";
    }
    ECMA_BYTRACE_START_TRACE(HITRACE_TAG_ARK, msg);
#endif
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(TraceLoadEnd)
{
#if ECMASCRIPT_ENABLE_TRACE_LOAD
    auto thread = JSThread::GlueToJSThread(argGlue);
    auto target_bundle_name = thread->GetEcmaVM()->GetJSOptions().GetTraceLoadBundleName();
    if (thread->GetEcmaVM()->GetBundleName() != ConvertToString(target_bundle_name)) {
        return JSTaggedValue::Undefined().GetRawData();
    }

    ECMA_BYTRACE_FINISH_TRACE(HITRACE_TAG_ARK);
#endif
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(ArrayForEachContinue)
{
    RUNTIME_STUBS_HEADER(ArrayForEachContinue);
    JSHandle<JSTaggedValue> thisArgHandle = GetHArg<JSTaggedValue>(argv, argc, 0);      // 0: means the zeroth parameter
    JSMutableHandle<JSTaggedValue> key(thread, GetHArg<JSTaggedValue>(argv, argc, 1));  // 1: means the first parameter
    JSHandle<JSTaggedValue> thisObjVal = GetHArg<JSTaggedValue>(argv, argc, 2);         // 2: means the second parameter
    JSHandle<JSTaggedValue> callbackFnHandle = GetHArg<JSTaggedValue>(argv, argc, 3);   // 3: means the third parameter
    JSHandle<JSTaggedValue> lengthHandle = GetHArg<JSTaggedValue>(argv, argc, 4);       // 4: means the fourth parameter
    const uint32_t argsLength = 3; // 3: «kValue, k, O»
    uint32_t i = static_cast<uint32_t>(key->GetInt());
    uint32_t len = static_cast<uint32_t>(lengthHandle->GetInt());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    while (i < len) {
        bool exists = JSTaggedValue::HasProperty(thread, thisObjVal, i);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
        if (exists) {
            JSHandle<JSTaggedValue> kValue = JSArray::FastGetPropertyByValue(thread, thisObjVal, i);
            key.Update(JSTaggedValue(i));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
            info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisObjVal.GetTaggedValue());
            JSTaggedValue funcResult = JSFunction::Call(info);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult.GetRawData());
        }
        i++;
    }

    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_STUBS(AOTEnableProtoChangeMarker)
{
    RUNTIME_STUBS_HEADER(AOTEnableProtoChangeMarker);
    JSHandle<JSFunction> result(GetHArg<JSTaggedValue>(argv, argc, 0)); // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> ihc = GetHArg<JSTaggedValue>(argv, argc, 1); // 1: means the first parameter
    JSHandle<AOTLiteralInfo> aotLiteralInfo(GetHArg<JSTaggedValue>(argv, argc, 2)); // 2: means the second parameter
    DefineFuncTryUseAOTHClass(thread, result, ihc, aotLiteralInfo);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(GetSharedModule)
{
    RUNTIME_STUBS_HEADER(GetSharedModule);
    JSHandle<JSTaggedValue> module = GetHArg<JSTaggedValue>(argv, argc, 0); // 0: means the zeroth parameter
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    return moduleManager->GenerateSendableFuncModule(module).GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(SetPrototypeTransition)
{
    RUNTIME_STUBS_HEADER(SetPrototypeTransition);
    JSHandle<JSObject> obj = GetHArg<JSObject>(argv, argc, 0); // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> proto = GetHArg<JSTaggedValue>(argv, argc, 1); // 1: means the third parameter
    ElementsKind oldKind = obj->GetJSHClass()->GetElementsKind();
    JSHClass::SetPrototypeTransition(thread, obj, proto);
    JSObject::TryMigrateToGenericKindForJSObject(thread, obj, oldKind);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(HasProperty)
{
    RUNTIME_STUBS_HEADER(HasProperty);
    JSHandle<JSTaggedValue> obj = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSTaggedValue indexValue = GetArg(argv, argc, 1);  // 1: means the first parameter
    uint32_t index = static_cast<uint32_t>(indexValue.GetInt());
    bool res = JSTaggedValue::HasProperty(thread, obj, index);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
    return JSTaggedValue(res).GetRawData();
}

DEF_RUNTIME_STUBS(ObjectPrototypeHasOwnProperty)
{
    RUNTIME_STUBS_HEADER(ObjectPrototypeHasOwnProperty);
    JSHandle<JSTaggedValue> thisValue = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> key = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSTaggedValue result = builtins::BuiltinsObject::HasOwnPropertyInternal(thread, thisValue, key);
    return result.GetRawData();
}

DEF_RUNTIME_STUBS(ReflectHas)
{
    RUNTIME_STUBS_HEADER(ReflectHas);
    JSHandle<JSTaggedValue> target = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> key = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSTaggedValue result = builtins::BuiltinsReflect::ReflectHasInternal(thread, target, key);
    return result.GetRawData();
}

DEF_RUNTIME_STUBS(ReflectConstruct)
{
    // newTarget = target, args = []
    RUNTIME_STUBS_HEADER(ReflectConstruct);
    JSHandle<JSTaggedValue> target = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<TaggedArray> args = thread->GetEcmaVM()->GetFactory()->EmptyArray();
    JSTaggedValue result = builtins::BuiltinsReflect::ReflectConstructInternal(thread, target, args, target);
    return result.GetRawData();
}

DEF_RUNTIME_STUBS(ReflectApply)
{
    RUNTIME_STUBS_HEADER(ReflectApply);
    JSHandle<JSTaggedValue> target = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> thisValue = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> argumentsList = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    JSTaggedValue result = builtins::BuiltinsReflect::ReflectApplyInternal(thread, target, thisValue, argumentsList);
    return result.GetRawData();
}

DEF_RUNTIME_STUBS(FunctionPrototypeApply)
{
    RUNTIME_STUBS_HEADER(FunctionPrototypeApply);
    JSHandle<JSTaggedValue> thisFunc = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> thisArg = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<JSTaggedValue> argArray = GetHArg<JSTaggedValue>(argv, argc, 2);  // 2: means the second parameter
    JSTaggedValue result = builtins::BuiltinsFunction::FunctionPrototypeApplyInternal(thread, thisFunc,
                                                                                      thisArg, argArray);
    return result.GetRawData();
}

DEF_RUNTIME_STUBS(FunctionPrototypeBind)
{
    RUNTIME_STUBS_HEADER(FunctionPrototypeBind);
    JSHandle<JSTaggedValue> target = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> thisArg = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    JSHandle<TaggedArray> argsArray = thread->GetEcmaVM()->GetFactory()->EmptyArray();
    JSTaggedValue result = builtins::BuiltinsFunction::FunctionPrototypeBindInternal(thread, target,
                                                                                     thisArg, argsArray);
    return result.GetRawData();
}

DEF_RUNTIME_STUBS(FunctionPrototypeCall)
{
    RUNTIME_STUBS_HEADER(FunctionPrototypeCall);
    JSHandle<JSTaggedValue> thisFunc = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    JSHandle<JSTaggedValue> thisArg = GetHArg<JSTaggedValue>(argv, argc, 1);  // 1: means the first parameter
    if (!thisFunc->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "call target is not callable", JSTaggedValue::VALUE_EXCEPTION);
    }
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    uint32_t argsLength = argc - 2;  // 2: thisFunc and thisArg
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, thisFunc, thisArg, undefined, argsLength);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::VALUE_EXCEPTION);
    uint32_t index = 0;
    for (uint32_t i = 2; i < argc; ++i) {  // 2: thisFunc and thisArg
        JSTaggedValue arg = GetArg(argv, argc, i);
        info->SetCallArg(index++, arg);
    }
    return JSFunction::Call(info).GetRawData();
}

DEF_RUNTIME_STUBS(GetCollationValueFromIcuCollator)
{
    RUNTIME_STUBS_HEADER(GetCollationValueFromIcuCollator);
    JSHandle<JSCollator> collator = GetHArg<JSCollator>(argv, argc, 0);  // 0: means the zeroth parameter

    UErrorCode status = U_ZERO_ERROR;
    icu::Collator *icuCollator = collator->GetIcuCollator();
    icu::Locale icu_locale(icuCollator->getLocale(ULOC_VALID_LOCALE, status));
    std::string collation_value =
        icu_locale.getUnicodeKeywordValue<std::string>("co", status);
    if (collation_value != "search" && collation_value != "") {
        return thread->GetEcmaVM()->GetFactory()->NewFromStdString(collation_value).GetTaggedValue().GetRawData();
    }
    return thread->GlobalConstants()->GetDefaultString().GetRawData();
}

DEF_RUNTIME_STUBS(GetAllFlagsInternal)
{
    RUNTIME_STUBS_HEADER(GetAllFlagsInternal);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    uint64_t bigFlagsStr = static_cast<uint64_t>(value->GetInt());
    std::string strFlags = "";
    strFlags.reserve(RegExpParser::FLAG_NUM);
    if (bigFlagsStr & RegExpParser::FLAG_HASINDICES) {
        strFlags += "d";
    }
    if (bigFlagsStr & RegExpParser::FLAG_GLOBAL) {
        strFlags += "g";
    }
    if (bigFlagsStr & RegExpParser::FLAG_IGNORECASE) {
        strFlags += "i";
    }
    if (bigFlagsStr & RegExpParser::FLAG_MULTILINE) {
        strFlags += "m";
    }
    if (bigFlagsStr & RegExpParser::FLAG_DOTALL) {
        strFlags += "s";
    }
    if (bigFlagsStr & RegExpParser::FLAG_UTF16) {
        strFlags += "u";
    }
    if (bigFlagsStr & RegExpParser::FLAG_STICKY) {
        strFlags += "y";
    }
    JSHandle<EcmaString> flagsString = factory->NewFromUtf8(std::string_view(strFlags));
    return flagsString.GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(SlowSharedObjectStoreBarrier)
{
    RUNTIME_STUBS_HEADER(SlowSharedObjectStoreBarrier);
    JSHandle<JSTaggedValue> value = GetHArg<JSTaggedValue>(argv, argc, 0);  // 0: means the zeroth parameter
    ASSERT(value->IsTreeString());
    JSHandle<JSTaggedValue> publishValue = JSTaggedValue::PublishSharedValueSlow(thread, value);
    return publishValue.GetTaggedValue().GetRawData();
}

void RuntimeStubs::ObjectCopy(JSTaggedType *dst, JSTaggedType *src, uint32_t count)
{
    DISALLOW_GARBAGE_COLLECTION;
    std::copy_n(src, count, dst);
}

void RuntimeStubs::ReverseArray(JSTaggedType *dst, uint32_t length)
{
    DISALLOW_GARBAGE_COLLECTION;
    std::reverse(dst, dst + length);
}

void RuntimeStubs::Initialize(JSThread *thread)
{
#define DEF_RUNTIME_STUB(name) kungfu::RuntimeStubCSigns::ID_##name
#define INITIAL_RUNTIME_FUNCTIONS(name) \
    thread->RegisterRTInterface(DEF_RUNTIME_STUB(name), reinterpret_cast<uintptr_t>(name));
    RUNTIME_STUB_WITHOUT_GC_LIST(INITIAL_RUNTIME_FUNCTIONS)
    RUNTIME_STUB_WITH_GC_LIST(INITIAL_RUNTIME_FUNCTIONS)
    RUNTIME_STUB_WITH_DFX(INITIAL_RUNTIME_FUNCTIONS)
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
