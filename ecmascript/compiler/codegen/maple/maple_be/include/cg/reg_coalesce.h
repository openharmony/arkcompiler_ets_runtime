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

#ifndef MAPLEBE_INCLUDE_CG_REGCOALESCE_H
#define MAPLEBE_INCLUDE_CG_REGCOALESCE_H
#include "live.h"

namespace maplebe {

using posPair = std::pair<uint32, uint32>;
class LiveInterval {
public:
    explicit LiveInterval(MapleAllocator &allocator)
        : ranges(allocator.Adapter()),
          conflict(allocator.Adapter()),
          defPoints(allocator.Adapter()),
          usePoints(allocator.Adapter()),
          alloc(allocator)
    {
    }

    void IncNumCall()
    {
        ++numCall;
    }

    MapleMap<uint32, MapleVector<posPair>> GetRanges()
    {
        return ranges;
    }

    void AddRange(uint32 bbid, uint32 end, bool alreadLive)
    {
        auto it = ranges.find(bbid);
        if (it == ranges.end()) {
            MapleVector<posPair> posVec(alloc.Adapter());
            posVec.emplace_back(std::pair(end, end));
            ranges.emplace(bbid, posVec);
        } else {
            MapleVector<posPair> &posVec = it->second;
            if (alreadLive) {
                CHECK_FATAL(posVec.size() > 0, "must not be zero");
                posPair lastPos = posVec[posVec.size() - 1];
                posVec[posVec.size() - 1] = std::pair(end, lastPos.second);
            } else {
                posVec.emplace_back(std::pair(end, end));
            }
        }
    }

    void MergeRanges(LiveInterval &lr)
    {
        auto lrDestRanges = lr.GetRanges();
        for (auto destRange : lrDestRanges) {
            uint32 bbid = destRange.first;
            auto &destPosVec = destRange.second;
            auto it = ranges.find(bbid);
            if (it == ranges.end()) {
                /* directly add it */
                MapleVector<posPair> posVec(alloc.Adapter());
                for (auto pos : destPosVec) {
                    posVec.emplace_back(std::pair(pos.first, pos.second));
                }
                ranges.emplace(bbid, posVec);
            } else {
                /* merge it simply, so that subpos may overlap. */
                auto &srcPosVec = it->second;
                for (auto pos1 : destPosVec) {
                    bool merged = false;
                    for (auto &pos2 : srcPosVec) {
                        if (!((pos1.first < pos2.first && pos1.second < pos2.first) ||
                              (pos2.first < pos1.second && pos2.second < pos1.first))) {
                            uint32 bgn = pos1.first < pos2.first ? pos1.first : pos2.first;
                            uint32 end = pos1.second > pos2.second ? pos1.second : pos2.second;
                            pos2 = std::pair(bgn, end);
                            merged = true;
                        }
                    }
                    /* add it directly when no overlap*/
                    if (!merged) {
                        srcPosVec.emplace_back(std::pair(pos1.first, pos1.second));
                    }
                }
            }
        }
    }

    void MergeConflict(LiveInterval &lr)
    {
        for (auto reg : lr.conflict) {
            conflict.insert(reg);
        }
    }

    void MergeRefPoints(LiveInterval &lr)
    {
        if (lr.GetDefPoint().size() != k1ByteSize) {
            for (auto insn : lr.defPoints) {
                defPoints.insert(insn);
            }
        }
        for (auto insn : lr.usePoints) {
            usePoints.insert(insn);
        }
    }

    void AddConflict(regno_t val)
    {
        conflict.insert(val);
    }

    MapleSet<uint32> GetConflict()
    {
        return conflict;
    }

    void AddRefPoint(Insn *val, bool isDef)
    {
        if (isDef) {
            defPoints.insert(val);
        } else {
            usePoints.insert(val);
        }
    }

    InsnMapleSet &GetDefPoint()
    {
        return defPoints;
    }

    InsnMapleSet &GetUsePoint()
    {
        return usePoints;
    }

    bool IsConflictWith(regno_t val)
    {
        return conflict.find(val) != conflict.end();
    }

    RegType GetRegType() const
    {
        return regType;
    }

    void SetRegType(RegType val)
    {
        this->regType = val;
    }

    regno_t GetRegNO() const
    {
        return regno;
    }

    void SetRegNO(regno_t val)
    {
        this->regno = val;
    }

    void Dump()
    {
        std::cout << "R" << regno << ": ";
        for (auto range : ranges) {
            uint32 bbid = range.first;
            std::cout << "BB" << bbid << ": < ";
            for (auto pos : range.second) {
                std::cout << "[" << pos.first << ", " << pos.second << ") ";
            }
            std::cout << " > ";
        }
        std::cout << "\n";
    }
    void DumpDefs()
    {
        std::cout << "R" << regno << ": ";
        for (auto def : defPoints) {
            def->Dump();
        }
        std::cout << "\n";
    }
    void DumpUses()
    {
        std::cout << "R" << regno << ": ";
        for (auto def : usePoints) {
            def->Dump();
        }
        std::cout << "\n";
    }

private:
    MapleMap<uint32, MapleVector<posPair>> ranges;
    MapleSet<uint32> conflict;
    InsnMapleSet defPoints;
    InsnMapleSet usePoints;
    uint32 numCall = 0;
    RegType regType = kRegTyUndef;
    regno_t regno = 0;
    MapleAllocator &alloc;
};

class LiveIntervalAnalysis {
public:
    LiveIntervalAnalysis(CGFunc &func, MemPool &memPool)
        : cgFunc(&func), memPool(&memPool), alloc(&memPool), vregIntervals(alloc.Adapter())
    {
    }

    virtual ~LiveIntervalAnalysis()
    {
        cgFunc = nullptr;
        memPool = nullptr;
        bfs = nullptr;
    }

    virtual void ComputeLiveIntervals() = 0;
    virtual void CoalesceRegisters() = 0;
    void Run();
    void Analysis();
    void DoAnalysis();
    void ClearBFS();
    void Dump();
    void CoalesceLiveIntervals(LiveInterval &lrDest, LiveInterval &lrSrc);
    LiveInterval *GetLiveInterval(regno_t regno)
    {
        auto it = vregIntervals.find(regno);
        if (it == vregIntervals.end()) {
            return nullptr;
        } else {
            return it->second;
        }
    }

protected:
    CGFunc *cgFunc;
    MemPool *memPool;
    MapleAllocator alloc;
    MapleMap<regno_t, LiveInterval *> vregIntervals;
    Bfs *bfs = nullptr;
    bool runAnalysis = false;
};

MAPLE_FUNC_PHASE_DECLARE(CgRegCoalesce, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CGliveIntervalAnalysis, maplebe::CGFunc)
LiveIntervalAnalysis *GetResult()
{
    return liveInterval;
}
LiveIntervalAnalysis *liveInterval = nullptr;
OVERRIDE_DEPENDENCE
MAPLE_FUNC_PHASE_DECLARE_END
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_REGCOALESCE_H */
