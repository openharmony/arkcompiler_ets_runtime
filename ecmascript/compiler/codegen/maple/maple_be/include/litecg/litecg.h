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

#ifndef MAPLE_LITECG_LITECG_H
#define MAPLE_LITECG_LITECG_H

#include "lmir_builder.h"
#include "cg_option.h"

namespace maple {
namespace litecg {

enum OutputType {
    kAsm,  // AT&T assembly file
    kElf,  // elf object file
};

enum TargetType {
    kX86_64,
    kAarch64,
};

enum DebugType {
    kDebug,
    kNoDebug,
};

enum InfoType { kQuiet, kVerbose };

class LiteCG {
public:
    LiteCG(Module &mirModule, const std::vector<std::string> &litecgOptions);
    ~LiteCG() = default;

    // configurations API.
    // If not specified, default to: Ofastcompile, Elf, X86_64, NoDebug, Quiet

    // O0/O1/Ofastcompile

    // return LiteCG& enables chaining of config functions.
    LiteCG &SetOutputType(OutputType config);
    LiteCG &SetupLiteCGEmitMemoryManager(void *codeSpace,
                                         maplebe::MemoryManagerAllocateDataSectionCallback dataSectionAllocator,
                                         maplebe::MemoryManagerSaveFunc2AddressInfoCallback funcAddrSaver,
                                         maplebe::MemoryManagerSaveFunc2FPtoPrevSPDeltaCallback funcFpSPDeltaSaver,
                                         maplebe::MemoryManagerSaveFunc2CalleeOffsetInfoCallback funcCallOffsetSaver,
                                         maplebe::MemoryManagerSavePC2DeoptInfoCallback pc2DeoptInfoSaver,
                                         maplebe::MemoryManagerSavePC2CallSiteInfoCallback pc2CallSiteInfoSaver);
#ifdef ARK_LITECG_DEBUG
    void DumpIRToFile(const std::string &fileName);
#endif
    void DumpCGIR();
    void DoCG(bool isJit = false);
    void SetAotCodeCommentFile(const std::string &aotCodeCommentFile);

private:
    Module &module;
    maplebe::CGOptions *cgOptions;
};

}  // namespace litecg

}  // namespace maple

#endif  // MAPLE_LITECG_LITECG_H
