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

#ifndef MAPLEBE_INCLUDE_CG_OPTIMIZE_COMMON_H
#define MAPLEBE_INCLUDE_CG_OPTIMIZE_COMMON_H
#include "cgfunc.h"

namespace maplebe {
inline const std::string kCfgoChaining = "red";
inline const std::string kCfgoSj = "burlywood1";
inline const std::string kCfgoFlipCond = "cadetblue1";
inline const std::string kCfgoAlways = "green";
inline const std::string kCfgoUnreach = "yellow";
inline const std::string kCfgoDup = "orange";
inline const std::string kCfgoEmpty = "purple";
inline const std::string kCfgoCrossJump = "brown";
inline const std::string kIcoIte = "blue"; /* if conversion optimization, if-then-else */
inline const std::string kIcoIt = "grey";  /* if conversion optimization, if-then-else */
inline const std::string kDup = "maize";   /* dup optimization */

class OptimizationPattern {
public:
    explicit OptimizationPattern(CGFunc &func)
        : patternName(func.GetMemoryPool()), cgFunc(&func), dotColor(func.GetMemoryPool())
    {
    }
    virtual ~OptimizationPattern() = default;

    bool IsKeepPosition() const
    {
        return keepPosition;
    }

    void SetKeepPosition(bool flag)
    {
        keepPosition = flag;
    }

    bool IsLabelInSwitchTable(LabelIdx label) const
    {
        return CGCFG::InSwitchTable(label, *cgFunc);
    }

    bool Search2Op(bool noOptimize);
    virtual void InitPattern() {}
    virtual bool Optimize(BB &curBB) = 0;

protected:
    void Log(uint32 bbID);

    bool keepPosition = false;
    MapleString patternName;
    CGFunc *cgFunc;
    MapleString dotColor;
    bool checkOnly = false;
};

class Optimizer {
public:
    Optimizer(CGFunc &func, MemPool &memPool)
        : cgFunc(&func),
          name(nullptr),
          memPool(&memPool),
          alloc(&memPool),
          diffPassPatterns(alloc.Adapter()),
          singlePassPatterns(alloc.Adapter())
    {
        func.GetTheCFG()->InitInsnVisitor(func);
    }

    virtual ~Optimizer() = default;
    void Run(const std::string &funcName, bool checkOnly = false);
    virtual void InitOptimizePatterns() = 0;

protected:
    CGFunc *cgFunc;
    const char *name;
    MemPool *memPool;
    MapleAllocator alloc;
    /* patterns need to run in different passes of cgFunc */
    MapleVector<OptimizationPattern *> diffPassPatterns;
    /* patterns can run in a single pass of cgFunc */
    MapleVector<OptimizationPattern *> singlePassPatterns;
};

class OptimizeLogger {
public:
    static OptimizeLogger &GetLogger()
    {
        static OptimizeLogger instance;
        return instance;
    }

    void Log(const std::string &patternName);
    void ClearLocal();
    void Print(const std::string &funcName);

private:
    OptimizeLogger() = default;

    ~OptimizeLogger() = default;

    OptimizeLogger(const OptimizeLogger &);
    OptimizeLogger &operator=(const OptimizeLogger &);

    std::map<const std::string, int> globalStat;
    std::map<const std::string, int> localStat;
};

class DotGenerator {
public:
    static void SetColor(uint32 bbID, const std::string &color);
    static void GenerateDot(const std::string &preFix, const CGFunc &cgFunc, const MIRModule &mod,
                            const std::string fname = "", regno_t vReg = 0);

private:
    static std::map<uint32, std::string> coloringMap;
    static std::string GetFileName(const MIRModule &mirModule, const std::string &filePreFix);
    static void DumpEdge(const CGFunc &cgFunction, std::ofstream &cfgFileOfStream);
    static void DumpBBInstructions(const CGFunc &cgFunction, regno_t vReg, std::ofstream &cfgFile);
    static bool FoundListOpndRegNum(ListOperand &listOpnd, const Insn &insnObj, regno_t vReg);
    static bool FoundMemAccessOpndRegNum(const MemOperand &memOperand, const Insn &insnObj, regno_t vReg);
    static bool FoundNormalOpndRegNum(const RegOperand &regOpnd, const Insn &insnObj, regno_t vReg);
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_OPTIMIZE_COMMON_H */
