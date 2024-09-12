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

#include "ecmascript/ohos/tests/mock/mock_aot_runtime_info.h"

namespace panda::test {
MockAotRuntimeInfo::MockAotRuntimeInfo()
{}
MockAotRuntimeInfo::~MockAotRuntimeInfo()
{}
bool MockAotRuntimeInfo::GetRuntimeBuildId(char *buildId, int length) const
{
    char tmp[NAME_MAX] = "abcd1234567890";
    if (strcpy_s(buildId, length, tmp) != 0) {
        return false;
    }
    return true;
}

bool MockAotRuntimeInfo::GetMicrosecondsTimeStamp(char *timestamp, size_t length) const
{
    char tmp[ecmascript::ohos::AotRuntimeInfo::TIME_STAMP_SIZE] = "1970-01-01 00:00:00";
    if (strcpy_s(timestamp, length, tmp) != 0) {
        return false;
    }
    return true;
}

bool MockAotRuntimeInfo::GetCrashSandBoxRealPath(char *realOutPath, size_t length) const
{
    if (strcpy_s(realOutPath, length, SANBOX_DIR) != 0) {
        return false;
    }
    if (strcat_s(realOutPath, length, "/") != 0) {
        return false;
    }
    if (strcat_s(realOutPath, length, AOT_RUNTIME_INFO) != 0) {
        return false;
    }
    return true;
}

bool MockAotRuntimeInfo::BuildRuntimeInfoPart(char *runtimeInfoPart, const char *soBuildId, const char *timestamp,
    ecmascript::ohos::RuntimeInfoType type) const
{
    return ecmascript::ohos::AotRuntimeInfo::BuildRuntimeInfoPart(runtimeInfoPart, soBuildId, timestamp, type);
}

void MockAotRuntimeInfo::SetRuntimeInfo(const char *realOutPath, char lines[][BUFFER_SIZE], int length) const
    {
        int fd = open(realOutPath,  O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd == -1) {
            return;
        }
        for (int i = 0; i < length && lines[i] != NULL; i++) {
            if (lines[i][0] != '\0') {
                write(fd, lines[i], strlen(lines[i]));
                if (IsRuntimeInfoCrashTest()) {
                    char buffer[BUFFER_SIZE];
                    memset_s(buffer, BUFFER_SIZE, 'a', BUFFER_SIZE);
                    write(fd, buffer, strlen(buffer));
                    continue;
                }
                write(fd, "\n", 1);
            }
        }
        close(fd);
    }
};
