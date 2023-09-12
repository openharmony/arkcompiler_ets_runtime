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
#include "ecmascript/compiler/type.h"
#include "ecmascript/mem/chunk.h"
#include "ecmascript/mem/chunk_containers.h"

#include "ecmascript/elements.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"
#include "libpandabase/macros.h"

#include "ecmascript/compiler/share_gate_meta_data.h"

namespace panda::ecmascript::kungfu {

class JSBytecodeMetaData : public GateMetaData {
public:
    explicit JSBytecodeMetaData(size_t valuesIn, EcmaOpcode opcode, uint32_t pcOffset, GateFlags flags)
        : GateMetaData(OpCode::JS_BYTECODE, flags, 1, 1, valuesIn),
        opcode_(opcode), pcOffset_(pcOffset)
    {
        SetKind(GateMetaData::Kind::JSBYTECODE);
    }

    static const JSBytecodeMetaData* Cast(const GateMetaData* meta)
    {
        meta->AssertKind(GateMetaData::Kind::JSBYTECODE);
        return static_cast<const JSBytecodeMetaData*>(meta);
    }

    uint32_t GetPcOffset() const
    {
        return pcOffset_;
    }

    void SetType(PGOSampleType type)
    {
        type_ = type;
    }

    PGOSampleType GetType() const
    {
        return type_;
    }

    EcmaOpcode GetByteCodeOpcode() const
    {
        return opcode_;
    }

    void SetElementsKind(ElementsKind kind)
    {
        elementsKind_ = kind;
        elementsKinds_.emplace_back(kind);
    }

    ElementsKind GetElementsKind() const
    {
        return elementsKind_;
    }

    std::vector<ElementsKind> GetElementsKinds() const
    {
        return elementsKinds_;
    }

private:
    EcmaOpcode opcode_;
    uint32_t pcOffset_;
    PGOSampleType type_;
    ElementsKind elementsKind_ {ElementsKind::GENERIC};
    std::vector<ElementsKind> elementsKinds_ {};
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