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
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/platform/dfx_crash_obj.h"

namespace panda::ecmascript {

class ModuleTraceScope {
public:
    ModuleTraceScope(JSThread *thread, [[maybe_unused]]const CString traceInfo)
        : enableESMTrace_(thread->GetEcmaVM()->GetJSOptions().EnableESMTrace())
    {
        if (enableESMTrace_) {
            ECMA_BYTRACE_START_TRACE(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, traceInfo.c_str());
        }
    }

    ~ModuleTraceScope()
    {
        if (enableESMTrace_) {
            ECMA_BYTRACE_FINISH_TRACE(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK);
        }
    }
private:
    bool enableESMTrace_ {false};
};

class ModuleLoggerTimeScope {
public:
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
    CString moduleRequestName_;
    ModuleLogger *moduleLogger_ {nullptr};
};

class ModuleImportStackScope {
    static constexpr size_t MODULE_IMPORT_STACK_HEAD_LENGTH = 19;
    static constexpr size_t ELLIPSIS_PLACEHOLDER_LENGTH = 5;
    static constexpr std::string_view NEWLINE_CHARACTER = "\n";
    static constexpr std::string_view ELLIPSIS_PLACEHOLDER = "\n....";
    static constexpr std::string_view MODULE_IMPORT_STACK_HEAD = "\nModuleImportStack:";
    static constexpr char ALIGNMENT_SYMBOL = '0';
    static constexpr std::string_view MODULE_IMPORT_STACK_NUMBER_LABEL = "\n#";
public:
    explicit ModuleImportStackScope(JSThread *thread, JSHandle<SourceTextModule> module)
        : enableModuleStack_(thread->GetEcmaVM()->GetJSOptions().EnableModuleImportStack())
    {
        if (enableModuleStack_) {
            moduleName_ = SourceTextModule::GetModuleName(module.GetTaggedValue());
            moduleManager_ = thread->GetModuleManager();
            if (!moduleName_.empty()) {
                PushModuleImportStack(moduleName_);
                std::string truncatedStack = TruncateModuleImportStack(
                    moduleManager_->GetModuleImportStackData(), 64 * 1024); // 64KB
                handle_ = SetCrashObject(DFXObjectType::STRING,
                    reinterpret_cast<uintptr_t>(truncatedStack.data()));
            }
        }
    }

    ~ModuleImportStackScope()
    {
        if (enableModuleStack_) {
            ResetCrashObject(handle_);
            PopModuleImportStack(moduleName_);
        }
    }
    NO_COPY_SEMANTIC(ModuleImportStackScope);
    NO_MOVE_SEMANTIC(ModuleImportStackScope);

private:
    void PushModuleImportStack(const CString &moduleName)
    {
        LOG_ECMA(DEBUG) << "ModuleImportStack: start push module:" << moduleName;
        if (moduleName.empty()) {
            LOG_ECMA(ERROR) << "ModuleImportStack: push moduleName is null";
            return;
        }
        std::string stackEntry = std::string(MODULE_IMPORT_STACK_NUMBER_LABEL)
            + ALIGNMENT_SYMBOL + " " + std::string(moduleName); // "\n#0 " + moduleName
        moduleManager_->moduleImportData_.insert(MODULE_IMPORT_STACK_HEAD_LENGTH, stackEntry);
        RenumberModuleImportStack();
    }

    void PopModuleImportStack(const CString &moduleName)
    {
        LOG_ECMA(DEBUG) << "ModuleImportStack: start pop module:" << moduleName;
        std::string stackContent = moduleManager_->moduleImportData_.substr(MODULE_IMPORT_STACK_HEAD_LENGTH);
        if (stackContent.empty()) {
            LOG_ECMA(ERROR) << "ModuleImportStack: pop module failed: nothing to pop";
            return;
        }
    
        size_t nextPos = stackContent.find(NEWLINE_CHARACTER, 1);
        if (nextPos == std::string::npos) {
            nextPos = stackContent.size();
        }
        stackContent.erase(0, nextPos);
        moduleManager_->moduleImportData_ = std::string(MODULE_IMPORT_STACK_HEAD) + stackContent;
        RenumberModuleImportStack();
    }
    
    void RenumberModuleImportStack()
    {
        std::string stackContent = moduleManager_->moduleImportData_.substr(MODULE_IMPORT_STACK_HEAD_LENGTH);
        size_t pos = 0;
        if (!stackContent.empty() && stackContent[0] == NEWLINE_CHARACTER[0]) {
            pos = 1;
        }

        int totalDepth = GetCurrentStackDepth();
        std::string renumberedStack;
        int stackIndex = 0;
    
        while (pos < stackContent.size()) {
            size_t nextPos = stackContent.find(NEWLINE_CHARACTER, pos);
            if (nextPos == std::string::npos) {
                nextPos = stackContent.size();
            }
            std::string stackEntry = stackContent.substr(pos, nextPos - pos);
            size_t spacePos = stackEntry.find(' ');
            if (spacePos != std::string::npos) {
                std::string moduleName = stackEntry.substr(spacePos + 1);
                renumberedStack += std::string(MODULE_IMPORT_STACK_NUMBER_LABEL)
                    + FormatStackNumber(stackIndex, totalDepth) + " " + moduleName;
                stackIndex++;
            }
            if (nextPos == pos) {
                nextPos++;
            }
            pos = nextPos;
        }
        moduleManager_->moduleImportData_ = std::string(MODULE_IMPORT_STACK_HEAD) + renumberedStack;
    }
    
    int GetNumberOfDigits(int number) const
    {
        if (number == 0) return 1;
        int digits = 0;
        while (number > 0) {
            number /= 10; // 10:Remaining number.
            digits++;
        }
        return digits;
    }

    std::string FormatStackNumber(int stackIndex, int totalDepth) const
    {
        int width = GetNumberOfDigits(totalDepth - 1);
        std::string formattedNumber = std::to_string(stackIndex);
        if (formattedNumber.length() < static_cast<size_t>(width)) {
            formattedNumber = std::string(width - formattedNumber.length(), ALIGNMENT_SYMBOL) + formattedNumber;
        }
        return formattedNumber;
    }

    int GetCurrentStackDepth() const
    {
        std::string stackContent = moduleManager_->moduleImportData_.substr(MODULE_IMPORT_STACK_HEAD_LENGTH);
        int depth = 0;
        size_t pos = 0;
        while (pos < stackContent.size()) {
            if (stackContent[pos] == NEWLINE_CHARACTER[0]) {
                depth++;
            }
            pos++;
        }
        return depth;
    }

    std::string TruncateModuleImportStack(std::string_view stackData, size_t maxSize)
    {
        if (stackData.size() <= maxSize) {
            return std::string(stackData);
        }
        size_t halfSize = (maxSize - ELLIPSIS_PLACEHOLDER_LENGTH) / 2; // 2:Divided into head and tail parts.
        size_t headEnd = halfSize;
        while (headEnd > 0 && stackData[headEnd] != NEWLINE_CHARACTER[0]) {
            headEnd--;
        }
        if (headEnd == 0) {
            headEnd = halfSize;
        }
        size_t tailStart = stackData.size() - halfSize;
        while (tailStart < stackData.size() && stackData[tailStart] != NEWLINE_CHARACTER[0]) {
            tailStart++;
        }
        if (tailStart >= stackData.size()) {
            tailStart = stackData.size() - halfSize;
        }
        std::string result;
        result.reserve(maxSize);
        result.append(stackData.substr(0, headEnd));
        result.append(ELLIPSIS_PLACEHOLDER);
        result.append(stackData.substr(tailStart));
        return result;
    }

    bool enableModuleStack_ {false};
    ModuleManager* moduleManager_;
    CString moduleName_;
    uintptr_t handle_ {0};
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_MODULE_MODULE_TOOLS_H
