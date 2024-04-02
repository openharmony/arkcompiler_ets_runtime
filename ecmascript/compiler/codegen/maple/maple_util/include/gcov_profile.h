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

#ifndef MAPLE_UTIL_INCLUDE_GCOV_PROFILE_H
#define MAPLE_UTIL_INCLUDE_GCOV_PROFILE_H

#include "mempool_allocator.h"
#include <string>
#include <unordered_map>

namespace maple {
typedef unsigned gcov_unsigned_t;
typedef int64_t gcov_type;
typedef uint64_t gcov_type_unsigned;
typedef unsigned location_t;
#define HOTCALLSITEFREQ 100

enum UpdateFreqOp {
    kKeepOrigFreq = 0,
    kUpdateOrigFreq = 0x1,
    kUpdateFreqbyScale = 0x2,
    kUpdateUnrolledFreq = 0x4,
    kUpdateUnrollRemainderFreq = 0x8,
};

class GcovFuncInfo {
public:
    GcovFuncInfo(MapleAllocator *alloc, unsigned funcIdent, unsigned lineno_cs, unsigned cfg_cs)
        : ident(funcIdent), lineno_checksum(lineno_cs), cfg_checksum(cfg_cs), counts(alloc->Adapter()) {};
    ~GcovFuncInfo() = default;

    int64_t GetFuncFrequency() const
    {
        return entry_freq;
    }
    void SetFuncFrequency(int64_t freq)
    {
        entry_freq = freq;
    }

    int64_t GetFuncRealFrequency() const
    {
        return real_entryfreq;
    }
    void SetFuncRealFrequency(int64_t freq)
    {
        real_entryfreq = freq;
    }

    std::unordered_map<uint32_t, uint64_t> &GetStmtFreqs()
    {
        return stmtFreqs;
    }
    int64_t GetStmtFreq(uint32_t stmtID)
    {
        if (stmtFreqs.count(stmtID) > 0) {
            return stmtFreqs[stmtID];
        }
        return -1;  // unstored
    }
    void SetStmtFreq(uint32_t stmtID, int64_t freq)
    {
        stmtFreqs[stmtID] = static_cast<uint64_t>(freq);
    }
    void EraseStmtFreq(uint32_t stmtID)
    {
        stmtFreqs.erase(stmtID);
    }
    void CopyStmtFreq(uint32_t newStmtID, uint32_t origStmtId, bool deleteOld = false)
    {
        DEBUG_ASSERT(GetStmtFreq(origStmtId) >= 0, "origStmtId no freq record");
        SetStmtFreq(newStmtID, GetStmtFreq(origStmtId));
        if (deleteOld) {
            EraseStmtFreq(origStmtId);
        }
    }
    bool IsHotCallSite(uint32_t stmtID)
    {
        if (stmtFreqs.count(stmtID) > 0) {
            uint64 freq = stmtFreqs[stmtID];
            return (freq >= HOTCALLSITEFREQ);
        }
        DEBUG_ASSERT(0, "should not be here");
        return false;
    }
    unsigned ident;
    unsigned lineno_checksum;
    unsigned cfg_checksum;

    // Raw arc coverage counts.
    unsigned num_counts;
    MapleVector<gcov_type> counts;
    int64_t entry_freq;                                // record entry bb frequence
    std::unordered_map<uint32_t, uint64_t> stmtFreqs;  // stmt_id is key, counter value
    int64_t real_entryfreq;                            // function prof data may be modified after clone/inline
};

class GcovProfileData {
public:
    GcovProfileData(MapleAllocator *alloc) : funcsCounter(alloc->Adapter()) {}

    GcovFuncInfo *GetFuncProfile(unsigned puidx)
    {
        if (funcsCounter.count(puidx) > 0) {
            return funcsCounter[puidx];
        }
        return nullptr;
    }
    void AddFuncProfile(unsigned puidx, GcovFuncInfo *funcData)
    {
        DEBUG_ASSERT(funcsCounter.count(puidx) == 0, "sanity check");
        funcsCounter[puidx] = funcData;
    }
    MapleUnorderedMap<unsigned, GcovFuncInfo *> funcsCounter;  // use puidx as key
};

}  // namespace maple
#endif  // MAPLE_UTIL_INCLUDE_GCOV_PROFILE_H
