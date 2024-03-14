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

#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/compiler/variable_type.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include "llvm-c/Core.h"
#include "llvm/Support/Host.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace panda::ecmascript::kungfu {
#define BINARY_CALL_SIGNATURE(name)                             \
    /* 3 : 3 input parameters */                                \
    CallSignature signature(#name, 0, 3,                        \
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY()); \
    *callSign = signature;                                      \
    /* 3 : 3 input parameters */                                \
    std::array<VariableType, 3> params = {                      \
        VariableType::NATIVE_POINTER(),                         \
        VariableType::JS_ANY(),                                 \
        VariableType::JS_ANY(),                                 \
    };                                                          \
    callSign->SetParameters(params.data());                     \
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);

DEF_CALL_SIGNATURE(Add)
{
    BINARY_CALL_SIGNATURE(Add)
}

DEF_CALL_SIGNATURE(Sub)
{
    BINARY_CALL_SIGNATURE(Sub)
}

DEF_CALL_SIGNATURE(Mul)
{
    BINARY_CALL_SIGNATURE(Mul)
}

DEF_CALL_SIGNATURE(Div)
{
    BINARY_CALL_SIGNATURE(Div)
}

DEF_CALL_SIGNATURE(Mod)
{
    BINARY_CALL_SIGNATURE(Mod)
}

DEF_CALL_SIGNATURE(Equal)
{
    BINARY_CALL_SIGNATURE(Equal)
}

DEF_CALL_SIGNATURE(NotEqual)
{
    BINARY_CALL_SIGNATURE(NotEqual)
}

DEF_CALL_SIGNATURE(StrictEqual)
{
    BINARY_CALL_SIGNATURE(StrictEqual)
}

DEF_CALL_SIGNATURE(StrictNotEqual)
{
    BINARY_CALL_SIGNATURE(StrictNotEqual)
}

DEF_CALL_SIGNATURE(Less)
{
    BINARY_CALL_SIGNATURE(Less)
}

DEF_CALL_SIGNATURE(LessEq)
{
    BINARY_CALL_SIGNATURE(LessEq)
}

DEF_CALL_SIGNATURE(Greater)
{
    BINARY_CALL_SIGNATURE(Greater)
}

DEF_CALL_SIGNATURE(GreaterEq)
{
    BINARY_CALL_SIGNATURE(GreaterEq)
}

DEF_CALL_SIGNATURE(Shl)
{
    BINARY_CALL_SIGNATURE(Shl)
}

DEF_CALL_SIGNATURE(Shr)
{
    BINARY_CALL_SIGNATURE(Shr)
}

DEF_CALL_SIGNATURE(Ashr)
{
    BINARY_CALL_SIGNATURE(Ashr)
}

DEF_CALL_SIGNATURE(And)
{
    BINARY_CALL_SIGNATURE(And)
}

DEF_CALL_SIGNATURE(Or)
{
    BINARY_CALL_SIGNATURE(Or)
}

DEF_CALL_SIGNATURE(Xor)
{
    BINARY_CALL_SIGNATURE(Xor)
}

#ifndef NDEBUG
DEF_CALL_SIGNATURE(MulGCTest)
{
    // 3 : 3 input parameters
    CallSignature MulGC("MulGCTest", 0, 3, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = MulGC;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::INT64(),
        VariableType::INT64(),
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}
#else
DEF_CALL_SIGNATURE(MulGCTest) {}
#endif

#define UNARY_CALL_SIGNATURE(name)                              \
    /* 2 : 2 input parameters */                                \
    CallSignature signature(#name, 0, 2,                        \
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY()); \
    *callSign = signature;                                      \
    /* 2 : 2 input parameters */                                \
    std::array<VariableType, 2> params = {                      \
        VariableType::NATIVE_POINTER(),                         \
        VariableType::JS_ANY(),                                 \
    };                                                          \
    callSign->SetParameters(params.data());                     \
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);

DEF_CALL_SIGNATURE(Inc)
{
    UNARY_CALL_SIGNATURE(Inc)
}

DEF_CALL_SIGNATURE(Dec)
{
    UNARY_CALL_SIGNATURE(Dec)
}

DEF_CALL_SIGNATURE(Neg)
{
    UNARY_CALL_SIGNATURE(Neg)
}

DEF_CALL_SIGNATURE(Not)
{
    UNARY_CALL_SIGNATURE(Not)
}

DEF_CALL_SIGNATURE(ToBooleanTrue)
{
    UNARY_CALL_SIGNATURE(ToBooleanTrue)
}

DEF_CALL_SIGNATURE(ToBooleanFalse)
{
    UNARY_CALL_SIGNATURE(ToBooleanFalse)
}

DEF_CALL_SIGNATURE(TypeOf)
{
    // 2 input parameters
    CallSignature TypeOf("TypeOf", 0, 2, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_POINTER());
    *callSign = TypeOf;
    // 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(), // glue
        VariableType::JS_ANY(), // ACC
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(SetPropertyByName)
{
    // 6 : 6 input parameters
    CallSignature setPropertyByName("SetPropertyByName", 0, 6, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = setPropertyByName;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::NATIVE_POINTER(),   // glue
        VariableType::JS_ANY(),           // receiver
        VariableType::INT64(),            // key
        VariableType::JS_ANY(),           // value
        VariableType::JS_ANY(),           // jsFunc
        VariableType::INT32(),            // slot id
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(DeprecatedSetPropertyByName)
{
    // 4 : 4 input parameters
    CallSignature setPropertyByName("DeprecatedSetPropertyByName", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::JS_ANY());
    *callSign = setPropertyByName;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY()
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(SetPropertyByNameWithOwn)
{
    // 4 : 4 input parameters
    CallSignature setPropertyByNameWithOwn("SetPropertyByNameWithOwn", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::JS_ANY());
    *callSign = setPropertyByNameWithOwn;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY()
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(SetPropertyByValue)
{
    // 6 : 6 input parameters
    CallSignature setPropertyByName("SetPropertyByValue", 0, 6, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::JS_ANY());
    *callSign = setPropertyByName;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::NATIVE_POINTER(),    // glue
        VariableType::JS_POINTER(),        // receiver
        VariableType::JS_ANY(),            // key
        VariableType::JS_ANY(),            // value
        VariableType::JS_ANY(),            // jsFunc
        VariableType::INT32(),             // slot id
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(DeprecatedSetPropertyByValue)
{
    // 4 : 4 input parameters
    CallSignature setPropertyByName("DeprecatedSetPropertyByValue", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::JS_ANY());
    *callSign = setPropertyByName;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY()
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(SetPropertyByValueWithOwn)
{
    // 4 : 4 input parameters
    CallSignature setPropertyByValueWithOwn("SetPropertyByValueWithOwn", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::JS_ANY());
    *callSign = setPropertyByValueWithOwn;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY()
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(GetPropertyByName)
{
    // 5 : 5 input parameters
    CallSignature getPropertyByName("GetPropertyByName", 0, 5, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = getPropertyByName;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),   // glue
        VariableType::JS_ANY(),           // receiver
        VariableType::INT64(),            // key
        VariableType::JS_ANY(),           // jsFunc
        VariableType::INT32(),            // slot id
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(Instanceof)
{
    // 5 : 5 input parameters
    CallSignature instanceof("Instanceof", 0, 5, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = instanceof;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),   // glue
        VariableType::JS_ANY(),           // object
        VariableType::JS_ANY(),           // target
        VariableType::JS_ANY(),           // jsFunc
        VariableType::INT32(),            // slot id
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(DeprecatedGetPropertyByName)
{
    // 3 : 3 input parameters
    CallSignature getPropertyByName("DeprecatedGetPropertyByName", 0, 3, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::JS_ANY());
    *callSign = getPropertyByName;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(), // glue
        VariableType::JS_ANY(),         // receiver
        VariableType::JS_POINTER(),     // key
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(TryLdGlobalByName)
{
    // 4 : 4 input parameters
    CallSignature signature("TryLdGlobalByName", 0, 4, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),   // glue
        VariableType::INT64(),            // key
        VariableType::JS_ANY(),           // jsFunc
        VariableType::INT32(),            // slot id
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(TryStGlobalByName)
{
    // 5 : 5 input parameters
    CallSignature signature("TryStGlobalByName", 0, 5, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),   // glue
        VariableType::INT64(),            // key
        VariableType::JS_ANY(),           // value
        VariableType::JS_ANY(),           // jsFunc
        VariableType::INT32(),            // slot id
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(LdGlobalVar)
{
    // 4 : 4 input parameters
    CallSignature signature("LdGlobalVar", 0, 4, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),   // glue
        VariableType::INT64(),            // key
        VariableType::JS_ANY(),           // jsFunc
        VariableType::INT32(),            // slot id
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(StGlobalVar)
{
    // 5 : 5 input parameters
    CallSignature signature("StGlobalVar", 0, 5, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),   // glue
        VariableType::INT64(),            // string id
        VariableType::JS_ANY(),           // value
        VariableType::JS_ANY(),           // jsFunc
        VariableType::INT32(),            // slot id
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}
DEF_CALL_SIGNATURE(StOwnByValue)
{
    // 4 : 4 input parameters
    CallSignature signature("StOwnByValue", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY()
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}
DEF_CALL_SIGNATURE(StOwnByIndex)
{
    // 4 : 4 input parameters
    CallSignature signature("StOwnByIndex", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::JS_ANY()); // hole or undefined
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::INT32(),
        VariableType::JS_ANY()
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}
DEF_CALL_SIGNATURE(StOwnByName)
{
    // 4 : 4 input parameters
    CallSignature signature("StOwnByName", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY()
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}
DEF_CALL_SIGNATURE(StOwnByValueWithNameSet)
{
    // 4 : 4 input parameters
    CallSignature signature("StOwnByValueWithNameSet", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY()
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}
DEF_CALL_SIGNATURE(StOwnByNameWithNameSet)
{
    // 4 : 4 input parameters
    CallSignature signature("StOwnByNameWithNameSet", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY()
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}
DEF_CALL_SIGNATURE(StObjByIndex)
{
    // 4 : 4 input parameters
    CallSignature signature("StObjByIndex", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::JS_ANY()); // hole or undefined
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::INT32(),
        VariableType::JS_ANY()
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}
DEF_CALL_SIGNATURE(LdObjByIndex)
{
    // 3 : 3 input parameters
    CallSignature signature("LdObjByIndex", 0, 3, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(), // glue
        VariableType::JS_ANY(), // receiver
        VariableType::INT32(), // index
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(GetPropertyByIndex)
{
    // 3 : 3 input parameters
    CallSignature getPropertyByIndex("GetPropertyByIndex", 0, 3, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::JS_ANY());
    *callSign = getPropertyByIndex;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(), // glue
        VariableType::JS_ANY(), // receiver
        VariableType::INT32(), // index
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(SetPropertyByIndex)
{
    // 4 : 4 input parameters
    CallSignature setPropertyByIndex("SetPropertyByIndex", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::JS_ANY()); // hole or undefined
    *callSign = setPropertyByIndex;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::INT32(),
        VariableType::JS_ANY()
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(SetPropertyByIndexWithOwn)
{
    // 4 : 4 input parameters
    CallSignature setPropertyByIndexWithOwn("SetPropertyByIndexWithOwn", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::JS_ANY()); // hole or undefined
    *callSign = setPropertyByIndexWithOwn;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::INT32(),
        VariableType::JS_ANY()
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(GetPropertyByValue)
{
    // 5 : 5 input parameters
    CallSignature getPropertyByValue("GetPropertyByValue", 0, 5, ArgumentsOrder::DEFAULT_ORDER,
                                      VariableType::JS_ANY());
    *callSign = getPropertyByValue;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),   // glue
        VariableType::JS_POINTER(),       // receiver
        VariableType::JS_ANY(),           // key
        VariableType::JS_ANY(),           // jsFunc
        VariableType::INT32(),            // slot id
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(DeprecatedGetPropertyByValue)
{
    // 3 : 3 input parameters
    CallSignature getPropertyByValue("DeprecatedGetPropertyByValue", 0, 3, ArgumentsOrder::DEFAULT_ORDER,
                                      VariableType::JS_ANY());
    *callSign = getPropertyByValue;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(TryLoadICByName)
{
    // 4 : 4 input parameters
    CallSignature tryLoadICByName("TryLoadICByName", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = tryLoadICByName;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(TryLoadICByValue)
{
    // 5 : 5 input parameters
    CallSignature tryLoadICByValue("TryLoadICByValue", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = tryLoadICByValue;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(TryStoreICByName)
{
    // 5 : 5 input parameters
    CallSignature tryStoreICByName("TryStoreICByName", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY()); // undefined or hole
    *callSign = tryStoreICByName;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_POINTER(),
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(TryStoreICByValue)
{
    // 6 : 6 input parameters
    CallSignature tryStoreICByValue("TryStoreICByValue", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY()); // undefined or hole
    *callSign = tryStoreICByValue;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_POINTER(),
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(SetValueWithBarrier)
{
    // 4 : 4 input parameters
    CallSignature setValueWithBarrier("SetValueWithBarrier", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::VOID());
    *callSign = setValueWithBarrier;

    std::array<VariableType, 4> params = { // 4 : 4 input parameters
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY()
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(NewThisObjectChecked)
{
    // 2 : 2 input parameters
    CallSignature signature("NewThisObjectChecked", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::JS_ANY(),          // ctor
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(ConstructorCheck)
{
    // 4 : 4 input parameters
    CallSignature signature("ConstructorCheck", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::JS_ANY(),          // ctor
        VariableType::JS_ANY(),          // result
        VariableType::JS_ANY(),          // thisObj
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(CreateEmptyArray)
{
    // 5 : 5 input parameters
    CallSignature signature("CreateEmptyArray", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::JS_ANY(),          // jsFunc
        VariableType::JS_ANY(),          // pc
        VariableType::INT32(),           // profileTypeInfo
        VariableType::INT32(),           // slotId
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(CreateArrayWithBuffer)
{
    // 6 : 6 input parameters
    CallSignature signature("CreateArrayWithBuffer", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::INT32(),           // index
        VariableType::JS_ANY(),          // jsFunc
        VariableType::JS_ANY(),          // pc
        VariableType::INT32(),           // profileTypeInfo
        VariableType::INT32(),           // slotId
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(CopyRestArgs)
{
    // 3 : 3 input parameters
    CallSignature signature("CopyRestArgs", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::INT32(),           // startIdx
        VariableType::INT32(),           // numArgs
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(NewJSObject)
{
    // 2 : 2 input parameters
    CallSignature signature("NewJSObject", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::JS_ANY(),          // hclass
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(NewLexicalEnv)
{
    // 3 : 3 input parameters
    CallSignature signature("NewLexicalEnv", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::JS_ANY(),          // parent
        VariableType::INT32(),           // numArgs
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(GetUnmapedArgs)
{
    // 2 : 2 input parameters
    CallSignature signature("GetUnmapedArgs", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::INT32(),           // numArgs
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(GetTaggedArrayPtrTest)
{
    // 2 : 2 input parameters
    CallSignature getTaggedArrayPtr("GetTaggedArrayPtrTest", 0, 2, ArgumentsOrder::DEFAULT_ORDER,
                                     VariableType::JS_POINTER());
    *callSign = getTaggedArrayPtr;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB);
}

DEF_CALL_SIGNATURE(Builtins)
{
    // 9 : 9 input parameters
    CallSignature builtins("Builtins", 0, 9,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = builtins;
    std::array<VariableType, 9> params = { // 9 : 9 input parameters
        VariableType::NATIVE_POINTER(),    // glue
        VariableType::NATIVE_POINTER(),    // native code
        VariableType::JS_ANY(),            // func
        VariableType::JS_ANY(),            // new target
        VariableType::JS_ANY(),            // this
        VariableType::NATIVE_POINTER(),    // argc
        VariableType::JS_ANY(),            // arg0
        VariableType::JS_ANY(),            // arg1
        VariableType::JS_ANY(),            // arg2
    };
    callSign->SetVariadicArgs(true);
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::BUILTINS_STUB);
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(BuiltinsWithArgv)
{
    // 7 : 7 input parameters
    CallSignature builtinsWtihArgv("Builtins", 0, 7,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = builtinsWtihArgv;
    std::array<VariableType, 7> params = { // 7 : 7 input parameters
        VariableType::NATIVE_POINTER(),    // glue
        VariableType::NATIVE_POINTER(),    // nativeCode
        VariableType::JS_ANY(),            // func
        VariableType::JS_ANY(),            // new target
        VariableType::JS_ANY(),            // this
        VariableType::NATIVE_POINTER(),    // argc
        VariableType::NATIVE_POINTER(),    // argv
    };
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::BUILTINS_WITH_ARGV_STUB);
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(BytecodeHandler)
{
    // 7 : 7 input parameters
    CallSignature bytecodeHandler("BytecodeHandler", 0, 7,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = bytecodeHandler;
    // 7 : 7 input parameters
    std::array<VariableType, 7> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::BYTECODE_HANDLER);
    callSign->SetCallConv(CallSignature::CallConv::GHCCallConv);
}

DEF_CALL_SIGNATURE(BytecodeDebuggerHandler)
{
    // 7 : 7 input parameters
    CallSignature bytecodeHandler("BytecodeDebuggerHandler", 0, 7,
                                  ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = bytecodeHandler;
    // 7 : 7 input parameters
    std::array<VariableType, 7> params = { VariableType::NATIVE_POINTER(),
                                           VariableType::NATIVE_POINTER(),
                                           VariableType::NATIVE_POINTER(),
                                           VariableType::JS_POINTER(),
                                           VariableType::JS_POINTER(),
                                           VariableType::JS_ANY(),
                                           VariableType::INT32() };
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::BYTECODE_DEBUGGER_HANDLER);
}

DEF_CALL_SIGNATURE(CallRuntime)
{
    /* 3 : 3 input parameters */
    CallSignature runtimeCallTrampoline("CallRuntime", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = runtimeCallTrampoline;
    /* 3 : 3 input parameters */
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::INT64(),
        VariableType::INT64(),
    };
    callSign->SetVariadicArgs(true);
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB);
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(AsmInterpreterEntry)
{
    /* 3 : 3 input parameters */
    CallSignature asmInterpreterEntry("AsmInterpreterEntry", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = asmInterpreterEntry;
    /* 3 : 3 input parameters */
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::INT32(),  // argc
        VariableType::NATIVE_POINTER(),  // argv
    };
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(GeneratorReEnterAsmInterp)
{
    /* 2 : 2 input parameters */
    CallSignature generatorReEnterAsmInterp("GeneratorReEnterAsmInterp", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = generatorReEnterAsmInterp;
    std::array<VariableType, 2> params = { /* 2 : 2 input parameters */
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::JS_POINTER(),      // context
    };
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(CallRuntimeWithArgv)
{
    /* 4 : 4 input parameters */
    CallSignature runtimeCallTrampoline("CallRuntimeWithArgv", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = runtimeCallTrampoline;
    std::array<VariableType, 4> params = { /* 4 : 4 input parameters */
        VariableType::NATIVE_POINTER(), // glue
        VariableType::INT64(),   // runtimeId
        VariableType::INT64(),   // argc
        VariableType::NATIVE_POINTER(), // argv
    };
    callSign->SetVariadicArgs(false);
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_VARARGS);
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(OptimizedCallAndPushUndefined)
{
    /* 5 : 5 input parameters */
    CallSignature optimizedCallAndPushUndefined("OptimizedCallAndPushUndefined", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = optimizedCallAndPushUndefined;
    std::array<VariableType, 5> params = { /* 5 : 5 input parameters */
        VariableType::NATIVE_POINTER(),     // glue
        VariableType::INT64(),       // actual argC
        VariableType::JS_ANY(),      // call target
        VariableType::JS_ANY(),      // new target
        VariableType::JS_ANY(),      // thisobj
    };
    callSign->SetVariadicArgs(true);
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
    callSign->SetCallConv(CallSignature::CallConv::WebKitJSCallConv);
}

DEF_CALL_SIGNATURE(OptimizedFastCallAndPushUndefined)
{
    /* 5 : 5 input parameters */
    CallSignature optimizedFastCallAndPushUndefined("OptimizedFastCallAndPushUndefined", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = optimizedFastCallAndPushUndefined;
    std::array<VariableType, 5> params = { /* 5 : 5 input parameters */
        VariableType::NATIVE_POINTER(),     // glue
        VariableType::INT64(),       // actual argC
        VariableType::JS_ANY(),      // call target
        VariableType::JS_ANY(),      // new target
        VariableType::JS_ANY(),      // thisobj
    };
    callSign->SetVariadicArgs(true);
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(JSCall)
{
    // 6 : 6 input parameters
    CallSignature jSCall("JSCall", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = jSCall;
    std::array<VariableType, 5> params = { // 5 : 5 input parameters
        VariableType::NATIVE_POINTER(),     // glue
        VariableType::INT64(),       // actual argC
        VariableType::JS_ANY(),      // call target
        VariableType::JS_ANY(),      // new target
        VariableType::JS_ANY(),      // thisobj
    };
    callSign->SetVariadicArgs(true);
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::WebKitJSCallConv);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(JSOptimizedCall)
{
    // 6 : 6 input parameters
    CallSignature jSCall("JSOptimizedCall", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = jSCall;
    std::array<VariableType, 5> params = { // 5 : 5 input parameters
        VariableType::NATIVE_POINTER(),     // glue
        VariableType::INT64(),       // actual argC
        VariableType::JS_ANY(),      // call target
        VariableType::JS_ANY(),      // new target
        VariableType::JS_ANY(),      // thisobj
    };
    callSign->SetVariadicArgs(true);
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::WebKitJSCallConv);
    callSign->SetTargetKind(CallSignature::TargetKind::OPTIMIZED_STUB);
}

DEF_CALL_SIGNATURE(JSOptimizedFastCall)
{
    // 3 : 3 input parameters
    CallSignature jSCall("JSOptimizedFastCall", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = jSCall;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::NATIVE_POINTER(),     // glue
        VariableType::JS_ANY(),      // call target
        VariableType::JS_ANY(),      // thisobj
    };
    callSign->SetVariadicArgs(true);
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
    callSign->SetTargetKind(CallSignature::TargetKind::OPTIMIZED_FAST_CALL_STUB);
}

DEF_CALL_SIGNATURE(JSCallNew)
{
    // 6 : 6 input parameters
    CallSignature jSCallNew("JSCallNew", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = jSCallNew;
    std::array<VariableType, 5> params = { // 5 : 5 input parameters
        VariableType::NATIVE_POINTER(),     // glue
        VariableType::INT64(),       // actual argC
        VariableType::JS_ANY(),      // call target
        VariableType::JS_ANY(),      // new target
        VariableType::JS_ANY(),      // thisobj
    };
    callSign->SetVariadicArgs(true);
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::WebKitJSCallConv);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(JSProxyCallInternalWithArgV)
{
    // 2 : 2 input parameters
    CallSignature jSProxyCallInternalWithArgV("JSProxyCallInternalWithArgV", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = jSProxyCallInternalWithArgV;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::NATIVE_POINTER(),     // glue
        VariableType::JS_ANY(),      // call target
    };
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
    callSign->SetGCLeafFunction(true);
    callSign->SetTailCall(true);
}

DEF_CALL_SIGNATURE(JSFunctionEntry)
{
    // 5 : 5 input parameters
    CallSignature jsCallFunctionEntry("JSFunctionEntry", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = jsCallFunctionEntry;
    std::array<VariableType, 5> params = {  // 5 : 5 input parameters
        VariableType::NATIVE_POINTER(),     // glue
        VariableType::INT64(),              // argc
        VariableType::NATIVE_POINTER(),     // argv
        VariableType::NATIVE_POINTER(),     // prevFp
        VariableType::BOOL(),               // isNew
    };
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(OptimizedFastCallEntry)
{
    // 4 : 4 input parameters
    CallSignature optimizedFastCallEntry("OptimizedFastCallEntry", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = optimizedFastCallEntry;
    std::array<VariableType, 4> params = {  // 4 : 4 input parameters
        VariableType::NATIVE_POINTER(),     // glue
        VariableType::INT64(),              // argc
        VariableType::NATIVE_POINTER(),     // argv
        VariableType::NATIVE_POINTER(),     // prevFp
    };
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(ResumeRspAndDispatch)
{
    // 8 : 8 input parameters
    CallSignature resumeRspAndDispatch("ResumeRspAndDispatch", 0, 8,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = resumeRspAndDispatch;
    std::array<VariableType, 8> params = { // 8 : 8 input parameters
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::NATIVE_POINTER(),
    };
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
    callSign->SetCallConv(CallSignature::CallConv::GHCCallConv);
}

DEF_CALL_SIGNATURE(ResumeRspAndReturn)
{
    // 3 : 3 input parameters
    CallSignature resumeRspAndReturn("ResumeRspAndReturn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = resumeRspAndReturn;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::JS_ANY(),
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
    };
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
    callSign->SetCallConv(CallSignature::CallConv::GHCCallConv);
}

DEF_CALL_SIGNATURE(ResumeCaughtFrameAndDispatch)
{
    // 7 : 7 input parameters
    CallSignature resumeCaughtFrameAndDispatch("ResumeCaughtFrameAndDispatch", 0, 7,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = resumeCaughtFrameAndDispatch;
    // 7 : 7 input parameters
    std::array<VariableType, 7> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
    callSign->SetCallConv(CallSignature::CallConv::GHCCallConv);
}

DEF_CALL_SIGNATURE(ResumeUncaughtFrameAndReturn)
{
    // 3 : 3 input parameters
    CallSignature resumeUncaughtFrameAndReturn("ResumeUncaughtFrameAndReturn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = resumeUncaughtFrameAndReturn;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
    };
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
    callSign->SetCallConv(CallSignature::CallConv::GHCCallConv);
}

DEF_CALL_SIGNATURE(ResumeRspAndRollback)
{
    // 8 : 8 input parameters
    CallSignature resumeRspAndRollback("ResumeRspAndRollback", 0, 8,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = resumeRspAndRollback;
    std::array<VariableType, 8> params = { // 8 : 8 input parameters
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::NATIVE_POINTER(),
    };
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
    callSign->SetCallConv(CallSignature::CallConv::GHCCallConv);
}

DEF_CALL_SIGNATURE(StringsAreEquals)
{
    // 2 : 2 input parameters
    CallSignature stringsAreEquals("StringsAreEquals", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::BOOL());
    *callSign = stringsAreEquals;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(JSHClassFindProtoTransitions)
{
    // 3 : 3 input parameters
    CallSignature bigIntSameValueZero("JSHClassFindProtoTransitions", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = bigIntSameValueZero;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(NumberHelperStringToDouble)
{
    // 1 : 1 input parameters
    CallSignature bigIntSameValueZero("NumberHelperStringToDouble", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = bigIntSameValueZero;
    std::array<VariableType, 1> params = { // 1 : 1 input parameters
        VariableType::JS_POINTER(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(GetStringToListCacheArray)
{
    // 1 : 1 input parameters
    CallSignature callSignature("GetStringToListCacheArray", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = callSignature;
    std::array<VariableType, 1> params = { // 1 : 1 input parameters
        VariableType::NATIVE_POINTER(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(BigIntEquals)
{
    // 2 : 2 input parameters
    CallSignature bigIntEquals("BigIntEquals", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::BOOL());
    *callSign = bigIntEquals;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(FastArraySort)
{
    // 2 : 2 input parameters
    CallSignature fastArraySort("FastArraySort", 0, 2, ArgumentsOrder::DEFAULT_ORDER, VariableType::INT32());
    *callSign = fastArraySort;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::JS_ANY(),
        VariableType::JS_ANY()
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(LocaleCompareNoGc)
{
    // 4 : 4 input parameters
    CallSignature localeCompareNoGc("LocaleCompareNoGc", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = localeCompareNoGc;
    std::array<VariableType, 4> params = { // 4 : 4 input parameters
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(ArrayTrim)
{
    // 3 : 3 input parameters
    CallSignature ArrayTrim("ArrayTrim", 0, 3, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = ArrayTrim;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::INT64()
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(BigIntSameValueZero)
{
    // 1 : 1 input parameters
    CallSignature bigIntSameValueZero("BigIntSameValueZero", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::BOOL());
    *callSign = bigIntSameValueZero;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(StringGetStart)
{
    CallSignature stringGetStart("StringGetStart", 0, 4, ArgumentsOrder::DEFAULT_ORDER, VariableType::INT32());
    *callSign = stringGetStart;
    std::array<VariableType, 4> params = { // 4 : four input parameters
        VariableType::BOOL(),
        VariableType::JS_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(StringGetEnd)
{
    CallSignature stringGetEnd("StringGetEnd", 0, 5, ArgumentsOrder::DEFAULT_ORDER, VariableType::INT32());
    *callSign = stringGetEnd;
    std::array<VariableType, 5> params = { // 5 : five input parameters
        VariableType::BOOL(),
        VariableType::JS_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

#define PUSH_CALL_ARGS_AND_DISPATCH_SIGNATURE_COMMON(name)                  \
    /* 1 : 1 input parameters */                                            \
    CallSignature signature(#name, 0, 1,                                    \
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());             \
    *callSign = signature;                                                  \
    std::array<VariableType, 1> params = { /* 1: 1 input parameters */      \
        VariableType::NATIVE_POINTER(),                                     \
    };                                                                      \
    callSign->SetVariadicArgs(true);                                        \
    callSign->SetParameters(params.data());                                 \
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);

#define PUSH_CALL_ARGS_AND_DISPATCH_SIGNATURE(name)                      \
    PUSH_CALL_ARGS_AND_DISPATCH_SIGNATURE_COMMON(name)                   \
    callSign->SetCallConv(CallSignature::CallConv::GHCCallConv);

#define PUSH_CALL_ARGS_AND_DISPATCH_NATIVE_SIGNATURE(name)               \
    PUSH_CALL_ARGS_AND_DISPATCH_SIGNATURE_COMMON(name)                   \
    callSign->SetCallConv(CallSignature::CallConv::WebKitJSCallConv);

#define PUSH_CALL_ARGS_AND_DISPATCH_NATIVE_RANGE_SIGNATURE(name)         \
    PUSH_CALL_ARGS_AND_DISPATCH_SIGNATURE_COMMON(name)                   \
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);

DEF_CALL_SIGNATURE(PushCallArgsAndDispatchNative)
{
    PUSH_CALL_ARGS_AND_DISPATCH_NATIVE_SIGNATURE(PushCallArgsAndDispatchNative)
}

DEF_CALL_SIGNATURE(PushCallArg0AndDispatch)
{
    PUSH_CALL_ARGS_AND_DISPATCH_SIGNATURE(PushCallArg0AndDispatch)
}

DEF_CALL_SIGNATURE(PushCallArg1AndDispatch)
{
    PUSH_CALL_ARGS_AND_DISPATCH_SIGNATURE(PushCallArg1AndDispatch)
}

DEF_CALL_SIGNATURE(PushCallArgs2AndDispatch)
{
    PUSH_CALL_ARGS_AND_DISPATCH_SIGNATURE(PushCallArgs2AndDispatch)
}

DEF_CALL_SIGNATURE(PushCallArgs3AndDispatch)
{
    PUSH_CALL_ARGS_AND_DISPATCH_SIGNATURE(PushCallArgs3AndDispatch)
}

DEF_CALL_SIGNATURE(PushCallThisArg0AndDispatch)
{
    PUSH_CALL_ARGS_AND_DISPATCH_SIGNATURE(PushCallThisArg0AndDispatch)
}

DEF_CALL_SIGNATURE(PushCallThisArg1AndDispatch)
{
    PUSH_CALL_ARGS_AND_DISPATCH_SIGNATURE(PushCallThisArg1AndDispatch)
}

DEF_CALL_SIGNATURE(PushCallThisArgs2AndDispatch)
{
    PUSH_CALL_ARGS_AND_DISPATCH_SIGNATURE(PushCallThisArgs2AndDispatch)
}

DEF_CALL_SIGNATURE(PushCallThisArgs3AndDispatch)
{
    PUSH_CALL_ARGS_AND_DISPATCH_SIGNATURE(PushCallThisArgs3AndDispatch)
}

DEF_CALL_SIGNATURE(PushCallRangeAndDispatchNative)
{
    PUSH_CALL_ARGS_AND_DISPATCH_NATIVE_RANGE_SIGNATURE(PushCallRangeAndDispatchNative)
}

DEF_CALL_SIGNATURE(PushCallNewAndDispatchNative)
{
    PUSH_CALL_ARGS_AND_DISPATCH_NATIVE_RANGE_SIGNATURE(PushCallNewAndDispatchNative)
}

DEF_CALL_SIGNATURE(PushNewTargetAndDispatchNative)
{
    PUSH_CALL_ARGS_AND_DISPATCH_NATIVE_RANGE_SIGNATURE(PushNewTargetAndDispatchNative)
}

DEF_CALL_SIGNATURE(PushCallNewAndDispatch)
{
    PUSH_CALL_ARGS_AND_DISPATCH_SIGNATURE(PushCallNewAndDispatch)
}

DEF_CALL_SIGNATURE(PushSuperCallAndDispatch)
{
    PUSH_CALL_ARGS_AND_DISPATCH_SIGNATURE(PushSuperCallAndDispatch)
}

DEF_CALL_SIGNATURE(PushCallRangeAndDispatch)
{
    PUSH_CALL_ARGS_AND_DISPATCH_SIGNATURE(PushCallRangeAndDispatch)
}

DEF_CALL_SIGNATURE(PushCallThisRangeAndDispatch)
{
    PUSH_CALL_ARGS_AND_DISPATCH_SIGNATURE(PushCallThisRangeAndDispatch)
}

DEF_CALL_SIGNATURE(CallGetter)
{
    PUSH_CALL_ARGS_AND_DISPATCH_NATIVE_RANGE_SIGNATURE(CallGetter)
}

DEF_CALL_SIGNATURE(CallSetter)
{
    PUSH_CALL_ARGS_AND_DISPATCH_NATIVE_RANGE_SIGNATURE(CallSetter)
}

DEF_CALL_SIGNATURE(CallContainersArgs3)
{
    PUSH_CALL_ARGS_AND_DISPATCH_NATIVE_RANGE_SIGNATURE(CallContainersArgs3)
}

DEF_CALL_SIGNATURE(CallReturnWithArgv)
{
    PUSH_CALL_ARGS_AND_DISPATCH_NATIVE_RANGE_SIGNATURE(CallReturnWithArgv)
}

DEF_CALL_SIGNATURE(JSCallWithArgV)
{
    // 5 : 5 input parameters
    CallSignature jSCallWithArgV("JSCallWithArgV", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = jSCallWithArgV;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),   // glue
        VariableType::INT64(),            // actualNumArgs
        VariableType::JS_ANY(),           // jsfunc
        VariableType::JS_ANY(),           // newTarget
        VariableType::JS_ANY(),           // this
    };
    callSign->SetVariadicArgs(true);
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(JSFastCallWithArgV)
{
    // 4 : 4 input parameters
    CallSignature jSFastCallWithArgV("JSFastCallWithArgV", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = jSFastCallWithArgV;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),   // glue
        VariableType::JS_ANY(),           // jsfunc
        VariableType::JS_ANY(),           // this
        VariableType::INT64(),            // actualNumArgs
    };
    callSign->SetVariadicArgs(true);
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(JSFastCallWithArgVAndPushUndefined)
{
    // 4 : 4 input parameters
    CallSignature jSCallWithArgV("JSFastCallWithArgVAndPushUndefined", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = jSCallWithArgV;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),   // glue
        VariableType::JS_ANY(),           // jsfunc
        VariableType::JS_ANY(),           // this
        VariableType::INT64(),            // actualNumArgs
    };
    callSign->SetVariadicArgs(true);
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(JSCallWithArgVAndPushUndefined)
{
    // 5 : 5 input parameters
    CallSignature jSCallWithArgVAndPushUndefined("JSCallWithArgVAndPushUndefined", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = jSCallWithArgVAndPushUndefined;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),   // glue
        VariableType::INT64(),            // actualNumArgs
        VariableType::JS_ANY(),           // jsfunc
        VariableType::JS_ANY(),           // newTarget
        VariableType::JS_ANY(),           // this
    };
    callSign->SetVariadicArgs(true);
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(CallOptimized)
{
    // 5 : 5 input parameters
    CallSignature jSCall("CallOptimized", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = jSCall;
    std::array<VariableType, 5> params = { // 5 : 5 input parameters
        VariableType::NATIVE_POINTER(),     // glue
        VariableType::INT64(),       // actual argC
        VariableType::JS_ANY(),      // call target
        VariableType::JS_ANY(),      // new target
        VariableType::JS_ANY(),      // thisobj
    };
    callSign->SetVariadicArgs(true);
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::WebKitJSCallConv);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(Dump)
{
    constexpr size_t N_INPUT_PARAMETERS = 1;
    CallSignature dump("Dump", 0, N_INPUT_PARAMETERS,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = dump;
    std::array<VariableType, N_INPUT_PARAMETERS> params = {
        VariableType::JS_POINTER() // Tagged value of the object to be dumped
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(DebugDump)
{
    constexpr size_t N_INPUT_PARAMETERS = 1;
    CallSignature debugDump("DebugDump", 0, N_INPUT_PARAMETERS,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = debugDump;
    std::array<VariableType, N_INPUT_PARAMETERS> params = {
        VariableType::JS_POINTER() // Tagged value of the object to be dumped
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(DumpWithHint)
{
    constexpr size_t N_INPUT_PARAMETERS = 2;
    CallSignature dumpWithHint("DumpWithHint", 0, N_INPUT_PARAMETERS,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = dumpWithHint;
    std::array<VariableType, N_INPUT_PARAMETERS> params = {
        VariableType::NATIVE_POINTER(), // String created via CircuitBuilder::StringPtr()
        VariableType::JS_POINTER()      // Tagged value of the object to be dumped
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(DebugDumpWithHint)
{
    constexpr size_t N_INPUT_PARAMETERS = 2;
    CallSignature debugDumpWithHint("DebugDumpWithHint", 0, N_INPUT_PARAMETERS,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = debugDumpWithHint;
    std::array<VariableType, N_INPUT_PARAMETERS> params = {
        VariableType::NATIVE_POINTER(), // String created via CircuitBuilder::StringPtr()
        VariableType::JS_POINTER()      // Tagged value of the object to be dumped
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(DebugPrint)
{
    // 1 : 1 input parameters
    CallSignature debugPrint("DebugPrint", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = debugPrint;
    // 1 : 1 input parameters
    std::array<VariableType, 1> params = {
        VariableType::INT32(),
    };
    callSign->SetVariadicArgs(true);
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(DebugPrintCustom)
{
    // 1 : 1 input parameters
    CallSignature debugPrintCustom("DebugPrintCustom", 0,  1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = debugPrintCustom;
    // 1 : 1 input parameters
    std::array<VariableType, 1> params = {
        VariableType::NATIVE_POINTER() // Format string created via CircuitBuilder::StringPtr()
    };
    callSign->SetVariadicArgs(true);
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(DebugPrintInstruction)
{
    // 2 : 2 input parameters
    CallSignature debugPrintInstruction("DebugPrintInstruction", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = debugPrintInstruction;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
    };
    callSign->SetVariadicArgs(true);
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(Comment)
{
    // 1 : 1 input parameters
    CallSignature comment("Comment", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = comment;
    // 1 : 1 input parameters
    std::array<VariableType, 1> params = {
        VariableType::NATIVE_POINTER(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(FatalPrint)
{
    // 1 : 1 input parameters
    CallSignature fatalPrint("FatalPrint", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = fatalPrint;
    // 1 : 1 input parameters
    std::array<VariableType, 1> params = {
        VariableType::INT32(),
    };
    callSign->SetVariadicArgs(true);
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(FatalPrintCustom)
{
    // 1 : 1 input parameters
    CallSignature fatalPrintCustom("FatalPrintCustom", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = fatalPrintCustom;
    // 1 : 1 input parameters
    std::array<VariableType, 1> params = {
        VariableType::NATIVE_POINTER() // Format string created via CircuitBuilder::StringPtr()
    };
    callSign->SetVariadicArgs(true);
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(GetActualArgvNoGC)
{
    CallSignature index("GetActualArgvNoGC", 0, 1, ArgumentsOrder::DEFAULT_ORDER, VariableType::NATIVE_POINTER());
    *callSign = index;
    std::array<VariableType, 1> params = {
        VariableType::NATIVE_POINTER(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(InsertOldToNewRSet)
{
    // 3 : 3 input parameters
    CallSignature index("InsertOldToNewRSet", 0, 3, ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = index;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::NATIVE_POINTER(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(InsertLocalToShareRSet)
{
    // 3 : 3 input parameters
    CallSignature index("InsertLocalToShareRSet", 0, 3, ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = index;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::NATIVE_POINTER(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

#define DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME(NAME)                                               \
    DEF_CALL_SIGNATURE(NAME)                                                                       \
    {                                                                                              \
        /* 1 : 1 input parameters */                                                               \
        CallSignature index(#NAME, 0, 1, ArgumentsOrder::DEFAULT_ORDER, VariableType::FLOAT64());  \
        *callSign = index;                                                                         \
        /* 1 : 1 input parameters */                                                               \
        std::array<VariableType, 1> params = {                                                     \
            VariableType::FLOAT64(),                                                               \
        };                                                                                         \
        callSign->SetParameters(params.data());                                                    \
        callSign->SetGCLeafFunction(true);                                                         \
        callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);                    \
    }

DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME(FloatAcos)
DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME(FloatAcosh)
DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME(FloatAsin)
DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME(FloatAsinh)
DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME(FloatAtan)
DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME(FloatAtanh)
DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME(FloatCos)
DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME(FloatCosh)
DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME(FloatSin)
DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME(FloatSinh)
DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME(FloatTan)
DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME(FloatTanh)
DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME(FloatExp)
DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME(FloatExpm1)
DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME(FloatTrunc)
DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME(FloatFloor)
DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME(FloatLog)
DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME(FloatLog2)
DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME(FloatLog10)
DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME(FloatLog1p)
DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME(FloatCbrt)
DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME(FloatClz32)
DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME(FloatCeil)

#undef DEF_FLOAT_UNARY_CALL_SIGNATURE_BY_NAME

#define DEF_FLOAT_BINARY_CALL_SIGNATURE_BY_NAME(NAME)                                              \
    DEF_CALL_SIGNATURE(NAME)                                                                       \
    {                                                                                              \
        /* 2 : 2 input parameters */                                                               \
        CallSignature index(#NAME, 0, 2, ArgumentsOrder::DEFAULT_ORDER, VariableType::FLOAT64());  \
        *callSign = index;                                                                         \
        /* 2 : 2 input parameters */                                                               \
        std::array<VariableType, 2> params = {                                                     \
            VariableType::FLOAT64(),                                                               \
            VariableType::FLOAT64(),                                                               \
        };                                                                                         \
        callSign->SetParameters(params.data());                                                    \
        callSign->SetGCLeafFunction(true);                                                         \
        callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);                    \
    }

DEF_FLOAT_BINARY_CALL_SIGNATURE_BY_NAME(FloatMod)
DEF_FLOAT_BINARY_CALL_SIGNATURE_BY_NAME(FloatAtan2)
DEF_FLOAT_BINARY_CALL_SIGNATURE_BY_NAME(FloatPow)

#undef DEF_FLOAT_BINARY_CALL_SIGNATURE_BY_NAME

DEF_CALL_SIGNATURE(FindElementWithCache)
{
    // 4 : 4 input parameters
    CallSignature index("FindElementWithCache", 0, 4, ArgumentsOrder::DEFAULT_ORDER, VariableType::INT32());
    *callSign = index;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(DoubleToInt)
{
    // 2 : 2 input parameters
    CallSignature index("DoubleToInt", 0, 2, ArgumentsOrder::DEFAULT_ORDER, VariableType::INT32());
    *callSign = index;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::FLOAT64(),
        VariableType::NATIVE_POINTER(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(DoubleToLength)
{
    // 1 : 1 input parameters
    CallSignature index("DoubleToLength", 0, 1, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = index;
    // 1 : 1 input parameters
    std::array<VariableType, 1> params = {
        VariableType::FLOAT64(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(MarkingBarrier)
{
    // 4 : 4 input parameters
    CallSignature index("MarkingBarrier", 0, 4, ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = index;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER()
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(StoreBarrier)
{
    // 4 : 4 input parameters
    CallSignature index("StoreBarrier", 0, 4, ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = index;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER()
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(CallArg0)
{
    // 2 : 2 input parameters
    CallSignature callArg0("callArg0", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = callArg0;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY()
    };
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB);
}

DEF_CALL_SIGNATURE(CallArg1)
{
    // 3 : 3 input parameters
    CallSignature callArg1("callArg1", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = callArg1;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY()
    };
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB);
}

DEF_CALL_SIGNATURE(CallArgs2)
{
    // 4 : 4 input parameters
    CallSignature callArgs2("callArgs2", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = callArgs2;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY()
    };
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB);
}

DEF_CALL_SIGNATURE(CallArgs3)
{
    // 5 : 5 input parameters
    CallSignature callArgs3("callArgs3", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = callArgs3;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY()
    };
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB);
}

DEF_CALL_SIGNATURE(CallThisRange)
{
    // 3 : 3 input parameters
    CallSignature callThisRange("callThisRange", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = callThisRange;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY()
    };
    callSign->SetVariadicArgs(true);
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB);
}

DEF_CALL_SIGNATURE(CallRange)
{
    // 2 : 2 input parameters
    CallSignature callRange("callRange", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = callRange;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY()
    };
    callSign->SetVariadicArgs(true);
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB);
}

DEF_CALL_SIGNATURE(JsProxyCallInternal)
{
    // 4 : 4 input parameters
    CallSignature proxyCallInternal("JsProxyCallInternal", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_POINTER());
    *callSign = proxyCallInternal;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),    // glue
        VariableType::INT64(),      // actual argC
        VariableType::JS_POINTER(), // callTarget
        VariableType::NATIVE_POINTER(),    // argv
    };
    callSign->SetVariadicArgs(false);
    callSign->SetParameters(params.data());
    callSign->SetTailCall(true);
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::COMMON_STUB);
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(JsBoundCallInternal)
{
    // 6 : 6 input parameters
    CallSignature boundCallInternal("JsBoundCallInternal", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_POINTER());
    *callSign = boundCallInternal;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::NATIVE_POINTER(),    // glue
        VariableType::INT64(),      // actual argC
        VariableType::JS_POINTER(), // callTarget
        VariableType::NATIVE_POINTER(),    // argv
        VariableType::JS_POINTER(), // this
        VariableType::JS_POINTER(), // new
    };
    callSign->SetVariadicArgs(false);
    callSign->SetParameters(params.data());
    callSign->SetTailCall(true);
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::COMMON_STUB);
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(CreateArrayFromList)
{
    // 3 : 3 input parameters
    CallSignature createArrayFromList("CreateArrayFromList", 0, 3, ArgumentsOrder::DEFAULT_ORDER,
                                     VariableType::JS_POINTER());
    *callSign = createArrayFromList;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::NATIVE_POINTER(),
    };

    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_VARARGS);
}

DEF_CALL_SIGNATURE(DeoptHandlerAsm)
{
    // 1 : 1 input parameters
    CallSignature deoptHandlerAsm("DeoptHandlerAsm", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = deoptHandlerAsm;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::NATIVE_POINTER(),     // glue
        VariableType::NATIVE_POINTER(),     // deoptType
        VariableType::NATIVE_POINTER(),     // depth
    };
    callSign->SetVariadicArgs(false);
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
    callSign->SetTargetKind(CallSignature::TargetKind::DEOPT_STUB);
}

DEF_CALL_SIGNATURE(TimeClip)
{
    // 1 : 1 input parameters
    CallSignature index("TimeClip", 0, 1, ArgumentsOrder::DEFAULT_ORDER, VariableType::FLOAT64());
    *callSign = index;
    // 1 : 1 input parameters
    std::array<VariableType, 1> params = {
        VariableType::FLOAT64(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(SetDateValues)
{
    // 3 : 3 input parameters
    CallSignature index("SetDateValues", 0, 3, ArgumentsOrder::DEFAULT_ORDER, VariableType::FLOAT64());
    *callSign = index;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::FLOAT64(),
        VariableType::FLOAT64(),
        VariableType::FLOAT64(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(StartCallTimer)
{
    CallSignature index("StartCallTimer", 0, 3, ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = index;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::BOOL()
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(EndCallTimer)
{
    CallSignature index("EndCallTimer", 0, 2, ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = index;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY()
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(GetSingleCharCodeByIndex)
{
    // 3 : 3 input parameters
    CallSignature signature("GetSingleCharCodeByIndex", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::INT32());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::JS_ANY(),          // ecmaString
        VariableType::INT32(),           // index
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(CreateStringBySingleCharCode)
{
    // 2 : 2 input parameters
    CallSignature signature("CreateStringByCharCode", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::INT32(),           // charcode
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(Getpropiterator)
{
    // 2 : 2 input parameters
    CallSignature signature("Getpropiterator", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::JS_ANY(),          // object
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(Getnextpropname)
{
    // 2 : 2 input parameters
    CallSignature signature("Getnextpropname", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::JS_ANY(),          // iter
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(CreateJSSetIterator)
{
    // 2 : 2 input parameters
    CallSignature signature("CreateJSSetIterator", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::JS_ANY(),          // obj
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(CreateJSMapIterator)
{
    // 2 : 2 input parameters
    CallSignature signature("CreateJSMapIterator", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::JS_ANY(),          // obj
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(FastStringEqual)
{
    // 3 : 3 input parameters
    CallSignature signature("FastStringEqual", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::BOOL());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::JS_ANY(),          // ecmaString1
        VariableType::JS_ANY(),          // ecmaString2
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(FastStringAdd)
{
    // 3 : 3 input parameters
    CallSignature signature("FastStringAdd", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::JS_ANY(),          // ecmaString1
        VariableType::JS_ANY(),          // ecmaString2
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(DeleteObjectProperty)
{
    // 3 : 3 input parameters
    CallSignature signature("DeleteObjectProperty", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::JS_ANY(),          // object
        VariableType::JS_ANY(),          // prop
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}
}  // namespace panda::ecmascript::kungfu
