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

#ifndef ECMASCRIPT_MODULE_JS_SHARED_MODULE_H
#define ECMASCRIPT_MODULE_JS_SHARED_MODULE_H

#include "ecmascript/module/js_module_source_text.h"

namespace panda::ecmascript {
class SendableClassModule {
public:
    static JSHandle<JSTaggedValue> GenerateSendableFuncModule(JSThread *thread, const JSHandle<JSTaggedValue> &module);

    static void CloneEnvOfSModule(JSThread *thread, JSHandle<SourceTextModule> &module,
                                  JSHandle<TaggedArray> &envRec);

    static JSHandle<TaggedArray> CloneModuleEnvironment(JSThread *thread,
                                                        const JSHandle<JSTaggedValue> &moduleEnvironment);

private:
    static JSHandle<JSTaggedValue> CloneRecordBinding(JSThread *thread, JSTaggedValue indexBinding);
};

class JSSharedModule {
public:
    static JSHandle<TaggedArray> CloneEnvForSModule(JSThread *thread, const JSHandle<SourceTextModule> &module,
        JSHandle<TaggedArray> &envRec);

private:
    static JSHandle<JSTaggedValue> CloneRecordBinding(JSThread *thread, JSTaggedValue indexBinding);
};

class ResolvedRecordBinding final : public Record {
public:
    CAST_CHECK(ResolvedRecordBinding, IsResolvedRecordBinding);

    static constexpr size_t MODULE_RECORD_OFFSET = Record::SIZE;
    ACCESSORS(ModuleRecord, MODULE_RECORD_OFFSET, INDEX_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(Index, int32_t, INDEX_OFFSET, END_OFFSET);
    DEFINE_ALIGN_SIZE(END_OFFSET);

    DECL_DUMP()
    DECL_VISIT_OBJECT(MODULE_RECORD_OFFSET, INDEX_OFFSET)
};

class SharedModuleHelper {
public:
    static JSHandle<JSTaggedValue> ParseSharedModule(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                     const CString &descriptor, const CString &moduleFilename);
};

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MODULE_JS_SHARED_MODULE_H
