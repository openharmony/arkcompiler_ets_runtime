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

#ifndef ECMASCRIPT_PATCH_PATCH_LOADER_H
#define ECMASCRIPT_PATCH_PATCH_LOADER_H

#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/napi/include/jsnapi.h"

namespace panda::ecmascript {
using PatchErrorCode = panda::JSNApi::PatchErrorCode;
using JSRecordInfo = JSPandaFile::JSRecordInfo;

struct BaseMethodIndex {
    uint32_t constpoolNum {UINT32_MAX};
    uint32_t constpoolIndex {UINT32_MAX};
    uint32_t literalIndex {UINT32_MAX};
    bool operator < (const BaseMethodIndex &methodIndex) const
    {
        if (constpoolNum < methodIndex.constpoolNum) {
            return true;
        }
        if (constpoolNum == methodIndex.constpoolNum && constpoolIndex < methodIndex.constpoolIndex) {
            return true;
        }
        if (constpoolNum == methodIndex.constpoolNum && constpoolIndex == methodIndex.constpoolIndex) {
            return literalIndex < methodIndex.literalIndex;
        }
        return false;
    }
};

class PatchLoader {
public:
    PatchLoader() = default;
    ~PatchLoader() = default;
    NO_COPY_SEMANTIC(PatchLoader);
    NO_MOVE_SEMANTIC(PatchLoader);

    static PatchErrorCode LoadPatchInternal(JSThread *thread, const JSPandaFile *baseFile, const JSPandaFile *patchFile,
                                            CMap<BaseMethodIndex, MethodLiteral *> &baseMethodInfo);
    static PatchErrorCode UnloadPatchInternal(JSThread *thread, const CString &patchFileName,
                                              const CString &baseFileName,
                                              const CMap<BaseMethodIndex, MethodLiteral *> &baseMethodInfo);

private:
    static bool FindAndReplaceSameMethod(JSThread *thread,
                                         const JSPandaFile *baseFile,
                                         const JSPandaFile *patchFile,
                                         CMap<BaseMethodIndex, MethodLiteral *> &baseMethodInfo);
    static void ReplaceMethod(JSThread *thread,
                              Method *destMethod,
                              MethodLiteral *srcMethodLiteral,
                              JSTaggedValue srcConstpool);
    static void InsertMethodInfo(BaseMethodIndex methodIndex, MethodLiteral *base,
                                 CMap<BaseMethodIndex, MethodLiteral *> &baseMethodInfo);

    static void ParseConstpoolWithMerge(JSThread *thread, const JSPandaFile *jsPandaFile,
                                        const CUnorderedMap<CString, JSRecordInfo> &patchRecordInfos,
                                        const JSPandaFile *baseFile = nullptr);
    static void GenerateConstpoolCache(JSThread *thread, const JSPandaFile *jsPandaFile,
                                       const CUnorderedMap<CString, JSRecordInfo> &patchRecordInfos,
                                       const JSPandaFile *baseFile = nullptr);
    static void FillExternalMethod(JSThread *thread, const JSPandaFile *patchFile, const JSPandaFile *baseFile,
                                   JSHandle<ConstantPool> patchConstpool, uint32_t id);

    static bool ExecutePatchMain(JSThread *thread, const JSPandaFile *patchFile, const JSPandaFile *baseFile,
                                 const CMap<BaseMethodIndex, MethodLiteral *> &baseMethodInfo);

    static void ClearPatchInfo(JSThread *thread, const CString &patchFileName);

    static bool CheckIsInvalidPatch(const JSPandaFile *baseFile, const JSPandaFile *patchFile, EcmaVM *vm);
    // Check module mismatch
    static bool CheckIsModuleMismatch(JSThread *thread, JSHandle<SourceTextModule> patchModule,
                                      JSHandle<SourceTextModule> baseModule);
    static bool CheckImportEntriesMismatch(JSThread *thread, JSTaggedValue patch, JSTaggedValue base);
    static bool CheckLocalExportEntriesMismatch(JSThread *thread, JSTaggedValue patch, JSTaggedValue base);
    static bool CheckIndirectExportEntriesMismatch(JSThread *thread, JSTaggedValue patch, JSTaggedValue base);
    static bool CheckStarExportEntriesMismatch(JSThread *thread, JSTaggedValue patch, JSTaggedValue base);
    static bool CheckImportEntryMismatch(ImportEntry *patch, ImportEntry *base);
    static bool CheckLocalExportEntryMismatch(LocalExportEntry *patch, LocalExportEntry *base);
    static bool CheckIndirectExportEntryMismatch(IndirectExportEntry *patch, IndirectExportEntry *base);
    static bool CheckStarExportEntryMismatch(StarExportEntry *patch, StarExportEntry *base);
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_PATCH_PATCH_LOADER_H
