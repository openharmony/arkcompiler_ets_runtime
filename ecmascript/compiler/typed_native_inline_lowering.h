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
namespace panda::ecmascript::kungfu {
class TypedNativeInlineLowering : public PassVisitor {
public:
    TypedNativeInlineLowering(Circuit* circuit,
                              RPOVisitor* visitor,
                              CompilationConfig* cmpCfg,
                              Chunk* chunk)
        : PassVisitor(circuit, chunk, visitor),
          circuit_(circuit),
          acc_(circuit),
          builder_(circuit, cmpCfg) {}
    ~TypedNativeInlineLowering() = default;
    GateRef VisitGate(GateRef gate) override;
private:
    enum class MathTrigonometricCheck : uint8_t {
        NOT_NAN,
        LT_ONE,
        ABS_GT_ONE
    };

    template <MathTrigonometricCheck CHECK = MathTrigonometricCheck::NOT_NAN>
    void LowerMathTrigonometric(GateRef gate, RuntimeStubCSigns::ID stub_id);
    void LowerMathAtan2(GateRef gate);

private:
    Circuit* circuit_ {nullptr};
    GateAccessor acc_;
    CircuitBuilder builder_;
};
}
#endif  // ECMASCRIPT_COMPILER_TYPED_HCR_LOWERING_H