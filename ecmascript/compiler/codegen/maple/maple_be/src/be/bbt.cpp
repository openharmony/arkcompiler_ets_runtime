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
#if DEBUG && defined(ARK_LITECG_DEBUG)
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
#endif
} /* namespace maplebe */