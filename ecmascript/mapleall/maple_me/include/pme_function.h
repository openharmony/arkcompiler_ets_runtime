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

#ifndef MAPLE_ME_INCLUDE_PME_FUNCTION_H
#define MAPLE_ME_INCLUDE_PME_FUNCTION_H
#include "pme_mir_extension.h"
#include "me_ir.h"

namespace maple {
class MeFunction;

class PreMeWhileInfo {
public:
    MIRSymbol *injectedIVSym = nullptr;  // the injected IV
    OriginalSt *ivOst = nullptr;         // the primary IV
    MeExpr *initExpr = nullptr;
    int32 stepValue = 0;
    MeExpr *tripCount = nullptr;
    bool canConvertDoloop = false;
};

class PreMeIfInfo {
public:
    LabelIdx endLabel = 0;   // the label that is out of the if statement
    LabelIdx elseLabel = 0;  // the label that is the begin of else branch
};

class PreMeFunction {
public:
    MemPool *pmemp;
    MapleAllocator pmeAlloc;
    MeFunction *meFunc;
    // key is label at beginning of lowered while code sequence
    MapleMap<LabelIdx, PreMeWhileInfo *> label2WhileInfo;
    // key is target label of first conditional branch of lowered if code sequence
    MapleMap<LabelIdx, PreMeIfInfo *> label2IfInfo;
    // for the labels that were created by PreMe, we won't emit it
    MapleSet<LabelIdx> pmeCreatedIfLabelSet;
    MapleSet<LabelIdx> pmeCreatedWhileLabelSet;

    PreMeFunction(MemPool *mp, MeFunction *func)
        : pmemp(mp),
          pmeAlloc(mp),
          meFunc(func),
          label2WhileInfo(pmeAlloc.Adapter()),
          label2IfInfo(pmeAlloc.Adapter()),
          pmeCreatedIfLabelSet(pmeAlloc.Adapter()),
          pmeCreatedWhileLabelSet(pmeAlloc.Adapter())
    {
    }
    virtual ~PreMeFunction() = default;

    void SetIfLabelCreatedByPreMe(LabelIdx lbidx)
    {
        pmeCreatedIfLabelSet.insert(lbidx);
    }

    bool IfLabelCreatedByPreMe(LabelIdx lbidx) const
    {
        return pmeCreatedIfLabelSet.count(lbidx) != 0;
    }

    void SetWhileLabelCreatedByPreMe(LabelIdx lbidx)
    {
        pmeCreatedWhileLabelSet.insert(lbidx);
    }

    bool WhileLabelCreatedByPreMe(LabelIdx lbidx) const
    {
        return pmeCreatedWhileLabelSet.count(lbidx) != 0;
    }
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_PME_FUNCTION_H
