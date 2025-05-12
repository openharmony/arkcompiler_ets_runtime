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

#include "common_components/taskpool/taskpool.h"
#include "ecmascript/jspandafile/program_object.h"

namespace panda::ecmascript {
namespace {
const CString OHOS_PKG_ABC_PATH_ROOT = "/ets/";  // abc file always under /ets/ dir in HAP/HSP
}  // namespace
JSPandaFile::JSPandaFile(const panda_file::File *pf, const CString &descriptor, CreateMode mode)
    : pf_(pf), desc_(descriptor), mode_(mode)
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
            std::string_view fieldName(utf::Mutf8AsCString(sd.data), sd.utf16_length);
            if ((IS_COMMON_JS == fieldName) || (MODULE_RECORD_IDX == fieldName)) {
                isBundlePack_ = false;
            }
        });
        if (!isBundlePack_) {
            return;
        }
    }
}

void JSPandaFile::CheckIsRecordWithBundleName(const CString &entry)
{
    size_t pos = entry.find('/');
    if (pos == CString::npos) {
        LOG_FULL(FATAL) << "CheckIsRecordWithBundleName Invalid parameter entry";
    }

    CString bundleName = entry.substr(0, pos);
    size_t bundleNameLen = bundleName.length();
    for (auto &[recordName, _] : jsRecordInfo_) {
        if (recordName.find(PACKAGE_PATH_SEGMENT) != CString::npos ||
            recordName.find(NPM_PATH_SEGMENT) != CString::npos) {
            continue;
        }
        // Confirm whether the current record is new or old by judging whether the recordName has a bundleName
        if (!(recordName.length() > bundleNameLen && (recordName.compare(0, bundleNameLen, bundleName) == 0))) {
            isRecordWithBundleName_ = false;
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
    for (auto& each : jsRecordInfo_) {
        delete each.second;
    }
    jsRecordInfo_.clear();
    methodLiteralMap_.clear();
    ClearNameMap();
    if (methodLiterals_ != nullptr) {
        JSPandaFileManager::FreeBuffer(methodLiterals_, sizeof(MethodLiteral) * numMethods_, isBundlePack_, mode_);
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
    numClasses_ = classIndexes.size();
    JSRecordInfo* info = new JSRecordInfo();
    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (pf_->IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(*pf_, classId);
        numMethods_ += cda.GetMethodsNumber();
        std::string_view desc(utf::Mutf8AsCString(cda.GetDescriptor()));
        if (info->moduleRecordIdx == -1 && MODULE_CLASS == desc) {
            cda.EnumerateFields([&](panda_file::FieldDataAccessor &fieldAccessor) -> void {
                panda_file::File::EntityId fieldNameId = fieldAccessor.GetNameId();
                panda_file::File::StringData sd = GetStringData(fieldNameId);
                std::string_view fieldName(utf::Mutf8AsCString(sd.data), sd.utf16_length);
                if (fieldName != desc_) {
                    info->moduleRecordIdx = fieldAccessor.GetValue<int32_t>().value();
                    info->classId = index;
                    return;
                }
            });
        }
        if (!info->isCjs && COMMONJS_CLASS == desc) {
            info->classId = index;
            info->isCjs = true;
        }
        if (!info->isSharedModule && IS_SHARED_MODULE == desc) {
            info->isSharedModule = true;
        }
        if (!info->hasTopLevelAwait && HASTLA_CLASS == desc) {
            info->hasTopLevelAwait = true;
        }
    }
    jsRecordInfo_.insert({JSPandaFile::ENTRY_FUNCTION_NAME, info});
    methodLiterals_ = static_cast<MethodLiteral *>(
        JSPandaFileManager::AllocateBuffer(sizeof(MethodLiteral) * numMethods_, isBundlePack_, mode_));
    methodLiteralMap_.reserve(numMethods_);
}

void JSPandaFile::InitializeMergedPF()
{
    Span<const uint32_t> classIndexes = pf_->GetClasses();
    numClasses_ = classIndexes.size();
    CString traceInfo = "JSPandaFile::InitializeMergedPF:" + ToCString(numClasses_);
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, traceInfo.c_str());
    jsRecordInfo_.reserve(numClasses_);
    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (pf_->IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(*pf_, classId);
        numMethods_ += cda.GetMethodsNumber();
        JSRecordInfo* info = new JSRecordInfo();
        info->classId = index;
        bool hasCjsFiled = false;
        bool hasJsonFiled = false;
        CString desc = utf::Mutf8AsCString(cda.GetDescriptor());
        CString recordName = ParseEntryPoint(desc);
        cda.EnumerateFields([&](panda_file::FieldDataAccessor &fieldAccessor) -> void {
            panda_file::File::EntityId fieldNameId = fieldAccessor.GetNameId();
            panda_file::File::StringData sd = GetStringData(fieldNameId);
            std::string_view fieldName(utf::Mutf8AsCString(sd.data), sd.utf16_length);
            if (IS_COMMON_JS == fieldName) {
                hasCjsFiled = true;
                info->isCjs = fieldAccessor.GetValue<bool>().value();
            } else if (IS_JSON_CONTENT == fieldName) {
                hasJsonFiled = true;
                info->isJson = true;
                info->jsonStringId = fieldAccessor.GetValue<uint32_t>().value();
            } else if (MODULE_RECORD_IDX == fieldName) {
                info->moduleRecordIdx = fieldAccessor.GetValue<int32_t>().value();
            } else if (IS_SHARED_MODULE == fieldName) {
                info->isSharedModule = fieldAccessor.GetValue<bool>().value();
            } else if (HAS_TOP_LEVEL_AWAIT == fieldName) {
                info->hasTopLevelAwait = fieldAccessor.GetValue<bool>().value();
            } else if (LAZY_IMPORT == fieldName) {
                info->lazyImportIdx = fieldAccessor.GetValue<uint32_t>().value();
            } else if (sd.utf16_length > PACKAGE_NAME_LEN &&
                       fieldName.substr(0, PACKAGE_NAME_LEN) == PACKAGE_NAME) {
                std::string_view packageSuffix = fieldName.substr(PACKAGE_NAME_LEN);
                info->npmPackageName = CString(packageSuffix);
            } else {
                npmEntries_.emplace(recordName, fieldName);
            }
        });
        if (hasCjsFiled || hasJsonFiled) {
            jsRecordInfo_.emplace(recordName, info);
        } else {
            delete info;
        }
    }
    methodLiterals_ = static_cast<MethodLiteral *>(
        JSPandaFileManager::AllocateBuffer(sizeof(MethodLiteral) * numMethods_, isBundlePack_, mode_));
    methodLiteralMap_.reserve(numMethods_);
}

CString JSPandaFile::GetEntryPoint(const CString &recordName) const
{
    CString entryPoint;
    if (FindOhmUrlInPF(recordName, entryPoint) && HasRecord(entryPoint)) {
        return entryPoint;
    }
    return CString();
}

CString JSPandaFile::GetRecordName(const CString &entryPoint) const
{
    if (entryPoint.empty() || entryPoint == JSPandaFile::ENTRY_FUNCTION_NAME) {
        return GetJSPandaFileDesc();
    }
    return entryPoint;
}

bool JSPandaFile::FindOhmUrlInPF(const CString &recordName, CString &entryPoint) const
{
    auto info = npmEntries_.find(recordName);
    if (info != npmEntries_.end()) {
        entryPoint = info->second;
        return true;
    }
    return false;
}

FunctionKind JSPandaFile::GetFunctionKind(panda_file::FunctionKind funcKind)
{
    FunctionKind kind;
    if ((static_cast<uint32_t>(funcKind) & SENDABLE_FUNCTION_MASK) != 0) {
        funcKind = static_cast<panda_file::FunctionKind>(static_cast<uint32_t>(funcKind) & (~SENDABLE_FUNCTION_MASK));
    }
    switch (funcKind) {
        case panda_file::FunctionKind::NONE:
            kind = FunctionKind::NONE_FUNCTION;
            break;
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

/*
 handle desc like: case1: /data/storage/el1/bundle/entry/ets/modules.abc -> entry/ets/modules.abc
 case2: /data/storage/el1/bundle/entry/ets/widgets.abc -> entry/ets/widgets.abc
 case3: /data/app/el1/bundle/public/com.xx.xx/entry/ets/modules.abc -> entry/ets/modules.abc
 case4: /data/app/el1/bundle/public/com.xx.xx/entry/ets/widgets.abc -> entry/ets/widgets.abc
*/
CString JSPandaFile::GetNormalizedFileDesc(const CString &desc)
{
    auto etsTokenPos = desc.rfind(OHOS_PKG_ABC_PATH_ROOT);
    if (etsTokenPos == std::string::npos) {
        // file not in OHOS package.
        return desc;
    }
    auto ohosModulePos = desc.rfind('/', etsTokenPos - 1);
    if (ohosModulePos == std::string::npos) {
        LOG_ECMA(ERROR) << "Get abcPath from desc failed. desc: " << desc;
        return desc;
    }
    // substring likes {ohosModuleName}/ets/modules.abc or {ohosModuleName}/ets/widgets.abc
    return desc.substr(ohosModulePos + 1);
}

CString JSPandaFile::GetNormalizedFileDesc() const
{
    return GetNormalizedFileDesc(desc_);
}

std::pair<std::string_view, bool> JSPandaFile::GetMethodName(EntityId methodId)
{
    LockHolder lock(methodNameMapMutex_);
    uint32_t id = methodId.GetOffset();
    auto iter = methodNameMap_.find(id);
    if (iter != methodNameMap_.end()) {
        panda_file::StringData sd = iter->second;
        return {std::string_view(utf::Mutf8AsCString(sd.data), sd.utf16_length), sd.is_ascii};
    }

    panda_file::MethodDataAccessor mda(*pf_, methodId);
    auto sd = GetStringData(mda.GetNameId());
    methodNameMap_.emplace(id, sd);
    return {std::string_view(utf::Mutf8AsCString(sd.data), sd.utf16_length), sd.is_ascii};
}

std::pair<std::string_view, bool> JSPandaFile::GetCpuProfilerMethodName(EntityId methodId) const
{
    panda_file::MethodDataAccessor mda(*pf_, methodId);
    auto sd = GetStringData(mda.GetNameId());
    std::string_view strView(utf::Mutf8AsCString(sd.data), sd.utf16_length);
    return {strView, sd.is_ascii};
}

CString JSPandaFile::GetRecordName(EntityId methodId)
{
    LockHolder lock(recordNameMapMutex_);
    uint32_t id = methodId.GetOffset();
    auto iter = recordNameMap_.find(id);
    if (iter != recordNameMap_.end()) {
        return iter->second;
    }

    panda_file::MethodDataAccessor mda(*pf_, methodId);
    panda_file::ClassDataAccessor cda(*pf_, mda.GetClassId());
    CString desc = utf::Mutf8AsCString(cda.GetDescriptor());
    auto name =  JSPandaFile::ParseEntryPoint(desc);
    recordNameMap_.emplace(id, name);
    return name;
}

CString JSPandaFile::GetRecordNameWithBundlePack(EntityId methodIdx)
{
    CString recordName = IsBundlePack() ? ENTRY_FUNCTION_NAME : GetRecordName(methodIdx);
    ASSERT(HasRecord(recordName));
    return recordName;
}


void JSPandaFile::ClearNameMap()
{
    {
        LockHolder lock(methodNameMapMutex_);
        methodNameMap_.clear();
    }
    {
        LockHolder lock(recordNameMapMutex_);
        recordNameMap_.clear();
    }
}

void JSPandaFile::GetClassAndMethodIndexes(std::vector<std::pair<uint32_t, uint32_t>> &indexes)
{
    // Each thread gets 128 classes each time. If less than 128, it gets 2 classes.
    indexes.clear();
    LockHolder lock(classIndexMutex_);
    if (classIndex_ >= numClasses_) {
        return;
    }
    uint32_t cnts = ASYN_TRANSLATE_CLSSS_COUNT;
    uint32_t minCount = (Taskpool::GetCurrentTaskpool()->GetTotalThreadNum() + 1) * ASYN_TRANSLATE_CLSSS_COUNT;
    if (numClasses_ - classIndex_ < minCount) {
        cnts = ASYN_TRANSLATE_CLSSS_MIN_COUNT;
    }
    for (uint32_t i = 0; i < cnts; i++) {
        uint32_t classIdx = 0;
        uint32_t methodIdx = 0;
        Span<const uint32_t> classIndexes = GetClasses();
        uint32_t index = 0;
        do {
            classIdx = classIndex_++;
            if (classIdx >= numClasses_) {
                return;
            }
            index = classIndexes[classIdx];
        } while (IsExternal(panda_file::File::EntityId(index)));

        methodIdx = methodIndex_;
        panda_file::File::EntityId classId(classIndexes[classIdx]);
        panda_file::ClassDataAccessor cda(*pf_, classId);
        methodIndex_ += cda.GetMethodsNumber();
        indexes.emplace_back(methodIdx, classIdx);
    }
}

bool JSPandaFile::TranslateClassesTask::Run([[maybe_unused]] uint32_t threadIndex)
{
    jsPandaFile_->TranslateClass(thread_, *methodNamePtr_);
    jsPandaFile_->ReduceTaskCount();
    return true;
}

void JSPandaFile::TranslateClass(JSThread *thread, const CString &methodName)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "JSPandaFile::TranslateClass");
    std::vector<std::pair<uint32_t, uint32_t>> indexes;
    indexes.reserve(ASYN_TRANSLATE_CLSSS_COUNT);
    do {
        GetClassAndMethodIndexes(indexes);
        uint32_t size = indexes.size();
        for (uint32_t i = 0; i < size; i++) {
            PandaFileTranslator::TranslateClass(thread, this, methodName, indexes[i].first, indexes[i].second);
        }
    } while (!indexes.empty());
}

void JSPandaFile::PostInitializeMethodTask(JSThread *thread, const std::shared_ptr<CString> &methodNamePtr)
{
    IncreaseTaskCount();
    Taskpool::GetCurrentTaskpool()->PostTask(
        std::make_unique<TranslateClassesTask>(thread->GetThreadId(), thread, this, methodNamePtr));
}

void JSPandaFile::IncreaseTaskCount()
{
    LockHolder holder(waitTranslateClassFinishedMutex_);
    runningTaskCount_++;
}

void JSPandaFile::WaitTranslateClassTaskFinished()
{
    LockHolder holder(waitTranslateClassFinishedMutex_);
    while (runningTaskCount_ > 0) {
        waitTranslateClassFinishedCV_.Wait(&waitTranslateClassFinishedMutex_);
    }
}

void JSPandaFile::ReduceTaskCount()
{
    LockHolder holder(waitTranslateClassFinishedMutex_);
    runningTaskCount_--;
    if (runningTaskCount_ == 0) {
        waitTranslateClassFinishedCV_.SignalAll();
    }
}

void JSPandaFile::SetAllMethodLiteralToMap()
{
    // async to optimize SetAllMethodLiteralToMap later
    CString traceInfo = "JSPandaFile::SetAllMethodLiteralToMap:" + ToCString(numMethods_);
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, traceInfo.c_str());
    MethodLiteral *methodLiterals = GetMethodLiterals();
    size_t methodIdx = 0;
    while (methodIdx < numMethods_) {
        MethodLiteral *methodLiteral = methodLiterals + (methodIdx++);
        SetMethodLiteralToMap(methodLiteral);
    }
}

void JSPandaFile::TranslateClasses(JSThread *thread, const CString &methodName)
{
    const std::shared_ptr<CString> methodNamePtr = std::make_shared<CString>(methodName);
    for (uint32_t i = 0; i < Taskpool::GetCurrentTaskpool()->GetTotalThreadNum(); i++) {
        PostInitializeMethodTask(thread, methodNamePtr);
    }
    TranslateClass(thread, methodName);
    WaitTranslateClassTaskFinished();
    SetAllMethodLiteralToMap();
}
}  // namespace panda::ecmascript
