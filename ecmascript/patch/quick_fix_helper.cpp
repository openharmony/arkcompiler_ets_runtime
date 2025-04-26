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
#include "ecmascript/patch/quick_fix_helper.h"

#include "ecmascript/patch/quick_fix_manager.h"

namespace panda::ecmascript {
JSTaggedValue QuickFixHelper::CreateMainFuncWithPatch(EcmaVM *vm, MethodLiteral *mainMethodLiteral,
                                                      const JSPandaFile *jsPandaFile)
{
    JSThread *thread = vm->GetJSThread();
    if (vm->GetStageOfColdReload() != StageOfColdReload::NOT_COLD_RELOAD) {
        EntityId methodId = mainMethodLiteral->GetMethodId();
        JSTaggedValue patchVal = vm->GetQuickFixManager()->CheckAndGetPatch(thread, jsPandaFile, methodId);
        return patchVal;
    }
    return JSTaggedValue::Hole();
}
}