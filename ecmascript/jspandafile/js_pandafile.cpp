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

#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/program_object.h"
#include "libpandafile/class_data_accessor-inl.h"

namespace panda::ecmascript {
JSPandaFile::JSPandaFile(const panda_file::File *pf, const CString &descriptor)
    : pf_(pf), desc_(descriptor)
{
    ASSERT(pf_ != nullptr);
    CheckIsBundlePack();
    if (isBundlePack_) {
        InitializeUnMergedPF();
    } else {
        InitializeMergedPF();
    }
    checksum_ = pf->GetHeader()->checksum;
    isNewVersion_ = pf_->GetHeader()->version > OLD_VERSION;
}

void JSPandaFile::CheckIsBundlePack()
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
            panda_file::File::StringData sd = GetStringData(fieldNameId);
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

void JSPandaFile::CheckIsNewRecord(EcmaVM *vm)
{
    if (vm->IsRecordWithBundleName()) {
        return;
    }
    for (auto info : jsRecordInfo_) {
        if (info.first.find(PACKAGE_PATH_SEGMENT) != CString::npos ||
            info.first.find(NPM_PATH_SEGMENT) != CString::npos) {
            continue;
        }
        CString recordName = info.first;
        CString bundleName = vm->GetBundleName();
        size_t bundleNameLen = bundleName.length();
        // Confirm whether the current record is new or old by judging whether the recordName has a bundleName
        if (recordName.length() > bundleNameLen && (recordName.compare(0, bundleNameLen, bundleName) == 0)) {
            vm->SetRecordWithBundleNameTag(true);
        }
        return;
    }
}

JSPandaFile::~JSPandaFile()
{
    if (pf_ != nullptr) {
        delete pf_;
        pf_ = nullptr;
    }

    constpoolMap_.clear();
    jsRecordInfo_.clear();
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
    map->emplace(offset, value.GetValue());
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
            cda.EnumerateFields([&](panda_file::FieldDataAccessor &fieldAccessor) -> void {
                panda_file::File::EntityId fieldNameId = fieldAccessor.GetNameId();
                panda_file::File::StringData sd = GetStringData(fieldNameId);
                CString fieldName = utf::Mutf8AsCString(sd.data);
                if (fieldName != desc_) {
                    info.moduleRecordIdx = fieldAccessor.GetValue<int32_t>().value();
                    return;
                }
            });
        }
        if (!info.isCjs && std::strcmp(COMMONJS_CLASS, desc) == 0) {
            info.isCjs = true;
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
        // get record info
        JSRecordInfo info;
        bool hasCjsFiled = false;
        bool hasJsonFiled = false;
        cda.EnumerateFields([&](panda_file::FieldDataAccessor &fieldAccessor) -> void {
            panda_file::File::EntityId fieldNameId = fieldAccessor.GetNameId();
            panda_file::File::StringData sd = GetStringData(fieldNameId);
            const char *fieldName = utf::Mutf8AsCString(sd.data);
            if (std::strcmp(IS_COMMON_JS, fieldName) == 0) {
                hasCjsFiled = true;
                info.isCjs = fieldAccessor.GetValue<bool>().value();
            } else if (std::strcmp(IS_JSON_CONTENT, fieldName) == 0) {
                hasJsonFiled = true;
                info.isJson = true;
                info.jsonStringId = fieldAccessor.GetValue<uint32_t>().value();
            } else if (std::strcmp(MODULE_RECORD_IDX, fieldName) == 0) {
                info.moduleRecordIdx = fieldAccessor.GetValue<int32_t>().value();
            } else if (std::strcmp(TYPE_FLAG, fieldName) == 0) {
                info.hasTSTypes = fieldAccessor.GetValue<uint8_t>().value() != 0;
            } else if (std::strcmp(TYPE_SUMMARY_OFFSET, fieldName) == 0) {
                info.typeSummaryOffset = fieldAccessor.GetValue<uint32_t>().value();
            } else if (std::strlen(fieldName) > PACKAGE_NAME_LEN &&
                       std::strncmp(fieldName, PACKAGE_NAME, PACKAGE_NAME_LEN) == 0) {
                info.npmPackageName = fieldName + PACKAGE_NAME_LEN;
            }
        });
        if (hasCjsFiled || hasJsonFiled) {
            CString desc = utf::Mutf8AsCString(cda.GetDescriptor());
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

bool JSPandaFile::IsModule(JSThread *thread, const CString &recordName) const
{
    if (IsBundlePack()) {
        return jsRecordInfo_.begin()->second.moduleRecordIdx != -1;
    }
    auto info = jsRecordInfo_.find(recordName);
    if (info != jsRecordInfo_.end()) {
        return info->second.moduleRecordIdx != -1;
    }
    CString msg = "Faild to load file '" + recordName + "', please check the request path.";
    THROW_REFERENCE_ERROR_AND_RETURN(thread, msg.c_str(), false);
}

bool JSPandaFile::IsCjs(JSThread *thread, const CString &recordName) const
{
    if (IsBundlePack()) {
        return jsRecordInfo_.begin()->second.isCjs;
    }
    auto info = jsRecordInfo_.find(recordName);
    if (info != jsRecordInfo_.end()) {
        return info->second.isCjs;
    }
    CString msg = "Faild to load file '" + recordName + "', please check the request path.";
    THROW_REFERENCE_ERROR_AND_RETURN(thread, msg.c_str(), false);
}

bool JSPandaFile::IsJson(JSThread *thread, const CString &recordName) const
{
    if (IsBundlePack()) {
        return jsRecordInfo_.begin()->second.isJson;
    }
    auto info = jsRecordInfo_.find(recordName);
    if (info != jsRecordInfo_.end()) {
        return info->second.isJson;
    }
    CString msg = "Faild to load file '" + recordName + "', please check the request path.";
    THROW_REFERENCE_ERROR_AND_RETURN(thread, msg.c_str(), false);
}

CString JSPandaFile::GetJsonStringId(JSThread *thread, const CString &recordName) const
{
    if (IsBundlePack()) {
        StringData sd = GetStringData(EntityId(jsRecordInfo_.begin()->second.jsonStringId));
        return utf::Mutf8AsCString(sd.data);
    }

    auto info = jsRecordInfo_.find(recordName);
    if (info != jsRecordInfo_.end()) {
        StringData sd = GetStringData(EntityId(info->second.jsonStringId));
        return utf::Mutf8AsCString(sd.data);
    }
    CString msg = "Faild to load file '" + recordName + "', please check the request path.";
    THROW_REFERENCE_ERROR_AND_RETURN(thread, msg.c_str(), "");
}

CString JSPandaFile::FindNpmEntryPoint(const CString &recordName) const
{
    if (HasRecord(recordName)) {
        return recordName;
    }
    Span<const uint32_t> classIndexes = pf_->GetClasses();
    CString entryPoint;
    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (pf_->IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(*pf_, classId);
        CString desc = utf::Mutf8AsCString(cda.GetDescriptor());
        if (recordName == ParseEntryPoint(desc)) {
            cda.EnumerateFields([&](panda_file::FieldDataAccessor &fieldAccessor) -> void {
                panda_file::File::EntityId fieldNameId = fieldAccessor.GetNameId();
                panda_file::File::StringData sd = GetStringData(fieldNameId);
                CString fieldName = utf::Mutf8AsCString(sd.data);
                if (HasRecord(fieldName)) {
                    entryPoint = fieldName;
                }
            });
        }
        if (!entryPoint.empty()) {
            return entryPoint;
        }
    }
    return entryPoint;
}

FunctionKind JSPandaFile::GetFunctionKind(panda_file::FunctionKind funcKind)
{
    FunctionKind kind;
    switch (funcKind) {
        case panda_file::FunctionKind::NONE:
        case panda_file::FunctionKind::FUNCTION:
            kind = FunctionKind::BASE_CONSTRUCTOR;
            break;
        case panda_file::FunctionKind::NC_FUNCTION:
            kind = FunctionKind::ARROW_FUNCTION;
            break;
        case panda_file::FunctionKind::GENERATOR_FUNCTION:
            kind = FunctionKind::GENERATOR_FUNCTION;
            break;
        case panda_file::FunctionKind::ASYNC_FUNCTION:
            kind = FunctionKind::ASYNC_FUNCTION;
            break;
        case panda_file::FunctionKind::ASYNC_GENERATOR_FUNCTION:
            kind = FunctionKind::ASYNC_GENERATOR_FUNCTION;
            break;
        case panda_file::FunctionKind::ASYNC_NC_FUNCTION:
            kind = FunctionKind::ASYNC_ARROW_FUNCTION;
            break;
        case panda_file::FunctionKind::CONCURRENT_FUNCTION:
            kind = FunctionKind::CONCURRENT_FUNCTION;
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return kind;
}

FunctionKind JSPandaFile::GetFunctionKind(ConstPoolType type)
{
    FunctionKind kind;
    switch (type) {
        case ConstPoolType::BASE_FUNCTION:
            kind = FunctionKind::BASE_CONSTRUCTOR;
            break;
        case ConstPoolType::NC_FUNCTION:
            kind = FunctionKind::ARROW_FUNCTION;
            break;
        case ConstPoolType::GENERATOR_FUNCTION:
            kind = FunctionKind::GENERATOR_FUNCTION;
            break;
        case ConstPoolType::ASYNC_FUNCTION:
            kind = FunctionKind::ASYNC_FUNCTION;
            break;
        case ConstPoolType::CLASS_FUNCTION:
            kind = FunctionKind::CLASS_CONSTRUCTOR;
            break;
        case ConstPoolType::METHOD:
            kind = FunctionKind::NORMAL_FUNCTION;
            break;
        case ConstPoolType::ASYNC_GENERATOR_FUNCTION:
            kind = FunctionKind::ASYNC_GENERATOR_FUNCTION;
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return kind;
}
}  // namespace panda::ecmascript
