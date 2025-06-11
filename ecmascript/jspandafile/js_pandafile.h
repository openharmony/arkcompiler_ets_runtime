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

#ifndef ECMASCRIPT_JSPANDAFILE_JS_PANDAFILE_H
#define ECMASCRIPT_JSPANDAFILE_JS_PANDAFILE_H

#include "common_components/taskpool/task.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/common.h"
#include "ecmascript/jspandafile/constpool_value.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/platform/mutex.h"

#include "libpandafile/file-inl.h"
#include "libpandafile/file_items.h"
#include "libpandafile/literal_data_accessor.h"

namespace panda {
namespace ecmascript {
class EcmaVM;

enum class CreateMode : uint8_t {
    RUNTIME = 0,
    DFX,
};

class JSPandaFile {
public:
    struct JSRecordInfo {
        uint32_t mainMethodIndex {0};
        bool isCjs {false};
        bool isJson {false};
        bool isSharedModule {false};
        int jsonStringId {-1};
        CUnorderedSet<const EcmaVM *> vmListOfParsedConstPool;
        int moduleRecordIdx {-1};
        bool hasTopLevelAwait {false};
        CUnorderedMap<uint32_t, uint64_t> constpoolMap;
        uint32_t lazyImportIdx {0};
        uint32_t classId {CLASSID_OFFSET_NOT_FOUND};
        CString npmPackageName;

        void SetParsedConstpoolVM(const EcmaVM *vm)
        {
            vmListOfParsedConstPool.insert(vm);
        }

        bool IsParsedConstpoolOfCurrentVM(const EcmaVM *vm) const
        {
            auto iter = vmListOfParsedConstPool.find(vm);
            if (iter != vmListOfParsedConstPool.end()) {
                return true;
            }
            return false;
        }
    };
    static constexpr char ENTRY_FUNCTION_NAME[] = "func_main_0";
    static constexpr char ENTRY_MAIN_FUNCTION[] = "_GLOBAL::func_main_0";
    static constexpr char PATCH_MAIN_FUNCTION[] = "_GLOBAL::patch_main_0";
    static constexpr char PATCH_FUNCTION_NAME_0[] = "patch_main_0";
    static constexpr char PATCH_FUNCTION_NAME_1[] = "patch_main_1";

    static constexpr char MODULE_CLASS[] = "L_ESModuleRecord;";
    static constexpr char COMMONJS_CLASS[] = "L_CommonJsRecord;";
    static constexpr char HASTLA_CLASS[] = "L_HasTopLevelAwait;";

    static constexpr std::string_view IS_COMMON_JS = "isCommonjs";
    static constexpr std::string_view IS_JSON_CONTENT = "jsonFileContent";
    static constexpr std::string_view MODULE_RECORD_IDX = "moduleRecordIdx";
    static constexpr std::string_view IS_SHARED_MODULE = "isSharedModule";
    static constexpr std::string_view HAS_TOP_LEVEL_AWAIT = "hasTopLevelAwait";
    static constexpr std::string_view LAZY_IMPORT = "moduleRequestPhaseIdx";
    static constexpr std::string_view PACKAGE_NAME = "pkgName@";
    static constexpr char MERGE_ABC_NAME[] = "modules.abc";
    static constexpr char NPM_PATH_SEGMENT[] = "node_modules";
    static constexpr char PACKAGE_PATH_SEGMENT[] = "pkg_modules";
    static constexpr char BUNDLE_INSTALL_PATH[] = "/data/storage/el1/bundle/";

    static constexpr std::string_view HAP_SUFFIX = ".hap";
    static constexpr int PACKAGE_NAME_LEN = 8;
    static constexpr int TYPE_SUMMARY_OFFSET_NOT_FOUND = 0;
    static constexpr int CLASSID_OFFSET_NOT_FOUND = 0;
    static constexpr int32_t PF_OFFSET = 0;
    static constexpr uint32_t ASYN_TRANSLATE_CLSSS_COUNT = 128;
    static constexpr uint32_t ASYN_TRANSLATE_CLSSS_MIN_COUNT = 2;
    static constexpr uint32_t DEFAULT_MAIN_METHOD_INDEX = 0;

    JSPandaFile(const panda_file::File *pf, const CString &descriptor, CreateMode state = CreateMode::RUNTIME);
    ~JSPandaFile();

    class TranslateClassesTask : public common::Task {
    public:
        TranslateClassesTask(int32_t id, JSThread *thread, JSPandaFile *jsPandaFile,
            const std::shared_ptr<CString> &methodNamePtr)
            : common::Task(id), thread_(thread), jsPandaFile_(jsPandaFile), methodNamePtr_(methodNamePtr) {};
        ~TranslateClassesTask() override = default;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(TranslateClassesTask);
        NO_MOVE_SEMANTIC(TranslateClassesTask);

    private:
        JSThread *thread_ {nullptr};
        JSPandaFile *jsPandaFile_ {nullptr};
        std::shared_ptr<CString> methodNamePtr_;
    };

    inline const CString &GetJSPandaFileDesc() const
    {
        return desc_;
    }

    CString PUBLIC_API GetNormalizedFileDesc() const;

    void SetHapPath(const CString &hapPath)
    {
        hapPath_ = hapPath;
    }

    const CString &GetJSPandaFileHapPath() const
    {
        return hapPath_;
    }

    static CString PUBLIC_API GetNormalizedFileDesc(const CString &desc);

    inline uint32_t GetChecksum() const
    {
        return checksum_;
    }

    inline const panda_file::File *GetPandaFile() const
    {
        return pf_;
    }

    std::pair<std::string_view, bool> GetMethodName(EntityId methodId);
    std::pair<std::string_view, bool> GetCpuProfilerMethodName(EntityId methodId) const;
    CString GetRecordName(EntityId methodId);
    CString PUBLIC_API GetRecordNameWithBundlePack(EntityId methodIdx);

    inline MethodLiteral* GetMethodLiterals() const
    {
        return methodLiterals_;
    }

    inline void SetMethodLiteralToMap(MethodLiteral *methodLiteral)
    {
        ASSERT(methodLiteral != nullptr);
        methodLiteralMap_.try_emplace(methodLiteral->GetMethodId().GetOffset(), methodLiteral);
    }

    inline const std::unordered_map<uint32_t, MethodLiteral *> &GetMethodLiteralMap() const
    {
        return methodLiteralMap_;
    }

    uint32_t GetNumMethods() const
    {
        return numMethods_;
    }

    uint32_t GetConstpoolIndex() const
    {
        return constpoolIndex_;
    }

    uint32_t GetMainMethodIndex(const CString &recordName = ENTRY_FUNCTION_NAME, bool isNewVersion = false) const
    {
        if (IsBundlePack()) {
            return jsRecordInfo_.begin()->second->mainMethodIndex;
        }
        auto info = jsRecordInfo_.find(recordName);
        if (info != jsRecordInfo_.end()) {
            return info->second->mainMethodIndex;
        }

        if (isNewVersion) {
            for (const auto &recordInfo : jsRecordInfo_) {
                LOG_ECMA(ERROR) << "All current record info: " << recordInfo.first;
            }
            LOG_ECMA(FATAL) << "can not get main method index: " << recordName;
            UNREACHABLE();
        }

        LOG_ECMA(ERROR) << "can not get main method index: " << recordName;
        return 0;
    }

    const CUnorderedMap<uint32_t, uint64_t> *GetConstpoolMapByReocrd(const CString &recordName) const
    {
        auto info = jsRecordInfo_.find(recordName);
        if (info != jsRecordInfo_.end()) {
            return &info->second->constpoolMap;
        }
        LOG_FULL(FATAL) << "find entryPoint failed: " << recordName;
        UNREACHABLE();
    }

    const CUnorderedMap<uint32_t, uint64_t> &GetConstpoolMap() const
    {
        return constpoolMap_;
    }

    uint32_t PUBLIC_API GetOrInsertConstantPool(ConstPoolType type, uint32_t offset,
                                                const CUnorderedMap<uint32_t, uint64_t> *constpoolMap = nullptr);

    void UpdateMainMethodIndex(uint32_t mainMethodIndex, const CString &recordName = ENTRY_FUNCTION_NAME)
    {
        LockHolder lock(jsRecordInfoMutex_);
        if (IsBundlePack()) {
            jsRecordInfo_.begin()->second->mainMethodIndex = mainMethodIndex;
        } else {
            auto info = jsRecordInfo_.find(recordName);
            if (info != jsRecordInfo_.end()) {
                info->second->mainMethodIndex = mainMethodIndex;
            }
        }
    }

    inline PUBLIC_API MethodLiteral *FindMethodLiteral(uint32_t offset) const
    {
        auto iter = methodLiteralMap_.find(offset);
        if (iter == methodLiteralMap_.end()) {
            return nullptr;
        }
        return iter->second;
    }

    inline int GetModuleRecordIdx(const CString &recordName = ENTRY_FUNCTION_NAME) const
    {
        if (IsBundlePack()) {
            return jsRecordInfo_.begin()->second->moduleRecordIdx;
        }
        auto info = jsRecordInfo_.find(recordName);
        if (info != jsRecordInfo_.end()) {
            return info->second->moduleRecordIdx;
        }
        // The array subscript will not have a negative number, and returning -1 means the search failed
        return -1;
    }

    int GetHasTopLevelAwait(const CString &recordName = ENTRY_FUNCTION_NAME) const
    {
        if (IsBundlePack()) {
            return jsRecordInfo_.begin()->second->hasTopLevelAwait;
        }
        auto info = jsRecordInfo_.find(recordName);
        if (info != jsRecordInfo_.end()) {
            return info->second->hasTopLevelAwait;
        }
        return false;
    }

    inline Span<const uint32_t> GetClasses() const
    {
        return pf_->GetClasses();
    }

    inline bool IsExternal(panda_file::File::EntityId id) const
    {
        return pf_->IsExternal(id);
    }

    inline bool Contain(uint8_t *data) const
    {
        uintptr_t header = ToUintPtr(GetHeader());
        uintptr_t dataPointer = ToUintPtr(data);
        if (header < dataPointer && dataPointer < (header + GetFileSize())) {
            return true;
        }
        return false;
    }

    inline panda_file::File::StringData GetStringData(panda_file::File::EntityId id) const
    {
        return pf_->GetStringData(id);
    }

    panda_file::File::EntityId ResolveMethodIndex(panda_file::File::EntityId id, uint16_t idx) const
    {
        return pf_->ResolveMethodIndex(id, idx);
    }

    panda_file::LiteralDataAccessor GetLiteralDataAccessor() const
    {
        EntityId literalArraysId = pf_->GetLiteralArraysId();
        panda_file::LiteralDataAccessor lda(*pf_, literalArraysId);
        return lda;
    }

    uint32_t GetConstpoolNum() const
    {
        return pf_->GetHeader()->num_indexes;
    }

    Span<const panda_file::File::EntityId> GetMethodIndex(const panda_file::File::IndexHeader *indexHeader) const
    {
        return pf_->GetMethodIndex(indexHeader);
    }

    const void *GetHeader() const
    {
        return static_cast<const void *>(pf_->GetHeader());
    }

    uint32_t GetFileSize() const
    {
        return pf_->GetHeader()->file_size;
    }

    inline JSRecordInfo* CheckAndGetRecordInfo(const CString &recordName) const
    {
        if (UNLIKELY(IsBundlePack())) {
            return jsRecordInfo_.begin()->second;
        }

        auto info = jsRecordInfo_.find(recordName);
        if (info != jsRecordInfo_.end()) {
            return info->second;
        }

        return nullptr;
    }

    CString GetJsonStringId(const JSRecordInfo &jsRecordInfo) const;

    inline bool PUBLIC_API IsModule(const JSRecordInfo *jsRecordInfo) const
    {
        return jsRecordInfo->moduleRecordIdx != -1;
    }

    inline bool IsCjs(const JSRecordInfo *jsRecordInfo) const
    {
        return jsRecordInfo->isCjs;
    }

    inline bool IsJson(const JSRecordInfo *jsRecordInfo) const
    {
        return jsRecordInfo->isJson;
    }

    inline bool IsSharedModule(const JSRecordInfo *jsRecordInfo) const
    {
        return jsRecordInfo->isSharedModule;
    }

    inline bool IsBundlePack() const
    {
        return isBundlePack_;
    }

    bool IsLoadedAOT() const
    {
        return (GetAOTFileInfoIndex() != INVALID_INDEX);
    }

    uint32_t GetFileUniqId() const
    {
        return static_cast<uint32_t>(GetPandaFile()->GetUniqId());
    }

    inline bool IsNewVersion() const
    {
        return isNewVersion_;
    }

    inline bool HasRecord(const CString &recordName) const
    {
        return jsRecordInfo_.find(recordName) != jsRecordInfo_.end();
    }

    JSRecordInfo &FindRecordInfo(const CString &recordName)
    {
        auto info = jsRecordInfo_.find(recordName);
        // check entry name, fix framework abc find recordName fail bug
        if (recordName == "_GLOBAL") {
            info = jsRecordInfo_.find(ENTRY_FUNCTION_NAME);
        }
        if (info == jsRecordInfo_.end()) {
            LOG_FULL(FATAL) << "find recordName failed: " << recordName;
            UNREACHABLE();
        }
        return *(info->second);
    }

    // note : it only uses in TDD
    void InsertJSRecordInfo(const CString &recordName)
    {
        JSRecordInfo* info = new JSRecordInfo();
        jsRecordInfo_.insert({recordName, info});
    }

    // note : it only uses in TDD
    void InsertNpmEntries(const CString &recordName, const CString &fieldName)
    {
        npmEntries_.insert({recordName, fieldName});
    }

    const CUnorderedMap<CString, JSRecordInfo*> &GetJSRecordInfo() const
    {
        return jsRecordInfo_;
    }

    static CString ParseEntryPoint(const CString &desc)
    {
        return desc.substr(1, desc.size() - 2); // 2 : skip symbol "L" and ";"
    }

    void CheckIsBundlePack();
    void CheckIsRecordWithBundleName(const CString &entry);
    bool IsRecordWithBundleName() const
    {
        return isRecordWithBundleName_;
    }
    CString GetEntryPoint(const CString &recordName) const;
    CString GetRecordName(const CString &entryPoint) const;
    bool FindOhmUrlInPF(const CString &recordName, CString &entryPoint) const;
    uint32_t GetAOTFileInfoIndex() const
    {
        return anFileInfoIndex_;
    }

    void SetAOTFileInfoIndex(uint32_t index)
    {
        if (IsLoadedAOT()) {
            LOG_ECMA(ERROR) << "Set Aot file info index failed. desc: " << GetJSPandaFileDesc()
                            << ", anFileIndex: " << anFileInfoIndex_ << "  vs " << index;
            return;
        }
        anFileInfoIndex_ = index;
    }

    static bool IsEntryOrPatch(const CString &name)
    {
        return (name == PATCH_FUNCTION_NAME_0) || (name == ENTRY_FUNCTION_NAME);
    }

    inline void DeleteParsedConstpoolVM(const EcmaVM *vm)
    {
        for (auto &recordInfo : jsRecordInfo_) {
            recordInfo.second->vmListOfParsedConstPool.erase(vm);
        }
    }
    static FunctionKind PUBLIC_API GetFunctionKind(panda_file::FunctionKind funcKind);
    static FunctionKind GetFunctionKind(ConstPoolType type);
    static bool PUBLIC_API IsSendableFunctionKind(panda_file::FunctionKind funcKind)
    {
        return (static_cast<uint32_t>(funcKind) & SENDABLE_FUNCTION_MASK) != 0;
    }

    const void *GetBase() const
    {
        return static_cast<const void *>(pf_->GetBase());
    }

    void ClearNameMap();

    void TranslateClasses(JSThread *thread, const CString &methodName);

    bool IsHapPath() const
    {
        return base::StringHelper::StringEndWith(hapPath_, HAP_SUFFIX);
    }

private:
    void InitializeUnMergedPF();
    void InitializeMergedPF();

    void WaitTranslateClassTaskFinished();

    void NotifyTranslateClassTaskCompleted();

    void IncreaseTaskCount();

    void TranslateClass(JSThread *thread, const CString &methodName);

    void PostInitializeMethodTask(JSThread *thread, const std::shared_ptr<CString> &methodNamePtr);

    void ReduceTaskCount();

    void SetAllMethodLiteralToMap();

    void GetClassAndMethodIndexes(std::vector<std::pair<uint32_t, uint32_t>> &indexes);

    static constexpr size_t VERSION_SIZE = 4;
    static constexpr std::array<uint8_t, VERSION_SIZE> OLD_VERSION {0, 0, 0, 2};
    static constexpr uint32_t SENDABLE_FUNCTION_MASK = 1 << 3;

    // please add member after *pf_. static constexpr int32_t PF_OFFSET = 0.
    const panda_file::File *pf_ {nullptr};
    CString hapPath_;
    uint32_t constpoolIndex_ {0};
    uint32_t checksum_ {0};
    std::unordered_map<uint32_t, MethodLiteral *> methodLiteralMap_;
    std::unordered_map<uint32_t, panda_file::File::StringData> methodNameMap_;
    CUnorderedMap<uint32_t, CString> recordNameMap_;
    Mutex methodNameMapMutex_;
    Mutex recordNameMapMutex_;
    Mutex waitTranslateClassFinishedMutex_;
    Mutex classIndexMutex_;
    Mutex jsRecordInfoMutex_;
    ConditionVariable waitTranslateClassFinishedCV_;
    uint32_t runningTaskCount_ {0};
    uint32_t classIndex_ {0};
    uint32_t methodIndex_ {0};

    CUnorderedMap<uint32_t, uint64_t> constpoolMap_;
    uint32_t numMethods_ {0};
    uint32_t numClasses_ {0};
    MethodLiteral *methodLiterals_ {nullptr};
    CString desc_;
    uint32_t anFileInfoIndex_ {INVALID_INDEX};
    bool isNewVersion_ {false};

    // marge abc
    bool isBundlePack_ {true}; // isBundlePack means app compile mode is JSBundle
    CUnorderedMap<CString, JSRecordInfo*> jsRecordInfo_;
    CUnorderedMap<CString, CString> npmEntries_;
    bool isRecordWithBundleName_ {true};
    CreateMode mode_ {CreateMode::RUNTIME};
    friend class JSPandaFileSnapshot;
};
}  // namespace ecmascript
}  // namespace panda
#endif // ECMASCRIPT_JSPANDAFILE_JS_PANDAFILE_H
