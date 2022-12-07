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
#include "ecmascript/stackmap/ark_stackmap_parser.h"

#include "ecmascript/aot_file_manager.h"
#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/stackmap/ark_stackmap_builder.h"

namespace panda::ecmascript::kungfu {
// implement simple binary-search is improve performance. if use std api, it'll trigger copy CallsiteHead.
int ArkStackMapParser::BinaraySearch(CallsiteHead *callsiteHead, uint32_t callsiteNum, uintptr_t callSiteAddr) const
{
    int slow = 0;
    int high = static_cast<int>(callsiteNum) - 1;
    int mid = 0;
    uint32_t v = 0;
    while (slow <= high) {
        mid = (slow + high) >> 1;
        v = callsiteHead[mid].calliteOffset;
        if (v == callSiteAddr) {
            return mid;
        } else if (v > callSiteAddr) {
            high = mid - 1;
        } else {
            slow = mid + 1;
        }
    }
    return -1;
}

void ArkStackMapParser::GetArkDeopt(uint8_t *stackmapAddr, uint32_t length,
    const CallsiteHead& callsiteHead, std::vector<kungfu::ARKDeopt> &deopts) const
{
    BinaryBufferParser binBufparser(stackmapAddr, length);
    ParseArkDeopt(callsiteHead, binBufparser, stackmapAddr, deopts);
}

void ArkStackMapParser::GetArkDeopt(uintptr_t callSiteAddr, uint8_t *stackmapAddr,
    std::vector<kungfu::ARKDeopt> &deopts) const
{
    StackMapSecHead *head = reinterpret_cast<StackMapSecHead *>(stackmapAddr);
    ASSERT(head != nullptr);
    uint32_t callsiteNum = head->callsiteNum;
    uint32_t length = head->totalSize;
    ASSERT((head->callsitEnd - head->callsitStart) == ((callsiteNum - 1) * sizeof(CallsiteHead)));

    CallsiteHead *callsiteHead = reinterpret_cast<CallsiteHead *>(stackmapAddr + sizeof(StackMapSecHead));
    int mid = BinaraySearch(callsiteHead, callsiteNum, callSiteAddr);
    if (mid == -1) {
        return;
    }
    CallsiteHead *found = callsiteHead + mid;
    GetArkDeopt(stackmapAddr, length, *found, deopts);
}

void ArkStackMapParser::GetConstInfo(uintptr_t callSiteAddr, ConstInfo &info, uint8_t *stackmapAddr) const
{
    std::vector<kungfu::ARKDeopt> deopts;
    GetArkDeopt(callSiteAddr, stackmapAddr, deopts);
    if (deopts.empty()) {
        return;
    }

    ARKDeopt target;
    OffsetType id = static_cast<OffsetType>(SpecVregIndex::BC_OFFSET_INDEX);
    target.Id = id;
    auto it = std::lower_bound(deopts.begin(), deopts.end(), target,
        [](const ARKDeopt& a, const ARKDeopt& b) {
            return a.Id < b.Id;
        });
    if (it == deopts.end() || (it->Id > id)) {
        return;
    }
    ASSERT(it->kind == LocationTy::Kind::CONSTANT);
    ASSERT(std::holds_alternative<OffsetType>(it->value));
    auto v = std::get<OffsetType>(it->value);
    info.emplace_back(v);
}

uintptr_t ArkStackMapParser::GetStackSlotAddress(const DwarfRegAndOffsetType info,
    uintptr_t callSiteSp, uintptr_t callsiteFp) const
{
    uintptr_t address = 0;
    if (info.first == GCStackMapRegisters::SP) {
        address = callSiteSp + info.second;
    } else if (info.first == GCStackMapRegisters::FP) {
        address = callsiteFp + info.second;
    } else {
        UNREACHABLE();
    }
    return address;
}

bool ArkStackMapParser::IteratorStackMap(const RootVisitor &visitor, const RootBaseAndDerivedVisitor &derivedVisitor,
    uintptr_t callSiteAddr, uintptr_t callsiteFp, uintptr_t callSiteSp, uint8_t *stackmapAddr) const
{
    StackMapSecHead *head = reinterpret_cast<StackMapSecHead *>(stackmapAddr);
    ASSERT(head != nullptr);
    uint32_t callsiteNum = head->callsiteNum;
    uint32_t length = head->totalSize;
    ArkStackMap arkStackMap;
    // BuiltinsStub current only sample stub, don't have stackmap&deopt.
    if (callsiteNum == 0) {
        return false;
    }
    ASSERT((head->callsitEnd - head->callsitStart) == ((callsiteNum - 1) * sizeof(CallsiteHead)));

    CallsiteHead *callsiteHead = reinterpret_cast<CallsiteHead *>(stackmapAddr + sizeof(StackMapSecHead));
    int mid = BinaraySearch(callsiteHead, callsiteNum, callSiteAddr);
    if (mid == -1) {
        return false;
    }
    CallsiteHead *found = callsiteHead + mid;
    BinaryBufferParser binBufparser(stackmapAddr, length);
    ParseArkStackMap(*found, binBufparser, stackmapAddr, arkStackMap);
    if (arkStackMap.size() == 0) {
        return false;
    }
    ASSERT(callsiteFp != callSiteSp);
    std::map<uintptr_t, uintptr_t> baseSet;
    for (size_t i = 0; i < arkStackMap.size(); i += 2) { // 2:base and derive
        const DwarfRegAndOffsetType baseInfo = arkStackMap.at(i);
        const DwarfRegAndOffsetType derivedInfo = arkStackMap.at(i + 1);
        uintptr_t base = GetStackSlotAddress(baseInfo, callSiteSp, callsiteFp);
        uintptr_t derived = GetStackSlotAddress(derivedInfo, callSiteSp, callsiteFp);
        if (*reinterpret_cast<uintptr_t *>(base) == 0) {
            base = derived;
        }
        if (*reinterpret_cast<uintptr_t *>(base) != 0) {
            // The base address may be marked repeatedly
            if (baseSet.find(base) == baseSet.end()) {
                baseSet.emplace(base, *reinterpret_cast<uintptr_t *>(base));
                visitor(Root::ROOT_FRAME, ObjectSlot(base));
            }

            if (base != derived) {
                derivedVisitor(Root::ROOT_FRAME, ObjectSlot(base), ObjectSlot(derived), baseSet[base]);
            }
        }
    }
    baseSet.clear();
    return true;
}
} // namespace panda::ecmascript::kungfu