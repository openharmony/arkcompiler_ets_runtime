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

#include "bbt.h"
namespace maplebe {
#if DEBUG
void BBT::Dump(const MIRModule &mod) const
{
    if (IsTry()) {
        LogInfo::MapleLogger() << "Try" << '\n';
    } else if (IsEndTry()) {
        LogInfo::MapleLogger() << "EndTry" << '\n';
    } else if (IsCatch()) {
        LogInfo::MapleLogger() << "Catch" << '\n';
    } else {
        LogInfo::MapleLogger() << "Plain" << '\n';
    }
    if (firstStmt != nullptr) {
        firstStmt->Dump(0);
        LogInfo::MapleLogger() << '\n';
        if (keyStmt != nullptr) {
            keyStmt->Dump(0);
            LogInfo::MapleLogger() << '\n';
        } else {
            LogInfo::MapleLogger() << "<<No-Key-Stmt>>" << '\n';
        }
        if (lastStmt != nullptr) {
            lastStmt->Dump(0);
        }
        LogInfo::MapleLogger() << '\n';
    } else {
        LogInfo::MapleLogger() << "<<Empty>>" << '\n';
    }
}

void BBT::ValidateStmtList(StmtNode *head, StmtNode *detached)
{
    static int nStmts = 0;
    int n = 0;
    int m = 0;
    if (head == nullptr && detached == nullptr) {
        nStmts = 0;
        return;
    }
    for (StmtNode *s = head; s != nullptr; s = s->GetNext()) {
        if (s->GetNext() != nullptr) {
            CHECK_FATAL(s->GetNext()->GetPrev() == s, "make sure the prev node of s' next is s");
        }
        if (s->GetPrev() != nullptr) {
            CHECK_FATAL(s->GetPrev()->GetNext() == s, "make sure the next node of s' prev is s");
        }
        ++n;
    }
    for (StmtNode *s = detached; s != nullptr; s = s->GetNext()) {
        if (s->GetNext() != nullptr) {
            CHECK_FATAL(s->GetNext()->GetPrev() == s, "make sure the prev node of s' next is s");
        }
        if (s->GetPrev() != nullptr) {
            CHECK_FATAL(s->GetPrev()->GetNext() == s, "make sure the next node of s' prev is s");
        }
        ++m;
    }
    CHECK_FATAL(nStmts <= n + m, "make sure nStmts <= n + m");
    nStmts = n + m;
}
#endif
} /* namespace maplebe */
