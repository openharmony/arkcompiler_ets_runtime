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

#include "ecmascript/compiler/codegen/maple/litecg_codegen.h"
#if defined(PANDA_TARGET_MACOS) || defined(PANDA_TARGET_IOS)
#include "ecmascript/base/llvm_helper.h"
#endif

#include <cstring>
#include <iomanip>
#include <vector>

#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/compiler/compiler_log.h"
#include "ecmascript/compiler/debug_info.h"
#include "ecmascript/compiler/litecg_ir_builder.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/stackmap/litecg_stackmap_type.h"
#include "ecmascript/stackmap/llvm_stackmap_parser.h"
#include "ecmascript/compiler/codegen/maple/maple_be/include/litecg/lmir_builder.h"
#include "ecmascript/compiler/codegen/maple/maple_be/include/litecg/litecg.h"

namespace panda::ecmascript::kungfu {
class CompilerLog;

using namespace panda::ecmascript;

LiteCGAssembler::LiteCGAssembler(LMIRModule &module) : lmirModule(module) {}

static uint8_t *AllocateCodeSection(void *object, uint32_t size, [[maybe_unused]] uint32_t alignment,
                                    const std::string &sectionName)
{
    struct CodeInfo &state = *static_cast<struct CodeInfo *>(object);
    return state.AllocaCodeSection(size, sectionName.c_str());
}

static void SaveFunc2Addr(void *object, std::string funcName, uint32_t address)
{
    struct CodeInfo &state = *static_cast<struct CodeInfo *>(object);
    state.SaveFunc2Addr(funcName, address);
}

static void SaveFunc2FPtoPrevSPDelta(void *object, std::string funcName, int32_t fp2PrevSpDelta)
{
    struct CodeInfo &state = *static_cast<struct CodeInfo *>(object);
    state.SaveFunc2FPtoPrevSPDelta(funcName, fp2PrevSpDelta);
}

static void SaveFunc2CalleeOffsetInfo(void *object, std::string funcName, kungfu::CalleeRegAndOffsetVec calleeRegInfo)
{
    struct CodeInfo &state = *static_cast<struct CodeInfo *>(object);
    state.SaveFunc2CalleeOffsetInfo(funcName, calleeRegInfo);
}

static void SavePC2DeoptInfo(void *object, uint64_t pc, std::vector<uint64_t> deoptInfo)
{
    struct CodeInfo &state = *static_cast<struct CodeInfo *>(object);
    state.SavePC2DeoptInfo(pc, deoptInfo);
}

void SavePC2CallSiteInfo(void *object, uint64_t pc, std::vector<uint64_t> callSiteInfo)
{
    struct CodeInfo &state = *static_cast<struct CodeInfo *>(object);
    state.SavePC2CallSiteInfo(pc, callSiteInfo);
}

void LiteCGAssembler::Run(const CompilerLog &log, [[maybe_unused]] bool fastCompileMode)
{
    maple::litecg::LiteCG liteCG(*lmirModule.GetModule());
    if (log.OutputLLIR()) {
        std::string irFileName = lmirModule.GetModule()->GetFileName() + ".mpl";
        liteCG.DumpIRToFile(irFileName);
    }
    liteCG.SetupLiteCGEmitMemoryManager(&codeInfo_, AllocateCodeSection, SaveFunc2Addr, SaveFunc2FPtoPrevSPDelta,
                                        SaveFunc2CalleeOffsetInfo, SavePC2DeoptInfo, SavePC2CallSiteInfo);
    liteCG.DoCG();
}

void LiteCGIRGeneratorImpl::GenerateCodeForStub(Circuit *circuit, const ControlFlowGraph &graph, size_t index,
                                                const CompilationConfig *cfg)
{
    (void)circuit;
    (void)graph;
    (void)index;
    (void)cfg;
}

void LiteCGIRGeneratorImpl::GenerateCode(Circuit *circuit, const ControlFlowGraph &graph, const CompilationConfig *cfg,
                                         const panda::ecmascript::MethodLiteral *methodLiteral,
                                         const JSPandaFile *jsPandaFile, const std::string &methodName,
                                         bool enableOptInlining, [[maybe_unused]]bool enableBranchProfiling)
{
    circuit->SetFrameType(FrameType::OPTIMIZED_JS_FUNCTION_FRAME);
    CallSignature::CallConv conv;
    if (methodLiteral->IsFastCall()) {
        conv = CallSignature::CallConv::CCallConv;
    } else {
        conv = CallSignature::CallConv::WebKitJSCallConv;
    }
    LiteCGIRBuilder builder(&graph, circuit, module_, cfg, conv, enableLog_, enableOptInlining, methodLiteral,
                            jsPandaFile, methodName);
    builder.Build();
}

void LiteCGAssembler::CollectAnStackMap(CGStackMapInfo &stackMapInfo)
{
    auto &liteCGStackMapInfo = static_cast<LiteCGStackMapInfo&>(stackMapInfo);
    const auto &codeInfo = GetCodeInfo();
    liteCGStackMapInfo.AppendCallSiteInfo(codeInfo.GetPC2CallsiteInfo());
    liteCGStackMapInfo.AppendDeoptInfo(codeInfo.GetPC2DeoptInfo());
}
}  // namespace panda::ecmascript::kungfu
