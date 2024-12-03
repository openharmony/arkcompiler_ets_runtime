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

#ifndef ECMASCRIPT_COMPILER_TYPED_NATIVE_INLINE_LOWERING_H
#define ECMASCRIPT_COMPILER_TYPED_NATIVE_INLINE_LOWERING_H

#include <cstdint>
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/combined_pass_visitor.h"
#include "ecmascript/compiler/pass_manager.h"
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/compiler/variable_type.h"
namespace panda::ecmascript::kungfu {
class TypedNativeInlineLowering : public PassVisitor {
public:
    TypedNativeInlineLowering(Circuit* circuit,
                              RPOVisitor* visitor,
                              PassContext *ctx,
                              CompilationConfig* cmpCfg,
                              Chunk* chunk)
        : PassVisitor(circuit, chunk, visitor),
          circuit_(circuit),
          acc_(circuit),
          builder_(circuit, cmpCfg),
          compilationEnv_(ctx->GetCompilationEnv()),
          isLiteCG_(ctx->GetCompilationEnv()->GetJSOptions().IsCompilerEnableLiteCG()) {}
    ~TypedNativeInlineLowering() = default;
    GateRef VisitGate(GateRef gate) override;
private:
    enum class DataViewProtoFunc : uint8_t { GET = 0, SET = 1 };
    enum class ArrayFindVariant : uint8_t { FIND = 0, FINDINDEX = 1 };

    enum ElmentSize : uint32_t { BITS_8 = 1, BITS_16 = 2, BITS_32 = 4, BITS_64 = 8 };
    //TAR:targetKind only Int or Double, ARR:arrayElementsKind only Int or Number
    enum NumberCompareKind : uint8_t { NONE = 0, TARINT_ARRINT, TARINT_ARRNUM, TARDOU_ARRINT, TARDOU_ARRNUM };

    inline bool NeedRunNaNLoopCheck(NumberCompareKind kind, BuiltinsStubCSigns::ID callID)
    {
        return ((kind == TARDOU_ARRNUM || kind == NONE) && callID == BuiltinsStubCSigns::ID::ArrayIncludes);
    }

    void ReplaceGateWithPendingException(GateRef gate, GateRef glue, GateRef state, GateRef depend, GateRef value);
    void LowerGeneralUnaryMath(GateRef gate, RuntimeStubCSigns::ID stubId);
    void LowerMathAtan2(GateRef gate);
    void LowerTrunc(GateRef gate);
    template <bool IS_CEIL>
    void LowerMathCeilFloor(GateRef gate);
    template <bool IS_CEIL>
    void LowerMathCeilFloorWithIntrinsic(GateRef gate);
    template <bool IS_CEIL>
    void LowerMathCeilFloorWithRuntimeCall(GateRef gate);
    void LowerMathPow(GateRef gate);
    void LowerMathExp(GateRef gate);
    void LowerMathSignInt(GateRef gate);
    void LowerMathSignTagged(GateRef gate);
    void LowerClz32Float64(GateRef gate);
    void LowerClz32Int32(GateRef gate);
    void LowerMathSqrt(GateRef gate);
    void LowerNewNumber(GateRef gate);
    template <bool IS_UNSIGNED>
    void LowerBigIntAsIntN(GateRef gate);
    GateRef BuildRounding(GateRef gate, GateRef value, OpCode op);
    void LowerTaggedRounding(GateRef gate);
    void LowerDoubleRounding(GateRef gate);
    void LowerArrayBufferIsView(GateRef gate);
    void LowerDataViewProtoFunc(GateRef gate, DataViewProtoFunc proto);
    GateRef BuildDoubleIsFinite(GateRef value);
    void LowerNumberIsFinite(GateRef gate);
    GateRef BuildTaggedIsInteger(GateRef gate, GateRef value, bool safe);
    void LowerNumberIsInteger(GateRef gate, OpCode op);
    void LowerNumberIsNaN(GateRef gate);
    void LowerNumberParseFloat(GateRef gate);
    void LowerNumberParseInt(GateRef gate);
    void LowerDateGetTime(GateRef gate);
    void LowerBigIntConstructor(GateRef gate);
    GateRef BuildTaggedPointerOverflowInt32(GateRef value);
    void LowerStringCharCodeAt(GateRef gate);
    void LowerStringSubstring(GateRef gate);
    void LowerStringSubStr(GateRef gate);
    void LowerStringSlice(GateRef gate);
    template <bool IS_SIGNED>
    void LowerBigIntConstructorInt32(GateRef gate);
    GateRef BuiltinIdToSize(GateRef ID);
    GateRef GetValueFromBuffer(GateRef bufferIndex, GateRef dataPointer, GateRef isLittleEndian, GateRef builtinId);
    GateRef SetValueInBuffer(GateRef bufferIndex,
                             GateRef value,
                             GateRef dataPointer,
                             GateRef isLittleEndian,
                             GateRef builtinId,
                             GateRef glue);

    GateRef BuildIntAbs(GateRef value);
    GateRef BuildDoubleAbs(GateRef value);
    GateRef BuildTNumberAbs(GateRef param);
    void LowerAbs(GateRef gate);
    void LowerIntAbs(GateRef gate);
    void LowerDoubleAbs(GateRef gate);

    template<bool IS_MAX>
    GateRef BuildIntMinMax(GateRef int1, GateRef int2, GateRef in1, GateRef in2);
    template<bool IS_MAX>
    GateRef BuildIntMinMax(GateRef in1, GateRef in2);
    template<bool IS_MAX>
    GateRef BuildDoubleMinMax(GateRef double1, GateRef double2, GateRef in1, GateRef in2);
    template<bool IS_MAX>
    GateRef BuildDoubleMinMax(GateRef in1, GateRef in2);
    template<bool IS_MAX>
    void LowerTNumberMinMax(GateRef gate);
    template<bool IS_MAX>
    void LowerMathMinMaxWithIntrinsic(GateRef gate);
    template<bool IS_MAX>
    void LowerMinMax(GateRef gate);
    template<bool IS_MAX>
    void LowerIntMinMax(GateRef gate);
    template<bool IS_MAX>
    void LowerDoubleMinMax(GateRef gate);
    GateRef NumberToInt32(GateRef gate);
    void LowerMathImul(GateRef gate);
    void LowerGlobalIsFinite(GateRef gate);
    void LowerGlobalIsNan(GateRef gate);
    void LowerGeneralWithoutArgs(GateRef gate, RuntimeStubCSigns::ID stubId);
    GateRef AllocateArrayIterator(GateRef glue, GateRef self, GateRef iteratorHClass, IterationKind iterationKind);
    void LowerTypedArrayIterator(GateRef gate, CommonStubCSigns::ID index, IterationKind iterationKind);

    GateRef LowerGlobalDoubleIsFinite(GateRef value);
    GateRef LowerGlobalTNumberIsFinite(GateRef value);
    GateRef LowerGlobalTNumberIsNan(GateRef value);

    void LowerObjectIs(GateRef gate);
    void LowerObjectGetPrototypeOf(GateRef gate);
    void LowerObjectCreate(GateRef gate);
    void LowerObjectIsPrototypeOf(GateRef gate);
    void LowerObjectHasOwnProperty(GateRef gate);
    void LowerReflectGetPrototypeOf(GateRef gate);
    void LowerReflectGet(GateRef gate);
    void LowerReflectHas(GateRef gate);
    void LowerReflectConstruct(GateRef gate);
    void LowerReflectApply(GateRef gate);
    void LowerFunctionPrototypeApply(GateRef gate);
    void LowerFunctionPrototypeBind(GateRef gate);
    void LowerFunctionPrototypeCall(GateRef gate);
    void LowerArraySort(GateRef gate);

    void LowerToCommonStub(GateRef gate, CommonStubCSigns::ID id);
    void LowerToBuiltinStub(GateRef gate, BuiltinsStubCSigns::ID id);

    GateRef FindFrameState(GateRef gate);
    void LowerArrayIncludesIndexOf(GateRef gate);
    GateRef NormalCompareLoop(GateRef elements, GateRef fromIndex, GateRef targetElement, GateRef arrayLength,
                              BuiltinsStubCSigns::ID callID, GateRef gate);
    GateRef IncludesUndefinedLoop(GateRef elements, GateRef fromIndex, GateRef arrayLength);
    GateRef TargetIntCompareLoop(GateRef elements,
                                 GateRef fromIndex,
                                 GateRef targetElement,
                                 GateRef arrayLength,
                                 NumberCompareKind kind,
                                 bool arrayHasHole);
    GateRef TargetIntCompareWithCompareKind(GateRef targetElement,
                                        GateRef doubleTarget,
                                        GateRef value,
                                        NumberCompareKind kind);
    GateRef TargetNumberCompareWithArrKind(GateRef doubleTarget, GateRef value, NumberCompareKind kind);
    GateRef TargetNumberCompareLoop(GateRef elements,
                                    GateRef fromIndex,
                                    GateRef targetElement,
                                    GateRef arrayLength,
                                    BuiltinsStubCSigns::ID callID,
                                    bool arrayHasHole,
                                    NumberCompareKind kind);
    GateRef TargetBigIntCompareLopp(GateRef elements, GateRef fromIndex, GateRef targetElement, GateRef arrayLength);
    GateRef TargetStringCompareLoop(
        GateRef elements, GateRef fromIndex, GateRef targetElement, GateRef arrayLength, bool arrayHasHole);
    GateRef TargetEqualCompareLoop(
        GateRef elements, GateRef fromIndex, GateRef targetElement, GateRef arrayLength, bool arrayHasHole);
    void LowerArrayIteratorBuiltin(GateRef gate);
    IterationKind GetArrayIterKindFromBuilin(BuiltinsStubCSigns::ID callID);
    void LowerArrayForEach(GateRef gate);
    void LowerArrayFindOrFindIndex(GateRef gate);
    void LowerArrayFilter(GateRef gate);
    void LowerArrayMap(GateRef gate);
    void LowerArraySome(GateRef gate);
    void LowerArrayEvery(GateRef gate);
    void LowerArrayPop(GateRef gate);
    void LowerArrayPush(GateRef gate);
    void LowerArraySlice(GateRef gate);
    void CheckAndCalcuSliceIndex(GateRef length,
                                 GateRef startHandler,
                                 GateRef endHandler,
                                 Label* exit,
                                 Label* checkIndexDone,
                                 Variable* res,
                                 Variable* start,
                                 Variable* end);

private:
    Circuit* circuit_ {nullptr};
    GateAccessor acc_;
    CircuitBuilder builder_;
    const CompilationEnv *compilationEnv_ {nullptr};
    bool isLiteCG_ {false};
};
}
#endif  // ECMASCRIPT_COMPILER_TYPED_HCR_LOWERING_H
