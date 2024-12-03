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

#include "ecmascript/dfx/hprof/rawheap_translate/rawheap_translate.h"
#include "ecmascript/dfx/hprof/rawheap_translate/serializer.h"

namespace rawheap_translate {
std::string RAWHEAP_TRANSLATE_HELPER =
    "Usage: rawheap_translator <filename.rawheap> <filename.heapsnapshot>\n"
    "at least 1 argv provide, you can also extend to include <filename.heapsnapshot>, "
    "if output file not available, an automatic one will be generated after all.";

int Main(const int argc, const char **argv)
{
    // 2: at least 1 argv provide, including <filename.rawheap>
    // 3: also extend to include output file <filename.heapsnapshot>
    if (argc < 2 || argc > 3) {
        LOG_ERROR("Main: input error!\n" + RAWHEAP_TRANSLATE_HELPER);
        return -1;
    }

    int newArgc = 1;
    std::string rawheapPathOrVersionCheck = argv[newArgc];
    if (rawheapPathOrVersionCheck == "-V" || rawheapPathOrVersionCheck == "-v") {
        std::cout << VERSION[MAJOR_VERSION_INDEX]
                  << '.' << VERSION[MINOR_VERSION_INDEX]
                  << '.' << VERSION[BUILD_VERSION_INDEX] << std::endl;
        return 0;
    }

    if (!EndsWith(rawheapPathOrVersionCheck, ".rawheap")) {
        LOG_ERROR("The second argument must be rawheap file!\n" + RAWHEAP_TRANSLATE_HELPER);
        return -1;
    }

    newArgc++;
    std::string outputPath {};
    if (newArgc < argc) {
        outputPath = argv[newArgc];
        if (!EndsWith(outputPath, ".heapsnapshot")) {
            LOG_ERROR("The last argument must be heapsnapshot file!\n" + RAWHEAP_TRANSLATE_HELPER);
            return -1;
        }
    } else {
        if (!GenerateDumpFileName(outputPath)) {
            LOG_ERROR("Generate dump file name failed!\n");
            return -1;
        }
    }

    LOG_INFO("Main: start to translate rawheap!");
    RawHeapTranslate translate;
    if (!translate.Translate(rawheapPathOrVersionCheck)) {
        return -1;
    }

    LOG_INFO("Main: start to serialize!");
    StreamWriter writer;
    if (!writer.Initialize(outputPath)) {
        return -1;
    }

    HeapSnapshotJSONSerializer::Serialize(&translate, &writer);
    LOG_INFO("Main: translate success! file save to " + outputPath);
    return 0;
}

}  // namespace rawheap_translate
int main(int argc, const char **argv)
{
    return rawheap_translate::Main(argc, argv);
}