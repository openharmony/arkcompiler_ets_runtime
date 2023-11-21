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

#ifndef MAPLE_IR_INCLUDE_BIN_MPLT_H
#define MAPLE_IR_INCLUDE_BIN_MPLT_H
#include "mir_module.h"
#include "mir_nodes.h"
#include "mir_preg.h"
#include "parser_opt.h"
#include "bin_mpl_export.h"
#include "bin_mpl_import.h"

namespace maple {
class BinaryMplt {
public:
    explicit BinaryMplt(MIRModule &md) : mirModule(md), binImport(md), binExport(md) {}

    virtual ~BinaryMplt() = default;

    void Export(const std::string &suffix, std::unordered_set<std::string> *dumpFuncSet = nullptr)
    {
        binExport.Export(suffix, dumpFuncSet);
    }

    bool Import(const std::string &modID, bool readCG = false, bool readSE = false)
    {
        importFileName = modID;
        return binImport.Import(modID, readCG, readSE);
    }

    const MIRModule &GetMod() const
    {
        return mirModule;
    }

    BinaryMplImport &GetBinImport()
    {
        return binImport;
    }

    BinaryMplExport &GetBinExport()
    {
        return binExport;
    }

    std::string &GetImportFileName()
    {
        return importFileName;
    }

    void SetImportFileName(const std::string &fileName)
    {
        importFileName = fileName;
    }

private:
    MIRModule &mirModule;
    BinaryMplImport binImport;
    BinaryMplExport binExport;
    std::string importFileName;
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_BIN_MPLT_H
