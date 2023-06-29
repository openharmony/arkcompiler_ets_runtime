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
class AOTFileGenerator;
class CompilerLog;
struct LOptions;
class Module;
class CompilationDriver {
public:
    CompilationDriver(PGOProfilerDecoder &profilerDecoder,
                      BytecodeInfoCollector* collector,
                      const std::string &compilemMethodsOption,
                      const std::string &compileSkipMethodsOption,
                      AOTFileGenerator *fileGenerator,
                      const std::string &fileName,
                      const std::string &triple,
                      LOptions *lOptions,
                      CompilerLog *log,
                      bool outputAsm,
                      size_t maxMethodsInModule);
    ~CompilationDriver();

    NO_COPY_SEMANTIC(CompilationDriver);
    NO_MOVE_SEMANTIC(CompilationDriver);

    bool IsPGOLoaded() const
    {
        return pfDecoder_.IsLoaded();
    }

    void UpdateCompileQueue(const CString &recordName, EntityId resolvedMethod)
    {
        const auto &methodList = bytecodeInfo_.GetMethodList();
        auto &resolvedMethodInfo = methodList.at(resolvedMethod.GetOffset());
        if (pfDecoder_.Match(recordName, resolvedMethod) && !resolvedMethodInfo.IsTypeInferAbort()) {
            return;
        }
        // update profile and update compile queue
        std::unordered_set<EntityId> fullResolvedMethodSet;
        auto dfs = [this, &fullResolvedMethodSet, resolvedMethod] (const CString &recordName,
            [[maybe_unused]] const std::unordered_set<EntityId> &oldIds) -> std::unordered_set<EntityId> & {
                fullResolvedMethodSet.clear();
                std::unordered_set<EntityId> currentResolvedMethodSet {resolvedMethod};
                uint32_t mainMethodOffset = jsPandaFile_->GetMainMethodIndex(recordName);
                SearchForCompilation(recordName, currentResolvedMethodSet,
                                     fullResolvedMethodSet, mainMethodOffset, true);
                return fullResolvedMethodSet;
            };

        pfDecoder_.Update(recordName, dfs);

        if (fullResolvedMethodSet.size() > 0) {
            bytecodeInfo_.AddRecordName(recordName);
        }
        for (auto &newMethod : fullResolvedMethodSet) {
            bytecodeInfo_.EraseSkippedMethod(newMethod.GetOffset());
        }
    }

    template <class Callback>
    void CompileMethod(const Callback &cb,
                       uint32_t index,
                       const std::string &methodName,
                       MethodLiteral *methodLiteral,
                       uint32_t methodOffset,
                       const MethodPcInfo &methodPcInfo,
                       MethodInfo &methodInfo)
    {
        Module *module = GetModule();
        cb(bytecodeInfo_.GetRecordName(index), methodName, methodLiteral, methodOffset,
            methodPcInfo, methodInfo, module);
        if (methodInfo.IsTypeInferAbort()) {
            return;
        }
        IncCompiledMethod();
        CompileModuleThenDestroyIfNeeded();
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
            compileQueue_.pop_front();
            while (!methodCompiledOrder.empty()) {
                auto compilingMethod = methodCompiledOrder.front();
                methodCompiledOrder.pop();
                bytecodeInfo_.AddMethodOffsetToRecordName(compilingMethod, bytecodeInfo_.GetRecordName(index));
                auto &methodInfo = methodList.at(compilingMethod);
                auto &methodPcInfo = methodPcInfos[methodInfo.GetMethodPcInfoIndex()];
                auto methodLiteral = jsPandaFile_->FindMethodLiteral(compilingMethod);
                const std::string methodName(MethodLiteral::GetMethodName(jsPandaFile_, methodLiteral->GetMethodId()));
                if (FilterMethod(bytecodeInfo_.GetRecordName(index), methodLiteral, methodPcInfo, methodName)) {
                    bytecodeInfo_.AddSkippedMethod(compilingMethod);
                } else {
                    if (!methodInfo.IsCompiled()) {
                        methodInfo.SetIsCompiled(true);
                        CompileMethod(cb, index, methodName, methodLiteral, compilingMethod, methodPcInfo, methodInfo);
                    } else if (NeedSecondaryCompile(methodInfo)) {
                        // if a method used to be not full compiled but now it's a deopt resolved method
                        // it should be full compiled again
                        CompileMethod(cb, index, methodName, methodLiteral, compilingMethod, methodPcInfo, methodInfo);
                    }
                }
                auto &innerMethods = methodInfo.GetInnerMethods();
                for (auto it : innerMethods) {
                    methodCompiledOrder.push(it);
                }
            }
            index++;
        }
        CompileLastModuleThenDestroyIfNeeded();
    }

    void FetchPGOMismatchResult();

    void AddResolvedMethod(const CString &recordName, uint32_t classLiteralOffset)
    {
        if (!IsPGOLoaded() || !bytecodeInfo_.HasClassDefMethod(classLiteralOffset)) {
            return;
        }
        uint32_t resolvedMethod = bytecodeInfo_.GetDefineMethod(classLiteralOffset);
        auto &methodList = bytecodeInfo_.GetMethodList();
        auto &methodInfo = methodList.at(resolvedMethod);
        methodInfo.SetResolvedMethod(true);
        panda_file::File::EntityId resolvedMethodId(resolvedMethod);
        UpdateCompileQueue(recordName, resolvedMethodId);
    }
private:
    // add maxMethodsInModule_ functions in a module and when a module is
    // full(maxMethodsInModule_ functions have been put into) or the module is the last module,
    // compile it and the destroy it.
    Module *GetModule();

    void IncCompiledMethod();

    bool IsCurModuleFull() const;

    void CompileModuleThenDestroyIfNeeded();

    void CompileLastModuleThenDestroyIfNeeded();

    void TopologicalSortForRecords();

    void UpdatePGO();

    void InitializeCompileQueue();

    bool NeedSecondaryCompile(const MethodInfo &methodInfo) const
    {
        return methodInfo.IsTypeInferAbort() && methodInfo.IsResolvedMethod();
    }

    void AddDependList(const CString &recordName, uint32_t methodOffset,
                       std::deque<CString> &dependList)
    {
        auto &methodList = bytecodeInfo_.GetMethodList();
        const auto &importRecordInfos = bytecodeInfo_.GetImportRecordsInfos();
        auto iter = importRecordInfos.find(recordName);
        // if the resolved method don't have import records need-inferred, just return.
        if (iter == importRecordInfos.end()) {
            return;
        }
        auto &methodInfo = methodList.at(methodOffset);
        // Get the import indexs collected in methodInfo.
        auto &importIndexs = methodInfo.GetImportIndexes();
        // idInRecord is a map like "importIndex: (exportRecord: exportIndex)".
        const auto &idInRecord = iter->second.GetImportIdToExportRecord();
        for (auto index : importIndexs) {
            auto it = idInRecord.find(index);
            if (it != idInRecord.end()) {
                dependList.emplace_back(it->second.first);
            }
        }
    }

    bool VerifyAndMarkCurMethod(uint32_t methodOffset, std::unordered_set<EntityId> &newMethodSet)
    {
        // if current method is at the boundary state， we should stop the define chain search.
        if (methodOffset == MethodInfo::DEFAULT_OUTMETHOD_OFFSET) {
            return false;
        }
        // if pgo profile is not matched with current abc file, methodOffset will be different.
        auto &methodList = bytecodeInfo_.GetMethodList();
        auto iter = methodList.find(methodOffset);
        if (iter == methodList.end()) {
            LOG_COMPILER(ERROR) << "The correct aot profile has not been used";
            return false;
        }
        auto &methodInfo = iter->second;
        // if current method has already been marked as PGO, stop searching upper layer of the define chain.
        if (methodInfo.IsPGO() && !methodInfo.IsTypeInferAbort()) {
            return false;
        }
        // we need to collect these new-marked PGO methods to update PGO profile
        panda_file::File::EntityId methodId(methodOffset);
        newMethodSet.insert(std::move(methodId));
        methodInfo.SetIsPGO(true);
        return true;
    }

    void UpdateResolveDepends(std::vector<CString> &dependNames, bool needUpdate)
    {
        if (needUpdate && !dependNames.empty()) {
            // depend methods should keep the topological sorting rule
            std::sort(dependNames.begin(), dependNames.end(), [this](auto first, auto second) {
                return sortedRecords_.at(first) < sortedRecords_.at(second);
            });
            auto resolvedMethod = compileQueue_.back();
            compileQueue_.pop_back();
            // it should be like "depended main method1, depended main method2, resolved method" in compileQueue.
            for (const auto &ele : dependNames) {
                bytecodeInfo_.AddRecordName(ele);
                auto eleOffset = jsPandaFile_->GetMainMethodIndex(ele);
                // these methods will be added to compile queue
                bytecodeInfo_.EraseSkippedMethod(eleOffset);
                compileQueue_.push_back(eleOffset);
            }
            compileQueue_.push_back(resolvedMethod);
        }
    }

    void SearchForCompilation(const CString &recordName, const std::unordered_set<EntityId> &methodSet,
                              std::unordered_set<EntityId> &newMethodSet,
                              uint32_t mainMethodOffset, bool needUpdateCompile)
    {
        auto &methodList = bytecodeInfo_.GetMethodList();
        std::unordered_set<EntityId> mainMethodSet;
        auto getMainMethodSet = [this, &mainMethodSet](const CString &importRecord,
            [[maybe_unused]] const std::unordered_set<EntityId> &oldIds) -> std::unordered_set<EntityId>& {
            mainMethodSet.clear();
            auto mainMethodOffset = jsPandaFile_->GetMainMethodIndex(importRecord);
            panda_file::File::EntityId mainMethodId(mainMethodOffset);
            mainMethodSet.insert(mainMethodId);
            return mainMethodSet;
        };

        std::vector<CString> importNames{};
        std::function<void(EntityId)> importBfs =
            [this, &methodList, &recordName, &importNames, &getMainMethodSet]
            (EntityId methodId) -> void {
            uint32_t methodOffset = methodId.GetOffset();
            std::deque<CString> importList{};
            AddDependList(recordName, methodOffset, importList);
            while (!importList.empty()) {
                auto importRecord = importList.front();
                importList.pop_front();
                // export syntax only exists in main method, so just judge and collect the main method.
                auto mainMethodOffset = jsPandaFile_->GetMainMethodIndex(importRecord);
                auto &mainMethodInfo = methodList.at(mainMethodOffset);
                // mark the main method in other record as PGO.
                if (mainMethodInfo.IsPGO()) {
                    continue;
                }
                importNames.emplace_back(importRecord);
                mainMethodInfo.SetIsPGO(true);
                pfDecoder_.Update(importRecord, getMainMethodSet);
                AddDependList(importRecord, mainMethodOffset, importList);
            }
        };

        std::function<void(EntityId, bool)> dfs =
            [this, &newMethodSet, &mainMethodOffset, &dfs, &methodList, &importBfs]
            (EntityId methodId, bool needUpdateCompile) -> void {
            uint32_t methodOffset = methodId.GetOffset();
            // verify whether we should stop the search for pgo methods or resolve methods.
            if (!VerifyAndMarkCurMethod(methodOffset, newMethodSet)) {
                return;
            }
            auto &methodInfo = methodList.at(methodOffset);
            auto outMethodOffset = methodInfo.GetOutMethodOffset();
            // for pgo method, collect its import depends method
            importBfs(methodId);
            if (needUpdateCompile) {
                // in deopt, we need to push the first un-marked method which is
                // in upper layer of the deopt method's define chain (or maybe the deopt method itself)
                // into the sharedCompiledQueue to trigger the compilation of the deopt method.
                if (methodOffset != mainMethodOffset) {
                    // few methods which have the same bytecodes as other method can't find its outter method
                    if (outMethodOffset == MethodInfo::DEFAULT_OUTMETHOD_OFFSET) {
                        compileQueue_.push_back(methodOffset);
                        return;
                    }
                    // currentMethod whose outtermethod has been marked as pgo need to push into queue
                    auto outMethodInfo = methodList.at(outMethodOffset);
                    if (outMethodInfo.IsPGO()) {
                        compileQueue_.push_back(methodOffset);
                        return;
                    }
                } else {
                    // if current searched method is an un-marked main method, just push it to compile queue
                    compileQueue_.push_back(methodOffset);
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
        // update compile queue only for resolve method when it depends on other record
        UpdateResolveDepends(importNames, needUpdateCompile);
    }

    bool FilterMethod(const CString &recordName, const MethodLiteral *methodLiteral,
                      const MethodPcInfo &methodPCInfo, const std::string &methodName) const;

    std::vector<std::string> SplitString(const std::string &str, const char ch) const;

    void ParseOption(const std::string &option, std::map<std::string, std::vector<std::string>> &optionMap) const;

    bool FilterOption(const std::map<std::string, std::vector<std::string>> &optionMap, const std::string &recordName,
                      const std::string &methodName) const;

    EcmaVM *vm_ {nullptr};
    const JSPandaFile *jsPandaFile_ {nullptr};
    PGOProfilerDecoder &pfDecoder_;
    BCInfo &bytecodeInfo_;
    std::deque<uint32_t> compileQueue_ {};
    std::map<CString, uint32_t> sortedRecords_ {};
    std::map<std::string, std::vector<std::string>> optionSelectMethods_ {};
    std::map<std::string, std::vector<std::string>> optionSkipMethods_ {};
    uint32_t compiledMethodCnt_ {0};
    AOTFileGenerator *fileGenerator_ {nullptr};
    std::string fileName_ {};
    std::string triple_ {};
    LOptions *lOptions_ {nullptr};
    CompilerLog *log_ {nullptr};
    bool outputAsm_ {false};
    size_t maxMethodsInModule_ {0};
};
} // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_COMPILATION_DRIVER_H
