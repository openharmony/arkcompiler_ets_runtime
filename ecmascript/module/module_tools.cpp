/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/module/module_tools.h"

#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/module/module_message_helper.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/platform/dfx_crash_obj.h"

namespace panda::ecmascript {
ModuleImportStackScope::ModuleImportStackScope(JSThread *thread, JSHandle<SourceTextModule> module)
    : enableModuleStack_(thread->GetEcmaVM()->GetJSOptions().EnableModuleImportStack())
{
    if (enableModuleStack_) {
        moduleName_ = SourceTextModule::GetModuleName(module.GetTaggedValue());
        moduleManager_ = thread->GetModuleManager();
        if (!moduleName_.empty()) {
            PushModuleImportStack(moduleName_);
            // Check for circular dependency
            auto &moduleImportSet = moduleManager_->GetModuleImportSet();
            if (moduleImportSet.find(moduleName_) != moduleImportSet.end()) {
                LOG_ECMA(WARN) << "Circular Module: circular dependency detected for module: " << moduleName_;
                ModuleMessageHelper::PrintCircularImportModuleStack(thread, moduleName_,
                    moduleManager_->GetModuleImportStackData());
            } else {
                moduleImportSet.insert(moduleName_);
            }
            truncatedStack_ = moduleManager_->GetImportStackDataForCPPCrash(
                moduleName_, 64 * 1024); // 64 * 1024 = 64KB
            handle_ = SetCrashObject(DFXObjectType::STRING,
                reinterpret_cast<uintptr_t>(truncatedStack_.data()));
        }
    }
}

ModuleImportStackScope::~ModuleImportStackScope()
{
    if (enableModuleStack_) {
        ResetCrashObject(handle_);
        PopModuleImportStack(moduleName_);
        // Remove from module import set
        moduleManager_->GetModuleImportSet().erase(moduleName_);
    }
}

void ModuleImportStackScope::PushModuleImportStack(const CString &moduleName)
{
    LOG_ECMA(DEBUG) << "ModuleImportStack: start push module:" << moduleName;
    if (moduleName.empty()) {
        LOG_ECMA(ERROR) << "ModuleImportStack: push moduleName is null";
        return;
    }
    std::string stackEntry = std::string(MODULE_IMPORT_STACK_NUMBER_LABEL)
        + ALIGNMENT_SYMBOL + " " + std::string(moduleName); // "\\n#0 " + moduleName
    moduleManager_->InsertModuleImportStackEntry(stackEntry, MODULE_IMPORT_STACK_HEAD_LENGTH);
    RenumberModuleImportStack();
}

void ModuleImportStackScope::PopModuleImportStack(const CString &moduleName)
{
    LOG_ECMA(DEBUG) << "ModuleImportStack: start pop module:" << moduleName;
    std::string stackContent = std::string(moduleManager_
        ->GetModuleImportStackContent(MODULE_IMPORT_STACK_HEAD_LENGTH));
    if (stackContent.empty()) {
        LOG_ECMA(ERROR) << "ModuleImportStack: pop module failed: nothing to pop";
        return;
    }

    size_t nextPos = stackContent.find(NEWLINE_CHARACTER, 1);
    if (nextPos == std::string::npos) {
        nextPos = stackContent.size();
    }
    stackContent.erase(0, nextPos);
    moduleManager_->SetModuleImportStackContent(stackContent, MODULE_IMPORT_STACK_HEAD);
    RenumberModuleImportStack();
}

void ModuleImportStackScope::RenumberModuleImportStack()
{
    std::string stackContent = std::string(moduleManager_
        ->GetModuleImportStackContent(MODULE_IMPORT_STACK_HEAD_LENGTH));
    size_t pos = 0;
    if (!stackContent.empty() && stackContent[0] == NEWLINE_CHARACTER[0]) {
        pos = 1;
    }
    int totalDepth = GetModuleImportStackDepth();
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
    moduleManager_->SetModuleImportStackContent(renumberedStack, MODULE_IMPORT_STACK_HEAD);
}

int ModuleImportStackScope::GetNumberOfDigits(int number) const
{
    if (number == 0) return 1;
    int digits = 0;
    while (number > 0) {
        number /= 10; // 10:Remaining number.
        digits++;
    }
    return digits;
}

std::string ModuleImportStackScope::FormatStackNumber(int stackIndex, int totalDepth) const
{
    int width = GetNumberOfDigits(totalDepth - 1);
    std::string formattedNumber = std::to_string(stackIndex);
    if (formattedNumber.length() < static_cast<size_t>(width)) {
        formattedNumber = std::string(width - formattedNumber.length(), ALIGNMENT_SYMBOL) + formattedNumber;
    }
    return formattedNumber;
}

int ModuleImportStackScope::GetModuleImportStackDepth() const
{
    std::string stackContent = std::string(moduleManager_
        ->GetModuleImportStackContent(MODULE_IMPORT_STACK_HEAD_LENGTH));
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

std::string ModuleTools::TruncateModuleImportStack(std::string_view stackContent, size_t maxSize)
{
    if (stackContent.size() <= maxSize) {
        return std::string(stackContent);
    }

    size_t halfSize = (maxSize -
        ModuleImportStackScope::ELLIPSIS_PLACEHOLDER_LENGTH) / 2; // 2:Divided into head and tail parts.
    size_t headEnd = halfSize;
    while (headEnd > 0 && stackContent[headEnd] != ModuleImportStackScope::NEWLINE_CHARACTER[0]) {
        headEnd--;
    }
    if (headEnd == 0) {
        headEnd = halfSize;
    }
    size_t tailStart = stackContent.size() - halfSize;
    while (tailStart < stackContent.size() && stackContent[tailStart] != ModuleImportStackScope::NEWLINE_CHARACTER[0]) {
        tailStart++;
    }
    if (tailStart >= stackContent.size()) {
        tailStart = stackContent.size() - halfSize;
    }
    std::string result;
    result.reserve(maxSize);
    result.append(stackContent.substr(0, headEnd));
    result.append(ModuleImportStackScope::ELLIPSIS_PLACEHOLDER);
    result.append(stackContent.substr(tailStart));
    return result;
}

std::string ModuleTools::TruncateImportStackForCPPCrash(const CString &moduleName,
    std::string_view stackData, size_t maxSize)
{
    std::string_view stackContent = stackData.substr(ModuleImportStackScope::MODULE_IMPORT_STACK_HEAD_LENGTH);
    std::string cppCrashHead = base::ConcatToStdString("Failed to load ", moduleName,
        ", the dependency import call stack is as follows");
    std::string truncatedStack = TruncateModuleImportStack(stackContent,
        maxSize - cppCrashHead.size());
    std::string result = base::ConcatToStdString(cppCrashHead, truncatedStack);
    return result;
}

std::string ModuleTools::TruncateImportStackForJSCrash(std::string_view stackData, size_t maxSize)
{
    std::string_view stackContent = stackData.substr(ModuleImportStackScope::MODULE_IMPORT_STACK_HEAD_LENGTH);
    std::string truncatedStack = TruncateModuleImportStack(stackContent,
        maxSize - ModuleImportStackScope::MODULE_IMPORT_STACK_HEAD_LENGTH);
    std::string result = base::ConcatToStdString(ModuleImportStackScope::MODULE_IMPORT_STACK_HEAD, truncatedStack);
    return result;
}
} // namespace panda::ecmascript
