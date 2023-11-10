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

#include "mir_parser.h"
#include "debug_info.h"
#include "bin_mplt.h"
#include "opcode_info.h"
#include <cstdlib>
#include "mir_function.h"
#include "mir_type.h"
#include <iostream>
#include <fstream>

using namespace maple;

std::unordered_set<std::string> dumpFuncSet = {};

int main(int argc, char **argv)
{
    if (argc < k2BitSize) {
        MIR_PRINTF("usage: mpldbg foo.mpl\n");
        exit(1);
    }
    std::vector<maple::MIRModule *> themodule(argc, nullptr);
    bool useBinary = false;
    MIRSrcLang srcLang = kSrcLangUnknown;
    // process the options which must come first
    maple::int32 i = 1;
    while (argv[i][0] == '-') {
        if (argv[i][1] == 'b' && argv[i][k2BitSize] == '\0') {
            useBinary = true;
        } else if (strncmp(argv[i], "-dumpfunc=", k10BitSize) == 0 && strlen(argv[i]) > k10BitSize) {
            std::string funcName(&argv[i][k10BitSize]);
            dumpFuncSet.insert(funcName);
        } else if (strcmp(argv[i], "-srclang=java") == 0) {
            srcLang = kSrcLangJava;
        } else if (strcmp(argv[i], "-srclang=c") == 0) {
            srcLang = kSrcLangC;
        } else if (strcmp(argv[i], "-srclang=c++") == 0) {
            srcLang = kSrcLangCPlusPlus;
        } else {
            ERR(kLncErr, "mpldbg: unrecognized command line option");
            return 1;
        }
        i++;
    }
    // process the input files
    while (i < argc) {
        themodule[i] = new maple::MIRModule(argv[i]);
        themodule[i]->SetSrcLang(srcLang);
        std::string::size_type lastdot = themodule[i]->GetFileName().find_last_of(".");
        bool ismplt = themodule[i]->GetFileName().compare(lastdot, k5BitSize, ".mplt") == 0;
        bool istmpl = themodule[i]->GetFileName().compare(lastdot, k5BitSize, ".tmpl") == 0;
        bool ismpl = themodule[i]->GetFileName().compare(lastdot, k5BitSize, ".mpl\0") == 0;
        bool isbpl = themodule[i]->GetFileName().compare(lastdot, k5BitSize, ".bpl\0") == 0;
        if (!ismplt && !istmpl && !ismpl && !isbpl) {
            ERR(kLncErr, "mpldbg: input must be .mplt or .mpl or .bpl or .tmpl file");
            return 1;
        }
        // input the file
        if (ismpl || istmpl) {
            maple::MIRParser theparser(*themodule[i]);
            if (!theparser.ParseMIR()) {
                theparser.EmitError(themodule[i]->GetFileName().c_str());
                return 1;
            }
        } else {
            BinaryMplImport binMplt(*themodule[i]);
            binMplt.SetImported(false);
            std::string modid = themodule[i]->GetFileName();
            if (!binMplt.Import(modid, true)) {
                ERR(kLncErr, "mpldbg: cannot open .mplt or .bpl file: %s", modid.c_str());
                return 1;
            }
        }

        themodule[i]->GetDbgInfo()->BuildDebugInfo();
        themodule[i]->SetWithDbgInfo(true);

        // output the file
        themodule[i]->OutputAsciiMpl(".dbg", (ismpl || isbpl) ? ".mpl" : ".tmpl");
        i++;
    }
    return 0;
}
