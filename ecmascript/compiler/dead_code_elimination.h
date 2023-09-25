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

#ifndef ECMASCRIPT_COMPILER_DEAD_CODE_ELIMINATION_H
#define ECMASCRIPT_COMPILER_DEAD_CODE_ELIMINATION_H

#include "ecmascript/compiler/combined_pass_visitor.h"

namespace panda::ecmascript::kungfu {

class DeadCodeElimination : public PassVisitor {
public:
    DeadCodeElimination(Circuit* circuit, RPOVisitor* visitor, Chunk* chunk)
        : PassVisitor(circuit, chunk, visitor)
        {
            deadGate_ = circuit->DeadGate();
        }
    GateRef VisitGate(GateRef gate) override;

private:
    GateRef StateIsDead(GateRef gate);
    GateRef EliminateMergeAndLoopBegin(GateRef gate);
    GateRef EliminateBranch(GateRef gate);
    GateRef EliminateReturn(GateRef gate);
    GateRef EliminateIfException(GateRef gate);
    GateRef EliminateLoopExit(GateRef gate);
    GateRef EliminateDependSelector(GateRef gate);
    GateRef EliminateValueSelector(GateRef gate);
    GateRef EliminateGate(GateRef gate);
    GateRef DeleteLoopExit(GateRef gate);
    void TryFindAndDeleteLoopExit(GateRef gate);
    void DecreaseAllSelectors(GateRef gate, size_t count);
    GateRef deadGate_;
};
}

#endif