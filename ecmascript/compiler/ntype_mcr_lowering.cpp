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

#include "ecmascript/compiler/ntype_mcr_lowering.h"
#include "ecmascript/dfx/vmstat/opt_code_profiler.h"
#include "ecmascript/compiler/new_object_stub_builder.h"

namespace panda::ecmascript::kungfu {

GateRef NTypeMCRLowering::VisitGate(GateRef gate)
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
        default:
            break;
    }
    return Circuit::NullGate();
}

void NTypeMCRLowering::LowerCreateArray(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    if (acc_.GetArraySize(gate) == 0) {
        LowerCreateEmptyArray(gate);
    } else {
        LowerCreateArrayWithOwn(gate, glue);
    }
}

void NTypeMCRLowering::LowerCreateEmptyArray(GateRef gate)
{
    GateRef length = builder_.Int32(0);
    GateRef elements = builder_.GetGlobalConstantValue(ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);

    auto array = NewJSArrayLiteral(gate, elements, length);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), array);
}

void NTypeMCRLowering::LowerCreateArrayWithOwn(GateRef gate, GateRef glue)
{
    uint32_t elementsLength = acc_.GetArraySize(gate);
    GateRef length = builder_.IntPtr(elementsLength);
    GateRef elements = CreateElementsWithLength(gate, glue, elementsLength);

    auto array = NewJSArrayLiteral(gate, elements, length);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), array);
}

void NTypeMCRLowering::LowerCreateArrayWithBuffer(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef index = acc_.GetValueIn(gate, 0);
    GateRef aotElmIndex = acc_.GetValueIn(gate, 1);
    auto elementIndex = acc_.GetConstantValue(aotElmIndex);
    uint32_t constPoolIndex = static_cast<uint32_t>(acc_.GetConstantValue(index));
    ArgumentAccessor argAcc(circuit_);
    GateRef frameState = GetFrameState(gate);
    GateRef jsFunc = argAcc.GetFrameArgsIn(frameState, FrameArgIdx::FUNC);
    GateRef literialElements = LoadFromConstPool(jsFunc, elementIndex);
    auto thread = tsManager_->GetEcmaVM()->GetJSThread();
    JSHandle<ConstantPool> constpoolHandle(tsManager_->GetConstantPool());
    JSTaggedValue arr = ConstantPool::GetLiteralFromCache<ConstPoolType::ARRAY_LITERAL>(
        thread, constpoolHandle.GetTaggedValue(), constPoolIndex, recordName_);
    JSHandle<JSArray> arrayHandle(thread, arr);
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

GateRef NTypeMCRLowering::LoadFromConstPool(GateRef jsFunc, size_t index)
{
    GateRef constPool = builder_.GetConstPool(jsFunc);
    return builder_.LoadFromTaggedArray(constPool, index);
}

GateRef NTypeMCRLowering::CreateElementsWithLength(GateRef gate, GateRef glue, size_t arrayLength)
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

GateRef NTypeMCRLowering::NewJSArrayLiteral(GateRef gate, GateRef elements, GateRef length)
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
    builder_.StoreConstOffset(VariableType::JS_POINTER(), array, lengthAccessorOffset, accessor);
    builder_.FinishAllocate();
    return array;
}

GateRef NTypeMCRLowering::NewTaggedArray(size_t length)
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
    builder_.FinishAllocate();

    return elements;
}

GateRef NTypeMCRLowering::LowerCallRuntime(GateRef glue, GateRef hirGate, int index, const std::vector<GateRef> &args,
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
}
