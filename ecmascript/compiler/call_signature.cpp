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

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif


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

DEF_CALL_SIGNATURE(JSTaggedValueHasProperty)
{
    BINARY_CALL_SIGNATURE(JSTaggedValueHasProperty)
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

DEF_CALL_SIGNATURE(GetPropertyByNameWithMega)
{
    // 7 : 7 input parameters
    CallSignature getPropertyByName("GetPropertyByNameWithMega", 0, 7, ArgumentsOrder::DEFAULT_ORDER,
                                    VariableType::JS_ANY());
    *callSign = getPropertyByName;
    // 7 : 7 input parameters
    std::array<VariableType, 7> params = {
        VariableType::NATIVE_POINTER(), // glue
        VariableType::JS_ANY(),         // receiver
        VariableType::INT64(),          // key
        VariableType::NATIVE_POINTER(), // MegaStubCache
        VariableType::JS_ANY(),         // prop
        VariableType::JS_ANY(),         // jsFunc
        VariableType::INT32(),          // slot id
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(SetPropertyByNameWithMega)
{
    // 8 : 8 input parameters
    CallSignature setPropertyByName("SetPropertyByNameWithMega", 0, 8, ArgumentsOrder::DEFAULT_ORDER,
                                    VariableType::JS_ANY());
    *callSign = setPropertyByName;
    // 8 : 8 input parameters
    std::array<VariableType, 8> params = {
        VariableType::NATIVE_POINTER(), // glue
        VariableType::JS_ANY(),         // receiver
        VariableType::INT64(),          // key
        VariableType::JS_ANY(),         // value
        VariableType::NATIVE_POINTER(), // MegaStubCache
        VariableType::JS_ANY(),         // prop
        VariableType::JS_ANY(),         // jsFunc
        VariableType::INT32(),          // slot id
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

DEF_CALL_SIGNATURE(Definefunc)
{
    // 6 : 6 input parameters
    CallSignature definefunc("Definefunc", 0, 6, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::JS_ANY());
    *callSign = definefunc;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::NATIVE_POINTER(),    // glue
        VariableType::JS_ANY(),            // jsFunc
        VariableType::INT32(),             // methodId
        VariableType::INT32(),             // length
        VariableType::JS_ANY(),            // lexEnv
        VariableType::INT32(),             // slotId
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(DefineField)
{
    // 4 : 4 input parameters
    CallSignature defineField("DefineField", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::JS_ANY());
    *callSign = defineField;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),    // glue
        VariableType::JS_ANY(),            // receiver
        VariableType::JS_ANY(),            // key
        VariableType::JS_ANY(),            // acc
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(ConvertCharToInt32)
{
    // 2 : 2 input parameters
    CallSignature convertCharToInt32("ConvertCharToInt32", 0, 2, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::INT32());
    *callSign = convertCharToInt32;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),     // glue
        VariableType::INT32(),              // charcode
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(ConvertCharToDouble)
{
    // 2 : 2 input parameters
    CallSignature convertCharToDouble("ConvertCharToDouble", 0, 2, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::FLOAT64());
    *callSign = convertCharToDouble;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),     // glue
        VariableType::INT32(),              // charcode
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

DEF_CALL_SIGNATURE(IsIn)
{
    BINARY_CALL_SIGNATURE(IsIn)
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

#define SETVALUEBARRIER_CALL_ARGS_SIGNATURE_COMMON(name)                    \
    /* 6 : 4 input parameters + 2 fake parameter */                         \
    CallSignature signature(#name, 0, 6,                                    \
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());               \
    *callSign = signature;                                                  \
    /* 6 : 4 input parameters + 2 fake parameter */                         \
    std::array<VariableType, 6> params = {                                  \
        VariableType::NATIVE_POINTER(),                                     \
        VariableType::JS_POINTER(),                                         \
        VariableType::NATIVE_POINTER(),                                     \
        VariableType::JS_ANY(),                                             \
        VariableType::INT64(),                                              \
        VariableType::INT64(),                                              \
    };                                                                      \
    callSign->SetParameters(params.data());                                 \
    callSign->SetGCLeafFunction(true);                                      \
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);              \
                                                                            \
    std::vector<CallSignature::ParamAttr> paramAttrs = {                    \
        CallSignature::ParamAttr::NoAttr,                                   \
        CallSignature::ParamAttr::NoAttr,                                   \
        CallSignature::ParamAttr::NoAttr,                                   \
        CallSignature::ParamAttr::NoAttr,                                   \
        CallSignature::ParamAttr::Dead,                                     \
        CallSignature::ParamAttr::Dead,                                     \
    };                                                                      \
    callSign->SetParamAttr(std::move(paramAttrs))

DEF_CALL_SIGNATURE(SetValueWithBarrier)
{
    SETVALUEBARRIER_CALL_ARGS_SIGNATURE_COMMON(SetValueWithBarrier);
}

DEF_CALL_SIGNATURE(SetNonSValueWithBarrier)
{
    SETVALUEBARRIER_CALL_ARGS_SIGNATURE_COMMON(SetNonSValueWithBarrier);
}

DEF_CALL_SIGNATURE(SetSValueWithBarrier)
{
    SETVALUEBARRIER_CALL_ARGS_SIGNATURE_COMMON(SetSValueWithBarrier);
}

DEF_CALL_SIGNATURE(ASMFastWriteBarrier)
{
    SETVALUEBARRIER_CALL_ARGS_SIGNATURE_COMMON(ASMFastWriteBarrier);
    callSign->SetTargetKind(CallSignature::TargetKind::ASM_CALL_BARRIER_STUB);
}

DEF_CALL_SIGNATURE(VerifyBarrier)
{
    SETVALUEBARRIER_CALL_ARGS_SIGNATURE_COMMON(VerifyBarrier);
}

#undef SETVALUEBARRIER_CALL_ARGS_SIGNATURE_COMMON

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

DEF_CALL_SIGNATURE(CreateObjectHavingMethod)
{
    // 3 : 3 input parameters
    CallSignature signature("CreateObjectHavingMethod", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::JS_ANY(),          // obj
        VariableType::JS_ANY(),          // levEnv
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(CopyRestArgs)
{
    // 5 : 5 input parameters
    CallSignature signature("CopyRestArgs", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::NATIVE_POINTER(),  // actual argv
        VariableType::INT32(),           // startIdx
        VariableType::INT32(),           // numArgs
        VariableType::JS_ANY(),          // actual argv Array
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

DEF_CALL_SIGNATURE(GetUnmappedArgs)
{
    // 4 : 4 input parameters
    CallSignature signature("GetUnmappedArgs", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::NATIVE_POINTER(),  // actual argv
        VariableType::INT32(),           // numArgs
        VariableType::JS_ANY(),          // actual argv Array
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(GetCallSpreadArgs)
{
    // 2 : 2 input parameters
    CallSignature signature("GetCallSpreadArgs", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::JS_ANY(),          // array
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
#if ENABLE_NEXT_OPTIMIZATION
    callSign->SetVariadicArgs(false);
#else
    callSign->SetVariadicArgs(true);
#endif
    callSign->SetParameters(params.data());
    callSign->SetTargetKind(CallSignature::TargetKind::BUILTINS_STUB);
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(BuiltinsWithArgv)
{
    // 7 : 7 input parameters
    CallSignature builtinsWtihArgv("BuiltinsWithArgv", 0, 7,
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

#define BASELINE_CALL_ARGS_SIGNATURE_COMMON(name)                           \
    /* 1 : 1 input parameters */                                            \
    CallSignature signature(#name, 0, 1,                                    \
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());             \
    *callSign = signature;                                                  \
    std::array<VariableType, 1> params = { /* 1: 1 input parameters */      \
        VariableType::NATIVE_POINTER(),                                     \
    };                                                                      \
    callSign->SetVariadicArgs(true);                                        \
    callSign->SetParameters(params.data());                                 \
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC); \
    callSign->SetCallConv(CallSignature::CallConv::GHCCallConv);

DEF_CALL_SIGNATURE(CallArg0AndCheckToBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallArg0AndCheckToBaseline)
}

DEF_CALL_SIGNATURE(CallArg1AndCheckToBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallArg1AndCheckToBaseline)
}

DEF_CALL_SIGNATURE(CallArgs2AndCheckToBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallArgs2AndCheckToBaseline)
}

DEF_CALL_SIGNATURE(CallArgs3AndCheckToBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallArgs3AndCheckToBaseline)
}

DEF_CALL_SIGNATURE(CallThisArg0AndCheckToBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallThisArg0AndCheckToBaseline)
}

DEF_CALL_SIGNATURE(CallThisArg1AndCheckToBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallThisArg1AndCheckToBaseline)
}

DEF_CALL_SIGNATURE(CallThisArgs2AndCheckToBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallThisArgs2AndCheckToBaseline)
}

DEF_CALL_SIGNATURE(CallThisArgs3AndCheckToBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallThisArgs3AndCheckToBaseline)
}

DEF_CALL_SIGNATURE(CallRangeAndCheckToBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallRangeAndCheckToBaseline)
}

DEF_CALL_SIGNATURE(CallNewAndCheckToBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallNewAndCheckToBaseline)
}

DEF_CALL_SIGNATURE(SuperCallAndCheckToBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(SuperCallAndCheckToBaseline)
}

DEF_CALL_SIGNATURE(CallThisRangeAndCheckToBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallThisRangeAndCheckToBaseline)
}

DEF_CALL_SIGNATURE(CallArg0AndDispatchFromBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallArg0AndDispatchFromBaseline)
}

DEF_CALL_SIGNATURE(CallArg1AndDispatchFromBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallArg1AndDispatchFromBaseline)
}

DEF_CALL_SIGNATURE(CallArgs2AndDispatchFromBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallArgs2AndDispatchFromBaseline)
}

DEF_CALL_SIGNATURE(CallArgs3AndDispatchFromBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallArgs3AndDispatchFromBaseline)
}

DEF_CALL_SIGNATURE(CallThisArg0AndDispatchFromBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallThisArg0AndDispatchFromBaseline)
}

DEF_CALL_SIGNATURE(CallThisArg1AndDispatchFromBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallThisArg1AndDispatchFromBaseline)
}

DEF_CALL_SIGNATURE(CallThisArgs2AndDispatchFromBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallThisArgs2AndDispatchFromBaseline)
}

DEF_CALL_SIGNATURE(CallThisArgs3AndDispatchFromBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallThisArgs3AndDispatchFromBaseline)
}

DEF_CALL_SIGNATURE(CallRangeAndDispatchFromBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallRangeAndDispatchFromBaseline)
}

DEF_CALL_SIGNATURE(CallNewAndDispatchFromBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallNewAndDispatchFromBaseline)
}

DEF_CALL_SIGNATURE(SuperCallAndDispatchFromBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(SuperCallAndDispatchFromBaseline)
}

DEF_CALL_SIGNATURE(CallThisRangeAndDispatchFromBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallThisRangeAndDispatchFromBaseline)
}

DEF_CALL_SIGNATURE(CallArg0AndCheckToBaselineFromBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallArg0AndCheckToBaselineFromBaseline)
}

DEF_CALL_SIGNATURE(CallArg1AndCheckToBaselineFromBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallArg1AndCheckToBaselineFromBaseline)
}

DEF_CALL_SIGNATURE(CallArgs2AndCheckToBaselineFromBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallArgs2AndCheckToBaselineFromBaseline)
}

DEF_CALL_SIGNATURE(CallArgs3AndCheckToBaselineFromBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallArgs3AndCheckToBaselineFromBaseline)
}

DEF_CALL_SIGNATURE(CallThisArg0AndCheckToBaselineFromBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallThisArg0AndCheckToBaselineFromBaseline)
}

DEF_CALL_SIGNATURE(CallThisArg1AndCheckToBaselineFromBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallThisArg1AndCheckToBaselineFromBaseline)
}

DEF_CALL_SIGNATURE(CallThisArgs2AndCheckToBaselineFromBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallThisArgs2AndCheckToBaselineFromBaseline)
}

DEF_CALL_SIGNATURE(CallThisArgs3AndCheckToBaselineFromBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallThisArgs3AndCheckToBaselineFromBaseline)
}

DEF_CALL_SIGNATURE(CallRangeAndCheckToBaselineFromBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallRangeAndCheckToBaselineFromBaseline)
}

DEF_CALL_SIGNATURE(CallNewAndCheckToBaselineFromBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallNewAndCheckToBaselineFromBaseline)
}

DEF_CALL_SIGNATURE(SuperCallAndCheckToBaselineFromBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(SuperCallAndCheckToBaselineFromBaseline)
}

DEF_CALL_SIGNATURE(CallThisRangeAndCheckToBaselineFromBaseline)
{
    BASELINE_CALL_ARGS_SIGNATURE_COMMON(CallThisRangeAndCheckToBaselineFromBaseline)
}

DEF_CALL_SIGNATURE(GetBaselineBuiltinFp)
{
    /* 1 : 1 input parameters */
    CallSignature getBaselineBuiltinFp("GetBaselineBuiltinFp", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::NATIVE_POINTER());
    *callSign = getBaselineBuiltinFp;
    std::array<VariableType, 1> params = { /* 1 : 1 input parameters */
        VariableType::NATIVE_POINTER(),  // glue
    };
    callSign->SetVariadicArgs(true);
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

#define AOT_CALL_SIGNATURE(name)                                        \
    /* 6 : 6 input parameters */                                        \
    CallSignature signature(#name, 0, 6,                                \
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());         \
    *callSign = signature;                                              \
    std::array<VariableType, 6> params = { /* 6 : 6 input parameters */ \
        VariableType::NATIVE_POINTER(),    /* glue */                   \
        VariableType::INT64(),             /* actual argC */            \
        VariableType::NATIVE_POINTER(),    /* actual argV */            \
        VariableType::JS_ANY(),            /* call target */            \
        VariableType::JS_ANY(),            /* new target */             \
        VariableType::JS_ANY(),            /* thisobj */                \
    };                                                                  \
    callSign->SetVariadicArgs(true);                                    \
    callSign->SetParameters(params.data());                             \
    callSign->SetCallConv(CallSignature::CallConv::WebKitJSCallConv);

#define FAST_AOT_CALL_SIGNATURE(name)                                   \
    /* 3 : 3 input parameters */                                        \
    CallSignature signature(#name, 0, 3,                                \
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());         \
    *callSign = signature;                                              \
    std::array<VariableType, 3> params = { /* 3 : 3 input parameters */ \
        VariableType::NATIVE_POINTER(),     /* glue */                  \
        VariableType::JS_ANY(),      /* call target */                  \
        VariableType::JS_ANY(),      /* thisobj */                      \
    };                                                                  \
    callSign->SetVariadicArgs(true);                                    \
    callSign->SetParameters(params.data());                             \
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);

DEF_CALL_SIGNATURE(OptimizedCallAndPushArgv)
{
    AOT_CALL_SIGNATURE(OptimizedCallAndPushArgv)
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(OptimizedFastCallAndPushArgv)
{
    /* 6 : 6 input parameters */
    CallSignature optimizedFastCallAndPushArgv("OptimizedFastCallAndPushArgv", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = optimizedFastCallAndPushArgv;
    std::array<VariableType, 6> params = { /* 6 : 6 input parameters */
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::INT64(),           // actual argC
        VariableType::NATIVE_POINTER(),  // actual argV
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
    AOT_CALL_SIGNATURE(JSCall)
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(JSOptimizedCall)
{
    AOT_CALL_SIGNATURE(JSOptimizedCall)
    callSign->SetTargetKind(CallSignature::TargetKind::OPTIMIZED_STUB);
}

DEF_CALL_SIGNATURE(JSCallNew)
{
    AOT_CALL_SIGNATURE(JSCallNew)
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(JSOptimizedFastCall)
{
    FAST_AOT_CALL_SIGNATURE(JSOptimizedFastCall)
    callSign->SetTargetKind(CallSignature::TargetKind::OPTIMIZED_FAST_CALL_STUB);
}

DEF_CALL_SIGNATURE(AOTCallToAsmInterBridge)
{
    AOT_CALL_SIGNATURE(AOTCallToAsmInterBridge)
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(FastCallToAsmInterBridge)
{
    FAST_AOT_CALL_SIGNATURE(FastCallToAsmInterBridge)
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

DEF_CALL_SIGNATURE(ResumeRspAndReturnBaseline)
{
    // 4 : 4 input parameters
    CallSignature resumeRspAndReturnBaseline("ResumeRspAndReturnBaseline", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = resumeRspAndReturnBaseline;
    std::array<VariableType, 4> params = { // 4 : 4 input parameters
        VariableType::JS_ANY(),            // %r13 - acc
        VariableType::NATIVE_POINTER(),    // %rbp - prevSp
        VariableType::NATIVE_POINTER(),    // %r12 - sp
        VariableType::NATIVE_POINTER(),    // %rbx - jumpSizeAfterCall
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

DEF_CALL_SIGNATURE(FinishObjSizeTracking)
{
    // 1 : 1 input parameters
    CallSignature finishObjSizeTracking("FinishObjSizeTracking", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = finishObjSizeTracking;
    std::array<VariableType, 1> params = { // 1 : 1 input parameters
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

DEF_CALL_SIGNATURE(CallBigIntAsIntN)
{
    // 2 : 2 input parameters
    CallSignature signature("CallBigIntAsIntN", 0, 2, ArgumentsOrder::DEFAULT_ORDER,
                            VariableType::JS_POINTER());
    *callSign = signature;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::FLOAT64(),
        VariableType::JS_POINTER()
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(CallBigIntAsUintN)
{
    // 2 : 2 input parameters
    CallSignature signature("CallBigIntAsUintN", 0, 2, ArgumentsOrder::DEFAULT_ORDER,
                            VariableType::JS_POINTER());
    *callSign = signature;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::FLOAT64(),
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

DEF_CALL_SIGNATURE(IntLexicographicCompare)
{
    // 2 : 2 input parameters
    CallSignature intLexicographicCompare("IntLexicographicCompare", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::INT32());
    *callSign = intLexicographicCompare;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::JS_ANY(),
        VariableType::JS_ANY()
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(DoubleLexicographicCompare)
{
    // 2 : 2 input parameters
    CallSignature doubleLexicographicCompare("DoubleLexicographicCompare", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::INT32());
    *callSign = doubleLexicographicCompare;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::JS_ANY(),
        VariableType::JS_ANY()
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(FastArraySortString)
{
    // 2 : 2 input parameters
    CallSignature fastArraySortString("FastArraySortString", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::INT32());
    *callSign = fastArraySortString;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY()
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(StringToNumber)
{
    // 4 : 4 input parameters
    CallSignature stringToNumber("StringToDoubleWithRadix", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = stringToNumber;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::JS_POINTER(),
        VariableType::INT32(),
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

DEF_CALL_SIGNATURE(ReverseTypedArray)
{
    CallSignature reverseTypedArray("ReverseTypedArray", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = reverseTypedArray;
    std::array<VariableType, 1> params = {
        VariableType::NATIVE_POINTER(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(ReverseArray)
{
    constexpr size_t paramCount = 2;
    // 3 : 3 input parameters
    CallSignature ArrayReverse("ReverseArray", 0, paramCount,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = ArrayReverse;
    // 3 : 3 input parameters
    std::array<VariableType, paramCount> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::INT32()
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(SortTypedArray)
{
    CallSignature sortTypedArray("SortTypedArray", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = sortTypedArray;
    std::array<VariableType, 1> params = {
        VariableType::NATIVE_POINTER(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(FillObject)
{
    constexpr size_t paramCount = 3;
    // 3 : 3 input parameters
    CallSignature ObjectFill("FillObject", 0, paramCount,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = ObjectFill;
    // 3 : 3 input parameters
    std::array<VariableType, paramCount> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32()
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(IsFastRegExp)
{
    // 3 : 3 input parameters
    CallSignature isFastRegExp("IsFastRegExp", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::BOOL());
    *callSign = isFastRegExp;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY()
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(LrInt)
{
    constexpr size_t paramCount = 1;
    // 1 : 1 input parameters
    CallSignature lrInt("LrInt", 0, paramCount, ArgumentsOrder::DEFAULT_ORDER, VariableType::INT8());
    *callSign = lrInt;
    std::array<VariableType, paramCount> params = {
        VariableType::FLOAT64(),
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

DEF_CALL_SIGNATURE(CallContainersArgs2)
{
    PUSH_CALL_ARGS_AND_DISPATCH_NATIVE_RANGE_SIGNATURE(CallContainersArgs2)
}

DEF_CALL_SIGNATURE(CallContainersArgs3)
{
    PUSH_CALL_ARGS_AND_DISPATCH_NATIVE_RANGE_SIGNATURE(CallContainersArgs3)
}

DEF_CALL_SIGNATURE(CallReturnWithArgv)
{
    PUSH_CALL_ARGS_AND_DISPATCH_NATIVE_RANGE_SIGNATURE(CallReturnWithArgv)
}

DEF_CALL_SIGNATURE(CallGetterToBaseline)
{
    PUSH_CALL_ARGS_AND_DISPATCH_NATIVE_RANGE_SIGNATURE(CallGetterToBaseline)
}

DEF_CALL_SIGNATURE(CallSetterToBaseline)
{
    PUSH_CALL_ARGS_AND_DISPATCH_NATIVE_RANGE_SIGNATURE(CallSetterToBaseline)
}

DEF_CALL_SIGNATURE(CallContainersArgs2ToBaseline)
{
    PUSH_CALL_ARGS_AND_DISPATCH_NATIVE_RANGE_SIGNATURE(CallContainersArgs2ToBaseline)
}

DEF_CALL_SIGNATURE(CallContainersArgs3ToBaseline)
{
    PUSH_CALL_ARGS_AND_DISPATCH_NATIVE_RANGE_SIGNATURE(CallContainersArgs3ToBaseline)
}

DEF_CALL_SIGNATURE(CallReturnWithArgvToBaseline)
{
    PUSH_CALL_ARGS_AND_DISPATCH_NATIVE_RANGE_SIGNATURE(CallReturnWithArgvToBaseline)
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

DEF_CALL_SIGNATURE(JSFastCallWithArgVAndPushArgv)
{
    // 4 : 4 input parameters
    CallSignature jSCallWithArgV("JSFastCallWithArgVAndPushArgv", 0, 4,
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

DEF_CALL_SIGNATURE(JSCallWithArgVAndPushArgv)
{
    // 5 : 5 input parameters
    CallSignature jSCallWithArgVAndPushArgv("JSCallWithArgVAndPushArgv", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = jSCallWithArgVAndPushArgv;
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
    // 6 : 6 input parameters
    CallSignature callOptimized("CallOptimized", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = callOptimized;
    std::array<VariableType, 6> params = { // 6 : 6 input parameters
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::INT64(),           // actual argC
        VariableType::NATIVE_POINTER(),  // actual argV
        VariableType::JS_ANY(),      // call target
        VariableType::JS_ANY(),      // new target
        VariableType::JS_ANY(),      // thisobj
    };
    callSign->SetVariadicArgs(true);
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::WebKitJSCallConv);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(SuperCallWithArgV)
{
    // 5 : 5 input parameters
    CallSignature superCallWithArgV("SuperCallWithArgV", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = superCallWithArgV;
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

DEF_CALL_SIGNATURE(CollectingOpcodes)
{
    // 2 : 2 input parameters
    CallSignature collectingOpcodes("CollectingOpcodes", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = collectingOpcodes;
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

DEF_CALL_SIGNATURE(DebugOsrEntry)
{
    // 2 : 2 input parameters
    CallSignature debugOsrEntry("DebugOsrEntry", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = debugOsrEntry;
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

DEF_CALL_SIGNATURE(SetBitAtomic)
{
    // 3 : 3 input parameters
    CallSignature index("SetBitAtomic", 0, 3, ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = index;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::INT32()
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

DEF_CALL_SIGNATURE(CallDateNow)
{
    CallSignature signature("CallDateNow", 0, 0, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::FLOAT64());
    *callSign = signature;
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(UpdateFieldType)
{
    // 2 : 2 input parameters
    CallSignature index("UpdateFieldType", 0, 2, ArgumentsOrder::DEFAULT_ORDER, VariableType::INT32());
    *callSign = index;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::JS_ANY(),
        VariableType::INT64(),
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

DEF_CALL_SIGNATURE(SaturateTruncDoubleToInt32)
{
    // 1 : 1 input parameters
    CallSignature index("SaturateTruncDoubleToInt32", 0, 1, ArgumentsOrder::DEFAULT_ORDER, VariableType::INT32());
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

DEF_CALL_SIGNATURE(SharedGCMarkingBarrier)
{
    // 4 : 4 input parameters
    CallSignature index("SharedGCMarkingBarrier", 0, 4, ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = index;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(CallArg0Stub)
{
    // 2 : 2 input parameters
    CallSignature callArg0Stub("callArg0Stub", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = callArg0Stub;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::NATIVE_POINTER(),     // glue
        VariableType::JS_ANY(),             // func
    };
    callSign->SetParameters(params.data());
}

DEF_CALL_SIGNATURE(CallArg1Stub)
{
    // 3 : 3 input parameters
    CallSignature callArg1Stub("callArg1Stub", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = callArg1Stub;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),     // glue
        VariableType::JS_ANY(),             // func
        VariableType::JS_ANY()              // a0
    };
    callSign->SetParameters(params.data());
}

DEF_CALL_SIGNATURE(CallArg2Stub)
{
    // 4 : 4 input parameters
    CallSignature callArg2Stub("callArg2Stub", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = callArg2Stub;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),     // glue
        VariableType::JS_ANY(),             // func
        VariableType::JS_ANY(),             // a0
        VariableType::JS_ANY()              // a1
    };
    callSign->SetParameters(params.data());
}

DEF_CALL_SIGNATURE(CallArg3Stub)
{
    // 5 : 5 input parameters
    CallSignature callArg3Stub("callArg3Stub", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = callArg3Stub;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),     // glue
        VariableType::JS_ANY(),             // func
        VariableType::JS_ANY(),             // a0
        VariableType::JS_ANY(),             // a1
        VariableType::JS_ANY()              // a2
    };
    callSign->SetParameters(params.data());
}

DEF_CALL_SIGNATURE(CallThis0Stub)
{
    // 3 : 3 input parameters
    CallSignature callThis0Stub("callThis0Stub", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = callThis0Stub;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),     // glue
        VariableType::JS_ANY(),             // func
        VariableType::JS_ANY(),             // this
    };
    callSign->SetParameters(params.data());
}

DEF_CALL_SIGNATURE(CallThis1Stub)
{
    // 4 : 4 input parameters
    CallSignature callThis1Stub("callThis1Stub", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = callThis1Stub;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),     // glue
        VariableType::JS_ANY(),             // func
        VariableType::JS_ANY(),             // this
        VariableType::JS_ANY()              // a0
    };
    callSign->SetParameters(params.data());
}

DEF_CALL_SIGNATURE(CallThis2Stub)
{
    // 5 : 5 input parameters
    CallSignature callThis2Stub("callThis2Stub", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = callThis2Stub;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::NATIVE_POINTER(),     // glue
        VariableType::JS_ANY(),             // func
        VariableType::JS_ANY(),             // this
        VariableType::JS_ANY(),             // a0
        VariableType::JS_ANY()              // a1
    };
    callSign->SetParameters(params.data());
}

DEF_CALL_SIGNATURE(CallThis3Stub)
{
    // 6 : 6 input parameters
    CallSignature callThis3Stub("callThis3Stub", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = callThis3Stub;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::NATIVE_POINTER(),     // glue
        VariableType::JS_ANY(),             // func
        VariableType::JS_ANY(),             // this
        VariableType::JS_ANY(),             // a0
        VariableType::JS_ANY(),             // a1
        VariableType::JS_ANY()              // a2
    };
    callSign->SetParameters(params.data());
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
    UNARY_CALL_SIGNATURE(CreateJSSetIterator)
}

DEF_CALL_SIGNATURE(JSSetEntries)
{
    UNARY_CALL_SIGNATURE(JSSetEntries)
}

DEF_CALL_SIGNATURE(CreateJSMapIterator)
{
    UNARY_CALL_SIGNATURE(CreateJSMapIterator)
}

DEF_CALL_SIGNATURE(JSMapKeys)
{
    UNARY_CALL_SIGNATURE(JSMapKeys)
}

DEF_CALL_SIGNATURE(JSMapValues)
{
    UNARY_CALL_SIGNATURE(JSMapValues)
}

DEF_CALL_SIGNATURE(JSMapGet)
{
    BINARY_CALL_SIGNATURE(JSMapGet)
}

DEF_CALL_SIGNATURE(StringIteratorNext)
{
    UNARY_CALL_SIGNATURE(StringIteratorNext)
}

DEF_CALL_SIGNATURE(ArrayIteratorNext)
{
    UNARY_CALL_SIGNATURE(ArrayIteratorNext)
}

DEF_CALL_SIGNATURE(MapIteratorNext)
{
    UNARY_CALL_SIGNATURE(MapIteratorNext)
}

DEF_CALL_SIGNATURE(SetIteratorNext)
{
    UNARY_CALL_SIGNATURE(SetIteratorNext)
}

DEF_CALL_SIGNATURE(GetIterator)
{
    UNARY_CALL_SIGNATURE(GetIterator)
}

DEF_CALL_SIGNATURE(JSMapHas)
{
    *callSign = CallSignature("JSMapHas", 0, ArgumentsOrder::DEFAULT_ORDER, VariableType::BOOL(),
        {
            VariableType::NATIVE_POINTER(),  // glue
            VariableType::JS_ANY(),          // obj
            VariableType::JS_ANY(),          // key
        });
}

DEF_CALL_SIGNATURE(JSSetHas)
{
    *callSign = CallSignature("JSSetHas", 0, ArgumentsOrder::DEFAULT_ORDER, VariableType::BOOL(),
        {
            VariableType::NATIVE_POINTER(),  // glue
            VariableType::JS_ANY(),          // obj
            VariableType::JS_ANY(),          // key
        });
}

DEF_CALL_SIGNATURE(JSMapDelete)
{
    *callSign = CallSignature("JSMapDelete", 0, ArgumentsOrder::DEFAULT_ORDER, VariableType::BOOL(),
        {
            VariableType::NATIVE_POINTER(),  // glue
            VariableType::JS_ANY(),          // obj
            VariableType::JS_ANY(),          // key
        });
}

DEF_CALL_SIGNATURE(JSSetDelete)
{
    *callSign = CallSignature("JSSetDelete", 0, ArgumentsOrder::DEFAULT_ORDER, VariableType::BOOL(),
        {
            VariableType::NATIVE_POINTER(),  // glue
            VariableType::JS_ANY(),          // obj
            VariableType::JS_ANY(),          // key
        });
}

DEF_CALL_SIGNATURE(JSSetAdd)
{
    *callSign = CallSignature("JSSetAdd", 0, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY(),
        {
            VariableType::NATIVE_POINTER(),  // glue
            VariableType::JS_ANY(),          // obj
            VariableType::JS_ANY(),          // key
        });
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

DEF_CALL_SIGNATURE(StringAdd)
{
    // 4 : 4 input parameters
    CallSignature signature("StringAdd", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = signature;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::JS_ANY(),          // ecmaString1
        VariableType::JS_ANY(),          // ecmaString2
        VariableType::INT32(),           // stringadd status
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

DEF_CALL_SIGNATURE(CopyTypedArrayBuffer)
{
    // 6 : 6 input parameters
    CallSignature CopyTypedArrayBuffer("CopyTypedArrayBuffer", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = CopyTypedArrayBuffer;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32()
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(CreateJSTypedArrayEntries)
{
    // 2 : 2 input parameters
    CallSignature signature("CreateJSTypedArrayEntries", 0, 2,
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

DEF_CALL_SIGNATURE(SameValue)
{
    // 3 : 3 input parameters
    CallSignature signature("SameValue", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::BOOL());
    *callSign = signature;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::NATIVE_POINTER(),  // glue
        VariableType::JS_ANY(),          // left
        VariableType::JS_ANY(),          // right
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}

DEF_CALL_SIGNATURE(CreateJSTypedArrayKeys)
{
    // 2 : 2 input parameters
    CallSignature signature("CreateJSTypedArrayKeys", 0, 2,
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

DEF_CALL_SIGNATURE(CreateJSTypedArrayValues)
{
    // 2 : 2 input parameters
    CallSignature signature("CreateJSTypedArrayValues", 0, 2,
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

DEF_CALL_SIGNATURE(GrowElementsCapacity)
{
    // 6 : 6 input parameters
    CallSignature growElementsCapacity("GrowElementsCapacity", 0, 3, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::JS_ANY());
    *callSign = growElementsCapacity;
    // 6 : 6 input parameters
    std::array<VariableType, 3> params = {     // 3 : 3 input parameters
        VariableType::NATIVE_POINTER(),    // glue
        VariableType::JS_ANY(),            // thisValue
        VariableType::INT32(),             // newlength
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);
}
#define CREATE_RSET_SIGNATURE_COMMON(name)                                  \
    CallSignature signature(#name, 0, 1,                                    \
        ArgumentsOrder::DEFAULT_ORDER, VariableType::NATIVE_POINTER());     \
    *callSign = signature;                                                  \
    std::array<VariableType, 1> params = {                                  \
        VariableType::NATIVE_POINTER(),                                     \
    };                                                                      \
    callSign->SetParameters(params.data());                                 \
    callSign->SetGCLeafFunction(true);                                      \
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC)

DEF_CALL_SIGNATURE(CreateLocalToShare)
{
    CREATE_RSET_SIGNATURE_COMMON(CreateLocalToShare);
}

DEF_CALL_SIGNATURE(CreateOldToNew)
{
    CREATE_RSET_SIGNATURE_COMMON(CreateOldToNew);
}

#undef CREATE_RSET_SIGNATURE_COMMON

DEF_CALL_SIGNATURE(BatchBarrier)
{
    constexpr size_t paramCount = 4;
    CallSignature signature("BatchBarrier", 0, paramCount, ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    std::array<VariableType, paramCount> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);;
}
DEF_CALL_SIGNATURE(ReverseBarrier)
{
    constexpr size_t paramCount = 4;
    CallSignature signature("ReverseBarrier", 0, paramCount, ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    std::array<VariableType, paramCount> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);;
}

DEF_CALL_SIGNATURE(ObjectCopy)
{
    constexpr size_t paramCount = 3;
    // 3 : 3 input parameters
    CallSignature ArrayCopy("ObjectCopy", 0, paramCount,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = ArrayCopy;
    // 3 : 3 input parameters
    std::array<VariableType, paramCount> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32()
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetTargetKind(CallSignature::TargetKind::RUNTIME_STUB_NO_GC);
}

DEF_CALL_SIGNATURE(MoveBarrierInRegion)
{
    constexpr size_t paramCount = 5;
    CallSignature signature("MoveBarrierInRegion", 0, paramCount, ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *callSign = signature;
    std::array<VariableType, paramCount> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::NATIVE_POINTER(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);;
}

DEF_CALL_SIGNATURE(MoveBarrierCrossRegion)
{
    constexpr size_t paramCount = 6;
    CallSignature signature("MoveBarrierCrossRegion", 0, paramCount, ArgumentsOrder::DEFAULT_ORDER,
                            VariableType::VOID());
    *callSign = signature;
    std::array<VariableType, paramCount> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
        VariableType::INT32(),
        VariableType::NATIVE_POINTER(),
        VariableType::NATIVE_POINTER(),
    };
    callSign->SetParameters(params.data());
    callSign->SetGCLeafFunction(true);
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);;
}

DEF_CALL_SIGNATURE(FindEntryFromNameDictionary)
{
    constexpr size_t paramCount = 3;
    CallSignature signature("FindEntryFromNameDictionary", 0, paramCount, ArgumentsOrder::DEFAULT_ORDER,
                            VariableType::INT32());
    *callSign = signature;
    std::array<VariableType, paramCount> params = {
        VariableType::NATIVE_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::CCallConv);;
}
}  // namespace panda::ecmascript::kungfu
