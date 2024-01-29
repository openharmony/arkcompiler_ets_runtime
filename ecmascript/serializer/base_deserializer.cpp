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

#include "ecmascript/serializer/base_deserializer.h"

#include "ecmascript/ecma_string_table.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/sparse_space.h"
#include "ecmascript/platform/mutex.h"
#include "ecmascript/runtime.h"
#include "ecmascript/runtime_lock.h"

namespace panda::ecmascript {

#define NEW_OBJECT_ALL_SPACES()                                           \
    (uint8_t)SerializedObjectSpace::OLD_SPACE:                            \
    case (uint8_t)SerializedObjectSpace::NON_MOVABLE_SPACE:               \
    case (uint8_t)SerializedObjectSpace::MACHINE_CODE_SPACE:              \
    case (uint8_t)SerializedObjectSpace::HUGE_SPACE

JSHandle<JSTaggedValue> BaseDeserializer::ReadValue()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "Deserialize dataSize: " + std::to_string(data_->Size()));
    AllocateToDifferentSpaces();
    JSHandle<JSTaggedValue> res = DeserializeJSTaggedValue();
    return res;
}

JSHandle<JSTaggedValue> BaseDeserializer::DeserializeJSTaggedValue()
{
    if (data_->IsIncompleteData()) {
        LOG_ECMA(ERROR) << "The serialization data is incomplete";
        return JSHandle<JSTaggedValue>();
    }

    // stop gc during deserialize
    heap_->SetOnSerializeEvent(true);

    uint8_t encodeFlag = data_->ReadUint8();
    JSTaggedType result = 0U;
    while (ReadSingleEncodeData(encodeFlag, ToUintPtr(&result), true) == 0) {
        encodeFlag = data_->ReadUint8();
    }
    // now new constpool here if newConstPoolInfos_ is not empty
    for (auto newConstpoolInfo : newConstPoolInfos_) {
        DeserializeConstPool(newConstpoolInfo);
        delete newConstpoolInfo;
    }
    newConstPoolInfos_.clear();

    // initialize concurrent func here after constpool is set
    for (auto func : concurrentFunctions_) {
        func->InitializeForConcurrentFunction(thread_);
    }
    concurrentFunctions_.clear();

    // new native binding object here
    for (auto nativeBindingInfo : nativeBindingInfos_) {
        DeserializeNativeBindingObject(nativeBindingInfo);
        delete nativeBindingInfo;
    }
    nativeBindingInfos_.clear();

    // new js error here
    for (auto jsErrorInfo : jsErrorInfos_) {
        DeserializeJSError(jsErrorInfo);
        delete jsErrorInfo;
    }
    jsErrorInfos_.clear();

    // recovery gc after serialize
    heap_->SetOnSerializeEvent(false);

    // If is on concurrent mark, push root object to stack for mark
    if (JSTaggedValue(result).IsHeapObject() && thread_->IsConcurrentMarkingOrFinished()) {
        Region *valueRegion = Region::ObjectAddressToRange(static_cast<uintptr_t>(result));
        TaggedObject *heapValue = JSTaggedValue(result).GetHeapObject();
        if (valueRegion->AtomicMark(heapValue)) {
            heap_->GetWorkManager()->Push(0, heapValue, valueRegion);
        }
    }

    return JSHandle<JSTaggedValue>(thread_, JSTaggedValue(result));
}

uintptr_t BaseDeserializer::DeserializeTaggedObject(SerializedObjectSpace space)
{
    size_t objSize = data_->ReadUint32();
    uintptr_t res = RelocateObjectAddr(space, objSize);
    objectVector_.push_back(res);
    size_t resIndex = objectVector_.size() - 1;
    DeserializeObjectField(res, res + objSize);
    JSType type = reinterpret_cast<TaggedObject *>(res)->GetClass()->GetObjectType();
    // String need remove duplicates if string table can find
    if (type == JSType::LINE_STRING || type == JSType::CONSTANT_STRING) {
        EcmaStringTable *stringTable = thread_->GetEcmaVM()->GetEcmaStringTable();
        RuntimeLockHolder locker(thread_, stringTable->mutex_);
        EcmaString *str = stringTable->GetStringThreadUnsafe(reinterpret_cast<EcmaString *>(res));
        if (str) {
            res = ToUintPtr(str);
            objectVector_[resIndex] = res;
        } else {
            EcmaStringAccessor(reinterpret_cast<EcmaString *>(res)).ClearInternString();
            stringTable->InternStringThreadUnsafe(reinterpret_cast<EcmaString *>(res));
        }
    }
    return res;
}

void BaseDeserializer::DeserializeObjectField(uintptr_t start, uintptr_t end)
{
    while (start < end) {
        uint8_t encodeFlag = data_->ReadUint8();
        start += ReadSingleEncodeData(encodeFlag, start);
    }
}

void BaseDeserializer::DeserializeConstPool(NewConstPoolInfo *info)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSPandaFile *jsPandaFile = info->jsPandaFile_;
    panda_file::File::EntityId methodId = info->methodId_;
    ObjectSlot slot = ObjectSlot(info->slotAddr_);
    JSHandle<ConstantPool> constpool = ConstantPool::CreateSharedConstPool(thread_->GetEcmaVM(), jsPandaFile, methodId);
    panda_file::IndexAccessor indexAccessor(*jsPandaFile->GetPandaFile(), methodId);
    int32_t index = static_cast<int32_t>(indexAccessor.GetHeaderIndex());
    thread_->GetCurrentEcmaContext()->AddConstpool(jsPandaFile, constpool.GetTaggedValue(), index);
    slot.Update(constpool.GetTaggedType());
    UpdateBarrier(constpool.GetTaggedType(), slot);
}

void BaseDeserializer::DeserializeNativeBindingObject(NativeBindingInfo *info)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    AttachFunc af = info->af_;
    void *bufferPointer = info->bufferPointer_;
    void *hint = info->hint_;
    void *attachData = info->attachData_;
    ObjectSlot slot = info->slot_;
    bool root = info->root_;
    Local<JSValueRef> attachVal = af(engine_, bufferPointer, hint, attachData);
    if (attachVal.IsEmpty()) {
        LOG_ECMA(ERROR) << "NativeBindingObject is empty";
        attachVal = JSValueRef::Undefined(thread_->GetEcmaVM());
    }
    JSTaggedType res = JSNApiHelper::ToJSHandle(attachVal).GetTaggedType();
    slot.Update(res);
    if (!root) {
        UpdateBarrier(res, slot);
    }
}

void BaseDeserializer::DeserializeJSError(JSErrorInfo *info)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    uint8_t type = info->errorType_;
    base::ErrorType errorType = base::ErrorType(type - static_cast<uint8_t>(JSType::JS_ERROR_FIRST));
    JSTaggedValue errorMsg = info->errorMsg_;
    ObjectSlot slot = info->slot_;
    bool root = info->root_;
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> errorTag = factory->NewJSError(errorType, JSHandle<EcmaString>(thread_, errorMsg));
    slot.Update(errorTag.GetTaggedType());
    if (!root) {
        UpdateBarrier(errorTag.GetTaggedType(), slot);
    }
}

void BaseDeserializer::HandleNewObjectEncodeFlag(SerializedObjectSpace space, ObjectSlot slot, bool isRoot)
{
    // deserialize object prologue
    bool isWeak = GetAndResetWeak();
    bool isTransferBuffer = GetAndResetTransferBuffer();
    void *bufferPointer = GetAndResetBufferPointer();
    ConstantPool *constpool = GetAndResetConstantPool();
    bool needNewConstPool = GetAndResetNeedNewConstPool();
    bool isErrorMsg = GetAndResetIsErrorMsg();
    bool functionInShared = GetAndResetFunctionInShared();

    // deserialize object here
    uintptr_t addr = DeserializeTaggedObject(space);

    // deserialize object epilogue
    if (isErrorMsg) {
        // defer new js error
        jsErrorInfos_.back()->errorMsg_ = JSTaggedValue(static_cast<JSTaggedType>(addr));
        return;
    }

    if (isTransferBuffer) {
        TransferArrayBufferAttach(addr);
    } else if (bufferPointer != nullptr) {
        ResetNativePointerBuffer(addr, bufferPointer);
    } else if (constpool != nullptr) {
        ResetMethodConstantPool(addr, constpool);
    } else if (needNewConstPool) {
        // defer new constpool
        newConstPoolInfos_.back()->slotAddr_ = addr + Method::CONSTANT_POOL_OFFSET;
    }
    if (functionInShared) {
        concurrentFunctions_.push_back(reinterpret_cast<JSFunction *>(addr));
    }
    TaggedObject *object = reinterpret_cast<TaggedObject *>(addr);
    if (object->GetClass()->IsJSNativePointer()) {
        JSNativePointer *nativePointer = reinterpret_cast<JSNativePointer *>(object);
        if (nativePointer->GetDeleter() != nullptr) {
            thread_->GetEcmaVM()->PushToNativePointerList(nativePointer);
        }
    } else if (object->GetClass()->IsJSFunction()) {
        JSFunction* func = reinterpret_cast<JSFunction *>(object);
        FunctionKind funcKind = func->GetFunctionKind();
        if (funcKind == FunctionKind::CONCURRENT_FUNCTION) {
            // defer initialize concurrent function until constpool is set
            concurrentFunctions_.push_back(reinterpret_cast<JSFunction *>(object));
        }
    }
    UpdateMaybeWeak(slot, addr, isWeak);
    if (!isRoot) {
        UpdateBarrier(addr, slot);
    }
}

void BaseDeserializer::HandleMethodEncodeFlag()
{
    panda_file::File::EntityId methodId = MethodLiteral::GetMethodId(data_->ReadJSTaggedType());
    JSPandaFile *jsPandaFile = reinterpret_cast<JSPandaFile *>(data_->ReadJSTaggedType());
    panda_file::IndexAccessor indexAccessor(*jsPandaFile->GetPandaFile(), methodId);
    int32_t index = static_cast<int32_t>(indexAccessor.GetHeaderIndex());
    JSTaggedValue constpool = thread_->GetCurrentEcmaContext()->FindConstpoolWithAOT(jsPandaFile, index);
    if (constpool.IsHole()) {
        LOG_ECMA(ERROR) << "ValueDeserialize: function deserialize can't find constpool from panda file: "
            << jsPandaFile;
        // defer new constpool until deserialize finish
        needNewConstPool_ = true;
        newConstPoolInfos_.push_back(new NewConstPoolInfo(jsPandaFile, methodId));
    } else {
        constpool_ = reinterpret_cast<ConstantPool *>(constpool.GetTaggedObject());
    }
}

void BaseDeserializer::TransferArrayBufferAttach(uintptr_t objAddr)
{
    ASSERT(JSTaggedValue(static_cast<JSTaggedType>(objAddr)).IsArrayBuffer());
    JSArrayBuffer *arrayBuffer = reinterpret_cast<JSArrayBuffer *>(objAddr);
    size_t arrayLength = arrayBuffer->GetArrayBufferByteLength();
    bool withNativeAreaAllocator = arrayBuffer->GetWithNativeAreaAllocator();
    JSNativePointer *np = reinterpret_cast<JSNativePointer *>(arrayBuffer->GetArrayBufferData().GetTaggedObject());
    arrayBuffer->Attach(thread_, arrayLength, JSTaggedValue(np), withNativeAreaAllocator);
}

void BaseDeserializer::ResetNativePointerBuffer(uintptr_t objAddr, void *bufferPointer)
{
    JSTaggedValue obj = JSTaggedValue(static_cast<JSTaggedType>(objAddr));
    ASSERT(obj.IsArrayBuffer() || obj.IsJSRegExp());
    auto nativeAreaAllocator = thread_->GetEcmaVM()->GetNativeAreaAllocator();
    JSNativePointer *np = nullptr;
    if (obj.IsArrayBuffer()) {
        JSArrayBuffer *arrayBuffer = reinterpret_cast<JSArrayBuffer *>(objAddr);
        arrayBuffer->SetWithNativeAreaAllocator(true);
        np = reinterpret_cast<JSNativePointer *>(arrayBuffer->GetArrayBufferData().GetTaggedObject());
        nativeAreaAllocator->IncreaseNativeSizeStats(arrayBuffer->GetArrayBufferByteLength(), NativeFlag::ARRAY_BUFFER);
    } else {
        JSRegExp *jsRegExp = reinterpret_cast<JSRegExp *>(objAddr);
        np = reinterpret_cast<JSNativePointer *>(jsRegExp->GetByteCodeBuffer().GetTaggedObject());
        nativeAreaAllocator->IncreaseNativeSizeStats(jsRegExp->GetLength(), NativeFlag::REGEXP_BTYECODE);
    }

    np->SetExternalPointer(bufferPointer);
    np->SetDeleter(NativeAreaAllocator::FreeBufferFunc);
    np->SetData(thread_->GetEcmaVM()->GetNativeAreaAllocator());
}

void BaseDeserializer::ResetMethodConstantPool(uintptr_t objAddr, ConstantPool *constpool)
{
    ASSERT(JSTaggedValue(static_cast<JSTaggedType>(objAddr)).IsMethod());
    Method *method = reinterpret_cast<Method *>(objAddr);
    method->SetConstantPool(thread_, JSTaggedValue(constpool), BarrierMode::SKIP_BARRIER);
}

size_t BaseDeserializer::ReadSingleEncodeData(uint8_t encodeFlag, uintptr_t addr, bool isRoot)
{
    size_t handledFieldSize = sizeof(JSTaggedType);
    ObjectSlot slot(addr);
    switch (encodeFlag) {
        case NEW_OBJECT_ALL_SPACES(): {
            SerializedObjectSpace space = SerializeData::DecodeSpace(encodeFlag);
            HandleNewObjectEncodeFlag(space, slot, isRoot);
            break;
        }
        case (uint8_t)EncodeFlag::REFERENCE: {
            uint32_t objIndex = data_->ReadUint32();
            uintptr_t objAddr = objectVector_[objIndex];
            UpdateMaybeWeak(slot, objAddr, GetAndResetWeak());
            UpdateBarrier(objAddr, slot);
            break;
        }
        case (uint8_t)EncodeFlag::WEAK: {
            ASSERT(!isWeak_);
            isWeak_ = true;
            handledFieldSize = 0;
            break;
        }
        case (uint8_t)EncodeFlag::PRIMITIVE: {
            JSTaggedType value = data_->ReadJSTaggedType();
            slot.Update(value);
            break;
        }
        case (uint8_t)EncodeFlag::MULTI_RAW_DATA: {
            uint32_t size = data_->ReadUint32();
            data_->ReadRawData(addr, size);
            handledFieldSize = size;
            break;
        }
        case (uint8_t)EncodeFlag::ROOT_OBJECT: {
            uint32_t index = data_->ReadUint32();
            uintptr_t objAddr = thread_->GetEcmaVM()->GetSnapshotEnv()->RelocateRootObjectAddr(index);
            if (!isRoot) {
                UpdateBarrier(objAddr, slot);
            }
            UpdateMaybeWeak(slot, objAddr, GetAndResetWeak());
            break;
        }
        case (uint8_t)EncodeFlag::OBJECT_PROTO: {
            uint8_t type = data_->ReadUint8();
            uintptr_t protoAddr = RelocateObjectProtoAddr(type);
            if (!isRoot) {
                UpdateBarrier(protoAddr, slot);
            }
            UpdateMaybeWeak(slot, protoAddr, GetAndResetWeak());
            break;
        }
        case (uint8_t)EncodeFlag::TRANSFER_ARRAY_BUFFER: {
            isTransferArrayBuffer_ = true;
            handledFieldSize = 0;
            break;
        }
        case (uint8_t)EncodeFlag::ARRAY_BUFFER:
        case (uint8_t)EncodeFlag::JS_REG_EXP: {
            size_t bufferLength = data_->ReadUint32();
            auto nativeAreaAllocator = thread_->GetEcmaVM()->GetNativeAreaAllocator();
            bufferPointer_ = nativeAreaAllocator->AllocateBuffer(bufferLength);
            data_->ReadRawData(ToUintPtr(bufferPointer_), bufferLength);
            heap_->IncreaseNativeBindingSize(bufferLength);
            handledFieldSize = 0;
            break;
        }
        case (uint8_t)EncodeFlag::METHOD: {
            HandleMethodEncodeFlag();
            handledFieldSize = 0;
            break;
        }
        case (uint8_t)EncodeFlag::NATIVE_BINDING_OBJECT: {
            slot.Update(JSTaggedValue::Undefined().GetRawData());
            AttachFunc af = reinterpret_cast<AttachFunc>(data_->ReadJSTaggedType());
            void *bufferPointer = reinterpret_cast<void *>(data_->ReadJSTaggedType());
            void *hint = reinterpret_cast<void *>(data_->ReadJSTaggedType());
            void *attachData = reinterpret_cast<void *>(data_->ReadJSTaggedType());
            // defer new native binding object until deserialize finish
            nativeBindingInfos_.push_back(new NativeBindingInfo(af, bufferPointer, hint, attachData, slot, isRoot));
            break;
        }
        case (uint8_t)EncodeFlag::JS_ERROR: {
            uint8_t type = data_->ReadUint8();
            ASSERT(type >= static_cast<uint8_t>(JSType::JS_ERROR_FIRST)
                && type <= static_cast<uint8_t>(JSType::JS_ERROR_LAST));
            jsErrorInfos_.push_back(new JSErrorInfo(type, JSTaggedValue::Undefined(), slot, isRoot));
            uint8_t flag = data_->ReadUint8();
            if (flag == 1) { // error msg is string
                isErrorMsg_ = true;
                handledFieldSize = 0;
            }
            break;
        }
        case (uint8_t)EncodeFlag::JS_FUNCTION_IN_SHARED: {
            functionInShared_ = true;
            handledFieldSize = 0;
            break;
        }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
    }
    return handledFieldSize;
}

void BaseDeserializer::UpdateBarrier(uintptr_t addr, ObjectSlot slot)
{
    Region *valueRegion = Region::ObjectAddressToRange(addr);
    if (valueRegion == nullptr) {
        return;
    }
    Region *rootRegion = Region::ObjectAddressToRange(slot.SlotAddress());
    // root region is impossible in young space when deserialize
    if (valueRegion->InYoungSpace()) {
        // Should align with '8' in 64 and 32 bit platform
        ASSERT(slot.SlotAddress() % static_cast<uint8_t>(MemAlignment::MEM_ALIGN_OBJECT) == 0);
        rootRegion->InsertOldToNewRSet(slot.SlotAddress());
    }
    if (!rootRegion->InSharedSpace() && valueRegion->InSharedSpace()) {
        rootRegion->AtomicInsertLocalToShareRset(slot.SlotAddress());
    }
    if (thread_->IsConcurrentMarkingOrFinished()) {
        Barriers::Update(thread_, slot.SlotAddress(), rootRegion, reinterpret_cast<TaggedObject *>(addr), valueRegion,
                         true);
    }
}

uintptr_t BaseDeserializer::RelocateObjectAddr(SerializedObjectSpace space, size_t objSize)
{
    uintptr_t res = 0U;
    switch (space) {
        case SerializedObjectSpace::OLD_SPACE: {
            if (oldSpaceBeginAddr_ + objSize > AlignUp(oldSpaceBeginAddr_, DEFAULT_REGION_SIZE)) {
                ASSERT(oldRegionIndex_ < regionVector_.size());
                oldSpaceBeginAddr_ = regionVector_[oldRegionIndex_++]->GetBegin();
            }
            res = oldSpaceBeginAddr_;
            oldSpaceBeginAddr_ += objSize;
            break;
        }
        case SerializedObjectSpace::NON_MOVABLE_SPACE: {
            if (nonMovableSpaceBeginAddr_ + objSize > AlignUp(nonMovableSpaceBeginAddr_, DEFAULT_REGION_SIZE)) {
                ASSERT(nonMovableRegionIndex_ < regionVector_.size());
                nonMovableSpaceBeginAddr_ = regionVector_[nonMovableRegionIndex_++]->GetBegin();
            }
            res = nonMovableSpaceBeginAddr_;
            nonMovableSpaceBeginAddr_ += objSize;
            break;
        }
        case SerializedObjectSpace::MACHINE_CODE_SPACE: {
            if (machineCodeSpaceBeginAddr_ + objSize > AlignUp(machineCodeSpaceBeginAddr_, DEFAULT_REGION_SIZE)) {
                ASSERT(machineCodeRegionIndex_ < regionVector_.size());
                machineCodeSpaceBeginAddr_ = regionVector_[machineCodeRegionIndex_++]->GetBegin();
            }
            res = nonMovableSpaceBeginAddr_;
            nonMovableSpaceBeginAddr_ += objSize;
            break;
        }
        default:
            // no gc for this allocate
            res = heap_->GetHugeObjectSpace()->Allocate(objSize);
            break;
    }
    return res;
}

JSTaggedType BaseDeserializer::RelocateObjectProtoAddr(uint8_t objectType)
{
    auto env = thread_->GetEcmaVM()->GetGlobalEnv();
    switch (objectType) {
        case (uint8_t)JSType::JS_OBJECT:
            return env->GetObjectFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_ERROR:
            return JSHandle<JSFunction>(env->GetErrorFunction())->GetFunctionPrototype().GetRawData();
        case (uint8_t)JSType::JS_EVAL_ERROR:
            return JSHandle<JSFunction>(env->GetEvalErrorFunction())->GetFunctionPrototype().GetRawData();
        case (uint8_t)JSType::JS_RANGE_ERROR:
            return JSHandle<JSFunction>(env->GetRangeErrorFunction())->GetFunctionPrototype().GetRawData();
        case (uint8_t)JSType::JS_REFERENCE_ERROR:
            return JSHandle<JSFunction>(env->GetReferenceErrorFunction())->GetFunctionPrototype().GetRawData();
        case (uint8_t)JSType::JS_TYPE_ERROR:
            return JSHandle<JSFunction>(env->GetTypeErrorFunction())->GetFunctionPrototype().GetRawData();
        case (uint8_t)JSType::JS_AGGREGATE_ERROR:
            return JSHandle<JSFunction>(env->GetAggregateErrorFunction())->GetFunctionPrototype().GetRawData();
        case (uint8_t)JSType::JS_URI_ERROR:
            return JSHandle<JSFunction>(env->GetURIErrorFunction())->GetFunctionPrototype().GetRawData();
        case (uint8_t)JSType::JS_SYNTAX_ERROR:
            return JSHandle<JSFunction>(env->GetSyntaxErrorFunction())->GetFunctionPrototype().GetRawData();
        case (uint8_t)JSType::JS_OOM_ERROR:
            return JSHandle<JSFunction>(env->GetOOMErrorFunction())->GetFunctionPrototype().GetRawData();
        case (uint8_t)JSType::JS_TERMINATION_ERROR:
            return JSHandle<JSFunction>(env->GetTerminationErrorFunction())->GetFunctionPrototype().GetRawData();
        case (uint8_t)JSType::JS_DATE:
            return env->GetDatePrototype().GetTaggedType();
        case (uint8_t)JSType::JS_ARRAY:
            return env->GetArrayPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_MAP:
            return env->GetMapPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_SET:
            return env->GetSetPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_REG_EXP:
            return env->GetRegExpPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_INT8_ARRAY:
            return env->GetInt8ArrayFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_UINT8_ARRAY:
            return env->GetUint8ArrayFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_UINT8_CLAMPED_ARRAY:
            return env->GetUint8ClampedArrayFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_INT16_ARRAY:
            return env->GetInt16ArrayFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_UINT16_ARRAY:
            return env->GetUint16ArrayFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_INT32_ARRAY:
            return env->GetInt32ArrayFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_UINT32_ARRAY:
            return env->GetUint32ArrayFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_FLOAT32_ARRAY:
            return env->GetFloat32ArrayFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_FLOAT64_ARRAY:
            return env->GetFloat64ArrayFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_BIGINT64_ARRAY:
            return env->GetBigInt64ArrayFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_BIGUINT64_ARRAY:
            return env->GetBigUint64ArrayFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_ARRAY_BUFFER:
            return JSHandle<JSFunction>(env->GetArrayBufferFunction())->GetFunctionPrototype().GetRawData();
        case (uint8_t)JSType::JS_SHARED_ARRAY_BUFFER:
            return JSHandle<JSFunction>(env->GetSharedArrayBufferFunction())->GetFunctionPrototype().GetRawData();
        case (uint8_t)JSType::JS_ASYNC_FUNCTION:
            return env->GetAsyncFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::BIGINT:
            return JSHandle<JSFunction>(env->GetBigIntFunction())->GetFunctionPrototype().GetRawData();
        default:
            LOG_ECMA(ERROR) << "Relocate unsupported JSType: " << JSHClass::DumpJSType(static_cast<JSType>(objectType));
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
    }
}

void BaseDeserializer::AllocateToDifferentSpaces()
{
    size_t oldSpaceSize = data_->GetOldSpaceSize();
    if (oldSpaceSize > 0) {
        heap_->GetOldSpace()->IncreaseLiveObjectSize(oldSpaceSize);
        AllocateToOldSpace(oldSpaceSize);
    }
    size_t nonMovableSpaceSize = data_->GetNonMovableSpaceSize();
    if (nonMovableSpaceSize > 0) {
        heap_->GetNonMovableSpace()->IncreaseLiveObjectSize(nonMovableSpaceSize);
        AllocateToNonMovableSpace(nonMovableSpaceSize);
    }
    size_t machineCodeSpaceSize = data_->GetMachineCodeSpaceSize();
    if (machineCodeSpaceSize > 0) {
        heap_->GetMachineCodeSpace()->IncreaseLiveObjectSize(machineCodeSpaceSize);
        AllocateToMachineCodeSpace(machineCodeSpaceSize);
    }
}

void BaseDeserializer::AllocateMultiRegion(SparseSpace *space, size_t spaceObjSize, size_t &regionIndex)
{
    regionIndex = regionVector_.size();
    size_t regionAlignedSize = SerializeData::AlignUpRegionAvailableSize(spaceObjSize);
    size_t regionNum = regionAlignedSize / Region::GetRegionAvailableSize();
    while (regionNum > 1) { // 1: one region have allocated before
        std::vector<size_t> regionRemainSizeVector = data_->GetRegionRemainSizeVector();
        space->ResetTopPointer(space->GetCurrentRegion()->GetEnd() - regionRemainSizeVector[regionRemainSizeIndex_++]);
        if (!space->Expand()) {
            LOG_ECMA(FATAL) << "BaseDeserializer::OutOfMemory when deserialize";
        }
        regionVector_.push_back(space->GetCurrentRegion());
        regionNum--;
    }
    size_t lastRegionRemainSize = regionAlignedSize - spaceObjSize;
    space->ResetTopPointer(space->GetCurrentRegion()->GetEnd() - lastRegionRemainSize);
}

void BaseDeserializer::AllocateToOldSpace(size_t oldSpaceSize)
{
    SparseSpace *space = heap_->GetOldSpace();
    uintptr_t object = space->Allocate(oldSpaceSize, false);
    if (UNLIKELY(object == 0U)) {
        oldSpaceBeginAddr_ = space->GetCurrentRegion()->GetBegin();
        AllocateMultiRegion(space, oldSpaceSize, oldRegionIndex_);
    } else {
        oldSpaceBeginAddr_ = object;
    }
}

void BaseDeserializer::AllocateToNonMovableSpace(size_t nonMovableSpaceSize)
{
    SparseSpace *space = heap_->GetNonMovableSpace();
    uintptr_t object = space->Allocate(nonMovableSpaceSize, false);
    if (UNLIKELY(object == 0U)) {
        nonMovableSpaceBeginAddr_ = space->GetCurrentRegion()->GetBegin();
        AllocateMultiRegion(space, nonMovableSpaceSize, nonMovableRegionIndex_);
    } else {
        nonMovableSpaceBeginAddr_ = object;
    }
}

void BaseDeserializer::AllocateToMachineCodeSpace(size_t machineCodeSpaceSize)
{
    SparseSpace *space = heap_->GetMachineCodeSpace();
    uintptr_t object = space->Allocate(machineCodeSpaceSize, false);
    if (UNLIKELY(object == 0U)) {
        machineCodeSpaceBeginAddr_ = space->GetCurrentRegion()->GetBegin();
        AllocateMultiRegion(space, machineCodeSpaceSize, machineCodeRegionIndex_);
    } else {
        machineCodeSpaceBeginAddr_ = object;
    }
}

}  // namespace panda::ecmascript

