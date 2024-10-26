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

#ifndef ECMASCRIPT_COMPILER_DEOPT_TYPE_H
#define ECMASCRIPT_COMPILER_DEOPT_TYPE_H

#include <cstdint>

namespace panda::ecmascript::kungfu {

#define GATE_META_DATA_DEOPT_REASON(V)                                 \
    V(NotInt1,                         NOTINT1)                        \
    V(NotInt2,                         NOTINT2)                        \
    V(NotInt3,                         NOTINT3)                        \
    V(NotInt4,                         NOTINT4)                        \
    V(NotInt5,                         NOTINT5)                        \
    V(NotInt6,                         NOTINT6)                        \
    V(NotInt7,                         NOTINT7)                        \
    V(NotDouble1,                      NOTDOUBLE1)                     \
    V(NotDouble2,                      NOTDOUBLE2)                     \
    V(NotDouble3,                      NOTDOUBLE3)                     \
    V(NotNumber1,                      NOTNUMBER1)                     \
    V(NotNumber2,                      NOTNUMBER2)                     \
    V(NotNumberOrString,               NOTNUMBERORSTRING)              \
    V(CannotStoreSpecailHole,          CANNOTSTORESPECAILHOLE)         \
    V(NotBool1,                        NOTBOOL1)                       \
    V(NotBool2,                        NOTBOOL2)                       \
    V(NotHeapObject1,                  NOTHEAPOBJECT1)                 \
    V(NotStableArray1,                 NOTSARRAY1)                     \
    V(NotStableArray2,                 NOTSARRAY2)                     \
    V(ElementsKindMismatchedAtLoad,    ELEMENSKINDMISMATCHEDATLOAD)    \
    V(ElementsKindMismatchedAtStore,   ELEMENSKINDMISMATCHEDATSTORE)   \
    V(InconsistentElementsKind,        INCONSISTENTELEMENTSKIND)       \
    V(NotArray1,                       NOTARRAY1)                      \
    V(NotArray2,                       NOTARRAY2)                      \
    V(InconsistentOnHeap1,             INCONSISTENTONHEAP1)            \
    V(InconsistentHClass1,             INCONSISTENTHCLASS1)            \
    V(InconsistentHClass2,             INCONSISTENTHCLASS2)            \
    V(InconsistentHClass3,             INCONSISTENTHCLASS3)            \
    V(InconsistentHClass4,             INCONSISTENTHCLASS4)            \
    V(InconsistentHClass5,             INCONSISTENTHCLASS5)            \
    V(InconsistentHClass6,             INCONSISTENTHCLASS6)            \
    V(InconsistentHClass7,             INCONSISTENTHCLASS7)            \
    V(InconsistentHClass8,             INCONSISTENTHCLASS8)            \
    V(InconsistentHClass9,             INCONSISTENTHCLASS9)            \
    V(InconsistentHClass10,            INCONSISTENTHCLASS10)           \
    V(InconsistentHClass11,            INCONSISTENTHCLASS11)           \
    V(InconsistentHClass12,            INCONSISTENTHCLASS12)           \
    V(InconsistentHClass13,            INCONSISTENTHCLASS13)           \
    V(InconsistentHClass14,            INCONSISTENTHCLASS14)           \
    V(NotEcmaObject1,                  NOTECMAOBJECT1)                 \
    V(NotNewObj1,                      NOTNEWOBJ1)                     \
    V(NotNewObj2,                      NOTNEWOBJ2)                     \
    V(NotNewObj3,                      NOTNEWOBJ3)                     \
    V(NotLegalIndex1,                  NOTLEGALIDX1)                   \
    V(NotNegativeOverflow1,            NOTNEGOV1)                      \
    V(NotCallTarget1,                  NOTCALLTGT1)                    \
    V(NotJSCallTarget1,                NOTJSCALLTGT1)                  \
    V(NotJSCallTarget2,                NOTJSCALLTGT2)                  \
    V(NotJSCallTarget3,                NOTJSCALLTGT3)                  \
    V(NotJSCallTarget4,                NOTJSCALLTGT4)                  \
    V(NotJSNewCallTarget,              NOTJSNEWCALLTGT)                \
    V(DivideZero1,                     DIVZERO1)                       \
    V(DivideZero2,                     DIVZERO2)                       \
    V(InlineFail1,                     INLINEFAIL1)                    \
    V(InlineFail2,                     INLINEFAIL2)                    \
    V(NotJSFastCallTarget1,            NOTJSFASTCALLTGT1)              \
    V(NotJSFastCallTarget2,            NOTJSFASTCALLTGT2)              \
    V(NotJSFastCallTarget3,            NOTJSFASTCALLTGT3)              \
    V(LexVarIsHole1,                   LEXVARISHOLE1)                  \
    V(ModZero1,                        MODZERO1)                       \
    V(RemainderIsNegativeZero,         REMAINDERISNEGATIVEZERO)        \
    V(Int32Overflow1,                  INT32OVERFLOW1)                 \
    V(NotString1,                      NOTSTRING1)                     \
    V(InconsistentType1,               INCONSISTENTTYPE1)              \
    V(NotNull1,                        NOTNULL1)                       \
    V(NotNull2,                        NOTNULL2)                       \
    V(BuiltinInstanceHClassMismatch,   BUILTININSTANCEHCLASSMISMATCH)  \
    V(BuiltinInstanceHClassMismatch2,  BUILTININSTANCEHCLASSMISMATCH2) \
    V(BuiltinPrototypeHClassMismatch1, BUILTINPROTOHCLASSMISMATCH1)    \
    V(BuiltinPrototypeHClassMismatch2, BUILTINPROTOHCLASSMISMATCH2)    \
    V(ProtoTypeChanged1,               PROTOTYPECHANGED1)              \
    V(ProtoTypeChanged2,               PROTOTYPECHANGED2)              \
    V(BuiltinIsHole1,                  BUILTINISHOLE1)                 \
    V(NewBuiltinCtorFail1,             NEWBUILTINCTORFAIL1)            \
    V(NewBuiltinCtorObject,            NEWBUILTINCTOROBJECT)           \
    V(NewBuiltinCtorArray,             NEWBUILTINCTORARRAY)            \
    V(NewBuiltinCtorFloat32Array,      NEWBUILTINCTORFLOAT32ARRAY)     \
    V(NewBuiltinCtorBoolean,           NEWBUILTINCTORBOOLEAN)          \
    V(IsUndefinedOrHole,               ISUNDEFINEDORHOLE)              \
    V(IsNotUndefinedOrHole,            ISNOTUNDEFINEDORHOLE)           \
    V(BuiltinInliningTypeGuard,        BUILTIN_INLINING_TYPE_GUARD)    \
    V(RangeError,                      RANGE_ERROR)                    \
    V(NotBigInt,                       NOT_BIG_INT)                    \
    V(OsrLoopExit,                     OSRLOOPEXIT)                    \
    V(IsNotMap,                        ISNOTMAP)                       \
    V(IsNotEcmaObject,                 ISNOTECMAOBJECT)                \
    V(IsNotDataView,                   ISNOTDATAVIEW)                  \
    V(IndexLessZero,                   INDEXLESSZERO)                  \
    V(ArrayBufferIsDetached,           ARRAYBUFFERISDETACHED)          \
    V(TotalSizeOverflow,               TOTALSIZEOVERFLOW)              \
    V(NotJSFunction,                   NOTJSFUNCTION)                  \
    V(NotSymbol,                       NOTSYMBOL)                      \
    V(IndexNotInt,                     INDEXNOTINT)                    \
    V(InitAotFailed,                   INIT_AOT_FAILED)                \
    V(NotEcmaObject,                   NOT_ECMA_OBJECT)                \
    V(NotCallable,                     NOTCALLABLE)                    \
    V(ArrayLengthOverMax,              ARRAYLENGTHOVERMAX)             \
    V(ArrayLengthNotWritable,          ARRAYLENGTHNOTWRITABLE)

enum class DeoptType : uint8_t {
    NONE = 0,
#define DECLARE_DEOPT_TYPE(NAME, TYPE) TYPE,
    GATE_META_DATA_DEOPT_REASON(DECLARE_DEOPT_TYPE)
#undef DECLARE_DEOPT_TYPE
};

} // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_DEOPT_TYPE_H
