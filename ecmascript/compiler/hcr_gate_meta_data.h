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

#ifndef ECMASCRIPT_COMPILER_HCR_GATE_META_DATA_H
#define ECMASCRIPT_COMPILER_HCR_GATE_META_DATA_H

#include <string>

#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/ecma_opcode_des.h"
#include "ecmascript/compiler/type.h"
#include "ecmascript/mem/chunk.h"
#include "ecmascript/mem/chunk_containers.h"

#include "ecmascript/elements.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"
#include "ecmascript/on_heap.h"
#include "libpandabase/macros.h"

#include "ecmascript/compiler/share_gate_meta_data.h"

namespace panda::ecmascript::kungfu {

class JSBytecodeMetaData : public GateMetaData {
public:
    explicit JSBytecodeMetaData(
        size_t valuesIn, uint32_t methodId, EcmaOpcode opcode, uint32_t pcOffset, uint32_t bcIndex, GateFlags flags)
        : GateMetaData(OpCode::JS_BYTECODE, flags, 1, 1, valuesIn), methodId_(methodId), opcode_(opcode),
          pcOffset_(pcOffset), bcIndex_(bcIndex)
    {
        SetKind(GateMetaData::Kind::JSBYTECODE);
    }

    bool equal(const GateMetaData &other) const override
    {
        if (!GateMetaData::equal(other)) {
            return false;
        }
        auto cast_other = static_cast<const JSBytecodeMetaData *>(&other);
        if (opcode_ == cast_other->opcode_ &&
            pcOffset_ == cast_other->pcOffset_ && type_ == cast_other->type_ &&
            elementsKinds_ == cast_other->elementsKinds_) {
            return true;
        }
        return false;
    }

    static const JSBytecodeMetaData* Cast(const GateMetaData* meta)
    {
        meta->AssertKind(GateMetaData::Kind::JSBYTECODE);
        return static_cast<const JSBytecodeMetaData*>(meta);
    }

    uint32_t GetMethodId() const
    {
        return methodId_;
    }

    uint32_t GetPcOffset() const
    {
        return pcOffset_;
    }

    uint32_t GetBcIndex() const
    {
        return bcIndex_;
    }

    void SetType(PGOTypeRef type)
    {
        type_ = type;
    }

    PGOTypeRef GetType() const
    {
        return type_;
    }

    EcmaOpcode GetByteCodeOpcode() const
    {
        return opcode_;
    }

    void SetElementsKind(ElementsKind kind)
    {
        elementsKinds_.emplace_back(kind);
    }

    ElementsKind GetElementsKind() const
    {
        auto size = elementsKinds_.size();
        if (size == 0) {
            return ElementsKind::GENERIC;
        }
        return elementsKinds_[0];
    }

    std::vector<ElementsKind> GetElementsKinds() const
    {
        return elementsKinds_;
    }

    void SetTransitionElementsKind(ElementsKind kind)
    {
        transitionElementsKinds_.emplace_back(kind);
    }

    ElementsKind GetTransitionElementsKind() const
    {
        auto size = transitionElementsKinds_.size();
        if (size == 0) {
            return ElementsKind::GENERIC;
        }
        return transitionElementsKinds_[0];
    }

    std::vector<ElementsKind> GetTransitionElementsKinds() const
    {
        return transitionElementsKinds_;
    }

    void SetElementsLength(uint32_t length)
    {
        elementsLength_ = length;
    }

    uint32_t GetElementsLength() const
    {
        return elementsLength_;
    }

    std::string Str() const
    {
        return GetEcmaOpcodeStr(opcode_);
    }

    void SetOnHeapMode(OnHeapMode onHeapMode)
    {
        onHeapMode_ = onHeapMode;
    }

    OnHeapMode GetOnHeapMode() const
    {
        return onHeapMode_;
    }

private:
    uint32_t methodId_;
    EcmaOpcode opcode_;
    uint32_t pcOffset_;
    uint32_t bcIndex_;
    uint32_t elementsLength_ { 0 };
    PGOTypeRef type_;
    std::vector<ElementsKind> elementsKinds_ {};
    std::vector<ElementsKind> transitionElementsKinds_ {};
    OnHeapMode onHeapMode_ {OnHeapMode::NONE};
};


class FrameStateOutput {
public:
    static constexpr uint32_t INVALID_INDEX = static_cast<uint32_t>(-1);
    explicit FrameStateOutput(uint32_t value) : index_(value) {}

    static FrameStateOutput Invalid()
    {
        return FrameStateOutput(INVALID_INDEX);
    }

    bool IsInvalid() const
    {
        return index_ == INVALID_INDEX;
    }

    uint32_t GetValue() const
    {
        return index_;
    }
private:
    uint32_t index_;
};

}

#endif  // ECMASCRIPT_COMPILER_HCR_GATE_META_DATA_H
