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

#include "ecmascript/mem/c_string.h"

#include <climits>
#include <string_view>

namespace panda::ecmascript {
enum class SnapshotFeatureState : int8_t {
    DEFAULT = 0,
    PANDAFILE = 1 << 0,
    MODULE = 1 << 1,
};

class ModulesSnapshotHelper {
public:

    static void RegisterSignalHandler();
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
    static void MarkModuleSnapshotLoaded();
    static void MarkJSPandaFileSnapshotLoaded();
    static void MarkSnapshotDisabledByOption();
    /**
     * @brief Try to disable snapshot feature
     * @param reason reason to disable snapshot, negative value means unknown reason,
     *               0 means uncaught exception, others means signal number
     */
    static void TryDisableSnapshot(int reason = -1);
    static void DisableSnapshotEscaper();
    static void RemoveSnapshotFiles(const CString &path);

private:
    static constexpr std::string_view MODULE_SNAPSHOT_STATE_FILE_NAME = "ArkModuleSnapshot.state";
    static constexpr std::string_view SNAPSHOT_FILE_SUFFIX = ".ams";
    static constexpr std::string_view DISABLE_REASON_UNCAUGHT_EXCEPTION = "UEC";
    static constexpr std::string_view DISABLE_REASON_SIGNAL = "SIG";
    static constexpr std::string_view DISABLE_REASON_UNKNOWN = "UNK";
    static constexpr char STATE_WORD_MODULE_SNAPSHOT_DISABLED = '1';
    static constexpr char STATE_WORD_ALL_SNAPSHOT_DIASBLED = '0';

    static size_t IntToString(int value, char *buf, size_t bufSize);
    static void UpdateFromStateFile(const CString &path);
    static bool SigchainHandler(int signo, void *info, void *ucontext);

    // cached path of state file, which is needed in signal handler
    static char g_stateFilePathBuffer_[PATH_MAX];
    static bool g_escaperDisabled_;
    static int g_featureState_;
    static int g_featureLoaded_;
};
} // namespace panda::ecmascript

#endif // ECMASCRIPT_SNAPSHOT_COMMON_MODULES_SNAPSHOT_HELPER_H