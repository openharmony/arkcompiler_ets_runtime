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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_DATAVIEW_H
#define ECMASCRIPT_BUILTINS_BUILTINS_DATAVIEW_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/js_dataview.h"

// List of functions in DataView, excluding the constructor and '@@' properties.
// V(name, func, length, stubIndex)
// where BuiltinsDataView::func refers to the native implementation of DataView.prototype[name].
//       kungfu::BuiltinsStubCSigns::stubIndex refers to the builtin stub index, or INVALID if no stub available.
#define BUILTIN_DATA_VIEW_PROTOTYPE_FUNCTIONS(V)                                    \
    /* For %Type% of 1 byte: */                                                     \
    /*   DataView.prototype.get%Type% ( byteOffset ) */                             \
    /* For %Type% of 2 or more bytes: */                                            \
    /*   DataView.prototype.get%Type% ( byteOffset [ , littleEndian ] ) */          \
    V("getFloat32",     GetFloat32,     1, DataViewGetFloat32  )                    \
    V("getFloat64",     GetFloat64,     1, DataViewGetFloat64  )                    \
    V("getInt8",        GetInt8,        1, DataViewGetInt8     )                    \
    V("getInt16",       GetInt16,       1, DataViewGetInt16    )                    \
    V("getInt32",       GetInt32,       1, DataViewGetInt32    )                    \
    V("getBigInt64",    GetBigInt64,    1, INVALID             )                    \
    V("getUint16",      GetUint16,      1, DataViewGetUint16   )                    \
    V("getUint32",      GetUint32,      1, DataViewGetUint32   )                    \
    V("getUint8",       GetUint8,       1, DataViewGetUint8    )                    \
    V("getBigUint64",   GetBigUint64,   1, INVALID             )                    \
    /* For %Type% of 1 bytes: */                                                    \
    /*   DataView.prototype.setInt8 ( byteOffset, value ) */                        \
    /* For %Type% of 2 or more bytes: */                                            \
    /*   DataView.prototype.setInt16 ( byteOffset, value [ , littleEndian ] ) */    \
    V("setFloat32",     SetFloat32,     2, DataViewSetFloat32)                      \
    V("setFloat64",     SetFloat64,     2, DataViewSetFloat64)                      \
    V("setInt8",        SetInt8,        2, DataViewSetInt8)                         \
    V("setInt16",       SetInt16,       2, DataViewSetInt16)                        \
    V("setInt32",       SetInt32,       2, DataViewSetInt32)                        \
    V("setBigInt64",    SetBigInt64,    2, INVALID)                                 \
    V("setUint8",       SetUint8,       2, DataViewSetUint8)                        \
    V("setUint16",      SetUint16,      2, DataViewSetUint16)                       \
    V("setUint32",      SetUint32,      2, DataViewSetUint32)                       \
    V("setBigUint64",   SetBigUint64,   2, INVALID)

namespace panda::ecmascript::builtins {
using DataViewType = ecmascript::DataViewType;
class BuiltinsDataView : public base::BuiltinsBase {
public:
    // 24.2.2.1 DataView (buffer [ , byteOffset [ , byteLength ] ] )
    static JSTaggedValue DataViewConstructor(EcmaRuntimeCallInfo *argv);
    // 24.2.4.1 get DataView.prototype.buffer
    static JSTaggedValue GetBuffer(EcmaRuntimeCallInfo *argv);
    // 24.2.4.2 get DataView.prototype.byteLength
    static JSTaggedValue GetByteLength(EcmaRuntimeCallInfo *argv);
    // 24.2.4.3 get DataView.prototype.byteOffset
    static JSTaggedValue GetOffset(EcmaRuntimeCallInfo *argv);
    // 24.2.4.5 DataView.prototype.getFloat32 ( byteOffset [ , littleEndian ] )
    static JSTaggedValue GetFloat32(EcmaRuntimeCallInfo *argv);
    // 24.2.4.6 DataView.prototype.getFloat64 ( byteOffset [ , littleEndian ] )
    static JSTaggedValue GetFloat64(EcmaRuntimeCallInfo *argv);
    // 24.2.4.7 DataView.prototype.getInt8 ( byteOffset )
    static JSTaggedValue GetInt8(EcmaRuntimeCallInfo *argv);
    // 24.2.4.8 DataView.prototype.getInt16 ( byteOffset [ , littleEndian ] )
    static JSTaggedValue GetInt16(EcmaRuntimeCallInfo *argv);
    // 24.2.4.9 DataView.prototype.getInt32 ( byteOffset [ , littleEndian ] )
    static JSTaggedValue GetInt32(EcmaRuntimeCallInfo *argv);
    // 24.2.4.10 DataView.prototype.getUint8 ( byteOffset )
    static JSTaggedValue GetUint8(EcmaRuntimeCallInfo *argv);
    // 24.2.4.11 DataView.prototype.getUint16 ( byteOffset [ , littleEndian ] )
    static JSTaggedValue GetUint16(EcmaRuntimeCallInfo *argv);
    // 24.2.4.12 DataView.prototype.getUint32 ( byteOffset [ , littleEndian ] )
    static JSTaggedValue GetUint32(EcmaRuntimeCallInfo *argv);
    // 25.3.4.5 DataView.prototype.getBigInt64 ( byteOffset [ , littleEndian ] )
    static JSTaggedValue GetBigInt64(EcmaRuntimeCallInfo *argv);
    // 25.3.4.6 DataView.prototype.getBigUint64 ( byteOffset [ , littleEndian ] )
    static JSTaggedValue GetBigUint64(EcmaRuntimeCallInfo *argv);
    // 24.2.4.13 DataView.prototype.setFloat32 ( byteOffset, value [ , littleEndian ] )
    static JSTaggedValue SetFloat32(EcmaRuntimeCallInfo *argv);
    // 24.2.4.14 DataView.prototype.setFloat64 ( byteOffset, value [ , littleEndian ] )
    static JSTaggedValue SetFloat64(EcmaRuntimeCallInfo *argv);
    // 24.2.4.15 DataView.prototype.setInt8 ( byteOffset, value )
    static JSTaggedValue SetInt8(EcmaRuntimeCallInfo *argv);
    // 24.2.4.16 DataView.prototype.setInt16 ( byteOffset, value [ , littleEndian ] )
    static JSTaggedValue SetInt16(EcmaRuntimeCallInfo *argv);
    // 24.2.4.17 DataView.prototype.setInt32 ( byteOffset, value [ , littleEndian ] )
    static JSTaggedValue SetInt32(EcmaRuntimeCallInfo *argv);
    // 24.2.4.18 DataView.prototype.setUint8 ( byteOffset, value )
    static JSTaggedValue SetUint8(EcmaRuntimeCallInfo *argv);
    // 24.2.4.19 DataView.prototype.setUint16( byteOffset, value [ , littleEndian ] )
    static JSTaggedValue SetUint16(EcmaRuntimeCallInfo *argv);
    // 24.2.4.20 DataView.prototype.setUint32 ( byteOffset, value [ , littleEndian ] )
    static JSTaggedValue SetUint32(EcmaRuntimeCallInfo *argv);
    // 25.3.4.15 DataView.prototype.setBigInt64 ( byteOffset, value [ , littleEndian ] )
    static JSTaggedValue SetBigInt64(EcmaRuntimeCallInfo *argv);
    // 25.3.4.16 DataView.prototype.setBigUint64 ( byteOffset, value [ , littleEndian ] )
    static JSTaggedValue SetBigUint64(EcmaRuntimeCallInfo *argv);

    // Excluding the '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetDataViewPrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(DATA_VIEW_PROTOTYPE_FUNCTIONS);
    }

    static size_t GetNumPrototypeInlinedProperties()
    {
        // 5 : 5 more inline properties in DataView.prototype:
        //   (1) DataView.prototype.constructor
        //   (2) DataView.prototype [ @@toStringTag ]
        //   (3) get buffer
        //   (4) get byteLength
        //   (5) get byteOffset
        return GetDataViewPrototypeFunctions().Size() + 5;
    }

private:
#define BUILTIN_DATA_VIEW_FUNCTION_ENTRY(name, func, length, id) \
    base::BuiltinFunctionEntry::Create(name, BuiltinsDataView::func, length, kungfu::BuiltinsStubCSigns::id),

    static constexpr std::array DATA_VIEW_PROTOTYPE_FUNCTIONS = {
        BUILTIN_DATA_VIEW_PROTOTYPE_FUNCTIONS(BUILTIN_DATA_VIEW_FUNCTION_ENTRY)
    };

#undef BUILTIN_DATA_VIEW_FUNCTION_ENTRY

    // 24.2.1.1 GetViewValue ( view, requestIndex, isLittleEndian, type )
    static JSTaggedValue GetViewValue(JSThread *thread, const JSHandle<JSTaggedValue> &view,
                                      const JSHandle<JSTaggedValue> &requestIndex,
                                      const JSHandle<JSTaggedValue> &littleEndian,
                                      DataViewType type);
    static JSTaggedValue SetViewValue(JSThread *thread, const JSHandle<JSTaggedValue> &view,
                                      const JSHandle<JSTaggedValue> &requestIndex,
                                      const JSHandle<JSTaggedValue> &littleEndian,
                                      DataViewType type, const JSHandle<JSTaggedValue> &value);

    static JSTaggedValue GetTypedValue(EcmaRuntimeCallInfo *argv, DataViewType type);
    static JSTaggedValue SetTypedValue(EcmaRuntimeCallInfo *argv, DataViewType type);
};
}  // namespace panda::ecmascript::builtins

#endif  // ECMASCRIPT_BUILTINS_BUILTINS_DATAVIEW_H
