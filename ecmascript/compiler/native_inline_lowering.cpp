/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "ecmascript/compiler/native_inline_lowering.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/circuit_builder_helper.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/message_string.h"

namespace panda::ecmascript::kungfu {
void NativeInlineLowering::RunNativeInlineLowering()
{
    std::vector<GateRef> gateList;
    circuit_->GetAllGates(gateList);
    for (const auto &gate : gateList) {
        auto op = acc_.GetOpCode(gate);
        if (op == OpCode::JS_BYTECODE) {
            EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
            switch (ecmaOpcode) {
                case EcmaOpcode::CALLTHIS0_IMM8_V8:
                case EcmaOpcode::CALLTHIS1_IMM8_V8_V8:
                case EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8:
                case EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
                case EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8:
                case EcmaOpcode::WIDE_CALLTHISRANGE_PREF_IMM16_V8:
                case EcmaOpcode::CALLRUNTIME_CALLINIT_PREF_IMM8_V8:
                    TryInlineNativeCallThis(gate);
                    break;
                case EcmaOpcode::CALLARG0_IMM8:
                case EcmaOpcode::CALLARG1_IMM8_V8:
                case EcmaOpcode::CALLARGS2_IMM8_V8_V8:
                case EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8:
                case EcmaOpcode::CALLRANGE_IMM8_IMM8_V8:
                case EcmaOpcode::WIDE_CALLRANGE_PREF_IMM16_V8:
                    break;
                default:
                    break;
            }
        }
    }

    if (EnableLog()) {
        LOG_COMPILER(INFO) << " ";
        LOG_COMPILER(INFO) << "\033[34m" << "================="
                           << " After Native Inline Lowering "
                           << "[" << GetMethodName() << "] "
                           << "=================" << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "=========================== End ===========================" << "\033[0m";
    }
}

void NativeInlineLowering::TryInlineNativeCallThis(GateRef gate)
{
    GateRef thisObj = acc_.GetValueIn(gate, 0);
    GateType thisType = acc_.GetGateType(thisObj);
    if (tsManager_->IsArrayTypeKind(thisType)) {
        TryInlineBuiltinsArrayFunc(gate);
    }
}

void NativeInlineLowering::TryInlineBuiltinsArrayFunc(GateRef gate)
{
    GateRef func = acc_.GetValueIn(gate, acc_.GetNumValueIn(gate) - 1); // 1: last value in
    GateType funcType = acc_.GetGateType(func);
    if (tsManager_->IsBuiltinObjectMethod(BuiltinTypeId::ARRAY, funcType)) {
        std::string name = tsManager_->GetFuncName(funcType);
        if (name == "forEach") {
            RunArrayForeachInline(gate);
        }
    }
}

void NativeInlineLowering::RunArrayForeachInline(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);

    GateRef thisObj = acc_.GetValueIn(gate, 0);
    GateRef callBack = acc_.GetValueIn(gate, 1);
    GateRef thisArg = builder_.Undefined();
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    if (ecmaOpcode == EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8) {
        thisArg = acc_.GetValueIn(gate, 2); // 2: this arg parameter
    }
    builder_.BuiltinPrototypeHClassCheck(thisObj, BuiltinTypeId::ARRAY);
    ElementsKind kind = acc_.TryGetArrayElementsKind(thisObj);
    if (!IsCreateArray(thisObj)) {
        builder_.StableArrayCheck(thisObj, kind, ArrayMetaDataAccessor::Mode::LOAD_ELEMENT);
    }
    Label callBackIsCallable(&builder_);
    Label callBackNotCallable(&builder_);
    builder_.Branch(builder_.IsCallable(callBack), &callBackIsCallable, &callBackNotCallable);
    builder_.Bind(&callBackNotCallable);
    {
        GateRef taggedId = builder_.Int64(GET_MESSAGE_STRING_ID(NonCallable));
        LowerCallRuntime(glue_, gate, RTSTUB_ID(ThrowTypeError), { builder_.ToTaggedIntPtr(taggedId) }, true);
        GateRef exception = builder_.ExceptionConstant();
        builder_.Return(exception);
    }
    builder_.Bind(&callBackIsCallable);
    Label exit(&builder_);
    ArrayForeachCall(gate, thisObj, callBack, thisArg, &exit);
    builder_.Bind(&exit);
    ReplaceHirDirectly(gate, builder_.Undefined());
}

void NativeInlineLowering::ArrayForeachCall(GateRef gate, GateRef thisObj, GateRef callBack, GateRef thisArg,
                                            Label *exit)
{
    GateRef length = builder_.LoadArrayLength(thisObj);
    Label loopHead(&builder_);
    Label loopEnd(&builder_);
    Label exitloop(&builder_);
    Label slowForeach(&builder_);
    DEFVALUE(i, (&builder_), VariableType::INT32(), builder_.Int32(0));
    DEFVALUE(propKey, (&builder_), VariableType::JS_ANY(), builder_.ToTaggedIntPtr(builder_.SExtInt32ToInt64(*i)));
    DEFVALUE(value, (&builder_), VariableType::JS_ANY(), builder_.Hole());
    builder_.Branch(builder_.Int32LessThan(*i, length), &loopHead, &exitloop);
    builder_.LoopBegin(&loopHead);
    ElementsKind kind = acc_.TryGetArrayElementsKind(thisObj);
    value = LoadArrayElement(kind, thisObj, *i);
    Label NotHole(&builder_);
    Label merge(&builder_);
    builder_.Branch(builder_.NotEqual(*value, builder_.Hole()), &NotHole, &exitloop);
    builder_.Bind(&NotHole);
    {
        auto depend = builder_.GetDepend();
        GateRef nativeCall = NativeCallTS(gate, depend,
            {glue_, builder_.Int64(6), callBack, builder_.Undefined(), thisArg, *value, *propKey, thisObj}); // 6: args
        builder_.SetDepend(nativeCall);
        i = builder_.Int32Add(*i, builder_.Int32(1));
        propKey = builder_.ToTaggedIntPtr(builder_.SExtInt32ToInt64(*i));
        builder_.Branch(builder_.IsStabelArray(glue_, thisObj), &merge, &exitloop);
    }
    builder_.Bind(&merge);
    builder_.Branch(builder_.Int32LessThan(*i, length), &loopEnd, &exitloop);
    builder_.Bind(&loopEnd);
    builder_.LoopEnd(&loopHead);
    builder_.Bind(&exitloop);
    builder_.Branch(builder_.Int32LessThan(*i, length), &slowForeach, exit);
    builder_.Bind(&slowForeach);
    {
        GateRef taggedLength = builder_.ToTaggedIntPtr(builder_.SExtInt32ToInt64(length));
        GateRef slowPathCall = LowerCallRuntime(glue_, gate, RTSTUB_ID(ArrayForEachContinue),
            {thisArg, *propKey, thisObj, callBack, taggedLength}, true);
        builder_.SetDepend(slowPathCall);
        builder_.Jump(exit);
    }
}

bool NativeInlineLowering::IsCreateArray(GateRef gate)
{
    if (acc_.GetOpCode(gate) != OpCode::JS_BYTECODE) {
        return false;
    }
    EcmaOpcode ecmaop = acc_.GetByteCodeOpcode(gate);
    switch (ecmaop) {
        case EcmaOpcode::CREATEEMPTYARRAY_IMM8:
        case EcmaOpcode::CREATEEMPTYARRAY_IMM16:
        case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
        case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM16_ID16:
            return true;
        default:
            return false;
    }
    UNREACHABLE();
    return false;
}

GateRef NativeInlineLowering::LoadArrayElement(ElementsKind kind, GateRef gate, GateRef index)
{
    switch (kind) {
        case ElementsKind::INT:
            return builder_.LoadElement<TypedLoadOp::ARRAY_LOAD_INT_ELEMENT>(gate, index);
        case ElementsKind::NUMBER:
            return builder_.LoadElement<TypedLoadOp::ARRAY_LOAD_DOUBLE_ELEMENT>(gate, index);
        case ElementsKind::OBJECT:
            return builder_.LoadElement<TypedLoadOp::ARRAY_LOAD_OBJECT_ELEMENT>(gate, index);
        default:
            if (!Elements::IsHole(kind)) {
                return builder_.LoadElement<TypedLoadOp::ARRAY_LOAD_TAGGED_ELEMENT>(gate, index);
            }
    }
    return builder_.LoadElement<TypedLoadOp::ARRAY_LOAD_HOLE_TAGGED_ELEMENT>(gate, index);
}

void NativeInlineLowering::ReplaceHirDirectly(GateRef gate, GateRef value)
{
    GateRef state = builder_.GetState();
    GateRef depend = builder_.GetDepend();
    auto uses = acc_.Uses(gate);
    for (auto it = uses.begin(); it != uses.end();) {
        if (acc_.IsStateIn(it)) {
            ASSERT(acc_.GetOpCode(*it) != OpCode::IF_SUCCESS &&
                acc_.GetOpCode(*it) != OpCode::IF_EXCEPTION);
            it = acc_.ReplaceIn(it, state);
        } else if (acc_.IsDependIn(it)) {
            it = acc_.ReplaceIn(it, depend);
        } else {
            ASSERT(acc_.IsValueIn(it));
            it = acc_.ReplaceIn(it, value);
        }
    }
    acc_.DeleteGate(gate);
}

GateRef NativeInlineLowering::LowerCallRuntime(GateRef glue, GateRef hirGate, int index,
    const std::vector<GateRef> &args, bool useLabel)
{
    if (useLabel) {
        GateRef result = builder_.CallRuntime(glue, index, Gate::InvalidGateRef, args, hirGate);
        return result;
    } else {
        const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(CallRuntime));
        GateRef target = builder_.IntPtr(index);
        GateRef result = builder_.Call(cs, glue, target, acc_.GetDependRoot(), args, hirGate);
        return result;
    }
}

GateRef NativeInlineLowering::NativeCallTS(GateRef gate, GateRef depend, const std::vector<GateRef> &args)
{
    const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(JSCall));
    GateRef target = builder_.IntPtr(RTSTUB_ID(JSCall));
    GateRef call = builder_.Call(cs, glue_, target, depend, args, gate);
    return call;
}
}  // namespace panda::ecmascript
