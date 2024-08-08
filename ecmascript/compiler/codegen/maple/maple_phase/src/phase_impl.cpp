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

#include "phase_impl.h"
#include <cstdlib>
#include "mpl_timer.h"
#include "option.h"

namespace maple {
// ========== FuncOptimizeImpl ==========
FuncOptimizeImpl::FuncOptimizeImpl(MIRModule &mod, bool currTrace)
    : trace(currTrace), module(&mod)
{
    builder = new (std::nothrow) MIRBuilderExt(module);
    CHECK_FATAL(builder != nullptr, "New fail in FuncOptimizeImpl ctor!");
}

FuncOptimizeImpl::~FuncOptimizeImpl()
{
    if (builder != nullptr) {
        delete builder;
        builder = nullptr;
    }
    currFunc = nullptr;
    module = nullptr;
}

void FuncOptimizeImpl::CreateLocalBuilder(pthread_mutex_t &mtx)
{
    // Each thread needs to use its own MIRBuilderExt.
    builder = new (std::nothrow) MIRBuilderExt(module, &mtx);
    CHECK_FATAL(builder != nullptr, "New fail in CreateLocalBuilder ctor!");
}

void FuncOptimizeImpl::ProcessFunc(MIRFunction *func)
{
    currFunc = func;
    SetCurrentFunction(*func);
    if (func->GetBody() != nullptr) {
        ProcessBlock(*func->GetBody());
    }
}

void FuncOptimizeImpl::ProcessBlock(StmtNode &stmt)
{
    switch (stmt.GetOpCode()) {
        case OP_if: {
            ProcessStmt(stmt);
            IfStmtNode &ifStmtNode = static_cast<IfStmtNode &>(stmt);
            if (ifStmtNode.GetThenPart() != nullptr) {
                ProcessBlock(*ifStmtNode.GetThenPart());
            }
            if (ifStmtNode.GetElsePart() != nullptr) {
                ProcessBlock(*ifStmtNode.GetElsePart());
            }
            break;
        }
        case OP_while:
        case OP_dowhile: {
            ProcessStmt(stmt);
            WhileStmtNode &whileStmtNode = static_cast<WhileStmtNode &>(stmt);
            if (whileStmtNode.GetBody() != nullptr) {
                ProcessBlock(*whileStmtNode.GetBody());
            }
            break;
        }
        case OP_block: {
            BlockNode &block = static_cast<BlockNode &>(stmt);
            for (StmtNode *stmtNode = block.GetFirst(), *next = nullptr; stmtNode != nullptr; stmtNode = next) {
                SetCurrentBlock(block);
                ProcessBlock(*stmtNode);
                next = stmtNode->GetNext();
            }
            break;
        }
        default: {
            ProcessStmt(stmt);
            break;
        }
    }
}

// ========== FuncOptimizeIterator ==========
thread_local FuncOptimizeImpl *FuncOptimizeIterator::phaseImplLocal = nullptr;

FuncOptimizeIterator::FuncOptimizeIterator(const std::string &phaseName, std::unique_ptr<FuncOptimizeImpl> phaseImpl)
    : MplScheduler(phaseName), phaseImpl(std::move(phaseImpl))
{
    char *envStr = getenv("MP_DUMPTIME");
    mplDumpTime = (envStr != nullptr && atoi(envStr) == 1);
}

FuncOptimizeIterator::~FuncOptimizeIterator() = default;

void FuncOptimizeIterator::Run(uint32 threadNum, bool isSeq)
{
    if (threadNum == 1) {
        RunSerial();
    } else {
        RunParallel(threadNum, isSeq);
    }
}

constexpr double kMicroseconds2Milli = 1000.0;
void FuncOptimizeIterator::RunSerial()
{
    MPLTimer timer;
    if (mplDumpTime) {
        timer.Start();
    }

    CHECK_NULL_FATAL(phaseImpl);
    for (MIRFunction *func : phaseImpl->GetMIRModule().GetFunctionList()) {
        auto dumpPhase = [this, func](bool dumpOption, std::string &&s) {
            if (dumpOption) {
                LogInfo::MapleLogger() << ">>>>> Dump " << s << schedulerName << " <<<<<\n";
                func->Dump();
                LogInfo::MapleLogger() << ">>>>> Dump " << s << schedulerName << " end <<<<<\n";
            }
        };
        bool dumpFunc = (Options::dumpFunc == "*" || Options::dumpFunc == func->GetName()) &&
                        (Options::dumpPhase == "*" || Options::dumpPhase == schedulerName);
        dumpPhase(Options::dumpBefore && dumpFunc, "before ");
        phaseImpl->SetDump(dumpFunc);
        phaseImpl->ProcessFunc(func);
        dumpPhase(Options::dumpAfter && dumpFunc, "after ");
    }

    phaseImpl->Finish();

    if (mplDumpTime) {
        timer.Stop();
        INFO(kLncInfo, "FuncOptimizeIterator::RunSerial (%s): %lf ms", schedulerName.c_str(),
             timer.ElapsedMicroseconds() / kMicroseconds2Milli);
    }
}

void FuncOptimizeIterator::RunParallel(uint32 threadNum, bool isSeq)
{
    MPLTimer timer;
    if (mplDumpTime) {
        timer.Start();
    }
    Reset();

    CHECK_NULL_FATAL(phaseImpl);
    for (MIRFunction *func : phaseImpl->GetMIRModule().GetFunctionList()) {
        std::unique_ptr<Task> task = std::make_unique<Task>(*func);
        ASSERT_NOT_NULL(task);
        AddTask(*task.get());
        tasksUniquePtr.emplace_back(std::move(task));
    }

    if (mplDumpTime) {
        timer.Stop();
        INFO(kLncInfo, "FuncOptimizeIterator::RunParallel (%s): AddTask() = %lf ms", schedulerName.c_str(),
             timer.ElapsedMicroseconds() / kMicroseconds2Milli);

        timer.Start();
    }

    int ret = RunTask(threadNum, isSeq);
    CHECK_FATAL(ret == 0, "RunTask failed");
    phaseImpl->Finish();
    Reset();

    if (mplDumpTime) {
        timer.Stop();
        INFO(kLncInfo, "FuncOptimizeIterator::RunParallel (%s): RunTask() = %lf ms", schedulerName.c_str(),
             timer.ElapsedMicroseconds() / kMicroseconds2Milli);
    }
}
}  // namespace maple
