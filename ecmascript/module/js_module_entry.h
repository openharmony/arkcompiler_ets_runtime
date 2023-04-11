/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MODULE_JS_MODULE_ENTRY_H
#define ECMASCRIPT_MODULE_JS_MODULE_ENTRY_H

#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/record.h"

namespace panda::ecmascript {
class ImportEntry final : public Record {
public:
    CAST_CHECK(ImportEntry, IsImportEntry);

    static constexpr size_t IMPORT_ENTRY_OFFSET = Record::SIZE;
    ACCESSORS(ModuleRequest, IMPORT_ENTRY_OFFSET, MODULE_REQUEST_OFFSET);
    ACCESSORS(ImportName, MODULE_REQUEST_OFFSET, IMPORT_NAME_OFFSET);
    ACCESSORS(LocalName, IMPORT_NAME_OFFSET, SIZE);

    DECL_DUMP()
    DECL_VISIT_OBJECT(IMPORT_ENTRY_OFFSET, SIZE)
};

class LocalExportEntry : public Record {
public:
    CAST_CHECK(LocalExportEntry, IsLocalExportEntry);

    static constexpr uint32_t LOCAL_DEFAULT_INDEX = 0;
    static constexpr size_t LOCAL_EXPORT_ENTRY_OFFSET = Record::SIZE;
    ACCESSORS(ExportName, LOCAL_EXPORT_ENTRY_OFFSET, LOCAL_NAME_OFFSET);
    ACCESSORS(LocalName, LOCAL_NAME_OFFSET, LOCAL_INDEX_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(LocalIndex, uint32_t, LOCAL_INDEX_OFFSET, LAST_OFFSET);
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_DUMP()
    DECL_VISIT_OBJECT(LOCAL_EXPORT_ENTRY_OFFSET, LOCAL_INDEX_OFFSET)
};

class IndirectExportEntry : public Record {
public:
    CAST_CHECK(IndirectExportEntry, IsIndirectExportEntry);

    static constexpr size_t INDIRECT_EXPORT_ENTRY_OFFSET = Record::SIZE;
    ACCESSORS(ExportName, INDIRECT_EXPORT_ENTRY_OFFSET, MODULE_REQUEST_OFFSET);
    ACCESSORS(ModuleRequest, MODULE_REQUEST_OFFSET, IMPORT_NAME_OFFSET);
    ACCESSORS(ImportName, IMPORT_NAME_OFFSET, SIZE);

    DECL_DUMP()
    DECL_VISIT_OBJECT(INDIRECT_EXPORT_ENTRY_OFFSET, SIZE)
};

class StarExportEntry : public Record {
public:
    CAST_CHECK(StarExportEntry, IsStarExportEntry);

    static constexpr size_t STAR_EXPORT_ENTRY_OFFSET = Record::SIZE;
    ACCESSORS(ModuleRequest, STAR_EXPORT_ENTRY_OFFSET, SIZE);

    DECL_DUMP()
    DECL_VISIT_OBJECT(STAR_EXPORT_ENTRY_OFFSET, SIZE)
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MODULE_JS_MODULE_ENTRY_H