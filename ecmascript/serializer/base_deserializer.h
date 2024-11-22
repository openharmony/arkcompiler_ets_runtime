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
struct NativeBindingInfo {
    AttachFunc af_ {nullptr};
    void *bufferPointer_ {nullptr};
    void *hint_ = {nullptr};
    void *attachData_ = {nullptr};
    uintptr_t objAddr_ {0U};
    size_t offset_ {0U};
    bool root_ {false};

    NativeBindingInfo(AttachFunc af, void *bufferPointer, void *hint, void *attachData,
                      uintptr_t objAddr, size_t offset, bool root) : af_(af), bufferPointer_(bufferPointer),
        hint_(hint), attachData_(attachData), objAddr_(objAddr), offset_(offset), root_(root) {}

    uintptr_t GetObjAddr() const
    {
        return objAddr_;
    }

    size_t GetFieldOffset() const
    {
        return offset_;
    }

    ObjectSlot GetSlot() const
    {
        return ObjectSlot(objAddr_ + offset_);
    }
};

struct JSErrorInfo {
    uint8_t errorType_ {0};
    JSTaggedValue errorMsg_;
    uintptr_t objAddr_ {0U};
    size_t offset_ {0U};
    bool root_ {false};

    JSErrorInfo(uint8_t errorType, JSTaggedValue errorMsg, uintptr_t objAddr, size_t offset, bool root)
        : errorType_(errorType), errorMsg_(errorMsg), objAddr_(objAddr), offset_(offset), root_(root) {}

    uintptr_t GetObjAddr() const
    {
        return objAddr_;
    }

    size_t GetFieldOffset() const
    {
        return offset_;
    }

    ObjectSlot GetSlot() const
    {
        return ObjectSlot(objAddr_ + offset_);
    }
};

class BaseDeserializer {
public:
    explicit BaseDeserializer(JSThread *thread, SerializeData *data, void *hint = nullptr);

    ~BaseDeserializer()
    {
        objectVector_.clear();
        regionVector_.clear();
    }

    NO_COPY_SEMANTIC(BaseDeserializer);
    NO_MOVE_SEMANTIC(BaseDeserializer);

    JSHandle<JSTaggedValue> ReadValue();

private:
    JSHandle<JSTaggedValue> DeserializeJSTaggedValue();
    uintptr_t DeserializeTaggedObject(SerializedObjectSpace space);
    void DeserializeNativeBindingObject(NativeBindingInfo *info);
    void DeserializeJSError(JSErrorInfo *info);
    uintptr_t RelocateObjectAddr(SerializedObjectSpace space, size_t objSize);
    JSTaggedType RelocateObjectProtoAddr(uint8_t objectType);
    void DeserializeObjectField(uintptr_t start, uintptr_t end);
    size_t ReadSingleEncodeData(uint8_t encodeFlag, uintptr_t objAddr, size_t fieldOffset, bool isRoot = false);
    void HandleNewObjectEncodeFlag(SerializedObjectSpace space, uintptr_t objAddr, size_t fieldOffset, bool isRoot);

    void TransferArrayBufferAttach(uintptr_t objAddr);
    void IncreaseSharedArrayBufferReference(uintptr_t objAddr);
    void ResetNativePointerBuffer(uintptr_t objAddr, void *bufferPointer);

    void AllocateToDifferentSpaces();
    void AllocateMultiRegion(SparseSpace *space, size_t spaceObjSize, size_t &regionIndex);
    void AllocateMultiSharedRegion(SharedSparseSpace *space, size_t spaceObjSize, size_t &regionIndex);
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

private:
    JSThread *thread_;
    Heap *heap_;
    SharedHeap *sheap_;
    SerializeData* data_;
    void *engine_;
    uintptr_t oldSpaceBeginAddr_ {0};
    uintptr_t nonMovableSpaceBeginAddr_ {0};
    uintptr_t machineCodeSpaceBeginAddr_ {0};
    uintptr_t sOldSpaceBeginAddr_ {0};
    uintptr_t sNonMovableSpaceBeginAddr_ {0};
    // SerializationChunk store shared objects which have been serialized
    SerializationChunk *sharedObjChunk_ {nullptr};
    CVector<JSTaggedType> objectVector_;
    CVector<Region *> regionVector_;
    size_t oldRegionIndex_ {0};
    size_t nonMovableRegionIndex_ {0};
    size_t machineCodeRegionIndex_ {0};
    size_t sOldRegionIndex_ {0};
    size_t sNonMovableRegionIndex_ {0};
    size_t regionRemainSizeIndex_ {0};
    bool isWeak_ {false};
    bool isTransferArrayBuffer_ {false};
    bool isSharedArrayBuffer_ {false};
    bool isErrorMsg_ {false};
    void *bufferPointer_ {nullptr};
    bool functionInShared_ {false};
    CVector<NativeBindingInfo *> nativeBindingInfos_;
    CVector<JSErrorInfo *> jsErrorInfos_;
    CVector<JSHandle<JSFunction>> concurrentFunctions_;
    size_t position_ {0};
};
}

#endif  // ECMASCRIPT_SERIALIZER_BASE_DESERIALIZER_H