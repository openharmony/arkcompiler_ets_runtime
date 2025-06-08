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
static const Version VERSION(2, 0, 0);
std::string RAWHEAP_TRANSLATE_HELPER =
    "Usage: rawheap_translator <filename.rawheap> <filename.heapsnapshot>\n"
    "at least 1 argv provide, you can also extend to include <filename.heapsnapshot>, "
    "if output file not available, an automatic one will be generated after all.";

bool ParseMetaData(FileReader &file, MetaParser *parser)
{
    if (!file.CheckAndGetHeaderAt(file.GetFileSize() - sizeof(uint64_t), 0)) {
        LOG_ERROR_ << "rawheap header error!";
        return false;
    }

    std::vector<char> metadata(file.GetHeaderRight());
    if (!file.Seek(file.GetHeaderLeft()) || !file.Read(metadata.data(), file.GetHeaderRight())) {
        LOG_ERROR_ << "read metadata failed!";
        return false;
    }

    cJSON *json = cJSON_ParseWithOpts(metadata.data(), nullptr, true);
    if (json == nullptr) {
        LOG_ERROR_ << "metadata cjson parse failed!";
        return false;
    }

    bool ret = parser->Parse(json);
    cJSON_Delete(json);
    return ret;
}

RawHeap *ParseRawheap(FileReader &file, MetaParser *metaParser)
{
    Version version;
    if (!version.Parse(RawHeap::ReadVersion(file))) {
        return nullptr;
    }

    if (VERSION < version) {
        LOG_ERROR_ << "The rawheap file's version " << version.ToString()
                   << " is not matched the current rawheap translator,"
                   << " please use the newest version of the translator!";
        return nullptr;
    }

    if (Version(1, 0, 0) < version) {
        return new RawHeapTranslateV2(metaParser);
    }

    return new RawHeapTranslateV1(metaParser);
}

void Translate(const std::string &inputPath, const std::string &outputPath)
{
    auto start = std::chrono::steady_clock::now();
    FileReader file;
    if (!file.Initialize(inputPath)) {
        return;
    }

    uint64_t fileSize = FileReader::GetFileSize(inputPath);
    if (!file.CheckAndGetHeaderAt(fileSize - sizeof(uint64_t), 0)) {
        LOG_ERROR_ << "Read rawheap file header failed!";
        return;
    }

    MetaParser metaParser;
    if (!ParseMetaData(file, &metaParser)) {
        return;
    }

    RawHeap *rawheap = ParseRawheap(file, &metaParser);
    if (rawheap == nullptr) {
        return;
    }

    if (!rawheap->Parse(file, file.GetHeaderLeft())) {
        delete rawheap;
        return;
    }

    rawheap->Translate();
    StreamWriter writer;
    if (!writer.Initialize(outputPath)) {
        return;
    }

    HeapSnapshotJSONSerializer::Serialize(rawheap, &writer);
    delete rawheap;
    auto end = std::chrono::steady_clock::now();
    int duration = (int)std::chrono::duration<double>(end - start).count();
    LOG_INFO_ << "File save to " << outputPath << ", cost " << std::to_string(duration) << 's';
}

bool ParseArgs(const int argc, const char **argv, std::string &input, std::string &output)
{
    const int minArgc = 2;  // 2: at least 1 argv provide, including <filename.rawheap>
    const int maxArgc = 3;  // 3: also extend to include output file <filename.heapsnapshot>
    if (argc < minArgc || argc > maxArgc) {
        std::cout << "Input error!\n" << RAWHEAP_TRANSLATE_HELPER << std::endl;
        return false;
    }

    int newArgc = 1;
    std::string rawheapPathOrVersionCheck = argv[newArgc];
    if (rawheapPathOrVersionCheck == "--version" || rawheapPathOrVersionCheck == "-v") {
        std::cout << VERSION.ToString() << std::endl;
        return false;
    }

    if (rawheapPathOrVersionCheck == "--help" || rawheapPathOrVersionCheck == "-h") {
        std::cout << RAWHEAP_TRANSLATE_HELPER << std::endl;
        return false;
    }

    if (!EndsWith(rawheapPathOrVersionCheck, ".rawheap")) {
        std::cout << "The second argument must be rawheap file!" << std::endl
                  << RAWHEAP_TRANSLATE_HELPER << std::endl;
        return false;
    }

    newArgc++;
    std::string outputPath {};
    if (newArgc < argc) {
        outputPath = argv[newArgc];
        if (!EndsWith(outputPath, ".heapsnapshot")) {
            std::cout << "The last argument must be heapsnapshot file!" << std::endl
                      << RAWHEAP_TRANSLATE_HELPER << std::endl;
            return false;
        }
    } else {
        if (!GenerateDumpFileName(outputPath)) {
            std::cout << "Generate dump file name failed!\n";
            return false;
        }
    }

    input = rawheapPathOrVersionCheck;
    output = outputPath;
    return true;
}

int Main(const int argc, const char **argv)
{
    std::string rawheapPath;
    std::string snapshotPath;
    if (!ParseArgs(argc, argv, rawheapPath, snapshotPath)) {
        return 0;
    }

    Translate(rawheapPath, snapshotPath);
    return 0;
}

}  // namespace rawheap_translate
int main(int argc, const char **argv)
{
    return rawheap_translate::Main(argc, argv);
}