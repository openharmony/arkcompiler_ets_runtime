/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <getopt.h>
#include "mdparser.h"
#include "mdgenerator.h"

using namespace MDGen;
namespace {
bool isGenSched = false;
std::string schedSrcPath = "";
std::string oFileDir = "";
}  // namespace

static int PrintHelpAndExit()
{
    maple::LogInfo::MapleLogger() << "Maplegen is usd to process md files and "
                                  << "generate architecture specific information in def files\n"
                                  << "usage: ./mplgen xxx.md outputdirectroy\n";
    return 1;
}

void ParseCommandLine(int argc, char **argv)
{
    int opt;
    int gOptionIndex = 0;
    std::string optStr = "s:o:";
    static struct option longOptions[] = {
        {"genSchdInfo", required_argument, NULL, 's'}, {"outDirectory", required_argument, NULL, 'o'}, {0, 0, 0, 0}};
    while ((opt = getopt_long(argc, argv, optStr.c_str(), longOptions, &gOptionIndex)) != -1) {
        switch (opt) {
            case 's':
                isGenSched = true;
                schedSrcPath = optarg;
                break;
            case 'o':
                oFileDir = optarg;
                break;
            default:
                break;
        }
    }
}

bool GenSchedFiles(const std::string &fileName, const std::string &fileDir)
{
    maple::MemPool *schedInfoMemPool = memPoolCtrler.NewMemPool("schedInfoMp", false /* isLcalPool */);
    MDClassRange moduleData("Schedule");
    MDParser parser(moduleData, schedInfoMemPool);
    if (!parser.ParseFile(fileName)) {
        delete schedInfoMemPool;
        return false;
    }
    SchedInfoGen schedEmiiter(moduleData, fileDir);
    schedEmiiter.Run();
    delete schedInfoMemPool;
    return true;
}

int main(int argc, char **argv)
{
    constexpr int minimumArgNum = 2;
    if (argc <= minimumArgNum) {
        return PrintHelpAndExit();
    }
    ParseCommandLine(argc, argv);
    if (isGenSched) {
        if (!GenSchedFiles(schedSrcPath, oFileDir)) {
            return 1;
        }
    }
    return 0;
}
