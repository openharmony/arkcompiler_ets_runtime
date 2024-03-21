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

#include "ecmascript/compiler/combined_pass_visitor.h"
#include "ecmascript/compiler/pass_manager.h"

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
          isLiteCG_(ctx->GetEcmaVM()->GetJSOptions().IsCompilerEnableLiteCG()) {}
    ~TypedNativeInlineLowering() = default;
    GateRef VisitGate(GateRef gate) override;
private:
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
    GateRef BuildRounding(GateRef gate, GateRef value, OpCode op);
    void LowerTaggedRounding(GateRef gate);
    void LowerDoubleRounding(GateRef gate);

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
    void LowerMathImul(GateRef gate);
    void LowerGlobalIsFinite(GateRef gate);
    void LowerGlobalIsNan(GateRef gate);

    GateRef LowerGlobalDoubleIsFinite(GateRef value);
    GateRef LowerGlobalTNumberIsFinite(GateRef value);
    GateRef LowerGlobalTNumberIsNan(GateRef value);

    GateRef FindFrameState(GateRef gate);
private:
    Circuit* circuit_ {nullptr};
    GateAccessor acc_;
    CircuitBuilder builder_;
    bool isLiteCG_ {false};
};
}
#endif  // ECMASCRIPT_COMPILER_TYPED_HCR_LOWERING_H