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

#ifndef ECMASCRIPT_MODULE_TOOLS_H
#define ECMASCRIPT_MODULE_TOOLS_H

#include "ecmascript/module/js_module_source_text.h"

namespace panda::ecmascript {

class ModuleTools {
public:
    static JSTaggedValue GetModuleValueFromIndexBindingForLog(JSThread *thread,
        JSHandle<SourceTextModule> module, JSTaggedValue resolvedBinding, int32_t index);

    static JSTaggedValue GetModuleValueFromRecordBindingForLog(JSThread *thread,
        JSHandle<SourceTextModule> module, JSTaggedValue resolvedBinding, int32_t index);

    static JSTaggedValue ProcessModuleLoadInfo(JSThread *thread, JSHandle<SourceTextModule> currentModule,
                                               JSTaggedValue resolvedBinding, int32_t index);

    static JSTaggedValue ProcessModuleNameSpaceLoadInfo(JSThread *thread,
                                                        JSHandle<SourceTextModule> currentModule,
                                                        JSHandle<SourceTextModule> requiredModule);
};

class ModuleTraceScope {
public:
    ModuleTraceScope(JSThread *thread, [[maybe_unused]]const CString traceInfo)
        : enableESMTrace_(thread->GetEcmaVM()->GetJSOptions().EnableESMTrace())
    {
        if (enableESMTrace_) {
            ECMA_BYTRACE_START_TRACE(HITRACE_TAG_ARK, traceInfo.c_str());
        }
    }

    ~ModuleTraceScope()
    {
        if (enableESMTrace_) {
            ECMA_BYTRACE_FINISH_TRACE(HITRACE_TAG_ARK);
        }
    }
private:
    bool enableESMTrace_ {false};
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_MODULE_MODULE_TOOLS_H
