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

#include "ecmascript/compiler/ntype_hcr_lowering.h"
#include "ecmascript/jit/jit.h"

namespace panda::ecmascript::kungfu {
GateRef NTypeHCRLowering::VisitGate(GateRef gate)
{
    GateRef glue = acc_.GetGlueFromArgList();
    auto op = acc_.GetOpCode(gate);
    switch (op) {
        case OpCode::CREATE_ARRAY: {
                Jit::JitLockHolder lock(compilationEnv_, "LowerCreateArray");
                LowerCreateArray(gate, glue);
            }
            break;
        case OpCode::CREATE_ARRAY_WITH_BUFFER: {
                Jit::JitLockHolder lock(compilationEnv_, "LowerCreateArrayWithBuffer");
                LowerCreateArrayWithBuffer(gate, glue);
            }
            break;
        case OpCode::CREATE_ARGUMENTS:
            LowerCreateArguments(gate, glue);
            break;
        case OpCode::STORE_MODULE_VAR:
            LowerStoreModuleVar(gate, glue);
            break;
        case OpCode::LD_LOCAL_MODULE_VAR:
            LowerLdLocalModuleVar(gate);
            break;
        default:
            break;
    }
    return Circuit::NullGate();
}

void NTypeHCRLowering::LowerCreateArray(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    if (acc_.GetArraySize(gate) == 0) {
        LowerCreateEmptyArray(gate, glue);
    } else {
        LowerCreateArrayWithOwn(gate, glue);
    }
}

void NTypeHCRLowering::LowerCreateEmptyArray(GateRef gate, GateRef glue)
{
    GateRef length = builder_.Int32(0);
    GateRef elements = Circuit::NullGate();
    GateRef value = acc_.GetValueIn(gate, 0);
    auto hintLength = static_cast<uint32_t>(acc_.GetConstantValue(value));
    elements = builder_.GetGlobalConstantValue(ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
    auto array = NewJSArrayLiteral(glue, gate, elements, length, hintLength);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), array);
}

void NTypeHCRLowering::LowerCreateArrayWithOwn(GateRef gate, GateRef glue)
{
    uint32_t elementsLength = acc_.GetArraySize(gate);
    GateRef length = builder_.IntPtr(elementsLength);
    GateRef elements = CreateElementsWithLength(gate, glue, elementsLength);

    auto array = NewJSArrayLiteral(glue, gate, elements, length);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), array);
}

void NTypeHCRLowering::LowerCreateArrayWithBuffer(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef cpId = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    uint32_t constPoolIndex = static_cast<uint32_t>(acc_.GetConstantValue(index));
    ArgumentAccessor argAcc(circuit_);
    GateRef frameState = GetFrameState(gate);
    GateRef jsFunc = argAcc.GetFrameArgsIn(frameState, FrameArgIdx::FUNC);
    GateRef module = builder_.GetModuleFromFunction(jsFunc);
    GateRef unsharedConstpool = argAcc.GetFrameArgsIn(frameState, FrameArgIdx::UNSHARED_CONST_POOL);
    GateRef sharedConstpool = argAcc.GetFrameArgsIn(frameState, FrameArgIdx::SHARED_CONST_POOL);
    GateRef cachedArray = builder_.GetObjectFromConstPool(glue, gate, sharedConstpool, unsharedConstpool,
                                                          module, builder_.TruncInt64ToInt32(index),
                                                          ConstPoolType::ARRAY_LITERAL);
    GateRef literialElements = builder_.GetElementsArray(cachedArray);
    uint32_t cpIdVal = static_cast<uint32_t>(acc_.GetConstantValue(cpId));
    JSTaggedValue arr = GetArrayLiteralValue(cpIdVal, constPoolIndex);
    ASSERT(!arr.IsUndefined());
    DISALLOW_GARBAGE_COLLECTION;
    JSArray *arrayHandle = JSArray::Cast(arr.GetTaggedObject());
    TaggedArray *arrayLiteral = TaggedArray::Cast(arrayHandle->GetElements());
    uint32_t literialLength = arrayLiteral->GetLength();
    uint32_t arrayLength = acc_.GetArraySize(gate);
    GateRef elements = Circuit::NullGate();
    GateRef length = Circuit::NullGate();
    if (arrayLength > literialLength) {
        elements = CreateElementsWithLength(gate, glue, arrayLength);
        for (uint32_t i = 0; i < literialLength; i++) {
            GateRef value = builder_.LoadFromTaggedArray(literialElements, i);
            builder_.StoreToTaggedArray(elements, i, value);
        }
        length = builder_.IntPtr(arrayLength);
    } else {
        elements = literialElements;
        length = builder_.IntPtr(literialLength);
    }

    auto array = NewJSArrayLiteral(glue, gate, elements, length);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), array);
}

GateRef NTypeHCRLowering::NewActualArgv(GateRef gate, GateRef glue)
{
    ArgumentAccessor argAcc(circuit_);
    auto funcIdx = static_cast<size_t>(CommonArgIdx::FUNC);
    size_t length = argAcc.ArgsCount() - funcIdx;
    GateRef array = CreateElementsWithLength(gate, glue, length);
    for (size_t i = funcIdx; i < argAcc.ArgsCount(); i++) {
        GateRef value = argAcc.ArgsAt(i);
        builder_.StoreToTaggedArray(array, i - funcIdx, value);
    }
    return array;
}

void NTypeHCRLowering::LowerCreateArguments(GateRef gate, GateRef glue)
{
    CreateArgumentsAccessor accessor = acc_.GetCreateArgumentsAccessor(gate);
    CreateArgumentsAccessor::Mode mode = accessor.GetMode();
    Environment env(gate, circuit_, &builder_);
    ArgumentAccessor argAcc(circuit_);
    GateRef frameState = GetFrameState(gate);
    GateRef actualArgc = builder_.TruncInt64ToInt32(argAcc.GetFrameArgsIn(frameState, FrameArgIdx::ACTUAL_ARGC));
    GateRef expectedArgc = builder_.Int32(methodLiteral_->GetNumArgs());
    GateRef argv = argAcc.GetFrameArgsIn(frameState, FrameArgIdx::ACTUAL_ARGV);
    DEFVALUE(actualArgv, (&builder_), VariableType::NATIVE_POINTER(), argv);
    DEFVALUE(actualArgvArray, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    GateRef startIdx = acc_.GetValueIn(gate, 0);
    GateRef check = builder_.BitAnd(builder_.Equal(actualArgc, expectedArgc),
        builder_.Equal(builder_.IntPtr(0), *actualArgv));
    Label calcActualArgv(&builder_);
    Label exit(&builder_);
    BRANCH_CIR(check, &calcActualArgv, &exit);
    builder_.Bind(&calcActualArgv);
    {
        actualArgvArray = NewActualArgv(gate, glue);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    switch (mode) {
        case CreateArgumentsAccessor::Mode::REST_ARGUMENTS: {
            GateRef newGate = builder_.CallStub(glue, gate, CommonStubCSigns::CopyRestArgs,
                { glue, *actualArgv, startIdx, actualArgc, *actualArgvArray });
            ReplaceGateWithPendingException(gate, builder_.GetState(), builder_.GetDepend(), newGate);
            break;
        }
        case CreateArgumentsAccessor::Mode::UNMAPPED_ARGUMENTS: {
            GateRef newGate = builder_.CallStub(glue, gate, CommonStubCSigns::GetUnmappedArgs,
                { glue, *actualArgv, actualArgc, *actualArgvArray });
            ReplaceGateWithPendingException(gate, builder_.GetState(), builder_.GetDepend(), newGate);
            break;
        }
        default: {
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
    }
}

GateRef NTypeHCRLowering::LoadFromConstPool(GateRef unsharedConstpool, size_t index, size_t valVecType)
{
    GateRef constPoolSize = builder_.GetLengthOfTaggedArray(unsharedConstpool);
    GateRef valVecIndex = builder_.Int32Sub(constPoolSize, builder_.Int32(valVecType));
    GateRef valVec = builder_.GetValueFromTaggedArray(unsharedConstpool, valVecIndex);
    return builder_.LoadFromTaggedArray(valVec, index);
}

GateRef NTypeHCRLowering::CreateElementsWithLength(GateRef gate, GateRef glue, size_t arrayLength)
{
    GateRef elements = Circuit::NullGate();
    GateRef length = builder_.IntPtr(arrayLength);
    if (arrayLength < MAX_TAGGED_ARRAY_LENGTH) {
        elements = NewTaggedArray(arrayLength, glue);
    } else {
        elements = LowerCallRuntime(glue, gate, RTSTUB_ID(NewTaggedArray), { builder_.Int32ToTaggedInt(length) }, true);
    }
    return elements;
}

GateRef NTypeHCRLowering::NewJSArrayLiteral(GateRef glue, GateRef gate, GateRef elements, GateRef length,
                                            uint32_t hintLength)
{
    ElementsKind kind = acc_.GetArrayMetaDataAccessor(gate).GetElementsKind();
    RegionSpaceFlag flag = RegionSpaceFlag::IN_YOUNG_SPACE;
    if (enablePgoSpace_) {
        flag = acc_.GetArrayMetaDataAccessor(gate).GetRegionSpaceFlag();
    }
    GateRef hclass = Circuit::NullGate();
    if (!Elements::IsGeneric(kind)) {
        // At define point, we use initial array class without IsPrototype set.
        auto hclassIndex = compilationEnv_->GetArrayHClassIndexMap().at(kind).first;
        hclass = builder_.GetGlobalConstantValue(hclassIndex);
    } else {
        GateRef globalEnv = builder_.GetGlobalEnv();
        hclass = builder_.GetGlobalEnvObjHClass(globalEnv, GlobalEnv::ARRAY_FUNCTION_INDEX);
    }

    JSHandle<JSFunction> arrayFunc(compilationEnv_->GetGlobalEnv()->GetArrayFunction());
    JSTaggedValue protoOrHClass = arrayFunc->GetProtoOrHClass();
    JSHClass *arrayHC = JSHClass::Cast(protoOrHClass.GetTaggedObject());
    size_t arraySize = arrayHC->GetObjectSize();
    size_t lengthAccessorOffset = arrayHC->GetInlinedPropertiesOffset(JSArray::LENGTH_INLINE_PROPERTY_INDEX);

    GateRef emptyArray = builder_.GetGlobalConstantValue(ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
    GateRef accessor = builder_.GetGlobalConstantValue(ConstantIndex::ARRAY_LENGTH_ACCESSOR);
    GateRef size = builder_.IntPtr(arrayHC->GetObjectSize());

    builder_.StartAllocate();
    GateRef array = builder_.HeapAlloc(glue, size, GateType::TaggedValue(), flag);
    // initialization
    for (size_t offset = JSArray::SIZE; offset < arraySize; offset += JSTaggedValue::TaggedTypeSize()) {
        builder_.StoreConstOffset(VariableType::INT64(), array, offset, builder_.Undefined());
    }
    builder_.StoreConstOffset(VariableType::JS_POINTER(), array, 0, hclass, MemoryAttribute::NeedBarrierAndAtomic());
    builder_.StoreConstOffset(VariableType::INT64(), array, ECMAObject::HASH_OFFSET,
                              builder_.Int64(JSTaggedValue(0).GetRawData()));
    builder_.StoreConstOffset(VariableType::JS_POINTER(), array, JSObject::PROPERTIES_OFFSET, emptyArray,
                              MemoryAttribute::NoBarrier());
    builder_.StoreConstOffset(VariableType::JS_POINTER(), array, JSObject::ELEMENTS_OFFSET, elements,
                              MemoryAttribute::NeedBarrier());
    builder_.StoreConstOffset(VariableType::INT32(), array, JSArray::LENGTH_OFFSET, length);
    if (hintLength > 0) {
        builder_.StoreConstOffset(VariableType::INT64(), array, JSArray::TRACK_INFO_OFFSET,
            builder_.Int64(JSTaggedValue(hintLength).GetRawData()));
    } else {
        builder_.StoreConstOffset(VariableType::INT64(), array, JSArray::TRACK_INFO_OFFSET, builder_.Undefined());
    }
    builder_.StoreConstOffset(VariableType::JS_POINTER(), array, lengthAccessorOffset, accessor,
                              MemoryAttribute::NeedBarrier());
    return builder_.FinishAllocate(array);
}

GateRef NTypeHCRLowering::NewTaggedArray(size_t length, GateRef glue)
{
    GateRef elementsHclass = builder_.GetGlobalConstantValue(ConstantIndex::ARRAY_CLASS_INDEX);
    GateRef elementsSize = builder_.ComputeTaggedArraySize(builder_.IntPtr(length));

    builder_.StartAllocate();
    GateRef elements = builder_.HeapAlloc(glue, elementsSize, GateType::TaggedValue(), RegionSpaceFlag::IN_YOUNG_SPACE);
    builder_.StoreConstOffset(VariableType::JS_POINTER(), elements, 0, elementsHclass,
                              MemoryAttribute::NeedBarrierAndAtomic());
    builder_.StoreConstOffset(VariableType::JS_ANY(), elements, TaggedArray::LENGTH_OFFSET,
        builder_.Int32ToTaggedInt(builder_.IntPtr(length)), MemoryAttribute::NoBarrier());
    size_t endOffset = TaggedArray::DATA_OFFSET + length * JSTaggedValue::TaggedTypeSize();
    // initialization
    for (size_t offset = TaggedArray::DATA_OFFSET; offset < endOffset; offset += JSTaggedValue::TaggedTypeSize()) {
        builder_.StoreConstOffset(VariableType::INT64(), elements, offset, builder_.Hole());
    }
    return builder_.FinishAllocate(elements);
}

GateRef NTypeHCRLowering::LowerCallRuntime(GateRef glue, GateRef hirGate, int index, const std::vector<GateRef> &args,
    bool useLabel)
{
    const std::string name = RuntimeStubCSigns::GetRTName(index);
    if (useLabel) {
        GateRef result = builder_.CallRuntime(glue, index, Gate::InvalidGateRef, args, hirGate, name.c_str());
        return result;
    } else {
        const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(CallRuntime));
        GateRef target = builder_.IntPtr(index);
        GateRef result = builder_.Call(cs, glue, target, dependEntry_, args, hirGate, name.c_str());
        return result;
    }
}

void NTypeHCRLowering::LowerStoreModuleVar(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef jsFunc = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef value = acc_.GetValueIn(gate, 2);
    GateRef moduleOffset = builder_.IntPtr(JSFunction::ECMA_MODULE_OFFSET);
    GateRef module = builder_.Load(VariableType::JS_ANY(), jsFunc, moduleOffset);
    GateRef localExportEntriesOffset = builder_.IntPtr(SourceTextModule::LOCAL_EXPORT_ENTTRIES_OFFSET);
    GateRef localExportEntries = builder_.Load(VariableType::JS_ANY(), module, localExportEntriesOffset);
    GateRef nameDictionaryOffset = builder_.IntPtr(SourceTextModule::NAME_DICTIONARY_OFFSET);
    GateRef data = builder_.Load(VariableType::JS_ANY(), module, nameDictionaryOffset);
    DEFVALUE(array, (&builder_), VariableType::JS_ANY(), data);

    Label dataIsUndefined(&builder_);
    Label exit(&builder_);
    BRANCH_CIR(builder_.TaggedIsUndefined(data), &dataIsUndefined, &exit);
    builder_.Bind(&dataIsUndefined);
    {
        GateRef size = builder_.GetLengthOfTaggedArray(localExportEntries);
        array = LowerCallRuntime(glue, gate, RTSTUB_ID(NewTaggedArray), { builder_.Int32ToTaggedInt(size) }, true);
        builder_.StoreConstOffset(VariableType::JS_ANY(), module, SourceTextModule::NAME_DICTIONARY_OFFSET, *array);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    GateRef dataOffset = builder_.Int32(TaggedArray::DATA_OFFSET);
    GateRef indexOffset = builder_.Int32Mul(index, builder_.Int32(JSTaggedValue::TaggedTypeSize()));
    GateRef offset = builder_.Int32Add(indexOffset, dataOffset);
    builder_.Store(VariableType::JS_ANY(), glue_, *array, offset, value);
    ReplaceGateWithPendingException(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void NTypeHCRLowering::LowerLdLocalModuleVar(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef jsFunc = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef moduleOffset = builder_.IntPtr(JSFunction::ECMA_MODULE_OFFSET);
    GateRef module = builder_.Load(VariableType::JS_ANY(), jsFunc, moduleOffset);
    GateRef nameDictionaryOffset = builder_.IntPtr(SourceTextModule::NAME_DICTIONARY_OFFSET);
    GateRef dictionary = builder_.Load(VariableType::JS_ANY(), module, nameDictionaryOffset);
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.Hole());
    Label dataIsNotUndefined(&builder_);
    Label exit(&builder_);
    BRANCH_CIR(builder_.TaggedIsUndefined(dictionary), &exit, &dataIsNotUndefined);
    builder_.Bind(&dataIsNotUndefined);
    {
        GateRef dataOffset = builder_.Int32(TaggedArray::DATA_OFFSET);
        GateRef indexOffset = builder_.Int32Mul(index, builder_.Int32(JSTaggedValue::TaggedTypeSize()));
        GateRef offset = builder_.Int32Add(indexOffset, dataOffset);
        result = builder_.Load(VariableType::JS_ANY(), dictionary, offset);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    ReplaceGateWithPendingException(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void NTypeHCRLowering::ReplaceGateWithPendingException(GateRef gate, GateRef state, GateRef depend, GateRef value)
{
    auto condition = builder_.HasPendingException(glue_);
    GateRef ifBranch = builder_.Branch(state, condition, 1, BranchWeight::DEOPT_WEIGHT, "checkException");
    GateRef ifTrue = builder_.IfTrue(ifBranch);
    GateRef ifFalse = builder_.IfFalse(ifBranch);
    GateRef eDepend = builder_.DependRelay(ifTrue, depend);
    GateRef sDepend = builder_.DependRelay(ifFalse, depend);

    StateDepend success(ifFalse, sDepend);
    StateDepend exception(ifTrue, eDepend);
    acc_.ReplaceHirWithIfBranch(gate, success, exception, value);
}
}
