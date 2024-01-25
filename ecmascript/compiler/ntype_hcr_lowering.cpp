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

#include "ecmascript/js_arguments.h"
#include "ecmascript/compiler/ntype_hcr_lowering.h"
#include "ecmascript/dfx/vmstat/opt_code_profiler.h"
#include "ecmascript/compiler/new_object_stub_builder.h"

namespace panda::ecmascript::kungfu {

GateRef NTypeHCRLowering::VisitGate(GateRef gate)
{
    GateRef glue = acc_.GetGlueFromArgList();
    auto op = acc_.GetOpCode(gate);
    switch (op) {
        case OpCode::CREATE_ARRAY:
            LowerCreateArray(gate, glue);
            break;
        case OpCode::CREATE_ARRAY_WITH_BUFFER:
            LowerCreateArrayWithBuffer(gate, glue);
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
        LowerCreateEmptyArray(gate);
    } else {
        LowerCreateArrayWithOwn(gate, glue);
    }
}

void NTypeHCRLowering::LowerCreateEmptyArray(GateRef gate)
{
    GateRef length = builder_.Int32(0);
    GateRef elements = Circuit::NullGate();
    GateRef value = acc_.GetValueIn(gate, 0);
    auto hintLength = static_cast<uint32_t>(acc_.GetConstantValue(value));
    elements = builder_.GetGlobalConstantValue(ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
    auto array = NewJSArrayLiteral(gate, elements, length, hintLength);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), array);
}

void NTypeHCRLowering::LowerCreateArrayWithOwn(GateRef gate, GateRef glue)
{
    uint32_t elementsLength = acc_.GetArraySize(gate);
    GateRef length = builder_.IntPtr(elementsLength);
    GateRef elements = CreateElementsWithLength(gate, glue, elementsLength);

    auto array = NewJSArrayLiteral(gate, elements, length);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), array);
}

void NTypeHCRLowering::LowerCreateArrayWithBuffer(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef cpId = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef aotElmIndex = acc_.GetValueIn(gate, 2);
    auto elementIndex = acc_.GetConstantValue(aotElmIndex);
    uint32_t constPoolIndex = static_cast<uint32_t>(acc_.GetConstantValue(index));
    ArgumentAccessor argAcc(circuit_);
    GateRef frameState = GetFrameState(gate);
    GateRef jsFunc = argAcc.GetFrameArgsIn(frameState, FrameArgIdx::FUNC);
    GateRef literialElements = LoadFromConstPool(jsFunc, elementIndex, ConstantPool::AOT_ARRAY_INFO_INDEX);
    uint32_t cpIdVal = static_cast<uint32_t>(acc_.GetConstantValue(cpId));
    JSTaggedValue arr = GetArrayLiteralValue(cpIdVal, constPoolIndex);
    JSHandle<JSArray> arrayHandle(thread_, arr);
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

    auto array = NewJSArrayLiteral(gate, elements, length);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), array);
}

void NTypeHCRLowering::LowerCreateArguments(GateRef gate, GateRef glue)
{
    CreateArgumentsAccessor accessor = acc_.GetCreateArgumentsAccessor(gate);
    CreateArgumentsAccessor::Mode mode = accessor.GetMode();
    Environment env(gate, circuit_, &builder_);
    ArgumentAccessor argAcc(circuit_);
    GateRef frameState = GetFrameState(gate);
    GateRef actualArgc = argAcc.GetFrameArgsIn(frameState, FrameArgIdx::ACTUAL_ARGC);
    GateRef startIdx = acc_.GetValueIn(gate, 0);

    switch (mode) {
        case CreateArgumentsAccessor::Mode::REST_ARGUMENTS: {
            GateRef newGate = builder_.CallStub(glue, gate, CommonStubCSigns::CopyRestArgs,
                { glue, startIdx, builder_.TruncInt64ToInt32(actualArgc) });
            acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), newGate);
            break;
        }
        case CreateArgumentsAccessor::Mode::UNMAPPED_ARGUMENTS: {
            GateRef newGate = builder_.CallStub(glue, gate, CommonStubCSigns::GetUnmapedArgs,
                { glue, builder_.TruncInt64ToInt32(actualArgc) });
            acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), newGate);
            break;
        }
        default: {
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
    }
}

GateRef NTypeHCRLowering::LoadFromConstPool(GateRef jsFunc, size_t index, size_t valVecType)
{
    GateRef constPool = builder_.GetConstPool(jsFunc);
    GateRef constPoolSize = builder_.GetLengthOfTaggedArray(constPool);
    GateRef valVecIndex = builder_.Int32Sub(constPoolSize, builder_.Int32(valVecType));
    GateRef valVec = builder_.GetValueFromTaggedArray(constPool, valVecIndex);
    return builder_.LoadFromTaggedArray(valVec, index);
}

GateRef NTypeHCRLowering::CreateElementsWithLength(GateRef gate, GateRef glue, size_t arrayLength)
{
    GateRef elements = Circuit::NullGate();
    GateRef length = builder_.IntPtr(arrayLength);
    if (arrayLength < MAX_TAGGED_ARRAY_LENGTH) {
        elements = NewTaggedArray(arrayLength);
    } else {
        elements = LowerCallRuntime(glue, gate, RTSTUB_ID(NewTaggedArray), { builder_.Int32ToTaggedInt(length) }, true);
    }
    return elements;
}

GateRef NTypeHCRLowering::NewJSArrayLiteral(GateRef gate, GateRef elements, GateRef length, uint32_t hintLength)
{
    ElementsKind kind = acc_.GetArrayMetaDataAccessor(gate).GetElementsKind();
    GateRef hclass = Circuit::NullGate();
    if (!Elements::IsGeneric(kind)) {
        auto thread = tsManager_->GetEcmaVM()->GetJSThread();
        auto hclassIndex = thread->GetArrayHClassIndexMap().at(kind);
        hclass = builder_.GetGlobalConstantValue(hclassIndex);
    } else {
        GateRef globalEnv = builder_.GetGlobalEnv();
        hclass = builder_.GetGlobalEnvObjHClass(globalEnv, GlobalEnv::ARRAY_FUNCTION_INDEX);
    }

    JSHandle<JSFunction> arrayFunc(tsManager_->GetEcmaVM()->GetGlobalEnv()->GetArrayFunction());
    JSTaggedValue protoOrHClass = arrayFunc->GetProtoOrHClass();
    JSHClass *arrayHC = JSHClass::Cast(protoOrHClass.GetTaggedObject());
    size_t arraySize = arrayHC->GetObjectSize();
    size_t lengthAccessorOffset = arrayHC->GetInlinedPropertiesOffset(JSArray::LENGTH_INLINE_PROPERTY_INDEX);

    GateRef emptyArray = builder_.GetGlobalConstantValue(ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
    GateRef accessor = builder_.GetGlobalConstantValue(ConstantIndex::ARRAY_LENGTH_ACCESSOR);
    GateRef size = builder_.IntPtr(arrayHC->GetObjectSize());

    builder_.StartAllocate();
    GateRef array = builder_.HeapAlloc(size, GateType::TaggedValue(), RegionSpaceFlag::IN_YOUNG_SPACE);
    // initialization
    for (size_t offset = JSArray::SIZE; offset < arraySize; offset += JSTaggedValue::TaggedTypeSize()) {
        builder_.StoreConstOffset(VariableType::INT64(), array, offset, builder_.Undefined());
    }
    builder_.StoreConstOffset(VariableType::JS_POINTER(), array, 0, hclass);
    builder_.StoreConstOffset(VariableType::INT64(), array, ECMAObject::HASH_OFFSET,
                              builder_.Int64(JSTaggedValue(0).GetRawData()));
    builder_.StoreConstOffset(VariableType::JS_POINTER(), array, JSObject::PROPERTIES_OFFSET, emptyArray);
    builder_.StoreConstOffset(VariableType::JS_POINTER(), array, JSObject::ELEMENTS_OFFSET, elements);
    builder_.StoreConstOffset(VariableType::INT32(), array, JSArray::LENGTH_OFFSET, length);
    if (hintLength > 0) {
        builder_.StoreConstOffset(VariableType::INT64(), array, JSArray::TRACK_INFO_OFFSET,
            builder_.Int64(JSTaggedValue(hintLength).GetRawData()));
    } else {
        builder_.StoreConstOffset(VariableType::INT64(), array, JSArray::TRACK_INFO_OFFSET, builder_.Undefined());
    }
    builder_.StoreConstOffset(VariableType::JS_POINTER(), array, lengthAccessorOffset, accessor);
    return builder_.FinishAllocate(array);
}

GateRef NTypeHCRLowering::NewTaggedArray(size_t length)
{
    GateRef elementsHclass = builder_.GetGlobalConstantValue(ConstantIndex::ARRAY_CLASS_INDEX);
    GateRef elementsSize = builder_.ComputeTaggedArraySize(builder_.IntPtr(length));

    builder_.StartAllocate();
    GateRef elements = builder_.HeapAlloc(elementsSize, GateType::TaggedValue(), RegionSpaceFlag::IN_YOUNG_SPACE);
    builder_.StoreConstOffset(VariableType::JS_POINTER(), elements, 0, elementsHclass);
    builder_.StoreConstOffset(VariableType::JS_ANY(), elements, TaggedArray::LENGTH_OFFSET,
        builder_.Int32ToTaggedInt(builder_.IntPtr(length)));
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
    if (useLabel) {
        GateRef result = builder_.CallRuntime(glue, index, Gate::InvalidGateRef, args, hirGate);
        return result;
    } else {
        const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(CallRuntime));
        GateRef target = builder_.IntPtr(index);
        GateRef result = builder_.Call(cs, glue, target, dependEntry_, args, hirGate);
        return result;
    }
}

void NTypeHCRLowering::LowerStoreModuleVar(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef jsFunc = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef value = acc_.GetValueIn(gate, 2);
    GateRef method = builder_.GetMethodFromFunction(jsFunc);
    GateRef moduleOffset = builder_.IntPtr(Method::ECMA_MODULE_OFFSET);
    GateRef module = builder_.Load(VariableType::JS_ANY(), method, moduleOffset);
    GateRef localExportEntriesOffset = builder_.IntPtr(SourceTextModule::LOCAL_EXPORT_ENTTRIES_OFFSET);
    GateRef localExportEntries = builder_.Load(VariableType::JS_ANY(), module, localExportEntriesOffset);
    GateRef nameDictionaryOffset = builder_.IntPtr(SourceTextModule::NAME_DICTIONARY_OFFSET);
    GateRef data = builder_.Load(VariableType::JS_ANY(), module, nameDictionaryOffset);
    DEFVALUE(array, (&builder_), VariableType::JS_ANY(), data);

    Label dataIsUndefined(&builder_);
    Label exit(&builder_);
    builder_.Branch(builder_.TaggedIsUndefined(data), &dataIsUndefined, &exit);
    builder_.Bind(&dataIsUndefined);
    {
        GateRef size = builder_.GetLengthOfTaggedArray(localExportEntries);
        Label fastpath(&builder_);
        Label slowPath(&builder_);
        Label finishNew(&builder_);
        builder_.Branch(builder_.Int32LessThan(size, builder_.Int32(MAX_TAGGED_ARRAY_LENGTH)), &fastpath, &slowPath);
        builder_.Bind(&fastpath);
        {
            array = NewTaggedArray(size);
            builder_.Jump(&finishNew);
        }
        builder_.Bind(&slowPath);
        {
            array = LowerCallRuntime(glue, gate, RTSTUB_ID(NewTaggedArray), { builder_.Int32ToTaggedInt(size) }, true);
            builder_.Jump(&finishNew);
        }
        builder_.Bind(&finishNew);
        {
            builder_.StoreConstOffset(VariableType::JS_ANY(), module, SourceTextModule::NAME_DICTIONARY_OFFSET, *array);
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&exit);
    GateRef dataOffset = builder_.Int32(TaggedArray::DATA_OFFSET);
    GateRef indexOffset = builder_.Int32Mul(index, builder_.Int32(JSTaggedValue::TaggedTypeSize()));
    GateRef offset = builder_.Int32Add(indexOffset, dataOffset);
    builder_.Store(VariableType::JS_ANY(), glue_, *array, offset, value);
    ReplaceGateWithPendingException(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

GateRef NTypeHCRLowering::NewTaggedArray(GateRef length)
{
    Label subentry(&builder_);
    builder_.SubCfgEntry(&subentry);
    Label exit(&builder_);
    DEFVALUE(array, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    GateRef hclass = builder_.GetGlobalConstantValue(ConstantIndex::ARRAY_CLASS_INDEX);
    GateRef size = builder_.ComputeTaggedArraySize(builder_.ZExtInt32ToPtr(length));
    builder_.StartAllocate();
    array = builder_.HeapAlloc(size, GateType::TaggedValue(), RegionSpaceFlag::IN_YOUNG_SPACE);
    builder_.StoreConstOffset(VariableType::JS_POINTER(), *array, 0, hclass);
    builder_.StoreConstOffset(VariableType::JS_ANY(), *array, TaggedArray::LENGTH_OFFSET,
        builder_.Int32ToTaggedInt(length));
    GateRef dataOffset = builder_.Int32(TaggedArray::DATA_OFFSET);
    GateRef offset = builder_.Int32Mul(length, builder_.Int32(JSTaggedValue::TaggedTypeSize()));
    GateRef endOffset = builder_.Int32Add(offset, builder_.Int32(TaggedArray::DATA_OFFSET));
    Label loopHead(&builder_);
    Label loopEnd(&builder_);
    DEFVALUE(startOffset, (&builder_), VariableType::INT32(), dataOffset);
    builder_.Branch(builder_.Int32UnsignedLessThan(*startOffset, endOffset), &loopHead, &exit);
    builder_.LoopBegin(&loopHead);
    builder_.Store(VariableType::INT64(), glue_, *array, *startOffset, builder_.Hole());
    startOffset = builder_.Int32Add(*startOffset, builder_.Int32(JSTaggedValue::TaggedTypeSize()));
    builder_.Branch(builder_.Int32UnsignedLessThan(*startOffset, endOffset), &loopEnd, &exit);
    builder_.Bind(&loopEnd);
    builder_.LoopEnd(&loopHead);
    builder_.Bind(&exit);
    auto ret = builder_.FinishAllocate(*array);
    builder_.SubCfgExit();
    return ret;
}

void NTypeHCRLowering::LowerLdLocalModuleVar(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef jsFunc = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef method = builder_.GetMethodFromFunction(jsFunc);
    GateRef moduleOffset = builder_.IntPtr(Method::ECMA_MODULE_OFFSET);
    GateRef module = builder_.Load(VariableType::JS_ANY(), method, moduleOffset);
    GateRef nameDictionaryOffset = builder_.IntPtr(SourceTextModule::NAME_DICTIONARY_OFFSET);
    GateRef dictionary = builder_.Load(VariableType::JS_ANY(), module, nameDictionaryOffset);
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.Hole());
    Label dataIsNotUndefined(&builder_);
    Label exit(&builder_);
    builder_.Branch(builder_.TaggedIsUndefined(dictionary), &exit, &dataIsNotUndefined);
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
    GateRef ifBranch = builder_.Branch(state, condition);
    GateRef ifTrue = builder_.IfTrue(ifBranch);
    GateRef ifFalse = builder_.IfFalse(ifBranch);
    GateRef eDepend = builder_.DependRelay(ifTrue, depend);
    GateRef sDepend = builder_.DependRelay(ifFalse, depend);

    StateDepend success(ifFalse, sDepend);
    StateDepend exception(ifTrue, eDepend);
    acc_.ReplaceHirWithIfBranch(gate, success, exception, value);
}
}
