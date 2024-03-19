/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_ESCAPE_ANALYSIS_EDITOR_H
#define ECMASCRIPT_COMPILER_ESCAPE_ANALYSIS_EDITOR_H

#include "ecmascript/compiler/combined_pass_visitor.h"
#include "ecmascript/compiler/escape_analysis.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {

class EscapeAnalysisEditor : public PassVisitor {
public:
    EscapeAnalysisEditor(Circuit* circuit, RPOVisitor* visitor,
                         Chunk* chunk, EscapeAnalysis* result, bool isTraced)
        : PassVisitor(circuit, chunk, visitor), circuit_(circuit), result_(result), isTraced_(isTraced) {}
    
    GateRef VisitFinishAllocate(GateRef gate);
    GateRef VisitGate(GateRef gate) override;
private:
    Circuit* circuit_;
    EscapeAnalysis* result_;
    bool isTraced_;
};

}
#endif