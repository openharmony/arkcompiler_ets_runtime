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

#include "ecmascript/compiler/type_inference/global_type_infer.h"

namespace panda::ecmascript::kungfu {
GlobalTypeInfer::GlobalTypeInfer(PassContext *ctx, const uint32_t methodOffset, const CString &recordName,
                                 PGOProfilerDecoder *decoder, bool enableOptTrackField, bool enableLog)
    : ctx_(ctx), jsPandaFile_(ctx_->GetJSPandaFile()), bcInfo_(ctx->GetBytecodeInfo()), methodOffset_(methodOffset),
      recordName_(recordName), decoder_(decoder), enableOptTrackField_(enableOptTrackField), enableLog_(enableLog),
      enableGlobalTypeInfer_(ctx_->GetTSManager()->GetEcmaVM()->GetJSOptions().IsEnableGlobalTypeInfer())
{
    CollectNamespaceMethods(methodOffset);
    for (const auto namespaceMethod : namespaceTypes_) {
        NewTypeInfer(namespaceMethod);
    }
}

GlobalTypeInfer::~GlobalTypeInfer()
{
    for (auto it : typeInfers_) {
        if (it.second != nullptr) {
            delete it.second;
        }
    }
    typeInfers_.clear();

    for (auto it : builders_) {
        if (it != nullptr) {
            delete it;
        }
    }
    builders_.clear();

    for (auto it:  circuits_) {
        if (it != nullptr) {
            delete it;
        }
    }
    circuits_.clear();
}

void GlobalTypeInfer::NewTypeInfer(const uint32_t methodOffset)
{
    auto module = ctx_->GetAOTModule();
    auto &methodList = bcInfo_.GetMethodList();
    MethodInfo &methodInfo = methodList.at(methodOffset);
    const auto &methodPcInfos = bcInfo_.GetMethodPcInfos();
    auto &methodPcInfo = methodPcInfos[methodInfo.GetMethodPcInfoIndex()];
    auto methodLiteral = jsPandaFile_->FindMethodLiteral(methodOffset);
    std::string fullName = module->GetFuncName(methodLiteral, jsPandaFile_);

    Circuit *circuit =
        new Circuit(ctx_->GetNativeAreaAllocator(), module->GetDebugInfo(),
                    fullName.c_str(), ctx_->GetCompilerConfig()->Is64Bit());
    circuit->SetFrameType(FrameType::OPTIMIZED_JS_FUNCTION_FRAME);
    circuits_.emplace_back(circuit);

    BytecodeCircuitBuilder *builder =
        new BytecodeCircuitBuilder(jsPandaFile_, methodLiteral, methodPcInfo, ctx_->GetTSManager(), circuit,
                                   ctx_->GetByteCodes(), jsPandaFile_->HasTSTypes(recordName_), enableLog_, true,
                                   fullName, recordName_, decoder_, false, enableOptTrackField_);
    builder->BytecodeToCircuit();
    builders_.emplace_back(builder);

    MethodTypeInfer *typeInfer = new MethodTypeInfer(builder, circuit, ctx_, methodInfo.GetMethodInfoIndex(),
                                                     enableLog_, fullName, recordName_, &methodInfo,
                                                     methodLiteral, enableGlobalTypeInfer_);
    typeInfers_.insert(std::make_pair(methodOffset, typeInfer));
}

void GlobalTypeInfer::CollectNamespaceMethod(const uint32_t methodOffset)
{
    auto &methodList = bcInfo_.GetMethodList();
    MethodInfo &methodInfo = methodList.at(methodOffset);
    auto &innerMethods = methodInfo.GetInnerMethods();
    uint32_t length = innerMethods.size();
    for (uint32_t i = 0; i < length; i++) {
        MethodInfo &innerMethodInfo = methodList.at(innerMethods[i]);
        if (innerMethodInfo.IsNamespace()) {
            namespaceTypes_.insert(innerMethods[i]);
            CollectNamespaceMethod(innerMethods[i]);
        }
    }
}

void GlobalTypeInfer::CollectNamespaceMethods(const uint32_t methodOffset)
{
    if (!enableGlobalTypeInfer_) {
        namespaceTypes_.clear();
        return;
    }
    CollectNamespaceMethod(methodOffset);
    if (namespaceTypes_.size() > MAX_GLOBAL_INFER_ALLOWED) {
        namespaceTypes_.clear();
    }
}

void GlobalTypeInfer::ProcessTypeInference(BytecodeCircuitBuilder *builder, Circuit *circuit)
{
    auto &methodList = bcInfo_.GetMethodList();
    MethodInfo &methodInfo = methodList.at(methodOffset_);
    MethodTypeInfer typeInfer(builder, circuit, ctx_, methodInfo.GetMethodInfoIndex(), enableLog_,
                              builder->GetMethodName(), recordName_, &methodInfo,
                              jsPandaFile_->FindMethodLiteral(methodOffset_),
                              enableGlobalTypeInfer_);

    TSManager *tsManager = ctx_->GetTSManager();
    std::stack<MethodTypeInfer *> typeInferStack;
    typeInferStack.push(&typeInfer);
    // In TS, a namespace declaration is only allowed at the top level of a namespace or module and
    // it can be thought of as a formalization of the IIFE pattern. Based on the function definition
    // domain chain, there will be no loops containing namespace declarations.
    // This shows that the following stack will definitely stop.
    while (!typeInferStack.empty()) {
        MethodTypeInfer *infer = typeInferStack.top();
        if (infer == nullptr) {
            typeInferStack.pop();
            continue;
        }
        uint32_t methodId = 0;
        GateType type;
        std::tie(type, methodId) = infer->TraverseInfer();
        if (IsLegalMethod(methodId)) {
            MethodTypeInfer *nextInfer = GetTypeInfer(methodId);
            if (nextInfer != nullptr) {
                nextInfer->SetNamespaceArgType(type);
                tsManager->StoreNamespaceType(methodId, type);
                typeInferStack.push(nextInfer);
                continue;
            }
        }
        typeInferStack.pop();
    }

    typeInfer.CheckAndPrint();
    RunTypeCheck();
}

void GlobalTypeInfer::RunTypeCheck()
{
    for (auto it : typeInfers_) {
        if (it.second != nullptr) {
            it.second->CheckAndPrint();
        }
    }
}
}  // namespace panda::ecmascript
