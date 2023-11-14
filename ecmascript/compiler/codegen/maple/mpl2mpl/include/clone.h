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

#ifndef MPL2MPL_INCLUDE_CLONE_H
#define MPL2MPL_INCLUDE_CLONE_H
#include "mir_module.h"
#include "mir_function.h"
#include "mir_builder.h"
#include "mempool.h"
#include "mempool_allocator.h"
#include "class_hierarchy_phase.h"
#include "me_ir.h"
#include "maple_phase_manager.h"

static constexpr char kFullNameStr[] = "INFO_fullname";
static constexpr char kClassNameStr[] = "INFO_classname";
static constexpr char kFuncNameStr[] = "INFO_funcname";
static constexpr char kVoidRetSuffix[] = "CLONEDignoreret";
namespace maple {
class ReplaceRetIgnored {
public:
    explicit ReplaceRetIgnored(MemPool *memPool);
    ~ReplaceRetIgnored() = default;

    bool ShouldReplaceWithVoidFunc(const CallMeStmt &stmt, const MIRFunction &calleeFunc) const;
    std::string GenerateNewBaseName(const MIRFunction &originalFunc) const;
    std::string GenerateNewFullName(const MIRFunction &originalFunc) const;
    const MapleSet<MapleString> *GetTobeClonedFuncNames() const
    {
        return &toBeClonedFuncNames;
    }

    bool IsInCloneList(const std::string &funcName) const
    {
        return toBeClonedFuncNames.find(MapleString(funcName, memPool)) != toBeClonedFuncNames.end();
    }

    static bool IsClonedFunc(const std::string &funcName)
    {
        return funcName.find(kVoidRetSuffix) != std::string::npos;
    }

private:
    MemPool *memPool;
    maple::MapleAllocator allocator;
    MapleSet<MapleString> toBeClonedFuncNames;
    bool RealShouldReplaceWithVoidFunc(Opcode op, size_t nRetSize, const MIRFunction &calleeFunc) const;
};

class Clone : public AnalysisResult {
public:
    Clone(MIRModule *mod, MemPool *memPool, MIRBuilder &builder, KlassHierarchy *kh)
        : AnalysisResult(memPool),
          mirModule(mod),
          allocator(memPool),
          mirBuilder(builder),
          kh(kh),
          replaceRetIgnored(memPool->New<ReplaceRetIgnored>(memPool))
    {
    }

    ~Clone() = default;

    static MIRSymbol *CloneLocalSymbol(const MIRSymbol &oldSym, const MIRFunction &newFunc);
    static void CloneSymbols(MIRFunction &newFunc, const MIRFunction &oldFunc);
    static void CloneLabels(MIRFunction &newFunc, const MIRFunction &oldFunc);
    MIRFunction *CloneFunction(MIRFunction &originalFunction, const std::string &newBaseFuncName,
                               MIRType *returnType = nullptr) const;
    MIRFunction *CloneFunctionNoReturn(MIRFunction &originalFunction);
    void DoClone();
    void CopyFuncInfo(MIRFunction &originalFunction, MIRFunction &newFunc) const;
    void UpdateFuncInfo(MIRFunction &newFunc);
    void CloneArgument(MIRFunction &originalFunction, ArgVector &argument) const;
    const ReplaceRetIgnored *GetReplaceRetIgnored() const
    {
        return replaceRetIgnored;
    }

    void UpdateReturnVoidIfPossible(CallMeStmt *callMeStmt, const MIRFunction &targetFunc);

private:
    MIRModule *mirModule;
    MapleAllocator allocator;
    MIRBuilder &mirBuilder;
    KlassHierarchy *kh;
    ReplaceRetIgnored *replaceRetIgnored;
};

}  // namespace maple
#endif  // MPL2MPL_INCLUDE_CLONE_H
