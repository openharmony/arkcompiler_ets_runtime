/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_CALL_SIGNATURE_H
#define ECMASCRIPT_COMPILER_BUILTINS_CALL_SIGNATURE_H

#include "ecmascript/compiler/builtins/builtins_call_signature_id_properties.h"
#include "ecmascript/base/config.h"
#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/global_env_fields.h"

namespace panda::ecmascript::kungfu {

class BuiltinsStubCSigns {
public:
    using ID = stubcsigns::BuiltinsStubCSignsID;
    static_assert(ID::NONE == 0);
    static constexpr int NONE = ID::NONE;
    static constexpr int NUM_OF_BUILTINS_STUBS = ID::NUM_OF_BUILTINS_STUBS;
    using SubID = builtinssubid::SubID;
    static constexpr int NUM_OF_BUILTINS_STUBS_EXTEND = SubID::NUM_OF_BUILTINS_STUBS_EXTEND;
    static_assert(SubID::NONE == 0);
#define ASSERT_ID_EQUAL(name) \
    static_assert((static_cast<uint32_t>(ID::name)) == (static_cast<uint32_t>(SubID::name)));

#define ASSERT_ID_EQUAL_DYN(name, type, ...) \
    static_assert((static_cast<uint32_t>(ID::type##name)) == (static_cast<uint32_t>(SubID::type##name)));

    BUILTINS_STUB_LIST(ASSERT_ID_EQUAL, ASSERT_ID_EQUAL_DYN, ASSERT_ID_EQUAL)
#undef ASSERT_ID_EQUAL_DYN
#undef ASSERT_ID_EQUAL

    static void Initialize();

    static void GetCSigns(std::vector<const CallSignature*>& callSigns);

    static const CallSignature *Get(size_t index)
    {
        ASSERT(index < NUM_OF_BUILTINS_STUBS_EXTEND);
        return &callSigns_[index];
    }

    static const std::string &GetName(int index)
    {
        ASSERT(index < NUM_OF_BUILTINS_STUBS_EXTEND);
        return callSigns_[index].GetName();
    }

    static const CallSignature* BuiltinsCSign()
    {
        return &builtinsCSign_;
    }

    static const CallSignature* BuiltinsWithArgvCSign()
    {
        return &builtinsWithArgvCSign_;
    }

    static bool IsFastBuiltin(ID builtinId)
    {
        return builtinId > NONE && builtinId < NUM_OF_BUILTINS_STUBS;
    }

    static bool IsTypedBuiltin(ID builtinId)
    {
        return (BuiltinsStubCSigns::ID::StringLocaleCompare == builtinId) ||
               (BuiltinsStubCSigns::ID::StringIteratorProtoNext == builtinId) ||
               (BuiltinsStubCSigns::ID::ArrayIteratorProtoNext == builtinId) ||
               (BuiltinsStubCSigns::ID::MapIteratorProtoNext == builtinId) ||
               (BuiltinsStubCSigns::ID::SetIteratorProtoNext == builtinId) ||
               (BuiltinsStubCSigns::ID::ArraySort == builtinId) ||
               (BuiltinsStubCSigns::ID::ArrayConcat == builtinId) ||
               (BuiltinsStubCSigns::ID::GlobalDecodeURIComponent == builtinId) ||
               ((BuiltinsStubCSigns::ID::TYPED_BUILTINS_FIRST <= builtinId) &&
               (builtinId <= BuiltinsStubCSigns::ID::TYPED_BUILTINS_LAST)) ||
               IsAOTCallThisBuiltin(builtinId);
    }

    static bool IsAOTCallThisBuiltin([[maybe_unused]]ID builtinId)
    {
#define BUILTINS_ID(METHOD, OBJECT, ...)  ID::OBJECT##METHOD,
        static std::unordered_set<BuiltinsStubCSigns::ID> callThisBuiltinsIds = {
            BUILTINS_METHOD_STUB_LIST(BUILTINS_ID, BUILTINS_ID, BUILTINS_ID, BUILTINS_ID)
        };
#undef BUILTINS_ID
#if ENABLE_NEXT_OPTIMIZATION
        return callThisBuiltinsIds.count(builtinId);
#else
        return false;
#endif
    }

    static bool IsTypedInlineBuiltin(ID builtinId)
    {
        if (ID::TYPED_BUILTINS_INLINE_FIRST <= builtinId && builtinId <= ID::TYPED_BUILTINS_INLINE_LAST) {
            return true;
        }
        if (BuiltinsStubCSigns::ID::DataViewSetInt32 <= builtinId &&
            builtinId <= BuiltinsStubCSigns::ID::DataViewSetFloat64) {
            return true;
        }
        // NOTE(schernykh): try to remove this switch and move StringFromCharCode to TYPED_BUILTINS_INLINE list
        switch (builtinId) {
            case BuiltinsStubCSigns::ID::ArraySort:
            case BuiltinsStubCSigns::ID::StringFromCharCode:
            case BuiltinsStubCSigns::ID::StringCharCodeAt:
            case BuiltinsStubCSigns::ID::StringSubstring:
            case BuiltinsStubCSigns::ID::StringSubStr:
            case BuiltinsStubCSigns::ID::StringSlice:
            case BuiltinsStubCSigns::ID::MapGet:
            case BuiltinsStubCSigns::ID::MapHas:
            case BuiltinsStubCSigns::ID::MapKeys:
            case BuiltinsStubCSigns::ID::MapValues:
            case BuiltinsStubCSigns::ID::MapEntries:
            case BuiltinsStubCSigns::ID::SetHas:
            case BuiltinsStubCSigns::ID::MapDelete:
            case BuiltinsStubCSigns::ID::SetDelete:
            case BuiltinsStubCSigns::ID::TypedArrayEntries:
            case BuiltinsStubCSigns::ID::TypedArrayKeys:
            case BuiltinsStubCSigns::ID::TypedArrayValues:
            case BuiltinsStubCSigns::ID::SetValues:
            case BuiltinsStubCSigns::ID::SetEntries:
            case BuiltinsStubCSigns::ID::MapClear:
            case BuiltinsStubCSigns::ID::SetClear:
            case BuiltinsStubCSigns::ID::SetAdd:
            case BuiltinsStubCSigns::ID::NumberParseFloat:
            case BuiltinsStubCSigns::ID::NumberParseInt:
            case BuiltinsStubCSigns::ID::NumberIsFinite:
            case BuiltinsStubCSigns::ID::NumberIsNaN:
            case BuiltinsStubCSigns::ID::NumberIsInteger:
            case BuiltinsStubCSigns::ID::NumberIsSafeInteger:
            case BuiltinsStubCSigns::ID::ObjectIs:
            case BuiltinsStubCSigns::ID::ObjectGetPrototypeOf:
            case BuiltinsStubCSigns::ID::ObjectGetProto:
            case BuiltinsStubCSigns::ID::ObjectCreate:
            case BuiltinsStubCSigns::ID::ObjectIsPrototypeOf:
            case BuiltinsStubCSigns::ID::ObjectHasOwnProperty:
            case BuiltinsStubCSigns::ID::ReflectGetPrototypeOf:
            case BuiltinsStubCSigns::ID::ReflectGet:
            case BuiltinsStubCSigns::ID::ReflectHas:
            case BuiltinsStubCSigns::ID::ReflectConstruct:
            case BuiltinsStubCSigns::ID::ReflectApply:
            case BuiltinsStubCSigns::ID::FunctionPrototypeApply:
            case BuiltinsStubCSigns::ID::FunctionPrototypeBind:
            case BuiltinsStubCSigns::ID::FunctionPrototypeCall:
            case BuiltinsStubCSigns::ID::FunctionPrototypeHasInstance:
            case BuiltinsStubCSigns::ID::ArrayIndexOf:
            case BuiltinsStubCSigns::ID::ArrayIncludes:
            case BuiltinsStubCSigns::ID::ArrayValues:
            case BuiltinsStubCSigns::ID::ArrayForEach:
            case BuiltinsStubCSigns::ID::ArrayFind:
            case BuiltinsStubCSigns::ID::ArrayFindIndex:
            case BuiltinsStubCSigns::ID::ArrayFilter:
            case BuiltinsStubCSigns::ID::ArrayMap:
            case BuiltinsStubCSigns::ID::ArraySome:
            case BuiltinsStubCSigns::ID::ArrayEvery:
            case BuiltinsStubCSigns::ID::ArrayPop:
            case BuiltinsStubCSigns::ID::ArraySlice:
            case BuiltinsStubCSigns::ID::ArrayPush:
                return true;
            default:
                return false;
        }
        return false;
    }

    static bool IsTypedBuiltinNumber(ID builtinId)
    {
        return BuiltinsStubCSigns::ID::NumberConstructor == builtinId;
    }

    static bool IsTypedBuiltinGlobal(ID builtinId)
    {
        return BuiltinsStubCSigns::ID::GlobalDecodeURIComponent == builtinId;
    }

    static bool IsTypedBuiltinCallThis0(ID builtinId)
    {
        switch (builtinId) {
            case BuiltinsStubCSigns::ID::MapIteratorProtoNext:
            case BuiltinsStubCSigns::ID::SetIteratorProtoNext:
            case BuiltinsStubCSigns::ID::StringIteratorProtoNext:
            case BuiltinsStubCSigns::ID::ArrayIteratorProtoNext:
            case BuiltinsStubCSigns::ID::IteratorProtoReturn:
            case BuiltinsStubCSigns::ID::ArrayConcat:
            case BuiltinsStubCSigns::ID::ArraySort:
                return true;
            default:
                return IsAOTCallThisBuiltin(builtinId);
        }
    }

    static bool IsTypedBuiltinCallThis1(ID builtinId)
    {
        switch (builtinId) {
            case BuiltinsStubCSigns::ID::ArrayConcat:
            case BuiltinsStubCSigns::ID::JsonStringify:
            case BuiltinsStubCSigns::ID::StringLocaleCompare:
                return true;
            default:
                return IsAOTCallThisBuiltin(builtinId);
        }
    }

    static bool IsTypedBuiltinCallThis2(ID builtinId)
    {
        return IsAOTCallThisBuiltin(builtinId);
    }

    static bool IsTypedBuiltinCallThis3(ID builtinId)
    {
        switch (builtinId) {
            case BuiltinsStubCSigns::ID::StringLocaleCompare:
                return true;
            default:
                return IsAOTCallThisBuiltin(builtinId);
        }
    }

    static bool IsSideEffectBuiltin(ID builtinId)
    {
        switch (builtinId) {
            case BuiltinsStubCSigns::ID::StringLocaleCompare:
                return false;
            default:
                return true;
        }
    }

    static bool CheckBuiltinsIdInvalid(ID builtinId)
    {
        auto result = kungfu::BuiltinsStubCSigns::GetGlobalEnvFieldIndex(builtinId);
        return result == GlobalEnvField::INVALID;
    }

    static size_t GetGlobalEnvIndex(ID builtinId);

    static std::string GetBuiltinName(ID id)
    {
        // Note: this function will be refactored soon in future.
        using namespace stubcsigns;
        static const std::map<BuiltinsStubCSigns::ID, const std::string> builtinId2Str = {
            {MathAcos, "Math.acos"},
            {MathAcosh, "Math.acosh"},
            {MathAsin, "Math.asin"},
            {MathAsinh, "Math.asinh"},
            {MathAtan, "Math.atan"},
            {MathAtan2, "Math.atan2"},
            {MathAtanh, "Math.atanh"},
            {MathCos, "Math.cos"},
            {MathCosh, "Math.cosh"},
            {MathSign, "Math.sign"},
            {MathSin, "Math.sin"},
            {MathSinh, "Math.sinh"},
            {MathTan, "Math.tan"},
            {MathTanh, "Math.tanh"},
            {MathLog, "Math.log"},
            {MathLog2, "Math.log2"},
            {MathLog10, "Math.log10"},
            {MathLog1p, "Math.log1p"},
            {MathExp, "Math.exp"},
            {MathExpm1, "Math.expm1"},
            {MathClz32, "Math.clz32"},
            {MathSqrt, "Math.sqrt"},
            {MathCbrt, "Math.cbrt"},
            {MathAbs, "Math.abs"},
            {MathPow, "Math.pow"},
            {MathTrunc, "Math.trunc"},
            {MathRound, "Math.round"},
            {MathFRound, "Math.fround"},
            {MathCeil, "Math.ceil"},
            {MathFloor, "Math.floor"},
            {MathImul, "Math.imul"},
            {MathMax, "Math.max"},
            {MathMin, "Math.min"},
            {DateGetTime, "Date.prototype.getTime"},
            {DateNow, "Date.now"},
            {TypedArrayEntries, "TypedArray.entries"},
            {TypedArrayKeys, "TypedArray.keys"},
            {TypedArrayValues, "TypedArray.values"},
            {GlobalIsFinite, "isFinite"},
            {GlobalIsNan, "isNan"},
            {BigIntAsIntN, "BigInt.asIntN"},
            {BigIntAsUintN, "BigInt.asUintN"},
            {MapGet, "Map.get"},
            {MapHas, "Map.has"},
            {MapKeys, "Map.keys"},
            {MapValues, "Map.values"},
            {MapEntries, "Map.entries"},
            {SetValues, "Set.values"},
            {SetEntries, "Set.entries"},
            {SetHas, "Set.has"},
            {MapDelete, "Map.delete"},
            {SetDelete, "Set.delete"},
            {MapClear, "Map.clear"},
            {SetClear, "Set.clear"},
            {SetAdd, "Set.add"},
            {BigIntConstructor, "BigInt"},
            {NumberParseFloat, "Number.parseFloat"},
            {NumberIsInteger, "Number.isInteger"},
            {NumberIsSafeInteger, "Number.isSafeInteger"},
            {NumberParseInt, "Number.parseInt"},
            {StringCharCodeAt, "String.prototype.charCodeAt"},
            {StringSubstring, "String.prototype.substring"},
            {StringSubStr, "String.prototype.substr"},
            {StringSlice, "String.prototype.slice"},
            {ObjectIs, "Object.is"},
            {ObjectGetPrototypeOf, "Object.getPrototypeOf"},
            {ObjectGetProto, "Object.prototype.getProto"},
            {ObjectCreate, "Object.create"},
            {ObjectIsPrototypeOf, "Object.prototype.isPrototypeOf"},
            {ObjectHasOwnProperty, "Object.prototype.hasOwnProperty"},
            {ReflectGetPrototypeOf, "Reflect.getPrototypeOf"},
            {ReflectGet, "Reflect.get"},
            {ReflectHas, "Reflect.has"},
            {ReflectConstruct, "Reflect.construct"},
            {ReflectApply, "Reflect.apply"},
            {FunctionPrototypeApply, "Function.prototype.apply"},
            {FunctionPrototypeBind, "Function.prototype.bind"},
            {FunctionPrototypeCall, "Function.prototype.call"},
            {FunctionPrototypeHasInstance, "Function.prototype.hasInstance"},
            {ArrayIncludes, "Array.prototype.includes"},
            {ArrayIndexOf, "Array.prototype.indexOf"},
            {ArrayEntries, "Array.prototype.entries"},
            {ArrayValues, "Array.prototype.values"},
            {ArrayKeys, "Array.prototype.keys"},
            {ArrayFind, "Array.prototype.find"},
            {ArrayFindIndex, "Array.prototype.findIndex"},
            {ArrayForEach, "Array.prototype.foreach"},
            {ArrayFilter, "Array.prototype.filter"},
            {ArraySome, "Array.prototype.some"},
            {ArrayMap, "Array.prototype.map"},
            {ArrayEvery, "Array.prototype.every"},
            {ArrayPop, "Array.prototype.pop"},
            {ArrayPush, "Array.prototype.push"},
            {ArraySlice, "Array.prototype.slice"},
            {DataViewGetInt8, "DataView.prototype.GetInt8"},
            {DataViewGetUint8, "DataView.prototype.GetUint8"},
            {DataViewGetInt16, "DataView.prototype.GetInt16"},
            {DataViewGetUint16, "DataView.prototype.GetUint16"},
            {DataViewGetInt32, "DataView.prototype.GetInt32"},
            {DataViewGetUint32, "DataView.prototype.GetUint32"},
            {DataViewGetFloat32, "DataView.prototype.GetFloat32"},
            {DataViewGetFloat64, "DataView.prototype.GetFloat64"},
            {DataViewSetInt8, "DataView.prototype.SetInt8"},
            {DataViewSetUint8, "DataView.prototype.SetUint8"},
            {DataViewSetInt16, "DataView.prototype.SetInt16"},
            {DataViewSetUint16, "DataView.prototype.SetUint16"},
            {DataViewSetInt32, "DataView.prototype.SetInt32"},
            {DataViewSetUint32, "DataView.prototype.SetUint32"},
            {DataViewSetFloat32, "DataView.prototype.SetFloat32"},
            {DataViewSetFloat64, "DataView.prototype.SetFloat64"},
            {ArrayIteratorProtoNext, "ArrayIterator.next"},
            {MapIteratorProtoNext, "MapIterator.next"},
            {SetIteratorProtoNext, "SetIterator.next"},
            {StringLocaleCompare, "String.LocaleCompare"},
            {ArraySort, "Array.Sort"},
        };
        if (builtinId2Str.count(id) > 0) {
            return builtinId2Str.at(id);
        }
#define ID_TO_STRING(Method, Object, ...)  {BuiltinsStubCSigns::ID::Object##Method, #Object"."#Method},
        static const std::unordered_map<BuiltinsStubCSigns::ID, const std::string> builtinId2Str2 = {
            BUILTINS_METHOD_STUB_LIST(ID_TO_STRING, ID_TO_STRING, ID_TO_STRING, ID_TO_STRING)
        };
#undef ID_TO_STRING
        if (builtinId2Str2.count(id) > 0) {
            return builtinId2Str2.at(id);
        }
        return "unnamed-builtin-"+std::to_string(id);
    }

    static GlobalEnvField GetGlobalEnvFieldIndex(ID builtinId)
    {
        ASSERT(builtinId != kungfu::BuiltinsStubCSigns::ID::INVALID);
#define GET_BUILTIN_GLOBAL_ENV_FIELD_INDEX(type, name, index) \
    case kungfu::BuiltinsStubCSigns::ID::name:                \
        return GlobalEnvField::index;
#define GET_BUILTIN_METHOD_STUB_INDEX_IMPL4(name, builtin, unused, index) \
    GET_BUILTIN_GLOBAL_ENV_FIELD_INDEX(unused, builtin##name, index##_INDEX)
#define GET_BUILTIN_METHOD_STUB_INDEX_IMPL6(name, builtin, unused0, unused1, unused2, index) \
    GET_BUILTIN_GLOBAL_ENV_FIELD_INDEX(unused0, builtin##name, index##_INDEX)
        switch (builtinId) {
            GLOBAL_ENV_INLINED_BUILTINS(GET_BUILTIN_GLOBAL_ENV_FIELD_INDEX)
            BUILTINS_METHOD_STUB_LIST(GET_BUILTIN_METHOD_STUB_INDEX_IMPL4, GET_BUILTIN_METHOD_STUB_INDEX_IMPL4,
                                      GET_BUILTIN_METHOD_STUB_INDEX_IMPL4, GET_BUILTIN_METHOD_STUB_INDEX_IMPL6)
            default:
                LOG_ECMA(DEBUG) << "GetGlobalEnvFieldIndex Invalid Id:" << builtinId;
                return GlobalEnvField::INVALID;
        }
#undef SET_BUILTIN_METHOD_STUB_IMPL6
#undef SET_BUILTIN_METHOD_STUB_IMPL4
#undef SET_BUILTIN_FUNCTION_CASE
    }

private:
    static CallSignature callSigns_[NUM_OF_BUILTINS_STUBS_EXTEND];
    static CallSignature builtinsCSign_;
    static CallSignature builtinsWithArgvCSign_;
};

enum class BuiltinsArgs : size_t {
    GLUE = 0,
    NATIVECODE,
    FUNC,
    NEWTARGET,
    THISVALUE,
    NUMARGS,
    ARG0_OR_ARGV,
    ARG1,
    ARG2,
    NUM_OF_INPUTS,
};

#define BUILTINS_STUB_ID(name) (::panda::ecmascript::kungfu::BuiltinsStubCSigns::ID::name)
// to distinguish with the positive method offset of js function
#define PGO_BUILTINS_STUB_ID(name) ((-1) * (::panda::ecmascript::kungfu::BuiltinsStubCSigns::ID::name))
#define IS_TYPED_BUILTINS_ID(id) kungfu::BuiltinsStubCSigns::IsTypedBuiltin(id)
#define IS_TYPED_INLINE_BUILTINS_ID(id) kungfu::BuiltinsStubCSigns::IsTypedInlineBuiltin(id)
#define IS_TYPED_BUILTINS_NUMBER_ID(id) kungfu::BuiltinsStubCSigns::IsTypedBuiltinNumber(id)
#define IS_TYPED_BUILTINS_GLOBAL_ID(id) kungfu::BuiltinsStubCSigns::IsTypedBuiltinGlobal(id)
#define IS_TYPED_BUILTINS_ID_CALL_THIS0(id) kungfu::BuiltinsStubCSigns::IsTypedBuiltinCallThis0(id)
#define IS_TYPED_BUILTINS_ID_CALL_THIS1(id) kungfu::BuiltinsStubCSigns::IsTypedBuiltinCallThis1(id)
#define IS_TYPED_BUILTINS_ID_CALL_THIS2(id) kungfu::BuiltinsStubCSigns::IsTypedBuiltinCallThis2(id)
#define IS_TYPED_BUILTINS_ID_CALL_THIS3(id) kungfu::BuiltinsStubCSigns::IsTypedBuiltinCallThis3(id)
#define GET_TYPED_ENV_FIELD_INEDX(id) kungfu::BuiltinsStubCSigns::GetGlobalEnvFieldIndex(id)
#define IS_INVALID_ID(id) kungfu::BuiltinsStubCSigns::CheckBuiltinsIdInvalid(id)
#define GET_TYPED_GLOBAL_ENV_INDEX(id) kungfu::BuiltinsStubCSigns::GetGlobalEnvIndex(id)
#define IS_SIDE_EFFECT_BUILTINS_ID(id) kungfu::BuiltinsStubCSigns::IsSideEffectBuiltin(id)
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_CALL_SIGNATURE_H
