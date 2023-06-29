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

#include "ecmascript/compiler/compilation_driver.h"
#include "ecmascript/compiler/file_generators.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/ts_types/ts_manager.h"

namespace panda::ecmascript::kungfu {
CompilationDriver::CompilationDriver(PGOProfilerDecoder &profilerDecoder,
                                     BytecodeInfoCollector *collector,
                                     const std::string &optionSelectMethods,
                                     const std::string &optionSkipMethods,
                                     AOTFileGenerator *fileGenerator,
                                     const std::string &fileName,
                                     const std::string &triple,
                                     LOptions *lOptions,
                                     CompilerLog *log,
                                     bool outputAsm,
                                     size_t maxMethodsInModule)
    : vm_(collector->GetVM()),
      jsPandaFile_(collector->GetJSPandaFile()),
      pfDecoder_(profilerDecoder),
      bytecodeInfo_(collector->GetBytecodeInfo()),
      fileGenerator_(fileGenerator),
      fileName_(fileName),
      triple_(triple),
      lOptions_(lOptions),
      log_(log),
      outputAsm_(outputAsm),
      maxMethodsInModule_(maxMethodsInModule)
{
    vm_->GetJSThread()->GetCurrentEcmaContext()->GetTSManager()->SetCompilationDriver(this);

    if (!optionSelectMethods.empty() && !optionSkipMethods.empty()) {
        LOG_COMPILER(FATAL) <<
            "--compiler-select-methods and --compiler-skip-methods should not be set at the same time";
    }

    if (!optionSelectMethods.empty()) {
        ParseOption(optionSelectMethods, optionSelectMethods_);
    }

    if (!optionSkipMethods.empty()) {
        ParseOption(optionSkipMethods, optionSkipMethods_);
    }
}

CompilationDriver::~CompilationDriver()
{
    vm_->GetJSThread()->GetCurrentEcmaContext()->GetTSManager()->SetCompilationDriver(nullptr);
}

Module *CompilationDriver::GetModule()
{
    return IsCurModuleFull() ? fileGenerator_->AddModule(fileName_, triple_, *lOptions_, outputAsm_)
                                : fileGenerator_->GetLatestModule();
}

void CompilationDriver::IncCompiledMethod()
{
    compiledMethodCnt_++;
}

bool CompilationDriver::IsCurModuleFull() const
{
    return (compiledMethodCnt_ % maxMethodsInModule_ == 0);
}

void CompilationDriver::CompileModuleThenDestroyIfNeeded()
{
    if (IsCurModuleFull()) {
        fileGenerator_->CompileLatestModuleThenDestroy();
    }
}

void CompilationDriver::CompileLastModuleThenDestroyIfNeeded()
{
    if (!IsCurModuleFull()) {
        fileGenerator_->CompileLatestModuleThenDestroy();
    }
}

void CompilationDriver::TopologicalSortForRecords()
{
    const auto &importRecordsInfos = bytecodeInfo_.GetImportRecordsInfos();
    std::queue<CString> recordList;
    std::unordered_map<CString, uint32_t> recordInDegree;
    std::unordered_map<CString, std::vector<CString>> exportRecords;
    std::vector<CString> &tpOrder = bytecodeInfo_.GetRecordNames();
    for (auto &record : tpOrder) {
        auto iter = importRecordsInfos.find(record);
        if (iter == importRecordsInfos.end()) {
            recordInDegree.emplace(record, 0);
            recordList.emplace(record);
        } else {
            recordInDegree.emplace(record, iter->second.GetImportRecordSize());
        }
    }
    tpOrder.clear();

    for (auto iter = importRecordsInfos.begin(); iter != importRecordsInfos.end(); iter++) {
        const auto &importRecords = iter->second.GetImportRecords();
        for (const auto &import : importRecords) {
            if (exportRecords.find(import) != exportRecords.end()) {
                exportRecords[import].emplace_back(iter->first);
            } else {
                exportRecords.emplace(import, std::vector<CString>{iter->first});
            }
        }
    }

    while (!recordList.empty()) {
        auto curRecord = recordList.front();
        tpOrder.emplace_back(curRecord);
        recordList.pop();
        auto iter = exportRecords.find(curRecord);
        if (iter != exportRecords.end()) {
            for (const auto &ele : iter->second) {
                if (recordInDegree[ele] > 0 && --recordInDegree[ele] == 0) {
                    recordList.emplace(ele);
                }
            }
        }
    }

    if (UNLIKELY(tpOrder.size() != recordInDegree.size())) {
        LOG_COMPILER(INFO) << "There are circular references in records";
        for (auto &it : recordInDegree) {
            if (it.second != 0) {
                tpOrder.emplace_back(it.first);
            }
        }
        ASSERT(tpOrder.size() == recordInDegree.size());
    }
    auto &mainMethods = bytecodeInfo_.GetMainMethodIndexes();
    auto sortId = 0;
    for (auto &it : tpOrder) {
        mainMethods.emplace_back(jsPandaFile_->GetMainMethodIndex(it));
        sortedRecords_.emplace(it, sortId++);
    }
    ASSERT(tpOrder.size() == mainMethods.size());
}

void CompilationDriver::FetchPGOMismatchResult()
{
    ASSERT(log_ != nullptr);
    uint32_t totalMethodCount = 0;
    uint32_t mismatchMethodCount = 0;
    std::set<std::pair<std::string, CString>> mismatchMethodSet {};
    pfDecoder_.GetMismatchResult(totalMethodCount, mismatchMethodCount, mismatchMethodSet);
    log_->SetPGOMismatchResult(totalMethodCount, mismatchMethodCount, mismatchMethodSet);
}

void CompilationDriver::UpdatePGO()
{
    std::unordered_set<EntityId> newMethodIds;
    auto dfs = [this, &newMethodIds] (const CString &recordName,
        const std::unordered_set<EntityId> &oldIds) -> std::unordered_set<EntityId> & {
            newMethodIds.clear();
            if (!jsPandaFile_->HasTSTypes(recordName)) {
                return newMethodIds;
            }
            uint32_t mainMethodOffset = jsPandaFile_->GetMainMethodIndex(recordName);
            SearchForCompilation(recordName, oldIds, newMethodIds, mainMethodOffset, false);
            return newMethodIds;
        };
    pfDecoder_.Update(dfs);
    FetchPGOMismatchResult();
}

void CompilationDriver::InitializeCompileQueue()
{
    TopologicalSortForRecords();
    auto &mainMethodIndexes = bytecodeInfo_.GetMainMethodIndexes();
    for (auto mainMethodIndex : mainMethodIndexes) {
        compileQueue_.push_back(mainMethodIndex);
    }
}

bool CompilationDriver::FilterMethod(const CString &recordName, const MethodLiteral *methodLiteral,
                                     const MethodPcInfo &methodPCInfo, const std::string &methodName) const
{
    if (methodPCInfo.methodsSize > bytecodeInfo_.GetMaxMethodSize() ||
        !pfDecoder_.Match(recordName, methodLiteral->GetMethodId())) {
        return true;
    }

    if (!optionSelectMethods_.empty()) {
        return !FilterOption(optionSelectMethods_, ConvertToStdString(recordName), methodName);
    } else if (!optionSkipMethods_.empty()) {
        return FilterOption(optionSkipMethods_, ConvertToStdString(recordName), methodName);
    }

    return false;
}

std::vector<std::string> CompilationDriver::SplitString(const std::string &str, const char ch) const
{
    std::vector<std::string> vec {};
    std::istringstream sstr(str.c_str());
    std::string split;
    while (getline(sstr, split, ch)) {
        vec.emplace_back(split);
    }
    return vec;
}

void CompilationDriver::ParseOption(const std::string &option,
                                    std::map<std::string, std::vector<std::string>> &optionMap) const
{
    const char colon = ':';
    const char comma = ',';
    std::string str = option;
    size_t posColon = 0;
    size_t posComma = 0;
    do {
        posColon = str.find_last_of(colon);
        std::string methodNameList = str.substr(posColon + 1, str.size());
        std::vector<std::string> methodNameVec = SplitString(methodNameList, comma);
        str = str.substr(0, posColon);
        posComma = str.find_last_of(comma);
        std::string recordName = str.substr(posComma + 1, str.size());
        str = str.substr(0, posComma);
        optionMap[recordName] = methodNameVec;
    } while (posComma != std::string::npos);
}

bool CompilationDriver::FilterOption(const std::map<std::string, std::vector<std::string>> &optionMap,
                                     const std::string &recordName, const std::string &methodName) const
{
    if (optionMap.empty()) {
        return false;
    }

    auto it = optionMap.find(recordName);
    if (it == optionMap.end()) {
        return false;
    }

    std::vector<std::string> vec = it->second;
    return find(vec.begin(), vec.end(), methodName) != vec.end();
}
} // namespace panda::ecmascript::kungfu
