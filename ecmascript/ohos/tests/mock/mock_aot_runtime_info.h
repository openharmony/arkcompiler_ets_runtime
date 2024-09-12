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

#ifndef MOCK_ECMASCRIPT_AOT_RUNTIME_INFO_H
#define MOCK_ECMASCRIPT_AOT_RUNTIME_INFO_H

#include "ecmascript/ohos/aot_runtime_info.h"

namespace panda::test {
class MockAotRuntimeInfo : public ecmascript::ohos::AotRuntimeInfo {
public:
    static constexpr const char *SANBOX_DIR = "ohos-crash-test";
    static constexpr const char *AOT_RUNTIME_INFO = "aot_runtime_info.log";
    MockAotRuntimeInfo();
    MockAotRuntimeInfo(bool isRuntimeInfoCrashTest) : isRuntimeInfoCrashTest_(isRuntimeInfoCrashTest) {}
    ~MockAotRuntimeInfo();

    bool GetRuntimeBuildId(char *buildId, int length) const override;
    bool GetMicrosecondsTimeStamp(char *timestamp, size_t length) const override;
    bool GetCrashSandBoxRealPath(char *realOutPath, size_t length) const override;

    bool BuildRuntimeInfoPart(char *runtimeInfoPart, const char *soBuildId, const char *timestamp,
        ecmascript::ohos::RuntimeInfoType type) const;
    void SetRuntimeInfo(const char *realOutPath, char lines[][BUFFER_SIZE], int length) const override;

protected:
    bool IsRuntimeInfoCrashTest() const
    {
        return isRuntimeInfoCrashTest_;
    }

    bool isRuntimeInfoCrashTest_ = false;
};
};  // namespace panda::test
#endif // MOCK_ECMASCRIPT_AOT_RUNTIME_INFO_H
