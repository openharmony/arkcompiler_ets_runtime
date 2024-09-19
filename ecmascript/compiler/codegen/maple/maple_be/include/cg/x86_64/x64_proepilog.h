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

#ifndef MAPLEBE_INCLUDE_CG_X64_X64_PROEPILOG_H
#define MAPLEBE_INCLUDE_CG_X64_X64_PROEPILOG_H

#include "proepilog.h"

namespace maplebe {
using namespace maple;

class X64GenProEpilog : public GenProEpilog {
public:
    explicit X64GenProEpilog(CGFunc &func) : GenProEpilog(func) {}
    ~X64GenProEpilog() override = default;

    bool NeedProEpilog() override;
    void Run() override;

private:
    void GenerateProlog(BB &bb);
    void GenerateEpilog(BB &bb);
    void GenerateCalleeSavedRegs(bool isPush);
    void GeneratePushCalleeSavedRegs(RegOperand &regOpnd, MemOperand &memOpnd, uint32 regSize);
    void GeneratePopCalleeSavedRegs(RegOperand &regOpnd, MemOperand &memOpnd, uint32 regSize);
    void GeneratePushRbpInsn();
    void GenerateMovRspToRbpInsn();
    void GenerateSubFrameSizeFromRspInsn();
    void GenerateAddFrameSizeToRspInsn();
    void GeneratePopInsn();
    void GenerateRetInsn();
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_X64_X64_PROEPILOG_H */
