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

#ifndef PROFILE_TYPE_H
#define PROFILE_TYPE_H

static constexpr uint8_t kProfileMagic[] = {'m', 'a', 'p', 'l', 'e', '.', 'p', 'r', 'o', 'f', 'i', 'l', 'e', '\0'};
static constexpr uint8_t kVer[] = {0, 0, 1};

enum ProfileFileType : uint8_t { kSystemServer = 0x00, kApp = 0x01 };

enum ProfileType : uint8_t {
    kFunction = 0x00,
    kClassMeta = 0x01,
    kFieldMeta = 0x02,
    kMethodMeta = 0x03,
    kReflectionStr = 0x04,
    kLiteral = 0x05,
    kBBInfo = 0x06,
    kIRCounter = 0x07,
    kAll = 0x08,
    kMethodSig = 0x09,
    kFileDesc = 0xFF
};

enum FuncIRItemIndex : uint8_t { kFuncIRItemNameIndex, kCounterStartIndex, kCounterEndIndex, kHashIndex };

enum FuncItemIndex : uint8_t { kFuncItemNameIndex, kFuncTypeIndex, kFuncCallTimesIndex };

enum FuncNameIndex : uint8_t {
    kClassNameIndex,
    kFuncNameIndex,
    kSignatureNameIndex,
};

struct ProfileDataInfo {
    uint32_t profileDataOff;
    uint8_t profileType;
    uint8_t mapleFileNum;
    uint16_t pad = 0;
    ProfileDataInfo() = default;
    ProfileDataInfo(uint32_t profileDataOff, uint8_t profileType, uint8_t mapleFileNum)
        : profileDataOff(profileDataOff), profileType(profileType), mapleFileNum(mapleFileNum)
    {
    }
};

struct FunctionItem {
    uint32_t classIdx;
    uint32_t methodIdx;
    uint32_t sigIdx;
    uint32_t callTimes;
    uint8_t type;
    FunctionItem(uint32_t classIdx, uint32_t methodIdx, uint32_t sigIdx, uint32_t callTimes, uint8_t type)
        : classIdx(classIdx), methodIdx(methodIdx), sigIdx(sigIdx), callTimes(callTimes), type(type)
    {
    }
};

struct FunctionIRProfItem {
    uint64_t hash;
    uint32_t classIdx;
    uint32_t methodIdx;
    uint32_t sigIdx;
    uint32_t counterStart;
    uint32_t counterEnd;
    FunctionIRProfItem(uint64_t hash, uint32_t classIdx, uint32_t methodIdx, uint32_t sigIdx, uint32_t start,
                       uint32_t end)
        : hash(hash), classIdx(classIdx), methodIdx(methodIdx), sigIdx(sigIdx), counterStart(start), counterEnd(end)
    {
    }
};

struct FuncCounterItem {
    uint32_t callTimes;
    FuncCounterItem(uint32_t callTimes) : callTimes(callTimes) {}
};

struct MetaItem {
    uint32_t idx;
    MetaItem(uint32_t idx) : idx(idx) {}
};

struct MethodSignatureItem {
    uint32_t methodIdx;
    uint32_t sigIdx;
    MethodSignatureItem(uint32_t methodIdx, uint32_t sigIdx) : methodIdx(methodIdx), sigIdx(sigIdx) {}
};

struct ReflectionStrItem {
    uint8_t type;
    uint32_t idx;
    ReflectionStrItem(uint32_t idx, uint8_t type) : type(type), idx(idx) {}
};

struct MapleFileProf {
    uint32_t idx;
    uint32_t num;
    uint32_t size;
    MapleFileProf(uint32_t idx, uint32_t num, uint32_t size) : idx(idx), num(num), size(size) {}
};

constexpr int kMagicNum = 14;
constexpr int kVerNum = 3;
constexpr int kCheckSumNum = 4;
struct Header {
    uint8_t magic[kMagicNum] = {};
    uint8_t ver[kVerNum] = {};
    uint8_t checkSum[kCheckSumNum] = {};
    uint8_t profileNum = 0;
    uint8_t profileFileType = 0;
    uint8_t pad = 0;
    uint32_t headerSize = 0;
    uint32_t stringCount = 0;
    uint32_t stringTabOff = 0;
    ProfileDataInfo data[1] = {};  // profile data info detemined by runtime
};

#endif
