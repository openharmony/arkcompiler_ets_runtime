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

#ifndef MAPLE_PHASE_INCLUDE_PHASE_IMPL_H
#define MAPLE_PHASE_INCLUDE_PHASE_IMPL_H
#include "mir_builder.h"
#include "mpl_scheduler.h"
#include "utils.h"

namespace maple {
class FuncOptimizeImpl : public MplTaskParam {
public:
    FuncOptimizeImpl(MIRModule &mod, bool trace = false);
    virtual ~FuncOptimizeImpl();
    // Each phase needs to implement its own Clone
    virtual FuncOptimizeImpl *Clone() = 0;
    MIRModule &GetMIRModule()
    {
        return *module;
    }

    const MIRModule &GetMIRModule() const
    {
        return *module;
    }

    void SetDump(bool dumpFunc)
    {
        dump = dumpFunc;
    }

    virtual void Finish() {}

protected:
    void SetCurrentFunction(MIRFunction &func)
    {
        currFunc = &func;
        DEBUG_ASSERT(builder != nullptr, "builder is null in FuncOptimizeImpl::SetCurrentFunction");
        builder->SetCurrentFunction(func);
        module->SetCurFunction(&func);
    }

    void SetCurrentBlock(BlockNode &block)
    {
        currBlock = &block;
    }

    MIRFunction *currFunc = nullptr;
    BlockNode *currBlock = nullptr;
    MIRBuilderExt *builder = nullptr;
    bool trace = false;
    bool dump = false;

private:
    MIRModule *module = nullptr;
};

class FuncOptimizeIterator : public MplScheduler {
public:
    FuncOptimizeIterator(const std::string &phaseName, std::unique_ptr<FuncOptimizeImpl> phaseImpl);
    virtual ~FuncOptimizeIterator();
    virtual void Run(uint32 threadNum = 1, bool isSeq = false);

protected:
    thread_local static FuncOptimizeImpl *phaseImplLocal;
    virtual MplTaskParam *CallbackGetTaskRunParam() const
    {
        return phaseImplLocal;
    }

    virtual MplTaskParam *CallbackGetTaskFinishParam() const
    {
        return phaseImplLocal;
    }

    virtual void CallbackThreadMainEnd()
    {
        delete phaseImplLocal;
        phaseImplLocal = nullptr;
    }

private:
    bool mplDumpTime = false;
    std::unique_ptr<FuncOptimizeImpl> phaseImpl;
    std::vector<std::unique_ptr<MplTask>> tasksUniquePtr;
};
}  // namespace maple
#endif  // MAPLE_PHASE_INCLUDE_PHASE_IMPL_H
