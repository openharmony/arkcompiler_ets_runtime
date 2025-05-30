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

#include <chrono>
#include "ecmascript/dfx/hprof/rawheap_translate/metadata_parse.h"
#include "ecmascript/dfx/hprof/rawheap_translate/rawheap_translate.h"
#include "ecmascript/dfx/hprof/rawheap_translate/serializer.h"

namespace rawheap_translate {
std::string RAWHEAP_TRANSLATE_HELPER =
    "Usage: rawheap_translator <filename.rawheap> <filename.heapsnapshot>\n"
    "at least 1 argv provide, you can also extend to include <filename.heapsnapshot>, "
    "if output file not available, an automatic one will be generated after all.";

void Translate(const std::string &inputPath, const std::string &outputPath)
{
    FileReader file;
    if (!file.Initialize(inputPath)) {
        return;
    }

    uint64_t fileSize = FileReader::GetFileSize(inputPath);
    if (!file.CheckAndGetHeaderAt(fileSize - sizeof(uint64_t), 0)) {
        LOG_ERROR_ << "Read rawheap file header failed!";
        return;
    }

    std::vector<char> metadata(file.GetHeaderRight());
    if (!file.Seek(file.GetHeaderLeft()) || !file.Read(metadata.data(), file.GetHeaderRight())) {
        LOG_ERROR_ << "Read rawheap file metadata failed!";
        return;
    }

    cJSON *json = cJSON_ParseWithOpts(metadata.data(), nullptr, true);
    if (json == nullptr) {
        LOG_ERROR_ << "Metadata cjson parse failed!";
        return;
    }

    MetaParser metaParser;
    bool ret = metaParser.Parse(json);
    cJSON_Delete(json);
    if (!ret || !CheckVersion(metaParser.GetMetaVersion())) {
        return;
    }

    RawHeapTranslateV1 translate(&metaParser);
    if (!translate.Parse(file, file.GetHeaderLeft())) {
        return;
    }

    LOG_INFO_ << "Start to translate rawheap!";
    translate.Translate();

    LOG_INFO_ << "Start to serialize!";
    StreamWriter writer;
    if (!writer.Initialize(outputPath)) {
        return;
    }

    HeapSnapshotJSONSerializer::Serialize(&translate, &writer);
}

int Main(const int argc, const char **argv)
{
    // 2: at least 1 argv provide, including <filename.rawheap>
    // 3: also extend to include output file <filename.heapsnapshot>
    if (argc < 2 || argc > 3) {
        LOG_ERROR_ << "Input error!\n" << RAWHEAP_TRANSLATE_HELPER;
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
        LOG_ERROR_ << "The second argument must be rawheap file!\n" << RAWHEAP_TRANSLATE_HELPER;
        return -1;
    }

    newArgc++;
    std::string outputPath {};
    if (newArgc < argc) {
        outputPath = argv[newArgc];
        if (!EndsWith(outputPath, ".heapsnapshot")) {
            LOG_ERROR_ << "The last argument must be heapsnapshot file!\n" << RAWHEAP_TRANSLATE_HELPER;
            return -1;
        }
    } else {
        if (!GenerateDumpFileName(outputPath)) {
            LOG_ERROR_ << "Generate dump file name failed!\n";
            return -1;
        }
    }

    auto start = std::chrono::steady_clock::now();
    Translate(rawheapPathOrVersionCheck, outputPath);
    auto end = std::chrono::steady_clock::now();
    int duration = (int)std::chrono::duration<double>(end - start).count();
    LOG_INFO_ << "Translate success! file save to " << outputPath << ", cost " << std::to_string(duration) << 's';
    return 0;
}

}  // namespace rawheap_translate
int main(int argc, const char **argv)
{
    return rawheap_translate::Main(argc, argv);
}