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

#include "ecmascript/compiler/baseline/baseline_call_signature.h"

namespace panda::ecmascript::kungfu {

#define BINARY_OP_SIGNATURE_DESC(name)                                  \
    CallSignature signature(name, 0, 4,                                 \
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());         \
    *callSign = signature;                                              \
    std::array<VariableType, 4> params = {                              \
        VariableType::NATIVE_POINTER(),                                 \
        VariableType::NATIVE_POINTER(),                                 \
        VariableType::JS_ANY(),                                         \
        VariableType::INT32(),                                          \
    }

#define SINGLE_OP_SIGNATURE_DESC(name)                                  \
    CallSignature signature(name, 0, 3,                                 \
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());         \
    *callSign = signature;                                              \
    std::array<VariableType, 3> params = {                              \
        VariableType::NATIVE_POINTER(),                                 \
        VariableType::NATIVE_POINTER(),                                 \
        VariableType::INT32(),                                          \
    }

DEF_CALL_SIGNATURE(BaselineTryLdGLobalByNameImm8ID16)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineTryLdGLobalByNameImm8ID16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStToGlobalRecordImm16ID16)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineStToGlobalRecordImm16ID16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineLdaStrID16)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineLdaStrID16", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCallArg1Imm8V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineCallArg1Imm8V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineLdsymbol)
{
    // 1 : 1 input parameters
    CallSignature signature("BaselineLdsymbol", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 1 : 1 input parameters
    std::array<VariableType, 1> params = {
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineLdglobal)
{
    // 1 : 1 input parameters
    CallSignature signature("BaselineLdglobal", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 1 : 1 input parameters
    std::array<VariableType, 1> params = {
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselinePoplexenv)
{
    // 2 : 2 input parameters
    CallSignature signature("BaselinePoplexenv", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineGetunmappedargs)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineGetunmappedargs", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineAsyncfunctionenter)
{
    // 2 : 2 input parameters
    CallSignature signature("BaselineAsyncfunctionenter", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCreateasyncgeneratorobjV8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineCreateasyncgeneratorobjV8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineDebugger)
{
    // 1 : 1 input parameters
    CallSignature signature("BaselineDebugger", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 1 : 1 input parameters
    std::array<VariableType, 1> params = {
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineGetpropiterator)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineGetpropiterator", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineGetiteratorImm8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineGetiteratorImm8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineGetiteratorImm16)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineGetiteratorImm16", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCloseiteratorImm8V8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineCloseiteratorImm8V8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCloseiteratorImm16V8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineCloseiteratorImm16V8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, OFFSET, V0, V1, V2
DEF_CALL_SIGNATURE(BaselineAsyncgeneratorresolveV8V8V8)
{
    // 6 : 6 input parameters
    CallSignature signature("BaselineAsyncgeneratorresolveV8V8V8", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT8(),
        VariableType::INT8(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCreateemptyobject)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineCreateemptyobject", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCreateemptyarrayImm8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineCreateemptyarrayImm8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCreateemptyarrayImm16)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineCreateemptyarrayImm16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCreategeneratorobjV8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineCreategeneratorobjV8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}


DEF_CALL_SIGNATURE(BaselineCreateiterresultobjV8V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineCreateiterresultobjV8V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCreateobjectwithexcludedkeysImm8V8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineCreateobjectwithexcludedkeysImm8V8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT16(),
        VariableType::JS_ANY(),
        VariableType::INT16(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCallthis0Imm8V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineCallthis0Imm8V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCreatearraywithbufferImm8Id16)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineCreatearraywithbufferImm8Id16", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCreatearraywithbufferImm16Id16)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineCreatearraywithbufferImm16Id16", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCallthis1Imm8V8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineCallthis1Imm8V8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCallthis2Imm8V8V8V8)
{
    // 6 : 6 input parameters
    CallSignature signature("BaselineCallthis2Imm8V8V8V8", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCreateobjectwithbufferImm8Id16)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineCreateobjectwithbufferImm8Id16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCreateobjectwithbufferImm16Id16)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineCreateobjectwithbufferImm16Id16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCreateregexpwithliteralImm8Id16Imm8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineCreateregexpwithliteralImm8Id16Imm8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCreateregexpwithliteralImm16Id16Imm8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineCreateregexpwithliteralImm16Id16Imm8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineNewobjapplyImm8V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineNewobjapplyImm8V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineNewobjapplyImm16V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineNewobjapplyImm16V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineNewlexenvImm8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineNewlexenvImm8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineNewlexenvwithnameImm8Id16)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineNewlexenvwithnameImm8Id16", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT16(),
        VariableType::INT16(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineAdd2Imm8V8)
{
    BINARY_OP_SIGNATURE_DESC("BaselineAdd2Imm8V8");
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineSub2Imm8V8)
{
    BINARY_OP_SIGNATURE_DESC("BaselineSub2Imm8V8");
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineMul2Imm8V8)
{
    BINARY_OP_SIGNATURE_DESC("BaselineMul2Imm8V8");
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineDiv2Imm8V8)
{
    BINARY_OP_SIGNATURE_DESC("BaselineDiv2Imm8V8");
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineMod2Imm8V8)
{
    BINARY_OP_SIGNATURE_DESC("BaselineMod2Imm8V8");
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineEqImm8V8)
{
    BINARY_OP_SIGNATURE_DESC("BaselineEqImm8V8");
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineNoteqImm8V8)
{
    BINARY_OP_SIGNATURE_DESC("BaselineNoteqImm8V8");
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineLessImm8V8)
{
    BINARY_OP_SIGNATURE_DESC("BaselineLessImm8V8");
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineLesseqImm8V8)
{
    BINARY_OP_SIGNATURE_DESC("BaselineLesseqImm8V8");
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineGreaterImm8V8)
{
    BINARY_OP_SIGNATURE_DESC("BaselineGreaterImm8V8");
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineGreatereqImm8V8)
{
    BINARY_OP_SIGNATURE_DESC("BaselineGreatereqImm8V8");
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineShl2Imm8V8)
{
    BINARY_OP_SIGNATURE_DESC("BaselineShl2Imm8V8");
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineShr2Imm8V8)
{
    BINARY_OP_SIGNATURE_DESC("BaselineShr2Imm8V8");
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineAshr2Imm8V8)
{
    BINARY_OP_SIGNATURE_DESC("BaselineAshr2Imm8V8");
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineAnd2Imm8V8)
{
    BINARY_OP_SIGNATURE_DESC("BaselineAnd2Imm8V8");
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineOr2Imm8V8)
{
    BINARY_OP_SIGNATURE_DESC("BaselineOr2Imm8V8");
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineXor2Imm8V8)
{
    BINARY_OP_SIGNATURE_DESC("BaselineXor2Imm8V8");
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineExpImm8V8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineExpImm8V8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineTypeofImm8)
{
    // 2 : 2 input parameters
    CallSignature signature("BaselineTypeofImm8", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineTypeofImm16)
{
    // 2 : 2 input parameters
    CallSignature signature("BaselineTypeofImm16", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineTonumberImm8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineTonumberImm8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineTonumericImm8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineTonumericImm8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineNegImm8)
{
    SINGLE_OP_SIGNATURE_DESC("BaselineNegImm8");
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineNotImm8)
{
    SINGLE_OP_SIGNATURE_DESC("BaselineNotImm8");
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineIncImm8)
{
    SINGLE_OP_SIGNATURE_DESC("BaselineIncImm8");
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineDecImm8)
{
    SINGLE_OP_SIGNATURE_DESC("BaselineDecImm8");
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineIsinImm8V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineIsinImm8V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineInstanceofImm8V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineInstanceofImm8V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStrictnoteqImm8V8)
{
    BINARY_OP_SIGNATURE_DESC("BaselineStrictnoteqImm8V8");
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStricteqImm8V8)
{
    BINARY_OP_SIGNATURE_DESC("BaselineStricteqImm8V8");
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineIstrue)
{
    // 1 : 1 input parameters
    CallSignature signature("BaselineIstrue", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineIsfalse)
{
    // 2 : 2 input parameters
    CallSignature signature("BaselineIsfalse", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCallthis3Imm8V8V8V8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineCallthis3Imm8V8V8V8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCallthisrangeImm8Imm8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineCallthisrangeImm8Imm8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT8(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineSupercallthisrangeImm8Imm8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineSupercallthisrangeImm8Imm8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT16(),
        VariableType::INT16(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineSupercallarrowrangeImm8Imm8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineSupercallarrowrangeImm8Imm8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT8(),
        VariableType::INT16(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineDefinefuncImm8Id16Imm8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineDefinefuncImm8Id16Imm8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineDefinefuncImm16Id16Imm8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineDefinefuncImm16Id16Imm8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineDefinemethodImm8Id16Imm8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineDefinemethodImm8Id16Imm8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::INT8(),
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineDefinemethodImm16Id16Imm8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineDefinemethodImm16Id16Imm8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::INT8(),
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCallarg0Imm8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineCallarg0Imm8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineSupercallspreadImm8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineSupercallspreadImm8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCallRuntimeSupercallforwardallargsPrefV8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineCallRuntimeSupercallforwardallargsPrefV8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineApplyImm8V8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineApplyImm8V8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCallargs2Imm8V8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineCallargs2Imm8V8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCallargs3Imm8V8V8V8)
{
    // 6 : 6 input parameters
    CallSignature signature("BaselineCallargs3Imm8V8V8V8", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCallrangeImm8Imm8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineCallrangeImm8Imm8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineLdexternalmodulevarImm8)
{
    // 2 : 2 input parameters
    CallSignature signature("BaselineLdexternalmodulevarImm8", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineLdthisbynameImm8Id16)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineLdthisbynameImm8Id16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineDefinegettersetterbyvalueV8V8V8V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineDefinegettersetterbyvalueV8V8V8V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineLdthisbynameImm16Id16)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineLdthisbynameImm16Id16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStthisbynameImm8Id16)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineStthisbynameImm8Id16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStthisbynameImm16Id16)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineStthisbynameImm16Id16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineLdthisbyvalueImm8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineLdthisbyvalueImm8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineLdthisbyvalueImm16)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineLdthisbyvalueImm16", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStthisbyvalueImm8V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineStthisbyvalueImm8V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStthisbyvalueImm16V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineStthisbyvalueImm16V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineDynamicimport)
{
    // 2 : 2 input parameters
    CallSignature signature("BaselineDynamicimport", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineDefineclasswithbufferImm8Id16Id16Imm16V8)
{
    // 6 : 6 input parameters
    CallSignature signature("BaselineDefineclasswithbufferImm8Id16Id16Imm16V8", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT16(),
        VariableType::INT16(),
        VariableType::INT16(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineDefineclasswithbufferImm16Id16Id16Imm16V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineDefineclasswithbufferImm16Id16Id16Imm16V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineResumegenerator)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineResumegenerator", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineGetresumemod)
{
    // 2 : 2 input parameters
    CallSignature signature("BaselineGetresumemod", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineGettemplateobjectImm8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineGettemplateobjectImm8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineGettemplateobjectImm16)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineGettemplateobjectImm16", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineGetnextpropnameV8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineGetnextpropnameV8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineSetobjectwithprotoImm8V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineSetobjectwithprotoImm8V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineDelobjpropV8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineDelobjpropV8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineAsyncfunctionawaituncaughtV8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineAsyncfunctionawaituncaughtV8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCopydatapropertiesV8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineCopydatapropertiesV8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStarrayspreadV8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineStarrayspreadV8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineSetobjectwithprotoImm16V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineSetobjectwithprotoImm16V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineLdobjbyvalueImm8V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineLdobjbyvalueImm8V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineLdobjbyvalueImm16V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineLdobjbyvalueImm16V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStobjbyvalueImm8V8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineStobjbyvalueImm8V8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStobjbyvalueImm16V8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineStobjbyvalueImm16V8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStownbyvalueImm8V8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineStownbyvalueImm8V8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStownbyvalueImm16V8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineStownbyvalueImm16V8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineLdsuperbyvalueImm8V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineLdsuperbyvalueImm8V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineLdsuperbyvalueImm16V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineLdsuperbyvalueImm16V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStsuperbyvalueImm8V8V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineStsuperbyvalueImm8V8V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStsuperbyvalueImm16V8V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineStsuperbyvalueImm16V8V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineLdobjbyindexImm8Imm16)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineLdobjbyindexImm8Imm16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineLdobjbyindexImm16Imm16)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineLdobjbyindexImm16Imm16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStobjbyindexImm8V8Imm16)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineStobjbyindexImm8V8Imm16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStobjbyindexImm16V8Imm16)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineStobjbyindexImm16V8Imm16", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStownbyindexImm8V8Imm16)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineStownbyindexImm8V8Imm16", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStownbyindexImm16V8Imm16)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineStownbyindexImm16V8Imm16", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineAsyncfunctionresolveV8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineAsyncfunctionresolveV8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineAsyncfunctionrejectV8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineAsyncfunctionrejectV8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCopyrestargsImm8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineCopyrestargsImm8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineLdlexvarImm4Imm4)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineLdlexvarImm4Imm4", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStlexvarImm4Imm4)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineStlexvarImm4Imm4", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineGetmodulenamespaceImm8)
{
    // 2 : 2 input parameters
    CallSignature signature("BaselineGetmodulenamespaceImm8", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStmodulevarImm8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineStmodulevarImm8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineTrystglobalbynameImm8Id16)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineTrystglobalbynameImm8Id16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineTryldglobalbynameImm16Id16)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineTryldglobalbynameImm16Id16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineTrystglobalbynameImm16Id16)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineTrystglobalbynameImm16Id16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineLdglobalvarImm16Id16)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineLdglobalvarImm16Id16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStglobalvarImm16Id16)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineStglobalvarImm16Id16", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineLdobjbynameImm8Id16)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineLdobjbynameImm8Id16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineLdobjbynameImm16Id16)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineLdobjbynameImm16Id16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStobjbynameImm8Id16V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineStobjbynameImm8Id16V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStobjbynameImm16Id16V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineStobjbynameImm16Id16V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStownbynameImm8Id16V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineStownbynameImm8Id16V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStownbynameImm16Id16V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineStownbynameImm16Id16V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineLdsuperbynameImm8Id16)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineLdsuperbynameImm8Id16", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineLdsuperbynameImm16Id16)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineLdsuperbynameImm16Id16", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStsuperbynameImm8Id16V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineStsuperbynameImm8Id16V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStsuperbynameImm16Id16V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineStsuperbynameImm16Id16V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineLdlocalmodulevarImm8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineLdlocalmodulevarImm8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineStconsttoglobalrecordImm16Id16)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineStconsttoglobalrecordImm16Id16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, ACC, RECEIVER, PROP_KEY
DEF_CALL_SIGNATURE(BaselineStownbyvaluewithnamesetImm8V8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineStownbyvaluewithnamesetImm8V8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, ACC, RECEIVER, PROP_KEY
DEF_CALL_SIGNATURE(BaselineStownbyvaluewithnamesetImm16V8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineStownbyvaluewithnamesetImm16V8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, ACC, CONST_POOL, STRING_ID, RECEIVER
DEF_CALL_SIGNATURE(BaselineStownbynamewithnamesetImm8Id16V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineStownbynamewithnamesetImm8Id16V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, ACC, CONST_POOL, STRING_ID, RECEIVER
DEF_CALL_SIGNATURE(BaselineStownbynamewithnamesetImm16Id16V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineStownbynamewithnamesetImm16Id16V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, CONST_POOL, STRING_ID
DEF_CALL_SIGNATURE(BaselineLdbigintId16)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineLdbigintId16", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// IMM
DEF_CALL_SIGNATURE(BaselineFldaiImm64)
{
    // 1 : 1 input parameters
    CallSignature signature("BaselineFldaiImm64", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 1 : 1 input parameters
    std::array<VariableType, 1> params = {
        VariableType::INT64(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, ACC, SP, PC, CONST_POOL, PROFILE_TYPE_INFO, HOTNESS_COUNTER
DEF_CALL_SIGNATURE(BaselineReturn)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineReturn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}


// GLUE, LEVEL, SLOT, FRAME
DEF_CALL_SIGNATURE(BaselineLdlexvarImm8Imm8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineLdlexvarImm8Imm8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, ACC, LEVEL, SLOT, FRAME
DEF_CALL_SIGNATURE(BaselineStlexvarImm8Imm8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineStlexvarImm8Imm8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE
DEF_CALL_SIGNATURE(BaselineJnstricteqV8Imm16)
{
    // 1 : 1 input parameters
    CallSignature signature("BaselineJnstricteqV8Imm16", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 1 : 1 input parameters
    std::array<VariableType, 1> params = {
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, ACC, V0
DEF_CALL_SIGNATURE(BaselineAsyncgeneratorrejectV8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineAsyncgeneratorrejectV8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, ACC, INDEX
DEF_CALL_SIGNATURE(BaselineSetgeneratorstateImm8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineSetgeneratorstateImm8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, ACC
DEF_CALL_SIGNATURE(BaselineGetasynciteratorImm8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineGetasynciteratorImm8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

//  GLUE, SP, ACC, INDEX0, INDEX1, ENV
DEF_CALL_SIGNATURE(BaselineLdPrivatePropertyImm8Imm16Imm16)
{
    // 6 : 6 input parameters
    CallSignature signature("BaselineLdPrivatePropertyImm8Imm16Imm16", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::JS_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, INDEX0, INDEX1, INDEX2
DEF_CALL_SIGNATURE(BaselineStPrivatePropertyImm8Imm16Imm16V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineStPrivatePropertyImm8Imm16Imm16V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, ACC, INDEX0, INDEX1, ENV
DEF_CALL_SIGNATURE(BaselineTestInImm8Imm16Imm16)
{
    // 6 : 6 input parameters
    CallSignature signature("BaselineTestInImm8Imm16Imm16", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::JS_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP
DEF_CALL_SIGNATURE(BaselineDeprecatedLdlexenvPrefNone)
{
    // 2 : 2 input parameters
    CallSignature signature("BaselineDeprecatedLdlexenvPrefNone", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, V0, V1, V2
DEF_CALL_SIGNATURE(BaselineWideCreateobjectwithexcludedkeysPrefImm16V8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineWideCreateobjectwithexcludedkeysPrefImm16V8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT16(),
        VariableType::INT8(),
        VariableType::INT16(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, ACC
DEF_CALL_SIGNATURE(BaselineThrowPrefNone)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineThrowPrefNone", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP
DEF_CALL_SIGNATURE(BaselineDeprecatedPoplexenvPrefNone)
{
    // 2 : 2 input parameters
    CallSignature signature("BaselineDeprecatedPoplexenvPrefNone", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, NUM_ARGS, IDX, HOTNESS_COUNTER, SLOT_ID
DEF_CALL_SIGNATURE(BaselineWideNewobjrangePrefImm16V8)
{
    // 6 : 6 input parameters
    CallSignature signature("BaselineWideNewobjrangePrefImm16V8", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    /// 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT16(),
        VariableType::INT16(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP
DEF_CALL_SIGNATURE(BaselineThrowNotexistsPrefNone)
{
    // 2 : 2 input parameters
    CallSignature signature("BaselineThrowNotexistsPrefNone", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, V0, V1
DEF_CALL_SIGNATURE(BaselineDeprecatedGetiteratornextPrefV8V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineDeprecatedGetiteratornextPrefV8V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, ACC, NUM_VARS
DEF_CALL_SIGNATURE(BaselineWideNewlexenvPrefImm16)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineWideNewlexenvPrefImm16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP
DEF_CALL_SIGNATURE(BaselineThrowPatternnoncoerciblePrefNone)
{
    // 2 : 2 input parameters
    CallSignature signature("BaselineThrowPatternnoncoerciblePrefNone", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, IMM_I16, FUNC, SLOT_ID, PROFILE_TYPE_INFO, PC
DEF_CALL_SIGNATURE(BaselineDeprecatedCreatearraywithbufferPrefImm16)
{
    // 6 : 6 input parameters
    CallSignature signature("BaselineDeprecatedCreatearraywithbufferPrefImm16", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    /// 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT16(),
        VariableType::INT32(),
        VariableType::JS_POINTER(),
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, ACC, NUM_VARS, SCOPE_ID
DEF_CALL_SIGNATURE(BaselineWideNewlexenvwithnamePrefImm16Id16)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineWideNewlexenvwithnamePrefImm16Id16", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT16(),
        VariableType::INT16(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP
DEF_CALL_SIGNATURE(BaselineThrowDeletesuperpropertyPrefNone)
{
    // 2 : 2 input parameters
    CallSignature signature("BaselineThrowDeletesuperpropertyPrefNone", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, IMM_I16, CONST_POOL, FRAME
DEF_CALL_SIGNATURE(BaselineDeprecatedCreateobjectwithbufferPrefImm16)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineDeprecatedCreateobjectwithbufferPrefImm16", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::INT16(),
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineNewobjrangeImm8Imm8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineNewobjrangeImm8Imm8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineNewobjrangeImm16Imm8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineNewobjrangeImm16Imm8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT16(),
        VariableType::INT16(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, ACC, ACTUAL_NUM_ARGS, VREG_ID, HOTNESS_COUNTER
DEF_CALL_SIGNATURE(BaselineWideCallrangePrefImm16V8)
{
    // 6 : 6 input parameters
    CallSignature signature("BaselineWideCallrangePrefImm16V8", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::INT8(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, V0
DEF_CALL_SIGNATURE(BaselineThrowConstassignmentPrefV8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineThrowConstassignmentPrefV8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, V0
DEF_CALL_SIGNATURE(BaselineDeprecatedTonumberPrefV8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineDeprecatedTonumberPrefV8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, ACC, ACTUAL_NUM_ARGS, VREG_ID, HOTNESS_COUNTER
DEF_CALL_SIGNATURE(BaselineWideCallthisrangePrefImm16V8)
{
    // 6 : 6 input parameters
    CallSignature signature("BaselineWideCallthisrangePrefImm16V8", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::INT8(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, V0
DEF_CALL_SIGNATURE(BaselineThrowIfnotobjectPrefV8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineThrowIfnotobjectPrefV8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, V0
DEF_CALL_SIGNATURE(BaselineDeprecatedTonumericPrefV8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineDeprecatedTonumericPrefV8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}


// GLUE, SP, RANGE, V0
DEF_CALL_SIGNATURE(BaselineWideSupercallthisrangePrefImm16V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineWideSupercallthisrangePrefImm16V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT16(),
        VariableType::INT16(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, V0, V1
DEF_CALL_SIGNATURE(BaselineThrowUndefinedifholePrefV8V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineThrowUndefinedifholePrefV8V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, CONST_POOL, STRING_ID
DEF_CALL_SIGNATURE(BaselineThrowUndefinedifholewithnamePrefId16)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineThrowUndefinedifholewithnamePrefId16", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, V0
DEF_CALL_SIGNATURE(BaselineDeprecatedNegPrefV8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineDeprecatedNegPrefV8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, ACC, RANGE, V0
DEF_CALL_SIGNATURE(BaselineWideSupercallarrowrangePrefImm16V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineWideSupercallarrowrangePrefImm16V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT16(),
        VariableType::INT16(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, ACC, IMM
DEF_CALL_SIGNATURE(BaselineThrowIfsupernotcorrectcallPrefImm8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineThrowIfsupernotcorrectcallPrefImm8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, INDEX
DEF_CALL_SIGNATURE(BaselineDeprecatedNotPrefV8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineDeprecatedNotPrefV8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, ACC, INDEX
DEF_CALL_SIGNATURE(BaselineWideLdobjbyindexPrefImm32)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineWideLdobjbyindexPrefImm32", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, ACC, IMM
DEF_CALL_SIGNATURE(BaselineThrowIfsupernotcorrectcallPrefImm16)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineThrowIfsupernotcorrectcallPrefImm16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT16(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, V0
DEF_CALL_SIGNATURE(BaselineDeprecatedIncPrefV8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineDeprecatedIncPrefV8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, ACC, V0, INDEX
DEF_CALL_SIGNATURE(BaselineWideStobjbyindexPrefV8Imm32)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineWideStobjbyindexPrefV8Imm32", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, INDEX
DEF_CALL_SIGNATURE(BaselineDeprecatedDecPrefV8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineDeprecatedDecPrefV8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, ACC, V0, INDEX
DEF_CALL_SIGNATURE(BaselineWideStownbyindexPrefV8Imm32)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineWideStownbyindexPrefV8Imm32", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, FUNC_REG, HOTNESS_COUNTER
DEF_CALL_SIGNATURE(BaselineDeprecatedCallarg0PrefV8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineDeprecatedCallarg0PrefV8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, INDEX
DEF_CALL_SIGNATURE(BaselineWideCopyrestargsPrefImm16)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineWideCopyrestargsPrefImm16", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, FUNC_REG, A0, HOTNESS_COUNTER
DEF_CALL_SIGNATURE(BaselineDeprecatedCallarg1PrefV8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineDeprecatedCallarg1PrefV8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
        VariableType::INT8(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, LEVEL, SLOT
DEF_CALL_SIGNATURE(BaselineWideLdlexvarPrefImm16Imm16)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineWideLdlexvarPrefImm16Imm16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, FUNC_REG, A0, A1, HOTNESS_COUNTER
DEF_CALL_SIGNATURE(BaselineDeprecatedCallargs2PrefV8V8V8)
{
    // 6 : 6 input parameters
    CallSignature signature("BaselineDeprecatedCallargs2PrefV8V8V8", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
        VariableType::INT8(),
        VariableType::INT8(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, ACC, LEVEL, SLOT
DEF_CALL_SIGNATURE(BaselineWideStlexvarPrefImm16Imm16)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineWideStlexvarPrefImm16Imm16", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, FUNC_REG, A0, A1, A2
DEF_CALL_SIGNATURE(BaselineDeprecatedCallargs3PrefV8V8V8V8)
{
    // 6 : 6 input parameters
    CallSignature signature("BaselineDeprecatedCallargs3PrefV8V8V8V8", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
        VariableType::INT8(),
        VariableType::INT8(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, INDEX
DEF_CALL_SIGNATURE(BaselineWideGetmodulenamespacePrefImm16)
{
    // 2 : 2 input parameters
    CallSignature signature("BaselineWideGetmodulenamespacePrefImm16", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::INT16(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

//  GLUE, SP, INDEX, FUNC_REG, HOTNESS_COUNTER
DEF_CALL_SIGNATURE(BaselineDeprecatedCallrangePrefImm16V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineDeprecatedCallrangePrefImm16V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT8(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, ACC, INDEX
DEF_CALL_SIGNATURE(BaselineWideStmodulevarPrefImm16)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineWideStmodulevarPrefImm16", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT16(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, V0, V1, V2
DEF_CALL_SIGNATURE(BaselineDeprecatedCallspreadPrefV8V8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineDeprecatedCallspreadPrefV8V8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
        VariableType::INT8(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, INDEX
DEF_CALL_SIGNATURE(BaselineWideLdlocalmodulevarPrefImm16)
{
    // 2 : 2 input parameters
    CallSignature signature("BaselineWideLdlocalmodulevarPrefImm16", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::INT16(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, INDEX, FUNC_REG, HOTNESS_COUNTER
DEF_CALL_SIGNATURE(BaselineDeprecatedCallthisrangePrefImm16V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineDeprecatedCallthisrangePrefImm16V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT8(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, INDEX
DEF_CALL_SIGNATURE(BaselineWideLdexternalmodulevarPrefImm16)
{
    // 2 : 2 input parameters
    CallSignature signature("BaselineWideLdexternalmodulevarPrefImm16", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::INT16(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, METHOD_ID, LITERAL_ID, LENGTH, VREG_IDS
DEF_CALL_SIGNATURE(BaselineDeprecatedDefineclasswithbufferPrefId16Imm16Imm16V8V8)
{
    // 6 : 6 input parameters
    CallSignature signature("BaselineDeprecatedDefineclasswithbufferPrefId16Imm16Imm16V8V8", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, INDEX
DEF_CALL_SIGNATURE(BaselineWideLdpatchvarPrefImm16)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineWideLdpatchvarPrefImm16", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, ACC, V0, FUNC
DEF_CALL_SIGNATURE(BaselineDeprecatedResumegeneratorPrefV8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineDeprecatedResumegeneratorPrefV8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, ACC, INDEX
DEF_CALL_SIGNATURE(BaselineWideStpatchvarPrefImm16)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineWideStpatchvarPrefImm16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, ACC, V0
DEF_CALL_SIGNATURE(BaselineDeprecatedGetresumemodePrefV8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineDeprecatedGetresumemodePrefV8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, V0
DEF_CALL_SIGNATURE(BaselineDeprecatedGettemplateobjectPrefV8)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineDeprecatedGettemplateobjectPrefV8", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, V0, V1
DEF_CALL_SIGNATURE(BaselineDeprecatedDelobjpropPrefV8V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineDeprecatedDelobjpropPrefV8V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, PC, V0, V1
DEF_CALL_SIGNATURE(BaselineDeprecatedSuspendgeneratorPrefV8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineDeprecatedSuspendgeneratorPrefV8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT8(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, PC, V0, V1
DEF_CALL_SIGNATURE(BaselineSuspendgeneratorV8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineSuspendgeneratorV8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, V0, V1
DEF_CALL_SIGNATURE(BaselineDeprecatedAsyncfunctionawaituncaughtPrefV8V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineDeprecatedAsyncfunctionawaituncaughtPrefV8V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, V0, V1
DEF_CALL_SIGNATURE(BaselineDeprecatedCopydatapropertiesPrefV8V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineDeprecatedCopydatapropertiesPrefV8V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, ACC, V0, V1
DEF_CALL_SIGNATURE(BaselineDeprecatedSetobjectwithprotoPrefV8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineDeprecatedSetobjectwithprotoPrefV8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT8(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, V0, V1
DEF_CALL_SIGNATURE(BaselineDeprecatedLdobjbyvaluePrefV8V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineDeprecatedLdobjbyvaluePrefV8V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, V0, V1
DEF_CALL_SIGNATURE(BaselineDeprecatedLdsuperbyvaluePrefV8V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineDeprecatedLdsuperbyvaluePrefV8V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, V0, INDEX
DEF_CALL_SIGNATURE(BaselineDeprecatedLdobjbyindexPrefV8Imm32)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineDeprecatedLdobjbyindexPrefV8Imm32", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, V0, V1
DEF_CALL_SIGNATURE(BaselineDeprecatedAsyncfunctionresolvePrefV8V8V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineDeprecatedAsyncfunctionresolvePrefV8V8V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, V0, V1
DEF_CALL_SIGNATURE(BaselineDeprecatedAsyncfunctionrejectPrefV8V8V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineDeprecatedAsyncfunctionrejectPrefV8V8V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, LEVEL, SLOT, V0
DEF_CALL_SIGNATURE(BaselineDeprecatedStlexvarPrefImm4Imm4V8)
{
      // 5 : 5 input parameters
    CallSignature signature("BaselineDeprecatedStlexvarPrefImm4Imm4V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, LEVEL, SLOT, V0
DEF_CALL_SIGNATURE(BaselineDeprecatedStlexvarPrefImm8Imm8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineDeprecatedStlexvarPrefImm8Imm8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, LEVEL, SLOT, V0
DEF_CALL_SIGNATURE(BaselineDeprecatedStlexvarPrefImm16Imm16V8)
{
     // 5 : 5 input parameters
    CallSignature signature("BaselineDeprecatedStlexvarPrefImm16Imm16V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, STRING_ID, CONST_POOL
DEF_CALL_SIGNATURE(BaselineDeprecatedGetmodulenamespacePrefId32)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineDeprecatedGetmodulenamespacePrefId32", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, ACC, STRING_ID, CONST_POOL
DEF_CALL_SIGNATURE(BaselineDeprecatedStmodulevarPrefId32)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineDeprecatedStmodulevarPrefId32", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, V0, STRING_ID, CONST_POOL
DEF_CALL_SIGNATURE(BaselineDeprecatedLdobjbynamePrefId32V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineDeprecatedLdobjbynamePrefId32V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT8(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, STRING_ID, V0
DEF_CALL_SIGNATURE(BaselineDeprecatedLdsuperbynamePrefId32V8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineDeprecatedLdsuperbynamePrefId32V8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, STRING_ID, FLAG_I8, CONST_POOL
DEF_CALL_SIGNATURE(BaselineDeprecatedLdmodulevarPrefId32Imm8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineDeprecatedLdmodulevarPrefId32Imm8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT8(),
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, ACC, STRING_ID, CONST_POOL
DEF_CALL_SIGNATURE(BaselineDeprecatedStconsttoglobalrecordPrefId32)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineDeprecatedStconsttoglobalrecordPrefId32", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, ACC, STRING_ID, CONST_POOL
DEF_CALL_SIGNATURE(BaselineDeprecatedStlettoglobalrecordPrefId32)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineDeprecatedStlettoglobalrecordPrefId32", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, ACC, STRING_ID, CONST_POOL
DEF_CALL_SIGNATURE(BaselineDeprecatedStclasstoglobalrecordPrefId32)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineDeprecatedStclasstoglobalrecordPrefId32", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP
DEF_CALL_SIGNATURE(BaselineDeprecatedLdhomeobjectPrefNone)
{
    // 2 : 2 input parameters
    CallSignature signature("BaselineDeprecatedLdhomeobjectPrefNone", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, ACC, CONSTPOOL, IMM_I16, FUNC
DEF_CALL_SIGNATURE(BaselineDeprecatedCreateobjecthavingmethodPrefImm16)
{
     // 4 : 4 input parameters
    CallSignature signature("BaselineDeprecatedCreateobjecthavingmethodPrefImm16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT16(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

//  GLUE, ACC, VREG, FUNC
DEF_CALL_SIGNATURE(BaselineDeprecatedDynamicimportPrefV8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineDeprecatedDynamicimportPrefV8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, ACC, FUNC
DEF_CALL_SIGNATURE(BaselineCallRuntimeNotifyConcurrentResultPrefNone)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineCallRuntimeNotifyConcurrentResultPrefNone", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, SLOT_ID, STRING_ID, V0
DEF_CALL_SIGNATURE(BaselineDefineFieldByNameImm8Id16V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineDefineFieldByNameImm8Id16V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineDefinePropertyByNameImm8Id16V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineDefinePropertyByNameImm8Id16V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, ACC, V0, V1
DEF_CALL_SIGNATURE(BaselineCallRuntimeDefineFieldByValuePrefImm8V8V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineCallRuntimeDefineFieldByValuePrefImm8V8V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT8(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, ACC, INDEX, V0
DEF_CALL_SIGNATURE(BaselineCallRuntimeDefineFieldByIndexPrefImm8Imm32V8)
{
    // 5 : 5 input parameters
    CallSignature signature("BaselineCallRuntimeDefineFieldByIndexPrefImm8Imm32V8", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCallRuntimeToPropertyKeyPrefNone)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineCallRuntimeToPropertyKeyPrefNone", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, COUNT, LITERAL_ID
DEF_CALL_SIGNATURE(BaselineCallRuntimeCreatePrivatePropertyPrefImm16Id16)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineCallRuntimeCreatePrivatePropertyPrefImm16Id16", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, ACC, LEVEL_INDEX, SLOT_INDEX, V0
DEF_CALL_SIGNATURE(BaselineCallRuntimeDefinePrivatePropertyPrefImm8Imm16Imm16V8)
{
    // 6 : 6 input parameters
    CallSignature signature("BaselineCallRuntimeDefinePrivatePropertyPrefImm8Imm16Imm16V8", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

//  GLUE, SP, ACC, V0, HOTNESS_COUNTER, SLOT_ID
DEF_CALL_SIGNATURE(BaselineCallRuntimeCallInitPrefImm8V8)
{
    // 6 : 6 input parameters
    CallSignature signature("BaselineCallRuntimeCallInitPrefImm8V8", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT8(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, METHOD_ID, LITERAL_ID, LENGTH, V0
DEF_CALL_SIGNATURE(BaselineCallRuntimeDefineSendableClassPrefImm16Id16Id16Imm16V8)
{
    // 6 : 6 input parameters
    CallSignature signature("BaselineCallRuntimeDefineSendableClassPrefImm16Id16Id16Imm16V8", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT16(),
        VariableType::INT16(),
        VariableType::INT16(),
        VariableType::INT8(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCallRuntimeLdSendableClassPrefImm16)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineCallRuntimeLdSendableClassPrefImm16", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT16(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCallRuntimeIstruePrefImm8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineCallRuntimeIstruePrefImm8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

DEF_CALL_SIGNATURE(BaselineCallRuntimeIsfalsePrefImm8)
{
    // 4 : 4 input parameters
    CallSignature signature("BaselineCallRuntimeIsfalsePrefImm8", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, ACC, SP
DEF_CALL_SIGNATURE(BaselineReturnundefined)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineReturnundefined", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
    };

    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, ACC
DEF_CALL_SIGNATURE(BaselineExceptionHandler)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineExceptionHandler", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}

// GLUE, SP, OFFSET
DEF_CALL_SIGNATURE(BaselineUpdateHotness)
{
    // 3 : 3 input parameters
    CallSignature signature("BaselineUpdateHotness", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
    };
    BASELINE_STUB_CALL_SIGNATURE_COMMON_SET();
}
}  // namespace panda::ecmascript::kungfu
