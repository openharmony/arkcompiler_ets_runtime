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

#include "ecmascript/platform/pandafile.h"

#include "ecmascript/log_wrapper.h"

namespace panda::ecmascript {
// use "@bundle" as ohmurl's rules, will be abandon later
std::pair<CString, CString> ParseAbcEntryPoint([[maybe_unused]] JSThread *thread, const CString &filename,
    [[maybe_unused]] std::string_view entryPoint)
{
    return {filename, entryPoint.data()};
}

CString GetAssetPath(EcmaVM *vm)
{
    return vm->GetAssetPath();
}

void LoadJSPandaFileFailLog([[maybe_unused]] const CString &failLog)
{
    LOG_NO_TAG(ERROR) << failLog;
}

std::unique_ptr<const panda_file::File> OpenPandaFileFromMemory(uint8_t *buffer, size_t size)
{
    return panda_file::OpenPandaFileFromSecureMemory(buffer, size);
}

std::unique_ptr<const panda_file::File> ParseAndOpenPandaFile(const void *buffer, size_t size,
    [[maybe_unused]] const CString &filename)
{
    return panda_file::OpenPandaFileFromMemory(buffer, size);
}
} // namespace panda::ecmascript