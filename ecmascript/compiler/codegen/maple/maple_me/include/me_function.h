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

#ifndef MAPLE_ME_INCLUDE_ME_FUNCTION_H
#define MAPLE_ME_INCLUDE_ME_FUNCTION_H
#include "mir_parser.h"
#include "mir_function.h"
#include "opcode_info.h"
#include "me_option.h"
#include "mempool.h"
#include "mempool_allocator.h"
#include "ver_symbol.h"
#include "bb.h"
#include "ssa_tab.h"
#include "func_emit.h"
#include "me_ir.h"
#include "profile.h"
#include "pme_function.h"

namespace maple {
class MeCFG;    // circular dependency exists, no other choice
class MeIRMap;  // circular dependency exists, no other choice
#if DEBUG
extern MIRModule *globalMIRModule;
extern MeFunction *globalFunc;
extern MeIRMap *globalIRMap;
extern SSATab *globalSSATab;
#endif

template <typename Iterator>
class FilterIterator {
public:
    using FilterFunc = std::function<bool(Iterator)>;
    using iterator_category = typename std::iterator_traits<Iterator>::iterator_category;
    using value_type = typename std::iterator_traits<Iterator>::value_type;
    using difference_type = typename std::iterator_traits<Iterator>::difference_type;
    using pointer = typename std::iterator_traits<Iterator>::pointer;
    using reference = typename std::iterator_traits<Iterator>::reference;
    using const_pointer = typename std::add_const<pointer>::type;
    using const_reference = typename std::add_const<reference>::type;

    explicit FilterIterator(Iterator it) : iterator(it) {}

    FilterIterator(Iterator it, FilterFunc func) : iterator(it), func(func)
    {
        while (!func(iterator)) {
            ++iterator;
        }
    }

    ~FilterIterator() = default;

    Iterator base() const
    {
        return iterator;
    }

    reference operator*() const
    {
        return *iterator;
    }

    pointer operator->() const
    {
        return iterator.operator->();
    }

    FilterIterator &operator++()
    {
        ++iterator;
        return func(iterator) ? *this : ++(*this);
    }

    FilterIterator &operator--()
    {
        --iterator;
        return func(iterator) ? *this : --(*this);
    }

    const FilterIterator operator++(int)
    {
        FilterIterator it = *this;
        ++(*this);
        return it;
    }

    const FilterIterator operator--(int)
    {
        FilterIterator it = *this;
        --(*this);
        return it;
    }

    bool operator==(const FilterIterator &it) const
    {
        return this->iterator == it.iterator;
    }

    bool operator!=(const FilterIterator &it) const
    {
        return !(*this == it);
    }

    bool operator==(const Iterator &it) const
    {
        return this->iterator == it;
    }

    bool operator!=(const Iterator &it) const
    {
        return !(*this == it);
    }

private:
    static bool FilterNone(Iterator)
    {
        return true;
    }
    Iterator iterator;
    FilterFunc func = FilterIterator::FilterNone;
};

template <typename Iterator>
inline auto build_filter_iterator(Iterator it) -> FilterIterator<Iterator>
{
    return FilterIterator<Iterator>(it);
}

template <typename Iterator, typename _Func>
inline auto build_filter_iterator(Iterator it, _Func func) -> FilterIterator<Iterator>
{
    return FilterIterator<Iterator>(it, std::function<bool(Iterator)>(func));
}

template <typename Iterator>
inline auto build_filter_iterator(Iterator it, std::function<bool(Iterator)> func) -> FilterIterator<Iterator>
{
    return FilterIterator<Iterator>(it, func);
}

template <typename Iterator>
bool FilterNullPtr(Iterator it, Iterator endIt)
{
    return it == endIt || *it != nullptr;
}

enum MeFuncHint {
    kReserved = 0x00,       // reserved
    kPlacementRCed = 0x01,  // method processed by placementrc
    kAnalyzeRCed = 0x02,    // method processed by analyzerc
    kRcLowered = 0x04,      // method lowered by rclowering
};

class MeFunction : public FuncEmit {
public:
    MeFunction(MIRModule *mod, MIRFunction *func, MemPool *memPool, StackMemPool &funcStackMP, MemPool *versMemPool,
               const std::string &fileName)
        : memPool(memPool),
          stackMP(funcStackMP),
          alloc(memPool),
          versMemPool(versMemPool),
          versAlloc(versMemPool),
          mirModule(*mod),
          mirFunc(func),
          laidOutBBVec(alloc.Adapter()),
          fileName(fileName, memPool),
          preMeFunc(nullptr),
          preMeMp(nullptr)
    {
    }

    ~MeFunction() override = default;
    void Verify() const;
    const std::string &GetName() const
    {
        return mirFunc->GetName();
    }

    VersionSt *GetVerSt(size_t veridx) const
    {
        return meSSATab->GetVerSt(veridx);
    }

    /* get or create label for bb */
    LabelIdx GetOrCreateBBLabel(BB &bb);

    bool IsEmpty() const
    {
        return mirFunc->IsEmpty();
    }

    bool HasException() const
    {
        return hasEH;
    }

    MapleAllocator &GetAlloc()
    {
        return alloc;
    }
    MapleAllocator &GetVersAlloc()
    {
        return versAlloc;
    }
    MemPool *GetVersMp()
    {
        // version mempool may be release if invalid
        if (versMemPool == nullptr) {
            versMemPool = new ThreadLocalMemPool(memPoolCtrler, "verst mempool");
            versAlloc.SetMemPool(versMemPool);
        }
        return versMemPool;
    }

    void ReleaseVersMemory()
    {
        if (versMemPool) {
            memPoolCtrler.DeleteMemPool(versMemPool);
            versMemPool = nullptr;
            versAlloc.SetMemPool(nullptr);
        }
    }

    const MapleVector<BB *> &GetLaidOutBBs() const
    {
        return laidOutBBVec;
    }

    void SetLaidOutBBs(const MapleVector<BB *> &laidout)
    {
        laidOutBBVec = laidout;
    }

    bool HasLaidOut() const
    {
        return !laidOutBBVec.empty();
    }

    void ClearLayout()
    {
        if (laidOutBBVec.empty()) {
            return;
        }
        laidOutBBVec.clear();
    }

    SSATab *GetMeSSATab()
    {
        return meSSATab;
    }
    void SetMeSSATab(SSATab *currMessaTab)
    {
        meSSATab = currMessaTab;
    }

    MIRFunction *GetMirFunc()
    {
        return mirFunc;
    }

    MIRModule &GetMIRModule() const
    {
        return mirModule;
    }

    MeIRMap *GetIRMap()
    {
        return irmap;
    }

    void SetIRMap(MeIRMap *currIRMap)
    {
        irmap = currIRMap;
    }

    MeCFG *GetCfg() const
    {
        return theCFG;
    }

    void SetTheCfg(MeCFG *currTheCfg)
    {
        theCFG = currTheCfg;
    }

    uint32 GetRegNum() const
    {
        return regNum;
    }

    void SetRegNum(uint32 num)
    {
        regNum = num;
    }

    uint32 GetHints() const
    {
        return hints;
    }

    void SetHints(uint32 num)
    {
        hints = num;
    }

    MemPool *GetMemPool()
    {
        return memPool;
    }

    StackMemPool &GetStackMemPool()
    {
        return stackMP;
    }

    void AddProfileDesc(uint64 hash, uint32 start, uint32 end)
    {
        mirFunc->AddProfileDesc(hash, start, end);
    }

    const IRProfileDesc *GetProfInf()
    {
        if (profileDesc == nullptr) {
            // return profileDesc with default value
            profileDesc = memPool->New<IRProfileDesc>();
        }
        return profileDesc;
    }
    bool IsIRProfValid() const
    {
        return profValid;
    }

    void SetProfValid(bool val)
    {
        profValid = val;
    }

    uint32 GetFrequency() const
    {
        return frequency;
    }

    void SetFrequency(uint32 f)
    {
        frequency = f;
    }

    void SetHasWriteInputAsmNode()
    {
        hasWriteInputAsmNode = true;
    };

    bool HasWriteInputAsmNode() const
    {
        return hasWriteInputAsmNode;
    }

    uint32 GetUniqueID() const
    {
        return mirFunc->GetPuidx();
    }

    bool IsUnSafe() const
    {
        return !mirFunc->GetAttr(FUNCATTR_safed) || mirFunc->GetAttr(FUNCATTR_unsafed);
    }

    bool IsSafe() const
    {
        return mirFunc->GetAttr(FUNCATTR_safed);
    }

    void PartialInit();

    void Release();

    MIRFunction *CurFunction() const
    {
        return mirModule.CurFunction();
    }
    void SetPreMeFunc(PreMeFunction *lfoF)
    {
        preMeFunc = lfoF;
    }
    PreMeFunction *GetPreMeFunc()
    {
        return preMeFunc;
    }
    void SetPmeMempool(MemPool *mp)
    {
        preMeMp = mp;
    }
    MemPool *GetPmeMempool()
    {
        return preMeMp;
    }
    bool IsLfo() const
    {
        return isLfo;
    }
    void SetLfo(bool islfo)
    {
        isLfo = islfo;
    }
    bool IsPme() const
    {
        return isPme;
    }
    void SetPme(bool set)
    {
        isPme = set;
    }
    bool IsTopLevelSSAValid() const
    {
        return static_cast<bool>(state & kSSATopLevel);
    }
    bool IsAddrTakenSSAValid() const
    {
        return static_cast<bool>(state & kSSAAddrTaken);
    }
    bool IsMemSSAValid()
    {
        return IsTopLevelSSAValid() && IsAddrTakenSSAValid();
    }
    bool IsMeIRAvailable() const
    {
        return static_cast<bool>(state & kSSAHSSA);
    }
    bool IsMplIRAvailable() const
    {
        return !static_cast<bool>(state & kSSAHSSA);
    }

    void ResetMeFuncState(uint8 s)
    {
        state &= ~s;
    }

    void SetMeFuncState(uint8 s)
    {
        state |= s;
    }

private:
    MemPool *memPool;
    StackMemPool &stackMP;
    MapleAllocator alloc;
    MemPool *versMemPool;
    MapleAllocator versAlloc;
    MIRModule &mirModule;
    MIRFunction *mirFunc;
    /* mempool */
    MapleVector<BB *> laidOutBBVec;
    MeCFG *theCFG = nullptr;
    SSATab *meSSATab = nullptr;
    MeIRMap *irmap = nullptr;
    /* input */
    MapleString fileName;
    uint32 regNum = 0;  // count virtual registers
    uint32 hints = 0;
    bool hasEH = false;                /* current has try statement */
    bool hasWriteInputAsmNode = false; /* set when ssa tab build */
    bool profValid = false;
    IRProfileDesc *profileDesc = nullptr;
    uint32 frequency = 0;
    // lfo
    bool isLfo = false;
    // during pme processing
    bool isPme = false;
    PreMeFunction *preMeFunc;
    MemPool *preMeMp;  // used for lfo function
    uint8 state = 0;   // current state : ssa level and ir flavor
public:
    uint32 dseRuns = 0;    // number of times dse phase has been run
    uint32 hdseRuns = 0;   // number of times hdse phase has been run
    uint32 hpropRuns = 0;  // number of times hprop phase has been run
    uint32 vrpRuns = 0;    // number of times vrp phase has been run
    bool genLMBC = false;  // whether outputing lmbc (low maple bytecode)
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_FUNCTION_H
