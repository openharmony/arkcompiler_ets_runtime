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

#include "ecmascript/base/string_helper.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/platform/filesystem.h"
#include "ecmascript/platform/signal_manager.h"

#include <csignal>
#include <mutex>
#include <vector>

namespace panda::ecmascript {
bool ModulesSnapshotHelper::g_escaperDisabled_ = false;
int ModulesSnapshotHelper::g_featureState_ = static_cast<int>(SnapshotFeatureState::DEFAULT);
int ModulesSnapshotHelper::g_featureLoaded_ = static_cast<int>(SnapshotFeatureState::DEFAULT);
char ModulesSnapshotHelper::g_stateFilePathBuffer_[PATH_MAX] = {};

void ModulesSnapshotHelper::RegisterSignalHandler()
{
    static std::once_flag flag{};
    std::call_once(flag, []() {
        // feature is not enabled, do not need to watch
        if ((g_featureState_ & static_cast<int>(SnapshotFeatureState::MODULE)) != 0 &&
            (g_featureState_ & static_cast<int>(SnapshotFeatureState::PANDAFILE)) != 0) {
            return;
        }

        std::vector registeredSignos{
#ifdef SIGSEGV
            SIGSEGV,
#endif
#ifdef SIGBUS
            SIGBUS,
#endif
#ifdef SIGABRT
            SIGABRT,
#endif
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

bool ModulesSnapshotHelper::SigchainHandler(int signo, void *info, void *ucontext)
{
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

void ModulesSnapshotHelper::TryDisableSnapshot(int reason)
{
    // snapshot feature is not loaded
    if (g_featureLoaded_ == static_cast<int>(SnapshotFeatureState::DEFAULT)) {
        return;
    }

    PosixFile stateFile(g_stateFilePathBuffer_, "w");
    std::string_view disableWord = "";
    // it means both snapshot feature is enabled at least during the process running,
    // disable module at first
    if ((g_featureLoaded_ & static_cast<int>(SnapshotFeatureState::MODULE)) != 0 &&
        (g_featureLoaded_ & static_cast<int>(SnapshotFeatureState::PANDAFILE)) != 0) {
        disableWord = &STATE_WORD_MODULE_SNAPSHOT_DISABLED;
    } else {
        // only one snapshot feature is loaded
        disableWord = &STATE_WORD_ALL_SNAPSHOT_DIASBLED;
    }

    stateFile.Write(disableWord);
    stateFile.Write(":", 1);
    if (reason == 0) {
        LOG_ECMA(WARN) << "js crash occurred, try to disable snapshot";
        stateFile.Write(DISABLE_REASON_UNCAUGHT_EXCEPTION);
        return;
    }
    if (reason > 0) {
        stateFile.Write(DISABLE_REASON_SIGNAL);
        constexpr size_t SIG_MAX_DIGITS = 2;
        char sigBuf[SIG_MAX_DIGITS]{};
        auto written = IntToString(reason, sigBuf, SIG_MAX_DIGITS);
        stateFile.Write(sigBuf, written);
        return;
    }
    stateFile.Write(DISABLE_REASON_UNKNOWN);
}

void ModulesSnapshotHelper::DisableSnapshotEscaper() { g_escaperDisabled_ = true; }

void ModulesSnapshotHelper::RemoveSnapshotFiles(const CString &path)
{
    if (FileExist(path.data())) {
        DeleteFilesWithSuffix(path.data(), SNAPSHOT_FILE_SUFFIX.data());
    }
}

size_t ModulesSnapshotHelper::IntToString(int value, char *buf, size_t bufSize)
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
    if (path != std::string_view(g_stateFilePathBuffer_) && !path.empty()) {
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
    if (memcpy_s(g_stateFilePathBuffer_, PATH_MAX, stateFilePath.c_str(), stateFilePath.length()) != EOK) {
        LOG_ECMA(WARN) << "memcpy_s failed when copy state file path";
    }
    if (g_escaperDisabled_) {
        return;
    }
    // maybe snapshot is running well
    if (!FileExist(stateFilePath.c_str())) {
        return;
    }
    PosixFile stateFile(stateFilePath, "r");
    char stateWord{};
    if (stateFile.IsValid() && stateFile.Read(&stateWord, sizeof(stateWord)) <= 0) {
        return;
    }
    switch (stateWord) {
        case STATE_WORD_MODULE_SNAPSHOT_DISABLED:
            g_featureState_ = static_cast<int>(SnapshotFeatureState::MODULE);
            LOG_ECMA(WARN) << "module snapshot is disabled due to crash occurs";
            break;
        case STATE_WORD_ALL_SNAPSHOT_DIASBLED:
            g_featureState_ = disableAll;
            LOG_ECMA(WARN) << "module and pandafile snapshots is disabled due to crash occurs";
            break;
        default:
            // invalid state word, file maybe create by user or application
            g_featureState_ = disableAll;
            LOG_ECMA(WARN) << "module and pandafile snapshots is disabled due to unexpected state word";
            break;
    }
}

} // namespace panda::ecmascript