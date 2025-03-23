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
    void GetConstInfo(uintptr_t callsite, LLVMStackMapType::ConstInfo& info, uint8_t *stackmapAddr = nullptr) const;
    void GetMethodOffsetInfo(uintptr_t callSiteAddr, std::map<uint32_t, uint32_t>& info,
                             uint8_t *stackmapAddr) const;
    bool IteratorStackMap(RootVisitor& visitor,
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
    void ParseArkStackMap(const CallsiteHeader& callsiteHead, uint8_t *ptr, ArkStackMap& stackMap) const;
#ifndef NDEBUG
    void ParseArkStackMapAndDeopt(uint8_t *ptr, uint32_t length) const;
#endif
    uintptr_t GetStackSlotAddress(uint8_t *stackmapAddr, uintptr_t callSiteSp, uintptr_t callsiteFp,
                                  uint32_t &offset) const;
    friend class ArkStackMapBuilder;
    bool enableLog_ {false};
};
}
#endif  // ECMASCRIPT_ARK_STACKMAP_PARSER_H