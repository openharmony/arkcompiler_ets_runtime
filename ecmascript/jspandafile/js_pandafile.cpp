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
    for (auto iter = jsRecordInfo_.begin(); iter != jsRecordInfo_.end(); iter++) {
        auto recordInfo = iter->second;
        recordInfo.constpoolMap.clear();
    }
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
                panda_file::File::StringData sd = pf_->GetStringData(fieldNameId);
                if (std::strcmp(reinterpret_cast<const char *>(sd.data), desc_.c_str())) {
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
        CString desc = utf::Mutf8AsCString(cda.GetDescriptor());
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
    LOG_FULL(FATAL) << "find entryPoint failed: " << recordName;
    UNREACHABLE();
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
    LOG_FULL(FATAL) << "find entryPoint failed: " << recordName;
    UNREACHABLE();
}

CString JSPandaFile::FindEntryPoint(const CString &recordName) const
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
        if (std::strcmp(recordName.c_str(), ParseEntryPoint(desc).c_str()) == 0) {
            cda.EnumerateFields([&](panda_file::FieldDataAccessor &fieldAccessor) -> void {
                panda_file::File::EntityId fieldNameId = fieldAccessor.GetNameId();
                panda_file::File::StringData sd = pf_->GetStringData(fieldNameId);
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

CString JSPandaFile::ParseOhmUrl(EcmaVM *vm, const CString &inputFileName, CString &outFileName)
{
    CString bundleInstallName(BUNDLE_INSTALL_PATH);
    size_t startStrLen = bundleInstallName.length();
    size_t pos = CString::npos;

    if (inputFileName.length() > startStrLen && inputFileName.compare(0, startStrLen, bundleInstallName) == 0) {
        pos = startStrLen;
    }
    CString entryPoint;
    if (pos != CString::npos) {
        pos = inputFileName.find('/', startStrLen);
        ASSERT(pos != CString::npos);
        CString moduleName = inputFileName.substr(startStrLen, pos - startStrLen);
        if (moduleName != vm->GetModuleName()) {
            outFileName = CString(BUNDLE_INSTALL_PATH) + moduleName + CString(MERGE_ABC_ETS_MODULES);
        }
        entryPoint = vm->GetBundleName() + "/" + inputFileName.substr(startStrLen);
    } else {
        // Temporarily handle the relative path sent by arkui
        entryPoint = vm->GetBundleName() + "/" + vm->GetModuleName() + MODULE_DEFAULE_ETS + inputFileName;
    }
    pos = entryPoint.rfind(".abc");
    if (pos != CString::npos) {
        entryPoint = entryPoint.substr(0, pos);
    }
    return entryPoint;
}

std::string JSPandaFile::ParseHapPath(const CString &fileName)
{
    CString bundleSubInstallName(BUNDLE_SUB_INSTALL_PATH);
    size_t startStrLen = bundleSubInstallName.length();
    if (fileName.length() > startStrLen && fileName.compare(0, startStrLen, bundleSubInstallName) == 0) {
        CString hapPath = fileName.substr(startStrLen);
        size_t pos = hapPath.find(MERGE_ABC_ETS_MODULES);
        if (pos != CString::npos) {
            return hapPath.substr(0, pos).c_str();
        }
    }
    return "";
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
