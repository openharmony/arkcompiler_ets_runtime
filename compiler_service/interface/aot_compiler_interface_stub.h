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

#ifndef OHOS_ARKCOMPILER_AOTCOMPILER_INTERFACE_STUB_H
#define OHOS_ARKCOMPILER_AOTCOMPILER_INTERFACE_STUB_H

#include <iremote_stub.h>
#include "iaot_compiler_interface.h"

namespace OHOS::ArkCompiler {
class AotCompilerInterfaceStub : public IRemoteStub<IAotCompilerInterface> {
public:
    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
                            MessageOption &option) override;
private:
    int32_t CommandAOTCompiler(MessageParcel &data, MessageParcel &reply);
    int32_t CommandStopAOTCompiler(MessageParcel &reply);
    int32_t CommandGetAOTVersion(MessageParcel& reply);
    int32_t CommandNeedReCompile(MessageParcel& data, MessageParcel& reply);
    static constexpr int32_t COMMAND_AOT_COMPILER = MIN_TRANSACTION_ID + 0;
    static constexpr int32_t COMMAND_STOP_AOT_COMPILER = MIN_TRANSACTION_ID + 1;
    static constexpr int32_t COMMAND_GET_AOT_VERSION = MIN_TRANSACTION_ID + 2;
    static constexpr int32_t COMMAND_NEED_RE_COMPILE = MIN_TRANSACTION_ID + 3;
};
} // namespace OHOS::ArkCompiler
#endif // OHOS_ARKCOMPILER_AOTCOMPILER_INTERFACE_STUB_H

