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
#ifndef ECMASCRIPT_JIT_TOOLS_H
#define ECMASCRIPT_JIT_TOOLS_H

#include "ecmascript/ohos/aot_crash_info.h"
#include "ecmascript/ohos/enable_aot_list_helper.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"

#if defined(JIT_ESCAPE_ENABLE) || defined(GET_PARAMETER_FOR_JIT)
#include "base/startup/init/interfaces/innerkits/include/syspara/parameters.h"
#endif

namespace panda::ecmascript::ohos {
using PGOProfilerManager = panda::ecmascript::pgo::PGOProfilerManager;
class JitTools {
public:

    static JitTools &GetInstance()
    {
        static JitTools singleJitTools;
        return singleJitTools;
    }

    void SetJitEnable(EcmaVM *vm, const std::string &bundleName)
    {
        JSRuntimeOptions &options = vm->GetJSOptions();
        options.SetEnableJitFrame(GetJitFrameEnable());
        vm->SetProcessStartRealtime(vm->InitializeStartRealTime());
        bool jitEscapeDisable = ohos::JitTools::GetJitEscapeDisable();
        if (jitEscapeDisable || !AotCrashInfo::IsJitEscape()) {
            if (ohos::EnableAotJitListHelper::GetInstance()->IsEnableJit(bundleName)) {
                bool isEnableFastJit = options.IsEnableJIT() && options.GetEnableAsmInterpreter();
                bool isEnableBaselineJit = options.IsEnableBaselineJIT() && options.GetEnableAsmInterpreter();
                options.SetEnableAPPJIT(true);
                // for app threshold
                uint32_t defaultSize = 150;
                uint32_t threshold = GetJitHotnessThreshold(defaultSize);
                options.SetJitHotnessThreshold(threshold);
                Jit::GetInstance()->SetEnableOrDisable(options, isEnableFastJit, isEnableBaselineJit);
                if (isEnableFastJit || isEnableBaselineJit) {
                    EnableJit(vm);
                }
            }
        }
    }

    void EnableJit(EcmaVM *vm) const
    {
        JSThread *thread = vm->GetAssociatedJSThread();
        JSRuntimeOptions &options = vm->GetJSOptions();
        std::shared_ptr<PGOProfiler> pgoProfiler = vm->GetPGOProfiler();
        if (!options.IsEnableJITPGO() || pgoProfiler == nullptr) {
            thread->SwitchJitProfileStubs(false);
        } else {
            // if not enable aot pgo
            if (!PGOProfilerManager::GetInstance()->IsEnable()) {
                // disable dump
                options.SetEnableProfileDump(false);
                Jit::GetInstance()->SetProfileNeedDump(false);
                // enable profiler
                options.SetEnablePGOProfiler(true);
                pgoProfiler->Reset(true);
                // switch pgo stub
                thread->SwitchJitProfileStubs(true);
            }
            pgoProfiler->InitJITProfiler();
        }

        bool isApp = Jit::GetInstance()->IsAppJit();
        options.SetEnableAPPJIT(isApp);
        bool profileNeedDump = Jit::GetInstance()->IsProfileNeedDump();
        options.SetEnableProfileDump(profileNeedDump);

        bool jitEnableLitecg = ohos::JitTools::IsJitEnableLitecg(options.IsCompilerEnableLiteCG());
        options.SetCompilerEnableLiteCG(jitEnableLitecg);
        uint8_t jitCallThreshold = ohos::JitTools::GetJitCallThreshold(options.GetJitCallThreshold());
        options.SetJitCallThreshold(jitCallThreshold);

        uint32_t jitHotnessThreshold = Jit::GetInstance()->GetHotnessThreshold();
        options.SetJitHotnessThreshold(jitHotnessThreshold);
        LOG_JIT(INFO) << "jit enable litecg:" << jitEnableLitecg << ", call threshold:" <<
            static_cast<int>(jitCallThreshold) << ", hotness threshold:" << jitHotnessThreshold;

        bool jitDisableCodeSign = ohos::JitTools::GetCodeSignDisable();
        options.SetDisableCodeSign(jitDisableCodeSign);
        LOG_JIT(INFO) << "jit disable codesigner:" << jitDisableCodeSign;

        vm->SetEnableJitLogSkip(ohos::JitTools::GetSkipJitLogEnable());
    }

    static bool GetJitEscapeDisable()
    {
    #ifdef JIT_ESCAPE_ENABLE
        return OHOS::system::GetBoolParameter("ark.jit.escape.disable", false);
    #endif
        return true;
    }

    static bool IsJitEnableLitecg(bool value)
    {
    #ifdef GET_PARAMETER_FOR_JIT
        return OHOS::system::GetBoolParameter("ark.jit.enable.litecg", true);
    #endif
        return value;
    }

    static uint32_t GetJitHotnessThreshold([[maybe_unused]] uint32_t threshold)
    {
    #ifdef GET_PARAMETER_FOR_JIT
        return OHOS::system::GetUintParameter("ark.jit.hotness.threshold", threshold);
    #endif
        return threshold;
    }

    static uint8_t GetJitCallThreshold(uint8_t threshold)
    {
    #ifdef GET_PARAMETER_FOR_JIT
        return OHOS::system::GetUintParameter("ark.jit.call.threshold", static_cast<uint8_t>(0));
    #endif
        return threshold;
    }

    static bool GetJitDumpObjEanble()
    {
    #ifdef JIT_ESCAPE_ENABLE
        return OHOS::system::GetBoolParameter("ark.jit.enable.dumpobj", false);
    #endif
        return false;
    }

    static bool GetJitFrameEnable()
    {
    #ifdef JIT_ESCAPE_ENABLE
        return OHOS::system::GetBoolParameter("ark.jit.enable.jitframe", false);
    #endif
        return false;
    }

    static bool GetCodeSignDisable()
    {
    #ifdef CODE_SIGN_ENABLE
        return OHOS::system::GetBoolParameter("ark.jit.codesign.disable", false);
    #endif
        return false;
    }

    static bool GetSkipJitLogEnable()
    {
    #ifdef GET_PARAMETER_FOR_JIT
        return OHOS::system::GetBoolParameter("ark.jit.enable.jitLogSkip", true);
    #endif
        // host no need skip jit log
        return false;
    }
};
}
#endif  // ECMASCRIPT_JIT_TOOLS_H
