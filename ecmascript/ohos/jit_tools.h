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

#include "ecmascript/ohos/enable_aot_list_helper.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/platform/aot_crash_info.h"

#if defined(JIT_ESCAPE_ENABLE) || defined(GET_PARAMETER_FOR_JIT) || defined(JIT_ENABLE_CODE_SIGN)
#include "base/startup/init/interfaces/innerkits/include/syspara/parameters.h"
#endif
#if defined(JIT_ENABLE_CODE_SIGN)
#include "jit_buffer_integrity.h"
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

    static uint16_t GetJitCallThreshold(uint16_t threshold)
    {
    #ifdef GET_PARAMETER_FOR_JIT
        return OHOS::system::GetUintParameter("ark.jit.call.threshold", threshold);
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

    static bool GetCodeSignDisable(bool value)
    {
    #ifdef JIT_ENABLE_CODE_SIGN
        return OHOS::system::GetBoolParameter("persist.ark.jit.codesign.disable", false) ||
               !OHOS::Security::CodeSign::IsSupportJitCodeSigner();
    #endif
        return value;
    }

    static bool IsSupportJitCodeSigner()
    {
    #ifdef JIT_ENABLE_CODE_SIGN
        return OHOS::Security::CodeSign::IsSupportJitCodeSigner();
    #endif
        return false;
    }

    static bool GetEnableJitFort(bool value)
    {
    #ifdef GET_PARAMETER_FOR_JIT
        return OHOS::system::GetBoolParameter("persist.ark.jit.enable.jitfort", true);
    #endif
        return value;
    }

    static bool GetEnableJitVerifyPass(bool value)
    {
    #ifdef GET_PARAMETER_FOR_JIT
        return OHOS::system::GetBoolParameter("persist.ark.jit.enable.jitverify", true);
    #endif
        return value;
    }

    static bool GetEnableAsyncCopyToFort(bool value)
    {
    #ifdef GET_PARAMETER_FOR_JIT
        return OHOS::system::GetBoolParameter("persist.ark.jit.enable.async.copytofort", true);
    #endif
        return value;
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
