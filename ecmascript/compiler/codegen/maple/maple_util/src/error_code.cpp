/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <error_code.h>

namespace maple {
void PrintErrorMessage(int ret)
{
    switch (ret) {
        case kErrorNoError:
        case kErrorExitHelp:
            break;
        case kErrorExit:
            ERR(kLncErr, "Error Exit!");
            break;
        case kErrorInvalidParameter:
            ERR(kLncErr, "Invalid Parameter!");
            break;
        case kErrorInitFail:
            ERR(kLncErr, "Init Fail!");
            break;
        case kErrorFileNotFound:
            ERR(kLncErr, "File Not Found!");
            break;
        case kErrorToolNotFound:
            ERR(kLncErr, "Tool Not Found!");
            break;
        case kErrorCompileFail:
            ERR(kLncErr, "Compile Fail!");
            break;
        case kErrorNotImplement:
            ERR(kLncErr, "Not Implement!");
            break;
        default:
            break;
    }
}
}  // namespace maple
