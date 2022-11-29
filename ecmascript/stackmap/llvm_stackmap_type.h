/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef ECMASCRIPT_LLVM_STACKMAP_TYPE_H
#define ECMASCRIPT_LLVM_STACKMAP_TYPE_H

#include <iostream>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>
#include "ecmascript/common.h"
#include "ecmascript/log_wrapper.h"

namespace panda::ecmascript::kungfu {
using OffsetType = int32_t;
using DwarfRegType = uint16_t;
using LargeInt = int64_t;
using DwarfRegAndOffsetType = std::pair<DwarfRegType, OffsetType>;
using CallSiteInfo = std::vector<DwarfRegAndOffsetType>;
using Fun2InfoType = std::pair<uintptr_t, DwarfRegAndOffsetType>;
using Pc2CallSiteInfo = std::unordered_map<uintptr_t, CallSiteInfo>;
using FpDelta = std::pair<int, uint32_t>;
using Func2FpDelta = std::unordered_map<uintptr_t, FpDelta>; // value: fpDelta & funcSize
using ConstInfo = std::vector<OffsetType>;
using DeoptInfoType = std::vector<std::variant<OffsetType, LargeInt, DwarfRegAndOffsetType>>;
using Pc2Deopt = std::unordered_map<uintptr_t, DeoptInfoType>;

struct Header {
    uint8_t  stackmapversion; // Stack Map Version (current version is 3)
    uint8_t  Reserved0; // Reserved (expected to be 0)
    uint16_t Reserved1; // Reserved (expected to be 0)
    void Print() const
    {
        LOG_COMPILER(DEBUG) << "----- head ----";
        LOG_COMPILER(DEBUG) << "   version:" << static_cast<int>(stackmapversion);
        LOG_COMPILER(DEBUG) << "+++++ head ++++";
    }
};

#pragma pack(1)
struct StkSizeRecordTy {
    uintptr_t functionAddress;
    uint64_t stackSize;
    uint64_t recordCount;
    void Print() const
    {
        LOG_COMPILER(DEBUG) << "               functionAddress:0x" << std::hex << functionAddress;
        LOG_COMPILER(DEBUG) << "               stackSize:0x" << std::hex << stackSize;
        LOG_COMPILER(DEBUG) << "               recordCount:" << std::hex << recordCount;
    }
};
#pragma pack()

struct ConstantsTy {
    uintptr_t LargeConstant;
    void Print() const
    {
        LOG_COMPILER(DEBUG) << "               LargeConstant:0x" << std::hex << LargeConstant;
    }
};

struct StkMapRecordHeadTy {
    uint64_t PatchPointID;
    uint32_t InstructionOffset;
    uint16_t Reserved;
    uint16_t NumLocations;
    void Print() const
    {
        LOG_COMPILER(DEBUG) << "               PatchPointID:0x" << std::hex << PatchPointID;
        LOG_COMPILER(DEBUG) << "               instructionOffset:0x" << std::hex << InstructionOffset;
        LOG_COMPILER(DEBUG) << "               Reserved:0x" << std::hex << Reserved;
        LOG_COMPILER(DEBUG) << "               NumLocations:0x" << std::hex << NumLocations;
    }
};

struct  LocationTy {
    enum class Kind: uint8_t {
        REGISTER = 1,
        DIRECT = 2,
        INDIRECT = 3,
        CONSTANT = 4,
        CONSTANTNDEX = 5,
    };
    static constexpr int CONSTANT_FIRST_ELEMENT_INDEX = 3;
    static constexpr int CONSTANT_DEOPT_CNT_INDEX = 2;
    Kind location;
    uint8_t Reserved_0;
    uint16_t LocationSize;
    uint16_t DwarfRegNum;
    uint16_t Reserved_1;
    OffsetType OffsetOrSmallConstant;

    std::string PUBLIC_API TypeToString(Kind loc) const;

    void Print() const
    {
        LOG_COMPILER(DEBUG)  << TypeToString(location);
        LOG_COMPILER(DEBUG) << ", size:" << std::dec << LocationSize;
        LOG_COMPILER(DEBUG) << "\tDwarfRegNum:" << DwarfRegNum;
        LOG_COMPILER(DEBUG) << "\t OffsetOrSmallConstant:" << OffsetOrSmallConstant;
    }
};

struct LiveOutsTy {
    DwarfRegType DwarfRegNum;
    uint8_t Reserved;
    uint8_t SizeinBytes;
    void Print() const
    {
        LOG_COMPILER(DEBUG) << "                  Dwarf RegNum:" << DwarfRegNum;
        LOG_COMPILER(DEBUG) << "                  Reserved:" << Reserved;
        LOG_COMPILER(DEBUG) << "                  SizeinBytes:" << SizeinBytes;
    }
};
struct StkMapRecordTy {
    struct StkMapRecordHeadTy head;
    std::vector<struct LocationTy> Locations;
    std::vector<struct LiveOutsTy> LiveOuts;
    void Print() const
    {
        head.Print();
        auto size = Locations.size();
        for (size_t i = 0; i < size; i++) {
            LOG_COMPILER(DEBUG) << "                   #" << std::dec << i << ":";
            Locations[i].Print();
        }
        size = LiveOuts.size();
        for (size_t i = 0; i < size; i++) {
            LOG_COMPILER(DEBUG) << "               liveOuts[" << i << "] info:";
        }
    }
};

class DataInfo {
public:
    explicit DataInfo(std::unique_ptr<uint8_t[]> data): data_(std::move(data)), offset_(0) {}
    ~DataInfo()
    {
        data_.reset();
        offset_ = 0;
    }
    template<class T>
    T Read()
    {
        T t = *reinterpret_cast<const T*>(data_.get() + offset_);
        offset_ += sizeof(T);
        return t;
    }
    unsigned int GetOffset() const
    {
        return offset_;
    }
private:
    std::unique_ptr<uint8_t[]> data_ {nullptr};
    unsigned int offset_ {0};
};

struct LLVMStackMap {
    struct Header head;
    std::vector<struct StkSizeRecordTy> StkSizeRecords;
    std::vector<struct ConstantsTy> Constants;
    std::vector<struct StkMapRecordTy> StkMapRecord;
    void Print() const
    {
        head.Print();
        for (size_t i = 0; i < StkSizeRecords.size(); i++) {
            LOG_COMPILER(DEBUG) << "stkSizeRecord[" << i << "] info:";
            StkSizeRecords[i].Print();
        }
        for (size_t i = 0; i < Constants.size(); i++) {
            LOG_COMPILER(DEBUG) << "constants[" << i << "] info:";
            Constants[i].Print();
        }
        for (size_t i = 0; i < StkMapRecord.size(); i++) {
            LOG_COMPILER(DEBUG) << "StkMapRecord[" << i << "] info:";
            StkMapRecord[i].Print();
        }
    }
};
} // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_LLVM_STACKMAP_TYPE_H