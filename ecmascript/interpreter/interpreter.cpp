/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/interpreter/interpreter-inl.h"

namespace panda::ecmascript {
// make EcmaRuntimeCallInfo in stack pointer as fallows:
//   +----------------------+   —
//   |        args...       |   ^
//   |----------------------|   |
//   |        numArgs       |   |
//   |----------------------|   |
//   |        this          |   |
//   |----------------------| EcmaRuntimeCallInfo
//   |       newTarget      |   |
//   |----------------------|   |
//   |        func          |   v
//   +----------------------+   —
//   |      base.type       |   ^
//   |----------------------|   |
//   |      base.prev       | InterpretedEntryFrame
//   |----------------------|   |
//   |          pc          |   v
//   +--------------------------+
EcmaRuntimeCallInfo* EcmaInterpreter::NewRuntimeCallInfoBase(
    JSThread *thread, JSTaggedType func, JSTaggedType thisObj, JSTaggedType newTarget,
    uint32_t numArgs, StackCheck needCheckStack)
{
    JSTaggedType *prevSp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
    JSTaggedType *newSp = GetInterpreterFrameEnd(thread, prevSp);
    if (needCheckStack == StackCheck::YES &&
        UNLIKELY(thread->DoStackOverflowCheck(newSp - numArgs - NUM_MANDATORY_JSFUNC_ARGS))) {
        return nullptr;
    }

    for (uint32_t i = 0; i < numArgs; i++) {
        *(--newSp) = JSTaggedValue::VALUE_UNDEFINED;
    }
    *(--newSp) = thisObj;
    *(--newSp) = newTarget;
    *(--newSp) = func;
    *(--newSp) = numArgs + NUM_MANDATORY_JSFUNC_ARGS;
    *(--newSp) = ToUintPtr(thread);
    EcmaRuntimeCallInfo *ecmaRuntimeCallInfo = reinterpret_cast<EcmaRuntimeCallInfo *>(newSp);

    // create entry frame.
    InterpretedEntryFrame *entryState = InterpretedEntryFrame::GetFrameFromSp(newSp);
    entryState->base.type = FrameType::INTERPRETER_ENTRY_FRAME;
    entryState->base.prev = prevSp;
    entryState->pc = nullptr;

    thread->SetCurrentSPFrame(newSp);
    return ecmaRuntimeCallInfo;
}

EcmaRuntimeCallInfo* EcmaInterpreter::NewRuntimeCallInfo(
    JSThread *thread, JSTaggedValue func, JSTaggedValue thisObj, JSTaggedValue newTarget,
    uint32_t numArgs, StackCheck needCheckStack)
{
    return NewRuntimeCallInfoBase(thread, func.GetRawData(), thisObj.GetRawData(), newTarget.GetRawData(),
        numArgs, needCheckStack);
}

EcmaRuntimeCallInfo* EcmaInterpreter::NewRuntimeCallInfo(
    JSThread *thread, JSHandle<JSTaggedValue> func, JSHandle<JSTaggedValue> thisObj,
    JSHandle<JSTaggedValue> newTarget, uint32_t numArgs, StackCheck needCheckStack)
{
    return NewRuntimeCallInfoBase(thread, func.GetTaggedType(), thisObj.GetTaggedType(), newTarget.GetTaggedType(),
        numArgs, needCheckStack);
}

EcmaRuntimeCallInfo* EcmaInterpreter::ReBuildRuntimeCallInfo(JSThread *thread, EcmaRuntimeCallInfo* info,
    int numArgs, StackCheck needCheckStack)
{
    JSTaggedValue func = info->GetFunctionValue();
    JSTaggedValue newTarget = info->GetNewTargetValue();
    JSTaggedValue thisObj = info->GetThisValue();
    JSTaggedType *currentSp = reinterpret_cast<JSTaggedType *>(info);

    InterpretedEntryFrame *currentEntryState = InterpretedEntryFrame::GetFrameFromSp(currentSp);
    JSTaggedType *prevSp = currentEntryState->base.prev;

    int actualArgc = static_cast<int>(info->GetArgsNumber());
    std::vector<JSTaggedType> args(actualArgc);
    for (int i = 0; i < actualArgc; i++) {
        args[i] = info->GetCallArgValue(actualArgc - i - 1).GetRawData();
    }
    currentSp += (info->GetArgsNumber() + NUM_MANDATORY_JSFUNC_ARGS + 2); // 2: include thread_ and numArgs_
    if (needCheckStack == StackCheck::YES &&
        UNLIKELY(thread->DoStackOverflowCheck(currentSp - numArgs - NUM_MANDATORY_JSFUNC_ARGS))) {
        return nullptr;
    }
    ASSERT(numArgs != actualArgc);
    for (int i = 0; i < (numArgs - actualArgc); i++) {
        *(--currentSp) = JSTaggedValue::VALUE_UNDEFINED;
    }
    for (int i = 0; i < actualArgc; i++) {
        *(--currentSp) = args[i];
    }
    *(--currentSp) = thisObj.GetRawData();
    *(--currentSp) = newTarget.GetRawData();
    *(--currentSp) = func.GetRawData();
    *(--currentSp) = numArgs + static_cast<int>(NUM_MANDATORY_JSFUNC_ARGS);
    *(--currentSp) = ToUintPtr(thread);
    EcmaRuntimeCallInfo *ecmaRuntimeCallInfo = reinterpret_cast<EcmaRuntimeCallInfo *>(currentSp);

    InterpretedEntryFrame *entryState = InterpretedEntryFrame::GetFrameFromSp(currentSp);
    entryState->base.type = FrameType::INTERPRETER_ENTRY_FRAME;
    entryState->base.prev = prevSp;
    entryState->pc = nullptr;

    thread->SetCurrentSPFrame(currentSp);
    return ecmaRuntimeCallInfo;
}
}  // namespace panda::ecmascript
