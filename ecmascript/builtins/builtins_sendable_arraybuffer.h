/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_SENDABLE_ARRAYBUFFER_H
#define ECMASCRIPT_BUILTINS_BUILTINS_SENDABLE_ARRAYBUFFER_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/builtins/builtins_arraybuffer.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/js_dataview.h"
#include "ecmascript/js_typed_array.h"

// List of functions in ArrayBuffer, excluding the '@@' properties.
// V(name, func, length, stubIndex)
// where BuiltinsArrayBuffer::func refers to the native implementation of ArrayBuffer[name].
//       kungfu::BuiltinsStubCSigns::stubIndex refers to the builtin stub index, or INVALID if no stub available.
#define BUILTIN_ARRAY_BUFFER_FUNCTIONS(V)                                           \
    /* ArrayBuffer.isView ( arg ) */                                                \
    V("isView", IsView, 1, ArrayBufferIsView)

namespace panda::ecmascript::builtins {
using DataViewType = ecmascript::DataViewType;
using BuiltinFunctionEntry = base::BuiltinFunctionEntry;

class BuiltinsSendableArrayBuffer : public BuiltinsArrayBuffer {
public:
    // 24.1.2.1 ArrayBuffer(length)
    static JSTaggedValue ArrayBufferConstructor(EcmaRuntimeCallInfo *argv);
    // 24.1.3.1 ArrayBuffer.isView(arg)
    static JSTaggedValue IsView(EcmaRuntimeCallInfo *argv);
    // 24.1.3.3 get ArrayBuffer[@@species]
    static JSTaggedValue Species(EcmaRuntimeCallInfo *argv);
    // 24.1.4.1 get ArrayBuffer.prototype.byteLength
    static JSTaggedValue GetByteLength(EcmaRuntimeCallInfo *argv);
    // 24.1.4.3 ArrayBuffer.prototype.slice()
    static JSTaggedValue Slice(EcmaRuntimeCallInfo *argv);
    // 24.1.1.2 IsDetachedBuffer(arrayBuffer)

    static bool IsDetachedBuffer(JSTaggedValue arrayBuffer);
    // 24.1.1.4 CloneArrayBuffer( srcBuffer, srcByteOffset [, cloneConstructor] )
    static JSTaggedValue CloneArrayBuffer(JSThread *thread, const JSHandle<JSTaggedValue> &srcBuffer,
                                          uint32_t srcByteOffset, JSHandle<JSTaggedValue> constructor);
    // 24.1.1.1 AllocateArrayBuffer(constructor, byteLength)
    static JSTaggedValue AllocateSendableArrayBuffer(
        JSThread *thread, const JSHandle<JSTaggedValue> &newTarget, uint64_t byteLength);

    // Excluding the '@@' internal properties
    static Span<const base::BuiltinFunctionEntry> GetArrayBufferFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(ARRAY_BUFFER_FUNCTIONS);
    }

    static size_t GetNumPrototypeInlinedProperties()
    {
        // 3 : 3 more inline properties in Set.prototype
        //  (1) ArrayBuffer.prototype.slice
        //  (2) get ArrayBuffer.prototype.byteLength
        //  (3) ArrayBuffer.prototype [ @@toStringTag ]
        return 3;
    }

    static Span<const std::pair<std::string_view, bool>> GetPrototypeProperties()
    {
        return Span<const std::pair<std::string_view, bool>>(ARRAYBUFFER_PROTOTYPE_PROPERTIES);
    }

    static Span<const std::pair<std::string_view, bool>> GetFunctionProperties()
    {
        return Span<const std::pair<std::string_view, bool>>(ARRAYBUFFER_FUNCTION_PROPERTIES);
    }

    static void *GetDataPointFromBuffer(JSTaggedValue arrBuf, uint32_t byteOffset = 0);
private:
#define BUILTIN_ARRAY_BUFFER_ENTRY(name, func, length, id) \
    BuiltinFunctionEntry::Create(name, BuiltinsSendableArrayBuffer::func, length, BUILTINS_STUB_ID(id)),

    static constexpr std::array ARRAY_BUFFER_FUNCTIONS = {
        BUILTIN_ARRAY_BUFFER_FUNCTIONS(BUILTIN_ARRAY_BUFFER_ENTRY)
    };
#undef BUILTIN_ARRAY_BUFFER_ENTRY

#define ARRAYBUFFER_PROPERTIES_PAIR(name, func, length, id) \
    std::pair<std::string_view, bool>(name, false),

    static constexpr std::array ARRAYBUFFER_PROTOTYPE_PROPERTIES = {
        std::pair<std::string_view, bool>("slice", false),
        std::pair<std::string_view, bool>("byteLength", true),
        std::pair<std::string_view, bool>("[Symbol.toStringTag]", false),
    };

    static constexpr std::array ARRAYBUFFER_FUNCTION_PROPERTIES = {
        std::pair<std::string_view, bool>("length", false),
        std::pair<std::string_view, bool>("name", false),
        std::pair<std::string_view, bool>("prototype", false),
        std::pair<std::string_view, bool>("[Symbol.species]", true),
        BUILTIN_ARRAY_BUFFER_FUNCTIONS(ARRAYBUFFER_PROPERTIES_PAIR)
    };
#undef ARRAYBUFFER_PROPERTIES_PAIR
};
}  // namespace panda::ecmascript::builtins

#endif  // ECMASCRIPT_BUILTINS_BUILTINS_SENDABLE_ARRAYBUFFER_H
