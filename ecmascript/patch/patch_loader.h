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

struct PatchInfo {
    // patch file name.
    CString patchFileName;
    // patch methodLiterals for load patch, <recordName, <methodName, MethodLiteral>>
    CMap<CString, CMap<CString, MethodLiteral*>> patchMethodLiterals;
    // base method info for unload patch, <BaseMethodIndex, base MethodLiteral>
    CMap<BaseMethodIndex, MethodLiteral *> baseMethodInfo;
    // save base constpool in global for avoid gc.
    CVector<JSHandle<JSTaggedValue>> baseConstpools;
};
class PatchLoader {
public:
    PatchLoader() = default;
    ~PatchLoader() = default;
    NO_COPY_SEMANTIC(PatchLoader);
    NO_MOVE_SEMANTIC(PatchLoader);

    static PatchErrorCode LoadPatchInternal(JSThread *thread, const JSPandaFile *baseFile,
                                            const JSPandaFile *patchFile, PatchInfo &patchInfo);
    static PatchErrorCode UnloadPatchInternal(JSThread *thread, const CString &patchFileName,
                                              const CString &baseFileName, PatchInfo &patchInfo);

    static MethodLiteral *FindSameMethod(PatchInfo &patchInfo, const JSPandaFile *baseFile, EntityId baseMethodId);

private:
    static PatchInfo GeneratePatchInfo(const JSPandaFile *patchFile);
    static void FindAndReplaceSameMethod(JSThread *thread,
                                         const JSPandaFile *baseFile,
                                         const JSPandaFile *patchFile,
                                         PatchInfo &patchInfo);
    static void SaveBaseMethodInfo(PatchInfo &patchInfo, const JSPandaFile *baseFile,
                                   EntityId baseMethodId, const BaseMethodIndex &indexs);
    static void ReplaceMethod(JSThread *thread,
                              Method *destMethod,
                              MethodLiteral *srcMethodLiteral,
                              JSTaggedValue srcConstpool);

    static bool ExecutePatchMain(JSThread *thread, const JSPandaFile *patchFile, const JSPandaFile *baseFile,
                                 PatchInfo &patchInfo);

    static void ClearPatchInfo(JSThread *thread, const CString &patchFileName);
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_PATCH_PATCH_LOADER_H
