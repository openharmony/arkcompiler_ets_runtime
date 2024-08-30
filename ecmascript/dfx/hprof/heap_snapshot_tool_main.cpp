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

#include <iostream>
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/napi/include/jsnapi.h"

namespace panda::ecmascript {

const std::string PC_TOOL_HELP_MSG =
    "Usage: ark_js_heap_snapshot_tool <bin_filename> <ouput_filename>\n"
    "\tWill decode the heapdump bin file to the target file named ouput_filename\n";

std::string GetHelper()
{
    std::string str;
    str.append(PC_TOOL_HELP_MSG);
    return str;
}

int HeapSnapshotPcToolMain(const int argc, const char **argv)
{
    if (argc < 3) { // 3: at least have 3 arguments
        std::cerr << GetHelper();
        return -1;
    }

    int newArgc = argc;
    std::string filePath = argv[argc - 2];
    std::string outputPath = argv[argc - 1];
    newArgc--;
    JSRuntimeOptions runtimeOptions;

    EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);
    if (vm == nullptr) {
        std::cerr << "Cannot Create vm" << std::endl;
        return -1;
    }
    std::cout << "(INFO) ark Decode: " << filePath << std::endl;
    DFXJSNApi::GenerateHeapSnapshotByBinFile(vm, filePath, outputPath);
    JSNApi::DestroyJSVM(vm);
    std::cout << "(INFO) ark HeapSnapshotPcToolMain finished" << std::endl;
    return 0;
}
}  // namespace panda::ecmascript


int main([[maybe_unused]] int argc, [[maybe_unused]] char const *argv[])
{
    return panda::ecmascript::HeapSnapshotPcToolMain(argc, argv);
}
