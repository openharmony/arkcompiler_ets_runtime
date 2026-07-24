/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/arksteed/arksteed_deopt_helper.h"

#include <algorithm>
#include <cstring>

#include "ecmascript/arksteed/arksteed_assembler.h"
#include "ecmascript/arksteed/arksteed_opcode.h"
#include "ecmascript/arksteed/arksteed_safepoint_table.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/frames.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/machine_code.h"

namespace panda::ecmascript::arksteed {

int64_t GetFloat64RawBits(double value)
{
    int64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

int64_t GetConstantSourceForArkSteedDeoptTranslation(const ValueVertex *value, ArkSteedDeoptValueKind valueKind)
{
    switch (value->GetOpcode()) {
        case VertexOpcode::TaggedConstant:
            ASSERT(valueKind == ArkSteedDeoptValueKind::TAGGED);
            return static_cast<int64_t>(value->Cast<TaggedConstantVertex>()->GetValue());
        case VertexOpcode::Int32Constant: {
            int32_t constant = value->Cast<Int32ConstantVertex>()->GetValue();
            if (valueKind == ArkSteedDeoptValueKind::RAW_INT32 ||
                valueKind == ArkSteedDeoptValueKind::INT32_TO_TAGGED) {
                return constant;
            }
            ASSERT(valueKind == ArkSteedDeoptValueKind::TAGGED);
            return static_cast<int64_t>(JSTaggedValue(constant).GetRawData());
        }
        case VertexOpcode::Int64Constant: {
            int64_t constant = value->Cast<Int64ConstantVertex>()->GetValue();
            if (valueKind == ArkSteedDeoptValueKind::RAW_INT32 ||
                valueKind == ArkSteedDeoptValueKind::INT32_TO_TAGGED) {
                return constant;
            }
            ASSERT(valueKind == ArkSteedDeoptValueKind::TAGGED);
            return static_cast<int64_t>(JSTaggedValue(static_cast<int>(constant)).GetRawData());
        }
        case VertexOpcode::Float64Constant:
            ASSERT(valueKind == ArkSteedDeoptValueKind::FLOAT64_TO_TAGGED_DOUBLE);
            return GetFloat64RawBits(value->Cast<Float64ConstantVertex>()->GetValue());
        default:
            UNREACHABLE();
    }
}

ArkSteedDeoptTranslationInput BuildArkSteedDeoptTranslationInput(ArkSteedAssembler *assembler,
                                                                 const EagerDeoptimizableMixin *vertex, uint32_t index)
{
    ArkSteedDeoptTranslationInput input {
        vertex->GetDeoptVReg(index),
        vertex->GetDeoptValueKind(index),
        ArkSteedDeoptSourceKind::CONSTANT,
        0,
    };
    const InstructionOperand &operand = vertex->GetDeoptSourceLocation(index)->GetOperand();
    if (operand.IsConstant()) {
        input.source = GetConstantSourceForArkSteedDeoptTranslation(vertex->GetDeoptFrameValue(index), input.valueKind);
        return input;
    }

    ASSERT(operand.IsAllocated());
    auto location = AllocatedState::Cast(operand);
    if (location.IsRegister()) {
        input.sourceKind = ArkSteedDeoptSourceKind::GP_REGISTER;
        input.source = location.GetRegister().Code();
        return input;
    }
    if (location.IsDoubleRegister()) {
        input.sourceKind = ArkSteedDeoptSourceKind::FP_REGISTER;
        input.source = location.GetDoubleRegister().Code();
        return input;
    }

    ASSERT(location.IsAnyStackSlot());
    input.sourceKind = ArkSteedDeoptSourceKind::STACK_SLOT;
    input.source = assembler->GetFramePointerOffsetForStackSlot(location.GetIndex(), location.GetRepresentation());
    return input;
}

std::vector<ArkSteedDeoptTranslationInput> BuildArkSteedDeoptTranslationInputs(ArkSteedAssembler *assembler,
                                                                               const EagerDeoptimizableMixin *vertex)
{
    std::vector<ArkSteedDeoptTranslationInput> inputs;
    inputs.reserve(vertex->GetDeoptFrameValueCount() + 1);
    inputs.push_back({
        static_cast<int32_t>(SpecVregIndex::INLINE_DEPTH),
        ArkSteedDeoptValueKind::RAW_INT32,
        ArkSteedDeoptSourceKind::CONSTANT,
        0,
    });
    for (uint32_t index = 0; index < vertex->GetDeoptFrameValueCount(); ++index) {
        inputs.push_back(BuildArkSteedDeoptTranslationInput(assembler, vertex, index));
    }
    std::sort(inputs.begin(), inputs.end(),
              [](const ArkSteedDeoptTranslationInput &lhs, const ArkSteedDeoptTranslationInput &rhs) {
                  return lhs.vreg < rhs.vreg;
              });
    return inputs;
}

uint32_t GetTaggedDeoptSnapshotGeneralRegisters(const std::vector<ArkSteedDeoptTranslationInput> &inputs)
{
    uint32_t taggedRegisters = 0;
    for (const auto &input : inputs) {
        if (input.valueKind != ArkSteedDeoptValueKind::TAGGED ||
            input.sourceKind != ArkSteedDeoptSourceKind::GP_REGISTER) {
            continue;
        }
        ASSERT(input.source >= 0);
        uint32_t registerCode = static_cast<uint32_t>(input.source);
        ASSERT(GetArkSteedDeoptGeneralSnapshotOffset(registerCode) >= 0);
        taggedRegisters |= 1U << registerCode;
    }
    return taggedRegisters;
}

void CollectUsedDeoptSnapshotRegisters(const std::vector<ArkSteedDeoptTranslationInput> &inputs,
                                       ArkSteedRegList *generalRegisters, ArkDoubleRegList *floatingRegisters)
{
    for (const auto &input : inputs) {
        if (input.sourceKind == ArkSteedDeoptSourceKind::GP_REGISTER) {
            ASSERT(input.source >= 0);
            uint32_t registerCode = static_cast<uint32_t>(input.source);
            ASSERT(GetArkSteedDeoptGeneralSnapshotOffset(registerCode) >= 0);
            ArkSteedRegister reg = ArkSteedRegister::FromCode(registerCode);
            ASSERT(GetAllocatableGeneralRegisters().Has(reg));
            generalRegisters->Set(reg);
        } else if (input.sourceKind == ArkSteedDeoptSourceKind::FP_REGISTER) {
            ASSERT(input.source >= 0);
            uint32_t registerCode = static_cast<uint32_t>(input.source);
            ASSERT(GetArkSteedDeoptFloatingSnapshotOffset(registerCode) >= 0);
            ArkSteedDoubleRegister reg = ArkSteedDoubleRegister::FromCode(registerCode);
            ASSERT(GetAllocatableDoubleRegisters().Has(reg));
            floatingRegisters->Set(reg);
        }
    }
}

namespace {
using MaterializedVreg = std::pair<Deoptimizier::VRegId, JSTaggedType>;

struct MaterializedArkSteedDeoptFrame {
    size_t inlineDepth {0};
    bool hasInlineDepth {false};
    std::vector<MaterializedVreg> values;
};

bool IsNonSteedOptimizedFrame(FrameType type)
{
    return type == FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME || type == FrameType::OPTIMIZED_JS_FUNCTION_FRAME ||
           type == FrameType::FASTJIT_FUNCTION_FRAME || type == FrameType::FASTJIT_FAST_CALL_FUNCTION_FRAME;
}

bool ReadTranslationFromSteedFrame(const FrameIterator &it, ArkSteedDeoptId deoptId,
                                   ArkSteedDeoptTranslation *translation)
{
    JSTaggedValue machineCodeValue(*it.GetMachineCodeSlot());
    if (!machineCodeValue.IsMachineCodeObject()) {
        return false;
    }

    const MachineCode *machineCode = MachineCode::Cast(machineCodeValue.GetTaggedObject());
    ArkSteedSafepointTable safepointTable(machineCode->GetStackMapOrOffsetTableAddress(),
                                          machineCode->GetStackMapOrOffsetTableSize());
    if (!safepointTable.IsValid()) {
        return false;
    }
    return safepointTable.GetArkSteedDeoptTranslation(deoptId, translation);
}

uint64_t ReadDeoptInputRaw(const ArkSteedDeoptTranslationInput &input, uintptr_t callsiteFp, uintptr_t snapshot)
{
    switch (input.sourceKind) {
        case ArkSteedDeoptSourceKind::CONSTANT:
            return static_cast<uint64_t>(input.source);
        case ArkSteedDeoptSourceKind::STACK_SLOT: {
            uintptr_t addr = callsiteFp + static_cast<intptr_t>(input.source);
            if (input.valueKind == ArkSteedDeoptValueKind::RAW_INT32 ||
                input.valueKind == ArkSteedDeoptValueKind::INT32_TO_TAGGED) {
                return static_cast<uint64_t>(*reinterpret_cast<int32_t *>(addr));
            }
            if (input.valueKind == ArkSteedDeoptValueKind::FLOAT64_TO_TAGGED_DOUBLE) {
                return *reinterpret_cast<uint64_t *>(addr);
            }
            return *reinterpret_cast<JSTaggedType *>(addr);
        }
        case ArkSteedDeoptSourceKind::GP_REGISTER:
            return ReadArkSteedDeoptGeneralRegister(snapshot, static_cast<uint32_t>(input.source));
        case ArkSteedDeoptSourceKind::FP_REGISTER:
            return ReadArkSteedDeoptFloatingRegisterBits(snapshot, static_cast<uint32_t>(input.source));
    }
    UNREACHABLE();
}

JSTaggedType MaterializeDeoptInput(const ArkSteedDeoptTranslationInput &input, uintptr_t callsiteFp, uintptr_t snapshot)
{
    uint64_t raw = ReadDeoptInputRaw(input, callsiteFp, snapshot);
    switch (input.valueKind) {
        case ArkSteedDeoptValueKind::TAGGED:
            return static_cast<JSTaggedType>(raw);
        case ArkSteedDeoptValueKind::RAW_INT32:
            return static_cast<JSTaggedType>(static_cast<int32_t>(raw));
        case ArkSteedDeoptValueKind::INT32_TO_TAGGED:
            return JSTaggedValue(static_cast<int32_t>(raw)).GetRawData();
        case ArkSteedDeoptValueKind::FLOAT64_TO_TAGGED_DOUBLE: {
            if (raw >= static_cast<uint64_t>(JSTaggedValue::TAG_INT - JSTaggedValue::DOUBLE_ENCODE_OFFSET)) {
                return JSTaggedValue(base::NAN_VALUE).GetRawData();
            }
            return static_cast<JSTaggedType>(raw + JSTaggedValue::DOUBLE_ENCODE_OFFSET);
        }
    }
    UNREACHABLE();
}

bool MaterializeTranslation(const ArkSteedDeoptTranslation &translation, uintptr_t callsiteFp, uintptr_t snapshot,
                            MaterializedArkSteedDeoptFrame *frame)
{
    ASSERT(frame != nullptr);
    frame->inlineDepth = 0;
    frame->hasInlineDepth = false;
    frame->values.clear();
    frame->values.reserve(translation.inputs.size());
    for (const auto &input : translation.inputs) {
        if (input.vreg == static_cast<int32_t>(SpecVregIndex::INLINE_DEPTH)) {
            if (frame->hasInlineDepth || input.valueKind != ArkSteedDeoptValueKind::RAW_INT32) {
                return false;
            }
            JSTaggedType value = MaterializeDeoptInput(input, callsiteFp, snapshot);
            int32_t inlineDepth = static_cast<int32_t>(value);
            if (inlineDepth < 0) {
                return false;
            }
            frame->inlineDepth = static_cast<size_t>(inlineDepth);
            frame->hasInlineDepth = true;
            continue;
        }
        JSTaggedType value = MaterializeDeoptInput(input, callsiteFp, snapshot);
        frame->values.emplace_back(static_cast<Deoptimizier::VRegId>(input.vreg), value);
    }
    return frame->hasInlineDepth;
}
}  // namespace

bool HandleArkSteedDeopt(JSThread *thread, ArkSteedDeoptId deoptId, JSTaggedType *result)
{
    if (thread == nullptr || result == nullptr) {
        return false;
    }

    JSTaggedType *asmBridgeSp = nullptr;
    JSTaggedType *lastLeave = const_cast<JSTaggedType *>(thread->GetLastLeaveFrame());
    FrameIterator it(lastLeave, thread);
    for (; !it.Done(); it.Advance<GCVisitedFlag::DEOPT>()) {
        FrameType frameType = it.GetFrameType();
        if (IsNonSteedOptimizedFrame(frameType)) {
            return false;
        }
        switch (frameType) {
            case FrameType::ASM_BRIDGE_FRAME:
                asmBridgeSp = it.GetSp();
                break;
            case FrameType::OPTIMIZED_FRAME:
            case FrameType::LEAVE_FRAME:
                break;
            case FrameType::STEED_FUNCTION_FRAME: {
                if (asmBridgeSp == nullptr) {
                    return false;
                }
                ArkSteedDeoptTranslation translation {};
                if (!ReadTranslationFromSteedFrame(it, deoptId, &translation)) {
                    return false;
                }

                uintptr_t callsiteFp = reinterpret_cast<uintptr_t>(it.GetSp());
                uintptr_t snapshot = GetArkSteedDeoptSnapshotFromCallsiteSp(it.GetCallSiteSp());
                MaterializedArkSteedDeoptFrame materialized;
                if (!MaterializeTranslation(translation, callsiteFp, snapshot, &materialized)) {
                    return false;
                }

                it.SetDeoptType(static_cast<uint32_t>(translation.type));
                Deoptimizier deopt(thread, materialized.inlineDepth, translation.type);
                auto frame = it.GetFrame<SteedFunctionFrame>();
                deopt.CollectSteedDeoptContext(it, frame, asmBridgeSp);
                deopt.CollectMaterializedVregs(materialized.values,
                                               Deoptimizier::ComputeShift(materialized.inlineDepth));
                deopt.UpdateAndDumpDeoptInfo(translation.type);
                JSHandle<JSTaggedValue> undefined(thread, JSTaggedValue::Undefined());
                *result = deopt.ConstructAsmInterpretFrame(undefined);
                return true;
            }
            default:
                return false;
        }
    }
    return false;
}

}  // namespace panda::ecmascript::arksteed
