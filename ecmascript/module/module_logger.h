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

#ifndef ECMASCRIPT_MODULE_MODULE_LOGGER_H
#define ECMASCRIPT_MODULE_MODULE_LOGGER_H

#include "ecmascript/module/js_module_source_text.h"

#include <fstream>
namespace panda::ecmascript {

struct ModuleLoadInfo {
    bool isUsed {false};
    CSet<CString> parentModules {};
    CMap<CString, CSet<CString>> upLevel {}; // parent module and importName
    double time {0};
};

class ModuleLogger {
public:
    explicit ModuleLogger(EcmaVM *vm);
    ~ModuleLogger()
    {
        for (auto &info : jsModuleLoadInfo_) {
            delete info.second;
        }
        jsModuleLoadInfo_.clear();
    }
    void SetStartTime(const CString &recordName);
    void SetEndTime(const CString &recordName);
    static void SetModuleLoggerTask(EcmaVM *vm);
    static void PrintModuleLoadInfoTask(void *data);
    void InsertModuleLoadInfo(JSHandle<SourceTextModule> currentModule,
                              JSHandle<SourceTextModule> exportModule,
                              int32_t index);
    void InsertParentModule(JSHandle<SourceTextModule> currentModule, JSHandle<SourceTextModule> requiredModule);
    void InsertEntryPointModule(JSHandle<SourceTextModule> currentModule);
    void PrintModuleLoadInfo();
    static std::string ToStringWithPrecision(const double num, const uint8_t n);

private:
    static constexpr const uint32_t MILLISECONDS_PER_SEC = 1000;
    static constexpr const double DOUBLE_MILLISECONDS_PER_SEC = 1000.0;
    static constexpr const uint8_t TWO = 2;
    static constexpr const uint8_t THREE = 3;
    static constexpr const uint64_t TWO_SECONDS = TWO * MILLISECONDS_PER_SEC;
    static constexpr const char FILEDIR[] = "/data/storage/el2/base/files/";
    static constexpr const char SUFFIX[] = "_redundant_file.txt";

    NO_COPY_SEMANTIC(ModuleLogger);
    NO_MOVE_SEMANTIC(ModuleLogger);

    bool CreateResultFile(std::string &path) const; // first time
    bool OpenResultFile(std::string &path) const;
    ModuleLoadInfo *GetModuleLoadInfo(const CString &recordName);

    void PrintSummary() const;
    void PrintUsedFileInfo() const;
    void PrintUnusedFileInfo() const;
    void ProcessModuleExecuteTime();
    EcmaVM *vm_ {nullptr};
    uint32_t tid_ {0};
    CUnorderedMap<CString, ModuleLoadInfo*> jsModuleLoadInfo_ {};
    CVector<std::pair<CString, ModuleLoadInfo*>> jsModuleLoadInfoRes_;
    uint32_t totalFileNumber_ {0};
    uint32_t unusedFileNumber_ {0};
    uint32_t usedFileNumber_ {0};
    int64_t totalTime_ {0};
    int64_t usedFileTime_ {0};
    int64_t unusedFileTime_ {0};
    Mutex mutex_;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_MODULE_MODULE_LOGGER_H
