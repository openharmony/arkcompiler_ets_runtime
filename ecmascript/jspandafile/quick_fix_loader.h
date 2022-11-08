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

#ifndef ECMASCRIPT_JSPANDAFILE_QUICK_FIX_LOADER_H
#define ECMASCRIPT_JSPANDAFILE_QUICK_FIX_LOADER_H

#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/c_containers.h"

namespace panda::ecmascript {
using EntityId = panda_file::File::EntityId;
class QuickFixLoader {
public:
    QuickFixLoader() = default;
    ~QuickFixLoader();

    bool LoadPatch(JSThread *thread, const CString &patchFileName, const CString &baseFileName);
    bool LoadPatch(JSThread *thread, const CString &patchFileName, const void *patchBuffer, size_t patchSize,
                   const CString &baseFileName);
    bool UnloadPatch(JSThread *thread, const CString &patchFileName);

private:
    bool LoadPatchInternal(JSThread *thread, const JSPandaFile *baseFile, const JSPandaFile *patchFile);
    bool ReplaceMethod(JSThread *thread,
                       const JSPandaFile *baseFile,
                       const JSPandaFile *patchFile,
                       const JSHandle<ConstantPool> &baseConstpool,
                       const JSHandle<ConstantPool> &patchConstpool);
    void ReplaceMethodInner(JSThread *thread,
                            Method *destMethod,
                            MethodLiteral *srcMethodLiteral,
                            JSTaggedValue srcConstpool);

    void InsertBaseClassMethodInfo(uint32_t constpoolIndex, uint32_t literalIndex, MethodLiteral *base);
    bool HasNormalMethodReplaced(uint32_t index) const;
    bool HasClassMethodReplaced(uint32_t constpoolIndex, uint32_t literalIndex) const;

    CString GetRecordName(const JSPandaFile *jsPandaFile, EntityId methodId);
    CVector<JSHandle<Program>> ParseAllConstpoolWithMerge(JSThread *thread, const JSPandaFile *jsPandaFile);
    void GenerateConstpoolCache(JSThread *thread, const JSPandaFile *jsPandaFile, JSHandle<ConstantPool> constpool);

    bool ExecutePatchMain(JSThread *thread, const JSHandle<Program> &patchProgram, const JSPandaFile *patchFile);
    bool ExecutePatchMain(JSThread *thread, const CVector<JSHandle<Program>> &programs, const JSPandaFile *patchFile);

    CUnorderedSet<CString> ParseStackInfo(const CString &stackInfo);

    void ClearReservedInfo()
    {
        reservedBaseMethodInfo_.clear();
        reservedBaseClassInfo_.clear();
    }
    void ClearPatchInfo(JSThread *thread, const CString &patchFileName) const;

    bool CheckIsInvalidPatch(const JSPandaFile *baseFile, const JSPandaFile *patchFile, EcmaVM *vm) const;
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

    CString baseFileName_;

    // For method unload patch.
    // key: base constpool index, value: base methodLiteral.
    CUnorderedMap<uint32_t, MethodLiteral *> reservedBaseMethodInfo_ {};

    // For class unload patch.
    // key: base constpool index.
    // key: class literal tagged array index, value: base methodLiteral.
    CUnorderedMap<uint32_t, CUnorderedMap<uint32_t, MethodLiteral *>> reservedBaseClassInfo_ {};
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_JSPANDAFILE_QUICK_FIX_LOADER_H
