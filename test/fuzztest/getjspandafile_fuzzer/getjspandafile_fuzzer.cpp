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

#include "getjspandafile_fuzzer.h"
#include "ecmascript/ohos/ohos_pkg_args.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::kungfu;

namespace OHOS {

    void GetJSPandaFileFuzzTest([[maybe_unused]]const uint8_t *data, size_t size)
    {
        constexpr size_t UINT32_SIZE = sizeof(uint32_t);
        
        // requires at least 12 bytes to properly extract and interpret three 32-bit unsigned integers (uint32_t)
        constexpr size_t MIN_SIZE_REQUIRED = UINT32_SIZE * 3;
        if (size < MIN_SIZE_REQUIRED) {
            return;
        }

        RuntimeOption option;
        JSRuntimeOptions runtimeOptions;

        uint32_t pathLen = *reinterpret_cast<const uint32_t*>(data);
        data += UINT32_SIZE;
        size -= UINT32_SIZE;

        std::string hapPath(reinterpret_cast<const char*>(data), pathLen);
        data += pathLen;
        size -= pathLen;

        uint32_t offset = *reinterpret_cast<const uint32_t*>(data);
        data += UINT32_SIZE;
        size -= UINT32_SIZE;

        uint32_t fileSize = *reinterpret_cast<const uint32_t*>(data);
        data += UINT32_SIZE;
        size -= UINT32_SIZE;

        // set runtimeOptions
        runtimeOptions.SetHapPath(hapPath);
        runtimeOptions.SetHapAbcOffset(offset);
        runtimeOptions.SetHapAbcSize(fileSize);

        std::shared_ptr<JSPandaFile> pf;
        OhosPkgArgs pkgArgs;

        pkgArgs.GetJSPandaFile(runtimeOptions, pf);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::GetJSPandaFileFuzzTest(data, size);
    return 0;
}