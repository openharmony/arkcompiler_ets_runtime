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

#ifndef MAPLEBE_INCLUDE_BE_SWITCH_LOWERER_H
#define MAPLEBE_INCLUDE_BE_SWITCH_LOWERER_H
#include "mir_nodes.h"
#include "mir_module.h"

namespace maplebe {
class BELowerer;

class SwitchLowerer {
public:
    SwitchLowerer(maple::MIRModule &mod, maple::SwitchNode &stmt, maple::MapleAllocator &allocator)
        : mirModule(mod), stmt(&stmt), switchItems(allocator.Adapter()), ownAllocator(&allocator)
    {
    }

    ~SwitchLowerer() = default;

    maple::BlockNode *LowerSwitch();

private:
    using Cluster = std::pair<maple::int32, maple::int32>;
    using SwitchItem = std::pair<maple::int32, maple::int32>;

    maple::MIRModule &mirModule;
    maple::SwitchNode *stmt;
    /*
     * the original switch table is sorted and then each dense (in terms of the
     * case tags) region is condensed into 1 switch item; in the switchItems
     * table, each item either corresponds to an original entry in the original
     * switch table (pair's second is 0), or to a dense region (pair's second
     * gives the upper limit of the dense range)
     */
    maple::MapleVector<SwitchItem> switchItems; /* uint32 is index in switchTable */
    maple::MapleAllocator *ownAllocator;
    const maple::int32 kClusterSwitchCutoff = 5;
    const float kClusterSwitchDensityHigh = 0.4;
    const float kClusterSwitchDensityLow = 0.2;
    const maple::int32 kMaxRangeGotoTableSize = 127;
    bool jumpToDefaultBlockGenerated = false;

    void FindClusters(maple::MapleVector<Cluster> &clusters) const;
    void InitSwitchItems(maple::MapleVector<Cluster> &clusters);
    maple::RangeGotoNode *BuildRangeGotoNode(maple::int32 startIdx, maple::int32 endIdx);
    maple::CompareNode *BuildCmpNode(maple::Opcode opCode, maple::uint32 idx);
    maple::GotoNode *BuildGotoNode(maple::int32 idx);
    maple::CondGotoNode *BuildCondGotoNode(maple::int32 idx, maple::Opcode opCode, maple::BaseNode &cond);
    maple::BlockNode *BuildCodeForSwitchItems(maple::int32 start, maple::int32 end, bool lowBNdChecked,
                                              bool highBNdChecked);
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_BE_SWITCH_LOWERER_H */
