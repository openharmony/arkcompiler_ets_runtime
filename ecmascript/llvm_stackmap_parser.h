/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_LLVM_STACKMAP_PARSER_H
#define ECMASCRIPT_LLVM_STACKMAP_PARSER_H

#include <iostream>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "ecmascript/ark_stackmap_type.h"
#include "ecmascript/common.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/llvm_stackmap_type.h"

namespace panda::ecmascript {
    class BinaryBufferParser;
}
namespace panda::ecmascript::kungfu {
class BinaryBufferWriter;
class LLVMStackMapParser {
public:
    bool PUBLIC_API CalculateStackMap(std::unique_ptr<uint8_t []> stackMapAddr);
    std::pair<uint8_t *, uint32_t> PUBLIC_API CalculateStackMap(std::unique_ptr<uint8_t []> stackMapAddr,
    uintptr_t hostCodeSectionAddr);
    void PUBLIC_API Print() const
    {
        if (IsLogEnabled()) {
            llvmStackMap_.Print();
        }
    }
    const CallSiteInfo *GetCallSiteInfoByPc(uintptr_t funcAddr) const;
    bool CollectGCSlots(uintptr_t callSiteAddr, uintptr_t callsiteFp,
                            std::set<uintptr_t> &baseSet, ChunkMap<DerivedDataKey, uintptr_t> *data,
                            [[maybe_unused]] bool isVerifying,
                            uintptr_t callSiteSp) const;
    bool CollectGCSlots(uintptr_t callSiteAddr, uintptr_t callsiteFp,
                        std::set<uintptr_t> &baseSet, ChunkMap<DerivedDataKey, uintptr_t> *data,
                        [[maybe_unused]] bool isVerifying,
                        uintptr_t callSiteSp, uint8_t *stackmapAddr) const;
    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    void PUBLIC_API CalculateFuncFpDelta(Func2FpDelta info, uint32_t moduleIndex);
    ConstInfo GetConstInfo(uintptr_t callsite, uint8_t *stackmapAddr = nullptr) const;

    explicit LLVMStackMapParser(bool enableLog = false)
    {
        pc2CallSiteInfoVec_.clear();
        fun2RecordNum_.clear();
        dataInfo_ = nullptr;
        enableLog_ = enableLog;
        funAddr_.clear();
        fun2FpDelta_.clear();
        pc2DeoptVec_.clear();
    }
    ~LLVMStackMapParser()
    {
        pc2CallSiteInfoVec_.clear();
        fun2RecordNum_.clear();
        dataInfo_ = nullptr;
        funAddr_.clear();
        fun2FpDelta_.clear();
        pc2DeoptVec_.clear();
    }
private:
    void CalcCallSite();
    ARKCallsitePackInfo GenArkCallsitePackInfo();
    std::vector<intptr_t> CalcCallsitePc(std::vector<std::pair<uintptr_t, DeoptInfoType>> &pc2Deopt,
        std::vector<std::pair<uintptr_t, CallSiteInfo>> &pc2StackMap);
    int FindLoc(std::vector<intptr_t> &CallsitePcs, intptr_t pc);
    std::pair<int, std::vector<ARKDeopt>> GenARKDeopt(const DeoptInfoType& deopt);
    void SaveArkCallsitePackInfo(uint8_t *ptr, uint32_t length, const ARKCallsitePackInfo& info);
    std::vector<ARKDeopt> ParseArkDeopt(const CallsiteHead& callsiteHead, BinaryBufferParser& binBufparser,
        uint8_t *ptr) const;
    ArkStackMap ParseArkStackMap(const CallsiteHead& callsiteHead, BinaryBufferParser& binBufparser,
        uint8_t *ptr) const;
    void ParseArkStackMapAndDeopt(uint8_t *ptr, uint32_t length) const;

    void PrintCallSiteInfo(const CallSiteInfo *infos, OptimizedLeaveFrame *frame) const;
    void PrintCallSiteInfo(const CallSiteInfo *infos, uintptr_t callSiteFp, uintptr_t callSiteSp) const;
    int FindFpDelta(uintptr_t funcAddr, uintptr_t callsitePc, uint32_t moduleIndex) const;
    inline uintptr_t GetStackSlotAddress(const DwarfRegAndOffsetType info,
        uintptr_t callSiteSp, uintptr_t callsiteFp) const;
    void CollectBaseAndDerivedPointers(const CallSiteInfo *infos, std::set<uintptr_t> &baseSet,
        ChunkMap<DerivedDataKey, uintptr_t> *data, [[maybe_unused]] bool isVerifying,
        uintptr_t callsiteFp, uintptr_t callSiteSp) const;
    void PrintCallSiteSlotAddr(const CallSiteInfo& callsiteInfo, uintptr_t callSiteSp,
        uintptr_t callsiteFp) const;
    int BinaraySearch(CallsiteHead *callsiteHead, uint32_t callsiteNum, uintptr_t callSiteAddr) const;
    std::vector<ARKDeopt> GetArkDeopt(uint8_t *stackmapAddr, uint32_t length, const CallsiteHead& callsiteHead) const;
    template <class Vec>
    std::vector<std::pair<uintptr_t, Vec>> SortCallSite(std::vector<std::unordered_map<uintptr_t, Vec>> &infos);
    void SaveArkStackMap(const ARKCallsitePackInfo& info, BinaryBufferWriter& writer);
    void SaveArkDeopt(const ARKCallsitePackInfo& info, BinaryBufferWriter& writer);

    struct LLVMStackMap llvmStackMap_;
    std::vector<Pc2CallSiteInfo> pc2CallSiteInfoVec_;
    std::vector<std::pair<uintptr_t, uint64_t>> fun2RecordNum_;
    [[maybe_unused]] std::unique_ptr<DataInfo> dataInfo_;
    bool enableLog_ {false};
    std::set<uintptr_t> funAddr_;
    std::vector<Func2FpDelta> fun2FpDelta_;
    std::vector<Pc2Deopt> pc2DeoptVec_;
    std::map<uint32_t, std::vector<Func2FpDelta>> module2fun2FpDelta_;
    std::map<uint32_t, std::set<uintptr_t>> module2funAddr_;
};

class BinaryBufferWriter {
public:
    BinaryBufferWriter(uint8_t *buffer, uint32_t length) : buffer_(buffer), length_(length) {}
    ~BinaryBufferWriter() = default;
    void WriteBuffer(const uint8_t *src, uint32_t count, bool flag = false);
    uint32_t GetOffset() const
    {
        return offset_;
    }
private:
    uint8_t *buffer_ {nullptr};
    uint32_t length_ {0};
    uint32_t offset_ {0};
};
} // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_LLVM_STACKMAP_PARSER_H