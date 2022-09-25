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

#include "ecmascript/jspandafile/js_pandafile.h"

#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/snapshot/mem/snapshot_processor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/program_object.h"

#include "libpandafile/class_data_accessor-inl.h"

namespace panda::ecmascript {
JSPandaFile::JSPandaFile(const panda_file::File *pf, const CString &descriptor) : pf_(pf), desc_(descriptor)
{
    ASSERT(pf_ != nullptr);
#if ECMASCRIPT_ENABLE_MERGE_ABC
    checkIsBundlePack();
    if (isBundlePack_) {
        InitializeUnMergedPF();
    } else {
        InitializeMergedPF();
    }
#else
    InitializeUnMergedPF();
#endif
    isNewVersion_ = pf_->GetHeader()->version > OLD_VERSION;
}

void JSPandaFile::checkIsBundlePack()
{
    Span<const uint32_t> classIndexes = pf_->GetClasses();
    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (pf_->IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(*pf_, classId);
        cda.EnumerateFields([&](panda_file::FieldDataAccessor &fieldAccessor) -> void {
            panda_file::File::EntityId fieldNameId = fieldAccessor.GetNameId();
            panda_file::File::StringData sd = pf_->GetStringData(fieldNameId);
            const char *fieldName = utf::Mutf8AsCString(sd.data);
            if (std::strcmp(IS_COMMON_JS, fieldName) == 0 || std::strcmp(MODULE_RECORD_IDX, fieldName) == 0) {
                isBundlePack_ = false;
            }
        });
        if (!isBundlePack_) {
            return;
        }
    }
}

JSPandaFile::~JSPandaFile()
{
    if (pf_ != nullptr) {
        delete pf_;
        pf_ = nullptr;
    }
    methodLiteralMap_.clear();
    if (methodLiterals_ != nullptr) {
        JSPandaFileManager::FreeBuffer(methodLiterals_);
        methodLiterals_ = nullptr;
    }
}

uint32_t JSPandaFile::GetOrInsertConstantPool(ConstPoolType type, uint32_t offset,
                                              const CUnorderedMap<uint32_t, uint64_t> *constpoolMap)
{
    CUnorderedMap<uint32_t, uint64_t> *map = nullptr;
    if (constpoolMap != nullptr && !IsBundlePack()) {
        map = const_cast<CUnorderedMap<uint32_t, uint64_t> *>(constpoolMap);
    } else {
        map = &constpoolMap_;
    }
    auto it = map->find(offset);
    if (it != map->cend()) {
        ConstPoolValue value(it->second);
        return value.GetConstpoolIndex();
    }
    ASSERT(constpoolIndex_ != UINT32_MAX);
    uint32_t index = constpoolIndex_++;
    ConstPoolValue value(type, index);
    map->insert({offset, value.GetValue()});
    return index;
}

void JSPandaFile::InitializeUnMergedPF()
{
    Span<const uint32_t> classIndexes = pf_->GetClasses();
    JSRecordInfo info;
    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (pf_->IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(*pf_, classId);
        numMethods_ += cda.GetMethodsNumber();
        const char *desc = utf::Mutf8AsCString(cda.GetDescriptor());
        if (info.moduleRecordIdx == -1 && std::strcmp(MODULE_CLASS, desc) == 0) {
            cda.EnumerateFields([&](panda_file::FieldDataAccessor &field_accessor) -> void {
                panda_file::File::EntityId field_name_id = field_accessor.GetNameId();
                panda_file::File::StringData sd = pf_->GetStringData(field_name_id);
                if (std::strcmp(reinterpret_cast<const char *>(sd.data), desc_.c_str())) {
                    info.moduleRecordIdx = field_accessor.GetValue<int32_t>().value();
                    return;
                }
            });
        }
        if (!info.isCjs && std::strcmp(COMMONJS_CLASS, desc) == 0) {
            info.isCjs = true;
        }

        if (!HasTSTypes() && std::strcmp(TS_TYPES_CLASS, desc) == 0) {
            cda.EnumerateFields([&](panda_file::FieldDataAccessor &fieldAccessor) -> void {
                panda_file::File::EntityId fieldNameId = fieldAccessor.GetNameId();
                panda_file::File::StringData sd = pf_->GetStringData(fieldNameId);
                const char *fieldName = utf::Mutf8AsCString(sd.data);
                if (std::strcmp(TYPE_FLAG, fieldName) == 0) {
                    hasTSTypes_ = fieldAccessor.GetValue<uint8_t>().value() != 0;
                }
                if (std::strcmp(TYPE_SUMMARY_INDEX, fieldName) == 0) {
                    typeSummaryIndex_ = fieldAccessor.GetValue<uint32_t>().value();
                }
            });
        }
    }
    jsRecordInfo_.insert({JSPandaFile::ENTRY_FUNCTION_NAME, info});
    methodLiterals_ =
        static_cast<MethodLiteral *>(JSPandaFileManager::AllocateBuffer(sizeof(MethodLiteral) * numMethods_));
}

void JSPandaFile::InitializeMergedPF()
{
    Span<const uint32_t> classIndexes = pf_->GetClasses();
    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (pf_->IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(*pf_, classId);
        numMethods_ += cda.GetMethodsNumber();
        CString desc = utf::Mutf8AsCString(cda.GetDescriptor());
        if (!HasTSTypes() && std::strcmp(TS_TYPES_CLASS, desc.c_str()) == 0) {
            cda.EnumerateFields([&](panda_file::FieldDataAccessor &fieldAccessor) -> void {
                panda_file::File::EntityId fieldNameId = fieldAccessor.GetNameId();
                panda_file::File::StringData sd = pf_->GetStringData(fieldNameId);
                const char *fieldName = utf::Mutf8AsCString(sd.data);
                if (std::strcmp(TYPE_FLAG, fieldName) == 0) {
                    hasTSTypes_ = fieldAccessor.GetValue<uint8_t>().value() != 0;
                }
                if (std::strcmp(TYPE_SUMMARY_INDEX, fieldName) == 0) {
                    typeSummaryIndex_ = fieldAccessor.GetValue<uint32_t>().value();
                }
            });
            continue;
        }
        // get record info
        JSRecordInfo info;
        bool hasCjsFiled = false;
        cda.EnumerateFields([&](panda_file::FieldDataAccessor &fieldAccessor) -> void {
            panda_file::File::EntityId fieldNameId = fieldAccessor.GetNameId();
            panda_file::File::StringData sd = pf_->GetStringData(fieldNameId);
            const char *fieldName = utf::Mutf8AsCString(sd.data);
            if (std::strcmp(IS_COMMON_JS, fieldName) == 0) {
                hasCjsFiled = true;
                info.isCjs = fieldAccessor.GetValue<bool>().value();
            } else {
                if (std::strcmp(MODULE_RECORD_IDX, fieldName) == 0) {
                    info.moduleRecordIdx = fieldAccessor.GetValue<int32_t>().value();
                }
            }
        });
        if (hasCjsFiled) {
            jsRecordInfo_.insert({ParseEntryPoint(desc), info});
        }
    }
    methodLiterals_ =
        static_cast<MethodLiteral *>(JSPandaFileManager::AllocateBuffer(sizeof(MethodLiteral) * numMethods_));
}

MethodLiteral *JSPandaFile::FindMethodLiteral(uint32_t offset) const
{
    auto iter = methodLiteralMap_.find(offset);
    if (iter == methodLiteralMap_.end()) {
        return nullptr;
    }
    return iter->second;
}

bool JSPandaFile::IsModule(const CString &recordName) const
{
    if (IsBundlePack()) {
        return jsRecordInfo_.begin()->second.moduleRecordIdx == -1 ? false : true;
    }
    auto info = jsRecordInfo_.find(recordName);
    if (info != jsRecordInfo_.end()) {
        return info->second.moduleRecordIdx == -1 ? false : true;
    }
    return false;
}

bool JSPandaFile::IsCjs(const CString &recordName) const
{
    if (IsBundlePack()) {
        return jsRecordInfo_.begin()->second.isCjs;
    }
    auto info = jsRecordInfo_.find(recordName);
    if (info != jsRecordInfo_.end()) {
        return info->second.isCjs;
    }
    return false;
}

CString JSPandaFile::FindrecordName(const CString &recordName) const
{
    Span<const uint32_t> classIndexes = pf_->GetClasses();
    CString name = "";
    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (pf_->IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(*pf_, classId);
        CString desc = utf::Mutf8AsCString(cda.GetDescriptor());
        if (std::strcmp(recordName.c_str(), ParseEntryPoint(desc).c_str()) == 0) {
            cda.EnumerateFields([&](panda_file::FieldDataAccessor &fieldAccessor) -> void {
                panda_file::File::EntityId fieldNameId = fieldAccessor.GetNameId();
                panda_file::File::StringData sd = pf_->GetStringData(fieldNameId);
                const char *fieldName = utf::Mutf8AsCString(sd.data);
                if (HasRecord(fieldName)) {
                    name = fieldName;
                }
            });
        }
        if (!name.empty()) {
            return name;
        }
    }
    return name;
}

std::string JSPandaFile::ParseOhmUrl(std::string fileName)
{
    std::string bundleInstallName(BUNDLE_INSTALL_PATH);
    size_t startStrLen =  bundleInstallName.length();
    size_t pos = std::string::npos;

    if (fileName.length() > startStrLen && fileName.compare(0, startStrLen, bundleInstallName) == 0) {
        pos = startStrLen;
    }
    std::string result = fileName;
    if (pos != std::string::npos) {
        result = fileName.substr(pos);
        pos = result.find('/');
        if (pos != std::string::npos) {
            result = result.substr(pos + 1);
        }
    } else {
        // Temporarily handle the relative path sent by arkui
        result = MODULE_DEFAULE_ETS + result;
    }
    pos = result.find_last_of(".");
    if (pos != std::string::npos) {
        result = result.substr(0, pos);
    }

    return result;
}

std::string JSPandaFile::ParseRecordName(const std::string &fileName)
{
    size_t begin = fileName.find_last_of("/");
    ASSERT(begin != std::string::npos);
    size_t end = fileName.find_last_of(".");
    ASSERT(end != std::string::npos);
    return fileName.substr(begin + 1, end - begin - 1);
}
}  // namespace panda::ecmascript
