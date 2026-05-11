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

#ifndef ECMASCRIPT_MODULE_TOOLS_H
#define ECMASCRIPT_MODULE_TOOLS_H

#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/module_logger.h"

namespace panda::ecmascript {

class ModuleManager;

class ModuleTraceScope {
public:
    template<typename... Args>
    static ModuleTraceScope Open(JSThread *thread, const Args&... args)
    {
        if (!thread->GetEcmaVM()->GetJSOptions().EnableESMTrace()) {
            return ModuleTraceScope();
        }
        return ModuleTraceScope(base::ConcatToCString(args...));
    }
    ~ModuleTraceScope()
    {
        if (enableESMTrace_) {
            ECMA_BYTRACE_FINISH_TRACE(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK);
        }
    }

private:
    ModuleTraceScope(const CString &info) : enableESMTrace_(true)
    {
        ECMA_BYTRACE_START_TRACE(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, info.c_str());
    }
    ModuleTraceScope() : enableESMTrace_(false) {}

    bool enableESMTrace_ {false};
};

class ModuleLoggerTimeScope {
public:
    void* operator new[](size_t size) = delete;
    void operator delete[](void* ptr) = delete;
    void* operator new(size_t size) = delete;
    void operator delete(void* ptr) = delete;
    // disable construct from temporary CString
    ModuleLoggerTimeScope(JSThread *thread, CString &&moduleRequestName) = delete;
    ModuleLoggerTimeScope(JSThread *thread, const CString &moduleRequestName)
        : moduleRequestName_(moduleRequestName), moduleLogger_(thread->GetModuleLogger())
    {
        if (moduleLogger_ != nullptr) {
            startTime_ = moduleLogger_->now();
        }
    }

    ~ModuleLoggerTimeScope()
    {
        if (moduleLogger_ != nullptr) {
            moduleLogger_->SetDuration(moduleRequestName_, startTime_);
        }
    }
private:
    double startTime_ {};
    // this should be replace to value type, if template constructor is supported in the future.
    const CString& moduleRequestName_;
    ModuleLogger *moduleLogger_ {nullptr};
};

class ModuleImportStackScope {
public:
    static constexpr size_t MODULE_IMPORT_STACK_HEAD_LENGTH = 19;
    static constexpr size_t ELLIPSIS_PLACEHOLDER_LENGTH = 5;
    static constexpr std::string_view NEWLINE_CHARACTER = "\n";
    static constexpr std::string_view ELLIPSIS_PLACEHOLDER = "\n....";
    static constexpr std::string_view MODULE_IMPORT_STACK_HEAD = "\nModuleImportStack:";
    static constexpr char ALIGNMENT_SYMBOL = '0';
    static constexpr std::string_view MODULE_IMPORT_STACK_NUMBER_LABEL = "\n#";

    explicit ModuleImportStackScope(JSThread *thread, JSHandle<SourceTextModule> module);
    ~ModuleImportStackScope();

    NO_COPY_SEMANTIC(ModuleImportStackScope);
    NO_MOVE_SEMANTIC(ModuleImportStackScope);

private:
    void PushModuleImportStack(const CString &moduleName);
    void PopModuleImportStack(const CString &moduleName);
    void RenumberModuleImportStack();
    int GetNumberOfDigits(int number) const;
    std::string FormatStackNumber(int stackIndex, int totalDepth) const;
    int GetModuleImportStackDepth() const;
    
    bool enableModuleStack_ {false};
    ModuleManager* moduleManager_;
    CString moduleName_;
    uintptr_t handle_ {0};
    std::string truncatedStack_;
};

class ModuleTools {
public:
    static std::string TruncateImportStackForCPPCrash(const CString &moduleName,
        std::string_view stackData, size_t maxSize);
    static std::string TruncateImportStackForJSCrash(std::string_view stackData, size_t maxSize);
private:
    static std::string TruncateModuleImportStack(std::string_view stackData, size_t maxSize);
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_MODULE_MODULE_TOOLS_H
