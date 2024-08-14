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

#include "ecmascript/byte_array.h"

#include "ecmascript/builtins/builtins_arraybuffer.h"

namespace panda::ecmascript {
void ByteArray::Set(JSThread* thread, uint32_t idx, DataViewType type, JSTaggedType val, uint32_t offset)
{
    void *pointer = GetData();
    auto *block = reinterpret_cast<uint8_t *>(pointer) + offset;
    builtins::BuiltinsArrayBuffer::SetValueInBuffer(thread, idx * GetByteLength(), block,
                                                    type, JSTaggedValue(val).GetNumber(), true);
}

JSTaggedValue ByteArray::Get(JSThread *thread, uint32_t idx, DataViewType type, uint32_t offset)
{
    void *pointer = GetData();
    auto *block = reinterpret_cast<uint8_t *>(pointer) + offset;
    return builtins::BuiltinsArrayBuffer::GetValueFromBuffer(thread, idx * GetByteLength(), block, type, true);
}
}  // namespace panda::ecmascript
