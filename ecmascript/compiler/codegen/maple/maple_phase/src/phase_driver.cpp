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

#include "phase_driver.h"
#include "mpl_timer.h"

namespace maple {
constexpr long kAlternateUnits = 1000;
thread_local PhaseDriverImpl *PhaseDriver::phaseImplLocal = nullptr;
PhaseDriver::PhaseDriver(const std::string &phaseName)
    : MplScheduler(phaseName), module(nullptr), phaseImpl(nullptr), phaseName(phaseName)
{
}

void PhaseDriver::RunAll(MIRModule *currModule, int thread, bool bSeq)
{
    module = currModule;
    phaseImpl = NewPhase();
    CHECK_FATAL(phaseImpl != nullptr, "null ptr check");
    phaseImpl->GlobalInit();
    if (thread == 1) {
        RunSerial();
    } else {
        RunParallel(thread, bSeq);
    }
    delete phaseImpl;
    phaseImpl = nullptr;
}

void PhaseDriver::RunSerial()
{
    phaseImplLocal = NewPhase();
    CHECK_FATAL(phaseImplLocal != nullptr, "null ptr check");
    phaseImplLocal->LocalInit();
    MPLTimer timer;
    if (dumpTime) {
        timer.Start();
    }
    RegisterTasks();
    if (dumpTime) {
        timer.Stop();
        INFO(kLncInfo, "PhaseDriver::RegisterTasks (%s): %lf ms", phaseName.c_str(),
             timer.ElapsedMicroseconds() / kAlternateUnits);
        timer.Start();
    }
    MplTask *task = GetTaskToRun();
    while (task != nullptr) {
        MplTaskParam *paramRun = CallbackGetTaskRunParam();
        (void)task->Run(paramRun);
        MplTaskParam *paramFinish = CallbackGetTaskFinishParam();
        (void)task->Run(paramFinish);
    }
    if (dumpTime) {
        timer.Stop();
        INFO(kLncInfo, "PhaseDriver::RunTask (%s): %lf ms", phaseName.c_str(),
             timer.ElapsedMicroseconds() / kAlternateUnits);
    }
}

void PhaseDriver::RunParallel(int thread, bool bSeq)
{
    MPLTimer timer;
    if (dumpTime) {
        timer.Start();
    }
    RegisterTasks();
    if (dumpTime) {
        timer.Stop();
        INFO(kLncInfo, "PhaseDriver::RegisterTasks (%s): %lf ms", phaseName.c_str(),
             timer.ElapsedMicroseconds() / kAlternateUnits);
    }
    if (dumpTime) {
        timer.Start();
    }
    int ret = RunTask(static_cast<uint32>(thread), bSeq);
    CHECK_FATAL(ret == 0, "RunTask failed");
    if (dumpTime) {
        timer.Stop();
        INFO(kLncInfo, "PhaseDriver::RunTask (%s): %lf ms", phaseName.c_str(),
             timer.ElapsedMicroseconds() / kAlternateUnits);
    }
}

void PhaseDriver::CallbackThreadMainStart()
{
    phaseImplLocal = NewPhase();
    CHECK_FATAL(phaseImplLocal != nullptr, "null ptr check");
    phaseImplLocal->LocalInit();
}

void PhaseDriver::CallbackThreadMainEnd()
{
    delete phaseImplLocal;
    phaseImplLocal = nullptr;
}
}  // namespace maple
