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
#ifndef MAPLE_PGO_INCLUDE_INSTRUMENT_H
#define MAPLE_PGO_INCLUDE_INSTRUMENT_H

#include "types_def.h"
#include "cfg_mst.h"
#include "mir_function.h"

namespace maple {
MIRSymbol *GetOrCreateFuncCounter(MIRFunction &func, uint32 elemCnt, uint32 cfgHash);

template <typename BB>
class BBEdge {
public:
    BBEdge(BB *src, BB *dest, uint64 w = 1, bool isCritical = false, bool isFake = false)
        : srcBB(src), destBB(dest), weight(w), inMST(false), isCritical(isCritical), isFake(isFake)
    {
    }

    ~BBEdge() = default;

    BB *GetSrcBB()
    {
        return srcBB;
    }

    BB *GetDestBB()
    {
        return destBB;
    }

    uint64 GetWeight() const
    {
        return weight;
    }

    void SetWeight(uint64 w)
    {
        weight = w;
    }

    bool IsCritical() const
    {
        return isCritical;
    }

    bool IsFake() const
    {
        return isFake;
    }

    bool IsInMST() const
    {
        return inMST;
    }

    void SetInMST()
    {
        inMST = true;
    }

    int32 GetCondition() const
    {
        return condition;
    }

    void SetCondition(int32 cond)
    {
        condition = cond;
    }

    bool IsBackEdge() const
    {
        return isBackEdge;
    }

    void SetIsBackEdge()
    {
        isBackEdge = true;
    }

private:
    BB *srcBB;
    BB *destBB;
    uint64 weight;
    bool inMST;
    bool isCritical;
    bool isFake;
    int32 condition = -1;
    bool isBackEdge = false;
};

template <typename BB>
class BBUseEdge : public BBEdge<BB> {
public:
    BBUseEdge(BB *src, BB *dest, uint64 w = 1, bool isCritical = false, bool isFake = false)
        : BBEdge<BB>(src, dest, w, isCritical, isFake)
    {
    }
    virtual ~BBUseEdge() = default;
    void SetCount(uint64 value)
    {
        countValue = value;
        valid = true;
    }

    uint64 GetCount() const
    {
        return countValue;
    }

    bool GetStatus() const
    {
        return valid;
    }

private:
    bool valid = false;
    uint64 countValue = 0;
};

template <class IRBB, class Edge>
class PGOInstrumentTemplate {
public:
    explicit PGOInstrumentTemplate(MemPool &mp) : mst(mp) {}

    void GetInstrumentBBs(std::vector<IRBB *> &bbs, IRBB *commonEntry) const;
    void PrepareInstrumentInfo(IRBB *commonEntry, IRBB *commmonExit)
    {
        mst.ComputeMST(commonEntry, commmonExit);
    }
    const MapleVector<Edge *> &GetAllEdges()
    {
        return mst.GetAllEdges();
    }

private:
    CFGMST<Edge, IRBB> mst;
};

template <typename BB>
class BBUseInfo {
public:
    explicit BBUseInfo(MemPool &tmpPool)
        : innerAlloc(&tmpPool), inEdges(innerAlloc.Adapter()), outEdges(innerAlloc.Adapter())
    {
    }
    virtual ~BBUseInfo() = default;
    void SetCount(uint64 value)
    {
        countValue = value;
        valid = true;
    }
    uint64 GetCount() const
    {
        return countValue;
    }

    bool GetStatus() const
    {
        return valid;
    }

    void AddOutEdge(BBUseEdge<BB> *e)
    {
        outEdges.push_back(e);
        if (!e->GetStatus()) {
            unknownOutEdges++;
        }
    }

    void AddInEdge(BBUseEdge<BB> *e)
    {
        inEdges.push_back(e);
        if (!e->GetStatus()) {
            unknownInEdges++;
        }
    }

    const MapleVector<BBUseEdge<BB> *> &GetInEdges() const
    {
        return inEdges;
    }

    MapleVector<BBUseEdge<BB> *> &GetInEdges()
    {
        return inEdges;
    }

    size_t GetInEdgeSize() const
    {
        return inEdges.size();
    }

    const MapleVector<BBUseEdge<BB> *> &GetOutEdges() const
    {
        return outEdges;
    }

    MapleVector<BBUseEdge<BB> *> &GetOutEdges()
    {
        return outEdges;
    }

    size_t GetOutEdgeSize() const
    {
        return outEdges.size();
    }

    void DecreaseUnKnownOutEdges()
    {
        unknownOutEdges--;
    }

    void DecreaseUnKnownInEdges()
    {
        unknownInEdges--;
    }

    uint32 GetUnknownOutEdges() const
    {
        return unknownOutEdges;
    }

    BBUseEdge<BB> *GetOnlyUnknownOutEdges();

    uint32 GetUnknownInEdges() const
    {
        return unknownInEdges;
    }

    BBUseEdge<BB> *GetOnlyUnknownInEdges();

    void Dump();

private:
    bool valid = false;
    uint64 countValue = 0;
    uint32 unknownInEdges = 0;
    uint32 unknownOutEdges = 0;
    MapleAllocator innerAlloc;
    MapleVector<BBUseEdge<BB> *> inEdges;
    MapleVector<BBUseEdge<BB> *> outEdges;
};
} /* namespace maple */
#endif  // MAPLE_PGO_INCLUDE_INSTRUMENT_H
