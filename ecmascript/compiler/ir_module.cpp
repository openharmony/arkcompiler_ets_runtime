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

#include "ecmascript/compiler/ir_module.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/debug_info_extractor.h"

namespace panda::ecmascript::kungfu {

std::string IRModule::GetFuncName(const MethodLiteral *methodLiteral,
                                  const JSPandaFile *jsPandaFile)
{
    std::string fileName(jsPandaFile->GetJSPandaFileDesc());
    std::string name = MethodLiteral::GetMethodName(jsPandaFile, methodLiteral->GetMethodId());
    auto methodId = methodLiteral->GetMethodId();
    DebugInfoExtractor *debugExtractor =
        JSPandaFileManager::GetInstance()->GetJSPtExtractor(jsPandaFile);
    std::string url = debugExtractor->GetSourceFile(methodId);
    auto offset = methodLiteral->GetMethodId().GetOffset();
    name += std::string("@") + url + std::string("@") + std::to_string(offset) + std::string("@") + fileName;
    return name;
}

}  // namespace panda::ecmascript::kungfu