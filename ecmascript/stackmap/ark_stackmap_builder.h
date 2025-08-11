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
#ifndef ECMASCRIPT_ARK_STACKMAP_BUILD_H
#define ECMASCRIPT_ARK_STACKMAP_BUILD_H

#include <iostream>
#include <memory>
#include <set>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <iomanip>

#include "ecmascript/frames.h"
#include "ecmascript/stackmap/ark_stackmap.h"
#include "ecmascript/stackmap/llvm/llvm_stackmap_type.h"

#include "libpandabase/utils/leb128.h"
namespace panda::ecmascript {
    class BinaryBufferParser;
}

namespace panda::ecmascript::kungfu {
class BinaryBufferWriter {
public:
    BinaryBufferWriter(uint8_t *buffer, uint32_t length) : buffer_(buffer), length_(length) {}
    ~BinaryBufferWriter() = default;
    void WriteBuffer(const uint8_t *src, uint32_t count);
    uint32_t GetOffset() const
    {
        return offset_;
    }
    void AlignOffset()
    {
        offset_ = AlignUp(offset_, LLVMStackMapType::STACKMAP_ALIGN_BYTES);
    }
private:
    uint8_t *buffer_ {nullptr};
    uint32_t length_ {0};
    uint32_t offset_ {0};
};

struct StackMapDumper {
    uint32_t callsiteHeadSize {0};
    uint32_t arkStackMapSize {0};
    uint32_t deoptSize {0};
    uint32_t callsiteNum {0};
    uint32_t stackmapNum {0};
    uint32_t deoptNum {0};
};

class ArkStackMapBuilder {
public:
    ArkStackMapBuilder() = default;
    ~ArkStackMapBuilder() = default;
    std::pair<std::shared_ptr<uint8_t>, uint32_t> PUBLIC_API Run(std::unique_ptr<uint8_t []> stackMapAddr,
        uintptr_t hostCodeSectionAddr, Triple triple);
    std::pair<std::shared_ptr<uint8_t>, uint32_t> PUBLIC_API GenerateArkStackMap(
        CGStackMapInfo &stackMapInfo, Triple triple);
    void PUBLIC_API Collect(std::unique_ptr<uint8_t []> stackMapAddr,
                            uintptr_t hostCodeSectionAddr,
                            uintptr_t hostCodeSectionOffset,
                            CGStackMapInfo &stackMapInfo);
    void SetTraceStackMap(bool flag)
    {
        traceStackMap_ = flag;
    }

private:
    static constexpr int DECIMAL_LENS = 2;
    static constexpr size_t DEOPT_ENTRY_SIZE = 2;
    static constexpr size_t GC_ENTRY_SIZE = 2;
    static constexpr size_t STACKMAP_TYPE_NUM = 2;

    template <class Vec>
    void SortCallSite(const std::vector<std::unordered_map<uintptr_t, Vec>> &infos,
        std::vector<std::pair<uintptr_t, Vec>>& result);
    void CalcCallsitePc(std::vector<std::pair<uintptr_t, LLVMStackMapType::DeoptInfoType>> &pc2Deopt,
                        std::vector<std::pair<uintptr_t, LLVMStackMapType::CallSiteInfo>> &pc2StackMap,
                        std::vector<intptr_t> &callsitePcs);
    void GenArkCallsiteAOTFileInfo(const CGStackMapInfo &stackMapInfo,
                                   ARKCallsiteAOTFileInfo &result, Triple triple);
    void SaveArkDeopt(const ARKCallsiteAOTFileInfo& info, BinaryBufferWriter& writer, Triple triple);
    void SaveArkStackMap(const ARKCallsiteAOTFileInfo& info, BinaryBufferWriter& writer, Triple triple);
    void SaveArkCallsiteAOTFileInfo(uint8_t *ptr, uint32_t length,
                                    const ARKCallsiteAOTFileInfo& info, Triple triple);
    int FindLoc(std::vector<intptr_t> &CallsitePcs, intptr_t pc);
    void GenARKDeopt(const LLVMStackMapType::DeoptInfoType& deopt,
        std::pair<uint32_t, std::vector<ARKDeopt>> &sizeAndArkDeopt, Triple triple);
    void Dump(const StackMapDumper& dumpInfo) const;

    bool traceStackMap_{false};
    StackMapDumper dumper_;
};
} // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_ARK_STACKMAP_BUILD_H
