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

#include "ecmascript/compiler/ir_module.h"

namespace panda::ecmascript::kungfu {

std::string IRModule::GetFuncName(const MethodLiteral *methodLiteral,
                                  const JSPandaFile *jsPandaFile)
{
    auto offset = methodLiteral->GetMethodId().GetOffset();
    std::string fileName = jsPandaFile->GetFileName();
    std::string name = MethodLiteral::GetMethodName(jsPandaFile, methodLiteral->GetMethodId());
    name += std::string("@") + std::to_string(offset) + std::string("@") + fileName;
    return name;
}

}  // namespace panda::ecmascript::kungfu