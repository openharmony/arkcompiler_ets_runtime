/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_STRING_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BUILTINS_STRING_STUB_BUILDER_H
#include "ecmascript/compiler/stub_builder-inl.h"

namespace panda::ecmascript::kungfu {
class BuiltinsStringStubBuilder : public StubBuilder {
public:
    explicit BuiltinsStringStubBuilder(StubBuilder *parent)
        : StubBuilder(parent) {}
    ~BuiltinsStringStubBuilder() override = default;
    NO_MOVE_SEMANTIC(BuiltinsStringStubBuilder);
    NO_COPY_SEMANTIC(BuiltinsStringStubBuilder);
    void GenerateCircuit() override {}

    GateRef StringAt(GateRef obj, GateRef index);
    GateRef FastSubString(GateRef glue, GateRef thisValue, GateRef from, GateRef len);
    GateRef FastSubUtf8String(GateRef glue, GateRef thisValue, GateRef from, GateRef len);
    GateRef FastSubUtf16String(GateRef glue, GateRef thisValue, GateRef from, GateRef len);
    void CopyChars(GateRef glue, GateRef dst, GateRef source, GateRef sourceLength, GateRef size, VariableType type);
    void CopyUtf16AsUtf8(GateRef glue, GateRef src, GateRef dst, GateRef sourceLength);
    GateRef StringIndexOf(GateRef lhsData, bool lhsIsUtf8, GateRef rhsData, bool rhsIsUtf8,
                          GateRef pos, GateRef max, GateRef rhsCount);
    GateRef StringIndexOf(GateRef lhs, GateRef rhs, GateRef pos);
    GateRef CreateFromEcmaString(GateRef glue, GateRef obj, GateRef index);
private:
    GateRef CanBeCompressed(GateRef utf16Data, GateRef utf16Len, bool isUtf16);
    GateRef GetUtf16Data(GateRef stringData, GateRef index);
    GateRef IsASCIICharacter(GateRef data);
    GateRef GetUtf8Data(GateRef stringData, GateRef index);
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_STRING_STUB_BUILDER_H