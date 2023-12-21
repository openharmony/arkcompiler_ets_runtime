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
#if defined(CODE_ENCRYPTION_ENABLE)
#include <sys/ioctl.h>
#include <sys/types.h>
#endif
#include "ecmascript/ohos/code_decrypt.h"

namespace panda::ecmascript::ohos {
#if defined(CODE_ENCRYPTION_ENABLE)
int DecryptSetKey(int fd, int srcAppId)
{
    struct code_decrypto_arg arg;
    arg.arg1_len = sizeof(srcAppId);
    arg.arg1 = reinterpret_cast<void*>(&srcAppId);
    arg.arg2_len = 0;
    arg.arg2 = nullptr;
    return ioctl(fd, CODE_DECRYPT_CMD_SET_KEY, &arg);
}

int DecrypRemoveKey(int fd, int srcAppId)
{
    struct code_decrypto_arg arg;
    arg.arg1_len = sizeof(srcAppId);
    arg.arg1 = reinterpret_cast<void*>(&srcAppId);
    arg.arg2_len = 0;
    arg.arg2 = nullptr;
    return ioctl(fd, CODE_DECRYPT_CMD_REMOVE_KEY, &arg);
}

int DecryptAssociateKey(int fd, int dstAppId, int srcAppId)
{
    struct code_decrypto_arg arg;
    arg.arg1_len = sizeof(dstAppId);
    arg.arg1 = reinterpret_cast<void*>(&dstAppId);
    arg.arg2_len = sizeof(srcAppId);
    arg.arg2 = reinterpret_cast<void*>(&srcAppId);
    return ioctl(fd, CODE_DECRYPT_CMD_SET_ASSOCIATE_KEY, &arg);
}

int DecrypRemoveAssociateKey(int fd, int dstAppId, int srcAppId)
{
    struct code_decrypto_arg arg;
    arg.arg1_len = sizeof(dstAppId);
    arg.arg1 = reinterpret_cast<void*>(&dstAppId);
    arg.arg2_len = sizeof(srcAppId);
    arg.arg2 = reinterpret_cast<void*>(&srcAppId);
    return ioctl(fd, CODE_DECRYPT_CMD_REMOVE_ASSOCIATE_KEY, &arg);
}
#endif
}  // namespace panda::ecmascript::ohos