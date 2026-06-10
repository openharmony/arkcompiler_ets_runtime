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
#include "ecmascript/jspandafile/js_pandafile.h"

namespace panda::ecmascript {
using JSRecordInfo = JSPandaFile::JSRecordInfo;

class SendableClassModule {
public:
    static JSHandle<JSTaggedValue> GenerateSendableFuncModule(JSThread *thread, const JSHandle<JSTaggedValue> &module);

    static void CloneEnvOfSModule(JSThread *thread, JSHandle<SourceTextModule> &module,
                                  JSHandle<TaggedArray> &envRec);

    static JSHandle<TaggedArray> CloneModuleEnvironment(JSThread *thread,
                                                        const JSHandle<JSTaggedValue> &moduleEnvironment);

private:
    static JSHandle<JSTaggedValue> CloneRecordIndexBinding(JSThread *thread, JSTaggedValue indexBinding);

    static JSHandle<JSTaggedValue> CloneRecordNameBinding(JSThread *thread, JSTaggedValue binding);
};

class JSSharedModule {
public:
    static JSHandle<TaggedArray> CloneEnvForSModule(JSThread *thread, const JSHandle<SourceTextModule> &module,
        JSHandle<TaggedArray> &envRec);
    static JSHandle<ModuleNamespace> SModuleNamespaceCreate(JSThread *thread, const JSHandle<JSTaggedValue> &module,
                                                            const JSHandle<TaggedArray> &exports);
private:
    static JSHandle<JSTaggedValue> CreateSharedSortedExports(JSThread *thread, const JSHandle<TaggedArray> &exports);
    static JSHandle<TaggedArray> GenerateSharedExports(JSThread *thread, const JSHandle<TaggedArray> &oldExports);
};

class ResolvedRecordIndexBinding final : public Record {
public:
    CAST_CHECK(ResolvedRecordIndexBinding, IsResolvedRecordIndexBinding);

    static constexpr size_t MODULE_RECORD_INDEX_OFFSET = Record::SIZE;
    ACCESSORS_PRIMITIVE_FIELD(ModuleRecord, uintptr_t, MODULE_RECORD_INDEX_OFFSET, ABC_FILE_NAME);
    ACCESSORS_PRIMITIVE_FIELD(AbcFileName, uintptr_t, ABC_FILE_NAME, INDEX_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(Index, int32_t, INDEX_OFFSET, BIT_FIELD_OFFSET);
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, END_OFFSET)
    DEFINE_ALIGN_SIZE(END_OFFSET);

    static constexpr size_t IS_UPDATED_FROM_RESOLVED_RECORD_BINDING_BITS = 1;
    FIRST_BIT_FIELD(BitField, IsUpdatedFromResolvedRecordBinding, bool, IS_UPDATED_FROM_RESOLVED_RECORD_BINDING_BITS)

    inline const CString *GetModuleRecordName() const
    {
        return reinterpret_cast<const CString *>(GetModuleRecord());
    }

    inline CString GetModuleRecordNameString() const
    {
        const CString *recordName = GetModuleRecordName();
        if (recordName == nullptr) {
            return CString();
        }
        return *recordName;
    }

    inline void SetModuleRecordName(const CString *recordName)
    {
        SetModuleRecord(reinterpret_cast<uintptr_t>(recordName));
    }

    inline CString GetAbcFileNameString() const
    {
        const CString *fileName = reinterpret_cast<const CString *>(GetAbcFileName());
        if (fileName == nullptr) {
            return CString();
        }
        return *fileName;
    }

    inline void SetAbcFileNameString(const CString *fileName)
    {
        SetAbcFileName(reinterpret_cast<uintptr_t>(fileName));
    }

    DECL_DUMP()
    DECL_VISIT_PRIMITIVE_OBJECT()
};

class ResolvedRecordBinding final : public Record {
public:
    CAST_CHECK(ResolvedRecordBinding, IsResolvedRecordBinding);

    static constexpr size_t MODULE_RECORD_OFFSET = Record::SIZE;
    ACCESSORS_PRIMITIVE_FIELD(ModuleRecord, uintptr_t, MODULE_RECORD_OFFSET, BINDING_NAME_OFFSET);
    ACCESSORS(BindingName, BINDING_NAME_OFFSET, SIZE);

    inline const CString *GetModuleRecordName() const
    {
        return reinterpret_cast<const CString *>(GetModuleRecord());
    }

    inline CString GetModuleRecordNameString() const
    {
        const CString *recordName = GetModuleRecordName();
        if (recordName == nullptr) {
            return CString();
        }
        return *recordName;
    }

    inline void SetModuleRecordName(const CString *recordName)
    {
        SetModuleRecord(reinterpret_cast<uintptr_t>(recordName));
    }

    DECL_DUMP()
    DECL_VISIT_OBJECT(BINDING_NAME_OFFSET, SIZE)
};

class SharedModuleHelper {
public:
    static JSHandle<JSTaggedValue> ParseSharedModule(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                     const CString &descriptor, const CString &moduleFilename,
                                                     JSRecordInfo *recordInfo);
};

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MODULE_JS_SHARED_MODULE_H
