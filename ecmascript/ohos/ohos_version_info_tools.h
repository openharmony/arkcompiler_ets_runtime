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
#ifndef ECMASCRIPT_OHOS_VERSION_INFO_TOOLS_H
#define ECMASCRIPT_OHOS_VERSION_INFO_TOOLS_H

#if defined(OHOS_GET_PARAMETER)
#include "parameters.h"
#endif

namespace panda::ecmascript::ohos {
class OhosVersionInfoTools {
public:
    static CString GetRomVersion()
    {
#if defined(OHOS_GET_PARAMETER)
        return OHOS::system::GetParameter("const.product.software.version", "").c_str();
#endif
        return "";
    }
};
}
#endif // ECMASCRIPT_OHOS_VERSION_INFO_TOOLS_H
