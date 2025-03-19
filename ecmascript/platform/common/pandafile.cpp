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

#include "ecmascript/module/module_path_helper.h"

namespace panda::ecmascript {
using PathHelper = base::PathHelper;

// use "@bundle" as ohmurl's rules, will be abandon later
std::pair<CString, CString> ParseAbcEntryPoint([[maybe_unused]] JSThread *thread, const CString &filename,
    [[maybe_unused]] std::string_view entryPoint)
{
    CString name;
    CString entry;
    [[maybe_unused]] EcmaVM *vm = thread->GetEcmaVM();
#if defined(OHOS_UNIT_TEST)
    return {filename, entryPoint.data()};
#endif
    CString normalName = PathHelper::NormalizePath(filename);
    ModulePathHelper::ParseAbcPathAndOhmUrl(vm, normalName, name, entry);
    if (name.empty()) {
        name = vm->GetAssetPath();
    }
    return std::make_pair(name, entry);
}

CString GetAssetPath(EcmaVM *vm)
{
    return vm->GetAssetPath();
}

void LoadJSPandaFileFailLog([[maybe_unused]] const CString &failLog)
{
    return;
}

std::unique_ptr<const panda_file::File> OpenPandaFileFromMemory(uint8_t *buffer, size_t size)
{
    return panda_file::OpenPandaFileFromMemory(buffer, size);
}

std::unique_ptr<const panda_file::File> ParseAndOpenPandaFile(const void *buffer, size_t size,
    [[maybe_unused]] const CString &filename)
{
    CString tag = ModulePathHelper::ParseFileNameToVMAName(filename);
    constexpr size_t PR_SET_VMA_ANON_NAME_MAX_LEN = 80;
    constexpr size_t ANON_FLAG_LEN = 7; // [anon:]
    if (tag.length() > PR_SET_VMA_ANON_NAME_MAX_LEN - ANON_FLAG_LEN) {
        tag = CString(ModulePathHelper::VMA_NAME_ARKTS_CODE);
    }
    return panda_file::OpenPandaFileFromMemory(buffer, size, tag.c_str());
}
} // namespace panda::ecmascript