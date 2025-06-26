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
#ifndef ECMASCRIPT_ARK_STACKMAP_PARSER_H
#define ECMASCRIPT_ARK_STACKMAP_PARSER_H

#include <cstdint>

#include "ecmascript/frames.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/stackmap/ark_stackmap.h"
#include "ecmascript/stackmap/llvm/llvm_stackmap_type.h"

namespace panda::ecmascript {
    class BinaryBufferParser;
}

namespace panda::ecmascript::kungfu {
class ArkStackMapParser {
public:
    explicit ArkStackMapParser(bool enableLog = false)
    {
        enableLog_ = enableLog;
    }
    ~ArkStackMapParser()
    {
        enableLog_ = false;
    }
    size_t GetInlineDepth(const std::vector<ARKDeopt> &deopts) const;
    size_t GetInlineDepth(uintptr_t callSiteAddr, uint8_t *stackmapAddr) const;
    void CollectStackTraceInfos(uintptr_t callSiteAddr,
                                std::vector<std::pair<JSTaggedType, uint32_t>> &info,
                                uintptr_t callsiteSp,
                                uintptr_t callsiteFp,
                                uint8_t *stackmapAddr) const;
    JSTaggedType GetFunction(const std::vector<ARKDeopt> &deopts, size_t currentDepth, size_t shift,
                             uintptr_t callsiteSp, uintptr_t callsiteFp) const;
    int32_t GetPcOffset(const std::vector<ARKDeopt> &deopts, size_t currentDepth, size_t shift) const;
    void IteratorStackMap(RootVisitor& visitor, uintptr_t callsiteFp,
                          uintptr_t callSiteSp, uint8_t *stackmapAddr,
                          uint32_t offset, uint16_t stackmapNum,
                          std::map<uintptr_t, uintptr_t> &baseSet) const;
    void IteratorDeopt(RootVisitor& visitor, uintptr_t callsiteFp,
                       uintptr_t callSiteSp, uint8_t *stackmapAddr,
                       uint32_t offset, uint16_t num,
                       std::map<uintptr_t, uintptr_t> &baseSet) const;
    bool IteratorStackMapAndDeopt(RootVisitor& visitor,
                                  uintptr_t callSiteAddr, uintptr_t callsiteFp,
                                  uintptr_t callSiteSp, uint8_t *stackmapAddr) const;
    void GetArkDeopt(uintptr_t callSiteAddr, uint8_t *stackmapAddr, std::vector<ARKDeopt>& deopts) const;

private:
    static constexpr size_t DEOPT_ENTRY_SIZE = 2;
    static constexpr size_t GC_ENTRY_SIZE = 2;

    int BinaraySearch(CallsiteHeader *callsiteHead, uint32_t callsiteNum, uintptr_t callSiteAddr) const;
    void GetArkDeopt(uint8_t *stackmapAddr, const CallsiteHeader& callsiteHead,
                     std::vector<ARKDeopt>& deopt) const;
    void ParseArkDeopt(const CallsiteHeader& callsiteHead, uint8_t *ptr, std::vector<ARKDeopt>& deopts) const;
#ifndef NDEBUG
    void ParseArkStackMap(const CallsiteHeader& callsiteHead, uint8_t *ptr, ArkStackMap& stackMap) const;
    void ParseArkStackMapAndDeopt(uint8_t *ptr, uint32_t length) const;
#endif
    uintptr_t GetStackSlotAddress(const LLVMStackMapType::DwarfRegAndOffsetType info,
                                  uintptr_t callSiteSp,
                                  uintptr_t callsiteFp) const;
    uintptr_t GetStackSlotAddress(uint8_t *stackmapAddr, uintptr_t callSiteSp, uintptr_t callsiteFp,
                                  uint32_t &offset, bool &nextIsBase, size_t &regOffsetSize) const;
    uintptr_t GetDeoptStackSlotAddress(uint8_t *stackmapAddr, uintptr_t callSiteSp,
                                       uintptr_t callsiteFp, uint32_t &offset) const;
    friend class ArkStackMapBuilder;
    bool enableLog_ {false};
};
}
#endif  // ECMASCRIPT_ARK_STACKMAP_PARSER_H