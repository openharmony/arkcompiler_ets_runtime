/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "modules_snapshot_helper.h"

#include <zlib.h>

#include "ecmascript/base/error_helper.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/platform/filesystem.h"
#include "ecmascript/platform/signal_manager.h"
#include "ecmascript/platform/time.h"
#include "ecmascript/serializer/serialize_data.h"

#include <csignal>
#include <cstddef>
#include <memory>
#include <mutex>
#include <securec.h>
#include <string>
#include <string_view>
#include <system_error>
#include <unistd.h>
#include <vector>

namespace panda::ecmascript {
bool ModulesSnapshotHelper::g_escaperDisabled_ = false;
int ModulesSnapshotHelper::g_featureState_ = static_cast<int>(SnapshotFeatureState::DEFAULT);
int ModulesSnapshotHelper::g_featureLoaded_ = static_cast<int>(SnapshotFeatureState::DEFAULT);
char ModulesSnapshotHelper::g_stateFilePathBuffer_[PATH_MAX] = {};
volatile bool ModulesSnapshotHelper::g_escaperTriggered_ {false};

ModulesSnapshotHelper::FileGuard::FileGuard(const CString &path, uint32_t tid)
    : target_(path), temp_(path + "." + CString(std::to_string(tid)))
{
}

ModulesSnapshotHelper::FileGuard::~FileGuard()
{
    if (!done_) {
        Unlink(temp_.c_str());
    }
}

bool ModulesSnapshotHelper::FileGuard::Done()
{
    if (done_) {
        return true;
    }
    auto err = Rename(temp_.c_str(), target_.c_str(), false);
    if (err) {
        LOG_ECMA(ERROR) << "Failed to rename to: '" << target_ << "', error: (" << err.value() << ")" << err.message();
        return false;
    }
    done_ = true;
    return true;
}

void ModulesSnapshotHelper::InitEscaper(EcmaVM *vm)
{
    static std::once_flag flag{};
    std::call_once(flag, [vm]() {
        // feature is not enabled, do not need to watch
        if ((g_featureState_ & static_cast<int>(SnapshotFeatureState::MODULE)) != 0 &&
            (g_featureState_ & static_cast<int>(SnapshotFeatureState::PANDAFILE)) != 0) {
            return;
        }

        vm->RegisterExtraJSCrashMessageCallback("modules snapshot escaper", []([[maybe_unused]] const EcmaVM* vm) {
            if (DoNeedEscape()) {
                TryDisableSnapshot(vm->GetJSThread());
            }
            return "";
        });

        constexpr const int SIGDUMP = 35;
        std::vector registeredSignos{
#ifdef SIGTRAP
            SIGTRAP,
#endif
#ifdef SIGABRT
            SIGABRT,
#endif
#ifdef SIGBUS
            SIGBUS,
#endif
#ifdef SIGFPE
            SIGFPE,
#endif
#ifdef SIGSEGV
            SIGSEGV,
#endif
#ifdef SIGSTKFLT
            SIGSTKFLT,
#endif
            SIGDUMP,
        };

        if (registeredSignos.empty()) {
            return;
        }
        DECL_SIGCHAIN_ACTION(action);
        if (!SignalManager::InitSigchainAction(action, registeredSignos, SigchainHandler)) {
            return;
        }
        for (auto signo : registeredSignos) {
            SignalManager::GetSignalManager(signo).AddSpecialHandler(action);
        }
    });
}

// escaper entry of signal handler, it would be called when cpp crash occurs
bool ModulesSnapshotHelper::SigchainHandler(int signo, void *info, void *ucontext)
{
    // snapshot feature is not loaded, already escaped or escaper is disabled, do not need to escape
    if (!DoNeedEscape()) {
        return false;
    }
    TryDisableSnapshot(signo);
    return false;
}

void ModulesSnapshotHelper::MarkModuleSnapshotLoaded()
{
    g_featureLoaded_ |= static_cast<int>(SnapshotFeatureState::MODULE);
}

void ModulesSnapshotHelper::MarkJSPandaFileSnapshotLoaded()
{
    g_featureLoaded_ |= static_cast<int>(SnapshotFeatureState::PANDAFILE);
}

void ModulesSnapshotHelper::MarkSnapshotDisabledByOption()
{
    g_featureState_ =
        static_cast<int>(SnapshotFeatureState::PANDAFILE) | static_cast<int>(SnapshotFeatureState::MODULE);
}

bool ModulesSnapshotHelper::TryDisableSnapshot(int reason)
{
    if (reason <= 0) {
        return TryDisableSnapshot(DISABLE_REASON_UNKNOWN);
    }
    constexpr size_t SIG_MAX_DIGITS = 2;
    char sigBuf[SIG_MAX_DIGITS]{};
    auto written = IntToString(reason, sigBuf, SIG_MAX_DIGITS);
    return TryDisableSnapshot(DISABLE_REASON_SIGNAL, std::string_view(sigBuf, written));
}

bool ModulesSnapshotHelper::TryDisableSnapshot(const std::string_view& reason, const std::string_view& extraInfo)
{
    // get crrent timestamp att first
    int64_t timestamp = GetCurrentTimestamp();
    PosixFile stateFile(g_stateFilePathBuffer_, "w");
    if (!stateFile.IsValid()) {
        return false;
    }
    std::string_view disableWord = "";
    // it means both snapshot feature is enabled at least during the process running,
    // disable module at first
    if ((g_featureLoaded_ & static_cast<int>(SnapshotFeatureState::MODULE)) != 0 &&
        (g_featureLoaded_ & static_cast<int>(SnapshotFeatureState::PANDAFILE)) != 0) {
        disableWord = &STATE_WORD_MODULE_SNAPSHOT_DISABLED;
    } else {
        // only one snapshot feature is loaded
        disableWord = &STATE_WORD_ALL_SNAPSHOT_DISABLED;
    }

    // core file content write failed.
    // probably the file system is broken, disk is full, don't have permission or other unknown reason.
    // return false and do not try to write anymore
    if (auto written = stateFile.Write(disableWord);
        written > 0 && static_cast<size_t>(written) < disableWord.size()) {
        return false;
    }
    stateFile.Write(":", 1);
    stateFile.Write(reason);
    stateFile.Write(extraInfo);
    constexpr size_t TIMESTAMP_MAX_DIGITS = 20;
    char timestampBuf[TIMESTAMP_MAX_DIGITS]{};
    stateFile.Write(std::string_view(", timestamp: "));
    auto written = IntToString(timestamp, timestampBuf, TIMESTAMP_MAX_DIGITS);
    stateFile.Write(timestampBuf, written);
    stateFile.Write(std::string_view("\n"));

    return true;
}

// escaper entry of uncaught exception, it would be called when uncaught exception occurs
void ModulesSnapshotHelper::TryDisableSnapshot(JSThread* thread)
{
    if (!DoNeedEscape()) {
        return;
    }

    if (!TryDisableSnapshot(DISABLE_REASON_UNCAUGHT_EXCEPTION)) {
        return;
    }
    LOG_ECMA(WARN) << "js crash occurred, try to disable snapshot";

    // Print exception info if there is a pending exception
    if (thread == nullptr || !thread->HasPendingException()) {
        return;
    }
    JSHandle<JSTaggedValue> exceptionHandle(thread, thread->GetException());
    if (!exceptionHandle->IsJSError()) {
        return;
    }
    // tostring method requires an exception-free environment
    thread->ClearException();
    std::string content {};
    content += "name: ";
    content += base::ErrorHelper::GetJSErrorInfo(thread, exceptionHandle, base::ErrorHelper::JSErrorProps::NAME);
    content += "\n";
    content += "message: ";
    content += base::ErrorHelper::GetJSErrorInfo(thread, exceptionHandle, base::ErrorHelper::JSErrorProps::MESSAGE);
    content += "\n";
    content += base::ErrorHelper::GetJSErrorInfo(thread, exceptionHandle, base::ErrorHelper::JSErrorProps::STACK);
    content += "\n";
    PosixFile stateFile(g_stateFilePathBuffer_, "r+");
    stateFile.Seek(0, PosixFile::SeekOrigin::END);
    stateFile.Write(content);
    LOG_ECMA(WARN) << "uncaught exception occurs, modues and (or) pandafile snapshot(s) will be disabled next time.\n"
        << content;
    thread->SetException(exceptionHandle.GetTaggedValue());
}

// escaper entry of freeze, it would be called when APP_FREEZE, SYS_FREEZE etc. event occurs
void ModulesSnapshotHelper::TryDisableSnapshotOnANR()
{
    if (!DoNeedEscape()) {
        return;
    }
    TryDisableSnapshot(DISABLE_APPLICATION_NOT_RESPONDING);
}

void ModulesSnapshotHelper::DisableSnapshotEscaper() { g_escaperDisabled_ = true; }

void ModulesSnapshotHelper::RemoveSnapshotFiles(const CString &path)
{
    if (FileExist(path.data())) {
        DeleteFilesWithSuffix(path.data(), SNAPSHOT_FILE_SUFFIX.data());
    }
}

void ModulesSnapshotHelper::RemoveFile(const CString& path)
{
    if (FileExist(path.data())) {
        Unlink(path.c_str());
    }
}

void ModulesSnapshotHelper::HandleCorruptedFile(const CString& dir, const CString& endsWith)
{
    if (!FileExist(dir.data())) {
        return;
    }
    const std::string backupSuffix(BACKUP_FILE_SUFFIX);
    auto err = Iterdir(dir.c_str(), [&dir, &endsWith, &backupSuffix](const std::string_view& name, FileType fileType) {
        if (fileType != FileType::REGULAR) {
            return;
        }
        if (name.size() < endsWith.size() || name.rfind(endsWith) != (name.size() - endsWith.size())) {
            return;
        }
        std::string fullPath(dir);
        fullPath += name;
        if (FileExist(fullPath.c_str())) {
            Rename(fullPath.c_str(), (fullPath + backupSuffix).c_str(), true);
        }
    });
    if (err) {
        LOG_ECMA(ERROR) << "HandleCorruptedFile failed, error: " << err.message();
    }
}

bool ModulesSnapshotHelper::SetReadOnly(const std::string_view &path, std::string *errorMsg)
{
    std::error_code ec{};
    if (Chmod(path, FILE_MODE_READONLY, ec)) {
        return true;
    }
    if (errorMsg != nullptr) {
        *errorMsg = ec.message();
    } else {
        LOG_ECMA(WARN) << "Failed to chmod file '" << path << "' to read-only, error: " << ec.message();
    }
    return false;
}

bool ModulesSnapshotHelper::WriteFileHeader(FileMemMapWriter& writer, const SnapshotVersionInfo::UniquePtr& header)
{
    // write snapshot file header
    if (!writer.WriteSingleData(header.get(), header->Size(), "SnapshotFileHeader")) {
        return false;
    }
    return true;
}

std::string ModulesSnapshotHelper::ReadFileHeader(FileMemMapReader& reader,
                                                  const SnapshotVersionInfo::UniquePtr& header)
{
    auto mHeader = SnapshotVersionInfo::FromBuffer(reader.GetReadPtr());
    if (!reader.Step(mHeader->Size())) {
        return "read snapshot file version info and description failed";
    }
    auto returnMismatch = [](const std::string_view& title, const std::string_view& read,
                             const std::string_view& current) {
        std::string errMsg = std::string(title) + " mismatch, read: " + std::string(read);
        errMsg += ", current: " + std::string(current);
        return errMsg;
    };
    if (mHeader->appVersionCode_ != header->appVersionCode_) {
        return returnMismatch("application version code", std::to_string(mHeader->appVersionCode_),
                              std::to_string(header->appVersionCode_));
    }
    auto mRomVersion = mHeader->GetRomVersion();
    auto romVersion = header->GetRomVersion();
    auto mDesc = mHeader->GetDescription();
    auto desc = header->GetDescription();
    if (romVersion != mRomVersion) {
        return returnMismatch("ROM version", mRomVersion, romVersion);
    }
    if (desc != mDesc) {
        return returnMismatch("snapshot file description", mDesc, desc);
    }
    return "";
}

bool ModulesSnapshotHelper::WriteDataInfo(FileMemMapWriter& writer, const std::unique_ptr<SerializeData>& data)
{
    SerializeDataInfo info {
        .dataIndex_ = data->dataIndex_,
        .sizeLimit_ = data->sizeLimit_,
        .bufferSize_ = data->bufferSize_,
        .bufferCapacity_ = data->bufferCapacity_,
        .regularSpaceSize_ = data->regularSpaceSize_,
        .pinSpaceSize_ = data->pinSpaceSize_,
        .oldSpaceSize_ = data->oldSpaceSize_,
        .nonMovableSpaceSize_ = data->nonMovableSpaceSize_,
        .machineCodeSpaceSize_ = data->machineCodeSpaceSize_,
        .sharedOldSpaceSize_ = data->sharedOldSpaceSize_,
        .sharedNonMovableSpaceSize_ = data->sharedNonMovableSpaceSize_,
        .incompleteData_ = data->incompleteData_,
    };
    CString message("serializeDataInfo");
    return writer.WriteSingleData(&info, sizeof(info), message);
}

bool ModulesSnapshotHelper::ReadDataInfo(FileMemMapReader& reader, const std::unique_ptr<SerializeData>& data)
{
    auto info = reinterpret_cast<const SerializeDataInfo*>(reader.GetReadPtr());
    if (!reader.Step(sizeof(SerializeDataInfo))) {
        return false;
    }
    data->dataIndex_ = info->dataIndex_;
    data->sizeLimit_ = info->sizeLimit_;
    data->bufferSize_ = info->bufferSize_;
    data->bufferCapacity_ = info->bufferCapacity_;
    data->regularSpaceSize_ = info->regularSpaceSize_;
    data->pinSpaceSize_ = info->pinSpaceSize_;
    data->oldSpaceSize_ = info->oldSpaceSize_;
    data->nonMovableSpaceSize_ = info->nonMovableSpaceSize_;
    data->machineCodeSpaceSize_ = info->machineCodeSpaceSize_;
    data->sharedOldSpaceSize_ = info->sharedOldSpaceSize_;
    data->sharedNonMovableSpaceSize_ = info->sharedNonMovableSpaceSize_;
    data->incompleteData_ = info->incompleteData_;
    return true;
}

bool ModulesSnapshotHelper::WriteDataToFile(const std::unique_ptr<SerializeData>& data,
                                            const CString& filePath, const SnapshotVersionInfo::UniquePtr& header,
                                            const CString& logPrefix)
{
    const CString logTag = logPrefix + "::WriteDataToFile";
    LOG_ECMA(DEBUG) << logTag;
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, logTag.c_str(), "");
    // calculate file total size
    // versionCode
    const uint32_t appVersionCode = header->appVersionCode_;
    constexpr const uint32_t checksumSize = sizeof(uint32_t);
    auto calcTotalSize = [&data, &header]() {
        size_t totalSize = header->Size();
        // GROUP_SIZE
        totalSize += sizeof(ModulesSnapshotHelper::SerializeDataInfo);

        if (g_isEnableCMCGC) {
            totalSize += CMC_GC_REGION_SIZE * sizeof(size_t);
            totalSize += data->regularRemainSizeVector_.size() * sizeof(size_t);
            totalSize += data->pinRemainSizeVector_.size() * sizeof(size_t);
        } else {
            // vector each element in vector's length
            totalSize += SERIALIZE_SPACE_NUM * sizeof(size_t);

            // vector data
            size_t totalVecBytes = 0;
            for (const auto& vec : data->regionRemainSizeVectors_) {
                totalVecBytes += vec.size() * sizeof(size_t);
            }
            totalSize += totalVecBytes;
        }
        // buffer data
        totalSize += data->bufferSize_;
        totalSize += checksumSize;
        return totalSize;
    };
    const size_t totalSize = calcTotalSize();
    if (FileExist(filePath.c_str())) {
        LOG_ECMA(WARN) << logTag << " snapshot file already exist: " << filePath;
        return false;
    }
    ModulesSnapshotHelper::FileGuard guard(filePath, JSThread::GetCurrentThreadId());
    MemMap fileMapMem =
        CreateFileMap(guard.GetTempPath().c_str(), totalSize, FILE_RDWR | FILE_CREAT | FILE_TRUNC, PAGE_PROT_READWRITE);
    if (fileMapMem.GetOriginAddr() == nullptr) {
        LOG_ECMA(ERROR) << logTag << " File mmap failed";
        return false;
    }
    MemMapScope memMapScope(fileMapMem);
    FileMemMapWriter writer(fileMapMem, logTag);
    if (!ModulesSnapshotHelper::WriteFileHeader(writer, header)) {
        return false;
    }
    if (!WriteDataInfo(writer, data)) {
        return false;
    }
    if (g_isEnableCMCGC) {
        size_t regularRemainSize = data->regularRemainSizeVector_.size();
        // regularRemainSizeVector size
        if (!writer.WriteSingleData(&regularRemainSize, sizeof(regularRemainSize), "regularRemainSize")) {
            return false;
        }
        // regularRemainSizeVector
        size_t regularRemainLen = regularRemainSize * sizeof(size_t);
        if (regularRemainLen > 0) {
            if (!writer.WriteSingleData(data->regularRemainSizeVector_.data(), regularRemainLen,
                                        "regularRemainSizeVector")) {
                return false;
            }
        }
        // pinRemainSizeVector size
        size_t pinRemainSize = data->pinRemainSizeVector_.size();
        if (!writer.WriteSingleData(&pinRemainSize, sizeof(pinRemainSize), "pinRemainSize")) {
            return false;
        }
        // pinRemainSizeVector
        size_t pinRemainLen = pinRemainSize * sizeof(size_t);
        if (pinRemainLen > 0) {
            if (!writer.WriteSingleData(data->pinRemainSizeVector_.data(), pinRemainLen, "pinRemainSizeVector")) {
                return false;
            }
        }
    } else {
        // write vector's size
        std::array<size_t, SERIALIZE_SPACE_NUM> vecSizes;
        for (int i = 0; i < SERIALIZE_SPACE_NUM; ++i) {
            vecSizes[i] = data->regionRemainSizeVectors_[i].size();
        }
        uint32_t vecSize = SERIALIZE_SPACE_NUM * sizeof(size_t);
        if (!writer.WriteSingleData(vecSizes.data(), vecSize, "vecSizes")) {
            return false;
        }

        // write vector's data
        for (const auto& vec : data->regionRemainSizeVectors_) {
            if (!vec.empty()) {
                uint32_t curVectorDataSize = vec.size() * sizeof(size_t);
                if (!writer.WriteSingleData(vec.data(), curVectorDataSize, "vec")) {
                    return false;
                }
            }
        }
    }
    // write buffer data
    if (data->bufferSize_ > 0) {
        if (!data->buffer_) {
            return false;
        }
        if (!writer.WriteSingleData(data->buffer_, data->bufferSize_, "buffer")) {
            return false;
        }
    }
    const uint32_t contentSize = fileMapMem.GetSize() - checksumSize;
    const uint32_t checksum = adler32(0, static_cast<const Bytef*>(fileMapMem.GetOriginAddr()), contentSize);
    if (!writer.WriteSingleData(&checksum, checksumSize, "checksum")) {
        return false;
    }
    FileSync(fileMapMem, FILE_MS_SYNC);
    memMapScope.Escape();
    FileUnMap(fileMapMem);
    if (guard.Done()) {
        ModulesSnapshotHelper::SetReadOnly(filePath);
    }
    LOG_ECMA(INFO) << logTag << " success";
    return true;
}

bool ModulesSnapshotHelper::ReadDataFromFile(const std::unique_ptr<SerializeData>& data, const CString& filePath,
                                             const SnapshotVersionInfo::UniquePtr& header, const CString& logPrefix)
{
    const CString logTag = logPrefix + "::ReadDataFromFile";
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, logTag.c_str(), "");
    if (!FileExist(filePath.c_str())) {
        LOG_ECMA(WARN) << logTag << " data file does not exist: " << filePath;
        return false;
    }
    MemMap fileMapMem = FileMap(filePath.c_str(), FILE_RDONLY, PAGE_PROT_READ);
    if (fileMapMem.GetOriginAddr() == nullptr) {
        LOG_ECMA(ERROR) << logTag << " File mmap failed";
        return false;
    }
#if defined(PANDA_TARGET_OHOS) && ENABLE_V70_OPTIMIZATION
    if (madvise(fileMapMem.GetOriginAddr(), fileMapMem.GetSize(), MADV_WILLNEED) != 0) {
        LOG_ECMA(INFO) << logTag << " madvise failed, errno: " << errno << ", " << strerror(errno);
    }
#endif
    LOG_ECMA(DEBUG) << logTag;
    MemMapScope memMapScope(fileMapMem);
    FileMemMapReader reader(fileMapMem, std::bind(RemoveFile, filePath), logTag);
    constexpr const uint32_t checksumSize = sizeof(uint32_t);
    const uint32_t contentSize = fileMapMem.GetSize() - checksumSize;
    uint32_t readCheckSum = 0;
    if (!reader.ReadFromOffset(&readCheckSum, checksumSize, contentSize, "checksum")) {
        return false;
    }
    const uint32_t checksum = adler32(0, static_cast<const Bytef*>(fileMapMem.GetOriginAddr()), contentSize);
    if (checksum != readCheckSum) {
        LOG_ECMA(ERROR) << logTag << " checksum compare failed, compute: " << checksum << ", written: "
                        << readCheckSum;
        return false;
    }

    if (auto err = ReadFileHeader(reader, header); !err.empty()) {
        LOG_ECMA(ERROR) << logTag << " file header check failed, error: " << err;
        return false;
    }

    if (!ReadDataInfo(reader, data)) {
        LOG_ECMA(ERROR) << logTag << " read serialize data info failed";
        return false;
    }
    if (data->incompleteData_) {
        LOG_ECMA(ERROR) << logTag << " has incompleteData: " << data->incompleteData_;
        return false;
    }

    if (g_isEnableCMCGC) {
        // read regularRemainSizeVectorSize
        size_t regularRemainSizeVectorSize;
        if (!reader.ReadSingleData(&regularRemainSizeVectorSize, sizeof(regularRemainSizeVectorSize),
                                   "regularRemainSizeVectorSize")) {
            return false;
        }
        // read regularRemainSizeVector
        size_t vecSize = regularRemainSizeVectorSize * sizeof(size_t);
        if (vecSize > 0) {
            data->regularRemainSizeVector_.resize(regularRemainSizeVectorSize);
            if (!reader.ReadSingleData(data->regularRemainSizeVector_.data(), vecSize, "regularRemainSizeVector")) {
                return false;
            }
        }
        // read pinRemainSizeVectorSize
        size_t pinRemainSizeVectorSize = 0;
        if (!reader.ReadSingleData(&pinRemainSizeVectorSize, sizeof(pinRemainSizeVectorSize),
                                   "pinRemainSizeVectorSize")) {
            return false;
        }
        // read pinRemainSizeVector
        vecSize = pinRemainSizeVectorSize * sizeof(size_t);
        if (vecSize > 0) {
            data->pinRemainSizeVector_.resize(pinRemainSizeVectorSize);
            if (!reader.ReadSingleData(data->pinRemainSizeVector_.data(), vecSize, "pinRemainSizeVector")) {
                return false;
            }
        }
    } else {
        // read vector size
        std::array<size_t, SERIALIZE_SPACE_NUM> vecSizes {};
        uint32_t vecSize = SERIALIZE_SPACE_NUM * sizeof(size_t);
        if (!reader.ReadSingleData(vecSizes.data(), vecSize, "vecSizes")) {
            return false;
        }
        // read each vector data
        for (int i = 0; i < SERIALIZE_SPACE_NUM; ++i) {
            auto& vec = data->regionRemainSizeVectors_[i];
            const size_t curVectorSize = vecSizes[i];
            const uint32_t curVectorDataSize = curVectorSize * sizeof(size_t);
            if (curVectorSize > 0) {
                vec.resize(curVectorSize);
                if (!reader.ReadSingleData(vec.data(), curVectorDataSize, "vec")) {
                    return false;
                }
            } else {
                vec.clear();
            }
        }
    }

    // read buffer data
    if (data->bufferSize_ > 0) {
        data->buffer_ = static_cast<uint8_t*>(malloc(data->bufferSize_));
        if (!data->buffer_) {
            return false;
        }
        if (!reader.ReadSingleData(data->buffer_, data->bufferSize_, "buffer")) {
            return false;
        }
    } else {
        data->buffer_ = nullptr;
    }

    LOG_ECMA(INFO) << logTag << " success";
    return true;
}

ModulesSnapshotHelper::SnapshotVersionInfo::UniquePtr ModulesSnapshotHelper::SnapshotVersionInfo::New(
    uint32_t appVersionCode, const std::string_view& romVersion, const std::string_view& description)
{
    size_t totalSize = Sizeof() + romVersion.size() + description.size() + TERMINATE_CHAR_COUNT;

    UniquePtr result(static_cast<SnapshotVersionInfo*>(malloc(totalSize)), SnapshotVersionInfo::Deleter);
    if (result == nullptr) {
        return result;
    }
    auto header = new (result.get()) SnapshotVersionInfo {appVersionCode, romVersion.size(), description.size()};
    char* buffer = header->buffer_;
    if (romVersion.size() > 0 && memcpy_s(buffer, romVersion.size(), romVersion.data(), romVersion.size()) != 0) {
        result.reset();
        LOG_ECMA(ERROR) << "memcpy_s failed when copy rom version to snapshot file header";
        return result;
    }
    buffer += romVersion.size();
    *(buffer++) = '\0';
    if (description.size() > 0 && memcpy_s(buffer, description.size(), description.data(), description.size())) {
        result.reset();
        LOG_ECMA(ERROR) << "memcpy_s failed when copy description to snapshot file header";
        return result;
    };
    buffer += description.size();
    *buffer = '\0';
    return result;
}

bool ModulesSnapshotHelper::DoNeedEscape()
{
    // snapshot feature is not loaded
    if (g_featureLoaded_ == static_cast<int>(SnapshotFeatureState::DEFAULT)) {
        return false;
    }
    // escaper is disabled, do not need to escape
    if (g_escaperDisabled_) {
        return false;
    }
    if (g_escaperTriggered_) {
        return false;
    }
    g_escaperTriggered_ = true;
    return true;
}

size_t ModulesSnapshotHelper::IntToString(int64_t value, char *buf, size_t bufSize)
{
    if (buf == nullptr || bufSize == 0) {
        return 0;
    }

    if (value == 0) {
        buf[0] = '0';
        return 1;
    }

    bool negative = (value < 0);
    uint64_t uVal = negative ? static_cast<uint64_t>(-value) : static_cast<uint64_t>(value);
    constexpr const int OCTET_BASE = 10;

    uint64_t temp = uVal;
    size_t digits = 0;
    while (temp != 0) {
        digits++;
        temp /= OCTET_BASE;
    }

    size_t needed = digits + (negative ? 1 : 0);
    if (needed > bufSize) {
        return 0;
    }

    char *p = buf + needed - 1;
    temp = uVal;
    while (temp != 0) {
        *(p--) = '0' + static_cast<char>(temp % OCTET_BASE);
        temp /= OCTET_BASE;
    }

    if (negative) {
        *p = '-';
    }

    return needed;
}

int ModulesSnapshotHelper::GetDisabledFeature(const CString &path)
{
    static std::once_flag flag;
    std::string_view cachedPath(g_stateFilePathBuffer_);
    if (!path.empty() && ((cachedPath.size() < MODULE_SNAPSHOT_STATE_FILE_NAME.size()) ||
                          (path != cachedPath.substr(0, cachedPath.size() - MODULE_SNAPSHOT_STATE_FILE_NAME.size())))) {
        UpdateFromStateFile(path);
    }
    return g_featureState_;
}

void ModulesSnapshotHelper::UpdateFromStateFile(const CString &path)
{
    constexpr static int disableAll =
        static_cast<int>(SnapshotFeatureState::PANDAFILE) | static_cast<int>(SnapshotFeatureState::MODULE);
    // snapshot feature is not enabled on current porocess
    if (!FileExist(path.c_str())) {
        g_featureState_ = disableAll;
        return;
    }
    auto stateFilePath = base::ConcatToCString(path, MODULE_SNAPSHOT_STATE_FILE_NAME);
    if (memset_s(g_stateFilePathBuffer_, PATH_MAX, 0, PATH_MAX) != EOK) {
        LOG_ECMA(WARN) << "memset_s failed when copy state file path";
    }
    if (memcpy_s(g_stateFilePathBuffer_, PATH_MAX, stateFilePath.c_str(), stateFilePath.length()) != EOK) {
        LOG_ECMA(WARN) << "memcpy_s failed when copy state file path";
    }
    // maybe snapshot is running well
    if (g_escaperDisabled_ || !FileExist(stateFilePath.c_str())) {
        return;
    }
    PosixFile stateFile(stateFilePath, "r");
    char stateWord{};
    if (stateFile.IsValid() && stateFile.Read(&stateWord, sizeof(stateWord)) <= 0) {
        g_featureState_ = disableAll;
        LOG_ECMA(WARN) << "module snapshot is disabled: invalid state file";
        return;
    }

    stateFile.Seek(1, PosixFile::SeekOrigin::CURRENT); // skip first ":"
    // Read reason
    std::string report = "unknown";
    report.resize(stateFile.Size());
    if (auto len = stateFile.Read(report); len > 0) {
        report.resize((len));
    }
    size_t sumLen = report.find("\n");
    auto summary = std::string_view(report.c_str(), sumLen != std::string::npos ? sumLen : report.size());
    std::string_view detail;
    if (sumLen != std::string::npos) {
        detail = std::string_view(summary.end() + 1, report.size() - sumLen - 1);
    }

    switch (stateWord) {
        case STATE_WORD_MODULE_SNAPSHOT_DISABLED:
            g_featureState_ |= static_cast<int>(SnapshotFeatureState::MODULE);
            LOG_ECMA(INFO) << "module snapshot is disabled: crash occurs. recent error: " << summary;
            LOG_ECMA(DEBUG)  << detail;
            break;
        case STATE_WORD_ALL_SNAPSHOT_DISABLED:
            g_featureState_ = disableAll;
            LOG_ECMA(INFO) << "module and pandafile snapshots are disabled: crash occurs. recent error: " << summary;
            LOG_ECMA(DEBUG)  << detail;
            break;
        default:
            // invalid state word, file maybe create by user or application
            g_featureState_ = disableAll;
            LOG_ECMA(WARN) << "module and pandafile snapshots are disabled: unexpected state word";
            break;
    }
}

SnapshotSaveTask::SnapshotSaveTask(int32_t id, std::unique_ptr<SerializeData> serializeData, const CString& path,
                                   ModulesSnapshotHelper::SnapshotVersionInfo::UniquePtr header,
                                   const CString& logPrefix)
    : Task(id), serializeData_(std::move(serializeData)), path_(path), header_(std::move(header)), logPrefix_(logPrefix)
{
}

bool SnapshotSaveTask::Run([[maybe_unused]] uint32_t threadIndex)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ModuleSnapshotTask", "");
    ModulesSnapshotHelper::WriteDataToFile(serializeData_, path_, header_, "SnapshotSaveTask");
    return true;
}

} // namespace panda::ecmascript
