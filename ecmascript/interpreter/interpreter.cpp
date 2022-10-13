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

#include "ecmascript/frames.h"
#include "ecmascript/interpreter/frame_handler.h"
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
EcmaRuntimeCallInfo* EcmaInterpreter::NewRuntimeCallInfo(
    JSThread *thread, JSHandle<JSTaggedValue> func, JSHandle<JSTaggedValue> thisObj, JSHandle<JSTaggedValue> newTarget,
    uint32_t numArgs, bool needCheckStack)
{
    JSTaggedType *prevSp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
    JSTaggedType *newSp = GetInterpreterFrameEnd(thread, prevSp);
    if (needCheckStack && UNLIKELY(thread->DoStackOverflowCheck(newSp - numArgs - NUM_MANDATORY_JSFUNC_ARGS))) {
        return nullptr;
    }

    for (uint32_t i = 0; i < numArgs; i++) {
        *(--newSp) = JSTaggedValue::VALUE_UNDEFINED;
    }
    *(--newSp) = thisObj.GetTaggedType();
    *(--newSp) = newTarget.GetTaggedType();
    *(--newSp) = func.GetTaggedType();
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
}  // namespace panda::ecmascript
