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

#ifndef MAPLEBE_INCLUDE_CG_STACKMAP_H
#define MAPLEBE_INCLUDE_CG_STACKMAP_H
/* Maple IR header */
#include "operand.h"
#include "mempool.h"
#include "mempool_allocator.h"

namespace maplebe {
using namespace maple;
enum VregLocationKind {
    kUnknownDeoptKind,
    kOnStack,
    kInRegister,
    kInConstValue,
};

struct LocationInfo {
    VregLocationKind locationKind;
    int64 value;  // physical registerNO, stack frame offset, or immediate value
};

struct DeoptVregLocationInfo {
    int32 deoptVreg;
    LocationInfo locationInfo;

    DeoptVregLocationInfo(int32 vreg, LocationInfo info) : deoptVreg(vreg), locationInfo(info) {}
};

class DataEncoder {
public:
    void EncodeLEB128(int64 value, std::vector<uint8_t> &res) const
    {
        bool needDecodeNextByte = true;
        while (needDecodeNextByte) {
            uint8 byte = static_cast<uint8>(static_cast<uint64_t>(value) & LOW_7_BITS_MASK);
            value >>= DATA_BITS_SHIFT;
            if ((value == 0 && (byte & FLAG_MASK) == 0) ||
                (value == -1 && (byte & FLAG_MASK) != 0)) {
                needDecodeNextByte = false;
            } else {
                byte |= NEXT_BYTE_FLAG_MASK;
            }
            res.push_back(byte);
        }
    }
private:
    const uint8 FLAG_MASK = 0x40;
    const uint8 LOW_7_BITS_MASK = 0x7f;
    const uint8 NEXT_BYTE_FLAG_MASK = 0x80;
    const uint8 DATA_BITS_SHIFT = 7;
};

class DeoptInfo : public DataEncoder {
public:
    DeoptInfo(MapleAllocator &alloc) : deoptVreg2Opnd(alloc.Adapter()), deoptVreg2LocationInfo(alloc.Adapter()) {}
    ~DeoptInfo() = default;

    void AddDeoptBundleInfo(int32 deoptVreg, Operand &opnd)
    {
        deoptVreg2Opnd.insert(std::pair<int32, Operand *>(deoptVreg, &opnd));
    }

    void ReplaceDeoptBundleInfo(int32 deoptVreg, Operand &opnd)
    {
        deoptVreg2Opnd.insert_or_assign(deoptVreg, &opnd);
    }

    const MapleUnorderedMap<int32, Operand *> &GetDeoptBundleInfo() const
    {
        return deoptVreg2Opnd;
    }

    void RecordDeoptVreg2LocationInfo(int32 deoptVregNO, LocationInfo locationInfo)
    {
        deoptVreg2LocationInfo.emplace_back(deoptVregNO, locationInfo);
    }

    const MapleVector<DeoptVregLocationInfo> &GetDeoptVreg2LocationInfo() const
    {
        return deoptVreg2LocationInfo;
    }

    void Dump() const
    {
        for (const auto &elem : deoptVreg2LocationInfo) {
            LogInfo::MapleLogger() << "    deoptVreg: " << elem.deoptVreg;
            if (elem.locationInfo.locationKind == kInRegister) {
                LogInfo::MapleLogger() << "  in register: " << elem.locationInfo.value << "\n";
            } else if (elem.locationInfo.locationKind == kOnStack) {
                LogInfo::MapleLogger() << "  on stack: " << elem.locationInfo.value << "\n";
            } else {
                LogInfo::MapleLogger() << "  in const value: " << elem.locationInfo.value << "\n";
            }
        }
    }

    std::vector<uint8> SerializeInfo() const
    {
        std::vector<uint8> info;
        for (const auto &elem : deoptVreg2LocationInfo) {
            EncodeLEB128(static_cast<int64_t>(elem.deoptVreg), info);
            EncodeLEB128(static_cast<int64_t>(elem.locationInfo.locationKind), info);
            EncodeLEB128(static_cast<int64_t>(elem.locationInfo.value), info);
        }
        return info;
    }

private:
    // info before RA
    MapleUnorderedMap<int32, Operand *> deoptVreg2Opnd;
    MapleVector<DeoptVregLocationInfo> deoptVreg2LocationInfo;
};

class ReferenceMap : public DataEncoder {
public:
    ReferenceMap(MapleAllocator &alloc) : referenceLocations(alloc.Adapter()) {}
    ~ReferenceMap() = default;
    void ReocordRegisterRoots(uint32 regNO)
    {
        referenceLocations.push_back({kInRegister, regNO});
    }

    void ReocordStackRoots(int64 offset)
    {
        referenceLocations.push_back({kOnStack, offset});
    }

    const MapleVector<LocationInfo> &GetReferenceLocations() const
    {
        return referenceLocations;
    }

    void Dump() const
    {
        for (const auto &elem : referenceLocations) {
            if (elem.locationKind == kInRegister) {
                LogInfo::MapleLogger() << "    in register: " << elem.value << "\n";
            } else {
                LogInfo::MapleLogger() << "    on stack: " << elem.value << "\n";
            }
        }
    }

    std::vector<uint8> SerializeInfo() const
    {
        std::vector<uint8> info;
        for (const auto &elem : referenceLocations) {
            EncodeLEB128(static_cast<int64_t>(elem.locationKind), info);
            EncodeLEB128(static_cast<int64_t>(elem.value), info);
        }
        return info;
    }

private:
    MapleVector<LocationInfo> referenceLocations;
};

class StackMap {
public:
    StackMap(MapleAllocator &alloc) : deoptInfo(alloc), referenceMap(alloc) {}
    ~StackMap() = default;

    DeoptInfo &GetDeoptInfo()
    {
        return deoptInfo;
    }

    const DeoptInfo &GetDeoptInfo() const
    {
        return deoptInfo;
    }

    ReferenceMap &GetReferenceMap()
    {
        return referenceMap;
    }

    const ReferenceMap &GetReferenceMap() const
    {
        return referenceMap;
    }

private:
    DeoptInfo deoptInfo;
    ReferenceMap referenceMap;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_STACKMAP_H */
