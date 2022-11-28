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

#include "ecmascript/frames.h"
#include "ecmascript/stackmap/ark_stackmap.h"

namespace panda::ecmascript {
    class BinaryBufferParser;
}

namespace panda::ecmascript::kungfu {
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

class ArkStackMapBuilder {
public:
    ArkStackMapBuilder() = default;
    ~ArkStackMapBuilder() = default;
    std::pair<std::shared_ptr<uint8_t>, uint32_t> PUBLIC_API Run(std::unique_ptr<uint8_t []> stackMapAddr,
        uintptr_t hostCodeSectionAddr);
private:
    template <class Vec>
    void SortCallSite(std::vector<std::unordered_map<uintptr_t, Vec>> &infos,
        std::vector<std::pair<uintptr_t, Vec>>& result);

    void CalcCallsitePc(std::vector<std::pair<uintptr_t, DeoptInfoType>> &pc2Deopt,
        std::vector<std::pair<uintptr_t, CallSiteInfo>> &pc2StackMap, std::vector<intptr_t> &callsitePcs);

    void GenArkCallsiteAOTFileInfo(std::vector<Pc2CallSiteInfo> &pc2stackMaps,
        std::vector<Pc2Deopt>& pc2DeoptVec, ARKCallsiteAOTFileInfo &result);
    void SaveArkDeopt(const ARKCallsiteAOTFileInfo& info, BinaryBufferWriter& writer);
    void SaveArkStackMap(const ARKCallsiteAOTFileInfo& info, BinaryBufferWriter& writer);
    void SaveArkCallsiteAOTFileInfo(uint8_t *ptr, uint32_t length, const ARKCallsiteAOTFileInfo& info);
    int FindLoc(std::vector<intptr_t> &CallsitePcs, intptr_t pc);
    void GenARKDeopt(const DeoptInfoType& deopt, std::pair<uint32_t, std::vector<kungfu::ARKDeopt>> &sizeAndArkDeopt);
};
} // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_ARK_STACKMAP_BUILD_H
