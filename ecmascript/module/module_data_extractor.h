/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MODULE_MODULE_DATA_EXTRACTOR_H
#define ECMASCRIPT_MODULE_MODULE_DATA_EXTRACTOR_H

#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/jspandafile/js_pandafile.h"

namespace panda::ecmascript {
using EntityId = panda_file::File::EntityId;

class ModuleDataExtractor {
public:
    ModuleDataExtractor() = default;
    virtual ~ModuleDataExtractor() = default;

    DEFAULT_NOEXCEPT_MOVE_SEMANTIC(ModuleDataExtractor);
    DEFAULT_COPY_SEMANTIC(ModuleDataExtractor);

    static void ExtractModuleDatas(JSThread *thread, const JSPandaFile *jsPandaFile,
                                   panda_file::File::EntityId moduleId,
                                   JSHandle<SourceTextModule> &moduleRecord,
                                   [[maybe_unused]]JSPandaFile::JSRecordInfo *recordInfo);
    static JSHandle<JSTaggedValue> ParseModule(JSThread *thread, const JSPandaFile *jsPandaFile,
                                               const CString &descriptor, const CString &moduleFilename,
                                               JSPandaFile::JSRecordInfo *recordInfo);
    static JSHandle<JSTaggedValue> ParseCjsModule(JSThread *thread, const JSPandaFile *jsPandaFile);
    static JSHandle<JSTaggedValue> ParseJsonModule(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                   const CString &moduleFilename, const CString &recordName);
    static JSHandle<JSTaggedValue> ParseNativeModule(JSThread *thread, const CString &moduleRequestName,
                                                     const CString &baseFileName, ModuleTypes moduleType);
    static JSTaggedValue JsonParse(JSThread *thread, const JSPandaFile *jsPandaFile, CString entryPoint);

private:
    static bool *ModuleLazyImportFlagAccessor(const JSPandaFile *pandaFile, EntityId lazyImportFlagId);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MODULE_MODULE_DATA_EXTRACTOR_H
