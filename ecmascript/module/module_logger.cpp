/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/module/module_logger.h"

#include "ecmascript/ecma_context.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/platform/parameters.h"
#include "ecmascript/runtime_lock.h"
namespace panda::ecmascript {

ModuleLogger::ModuleLogger(EcmaVM *vm) : vm_(vm)
{
    tid_ = os::thread::GetCurrentThreadId();
}

void ModuleLogger::PrintModuleLoadInfo()
{
    ProcessModuleExecuteTime();
    // copy jsModuleLoadInfo from map to vector in order to sort by cost time.
    jsModuleLoadInfoRes_.insert(jsModuleLoadInfoRes_.begin(), jsModuleLoadInfo_.begin(), jsModuleLoadInfo_.end());
    std::sort(jsModuleLoadInfoRes_.begin(), jsModuleLoadInfoRes_.end(),
        [](auto &x, auto &y) { return x.second->time > y.second->time; });
    PrintSummary();
    PrintUsedFileInfo();
    PrintUnusedFileInfo();
}

bool ModuleLogger::CreateResultFile(std::string &path) const
{
    path = base::ConcatToCString(FILEDIR, vm_->GetBundleName());
    if (vm_->IsWorkerThread()) {
        base::AppendToBaseString(path, "_", std::to_string(tid_));
    }
    path += SUFFIX;
    constexpr mode_t defaultMode = S_IRUSR | S_IWUSR | S_IRGRP; // -rw-r--
    int fd = creat(path.c_str(), defaultMode);
    if (fd == -1) {
        LOG_ECMA(ERROR) << "file create failed, errno = "<< errno;
        return false;
    }
    close(fd);
    return true;
}

bool ModuleLogger::OpenResultFile(std::string &path) const
{
    path = base::ConcatToCString(FILEDIR, vm_->GetBundleName());
    if (vm_->IsWorkerThread()) {
        base::AppendToBaseString(path, "_", std::to_string(tid_));
    }
    path += SUFFIX;
    if (access(path.c_str(), F_OK) == 0) {
        if (access(path.c_str(), W_OK) == 0) {
            LOG_ECMA(DEBUG) << "file open success";
            return true;
        }
        LOG_ECMA(ERROR) << "file create failed, W_OK false";
        return false;
    }
    return CreateResultFile(path);
}

void ModuleLogger::InsertModuleLoadInfo(JSHandle<SourceTextModule> currentModule,
    JSHandle<SourceTextModule> exportModule, int32_t index)
{
    JSThread *thread = vm_->GetJSThread();
    RuntimeLockHolder locker(thread, mutex_);
    CString curName = SourceTextModule::GetModuleName(currentModule.GetTaggedValue());
    CString sonName = SourceTextModule::GetModuleName(exportModule.GetTaggedValue());
    ModuleLoadInfo *sonModule = GetModuleLoadInfo(sonName);
    sonModule->isUsed = true;
    // get importName
    CString exportNameStr;
    if (index < 0) {
        exportNameStr = "*";
    } else {
        JSHandle<TaggedArray> importArray(thread, currentModule->GetImportEntries());
        JSHandle<ImportEntry> importEntry(thread, importArray->Get(index));
        JSHandle<JSTaggedValue> importName(thread, importEntry->GetImportName());
        exportNameStr = ModulePathHelper::Utf8ConvertToString(importName.GetTaggedValue());
    }
    auto &upLevelInfo = sonModule->upLevel;
    auto ulModule = upLevelInfo.find(curName);
    if (ulModule == upLevelInfo.end()) {
        CSet<CString> tmp;
        tmp.insert(exportNameStr);
        upLevelInfo.emplace(curName, std::move(tmp));
        return;
    }
    ulModule->second.insert(exportNameStr);
}

void ModuleLogger::InsertParentModule(JSHandle<SourceTextModule> currentModule,
                                      JSHandle<SourceTextModule> requiredModule)
{
    RuntimeLockHolder locker(vm_->GetJSThread(), mutex_);
    CString curName = currentModule->GetEcmaModuleRecordNameString();
    CString sonName = requiredModule->GetEcmaModuleRecordNameString();
    ModuleLoadInfo *sonModule = GetModuleLoadInfo(sonName);
    sonModule->parentModules.insert(std::move(curName));
}

void ModuleLogger::InsertEntryPointModule(JSHandle<SourceTextModule> currentModule)
{
    RuntimeLockHolder locker(vm_->GetJSThread(), mutex_);
    CString curName = currentModule->GetEcmaModuleRecordNameString();
    ModuleLoadInfo *curModule = GetModuleLoadInfo(curName);
    curModule->isUsed = true;
    auto &upLevelInfo = curModule->upLevel;
    CSet<CString> tmp;
    upLevelInfo.emplace("EntryPoint", tmp);
}

void ModuleLogger::PrintSummary() const
{
    std::string path;
    if (!CreateResultFile(path)) {
        LOG_ECMA(ERROR) << "open file fail, no log anymore";
        return;
    }
    std::ofstream fileHandle;
    fileHandle.open(path, std::ios_base::app);
    std::string start = base::ConcatToStdString("<----Summary----> Total file number: ",
        std::to_string(totalFileNumber_), ", total time: ",
        std::to_string(totalTime_), "ms, including used file:", std::to_string(usedFileNumber_),
        ", cost time: ", std::to_string(usedFileTime_), "ms, and unused file: ",
        std::to_string(unusedFileNumber_), ", cost time: ", std::to_string(unusedFileTime_), "ms\n");
    fileHandle << start;
    fileHandle.close();
}

void ModuleLogger::PrintUsedFileInfo() const
{
    std::string path;
    if (!OpenResultFile(path)) {
        LOG_ECMA(ERROR) << "open file fail, no log anymore";
        return;
    }
    std::ofstream fileHandle;
    fileHandle.open(path, std::ios_base::app);
    std::string start = base::ConcatToStdString("<----used file start----> used file: ",
        std::to_string(usedFileNumber_), ", cost time: ", std::to_string(usedFileTime_), "ms\n");
    fileHandle << start;
    int32_t fileNumber = 1;
    for (auto &i : jsModuleLoadInfoRes_) {
        ModuleLoadInfo *info = i.second;
        if (info->isUsed) {
            std::string content = base::ConcatToStdString("used file ",
                std::to_string(fileNumber++), ": ", i.first.c_str(),
                ", cost time: ", ToStringWithPrecision(info->time, THREE), "ms\n");
            fileHandle << content;
            auto &upInfo = info->upLevel;
            int32_t parentModuleCount = 1;
            for (auto &k : upInfo) {
                std::string parentInfoStr = base::ConcatToStdString("          parentModule ",
                    std::to_string(parentModuleCount++), ": ", k.first.c_str(), " ");
                for (auto &exportN : k.second) {
                    base::AppendToBaseString(parentInfoStr, exportN, " ");
                }
                parentInfoStr += "\n";
                fileHandle << parentInfoStr;
            }
        }
    }
    std::string end = "<----------used file end----------> \n";
    fileHandle << end;
    fileHandle.close();
}

void ModuleLogger::PrintUnusedFileInfo() const
{
    std::string path;
    if (!OpenResultFile(path)) {
        LOG_ECMA(ERROR) << "open file fail, no log anymore";
        return;
    }
    std::ofstream fileHandle;
    fileHandle.open(path, std::ios_base::app);
    std::string start = base::ConcatToStdString("<----unused file start----> unused file: ",
        std::to_string(unusedFileNumber_), ", cost time: ", std::to_string(unusedFileTime_), "ms\n");
    fileHandle << start;
    int32_t fileNumber = 1;
    for (auto &i : jsModuleLoadInfoRes_) {
        ModuleLoadInfo *info = i.second;
        if (!info->isUsed) {
            std::string content = base::ConcatToStdString("unused file ", std::to_string(fileNumber++), ": ",
                i.first.c_str(), ", cost time: ", ToStringWithPrecision(info->time, THREE), "ms\n");
            fileHandle << content;
            int32_t parentNumber = 1;
            CSet<CString> parents = info->parentModules;
            for (auto &k : parents) {
                std::string parent = base::ConcatToStdString("              parentModule ",
                    std::to_string(parentNumber++), ": ", k.c_str(), "\n");
                fileHandle << parent;
            }
        }
    }
    std::string end = "<----------unused file end----------> \n";
    fileHandle << end;
    fileHandle.close();
}

void ModuleLogger::ProcessModuleExecuteTime()
{
    RuntimeLockHolder locker(vm_->GetJSThread(), mutex_);
    totalFileNumber_ = jsModuleLoadInfo_.size();
    double totalTime = 0;
    double usedFileTime = 0;
    double unusedFileTime = 0;
    for (auto &i : jsModuleLoadInfo_) {
        ModuleLoadInfo *info = i.second;
        double time = info->time;
        if (info->isUsed) {
            usedFileNumber_++;
            usedFileTime += time;
        } else {
            unusedFileNumber_++;
            unusedFileTime += time;
        }
        totalTime += time;
    }
    totalTime_ = static_cast<int64_t>(totalTime);
    usedFileTime_ = static_cast<int64_t>(usedFileTime);
    unusedFileTime_ = static_cast<int64_t>(unusedFileTime);
}

ModuleLoadInfo *ModuleLogger::GetModuleLoadInfo(const CString &recordName)
{
    auto info = jsModuleLoadInfo_.find(recordName);
    if (info != jsModuleLoadInfo_.end()) {
        return info->second;
    }
    ModuleLoadInfo *newInfo = new ModuleLoadInfo();
    jsModuleLoadInfo_.emplace(recordName, newInfo);
    return newInfo;
}

void ModuleLogger::SetStartTime(const CString &recordName)
{
    // func_main_0/_GLOBAL::func_main_0 is system abc or so js/abc, need to exclude.
    if (recordName == ModulePathHelper::ENTRY_FUNCTION_NAME ||
        recordName == ModulePathHelper::ENTRY_MAIN_FUNCTION) {
        return;
    }
    RuntimeLockHolder locker(vm_->GetJSThread(), mutex_);
    ModuleLoadInfo *info = GetModuleLoadInfo(recordName);
    info->time = std::chrono::duration_cast<std::chrono::microseconds>
                 (std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

void ModuleLogger::SetEndTime(const CString &recordName)
{
    // func_main_0/_GLOBAL::func_main_0 is system abc or so js/abc, need to exclude.
    if (recordName == ModulePathHelper::ENTRY_FUNCTION_NAME ||
        recordName == ModulePathHelper::ENTRY_MAIN_FUNCTION) {
        return;
    }
    RuntimeLockHolder locker(vm_->GetJSThread(), mutex_);
    ModuleLoadInfo *info = GetModuleLoadInfo(recordName);
    double time = info->time;
    info->time = (std::chrono::duration_cast<std::chrono::microseconds>
                    (std::chrono::high_resolution_clock::now().time_since_epoch())
                    .count() - time) / DOUBLE_MILLISECONDS_PER_SEC;
}

std::string ModuleLogger::ToStringWithPrecision(const double num, const uint8_t n)
{
    std::ostringstream out;
    out << std::setprecision(n) << num;
    return out.str();
}

void ModuleLogger::PrintModuleLoadInfoTask(void *data)
{
    ModuleLogger* moduleLogger = static_cast<ModuleLogger*>(data);
    if (moduleLogger == nullptr) {
        return;
    }
    moduleLogger->PrintModuleLoadInfo();
}

void ModuleLogger::SetModuleLoggerTask(EcmaVM *vm)
{
    ModuleLogger *moduleLogger = vm->GetJSThread()->GetModuleLogger();
    if (moduleLogger == nullptr) {
        return;
    }
    TimerTaskCallback timerTaskCallBack = vm->GetTimerTaskCallback();
    if (timerTaskCallBack == nullptr) {
        LOG_FULL(ERROR) << "TimerTaskCallback is nullptr";
        return;
    }
    uint64_t importDurationInMs = GetImportDuration(TWO_SECONDS);
    timerTaskCallBack(vm, moduleLogger, PrintModuleLoadInfoTask, importDurationInMs, 0);
}
} // namespace panda::ecmascript
