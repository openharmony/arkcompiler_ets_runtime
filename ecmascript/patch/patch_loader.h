/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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
#include "ecmascript/method.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/napi/include/jsnapi.h"

namespace panda::ecmascript {
using PatchErrorCode = panda::JSNApi::PatchErrorCode;
using JSRecordInfo = JSPandaFile::JSRecordInfo;
using LiteralDataAccessor = panda_file::LiteralDataAccessor;
using LiteralValue = panda_file::LiteralDataAccessor::LiteralValue;
using LiteralTag = panda_file::LiteralTag;
class ConstantPool;
class JSThread;

struct BaseMethodIndex {
    uint32_t constpoolNum {UINT32_MAX};
    uint32_t constpoolIndex {UINT32_MAX};
    uint32_t literalIndex {UINT32_MAX};
    struct Hash {
        std::size_t operator()(const BaseMethodIndex &baseMethodIndex) const
        {
            return std::hash<uint32_t>{}(baseMethodIndex.constpoolNum) ^ (std::hash<uint32_t>{}(
                baseMethodIndex.constpoolIndex) << 1) ^ std::hash<uint32_t>{}(baseMethodIndex.literalIndex);
        }
    };

    bool operator==(const BaseMethodIndex &baseMethodIndex) const
    {
        return constpoolNum == baseMethodIndex.constpoolNum && constpoolIndex == baseMethodIndex.constpoolIndex &&
            literalIndex == baseMethodIndex.literalIndex;
    }
};

struct PatchMethodIndex {
    CString recordName;
    CString className;
    CString methodName;
    struct Hash {
        std::size_t operator()(const PatchMethodIndex &patchMethodIndex) const
        {
            return std::hash<CString>{}(patchMethodIndex.recordName) ^
                std::hash<CString>{}(patchMethodIndex.className) ^ std::hash<CString>{}(patchMethodIndex.methodName);
        }
    };

    bool operator==(const PatchMethodIndex &patchMethodIndex) const
    {
        return recordName == patchMethodIndex.recordName && className == patchMethodIndex.className &&
            methodName == patchMethodIndex.methodName;
    }
};

struct ReplacedMethod {
    EntityId methodId;
    CString fileName;
    struct Hash {
        std::size_t operator()(const ReplacedMethod &replacedMethod) const
        {
            return std::hash<EntityId>{}(replacedMethod.methodId) ^ std::hash<CString>{}(replacedMethod.fileName);
        }
    };

    bool operator==(const ReplacedMethod &replacedMethod) const
    {
        return methodId == replacedMethod.methodId && fileName == replacedMethod.fileName;
    }
};


struct PatchInfo {
    // patch file name.
    CString patchFileName;
    // patch methodLiterals for load patch, <recordName, <methodName, MethodLiteral>>
    CUnorderedMap<PatchMethodIndex, MethodLiteral*, PatchMethodIndex::Hash> patchMethodLiterals;
    // base method info for unload patch, <BaseMethodIndex, base MethodLiteral>
    CUnorderedMap<BaseMethodIndex, MethodLiteral *, BaseMethodIndex::Hash> baseMethodInfo;
    // save base constpool in global for avoid gc.
    CVector<JSHandle<JSTaggedValue>> baseConstpools;
    // patch replaced recordNames.
    CUnorderedSet<CString> replacedRecordNames;
    // patch replaced methods.
    CUnorderedMap<ReplacedMethod, CString, ReplacedMethod::Hash> replacedPatchMethods;
};

enum class StageOfHotReload : int32_t {
    BEGIN_EXECUTE_PATCHMAIN = -1, // -1: For intercepting Evaluate()
    INITIALIZE_STAGE_OF_HOTRELOAD, // 0 : initialize stageOfHotreload_ in ecma_context.h
    LOAD_END_EXECUTE_PATCHMAIN, // 1: for Interceptint get module var
    UNLOAD_END_EXECUTE_PATCHMAIN // 2 :for execute abc normally
};

enum class StageOfColdReload : int32_t {
    NOT_COLD_RELOAD,
    IS_COLD_RELOAD
};

class PatchLoader {
public:
    PatchLoader() = default;
    ~PatchLoader() = default;
    NO_COPY_SEMANTIC(PatchLoader);
    NO_MOVE_SEMANTIC(PatchLoader);

    static PatchErrorCode LoadPatchInternal(JSThread *thread, const JSPandaFile *baseFile,
                                            const JSPandaFile *patchFile, PatchInfo &patchInfo,
                                            const CMap<uint32_t, CString> &baseClassInfo);
    static PatchErrorCode UnloadPatchInternal(JSThread *thread, const CString &patchFileName,
                                              const CString &baseFileName, PatchInfo &patchInfo);

    static MethodLiteral *FindSameMethod(PatchInfo &patchInfo, const JSPandaFile *baseFile,
                                         EntityId baseMethodId, const CMap<uint32_t, CString> &baseClassInfo);
    static void ExecuteFuncOrPatchMain(
        JSThread *thread, const JSPandaFile *jsPandaFile, const PatchInfo &patchInfo, bool loadPatch = true);
    static CMap<uint32_t, CString> CollectClassInfo(const JSPandaFile *jsPandaFile);
    static void UpdateModuleForColdPatch(
        JSThread *thread, EntityId methodId, CString &recordName, bool hasModule = true);
    static void UpdateJSFunction(JSThread *thread, PatchInfo &patchInfo);

private:
    static PatchInfo GeneratePatchInfo(const JSPandaFile *patchFile);
    static CString GetRealName(const JSPandaFile *jsPandaFile, EntityId entityId, CString &className);
    static void FindAndReplaceSameMethod(JSThread *thread,
                                         const JSPandaFile *baseFile,
                                         const JSPandaFile *patchFile,
                                         PatchInfo &patchInfo,
                                         const CMap<uint32_t, CString> &baseClassInfo);
    static void SaveBaseMethodInfo(PatchInfo &patchInfo, const JSPandaFile *baseFile,
                                   EntityId baseMethodId, const BaseMethodIndex &indexs);
    static void ReplaceMethod(JSThread *thread,
                              Method *destMethod,
                              MethodLiteral *srcMethodLiteral,
                              JSTaggedValue srcConstpool);

    static void ClearPatchInfo(JSThread *thread, const CString &patchFileName);

    static Method *GetPatchMethod(JSThread *thread,
        const BaseMethodIndex &methodIndex, const JSTaggedValue baseConstpool);
    static void FindAndReplaceClassLiteral(JSThread *thread, const JSPandaFile *baseFile,
                                           const JSPandaFile *patchFile, JSTaggedValue constpoolValue,
                                           PatchInfo &patchInfo, uint32_t constpoolIndex,
                                           uint32_t constpoolNum, const CMap<uint32_t, CString> &baseClassInfo);
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_PATCH_PATCH_LOADER_H
