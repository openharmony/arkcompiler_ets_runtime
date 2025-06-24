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

#ifndef ECMASCRIPT_SERIALIZER_BASE_DESERIALIZER_H
#define ECMASCRIPT_SERIALIZER_BASE_DESERIALIZER_H

#include "ecmascript/serializer/serialize_data.h"

namespace panda::ecmascript {
class Heap;
class JSThread;
struct NativeBindingAttachInfo {
    AttachFunc af_ {nullptr};
    void *bufferPointer_ {nullptr};
    void *hint_ = {nullptr};
    void *attachData_ = {nullptr};
    JSHandle<JSTaggedValue> obj_;
    size_t offset_ {0U};

    NativeBindingAttachInfo(AttachFunc af, void *bufferPointer, void *hint, void *attachData,
                            JSHandle<JSTaggedValue> obj, size_t offset) : af_(af), bufferPointer_(bufferPointer),
        hint_(hint), attachData_(attachData), obj_(obj), offset_(offset) {}

    uintptr_t GetObjAddr() const
    {
        return static_cast<uintptr_t>(obj_.GetTaggedType());
    }

    size_t GetFieldOffset() const
    {
        return offset_;
    }

    ObjectSlot GetSlot() const
    {
        return ObjectSlot(GetObjAddr() + offset_);
    }
};

struct JSErrorInfo {
    uint8_t errorType_ {0};
    JSHandle<JSTaggedValue> errorMsg_;
    JSHandle<JSTaggedValue> obj_;
    size_t offset_ {0U};

    JSErrorInfo(uint8_t errorType, JSHandle<JSTaggedValue> errorMsg, JSHandle<JSTaggedValue> obj, size_t offset)
        : errorType_(errorType), errorMsg_(errorMsg), obj_(obj), offset_(offset) {}

    uintptr_t GetObjAddr() const
    {
        return static_cast<uintptr_t>(obj_.GetTaggedType());
    }

    size_t GetFieldOffset() const
    {
        return offset_;
    }

    ObjectSlot GetSlot() const
    {
        return ObjectSlot(GetObjAddr() + offset_);
    }
};

class BaseDeserializer {
public:
    explicit BaseDeserializer(JSThread *thread, SerializeData *data, void *hint = nullptr);

    virtual ~BaseDeserializer()
    {
        objectVector_.clear();
        regionVector_.clear();
    }

    NO_COPY_SEMANTIC(BaseDeserializer);
    NO_MOVE_SEMANTIC(BaseDeserializer);

    JSHandle<JSTaggedValue> ReadValue();

protected:
    virtual uintptr_t DeserializeTaggedObject(SerializedObjectSpace space);

private:
    JSHandle<JSTaggedValue> DeserializeJSTaggedValue();
    void DeserializeNativeBindingObject(NativeBindingAttachInfo *info);
    void DeserializeJSError(JSErrorInfo *info);
    uintptr_t RelocateObjectAddr(SerializedObjectSpace space, size_t objSize);
    JSTaggedType RelocateObjectProtoAddr(uint8_t objectType);
    void DeserializeObjectField(uintptr_t start, uintptr_t end);
    size_t ReadSingleEncodeData(uint8_t encodeFlag, uintptr_t objAddr, size_t fieldOffset);
    virtual size_t DerivedExtraReadSingleEncodeData(uint8_t encodeFlag, uintptr_t objAddr, size_t fieldOffset);
    void HandleNewObjectEncodeFlag(SerializedObjectSpace space, uintptr_t objAddr, size_t fieldOffset);

    void TransferArrayBufferAttach(uintptr_t objAddr);
    void IncreaseSharedArrayBufferReference(uintptr_t objAddr);
    void ResetNativePointerBuffer(uintptr_t objAddr, void *bufferPointer);

    void AllocateToDifferentSpaces();
    enum class RegionType : uint8_t {
        RegularRegion,
        PinRegion,
    };
    void AllocateToRegularSpace(size_t regularSpaceSize);
    void AllocateToPinSpace(size_t pinSpaceSize);
    uintptr_t AllocateMultiCMCRegion(size_t spaceObjSize, size_t &regionIndex, RegionType regionType);
    void AllocateMultiRegion(SparseSpace *space, size_t spaceObjSize, size_t &regionIndex,
                             SerializedObjectSpace spaceType);
    void AllocateMultiSharedRegion(SharedSparseSpace *space, size_t spaceObjSize, size_t &regionIndex,
                                   SerializedObjectSpace spaceType);
    void AllocateToOldSpace(size_t oldSpaceSize);
    void AllocateToNonMovableSpace(size_t nonMovableSpaceSize);
    void AllocateToMachineCodeSpace(size_t machineCodeSpaceSize);
    void AllocateToSharedOldSpace(size_t sOldSpaceSize);
    void AllocateToSharedNonMovableSpace(size_t sNonMovableSpaceSize);
    bool GetAndResetWeak()
    {
        bool isWeak = isWeak_;
        if (isWeak_) {
            isWeak_ = false;
        }
        return isWeak;
    }

    bool GetAndResetTransferBuffer()
    {
        bool isTransferArrayBuffer = isTransferArrayBuffer_;
        if (isTransferArrayBuffer_) {
            isTransferArrayBuffer_ = false;
        }
        return isTransferArrayBuffer;
    }

    bool GetAndResetSharedArrayBuffer()
    {
        bool isSharedArrayBuffer = isSharedArrayBuffer_;
        if (isSharedArrayBuffer_) {
            isSharedArrayBuffer_ = false;
        }
        return isSharedArrayBuffer;
    }

    bool GetAndResetIsErrorMsg()
    {
        bool isErrorMsg = isErrorMsg_;
        if (isErrorMsg_) {
            isErrorMsg_ = false;
        }
        return isErrorMsg;
    }

    bool GetAndResetFunctionInShared()
    {
        bool functionInShared = functionInShared_;
        if (functionInShared_) {
            functionInShared_ = false;
        }
        return functionInShared;
    }

    void *GetAndResetBufferPointer()
    {
        if (bufferPointer_) {
            void *buffer = bufferPointer_;
            bufferPointer_ = nullptr;
            return buffer;
        }
        return nullptr;
    }

    void DeserializeFatalOutOfMemory(size_t size, bool dump = true, bool isShared = false)
    {
        if (isShared) {
            if (dump) {
                sheap_->DumpHeapSnapshotBeforeOOM(false, thread_, SharedHeapOOMSource::DESERIALIZE);
            }
            LOG_ECMA(FATAL) << "BaseDeserializer::OutOfMemory when deserialize shared obj size: " << size
                << ", old space heap object size: "
                << sheap_->GetOldSpace()->GetHeapObjectSize()
                << ", old space committed size: "
                << sheap_->GetOldSpace()->GetCommittedSize()
                << ", non movable space heap object size: "
                << sheap_->GetNonMovableSpace()->GetHeapObjectSize()
                << ", non movable space committed size: "
                << sheap_->GetNonMovableSpace()->GetCommittedSize()
                << ", huge space committed size: "
                << sheap_->GetHugeObjectSpace()->GetCommittedSize();
        } else {
            if (dump) {
                heap_->StatisticHeapDetail();
                heap_->DumpHeapSnapshotBeforeOOM(false);
            }
            LOG_ECMA(FATAL) << "BaseDeserializer::OutOfMemory when deserialize obj size: " << size
                << ", old space heap object size: "
                << heap_->GetOldSpace()->GetHeapObjectSize()
                << ", old space committed size: "
                << heap_->GetOldSpace()->GetCommittedSize()
                << ", non movable space heap object size: "
                << heap_->GetNonMovableSpace()->GetHeapObjectSize()
                << ", non movable space committed size: "
                << heap_->GetNonMovableSpace()->GetCommittedSize()
                << ", huge space committed size: "
                << heap_->GetHugeObjectSpace()->GetCommittedSize();
        }
    }

    void UpdateMaybeWeak(ObjectSlot slot, uintptr_t addr, bool isWeak)
    {
        isWeak ? slot.UpdateWeak(addr) : slot.Update(addr);
    }

    bool *GetLazyArray()
    {
        if (moduleLazyArray_) {
            bool *buffer = moduleLazyArray_;
            moduleLazyArray_ = nullptr;
            return buffer;
        }
        return nullptr;
    }

protected:
    JSThread *thread_;
    SerializeData *data_;
    void *engine_;
    size_t position_ {0};
    CVector<JSTaggedType> objectVector_ {};

    virtual void DeserializeSpecialRecordedObjects() {}

private:
    Heap *heap_;
    SharedHeap *sheap_;
    uintptr_t currentRegularObjectAddr_ {0};
    uintptr_t currentRegularRegionBeginAddr_ {0};
    uintptr_t currentPinObjectAddr_ {0};
    uintptr_t currentPinRegionBeginAddr_ {0};
    size_t regularRegionIndex_ {0};
    size_t pinRegionIndex_ {0};
    CVector<uintptr_t> regionVector_;
    uintptr_t oldSpaceBeginAddr_ {0};
    uintptr_t nonMovableSpaceBeginAddr_ {0};
    uintptr_t machineCodeSpaceBeginAddr_ {0};
    uintptr_t sOldSpaceBeginAddr_ {0};
    uintptr_t sNonMovableSpaceBeginAddr_ {0};
    size_t oldRegionIndex_ {0};
    size_t nonMovableRegionIndex_ {0};
    size_t machineCodeRegionIndex_ {0};
    size_t sOldRegionIndex_ {0};
    size_t sNonMovableRegionIndex_ {0};
    // SerializationChunk store shared objects which have been serialized
    SerializationChunk *sharedObjChunk_ {nullptr};
    bool isWeak_ {false};
    bool isTransferArrayBuffer_ {false};
    bool isSharedArrayBuffer_ {false};
    bool isErrorMsg_ {false};
    void *bufferPointer_ {nullptr};
    bool functionInShared_ {false};
    CVector<NativeBindingAttachInfo> nativeBindingAttachInfos_;
    CVector<JSErrorInfo> jsErrorInfos_;
    CVector<JSHandle<JSFunction>> concurrentFunctions_;
    // module deserialize
    CString moduleFileName_ {};
    CString moduleRecordName_ {};
    bool* moduleLazyArray_ {nullptr};
};
}

#endif  // ECMASCRIPT_SERIALIZER_BASE_DESERIALIZER_H
