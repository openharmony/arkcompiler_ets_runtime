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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_EMITTER_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_EMITTER_H

#include "asm_emit.h"

namespace maplebe {
using namespace maple;

class AArch64AsmEmitter : public AsmEmitter {
public:
    AArch64AsmEmitter(CG &cg, const std::string &asmFileName) : AsmEmitter(cg, asmFileName) {}
    ~AArch64AsmEmitter() = default;

    void EmitBBHeaderLabel(FuncEmitInfo &funcEmitInfo, const std::string &name, LabelIdx labIdx) override;
    void RecordRegInfo(FuncEmitInfo &funcEmitInfo) const;
    void Run(FuncEmitInfo &funcEmitInfo) override;
    void CloseOutput() override
    {
        if (fileStream.is_open()) {
            fileStream << outStream.str();
            fileStream.close();

            auto options = maplebe::CGOptions::GetInstance();
            options.GetLogStream() << "~~~~~~~~~~~~~~ LiteCG aarch64 disasm begin ~~~~~~~~~~~~~~\n";
            options.GetLogStream() << outStream.str();
            options.GetLogStream() << "~~~~~~~~~~~~~~ LiteCG aarch64 disasm end  ~~~~~~~~~~~~~~~\n";
        }
    }

private:
#ifdef ARK_LITECG_DEBUG
    const MapleString *currDebugComment { nullptr };
#endif

    /* cfi & dbg need target info ? */
    void EmitAArch64CfiInsn(Emitter &emitter, const Insn &insn) const;

    void EmitAArch64Insn(Emitter &emitter, Insn &insn) const;
    void EmitAdrpLdr(Emitter &emitter, const Insn &insn) const;
    void EmitCounter(Emitter &emitter, const Insn &insn) const;
    void EmitInlineAsm(Emitter &emitter, const Insn &insn) const;
    void EmitClinitTail(Emitter &emitter, const Insn &insn) const;
    void EmitLazyLoad(Emitter &emitter, const Insn &insn) const;
    void EmitAdrpLabel(Emitter &emitter, const Insn &insn) const;
    void EmitLazyLoadStatic(Emitter &emitter, const Insn &insn) const;
    void EmitArrayClassCacheLoad(Emitter &emitter, const Insn &insn) const;
    void EmitGetAndAddInt(Emitter &emitter, const Insn &insn) const;
    void EmitGetAndSetInt(Emitter &emitter, const Insn &insn) const;
    void EmitCompareAndSwapInt(Emitter &emitter, const Insn &insn) const;
    void EmitStringIndexOf(Emitter &emitter, const Insn &insn) const;
    void EmitLazyBindingRoutine(Emitter &emitter, const Insn &insn) const;
    void EmitCheckThrowPendingException(Emitter &emitter, Insn &insn) const;
    void EmitCTlsDescRel(Emitter &emitter, const Insn &insn) const;
    void EmitCTlsDescCall(Emitter &emitter, const Insn &insn) const;
    void EmitSyncLockTestSet(Emitter &emitter, const Insn &insn) const;

    bool CheckInsnRefField(const Insn &insn, size_t opndIndex) const;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_EMITTER_H */
