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
#include "ecmascript/stackmap/llvm/llvm_stackmap_type.h"

namespace panda::ecmascript::kungfu {
void LLVMStackMapType::EncodeRegAndOffset(std::vector<uint8_t> &regOffset, size_t &regOffsetSize,
    DwarfRegType reg, OffsetType offset, Triple triple)
{
    SLeb128Type dwarfRegAndOff = offset;
    auto fpReg = GCStackMapRegisters::GetFpRegByTriple(triple);
    auto spReg = GCStackMapRegisters::GetSpRegByTriple(triple);
    if (reg == fpReg) {
        dwarfRegAndOff = (dwarfRegAndOff << 1) + FP_VALUE;
    } else if (reg == spReg) {
        dwarfRegAndOff = (dwarfRegAndOff << 1) + SP_VALUE;
    } else {
        LOG_ECMA(FATAL) << "dwarfreg branch is unreachable";
        UNREACHABLE();
    }
    size_t valueSize = panda::leb128::SignedEncodingSize(dwarfRegAndOff);
    regOffset.resize(valueSize);
    regOffsetSize = panda::leb128::EncodeSigned(dwarfRegAndOff, regOffset.data());
}

void LLVMStackMapType::DecodeRegAndOffset(SLeb128Type regOffset, DwarfRegType &reg, OffsetType &offset)
{
    if (regOffset % STACKMAP_TYPE_NUM == LLVMStackMapType::FP_VALUE) {
        reg = GCStackMapRegisters::FP;
    } else {
        reg = GCStackMapRegisters::SP;
    }
    offset = static_cast<LLVMStackMapType::OffsetType>(regOffset >> 1);
}

void LLVMStackMapType::EncodeVRegsInfo(std::vector<uint8_t> &vregsInfo, size_t &vregsInfoSize,
    VRegId id, LocationTy::Kind kind)
{
    SLeb128Type vregIdAndKind = id;
    if (kind == LocationTy::Kind::CONSTANT || kind == LocationTy::Kind::CONSTANTNDEX) {
        vregIdAndKind = (vregIdAndKind << 1) + CONSTANT_TYPE;
    } else if (kind == LocationTy::Kind::INDIRECT) {
        vregIdAndKind = (vregIdAndKind << 1) + OFFSET_TYPE;
    } else {
        LOG_ECMA(FATAL) << "vreg kind branch is unreachable";
        UNREACHABLE();
    }
    size_t vregIdAndKindSize = panda::leb128::SignedEncodingSize(vregIdAndKind);
    vregsInfo.resize(vregIdAndKindSize);
    vregsInfoSize = panda::leb128::EncodeSigned(vregIdAndKind, vregsInfo.data());
}

void LLVMStackMapType::DecodeVRegsInfo(SLeb128Type vregsInfo, VRegId &id, KindType &kind)
{
    if (vregsInfo % STACKMAP_TYPE_NUM == CONSTANT_TYPE) {
        kind = CONSTANT_TYPE;
    } else {
        kind = OFFSET_TYPE;
    }
    id = static_cast<VRegId>(vregsInfo >> 1);
}
}  // namespace panda::ecmascript::kungfu