/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_MEM_EMBEDDED_CODE_REF_SET_H
#define ECMASCRIPT_MEM_EMBEDDED_CODE_REF_SET_H

#include <unordered_map>
#include <vector>

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/embedded_code_ref.h"
#include "ecmascript/mem/visitor.h"

namespace panda::ecmascript {

class MachineCode;

struct EmbeddedCodeRefEntry {
    uint32_t codeOffset {0};
    EmbeddedCodeRefRelocKind kind {EmbeddedCodeRefRelocKind::X64_MOVABS_IMM64};
    uint8_t width {0};
    JSTaggedType target {JSTaggedValue::VALUE_UNDEFINED};
};
static_assert(sizeof(EmbeddedCodeRefEntry) == sizeof(uint64_t) * 2U);

class EmbeddedCodeRefSet {
public:
    EmbeddedCodeRefSet() = default;
    ~EmbeddedCodeRefSet() = default;

    NO_COPY_SEMANTIC(EmbeddedCodeRefSet);
    NO_MOVE_SEMANTIC(EmbeddedCodeRefSet);

    bool InstallOwner(MachineCode *owner, std::vector<EmbeddedCodeRefEntry> entries);
    void RemoveOwner(MachineCode *owner);
    void Clear();

    void VisitYoungTargets(RootVisitor &visitor);
    void VisitMarkedLocalTargets(RootVisitor &visitor);
    void VisitSharedTargets(RootVisitor &visitor);

    bool UpdateYoungTargets();
    bool UpdateMarkedLocalTargets();
    bool UpdateSharedTargets();

private:
    // Entries are partitioned as [young local][old local][shared].
    struct OwnerEntries {
        std::vector<EmbeddedCodeRefEntry> entries;
        uint32_t youngEnd {0};
        uint32_t localEnd {0};
    };

    struct EntryRange {
        uint32_t begin {0};
        uint32_t end {0};
    };

    enum class TargetDomain : uint8_t {
        YOUNG,
        LOCAL,
        SHARED,
    };

    static bool IsOwnerMarked(MachineCode *owner);
    static bool IsStrongHeapObject(JSTaggedType target);
    static bool ValidateRecord(MachineCode *owner, uint32_t codeOffset, EmbeddedCodeRefRelocKind kind, uint8_t width,
                               JSTaggedType target);
    static bool PatchRecord(MachineCode *owner, const EmbeddedCodeRefEntry &entry);
    static EntryRange GetRange(const OwnerEntries &ownerEntries, TargetDomain domain);
    static void VisitTargets(OwnerEntries &ownerEntries, EntryRange range, RootVisitor &visitor);
    bool UpdateTargets(TargetDomain domain, bool markedOwnersOnly);
    void CompactYoungEntries();

    // Access is serialized by the host thread's JIT/GC lock for installation
    // and local GC, or by SuspendAll for shared GC. Owner removal only occurs
    // during GC sweep preparation or heap teardown.
    std::unordered_map<MachineCode *, OwnerEntries> refsByOwner_;
};

}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_EMBEDDED_CODE_REF_SET_H
