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

#ifndef ECMASCRIPT_COMPILER_PASS_OPTIONS_H
#define ECMASCRIPT_COMPILER_PASS_OPTIONS_H

namespace panda::ecmascript::kungfu {
#define OPTION_LIST(V)                                                           \
    V(ArrayBoundsCheckElimination, true)                                         \
    V(TypeLowering, true)                                                        \
    V(EarlyElimination, true)                                                    \
    V(LaterElimination, true)                                                    \
    V(ValueNumbering, false)                                                     \
    V(OptInlining, false)                                                        \
    V(OptNoGCCall, false)                                                        \
    V(OptPGOType, false)                                                         \
    V(NoCheck, false)                                                            \
    V(OptString, true)                                                           \
    V(OptTrackField, false)                                                      \
    V(OptLoopPeeling, false)                                                     \
    V(OptLoopInvariantCodeMotion, false)                                         \
    V(CollectLiteralInfo, false)                                                 \
    V(OptConstantFolding, true)                                                  \
    V(LexenvSpecialization, false)                                               \
    V(InlineNative, false)                                                       \
    V(LoweringBuiltin, false)                                                    \
    V(FastModule, false)                                                         \
    V(OptBranchProfiling, true)                                                  \
    V(EscapeAnalysis, false)                                                     \
    V(InductionVariableAnalysis, false)                                          \
    V(VerifierPass, true)                                                        \
    V(MergePoly, true)

#define OPTION_BUILDER(NAME, DEFAULT)                                            \
    Builder &Enable##NAME(bool value) {                                          \
        options_.enable##NAME##_ = value;                                        \
        return *this;                                                            \
    }

#define OPTIONS(NAME, DEFAULT) bool                                              \
    enable##NAME##_{DEFAULT};

#define GET_OPTION(NAME, DEFAULT)                                                \
    bool Enable##NAME() const { return enable##NAME##_; }

#define SET_OPTION(NAME, DEFAULT)                                                \
    void Set##NAME(bool value) { enable##NAME##_ = value; }

class PassOptions {
public:
    PassOptions() = default;
    class Builder;
    OPTION_LIST(GET_OPTION)
    OPTION_LIST(SET_OPTION)

private:
    OPTION_LIST(OPTIONS)
};

class PassOptions::Builder {
public:
    OPTION_LIST(OPTION_BUILDER)
    PassOptions Build() { return options_; }

private:
    PassOptions options_{};
};
#undef OPTION_BUILDER
#undef GET_OPTION
#undef OPTIONS
#undef OPTION_LIST
} // namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_PASS_OPTIONS_H