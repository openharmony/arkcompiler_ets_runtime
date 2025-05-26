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
#include <cmath>
#include "ecmascript/base/bit_helper.h"
#include "ecmascript/common.h"
#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/stackmap/cg_stackmap.h"

#include "libpandabase/utils/leb128.h"

namespace panda::ecmascript::kungfu {
struct LocationTy {
    enum class Kind: uint8_t {
        REGISTER = 1,
        DIRECT = 2,
        INDIRECT = 3,
        CONSTANT = 4,
        CONSTANTNDEX = 5,
    };
    static constexpr int CONSTANT_FIRST_ELEMENT_INDEX = 3;
    // LLVM's stackmap = [cc, flags, numDeopt, <c0, l0>, .., gc-pair<base0, deriv0>, .. ] described in StackMaps.cpp
    static constexpr int CONSTANT_DEOPT_CNT_INDEX = 2;

    Kind location;
    uint8_t reserved_0;
    uint16_t locationSize;
    uint16_t dwarfRegNum;
    uint16_t reserved_1;
    int32_t offsetOrSmallConstant;

    std::string PUBLIC_API TypeToString(Kind loc) const;

    void Print() const
    {
        LOG_COMPILER(DEBUG)  << TypeToString(location);
        LOG_COMPILER(DEBUG) << ", size:" << std::dec << locationSize;
        LOG_COMPILER(DEBUG) << "\tDwarfRegNum:" << dwarfRegNum;
        LOG_COMPILER(DEBUG) << "\t OffsetOrSmallConstant:" << offsetOrSmallConstant;
    }
};

class LLVMStackMapType {
public:
    using OffsetType = int32_t;
    using DwarfRegType = uint16_t;
    using IntType = int32_t;
    using LargeInt = int64_t;
    using RegType = int8_t;
    using KindType = int8_t;
    using VRegId = int16_t;
    using SLeb128Type = int64_t;
    using DwarfRegAndOffsetType = std::pair<DwarfRegType, OffsetType>;
    using CallSiteInfo = std::vector<DwarfRegAndOffsetType>;
    using Fun2InfoType = std::pair<uintptr_t, DwarfRegAndOffsetType>;
    using Pc2CallSiteInfo = std::unordered_map<uintptr_t, CallSiteInfo>;
    using FpDelta = std::pair<int, uint32_t>;
    using Func2FpDelta = std::unordered_map<uintptr_t, FpDelta>; // value: fpDelta & funcSize
    using ConstInfo = std::vector<IntType>;
    using DeoptInfoType = std::vector<std::variant<IntType, LargeInt, DwarfRegAndOffsetType>>;
    using Pc2Deopt = std::unordered_map<uintptr_t, DeoptInfoType>;

    static constexpr size_t STACKMAP_ALIGN_BYTES = 2;
    static constexpr KindType CONSTANT_TYPE = 0;
    static constexpr KindType OFFSET_TYPE = 1;
    static constexpr uint16_t INVALID_DWARF_REG = 0;

    template <typename T>
    static void EncodeData(std::vector<uint8_t> &outData, size_t &dataSize, T value)
    {
        static_assert(std::is_signed_v<T>, "T must be signed");
        SLeb128Type data = value;
        size_t valueSize = panda::leb128::SignedEncodingSize(data);
        outData.resize(valueSize);
        dataSize = panda::leb128::EncodeSigned(data, outData.data());
    }

    static void EncodeRegAndOffset(std::vector<uint8_t> &regOffset, size_t &regOffsetSize,
        DwarfRegType reg, OffsetType offset, Triple triple, bool isBase = false);
    static bool DecodeRegAndOffset(SLeb128Type regOffset, DwarfRegType &reg, OffsetType &offset);
    static void EncodeVRegsInfo(std::vector<uint8_t> &vregsInfo, size_t &vregsInfoSize,
        VRegId id, LocationTy::Kind kind);
    static void DecodeVRegsInfo(SLeb128Type vregsInfo, VRegId &id, KindType &kind);

private:
    static constexpr size_t STACKMAP_TYPE_NUM = 2;
    static constexpr size_t STACKMAP_OFFSET_MUL = 8;
    static constexpr size_t STACKMAP_OFFSET_EXP = 3; // 2^3 = 8
    static constexpr RegType FP_VALUE = 0;
    static constexpr RegType SP_VALUE = 1;
    static constexpr KindType NO_DERIVED = 2;
};

struct Header {
    uint8_t  stackmapversion; // Stack Map Version (current version is 3)
    uint8_t  reserved0; // Reserved (expected to be 0)
    uint16_t reserved1; // Reserved (expected to be 0)
    void Print() const
    {
        LOG_COMPILER(DEBUG) << "----- head ----";
        LOG_COMPILER(DEBUG) << "   version:" << static_cast<int>(stackmapversion);
        LOG_COMPILER(DEBUG) << "+++++ head ++++";
    }
};

#pragma pack(1)
struct StkMapSizeRecordTy {
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
    uintptr_t largeConstant;
    void Print() const
    {
        LOG_COMPILER(DEBUG) << "               LargeConstant:0x" << std::hex << largeConstant;
    }
};

struct StkMapRecordHeadTy {
    uint64_t patchPointID;
    uint32_t instructionOffset;
    uint16_t reserved;
    uint16_t numLocations;
    void Print() const
    {
        LOG_COMPILER(DEBUG) << "               PatchPointID:0x" << std::hex << patchPointID;
        LOG_COMPILER(DEBUG) << "               instructionOffset:0x" << std::hex << instructionOffset;
        LOG_COMPILER(DEBUG) << "               Reserved:0x" << std::hex << reserved;
        LOG_COMPILER(DEBUG) << "               NumLocations:0x" << std::hex << numLocations;
    }
};
struct LiveOutsTy {
    LLVMStackMapType::DwarfRegType dwarfRegNum;
    uint8_t reserved;
    uint8_t sizeinBytes;
    void Print() const
    {
        LOG_COMPILER(DEBUG) << "                  Dwarf RegNum:" << dwarfRegNum;
        LOG_COMPILER(DEBUG) << "                  Reserved:" << reserved;
        LOG_COMPILER(DEBUG) << "                  SizeinBytes:" << sizeinBytes;
    }
};
struct StkMapRecordTy {
    struct StkMapRecordHeadTy head;
    std::vector<struct LocationTy> locations;
    std::vector<struct LiveOutsTy> liveOuts;
    void Print() const
    {
        head.Print();
        auto size = locations.size();
        for (size_t i = 0; i < size; i++) {
            LOG_COMPILER(DEBUG) << "                   #" << std::dec << i << ":";
            locations[i].Print();
        }
        size = liveOuts.size();
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
    std::vector<struct StkMapSizeRecordTy> stkSizeRecords;
    std::vector<struct ConstantsTy> constants;
    std::vector<struct StkMapRecordTy> stkMapRecord;
    void Print() const
    {
        head.Print();
        for (size_t i = 0; i < stkSizeRecords.size(); i++) {
            LOG_COMPILER(DEBUG) << "StkSizeRecord[" << i << "] info:";
            stkSizeRecords[i].Print();
        }
        for (size_t i = 0; i < constants.size(); i++) {
            LOG_COMPILER(DEBUG) << "Constants[" << i << "] info:";
            constants[i].Print();
        }
        for (size_t i = 0; i < stkMapRecord.size(); i++) {
            LOG_COMPILER(DEBUG) << "StkMapRecord[" << i << "] info:";
            stkMapRecord[i].Print();
        }
    }
};

class LLVMStackMapInfo : public CGStackMapInfo {
public:
    LLVMStackMapInfo() : CGStackMapInfo() {}
    ~LLVMStackMapInfo() = default;

    const std::vector<LLVMStackMapType::Pc2CallSiteInfo> &GetCallSiteInfoVec() const
    {
        return pc2CallSiteInfoVec_;
    }

    const std::vector<LLVMStackMapType::Pc2Deopt> &GetDeoptInfoVec() const
    {
        return pc2DeoptVec_;
    }

    void AppendCallSiteInfo(const LLVMStackMapType::Pc2CallSiteInfo &pc2CallSiteInfo)
    {
        pc2CallSiteInfoVec_.push_back(pc2CallSiteInfo);
    }

    void AppendDeoptInfo(const LLVMStackMapType::Pc2Deopt &pc2DeoptInfo)
    {
        pc2DeoptVec_.push_back(pc2DeoptInfo);
    }

    void PopCallSiteInfo()
    {
        pc2CallSiteInfoVec_.pop_back();
    }

    void PopDeoptInfo()
    {
        pc2DeoptVec_.pop_back();
    }

    CGStackMapKind GetStackMapKind() const override
    {
        return kLLVMStackMapInfo;
    }

private:
    std::vector<LLVMStackMapType::Pc2CallSiteInfo> pc2CallSiteInfoVec_;
    std::vector<LLVMStackMapType::Pc2Deopt> pc2DeoptVec_;
};
} // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_LLVM_STACKMAP_TYPE_H