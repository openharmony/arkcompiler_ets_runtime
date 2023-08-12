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

#ifndef ECMASCRIPT_DFX_VMSTAT_FCUNTION_CALL_TIMER_H
#define ECMASCRIPT_DFX_VMSTAT_FCUNTION_CALL_TIMER_H

#include "ecmascript/dfx/vmstat/caller_stat.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/method.h"

namespace panda::ecmascript {
class EcmaVM;

// Description:
// FunctionCallTimer is a tool used to count the number of calls, maximum time, total time, and average time of JS&TS
// functions in an application or use case.

// Use:
// If you want to use FunctionCallTimer, open the ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER macro.

// Implementation:
// In AOT, StartCallTimer and EndCallTimer are inserted before and after the function call and generator reentry into
// aot. In ASM interpreter, StartCallTimer is inserted into JSCallDispatch and Resumegenerator instruction. EndCallTimer
// is inserted into the Return, ReturnUndefined and suspend related instruction.
// It should be particularly pointed out that native functions are not counted separately at present considering
// the performance overhead during statistics.
// The specific calculation method is given in the following example.

// T1(StartCallTimer)
// foo() {
//     T2(StartCallTimer)
//     bar();
//     T3(EndCallTimer)
// }
// T4(EndCallTimer)

// bar's self time is (T3 - T2).
// foo's self time is (T2 - T1) + (T4 - T3).

class FunctionCallStat : public PandaRuntimeCallerStat {
public:
    explicit FunctionCallStat(const CString &name, bool isAot) : PandaRuntimeCallerStat(name), isAot_(isAot) {}
    FunctionCallStat() = default;
    ~FunctionCallStat() = default;

    bool IsAot() const
    {
        return isAot_;
    }
private:
    bool isAot_ {false};
};

class FunctionCallTimer {
public:
    FunctionCallTimer() = default;
    ~FunctionCallTimer() = default;
    void StartCount(size_t id, bool isAot);
    void StopCount(Method *method);
    void PrintAllStats();
    CString GetFullName(Method *method);
    void InitialStatAndTimer(Method *method, size_t methodId, bool isAot);
    void ResetStat();

private:
    PandaRuntimeTimer *currentTimer_ = nullptr;
    CMap<size_t, FunctionCallStat> aotCallStat_ {};
    CMap<size_t, FunctionCallStat> intCallStat_ {};
    CMap<size_t, PandaRuntimeTimer> callTimer_ {};
};
}
#endif // ECMASCRIPT_DFX_VMSTAT_FCUNTION_CALL_TIMER_H