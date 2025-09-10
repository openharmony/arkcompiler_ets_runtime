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

#include "common_interfaces/heap/heap_allocator.h"
#include "ecmascript/free_object.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "mem/mem.h"

namespace panda::ecmascript {

BaseDeserializer::BaseDeserializer(JSThread *thread, SerializeData *data, void *hint)
    : thread_(thread), data_(data), engine_(hint), heap_(const_cast<Heap *>(thread->GetEcmaVM()->GetHeap()))
{
    sheap_ = SharedHeap::GetInstance();
    uint32_t index = data_->GetDataIndex();
    if (index != 0) {
        sharedObjChunk_ = Runtime::GetInstance()->GetSerializeRootMapValue(thread_, index);
        if (sharedObjChunk_ == nullptr) {
            LOG_ECMA(FATAL) << "Unknown serializer root index: " << index;
            UNREACHABLE();
        }
    }
}

JSHandle<JSTaggedValue> BaseDeserializer::ReadValue()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK,
        ("Deserialize dataSize: " + std::to_string(data_->Size())).c_str(), "");
    if (data_->IsIncompleteData()) {
        LOG_ECMA(ERROR) << "The serialization data is incomplete";
        return JSHandle<JSTaggedValue>();
    }
    JSHandle<JSTaggedValue> res = DeserializeJSTaggedValue();
    return res;
}

JSHandle<JSTaggedValue> BaseDeserializer::DeserializeJSTaggedValue()
{
    // Use a heap address to store the result object, to unify the process, so that do not need to do special
    // check `IsRoot`.
    JSHandle<TaggedArray> resHolderHandle = heap_->GetEcmaVM()->GetFactory()->NewTaggedArray(1);
    AllocateToDifferentSpaces();

    // stop gc during deserialize
    heap_->SetOnSerializeEvent(true);

    uint8_t encodeFlag = data_->ReadUint8(position_);
    uintptr_t resHolderAddr = static_cast<uintptr_t>(resHolderHandle.GetTaggedType());
    while (ReadSingleEncodeData(encodeFlag, resHolderAddr, TaggedArray::DATA_OFFSET) == 0) {
        encodeFlag = data_->ReadUint8(position_);
    }

    // initialize concurrent func here
    for (auto func : concurrentFunctions_) {
        JSFunction::InitializeForConcurrentFunction(thread_, func);
    }
    concurrentFunctions_.clear();

    // new native binding object here
    for (auto &nativeBindingInfo : nativeBindingAttachInfos_) {
        DeserializeNativeBindingObject(&nativeBindingInfo);
    }
    nativeBindingAttachInfos_.clear();

    // new js error here
    for (auto &jsErrorInfo : jsErrorInfos_) {
        DeserializeJSError(&jsErrorInfo);
    }
    jsErrorInfos_.clear();

    // May be some other special object need to process at last
    DeserializeSpecialRecordedObjects();

    // recovery gc after serialize
    heap_->SetOnSerializeEvent(false);

    return JSHandle<JSTaggedValue>(thread_, resHolderHandle->Get(thread_, 0));
}

uintptr_t BaseDeserializer::DeserializeTaggedObject(SerializedObjectSpace space)
{
    size_t objSize = data_->ReadUint32(position_);
    uintptr_t res = RelocateObjectAddr(space, objSize);
    objectVector_.push_back(static_cast<JSTaggedType>(res));
    DeserializeObjectField(res, res + objSize);
    return res;
}

void BaseDeserializer::DeserializeObjectField(uintptr_t start, uintptr_t end)
{
    size_t offset = 0; // 0: initial offset
    while (start + offset < end) {
        uint8_t encodeFlag = data_->ReadUint8(position_);
        offset += ReadSingleEncodeData(encodeFlag, start, offset);
    }
}

void BaseDeserializer::DeserializeNativeBindingObject(NativeBindingAttachInfo *info)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    AttachFunc af = info->af_;
    void *bufferPointer = info->bufferPointer_;
    void *hint = info->hint_;
    void *attachData = info->attachData_;
    Local<JSValueRef> attachVal;
    {
        ThreadNativeScope nativeScope(thread_);
        attachVal = af(engine_, bufferPointer, hint, attachData);
    }
    if (attachVal.IsEmpty()) {
        LOG_ECMA(ERROR) << "NativeBindingObject is empty";
        attachVal = JSValueRef::Undefined(thread_->GetEcmaVM());
    }
    JSTaggedType res = JSNApiHelper::ToJSHandle(attachVal).GetTaggedType();
    ObjectSlot slot = info->GetSlot();
    slot.Update(res);
    if (!JSTaggedValue(res).IsInvalidValue()) {
        WriteBarrier(thread_, reinterpret_cast<void *>(info->GetObjAddr()), info->GetFieldOffset(), res);
    }
}

void BaseDeserializer::DeserializeJSError(JSErrorInfo *info)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    uint8_t type = info->errorType_;
    base::ErrorType errorType = base::ErrorType(type - static_cast<uint8_t>(JSType::JS_ERROR_FIRST));
    JSHandle<JSTaggedValue> errorMsg = info->errorMsg_;
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> errorTag = factory->NewJSError(errorType, JSHandle<EcmaString>(errorMsg), StackCheck::NO);
    ObjectSlot slot = info->GetSlot();
    slot.Update(errorTag.GetTaggedType());
    if (!errorTag.GetTaggedValue().IsInvalidValue()) {
        WriteBarrier(thread_, reinterpret_cast<void *>(info->GetObjAddr()), info->GetFieldOffset(),
                     errorTag.GetTaggedType());
    }
}

void BaseDeserializer::HandleNewObjectEncodeFlag(SerializedObjectSpace space,  uintptr_t objAddr, size_t fieldOffset)
{
    // deserialize object prologue
    bool isWeak = GetAndResetWeak();
    bool isTransferBuffer = GetAndResetTransferBuffer();
    bool isSharedArrayBuffer = GetAndResetSharedArrayBuffer();
    void *bufferPointer = GetAndResetBufferPointer();
    // save lazyArray, moduleFileName, moduleRecordName here, cause content will change in DeserializeTaggedObject.
    bool* lazyArray = GetLazyArray();
    CString moduleFileName = moduleFileName_;
    moduleFileName_.clear();
    CString moduleRecordName = moduleRecordName_;
    moduleRecordName_.clear();
    // deserialize object here
    uintptr_t addr = DeserializeTaggedObject(space);

    // deserialize object epilogue
    if (isTransferBuffer) {
        TransferArrayBufferAttach(addr);
    } else if (isSharedArrayBuffer) {
        IncreaseSharedArrayBufferReference(addr);
    } else if (bufferPointer != nullptr) {
        ResetNativePointerBuffer(addr, bufferPointer);
    }
    TaggedObject *object = reinterpret_cast<TaggedObject *>(addr);
    if (object->GetClass()->IsJSNativePointer()) {
        JSNativePointer *nativePointer = reinterpret_cast<JSNativePointer *>(object);
        if (nativePointer->GetDeleter() != nullptr) {
            if (!object->GetClass()->IsJSShared()) {
                thread_->GetEcmaVM()->PushToNativePointerList(nativePointer);
            }
        }
    } else if (object->GetClass()->IsJSFunction()) {
        JSFunction* func = reinterpret_cast<JSFunction *>(object);
        FunctionKind funcKind = func->GetFunctionKind(thread_);
        if (funcKind == FunctionKind::CONCURRENT_FUNCTION || object->GetClass()->IsJSSharedFunction()) {
            // defer initialize concurrent function
            JSHandle<JSFunction> funcHandle(thread_, func);
            concurrentFunctions_.push_back(funcHandle);
        }
        func->SetRawProfileTypeInfo<SKIP_BARRIER>(thread_, thread_->GlobalConstants()->GetEmptyProfileTypeInfoCell());
        func->SetWorkNodePointer(reinterpret_cast<uintptr_t>(nullptr));
    } else if (object->GetClass()->IsSourceTextModule()) {
        SourceTextModule* module = reinterpret_cast<SourceTextModule *>(object);
        module->SetEcmaModuleFilenameStringForDeserialize(moduleFileName);
        module->SetEcmaModuleRecordNameStringForDeserialize(moduleRecordName);
        module->SetLazyImportArrayForDeserialize(lazyArray);
        if (module->GetStatus() > ModuleStatus::INSTANTIATED) {
            module->SetStatus(ModuleStatus::INSTANTIATED);
        }
        module->SetException(thread_, thread_->GlobalConstants()->GetHole());
        module->SetCycleRoot(thread_, JSTaggedValue(module));
    }
    UpdateMaybeWeak(ObjectSlot(objAddr + fieldOffset), addr, isWeak);
    WriteBarrier<WriteBarrierType::DESERIALIZE>(thread_, reinterpret_cast<void *>(objAddr), fieldOffset,
                                                static_cast<JSTaggedType>(addr));
}

void BaseDeserializer::TransferArrayBufferAttach(uintptr_t objAddr)
{
    ASSERT(JSTaggedValue(static_cast<JSTaggedType>(objAddr)).IsArrayBuffer());
    JSArrayBuffer *arrayBuffer = reinterpret_cast<JSArrayBuffer *>(objAddr);
    size_t arrayLength = arrayBuffer->GetArrayBufferByteLength();
    bool withNativeAreaAllocator = arrayBuffer->GetWithNativeAreaAllocator();
    JSNativePointer *np =
        reinterpret_cast<JSNativePointer *>(arrayBuffer->GetArrayBufferData(thread_).GetTaggedObject());
    arrayBuffer->Attach(thread_, arrayLength, JSTaggedValue(np), withNativeAreaAllocator);
}

void BaseDeserializer::IncreaseSharedArrayBufferReference(uintptr_t objAddr)
{
    ASSERT(JSTaggedValue(static_cast<JSTaggedType>(objAddr)).IsSharedArrayBuffer());
    JSArrayBuffer *arrayBuffer = reinterpret_cast<JSArrayBuffer *>(objAddr);
    size_t arrayLength = arrayBuffer->GetArrayBufferByteLength();
    JSNativePointer *np =
        reinterpret_cast<JSNativePointer *>(arrayBuffer->GetArrayBufferData(thread_).GetTaggedObject());
    void *buffer = np->GetExternalPointer();
    if (JSSharedMemoryManager::GetInstance()->CreateOrLoad(&buffer, arrayLength)) {
        LOG_ECMA(FATAL) << "BaseDeserializer::IncreaseSharedArrayBufferReference failed";
    }
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
        np = reinterpret_cast<JSNativePointer *>(arrayBuffer->GetArrayBufferData(thread_).GetTaggedObject());
        nativeAreaAllocator->IncreaseNativeSizeStats(arrayBuffer->GetArrayBufferByteLength(), NativeFlag::ARRAY_BUFFER);
    } else {
        JSRegExp *jsRegExp = reinterpret_cast<JSRegExp *>(objAddr);
        np = reinterpret_cast<JSNativePointer *>(jsRegExp->GetByteCodeBuffer(thread_).GetTaggedObject());
        nativeAreaAllocator->IncreaseNativeSizeStats(jsRegExp->GetLength(), NativeFlag::REGEXP_BTYECODE);
    }

    np->SetExternalPointer(bufferPointer);
    np->SetDeleter(NativeAreaAllocator::FreeBufferFunc);
    np->SetData(thread_->GetEcmaVM()->GetNativeAreaAllocator());
}

size_t BaseDeserializer::ReadSingleEncodeData(uint8_t encodeFlag, uintptr_t objAddr, size_t fieldOffset)
{
    size_t handledFieldSize = sizeof(JSTaggedType);
    ObjectSlot slot(objAddr + fieldOffset);
    switch (encodeFlag) {
        case (uint8_t)SerializedObjectSpace::REGULAR_SPACE:
        case (uint8_t)SerializedObjectSpace::PIN_SPACE:
        case (uint8_t)SerializedObjectSpace::LARGE_SPACE:
        case (uint8_t)SerializedObjectSpace::OLD_SPACE:
        case (uint8_t)SerializedObjectSpace::NON_MOVABLE_SPACE:
        case (uint8_t)SerializedObjectSpace::MACHINE_CODE_SPACE:
        case (uint8_t)SerializedObjectSpace::HUGE_SPACE:
        case (uint8_t)SerializedObjectSpace::SHARED_OLD_SPACE:
        case (uint8_t)SerializedObjectSpace::SHARED_NON_MOVABLE_SPACE:
        case (uint8_t)SerializedObjectSpace::SHARED_HUGE_SPACE: {
            SerializedObjectSpace space = SerializeData::DecodeSpace(encodeFlag);
            HandleNewObjectEncodeFlag(space, objAddr, fieldOffset);
            break;
        }
        case (uint8_t)EncodeFlag::REFERENCE: {
            uint32_t valueIndex = data_->ReadUint32(position_);
            JSTaggedType valueAddr = objectVector_.at(valueIndex);
            UpdateMaybeWeak(slot, valueAddr, GetAndResetWeak());
            WriteBarrier<WriteBarrierType::DESERIALIZE>(thread_, reinterpret_cast<void *>(objAddr), fieldOffset,
                                                        valueAddr);
            break;
        }
        case (uint8_t)EncodeFlag::WEAK: {
            ASSERT(!isWeak_);
            isWeak_ = true;
            handledFieldSize = 0;
            break;
        }
        case (uint8_t)EncodeFlag::PRIMITIVE: {
            JSTaggedType value = data_->ReadJSTaggedType(position_);
            slot.Update(value);
            break;
        }
        case (uint8_t)EncodeFlag::MULTI_RAW_DATA: {
            uint32_t size = data_->ReadUint32(position_);
            data_->ReadRawData(objAddr + fieldOffset, size, position_);
            handledFieldSize = size;
            break;
        }
        case (uint8_t)EncodeFlag::ROOT_OBJECT: {
            uint32_t index = data_->ReadUint32(position_);
            uintptr_t valueAddr = thread_->GetEcmaVM()->GetSnapshotEnv()->RelocateRootObjectAddr(index);
            if (valueAddr > JSTaggedValue::INVALID_VALUE_LIMIT) {
                WriteBarrier(thread_, reinterpret_cast<void *>(objAddr), fieldOffset,
                             static_cast<JSTaggedType>(valueAddr));
            }
            UpdateMaybeWeak(slot, valueAddr, GetAndResetWeak());
            break;
        }
        case (uint8_t)EncodeFlag::OBJECT_PROTO: {
            uint8_t type = data_->ReadUint8(position_);
            uintptr_t protoAddr = RelocateObjectProtoAddr(type);
            if (protoAddr > JSTaggedValue::INVALID_VALUE_LIMIT) {
                WriteBarrier(thread_, reinterpret_cast<void *>(objAddr), fieldOffset,
                             static_cast<JSTaggedType>(protoAddr));
            }
            UpdateMaybeWeak(slot, protoAddr, GetAndResetWeak());
            break;
        }
        case (uint8_t)EncodeFlag::TRANSFER_ARRAY_BUFFER: {
            isTransferArrayBuffer_ = true;
            handledFieldSize = 0;
            break;
        }
        case (uint8_t)EncodeFlag::SHARED_ARRAY_BUFFER: {
            isSharedArrayBuffer_ = true;
            handledFieldSize = 0;
            break;
        }
        case (uint8_t)EncodeFlag::ARRAY_BUFFER:
        case (uint8_t)EncodeFlag::SENDABLE_ARRAY_BUFFER:
        case (uint8_t)EncodeFlag::JS_REG_EXP: {
            size_t bufferLength = data_->ReadUint32(position_);
            auto nativeAreaAllocator = thread_->GetEcmaVM()->GetNativeAreaAllocator();
            bufferPointer_ = nativeAreaAllocator->AllocateBuffer(bufferLength);
            heap_->IncNativeSizeAfterLastGC(bufferLength);
            data_->ReadRawData(ToUintPtr(bufferPointer_), bufferLength, position_);
            heap_->IncreaseNativeBindingSize(bufferLength);
            handledFieldSize = 0;
            break;
        }
        case (uint8_t)EncodeFlag::NATIVE_BINDING_OBJECT: {
            slot.Update(JSTaggedValue::Undefined().GetRawData());
            AttachFunc af = reinterpret_cast<AttachFunc>(data_->ReadJSTaggedType(position_));
            void *bufferPointer = reinterpret_cast<void *>(data_->ReadJSTaggedType(position_));
            void *hint = reinterpret_cast<void *>(data_->ReadJSTaggedType(position_));
            void *attachData = reinterpret_cast<void *>(data_->ReadJSTaggedType(position_));
            JSHandle<JSTaggedValue> obj(thread_, JSTaggedValue(static_cast<JSTaggedType>(objAddr)));
            // defer new native binding object until deserialize finish
            nativeBindingAttachInfos_.emplace_back(af, bufferPointer, hint, attachData, obj, fieldOffset);
            break;
        }
        case (uint8_t)EncodeFlag::JS_ERROR: {
            slot.Update(JSTaggedValue::Undefined().GetRawData());
            uint8_t type = data_->ReadUint8(position_);
            ASSERT(type >= static_cast<uint8_t>(JSType::JS_ERROR_FIRST)
                && type <= static_cast<uint8_t>(JSType::JS_ERROR_LAST));
            JSHandle<JSTaggedValue> obj(thread_, JSTaggedValue(static_cast<JSTaggedType>(objAddr)));
            jsErrorInfos_.emplace_back(type, JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined()), obj,
                                       fieldOffset);
            uint8_t flag = data_->ReadUint8(position_);
            if (flag == 1) { // error msg is string
                isErrorMsg_ = true;
                handledFieldSize = 0;
            }
            break;
        }
        case (uint8_t)EncodeFlag::SHARED_OBJECT: {
            uint32_t index = data_->ReadUint32(position_);
            if (UNLIKELY(index >= sharedObjChunk_->Size())) {
                LOG_ECMA(FATAL) << "Shared object index invalid, index: " << index << " chunkSize: "
                    << sharedObjChunk_->Size();
                UNREACHABLE();
            }
            JSTaggedType value = sharedObjChunk_->Get(index);
            objectVector_.push_back(value);
            bool isErrorMsg = GetAndResetIsErrorMsg();
            if (isErrorMsg) {
                // defer new js error
                jsErrorInfos_.back().errorMsg_ = JSHandle<JSTaggedValue>(thread_, JSTaggedValue(value));
                break;
            }
            WriteBarrier(thread_, reinterpret_cast<void *>(objAddr), fieldOffset, value);
            UpdateMaybeWeak(slot, value, GetAndResetWeak());
            break;
        }
        case (uint8_t)EncodeFlag::GLOBAL_ENV: {
            slot.Update(thread_->GetGlobalEnv().GetTaggedValue().GetTaggedObject());
            break;
        }
        case (uint8_t)EncodeFlag::MODULE_FILE_NAME: {
            uint32_t len = data_->ReadUint32(position_);
            if (len > 0) {
                moduleFileName_.resize(len);
                data_->ReadRawData(ToUintPtr(moduleFileName_.data()), len, position_);
            } else {
                LOG_ECMA(FATAL) << "ReadSingleEncodeData MODULE_FILE_NAME is empty.";
            }
            handledFieldSize = 0;
            break;
        }
        case (uint8_t)EncodeFlag::MODULE_RECORD_NAME: {
            uint32_t len = data_->ReadUint32(position_);
            if (len > 0) {
                moduleRecordName_.resize(len);
                data_->ReadRawData(ToUintPtr(moduleRecordName_.data()), len, position_);
            } else {
                moduleRecordName_ = "";
            }
            handledFieldSize = 0;
            break;
        }
        case (uint8_t)EncodeFlag::MODULE_LAZY_ARRAY: {
            uint32_t len = data_->ReadUint32(position_);
            if (len > 0) {
                // moduleLazyArray_ will delete in release module process.
                moduleLazyArray_ = new bool[len/sizeof(bool)];
                data_->ReadRawData(ToUintPtr(moduleLazyArray_), len, position_);
            } else {
                moduleLazyArray_ = nullptr;
            }
            handledFieldSize = 0;
            break;
        }
        default:
            // This flag may be supported by subclass.
            return DerivedExtraReadSingleEncodeData(encodeFlag, objAddr, fieldOffset);
    }
    return handledFieldSize;
}

size_t BaseDeserializer::DerivedExtraReadSingleEncodeData(uint8_t encodeFlag, uintptr_t objAddr, size_t fieldOffset)
{
    LOG_ECMA(FATAL) << "this branch is unreachable " << static_cast<int>(encodeFlag);
    UNREACHABLE();
}

uintptr_t BaseDeserializer::RelocateObjectAddr(SerializedObjectSpace space, size_t objSize)
{
    uintptr_t res = 0U;
    switch (space) {
        case SerializedObjectSpace::REGULAR_SPACE: {
            if (currentRegularObjectAddr_ + objSize >
                    currentRegularRegionBeginAddr_ + common::Heap::GetNormalRegionAvailableSize()) {
                ASSERT(regularRegionIndex_ < regionVector_.size());
                currentRegularObjectAddr_ = regionVector_[regularRegionIndex_++];
                currentRegularRegionBeginAddr_ = currentRegularObjectAddr_;
            }
            res = currentRegularObjectAddr_;
            currentRegularObjectAddr_ += objSize;
            break;
        }
        case SerializedObjectSpace::PIN_SPACE: {
            if (currentPinObjectAddr_ + objSize >
                    currentPinRegionBeginAddr_ + common::Heap::GetNormalRegionAvailableSize()) {
                ASSERT(pinRegionIndex_ < regionVector_.size());
                currentPinObjectAddr_ = regionVector_[pinRegionIndex_++];
                currentPinRegionBeginAddr_ = currentPinObjectAddr_;
            }
            res = currentPinObjectAddr_;
            currentPinObjectAddr_ += objSize;
            break;
        }
        case SerializedObjectSpace::LARGE_SPACE: {
            // no gc for this allocate
            res = common::HeapAllocator::AllocateLargeRegion(objSize);
            if (res == 0) {
                DeserializeFatalOutOfMemory(objSize, false, false);
            }
            break;
        }
        case SerializedObjectSpace::OLD_SPACE: {
            if (oldSpaceBeginAddr_ + objSize > AlignUp(oldSpaceBeginAddr_, DEFAULT_REGION_SIZE)) {
                ASSERT(oldRegionIndex_ < regionVector_.size());
                oldSpaceBeginAddr_ = regionVector_[oldRegionIndex_++];
            }
            res = oldSpaceBeginAddr_;
            oldSpaceBeginAddr_ += objSize;
            break;
        }
        case SerializedObjectSpace::NON_MOVABLE_SPACE: {
            if (nonMovableSpaceBeginAddr_ + objSize > AlignUp(nonMovableSpaceBeginAddr_, DEFAULT_REGION_SIZE)) {
                ASSERT(nonMovableRegionIndex_ < regionVector_.size());
                nonMovableSpaceBeginAddr_ = regionVector_[nonMovableRegionIndex_++];
            }
            res = nonMovableSpaceBeginAddr_;
            nonMovableSpaceBeginAddr_ += objSize;
            break;
        }
        case SerializedObjectSpace::MACHINE_CODE_SPACE: {
            if (machineCodeSpaceBeginAddr_ + objSize > AlignUp(machineCodeSpaceBeginAddr_, DEFAULT_REGION_SIZE)) {
                ASSERT(machineCodeRegionIndex_ < regionVector_.size());
                machineCodeSpaceBeginAddr_ = regionVector_[machineCodeRegionIndex_++];
            }
            res = machineCodeSpaceBeginAddr_;
            machineCodeSpaceBeginAddr_ += objSize;
            break;
        }
        case SerializedObjectSpace::HUGE_SPACE: {
            // no gc for this allocate
            res = heap_->GetHugeObjectSpace()->Allocate(objSize, thread_, AllocateEventType::DESERIALIZE);
            if (res == 0) {
                DeserializeFatalOutOfMemory(objSize, false, false);
            }
            break;
        }
        case SerializedObjectSpace::SHARED_OLD_SPACE: {
            if (sOldSpaceBeginAddr_ + objSize > AlignUp(sOldSpaceBeginAddr_, DEFAULT_REGION_SIZE)) {
                ASSERT(sOldRegionIndex_ < regionVector_.size());
                sOldSpaceBeginAddr_ = regionVector_[sOldRegionIndex_++];
            }
            res = sOldSpaceBeginAddr_;
            sOldSpaceBeginAddr_ += objSize;
            break;
        }
        case SerializedObjectSpace::SHARED_NON_MOVABLE_SPACE: {
            if (sNonMovableSpaceBeginAddr_ + objSize > AlignUp(sNonMovableSpaceBeginAddr_, DEFAULT_REGION_SIZE)) {
                ASSERT(sNonMovableRegionIndex_ < regionVector_.size());
                sNonMovableSpaceBeginAddr_ = regionVector_[sNonMovableRegionIndex_++];
            }
            res = sNonMovableSpaceBeginAddr_;
            sNonMovableSpaceBeginAddr_ += objSize;
            break;
        }
        case SerializedObjectSpace::SHARED_HUGE_SPACE: {
            // no gc for this allocate
            res = sheap_->GetHugeObjectSpace()->Allocate(thread_, objSize, AllocateEventType::DESERIALIZE);
            if (res == 0) {
                DeserializeFatalOutOfMemory(objSize, false, true);
            }
            break;
        }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
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
            return JSHandle<JSFunction>(env->GetErrorFunction())->GetFunctionPrototype(thread_).GetRawData();
        case (uint8_t)JSType::JS_EVAL_ERROR:
            return JSHandle<JSFunction>(env->GetEvalErrorFunction())->GetFunctionPrototype(thread_).GetRawData();
        case (uint8_t)JSType::JS_RANGE_ERROR:
            return JSHandle<JSFunction>(env->GetRangeErrorFunction())->GetFunctionPrototype(thread_).GetRawData();
        case (uint8_t)JSType::JS_REFERENCE_ERROR:
            return JSHandle<JSFunction>(env->GetReferenceErrorFunction())->GetFunctionPrototype(thread_).GetRawData();
        case (uint8_t)JSType::JS_TYPE_ERROR:
            return JSHandle<JSFunction>(env->GetTypeErrorFunction())->GetFunctionPrototype(thread_).GetRawData();
        case (uint8_t)JSType::JS_AGGREGATE_ERROR:
            return JSHandle<JSFunction>(env->GetAggregateErrorFunction())->GetFunctionPrototype(thread_).GetRawData();
        case (uint8_t)JSType::JS_URI_ERROR:
            return JSHandle<JSFunction>(env->GetURIErrorFunction())->GetFunctionPrototype(thread_).GetRawData();
        case (uint8_t)JSType::JS_SYNTAX_ERROR:
            return JSHandle<JSFunction>(env->GetSyntaxErrorFunction())->GetFunctionPrototype(thread_).GetRawData();
        case (uint8_t)JSType::JS_OOM_ERROR:
            return JSHandle<JSFunction>(env->GetOOMErrorFunction())->GetFunctionPrototype(thread_).GetRawData();
        case (uint8_t)JSType::JS_TERMINATION_ERROR:
            return JSHandle<JSFunction>(env->GetTerminationErrorFunction())->GetFunctionPrototype(thread_).GetRawData();
        case (uint8_t)JSType::JS_DATE:
            return env->GetDatePrototype().GetTaggedType();
        case (uint8_t)JSType::JS_ARRAY:
            return env->GetArrayPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_SHARED_ARRAY:
            return env->GetSharedArrayPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_API_BITVECTOR:
            return env->GetBitVectorPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_MAP:
            return env->GetMapPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_SHARED_MAP:
            return env->GetSharedMapPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_SET:
            return env->GetSetPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_SHARED_SET:
            return env->GetSharedSetPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_SENDABLE_ARRAY_BUFFER:
            return env->GetSendableArrayBufferPrototype().GetTaggedType();
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
        case (uint8_t)JSType::JS_SHARED_INT8_ARRAY:
            return env->GetSharedInt8ArrayFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_SHARED_UINT8_ARRAY:
            return env->GetSharedUint8ArrayFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_SHARED_UINT8_CLAMPED_ARRAY:
            return env->GetSharedUint8ClampedArrayFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_SHARED_INT16_ARRAY:
            return env->GetSharedInt16ArrayFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_SHARED_UINT16_ARRAY:
            return env->GetSharedUint16ArrayFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_SHARED_INT32_ARRAY:
            return env->GetSharedInt32ArrayFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_SHARED_UINT32_ARRAY:
            return env->GetSharedUint32ArrayFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_SHARED_FLOAT32_ARRAY:
            return env->GetSharedFloat32ArrayFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_SHARED_FLOAT64_ARRAY:
            return env->GetSharedFloat64ArrayFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_SHARED_BIGINT64_ARRAY:
            return env->GetSharedBigInt64ArrayFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_SHARED_BIGUINT64_ARRAY:
            return env->GetSharedBigUint64ArrayFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_ARRAY_BUFFER:
            return JSHandle<JSFunction>(env->GetArrayBufferFunction())->GetFunctionPrototype(thread_).GetRawData();
        case (uint8_t)JSType::JS_SHARED_ARRAY_BUFFER:
            return JSHandle<JSFunction>(env->GetSharedArrayBufferFunction())
                ->GetFunctionPrototype(thread_)
                .GetRawData();
        case (uint8_t)JSType::JS_ASYNC_FUNCTION:
            return env->GetAsyncFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::JS_SHARED_ASYNC_FUNCTION:
            return env->GetSAsyncFunctionPrototype().GetTaggedType();
        case (uint8_t)JSType::BIGINT:
            return JSHandle<JSFunction>(env->GetBigIntFunction())->GetFunctionPrototype(thread_).GetRawData();
        default:
            LOG_ECMA(FATAL) << "Relocate unsupported JSType: " << JSHClass::DumpJSType(static_cast<JSType>(objectType));
            UNREACHABLE();
            break;
    }
}

void BaseDeserializer::AllocateToDifferentSpaces()
{
    if (g_isEnableCMCGC) {
        AllocateToDifferentCMCSpaces();
    } else {
        if (!AllocateToDifferentLocalSpaces(true)) {
            // If allocate local obj fail, trigger old gc and retry again
            heap_->CollectGarbage(TriggerGCType::OLD_GC, GCReason::ALLOCATION_FAILED);
            AllocateToDifferentLocalSpaces(false);
        }
        AllocateToDifferentSharedSpaces();
    }
}

bool BaseDeserializer::AllocateToDifferentLocalSpaces(bool isFirstAllocate)
{
    size_t oldSpaceSize = data_->GetOldSpaceSize();
    if (oldSpaceSize > 0) {
        // Old space should increase space live object size here since the allocate is independent
        heap_->GetOldSpace()->IncreaseLiveObjectSize(oldSpaceSize);
        if (!AllocateToOldSpace(oldSpaceSize, isFirstAllocate)) {
            heap_->GetOldSpace()->DecreaseLiveObjectSize(oldSpaceSize);
            return false;
        }
    }
    size_t nonMovableSpaceSize = data_->GetNonMovableSpaceSize();
    if (nonMovableSpaceSize > 0) {
        if (!AllocateToNonMovableSpace(nonMovableSpaceSize, isFirstAllocate)) {
            return false;
        }
    }
    size_t machineCodeSpaceSize = data_->GetMachineCodeSpaceSize();
    if (machineCodeSpaceSize > 0) {
        if (!AllocateToMachineCodeSpace(machineCodeSpaceSize, isFirstAllocate)) {
            return false;
        }
    }
    return true;
}

void BaseDeserializer::AllocateToDifferentSharedSpaces()
{
    size_t sOldSpaceSize = data_->GetSharedOldSpaceSize();
    if (sOldSpaceSize > 0) {
        sheap_->GetOldSpace()->IncreaseLiveObjectSize(sOldSpaceSize);
        AllocateToSharedOldSpace(sOldSpaceSize);
    }
    size_t sNonMovableSpaceSize = data_->GetSharedNonMovableSpaceSize();
    if (sNonMovableSpaceSize > 0) {
        sheap_->GetNonMovableSpace()->IncreaseLiveObjectSize(sNonMovableSpaceSize);
        AllocateToSharedNonMovableSpace(sNonMovableSpaceSize);
    }
}

void BaseDeserializer::AllocateToDifferentCMCSpaces()
{
    size_t regularSpaceSize = data_->GetRegularSpaceSize();
    if (regularSpaceSize > 0) {
        // statistic object size
        AllocateToRegularSpace(regularSpaceSize);
    }
    size_t pinSpaceSize = data_->GetPinSpaceSize();
    if (pinSpaceSize > 0) {
        // statistic object size
        AllocateToPinSpace(pinSpaceSize);
    }
}

void BaseDeserializer::AllocateToRegularSpace(size_t regularSpaceSize)
{
    if (regularSpaceSize <= common::Heap::GetNormalRegionAvailableSize()) {
        currentRegularObjectAddr_ = common::HeapAllocator::AllocateNoGC(regularSpaceSize);
    } else {
        currentRegularObjectAddr_ = AllocateMultiCMCRegion(regularSpaceSize, regularRegionIndex_,
                                                           RegionType::RegularRegion);
    }
    currentRegularRegionBeginAddr_ = currentRegularObjectAddr_;
    if (currentRegularObjectAddr_ == 0U) {
        LOG_ECMA(FATAL) << "Deserialize oom error";
    }
}
void BaseDeserializer::AllocateToPinSpace(size_t pinSpaceSize)
{
    if (pinSpaceSize <= common::Heap::GetNormalRegionAvailableSize()) {
        currentPinObjectAddr_ = common::HeapAllocator::AllocateNonmoveNoGC(pinSpaceSize);
    } else {
        currentPinObjectAddr_ = AllocateMultiCMCRegion(pinSpaceSize, pinRegionIndex_, RegionType::PinRegion);
    }
    currentPinRegionBeginAddr_ = currentPinObjectAddr_;
    if (currentPinObjectAddr_ == 0U) {
        LOG_ECMA(FATAL) << "Deserialize oom error";
    }
}

uintptr_t BaseDeserializer::AllocateMultiCMCRegion(size_t spaceObjSize, size_t &regionIndex, RegionType regionType)
{
    constexpr size_t REGION_SIZE = common::Heap::GetNormalRegionAvailableSize();
    ASSERT(REGION_SIZE != 0);
    ASSERT(spaceObjSize > REGION_SIZE);
    regionIndex = regionVector_.size();
    size_t regionAlignedSize = SerializeData::AlignUpRegionAvailableSize(spaceObjSize);
    ASSERT(regionAlignedSize % REGION_SIZE == 0);
    size_t regionNum = regionAlignedSize / REGION_SIZE;
    uintptr_t firstRegionAddr = 0U;
    std::vector<size_t> regionRemainSizeVector;
    size_t regionRemainSizeIndex = 0;
    if (regionType == RegionType::RegularRegion) {
        regionRemainSizeVector = data_->GetRegularRemainSizeVector();
    } else {
        regionRemainSizeVector = data_->GetPinRemainSizeVector();
    }
    while (regionNum > 0) {
        uintptr_t regionAddr = 0U;
        if (regionType == RegionType::RegularRegion) {
            regionAddr = common::HeapAllocator::AllocateOldRegion();
        } else {
            regionAddr = common::HeapAllocator::AllocateNonMovableRegion();
        }
        if (regionAddr == 0U) {
            LOG_ECMA(FATAL) << "Deserialize allocate multi cmc region fail";
        }
        if (firstRegionAddr == 0U) {
            firstRegionAddr = regionAddr;
        } else {
            regionVector_.push_back(regionAddr);
        }
        // fill region not used size
        if (regionNum == 1) {  // last region
            size_t lastRegionRemainSize = regionAlignedSize - spaceObjSize;
            FreeObject::FillFreeObject(heap_, regionAddr + REGION_SIZE - lastRegionRemainSize,
                                       lastRegionRemainSize);
        } else {
            auto regionAliveObjSize = REGION_SIZE - regionRemainSizeVector[regionRemainSizeIndex++];
            FreeObject::FillFreeObject(heap_, regionAddr + regionAliveObjSize, REGION_SIZE - regionAliveObjSize);
        }
        regionNum--;
    }
    return firstRegionAddr;
}

bool BaseDeserializer::AllocateMultiRegion(SparseSpace *space, size_t spaceObjSize, size_t &regionIndex,
                                           SerializedObjectSpace spaceType, bool isFirstAllocate)
{
    ASSERT(spaceType != SerializedObjectSpace::NON_MOVABLE_SPACE);
    regionIndex = regionVector_.size();
    size_t regionAlignedSize = SerializeData::AlignUpRegionAvailableSize(spaceObjSize);
    size_t regionNum = regionAlignedSize / Region::GetRegionAvailableSize();
    size_t index = 0;
    while (regionNum > 1) { // 1: one region have allocated before
        auto regionRemainSizeVector = data_->GetRegionRemainSizeVectors().at(static_cast<uint8_t>(spaceType));
        auto regionAliveObjSize = Region::GetRegionAvailableSize() - regionRemainSizeVector[index++];
        space->GetCurrentRegion()->IncreaseAliveObject(regionAliveObjSize);
        space->ResetTopPointer(space->GetCurrentRegion()->GetBegin() + regionAliveObjSize);
        if (!space->Expand()) {
            if (!isFirstAllocate) {
                DeserializeFatalOutOfMemory(spaceObjSize);
            }
            regionVector_.clear();
            return false;
        }
        Region *currentRegion = space->GetCurrentRegion();
        FreeObject::FillFreeObject(heap_, currentRegion->GetBegin(), currentRegion->GetSize());
        regionVector_.push_back(currentRegion->GetBegin());
        regionNum--;
    }
    size_t lastRegionRemainSize = regionAlignedSize - spaceObjSize;
    space->GetCurrentRegion()->IncreaseAliveObject(Region::GetRegionAvailableSize() - lastRegionRemainSize);
    space->ResetTopPointer(space->GetCurrentRegion()->GetEnd() - lastRegionRemainSize);
    return true;
}

bool BaseDeserializer::AllocateMultiNonmovableRegion(SparseSpace *space, size_t spaceObjSize, size_t &regionIndex,
                                                     [[maybe_unused]] SerializedObjectSpace spaceType,
                                                     bool isFirstAllocate)
{
    ASSERT(spaceType == SerializedObjectSpace::NON_MOVABLE_SPACE);
    regionIndex = regionVector_.size();
    size_t allocatedSize = 0;
    const std::vector<size_t> &remainSize = data_->GetRegionRemainSizeVectors().at(static_cast<int>(spaceType));
    size_t numRegion = 0;
    size_t totalRegion = remainSize.size();
    while (allocatedSize < spaceObjSize) {
        size_t leftSize = spaceObjSize - allocatedSize;
        size_t size = std::min(leftSize, Region::GetRegionAvailableSize());
        uintptr_t obj = space->Allocate(size, false);
        if (obj == 0) {
            if (!isFirstAllocate) {
                DeserializeFatalOutOfMemory(spaceObjSize);
            }
            regionVector_.clear();
            return false;
        }
        if (allocatedSize == 0) {
            // The first region
            nonMovableSpaceBeginAddr_ = obj;
        } else {
            regionVector_.push_back(obj);
        }
        allocatedSize += size;
        if (numRegion < totalRegion) {
            size_t wastedSize = remainSize[numRegion++];
            // Fill actual used size
            FreeObject::FillFreeObject(heap_, obj, size - wastedSize);
            // Fill wasted size
            FreeObject::FillFreeObject(heap_, obj + size - wastedSize, wastedSize);
        } else {
            // Fill last allocate size
            FreeObject::FillFreeObject(heap_, obj, size);
        }
    }
    return true;
}

void BaseDeserializer::AllocateMultiSharedRegion(SharedSparseSpace *space, size_t spaceObjSize, size_t &regionIndex,
                                                 SerializedObjectSpace spaceType)
{
    regionIndex = regionVector_.size();
    size_t regionAlignedSize = SerializeData::AlignUpRegionAvailableSize(spaceObjSize);
    size_t regionNum = regionAlignedSize / Region::GetRegionAvailableSize();
    auto regionRemainSizeVector = data_->GetRegionRemainSizeVectors().at(static_cast<uint8_t>(spaceType));
    std::vector<Region *> allocateRegions;
    size_t index = 0;
    while (regionNum > 0) {
        if (space->CommittedSizeExceed()) {
            DeserializeFatalOutOfMemory(spaceObjSize, true, true);
        }
        Region *region = space->AllocateDeserializeRegion(thread_);
        FreeObject::FillFreeObject(sheap_, region->GetBegin(), region->GetSize());
        if (regionNum == 1) { // 1: Last allocate region
            size_t lastRegionRemainSize = regionAlignedSize - spaceObjSize;
            region->SetHighWaterMark(region->GetEnd() - lastRegionRemainSize);
        } else {
            region->SetHighWaterMark(region->GetEnd() - regionRemainSizeVector[index++]);
        }
        region->IncreaseAliveObject(region->GetAllocatedBytes());
        regionVector_.push_back(region->GetBegin());
        allocateRegions.push_back(region);
        regionNum--;
    }
    space->MergeDeserializeAllocateRegions(allocateRegions);
}

bool BaseDeserializer::AllocateToOldSpace(size_t oldSpaceSize, bool isFirstAllocate)
{
    OldSpace *space = heap_->GetOldSpace();
    uintptr_t object = space->AllocateSlow(oldSpaceSize, true);
    if (UNLIKELY(object == 0U)) {
        if (space->CommittedSizeExceed()) {
            if (!isFirstAllocate) {
                DeserializeFatalOutOfMemory(oldSpaceSize);
            }
            return false;
        }
        oldSpaceBeginAddr_ = space->GetCurrentRegion()->GetBegin();
        FreeObject::FillFreeObject(heap_, oldSpaceBeginAddr_, space->GetCurrentRegion()->GetSize());
        return AllocateMultiRegion(space, oldSpaceSize, oldRegionIndex_,
                                   SerializedObjectSpace::OLD_SPACE, isFirstAllocate);
    }
    FreeObject::FillFreeObject(heap_, object, oldSpaceSize);
    oldSpaceBeginAddr_ = object;
    return true;
}

bool BaseDeserializer::AllocateToNonMovableSpace(size_t nonMovableSpaceSize, bool isFirstAllocate)
{
    SparseSpace *space = heap_->GetNonMovableSpace();
    uintptr_t object = space->Allocate(nonMovableSpaceSize, false);
    if (UNLIKELY(object == 0U)) {
        return AllocateMultiNonmovableRegion(space, nonMovableSpaceSize, nonMovableRegionIndex_,
                                             SerializedObjectSpace::NON_MOVABLE_SPACE, isFirstAllocate);
    }
    FreeObject::FillFreeObject(heap_, object, nonMovableSpaceSize);
    nonMovableSpaceBeginAddr_ = object;
    return true;
}

bool BaseDeserializer::AllocateToMachineCodeSpace(size_t machineCodeSpaceSize, bool isFirstAllocate)
{
    SparseSpace *space = heap_->GetMachineCodeSpace();
    uintptr_t object = space->Allocate(machineCodeSpaceSize, false);
    if (UNLIKELY(object == 0U)) {
        if (space->CommittedSizeExceed()) {
            if (!isFirstAllocate) {
                DeserializeFatalOutOfMemory(machineCodeSpaceSize);
            }
            return false;
        }
        machineCodeSpaceBeginAddr_ = space->GetCurrentRegion()->GetBegin();
        FreeObject::FillFreeObject(heap_, machineCodeSpaceBeginAddr_, space->GetCurrentRegion()->GetSize());
        return AllocateMultiRegion(space, machineCodeSpaceSize, machineCodeRegionIndex_,
                                   SerializedObjectSpace::MACHINE_CODE_SPACE, isFirstAllocate);
    }
    FreeObject::FillFreeObject(heap_, object, machineCodeSpaceSize);
    machineCodeSpaceBeginAddr_ = object;
    return true;
}

void BaseDeserializer::AllocateToSharedOldSpace(size_t sOldSpaceSize)
{
    SharedSparseSpace *space = sheap_->GetOldSpace();
    uintptr_t object = space->AllocateNoGCAndExpand(thread_, sOldSpaceSize);
    if (UNLIKELY(object == 0U)) {
        AllocateMultiSharedRegion(space, sOldSpaceSize, sOldRegionIndex_, SerializedObjectSpace::SHARED_OLD_SPACE);
        sOldSpaceBeginAddr_ = regionVector_[sOldRegionIndex_++];
    } else {
        if (thread_->IsSharedConcurrentMarkingOrFinished()) {
            Region *region = Region::ObjectAddressToRange(object);
            region->IncreaseAliveObject(sOldSpaceSize);
        }
        FreeObject::FillFreeObject(sheap_, object, sOldSpaceSize);
        sOldSpaceBeginAddr_ = object;
    }
}

void BaseDeserializer::AllocateToSharedNonMovableSpace(size_t sNonMovableSpaceSize)
{
    SharedNonMovableSpace *space = sheap_->GetNonMovableSpace();
    uintptr_t object = space->AllocateNoGCAndExpand(thread_, sNonMovableSpaceSize);
    if (UNLIKELY(object == 0U)) {
        AllocateMultiSharedRegion(space, sNonMovableSpaceSize, sNonMovableRegionIndex_,
                                  SerializedObjectSpace::SHARED_NON_MOVABLE_SPACE);
        sNonMovableSpaceBeginAddr_ = regionVector_[sNonMovableRegionIndex_++];
    } else {
        if (thread_->IsSharedConcurrentMarkingOrFinished()) {
            Region *region = Region::ObjectAddressToRange(object);
            region->IncreaseAliveObject(sNonMovableSpaceSize);
        }
        FreeObject::FillFreeObject(sheap_, object, sNonMovableSpaceSize);
        sNonMovableSpaceBeginAddr_ = object;
    }
}

}  // namespace panda::ecmascript

