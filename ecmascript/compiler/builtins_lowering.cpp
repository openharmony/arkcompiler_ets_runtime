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

#include "ecmascript/compiler/builtins_lowering.h"

#include "builtins/builtins_string_stub_builder.h"
#include "ecmascript/global_env.h"

namespace panda::ecmascript::kungfu {
void BuiltinLowering::LowerTypedCallBuitin(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto valuesIn = acc_.GetNumValueIn(gate);
    ASSERT(valuesIn >= 1);
    auto idGate = acc_.GetValueIn(gate, valuesIn - 1);
    auto id = static_cast<BuiltinsStubCSigns::ID>(acc_.GetConstantValue(idGate));
    if (traceBuiltins_) {
        AddTraceLogs(gate, id);
    }
    switch (id) {
        case BUILTINS_STUB_ID(StringLocaleCompare):
            LowerTypedLocaleCompare(gate);
            break;
        case BUILTINS_STUB_ID(JsonStringify):
            LowerTypedStringify(gate);
            break;
        case BUILTINS_STUB_ID(MapProtoIterator):
        case BUILTINS_STUB_ID(SetProtoIterator):
        case BUILTINS_STUB_ID(StringProtoIterator):
        case BUILTINS_STUB_ID(ArrayProtoIterator):
        case BUILTINS_STUB_ID(TypedArrayProtoIterator):
            LowerBuiltinIterator(gate, id);
            break;
        case BUILTINS_STUB_ID(MapIteratorProtoNext):
        case BUILTINS_STUB_ID(SetIteratorProtoNext):
        case BUILTINS_STUB_ID(StringIteratorProtoNext):
        case BUILTINS_STUB_ID(ArrayIteratorProtoNext):
            LowerIteratorNext(gate, id);
            break;
        case BUILTINS_STUB_ID(IteratorProtoReturn):
            LowerIteratorReturn(gate, id);
            break;
        case BUILTINS_STUB_ID(NumberConstructor):
            LowerNumberConstructor(gate);
            break;
        case BUILTINS_STUB_ID(GlobalDecodeURIComponent):
            LowerGlobalDecodeURIComponent(gate);
            break;
        default:
            LowerCallBuiltinStub(gate, id);
            break;
    }
}

void BuiltinLowering::LowerTypedFloor(GateRef gate)
{
    auto ret = TypedFloor(gate);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), ret);
}

GateRef BuiltinLowering::TypedLocaleCompare(GateRef glue, GateRef gate, GateRef thisObj, GateRef thatObj)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);

    Label slowPath(&builder_);
    Label fastPath(&builder_);
    Label localeCompareGC(&builder_);
    Label exit(&builder_);
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.Undefined());

    GateRef isString = builder_.BothAreString(glue, thisObj, thatObj);
    builder_.Branch(isString, &fastPath, &slowPath);
    builder_.Bind(&fastPath);
    {
        result = builder_.CallRuntime(glue, RTSTUB_ID(LocaleCompareCacheable), Gate::InvalidGateRef,
            { builder_.Undefined(), thisObj, thatObj }, gate);
        GateRef status = builder_.TaggedIsUndefined(*result);
        builder_.Branch(status, &localeCompareGC, &exit, BranchWeight::ONE_WEIGHT, BranchWeight::STRONG_WEIGHT,
            "TypedLocaleCompare");
        builder_.Bind(&localeCompareGC);
        {
            result = builder_.CallRuntime(glue, RTSTUB_ID(LocaleCompareWithGc), Gate::InvalidGateRef,
                { builder_.Undefined(), thisObj, thatObj, builder_.Undefined() }, gate);
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&slowPath);
    {
        result = LowerCallRuntime(glue, gate, RTSTUB_ID(LocaleCompare),
            { thisObj, thatObj, builder_.Undefined(), builder_.Undefined()});
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinLowering::TypedFloor(GateRef gate)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);

    Label numberBranch(&builder_);
    Label notNumberBranch(&builder_);
    Label exit(&builder_);

    GateRef para1 = acc_.GetValueIn(gate, 0);
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());

    BRANCH_CIR(builder_.TaggedIsNumber(para1), &numberBranch, &notNumberBranch);
    builder_.Bind(&numberBranch);
    {
        GateRef value = builder_.GetDoubleOfTNumber(para1);
        Label IsNan(&builder_);
        Label NotNan(&builder_);
        GateRef condition = builder_.DoubleIsNAN(value);
        BRANCH_CIR(condition, &IsNan, &NotNan);
        builder_.Bind(&NotNan);
        {
            GateRef glue = glue_;
            int index = RTSTUB_ID(FloatFloor);
            GateRef floor = builder_.CallNGCRuntime(glue, index, Gate::InvalidGateRef, {value}, gate);
            result = builder_.DoubleToTaggedDoublePtr(floor);
            builder_.Jump(&exit);
        }
        builder_.Bind(&IsNan);
        {
            result = builder_.DoubleToTaggedDoublePtr(builder_.Double(base::NAN_VALUE));
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&notNumberBranch);
    {
        builder_.Jump(&exit);
    }

    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinLowering::IntToTaggedIntPtr(GateRef x)
{
    GateRef val = builder_.SExtInt32ToInt64(x);
    return builder_.ToTaggedIntPtr(val);
}

GateRef BuiltinLowering::LowerCallRuntime(GateRef glue, GateRef gate, int index, const std::vector<GateRef> &args,
                                          bool useLabel)
{
    const std::string name = RuntimeStubCSigns::GetRTName(index);
    if (useLabel) {
        GateRef result = builder_.CallRuntime(glue, index, Gate::InvalidGateRef, args, gate, name.c_str());
        return result;
    } else {
        const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(CallRuntime));
        GateRef target = builder_.IntPtr(index);
        GateRef result = builder_.Call(cs, glue, target, builder_.GetDepend(), args, gate, name.c_str());
        return result;
    }
}

void BuiltinLowering::ReplaceHirWithValue(GateRef hirGate, GateRef value)
{
    acc_.ReplaceHirDirectly(hirGate, builder_.GetStateDepend(), value);
}

void BuiltinLowering::LowerTypedLocaleCompare(GateRef gate)
{
    GateRef glue = glue_;

    size_t index = 0;
    GateRef thisObj = acc_.GetValueIn(gate, index++);
    GateRef thatObj = acc_.GetValueIn(gate, index++);

    static constexpr size_t NUM_OF_ARGS = 4;
    ASSERT(acc_.GetNumValueIn(gate) > 0);
    size_t argsIn = acc_.GetNumValueIn(gate) - 1;
    GateRef result = Circuit::NullGate();
    if (argsIn == 2) { // 2: string.localeCompare(string)
        // If the number of args is two, it must satisfy conditions for cache optimization.
        // The cache of icu collator if locale is undefined
        result = TypedLocaleCompare(glue, gate, thisObj, thatObj);
    } else {
        // willdo: Implement cache fastpath
        std::vector<GateRef> args = { thisObj, thatObj };
        ASSERT(argsIn <= NUM_OF_ARGS);
        args.reserve(NUM_OF_ARGS);
        while (index < argsIn) {
            GateRef arg = acc_.GetValueIn(gate, index++);
            args.emplace_back(arg);
        }
        while (index < NUM_OF_ARGS) {
            args.emplace_back(builder_.Undefined());
            index++;
        }
        result = LowerCallRuntime(glue, gate, RTSTUB_ID(LocaleCompare), args);
    }
    ReplaceHirWithValue(gate, result);
}

GateRef BuiltinLowering::LowerCallTargetCheck(Environment *env, GateRef gate)
{
    builder_.SetEnvironment(env);
    GateRef idGate = acc_.GetValueIn(gate, 1);
    BuiltinsStubCSigns::ID id = static_cast<BuiltinsStubCSigns::ID>(acc_.GetConstantValue(idGate));
    switch (id) {
        case BuiltinsStubCSigns::ID::MapProtoIterator:
        case BuiltinsStubCSigns::ID::SetProtoIterator:
        case BuiltinsStubCSigns::ID::StringProtoIterator:
        case BuiltinsStubCSigns::ID::ArrayProtoIterator:
        case BuiltinsStubCSigns::ID::TypedArrayProtoIterator: {
            return LowerCallTargetCheckWithDetector(gate, id);
        }
        case BuiltinsStubCSigns::ID::DateGetTime:
        case BuiltinsStubCSigns::ID::MapClear:
        case BuiltinsStubCSigns::ID::MapDelete:
        case BuiltinsStubCSigns::ID::MapGet:
        case BuiltinsStubCSigns::ID::MapHas:
        case BuiltinsStubCSigns::ID::SetAdd:
        case BuiltinsStubCSigns::ID::SetClear:
        case BuiltinsStubCSigns::ID::SetDelete:
        case BuiltinsStubCSigns::ID::SetHas: {
            return LowerCallTargetCheckWithObjectType(gate, id);
        }
        case BuiltinsStubCSigns::ID::BigIntConstructor:
        case BuiltinsStubCSigns::ID::NumberConstructor: {
            return LowerCallTargetCheckWithGlobalEnv(gate, id);
        }
        default: {
            return LowerCallTargetCheckDefault(gate, id);
        }
    }
}

GateRef BuiltinLowering::LowerCallTargetCheckDefault(GateRef gate, BuiltinsStubCSigns::ID id)
{
    GateRef globalEnvFunction =
        builder_.GetGlobalEnvValue(VariableType::JS_ANY(), glue_, circuit_->GetGlobalEnvCache(),
                                   static_cast<size_t>(GET_TYPED_ENV_FIELD_INEDX(id)));
    GateRef function = acc_.GetValueIn(gate, 0); // 0: function
    return builder_.Equal(function, globalEnvFunction);
}

GateRef BuiltinLowering::LowerCallTargetCheckWithGlobalEnv(GateRef gate, BuiltinsStubCSigns::ID id)
{
    GateRef globalEnv = circuit_->GetGlobalEnvCache();
    GateRef globalFunction =
        builder_.GetGlobalEnvObj(globalEnv, GET_TYPED_GLOBAL_ENV_INDEX(id));
    GateRef target = acc_.GetValueIn(gate, 0); // 0:target
    return builder_.Equal(target, globalFunction);
}

GateRef BuiltinLowering::LowerCallTargetCheckWithDetector(GateRef gate, BuiltinsStubCSigns::ID id)
{
    JSType expectType = JSType::INVALID;
    GateRef detectorValue = Gate::InvalidGateRef;
    switch (id) {
        case BuiltinsStubCSigns::ID::MapProtoIterator: {
            expectType = JSType::JS_MAP;
            detectorValue = builder_.BoolNot(builder_.GetMapIteratorDetector(circuit_->GetGlobalEnvCache()));
            break;
        }
        case BuiltinsStubCSigns::ID::SetProtoIterator: {
            expectType = JSType::JS_SET;
            detectorValue = builder_.BoolNot(builder_.GetSetIteratorDetector(circuit_->GetGlobalEnvCache()));
            break;
        }
        case BuiltinsStubCSigns::ID::StringProtoIterator: {
            expectType = JSType::STRING_FIRST;
            detectorValue = builder_.BoolNot(builder_.GetStringIteratorDetector(circuit_->GetGlobalEnvCache()));
            break;
        }
        case BuiltinsStubCSigns::ID::ArrayProtoIterator: {
            expectType = JSType::JS_ARRAY;
            detectorValue = builder_.BoolNot(builder_.GetArrayIteratorDetector(circuit_->GetGlobalEnvCache()));
            break;
        }
        case BuiltinsStubCSigns::ID::TypedArrayProtoIterator: {
            expectType = JSType::JS_TYPED_ARRAY_FIRST;
            detectorValue = builder_.BoolNot(builder_.GetTypedArrayIteratorDetector(circuit_->GetGlobalEnvCache()));
            break;
        }
        default: {
            LOG_COMPILER(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
    }
    GateRef obj = acc_.GetValueIn(gate, 2);  // 2: iterator obj
    return LogicAndBuilder(builder_.GetCurrentEnvironment())
        .And(builder_.TaggedIsHeapObjectOp(obj))
        .And(builder_.IsSpecificObjectType(obj, expectType))
        .And(detectorValue)
        .Done();
}

GateRef BuiltinLowering::LowerCallTargetCheckWithObjectType(GateRef gate, BuiltinsStubCSigns::ID id)
{
    JSType expectType = JSType::INVALID;
    switch (id) {
        case BuiltinsStubCSigns::ID::MapClear:
        case BuiltinsStubCSigns::ID::MapDelete:
        case BuiltinsStubCSigns::ID::MapGet:
        case BuiltinsStubCSigns::ID::MapHas: {
            expectType = JSType::JS_MAP;
            break;
        }
        case BuiltinsStubCSigns::ID::SetAdd:
        case BuiltinsStubCSigns::ID::SetClear:
        case BuiltinsStubCSigns::ID::SetDelete:
        case BuiltinsStubCSigns::ID::SetHas: {
            expectType = JSType::JS_SET;
            break;
        }
        case BuiltinsStubCSigns::ID::DateGetTime: {
            expectType = JSType::JS_DATE;
            break;
        }
        default: {
            LOG_COMPILER(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
    }
    GateRef obj = acc_.GetValueIn(gate, 2);  // 2: receiver obj
    return LogicAndBuilder(builder_.GetCurrentEnvironment())
        .And(builder_.TaggedIsHeapObjectOp(obj))
        .And(builder_.IsSpecificObjectType(obj, expectType))
        .And(LowerCallTargetCheckDefault(gate, id))
        .Done();
}

GateRef BuiltinLowering::CheckPara(GateRef gate, GateRef funcCheck)
{
    GateRef idGate = acc_.GetValueIn(gate, 1);
    BuiltinsStubCSigns::ID id = static_cast<BuiltinsStubCSigns::ID>(acc_.GetConstantValue(idGate));
    if (IS_TYPED_INLINE_BUILTINS_ID(id)) {
        // Don't need check param. Param was checked before
        return funcCheck;
    }
    if (IS_TYPED_BUILTINS_ID(id)) {
        // Don't need check param. Param was checked before
        return funcCheck;
    }
    switch (id) {
        case BuiltinsStubCSigns::ID::StringLocaleCompare:
        case BuiltinsStubCSigns::ID::ArrayConcat:
        case BuiltinsStubCSigns::ID::ArraySort:
        case BuiltinsStubCSigns::ID::JsonStringify:
        case BuiltinsStubCSigns::ID::MapProtoIterator:
        case BuiltinsStubCSigns::ID::SetProtoIterator:
        case BuiltinsStubCSigns::ID::StringProtoIterator:
        case BuiltinsStubCSigns::ID::ArrayProtoIterator:
        case BuiltinsStubCSigns::ID::TypedArrayProtoIterator:
        case BuiltinsStubCSigns::ID::MapIteratorProtoNext:
        case BuiltinsStubCSigns::ID::SetIteratorProtoNext:
        case BuiltinsStubCSigns::ID::StringIteratorProtoNext:
        case BuiltinsStubCSigns::ID::ArrayIteratorProtoNext:
        case BuiltinsStubCSigns::ID::IteratorProtoReturn:
        case BuiltinsStubCSigns::ID::NumberConstructor:
        case BuiltinsStubCSigns::ID::TypedArrayEntries:
        case BuiltinsStubCSigns::ID::TypedArrayKeys:
        case BuiltinsStubCSigns::ID::TypedArrayValues:
        case BuiltinsStubCSigns::ID::GlobalDecodeURIComponent:
            // Don't need check para
            return funcCheck;
        default: {
            LOG_COMPILER(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
    }
}

void BuiltinLowering::LowerTypedStringify(GateRef gate)
{
    GateRef glue = glue_;
    GateRef value = acc_.GetValueIn(gate, 1);
    std::vector<GateRef> args;
    args.emplace_back(value);
    GateRef result = LowerCallRuntime(glue, gate, RTSTUB_ID(FastStringify), args);
    ReplaceHirWithValue(gate, result);
}

void BuiltinLowering::LowerBuiltinIterator(GateRef gate, BuiltinsStubCSigns::ID id)
{
    GateRef glue = glue_;
    GateRef obj = acc_.GetValueIn(gate, 0);
    GateRef result = Circuit::NullGate();
    switch (id) {
        case BUILTINS_STUB_ID(MapProtoIterator): {
            result = builder_.CallStub(glue, gate, CommonStubCSigns::CreateJSMapIterator, { glue, obj });
            break;
        }
        case BUILTINS_STUB_ID(SetProtoIterator): {
            result = builder_.CallStub(glue, gate, CommonStubCSigns::CreateJSSetIterator, { glue, obj });
            break;
        }
        case BUILTINS_STUB_ID(StringProtoIterator): {
            result = LowerCallRuntime(glue, gate, RTSTUB_ID(CreateStringIterator), { obj }, true);
            break;
        }
        case BUILTINS_STUB_ID(ArrayProtoIterator): {
            result = LowerCallRuntime(glue, gate, RTSTUB_ID(NewJSArrayIterator), { obj }, true);
            break;
        }
        case BUILTINS_STUB_ID(TypedArrayProtoIterator): {
            result = LowerCallRuntime(glue, gate, RTSTUB_ID(NewJSTypedArrayIterator), { obj }, true);
            break;
        }
        default:
            UNREACHABLE();
    }
    ReplaceHirWithValue(gate, result);
}

void BuiltinLowering::LowerIteratorNext(GateRef gate, BuiltinsStubCSigns::ID id)
{
    GateRef glue = glue_;
    GateRef thisObj = acc_.GetValueIn(gate, 0);
    GateRef result = Circuit::NullGate();
    switch (id) {
        case BUILTINS_STUB_ID(MapIteratorProtoNext): {
            result = builder_.CallStub(glue, gate, CommonStubCSigns::MapIteratorNext,
                                       {glue, thisObj, circuit_->GetGlobalEnvCache()});
            break;
        }
        case BUILTINS_STUB_ID(SetIteratorProtoNext): {
            result = builder_.CallStub(glue, gate, CommonStubCSigns::SetIteratorNext,
                                       {glue, thisObj, circuit_->GetGlobalEnvCache()});
            break;
        }
        case BUILTINS_STUB_ID(StringIteratorProtoNext): {
            result = builder_.CallStub(glue, gate, CommonStubCSigns::StringIteratorNext,
                                       {glue, thisObj, circuit_->GetGlobalEnvCache()});
            break;
        }
        case BUILTINS_STUB_ID(ArrayIteratorProtoNext): {
            result = builder_.CallStub(glue, gate, CommonStubCSigns::ArrayIteratorNext,
                                       {glue, thisObj, circuit_->GetGlobalEnvCache()});
            break;
        }
        default:
            UNREACHABLE();
    }
    ReplaceHirWithValue(gate, result);
}

void BuiltinLowering::LowerIteratorReturn(GateRef gate, BuiltinsStubCSigns::ID id)
{
    GateRef glue = glue_;
    GateRef thisObj = acc_.GetValueIn(gate, 0);
    GateRef result = Circuit::NullGate();
    switch (id) {
        case BUILTINS_STUB_ID(IteratorProtoReturn): {
            result = LowerCallRuntime(glue, gate, RTSTUB_ID(IteratorReturn), { thisObj }, true);
            break;
        }
        default:
            UNREACHABLE();
    }
    ReplaceHirWithValue(gate, result);
}

void BuiltinLowering::LowerNumberConstructor(GateRef gate)
{
    auto env = builder_.GetCurrentEnvironment();
    GateRef glue = glue_;
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), IntToTaggedIntPtr(builder_.Int32(0)));
    GateRef param = acc_.GetValueIn(gate, 0);
    Label exit(env);
    Label isNumber(env);
    Label notNumber(env);
    BRANCH_CIR(builder_.TaggedIsNumber(param), &isNumber, &notNumber);
    builder_.Bind(&isNumber);
    {
        result = param;
        builder_.Jump(&exit);
    }
    builder_.Bind(&notNumber);
    {
        Label isLineUtf8String(env);
        Label notLineUtf8String(env);
        // only line string can be integer, because length of sliced string and tree string is at least 13
        BRANCH_CIR(builder_.TaggedIsLineUtf8String(glue, param), &isLineUtf8String, &notLineUtf8String);
        builder_.Bind(&isLineUtf8String);
        {
            Label nonZeroLength(env);
            auto length = builder_.GetLengthFromString(param);
            BRANCH_CIR(builder_.Equal(length, builder_.Int32(0)), &exit, &nonZeroLength);
            builder_.Bind(&nonZeroLength);
            Label isInteger(env);
            BuiltinsStringStubBuilder stringStub(builder_.GetCurrentEnvironment(), circuit_->GetGlobalEnvCache());
            GateRef dataUtf8 = builder_.PtrAdd(param, builder_.IntPtr(LineString::DATA_OFFSET));
            GateRef res = stringStub.StringDataToUint(dataUtf8, length, std::numeric_limits<int32_t>::max());
            BRANCH_CIR(builder_.Int64NotEqual(res, builder_.Int64(-1)), &isInteger, &notLineUtf8String);
            builder_.Bind(&isInteger);
            {
                result = IntToTaggedIntPtr(res);
                builder_.Jump(&exit);
            }
        }
        builder_.Bind(&notLineUtf8String);
        {
            result = LowerCallRuntime(glue, gate, RTSTUB_ID(ToNumericConvertBigInt), { param }, true);
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&exit);
    ReplaceHirWithValue(gate, *result);
}

void BuiltinLowering::LowerGlobalDecodeURIComponent(GateRef gate)
{
    GateRef glue = glue_;
    GateRef param = acc_.GetValueIn(gate, 0);
    GateRef result = LowerCallRuntime(glue, gate, RTSTUB_ID(DecodeURIComponent), { param }, true);
    ReplaceHirWithValue(gate, result);
}

void BuiltinLowering::LowerCallBuiltinStub(GateRef gate, BuiltinsStubCSigns::ID id)
{
    Environment env(gate, circuit_, &builder_);
    size_t numIn = acc_.GetNumValueIn(gate);
    GateRef glue = glue_;
    GateRef function = builder_.GetGlobalEnvValue(VariableType::JS_ANY(), glue, circuit_->GetGlobalEnvCache(),
                                                  static_cast<size_t>(GET_TYPED_ENV_FIELD_INEDX(id)));
    GateRef nativeCode = builder_.LoadWithoutBarrier(VariableType::NATIVE_POINTER(), function,
        builder_.IntPtr(JSFunction::CODE_ENTRY_OFFSET));
    std::vector<GateRef> args(static_cast<size_t>(BuiltinsArgs::NUM_OF_INPUTS), builder_.Undefined());
    args[static_cast<size_t>(BuiltinsArgs::GLUE)] = glue;
    args[static_cast<size_t>(BuiltinsArgs::NATIVECODE)] = nativeCode;
    args[static_cast<size_t>(BuiltinsArgs::FUNC)] = function;
    args[static_cast<size_t>(BuiltinsArgs::NEWTARGET)] = builder_.Undefined();
    args[static_cast<size_t>(BuiltinsArgs::THISVALUE)] = acc_.GetValueIn(gate, 0);
    args[static_cast<size_t>(BuiltinsArgs::NUMARGS)] = builder_.Int32(numIn - 2); // 2: skip thisValue and id
    for (size_t idx = 1; idx < numIn - 1; idx++) {
        args[static_cast<size_t>(BuiltinsArgs::ARG0_OR_ARGV) + idx - 1] = acc_.GetValueIn(gate, idx);
    }
    const CallSignature *cs = BuiltinsStubCSigns::BuiltinsCSign();
    size_t ptrSize = env.Is32Bit() ? sizeof(uint32_t) : sizeof(uint64_t);
    GateRef target = builder_.IntPtr(static_cast<int64_t>(id) * ptrSize);
    GateRef ret = builder_.Call(cs, glue, target, builder_.GetDepend(), args, gate,
                                BuiltinsStubCSigns::GetBuiltinName(id).c_str());
    ReplaceHirWithValue(gate, ret);
}

void BuiltinLowering::AddTraceLogs(GateRef gate, BuiltinsStubCSigns::ID id)
{
    size_t index = RTSTUB_ID(AotCallBuiltinTrace);
    GateRef frameArgs =  acc_.GetFrameArgs(gate);
    GateRef callerFunc = acc_.GetValueIn(frameArgs, static_cast<size_t>(FrameArgIdx::FUNC));
    std::vector<GateRef> args{callerFunc, builder_.Int32ToTaggedInt(builder_.Int32(id))};
    GateRef trace = builder_.CallRuntime(glue_, index, Gate::InvalidGateRef, args, gate);
    acc_.SetDep(gate, trace);
    builder_.SetDepend(acc_.GetDep(gate));  // set gate depend: profiling or STATE_SPLIT
}
}  // namespace panda::ecmascript::kungfu
