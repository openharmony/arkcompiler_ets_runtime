/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_PLATFORM_PANDAFILE_H
#define ECMASCRIPT_PLATFORM_PANDAFILE_H

#include <string>
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/js_thread.h"
#include "os/file.h"

namespace panda::ecmascript {
    std::pair<CString, CString> ParseAbcEntryPoint([[maybe_unused]] JSThread *thread, const CString &filename,
        [[maybe_unused]] std::string_view entryPoint);
    CString GetAssetPath(EcmaVM *vm);
    void LoadJSPandaFileFailLog([[maybe_unused]] const CString &failLog);
    std::unique_ptr<const panda_file::File> OpenPandaFileFromMemory(uint8_t *buffer, size_t size);
    std::unique_ptr<const panda_file::File> ParseAndOpenPandaFile(const void *buffer, size_t size,
        [[maybe_unused]] const CString &filename);
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_PLATFORM_PANDAFILE_H