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

#ifndef ECMASCRIPT_COMPILER_TYPE_INFERENCE_PGO_TYPE_INFER_H
#define ECMASCRIPT_COMPILER_TYPE_INFERENCE_PGO_TYPE_INFER_H

#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/gate_accessor.h"

namespace panda::ecmascript::kungfu {
struct CollectedType;
class PGOTypeInfer {
public:
    PGOTypeInfer(Circuit *circuit, BytecodeCircuitBuilder *builder, const std::string &name,
                 Chunk *chunk, bool enableLog)
        : circuit_(circuit), acc_(circuit), argAcc_(circuit), builder_(builder), methodName_(name),
          enableLog_(enableLog), profiler_(chunk) {}
    ~PGOTypeInfer() = default;

    void Run();

private:
    struct Profiler {
        struct Value {
            GateRef gate;
            GateType tsType;
            CVector<GateType> pgoTypes;
            CVector<GateType> inferTypes;
        };
        Profiler(Chunk *chunk) : datas(chunk) {}
        ChunkVector<Value> datas;
    };

    inline bool IsLogEnabled() const
    {
        return enableLog_;
    }

    inline const std::string &GetMethodName() const
    {
        return methodName_;
    }

    inline bool IsMonoTypes(const ChunkSet<GateType> &types) const
    {
        return types.size() == 1;
    }

    inline bool IsMonoNumberType(const PGORWOpType &pgoTypes) const
    {
        // "ldobjbyvalue" will collect the type of the variable inside the square brackets while pgo collecting.
        // If the type is "number", it will be marked as an "Element".
        return pgoTypes.GetCount() == 1 && pgoTypes.GetObjectInfo(0).InElement();
    }

    void RunTypeInfer(GateRef gate);
    void InferLdObjByName(GateRef gate);
    void InferStOwnByIndex(GateRef gate);
    void InferAccessObjByValue(GateRef gate);
    void InferCreateArray(GateRef gate);

    void TrySetElementsKind(GateRef gate);
    void TrySetTransitionElementsKind(GateRef gate);
    void TrySetPropKeyKind(GateRef gate, GateRef propKey);
    void TrySetOnHeapMode(GateRef gate);

    Circuit *circuit_ {nullptr};
    GateAccessor acc_;
    ArgumentAccessor argAcc_;
    BytecodeCircuitBuilder *builder_ {nullptr};
    const std::string &methodName_;
    bool enableLog_ {false};
    Profiler profiler_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TYPE_INFERENCE_PGO_TYPE_INFER_H
