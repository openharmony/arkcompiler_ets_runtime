/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/builtins/containers_lightweightmap_stub_builder.h"

#include "ecmascript/compiler/builtins/builtins_stubs.h"
#include "ecmascript/compiler/builtins/containers_stub_builder.h"
#include "ecmascript/compiler/call_stub_builder.h"
#include "ecmascript/compiler/stub_builder-inl.h"

namespace panda::ecmascript::kungfu {
void ContainersLightWeightMapStubBuilder::ForEach(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result,
                                                  Label *exit, Label *slowPath)
{
    ContainersCommonStubBuilder commonBuilder(this, GetCurrentGlobalEnv());
    commonBuilder.ContainersLightWeightCall(glue, thisValue, numArgs, result, exit, slowPath,
                                            ContainersType::LIGHTWEIGHTMAP_FOREACH);
}
}
