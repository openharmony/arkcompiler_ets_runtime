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

#ifndef MAPLE_IR_INCLUDE_MEM_REFERENCE_TABLE_H
#define MAPLE_IR_INCLUDE_MEM_REFERENCE_TABLE_H

#include "mempool_allocator.h"
#include "mir_module.h"
#include "orig_symbol.h"

namespace maple {

using MemDefUseSet = MapleUnorderedSet<OStIdx>;

class MemDefUse {
public:
    explicit MemDefUse(MapleAllocator &allocator) : defSet(allocator.Adapter()), useSet(allocator.Adapter()) {}
    ~MemDefUse();

    const MemDefUseSet &GetDefSet() const
    {
        return defSet;
    }

    MemDefUseSet &GetDefSet()
    {
        return defSet;
    }

    const MemDefUseSet &GetUseSet() const
    {
        return useSet;
    }

    MemDefUseSet &GetUseSet()
    {
        return useSet;
    }

    void SetIndependent()
    {
        isIndependent = true;
    }

    bool IsIndependent() const
    {
        return isIndependent;
    }

    void MergeOthers(MemDefUse &rhs)
    {
        if (this == &rhs) {
            return;
        }
        defSet.insert(rhs.defSet.begin(), rhs.defSet.end());
        useSet.insert(rhs.useSet.begin(), rhs.useSet.end());
        isIndependent = rhs.isIndependent || isIndependent;
    }

private:
    MemDefUseSet defSet;
    MemDefUseSet useSet;
    // The field is set TRUE to indicate that the callee-save and callee-reload instructions do not
    // conflict with any other memory access instructions.
    bool isIndependent = false;
};

using MemDefUsePart = MapleUnorderedMap<BaseNode *, MemDefUse *>;
using OstTable = MapleVector<OriginalSt>;
class MemReferenceTable {
public:
    MemReferenceTable(MapleAllocator &allocator, MIRFunction &func)
        : allocator(allocator), func(func), ostTable(allocator.Adapter()), memDefUsePart(allocator.Adapter())
    {
    }
    ~MemReferenceTable() {}

    MIRFunction &GetFunction()
    {
        return func;
    }

    MemDefUsePart &GetMemDefUsePart()
    {
        return memDefUsePart;
    }

    OstTable &GetOstTable()
    {
        return ostTable;
    }

    MemDefUse *GetOrCreateMemDefUseFromBaseNode(BaseNode *node)
    {
        auto iter = memDefUsePart.find(node);
        if (iter != memDefUsePart.end()) {
            return iter->second;
        }
        auto *newDefUse = allocator.New<MemDefUse>(allocator);
        memDefUsePart[node] = newDefUse;
        return newDefUse;
    }

private:
    MapleAllocator &allocator;
    MIRFunction &func;
    OstTable ostTable;
    MemDefUsePart memDefUsePart;
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_MEM_REFERENCE_TABLE_H
