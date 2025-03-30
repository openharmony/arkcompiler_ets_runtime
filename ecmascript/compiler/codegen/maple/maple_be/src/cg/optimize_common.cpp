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

#include "optimize_common.h"
#include <fstream>

/* This file provides common class and function for cfgo and ico. */
namespace maplebe {
void Optimizer::Run(const std::string &funcName, bool checkOnly)
{
    /* Initialize cfg optimization patterns */
    InitOptimizePatterns();

    /* For each pattern, search cgFunc for optimization */
    for (OptimizationPattern *p : diffPassPatterns) {
        p->Search2Op(checkOnly);
    }
    /* Search the cgFunc for multiple possible optimizations in one pass */
    if (!singlePassPatterns.empty()) {
        bool changed = false;
        do {
            changed = false;
            for (OptimizationPattern *p : singlePassPatterns) {
                BB *curBB = cgFunc->GetFirstBB();
                while (curBB != nullptr) {
                    if (p->Optimize(*curBB)) {
                        changed = true;
                    }
                    if (p->IsKeepPosition()) {
                        p->SetKeepPosition(false);
                    } else {
                        curBB = curBB->GetNext();
                    }
                }
            }
        } while (changed);
    }

    // Update commonExitBB info, especially in infinite loop case.
    // But we can not get the commonExitBB by traversal CFG, so
    // it needs to be handled separately.
    cgFunc->GetTheCFG()->UpdateCommonExitBBInfo();

    OptimizeLogger::GetLogger().ClearLocal();
}

bool OptimizationPattern::Search2Op(bool noOptimize)
{
    bool hasChange = false;
    checkOnly = noOptimize;
    InitPattern();
    BB *curBB = cgFunc->GetFirstBB();
    while (curBB != nullptr) {
        bool changed = false;
        do {
            changed = Optimize(*curBB);
            hasChange = hasChange || changed;
        } while (changed);
        if (keepPosition) {
            keepPosition = false;
        } else {
            curBB = curBB->GetNext();
        }
    }
    return hasChange;
}

void OptimizationPattern::Log(uint32 bbID)
{
    OptimizeLogger::GetLogger().Log(patternName.c_str());
    DotGenerator::SetColor(bbID, dotColor.c_str());
}

std::map<uint32, std::string> DotGenerator::coloringMap;

void DotGenerator::SetColor(uint32 bbID, const std::string &color)
{
    coloringMap[bbID] = color;
}

std::string DotGenerator::GetFileName(const MIRModule &mirModule, const std::string &filePreFix)
{
    std::string fileName;
    if (!filePreFix.empty()) {
        fileName.append(filePreFix);
        fileName.append("-");
    }
    fileName.append(mirModule.GetFileName());
    for (uint32 i = 0; i < fileName.length(); i++) {
        if (fileName[i] == ';' || fileName[i] == '/' || fileName[i] == '|') {
            fileName[i] = '_';
        }
    }

    fileName.append(".dot");
    return fileName;
}

void DotGenerator::DumpEdge(const CGFunc &cgFunction, std::ofstream &cfgFileOfStream)
{
    FOR_ALL_BB_CONST(bb, &cgFunction)
    {
        for (auto *succBB : bb->GetSuccs()) {
            cfgFileOfStream << "BB" << bb->GetId() << " -> " << "BB" << succBB->GetId() << " [color=green];\n";
        }
    }
}

bool DotGenerator::FoundListOpndRegNum(ListOperand &listOpnd, const Insn &insnObj, regno_t vReg)
{
    bool exist = false;
    for (auto op : listOpnd.GetOperands()) {
        RegOperand *regOpnd = static_cast<RegOperand *>(op);
        if (op->IsRegister() && regOpnd->GetRegisterNumber() == vReg) {
            LogInfo::MapleLogger() << "BB" << insnObj.GetBB()->GetId() << " [style=filled, fillcolor=red];\n";
            exist = true;
            break;
        }
    }
    return exist;
}

bool DotGenerator::FoundMemAccessOpndRegNum(const MemOperand &memOperand, const Insn &insnObj, regno_t vReg)
{
    Operand *base = memOperand.GetBaseRegister();
    Operand *offset = memOperand.GetIndexRegister();
    bool exist = false;
    if (base != nullptr && base->IsRegister()) {
        RegOperand *regOpnd = static_cast<RegOperand *>(base);
        if (regOpnd->GetRegisterNumber() == vReg) {
            LogInfo::MapleLogger() << "BB" << insnObj.GetBB()->GetId() << " [style=filled, fillcolor=red];\n";
            exist = true;
        }
    } else if (offset != nullptr && offset->IsRegister()) {
        RegOperand *regOpnd = static_cast<RegOperand *>(offset);
        if (regOpnd->GetRegisterNumber() == vReg) {
            LogInfo::MapleLogger() << "BB" << insnObj.GetBB()->GetId() << " [style=filled, fillcolor=red];\n";
            exist = true;
        }
    }
    return exist;
}

bool DotGenerator::FoundNormalOpndRegNum(const RegOperand &regOpnd, const Insn &insnObj, regno_t vReg)
{
    bool exist = false;
    if (regOpnd.GetRegisterNumber() == vReg) {
        LogInfo::MapleLogger() << "BB" << insnObj.GetBB()->GetId() << " [style=filled, fillcolor=red];\n";
        exist = true;
    }
    return exist;
}

void DotGenerator::DumpBBInstructions(const CGFunc &cgFunction, regno_t vReg, std::ofstream &cfgFile)
{
    FOR_ALL_BB_CONST(bb, &cgFunction)
    {
        if (vReg != 0) {
            FOR_BB_INSNS_CONST(insn, bb)
            {
                bool found = false;
                uint32 opndNum = insn->GetOperandSize();
                for (uint32 i = 0; i < opndNum; ++i) {
                    Operand &opnd = insn->GetOperand(i);
                    if (opnd.IsList()) {
                        auto &listOpnd = static_cast<ListOperand &>(opnd);
                        found = FoundListOpndRegNum(listOpnd, *insn, vReg);
                    } else if (opnd.IsMemoryAccessOperand()) {
                        auto &memOpnd = static_cast<MemOperand &>(opnd);
                        found = FoundMemAccessOpndRegNum(memOpnd, *insn, vReg);
                    } else {
                        if (opnd.IsRegister()) {
                            auto &regOpnd = static_cast<RegOperand &>(opnd);
                            found = FoundNormalOpndRegNum(regOpnd, *insn, vReg);
                        }
                    }
                    if (found) {
                        break;
                    }
                }
                if (found) {
                    break;
                }
            }
        }
        cfgFile << "BB" << bb->GetId() << "[";
        auto it = coloringMap.find(bb->GetId());
        if (it != coloringMap.end()) {
            cfgFile << "style=filled,fillcolor=" << it->second << ",";
        }
        if (bb->GetKind() == BB::kBBIf) {
            cfgFile << "shape=diamond,label= \" BB" << bb->GetId() << ":\n";
        } else {
            cfgFile << "shape=box,label= \" BB" << bb->GetId() << ":\n";
        }
        cfgFile << "{ ";
        cfgFile << bb->GetKindName() << "\n";
        cfgFile << bb->GetFrequency() << "\n";
        if (bb->GetLabIdx() != 0) {
            cfgFile << "LabIdx=" << bb->GetLabIdx() << "\n";
        }
        cfgFile << "}\"];\n";
    }
}

/* Generate dot file for cfg */
void DotGenerator::GenerateDot(const std::string &preFix, const CGFunc &cgFunc, const MIRModule &mod,
                               const std::string fname, regno_t vReg)
{
    std::ofstream cfgFile;
    std::streambuf *coutBuf = std::cout.rdbuf(); /* keep original cout buffer */
    std::streambuf *buf = cfgFile.rdbuf();
    std::cout.rdbuf(buf);
    std::string fileName = GetFileName(mod, (preFix + "-" + fname));

    cfgFile.open(fileName, std::ios::trunc);
    CHECK_FATAL(cfgFile.is_open(), "Failed to open output file: %s", fileName.c_str());
    cfgFile << "digraph {\n";
    /* dump edge */
    DumpEdge(cgFunc, cfgFile);

    /* dump instruction in each BB */
    DumpBBInstructions(cgFunc, vReg, cfgFile);

    cfgFile << "}\n";
    coloringMap.clear();
    cfgFile.flush();
    cfgFile.close();
    std::cout.rdbuf(coutBuf);
}

void OptimizeLogger::Print(const std::string &funcName)
{
    if (!localStat.empty()) {
        LogInfo::MapleLogger() << funcName << '\n';
        for (const auto &localStatPair : localStat) {
            LogInfo::MapleLogger() << "Optimized " << localStatPair.first << ":" << localStatPair.second << "\n";
        }

        ClearLocal();
        LogInfo::MapleLogger() << "Total:" << '\n';
        for (const auto &globalStatPair : globalStat) {
            LogInfo::MapleLogger() << "Optimized " << globalStatPair.first << ":" << globalStatPair.second << "\n";
        }
    }
}

void OptimizeLogger::Log(const std::string &patternName)
{
    auto itemInGlobal = globalStat.find(patternName);
    if (itemInGlobal != globalStat.end()) {
        itemInGlobal->second++;
    } else {
        (void)globalStat.emplace(std::pair<const std::string, int>(patternName, 1));
    }
    auto itemInLocal = localStat.find(patternName);
    if (itemInLocal != localStat.end()) {
        itemInLocal->second++;
    } else {
        (void)localStat.emplace(std::pair<const std::string, int>(patternName, 1));
    }
}

void OptimizeLogger::ClearLocal()
{
    localStat.clear();
}
} /* namespace maplebe */
