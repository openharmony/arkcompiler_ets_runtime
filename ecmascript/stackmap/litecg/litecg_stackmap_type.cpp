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
#include "ecmascript/stackmap/litecg/litecg_stackmap_type.h"

namespace panda::ecmascript::kungfu {
static int64_t DecodeSLEB128(const std::vector<uint8_t> &bytes, size_t &index)
{
    uint64_t res = 0;
    uint64_t shift = 0;
    constexpr uint8_t FLAG_MASK = 0x40;
    constexpr uint8_t LOW_7_BITS_MASK = 0x7f;
    constexpr uint8_t NEXT_BYTE_FLAG_MASK = 0x80;
    constexpr uint8_t DATA_BITS_SHIFT = 7;
    constexpr uint64_t BIT_SIZE_64 = 64;
    bool needDecodeNextByte = false;
    uint8_t byte = 0;
    do {
        byte = bytes[index];
        needDecodeNextByte = ((byte & NEXT_BYTE_FLAG_MASK) != 0);
        uint8_t low7Bit = (byte & LOW_7_BITS_MASK);
        res |= (static_cast<uint64_t>(low7Bit) << shift);
        shift += DATA_BITS_SHIFT;
        index++;
    } while (needDecodeNextByte);

    if (shift < BIT_SIZE_64 && (byte & FLAG_MASK) != 0) {
        res |= UINT64_MAX << shift;
    }
    return static_cast<int64_t>(res);
}

void LiteCGStackMapInfo::ConvertToLLVMPc2CallSiteInfo(LLVMPc2CallSiteInfo &pc2CallSiteInfo,
                                                      const LiteCGPc2CallSiteInfo &liteCGPc2CallSiteInfo,
                                                      int fpReg) const
{
    for (const auto &elem : liteCGPc2CallSiteInfo) {
        const auto &litecgCallSiteInfo = elem.second;
        LLVMCallSiteInfo llvmCallSiteInfo;
        // parse std::vector<uint8_t>
        size_t index = 0;
        while (index < litecgCallSiteInfo.size()) {
            int64_t kind = DecodeSLEB128(litecgCallSiteInfo, index);
            int64_t value = DecodeSLEB128(litecgCallSiteInfo, index);
            if (kind == 2) {  // kind is 2 means register
                llvmCallSiteInfo.push_back(LLVMDwarfRegAndOffsetType(0xFFFFU, static_cast<LLVMOffsetType>(value)));
            } else if (kind == 1) { // stack
                llvmCallSiteInfo.push_back(LLVMDwarfRegAndOffsetType(fpReg, static_cast<LLVMOffsetType>(value)));
            } else {
                LOG_ECMA(FATAL) << "only stack and reigster is supported currently";
                UNREACHABLE();
            }
        }
        uintptr_t pc = static_cast<uintptr_t>(elem.first);
        pc2CallSiteInfo[pc] = llvmCallSiteInfo;
    }
}

void LiteCGStackMapInfo::ConvertToLLVMPv2Deopt(LLVMPc2Deopt &pc2DeoptInfo, const LiteCGPc2Deopt &liteCGPc2DeoptInfo,
                                               int fpReg) const
{
    for (const auto &elem : liteCGPc2DeoptInfo) {
        const auto &litecgDeoptInfo = elem.second;
        LLVMDeoptInfoType llvmDeoptInfo;
        // parse std::vector<uint8_t>
        size_t index = 0;
        while (index < litecgDeoptInfo.size()) {
            int64_t deoptVreg = DecodeSLEB128(litecgDeoptInfo, index);
            int64_t kind = DecodeSLEB128(litecgDeoptInfo, index);
            int64_t value = DecodeSLEB128(litecgDeoptInfo, index);
            llvmDeoptInfo.push_back(static_cast<LLVMIntType>(deoptVreg));
            if (kind == 2) { // kind is 2 means register
                llvmDeoptInfo.push_back(LLVMDwarfRegAndOffsetType(0xFFFFU, static_cast<LLVMOffsetType>(value)));
            } else if (kind == 1) { // stack
                llvmDeoptInfo.push_back(LLVMDwarfRegAndOffsetType(fpReg, static_cast<LLVMOffsetType>(value)));
            } else if (value > INT32_MAX || value < INT32_MIN) {  // large imm
                llvmDeoptInfo.push_back(static_cast<LLVMLargeInt>(value));
            } else {  // imm
                llvmDeoptInfo.push_back(static_cast<LLVMIntType>(value));
            }
        }
        pc2DeoptInfo[elem.first] = llvmDeoptInfo;
    }
}

void LiteCGStackMapInfo::ConvertToLLVMStackMapInfo(std::vector<LLVMPc2CallSiteInfo> &pc2StackMapsVec,
                                                   std::vector<LLVMPc2Deopt> &pc2DeoptInfoVec, Triple triple) const
{
    auto fpReg = GCStackMapRegisters::GetFpRegByTriple(triple);
    for (const auto &callSiteInfo : pc2CallSiteInfoVec_) {
        LLVMPc2CallSiteInfo pc2CallSiteInfo;
        ConvertToLLVMPc2CallSiteInfo(pc2CallSiteInfo, callSiteInfo, fpReg);
        pc2StackMapsVec.push_back(pc2CallSiteInfo);
    }
    for (const auto &deoptInfo : pc2DeoptVec_) {
        LLVMPc2Deopt pc2DeoptInfo;
        ConvertToLLVMPv2Deopt(pc2DeoptInfo, deoptInfo, fpReg);
        pc2DeoptInfoVec.push_back(pc2DeoptInfo);
    }
}
}  // namespace panda::ecmascript::kungfu