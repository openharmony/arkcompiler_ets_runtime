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

#include "prof_dump_json.h"
#include "prof_convert_json.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"

namespace native {
using namespace panda::ecmascript::pgo;
static std::string g_result = "";

size_t ConvertApToJson(const char *path, const size_t pathSize)
{
    std::string apFilePath(path, pathSize);
    PGOProfilerDecoder decoder(apFilePath, 1);
    if (!decoder.LoadFull()) {
        return 0;
    }
    // parse recordDetailInofo
    std::vector<ProfileType::jModuleType> modules;
    auto recordDetailInfos = decoder.GetRecordDetailInfosPtr();
    auto recordInfos = recordDetailInfos->GetRecordInfos();
    auto recordPool = recordDetailInfos->GetRecordPool();
    for (auto iter = recordInfos.begin(); iter != recordInfos.end(); iter++) {
        std::string moduleName(recordPool->GetName(iter->first));
        if (moduleName.empty()) {
            continue;
        }
        ProfileType::jModuleType oneModule;
        oneModule.insert(std::make_pair(DumpJsonUtils::MODULE_NAME, moduleName));
        iter->second->ProcessToJson(1, oneModule);
        modules.push_back(oneModule);
    }
    // parse abcFilePool
    cJSON *allMessage = cJSON_CreateObject();
    if (allMessage == nullptr) {
        return 0;
    }

    JsonConverter convert;
    cJSON* recordDetailInofo = convert.Convert(modules);
    cJSON_AddItemToObject(allMessage, DumpJsonUtils::RECORD_DETAIL.c_str(), recordDetailInofo);

    std::vector<ProfileType::StringMap> abcFilePoolMessage;
    decoder.GetAbcFilePool()->GetPool()->ProcessToJson(abcFilePoolMessage);
    cJSON* abcFilePool = convert.ConvertStr(abcFilePoolMessage);
    cJSON_AddItemToObject(allMessage, DumpJsonUtils::ABC_FILE_POOL.c_str(), abcFilePool);

    char *data = cJSON_PrintUnformatted(allMessage);
    if (data == nullptr) {
        cJSON_Delete(allMessage);
        LOG_NO_TAG(ERROR) << "Failed to convert cJSON object to string";
        return 0;
    }
    g_result = std::string(data);
    cJSON_Delete(allMessage);
    cJSON_free(data);
    return g_result.length();
}

bool GetConvertResult(char* buffer, size_t length)
{
    if (!g_result.empty()) {
        if (memcpy_s(buffer, length, g_result.c_str(), g_result.length()) != EOK) {
            return false;
        }
        g_result.clear();
        return true;
    }
    return false;
}
} // namespace native
