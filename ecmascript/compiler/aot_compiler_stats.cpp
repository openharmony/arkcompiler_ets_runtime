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

#include "ecmascript/compiler/aot_compiler_stats.h"

#ifdef ENABLE_HISYSEVENT
#include "hisysevent.h"
#endif

namespace panda::ecmascript {

void AotCompilerStats::PrintCompilerStatsLog()
{
    EndCompiler();
    LOG_GC(ERROR) << "AotComiler LOG\n "
        << "bundleName: " << GetBundleName() << "\n"
        << "pgoFilePath: " << GetPgoPath() << "\n"
        << "aotFilePath: " << GetAotFilePath() << "\n"
        << "totalTile: " << GetTotalTime() << "\n"
        << "isLiteCg: " << IsLiteCg() << "\n"
        << "compilerMethodCount: " <<  GetCompilerMethodCount() << "\n"
        << "pgoFileLegal: " << GetPgoFileLegal() << "\n";
    if (!GetPgoFileLegal() || IsLongTimeCompiler()) {
        SendSysEvent();
    }
}

void AotCompilerStats::SendSysEvent() const
{
#ifdef ENABLE_HISYSEVENT
    LOG_GC(ERROR) << "send sys event";
    int32_t ret = HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::ARKTS_RUNTIME,
        "ARK_COMPILER_LOG",
        OHOS::HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "BUNDLE_NAME", GetBundleName(),
        "PGO_FILE_PATH", GetPgoPath(),
        "AOT_FILE_PATH", GetAotFilePath(),
        "COMPILER_TIME", GetTotalTime(),
        "IS_LITECG", IsLiteCg(),
        "COMPILER_METHOD_COUNT", GetCompilerMethodCount(),
        "PGO_FILE_LEGAL", GetPgoFileLegal());
    if (ret != 0) {
        LOG_GC(ERROR) << "AotCompilerStats HiSysEventWrite Failed! ret = " << ret;
    }
#endif
}

} // namespace panda::ecmascript