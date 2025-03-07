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

#ifndef ECMASCRIPT_MESSAGE_STRING_H
#define ECMASCRIPT_MESSAGE_STRING_H

#include <string>

#include "ecmascript/compiler/common_stub_csigns.h"
#include "ecmascript/compiler/interpreter_stub.h"

namespace panda::ecmascript {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define COMMON_MESSAGE_STRING_LIST(V)                                                        \
    V(SetReadOnlyProperty, "Cannot assign to read only property")                            \
    V(SetTypeMismatchedSharedProperty, "Cannot set sendable property with mismatched type")  \
    V(CreateObjectWithSendableProto, "Cannot create object with sendable proto")             \
    V(UpdateSendableAttributes, "Cannot update sendable object's attributes")                \
    V(SetProtoWithSendable, "Cannot set proto with sendable object")                         \
    V(ClassNotDerivedFromShared, "Class not derived from a sendable object")                 \
    V(NotSendableSubClass, "The subclass of sendable class must be a sendable class")        \
    V(FunctionCallNotConstructor, "class constructor cannot call")                           \
    V(SetPropertyWhenNotExtensible, "Cannot add property in prevent extensions")             \
    V(GetPropertyOutOfBounds, "Get Property index out-of-bounds")                            \
    V(CanNotSetPropertyOnContainer, "Cannot set property on Container")                      \
    V(NonCallable, "CallObj is NonCallable")                                                 \
    V(ASM_INTERPRETER_STUB_NAME, "ASM_INTERPRETER stub name: ")                              \
    V(OPCODE_OVERFLOW, "opcode overflow!")                                                   \
    V(INT32_VALUE, "value: %ld")                                                             \
    V(TargetTypeNotObject, "Type of target is not Object")                                   \
    V(TargetTypeNotTypedArray, "The O is not a TypedArray.")                                 \
    V(CanNotGetNotEcmaObject, "Can not get Prototype on non ECMA Object")                    \
    V(SendableArrayForJson, "Array not supported for SENDABLE_JSON")                         \
    V(InstanceOfErrorTargetNotCallable, "InstanceOf error when target is not Callable")      \
    V(ApplyTargetNotCallable, "apply target is not callable")                                \
    V(TargetNotStableJSArray, "target not stable JSArray")                                   \
    V(LenGreaterThanMax, "len is bigger than 2^32 - 1")                                      \
    V(ElementTypeNoElementTypes, "CreateListFromArrayLike: not an element of elementTypes")  \
    V(TargetIsDetachedBuffer, "Is Detached Buffer")                                          \
    V(ThisBranchIsUnreachable, "this branch is unreachable")                                 \
    V(CanNotConvertNotUndefinedObject, "Cannot convert a UNDEFINED value to a JSObject")     \
    V(CanNotConvertNotNullObject, "Cannot convert a NULL value to a JSObject")               \
    V(CanNotConvertNotHoleObject, "Cannot convert a HOLE value to a JSObject")               \
    V(CanNotConvertUnknowObject, "Cannot convert a Unknown object value to a JSObject")      \
    V(CanNotConvertNotValidObject, "Obj is not a valid object")                              \
    V(CanNotConvertContainerObject, "Can not delete property in Container Object")           \
    V(InvalidStringLength, "Invalid string length")                                          \
    V(InvalidNewTarget, "new.target is not an object")                                       \
    V(ObjIsNotCallable, "obj is not Callable")                                               \
    V(SharedObjectRefersLocalObject, "shared object refers a local object")                  \
    V(InvalidRadixLength, "radix must be 2 to 36")                                           \
    V(SetPrototypeOfFailed, "SetPrototypeOf: prototype set failed")                          \
    V(ReviverOnlySupportUndefined, "reviver only supports undefined for SENDABLE_JSON")      \
    V(DefineFieldField, "DefineField: obj is not Object")                                    \
    V(IsNotPropertyKey, "key is not a property key")                                         \
    V(CreateDataPropertyFailed, "failed to create data property")                            \
    V(ValueIsNonSObject, "value is not a shared object")                                     \
    V(MapIteratorTypeError, "this value is not a map iterator")                              \
    V(SetIteratorTypeError, "this value is not a set iterator")                              \
    V(LengthError, "length must be positive integer")                                        \
    V(IterNotObject, "JSIterator::GetIterator: iter is not object")                          \
    V(CanNotConvertObjectToPrimitiveValue, "Cannot convert object to primitive value")       \
    V(CanNotConvertIllageValueToString, "Cannot convert a illegal value to a String")        \
    V(CanNotConvertIllageValueToPrimitive, "Cannot convert a illegal value to a Primitive")  \
    V(ProxyGetPropertyHandlerIsNull, "JSProxy::GetProperty: handler is Null")                \
    V(ProxySetPropertyHandlerIsNull, "JSProxy::SetProperty: handler is Null")                \
    V(ProxySetPropertyReturnFalse, "JSProxy::SetProperty: 'set' return false")               \
    V(InOperatorOnNonObject, "Cannot use 'in' operator in Non-Object")

#define DEBUG_CHECK_MESSAGE_STRING_LIST(V)                                                   \
    V(IsCallable)                                                                            \
    V(LoadHClass)                                                                            \
    V(IsDictionaryMode)                                                                      \
    V(IsClassConstructor)                                                                    \
    V(IsClassPrototype)                                                                      \
    V(IsExtensible)                                                                          \
    V(IsEcmaObject)                                                                          \
    V(IsJSObject)                                                                            \
    V(IsString)                                                                              \
    V(IsJSHClass)                                                                            \
    V(IsNotDictionaryMode)                                                                   \
    V(InitializeWithSpeicalValue)                                                            \
    V(IsSendableFunctionModule)

class MessageString {
public:
    enum MessageId {
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEF_MESSAGE_ID(name, string) Message_##name,
        COMMON_MESSAGE_STRING_LIST(DEF_MESSAGE_ID)
#undef DEF_MESSAGE_ID
#define DEF_MESSAGE_ID(name) Message_##name,
        ASM_INTERPRETER_BC_STUB_LIST(DEF_MESSAGE_ID, DEF_MESSAGE_ID, DEF_MESSAGE_ID)
        ASM_INTERPRETER_SECOND_BC_STUB_ID_LIST(DEF_MESSAGE_ID)
        ASM_INTERPRETER_BC_HELPER_STUB_LIST(DEF_MESSAGE_ID)
        BASELINE_COMPILER_BUILTIN_LIST(DEF_MESSAGE_ID)
        COMMON_STUB_LIST(DEF_MESSAGE_ID)
#define DEF_MESSAGE_ID_DYN(name, ...) DEF_MESSAGE_ID(name)
        ASM_INTERPRETER_BC_PROFILER_STUB_LIST(DEF_MESSAGE_ID_DYN)
#undef DEF_MESSAGE_ID_DYN
#define DEF_BUILTINS_STUB_MESSAGE_ID(name, type, ...) Message_##type##name,
        BUILTINS_STUB_LIST(DEF_MESSAGE_ID, DEF_BUILTINS_STUB_MESSAGE_ID, DEF_MESSAGE_ID)
#undef DEF_BUILTINS_STUB_MESSAGE_ID
        RUNTIME_ASM_STUB_LIST(DEF_MESSAGE_ID)
        DEBUG_CHECK_MESSAGE_STRING_LIST(DEF_MESSAGE_ID)
#undef DEF_MESSAGE_ID
        MAX_MESSAGE_COUNT,
        ASM_INTERPRETER_START = Message_INT32_VALUE + 1,
        BUILTINS_STUB_START = Message_StringCharCodeAt,
        BUILTINS_STUB_LAST = Message_ArrayConstructor,
    };
    static const std::string& PUBLIC_API GetMessageString(int id);

    static bool IsBuiltinsStubMessageString(int id)
    {
        return (id >= BUILTINS_STUB_START) && (id <= BUILTINS_STUB_LAST);
    }
};

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GET_MESSAGE_STRING_ID(name) static_cast<int>((MessageString::MessageId::Message_##name))
#define GET_MESSAGE_STRING(name)  MessageString::GetMessageString(GET_MESSAGE_STRING_ID(name)).c_str()
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MESSAGE_STRING_H
