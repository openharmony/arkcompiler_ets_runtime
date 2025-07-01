/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/shared_objects/js_sendable_arraybuffer.h"
#include "ecmascript/mem/barriers-inl.h"
#include "ecmascript/platform/os.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript {
void JSSendableArrayBuffer::CopyDataBlockBytes(
    JSTaggedValue toBlock, JSTaggedValue fromBlock, int32_t fromIndex, int32_t count)
{
    void *fromBuf = JSNativePointer::Cast(fromBlock.GetTaggedObject())->GetExternalPointer();
    void *toBuf = JSNativePointer::Cast(toBlock.GetTaggedObject())->GetExternalPointer();
    CopyDataPointBytes(toBuf, fromBuf, fromIndex, count);
}

void JSSendableArrayBuffer::CopyDataPointBytes(void *toBuf, void *fromBuf, int32_t fromIndex, int32_t count)
{
    auto *from = static_cast<uint8_t *>(fromBuf);
    auto *to = static_cast<uint8_t *>(toBuf);
    ASSERT(from != nullptr && to != nullptr);
    // LCOV_EXCL_START
    if (memcpy_s(to, count, from + fromIndex, count) != EOK) {  // NOLINT
        LOG_FULL(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    } // LCOV_EXCL_STOP
}

void JSSendableArrayBuffer::Attach(JSThread *thread, uint32_t arrayBufferByteLength,
                                   JSTaggedValue arrayBufferData, bool transferWithNativeAreaAllocator)
{
    ASSERT(arrayBufferData.IsJSNativePointer());
    // only in transition, should the JSSendableArrayBuffer with NativeAreaAllocator increase mem usage
    if (transferWithNativeAreaAllocator) {
        LOG_FULL(DEBUG) << "attaching for transfer";
        JSHandle<JSNativePointer> np(thread, arrayBufferData.GetTaggedObject());
        NativeAreaAllocator *allocator = SharedHeap::GetInstance()->GetNativeAreaAllocator();
        if (allocator == nullptr) {
            LOG_ECMA(FATAL) << "JSSendableArrayBuffer::Attach:allocator is nullptr";
        }
        allocator->IncreaseNativeMemoryUsage(MallocUsableSize(np->GetExternalPointer()));
        np->SetData(allocator);
    }
    SetArrayBufferByteLength(arrayBufferByteLength);
    SetArrayBufferData(thread, arrayBufferData);
}

void JSSendableArrayBuffer::Detach(JSThread *thread, bool transferWithNativeAreaAllocator)
{
    JSTaggedValue arrayBufferData = GetArrayBufferData(thread);
    // already detached.
    if (arrayBufferData.IsNull()) {
        return;
    }

    // only in transition, should the JSSendableArrayBuffer with NativeAreaAllocator decrease mem usage
    if (transferWithNativeAreaAllocator) {
        LOG_FULL(DEBUG) << "detaching for transfer";
        JSHandle<JSNativePointer> np(thread, arrayBufferData.GetTaggedObject());
        NativeAreaAllocator *allocator = SharedHeap::GetInstance()->GetNativeAreaAllocator();
        allocator->DecreaseNativeMemoryUsage(MallocUsableSize(np->GetExternalPointer()));
        np->SetData(nullptr);
    }
    SetArrayBufferData(thread, JSTaggedValue::Null());
    SetArrayBufferByteLength(0);
}
}  // namespace panda::ecmascript