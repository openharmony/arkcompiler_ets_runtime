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

#include "ecmascript/jspandafile/program_object.h"

namespace panda::ecmascript {
namespace {
const CString OHOS_PKG_ABC_PATH_ROOT = "/ets/";  // abc file always under /ets/ dir in HAP/HSP
}  // namespace
bool JSPandaFile::loadedFirstPandaFile = false;
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
    if (!loadedFirstPandaFile && !isBundlePack_) {
        // Tag the first merged abc to use constant string. The lifetime of this first panda file is the same
        // as the vm. And make sure the first pandafile is the same at the compile time and runtime.
        isFirstPandafile_ = false;
        loadedFirstPandaFile = true;
    }
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

void JSPandaFile::CheckIsRecordWithBundleName(const CString &entry)
{
    size_t pos = entry.find('/');
    if (pos == CString::npos) {
        LOG_FULL(FATAL) << "CheckIsRecordWithBundleName Invalid parameter entry";
    }

    CString bundleName = entry.substr(0, pos);
    size_t bundleNameLen = bundleName.length();
    for (auto info : jsRecordInfo_) {
        if (info.first.find(PACKAGE_PATH_SEGMENT) != CString::npos ||
            info.first.find(NPM_PATH_SEGMENT) != CString::npos) {
            continue;
        }
        CString recordName = info.first;
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
        if (isBundlePack_) {
            JSPandaFileManager::FreeBuffer(methodLiterals_);
        } else {
            auto size = AlignUp(sizeof(MethodLiteral) * numMethods_, PageSize());
            PageClearTag(methodLiterals_, size);
            PageUnmap(MemMap(methodLiterals_, size));
        }
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
        const char *desc = utf::Mutf8AsCString(cda.GetDescriptor());
        if (info->moduleRecordIdx == -1 && std::strcmp(MODULE_CLASS, desc) == 0) {
            cda.EnumerateFields([&](panda_file::FieldDataAccessor &fieldAccessor) -> void {
                panda_file::File::EntityId fieldNameId = fieldAccessor.GetNameId();
                panda_file::File::StringData sd = GetStringData(fieldNameId);
                CString fieldName = utf::Mutf8AsCString(sd.data);
                if (fieldName != desc_) {
                    info->moduleRecordIdx = fieldAccessor.GetValue<int32_t>().value();
                    info->classId = index;
                    return;
                }
            });
        }
        if (!info->isCjs && std::strcmp(COMMONJS_CLASS, desc) == 0) {
            info->classId = index;
            info->isCjs = true;
        }
        if (!info->isSharedModule && std::strcmp(IS_SHARED_MODULE, desc) == 0) {
            info->isSharedModule = true;
        }
        if (!info->hasTopLevelAwait && std::strcmp(HASTLA_CLASS, desc) == 0) {
            info->hasTopLevelAwait = true;
        }
    }
    jsRecordInfo_.insert({JSPandaFile::ENTRY_FUNCTION_NAME, info});
    methodLiterals_ =
        static_cast<MethodLiteral *>(JSPandaFileManager::AllocateBuffer(sizeof(MethodLiteral) * numMethods_));
    methodLiteralMap_.reserve(numMethods_);
}

void JSPandaFile::InitializeMergedPF()
{
    Span<const uint32_t> classIndexes = pf_->GetClasses();
    numClasses_ = classIndexes.size();
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
            const char *fieldName = utf::Mutf8AsCString(sd.data);
            if (std::strcmp(IS_COMMON_JS, fieldName) == 0) {
                hasCjsFiled = true;
                info->isCjs = fieldAccessor.GetValue<bool>().value();
            } else if (std::strcmp(IS_JSON_CONTENT, fieldName) == 0) {
                hasJsonFiled = true;
                info->isJson = true;
                info->jsonStringId = fieldAccessor.GetValue<uint32_t>().value();
            } else if (std::strcmp(MODULE_RECORD_IDX, fieldName) == 0) {
                info->moduleRecordIdx = fieldAccessor.GetValue<int32_t>().value();
            } else if (std::strcmp(IS_SHARED_MODULE, fieldName) == 0) {
                info->isSharedModule = fieldAccessor.GetValue<bool>().value();
            } else if (std::strcmp(HAS_TOP_LEVEL_AWAIT, fieldName) == 0) {
                info->hasTopLevelAwait = fieldAccessor.GetValue<bool>().value();
            } else if (std::strcmp(LAZY_IMPORT, fieldName) == 0) {
                info->lazyImportIdx = fieldAccessor.GetValue<uint32_t>().value();
            } else if (std::strlen(fieldName) > PACKAGE_NAME_LEN &&
                       std::strncmp(fieldName, PACKAGE_NAME, PACKAGE_NAME_LEN) == 0) {
                info->npmPackageName = fieldName + PACKAGE_NAME_LEN;
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
    auto size = AlignUp(sizeof(MethodLiteral) * numMethods_, PageSize());
    methodLiterals_ =
        static_cast<MethodLiteral *>(PageMap(size, PAGE_PROT_READWRITE).GetMem());
    PageTag(methodLiterals_, size, PageTagType::METHOD_LITERAL);
    methodLiteralMap_.reserve(numMethods_);
}

MethodLiteral *JSPandaFile::FindMethodLiteral(uint32_t offset) const
{
    auto iter = methodLiteralMap_.find(offset);
    if (iter == methodLiteralMap_.end()) {
        return nullptr;
    }
    return iter->second;
}

bool JSPandaFile::IsFirstMergedAbc() const
{
    if (isFirstPandafile_ && !IsBundlePack()) {
        return true;
    }
    return false;
}

bool JSPandaFile::CheckAndGetRecordInfo(const CString &recordName, [[maybe_unused]] JSRecordInfo **recordInfo) const
{
    if (IsBundlePack()) {
        *recordInfo = jsRecordInfo_.begin()->second;
        return true;
    }
    auto info = jsRecordInfo_.find(recordName);
    if (info != jsRecordInfo_.end()) {
        *recordInfo = info->second;
        return true;
    }
    return false;
}

const JSRecordInfo* JSPandaFile::GetRecordInfo(const CString &recordName)
{
    if (IsBundlePack()) {
        return jsRecordInfo_.begin()->second;
    }
    auto info = jsRecordInfo_.find(recordName);
    if (info != jsRecordInfo_.end()) {
        return info->second;
    }
    LOG_FULL(FATAL) << "Get record info failed";
    UNREACHABLE();
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

size_t JSPandaFile::GetClassAndMethodIndex(size_t *methodIdx)
{
    LockHolder lock(classIndexMutex_);
    size_t result = 0;
    Span<const uint32_t> classIndexes = GetClasses();
    uint32_t index = 0;
    do {
        result = classIndex_++;
        if (result >= numClasses_) {
            return result;
        }
        index = classIndexes[result];
    } while (IsExternal(panda_file::File::EntityId(index)));

    *methodIdx = methodIndex_;
    panda_file::File::EntityId classId(classIndexes[result]);
    panda_file::ClassDataAccessor cda(*pf_, classId);
    methodIndex_ += cda.GetMethodsNumber();
    return result;
}

bool JSPandaFile::TranslateClassesTask::Run([[maybe_unused]] uint32_t threadIndex)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "TranslateClassesTask::Run");
    jsPandaFile_->TranslateClass(thread_, *methodNamePtr_);
    jsPandaFile_->ReduceTaskCount();
    return true;
}

void JSPandaFile::TranslateClass(JSThread *thread, const CString &methodName)
{
    size_t methodIdx = 0;
    size_t classIdx = GetClassAndMethodIndex(&methodIdx);
    while (classIdx < numClasses_) {
        PandaFileTranslator::TranslateClass(thread, this, methodName, methodIdx, classIdx);
        classIdx = GetClassAndMethodIndex(&methodIdx);
    }
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
