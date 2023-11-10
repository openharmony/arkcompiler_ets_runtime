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

#ifndef MAPLE_IR_INCLUDE_BIN_MIR_FILE_H
#define MAPLE_IR_INCLUDE_BIN_MIR_FILE_H
#include <string>
#include "types_def.h"

namespace maple {
const std::string kBinMirFileID = "HWCMPL";  // for magic in file header
constexpr uint8 kVersionMajor = 0;           // experimental version
constexpr uint8 kVersionMinor = 1;
constexpr int kMagicSize = 7;

enum BinMirFileType {
    kMjsvmFileTypeCmplV1,
    kMjsvmFileTypeCmpl,  // kCmpl v2 is the release version of
    kMjsvmFileTypeUnknown
};

inline uint8 MakeVersionNum(uint8 major, uint8 minor)
{
    uint8 mj = major & 0x0Fu;
    uint8 mn = minor & 0x0Fu;
    constexpr uint8 shiftNum = 4;
    return (mj << shiftNum) | mn;
}

// file header for binary format kMmpl, 8B in total
// Note the header is different with the specification
struct BinMIRFileHeader {
    char magic[kMagicSize];  // “HWCMPL”, or "HWLOS_"
    uint8 segNum;            // number of segments (e.g. one raw IR file is a segment unit)
    uint8 type;              // enum of type of VM file (e.g. MapleIR, TE)
    uint8 version;           // version of IR format (should be major.minor)
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_BIN_MIR_FILE_H
