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

#include "ecmascript/compiler/escape_analysis_editor.h"

namespace panda::ecmascript::kungfu {

GateRef EscapeAnalysisEditor::VisitFinishAllocate(GateRef gate)
{
    GateRef depend = acc_.GetDep(gate);
    if (acc_.GetOpCode(gate) == OpCode::START_ALLOCATE) {
        visitor_->RelaxStateAndDepend(gate);
        visitor_->RelaxStateAndDepend(depend);
    }
    return Circuit::NullGate();
}

GateRef EscapeAnalysisEditor::VisitGate(GateRef gate)
{
    GateRef replacement = result_->TryGetReplacement(gate);
    if (replacement != Circuit::NullGate()) {
        if (replacement == circuit_->DeadGate()) {
            visitor_->RelaxStateAndDepend(gate);
            return replacement;
        }
        if (isTraced_) {
            LOG_COMPILER(INFO) << "[escape analysis editor] replace " << acc_.GetId(gate) <<
                                  " with " << acc_.GetId(replacement);
        }
        return replacement;
    }
    auto opcode = acc_.GetOpCode(gate);
    switch (opcode) {
        case OpCode::FINISH_ALLOCATE:
            return VisitFinishAllocate(gate);
        default:
            break;
    }
    return Circuit::NullGate();
}

}