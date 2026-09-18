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

#include "ecmascript/mem/embedded_code_ref_set.h"

#include <algorithm>
#include <limits>

#include "ecmascript/jit/jit_fort_write_scope.h"
#include "ecmascript/mem/machine_code.h"
#include "ecmascript/mem/region-inl.h"

namespace panda::ecmascript {

bool EmbeddedCodeRefSet::IsStrongHeapObject(JSTaggedType target)
{
    JSTaggedValue value(target);
    return value.IsHeapObject() && !value.IsWeakForHeapObject();
}

bool EmbeddedCodeRefSet::ValidateRecord(MachineCode *owner, uint32_t codeOffset, EmbeddedCodeRefRelocKind kind,
                                        uint8_t width, JSTaggedType target)
{
    if (owner == nullptr || width != sizeof(JSTaggedType) || !IsStrongHeapObject(target)) {
        return false;
    }
    if (codeOffset > owner->GetInstructionsSize() || width > owner->GetInstructionsSize() - codeOffset) {
        return false;
    }
#if defined(PANDA_TARGET_AMD64)
    return kind == EmbeddedCodeRefRelocKind::X64_MOVABS_IMM64;
#elif defined(PANDA_TARGET_ARM64)
    return kind == EmbeddedCodeRefRelocKind::ARM64_LITERAL64 && codeOffset % sizeof(JSTaggedType) == 0;
#else
    return false;
#endif
}

bool EmbeddedCodeRefSet::PatchRecord(MachineCode *owner, const EmbeddedCodeRefEntry &entry)
{
    ASSERT(ValidateRecord(owner, entry.codeOffset, entry.kind, entry.width, entry.target));
    uintptr_t patchAddress = owner->GetText() + entry.codeOffset;
    if (memcpy_s(reinterpret_cast<void *>(patchAddress), entry.width, &entry.target, sizeof(entry.target)) != EOK) {
        return false;
    }
    return true;
}

bool EmbeddedCodeRefSet::InstallOwner(MachineCode *owner, std::vector<EmbeddedCodeRefEntry> entries)
{
    if (entries.empty()) {
        return true;
    }
    if (entries.size() > std::numeric_limits<uint32_t>::max()) {
        return false;
    }

    for (const auto &entry : entries) {
        if (!ValidateRecord(owner, entry.codeOffset, entry.kind, entry.width, entry.target)) {
            return false;
        }
    }
    OwnerEntries ownerEntries;
    ownerEntries.entries = std::move(entries);
    auto localEnd =
        std::partition(ownerEntries.entries.begin(), ownerEntries.entries.end(),
                       [](const EmbeddedCodeRefEntry &entry) { return !JSTaggedValue(entry.target).IsInSharedHeap(); });
    ownerEntries.localEnd = static_cast<uint32_t>(std::distance(ownerEntries.entries.begin(), localEnd));
    auto youngEnd = std::partition(ownerEntries.entries.begin(), localEnd, [](const EmbeddedCodeRefEntry &entry) {
        JSTaggedValue target(entry.target);
        Region *targetRegion = Region::ObjectAddressToRange(target.GetRawHeapObject());
        return targetRegion->InYoungSpace();
    });
    ownerEntries.youngEnd = static_cast<uint32_t>(std::distance(ownerEntries.entries.begin(), youngEnd));

    if (refsByOwner_.find(owner) != refsByOwner_.end()) {
        return false;
    }
    JitFortWriteScope writeScope;
    if (!writeScope.Opened()) {
        return false;
    }
    for (const auto &entry : ownerEntries.entries) {
        if (!PatchRecord(owner, entry)) {
            return false;
        }
    }
    refsByOwner_.emplace(owner, std::move(ownerEntries));
    return true;
}

void EmbeddedCodeRefSet::RemoveOwner(MachineCode *owner)
{
    refsByOwner_.erase(owner);
}

void EmbeddedCodeRefSet::Clear()
{
    refsByOwner_.clear();
}

bool EmbeddedCodeRefSet::IsOwnerMarked(MachineCode *owner)
{
    Region *region = Region::ObjectAddressToRange(owner);
    ASSERT(!region->IsFreshRegion());
    return region->Test(owner);
}

EmbeddedCodeRefSet::EntryRange EmbeddedCodeRefSet::GetRange(const OwnerEntries &ownerEntries, TargetDomain domain)
{
    ASSERT(ownerEntries.youngEnd <= ownerEntries.localEnd);
    ASSERT(ownerEntries.localEnd <= ownerEntries.entries.size());
    switch (domain) {
        case TargetDomain::YOUNG:
            return {0, ownerEntries.youngEnd};
        case TargetDomain::LOCAL:
            return {0, ownerEntries.localEnd};
        case TargetDomain::SHARED:
            return {ownerEntries.localEnd, static_cast<uint32_t>(ownerEntries.entries.size())};
        default:
            UNREACHABLE();
    }
}

void EmbeddedCodeRefSet::VisitTargets(OwnerEntries &ownerEntries, EntryRange range, RootVisitor &visitor)
{
    ASSERT(range.begin <= range.end);
    ASSERT(range.end <= ownerEntries.entries.size());
    for (uint32_t index = range.begin; index < range.end; ++index) {
        EmbeddedCodeRefEntry &entry = ownerEntries.entries[index];
        visitor.VisitRoot(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&entry.target)));
    }
}

void EmbeddedCodeRefSet::VisitYoungTargets(RootVisitor &visitor)
{
    for (auto &[owner, ownerEntries] : refsByOwner_) {
        static_cast<void>(owner);
        VisitTargets(ownerEntries, GetRange(ownerEntries, TargetDomain::YOUNG), visitor);
    }
}

void EmbeddedCodeRefSet::VisitMarkedLocalTargets(RootVisitor &visitor)
{
    for (auto &[owner, ownerEntries] : refsByOwner_) {
        if (IsOwnerMarked(owner)) {
            VisitTargets(ownerEntries, GetRange(ownerEntries, TargetDomain::LOCAL), visitor);
        }
    }
}

void EmbeddedCodeRefSet::VisitSharedTargets(RootVisitor &visitor)
{
    for (auto &[owner, ownerEntries] : refsByOwner_) {
        static_cast<void>(owner);
        VisitTargets(ownerEntries, GetRange(ownerEntries, TargetDomain::SHARED), visitor);
    }
}

bool EmbeddedCodeRefSet::UpdateTargets(TargetDomain domain, bool markedOwnersOnly)
{
    bool hasTargetToPatch = false;
    for (const auto &[owner, ownerEntries] : refsByOwner_) {
        if (markedOwnersOnly && !IsOwnerMarked(owner)) {
            continue;
        }
        EntryRange range = GetRange(ownerEntries, domain);
        if (range.begin != range.end) {
            hasTargetToPatch = true;
            break;
        }
    }
    if (!hasTargetToPatch) {
        return true;
    }

    JitFortWriteScope writeScope;
    if (!writeScope.Opened()) {
        return false;
    }
    for (auto &[owner, ownerEntries] : refsByOwner_) {
        if (markedOwnersOnly && !IsOwnerMarked(owner)) {
            continue;
        }
        bool ownerPatched = false;
        EntryRange range = GetRange(ownerEntries, domain);
        for (uint32_t index = range.begin; index < range.end; ++index) {
            const EmbeddedCodeRefEntry &entry = ownerEntries.entries[index];
            ASSERT(IsStrongHeapObject(entry.target));
            ASSERT(JSTaggedValue(entry.target).IsInSharedHeap() == (domain == TargetDomain::SHARED));
            if (!PatchRecord(owner, entry)) {
                return false;
            }
            ownerPatched = true;
        }
        if (ownerPatched) {
            uintptr_t text = owner->GetText();
            __builtin___clear_cache(reinterpret_cast<char *>(text),
                                    reinterpret_cast<char *>(text + owner->GetInstructionsSize()));
        }
    }
    return true;
}

void EmbeddedCodeRefSet::CompactYoungEntries()
{
    for (auto &[owner, ownerEntries] : refsByOwner_) {
        static_cast<void>(owner);
        ASSERT(ownerEntries.youngEnd <= ownerEntries.localEnd);
        ASSERT(ownerEntries.localEnd <= ownerEntries.entries.size());
        auto oldYoungEnd = ownerEntries.entries.begin() + ownerEntries.youngEnd;
        auto newYoungEnd =
            std::partition(ownerEntries.entries.begin(), oldYoungEnd, [](const EmbeddedCodeRefEntry &entry) {
                ASSERT(IsStrongHeapObject(entry.target));
                JSTaggedValue target(entry.target);
                ASSERT(!target.IsInSharedHeap());
                Region *targetRegion = Region::ObjectAddressToRange(target.GetRawHeapObject());
                ASSERT(targetRegion->InYoungSpace() || targetRegion->InGeneralOldSpace());
                return targetRegion->InYoungSpace();
            });
        ownerEntries.youngEnd = static_cast<uint32_t>(std::distance(ownerEntries.entries.begin(), newYoungEnd));
    }
}

bool EmbeddedCodeRefSet::UpdateYoungTargets()
{
    if (!UpdateTargets(TargetDomain::YOUNG, false)) {
        return false;
    }
    CompactYoungEntries();
    return true;
}

bool EmbeddedCodeRefSet::UpdateMarkedLocalTargets()
{
    return UpdateTargets(TargetDomain::LOCAL, true);
}

bool EmbeddedCodeRefSet::UpdateSharedTargets()
{
    if (Jit::GetInstance()->IsEnableJitFort() && !Jit::GetInstance()->IsAppJit() &&
        !refsByOwner_.empty()) {
        static thread_local bool jitFortEnabled = false;
        if (!jitFortEnabled) {
            if (!JitFort::InitJitFort()) {
                return false;
            }
            jitFortEnabled = true;
        }
    }
    return UpdateTargets(TargetDomain::SHARED, false);
}

}  // namespace panda::ecmascript
