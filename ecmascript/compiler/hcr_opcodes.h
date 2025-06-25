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

#ifndef ECMASCRIPT_COMPILER_HCR_OPCODE_H
#define ECMASCRIPT_COMPILER_HCR_OPCODE_H

namespace panda::ecmascript::kungfu {

#define HCR_IMMUTABLE_META_DATA_CACHE_LIST(V)                                                   \
    V(NumberToString, NUMBER_TO_STRING, GateFlags::NONE_FLAG, 1, 1, 1)                          \
    V(ToLength, TO_LENGTH, GateFlags::NONE_FLAG, 1, 1, 1)                                       \
    V(StoreModuleVar, STORE_MODULE_VAR, GateFlags::NONE_FLAG, 1, 1, 3)                          \
    V(OrdinaryHasInstance, ORDINARY_HAS_INSTANCE, GateFlags::CHECKABLE, 1, 1, 2)                \
    V(MigrateArrayWithKind, MIGRATE_ARRAY_WITH_KIND, GateFlags::NONE_FLAG, 1, 1, 3)             \
    V(LdLocalModuleVar, LD_LOCAL_MODULE_VAR, GateFlags::NONE_FLAG, 1, 1, 2)                     \
    V(LdExternalModuleVar, LD_EXTERNAL_MODULE_VAR, GateFlags::NONE_FLAG, 1, 1, 2)


#define HCR_GATE_META_DATA_LIST_WITH_VALUE_IN(V)                                                 \
    V(Call, CALL, GateFlags::NONE_FLAG, 0, 1, value)                                             \
    V(NoGcRuntimeCall, NOGC_RUNTIME_CALL, GateFlags::NO_WRITE, 0, 1, value)                      \
    V(BytecodeCall, BYTECODE_CALL, GateFlags::NONE_FLAG, 0, 1, value)                            \
    V(DebuggerBytecodeCall, DEBUGGER_BYTECODE_CALL, GateFlags::NONE_FLAG, 0, 1, value)           \
    V(BuiltinsCallWithArgv, BUILTINS_CALL_WITH_ARGV, GateFlags::NONE_FLAG, 0, 1, value)          \
    V(BuiltinsCall, BUILTINS_CALL, GateFlags::NONE_FLAG, 0, 1, value)                            \
    V(RuntimeCall, RUNTIME_CALL, GateFlags::NONE_FLAG, 0, 1, value)                              \
    V(RuntimeCallWithArgv, RUNTIME_CALL_WITH_ARGV, GateFlags::NONE_FLAG, 0, 1, value)            \
    V(SaveRegister, SAVE_REGISTER, GateFlags::NONE_FLAG, 0, 1, value)                            \
    V(ArrayConstructor, ARRAY_CONSTRUCTOR, GateFlags::NONE_FLAG, 1, 1, value)                    \
    V(ObjectConstructor, OBJECT_CONSTRUCTOR, GateFlags::NONE_FLAG, 1, 1, value)                  \
    V(BooleanConstructor, BOOLEAN_CONSTRUCTOR, GateFlags::NONE_FLAG, 1, 1, value)                \
    V(BaselineCall, BASELINE_CALL, GateFlags::NONE_FLAG, 0, 1, value)                            \
    V(ASMCallBarrier, ASM_CALL_BARRIER, GateFlags::NONE_FLAG, 0, 1, value)

#define HCR_GATE_META_DATA_LIST_WITH_PC_OFFSET(V)                              \
    V(Construct, CONSTRUCT, GateFlags::HAS_FRAME_STATE, 1, 1, value)           \
    V(CallInternal, CALLINTERNAL, GateFlags::HAS_FRAME_STATE, 1, 1, value)     \
    V(Float32ArrayConstructor, FLOAT32_ARRAY_CONSTRUCTOR, GateFlags::HAS_FRAME_STATE, 1, 1, value)     \

#define HCR_GATE_META_DATA_LIST_FOR_NEW(V)                                     \
    V(CallNew, CALL_NEW, GateFlags::HAS_FRAME_STATE, 1, 1, value)

#define HCR_GATE_META_DATA_LIST_WITH_PC_OFFSET_FIXED_VALUE(V)                                   \
    V(CallPrivateGetter, CALL_PRIVATE_GETTER, GateFlags::HAS_FRAME_STATE, 1, 1, 2)              \
    V(CallPrivateSetter, CALL_PRIVATE_SETTER, GateFlags::HAS_FRAME_STATE, 1, 1, 3)              \
    V(CallGetter, CALL_GETTER, GateFlags::HAS_FRAME_STATE, 1, 1, 3)                             \
    V(CallSetter, CALL_SETTER, GateFlags::HAS_FRAME_STATE, 1, 1, 4)                             \
    V(MonoCallGetterOnProto, MONO_CALL_GETTER_ON_PROTO, GateFlags::HAS_FRAME_STATE, 1, 1, 4)    \
    V(CreateArray, CREATE_ARRAY, GateFlags::NONE_FLAG, 1, 1, 1)                                 \
    V(CreateArrayWithBuffer, CREATE_ARRAY_WITH_BUFFER, GateFlags::CHECKABLE, 1, 1, 2)           \
    V(CreateArguments, CREATE_ARGUMENTS, GateFlags::HAS_FRAME_STATE, 1, 1, 1)

#define HCR_GATE_META_DATA_LIST_WITH_VALUE(V)                                           \
    V(RestoreRegister, RESTORE_REGISTER, GateFlags::NONE_FLAG, 0, 0, 1)                 \
    V(LoadHClass, LOAD_HCLASS, GateFlags::NO_WRITE, 0, 1, 2)

#define HCR_GATE_META_DATA_LIST_WITH_BOOL_VALUE_IN(V)                              \
    V(CallOptimized, CALL_OPTIMIZED, GateFlags::NONE_FLAG, 0, 1, value)            \
    V(FastCallOptimized, FAST_CALL_OPTIMIZED, GateFlags::NONE_FLAG, 0, 1, value)

#define HCR_GATE_META_DATA_LIST_WITH_SIZE(V)                                       \
    HCR_GATE_META_DATA_LIST_WITH_VALUE_IN(V)

#define HCR_GATE_META_DATA_LIST_WITH_ONE_PARAMETER(V)         \
    HCR_GATE_META_DATA_LIST_WITH_VALUE(V)

#define HCR_GATE_OPCODE_LIST(V)     \
    V(JS_BYTECODE)

}
#endif  // ECMASCRIPT_COMPILER_HCR_OPCODE_H
