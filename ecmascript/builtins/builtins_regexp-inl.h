/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_REGEXP_INL_H
#define ECMASCRIPT_BUILTINS_BUILTINS_REGEXP_INL_H

#include "ecmascript/ecma_context.h"

namespace panda::ecmascript::builtins {

/* static */
template <int N>
JSTaggedValue RegExpGlobalResult::GetCapture(JSThread *thread)
{
    JSHandle<builtins::RegExpGlobalResult> globalTable(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    JSTaggedValue res = globalTable->Get(CAPTURE_START_INDEX + N - 1);
    int captureNum = globalTable->GetTotalCaptureCounts().GetInt();
    if (res.IsHole() && (N < captureNum)) {
        int startIndex = globalTable->GetStartOfCaptureIndex(N).GetInt();
        int endIndex = globalTable->GetEndOfCaptureIndex(N).GetInt();
        int len = endIndex - startIndex;
        if (len < 0) {
            res = JSTaggedValue::Undefined();
        } else {
            res = JSTaggedValue(EcmaStringAccessor::FastSubString(thread->GetEcmaVM(),
                JSHandle<EcmaString>(thread, EcmaString::Cast(globalTable->GetInputString())),
                static_cast<uint32_t>(startIndex), static_cast<uint32_t>(len)));
        }
        globalTable->Set(thread, CAPTURE_START_INDEX + N - 1, res);
    } else if (res.IsHole()) {
        res = thread->GetEcmaVM()->GetFactory()->GetEmptyString().GetTaggedValue();
        globalTable->Set(thread, CAPTURE_START_INDEX + N - 1, res);
    }
    return res;
}

}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_REGEXP_INL_H
