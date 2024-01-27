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
#ifndef ECMASCRIPT_COMPILER_APP_CRYPTO_H
#define ECMASCRIPT_COMPILER_APP_CRYPTO_H
#include "ecmascript/common.h"

namespace panda::ecmascript::ohos {
// After referencing the kernel header file, need to remove it
#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000
#endif
#define DEV_APP_CRYPTO_PATH "/dev/code_decrypt"

#define CODE_DECRYPT_CMD_SET_KEY  _IOW('c', 0x01, struct code_decrypto_arg)
#define CODE_DECRYPT_CMD_REMOVE_KEY  _IOW('c', 0x02, struct code_decrypto_arg)
#define CODE_DECRYPT_CMD_SET_ASSOCIATE_KEY  _IOW('c', 0x03, struct code_decrypto_arg)
#define CODE_DECRYPT_CMD_REMOVE_ASSOCIATE_KEY  _IOW('c', 0x04, struct code_decrypto_arg)
#define CODE_DECRYPT_CMD_CLEAR_CACHE_BY_KEY  _IOW('c', 0x05, struct code_decrypto_arg)
#define CODE_DECRYPT_CMD_CLEAR_CACHE_BY_FILE  _IOW('c', 0x06, struct code_decrypto_arg)
#define CODE_DECRYPT_CMD_CLEAR_SPECIFIC_CACHE  _IOW('c', 0x07, struct code_decrypto_arg)

struct code_decrypto_arg {
    int arg1_len;
    int arg2_len;
    void *arg1;
    void *arg2;
};
#if defined(CODE_ENCRYPTION_ENABLE)
int PUBLIC_API DecryptSetKey(int fd, int srcAppId);
int PUBLIC_API DecrypRemoveKey(int fd, int srcAppId);
int PUBLIC_API DecryptAssociateKey(int fd, int dstAppId, int srcAppId);
int DecrypRemoveAssociateKey(int fd, int dstAppId, int srcAppId);
#endif
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_APP_CRYPTO_H
