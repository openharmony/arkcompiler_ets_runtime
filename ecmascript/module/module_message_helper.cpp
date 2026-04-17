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

#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/module_message_helper.h"

#include "ecmascript/module/module_logger.h"

namespace panda::ecmascript {

bool ModuleMessageHelper::EnsureResultFile(const EcmaVM *vm, const uint32_t tid,
                                           const std::string_view fileSuffix, std::string &path)
{
    CString bundleName = vm->GetBundleName();
    path = base::ConcatToCString(FILEDIR, bundleName);
    if (vm->IsWorkerThread()) {
        base::AppendToBaseString(path, "_", std::to_string(tid));
    }
    path += fileSuffix;
    
    if (access(path.c_str(), F_OK) == 0) {
        if (access(path.c_str(), W_OK) == 0) {
            return true;
        }
        LOG_ECMA(ERROR) << "file exists but not writable";
        return false;
    }
    
    constexpr mode_t defaultMode = S_IRUSR | S_IWUSR | S_IRGRP;
    int fd = creat(path.c_str(), defaultMode);
    if (fd == -1) {
        LOG_ECMA(ERROR) << "file create failed, errno = " << errno << " path:" << path;
        return false;
    }
    close(fd);
    return true;
}

void ModuleMessageHelper::PrintCircularImportModuleStack(JSThread *thread, const CString &moduleName,
                                                         std::string_view stack)
{
    std::string path;
    std::string circularImport = base::ConcatToStdString("_", std::to_string(getpid()), CIRCULAR_IMPORT_FILE);
    if (!EnsureResultFile(thread->GetEcmaVM(), os::thread::GetCurrentThreadId(),
        std::string_view(circularImport), path)) {
        LOG_ECMA(ERROR) << "Failed to open circular import log file for module: " << moduleName;
        return;
    }
    std::ofstream fileHandle;
    fileHandle.open(path, std::ios_base::app);
    if (!fileHandle.is_open()) {
        LOG_ECMA(ERROR) << "Circular Module Import file open failed: " << path;
        return;
    }
    std::string moduleStack(stack);
    if (moduleStack.empty()) {
        LOG_ECMA(ERROR) << "Circular import stack trace is empty for module: " << moduleName;
        return;
    }
    std::string start = base::ConcatToStdString("circular Module Name: ", moduleName);
    fileHandle << start << moduleStack << "\n";
    if (!fileHandle.good()) {
        LOG_ECMA(ERROR) << "Failed to write circular import stack trace to file: " << path;
    }
    fileHandle << "\n";
    fileHandle.close();
}
}  // namespace panda::ecmascript