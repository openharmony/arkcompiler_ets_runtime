/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_COMPILATION_DRIVER_H
#define ECMASCRIPT_COMPILER_COMPILATION_DRIVER_H

#include "ecmascript/compiler/bytecode_info_collector.h"

namespace panda::ecmascript::kungfu {
class CompilationDriver {
public:
    explicit CompilationDriver(JSPandaFile *jsPandaFile, PGOProfilerLoader &profilerLoader,
                               BCInfo &bytecodeInfo)
        : jsPandaFile_(jsPandaFile), pfLoader_(profilerLoader), bytecodeInfo_(bytecodeInfo)
    {
    }

    ~CompilationDriver() = default;
    NO_COPY_SEMANTIC(CompilationDriver);
    NO_MOVE_SEMANTIC(CompilationDriver);

    bool IsPGOLoaded() const
    {
        return pfLoader_.IsLoaded();
    }

    void UpdateCompileQueue(const CString &recordName, EntityId resolvedMethod)
    {
        if (pfLoader_.Match(recordName, resolvedMethod)) {
            return;
        }
        // update profile and update compile queue
        std::unordered_set<EntityId> fullResolvedMethodSet;
        auto dfs = [this, &fullResolvedMethodSet, resolvedMethod] (const CString &recordName,
            [[maybe_unused]] const std::unordered_set<EntityId> &oldIds) -> std::unordered_set<EntityId> &{
                fullResolvedMethodSet.clear();
                std::unordered_set<EntityId> currentResolvedMethodSet {resolvedMethod};
                uint32_t mainMethodOffset = jsPandaFile_->GetMainMethodIndex(recordName);
                SearchForCompilation(currentResolvedMethodSet, fullResolvedMethodSet, mainMethodOffset, true);
                return fullResolvedMethodSet;
            };

        pfLoader_.Update(recordName, dfs);

        if (fullResolvedMethodSet.size() > 0) {
            bytecodeInfo_.AddRecordName(recordName);
        }
        for (auto &newMethod : fullResolvedMethodSet) {
            bytecodeInfo_.EraseSkippedMethod(newMethod.GetOffset());
        }
    }

    template <class Callback>
    void Run(const Callback &cb)
    {
        UpdatePGO();
        InitializeCompileQueue();
        uint32_t index = 0;
        auto &methodList = bytecodeInfo_.GetMethodList();
        const auto &methodPcInfos = bytecodeInfo_.GetMethodPcInfos();
        while (!compileQueue_.empty()) {
            std::queue<uint32_t> methodCompiledOrder;
            methodCompiledOrder.push(compileQueue_.front());
            compileQueue_.pop();
            while (!methodCompiledOrder.empty()) {
                auto compilingMethod = methodCompiledOrder.front();
                methodCompiledOrder.pop();
                bytecodeInfo_.AddMethodOffsetToRecordName(compilingMethod, bytecodeInfo_.GetRecordName(index));
                auto &methodInfo = methodList.at(compilingMethod);
                auto &methodPcInfo = methodPcInfos[methodInfo.GetMethodPcInfoIndex()];
                auto methodLiteral = jsPandaFile_->FindMethodLiteral(compilingMethod);
                const std::string methodName(MethodLiteral::GetMethodName(jsPandaFile_, methodLiteral->GetMethodId()));
                if (FilterMethod(bytecodeInfo_.GetRecordName(index), methodLiteral, methodPcInfo)) {
                    bytecodeInfo_.AddSkippedMethod(compilingMethod);
                } else {
                    if (!methodInfo.IsCompiled()) {
                        methodInfo.SetIsCompiled(true);
                        cb(bytecodeInfo_.GetRecordName(index), methodName, methodLiteral, compilingMethod,
                           methodPcInfo, methodInfo.GetMethodInfoIndex());
                    }
                }
                auto &innerMethods = methodInfo.GetInnerMethods();
                for (auto it : innerMethods) {
                    methodCompiledOrder.push(it);
                }
            }
            index++;
        }
    }

    void AddResolvedMethod(const CString &recordName, uint32_t classLiteralOffset)
    {
        if (!IsPGOLoaded() || !bytecodeInfo_.HasClassDefMethod(classLiteralOffset)) {
            return;
        }
        uint32_t resolvedMethod = bytecodeInfo_.GetDefineMethod(classLiteralOffset);
        panda_file::File::EntityId resolvedMethodId(resolvedMethod);
        UpdateCompileQueue(recordName, resolvedMethodId);
    }
private:
    void UpdatePGO();

    void InitializeCompileQueue();

    void SearchForCompilation(const std::unordered_set<EntityId> &methodSet, std::unordered_set<EntityId> &newMethodSet,
                              uint32_t mainMethodOffset, bool needUpdateCompile)
    {
        auto &methodList = bytecodeInfo_.GetMethodList();
        std::function<void(EntityId, bool)> dfs = [this, &newMethodSet, &mainMethodOffset, &dfs, &methodList]
            (EntityId methodId, bool needUpdateCompile) -> void {
            uint32_t methodOffset = methodId.GetOffset();
            if (methodOffset == MethodInfo::DEFAULT_OUTMETHOD_OFFSET) {
                return;
            }
            // if pgo profile is not matched with current abc file, methodOffset will be different
            if (methodList.find(methodOffset) == methodList.end()) {
                LOG_COMPILER(ERROR) << "The correct aot profile has not been used";
                return;
            }
            auto &methodInfo = methodList.at(methodOffset);
            auto outMethodOffset = methodInfo.GetOutMethodOffset();
            // if current method has already been marked as PGO, stop searching upper layer of the define chain
            if (methodInfo.IsPGO()) {
                return;
            }
            // we need to collect these new-marked PGO methods to update PGO profile
            newMethodSet.insert(methodId);
            methodInfo.SetIsPGO(true);
            if (needUpdateCompile) {
                // in deopt, we need to push the first un-marked method which is
                // in upper layer of the deopt method's define chain (or maybe the deopt method itself)
                // into the sharedCompiledQueue to trigger the compilation of the deopt method.
                if (methodOffset != mainMethodOffset) {
                    // few methods which have the same bytecodes as other method can't find its outter method
                    if (outMethodOffset == MethodInfo::DEFAULT_OUTMETHOD_OFFSET) {
                        compileQueue_.push(methodOffset);
                        return;
                    }
                    // currentMethod whose outtermethod has been marked as pgo need to push into queue
                    auto outMethodInfo = methodList.at(outMethodOffset);
                    if (outMethodInfo.IsPGO()) {
                        compileQueue_.push(methodOffset);
                    }
                } else {
                    // if current searched method is an un-marked main method, just push it to compile queue
                    compileQueue_.push(methodOffset);
                }
            }
            if (methodOffset == mainMethodOffset) {
                return;
            }
            EntityId outMethod(outMethodOffset);
            dfs(outMethod, needUpdateCompile);
        };

        // search as link list, a one-way define chain
        for (auto pgoMethod = methodSet.begin(); pgoMethod != methodSet.end(); pgoMethod++) {
            dfs(*pgoMethod, needUpdateCompile);
        }
    }

    bool FilterMethod(const CString &recordName, const MethodLiteral *methodLiteral,
                      const MethodPcInfo &methodPCInfo) const;

    JSPandaFile *jsPandaFile_ {nullptr};
    PGOProfilerLoader &pfLoader_;
    BCInfo &bytecodeInfo_;
    std::queue<uint32_t> compileQueue_ {};
};
} // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_COMPILATION_DRIVER_H
