/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/jit/jit_dfx.h"
#include "ecmascript/runtime.h"
#include "libpandafile/bytecode_instruction-inl.h"
#include "libpandafile/code_data_accessor.h"
#include "libpandafile/class_data_accessor-inl.h"
#include "ecmascript/platform/file.h"

#ifdef ENABLE_HISYSEVENT
#include "hisysevent.h"
#endif

namespace panda::ecmascript {
JitDfx JitDfx::instance;
thread_local uint32_t JitDfx::prefixOffset_ = 0;
class NullStream : public std::ostream {
public:
    NullStream() : std::ostream(&buffer_) {}

private:
    class NullBuffer : public std::streambuf {
    public:
        int overflow(int c) override
        {
            return c;
        }
    };
    NullBuffer buffer_;
};

JitDfx *JitDfx::GetInstance()
{
    return &instance;
}

void JitDfx::Init(const JSRuntimeOptions &options, std::string &bundleName)
{
    if (options.IsEnableJitDfxDump()) {
        EnableDump();
    }
    ResetCompilerTime();
    ResetBlockUIEventTime();
    SetBundleName(ConvertToString(bundleName));
    // main thread
    SetPidNumber(JSThread::GetCurrentThreadId());
}

void JitDfx::EnableDump()
{
    isEnableDump_ = true;
}

void JitDfx::OpenLogFile(uint32_t threadId)
{
#ifdef PANDA_TARGET_OHOS
    CString path = CString("/data/storage/ark-profile/jit_dfx_") + ToCString(threadId) + CString(".log");
#else
    CString path = CString("jit_dfx_") + ToCString(threadId) + CString(".log");
#endif
    std::string realOutPath;
    if (!ecmascript::RealPath(path.c_str(), realOutPath, false)) {
        return;
    }
    logFiles_[threadId].open(realOutPath, std::ios::out);
}

std::ostream &JitDfx::GetLogFileStream()
{
    if (!isEnableDump_) {
        static NullStream nullStream_;
        return nullStream_;
    }
    uint32_t threadId = os::thread::GetCurrentThreadId();
    auto it = logFiles_.find(threadId);
    if (it == logFiles_.end()) {
        OpenLogFile(threadId);
    }
    return logFiles_[threadId];
}

void JitDfx::DumpBytecodeInst(JSThread *thread, Method *method)
{
    if (!isEnableDump_) {
        return;
    }
    CString methodInfo = method->GetRecordNameStr(thread) + "." + CString(method->GetMethodName(thread));
    MethodLiteral *methodLiteral = method->GetMethodLiteral(thread);
    auto jsPandaFile = method->GetJSPandaFile(thread);
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    ASSERT(methodLiteral != nullptr);
    panda_file::File::EntityId methodIdx = methodLiteral->GetMethodId();
    panda_file::MethodDataAccessor mda(*pf, methodIdx);
    auto codeId = mda.GetCodeId();
    panda_file::CodeDataAccessor codeDataAccessor(*pf, codeId.value());
    uint32_t codeSize = codeDataAccessor.GetCodeSize();
    const uint8_t *insns = codeDataAccessor.GetInstructions();

    std::ostringstream ss;
    ss << "BytecodeInst func:" << methodInfo << "\n";
    auto bcIns = BytecodeInst(insns);
    auto bcInsLast = bcIns.JumpTo(codeSize);
    while (bcIns.GetAddress() != bcInsLast.GetAddress()) {
        ss << bcIns << std::endl;
        auto nextInst = bcIns.GetNext();
        bcIns = nextInst;
    }

    GetLogFileStream() << ss.str() << std::endl;
}

void JitDfx::TraceJitCode(JSThread *thread, Method *method, bool isEntry)
{
    if (!isEnableDump_) {
        return;
    }
    if (!isEntry) {
        prefixOffset_ -= 1;
    }
    CString prefixStr = isEntry ? CString("JitCodeEntry:") : CString("JitCodeExit :");
    CString methodInfo = method->GetRecordNameStr(thread) + "." + CString(method->GetMethodName(thread));
    static CString blackSpace("  ");
    CString prefix;
    for (uint32_t i = 0; i < prefixOffset_; i++) {
        prefix += blackSpace;
    }
    if (isEntry) {
        prefixOffset_ += 1;
    }
    LOG_JIT(INFO) << prefixStr << prefix << methodInfo;
}

void JitDfx::PrintJitStatsLog()
{
    if (checkUploadConditions()) {
        LOG_JIT(DEBUG) << "Jit Compiler stats Log: "
        << " bundleName: " << GetBundleName()
        << " pid: " << GetPidNumber()
        << " total main thread time: " << GetTotalTimeOnMainThread()
        << " total Jit thread time: " << GetTotalTimeOnJitThread()
        << " total Baseline Jit times: " << GetTotalBaselineJitCount()
        << " total Fastopt Jit times: " << GetTotalFastoptJitCount()
        << " report time interval:"
        << std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - jitEventParams.start_).count()
        << " total time on hold lock: " << GetTotalLockHoldingTime()
        << " max time on hold lock: " << GetMaxLockHoldingTime()
        << " longtime of hold lock: " << GetLongtimeLockCount()
        << " JitDeopt times: " << GetJitDeoptCount()
        << "\n";
        SendJitStatsEvent();
        InitializeRecord();
    }
}

void JitDfx::SendJitStatsEvent() const
{
#ifdef ENABLE_HISYSEVENT
    int32_t ret = HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::ARKTS_RUNTIME,
        "ARK_STATS_JIT",
        OHOS::HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "BUNDLE_NAME", ConvertToStdString(GetBundleName()),
        "PID", GetPidNumber(),
        "TIME_INTERVAL",
        std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - jitEventParams.start_).count(),
        "TOTAL_BASELINE_JIT_TIMES", GetTotalBaselineJitCount(),
        "TOTAL_FASTOPT_JIT_TIMES", GetTotalFastoptJitCount(),
        "TOTAL_TIME_ON_MAIN_THREAD", GetTotalTimeOnMainThread(),
        "TOTAL_TIME_ON_JIT_THREAD", GetTotalTimeOnJitThread(),
        "TOTAL_TIME_ON_HOLD_LOCK", GetTotalLockHoldingTime(),
        "MAX_TIME_ON_HOLD_LOCK", GetMaxLockHoldingTime(),
        "LONG_TIME_OF_HOLD_LOCK", GetLongtimeLockCount(),
        "UNINSTALL_TIME", GetJitDeoptCount());
    if (ret != 0) {
        LOG_JIT(ERROR) << "Jit Compiler Stats send stats event failed! ret = " << ret;
    }
#endif
}

void JitDfx::PrintJitBlockUILog()
{
    std::string jitType = isBaselineJit_ ? "Baseline JIT" : "Fast optimization JIT";
    LOG_JIT(DEBUG) << "Jit BlockUI Event Log: "
        << " bundleName: " << GetBundleName()
        << " pid: " << GetPidNumber()
        << " Single main thread time: " << GetSingleTimeOnMainThread()
        << " Single Jit thread time: " << GetSingleTimeOnJitThread()
        << " Jit type: " << jitType
        << " method info: " << GetMethodInfo()
        << "\n";
    SendJitBlockUIEvent();
    InitializeBlockUIRecord();
}

void JitDfx::SendJitBlockUIEvent() const
{
#ifdef ENABLE_HISYSEVENT
    std::string jitType = isBaselineJit_ ? "Baseline JIT" : "Fast optimization JIT";
    int32_t ret = HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::ARKTS_RUNTIME,
        "ARK_BLOCKUI_JIT",
        OHOS::HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "BUNDLE_NAME", ConvertToStdString(GetBundleName()),
        "PID", GetPidNumber(),
        "JIT_TYPE", jitType,
        "JIT_FUNCTION_NAME", ConvertToStdString(GetMethodInfo()),
        "TIME_ON_MAIN_THREAD", GetSingleTimeOnMainThread(),
        "TIME_ON_JIT_THREAD", GetSingleTimeOnJitThread());
    if (ret != 0) {
        LOG_JIT(ERROR) << "Jit Compiler Stats send jit blockUI event failed! ret = " << ret;
    }
#endif
}

void JitDfx::InitializeRecord()
{
    jitEventParams.totalBaselineJitTimes_.store(0);
    jitEventParams.totalFastoptJitTimes_.store(0);
    jitEventParams.jitDeoptTimes_.store(0);
    jitEventParams.longtimeLockTimes_.store(0);
    jitEventParams.totalTimeOnMainThread_.store(0);
    jitEventParams.totalTimeOnJitThread_.store(0);
    jitEventParams.totalLockHoldingTime_.store(0);
    jitEventParams.maxLockHoldingTime_ .store(0);
    ResetCompilerTime();
}

void JitDfx::InitializeBlockUIRecord()
{
    jitEventParams.singleTimeOnMainThread_.store(0);
    jitEventParams.singleTimeOnJitThread_.store(0);
    isBaselineJit_ = true;
    methodInfo_ = "";
    ResetBlockUIEventTime();
}
}  // namespace panda::ecmascript
