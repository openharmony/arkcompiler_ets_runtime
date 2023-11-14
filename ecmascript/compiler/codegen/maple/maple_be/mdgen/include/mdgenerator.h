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

#ifndef MAPLEBE_MDGEN_INCLUDE_MDGENERATOR_H
#define MAPLEBE_MDGEN_INCLUDE_MDGENERATOR_H
#include <fstream>
#include "mdrecord.h"
#include "mpl_logging.h"

namespace MDGen {
class MDCodeGen {
public:
    MDCodeGen(const MDClassRange &inputRange, const std::string &oFileDirArg)
        : curKeeper(inputRange), outputFileDir(oFileDirArg)
    {
    }
    virtual ~MDCodeGen() = default;

    const std::string &GetOFileDir() const
    {
        return outputFileDir;
    }
    void SetTargetArchName(const std::string &archName) const
    {
        targetArchName = archName;
    }

    void EmitCheckPtr(std::ofstream &outputFile, const std::string &emitName, const std::string &name,
                      const std::string &ptrType) const;
    void EmitFileHead(std::ofstream &outputFile, const std::string &headInfo) const;
    MDClass GetSpecificClass(const std::string &className);

protected:
    MDClassRange curKeeper;

private:
    static std::string targetArchName;
    std::string outputFileDir;
};

class SchedInfoGen : public MDCodeGen {
public:
    SchedInfoGen(const MDClassRange &inputRange, const std::string &oFileDirArg) : MDCodeGen(inputRange, oFileDirArg) {}
    ~SchedInfoGen() override
    {
        if (outFile.is_open()) {
            outFile.close();
        }
    }

    void EmitArchDef();
    const std::string &GetArchName();
    void EmitUnitIdDef();
    void EmitUnitDef();
    void EmitUnitNameDef();
    void EmitLatencyDef();
    void EmitResvDef();
    void EmitBypassDef();
    void Run();

private:
    std::ofstream outFile;
};
} /* namespace MDGen */

#endif /* MAPLEBE_MDGEN_INCLUDE_MDGENERATOR_H */