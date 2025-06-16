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

#include "ecmascript/log_wrapper.h"
#include "ecmascript/platform/directory.h"
#include "ecmascript/platform/os.h"
#ifdef ENABLE_HISYSEVENT
#include "hisysevent.h"
#endif

namespace panda::ecmascript {

const std::string DOMAIN_FOR_DATA_PARTITION = "FILEMANAGEMENT";
const std::string EVENT_FOR_DATA_PARTITION = "USER_DATA_SIZE";
const std::string PARTITION_NAME = "/data";
const std::string COMPONENT_NAME = "ets_runtime";

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

int32_t AotCompilerStats::SendDataPartitionSysEvent(const std::string &path) const
{
#ifdef ENABLE_HISYSEVENT
    if (path.empty()) {
        return -1;
    }
    LOG_ECMA(INFO) << "send data partition sys";
    int32_t ret = HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::FILEMANAGEMENT,
        "USER_DATA_SIZE",
        OHOS::HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "COMPONENT_NAME", COMPONENT_NAME,
        "PARTITION_NAME", PARTITION_NAME,
        "REMAIN_PARTITION_SIZE", panda::ecmascript::GetDeviceValidSize(PARTITION_NAME),
        "FILE_OR_FOLDER_PATH", path,
        "FILE_OR_FOLDER_SIZE", panda::ecmascript::GetFolderSize(path));
    if (ret != 0) {
        LOG_ECMA(ERROR) << "AotCompilerStats SendDataPartitionSysEvent Failed! ret = " << ret;
    }
    return ret;
#endif
    return 0;
}

} // namespace panda::ecmascript