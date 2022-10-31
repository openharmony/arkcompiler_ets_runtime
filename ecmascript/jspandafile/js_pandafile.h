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

#ifndef ECMASCRIPT_JSPANDAFILE_JS_PANDAFILE_H
#define ECMASCRIPT_JSPANDAFILE_JS_PANDAFILE_H

#include "ecmascript/common.h"
#include "ecmascript/js_function.h"
#include "ecmascript/jspandafile/constpool_value.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/mem/c_containers.h"

#include "libpandafile/file.h"
namespace panda {
namespace ecmascript {
class JSPandaFile {
public:
    struct JSRecordInfo {
        uint32_t mainMethodIndex {0};
        bool isCjs {false};
        bool hasParsedLiteralConstPool {false};
        int moduleRecordIdx {-1};
        CUnorderedMap<uint32_t, uint64_t> constpoolMap;
        bool hasTSTypes {false};
        uint32_t typeSummaryOffset {0};

        bool HasTSTypes () const
        {
            return hasTSTypes;
        }

        uint32_t GetTypeSummaryOffset() const
        {
            return typeSummaryOffset;
        }
    };
    static constexpr char ENTRY_FUNCTION_NAME[] = "func_main_0";
    static constexpr char ENTRY_MAIN_FUNCTION[] = "_GLOBAL::func_main_0";
    static constexpr char PATCH_ENTRY_FUNCTION[] = "_GLOBAL::patch_main_0";
    static constexpr char PATCH_FUNCTION_NAME_0[] = "patch_main_0";
    static constexpr char PATCH_FUNCTION_NAME_1[] = "patch_main_1";

    static constexpr char MODULE_CLASS[] = "L_ESModuleRecord;";
    static constexpr char COMMONJS_CLASS[] = "L_CommonJsRecord;";
    static constexpr char TYPE_FLAG[] = "typeFlag";
    static constexpr char TYPE_SUMMARY_OFFSET[] = "typeSummaryOffset";

    static constexpr char IS_COMMON_JS[] = "isCommonjs";
    static constexpr char MODULE_RECORD_IDX[] = "moduleRecordIdx";
    static constexpr char MODULE_DEFAULE_ETS[] = "ets/";
    static constexpr char BUNDLE_INSTALL_PATH[] = "/data/storage/el1/bundle/";
    static constexpr char NODE_MODULES[] = "node_modules";
    static constexpr char NODE_MODULES_ZERO[] = "node_modules/0/";
    static constexpr char NODE_MODULES_ONE[] = "node_modules/1/";
    static constexpr char MERGE_ABC_NAME[] = "modules.abc";

    JSPandaFile(const panda_file::File *pf, const CString &descriptor);
    ~JSPandaFile();

    const CString &GetJSPandaFileDesc() const
    {
        return desc_;
    }

    const panda_file::File *GetPandaFile() const
    {
        return pf_;
    }

    std::string GetFileName() const
    {
        return pf_->GetFilename();
    }

    MethodLiteral* GetMethodLiterals() const
    {
        return methodLiterals_;
    }

    void SetMethodLiteralToMap(MethodLiteral *methodLiteral)
    {
        if (methodLiteral != nullptr) {
            methodLiteralMap_.emplace(methodLiteral->GetMethodId().GetOffset(), methodLiteral);
        }
    }

    const CUnorderedMap<uint32_t, MethodLiteral *> &GetMethodLiteralMap() const
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

    uint32_t GetMainMethodIndex(const CString &recordName = ENTRY_FUNCTION_NAME) const
    {
        if (IsBundlePack()) {
            return jsRecordInfo_.begin()->second.mainMethodIndex;
        }
        auto info = jsRecordInfo_.find(recordName);
        if (info != jsRecordInfo_.end()) {
            return info->second.mainMethodIndex;
        }
        return 0;
    }

    const CUnorderedMap<uint32_t, uint64_t> *GetConstpoolMapByReocrd(const CString &recordName) const
    {
        auto info = jsRecordInfo_.find(recordName);
        if (info != jsRecordInfo_.end()) {
            return &info->second.constpoolMap;
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
        if (IsBundlePack()) {
            jsRecordInfo_.begin()->second.mainMethodIndex = mainMethodIndex;
        } else {
            auto info = jsRecordInfo_.find(recordName);
            if (info != jsRecordInfo_.end()) {
                info->second.mainMethodIndex = mainMethodIndex;
            }
        }
    }

    PUBLIC_API MethodLiteral *FindMethodLiteral(uint32_t offset) const;

    int GetModuleRecordIdx(const CString &recordName = ENTRY_FUNCTION_NAME) const
    {
        if (IsBundlePack()) {
            return jsRecordInfo_.begin()->second.moduleRecordIdx;
        }
        auto info = jsRecordInfo_.find(recordName);
        if (info != jsRecordInfo_.end()) {
            return info->second.moduleRecordIdx;
        }
        // The array subscript will not have a negative number, and returning -1 means the search failed
        return -1;
    }

    Span<const uint32_t> GetClasses() const
    {
        return pf_->GetClasses();
    }

    bool PUBLIC_API IsModule(const CString &recordName = ENTRY_FUNCTION_NAME) const;

    bool IsCjs(const CString &recordName = ENTRY_FUNCTION_NAME) const;

    bool IsBundlePack() const
    {
        return isBundlePack_;
    }

    void SetLoadedAOTStatus(bool status)
    {
        isLoadedAOT_ = status;
    }

    bool IsLoadedAOT() const
    {
        return isLoadedAOT_;
    }

    uint32_t GetFileUniqId() const
    {
        return static_cast<uint32_t>(GetPandaFile()->GetUniqId());
    }

    bool IsNewVersion() const
    {
        return isNewVersion_;
    }

    bool HasRecord(const CString &recordName) const
    {
        auto info = jsRecordInfo_.find(recordName);
        if (info != jsRecordInfo_.end()) {
            return true;
        }
        return false;
    }

    JSRecordInfo FindRecordInfo(const CString &recordName) const
    {
        auto info = jsRecordInfo_.find(recordName);
        if (info == jsRecordInfo_.end()) {
            LOG_FULL(FATAL) << "find recordName failed: " << recordName;
            UNREACHABLE();
        }
        return info->second;
    }

    void UpdateHasParsedLiteralConstpool(const CString &recordName)
    {
        auto info = jsRecordInfo_.find(recordName);
        if (info == jsRecordInfo_.end()) {
            LOG_FULL(FATAL) << "find recordName failed: " << recordName;
            UNREACHABLE();
        }
        info->second.hasParsedLiteralConstPool = true;
    }

    // note : it only uses in TDD
    void InsertJSRecordInfo(const CString &recordName)
    {
        JSRecordInfo info;
        jsRecordInfo_.insert({recordName, info});
    }

    const CUnorderedMap<CString, JSRecordInfo> &GetJSRecordInfo() const
    {
        return jsRecordInfo_;
    }

    CString ParseEntryPoint(const CString &recordName) const
    {
        return recordName.substr(1, recordName.size() - 2); // 2 : skip symbol "L" and ";"
    }

    void CheckIsBundlePack();

    CString FindEntryPoint(const CString &record) const;

    static CString ParseOhmUrl(const CString &fileName);
    // For local merge abc, get record name from file name.
    static CString PUBLIC_API ParseRecordName(const CString &fileName);

    bool IsSystemLib() const
    {
        return false;
    }

    uint32_t GetAOTFileInfoIndex() const
    {
        return anFileInfoIndex_;
    }

    // If the system library is loaded, aotFileInfos has two elements
    // 0: system library, 1: application
    // Note: There is no system library currently, so the anFileInfoIndex_ is 0
    void SetAOTFileInfoIndex()
    {
        if (IsSystemLib()) {
            anFileInfoIndex_ = 0;
        } else {
            anFileInfoIndex_ = 1;
        }
    }

    static bool IsEntryOrPatch(const CString &name)
    {
        return (name == PATCH_FUNCTION_NAME_0) || (name == ENTRY_FUNCTION_NAME);
    }

    bool HasTSTypes(const CString &recordName) const
    {
        JSRecordInfo recordInfo = jsRecordInfo_.at(recordName);
        return recordInfo.HasTSTypes();
    }

    uint32_t GetTypeSummaryOffset(const CString &recordName) const
    {
        JSRecordInfo recordInfo = jsRecordInfo_.at(recordName);
        return recordInfo.GetTypeSummaryOffset();
    }

private:
    void InitializeUnMergedPF();
    void InitializeMergedPF();

    static constexpr size_t VERSION_SIZE = 4;
    static constexpr std::array<uint8_t, VERSION_SIZE> OLD_VERSION {0, 0, 0, 2};

    uint32_t constpoolIndex_ {0};
    CUnorderedMap<uint32_t, MethodLiteral *> methodLiteralMap_;
    CUnorderedMap<uint32_t, uint64_t> constpoolMap_;
    uint32_t numMethods_ {0};
    MethodLiteral *methodLiterals_ {nullptr};
    const panda_file::File *pf_ {nullptr};
    CString desc_;
    bool isLoadedAOT_ {false};
    uint32_t anFileInfoIndex_ {0};
    bool isNewVersion_ {false};

    // marge abc
    bool isBundlePack_ {true}; // isBundlePack means app compile mode is JSBundle
    CUnorderedMap<CString, JSRecordInfo> jsRecordInfo_;
};
}  // namespace ecmascript
}  // namespace panda
#endif // ECMASCRIPT_JSPANDAFILE_JS_PANDAFILE_H
