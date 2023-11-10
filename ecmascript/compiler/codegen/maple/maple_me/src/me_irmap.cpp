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

#include "me_irmap.h"

namespace maple {
void MeIRMap::Dump()
{
    // back up mempool and use a new mempool every time
    // we dump IRMap, restore the mempool afterwards
    MIRFunction *mirFunction = func.GetMirFunc();
    MemPool *backup = mirFunction->GetCodeMempool();
    mirFunction->SetMemPool(new ThreadLocalMemPool(memPoolCtrler, "IR Dump"));
    auto theCFG = func.GetCfg();
    LogInfo::MapleLogger() << "===================Me IR dump==================\n";
    auto eIt = theCFG->valid_end();
    for (auto bIt = theCFG->valid_begin(); bIt != eIt; ++bIt) {
        auto *bb = *bIt;
        bb->DumpHeader(&GetMIRModule());
        LogInfo::MapleLogger() << "frequency : " << bb->GetFrequency() << "\n";
        bb->DumpMeVarPiList(this);
        bb->DumpMePhiList(this);
        int i = 0;
        for (auto &meStmt : bb->GetMeStmts()) {
            if (GetDumpStmtNum()) {
                LogInfo::MapleLogger() << "(" << i++ << ") ";
            }
            if (meStmt.GetOp() != OP_piassign) {
                meStmt.EmitStmt(GetSSATab()).Dump(0);
            }
            meStmt.Dump(this);
        }
    }
    mirFunction->ReleaseCodeMemory();
    mirFunction->SetMemPool(backup);
}
}  // namespace maple
