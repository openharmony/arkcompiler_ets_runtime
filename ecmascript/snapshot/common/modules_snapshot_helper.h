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

#ifndef ECMASCRIPT_SNAPSHOT_COMMON_MODULES_SNAPSHOT_HELPER_H
#define ECMASCRIPT_SNAPSHOT_COMMON_MODULES_SNAPSHOT_HELPER_H

#include "common_components/taskpool/task.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/platform/file.h"

#include <climits>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

namespace panda::ecmascript {
class JSThread;
class EcmaVM;
class SerializeData;

enum class SnapshotFeatureState : int8_t {
    DEFAULT = 0,
    PANDAFILE = 1 << 0,
    MODULE = 1 << 1,
};

class ModulesSnapshotHelper {
public:
    struct alignas(sizeof(uint64_t)) SnapshotVersionInfo {
        // SnapshotVersionInfo layout
        // +--------------------------------------+
        // |       Application Version Code       |
        // +--------------------------------------+
        // |      System Version Code Length      |
        // |      Description Length              |
        // +--------------------------------------+ <- data
        // |      System Version Code             |
        // |      Description                     |
        // +--------------------------------------+
        static constexpr const size_t TERMINATE_CHAR_COUNT = 2;
        static constexpr size_t Sizeof() { return offsetof(SnapshotVersionInfo, buffer_); }

        static void Deleter(void* ptr)
        {
            if (ptr != nullptr) {
                free(ptr);
            }
        }

        using UniquePtr = std::unique_ptr<SnapshotVersionInfo, decltype(&Deleter)>;
        static UniquePtr New(uint32_t appVersionCode, const std::string_view& romVersion,
                                        const std::string_view& description);
        void Delete() { free(this); }

        template <typename T>
        static const SnapshotVersionInfo* FromBuffer(const T* buffer)
        {
            return reinterpret_cast<const SnapshotVersionInfo*>(buffer);
        }

        size_t Size() const { return Sizeof() + romVerLength_ + descLength_ + TERMINATE_CHAR_COUNT; }
        std::string_view GetRomVersion() const { return std::string_view(buffer_, romVerLength_); }
        std::string_view GetDescription() const { return std::string_view(buffer_ + romVerLength_ + 1, descLength_); }

        uint32_t appVersionCode_ {0};
        size_t romVerLength_ {0};
        size_t descLength_ {0};
        // fixed char array for `romVersion` and `description`;
        char buffer_[];
    };

    class FileGuard {
    public:
        FileGuard(const CString &path, uint32_t tid);
        ~FileGuard();
        bool Done();
        const CString &GetTempPath() const
        {
            return temp_;
        }
    private:
        CString target_ {};
        CString temp_ {};
        bool done_ {false};
    };

    static void InitEscaper(EcmaVM *vm);
    static int GetDisabledFeature(const CString &path);
    inline static bool IsPandafileSnapshotDisabled(const CString &path)
    {
        return (ModulesSnapshotHelper::GetDisabledFeature(path) &
                   static_cast<int>(SnapshotFeatureState::PANDAFILE)) != 0;
    }
    inline static bool IsModuleSnapshotDisabled(const CString &path)
    {
        return (ModulesSnapshotHelper::GetDisabledFeature(path) &
                   static_cast<int>(SnapshotFeatureState::MODULE)) != 0;
    }
    // ConstPoolSnapshot currently shares the same feature policy as ModuleSnapshot.
    inline static bool IsConstPoolSnapshotDisabled(const CString &path)
    {
        return IsModuleSnapshotDisabled(path);
    }
    static void MarkModuleSnapshotLoaded();
    static void MarkJSPandaFileSnapshotLoaded();
    static void MarkSnapshotDisabledByOption();
    inline static void MarkConstPoolSnapshotLoaded()
    {
        MarkModuleSnapshotLoaded();
    }
    /**
     * @brief Try to disable snapshot feature
     * @param reason reason to disable snapshot, number less than 0 means unknown reason,
     */
    static bool TryDisableSnapshot(int reason = -1);
    static void TryDisableSnapshot(JSThread* thread);
    static void TryDisableSnapshotOnANR();
    static void DisableSnapshotEscaper();

    static void RemoveSnapshotFiles(const CString& path);
    static void RemoveFile(const CString& path);
    static void HandleCorruptedFile(const CString& dir, const CString& endsWith);
    static bool SetReadOnly(const std::string_view& path, std::string* errorMsg = nullptr);

    static bool WriteDataToFile(const std::unique_ptr<SerializeData>& data, const CString& filePath,
                                const SnapshotVersionInfo::UniquePtr& header, const CString& logPrefix);
    static bool ReadDataFromFile(const std::unique_ptr<SerializeData>& data, const CString& filePath,
                                 const SnapshotVersionInfo::UniquePtr& header, const CString& logPrefix);

    static size_t GetAlignUpPadding(const uint8_t* curPtr, void* originAddr, const size_t alignment)
    {
        const auto originPtr = static_cast<uint8_t*>(originAddr);
        return AlignUp(curPtr - originPtr, alignment) - (curPtr - originPtr);
    }

    static bool WriteFileHeader(FileMemMapWriter& writer, const SnapshotVersionInfo::UniquePtr &header);
    static std::string ReadFileHeader(FileMemMapReader& reader, const SnapshotVersionInfo::UniquePtr &header);

private:
    struct alignas(sizeof(uint64_t)) SerializeDataInfo {
        uint32_t dataIndex_;
        uint64_t sizeLimit_;
        size_t bufferSize_;
        size_t bufferCapacity_;
        size_t regularSpaceSize_;
        size_t pinSpaceSize_;
        size_t oldSpaceSize_;
        size_t nonMovableSpaceSize_;
        size_t machineCodeSpaceSize_;
        size_t sharedOldSpaceSize_;
        size_t sharedNonMovableSpaceSize_;
        bool incompleteData_;
    };

    static constexpr std::string_view MODULE_SNAPSHOT_STATE_FILE_NAME = "ArkModuleSnapshot.state";
    static constexpr std::string_view SNAPSHOT_FILE_SUFFIX = ".ams";
    static constexpr std::string_view BACKUP_FILE_SUFFIX = ".bak";
    static constexpr std::string_view FILE_MODE_READONLY = "r";
    static constexpr std::string_view DISABLE_APPLICATION_NOT_RESPONDING = "APPLICATION_NOT_RESPONDING";
    static constexpr std::string_view DISABLE_REASON_UNCAUGHT_EXCEPTION = "UNCAUGHT_EXCEPTION";
    static constexpr std::string_view DISABLE_REASON_SIGNAL = "SIGNAL";
    static constexpr std::string_view DISABLE_REASON_UNKNOWN = "UNKNOWN";
    static constexpr char STATE_WORD_MODULE_SNAPSHOT_DISABLED = '1';
    static constexpr char STATE_WORD_ALL_SNAPSHOT_DISABLED = '0';
    // const val for serialize data
    static constexpr int CMC_GC_REGION_SIZE = 2;

    // utils for escaper
    static bool TryDisableSnapshot(const std::string_view& reason, const std::string_view& extraInfo = "");
    static bool DoNeedEscape();
    static size_t IntToString(int64_t value, char *buf, size_t bufSize);
    static void UpdateFromStateFile(const CString &path);
    static bool SigchainHandler(int signo, void *info, void *ucontext);

    // utils for snapshot file read/write
    static bool WriteDataInfo(FileMemMapWriter& writer, const std::unique_ptr<SerializeData>& data);
    static bool ReadDataInfo(FileMemMapReader& reader, const std::unique_ptr<SerializeData>& data);

    // cached path of state file, which is needed in signal handler
    static char g_stateFilePathBuffer_[PATH_MAX];
    static bool g_escaperDisabled_;
    static int g_featureState_;
    static int g_featureLoaded_;
    static volatile bool g_escaperTriggered_;
};

class SnapshotSaveTask : public common::Task {
public:
    SnapshotSaveTask(int32_t id, std::unique_ptr<SerializeData> serializeData, const CString& path,
                     ModulesSnapshotHelper::SnapshotVersionInfo::UniquePtr header, const CString& logPrefix);
    bool Run(uint32_t threadIndex) override;

    NO_COPY_SEMANTIC(SnapshotSaveTask);
    NO_MOVE_SEMANTIC(SnapshotSaveTask);

private:
    std::unique_ptr<SerializeData> serializeData_ {};
    CString path_ {};
    ModulesSnapshotHelper::SnapshotVersionInfo::UniquePtr header_;
    CString logPrefix_ {};
};

} // namespace panda::ecmascript

#endif // ECMASCRIPT_SNAPSHOT_COMMON_MODULES_SNAPSHOT_HELPER_H
